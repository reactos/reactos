//*************************************************************
//  File name:    PROFILE.C
//
//  Description:  Profile control panel applet
//
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1992-1994
//  All rights reserved
//
//*************************************************************
#include <windows.h>
#include <cpl.h>
#include "profile.h"


//
// Global Variables
//

HINSTANCE hInstance;
LPTSTR    glpList;
TCHAR szEnvDomainName[]   = TEXT("USERDOMAIN");
TCHAR szEnvUserName[]     = TEXT("USERNAME");
TCHAR szEnvComputerName[] = TEXT("COMPUTERNAME");
TCHAR szProfileRegInfo[]  = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon");
TCHAR szProfileType[]     = TEXT("ProfileType");
TCHAR szProfilePath[]     = TEXT("ProfilePath");
TCHAR szSaveList[]        = TEXT("SaveList");
TCHAR szDontSaveList[]    = TEXT("DontSaveList");
TCHAR szSaveOnUnlisted[]  = TEXT("SaveOnUnlisted");
TCHAR szComma[]           = TEXT(",");
TCHAR szOne[]             = TEXT("1");
TCHAR szZero[]            = TEXT("0");
TCHAR szShellHelp[]       = TEXT("ShellHelp");
TCHAR szHelpFileName[]    = TEXT("profile.hlp");
UINT  uiShellHelp;
BOOL  bUserMadeAChange;

//*************************************************************
//
//  RunApplet()
//
//  Purpose:    Called when the user runs the Profile Applet
//
//  Parameters: HWND hwnd - Window handle
//
//
//  Return:     void
//
//*************************************************************

void RunApplet(HWND hwnd)
{
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_PROFILE), hwnd,
             (DLGPROC)ProfileDlgProc);
}

//*************************************************************
//
//  ProfileDlgProc()
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

LRESULT CALLBACK ProfileDlgProc(HWND hDlg, UINT message,
                                WPARAM wParam, LPARAM lParam)
{
        switch (message)
           {
           case WM_INITDIALOG:
              return (InitializeDialog(hDlg));

           case WM_COMMAND:
              switch (LOWORD(wParam)) {
                 case IDD_DONTSAVECHANGE:
                 case IDD_SAVECHANGE:
                     {
                     WORD   wListBoxID, wIndex;
                     BOOL   bFound;

                     //
                     // Determine which "change" button the user pressed.
                     //

                     if (LOWORD(wParam) == IDD_DONTSAVECHANGE) {
                        wListBoxID = IDD_DONTSAVELIST;

                     } else {
                        wListBoxID = IDD_SAVELIST;
                     }

                     //
                     // Create a list of names to pass to the edit dialog box.
                     // NamesDlgProc will free this memory for us.
                     //

                     glpList = CreateList (hDlg, wListBoxID, NULL, &bFound);

                     if (!glpList) {
                        KdPrint(("Received a null pointer from CreateList"));
                        break;
                     }

                     //
                     // Find index of highlighted name so it is highlighted
                     // in the change dialog box also.
                     //

                     wIndex = (WORD) SendDlgItemMessage (hDlg, wListBoxID,
                                                         LB_GETCURSEL, 0, 0);

                     if (wIndex == (WORD)LB_ERR) {
                        wIndex = 0;
                     }

                     //
                     //  Display the Computer Names dialog box.
                     //

                     if (DialogBoxParam (hInstance,
                                         MAKEINTRESOURCE(IDD_COMPUTERNAMES),
                                         hDlg, (DLGPROC)NamesDlgProc, wIndex)) {
                        //
                        // User pressed OK button, so update the list box.
                        //

                        ParseAndAddComputerNames(hDlg, wListBoxID, glpList);

                        //
                        // Free the memory allocated by CreateList inside of
                        // NamesDlgProc
                        //

                        GlobalFree (glpList);

                        //
                        // Update the global state.
                        //

                        bUserMadeAChange = TRUE;
                     }

                     }
                     break;

                 case IDD_SAVELIST:
                 case IDD_DONTSAVELIST:
                     {
                     WORD   wButtonID;

                     //
                     // We are only interested in double click messages.
                     //

                     if (HIWORD (wParam) != LBN_DBLCLK) {
                        break;
                     }

                     //
                     // Determine which listbox was double clicked.
                     //

                     if (LOWORD(wParam) == IDD_SAVELIST) {
                        wButtonID = IDD_SAVECHANGE;

                     } else {
                        wButtonID = IDD_DONTSAVECHANGE;
                     }

                     //
                     // Post a message to post up the change dialog box.
                     //

                     PostMessage (hDlg, WM_COMMAND, MAKELONG (wButtonID,0), 0);
                     }
                     break;

                 case IDD_DEFAULTSAVE:
                 case IDD_DEFAULTDONTSAVE:
                     bUserMadeAChange = TRUE;
                     break;

                 case IDOK:

                     //
                     //  Notify winhelp that we are leaving, save settings
                     //  and leave if appropriate.
                     //

                     WinHelp (hDlg, szHelpFileName, HELP_QUIT, 0);

                     //
                     // If the user didn't make any changes, then
                     // return FALSE through EndDialog.
                     //

                     if (!bUserMadeAChange) {
                         EndDialog(hDlg, FALSE);
                     } else {
                         if (SaveSettings(hDlg)) {
                            EndDialog(hDlg, TRUE);
                         }
                     }
                     return TRUE;

                 case IDCANCEL:

                     //
                     //  Notify winhelp that we are leaving and exit
                     //

                     WinHelp (hDlg, szHelpFileName, HELP_QUIT, 0);
                     EndDialog(hDlg, FALSE);
                     return TRUE;

                 case IDD_HELP:

                     //
                     // User requested help.
                     //

                     WinHelp (hDlg, szHelpFileName, HELP_CONTENTS, 0);
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
//  InitializeDialog()
//
//  Purpose:    Initializes the profile dialog box
//
//  Parameters: HWND hDlg - Dialog box window handle
//      
//
//  Return:     TRUE - successfully initialized
//              FALSE - failed to initialize
//
//*************************************************************
BOOL InitializeDialog (HWND hDlg)
{
    TCHAR szDomainName [MAX_DOMAIN_NAME];
    TCHAR szUserName [MAX_USER_NAME];
    TCHAR szTempBuffer [MAX_TEMP_BUFFER];
    TCHAR szUnknown [UNKNOWN_LEN];
    TCHAR szFormatBuffer [MAX_TEMP_BUFFER];
    LONG  lResult;
    HKEY  hKey;
    DWORD dwDisp, dwType, dwMaxBufferSize;


    //
    // Initialize the save changes global.
    //

    bUserMadeAChange = FALSE;

    //
    // Load the "unknown" string from the resources
    //

    if (!LoadString (hInstance, IDS_UNKNOWN, szUnknown, UNKNOWN_LEN)) {
        KdPrint(("Failed to load 'unknown' string."));
        return FALSE;
    }

    //
    // Retrieve the domain name
    //

    if (!GetEnvironmentVariable (szEnvDomainName, szDomainName,
                                 MAX_DOMAIN_NAME)){
        //
        // Failed to get the domain name.  Initalize to Unknown.
        //

        lstrcpy (szDomainName, szUnknown);
    }

    //
    // Retrieve the user name
    //

    if (!GetEnvironmentVariable (szEnvUserName, szUserName,
                                 MAX_USER_NAME)){
        //
        // Failed to get the user name.  Initalize to Unknown.
        //

        lstrcpy (szUserName, szUnknown);
    }


    //
    // Load the format string from the resources
    //

    if (!LoadString (hInstance, IDS_FORMAT, szFormatBuffer, MAX_TEMP_BUFFER)) {
        KdPrint(("Failed to load format string."));
        return FALSE;
    }


    //
    // Build the name field, and add it to the dialog box
    //

    wsprintf (szTempBuffer, szFormatBuffer, szDomainName, szUserName);
    SetDlgItemText (hDlg, IDD_USERNAME, szTempBuffer);

    //
    // Retrieve the local computer name
    //

    if (!GetEnvironmentVariable (szEnvComputerName, szTempBuffer,
                                 MAX_TEMP_BUFFER)){
        //
        // Failed to get the computer name.  Initalize to Unknown.
        //

        lstrcpy (szTempBuffer, szUnknown);
    }
    SetDlgItemText (hDlg, IDD_COMPUTERNAME, szTempBuffer);


    //
    // Now query the registry
    //
    // First we need a key
    //

    lResult = RegCreateKeyEx (HKEY_CURRENT_USER, szProfileRegInfo, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                              NULL, &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
       KdPrint(("Failed to open registry key."));
       return FALSE;
    }


    //
    // Query and set the profile path
    //

    dwMaxBufferSize = MAX_TEMP_BUFFER;
    szTempBuffer[0] = TEXT('\0');
    RegQueryValueEx (hKey, szProfilePath , NULL, &dwType,
                    (LPBYTE) szTempBuffer, &dwMaxBufferSize);

    if (szTempBuffer[0] == TEXT('\0')) {
       lstrcpy (szTempBuffer, szUnknown);
    }

    SetDlgItemText (hDlg, IDD_PATH, szTempBuffer);


    //
    // Query and set the computer names to save the profile from
    //

    dwMaxBufferSize = MAX_TEMP_BUFFER;
    szTempBuffer[0] = TEXT('\0');
    RegQueryValueEx (hKey, szSaveList , NULL, &dwType,
                    (LPBYTE) szTempBuffer, &dwMaxBufferSize);

    if (szTempBuffer[0] != TEXT('\0')) {
       ParseAndAddComputerNames(hDlg, IDD_SAVELIST, szTempBuffer);
    }


    //
    // Query and set the computer names to not save the profile from
    //

    dwMaxBufferSize = MAX_TEMP_BUFFER;
    szTempBuffer[0] = TEXT('\0');
    RegQueryValueEx (hKey, szDontSaveList , NULL, &dwType,
                    (LPBYTE) szTempBuffer, &dwMaxBufferSize);

    if (szTempBuffer[0] != TEXT('\0')) {
       ParseAndAddComputerNames(hDlg, IDD_DONTSAVELIST, szTempBuffer);
    }


    //
    // Query and set the default choice
    //

    dwMaxBufferSize = MAX_TEMP_BUFFER;
    szTempBuffer[0] = TEXT('\0');
    RegQueryValueEx (hKey, szSaveOnUnlisted , NULL, &dwType,
                    (LPBYTE) szTempBuffer, &dwMaxBufferSize);

    //
    // If the buffer still has NULL or one in it, then the default
    // case is to always save the profile.
    //

    if ( (szTempBuffer[0] == TEXT('\0')) || (szTempBuffer[0] == TEXT('1')) ) {
       CheckDlgButton (hDlg, IDD_DEFAULTSAVE, 1);
    } else {
       CheckDlgButton (hDlg, IDD_DEFAULTDONTSAVE, 1);
    }


    //
    // Close the registry key
    //

    RegCloseKey (hKey);


    //
    // Return success
    //

    return TRUE;

}

//*************************************************************
//
//  ParseAndAddComputerNames()
//
//  Purpose:    Parse the list of computer names, and add them
//              to the list box.
//
//  Parameters: HWND hDlg      - Window handle of dialog box
//              WORD idListBox - ListBox id
//              LPTSTR szNames - List of names separated by commas
//      
//
//  Return:     void
//
//*************************************************************

void ParseAndAddComputerNames(HWND hDlg, WORD idListBox, LPTSTR szNames)
{
    LPTSTR lpHead, lpTail;
    TCHAR  chLetter;

    //
    // Turn off redraw, and empty the listbox.
    //

    SendDlgItemMessage (hDlg, idListBox, WM_SETREDRAW, 0, 0);
    SendDlgItemMessage (hDlg, idListBox, LB_RESETCONTENT, 0, 0);

    lpHead = lpTail = szNames;

    while (*lpHead) {

        //
        // Search for the comma, or the end of the list.
        //

        while (*lpHead != TEXT(',') && *lpHead) {
            lpHead++;
        }

        //
        // If the head pointer is not pointing at the
        // tail pointer, then we have something to add
        // to the list.
        //

        if (lpHead != lpTail) {

            //
            // Store the letter pointed to by lpHead in a temporary
            // variable (chLetter).  Replace that letter with NULL,
            // and add the string to the list box starting from
            // lpTail.  After the string is added, replace the letter
            // we used.
            //

            chLetter = *lpHead;
            *lpHead = TEXT('\0');
            SendDlgItemMessage (hDlg, idListBox, LB_ADDSTRING, 0,
                                (LPARAM) lpTail);
            *lpHead = chLetter;
        }

        //
        // If we are not at the end of the list, then move the
        // head pointer forward one character.
        //

        if (*lpHead) {
           lpHead++;
        }


        //
        // Move the tail pointer to the head pointer
        //

        lpTail = lpHead;
    }

    //
    // Turn redraw back on, and display the results.
    //

    SendDlgItemMessage (hDlg, idListBox, WM_SETREDRAW, 1, 0);
    InvalidateRect (GetDlgItem(hDlg, idListBox), NULL, TRUE);

}

//*************************************************************
//
//  SaveSettings()
//
//  Purpose:    Saves the current selections to the registry
//
//  Parameters: HWND hDlg - Handle to dialog box
//      
//
//  Return:     BOOL - TRUE ok to exit dialog box
//                     FALSE do not exit dialog box
//
//*************************************************************
BOOL SaveSettings (HWND hDlg)
{
    LONG   lResult;
    HKEY   hKey = 0;
    DWORD  dwDisp;
    TCHAR  szProfile [PROFILE_NAME_LEN];
    TCHAR  szTempBuffer [MAX_TEMP_BUFFER];
    TCHAR  szTempBuffer2 [MAX_TEMP_BUFFER];
    TCHAR  szComputerName [MAX_COMPUTER_NAME];
    TCHAR  szSpecificErrorMsg [MAX_ERROR_MSG];
    LPTSTR lpSaveList, lpDoNotSaveList;
    WORD   wError, wMBStyle;
    INT    iMBResult;
    BOOL   bFound = FALSE;
    BOOL   bSaveOnUnlisted;

    //
    // Check for computer names that exist in both listboxs.
    //

    if (!CompareLists (hDlg, IDD_SAVELIST, IDD_DONTSAVELIST)) {
       KdPrint(("One or more computer names exist in both lists."));
       wError = IDS_DUPLICATENAME;
       goto AbortSave;
    }

    //
    // Retreive the computer name from the dialog box
    //

    GetDlgItemText (hDlg, IDD_COMPUTERNAME, szComputerName, MAX_COMPUTER_NAME);


    //
    // Check the default flag
    //

    bSaveOnUnlisted = IsDlgButtonChecked (hDlg, IDD_DEFAULTSAVE);


    //
    // Create the list of computer names on which the profile will not be saved.
    //

    lpDoNotSaveList = CreateList (hDlg, IDD_DONTSAVELIST, szComputerName,
                                  &bFound);


    //
    // Check for NULL pointer
    //

    if (!lpDoNotSaveList) {
       KdPrint(("Don't save list:  Received a null pointer from CreateList"));
       wError = IDS_UNABLETOSAVE;
       goto AbortSave;
    }


    //
    // Check to see if the computer was in the list.  If so, this is
    // an error case.
    //

    if (bFound) {
       KdPrint(("This computer is in the Do Not Save Profile List"));
       wError = IDS_NAMEINDONTLIST;
       goto AbortSave;
    }


    //
    // Create the list of computer names on which the profile will be saved.
    //

    lpSaveList = CreateList (hDlg, IDD_SAVELIST, szComputerName, &bFound);


    //
    // Check for NULL pointer
    //

    if (!lpSaveList) {
       KdPrint(("Do save list:  Received a null pointer from CreateList"));
       wError = IDS_UNABLETOSAVE;
       goto AbortSave;
    }

    //
    // Check to see if the computer was in the list.  If not and the
    // user has "Do Not save on unlisted computer" turned on, then this
    // is an error case.
    //

    if (!bFound && !bSaveOnUnlisted) {
       KdPrint(("This computer is not in the Save Profile List and Do Not save on Unlisted computer is turned on"));
       wError = IDS_NONAMEANDDONOTSAVE;
       goto AbortSave;
    }


    //
    // Open the registry key
    //

    lResult = RegCreateKeyEx (HKEY_CURRENT_USER, szProfileRegInfo, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                              NULL, &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
       KdPrint(("Failed to open registry key."));
       wError = IDS_UNABLETOSAVE;
       goto AbortSave;
    }


    //
    // Write the Do Not Save list to the registry
    //

    RegSetValueEx (hKey, szDontSaveList, 0, REG_SZ,
                  (LPBYTE) lpDoNotSaveList,
                  sizeof (TCHAR) * lstrlen (lpDoNotSaveList) + 1);


    //
    // Write the Save list to the registry
    //

    RegSetValueEx (hKey, szSaveList, 0, REG_SZ,
                  (LPBYTE) lpSaveList,
                  sizeof (TCHAR) * lstrlen (lpSaveList) + 1);


    //
    // Write the default setting to the registry
    //

    if (bSaveOnUnlisted) {
        lstrcpy (szTempBuffer, szOne);
    } else {
        lstrcpy (szTempBuffer, szZero);
    }

    RegSetValueEx (hKey, szSaveOnUnlisted, 0, REG_SZ,
                  (LPBYTE) szTempBuffer,
                  sizeof (TCHAR) * lstrlen (szTempBuffer) + 1);


    //
    // Close the registry key
    //

    RegCloseKey (hKey);


    //
    // Success.  Now we need to tell the user to log off all other computers,
    // and then log off of this one.
    //

    wError = IDS_LOGOFFNOTICE;

AbortSave:


    //
    // Free up the memory allocated by CreateList
    //

    GlobalFree (lpSaveList);
    GlobalFree (lpDoNotSaveList);


    //
    // Load the error string and message box title
    //

    if (LoadString (hInstance, IDS_NAME, szProfile, PROFILE_NAME_LEN) &&
        LoadString (hInstance, IDS_BASEERRORMSG, szTempBuffer, MAX_TEMP_BUFFER) &&
        LoadString (hInstance, wError, szSpecificErrorMsg, MAX_ERROR_MSG)) {

        //
        // If it is a user error, then we want to prompt with Yes/No buttons
        // otherwise offer an OK button.
        //

        if (wError == IDS_UNABLETOSAVE) {
            wMBStyle = MB_OK | MB_ICONEXCLAMATION;
            lstrcpy (szTempBuffer2, szSpecificErrorMsg);

        } else if (wError == IDS_LOGOFFNOTICE) {
            wMBStyle = MB_OK | MB_ICONASTERISK;
            lstrcpy (szTempBuffer2, szSpecificErrorMsg);

        } else {
            wMBStyle = MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION;
            wsprintf (szTempBuffer2, szTempBuffer, szSpecificErrorMsg);
        }

        //
        // Display the message
        //

        iMBResult = MessageBox (hDlg, szTempBuffer2, szProfile,
                                wMBStyle);


        //
        // If the user chooses NO, the we don't want to exit the
        // dialog box.
        //

        if (iMBResult == IDNO) {
            return FALSE;
        } else {
            return TRUE;
        }
    }

}

//*************************************************************
//
//  CreateList()
//
//  Purpose:    Creates a list of computer names from the
//              given listbox.  Each name is seperated by
//              a comma.  Also, if the given computer name
//              is found in the list, the bool variable is
//              set.
//
//  Parameters: HWND   hDlg           - Window handle
//              WORD   idListBox      - ID of listbox
//              LPTSTR szComputerName - Name of computer to watch for
//              LPBOOL lpFound        - Flag to set if found
//      
//
//  Return:     LPTSTR - Pointer to buffer if successful
//                       NULL if not
//
//*************************************************************

LPTSTR CreateList (HWND hDlg, WORD idListBox, LPTSTR szComputerName,
                   LPBOOL lpFound)
{
    LPTSTR lpList;
    WORD   wCount, wIndex;
    TCHAR  szName[MAX_COMPUTER_NAME];

    //
    // Initialize the flag
    //

    *lpFound = FALSE;

    //
    // Get the number of items in the listbox
    //

    wCount = (WORD) SendDlgItemMessage (hDlg, idListBox, LB_GETCOUNT, 0, 0);

    //
    // Allocate a buffer to use
    //

    lpList = GlobalAlloc (GPTR, (wCount ? wCount : 1) * (MAX_COMPUTER_NAME * sizeof(TCHAR)));

    if (!lpList) {
        KdPrint(("CreateList:  Received a null pointer from GlobalAlloc"));
        return NULL;
    }

    //
    // Null terminate
    //

    lpList[0] = TEXT('\0');

    for (wIndex = 0; wIndex < wCount; wIndex++) {

        //
        // Retreive and item from the list
        //

        SendDlgItemMessage (hDlg, idListBox, LB_GETTEXT, wIndex,
                           (LPARAM) szName);

        //
        // Check to see if we found the requested name.
        //

        if (!lstrcmpi (szComputerName, szName)) {
            *lpFound = TRUE;
        }

        //
        // Add name to the end of the buffer
        //
        lstrcat (lpList, szName);

        //
        // If we are not adding the last item, then insert a comma
        //

        if (wIndex != (wCount - 1)) {
           lstrcat (lpList, szComma);
        }
    }

    //
    // Success
    //

    return (lpList);
}

//*************************************************************
//
//  CompareLists()
//
//  Purpose:    Compares one listbox to the other listbox
//              looking for names that appear in both.
//
//  Parameters: HWND hDlg - Window handle of dialog box
//              WORD idList1 - ID of first listbox
//              WORD idList2 - ID of second listbox
//      
//
//  Return:     BOOL - TRUE if not duplicates exist
//                     FALSE if a duplicate does exist
//
//*************************************************************

BOOL CompareLists (HWND hDlg, WORD idList1, WORD idList2)
{
    WORD wList1Count, wIndex;
    TCHAR szName[MAX_COMPUTER_NAME];

    //
    // Get the number of items in the first listbox
    //

    wList1Count = (WORD)SendDlgItemMessage (hDlg, idList1, LB_GETCOUNT, 0, 0);

    if (wList1Count == LB_ERR) {
       return TRUE;
    }

    //
    // Loop through listbox 1 comparing with listbox 2
    //

    for (wIndex=0; wIndex < wList1Count; wIndex++) {

        SendDlgItemMessage (hDlg, idList1, LB_GETTEXT, wIndex, (LPARAM)szName);

        if (SendDlgItemMessage (hDlg, idList2, LB_FINDSTRINGEXACT,
                                0, (LPARAM)szName) != LB_ERR) {
           return FALSE;
       }
    }

    return TRUE;
}
