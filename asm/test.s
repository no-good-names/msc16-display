# SPDX-License-Identifier: 0BSD

.macro TESTMACRO
	.zstring "This is a test macro"
.endm

.org $100
init:
	ld %a, 1
	jnz init_end
init_mid:
	cmp %a, %a
	cmp %a, %a
	ld %b, 1
	jnz init_mid
init_end:
	cmp %c, %d
	xor %d, %a
	ld %a, 30
	or %b, %a
	ld %a, 1
	jnz init_mid

TESTMACRO
