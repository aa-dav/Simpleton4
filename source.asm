PORT_CONSOLE    = $FFFF

	move sp $60
	move r1 100
	add r2 r1 -1
	dw 0

	org $30