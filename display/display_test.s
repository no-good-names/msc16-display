.org $100

_start:
	ld %a, 1
	jnz init

init:
	ld %a, %a
	jnz init

.org $2149