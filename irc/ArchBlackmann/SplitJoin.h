// SplitJoin.h
//
// This code is copyright 2003-2004 Royce Mitchell III
// and released under the BSD & LGPL licenses

#ifndef SPLITJOIN_H
#define SPLITJOIN_H

#include <vector>
#include <string>

bool Split (
	std::vector<std::string>& vec,
	const char* csv,
	char sep=',',
	bool merge=false );

bool Join (
	std::string& csv,
	std::vector<std::string>& vec,
	char sep=',' );

inline bool Split (
	std::vector<std::string>& vec,
	const std::string& csv,
	char sep=',',
	bool merge=false )
{
	return Split ( vec, csv.c_str(), sep, merge );
}

#endif//SPLIT_H
