/* SPDX-License-Identifier: GPL-2.0-only */
#include <stdio.h>
#include <unistd.h>
#include "bus.h"

int main()
{
	cpu_t cpu;
	cpu_init(&cpu);

	FILE *fp = fopen("../asm/out.bin", "rb");
	fread(&cpu.memory, 1, 0x10000, fp);
	fclose(fp);

	while (1) {
		cpu_advance(&cpu);
		usleep(10000);
	}
}