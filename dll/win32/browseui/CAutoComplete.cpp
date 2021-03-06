/*
 *    AutoComplete interfaces implementation.
 *
 *    Copyright 2004    Maxime Bellengé <maxime.bellenge@laposte.net>
 *    Copyright 2009  Andrew Hill
 *    Copyright 2020-2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
  Implemented:
  - ACO_AUTOAPPEND style
  - ACO_AUTOSUGGEST style
  - ACO_UPDOWNKEYDROPSLIST style

  - Handle pwzsRegKeyPath and pwszQuickComplete in Init

  TODO:
  - implement ACO_SEARCH style
  - implement ACO_FILTERPREFIXES style
  - implement ACO_USETAB style
  - implement ACO_RTLREADING style

 */

#include "precomp.h"

#define CX_LIST 30160 // width of m_hwndList
#define CY_LIST 288 // height of drop-down window
#define CY_ITEM 18 // default height of listview item
#define COMPLETION_TIMEOUT 250 // in milliseconds
#define MAX_ITEM_COUNT 1000

//////////////////////////////////////////////////////////////////////////////
// CACEditCtrl

// range of WCHAR (inclusive)
struct RANGE
{
    WCHAR from, to;
};

// a callback function for bsearch: comparison of two ranges
static inline int RangeCompare(const void *x, const void *y)
{
    const RANGE *a = reinterpret_cast<const RANGE *>(x);
    const RANGE *b = reinterpret_cast<const RANGE *>(y);
    if (a->to < b->from)
        return -1;
    if (b->to < a->from)
        return 1;
    return 0;
}

// @implemented
// is the WCHAR a word break?
static inline BOOL IsWordBreak(WCHAR ch)
{
    // the ranges of word break characters
    static const RANGE s_ranges[] =
    {
        { 0x0009, 0x0009 }, { 0x0020, 0x002f }, { 0x003a, 0x0040 }, { 0x005b, 0x0060 },
        { 0x007b, 0x007e }, { 0x00ab, 0x00ab }, { 0x00ad, 0x00ad }, { 0x00bb, 0x00bb },
        { 0x02c7, 0x02c7 }, { 0x02c9, 0x02c9 }, { 0x055d, 0x055d }, { 0x060c, 0x060c },
        { 0x2002, 0x200b }, { 0x2013, 0x2014 }, { 0x2016, 0x2016 }, { 0x2018, 0x2018 },
        { 0x201c, 0x201d }, { 0x2022, 0x2022 }, { 0x2025, 0x2027 }, { 0x2039, 0x203a },
        { 0x2045, 0x2046 }, { 0x207d, 0x207e }, { 0x208d, 0x208e }, { 0x226a, 0x226b },
        { 0x2574, 0x2574 }, { 0x3001, 0x3003 }, { 0x3005, 0x3005 }, { 0x3008, 0x3011 },
        { 0x3014, 0x301b }, { 0x301d, 0x301e }, { 0x3041, 0x3041 }, { 0x3043, 0x3043 },
        { 0x3045, 0x3045 }, { 0x3047, 0x3047 }, { 0x3049, 0x3049 }, { 0x3063, 0x3063 },
        { 0x3083, 0x3083 }, { 0x3085, 0x3085 }, { 0x3087, 0x3087 }, { 0x308e, 0x308e },
        { 0x309b, 0x309e }, { 0x30a1, 0x30a1 }, { 0x30a3, 0x30a3 }, { 0x30a5, 0x30a5 },
        { 0x30a7, 0x30a7 }, { 0x30a9, 0x30a9 }, { 0x30c3, 0x30c3 }, { 0x30e3, 0x30e3 },
        { 0x30e5, 0x30e5 }, { 0x30e7, 0x30e7 }, { 0x30ee, 0x30ee }, { 0x30f5, 0x30f6 },
        { 0x30fc, 0x30fe }, { 0xfd3e, 0xfd3f }, { 0xfe30, 0xfe31 }, { 0xfe33, 0xfe44 },
        { 0xfe4f, 0xfe51 }, { 0xfe59, 0xfe5e }, { 0xff08, 0xff09 }, { 0xff0c, 0xff0c },
        { 0xff0e, 0xff0e }, { 0xff1c, 0xff1c }, { 0xff1e, 0xff1e }, { 0xff3b, 0xff3b },
        { 0xff3d, 0xff3d }, { 0xff40, 0xff40 }, { 0xff5b, 0xff5e }, { 0xff61, 0xff64 },
        { 0xff67, 0xff70 }, { 0xff9e, 0xff9f }, { 0xffe9, 0xffe9 }, { 0xffeb, 0xffeb },
    };
    RANGE range = { ch, ch };
    return !!bsearch(&range, s_ranges, _countof(s_ranges), sizeof(RANGE), RangeCompare);
}

// @implemented
// This function is an application-defined callback function.
// https://docs.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-editwordbreakprocw
static INT CALLBACK
EditWordBreakProcW(LPWSTR lpch, INT index, INT count, INT code)
{
    switch (code)
    {
        case WB_ISDELIMITER:
            return IsWordBreak(lpch[index]);

        case WB_LEFT:
            if (index)
                --index;
            while (index && !IsWordBreak(lpch[index]))
                --index;
            return index;

        case WB_RIGHT:
            if (!count)
                break;
            while (index < count && lpch[index] && !IsWordBreak(lpch[index]))
                ++index;
            return index;

        default:
            break;
    }
    return 0;
}

// @implemented
CACEditCtrl::CACEditCtrl() : m_pDropDown(NULL), m_fnOldWordBreakProc(NULL)
{
}

// @implemented
VOID CACEditCtrl::HookWordBreakProc(BOOL bHook)
{
    if (bHook)
    {
        m_fnOldWordBreakProc = reinterpret_cast<EDITWORDBREAKPROCW>(
            SendMessageW(EM_SETWORDBREAKPROC, 0,
                reinterpret_cast<LPARAM>(EditWordBreakProcW)));
    }
    else
    {
        SendMessageW(EM_SETWORDBREAKPROC, 0,
                     reinterpret_cast<LPARAM>(m_fnOldWordBreakProc));
    }
}

// WM_CHAR
// This message is posted to the window with the keyboard focus when WM_KEYDOWN is translated.
LRESULT CACEditCtrl::OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("CACEditCtrl::OnChar(%p)\n", this);
    ATLASSERT(m_pDropDown);

    if (m_pDropDown->OnEditChar(wParam, lParam))
        return 0; // eat

    return DefWindowProcW(uMsg, wParam, lParam); // do default
}

// WM_CLEAR @implemented
// An application sends this message to an edit control to delete the current selection.
LRESULT CACEditCtrl::OnClear(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("CACEditCtrl::OnClear(%p)\n", this);
    ATLASSERT(m_pDropDown);
    LRESULT ret = DefWindowProcW(uMsg, wParam, lParam); // do default
    m_pDropDown->OnEditUpdate(TRUE);
    return ret;
}

// WM_CUT @implemented
// An application sends this message to an edit control to cut the current selection.
LRESULT CACEditCtrl::OnCut(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("CACEditCtrl::OnCut(%p)\n", this);
    ATLASSERT(m_pDropDown);
    LRESULT ret = DefWindowProcW(uMsg, wParam, lParam); // do default
    m_pDropDown->OnEditUpdate(TRUE);
    return ret;
}

// WM_DESTROY
// This message is sent when a window is being destroyed.
LRESULT CACEditCtrl::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("CACEditCtrl::OnDestroy(%p)\n", this);
    ATLASSERT(m_pDropDown);
    CAutoComplete *pDropDown = m_pDropDown;

    // unhook word break procedure
    HookWordBreakProc(FALSE);

    // unsubclass
    HWND hwndEdit = UnsubclassWindow();

    // close the drop-down window
    if (pDropDown)
    {
        pDropDown->PostMessageW(WM_CLOSE, 0, 0);
    }

    return ::DefWindowProcW(hwndEdit, uMsg, wParam, lParam); // do default
}

// WM_GETDLGCODE
// By responding to this message, an application can take control of a particular type of
// input and process the input itself.
LRESULT CACEditCtrl::OnGetDlgCode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("CACEditCtrl::OnGetDlgCode(%p)\n", this);
    ATLASSERT(m_pDropDown);

    LRESULT ret = DefWindowProcW(uMsg, wParam, lParam); // get default

    if (m_pDropDown->IsWindowVisible())
        ret |= DLGC_WANTALLKEYS; // we want all keys to manipulate the list

    return ret;
}

// WM_KEYDOWN
// This message is posted to the window with the keyboard focus when a non-system key is pressed.
LRESULT CACEditCtrl::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("CACEditCtrl::OnKeyDown(%p)\n", this);
    ATLASSERT(m_pDropDown);

    // is suggestion available?
    if (!m_pDropDown || !m_pDropDown->CanAutoSuggest() || !m_pDropDown->IsWindowVisible())
    {
        // if not so, then do default
        return DefWindowProcW(uMsg, wParam, lParam);
    }

    if (m_pDropDown->OnEditKeyDown(wParam, lParam))
        return 0; // eat

    return DefWindowProcW(uMsg, wParam, lParam); // do default
}

// WM_KILLFOCUS @implemented
// This message is sent to a window immediately before it loses the keyboard focus.
LRESULT CACEditCtrl::OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("CACEditCtrl::OnKillFocus(%p)\n", this);
    ATLASSERT(m_pDropDown);

    // hide the list if lost focus
    HWND hwndGotFocus = (HWND)wParam;
    if (hwndGotFocus != m_hWnd && hwndGotFocus != m_pDropDown->m_hWnd)
    {
        m_pDropDown->HideDropDown();
    }

    return DefWindowProcW(uMsg, wParam, lParam); // do default
}

// WM_PASTE @implemented
// An application sends this message to an edit control to paste at the current caret position.
LRESULT CACEditCtrl::OnPaste(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("CACEditCtrl::OnPaste(%p)\n", this);
    ATLASSERT(m_pDropDown);

    LRESULT ret = DefWindowProcW(uMsg, wParam, lParam); // do default
    m_pDropDown->OnEditUpdate(TRUE);
    return ret;
}

// WM_SETFOCUS
// This message is sent to a window after it has gained the keyboard focus.
LRESULT CACEditCtrl::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("CACEditCtrl::OnSetFocus(%p)\n", this);
    ATLASSERT(m_pDropDown);
    return DefWindowProcW(uMsg, wParam, lParam); // do default
}

// WM_SETTEXT
// An application sends this message to set the text of a window.
LRESULT CACEditCtrl::OnSetText(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("CACEditCtrl::OnSetText(%p)\n", this);
    ATLASSERT(m_pDropDown);

    if (!m_pDropDown->m_bInSetText)
    {
        // it's mechanical WM_SETTEXT
        m_pDropDown->HideDropDown();
    }

    return DefWindowProcW(uMsg, wParam, lParam); // do default
}

//////////////////////////////////////////////////////////////////////////////
// CACListView

// @implemented
CACListView::CACListView() : m_pDropDown(NULL), m_cyItem(CY_ITEM)
{
}

HWND CACListView::Create(HWND hwndParent)
{
    ATLASSERT(m_hWnd == NULL);

    LPCWSTR text = L"Internet Explorer";
    DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | LVS_NOCOLUMNHEADER |
                    LVS_OWNERDATA | LVS_OWNERDRAWFIXED | LVS_SINGLESEL | LVS_REPORT;
    m_hWnd = ::CreateWindowExW(0, GetWndClassName(), text, dwStyle,
                               0, 0, 0, 0, hwndParent, NULL,
                               _AtlBaseModule.GetModuleInstance(), NULL);
    return m_hWnd;
}

// @implemented
INT CACListView::GetItemCount()
{
    ATLASSERT(m_pDropDown);
    ATLASSERT(GetStyle() & LVS_OWNERDATA);
    return CListView::GetItemCount();
}

// @implemented
CStringW CACListView::GetItemText(INT iItem)
{
    // NOTE: LVS_OWNERDATA doesn't support LVM_GETITEMTEXT.
    ATLASSERT(m_pDropDown);
    ATLASSERT(GetStyle() & LVS_OWNERDATA);
    return m_pDropDown->GetItemText(iItem);
}

// @implemented
INT CACListView::ItemFromPoint(INT x, INT y)
{
    LV_HITTESTINFO hittest;
    hittest.pt.x = x;
    hittest.pt.y = y;
    return HitTest(&hittest);
}

// @implemented
INT CACListView::GetCurSel()
{
    return GetNextItem(-1, LVNI_ALL | LVNI_SELECTED);
}

// @implemented
VOID CACListView::SetCurSel(INT iItem)
{
    if (iItem == -1)
        SetItemState(-1, 0, LVIS_SELECTED); // select none
    else
        SetItemState(iItem, LVIS_SELECTED, LVIS_SELECTED);
}

// @implemented
VOID CACListView::SelectHere(INT x, INT y)
{
    INT iItem = ItemFromPoint(x, y);
    SetCurSel(iItem);
}

// WM_LBUTTONDBLCLK @implemented
// This message is posted when the user double-clicks while the cursor is inside.
LRESULT CACListView::OnLButtonDblClk(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("CACListView::OnLButtonDblClk(%p)\n", this);
    // avoid the default processing that will set focus
    ATLASSERT(m_pDropDown);
    INT iItem = ItemFromPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    if (iItem != -1)
    {
        m_pDropDown->SelectItem(iItem);
        m_pDropDown->HideDropDown();
    }
    return 0;
}

// WM_LBUTTONDOWN @implemented
// This message is posted when the user pressed the left mouse button while the cursor is inside.
LRESULT CACListView::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("CACListView::OnLButtonDown(%p)\n", this);
    // avoid the default processing that will set focus
    ATLASSERT(m_pDropDown);
    SelectHere(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
// CACScrollBar

CACScrollBar::CACScrollBar() : m_pDropDown(NULL)
{
}

HWND CACScrollBar::Create(HWND hwndParent)
{
    ATLASSERT(m_hWnd == NULL);

    DWORD dwStyle = WS_CHILD | WS_VISIBLE | SBS_BOTTOMALIGN | SBS_VERT;
    m_hWnd = ::CreateWindowExW(0, GetWndClassName(), NULL, dwStyle,
                               0, 0, 0, 0, hwndParent, NULL,
                               _AtlBaseModule.GetModuleInstance(), NULL);
    return m_hWnd;
}

//////////////////////////////////////////////////////////////////////////////
// CACSizeBox

CACSizeBox::CACSizeBox() : m_pDropDown(NULL), m_bDowner(TRUE)
{
}

HWND CACSizeBox::Create(HWND hwndParent)
{
    ATLASSERT(m_hWnd == NULL);

    DWORD dwStyle = WS_CHILD | WS_VISIBLE | SBS_SIZEBOX;
    m_hWnd = ::CreateWindowExW(0, GetWndClassName(), NULL, dwStyle,
                               0, 0, 0, 0, hwndParent, NULL,
                               _AtlBaseModule.GetModuleInstance(), NULL);
    return m_hWnd;
}

VOID CACSizeBox::SetDowner(BOOL bDowner)
{
    m_bDowner = bDowner;
}

//////////////////////////////////////////////////////////////////////////////
// CAutoComplete public methods

// @implemented
CAutoComplete::CAutoComplete()
    : m_bInSetText(FALSE)
    , m_bInSelectItem(FALSE)
    , m_pLayout(NULL)
    , m_bDowner(TRUE)
    , m_dwOptions(ACO_AUTOAPPEND | ACO_AUTOSUGGEST)
    , m_bEnabled(TRUE) // enabled by default
    , m_hwndCombo(NULL)
    , m_bShowScroll(FALSE)
    , m_hFont(NULL)
{
}

// @implemented
CAutoComplete::~CAutoComplete()
{
    if (m_pLayout)
    {
        ::LayoutDestroy(m_pLayout); // see "layout.h"
        m_pLayout = NULL;
    }
    if (m_hFont)
    {
        ::DeleteObject(m_hFont);
        m_hFont = NULL;
    }
}

HWND CAutoComplete::CreateDropDown()
{
    ATLASSERT(m_hWnd == NULL);

    DWORD dwStyle = WS_POPUP | /*WS_VISIBLE |*/ WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_BORDER;
    DWORD dwExStyle = WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOPARENTNOTIFY;
    CRect rc(0, 0, 100, 100);

    return Create(NULL, &rc, NULL, dwStyle, dwExStyle);
}

// @implemented
BOOL CAutoComplete::CanAutoSuggest()
{
    return !!(m_dwOptions & ACO_AUTOSUGGEST);
}

// @implemented
BOOL CAutoComplete::CanAutoAppend()
{
    return !!(m_dwOptions & ACO_AUTOAPPEND);
}

// @implemented
BOOL CAutoComplete::IsComboBoxDropped()
{
    if (!::IsWindow(m_hwndCombo))
        return FALSE;
    return (BOOL)::SendMessageW(m_hwndCombo, CB_GETDROPPEDSTATE, 0, 0);
}

// @implemented
INT CAutoComplete::GetItemCount()
{
    return m_outerList.GetSize();
}

// @implemented
CStringW CAutoComplete::GetItemText(INT iItem)
{
    return ((iItem < m_outerList.GetSize()) ? m_outerList[iItem] : L"");
}

CStringW CAutoComplete::GetEditText()
{
    BSTR bstrText = NULL;
    CStringW strText;
    if (m_hwndEdit.GetWindowTextW(bstrText))
    {
        strText = bstrText;
        ::SysFreeString(bstrText);
    }
    return strText;
}

VOID CAutoComplete::SetEditText(LPCWSTR pszText)
{
    m_bInSetText = TRUE;
    m_hwndEdit.SetWindowTextW(pszText);
    m_bInSetText = FALSE;
}

CStringW CAutoComplete::GetStemText()
{
    CStringW strText = GetEditText();
    INT ich = strText.ReverseFind(L'\\');
    if (ich == -1)
        return L"";
    return strText.Left(ich + 1);
}

// @implemented
VOID CAutoComplete::ShowDropDown()
{
    if (!m_hWnd || !CanAutoSuggest())
        return;

    INT cItems = GetItemCount();
    if (cItems == 0 || ::GetFocus() != m_hwndEdit || IsComboBoxDropped())
    {
        // hide the drop-down if necessary
        HideDropDown();
        return;
    }

    RepositionDropDown();
}

// @implemented
VOID CAutoComplete::HideDropDown()
{
    if (m_hwndCombo)
    {
        ::SendMessageW(m_hwndCombo, CB_SHOWDROPDOWN, FALSE, 0);
        ::InvalidateRect(m_hwndCombo, NULL, TRUE);
    }

    ShowWindow(SW_HIDE);

    m_hwndList.DeleteAllItems();
    m_outerList.RemoveAll();
}

// @implemented
VOID CAutoComplete::SelectItem(INT iItem)
{
    m_hwndList.SetCurSel(iItem);
}

VOID CAutoComplete::DoAutoAppend()
{
    if (!CanAutoSuggest() || m_strText.IsEmpty())
        return;

    INT cItems = m_innerList.GetSize();
    if (cItems == 0)
        return; // don't append

    // get the common string
    CStringW strCommon;
    for (INT iItem = 0; iItem < cItems; ++iItem)
    {
        // get the text of the item
        const CString& strItem = m_innerList[iItem];

        if (iItem == 0) // the first item
        {
            strCommon = strItem; // store the text
            continue;
        }

        for (INT ich = 0; ich < strCommon.GetLength(); ++ich)
        {
            if (ich < strItem.GetLength() && strCommon[ich] != strItem[ich])
            {
                strCommon = strCommon.Left(ich); // shrink the common string
                break;
            }
        }
    }

    CStringW strText = GetEditText(); // get the text

    if (strCommon.IsEmpty() || strCommon.GetLength() <= strText.GetLength())
        return; // no suggestion

    // append suggestion
    INT cchAppend = strCommon.GetLength() - strText.GetLength();
    CStringW strAppend = strCommon.Right(cchAppend);
    strText += strAppend;
    SetEditText(strText);
    INT ich0 = strText.GetLength();
    INT ich1 = ich0 + cchAppend;
    m_hwndEdit.SendMessageW(EM_SETSEL, ich0, ich1);
}

INT CAutoComplete::UpdateInnerList()
{
    // get text
    CStringW strText = GetEditText();

    BOOL bReset = FALSE, bExpand = FALSE;

    // if previous text was empty
    if (m_strText.IsEmpty())
    {
        bReset = TRUE;
    }
    // save text
    m_strText = strText;

    // do expand the items if the stem is changed
    CStringW strStemText = GetStemText();
    if (m_strStemText != strStemText)
    {
        m_strStemText = strStemText;
        bExpand = bReset = TRUE;
    }

    // reset if necessary
    if (bReset)
    {
        HRESULT hr = m_pEnum->Reset(); // IEnumString::Reset
        TRACE("m_pEnum->Reset(%p): 0x%08lx\n",
              static_cast<IUnknown *>(m_pEnum), hr);
    }

    // update ac list if necessary
    if (bExpand)
    {
        HRESULT hr = m_pACList->Expand(strStemText); // IACList::Expand
        TRACE("m_pACList->Expand(%p, %S): 0x%08lx\n",
              static_cast<IUnknown *>(m_pACList),
              static_cast<LPCWSTR>(strStemText), hr);
    }

    if (bExpand || m_innerList.GetSize() == 0)
    {
        // reload the inner list
        ReLoadInnerList();
    }

    return m_innerList.GetSize();
}

INT CAutoComplete::UpdateOuterList()
{
    // get the text info
    CString strText = GetEditText();
    INT cchText = strText.GetLength();

    // update the outer list from the inner list
    m_outerList.RemoveAll();
    for (INT iItem = 0; iItem < m_innerList.GetSize(); ++iItem)
    {
        const CStringW& strTarget = m_innerList[iItem];
        if (::StrCmpNI(strTarget, strText, cchText) == 0)
        {
            m_outerList.Add(strTarget);
        }
    }

    // set the item count of the listview
    INT cItems = m_outerList.GetSize();
    m_hwndList.SendMessageW(LVM_SETITEMCOUNT, cItems, 0);

    // delete listview items
    m_hwndList.DeleteAllItems();

    // insert listview items
    if (cchText > 0 && m_outerList.GetSize() > 0)
    {
        // insert items
        LV_ITEMW item = { LVIF_TEXT };
        item.pszText = LPSTR_TEXTCALLBACK;
        for (INT iItem = 0; iItem < cItems; ++iItem)
        {
            item.iItem = iItem;
            m_hwndList.InsertItem(&item);
        }

        ATLASSERT(m_hwndList.GetItemCount() > 0);
    }
    else
    {
        cItems = 0;
    }

    return cItems;
}

VOID CAutoComplete::UpdateCompletion(BOOL bAppendOK)
{
    UINT cItems = UpdateInnerList();
    if (cItems == 0) // no items
    {
        HideDropDown();
        return;
    }

    if (CanAutoSuggest())
    {
        m_bInSelectItem = TRUE;
        SelectItem(-1); // select none
        m_bInSelectItem = FALSE;

        if (!UpdateOuterList())
            HideDropDown();
        else
            RepositionDropDown();
        return;
    }

    if (CanAutoAppend() && bAppendOK)
    {
        DoAutoAppend();
        return;
    }
}

BOOL CAutoComplete::OnEditKeyDown(WPARAM wParam, LPARAM lParam)
{
    if (!CanAutoSuggest())
        return FALSE; // default

    UINT vk = (UINT)wParam; // virtual key
    switch (vk)
    {
        case VK_UP: case VK_DOWN: // [Arrow Up]/[Arrow Down] key
        case VK_PRIOR: case VK_NEXT: // [PageUp]/[PageDown] key
            return OnListUpDown(vk);

        case VK_ESCAPE: case VK_TAB: // [Esc]/[Tab] key
        {
            SetEditText(m_strText); // revert
            INT cch = m_strText.GetLength();
            m_hwndEdit.SendMessageW(EM_SETSEL, cch, cch);
            HideDropDown(); // hide
            return TRUE; // non-default
        }

        case VK_RETURN: // [Enter] key
            if (::GetKeyState(VK_CONTROL) < 0) // [Ctrl] key
            {
                // quick edit
                CStringW strText = GetEditText();
                SetEditText(GetQuickEdit(strText));
            }
            else
            {
                // select all
                INT cch = m_hwndEdit.GetWindowTextLengthW();
                m_hwndEdit.SendMessageW(EM_SETSEL, 0, cch);
            }
            HideDropDown(); // hide
            return FALSE; // default

        case VK_DELETE: // [Del] key
            OnEditUpdate(FALSE);
            return FALSE; // default

        default:
        {
            return FALSE; // default
        }
    }
}

BOOL CAutoComplete::OnEditChar(WPARAM wParam, LPARAM lParam)
{
    if (!CanAutoSuggest() && !CanAutoAppend())
        return FALSE; // default

    m_hwndEdit.DefWindowProcW(WM_CHAR, wParam, lParam);
    OnEditUpdate(wParam != VK_BACK && wParam != VK_DELETE);
    return TRUE; // non-default
}

VOID CAutoComplete::OnEditUpdate(BOOL bAppendOK)
{
    CString strText = GetEditText();

    if (::StrCmpIW(m_strText, strText) == 0)
    {
        // no change
        if (CanAutoSuggest() && !IsWindowVisible())
            ShowDropDown();
        return;
    }

    UpdateCompletion(bAppendOK);
}

VOID CAutoComplete::OnListSelChange()
{
    CStringW text;
    INT iItem = m_hwndList.GetCurSel();
    if (iItem != -1)
    {
        text = GetItemText(iItem);
    }
    else
    {
        text = m_strText;
    }
    SetEditText(text);

    INT cch = text.GetLength();
    m_hwndEdit.SendMessageW(EM_SETSEL, cch, cch);
}

BOOL CAutoComplete::OnListUpDown(UINT vk)
{
    if (!CanAutoSuggest())
        return FALSE; // default

    if ((m_dwOptions & ACO_UPDOWNKEYDROPSLIST) && !IsWindowVisible())
    {
        ShowDropDown();
        return TRUE; // non-default
    }
    
    INT iItem = m_hwndList.GetCurSel();
    INT cItems = m_hwndList.GetItemCount();
    switch (vk)
    {
        case VK_UP: // [Arrow Up]
            if (iItem == -1)
                iItem = cItems - 1;
            else if (iItem == 0)
                iItem = -1;
            else
                --iItem;
            m_hwndList.SetCurSel(iItem);
            break;
        case VK_DOWN: // [Arrow Down]
            if (iItem == -1)
                iItem = 0;
            else if (iItem == cItems - 1)
                iItem = -1;
            else
                ++iItem;
            m_hwndList.SetCurSel(iItem);
            break;
        case VK_PRIOR: // [PageUp]
            FIXME("VK_PRIOR\n");
            break;
        case VK_NEXT: // [PageDown]
            FIXME("VK_NEXT\n");
            break;
        default:
        {
            ATLASSERT(FALSE);
            break;
        }
    }

    return TRUE; // non-default
}

//////////////////////////////////////////////////////////////////////////////
// CAutoComplete IAutoComplete methods

// @implemented
STDMETHODIMP CAutoComplete::Enable(BOOL fEnable)
{
    TRACE("(%p)->(%d)\n", this, fEnable);
    m_bEnabled = fEnable;
    return S_OK;
}

STDMETHODIMP
CAutoComplete::Init(HWND hwndEdit, IUnknown *punkACL,
                    LPCOLESTR pwszRegKeyPath, LPCOLESTR pwszQuickComplete)
{
    TRACE("(%p)->(0x%08lx, %p, %s, %s)\n",
          this, hwndEdit, punkACL, debugstr_w(pwszRegKeyPath), debugstr_w(pwszQuickComplete));

    if (m_dwOptions & ACO_AUTOSUGGEST)
        TRACE(" ACO_AUTOSUGGEST\n");
    if (m_dwOptions & ACO_AUTOAPPEND)
        TRACE(" ACO_AUTOAPPEND\n");
    if (m_dwOptions & ACO_SEARCH)
        FIXME(" ACO_SEARCH not supported\n");
    if (m_dwOptions & ACO_FILTERPREFIXES)
        FIXME(" ACO_FILTERPREFIXES not supported\n");
    if (m_dwOptions & ACO_USETAB)
        FIXME(" ACO_USETAB not supported\n");
    if (m_dwOptions & ACO_UPDOWNKEYDROPSLIST)
        TRACE(" ACO_UPDOWNKEYDROPSLIST\n");
    if (m_dwOptions & ACO_RTLREADING)
        FIXME(" ACO_RTLREADING not supported\n");

    // sanity check
    if (m_hwndEdit || !punkACL)
    {
        ATLASSERT(0);
        return E_INVALIDARG;
    }

    // set this pointer to m_hwndEdit
    m_hwndEdit.m_pDropDown = this;

    // subclass it
    m_hwndEdit.SubclassWindow(hwndEdit);
    // set word break procedure
    m_hwndEdit.HookWordBreakProc(TRUE);

    // get an IEnumString
    ATLASSERT(!m_pEnum);
    punkACL->QueryInterface(IID_IEnumString, (VOID **)&m_pEnum);

    // get an IACList
    ATLASSERT(!m_pACList);
    punkACL->QueryInterface(IID_IACList, (VOID **)&m_pACList);

    // add reference
    AddRef();

    // create/destroy the drop-down window if necessary
    UpdateDropDownState();

    // load quick completion info
    LoadQuickComplete(pwszRegKeyPath, pwszQuickComplete);

    // any combobox for m_hwndEdit?
    m_hwndCombo = NULL;
    HWND hwndParent = ::GetParent(m_hwndEdit);
    WCHAR szClass[16];
    if (::GetClassNameW(hwndParent, szClass, _countof(szClass)))
    {
        if (::StrCmpI(szClass, L"COMBOBOX") == 0 || ::StrCmpI(szClass, WC_COMBOBOXEXW) == 0)
        {
            m_hwndCombo = hwndParent; // get combobox
        }
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
// CAutoComplete IAutoComplete2 methods

// @implemented
STDMETHODIMP CAutoComplete::GetOptions(DWORD *pdwFlag)
{
    TRACE("(%p) -> (%p)\n", this, pdwFlag);
    if (pdwFlag)
    {
        *pdwFlag = m_dwOptions;
        return S_OK;
    }
    return E_INVALIDARG;
}

// @implemented
STDMETHODIMP CAutoComplete::SetOptions(DWORD dwFlag)
{
    TRACE("(%p) -> (0x%x)\n", this, dwFlag);
    m_dwOptions = dwFlag;
    UpdateDropDownState();
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
// CAutoComplete IAutoCompleteDropDown methods

// @implemented
STDMETHODIMP CAutoComplete::GetDropDownStatus(DWORD *pdwFlags, LPWSTR *ppwszString)
{
    BOOL dropped = m_hwndList.IsWindowVisible();

    if (pdwFlags)
        *pdwFlags = (dropped ? ACDD_VISIBLE : 0);

    if (ppwszString)
    {
        *ppwszString = NULL;

        if (dropped)
        {
            // get selected item
            INT iItem = m_hwndList.GetCurSel();
            if (iItem >= 0)
            {
                // get the text of item
                CStringW strText = m_hwndList.GetItemText(iItem);

                // store to *ppwszString
                SHStrDupW(strText, ppwszString);

                if (*ppwszString == NULL)
                    return E_OUTOFMEMORY;
            }
        }
    }

    return S_OK;
}

STDMETHODIMP CAutoComplete::ResetEnumerator()
{
    FIXME("(%p): stub\n", this);

    HideDropDown();

    if (IsWindowVisible())
    {
        OnEditUpdate(FALSE);
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
// CAutoComplete IEnumString methods

// @implemented
STDMETHODIMP CAutoComplete::Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    TRACE("(%p, %d, %p, %p)\n", this, celt, rgelt, pceltFetched);
    if (rgelt)
        *rgelt = NULL;
    if (*pceltFetched)
        *pceltFetched = 0;
    if (!m_pEnum || !rgelt || !pceltFetched || celt != 1)
        return E_INVALIDARG;

    HRESULT hr;
    LPWSTR pszText;
    for (;;)
    {
        ULONG cGot;
        hr = m_pEnum->Next(1, &pszText, &cGot);
        if (hr != S_OK)
            break;
        ::CoTaskMemFree(pszText);
    }

    if (hr == S_OK)
    {
        *rgelt = pszText;
        *pceltFetched = 1;
    }
    return hr;
}

// @implemented
STDMETHODIMP CAutoComplete::Skip(ULONG celt)
{
    TRACE("(%p, %d)\n", this, celt);
    return E_NOTIMPL;
}

// @implemented
STDMETHODIMP CAutoComplete::Reset()
{
    TRACE("(%p)\n", this);
    if (m_pEnum)
        return m_pEnum->Reset();
    return E_FAIL;
}

// @implemented
STDMETHODIMP CAutoComplete::Clone(IEnumString **ppOut)
{
    TRACE("(%p, %p)\n", this, ppOut);
    if (ppOut)
        *ppOut = NULL;
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////
// CAutoComplete protected methods

VOID CAutoComplete::UpdateScrollBar()
{
    if (!m_hwndScrollBar)
        return;

    // copy scroll info from m_hwndList to m_hwndScrollBar
    SCROLLINFO si = { sizeof(si), SIF_ALL };
    m_hwndList.GetScrollInfo(SB_VERT, &si);
    m_hwndScrollBar.SetScrollInfo(SB_CTL, &si, TRUE);

    // show/hide scroll bar
    BOOL bShowScroll = ((UINT)(si.nMax - si.nMin) >= si.nPage);
    if (m_bShowScroll != bShowScroll)
    {
        m_bShowScroll = bShowScroll;
        m_hwndScrollBar.ShowScrollBar(SB_CTL, m_bShowScroll);

        // do re-arrange
        ReArrangeControls(m_bDowner);
    }
}

VOID CAutoComplete::UpdateDropDownState()
{
    if (CanAutoSuggest())
    {
        // create the drop-down window if not existed
        if (!m_hWnd)
        {
            AddRef();
            if (!CreateDropDown())
                Release();
        }
    }
    else
    {
        // hide if existed
        if (m_hWnd)
            ShowWindow(SW_HIDE);
    }
}

// @implemented
BOOL CAutoComplete::ReArrangeControls(BOOL bDowner)
{
    // re-calculate the rectangles
    CRect rcList, rcScrollBar, rcSizeBox;
    ReCalcRects(bDowner, rcList, rcScrollBar, rcSizeBox);

    // show or hide scroll bar
    m_hwndScrollBar.ShowWindow(m_bShowScroll ? SW_SHOWNOACTIVATE : SW_HIDE);

    // move the controls
    m_hwndList.MoveWindow(rcList.left, rcList.top, rcList.right, rcList.bottom);
    m_hwndScrollBar.MoveWindow(rcScrollBar.left, rcScrollBar.top, rcScrollBar.right, rcScrollBar.bottom);
    m_hwndSizeBox.MoveWindow(rcSizeBox.left, rcSizeBox.top, rcSizeBox.right, rcSizeBox.bottom);

    // update layout
    return UpdateLayout(bDowner);
}

// @implemented
VOID CAutoComplete::ReCalcRects(BOOL bDowner, CRect& rcList, CRect& rcScrollBar, CRect& rcSizeBox)
{
    // get the client rectangle
    CRect rc;
    GetClientRect(&rc);

    // copy rectangle rc
    rcList = rcScrollBar = rcSizeBox = rc;

    // the list
    rcList.right = rcList.left + CX_LIST;

    // the scroll bar
    if (m_bShowScroll)
        rcScrollBar.left = rcScrollBar.right - GetSystemMetrics(SM_CXVSCROLL);
    else
        rcScrollBar.left = rcScrollBar.right;

    // the size box
    rcSizeBox.left = rcSizeBox.right - GetSystemMetrics(SM_CXVSCROLL);
    if (!bDowner)
        rcSizeBox.top = 0;
    else
        rcSizeBox.top = rcSizeBox.bottom - GetSystemMetrics(SM_CYHSCROLL);
}

// @implemented
BOOL CAutoComplete::UpdateLayout(BOOL bDowner)
{
    m_bDowner = bDowner;

    // initialize the layout
    LAYOUT_INFO info[] =
    {
        { 0, BF_TOPLEFT | BF_BOTTOMLEFT, m_hwndList },
        { 0, BF_RIGHT, m_hwndScrollBar },
        { 0, (UINT)(bDowner ? BF_BOTTOMRIGHT : BF_TOPLEFT), m_hwndSizeBox },
    };
    LAYOUT_DATA *pOldLayout = m_pLayout;
    m_pLayout = ::LayoutInit(m_hWnd, info, -(INT)_countof(info)); // see "layout.h"
    ::LayoutDestroy(pOldLayout);

    // re-arrange the controls
    PostMessageW(WM_SIZE, 0, 0);

    return m_pLayout != NULL;
}

// @implemented
VOID CAutoComplete::LoadQuickComplete(LPCWSTR pwszRegKeyPath, LPCWSTR pwszQuickComplete)
{
    m_strQuickComplete.Empty();

    if (pwszRegKeyPath)
    {
        CStringW strPath = pwszRegKeyPath;
        INT ichSep = strPath.ReverseFind(L'\\');
        if (ichSep != -1)
        {
            CStringW strKey = strPath.Left(ichSep);
            CStringW strName = strPath.Mid(ichSep + 1);
            WCHAR szValue[MAX_PATH] = L"";
            DWORD cbValue = sizeof(szValue);
            SHRegGetUSValueW(pwszRegKeyPath, strName, NULL,
                             szValue, &cbValue, FALSE, NULL, 0);
            if (szValue[0] != 0 && cbValue != 0)
            {
                m_strQuickComplete = szValue;
            }
        }
    }

    if (pwszQuickComplete && m_strQuickComplete.IsEmpty())
    {
        m_strQuickComplete = pwszQuickComplete;
    }
}

// @implemented
CStringW CAutoComplete::GetQuickEdit(const CStringW& strText)
{
    if (strText.IsEmpty() || m_strQuickComplete.IsEmpty())
        return strText;

    // m_strQuickComplete will be "www.%s.com" etc.
    CStringW ret;
    ret.Format(m_strQuickComplete, static_cast<LPCWSTR>(strText));
    return ret;
}

VOID CAutoComplete::RepositionDropDown()
{
    // get nearest monitor from m_hwndEdit
    HMONITOR hMon = ::MonitorFromWindow(m_hwndEdit, MONITOR_DEFAULTTONEAREST);
    ATLASSERT(hMon != NULL);
    if (hMon == NULL)
        return;

    // get nearest monitor info
    MONITORINFO mi = { sizeof(mi) };
    if (!::GetMonitorInfo(hMon, &mi))
    {
        ATLASSERT(FALSE);
        return;
    }

    // get count and item height
    INT cItems = GetItemCount();
    INT cyItem = m_hwndList.m_cyItem;

    // get m_hwndEdit position
    RECT rcEdit;
    m_hwndEdit.GetWindowRect(&rcEdit);
    INT x = rcEdit.left, y = rcEdit.bottom;

    // get list extent
    RECT rcMon = mi.rcMonitor;
    INT cx = rcEdit.right - rcEdit.left, cy = cItems * cyItem;
    if (cy > CY_LIST)
    {
        cy = CY_LIST;
        m_bShowScroll = TRUE; // to show scroll bar
    }
    else
    {
        m_bShowScroll = FALSE; // to hide scroll bar
    }

    // convert rectangle for frame
    RECT rc = { 0, 0, cx, cy };
    AdjustWindowRectEx(&rc, GetStyle(), FALSE, GetExStyle());
    cx = rc.right - rc.left;
    cy = rc.bottom - rc.top;

    // is the drop-down window a 'downer' or 'upper'?
    // NOTE: 'downer' is below the EDIT control. 'upper' is above the EDIT control.
    m_bDowner = (rcEdit.bottom + cy < rcMon.bottom);
    m_hwndSizeBox.SetDowner(m_bDowner);

    // adjust y if not downer
    if (!m_bDowner)
        y = rcEdit.top - cy;

    // do re-arrange controls
    ReArrangeControls(m_bDowner);

    // move
    MoveWindow(x, y, cx, cy);

    // show without activation
    ShowWindow(SW_SHOWNOACTIVATE);
}

INT CAutoComplete::ReLoadInnerList()
{
    m_innerList.RemoveAll(); // clear contents

    DWORD dwTick = ::GetTickCount(); // used for timeout

    // reload the items
    LPWSTR pszItem;
    ULONG cGot;
    HRESULT hr;
    for (ULONG cTotal = 0; cTotal < MAX_ITEM_COUNT; ++cTotal)
    {
        // get next item
        hr = m_pEnum->Next(1, &pszItem, &cGot);
        //TRACE("m_pEnum->Next(%p): 0x%08lx\n", reinterpret_cast<IUnknown *>(m_pEnum), hr);
        if (hr != S_OK)
            break;

        m_innerList.Add(pszItem); // append item to m_innerList
        ::CoTaskMemFree(pszItem); // free

        // check the timeout
        if (::GetTickCount() - dwTick >= COMPLETION_TIMEOUT)
            break; // too late
    }

    return m_innerList.GetSize();
}

//////////////////////////////////////////////////////////////////////////////
// CAutoComplete message handlers

// WM_CREATE
// This message is sent when the window is about to be created after WM_NCCREATE.
// The return value is -1 (failure) or zero (success).
LRESULT CAutoComplete::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("CAutoComplete::OnCreate(%p)\n", this);

    // set the pointer of CAutoComplete
    m_hwndList.m_pDropDown = this;
    m_hwndScrollBar.m_pDropDown = this;
    m_hwndSizeBox.m_pDropDown = this;

    // get listview item height
    m_hwndList.m_cyItem = CY_ITEM;
    HDC hDC = GetDC();
    if (hDC)
    {
        HGDIOBJ hFontOld = ::SelectObject(hDC, m_hFont);
        TEXTMETRICW tm;
        if (::GetTextMetricsW(hDC, &tm))
        {
            m_hwndList.m_cyItem = (tm.tmHeight * 3) / 2;
        }
        ::SelectObject(hDC, hFontOld);
        ReleaseDC(hDC);
    }

    // create the children
    m_hwndList.Create(m_hWnd);
    if (!m_hwndList)
        return -1; // failure
    m_hwndScrollBar.Create(m_hWnd);
    if (!m_hwndScrollBar)
        return -1; // failure
    m_hwndSizeBox.Create(m_hWnd);
    if (!m_hwndSizeBox)
        return -1; // failure

    // set the list font
    m_hFont = reinterpret_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
    m_hwndList.SendMessageW(WM_SETFONT, (WPARAM)m_hFont, TRUE);

    // set extended listview style
    DWORD exstyle =
        LVS_EX_ONECLICKACTIVATE | LVS_EX_FULLROWSELECT | LVS_EX_TRACKSELECT;
    m_hwndList.SetExtendedListViewStyle(exstyle, exstyle);

    return 0; // success
}

// WM_DESTROY
// This message is sent when a window is being destroyed.
LRESULT CAutoComplete::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("CAutoComplete::OnDestroy(%p)\n", this);

    // unsubclass EDIT control
    if (m_hwndEdit)
    {
        m_hwndEdit.HookWordBreakProc(FALSE);
        m_hwndEdit.UnsubclassWindow();
    }

    // clear CAutoComplete pointers
    m_hwndEdit.m_pDropDown = NULL;
    m_hwndList.m_pDropDown = NULL;
    m_hwndScrollBar.m_pDropDown = NULL;
    m_hwndSizeBox.m_pDropDown = NULL;

    // destroy controls
    m_hwndList.DestroyWindow();
    m_hwndScrollBar.DestroyWindow();
    m_hwndSizeBox.DestroyWindow();

    // clean up
    m_hwndCombo = NULL;
    Release();

    return 0;
}

// WM_DRAWITEM @implemented
// This message is sent to the owner window to draw m_hwndList.
LRESULT CAutoComplete::OnDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    LPDRAWITEMSTRUCT pDraw = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
    ATLASSERT(pDraw != NULL);

    ATLASSERT(m_hwndList.GetStyle() & LVS_OWNERDRAWFIXED);

    // sanity check
    if (pDraw->CtlType != ODT_LISTVIEW || pDraw->hwndItem != m_hwndList)
        return FALSE;

    // item rectangle
    RECT rcItem = pDraw->rcItem;

    // get info
    UINT iItem = pDraw->itemID; // the index of item
    CStringW strItem = m_hwndList.GetItemText(iItem); // get text of item

    // draw background and text
    HDC hDC = pDraw->hDC;
    BOOL bSelected = (pDraw->itemState & ODS_SELECTED);
    if (bSelected)
    {
        ::FillRect(hDC, &rcItem, ::GetSysColorBrush(COLOR_HIGHLIGHT));
        ::SetTextColor(hDC, ::GetSysColor(COLOR_HIGHLIGHTTEXT));
    }
    else
    {
        ::FillRect(hDC, &rcItem, ::GetSysColorBrush(COLOR_WINDOW));
        ::SetTextColor(hDC, ::GetSysColor(COLOR_WINDOWTEXT));
    }
    UINT uDT_ = DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER;
    ::SetBkMode(hDC, TRANSPARENT);
    ::DrawTextW(hDC, strItem, -1, &rcItem, uDT_);
    ::DrawTextW(hDC, L"OK", -1, &rcItem, uDT_);

    return TRUE;
}

// WM_GETMINMAXINFO @implemented
// This message is sent to a window when the size or position of the window is about to change.
LRESULT CAutoComplete::OnGetMinMaxInfo(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    // restrict minimum size
    LPMINMAXINFO pInfo = reinterpret_cast<LPMINMAXINFO>(lParam);
    pInfo->ptMinTrackSize.x = ::GetSystemMetrics(SM_CXVSCROLL);
    pInfo->ptMinTrackSize.y = ::GetSystemMetrics(SM_CYHSCROLL);
    return 0;
}

// WM_MEASUREITEM @implemented
// This message is sent to the owner window to get the item extent of m_hwndList.
LRESULT CAutoComplete::OnMeasureItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    LPMEASUREITEMSTRUCT pMeasure = reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);
    ATLASSERT(pMeasure != NULL);
    if (pMeasure->CtlType != ODT_LISTVIEW)
        return FALSE;

    ATLASSERT(m_hwndList.GetStyle() & LVS_OWNERDRAWFIXED);
    pMeasure->itemHeight = m_hwndList.m_cyItem;
    return TRUE;
}

// WM_MOUSEACTIVATE @implemented
// The return value of this message specifies whether the window should be activated or not.
LRESULT CAutoComplete::OnMouseActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    // don't activate by mouse
    return (LRESULT)MA_NOACTIVATE;
}

// WM_NOTIFY
// This message informs the parent window of a control that an event has occurred.
LRESULT CAutoComplete::OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    LPNMHDR pnmh = reinterpret_cast<LPNMHDR>(lParam);
    ATLASSERT(pnmh != NULL);

    switch (pnmh->code)
    {
        case NM_DBLCLK: // double-clicked
        {
            TRACE("NM_DBLCLK\n");
            HideDropDown();
            break;
        }
        case LVN_GETDISPINFOA:
        {
            TRACE("LVN_GETDISPINFOA\n");
            if (pnmh->hwndFrom != m_hwndList)
                break;

            INT iItem = m_hwndList.GetCurSel();
            if (iItem == -1)
                break;

            CStringW strText = GetItemText(iItem);
            LV_DISPINFOA *pDispInfo = reinterpret_cast<LV_DISPINFOA *>(pnmh);
            LV_ITEMA *pItem = &pDispInfo->item;
            if (pItem->mask & LVIF_TEXT)
            {
                SHUnicodeToAnsi(strText, pItem->pszText, pItem->cchTextMax);
            }
            break;
        }
        case LVN_GETDISPINFOW:
        {
            TRACE("LVN_GETDISPINFOW\n");
            if (pnmh->hwndFrom != m_hwndList)
                break;

            INT iItem = m_hwndList.GetCurSel();
            if (iItem == -1)
                break;

            CStringW strText = GetItemText(iItem);
            LV_DISPINFOW *pDispInfo = reinterpret_cast<LV_DISPINFOW *>(pnmh);
            LV_ITEMW *pItem = &pDispInfo->item;
            if (pItem->mask & LVIF_TEXT)
                StringCbCopyW(pItem->pszText, pItem->cchTextMax, strText);
            break;
        }
        case LVN_HOTTRACK: // enabled by LVS_EX_TRACKSELECT
        {
            TRACE("LVN_HOTTRACK\n");
            LPNMLISTVIEW pListView = reinterpret_cast<LPNMLISTVIEW>(pnmh);
            INT iItem = pListView->iItem;
            TRACE("LVN_HOTTRACK: iItem:%d\n", iItem);
            m_hwndList.SetCurSel(iItem);
            break;
        }
        case LVN_ITEMACTIVATE: // enabled by LVS_EX_ONECLICKACTIVATE
        {
            TRACE("LVN_ITEMACTIVATE\n");
            LPNMITEMACTIVATE pItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);
            INT iItem = pItemActivate->iItem;
            TRACE("LVN_ITEMACTIVATE: iItem:%d\n", iItem);
            if (pItemActivate->uChanged & LVIF_STATE)
            {
                m_hwndList.SetCurSel(iItem);
            }
            break;
        }
        case LVN_ITEMCHANGED:
        {
            TRACE("LVN_ITEMCHANGED\n");
            LPNMLISTVIEW pListView = reinterpret_cast<LPNMLISTVIEW>(pnmh);
            if (pListView->uChanged & LVIF_STATE)
            {
                if (pListView->uNewState & LVIS_SELECTED)
                {
                    if (!m_bInSelectItem)
                    {
                        // listview selection changed
                        OnListSelChange();
                    }
                }
            }
            break;
        }
    }

    return 0;
}

// WM_NCHITTEST @implemented
// The return value is indicating the cursor shape and the behaviour.
LRESULT CAutoComplete::OnNCHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("CAutoComplete::OnNCHitTest(%p)\n", this);
    RECT rc;
    if (m_hwndSizeBox.GetWindowRect(&rc))
    {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        if (::PtInRect(&rc, pt))
        {
            // resize if the point in the m_hwndSizeBox
            return m_bDowner ? HTBOTTOMRIGHT : HTTOPRIGHT;
        }
    }
    return DefWindowProcW(uMsg, wParam, lParam); // do default
}

// WM_SIZE @implemented
// This message is sent when the window size is changed.
LRESULT CAutoComplete::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("CAutoComplete::OnSize(%p)\n", this);
    ::LayoutUpdate(m_hWnd, m_pLayout, NULL, 0); // see "layout.h"
    UpdateScrollBar();
    m_hwndList.InvalidateRect(NULL, FALSE);
    return 0;
}

// WM_VSCROLL
// This message is sent when a scroll event occurs.
LRESULT CAutoComplete::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("CAutoComplete::OnVScroll(%p)\n", this);
    m_hwndList.SendMessageW(WM_VSCROLL, wParam, lParam);
    return 0;
}
