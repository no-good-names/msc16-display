# MSC-16: Michael's Super-Awesome Computer

A simple 16-bit architecture, with an emulator

# MSC-16 Chip Spec

## Registers

### General Purpose:
A, B, C, D

### Special Purpose:
IP, SP, FLAGS

## Instructions

```
CMP (R1) (R2) -> FLAGS | (R1) - (R2) -> FLAGS
ADD (R1) (R2) -> (R1)  | (R1) + (R2) -> (R1)
SUB (R1) (R2) -> (R1)  | (R1) - (R2) -> (R1)
JNZ (IMM/REG)
PUSH (REG)
POP (REG)
ST (IMM/REGPTR) (REG) -> (MEM)
LD (REG) (IMM/REGPTR) -> (REG)
```

## Opcodes

All instructions are of single byte length. The lower three bits of the specify
the instruction. The addressing mode is further decoded from the opcode.

### CMP: 0x00

Bit pattern: 0AAB B000

A: Register 1
B: Register 2

### ADD: 0x01

Bit pattern: 0AAB B001

A: Destination register
B: Source register

### SUB: 0x02

Bit pattern: 0AAB B010

A: Destination register
B: Source register

### JNZ: 0x03

Bit pattern: 00AB B011

A: Set if immediate addressing mode
B: Register (if using register addressing mode)

### PUSH: 0x04

Bit pattern: 000A A100

A: Register

### POP: 0x05

Bit pattern: 000A A101

A: Register

### ST: 0x06

Bit pattern: ABBC C110

A: Set if immediate addressing mode
B: Source register
C: Destination pointer-containing register (if using register addressing mode)

### LD: 0x07

Bit pattern: ABBC C111

A: Set if immediate addressing mode
B: Destination register
C: Source pointer-containing register (if using register addressing mode)

