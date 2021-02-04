# Simpleton 4

Some of 8bit-guy videos including covering of project 'Gigatron TTL' inspired me to make some ISAs.
This is fourth generation of the same idea (see below).

First of all: this ISA is definitely suboptimal.
*The only goal is to make instruction format as simple as possible keeping programming flexible and non-esoteric.*
I know that code density could be improved, leading to something like MSP 430 as a result. So, it's not an option. :)

Second: memory and registers are 16-bit wide for simplicity.
There are eight registers R0-R7 and 128Kb of 65636 16-bit memory cells.

So, instruction opcode is 16-bit wide too. There is the only one instruction format:
![Instruction format picture](https://gamedev.ru/files/images/simpleton4.png)