/* SPDX-License-Identifier: GPL-2.0-only */
#include <stdlib.h>

#include "opcodes.h"

/* because fuck big-endian
 * 
 * seriously though, this emulator will NOT work on a big-endian machine
 */
void mem_write(cpu_t *cpu, u16 address, u16 value)
{
	u16 *mem = (u16 *)cpu->memory;
	mem[address] = value;
}

u16 mem_read(cpu_t *cpu, u16 address)
{
	u16 *mem = (u16 *)cpu->memory;
	return mem[address];
}

u16 inst_cmp(cpu_t *cpu, u16 *r1, u16 *r2)
{
	return *r1 - *r2;
}

u16 inst_add(cpu_t *cpu, u16 *r1, u16 *r2)
{
	*r1 += *r2;
	return *r1;
}

u16 inst_sub(cpu_t *cpu, u16 *r1, u16 *r2)
{
	*r1 -= *r2;
	return *r1;
}

u16 inst_jnz(cpu_t *cpu, u16 *r1, u16 *r2)
{
	if (!TEST_FLAG(cpu, FLAG_Z))
		cpu->ip = *r1;

	return 0;
}

u16 inst_push(cpu_t *cpu, u16 *r1, u16 *r2)
{
	cpu->sp -= 2;
	mem_write(cpu, cpu->sp, *r1);

	return *r1;
}

u16 inst_pop(cpu_t *cpu, u16 *r1, u16 *r2)
{
	*r1 = mem_read(cpu, cpu->sp);
	cpu->sp += 2;

	return *r1;
}

u16 inst_st_ld(cpu_t *cpu, u16 *r1, u16 *r2)
{
	*r1 = *r2;
	return *r1;
}

void cpu_advance(cpu_t *cpu)
{
	static void *inst_select[8] = { inst_cmp, inst_add, inst_sub, inst_jnz, inst_push, inst_pop, inst_st_ld, inst_st_ld };

	u8 opcode = cpu->memory[cpu->ip];
	u8 instruction = opcode & 0x7;
	u8 admode = 0; /* 1 if immediate, 0 if register */

	u16 *src = NULL;
	u16 *dst = NULL;

	switch (instruction) {
	case INST_CMP:
	case INST_ADD:
	case INST_SUB:
		src = &CPU_REG(cpu, (opcode >> 3) & 0x3);
		dst = &CPU_REG(cpu, (opcode >> 5) & 0x3);
		break;
	case INST_JNZ:
		admode = opcode >> 5;
		if (admode) {
			dst = (u16 *)&cpu->memory[*dst];
			CPU_ADVANCE(cpu, 2);
		} else {
			dst = &CPU_REG(cpu, (opcode >> 3) & 0x3);
		}

		break;
	case INST_PUSH:
	case INST_POP:
		dst = &CPU_REG(cpu, (opcode >> 3) & 0x3);
		break;
	case INST_ST:
	case INST_LD:
		src = &CPU_REG(cpu, (opcode >> 3) & 0x3);
		admode = opcode >> 7;

		if (admode) {
			dst = (u16 *)&cpu->memory[*dst];
			CPU_ADVANCE(cpu, 2);
		} else {
			dst = &CPU_REG(cpu, (opcode >> 5) & 0x3);
		}

		break;
		break;
	}

	u16 (*inst)(cpu_t *, u16 *, u16 *) = inst_select[instruction];

	u16 tmp = 0;
	if (dst)
		tmp = *dst;

	u16 result = inst(cpu, src, dst);

	INVALIDATE_FLAGS(cpu);

	if (result == 0)
		SET_FLAG(cpu, FLAG_Z);
	if (result & 0x8000)
		SET_FLAG(cpu, FLAG_N);

	if (instruction == INST_CMP || instruction == INST_SUB) {
		if (tmp < result)
			SET_FLAG(cpu, FLAG_V);
	} else if (instruction == INST_ADD) {
		if (tmp > result)
			SET_FLAG(cpu, FLAG_V);
	}

	CPU_ADVANCE(cpu, 1);
}

