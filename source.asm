		mode new

PORT_CONSOLE	= $FFFF

		sp <- $70       ; Setup stack
		pc <- start

str_print	r1 <= [ r0 ]           ; testing move (addi r1 [ r0 ] 0)
		jz .exit           
		[ PORT_CONSOLE ] <- r1 ; output char to console
		r0 <- r0 + 1           ; increment r0
		pc <- str_print        ; jump to the beginning of procedure
.exit		ret	           ; shortcut for move pc [ sp ]

str0		dw "Hello, world!" 13 10 0

start		r0 <- str0
		call str_print
		[ PORT_CONSOLE ] <- $3E	; '>'

.inp_loop	r0 <= [ PORT_CONSOLE ]
		jz .inp_loop
		[ PORT_CONSOLE ] <- r0
		pc <- .inp_loop

		dw 0			; STOP