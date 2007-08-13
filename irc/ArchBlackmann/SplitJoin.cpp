// SplitJoin.cpp
//
// This code is copyright 2003-2004 Royce Mitchell III
// and released under the BSD & LGPL licenses

#ifdef _MSC_VER
#pragma warning ( disable : 4786 ) // MSVC6 can't handle too-long template names
#endif//_MSC_VER

//#include <sstream>

#include "SplitJoin.h"

#include <string.h>

using std::string;
using std::vector;
//using std::stringstream;

static const char* quotes = "\"\'";

bool Split ( vector<string>& vec, const char* csv, char sep, bool merge )
{
	string scsv ( csv );
	char* col = &scsv[0];
	vec.resize ( 0 );
	for ( ;; )
	{
		char* p = col;
		while ( isspace(*p) && *p != sep )
			p++;
		char quote = 0;
		if ( strchr ( quotes, *p ) )
			quote = *p++;
		while ( *p && (*p != sep || quote) )
		{
			if ( *p++ == quote )
				break;
		}

		while ( isspace(*p) && *p != sep )
			p++;

		if ( *p && *p != sep )
			return false;

		string scol ( col, p-col );

		//quote = scol[0];
		if ( quote )
		{
			if ( scol[scol.size()-1] == quote )
				scol = string ( &scol[1], scol.size()-2 );
		}

		if ( scol.length() || !merge )
			vec.push_back ( scol );

		if ( !*p )
			break;

		col = p + 1;
	}
	return true;
}

bool Join ( string& csv, vector<string>& vec, char sep )
{
	csv.resize(0);
	for ( int i = 0; i < vec.size(); i++ )
	{
		if ( i )
			csv += sep;
		string& s = vec[i];
		if ( strchr ( s.c_str(), sep ) )
		{
			if ( strchr ( s.c_str(), '\"' ) )
			{
				if ( strchr ( s.c_str(), '\'' ) )
					return false; // the sep, " and ' are all in the string, can't build valid output
				csv += '\'';
				csv += s;
				csv += '\'';
			}
			else
			{
				csv += '\"';
				csv += s;
				csv += '\"';
			}
		}
		else
			csv += s;
	}
	return true;
}
