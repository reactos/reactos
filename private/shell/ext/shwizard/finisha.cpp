#include <windows.h>
#include <windowsx.h>
#include <string.h>
#include <prsht.h>

#include "winreg.h"
#include "resource.h"
#include "shwizard.h"

INT_PTR APIENTRY FinishAProc (HWND, UINT, WPARAM, LPARAM);

void FinishA_OnCancel       (HWND);
void FinishA_OnInitDialog   (HWND);
void FinishA_OnSetActive    (HWND);
void FinishA_OnWizardBack   (HWND);
void FinishA_OnWizardFinish (HWND);
void FinishA_FinalDisplay   (HWND);


INT_PTR APIENTRY FinishAProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_COMMAND:
            break;
        case WM_INITDIALOG:
            FinishA_OnInitDialog(hDlg);
            break;
        case WM_NOTIFY:
            switch (((NMHDR FAR *)lParam)->code) {
                case PSN_KILLACTIVE:
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
                    return(1);
                    break;
                case PSN_QUERYCANCEL:
                    FinishA_OnCancel(hDlg);
                    return(FALSE);          // must do this to allow cancel to complete
                case PSN_RESET:
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
                    break;
                case PSN_SETACTIVE:
                    FinishA_OnSetActive(hDlg);
                    break;
                case PSN_WIZBACK:
                    FinishA_OnWizardBack(hDlg);
                    break;
                case PSN_WIZFINISH:
                    FinishA_OnWizardFinish(hDlg);
                    break;
                default:
                    return(FALSE);
            }
            break;
        default:
            return(FALSE);
    }

    return(TRUE);   

}   /*  end StartPage() */


void FinishA_OnInitDialog (HWND hDlg)
{

}   /*  end FinishA_OnInitDialog() */


void FinishA_OnSetActive (HWND hDlg)
{
    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_FINISH);
    FinishA_FinalDisplay(hDlg);

}   /*  end FinishA_OnSetActive() */


void FinishA_OnCancel (HWND hDlg)
{
    if (g_bTemplateCopied)
        DeleteFile(g_szFullHTMLFile);

}   /*  end FinishA_OnCancel() */


void FinishA_OnWizardBack (HWND hDlg)
{
    // Remove desktop.ini only if we had copied it during this wizard session
    if (g_bTemplateCopied) {
        DeleteFile(g_szFullHTMLFile);
        g_bTemplateCopied = FALSE;
    }

    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_PAGEA3);

}   /*  end FinishA_OnWizardBack() */


void FinishA_OnWizardFinish (HWND hDlg)
{
    UpdateAddWebView();     // Update desktop.ini

}   /*  end FinishA_OnWizardFinished() */


// Sets the appropriate text message in the dialog
void FinishA_FinalDisplay (HWND hDlg)
{
    TCHAR szFinalDisplay[MAX_PATH], szTemp[MAX_PATH];

    LoadString(g_hAppInst, IDS_TEMPLATE_DONE1, szFinalDisplay, ARRAYSIZE(szFinalDisplay));

    // Add more info.
    if (g_iFlagA == CUSTOM_TEMPLATE)  // A custom template was chosen.
    {
        LoadString(g_hAppInst, IDS_TEMPLATE_DONE2, szTemp, ARRAYSIZE(szTemp));
    }
    else
    {
        LoadString(g_hAppInst, IDS_TEMPLATE_DONE3, szTemp, ARRAYSIZE(szTemp));
    }
    lstrcat(szFinalDisplay, TEXT("\r\n\r\n"));
    lstrcat(szFinalDisplay, szTemp);

    SetDlgItemText(hDlg, IDC_FINISHA, szFinalDisplay);
}   /*  end FinalDisplay() */
