/***
*perror.c - print system error message
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines perror() - print system error message
*       System error message are indexed by errno; conforms to XENIX
*       standard, with much compatability with 1983 uniforum draft standard.
*
*******************************************************************************/
#include <corecrt_internal.h>
#include <corecrt_internal_lowio.h>
#include <corecrt_internal_ptd_propagation.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma warning(disable:__WARNING_RETVAL_IGNORED_FUNC_COULD_FAIL) // 6031 return value ignored

/***
*void perror(message) - print system error message
*
*Purpose:
*       prints user's error message, then follows it with ": ", then the system
*       error message, then a newline.  All output goes to stderr.  If user's
*       message is nullptr or a null string, only the system error message is
*       printer.  If errno is weird, prints "Unknown error".
*
*Entry:
*       const char *message - users message to prefix system error message
*
*Exit:
*       Prints message; no return value.
*
*Exceptions:
*
*******************************************************************************/

static void __cdecl _perror_internal(char const* const user_prefix, __crt_cached_ptd_host& ptd)
{
    int const fh = 2;

    __acrt_lowio_lock_fh(fh);
    __try
    {
        if (user_prefix != nullptr && user_prefix[0] != '\0')
        {
            _write_nolock(fh, user_prefix, static_cast<unsigned>(strlen(user_prefix)), ptd);
            _write_nolock(fh, ": ", 2, ptd);
        }

        // Use PTD directly to access previously set errno value.
        char const* const system_message = _get_sys_err_msg(ptd.get_raw_ptd()->_terrno);

        _write_nolock(fh, system_message, static_cast<unsigned>(strlen(system_message)), ptd);
        _write_nolock(fh, "\n", 1, ptd);

    }
    __finally
    {
        __acrt_lowio_unlock_fh( fh );
    }
    __endtry
}

extern "C" void __cdecl perror(char const* const user_prefix)
{
    __crt_cached_ptd_host ptd;
    return _perror_internal(user_prefix, ptd);
}
