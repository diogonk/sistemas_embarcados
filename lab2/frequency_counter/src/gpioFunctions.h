#include <stdbool.h>
#include <stdint.h>

void buttonsInit(void);
uint8_t readButtons(void);
void writeValues(uint32_t readCounter, uint32_t lastCounter);
