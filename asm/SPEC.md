# MSC-16 Assembler Manual

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

## Language Syntax

All instructions may be referred to in any case.

There are only two addressing modes: immediate and register.
For the instructions that support immediate addressing mode, the immediate value
is prefixed with a `#` symbol and is assumed to be a decimal number, unless
it is prefixed with a `$`, specifying that it is a hexadecimal number.

For register addressing mode, the register name is prefixed with a % symbol.

## Examples

```
CMP %A %B     # subtract B from A, store result in FLAGS
JNZ #$10      # jump to address 0x10 if FLAGS is not zero
JNZ %A        # jump to address stored in A if FLAGS is not zero
ST %A %B      # store the value in B to the address stored in A
ST #$10 %A    # store the value in A to the address 0x10
LD %A %B      # load the value at the address stored in B to A
LD #$10 %A    # load the value at address 0x10 to A
```
