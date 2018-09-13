/*++ BUILD Version: 0015    // Increment this if a change has global effects

/****************************** Module Header ******************************\
* Module Name: usersrv.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Typedefs, defines, and prototypes that are used exclusively by the User
* and Console server-side DLL.
*
* History:
* 04-28-91 DarrinM      Created from PROTO.H, MACRO.H, and STRTABLE.H
* 01-25-95 JimA         Split off from kernel-mode.
\***************************************************************************/

#ifndef _USERSRV_
#define _USERSRV_

#include <windows.h>
#include <w32gdip.h>

 /*
  * Enable warnings that are turned off default for NT but we want on
  */
#ifndef RC_INVOKED       // RC can't handle #pragmas
#pragma warning(error:4101)   // Unreferenced local variable
#endif

#ifndef _USERKDX_  /* if not building ntuser\kdexts */
#include <stddef.h>
#include <w32gdip.h>
#include <ddeml.h>
#include "ddemlp.h"
#include "winuserp.h"
#include "winuserk.h"
#include <dde.h>
#include <ddetrack.h>
#include "kbd.h"
#include <wowuserp.h>
#include <memory.h>
#include <w32err.h>
#include <string.h>
#include "help.h"

#include "user.h"
#include "cscall.h"
#undef MONITOR

#include "strid.h"
#include "csrmsg.h"
#endif /* _USERKDX_ */

typedef struct tagCTXHARDERRORINFO {
    CLIENT_ID ClientId;
    ULONG MessageId;
    LPWSTR pTitle;
    LPWSTR pMessage;
    ULONG Style;
    ULONG Timeout;
    ULONG Response;
    PULONG pResponse;
    HANDLE hEvent;
    BOOLEAN DoNotWait;
    struct tagCTXHARDERRORINFO * pchiNext;
} CTXHARDERRORINFO, *PCTXHARDERRORINFO;

/*
 * EndTask dialog, controls, timers, etc
 */
#define IDD_ENDTASK             10
#define IDC_STATUSICON          0x100
#define IDC_STATUSMSG           0x101
#define IDC_STATUSCANCEL        0x102
#define IDC_ENDNOW              0x103

#define IDI_CONSOLE             1
#define ETD_XICON               10
#define ETD_YICON               10

#define IDB_WARNING             0x200

#define IDT_CHECKAPPSTATE       0x300
#define IDT_PROGRESS            0x301

/*
 * End task dialog parameters.
 */
INT_PTR APIENTRY EndTaskDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
typedef struct _ENDDLGPARAMS {
    DWORD dwFlags;          /* EDPF_* flags */
    DWORD dwClientFlags;    /* WMCS_* flags defined in ntuser\user.h */
    DWORD dwRet;            /* User selection */
    UINT uStrId;            /* IDC_STATUS message */
    PCSR_THREAD pcsrt;      /* Not provided for console */
    LPARAM lParam;          /* hwnd for windows - pwcTitle for Console */
    DWORD dwCheckTimerCount;/* IDT_CHECKAPPTIMER tick count */
    HBITMAP hbmpWarning;    /* Warning bitmap to display on icon if not waiting */
    RECT rcWarning;         /* Warning bitmap position */
    HICON hIcon;            /* Application's icon */
    RECT rcBar;             /* Progress bar rect including edge */
    RECT rcProgress;        /* Next block's rect */
    int iProgressStop;      /* Progress bar right most coordinate */
    int iProgressWidth;     /* Progress bar block width */
    HBRUSH hbrProgress;     /* Used to draw progress bar blocks */
    RECT rcEndButton;       /* End button original position (move while in wait mode) */
} ENDDLGPARAMS;
/*
 * ENDDLGPARAMS dwFlags field
 */
#define EDPF_NODLG      0x00000001
#define EDPF_RESPONSE   0x00000002
#define EDPF_HUNG       0x00000004
#define EDPF_WAIT       0x00000008
#define EDPF_INPUT      0x00000010
/*
 * Commands returned from ThreadShutdownNotify
 */
#define TSN_APPSAYSOK        1
#define TSN_APPSAYSNOTOK     2
#define TSN_USERSAYSKILL     3
#define TSN_USERSAYSCANCEL   4
#define TSN_NOWINDOW         5
/*
 * Shared data between user and console
 */
extern HANDLE ghModuleWin;
extern DWORD gCmsHungAppTimeout;
extern DWORD gCmsWaitToKillTimeout;
extern DWORD gdwHungToKillCount;
extern DWORD gdwServicesProcessId;
extern DWORD gdwServicesWaitToKillTimeout;
extern DWORD gdwProcessTerminateTimeout;

/*
 * Hard error information
 */
typedef struct tagHARDERRORINFO {
    struct tagHARDERRORINFO *phiNext;
    PCSR_THREAD pthread;
    HANDLE hEventHardError;
    PHARDERROR_MSG pmsg;
    DWORD dwHEIFFlags;
    UNICODE_STRING usText; /* MessageBox text, caption and flags */
    UNICODE_STRING usCaption;
    DWORD dwMBFlags;
    DWORD dwVDMParam0;
    DWORD dwVDMParam1;
    PCTXHARDERRORINFO pCtxHEInfo;
} HARDERRORINFO, *PHARDERRORINFO;

#define HEIF_ACTIVE         0x00000001
#define HEIF_NUKED          0x00000002
#define HEIF_ALLOCATEDMSG   0x00000004
#define HEIF_REPLIED        0x00000008
#define HEIF_DEREFTHREAD    0x00000010
#define HEIF_WRONGDESKTOP   0x00000020
#define HEIF_SYSTEMERROR    0x00000040
#define HEIF_VDMERROR       0x00000080

BOOL BoostHardError(ULONG_PTR dwProcessId, DWORD dwCode);
#define BHE_ACTIVATE        0
#define BHE_FORCE           1
#define BHE_TEST            2
DWORD ThreadShutdownNotify(DWORD dwClientFlags, ULONG_PTR dwThread, LPARAM lParam);

/*
 * WM_CLIENTSHUTDOWN message callback
 */
typedef struct tagWMCSDATA {
    DWORD dwFlags;
    DWORD dwRet;
} WMCSDATA, *PWMCSDATA;

#define WMCSD_IGNORE    0x00000001
#define WMCSD_REPLY     0x00000002
#define WMCSD_RECEIVED  0x00000004
/*
 * Prototypes from server.c
 */
BOOL CALLBACK FindWindowFromThread (HWND hwnd, LPARAM lParam);

/*
 * !!! LATER - move other internal routines out of winuserp.h
 */

int  InternalDoEndTaskDialog(TCHAR* pszTitle, HANDLE h, int cSeconds);

#ifndef _USERKDX_  /* if not building ntuser\kdexts */
LPWSTR RtlLoadStringOrError(
    HANDLE hModule,
    UINT wID,
    LPWSTR lpDefault,
    PBOOL pAllocated,
    BOOL bAnsi
    );
#define ServerLoadString(hmod, id, default, allocated)\
        RtlLoadStringOrError((hmod), (id), (default), (allocated), FALSE)
#endif /* _USERKDX_ */


#define EnterCrit()     RtlEnterCriticalSection(&gcsUserSrv)
#define LeaveCrit()     RtlLeaveCriticalSection(&gcsUserSrv)


#ifdef FE_IME
BOOL  IsImeWindow( HWND hwnd );
#endif

#include "globals.h"

#endif  // !_USERSRV_
