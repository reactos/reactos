// formatba.cpp : implementation file
//
// Copyright (C) 1992-1999 Microsoft Corporation
// All rights reserved.

#include "stdafx.h"

#include "wordpad.h"
#include "wordpdoc.h"
#include "wordpvw.h"
#include "formatba.h"
#include "strings.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

// reserve lobyte for charset
#define PRINTER_FONT 0x0100
#define TT_FONT 0x0200
#define DEVICE_FONT 0x0400
#define PS_OPENTYPE_FONT  0x800
#define TT_OPENTYPE_FONT  0x1000
#define TYPE1_FONT  0x2000

#define BMW 16
#define BMH 15

static int nFontSizes[] =
    {8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72};
int CLocalComboBox::m_nFontHeight = 0;

class CFontDesc
{
public:
    CFontDesc(LPCTSTR lpszName, LPCTSTR lpszScript, BYTE nCharSet,
        BYTE nPitchAndFamily, DWORD dwFlags);
    CString m_strName;
    CString m_strScript;
    BYTE m_nCharSet;
    BYTE m_nPitchAndFamily;
    DWORD m_dwFlags;
};

CFontDesc::CFontDesc(LPCTSTR lpszName, LPCTSTR lpszScript, BYTE nCharSet,
    BYTE nPitchAndFamily, DWORD dwFlags)
{
    m_strName = lpszName;
    m_strScript = lpszScript;
    m_nCharSet = nCharSet;
    m_nPitchAndFamily = nPitchAndFamily;
    m_dwFlags = dwFlags;
}

BEGIN_MESSAGE_MAP(CFormatBar, CToolBar)
    //{{AFX_MSG_MAP(CFormatBar)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
    ON_CBN_DROPDOWN(IDC_FONTSIZE, OnFontSizeDropDown)
    ON_CBN_DROPDOWN(IDC_SCRIPT, OnScriptDropDown)
    ON_CBN_KILLFOCUS(IDC_FONTNAME, OnFontNameKillFocus)
    ON_CBN_KILLFOCUS(IDC_FONTSIZE, OnFontSizeKillFocus)
    ON_CBN_KILLFOCUS(IDC_SCRIPT, OnScriptKillFocus)
    ON_CBN_SETFOCUS(IDC_FONTNAME, OnComboSetFocus)
    ON_CBN_SETFOCUS(IDC_FONTSIZE, OnComboSetFocus)
    ON_CBN_SETFOCUS(IDC_SCRIPT, OnComboSetFocus)
    ON_CBN_CLOSEUP(IDC_FONTNAME, OnComboCloseUp)
    ON_CBN_CLOSEUP(IDC_FONTSIZE, OnComboCloseUp)
    ON_CBN_CLOSEUP(IDC_SCRIPT, OnComboCloseUp)
    ON_REGISTERED_MESSAGE(CWordPadApp::m_nPrinterChangedMsg, OnPrinterChanged)
    // Global help commands
END_MESSAGE_MAP()

static CSize GetBaseUnits(CFont* pFont)
{
    ASSERT(pFont != NULL);
    ASSERT(pFont->GetSafeHandle() != NULL);
    pFont = theApp.m_dcScreen.SelectObject(pFont);
    TEXTMETRIC tm;
    VERIFY(theApp.m_dcScreen.GetTextMetrics(&tm));

    theApp.m_dcScreen.SelectObject(pFont);
    return CSize(tm.tmAveCharWidth, tm.tmHeight);
}

CFormatBar::CFormatBar()
{
    CFont fnt;
    fnt.Attach(GetStockObject(theApp.m_nDefFont));
    m_szBaseUnits = GetBaseUnits(&fnt);
    CLocalComboBox::m_nFontHeight = m_szBaseUnits.cy;
}

void CFormatBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
    CToolBar::OnUpdateCmdUI(pTarget, bDisableIfNoHndler);
    // don't update combo boxes if any of them have the focus
    if (!m_comboFontName.HasFocus()
        && !m_comboFontSize.HasFocus()
        && !m_comboScript.HasFocus())
    {
        SyncToView();
    }
}

void CFormatBar::SyncToView()
{
    USES_CONVERSION;
    // get the current font from the view and update
    CHARHDR fh;
    CHARFORMAT& cf = fh.cf;
    fh.hwndFrom = m_hWnd;
    fh.idFrom = GetDlgCtrlID();
    fh.code = FN_GETFORMAT;
    VERIFY(GetOwner()->SendMessage(WM_NOTIFY, fh.idFrom, (LPARAM)&fh));

    if (cf.dwMask & CFM_FACE)
        m_comboFontName.SetTheText(cf.szFaceName);
    else
        m_comboFontName.SetTheText(TEXT(""));

    CString charsetname = TEXT("");
    if (cf.dwMask & CFM_CHARSET)
    {
        for (int i = 0; i < m_comboFontName.m_arrayFontDesc.GetSize(); i++)
        {
            CFontDesc *pDesc = (CFontDesc *) m_comboFontName.m_arrayFontDesc[i];

            if (pDesc->m_strName == cf.szFaceName
                && pDesc->m_nCharSet == cf.bCharSet)
            {
                charsetname = pDesc->m_strScript;
                break;
            }
        }
    }
    m_comboScript.SetTheText(charsetname);

    static bool init_script_history = true;

    if (init_script_history && TEXT('\0') != charsetname[0])
    {
        OnScriptDropDown();
        m_comboScript.PickScript();
        init_script_history = false;
    }

    // SetTwipSize only updates if different
    // -1 means selection is not a single point size
    m_comboFontSize.SetTwipSize( (cf.dwMask & CFM_SIZE) ? cf.yHeight : -1);
}

BYTE CScriptComboBox::PickScript()
{
    CString scriptname;
    int     scriptindex;

    GetTheText(scriptname);
    scriptindex = FindStringExact(-1, scriptname);

    if (CB_ERR == scriptindex)
    {
        int  i = m_history_index;

        do
        {
            scriptindex = FindStringExact(-1, m_script_history[i]);

            if (CB_ERR != scriptindex)
                break;

            i = (i + HistorySize - 1) % HistorySize;
        }
        while (i != m_history_index);

        if (CB_ERR == scriptindex)
        {
            if (1 != GetCount())
            {
                SetCurSel(-1);
                return DEFAULT_CHARSET;
            }

            scriptindex = 0;
        }

        SetCurSel(scriptindex);
        GetTheText(scriptname);
    }

    if (m_script_history[m_history_index] != scriptname)
    {
        m_history_index = (m_history_index + 1) % HistorySize;
        m_script_history[m_history_index] = scriptname;
    }

    return (BYTE) GetItemData(scriptindex);
}

void CFormatBar::OnFontSizeDropDown()
{
    CString str;
    m_comboFontName.GetTheText(str);
    LPCTSTR lpszName = NULL;
    BOOL bPrinterFont= FALSE;
    int nIndex = m_comboFontName.FindStringExact(-1, str);
    if (nIndex != CB_ERR)
    {
        CFontDesc* pDesc = (CFontDesc*)m_comboFontName.GetItemData(nIndex);
        ASSERT(pDesc != NULL);
        bPrinterFont = pDesc->m_dwFlags & PRINTER_FONT;
        lpszName = pDesc->m_strName;
    }

    int nSize = m_comboFontSize.GetTwipSize();
    if (nSize == -2) // error
    {
        AfxMessageBox(IDS_INVALID_NUMBER, MB_OK|MB_ICONINFORMATION);
        nSize = m_comboFontSize.m_nTwipsLast;
    }
    else if ((nSize >= 0 && nSize < 20) || nSize > 32760)
    {
        AfxMessageBox(IDS_INVALID_FONTSIZE, MB_OK|MB_ICONINFORMATION);
        nSize = m_comboFontSize.m_nTwipsLast;
    }

    if (bPrinterFont)
        m_comboFontSize.EnumFontSizes(m_dcPrinter, lpszName);
    else
        m_comboFontSize.EnumFontSizes(theApp.m_dcScreen, lpszName);
    m_comboFontSize.SetTwipSize(nSize);
}

void CFormatBar::OnScriptDropDown()
{
    CString fontname;
    CString scriptname;
    m_comboFontName.GetTheText(fontname);
    m_comboScript.GetTheText(scriptname);

    m_comboScript.ResetContent();
    m_comboScript.SetTheText(scriptname);

    for (int i = 0; i < m_comboFontName.m_arrayFontDesc.GetSize(); i++)
    {
        CFontDesc* pDesc = (CFontDesc*)m_comboFontName.m_arrayFontDesc[i];

        if (pDesc->m_strName == fontname)
        {
            // HACKHACK: GDI enumerates symbol type fonts multiple times.
            //           remove the duplicated charsets (ntbug:198753)

            if (CB_ERR == m_comboScript.FindStringExact(-1, pDesc->m_strScript))
            {
                int nIndex = m_comboScript.AddString(pDesc->m_strScript);
                m_comboScript.SetItemData(nIndex, pDesc->m_nCharSet);
            }
        }
    }
}

void CFormatBar::OnComboCloseUp()
{
    NotifyOwner(NM_RETURN);
}

void CFormatBar::OnComboSetFocus()
{
    NotifyOwner(NM_SETFOCUS);
}

void CFormatBar::OnFontNameKillFocus()
{
    USES_CONVERSION;
    // get the current font from the view and update
    NotifyOwner(NM_KILLFOCUS);

    CCharFormat cf;
    cf.szFaceName[0] = NULL;

    // this will retrieve the font entered in the edit control
    // it tries to match the font to something already present in the combo box
    // this effectively ignores case of a font the user enters
    // if a user enters arial, this will cause it to become Arial
    CString str;
    m_comboFontName.GetTheText(str);    // returns "arial"
    m_comboFontName.SetTheText(str);    // selects "Arial"
    m_comboFontName.GetTheText(str);    // returns "Arial"

    // if font name box is not empty
    if (str[0] != NULL)
    {
        cf.dwMask = CFM_FACE;
        int nIndex = m_comboFontName.FindStringExact(-1, str);
        if (nIndex != CB_ERR)
        {
            CFontDesc* pDesc = (CFontDesc*)m_comboFontName.GetItemData(nIndex);
            ASSERT(pDesc != NULL);
            ASSERT(pDesc->m_strName.GetLength() < LF_FACESIZE);
            lstrcpyn(cf.szFaceName, pDesc->m_strName, LF_FACESIZE);
            cf.bPitchAndFamily = pDesc->m_nPitchAndFamily;
        }
        else // unknown font
        {
            ASSERT(str.GetLength() < LF_FACESIZE);
            lstrcpyn(cf.szFaceName, str, LF_FACESIZE);
            cf.bPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
        }

        OnScriptDropDown();
        cf.bCharSet = m_comboScript.PickScript();
        cf.dwMask |= CFM_CHARSET;
        SetCharFormat(cf);
    }
}

void CFormatBar::OnFontSizeKillFocus()
{
    NotifyOwner(NM_KILLFOCUS);
    int nSize = m_comboFontSize.GetTwipSize();
    if (nSize == -2)
    {
        AfxMessageBox(IDS_INVALID_NUMBER, MB_OK|MB_ICONINFORMATION);
        nSize = m_comboFontSize.m_nTwipsLast;
    }
    else if ((nSize >= 0 && nSize < 20) || nSize > 32760)
    {
        AfxMessageBox(IDS_INVALID_FONTSIZE, MB_OK|MB_ICONINFORMATION);
        nSize = m_comboFontSize.m_nTwipsLast;
    }
    else if (nSize > 0)
    {
        CCharFormat cf;
        cf.dwMask = CFM_SIZE;
        cf.yHeight = nSize;
        SetCharFormat(cf);
    }
}

void CFormatBar::OnScriptKillFocus()
{
    NotifyOwner(NM_KILLFOCUS);

    CString str;
    m_comboScript.GetTheText(str);
    m_comboScript.SetTheText(str);
    m_comboScript.GetTheText(str);

    if (str[0] != NULL)
    {
        CCharFormat cf;
        cf.dwMask = CFM_CHARSET;
        cf.bCharSet = m_comboScript.PickScript();
        SetCharFormat(cf);
    }
}

LONG CFormatBar::OnPrinterChanged(UINT, LONG)
{
    theApp.CreatePrinterDC(m_dcPrinter);
    m_comboFontName.EnumFontFamiliesEx(m_dcPrinter);
    return 0;
}

int CFormatBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CToolBar::OnCreate(lpCreateStruct) == -1)
        return -1;

    theApp.m_listPrinterNotify.AddTail(m_hWnd);

    CRect rect(0,0, LF_FACESIZE*m_szBaseUnits.cx, 200);
    if (!m_comboFontName.Create(WS_TABSTOP|WS_VISIBLE|WS_TABSTOP|
        WS_VSCROLL|CBS_DROPDOWN|CBS_SORT|CBS_AUTOHSCROLL|CBS_HASSTRINGS|
        CBS_OWNERDRAWFIXED, rect, this, IDC_FONTNAME))
    {
        TRACE0("Failed to create fontname combo-box\n");
        return -1;
    }
    m_comboFontName.LimitText(LF_FACESIZE);

    rect.SetRect(0, 0, 10*m_szBaseUnits.cx, 200);
    if (!m_comboFontSize.Create(WS_TABSTOP|WS_VISIBLE|WS_TABSTOP|
        WS_VSCROLL|CBS_DROPDOWN, rect, this, IDC_FONTSIZE))
    {
        TRACE0("Failed to create fontsize combo-box\n");
        return -1;
    }

    rect.SetRect(0, 0, 3*LF_FACESIZE*m_szBaseUnits.cx/4, 200);
    if (!m_comboScript.Create(WS_TABSTOP|WS_VISIBLE|WS_TABSTOP|
        WS_VSCROLL|CBS_DROPDOWN|CBS_SORT|CBS_AUTOHSCROLL|CBS_HASSTRINGS,
        rect, this, IDC_SCRIPT))
    {
        TRACE0("Failed to create script combo-box\n");
        return -1;
    }

    m_comboFontSize.LimitText(4);
    m_comboFontName.EnumFontFamiliesEx(m_dcPrinter);

    return 0;
}

void CFormatBar::OnDestroy()
{
    CToolBar::OnDestroy();
    POSITION pos = theApp.m_listPrinterNotify.Find(m_hWnd);
    ASSERT(pos != NULL);
    theApp.m_listPrinterNotify.RemoveAt(pos);
}

void CFormatBar::PositionCombos()
{
    CRect rect;
    // make font name box same size as font size box
    // this is necessary since font name box is owner draw
    m_comboFontName.SetItemHeight(-1, m_comboFontSize.GetItemHeight(-1));

    m_comboFontName.GetWindowRect(&rect);
    int nHeight = rect.Height();

    m_comboFontName.GetWindowRect(&rect);
    SetButtonInfo(0, IDC_FONTNAME, TBBS_SEPARATOR, rect.Width());
    GetItemRect(0, &rect); // FontName ComboBox
    m_comboFontName.SetWindowPos(NULL, rect.left,
        ((rect.Height() - nHeight) / 2) + rect.top, 0, 0,
        SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);

    m_comboFontSize.GetWindowRect(&rect);
    SetButtonInfo(2, IDC_FONTSIZE, TBBS_SEPARATOR, rect.Width());
    GetItemRect(2, &rect); // FontSize ComboBox
    m_comboFontSize.SetWindowPos(NULL, rect.left,
        ((rect.Height() - nHeight) / 2) + rect.top, 0, 0,
        SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);

    m_comboScript.GetWindowRect(&rect);
    SetButtonInfo(4, IDC_SCRIPT, TBBS_SEPARATOR, rect.Width());
    GetItemRect(4, &rect); // Script ComboBox
    m_comboScript.SetWindowPos(NULL, rect.left,
        ((rect.Height() - nHeight) / 2) + rect.top, 0, 0,
        SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);
}

/////////////////////////////////////////////////////////////////////////////
// CFontComboBox

BEGIN_MESSAGE_MAP(CFontComboBox, CLocalComboBox)
    //{{AFX_MSG_MAP(CFontComboBox)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
    // Global help commands
END_MESSAGE_MAP()

CFontComboBox::CFontComboBox()
{
    VERIFY(m_bmFontType.LoadBitmap(IDB_FONTTYPE));
}

void CFontComboBox::OnDestroy()
{
    // destroy all the CFontDesc's
    EmptyContents();
    CLocalComboBox::OnDestroy();
}

void CFontComboBox::EmptyContents()
{
    // destroy all the CFontDesc's
    int nCount = GetCount();
    for (int i=0;i<nCount;i++)
    {
        delete (CFontDesc*)GetItemData(i);
        SetItemData(i, NULL);
    }
}

void CFontComboBox::EnumFontFamiliesEx(CDC& dc, BYTE nCharSet)
{
    CString str;
    GetTheText(str);

    EmptyContents();
    ResetContent();
    m_arrayFontDesc.RemoveAll();

    LOGFONT lf;
    memset(&lf, 0, sizeof(LOGFONT));
    lf.lfCharSet = nCharSet;

    if (dc.m_hDC != NULL)
    {
        ::EnumFontFamiliesEx(dc.m_hDC, &lf,
                (FONTENUMPROC) EnumFamPrinterCallBackEx, (LPARAM) this, NULL);
    }
    else
    {
        HDC hDC = theApp.m_dcScreen.m_hDC;
        ASSERT(hDC != NULL);

        ::EnumFontFamiliesEx(hDC, &lf,
                (FONTENUMPROC) EnumFamScreenCallBackEx, (LPARAM) this, NULL);
    }

    // Add the fonts to the dropdown.  Don't add fonts that differ only by
    // charset.

    int nCount = (int)m_arrayFontDesc.GetSize();

    for (int i = 0; i < nCount; i++)
    {
        CFontDesc *pDesc = (CFontDesc*) m_arrayFontDesc[i];

        for (int j = i - 1; j >= 0; --j)
        {
            CFontDesc *otherfont = (CFontDesc*) m_arrayFontDesc[j];

            if (pDesc->m_strName == otherfont->m_strName
                && pDesc->m_dwFlags == otherfont->m_dwFlags)
            {
                break;
            }
        }

        if (j < 0)
        {
            int nIndex = AddString(pDesc->m_strName);
            ASSERT(nIndex >= 0);
            if (nIndex >= 0)
                SetItemData(nIndex, (DWORD_PTR)pDesc);
        }
    }

    SetTheText(str);
}

void CFontComboBox::AddFont(ENUMLOGFONT* pelf, DWORD dwType, LPCTSTR lpszScript)
{
    LOGFONT& lf = pelf->elfLogFont;

    if (lf.lfCharSet == MAC_CHARSET) // don't put in MAC fonts, commdlg doesn't either
        return;

    CFontDesc* pDesc = new CFontDesc(lf.lfFaceName, lpszScript,
        lf.lfCharSet, lf.lfPitchAndFamily, dwType);
    m_arrayFontDesc.Add(pDesc);
}

VOID   vGetFontType(NEWTEXTMETRICEX* lpntm, int FontType, DWORD* pdwData)
{
    DWORD  ntmFlags = lpntm->ntmTm.ntmFlags;

    if (ntmFlags & NTM_PS_OPENTYPE)
    {
        *pdwData |= PS_OPENTYPE_FONT;
    }
    else if (ntmFlags & NTM_TYPE1)
    {
        *pdwData |= TYPE1_FONT;
    }
    else
    {
        if (FontType & TRUETYPE_FONTTYPE)
        {
            if (ntmFlags & NTM_TT_OPENTYPE)
                *pdwData |= TT_OPENTYPE_FONT;
            else
                *pdwData |= TT_FONT;
        }
        else if (FontType & DEVICE_FONTTYPE)
            *pdwData |= DEVICE_FONT;
    }
}

BOOL CALLBACK AFX_EXPORT CFontComboBox::EnumFamScreenCallBackEx(ENUMLOGFONTEX* pelf,
    NEWTEXTMETRICEX* lpntm, int FontType, LPVOID pThis)
{
    // don't put in non-printer raster fonts
    if (FontType & RASTER_FONTTYPE)
        return 1;

    DWORD dwData = 0;

    vGetFontType(lpntm, FontType, &dwData);
    ((CFontComboBox *)pThis)->AddFont((ENUMLOGFONT*)pelf, dwData, CString(pelf->elfScript));
    return 1;
}

BOOL CALLBACK AFX_EXPORT CFontComboBox::EnumFamPrinterCallBackEx(ENUMLOGFONTEX* pelf,
    NEWTEXTMETRICEX* lpntm, int FontType, LPVOID pThis)
{
    DWORD dwData = PRINTER_FONT;

    vGetFontType(lpntm, FontType, &dwData);
    ((CFontComboBox *)pThis)->AddFont((ENUMLOGFONT*)pelf, dwData, CString(pelf->elfScript));
    return 1;
}

void CFontComboBox::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
    ASSERT(lpDIS->CtlType == ODT_COMBOBOX);
    int id = (int)(WORD)lpDIS->itemID;

    CDC *pDC = CDC::FromHandle(lpDIS->hDC);
    CRect rc(lpDIS->rcItem);
    if (lpDIS->itemState & ODS_FOCUS)
        pDC->DrawFocusRect(rc);
    int nIndexDC = pDC->SaveDC();

    CBrush brushFill;
    if (lpDIS->itemState & ODS_SELECTED)
    {
        brushFill.CreateSolidBrush(::GetSysColor(COLOR_HIGHLIGHT));
        pDC->SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
    }
    else
        brushFill.CreateSolidBrush(pDC->GetBkColor());
    pDC->SetBkMode(TRANSPARENT);
    pDC->FillRect(rc, &brushFill);

    CFontDesc* pDesc= (CFontDesc*)lpDIS->itemData;
    ASSERT(pDesc != NULL);
    DWORD dwData = pDesc->m_dwFlags;
    if (dwData) // truetype or device flag set by SetItemData
    {
        CDC dc;
        dc.CreateCompatibleDC(pDC);
        CBitmap* pBitmap = dc.SelectObject(&m_bmFontType);
        int xSrc;

        if (dwData & TT_FONT)
            xSrc = 1;
        else if (dwData & TT_OPENTYPE_FONT)
            xSrc = 2;
        else if (dwData & PS_OPENTYPE_FONT)
            xSrc = 3;
        else if (dwData & TYPE1_FONT)
            xSrc = 4;
        else // DEVICE_FONT
            xSrc = 0;

        pDC->BitBlt(rc.left, rc.top, BMW, BMH, &dc, xSrc*BMW, 0, SRCAND);
        dc.SelectObject(pBitmap);
    }

    rc.left += BMW + 6;
    CString strText;
    GetLBText(id, strText);
    pDC->TextOut(rc.left,rc.top,strText,strText.GetLength());

    pDC->RestoreDC(nIndexDC);
}

void CFontComboBox::MeasureItem(LPMEASUREITEMSTRUCT lpMIS)
{
    ASSERT(lpMIS->CtlType == ODT_COMBOBOX);
    ASSERT(m_nFontHeight > 0);
    CRect rc;

    GetWindowRect(&rc);
    lpMIS->itemWidth = rc.Width();
    lpMIS->itemHeight = max(BMH, m_nFontHeight);
}

int CFontComboBox::CompareItem(LPCOMPAREITEMSTRUCT lpCIS)
{
    ASSERT(lpCIS->CtlType == ODT_COMBOBOX);
    int id1 = (int)(WORD)lpCIS->itemID1;
    int id2 = (int)(WORD)lpCIS->itemID2;
    CString str1,str2;
    if (id1 == -1)
        return -1;
    if (id2 == -1)
        return 1;
    GetLBText(id1, str1);
    GetLBText(id2, str2);
    return str1.Collate(str2);
}

/////////////////////////////////////////////////////////////////////////////
// CSizeComboBox

CSizeComboBox::CSizeComboBox()
{
    m_nTwipsLast = 0;
}

void CSizeComboBox::EnumFontSizes(CDC& dc, LPCTSTR pFontName)
{
    ResetContent();
    if (pFontName == NULL)
        return;
    if (pFontName[0] == NULL)
        return;

    ASSERT(dc.m_hDC != NULL);
    m_nLogVert = dc.GetDeviceCaps(LOGPIXELSY);

    ::EnumFontFamilies(dc.m_hDC, pFontName,
        (FONTENUMPROC) EnumSizeCallBack, (LPARAM) this);
}

void CSizeComboBox::TwipsToPointString(LPTSTR lpszBuf, int nTwips)
{
    ASSERT(lpszBuf != NULL);
    lpszBuf[0] = NULL;
    if (nTwips >= 0)
    {
        // round to nearest half point
        nTwips = (nTwips+5)/10;
        if ((nTwips%2) == 0)
            _stprintf(lpszBuf, _T("%ld"), nTwips/2);
        else
            _stprintf(lpszBuf, _T("%.1f"), (float)nTwips/2.F);
    }
}

void CSizeComboBox::SetTwipSize(int nTwips)
{
    if (nTwips != GetTwipSize())
    {
        TCHAR buf[10];
        TwipsToPointString(buf, nTwips);
        SetTheText(buf, TRUE);
    }
    m_nTwipsLast = nTwips;
}

int CSizeComboBox::GetTwipSize()
{
    // return values
    // -2 -- error
    // -1 -- edit box empty
    // >=0 -- font size in twips
    CString str;
    GetTheText(str);
    LPCTSTR lpszText = str;

    while (*lpszText == ' ' || *lpszText == '\t')
        lpszText++;

    if (lpszText[0] == NULL)
        return -1; // no text in control

    double d = _tcstod(lpszText, (LPTSTR*)&lpszText);
    while (*lpszText == ' ' || *lpszText == '\t')
        lpszText++;

    if (*lpszText != NULL)
        return -2;   // not terminated properly

    return (d<0.) ? 0 : (int)(d*20.);
}

BOOL CALLBACK AFX_EXPORT CSizeComboBox::EnumSizeCallBack(LOGFONT FAR* /*lplf*/,
        LPNEWTEXTMETRIC lpntm, int FontType, LPVOID lpv)
{
    CSizeComboBox* pThis = (CSizeComboBox*)lpv;
    ASSERT(pThis != NULL);
    TCHAR buf[10];
    if (
        (FontType & TRUETYPE_FONTTYPE) ||
        !( (FontType & TRUETYPE_FONTTYPE) || (FontType & RASTER_FONTTYPE) )
        ) // if truetype or vector font
    {
        // this occurs when there is a truetype and nontruetype version of a font
        if (pThis->GetCount() != 0)
            pThis->ResetContent();

        for (int i = 0; i < 16; i++)
        {
            wsprintf(buf, _T("%d"), nFontSizes[i]);
            pThis->AddString(buf);
        }
        return FALSE; // don't call me again
    }
    // calc character height in pixels
    pThis->InsertSize(MulDiv(lpntm->tmHeight-lpntm->tmInternalLeading,
        1440, pThis->m_nLogVert));
    return TRUE; // call me again
}

void CSizeComboBox::InsertSize(int nSize)
{
    ASSERT(nSize > 0);
    DWORD dwSize = (DWORD)nSize;
    TCHAR buf[10];
    TwipsToPointString(buf, nSize);
    if (FindStringExact(-1, buf) == CB_ERR)
    {
        int nIndex = -1;
        int nPos = 0;
        DWORD_PTR dw;
        while ((dw = GetItemData(nPos)) != CB_ERR)
        {
            if (dw > dwSize)
            {
                nIndex = nPos;
                break;
            }
            nPos++;
        }
        nIndex = InsertString(nIndex, buf);
        ASSERT(nIndex != CB_ERR);
        if (nIndex != CB_ERR)
            SetItemData(nIndex, dwSize);
    }
}

/////////////////////////////////////////////////////////////////////////////
// CLocalComboBox

BEGIN_MESSAGE_MAP(CLocalComboBox, CComboBox)
    //{{AFX_MSG_MAP(CLocalComboBox)
    ON_WM_CREATE()
    //}}AFX_MSG_MAP
    // Global help commands
END_MESSAGE_MAP()

void CLocalComboBox::GetTheText(CString& str)
{
    int nIndex = GetCurSel();
    if (nIndex == CB_ERR)
        GetWindowText(str);
    else
        GetLBText(nIndex, str);
}

void CLocalComboBox::SetTheText(LPCTSTR lpszText,BOOL bMatchExact)
{
    int idx = (bMatchExact) ? FindStringExact(-1,lpszText) :
        FindString(-1, lpszText);
    SetCurSel( (idx==CB_ERR) ? -1 : idx);
    if (idx == CB_ERR)
        SetWindowText(lpszText);
}

BOOL CLocalComboBox::LimitText(int nMaxChars)
{
    BOOL b = CComboBox::LimitText(nMaxChars);
    if (b)
        m_nLimitText = nMaxChars;
    return b;
}

int CLocalComboBox::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CComboBox::OnCreate(lpCreateStruct) == -1)
        return -1;
    SendMessage(WM_SETFONT, (WPARAM)GetStockObject(theApp.m_nDefFont));
    return 0;
}

BOOL CLocalComboBox::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN)
    {
        CFormatBar* pBar = (CFormatBar*)GetParent();
        switch (pMsg->wParam)
        {
        case VK_ESCAPE:
            pBar->SyncToView();
            pBar->NotifyOwner(NM_RETURN);
            return TRUE;
        case VK_RETURN:
            pBar->NotifyOwner(NM_RETURN);
            return TRUE;
        case VK_TAB:
            pBar->GetNextDlgTabItem(this)->SetFocus();
            return TRUE;
        case VK_UP:
        case VK_DOWN:
            if ((GetKeyState(VK_MENU) >= 0) && (GetKeyState(VK_CONTROL) >=0) &&
                !GetDroppedState())
            {
                ShowDropDown();
                return TRUE;
            }
        }
    }

    //
    // Unless we catch the 'CatchKeys' keys here, MFC will wander around
    // trying to figure out what to do with them and eventually realize
    // that they are accellerators for the frame window and dispatch them
    // there.  We want them sent to the combobox's edit controls.
    //

    if (WM_KEYDOWN == pMsg->message || WM_SYSKEYDOWN == pMsg->message)
    {
        static const struct
        {
            int     modifier;
            WPARAM  virtkey;
        }
        CatchKeys[] =
        {
            {VK_CONTROL, 'C'},          // control-C copy
            {VK_CONTROL, 'V'},          // control-V paste
            {VK_MENU,    VK_BACK},      // alt-back undo
            {VK_SHIFT,   VK_DELETE},    // shift-delete cut
            {VK_CONTROL, VK_INSERT},    // control-insert copy
            {VK_SHIFT,   VK_INSERT},    // shift-insert paste
            {VK_CONTROL, 'X'},          // control-X cut
            {VK_CONTROL, 'Z'}           // control-Z undo
        };

        for (int i = 0; i < sizeof(CatchKeys)/sizeof(CatchKeys[0]); i++)
        {
            if (pMsg->wParam == CatchKeys[i].virtkey)
            {
                if (GetKeyState(CatchKeys[i].modifier) < 0)
                {
                    ::TranslateMessage(pMsg);
                    ::DispatchMessage(pMsg);
                    return TRUE;
                }
            }
        }
    }

    return CComboBox::PreTranslateMessage(pMsg);
}

void CFormatBar::NotifyOwner(UINT nCode)
{
    NMHDR nm;
    nm.hwndFrom = m_hWnd;
    nm.idFrom = GetDlgCtrlID();
    nm.code = nCode;
    GetOwner()->SendMessage(WM_NOTIFY, nm.idFrom, (LPARAM)&nm);
}

void CFormatBar::SetCharFormat(CCharFormat& cf)
{
    CHARHDR fnm;
    fnm.hwndFrom = m_hWnd;
    fnm.idFrom = GetDlgCtrlID();
    fnm.code = FN_SETFORMAT;
    fnm.cf = cf;
    VERIFY(GetOwner()->SendMessage(WM_NOTIFY, fnm.idFrom, (LPARAM)&fnm));
}
