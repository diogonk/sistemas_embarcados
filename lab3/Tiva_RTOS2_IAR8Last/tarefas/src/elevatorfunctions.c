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

enum elevatorStatus
{
    RESET,
    IDLE,
    GOING_DOWN,
    GOING_UP,
    WAITING
};
enum doorsStatus
{
    OPENED,
    CLOSED
};
char letters[26] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

extern osMessageQueueId_t msgTxUARTQueue;

void findNextFloor(elevator_t *elevator, uint16_t floortoStop);

void elevatorController(void *arg)
{

    elevator_t *elevator = (elevator_t *)arg;
    elevator->status = RESET;
    elevator->next_status = IDLE;
    elevator->nextFloor = 0;
    elevator->currentFloor = 0;
    elevator->doors = CLOSED;
    char strTx[MSG_STRING_SIZE]; //8bytes
    while (1)
    {
        switch (elevator->status)
        {
        case RESET:
            sprintf(strTx, "%cr", elevator->identifier);
            osMessageQueuePut(msgTxUARTQueue, &strTx, 4, osWaitForever);
            while (elevator->doors == CLOSED)
            {
            }
            elevator->status = IDLE;
            break;

        case IDLE:
            if (elevator->nextFloor > elevator->currentFloor)
            {
                sprintf(strTx, "%cf", elevator->identifier);
                osMessageQueuePut(msgTxUARTQueue, &strTx, 4, osWaitForever);
                while (elevator->doors == OPENED)
                {
                }
                sprintf(strTx, "%cs", elevator->identifier);
                osMessageQueuePut(msgTxUARTQueue, &strTx, 4, osWaitForever);
                elevator->status = GOING_UP;
            }
            else if (elevator->nextFloor < elevator->currentFloor)
            {
                sprintf(strTx, "%cf", elevator->identifier);
                osMessageQueuePut(msgTxUARTQueue, &strTx, 4, osWaitForever);
                while (elevator->doors == OPENED)
                {
                }
                sprintf(strTx, "%cd", elevator->identifier);
                osMessageQueuePut(msgTxUARTQueue, &strTx, 4, osWaitForever);
                elevator->status = GOING_DOWN;
            }
            break;

        case GOING_DOWN:
        case GOING_UP:
            if (elevator->currentFloor == elevator->nextFloor)
            {
                sprintf(strTx, "%cp", elevator->identifier);
                osMessageQueuePut(msgTxUARTQueue, &strTx, 4, osWaitForever);
                sprintf(strTx, "%ca", elevator->identifier);
                osMessageQueuePut(msgTxUARTQueue, &strTx, 4, osWaitForever);
                osTimerStart(elevator->timerWaitState, 5000);
                elevator->status = WAITING;
            }
            else if (elevator->doors == OPENED)
            {
                sprintf(strTx, "%cf", elevator->identifier);
                osMessageQueuePut(msgTxUARTQueue, &strTx, 4, osWaitForever);
                while (elevator->doors == OPENED)
                {
                }
                if (elevator->status == GOING_DOWN)
                    sprintf(strTx, "%cd", elevator->identifier);
                else
                    sprintf(strTx, "%cs", elevator->identifier);
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
{ //22bytes
    char msgRcv[MSG_STRING_SIZE];
    uint8_t msgPrior;
    elevator_t *elevator = (elevator_t *)arg;
    uint16_t floortoStop = 0;
    uint16_t internalReq = 0;
    uint16_t externalReqDown = 0;
    uint16_t floorStatus;
    uint16_t externalReqUp = 0;
    int newRequest;
    uint8_t floorUpdate;
    while (1)
    {
        if (osMessageQueueGet(elevator->requestMsg, &msgRcv, &msgPrior, 1000) == osOK)
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
                sscanf(msgRcv, "%*c%*c%d%*c", &newRequest);
                if (newRequest >= 0 && newRequest <= 15)
                {
                    if (msgRcv[4] == 'd')
                    {
                        externalReqDown |= 1 << newRequest;
                    }
                    else if (msgRcv[4] == 's')
                    {
                        externalReqUp |= 1 << newRequest;
                    }
                }
                break;

            case 'I':
                /*Internal Requisition*/
                internalReq |= 1 << (msgRcv[2] - 97); //97 is the offset ascii. a = 97, 0 floor;
                break;

            default:
                sscanf(msgRcv, "%*c%d", &newRequest);
                elevator->currentFloor = newRequest;
                break;
            }
        }
        if (elevator->status != RESET)
        {
            for (floorUpdate = 0; floorUpdate < 16; floorUpdate++)
            {
                floorStatus = (1 << floorUpdate);
                if (externalReqDown & floorStatus)
                {
                    if (elevator->status == GOING_DOWN && elevator->next_status != GOING_UP)
                    {
                        if (elevator->currentFloor > floorUpdate)
                        {
                            floortoStop |= floorStatus;
                            externalReqDown &= ~(floorStatus);
                        }
                    }
                    else if (elevator->status == GOING_UP && elevator->next_status == GOING_DOWN)
                    {
                        floortoStop |= floorStatus;
                        externalReqDown &= ~(floorStatus);
                    }
                    if (elevator->next_status == IDLE && elevator->status == IDLE)
                    {
                        floortoStop |= floorStatus;
                        externalReqDown &= ~(floorStatus);
                        elevator->next_status = GOING_DOWN;
                    }
                }
                if (externalReqUp & floorStatus)
                {
                    if (elevator->status == GOING_UP && elevator->next_status != GOING_DOWN)
                    {
                        if (elevator->currentFloor < floorUpdate)
                        {
                            floortoStop |= floorStatus;
                            externalReqUp &= ~(floorStatus);
                        }
                    }
                    else if (elevator->status == GOING_DOWN && elevator->next_status == GOING_UP)
                    {
                        floortoStop |= floorStatus;
                        externalReqUp &= ~(floorStatus);
                    }
                    if (elevator->next_status == IDLE && elevator->status == IDLE)
                    {
                        floortoStop |= floorStatus;
                        externalReqUp &= ~(floorStatus);
                        elevator->next_status = GOING_UP;
                    }
                }
                if (internalReq & floorStatus)
                {
                    if (elevator->status == GOING_UP)
                    {
                        if (floorUpdate > elevator->currentFloor)
                        {
                            floortoStop |= floorStatus;
                            internalReq &= ~(floorStatus);
                        }
                    }
                    else if (elevator->status == GOING_DOWN)
                    {
                        if (floorUpdate < elevator->currentFloor)
                        {
                            floortoStop |= floorStatus;
                            internalReq &= ~(floorStatus);
                        }
                    }
                    else if (elevator->status == WAITING)
                    {
                        if (elevator->next_status == GOING_UP)
                        {
                            if (floorUpdate > elevator->currentFloor)
                            {
                                floortoStop |= floorStatus;
                                internalReq &= ~(floorStatus);
                            }
                        }
                        else if (elevator->next_status == GOING_DOWN)
                        {
                            if (floorUpdate < elevator->currentFloor)
                            {
                                floortoStop |= floorStatus;
                                internalReq &= ~(floorStatus);
                            }
                        }
                    }
                    else if (elevator->status == IDLE)
                    {
                        floortoStop |= floorStatus;
                        internalReq &= ~(floorStatus);
                    }
                }
            }

            if(elevator->currentFloor == elevator->nextFloor && (floortoStop & (1<<elevator->nextFloor)))
                floortoStop &= ~(1<<elevator->nextFloor);
            
            findNextFloor(elevator, floortoStop);

            if (floortoStop == 0)
            {
                elevator->next_status = IDLE;
            }
        }
    }
}

void findNextFloor(elevator_t *elevator, uint16_t floortoStop)
{
    uint8_t floorUpdate;
    uint16_t floorStatus;

    /*UPDATE NEXT FLOOR*/
    switch (elevator->status)
    {
    case IDLE: //if its in idle, find the first requisition
        for (floorUpdate = 0; floorUpdate < 16; floorUpdate++)
        {
            floorStatus = (1 << floorUpdate);
            if ((floortoStop & floorStatus))
            {
                if (elevator->nextFloor == elevator->currentFloor)
                    elevator->nextFloor = floorUpdate;
            }
        }
        break;

    case GOING_DOWN:
        for (floorUpdate = elevator->currentFloor; floorUpdate > 0 && floorUpdate < elevator->currentFloor; floorUpdate--)
        {
            floorStatus = (1 << floorUpdate);
            if ((floortoStop & floorStatus))
            {
                if (elevator->next_status == GOING_UP)
                {
                    if (floorUpdate < elevator->nextFloor)
                    {
                        elevator->nextFloor = floorUpdate;
                    }
                }
                else
                {
                    if (floorUpdate > elevator->nextFloor)
                    {
                        elevator->nextFloor = floorUpdate;
                    }
                }
            }
            break;
        }
    case GOING_UP:
        for (floorUpdate = elevator->currentFloor; floorUpdate < 16; floorUpdate++)
        {
            floorStatus = (1 << floorUpdate);
            if ((floortoStop & floorStatus))
            {
                if (elevator->next_status == GOING_DOWN)
                {
                    if (floorUpdate > elevator->nextFloor)
                    {
                        elevator->nextFloor = floorUpdate;
                    }
                }
                else
                {
                    if (floorUpdate < elevator->nextFloor)
                    {
                        elevator->nextFloor = floorUpdate;
                    }
                }
            }
        }
        break;
    case WAITING:

        if (osThreadFlagsWait(0x0001, osFlagsWaitAny, osWaitForever) == 0x0001) // wait for timer to exit waiting
        {
            for (floorUpdate = 0; floorUpdate < 16; floorUpdate++)
            {
                floorStatus = (1 << floorUpdate);
                if ((floortoStop & floorStatus))
                {
                    if (elevator->next_status == GOING_DOWN)
                    {
                        if (floorUpdate < elevator->currentFloor)
                        {
                            elevator->nextFloor = floorUpdate;
                        }
                    }
                    else if (elevator->next_status == GOING_UP)
                    {
                        if (floorUpdate > elevator->currentFloor)
                        {
                            elevator->nextFloor = floorUpdate;
                        }
                    }
                }
            }
            elevator->status = elevator->next_status;
        }
        break;
    default:
        break;
    }
}

void timerElevatorWaitStateCallback(void *arg)
{
    osThreadId_t *elevatorThread = (osThreadId_t *)arg;
    osThreadFlagsSet(*elevatorThread, 0x0001);
}