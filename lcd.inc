lcd_command8:				; used for init (we need some 8-bit commands to switch to 4-bit mode!)
	push temp				; prepare temporary variable
	in	temp, LCD_DDR		; we need to set the high nibble of LCD_DDR while leaving
							; the other bits untouched. Using temp for that.
	sbr	temp, 0b11110000	; set high nibble in temp
	out	LCD_DDR, temp		; write value to LCD_DDR again
	in	temp, LCD_PORT		; then get the port value
	cbr	temp, 0b11110000	; and clear the data bits
	cbr	arg, 0b00001111		; then clear the low nibble of the argument
							; so that no control line bits are overwritten
	or	temp, arg			; then set the data bits (from the argument) in the
							; port value
	out	LCD_PORT, temp		; and write the port value.
	sbi	LCD_PORT, LCD_E		; now strobe LCD_E
	nop
	nop
	nop
	cbi	LCD_PORT, LCD_E
	in	temp, LCD_DDR		; get LCD_DDR to make the data lines input again
	cbr	temp, 0b11110000	; clear data line direction bits
	out	LCD_DDR, temp		; and write to LCD_DDR
	pop	temp
ret

lcd_putchar:
	push temp				; prepare temporary variable
	push arg				; save the argmuent (it's destroyed in between)
	in temp, LCD_DDR		; get data direction bits
	sbr	temp, 0b11110000	; set the data lines to output
	out	LCD_DDR, temp		; write value to LCD_DDR
	in temp, LCD_PORT		; then get the data from LCD_PORT
	cbr temp, 0b11111110	; clear ALL LCD lines (data and control!)
	cbr arg, 0b00001111		; we have to write the high nibble of our argument first
							; so mask off the low nibble
	or temp, arg			; now set the argument bits in the port value
	out	LCD_PORT, temp		; and write the port value
	sbi	LCD_PORT, LCD_RS	; now take RS high for LCD char data register access
	sbi	LCD_PORT, LCD_E		; strobe LCD_E
	nop
	nop
	nop
	cbi	LCD_PORT, LCD_E
	pop arg					; restore the argument, we need the low nibble now...
	cbr temp, 0b11110000	; clear the data bits of our port value
	swap arg				; we want to write the LOW nibble of the argument to
							; the LCD data lines, which are the HIGH port nibble!
	cbr	arg, 0b00001111		; clear unused bits in argument
	or temp, arg			; and set the required argument bits in the port value
	out	LCD_PORT, temp		; write data to port
	sbi	LCD_PORT, LCD_RS	; again, set RS
	sbi	LCD_PORT, LCD_E		; strobe LCD_E
	nop
	nop
	nop
	cbi	LCD_PORT, LCD_E
	cbi	LCD_PORT, LCD_RS
	in temp, LCD_DDR
	cbr	temp, 0b11110000	; data lines are input again
	out	LCD_DDR, temp
	pop	temp
ret

lcd_command:				; same as LCD_putchar, but with RS low!
	push temp				; prepare temporary variable
	push arg
	in temp, LCD_DDR
	sbr temp, 0b11110000
	out	LCD_DDR, temp
	in temp, LCD_PORT
	cbr temp, 0b11111110
	cbr arg, 0b00001111
	or temp, arg

	out	LCD_PORT, temp
	sbi	LCD_PORT, LCD_E
	nop
	nop
	nop
	cbi	LCD_PORT, LCD_E
	pop arg
	cbr	temp, 0b11110000
	swap arg
	cbr	arg, 0b00001111
	or temp, arg
	out	LCD_PORT, temp
	sbi	LCD_PORT, LCD_E
	nop
	nop
	nop
	cbi	LCD_PORT, LCD_E
	in temp, LCD_DDR
	cbr	temp, 0b11110000
	out	LCD_DDR, temp
	pop	temp
ret

LCD_getchar:
	push temp				; prepare temporary variable
	in temp, LCD_DDR		; make sure the data lines are inputs
	andi temp, 0b00001111	; so clear their DDR bits
	out	LCD_DDR, temp
	sbi	LCD_PORT, LCD_RS	; we want to access the char data register, so RS high
	sbi	LCD_PORT, LCD_RW	; we also want to read from the LCD -> RW high
	sbi	LCD_PORT, LCD_E		; while E is high
	nop
	in temp, LCD_PIN		; we need to fetch the HIGH nibble
	andi temp, 0b11110000	; mask off the control line data
	mov	return, temp		; and copy the HIGH nibble to return
	cbi	LCD_PORT, LCD_E		; now take E low again
	nop						; wait a bit before strobing E again
	nop	
	sbi	LCD_PORT, LCD_E		; same as above, now we're reading the low nibble
	nop
	in temp, LCD_PIN		; get the data
	andi temp, 0b11110000	; and again mask off the control line bits
	swap temp				; temp HIGH nibble contains data LOW nibble! so swap
	or return, temp			; and combine with previously read high nibble
	cbi	LCD_PORT, LCD_E		; take all control lines low again
	cbi	LCD_PORT, LCD_RS
	cbi	LCD_PORT, LCD_RW
	pop	temp
ret							; the character read from the LCD is now in return

LCD_getaddr:				; works just like LCD_getchar, but with RS low,
							; return.7 is the busy flag
	push temp				; prepare temporary variable
	in temp, LCD_DDR
	andi temp, 0b00001111
	out	LCD_DDR, temp
	cbi	LCD_PORT, LCD_RS
	sbi	LCD_PORT, LCD_RW
	sbi	LCD_PORT, LCD_E
	nop
	in temp, LCD_PIN
	andi temp, 0b11110000
	mov	return, temp
	cbi	LCD_PORT, LCD_E
	nop
	nop
	sbi	LCD_PORT, LCD_E
	nop
	in temp, LCD_PIN
	andi temp, 0b11110000
	swap temp
	or return, temp
	cbi	LCD_PORT, LCD_E
	cbi	LCD_PORT, LCD_RW
	pop	temp
ret

LCD_wait:					; read address and busy flag until busy flag cleared
	rcall LCD_getaddr
	andi return, 0x80
	brne LCD_wait
	ret

LCD_delay:
	clr	r2
	LCD_delay_outer:
	clr	r3
	LCD_delay_inner:
	dec	r3
	brne LCD_delay_inner
	dec	r2
	brne LCD_delay_outer
ret

LCD_init:					; Initialize LCD
	push	temp			; prepare temporary variable
							;control lines are output, rest is input
	ldi	temp, (1<<LCD_RS)|(1<<LCD_RW)|(1<<LCD_E)	
	out	LCD_DDR, temp
	
	rcall	LCD_delay		; first, we'll tell the LCD that we want to use it
	ldi	arg, (1<<LCD_E)		; in 4-bit mode.
	rcall LCD_command8		; LCD is still in 8-BIT MODE while writing this command!!!
	rcall LCD_wait

	ldi	arg, 0x28			; NOW: 2 lines, 5*7 font, 4-BIT MODE!
	rcall LCD_command
	rcall LCD_wait

	ldi arg, (1<<3)|(1<<2)|(CURSOR<<1)|(BLINK<<0)
	rcall LCD_command
	rcall LCD_wait

	ldi	arg, 0x06			; auto-inc cursor
	rcall LCD_command

	ldi	arg, 0x01			; clear display, cursor -> home
	rcall LCD_command
	rcall LCD_wait

	pop temp
ret

.MACRO LCD_print
	push ZL					; save ZL
	push ZH					; save ZH
	
	ldi ZL, LOW(@0*2)		; Z <= Address from byte array '@0'
	ldi ZH, HIGH(@0*2)

print:						; loop over elements in byte array
	lpm	arg, Z				; read byte from byte array
	tst arg					; arg == 0 (end mark)?
	breq endprnt			; end mark    => jump to endprnt:
	rcall LCD_putchar		; no end mark => send char to LCD
	rcall LCD_wait
	adiw ZL, 1				; Increment array address
	rjmp print
endprnt:

	pop ZH					; restore ZH
	pop ZL					; restore ZL
.ENDM
