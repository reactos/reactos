/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/vfwprintf.c
 * PURPOSE:         Implementation of vfwprintf
 * PROGRAMMER:      Timo Kreuzer
 *                  Samuel Serapión
 */

#include <precomp.h>
#include <wchar.h>

void CDECL _lock_file(FILE* file);
void CDECL _unlock_file(FILE* file);
int CDECL wstreamout(FILE *stream, const wchar_t *format, va_list argptr);

int
CDECL
vfwprintf(FILE* file, const wchar_t *format, va_list argptr)
{
     int ret;

    _lock_file(file);
    ret = wstreamout(file, format, argptr);
    _unlock_file(file);

    return ret;
}

int
CDECL
vfwprintf_s(FILE* file, const wchar_t *format, va_list argptr)
{
    int ret;

    if(!MSVCRT_CHECK_PMT( file != NULL)) {
        *_errno() = EINVAL;
        return -1;
    }

    _lock_file(file);
    ret = wstreamout(file, format, argptr);
    _unlock_file(file);

    return ret;
}
