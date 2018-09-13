#define NOSHELLDEBUG	// don't take shell versions of this

#include <windows.h>

#include "debug.h"

#ifdef DEBUG

#define DEBUG_BREAK        try { _asm { int 3 } } except (EXCEPTION_EXECUTE_HANDLER) {;}

UINT g_fDebugMask = 0x00ff;

UINT WINAPI SetDebugMask(UINT mask)
{
    UINT wOld = g_fDebugMask;
    g_fDebugMask = mask;

    return wOld;
}

UINT WINAPI GetDebugMask()
{
    return g_fDebugMask;
}

void WINAPI AssertFailed(LPCTSTR pszFile, int line)
{
    LPCTSTR psz;
    TCHAR ach[256];
    static TCHAR szAssertFailed[] = TEXT("Assertion failed in %s on line %d\r\n");

    // Strip off path info from filename string, if present.
    //
    if (g_fDebugMask & DM_ASSERT)
    {
        for (psz = pszFile + lstrlen(pszFile); psz != pszFile; psz=CharPrev(pszFile, psz))
        {
            if ((CharPrev(pszFile, psz)!= (psz-2)) && *(psz - 1) == TEXT('\\'))
                break;
        }
        wsprintf(ach, szAssertFailed, psz, line);
        OutputDebugString(ach);
	
	DEBUG_BREAK
    }
}

void _cdecl _AssertMsg(BOOL f, LPCTSTR pszMsg, ...)
{
    TCHAR ach[256];

    if (!f && (g_fDebugMask & DM_ASSERT))
    {
        wvsprintf(ach, pszMsg, (LPVOID)(&pszMsg + 1));
        lstrcat(ach, TEXT("\r\n"));
        OutputDebugString(ach);
	DEBUG_BREAK
    }
}

void _cdecl _DebugMsg(UINT mask, LPCTSTR pszMsg, ...)
{
    TCHAR ach[2*MAX_PATH+40];  // Handles 2*largest path + slop for message

    if (g_fDebugMask & mask)
    {
        wvsprintf(ach, pszMsg, (LPVOID)(&pszMsg + 1));
        lstrcat(ach, TEXT("\r\n"));
        OutputDebugString(ach);
    }
}

#endif // DEBUG

