//library to control uart
#ifndef UART_DEVICE_H_
#define UART_DEVICE_H_

void initUART(void);
void UARTRxHandler(void);
void newMessage(void *arg);
void uartTx(void *arg);

#define MSG_TX_BUFFER_SIZE 8
#define MSG_STRING_SIZE 8

#endif //UART_DEVICE_H_