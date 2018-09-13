/****************************** Module Header ******************************\
* Module Name: ginamgr.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* GINA Management code
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#if DBG
DWORD   GinaBreakFlags;
#endif

PWLX_ISLOCKOK   IsLockOkFn;


PVOID   BadGinahWlx;
#define BADGINA_REBOOT_SAS  256


VOID
InitBadGina(
    PTERMINAL pTerm );



BOOL
WINAPI
DummyWlxScreenSaverNotify(
    PVOID           pWlxContext,
    BOOL *          Secure)
{
    if ( *Secure && (IsLockOkFn != NULL ) )
    {
        *Secure = IsLockOkFn( pWlxContext );
    }

    return( TRUE );
}

BOOL
WINAPI
DummyWlxNetworkProviderLoad(
    PVOID Context,
    PWLX_MPR_NOTIFY_INFO Info
    )
{
    return FALSE ;
}


BOOL
WINAPI
DummyWlxDisplayStatusMessage(PVOID pContext,
                             HDESK hDesktop,
                             DWORD dwOptions,
                             PWSTR pTitle,
                             PWSTR pMessage)
{
    return TRUE;
}

BOOL
WINAPI
DummyWlxGetStatusMessage(PVOID pContext,
                         DWORD * pdwOptions,
                         PWSTR pMessage,
                         DWORD dwBufferSize)
{
    return TRUE;
}

BOOL
WINAPI
DummyWlxRemoveStatusMessage(PVOID pContext)
{
    return TRUE;
}

//*************************************************************
//
//  LoadGinaDll()
//
//  Purpose:    Loads the gina DLL and negotiates the version number
//
//  Parameters: pTerm      -   Terminal to load the gina for
//              lpGinaName - Gina DLL name
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL LoadGinaDll(
    PTERMINAL   pTerm,
    LPWSTR      lpGinaName)
{
    DWORD dwGinaLevel = 0;
    BOOL bResult;
    HKEY Key ;
    int err ;
    PVOID p;

    //
    // Load the DLL.  If this is not the default MSGINA, poke the dispatch table
    // into the registry.  If it is, delete the key (if present)
    //


    err = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                WINLOGON_KEY,
                0,
                KEY_READ | KEY_WRITE,
                &Key );

    if ( err != 0 )
    {
        return FALSE ;
    }

    if ( (_wcsicmp( lpGinaName, TEXT("msgina.dll") ) != 0 ) &&
         (! pTerm->SafeMode ) )
    {
        p = &WlxDispatchTable ;

        RegSetValueEx(
                Key,
                TEXT("Key"),
                0,
                REG_BINARY,
                (PUCHAR) &p,
                sizeof( PVOID ) );

    }
    else
    {
        RegDeleteValue( Key, TEXT("Key") );
    }


    pTerm->Gina.hInstance = LoadLibraryW(lpGinaName);

    if (!pTerm->Gina.hInstance)
    {
        DebugLog((DEB_ERROR, "Error %d loading Gina DLL %ws\n", GetLastError(), lpGinaName));
        goto LoadGina_ErrorReturn;
    }


    //
    // Get the function pointers
    //

    pTerm->Gina.pWlxNegotiate = (PWLX_NEGOTIATE) GetProcAddress(pTerm->Gina.hInstance, WLX_NEGOTIATE_NAME);
    if (!pTerm->Gina.pWlxNegotiate)
    {
        DebugLog((DEB_ERROR, "Could not find WlxNegotiate entry point\n"));
        goto LoadGina_ErrorReturn;
    }

    pTerm->Gina.pWlxInitialize = (PWLX_INITIALIZE) GetProcAddress(pTerm->Gina.hInstance, WLX_INITIALIZE_NAME);
    if (!pTerm->Gina.pWlxInitialize)
    {
        DebugLog((DEB_ERROR, "Could not find WlxInitialize entry point\n"));
        goto LoadGina_ErrorReturn;

    }

    pTerm->Gina.pWlxDisplaySASNotice = (PWLX_DISPLAYSASNOTICE) GetProcAddress(pTerm->Gina.hInstance, WLX_DISPLAYSASNOTICE_NAME);
    if (!pTerm->Gina.pWlxDisplaySASNotice)
    {
        DebugLog((DEB_ERROR, "Could not find WlxDisplaySASNotice entry point\n"));
        goto LoadGina_ErrorReturn;
    }

    pTerm->Gina.pWlxLoggedOutSAS = (PWLX_LOGGEDOUTSAS) GetProcAddress(pTerm->Gina.hInstance, WLX_LOGGEDOUTSAS_NAME);
    if (!pTerm->Gina.pWlxLoggedOutSAS)
    {
        DebugLog((DEB_ERROR, "Could not find WlxLoggedOutSAS entry point\n"));
        goto LoadGina_ErrorReturn;
    }

    pTerm->Gina.pWlxActivateUserShell = (PWLX_ACTIVATEUSERSHELL) GetProcAddress(pTerm->Gina.hInstance, WLX_ACTIVATEUSERSHELL_NAME);
    if (!pTerm->Gina.pWlxActivateUserShell)
    {
        DebugLog((DEB_ERROR, "Could not find WlxActivateUserShell entry point\n"));
        goto LoadGina_ErrorReturn;
    }

    pTerm->Gina.pWlxLoggedOnSAS = (PWLX_LOGGEDONSAS) GetProcAddress(pTerm->Gina.hInstance, WLX_LOGGEDONSAS_NAME);
    if (!pTerm->Gina.pWlxLoggedOnSAS)
    {
        DebugLog((DEB_ERROR, "Could not find WlxLoggedOnSAS entry point\n"));
        goto LoadGina_ErrorReturn;
    }

    pTerm->Gina.pWlxDisplayLockedNotice = (PWLX_DISPLAYLOCKEDNOTICE) GetProcAddress(pTerm->Gina.hInstance, WLX_DISPLAYLOCKED_NAME);
    if (!pTerm->Gina.pWlxDisplayLockedNotice)
    {
        DebugLog((DEB_ERROR, "Could not find WlxDisplayLockedNotice\n"));
        goto LoadGina_ErrorReturn;
    }

    pTerm->Gina.pWlxWkstaLockedSAS = (PWLX_WKSTALOCKEDSAS) GetProcAddress(pTerm->Gina.hInstance, WLX_WKSTALOCKEDSAS_NAME);
    if (!pTerm->Gina.pWlxWkstaLockedSAS)
    {
        DebugLog((DEB_ERROR, "Could not find WlxWkstaLockedSAS entry point \n"));
        goto LoadGina_ErrorReturn;
    }

    pTerm->Gina.pWlxIsLockOk = (PWLX_ISLOCKOK) GetProcAddress(pTerm->Gina.hInstance, WLX_ISLOCKOK_NAME);
    if (!pTerm->Gina.pWlxIsLockOk)
    {
        DebugLog((DEB_ERROR, "Could not find WlxIsLockOk entry point"));
        goto LoadGina_ErrorReturn;
    }
    IsLockOkFn = pTerm->Gina.pWlxIsLockOk;

    pTerm->Gina.pWlxIsLogoffOk = (PWLX_ISLOGOFFOK) GetProcAddress(pTerm->Gina.hInstance, WLX_ISLOGOFFOK_NAME);
    if (!pTerm->Gina.pWlxIsLogoffOk)
    {
        DebugLog((DEB_ERROR, "Could not find WlxIsLogoffOk entry point"));
        goto LoadGina_ErrorReturn;
    }

    pTerm->Gina.pWlxLogoff = (PWLX_LOGOFF) GetProcAddress(pTerm->Gina.hInstance, WLX_LOGOFF_NAME);
    if (!pTerm->Gina.pWlxLogoff)
    {
        DebugLog((DEB_ERROR, "Could not find WlxLogoff entry point\n"));
        goto LoadGina_ErrorReturn;
    }

    pTerm->Gina.pWlxShutdown = (PWLX_SHUTDOWN) GetProcAddress(pTerm->Gina.hInstance, WLX_SHUTDOWN_NAME);
    if (!pTerm->Gina.pWlxShutdown)
    {
        DebugLog((DEB_ERROR, "Could not find WlxShutdown entry point \n"));
        goto LoadGina_ErrorReturn;
    }


    //
    // New interfaces for NT 4.0
    //

    pTerm->Gina.pWlxStartApplication = (PWLX_STARTAPPLICATION) GetProcAddress(pTerm->Gina.hInstance, WLX_STARTAPPLICATION_NAME);
    if (!pTerm->Gina.pWlxStartApplication)
    {
        DebugLog((DEB_TRACE, "Could not find WlxStartApplication entry point \n"));
        pTerm->Gina.pWlxStartApplication = WlxStartApplication;
    }

    pTerm->Gina.pWlxScreenSaverNotify = (PWLX_SSNOTIFY) GetProcAddress(pTerm->Gina.hInstance, WLX_SSNOTIFY_NAME);
    if (!pTerm->Gina.pWlxScreenSaverNotify)
    {
        pTerm->Gina.pWlxScreenSaverNotify = DummyWlxScreenSaverNotify;
    }

    //
    // New interfaces for NT 5.0
    //

    pTerm->Gina.pWlxNetworkProviderLoad = (PWLX_NPLOAD) GetProcAddress( pTerm->Gina.hInstance, WLX_NPLOAD_NAME );
    if ( !pTerm->Gina.pWlxNetworkProviderLoad )
    {
        pTerm->Gina.pWlxNetworkProviderLoad = DummyWlxNetworkProviderLoad ;
    }

    pTerm->Gina.pWlxDisplayStatusMessage = (PWLX_DISPLAYSTATUSMESSAGE) GetProcAddress( pTerm->Gina.hInstance, WLX_DISPLAYSTATUSMESSAGE_NAME );
    if ( !pTerm->Gina.pWlxDisplayStatusMessage )
    {
        pTerm->Gina.pWlxDisplayStatusMessage = DummyWlxDisplayStatusMessage ;
    }

    pTerm->Gina.pWlxGetStatusMessage = (PWLX_GETSTATUSMESSAGE) GetProcAddress( pTerm->Gina.hInstance, WLX_GETSTATUSMESSAGE_NAME );
    if ( !pTerm->Gina.pWlxGetStatusMessage )
    {
        pTerm->Gina.pWlxGetStatusMessage = DummyWlxGetStatusMessage ;
    }

    pTerm->Gina.pWlxRemoveStatusMessage = (PWLX_REMOVESTATUSMESSAGE) GetProcAddress( pTerm->Gina.hInstance, WLX_REMOVESTATUSMESSAGE_NAME );
    if ( !pTerm->Gina.pWlxRemoveStatusMessage )
    {
        pTerm->Gina.pWlxRemoveStatusMessage = DummyWlxRemoveStatusMessage ;
    }


    //
    // Negotiate a version number with the gina
    //

    bResult = pTerm->Gina.pWlxNegotiate(WLX_CURRENT_VERSION, &dwGinaLevel);

    if (!bResult)
    {
        DebugLog((DEB_ERROR, "%ws failed the WlxNegotiate call\n", lpGinaName));
        goto LoadGina_ErrorReturn;
    }


    if (dwGinaLevel > WLX_CURRENT_VERSION)
    {
        DebugLog((DEB_ERROR, "%ws is at version %d, can't support\n", lpGinaName, dwGinaLevel));
        goto LoadGina_ErrorReturn;
    }

    return TRUE;

LoadGina_ErrorReturn:

    //
    // FreeLibrary the DLL if it was loaded
    //

    if (pTerm->Gina.hInstance)
        FreeLibrary (pTerm->Gina.hInstance);


    //
    // Reset the gina structure to NULL in case any of the
    // GetProcAddress calls above succeeded.
    //

    ZeroMemory (&pTerm->Gina, sizeof(GINASESSION));


    //
    // Setup to use the dummy bad gina functions
    //

    InitBadGina(pTerm);

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   BadGinaNegotiate
//
//  Synopsis:   Stub for the BadGina functions - handles the negotiate call
//
//  Arguments:  [dwWinlogonVersion] --
//              [pdwDllVersion]     --
//
//  History:    8-13-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
WINAPI
BadGinaNegotiate(
    DWORD                   dwWinlogonVersion,
    PDWORD                  pdwDllVersion
    )
{
    *pdwDllVersion = WLX_CURRENT_VERSION ;
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   BadGinaInitialize
//
//  Synopsis:   Initialize call
//
//  Arguments:  [lpWinsta]           --
//              [hWlx]               --
//              [pvReserved]         --
//              [pWinlogonFunctions] --
//              [pWlxContext]        --
//
//  History:    8-13-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
WINAPI
BadGinaInitialize(
    LPWSTR                  lpWinsta,
    HANDLE                  hWlx,
    PVOID                   pvReserved,
    PVOID                   pWinlogonFunctions,
    PVOID *                 pWlxContext
    )
{
    BadGinahWlx = hWlx ;

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   BadGinaInit
//
//  Synopsis:   Initializes the bad-gina dialog box
//
//  Arguments:  [hDlg] --
//
//  History:    8-13-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BadGinaInit(
    HWND    hDlg)
{
    WCHAR   Message[ MAX_PATH * 2 ];
    WCHAR   Format[ MAX_PATH ];
    WCHAR   Dll[ MAX_PATH ];

    LoadString( GetModuleHandle(NULL), IDS_BAD_GINA, Format, MAX_PATH );

    GetProfileString(APPLICATION_NAME, GINA_KEY, TEXT("msgina.dll"), Dll, MAX_PATH);

    _snwprintf( Message, MAX_PATH * 2, Format, Dll );

    SetDlgItemText( hDlg, IDD_BADGINA_LINE_1, Message );

    return( 0 );

}


//+---------------------------------------------------------------------------
//
//  Function:   BadGinaDlgProc
//
//  Synopsis:   Self explanatory
//
//  Arguments:  [hDlg]    --
//              [Message] --
//              [wParam]  --
//              [lParam]  --
//
//  History:    8-13-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
INT_PTR
CALLBACK
BadGinaDlgProc(
    HWND    hDlg,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam )
{
    switch ( Message )
    {
        case WM_INITDIALOG:

           CentreWindow( hDlg );

           BadGinaInit( hDlg );

           return( TRUE );

        case WM_COMMAND:
            if ( LOWORD( wParam ) == IDOK )
            {
                WlxSasNotify( BadGinahWlx, BADGINA_REBOOT_SAS );

                EndDialog( hDlg, IDOK );

                return( TRUE );
            }

    }

    return( FALSE );
}


//+---------------------------------------------------------------------------
//
//  Function:   BadGinaDisplaySASNotice
//
//  Synopsis:   Displays the bad gina DLL
//
//  Arguments:  [pWlxContext] --
//
//  History:    8-13-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
WINAPI
BadGinaDisplaySASNotice(
    PVOID                   pWlxContext
    )
{
        WlxDialogBox( BadGinahWlx,
                      GetModuleHandle( NULL ),
                      MAKEINTRESOURCE( IDD_BAD_GINA_DIALOG ),
                      NULL,
                      BadGinaDlgProc );

}


//+---------------------------------------------------------------------------
//
//  Function:   BadGinaLoggedOutSAS
//
//  Synopsis:   Handles the SAS request
//
//  Arguments:  [pWlxContext]       --
//              [dwSasType]         --
//              [pAuthenticationId] --
//              [pLogonSid]         --
//              [pdwOptions]        --
//              [phToken]           --
//              [pNprNotifyInfo]    --
//              [pProfile]          --
//
//  History:    8-13-96   RichardW   Created
//
//  Notes:      Always reboots the system
//
//----------------------------------------------------------------------------
int
WINAPI
BadGinaLoggedOutSAS(
    PVOID                   pWlxContext,
    DWORD                   dwSasType,
    PLUID                   pAuthenticationId,
    PSID                    pLogonSid,
    PDWORD                  pdwOptions,
    PHANDLE                 phToken,
    PWLX_MPR_NOTIFY_INFO    pNprNotifyInfo,
    PVOID *                 pProfile
    )
{
    if ( dwSasType != BADGINA_REBOOT_SAS )
    {
        WlxDialogBox( BadGinahWlx,
                      GetModuleHandle( NULL ),
                      MAKEINTRESOURCE( IDD_BAD_GINA_DIALOG ),
                      NULL,
                      BadGinaDlgProc );
    }

    return( WLX_SAS_ACTION_SHUTDOWN_REBOOT );

}


//+---------------------------------------------------------------------------
//
//  Function:   BadGinaShutdown
//
//  Synopsis:   Shutdown notification
//
//  Arguments:  [pWlxContext]  --
//              [ShutdownType] --
//
//  History:    8-13-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
WINAPI
BadGinaShutdown(
    PVOID                   pWlxContext,
    DWORD                   ShutdownType
    )
{
    return;
}


//+---------------------------------------------------------------------------
//
//  Function:   BadGinaScreenSaverNotify
//
//  Synopsis:   Prevents any screensavers from running
//
//  Arguments:  [pWlxContext] --
//              [pSecure]     --
//
//  History:    8-13-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
WINAPI
BadGinaScreenSaverNotify(
    PVOID                   pWlxContext,
    BOOL *                  pSecure)
{
    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Function:   InitBadGina
//
//  Synopsis:   Sets up the GINA session
//
//  Arguments:  (none)
//
//  History:    8-13-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
InitBadGina(
    PTERMINAL pTerm )
{
    pTerm->Gina.hInstance = GetModuleHandle( NULL );
    pTerm->Gina.pWlxNegotiate = BadGinaNegotiate;
    pTerm->Gina.pWlxInitialize = BadGinaInitialize;
    pTerm->Gina.pWlxDisplaySASNotice = BadGinaDisplaySASNotice;
    pTerm->Gina.pWlxLoggedOutSAS = BadGinaLoggedOutSAS;
    pTerm->Gina.pWlxShutdown = BadGinaShutdown;
    pTerm->Gina.pWlxScreenSaverNotify = BadGinaScreenSaverNotify;
}
