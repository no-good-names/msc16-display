# SPDX-License-Identifier: 0BSD

	CMP %A, %A
test_label:
	CMP %A, %B
	JNZ test_label2
	LD %A, 0
	JNZ test_label3
	ST %A, %B
test_label2:
	LD %B, 0
	JNZ test_label
	ST %A, %B
test_label3:
	XOR %A, %A
	JNZ test_label
