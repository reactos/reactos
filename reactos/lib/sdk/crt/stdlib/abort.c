/*
 * PROJECT:         ReactOS C runtime library
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            lib/sdk/crt/stdlib/abort.c
 * PURPOSE:         abort implementation
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include "precomp.h"
#include <signal.h>

unsigned int __abort_behavior =  _WRITE_ABORT_MSG | _CALL_REPORTFAULT;

static const char abort_msg[] =
    "This application has requested the Runtime to terminate in an unusual way.\n"
    "Please contact the applications's support team for more information.\0";

/*!
 * \brief Aborts the program.
 *
 * \note The function does not return.
 */
void
__cdecl
abort (
    void)
{
    /* Check if a message should be output */
    if (__abort_behavior & _WRITE_ABORT_MSG)
    {
        /* Check if we should display a message box */
        if (((msvcrt_error_mode == _OUT_TO_DEFAULT) && (__app_type == _GUI_APP)) ||
            (msvcrt_error_mode == _OUT_TO_MSGBOX))
        {
            /* Output a message box */
            __crt_MessageBoxA(abort_msg, MB_OK | MB_ICONERROR);
        }
        else
        {
            /* Print message to stderr */
            fprintf(stderr, "%s\n", abort_msg);
        }
    }

    /* Check if faultrep handler should be called */
    if (__abort_behavior & _CALL_REPORTFAULT)
    {
        /// \todo unimplemented
        (void)0;
    }

    raise(SIGABRT);
    _exit(3);
}

