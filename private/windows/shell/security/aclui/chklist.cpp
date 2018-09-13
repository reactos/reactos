//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       chklist.cpp
//
//  This file contains the implementation of the CheckList control.
//
//--------------------------------------------------------------------------

#include "aclpriv.h"

//
// Text and Background colors
//
#define TEXT_COLOR  COLOR_WINDOWTEXT
#define BK_COLOR    COLOR_WINDOW

//
// Default dimensions for child controls. All are in dialog units.
// Currently only the column width is user-adjustable (via the
// CLM_SETCOLUMNWIDTH message).
//
#define DEFAULT_COLUMN_WIDTH    32
#define DEFAULT_CHECK_WIDTH     9
#define DEFAULT_HORZ_SPACE      7
#define DEFAULT_VERTICAL_SPACE  3
#define DEFAULT_ITEM_HEIGHT     8


//
// 16 bits are used for the control ID's, divided into n bits for
// the subitem (least significant) and 16-n bits for the item index.
//
// ID_SUBITEM_BITS can be adjusted to control the maximum number of
// items and subitems. For example, to allow up to 7 subitems and 8k
// items, set ID_SUBITEM_BITS to 3.
//

// Use the low 2 bits for the subitem index, the rest for the item index.
// (4 subitems max, 16k items max)
#define ID_SUBITEM_BITS         2

#define ID_SUBITEM_MASK         ((1 << ID_SUBITEM_BITS) - 1)
#define GET_ITEM(id)            ((id) >> ID_SUBITEM_BITS)
#define GET_SUBITEM(id)         ((id) & ID_SUBITEM_MASK)

#define MAKE_CTRL_ID(i, s)      (0xffff & (((i) << ID_SUBITEM_BITS) | ((s) & ID_SUBITEM_MASK)))
#define MAKE_LABEL_ID(i)        MAKE_CTRL_ID(i, 0)
// Note that the subitem (column) index is one-based for the checkboxes
// (the zero column is the label).  The item (row) index is zero-based.

#define MAX_CHECK_COLUMNS       ID_SUBITEM_MASK


TCHAR const c_szStaticClass[]   = TEXT("STATIC");
TCHAR const c_szButtonClass[]   = TEXT("BUTTON");


class CCheckList
{
private:
    LONG m_cItems;
    LONG m_cSubItems;
    RECT m_rcItemLabel;
    LONG m_nCheckPos[MAX_CHECK_COLUMNS];
    LONG m_cxCheckBox;
    LONG m_cxCheckColumn;
    HWND m_hwndCheckFocus;
    LPTSTR m_pszColumnDesc[MAX_CHECK_COLUMNS];

    int m_cWheelDelta;
    static UINT g_ucScrollLines;

private:
    CCheckList(HWND hWnd, LPCREATESTRUCT lpcs);
    ~CCheckList(void);

    LRESULT MsgCommand(HWND hWnd, WORD idCmd, WORD wNotify, HWND hwndCtrl);
    void MsgPaint(HWND hWnd, HDC hdc);
    void MsgVScroll(HWND hWnd, int nCode, int nPos);
    void MsgMouseWheel(HWND hWnd, WORD fwFlags, int zDelta);
    void MsgButtonDown(HWND hWnd, WPARAM fwFlags, int xPos, int yPos);
    void MsgEnable(HWND hWnd, BOOL fEnabled);
    void MsgSize(HWND hWnd, DWORD dwSizeType, LONG nWidth, LONG nHeight);

    LONG AddItem(HWND hWnd, LPCTSTR pszLabel, LPARAM lParam);
    void SetState(HWND hWnd, WORD iItem, WORD iSubItem, LONG lState);
    LONG GetState(HWND hWnd, WORD iItem, WORD iSubItem);
    void SetColumnWidth(HWND hWnd, LONG cxDialog, LONG cxColumn);
    void ResetContent(HWND hWnd);
    LONG GetVisibleCount(HWND hWnd);
    LONG GetTopIndex(HWND hWnd);
    void SetTopIndex(HWND hWnd, LONG nIndex)
    { m_cWheelDelta = 0; MsgVScroll(hWnd, SB_THUMBPOSITION, nIndex * m_rcItemLabel.bottom); }
    void EnsureVisible(HWND hWnd, LONG nIndex);
    void DrawCheckFocusRect(HWND hWnd, HWND hwndCheck, BOOL fDraw);
    void GetColumnDescriptions(HWND hWnd);

public:
    static LRESULT CALLBACK WindowProc(HWND hWnd,
                                       UINT uMsg,
                                       WPARAM wParam,
                                       LPARAM lParam);
};


BOOL RegisterCheckListWndClass(void)
{
    WNDCLASS wc;

    wc.style            = 0;
    wc.lpfnWndProc      = CCheckList::WindowProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = hModule;
    wc.hIcon            = NULL;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(BK_COLOR+1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = TEXT(WC_CHECKLIST);

    return (BOOL)RegisterClass(&wc);
}


UINT CCheckList::g_ucScrollLines = (UINT)-1;


CCheckList::CCheckList(HWND hWnd, LPCREATESTRUCT lpcs)
: m_cItems(0), m_hwndCheckFocus(NULL), m_cWheelDelta(0)
{
    TraceEnter(TRACE_CHECKLIST, "CCheckList::CCheckList");
    TraceAssert(hWnd != NULL);
    TraceAssert(lpcs != NULL);

    //
    // Get number of check columns
    //
    m_cSubItems = lpcs->style & CLS_CHECKMASK;

    //
    // Convert default coordinates from dialog units to pixels
    //
    RECT rc;
    rc.left = DEFAULT_CHECK_WIDTH;
    rc.right = DEFAULT_COLUMN_WIDTH;
    rc.top = rc.bottom = 0;
    MapDialogRect(lpcs->hwndParent, &rc);

    // Save the converted values
    m_cxCheckBox = rc.left;
    m_cxCheckColumn = rc.right;

    rc.left = DEFAULT_HORZ_SPACE;
    rc.top = DEFAULT_VERTICAL_SPACE;
    rc.right = 10;              // bogus (unused)
    rc.bottom = DEFAULT_VERTICAL_SPACE + DEFAULT_ITEM_HEIGHT;
    MapDialogRect(lpcs->hwndParent, &rc);

    // Save the converted values
    m_rcItemLabel = rc;

    //
    // Get info for mouse wheel scrolling
    //
    if ((UINT)-1 == g_ucScrollLines)
    {
        g_ucScrollLines = 3; // default
        SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &g_ucScrollLines, 0);
    }

    TraceLeaveVoid();
}

CCheckList::~CCheckList(void)
{
    for (LONG j = 0; j < m_cSubItems; j++)
    {
        LocalFreeString(&m_pszColumnDesc[j]);
    }
}

LRESULT
CCheckList::MsgCommand(HWND hWnd, WORD idCmd, WORD wNotify, HWND hwndCtrl)
{
    TraceEnter(TRACE_CHECKLIST, "CCheckList::MsgCommand");

    // Should only get notifications from visible, enabled, check boxes
    TraceAssert(GET_ITEM(idCmd) < m_cItems);
    TraceAssert(0 < GET_SUBITEM(idCmd) && GET_SUBITEM(idCmd) <= m_cSubItems);
    TraceAssert(hwndCtrl && IsWindowEnabled(hwndCtrl));

    switch (wNotify)
    {
    case BN_CLICKED:
        {
            NM_CHECKLIST nmc;
            nmc.hdr.hwndFrom = hWnd;
            nmc.hdr.idFrom = GetDlgCtrlID(hWnd);
            nmc.hdr.code = CLN_CLICK;
            nmc.iItem = GET_ITEM(idCmd);
            nmc.iSubItem = GET_SUBITEM(idCmd);
            nmc.dwState = (DWORD)SendMessage(hwndCtrl, BM_GETCHECK, 0, 0);
            if (!IsWindowEnabled(hwndCtrl))
                nmc.dwState |= CLST_DISABLED;
            nmc.dwItemData = GetWindowLongPtr(GetDlgItem(hWnd, MAKE_LABEL_ID(nmc.iItem)),
                                              GWLP_USERDATA);
            nmc.cchTextMax = 0;
            nmc.pszText = NULL;

            SendMessage(GetParent(hWnd),
                        WM_NOTIFY,
                        nmc.hdr.idFrom,
                        (LPARAM)&nmc);
        }
        break;

    case BN_SETFOCUS:
        if (m_hwndCheckFocus != hwndCtrl)   // Has the focus moved?
        {
            // Remember where the focus is
            m_hwndCheckFocus = hwndCtrl;

            // Make sure the row is scrolled into view
            EnsureVisible(hWnd, GET_ITEM(idCmd));
        }
        // Always draw the focus rect
        DrawCheckFocusRect(hWnd, hwndCtrl, TRUE);
        break;

    case BN_KILLFOCUS:
        // Remove the focus rect
        m_hwndCheckFocus = NULL;
        DrawCheckFocusRect(hWnd, hwndCtrl, FALSE);
        break;
    }

    TraceLeaveValue(0);
}


void
CCheckList::MsgPaint(HWND hWnd, HDC hdc)
{
    if (hdc == NULL && m_hwndCheckFocus != NULL)
    {
        // This will cause a focus rect to be drawn after the window and
        // all checkboxes have been painted.
        PostMessage(hWnd,
                    WM_COMMAND,
                    GET_WM_COMMAND_MPS(GetDlgCtrlID(m_hwndCheckFocus), m_hwndCheckFocus, BN_SETFOCUS));
    }

    // Default paint
    DefWindowProc(hWnd, WM_PAINT, (WPARAM)hdc, 0);
}


void
CCheckList::MsgVScroll(HWND hWnd, int nCode, int nPos)
{
    UINT cScrollUnitsPerLine;
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;

    if (!GetScrollInfo(hWnd, SB_VERT, &si))
        return;

    cScrollUnitsPerLine = m_rcItemLabel.bottom;

    // One page is always visible, so adjust the range to a more useful value
    si.nMax -= si.nPage - 1;

    switch (nCode)
    {
    case SB_LINEUP:
        // "line" is the height of one item (includes the space in between)
        nPos = si.nPos - cScrollUnitsPerLine;
        break;

    case SB_LINEDOWN:
        nPos = si.nPos + cScrollUnitsPerLine;
        break;

    case SB_PAGEUP:
        nPos = si.nPos - si.nPage;
        break;

    case SB_PAGEDOWN:
        nPos = si.nPos + si.nPage;
        break;

    case SB_TOP:
        nPos = si.nMin;
        break;

    case SB_BOTTOM:
        nPos = si.nMax;
        break;

    case SB_ENDSCROLL:
        nPos = si.nPos;     // don't go anywhere
        break;

    case SB_THUMBTRACK:
        // Do nothing here to allow tracking
        // nPos = si.nPos;    // Do this to prevent tracking
    case SB_THUMBPOSITION:
        // nothing to do here... nPos is passed in
        break;
    }

    // Make sure the new position is within the range
    if (nPos < si.nMin)
        nPos = si.nMin;
    else if (nPos > si.nMax)
        nPos = si.nMax;

    if (nPos != si.nPos)  // are we moving?
    {
        SetScrollPos(hWnd, SB_VERT, nPos, TRUE);
        ScrollWindow(hWnd, 0, si.nPos - nPos, NULL, NULL);
    }
}


void
CCheckList::MsgMouseWheel(HWND hWnd, WORD fwFlags, int iWheelDelta)
{
    int cDetants;

    if ((fwFlags & (MK_SHIFT | MK_CONTROL)) || 0 == g_ucScrollLines)
        return;

    TraceEnter(TRACE_CHECKLIST, "CCheckList::MsgMouseWheel");

    // Update count of scroll amount
    m_cWheelDelta -= iWheelDelta;
    cDetants = m_cWheelDelta / WHEEL_DELTA;
    if (0 == cDetants)
        TraceLeaveVoid();
    m_cWheelDelta %= WHEEL_DELTA;

    if (WS_VSCROLL & GetWindowLong(hWnd, GWL_STYLE))
    {
        SCROLLINFO  si;
        UINT        cScrollUnitsPerLine;
        UINT        cLinesPerPage;
        UINT        cLinesPerDetant;

        // Get the scroll amount of one line
        cScrollUnitsPerLine = m_rcItemLabel.bottom;
        TraceAssert(cScrollUnitsPerLine > 0);

        si.cbSize = sizeof(SCROLLINFO);
        si.fMask = SIF_PAGE | SIF_POS;
        if (!GetScrollInfo(hWnd, SB_VERT, &si))
            TraceLeaveVoid();

        // The size of a page is at least one line, and
        // leaves one line of overlap
        cLinesPerPage = (si.nPage - cScrollUnitsPerLine) / cScrollUnitsPerLine;
        cLinesPerPage = max(1, cLinesPerPage);

        // Don't scroll more than one page per detant
        cLinesPerDetant = min(cLinesPerPage, g_ucScrollLines);

        si.nPos += cDetants * cLinesPerDetant * cScrollUnitsPerLine;

        MsgVScroll(hWnd, SB_THUMBTRACK, si.nPos);
    }
    TraceLeaveVoid();
}


void
CCheckList::MsgButtonDown(HWND hWnd, WPARAM /*fwFlags*/, int /*xPos*/, int yPos)
{
    LONG nItemIndex;
    HWND hwndCheck;
    RECT rc;

    // Get position of the top visible item in client coords
    nItemIndex = GetTopIndex(hWnd);
    hwndCheck = GetDlgItem(hWnd, MAKE_CTRL_ID(nItemIndex, 0));
    GetWindowRect(hwndCheck, &rc);
    MapWindowPoints(NULL, hWnd, (LPPOINT)&rc, 2);

    // Find nearest item
    nItemIndex += (yPos - rc.top + m_rcItemLabel.top/2)/m_rcItemLabel.bottom;
    nItemIndex = max(0, min(nItemIndex, m_cItems - 1)); // 0 <= y < m_cItems

    // Set focus to first subitem that is enabled
    for (LONG j = 1; j <= m_cSubItems; j++)
    {
        hwndCheck = GetDlgItem(hWnd, MAKE_CTRL_ID(nItemIndex, j));
        if (IsWindowEnabled(hwndCheck))
        {
            SetFocus(hwndCheck);
            break;
        }
    }
}


void
CCheckList::MsgEnable(HWND hWnd, BOOL fEnabled)
{
    static BOOL bInMsgEnable = FALSE;

    if (!bInMsgEnable)
    {
        bInMsgEnable = TRUE;
        for (LONG i = 0; i < m_cItems; i++)
        {
            for (LONG j = 1; j <= m_cSubItems; j++)
            {
                EnableWindow(GetDlgItem(hWnd, MAKE_CTRL_ID(i, j)), fEnabled);
            }
        }
        if (!fEnabled)
            EnableWindow(hWnd, TRUE);
        bInMsgEnable = FALSE;
    }
}


void
CCheckList::MsgSize(HWND hWnd, DWORD dwSizeType, LONG nWidth, LONG nHeight)
{
    TraceEnter(TRACE_CHECKLIST, "CCheckList::MsgSize");
    TraceAssert(hWnd != NULL);

    if (dwSizeType == SIZE_RESTORED)
    {
        RECT rc;
        SCROLLINFO si;

        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE;
        si.nMin = 0;
        si.nMax = m_cItems * m_rcItemLabel.bottom + m_rcItemLabel.top - 1;
        si.nPage = nHeight;                         // ^^^^^^^^^ extra space

        SetScrollInfo(hWnd, SB_VERT, &si, FALSE);

        // Don't trust the width value passed in, since SetScrollInfo may
        // affect it if the scroll bar is turning on or off.
        GetClientRect(hWnd, &rc);
        nWidth = rc.right;

        // If the scrollbar is turned on, artificially bump up the width
        // by the width of the scrollbar, so the boxes don't jump to the left
        // when we have a scrollbar.
        if ((UINT)si.nMax >= si.nPage)
            nWidth += GetSystemMetrics(SM_CYHSCROLL);

        SetColumnWidth(hWnd, nWidth, m_cxCheckColumn);
    }

    TraceLeaveVoid();
}


LONG
CCheckList::AddItem(HWND hWnd, LPCTSTR pszLabel, LPARAM lParam)
{
    HWND hwndNew;
    HWND hwndPrev;
    RECT rc;
    LONG cyOffset;

    TraceEnter(TRACE_CHECKLIST, "CCheckList::AddItem");
    TraceAssert(hWnd != NULL);
    TraceAssert(pszLabel != NULL && !IsBadStringPtr(pszLabel, MAX_PATH));

    // If this is the first item, get column descriptions
    if (0 == m_cItems)
        GetColumnDescriptions(hWnd);

    // Calculate the position of the new static label
    rc = m_rcItemLabel;
    cyOffset = m_cItems * m_rcItemLabel.bottom;
    OffsetRect(&rc, 0, cyOffset);

    // Create a new label control
    hwndNew = CreateWindowEx(WS_EX_NOPARENTNOTIFY,
                             c_szStaticClass,
                             pszLabel,
                             WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP | SS_NOPREFIX,// | WS_GROUP,
                             rc.left,
                             rc.top,
                             rc.right - rc.left,
                             rc.bottom - rc.top,
                             hWnd,
                             (HMENU)MAKE_LABEL_ID(m_cItems),
                             hModule,
                             NULL);
    if (!hwndNew)
        TraceLeaveValue(-1);

    // Save item data
    SetWindowLongPtr(hwndNew, GWLP_USERDATA, lParam);

    // Set the font
    SendMessage(hwndNew,
                WM_SETFONT,
                SendMessage(GetParent(hWnd), WM_GETFONT, 0, 0),
                0);

    // Set Z-order position just after the last checkbox. This keeps
    // tab order correct.
    if (m_cItems > 0)
    {
        hwndPrev = GetDlgItem(hWnd, MAKE_CTRL_ID(m_cItems - 1, m_cSubItems));
        SetWindowPos(hwndNew,
                     hwndPrev,
                     0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }

    // Create new checkboxes
    for (LONG j = 0; j < m_cSubItems; j++)
    {
        // Build window text for the control. The text is
        // hidden, but used for accessibility. (341042)
        LPCTSTR pszCheckText = pszLabel;
        LPTSTR pszT = NULL;
        if (m_pszColumnDesc[j] &&
            FormatStringID(&pszT, hModule, IDS_FMT_CHECKLABEL, pszLabel, m_pszColumnDesc[j]))
        {
            pszCheckText = pszT;
        }

        hwndPrev = hwndNew;
        hwndNew = CreateWindowEx(WS_EX_NOPARENTNOTIFY,
                                 c_szButtonClass,
                                 pszCheckText,
                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_NOTIFY | BS_FLAT | BS_AUTOCHECKBOX,
                                 m_nCheckPos[j],
                                 rc.top,
                                 m_cxCheckBox,
                                 rc.bottom - rc.top,
                                 hWnd,
                                 (HMENU)MAKE_CTRL_ID(m_cItems, j + 1),
                                 hModule,
                                 NULL);

        LocalFreeString(&pszT);

        if (!hwndNew)
        {
            while (j >= 0)
            {
                DestroyWindow(GetDlgItem(hWnd, MAKE_CTRL_ID(m_cItems, j)));
                j--;
            }

            TraceLeaveValue(-1);
        }

        SetWindowPos(hwndNew,
                     hwndPrev,
                     0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }

    // We now officially have a new item
    m_cItems++;

    //
    // The last thing is to calculate the scroll range
    //
    LONG nBottom = rc.bottom;
    GetClientRect(hWnd, &rc);

    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE;
    si.nMin = 0;
    si.nMax = nBottom + m_rcItemLabel.top - 1;
    si.nPage = rc.bottom; // ^^^^^^^^^ extra space

    SetScrollInfo(hWnd, SB_VERT, &si, FALSE);

    TraceLeaveValue(m_cItems - 1);  // return the index of the new item
}


void
CCheckList::SetState(HWND hWnd, WORD iItem, WORD iSubItem, LONG lState)
{
    TraceEnter(TRACE_CHECKLIST, "CCheckList::SetState");
    TraceAssert(hWnd != NULL);
    TraceAssert(iItem < m_cItems);
    TraceAssert(0 < iSubItem && iSubItem <= m_cSubItems);

    HWND hwndCtrl = GetDlgItem(hWnd, MAKE_CTRL_ID(iItem, iSubItem));
    if (hwndCtrl != NULL)
    {
        SendMessage(hwndCtrl, BM_SETCHECK, lState & CLST_CHECKED, 0);
        EnableWindow(hwndCtrl, !(lState & CLST_DISABLED));
    }

    TraceLeaveVoid();
}


LONG
CCheckList::GetState(HWND hWnd, WORD iItem, WORD iSubItem)
{
    LONG lState = 0;

    TraceEnter(TRACE_CHECKLIST, "CCheckList::GetState");
    TraceAssert(hWnd != NULL);
    TraceAssert(iItem < m_cItems);
    TraceAssert(0 < iSubItem && iSubItem <= m_cSubItems);

    HWND hwndCtrl = GetDlgItem(hWnd, MAKE_CTRL_ID(iItem, iSubItem));

    if (hwndCtrl != NULL)
    {
        lState = (LONG)SendMessage(hwndCtrl, BM_GETCHECK, 0, 0);
        TraceAssert(!(lState & BST_INDETERMINATE));

        if (!IsWindowEnabled(hwndCtrl))
            lState |= CLST_DISABLED;
    }

    TraceLeaveValue(lState);
}


void
CCheckList::SetColumnWidth(HWND hWnd, LONG cxDialog, LONG cxColumn)
{
    LONG j;

    TraceEnter(TRACE_CHECKLIST, "CCheckList::SetColumnWidth");
    TraceAssert(hWnd != NULL);
    TraceAssert(cxColumn > 10);

    m_cxCheckColumn = cxColumn;

    if (m_cSubItems > 0)
    {
        m_nCheckPos[m_cSubItems-1] = cxDialog                       // dlg width
                                    - m_rcItemLabel.left            // right margin
                                    - (cxColumn + m_cxCheckBox)/2;  // 1/2 col & 1/2 checkbox

        for (j = m_cSubItems - 1; j > 0; j--)
            m_nCheckPos[j-1] = m_nCheckPos[j] - cxColumn;

        //              (leftmost check pos) - (horz margin)
        m_rcItemLabel.right = m_nCheckPos[0] - m_rcItemLabel.left;
    }
    else
        m_rcItemLabel.right = cxDialog - m_rcItemLabel.left;

    LONG nTop = m_rcItemLabel.top;
    LONG nBottom = m_rcItemLabel.bottom;

    for (LONG i = 0; i < m_cItems; i++)
    {
        MoveWindow(GetDlgItem(hWnd, MAKE_LABEL_ID(i)),
                   m_rcItemLabel.left,
                   nTop,
                   m_rcItemLabel.right - m_rcItemLabel.left,
                   nBottom - nTop,
                   FALSE);

        for (j = 0; j < m_cSubItems; j++)
        {
            MoveWindow(GetDlgItem(hWnd, MAKE_CTRL_ID(i, j + 1)),
                       m_nCheckPos[j],
                       nTop,
                       m_cxCheckBox,
                       nBottom - nTop,
                       FALSE);
        }

        nTop += m_rcItemLabel.bottom;
        nBottom += m_rcItemLabel.bottom;
    }

    TraceLeaveVoid();
}


void
CCheckList::ResetContent(HWND hWnd)
{
    for (LONG i = 0; i < m_cItems; i++)
        for (LONG j = 0; j <= m_cSubItems; j++)
            DestroyWindow(GetDlgItem(hWnd, MAKE_CTRL_ID(i, j)));

    // Hide the scroll bar
    SetScrollRange(hWnd, SB_VERT, 0, 0, FALSE);
    m_cItems = 0;
}


LONG
CCheckList::GetVisibleCount(HWND hWnd)
{
    LONG nCount = 1;
    RECT rc;

    if (GetClientRect(hWnd, &rc) && m_rcItemLabel.bottom > 0)
        nCount = max(1, rc.bottom / m_rcItemLabel.bottom);

    return nCount;
}


LONG
CCheckList::GetTopIndex(HWND hWnd)
{
    LONG nIndex = 0;
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_POS;

    if (GetScrollInfo(hWnd, SB_VERT, &si) && m_rcItemLabel.bottom > 0)
        nIndex = max(0, si.nPos / m_rcItemLabel.bottom);

    return nIndex;
}


void
CCheckList::EnsureVisible(HWND hWnd, LONG nItemIndex)
{
    LONG nTopIndex = GetTopIndex(hWnd);

    // Note that the top item may only be partially visible,
    // so we need to test for equality here.  Raid #208449
    if (nItemIndex <= nTopIndex)
    {
        SetTopIndex(hWnd, nItemIndex);
    }
    else
    {
        LONG nVisible = GetVisibleCount(hWnd);

        if (nItemIndex >= nTopIndex + nVisible)
            SetTopIndex(hWnd, nItemIndex - nVisible + 1);
    }
}


void
CCheckList::DrawCheckFocusRect(HWND hWnd, HWND hwndCheck, BOOL fDraw)
{
    RECT rcCheck;
    HDC hdc;

    TraceEnter(TRACE_CHECKLIST, "CCheckList::DrawCheckFocusRect");
    TraceAssert(hWnd != NULL);
    TraceAssert(hwndCheck != NULL);

    GetWindowRect(hwndCheck, &rcCheck);
    MapWindowPoints(NULL, hWnd, (LPPOINT)&rcCheck, 2);
    InflateRect(&rcCheck, 2, 2);    // draw *outside* the checkbox

    hdc = GetDC(hWnd);
    if (hdc)
    {
        // Always erase before drawing, since we may already be
        // partially visible and drawing is an XOR operation.
        // (Don't want to leave any turds on the screen.)

        FrameRect(hdc, &rcCheck, GetSysColorBrush(BK_COLOR));

        if (fDraw)
        {
            SetTextColor(hdc, GetSysColor(TEXT_COLOR));
            SetBkColor(hdc, GetSysColor(BK_COLOR));
            DrawFocusRect(hdc, &rcCheck);
        }

        ReleaseDC(hWnd, hdc);
    }

    TraceLeaveVoid();
}


void
CCheckList::GetColumnDescriptions(HWND hWnd)
{
    //
    // Get column descriptions for accessibility
    //
    TCHAR szDescription[MAX_PATH];
    NM_CHECKLIST nmc;
    nmc.hdr.hwndFrom = hWnd;
    nmc.hdr.idFrom = GetDlgCtrlID(hWnd);
    nmc.hdr.code = CLN_GETCOLUMNDESC;
    nmc.iItem = 0;
    nmc.dwState = 0;
    nmc.dwItemData = 0;
    nmc.cchTextMax = ARRAYSIZE(szDescription);
    nmc.pszText = szDescription;

    for (LONG j = 0; j < m_cSubItems; j++)
    {
        szDescription[0] = TEXT('\0');
        nmc.iSubItem = j+1;

        SendMessage(GetParent(hWnd),
                    WM_NOTIFY,
                    nmc.hdr.idFrom,
                    (LPARAM)&nmc);

        LocalFreeString(&m_pszColumnDesc[j]);
        if (szDescription[0])
            LocalAllocString(&m_pszColumnDesc[j], szDescription);
    }
}


LRESULT
CALLBACK
CCheckList::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;
    CCheckList *pThis = (CCheckList*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    TraceEnter(TRACE_CHECKLIST, "CCheckList::WindowProc");
    TraceAssert(hWnd != NULL);

    switch (uMsg)
    {
    case WM_NCCREATE:
        pThis = new CCheckList(hWnd, (LPCREATESTRUCT)lParam);
        if (pThis != NULL)
        {
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
            lResult = TRUE;
        }
        break;

    case WM_NCDESTROY:
        delete pThis;
        break;

    case WM_COMMAND:
        TraceAssert(pThis != NULL);
        lResult = pThis->MsgCommand(hWnd,
                                    GET_WM_COMMAND_ID(wParam, lParam),
                                    GET_WM_COMMAND_CMD(wParam, lParam),
                                    GET_WM_COMMAND_HWND(wParam, lParam));
        break;

    case WM_CTLCOLORSTATIC:
        TraceAssert(pThis != NULL);
        SetBkMode((HDC)wParam, TRANSPARENT);
        SetTextColor((HDC)wParam, GetSysColor(TEXT_COLOR));
        SetBkColor((HDC)wParam, GetSysColor(BK_COLOR));
        lResult = (LRESULT)GetSysColorBrush(BK_COLOR);
        break;

    case WM_PAINT:
        TraceAssert(pThis != NULL);
        pThis->MsgPaint(hWnd, (HDC)wParam);
        break;

    case WM_VSCROLL:
        TraceAssert(pThis != NULL);
        pThis->MsgVScroll(hWnd,
                          (int)(short)GET_WM_VSCROLL_CODE(wParam, lParam),
                          (int)(short)GET_WM_VSCROLL_POS(wParam, lParam));
        break;

    case WM_MOUSEWHEEL:
        TraceAssert(pThis != NULL);
        pThis->MsgMouseWheel(hWnd,
                             LOWORD(wParam),
                             (int)(short)HIWORD(wParam));
        break;

    case WM_LBUTTONDOWN:
        TraceAssert(pThis != NULL);
        pThis->MsgButtonDown(hWnd,
                             wParam,
                             (int)(short)LOWORD(lParam),
                             (int)(short)HIWORD(lParam));
        break;

    case WM_ENABLE:
        TraceAssert(pThis != NULL);
        pThis->MsgEnable(hWnd, (BOOL)wParam);
        break;

    case WM_SETFONT:
        TraceAssert(pThis != NULL);
        {
            for (LONG i = 0; i < pThis->m_cItems; i++)
                SendDlgItemMessage(hWnd,
                                   MAKE_LABEL_ID(i),
                                   WM_SETFONT,
                                   wParam,
                                   lParam);
        }
        break;

    case WM_SIZE:
        TraceAssert(pThis != NULL);
        pThis->MsgSize(hWnd, (DWORD)wParam, LOWORD(lParam), HIWORD(lParam));
        break;

    case CLM_ADDITEM:
        TraceAssert(pThis != NULL);
        lResult = pThis->AddItem(hWnd, (LPCTSTR)wParam, lParam);
        break;

    case CLM_GETITEMCOUNT:
        TraceAssert(pThis != NULL);
        lResult = pThis->m_cItems;
        break;

    case CLM_SETSTATE:
        TraceAssert(pThis != NULL);
        pThis->SetState(hWnd, LOWORD(wParam), HIWORD(wParam), (LONG)lParam);
        break;

    case CLM_GETSTATE:
        TraceAssert(pThis != NULL);
        lResult = pThis->GetState(hWnd, LOWORD(wParam), HIWORD(wParam));
        break;

    case CLM_SETCOLUMNWIDTH:
        TraceAssert(pThis != NULL);
        {
            RECT rc;
            LONG cxDialog;

            GetClientRect(hWnd, &rc);
            cxDialog = rc.right;

            rc.right = (LONG)lParam;
            MapDialogRect(GetParent(hWnd), &rc);

            pThis->SetColumnWidth(hWnd, cxDialog, rc.right);
        }
        break;

    case CLM_SETITEMDATA:
        TraceAssert(GET_ITEM(wParam) < (ULONG)pThis->m_cItems);
        SetWindowLongPtr(GetDlgItem(hWnd, MAKE_LABEL_ID((int)wParam)),
                         GWLP_USERDATA,
                         lParam);
        break;

    case CLM_GETITEMDATA:
        TraceAssert(GET_ITEM(wParam) < (ULONG)pThis->m_cItems);
        lResult = GetWindowLongPtr(GetDlgItem(hWnd, MAKE_LABEL_ID((int)wParam)),
                                   GWLP_USERDATA);
        break;

    case CLM_RESETCONTENT:
        TraceAssert(pThis != NULL);
        pThis->ResetContent(hWnd);
        break;

    case CLM_GETVISIBLECOUNT:
        TraceAssert(pThis != NULL);
        lResult = pThis->GetVisibleCount(hWnd);
        break;

    case CLM_GETTOPINDEX:
        TraceAssert(pThis != NULL);
        lResult = pThis->GetTopIndex(hWnd);
        break;

    case CLM_SETTOPINDEX:
        TraceAssert(pThis != NULL);
        pThis->SetTopIndex(hWnd, (LONG)wParam);
        break;

    case CLM_ENSUREVISIBLE:
        TraceAssert(pThis != NULL);
        pThis->EnsureVisible(hWnd, (LONG)wParam);
        break;

    //
    // Always refer to the chklist window for help. Don't pass
    // one of the child window handles here.
    //
    case WM_HELP:
        ((LPHELPINFO)lParam)->hItemHandle = hWnd;
        lResult = SendMessage(GetParent(hWnd), uMsg, wParam, lParam);
        break;
    case WM_CONTEXTMENU:
        lResult = SendMessage(GetParent(hWnd), uMsg, (WPARAM)hWnd, lParam);
        break;

    default:
        lResult = DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    TraceLeaveValue(lResult);
}
