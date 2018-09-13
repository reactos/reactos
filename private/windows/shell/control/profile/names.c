//*************************************************************
//  File name:    NAMES.C
//
//  Description:  Profile control panel applet
//                This file contains the source code for
//                "Computer Names" dialog box.
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1992-1994
//  All rights reserved
//
//*************************************************************
#include <windows.h>
#include "profile.h"


//*************************************************************
//
//  NamesDlgProc()
//
//  Purpose:    Dialog box procedure
//
//  Parameters: HWND hDlg     - Window handle of dialog box
//              UINT message  - Window message
//              WPARAM wParam - WORD parameter
//              LPARAM lParam - LONG parameter
//      
//
//  Return:     (BOOL) TRUE if message was processed
//                     FALSE if not
//
//*************************************************************

LRESULT CALLBACK NamesDlgProc(HWND hDlg, UINT message,
                              WPARAM wParam, LPARAM lParam)
{
        TCHAR szNewName [MAX_COMPUTER_NAME];
        LPTSTR lpNewName;
        BOOL  bEnableAdd;


        switch (message)
           {
           case WM_INITDIALOG:

              //
              // Add the names to the list box.
              //

              ParseAndAddComputerNames(hDlg, IDD_NAMESLIST, glpList);

              //
              // Find out if anything was added to the list.
              // If so, then highlight the selected item.  Otherwise,
              // disable the delete buttons.
              //

              if (SendDlgItemMessage (hDlg, IDD_NAMESLIST, LB_GETCOUNT, 0,0)) {
                 SendDlgItemMessage (hDlg, IDD_NAMESLIST, LB_SETSEL, 1, lParam);
              } else {
                 EnableWindow(GetDlgItem(hDlg, IDD_DELETE), FALSE);
                 EnableWindow(GetDlgItem(hDlg, IDD_CLEARALL), FALSE);
              }

              //
              // Disable the "Add" button, and free the buffer.  It will
              // be created again if the users presses the OK button.
              //

              EnableWindow (GetDlgItem(hDlg, IDD_ADD), FALSE);
              GlobalFree (glpList);

              //
              // Post ourselves a message so we can set the focus
              // appropriately.
              //

              PostMessage (hDlg, WM_USER+1, 0, 0);

              return TRUE;

           case WM_USER+1:

              //
              // Set the focus to the new computer name edit control
              // since most people will be starting from here.
              //

              SetFocus (GetDlgItem(hDlg, IDD_NEWNAME));

              break;

           case WM_COMMAND:
              switch (LOWORD(wParam)) {
                 case IDD_NEWNAME:

                    //
                    // Enable the Add button if appropriate.
                    //

                    if (HIWORD(wParam) == EN_UPDATE) {
                       bEnableAdd = GetDlgItemText(hDlg, IDD_NEWNAME,
                                     szNewName, MAX_COMPUTER_NAME);

                       EnableWindow (GetDlgItem (hDlg, IDD_ADD), bEnableAdd);

                       if (bEnableAdd) {
                          SetDefButton (hDlg, IDD_ADD);
                       } else {
                          SetDefButton (hDlg, IDOK);
                       }
                    }
                    break;

                 case IDD_ADD:

                    //
                    // Retrieve the new name from the edit control
                    //

                    GetDlgItemText(hDlg, IDD_NEWNAME,
                                   szNewName, MAX_COMPUTER_NAME);

                    //
                    // Check to see if the user entered the new computer
                    // name with a \\ infront of it.  If so, remove it.
                    //

                    lpNewName = szNewName;

                    if ( (szNewName[0] == TEXT('\\')) &&
                         (szNewName[1] == TEXT('\\')) ) {
                       lpNewName += 2;
                    }

                    //
                    // Add the new name if it doesn't already exist.
                    //

                    if (SendDlgItemMessage (hDlg, IDD_NAMESLIST, LB_FINDSTRINGEXACT,
                                            0, (LPARAM) lpNewName) == LB_ERR) {
                        SendDlgItemMessage (hDlg, IDD_NAMESLIST, LB_ADDSTRING,
                                            0, (LPARAM) lpNewName);
                    }

                    //
                    // Erase the contents of the edit control, and set the
                    // focus back to the edit control for quick entry of
                    // names.
                    //

                    SetWindowText (GetDlgItem(hDlg, IDD_NEWNAME), NULL);
                    SetFocus (GetDlgItem(hDlg, IDD_NEWNAME));
                    SetDefButton (hDlg, IDOK);

                    //
                    // Enable the delete buttons.
                    //

                    EnableWindow (GetDlgItem(hDlg, IDD_DELETE), TRUE);
                    EnableWindow (GetDlgItem(hDlg, IDD_CLEARALL), TRUE);

                    break;

                 case IDD_DELETE:
                    {
                    INT iSel [MAX_NUM_COMPUTERS];
                    INT iCount, i;

                    //
                    // Retrieve an array of selected items.
                    //

                    iCount = SendDlgItemMessage (hDlg, IDD_NAMESLIST, LB_GETSELITEMS,
                                                 MAX_NUM_COMPUTERS, (LPARAM) iSel);

                    if (iCount == LB_ERR) {
                       break;
                    }

                    //
                    // Now loop through the array and delete the items.
                    // Note that we have to do this from the bottom up,
                    // or the index's would be wrong as items are deleted
                    // from the top of the list.
                    //

                    for (i = iCount-1; i >= 0; i--) {
                        SendDlgItemMessage (hDlg, IDD_NAMESLIST, LB_DELETESTRING,
                                            iSel[i], 0);
                    }

                    //
                    // Find out if anything is left in the list.
                    // If not, then disable the delete buttons.
                    //

                    if (!SendDlgItemMessage (hDlg, IDD_NAMESLIST,
                                             LB_GETCOUNT, 0,0)) {
                       EnableWindow(GetDlgItem(hDlg, IDD_DELETE), FALSE);
                       EnableWindow(GetDlgItem(hDlg, IDD_CLEARALL), FALSE);
                    }

                    }
                    break;

                 case IDD_CLEARALL:

                    //
                    //  User requested to empty the entire list.
                    //

                    SendDlgItemMessage (hDlg, IDD_NAMESLIST, LB_RESETCONTENT,
                                        0, 0);

                    EnableWindow(GetDlgItem(hDlg, IDD_DELETE), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDD_CLEARALL), FALSE);

                    break;

                 case IDOK:
                    {
                    BOOL bFound;
                    TCHAR szProfile[MAX_ERROR_MSG];
                    TCHAR szErrorMsg [MAX_ERROR_MSG];

                    //
                    // Make sure the user doesn't still have a name
                    // in the "New Name" field that he forgot to "add"
                    //

                    if (GetDlgItemText(hDlg, IDD_NEWNAME,
                                   szNewName, MAX_COMPUTER_NAME)) {
                        LoadString (hInstance, IDS_NAME, szProfile, MAX_ERROR_MSG);
                        LoadString (hInstance, IDS_ADDNAME, szErrorMsg, MAX_ERROR_MSG);

                        if (MessageBox (hDlg, szErrorMsg, szProfile,
                                        MB_YESNO | MB_ICONQUESTION) == IDYES) {

                            //
                            // User would like to add the name before leaving.
                            // Check to see if the user entered the new computer
                            // name with a \\ infront of it.  If so, remove it.
                            //

                            lpNewName = szNewName;

                            if ( (szNewName[0] == TEXT('\\')) &&
                                 (szNewName[1] == TEXT('\\')) ) {
                               lpNewName += 2;
                            }

                            //
                            // Check for duplicates first, then add the name
                            // if it doesn't exist.
                            //

                            if (SendDlgItemMessage (hDlg, IDD_NAMESLIST,
                                                    LB_FINDSTRINGEXACT,
                                                    0,
                                                    (LPARAM) lpNewName) == LB_ERR) {
                                SendDlgItemMessage (hDlg, IDD_NAMESLIST,
                                                    LB_ADDSTRING,
                                                    0,
                                                    (LPARAM) lpNewName);
                            }

                        }

                    }


                    //
                    // Create the list of names to be passed back.
                    //

                    glpList = CreateList (hDlg, IDD_NAMESLIST, NULL, &bFound);

                    EndDialog(hDlg, TRUE);
                    return TRUE;
                    }

                 case IDCANCEL:

                    //
                    // Close the dialog box.
                    //

                    EndDialog(hDlg, FALSE);
                    return TRUE;

                 case IDD_NAMESHELP:

                     //
                     // User requested help
                     //

                     WinHelp (hDlg, szHelpFileName, HELP_CONTEXT,
                              NAMES_HELP_CONTEXT);
                     break;


                 default:
                    break;
              }
              break;

           default:

              //
              // User requested help via the F1 key.
              //

              if (message == uiShellHelp) {
                  WinHelp (hDlg, szHelpFileName, HELP_CONTENTS, 0);
              }
              break;
           }

        return FALSE;
}

//*************************************************************
//
//  SetDefButton()
//
//  Purpose:    Sets the default button
//
//  Parameters: HWND hDlg     - Window handle of dialog box
//              INT  idButton - ID of button
//
//  Return:     void
//
//*************************************************************

void SetDefButton(HWND hwndDlg, INT  idButton)
{
    LRESULT lr;

    if (HIWORD(lr = SendMessage(hwndDlg, DM_GETDEFID, 0, 0)) == DC_HASDEFID)
    {
        HWND hwndOldDefButton = GetDlgItem(hwndDlg, LOWORD(lr));

        //
        // If we are setting the default button to the button which is
        // already default, then exit now.
        //

        if (LOWORD(lr) == idButton) {
            return;
        }

        SendMessage (hwndOldDefButton,
                     BM_SETSTYLE,
                     MAKEWPARAM(BS_PUSHBUTTON, 0),
                     MAKELPARAM(TRUE, 0));
    }

    SendMessage( hwndDlg, DM_SETDEFID, idButton, 0L );
    SendMessage( GetDlgItem(hwndDlg, idButton),
                 BM_SETSTYLE,
                 MAKEWPARAM( BS_DEFPUSHBUTTON, 0 ),
                 MAKELPARAM( TRUE, 0 ));
}
