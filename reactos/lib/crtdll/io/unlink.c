/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/io/unlink.c
 * PURPOSE:     Deletes a file
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <windows.h>
#include <io.h>

int unlink( const char *filename )
{
	return _unlink(filename);
}

int _unlink( const char *filename )
{
	if ( !DeleteFile(filename) )
		return -1;
	return 0;
}