/////////////////////////////////////////////////////////////////////////////
// Diagnostic Trace
//
#include <stdio.h> 
#include <stdarg.h>
#include <windows.h>
//#include <tchar.h>
#include "trace.h"

#ifdef _DEBUG

#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

void _DebugBreak(void)
{
    DebugBreak();
}

//void Trace(TCHAR* lpszFormat, ...)
void Trace(char* lpszFormat, ...)
{
    va_list args;
    int nBuf;
    char szBuffer[512];

    va_start(args, lpszFormat);
    nBuf = _vsnprintf(szBuffer, sizeof(szBuffer)/sizeof(TCHAR), lpszFormat, args);
    OutputDebugStringA(szBuffer);
    // was there an error? was the expanded string too long?
    //ASSERT(nBuf >= 0);
    va_end(args);
}

void Assert(void* assert, const char* file, int line, void* msg)
{
    if (msg == NULL) {
        printf("ASSERT -- %s occured on line %u of file %s.\n",
               assert, line, file);
    } else {
        printf("ASSERT -- %s occured on line %u of file %s: Message = %s.\n",
               assert, line, file, msg);
    }
}

#else

//void Trace(TCHAR* lpszFormat, ...) { };
void Trace(char* lpszFormat, ...) { };
void Assert(void* assert, const char* file, int line, void* msg) { };

#endif //_DEBUG
/////////////////////////////////////////////////////////////////////////////
