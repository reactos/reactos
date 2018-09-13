//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       dllmain.cpp
//
//  Contents:   DllMain entry point
//
//  History:    08-May-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include <stdpch.h>

//
// Module instance
//

HINSTANCE g_hModule = NULL;
//+---------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   Windows DLL entry point
//
//  Arguments:  [hInstance]  -- module instance
//              [dwReason]   -- reason code
//              [pvReserved] -- reserved
//
//  Returns:    TRUE if everything ok, FALSE otherwise
//
//  Notes:
//
//----------------------------------------------------------------------------
extern "C" BOOL WINAPI
TrustUIDllMain (HINSTANCE hInstance, DWORD dwReason, LPVOID pvReserved)
{
    switch ( dwReason )
    {
    case DLL_PROCESS_ATTACH:

        //
        // Keep the module instance handle for resource loading usage
        //

        g_hModule = hInstance;

        //
        // Initialize rich edit control DLL
        //

       /* if ( LoadLibrary(TEXT("riched32.dll")) == NULL )
        {
            return( FALSE );
        }*/

        //
        // Initialize the common controls
        //

        InitCommonControls();
        break;
    case DLL_PROCESS_DETACH:
        break;
    }

    return( TRUE );
}


