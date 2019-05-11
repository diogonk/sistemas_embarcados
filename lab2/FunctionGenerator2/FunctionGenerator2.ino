const byte LED = 10;  // Timer 2 "A" output: OC2A

void setup() {
 pinMode (LED, OUTPUT);
 
 // set up Timer 2
 TCCR2A = _BV (COM2A0) | _BV(WGM21);  // CTC, toggle OC2A on Compare Match
 TCCR2B = _BV (CS20);   // No prescaler
 OCR2A =  1;           // http://eleccelerator.com/avr-timer-calculator/  200kHz
                           
} 

void loop() { }
