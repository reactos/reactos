/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/stdlib/fullpath.c
 * PURPOSE:     Gets the fullpathname
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <stdlib.h>
#include <windows.h>


char *_fullpath( char *absPath, const char *relPath, size_t maxLength )
{
	char *lpFilePart;
	if ( GetFullPathName(relPath,maxLength,absPath,&lpFilePart) == 0 )
		return NULL;

	return absPath;
}
		
  

  
