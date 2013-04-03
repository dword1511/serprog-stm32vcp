#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

extern void USB_Istr(void);

#define NMIException               EmptyVect

#define HardFaultException         FaultVect
#define MemManageException         FaultVect
#define BusFaultException          FaultVect
#define UsageFaultException        FaultVect

#define DebugMonitor               EmptyVect
#define SVCHandler                 EmptyVect
#define PendSVC                    EmptyVect
#define SysTickHandler             EmptyVect
#define WWDG_IRQHandler            EmptyVect
#define PVD_IRQHandler             EmptyVect
#define TAMPER_IRQHandler          EmptyVect
#define RTC_IRQHandler             EmptyVect
#define FLASH_IRQHandler           EmptyVect
#define RCC_IRQHandler             EmptyVect
#define EXTI0_IRQHandler           EmptyVect
#define EXTI1_IRQHandler           EmptyVect
#define EXTI2_IRQHandler           EmptyVect
#define EXTI3_IRQHandler           EmptyVect
#define EXTI4_IRQHandler           EmptyVect
#define DMA1_Channel1_IRQHandler   EmptyVect
#define DMA1_Channel2_IRQHandler   EmptyVect
#define DMA1_Channel3_IRQHandler   EmptyVect
#define DMA1_Channel4_IRQHandler   EmptyVect
#define DMA1_Channel5_IRQHandler   EmptyVect
#define DMA1_Channel6_IRQHandler   EmptyVect
#define DMA1_Channel7_IRQHandler   EmptyVect
#define ADC1_2_IRQHandler          EmptyVect
#define USB_HP_CAN_TX_IRQHandler   EmptyVect

#define USB_LP_CAN_RX0_IRQHandler  USB_Istr

#define CAN_RX1_IRQHandler         EmptyVect
#define CAN_SCE_IRQHandler         EmptyVect
#define EXTI9_5_IRQHandler         EmptyVect
#define TIM1_BRK_IRQHandler        EmptyVect
#define TIM1_UP_IRQHandler         EmptyVect
#define TIM1_TRG_COM_IRQHandler    EmptyVect
#define TIM1_CC_IRQHandler         EmptyVect
#define TIM2_IRQHandler            EmptyVect
#define TIM3_IRQHandler            EmptyVect
#define TIM4_IRQHandler            EmptyVect
#define I2C1_EV_IRQHandler         EmptyVect
#define I2C1_ER_IRQHandler         EmptyVect
#define I2C2_EV_IRQHandler         EmptyVect
#define I2C2_ER_IRQHandler         EmptyVect
#define SPI1_IRQHandler            EmptyVect
#define SPI2_IRQHandler            EmptyVect
#define USART1_IRQHandler          EmptyVect
#define USART2_IRQHandler          EmptyVect
#define USART3_IRQHandler          EmptyVect
#define EXTI15_10_IRQHandler       EmptyVect
#define RTCAlarm_IRQHandler        EmptyVect
#define USBWakeUp_IRQHandler       EmptyVect
#define TIM8_BRK_IRQHandler        EmptyVect
#define TIM8_UP_IRQHandler         EmptyVect
#define TIM8_TRG_COM_IRQHandler    EmptyVect
#define TIM8_CC_IRQHandler         EmptyVect
#define ADC3_IRQHandler            EmptyVect
#define FSMC_IRQHandler            EmptyVect
#define SDIO_IRQHandler            EmptyVect
#define TIM5_IRQHandler            EmptyVect
#define SPI3_IRQHandler            EmptyVect
#define UART4_IRQHandler           EmptyVect
#define UART5_IRQHandler           EmptyVect
#define TIM6_IRQHandler            EmptyVect
#define TIM7_IRQHandler            EmptyVect
#define DMA2_Channel1_IRQHandler   EmptyVect
#define DMA2_Channel2_IRQHandler   EmptyVect
#define DMA2_Channel3_IRQHandler   EmptyVect
#define DMA2_Channel4_5_IRQHandler EmptyVect

#endif /* __INTERRUPTS_H__ */