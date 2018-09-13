/****************************** Module Header ******************************\
* Module Name: winlogon.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Main header file for winlogon
*
* History:
* 12-09-91 Davidc       Created.
*  6-May-1992 SteveDav     Added space for WINMM sound function
\***************************************************************************/


#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <lmsname.h>
#include <ntpoapi.h>
#endif


#include <windows.h>
#include <winbasep.h>
#include <winuserp.h>
#include <mmsystem.h>
#include <winwlx.h>
#include <regstr.h>

#include <winsta.h>
#include <muproto.h>


#ifndef RC_INVOKED

//
// Exit Codes, that will show up the bugcheck:
//

#define EXIT_INITIALIZATION_ERROR   1024
#define EXIT_SECURITY_INIT_ERROR    1025
#define EXIT_GINA_ERROR             1026
#define EXIT_SYSTEM_PROCESS_ERROR   1027
#define EXIT_NO_SAS_ERROR           1028
#define EXIT_SHUTDOWN_FAILURE       1029
#define EXIT_GINA_INIT_ERROR        1030
#define EXIT_PRIMARY_TERMINAL_ERROR 1031
#define EXIT_SAS_WINDOW_ERROR       1032
#define EXIT_DEVICE_MAP_ERROR       1033
#define EXIT_NO_MEMORY              1034

//
// Tempoary development aid - system logon capability
//

#if DBG
#define SYSTEM_LOGON
#endif

//
// Temporary development aid - Ctrl-Tasklist starts cmd shell
//
#if DBG
#define CTRL_TASKLIST_SHELL
#endif


//
// Define the input timeout delay for logon dialogs (seconds)
//

#define LOGON_TIMEOUT                       120


//
// Define the input timeout delay for the security options dialog (seconds)
//

#define OPTIONS_TIMEOUT                     120


//
// Define the number of days warning we give the user before their password expires
//

#define PASSWORD_EXPIRY_WARNING_DAYS        14


//
// Define the maximum time we display the 'wait for user to be logged off'
// dialog. This dialog should be interrupted by the user being logged off.
// This timeout is a safety measure in case that doesn't happen because
// of some system error.
//

#define WAIT_FOR_USER_LOGOFF_DLG_TIMEOUT    120 // seconds


//
// Define the account lockout limits
//
// A delay of LOCKOUT_BAD_LOGON_DELAY seconds will be added to
// each failed logon if more than LOCKOUT_BAD_LOGON_COUNT failed logons
// have occurred in the last LOCKOUT_BAD_LOGON_PERIOD seconds.
//

#define LOCKOUT_BAD_LOGON_COUNT             5
#define LOCKOUT_BAD_LOGON_PERIOD            60 // seconds
#define LOCKOUT_BAD_LOGON_DELAY             30 // seconds



//
// Define the maximum length of strings we'll use in winlogon
//

#define MAX_STRING_LENGTH   255
#define MAX_STRING_BYTES    (MAX_STRING_LENGTH + 1)


//
// Define the typical length of a string
// This is used as an initial allocation size for most string routines.
// If this is insufficient, the block is reallocated larger and
// the operation retried. i.e. Make this big enough for most strings
// to fit first time.
//

#define TYPICAL_STRING_LENGTH   60


//
// Private dialog message used to hide the window
//

#define WM_HIDEOURSELVES    (WM_USER + 600)


//
// Windows object names
//

#define BASE_WINDOW_STATION_NAME      TEXT("WinSta")
#define WINDOW_STATION_NAME           TEXT("WinSta0")
#define APPLICATION_DESKTOP_NAME      TEXT("Default")
#define WINLOGON_DESKTOP_NAME         TEXT("Winlogon")
#define SCREENSAVER_DESKTOP_NAME      TEXT("Screen-saver")

#define WINLOGON_DESKTOP_PATH         TEXT("%s\\Winlogon")
#define SCREENSAVER_DESKTOP_PATH      TEXT("%s\\Screen-saver")
#define APPLICATION_DESKTOP_PATH      TEXT("%s\\Default")

#define WL_NOTIFY_LOGON              0
#define WL_NOTIFY_LOGOFF             1
#define WL_NOTIFY_STARTUP            2
#define WL_NOTIFY_SHUTDOWN           3
#define WL_NOTIFY_STARTSCREENSAVER   4
#define WL_NOTIFY_STOPSCREENSAVER    5
#define WL_NOTIFY_LOCK               6
#define WL_NOTIFY_UNLOCK             7
#define WL_NOTIFY_STARTSHELL         8
#define WL_NOTIFY_POSTSHELL          9
#define WL_NOTIFY_MAX                10

typedef struct _WLP_NOTIFICATION_OBJECT {
    LIST_ENTRY List ;           // List control
    PWSTR DllName ;             // full path to the DLL
    ULONG   Type ;              // bitmask indicating notification types
    HMODULE Dll ;               // handle to DLL
    HKEY Key ;                  // Registry key
    LONG RefCount ;             // Refcount on this object
    DWORD dwMaxWait;            // Max wait time for sync calls (in seconds)

    LPTHREAD_START_ROUTINE EntryPoints[WL_NOTIFY_MAX];
} WLP_NOTIFICATION_OBJECT, * PWLP_NOTIFICATION_OBJECT ;



//
// Terminal registry location
//

#define TERMINAL_KEY L"SYSTEM\\CurrentControlSet\\Control\\Terminals"

#define DESKTOP_SECTION_KEY L"System\\CurrentControlSet\\Control\\Session Manager\\SubSystems\\DesktopSectionSizes"


//
// Default terminal information
//

#define DEFAULT_TERMINAL_NAME         TEXT("Default")
#define DEFAULT_MOUSE_DEVICE          TEXT("\\Device\\PointerClass0")
#define DEFAULT_KEYBOARD_DEVICE       TEXT("\\Device\\KeyboardClass0")



//
// Define the structure that contains information used when starting
// user processes.
// This structure should only be modified by SetUserProcessData()
//

typedef struct {
    HANDLE                  UserToken;  // NULL if no user logged on
    PSID                    UserSid;    // == WinlogonSid if no user logged on
    PSECURITY_DESCRIPTOR    NewProcessSD;
    PSECURITY_DESCRIPTOR    NewProcessTokenSD;
    PSECURITY_DESCRIPTOR    NewThreadSD;
    PSECURITY_DESCRIPTOR    NewThreadTokenSD;
    QUOTA_LIMITS            Quotas;
    LPTSTR                  CurrentDirectory;
    PVOID                   pEnvironment;
    HKEY                    hCurrentUser ;
    LONG                    Ref ;
    RTL_CRITICAL_SECTION    Lock ;
} USER_PROCESS_DATA;
typedef USER_PROCESS_DATA *PUSER_PROCESS_DATA;

//
// Define the structure that contains information about the user's profile.
//

typedef struct {
    LPTSTR ProfilePath;
    HANDLE hProfile;
    LPTSTR PolicyPath;
    LPTSTR NetworkDefaultUserProfile;
    LPTSTR ServerName;
    LPTSTR Environment;
    HANDLE hGPOEvent;
    HANDLE hGPOThread;
    HANDLE hGPONotifyEvent;
    HANDLE hGPOWaitEvent;
    DWORD  dwSysPolicyFlags;
    HANDLE hAutoEnrollmentHandler;
    PVOID  NetworkProviderTask;
} USER_PROFILE_INFO;
typedef USER_PROFILE_INFO *PUSER_PROFILE_INFO;



//
// Get any data types defined in module headers and used in GLOBALS
//
#define TYPES_ONLY
#include "ginamgr.h"
#undef TYPES_ONLY

typedef struct _TERMINAL *PTERMINAL;

typedef struct _MUGLOBALS {
    //
    // WinStation is logging off
    //
    BOOL fLogoffInProgress;

    //
    // Callback information
    //
    WCHAR CallbackNumber[ CALLBACK_LENGTH + 1 ];
    CALLBACKCLASS Callback;

    ULONG ConnectToSessionId;

    // Stored user configuration info gathered from local, DC, and
    // SAM databases.
    // QueryConfigResult can have the following values:
    //     ERROR_SUCCESS: The USerConfig was queried successfully.
    //     ERROR_INVALID_DATA: The UserConfig has not yet been queried.
    //     ERROR_GEN_FAILURE: The UserConfig was queried but the query failed.
    DWORD ConfigQueryResult;
    USERCONFIG UserConfig;
} MUGLOBALS, *PMUGLOBALS;



//
// Hydra Defines
//
#define WINSTATIONS_DISABLED    TEXT("WinStationsDisabled")
typedef int DLG_RETURN_TYPE, * PDLG_RETURN_TYPE;

//
// Hydra Global variables
//
extern ULONG     g_SessionId;
extern int       g_Console;
extern BOOL      g_IsTerminalServer;

typedef enum _WinstaState {
    Winsta_PreLoad,
    Winsta_Initialize,
    Winsta_NoOne,
    Winsta_NoOne_Display,
    Winsta_NoOne_SAS,
    Winsta_LoggedOnUser_StartShell,
    Winsta_LoggedOnUser,
    Winsta_LoggedOn_SAS,
    Winsta_Locked,
    Winsta_Locked_Display,
    Winsta_Locked_SAS,
    Winsta_WaitForLogoff,
    Winsta_WaitForShutdown,
    Winsta_Shutdown,
    Winsta_StateMax
} WinstaState;

#define IsSASState(State)   ((State == Winsta_NoOne_SAS) || \
                             (State == Winsta_LoggedOn_SAS) || \
                             (State == Winsta_Locked_SAS) )

#define IsDisplayState(State)   ((State == Winsta_NoOne_Display) || \
                                 (State == Winsta_LoggedOnUser) || \
                                 (State == Winsta_Locked_Display) || \
                                 (State == Winsta_WaitForLogoff) )

#define IsLocked( State )   ( ( State == Winsta_Locked ) || \
                              ( State == Winsta_Locked_SAS ) || \
                              ( State == Winsta_Locked_Display ) )

#if DBG
extern char * StateNames[Winsta_StateMax];

#define GetState(x) ( x < (sizeof(StateNames) / sizeof(char *)) ? StateNames[x] : "Invalid")
#endif

extern BOOLEAN  SasMessages;
#define DisableSasMessages()    SasMessages = FALSE;
#define TestSasMessages()       (SasMessages)
VOID
EnableSasMessages(HWND hWnd, PTERMINAL pTerm);

BOOL
QueueSasEvent(
    DWORD     dwSasType,
    PTERMINAL pTerm);

BOOL
FetchPendingSas(
    PDWORD    pSasType,
    PTERMINAL pTerm);

BOOL
KillMessageBox(DWORD SasCode);

PWSTR
AllocAndDuplicateString(
    PWSTR   pszString);

VOID
WlWalkNotifyList(
    PTERMINAL Terminal,
    DWORD Operation
    );

BOOL
WlpInitializeNotifyList(
    PTERMINAL Terminal
    );

BOOL
WlAddInternalNotify(
    LPTHREAD_START_ROUTINE  Function,
    DWORD Operation,
    BOOL Asynchronous,
    BOOL Impersonate,
    PWSTR Tag,
    DWORD dwMaxWait
    );


DWORD
PokeComCtl32(
    PVOID ignored
    );

NTSTATUS
LocalInitiateSystemShutdown(
    PUNICODE_STRING Message,
    ULONG Timeout,
    BOOLEAN bForceAppsClosed,
    BOOLEAN bRebootAfterShutdown
    );


//
// Multimedia initialization and migration
//

VOID WinmmLogon( BOOL );
VOID WinmmLogoff( VOID );
BOOL MigrateAllDrivers( VOID );
VOID MigrateSoundEvents( VOID );


typedef enum _ActiveDesktops {
    Desktop_Winlogon,
    Desktop_ScreenSaver,
    Desktop_Application,
    Desktop_Previous
} ActiveDesktops;

typedef struct _WINDOWSTATION {

    //
    // Next window station
    //

    struct _WINDOWSTATION * pNext;


    //
    // Handle to the window station object
    //

    HWINSTA hwinsta;
    LPWSTR  lpWinstaName;

    //
    // Desktops
    //

    HDESK           hdeskPrevious;
    HDESK           hdeskApplication;
    HDESK           hdeskWinlogon;
    HDESK           hdeskScreenSaver;   // screensaver's desktop
    ActiveDesktops  ActiveDesktop;      // Current, active desktop
    ActiveDesktops  PreviousDesktop;    // Previous desktop


    //
    // User information
    //

    HANDLE              hToken;         // user's token
    //PVOID               pGinaContext;
    USER_PROCESS_DATA   UserProcessData;
    USER_PROFILE_INFO   UserProfile;
    LUID                LogonId;
    PWSTR               LogonScripts;
    PWSTR               UserName;
    PWSTR               Domain ;

    // Stored ACL
    PVOID   Acl;

} WINDOWSTATION;
typedef WINDOWSTATION *PWINDOWSTATION;


#define MAX_WINDOW_MAPPERS  32


typedef struct _WindowMapper {
    DWORD                   fMapper;
    HWND                    hWnd;
    DLGPROC                 DlgProc;
    struct _WindowMapper *  pPrev;
    LPARAM                  InitialParameter;
} WindowMapper, * PWindowMapper;

#define WLX_SAS_INTERNAL_SC_EVENT   126

typedef struct _TERMINAL {

    //
    // Terminal signature
    //

    DWORD   CheckMark;


    //
    // Next terminal
    //

    struct _TERMINAL * pNext;


    //
    // Winlogon's window station
    //

    PWINDOWSTATION pWinStaWinlogon;


    //
    // Desktop switching information
    //

    PWSTR   pszDesktop;         // Name of current desktop.
    DWORD   DesktopLength;      // Length of name.


    //
    // SAS window handle
    //

    HWND hwndSAS;


    //
    // Misc items
    //

    DWORD   IniRef;

    BOOL    UserLoggedOn;
    DWORD   LogoffFlags;
    DWORD   TickCount;


    BOOL    ForwardCAD;
    BOOL    EnableSC ;
    BOOL    SafeMode ;
    DWORD   SasType;
    DWORD   LastGinaRet;

    HANDLE  hToken;    // Machine token
    HANDLE  hGPOEvent;
    HANDLE  hGPOThread;
    HANDLE  hGPONotifyEvent;
    HANDLE  hGPOWaitEvent;
    HANDLE hAutoEnrollmentHandler;
    DWORD   ErrorMode ;
    DWORD   SmartCardTid ;
    PVOID   CurrentScEvent ;
    //
    // These items need to be reviewed.
    //

    // Filled in by InitializeSecurity() at startup
    WinstaState WinlogonState;
    WinstaState PreviousWinlogonState;

    BOOL        ScreenSaverActive;
    BOOL        ShutdownStarted;

    BOOL        bIgnoreScreenSaverRequest;

    WindowMapper    Mappers[MAX_WINDOW_MAPPERS];
    DWORD           cActiveWindow;
    DWORD           PendingSasEvents[MAX_WINDOW_MAPPERS];
    DWORD           PendingSasHead;
    DWORD           PendingSasTail;
    BOOL            MessageBoxActive ;


    //
    // Gina information
    //

    GINASESSION Gina;

    //
    // Multi-User specific part of winlogon globals struct
    //
    MUGLOBALS MuGlobals;

    //
    // used to set the "ignore auto logon" flag when user does a log off
    //
    BOOL            IgnoreAutoLogon;


} TERMINAL;
typedef TERMINAL *PTERMINAL;

//
// Global variables
//

extern HANDLE    hFontThread;
extern BOOL      ExitWindowsInProgress;
extern BOOL      KernelDebuggerPresent;

extern HINSTANCE g_hInstance;
extern PTERMINAL g_pTerminals;
extern UINT      g_uSetupType;
extern BOOL      g_fExecuteSetup;
extern PSID      g_WinlogonSid;
extern BOOL      g_fAllowStatusUI;
extern PSID      gAdminSid ;
extern PSID      gLocalSid ;

extern HANDLE    NetworkProviderEvent ;


//
// Macro to determine if a given pointer is a terminal object.
//
#define TERMINAL_CHECKMARK   0x7465726d
#define VerifyHandle(h) ((PTERMINAL) ((((PTERMINAL)h)->CheckMark == TERMINAL_CHECKMARK) ? h : NULL))



//
// Define a macro to determine if we're a workstation or not
// This allows easy changes as new product types are added.
//

#define IsWorkstation(prodtype)  (((prodtype) == NtProductWinNt) \
                                   || ((prodtype) == NtProductServer))


//
// Macro for determining a buffer size
//

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))


#define DLG_SUCCESS IDOK
#define DLG_FAILURE IDCANCEL

//
// Include individual module header files
//
#include "wlxutil.h"
#include "wlx.h"
#include "sas.h"
#include "access.h"
#include "winutil.h"
#include "sysinit.h"
#include "ginamgr.h"
#include "debug.h"
#include "strings.h"
#include "wlxutil.h"
#include "secutil.h"
#include "logoff.h"
#include "misc.h"
#include "msgalias.h"
#include "usrpro.h"
#include "usrenv.h"
#include "envvar.h"
#include "monitor.h"
#include "scrnsave.h"
#include "timeout.h"
#include "provider.h"
#include "removabl.h"
#include "terminal.h"
#include "secboot.h"
#include "scavenge.h"
#include "sc.h"
#include "jobwait.h"

#ifdef _X86_
#include "os2ssmig.h"
#endif

#endif  /* !RC_INVOKED */

//
// Include resource header files
//
#include "win31mig.h"
#include "wlevents.h"
#include "stringid.h"
#include "dialogs.h"
#include "sbdlg.h"

#include "ctxdlgs.h"

#include <sfcapip.h>
