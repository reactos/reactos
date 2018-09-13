#include "shole.h"

#ifdef WIN32
#define DEBUG_BREAK        _try { DebugBreak(); } except (EXCEPTION_EXECUTE_HANDLER) {;}
#else
#define DEBUG_BREAK        _asm { int 3 }
#endif


#ifdef DEBUG

//========== Debug output routines =========================================

UINT wDebugMask = 0x00ff;

UINT WINAPI SetDebugMask(UINT mask)
{
    UINT wOld = wDebugMask;
    wDebugMask = mask;

    return wOld;
}

UINT WINAPI GetDebugMask()
{
    return wDebugMask;
}

void WINAPI AssertFailed(LPCTSTR pszFile, int line)
{
    LPCTSTR psz;
    TCHAR ach[256];
    static TCHAR szAssertFailed[] = TEXT("Assertion failed in %s on line %d\r\n");

    // Strip off path info from filename string, if present.
    //
    if (wDebugMask & DM_ASSERT)
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

#define WINCAPI __cdecl
void WINCAPI _AssertMsg(BOOL f, LPCTSTR pszMsg, ...)
{
    TCHAR ach[256];

    if (!f && (wDebugMask & DM_ASSERT))
    {
#ifdef WINNT
        va_list ArgList;

        va_start(ArgList, pszMsg);
        wvsprintf(ach, pszMsg, ArgList);
        va_end(ArgList);
#else
        wvsprintf(ach, pszMsg, (LPVOID)(&pszMsg + 1));
#endif
        lstrcat(ach, TEXT("\r\n"));
        OutputDebugString(ach);
        DEBUG_BREAK
    }
}

void WINCAPI _DebugMsg(UINT mask, LPCTSTR pszMsg, ...)
{
    TCHAR ach[2*MAX_PATH+40];  // Handles 2*largest path + slop for message

    if (wDebugMask & mask)
    {
#ifdef WINNT
        va_list ArgList;

        va_start(ArgList, pszMsg);
        try {
            wvsprintf(ach, pszMsg, ArgList);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            OutputDebugString(TEXT("SHELL32: DebugMsg exception: "));
            OutputDebugString(pszMsg);
        }
        va_end(ArgList);
        OutputDebugString(TEXT("SHELL32: "));
#else
        wvsprintf(ach, pszMsg, (LPVOID)(&pszMsg + 1));
#endif
        lstrcat(ach, TEXT("\r\n"));
        OutputDebugString(ach);
    }
}

#endif // DEBUG
