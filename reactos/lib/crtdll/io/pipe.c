/* $Id: pipe.c,v 1.6 2004/08/15 17:34:26 chorns Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/io/pipe.c
 * PURPOSE:     Creates a pipe
 * PROGRAMER:   DJ Delorie
 * UPDATE HISTORY:
 *              28/12/98: Appropriated for Reactos
 */

#include "precomp.h"
#include <msvcrt/io.h>
#include <msvcrt/internal/file.h>


/*
 * @implemented
 */
int _pipe(int _fildes[2], unsigned int size, int mode )
{	
	HANDLE hReadPipe, hWritePipe;
	
	if ( !CreatePipe(&hReadPipe,&hWritePipe,NULL,size))
		return -1;

	 _fildes[0] = __fileno_alloc(hReadPipe,  mode);
	 _fildes[1] = __fileno_alloc(hWritePipe, mode);
		
	return 0;
}
