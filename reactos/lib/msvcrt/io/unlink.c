/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/io/unlink.c
 * PURPOSE:     Deletes a file
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <windows.h>
#include <crtdll/io.h>



int _unlink( const char *filename )
{
	if ( !DeleteFileA(filename) )
		return -1;
	return 0;
}
