/***
*seterrm.c - Set mode for handling critical errors
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines signal() and raise().
*
*******************************************************************************/

#include <corecrt_internal.h>
#include <stdlib.h>

/***
*void _seterrormode(mode) - set the critical error mode
*
*Purpose:
*
*Entry:
*   int mode - error mode:
*
*               0 means system displays a prompt asking user how to
*               respond to the error. Choices differ depending on the
*               error but may include Abort, Retry, Ignore, and Fail.
*
*               1 means the call system call causing the error will fail
*               and return an error indicating the cause.
*
*Exit:
*   none
*
*Exceptions:
*
*******************************************************************************/

extern "C" void __cdecl _seterrormode(int const mode)
{
    SetErrorMode(mode);
}
