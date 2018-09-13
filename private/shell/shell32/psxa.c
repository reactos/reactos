///////////////////////////////////////////////////////////////////////////////
//
// PSXA.C   --   PropSheetExtArray implementation
//
// Manipulates a group of property sheet extension objects (see PSXA.H)
//
///////////////////////////////////////////////////////////////////////////////

#include "shellprv.h"
#pragma  hdrstop

//-----------------------------------------------------------------------------
//
// PSXA
//
// header for an array of IShellPropSheetExt interface pointers
//
//-----------------------------------------------------------------------------

typedef struct
{
    UINT count, alloc;
    IShellPropSheetExt *interfaces[ 0 ];
} PSXA;


//-----------------------------------------------------------------------------
//
// _PSXACALLINFO
//
// used to forward LPFNADDPROPSHEETPAGE calls with added error checking
//
//-----------------------------------------------------------------------------

typedef struct
{
    LPFNADDPROPSHEETPAGE pfn;
    LPARAM lparam;
    UINT count;
    BOOL allowmulti;
    BOOL alreadycalled;
} _PSXACALLINFO;


//-----------------------------------------------------------------------------
//
// _PsxaCallOwner
//
// forwards an LPFNADDPROPSHEETPAGE call with added error checking
//
//-----------------------------------------------------------------------------

BOOL CALLBACK _PsxaCallOwner( HPROPSHEETPAGE hpage, LPARAM lparam )
{
    _PSXACALLINFO *callinfo = (_PSXACALLINFO *)lparam;
    if( callinfo )
    {
        if( !callinfo->allowmulti && callinfo->alreadycalled )
            return FALSE;

        if( callinfo->pfn( hpage, callinfo->lparam ) )
        {
            callinfo->alreadycalled = TRUE;
            callinfo->count++;
            return TRUE;
        }
    }
    return FALSE;
}


//-----------------------------------------------------------------------------
//
// InitPropSheetExt
//
// creates an instance of the property sheet extension referred to by szCLSID
// initializes it via the IShellExtInit (if IShellExtInit is supported)
//
//-----------------------------------------------------------------------------

BOOL InitPropSheetExt(IShellPropSheetExt **ppspx, LPCTSTR pszCLSID, HKEY hKey, IDataObject *pDataObj)
{
    if (SUCCEEDED(SHExtCoCreateInstance( pszCLSID, NULL, NULL, &IID_IShellPropSheetExt, ppspx)))
    {
        IShellExtInit *psxi;

        if( SUCCEEDED( (*ppspx)->lpVtbl->QueryInterface( *ppspx,
            &IID_IShellExtInit, &psxi ) ) )
        {
            if( !SUCCEEDED( psxi->lpVtbl->Initialize( psxi, NULL, pDataObj, hKey ) ) )
            {
                (*ppspx)->lpVtbl->Release( *ppspx );
                *ppspx = NULL;
            }

            psxi->lpVtbl->Release( psxi );
        }
    }

    return BOOLFROMPTR(*ppspx);
}


//-----------------------------------------------------------------------------
//
// SHCreatePropSheetExtArray
//
// uses hKey and pszSubKey to find property sheet handlers in the registry
// loads up to max_iface IShellPropSheetExt interfaces (so I'm lazy...)
// returns a handle (pointer) to a newly allocated PSXA
//
//-----------------------------------------------------------------------------

HPSXA SHCreatePropSheetExtArrayEx( HKEY hKey, LPCTSTR pszLocation, UINT max_iface, IDataObject * pDataObj)
{
    BOOL success = FALSE;

    PSXA *psxa =
        LocalAlloc( LPTR, SIZEOF( PSXA ) +
        SIZEOF( IShellPropSheetExt * ) * max_iface );

    if( psxa )
    {
        IShellPropSheetExt **spsx = psxa->interfaces;
        HKEY hkLocation;
        UINT i;

        psxa->count = 0;
        psxa->alloc = max_iface;

        for( i = 0; i < psxa->alloc; i++, spsx++ )
            *spsx = NULL;

        if( RegOpenKey( hKey, pszLocation, &hkLocation ) == ERROR_SUCCESS )
        {
            HKEY hkHandlers;

            if( RegOpenKey( hkLocation, c_szPropSheet, &hkHandlers ) ==
                ERROR_SUCCESS )
            {
                TCHAR szChild[ 64 ]; // yes, this is totally arbitrary...

                // fill until there's no room or no more subkeys to get
                for( i = 0;
                    ( psxa->count < psxa->alloc ) &&
                    ( RegEnumKey( hkHandlers, (int)i, szChild,
                    ARRAYSIZE( szChild ) ) == ERROR_SUCCESS );
                    i++ )
                {
                    TCHAR szCLSID[ MAX_PATH ];
                    LONG len_szCLSID = ARRAYSIZE( szCLSID );

                    if( SHRegQueryValue( hkHandlers, szChild, szCLSID,
                        &len_szCLSID ) == ERROR_SUCCESS )
                    {
                        if( InitPropSheetExt( &psxa->interfaces[ psxa->count ],
                            szCLSID, hKey, pDataObj) )
                        {
                            psxa->count++;
                        }
                    }
                }

                RegCloseKey( hkHandlers );
                success = TRUE;
            }

            RegCloseKey( hkLocation );
        }
    }

    if( !success )
    {
        SHDestroyPropSheetExtArray( (HPSXA)psxa );
        psxa = NULL;
    }

    return (HPSXA)psxa;
}

HPSXA SHCreatePropSheetExtArray( HKEY hKey, LPCTSTR pszLocation, UINT max_iface )
{
    return SHCreatePropSheetExtArrayEx( hKey, pszLocation, max_iface, NULL );
}

//-----------------------------------------------------------------------------
//
// SHDestroyPropSheetExtArray
//
// releases interfaces in a PSXA and frees the memory it occupies
//
//-----------------------------------------------------------------------------

void SHDestroyPropSheetExtArray( HPSXA hpsxa )
{
    PSXA *psxa = (PSXA *)hpsxa;
    IShellPropSheetExt **spsx = psxa->interfaces;
    UINT i;

    // release the interfaces
    for( i = 0; i < psxa->count; i++, spsx++ )
        (*spsx)->lpVtbl->Release( *spsx );

    LocalFree( psxa );
}


//-----------------------------------------------------------------------------
//
// SHAddFromPropSheetExtArray
//
// asks each interface in a PSXA to add pages for a proprty sheet
// returns the number of pages actually added
//
//-----------------------------------------------------------------------------

UINT SHAddFromPropSheetExtArray( HPSXA hpsxa, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam )
{
    PSXA *psxa = (PSXA *)hpsxa;
    IShellPropSheetExt **spsx = psxa->interfaces;
    _PSXACALLINFO callinfo = { lpfnAddPage, lParam, 0, TRUE, FALSE };
    UINT i;

    for( i = 0; i < psxa->count; i++, spsx++ )
        (*spsx)->lpVtbl->AddPages( *spsx, _PsxaCallOwner, (LPARAM)&callinfo );

    return callinfo.count;
}


//-----------------------------------------------------------------------------
//
// SHReplaceFromPropSheetExtArray
//
// asks each interface in a PSXA to replace a page in a prop sheet
// each interface is only allowed to add up to one replacement
// returns the total number of replacements added
//
//-----------------------------------------------------------------------------

UINT SHReplaceFromPropSheetExtArray(HPSXA hpsxa, UINT uPageID,
                                    LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam)
{
    PSXA *psxa = (PSXA *)hpsxa;
    IShellPropSheetExt **spsx = psxa->interfaces;
    _PSXACALLINFO callinfo = { lpfnReplaceWith, lParam, 0, FALSE, FALSE };
    UINT i;

    for( i = 0; i < psxa->count; i++, spsx++ )
    {
        // reset the call flag so that each provider gets a chance
        callinfo.alreadycalled = FALSE;

        if ((*spsx)->lpVtbl->ReplacePage)
            (*spsx)->lpVtbl->ReplacePage( *spsx, uPageID, _PsxaCallOwner, (LPARAM)&callinfo );
    }

    return callinfo.count;
}
