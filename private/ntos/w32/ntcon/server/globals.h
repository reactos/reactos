/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    globals.h

Abstract:

    This module contains the global variables used by the
    console server DLL.

Author:

    Jerry Shea (jerrysh) 21-Sep-1993

Revision History:

--*/

extern CONSOLE_REGISTRY_INFO DefaultRegInfo;
extern PFONT_INFO FontInfo;

extern UINT       OEMCP;
extern UINT       WINDOWSCP;
extern HANDLE     ghInstance;
extern HICON      ghDefaultIcon;
extern HICON      ghDefaultSmIcon;
extern HCURSOR    ghNormalCursor;
extern CRITICAL_SECTION ConsoleHandleLock;
extern int        DialogBoxCount;
extern LPTHREAD_START_ROUTINE CtrlRoutine;  // client side ctrl-thread routine

// IME
extern LPTHREAD_START_ROUTINE ConsoleIMERoutine;  // client side console IME routine


extern BOOL FullScreenInitialized;
extern CRITICAL_SECTION ConsoleVDMCriticalSection;
extern PCONSOLE_INFORMATION ConsoleVDMOnSwitching;

extern DWORD      InputThreadTlsIndex;

extern int        MinimumWidthX;
extern SHORT      VerticalScrollSize;
extern SHORT      HorizontalScrollSize;
extern SHORT      VerticalClientToWindow;
extern SHORT      HorizontalClientToWindow;
extern BOOL       fOneTimeInitialized;
extern UINT       ConsoleOutputCP;
extern UINT       ProgmanHandleMessage;

extern DWORD      gExtendedEditKey;
extern BOOL       gfTrimLeadingZeros;

extern BOOL       gfLoadConIme;

// FE
extern ULONG NumberOfFonts;

extern CRITICAL_SECTION gInputThreadMsgLock;
