/* USART2 + DMA ile bloklamayan gonderim.
 * Write veriyi halka tampona yazar; DMA tamponu arka planda gonderir. */
#include "uart_tx.h"
#include <stdarg.h>
#include <stdio.h>
#include "board.h"
#include "bsp.h"

static UART_HandleTypeDef s_huart;
static DMA_HandleTypeDef  s_hdma_tx;

static uint8_t           s_buf[UART_TX_BUFFER_SIZE];
static volatile uint32_t s_head;      /* yazma indeksi (ana dongu) */
static volatile uint32_t s_tail;      /* gonderme indeksi (kesme) */
static volatile uint32_t s_dma_len;   /* DMA'daki bayt sayisi, 0 = bosta */
static volatile uint32_t s_dropped;

static void StartNextChunk(void);

void UartTx_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    UART_GPIO_CLK_ENABLE();
    UART_CLK_ENABLE();
    UART_DMA_CLK_ENABLE();

    gpio.Pin       = UART_TX_PIN | UART_RX_PIN;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = UART_GPIO_AF;
    HAL_GPIO_Init(UART_GPIO_PORT, &gpio);

    s_hdma_tx.Instance                 = UART_TX_DMA_STREAM;
    s_hdma_tx.Init.Channel             = UART_TX_DMA_CHANNEL;
    s_hdma_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    s_hdma_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    s_hdma_tx.Init.MemInc              = DMA_MINC_ENABLE;
    s_hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    s_hdma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    s_hdma_tx.Init.Mode                = DMA_NORMAL;
    s_hdma_tx.Init.Priority            = DMA_PRIORITY_LOW;
    s_hdma_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&s_hdma_tx) != HAL_OK) {
        Error_Handler();
    }
    __HAL_LINKDMA(&s_huart, hdmatx, s_hdma_tx);

    s_huart.Instance          = UART_INSTANCE;
    s_huart.Init.BaudRate     = UART_BAUDRATE;
    s_huart.Init.WordLength   = UART_WORDLENGTH_8B;
    s_huart.Init.StopBits     = UART_STOPBITS_1;
    s_huart.Init.Parity       = UART_PARITY_NONE;
    s_huart.Init.Mode         = UART_MODE_TX_RX;
    s_huart.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    s_huart.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&s_huart) != HAL_OK) {
        Error_Handler();
    }

    HAL_NVIC_SetPriority(UART_TX_DMA_IRQn, IRQ_PRIO_UART, 0U);
    HAL_NVIC_EnableIRQ(UART_TX_DMA_IRQn);
    HAL_NVIC_SetPriority(UART_IRQn, IRQ_PRIO_UART, 0U);
    HAL_NVIC_EnableIRQ(UART_IRQn);
}

/* Tampon doluysa sigmayan kisim atilir */
size_t UartTx_Write(const char *data, size_t len)
{
    uint32_t head = s_head;
    uint32_t free_space = (s_tail + UART_TX_BUFFER_SIZE - head - 1U) % UART_TX_BUFFER_SIZE;
    size_t   n = (len < free_space) ? len : free_space;

    for (size_t i = 0U; i < n; i++) {
        s_buf[head] = (uint8_t)data[i];
        head = (head + 1U) % UART_TX_BUFFER_SIZE;
    }
    s_head = head;

    if (n < len) {
        s_dropped += (uint32_t)(len - n);
    }

    StartNextChunk();
    return n;
}

size_t UartTx_Printf(const char *fmt, ...)
{
    char    line[128];
    va_list args;
    int     len;

    va_start(args, fmt);
    len = vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    if (len <= 0) {
        return 0U;
    }
    if ((size_t)len >= sizeof(line)) {
        len = (int)sizeof(line) - 1;
    }
    return UartTx_Write(line, (size_t)len);
}

uint32_t UartTx_DroppedBytes(void)
{
    return s_dropped;
}

/* DMA bostaysa tampondaki bir sonraki parcayi gonder.
 * Hem ana donguden hem kesmeden cagrildigi icin kesmeler kapali calisir. */
static void StartNextChunk(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    if ((s_dma_len == 0U) && (s_head != s_tail)) {
        uint32_t head = s_head;
        uint32_t tail = s_tail;
        uint32_t len  = (head > tail) ? (head - tail) : (UART_TX_BUFFER_SIZE - tail);

        s_dma_len = len;
        if (HAL_UART_Transmit_DMA(&s_huart, &s_buf[tail], (uint16_t)len) != HAL_OK) {
            s_dma_len = 0U;
        }
    }

    __set_PRIMASK(primask);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == UART_INSTANCE) {
        s_tail    = (s_tail + s_dma_len) % UART_TX_BUFFER_SIZE;
        s_dma_len = 0U;
        StartNextChunk();
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    HAL_UART_TxCpltCallback(huart);
}

void UartTx_UartIRQHandler(void)
{
    HAL_UART_IRQHandler(&s_huart);
}

void UartTx_DmaIRQHandler(void)
{
    HAL_DMA_IRQHandler(&s_hdma_tx);
}
