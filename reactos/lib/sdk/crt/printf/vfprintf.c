/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/vfprintf.c
 * PURPOSE:         Implementation of vfprintf
 * PROGRAMMER:      Timo Kreuzer
 *                  Samuel Serapión
 */

#include <precomp.h>

void CDECL _lock_file(FILE* file);
void CDECL _unlock_file(FILE* file);
int CDECL streamout(FILE *stream, const char *format, va_list argptr);

int
CDECL
vfprintf(FILE *file, const char *format, va_list argptr)
{
    int result;

    _lock_file(file);
    result = streamout(file, format, argptr);
    _unlock_file(file);

    return result;
}

int
CDECL
vfprintf_s(FILE* file, const char *format, va_list argptr)
{
    int result;

    if(!MSVCRT_CHECK_PMT(format != NULL)) {
        *_errno() = EINVAL;
        return -1;
    }

    _lock_file(file);
    result = streamout(file, format, argptr);
    _unlock_file(file);

    return result;
}
