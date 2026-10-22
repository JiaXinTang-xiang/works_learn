#include "Wireless_task.h"

/**************************************************************************
函数功能：Wireless初始化函数
函数说明：
        配置波特率的参数计算公式
        BRD = UART Clock / （OVS * Baudrate） = integerDivisor.X
        fractionalDivisor = （X*64）+0.5
入口参数：选择波特率，如（115200）
返回  值：无
**************************************************************************/
void Wireless_init(uint32_t baud)
{
    DL_UART_Main_enable(Wireless_INST);
    NVIC_ClearPendingIRQ(Wireless_INST_INT_IRQN);
    NVIC_EnableIRQ(Wireless_INST_INT_IRQN);
}

//串口发送单个字符
void Wireless_send_char(char ch)
{
    //当串口0忙的时候等待，不忙的时候再发送传进来的字符
    while( DL_UART_isBusy(Wireless_INST) == true );
    //发送单个字符
    DL_UART_Main_transmitData(Wireless_INST, ch);
}
//串口发送字符串
void Wireless_send_string(char* str)
{
    //当前字符串地址不在结尾 并且 字符串首地址不为空
    while(*str!=0&&str!=0)
    {
        //发送字符串首地址中的字符，并且在发送完成之后首地址自增
        Wireless_send_char(*str++);
    }
}


void Wireless_send_SendByte(uint8_t* data, uint32_t len)
{
    for(uint32_t i = 0; i < len; i++)
    {
        Wireless_send_char(data[i]);  // 直接发送原始字节
    }
}


/*
函数功能：UART5中断服务函数
*/

void Wireless_INST_IRQHandler(void)
{
    uint32_t iidx = DL_UART_Main_getPendingInterrupt(UART5);

    if (iidx == DL_UART_MAIN_IIDX_RX)           /* 接收到数据 */
    {
        uint8_t res = DL_UART_receiveData(UART5);
        if (res == '1')
        {
		
		}
    }
}
