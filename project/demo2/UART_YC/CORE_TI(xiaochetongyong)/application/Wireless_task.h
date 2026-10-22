#ifndef __WIRELESS_TASK_H
#define __WIRELESS_TASK_H

#include "bsp.h"

void Wireless_init(uint32_t baud);
void Wireless_send_char(char ch);
void Wireless_send_string(char* str);
void Wireless_send_SendByte(uint8_t* data, uint32_t len);
#endif
