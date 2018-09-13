/****************************** Module Header ******************************\
* Module Name: security.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Define various winlogon security-related routines
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/


extern PSID pWinlogonSid;

//
// Types used by security descriptor helper routines
//

typedef LONG    ACEINDEX;
typedef ACEINDEX *PACEINDEX;

typedef struct _MYACE {
    PSID    Sid;
    ACCESS_MASK AccessMask;
    UCHAR   InheritFlags;
} MYACE;
typedef MYACE *PMYACE;


//
// Exported function prototypes
//


VOID
SetMyAce(
    PMYACE MyAce,
    PSID Sid,
    ACCESS_MASK Mask,
    UCHAR InheritFlags
    );

PSECURITY_DESCRIPTOR
CreateSecurityDescriptor(
    PMYACE  MyAce,
    ACEINDEX AceCount
    );

BOOL
DeleteSecurityDescriptor(
    PSECURITY_DESCRIPTOR SecurityDescriptor
    );

BOOL
SetWinlogonDesktopSecurity(
    IN HDESK   hdesk,
    IN PSID    WinlogonSid
    );

BOOL
SetUserDesktopSecurity(
    IN HDESK   hdesk,
    IN PSID    UserSid,
    IN PSID    WinlogonSid
    );

BOOL
InitializeSecurity(void);

BOOL
InitializeWinstaSecurity(
    PWINDOWSTATION pWS);

PSID
CreateLogonSid(
    PLUID LogonId OPTIONAL
    );

VOID
DeleteLogonSid(
    PSID Sid
    );

PSECURITY_DESCRIPTOR
CreateUserProfileKeySD(
    PSID    UserSid,
    PSID    WinlogonSid,
    BOOL    AllAccess
    );

BOOL
EnablePrivilege(
    ULONG Privilege,
    BOOL Enable
    );

BOOL
SetUserProcessData(
    PUSER_PROCESS_DATA UserProcessData,
    HANDLE  UserToken,
    PQUOTA_LIMITS Quotas OPTIONAL,
    PSID    UserSid,
    PSID    WinlogonSid
    );

BOOL
SecurityChangeUser(
    PTERMINAL pTerm,
    HANDLE Token,
    PQUOTA_LIMITS Quotas OPTIONAL,
    PSID LogonSid,
    BOOL UserLoggedOn
    );

BOOL
TestTokenForAdmin(
    HANDLE Token
    );

BOOL
TestUserForAdmin(
    PTERMINAL pTerm,
    IN PWCHAR UserName,
    IN PWCHAR Domain,
    IN PUNICODE_STRING PasswordString
    );

HANDLE
ImpersonateUser(
    PUSER_PROCESS_DATA UserProcessData,
    HANDLE ThreadHandle OPTIONAL
    );

BOOL
StopImpersonating(
    HANDLE ThreadHandle
    );

BOOL
TestUserPrivilege(
    PTERMINAL pTerm,
    ULONG Privilege
    );

VOID
HidePassword(
    PUCHAR Seed OPTIONAL,
    PUNICODE_STRING Password
    );


VOID
RevealPassword(
    PUNICODE_STRING HiddenPassword
    );

VOID
ErasePassword(
    PUNICODE_STRING Password
    );

BOOL
SetProcessToken(
    HANDLE      hProcess,
    HANDLE      hThread,
    PSECURITY_DESCRIPTOR    psd,
    HANDLE      hToken
    );

PSECURITY_DESCRIPTOR
CreateUserThreadSD(
    PSID    UserSid,
    PSID    WinlogonSid
    );

PSECURITY_DESCRIPTOR
CreateUserThreadTokenSD(
    PSID    UserSid,
    PSID    WinlogonSid
    );

HANDLE ExecUserThread(
    IN PTERMINAL pTerm,
    IN LPTHREAD_START_ROUTINE lpStartAddress,
    IN LPVOID Parameter,
    IN DWORD Flags,
    OUT LPDWORD ThreadId
    );

BOOL
RemoveUserFromWinsta(
    PWINDOWSTATION      pWS,
    HANDLE              Token );

BOOL
AddUserToWinsta(
    PWINDOWSTATION      pWS,
    PSID                LogonSid,
    HANDLE              Token );

BOOL
FastSetWinstaSecurity(
    PWINDOWSTATION      pWS,
    BOOL                FullAccess);
