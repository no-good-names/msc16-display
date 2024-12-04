/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <any>
#include "common.hh"
#include "asm.hh"
#include "hash.h"

struct instruction;
struct token {
	enum type {
		OPC,
		IMM_DEC,
		IMM_HEX,
		REG,
		LABEL,
		LABEL_REF,
	} type;

	string str;
	union {
		size_t val;
		instruction *label;
	};
	size_t addr;
	size_t line_no;
};

struct operand {
	enum type {
		NONE,
		REG,
		LABEL_REF,
		IMM,
	} type;

	std::any val;
};

struct instruction {
	enum type {
		OPC,
		LABEL,
	} type;

	vector<token> tokens;

	uint64_t opcode;
	uint64_t admode;

	operand r1;
	operand r2;
};

static vector<token> unres_label_refs;
static vector<instruction> labels;
static vector<instruction> instrs;

static string token_types[] = {
	"OPC", "IMM_DEC", "IMM_HEX", "REG", "LABEL", "LABEL_REF",
};

static enum token::type get_token_type(const string &token_s)
{
	if (token_s[0] == '%')
		return token::REG;
	else if (isdigit(token_s[0]))
		return token::IMM_DEC;
	else if (token_s[0] == '$')
		return token::IMM_HEX;
	else if (token_s.back() == ':')
		return token::LABEL;
	else if (asm_keyword_lookup(token_s.c_str(), token_s.size()))
		return token::OPC;
	else
		return token::LABEL_REF;
}

static vector<string> tokenize_line_s(const string &line)
{
	static vector<char> separators = { ' ', '\t', ',' };
	vector<string> tokens_s;
	auto is_separator = [&](char c) {
		for (char sep : separators) {
			if (c == sep)
				return true;
		}

		return false;
	};

	for (size_t i = 0; i < line.size(); i++) {
		if (is_separator(line[i]))
			continue;
		if (line[i] == '#')
			break;

		size_t j = i;
		while (j < line.size() && !is_separator(line[j]))
			j++;

		size_t sub_len = j - i;

		tokens_s.push_back(line.substr(i, sub_len));

		i = j;
	}

	return tokens_s;
}

static instruction tokenize_line(const string &line, size_t line_no)
{
	instruction ret;

	vector<string> tokens_s = tokenize_line_s(line);
	for (const string &token_s : tokens_s) {
		token t;
		t.type = get_token_type(token_s);
		t.str = token_s;
		t.line_no = line_no;

		switch (t.type) {
		case token::IMM_DEC:
			t.val = stoul(token_s);
			break;
		case token::IMM_HEX:
			t.val = stoul(token_s.substr(1, token_s.length() - 1), nullptr, 16);
			break;
		case token::LABEL_REF:
			t.label = nullptr;
			break;
		default:
			break;
		}

		ret.tokens.push_back(t);
	}

	return ret;
}

static void decode(instruction &ins)
{
	token t0 = ins.tokens[0];

	switch (t0.type) {
	case token::LABEL:
		ins.type = instruction::LABEL;
		break;
	case token::OPC:
		ins.type = instruction::OPC;
		break;
	default:
		cerr << "Error on line " << t0.line_no << ": Invalid token type: " << token_types[t0.type] << endl;
	}

	size_t n_expected = 1;
	if (ins.type == instruction::LABEL) {
		if (ins.tokens.size() != 1) {
			cerr << "Error on line " << t0.line_no << ": Expected 1 token, got " << ins.tokens.size();
			cerr << " (label)" << endl;
		}
		return;
	}

	struct keyword *kw = asm_keyword_lookup(t0.str.c_str(), t0.str.size());
	ins.opcode = kw->opc;
	n_expected += kw->n_args;

	if (ins.tokens.size() != n_expected) {
		cerr << "Error on line " << t0.line_no << ": Expected " << n_expected << " tokens, got " << ins.tokens.size();
		cerr << " (opcode: " << t0.str << ")" << endl;
	}
}

string assemble(const string &src)
{
	string buf;

	vector<string> lines;
	stringstream ss(src);

	for (string line; getline(ss, line);)
		lines.push_back(line);

	ss.clear();

	for (size_t i = 0; i < lines.size(); i++) {
		instruction ins = tokenize_line(lines[i], i + 1);
		if (ins.tokens.empty())
			continue;

		if (ins.type == instruction::LABEL)
			labels.push_back(ins);

		instrs.push_back(ins);
	}

	for (instruction &ins : instrs)
		decode(ins);

	return buf;
}
