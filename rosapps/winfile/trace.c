/////////////////////////////////////////////////////////////////////////////
// Diagnostic Trace
//
#include <stdio.h> 
#include <stdarg.h>
#include "trace.h"
//#include "windows.h"

DeclAssertFile;  // Should be added at the begining of each .C/.CPP


#ifdef _DEBUG

#ifdef WIN32
//#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
//#include <windows.h>
//#include <assert.h>
//WINBASEAPI VOID WINAPI DebugBreak(VOID);
//WINBASEAPI VOID WINAPI OutputDebugStringA(LPCSTR lpOutputString);
//WINBASEAPI VOID WINAPI OutputDebugStringW(LPCWSTR lpOutputString);
void __stdcall DebugBreak(void);
void __stdcall OutputDebugStringA(char* lpOutputString);
void __stdcall OutputDebugStringW(wchar_t* lpOutputString);
#ifdef UNICODE
#define OutputDebugString  OutputDebugStringW
#else
#define OutputDebugString  OutputDebugStringA
#endif // !UNICODE

#else
#include "hardware.h"
#endif // WIN32


#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

void _DebugBreak(void)
{
    DebugBreak();
}

void Trace(char* lpszFormat, ...)
{
    va_list args;
    int nBuf;
    char szBuffer[512];

    va_start(args, lpszFormat);
//  nBuf = vsprintf(szBuffer, lpszFormat, args);
//  nBuf = _vsntprintf(szBuffer, _countof(szBuffer), lpszFormat, args);
    nBuf = _vsnprintf(szBuffer, sizeof(szBuffer), lpszFormat, args);
    OutputDebugString(szBuffer);
    // was there an error? was the expanded string too long?
    ASSERT(nBuf >= 0);
    va_end(args);
}

void Assert(void* assert, char* file, int line, void* msg)
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

//inline void Trace(char* lpszFormat, ...) { };
//inline void Assert(void* assert, char* file, int line, void* msg) { };
void Trace(char* lpszFormat, ...) { };
void Assert(void* assert, char* file, int line, void* msg) { };

#endif //_DEBUG
/////////////////////////////////////////////////////////////////////////////
