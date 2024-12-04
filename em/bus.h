/* SPDX-License-Identifier: GPL-2.0-only */
/* Internal bus for the CPU
 * 
 * This bus is used to read and write data from/to memory and registers.
 * Not to be confused with the system bus.
 */
#ifndef _BUS_H_
#define _BUS_H_

#include "opcodes.h"

#define BUS_REG 0
#define BUS_MEM 1

typedef struct _busptr {
	u16 reg_mem_addr; /* register or memory address */
	u8 type;
} busptr_t;

u16 cpu_bus_read(cpu_t *cpu, busptr_t *ptr);
void cpu_bus_write(cpu_t *cpu, busptr_t *ptr, u16 value);

#endif /* _BUS_H_ */

