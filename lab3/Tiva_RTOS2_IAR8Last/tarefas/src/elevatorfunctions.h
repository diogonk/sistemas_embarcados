//elevatorFunctions.h
#ifndef ELEVATOR_FUNCTIONS_H_
#define ELEVATOR_FUNCTIONS_H_

#include <stdint.h>
#include "cmsis_os2.h" // CMSIS-RTOS

typedef struct elevator_t{
    uint8_t currentFloor;
    uint8_t nextFloor;
    uint8_t status;
    uint8_t next_status;
    uint8_t doors;                  
    unsigned char identifier;       // 6 bytes
    osTimerId_t timerWaitState;     //96 bytes
    osMessageQueueId_t requestMsg;  //64 bytes
} elevator_t;   // 166 bytes

void elevatorController(void *arg);
void elevatorMananger(void *arg);
void timerElevatorWaitStateCallback(void *arg);

#endif //ELEVATOR_FUNCTIONS_H_