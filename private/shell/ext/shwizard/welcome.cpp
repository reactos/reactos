#include "shwizard.h"

void CCTFWiz_Welcome::OnInit()
{
    HFONT hTitleFont = _pCommonInfo->GetTitleFont();
    //ASSERT(hTitleFont);
    // It's an intro page, so set the title font
    SetWindowFont(GetDlgItem(_hwndDlg, IDC_TITLE_WELCOME), hTitleFont, TRUE);
}

INT_PTR APIENTRY CCTFWiz_Welcome::WndProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL bRet = TRUE;
    CCTFWiz_Welcome* pWelcome = NULL;

    pWelcome = (CCTFWiz_Welcome*)GetWindowPtr(hwndDlg, GWLP_USERDATA);
    //ASSERT(pWelcome);
    //ASSERT(pWelcome->_hwndDlg == hwndDlg);

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        CCTF_CommonInfo* pCommonInfo = (CCTF_CommonInfo*)((LPPROPSHEETPAGE)lParam)->lParam;
        //ASSERT(pCommonInfo);
        pWelcome = new CCTFWiz_Welcome(hwndDlg, pCommonInfo);
        if (pWelcome)
        {
            SetWindowPtr(hwndDlg, GWLP_USERDATA, pWelcome);
            pWelcome->OnInit();
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
        delete pWelcome;
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
            pWelcome->_pCommonInfo->OnCancel(hwndDlg);
            break;
        case PSN_SETACTIVE:
            pWelcome->_pCommonInfo->OnSetActive(hwndDlg);
            break;
        case PSN_WIZNEXT:
            pWelcome->_pCommonInfo->OnNext(hwndDlg);
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
    return(bRet);
}

