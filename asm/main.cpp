/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <getopt.h>
#include "common.hpp"
#include "asm.hpp"

static string read_file(const string &if_name)
{
	ifstream ifile(if_name);
	if (!ifile) {
		cerr << "Error: cannot open file " << if_name << '\n';
	}

	string buf;

	string line;
	while (getline(ifile, line)) {
		buf += line + '\n';
	}

	return buf;
}

int main(int argc, char *argv[])
{
	/* Parse arguments */
	int opt;

	string if_name;
	string of_name;

	while ((opt = getopt(argc, argv, "c:o:")) != -1) {
		switch (opt) {
		case 'c':
			if_name = optarg;
			break;
		case 'o':
			of_name = optarg;
			break;
		default:
			cerr << "Usage: " << argv[0] << " [-co] [file...]\n";
			return 1;
		}
	}

	if (if_name.empty()) {
		if_name = "test.s";
	}

	if (of_name.empty())
		of_name = "out.bin";

	string buf = read_file(if_name);

	string ret = assemble(buf);

	ofstream ofile(of_name);
	ofile << ret;

	return 0;
}
