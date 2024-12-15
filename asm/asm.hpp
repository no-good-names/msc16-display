/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include "common.hpp"

#define INST_CMP 0x0
#define INST_ADD 0x1
#define INST_SUB 0x2
#define INST_JNZ 0x3
#define INST_PUSH 0x4
#define INST_POP 0x5
#define INST_ST 0x6
#define INST_LD 0x7
#define INST_OR 0x8
#define INST_AND 0x9
#define INST_XOR 0xA
#define INST_LSH 0xB
#define INST_RSH 0xC
#define INST_CLI 0xD
#define INST_STI 0xE
#define INST_INT 0xF
#define MACR_STR 0x10
#define MACR_ZST 0x11
#define MACR_ORG 0x12
#define MACR_DEF 0x13
#define MACR_END 0x14

struct token {
	enum type {
		OPC,
		IMM_DEC,
		IMM_HEX,
		REG,
		LABEL,
		LABEL_REF,
		STRING,
	} type;

	string str;
};

struct instruction {
	vector<token> tokens;

	uint16_t opcode;

	size_t line_no;
};

struct line {
	string line;
	size_t line_no;
};

enum token::type get_token_type(const string &token_s);
instruction tokenize_line(line &line, size_t line_no);
string assemble(const string &src);
int preprocess(vector<line> &lines);
