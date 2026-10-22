#include "bsp_key.h"

//void key_init(void)
//{
//    NVIC_EnableIRQ(key_INT_IRQN); // 开启中断
//}

uint8_t Key_Num;

void GROUP1_IRQHandler(void)
{
}

uint8_t Key_GetNum(void)
{
    uint8_t Temp;
    if (Key_Num)
    {
        Temp = Key_Num;
        Key_Num = 0;
        return Temp;
    }
    return 0;
}

uint8_t Key_GetState()
{
    if (DL_GPIO_readPins(key_PORT, key_user_PIN) == 0)
    {
        return 1;
    }
    if (DL_GPIO_readPins(key_PORT, key_PIN_6_PIN) == 0)
    {
        return 2;
    }
    if (DL_GPIO_readPins(key_PORT, key_PIN_7_PIN) == 0)
    {
        return 3;
    }
//		if (DL_GPIO_readPins(key_PORT, key_PIN_6_PIN) == 0)
//    {
//        return 4;
//    }
//		if (DL_GPIO_readPins(key_PORT, key_PIN_7_PIN) == 0)
//    {
//        return 5;
//    }
    return 0;
}

void Key_Tick(void)
{
    static uint8_t Count;
    static uint8_t CurrState;
    static uint8_t PrevState;

    Count++;
    if (Count >= 1)
    {
        Count = 0;
        PrevState = CurrState;
        CurrState = Key_GetState();
        if (CurrState == 0 && PrevState != 0)
        {
            Key_Num = PrevState;
        }
    }
}
