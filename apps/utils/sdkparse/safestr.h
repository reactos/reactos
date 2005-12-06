//
// safestr.h
//
// safe versions of some string manipulation routines
//

#ifndef __SAFESTR_H
#define __SAFESTR_H

#include <tchar.h>

#ifndef tchar
#define tchar TCHAR
#endif//tchar

#include <string.h>
#include "assert.h"

inline size_t safestrlen ( const tchar *string )
{
	if ( !string )
		return 0;
	return _tcslen(string);
}

inline tchar* safestrcpy ( tchar* strDest, const tchar* strSource )
{
	ASSERT(strDest);
	if ( !strSource )
		*strDest = 0;
	else
		_tcscpy ( strDest, strSource );
	return strDest;
}

inline tchar* safestrncpy ( tchar* strDest, const tchar* strSource, size_t count )
{
	ASSERT(strDest);
	if ( !strSource )
		count = 0;
	else
		_tcsncpy ( strDest, strSource, count );
	strDest[count] = 0;
	return strDest;
}

inline tchar* safestrlwr ( tchar* str )
{
	if ( !str )
		return 0;
	return _tcslwr(str);
}

inline tchar* safestrupr ( tchar* str )
{
	if ( !str )
		return 0;
	return _tcsupr(str);
}

#endif//__SAFESTR_H
