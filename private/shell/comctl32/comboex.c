#include "ctlspriv.h"

#ifdef UNIX

#define EDIT_SELECTALL( hwnd )
#define GetTextExtentPoint GetTextExtentPoint32

#else

#define EDIT_SELECTALL( hwnd )  Edit_SetSel(hwnd, 0, 0);  \
                                Edit_SetSel(hwnd, 0, -1);

#endif

const TCHAR FAR c_szComboBox[] = TEXT("combobox");
const TCHAR FAR c_szComboBoxEx[] = WC_COMBOBOXEX;


#define COMBO_MARGIN        4
#define COMBO_WIDTH         g_cxSmIcon
#define COMBO_HEIGHT        g_cySmIcon
#define COMBO_BORDER        3

typedef struct {
    LPTSTR pszText;
    int iImage;
    int iSelectedImage;
    int iOverlay;
    int iIndent;
    LPARAM lParam;
} CEITEM, *PCEITEM;


typedef struct {
    CONTROLINFO ci;
    HWND hwndCombo;
    HWND hwndEdit;
    DWORD dwExStyle;
    HIMAGELIST himl;
    HFONT hFont;
    int cxIndent;
    WPARAM iSel;
    CEITEM cei;
    BOOL fEditItemSet       :1;
    BOOL fEditChanged       :1;
    BOOL fFontCreated       :1;
    BOOL fInEndEdit         :1;
    BOOL fInDrop            :1;
} COMBOEX, *PCOMBOBOXEX;


void ComboEx_OnWindowPosChanging(PCOMBOBOXEX pce, LPWINDOWPOS pwp);
HFONT ComboEx_GetFont(PCOMBOBOXEX pce);
BOOL ComboEx_OnGetItem(PCOMBOBOXEX pce, PCOMBOBOXEXITEM pceItem);
int ComboEx_ComputeItemHeight(PCOMBOBOXEX pce, BOOL);
int ComboEx_OnFindStringExact(PCOMBOBOXEX pce, int iStart, LPCTSTR lpsz);
int WINAPI ShellEditWordBreakProc(LPTSTR lpch, int ichCurrent, int cch, int code);

LRESULT CALLBACK ComboSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
int ComboEx_StrCmp(PCOMBOBOXEX pce, LPCTSTR psz1, LPCTSTR psz2);

#define ComboEx_Editable(pce) (((pce)->ci.style & CBS_DROPDOWNLIST) == CBS_DROPDOWN)

void ComboEx_OnSetFont(PCOMBOBOXEX pce, HFONT hFont, BOOL fRedraw)
{
    int iHeight;
    HFONT hfontOld = NULL;

    if (pce->fFontCreated)
        hfontOld = ComboEx_GetFont(pce);

    if (!hFont) {
        LOGFONT lf;
        SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, FALSE);
        hFont = CreateFontIndirect(&lf);
        pce->fFontCreated = TRUE;
    } else {
        pce->fFontCreated = FALSE;
    }
    pce->ci.uiCodePage = GetCodePageForFont(hFont);

    SendMessage(pce->hwndCombo, WM_SETFONT, (WPARAM)hFont, fRedraw);
    if (pce->hwndEdit)
    {
        SendMessage(pce->hwndEdit, WM_SETFONT, (WPARAM)hFont, fRedraw);
        SendMessage(pce->hwndEdit, EM_SETMARGINS, EC_USEFONTINFO, 0L);
    }

    iHeight = ComboEx_ComputeItemHeight(pce, FALSE);
    SendMessage(pce->ci.hwnd, CB_SETITEMHEIGHT, (WPARAM)-1, (LPARAM)iHeight);
    SendMessage(pce->hwndCombo, CB_SETITEMHEIGHT, 0, (LPARAM)iHeight);

    // do this last so that we don't have a nuked font as we try to create the new one
    if (hfontOld)
        DeleteObject(hfontOld);
}


void ComboEx_OnDestroy(PCOMBOBOXEX pce)
{
    // don't need do destroy hwndCombo.. it will be destroyed along with us.
    SendMessage(pce->hwndCombo, CB_RESETCONTENT, 0, 0);
    // we may still have string allocated for the item in the edit box so free it
    if (pce->cei.pszText)
        Str_Set(&(pce->cei.pszText), NULL);
    if (pce->fFontCreated) {
        DeleteObject(ComboEx_GetFont(pce));
    }

    if (pce->hwndEdit)
        RemoveWindowSubclass(pce->hwndEdit,  EditSubclassProc,  0);

    if (pce->hwndCombo)
        RemoveWindowSubclass(pce->hwndCombo, ComboSubclassProc, 0);

    SetWindowPtr(pce->ci.hwnd, 0, 0);
    LocalFree(pce);
}

// this gets the client rect without the scrollbar part and the border
void ComboEx_GetComboClientRect(PCOMBOBOXEX pce, LPRECT lprc)
{
    GetClientRect(pce->hwndCombo, lprc);
    InflateRect(lprc, -g_cxEdge, -g_cyEdge);
    lprc->right -= g_cxScrollbar;
}

// returns the edit box (creating it if necessary) or NULL if the combo does
// not have an edit box
HWND ComboEx_GetEditBox(PCOMBOBOXEX pce)
{
    HFONT hfont;
    DWORD dwStyle;
    DWORD dwExStyle = 0;

    if (pce->hwndEdit)
        return(pce->hwndEdit);

    if (!ComboEx_Editable(pce))
        return(NULL);

    dwStyle = WS_VISIBLE | WS_CLIPSIBLINGS | WS_CHILD | ES_LEFT;

    if (pce->ci.style & CBS_AUTOHSCROLL)
        dwStyle |= ES_AUTOHSCROLL;
    if (pce->ci.style & CBS_OEMCONVERT)
        dwStyle |= ES_OEMCONVERT;
#if 0
    if (pce->ci.style & CBS_UPPERCASE)
        dwStyle |= ES_UPPERCASE;
    if (pce->ci.style & CBS_LOWERCASE)
        dwStyle |= ES_LOWERCASE;
#endif

    dwExStyle = pce->ci.dwExStyle & (WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR);

    pce->hwndEdit = CreateWindowEx(dwExStyle, c_szEdit, c_szNULL, dwStyle, 0, 0, 0, 0,
                                   pce->hwndCombo, (HMENU)GetDlgCtrlID(pce->ci.hwnd), HINST_THISDLL, 0);

    if (!pce->hwndEdit ||
        !SetWindowSubclass(pce->hwndEdit, EditSubclassProc, 0, (DWORD_PTR)pce))
    {
        return NULL;
    }

    hfont = ComboEx_GetFont(pce);
    if (hfont)
        FORWARD_WM_SETFONT(pce->hwndEdit, hfont,
                           FALSE, SendMessage);

    return(pce->hwndEdit);
}

///
/// the edit box handling...
/*

 we want the edit box up on CBN_SETFOCUS and CBN_CLOSEUP
 remove it on CBN_DROPDOWN and on CBN_KILLFOCUS

 this assumes that CBN_SETFOCUS and CBN_KILLFOCUS will come before and after
 CBN_DROPDOWN and CBN_CLOSEUP respectively
 */

// Really a BOOL return value
LRESULT ComboEx_EndEdit(PCOMBOBOXEX pce, int iWhy)
{
    NMCBEENDEDIT    nm;
    LRESULT         fRet;

    if (!ComboEx_GetEditBox(pce))
        return(FALSE);

    pce->fInEndEdit = TRUE;

    GetWindowText(pce->hwndEdit, nm.szText, ARRAYSIZE(nm.szText));

    nm.fChanged = pce->fEditChanged;
    nm.iWhy = iWhy;

    nm.iNewSelection = ComboEx_OnFindStringExact(pce, ComboBox_GetCurSel(pce->hwndCombo) - 1, nm.szText);
    fRet = BOOLFROMPTR(CCSendNotify(&pce->ci, CBEN_ENDEDIT, &nm.hdr));

    pce->fInEndEdit = FALSE;

    if (!fRet) 
    {
        if (nm.iNewSelection != ComboBox_GetCurSel(pce->hwndCombo))
        {
            if (nm.iNewSelection != -1)
            {
                SendMessage(pce->ci.hwnd, CB_SETCURSEL, nm.iNewSelection, 0);
            }
            else
            {
                //if the selection is -1 and if we do a CB_SETCURSEL  on comboboxex then it nukes the text in
                //the edit window. Which is not the desired behavior. We need to update the Current Selection in the                 
                //child combobox but leave the text as it is.
                SendMessage(pce->hwndCombo, CB_SETCURSEL, nm.iNewSelection,0);
            }
        }
        pce->fEditChanged = FALSE;
    }
    InvalidateRect(pce->hwndCombo, NULL, FALSE);

    return(fRet);
}

void ComboEx_SizeEditBox(PCOMBOBOXEX pce)
{
    RECT rc;
    int cxIcon = 0, cyIcon = 0;

    ComboEx_GetComboClientRect(pce, &rc);
    InvalidateRect(pce->hwndCombo, &rc, TRUE); // erase so that the selection highlight is erased
    if (pce->himl && !(pce->dwExStyle & CBES_EX_NOEDITIMAGEINDENT))
    {
        // Make room for icons.
        ImageList_GetIconSize(pce->himl, &cxIcon, &cyIcon);
        if (cxIcon)
            cxIcon += COMBO_MARGIN;
    }

    // combobox edit field is one border in from the entire combobox client
    // rect -- thus add one border to edit control's left side
    rc.left += g_cxBorder + cxIcon;
    rc.bottom -= g_cyBorder;
    rc.top = rc.bottom - ComboEx_ComputeItemHeight(pce, TRUE) - g_cyBorder;
    SetWindowPos(pce->hwndEdit, NULL, rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc),
                 SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW);

}

BOOL ComboEx_GetCurSelText(PCOMBOBOXEX pce, LPTSTR pszText, int cchText)
{

    COMBOBOXEXITEM cei;
    BOOL bRet = TRUE;

    cei.mask = CBEIF_TEXT;
    cei.pszText = pszText;
    cei.cchTextMax = cchText;
    cei.iItem = ComboBox_GetCurSel(pce->hwndCombo);
    
    if (cei.iItem == -1 ) 
    {
        pszText[0] = 0;
        bRet = FALSE;
    } 
    else 
    {
        ComboEx_OnGetItem(pce, &cei);
    }
    return bRet;
}

void ComboEx_UpdateEditText(PCOMBOBOXEX pce, BOOL fClearOnNoSel)
{
    if (!pce->fInEndEdit)
    {
        TCHAR szText[CBEMAXSTRLEN];

        HWND hwndEdit = ComboEx_Editable(pce) ? pce->hwndEdit : pce->hwndCombo;

        if (ComboEx_GetCurSelText(pce, szText, ARRAYSIZE(szText)) || fClearOnNoSel) {
            SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)szText);
            EDIT_SELECTALL( hwndEdit );
        }
    }
}

BOOL ComboEx_BeginEdit(PCOMBOBOXEX pce)
{
    if (!ComboEx_GetEditBox(pce))
        return(FALSE);

    SetFocus(pce->hwndEdit);
    return(TRUE);
}

BOOL ComboSubclass_HandleButton(PCOMBOBOXEX pce, WPARAM wParam, LPARAM lParam)
{
    RECT rc;
#ifndef UNIX
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
#else
    POINT pt;
    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);
#endif

    ComboEx_GetComboClientRect(pce, &rc);
    InflateRect(&rc, g_cxEdge, g_cyEdge);

    if (PtInRect(&rc, pt)) {

        //
        //  CheckForDragBegin yields, so we must revalidate on the way back.
        //
        HWND hwndCombo = pce->hwndCombo;
        if (CheckForDragBegin(pce->hwndCombo, LOWORD(lParam), HIWORD(lParam)))
        {
            NMCBEDRAGBEGIN  nmcbebd;
            LRESULT fRet;

            nmcbebd.iItemid = -1;
            GetWindowText(pce->hwndEdit, nmcbebd.szText, ARRAYSIZE(nmcbebd.szText));

            // BUGBUG - raymondc - why do we ignore the return code?
            fRet = CCSendNotify(&pce->ci, CBEN_DRAGBEGIN, &nmcbebd.hdr);
            return TRUE;
        }
        // CheckForDragBegin yields, so revalidate before continuing
        else if (IsWindow(hwndCombo)) {

            // a click on our border should start edit mode as well
            if (ComboEx_Editable(pce)) {
                if (!ComboEx_BeginEdit(pce))
                    SetFocus(pce->hwndCombo);
                return TRUE;
            }
            return FALSE;
        }
   }
   return FALSE;
}

BOOL ComboSubclass_HandleCommand(PCOMBOBOXEX pce, WPARAM wParam, LPARAM lParam)
{
    UINT idCmd = GET_WM_COMMAND_ID(wParam, lParam);
    UINT uCmd = GET_WM_COMMAND_CMD(wParam, lParam);
    HWND hwnd = GET_WM_COMMAND_HWND(wParam, lParam);

    switch (uCmd)
    {
        case EN_SETFOCUS:
            if (!pce->fInDrop)
            {
                EDIT_SELECTALL( pce->hwndEdit );
                CCSendNotify(&pce->ci, CBEN_BEGINEDIT, NULL);
                pce->fEditChanged = FALSE;
            }
            break;

        case EN_KILLFOCUS:
        {
            HWND hwndFocus;
            hwndFocus = GetFocus();
            if (hwndFocus != pce->hwndCombo)
            {
                ComboEx_EndEdit(pce, CBENF_KILLFOCUS);
                SendMessage(pce->hwndCombo, WM_KILLFOCUS, (WPARAM)hwndFocus, 0);
            }

            break;
        }

        case EN_CHANGE:
        {
            TCHAR szTextOrig[CBEMAXSTRLEN];
            TCHAR szTextNow[CBEMAXSTRLEN];
            WPARAM iItem;

            iItem = ComboBox_GetCurSel(pce->hwndCombo);

            if(iItem == -1)
            {
                if (pce->fEditItemSet && pce->cei.pszText) 
                {
                    Str_GetPtr(pce->cei.pszText, szTextOrig, ARRAYSIZE(szTextOrig));
                }
                else
                {
                    szTextOrig[0] = TEXT('\0');
                }
            }
            else 
            {
                ComboEx_GetCurSelText(pce,szTextOrig, ARRAYSIZE(szTextOrig));
            }

#ifndef UNIX
            GetWindowText(pce->hwndEdit, szTextNow, ARRAYSIZE(szTextNow));
#else
            GetWindowText(pce->hwndEdit, szTextNow, ARRAYSIZE(szTextNow)-1);
#endif
            pce->fEditChanged = (ComboEx_StrCmp(pce, szTextOrig, szTextNow) != 0);
            SendMessage(pce->ci.hwndParent, WM_COMMAND,
                    GET_WM_COMMAND_MPS(idCmd, pce->ci.hwnd, CBN_EDITCHANGE));

            break;
        }
    }

    return(hwnd == pce->hwndEdit);
}

void EraseWindow(HWND hwnd, HDC hdc, COLORREF clr)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    FillRectClr(hdc, &rc, clr);
}

LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    PCOMBOBOXEX pce = (PCOMBOBOXEX)dwRefData;

    if (uMsg == WM_SETFONT ||
        uMsg == WM_WININICHANGE) {
        return DefSubclassProc(hwnd, uMsg, wParam, lParam);

    }

    switch(uMsg) {
    case WM_ERASEBKGND:
        EraseWindow(hwnd, (HDC)wParam, GetSysColor(COLOR_WINDOW));
        break;

    case WM_DESTROY:
        RemoveWindowSubclass(hwnd, EditSubclassProc, 0);
        break;

    case WM_CHAR:
        switch ((TCHAR)wParam) {
        case TEXT('\n'):
        case TEXT('\r'):
            // return... don't go to wndproc because
            // the edit control beeps on enter
            return 0;
        }
        break;

    case WM_SIZE:
        if (GetFocus() != hwnd) {
            Edit_SetSel(pce->hwndEdit, 0, 0);    // makesure everything is scrolled over first
        }
        break;

    case WM_KEYDOWN:
        switch(wParam) {
        case VK_RETURN:
            if (!ComboEx_EndEdit(pce, CBENF_RETURN))
                // we know we have an edit window, so FALSE return means
                // app returned FALSE to CBEN_ENDEDIT notification
                ComboEx_BeginEdit(pce);
            break;

        case VK_ESCAPE:
            pce->fEditChanged = FALSE;
            if (!ComboEx_EndEdit(pce, CBENF_ESCAPE)) {
                if(pce->fEditItemSet) {
                    if(pce->cei.pszText) {
                        SendMessage(pce->hwndEdit, WM_SETTEXT, (WPARAM)0, (LPARAM)pce->cei.pszText);
                        EDIT_SELECTALL( pce->hwndEdit );
                    }
                    RedrawWindow(pce->hwndCombo, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
                }else {
                    ComboEx_BeginEdit(pce);
                }
            }
            break;

        // Pass these to the combobox itself to make it work properly...
        case VK_HOME:
        case VK_END:
            if (!pce->fInDrop)
                break;

        case VK_F4:
        case VK_UP:
        case VK_DOWN:
        case VK_PRIOR:
        case VK_NEXT:
            if (pce->hwndCombo)
                return SendMessage(pce->hwndCombo, uMsg, wParam, lParam);
            break;
        }
        break;

    case WM_LBUTTONDOWN:
        if (GetFocus() != pce->hwndEdit)
        {
            SetFocus(pce->hwndEdit);
#ifndef UNIX
            // IEUNIX : since we disabled autoselection on first click in address bar,
            // we should not eat this message. This allows the dragging to begin with
            // the first click.
            return(0L); // eat this message
#endif
        }
        break;

    case WM_SYSKEYDOWN:
        switch(wParam) {
        // Pass these to the combobox itself to make it work properly...
        case VK_UP:
        case VK_DOWN:
            {
                LRESULT lR;
                if (pce->hwndCombo)
                {
                    lR=SendMessage(pce->hwndCombo, uMsg, wParam, lParam);
#ifdef KEYBOARDCUES
                    //notify of navigation key usage
                    CCNotifyNavigationKeyUsage(&(pce->ci), UISF_HIDEFOCUS);
#endif
                    return lR;
                }
            }
        }
    }

    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

LRESULT ComboEx_GetLBText(PCOMBOBOXEX pce, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    COMBOBOXEXITEM cei;
    TCHAR szText[CBEMAXSTRLEN];
    cei.mask = CBEIF_TEXT;
    cei.pszText = szText;
    cei.cchTextMax = ARRAYSIZE(szText);
    cei.iItem = (INT_PTR)wParam;
    if (!ComboEx_OnGetItem(pce, &cei))
        return CB_ERR;

    if (lParam && uMsg == CB_GETLBTEXT) {
#ifdef UNICODE_WIN9x
        if (pce->ci.bUnicode) {
            lstrcpy((LPTSTR)lParam, szText);
        } else {
            WideCharToMultiByte(pce->ci.uiCodePage, 0, szText, -1, (LPSTR)lParam, CBEMAXSTRLEN, NULL,NULL);
        }
#else
        lstrcpy((LPTSTR)lParam, szText);
#endif
    }
    return lstrlen(szText);
}

LRESULT CALLBACK ComboSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    PCOMBOBOXEX pce = (PCOMBOBOXEX)dwRefData;

    switch (uMsg) {

    case WM_ERASEBKGND:
        EraseWindow(hwnd, (HDC)wParam, GetSysColor(COLOR_WINDOW));
        break;

    case CB_GETLBTEXT:
    case CB_GETLBTEXTLEN:
        return ComboEx_GetLBText(pce, uMsg, wParam, lParam);

    case WM_RBUTTONDOWN:
        //Fall Thru
    case WM_LBUTTONDOWN:
        if (ComboSubclass_HandleButton(pce, wParam, lParam)) {
            return 0;
        }
        break;

    case WM_COMMAND:
        if (ComboSubclass_HandleCommand(pce, wParam, lParam))
            return 0;
        break;

    case WM_DESTROY:
        RemoveWindowSubclass(hwnd, ComboSubclassProc, 0);
        break;

    case WM_SETCURSOR:
        if (pce) {
            NMMOUSE nm = {0};
            nm.dwHitInfo = lParam;
            if (CCSendNotify(&pce->ci, NM_SETCURSOR, &nm.hdr))
                return 0;
        }
        break;
    }

    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

BOOL ComboEx_OnCreate(HWND hwnd, LPCREATESTRUCT lpcs)
{
    PCOMBOBOXEX pce;
    DWORD dwStyle;
    DWORD dwExStyle = 0;

    pce = (PCOMBOBOXEX)LocalAlloc(LPTR, sizeof(COMBOEX));
    if (!pce)
        return FALSE;

    SetWindowPtr(hwnd, 0, pce);

    // BUGBUG: force off borders off ourself
    lpcs->style &= ~(WS_BORDER | WS_VSCROLL | WS_HSCROLL | CBS_UPPERCASE | CBS_LOWERCASE);
    SetWindowLong(hwnd, GWL_STYLE, lpcs->style);
    CIInitialize(&pce->ci, hwnd, lpcs);

    // or in CBS_SIMPLE because we can never allow the sub combo box
    // to have just drop down.. it's either all simple or dropdownlist
    dwStyle = CBS_OWNERDRAWFIXED | CBS_SIMPLE | CBS_NOINTEGRALHEIGHT | WS_VISIBLE |WS_VSCROLL | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

    dwStyle |= (lpcs->style & (CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD));

    if ((lpcs->style & CBS_DROPDOWNLIST) == CBS_SIMPLE)
        dwStyle |= (lpcs->style & (CBS_AUTOHSCROLL | CBS_OEMCONVERT | CBS_UPPERCASE | CBS_LOWERCASE));

    dwExStyle = lpcs->dwExStyle & (WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR);

    pce->hwndCombo = CreateWindowEx(dwExStyle, c_szComboBox, lpcs->lpszName,
                                    dwStyle,
                                    0, 0, lpcs->cx, lpcs->cy,
                                    hwnd, lpcs->hMenu, lpcs->hInstance, 0);

    if (!pce->hwndCombo ||
        !SetWindowSubclass(pce->hwndCombo, ComboSubclassProc, 0, (DWORD_PTR)pce) ||
        (!ComboEx_GetEditBox(pce) && ComboEx_Editable(pce)))
    {
        ComboEx_OnDestroy(pce);
        return FALSE;
    }

    ComboEx_OnSetFont(pce, NULL, FALSE);
    pce->cxIndent = 10;
    pce->iSel = -1;

    ComboEx_OnWindowPosChanging(pce, NULL);
    return TRUE;
}


HIMAGELIST ComboEx_OnSetImageList(PCOMBOBOXEX pce, HIMAGELIST himl)
{
    int iHeight;
    HIMAGELIST himlOld = pce->himl;

    pce->himl = himl;

    iHeight = ComboEx_ComputeItemHeight(pce, FALSE);
    SendMessage(pce->ci.hwnd, CB_SETITEMHEIGHT, (WPARAM)-1, iHeight);
    SendMessage(pce->hwndCombo, CB_SETITEMHEIGHT, 0, iHeight);

    InvalidateRect(pce->hwndCombo, NULL, TRUE);

    if (pce->hwndEdit)
        ComboEx_SizeEditBox(pce);

    return himlOld;
}

void ComboEx_OnDrawItem(PCOMBOBOXEX pce, LPDRAWITEMSTRUCT pdis)
{
    HDC hdc = pdis->hDC;
    RECT rc = pdis->rcItem;
    TCHAR szText[CBEMAXSTRLEN];
    int offset = 0;
    int xString, yString, xCombo;
    int cxIcon = 0, cyIcon = 0;
    int iLen;
    BOOL fSelected = FALSE;
    SIZE sizeText;
    COMBOBOXEXITEM cei;
    BOOL fNoText = FALSE;
    BOOL fEnabled = IsWindowEnabled(pce->hwndCombo);
    BOOL fRTLReading = FALSE;
    UINT OldTextAlign;

    // Setup the dc before we use it.
    fRTLReading = GetWindowLong(pdis->hwndItem, GWL_EXSTYLE) & WS_EX_RTLREADING;
    if (fRTLReading) {
        OldTextAlign = GetTextAlign(hdc);
        SetTextAlign(hdc, OldTextAlign|TA_RTLREADING);
    }

    rc.top += g_cyBorder;

    szText[0] = 0;
    if (pdis->itemID != -1)
    {
        cei.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_OVERLAY | CBEIF_SELECTEDIMAGE| CBEIF_INDENT;
        cei.pszText = szText;
        cei.cchTextMax = ARRAYSIZE(szText);
        cei.iItem = pdis->itemID;

        ComboEx_OnGetItem(pce, &cei);

        if (pce->iSel == (int)pdis->itemID ||
            ((pce->iSel == -1) && ((int)pdis->itemID == ComboBox_GetCurSel(pce->hwndCombo))))
            fSelected = TRUE;
    }
    else {
        if(pce->fEditItemSet) {
            cei.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_OVERLAY | CBEIF_SELECTEDIMAGE| CBEIF_INDENT;
            cei.pszText = szText;
            cei.cchTextMax = ARRAYSIZE(szText);
            cei.iItem = pdis->itemID;

            ComboEx_OnGetItem(pce, &cei);
        }
    }

    if (pce->himl && !(pce->dwExStyle & CBES_EX_NOEDITIMAGEINDENT))
    {
        ImageList_GetIconSize(pce->himl, &cxIcon, &cyIcon);
        if (cxIcon)
            cxIcon += COMBO_MARGIN;
    }

    // if we're not drawing the edit box, figure out how far to indent
    // over
    if (!(pdis->itemState & ODS_COMBOBOXEDIT))
    {
        offset = (pce->cxIndent * cei.iIndent) + COMBO_BORDER;
    }
    else
    {
        if (pce->hwndEdit)
            fNoText = TRUE;

        if (pce->dwExStyle & CBES_EX_NOEDITIMAGEINDENT)
            cxIcon = 0;
    }

    xCombo = rc.left + offset;
    rc.left = xString = xCombo + cxIcon;
    iLen = lstrlen(szText);
    GetTextExtentPoint(hdc, szText, iLen, &sizeText);

    rc.right = rc.left + sizeText.cx;
    rc.left--;
    rc.right++;

    if (pdis->itemAction != ODA_FOCUS)
    {
        int yMid;
        BOOL fTextHighlight = FALSE;;

        yMid = (rc.top + rc.bottom) / 2;
        // center the string within rc
        yString = yMid - (sizeText.cy/2);

        if (pdis->itemState & ODS_SELECTED) {
            if (!(pdis->itemState & ODS_COMBOBOXEDIT) ||
                !ComboEx_Editable(pce)) {
                fTextHighlight = TRUE;
            }
        }

        if ( !fEnabled ) {
            SetBkColor(hdc, g_clrBtnFace);
            SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));
        } else {
            SetBkColor(hdc, GetSysColor(fTextHighlight ?
                            COLOR_HIGHLIGHT : COLOR_WINDOW));
            SetTextColor(hdc, GetSysColor(fTextHighlight ?
                            COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
        }

        if ((pdis->itemState & ODS_COMBOBOXEDIT) &&
                (rc.right > pdis->rcItem.right))
        {
            // Need to clip as user does not!
            rc.right = pdis->rcItem.right;
        }

        if (!fNoText) {
            ExtTextOut(hdc, xString, yString, ETO_OPAQUE | ETO_CLIPPED, &rc, szText, iLen, NULL);
        }

        if (pce->himl && (pdis->itemID != -1 || pce->fEditItemSet) &&
            !((pce->dwExStyle & (CBES_EX_NOEDITIMAGE | CBES_EX_NOEDITIMAGEINDENT)) &&
              (pdis->itemState & ODS_COMBOBOXEDIT))) {

            DWORD fTransparent = 0;

            if ((pdis->itemState & ODS_COMBOBOXEDIT) && !fEnabled) {
                fTransparent = ILD_TRANSPARENT;
            }

            if (pce->himl && (pdis->itemID != -1 || pce->fEditItemSet) &&
                !((pce->dwExStyle & (CBES_EX_NOEDITIMAGE | CBES_EX_NOEDITIMAGEINDENT))))
            {
                ImageList_Draw(pce->himl,
                               (fSelected) ? cei.iSelectedImage : cei.iImage,
                               hdc, xCombo, yMid - (cyIcon/2),
                               INDEXTOOVERLAYMASK(cei.iOverlay) |
                               fTransparent |
                               ((pdis->itemState & ODS_SELECTED) ? (ILD_SELECTED | ILD_FOCUS) : ILD_NORMAL));
            }
        }
    }


    if ((pdis->itemAction == ODA_FOCUS ||
        (pdis->itemState & ODS_FOCUS))
#ifdef KEYBOARDCUES
        && !(CCGetUIState(&(pce->ci)) & UISF_HIDEFOCUS)
#endif
        )
    {
        if (!fNoText) {
            DrawFocusRect(hdc, &rc);
        }
    }

    // Restore the text align in the dc.
    if (fRTLReading) {
        SetTextAlign(hdc, OldTextAlign);
    }
}

int ComboEx_ComputeItemHeight(PCOMBOBOXEX pce, BOOL fTextOnly)
{
    HDC hdc;
    HFONT hfontOld;
    int dyDriveItem;
    SIZE siz;

    hdc = GetDC(NULL);
    hfontOld = ComboEx_GetFont(pce);
    if (hfontOld)
        hfontOld = SelectObject(hdc, hfontOld);

    GetTextExtentPoint(hdc, TEXT("W"), 1, &siz);
    dyDriveItem = siz.cy;

    if (hfontOld)
        SelectObject(hdc, hfontOld);
    ReleaseDC(NULL, hdc);

    if (fTextOnly)
        return dyDriveItem;

    dyDriveItem += COMBO_BORDER;

    // now take into account the icon
    if (pce->himl) {
        int cxIcon = 0, cyIcon = 0;
        ImageList_GetIconSize(pce->himl, &cxIcon, &cyIcon);

        if (dyDriveItem < cyIcon)
            dyDriveItem = cyIcon;
    }

    return dyDriveItem;
}

void ComboEx_OnMeasureItem(PCOMBOBOXEX pce, LPMEASUREITEMSTRUCT pmi)
{

    pmi->itemHeight = ComboEx_ComputeItemHeight(pce, FALSE);

}

void ComboEx_ISetItem(PCOMBOBOXEX pce, PCEITEM pcei, PCOMBOBOXEXITEM pceItem)
{
    if (pceItem->mask & CBEIF_INDENT)
        pcei->iIndent = pceItem->iIndent;
    if (pceItem->mask & CBEIF_IMAGE)
        pcei->iImage = pceItem->iImage;
    if (pceItem->mask & CBEIF_SELECTEDIMAGE)
        pcei->iSelectedImage = pceItem->iSelectedImage;
    if (pceItem->mask & CBEIF_OVERLAY)
        pcei->iOverlay = pceItem->iOverlay;

    if (pceItem->mask & CBEIF_TEXT) {
        Str_Set(&pcei->pszText, pceItem->pszText);
    }

    if (pceItem->mask & CBEIF_LPARAM) {
        pcei->lParam = pceItem->lParam;
    }

}

#define ComboEx_GetItemPtr(pce, iItem) \
        ((PCEITEM)SendMessage((pce)->hwndCombo, CB_GETITEMDATA, iItem, 0))
#define ComboEx_Count(pce) \
        ((int)SendMessage((pce)->hwndCombo, CB_GETCOUNT, 0, 0))


BOOL ComboEx_OnGetItem(PCOMBOBOXEX pce, PCOMBOBOXEXITEM pceItem)
{
    PCEITEM pcei;
    NMCOMBOBOXEX nm;

    if(pceItem->iItem != -1) {
        pcei = ComboEx_GetItemPtr(pce, pceItem->iItem);
    }
    else {
        pcei = &(pce->cei);
    }

    if ((!pcei) || (pcei == (PCEITEM)-1))
        return FALSE;

    nm.ceItem.mask = 0;

    if (pceItem->mask & CBEIF_TEXT) {

        if (pcei->pszText == LPSTR_TEXTCALLBACK) {
            nm.ceItem.mask |= CBEIF_TEXT;
        } else {
            if(pceItem->iItem != -1) {
                Str_GetPtr(pcei->pszText, pceItem->pszText, pceItem->cchTextMax);
            }else {
                SendMessage(pce->hwndEdit, WM_GETTEXT, (WPARAM)pceItem->cchTextMax, (LPARAM)pceItem->pszText);
            }
        }
    }

    if (pceItem->mask & CBEIF_IMAGE) {

        if (pcei->iImage == I_IMAGECALLBACK) {
            nm.ceItem.mask |= CBEIF_IMAGE;
        }
        pceItem->iImage = pcei->iImage;

    }

    if (pceItem->mask & CBEIF_SELECTEDIMAGE) {

        if (pcei->iSelectedImage == I_IMAGECALLBACK) {
            nm.ceItem.mask |= CBEIF_SELECTEDIMAGE;
        }
        pceItem->iSelectedImage = pcei->iSelectedImage;
    }

    if (pceItem->mask & CBEIF_OVERLAY) {

        if (pcei->iOverlay == I_IMAGECALLBACK) {
            nm.ceItem.mask |= CBEIF_OVERLAY;
        }
        pceItem->iOverlay = pcei->iOverlay;
    }

    if (pceItem->mask & CBEIF_INDENT) {

        if (pcei->iIndent == I_INDENTCALLBACK) {
            nm.ceItem.mask |= CBEIF_INDENT;
            pceItem->iIndent = 0;
        } else {
            pceItem->iIndent = pcei->iIndent;
        }
    }

    if (pceItem->mask & CBEIF_LPARAM) {
        pceItem->lParam = pcei->lParam;
    }



    // is there anything to call back for?
    if (nm.ceItem.mask) {
        UINT uMask = nm.ceItem.mask;

        nm.ceItem = *pceItem;
        nm.ceItem.lParam = pcei->lParam;
        nm.ceItem.mask = uMask;

        if ((nm.ceItem.mask & CBEIF_TEXT) &&
            nm.ceItem.cchTextMax) {
            // null terminate just in case they don't respond
            *nm.ceItem.pszText = 0;
        }

        CCSendNotify(&pce->ci, CBEN_GETDISPINFO, &nm.hdr);

        if (nm.ceItem.mask & CBEIF_INDENT)
            pceItem->iIndent = nm.ceItem.iIndent;
        if (nm.ceItem.mask & CBEIF_IMAGE)
            pceItem->iImage = nm.ceItem.iImage;
        if (nm.ceItem.mask & CBEIF_SELECTEDIMAGE)
            pceItem->iSelectedImage = nm.ceItem.iSelectedImage;
        if (nm.ceItem.mask & CBEIF_OVERLAY)
            pceItem->iOverlay = nm.ceItem.iOverlay;
        if (nm.ceItem.mask & CBEIF_TEXT)
            if (pceItem->mask & CBEIF_TEXT)
                pceItem->pszText = CCReturnDispInfoText(nm.ceItem.pszText, pceItem->pszText, pceItem->cchTextMax);
            else
                pceItem->pszText = nm.ceItem.pszText;

        if (nm.ceItem.mask & CBEIF_DI_SETITEM) {

            ComboEx_ISetItem(pce, pcei, &nm.ceItem);
        }
    }
    return TRUE;

}

#ifdef UNICODE
BOOL ComboEx_OnGetItemA(PCOMBOBOXEX pce, PCOMBOBOXEXITEMA pceItem)
{
    LPWSTR pwszText;
    LPSTR pszTextSave;
    BOOL fRet;

    if (!(pceItem->mask & CBEIF_TEXT)) {
        return ComboEx_OnGetItem(pce, (PCOMBOBOXEXITEM)pceItem);
    }

    pwszText = (LPWSTR)LocalAlloc(LPTR, (pceItem->cchTextMax+1)*sizeof(WCHAR));
    if (!pwszText)
        return FALSE;
    pszTextSave = pceItem->pszText;
    ((PCOMBOBOXEXITEM)pceItem)->pszText = pwszText;
    fRet = ComboEx_OnGetItem(pce, (PCOMBOBOXEXITEM)pceItem);
    pceItem->pszText = pszTextSave;

    if (fRet) {
        // BUGBUG: WCTMB failes w/ ERROR_INSUFFICIENT_BUFFER whereas the native-A implementation truncates
        WideCharToMultiByte(CP_ACP, 0, pwszText, -1,
                            (LPSTR)pszTextSave, pceItem->cchTextMax, NULL, NULL);
    }
    LocalFree(pwszText);
    return fRet;

}
#endif

BOOL ComboEx_OnSetItem(PCOMBOBOXEX pce, PCOMBOBOXEXITEM pceItem)
{
    if(pceItem->iItem != -1) {
        PCEITEM pcei = ComboEx_GetItemPtr(pce, pceItem->iItem);
        UINT rdwFlags = 0;

        if (pcei == (PCEITEM)-1)
            return FALSE;

        ComboEx_ISetItem(pce, pcei, pceItem);

        if (rdwFlags & (CBEIF_INDENT | CBEIF_IMAGE |CBEIF_SELECTEDIMAGE | CBEIF_TEXT | CBEIF_OVERLAY)) {
            rdwFlags = RDW_ERASE | RDW_INVALIDATE;
        }
        // BUGBUG: do something better..

        if (rdwFlags) {
            RedrawWindow(pce->hwndCombo, NULL, NULL, rdwFlags);
        }

        if (pceItem->iItem == ComboBox_GetCurSel(pce->hwndCombo))
            ComboEx_UpdateEditText(pce, FALSE);
        // BUGUBG: notify item changed
        return TRUE;

  } else {

        pce->cei.iImage = -1;
        pce->cei.iSelectedImage = -1;

        ComboEx_ISetItem(pce, &(pce->cei), pceItem);

        pce->fEditItemSet = TRUE;

        if (!pce->hwndEdit){
            Str_Set(&pce->cei.pszText, NULL);
            pce->fEditItemSet = FALSE;
            return(CB_ERR);
        }

        if(pce->cei.pszText) {
            SendMessage(pce->hwndEdit, WM_SETTEXT, (WPARAM)0, (LPARAM)pce->cei.pszText);
            EDIT_SELECTALL( pce->hwndEdit );
        }
        RedrawWindow(pce->hwndCombo, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
        return TRUE;

   }
}

void ComboEx_HandleDeleteItem(PCOMBOBOXEX pce, LPDELETEITEMSTRUCT pdis)
{
    PCEITEM pcei = (PCEITEM)pdis->itemData;
    if (pcei) {
        NMCOMBOBOXEX nm;

        Str_Set(&pcei->pszText, NULL);

        nm.ceItem.iItem = pdis->itemID;
        nm.ceItem.mask = CBEIF_LPARAM;
        nm.ceItem.lParam = pcei->lParam;
        CCSendNotify(&pce->ci, CBEN_DELETEITEM, &nm.hdr);

        LocalFree(pcei);
    }
}

LRESULT ComboEx_OnInsertItem(PCOMBOBOXEX pce, PCOMBOBOXEXITEM pceItem)
{
    LRESULT iRet;
    PCEITEM pcei = (PCEITEM)LocalAlloc(LPTR, sizeof(CEITEM));

    if (!pcei)
        return -1;

    pcei->iImage = -1;
    pcei->iSelectedImage = -1;
    //pcei->iOverlay = 0;
    //pcei->iIndent = 0;

    ComboEx_ISetItem(pce, pcei, pceItem);

    iRet = ComboBox_InsertString(pce->hwndCombo, pceItem->iItem, pcei);
    if (iRet != -1) {
        NMCOMBOBOXEX nm;

        nm.ceItem = *pceItem;
        CCSendNotify(&pce->ci, CBEN_INSERTITEM, &nm.hdr);
    }
    return iRet;
}


void ComboEx_OnWindowPosChanging(PCOMBOBOXEX pce, LPWINDOWPOS pwp)
{
    RECT rcWindow, rcClient;
    RECT rc;
    int  cxInner;
    int cy;

    GetWindowRect(pce->ci.hwnd, &rcWindow);

    if (pwp) {
        // check to see if our size & position aren't actually changing (rebar, for one, 
        // does lots of DeferWindowPos calls that don't actually change our size or position
        // but still generate WM_WINDOWPOSCHANGING msgs).  we avoid flicker by bailing here.
        RECT rcWp;
        SetRect(&rcWp, pwp->x, pwp->y, pwp->x + pwp->cx, pwp->y + pwp->cy);
        MapWindowRect(GetParent(pce->ci.hwnd), HWND_DESKTOP, (LPPOINT)&rcWp);
        if (EqualRect(&rcWp, &rcWindow)) {
            // this is a noop, so bail
            return;
        }
    }

    GetClientRect(pce->ci.hwnd, &rcClient);

    if (pwp)
        cxInner = pwp->cx + RECTWIDTH(rcWindow) - RECTWIDTH(rcClient);
    else
        cxInner = RECTWIDTH(rcClient);

    GetWindowRect(pce->hwndCombo, &rc);
    if (cxInner) {

        // don't size the inner combo if width is 0; otherwise, the below
        // computation will make the comboEX the height of the inner combo
        // top + inner combo dropdown instead of JUST the inner combo top
        cy = (pwp && ((pce->ci.style & CBS_DROPDOWNLIST) == CBS_SIMPLE)) ?  pwp->cy : RECTHEIGHT(rc);

        SetWindowPos(pce->hwndCombo, NULL, 0, 0, cxInner, cy,
                                          SWP_NOACTIVATE | (pce->hwndEdit ? SWP_NOREDRAW : 0));
    }

    GetWindowRect(pce->hwndCombo, &rc);

    cy = RECTHEIGHT(rc) + (RECTHEIGHT(rcWindow) - RECTHEIGHT(rcClient));

    if (pwp) {
        if (cy < pwp->cy || !(pce->dwExStyle & CBES_EX_NOSIZELIMIT)) {
            pwp->cy = cy;
        }
    } else {

        if (cy < RECTHEIGHT(rcWindow) || !(pce->dwExStyle & CBES_EX_NOSIZELIMIT)) {
            SetWindowPos(pce->ci.hwnd, NULL, 0, 0,
                         RECTWIDTH(rcWindow),
                         cy,
                         SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
        }
    }

    if (pce->hwndEdit)
    {
        ComboEx_SizeEditBox(pce);
        InvalidateRect(pce->hwndCombo, NULL, TRUE);
    }
}

LRESULT ComboEx_HandleCommand(PCOMBOBOXEX pce, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres;
    UINT idCmd = GET_WM_COMMAND_ID(wParam, lParam);
    UINT uCmd = GET_WM_COMMAND_CMD(wParam, lParam);

    if (!pce)
        return 0;

    if (uCmd == CBN_SELCHANGE)
        // update the edit text before forwarding this notification 'cause in
        // a normal combobox, the edit control will have already been updated
        // upon receipt of this notification
        ComboEx_UpdateEditText(pce, FALSE);

    lres = SendMessage(pce->ci.hwndParent, WM_COMMAND, GET_WM_COMMAND_MPS(idCmd, pce->ci.hwnd, uCmd));

    switch (uCmd) {

    case CBN_DROPDOWN:
        pce->iSel = ComboBox_GetCurSel(pce->hwndCombo);
        ComboEx_EndEdit(pce, CBENF_DROPDOWN);
        if (GetFocus() == pce->hwndEdit)
            SetFocus(pce->hwndCombo);
        pce->fInDrop = TRUE;
        break;

    case CBN_KILLFOCUS:
        ComboEx_EndEdit(pce, CBENF_KILLFOCUS);
        break;

    case CBN_CLOSEUP:
        pce->iSel = -1;
        ComboEx_BeginEdit(pce);
        pce->fInDrop = FALSE;
        break;

    case CBN_SETFOCUS:
        ComboEx_BeginEdit(pce);
        break;

    }

    return lres;
}

LRESULT ComboEx_OnGetItemData(PCOMBOBOXEX pce, WPARAM i)
{
    PCEITEM pcei = (PCEITEM)SendMessage(pce->hwndCombo, CB_GETITEMDATA, i, 0);
    if (pcei == NULL || pcei == (PCEITEM)CB_ERR) {
        return CB_ERR;
    }

    return pcei->lParam;
}

LRESULT ComboEx_OnSetItemData(PCOMBOBOXEX pce, int i, LPARAM lParam)
{
    PCEITEM pcei = (PCEITEM)SendMessage(pce->hwndCombo, CB_GETITEMDATA, i, 0);
    if (pcei == NULL || pcei == (PCEITEM)CB_ERR) {
        return CB_ERR;
    }
    pcei->lParam = lParam;
    return 0;
}

int ComboEx_OnFindStringExact(PCOMBOBOXEX pce, int iStart, LPCTSTR lpsz)
{
    int i;
    int iMax = ComboEx_Count(pce);
    TCHAR szText[CBEMAXSTRLEN];
    COMBOBOXEXITEM cei;

    if (iStart < 0)
        iStart = -1;

    cei.mask = CBEIF_TEXT;
    cei.pszText = szText;
    cei.cchTextMax = ARRAYSIZE(szText);

    for (i = iStart + 1 ; i < iMax; i++) {
        cei.iItem = i;
        if (ComboEx_OnGetItem(pce, &cei)) {
            if (!ComboEx_StrCmp(pce, lpsz, szText)) {
                return i;
            }
        }
    }

    for (i = 0; i <= iStart; i++) {
        cei.iItem = i;
        if (ComboEx_OnGetItem(pce, &cei)) {
            if (!ComboEx_StrCmp(pce, lpsz, szText)) {
                return i;
            }
        }
    }

    return CB_ERR;
}

int ComboEx_StrCmp(PCOMBOBOXEX pce, LPCTSTR psz1, LPCTSTR psz2)
{
    if (pce->dwExStyle & CBES_EX_CASESENSITIVE) {
        return lstrcmp(psz1, psz2);
    }
    return lstrcmpi(psz1, psz2);
}

DWORD ComboEx_OnSetExStyle(PCOMBOBOXEX pce, DWORD dwExStyle, DWORD dwExMask)
{
    DWORD dwRet;
    DWORD dwChange;

    if (dwExMask)
        dwExStyle = (pce->dwExStyle & ~ dwExMask) | (dwExStyle & dwExMask);

    dwRet = pce->dwExStyle;
    dwChange = (pce->dwExStyle ^ dwExStyle);

    pce->dwExStyle = dwExStyle;
    if (dwChange & (CBES_EX_NOEDITIMAGE | CBES_EX_NOEDITIMAGEINDENT)) {
        InvalidateRect(pce->ci.hwnd, NULL, TRUE);
        if (pce->hwndEdit)
        {
            ComboEx_SizeEditBox(pce);
            InvalidateRect(pce->hwndEdit, NULL, TRUE);
        }
    }

    if (dwChange & CBES_EX_PATHWORDBREAKPROC)
        SetPathWordBreakProc(pce->hwndEdit, (pce->dwExStyle & CBES_EX_PATHWORDBREAKPROC));

    return dwRet;
}

HFONT ComboEx_GetFont(PCOMBOBOXEX pce)
{
    if (pce->hwndCombo)
        return (HFONT)SendMessage(pce->hwndCombo, WM_GETFONT, 0, 0);

    return NULL;
}

LRESULT CALLBACK ComboExWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres = 0;
    PCOMBOBOXEX pce = (PCOMBOBOXEX)GetWindowPtr(hwnd, 0);

    if (!pce) {
        if (uMsg != WM_NCCREATE &&
            uMsg != WM_CREATE)
            goto DoDefault;
    }

    switch (uMsg) {
        HANDLE_MSG(pce, WM_SETFONT, ComboEx_OnSetFont);

    case WM_ENABLE:
        if (pce->hwndCombo)
            EnableWindow(pce->hwndCombo, (BOOL) wParam);
        if (pce->hwndEdit)
            EnableWindow(pce->hwndEdit, (BOOL) wParam);
        break;

    case WM_WININICHANGE:
        InitGlobalMetrics(wParam);
        // only need to re-create this font if we created it in the first place
        // and somebody changed the font (or did a wildcard change)
        //
        // NOTE: Some people broadcast a nonclient metrics change when they
        //       change the icon title logfont, so watch for both.
        //
        if (pce && pce->fFontCreated &&
            ((wParam == 0 && lParam == 0) ||
             wParam == SPI_SETICONTITLELOGFONT ||
             wParam == SPI_SETNONCLIENTMETRICS))
        {
            ComboEx_OnSetFont(pce, NULL, TRUE);
        }
        break;

    case WM_SYSCOLORCHANGE:
        InitGlobalColors();
        break;

    case WM_NOTIFYFORMAT:
        return CIHandleNotifyFormat(&pce->ci, lParam);
        break;

    case WM_NCCREATE:
        // strip off the scroll bits
        SetWindowBits(hwnd, GWL_STYLE, WS_BORDER | WS_VSCROLL | WS_HSCROLL, 0);
        goto DoDefault;

    case WM_CREATE:
        if (!ComboEx_OnCreate(hwnd, (LPCREATESTRUCT)lParam))
            lres = -1; // OnCreate falied. Fail WM_CREATE
        break;


    case WM_DESTROY:
        ASSERT(pce);
        ComboEx_OnDestroy(pce);
        break;

    case WM_WINDOWPOSCHANGING:
        ComboEx_OnWindowPosChanging(pce, (LPWINDOWPOS)lParam);
        break;

#if 0
    case WM_SIZE:
        ComboEx_OnSize(pce);
        break;
#endif

    case WM_DRAWITEM:
        ComboEx_OnDrawItem(pce, (LPDRAWITEMSTRUCT)lParam);
        break;

    case WM_MEASUREITEM:
        ComboEx_OnMeasureItem(pce, (LPMEASUREITEMSTRUCT)lParam);
        break;

    case WM_COMMAND:
        return ComboEx_HandleCommand(pce, wParam, lParam);

    case WM_GETFONT:
        return (LRESULT)ComboEx_GetFont(pce);

    case WM_SETFOCUS:
        if (pce->hwndCombo)
            SetFocus(pce->hwndCombo);
        break;

    case WM_DELETEITEM:
        ComboEx_HandleDeleteItem(pce, (LPDELETEITEMSTRUCT)lParam);
        return TRUE;

#ifdef KEYBOARDCUES
    case WM_UPDATEUISTATE:
        //not sure need to set bit, will probably not use it, on the other hand this
        //  is consistent with remaining of common controls and not very expensive
        CCOnUIState(&(pce->ci), WM_UPDATEUISTATE, wParam, lParam);

        goto DoDefault;
#endif
    // this is for backcompat only.
    case CBEM_SETEXSTYLE:
        return ComboEx_OnSetExStyle(pce, (DWORD)wParam, 0);
        
    case CBEM_SETEXTENDEDSTYLE:
        return ComboEx_OnSetExStyle(pce, (DWORD)lParam, (DWORD)wParam);

    case CBEM_GETEXTENDEDSTYLE:
        return pce->dwExStyle;

    case CBEM_GETCOMBOCONTROL:
        return (LRESULT)pce->hwndCombo;

    case CBEM_SETIMAGELIST:
        return (LRESULT)ComboEx_OnSetImageList(pce, (HIMAGELIST)lParam);

    case CBEM_GETIMAGELIST:
        return (LRESULT)pce->himl;

#ifdef UNICODE
    case CBEM_GETITEMA:
        return ComboEx_OnGetItemA(pce, (PCOMBOBOXEXITEMA)lParam);
#endif

    case CBEM_GETITEM:
        return ComboEx_OnGetItem(pce, (PCOMBOBOXEXITEM)lParam);

#ifdef UNICODE
    case CBEM_SETITEMA: {
            LRESULT lResult;
            LPWSTR lpStrings;
            UINT   uiCount;
            LPSTR  lpAnsiString = (LPSTR) ((PCOMBOBOXEXITEM)lParam)->pszText;

           if ((((PCOMBOBOXEXITEM)lParam)->mask & CBEIF_TEXT) &&
               (((PCOMBOBOXEXITEM)lParam)->pszText != LPSTR_TEXTCALLBACK)) {

                uiCount = lstrlenA(lpAnsiString)+1;
                lpStrings = LocalAlloc(LPTR, (uiCount) * sizeof(TCHAR));

                if (!lpStrings)
                    return -1;

                MultiByteToWideChar(CP_ACP, 0, (LPCSTR) lpAnsiString, uiCount,
                                   lpStrings, uiCount);

                ((PCOMBOBOXEXITEMA)lParam)->pszText = (LPSTR)lpStrings;
                lResult = ComboEx_OnSetItem(pce, (PCOMBOBOXEXITEM)lParam);
                ((PCOMBOBOXEXITEMA)lParam)->pszText = lpAnsiString;
                LocalFree(lpStrings);

                return lResult;
            } else {
                return ComboEx_OnSetItem(pce, (PCOMBOBOXEXITEM)lParam);
            }
        }
#endif
    case CBEM_SETITEM:
        return ComboEx_OnSetItem(pce, (PCOMBOBOXEXITEM)lParam);

#ifdef UNICODE
    case CBEM_INSERTITEMA: {
            LRESULT lResult;
            LPWSTR lpStrings;
            UINT   uiCount;
            LPSTR  lpAnsiString = (LPSTR) ((PCOMBOBOXEXITEM)lParam)->pszText;

            if (!lpAnsiString || lpAnsiString == (LPSTR)LPSTR_TEXTCALLBACK)
                return ComboEx_OnInsertItem(pce, (PCOMBOBOXEXITEM)lParam);

            uiCount = lstrlenA(lpAnsiString)+1;
            lpStrings = LocalAlloc(LPTR, (uiCount) * sizeof(TCHAR));

            if (!lpStrings)
                return -1;

            MultiByteToWideChar(CP_ACP, 0, (LPCSTR) lpAnsiString, uiCount,
                               lpStrings, uiCount);

            ((PCOMBOBOXEXITEMA)lParam)->pszText = (LPSTR)lpStrings;
            lResult = ComboEx_OnInsertItem(pce, (PCOMBOBOXEXITEM)lParam);
            ((PCOMBOBOXEXITEMA)lParam)->pszText = lpAnsiString;
            LocalFree(lpStrings);

            return lResult;
        }
#endif

    case CBEM_INSERTITEM:
        return ComboEx_OnInsertItem(pce, (PCOMBOBOXEXITEM)lParam);



    case CBEM_GETEDITCONTROL:
        return (LRESULT)pce->hwndEdit;

    case CBEM_HASEDITCHANGED:
        return pce->fEditChanged;

    case CB_GETITEMDATA:
        return ComboEx_OnGetItemData(pce, (int)wParam);

    case CB_SETITEMDATA:
        return ComboEx_OnSetItemData(pce, (int)wParam, lParam);

    case CB_LIMITTEXT:
        if (ComboEx_GetEditBox(pce))
            Edit_LimitText(pce->hwndEdit, wParam);
        break;

    case CB_FINDSTRINGEXACT:
    {
        LPCTSTR psz = (LPCTSTR)lParam;
#ifdef UNICODE_WIN9x
        TCHAR szText[CBEMAXSTRLEN];
        if (!pce->ci.bUnicode) {
            MultiByteToWideChar(pce->ci.uiCodePage, 0, (LPCSTR)lParam, -1, szText, ARRAYSIZE(szText));
            psz = szText;
        }
#endif
        return ComboEx_OnFindStringExact(pce, (int)wParam, psz);
    }

    case CB_SETITEMHEIGHT:
        lres = SendMessage(pce->hwndCombo, uMsg, wParam, lParam);
        if (wParam == (WPARAM)-1) {
            RECT rcWindow, rcClient;
            int cy;

            GetWindowRect(pce->hwndCombo, &rcWindow);
            cy = RECTHEIGHT(rcWindow);

            GetWindowRect(pce->ci.hwnd, &rcWindow);
            GetClientRect(pce->ci.hwnd, &rcClient);

            cy = cy + (RECTHEIGHT(rcWindow) - RECTHEIGHT(rcClient));

            SetWindowPos(pce->ci.hwnd, NULL, 0, 0,
                         RECTWIDTH(rcWindow),
                         cy,
                         SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
        }
        break;

    case CB_INSERTSTRING:
    case CB_ADDSTRING:
    case CB_SETEDITSEL:
    case CB_FINDSTRING:
    case CB_DIR:
        // override to do nothing
        break;

    case CB_SETCURSEL:
    case CB_RESETCONTENT:
    case CB_DELETESTRING:
        lres = SendMessage(pce->hwndCombo, uMsg, wParam, lParam);
        ComboEx_UpdateEditText(pce, uMsg == CB_SETCURSEL);
        break;

    case WM_SETTEXT:
        if (!pce->hwndEdit)
            return(CB_ERR);
        
#ifdef UNICODE_WIN9x
        // these wm_* messages are always TCHAR
        lres = SendMessageA(pce->hwndEdit, uMsg, wParam, lParam);
#else
        lres = SendMessage(pce->hwndEdit, uMsg, wParam, lParam);
#endif
        EDIT_SELECTALL( pce->hwndEdit );
        RedrawWindow(pce->hwndCombo, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
        return(lres);

    case WM_CUT:
    case WM_COPY:
    case WM_PASTE:
    case WM_GETTEXT:
    case WM_GETTEXTLENGTH:
        if (!pce->hwndEdit)
            return 0;
#ifdef UNICODE_WIN9x
        // these wm_* messages are always TCHAR
        return(SendMessageA(pce->hwndEdit, uMsg, wParam, lParam));
#else
        return(SendMessage(pce->hwndEdit, uMsg, wParam, lParam));
#endif

    case WM_SETREDRAW:
        if (pce->hwndEdit)
            SendMessage(pce->hwndEdit, uMsg, wParam, lParam);
        break;

    case CB_GETEDITSEL:
        if (pce->hwndEdit)
            return SendMessage(pce->hwndEdit, EM_GETSEL, wParam, lParam);
        // else fall through

    // Handle it being in a dialog...
    // BUGBUG:: May want to handle it differently when edit control has
    // focus...
    case WM_GETDLGCODE:
    case CB_SHOWDROPDOWN:
    case CB_SETEXTENDEDUI:
    case CB_GETEXTENDEDUI:
    case CB_GETDROPPEDSTATE:
    case CB_GETDROPPEDCONTROLRECT:
    case CB_GETCURSEL:
    case CB_GETCOUNT:
    case CB_SELECTSTRING:
    case CB_GETITEMHEIGHT:
    case CB_SETDROPPEDWIDTH:
        return SendMessage(pce->hwndCombo, uMsg, wParam, lParam);

    case CB_GETLBTEXT:
    case CB_GETLBTEXTLEN:
        return ComboEx_GetLBText(pce, uMsg, wParam, lParam);

    default:
        if (CCWndProc(&pce->ci, uMsg, wParam, lParam, &lres))
            return lres;

DoDefault:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return lres;
}


BOOL InitComboExClass(HINSTANCE hinst)
{
    WNDCLASS wc;

    if (!GetClassInfo(hinst, c_szComboBoxEx, &wc)) {
        wc.lpfnWndProc     = ComboExWndProc;
        wc.hCursor         = LoadCursor(NULL, IDC_ARROW);
        wc.hIcon           = NULL;
        wc.lpszMenuName    = NULL;
        wc.hInstance       = hinst;
        wc.lpszClassName   = c_szComboBoxEx;
        wc.hbrBackground   = (HBRUSH)(COLOR_WINDOW + 1); // NULL;
        wc.style           = CS_GLOBALCLASS;
        wc.cbWndExtra      = sizeof(PCOMBOBOXEX);
        wc.cbClsExtra      = 0;

        return RegisterClass(&wc);
    }
    return TRUE;

}

//---------------------------------------------------------------------------
// SetPathWordBreakProc does special break processing for edit controls.
//
// The word break proc is called when ctrl-(left or right) arrow is pressed in the
// edit control.  Normal processing provided by USER breaks words at spaces or tabs,
// but for us it would be nice to break words at slashes, backslashes, & periods too
// since it may be common to have paths or url's typed in.
void WINAPI SetPathWordBreakProc(HWND hwndEdit, BOOL fSet)
{
#ifndef WINNT
    // There is a bug with how USER handles WH_CALLWNDPROC global hooks in Win95 that
    // causes us to blow up if one is installed and a wordbreakproc is set.  Thus,
    // if an app is running that has one of these hooks installed (intellipoint 1.1 etc.) then
    // if we install our wordbreakproc the app will fault when the proc is called.  There
    // does not appear to be any way for us to work around it since USER's thunking code
    // trashes the stack so this API is disabled for Win95.
    return;
#else
    FARPROC lpfnOld;
    // Don't shaft folks who set their own break proc - leave it alone.
    lpfnOld = (FARPROC)SendMessage(hwndEdit, EM_GETWORDBREAKPROC, 0, 0L);

    if (fSet) {
        if (!lpfnOld)
            SendMessage(hwndEdit, EM_SETWORDBREAKPROC, 0, (LPARAM)ShellEditWordBreakProc);
    } else {
        if (lpfnOld == (FARPROC)ShellEditWordBreakProc)
            SendMessage(hwndEdit, EM_SETWORDBREAKPROC, 0, 0L);
    }
#endif
}

#ifdef WINNT
BOOL IsDelimiter(TCHAR ch)
{
    return (ch == TEXT(' ')  ||
            ch == TEXT('\t') ||
            ch == TEXT('.')  ||
            ch == TEXT('/')  ||
            ch == TEXT('\\'));
}

int WINAPI ShellEditWordBreakProc(LPTSTR lpch, int ichCurrent, int cch, int code)
{
    LPTSTR lpchT = lpch + ichCurrent;
    int iIndex;
    BOOL fFoundNonDelimiter = FALSE;
    static BOOL fRight = FALSE;  // hack due to bug in USER

    switch (code) {
        case WB_ISDELIMITER:
            fRight = TRUE;
            // Simple case - is the current character a delimiter?
            iIndex = (int)IsDelimiter(*lpchT);
            break;

        case WB_LEFT:
            // Move to the left to find the first delimiter.  If we are
            // currently at a delimiter, then skip delimiters until we
            // find the first non-delimiter, then start from there.
            //
            // Special case for fRight - if we are currently at a delimiter
            // then just return the current word!
            while ((lpchT = CharPrev(lpch, lpchT)) != lpch) {
                if (IsDelimiter(*lpchT)) {
                    if (fRight || fFoundNonDelimiter)
                        break;
                } else {
                    fFoundNonDelimiter = TRUE;
                    fRight = FALSE;
                }
            }
            iIndex = (int) (lpchT - lpch);

            // We are currently pointing at the delimiter, next character
            // is the beginning of the next word.
            if (iIndex > 0 && iIndex < cch)
                iIndex++;

            break;

        case WB_RIGHT:
            fRight = FALSE;

            // If we are not at a delimiter, then skip to the right until
            // we find the first delimiter.  If we started at a delimiter, or
            // we have just finished scanning to the first delimiter, then
            // skip all delimiters until we find the first non delimiter.
            //
            // Careful - the string passed in to us may not be NULL terminated!
            fFoundNonDelimiter = !IsDelimiter(*lpchT);
            if (lpchT != (lpch + cch)) {
                while ((lpchT = FastCharNext(lpchT)) != (lpch + cch)) {
                    if (IsDelimiter(*lpchT)) {
                        fFoundNonDelimiter = FALSE;
                    } else {
                        if (!fFoundNonDelimiter)
                            break;
                    }
                }
            }
            // We are currently pointing at the next word.
            iIndex = (int) (lpchT - lpch);
            break;
    }

    return iIndex;
}
#endif
