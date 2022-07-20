/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/dialogs.cpp
 * PURPOSE:     Window procedures of the dialog windows plus launching functions
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

#include "dialogs.h"

#include <winnls.h>

/* GLOBALS **********************************************************/

CMirrorRotateDialog mirrorRotateDialog;
CAttributesDialog attributesDialog;
CStretchSkewDialog stretchSkewDialog;
CFontsDialog fontsDialog;

/* FUNCTIONS ********************************************************/

LRESULT CMirrorRotateDialog::OnInitDialog(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CheckDlgButton(IDD_MIRRORROTATERB1, BST_CHECKED);
    CheckDlgButton(IDD_MIRRORROTATERB4, BST_CHECKED);
    return 0;
}

LRESULT CMirrorRotateDialog::OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    EndDialog(0);
    return 0;
}

LRESULT CMirrorRotateDialog::OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (IsDlgButtonChecked(IDD_MIRRORROTATERB1))
        EndDialog(1);
    else if (IsDlgButtonChecked(IDD_MIRRORROTATERB2))
        EndDialog(2);
    else if (IsDlgButtonChecked(IDD_MIRRORROTATERB4))
        EndDialog(3);
    else if (IsDlgButtonChecked(IDD_MIRRORROTATERB5))
        EndDialog(4);
    else if (IsDlgButtonChecked(IDD_MIRRORROTATERB6))
        EndDialog(5);
    return 0;
}

LRESULT CMirrorRotateDialog::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(0);
    return 0;
}

LRESULT CMirrorRotateDialog::OnRadioButton3(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    ::EnableWindow(GetDlgItem(IDD_MIRRORROTATERB4), TRUE);
    ::EnableWindow(GetDlgItem(IDD_MIRRORROTATERB5), TRUE);
    ::EnableWindow(GetDlgItem(IDD_MIRRORROTATERB6), TRUE);
    return 0;
}

LRESULT CMirrorRotateDialog::OnRadioButton12(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    ::EnableWindow(GetDlgItem(IDD_MIRRORROTATERB4), FALSE);
    ::EnableWindow(GetDlgItem(IDD_MIRRORROTATERB5), FALSE);
    ::EnableWindow(GetDlgItem(IDD_MIRRORROTATERB6), FALSE);
    return 0;
}



LRESULT CAttributesDialog::OnInitDialog(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    newWidth = imageModel.GetWidth();
    newHeight = imageModel.GetHeight();

    CheckDlgButton(IDD_ATTRIBUTESRB3, BST_CHECKED);
    CheckDlgButton(IDD_ATTRIBUTESRB5, BST_CHECKED);
    SetDlgItemInt(IDD_ATTRIBUTESEDIT1, newWidth, FALSE);
    SetDlgItemInt(IDD_ATTRIBUTESEDIT2, newHeight, FALSE);

    if (isAFile)
    {
        TCHAR date[100];
        TCHAR temp[100];
        GetDateFormat(LOCALE_USER_DEFAULT, 0, &fileTime, NULL, date, _countof(date));
        GetTimeFormat(LOCALE_USER_DEFAULT, 0, &fileTime, NULL, temp, _countof(temp));
        _tcscat(date, _T(" "));
        _tcscat(date, temp);
        CString strSize;
        strSize.Format(IDS_FILESIZE, fileSize);
        SetDlgItemText(IDD_ATTRIBUTESTEXT6, date);
        SetDlgItemText(IDD_ATTRIBUTESTEXT7, strSize);
    }
    CString strRes;
    strRes.Format(IDS_PRINTRES, fileHPPM, fileVPPM);
    SetDlgItemText(IDD_ATTRIBUTESTEXT8, strRes);
    return 0;
}

LRESULT CAttributesDialog::OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    EndDialog(0);
    return 0;
}

LRESULT CAttributesDialog::OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(1);
    return 0;
}

LRESULT CAttributesDialog::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(0);
    return 0;
}

LRESULT CAttributesDialog::OnDefault(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    newWidth = imageModel.GetWidth();
    newHeight = imageModel.GetHeight();
    CheckDlgButton(IDD_ATTRIBUTESRB3, BST_CHECKED);
    CheckDlgButton(IDD_ATTRIBUTESRB5, BST_CHECKED);
    SetDlgItemInt(IDD_ATTRIBUTESEDIT1, newWidth, FALSE);
    SetDlgItemInt(IDD_ATTRIBUTESEDIT2, newHeight, FALSE);
    return 0;
}

LRESULT CAttributesDialog::OnRadioButton1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    CString strNum;
    strNum.Format(_T("%.3lf"), newWidth / (0.0254 * fileHPPM));
    SetDlgItemText(IDD_ATTRIBUTESEDIT1, strNum);
    strNum.Format(_T("%.3lf"), newHeight / (0.0254 * fileVPPM));
    SetDlgItemText(IDD_ATTRIBUTESEDIT2, strNum);
    return 0;
}

LRESULT CAttributesDialog::OnRadioButton2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    CString strNum;
    strNum.Format(_T("%.3lf"), newWidth * 100.0 / fileHPPM);
    SetDlgItemText(IDD_ATTRIBUTESEDIT1, strNum);
    strNum.Format(_T("%.3lf"), newHeight * 100.0 / fileVPPM);
    SetDlgItemText(IDD_ATTRIBUTESEDIT2, strNum);
    return 0;
}

LRESULT CAttributesDialog::OnRadioButton3(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    SetDlgItemInt(IDD_ATTRIBUTESEDIT1, newWidth, FALSE);
    SetDlgItemInt(IDD_ATTRIBUTESEDIT2, newHeight, FALSE);
    return 0;
}

LRESULT CAttributesDialog::OnEdit1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (Edit_GetModify(hWndCtl))
    {
        TCHAR tempS[100];
        if (IsDlgButtonChecked(IDD_ATTRIBUTESRB1))
        {
            GetDlgItemText(IDD_ATTRIBUTESEDIT1, tempS, _countof(tempS));
            newWidth = max(1, (int) (_tcstod(tempS, NULL) * fileHPPM * 0.0254));
        }
        else if (IsDlgButtonChecked(IDD_ATTRIBUTESRB2))
        {
            GetDlgItemText(IDD_ATTRIBUTESEDIT1, tempS, _countof(tempS));
            newWidth = max(1, (int) (_tcstod(tempS, NULL) * fileHPPM / 100));
        }
        else if (IsDlgButtonChecked(IDD_ATTRIBUTESRB3))
        {
            GetDlgItemText(IDD_ATTRIBUTESEDIT1, tempS, _countof(tempS));
            newWidth = max(1, _tstoi(tempS));
        }
        Edit_SetModify(hWndCtl, FALSE);
    }
    return 0;
}

LRESULT CAttributesDialog::OnEdit2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (Edit_GetModify(hWndCtl))
    {
        TCHAR tempS[100];
        if (IsDlgButtonChecked(IDD_ATTRIBUTESRB1))
        {
            GetDlgItemText(IDD_ATTRIBUTESEDIT2, tempS, _countof(tempS));
            newHeight = max(1, (int) (_tcstod(tempS, NULL) * fileVPPM * 0.0254));
        }
        else if (IsDlgButtonChecked(IDD_ATTRIBUTESRB2))
        {
            GetDlgItemText(IDD_ATTRIBUTESEDIT2, tempS, _countof(tempS));
            newHeight = max(1, (int) (_tcstod(tempS, NULL) * fileVPPM / 100));
        }
        else if (IsDlgButtonChecked(IDD_ATTRIBUTESRB3))
        {
            GetDlgItemText(IDD_ATTRIBUTESEDIT2, tempS, _countof(tempS));
            newHeight = max(1, _tstoi(tempS));
        }
        Edit_SetModify(hWndCtl, FALSE);
    }
    return 0;
}



LRESULT CStretchSkewDialog::OnInitDialog(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SetDlgItemInt(IDD_STRETCHSKEWEDITHSTRETCH, 100, FALSE);
    SetDlgItemInt(IDD_STRETCHSKEWEDITVSTRETCH, 100, FALSE);
    SetDlgItemInt(IDD_STRETCHSKEWEDITHSKEW, 0, FALSE);
    SetDlgItemInt(IDD_STRETCHSKEWEDITVSKEW, 0, FALSE);
    return 0;
}

LRESULT CStretchSkewDialog::OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    EndDialog(0);
    return 0;
}

LRESULT CStretchSkewDialog::OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    CString strrcIntNumbers;
    CString strrcPercentage;
    CString strrcAngle;
    BOOL tr1, tr2, tr3, tr4;

    strrcIntNumbers.LoadString(hProgInstance, IDS_INTNUMBERS);
    strrcPercentage.LoadString(hProgInstance, IDS_PERCENTAGE);
    strrcAngle.LoadString(hProgInstance, IDS_ANGLE);

    percentage.x = GetDlgItemInt(IDD_STRETCHSKEWEDITHSTRETCH, &tr1, FALSE);
    percentage.y = GetDlgItemInt(IDD_STRETCHSKEWEDITVSTRETCH, &tr2, FALSE);
    angle.x = GetDlgItemInt(IDD_STRETCHSKEWEDITHSKEW, &tr3, TRUE);
    angle.y = GetDlgItemInt(IDD_STRETCHSKEWEDITVSKEW, &tr4, TRUE);

    if (!(tr1 && tr2 && tr3 && tr4))
        MessageBox(strrcIntNumbers, NULL, MB_ICONEXCLAMATION);
    else if (percentage.x < 1 || percentage.x > 500 || percentage.y < 1 || percentage.y > 500)
        MessageBox(strrcPercentage, NULL, MB_ICONEXCLAMATION);
    else if (angle.x < -89 || angle.x > 89 || angle.y < -89 || angle.y > 89)
        MessageBox(strrcAngle, NULL, MB_ICONEXCLAMATION);
    else
        EndDialog(1);
    return 0;
}

LRESULT CStretchSkewDialog::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(0);
    return 0;
}

static INT CALLBACK
EnumFontFamProc(ENUMLOGFONT *lpelf, NEWTEXTMETRIC *lpntm, INT FontType, LPARAM lParam)
{
    CSimpleArray<CString>& arrFontNames = *reinterpret_cast<CSimpleArray<CString>*>(lParam);
    LPTSTR name = lpelf->elfLogFont.lfFaceName;
    if (name[0] == TEXT('@')) // Vertical fonts
        return TRUE;

    for (INT i = 0; i < arrFontNames.GetSize(); ++i)
    {
        if (arrFontNames[i] == name) // Already exists
            return TRUE;
    }

    arrFontNames.Add(name);
    return TRUE;
}

// TODO: AutoComplete font names
// TODO: Vertical text
CFontsDialog::CFontsDialog()
{
}

void CFontsDialog::InitFontNames()
{
    // List the fonts
    CSimpleArray<CString> arrFontNames;
    HDC hDC = CreateCompatibleDC(NULL);
    if (hDC)
    {
        EnumFontFamilies(hDC, NULL, (FONTENUMPROC)EnumFontFamProc,
                         reinterpret_cast<LPARAM>(&arrFontNames));
        DeleteDC(hDC);
    }

    // Actually add them to the combobox
    HWND hwndNames = GetDlgItem(IDD_FONTSNAMES);
    SendMessage(hwndNames, CB_RESETCONTENT, 0, 0);
    for (INT i = 0; i < arrFontNames.GetSize(); ++i)
    {
        ComboBox_AddString(hwndNames, arrFontNames[i]);
    }

    ::SetWindowText(hwndNames, registrySettings.strFontName);
}

void CFontsDialog::InitFontSizes()
{
    static const INT s_sizes[] =
    {
        8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72
    };

    HWND hwndSizes = GetDlgItem(IDD_FONTSSIZES);
    ComboBox_ResetContent(hwndSizes);

    TCHAR szText[16];
    for (UINT i = 0; i < _countof(s_sizes); ++i)
    {
        wsprintf(szText, TEXT("%d"), s_sizes[i]);
        INT iItem = ComboBox_AddString(hwndSizes, szText);
        if (s_sizes[i] == (INT)registrySettings.PointSize)
            ComboBox_SetCurSel(hwndSizes, iItem);
    }

    if (ComboBox_GetCurSel(hwndSizes) == CB_ERR)
    {
        wsprintf(szText, TEXT("%d"), (INT)registrySettings.PointSize);
        ::SetWindowText(hwndSizes, szText);
    }
}

void CFontsDialog::InitToolbar()
{
    HWND hwndToolbar = GetDlgItem(IDD_FONTSTOOLBAR);
    SendMessage(hwndToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessage(hwndToolbar, TB_SETBITMAPSIZE, 0, MAKELPARAM(16, 16));
    SendMessage(hwndToolbar, TB_SETBUTTONWIDTH, 0, MAKELPARAM(20, 20));
    
    TBADDBITMAP AddBitmap;
    AddBitmap.hInst = hProgInstance;
    AddBitmap.nID = IDB_FONTSTOOLBAR;
    SendMessage(hwndToolbar, TB_ADDBITMAP, 4, (LPARAM)&AddBitmap);

    HIMAGELIST himl = ImageList_LoadImage(hProgInstance, MAKEINTRESOURCE(IDB_FONTSTOOLBAR),
                                          16, 8, RGB(255, 0, 255), IMAGE_BITMAP,
                                          LR_CREATEDIBSECTION);
    SendMessage(hwndToolbar, TB_SETIMAGELIST, 0, (LPARAM)himl);

    TBBUTTON buttons[] =
    {
        { 0, IDM_BOLD, TBSTATE_ENABLED, TBSTYLE_CHECK },
        { 1, IDM_ITALIC, TBSTATE_ENABLED, TBSTYLE_CHECK },
        { 2, IDM_UNDERLINE, TBSTATE_ENABLED, TBSTYLE_CHECK },
        { 3, IDM_VERTICAL, 0, TBSTYLE_CHECK }, // TODO:
    };
    SendMessage(hwndToolbar, TB_ADDBUTTONS, _countof(buttons), (LPARAM)&buttons);

    SendMessage(hwndToolbar, TB_CHECKBUTTON, IDM_BOLD, registrySettings.Bold);
    SendMessage(hwndToolbar, TB_CHECKBUTTON, IDM_ITALIC, registrySettings.Italic);
    SendMessage(hwndToolbar, TB_CHECKBUTTON, IDM_UNDERLINE, registrySettings.Underline);
}

LRESULT CFontsDialog::OnInitDialog(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // TODO: Tooltips
    InitFontNames();
    InitFontSizes();
    InitToolbar();

    if (registrySettings.FontsPositionX != 0 || registrySettings.FontsPositionY != 0)
    {
        SetWindowPos(NULL,
                     registrySettings.FontsPositionX, registrySettings.FontsPositionY,
                     0, 0,
                     SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
        SendMessage(DM_REPOSITION, 0, 0);
    }

    if (!registrySettings.ShowTextTool)
        ShowWindow(SW_HIDE);

    return TRUE;
}

LRESULT CFontsDialog::OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ShowWindow(SW_HIDE); // Just hide. Recycle for optimization
    return 0;
}

void CFontsDialog::OnFontName(UINT codeNotify)
{
    HWND hwndNames = GetDlgItem(IDD_FONTSNAMES);
    INT iItem = CB_ERR;
    UINT cch;
    TCHAR szText[LF_FACESIZE];

    switch (codeNotify)
    {
        case CBN_SELCHANGE:
            iItem = ComboBox_GetCurSel(hwndNames);
            cch = ComboBox_GetLBTextLen(hwndNames, iItem);
            if (iItem != CB_ERR && 0 < cch && cch < _countof(szText))
            {
                ComboBox_GetLBText(hwndNames, iItem, szText);
            }
            break;

        case CBN_EDITCHANGE:
            GetDlgItemText(IDD_FONTSNAMES, szText, _countof(szText));
            iItem = ComboBox_FindStringExact(hwndNames, -1, szText);
            break;
    }

    if (iItem != CB_ERR && registrySettings.strFontName.CompareNoCase(szText) != 0)
    {
        registrySettings.strFontName = szText;
        toolsModel.NotifyToolChanged();
    }
}

void CFontsDialog::OnFontSize(UINT codeNotify)
{
    HWND hwndSizes = GetDlgItem(IDD_FONTSSIZES);
    WCHAR szText[8];
    INT iItem, PointSize = 0;
    UINT cch;

    switch (codeNotify)
    {
        case CBN_SELCHANGE:
            iItem = ComboBox_GetCurSel(hwndSizes);
            cch = ComboBox_GetLBTextLen(hwndSizes, iItem);
            if (iItem != CB_ERR && 0 < cch && cch < _countof(szText))
            {
                ComboBox_GetLBText(hwndSizes, iItem, szText);
                PointSize = _ttoi(szText);
            }
            break;

        case CBN_EDITCHANGE:
            ::GetWindowText(hwndSizes, szText, _countof(szText));
            PointSize = _ttoi(szText);
            break;
    }

    if (PointSize > 0)
    {
        registrySettings.PointSize = PointSize;
        toolsModel.NotifyToolChanged();
    }
}

LRESULT CFontsDialog::OnCommand(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UINT id = LOWORD(wParam);
    UINT codeNotify = HIWORD(wParam);
    HWND hwndToolbar = GetDlgItem(IDD_FONTSTOOLBAR);
    BOOL bChecked = ::SendMessage(hwndToolbar, TB_ISBUTTONCHECKED, id, 0);

    switch (id)
    {
        case IDCANCEL:
            ShowWindow(SW_HIDE);
            registrySettings.ShowTextTool = FALSE;
            break;

        case IDD_FONTSNAMES:
            OnFontName(codeNotify);
            break;

        case IDD_FONTSSIZES:
            OnFontSize(codeNotify);
            break;

        case IDM_BOLD:
            registrySettings.Bold = bChecked;
            toolsModel.NotifyToolChanged();
            break;

        case IDM_ITALIC:
            registrySettings.Italic = bChecked;
            toolsModel.NotifyToolChanged();
            break;

        case IDM_UNDERLINE:
            registrySettings.Underline = bChecked;
            toolsModel.NotifyToolChanged();
            break;

        case IDM_VERTICAL:
            // TODO:
            break;
    }
    return 0;
}

LRESULT CFontsDialog::OnNotify(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    NMHDR *pnmhdr = reinterpret_cast<NMHDR *>(lParam);
    if (pnmhdr->code == TTN_NEEDTEXT)
    {
        LPTOOLTIPTEXT pToolTip = reinterpret_cast<LPTOOLTIPTEXT>(lParam);
        pToolTip->hinst = hProgInstance;
        switch (pnmhdr->idFrom)
        {
            case IDM_BOLD:      pToolTip->lpszText = MAKEINTRESOURCE(IDS_BOLD); break;
            case IDM_ITALIC:    pToolTip->lpszText = MAKEINTRESOURCE(IDS_ITALIC); break;
            case IDM_UNDERLINE: pToolTip->lpszText = MAKEINTRESOURCE(IDS_UNDERLINE); break;
            case IDM_VERTICAL:  pToolTip->lpszText = MAKEINTRESOURCE(IDS_VERTICAL); break;

            default:
                break;
        }
    }
    return 0;
}

LRESULT CFontsDialog::OnMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT rc;
    GetWindowRect(&rc);
    registrySettings.FontsPositionX = rc.left;
    registrySettings.FontsPositionY = rc.top;
    return 0;
}

LRESULT CFontsDialog::OnToolsModelToolChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam != TOOL_TEXT)
        ShowWindow(SW_HIDE);

    return 0;
}

LRESULT CFontsDialog::OnMeasureItem(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam == IDD_FONTSNAMES)
    {
        LPMEASUREITEMSTRUCT pMeasureItem = reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);
        RECT rc;
        ::GetClientRect(GetDlgItem(IDD_FONTSNAMES), &rc);
        pMeasureItem->itemWidth = rc.right - rc.left;
        pMeasureItem->itemHeight = GetSystemMetrics(SM_CYVSCROLL);
        return TRUE;
    }
    return 0;
}

LRESULT CFontsDialog::OnDrawItem(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // TODO: Owner-draw the font types
    if (wParam == IDD_FONTSNAMES)
    {
        LPDRAWITEMSTRUCT pDrawItem = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
        if (pDrawItem->itemID == (UINT)-1)
            return TRUE;

        SetBkMode(pDrawItem->hDC, TRANSPARENT);

        HWND hwndItem = pDrawItem->hwndItem;
        RECT rcItem = pDrawItem->rcItem;
        if (pDrawItem->itemState & ODS_SELECTED)
        {
            FillRect(pDrawItem->hDC, &rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
            SetTextColor(pDrawItem->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
        }
        else
        {
            FillRect(pDrawItem->hDC, &rcItem, GetSysColorBrush(COLOR_WINDOW));
            SetTextColor(pDrawItem->hDC, GetSysColor(COLOR_WINDOWTEXT));
        }

        TCHAR szText[LF_FACESIZE];
        if ((UINT)ComboBox_GetLBTextLen(hwndItem, pDrawItem->itemID) < _countof(szText))
        {
            szText[0] = 0;
            ComboBox_GetLBText(hwndItem, pDrawItem->itemID, szText);

            rcItem.left += 24;
            DrawText(pDrawItem->hDC, szText, -1, &rcItem, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }

        if (pDrawItem->itemState & ODS_FOCUS)
            ::DrawFocusRect(pDrawItem->hDC, &pDrawItem->rcItem);

        return TRUE;
    }
    return 0;
}
