/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WUMAN.C
 *  WOW32 16-bit User API support (manually-coded thunks)
 *
 *  History:
 *  Created 27-Jan-1991 by Jeff Parsons (jeffpar)
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wuman.c);

WBP W32WordBreakProc = NULL;

extern DWORD fThunkStrRtns;

extern WORD gwKrnl386CodeSeg1;
extern WORD gwKrnl386CodeSeg2;
extern WORD gwKrnl386CodeSeg3;
extern WORD gwKrnl386DataSeg1;

ULONG FASTCALL WU32ExitWindows(PVDMFRAME pFrame)
// BUGBUG mattfe 4-mar-92, this routine should not return if we close down
// all the apps successfully.
{
    ULONG ul;
    register PEXITWINDOWS16 parg16;

    GETARGPTR(pFrame, sizeof(EXITWINDOWS16), parg16);

    ul = GETBOOL16(ExitWindows(
    DWORD32(parg16->dwReserved),
    WORD32(parg16->wReturnCode)
    ));

    FREEARGPTR(parg16);

    RETURN(ul);
}

WORD gUser16CS = 0;

ULONG FASTCALL WU32NotifyWow(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    register PNOTIFYWOW16 parg16;

    GETARGPTR(pFrame, sizeof(NOTIFYWOW16), parg16);

    switch (FETCHWORD(parg16->Id)) {
        case NW_LOADACCELERATORS:
            ul = WU32LoadAccelerators(FETCHDWORD(parg16->pData));
            break;

        case NW_LOADICON:
        case NW_LOADCURSOR:
            ul = (ULONG) W32CheckIfAlreadyLoaded(parg16->pData, FETCHWORD(parg16->Id));
            break;

        case NW_WINHELP:
            {
                // this call is made from IWinHelp in USER.exe to find the
                // '16bit' help window if it exists.
                //

                LPSZ lpszClass;
                GETMISCPTR(parg16->pData, lpszClass);
                ul = (ULONG)(pfnOut.pfnWOWFindWindow)((LPCSTR)lpszClass, (LPCSTR)NULL);
                if (ul) {
                    // check if hwndWinHelp belongs to this process or not.
                    DWORD pid, pidT;
                    pid = pidT = GetCurrentProcessId();
                    GetWindowThreadProcessId((HWND)ul, &pid);
                    ul = (ULONG)MAKELONG((WORD)GETHWND16(ul),(WORD)(pid == pidT));
                }
                FREEMISCPTR(lpszClass);
            }
            break;

        case NW_KRNL386SEGS:
            {
                PKRNL386SEGS pKrnl386Segs;
                
                GETVDMPTR(parg16->pData, sizeof(KRNL386SEGS), pKrnl386Segs);

                gwKrnl386CodeSeg1 = pKrnl386Segs->CodeSeg1;
                gwKrnl386CodeSeg2 = pKrnl386Segs->CodeSeg2;
                gwKrnl386CodeSeg3 = pKrnl386Segs->CodeSeg3;
                gwKrnl386DataSeg1 = pKrnl386Segs->DataSeg1;
            }
            break;

        case NW_FINALUSERINIT:
            {
                static BYTE CallCsrFlag = 0;
                extern DWORD   gpsi;
                PUSERCLIENTGLOBALS pfinit16;
                WORD UNALIGNED *pwMaxDWPMsg;
                PBYTE pDWPBits;
#ifdef DEBUG
                WORD wMsg;
                int i;
                PSZ pszFormat;
#endif

                GETVDMPTR(parg16->pData, sizeof(USERCLIENTGLOBALS), pfinit16);
                GETVDMPTR(pfinit16->lpwMaxDWPMsg, sizeof(WORD), pwMaxDWPMsg);
                GETVDMPTR(pfinit16->lpDWPBits, pfinit16->cbDWPBits, pDWPBits);

                // store the 16bit hmod of user.exe
                gUser16hInstance = (WORD)pfinit16->hInstance;
                WOW32ASSERTMSGF((gUser16hInstance),
                                ("WOW Error gUser16hInstance == NULL!\n"));

                // store the 16bit CS of user.exe
                gUser16CS = HIWORD(pFrame->vpCSIP);

                // initialize user16client globals

                if (pfinit16->lpgpsi) {
                    BYTE **lpT;
                    GETVDMPTR(pfinit16->lpgpsi, sizeof(DWORD), lpT);
                    *lpT = (BYTE *)gpsi;
                    FLUSHVDMCODEPTR((ULONG)pfinit16->lpgpsi, sizeof(DWORD), lpT);
                    FREEVDMPTR(lpT);
                }


                if (pfinit16->lpCsrFlag) {
                    BYTE **lpT;
                    GETVDMPTR(pfinit16->lpCsrFlag, sizeof(DWORD), lpT);
                    *lpT = (LPSTR)&CallCsrFlag;
                    FLUSHVDMCODEPTR((ULONG)pfinit16->lpCsrFlag, sizeof(DWORD), lpT);
                    FREEVDMPTR(lpT);
                }

                if (pfinit16->lpHighestAddress) {
                    DWORD *lpT;
                    SYSTEM_BASIC_INFORMATION sbi;
                    NTSTATUS Status;

                    GETVDMPTR(pfinit16->lpHighestAddress, sizeof(DWORD), lpT);
                    Status = NtQuerySystemInformation(SystemBasicInformation,
                                                      &sbi,
                                                      sizeof(sbi),
                                                      NULL);

                    WOW32ASSERTMSGF((NT_SUCCESS(Status)),
                                ("WOW Error NtQuerySystemInformation failed!\n"));

                    *lpT = sbi.MaximumUserModeAddress;
                    FLUSHVDMCODEPTR((ULONG)pfinit16->lpHighestAddress, sizeof(DWORD), lpT);
                    FREEVDMPTR(lpT);
                }


                /* No longer required now that user32 & user.exe are separate
     DEAD CODE  if (HIWORD(pfinit16->dwBldInfo) != HIWORD(pfnOut.dwBldInfo)) {
     DEAD CODE      MessageBeep(0);
     DEAD CODE      MessageBoxA(NULL, "user.exe and user32.dll are mismatched.",
     DEAD CODE                  "WOW Error", MB_OK | MB_ICONEXCLAMATION);
     DEAD CODE  }
                */

                *pwMaxDWPMsg = (pfnOut.pfnWowGetDefWindowProcBits)(pDWPBits, pfinit16->cbDWPBits);

                FLUSHVDMCODEPTR(pfinit16->lpwMaxDWPMsg, sizeof(WORD), pwMaxDWPMsg);
                FLUSHVDMCODEPTR(pfinit16->lpDWPBits, pfinit16->cbDWPBits, pDWPBits);

#ifdef DEBUG
                LOGDEBUG(LOG_TRACE, ("WU32NotifyWow: got DefWindowProc bits, wMaxDWPMsg = 0x%x.\n", *pwMaxDWPMsg));
                LOGDEBUG(LOG_TRACE, ("The following messages will be passed on to 32-bit DefWindowProc:\n"));

#define FPASSTODWP32(msg) \
    (pDWPBits[msg >> 3] & (1 << (msg & 7)))

                wMsg = 0;
                i = 0;

                while (wMsg <= *pwMaxDWPMsg) {
                    if (FPASSTODWP32(wMsg)) {
                        if ( i & 3 ) {
                            pszFormat = ", %s";
                        } else {
                            pszFormat = "\n%s";
                        }
                        LOGDEBUG(LOG_TRACE, (pszFormat, aw32Msg[wMsg].lpszW32));
                        i++;
                    }
                    wMsg++;
                }

                LOGDEBUG(LOG_TRACE, ("\n\n"));
#endif

                gpfn16GetProcModule = pfinit16->pfnGetProcModule;

                //
                // Return value tells User16 whether to thunk
                // string routines to Win32 or use the fast
                // US-only versions.  TRUE means thunk.
                //
                // If the locale is U.S. English, we default to
                // not thunking, outside the U.S. we default to
                // thunking.  See wow32.c's use of fThunkStrRtns.
                //
                // We engage in this nastiness because the Winstone 94
                // Access 1.1 test takes *twice* as long to run in
                // the US if we thunk lstrcmp and lstrcmpi to Win32.
                //
                // By adding a value "ThunkNLS" to the WOW registry
                // key of type REG_DWORD, the user can force thunking
                // to Win32 (value 1) or use the fast US-only ones (value 0).
                //

                ul = fThunkStrRtns;

                FREEVDMPTR(pDWPBits);
                FREEVDMPTR(pwMaxDWPMsg);
                FREEVDMPTR(pfinit16);

            }
            break;

        default:
            ul = 0;
            break;
    }

    FREEARGPTR(parg16);
    return ul;
}


ULONG FASTCALL WU32WOWWordBreakProc(PVDMFRAME pFrame)
{
    PSZ         psz1;
    ULONG ul;
    register PWOWWORDBREAKPROC16 parg16;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);
    GETPSZPTR(parg16->lpszEditText, psz1);

    ul = (*W32WordBreakProc)(psz1, parg16->ichCurrentWord, parg16->cbEditText,
                             parg16->action);

    FREEPSZPTR(psz1);
    FREEARGPTR(parg16);
    RETURN(ul);
}

//
// WU32MouseEvent:  Thunk for 16-bit register-based API mouse_event,
//                  with the help of user16 function mouse_event (in
//                  winmisc2.asm).
//

ULONG FASTCALL WU32MouseEvent(PVDMFRAME pFrame)
{
    ULONG ul;
    register PMOUSEEVENT16 parg16;
    typedef ULONG (WINAPI *PFMOUSE_EVENT)(DWORD, DWORD, DWORD, DWORD, DWORD);

    GETARGPTR(pFrame, sizeof(PMOUSEEVENT16), parg16);

    //
    // mouse_event is declared void, but we'll return the same value as
    // user32.
    //

    ul = ((PFMOUSE_EVENT)(PVOID)mouse_event)(
             parg16->wFlags,
             parg16->dx,
             parg16->dy,
             parg16->cButtons,
             parg16->dwExtraInfo
             );

    FREEARGPTR(parg16);
    RETURN(ul);
}



//
// WU32KeybdEvent:  Thunk for 16-bit register-based API keybd_event,
//                  with the help of user16 function keybd_event (in
//                  winmisc2.asm).
//

ULONG FASTCALL WU32KeybdEvent(PVDMFRAME pFrame)
{
    ULONG ul;
    register PKEYBDEVENT16 parg16;
    typedef ULONG (WINAPI *PFKEYBD_EVENT)(BYTE, BYTE, DWORD, DWORD);

    GETARGPTR(pFrame, sizeof(PKEYBDEVENT16), parg16);

    //
    // keybd_event is declared void, but we'll return the same value as
    // user32.
    //

    ul = ((PFKEYBD_EVENT)(PVOID)keybd_event)(
             LOBYTE(parg16->bVirtualKey),
             LOBYTE(parg16->bScanCode),
             ((HIBYTE(parg16->bVirtualKey) == 0x80) ? KEYEVENTF_KEYUP : 0) |
             ((HIBYTE(parg16->bScanCode) == 0x1) ? KEYEVENTF_EXTENDEDKEY : 0),
             parg16->dwExtraInfo
             );

    FREEARGPTR(parg16);
    RETURN(ul);
}
