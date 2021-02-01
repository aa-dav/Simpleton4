PORT_CONSOLE    = $FFFF

	addis r0 100 0
	addis r1 r0 1
	add r3 r0 r1
	addis [ $20 ] r0 -1