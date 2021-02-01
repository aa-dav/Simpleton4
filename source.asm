PORT_CONSOLE    = $FFFF

	addis [ var2 ] $FFF0
	addis [ var3 ] $0000
	xor [ var1 ] [ var2 ] [ var3 ]

	org $30

var1	dw 0
var2	dw 0
var3	dw 0
