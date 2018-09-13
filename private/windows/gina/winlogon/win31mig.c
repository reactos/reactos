/****************************** Module Header ******************************\
* Module Name: logon.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Implements functions to allow a user to control migration of
* Windows 3.1 configuration information from the .INI, .GRP and REG.DAT
* files into the Windows/NT when the logon for the first time.
*
* History:
* 02-23-93 Stevewo      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

typedef struct _WIN31_MIGRATION_DIALOG {
    PTERMINAL    pTerm;
    DWORD       Win31MigrationFlags;
} WIN31_MIGRATION_DIALOG, * PWIN31_MIGRATION_DIALOG;

//
// Private prototypes
//

INT_PTR WINAPI
Win31MigrationDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );


/***************************************************************************\
* FUNCTION: IsWin9xUpgrade
*
* PURPOSE:  Checks if this OS was a Win9x upgrade.
*
* RETURNS:  TRUE/FALSE
*
* HISTORY:
*
*   02-16-99 jimschm      Created.
*
\***************************************************************************/

BOOL
IsWin9xUpgrade (
    VOID
    )
{
    HKEY Key;
    LONG rc;
    DWORD Size;
    DWORD ValueData;
    BOOL b = FALSE;

    rc = RegOpenKeyEx (
            HKEY_LOCAL_MACHINE,
            WINLOGON_KEY,
            0,
            KEY_READ,
            &Key
            );

    if (rc != ERROR_SUCCESS) {
        DebugLog((DEB_ERROR, "Unable to open %s\n", WINLOGON_KEY));
        return FALSE;
    }


    Size = sizeof (ValueData);

    rc = RegQueryValueEx (
            Key,
            TEXT("win9xupg"),
            NULL,
            NULL,
            (PBYTE) &ValueData,
            &Size
            );

    if (rc == ERROR_SUCCESS && ValueData) {
        b = TRUE;
    }

    RegCloseKey (Key);

    return b;
}




/***************************************************************************\
* FUNCTION: Windows31Migration
*
* PURPOSE:  Checks to see if there is any Windows 3.1 data to
*           migrate to the Windows/NT registry, and if so,
*           puts up a dialog box for the user to control the
*           process and watch it happen.
*
* RETURNS:  TRUE/FALSE
*
* HISTORY:
*
*   02-23-93 Stevewo      Created.
*
\***************************************************************************/

BOOL
Windows31Migration(
    PTERMINAL pTerm
    )
{
    HANDLE ImpersonationHandle;
    WIN31_MIGRATION_DIALOG  DialogInfo;
    DWORD Win31MigrationFlags;
    BOOL bDisplayDialog = TRUE;

    if (IsWin9xUpgrade()) {
        return(TRUE);
    }

    //
    // Get in the correct context before we reference the registry
    //

    ImpersonationHandle = ImpersonateUser(&pTerm->pWinStaWinlogon->UserProcessData, NULL);
    if (ImpersonationHandle == NULL) {
        DebugLog((DEB_ERROR, "Win31Migration failed to impersonate user for query\n"));
        return(TRUE);
    }

    Win31MigrationFlags = QueryWindows31FilesMigration( Win31LogonEvent );

    StopImpersonating(ImpersonationHandle);

    if (Win31MigrationFlags == 0) {
        return(TRUE);
    }

    DialogInfo.pTerm = pTerm;
    DialogInfo.Win31MigrationFlags = Win31MigrationFlags;

    return WlxDialogBoxParam(pTerm,
                             g_hInstance,
                            (LPTSTR) MAKEINTRESOURCE(IDD_WIN31MIG),
                            NULL,
                            Win31MigrationDlgProc,
                            (LPARAM)&DialogInfo
                            );
}


BOOL WINAPI
Win31MigrationStatusCallback(
    IN PWSTR Status,
    IN PVOID CallbackParameter
    )
{
    HWND hDlg = (HWND)CallbackParameter;

    return SetDlgItemTextW(hDlg, IDD_WIN31MIG_STATUS, Status);
}



/***************************************************************************\
* FUNCTION: Win31MigrationDlgProc
*
* PURPOSE:  Processes messages for Windows 3.1 Migration dialog
*
* RETURNS:  TRUE/FALSE
*
* HISTORY:
*
*   02-23-93 Stevewo      Created.
*
\***************************************************************************/

INT_PTR WINAPI
Win31MigrationDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PWIN31_MIGRATION_DIALOG pDialogInfo = (PWIN31_MIGRATION_DIALOG) GetWindowLongPtr(hDlg, GWLP_USERDATA);
    HANDLE ImpersonationHandle;
    UINT idFocus = (UINT) -1;

    switch (message) {

    case WM_INITDIALOG:

        pDialogInfo = (PWIN31_MIGRATION_DIALOG) lParam;
        //
        // Make sure that we don't get killed by a stray SAS
        //
        SetMapperFlag(hDlg, MAPPERFLAG_WINLOGON, pDialogInfo->pTerm);

        SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);


        if (pDialogInfo->Win31MigrationFlags & WIN31_MIGRATE_INIFILES) {
            CheckDlgButton(hDlg, idFocus = IDD_WIN31MIG_INIFILES, 1 );
        } else {
            CheckDlgButton(hDlg, IDD_WIN31MIG_INIFILES, 0 );
        }

        if (pDialogInfo->Win31MigrationFlags & WIN31_MIGRATE_GROUPS) {
            CheckDlgButton(hDlg, idFocus = IDD_WIN31MIG_GROUPS, 1 );
        } else {
            CheckDlgButton(hDlg, IDD_WIN31MIG_GROUPS, 0 );
        }

        CentreWindow(hDlg);
        if ( idFocus != (UINT) -1 )
        {
            SetFocus(GetDlgItem(hDlg, idFocus));
        }


        return(TRUE);

    case WM_COMMAND:

        switch (HIWORD(wParam)) {

        default:

            switch (LOWORD(wParam)) {

            case IDOK:
                pDialogInfo->Win31MigrationFlags = 0;
                if (IsDlgButtonChecked(hDlg, IDD_WIN31MIG_INIFILES) == 1) {
                    pDialogInfo->Win31MigrationFlags |= WIN31_MIGRATE_INIFILES;
                }

                if (IsDlgButtonChecked(hDlg, IDD_WIN31MIG_GROUPS) == 1) {
                    pDialogInfo->Win31MigrationFlags |= WIN31_MIGRATE_GROUPS;
                }

                if (pDialogInfo->Win31MigrationFlags != 0) {

                    SetCursor( LoadCursor( NULL, IDC_WAIT ) );

                    //
                    // Get in the correct context before we reference the registry
                    //

                    ImpersonationHandle = ImpersonateUser(&pDialogInfo->pTerm->pWinStaWinlogon->UserProcessData, NULL);
                    if (ImpersonationHandle == NULL) {
                        DebugLog((DEB_ERROR, "Win31MigrationDlgProc failed to impersonate user for update\n"));
                        EndDialog(hDlg, DLG_FAILURE);
                        return(TRUE);
                    }

                    OpenProfileUserMapping();

                    SynchronizeWindows31FilesAndWindowsNTRegistry( Win31LogonEvent,
                                                                   pDialogInfo->Win31MigrationFlags,
                                                                   Win31MigrationStatusCallback,
                                                                   hDlg
                                                                 );

                    CloseProfileUserMapping();
                    StopImpersonating(ImpersonationHandle);

                    //
                    // If they migrated the groups, then we need to set
                    // the group convert flag so links are created.
                    //


                    if (pDialogInfo->Win31MigrationFlags & WIN31_MIGRATE_GROUPS) {
                        HKEY hKey;
                        DWORD dwDisp;
                        BOOL bRunGrpConv;

                        if (OpenHKeyCurrentUser(pDialogInfo->pTerm->pWinStaWinlogon)) {

                            if (RegCreateKeyEx (pDialogInfo->pTerm->pWinStaWinlogon->UserProcessData.hCurrentUser,
                                                WINLOGON_KEY,
                                                0, NULL, REG_OPTION_NON_VOLATILE,
                                                KEY_ALL_ACCESS, NULL, &hKey, &dwDisp) ==
                                                ERROR_SUCCESS) {


                                bRunGrpConv = TRUE;

                                RegSetValueEx (hKey, TEXT("RunGrpConv"), 0, REG_DWORD,
                                              (LPBYTE) &bRunGrpConv, sizeof(bRunGrpConv));

                                RegCloseKey (hKey);
                            }

                            CloseHKeyCurrentUser(pDialogInfo->pTerm->pWinStaWinlogon);
                        }
                    }

                    SetCursor( LoadCursor( NULL, IDC_ARROW ) );
                }

                EndDialog(hDlg, DLG_SUCCESS);

                if (pDialogInfo->Win31MigrationFlags & WIN31_MIGRATE_INIFILES) {
                    InitSystemParametersInfo(pDialogInfo->pTerm, TRUE);
                }

                return(TRUE);

            case IDCANCEL:
                EndDialog(hDlg, DLG_FAILURE);
                return(TRUE);

            }
            break;

        }
        break;

    case WLX_WM_SAS:
        // Ignore it
        return(TRUE);

    case WM_PAINT:
        break;  // Fall through to do default processing
                // We may have validated part of the window.
    }

    // We didn't process the message
    return(FALSE);
}
