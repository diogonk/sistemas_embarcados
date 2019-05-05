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

uint32_t FrequencyCounter;

void OutCRLF(void){
  UART_OutChar(CR);
  UART_OutChar(LF);
}


void timerInit(void)
{
  //volatile uint32_t * PERIPH_REG;
  
  //SYSCTL_RCGCTIMER_R;
  SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R0;
  while(!(SYSCTL_RCGCTIMER_R & SYSCTL_RCGCTIMER_R0)){}

  //SYSCTL_RCGCGPIO_R;
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R3; //Enable port D clock
  while(!(SYSCTL_RCGCGPIO_R & SYSCTL_RCGCGPIO_R3)){}

  //GPIO_PORTD_AHB_AMSEL_R;//desabilita analogica
  GPIO_PORTD_AHB_AMSEL_R = 0x00;

  //GPIO_PORTD_AHB_PCTL_R;//desabilita analogica
  GPIO_PORTD_AHB_PCTL_R = 0x00;

  //GPIO_PORTD_AHB_DIR_R;//Coloca como Entrada
  GPIO_PORTD_AHB_DIR_R &= ~0x01;
  
  //GPIO_PORTD_AHB_DIR_R;//Coloca como Entrada
  GPIO_PORTD_AHB_DEN_R |= 0x01;

  //GPIO_PORTD_AHB_AFSEL_R; //Enable Alternate Function
  GPIO_PORTD_AHB_AFSEL_R |= 0x01;

  //GPIO_PORTD_AHB_PCTL_R; //Enable Alternate Function
  GPIO_PORTD_AHB_PCTL_R |= 0x03;

  //page 973 datasheet tiva
  //TIMER0_CTL_R; //Disable timer  p. 986
  TIMER0_CTL_R &= ~(TIMER_CTL_TAEN);
  
  //TIMER0_CFG_R; //0x04 p. 976
  TIMER0_CFG_R |= (TIMER_CFG_16_BIT);

  //TIMER0_TAMR_R; //TACMR = 0x00 TAMR = 0x03  p. 977
  //TIMER0_TAMR_R &= ~(TIMER_TAMR_TACMR);
  TIMER0_TAMR_R = (TIMER_TAMR_TACDIR | TIMER_TAMR_TAMR_CAP); //TIMER_TAMR_TACDIR

/*In up-count mode, the timer counts from 0x0 to the value in the GPTMTnMATCHR and
  GPTMTnPMR registers. Note that when executing an up-count, the value of the GPTMTnPR
  and GPTMTnILR must be greater than the value of GPTMTnPMR and GPTMTnMATCHR
*/
  //TIMER0_TAMATCHR_R; //timer A value match p.1006
  TIMER0_TAMATCHR_R = 0xFFFF; //timer A value match

  //TIMER0_TAPMR_R; //prescale value match p.1010
  TIMER0_TAPMR_R = 0xFF;

  //TIMER0_CTL_R; //TAEVENT  p. 986
  TIMER0_CTL_R &= ~(0x0C); //POSITIVE EDGE [3:2] = 0x00;

  //TIMER0_TAPR_R; //Write the prescale value p.1008
  //TIMER0_TAPR_R = 0x00;

  //TIMER0_TAILR_R; //Load the timer start value p.1004
  //TIMER0_TAILR_R = 0x0000;

  //TIMER0_IMR_R; // If using interrupt are required, set CnEIM p. 993

  //TIMER0_CTL_R; //Enable timer p. 986
  TIMER0_CTL_R |= (TIMER_CTL_TAEN);
}

void SysTick_Handler(void){
  //volatile uint32_t * PERIPH_REG = (uint32_t*)TIMER0_RIS_R;;
  TIMER0_CTL_R &= ~(TIMER_CTL_TAEN);
  FrequencyCounter = TIMER0_TAR_R * 2;//TimerValueGet(TIMER2_BASE, TIMER_A);
  TIMER0_TAR_R = 0x00;
  TIMER0_TAV_R = 0x00;
  UART_OutString("Frequency Read: ");
  UART_OutUDec(FrequencyCounter);
  OutCRLF();
  //timerInit();
  TIMER0_CTL_R |= (TIMER_CTL_TAEN);
} // SysTick_Handler

void timerCounterInit(void) // uses pin PA4 on the Tiva CPU (See CPU doc P 1808/1890)

{
   MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); // enable the GPIO used for the pin.
   MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2); // enable the timer.
   while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER2))
   { // Wait for the Timer0 module to be ready.
   }
   MAP_GPIOPinConfigure(GPIO_PA4_T2CCP0); // set the alternate function
   MAP_GPIOPinTypeTimer(GPIO_PORTA_BASE, GPIO_PIN_4); // configure the pin as a timer
   MAP_TimerConfigure(TIMER2_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_COUNT_UP);
   MAP_TimerMatchSet(TIMER2_BASE, TIMER_A, 0x0010); //
   MAP_TimerPrescaleMatchSet(TIMER2_BASE, TIMER_A, 0x03);
   MAP_TimerControlEvent(TIMER2_BASE, TIMER_A, TIMER_EVENT_POS_EDGE); // config the edge.
   MAP_TimerIntEnable(TIMER2_BASE, TIMER_CAPA_MATCH); // select the IRQ's to enable.
   MAP_IntEnable(INT_TIMER2A); // enable the IRQ 39.
   MAP_TimerEnable(TIMER2_BASE, TIMER_A); // enable the timer. 
}


void main(void){
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
  timerInit();
  //timerCounterInit();
  UART_Init();
  SysTickIntEnable();

  while(1){
    
  } // while
} // main