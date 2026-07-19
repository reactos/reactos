/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     C99 printf-family shims for llvm-mingw runtime libraries
 * COPYRIGHT:   Copyright 2026 Ahmed Arif <arif.ing@outlook.com>
 */

#include <stdio.h>
#include <stdarg.h>

/* llvm-mingw's libc++ needs the C99 vsnprintf/snprintf contract, which msvcrt's _vsnprintf (aliased onto
 * these names by the crt headers) does not follow: undo the mapping and bridge over _vsnprintf/_vscprintf */
#undef vsnprintf
#undef snprintf

int vsnprintf(char *buffer, size_t count, const char *format, va_list argptr)
{
    va_list ap;
    int result;

    va_copy(ap, argptr);
    result = _vsnprintf(buffer, count, format, ap);
    va_end(ap);

    if (result >= 0 && (size_t)result < count)
        return result;

    /* Truncated: terminate and return the would-be length */
    if (count != 0)
        buffer[count - 1] = '\0';

    va_copy(ap, argptr);
    result = _vscprintf(format, ap);
    va_end(ap);

    return result;
}

int snprintf(char *buffer, size_t count, const char *format, ...)
{
    va_list argptr;
    int result;

    va_start(argptr, format);
    result = vsnprintf(buffer, count, format, argptr);
    va_end(argptr);

    return result;
}
