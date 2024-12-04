# MSC-16 Assembler Manual

## Registers

### General Purpose:
A, B, C, D

### Special Purpose:
IP, SP, FLAGS

## Instructions

```
INSTRUCTION                     |  DESCRIPTION           |  OPCODE |
--------------------------------|------------------------|---------|
CMP (R1) (R2) -> FLAGS          |  (R1) - (R2) -> FLAGS  |  0x0    |
ADD (R1) (R2) -> (R1)           |  (R1) + (R2) -> (R1)   |  0x1    |
SUB (R1) (R2) -> (R1)           |  (R1) - (R2) -> (R1)   |  0x2    |
JNZ (IMM/REG)                   |  (IMM/REG)   -> (IP)   |  0x3    |
PUSH (REG)                      |  (REG)  -> (--SP)      |  0x4    |
POP (REG)                       |  (SP++) -> (REG)       |  0x5    |
ST (IMM/REGPTR) (REG)           |  (REG)  -> (MEM)       |  0x6    |
LD (REG) (IMM/REGPTR)           |  (MEM)  -> (REG)       |  0x7    |
OR (R1) (R2) -> (R1)            |  (R1) | (R2) -> (R1)   |  0x8    |
AND (R1) (R2) -> (R1)           |  (R1) & (R2) -> (R1)   |  0x9    |
XOR (R1) (R2) -> (R1)           |  (R1) ^ (R2) -> (R1)   |  0xA    |
LSH (R1) (R2) -> (R1)           |  (R1) << (R2) -> (R1)  |  0xB    |
RSH (R1) (R2) -> (R1)           |  (R1) >> (R2) -> (R1)  |  0xC    |
CLI                             |  FLAGS &= (~I)         |  0xD    |
STI                             |  FLAGS |= (I)          |  0xE    |
INT (IMM)                       |                        |  0xF    |
```

## Language Syntax

All instructions may be referred to in any case.

There are only two addressing modes: immediate and register.
For the instructions that support immediate addressing mode is assumed to be a
decimal number, unless it is prefixed with a `$`, specifying that it is a 
hexadecimal number.

For register addressing mode, the register name is prefixed with a % symbol.

## Examples

```
CMP %A %B     # subtract B from A, store result in FLAGS
JNZ $10      # jump to address 0x10 if FLAGS is not zero
JNZ %A        # jump to address stored in A if FLAGS is not zero
ST %A %B      # store the value in B to the address stored in A
ST $10 %A    # store the value in A to the address 0x10
LD %A %B      # load the value at the address stored in B to A
LD $10 %A    # load the value at address 0x10 to A
```
