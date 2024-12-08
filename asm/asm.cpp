/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "common.hpp"
#include "asm.hpp"
#include "hash.h"

#define ADMODE_IMM 0x0008
#define ADMODE_REG 0x0000

#define LOWER_BYTE(x) ((x) & 0xFF)
#define UPPER_BYTE(x) (((x) >> 8) & 0xFF)

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

struct instruction {
	enum type {
		OPC,
		LABEL,
	} type;

	vector<token> tokens;

	uint16_t opcode;
};

static vector<instruction> instrs;
static unordered_map<string, size_t> labels;
static unordered_map<string, vector<size_t> > unresolved_labels;

static vector<unsigned char> stream;
static size_t cur_index = 0;

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

static void resolve_label(const string &label)
{
	auto it = unresolved_labels.find(label);
	if (it == unresolved_labels.end())
		return;

	if (!labels.count(label))
		return;

	size_t ptr = labels[label];
	for (size_t ref_ptr : it->second) {
		stream[ref_ptr] = LOWER_BYTE(ptr);
		stream[ref_ptr + 1] = UPPER_BYTE(ptr);
	}
}

static void attempt_resolve_label(const string &label, size_t ref_ptr)
{
	auto it = labels.find(label);
	if (it == labels.end()) {
		/* Mark as unresolved */
		unresolved_labels[label].push_back(ref_ptr);
	} else {
		/* Found */
		size_t ptr = it->second;

		stream[ref_ptr] = LOWER_BYTE(ptr);
		stream[ref_ptr + 1] = UPPER_BYTE(ptr);
	}
}

static int parse_register(const string &reg)
{
	char r1 = tolower(reg[1]);
	r1 = tolower(r1);
	if (reg.size() != 2) {
		cerr << "Invalid register: " << reg << endl;
		return -1;
	}

	char r = r1;
	if (r < 'a' || r > 'd') {
		cerr << "Invalid register: " << reg << endl;
		return -1;
	}

	return r - 'a';
}

static int parse_register_or_imm(const string &reg, long &ret)
{
	if (reg[0] == '%') {
		ret = parse_register(reg);
		return ADMODE_REG;
	} else if (isdigit(reg[0])) {
		ret = stoul(reg);
		return ADMODE_IMM;
	} else if (reg[0] == '$') {
		ret = stoul(reg.substr(1, reg.length() - 1), nullptr, 16);
		return ADMODE_IMM;
	} else {
		ret = 0;
		return 4;
	}
}

static void parse_instruction_arith(instruction &ins)
{
	token t1 = ins.tokens[1];
	token t2 = ins.tokens[2];

	string r1 = t1.str;
	string r2 = t2.str;

	ins.opcode = ADMODE_REG;
	ins.opcode |= ins.opcode << 12;
	ins.opcode |= (parse_register(r1) & 0x3) << 6;
	ins.opcode |= (parse_register(r2) & 0x3) << 4;

	stream[cur_index] = ins.opcode;
	cur_index++;
}

static void parse_instruction_ld(instruction &ins)
{
	/* In ST, r1 is optional IMM/REGPTR, and r2 is mandatory REG */
	/* In LD, r1 is mandatory REG, and r2 is optional IMM/REGPTR */
	token t1 = ins.tokens[1];
	token t2 = ins.tokens[2];

	long rx_val;
	int rx_result = parse_register(t1.str);

	long opt_val;
	int opt_result = parse_register_or_imm(t2.str, opt_val);

	if (opt_result == -1 || rx_result == -1) {
		cerr << "Error on line " << t1.line_no << ": Invalid register/imm: " << t1.str << " or " << t2.str << endl;
		return;
	}

	if (opt_result == 4) {
		/* Label reference */
		ins.opcode = ADMODE_IMM;
		ins.opcode |= (rx_result & 0x3) << 6;
		attempt_resolve_label(t2.str, cur_index + 1);
	} else {
		ins.opcode = opt_result;
	}

	ins.opcode |= INST_LD << 12;

	if (ins.opcode & ADMODE_IMM) {
		stream[cur_index + 1] = LOWER_BYTE(opt_val);
		stream[cur_index + 2] = UPPER_BYTE(opt_val);
		cur_index += 2;
	} else {
		ins.opcode |= (opt_val) << 4;
	}

	stream[cur_index] = ins.opcode;
	cur_index++;
}

static void parse_instruction_st(instruction &ins)
{
	/* In ST, r1 is optional IMM/REGPTR, and r2 is mandatory REG */
	/* In LD, r1 is mandatory REG, and r2 is optional IMM/REGPTR */
	token t1 = ins.tokens[1];
	token t2 = ins.tokens[2];

	long rx_val;
	int rx_result = parse_register(t2.str);

	long opt_val;
	int opt_result = parse_register_or_imm(t1.str, opt_val);

	if (opt_result == -1 || rx_result == -1) {
		cerr << "Error on line " << t1.line_no << ": Invalid register/imm: " << t1.str << " or " << t2.str << endl;
		return;
	}

	if (opt_result == 4) {
		/* Label reference */
		ins.opcode = ADMODE_IMM;
		ins.opcode |= (rx_result & 0x3) << 4;
		attempt_resolve_label(t1.str, cur_index + 1);
	} else {
		ins.opcode = opt_result;
	}

	ins.opcode |= INST_LD << 12;

	if (ins.opcode & ADMODE_IMM) {
		stream[cur_index + 1] = LOWER_BYTE(opt_val);
		stream[cur_index + 2] = UPPER_BYTE(opt_val);
		cur_index += 2;
	} else {
		ins.opcode |= (opt_val) << 6;
	}

	stream[cur_index] = ins.opcode;
	cur_index++;
}

static void parse_inst_push_pop(instruction &ins)
{
	token t1 = ins.tokens[1];

	int r1_result = parse_register(t1.str);
	if (r1_result == -1) {
		cerr << "Error on line " << t1.line_no << ": Invalid register: " << t1.str << endl;
		return;
	}

	ins.opcode <<= 12;
	ins.opcode |= r1_result << 6;

	stream[cur_index] = ins.opcode;
	cur_index++;
}

static void parse_inst_jnz(instruction &ins)
{
	token t1 = ins.tokens[1];

	long val;
	int result = parse_register_or_imm(t1.str, val);

	if (result == -1) {
		cerr << "Error on line " << t1.line_no << ": Invalid register/imm: " << t1.str << endl;
		return;
	}

	ins.opcode = INST_JNZ << 12;
	ins.opcode |= result;

	if (result == 4) {
		attempt_resolve_label(t1.str, cur_index + 1);
		ins.opcode |= ADMODE_IMM;
		cur_index += 2;
	} else if (result == ADMODE_IMM) {
		stream[cur_index + 1] = LOWER_BYTE(val);
		stream[cur_index + 2] = UPPER_BYTE(val);

		ins.opcode |= ADMODE_IMM;
		cur_index += 2;
	} else {
		ins.opcode |= val << 6;
	}

	stream[cur_index] = ins.opcode;
	cur_index++;
}

static void parse_inst_cli_sti(instruction &ins)
{
	ins.opcode <<= 12;
	stream[cur_index] = ins.opcode;
}

static void parse_inst_int(instruction &ins)
{
	token t1 = ins.tokens[1];

	long val;
	int result = parse_register_or_imm(t1.str, val);

	if (result == 4 || result == ADMODE_REG) {
		cerr << "Error on line " << t1.line_no << ": Invalid imm: " << t1.str << endl;
		return;
	}

	ins.opcode = INST_INT << 12;
	ins.opcode |= ADMODE_IMM;

	stream[cur_index] = ins.opcode;
	stream[cur_index + 1] = LOWER_BYTE(val);
	stream[cur_index + 2] = UPPER_BYTE(val);
}

static void inst_parse(instruction &ins)
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
		cerr << "Expected: LABEL or OPC" << endl;
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

	switch (ins.opcode) {
	case INST_CMP:
	case INST_ADD:
	case INST_SUB:
	case INST_OR:
	case INST_AND:
	case INST_XOR:
	case INST_RSH:
	case INST_LSH:
		parse_instruction_arith(ins);
		break;
	case INST_ST:
		parse_instruction_st(ins);
		break;
	case INST_LD:
		parse_instruction_ld(ins);
		break;
	case INST_PUSH:
	case INST_POP:
		parse_inst_push_pop(ins);
		break;
	case INST_JNZ:
		parse_inst_jnz(ins);
		break;
	case INST_CLI:
	case INST_STI:
		parse_inst_cli_sti(ins);
		break;
	case INST_INT:
		parse_inst_int(ins);
		break;
	default:
		cerr << "Error on line " << t0.line_no << ": Invalid opcode: " << ins.opcode << endl;
	}
}

string assemble(const string &src)
{
	vector<string> lines;
	stringstream ss(src);

	for (string line; getline(ss, line);)
		lines.push_back(line);

	ss.clear();

	for (size_t i = 0; i < lines.size(); i++) {
		instruction ins = tokenize_line(lines[i], i + 1);
		if (ins.tokens.empty())
			continue;

		if (ins.tokens[0].type == token::LABEL) {
			string label = ins.tokens[0].str.substr(0, ins.tokens[0].str.size() - 1);
			if (labels.find(label) != labels.end()) {
				cerr << "Error on line " << ins.tokens[0].line_no << ": Duplicate label: " << label << endl;
				continue;
			}
			labels[label] = cur_index;

			resolve_label(label);
		}

		instrs.push_back(ins);
	}

	for (instruction &ins : instrs)
		inst_parse(ins);

	string ret;
	for (int i = 0; i < stream.size(); i++) {
		ret += stream[i];
	}

	return ret;
}
