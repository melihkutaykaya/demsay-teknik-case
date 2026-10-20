/* Saat ayari, LED ve hata yakalayici */
#ifndef BSP_H
#define BSP_H

#include <stdbool.h>

void BSP_Init(void);
void BSP_LedSet(bool on);
void BSP_LedToggle(void);
void Error_Handler(void);

#endif
