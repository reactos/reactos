/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WUWIND.H
 *  WOW32 16-bit User API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
--*/


/* Enumeration handler data
 */
typedef struct _WNDDATA {       /* wnddata */
    VPPROC  vpfnEnumWndProc;    // 16-bit enumeration function
    DWORD   dwUserWndParam;     // user param, if any
} WNDDATA, *PWNDDATA;


/* Function prototypes
 */
ULONG FASTCALL WU32AdjustWindowRect(PVDMFRAME pFrame);
ULONG FASTCALL WU32AdjustWindowRectEx(PVDMFRAME pFrame);
ULONG FASTCALL WU32ChildWindowFromPoint(PVDMFRAME pFrame);
ULONG FASTCALL WU32ChildWindowFromPointEx(PVDMFRAME pFrame);
ULONG FASTCALL WU32CreateWindow(PVDMFRAME pFrame);
ULONG FASTCALL WU32CreateWindowEx(PVDMFRAME pFrame);
ULONG FASTCALL W32CreateWindow(PVDMFRAME pFrame);
ULONG FASTCALL WU32DeferWindowPos(PVDMFRAME pFrame);
ULONG FASTCALL WU32DestroyWindow(PVDMFRAME pFrame);
ULONG FASTCALL WU32EndDeferWindowPos(PVDMFRAME pFrame);

BOOL    W32EnumWindowFunc(HWND hwnd, DWORD lParam);

ULONG FASTCALL WU32EnumChildWindows(PVDMFRAME pFrame);
ULONG FASTCALL WU32EnumTaskWindows(PVDMFRAME pFrame);
ULONG FASTCALL WU32EnumWindows(PVDMFRAME pFrame);
ULONG FASTCALL WU32FindWindow(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetActiveWindow(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetWindowDC(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetWindowLong(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetWindowTask(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetWindowText(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetWindowWord(PVDMFRAME pFrame);
ULONG FASTCALL WU32MenuItemFromPoint(PVDMFRAME pFrame);
ULONG FASTCALL WU32MoveWindow(PVDMFRAME pFrame);
ULONG FASTCALL WU32ScrollWindow(PVDMFRAME pFrame);
ULONG FASTCALL WU32SetWindowLong(PVDMFRAME pFrame);
ULONG FASTCALL WU32SetWindowPos(PVDMFRAME pFrame);
ULONG FASTCALL WU32SetWindowText(PVDMFRAME pFrame);
ULONG FASTCALL WU32SetWindowWord(PVDMFRAME pFrame);
ULONG FASTCALL WU32UpdateWindow(PVDMFRAME pFrame);
ULONG FASTCALL WU32WindowFromPoint(PVDMFRAME pFrame);

ULONG FASTCALL GetGWW_HINSTANCE(HWND hwnd);

extern HWND hwndProgman;
