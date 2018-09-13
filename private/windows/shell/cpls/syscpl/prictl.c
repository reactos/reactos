//***************************************************************************
//
// prictl.c
//
// History:
//  17:00 on Mon   18 Sep 1995  -by-  Steve Cathcart   [stevecat]
//        Changes for product update - SUR release NT v4.0
//
//
//  Copyright (C) 1994-1995 Microsoft Corporation
//
//***************************************************************************

//==========================================================================
//                              Include files
//==========================================================================

#include "system.h"


//==========================================================================
//                                Globals
//==========================================================================

HKEY  m_Hkey;
TCHAR m_szRegPriKey[] = TEXT( "SYSTEM\\CurrentControlSet\\Control\\PriorityControl" );
TCHAR m_szRegPriority[] = TEXT( "Win32PrioritySeparation" );


//==========================================================================
//                         Local Data Declarations
//==========================================================================

short WhichRadioButtonx( HWND, short, short );


//==========================================================================
//                                Functions
//==========================================================================

BOOL  TaskingDlg( HWND  hdlg, UINT  uMessage, WPARAM  wParam, LPARAM  lParam )
{
    short  iButtonChoice;
    LONG   RegRes;
    DWORD  Type, Value, Length;
    int    InitButton;

    switch( uMessage )
    {
       case WM_INITDIALOG:

          HourGlass( TRUE );

          InitButton = IDB_DEFAULT;

          //
          // initialize from the registry
          //

          // sets the id in the last paramater as the selected button

          RegRes = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                 m_szRegPriKey,
                                 0,
                                 KEY_QUERY_VALUE | KEY_SET_VALUE,
                                 &m_Hkey );

          if( RegRes == ERROR_SUCCESS )
          {
              Length = sizeof( Value );
              RegRes = RegQueryValueEx( m_Hkey,
                                        m_szRegPriority,
                                        NULL,
                                        &Type,
                                        (LPBYTE) &Value,
                                        &Length );

              if( RegRes == ERROR_SUCCESS )
              {
                  switch( Value )
                  {
                      case 0:
                          InitButton = IDB_NONE;
                          break;
                      case 1:
                          InitButton = IDB_SMALLER;
                          break;
                      case 2:
                      default:
                          InitButton = IDB_DEFAULT;
                          break;
                  }
              }
          }
          else
              m_Hkey = NULL;

          CheckRadioButton( hdlg, IDB_DEFAULT, IDB_NONE, InitButton );
          HourGlass( FALSE );
          return 0;
          break;

       case WM_COMMAND:
       {
          WORD wNotifCode;

          wNotifCode = HIWORD( wParam );

          switch( LOWORD( wParam ) )
          {
             case IDB_OK:
                //
                //  find out which radio button was clicked.
                //

                iButtonChoice = WhichRadioButtonx( hdlg, IDB_DEFAULT, IDB_NONE );

                switch( iButtonChoice )
                {
                    case IDB_NONE:
                        Value = 0;
                        break;

                    case IDB_SMALLER:
                        Value = 1;
                        break;

                    case IDB_DEFAULT:
                    default:
                        Value = 2;
                        break;
                }

                if( m_Hkey )
                {
                    Type = REG_DWORD;
                    Length = sizeof( Value );
                    RegSetValueEx( m_Hkey,
                                   m_szRegPriority,
                                   0,
                                   REG_DWORD,
                                   (LPBYTE) &Value,
                                   Length );

                    RegCloseKey( m_Hkey );
                    m_Hkey = NULL;
                }

                EndDialog( hdlg,TRUE );
                break;

             case IDB_CANCEL:
                EndDialog( hdlg,FALSE );
                break;

             case IDB_HELP:
                goto DoHelp;
                break;

             default:
                return FALSE;
          }
          break;
       }  /* end of WM_COMMAND */

       default:
          if( uMessage == g_wHelpMessage )
          {
DoHelp:
              SysHelp( hdlg );
          }
          else
             return FALSE;
          break;
    }

    return TRUE;

} /* MainBox */


/* This finds the first control that is checked in the range
 * of controls (nFirst, nLast), inclusive.  If none is checked,
 * the last button will be returned. [but this can never happen in this
 * quality GMT applet]
 */
short WhichRadioButtonx( HWND hDlg, short nFirst, short nLast )
{
    for ( ; nFirst < nLast; ++nFirst )
    {
       if( IsDlgButtonChecked( hDlg, nFirst ) )
          break;
    }
    return( nFirst );
}
