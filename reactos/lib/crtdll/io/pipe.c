/* $Id: pipe.c,v 1.4 2002/11/24 18:42:13 robd Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/io/pipe.c
 * PURPOSE:     Creates a pipe
 * PROGRAMER:   DJ Delorie
 * UPDATE HISTORY:
 *              28/12/98: Appropriated for Reactos
 */
#include <windows.h>
#include <msvcrt/io.h>
#include <msvcrt/internal/file.h>


int _pipe(int _fildes[2], unsigned int size, int mode )
{	
	HANDLE hReadPipe, hWritePipe;
	
	if ( !CreatePipe(&hReadPipe,&hWritePipe,NULL,size))
		return -1;

	 _fildes[0] = __fileno_alloc(hReadPipe,  mode);
	 _fildes[1] = __fileno_alloc(hWritePipe, mode);
		
	return 0;
}
