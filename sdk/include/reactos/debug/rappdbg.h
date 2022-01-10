/*
 * PROJECT:     ReactOS headers
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Debugging the ReactOS applications
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#ifdef NDEBUG
#undef DBG
#endif

#ifndef _INC_WINDOWS
#include <windows.h>
#endif

#if !defined(_STRSAFE_H_INCLUDED_) && !defined(NO_STRSAFE)
#include <strsafe.h>
#endif

#include <assert.h>

#ifndef ASSERT
#define ASSERT assert
#endif

#ifndef DPRINT
# if DBG
#  ifndef APPDBG_BUFSIZE
#   define APPDBG_BUFSIZE 512
#  endif

static inline ULONG rappdbg_printfA(const char *file, int line, const char *fmt, ...)
{
    char buf[APPDBG_BUFSIZE];
    va_list va;
    const char *fname;
    size_t cch;

    va_start(va, fmt);

    fname = strrchr(file, '\\');
    if (!fname)
        fname = strrchr(file, '/');
    if (!fname)
        fname = file;
    else
        ++fname;

#  ifdef NO_STRSAFE
    cch = wsprintfA(buf, "%s:%d: ", fname, line);
    wvsprintfA(&buf[cch], fmt, va);
#  else
    StringCchPrintfA(buf, _countof(buf), "%s:%d: ", fname, line);
    StringCchLengthA(buf, _countof(buf), &cch);
    StringCchVPrintfA(&buf[cch], _countof(buf) - cch, fmt, va);
#  endif

    OutputDebugStringA(buf);
    va_end(va);

    return 0;
}

#  define DPRINT(fmt, ...)  rappdbg_printfA(__FILE__, __LINE__, __VA_ARGS__)
# else /* !DBG */
#  define DPRINT(fmt, ...)  /*empty*/
# endif /* !DBG */
#endif /* DPRINT */
