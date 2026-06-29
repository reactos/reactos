
#pragma once

#include <stdio.h>
#define ConInitStdStreams() /* empty */
#define StdOut stdout
#define StdErr stderr
void ConPuts(FILE *fp, LPCWSTR psz)
{
    fputws(psz, fp);
}
void ConPrintf(FILE *fp, LPCWSTR psz, ...)
{
    va_list va;
    va_start(va, psz);
    vfwprintf(fp, psz, va);
    va_end(va);
}
void ConResPuts(FILE *fp, UINT nID)
{
    WCHAR sz[MAX_PATH];
    LoadStringW(NULL, nID, sz, _countof(sz));
    fputws(sz, fp);
}
void ConResPrintf(FILE *fp, UINT nID, ...)
{
    va_list va;
    WCHAR sz[MAX_PATH];
    va_start(va, nID);
    LoadStringW(NULL, nID, sz, _countof(sz));
    vfwprintf(fp, sz, va);
    va_end(va);
}
