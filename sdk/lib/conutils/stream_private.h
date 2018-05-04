/*
 * PROJECT:     ReactOS Console Utilities Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Provides basic abstraction wrappers around CRT streams or
 *              Win32 console API I/O functions, to deal with i18n + Unicode
 *              related problems.
 * COPYRIGHT:   Copyright 2017-2018 ReactOS Team
 *              Copyright 2017-2018 Hermes Belusca-Maito
 */

#ifndef __STREAM_PRIVATE_H__
#define __STREAM_PRIVATE_H__

#pragma once

/*
 * Console I/O streams
 */

#if 0
// Shadow type, implementation-specific
typedef struct _CON_STREAM CON_STREAM, *PCON_STREAM;
#endif

typedef struct _CON_STREAM
{
    CON_WRITE_FUNC WriteFunc;

#ifdef USE_CRT
    FILE* fStream;
#else
    BOOL IsInitialized;
    CRITICAL_SECTION Lock;

    HANDLE hHandle;

    /*
     * TRUE if 'hHandle' refers to a console, in which case I/O UTF-16
     * is directly used. If 'hHandle' refers to a file or a pipe, the
     * 'Mode' flag is used.
     */
    BOOL IsConsole;

    /*
     * The 'Mode' flag is used to know the translation mode
     * when 'hHandle' refers to a file or a pipe.
     */
    CON_STREAM_MODE Mode;
    UINT CodePage;  // Used to convert UTF-16 text to some ANSI code page.
#endif /* defined(USE_CRT) */
} CON_STREAM, *PCON_STREAM;

#endif  /* __STREAM_PRIVATE_H__ */

/* EOF */
