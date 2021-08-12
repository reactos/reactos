
#pragma once

class CListView: public CWindow
{
public:

    HWND Create(HWND hWndParent, _U_RECT rect, LPCTSTR szWindowName = NULL, DWORD dwStyle = 0,
                    DWORD dwExStyle = 0, _U_MENUorID MenuOrID = 0U, LPVOID lpCreateParam = NULL)
    {
        m_hWnd = ::CreateWindowEx(dwExStyle,
                                  WC_LISTVIEW,
                                  szWindowName,
                                  dwStyle,
                                  rect.m_lpRect->left,
                                  rect.m_lpRect->top,
                                  rect.m_lpRect->right - rect.m_lpRect->left,
                                  rect.m_lpRect->bottom - rect.m_lpRect->top,
                                  hWndParent,
                                  MenuOrID.m_hMenu,
                                  _AtlBaseModule.GetModuleInstance(),
                                  lpCreateParam);

        return m_hWnd;
    }

    void SetRedraw(BOOL redraw)
    {
        SendMessage(WM_SETREDRAW, redraw);
    }

    BOOL SetTextBkColor(COLORREF cr)
    {
        return (BOOL)SendMessage(LVM_SETTEXTBKCOLOR, 0, cr);
    }

    BOOL SetBkColor(COLORREF cr)
    {
        return (BOOL)SendMessage(LVM_SETBKCOLOR, 0, cr);
    }

    BOOL SetTextColor(COLORREF cr)
    {
        return (BOOL)SendMessage(LVM_SETTEXTCOLOR, 0, cr);
    }

    DWORD SetExtendedListViewStyle(DWORD dw, DWORD dwMask = 0)
    {
        return (DWORD)SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, dwMask, dw);
    }

    int InsertColumn(int iCol, LV_COLUMN* pcol)
    {
        return (int)SendMessage(LVM_INSERTCOLUMN, iCol, reinterpret_cast<LPARAM>(pcol));
    }

    int InsertColumn(int iCol, LPWSTR pszText, int fmt, int width = -1, int iSubItem = -1, int iImage = -1, int iOrder = -1)
    {
        LV_COLUMN column = {0};
        column.mask = LVCF_TEXT|LVCF_FMT;
        column.pszText = pszText;
        column.fmt = fmt;
        if(width != -1)
        {
            column.mask |= LVCF_WIDTH;
            column.cx = width;
        }
        if(iSubItem != -1)
        {
            column.mask |= LVCF_SUBITEM;
            column.iSubItem = iSubItem;
        }
        if(iImage != -1)
        {
            column.mask |= LVCF_IMAGE;
            column.iImage = iImage;
        }
        if(iOrder != -1)
        {
            column.mask |= LVCF_ORDER;
            column.iOrder = iOrder;
        }
        return InsertColumn(iCol, &column);
    }

    int GetColumnWidth(int iCol)
    {
        return (int)SendMessage(LVM_GETCOLUMNWIDTH, iCol);
    }

    HIMAGELIST SetImageList(HIMAGELIST himl, int iImageList)
    {
        return (HIMAGELIST)SendMessage(LVM_SETIMAGELIST, iImageList, reinterpret_cast<LPARAM>(himl));
    }

    int InsertItem(const LV_ITEM * pitem)
    {
        return (int)SendMessage(LVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(pitem));
    }

    BOOL DeleteItem(int i)
    {
        return (BOOL)SendMessage(LVM_DELETEITEM, i, 0);
    }

    BOOL GetItem(LV_ITEM* pitem)
    {
        return (BOOL)SendMessage(LVM_GETITEM, 0, reinterpret_cast<LPARAM>(pitem));
    }

    BOOL SetItem(const LV_ITEM * pitem)
    {
        return (BOOL)SendMessage(LVM_SETITEM, 0, reinterpret_cast<LPARAM>(pitem));
    }

    BOOL FindItem(int iStart, const LV_FINDINFO * plvfi)
    {
        return (BOOL)SendMessage(LVM_FINDITEM, iStart, (LPARAM) plvfi);
    }

    int GetItemCount()
    {
        return SendMessage(LVM_GETITEMCOUNT);
    }

    BOOL DeleteAllItems()
    {
        return (BOOL)SendMessage(LVM_DELETEALLITEMS);
    }

    BOOL Update(int i)
    {
        return (BOOL)SendMessage(LVM_UPDATE, i, 0);
    }

    UINT GetSelectedCount()
    {
        return (UINT)SendMessage(LVM_GETSELECTEDCOUNT);
    }

    BOOL SortItems(PFNLVCOMPARE pfnCompare, PVOID lParam)
    {
        return (BOOL)SendMessage(LVM_SORTITEMS, (WPARAM)lParam, (LPARAM) pfnCompare);
    }

    BOOL EnsureVisible(int i, BOOL fPartialOK)
    {
        return (BOOL)SendMessage(LVM_ENSUREVISIBLE, i, MAKELPARAM((fPartialOK),0));
    }

    HWND EditLabel(int i)
    {
        return (HWND)SendMessage(LVM_EDITLABEL, i, 0);
    }

    int GetSelectionMark()
    {
        return (int)SendMessage(LVM_GETSELECTIONMARK);
    }

    int GetNextItem(int i, WORD flags)
    {
        return (int)SendMessage(LVM_GETNEXTITEM, i, MAKELPARAM((flags),0));
    }

    void GetItemSpacing(SIZE& spacing, BOOL bSmallIconView = FALSE)
    {
        DWORD ret = SendMessage(LVM_GETITEMSPACING, bSmallIconView);
        spacing.cx = LOWORD(ret);
        spacing.cy = HIWORD(ret);
    }

    UINT GetItemState(int i, UINT mask)
    {
        return SendMessage(LVM_GETITEMSTATE, i, (LPARAM)mask);
    }

    void SetItemState(int i, UINT state, UINT mask)
    {
        LV_ITEM item;
        item.stateMask = mask;
        item.state = state;
        SendMessage(LVM_SETITEMSTATE, i, reinterpret_cast<LPARAM>(&item));
    }

    BOOL SetItemText(int i, int subItem, LPCWSTR text)
    {
        LVITEMW item;
        item.iSubItem = subItem;
        item.pszText = (LPWSTR)text;
        return SendMessage(LVM_SETITEMTEXT, i, (LPARAM)&item);
    }

    void SetCheckState(int i, BOOL check)
    {
        SetItemState(i, INDEXTOSTATEIMAGEMASK((check)?2:1), LVIS_STATEIMAGEMASK);
    }

    int HitTest(LV_HITTESTINFO * phtInfo)
    {
        return (int)SendMessage(LVM_HITTEST, 0, reinterpret_cast<LPARAM>(phtInfo));
    }

    DWORD_PTR GetItemData(int i)
    {
        LVITEMW lvItem = { 0 };
        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = i;
        BOOL ret = GetItem(&lvItem);
        return (DWORD_PTR)(ret ? lvItem.lParam : NULL);
    }

    BOOL GetSelectedItem(LV_ITEM* pItem)
    {
        pItem->iItem = GetNextItem(-1, LVNI_ALL | LVNI_SELECTED);
        if (pItem->iItem == -1)
            return FALSE;
        return GetItem(pItem);
    }

    void GetItemText(int iItem, int iSubItem, LPTSTR pszText, int cchTextMax)
    {
        LV_ITEM itemInfo;
        itemInfo.iSubItem = iSubItem;
        itemInfo.pszText = pszText;
        itemInfo.cchTextMax = cchTextMax;

        SendMessage(LVM_GETITEMTEXT, iItem, (LPARAM) &itemInfo);
    }

    BOOL GetItemPosition(int nItem, POINT* pPoint)
    {
        return (BOOL)SendMessage(LVM_GETITEMPOSITION, nItem, (LPARAM)pPoint);
    }

    BOOL SetItemPosition(int nItem, POINT* pPoint)
    {
        return (BOOL)SendMessage(LVM_SETITEMPOSITION, nItem, MAKELPARAM(pPoint->x, pPoint->y));
    }

    BOOL Arrange(UINT nCode)
    {
        return (BOOL)SendMessage(LVM_ARRANGE, nCode, 0);
    }

};

template<typename TItemData = DWORD_PTR>
class CToolbar :
    public CWindow
{
public: // Configuration methods

    // Hack:
    // Use DECLARE_WND_SUPERCLASS instead!
    HWND Create(HWND hWndParent, DWORD dwStyles = 0, DWORD dwExStyles = 0)
    {
        if (!dwStyles)
        {
            dwStyles = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN;
        }

        if (!dwExStyles)
        {
            dwExStyles = WS_EX_TOOLWINDOW;
        }

        m_hWnd = CreateWindowExW(dwExStyles,
                                TOOLBARCLASSNAME,
                                NULL,
                                dwStyles,
                                0, 0, 0, 0, hWndParent,
                                NULL,
                                _AtlBaseModule.GetModuleInstance(),
                                NULL);

        if (!m_hWnd)
            return NULL;

        /* Identify the version we're using */
        SetButtonStructSize();

        return m_hWnd;
    }

    DWORD SetButtonStructSize()
    {
        return SendMessageW(TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    }

    HWND GetTooltip()
    {
        return (HWND)SendMessageW(TB_GETTOOLTIPS);
    }

    DWORD SetTooltip(HWND hWndTooltip)
    {
        return SendMessageW(TB_SETTOOLTIPS, reinterpret_cast<WPARAM>(hWndTooltip), 0);
    }

    INT GetHotItem()
    {
        return SendMessageW(TB_GETHOTITEM);
    }

    DWORD SetHotItem(INT item)
    {
        return SendMessageW(TB_SETHOTITEM, item);
    }

    DWORD SetDrawTextFlags(DWORD useBits, DWORD bitState)
    {
        return SendMessageW(TB_SETDRAWTEXTFLAGS, useBits, bitState);
    }

public: // Button list management methods
    int GetButtonCount()
    {
        return SendMessageW(TB_BUTTONCOUNT);
    }

    DWORD GetButton(int index, TBBUTTON * btn)
    {
        return SendMessageW(TB_GETBUTTON, index, reinterpret_cast<LPARAM>(btn));
    }

    DWORD AddButton(TBBUTTON * btn)
    {
        return SendMessageW(TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(btn));
    }

    DWORD AddButtons(int count, TBBUTTON * buttons)
    {
        return SendMessageW(TB_ADDBUTTONS, count, reinterpret_cast<LPARAM>(buttons));
    }

    DWORD InsertButton(int insertAt, TBBUTTON * btn)
    {
        return SendMessageW(TB_INSERTBUTTON, insertAt, reinterpret_cast<LPARAM>(btn));
    }

    DWORD MoveButton(int oldIndex, int newIndex)
    {
        return SendMessageW(TB_MOVEBUTTON, oldIndex, newIndex);
    }

    DWORD DeleteButton(int index)
    {
        return SendMessageW(TB_DELETEBUTTON, index, 0);
    }

    DWORD GetButtonInfo(int cmdId, TBBUTTONINFO * info)
    {
        return SendMessageW(TB_GETBUTTONINFO, cmdId, reinterpret_cast<LPARAM>(info));
    }

    DWORD SetButtonInfo(int cmdId, TBBUTTONINFO * info)
    {
        return SendMessageW(TB_SETBUTTONINFO, cmdId, reinterpret_cast<LPARAM>(info));
    }

    DWORD CheckButton(int cmdId, BOOL bCheck)
    {
        return SendMessageW(TB_CHECKBUTTON, cmdId, MAKELPARAM(bCheck, 0));
    }

public: // Layout management methods
    DWORD GetButtonSize()
    {
        return SendMessageW(TB_GETBUTTONSIZE);
    }

    DWORD SetButtonSize(int w, int h)
    {
        return SendMessageW(TB_SETBUTTONSIZE, 0, MAKELPARAM(w, h));
    }

    DWORD AutoSize()
    {
        return SendMessageW(TB_AUTOSIZE);
    }

    DWORD GetMaxSize(LPSIZE size)
    {
        return SendMessageW(TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(size));
    }

    DWORD GetIdealSize(BOOL useHeight, LPSIZE size)
    {
        return SendMessageW(TB_GETIDEALSIZE, useHeight, reinterpret_cast<LPARAM>(size));
    }

    DWORD GetMetrics(TBMETRICS * tbm)
    {
        return SendMessageW(TB_GETMETRICS, 0, reinterpret_cast<LPARAM>(tbm));
    }

    DWORD SetMetrics(TBMETRICS * tbm)
    {
        return SendMessageW(TB_SETMETRICS, 0, reinterpret_cast<LPARAM>(tbm));
    }

    DWORD GetItemRect(int index, LPRECT prcItem)
    {
        return SendMessageW(TB_GETITEMRECT, index, reinterpret_cast<LPARAM>(prcItem));
    }

    DWORD SetRedraw(BOOL bEnable)
    {
        return SendMessageW(WM_SETREDRAW, bEnable);
    }

    DWORD GetPadding()
    {
        return SendMessageW(TB_GETPADDING);
    }

    DWORD SetPadding(int x, int y)
    {
        return SendMessageW(TB_SETPADDING, 0, MAKELPARAM(x, y));
    }

public: // Image list management methods
    HIMAGELIST SetImageList(HIMAGELIST himl)
    {
        return (HIMAGELIST)SendMessageW(TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(himl));
    }

public: // Other methods
    INT HitTest(PPOINT ppt)
    {
        return (INT) SendMessageW(TB_HITTEST, 0, reinterpret_cast<LPARAM>(ppt));
    }

public: // Utility methods
    TItemData * GetItemData(int index)
    {
        TBBUTTON btn;
        GetButton(index, &btn);
        return (TItemData*) btn.dwData;
    }

    DWORD SetItemData(int index, TItemData * data)
    {
        TBBUTTONINFOW info = { 0 };
        info.cbSize = sizeof(info);
        info.dwMask = TBIF_BYINDEX | TBIF_LPARAM;
        info.lParam = (DWORD_PTR) data;
        return SetButtonInfo(index, &info);
    }
};

class CStatusBar :
    public CWindow
{
public:
    VOID SetText(LPCWSTR lpszText)
    {
        SendMessage(SB_SETTEXT, SBT_NOBORDERS, (LPARAM) lpszText);
    }

    HWND Create(HWND hwndParent, HMENU hMenu)
    {
        m_hWnd = CreateWindowExW(0,
            STATUSCLASSNAMEW,
            NULL,
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0,
            hwndParent,
            hMenu,
            _AtlBaseModule.GetModuleInstance(),
            NULL);

        return m_hWnd;
    }

};

class CTreeView :
    public CWindow
{
public:
    HWND Create(HWND hwndParent)
    {
        m_hWnd = CreateWindowExW(WS_EX_CLIENTEDGE,
            WC_TREEVIEWW,
            L"",
            WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_SHOWSELALWAYS,
            0, 28, 200, 350,
            hwndParent,
            NULL,
            _AtlBaseModule.GetModuleInstance(),
            NULL);

        return m_hWnd;
    }

    HTREEITEM AddItem(HTREEITEM hParent, LPWSTR lpText, INT Image, INT SelectedImage, LPARAM lParam)
    {
        TVINSERTSTRUCTW Insert;

        ZeroMemory(&Insert, sizeof(TV_INSERTSTRUCT));

        Insert.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        Insert.hInsertAfter = TVI_LAST;
        Insert.hParent = hParent;
        Insert.item.iSelectedImage = SelectedImage;
        Insert.item.iImage = Image;
        Insert.item.lParam = lParam;
        Insert.item.pszText = lpText;

        return InsertItem(&Insert);
    }

    void SetRedraw(BOOL redraw)
    {
        SendMessage(WM_SETREDRAW, redraw);
    }

    BOOL SetBkColor(COLORREF cr)
    {
        return (BOOL) SendMessage(TVM_SETBKCOLOR, 0, cr);
    }

    BOOL SetTextColor(COLORREF cr)
    {
        return (BOOL) SendMessage(TVM_SETTEXTCOLOR, 0, cr);
    }

    HIMAGELIST SetImageList(HIMAGELIST himl, int iImageList)
    {
        return (HIMAGELIST) SendMessage(TVM_SETIMAGELIST, iImageList, reinterpret_cast<LPARAM>(himl));
    }

    HTREEITEM InsertItem(const TVINSERTSTRUCTW * pitem)
    {
        return (HTREEITEM) SendMessage(TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(pitem));
    }

    BOOL DeleteItem(HTREEITEM i)
    {
        return (BOOL) SendMessage(TVM_DELETEITEM, 0, (LPARAM)i);
    }

    BOOL GetItem(TV_ITEM* pitem)
    {
        return (BOOL) SendMessage(TVM_GETITEM, 0, reinterpret_cast<LPARAM>(pitem));
    }

    BOOL SetItem(const TV_ITEM * pitem)
    {
        return (BOOL) SendMessage(TVM_SETITEM, 0, reinterpret_cast<LPARAM>(pitem));
    }

    int GetItemCount()
    {
        return SendMessage(TVM_GETCOUNT);
    }

    BOOL EnsureVisible(HTREEITEM i)
    {
        return (BOOL) SendMessage(TVM_ENSUREVISIBLE, 0, (LPARAM)i);
    }

    HWND EditLabel(HTREEITEM i)
    {
        return (HWND) SendMessage(TVM_EDITLABEL, 0, (LPARAM)i);
    }

    HTREEITEM GetNextItem(HTREEITEM i, WORD flags)
    {
        return (HTREEITEM)SendMessage(TVM_GETNEXTITEM, flags, (LPARAM)i);
    }

    UINT GetItemState(int i, UINT mask)
    {
        return SendMessage(TVM_GETITEMSTATE, i, (LPARAM) mask);
    }

    HTREEITEM HitTest(TVHITTESTINFO * phtInfo)
    {
        return (HTREEITEM) SendMessage(TVM_HITTEST, 0, reinterpret_cast<LPARAM>(phtInfo));
    }

    DWORD_PTR GetItemData(HTREEITEM item)
    {
        TVITEMW lvItem;
        lvItem.hItem = item;
        lvItem.mask = TVIF_PARAM;
        BOOL ret = GetItem(&lvItem);
        return (DWORD_PTR) (ret ? lvItem.lParam : NULL);
    }

    HTREEITEM GetSelection()
    {
        return GetNextItem(NULL, TVGN_CARET);
    }

    BOOL Expand(HTREEITEM item, DWORD action)
    {
        return SendMessage(TVM_EXPAND, action, (LPARAM)item);
    }

    BOOL SelectItem(HTREEITEM item, DWORD action = TVGN_CARET)
    {
        return SendMessage(TVM_SELECTITEM, action, (LPARAM) item);
    }

};

class CTooltips :
    public CWindow
{
public: // Configuration methods

    HWND Create(HWND hWndParent, DWORD dwStyles = WS_POPUP | TTS_NOPREFIX, DWORD dwExStyles = WS_EX_TOPMOST)
    {
        RECT r = { 0 };
        return CWindow::Create(TOOLTIPS_CLASS, hWndParent, r, L"", dwStyles, dwExStyles);
    }

public: // Relay event

    // Win7+: Can use GetMessageExtraInfo to provide the WPARAM value.
    VOID RelayEvent(MSG * pMsg, WPARAM extraInfo = 0)
    {
        SendMessageW(TTM_RELAYEVENT, extraInfo, reinterpret_cast<LPARAM>(pMsg));
    }

public: // Helpers

    INT GetToolCount()
    {
        return SendMessageW(TTM_GETTOOLCOUNT, 0, 0);
    }

    BOOL AddTool(IN CONST TTTOOLINFOW * pInfo)
    {
        return SendMessageW(TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(pInfo));
    }

    VOID DelTool(IN HWND hwndToolOwner, IN UINT uId)
    {
        TTTOOLINFOW info = { sizeof(TTTOOLINFOW), 0 };
        info.hwnd = hwndToolOwner;
        info.uId = uId;
        SendMessageW(TTM_DELTOOL, 0, reinterpret_cast<LPARAM>(&info));
    }

    VOID NewToolRect(IN HWND hwndToolOwner, IN UINT uId, IN RECT rect)
    {
        TTTOOLINFOW info = { sizeof(TTTOOLINFOW), 0 };
        info.hwnd = hwndToolOwner;
        info.uId = uId;
        info.rect = rect;
        SendMessageW(TTM_NEWTOOLRECT, 0, reinterpret_cast<LPARAM>(&info));
    }

    BOOL GetToolInfo(IN HWND hwndToolOwner, IN UINT uId, IN OUT TTTOOLINFOW * pInfo)
    {
        pInfo->hwnd = hwndToolOwner;
        pInfo->uId = uId;
        return SendMessageW(TTM_GETTOOLINFO, 0, reinterpret_cast<LPARAM>(pInfo));
    }

    VOID SetToolInfo(IN CONST TTTOOLINFOW * pInfo)
    {
        SendMessageW(TTM_SETTOOLINFO, 0, reinterpret_cast<LPARAM>(pInfo));
    }

    BOOL HitTest(IN CONST TTHITTESTINFOW * pInfo)
    {
        return SendMessageW(TTM_HITTEST, 0, reinterpret_cast<LPARAM>(pInfo));
    }

    VOID GetText(IN HWND hwndToolOwner, IN UINT uId, OUT PWSTR pBuffer, IN DWORD cchBuffer)
    {
        TTTOOLINFOW info = { sizeof(TTTOOLINFOW), 0 };
        info.hwnd = hwndToolOwner;
        info.uId = uId;
        info.lpszText = pBuffer;
        SendMessageW(TTM_GETTEXT, cchBuffer, reinterpret_cast<LPARAM>(&info));
    }

    VOID UpdateTipText(IN HWND hwndToolOwner, IN UINT uId, IN PCWSTR szText, IN HINSTANCE hinstResourceOwner = NULL)
    {
        TTTOOLINFOW info = { sizeof(TTTOOLINFOW), 0 };
        info.hwnd = hwndToolOwner;
        info.uId = uId;
        info.lpszText = const_cast<PWSTR>(szText);
        info.hinst = hinstResourceOwner;
        SendMessageW(TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&info));
    }

    BOOL EnumTools(IN CONST TTTOOLINFOW * pInfo)
    {
        return SendMessageW(TTM_ENUMTOOLS, 0, reinterpret_cast<LPARAM>(pInfo));
    }

    BOOL GetCurrentTool(OUT OPTIONAL TTTOOLINFOW * pInfo = NULL)
    {
        return SendMessageW(TTM_GETCURRENTTOOL, 0, reinterpret_cast<LPARAM>(pInfo));
    }

    VOID GetTitle(TTGETTITLE * pTitleInfo)
    {
        SendMessageW(TTM_GETTITLE, 0, reinterpret_cast<LPARAM>(pTitleInfo));
    }

    BOOL SetTitle(PCWSTR szTitleText, WPARAM icon = 0)
    {
        return SendMessageW(TTM_SETTITLE, icon, reinterpret_cast<LPARAM>(szTitleText));
    }

    VOID TrackActivate(IN HWND hwndToolOwner, IN UINT uId)
    {
        TTTOOLINFOW info = { sizeof(TTTOOLINFOW), 0 };
        info.hwnd = hwndToolOwner;
        info.uId = uId;
        SendMessageW(TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(&info));
    }

    VOID TrackDeactivate()
    {
        SendMessageW(TTM_TRACKACTIVATE, FALSE, NULL);
    }

    VOID TrackPosition(IN WORD x, IN WORD y)
    {
        SendMessageW(TTM_TRACKPOSITION, 0, MAKELPARAM(x, y));
    }

    // Opens the tooltip
    VOID Popup()
    {
        SendMessageW(TTM_POPUP);
    }

    // Closes the tooltip - Pressing the [X] for a TTF_CLOSE balloon is equivalent to calling this
    VOID Pop()
    {
        SendMessageW(TTM_POP);
    }

    // Delay times for AUTOMATIC tooltips (they don't affect balloons)
    INT GetDelayTime(UINT which)
    {
        return SendMessageW(TTM_GETDELAYTIME, which);
    }

    VOID SetDelayTime(UINT which, WORD time)
    {
        SendMessageW(TTM_SETDELAYTIME, which, MAKELPARAM(time, 0));
    }

    // Activates or deactivates the automatic tooltip display when hovering a control
    VOID Activate(IN BOOL bActivate = TRUE)
    {
        SendMessageW(TTM_ACTIVATE, bActivate);
    }

    // Adjusts the position of a tooltip when used to display trimmed text
    VOID AdjustRect(IN BOOL bTextToWindow, IN OUT RECT * pRect)
    {
        SendMessageW(TTM_ADJUSTRECT, bTextToWindow, reinterpret_cast<LPARAM>(pRect));
    }

    // Useful for TTF_ABSOLUTE|TTF_TRACK tooltip positioning
    SIZE GetBubbleSize(IN TTTOOLINFOW * pInfo)
    {
        DWORD ret = SendMessageW(TTM_GETBUBBLESIZE, 0, reinterpret_cast<LPARAM>(pInfo));
        const SIZE sz = { LOWORD(ret), HIWORD(ret) };
        return sz;
    }

    // Fills the RECT with the margin size previously set. Default is 0 margins.
    VOID GetMargin(OUT RECT * pRect)
    {
        SendMessageW(TTM_GETMARGIN, 0, reinterpret_cast<LPARAM>(pRect));
    }

    VOID SetMargin(IN RECT * pRect)
    {
        SendMessageW(TTM_SETMARGIN, 0, reinterpret_cast<LPARAM>(pRect));
    }

    // Gets a previously established max width. Returns -1 if no limit is set
    INT GetMaxTipWidth()
    {
        return SendMessageW(TTM_GETMAXTIPWIDTH);
    }

    INT SetMaxTipWidth(IN OPTIONAL INT width = -1)
    {
        return SendMessageW(TTM_SETMAXTIPWIDTH, 0, width);
    }

    // Get the color of the tooltip text
    COLORREF GetTipTextColor()
    {
        return SendMessageW(TTM_GETTIPTEXTCOLOR);
    }

    VOID SetTipTextColor(IN COLORREF textColor)
    {
        SendMessageW(TTM_SETTIPTEXTCOLOR, textColor);
    }

    COLORREF GetTipBkColor()
    {
        return SendMessageW(TTM_GETTIPBKCOLOR);
    }

    VOID SetTipBkColor(IN COLORREF textColor)
    {
        SendMessageW(TTM_SETTIPBKCOLOR, textColor);
    }

    VOID SetWindowTheme(IN PCWSTR szThemeName)
    {
        SendMessageW(TTM_SETWINDOWTHEME, 0, reinterpret_cast<LPARAM>(szThemeName));
    }

    // Forces redraw
    VOID Update()
    {
        SendMessageW(TTM_UPDATE);
    }

    HWND WindowFromPoint(IN POINT * pPoint)
    {
        return reinterpret_cast<HWND>(SendMessageW(TTM_WINDOWFROMPOINT, 0, reinterpret_cast<LPARAM>(pPoint)));
    }
};
