#include "Vision_task.h"

uint8_t rx_buffer[6];        /* UART接收缓冲, 视觉模块数据包 */
extern uint8_t RxState;		//定义表示当前状态机状态的静态变量

extern uint16_t center_x;
extern uint16_t center_y;
extern uint16_t area_value;

uint8_t Vison_RxFlag;					//定义接收数据包标志位

void Vision_init(void) {

//	DL_UART_enable(UART_Vision_INST);
//    DL_UART_enableInterrupt(UART_Vision_INST, DL_UART_INTERRUPT_RX);
//	//清除串口中断标志
	NVIC_ClearPendingIRQ(UART_Vision_INST_INT_IRQN);
	NVIC_EnableIRQ(UART_Vision_INST_INT_IRQN);
	DL_UART_Main_receiveData(UART_Vision_INST);
}

/**
  * 函    数：获取串口接收数据包标志位
  * 参    数：无
  * 返 回 值：串口接收数据包标志位，范围：0~1，接收到数据包后，标志位置1，读取后标志位自动清零
  */
uint8_t Vison_GetRxFlag(void)
{
	if (Vison_RxFlag == 1)			//如果标志位为1
	{
		Vison_RxFlag = 0;
		return 1;					//则返回1，并自动清零标志位
	}
	return 0;						//如果标志位为0，则返回0
}

void Vison_Task(uint16_t *X, uint16_t *Y, uint16_t *Area)
{	            
		*X = (rx_buffer[0] << 8) | rx_buffer[1];
		*Y = (rx_buffer[2] << 8) | rx_buffer[3];
		*Area = (rx_buffer[4] << 8) | rx_buffer[5];    
		RxState = 0;
		memset(rx_buffer, 0, 6);
}


int i;

void UART_Vision_INST_IRQHandler(void) 
{
	static uint32_t RxState = 0;
	static uint32_t pRxPacket = 0;
	
	// 如果产生了串口中断
    if (DL_UART_getPendingInterrupt(UART_Vision_INST)){
		DL_UART_clearInterruptStatus(UART_Vision_INST, DL_UART_INTERRUPT_RX); 
		uint8_t RxData = DL_UART_receiveData(UART_Vision_INST);
		
		/*当前状态为0，接收数据包包头*/
		if (RxState == 0)
		{
			if (RxData == 0xAA)			//如果数据确实是包头
			{
				RxState = 1;			//置下一个状态
			}
		}
		else if(RxState == 1)
		{
			if (RxData == 0x06)			//如果数据是第二个校验位
			{
				RxState = 2;			//置下一个状态
				pRxPacket = 0;			//数据包的位置2
			}
		}
		/*当前状态为2，接收数据包数据*/
		else if (RxState == 2)
		{
			rx_buffer[pRxPacket] = RxData;	//将数据存入数据包数组的指定位置
			pRxPacket ++;					//数据包的位置自增
			if (pRxPacket >= 6)				//如果收够6个数据
			{
				RxState = 3;				//置下一个状态
			}
		}
		/*当前状态为3，接收数据包包尾*/
		else if (RxState == 3)
		{
			if (RxData == 0x55)			//如果数据确实是包尾部
			{
				Vison_Task(&center_x, &center_y, &area_value);
				RxState = 0;			//状态归0
				Vison_RxFlag = 1;		//接收数据包标志位置1，成功接收一个数据包
			}
		}
		
		i+=1;
	
	}
}

