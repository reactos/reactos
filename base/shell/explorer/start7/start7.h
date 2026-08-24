/*
 * ReactOS Explorer
 *
 * Copyright 2026 ReactOS contributors
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _EXPLORER_START7_H_
#define _EXPLORER_START7_H_

/*
 * A Windows 7 styled two-column Start menu ("modern" Start menu).
 *
 * It implements the same IMenuPopup surface that the classic menu exposes to
 * CTrayWindow (Popup / OnSelect / SetSubMenu), so the two implementations can
 * coexist and be swapped at runtime depending on SHELLSTATE.fStartPanelOn
 * (the radio buttons on the "Start Menu" page of the taskbar properties).
 *
 * TODO:
 * - Pinning / most frequently used programs section.
 * - Sliding "All Programs" view.
 * - Localized strings for the power menu entries.
 * - Context menus on items.
 */

/* Layout metrics */
#define SM7_BORDER        1
#define SM7_LEFT_WIDTH    232
#define SM7_RIGHT_WIDTH   162
#define SM7_ITEM_HEIGHT   38
#define SM7_RITEM_HEIGHT  32
#define SM7_SEARCH_HEIGHT 34
#define SM7_BAR_HEIGHT    46
#define SM7_RADIUS        12

/* Palette (Aero-like, works without DWM composition) */
#define SM7_CLR_FRAME     RGB(70, 96, 128)
#define SM7_CLR_WHITE     RGB(255, 255, 255)
#define SM7_CLR_RIGHT_TOP RGB(248, 251, 254)
#define SM7_CLR_RIGHT_BOT RGB(214, 229, 245)
#define SM7_CLR_BAR_TOP   RGB(240, 246, 252)
#define SM7_CLR_BAR_BOT   RGB(188, 212, 238)
#define SM7_CLR_DIVIDER   RGB(173, 199, 227)
#define SM7_CLR_SEP       RGB(213, 224, 238)
#define SM7_CLR_HOVER_BG  RGB(206, 233, 248)
#define SM7_CLR_HOVER_BRD RGB(116, 166, 217)
#define SM7ClrGrayText    RGB(120, 120, 120)
#define SM7_CLR_TEXT      RGB(0, 0, 0)

/* Special command ids reused from the classic menu numeric space */
#define SM7_CMD_SHUTDOWN   TRAYCMD_SHUTDOWN_DIALOG
#define SM7_CMD_LOGOFF     TRAYCMD_LOGOFF_DIALOG
#define SM7_CMD_LOCK       TRAYCMD_LOCK_DESKTOP
#define SM7_CMD_SWITCHUSER TRAYCMD_SWITCH_USER_DIALOG
#define SM7_CMD_HELP       TRAYCMD_HELP_AND_SUPPORT

/* Marker commands used internally by this implementation */
#define SM7_CMD_NONE       0

enum SM7_PANE
{
    PANE_NONE,
    PANE_LEFT,
    PANE_RIGHT,
    PANE_BAR,
};

struct SM7_ITEM
{
    WCHAR name[128];
    LPITEMIDLIST pidl;              /* shell item to execute (may be NULL) */
    IShellFolder *psf;              /* parent folder of pidl (may be NULL) */
    UINT cmd;                       /* tray command for special entries */
    HICON hIcon;
    BOOL bSeparator;
    BOOL bBold;

    SM7_ITEM()
    {
        name[0] = 0;
        pidl = NULL;
        psf = NULL;
        cmd = SM7_CMD_NONE;
        hIcon = NULL;
        bSeparator = FALSE;
        bBold = FALSE;
    }
};

/* Messages used by the pane canvases to talk to their parent menu window */
#define UWM_CANVAS_PAINT (WM_APP + 10) /* wParam: HDC, lParam: pane id */
#define UWM_CANVAS_MOUSE (WM_APP + 11) /* wParam: mouse msg, lParam: packed parent-client pt */
#define UWM_CANVAS_LEAVE (WM_APP + 12) /* lParam: pane id */

#define SM7_GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define SM7_GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

/* Shared helpers implemented in start7.cpp */
VOID SM7FillGradient(HDC hdc, const RECT &rc, COLORREF c1, COLORREF c2);
BOOL SM7RegisterCanvasClass(IN HINSTANCE hInstance);
LRESULT CALLBACK SM7EditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL UseModernStartMenu(VOID);

IMenuPopup*
CreateWin7StartMenu(IN ITrayWindow *Tray);

typedef CWinTraits<WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                   WS_EX_TOOLWINDOW | WS_EX_TOPMOST> SM7WinTraits;

class CWin7StartMenu :
    public CComCoClass<CWin7StartMenu>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CWindowImpl<CWin7StartMenu, CWindow, SM7WinTraits>,
    public IOleWindow,
    public IOleCommandTarget,
    public IMenuPopup
{
    CComPtr<ITrayWindow> m_Tray;

    HWND m_hwndLeft;                /* canvas covering the programs list */
    HWND m_hwndRight;               /* canvas covering the links column */
    HWND m_hwndBar;                 /* canvas covering the bottom bar */
    HWND m_hwndSearch;              /* the search edit control */
    WNDPROC m_fnEditProc;           /* original proc of the search box */

    CAtlArray<SM7_ITEM> m_LeftItems;
    CAtlArray<SM7_ITEM> m_RightItems;

    INT m_HoverPane, m_HoverIndex;
    INT m_ActivePane, m_ActiveIndex;   /* keyboard/mouse selection */
    INT m_BarHover;                    /* 0 none, 1 shutdown, 2 arrow */

    RECT m_rcLeft, m_rcRight, m_rcBar; /* client coords */
    RECT m_rcSearch;                   /* client coords of the search box */
    RECT m_rcShutdown, m_rcArrow;      /* client coords within the bar */

    HFONT m_Font;
    HFONT m_FontBold;
    WCHAR m_szFilter[64];

public:
    CWin7StartMenu();
    virtual ~CWin7StartMenu();

    HRESULT Initialize(IN ITrayWindow *Tray);

    DECLARE_NOT_AGGREGATABLE(CWin7StartMenu)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CWin7StartMenu)
        COM_INTERFACE_ENTRY_IID(IID_IMenuPopup, IMenuPopup)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
    END_COM_MAP()

    DECLARE_WND_CLASS_EX(L"Win7StartMenu", CS_SAVEBITS | CS_DROPSHADOW, -1)

    BEGIN_MSG_MAP(CWin7StartMenu)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
        MESSAGE_HANDLER(WM_SYSCOMMAND, OnSysCommand)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(UWM_CANVAS_PAINT, OnCanvasPaint)
        MESSAGE_HANDLER(UWM_CANVAS_MOUSE, OnCanvasMouse)
        MESSAGE_HANDLER(UWM_CANVAS_LEAVE, OnCanvasLeave)
    END_MSG_MAP()

    /* *** IMenuPopup *** */
    STDMETHOD(Popup)(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags) override;
    STDMETHOD(OnSelect)(DWORD dwSelectType) override;
    STDMETHOD(SetSubMenu)(IMenuPopup *pmp, BOOL fSet) override;

    /* *** IOleWindow *** */
    STDMETHOD(GetWindow)(HWND *phwnd) override;
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode) override;

    /* *** IDeskBar (base of IMenuPopup) *** */
    STDMETHOD(SetClient)(IUnknown *punkClient) override;
    STDMETHOD(GetClient)(IUnknown **ppunkClient) override;
    STDMETHOD(OnPosRectChangeDB)(LPRECT prc) override;

    /* *** IOleCommandTarget *** */
    STDMETHOD(QueryStatus)(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[],
                           OLECMDTEXT *pCmdText) override;
    STDMETHOD(Exec)(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt,
                    VARIANTARG *pvaIn, VARIANTARG *pvaOut) override;

private:
    /* *** window.cpp: construction, layout, show/hide, COM glue *** */
    BOOL CreateMenuWindow();
    BOOL CreateChildren();
    BOOL CreateFonts();
    VOID ComputeLayout();
    VOID ApplyChildrenLayout();
    VOID ResizeToContent();
    VOID Hide();
    VOID UnsubclassSearch();

    /* prop plumbing shared with the canvas/edit procs in start7.cpp */
    VOID SetSearchProps(HWND hwnd);
    VOID ClearSearchProps(HWND hwnd);
    VOID AttachPane(HWND hwndPane, INT pane);

    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

    /* *** paint.cpp: geometry helpers and rendering *** */
    static int ItemHeight(INT pane);
    int ItemCount(INT pane) const;
    RECT PaneRect(INT pane) const;
    RECT ItemRect(INT pane, INT index) const;
    BOOL IsSeparator(INT pane, INT index) const;
    int HitTestPane(INT pane, INT x, INT y) const;
    HWND PaneWindow(INT pane) const;
    VOID InvalidateItem(INT pane, INT index);

    VOID PaintItem(HDC hdc, INT pane, INT index, BOOL bHover, BOOL bActive);
    VOID DrawBarButton(HDC hdc, INT which);

    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnCanvasPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

    /* *** items.cpp: item data, searching, execution *** */
    static VOID ClearItemsOf(CAtlArray<SM7_ITEM> &arr);
    VOID ClearItems();
    static VOID SortItemsByName(CAtlArray<SM7_ITEM> &arr, SIZE_T iFirst);
    static HRESULT AppendFolderItems(CAtlArray<SM7_ITEM> &arr, IN IShellFolder *psfFolder);
    VOID AppendSingleItem(IN IShellFolder *psfFolder, IN LPCITEMIDLIST pidlItem,
                          IN LPCWSTR pszName);
    HRESULT SearchProgramsRecursive(IN IShellFolder *psfFolder, IN UINT uDepth);
    HRESULT FillLeftItems();
    VOID AppendRightPidlItem(UINT csidl, LPCWSTR pszFallbackName, UINT cmd);
    HRESULT FillRightItems();

    struct FOLDERMENU_CTX;
    static VOID AppendFolderMenu(FOLDERMENU_CTX &ctx, HMENU hmenu,
                                 IN IShellFolder *psfFolder, UINT uDepth);
    VOID TrackFolderSubmenu(IN IShellFolder *psf, IN LPCITEMIDLIST pidlFolder,
                            INT pane, INT index);
    VOID ExecuteItem(INT pane, INT index);
    VOID ShellExecutePidl(LPCITEMIDLIST pidl);

    /* *** input.cpp: selection state and user input *** */
    VOID SetHover(INT pane, INT index);
    VOID SetActive(INT pane, INT index);
    VOID MoveSelection(INT pane, INT delta);
    VOID SwitchColumn(INT pane);
    VOID TrackPowerMenu();

    LRESULT OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnCanvasMouse(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnCanvasLeave(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
};

#endif /* _EXPLORER_START7_H_ */
