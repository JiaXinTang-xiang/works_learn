#include "TypecUART_task.h"

/**************************************************************************
函数功能：重定义fputc/fputs函数
函数说明：用于重定义fputc/fputs函数，由于fputc/fputs函数仅有一个，所以无法
实现多个串口均用printf打印，所以本工程仅USART0使用printf，若有需要自行更改。
入口参数：略
返回  值：略
其    他：如果发现printf可以输出一般字符，而无法输出参数，
需要重定向fputs的同时，再重定向puts
**************************************************************************/
int fputc(int ch, FILE *f)
{
    /* 发送一个数据 */
    DL_UART_Main_transmitData(UART0, (uint8_t)ch);
    /* 等待数据传输完毕 */
    while (DL_UART_Main_isBusy(UART0))
        ;

    return ch;
}

int fputs(const char *_ptr, register FILE *_fp)
{
    uint16_t i, len;
    len = strlen(_ptr);
    for (i = 0; i < len; i++)
    {
        /* 发送一个数据 */
        DL_UART_Main_transmitData(UART0, (uint8_t)_ptr[i]);
        /* 等待数据传输完毕 */
        while (DL_UART_Main_isBusy(UART0))
            ;
    }

    return len;
}

/**************************************************************************
函数功能：UART0初始化函数
函数说明：
        配置波特率的参数计算公式
        BRD = UART Clock / （OVS * Baudrate） = integerDivisor.X
        fractionalDivisor = （X*64）+0.5
入口参数：选择波特率，目前只有9600
返回  值：无
**************************************************************************/
void uart0_init(uint32_t baud)
{
    DL_UART_Main_enable(TypeC_INST);
    NVIC_ClearPendingIRQ(TypeC_INST_INT_IRQN);
    NVIC_EnableIRQ(TypeC_INST_INT_IRQN);
}



//串口发送单个字符
void uart0_send_char(char ch)
{
    //当串口0忙的时候等待，不忙的时候再发送传进来的字符
    while( DL_UART_isBusy(TypeC_INST) == true );
    //发送单个字符
    DL_UART_Main_transmitData(TypeC_INST, ch);
}
//串口发送字符串
void uart0_send_string(char* str)
{
    //当前字符串地址不在结尾 并且 字符串首地址不为空
    while(*str!=0&&str!=0)
    {
        //发送字符串首地址中的字符，并且在发送完成之后首地址自增
        uart0_send_char(*str++);
    }
}


void uart0_send_SendByte(uint8_t* data, uint32_t len)
{
    for(uint32_t i = 0; i < len; i++)
    {
        uart0_send_char(data[i]);  // 直接发送原始字节
    }
}


/**************************************************************************
函数功能：UART0中断服务函数
说明：
  - RX 中断：接收数据并处理指令，使用 uart_send_string 非阻塞回复
  - TX 中断：从环形缓冲区取下一字节发送，发完后自动关闭 TX 中断
  - 整个 ISR 执行时间极短（微秒级），不会阻塞 WS2812 定时器 ISR
**************************************************************************/

void UART0_IRQHandler(void)
{
    uint32_t iidx = DL_UART_Main_getPendingInterrupt(UART0);

    if (iidx == DL_UART_MAIN_IIDX_RX)           /* 接收到数据 */
    {
        uint8_t res = DL_UART_receiveData(UART0);
        if (res == '1')
        {
            set_RGB(254, 0, 0);
            FMQ_Status(1);
            uart0_send_string("ok\r\n");
        }
        else if (res == '2')
        {
            set_RGB(0, 255, 0);
            FMQ_Status(1);
            uart0_send_string("ok\r\n");
        }
        else if (res == '3')
        {
            set_RGB(0, 0, 255);
            FMQ_Status(1);
            uart0_send_string("ok\r\n");
        }
        else if (res == '0')
        {
            set_RGB(0, 0, 0);
            FMQ_Status(0);
            uart0_send_string("ok\r\n");
        }
    }
}

/**************************************************************************
函数功能：浮点转字符串
函数说明：无
入口参数：value ->浮点数  *str ->字符串  precision ->保留有效数字位数
返回  值：无
**************************************************************************/
void doubleToStr(double value, char *str, int precision)
{
    long wholePart = (long)value;
    double fractionalPart = fmod(value, 1.0);
    /* 使用sprintf函数将整数部分和一个小数点写入字符串 */
    sprintf(str, "%ld.", wholePart);
    str += strlen(str);
    for (int i = 0; i < precision; ++i)
    {
        fractionalPart *= 10;
        long digit = (long)fractionalPart;
        sprintf(str, "%ld", digit);
        str += strlen(str);
        fractionalPart -= digit;
    }
    *str = '\0';
}
