/* SPDX-License-Identifier: GPL-2.0-only */
#include "common.hpp"
#include "asm.hpp"
#include "hash.h"

struct macro {
	string name;
	string body;
};

static unordered_map<string, macro> macros;

static macro *macro_register(const string &name)
{
	if (macros.contains(name)) {
		cerr << "Error: Macro already defined: " << name << endl;
		return nullptr;
	}

	macro m;
	m.name = name;
	macros[name] = m;

	return &macros[name];
}

int preprocess(vector<line> &lines)
{
	macro *cur_macro = nullptr;
	for (int i = 0; i < lines.size(); i++) {
		line *line = &lines[i];
		size_t line_no = i + 1;
		instruction ins = tokenize_line(*line, i + 1);

		if (ins.tokens.empty())
			continue;

		struct keyword *kw = asm_keyword_lookup(ins.tokens[0].str.c_str(), ins.tokens[0].str.size());

		if (kw && kw->opc == MACR_DEF) {
			if (cur_macro) {
				cerr << "Error on line " << line_no << ": Nested macro definition" << endl;
				return -1;
			}

			macro *m = macro_register(ins.tokens[1].str);
			if (!m)
				return -1;

			cur_macro = m;
		} else if (kw && kw->opc == MACR_END) {
			if (!cur_macro) {
				cerr << "Error on line " << line_no << ": Macro end without definition" << endl;
				return -1;
			}

			cur_macro = nullptr;
		} else if (cur_macro) {
			cur_macro->body += line->line + "\n";
		} else if (macros.contains(ins.tokens[0].str)) {
			macro m = macros[ins.tokens[0].str];

			stringstream ss(m.body);
			string line_read;
			for (size_t j = 1; getline(ss, line_read); j++) {
				struct line replace;
				replace.line = line_read;
				replace.line_no = line_no;
				lines.insert(lines.begin() + i + j, replace);
			}

			lines.erase(lines.begin() + i);
		}
	}

	return 0;
}
