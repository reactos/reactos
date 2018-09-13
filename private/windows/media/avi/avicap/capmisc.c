/****************************************************************************
 *
 *   capmisc.c
 *
 *   Miscellaneous status and error routines.
 *
 *   Microsoft Video for Windows Sample Capture Class
 *
 *   Copyright (c) 1992, 1993 Microsoft Corporation.  All Rights Reserved.
 *
 *    You have a royalty-free right to use, modify, reproduce and
 *    distribute the Sample Files (and/or any modified version) in
 *    any way you find useful, provided that you agree that
 *    Microsoft has no warranty obligations or liability for any
 *    Sample Application Files which are modified.
 *
 ***************************************************************************/

#define INC_OLE2
#pragma warning(disable:4103)
#include <windows.h>
#include <windowsx.h>
#include <win32.h>
#include <mmsystem.h>
#include <msvideo.h>
#include <drawdib.h>

#include "ivideo32.h"
#include "avicap.h"
#include "avicapi.h"

#include <stdarg.h>

static TCHAR szNull[] = TEXT("");

/*
 *
 *   GetKey
 *           Peek into the message que and get a keystroke
 *
 */
UINT GetKey(BOOL fWait)
{
    MSG msg;

    msg.wParam = 0;

    if (fWait)
         GetMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST);

    while(PeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE|PM_NOYIELD))
         ;
    return msg.wParam;
}

// wID is the string resource, which can be a format string
//
void FAR CDECL statusUpdateStatus (LPCAPSTREAM lpcs, UINT wID, ...)
{
    TCHAR ach[256];
    TCHAR szFmt[132];
    va_list va;

    if (!lpcs->CallbackOnStatus)
        return;

    if (wID == 0) {
        if (lpcs->fLastStatusWasNULL)   // No need to send NULL twice in a row
            return;
        lpcs->fLastStatusWasNULL = TRUE;
        ach[0] = 0;
    }
    else {
        lpcs->fLastStatusWasNULL = FALSE;
        if (!LoadString(lpcs->hInst, wID, szFmt, NUMELMS(szFmt))) {
            MessageBeep (0);
            return;
        }
        else {
            va_start(va, wID);
            wvsprintf(ach, szFmt, va);
            va_end(va);
        }
    }

   #ifdef UNICODE
    //
    // if the status callback function is expecting ansi
    // strings, then convert the UNICODE status string to
    // ansi before calling him
    //
    if (lpcs->fUnicode & VUNICODE_STATUSISANSI) {

        char achAnsi[256];

        // convert string to Ansi and callback.
        // that we cast achAnsi to WChar on the call to
        // avoid a bogus warning.
        //
        WideToAnsi(achAnsi, ach, lstrlen(ach)+1);
        lpcs->CallbackOnStatus(lpcs->hwnd, wID, (LPWSTR)achAnsi);
    }
    else
   #endif
       lpcs->CallbackOnStatus(lpcs->hwnd, wID, ach);
}

// wID is the string resource, which can be a format string
//
void FAR CDECL errorUpdateError (LPCAPSTREAM lpcs, UINT wID, ...)
{
    TCHAR ach[256];
    TCHAR szFmt[132];
    va_list va;

    lpcs->dwReturn = wID;

    if (!lpcs->CallbackOnError)
        return;

    if (wID == 0) {
        if (lpcs->fLastErrorWasNULL)   // No need to send NULL twice in a row
            return;
        lpcs->fLastErrorWasNULL = TRUE;
        ach[0] = 0;
    }
    else if (!LoadString(lpcs->hInst, wID, szFmt, NUMELMS(szFmt))) {
        MessageBeep (0);
        lpcs->fLastErrorWasNULL = FALSE;
        return;
    }
    else {
        lpcs->fLastErrorWasNULL = FALSE;
        va_start(va, wID);
        wvsprintf(ach, szFmt, va);
        va_end(va);
    }

   #ifdef UNICODE
    if (lpcs->fUnicode & VUNICODE_ERRORISANSI)
    {
        char achAnsi[256];

        // convert string to Ansi and callback.
        // that we cast achAnsi to WChar on the call to
        // avoid a bogus warning.
        //
        WideToAnsi(achAnsi, ach, lstrlen(ach)+1);
        lpcs->CallbackOnError(lpcs->hwnd, wID, (LPWSTR)achAnsi);
    }
    else
   #endif
    {
        lpcs->CallbackOnError(lpcs->hwnd, wID, ach);
    }
}

// Callback client with ID of driver error msg
void errorDriverID (LPCAPSTREAM lpcs, DWORD dwError)
{
    // this is the correct code, but NT VfW 1.0 has a bug
    // that videoGetErrorText is ansi. need vfw1.1 to fix this

#ifndef UNICODE
    char ach[132];
#endif

    lpcs->fLastErrorWasNULL = FALSE;
    lpcs->dwReturn = dwError;

    if (!lpcs->CallbackOnError)
        return;


   #ifdef UNICODE
    if (lpcs->fUnicode & VUNICODE_ERRORISANSI) {
        char achAnsi[256];
	achAnsi[0]=0;
        if (dwError)
            videoGetErrorTextA(lpcs->hVideoIn, dwError, achAnsi, NUMELMS(achAnsi));
        lpcs->CallbackOnError (lpcs->hwnd, IDS_CAP_DRIVER_ERROR, (LPWSTR)achAnsi);
    } else {
	// pass unicode string to error handler
        WCHAR achWide[256];
	achWide[0]=0;
        if (dwError)
            videoGetErrorTextW(lpcs->hVideoIn, dwError, achWide, NUMELMS(achWide));
        lpcs->CallbackOnError (lpcs->hwnd, IDS_CAP_DRIVER_ERROR, (LPWSTR)achWide);
    }
   #else  // not unicode
    ach[0] = 0;
    if (dwError)
        videoGetErrorText (lpcs->hVideoIn, dwError, ach, NUMELMS(ach));
        lpcs->CallbackOnError(lpcs->hwnd, IDS_CAP_DRIVER_ERROR, ach);
   #endif
}

#ifdef  _DEBUG

void FAR cdecl dprintf(LPSTR szFormat, ...)
{
    UINT n;
    char ach[256];
    va_list va;

    static BOOL fDebug = -1;

    if (fDebug == -1)
        fDebug = GetProfileIntA("Debug", "AVICAP32", FALSE);

    if (!fDebug)
        return;

#ifdef _WIN32
    n = wsprintfA(ach, "AVICAP32: (tid %x) ", GetCurrentThreadId());
#else
    strcpy(ach, "AVICAP32: ");
    n = strlen(ach);
#endif

    va_start(va, szFormat);
    wvsprintfA(ach+n, szFormat, va);
    va_end(va);

    lstrcatA(ach, "\r\n");

    OutputDebugStringA(ach);
}

/* _Assert(fExpr, szFile, iLine)
 *
 * If <fExpr> is TRUE, then do nothing.  If <fExpr> is FALSE, then display
 * an "assertion failed" message box allowing the user to abort the program,
 * enter the debugger (the "Retry" button), or igore the error.
 *
 * <szFile> is the name of the source file; <iLine> is the line number
 * containing the _Assert() call.
 */

BOOL FAR PASCAL
_Assert(BOOL fExpr, LPSTR szFile, int iLine)
{
    static char       ach[300];         // debug output (avoid stack overflow)
    int               id;
    int               iExitCode;
    void FAR PASCAL DebugBreak(void);

    /* check if assertion failed */
    if (fExpr)
             return fExpr;

    /* display error message */
    wsprintfA(ach, "File %s, line %d", (LPSTR) szFile, iLine);
    MessageBeep(MB_ICONHAND);
    id = MessageBoxA (NULL, ach, "Assertion Failed",
                      MB_SYSTEMMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE);

    /* abort, debug, or ignore */
    switch (id)
    {
    case IDABORT: /* kill this application */
        iExitCode = 0;
        ExitProcess(0);
        break;

    case IDRETRY: /* break into the debugger */
        DebugBreak();
        break;

    case IDIGNORE:
        /* ignore the assertion failure */
        break;
    }

    return FALSE;
}

#endif
