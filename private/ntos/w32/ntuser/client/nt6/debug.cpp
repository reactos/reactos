//
//
//

#include "pch.h"
#include "debug.h"

#ifdef _DEBUG
void FAR _cdecl 
TRACE(
	LPTSTR lpszFormat, 
	...) 
{
	TCHAR	szBuf[1024];
	int	cchAdd;

	cchAdd = wvsprintf(szBuf, lpszFormat, (LPSTR)(&lpszFormat + 1));
	OutputDebugString(szBuf);
}
#endif

