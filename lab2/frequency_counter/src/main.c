#include <stdbool.h>
#include <stdint.h>
//#include "inc/tm4c1294ncpdt.h" // CMSIS-Core
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h" // driverlib
#include "driverlib/gpio.h"
#include "driverlib/systick.h"
#include "driverlib/pin_map.h"
#include "driverlib/timer.h"
#include "driverlib/rom_map.h"
#include "driverlib/interrupt.h"
#include "inc/tm4c1294ncpdt.h"
#include "UART.h"

uint32_t timerValue;
uint8_t newAcquire = 0;

#define TIMER_O_TAV             0x00000050  // GPTM Timer A Value
#define TIMER_O_TBV             0x00000054  // GPTM Timer B Value
#define HWREG(x)                                                              \
        (*((volatile uint32_t *)(x)))
#define TimerValueSet(ulBase, ulTimer, ulValue)                               \
   HWREG((ulBase) + ((ulTimer)==TIMER_A ? TIMER_O_TAV : TIMER_O_TBV)) =      \
       (ulValue)

void OutCRLF(void){
  UART_OutChar(CR);
  UART_OutChar(LF);
}

void SysTick_Handler(void){
  timerValue = TIMER0_TAV_R;
  newAcquire = 1;
} // SysTick_Handler

void timerCounterInit(void) // uses pin PA4 on the Tiva CPU (See CPU doc P 1808/1890)

{
   SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD); // enable the GPIO used for the pin.
   SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0); // enable the timer.
   while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0))
   { // Wait for the Timer0 module to be ready.
   }
   GPIOPinTypeTimer(GPIO_PORTD_BASE, GPIO_PIN_0); // configure the pin as a timer
   GPIOPinConfigure(GPIO_PD0_T0CCP0); // set the alternate function
  
   TimerConfigure(TIMER0_BASE, (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_COUNT_UP));
   //TimerPrescaleSet(TIMER0_BASE, TIMER_A, 0xFF);
   //TimerLoadSet(TIMER0_BASE, TIMER_A, 0xFFFF);
   TimerControlEvent(TIMER0_BASE, TIMER_A, TIMER_EVENT_POS_EDGE); // config the edge.
   TimerMatchSet(TIMER0_BASE, TIMER_A, 0xFFFF); //
   TimerPrescaleMatchSet(TIMER0_BASE, TIMER_A, 0xFF);
   
   //TimerIntEnable(TIMER0_BASE, TIMER_CAPA_MATCH); // select the IRQ's to enable.
   //MAP_IntEnable(INT_TIMER0A); // enable the IRQ 39.
   TimerEnable(TIMER0_BASE, TIMER_A); // enable the timer. 
}


void main(void){
  uint32_t lastValue;
  uint32_t ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                              SYSCTL_OSC_MAIN |
                                              SYSCTL_USE_PLL |
                                              SYSCTL_CFG_VCO_480),
                                              24000000); // PLL em 24MHz
  
  SysTickEnable();
  SysTickPeriodSet(12000000); // f = 1Hz
  /*
    Inicializa o timer
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0)){}
    
    //Configurar o pino PD0 como CCP0 do timer 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD)){}
    
    GPIOPinTypeTimer(GPIO_PORTD_BASE, GPIO_PIN_0);
    GPIOPinConfigure(GPIO_PD0_T0CCP0);
  
    // Set the pin to use the internal pull-up.
    GPIOPadConfigSet(GPIO_PORTD_BASE, GPIO_PIN_0,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    
    // Configure the timers in upward edge count mode.
    TimerConfigure(TIMER0_BASE, (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_COUNT));

    TimerControlEvent(TIMER0_BASE, TIMER_A, TIMER_EVENT_POS_EDGE);
    TimerLoadSet(TIMER0_BASE, TIMER_A, 0);
    TimerMatchSet(TIMER0_BASE, TIMER_A, 65535);
    TimerIntEnable(TIMER0_BASE, TIMER_CAPA_MATCH);
    TimerEnable(TIMER0_BASE, TIMER_A);
    */
  UART_Init();
  //timerInit();
  timerCounterInit();
  SysTickIntEnable();

  while(1){
    if( newAcquire == 1)
    {
      newAcquire = 0;
      UART_OutString("Frequency Read: ");
      UART_OutUDec(timerValue>=lastValue? (timerValue-lastValue)*2 : ((0xFFFFFF-lastValue)+timerValue)*2);
      OutCRLF();
      lastValue = timerValue;
    }

  } // while
} // main