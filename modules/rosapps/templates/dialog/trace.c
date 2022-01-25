/////////////////////////////////////////////////////////////////////////////
// Diagnostic Trace
//
#include <stdio.h>
#include <stdarg.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include "trace.h"


#ifdef _DEBUG

void _DebugBreak(void)
{
    DebugBreak();
}

void Trace(TCHAR* lpszFormat, ...)
{
    va_list args;
    int nBuf;
    TCHAR szBuffer[512];

    va_start(args, lpszFormat);
    nBuf = _vsntprintf(szBuffer, sizeof(szBuffer)/sizeof(TCHAR), lpszFormat, args);
    OutputDebugString(szBuffer);
    // was there an error? was the expanded string too long?
    //ASSERT(nBuf >= 0);
    va_end(args);
}

void Assert(void* assert_, TCHAR* file, int line, void* msg)
{
    if (msg == NULL) {
        printf("ASSERT -- %s occured on line %u of file %s.\n",
               (char *)assert_, line, file);
    } else {
        printf("ASSERT -- %s occured on line %u of file %s: Message = %s.\n",
               (char *)assert_, line, file, (char *)msg);
    }
}

#else

void Trace(TCHAR* lpszFormat, ...) { };
void Assert(void* assert, TCHAR* file, int line, void* msg) { };

#endif //_DEBUG
/////////////////////////////////////////////////////////////////////////////
