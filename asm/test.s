# SPDX-License-Identifier: 0BSD

test_label:
	CMP %A, %B
	JNZ test_label
	ST %A, %B
