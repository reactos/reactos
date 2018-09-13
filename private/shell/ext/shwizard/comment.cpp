#include "shwizard.h"
#include <wininet.h>

void Comment_OnInitDialog(HWND hDlg)
{
    TCHAR szComment[INTERNET_MAX_URL_LENGTH * 3];
    GetCurrentComment(szComment, ARRAYSIZE(szComment));
    SendMessage(GetDlgItem(hDlg, IDC_EDIT_COMMENT), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)szComment);
}

void SaveComment(HWND hDlg)
{
    TCHAR szComment[INTERNET_MAX_URL_LENGTH * 3];
    SendMessage(GetDlgItem(hDlg, IDC_EDIT_COMMENT), WM_GETTEXT, (WPARAM)ARRAYSIZE(szComment), (LPARAM)szComment);
    UpdateComment(szComment);
}

INT_PTR APIENTRY Comment_WndProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL bRet = TRUE;
    switch (msg)
    {
    case WM_INITDIALOG:
        Comment_OnInitDialog(hDlg);
        break;
    case WM_DESTROY:
    {
        if (g_pCommonInfo->WasItCustomized())
        {
            SaveComment(hDlg);
        }
        break;
    }
    case WM_NOTIFY:
    {
        switch (((NMHDR FAR *)lParam)->code)
        {
        case PSN_QUERYCANCEL:
            bRet = FALSE;
        case PSN_KILLACTIVE:
        case PSN_RESET:
            g_pCommonInfo->OnCancel(hDlg);
            break;
        case PSN_SETACTIVE:
            g_pCommonInfo->OnSetActive(hDlg);
            break;
        case PSN_WIZBACK:
            g_pCommonInfo->OnBack(hDlg);
            break;
        case PSN_WIZNEXT:
            g_pCommonInfo->OnNext(hDlg);
            break;
        default:
            bRet = FALSE;
            break;
        }
        break;
    }
    default:
        bRet = FALSE;
        break;
    }
    return bRet;
}

