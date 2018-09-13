#include "private.h"   // Class Definitions 



BOOL PromptToGoOffline(VOID);
BOOL PromptToGoOnline(VOID);
BOOL CALLBACK GoOfflinePromptDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
        LPARAM lParam);
BOOL CALLBACK GoOnlinePromptDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
        LPARAM lParam);

// Make sure that Notification Sinks are apartment model and hence are always called back
// on the same thread making it unnecessary to use any critical sections -- BUGBUG





// TRUE means that the state now is Online
// FALSE means that the user chose to remain Offline

BOOL
OnConnectedNotification(void)
{
    BOOL fRet = TRUE;
            
    // Check to see if the user is offline and wants to "go online"
    if((IsGlobalOffline()))
    {
        //Ask the user with a dialog
        fRet = PromptToGoOnline();
    }  

    return fRet;
}

// TRUE means that the state now is Offline
// FALSE means that the user chose to remain Online

BOOL
OnDisconnectedNotification(void)
{      
    BOOL fRet = TRUE;
    // Check to see if the user wants to go offline
    if(!(IsGlobalOffline()))
    {
        //Ask the user with a dialog, if the user says yes, then
        // toggle to offline mode by calling wininet
        fRet = PromptToGoOffline();
    }

        
    return fRet;

}
    

BOOL PromptToGoOffline(VOID)
{

    // run the dialog
    BOOL fRet = DialogBoxParam(MLGetHinst(),MAKEINTRESOURCE(IDD_GO_OFFLINE_DLG),
            NULL,GoOfflinePromptDlgProc,(LPARAM) 0);

    return fRet;
}

/*******************************************************************

        NAME:           GoOfflinePromptDlgProc

        SYNOPSIS:       Dialog proc for Go Offline dialog

********************************************************************/
BOOL CALLBACK GoOfflinePromptDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
        LPARAM lParam)
{
    switch (uMsg) {
    case WM_INITDIALOG:
        SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        return TRUE;
        break;
    case WM_COMMAND:
        switch (wParam) {
        case IDOK:
            SetGlobalOffline(TRUE); 
            EndDialog(hDlg,TRUE);
            return TRUE;
            break;
        case IDCANCEL:            
            EndDialog(hDlg,FALSE);
            return TRUE;
            break;
        default:
            break;
        }
        break;
   default:
        break;
   }

    return FALSE;
}


BOOL PromptToGoOnline(VOID)
{

    // run the dialog
    BOOL fRet = DialogBoxParam(MLGetHinst(),MAKEINTRESOURCE(IDD_GO_ONLINE_DLG),
            NULL,GoOnlinePromptDlgProc,(LPARAM) 0);

    return fRet;
}

/*******************************************************************

        NAME:           GoOnlinePromptDlgProc

        SYNOPSIS:       Dialog proc for Go Online dialog

********************************************************************/
BOOL CALLBACK GoOnlinePromptDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
        LPARAM lParam)
{
    switch (uMsg) {
    case WM_INITDIALOG:
        return TRUE;
        break;
    case WM_COMMAND:
        switch (wParam) {
        case IDOK:
            SetGlobalOffline(FALSE); 
            EndDialog(hDlg,TRUE);
            return TRUE;
            break;
        case IDCANCEL: 
            EndDialog(hDlg,FALSE);
            return TRUE;
            break;
         default:
            break;
        }
        break;
    default:
        break;
    }

    return FALSE;
}
