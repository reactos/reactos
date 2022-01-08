/*
 * PROJECT:     ReactOS headers
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Debugging the ReactOS applications
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#ifndef _INC_WINDOWS
    #include <windows.h>
#endif
#ifdef NO_STRSAFE
    #include <strsafe.h>
#endif

#include <assert.h>

#ifndef ASSERT
    #define ASSERT assert
#endif
#ifndef STATIC_ASSERT
    #define STATIC_ASSERT C_ASSERT
#endif

#ifndef DPRINT
    #if DBG
        #ifndef APPDBG_BUFSIZE
            #define APPDBG_BUFSIZE 512
        #endif

        static inline void
        rappdbg_printfA(const char *fmt, ...)
        {
            char buf[APPDBG_BUFSIZE];
            va_list va;
            va_start(va, fmt);
#ifdef NO_STRSAFE
            wvsprintfA(buf, fmt, va);
#else
            StringCchVPrintfA(buf, _countof(buf), fmt, va);
#endif
            OutputDebugStringA(buf);
            va_end(va);
        }

        #define DPRINT rappdbg_printfA
    #else
        #define DPRINT 1 ? 0 : (void)
    #endif
#endif

#ifndef DPRINT1
    #define DPRINT1 DPRINT
#endif
