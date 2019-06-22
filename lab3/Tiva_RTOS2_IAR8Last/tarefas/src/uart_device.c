#include "uart_device.h"
#include <stdint.h>
#include <stdbool.h>


#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
//#include "inc/hw_uart.h"
#include "cmsis_os2.h" // CMSIS-RTOS
#include "elevatorfunctions.h" // device drivers

// standard ASCII symbols
#define CR   0x0D
#define LF   0x0A
#define BS   0x08
#define ESC  0x1B
#define SP   0x20
#define DEL  0x7F

#define GPIO_PORTA_AFSEL_R      (*((volatile uint32_t *)0x40058420))
#define GPIO_PORTA_DEN_R        (*((volatile uint32_t *)0x4005851C))
#define GPIO_PORTA_AMSEL_R      (*((volatile uint32_t *)0x40058528))
#define GPIO_PORTA_PCTL_R       (*((volatile uint32_t *)0x4005852C))
#define UART0_DR_R              (*((volatile uint32_t *)0x4000C000))
#define UART0_FR_R              (*((volatile uint32_t *)0x4000C018))
#define UART_FR_TXFF            0x00000020  // UART Transmit FIFO Full
#define UART_FR_RXFE            0x00000010  // UART Receive FIFO Empty
#define UART0_IBRD_R            (*((volatile uint32_t *)0x4000C024))
#define UART0_FBRD_R            (*((volatile uint32_t *)0x4000C028))
#define UART0_LCRH_R            (*((volatile uint32_t *)0x4000C02C))
#define UART_LCRH_WLEN_8        0x00000060  // 8 bit word length
#define UART_LCRH_FEN           0x00000010  // UART Enable FIFOs
#define UART0_CTL_R             (*((volatile uint32_t *)0x4000C030))
#define UART0_IFLS_R            (*((volatile uint32_t *)0x4000C030))//UART Interrupt FIFO Level Select 
#define UART0_IM_R              (*((volatile uint32_t *)0x4000C038))//UART Interrupt MASK
#define UART0_RIS_R              (*((volatile uint32_t *)0x4000C03C))//UART RAW Interrupt Status
#define UART_CTL_HSE            0x00000020  // High-Speed Enable
#define UART_CTL_UARTEN         0x00000001  // UART Enable
#define UART0_CC_R              (*((volatile uint32_t *)0x4000CFC8))
#define UART_CC_CS_M            0x0000000F  // UART Baud Clock Source
#define UART_CC_CS_SYSCLK       0x00000000  // System clock (based on clock
                                            // source and divisor factor)
#define UART_CC_CS_PIOSC        0x00000005  // PIOSC
#define SYSCTL_ALTCLKCFG_R      (*((volatile uint32_t *)0x400FE138))
#define SYSCTL_ALTCLKCFG_ALTCLK_M                                             \
                                0x0000000F  // Alternate Clock Source
#define SYSCTL_ALTCLKCFG_ALTCLK_PIOSC                                         \
                                0x00000000  // PIOSC
#define SYSCTL_RCGCGPIO_R       (*((volatile uint32_t *)0x400FE608))
#define SYSCTL_RCGCGPIO_R0      0x00000001  // GPIO Port A Run Mode Clock
                                            // Gating Control
#define SYSCTL_RCGCUART_R       (*((volatile uint32_t *)0x400FE618))
#define SYSCTL_RCGCUART_R0      0x00000001  // UART Module 0 Run Mode Clock
                                            // Gating Control
#define SYSCTL_PRGPIO_R         (*((volatile uint32_t *)0x400FEA08))
#define SYSCTL_PRGPIO_R0        0x00000001  // GPIO Port A Peripheral Ready
#define SYSCTL_PRUART_R         (*((volatile uint32_t *)0x400FEA18))
#define SYSCTL_PRUART_R0        0x00000001  // UART Module 0 Peripheral Ready


//extern variables
extern osThreadId_t newMessageThread;
extern osMessageQueueId_t msgTxUARTQueue;
extern elevator_t elevators[3];
/*
void init_uart(void)
{

    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0)){}
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    // Initialize the UART. Set the baud rate, number of data bits, turn off
    // parity, number of stop bits, and stick mode. The UART is enabled by the
    // function call.
    //
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                         UART_CONFIG_PAR_NONE));
    UARTCharPut(UART0_BASE, '!');
    UARTIntEnable(UART0_BASE, UART_INT_RX);
}
*/

void initUART(void){
                                        // activate clock for UART0
  SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R0;
                                        // activate clock for Port A
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R0;
                                        // allow time for clock to stabilize
  while((SYSCTL_PRUART_R&SYSCTL_PRUART_R0) == 0){};
  UART0_CTL_R &= ~UART_CTL_UARTEN;      // disable UART
  UART0_IBRD_R = 8;                     // IBRD = int(16,000,000 / (16 * 115,200)) = int(8.681)
  UART0_FBRD_R = 44;                    // FBRD = round(0.6806 * 64) = 44
                                        // 8 bit word length (no parity bits, one stop bit, FIFOs)
  UART0_LCRH_R = (UART_LCRH_WLEN_8|UART_LCRH_FEN);
                                        // UART gets its clock from the alternate clock source as defined by SYSCTL_ALTCLKCFG_R
  UART0_CC_R = (UART0_CC_R&~UART_CC_CS_M)+UART_CC_CS_PIOSC;
                                        // the alternate clock source is the PIOSC (default)
  SYSCTL_ALTCLKCFG_R = (SYSCTL_ALTCLKCFG_R&~SYSCTL_ALTCLKCFG_ALTCLK_M)+SYSCTL_ALTCLKCFG_ALTCLK_PIOSC;
  UART0_CTL_R &= ~UART_CTL_HSE;         // high-speed disable; divide clock by 16 rather than 8 (default)
  UART0_CTL_R |= UART_CTL_UARTEN;       // enable UART
                                        // allow time for clock to stabilize
  while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R0) == 0){};
  GPIO_PORTA_AFSEL_R |= 0x03;           // enable alt funct on PA1-0
  GPIO_PORTA_DEN_R |= 0x03;             // enable digital I/O on PA1-0
                                        // configure PA1-0 as UART
  GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R&0xFFFFFF00)+0x00000011;
  GPIO_PORTA_AMSEL_R &= ~0x03;          // disable analog functionality on PA
  //UARTCharPut(UART0_BASE, '!');
  UARTFIFOLevelSet(UART0_BASE, UART_FIFO_TX1_8, UART_FIFO_RX1_8);
  UARTIntEnable(UART0_BASE, UART_INT_RX);
  UARTIntRegister(UART0_BASE, UARTRxHandler);
  //UART0_IM_R |= UART_INT_RX;
}
void UARTRxHandler(void)
{
    UARTIntClear(UART0_BASE, UART_INT_RX);
    osThreadFlagsSet(newMessageThread, 0x0001); // sent message to the thread that will read the uart
}

void stringOut(char *pt)
{
    while(*pt){
        UARTCharPut(UART0_BASE, *pt);
        pt++;
    }
}
void uartTx(void *arg)
{
    char msgRcv[MSG_STRING_SIZE];
    uint8_t msgPrior;
    uint8_t charPosition;
    while(1)
    {
        osMessageQueueGet(msgTxUARTQueue, &msgRcv, &msgPrior, osWaitForever);
        charPosition = 0;
        while(msgRcv[charPosition])
        {
            UARTCharPut(UART0_BASE, msgRcv[charPosition]);
            charPosition++;
        }
        UARTCharPut(UART0_BASE, CR);
    }
}


void newMessage(void *arg)
{
    char stringBuffer[MSG_STRING_SIZE];
    char * bufPt;
    uint16_t length;
    char character;
    while(1)
    {
        osThreadFlagsWait(0x0001, osFlagsWaitAny, osWaitForever);
        /* Verify if there is data to receive in UART0*/
        while((UART0_FR_R&UART_FR_RXFE) == 0)
        {        
            length = 0;
            bufPt = &stringBuffer[0];
            character = UARTCharGet(UART0_BASE);
            while(character != CR){
                if(character == BS){
                    if(length){
                        bufPt--;
                        length--;
                    }
                }
                else if(length < MSG_STRING_SIZE){
                    *bufPt = character;
                    bufPt++;
                    length++;
                }
                character = UARTCharGet(UART0_BASE);
            }
            while(character != LF){character = UARTCharGet(UART0_BASE);}
            *bufPt = 0;
            //stringOut(&stringBuffer[0]);
            if(stringBuffer[0] == 'e')
                osMessageQueuePut(elevators[0].requestMsg, &stringBuffer, 4, osWaitForever);
            else if(stringBuffer[0] == 'c')
                osMessageQueuePut(elevators[1].requestMsg, &stringBuffer, 4, osWaitForever);
            else if(stringBuffer[0] == 'd')
                osMessageQueuePut(elevators[2].requestMsg, &stringBuffer, 4, osWaitForever);
        }
    }
}
