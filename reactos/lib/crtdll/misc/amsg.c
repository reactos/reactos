/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/conio/kbhit.c
 * PURPOSE:     Checks for keyboard hits
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <crtdll/stdlib.h>
#include <crtdll/stdio.h>
#include <crtdll/string.h>

int  _aexit_rtn_dll(int exitcode)
{
	_exit(exitcode);
}

void _amsg_exit (int errnum)
{
	fprintf(stderr,strerror(errnum));
        _aexit_rtn_dll(-1);      
}

