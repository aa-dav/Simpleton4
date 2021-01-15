PORT_CONSOLE    = $FFFF
    	sp  = $FF00

    	pc  = start
    
mark	dw	0

; string_input
; in: r0 - string buffer
;     r1 - max buffer size
; out: 
string_input  r3  = r0    ; remember beginning
.loop    r2  =? [ PORT_CONSOLE ]
    pc  = .loop @z
    r2  <?> 13
    pc  = .end @z  ; if CR
    r2  <?> 8
    pc  = .backsp @z  ; if BS
    r1  =? r1
    pc  = .overfl @z  ; if buffer overflow
    ; accept symbol
    [ PORT_CONSOLE ] = r2
    r1  =-1 r1
    [ r0 ]  = r2
    r0  =+1 r0
    pc  = .loop    ; continue input
    ; backspace
.backsp    r0  <?> r3
    pc  = .loop @z  ; ignore del at start of line
    [ PORT_CONSOLE ] = r2
    [ PORT_CONSOLE ] = 32  ; erase prev symbol at (windows) console...
    [ PORT_CONSOLE ] = r2
    r1  =+1 r1
    r0  =-1 r0
    pc  = .loop
    ; overflow
.overfl    pc  = .loop ; just continue
    ; end
.end    [ r0 ]  = 0
    ret

; string_print
; in: r0 - string buffer
string_print  r1  =? [ r0 ]
    ret @z
    r0  =+1 r0
    [ PORT_CONSOLE ] = r1
    pc  = string_print

; string_len  
; in:  r0 - string buffer
; out:  r0 - length of the string
string_len  r1  = 0
.loop    r2  = [ r0 ]
    pc  = .end @z
    r0  =+1 r0
    r1  =+1 r1
    pc  = .loop
.end    r0  = r1
    ret

start    
	r0	= $0001
	r1	= $1000
	r0	&= r1
	[ mark ] = r0
	
    r0  = msg1
    call  string_print
    r0  = buf
    r1  = 10
    call  string_input
    [ PORT_CONSOLE ] = 10

    r0  = msg2
    call   string_print
    r0  = buf
    call  string_print
    r0  = CrLf
    call  string_print

    dw  0
buf    ds  12 $AAAA
msg1    dw  "Enter command: " 0
msg2    dw  "You entered this text: " 0
CrLf    dw  13 10 0