# Simpleton 4

Some of 8bit-guy videos including covering of project 'Gigatron TTL' inspired me to make some ISAs.
This is fourth generation of the same idea (see below).

First of all: this ISA is definitely suboptimal.

**The only goal is to make instruction format as simple as possible keeping programming flexible and non-esoteric.**

I know that code density could be improved, leading to something like MSP 430 as a result. So, it's not an option. :)

Second: memory and registers are 16-bit wide for simplicity.
There are eight registers R0-R7 and 128Kb of 65636 16-bit memory cells.

So, instruction opcode is 16-bit wide too. There is the only one instruction format:
![Instruction format picture](https://gamedev.ru/files/images/simpleton4.png)

Every instruction do the only one thing: takes two operands X and Y, writes them into ALU with operation code (INSTR) and writes result to the destination R. Even calls or conditional jumps do nothing but this.
Fields X, Y and R are registers codes 0-7. Plus bits of indirection (XI, YI, RI) which (if set) tell to work with memory cell registers points to. 4:4:4:4 bit format makes it easier to read machine codes as a result too.
So, every instruction do C expression: R = Y op X, where op is operation code. Square brackets around operand mean indirect mode (indirection bit = 1).

Registers **R5-R7** have synonyms and special features:
**R5 = SP** - stack pointer - post-increments after indirect reading and pre-decrements before indirect writing.
**R6 = PC** - program counter - always post-increments after indirect reading
**R7 = PSW** = processor status word (flags, including enable/disable interrupts). Indirect reading/writing is meaningless, so it works as 'immediate indirect' address mode (data from [ PC++ ] serves as address of memory cell we work with).
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