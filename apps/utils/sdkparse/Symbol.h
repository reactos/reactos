// Symbol.h

#ifndef SYMBOL_H
#define SYMBOL_H

#include "Type.h"

class Symbol
{
public:
	Type type;
	std::vector<std::string> names;
	std::vector<std::string> dependencies;
	std::vector<std::string> ifs;
	std::string definition;
};

#endif//SYMBOL_H
