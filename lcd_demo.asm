; Some notes on the hardware:
; ATmega16 (clock frequency doesn't matter, tested with 1 MHz to 16 MHz)
; PortA.1 -> LCD RS (register select)
; PortA.2 -> LCD RW (read/write)
; PortA.3 -> LCd E (Enable)
; PortA.4 ... PortA.7 -> LCD data.4 ... data.7
; the other LCD data lines can be left open or tied to ground.

.include "m161def.inc"

; Define how the LCD is connected to the microcontroller
.EQU LCD_PORT=PORTA
.EQU LCD_DDR=DDRA
.EQU LCD_PIN=PinA
.EQU LCD_RS	= 1
.EQU LCD_RW	= 2
.EQU LCD_E	= 3

.EQU CURSOR	= 0
.EQU BLINK	= 0

.def temp	= r16
.def arg    = r17			; argument for calling subroutines
.def return	= r18			; return value from subroutines
.CSEG

rjmp reset
.ORG $10

; include LCD routines
.include "./lcd.inc"

reset:
	ldi	temp, low(RAMEND)
	out	SPL, temp
	ldi	temp, high(RAMEND)
	out	SPH, temp

	rcall LCD_init
	rcall LCD_wait

	LCD_print z1
	LCD_print z3
	LCD_print z2
	LCD_print z4

loop:						; Main loop
	rjmp loop

; Define some constants
;            12345678901234567890
z1:	.DB		"*** Erste  Zeile ***",0,0	; byte const + ctr + lf + zero mark
z2: .DB		"*** Zweite Zeile ***",0,0
z3: .DB		"*** Dritte Zeile ***",0,0
z4: .DB		"*** Vierte Zeile ***",0,0

.EXIT
