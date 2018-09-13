/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    pipdar.c

Abstract:

    This module contains the PIP code for darwin.

Author:

    Dave Hastings (daveh) creation-date 06-Jun-1997

Revision History:

--*/
#include <windows.h>
// bugbug
#include <shlwapi.h>
#include <msi.h>
#include <appmgmt.h>
#include "pip.h"
#include "resource.h"

typedef struct _PipInfoDar {
    DWORD EnumIndex;
} PIPINFODAR, *PPIPINFODAR;

typedef struct _DarwinAppInfo {
    PWCHAR ProductId;
    PWCHAR UpgradeUrl;
} DARWINAPPINFO, *PDARWINAPPINFO;

BOOL
PipDarUninstall(
    HANDLE Information
    );

BOOL
PipDarModify(
    HANDLE Information
    );

BOOL
PipDarRepair(
    HANDLE Information,
    DWORD Action
    );

BOOL
PipDarUpgrade(
    HANDLE Information
    );

VOID
PipDarFreeHandle(
    HANDLE Handle
    );

WCHAR UnknownVersion[25];

// bugbug
extern HINSTANCE Instance;

HANDLE
PipDarwinInitialize(
    VOID
    )
/*++

Routine Description:

    This routine performs initialization for enumerating Darwin
    applications.

Arguments:

    None

Return Value:

    The pointer to the PipInfoDar structure for success, INVALID_HANDLE_VALUE for failure

--*/
{
    PPIPINFODAR PipInfo;
    LONG RetVal;

    //
    // bugbug maybe in dll attach routine?
    //
    //
    // Load our strings
    //
    LoadString(Instance, IDS_UNKNOWN_VERSION, UnknownVersion, 25);

    //
    // Allocate our structure and initialize (NB. We have to zero the memory
    // to get EnumIndex properly initialized)
    //
    PipInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PIPINFODAR));

    if (PipInfo == NULL) {
        return INVALID_HANDLE_VALUE;
    }

    return (HANDLE)PipInfo;
}

ULONG
PipDarwinFindNextProduct(
        HANDLE IN Handle,
        PULONG OUT Capabilities,
        PPIPACTIONS OUT Actions,
        PPIPAPPLICATIONINFORMATION IN OUT ApplicationInformation
        )
/*++

Routine Description:

    This routine enumerates information about Darwin applications.

Arguments:

    Handle - Supplies a pointer to the enumeration data.
    Capabilities - Returns the capabilities flag for this application.
    ApplicationInformation - Returns information about the installed produc.

Return Value:

    ERROR_SUCCESS for Success, ERROR_NO_MORE_ITEMS when all products have been
    enumerated.

Notes:
        //bugbug -- Let's not trash the parameters unless we really succeeded!

--*/
{
    LONG RetVal;
    HKEY SubKey;
    WCHAR ProductName[256];
    ULONG ProductNameSize;
    PPIPINFODAR PipInfo;
    WCHAR ProductId[39];
    PDARWINAPPINFO DarwinInfo;
    WCHAR KeyName[MAX_PATH];
    WCHAR UrlString[MAX_PATH];
    DWORD UrlStringLength;
    DWORD ValueType;

    PipInfo = (PPIPINFODAR)Handle;

    RetVal = MsiEnumProducts(
        PipInfo->EnumIndex,
        ProductId
        );

    if (RetVal != ERROR_SUCCESS) {
        return RetVal;
    }

    //
    // Get the information from the ProductId
    //
    ProductNameSize = 256;
    RetVal = MsiGetProductInfo(
        ProductId,
        INSTALLPROPERTY_PRODUCTNAME,
        ProductName,
        &ProductNameSize
        );

    *Capabilities = 0;

    //
    // Fill in the return structure
    //
    ApplicationInformation->ProductNameSize = ProductNameSize;
    lstrcpy(ApplicationInformation->ProductName, ProductName);

    //
    // Get the version
    //
    // bugbug for now, unknown is the only answer
    lstrcpy(ApplicationInformation->InstalledVersion, UnknownVersion);

    //
    // Return the product code as the app specific info
    //
    DarwinInfo = HeapAlloc(
        GetProcessHeap(),
        0,
        sizeof(DARWINAPPINFO)
        );

    DarwinInfo->ProductId = HeapAlloc(
        GetProcessHeap(),
        0,
        39 * sizeof(WCHAR)
        );
    lstrcpy(DarwinInfo->ProductId, ProductId);

    ApplicationInformation->h = (HANDLE)DarwinInfo;

    *Capabilities |= PIP_UNINSTALL | PIP_MODIFY | PIP_REPAIR;
    Actions->Uninstall = PipDarUninstall;
    Actions->Modify = PipDarModify;
    Actions->Repair = PipDarRepair;

    //
    // Move on to the next app
    //
    PipInfo->EnumIndex++;

    //
    // N.B.  All information obtained after this point is
    // optional.  Failure to determine a piece of information
    // will result in success, but that piece of information won't
    // be returned
    //

    //
    // Look up upgrade URL in registry
    //

    lstrcpy(KeyName, L"software\\Installer\\Products\\");
    lstrcat(KeyName, ProductId);

    RetVal = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        KeyName,
        0,
        KEY_READ,
        &SubKey
        );

    if (RetVal != ERROR_SUCCESS) {
        return ERROR_SUCCESS;
    }

    UrlStringLength = MAX_PATH * sizeof(WCHAR);
    RetVal = RegQueryValueEx(
        SubKey,
        L"URLUpdateInfo",
        NULL,
        &ValueType,
        (PBYTE)UrlString,
        &UrlStringLength
        );

    RegCloseKey(SubKey);

    if (RetVal != ERROR_SUCCESS) {
        return ERROR_SUCCESS;
    }

    DarwinInfo->UpgradeUrl = HeapAlloc(
        GetProcessHeap(),
        0,
        (lstrlen(UrlString) + 1) * sizeof(WCHAR)
        );

    lstrcpy(DarwinInfo->UpgradeUrl, UrlString);
    Actions->Upgrade = PipDarUpgrade;
    *Capabilities |= PIP_UPGRADE;
    Actions->FreeHandle = PipDarFreeHandle;

    return ERROR_SUCCESS;
}

BOOL
PipDarwinUninitialize(
        HANDLE IN Handle
        )
/*++

Routine Description:

    This function closes all handles, and frees all memory associated with
    the enumeration.

Arguments:

    Handle - Supplies a pointer to the PipInfo.

Return Value:

    TRUE for success

--*/
{
    PPIPINFODAR PipInfo;

    PipInfo = (PPIPINFODAR)Handle;

    HeapFree(GetProcessHeap(), 0, PipInfo);

    return TRUE;
}

BOOL
PipDarUninstall(
    HANDLE Information
    )
/*++

Routine Description:

    This routine uninstalls the application described by Information.

Arguments:

    Information - Supplies the information to unistall the application.

Return Value:

    TRUE for success.

--*/
{
    UINT RetVal;
    PDARWINAPPINFO AppInfo;

    AppInfo = Information;
    RetVal = MsiConfigureProduct(
        AppInfo->ProductId,
        INSTALLLEVEL_DEFAULT,
        INSTALLSTATE_ABSENT
        );

    if (RetVal == ERROR_SUCCESS) {
        // From appmgmt.h.
        UninstallApplication( AppInfo->ProductId );
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
PipDarModify(
    HANDLE Information
    )
/*++

Routine Description:

    This routine start the modify process for a darwin app.

Arguments:

    Information -- Supplies the products code for the application.

Return Value:

    TRUE for success

--*/
{
    WCHAR CommandLine[255];
    BOOL RetVal;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    PDARWINAPPINFO AppInfo;

    AppInfo = Information;

    //
    // Create the command line
    //
    lstrcpy(CommandLine, L"msiexec /C");
    lstrcat(CommandLine, AppInfo->ProductId);

    //
    // Create the process
    //
    StartupInfo.cb = sizeof(STARTUPINFO);
    StartupInfo.lpReserved = NULL;
    StartupInfo.lpDesktop = NULL;
    StartupInfo.lpTitle = NULL;
    StartupInfo.dwFlags = 0;
    StartupInfo.cbReserved2 = 0;
    StartupInfo.lpReserved2 = NULL;

    RetVal = CreateProcess(
        NULL,
        CommandLine,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &StartupInfo,
        &ProcessInformation
        );

    CloseHandle(ProcessInformation.hProcess);
    CloseHandle(ProcessInformation.hThread);

    return RetVal;
}

BOOL
PipDarRepair(
    HANDLE Information,
    DWORD Action
    )
/*++

Routine Description:

    This routine kicks off the darwin application repair process.

Arguments:

    Information - Supplies the Product Code for the application.
    Action - Supplies the actual repair action to perform (PIP_REPAIR_REPAIR,
        or PIP_REPAIR_REINSTALL).

Return Value:

    TRUE for Success

--*/
{
    DWORD ReinstallAction;
    DWORD RetVal;
    PDARWINAPPINFO AppInfo;

    AppInfo = Information;

    if (Action == PIP_REPAIR_REINSTALL) {
        ReinstallAction = REINSTALLMODE_REPAIR |
            REINSTALLMODE_FILEREPLACE |
            REINSTALLMODE_USERDATA |
            REINSTALLMODE_MACHINEDATA |
            REINSTALLMODE_SHORTCUT;
    } else if (Action == PIP_REPAIR_REPAIR) {
        ReinstallAction = REINSTALLMODE_REPAIR |
            REINSTALLMODE_FILEOLDERVERSION |
            REINSTALLMODE_FILEVERIFY |
            REINSTALLMODE_USERDATA |
            REINSTALLMODE_MACHINEDATA |
            REINSTALLMODE_SHORTCUT;
    } else {
        //
        // internal error
        //
#if DBG
        OutputDebugString(L"PIP: Internal error in Darwin repair\n");
#endif
        return FALSE;
    }

    RetVal = MsiReinstallProduct(
        AppInfo->ProductId,
        ReinstallAction
        );

    if (RetVal == ERROR_SUCCESS) {
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
PipDarUpgrade(
    HANDLE Information
    )
/*++

Routine Description:

    This routine start the modify process for a darwin app.

Arguments:

    Information -- Supplies the products code for the application.

Return Value:

    TRUE for success

--*/
{
    PDARWINAPPINFO AppInfo;

    AppInfo = Information;

    ShellExecute(
        NULL,
        NULL,
        AppInfo->UpgradeUrl,
        NULL,
        NULL,
        SW_SHOWDEFAULT
        );

    return TRUE;
}

VOID
PipDarFreeHandle(
    HANDLE Handle
    )
/*++

Routine Description:

    This function frees the type specific information for
    Darwin apps.

Arguments:

    Handle - Supplies the pointer to the data to free.

Return Value:

    None.

--*/
{
    PDARWINAPPINFO DarwinInfo;

    DarwinInfo = (PDARWINAPPINFO)Handle;

    HeapFree(
        GetProcessHeap(),
        0,
        DarwinInfo->ProductId
        );

    if (DarwinInfo->UpgradeUrl) {

        HeapFree(
            GetProcessHeap(),
            0,
            DarwinInfo->UpgradeUrl
            );
    }

    HeapFree(
        GetProcessHeap(),
        0,
        DarwinInfo
        );

}
