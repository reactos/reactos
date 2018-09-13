/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WUSER31.H
 *  WOW32 16-bit Win 3.1 User API support
 *
 *  History:
 *  Created 16-Mar-1992 by Chandan S. Chauhan (ChandanC)
--*/

#define WINDOWPLACEMENT16TO32(vp,lp) {\
    LPWINDOWPLACEMENT16 lp16;\
    GETVDMPTR(vp, sizeof(WINDOWPLACEMENT16), lp16);\
    (lp)->length  = sizeof(WINDOWPLACEMENT);\
    (lp)->flags   = FETCHWORD(lp16->flags);\
    (lp)->showCmd = FETCHWORD(lp16->showCmd);\
    (lp)->ptMinPosition.x      = (INT) FETCHSHORT(lp16->ptMinPosition.x);\
    (lp)->ptMinPosition.y      = (INT) FETCHSHORT(lp16->ptMinPosition.y);\
    (lp)->ptMaxPosition.x      = (INT) FETCHSHORT(lp16->ptMaxPosition.x);\
    (lp)->ptMaxPosition.y      = (INT) FETCHSHORT(lp16->ptMaxPosition.y);\
    (lp)->rcNormalPosition.left   = (INT) FETCHSHORT(lp16->rcNormalPosition.left);\
    (lp)->rcNormalPosition.top    = (INT) FETCHSHORT(lp16->rcNormalPosition.top);\
    (lp)->rcNormalPosition.right  = (INT) FETCHSHORT(lp16->rcNormalPosition.right);\
    (lp)->rcNormalPosition.bottom = (INT) FETCHSHORT(lp16->rcNormalPosition.bottom);\
    FREEVDMPTR(lp16);\
}



#define WINDOWPLACEMENT32TO16(vp,lp) {\
    LPWINDOWPLACEMENT16 lp16;\
    GETVDMPTR(vp, sizeof(WINDOWPLACEMENT16), lp16);\
    STOREWORD(lp16->length, sizeof(WINDOWPLACEMENT16));\
    STOREWORD(lp16->flags, (lp)->flags);\
    STOREWORD(lp16->showCmd, (lp)->showCmd);\
    STORESHORT(lp16->ptMinPosition.x, (lp)->ptMinPosition.x);\
    STORESHORT(lp16->ptMinPosition.y, (lp)->ptMinPosition.y);\
    STORESHORT(lp16->ptMaxPosition.x, (lp)->ptMaxPosition.x);\
    STORESHORT(lp16->ptMaxPosition.y, (lp)->ptMaxPosition.y);\
    STORESHORT(lp16->rcNormalPosition.left, (lp)->rcNormalPosition.left);\
    STORESHORT(lp16->rcNormalPosition.top, (lp)->rcNormalPosition.top);\
    STORESHORT(lp16->rcNormalPosition.right, (lp)->rcNormalPosition.right);\
    STORESHORT(lp16->rcNormalPosition.bottom, (lp)->rcNormalPosition.bottom);\
    FLUSHVDMPTR(vp, sizeof(WINDOWPLACEMENT16), lp16);\
    FREEVDMPTR(lp16);\
}




ULONG FASTCALL WU32DlgDirSelectComboBoxEx(PVDMFRAME pFrame);
ULONG FASTCALL WU32DlgDirSelectEx(PVDMFRAME pFrame);
ULONG FASTCALL WU32EnableScrollBar(PVDMFRAME pFrame);
ULONG FASTCALL WU32ExitWindowsExec(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetClipCursor(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetCursor(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetDCEx(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetMessageExtraInfo(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetOpenClipboardWindow(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetWindowPlacement(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetFreeSystemResources(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetQueueStatus(PVDMFRAME pFrame);
ULONG FASTCALL WU32IsMenu(PVDMFRAME pFrame);
ULONG FASTCALL WU32LockWindowUpdate(PVDMFRAME pFrame);
ULONG FASTCALL WU32RedrawWindow(PVDMFRAME pFrame);
ULONG FASTCALL WU32ScrollWindowEx(PVDMFRAME pFrame);
ULONG FASTCALL WU32SetWindowPlacement(PVDMFRAME pFrame);
ULONG FASTCALL WU32SystemParametersInfo(PVDMFRAME pFrame);

ULONG FASTCALL WU32MapWindowPoints(PVDMFRAME pFrame);

//
// for ExitWindowsExec support
//

#define EWEXEC_SET   0
#define EWEXEC_DEL   1
#define EWEXEC_QUERY 2
#define WOWSZ_EWEXECEVENT "WOWEWExecEvent"
#define WOWSZ_EWEXECVALUE "EWExecCmdLine"

BOOL W32EWExecData(DWORD fnid, LPSTR lpData, DWORD cb);
BOOL W32EWExecer(VOID);

