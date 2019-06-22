#include "elevatorfunctions.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "cmsis_os2.h" // CMSIS-RTOS
#include "uart_device.h"

//  STATUS
//  0 - RESET
//  1 - IDLE
//  2 - GOING DOWN
//  3 - GOING UP
//  4 - WAITING

enum elevatorStatus {RESET, IDLE, GOING_DOWN, GOING_UP, WAITING};
enum doorsStatus {OPENED, CLOSED};
char letters[26] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'};

extern osMessageQueueId_t msgTxUARTQueue;

void elevatorController(void *arg)
{

    elevator_t * elevator = (elevator_t *) arg;
    elevator->status = RESET;
    elevator->next_status = IDLE;
    elevator->nextFloor = 0;
    elevator->currentFloor = 0;
    elevator->doors = CLOSED;
    char strTx[MSG_STRING_SIZE];//8bytes
    while(1)
    {
        switch (elevator->status)
        {
        case RESET:
            sprintf(strTx, "%cr",elevator->identifier);
            osMessageQueuePut(msgTxUARTQueue, &strTx, 4, osWaitForever);
            while(elevator->doors == CLOSED){}
            elevator->status = IDLE;
            break;

        case IDLE:
            if(elevator->nextFloor > elevator->currentFloor)
            {
                sprintf(strTx, "%cf",elevator->identifier);
                osMessageQueuePut(msgTxUARTQueue, &strTx, 4, osWaitForever);
                while(elevator->doors == OPENED){}
                sprintf(strTx, "%cs",elevator->identifier);
                osMessageQueuePut(msgTxUARTQueue, &strTx, 4, osWaitForever);
                elevator->status = GOING_UP;
            }
            else if(elevator->nextFloor < elevator->currentFloor)
            {
                sprintf(strTx, "%cf",elevator->identifier);
                osMessageQueuePut(msgTxUARTQueue, &strTx, 4, osWaitForever);
                while(elevator->doors == OPENED){}
                sprintf(strTx, "%cd",elevator->identifier);
                osMessageQueuePut(msgTxUARTQueue, &strTx, 4, osWaitForever);
                elevator->status = GOING_DOWN;
            }
            break;

        case GOING_DOWN:
        case GOING_UP:
            if(elevator->currentFloor == elevator->nextFloor)
            {
                sprintf(strTx, "%cp",elevator->identifier);
                osMessageQueuePut(msgTxUARTQueue, &strTx, 4, osWaitForever);
                sprintf(strTx, "%ca",elevator->identifier);
                osMessageQueuePut(msgTxUARTQueue, &strTx, 4, osWaitForever);
                osTimerStart(elevator->timerWaitState, 5000);
                elevator->status = WAITING;
            }
            else if(elevator->doors == OPENED)
            {
                sprintf(strTx, "%cf",elevator->identifier);
                osMessageQueuePut(msgTxUARTQueue, &strTx, 4, osWaitForever);
                while(elevator->doors == OPENED){}
                if(elevator->status == GOING_DOWN)
                    sprintf(strTx, "%cd",elevator->identifier);
                else
                    sprintf(strTx, "%cs",elevator->identifier);
                osMessageQueuePut(msgTxUARTQueue, &strTx, 4, osWaitForever);
            }
            break;

        case WAITING:
            break;

        default:
            break;
        }
    }
}


void elevatorMananger(void *arg)
{   //22bytes
    char msgRcv[MSG_STRING_SIZE];
    uint8_t msgPrior;
    elevator_t * elevator = (elevator_t *) arg;
    uint16_t floortoStop=0;
    uint16_t externalReqDown=0;
    uint16_t floorStatus;
    uint16_t externalReqUp=0;
    int newRequest;
    uint8_t floorUpdate;
    while(1)
    {
        if(osMessageQueueGet(elevator->requestMsg, &msgRcv, &msgPrior, 0) == osOK)
        {
            switch (msgRcv[1])
            {
            case 'F':
                elevator->doors = CLOSED;
                break;

            case 'A':
                elevator->doors = OPENED;
                break;

            case 'E':
                /*External Requisition*/
                sscanf(msgRcv,"%*c%*c%d%*c", &newRequest);
                if(newRequest>=0 && newRequest <= 15)
                {
                    if(msgRcv[4] == 'd')
                    {
                        externalReqDown |= 1 << newRequest;
                    }
                    else if(msgRcv[4] == 's')
                    {
                        externalReqUp |= 1 << newRequest;
                    }
                }
                break;

            case 'I':
                /*Internal Requisition*/
                floortoStop |= 1 << (msgRcv[2] - 97); //97 is the offset ascii. a = 97, 0 floor;
                break;

            default:
                sscanf(msgRcv,"%*c%d", &newRequest);
                elevator->currentFloor = newRequest;
                break;
            }
        }
        if(elevator->status != RESET)
        {
            for(floorUpdate = 0; floorUpdate<16; floorUpdate++)
            {
                floorStatus = (1 << floorUpdate);
                if(externalReqDown & floorStatus)
                {
                    if(elevator->status == GOING_DOWN && elevator->next_status == GOING_DOWN)
                    {
                        if(elevator->currentFloor>floorUpdate)
                        {
                            floortoStop |=  floorStatus;
                            externalReqDown &= ~(floorStatus);
                        }
                    }
                    else if(elevator->status == GOING_UP && elevator->next_status == GOING_DOWN)
                    {
                        floortoStop |=  floorStatus;
                        externalReqDown &= ~(floorStatus);
                    }
                    if(elevator->next_status == IDLE)
                    {
                        floortoStop |=  floorStatus;
                        externalReqDown &= ~(floorStatus);
                        elevator->next_status = GOING_DOWN;
                    }

                }
                if(externalReqUp & floorStatus)
                {
                    if(elevator->status == GOING_UP && elevator->next_status == GOING_UP)
                    {
                        if(elevator->currentFloor<floorUpdate)
                        {
                            floortoStop |=  floorStatus;
                            externalReqUp &= ~(floorStatus);
                        }
                    }
                    else if(elevator->status == GOING_DOWN && elevator->next_status == GOING_UP)
                    {
                        floortoStop |=  floorStatus;
                        externalReqUp &= ~(floorStatus);
                    }
                    if(elevator->next_status == IDLE)
                    {
                        floortoStop |=  floorStatus;
                        externalReqUp &= ~(floorStatus);
                        elevator->next_status = GOING_UP;
                    }
                }
                if((floortoStop & floorStatus))
                {
                    /*UPDATE NEXT FLOOR*/
                    switch (elevator->status)
                    {
                    case IDLE:
                        elevator->nextFloor = floorUpdate;
                        break;

                    case GOING_DOWN:
                        if(floorUpdate > elevator->nextFloor)
                        {
                            elevator->nextFloor = floorUpdate;
                        }
                        break;

                    case GOING_UP:
                        if(floorUpdate < elevator->nextFloor)
                        {
                            elevator->nextFloor = floorUpdate;
                        }

                        break;
                    case WAITING:
                        break;
                    default:
                        break;
                    }
                }
            }

            if(floortoStop == 0)
            {
                elevator->status = IDLE;
                elevator->next_status = IDLE;
            }

            if(osThreadFlagsWait(0x0001, osFlagsWaitAny, 0) ==  0x0001)
            {
                //osTimerStop(elevator->timerWaitState);
                floortoStop &= ~(1 << elevator->currentFloor);
                if(elevator->next_status == GOING_UP)
                {
                    for(floorUpdate = elevator->currentFloor; floorUpdate<16; floorUpdate++)
                    {
                        floorStatus = (1 << floorUpdate);
                        if(floortoStop & floorStatus)
                            elevator->nextFloor = floorUpdate;
                    }
                }
                else if(elevator->next_status == GOING_DOWN)
                {
                    for(floorUpdate = elevator->currentFloor; floorUpdate>0; floorUpdate--)
                    {
                        floorStatus = (1 << floorUpdate);
                        if(floortoStop & floorStatus)
                            elevator->nextFloor = floorUpdate;
                    }
                    if(floortoStop & (1 << 0))
                        elevator->nextFloor = floorUpdate;
                }

                elevator->status = elevator->next_status;
            }
        }
    }
}

void timerElevatorWaitStateCallback(void *arg)
{
    osThreadId_t * elevatorManangerThread = (osThreadId_t*)arg;
    osThreadFlagsSet(*elevatorManangerThread, 0x0001);
}