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
  PUSH {R1, R2, R3, R4, R5}
  MOV R0, #0        ;toogle counter
  MOV R2, #0        ;read gpio
  MOV R3, #0        ;time acquire
  LDR R4, =GPIO_PORTK_AHB_DATA_BITS_R				;Carrega o valor do offset do data register
  ADD R4, R4, #0x04	    ;Soma ao offset o endereço do bit0 para leitura amigavel [BIT0]
  //MOV  R5, #0xE1C0    ; Em 8 ciclos de máquina, 15.000.000 de leitura podem ser realizadas 
  //MOVT R5, #0x00E4    ; considerando um PLL de 120MHz
  MOV  R5, #0xC6C0    ; Em 8 ciclos de máquina, 3.000.000 de leitura podem ser realizadas 
  MOVT R5, #0x002D    ; considerando um PLL de 24MHz
  LDR R1, [R4]      ;Last State

NEWSAMPLING
  LDR R2, [R4]      ;Leitura GPIO
  CMP R2, R1
  ITT NE
  ADDNE R0, #0x01   ;estado anterior diferente do atual, conta um pulso
  MOVNE R1, R2      ;salva estado anterior
  SUBS R5, #1     
  BPL NEWSAMPLING   ;verifica se já terminou a aquisição
  

//EXITFUNC
  POP {R1, R2, R3, R4, R5}
  BX LR

  END