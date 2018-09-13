/*++

Copyright (c) 1990-1995,  Microsoft Corporation  All rights reserved.

Module Name:

    init.c

Abstract:

    This module contains the init routines for the Win32 common dialogs.

Revision History:

--*/



//
//  Include Files.
//

#include <windows.h>
#include <shlobj.h>
#include "privcomd.h"




//
//  External Declarations.
//

extern HDC hdcMemory;
extern HBITMAP hbmpOrigMemBmp;

extern CRITICAL_SECTION g_csLocal;
extern CRITICAL_SECTION g_csNetThread;
extern CRITICAL_SECTION g_csExtError;

extern DWORD g_tlsiCurDir;
extern DWORD g_tlsiCurThread;
extern DWORD g_tlsiExtError;

extern HANDLE hMPR;
extern HANDLE hMPRUI;
extern HANDLE hLNDEvent;

extern DWORD dwNumDisks;
extern OFN_DISKINFO gaDiskInfo[MAX_DISKS];

extern DWORD cbNetEnumBuf;
extern LPTSTR gpcNetEnumBuf;




//
//  Global Variables.
//

WCHAR szmsgLBCHANGEW[]          = LBSELCHSTRINGW;
WCHAR szmsgSHAREVIOLATIONW[]    = SHAREVISTRINGW;
WCHAR szmsgFILEOKW[]            = FILEOKSTRINGW;
WCHAR szmsgCOLOROKW[]           = COLOROKSTRINGW;
WCHAR szmsgSETRGBW[]            = SETRGBSTRINGW;
WCHAR szCommdlgHelpW[]          = HELPMSGSTRINGW;

TCHAR szShellIDList[]           = CFSTR_SHELLIDLIST;

//
//  Private message for WOW to indicate 32-bit logfont
//  needs to be thunked back to 16-bit log font.
//
CHAR szmsgWOWLFCHANGE[]         = "WOWLFChange";

//
//  Private message for WOW to indicate 32-bit directory needs to be
//  thunked back to 16-bit task directory.
//
CHAR szmsgWOWDIRCHANGE[]        = "WOWDirChange";
CHAR szmsgWOWCHOOSEFONT_GETLOGFONT[]  = "WOWCHOOSEFONT_GETLOGFONT";

CHAR szmsgLBCHANGEA[]           = LBSELCHSTRINGA;
CHAR szmsgSHAREVIOLATIONA[]     = SHAREVISTRINGA;
CHAR szmsgFILEOKA[]             = FILEOKSTRINGA;
CHAR szmsgCOLOROKA[]            = COLOROKSTRINGA;
CHAR szmsgSETRGBA[]             = SETRGBSTRINGA;
CHAR szCommdlgHelpA[]           = HELPMSGSTRINGA;

UINT g_cfCIDA;





////////////////////////////////////////////////////////////////////////////
//
//  FInitColor
//
////////////////////////////////////////////////////////////////////////////

extern DWORD rgbClient;
extern HBITMAP hRainbowBitmap;

INT FInitColor(
    HANDLE hInst)
{
    cyCaption = (short)GetSystemMetrics(SM_CYCAPTION);
    cyBorder = (short)GetSystemMetrics(SM_CYBORDER);
    cxBorder = (short)GetSystemMetrics(SM_CXBORDER);
    cyVScroll = (short)GetSystemMetrics(SM_CYVSCROLL);
    cxVScroll = (short)GetSystemMetrics(SM_CXVSCROLL);
    cxSize = (short)GetSystemMetrics(SM_CXSIZE);

    rgbClient = GetSysColor(COLOR_3DFACE);

    hRainbowBitmap = 0;

    return (TRUE);
    hInst;
}


////////////////////////////////////////////////////////////////////////////
//
//  FInitFile
//
////////////////////////////////////////////////////////////////////////////

BOOL FInitFile(
    HANDLE hins)
{
    bMouse = GetSystemMetrics(SM_MOUSEPRESENT);

    wWinVer = 0x0A0A;

    //
    //  Initialize these to reality.
    //
#if DPMICDROMCHECK
    wCDROMIndex = InitCDROMIndex((LPWORD)&wNumCDROMDrives);
#endif

    //
    // special WOW messages
    //
    msgWOWLFCHANGE       = RegisterWindowMessageA((LPSTR)szmsgWOWLFCHANGE);
    msgWOWDIRCHANGE      = RegisterWindowMessageA((LPSTR)szmsgWOWDIRCHANGE);
    msgWOWCHOOSEFONT_GETLOGFONT = RegisterWindowMessageA((LPSTR)szmsgWOWCHOOSEFONT_GETLOGFONT);

    msgLBCHANGEA         = RegisterWindowMessageA((LPSTR)szmsgLBCHANGEA);
    msgSHAREVIOLATIONA   = RegisterWindowMessageA((LPSTR)szmsgSHAREVIOLATIONA);
    msgFILEOKA           = RegisterWindowMessageA((LPSTR)szmsgFILEOKA);
    msgCOLOROKA          = RegisterWindowMessageA((LPSTR)szmsgCOLOROKA);
    msgSETRGBA           = RegisterWindowMessageA((LPSTR)szmsgSETRGBA);

    msgLBCHANGEW         = RegisterWindowMessageW((LPWSTR)szmsgLBCHANGEW);
    msgSHAREVIOLATIONW   = RegisterWindowMessageW((LPWSTR)szmsgSHAREVIOLATIONW);
    msgFILEOKW           = RegisterWindowMessageW((LPWSTR)szmsgFILEOKW);
    msgCOLOROKW          = RegisterWindowMessageW((LPWSTR)szmsgCOLOROKW);
    msgSETRGBW           = RegisterWindowMessageW((LPWSTR)szmsgSETRGBW);

    g_cfCIDA             = RegisterClipboardFormat(szShellIDList);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  LibMain
//
//  Initializes any instance specific data needed by functions in the
//  common dialogs.
//
//  Returns:   TRUE    - success
//             FALSE   - failure
//
////////////////////////////////////////////////////////////////////////////

BOOL LibMain(
    HANDLE hModule,
    DWORD dwReason,
    LPVOID lpRes)
{
    switch (dwReason)
    {
        case ( DLL_THREAD_ATTACH ) :
        {
            //
            //  Threads can only enter the comdlg32 dll from the
            //  Get{Open,Save}FileName apis, so the TLS lpCurDir alloc is
            //  done inside the InitFileDlg routine in fileopen.c
            //
            return (TRUE);
            break;
        }
        case ( DLL_THREAD_DETACH ) :
        {
            LPTSTR lpCurDir;
            LPDWORD lpCurThread;
            LPDWORD lpExtError;

            if (lpCurDir = (LPTSTR)TlsGetValue(g_tlsiCurDir))
            {
                LocalFree(lpCurDir);
                TlsSetValue(g_tlsiCurDir, NULL);
            }

            if (lpCurThread = (LPDWORD)TlsGetValue(g_tlsiCurThread))
            {
                LocalFree(lpCurThread);
                TlsSetValue(g_tlsiCurThread, NULL);
            }

            if (lpExtError = (LPDWORD)TlsGetValue(g_tlsiExtError))
            {
                LocalFree(lpExtError);
                TlsSetValue(g_tlsiExtError, NULL);
            }

            return (TRUE);
        }
        case ( DLL_PROCESS_ATTACH ) :
        {
            g_hinst = (HANDLE)hModule;

            if (!FInitColor(g_hinst) || !FInitFile(g_hinst))
            {
                goto CantInit;
            }

            DisableThreadLibraryCalls(hModule);

            //
            //  msgHELP is sent whenever a help button is pressed in one of
            //  the common dialogs (provided an owner was declared and the
            //  call to RegisterWindowMessage doesn't fail.
            //
            msgHELPA = RegisterWindowMessageA((LPSTR)szCommdlgHelpA);
            msgHELPW = RegisterWindowMessageW((LPWSTR)szCommdlgHelpW);

            //
            //  Need a semaphore locally for managing array of disk info.
            //
            InitializeCriticalSection(&g_csLocal);

            //
            //  Need a semaphore for control access to CreateThread.
            //
            InitializeCriticalSection(&g_csNetThread);

            //
            //  Need a semaphore for access to extended error info.
            //
            InitializeCriticalSection(&g_csExtError);

            //
            //  Allocate a tls index for curdir so we can make it per-thread.
            //
            if ((g_tlsiCurDir = TlsAlloc()) == 0xFFFFFFFF)
            {
                StoreExtendedError(CDERR_INITIALIZATION);
                goto CantInit;
            }

            //
            //  Allocate a tls index for curthread so we can give each a
            //  number.
            //
            if ((g_tlsiCurThread = TlsAlloc()) == 0xFFFFFFFF)
            {
                StoreExtendedError(CDERR_INITIALIZATION);
                goto CantInit;
            }

            //
            //  Allocate a tls index for extended error.
            //
            if ((g_tlsiExtError = TlsAlloc()) == 0xFFFFFFFF)
            {
                StoreExtendedError(CDERR_INITIALIZATION);
                goto CantInit;
            }

            dwNumDisks = 0;

            //
            //  NetEnumBuf allocated in ListNetDrivesHandler.
            //
            cbNetEnumBuf = WNETENUM_BUFFSIZE;

            hMPR = NULL;
            hMPRUI = NULL;

            hLNDEvent = NULL;

            return (TRUE);
            break;
        }
        case ( DLL_PROCESS_DETACH ) :
        {
            //
            //  We only want to do our clean up work if we are being called
            //  with freelibrary, not if the process is ending.
            //
            if (lpRes == NULL)
            {
                TermFile();
                TermPrint();
                TermColor();
                TermFont();

                TlsFree(g_tlsiCurDir);
                TlsFree(g_tlsiCurThread);
                TlsFree(g_tlsiExtError);

                DeleteCriticalSection(&g_csLocal);
                DeleteCriticalSection(&g_csNetThread);
                DeleteCriticalSection(&g_csExtError);
            }

            return (TRUE);
            break;
        }
    }

CantInit:
    return (FALSE);
    lpRes;
}

