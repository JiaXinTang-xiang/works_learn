/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G351X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G351X
#define CONFIG_MSPM0G3519

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)


#define GPIO_HFXT_PORT                                                     GPIOA
#define GPIO_HFXIN_PIN                                             DL_GPIO_PIN_5
#define GPIO_HFXIN_IOMUX                                         (IOMUX_PINCM10)
#define GPIO_HFXOUT_PIN                                            DL_GPIO_PIN_6
#define GPIO_HFXOUT_IOMUX                                        (IOMUX_PINCM11)
#define CPUCLK_FREQ                                                     80000000
/* Defines for SYSPLL_ERR_01 Workaround */
/* Represent 1.000 as 1000 */
#define FLOAT_TO_INT_SCALE                                               (1000U)
#define FCC_EXPECTED_RATIO                                                  2000
#define FCC_UPPER_BOUND                       (FCC_EXPECTED_RATIO * (1 + 0.003))
#define FCC_LOWER_BOUND                       (FCC_EXPECTED_RATIO * (1 - 0.003))

bool SYSCFG_DL_SYSCTL_SYSPLL_init(void);


/* Defines for TimerG6_PWM */
#define TimerG6_PWM_INST                                                   TIMG6
#define TimerG6_PWM_INST_IRQHandler                             TIMG6_IRQHandler
#define TimerG6_PWM_INST_INT_IRQN                               (TIMG6_INT_IRQn)
#define TimerG6_PWM_INST_CLK_FREQ                                       80000000
/* GPIO defines for channel 0 */
#define GPIO_TimerG6_PWM_C0_PORT                                           GPIOA
#define GPIO_TimerG6_PWM_C0_PIN                                   DL_GPIO_PIN_29
#define GPIO_TimerG6_PWM_C0_IOMUX                                 (IOMUX_PINCM4)
#define GPIO_TimerG6_PWM_C0_IOMUX_FUNC                IOMUX_PINCM4_PF_TIMG6_CCP0
#define GPIO_TimerG6_PWM_C0_IDX                              DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_TimerG6_PWM_C1_PORT                                           GPIOA
#define GPIO_TimerG6_PWM_C1_PIN                                   DL_GPIO_PIN_30
#define GPIO_TimerG6_PWM_C1_IOMUX                                 (IOMUX_PINCM5)
#define GPIO_TimerG6_PWM_C1_IOMUX_FUNC                IOMUX_PINCM5_PF_TIMG6_CCP1
#define GPIO_TimerG6_PWM_C1_IDX                              DL_TIMER_CC_1_INDEX

/* Defines for PWM_0 */
#define PWM_0_INST                                                         TIMG7
#define PWM_0_INST_IRQHandler                                   TIMG7_IRQHandler
#define PWM_0_INST_INT_IRQN                                     (TIMG7_INT_IRQn)
#define PWM_0_INST_CLK_FREQ                                              5000000
/* GPIO defines for channel 0 */
#define GPIO_PWM_0_C0_PORT                                                 GPIOA
#define GPIO_PWM_0_C0_PIN                                         DL_GPIO_PIN_28
#define GPIO_PWM_0_C0_IOMUX                                       (IOMUX_PINCM3)
#define GPIO_PWM_0_C0_IOMUX_FUNC                      IOMUX_PINCM3_PF_TIMG7_CCP0
#define GPIO_PWM_0_C0_IDX                                    DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_PWM_0_C1_PORT                                                 GPIOA
#define GPIO_PWM_0_C1_PIN                                         DL_GPIO_PIN_31
#define GPIO_PWM_0_C1_IOMUX                                       (IOMUX_PINCM6)
#define GPIO_PWM_0_C1_IOMUX_FUNC                      IOMUX_PINCM6_PF_TIMG7_CCP1
#define GPIO_PWM_0_C1_IDX                                    DL_TIMER_CC_1_INDEX




/* Defines for QEI_0 */
#define QEI_0_INST                                                         TIMG9
#define QEI_0_INST_IRQHandler                                   TIMG9_IRQHandler
#define QEI_0_INST_INT_IRQN                                     (TIMG9_INT_IRQn)
/* Pin configuration defines for QEI_0 PHA Pin */
#define GPIO_QEI_0_PHA_PORT                                                GPIOB
#define GPIO_QEI_0_PHA_PIN                                        DL_GPIO_PIN_29
#define GPIO_QEI_0_PHA_IOMUX                                     (IOMUX_PINCM66)
#define GPIO_QEI_0_PHA_IOMUX_FUNC                    IOMUX_PINCM66_PF_TIMG9_CCP0
/* Pin configuration defines for QEI_0 PHB Pin */
#define GPIO_QEI_0_PHB_PORT                                                GPIOB
#define GPIO_QEI_0_PHB_PIN                                        DL_GPIO_PIN_30
#define GPIO_QEI_0_PHB_IOMUX                                     (IOMUX_PINCM67)
#define GPIO_QEI_0_PHB_IOMUX_FUNC                    IOMUX_PINCM67_PF_TIMG9_CCP1

/* Defines for QEI_1 */
#define QEI_1_INST                                                         TIMG8
#define QEI_1_INST_IRQHandler                                   TIMG8_IRQHandler
#define QEI_1_INST_INT_IRQN                                     (TIMG8_INT_IRQn)
/* Pin configuration defines for QEI_1 PHA Pin */
#define GPIO_QEI_1_PHA_PORT                                                GPIOA
#define GPIO_QEI_1_PHA_PIN                                        DL_GPIO_PIN_26
#define GPIO_QEI_1_PHA_IOMUX                                     (IOMUX_PINCM59)
#define GPIO_QEI_1_PHA_IOMUX_FUNC                    IOMUX_PINCM59_PF_TIMG8_CCP0
/* Pin configuration defines for QEI_1 PHB Pin */
#define GPIO_QEI_1_PHB_PORT                                                GPIOA
#define GPIO_QEI_1_PHB_PIN                                        DL_GPIO_PIN_27
#define GPIO_QEI_1_PHB_IOMUX                                     (IOMUX_PINCM60)
#define GPIO_QEI_1_PHB_IOMUX_FUNC                    IOMUX_PINCM60_PF_TIMG8_CCP1


/* Defines for TimerA1 */
#define TimerA1_INST                                                     (TIMA1)
#define TimerA1_INST_IRQHandler                                 TIMA1_IRQHandler
#define TimerA1_INST_INT_IRQN                                   (TIMA1_INT_IRQn)
#define TimerA1_INST_LOAD_VALUE                                             (9U)



/* Defines for TypeC */
#define TypeC_INST                                                         UART0
#define TypeC_INST_FREQUENCY                                            20000000
#define TypeC_INST_IRQHandler                                   UART0_IRQHandler
#define TypeC_INST_INT_IRQN                                       UART0_INT_IRQn
#define GPIO_TypeC_RX_PORT                                                 GPIOA
#define GPIO_TypeC_TX_PORT                                                 GPIOA
#define GPIO_TypeC_RX_PIN                                         DL_GPIO_PIN_11
#define GPIO_TypeC_TX_PIN                                         DL_GPIO_PIN_10
#define GPIO_TypeC_IOMUX_RX                                      (IOMUX_PINCM22)
#define GPIO_TypeC_IOMUX_TX                                      (IOMUX_PINCM21)
#define GPIO_TypeC_IOMUX_RX_FUNC                       IOMUX_PINCM22_PF_UART0_RX
#define GPIO_TypeC_IOMUX_TX_FUNC                       IOMUX_PINCM21_PF_UART0_TX
#define TypeC_BAUD_RATE                                                 (115200)
#define TypeC_IBRD_20_MHZ_115200_BAUD                                       (10)
#define TypeC_FBRD_20_MHZ_115200_BAUD                                       (54)
/* Defines for UART_4 */
#define UART_4_INST                                                        UART7
#define UART_4_INST_FREQUENCY                                           40000000
#define UART_4_INST_IRQHandler                                  UART7_IRQHandler
#define UART_4_INST_INT_IRQN                                      UART7_INT_IRQn
#define GPIO_UART_4_RX_PORT                                                GPIOB
#define GPIO_UART_4_TX_PORT                                                GPIOB
#define GPIO_UART_4_RX_PIN                                        DL_GPIO_PIN_18
#define GPIO_UART_4_TX_PIN                                        DL_GPIO_PIN_17
#define GPIO_UART_4_IOMUX_RX                                     (IOMUX_PINCM44)
#define GPIO_UART_4_IOMUX_TX                                     (IOMUX_PINCM43)
#define GPIO_UART_4_IOMUX_RX_FUNC                      IOMUX_PINCM44_PF_UART7_RX
#define GPIO_UART_4_IOMUX_TX_FUNC                      IOMUX_PINCM43_PF_UART7_TX
#define UART_4_BAUD_RATE                                                (115200)
#define UART_4_IBRD_40_MHZ_115200_BAUD                                      (21)
#define UART_4_FBRD_40_MHZ_115200_BAUD                                      (45)
/* Defines for UART_Vision */
#define UART_Vision_INST                                                   UART4
#define UART_Vision_INST_FREQUENCY                                      80000000
#define UART_Vision_INST_IRQHandler                             UART4_IRQHandler
#define UART_Vision_INST_INT_IRQN                                 UART4_INT_IRQn
#define GPIO_UART_Vision_RX_PORT                                           GPIOB
#define GPIO_UART_Vision_TX_PORT                                           GPIOB
#define GPIO_UART_Vision_RX_PIN                                   DL_GPIO_PIN_11
#define GPIO_UART_Vision_TX_PIN                                   DL_GPIO_PIN_10
#define GPIO_UART_Vision_IOMUX_RX                                (IOMUX_PINCM28)
#define GPIO_UART_Vision_IOMUX_TX                                (IOMUX_PINCM27)
#define GPIO_UART_Vision_IOMUX_RX_FUNC                 IOMUX_PINCM28_PF_UART4_RX
#define GPIO_UART_Vision_IOMUX_TX_FUNC                 IOMUX_PINCM27_PF_UART4_TX
#define UART_Vision_BAUD_RATE                                             (9600)
#define UART_Vision_IBRD_80_MHZ_9600_BAUD                                  (520)
#define UART_Vision_FBRD_80_MHZ_9600_BAUD                                   (53)
/* Defines for Wireless */
#define Wireless_INST                                                      UART5
#define Wireless_INST_FREQUENCY                                         80000000
#define Wireless_INST_IRQHandler                                UART5_IRQHandler
#define Wireless_INST_INT_IRQN                                    UART5_INT_IRQn
#define GPIO_Wireless_RX_PORT                                              GPIOA
#define GPIO_Wireless_TX_PORT                                              GPIOA
#define GPIO_Wireless_RX_PIN                                       DL_GPIO_PIN_0
#define GPIO_Wireless_TX_PIN                                       DL_GPIO_PIN_1
#define GPIO_Wireless_IOMUX_RX                                    (IOMUX_PINCM1)
#define GPIO_Wireless_IOMUX_TX                                    (IOMUX_PINCM2)
#define GPIO_Wireless_IOMUX_RX_FUNC                     IOMUX_PINCM1_PF_UART5_RX
#define GPIO_Wireless_IOMUX_TX_FUNC                     IOMUX_PINCM2_PF_UART5_TX
#define Wireless_BAUD_RATE                                              (115200)
#define Wireless_IBRD_80_MHZ_115200_BAUD                                    (43)
#define Wireless_FBRD_80_MHZ_115200_BAUD                                    (26)
/* Defines for Communication */
#define Communication_INST                                                 UART1
#define Communication_INST_FREQUENCY                                    40000000
#define Communication_INST_IRQHandler                           UART1_IRQHandler
#define Communication_INST_INT_IRQN                               UART1_INT_IRQn
#define GPIO_Communication_RX_PORT                                         GPIOB
#define GPIO_Communication_TX_PORT                                         GPIOB
#define GPIO_Communication_RX_PIN                                  DL_GPIO_PIN_5
#define GPIO_Communication_TX_PIN                                  DL_GPIO_PIN_4
#define GPIO_Communication_IOMUX_RX                              (IOMUX_PINCM18)
#define GPIO_Communication_IOMUX_TX                              (IOMUX_PINCM17)
#define GPIO_Communication_IOMUX_RX_FUNC               IOMUX_PINCM18_PF_UART1_RX
#define GPIO_Communication_IOMUX_TX_FUNC               IOMUX_PINCM17_PF_UART1_TX
#define Communication_BAUD_RATE                                         (115200)
#define Communication_IBRD_40_MHZ_115200_BAUD                               (21)
#define Communication_FBRD_40_MHZ_115200_BAUD                               (45)




/* Defines for SPI_1 */
#define SPI_1_INST                                                         SPI1
#define SPI_1_INST_IRQHandler                                   SPI1_IRQHandler
#define SPI_1_INST_INT_IRQN                                       SPI1_INT_IRQn
#define GPIO_SPI_1_PICO_PORT                                              GPIOB
#define GPIO_SPI_1_PICO_PIN                                      DL_GPIO_PIN_15
#define GPIO_SPI_1_IOMUX_PICO                                   (IOMUX_PINCM32)
#define GPIO_SPI_1_IOMUX_PICO_FUNC                   IOMUX_PINCM32_PF_SPI1_PICO
#define GPIO_SPI_1_POCI_PORT                                              GPIOB
#define GPIO_SPI_1_POCI_PIN                                      DL_GPIO_PIN_14
#define GPIO_SPI_1_IOMUX_POCI                                   (IOMUX_PINCM31)
#define GPIO_SPI_1_IOMUX_POCI_FUNC                   IOMUX_PINCM31_PF_SPI1_POCI
/* GPIO configuration for SPI_1 */
#define GPIO_SPI_1_SCLK_PORT                                              GPIOB
#define GPIO_SPI_1_SCLK_PIN                                      DL_GPIO_PIN_16
#define GPIO_SPI_1_IOMUX_SCLK                                   (IOMUX_PINCM33)
#define GPIO_SPI_1_IOMUX_SCLK_FUNC                   IOMUX_PINCM33_PF_SPI1_SCLK


/* Defines for VREF */
#define VREF_VOLTAGE_MV                                                     3300
#define GPIO_VREF_VREFPOS_PORT                                             GPIOA
#define GPIO_VREF_VREFPOS_PIN                                     DL_GPIO_PIN_23
#define GPIO_VREF_IOMUX_VREFPOS                                  (IOMUX_PINCM53)
#define GPIO_VREF_IOMUX_VREFPOS_FUNC                IOMUX_PINCM53_PF_UNCONNECTED
#define GPIO_VREF_VREFNEG_PORT                                             GPIOA
#define GPIO_VREF_VREFNEG_PIN                                     DL_GPIO_PIN_21
#define GPIO_VREF_IOMUX_VREFNEG                                  (IOMUX_PINCM46)
#define GPIO_VREF_IOMUX_VREFNEG_FUNC                IOMUX_PINCM46_PF_UNCONNECTED




/* Port definition for Pin Group IMU */
#define IMU_PORT                                                         (GPIOB)

/* Defines for CS_IMU: GPIOB.12 with pinCMx 29 on package pin 36 */
#define IMU_CS_IMU_PIN                                          (DL_GPIO_PIN_12)
#define IMU_CS_IMU_IOMUX                                         (IOMUX_PINCM29)
/* Port definition for Pin Group FMQ */
#define FMQ_PORT                                                         (GPIOB)

/* Defines for BBB: GPIOB.26 with pinCMx 57 on package pin 76 */
#define FMQ_BBB_PIN                                             (DL_GPIO_PIN_26)
#define FMQ_BBB_IOMUX                                            (IOMUX_PINCM57)
/* Port definition for Pin Group JGB */
#define JGB_PORT                                                         (GPIOA)

/* Defines for jgb: GPIOA.25 with pinCMx 55 on package pin 74 */
#define JGB_jgb_PIN                                             (DL_GPIO_PIN_25)
#define JGB_jgb_IOMUX                                            (IOMUX_PINCM55)
/* Port definition for Pin Group LED */
#define LED_PORT                                                         (GPIOA)

/* Defines for L1: GPIOA.14 with pinCMx 36 on package pin 43 */
#define LED_L1_PIN                                              (DL_GPIO_PIN_14)
#define LED_L1_IOMUX                                             (IOMUX_PINCM36)
/* Defines for L2: GPIOA.2 with pinCMx 7 on package pin 10 */
#define LED_L2_PIN                                               (DL_GPIO_PIN_2)
#define LED_L2_IOMUX                                              (IOMUX_PINCM7)
/* Port definition for Pin Group key */
#define key_PORT                                                         (GPIOB)

/* Defines for user: GPIOB.31 with pinCMx 68 on package pin 27 */
// pins affected by this interrupt request:["user","PIN_6","PIN_7"]
#define key_INT_IRQN                                            (GPIOB_INT_IRQn)
#define key_INT_IIDX                            (DL_INTERRUPT_GROUP1_IIDX_GPIOB)
#define key_user_IIDX                                       (DL_GPIO_IIDX_DIO31)
#define key_user_PIN                                            (DL_GPIO_PIN_31)
#define key_user_IOMUX                                           (IOMUX_PINCM68)
/* Defines for PIN_6: GPIOB.6 with pinCMx 23 on package pin 30 */
#define key_PIN_6_IIDX                                       (DL_GPIO_IIDX_DIO6)
#define key_PIN_6_PIN                                            (DL_GPIO_PIN_6)
#define key_PIN_6_IOMUX                                          (IOMUX_PINCM23)
/* Defines for PIN_7: GPIOB.7 with pinCMx 24 on package pin 31 */
#define key_PIN_7_IIDX                                       (DL_GPIO_IIDX_DIO7)
#define key_PIN_7_PIN                                            (DL_GPIO_PIN_7)
#define key_PIN_7_IOMUX                                          (IOMUX_PINCM24)
/* Defines for AIN1: GPIOB.20 with pinCMx 48 on package pin 67 */
#define TB6612_AIN1_PORT                                                 (GPIOB)
#define TB6612_AIN1_PIN                                         (DL_GPIO_PIN_20)
#define TB6612_AIN1_IOMUX                                        (IOMUX_PINCM48)
/* Defines for AIN2: GPIOB.24 with pinCMx 52 on package pin 71 */
#define TB6612_AIN2_PORT                                                 (GPIOB)
#define TB6612_AIN2_PIN                                         (DL_GPIO_PIN_24)
#define TB6612_AIN2_IOMUX                                        (IOMUX_PINCM52)
/* Defines for BIN1: GPIOA.8 with pinCMx 19 on package pin 22 */
#define TB6612_BIN1_PORT                                                 (GPIOA)
#define TB6612_BIN1_PIN                                          (DL_GPIO_PIN_8)
#define TB6612_BIN1_IOMUX                                        (IOMUX_PINCM19)
/* Defines for BIN2: GPIOA.9 with pinCMx 20 on package pin 23 */
#define TB6612_BIN2_PORT                                                 (GPIOA)
#define TB6612_BIN2_PIN                                          (DL_GPIO_PIN_9)
#define TB6612_BIN2_IOMUX                                        (IOMUX_PINCM20)
/* Port definition for Pin Group XUNJI */
#define XUNJI_PORT                                                       (GPIOC)

/* Defines for LLL: GPIOC.7 with pinCMx 85 on package pin 64 */
#define XUNJI_LLL_PIN                                            (DL_GPIO_PIN_7)
#define XUNJI_LLL_IOMUX                                          (IOMUX_PINCM85)
/* Defines for LL: GPIOC.2 with pinCMx 76 on package pin 50 */
#define XUNJI_LL_PIN                                             (DL_GPIO_PIN_2)
#define XUNJI_LL_IOMUX                                           (IOMUX_PINCM76)
/* Defines for L: GPIOC.3 with pinCMx 77 on package pin 51 */
#define XUNJI_L_PIN                                              (DL_GPIO_PIN_3)
#define XUNJI_L_IOMUX                                            (IOMUX_PINCM77)
/* Defines for ML: GPIOC.4 with pinCMx 78 on package pin 52 */
#define XUNJI_ML_PIN                                             (DL_GPIO_PIN_4)
#define XUNJI_ML_IOMUX                                           (IOMUX_PINCM78)
/* Defines for MR: GPIOC.5 with pinCMx 79 on package pin 53 */
#define XUNJI_MR_PIN                                             (DL_GPIO_PIN_5)
#define XUNJI_MR_IOMUX                                           (IOMUX_PINCM79)
/* Defines for R: GPIOC.0 with pinCMx 74 on package pin 46 */
#define XUNJI_R_PIN                                              (DL_GPIO_PIN_0)
#define XUNJI_R_IOMUX                                            (IOMUX_PINCM74)
/* Defines for RR: GPIOC.1 with pinCMx 75 on package pin 47 */
#define XUNJI_RR_PIN                                             (DL_GPIO_PIN_1)
#define XUNJI_RR_IOMUX                                           (IOMUX_PINCM75)
/* Defines for RRR: GPIOC.6 with pinCMx 84 on package pin 63 */
#define XUNJI_RRR_PIN                                            (DL_GPIO_PIN_6)
#define XUNJI_RRR_IOMUX                                          (IOMUX_PINCM84)
/* Port definition for Pin Group OLED */
#define OLED_PORT                                                        (GPIOB)

/* Defines for SCL: GPIOB.3 with pinCMx 16 on package pin 19 */
#define OLED_SCL_PIN                                             (DL_GPIO_PIN_3)
#define OLED_SCL_IOMUX                                           (IOMUX_PINCM16)
/* Defines for SDA: GPIOB.2 with pinCMx 15 on package pin 18 */
#define OLED_SDA_PIN                                             (DL_GPIO_PIN_2)
#define OLED_SDA_IOMUX                                           (IOMUX_PINCM15)
/* Defines for PIN_0: GPIOB.0 with pinCMx 12 on package pin 15 */
#define key_PIN_0_PIN                                            (DL_GPIO_PIN_0)
#define key_PIN_0_IOMUX                                          (IOMUX_PINCM12)
/* Defines for PIN_1: GPIOB.1 with pinCMx 13 on package pin 16 */
#define key_PIN_1_PIN                                            (DL_GPIO_PIN_1)
#define key_PIN_1_IOMUX                                          (IOMUX_PINCM13)


/* Defines for TRNG */
/*
 * The TRNG interrupt is part of INT_GROUP1. Refer to the TRM for more details
 * on interrupt grouping
 */
#define TRNG_INT_IRQN                                            (TRNG_INT_IRQn)
#define TRNG_INT_IIDX                            (DL_INTERRUPT_GROUP1_IIDX_TRNG)



/* Defines for MCAN0 */
#define MCAN0_INST                                                        CANFD1
#define GPIO_MCAN0_CAN_TX_PORT                                             GPIOB
#define GPIO_MCAN0_CAN_TX_PIN                                     DL_GPIO_PIN_21
#define GPIO_MCAN0_IOMUX_CAN_TX                                  (IOMUX_PINCM49)
#define GPIO_MCAN0_IOMUX_CAN_TX_FUNC               IOMUX_PINCM49_PF_CANFD1_CANTX
#define GPIO_MCAN0_CAN_RX_PORT                                             GPIOB
#define GPIO_MCAN0_CAN_RX_PIN                                     DL_GPIO_PIN_22
#define GPIO_MCAN0_IOMUX_CAN_RX                                  (IOMUX_PINCM50)
#define GPIO_MCAN0_IOMUX_CAN_RX_FUNC               IOMUX_PINCM50_PF_CANFD1_CANRX
#define MCAN0_INST_IRQHandler                                 CANFD1_IRQHandler
#define MCAN0_INST_INT_IRQN                                     CANFD1_INT_IRQn


/* Defines for MCAN0 MCAN RAM configuration */
#define MCAN0_INST_MCAN_STD_ID_FILT_START_ADDR     (0)
#define MCAN0_INST_MCAN_STD_ID_FILTER_NUM          (1)
#define MCAN0_INST_MCAN_EXT_ID_FILT_START_ADDR     (48)
#define MCAN0_INST_MCAN_EXT_ID_FILTER_NUM          (1)
#define MCAN0_INST_MCAN_TX_BUFF_START_ADDR         (148)
#define MCAN0_INST_MCAN_TX_BUFF_SIZE               (2)
#define MCAN0_INST_MCAN_FIFO_1_START_ADDR          (192)
#define MCAN0_INST_MCAN_FIFO_1_NUM                 (2)
#define MCAN0_INST_MCAN_TX_EVENT_START_ADDR        (164)
#define MCAN0_INST_MCAN_TX_EVENT_SIZE              (2)
#define MCAN0_INST_MCAN_EXT_ID_AND_MASK            (0x1FFFFFFFU)
#define MCAN0_INST_MCAN_RX_BUFF_START_ADDR         (208)
#define MCAN0_INST_MCAN_FIFO_0_START_ADDR          (172)
#define MCAN0_INST_MCAN_FIFO_0_NUM                 (3)

#define MCAN0_INST_MCAN_INTERRUPTS (DL_MCAN_INTERRUPT_RF0N)




/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);

bool SYSCFG_DL_SYSCTL_SYSPLL_init(void);
void SYSCFG_DL_TimerG6_PWM_init(void);
void SYSCFG_DL_PWM_0_init(void);
void SYSCFG_DL_QEI_0_init(void);
void SYSCFG_DL_QEI_1_init(void);
void SYSCFG_DL_TimerA1_init(void);
void SYSCFG_DL_TypeC_init(void);
void SYSCFG_DL_UART_4_init(void);
void SYSCFG_DL_UART_Vision_init(void);
void SYSCFG_DL_Wireless_init(void);
void SYSCFG_DL_Communication_init(void);
void SYSCFG_DL_SPI_1_init(void);
void SYSCFG_DL_VREF_init(void);

void SYSCFG_DL_TRNG_init(void);
void SYSCFG_DL_MCAN0_init(void);

bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
