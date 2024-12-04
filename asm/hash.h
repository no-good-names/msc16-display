/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <stdlib.h>
#ifndef _ASM_H_
#define _ASM_H_

#ifdef __cplusplus
extern "C" {
#endif

#define KW_R2_IMM 0x1
#define KW_R2_REG 0x2
#define KW_R1_IMM 0x4
#define KW_R1_REG 0x8

#ifndef GPERF_IMPL
struct keyword {
	const char *name;
	int opc;
	int n_args;
	int arg_fmt;
};
#endif /* GPERF_IMPL */

struct keyword *asm_keyword_lookup(const char *str, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* _ASM_H_ */
