PORT_CONSOLE    = $FFFF

	move r0 var1
	move r1 10
loop	movet [ r0 ] r1
	jz exit
	addi r0 r0 1
	addi r1 r1 -1
	move pc loop
exit	dw 0

	org $30
var1	dw 0