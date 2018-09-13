/*
 * prictl.C
 *
 * Copyright (C) 1990 Microsoft Corporation.
 *
 * The Priority control panel applet.
 *
 * History:
 *
 *  markta  1992 hacked midi applet for markl's use
 */
/* Revision history:

*/

/*-=-=-=-=- Include Files       -=-=-=-=-*/

#include        <windows.h>
#include        "main.h"
#include        "prictl.h"
#include        <cpl.h>
#include        "cphelp.h"

HKEY Hkey;

short    WhichRadioButtonx (HWND, short, short);

BOOL  TaskingDlg(
    HWND    hdlg,
    UINT    uMessage,
    WPARAM  wParam,
    LPARAM  lParam)
{
    short  iButtonChoice;
    LONG   RegRes;
    DWORD  Type, Value, Length;
    int    InitButton;

    switch (uMessage)
    {
       case WM_INITDIALOG:

          HourGlass (TRUE);

          InitButton = IDB_DEFAULT;

          //
          // initialize from the registry
          //

          // sets the id in the last paramater as the selected button

          RegRes = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                                 TEXT("SYSTEM\\CurrentControlSet\\Control\\PriorityControl"),
                                 0,
                                 KEY_QUERY_VALUE | KEY_SET_VALUE,
                                 &Hkey);
          if (RegRes == ERROR_SUCCESS)
          {
              Length = sizeof(Value);
              RegRes = RegQueryValueEx (Hkey,
                                        TEXT("Win32PrioritySeparation"),
                                        NULL,
                                        &Type,
                                        (LPBYTE) &Value,
                                        &Length);
              if (RegRes == ERROR_SUCCESS)
              {
                  switch (Value)
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
              Hkey = NULL;

          CheckRadioButton (hdlg, IDB_DEFAULT, IDB_NONE, InitButton);
          HourGlass (FALSE);
          return 0;
          break;

       case WM_COMMAND:
       {
          WORD wNotifCode;

          wNotifCode = HIWORD(wParam);

          switch (LOWORD(wParam))
          {
             case IDB_OK:

                // find out which radio button was clicked.
                iButtonChoice = WhichRadioButtonx(hdlg, IDB_DEFAULT, IDB_NONE);
                switch (iButtonChoice)
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
                if (Hkey)
                {
                    Type = REG_DWORD;
                    Length = sizeof(Value);
                    RegSetValueEx (Hkey,
                                   TEXT("Win32PrioritySeparation"),
                                   0,
                                   REG_DWORD,
                                   (LPBYTE)&Value,
                                   Length);

                    RegCloseKey (Hkey);
                    Hkey = NULL;
                }

                EndDialog (hdlg,TRUE);
                break;

             case IDB_CANCEL:
                EndDialog (hdlg,FALSE);
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
          if (uMessage == wHelpMessage)
          {
DoHelp:
              CPHelp (hdlg);
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
short WhichRadioButtonx (HWND hDlg, short nFirst, short nLast)
{
    for ( ; nFirst < nLast; ++nFirst)
    {
       if (IsDlgButtonChecked (hDlg, nFirst))
          break;
    }
    return (nFirst);
}
