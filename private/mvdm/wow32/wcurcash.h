/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WCURCASH.H
 *  WOW32 Cursor & Icon cash worker routines.
 *
 *  History:
 *  Created on Jan 27th-93 by ChandanC
 *
--*/


typedef struct _CURICON {
    struct _CURICON *pNext; // pointer to next hDDE alias
    DWORD   lpszIcon;       // name of resource
    HICON16 hIcon16;        // 16 bit handle of the Icon/Cursor given to app
    HICON16 hRes16;         // 16 bit handle of the resource
    WORD    ResType;        // type of resource, ie RT_ICON or RT_CURSOR
    HAND16  hInst;          // instance handle that owns the resource
    DWORD   dwThreadID;     // ID of the thread
} CURICON, *PCURICON;


HICON16 W32CheckWOWCashforIconCursors(VPVOID pData, WORD ResType);
BOOL    W32AddCursorIconCash (WORD hInst, LPSTR psz1, HICON16 hIcon16, HICON16 hRes16, WORD ResType);
HICON16 W32FindCursorIcon (WORD hInst, LPSTR psz, WORD ResType, HICON16 *phRes16);
VOID    W32DeleteCursorIconCash (HICON16 hRes16);
VOID    W32DeleteCursorIconCashForTask ();
