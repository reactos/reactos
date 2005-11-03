// Uses hard-coded filenames to require a minimum of input.
// You think this file should be licensed? OK then,
// Copyright (C) 2004, Mike Nordell. Use as you whish.
#include <fstream>
#include <string>
#include <algorithm>

const char szSrc1[] = "..\\..\\reactos\\ntoskrnl\\ntoskrnl.def";
const char szDst1[] = "..\\ntoskrnl\\ntoskrnl.def";
const char szSrc2[] = "..\\..\\reactos\\hal\\hal\\hal.def";
const char szDst2[] = "..\\hal\\hal.def";

enum File
{
	Kernel,
	HAL
};

std::string do_kernel_replacements(std::string& s)
{
	std::string s2 = s.c_str();	// to fixup size after replacements
	if (s2 == "ExAllocateFromPagedLookasideList") {
		s2 += "=ExiAllocateFromPagedLookasideList";
	} else
	if (s2 == "ExFreeToPagedLookasideList") {
		s2 += "=ExiFreeToPagedLookasideList";
	} else
	if (s2 == "MmLockPagableImageSection") {
		s2 += "=MmLockPagableDataSection";
	}
	return s2;
}

void convert_def(const char* szSrc, const char* szDst, File file)
{
	using namespace std;
	ifstream in(szSrc);
	ofstream out(szDst);
	string s;
	while (getline(in, s).good()) {
		if (!s.size()) {
			continue;
		}
		if (s[0] != ';') {	// only replace non-comment lines
			if (s[0] == '@') {
				s.erase(0, 1);
			}
			replace(s.begin(), s.end(), '@', '\0');
			if (file == Kernel) {
				s = do_kernel_replacements(s);
			}
		}
		out << s << endl;
	}
}


int main()
{
	convert_def(szSrc1, szDst1, Kernel);
	convert_def(szSrc2, szDst2, HAL);
	return 0;
}

