// chomp.cpp
// This file is (C) 2004 Royce Mitchell III
// and released under the BSD & LGPL licenses

#include "chomp.h"

std::string chomp ( const std::string& s )
{
	const char* p = &s[0];
	const char* p2 = &s[0] + s.size();
	while ( p2 > p && strchr("\r\n", p2[-1]) )
		p2--;
	return std::string ( p, p2-p );
}

