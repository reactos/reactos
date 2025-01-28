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

#include "precomp.h"
#include <imm.h> // For IMN_OPENCANDIDATE
#include <process.h> // _beginthreadex

/*
  TODO:
  - implement ACO_SEARCH style
  - implement ACO_FILTERPREFIXES style
  - implement ACO_RTLREADING style
 */

#define CX_LIST 30160 // width of m_hwndList (very wide but alright)
#define CY_LIST 288 // maximum height of drop-down window
#define CY_ITEM 18 // default height of listview item
#define MAX_ITEM_COUNT 1000 // the maximum number of items
#define WATCH_TIMER_ID 0xFEEDBEEF // timer ID to watch m_rcEdit
#define WATCH_INTERVAL 300 // in milliseconds

static HHOOK s_hMouseHook = NULL; // hook handle
static HWND s_hWatchWnd = NULL; // the window handle to watch

struct PREFIX_INFO
{
    LPCWSTR psz;
    INT cch;
};
static const PREFIX_INFO s_prefixes[] =
{
    { L"https://", 8 },
    { L"http://www.", 11 },
    { L"http://", 7 },
    { L"www.", 4 },
};

static BOOL DropPrefix(const CStringW& str, CStringW& strBody)
{
    for (size_t iPrefix = 0; iPrefix < _countof(s_prefixes); ++iPrefix)
    {
        LPCWSTR psz = s_prefixes[iPrefix].psz;
        INT cch = s_prefixes[iPrefix].cch;
        if (::StrCmpNIW(str, psz, cch) == 0)
        {
            strBody = str.Mid(cch);
            return TRUE;
        }
    }
    strBody = str;
    return FALSE;
}

static BOOL DoesMatch(const CStringW& strTarget, const CStringW& strText)
{
    CStringW strBody;
    if (DropPrefix(strTarget, strBody))
    {
        if (::StrCmpNIW(strBody, strText, strText.GetLength()) == 0)
            return TRUE;
    }
    else if (::StrCmpNIW(strTarget, strText, strText.GetLength()) == 0)
    {
        return TRUE;
    }
    return FALSE;
}

// mouse hook procedure to watch the mouse click
// https://learn.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ms644988(v=vs.85)
static LRESULT CALLBACK MouseProc(INT nCode, WPARAM wParam, LPARAM lParam)
{
    if (s_hMouseHook == NULL)
        return 0; // do default
    // if the user clicked the outside of s_hWatchWnd, then hide the drop-down window
    if (nCode == HC_ACTION && // an action?
        s_hWatchWnd && ::IsWindow(s_hWatchWnd) && // s_hWatchWnd is valid?
        ::GetCapture() == NULL) // no capture? (dragging something?)
    {
        RECT rc;
        MOUSEHOOKSTRUCT *pMouseHook = reinterpret_cast<MOUSEHOOKSTRUCT *>(lParam);
        switch (wParam)
        {
            case WM_LBUTTONDOWN: case WM_LBUTTONUP:
            case WM_RBUTTONDOWN: case WM_RBUTTONUP:
            case WM_MBUTTONDOWN: case WM_MBUTTONUP:
            case WM_NCLBUTTONDOWN: case WM_NCLBUTTONUP:
            case WM_NCRBUTTONDOWN: case WM_NCRBUTTONUP:
            case WM_NCMBUTTONDOWN: case WM_NCMBUTTONUP:
            {
                ::GetWindowRect(s_hWatchWnd, &rc);
                if (!::PtInRect(&rc, pMouseHook->pt)) // outside of s_hWatchWnd?
                {
                    ::ShowWindowAsync(s_hWatchWnd, SW_HIDE); // hide it
                }
                break;
            }
        }
    }
    return ::CallNextHookEx(s_hMouseHook, nCode, wParam, lParam); // go next hook
}

//////////////////////////////////////////////////////////////////////////////
// sorting algorithm
// https://web.archive.org/web/20210228194050/http://www.ics.kagoshima-u.ac.jp/~fuchida/edu/algorithm/sort-algorithm/

typedef CSimpleArray<CStringW> list_t;

static inline INT compare1(const CStringW& str1, const CStringW& str2)
{
    CStringW s1, s2;
    DropPrefix(str1, s1);
    DropPrefix(str2, s2);
    return s1.CompareNoCase(s2);
}

static inline INT pivot(list_t& list, INT i, INT j)
{
    INT k = i + 1;
    while (k <= j && compare1(list[i], list[k]) == 0)
        k++;
    if (k > j)
        return -1;
    if (compare1(list[i], list[k]) >= 0)
        return i;
    return k;
}

static inline INT partition(list_t& list, INT i, INT j, const CStringW& x)
{
    INT left = i, right = j;
    while (left <= right)
    {
        while (left <= j && compare1(list[left], x) < 0)
            left++;
        while (right >= i && compare1(list[right], x) >= 0)
            right--;
        if (left > right)
            break;

        CStringW tmp = list[left];
        list[left] = list[right];
        list[right] = tmp;

        left++;
        right--;
    }
    return left;
}

static void quicksort(list_t& list, INT i, INT j)
{
    if (i == j)
        return;
    INT p = pivot(list, i, j);
    if (p == -1)
        return;
    INT k = partition(list, i, j, list[p]);
    quicksort(list, i, k - 1);
    quicksort(list, k, j);
}

static inline void DoSort(list_t& list)
{
    if (list.GetSize() <= 1) // sanity check
        return;
    quicksort(list, 0, list.GetSize() - 1); // quick sort
}

// std::unique
static INT DoUnique(list_t& list)
{
    INT first = 0, last = list.GetSize();
    if (first == last)
        return last;
    INT result = first;
    while (++first != last)
    {
        if (compare1(list[result], list[first]) != 0)
            list[++result] = list[first];
    }
    return ++result;
}

static inline void DoUniqueAndTrim(list_t& list)
{
    INT last = DoUnique(list);
    while (list.GetSize() > last)
    {
        list.RemoveAt(last);
    }
}

//////////////////////////////////////////////////////////////////////////////

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
    // binary search
    RANGE range = { ch, ch };
    return !!bsearch(&range, s_ranges, _countof(s_ranges), sizeof(RANGE), RangeCompare);
}

// This function is an application-defined callback function.
// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-editwordbreakprocw
static INT CALLBACK
EditWordBreakProcW(LPWSTR lpch, INT index, INT count, INT code)
{
    switch (code)
    {
        case WB_ISDELIMITER:
            return IsWordBreak(lpch[index]);
        case WB_LEFT:
        {
            if (index)
                --index;
            while (index && !IsWordBreak(lpch[index]))
                --index;
            return index;
        }
        case WB_RIGHT:
        {
            if (!count)
                break;
            while (index < count && lpch[index] && !IsWordBreak(lpch[index]))
                ++index;
            return index;
        }
    }
    return 0;
}

static LRESULT CALLBACK
EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                 UINT_PTR uSubclassID, DWORD_PTR dwData)
{
    CAutoComplete *pThis = reinterpret_cast<CAutoComplete *>(dwData);
    return pThis->EditWndProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CAutoComplete::EditWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret;
    HWND hwndGotFocus;
    switch (uMsg)
    {
        case WM_CHAR:
            return OnEditChar(wParam, lParam);
        case WM_CUT: case WM_PASTE: case WM_CLEAR:
            ret = ::DefSubclassProc(hwnd, uMsg, wParam, lParam); // do default
            StartCompletion(TRUE);
            return ret;
        case WM_GETDLGCODE:
            ret = ::DefSubclassProc(hwnd, uMsg, wParam, lParam); // do default
            // some special keys need default processing. we handle them here
            switch (wParam)
            {
                case VK_RETURN:
                    if (IsWindowVisible() || ::GetKeyState(VK_CONTROL) < 0)
                        OnEditKeyDown(VK_RETURN, 0);
                    break;
                case VK_TAB:
                    if (IsWindowVisible() && UseTab())
                        ret |= DLGC_WANTALLKEYS; // we want all keys to manipulate the list
                    break;
                case VK_ESCAPE:
                    if (IsWindowVisible())
                        ret |= DLGC_WANTALLKEYS; // we want all keys to manipulate the list
                    break;
                default:
                {
                    ret |= DLGC_WANTALLKEYS; // we want all keys to manipulate the list
                    break;
                }
            }
            return ret;
        case WM_KEYDOWN:
            if (OnEditKeyDown(wParam, lParam))
                return TRUE; // eat
            break;
        case WM_SETFOCUS:
            m_bEditHasFocus = TRUE;
            break;
        case WM_KILLFOCUS:
            // hide the list if lost focus
            hwndGotFocus = (HWND)wParam;
            if (hwndGotFocus != m_hwndEdit && hwndGotFocus != m_hWnd)
                HideDropDown();
            m_bEditHasFocus = FALSE;
            break;
        case WM_IME_NOTIFY:
            if (wParam == IMN_OPENCANDIDATE)
                HideDropDown();
            break;
        case WM_SETTEXT:
            if (!m_bInSetText)
                HideDropDown(); // it's mechanical WM_SETTEXT
            break;
        case WM_DESTROY:
        {
            if (m_fnOldWordBreakProc)
            {
                ::SendMessageW(hwnd, EM_SETWORDBREAKPROC, 0, reinterpret_cast<LPARAM>(m_fnOldWordBreakProc));
                m_fnOldWordBreakProc = NULL;
            }
            ::RemoveWindowSubclass(hwnd, EditSubclassProc, 0);
            if (::IsWindow(m_hWnd))
                PostMessageW(WM_CLOSE, 0, 0);
            // remove reference to m_hwndEdit
            Release();
            break;
        }
    }

    return ::DefSubclassProc(hwnd, uMsg, wParam, lParam); // do default
}

//////////////////////////////////////////////////////////////////////////////
// CACListView

CACListView::CACListView() : m_pDropDown(NULL), m_cyItem(CY_ITEM)
{
}

HWND CACListView::Create(HWND hwndParent)
{
    ATLASSERT(m_hWnd == NULL);
    DWORD dwStyle = WS_CHILD | /*WS_VISIBLE |*/ WS_CLIPSIBLINGS | LVS_NOCOLUMNHEADER |
                    LVS_OWNERDATA | LVS_OWNERDRAWFIXED | LVS_SINGLESEL | LVS_REPORT;
    HWND hWnd = ::CreateWindowExW(0, GetWndClassName(), L"Internet Explorer", dwStyle,
                                  0, 0, 0, 0, hwndParent, NULL,
                                  _AtlBaseModule.GetModuleInstance(), NULL);
    SubclassWindow(hWnd); // do subclass to handle messages
    // set extended listview style
    DWORD exstyle = LVS_EX_ONECLICKACTIVATE | LVS_EX_FULLROWSELECT | LVS_EX_TRACKSELECT;
    SetExtendedListViewStyle(exstyle, exstyle);
    // insert one column (needed to insert items)
    LV_COLUMNW column = { LVCF_FMT | LVCF_WIDTH };
    column.fmt = LVCFMT_LEFT;
    column.cx = CX_LIST - ::GetSystemMetrics(SM_CXVSCROLL);
    InsertColumn(0, &column);
    return m_hWnd;
}

// set font handle
VOID CACListView::SetFont(HFONT hFont)
{
    SendMessageW(WM_SETFONT, (WPARAM)hFont, TRUE); // set font

    // get listview item height
    m_cyItem = CY_ITEM;
    HDC hDC = GetDC();
    if (hDC)
    {
        HGDIOBJ hFontOld = ::SelectObject(hDC, hFont);
        TEXTMETRICW tm;
        if (::GetTextMetricsW(hDC, &tm))
        {
            m_cyItem = (tm.tmHeight * 3) / 2; // 3/2 of text height
        }
        ::SelectObject(hDC, hFontOld);
        ReleaseDC(hDC);
    }
}

// get the number of visible items
INT CACListView::GetVisibleCount()
{
    if (m_cyItem <= 0) // avoid "division by zero"
        return 0;
    CRect rc;
    GetClientRect(&rc);
    return rc.Height() / m_cyItem;
}

// get the text of an item
CStringW CACListView::GetItemText(INT iItem)
{
    // NOTE: LVS_OWNERDATA doesn't support LVM_GETITEMTEXT.
    ATLASSERT(m_pDropDown);
    ATLASSERT(GetStyle() & LVS_OWNERDATA);
    return m_pDropDown->GetItemText(iItem);
}

// get the item index from position
INT CACListView::ItemFromPoint(INT x, INT y)
{
    LV_HITTESTINFO hittest;
    hittest.pt.x = x;
    hittest.pt.y = y;
    return HitTest(&hittest);
}

// get current selection
INT CACListView::GetCurSel()
{
    return GetNextItem(-1, LVNI_ALL | LVNI_SELECTED);
}

// set current selection
VOID CACListView::SetCurSel(INT iItem)
{
    if (iItem == -1)
        SetItemState(-1, 0, LVIS_SELECTED); // select none
    else
        SetItemState(iItem, LVIS_SELECTED, LVIS_SELECTED);
}

// select the specific position (in client coordinates)
VOID CACListView::SelectHere(INT x, INT y)
{
    SetCurSel(ItemFromPoint(x, y));
}

// WM_LBUTTONUP / WM_MBUTTONUP / WM_RBUTTONUP @implemented
LRESULT CACListView::OnButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("CACListView::OnButtonUp(%p)\n", this);
    return 0; // eat
}

// WM_LBUTTONDOWN @implemented
// This message is posted when the user pressed the left mouse button while the cursor is inside.
LRESULT CACListView::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("CACListView::OnLButtonDown(%p)\n", this);
    ATLASSERT(m_pDropDown);
    INT iItem = ItemFromPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    if (iItem != -1)
    {
        m_pDropDown->SelectItem(iItem); // select the item
        CStringW strText = GetItemText(iItem); // get text of item
        m_pDropDown->SetEditText(strText); // set text
        m_pDropDown->SetEditSel(0, strText.GetLength()); // select all
        m_pDropDown->HideDropDown(); // hide
    }
    return 0;
}

// WM_MBUTTONDOWN / WM_RBUTTONDOWN @implemented
LRESULT CACListView::OnMRButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("CACListView::OnMRButtonDown(%p)\n", this);
    return 0; // eat
}

// WM_MOUSEWHEEL @implemented
LRESULT CACListView::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("CACListView::OnMouseWheel(%p)\n", this);
    ATLASSERT(m_pDropDown);
    LRESULT ret = DefWindowProcW(uMsg, wParam, lParam); // do default
    m_pDropDown->UpdateScrollBar();
    return ret;
}

// WM_NCHITTEST
LRESULT CACListView::OnNCHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("CACListView::OnNCHitTest(%p)\n", this);
    ATLASSERT(m_pDropDown);
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }; // in screen coordinates
    ScreenToClient(&pt); // into client coordinates
    HWND hwndTarget = m_pDropDown->ChildWindowFromPoint(pt);
    if (hwndTarget != m_hWnd)
        return HTTRANSPARENT; // pass through (for resizing the drop-down window)
    bHandled = FALSE; // do default
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
// CACScrollBar

HWND CACScrollBar::Create(HWND hwndParent)
{
    ATLASSERT(m_hWnd == NULL);
    DWORD dwStyle = WS_CHILD | /*WS_VISIBLE |*/ SBS_BOTTOMALIGN | SBS_VERT;
    m_hWnd = ::CreateWindowExW(0, GetWndClassName(), NULL, dwStyle,
                               0, 0, 0, 0, hwndParent, NULL,
                               _AtlBaseModule.GetModuleInstance(), NULL);
    // we don't subclass because no message handling is needed
    return m_hWnd;
}

//////////////////////////////////////////////////////////////////////////////
// CACSizeBox

HWND CACSizeBox::Create(HWND hwndParent)
{
    ATLASSERT(m_hWnd == NULL);
    DWORD dwStyle = WS_CHILD | /*WS_VISIBLE |*/ SBS_SIZEBOX;
    HWND hWnd = ::CreateWindowExW(0, GetWndClassName(), NULL, dwStyle,
                                  0, 0, 0, 0, hwndParent, NULL,
                                  _AtlBaseModule.GetModuleInstance(), NULL);
    SubclassWindow(hWnd); // do subclass to handle message
    return m_hWnd;
}

VOID CACSizeBox::SetStatus(BOOL bDowner, BOOL bLongList)
{
    // set flags
    m_bDowner = bDowner;
    m_bLongList = bLongList;

    if (bLongList)
    {
        SetWindowRgn(NULL, TRUE); // reset window region
        return;
    }

    RECT rc;
    GetWindowRect(&rc); // get size-box size
    ::OffsetRect(&rc, -rc.left, -rc.top); // window regions use special coordinates
    ATLASSERT(rc.left == 0 && rc.top == 0);

    // create a trianglar region
    HDC hDC = ::CreateCompatibleDC(NULL);
    ::BeginPath(hDC);
    if (m_bDowner)
    {
        ::MoveToEx(hDC, rc.right, 0, NULL);
        ::LineTo(hDC, rc.right, rc.bottom);
        ::LineTo(hDC, 0, rc.bottom);
        ::LineTo(hDC, rc.right, 0);
    }
    else
    {
        ::MoveToEx(hDC, rc.right, rc.bottom, NULL);
        ::LineTo(hDC, rc.right, 0);
        ::LineTo(hDC, 0, 0);
        ::LineTo(hDC, rc.right, rc.bottom);
    }
    ::EndPath(hDC);
    HRGN hRgn = ::PathToRegion(hDC);
    ::DeleteDC(hDC);

    SetWindowRgn(hRgn, TRUE); // set the trianglar region
}

// WM_ERASEBKGND
LRESULT CACSizeBox::OnEraseBkGnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    return TRUE; // do nothing (for quick drawing)
}

// WM_NCHITTEST
LRESULT CACSizeBox::OnNCHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    return HTTRANSPARENT; // pass through
}

// WM_PAINT
LRESULT CACSizeBox::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    CRect rc;
    GetClientRect(&rc);

    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(&ps);
    if (!hDC)
        return 0;

    // fill background
    ::FillRect(hDC, &rc, ::GetSysColorBrush(COLOR_3DFACE));

    // draw size-box
    INT cxy = rc.Width();
    for (INT shift = 0; shift < 2; ++shift)
    {
        // choose pen color
        INT iColor = ((shift == 0) ? COLOR_HIGHLIGHTTEXT : COLOR_3DSHADOW);
        HPEN hPen = ::CreatePen(PS_SOLID, 1, ::GetSysColor(iColor));
        HGDIOBJ hPenOld = ::SelectObject(hDC, hPen);
        // do loop to draw the slanted lines
        for (INT delta = cxy / 4; delta < cxy; delta += cxy / 4)
        {
            // draw a grip line
            if (m_bDowner)
            {
                ::MoveToEx(hDC, rc.right, rc.top + delta + shift, NULL);
                ::LineTo(hDC, rc.left + delta + shift, rc.bottom);
            }
            else
            {
                ::MoveToEx(hDC, rc.left + delta + shift, rc.top, NULL);
                ::LineTo(hDC, rc.right, rc.bottom - delta - shift);
            }
        }
        // delete pen
        ::SelectObject(hDC, hPenOld);
        ::DeleteObject(hPen);
    }

    EndPaint(&ps);
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
// CAutoComplete public methods

CAutoComplete::CAutoComplete()
    : m_bInSetText(FALSE), m_bInSelectItem(FALSE), m_bEditHasFocus(FALSE)
    , m_bDowner(TRUE), m_dwOptions(ACO_AUTOAPPEND | ACO_AUTOSUGGEST)
    , m_bEnabled(TRUE), m_hwndCombo(NULL), m_hFont(NULL), m_bResized(FALSE)
    , m_hwndEdit(NULL), m_fnOldEditProc(NULL), m_fnOldWordBreakProc(NULL)
    , m_hThread(NULL), m_pThread(NULL)
{
}

HWND CAutoComplete::CreateDropDown()
{
    ATLASSERT(m_hWnd == NULL);
    DWORD dwStyle = WS_POPUP | /*WS_VISIBLE |*/ WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_BORDER;
    DWORD dwExStyle = WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOPARENTNOTIFY;
    Create(NULL, NULL, NULL, dwStyle, dwExStyle);
    TRACE("CAutoComplete::CreateDropDown(%p): m_hWnd=%p, m_hwndEdit=%p\n",
          this, m_hWnd, m_hwndEdit);
    return m_hWnd;
}

CAutoComplete::~CAutoComplete()
{
    TRACE("CAutoComplete::~CAutoComplete(%p)\n", this);
    if (m_hThread)
    {
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }
    if (m_hFont)
    {
        ::DeleteObject(m_hFont);
        m_hFont = NULL;
    }
    // quit holding them
    m_pEnum.Release();
    m_pACList.Release();
}

inline BOOL CAutoComplete::CanAutoSuggest() const
{
    return !!(m_dwOptions & ACO_AUTOSUGGEST) && m_bEnabled;
}

inline BOOL CAutoComplete::CanAutoAppend() const
{
    return !!(m_dwOptions & ACO_AUTOAPPEND) && m_bEnabled;
}

inline BOOL CAutoComplete::UseTab() const
{
    return !!(m_dwOptions & ACO_USETAB) && m_bEnabled;
}

inline BOOL CAutoComplete::IsComboBoxDropped() const
{
    if (!::IsWindow(m_hwndCombo))
        return FALSE;
    return (BOOL)::SendMessageW(m_hwndCombo, CB_GETDROPPEDSTATE, 0, 0);
}

inline BOOL CAutoComplete::FilterPrefixes() const
{
    return !!(m_dwOptions & ACO_FILTERPREFIXES) && m_bEnabled;
}

inline INT CAutoComplete::GetItemCount() const
{
    return m_outerList.GetSize();
}

CStringW CAutoComplete::GetItemText(INT iItem) const
{
    if (iItem < 0 || m_outerList.GetSize() <= iItem)
        return L"";
    return m_outerList[iItem];
}

inline CStringW CAutoComplete::GetEditText() const
{
    WCHAR szText[L_MAX_URL_LENGTH];
    if (::GetWindowTextW(m_hwndEdit, szText, _countof(szText)))
        return szText;
    return L"";
}

VOID CAutoComplete::SetEditText(LPCWSTR pszText)
{
    m_bInSetText = TRUE; // don't hide drop-down
    ::CallWindowProcW(m_fnOldEditProc, m_hwndEdit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(pszText));
    m_bInSetText = FALSE;
}

inline CStringW CAutoComplete::GetStemText(const CStringW& strText) const
{
    INT ich = strText.ReverseFind(L'\\');
    if (ich == -1)
        return L""; // no stem
    return strText.Left(ich + 1);
}

VOID CAutoComplete::SetEditSel(INT ich0, INT ich1)
{
    ::CallWindowProcW(m_fnOldEditProc, m_hwndEdit, EM_SETSEL, ich0, ich1);
}

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

VOID CAutoComplete::HideDropDown()
{
    ShowWindow(SW_HIDE);
}

VOID CAutoComplete::SelectItem(INT iItem)
{
    m_hwndList.SetCurSel(iItem);
    if (iItem != -1)
        m_hwndList.EnsureVisible(iItem, FALSE);
}

VOID CAutoComplete::DoAutoAppend(PAC_THREAD pThread)
{
    if (!CanAutoAppend()) // can we auto-append?
        return; // don't append

    CStringW strText = GetEditText(); // get the text
    if (strText.IsEmpty())
        return; // don't append

    INT cItems = m_outerList.GetSize(); // get the number of items
    if (cItems == 0)
        return; // don't append

    // get the common string
    CStringW strCommon;
    BOOL bFound = FALSE;
    for (INT iItem = 0; iItem < cItems; ++iItem)
    {
        const CStringW& strItem = m_outerList[iItem]; // get the text of the item

        CStringW strBody;
        if (DropPrefix(strItem, strBody) &&
            ::StrCmpNIW(strBody, strText, strText.GetLength()) == 0)
        {
            if (!bFound)
            {
                bFound = TRUE;
                strCommon = strBody;
                continue;
            }
            for (INT ich = 0; ich < strBody.GetLength(); ++ich)
            {
                if (strCommon.GetLength() <= ich)
                    break;
                if (ChrCmpIW(strCommon[ich], strBody[ich]) != 0)
                {
                    strCommon = strCommon.Left(ich);
                    break;
                }
            }
            continue;
        }

        if (::StrCmpNIW(strItem, strText, strText.GetLength()) == 0)
        {
            if (!bFound)
            {
                bFound = TRUE;
                strCommon = strItem;
                continue;
            }

            for (INT ich = 0; ich < strItem.GetLength(); ++ich)
            {
                if (strCommon.GetLength() <= ich)
                    break;
                if (ChrCmpIW(strCommon[ich], strItem[ich]) != 0)
                {
                    strCommon = strCommon.Left(ich);
                    break;
                }
            }
        }
    }

    if (strCommon.IsEmpty() || strCommon.GetLength() <= strText.GetLength())
        return; // no suggestion

    // append suggestion
    SetEditText(strCommon);

    // select the appended suggestion
    SetEditSel(strText.GetLength(), strCommon.GetLength());
}

// go back a word ([Ctrl]+[Backspace])
VOID CAutoComplete::DoBackWord()
{
    // get current selection
    INT ich0, ich1;
    ::CallWindowProcW(m_fnOldEditProc, m_hwndEdit, EM_GETSEL,
                      reinterpret_cast<WPARAM>(&ich0), reinterpret_cast<LPARAM>(&ich1));
    if (ich0 <= 0 || ich0 != ich1) // there is selection or left-side end
        return; // don't do anything
    // get text
    CStringW str = GetEditText();
    // extend the range
    while (ich0 > 0 && IsWordBreak(str[ich0 - 1]))
        --ich0;
    while (ich0 > 0 && !IsWordBreak(str[ich0 - 1]))
        --ich0;
    // select range
    SetEditSel(ich0, ich1);
    // replace selection with empty text (this is actually deletion)
    ::CallWindowProcW(m_fnOldEditProc, m_hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)L"");
}

VOID CAutoComplete::UpdateScrollBar()
{
    // copy scroll info from m_hwndList to m_hwndScrollBar
    SCROLLINFO si = { sizeof(si), SIF_ALL };
    m_hwndList.GetScrollInfo(SB_VERT, &si);
    m_hwndScrollBar.SetScrollInfo(SB_CTL, &si, FALSE);

    // show/hide scroll bar
    INT cVisibles = m_hwndList.GetVisibleCount();
    INT cItems = m_hwndList.GetItemCount();
    BOOL bShowScroll = (cItems > cVisibles);
    m_hwndScrollBar.ShowWindow(bShowScroll ? SW_SHOWNOACTIVATE : SW_HIDE);
    if (bShowScroll)
        m_hwndScrollBar.InvalidateRect(NULL, FALSE); // redraw
}

BOOL CAutoComplete::OnEditKeyDown(WPARAM wParam, LPARAM lParam)
{
    TRACE("CAutoComplete::OnEditKeyDown(%p, %p)\n", this, wParam);

    UINT vk = (UINT)wParam; // virtual key
    switch (vk)
    {
        case VK_UP: case VK_DOWN:
        case VK_PRIOR: case VK_NEXT:
            // is suggestion available?
            if (!CanAutoSuggest())
                return FALSE; // do default
            if (IsWindowVisible())
                return OnListUpDown(vk);
            break;
        case VK_ESCAPE:
        {
            // is suggestion available?
            if (!CanAutoSuggest())
                return FALSE; // do default
            if (IsWindowVisible())
            {
                SetEditText(m_strText); // revert the edit text
                // select the end
                INT cch = m_strText.GetLength();
                SetEditSel(cch, cch);
                HideDropDown(); // hide
                return TRUE; // eat
            }
            break;
        }
        case VK_RETURN:
        {
            if (::GetKeyState(VK_CONTROL) < 0)
            {
                // quick edit
                CStringW strText = GetEditText();
                SetEditText(GetQuickEdit(strText));
            }
            else
            {
                // if item is selected, then update the edit text
                INT iItem = m_hwndList.GetCurSel();
                if (iItem != -1)
                {
                    CStringW strText = GetItemText(iItem);
                    SetEditText(strText);
                }
            }
            // select all
            INT cch = ::CallWindowProcW(m_fnOldEditProc, m_hwndEdit, WM_GETTEXTLENGTH, 0, 0);
            SetEditSel(0, cch);
            // hide
            HideDropDown();
            break;
        }
        case VK_TAB:
        {
            // ACO_USETAB
            if (IsWindowVisible() && UseTab())
            {
                if (GetKeyState(VK_SHIFT) < 0)
                    return OnListUpDown(VK_UP);
                else
                    return OnListUpDown(VK_DOWN);
            }
            break;
        }
        case VK_DELETE:
        {
            // is suggestion available?
            if (!CanAutoSuggest())
                return FALSE; // do default
            ::DefSubclassProc(m_hwndEdit, WM_KEYDOWN, VK_DELETE, 0); // do default
            StartCompletion(FALSE);
            return TRUE; // eat
        }
        case VK_BACK:
        {
            if (::GetKeyState(VK_CONTROL) < 0)
            {
                DoBackWord();
                return TRUE; // eat
            }
            break;
        }
    }
    return FALSE; // default
}

LRESULT CAutoComplete::OnEditChar(WPARAM wParam, LPARAM lParam)
{
    TRACE("CACEditCtrl::OnEditChar(%p, %p)\n", this, wParam);
    if (wParam == L'\n' || wParam == L'\t')
        return 0; // eat
    LRESULT ret = ::DefSubclassProc(m_hwndEdit, WM_CHAR, wParam, lParam); // do default
    if (CanAutoSuggest() || CanAutoAppend())
        StartCompletion(wParam != VK_BACK);
    return ret;
}

VOID CAutoComplete::OnListSelChange()
{
    // update EDIT text
    INT iItem = m_hwndList.GetCurSel();
    CStringW text = ((iItem != -1) ? GetItemText(iItem) : m_strText);
    SetEditText(text);
    // ensure the item visible
    m_hwndList.EnsureVisible(iItem, FALSE);
    // select the end
    INT cch = text.GetLength();
    SetEditSel(cch, cch);
}

BOOL CAutoComplete::OnListUpDown(UINT vk)
{
    if (!CanAutoSuggest())
        return FALSE; // default

    if ((m_dwOptions & ACO_UPDOWNKEYDROPSLIST) && !IsWindowVisible())
    {
        ShowDropDown();
        return TRUE; // eat
    }

    INT iItem = m_hwndList.GetCurSel(); // current selection
    INT cItems = m_hwndList.GetItemCount(); // the number of items
    switch (vk)
    {
        case VK_UP:
            if (iItem == -1)
                iItem = cItems - 1;
            else if (iItem == 0)
                iItem = -1;
            else
                --iItem;
            m_hwndList.SetCurSel(iItem);
            break;
        case VK_DOWN:
            if (iItem == -1)
                iItem = 0;
            else if (iItem == cItems - 1)
                iItem = -1;
            else
                ++iItem;
            m_hwndList.SetCurSel(iItem);
            break;
        case VK_PRIOR:
            if (iItem == -1)
            {
                iItem = cItems - 1;
            }
            else if (iItem == 0)
            {
                iItem = -1;
            }
            else
            {
                iItem -= m_hwndList.GetVisibleCount() - 1;
                if (iItem < 0)
                    iItem = 0;
            }
            m_hwndList.SetCurSel(iItem);
            break;
        case VK_NEXT:
            if (iItem == -1)
            {
                iItem = 0;
            }
            else if (iItem == cItems - 1)
            {
                iItem = -1;
            }
            else
            {
                iItem += m_hwndList.GetVisibleCount() - 1;
                if (iItem > cItems)
                    iItem = cItems - 1;
            }
            m_hwndList.SetCurSel(iItem);
            break;
        default:
        {
            ATLASSERT(FALSE);
            break;
        }
    }

    return TRUE; // eat
}

//////////////////////////////////////////////////////////////////////////////
// CAutoComplete IAutoComplete methods

// @implemented
STDMETHODIMP CAutoComplete::Enable(BOOL fEnable)
{
    TRACE("(%p)->Enable(%d)\n", this, fEnable);
    m_bEnabled = fEnable;
    return S_OK;
}

STDMETHODIMP
CAutoComplete::Init(HWND hwndEdit, IUnknown *punkACL,
                    LPCOLESTR pwszRegKeyPath, LPCOLESTR pwszQuickComplete)
{
    TRACE("(%p)->Init(0x%08lx, %p, %s, %s)\n",
          this, hwndEdit, punkACL, debugstr_w(pwszRegKeyPath), debugstr_w(pwszQuickComplete));
    // sanity check
    if (m_hwndEdit || !punkACL)
        return E_FAIL;
    if (!hwndEdit)
        return E_INVALIDARG;
    // do subclass textbox to watch messages
    m_fnOldEditProc = reinterpret_cast<WNDPROC>(::GetWindowLongPtrW(hwndEdit, GWLP_WNDPROC));
    if (!m_fnOldEditProc)
        return E_FAIL;
    if (!::SetWindowSubclass(hwndEdit, EditSubclassProc, 0, reinterpret_cast<DWORD_PTR>(this)))
        return E_FAIL;
    m_hwndEdit = hwndEdit;
    m_bEditHasFocus = (::GetFocus() == hwndEdit);
    // add reference to m_hwndEdit
    AddRef();
    // set word break procedure
    m_fnOldWordBreakProc = reinterpret_cast<EDITWORDBREAKPROCW>(
        ::SendMessageW(m_hwndEdit, EM_SETWORDBREAKPROC, 0,
                       reinterpret_cast<LPARAM>(EditWordBreakProcW)));
    // save position
    ::GetWindowRect(m_hwndEdit, &m_rcEdit);

    // get an IEnumString
    punkACL->QueryInterface(IID_IEnumString, (VOID **)&m_pEnum);
    TRACE("m_pEnum: %p\n", static_cast<void *>(m_pEnum));

    // get an IACList
    punkACL->QueryInterface(IID_IACList, (VOID **)&m_pACList);
    TRACE("m_pACList: %p\n", static_cast<void *>(m_pACList));

    UpdateDropDownState(); // create/hide the drop-down window if necessary

    // load quick completion info
    LoadQuickComplete(pwszRegKeyPath, pwszQuickComplete);

    // any combobox for m_hwndEdit?
    m_hwndCombo = NULL;
    HWND hwndParent = ::GetParent(m_hwndEdit);
    WCHAR szClass[16];
    if (::GetClassNameW(hwndParent, szClass, _countof(szClass)))
    {
        if (::StrCmpIW(szClass, WC_COMBOBOXW) == 0 ||
            ::StrCmpIW(szClass, WC_COMBOBOXEXW) == 0)
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

    if (m_dwOptions & ACO_SEARCH)
        FIXME(" ACO_SEARCH not supported\n");
    if (m_dwOptions & ACO_FILTERPREFIXES)
        FIXME(" ACO_FILTERPREFIXES not supported\n");
    if (m_dwOptions & ACO_RTLREADING)
        FIXME(" ACO_RTLREADING not supported\n");

    UpdateDropDownState(); // create/hide the drop-down window if necessary
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

    Reset();
    m_hwndList.SendMessageW(LVM_SETITEMCOUNT, 0, 0);
    m_outerList.RemoveAll();
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
    if (celt != 1 || !rgelt || !pceltFetched || !m_pEnum)
        return E_INVALIDARG;

    LPWSTR pszText = NULL;
    HRESULT hr = m_pEnum->Next(1, &pszText, pceltFetched);
    if (hr == S_OK)
        *rgelt = pszText;
    else
        ::CoTaskMemFree(pszText);
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

VOID CAutoComplete::UpdateDropDownState()
{
    if (CanAutoSuggest())
    {
        // create the drop-down window if not existed
        if (!m_hWnd)
            CreateDropDown();
    }
    else
    {
        // hide if existed
        if (m_hWnd)
            ShowWindow(SW_HIDE);
    }
}

// calculate the positions of the controls
VOID CAutoComplete::CalcRects(BOOL bDowner, RECT& rcList, RECT& rcScrollBar, RECT& rcSizeBox) const
{
    // get the client rectangle
    RECT rcClient;
    GetClientRect(&rcClient);

    // the list
    rcList = rcClient;
    rcList.right = rcList.left + CX_LIST;

    // the scroll bar
    rcScrollBar = rcClient;
    rcScrollBar.left = rcClient.right - GetSystemMetrics(SM_CXVSCROLL);
    if (bDowner)
    {
        rcScrollBar.top = 0;
        rcScrollBar.bottom = rcClient.bottom - GetSystemMetrics(SM_CYHSCROLL);
    }
    else
    {
        rcScrollBar.top = GetSystemMetrics(SM_CYHSCROLL);
    }

    // the size box
    rcSizeBox = rcClient;
    rcSizeBox.left = rcClient.right - GetSystemMetrics(SM_CXVSCROLL);
    if (bDowner)
    {
        rcSizeBox.top = rcClient.bottom - GetSystemMetrics(SM_CYHSCROLL);
    }
    else
    {
        rcSizeBox.top = 0;
        rcSizeBox.bottom = rcClient.top + GetSystemMetrics(SM_CYHSCROLL);
    }
}

VOID CAutoComplete::LoadQuickComplete(LPCWSTR pwszRegKeyPath, LPCWSTR pwszQuickComplete)
{
    m_strQuickComplete.Empty();

    if (pwszRegKeyPath)
    {
        CStringW strPath = pwszRegKeyPath;
        INT ichSep = strPath.ReverseFind(L'\\'); // find separator
        if (ichSep != -1) // found the last separator
        {
            // split by the separator
            CStringW strKey = strPath.Left(ichSep);
            CStringW strName = strPath.Mid(ichSep + 1);

            // load from registry
            WCHAR szValue[MAX_PATH] = L"";
            DWORD cbValue = sizeof(szValue), dwType = REG_NONE;
            SHRegGetUSValueW(pwszRegKeyPath, strName, &dwType,
                             szValue, &cbValue, FALSE, NULL, 0);
            if (szValue[0] != 0 && cbValue != 0 &&
                (dwType == REG_SZ || dwType == REG_EXPAND_SZ))
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

CStringW CAutoComplete::GetQuickEdit(LPCWSTR pszText) const
{
    if (pszText[0] == 0 || m_strQuickComplete.IsEmpty())
        return pszText;

    // m_strQuickComplete will be "www.%s.com" etc.
    CStringW ret;
    ret.Format(m_strQuickComplete, pszText);
    return ret;
}

VOID CAutoComplete::RepositionDropDown()
{
    // If Edit has no focus, don't open auto-complete
    if (!m_bEditHasFocus)
    {
        TRACE("!m_bEditHasFocus\n");
        return;
    }

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
    ATLASSERT(cyItem > 0);

    // get m_hwndEdit position
    RECT rcEdit;
    ::GetWindowRect(m_hwndEdit, &rcEdit);
    INT x = rcEdit.left, y = rcEdit.bottom;

    // get list extent
    RECT rcMon = mi.rcMonitor;
    INT cx = rcEdit.right - rcEdit.left, cy = cItems * cyItem;
    BOOL bLongList = FALSE;
    if (cy > CY_LIST)
    {
        cy = INT(CY_LIST / cyItem) * cyItem;
        bLongList = TRUE;
    }

    // convert rectangle for frame
    RECT rc = { 0, 0, cx, cy };
    AdjustWindowRectEx(&rc, GetStyle(), FALSE, GetExStyle());
    cy = rc.bottom - rc.top;

    if (!m_bResized)
    {
        // is the drop-down window a 'downer' or 'upper'?
        // NOTE: 'downer' is below the EDIT control. 'upper' is above the EDIT control.
        m_bDowner = (rcEdit.bottom + cy < rcMon.bottom);
    }

    // adjust y and cy
    if (m_bDowner)
    {
        if (rcMon.bottom < y + cy)
        {
            cy = ((rcMon.bottom - y) / cyItem) * cyItem;
            bLongList = TRUE;
        }
    }
    else
    {
        if (rcEdit.top < rcMon.top + cy)
        {
            cy = ((rcEdit.top - rcMon.top) / cyItem) * cyItem;
            bLongList = TRUE;
        }
        y = rcEdit.top - cy;
    }

    // set status
    m_hwndSizeBox.SetStatus(m_bDowner, bLongList);

    if (m_bResized) // already resized?
        PostMessageW(WM_SIZE, 0, 0); // re-layout
    else
        MoveWindow(x, y, cx, cy); // move

    // show without activation
    ShowWindow(SW_SHOWNOACTIVATE);
}

VOID
CAutoComplete::ExtractInnerList(CSimpleArray<CStringW>& outerList,
                                const CSimpleArray<CStringW>& innerList,
                                const CString& strText)
{
    for (INT iItem = 0; iItem < innerList.GetSize(); ++iItem)
    {
        if (m_pThread || !m_hThread)
            break;

        const CStringW& strTarget = innerList[iItem];
        if (DoesMatch(strTarget, strText))
        {
            outerList.Add(strTarget);

            if (outerList.GetSize() >= MAX_ITEM_COUNT)
                break;
        }
    }
}

VOID CAutoComplete::ReLoadInnerList(PAC_THREAD pThread)
{
    pThread->m_innerList.RemoveAll(); // clear contents

    if (!m_pEnum || pThread->m_strText.IsEmpty())
        return;

    // reload the items
    LPWSTR pszItem;
    ULONG cGot;
    HRESULT hr;
    CSimpleArray<CStringW>& innerList = pThread->m_innerList;
    while (!m_pThread && m_hThread)
    {
        // get next item
        hr = m_pEnum->Next(1, &pszItem, &cGot);
        if (hr != S_OK)
            break;

        innerList.Add(pszItem); // append item to innerList
        ::CoTaskMemFree(pszItem); // free
    }
}

VOID CAutoComplete::StartCompletion(BOOL bAppendOK)
{
    TRACE("CAutoComplete::StartCompletion(%p, %d)\n", this, bAppendOK);

    if (!m_pEnum || (!CanAutoSuggest() && !CanAutoAppend()))
        return;

    ::SendMessageW(m_hWnd, AUTOCOMP_START, bAppendOK, 0);
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

    // create the children
    m_hwndList.Create(m_hWnd);
    if (!m_hwndList)
        return -1; // failure
    m_hwndSizeBox.Create(m_hWnd);
    if (!m_hwndSizeBox)
        return -1; // failure
    m_hwndScrollBar.Create(m_hWnd);
    if (!m_hwndScrollBar)
        return -1; // failure

    // show the controls
    m_hwndList.ShowWindow(SW_SHOWNOACTIVATE);
    m_hwndSizeBox.ShowWindow(SW_SHOWNOACTIVATE);
    m_hwndScrollBar.ShowWindow(SW_SHOWNOACTIVATE);

    // set the list font
    m_hFont = reinterpret_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
    m_hwndList.SetFont(m_hFont);

    // add reference so we won't be deleted during message processing
    AddRef();
    return 0; // success
}

// WM_NCDESTROY
LRESULT CAutoComplete::OnNCDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("CAutoComplete::OnNCDestroy(%p)\n", this);

    // hide
    if (IsWindowVisible())
        HideDropDown();

    // clear CAutoComplete pointers
    m_hwndList.m_pDropDown = NULL;
    m_hwndScrollBar.m_pDropDown = NULL;
    m_hwndSizeBox.m_pDropDown = NULL;

    // destroy controls
    if (m_hwndList) m_hwndList.DestroyWindow();
    if (m_hwndScrollBar) m_hwndScrollBar.DestroyWindow();
    if (m_hwndSizeBox) m_hwndSizeBox.DestroyWindow();

    // clean up
    m_hwndCombo = NULL;

    // Tell ATL to clean up
    bHandled = 0;

    return 0;
}

VOID CAutoComplete::OnFinalMessage(HWND)
{
    // The message loop is finished, now we can safely destruct!
    Release();
}

// WM_EXITSIZEMOVE
// This message is sent once to a window after it has exited the moving or sizing mode.
LRESULT CAutoComplete::OnExitSizeMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("CAutoComplete::OnExitSizeMove(%p)\n", this);
    m_bResized = TRUE; // remember resized

    ModifyStyle(WS_THICKFRAME, 0); // remove thick frame to resize
    // frame changed
    UINT uSWP_ = SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE;
    SetWindowPos(NULL, 0, 0, 0, 0, uSWP_);

    ::SetFocus(m_hwndEdit); // restore focus
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

    // draw background and set text color
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

    // draw text
    rcItem.left += ::GetSystemMetrics(SM_CXBORDER);
    HGDIOBJ hFontOld = ::SelectObject(hDC, m_hFont);
    const UINT uDT_ = DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER;
    ::SetBkMode(hDC, TRANSPARENT);
    ::DrawTextW(hDC, strItem, -1, &rcItem, uDT_);
    ::SelectObject(hDC, hFontOld);

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
    if (!m_hwndList)
        return FALSE;
    ATLASSERT(m_hwndList.GetStyle() & LVS_OWNERDRAWFIXED);
    pMeasure->itemHeight = m_hwndList.m_cyItem; // height of item
    return TRUE;
}

// WM_MOUSEACTIVATE @implemented
// The return value of this message specifies whether the window should be activated or not.
LRESULT CAutoComplete::OnMouseActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    return MA_NOACTIVATE; // don't activate by mouse
}

// WM_NCACTIVATE
// This message is sent to a window to indicate an active or inactive state.
LRESULT CAutoComplete::OnNCActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    bHandled = FALSE; // do default
    return 0;
}

// WM_NCLBUTTONDOWN
LRESULT CAutoComplete::OnNCLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    switch (wParam)
    {
        case HTBOTTOMRIGHT: case HTTOPRIGHT:
        {
            // add thick frame to resize.
            ModifyStyle(0, WS_THICKFRAME);
            // frame changed
            UINT uSWP_ = SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE;
            SetWindowPos(NULL, 0, 0, 0, 0, uSWP_);
            break;
        }
    }
    bHandled = FALSE; // do default
    return 0;
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
        case NM_HOVER: // mouse is hovering
        {
            POINT pt;
            ::GetCursorPos(&pt); // get cursor position in screen coordinates
            m_hwndList.ScreenToClient(&pt); // into client coordinates
            INT iItem = m_hwndList.ItemFromPoint(pt.x, pt.y);
            if (iItem != -1)
            {
                m_bInSelectItem = TRUE; // don't respond
                m_hwndList.SetCurSel(iItem); // select
                m_bInSelectItem = FALSE;
            }
            return TRUE; // eat
        }
        case LVN_GETDISPINFOA: // for user's information only
        {
            TRACE("LVN_GETDISPINFOA\n");
            if (pnmh->hwndFrom != m_hwndList)
                break;

            LV_DISPINFOA *pDispInfo = reinterpret_cast<LV_DISPINFOA *>(pnmh);
            LV_ITEMA *pItem = &pDispInfo->item;
            INT iItem = pItem->iItem;
            if (iItem == -1)
                break;

            CStringW strText = GetItemText(iItem);
            if (pItem->mask & LVIF_TEXT)
                SHUnicodeToAnsi(strText, pItem->pszText, pItem->cchTextMax);
            break;
        }
        case LVN_GETDISPINFOW: // for user's information only
        {
            TRACE("LVN_GETDISPINFOW\n");
            if (pnmh->hwndFrom != m_hwndList)
                break;

            LV_DISPINFOW *pDispInfo = reinterpret_cast<LV_DISPINFOW *>(pnmh);
            LV_ITEMW *pItem = &pDispInfo->item;
            INT iItem = pItem->iItem;
            if (iItem == -1)
                break;

            CStringW strText = GetItemText(iItem);
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
            m_hwndList.EnsureVisible(iItem, FALSE);
            return TRUE;
        }
        case LVN_ITEMACTIVATE: // enabled by LVS_EX_ONECLICKACTIVATE
        {
            TRACE("LVN_ITEMACTIVATE\n");
            LPNMITEMACTIVATE pItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);
            INT iItem = pItemActivate->iItem;
            TRACE("LVN_ITEMACTIVATE: iItem:%d\n", iItem);
            if (iItem != -1) // the item is clicked
            {
                SelectItem(iItem);
                HideDropDown();
            }
            break;
        }
        case LVN_ITEMCHANGED: // item info is changed
        {
            TRACE("LVN_ITEMCHANGED\n");
            LPNMLISTVIEW pListView = reinterpret_cast<LPNMLISTVIEW>(pnmh);
            if (pListView->uChanged & LVIF_STATE) // selection changed
            {
                // listview selection changed
                if (!m_bInSelectItem)
                {
                    OnListSelChange();
                }
                UpdateScrollBar();
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
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }; // in screen coordinates
    ScreenToClient(&pt); // into client coordinates
    if (ChildWindowFromPoint(pt) == m_hwndSizeBox) // hit?
    {
        // allow resizing (with cursor shape)
        return m_bDowner ? HTBOTTOMRIGHT : HTTOPRIGHT;
    }
    bHandled = FALSE; // do default
    return 0;
}

// WM_SIZE @implemented
// This message is sent when the window size is changed.
LRESULT CAutoComplete::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    // calculate the positions of the controls
    CRect rcList, rcScrollBar, rcSizeBox;
    CalcRects(m_bDowner, rcList, rcScrollBar, rcSizeBox);

    // reposition the controls in smartest way
    UINT uSWP_ = SWP_NOACTIVATE | SWP_NOCOPYBITS;
    HDWP hDWP = ::BeginDeferWindowPos(3);
    hDWP = ::DeferWindowPos(hDWP, m_hwndScrollBar, HWND_TOP,
                            rcScrollBar.left, rcScrollBar.top,
                            rcScrollBar.Width(), rcScrollBar.Height(), uSWP_);
    hDWP = ::DeferWindowPos(hDWP, m_hwndSizeBox, m_hwndScrollBar,
                            rcSizeBox.left, rcSizeBox.top,
                            rcSizeBox.Width(), rcSizeBox.Height(), uSWP_);
    hDWP = ::DeferWindowPos(hDWP, m_hwndList, m_hwndSizeBox,
                            rcList.left, rcList.top,
                            rcList.Width(), rcList.Height(), uSWP_);
    ::EndDeferWindowPos(hDWP);

    UpdateScrollBar();
    return 0;
}

// WM_SHOWWINDOW
LRESULT CAutoComplete::OnShowWindow(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    // hook mouse events
    BOOL bShow = (BOOL)wParam;
    if (bShow)
    {
        if (s_hWatchWnd != m_hWnd && ::IsWindowVisible(s_hWatchWnd))
            ::ShowWindowAsync(s_hWatchWnd, SW_HIDE);
        s_hWatchWnd = m_hWnd; // watch this

        // unhook mouse if any
        if (s_hMouseHook)
        {
            HHOOK hHookOld = s_hMouseHook;
            s_hMouseHook = NULL;
            ::UnhookWindowsHookEx(hHookOld);
        }

        // hook mouse
        s_hMouseHook = ::SetWindowsHookEx(WH_MOUSE, MouseProc, NULL, ::GetCurrentThreadId());
        ATLASSERT(s_hMouseHook != NULL);

        // set timer
        SetTimer(WATCH_TIMER_ID, WATCH_INTERVAL, NULL);

        bHandled = FALSE; // do default
        return 0;
    }
    else
    {
        // kill timer
        KillTimer(WATCH_TIMER_ID);

        s_hWatchWnd = NULL; // unwatch

        // unhook mouse if any
        if (s_hMouseHook)
        {
            HHOOK hHookOld = s_hMouseHook;
            s_hMouseHook = NULL;
            ::UnhookWindowsHookEx(hHookOld);
        }

        LRESULT ret = DefWindowProcW(uMsg, wParam, lParam); // do default

        if (m_hwndCombo)
            ::InvalidateRect(m_hwndCombo, NULL, TRUE); // redraw

        m_outerList.RemoveAll(); // no use
        return ret;
    }
}

// WM_TIMER
LRESULT CAutoComplete::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (wParam != WATCH_TIMER_ID) // sanity check
        return 0;

    // if the textbox is dead, then kill the timer
    if (!::IsWindow(m_hwndEdit))
    {
        KillTimer(WATCH_TIMER_ID);
        return 0;
    }

    // m_hwndEdit is moved?
    RECT rcEdit;
    ::GetWindowRect(m_hwndEdit, &rcEdit);
    if (!::EqualRect(&rcEdit, &m_rcEdit))
    {
        // if so, hide
        HideDropDown();

        m_rcEdit = rcEdit; // update rectangle
        m_bResized = FALSE; // clear flag
    }

    return 0;
}

// WM_VSCROLL
// This message is sent when a scroll event occurs.
LRESULT CAutoComplete::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("CAutoComplete::OnVScroll(%p)\n", this);
    WORD code = LOWORD(wParam);
    switch (code)
    {
        case SB_THUMBPOSITION: case SB_THUMBTRACK:
        {
            // get the scrolling info
            INT nPos = HIWORD(wParam);
            SCROLLINFO si = { sizeof(si), SIF_ALL };
            m_hwndList.GetScrollInfo(SB_VERT, &si);

            // scroll the list-view by CListView::EnsureVisible
            INT cItems = m_hwndList.GetItemCount();
            // iItem : cItems == (nPos - si.nMin) : (si.nMax - si.nMin).
            INT iItem = cItems * (nPos - si.nMin) / (si.nMax - si.nMin);
            if (nPos > si.nPos)
            {
                iItem += m_hwndList.GetVisibleCount();
                if (iItem >= cItems)
                    iItem = cItems - 1;
            }
            m_hwndList.EnsureVisible(iItem, FALSE);

            // update scrolling position of m_hwndScrollBar
            si.fMask = SIF_POS;
            m_hwndList.GetScrollInfo(SB_VERT, &si);
            m_hwndScrollBar.SetScrollInfo(SB_VERT, &si, FALSE);
            break;
        }
        default:
        {
            // pass it to m_hwndList
            m_hwndList.SendMessageW(WM_VSCROLL, wParam, lParam);
            UpdateScrollBar();
            break;
        }
    }
    return 0;
}

static inline PAC_THREAD
InterlockedExchangeThreadData(volatile PAC_THREAD *Target, PAC_THREAD Value)
{
    return reinterpret_cast<PAC_THREAD>(
        ::InterlockedExchangePointer(reinterpret_cast<volatile PVOID *>(Target), Value));
}

static unsigned __stdcall AutoCompThreadProc(void *arg)
{
    CAutoComplete* pThis = reinterpret_cast<CAutoComplete*>(arg);
    pThis->AutoCompThreadProc();
    return 0;
}

VOID CAutoComplete::AutoCompThreadProc()
{
    for (;;)
    {
        PAC_THREAD pThread = InterlockedExchangeThreadData(&m_pThread, NULL);
        if (!pThread)
            break;
        DoThreadWork(pThread);
    }
}

VOID CAutoComplete::DoThreadWork(PAC_THREAD pThread)
{
    if (pThread->m_bExpand || m_innerList.GetSize() == 0)
    {
        ReLoadInnerList(pThread);
    }
    else
    {
        pThread->m_innerList = m_innerList;
    }

    if (m_pThread || !m_hThread)
    {
        delete pThread;
        return;
    }

    ExtractInnerList(pThread->m_outerList, pThread->m_innerList, pThread->m_strText);

    if (m_pThread || !m_hThread)
    {
        delete pThread;
        return;
    }

    DoSort(pThread->m_outerList);
    DoUniqueAndTrim(pThread->m_outerList);

    if (m_pThread || !m_hThread ||
        !::PostMessageW(m_hWnd, AUTOCOMP_FINISH, 0, (LPARAM)pThread))
    {
        delete pThread;
    }
}

// AUTOCOMP_START
LRESULT CAutoComplete::OnAutoCompStart(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    BOOL bAppendOK = (BOOL)wParam;

    CStringW strText = GetEditText();
    if (m_strText.CompareNoCase(strText) == 0)
    {
        // no change
        return 0;
    }

    PAC_THREAD pThread = new AC_THREAD { this, bAppendOK, strText };

    // if previous text was empty
    if (m_strText.IsEmpty())
    {
        pThread->m_bReset = TRUE;
    }
    m_strText = strText;

    // do expand the items if the stem is changed
    CStringW strStemText = GetStemText(pThread->m_strText);
    if (m_strStemText.CompareNoCase(strStemText) != 0)
    {
        pThread->m_bReset = TRUE;
        pThread->m_bExpand = !strStemText.IsEmpty();
        m_strStemText = strStemText;
    }

    // reset if necessary
    if (pThread->m_bReset && m_pEnum)
    {
        HRESULT hr = m_pEnum->Reset(); // IEnumString::Reset
        TRACE("m_pEnum->Reset(%p): 0x%08lx\n",
              static_cast<IUnknown *>(m_pEnum), hr);
    }

    // update ac list if necessary
    if (pThread->m_bExpand && m_pACList)
    {
        HRESULT hr = m_pACList->Expand(strStemText); // IACList::Expand
        TRACE("m_pACList->Expand(%p, %S): 0x%08lx\n",
              static_cast<IUnknown *>(m_pACList),
              static_cast<LPCWSTR>(strStemText), hr);
    }

    PAC_THREAD pOld = InterlockedExchangeThreadData(&m_pThread, pThread);
    if (pOld)
        delete pOld;

    BOOL bDoStart = FALSE;
    if (m_hThread)
    {
        if (WaitForSingleObject(m_hThread, 0) != WAIT_TIMEOUT)
        {
            CloseHandle(m_hThread);
            m_hThread = NULL;
            bDoStart = TRUE;
        }
    }
    else
    {
        bDoStart = TRUE;
    }

    if (bDoStart)
        m_hThread = (HANDLE)_beginthreadex(NULL, 0, ::AutoCompThreadProc, this, 0, NULL);

    return 0;
}

// AUTOCOMP_FINISH
LRESULT CAutoComplete::OnAutoCompFinish(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    PAC_THREAD pThread = reinterpret_cast<PAC_THREAD>(lParam);
    if (m_pThread == NULL)
    {
        FinishCompletion(pThread);
    }
    CloseHandle(m_hThread);
    m_hThread = NULL;
    delete pThread;
    return 0;
}

VOID CAutoComplete::FinishCompletion(PAC_THREAD pThread)
{
    if (m_pThread || !m_hThread)
        return;

    if (!CanAutoSuggest() && !CanAutoAppend())
        return;

    // set inner list
    m_innerList = pThread->m_innerList;

    // set the items of the virtual listview
    m_outerList = pThread->m_outerList; // FIXME: We need more speed!
    m_hwndList.SendMessageW(LVM_SETITEMCOUNT, m_outerList.GetSize(), 0);

    // save text
    m_strText = pThread->m_strText;
    m_strStemText = GetStemText(m_strText);

    if (CanAutoSuggest()) // can we auto-suggest?
    {
        m_bInSelectItem = TRUE; // don't respond
        SelectItem(-1); // select none
        m_bInSelectItem = FALSE;

        if (m_outerList.GetSize() > 0)
            RepositionDropDown();
        else
            HideDropDown();
    }

    if (CanAutoAppend() && pThread->m_bAppendOK) // can we auto-append?
    {
        DoAutoAppend(pThread);
    }
}
