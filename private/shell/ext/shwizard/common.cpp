#include "shwizard.h"

HFONT CCTF_CommonInfo::GetTitleFont()
{
    return _hTitleFont;
}

BOOL CCTF_CommonInfo::WasThisOptionalPathUsed(UINT uiPath)
{
    BOOL bUsed = FALSE;
    for (int i = 0; i < ARRAYSIZE(_uiPathChoices); i++)
    {
        if (_uiPathChoices[i].bChoice && (_uiPathChoices[i].ui == uiPath))
        {
            bUsed = TRUE;
            break;
        }
    }
    return bUsed;
}

BOOL CCTF_CommonInfo::WasThisFeatureUnCustomized(UINT uiFeature)
{
    BOOL bUnCustomized = FALSE;
    for (int i = 0; i < ARRAYSIZE(_uiFeatures); i++)
    {
        if (_uiFeatures[i].bChoice && (_uiFeatures[i].ui == uiFeature))
        {
            bUnCustomized = TRUE;
            break;
        }
    }
    return bUnCustomized;
}

BOOL CCTF_CommonInfo::SetPathChoice(UINT uiPath, BOOL bChoice)
{
    BOOL bSet = FALSE;
    for (int i = 0; i < ARRAYSIZE(_uiPathChoices); i++)
    {
        if (_uiPathChoices[i].ui == uiPath)
        {
            _uiPathChoices[i].bChoice = bChoice;
            bSet = TRUE;
            break;
        }
    }
    return bSet;
}

BOOL CCTF_CommonInfo::SetUnCustomizedFeature(UINT uiFeature, BOOL bUnCustomized)
{
    BOOL bSet = FALSE;
    for (int i = 0; i < ARRAYSIZE(_uiFeatures); i++)
    {
        if (_uiFeatures[i].ui == uiFeature)
        {
            _uiFeatures[i].bChoice = bUnCustomized;
            bSet = TRUE;
            break;
        }
    }
    return bSet;
}

void CCTF_CommonInfo::SetPath(UINT uiPath)
{
    _uiPath = uiPath;
}

static const UINT auiPageSequence[] = {IDD_WELCOME, IDD_CHOOSE_PATH, IDD_REMOVE, IDD_FINISHR};

UINT CCTF_CommonInfo::GetNextPrevPage_Helper(BOOL bNext)
{
    UINT uiNextPrevPage = 0;
    if ((_uiCurrentPage != IDD_CHOOSE_PATH) || (_uiPath != IDC_CUSTOMIZE) || !bNext)
    {
        for (int i = 0; i < ARRAYSIZE(auiPageSequence); i++)
        {
            if (auiPageSequence[i] == _uiCurrentPage)
            {
                if (bNext)  // Next
                {
                    if (i < (ARRAYSIZE(auiPageSequence) - 1))
                    {
                        uiNextPrevPage = auiPageSequence[i + 1];
                    }
                }
                else    // Previous
                {
                    if (i > 0)
                    {
                        uiNextPrevPage = auiPageSequence[i - 1];
                    }
                }
                break;
            }
        }
    }
    return uiNextPrevPage;
}

UINT CCTF_CommonInfo::GetNextPage_Helper(int iChoice)
{
    UINT uiNextPage = IDD_FINISHT;
    if (iChoice < ARRAYSIZE(_uiPathChoices))
    {
        uiNextPage = _uiPathChoices[iChoice].bChoice ? 
                _uiPathChoices[iChoice].ui : GetNextPage_Helper(iChoice + 1);
    }
    return uiNextPage;
}

UINT CCTF_CommonInfo::GetPrevPage_Helper(int iChoice)
{
    UINT uiPrevPage = IDD_CHOOSE_PATH;
    if (iChoice >= 0)
    {
        uiPrevPage = _uiPathChoices[iChoice].bChoice ?
                _uiPathChoices[iChoice].ui : GetPrevPage_Helper(iChoice - 1);
    }
    return uiPrevPage;
}

UINT CCTF_CommonInfo::GetNextPage()
{
    UINT uiNextPage = GetNextPrevPage_Helper(TRUE);
    if (!uiNextPage)
    {
        for (int i = 0; i < ARRAYSIZE(_uiPathChoices); i++)
        {
            if (_uiPathChoices[i].bChoice)
            {
                if (_uiCurrentPage == IDD_CHOOSE_PATH)
                {
                    uiNextPage = GetNextPage_Helper(0);
                    break;
                }
                else if (_uiCurrentPage == _uiPathChoices[i].ui)
                {
                    uiNextPage = GetNextPage_Helper(i + 1);
                    break;
                }
            }
        }
    }
    return uiNextPage;
}

UINT CCTF_CommonInfo::GetPrevPage()
{
    UINT uiPrevPage = GetNextPrevPage_Helper(FALSE);
    if (!uiPrevPage)
    {
        for (int i = 0; i < ARRAYSIZE(_uiPathChoices); i++)
        {
            if (_uiPathChoices[i].bChoice)
            {
                if (_uiCurrentPage == IDD_FINISHT)
                {
                    uiPrevPage = GetPrevPage_Helper(ARRAYSIZE(_uiPathChoices) - 1);
                    break;
                }
                else if (_uiCurrentPage == _uiPathChoices[i].ui)
                {
                    uiPrevPage = GetPrevPage_Helper(i - 1);
                    break;
                }
            }
        }
    }
    return uiPrevPage;
}

UINT CCTF_CommonInfo::OnNext(HWND hwndDlg)
{
    UINT uiNextPage = GetNextPage();
    //ASSERT(uiNextPage);
    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, uiNextPage);
    _uiCurrentPage = uiNextPage;
    return uiNextPage;
}

UINT CCTF_CommonInfo::OnBack(HWND hwndDlg)
{
    UINT uiPrevPage = GetPrevPage();
    //ASSERT(uiPrevPage);
    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, uiPrevPage);
    _uiCurrentPage = uiPrevPage;
    return uiPrevPage;
}

void CCTF_CommonInfo::OnSetActive(HWND hwndDlg)
{
    DWORD dwButtons = 0;
    UINT uiNextPage = GetNextPage();
    if (uiNextPage)
    {
        dwButtons |= PSWIZB_NEXT;
    }
    else
    {
        dwButtons |= PSWIZB_FINISH;
    }
    if (GetPrevPage())
    {
        dwButtons |= PSWIZB_BACK;
    }
    PropSheet_SetWizButtons(GetParent(hwndDlg), dwButtons);
    if (!uiNextPage)
    {
        EnableWindow(GetDlgItem(GetParent(hwndDlg), ID_WIZNEXT), FALSE);
    }
}

void CCTF_CommonInfo::OnFinishCustomization(HWND hwndDlg)
{
    _bCustomized = TRUE;
}

void CCTF_CommonInfo::OnFinishUnCustomization(HWND hwndDlg)
{
    _bUnCustomized = TRUE;
}

void CCTF_CommonInfo::OnCancel(HWND hwndDlg)
{
    _bCustomized = FALSE;
    _bUnCustomized = FALSE;
}

BOOL CCTF_CommonInfo::WasItCustomized()
{
    return _bCustomized;
}

BOOL CCTF_CommonInfo::WasItUnCustomized()
{
    return _bUnCustomized;
}

CCTF_CommonInfo::CCTF_CommonInfo()
{
    // Figure out the font for the titles on the intro and ending pages and store it in _hTitleFont
    NONCLIENTMETRICS ncm = {0};
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
    
    // Create the welcome/finish title font
    LOGFONT titleLogFont = ncm.lfMessageFont;
    titleLogFont.lfWeight = FW_BOLD;
    LoadString(g_hAppInst, IDS_WELCOME_TITLE_FONT, titleLogFont.lfFaceName, ARRAYSIZE(titleLogFont.lfFaceName));
    HDC hdc = GetDC(NULL);  //gets the screen DC
    int fontSize = 12;      // Should be size 12 (see wiz97 spec)
    titleLogFont.lfHeight = 0 - GetDeviceCaps(hdc, LOGPIXELSY) * fontSize / 72;
    _hTitleFont = CreateFontIndirect(&titleLogFont);
    ReleaseDC(NULL, hdc);

    _uiCurrentPage = IDD_WELCOME;
    _uiPathChoices[0].ui = IDD_PAGEA3;
    _uiPathChoices[0].bChoice = FALSE;
    _uiPathChoices[1].ui = IDD_PAGET1;
    _uiPathChoices[1].bChoice = FALSE;
    _uiPathChoices[2].ui = IDD_COMMENT;
    _uiPathChoices[2].bChoice = FALSE;
    _uiFeatures[0].ui = IDC_RESTORE_HTML;
    _uiFeatures[0].bChoice = FALSE;
    _uiFeatures[1].ui = IDC_REMOVE_BACKGROUND;
    _uiFeatures[1].bChoice = FALSE;
    _uiFeatures[2].ui = IDC_RESTORE_ICONTEXT;
    _uiFeatures[2].bChoice = FALSE;
    _uiFeatures[3].ui = IDC_REMOVE_COMMENT;
    _uiFeatures[3].bChoice = FALSE;
    _uiPath = IDC_CUSTOMIZE;
    _bCustomized = FALSE;
    _bUnCustomized = FALSE;
}

CCTF_CommonInfo::~CCTF_CommonInfo()
{
    if (_hTitleFont)
    {
        // Destroy the fonts
        DeleteObject(_hTitleFont);
    }
}

