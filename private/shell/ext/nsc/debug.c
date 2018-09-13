#include <windows.h>

#include "debug.h"

// #define DEBUG_BREAK        __try { _asm { int 3 } } __except (EXCEPTION_EXECUTE_HANDLER) {;}
#define DEBUG_BREAK        _try { DebugBreak(); } _except (EXCEPTION_EXECUTE_HANDLER) {;}

#ifdef _DEBUG

UINT g_fDebugMask = 0x00ff;

UINT SetDebugMask(UINT mask)
{
    UINT wOld = g_fDebugMask;
    g_fDebugMask = mask;

    return wOld;
}

UINT GetDebugMask()
{
    return g_fDebugMask;
}

void AssertFailed(LPCSTR pszFile, int line)
{
    LPCSTR psz;
    char ach[256];
    static char szAssertFailed[] = "Assertion failed in %s on line %d\r\n";

    // Strip off path info from filename string, if present.
    //
    if (g_fDebugMask & DM_ASSERT)
    {
        for (psz = pszFile + lstrlen(pszFile); psz != pszFile; psz=AnsiPrev(pszFile, psz))
        {
            if ((AnsiPrev(pszFile, psz)!= (psz-2)) && *(psz - 1) == '\\')
                break;
        }
        wsprintf(ach, szAssertFailed, psz, line);
        OutputDebugString(ach);
	
	DEBUG_BREAK
    }
}

void _cdecl _AssertMsg(BOOL f, LPCSTR pszMsg, ...)
{
    char ach[256];

    if (!f && (g_fDebugMask & DM_ASSERT))
    {
        wvsprintf(ach, pszMsg, (LPVOID)(&pszMsg + 1));
        lstrcat(ach, "\r\n");
        OutputDebugString(ach);
	DEBUG_BREAK
    }
}

void _cdecl _DebugMsg(UINT mask, LPCSTR pszMsg, ...)
{
    char ach[2*MAX_PATH+40];  // Handles 2*largest path + slop for message

    if (g_fDebugMask & mask)
    {
        wvsprintf(ach, pszMsg, (LPVOID)(&pszMsg + 1));
        lstrcat(ach, "\r\n");
        OutputDebugString(ach);
    }
}

#endif // _DEBUG
