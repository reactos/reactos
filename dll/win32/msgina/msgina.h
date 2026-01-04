#ifndef _MSGINA_H
#define _MSGINA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <tchar.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#include <winwlx.h>
#include <ndk/rtlfuncs.h>
#include <ndk/setypes.h> // For SE_*_PRIVILEGE
#include <ntsecapi.h>

#include <strsafe.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(msgina);

#include "resource.h"


typedef struct
{
    HANDLE hWlx;
    LPWSTR station;
    PWLX_DISPATCH_VERSION_1_3 pWlxFuncs;
    HANDLE hDllInstance;
    HWND hStatusWindow;
    HANDLE LsaHandle;
    ULONG AuthenticationPackage;
    BOOL bDisableCAD;
    BOOL bAutoAdminLogon;
    BOOL bDontDisplayLastUserName;
    BOOL bShutdownWithoutLogon;
    BOOL bIgnoreShiftOverride;

    ULONG nShutdownAction;

    /* Information to be filled during logon */
    WCHAR UserName[256];
    WCHAR DomainName[256];
    WCHAR Password[256];
    SYSTEMTIME LogonTime;
    HANDLE UserToken;
    PLUID pAuthenticationId;
    PDWORD pdwOptions;
    PWLX_MPR_NOTIFY_INFO pMprNotifyInfo;
    PVOID *pProfile;
} GINA_CONTEXT, *PGINA_CONTEXT;

extern HINSTANCE hDllInstance;

typedef BOOL (*PFGINA_INITIALIZE)(PGINA_CONTEXT);
typedef BOOL (*PFGINA_DISPLAYSTATUSMESSAGE)(PGINA_CONTEXT, HDESK, DWORD, PWSTR, PWSTR);
typedef BOOL (*PFGINA_REMOVESTATUSMESSAGE)(PGINA_CONTEXT);
typedef VOID (*PFGINA_DISPLAYSASNOTICE)(PGINA_CONTEXT);
typedef INT (*PFGINA_LOGGEDONSAS)(PGINA_CONTEXT, DWORD);
typedef INT (*PFGINA_LOGGEDOUTSAS)(PGINA_CONTEXT);
typedef INT (*PFGINA_LOCKEDSAS)(PGINA_CONTEXT);
typedef VOID (*PFGINA_DISPLAYLOCKEDNOTICE)(PGINA_CONTEXT);

typedef struct _GINA_UI
{
    PFGINA_INITIALIZE Initialize;
    PFGINA_DISPLAYSTATUSMESSAGE DisplayStatusMessage;
    PFGINA_REMOVESTATUSMESSAGE RemoveStatusMessage;
    PFGINA_DISPLAYSASNOTICE DisplaySASNotice;
    PFGINA_LOGGEDONSAS LoggedOnSAS;
    PFGINA_LOGGEDOUTSAS LoggedOutSAS;
    PFGINA_LOCKEDSAS LockedSAS;
    PFGINA_DISPLAYLOCKEDNOTICE DisplayLockedNotice;
} GINA_UI, *PGINA_UI;

/* lsa.c */

NTSTATUS
ConnectToLsa(
    PGINA_CONTEXT pgContext);

NTSTATUS
MyLogonUser(
    HANDLE LsaHandle,
    ULONG AuthenticationPackage,
    LPWSTR lpszUsername,
    LPWSTR lpszDomain,
    LPWSTR lpszPassword,
    PHANDLE phToken,
    PNTSTATUS SubStatus);

/* msgina.c */

BOOL
DoAdminUnlock(
    IN PGINA_CONTEXT pgContext,
    IN PWSTR UserName,
    IN PWSTR Domain,
    IN PWSTR Password);

NTSTATUS
DoLoginTasks(
    IN OUT PGINA_CONTEXT pgContext,
    IN PWSTR UserName,
    IN PWSTR Domain,
    IN PWSTR Password,
    OUT PNTSTATUS SubStatus);

BOOL
CreateProfile(
    IN OUT PGINA_CONTEXT pgContext,
    IN PWSTR UserName,
    IN PWSTR Domain,
    IN PWSTR Password);

/* shutdown.c */

/**
 * @brief   Shutdown state flags
 * @see
 * https://learn.microsoft.com/en-us/previous-versions/windows/it-pro/windows-2000-server/cc962586(v=technet.10)
 * https://learn.microsoft.com/en-us/previous-versions/windows/it-pro/windows-server-2003/cc783367(v=ws.10)
 **/
#define WLX_SHUTDOWN_STATE_LOGOFF       0x01    ///< "Log off <username>"
#define WLX_SHUTDOWN_STATE_POWER_OFF    0x02    ///< "Shut down"
#define WLX_SHUTDOWN_STATE_REBOOT       0x04    ///< "Restart"
// 0x08 ///< "Restart in MS-DOS mode" - Yes, WinNT/2k/XP/2k3 msgina.dll/shell32.dll has it!
#define WLX_SHUTDOWN_STATE_SLEEP        0x10    ///< "Stand by"
#define WLX_SHUTDOWN_STATE_SLEEP2       0x20    ///< "Stand by (with wakeup events disabled)"
#define WLX_SHUTDOWN_STATE_HIBERNATE    0x40    ///< "Hibernate"
#define WLX_SHUTDOWN_STATE_DISCONNECT   0x80    ///< "Disconnect" (only available in Terminal Services sessions)
#define WLX_SHUTDOWN_AUTOUPDATE         0x100   ///< Set when updates are queued

DWORD
LoadShutdownSelState(
    _In_ HKEY hKeyCurrentUser);

VOID
SaveShutdownSelState(
    _In_ HKEY hKeyCurrentUser,
    _In_ DWORD ShutdownCode);

DWORD
GetAllowedShutdownOptions(
    _In_opt_ HKEY hKeyCurrentUser,
    _In_opt_ HANDLE hUserToken);

INT_PTR
ShutdownDialog(
    IN HWND hwndDlg,
    IN DWORD ShutdownOptions,
    IN PGINA_CONTEXT pgContext);

/* utils.c */

LONG
RegOpenLoggedOnHKCU(
    _In_opt_ HANDLE hUserToken,
    _In_ REGSAM samDesired,
    _Out_ PHKEY phkResult);

LONG
ReadRegSzValue(
    _In_ HKEY hKey,
    _In_ PCWSTR pszValue,
    _Out_ PWSTR* pValue);

LONG
ReadRegDwordValue(
    _In_ HKEY hKey,
    _In_ PCWSTR pszValue,
    _Out_ PDWORD pValue);

BOOL
TestTokenPrivilege(
    _In_opt_ HANDLE hToken,
    _In_ ULONG Privilege);

PWSTR
DuplicateString(
    _In_opt_ PCWSTR Str);


#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _MSGINA_H */
