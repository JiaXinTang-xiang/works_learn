#ifndef __VSION_TASK_H
#define __VSION_TASK_H

#include "bsp.h"

void Vision_init(void);
void Vison_Task(uint16_t *X, uint16_t *Y, uint16_t *Area);
void UART_Vision_INST_IRQHandler(void);

#endif
