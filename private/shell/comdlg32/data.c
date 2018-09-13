/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

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

#include "comdlg32.h"




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

SIZE  g_sizeDlg;

TCHAR g_szInitialCurDir[MAX_PATH];


//
//  Color
//
DWORD rgbClient;
WORD gHue, gSat, gLum;
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

DWORD dwNumDisks;

HANDLE hMPR;
HANDLE hMPRUI;
HANDLE hLNDEvent;

DWORD g_tlsiCurDlg;    // TLS index used to get the ptr to current CURDLG struct
                       // for each thread (see CURDLG in comdlg32.h)

DWORD g_tlsiExtError;  // ExtErrors are the most recent error per thread.

DWORD g_tlsLangID;     // TLS index used to get the current LangID for each thread.

DWORD cbNetEnumBuf;
LPTSTR gpcNetEnumBuf;

#ifdef WX86
  PALLOCCALLBX86 pfnAllocCallBx86;
#endif
