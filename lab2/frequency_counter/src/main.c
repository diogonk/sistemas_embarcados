#include <stdbool.h>
#include <stdint.h>
//#include "inc/tm4c1294ncpdt.h" // CMSIS-Core

#include "driverlib/sysctl.h" // driverlib
#include "gpioFunctions.h"
#include "timerFunctions.h"
#include "driverlib/systick.h"
#include "UART.h"
#include "definitions.h"

uint32_t timerValue;
uint32_t lastValue;
uint16_t newAcquire = 0;
uint16_t FrequencyDivider = 1;


void systemStart()
{
    uint32_t ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                            SYSCTL_OSC_MAIN |
                                            SYSCTL_USE_PLL |
                                            SYSCTL_CFG_VCO_480),
                                            FREQMIN);
    SysTickEnable();
    SysTickPeriodSet(FREQMIN/FrequencyDivider);
    buttonsInit();
    UART_Init();
    timerCounterInit();
    SysTickIntEnable();
    
}

void main(void){
  uint8_t settingsValue;
  systemStart();
  
  while(1){
    settingsValue = readButtons();
    if(settingsValue != 0)
      changesysTick(settingsValue);

    if( newAcquire == 1)
    {
      newAcquire = 0;
      writeValues(timerValue, lastValue);
      lastValue = timerValue;
    }
  } // while
} // main