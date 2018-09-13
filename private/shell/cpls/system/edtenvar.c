/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    edtenvar.c

Abstract:

    Implements the Edit Environment Variables dialog of the
    System Control Panel Applet

Author:

    Scott Hallock (scotthal) 11-Nov-1997

Revision History:

--*/
#include "sysdm.h"

//
// Global Variables
//
UINT g_VarType = INVALID_VAR_TYPE;
UINT g_EditType = INVALID_EDIT_TYPE;
TCHAR g_szVarName[BUFZ];
TCHAR g_szVarValue[BUFZ];

//
// Help IDs
//
DWORD aEditEnvVarsHelpIds[] = {
    IDC_ENVVAR_EDIT_NAME_LABEL,  (IDH_ENV_EDIT + 0),
    IDC_ENVVAR_EDIT_NAME,        (IDH_ENV_EDIT + 0),
    IDC_ENVVAR_EDIT_VALUE_LABEL, (IDH_ENV_EDIT + 1),
    IDC_ENVVAR_EDIT_VALUE,       (IDH_ENV_EDIT + 1),
    0, 0
};

//
// Function prototypes
//
BOOL
InitEnvVarsEdit(
    IN HWND hDlg
);

BOOL
EnvVarsEditHandleCommand(
    IN HWND hDlg,
    IN WPARAM wParam,
    IN LPARAM lParam
);

//
// Function implementation
//
INT_PTR
APIENTRY
EnvVarsEditDlg(
    IN HWND hDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
)
/*++

Routine Description:

    Handles messages sent to the New.../Edit... dialog.

Arguments:

    hDlg -
        Supplies the window handle

    uMsg -
        Supplies the message being sent

    wParam -
        Supplies message parameter

    lParam -
        Supplies message parameter

Return Value:

    TRUE if message was handled.
    FALSE if message was unhandled.

--*/
{
    BOOL fInitializing = FALSE;

    switch (uMsg) {
        case WM_INITDIALOG: {
            BOOL fSuccess = FALSE;

            fInitializing = TRUE;

            fSuccess = InitEnvVarsEdit(hDlg);
            if (!fSuccess) {
                EndDialog(hDlg, EDIT_ERROR);
            } // if

            fInitializing = FALSE;
            break;
        } // case WM_INITDIALOG

        case WM_COMMAND:
            return EnvVarsEditHandleCommand(hDlg, wParam, lParam);
            break;

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP, (DWORD_PTR) (LPSTR) aEditEnvVarsHelpIds);
            break;
    
        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) aEditEnvVarsHelpIds);
            break;

        default:
            return(FALSE);
            break;

    } // switch (uMsg)

    return(TRUE);
}

BOOL
InitEnvVarsEdit(
    IN HWND hDlg
)
/*++

Routine Description:

    Initializes the Edit Environment Variables dialog by placing initial
    values into the text editing controls if necessary.

Arguments:

    hDlg -
        Supplies window handle

Return Value:

    TRUE if successful
    FALSE if an error occurs

--*/
{
    TCHAR szCaption[EDIT_ENVVAR_CAPTION_LENGTH];
    LRESULT fRetVal = FALSE;
    INT  nResult = 0;

    ASSERT(INVALID_EDIT_TYPE != g_EditType);
    ASSERT(INVALID_VAR_TYPE != g_VarType);

    __try {

        fRetVal = SendMessage(
            GetDlgItem(hDlg, IDC_ENVVAR_EDIT_NAME),
            EM_LIMITTEXT,
            MAX_PATH - 1,
            0
        );
        fRetVal = SendMessage(
            GetDlgItem(hDlg, IDC_ENVVAR_EDIT_VALUE),
            EM_LIMITTEXT,
            MAX_VALUE_LEN - 1,
            0
        );

        switch (g_EditType) {
            //
            // If this is to be a New.. dialog, we only need to
            // load the proper capiton for the variable type
            //
            case NEW_VAR:

                switch (g_VarType) {
                    case SYSTEM_VAR:
                        nResult = LoadString(
                            hInstance,
                            IDS_NEW_SYSVAR_CAPTION,
                            szCaption,
                            EDIT_ENVVAR_CAPTION_LENGTH
                        );
                        break;

                    case USER_VAR:
                        nResult = LoadString(
                            hInstance,
                            IDS_NEW_USERVAR_CAPTION,
                            szCaption,
                            EDIT_ENVVAR_CAPTION_LENGTH
                        );
                        break;

                    default:
                        __leave;
                        break;
                } // switch (g_VarType)

                //
                // Set the focus to the "Name" field
                // SetFocus() doesn't want to work here
                //
                PostMessage(
                    hDlg, 
                    WM_NEXTDLGCTL,
                    (WPARAM) GetDlgItem(hDlg, IDC_ENVVAR_EDIT_NAME),
                    (LPARAM) TRUE
                );

                break;

            //
            // If this is to be an Edit.. dialog, then we need to load the
            // proper caption and fill in initial values for the edit
            // controls
            //
            case EDIT_VAR:

                switch (g_VarType) {
                    case SYSTEM_VAR:
                        nResult = LoadString(
                            hInstance,
                            IDS_EDIT_SYSVAR_CAPTION,
                            szCaption,
                            EDIT_ENVVAR_CAPTION_LENGTH
                        );
                        break;

                     case USER_VAR:
                        nResult = LoadString(
                            hInstance,
                            IDS_EDIT_USERVAR_CAPTION,
                            szCaption,
                            EDIT_ENVVAR_CAPTION_LENGTH
                        );
                        break;
    
                    default:
                         __leave;
                        break;
                } // switch (g_VarType)
        
                SetDlgItemText(
                    hDlg,
                    IDC_ENVVAR_EDIT_NAME,
                    g_szVarName
                );
                SetDlgItemText(
                    hDlg,
                    IDC_ENVVAR_EDIT_VALUE,
                    g_szVarValue
                );

                //
                // Set the focus to the "Value" field
                // SetFocus() doesn't want to work here
                //
                PostMessage(
                    hDlg, 
                    WM_NEXTDLGCTL,
                    (WPARAM) GetDlgItem(hDlg, IDC_ENVVAR_EDIT_VALUE),
                    (LPARAM) TRUE
                );
        
                break;
        } // switch (g_EditType)
        
        fRetVal = SendMessage(
            GetDlgItem(hDlg, IDC_ENVVAR_EDIT_NAME),
            EM_SETSEL,
            0,
            -1
        );
        fRetVal = SendMessage(
            GetDlgItem(hDlg, IDC_ENVVAR_EDIT_VALUE),
            EM_SETSEL,
            0,
            -1
        );

        fRetVal = SetWindowText(hDlg, szCaption);

    } // __try
    __finally {
        //
        // Nothing to clean up.  __try is only there for __leave on
        // failure capability.
        //
    } // __finally

    return fRetVal ? TRUE : FALSE;
}

BOOL
EnvVarsEditHandleCommand(
    IN HWND hDlg,
    IN WPARAM wParam,
    IN LPARAM lParam
)
/*++

Routine Description:

    Handles WM_COMMAND messages sent to the Edit Environment Variables
    dialog

Arguments:

    hDlg -
        Supplies window handle

    wParam -
        Supplies message parameter

    lParam -
        Supplies message parameter

Return Value:

    TRUE if message was handled
    FALSE if message was unhandled

--*/
{
    switch (LOWORD(wParam)) {

        case IDOK:
            GetDlgItemText(
                hDlg,
                IDC_ENVVAR_EDIT_NAME,
                g_szVarName,
                BUFZ
            );
            GetDlgItemText(
                hDlg,
                IDC_ENVVAR_EDIT_VALUE,
                g_szVarValue,
                BUFZ
            );
            EndDialog(hDlg, EDIT_CHANGE);
            break;

        case IDCANCEL:
            EndDialog(hDlg, EDIT_NO_CHANGE);
            break;

        default:
            return(FALSE);
            break;

    } // switch (LOWORD(wParam))

    return(TRUE);
}        
