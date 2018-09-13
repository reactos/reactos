#include "shwizard.h"
#include <shlwapi.h>

void CCTFWiz_FinishCustomization::OnInit()
{
    HFONT hTitleFont = _pCommonInfo->GetTitleFont();
    //ASSERT(hTitleFont);
    // It's an intro page, so set the title font
    SetWindowFont(GetDlgItem(_hwndDlg, IDC_TITLE_WELCOME), hTitleFont, TRUE);
}

void CCTFWiz_FinishCustomization::OnSetActive()
{
    // First, the the common OnSetActive().
    _pCommonInfo->OnSetActive(_hwndDlg);

    // Set the appropriate text message in the dialog
    TCHAR szFinalDisplay[MAX_PATH*2], szTemp[MAX_PATH];

    szFinalDisplay[0] = TEXT('\0');
    if (_pCommonInfo->WasThisOptionalPathUsed(IDD_PAGEA3))
    {
        LoadString(g_hAppInst, IDS_COMPLETE_TEMPLATE, szTemp, ARRAYSIZE(szTemp));
        StrCatBuff(szFinalDisplay, szTemp, ARRAYSIZE(szFinalDisplay));
        StrCatBuff(szFinalDisplay, TEXT("\n"), ARRAYSIZE(szFinalDisplay));
    }
    if (_pCommonInfo->WasThisOptionalPathUsed(IDD_PAGET1))
    {
        LoadString(g_hAppInst, IDS_COMPLETE_BACKGROUND, szTemp, ARRAYSIZE(szTemp));
        StrCatBuff(szFinalDisplay, szTemp, ARRAYSIZE(szFinalDisplay));
        StrCatBuff(szFinalDisplay, TEXT("\n"), ARRAYSIZE(szFinalDisplay));
    }
    if (_pCommonInfo->WasThisOptionalPathUsed(IDD_COMMENT))
    {
        LoadString(g_hAppInst, IDS_COMPLETE_COMMENT, szTemp, ARRAYSIZE(szTemp));
        StrCatBuff(szFinalDisplay, szTemp, ARRAYSIZE(szFinalDisplay));
        StrCatBuff(szFinalDisplay, TEXT("\n"), ARRAYSIZE(szFinalDisplay));
    }
    // Display the string
    SetWindowText(GetDlgItem(_hwndDlg, IDC_FINISHT), szFinalDisplay);
}

INT_PTR APIENTRY CCTFWiz_FinishCustomization::WndProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL bRet = TRUE;
    CCTFWiz_FinishCustomization* pFinishCustomization = NULL;

    pFinishCustomization = (CCTFWiz_FinishCustomization*)GetWindowPtr(hwndDlg, GWLP_USERDATA);
    //ASSERT(pFinishCustomization);
    //ASSERT(pFinishCustomization->_hwndDlg == hwndDlg);

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        CCTF_CommonInfo* pCommonInfo = (CCTF_CommonInfo*)((LPPROPSHEETPAGE)lParam)->lParam;
        //ASSERT(pCommonInfo);
        pFinishCustomization = new CCTFWiz_FinishCustomization(hwndDlg, pCommonInfo);
        if (pFinishCustomization)
        {
            SetWindowPtr(hwndDlg, GWLP_USERDATA, pFinishCustomization);
            pFinishCustomization->OnInit();
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
        delete pFinishCustomization;
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
            pFinishCustomization->_pCommonInfo->OnCancel(hwndDlg);
            bRet = FALSE;   // must do this to allow cancel to complete
            break;
        case PSN_SETACTIVE:
            pFinishCustomization->OnSetActive();
            break;
        case PSN_WIZBACK:
            pFinishCustomization->_pCommonInfo->OnBack(hwndDlg);
            break;
        case PSN_WIZFINISH:
            pFinishCustomization->_pCommonInfo->OnFinishCustomization(hwndDlg);
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

