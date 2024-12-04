# MSC-16: Michael's Super-Awesome Computer

A simple 16-bit architecture, with an emulator

## Motivation

N/A

## Building

You can build the packages individually by invoking `make` in their directories.

The following are required to build all packages:

```
make
gcc
gperf
```

# MSC-16 Chip Spec

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

## Flags

```
0x1: Zero
0x2: Negative
0x4: Overflow
0x8: Interrupt Enable
```

## Memory

16-bit addressable memory

## Opcodes

All instructions are of 16-bit length, stored in memory in little-endian format

After reading from memory, we decode the instruction into the following format:

```
AAAA RRRR BBCC MRRR

A: Instruction
B: R1
C: R2
M: Mode (1 for immediate)
R: Reserved
```

In the case of an immediate value, the immediate value is stored in the next
two bytes of memory, again in little-endian format. Due to the way the opcode
is decoded, this format allows for duplicate opcodes.

