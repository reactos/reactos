/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    admin.c

Abstract:

    This module contians helper routines for dealing with
    administered users (finding the GPT path, getting published
    apps etc).

Author:

    Dave Hastings (daveh) creation-date 06-May-1997

Revision History:

--*/

#include <windows.h>
#include <userenv.h>
#include <msi.h>
#include "resource.h"
#include "pip.h"
#include "appmgr.h"
#include "cstore.h"

//
// The following is used to disable features according to 
// policy applied by the administrator.
//
ULONG FeatureMask;

HRESULT
GetPublishedApps(
    PACKAGEDISPINFO *Packages,
    PULONG NumberOfPackages
	)
/*++

Routine Description:

    This function finds all of the published application packages, and
	extracts the product name and package path.

Arguments:

    IEnumPackages -- Supplies the package enumeration interface
    Packages -- Returns the package information
    NumberOfPackages -- Supplies the size of the array/returns the number of packages

Return Value:

    TRUE for success.

--*/
{
    ULONG PackagesToGet;
    HRESULT hr;
    IEnumPackage *IEnumPackage;

    PackagesToGet = *NumberOfPackages;
    *NumberOfPackages = 0;

    hr = CsEnumApps(
        NULL,
        NULL,
        NULL,
        APPINFO_PUBLISHED | APPINFO_VISIBLE | APPINFO_MSI,
        &IEnumPackage
        );

    if (!SUCCEEDED(hr)) {
        goto exit;
    }

    hr = IEnumPackage->lpVtbl->Next(
        IEnumPackage, 
        PackagesToGet, 
        Packages, 
        NumberOfPackages
        );

    IEnumPackage->lpVtbl->Release(IEnumPackage);

exit:
    return hr;
}

BOOL
InitializePolicy(
    VOID
    )
/*++

Routine Description:

    This routine gets the policy information from the registry for the 
    programs wizard.

Arguments:

    None

Return Value:

    TRUE if the policy information was retrieved from the registry
    FALSE for an error (not finding any policy information is not an error)

--*/
{
    HKEY PolicyKey;
    LONG Result;
    DWORD Type;
    DWORD BufferSize;

    //
    // Open our policy key in the registry
    //
    Result = RegOpenKeyEx(
        HKEY_CURRENT_USER, 
        L"Software\\Policies\\Windows\\Programs Wizard",
        0,
        KEY_READ,
        &PolicyKey
        );

    if (Result != ERROR_SUCCESS) {
        //
        // Bugbug -- need to determine the apropriate error codes.
        //           for now, we just accept all errors as indications
        //           that policy is not set
        //
        return TRUE;
    }

    //
    // Get the feature mask from the registry
    //
    BufferSize = sizeof(ULONG);
    Result = RegQueryValueEx(
        PolicyKey,
        L"FeatureMask",
        NULL,
        &Type,
        (PBYTE)&FeatureMask,
        &BufferSize
        );

    if ((Result == ERROR_MORE_DATA) || (Type != REG_DWORD)) {
        return FALSE;
    }
}       
