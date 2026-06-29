/***
*errmode.c - modify __error_mode and __acrt_app_type
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines _set_error_mode() and _set_app_type(), the routines used
*       to modify __error_mode and __acrt_app_type variables. Together, these
*       two variables determine how/where the C runtime writes error
*       messages.
*
*******************************************************************************/

#include <corecrt_internal.h>
#include <stdlib.h>

extern "C" {



/*
 * __error_mode, together with __acrt_app_type, determine how error messages
 * are written out.
 */
static int __acrt_error_mode = _OUT_TO_DEFAULT;

/***
*int _set_error_mode(int modeval) - interface to change __error_mode
*
*Purpose:
*       Control the error (output) sink by setting the value of __error_mode.
*       Explicit controls are to direct output t o standard error (FILE * or
*       C handle or NT HANDLE) or to use the MessageBox API. This routine is
*       exposed and documented for the users.
*
*Entry:
*       int modeval =   _OUT_TO_DEFAULT, error sink is determined by __acrt_app_type
*                       _OUT_TO_STDERR,  error sink is standard error
*                       _OUT_TO_MSGBOX,  error sink is a message box
*                       _REPORT_ERRMODE, report the current __error_mode value
*
*Exit:
*       Returns old setting or -1 if an error occurs.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _set_error_mode(int const new_error_mode)
{
    switch (new_error_mode)
    {
    case _OUT_TO_DEFAULT:
    case _OUT_TO_STDERR:
    case _OUT_TO_MSGBOX:
    {
        int const old_error_mode = __acrt_error_mode;
        __acrt_error_mode = new_error_mode;
        return old_error_mode;
    }

    case _REPORT_ERRMODE:
    {
        return __acrt_error_mode;
    }

    default:
    {
        _VALIDATE_RETURN(("Invalid error_mode", 0), EINVAL, -1);
    }
    }

    return 0;
}



} // extern "C"
