#include <cmsis/stm32.h>
#include <stm32/rcc.h>
#include <stm32/flash.h>
#include <stm32/gpio.h>
#include "interrupts.h"

void ResetISR(void);
extern int main(void);

void EmptyVect(void) {}

void FaultVect(void) {
  while(1);
}

#ifndef STACK_SIZE
  #define STACK_SIZE 1024
#endif /* STACK_SIZE */

//*****************************************************************************
// The minimal vector table for a Cortex-M3.  Note that the proper constructs
// must be placed on this to ensure that it ends up at physical address
// 0x00000000.
//*****************************************************************************
/* init value for the stack pointer. defined in linker script */
extern void _estack;	
__attribute__ ((section(".isr_vector")))
void (* const g_pfnVectors[])(void) = {
  &_estack,            /* The initial stack pointer*/
  ResetISR,
  NMIException,
  HardFaultException,
  MemManageException,
  BusFaultException,
  UsageFaultException,
  0, 0, 0, 0,          /* Reserved */ 
  SVCHandler,
  DebugMonitor,
  0,                   /* Reserved */
  PendSVC,
  SysTickHandler,
  WWDG_IRQHandler,
  PVD_IRQHandler,
  TAMPER_IRQHandler,
  RTC_IRQHandler,
  FLASH_IRQHandler,
  RCC_IRQHandler,
  EXTI0_IRQHandler,
  EXTI1_IRQHandler,
  EXTI2_IRQHandler,
  EXTI3_IRQHandler,
  EXTI4_IRQHandler,
  DMA1_Channel1_IRQHandler,
  DMA1_Channel2_IRQHandler,
  DMA1_Channel3_IRQHandler,
  DMA1_Channel4_IRQHandler,
  DMA1_Channel5_IRQHandler,
  DMA1_Channel6_IRQHandler,
  DMA1_Channel7_IRQHandler,
  ADC1_2_IRQHandler,
  USB_HP_CAN_TX_IRQHandler,
  USB_LP_CAN_RX0_IRQHandler,
  CAN_RX1_IRQHandler,
  CAN_SCE_IRQHandler,
  EXTI9_5_IRQHandler,
  TIM1_BRK_IRQHandler,
  TIM1_UP_IRQHandler,
  TIM1_TRG_COM_IRQHandler,
  TIM1_CC_IRQHandler,
  TIM2_IRQHandler,
  TIM3_IRQHandler,
  TIM4_IRQHandler,
  I2C1_EV_IRQHandler,
  I2C1_ER_IRQHandler,
  I2C2_EV_IRQHandler,
  I2C2_ER_IRQHandler,
  SPI1_IRQHandler,
  SPI2_IRQHandler,
  USART1_IRQHandler,
  USART2_IRQHandler,
  USART3_IRQHandler,
  EXTI15_10_IRQHandler,
  RTCAlarm_IRQHandler,
  USBWakeUp_IRQHandler,
};

//*****************************************************************************
// The following are constructs created by the linker, indicating where the
// the "data" and "bss" segments reside in memory.  The initializers for the
// for the "data" segment resides immediately following the "text" segment.
//*****************************************************************************
extern uint32_t _etext;
extern uint32_t _data;
extern uint32_t _edata;
extern uint32_t _bss;
extern uint32_t _ebss;

//*****************************************************************************
// This is the code that gets called when the processor first starts execution
// following a reset event.  Only the absolutely necessary set is performed,
// after which the application supplied main() routine is called.  Any fancy
// actions (such as making decisions based on the reset cause register, and
// resetting the bits in that register) are left solely in the hands of the
// application.
//*****************************************************************************
void ResetISR(void) {
  uint32_t *pulSrc, *pulDest;

  /* Copy the data segment initializers from flash to SRAM, then zero fill the bss segment. */
  pulSrc = &_etext;
  for(pulDest = &_data; pulDest < &_edata; ) *pulDest++ = *pulSrc++;
  for(pulDest = &_bss; pulDest < &_ebss; ) *pulDest++ = 0;

  /* Configure system clock. */
  RCC_DeInit();
  RCC_HSEConfig(RCC_HSE_ON);
  if(RCC_WaitForHSEStartUp() == SUCCESS) {
    /* Flash latency have to be set to 2 cycles when SYSCLK > 48MHz */
    FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
    FLASH_SetLatency(FLASH_Latency_2);

    RCC_HCLKConfig(RCC_SYSCLK_Div1);
    RCC_PCLK2Config(RCC_HCLK_Div1);
    RCC_PCLK1Config(RCC_HCLK_Div2);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9); /* 8x9 = 72MHz */
    RCC_PLLCmd(ENABLE);
    while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
    while(RCC_GetSYSCLKSource() != 0x08);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1 | RCC_AHBPeriph_DMA2, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
  }

  main();

  while(1);
}
