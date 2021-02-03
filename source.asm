PORT_CONSOLE    = $FFFF


		move sp $70	; Setup stack

		move r0 str0
		call str_print
		move r0 str0
		call str_print
		dw 0 ; STOP

str_print	movet r1 [ r0 ]
		jz .exit
		move [ PORT_CONSOLE ] r1
		addi r0 r0 1
		move pc str_print
.exit		ret


str0		dw "Hello, world!" 10 0