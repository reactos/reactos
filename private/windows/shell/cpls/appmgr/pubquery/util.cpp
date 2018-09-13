/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    util.cpp

Abstract:

    This module contains utility functions.

Author:

    Dave Hastings (daveh) creation-date 11-Nov-1997

Revision History:


--*/
#include "pubquery.h"

HRESULT
HandleQueryInterface(
    REFIID riid,
    PVOID *ppvInterface,
    INTERFACES *Interfaces,
    ULONG ArraySize
    )
/*++

Routine Description:

    This routine is a standard implementation of QueryInterface.

Arguments:

    riid -- Supplies the IID of the interface to find.
    ppvInterface -- Returns the desired interface pointer.
    Interfaces -- Supplies the IIDs and interface pointers for this object.
    ArraySize -- Supplies the size of the array interface

Return Value:

    
--*/
{
    HRESULT hr = S_OK;
    INT i;

    if ((ppvInterface == NULL) || (Interfaces == NULL) || (ArraySize == 0)) {
        hr = E_INVALIDARG;
        goto exit_gracefully;
    }

    *ppvInterface = NULL;

    for (i = 0; i < ArraySize; i++) {

        if (IsEqualIID(riid, *Interfaces[i].piid)) {
            *ppvInterface = Interfaces[i].pvObject;
            goto exit_gracefully;
        }
    }

    hr = E_NOINTERFACE;

exit_gracefully:

    if (SUCCEEDED(hr)) {
        ((LPUNKNOWN)*ppvInterface)->AddRef();
    }

    return hr;
}

VOID
PopulateListView(
    HWND ListView,
    PACKAGEDISPINFO *Packages,
    ULONG NumberOfPackages
    )
/*++

Routine Description:

    This routine puts the information about the applications
    into the list view.

Arguments:

    ListView -- Supplies the window handle of the listview.
    Packages -- Supplies the list of packages.
    NumberOfPackages -- Supplies the number of packages in the list.

Return Value:

    None.

--*/
{
    ULONG i;
    PACKAGEDISPINFO *PackageInfo;
    LV_ITEM Item;
    LONG Index;

    for (i = 0; i < NumberOfPackages; i++) {

        //
        // Keep the information about this app for later
        //
        PackageInfo = (PACKAGEDISPINFO *)CoTaskMemAlloc(sizeof(PACKAGEDISPINFO) + 20);

        *PackageInfo = Packages[i];

        wsprintf(
            (PWCHAR)((PCHAR)PackageInfo + sizeof(PACKAGEDISPINFO)),
            L"%d.%d",
            Packages[i].dwVersionHi,
            Packages[i].dwVersionLo
            );

        //
        // Insert the product in the list
        //
        Item.mask = LVIF_TEXT | LVIF_PARAM;
        Item.iItem = 0;
        Item.iSubItem = 0;
        Item.pszText = Packages[i].pszPackageName;
        Item.lParam = (LPARAM)PackageInfo;
        Index = ListView_InsertItem(ListView, &Item);

        //
        // Insert the version number
        //
        if (Index == -1) {
            //
            // Didn't insert the item bugbug report error?
            //
            CoTaskMemFree(PackageInfo);
            continue;
        }


        Item.mask = LVIF_TEXT;
        Item.iItem = Index;
        Item.iSubItem = 1;
        Item.pszText = (PWCHAR)((PCHAR)PackageInfo + sizeof(PACKAGEDISPINFO));
        ListView_SetItem(ListView, &Item);
    }
}

