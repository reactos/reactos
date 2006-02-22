//
// EnumFilesImpl.h
//

#include "EnumFiles.h"
#include "FixLFN.h"
#include "safestr.h"
#ifndef UNDER_CE
#include <direct.h>
#endif//UNDER_CE
#include <stdio.h>

BOOL EnumFilesInDirectory ( const TCHAR* szDirectory_, const TCHAR* szFileSpec, MYENUMFILESPROC pProc, long lParam, BOOL bSubsToo, BOOL bSubsMustMatchFileSpec )
{
	TCHAR szDirectory[MAX_PATH+1];
	TCHAR szSearchPath[MAX_PATH+1];
	TCHAR szTemp[MAX_PATH+1];

	if ( safestrlen(szDirectory_) > MAX_PATH || !szFileSpec || !pProc )
		return FALSE;
	if ( szDirectory_ )
		_tcscpy ( szDirectory, szDirectory_ );
	else
#ifdef UNDER_CE
		_tcscpy ( szDirectory, _T("") );
#else//UNDER_CE
		getcwd ( szDirectory, sizeof(szDirectory)-1 );
#endif//UNDER_CE
	int dirlen = _tcslen(szDirectory);
	if ( dirlen > 0 && szDirectory[dirlen-1] != '\\' )
		_tcscat ( szDirectory, _T("\\") );

	// first search for all files in directory that match szFileSpec...
	_sntprintf ( szSearchPath, sizeof(szSearchPath)-1, _T("%s%s"), szDirectory, szFileSpec );
	WIN32_FIND_DATA wfd;
	HANDLE hfind = FindFirstFile ( szSearchPath, &wfd );
	if ( hfind != INVALID_HANDLE_VALUE )
	{
		do
		{
			if ( !_tcscmp ( wfd.cFileName, _T(".") ) || !_tcscmp ( wfd.cFileName, _T("..") ) )
				continue;
			if ( wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				if ( !bSubsMustMatchFileSpec )
					continue;
				_sntprintf ( szTemp, sizeof(szTemp)-1, _T("%s%s"), szDirectory, wfd.cFileName );
				if ( bSubsToo )
				{
					if ( !EnumFilesInDirectory ( szTemp, szFileSpec, pProc, lParam, bSubsToo, bSubsMustMatchFileSpec ) )
					{
						FindClose ( hfind );
						return FALSE;
					}
				}
			}
			_sntprintf ( szTemp, sizeof(szTemp)-1, _T("%s%s"), szDirectory, wfd.cFileName );
			FixLFN(szTemp,szTemp);
			if ( !pProc ( &wfd, szTemp, lParam ) )
			{
				FindClose ( hfind );
				return FALSE;
			}
		} while ( FindNextFile ( hfind, &wfd ) );
		FindClose ( hfind );
	}
	if ( !bSubsToo || bSubsMustMatchFileSpec )
		return TRUE;

	// now search for all subdirectories...
	_sntprintf ( szSearchPath, sizeof(szSearchPath)-1, _T("%s*.*"), szDirectory );
	hfind = FindFirstFile ( szSearchPath, &wfd );
	if ( hfind != INVALID_HANDLE_VALUE )
	{
		do
		{
			if ( !_tcscmp ( wfd.cFileName, _T(".") ) || !_tcscmp ( wfd.cFileName, _T("..") ) )
				continue;
			if ( !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
				continue;
			_sntprintf ( szTemp, sizeof(szTemp)-1, _T("%s%s"), szDirectory, wfd.cFileName );
			if ( FALSE == EnumFilesInDirectory ( szTemp, szFileSpec, pProc, lParam, bSubsToo, bSubsMustMatchFileSpec ) )
			{
				FindClose ( hfind );
				return FALSE;
			}
		} while ( FindNextFile ( hfind, &wfd ) );
		FindClose ( hfind );
	}

	return TRUE;
}
