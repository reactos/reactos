//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       ginamgr.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-24-94   RichardW   Created
//
//----------------------------------------------------------------------------



#ifdef TYPES_ONLY

typedef
BOOL (WINAPI * PWLX_NEGOTIATE)(
    DWORD, DWORD *);

typedef
BOOL (WINAPI * PWLX_INITIALIZE)(
    LPWSTR, HANDLE, PVOID, PVOID, PVOID *);

typedef
VOID (WINAPI * PWLX_DISPLAYSASNOTICE)(
    PVOID );

typedef
int (WINAPI * PWLX_LOGGEDOUTSAS)(
    PVOID, DWORD, PLUID, PSID, PDWORD, PHANDLE, PWLX_MPR_NOTIFY_INFO, PVOID);

typedef
BOOL (WINAPI * PWLX_ACTIVATEUSERSHELL)(
    PVOID, PWSTR, PWSTR, PVOID);

typedef
int (WINAPI * PWLX_LOGGEDONSAS)(
    PVOID, DWORD, PVOID);

typedef
VOID (WINAPI * PWLX_DISPLAYLOCKEDNOTICE)(
    PVOID );

typedef
int (WINAPI * PWLX_WKSTALOCKEDSAS)(
    PVOID, DWORD );

typedef
BOOL (WINAPI * PWLX_ISLOCKOK)(
    PVOID );

typedef
BOOL (WINAPI * PWLX_ISLOGOFFOK)(
    PVOID );

typedef
VOID (WINAPI * PWLX_LOGOFF)(
    PVOID );

typedef
VOID (WINAPI * PWLX_SHUTDOWN)(
    PVOID, DWORD );

typedef
BOOL (WINAPI * PWLX_STARTAPPLICATION)(
    PVOID, PWSTR, PVOID, PWSTR);

typedef
BOOL (WINAPI * PWLX_SSNOTIFY)(
    PVOID, BOOL *);

typedef
BOOL (WINAPI * PWLX_NPLOAD)(
    PVOID, PWLX_MPR_NOTIFY_INFO );

typedef
BOOL (WINAPI * PWLX_DISPLAYSTATUSMESSAGE)(
    PVOID, HDESK, DWORD, PWSTR, PWSTR);

typedef
BOOL (WINAPI * PWLX_GETSTATUSMESSAGE)(
    PVOID, DWORD *, PWSTR, DWORD);

typedef
BOOL (WINAPI * PWLX_REMOVESTATUSMESSAGE)(
    PVOID);


#define WLX_NEGOTIATE_NAME               "WlxNegotiate"
#define WLX_INITIALIZE_NAME              "WlxInitialize"
#define WLX_DISPLAYSASNOTICE_NAME        "WlxDisplaySASNotice"
#define WLX_LOGGEDOUTSAS_NAME            "WlxLoggedOutSAS"
#define WLX_ACTIVATEUSERSHELL_NAME       "WlxActivateUserShell"
#define WLX_LOGGEDONSAS_NAME             "WlxLoggedOnSAS"
#define WLX_DISPLAYLOCKED_NAME           "WlxDisplayLockedNotice"
#define WLX_WKSTALOCKEDSAS_NAME          "WlxWkstaLockedSAS"
#define WLX_ISLOCKOK_NAME                "WlxIsLockOk"
#define WLX_ISLOGOFFOK_NAME              "WlxIsLogoffOk"
#define WLX_LOGOFF_NAME                  "WlxLogoff"
#define WLX_SHUTDOWN_NAME                "WlxShutdown"
#define WLX_STARTAPPLICATION_NAME        "WlxStartApplication"
#define WLX_SSNOTIFY_NAME                "WlxScreenSaverNotify"
#define WLX_NPLOAD_NAME                  "WlxNetworkProviderLoad"
#define WLX_DISPLAYSTATUSMESSAGE_NAME    "WlxDisplayStatusMessage"
#define WLX_GETSTATUSMESSAGE_NAME        "WlxGetStatusMessage"
#define WLX_REMOVESTATUSMESSAGE_NAME     "WlxRemoveStatusMessage"


typedef struct _GINASESSION {
    HANDLE                    hWlx;               // Handle used by the DLL to call us
    HANDLE                    hInstance;          // Handle of the DLL
    PVOID                     pGinaContext;       // Pointer we store for them
    DWORD                     cTimeout;           // Current timeout, in sec.
    PWLX_NEGOTIATE            pWlxNegotiate;      // WlxNegotiate function
    PWLX_INITIALIZE           pWlxInitialize;     // WlxInitialize function
    PWLX_DISPLAYSASNOTICE     pWlxDisplaySASNotice;
    PWLX_LOGGEDOUTSAS         pWlxLoggedOutSAS;
    PWLX_ACTIVATEUSERSHELL    pWlxActivateUserShell;
    PWLX_LOGGEDONSAS          pWlxLoggedOnSAS;
    PWLX_DISPLAYLOCKEDNOTICE  pWlxDisplayLockedNotice;
    PWLX_WKSTALOCKEDSAS       pWlxWkstaLockedSAS;
    PWLX_ISLOCKOK             pWlxIsLockOk;
    PWLX_ISLOGOFFOK           pWlxIsLogoffOk;
    PWLX_LOGOFF               pWlxLogoff;
    PWLX_SHUTDOWN             pWlxShutdown;
    PWLX_STARTAPPLICATION     pWlxStartApplication;
    PWLX_SSNOTIFY             pWlxScreenSaverNotify;
    PWLX_NPLOAD               pWlxNetworkProviderLoad;
    PWLX_DISPLAYSTATUSMESSAGE pWlxDisplayStatusMessage;
    PWLX_GETSTATUSMESSAGE     pWlxGetStatusMessage;
    PWLX_REMOVESTATUSMESSAGE  pWlxRemoveStatusMessage;
} GINASESSION, * PGINASESSION;


#define BREAK_NEGOTIATE     0x00000001
#define BREAK_INITIALIZE    0x00000002
#define BREAK_DISPLAY       0x00000004
#define BREAK_LOGGEDOUT     0x00000008
#define BREAK_ACTIVATE      0x00000010
#define BREAK_LOGGEDON      0x00000020
#define BREAK_DISPLAYLOCKED 0x00000040
#define BREAK_WKSTALOCKED   0x00000080
#define BREAK_ISLOCKOK      0x00000100
#define BREAK_ISLOGOFFOK    0x00000200
#define BREAK_LOGOFF        0x00000400
#define BREAK_SHUTDOWN      0x00000800

#define FLAG_ON(dw, f)      dw |= (f)
#define FLAG_OFF(dw, f)     dw &= (~(f))
#define TEST_FLAG(dw, f)    ((BOOL)(dw & (f)))

#else // not TYPES_ONLY

extern  DWORD   GinaBreakFlags;

BOOL LoadGinaDll(PTERMINAL pTerm, LPWSTR lpGinaName);


void
CADNotify(PTERMINAL pTerm, DWORD SasType);

#define IsShutdown(Result)    ((Result == WLX_SAS_ACTION_SHUTDOWN)           || \
                               (Result == WLX_SAS_ACTION_SHUTDOWN_REBOOT)    || \
                               (Result == WLX_SAS_ACTION_SHUTDOWN_POWER_OFF))

#define IsSuspend(Result)     ((Result == WLX_SAS_ACTION_SHUTDOWN_SLEEP)     || \
                               (Result == WLX_SAS_ACTION_SHUTDOWN_SLEEP2)    || \
                               (Result == WLX_SAS_ACTION_SHUTDOWN_HIBERNATE))


#endif
