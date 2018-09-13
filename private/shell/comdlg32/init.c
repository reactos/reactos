/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    init.c

Abstract:

    This module contains the init routines for the Win32 common dialogs.

Revision History:

--*/



//
//  Include Files.
//

#include "comdlg32.h"
#include <shlobj.h>




//
//  External Declarations.
//

extern HDC hdcMemory;
extern HBITMAP hbmpOrigMemBmp;

extern CRITICAL_SECTION g_csLocal;
extern CRITICAL_SECTION g_csNetThread;

// TLS index to get current dlg info for the current thread
extern DWORD g_tlsiCurDlg;   

// TLS index to get most recent ExtError for the current thread
extern DWORD g_tlsiExtError; 

// TLS index to get LangID used for the current thread
extern DWORD g_tlsLangID;

extern HANDLE hMPR;
extern HANDLE hMPRUI;
extern HANDLE hLNDEvent;

extern DWORD dwNumDisks;
extern OFN_DISKINFO gaDiskInfo[MAX_DISKS];

extern DWORD cbNetEnumBuf;
extern LPTSTR gpcNetEnumBuf;

extern SIZE g_sizeDlg;

extern TCHAR g_szInitialCurDir[MAX_PATH];

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

BOOL g_bMirroredOS              = FALSE;
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

int FInitColor(
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

#ifdef UNICODE
    msgLBCHANGEW         = RegisterWindowMessageW((LPWSTR)szmsgLBCHANGEW);
    msgSHAREVIOLATIONW   = RegisterWindowMessageW((LPWSTR)szmsgSHAREVIOLATIONW);
    msgFILEOKW           = RegisterWindowMessageW((LPWSTR)szmsgFILEOKW);
    msgCOLOROKW          = RegisterWindowMessageW((LPWSTR)szmsgCOLOROKW);
    msgSETRGBW           = RegisterWindowMessageW((LPWSTR)szmsgSETRGBW);
#else
    msgLBCHANGEW         = msgLBCHANGEA;
    msgSHAREVIOLATIONW   = msgSHAREVIOLATIONA;
    msgFILEOKW           = msgFILEOKA;
    msgCOLOROKW          = msgCOLOROKA;
    msgSETRGBW           = msgSETRGBA;
#endif

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

// ccover needs to link to C-runtime, so we rename LibMain to DllMain
#ifdef CCOVER 
#define LibMain DllMain
#endif

BOOL LibMain(
    HANDLE hModule,
    DWORD dwReason,
    LPVOID lpRes)
{
    switch (dwReason)
    {
        case ( DLL_THREAD_ATTACH ) :
        case ( DLL_THREAD_DETACH ) :
        {
            //
            //  Threads can only enter and leave the comdlg32 dll from the
            //  Get{Open,Save}FileName apis, so the TLS lpCurDlg alloc is
            //  done inside the InitFileDlg routine in fileopen.c
            //
            return (TRUE);
            break;
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
            //  call to RegisterWindowMessage doesn't fail).
            //
            msgHELPA = RegisterWindowMessageA((LPSTR)szCommdlgHelpA);
#ifdef UNICODE
            msgHELPW = RegisterWindowMessageW((LPWSTR)szCommdlgHelpW);
#else
            msgHELPW = msgHELPA;
#endif

            //
            //  Need a semaphore locally for managing array of disk info.
            //
            InitializeCriticalSection(&g_csLocal);

            //
            //  Need a semaphore for control access to CreateThread.
            //
            InitializeCriticalSection(&g_csNetThread);

            //
            //  Allocate a tls index for curdlg so we can make it per-thread.
            //
            if ((g_tlsiCurDlg = TlsAlloc()) != 0xFFFFFFFF)
            {
                // mark the list as empty
                TlsSetValue(g_tlsiCurDlg, (LPVOID) 0);
            }
            else
            {
                StoreExtendedError(CDERR_INITIALIZATION);
                goto CantInit;
            }

            //
            //  Store the current directory on process attach.
            //
            GetCurrentDirectory(ARRAYSIZE(g_szInitialCurDir), g_szInitialCurDir);

            //
            //  Allocate a tls index for extended error.
            //
            if ((g_tlsiExtError = TlsAlloc()) == 0xFFFFFFFF)
            {
                StoreExtendedError(CDERR_INITIALIZATION);
                goto CantInit;
            }

            //
            //  Allocate a tls index for LangID so we can make it per-thread.
            //  it heavily used by CDLoadString.
            //
            if ((g_tlsLangID = TlsAlloc()) != 0xFFFFFFFF)
            {
                // mark the list as Neutral
                TlsSetValue(g_tlsLangID, (LPVOID) MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
            }
            else
            {
                StoreExtendedError(CDERR_INITIALIZATION);
                goto CantInit;
            }



            dwNumDisks = 0;
            gpcNetEnumBuf = NULL;

            //
            //  NetEnumBuf allocated in ListNetDrivesHandler.
            //
            cbNetEnumBuf = WNETENUM_BUFFSIZE;

            hMPR = NULL;
            hMPRUI = NULL;

            hLNDEvent = NULL;

            //
            //  For file open dialog.
            //
            g_sizeDlg.cx = 0;
            g_sizeDlg.cy = 0;

            g_bMirroredOS = IS_MIRRORING_ENABLED();
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

                TlsFree(g_tlsiCurDlg);
                TlsFree(g_tlsiExtError);
                TlsFree(g_tlsLangID);

                DeleteCriticalSection(&g_csLocal);
                DeleteCriticalSection(&g_csNetThread);
            }

            return (TRUE);
            break;
        }
    }

CantInit:
    return (FALSE);
    lpRes;
}
