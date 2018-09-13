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

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <msvideo.h>
#include <drawdib.h>
#include "avicap.h"
#include "avicapi.h"

static char szNull[] = "";

/*
 *
 *   GetKey
 *           Peek into the message que and get a keystroke
 *
 */
WORD GetKey(BOOL fWait)
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
void FAR _cdecl statusUpdateStatus (LPCAPSTREAM lpcs, WORD wID, ...)
{
    char ach[256];
    char szFmt[132];
    int j, k;
    BOOL fHasFormatChars = FALSE;
    
    if (lpcs-> CallbackOnStatus) {
        if (wID == NULL) {
            if (lpcs->fLastStatusWasNULL)   // No need to send NULL twice in a row
                return;
            lpcs->fLastStatusWasNULL = TRUE;
            lstrcpy (ach, szNull);
        }
        else if (!LoadString(lpcs->hInst, wID, szFmt, sizeof (szFmt))) {
            lpcs->fLastStatusWasNULL = FALSE;
            MessageBeep (0);
            return;
        }
        else {
            lpcs->fLastStatusWasNULL = FALSE;
            k = lstrlen (szFmt);
            for (j = 0; j < k; j++) {
                if (szFmt[j] == '%') {
                   fHasFormatChars = TRUE;
                   break;
                }
            }
            if (fHasFormatChars)
                wvsprintf(ach, szFmt, (LPSTR)(((WORD FAR *)&wID) + 1));
            else
                lstrcpy (ach, szFmt);
        }

        (*(lpcs->CallbackOnStatus)) (lpcs->hwnd, wID, ach);
    }
}

// wID is the string resource, which can be a format string
void FAR _cdecl errorUpdateError (LPCAPSTREAM lpcs, WORD wID, ...)
{
    char ach[256];
    char szFmt[132];
    int j, k;
    BOOL fHasFormatChars = FALSE;
    
    lpcs->dwReturn = wID;

    if (lpcs-> CallbackOnError) {
        if (wID == NULL) {
            if (lpcs->fLastErrorWasNULL)   // No need to send NULL twice in a row
                return;
            lpcs->fLastErrorWasNULL = TRUE;
            lstrcpy (ach, szNull);
        }
        else if (!LoadString(lpcs->hInst, wID, szFmt, sizeof (szFmt))) {
            MessageBeep (0);
            lpcs->fLastErrorWasNULL = FALSE;
            return;
        }
        else {
            lpcs->fLastErrorWasNULL = FALSE;
            k = lstrlen (szFmt);
            for (j = 0; j < k; j++) {
                if (szFmt[j] == '%') {
                   fHasFormatChars = TRUE;
                   break;
                }
            }
            if (fHasFormatChars)
                wvsprintf(ach, szFmt, (LPSTR)(((WORD FAR *)&wID) + 1));
            else
                lstrcpy (ach, szFmt);
        }

        (*(lpcs->CallbackOnError)) (lpcs->hwnd, wID, ach);
    }
}

// Callback client with ID of driver error msg
void errorDriverID (LPCAPSTREAM lpcs, DWORD dwError)
{
    char ach[132];
    
    lpcs->fLastErrorWasNULL = FALSE;
    lpcs->dwReturn = dwError;

    if (lpcs-> CallbackOnError) {
        if (!dwError)
            lstrcpy (ach, szNull);
        else {
            videoGetErrorText (lpcs->hVideoIn,
                        (UINT)dwError, ach, sizeof(ach));
        }
        (*(lpcs->CallbackOnError)) (lpcs->hwnd, IDS_CAP_DRIVER_ERROR, ach);
    }
}


#ifdef  _DEBUG

void FAR cdecl dprintf(LPSTR szFormat, ...)
{
    char ach[128];

    static BOOL fDebug = -1;

    if (fDebug == -1)
        fDebug = GetProfileInt("Debug", "AVICAP", FALSE);

    if (!fDebug)
        return;

    lstrcpy(ach, "AVICAP: ");
    wvsprintf(ach+8,szFormat,(LPSTR)(&szFormat+1));
    lstrcat(ach, "\r\n");

    OutputDebugString(ach);
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
#pragma optimize("", off)
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
         wsprintf(ach, "File %s, line %d", (LPSTR) szFile, iLine);
         MessageBeep(MB_ICONHAND);
	 id = MessageBox(NULL, ach, "Assertion Failed", MB_SYSTEMMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE);

         /* abort, debug, or ignore */
         switch (id)
         {

         case IDABORT:

                  /* kill this application */
                  iExitCode = 0;
#ifndef WIN32
                  _asm
                  {
                           mov      ah, 4Ch
                           mov      al, BYTE PTR iExitCode
                           int     21h
                  }
#endif // WIN16
                  break;

         case IDRETRY:

                  /* break into the debugger */
                  DebugBreak();
                  break;

         case IDIGNORE:

                  /* ignore the assertion failure */
                  break;

         }
         
         return FALSE;
}
#pragma optimize("", on)

#endif

