/* SPDX-License-Identifier: GPL-2.0-only */
#include "opcodes.h"
#include "bus.h"

u16 cpu_bus_read(cpu_t *cpu, busptr_t *ptr)
{
	if (ptr->type == BUS_MEM) {
		u8 *mem = (u8 *)&cpu->memory[ptr->reg_mem_addr];
		return mem[0] | ((u16)mem[1] << 8);
	} else {
		return CPU_REG_READ(cpu, ptr->reg_mem_addr);
	}
}

void cpu_bus_write(cpu_t *cpu, busptr_t *ptr, u16 value)
{
	if (ptr->type == BUS_MEM) {
		u8 *mem = (u8 *)&cpu->memory[ptr->reg_mem_addr];
		mem[0] = value & 0xFF;
		mem[1] = (value >> 8) & 0xFF;
	} else {
		CPU_REG_WRITE(cpu, ptr->reg_mem_addr, value);
	}
}

