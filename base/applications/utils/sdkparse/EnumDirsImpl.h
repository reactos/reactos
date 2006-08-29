//
// EnumDirs.cpp
//

#include "EnumDirs.h"
#include <stdio.h>

#if defined(UNDER_CE) && !defined(assert)
#define assert(x)
#endif

BOOL EnumDirs ( const TCHAR* szDirectory_, const TCHAR* szFileSpec, MYENUMDIRSPROC pProc, long lParam )
{
	assert ( szDirectory_ && szFileSpec && pProc );
	TCHAR szDirectory[MAX_PATH+1];
	TCHAR szSearchPath[MAX_PATH+1];
	TCHAR szTemp[MAX_PATH+1];
	_tcscpy ( szDirectory, szDirectory_ );
	if ( szDirectory[_tcslen(szDirectory)-1] != '\\' )
		_tcscat ( szDirectory, _T("\\") );
	_sntprintf ( szSearchPath, _MAX_PATH, _T("%s%s"), szDirectory, szFileSpec );
	WIN32_FIND_DATA wfd;
	HANDLE hfind = FindFirstFile ( szSearchPath, &wfd );
	if ( hfind == INVALID_HANDLE_VALUE )
		return TRUE;
	do
	{
		if ( !_tcscmp ( wfd.cFileName, _T(".") ) || !_tcscmp ( wfd.cFileName, _T("..") ) )
			continue;
		_sntprintf ( szTemp, _MAX_PATH, _T("%s%s"), szDirectory, wfd.cFileName );
		if ( wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			if ( !pProc ( &wfd, lParam ) )
			{
				FindClose ( hfind );
				return FALSE;
			}
		}
	} while ( FindNextFile ( hfind, &wfd ) );
	FindClose ( hfind );
	return TRUE;
}
