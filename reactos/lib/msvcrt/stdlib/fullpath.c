/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/stdlib/fullpath.c
 * PURPOSE:     Gets the fullpathname
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <msvcrti.h>


char *_fullpath( char *absPath, const char *relPath, size_t maxLength )
{
	char *lpFilePart;
	if ( GetFullPathNameA(relPath,maxLength,absPath,&lpFilePart) == 0 )
		return NULL;

	return absPath;
}

wchar_t *_wfullpath( wchar_t *absPath, const wchar_t *relPath, size_t maxLength )
{
	wchar_t *lpFilePart;
	if ( GetFullPathNameW(relPath,maxLength,absPath,&lpFilePart) == 0 )
		return NULL;

	return absPath;
}
