# SPDX-License-Identifier: 0BSD

	CMP %A, %A
test_label:
	CMP %A, %B
	JNZ test_label
	ST %A, %B
