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
	case '_':
		I(__cdecl);
		I(__declspec);
		I(__except);
		I(__fastcall);
		I(__finally);
		I(__inline);
		I(__int8);
		I(__int16);
		I(__int32);
		I(__int64);
		I(__leave);
		I(__stdcall);
		I(__try);
		break;
	case 'b':
		I(bool);
		I(break);
		break;
	case 'c':
		I(case);
		I(catch);
		I(char);
		I(class);
		I(const);
		I(const_cast);
		I(continue);
		break;
	case 'd':
		I(default);
		I(delete);
		I(dllexport);
		I(dllimport);
		I(do);
		I(double);
		I(dynamic_cast);
		break;
	case 'e':
		I(else);
		I(enum);
		I(explicit);
		I(extern);
		break;
	case 'f':
		I(false);
		I(float);
		I(for);
		I(friend);
		break;
	case 'g':
		I(goto);
		break;
	case 'i':
		I(if);
		I(inline);
		I(int);
		break;
	case 'l':
		I(long);
		break;
	case 'm':
		I(mutable);
		break;
	case 'n':
		I(naked);
		I(namespace);
		I(new);
		I(noreturn);
		break;
	case 'o':
		I(operator);
		break;
	case 'p':
		I(private);
		I(protected);
		I(public);
		break;
	case 'r':
		I(register);
		I(reinterpret_cast);
		I(return);
		break;
	case 's':
		I(short);
		I(signed);
		I(sizeof);
		I(static);
		I(static_cast);
		I(struct);
		I(switch);
		break;
	case 't':
		I(template);
		I(this);
		I(thread);
		I(throw);
		I(true);
		I(try);
		I(typedef);
		I(typeid);
		I(typename);
		break;
	case 'u':
		I(union);
		I(unsigned);
		I(using);
		I(uuid);
		I(__uuidof);
		break;
	case 'v':
		I(virtual);
		I(void);
		I(volatile);
		break;
	case 'w':
		I(wmain);
		I(while);
		break;
	}
	return false;
}
