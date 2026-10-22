#include "drv_can.h"

extern QD4310_t Motor_0; // 电机结构体变量

uint32_t gInterruptLine1Status_CAN = 0; // CAN一些状态类变量初始化
bool gServiceInt_CAN = false;   
volatile uint32_t RxID_CAN;
volatile uint8_t  RxLength_CAN;
volatile uint8_t  RxDataCAN[8];

DL_MCAN_RxBufElement rxMsg;
DL_MCAN_RxFIFOStatus rxFS;

// 数据解析函数
static void processRxMsg(DL_MCAN_RxBufElement *rxMsg)
{
    uint32_t id;

    // 标准ID提取（官方写法）
    id = ((rxMsg->id & (uint32_t)0x1FFC0000) >> (uint32_t)18);

    // 更新显示变量
    RxID_CAN     = id;
    RxLength_CAN = rxMsg->dlc;
    for(uint8_t n=0; n<RxLength_CAN; n++){
        RxDataCAN[n] = rxMsg->data[n];
    }
    // 电机数据解析
    if(RxID_CAN == 0x500 || RxID_CAN == 0x501)
    {
        if( (RxID_CAN & 0xFF) == Motor_0.id )
        {
            QD4310_Update(&Motor_0, rxMsg->data);
        }
    }
}

// 中断函数	(目前用不了 收不到数据不知道为啥)
void CANFD1_IRQHandler(void)
{   
    switch (DL_MCAN_getPendingInterrupt(MCAN0_INST))
    {
        case DL_MCAN_IIDX_LINE1:
            gInterruptLine1Status_CAN |= DL_MCAN_getIntrStatus(MCAN0_INST);
            DL_MCAN_clearIntrStatus(MCAN0_INST, gInterruptLine1Status_CAN, DL_MCAN_INTR_SRC_MCAN_LINE_1);
            
            if ((gInterruptLine1Status_CAN & MCAN_IR_RF0N_MASK) == MCAN_IR_RF0N_MASK)
            {
                // 先获取FIFO状态
                DL_MCAN_getRxFIFOStatus(MCAN0_INST, &rxFS);
                
                // 检查FIFO是否有数据
                if(rxFS.fillLvl != 0)
                {
                    // 从FIFO读取数据
                    DL_MCAN_readMsgRam(MCAN0_INST, DL_MCAN_MEM_TYPE_FIFO, 0, rxFS.num, &rxMsg);
                    
                    // 释放FIFO
                    DL_MCAN_writeRxFIFOAck(MCAN0_INST, rxFS.num, rxFS.getIdx);
                    
                    // 解析数据
                    processRxMsg(&rxMsg);
                }
                
                // 清除中断标志
                gInterruptLine1Status_CAN &= ~(MCAN_IR_RF0N_MASK);
            }
            break;

        default:
            break;
    }
}

