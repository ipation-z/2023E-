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
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

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



#define CPUCLK_FREQ                                                     32000000



/* Defines for TIMER_1 */
#define TIMER_1_INST                                                    (TIMG12)
#define TIMER_1_INST_IRQHandler                                TIMG12_IRQHandler
#define TIMER_1_INST_INT_IRQN                                  (TIMG12_INT_IRQn)
#define TIMER_1_INST_LOAD_VALUE                                         (31999U)
/* Defines for TIMER_2 */
#define TIMER_2_INST                                                     (TIMG6)
#define TIMER_2_INST_IRQHandler                                 TIMG6_IRQHandler
#define TIMER_2_INST_INT_IRQN                                   (TIMG6_INT_IRQn)
#define TIMER_2_INST_LOAD_VALUE                                         (31999U)



/* Defines for UART_0 */
#define UART_0_INST                                                        UART0
#define UART_0_INST_FREQUENCY                                            4000000
#define UART_0_INST_IRQHandler                                  UART0_IRQHandler
#define UART_0_INST_INT_IRQN                                      UART0_INT_IRQn
#define GPIO_UART_0_RX_PORT                                                GPIOA
#define GPIO_UART_0_TX_PORT                                                GPIOA
#define GPIO_UART_0_RX_PIN                                        DL_GPIO_PIN_11
#define GPIO_UART_0_TX_PIN                                        DL_GPIO_PIN_10
#define GPIO_UART_0_IOMUX_RX                                     (IOMUX_PINCM22)
#define GPIO_UART_0_IOMUX_TX                                     (IOMUX_PINCM21)
#define GPIO_UART_0_IOMUX_RX_FUNC                      IOMUX_PINCM22_PF_UART0_RX
#define GPIO_UART_0_IOMUX_TX_FUNC                      IOMUX_PINCM21_PF_UART0_TX
#define UART_0_BAUD_RATE                                                (115200)
#define UART_0_IBRD_4_MHZ_115200_BAUD                                        (2)
#define UART_0_FBRD_4_MHZ_115200_BAUD                                       (11)
/* Defines for UART_1 */
#define UART_1_INST                                                        UART1
#define UART_1_INST_FREQUENCY                                            4000000
#define UART_1_INST_IRQHandler                                  UART1_IRQHandler
#define UART_1_INST_INT_IRQN                                      UART1_INT_IRQn
#define GPIO_UART_1_RX_PORT                                                GPIOA
#define GPIO_UART_1_TX_PORT                                                GPIOA
#define GPIO_UART_1_RX_PIN                                         DL_GPIO_PIN_9
#define GPIO_UART_1_TX_PIN                                         DL_GPIO_PIN_8
#define GPIO_UART_1_IOMUX_RX                                     (IOMUX_PINCM20)
#define GPIO_UART_1_IOMUX_TX                                     (IOMUX_PINCM19)
#define GPIO_UART_1_IOMUX_RX_FUNC                      IOMUX_PINCM20_PF_UART1_RX
#define GPIO_UART_1_IOMUX_TX_FUNC                      IOMUX_PINCM19_PF_UART1_TX
#define UART_1_BAUD_RATE                                                (115200)
#define UART_1_IBRD_4_MHZ_115200_BAUD                                        (2)
#define UART_1_FBRD_4_MHZ_115200_BAUD                                       (11)




/* Defines for ST7735_SPI */
#define ST7735_SPI_INST                                                    SPI0
#define ST7735_SPI_INST_IRQHandler                              SPI0_IRQHandler
#define ST7735_SPI_INST_INT_IRQN                                  SPI0_INT_IRQn
#define GPIO_ST7735_SPI_PICO_PORT                                         GPIOA
#define GPIO_ST7735_SPI_PICO_PIN                                 DL_GPIO_PIN_14
#define GPIO_ST7735_SPI_IOMUX_PICO                              (IOMUX_PINCM36)
#define GPIO_ST7735_SPI_IOMUX_PICO_FUNC              IOMUX_PINCM36_PF_SPI0_PICO
/* GPIO configuration for ST7735_SPI */
#define GPIO_ST7735_SPI_SCLK_PORT                                         GPIOA
#define GPIO_ST7735_SPI_SCLK_PIN                                 DL_GPIO_PIN_12
#define GPIO_ST7735_SPI_IOMUX_SCLK                              (IOMUX_PINCM34)
#define GPIO_ST7735_SPI_IOMUX_SCLK_FUNC              IOMUX_PINCM34_PF_SPI0_SCLK



/* Defines for PIN_DIR: GPIOB.19 with pinCMx 45 on package pin 16 */
#define GPIO_MOTOR1_PIN_DIR_PORT                                         (GPIOB)
#define GPIO_MOTOR1_PIN_DIR_PIN                                 (DL_GPIO_PIN_19)
#define GPIO_MOTOR1_PIN_DIR_IOMUX                                (IOMUX_PINCM45)
/* Defines for PIN_MS1: GPIOA.1 with pinCMx 2 on package pin 34 */
#define GPIO_MOTOR1_PIN_MS1_PORT                                         (GPIOA)
#define GPIO_MOTOR1_PIN_MS1_PIN                                  (DL_GPIO_PIN_1)
#define GPIO_MOTOR1_PIN_MS1_IOMUX                                 (IOMUX_PINCM2)
/* Defines for PIN_MS2: GPIOA.28 with pinCMx 3 on package pin 35 */
#define GPIO_MOTOR1_PIN_MS2_PORT                                         (GPIOA)
#define GPIO_MOTOR1_PIN_MS2_PIN                                 (DL_GPIO_PIN_28)
#define GPIO_MOTOR1_PIN_MS2_IOMUX                                 (IOMUX_PINCM3)
/* Defines for PIN_MS3: GPIOB.24 with pinCMx 52 on package pin 23 */
#define GPIO_MOTOR1_PIN_MS3_PORT                                         (GPIOB)
#define GPIO_MOTOR1_PIN_MS3_PIN                                 (DL_GPIO_PIN_24)
#define GPIO_MOTOR1_PIN_MS3_IOMUX                                (IOMUX_PINCM52)
/* Defines for PIN_SW: GPIOA.31 with pinCMx 6 on package pin 39 */
#define GPIO_MOTOR1_PIN_SW_PORT                                          (GPIOA)
#define GPIO_MOTOR1_PIN_SW_PIN                                  (DL_GPIO_PIN_31)
#define GPIO_MOTOR1_PIN_SW_IOMUX                                  (IOMUX_PINCM6)
/* Defines for PIN_EN: GPIOA.0 with pinCMx 1 on package pin 33 */
#define GPIO_MOTOR1_PIN_EN_PORT                                          (GPIOA)
#define GPIO_MOTOR1_PIN_EN_PIN                                   (DL_GPIO_PIN_0)
#define GPIO_MOTOR1_PIN_EN_IOMUX                                  (IOMUX_PINCM1)
/* Defines for PIN_STEP: GPIOB.20 with pinCMx 48 on package pin 19 */
#define GPIO_MOTOR1_PIN_STEP_PORT                                        (GPIOB)
#define GPIO_MOTOR1_PIN_STEP_PIN                                (DL_GPIO_PIN_20)
#define GPIO_MOTOR1_PIN_STEP_IOMUX                               (IOMUX_PINCM48)
/* Defines for PIN_SW2: GPIOA.2 with pinCMx 7 on package pin 42 */
#define GPIO_MOTOR1_PIN_SW2_PORT                                         (GPIOA)
#define GPIO_MOTOR1_PIN_SW2_PIN                                  (DL_GPIO_PIN_2)
#define GPIO_MOTOR1_PIN_SW2_IOMUX                                 (IOMUX_PINCM7)
/* Port definition for Pin Group ST7735 */
#define ST7735_PORT                                                      (GPIOA)

/* Defines for RES: GPIOA.15 with pinCMx 37 on package pin 8 */
#define ST7735_RES_PIN                                          (DL_GPIO_PIN_15)
#define ST7735_RES_IOMUX                                         (IOMUX_PINCM37)
/* Defines for DC: GPIOA.16 with pinCMx 38 on package pin 9 */
#define ST7735_DC_PIN                                           (DL_GPIO_PIN_16)
#define ST7735_DC_IOMUX                                          (IOMUX_PINCM38)
/* Defines for CS: GPIOA.17 with pinCMx 39 on package pin 10 */
#define ST7735_CS_PIN                                           (DL_GPIO_PIN_17)
#define ST7735_CS_IOMUX                                          (IOMUX_PINCM39)
/* Defines for BL: GPIOA.18 with pinCMx 40 on package pin 11 */
#define ST7735_BL_PIN                                           (DL_GPIO_PIN_18)
#define ST7735_BL_IOMUX                                          (IOMUX_PINCM40)
/* Defines for PIN2_DIR: GPIOB.9 with pinCMx 26 on package pin 61 */
#define GPIO_MOTOR2_PIN2_DIR_PORT                                        (GPIOB)
#define GPIO_MOTOR2_PIN2_DIR_PIN                                 (DL_GPIO_PIN_9)
#define GPIO_MOTOR2_PIN2_DIR_IOMUX                               (IOMUX_PINCM26)
/* Defines for PIN2_MS1: GPIOA.26 with pinCMx 59 on package pin 30 */
#define GPIO_MOTOR2_PIN2_MS1_PORT                                        (GPIOA)
#define GPIO_MOTOR2_PIN2_MS1_PIN                                (DL_GPIO_PIN_26)
#define GPIO_MOTOR2_PIN2_MS1_IOMUX                               (IOMUX_PINCM59)
/* Defines for PIN2_MS2: GPIOA.25 with pinCMx 55 on package pin 26 */
#define GPIO_MOTOR2_PIN2_MS2_PORT                                        (GPIOA)
#define GPIO_MOTOR2_PIN2_MS2_PIN                                (DL_GPIO_PIN_25)
#define GPIO_MOTOR2_PIN2_MS2_IOMUX                               (IOMUX_PINCM55)
/* Defines for PIN2_MS3: GPIOA.22 with pinCMx 47 on package pin 18 */
#define GPIO_MOTOR2_PIN2_MS3_PORT                                        (GPIOA)
#define GPIO_MOTOR2_PIN2_MS3_PIN                                (DL_GPIO_PIN_22)
#define GPIO_MOTOR2_PIN2_MS3_IOMUX                               (IOMUX_PINCM47)
/* Defines for PIN2_EN: GPIOA.27 with pinCMx 60 on package pin 31 */
#define GPIO_MOTOR2_PIN2_EN_PORT                                         (GPIOA)
#define GPIO_MOTOR2_PIN2_EN_PIN                                 (DL_GPIO_PIN_27)
#define GPIO_MOTOR2_PIN2_EN_IOMUX                                (IOMUX_PINCM60)
/* Defines for PIN2_STEP: GPIOA.21 with pinCMx 46 on package pin 17 */
#define GPIO_MOTOR2_PIN2_STEP_PORT                                       (GPIOA)
#define GPIO_MOTOR2_PIN2_STEP_PIN                               (DL_GPIO_PIN_21)
#define GPIO_MOTOR2_PIN2_STEP_IOMUX                              (IOMUX_PINCM46)
/* Defines for PIN2_SW1: GPIOA.24 with pinCMx 54 on package pin 25 */
#define GPIO_MOTOR2_PIN2_SW1_PORT                                        (GPIOA)
#define GPIO_MOTOR2_PIN2_SW1_PIN                                (DL_GPIO_PIN_24)
#define GPIO_MOTOR2_PIN2_SW1_IOMUX                               (IOMUX_PINCM54)
/* Defines for PIN2_SW2: GPIOA.23 with pinCMx 53 on package pin 24 */
#define GPIO_MOTOR2_PIN2_SW2_PORT                                        (GPIOA)
#define GPIO_MOTOR2_PIN2_SW2_PIN                                (DL_GPIO_PIN_23)
#define GPIO_MOTOR2_PIN2_SW2_IOMUX                               (IOMUX_PINCM53)
/* Defines for PIN_CHANGE_INDEX: GPIOB.18 with pinCMx 44 on package pin 15 */
#define GPIO_CONTROL_PIN_CHANGE_INDEX_PORT                               (GPIOB)
#define GPIO_CONTROL_PIN_CHANGE_INDEX_PIN                       (DL_GPIO_PIN_18)
#define GPIO_CONTROL_PIN_CHANGE_INDEX_IOMUX                      (IOMUX_PINCM44)
/* Defines for PIN_STATE_SWITCH: GPIOB.2 with pinCMx 15 on package pin 50 */
#define GPIO_CONTROL_PIN_STATE_SWITCH_PORT                               (GPIOB)
#define GPIO_CONTROL_PIN_STATE_SWITCH_PIN                        (DL_GPIO_PIN_2)
#define GPIO_CONTROL_PIN_STATE_SWITCH_IOMUX                      (IOMUX_PINCM15)
/* Defines for PIN_RESET: GPIOA.7 with pinCMx 14 on package pin 49 */
#define GPIO_CONTROL_PIN_RESET_PORT                                      (GPIOA)
#define GPIO_CONTROL_PIN_RESET_PIN                               (DL_GPIO_PIN_7)
#define GPIO_CONTROL_PIN_RESET_IOMUX                             (IOMUX_PINCM14)
/* Defines for PIN_STOP: GPIOA.13 with pinCMx 35 on package pin 6 */
#define GPIO_CONTROL_PIN_STOP_PORT                                       (GPIOA)
#define GPIO_CONTROL_PIN_STOP_PIN                               (DL_GPIO_PIN_13)
#define GPIO_CONTROL_PIN_STOP_IOMUX                              (IOMUX_PINCM35)
/* Port definition for Pin Group GPIO_WARN */
#define GPIO_WARN_PORT                                                   (GPIOB)

/* Defines for PIN_BUZZER: GPIOB.6 with pinCMx 23 on package pin 58 */
#define GPIO_WARN_PIN_BUZZER_PIN                                 (DL_GPIO_PIN_6)
#define GPIO_WARN_PIN_BUZZER_IOMUX                               (IOMUX_PINCM23)
/* Defines for PIN_LED: GPIOB.7 with pinCMx 24 on package pin 59 */
#define GPIO_WARN_PIN_LED_PIN                                    (DL_GPIO_PIN_7)
#define GPIO_WARN_PIN_LED_IOMUX                                  (IOMUX_PINCM24)
/* Defines for PIN_GND: GPIOB.8 with pinCMx 25 on package pin 60 */
#define GPIO_WARN_PIN_GND_PIN                                    (DL_GPIO_PIN_8)
#define GPIO_WARN_PIN_GND_IOMUX                                  (IOMUX_PINCM25)




/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_TIMER_1_init(void);
void SYSCFG_DL_TIMER_2_init(void);
void SYSCFG_DL_UART_0_init(void);
void SYSCFG_DL_UART_1_init(void);
void SYSCFG_DL_ST7735_SPI_init(void);

void SYSCFG_DL_SYSTICK_init(void);

bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
