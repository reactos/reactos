//*******************************************************************************************
//
// Filename : debug.c
//	
//				Debug routines
//
// Copyright (c) 1994 - 1996 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************

#include "pch.h"

#include "cvdebug.h"

#ifdef _X86_
// Use int 3 so we stop immediately in the source
#define DEBUG_BREAK        __try { _asm { int 3 } } __except (EXCEPTION_EXECUTE_HANDLER) {;}
#else
#define DEBUG_BREAK        __try { DebugBreak(); } __except (EXCEPTION_EXECUTE_HANDLER) {;}
#endif

#ifdef DEBUG

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

void WINAPI AssertFailed(LPCSTR pszFile, int line)
{
    LPCSTR psz;
    CHAR ach[256];
    static CHAR szAssertFailed[] = "Assertion failed in %s on line %d\r\n";

    // Strip off path info from filename string, if present.
    //
    if (g_fDebugMask & DM_ASSERT)
    {
        for (psz = pszFile + lstrlenA(pszFile); psz != pszFile; psz=CharPrevA(pszFile, psz))
        {
            if ((CharPrevA(pszFile, psz)!= (psz-2)) && *(psz - 1) == '\\')
                break;
        }
        wsprintfA(ach, szAssertFailed, psz, line);
        OutputDebugStringA(ach);
	
	DEBUG_BREAK
    }
}

void _cdecl _AssertMsg(BOOL f, LPCSTR pszMsg, ...)
{
    CHAR ach[256];

    if (!f && (g_fDebugMask & DM_ASSERT))
    {
        va_list ArgList;
        va_start(ArgList, pszMsg);
        wvsprintfA(ach, pszMsg, ArgList);
        lstrcatA(ach, "\r\n");
        OutputDebugStringA(ach);
        va_end(ArgList);
    	DEBUG_BREAK
    }
}

void _cdecl _DebugMsg(UINT mask, LPCSTR pszMsg, ...)
{
    CHAR ach[2*MAX_PATH+40];  // Handles 2*largest path + slop for message

    if (g_fDebugMask & mask)
    {
        va_list ArgList;
        va_start(ArgList, pszMsg);
        wvsprintfA(ach, pszMsg, ArgList);
        lstrcatA(ach, "\r\n");
        OutputDebugStringA(ach);
        va_end(ArgList);
    }
}

#endif // DEBUG

#define SZ_DEBUGINI         "ccshell.ini"
#define SZ_DEBUGSECTION     "cabview"
#define SZ_MODULE           "CABVIEW"
#define DECLARE_DEBUG
#include <ccstock.h>
#include <debug.h>

