/****************************** Module Header ******************************\
* Module Name: provider.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Define functions that support multiple network providers
*
* History:
* 01-13-93 Davidc       Created.
\***************************************************************************/


//
// Exported function prototypes
//

int
MprLogonNotify(
    PTERMINAL pTerm,
    HWND hwndOwner,
    LPTSTR Name,
    LPTSTR Domain,
    LPTSTR Password,
    LPTSTR OldPassword OPTIONAL,
    PLUID LogonId,
    LPWSTR *MprLogonScripts
    );

int
MprChangePasswordNotify(
    PTERMINAL pTerm,
    HWND hwndOwner,
    PWSTR Provider,
    LPTSTR Name,
    LPTSTR Domain,
    LPTSTR Password,
    LPTSTR OldPassword,
    DWORD ChangeInfo,
    BOOL PassThrough
    );

BOOL
NoNeedToNotify(
    VOID
    );
