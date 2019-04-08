; -------------------------------------------------------------------------------
    PUBLIC count_pulse_hz

    SECTION .text : CODE (2)
    THUMB                           ;Instrucoes tipo THUMB-2
; -------------------------------------------------------------------------------

GPIO_PORTJ_AHB_DATA_BITS_R  EQU    0x40060000
GPIO_PORTK_AHB_DATA_BITS_R  EQU    0x40061000

; -------------------------------------------------------------------------------

count_pulse_hz
  ; Funcao read_freq_hz
  ; Parametro de entrada: N�o tem
  ; Parametro de saida: R0 - Contagem de pulsos alternados
  PUSH {R4,R5,R6,R7}
  MOV R0, #0        ;toogle counter
  MOV R1, #0        ;last state
  MOV R2, #0        ;read gpio
  MOV R3, #0        ;time acquire
  LDR R4, =GPIO_PORTK_AHB_DATA_BITS_R				;Carrega o valor do offset do data register
  ADD R4, R4, #0x04	    ;Soma ao offset o endereço do bit0 para leitura amigavel [BIT0]
  MOV R5, #0        ;time sampling
  MOV R6, #0        ;for khz purpose
  MOV R7, #0x3600   ; 240E6 
  MOVT R7, #0x016E   ; são 240M ciclos de máquina, o que representa 1 segundo
NEWSAMPLING
  LDR R2, [R4]      ;Leitura GPIO
  CMP R2, R1
  ITT NE
  ADDNE R0, #0x01   ;estado anterior diferente do atual, conta um pulso
  MOVNE R1, R2      ;salva estado anterior
  CMP R3, R7        ;verifica se já terminou a aquisição
  BGE EXITFUNC
  
  NOP               ;Just to have even number of cycles
  ADD R5, R5, #10
WAITSAMPLING
  NOP
  CMP R5, #240     
  ITTT GE
  ADDGE R5, R5, #9
  ADDGE R3, R3, R5
  SUBGE R5, R5, #240
  BGE NEWSAMPLING

  ADD R5, R5, #10
  B WAITSAMPLING

EXITFUNC
  POP {R4,R5,R6,R7}
  BX LR

  END