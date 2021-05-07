/*
 * PROJECT:     ReactOS command-line utilities
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Providing <conutils.h> functionality outside of ReactOS
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */
#pragma once
#include <stdio.h>
#include <locale.h>
#define ConInitStdStreams() setlocale(LC_CTYPE, "")
#define StdOut stdout
#define StdErr stderr

static inline void ConPuts(FILE *fp, LPCWSTR psz)
{
    fputws(psz, fp);
}

static inline void ConPrintf(FILE *fp, LPCWSTR psz, ...)
{
    va_list va;
    va_start(va, psz);
    vfwprintf(fp, psz, va);
    va_end(va);
}

static inline void ConResPuts(FILE *fp, UINT nID)
{
    WCHAR sz[MAX_PATH];
    LoadStringW(NULL, nID, sz, _countof(sz));
    fputws(sz, fp);
}

static inline void ConResPrintf(FILE *fp, UINT nID, ...)
{
    va_list va;
    WCHAR sz[MAX_PATH];
    va_start(va, nID);
    LoadStringW(NULL, nID, sz, _countof(sz));
    vfwprintf(fp, sz, va);
    va_end(va);
}
