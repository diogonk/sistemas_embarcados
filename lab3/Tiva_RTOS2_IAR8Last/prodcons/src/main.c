#include "system_tm4c1294.h" // CMSIS-Core
#include "driverleds.h" // device drivers
#include "cmsis_os2.h" // CMSIS-RTOS

#define BUFFER_SIZE 8

osThreadId_t produtor_id, consumidor_id;
osSemaphoreId_t vazio_id, cheio_id;
osMessageQueueId_t msg_queue_id;

uint8_t buffer[BUFFER_SIZE];

void produtor(void *arg){
  uint8_t index_i = 0, count = 0;
  
  while(1){
    /*
    osSemaphoreAcquire(vazio_id, osWaitForever); // há espaço disponível?
    buffer[index_i] = count; // coloca no buffer
    osSemaphoreRelease(cheio_id); // sinaliza um espaço a menos
    
    index_i++; // incrementa índice de colocação no buffer
    if(index_i >= BUFFER_SIZE)
      index_i = 0;
    */
    osMessageQueuePut(msg_queue_id, &count, 4, osWaitForever);

    count++;
    count &= 0x0F; // produz nova informação
    osDelay(500);
  } // while
} // produtor

void consumidor(void *arg){
  uint8_t index_o = 0, state;
  void * msg_rcv;
  
  while(1){
    uint8_t msg_prior;
    uint32_t queue_count;
    //osSemaphoreAcquire(cheio_id, osWaitForever); // há dado disponível?
    //state = buffer[index_o]; // retira do buffer
    //osSemaphoreRelease(vazio_id); // sinaliza um espaço a mais
    
    //index_o++;
    //if(index_o >= BUFFER_SIZE) // incrementa índice de retirada do buffer
    //  index_o = 0;
    //queue_count = osMessageQueueGetCount(msg_queue_id);
    osMessageQueueGet(msg_queue_id, &state, &msg_prior, osWaitForever);
    //state = (uint8_t) *msg_rcv;
    LEDWrite(LED4|LED3|LED2|LED1, state); // apresenta informação consumida
    osDelay(500);
  } // while
} // consumidor

void main(void){
  SystemInit();
  LEDInit(LED4|LED3|LED2|LED1);

  osKernelInitialize();

  produtor_id = osThreadNew(produtor, NULL, NULL);
  consumidor_id = osThreadNew(consumidor, NULL, NULL);
  msg_queue_id = osMessageQueueNew(BUFFER_SIZE, sizeof(uint8_t), NULL);
  
  //vazio_id = osSemaphoreNew(BUFFER_SIZE, BUFFER_SIZE, NULL); // espaços disponíveis = BUFFER_SIZE
  //cheio_id = osSemaphoreNew(BUFFER_SIZE, 0, NULL); // espaços ocupados = 0
  
  if(osKernelGetState() == osKernelReady)
    osKernelStart();

  while(1);
} // main
