#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "inc/tm4c1294ncpdt.h" // CMSIS-Core
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h" // driverlib
#include "driverlib/gpio.h"
#include "driverlib/systick.h"
#include "inc/UART.h"

/*
https://dzone.com/articles/cycle-counting-on-an-arm-cortex-m-with-dwt
*/
  /* DWT (Data Watchpoint and Trace) registers, only exists on ARM Cortex with a DWT unit */
  #define KIN1_DWT_CONTROL             (*((volatile uint32_t*)0xE0001000))
    /*!< DWT Control register */
  #define KIN1_DWT_CYCCNTENA_BIT       (1UL<<0)
    /*!< CYCCNTENA bit in DWT_CONTROL register */
  #define KIN1_DWT_CYCCNT              (*((volatile uint32_t*)0xE0001004))
    /*!< DWT Cycle Counter register */
  #define KIN1_DEMCR                   (*((volatile uint32_t*)0xE000EDFC))
    /*!< DEMCR: Debug Exception and Monitor Control Register */
  #define KIN1_TRCENA_BIT              (1UL<<24)
    /*!< Trace enable bit in DEMCR register */

#define KIN1_InitCycleCounter() \
  KIN1_DEMCR |= KIN1_TRCENA_BIT
  /*!< TRCENA: Enable trace and debug block DEMCR (Debug Exception and Monitor Control Register */
 
#define KIN1_ResetCycleCounter() \
  KIN1_DWT_CYCCNT = 0
  /*!< Reset cycle counter */
 
#define KIN1_EnableCycleCounter() \
  KIN1_DWT_CONTROL |= KIN1_DWT_CYCCNTENA_BIT
  /*!< Enable cycle counter */
 
#define KIN1_DisableCycleCounter() \
  KIN1_DWT_CONTROL &= ~KIN1_DWT_CYCCNTENA_BIT
  /*!< Disable cycle counter */
 
#define KIN1_GetCycleCounter() \
  KIN1_DWT_CYCCNT
  /*!< Read cycle counter register */



uint8_t LED_D1 = 0;
//extern int f_asm(int a, int b);
extern int count_pulse_hz(void);
extern int count_pulse_khz(void);

// U0Rx (VCP receive) connected to PA0
// U0Tx (VCP transmit) connected to PA1
// Note: Connected LaunchPad JP4 and JP5 inserted parallel with long side of board.
void OutCRLF(void){
  UART_OutChar(CR);
  UART_OutChar(LF);
}

void SysTick_Handler(void){
    GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_1, LED_D1); // habilita ou desabilita saída PK1
    LED_D1 ^= GPIO_PIN_1; // Troca estado
} // SysTick_Handler

void main(void)
{
  //uint32_t cycles; /* number of cycles */
  char string_rx[4];      /*string para receber dados da UART*/
  uint32_t read_sequence; /*sequencia da leitura*/
  uint32_t frequency;     /*Frequencia calculada*/
  uint8_t is_khz_to_read = 0; /*flag para habilitar leitura em kHz[1] ou Hz[0]*/

  //KIN1_InitCycleCounter(); /* enable DWT hardware | usado para contar o numero de ciclos*/
  UART_Init();
  uint32_t ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                              SYSCTL_OSC_MAIN |
                                              SYSCTL_USE_PLL |
                                              SYSCTL_CFG_VCO_480),
                                              120000000); // PLL em 24MHz

  //SysTickEnable();
  //SysTickPeriodSet(2400000); // f = 5Hz

  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ); // Habilita GPIO J (push-button SW1 = PJ0, push-button SW2 = PJ1)
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ)); // Aguarda final da habilitação
    
  GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1); // push-buttons SW1 e SW2 como entrada
  GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);


  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK); // Habilita GPIO K 0 = input| 1 = output
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOK)); // Aguarda final da habilitação  
  
  GPIOPinTypeGPIOInput(GPIO_PORTK_BASE, GPIO_PIN_0); // PIN 0 as input
  GPIOPadConfigSet(GPIO_PORTK_BASE, GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
  GPIOPinTypeGPIOOutput(GPIO_PORTK_BASE, GPIO_PIN_1); // PIN 1 as output
  GPIOPadConfigSet(GPIO_PORTK_BASE, GPIO_PIN_1, GPIO_STRENGTH_12MA, GPIO_PIN_TYPE_STD);
  
  //SysTickIntEnable();


  //KIN1_ResetCycleCounter(); /* reset cycle counter */
  //KIN1_EnableCycleCounter(); /* start counting */

  while(1)
  {
    /* Verify if there is data to receive in UART0*/
    if((UART0_FR_R&UART_FR_RXFE) == 0)
    {
      UART_InString(string_rx,3); OutCRLF();
      if(strcmp(string_rx, "kHz") == 0) //0x6b487a0d
        is_khz_to_read = 1;
      else
        is_khz_to_read = 0;
    }

   if(!(GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_1) == GPIO_PIN_1))
   {
     UART_OutString(is_khz_to_read?"kHz:": "Hz:");
    for(read_sequence=0; read_sequence<10; read_sequence++)
    {
      //UART_OutUDec(read_sequence);
      //KIN1_ResetCycleCounter(); /* reset cycle counter */
      frequency = is_khz_to_read? count_pulse_khz()/2:count_pulse_hz()/2;
      //cycles = KIN1_GetCycleCounter(); /* get cycle counter */
      UART_OutUDec(frequency); UART_OutChar(';');
    }//for
    OutCRLF();
   }
  } // while
} // main