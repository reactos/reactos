//***************************************************************************
//
// cpl.c
//
// History:
//      27 Feb 1995  Steve Cathcart   [stevecat]
//          Ported to Windows NT and Unicode, cleaned up                  
//  17:00 on Mon   18 Sep 1995  -by-  Steve Cathcart   [stevecat]
//        Changes for product update - SUR release NT v4.0
//
//
//  Copyright (C) 1994-1995 Microsoft Corporation
//
//***************************************************************************

//==========================================================================
//                             Include files
//==========================================================================
#include "system.h"
#include <cpl.h>


//==========================================================================
//                                Globals
//==========================================================================

HANDLE  g_hInst  = NULL;
BOOL    g_bSetup = FALSE;           // Set TRUE when running under "Setup"

TCHAR g_szSysDir[ PATHMAX ];        //  GetSystemDirectory
TCHAR g_szWinDir[ PATHMAX ];        //  GetWindowsDirectory
TCHAR g_szClose[ 40 ];              //  "Close" string
TCHAR g_szSharedDir[ PATHMAX ];     //  Shared dir found by Version apis
TCHAR g_szErrMem[ 200 ];            //  Low memory message
TCHAR g_szSystemApplet[ 30 ];       //  "System Control Panel Applet" title
TCHAR g_szNull[]  = TEXT("");       //  Null string

//
//  Stuff for Help
//

UINT   g_wHelpMessage;
DWORD  g_dwContext = IDH_CHILD_SYSTEM;


//
//  Module globals
//

TCHAR  m_szHelpFile[ PATHMAX ];
//TCHAR  m_szSysHelp[] = TEXT("system.hlp");
TCHAR  m_szSysHelp[] = TEXT("control.hlp");


//==========================================================================
//                            External Declarations
//==========================================================================
/*  data  */


/*  functions  */
// extern BOOL FAR PASCAL OpenSystemPropertySheet( HWND hwnd, LPCTSTR cmdline );

//==========================================================================
//                            Local Definitions
//==========================================================================



//==========================================================================
//                         Local Data Declarations
//==========================================================================



//==========================================================================
//                                Functions
//==========================================================================


BOOL APIENTRY LibMain( HANDLE hDll, DWORD dwReason, LPVOID lpReserved )
{
    switch( dwReason )
    {
    case DLL_PROCESS_ATTACH:
    	g_hInst = hDll;
        DisableThreadLibraryCalls(hDll);
    	break;

    case DLL_PROCESS_DETACH:
    	break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_THREAD_ATTACH:
    default:
    	break;
    }

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
//  InitSystemApplet
//
//
/////////////////////////////////////////////////////////////////////////////

BOOL InitSystemApplet( HWND hwndParent )
{
    DWORD  dwClass, dwShare;
    TCHAR  szClass[ 40 ];


    if( !RegisterArrowClass( g_hInst ) )
    {
        return FALSE;
    }

    LoadString( g_hInst, INITS,   g_szErrMem,       CharSizeOf( g_szErrMem ) );
    LoadString( g_hInst, INITS+1, g_szSystemApplet, CharSizeOf( g_szSystemApplet ) );

    //
    //  Get the "Close" string
    //

    LoadString( g_hInst, INITS+2,  g_szClose,    CharSizeOf( g_szClose ) );

    //
    //  Get the Windows and the System dirs, plus the directory for
    //  installing shared files, and the full system.ini, control.ini,
    //  and setup.inf paths
    //

    GetWindowsDirectory( g_szWinDir, PATHMAX );

    BackslashTerm( g_szWinDir );

    GetSystemDirectory( g_szSysDir, PATHMAX );

    BackslashTerm( g_szSysDir );

    //
    //  Create a fully qualified path to our Help file because of
    //  potential collisions with Win 3.1 help file in Windows dirs.
    //

    lstrcpy( m_szHelpFile, g_szSysDir );
    lstrcat( m_szHelpFile, m_szSysHelp );

    dwClass = CharSizeOf( szClass );
    dwShare = CharSizeOf( g_szSharedDir );

    VerFindFile( VFFF_ISSHAREDFILE, g_szNull, NULL, g_szNull, szClass, &dwClass,
                 g_szSharedDir, &dwShare );

    g_wHelpMessage    = RegisterWindowMessage( TEXT( "ShellHelp" ) );

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
//  TerminateSystemApplet
//
//
/////////////////////////////////////////////////////////////////////////////

void TerminateSystemApplet( HWND hWnd )
{
    UnRegisterArrowClass( g_hInst );
}


/////////////////////////////////////////////////////////////////////////////
//
// CplApplet:
//
//  The main applet information manager.
//
/////////////////////////////////////////////////////////////////////////////

LONG WINAPI CPlApplet( HWND hwnd, UINT uMsg, LPARAM lParam1, LPARAM lParam2 )
{
    static BOOL s_fReEntered = FALSE;
    static int  s_iInitCount = 0;

    switch( uMsg )
    {
    case CPL_INIT:
        if( !s_iInitCount )
        {
            if( !InitSystemApplet( hwnd ) )
                return FALSE;
        }

        s_iInitCount++;

        return TRUE;
    
    case CPL_GETCOUNT:
        //
        // How many applets are in this DLL?
        //

        return 1;

    case CPL_INQUIRE:
        //
        // Fill the CPLINFO with the pertinent information.
        //

        #define lpOldCPlInfo ((LPCPLINFO)lParam2)

        switch (lParam1)
        {
        case 0:
            lpOldCPlInfo->idIcon = SYSTEM_ICON;
            lpOldCPlInfo->idName = IDS_SYSTEM;
            lpOldCPlInfo->idInfo = IDS_SYSTEMINFO;
            break;
        }

        lpOldCPlInfo->lData = 0L;
        return TRUE;

    case CPL_NEWINQUIRE:

        #define lpCPlInfo ((LPNEWCPLINFO)lParam2)

        switch( lParam1 )
        {
        case 0:
            lpCPlInfo->hIcon = LoadIcon( g_hInst, MAKEINTRESOURCE( SYSTEM_ICON ) );
            LoadString( g_hInst, IDS_SYSTEM, lpCPlInfo->szName, sizeof( lpCPlInfo->szName ) );
            LoadString( g_hInst, IDS_SYSTEMINFO, lpCPlInfo->szInfo, sizeof( lpCPlInfo->szInfo ) );
            lpCPlInfo->dwHelpContext = 0;
            break;
        }

        lpCPlInfo->dwSize = sizeof( NEWCPLINFO );
        lpCPlInfo->lData = 0L;
        lpCPlInfo->szHelpFile[ 0 ] = 0;
        return TRUE;

    case CPL_DBLCLK:
        lParam2 = (LPARAM)0;

        //
        // fall through...
        //

    case CPL_STARTWPARMS:
        //
        // Do the applet thing.
        //

        switch (lParam1)
        {
        case 0:
//	        OpenSystemPropertySheet( hwnd, (LPCTSTR) lParam2 );

            DialogBox( g_hInst, (LPTSTR) MAKEINTRESOURCE( DLG_SYSTEM ), hwnd,
                       (DLGPROC) SystemDlg );
            break;
        }
        break;      

    case CPL_EXIT:
        s_iInitCount--;

        //
        // Free up any allocations of resources made.
        //

        if( !s_iInitCount )
            TerminateSystemApplet( hwnd );

        s_fReEntered = FALSE;

        break;
            
    case CPL_SETUP:

        //
        //  Private message sent when this applet is running under "Setup"
        //

        g_bSetup= TRUE;

        break;

    default:
        return 0L;
    }

    return 1L;
}

void SysHelp( HWND hWnd )
{
//    char    szBuf[ 80 ];

//    wsprintf( szBuf, "Help Context %ld", g_dwContext );

    WinHelp( hWnd, m_szHelpFile, HELP_CONTEXT, g_dwContext );
}
