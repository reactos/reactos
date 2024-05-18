// binary2cstr.cpp

#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif//_MSC_VER

#include "binary2cstr.h"

using std::string;

string binary2cstr ( const string& src )
{
	string dst;
	for ( int i = 0; i < src.size(); i++ )
	{
		char c = src[i];
		switch ( c )
		{
		case '\n':
			dst += "\\n";
			break;
		case '\r':
			dst += "\\r";
			break;
		case '\t':
			dst += "\\t";
			break;
		case '\v':
			dst += "\\v";
			break;
		case '\"':
			dst += "\x22";
			break;
		default:
			if ( isprint ( c ) )
				dst += c;
			else
			{
				dst += "\\x";
				char tmp[16];
				_snprintf ( tmp, sizeof(tmp)-1, "%02X", (unsigned)(unsigned char)c );
				tmp[sizeof(tmp)-1] = '\0';
				dst += tmp;
			}
			break;
		}
	}
	return dst;
}
