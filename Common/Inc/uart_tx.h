/* USART2 + DMA ile bloklamayan gonderim (halka tampon) */
#ifndef UART_TX_H
#define UART_TX_H

#include <stddef.h>
#include <stdint.h>

#define UART_TX_BUFFER_SIZE   512U

void     UartTx_Init(void);
size_t   UartTx_Write(const char *data, size_t len);
size_t   UartTx_Printf(const char *fmt, ...);
uint32_t UartTx_DroppedBytes(void);

void UartTx_UartIRQHandler(void);
void UartTx_DmaIRQHandler(void);

#endif
