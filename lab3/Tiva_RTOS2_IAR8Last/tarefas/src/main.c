#include "system_tm4c1294.h" // CMSIS-Core
#include "driverleds.h" // device drivers
#include "cmsis_os2.h" // CMSIS-RTOS
#include "uart_device.h" // device drivers
#include "elevatorfunctions.h" // device drivers

osThreadId_t newMessageThread, UARTTxThread;
osMessageQueueId_t msgTxUARTQueue;

osThreadId_t elevatorControllerThreads[3], elevatorManangerThreads[3];
elevator_t elevators[3];


void main(void){
  SystemInit();
  initUART();
  osKernelInitialize();

  newMessageThread = osThreadNew(newMessage, NULL, NULL);//512 bytes
  UARTTxThread = osThreadNew(uartTx, NULL, NULL);//512 bytes

  osThreadSetPriority(newMessageThread, osPriorityHigh);
  osThreadSetPriority(UARTTxThread, osPriorityHigh);

  msgTxUARTQueue = osMessageQueueNew(MSG_TX_BUFFER_SIZE, MSG_STRING_SIZE, NULL); //64 bytes

  elevators[0].identifier = 'e';
  elevators[1].identifier = 'c';
  elevators[2].identifier = 'd';

  elevators[0].requestMsg = osMessageQueueNew(MSG_TX_BUFFER_SIZE, MSG_STRING_SIZE, NULL);//64 bytes
  elevators[1].requestMsg = osMessageQueueNew(MSG_TX_BUFFER_SIZE, MSG_STRING_SIZE, NULL);//64 bytes
  elevators[2].requestMsg = osMessageQueueNew(MSG_TX_BUFFER_SIZE, MSG_STRING_SIZE, NULL);//64 bytes

  elevatorControllerThreads[0] = osThreadNew(elevatorController, (void *)&elevators[0], NULL);//512 bytes
  elevatorControllerThreads[1] = osThreadNew(elevatorController, (void *)&elevators[1], NULL);
  elevatorControllerThreads[2] = osThreadNew(elevatorController, (void *)&elevators[2], NULL);
  
  elevatorManangerThreads[0] = osThreadNew(elevatorMananger, (void *)&elevators[0], NULL);
  elevatorManangerThreads[1] = osThreadNew(elevatorMananger, (void *)&elevators[1], NULL);
  elevatorManangerThreads[2] = osThreadNew(elevatorMananger, (void *)&elevators[2], NULL);

  elevators[0].timerWaitState = osTimerNew(timerElevatorWaitStateCallback, osTimerOnce, (void *)&elevatorManangerThreads[0], NULL);
  elevators[1].timerWaitState = osTimerNew(timerElevatorWaitStateCallback, osTimerOnce, (void *)&elevatorManangerThreads[1], NULL);
  elevators[2].timerWaitState = osTimerNew(timerElevatorWaitStateCallback, osTimerOnce, (void *)&elevatorManangerThreads[2], NULL);
  
  if(osKernelGetState() == osKernelReady)
    osKernelStart();

  while(1);
} // main
