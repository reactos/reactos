/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

_WdfVersionBuild_

Module Name:

    WdfInstaller.h

Abstract:

    Contains prototypes for the WDF installer support.

Author:

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _WDFINSTALLER_H_
#define _WDFINSTALLER_H_



#if (NTDDI_VERSION >= NTDDI_WIN2K)



typedef struct _WDF_COINSTALLER_INSTALL_OPTIONS {
    ULONG  Size;
    BOOL   ShowRebootPrompt;
} WDF_COINSTALLER_INSTALL_OPTIONS, *PWDF_COINSTALLER_INSTALL_OPTIONS;

VOID
FORCEINLINE
WDF_COINSTALLER_INSTALL_OPTIONS_INIT(
    __out PWDF_COINSTALLER_INSTALL_OPTIONS ClientOptions
    )
{
    RtlZeroMemory(ClientOptions, sizeof(WDF_COINSTALLER_INSTALL_OPTIONS));

    ClientOptions->Size = sizeof(WDF_COINSTALLER_INSTALL_OPTIONS);
}


//----------------------------------------------------------------------------
// To be called before (your) WDF driver is installed.
//----------------------------------------------------------------------------
ULONG
WINAPI
WdfPreDeviceInstall(
    __in LPCWSTR  InfPath,
    __in_opt LPCWSTR  InfSectionName
    );

typedef
ULONG
(WINAPI *PFN_WDFPREDEVICEINSTALL)(
    __in LPCWSTR  InfPath,
    __in_opt LPCWSTR  InfSectionName
    );

ULONG
WINAPI
WdfPreDeviceInstallEx(
    __in LPCWSTR  InfPath,
    __in_opt LPCWSTR  InfSectionName,
    __in PWDF_COINSTALLER_INSTALL_OPTIONS ClientOptions
    );

typedef
ULONG
(WINAPI *PFN_WDFPREDEVICEINSTALLEX)(
    __in LPCWSTR  InfPath,
    __in_opt LPCWSTR  InfSectionName,
    __in PWDF_COINSTALLER_INSTALL_OPTIONS ClientOptions
    );

//----------------------------------------------------------------------------
// To be called after (your) WDF driver is installed.
//----------------------------------------------------------------------------
ULONG
WINAPI
WdfPostDeviceInstall(
    __in LPCWSTR  InfPath,
    __in_opt LPCWSTR  InfSectionName
    );

typedef
ULONG
(WINAPI *PFN_WDFPOSTDEVICEINSTALL)(
    __in LPCWSTR  InfPath,
    __in_opt LPCWSTR  InfSectionName
    );

//----------------------------------------------------------------------------
// To be called before (your) WDF driver is removed.
//----------------------------------------------------------------------------
ULONG
WINAPI
WdfPreDeviceRemove(
    __in LPCWSTR  InfPath,
    __in_opt LPCWSTR  InfSectionName

    );

typedef
ULONG
(WINAPI *PFN_WDFPREDEVICEREMOVE)(
    __in LPCWSTR  InfPath,
    __in_opt LPCWSTR  InfSectionName
    );

//----------------------------------------------------------------------------
// To be called after (your) WDF driver is removed.
//----------------------------------------------------------------------------
ULONG
WINAPI
WdfPostDeviceRemove(
    __in LPCWSTR  InfPath,
    __in_opt LPCWSTR  InfSectionName
    );

typedef
ULONG
(WINAPI *PFN_WDFPOSTDEVICEREMOVE)(
    __in LPCWSTR  InfPath,
    __in_opt LPCWSTR  InfSectionName

    );



#endif // (NTDDI_VERSION >= NTDDI_WIN2K)


#endif // _WDFINSTALLER_H_

