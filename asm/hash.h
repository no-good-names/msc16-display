/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _HASH_H_
#define _HASH_H_

#include <stdlib.h>

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
};
#endif /* GPERF_IMPL */

struct keyword *asm_keyword_lookup(const char *str, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* _HASH_H_ */
