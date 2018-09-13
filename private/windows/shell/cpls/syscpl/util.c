/** FILE: util.c *********** Module Header ********************************
 *
 *  System applet utility library routines. This file contains string,
 *  cursor, SendWinIniChange() and Memory allocation routines.
 *
 * History:
 *  15:30 on Thur  25 Apr 1991  -by-  Steve Cathcart   [stevecat]
 *        Took base code from Win 3.1 source
 *  10:30 on Tues  04 Feb 1992  -by-  Steve Cathcart   [stevecat]
 *        Updated code to latest Win 3.1 sources
 *  15:30 on Thur  03 May 1994  -by-  Steve Cathcart   [stevecat]
 *        Increased  MyMessageBox buffers, Restart dialog changes
 *  17:00 on Mon   18 Sep 1995  -by-  Steve Cathcart   [stevecat]
 *        Changes for product update - SUR release NT v4.0
 *
 *  Copyright (C) 1990-1995 Microsoft Corporation
 *
 *************************************************************************/
/* Notes -

    Global Functions:

      U T I L I T Y

        BackslashTerm () - add backslash char to path
        CheckVal ( ) -
        DoDialogBoxParam () - invoke dialog, set help context
        ErrMemDlg () - display Memory Error message box
        HourGlass () - Turn on/off hour glass cursor
        MyAtoi( ) - To convert from Unicode to ANSI string before calling atoi
        MyMessageBox () - display message to user, with parameters
        RestartDlg () - display "Restart System" message, restart system
        SendWinIniChange () - broadcast system change message via USER


      M E M O R Y   A L L O C A T I O N

        AllocMem () - Allocate a block of memory, with error checks
        AllocStr () - Allocate memory for a string
        AllocStrA () - Allocate memory for an ANSI string
        FreeMem () - Check and free memory block allocated with AllocMem
        FreeStr () - Free string memory allocated with AllocStr
        FreeStrA () - Free string memory allocated with AllocStrA
        ReallocMem () - Resize a memory block originally from AllocMem
        ReallocStr () - Resize a string memory block from AllocStr
        ReallocStrA () - Resize a string memory block from AllocStrA

    Local Functions:

 */

//==========================================================================
//                                Include files
//==========================================================================

#include <nt.h>    // For shutdown privilege.
#include <ntrtl.h>
#include <nturtl.h>

// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>

// Application specific
#include "system.h"


#define INT_SIZE_LENGTH   20




LPTSTR BackslashTerm( LPTSTR pszPath )
{
    LPTSTR pszEnd;

    pszEnd = pszPath + lstrlen( pszPath );

    //
    //  Get the end of the source directory
    //

    switch( *CharPrev( pszPath, pszEnd ) )
    {
    case TEXT('\\'):
    case TEXT(':'):
        break;

    default:
        *pszEnd++ = TEXT( '\\' );
        *pszEnd = TEXT( '\0' );
    }
    return( pszEnd );
}

BOOL CheckVal( HWND hDlg, WORD wID, WORD wMin, WORD wMax, WORD wMsgID )
{
    WORD nVal;
    BOOL bOK;
    HWND hVal;

    if( wMin > wMax )
    {
        nVal = wMin;
        wMin = wMax;
        wMax = nVal;
    }

    nVal = (WORD) GetDlgItemInt( hDlg, wID, &bOK, FALSE );

    if( !bOK || ( nVal < wMin ) || ( nVal > wMax ) )
    {
        MyMessageBox( hDlg, wMsgID, INITS + 1,
                      MB_OK | MB_ICONINFORMATION, wMin, wMax );

        SendMessage( hDlg, WM_NEXTDLGCTL,
                     (WPARAM) ( hVal = GetDlgItem( hDlg, wID ) ), 1L );

//        SendMessage(hVal, EM_SETSEL, NULL, MAKELONG(0, 32767));

        SendMessage( hVal, EM_SETSEL, 0, 32767 );

        return( FALSE );
    }

    return( TRUE );
}


//
//  This does what is necessary to bring up a dialog box
//

int DoDialogBoxParam( int nDlg,
                      HWND hParent,
                      DLGPROC lpProc,
                      DWORD dwHelpContext,
                      DWORD dwParam)
{
    DWORD dwSave;

    dwSave = g_dwContext;
    g_dwContext = dwHelpContext;
    nDlg = DialogBoxParam( g_hInst, (LPTSTR) MAKEINTRESOURCE( nDlg ),
                           hParent, lpProc, dwParam);
    g_dwContext = dwSave;

    if( nDlg == -1 )
        MyMessageBox( hParent, INITS, INITS+1, IDOK );

    return( nDlg );
}

void ErrMemDlg( HWND hParent )
{
    MessageBox( hParent, g_szErrMem, g_szSystemApplet,
                MB_OK | MB_ICONHAND | MB_SYSTEMMODAL );
}


//
//  Turn hourglass on or off
//

void HourGlass( BOOL bOn )
{
    if( !GetSystemMetrics( SM_MOUSEPRESENT ) )
        ShowCursor( bOn );

    SetCursor( LoadCursor( NULL, bOn ? IDC_WAIT : IDC_ARROW ) );
}


///////////////////////////////////////////////////////////////////////////////
//
//   MyAtoi
//
//   Desc:  To convert from Unicode to ANSI string before
//          calling CRT atoi and atol functions.
//
///////////////////////////////////////////////////////////////////////////////

int MyAtoi( LPTSTR  string )
{
   CHAR   szAnsi[ INT_SIZE_LENGTH ];
   BOOL   fDefCharUsed;

#ifdef UNICODE
   WideCharToMultiByte( CP_ACP, 0, string, INT_SIZE_LENGTH,
                        szAnsi, INT_SIZE_LENGTH, NULL, &fDefCharUsed );

   return( atoi( szAnsi ) );
#else
   return( atoi( string ) );
#endif

}


int MyMessageBox( HWND hWnd, DWORD wText, DWORD wCaption, DWORD wType, ... )
{
    TCHAR   szText[ 4 * PATHMAX ], szCaption[ 2 * PATHMAX ];
    int     ival;
    va_list parg;

    va_start( parg, wType );

    if( wText == INITS )
        goto NoMem;

    if( !LoadString( g_hInst, wText, szCaption, CharSizeOf( szCaption ) ) )
        goto NoMem;

    wvsprintf( szText, szCaption, parg );

    if( !LoadString( g_hInst, wCaption, szCaption, CharSizeOf( szCaption ) ) )
        goto NoMem;

    if( (ival = MessageBox( hWnd, szText, szCaption, wType ) ) == 0 )
        goto NoMem;

    va_end( parg );

    return( ival );

NoMem:
    va_end( parg );

    ErrMemDlg( hWnd );
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// RestartDlg
//
// The following function is the dialog procedure for bringing up a system
// restart message.  This dialog is called whenever the user is advised to
// reboot the system.  The dialog contains an IDOK and IDCANCEL button, which
// instructs the function whether to restart windows immediately.
//
// Parameters:
//
// lParam - The LOWORD portion contains an index to the resource string
//          used to compose the restart message.  This string is inserted
//          before the string IDS_RESTART.
//
// Return Value: The usual dialog return value.
//
///////////////////////////////////////////////////////////////////////////////

BOOL RestartDlg( HWND hDlg, UINT message, DWORD wParam, LONG lParam )
{
    TCHAR   szMessage[ 200 ];
    TCHAR   szTemp[ 100 ];
    BOOLEAN PreviousPriv;

    switch (message)
    {

        case WM_INITDIALOG:

            //
            //  Set up the restart message
            //

            LoadString( g_hInst, LOWORD(lParam), szMessage, CharSizeOf( szMessage ) );

            if( LoadString( g_hInst, IDS_RESTART, szTemp, CharSizeOf( szTemp ) ) )
                lstrcat( szMessage, szTemp );

            SetDlgItemText( hDlg, RESTART_TEXT, szMessage );
            break;

        case WM_COMMAND:

            switch( LOWORD( wParam ) )
            {
                case IDOK:
                    RtlAdjustPrivilege( SE_SHUTDOWN_PRIVILEGE,
                                        TRUE, FALSE, &PreviousPriv );
                    ExitWindowsEx( EWX_REBOOT | EWX_SHUTDOWN, (DWORD) (-1) );
                    break;

                case IDCANCEL:
                    EndDialog( hDlg, 0L );
                    break;

                default:
                    return( FALSE );
            }
            return( TRUE );

        default:
            return( FALSE );
    }

    return( FALSE );
}

void SendWinIniChange( LPTSTR lpSection )
{
// NOTE: We have (are) gone through several iterations of which USER
//       api is the correct one to use.  The main problem for the Control
//       Panel is to avoid being HUNG if another app (top-level window)
//       is HUNG.  Another problem is that we pass a pointer to a message
//       string in our address space.  SendMessage will 'thunk' this properly
//       for each window, but PostMessage and SendNotifyMessage will not.
//       That finally brings us to try to use SendMessageTimeout(). 9/21/92
//
// Try SendNotifyMessage in build 260 or later - kills earlier builds
//    SendNotifyMessage ((HWND)-1, WM_WININICHANGE, 0L, (LONG)lpSection);
//    PostMessage ((HWND)-1, WM_WININICHANGE, 0L, (LONG)lpSection);
//  [stevecat] 4/4/92
//
//    SendMessage ((HWND)-1, WM_WININICHANGE, 0L, (LPARAM)lpSection);
//
    //  NOTE: The final parameter (LPDWORD lpdwResult) must be NULL

    SendMessageTimeout( (HWND)-1, WM_WININICHANGE, 0L, (LONG)lpSection,
                                            SMTO_ABORTIFHUNG, 1000, NULL );
}


///////////////////////////////////////////////////////////////////////////////
//
//          M E M O R Y   A L L O C A T I O N   R O U T I N E S
//
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// 
// Routine Description:
// 
//     This function will allocate local memory. It will possibly allocate
//     extra memory and fill this with debugging information for the
//     debugging version.
// 
// Arguments:
// 
//     cb - The amount of memory to allocate
// 
// Return Value:
// 
//     NON-NULL - A pointer to the allocated memory
// 
//     FALSE/NULL - The operation failed. Extended error status is available
//         using GetLastError.
// 
/////////////////////////////////////////////////////////////////////////////

LPVOID AllocMem( DWORD cb )
{
    LPDWORD  pMem;
    DWORD    cbNew;

    cbNew = cb + 2 * sizeof( DWORD );

    if( cbNew & 3 )
        cbNew += sizeof( DWORD ) - ( cbNew & 3 );

    pMem = (LPDWORD) LocalAlloc( LPTR, cbNew );

    if( !pMem )
        return NULL;

    // memset( pMem, 0, cbNew );     // This might go later if done in NT

    *pMem = cb;

    *(LPDWORD)( (LPBYTE) pMem + cbNew - sizeof( DWORD ) ) = 0xdeadbeef;

    return (LPVOID)( pMem + 1 );
}


BOOL FreeMem( LPVOID pMem, DWORD  cb )
{
    DWORD   cbNew;
    LPDWORD pNewMem;
    TCHAR   szError[ 128 ];

    if( !pMem )
        return TRUE;

    pNewMem = pMem;
    pNewMem--;

    cbNew = cb + 2 * sizeof( DWORD );

    if( cbNew & 3 )
        cbNew += sizeof( DWORD ) - ( cbNew & 3 );

    if( ( *pNewMem != cb ) ||
        (*(LPDWORD)( (LPBYTE) pNewMem + cbNew - sizeof( DWORD ) ) != 0xdeadbeef))
    {
#if DBG
        wsprintf (szError, TEXT("Corrupt Memory in Control Panel : %0lx\n"), pMem);
        OutputDebugString(szError);
        // DbgBreakPoint();
        //  Corrupt Memory in Control Panel - try to free it anyway
        return FALSE;
#endif  // DBG
    }

    return ( ( (HLOCAL) pNewMem == LocalFree ( (LPVOID) pNewMem ) ) );
}


LPVOID ReallocMem( LPVOID lpOldMem, DWORD cbOld, DWORD cbNew )
{
   LPVOID lpNewMem;


   lpNewMem = AllocMem( cbNew );

   if( lpOldMem )
   {
      memcpy( lpNewMem, lpOldMem, min( cbNew, cbOld ) );
      FreeMem( lpOldMem, cbOld );
   }
   return lpNewMem;
}


/////////////////////////////////////////////////////////////////////////////
// 
// Routine Description:
// 
//     These functions will allocate or reallocate enough local memory to
//     store the specified  string, and copy that string to the allocated
//     memory.  The FreeStr function frees memory that was initially
//     allocated by AllocStr.
//
// Arguments:
// 
//     lpStr - Pointer to the string that needs to be allocated and stored
// 
// Return Value:
// 
//     NON-NULL - A pointer to the allocated memory containing the string
// 
//     FALSE/NULL - The operation failed. Extended error status is available
//         using GetLastError.
// 
/////////////////////////////////////////////////////////////////////////////

LPTSTR AllocStr( LPTSTR lpStr )
{
    LPTSTR lpMem;


    if( !lpStr )
        return NULL;

    if( lpMem = AllocMem ( ByteCountOf( lstrlen( lpStr ) + 1 ) ) )
           lstrcpy( lpMem, lpStr );

    return lpMem;
}


BOOL FreeStr( LPTSTR lpStr )
{
   return lpStr ? FreeMem( lpStr, ByteCountOf( lstrlen( lpStr ) + 1 ) ) : FALSE;
}


BOOL ReallocStr (LPTSTR *plpStr, LPTSTR lpStr)
{
   FreeStr (*plpStr);
   *plpStr = AllocStr (lpStr);

   return TRUE;
}


