//*****************************************************************************
//
// HOOKS -
//
//     Header file for 32bit stubs and thunks of 16bit hooks
//
//
// 01-07-92  NanduriR   Created.
//
//*****************************************************************************

typedef LONG (APIENTRY *HKPROC)(INT, LONG, LONG);

typedef struct {
    HANDLE hMod;                  // Module handle
    INT    cHookProcs;            // Total Number of thunk stubs.
} HOOKPERPROCESSDATA, FAR *LPHOOKPERPROCESSDATA;

typedef struct {
    BYTE   iIndex;                // array index;
    BYTE   InUse;                 // TRUE if this Proc32 is already hooked
    HAND16 hMod16;                // 16bit HookDLL module handle
    HANDLE hMod;                  // Modulehande of Thunk Hook Dll
    HKPROC Proc32;                // 32bit HookProc stub
    INT    iHook;                 // type of Hook
    DWORD  Proc16;                // actual 16bit HookProc
    INT    TaskId;                // id of task that callled setwindowshook
    HHOOK  hHook;                 // handle returned by SetWindowHookEx
} HOOKSTATEDATA, FAR *LPHOOKSTATEDATA;

typedef struct {
    INT   nCode;                  // the input params to a hook func.
    LONG  wParam;
    LONG  lParam;
} HOOKPARAMS, FAR *LPHOOKPARAMS;

#define PUTMSGFILTER16(pMsg16,lpMsg) {\
        STOREWORD(pMsg16->hwnd, GETHWND16((lpMsg)->hwnd));\
        STOREWORD(pMsg16->message,  (lpMsg)->message);\
        STOREWORD(pMsg16->wParam,   (lpMsg)->wParam);\
        STORELONG(pMsg16->lParam,   (lpMsg)->lParam);\
        STORELONG(pMsg16->time, (lpMsg)->time);\
        STOREWORD(pMsg16->pt.x, (lpMsg)->pt.x);\
        STOREWORD(pMsg16->pt.y, (lpMsg)->pt.y);\
    }

#define GETMSGFILTER16(pMsg16,lpMsg) {\
        (lpMsg)->hwnd      = HWND32(FETCHWORD(pMsg16->hwnd));\
        (lpMsg)->message   = FETCHWORD(pMsg16->message);\
        (lpMsg)->wParam    = FETCHWORD(pMsg16->wParam);\
        (lpMsg)->lParam    = FETCHLONG(pMsg16->lParam);\
        (lpMsg)->time      = FETCHLONG(pMsg16->time);\
        (lpMsg)->pt.x      = FETCHSHORT(pMsg16->pt.x);\
        (lpMsg)->pt.y      = FETCHSHORT(pMsg16->pt.y);\
    }

#define PUTMOUSEHOOKSTRUCT16(pMHStruct16,lpMHStruct) {\
        STOREWORD(pMHStruct16->pt.x, (lpMHStruct)->pt.x);\
        STOREWORD(pMHStruct16->pt.y, (lpMHStruct)->pt.y);\
        STOREWORD(pMHStruct16->hwnd, GETHWND16((lpMHStruct)->hwnd));\
        STOREWORD(pMHStruct16->wHitTestCode,   (lpMHStruct)->wHitTestCode);\
        STORELONG(pMHStruct16->dwExtraInfo,   (lpMHStruct)->dwExtraInfo);\
    }


#define GETMOUSEHOOKSTRUCT16(pMHStruct16,lpMHStruct) {\
        (lpMHStruct)->pt.x        = FETCHSHORT(pMHStruct16->pt.x);\
        (lpMHStruct)->pt.y        = FETCHSHORT(pMHStruct16->pt.y);\
        (lpMHStruct)->hwnd           = HWND32(FETCHWORD(pMHStruct16->hwnd));\
        (lpMHStruct)->wHitTestCode   = FETCHWORD(pMHStruct16->wHitTestCode);\
        (lpMHStruct)->dwExtraInfo    = FETCHLONG(pMHStruct16->dwExtraInfo);\
    }


// afterdark 3.0 compares the t1=lpeventmsg->time with t2=getcurrenttime().
// physically t2 > t1 always -  we truncate t2 to a multiple of 64 and
// thus sometimes t2 < t1 (numerically) which confuses the app and
// triggers the screen saver. So we do identical truncation here.
// No compatibility flag is used
//                                                     - nanduri

#define PUTEVENTMSG16(pEventMsg16,lpEventMsg) {\
        STOREWORD(pEventMsg16->message,  (lpEventMsg)->message);\
        STOREWORD(pEventMsg16->paramL,   (lpEventMsg)->paramL);\
        STOREWORD(pEventMsg16->paramH,   (lpEventMsg)->paramH);\
        STORELONG(pEventMsg16->time, GRAINYTICS((lpEventMsg)->time));\
    }


#define PUTCBTACTIVATESTRUCT16(pCbtAStruct16,lpCbtAStruct) {\
     STOREWORD(pCbtAStruct16->fMouse,  (lpCbtAStruct)->fMouse);\
     STOREWORD(pCbtAStruct16->hWndActive, (GETHWND16((lpCbtAStruct)->hWndActive)));\
    }


#define GETCBTACTIVATESTRUCT16(pCbtAStruct16,lpCbtAStruct) {\
     (lpCbtAStruct)->fMouse = FETCHWORD(pCbtAStruct16->fMouse);\
     (lpCbtAStruct)->hWndActive = HWND32(FETCHWORD(pCbtAStruct16->hWndActive));\
    }


LONG APIENTRY WU32StdHookProc(INT nCode, LONG wParam, LONG lParam, INT iFunc);
LONG APIENTRY WU32SubStdHookProc01(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc02(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc03(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc04(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc05(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc06(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc07(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc08(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc09(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc10(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc11(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc12(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc13(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc14(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc15(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc16(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc17(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc18(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc19(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc20(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc21(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc22(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc23(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc24(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc25(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc26(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc27(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc28(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc29(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc30(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc31(INT nCode, LONG wParam, LONG lParam);
LONG APIENTRY WU32SubStdHookProc32(INT nCode, LONG wParam, LONG lParam);

BOOL W32InitHookState(HANDLE hMod);
BOOL W32GetNotInUseHookStateData(LPHOOKSTATEDATA lpData);
BOOL W32GetHookStateData(LPHOOKSTATEDATA lpData);
BOOL W32SetHookStateData(LPHOOKSTATEDATA lpData);
BOOL W32GetThunkHookProc(INT iHook, DWORD Proc16, LPHOOKSTATEDATA lpData);
HHOOK W32FreeHHook(INT iHook, DWORD Proc16);
HHOOK W32FreeHHookOfIndex(INT iFunc);
BOOL W32GetHookParams(LPHOOKPARAMS lpHookParams);
LONG ThunkCallWndProcHook(INT nCode, LONG wParam, LPCWPSTRUCT lpCwpStruct,
                                                     LPHOOKSTATEDATA lpHSData);
LONG ThunkCbtHook(INT nCode, LONG wParam, LONG lParam,
                                                     LPHOOKSTATEDATA lpHSData);
LONG ThunkKeyBoardHook(INT nCode, LONG wParam, LONG lParam,
                                                     LPHOOKSTATEDATA lpHSData);
LONG ThunkMsgFilterHook(INT nCode, LONG wParam, LPMSG lpMsg,
                                                     LPHOOKSTATEDATA lpHSData);
LONG ThunkJournalHook(INT nCode, LONG wParam, LPEVENTMSG lpEventMsg,
                                                     LPHOOKSTATEDATA lpHSData);
LONG ThunkDebugHook(INT nCode, LONG wParam, LONG lParam,
                                                     LPHOOKSTATEDATA lpHSData);
LONG ThunkMouseHook(INT nCode, LONG wParam, LPMOUSEHOOKSTRUCT lpMHStruct,
                                                     LPHOOKSTATEDATA lpHSData);
LONG ThunkShellHook(INT nCode, LONG wParam, LONG lParam,
                                                     LPHOOKSTATEDATA lpHSData);


LONG APIENTRY WU32StdDefHookProc(INT nCode, LONG wParam, LONG lParam, INT iFunc);
VOID W32UnhookHooks( HAND16 hMod16, BOOL fQueue );
BOOL W32FreeOwnedHooks(INT iTaskId);
INT W32IsDuplicateHook(INT iHook, DWORD Proc16, INT TaskId);

LONG ThunkCallWndProcHook16(INT nCode, LONG wParam, VPVOID lpCwpStruct,
                                                     LPHOOKSTATEDATA lpHSData);
LONG ThunkCbtHook16(INT nCode, LONG wParam, VPVOID lParam,
                                                     LPHOOKSTATEDATA lpHSData);
LONG ThunkKeyBoardHook16(INT nCode, LONG wParam, LONG lParam,
                                                     LPHOOKSTATEDATA lpHSData);
LONG ThunkMsgFilterHook16(INT nCode, LONG wParam, VPVOID lpMsg,
                                                     LPHOOKSTATEDATA lpHSData);
LONG ThunkJournalHook16(INT nCode, LONG wParam, VPVOID lpEventMsg,
                                                     LPHOOKSTATEDATA lpHSData);
LONG ThunkDebugHook16(INT nCode, LONG wParam, LONG lParam,
                                                     LPHOOKSTATEDATA lpHSData);
LONG ThunkMouseHook16(INT nCode, LONG wParam, VPVOID lpMHStruct,
                                                     LPHOOKSTATEDATA lpHSData);
LONG ThunkShellHook16(INT nCode, LONG wParam, LONG lParam,
                                                     LPHOOKSTATEDATA lpHSData);
DWORD  W32GetHookDDEMsglParam(VOID);
VOID GetEventMessage16(PEVENTMSG16 pEventMsg16, LPEVENTMSG  lpEventMsg);

