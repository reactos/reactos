// strip_comments.cpp

#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif//_MSC_VER

#include "strip_comments.h"

void strip_comments ( std::string& s, bool strip_lf )
{
	char* src = &s[0];
	char* dst = src;
	while ( *src )
	{
		if ( src[0] == '/' && src[1] == '/' )
		{
			src += 2;
			while ( *src && *src != '\n' )
				src++;
			if ( *src )
				src++; // skip newline
		}
		else if ( src[0] == '/' && src[1] == '*' )
		{
			src += 2;
			char* newsrc = strstr ( src, "*/" );
			if ( !newsrc )
				break;
			src = newsrc;
			//while ( *src && ( src[0] != '*' || src[1] != '/' ) )
			//	src++;
			if ( *src ) src++;
			if ( *src ) src++;
		}
		else if ( src[0] == '\r' && strip_lf )
			src++;
		else
			*dst++ = *src++;
	}
	*dst = '\0';

	s.resize ( dst-&s[0] );
}
