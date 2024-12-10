/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "common.hpp"
#include "asm.hpp"
#include "hash.h"

#define ADMODE_IMM 0x0008
#define ADMODE_REG 0x0000

#define LOWER_BYTE(x) ((x) & 0xFF)
#define UPPER_BYTE(x) (((x) >> 8) & 0xFF)
#define WRITE_STREAM(x, index)                     \
	do {                                       \
		stream[index] = LOWER_BYTE(x);     \
		stream[index + 1] = UPPER_BYTE(x); \
	} while (0)

struct instruction;
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

	size_t line_no;
	uint16_t opcode;
};

static vector<instruction> instrs;
static unordered_map<string, size_t> labels;
static unordered_map<string, vector<size_t> > unresolved_labels;

static vector<unsigned char> stream;
static size_t cur_index = 0;
static size_t n_errors = 0;

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
	else if (token_s[0] == '"' && token_s.back() == '"')
		return token::STRING;
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

	bool is_escape = false;
	bool quote_open = false;
	string cur_token;
	for (size_t i = 0; i < line.size(); i++) {
		char c = line[i];
		bool quote_end = false;

		if (c == '#')
			break;

		if (c == '\\' && !is_escape) {
			is_escape = true;
			continue;
		}

		if (c == '"' && !is_escape) {
			quote_end = quote_open;
			quote_open = !quote_open;
		}

		if (is_separator(c) && !quote_open) {
			if (!cur_token.empty())
				tokens_s.push_back(cur_token);

			cur_token.clear();
		} else {
			cur_token.push_back(c);
		}

		is_escape = false;
	}

	if (!cur_token.empty())
		tokens_s.push_back(cur_token);

	if (quote_open) {
		cerr << "Error: Unclosed quote" << endl;
		n_errors++;
	}

	return tokens_s;
}

static instruction tokenize_line(const string &line, size_t line_no)
{
	instruction ret;

	ret.line_no = line_no;

	vector<string> tokens_s = tokenize_line_s(line);
	for (const string &token_s : tokens_s) {
		token t;
		t.type = get_token_type(token_s);
		t.str = token_s;

		ret.tokens.push_back(t);
	}

	return ret;
}

static void resolve_label(const string &label)
{
	if (!labels.contains(label)) {
		cerr << "Error: Label not found: " << label << endl;
		n_errors++;
		return;
	}

	if (!unresolved_labels.contains(label))
		return;

	for (size_t ref_ptr : unresolved_labels[label]) {
		size_t ptr = labels[label];

		WRITE_STREAM(ptr, ref_ptr);
	}
}

static void attempt_resolve_label(const string &label, size_t ref_ptr)
{
	if (!labels.contains(label)) {
		/* Mark as unresolved */
		unresolved_labels[label].push_back(ref_ptr);
	} else {
		/* Found */
		size_t ptr = labels[label];
		WRITE_STREAM(ptr, ref_ptr);
	}
}

static int parse_register(const string &reg)
{
	char r1 = tolower(reg[1]);
	r1 = tolower(r1);
	if (reg.size() != 2) {
		cerr << "Invalid register: " << reg << endl;
		n_errors++;
		return -1;
	}

	char r = r1;
	if (r < 'a' || r > 'd') {
		cerr << "Invalid register: " << reg << endl;
		n_errors++;
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

	WRITE_STREAM(ins.opcode, cur_index);
	cur_index += 2;
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
		cerr << "Error on line " << ins.line_no << ": Invalid register/imm: " << t1.str << " or " << t2.str << endl;
		n_errors++;
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
		WRITE_STREAM(ins.opcode, cur_index);
		WRITE_STREAM(opt_val, cur_index + 2);
		cur_index += 4;
	} else {
		ins.opcode |= (opt_val) << 4;
		WRITE_STREAM(ins.opcode, cur_index);
		cur_index += 2;
	}
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
		cerr << "Error on line " << ins.line_no << ": Invalid register/imm: " << t1.str << " or " << t2.str << endl;
		n_errors++;
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
		WRITE_STREAM(ins.opcode, cur_index);
		WRITE_STREAM(opt_val, cur_index + 2);
		cur_index += 4;
	} else {
		ins.opcode |= (opt_val) << 6;
		WRITE_STREAM(ins.opcode, cur_index);
		cur_index += 2;
	}
}

static void parse_inst_push_pop(instruction &ins)
{
	token t1 = ins.tokens[1];

	int r1_result = parse_register(t1.str);
	if (r1_result == -1) {
		cerr << "Error on line " << ins.line_no << ": Invalid register: " << t1.str << endl;
		n_errors++;
		return;
	}

	ins.opcode <<= 12;
	ins.opcode |= r1_result << 6;

	WRITE_STREAM(ins.opcode, cur_index);
	cur_index += 2;
}

static void parse_inst_jnz(instruction &ins)
{
	token t1 = ins.tokens[1];

	long val;
	int result = parse_register_or_imm(t1.str, val);

	if (result == -1) {
		cerr << "Error on line " << ins.line_no << ": Invalid register/imm: " << t1.str << endl;
		n_errors++;
		return;
	}

	ins.opcode = INST_JNZ << 12;
	/* either of these bits will result in ADMODE_IMM */
	ins.opcode |= (result) & 0x8 | ((result & 0x4) << 1);

	if (result == 4) {
		WRITE_STREAM(ins.opcode, cur_index);
		attempt_resolve_label(t1.str, cur_index + 2);
		cur_index += 4;
	} else if (result == ADMODE_IMM) {
		WRITE_STREAM(ins.opcode, cur_index);
		WRITE_STREAM(val, cur_index + 2);

		cur_index += 2;
	} else {
		ins.opcode |= val << 6;
		WRITE_STREAM(ins.opcode, cur_index);
		cur_index += 2;
	}
}

static void parse_inst_cli_sti(instruction &ins)
{
	WRITE_STREAM(ins.opcode, cur_index);
	cur_index += 2;
}

static void parse_inst_int(instruction &ins)
{
	token t1 = ins.tokens[1];

	long val;
	int result = parse_register_or_imm(t1.str, val);

	if (result == 4 || result == ADMODE_REG) {
		cerr << "Error on line " << ins.line_no << ": Invalid imm: " << t1.str << endl;
		n_errors++;
		return;
	}

	ins.opcode = INST_INT << 12;
	ins.opcode |= ADMODE_IMM;

	stream[cur_index] = LOWER_BYTE(ins.opcode);
	stream[cur_index + 1] = UPPER_BYTE(ins.opcode);
	stream[cur_index + 2] = LOWER_BYTE(val);
	stream[cur_index + 3] = UPPER_BYTE(val);

	cur_index += 4;
}

static void parse_macro_str(instruction &ins)
{
	token t1 = ins.tokens[1];
	string str = t1.str.substr(1, t1.str.size() - 2);

	for (size_t i = 0; i < str.size(); i++) {
		stream[cur_index++] = str[i];
	}
}

static void parse_macro_stz(instruction &ins)
{
	parse_macro_str(ins);
	stream[cur_index++] = 0;
}

static void parse_macro_org(instruction &ins)
{
	token t1 = ins.tokens[1];
	long val;
	int result = parse_register_or_imm(t1.str, val);

	if (result != ADMODE_IMM) {
		cerr << "Error on line " << ins.line_no << ": Invalid imm: " << t1.str << endl;
		n_errors++;
		return;
	}

	cur_index = val;
}

static void inst_parse(instruction &ins)
{
	token t0 = ins.tokens[0];

	size_t n_expected = 1;

	if (t0.type == token::LABEL) {
		if (ins.tokens.size() != 1) {
			cerr << "Error on line " << ins.line_no << ": Expected 1 token, got " << ins.tokens.size();
			cerr << " (label)" << endl;
			n_errors++;
		}
		return;
	}

	struct keyword *kw = asm_keyword_lookup(t0.str.c_str(), t0.str.size());
	ins.opcode = kw->opc;
	n_expected += kw->n_args;

	if (ins.tokens.size() != n_expected) {
		cerr << "Error on line " << ins.line_no << ": Expected " << n_expected << " tokens, got " << ins.tokens.size();
		cerr << " (opcode: " << t0.str << ")" << endl;
		n_errors++;
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
	case MACR_STR:
		parse_macro_str(ins);
		break;
	case MACR_ZST:
		parse_macro_stz(ins);
		break;
	case MACR_ORG:
		parse_macro_org(ins);
		break;
	default:
		cerr << "Error on line " << ins.line_no << ": Invalid opcode: " << ins.opcode << endl;
		n_errors++;
		break;
	}
}

string assemble(const string &src)
{
	vector<string> lines;
	stringstream ss(src);

	for (string line; getline(ss, line);)
		lines.push_back(line);

	ss.clear();

	stream.resize(0x10000);
	static size_t max_index = 0;

	for (size_t i = 0; i < lines.size(); i++) {
		instruction ins = tokenize_line(lines[i], i + 1);
		if (ins.tokens.empty())
			continue;

		if (ins.tokens[0].type == token::LABEL) {
			string label = ins.tokens[0].str.substr(0, ins.tokens[0].str.size() - 1);
			if (labels.contains(label)) {
				cerr << "Error on line " << ins.line_no << ": Duplicate label: " << label << endl;
				n_errors++;
				continue;
			}
			labels[label] = cur_index;

			resolve_label(label);
		}

		instrs.push_back(ins);
		inst_parse(ins);
		if (cur_index > max_index)
			max_index = cur_index;
	}

	stream.resize(max_index);

	if (n_errors)
		return "";

	return string(stream.begin(), stream.end());
}
