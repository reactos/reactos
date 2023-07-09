/****************************** Module Header ******************************\
* Module Name: csrhlpr.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This header file contains the prototypes for functions that marshel data
* for LPC from USER32 to CSR and are not found anywhere else.
*
* History:
* 10-21-98 mzoran     Created.
\***************************************************************************/

NTSTATUS
APIENTRY
CallUserpExitWindowsEx(
    IN UINT uFlags,
    IN DWORD dwReserved,
    OUT PBOOL pfSuccess);

NTSTATUS
APIENTRY
CallUserpRegisterLogonProcess(
    IN DWORD dwProcessId);

VOID
APIENTRY
Logon(
    IN BOOL fLogon);

VOID
APIENTRY
CsrWin32HeapFail(
    IN DWORD dwFlags,
    IN BOOL  bFail);

UINT
APIENTRY
CsrWin32HeapStat(
    PDBGHEAPSTAT    phs,
    DWORD   dwLen);

NTSTATUS
APIENTRY
UserConnectToServer(
    IN PWSTR ObjectDirectory,
    IN OUT PUSERCONNECT ConnectionInformation,
    IN OUT PULONG ConnectionInformationLength OPTIONAL,
    OUT PBOOLEAN CalledFromServer OPTIONAL
    );

#if !defined(BUILD_WOW6432) || defined(_WIN64)

_inline
NTSTATUS
UserConnectToServer(
    IN PWSTR ObjectDirectory,
    IN OUT PUSERCONNECT ConnectionInformation,
    IN OUT PULONG ConnectionInformationLength OPTIONAL,
    OUT PBOOLEAN CalledFromServer OPTIONAL
    ) {

    return CsrClientConnectToServer(ObjectDirectory,
                                    USERSRV_SERVERDLL_INDEX,
                                    NULL,
                                    ConnectionInformation,
                                    ConnectionInformationLength,
                                    CalledFromServer);

}

#endif
