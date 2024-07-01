/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/_sxprintf.c
 * PURPOSE:         Implementation of swprintf
 * PROGRAMMER:      Timo Kreuzer
 */

#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <tchar.h>
#if IS_SECAPI
#include <internal/safecrt.h>
#endif

#ifdef _UNICODE
#define _tstreamout wstreamout
#else
#define _tstreamout streamout
#endif

#define min(a,b) (((a) < (b)) ? (a) : (b))

int __cdecl _tstreamout(FILE *stream, const _TCHAR *format, va_list argptr);

int
#if defined(USER32_WSPRINTF) && defined(_M_IX86)
__stdcall
#else
__cdecl
#endif
_sxprintf(
    _TCHAR *buffer,
#if IS_SECAPI
    size_t sizeOfBuffer,
#endif
#if USE_COUNT
   size_t count,
#endif
    const _TCHAR *format,
#if USE_VARARGS
    va_list argptr)
#else
    ...)
#endif
{
#if !USE_COUNT
    const size_t count = INT_MAX;
#endif
#if !IS_SECAPI
    const size_t sizeOfBuffer = count;
#endif
#if !USE_VARARGS
    va_list argptr;
#endif
    int result;
    FILE stream;

#if IS_SECAPI
    /* Validate parameters */
    if (MSVCRT_CHECK_PMT(((buffer == NULL) || (format == NULL) || (sizeOfBuffer <= 0))))
    {
        errno = EINVAL;
        return -1;
    }

    /* Limit output to count + 1 characters */
    if (count != -1)
        sizeOfBuffer = min(sizeOfBuffer, count + 1);
#endif

    /* Setup the FILE structure */
    stream._base = (char*)buffer;
    stream._ptr = stream._base;
    stream._charbuf = 0;
    stream._cnt = (int)(sizeOfBuffer * sizeof(_TCHAR));
    stream._bufsiz = 0;
    stream._flag = _IOSTRG | _IOWRT;
    stream._tmpfname = 0;

#if !USE_VARARGS
    va_start(argptr, format);
#endif
    result = _tstreamout(&stream, format, argptr);
#if !USE_VARARGS
    va_end(argptr);
#endif

#if IS_SECAPI
    /* Check for failure or unterminated string */
    if ((result < 0) || (result == sizeOfBuffer))
    {
        /* Null-terminate the buffer at the end */
        buffer[sizeOfBuffer-1] = _T('\0');

        /* Check if we can truncate */
        if (count != _TRUNCATE)
        {
            /* We can't, invoke invalid parameter handler */
            MSVCRT_INVALID_PMT("Buffer is too small", ERANGE);

            /* If we came back, set the buffer to an empty string */
            *buffer = 0;
        }

        /* Return failure */
        return -1;
    }

    /* Null-terminate the buffer after the string */
    buffer[result] = _T('\0');
#else
    /* Only zero terminate if there is enough space left */
    if ((stream._cnt >= sizeof(_TCHAR)) && (stream._ptr))
        *(_TCHAR*)stream._ptr = _T('\0');
#endif

    return result;
}


