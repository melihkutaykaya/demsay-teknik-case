/* Saat ayari, LED ve hata yakalayici */
#include "bsp.h"
#include "board.h"

static void SystemClock_Config(void);
static void Led_Init(void);

void BSP_Init(void)
{
    if (HAL_Init() != HAL_OK) {
        Error_Handler();
    }
    SystemClock_Config();
    Led_Init();

    /* __WFI() sirasinda debugger baglantisi kopmasin */
    HAL_DBGMCU_EnableDBGSleepMode();
}

void BSP_LedSet(bool on)
{
    HAL_GPIO_WritePin(LED_GPIO_PORT, LED_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void BSP_LedToggle(void)
{
    HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_PIN);
}

/* HSI 16 MHz -> PLL -> 84 MHz. APB1 = 42 MHz, APB2 = 84 MHz, timer'lar 84 MHz. */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    osc.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState            = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState        = RCC_PLL_ON;
    osc.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    osc.PLL.PLLM            = 16U;
    osc.PLL.PLLN            = 336U;
    osc.PLL.PLLP            = RCC_PLLP_DIV4;
    osc.PLL.PLLQ            = 7U;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        Error_Handler();
    }

    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                         RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

static void Led_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    LED_GPIO_CLK_ENABLE();

    gpio.Pin   = LED_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_PORT, &gpio);
}

/* Kurtarilamaz hata: LED hizli yanip soner */
void Error_Handler(void)
{
    __disable_irq();
    for (;;) {
        for (volatile uint32_t i = 0U; i < 1000000U; i++) {
        }
        LED_GPIO_PORT->ODR ^= LED_PIN;
    }
}
