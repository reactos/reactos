#include "shwizard.h"

void Remove_OnInitDialog(HWND hwndDlg);
void Remove_OnSetActive(HWND hwndDlg);
void Remove_OnNext(HWND hwndDlg);
void Remove_OnFinish(HWND hwndDlg);

INT_PTR APIENTRY RemoveProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL bRet = TRUE;
    switch (msg)
    {
    case WM_INITDIALOG:
        Remove_OnInitDialog(hwndDlg);
        break;
    case WM_DESTROY:
        if (g_pCommonInfo->WasItUnCustomized())
        {
            Remove_OnFinish(hwndDlg);   // Update desktop.ini
        }
        break;
    case WM_NOTIFY:
    {
        switch (((NMHDR FAR *)lParam)->code)
        {
        case PSN_QUERYCANCEL:
            bRet = FALSE;
        case PSN_KILLACTIVE:
        case PSN_RESET:
            g_pCommonInfo->OnCancel(hwndDlg);
            break;
        case PSN_SETACTIVE:
            Remove_OnSetActive(hwndDlg);
            break;
        case PSN_WIZNEXT:
            Remove_OnNext(hwndDlg);
            break;
        case PSN_WIZBACK:
            g_pCommonInfo->OnBack(hwndDlg);
            break;
        default:
            bRet = FALSE;
        }
        break;
    }
    default:
        bRet = FALSE;
    }
    return bRet;
}

void Remove_OnInitDialog (HWND hwndDlg)
{
    Button_SetCheck(GetDlgItem(hwndDlg, IDC_RESTORE_HTML), BST_CHECKED);
    if (!IsWebViewTemplateSet())
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_RESTORE_HTML), FALSE);
    }
    Button_SetCheck(GetDlgItem(hwndDlg, IDC_REMOVE_BACKGROUND), BST_CHECKED);
    if (!IsBackgroundImageSet())
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVE_BACKGROUND), FALSE);
    }
    Button_SetCheck(GetDlgItem(hwndDlg, IDC_RESTORE_ICONTEXT), BST_CHECKED);
    if (!IsIconTextColorSet())
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_RESTORE_ICONTEXT), FALSE);
    }
    Button_SetCheck(GetDlgItem(hwndDlg, IDC_REMOVE_COMMENT), BST_CHECKED);
    if (!IsFolderCommentSet())
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVE_COMMENT), FALSE);
    }
}

void Remove_OnSetActive(HWND hwndDlg)
{
    g_pCommonInfo->SetUnCustomizedFeature(IDC_RESTORE_HTML, FALSE);
    g_pCommonInfo->SetUnCustomizedFeature(IDC_REMOVE_BACKGROUND, FALSE);
    g_pCommonInfo->SetUnCustomizedFeature(IDC_RESTORE_ICONTEXT, FALSE);
    g_pCommonInfo->SetUnCustomizedFeature(IDC_REMOVE_COMMENT, FALSE);
    g_pCommonInfo->OnSetActive(hwndDlg);
}

void Remove_OnNext(HWND hwndDlg)
{
    if (IsWindowEnabled(GetDlgItem(hwndDlg, IDC_RESTORE_HTML)) && IsDlgButtonChecked(hwndDlg, IDC_RESTORE_HTML))
    {
        g_pCommonInfo->SetUnCustomizedFeature(IDC_RESTORE_HTML, TRUE);
    }
    if (IsWindowEnabled(GetDlgItem(hwndDlg, IDC_REMOVE_BACKGROUND)) && IsDlgButtonChecked(hwndDlg, IDC_REMOVE_BACKGROUND))
    {
        g_pCommonInfo->SetUnCustomizedFeature(IDC_REMOVE_BACKGROUND, TRUE);
    }
    if (IsWindowEnabled(GetDlgItem(hwndDlg, IDC_RESTORE_ICONTEXT)) && IsDlgButtonChecked(hwndDlg, IDC_RESTORE_ICONTEXT))
    {
        g_pCommonInfo->SetUnCustomizedFeature(IDC_RESTORE_ICONTEXT, TRUE);
    }
    if (IsWindowEnabled(GetDlgItem(hwndDlg, IDC_REMOVE_COMMENT)) && IsDlgButtonChecked(hwndDlg, IDC_REMOVE_COMMENT))
    {
        g_pCommonInfo->SetUnCustomizedFeature(IDC_REMOVE_COMMENT, TRUE);
    }
    g_pCommonInfo->OnNext(hwndDlg);
}

void Remove_OnFinish(HWND hwndDlg)
{
    if (g_pCommonInfo->WasThisFeatureUnCustomized(IDC_RESTORE_HTML))
    {
        RemoveWebViewTemplateSettings();
    }
    if (g_pCommonInfo->WasThisFeatureUnCustomized(IDC_REMOVE_BACKGROUND))
    {
        RemoveBackgroundImage();
    }
    if (g_pCommonInfo->WasThisFeatureUnCustomized(IDC_RESTORE_ICONTEXT))
    {
        RestoreIconTextColor();
    }
    if (g_pCommonInfo->WasThisFeatureUnCustomized(IDC_REMOVE_COMMENT))
    {
        RemoveFolderComment();
    }
}

