/*++

Copyright (c) 1990-1995,  Microsoft Corporation  All rights reserved.

Module Name:

    data.c

Abstract:

    This module contains the global data for the Win32 common dialogs.
    Anything added here must have 'extern' added to privcomd.h.

Revision History:

--*/



//
//  Include Files.
//

#include "windows.h"
#include <port1632.h>
#include "privcomd.h"
#include "color.h"




//
//  Global Variables.
//

//
//  FileOpen
//
TCHAR szOEMBIN[]        = TEXT("OEMBIN");
TCHAR szNull[]          = TEXT("");
TCHAR szStar[]          = TEXT("*");
TCHAR szStarDotStar[12] = TEXT("*.*");
TCHAR szDotStar[]       = TEXT(".*");


//
//  Color
//
DWORD rgbClient;
WORD H,S,L;
HBITMAP hRainbowBitmap;
BOOL bMouseCapture;
WNDPROC lpprocStatic;
SHORT nDriverColors;
BOOL bUserPressedCancel;

HWND hSave;

WNDPROC qfnColorDlg = NULL;
HDC hDCFastBlt = NULL;

SHORT cyCaption, cyBorder, cyVScroll;
SHORT cxVScroll, cxBorder, cxSize;
SHORT nBoxHeight, nBoxWidth;


//
//  dlgs.c
//
HINSTANCE g_hinst = NULL;

BOOL bMouse;                      // system has a mouse
BOOL bCursorLock;
WORD wWinVer;                     // Windows version
WORD wDOSVer;                     // DOS version

UINT msgHELPA;                    // initialized using RegisterWindowMessage
UINT msgHELPW;                    // initialized using RegisterWindowMessage

HDC hdcMemory = HNULL;            // temp DC used to draw bitmaps
HBITMAP hbmpOrigMemBmp = HNULL;   // bitmap originally selected into hdcMemory

OFN_DISKINFO gaDiskInfo[MAX_DISKS];

CRITICAL_SECTION g_csLocal;
CRITICAL_SECTION g_csNetThread;
CRITICAL_SECTION g_csExtError;

DWORD dwNumDisks;

HANDLE hMPR;
HANDLE hMPRUI;
HANDLE hLNDEvent;

DWORD g_tlsiCurDir;
DWORD g_tlsiCurThread;
DWORD g_tlsiExtError;

DWORD cbNetEnumBuf;
LPTSTR gpcNetEnumBuf;

