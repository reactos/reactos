// trim.cpp
// This file is (C) 2004 Royce Mitchell III
// and released under the BSD & LGPL licenses

#include "trim.h"

std::string trim ( const std::string& s )
{
	const char* p = &s[0];
	const char* p2 = p + s.size();
	while ( *p == ' ' )
		p++;
	while ( p2 > p && p2[-1] == ' ' )
		p2--;
	return std::string ( p, p2-p );
}

