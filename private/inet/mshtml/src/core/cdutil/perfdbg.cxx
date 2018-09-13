//+------------------------------------------------------------------------
//
//  File:       perfdbg.cxx
//
//  Contents:   PerfDbgLogFn
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#if DBG==1

int __cdecl PerfDbgLogFn(int tag, void * pvObj, char * pchFmt, ...)
{
    static char ach[1024];
    static CCriticalSection s_cs;

    if (IsPerfDbgEnabled(tag))
    {
        LOCK_SECTION(s_cs);

        va_list vl;
        va_start(vl, pchFmt);
        ach[0] = 0;
        wsprintfA(ach, "[%lX] %8lX ", GetCurrentThreadId(), pvObj);
#ifndef WIN16
        wvsprintfA(ach + lstrlenA(ach), pchFmt, vl);
#else
        wvsprintf(ach + lstrlen(ach), pchFmt, vl);
#endif
        TraceTag((tag, "%s", ach));
        va_end(vl);
    }

    return 0;
}

#endif
