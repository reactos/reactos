/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    pip95.c

Abstract:

    This module contains code for enumerating applications 
    listed in the Logo95/Logo3 uninstall registry entries.

Author:

    Dave Hastings (daveh) creation-date-01-May-1997

Notes:

    This should move to a more sensible place.  It doesn't
    really belong here.

Revision History:

    
--*/

#include <windows.h>
// bugbug
#include <shlwapi.h>
#include "pip.h"
#include "resource.h"

typedef struct _PipInfo95 {
    HKEY Key;
    DWORD EnumIndex;
} PIPINFO95, *PPIPINFO95;

typedef struct Logo95Info {
    PWSTR UninstallString;
    PWSTR ModifyPath;
    PWSTR RegKeyName;
} LOGO95INFO, *PLOGO95INFO;


LONG
ReadAndExpandRegString(
    HKEY RegKey,
    HANDLE Heap,
    PWCHAR ValueName,
    PWCHAR *Value
    );


BOOL
Pip95Uninstall(
    HANDLE Information
    );

BOOL
Pip95Modify(
    HANDLE Information
    );

VOID
Pip95FreeHandle(
    HANDLE Handle
    );

VOID 
Pip95GetProperties(
    HANDLE Handle,
    PPIPAPPLICATIONPROPERTIES Properties
    );

WCHAR UnknownVersion[25];

// bugbug
extern HINSTANCE Instance;

HANDLE
PipInitialize(
    VOID
    )
/*++

Routine Description:

    This routine performs initialization for enumerating Logo95/Logo3
    applications.

Arguments:

    None

Return Value:

    The pointer to the PipInfo95 structure for success, INVALID_HANDLE_VALUE for failure

--*/
{
    PPIPINFO95 PipInfo;
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
    PipInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PIPINFO95));

    if (PipInfo == NULL) {
        return INVALID_HANDLE_VALUE;
    }

    //
    // Open the uninstall key in the registry
    //
    RetVal = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE, 
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        0,
        KEY_READ,
        &(PipInfo->Key)
        );

    if (RetVal != ERROR_SUCCESS) {
        HeapFree(GetProcessHeap(), 0, PipInfo);
        return INVALID_HANDLE_VALUE;
    }

    return (HANDLE)PipInfo;
}

ULONG
PipFindNextProduct(
	HANDLE IN Handle,
	PULONG OUT Capabilities,
	PPIPACTIONS OUT Actions,
	PPIPAPPLICATIONINFORMATION IN OUT ApplicationInformation
	)
/*++

Routine Description:

    This routine enumerates information about Logo95/Logo3 applications.

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
    WCHAR KeyName[256];
    ULONG KeyNameSize;
    WCHAR ProductName[256];
    ULONG ProductNameSize;
    PWCHAR UninstallString = NULL;
    PWCHAR ModifyPathString = NULL;
	BOOL ProductNamePresent;
	BOOL UninstallStringPresent;
	BOOL DarwinApp;
    FILETIME FileTime;
    PPIPINFO95 PipInfo;
    DWORD Type;
    PLOGO95INFO Logo95Info;
    ULONG Caps = 0;
    HANDLE Heap;
    
    PipInfo = (PPIPINFO95)Handle;
    Heap = GetProcessHeap();

TryTheNextOne:
	ProductNamePresent = FALSE;
	UninstallStringPresent = FALSE;

    //
    // Find the next product subkey
    //
    KeyNameSize = 256;
    RetVal = RegEnumKeyEx(
        PipInfo->Key,
        PipInfo->EnumIndex,
        KeyName,
        &KeyNameSize,
        NULL,
        NULL,
        NULL,
        &FileTime
        );

    PipInfo->EnumIndex++;

    if (RetVal != ERROR_SUCCESS) {
        return RetVal;
    }

    //
    // Open the key and get the product name
    //
    RetVal = RegOpenKeyEx(
        PipInfo->Key,
        KeyName,
        0,
        KEY_READ,
        &SubKey
        );

    if (RetVal != ERROR_SUCCESS) {
        return RetVal;
    }

    //
    // Get the DisplayName value
    //

    // bugbug
    ProductNameSize = ApplicationInformation->ProductNameSize;

    RetVal = RegQueryValueEx(
        SubKey,
        L"DisplayName",
        0,
        &Type,
        (PBYTE)ProductName,
        &ProductNameSize
        );


    if (RetVal == ERROR_FILE_NOT_FOUND) {
        goto TryTheNextOne;
        RegCloseKey(SubKey);
    }


    if (RetVal != ERROR_SUCCESS) {
        return RetVal;
    }

    ApplicationInformation->ProductNameSize = ProductNameSize;
    lstrcpy(ApplicationInformation->ProductName, ProductName);

    //
    // Get the version
    //
    // bugbug for now, unknown is the only answer
    lstrcpy(ApplicationInformation->InstalledVersion, UnknownVersion);

    //
    // Get the uninstaller string
    //

    RetVal = ReadAndExpandRegString(
        SubKey,
        Heap,
        L"UninstallString",
        &UninstallString
        );


    if (RetVal != ERROR_SUCCESS) {

        //
        // If there is no uninstall string, the we just try to 
        // find the ModifyPath
        //
        if (RetVal != ERROR_FILE_NOT_FOUND) {
            goto cleanup;
        }

    } else {

		if (StrStrI(UninstallString, L"msiexec")) {
			//
			// This is a repackaged darwin app
			// bugbug nls, will msiexec name ever change?
			//
            HeapFree(Heap, 0, UninstallString);
			goto TryTheNextOne;
		}

        Caps = PIP_UNINSTALL;
    }
    
    //
    // Get the ModifyPath
    //

    RetVal = ReadAndExpandRegString(
        SubKey,
        Heap,
        L"ModifyPath",
        &ModifyPathString
        );

    if (RetVal != ERROR_SUCCESS) {
        //
        // If there is no ModifyPath, we just continue
        //
        if (RetVal != ERROR_FILE_NOT_FOUND) {
            goto cleanup;
        }
    } else {

        Caps |= PIP_MODIFY;

    }

    if (Caps == 0) {
        ApplicationInformation->h = NULL;
    } else {

        //
        // Allocate space for the structures and store the info
        //

        Logo95Info = HeapAlloc(
            Heap,
            0,
            sizeof(LOGO95INFO)
            );

        ApplicationInformation->h = (HANDLE)Logo95Info;
        Logo95Info->UninstallString = UninstallString;
        UninstallString = NULL;
        Logo95Info->ModifyPath = ModifyPathString;
        ModifyPathString = NULL;
        Logo95Info->RegKeyName = HeapAlloc(
            Heap,
            0,
            (KeyNameSize + 1) * sizeof(WCHAR)
            );
        lstrcpy(Logo95Info->RegKeyName, KeyName);
        Actions->Uninstall = Pip95Uninstall;
        Actions->Modify = Pip95Modify;
        Actions->GetProperties = Pip95GetProperties;

    }

    Actions->FreeHandle = Pip95FreeHandle;
    *Capabilities = Caps;
    RetVal = ERROR_SUCCESS;

cleanup:

    if (ModifyPathString != NULL) {
        HeapFree(
            Heap,
            0,
            ModifyPathString
            );
    }

    if (UninstallString != NULL) {
        HeapFree(
            Heap,
            0,
            UninstallString
            );
    }

    return RetVal;

#if 0
	if (RetVal != ERROR_SUCCESS) {
        *Capabilities = 0;
        ApplicationInformation->h = NULL;
        if (RetVal != ERROR_FILE_NOT_FOUND) {
            goto cleanup;
        }

    } else {

		if (StrStrI(UninstallString, L"msiexec")) {
			//
			// This is a repackaged darwin app
			// bugbug nls, will msiexec name ever change?
			//
            HeapFree(GetProcessHeap(), 0, UninstallString);
			goto TryTheNextOne;
		}
         *
    }

    RetVal = ReadAndExpandRegString(
        SubKey,
        GetProcessHeap(),
        L"ModifyPath",
        &ModifyPath
        );

	if (RetVal != ERROR_SUCCESS) {
        *Capabilities = 0;
        ApplicationInformation->h = NULL;
        if (RetVal != ERROR_FILE_NOT_FOUND) {
            goto cleanup;
        }

    } else {


        *Capabilities = PIP_UNINSTALL;

        Logo95Info = HeapAlloc(
            GetProcessHeap(),
            0,
            sizeof(LOGO95INFO)
            );
        ApplicationInformation->h = (HANDLE)Logo95Info;
        Logo95Info->UninstallString = HeapAlloc(
            GetProcessHeap(),
            0,
            UninstallStringSize * sizeof(WCHAR)
            );

        Logo95Info->RegKeyName = HeapAlloc(
            GetProcessHeap(),
            0,
            (KeyNameSize + 1) * sizeof(WCHAR)
            );
        lstrcpy(Logo95Info->RegKeyName, KeyName);
        Actions->Uninstall = Pip95Uninstall;
        Actions->GetProperties = Pip95GetProperties;
    }

    Actions->FreeHandle = Pip95FreeHandle;

    return ERROR_SUCCESS;
#endif
}

BOOL
PipUninitialize(
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
    PPIPINFO95 PipInfo;
    
    PipInfo = (PPIPINFO95)Handle;

    RegCloseKey(PipInfo->Key);

    HeapFree(GetProcessHeap(), 0, PipInfo);

    return TRUE;
}

BOOL
Pip95Uninstall(
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
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    BOOL RetVal;
    PLOGO95INFO Logo95Info;
    HANDLE WaitHandles[1];
    DWORD WaitRetVal;
    LONG MsgRetVal;
    MSG Msg;
    
    Logo95Info = (PLOGO95INFO)Information;
    StartupInfo.cb = sizeof(STARTUPINFO);
    StartupInfo.lpReserved = NULL;
    StartupInfo.lpDesktop = NULL;
    StartupInfo.lpTitle = NULL;
    StartupInfo.dwFlags = 0;
    StartupInfo.cbReserved2 = 0;
    StartupInfo.lpReserved2 = NULL;
    StartupInfo.wShowWindow = SW_SHOWNORMAL;

    RetVal = CreateProcess(
        NULL,
        Logo95Info->UninstallString,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &StartupInfo,
        &ProcessInformation
        );

    //
    // Wait for the uninstall to finish.  Otherwise, when we
    // update the listbox of installed apps, it looks like the
    // one we're uninstalling is still installed
    //
    WaitHandles[0] = ProcessInformation.hProcess;
    
    do {
        WaitRetVal = MsgWaitForMultipleObjects(
            1,
            WaitHandles,
            FALSE,
            INFINITE,
            QS_ALLEVENTS
            );

        if (WaitRetVal == WAIT_OBJECT_0 + 1) {
            //
            // Got a paint message
            //
            MsgRetVal = GetMessage(&Msg, NULL, 0, 0);
            if ((MsgRetVal) && (MsgRetVal != -1)) {
                DispatchMessage(&Msg);
            }
        }
    } while ((WaitRetVal != WAIT_OBJECT_0) && (WaitRetVal != WAIT_ABANDONED_0));

    CloseHandle(ProcessInformation.hProcess);
    CloseHandle(ProcessInformation.hThread);

    return RetVal;
}

BOOL
Pip95Modify(
    HANDLE Information
    )
/*++

Routine Description:

    This routine modifies the application described by Information.

Arguments:

    Information - Supplies the information to modify the application.

Return Value:

    TRUE for success.

--*/
{
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    BOOL RetVal;
    PLOGO95INFO Logo95Info;
    
    Logo95Info = (PLOGO95INFO)Information;
    StartupInfo.cb = sizeof(STARTUPINFO);
    StartupInfo.lpReserved = NULL;
    StartupInfo.lpDesktop = NULL;
    StartupInfo.lpTitle = NULL;
    StartupInfo.dwFlags = 0;
    StartupInfo.cbReserved2 = 0;
    StartupInfo.lpReserved2 = NULL;
    StartupInfo.wShowWindow = SW_SHOWNORMAL;

    RetVal = CreateProcess(
        NULL,
        Logo95Info->ModifyPath,
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

VOID
Pip95FreeHandle(
    HANDLE Handle
    )
/*++

Routine Description:

    This function frees the type specific information for
    logo95 apps.

Arguments:

    Handle - Supplies the pointer to the data to free.

Return Value:

    None.

--*/
{
    HeapFree(
        GetProcessHeap(),
        0,
        Handle
        );
}

VOID 
Pip95GetProperties(
    HANDLE Handle,
    PPIPAPPLICATIONPROPERTIES Properties
    )
{
    WCHAR Buffer[256];
    PLOGO95INFO Logo95Info;
    HKEY RegKey, SubKey;
    LONG RetVal;
    DWORD Type;

    Logo95Info = (PLOGO95INFO)Handle;

    //
    // Initialize the properties structure
    //
    lstrcpy(Properties->ProductName, L"");
    lstrcpy(Properties->ProductVersion, L"");
    lstrcpy(Properties->Publisher, L"");
    lstrcpy(Properties->ProductID, L"");
    lstrcpy(Properties->RegisteredOwner, L"");
    lstrcpy(Properties->RegisteredCompany, L"");
    lstrcpy(Properties->Language, L"");
    lstrcpy(Properties->SupportUrl, L"");
    lstrcpy(Properties->SupportTelephone, L"");
    lstrcpy(Properties->HelpFile, L"");
    lstrcpy(Properties->InstallLocation, L"");
    lstrcpy(Properties->InstallSource, L"");
    lstrcpy(Properties->RequiredByPolicy, L"");
    lstrcpy(Properties->AdministrativeContact, L"");

    //
    // Open the uninstall key in the registry
    //
    RetVal = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE, 
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        0,
        KEY_READ,
        &RegKey
        );

    if (RetVal != ERROR_SUCCESS) {
        return;
    }

    //
    // Open the proper subkey
    //
    RetVal = RegOpenKeyEx(
        RegKey,
        Logo95Info->RegKeyName,
        0,
        KEY_READ,
        &SubKey
        );

    RegCloseKey(RegKey);

    if (RetVal != ERROR_SUCCESS) {
        return;
    }

    //
    // Get the values we're looking for
    //
    RegQueryValueEx(
        SubKey,
        L"DisplayName",
        0,
        &Type,
        (LPBYTE)Properties->ProductName,
        &(Properties->ProductNameSize)
        );

    RegQueryValueEx(
        SubKey,
        L"Publisher",
        0,
        &Type,
        (LPBYTE)Properties->Publisher,
        &(Properties->PublisherSize)
        );

    RegQueryValueEx(
        SubKey,
        L"ProductID",
        0,
        &Type,
        (LPBYTE)Properties->ProductID,
        &(Properties->ProductIDSize)
        );

    RegQueryValueEx(
        SubKey,
        L"RegOwner",
        0,
        &Type,
        (LPBYTE)Properties->RegisteredOwner,
        &(Properties->RegisteredOwnerSize)
        );

    RegQueryValueEx(
        SubKey,
        L"RegCompany",
        0,
        &Type,
        (LPBYTE)Properties->RegisteredCompany,
        &(Properties->RegisteredCompanySize)
        );

    RegQueryValueEx(
        SubKey,
        L"HelpTelephone",
        0,
        &Type,
        (LPBYTE)Properties->SupportTelephone,
        &(Properties->SupportTelephoneSize)
        );

    RegQueryValueEx(
        SubKey,
        L"InstallLocation",
        0,
        &Type,
        (LPBYTE)Properties->InstallLocation,
        &(Properties->InstallLocationSize)
        );

    RegQueryValueEx(
        SubKey,
        L"InstallSource",
        0,
        &Type,
        (LPBYTE)Properties->InstallSource,
        &(Properties->InstallSourceSize)
        );
}