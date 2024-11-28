/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _OPCODES_H_
#define _OPCODES_H_

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long long u64;

#define INST_CMP 0x0
#define INST_ADD 0x1
#define INST_SUB 0x2
#define INST_JNZ 0x3
#define INST_PUSH 0x4
#define INST_POP 0x5
#define INST_ST 0x6
#define INST_LD 0x7

#define FLAG_Z 0x1
#define FLAG_N 0x2
#define FLAG_V 0x4

#define INVALIDATE_FLAGS(cpu) (cpu->flags = 0)
#define SET_FLAG(cpu, flag) (cpu->flags |= flag)
#define CLEAR_FLAG(cpu, flag) (cpu->flags &= ~flag)
#define TEST_FLAG(cpu, flag) (cpu->flags & flag)

#define CPU_REG(cpu, reg) (cpu->r[reg])
#define CPU_ADVANCE(cpu, n) (cpu->ip += n)

typedef struct cpu {
	union {
		struct {
			u16 a;
			u16 b;
			u16 c;
			u16 d;
		} __attribute__((packed));
		u16 r[4];
		u64 r64;
	};

	u16 sp;
	u16 ip;
	u16 flags;

	u8 memory[0x10000];
} cpu_t;

typedef struct opcode {
	u8 instruction;
	u16 *source;
	u16 *destination;
} opcode_t;

#endif /* _OPCODES_H_ */
