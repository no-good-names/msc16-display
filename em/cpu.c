/* SPDX-License-Identifier: GPL-2.0-only */
#include <stdio.h>
#include "opcodes.h"
#include "bus.h"

#define likely(x) (__builtin_expect(!!(x), 1))

#define OPC_R1(opc) ((opc >> 6) & 0x3)
#define OPC_R2(opc) ((opc >> 4) & 0x3)

#define GEN_ARITH_INST(name, op, write)                                                             \
	u16 inst_##name(cpu_t *cpu, busptr_t *r1, busptr_t *r2)                                     \
	{                                                                                           \
		printf(#name " %c %c\n", 'A' + cpu_bus_read(cpu, r1), 'A' + cpu_bus_read(cpu, r2)); \
		u16 t1 = cpu_bus_read(cpu, r1);                                                     \
		u16 t2 = cpu_bus_read(cpu, r2);                                                     \
		u16 result = t1 op t2;                                                              \
		if (likely(write))                                                                  \
			cpu_bus_write(cpu, r1, result);                                             \
		return result;                                                                      \
	}

GEN_ARITH_INST(add, +, 1);
GEN_ARITH_INST(sub, -, 1);
GEN_ARITH_INST(cmp, -, 0);
GEN_ARITH_INST(and, &, 1);
GEN_ARITH_INST(or, |, 1);
GEN_ARITH_INST(xor, ^, 1);
GEN_ARITH_INST(lsh, <<, 1);
GEN_ARITH_INST(rsh, >>, 1);

u16 inst_jnz(cpu_t *cpu, busptr_t *r1, busptr_t *r2)
{
	printf("jnz %d\n", cpu_bus_read(cpu, r1));
	if (!TEST_FLAG(cpu, FLAG_Z)) {
		cpu->ip = cpu_bus_read(cpu, r1);
	}

	return 0;
}

u16 inst_push(cpu_t *cpu, busptr_t *r1)
{
	printf("push %c\n", 'a' + cpu_bus_read(cpu, r1));
	cpu->sp -= 2;
	u16 rval = cpu_bus_read(cpu, r1);

	busptr_t sp = { .reg_mem_addr = cpu->sp, .type = BUS_MEM };
	cpu_bus_write(cpu, &sp, rval);

	return rval;
}

u16 inst_pop(cpu_t *cpu, busptr_t *r1)
{
	printf("pop %c\n", 'a' + cpu_bus_read(cpu, r1));
	busptr_t sp = { .reg_mem_addr = cpu->sp, .type = BUS_MEM };
	u16 val = cpu_bus_read(cpu, &sp);
	cpu_bus_write(cpu, r1, val);

	cpu->sp += 2;

	return val;
}

u16 inst_st_ld(cpu_t *cpu, busptr_t *r1, busptr_t *r2)
{
	printf("st/ld %4x %4x\n", cpu_bus_read(cpu, r1), cpu_bus_read(cpu, r2));
	u16 t1 = cpu_bus_read(cpu, r2);
	cpu_bus_write(cpu, r1, t1);
	return t1;
}

u16 inst_cli(cpu_t *cpu)
{
	printf("cli\n");
	CLEAR_FLAG(cpu, FLAG_I);
	return 0;
}

u16 inst_sti(cpu_t *cpu)
{
	printf("sti\n");
	SET_FLAG(cpu, FLAG_I);
	return 0;
}

u16 inst_int(cpu_t *cpu, busptr_t *r1, busptr_t *r2)
{
	printf("int %d\n", cpu_bus_read(cpu, r1));
	if (TEST_FLAG(cpu, FLAG_I))
		cpu->ip = cpu_bus_read(cpu, r1);

	return 0;
}

void cpu_advance(cpu_t *cpu)
{
	static const int n_inst = 16;
	static void *inst_select[] = { inst_cmp, inst_add, inst_sub, inst_jnz, inst_push, inst_pop, inst_st_ld, inst_st_ld,
				       inst_or,	 inst_and, inst_xor, inst_lsh, inst_rsh,  inst_cli, inst_sti,	inst_int };

	u16 ip = cpu->ip;
	busptr_t r1 = { .reg_mem_addr = ip, .type = BUS_MEM };
	busptr_t r2;

	u16 opcode = cpu_bus_read(cpu, &r1);
	u16 inst = opcode >> 12;
	u16 admode = (opcode & 0x8) >> 3;

	if (inst >= n_inst) {
		IP_ADVANCE(cpu, 1);
		return;
	}

	r1 = (busptr_t){ 0 };
	r2 = (busptr_t){ 0 };

	switch (inst) {
	case INST_CMP:
	case INST_ADD:
	case INST_SUB:
	case INST_OR:
	case INST_AND:
	case INST_XOR:
	case INST_LSH:
	case INST_RSH:
	case INST_PUSH:
	case INST_POP:
		r1.reg_mem_addr = OPC_R1(opcode);
		r2.reg_mem_addr = OPC_R2(opcode);
		r1.type = BUS_REG;
		r2.type = BUS_REG;

		break;
	case INST_JNZ:
		if (admode) {
			r1.reg_mem_addr = ip + 2;
			r1.type = BUS_MEM;
		} else {
			r1.reg_mem_addr = OPC_R1(opcode);
			r1.type = BUS_REG;
		}

		break;
	case INST_ST:
		if (admode) {
			r1.reg_mem_addr = ip + 2;
			IP_ADVANCE(cpu, 2);
			r1.type = BUS_MEM;
		} else {
			r1.reg_mem_addr = OPC_R1(opcode);
			r1.type = BUS_REG;
		}

		r2.reg_mem_addr = OPC_R2(opcode);

		break;
	case INST_LD:
		if (admode) {
			r2.reg_mem_addr = ip + 2;
			r2.type = BUS_MEM;
		} else {
			r2.reg_mem_addr = OPC_R2(opcode);
			r2.type = BUS_REG;
		}

		r1.reg_mem_addr = OPC_R1(opcode);

		break;
	case INST_INT:
		r1.reg_mem_addr = ip + 2;
		r1.type = BUS_MEM;
		IP_ADVANCE(cpu, 2);

		break;
	default:
		break;
	}

	u16 (*inst_func)(cpu_t *, busptr_t *, busptr_t *) = inst_select[inst];
	printf("ip: %4x opcode: %4x: ", ip, opcode);
	u16 ip_cur = cpu->ip;
	u16 result = inst_func(cpu, &r1, &r2);

	INVALIDATE_FLAGS(cpu);

	if (result == 0)
		SET_FLAG(cpu, FLAG_Z);
	if (result & 0x8000)
		SET_FLAG(cpu, FLAG_N);

	if (cpu->ip == ip_cur) {
		if (admode) {
			IP_ADVANCE(cpu, 2);
		}

		IP_ADVANCE(cpu, 2);
	}
}

void cpu_init(cpu_t *cpu)
{
	cpu->ip = 0;
	cpu->sp = 0x1000;
	cpu->flags = 0;
}
