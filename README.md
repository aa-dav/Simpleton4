# Simpleton 4

Some of 8bit-guy videos including covering of project 'Gigatron TTL' inspired me to make some ISAs.
This is fourth generation of the same idea (see below).

First of all: this ISA is definitely suboptimal.

**The only goal is to make instruction format as simple as possible keeping programming flexible and non-esoteric.**

I know that code density could be improved, leading to something like MSP 430 as a result. So, it's not an option. :)

Second: memory and registers are 16-bit wide for simplicity.
There are eight registers R0-R7 and 128Kb of 65636 16-bit memory cells.

So, instruction opcode is 16-bit wide too. There is the only one instruction format:
![Instruction format picture](https://cdn.jpg.wtf/futurico/95/66/1613283583-9566e916e4b56fca243a37105c20898d.png)

Every instruction do the only one thing: takes two operands X and Y, writes them into ALU with operation code (INSTR) and writes result to the destination R. Even calls or conditional jumps do nothing but this.
Fields X, Y and R are registers codes 0-7. Plus bits of indirection (XI, YI, RI) which (if set) tell to work with memory cell registers points to. 4:4:4:4 bit format makes it easier to read machine codes as a result too.
So, every instruction do C expression: R = Y op X, where op is operation code. Square brackets around operand mean indirect mode (indirection bit = 1).

Registers **R5-R7** have synonyms and special features:
- **R5 = SP** - stack pointer - post-increments after indirect reading and pre-decrements before indirect writing.
- **R6 = PC** - program counter - always post-increments after indirect reading
- **R7 = PSW** = processor status word (flags, including enable/disable interrupts). Indirect reading/writing is meaningless, so it works as 'immediate indirect' address mode (data from [ PC++ ] serves as address of memory cell we work with).

Immediate data is available as indirect PC reading because after reading instruction code PC post-increments (as always) and points to next word and will be advanced to the next word after indirect reading.
During machine cycles immediates for operands and result are read from [ PC++ ] (if needed) in next order: X, Y, R. This is why it's simplier to express instruction as R = Y + X.
Also some instructions treat field XI+X of opcode as 4-bit 'inplace immediate -8..+7'. These instructions have letter 'i' (inplace immediate) in their symbolic names.

So, we can have next instructions in asm syntax: 
```
add r0 r1 r2		; regular add of registers: r0 = r1 + r2
adc r1 r1 r3		; add with carry
add [ r2 ] 100 R4		; add R4 with immediate (PC+indirect) 100 and write it to memory R2 points to
add [ R3 ] R4 [ $200 ]		; immediate indirect [ $200 ] (hex number) (PSW+indirect)
add [ 100 ] [ 20 ] [ 30 ]	; register-free 4-word-long instruction
```
There is no commas in this syntax, so complex compile-time expressions must be placed in round brackets (however this is not implemented yet):
```
add r0 r0 (label + 4 * offset)	; compile-time expression
```

There are 16 ALU operations possible. Next opcodes are in use right now:
- 00 **ADDI** - add Y with INPLACE immediate in XI+X
- 01 **ADDIS** - add Y with INPLACE immediate in XI+X silent (doesn't update flags!)
- 02 **ADDS** - add silent (doesn't update flags)
- 03 **ADD** - add
- 04 **ADC** - add with carry
- 05 **SUB** - sub
- 06 **SBC** - sub with carry
- 07 **AND** - and
- 08 **OR** - or
- 09 **XOR** - xor
- 0A **CMP** - compare (as if Y - X) Always returns Y from ALU, so it works as usual CMP in form: cmp a a b, but can have more tricky behaviour if R <> Y.
- 0B **CADD** - conditional add. never updates flags. (see below!)
- 0C **RRCI** - rotate Y right (cyclic) by INPLACE immediate bit, carry gets last rotated bit
- 0D **RRC** - rotate Y right (cyclic) by X bit, carry gets last rotated bit

## NOTES:

1. There is no separate MOVE opcode because it's ADDIS with 0 in XI+X:
(inplace) immediate in X could be omitted in assembler syntax:
```
addis r0 r1 1
addis r0 r1 0 ;
move [ r2 ] r1 		; There is shortcut syntax 'move' (addis [ r2 ] r1 0)
move [ r3 ] [ 100 ]
```
2. writing immediate in PC is JUMP and adding (silent) PC with immediate is relative JUMP:
```
move pc address		; jump
adds pc pc offset	; relative jump
```
3. conditional add is the key to the conditional branching, it works in this way: X argument (16 bit) is decoupled in two parts: upper 3 bits are conditional code and lower 13 bits are sign-extended to 16 bit of -4096..+4095 addendum. That is condition code is not part of opcode, but part of data! If condition is false - ALU skips addition and returns Y without changes.
So, to implement conditional branching we just do:
```
cadd pc pc condition_with_offset
```
Assembler provides automatic offset calculation for labels of course.
For simplicity alternate syntax is supported (jz, jnz, jc, jnc and so on):
```
JNZ LABEL
```
4. CALL may be implemented as:
```
addis sp pc 2 		; precalculate return address
move pc proc_address
```
There is shortcut for this in asssembler in usual form:
```
call proc_address
```
So, it's two instructions and 3 words. This is the most visible penalty of unified instruction format.

5. But RET is just:
```
move pc [ sp ] ; one-word addis instruction
```
6. ADDI could be used as move with updating flags (testing move, useful in loops like STRCPY).
```
StrCpy: 		; R1 - pointer to src, R2 - pointer to dst
  movet [ r2 ] [ r1 ] 	; movet is shortcut for addi x y 0 ; 'move with Test'
  jz Exit
  addis r1 r1 1		; increment r1
  addis r2 r2 1		; increment r2
  move pc StrCpy
Exit:
  ret 			; shortcut for move pc [ sp ]
```
7. Disabling/enabling interrupts is just simple as:
```
and psw psw flag_mask ; enable
or psw psw ~flag_mask ; disable (inverse of flag bit)
```

Given $FFFF is console memory-mapped input/output port next program is assembled and emulated already:
```
PORT_CONSOLE    = $FFFF

      move sp $70	; Setup stack

      move r0 str0	; shortcut for addis r0 str0 0
      call str_print	; shortcut for addis [ sp ] pc 2 : addis pc str_print 0
      move r0 str0
      call str_print
      dw 0 ; STOP

str_print   movet r1 [ r0 ]
      jz .exit      ; shortcut for cadd pc pc mix_of_offset_and_condition_code
      move [ PORT_CONSOLE ] r1   ; output char to console
      addi r0 r0 1   ; increment r0
      move pc str_print   ; jump to the beginning of procedure
.exit      ret         ; shortcut for move pc [ sp ]


str0      dw "Hello, world!" 10 0
```

### New assembler syntax

Assembler instructions 'mode classic' and 'mode new' can switch assembler back and forth new 'math notation'.
Most of instructions could be expressed as 'R = Y op X' where 'op' is operation sign.
If X is omitted it becomes literal '0' (zero).
If 'op' is omitted it becomes '+' sign, so pseudoinstruction 'move' is not needed anymore (see below).
Next instructions fulfill this pattern (example for R=R0, Y=R1 and X=[ label ]):
```
02 - ADDS  - r0 = r1 +s [ label ] ; add silent (doesn't update flags)
03 - ADD  - r0 = r1 +  [ label ] ; add
04 - ADC  - r0 = r1 +c [ lavel ] ; add with carry
05 - SUB  - r0 = r1 -  [ label ] ; sub
06 - SBC  - r0 = r1 -c [ label ] ; sub with carry
07 - AND  - r0 = r1 &  [ label ] ; and
08 - OR   - r0 = r1 |  [ label ] ; or
09 - XOR  - r0 = r1 ^  [ label ] ; xor
0A - CMP  - r0 = r1 ?  [ label ] ; compare (as Y - X), op updates flags and returns Y
0B - CADD - r0 = r1 +? [ label ] ; conditional add. never updates flags.
0D - RRC  - r0 = r1 >> [ label ] ; rotate Y right (cyclic) by X bits
```
But there are 3 opcodes (right now) which fall out of this pattern and have special syntax:
```
00 - ADDIS - r0 <- r1 - 1         ; add Y with INPLACE immediate in XI+X SILENT (flags are not updated)
01 - ADDI  - r0 <= r1 + 3         ; add Y with INPLACE immediate in XI+X
0C - RRCI  - r0 <= r1 >> 15       ; rotate Y right (cyclic) by INPLACE immediate bits
```
First of all - it's 'inplace immediate' commands: addi, addis and rcci. These of them who updates flags use '<=' as sign of this special case. 
The only exceptions is 'addis' which uses '<-' to signal that it's not updates flags.

Note, that 'addis a b -1' (negative inplace immediate) could be expressed in new syntax as 'a <- b + -1'. But also 'a <- b - 1' is correct (that is like 'sub' opcode). This is another reason for exclusion of addi(s) from regular '=' syntax.
Also, 'move' is as simple as: 'a <- b' which is shortcut for 'a <- b + 0'.

Note, that all pseudoops used for simplicity are still in place: 'jnz/call/ret' and so on.

So, example above could be rewritten as:
```
            mode new

PORT_CONSOLE    = $FFFF

            sp <- $70       ; Setup stack

            r0 <- str0      ; shortcut for addis r0 str0 0
            call str_print
            r0 <- str0
            call str_print
            dw 0 ; STOP

str_print   r1 <= [ r0 ]           ; testing move (addi r1 [ r0 ] 0)
            jz .exit           
            [ PORT_CONSOLE ] <- r1 ; output char to console
            r0 <- r0 + 1           ; increment r0
            pc <- str_print        ; jump to the beginning of procedure
.exit       ret                    ; shortcut for move pc [ sp ]

str0        dw "Hello, world!" 10 0
```
