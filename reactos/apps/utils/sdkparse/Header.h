// Header.h

#ifndef HEADER_H
#define HEADER_H

#include "Symbol.h"

class Header
{
public:
	std::string filename;
	std::vector<std::string> includes, libc_includes, pragmas;
	std::vector<Symbol*> symbols;
	bool done, externc;

	std::vector<std::string> ifs, ifspreproc;

	Header ( const std::string& filename_ )
		: filename(filename_)
	{
		done = false;
		externc = false;
	}
};

#endif//HEADER_H
