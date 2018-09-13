#include "shwizard.h"
#include <shlwapi.h>

void CCTFWiz_FinishUnCustomization::OnInit()
{
    HFONT hTitleFont = _pCommonInfo->GetTitleFont();
    //ASSERT(hTitleFont);
    // It's an intro page, so set the title font
    SetWindowFont(GetDlgItem(_hwndDlg, IDC_TITLE_WELCOME), hTitleFont, TRUE);
}

void CCTFWiz_FinishUnCustomization::OnSetActive()
{
    // First, the the common OnSetActive().
    _pCommonInfo->OnSetActive(_hwndDlg);

    // Set the appropriate text message in the dialog
    TCHAR szFinalDisplay[MAX_PATH*2], szTemp[MAX_PATH];
    szFinalDisplay[0] = TEXT('\0');
    if (_pCommonInfo->WasThisFeatureUnCustomized(IDC_RESTORE_HTML))
    {
        LoadString(g_hAppInst, IDS_RESTORE_HTML, szTemp, ARRAYSIZE(szTemp));
        StrCatBuff(szFinalDisplay, szTemp, ARRAYSIZE(szFinalDisplay));
        StrCatBuff(szFinalDisplay, TEXT("\n"), ARRAYSIZE(szFinalDisplay));
    }
    if (_pCommonInfo->WasThisFeatureUnCustomized(IDC_REMOVE_BACKGROUND))
    {
        LoadString(g_hAppInst, IDS_REMOVE_BACKGROUND, szTemp, ARRAYSIZE(szTemp));
        StrCatBuff(szFinalDisplay, szTemp, ARRAYSIZE(szFinalDisplay));
        StrCatBuff(szFinalDisplay, TEXT("\n"), ARRAYSIZE(szFinalDisplay));
    }
    if (_pCommonInfo->WasThisFeatureUnCustomized(IDC_RESTORE_ICONTEXT))
    {
        LoadString(g_hAppInst, IDS_RESTORE_ICONTEXTCOLOR, szTemp, ARRAYSIZE(szTemp));
        StrCatBuff(szFinalDisplay, szTemp, ARRAYSIZE(szFinalDisplay));
        StrCatBuff(szFinalDisplay, TEXT("\n"), ARRAYSIZE(szFinalDisplay));
    }
    if (_pCommonInfo->WasThisFeatureUnCustomized(IDC_REMOVE_COMMENT))
    {
        LoadString(g_hAppInst, IDS_REMOVE_COMMENT, szTemp, ARRAYSIZE(szTemp));
        StrCatBuff(szFinalDisplay, szTemp, ARRAYSIZE(szFinalDisplay));
        StrCatBuff(szFinalDisplay, TEXT("\n"), ARRAYSIZE(szFinalDisplay));
    }
    // Display the string
    SetWindowText(GetDlgItem(_hwndDlg, IDC_FINISHT), szFinalDisplay);
}

INT_PTR APIENTRY CCTFWiz_FinishUnCustomization::WndProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL bRet = TRUE;
    CCTFWiz_FinishUnCustomization* pFinishUnCustomization = NULL;

    pFinishUnCustomization = (CCTFWiz_FinishUnCustomization*)GetWindowPtr(hwndDlg, GWLP_USERDATA);
    //ASSERT(pFinishUnCustomization);
    //ASSERT(pFinishUnCustomization->_hwndDlg == hwndDlg);

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        CCTF_CommonInfo* pCommonInfo = (CCTF_CommonInfo*)((LPPROPSHEETPAGE)lParam)->lParam;
        //ASSERT(pCommonInfo);
        pFinishUnCustomization = new CCTFWiz_FinishUnCustomization(hwndDlg, pCommonInfo);
        if (pFinishUnCustomization)
        {
            SetWindowPtr(hwndDlg, GWLP_USERDATA, pFinishUnCustomization);
            pFinishUnCustomization->OnInit();
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
        delete pFinishUnCustomization;
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
            pFinishUnCustomization->_pCommonInfo->OnCancel(hwndDlg);
            bRet = FALSE;   // must do this to allow cancel to complete
            break;
        case PSN_SETACTIVE:
            pFinishUnCustomization->OnSetActive();
            break;
        case PSN_WIZBACK:
            pFinishUnCustomization->_pCommonInfo->OnBack(hwndDlg);
            break;
        case PSN_WIZFINISH:
            pFinishUnCustomization->_pCommonInfo->OnFinishUnCustomization(hwndDlg);
            break;
        default:
            bRet = FALSE ;
            break;
        }
        break;
    }
    default:
        bRet = FALSE ;
        break;
    }
    return bRet;   
}
