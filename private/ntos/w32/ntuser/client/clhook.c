/****************************** Module Header ******************************\
* Module Name: clhook.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Client-side hook code.
*
* 05-09-1991 ScottLu Created.
* 08-Feb-1992 IanJa Unicode/ANSI
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* SetWindowsHookExAW
*
* Client side routine for SetWindowsHookEx(). Needs to remember the library
* name since hmods aren't global. Remembers the hmod as well so that
* it can be used to calculate pfnFilter in different process contexts.
*
* History:
* 05-15-91 ScottLu Created.
\***************************************************************************/

HHOOK SetWindowsHookExAW(
    int idHook,
    HOOKPROC lpfn,
    HINSTANCE hmod,
    DWORD dwThreadID,
    DWORD dwFlags)
{
    WCHAR pwszLibFileName[MAX_PATH];

    /*
     * If we're passing an hmod, we need to grab the file name of the
     * module while we're still on the client since module handles
     * are NOT global.
     */
    if (hmod != NULL) {
        if (GetModuleFileNameW(hmod, pwszLibFileName,
                sizeof(pwszLibFileName)/sizeof(TCHAR)) == 0) {

            /*
             * hmod is bogus - return NULL.
             */
            return NULL;
        }

#ifdef WX86
        try {
           if (RtlImageNtHeader(hmod)->FileHeader.Machine == IMAGE_FILE_MACHINE_I386) {
               dwFlags |= HF_WX86KNOWNDLL;
           }
        } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
            return NULL;
        }
#endif
    }

    return _SetWindowsHookEx(hmod,
            (hmod == NULL) ? NULL : pwszLibFileName,
            dwThreadID, idHook, (PROC)lpfn, dwFlags);
}

/***************************************************************************\
* SetWindowsHookA,
* SetWindowsHookW
*
* ANSI and Unicode wrappers for NtUserSetWindowsHookAW(). Could easily be macros
* instead, but do we want to expose NtUserSetWindowsHookAW() ?
*
* History:
* 30-Jan-1992 IanJa   Created
\***************************************************************************/
HHOOK
WINAPI
SetWindowsHookA(
    int nFilterType,
    HOOKPROC pfnFilterProc)
{
    return NtUserSetWindowsHookAW(nFilterType, pfnFilterProc, HF_ANSI);
}

HHOOK
WINAPI
SetWindowsHookW(
    int nFilterType,
    HOOKPROC pfnFilterProc)
{
    return NtUserSetWindowsHookAW(nFilterType, pfnFilterProc, 0);
}


/***************************************************************************\
* SetWindowsHookExA,
* SetWindowsHookExW
*
* ANSI and Unicode wrappers for SetWindowsHookExAW(). Could easily be macros
* instead, but do we want to expose SetWindowsHookExAW() ?
*
* History:
* 30-Jan-1992 IanJa Created
\***************************************************************************/
HHOOK WINAPI SetWindowsHookExA(
    int idHook,
    HOOKPROC lpfn,
    HINSTANCE hmod,
    DWORD dwThreadId)
{
    return SetWindowsHookExAW(idHook, lpfn, hmod, dwThreadId, HF_ANSI);
}

HHOOK WINAPI SetWindowsHookExW(
    int idHook,
    HOOKPROC lpfn,
    HINSTANCE hmod,
    DWORD dwThreadId)
{
    return SetWindowsHookExAW(idHook, lpfn, hmod, dwThreadId, 0);
}

