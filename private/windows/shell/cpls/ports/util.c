/** FILE: util.c *********** Module Header ********************************
 *
 *  Ports applet utility library routines. This file contains string,
 *  cursor, SendWinIniChange() routines.
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
 *  Nov 1997					-by-  Doron Holan	   [stevecat]
 *        Removed obsolete cpl code
 *
 *  Copyright (C) 1990-1995 Microsoft Corporation
 *
 *************************************************************************/
/* Notes -

    Global Functions:

      U T I L I T Y

        BackslashTerm () - add backslash char to path
        ErrMemDlg () - display Memory Error message box
        MyAtoi () - To convert from Unicode to ANSI string before calling atoi
        myatoi () - local implementation of atoi for Unicode strings
        MyItoa () - To convert from ANSI to Unicode string after calling itoa
        MyMessageBox () - display message to user, with parameters
        MyUltoa () - To convert from Unicode to ANSI string before calling ultoa
        SendWinIniChange () - broadcast system change message via USER
        strscan () - Find a string within another string
        StripBlanks () - Strip leading and trailing blanks from a string


    Local Functions:

 */

//==========================================================================
//                                Include files
//==========================================================================

// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// Application specific
#include "ports.h"


#define INT_SIZE_LENGTH   20
#define LONG_SIZE_LENGTH  40


LPTSTR 
BackslashTerm(LPTSTR pszPath)
{
    LPTSTR pszEnd;

    pszEnd = pszPath + lstrlen(pszPath);

    //
    //  Get the end of the source directory
    //
    switch(*CharPrev(pszPath, pszEnd)) {
    case TEXT('\\'):
    case TEXT(':'):
        break;

    default:
        *pszEnd++ = TEXT('\\');
        *pszEnd = TEXT('\0');
    }

    return pszEnd;
}

void 
ErrMemDlg(HWND hParent)
{
    MessageBox(hParent, g_szErrMem, g_szPortsApplet,
               MB_OK | MB_ICONHAND | MB_SYSTEMMODAL );
}

///////////////////////////////////////////////////////////////////////////////
//
//   MyAtoi
//
//   Desc:  To convert from Unicode to ANSI string before
//          calling CRT atoi and atol functions.
//
///////////////////////////////////////////////////////////////////////////////

int 
MyAtoi(LPTSTR  string)
{
   CHAR   szAnsi[ INT_SIZE_LENGTH ];
   BOOL   fDefCharUsed;

#ifdef UNICODE
   WideCharToMultiByte(CP_ACP, 0, string, INT_SIZE_LENGTH,
                       szAnsi, INT_SIZE_LENGTH, NULL, &fDefCharUsed);

   return atoi(szAnsi);
#else
   return atoi(string);
#endif

}


int 
myatoi(LPTSTR pszInt)
{
    int   retval;
    TCHAR cSave;

    for (retval = 0; *pszInt; ++pszInt) {
        if ((cSave = (TCHAR) (*pszInt - TEXT('0'))) > (TCHAR) 9)
            break;

        retval = (int) (retval * 10 + (int) cSave);
    }
    return (retval);
}



///////////////////////////////////////////////////////////////////////////////
//
//   MyItoa
//
//   Desc:  To convert from ANSI to Unicode string after calling
//          CRT itoa function.
//
///////////////////////////////////////////////////////////////////////////////

LPTSTR 
MyItoa(INT value, LPTSTR string, INT radix)
{
   CHAR   szAnsi[INT_SIZE_LENGTH];

#ifdef UNICODE

   _itoa(value, szAnsi, radix);
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szAnsi, -1,
                       string, INT_SIZE_LENGTH );
#else

   _itoa(value, string, radix);

#endif

   return (string);
 
} // end of MyItoa()


LPTSTR 
MyUltoa(unsigned long value, 
		LPTSTR  string, 
		INT  radix)
{
   CHAR   szAnsi[ LONG_SIZE_LENGTH ];

#ifdef UNICODE

   _ultoa(value, szAnsi, radix);
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szAnsi, -1,
                       string, LONG_SIZE_LENGTH );
#else

   _ultoa(value, string, radix);

#endif

   return( string );

} // end of MyUltoa()


int 
MyMessageBox(HWND hWnd, 
			 DWORD wText, 
			 DWORD wCaption, 
			 DWORD wType, 
			 ...)
{
    TCHAR   szText[4 * PATHMAX], 
			szCaption[2 * PATHMAX];
    int     ival;
    va_list parg;

    va_start(parg, wType);

    if (wText == INITS)
        goto NoMem;

    if (!LoadString(g_hInst, wText, szCaption, CharSizeOf(szCaption)))
        goto NoMem;

    wvsprintf(szText, szCaption, parg);

    if (!LoadString(g_hInst, wCaption, szCaption, CharSizeOf(szCaption)))
        goto NoMem;

    if ((ival = MessageBox(hWnd, szText, szCaption, wType)) == 0)
        goto NoMem;

    va_end(parg);

    return ival;

NoMem:
    va_end(parg);
    ErrMemDlg(hWnd);

    return 0;
}

void 
SendWinIniChange(LPTSTR lpSection)
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

    SendMessageTimeout((HWND)-1, 
					   WM_WININICHANGE, 
					   0L, 
					   (WPARAM) lpSection,
					   SMTO_ABORTIFHUNG,
					   1000, 
					   NULL);
}

LPTSTR 
strscan(LPTSTR pszString, 
		LPTSTR pszTarget)
{
    LPTSTR psz;

    if (psz = _tcsstr( pszString, pszTarget))
        return (psz);
    else
        return (pszString + lstrlen(pszString));
}


///////////////////////////////////////////////////////////////////////////////
//
//  StripBlanks()
//
//   Strips leading and trailing blanks from a string.
//   Alters the memory where the string sits.
//
///////////////////////////////////////////////////////////////////////////////

void 
StripBlanks(LPTSTR pszString)
{
    LPTSTR  pszPosn;

    //
    //  strip leading blanks
    //

    pszPosn = pszString;

    while (*pszPosn == TEXT(' '))
        pszPosn++;

    if (pszPosn != pszString)
        lstrcpy(pszString, pszPosn);

    //
    //  strip trailing blanks
    //

    if ((pszPosn = pszString + lstrlen(pszString)) != pszString) {
       pszPosn = CharPrev(pszString, pszPosn);

       while (*pszPosn == TEXT(' '))
           pszPosn = CharPrev(pszString, pszPosn);

       pszPosn = CharNext(pszPosn);

       *pszPosn = TEXT('\0');
    }
}

BOOL ReadRegistryByte(HKEY       hKey,
                      PTCHAR     valueName,
                      PBYTE      regData)
{
    DWORD       regDataType = 0;
    DWORD       regDataSize = 0;

    regDataSize = sizeof(*regData);
    if ((ERROR_SUCCESS != RegQueryValueEx(hKey,
                                          valueName,
                                          NULL,
                                          &regDataType,
                                          regData,
                                          &regDataSize))
        || (regDataSize != sizeof(BYTE))
        || (regDataType != REG_BINARY))
    {
        //
        // Read was unsuccessful  or not a binary value, regData is not set
        //
        return FALSE;
    }

    //
    // Read was a success, regData contains the value read in
    //
    return TRUE;
}

BOOL IsParPort(IN HDEVINFO         deviceInfoSet,
               IN PSP_DEVINFO_DATA deviceInfoData)
{
    HKEY        hKey;
    BYTE        regData = 0;
    BOOL        success;

    hKey = SetupDiOpenDevRegKey(deviceInfoSet,
                                deviceInfoData,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DRV,
                                KEY_READ);

    if (hKey == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    success = ReadRegistryByte(hKey,
                               m_szPortSubClass,
                               &regData);
    RegCloseKey(hKey);

    if (success) {
        return !regData;
    }
    else {
        return FALSE;
    }
}

// @@BEGIN_DDKSPLIT
#if 0
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

            LoadString( g_hInst, LOWORD(lParam), szMessage,
                        CharSizeOf( szMessage ) );

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
       MyMessageBox( hParent, INITS, IDS_INIT_NAME, IDOK );

    return( nDlg );
}

#endif // 0
// @@END_DDKSPLIT
