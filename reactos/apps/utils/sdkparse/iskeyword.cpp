// iskeyword.cpp

#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif//_MSC_VER

#include <string>

#include "iskeyword.h"

using std::string;

bool iskeyword ( const string& ident )
{
#define I(s) if ( ident == #s ) return true;
	switch ( ident[0] )
	{
	case 'b':
		I(bool);
		break;
	case 'c':
		I(char);
		I(const);
		break;
	case 'd':
		I(do);
		I(double);
		break;
	case 'f':
		I(false);
		I(float);
		I(for);
		break;
	case 'i':
		I(if);
		I(int);
		break;
	case 'l':
		I(long);
		break;
	case 'r':
		I(return);
		break;
	case 's':
		I(short);
		I(struct);
		I(switch);
		break;
	case 't':
		I(true);
		I(typedef);
		break;
	case 'w':
		I(while);
		break;
	}
	return false;
}
