/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WMDISP32.H
 *  WOW32 32-bit message thunks
 *
 *  History:
 *  Created 19-Feb-1992 by Chandan S. Chauhan (ChandanC)
 *  Changed 12-May-1992 by Mike Tricker (MikeTri) Added MultiMedia prototypes
--*/
#ifndef _DEF_WMDISP32_  // if this hasn't already been included
#define _DEF_WMDISP32_


/* Types
 */

//
// W32MSGPARAMEX structure defined below is passed to all the 32->16
// message thunks.  pww provides quick access to WOW words, and
// dwParam provides a DWORD to squirrel away a value during thunking
// for use in unthunking.  The scope of dwParam is strictly the
// thunking and subsequent unthunking of one message.
//

typedef struct _WM32MSGPARAMEX *LPWM32MSGPARAMEX;
typedef BOOL   (FASTCALL LPFNM32PROTO)(LPWM32MSGPARAMEX lpwm32mpex);
typedef LPFNM32PROTO *LPFNM32;

typedef struct _WM32MSGPARAMEX {
    HWND hwnd;
    UINT uMsg;
    UINT uParam;
    LONG lParam;
    PARM16 Parm16;
    LPFNM32 lpfnM32;    // function address
    BOOL fThunk;
    LONG lReturn;
    PWW  pww;
    DWORD dwParam;
    BOOL fFree;
    DWORD dwTmp[2];
} WM32MSGPARAMEX;

/* Dispatch table entry
 */
typedef struct _M32 {   /* w32 */
    LPFNM32 lpfnM32;    // function address
#ifdef DEBUG_OR_WOWPROFILE
    LPSZ    lpszW32;    // message name (DEBUG version only)
    DWORD   cCalls;     // # times the message has been passed
    DWORD   cTics;      // sum total of thunk tics
#endif
} M32, *PM32;

extern  BOOL fThunkDDEmsg;

#define WIN31_MM_CALCSCROLL  0x10AC   // WM_USER+0xCAC

/* Function prototypes
 */
LONG    W32Win16WndProcEx(HWND hwnd, UINT uMsg, UINT uParam, LONG lParam, DWORD dwCPD, PWW pww);
BOOL    W32Win16DlgProcEx(HWND hwnd, UINT uMsg, UINT uParam, LONG lParam, DWORD dwCPD, PWW pww);

LPFNM32PROTO WM32NoThunking;
LPFNM32PROTO WM32Undocumented;
LPFNM32PROTO WM32Create;
LPFNM32PROTO WM32Activate;
LPFNM32PROTO WM32VKeyToItem;
LPFNM32PROTO WM32SetFocus;
LPFNM32PROTO WM32SetText;
LPFNM32PROTO WM32GetText;
LPFNM32PROTO WM32EraseBkGnd;
LPFNM32PROTO WM32ActivateApp;
LPFNM32PROTO WM32RenderFormat;
LPFNM32PROTO WM32GetMinMaxInfo;
LPFNM32PROTO WM32NCPaint;
LPFNM32PROTO WM32NCDestroy;
LPFNM32PROTO WM32GetDlgCode;
LPFNM32PROTO WM32NextDlgCtl;
LPFNM32PROTO WM32DrawItem;
LPFNM32PROTO WM32MeasureItem;
LPFNM32PROTO WM32DeleteItem;
LPFNM32PROTO WM32SetFont;
LPFNM32PROTO WM32QueryDragIcon;
LPFNM32PROTO WM32CompareItem;
LPFNM32PROTO WM32NCCalcSize;
LPFNM32PROTO WM32Command;
LPFNM32PROTO WM32Timer;
LPFNM32PROTO WM32HScroll;
LPFNM32PROTO WM32InitMenu;
LPFNM32PROTO WM32MenuSelect;
LPFNM32PROTO WM32MenuChar;
LPFNM32PROTO WM32EnterIdle;
LPFNM32PROTO WM32ParentNotify;
LPFNM32PROTO WM32MDICreate;
LPFNM32PROTO WM32MDIActivate;
LPFNM32PROTO WM32MDIGetActive;
LPFNM32PROTO WM32MDISetMenu;
LPFNM32PROTO WM32PaintClipBoard;
LPFNM32PROTO WM32SizeClipBoard;
LPFNM32PROTO WM32AskCBFormatName;
LPFNM32PROTO WM32ChangeCBChain;
LPFNM32PROTO WM32DDEInitiate;
LPFNM32PROTO WM32DDEAck;
LPFNM32PROTO WM32DDERequest;
LPFNM32PROTO WM32DDEAdvise;
LPFNM32PROTO WM32DDEData;
LPFNM32PROTO WM32DDEPoke;
LPFNM32PROTO WM32DDEExecute;
LPFNM32PROTO WM32CtlColor;
LPFNM32PROTO WM32GetFont;
LPFNM32PROTO WM32MNFindMenuWindow;
LPFNM32PROTO WM32NextMenu;
LPFNM32PROTO WM32Destroy;
LPFNM32PROTO WM32WindowPosChanging ;
LPFNM32PROTO WM32DropFiles ;
LPFNM32PROTO WM32DropObject ;
LPFNM32PROTO WM32DestroyClipboard;
LPFNM32PROTO WM32NextMenu;
LPFNM32PROTO WM32CopyData;
LPFNM32PROTO WM32MMCalcScroll;
LPFNM32PROTO WM32Thunk16To32;
LPFNM32PROTO WM32WinHelp;
LPFNM32PROTO WM32Notify;
LPFNM32PROTO WM32Sizing;
LPFNM32PROTO WM32xxxUIState;
LPFNM32PROTO WM32NotifyWow;
#ifdef FE_IME
LPFNM32PROTO WM32IMEReport;
#endif // FE_IME
LPFNM32PROTO WM32PrintClient;

#endif  // #ifndef _DEF_WMDISP32_ THIS SHOULD BE THE LAST LINE IN THIS FILE

