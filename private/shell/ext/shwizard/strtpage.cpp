#include "shwizard.h"
#include <string.h>
#include <tchar.h>

#include <shlguidp.h>
#include <shellids.h>
#include <shlwapi.h>

#include "winreg.h"

void CCTF_ChoosePath::Describe(UINT uiDescId)
{
    if (uiDescId)
    {
        TCHAR szText[MAX_PATH];
        LoadString(g_hAppInst, uiDescId, szText, ARRAYSIZE(szText));

        SendMessage(GetDlgItem(_hwndDlg, IDC_INTRO_DESC), WM_SETTEXT, 0, (LPARAM)szText);
    }
}


void GetHTMLFileName(void)
{
    GetTemporaryTemplatePath(g_szFullHTMLFile);
}

typedef struct tagMapIDDIDC {
    DWORD idd;
    DWORD idc;
    TCHAR szRegValue[30];
} MapIDDIDC;

MapIDDIDC aDialogs[] = {
    {IDD_PAGEA3, IDC_WEBVIEW_TEMPLATE, REG_VAL_TEMPLATE_CHECKED},
    {IDD_PAGET1, IDC_LISTVIEW_STUFF, REG_VAL_LISTVIEW_CHECKED},
    {IDD_COMMENT, IDC_COMMENT, REG_VAL_COMMENT_CHECKED}};

void CCTF_ChoosePath::OnInit()
{
    // Set the default radio button
    Button_SetCheck(GetDlgItem(_hwndDlg, IDC_CUSTOMIZE), BST_CHECKED);
    Describe(IDS_CUSTOMIZE);

    // Set the default status of the check boxes
    for (int i = 0; i < ARRAYSIZE(aDialogs); i++)
    {
        DWORD dwType, dwChecked, cbData = sizeof(dwChecked);

        if (!(ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, REG_FC_WIZARD, aDialogs[i].szRegValue, &dwType, &dwChecked, &cbData)))
        {
            dwChecked = (aDialogs[i].idc == IDC_WEBVIEW_TEMPLATE);
        }
        Button_SetCheck(GetDlgItem(_hwndDlg, aDialogs[i].idc), dwChecked ? BST_CHECKED : BST_UNCHECKED);
        _pCommonInfo->SetPathChoice(aDialogs[i].idd, dwChecked);
    }

    // Disable IDC_REMOVE if not valid
    if (!IsBackgroundImageSet() && !IsIconTextColorSet() && !IsFolderCommentSet() && !IsWebViewTemplateSet())
    {
        EnableWindow(GetDlgItem(_hwndDlg, IDC_REMOVE), FALSE);
    }
    _pCommonInfo->OnSetActive(_hwndDlg);    // Set the wizard buttons properly now.
    
    GetHTMLFileName();          // pre-load file paths
}

INT_PTR APIENTRY CCTF_ChoosePath::WndProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL bRet = TRUE;
    CCTF_ChoosePath* pChoosePath = NULL;

    pChoosePath = (CCTF_ChoosePath*)GetWindowPtr(hwndDlg, GWLP_USERDATA);
    //ASSERT(pChoosePath);
    //ASSERT(pChoosePath->_hwndDlg == hwndDlg);

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        CCTF_CommonInfo* pCommonInfo = (CCTF_CommonInfo*)((LPPROPSHEETPAGE)lParam)->lParam;
        //ASSERT(pCommonInfo);
        pChoosePath = new CCTF_ChoosePath(hwndDlg, pCommonInfo);
        if (pChoosePath)
        {
            SetWindowPtr(hwndDlg, GWLP_USERDATA, pChoosePath);
            pChoosePath->OnInit();
        }
        else
        {
            bRet = FALSE ;
        }
        break;
    }

    case WM_DESTROY:
    {
        SetWindowPtr(hwndDlg, GWLP_USERDATA, NULL);
        delete pChoosePath;
        break;
    }
    
    case WM_COMMAND:
    {
        UINT uiDescId = IDS_CUSTOMIZE;  // Default to the desc for the customization option
        DWORD dwChecked;

        switch (LOWORD(wParam))
        {
        case IDC_CUSTOMIZE:
            pChoosePath->_pCommonInfo->SetPath(IDC_CUSTOMIZE);
            break;
        case IDC_REMOVE:
            pChoosePath->_pCommonInfo->SetPath(IDD_REMOVE);
            uiDescId = IDS_UNCUSTOMIZE;
            break;
        case IDC_LISTVIEW_STUFF:
            pChoosePath->_pCommonInfo->SetPathChoice(IDD_PAGET1, dwChecked = BOOLIFY(IsDlgButtonChecked(hwndDlg, IDC_LISTVIEW_STUFF)));
            SHSetValue(HKEY_CURRENT_USER, REG_FC_WIZARD, REG_VAL_LISTVIEW_CHECKED, REG_DWORD, (LPBYTE)&dwChecked, SIZEOF(dwChecked));
            break;
        case IDC_COMMENT:
            pChoosePath->_pCommonInfo->SetPathChoice(IDD_COMMENT, dwChecked = BOOLIFY(IsDlgButtonChecked(hwndDlg, IDC_COMMENT)));
            SHSetValue(HKEY_CURRENT_USER, REG_FC_WIZARD, REG_VAL_COMMENT_CHECKED, REG_DWORD, (LPBYTE)&dwChecked, SIZEOF(dwChecked));
            break;
        case IDC_WEBVIEW_TEMPLATE:
            pChoosePath->_pCommonInfo->SetPathChoice(IDD_PAGEA3, dwChecked = BOOLIFY(IsDlgButtonChecked(hwndDlg, IDC_WEBVIEW_TEMPLATE)));
            SHSetValue(HKEY_CURRENT_USER, REG_FC_WIZARD, REG_VAL_TEMPLATE_CHECKED, REG_DWORD, (LPBYTE)&dwChecked, SIZEOF(dwChecked));
            break;
        }
        pChoosePath->Describe(uiDescId);
        BOOL bEnableCheckBoxes = TRUE;
        if (IsDlgButtonChecked(hwndDlg, IDC_REMOVE))
        {
            // Disable the check boxes
            bEnableCheckBoxes = FALSE;
        }
        EnableWindow(GetDlgItem(hwndDlg, IDC_LISTVIEW_STUFF), bEnableCheckBoxes);
        EnableWindow(GetDlgItem(hwndDlg, IDC_COMMENT), bEnableCheckBoxes);
        EnableWindow(GetDlgItem(hwndDlg, IDC_WEBVIEW_TEMPLATE), bEnableCheckBoxes);
        pChoosePath->_pCommonInfo->OnSetActive(hwndDlg);
        break;
    }
    
    case WM_NOTIFY:
    {
        UINT uiCode = ((NMHDR FAR *)lParam)->code;
        switch (uiCode)
        {
        case PSN_QUERYCANCEL:
            bRet = FALSE;
        case PSN_KILLACTIVE:
        case PSN_RESET:
            pChoosePath->_pCommonInfo->OnCancel(hwndDlg);
            break;
        case PSN_SETACTIVE:
            pChoosePath->_pCommonInfo->OnSetActive(hwndDlg);
            break;
        case PSN_WIZNEXT:
            pChoosePath->_pCommonInfo->OnNext(hwndDlg);
            break;
        case PSN_WIZBACK:
            pChoosePath->_pCommonInfo->OnBack(hwndDlg);
            break;
        default:
            return(FALSE);
        }
        break;
    }
    default:
        bRet = FALSE;
        break;
    }
    return bRet;   
}

