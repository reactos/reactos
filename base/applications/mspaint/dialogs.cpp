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

static
INT CALLBACK
EnumFontFamProc(ENUMLOGFONT *lpelf,
                NEWTEXTMETRIC *lpntm,
                INT FontType,
                LPARAM lParam)
{
    CFontsDialog *pThis = reinterpret_cast<CFontsDialog*>(lParam);
    LPTSTR name = lpelf->elfLogFont.lfFaceName;
    if (name[0] == TEXT('@'))
        return TRUE;

    for (INT i = 0; i < pThis->m_arrFontNames.GetSize(); ++i)
    {
        if (pThis->m_arrFontNames[i] == name)
            return TRUE;
    }

    pThis->m_arrFontNames.Add(name);
    return TRUE;
}

CFontsDialog::CFontsDialog()
{
    m_bBold = m_bItalic = m_bUnderline = FALSE;
    m_nFontSize = 14;
}

static int CompareFontNames(const void *x, const void *y)
{
    const CString *a = reinterpret_cast<const CString *>(x);
    const CString *b = reinterpret_cast<const CString *>(y);
    return lstrcmpi(*a, *b);
}

void CFontsDialog::InitNames()
{
    // get default gui font
    LOGFONT lf;
    GetObject(GetStockFont(DEFAULT_GUI_FONT), sizeof(lf), &lf);
    if (!m_strFontName.GetLength())
        m_strFontName = lf.lfFaceName;

    HDC hDC = CreateCompatibleDC(NULL);
    if (!hDC)
        return;

    m_arrFontNames.RemoveAll();
    EnumFontFamilies(hDC, NULL, (FONTENUMPROC)EnumFontFamProc, reinterpret_cast<LPARAM>(this));
    DeleteDC(hDC);

    qsort(m_arrFontNames.GetData(), m_arrFontNames.GetSize(), sizeof(CString), CompareFontNames);

    HWND hwndNames = GetDlgItem(IDD_FONTSNAMES);
    SendMessage(hwndNames, CB_RESETCONTENT, 0, 0);
    for (INT i = 0; i < m_arrFontNames.GetSize(); ++i)
    {
        CString& name = m_arrFontNames[i];
        COMBOBOXEXITEM item = { CBEIF_TEXT, -1 };
        item.pszText = const_cast<LPWSTR>(&name[0]);
        if (ComboBox_FindStringExact(hwndNames, -1, item.pszText) == CB_ERR)
        {
            SendMessage(hwndNames, CBEM_INSERTITEM, 0, (LPARAM)&item);
        }
    }

    // set the default font name
    INT iFont = ComboBox_FindStringExact(hwndNames, -1, m_strFontName);
    if (iFont != CB_ERR)
        ComboBox_SetCurSel(hwndNames, iFont);
    ::SetWindowText(hwndNames, m_strFontName);
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
    INT iItem;
    for (UINT i = 0; i < _countof(s_sizes); ++i)
    {
        wsprintf(szText, TEXT("%d"), s_sizes[i]);
        iItem = ComboBox_AddString(hwndSizes, szText);
        if (s_sizes[i] == m_nFontSize)
            ComboBox_SetCurSel(hwndSizes, iItem);
    }
}

void CFontsDialog::InitToolbar()
{
    HWND hwndToolbar = GetDlgItem(IDD_FONTSTOOLBAR);
    SendMessage(hwndToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessage(hwndToolbar, TB_SETBITMAPSIZE, 0, MAKELPARAM(16, 16));
    SendMessage(hwndToolbar, TB_SETBUTTONWIDTH, 0, MAKELPARAM(22, 22));
    
    TBADDBITMAP AddBitmap;
    AddBitmap.hInst = hProgInstance;
    AddBitmap.nID = IDB_FONTSTOOLBAR;
    SendMessage(hwndToolbar, TB_ADDBITMAP, 4, (LPARAM)&AddBitmap);

    HIMAGELIST himl = ImageList_LoadBitmap(hProgInstance, MAKEINTRESOURCE(IDB_FONTSTOOLBAR), 16, 8, RGB(255, 255, 255));
    SendMessage(hwndToolbar, TB_SETIMAGELIST, 0, (LPARAM)himl);

    // TODO: Tooltips
    TBBUTTON buttons[] =
    {
        { 0, IDM_BOLD, TBSTATE_ENABLED, TBSTYLE_CHECK },
        { 1, IDM_ITALIC, TBSTATE_ENABLED, TBSTYLE_CHECK },
        { 2, IDM_UNDERLINE, TBSTATE_ENABLED, TBSTYLE_CHECK },
        { 3, IDM_VERTICAL, 0, TBSTYLE_CHECK }, // TODO:
    };
    SendMessage(hwndToolbar, TB_ADDBUTTONS, _countof(buttons), (LPARAM)&buttons);

    SendMessage(hwndToolbar, TB_CHECKBUTTON, IDM_BOLD, m_bBold);
    SendMessage(hwndToolbar, TB_CHECKBUTTON, IDM_ITALIC, m_bItalic);
    SendMessage(hwndToolbar, TB_CHECKBUTTON, IDM_UNDERLINE, m_bUnderline);
}

LRESULT CFontsDialog::OnInitDialog(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    InitNames();
    InitFontSizes();
    InitToolbar();
    return TRUE;
}

LRESULT CFontsDialog::OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ShowWindow(SW_HIDE);
    return 0;
}

void CFontsDialog::OnFontName(UINT codeNotify)
{
    HWND hwndNames = GetDlgItem(IDD_FONTSNAMES);
    WCHAR szText[LF_FACESIZE];
    INT iItem;
    switch (codeNotify)
    {
        case CBN_SELCHANGE:
            iItem = ComboBox_GetCurSel(hwndNames);
            if (iItem != CB_ERR)
            {
                COMBOBOXEXITEM item = { CBEIF_TEXT, iItem, szText, _countof(szText) };
                SendMessage(hwndNames, CBEM_GETITEM, 0, (LPARAM)&item);
                if (m_strFontName != szText)
                {
                    m_strFontName = szText;
                    toolsModel.NotifyToolChanged();
                }
            }
            break;

        case CBN_EDITCHANGE:
            GetDlgItemText(IDD_FONTSNAMES, szText, _countof(szText));
            for (INT i = 0; i < m_arrFontNames.GetSize(); ++i)
            {
                CString& name = m_arrFontNames[i];
                if (name == szText && m_strFontName != szText)
                {
                    m_strFontName = szText;
                    toolsModel.NotifyToolChanged();
                }
            }
            break;
    }
}

void CFontsDialog::OnFontSize(UINT codeNotify)
{
    HWND hwndSizes = GetDlgItem(IDD_FONTSSIZES);
    WCHAR szText[LF_FACESIZE];
    INT iItem;
    switch (codeNotify)
    {
        case CBN_SELCHANGE:
            iItem = ComboBox_GetCurSel(hwndSizes);
            if (iItem != CB_ERR)
            {
                ComboBox_GetLBText(hwndSizes, iItem, szText);
                m_nFontSize = _ttoi(szText);
                toolsModel.NotifyToolChanged();
            }
            break;

        case CBN_EDITCHANGE:
            GetDlgItemText(IDD_FONTSSIZES, szText, _countof(szText));
            m_nFontSize = _ttoi(szText);
            toolsModel.NotifyToolChanged();
            break;
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
            DestroyWindow();
            break;

        case IDD_FONTSNAMES:
            OnFontName(codeNotify);
            break;

        case IDD_FONTSSIZES:
            OnFontSize(codeNotify);
            break;

        case IDM_BOLD:
            m_bBold = bChecked;
            toolsModel.NotifyToolChanged();
            break;

        case IDM_ITALIC:
            m_bItalic = bChecked;
            toolsModel.NotifyToolChanged();
            break;

        case IDM_UNDERLINE:
            m_bUnderline = bChecked;
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
    NMHDR *pnmhdr = (NMHDR *)lParam;
    if (pnmhdr->code == TTN_NEEDTEXT)
    {
        LPTOOLTIPTEXT pToolTip = (LPTOOLTIPTEXT)pnmhdr;
        pToolTip->hinst = hProgInstance;
        switch (pnmhdr->idFrom)
        {
            case IDM_BOLD:
                pToolTip->lpszText = MAKEINTRESOURCE(IDS_BOLD);
                break;
            case IDM_ITALIC:
                pToolTip->lpszText = MAKEINTRESOURCE(IDS_ITALIC);
                break;
            case IDM_UNDERLINE:
                pToolTip->lpszText = MAKEINTRESOURCE(IDS_UNDERLINE);
                break;
            case IDM_VERTICAL:
                pToolTip->lpszText = MAKEINTRESOURCE(IDS_VERTICAL);
                break;
            default:
                break;
        }
    }
    return 0;
}
