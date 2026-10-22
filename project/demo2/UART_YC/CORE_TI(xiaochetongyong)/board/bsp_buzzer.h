#ifndef __BSP_BUZZER_H
#define __BSP_BUZZER_H

#include "bsp.h"


/*控制LED1高低电平*/
#define FMQ_Status(en)                                      \
    if (en)                                           \
        DL_GPIO_setPins(FMQ_PORT, FMQ_BBB_PIN);        \
    else                                              \
        DL_GPIO_clearPins(FMQ_PORT, FMQ_BBB_PIN);

/*控制LED1反转*/
#define FMQ_toggle DL_GPIO_togglePins(FMQ_PORT, FMQ_BBB_PIN);

void FMQ_init(void);

#endif
