/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/misc/amsg.c
 * PURPOSE:     Print runtime error messages
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <msvcrt/stdlib.h>
#include <msvcrt/stdio.h>
#include <msvcrt/string.h>

int _aexit_rtn_dll(int exitcode)
{
	_exit(exitcode);
}

void _amsg_exit(int errnum)
{
	fprintf(stderr,strerror(errnum));
    _aexit_rtn_dll(-1);      
}

