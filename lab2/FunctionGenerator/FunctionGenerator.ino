/*
  Simple Waveform generator with Arduino Due

  * connect two push buttons to the digital pins 2 and 3 
    with a 10 kilohm pulldown resistor to choose the waveform
    to send to the DAC0 and DAC1 channels
  * connect a 10 kilohm potentiometer to A0 to control the 
    signal frequency

 */

#include "Waveforms.h"

#define oneHzSample 1000000/maxSamplesNum  // sample for the 1Hz signal expressed in microseconds 

const int button0 = 2;//, button1 = 3;
const int generatorPinOut = 3;
int i = 0;
unsigned int sample;
int a;

void setup() {
  attachInterrupt(button0, wave0Select, RISING);  // Interrupt attached to the button connected to pin 2
    //Initialize serial and wait for port to open:
  //pinMode(generatorPinOut, OUTPUT);
  //TIMSK2 = (TIMSK2 & B11111110) | 0x01;
  //TCCR2B = (TCCR2B & B11111000) | 0x07;
  //OCR2A = 10;
  //OCR2A = 20;
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // prints title with ending line break
  Serial.println("Finish Setup");
}

void loop() {

}

// function hooked to the interrupt on digital pin 2
void wave0Select() {
  noTone(generatorPinOut);
  sample = map(analogRead(A0), 0, 1023, 1, 65000);
  tone(generatorPinOut, sample);
  Serial.print("Frequency Generator: ");
  Serial.println(sample);
}
