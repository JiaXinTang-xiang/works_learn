#ifndef __TYPECUART_TASK_H
#define __TYPECUART_TASK_H

#include "bsp.h"

void uart0_init(uint32_t baud);
void doubleToStr(double value, char *str, int precision);

/* 非阻塞 UART 发送（ISR 安全） */
void uart_send_string(const char *str);
void uart_send_data(const uint8_t *data, uint16_t len);

#endif
