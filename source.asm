		mode new

PORT_CONSOLE	= $FFFF

		sp <- $70       ; Setup stack

		r0 <- str0      ; shortcut for addis r0 str0 0
		call str_print
		r0 <- str0
		call str_print
		dw 0 ; STOP

str_print	r1 <= [ r0 ]           ; testing move (addi r1 [ r0 ] 0)
		jz .exit           
		[ PORT_CONSOLE ] <- r1 ; output char to console
		r0 <- r0 + 1           ; increment r0
		pc <- str_print        ; jump to the beginning of procedure
.exit		ret                    ; shortcut for move pc [ sp ]

str0		dw "Hello, world!" 10 0
