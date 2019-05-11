//Device Driver: GPIO
#include "gpioFunctions.h"



#include "inc/hw_types.h"

#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h" // driverlib
#include "driverlib/gpio.h"
#include "driverlib/systick.h"
#include "driverlib/pin_map.h"
#include "inc/tm4c1294ncpdt.h"
#include "definitions.h"
#include "UART.h"


void buttonsInit(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ); // Habilita GPIO J (push-button SW1 = PJ0, push-button SW2 = PJ1)
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ)); // Aguarda final da habilitacao
    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1); // push-buttons SW1 e SW2 como entrada
    GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}
uint8_t readButtons(void)
{
    uint32_t buttonsValue;
    buttonsValue = GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    if(buttonsValue == GPIO_PIN_0) 
        return 1;
    else if(buttonsValue == GPIO_PIN_1)
        return 2;

    return 0;
}
void writeValues(uint32_t readCounter, uint32_t lastCounter)
{
    UART_OutUDec(readCounter>=lastCounter?
    (readCounter-lastCounter)*FrequencyDivider : 
    ((MAXCOUNTER-lastCounter)+readCounter)*FrequencyDivider);
    OutCRLF();
}