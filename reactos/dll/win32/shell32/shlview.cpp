/*
 *    ShellView
 *
 *    Copyright 1998,1999    <juergen.schmied@debitel.net>
 *
 * This is the view visualizing the data provided by the shellfolder.
 * No direct access to data from pidls should be done from here.
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
 *
 * FIXME: The order by part of the background context menu should be
 * built according to the columns shown.
 *
 * FIXME: CheckToolbar: handle the "new folder" and "folder up" button
 *
 * FIXME: ShellView_FillList: consider sort orders
 */

/*
TODO:
1. Load/Save the view state from/into the stream provided by the ShellBrowser.
2. Let the shell folder sort items.
3. Code to merge menus in the shellbrowser is incorrect.
4. Move the background context menu creation into shell view. It should store the
    shell view HWND to send commands.
5. Send init, measure, and draw messages to context menu during tracking.
6. Shell view should do SetCommandTarget on internet toolbar.
7. When editing starts on item, set edit text to for editing value.
8. When shell view is called back for item info, let listview save the value.
9. Shell view should update status bar.
10. Fix shell view to handle view mode popup exec.
11. The background context menu should have a pidl just like foreground menus. This
    causes crashes when dynamic handlers try to use the NULL pidl.
12. The SHELLDLL_DefView should not be filled with blue unconditionally. This causes
    annoying flashing of blue even on XP, and is not correct.
13. Reorder of columns doesn't work - might be bug in comctl32
*/

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#undef SV_CLASS_NAME

static const WCHAR SV_CLASS_NAME[] = {'S', 'H', 'E', 'L', 'L', 'D', 'L', 'L', '_', 'D', 'e', 'f', 'V', 'i', 'e', 'w', 0};

typedef struct
{   BOOL    bIsAscending;
    INT     nHeaderID;
    INT     nLastHeaderID;
} LISTVIEW_SORT_INFO, *LPLISTVIEW_SORT_INFO;

#define SHV_CHANGE_NOTIFY WM_USER + 0x1111

class CDefView :
    public CWindowImpl<CDefView, CWindow, CControlWinTraits>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellView,
    public IFolderView,
    public IOleCommandTarget,
    public IDropTarget,
    public IDropSource,
    public IViewObject,
    public IServiceProvider
{
    private:
        CComPtr<IShellFolder>                pSFParent;
        CComPtr<IShellFolder2>                pSF2Parent;
        CComPtr<IShellBrowser>                pShellBrowser;
        CComPtr<ICommDlgBrowser>            pCommDlgBrowser;
        HWND                                hWndList;            /* ListView control */
        HWND                                hWndParent;
        FOLDERSETTINGS                        FolderSettings;
        HMENU                                hMenu;
        UINT                                uState;
        UINT                                cidl;
        LPITEMIDLIST                        *apidl;
        LISTVIEW_SORT_INFO                    ListViewSortInfo;
        ULONG                                hNotify;            /* change notification handle */
        HANDLE                                hAccel;
        DWORD                                dwAspects;
        DWORD                                dwAdvf;
        CComPtr<IAdviseSink>                pAdvSink;
        // for drag and drop
        CComPtr<IDropTarget>                pCurDropTarget;        /* The sub-item, which is currently dragged over */
        CComPtr<IDataObject>                pCurDataObject;        /* The dragged data-object */
        LONG                                iDragOverItem;        /* Dragged over item's index, iff pCurDropTarget != NULL */
        UINT                                cScrollDelay;        /* Send a WM_*SCROLL msg every 250 ms during drag-scroll */
        POINT                                ptLastMousePos;        /* Mouse position at last DragOver call */
        //
        CComPtr<IContextMenu2>                pCM;
    public:
        CDefView();
        ~CDefView();
        HRESULT WINAPI Initialize(IShellFolder *shellFolder);
        HRESULT IncludeObject(LPCITEMIDLIST pidl);
        HRESULT OnDefaultCommand();
        HRESULT OnStateChange(UINT uFlags);
        void CheckToolbar();
        void SetStyle(DWORD dwAdd, DWORD dwRemove);
        BOOL CreateList();
        void UpdateListColors();
        BOOL InitList();
        static INT CALLBACK CompareItems(LPVOID lParam1, LPVOID lParam2, LPARAM lpData);
        static INT CALLBACK ListViewCompareItems(LPVOID lParam1, LPVOID lParam2, LPARAM lpData);
        int LV_FindItemByPidl(LPCITEMIDLIST pidl);
        BOOLEAN LV_AddItem(LPCITEMIDLIST pidl);
        BOOLEAN LV_DeleteItem(LPCITEMIDLIST pidl);
        BOOLEAN LV_RenameItem(LPCITEMIDLIST pidlOld, LPCITEMIDLIST pidlNew);
        static INT CALLBACK fill_list(LPVOID ptr, LPVOID arg);
        HRESULT FillList();
        HMENU BuildFileMenu();
        void MergeFileMenu(HMENU hSubMenu);
        void MergeViewMenu(HMENU hSubMenu);
        UINT GetSelections();
        HRESULT OpenSelectedItems();
        void OnDeactivate();
        void DoActivate(UINT uState);
        HRESULT drag_notify_subitem(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

        // *** IOleWindow methods ***
        virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *lphwnd);
        virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

        // *** IShellView methods ***
        virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator(MSG *pmsg);
        virtual HRESULT STDMETHODCALLTYPE EnableModeless(BOOL fEnable);
        virtual HRESULT STDMETHODCALLTYPE UIActivate(UINT uState);
        virtual HRESULT STDMETHODCALLTYPE Refresh();
        virtual HRESULT STDMETHODCALLTYPE CreateViewWindow(IShellView *psvPrevious, LPCFOLDERSETTINGS pfs, IShellBrowser *psb, RECT *prcView, HWND *phWnd);
        virtual HRESULT STDMETHODCALLTYPE DestroyViewWindow();
        virtual HRESULT STDMETHODCALLTYPE GetCurrentInfo(LPFOLDERSETTINGS pfs);
        virtual HRESULT STDMETHODCALLTYPE AddPropertySheetPages(DWORD dwReserved, LPFNSVADDPROPSHEETPAGE pfn, LPARAM lparam);
        virtual HRESULT STDMETHODCALLTYPE SaveViewState();
        virtual HRESULT STDMETHODCALLTYPE SelectItem(LPCITEMIDLIST pidlItem, SVSIF uFlags);
        virtual HRESULT STDMETHODCALLTYPE GetItemObject(UINT uItem, REFIID riid, void **ppv);

        // *** IFolderView methods ***
        virtual HRESULT STDMETHODCALLTYPE GetCurrentViewMode(UINT *pViewMode);
        virtual HRESULT STDMETHODCALLTYPE SetCurrentViewMode(UINT ViewMode);
        virtual HRESULT STDMETHODCALLTYPE GetFolder(REFIID riid, void **ppv);
        virtual HRESULT STDMETHODCALLTYPE Item(int iItemIndex, LPITEMIDLIST *ppidl);
        virtual HRESULT STDMETHODCALLTYPE ItemCount(UINT uFlags, int *pcItems);
        virtual HRESULT STDMETHODCALLTYPE Items(UINT uFlags, REFIID riid, void **ppv);
        virtual HRESULT STDMETHODCALLTYPE GetSelectionMarkedItem(int *piItem);
        virtual HRESULT STDMETHODCALLTYPE GetFocusedItem(int *piItem);
        virtual HRESULT STDMETHODCALLTYPE GetItemPosition(LPCITEMIDLIST pidl, POINT *ppt);
        virtual HRESULT STDMETHODCALLTYPE GetSpacing(POINT *ppt);
        virtual HRESULT STDMETHODCALLTYPE GetDefaultSpacing(POINT *ppt);
        virtual HRESULT STDMETHODCALLTYPE GetAutoArrange();
        virtual HRESULT STDMETHODCALLTYPE SelectItem(int iItem, DWORD dwFlags);
        virtual HRESULT STDMETHODCALLTYPE SelectAndPositionItems(UINT cidl, LPCITEMIDLIST *apidl, POINT *apt, DWORD dwFlags);

        // *** IOleCommandTarget methods ***
        virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText);
        virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

        // *** IDropTarget methods ***
        virtual HRESULT STDMETHODCALLTYPE DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
        virtual HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
        virtual HRESULT STDMETHODCALLTYPE DragLeave();
        virtual HRESULT STDMETHODCALLTYPE Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

        // *** IDropSource methods ***
        virtual HRESULT STDMETHODCALLTYPE QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState);
        virtual HRESULT STDMETHODCALLTYPE GiveFeedback(DWORD dwEffect);

        // *** IViewObject methods ***
        virtual HRESULT STDMETHODCALLTYPE Draw(DWORD dwDrawAspect, LONG lindex, void *pvAspect, DVTARGETDEVICE *ptd,
                                               HDC hdcTargetDev, HDC hdcDraw, LPCRECTL lprcBounds, LPCRECTL lprcWBounds,
                                               BOOL ( STDMETHODCALLTYPE *pfnContinue )(ULONG_PTR dwContinue), ULONG_PTR dwContinue);
        virtual HRESULT STDMETHODCALLTYPE GetColorSet(DWORD dwDrawAspect, LONG lindex, void *pvAspect,
                DVTARGETDEVICE *ptd, HDC hicTargetDev, LOGPALETTE **ppColorSet);
        virtual HRESULT STDMETHODCALLTYPE Freeze(DWORD dwDrawAspect, LONG lindex, void *pvAspect, DWORD *pdwFreeze);
        virtual HRESULT STDMETHODCALLTYPE Unfreeze(DWORD dwFreeze);
        virtual HRESULT STDMETHODCALLTYPE SetAdvise(DWORD aspects, DWORD advf, IAdviseSink *pAdvSink);
        virtual HRESULT STDMETHODCALLTYPE GetAdvise(DWORD *pAspects, DWORD *pAdvf, IAdviseSink **ppAdvSink);

        // *** IServiceProvider methods ***
        virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

        // message handlers
        LRESULT OnShowWindow(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
        LRESULT OnGetDlgCode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
        LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
        LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
        LRESULT OnSysColorChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
        LRESULT OnGetShellBrowser(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
        LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
        LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
        LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
        LRESULT OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
        LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
        LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
        LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
        LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
        LRESULT OnChangeNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
        LRESULT OnCustomItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
        LRESULT OnSettingChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

        static ATL::CWndClassInfo& GetWndClassInfo()
        {
            static ATL::CWndClassInfo wc =
            {
                {   sizeof(WNDCLASSEX), 0, StartWindowProc,
                    0, 0, NULL, NULL,
                    LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_BACKGROUND + 1), NULL, SV_CLASS_NAME, NULL
                },
                NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
            };
            return wc;
        }

        virtual WNDPROC GetWindowProc()
        {
            return WindowProc;
        }

        static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        {
            CDefView                        *pThis;
            LRESULT                            result;

            // must hold a reference during message handling
            pThis = reinterpret_cast<CDefView *>(hWnd);
            pThis->AddRef();
            result = CWindowImpl<CDefView, CWindow, CControlWinTraits>::WindowProc(hWnd, uMsg, wParam, lParam);
            pThis->Release();
            return result;
        }

        BEGIN_MSG_MAP(CDefView)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
        MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(SHV_CHANGE_NOTIFY, OnChangeNotify)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_DRAWITEM, OnCustomItem)
        MESSAGE_HANDLER(WM_MEASUREITEM, OnCustomItem)
        MESSAGE_HANDLER(WM_SHOWWINDOW, OnShowWindow)
        MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        MESSAGE_HANDLER(WM_SYSCOLORCHANGE, OnSysColorChange)
        MESSAGE_HANDLER(CWM_GETISHELLBROWSER, OnGetShellBrowser)
        MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChange)
        END_MSG_MAP()

        BEGIN_COM_MAP(CDefView)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
        COM_INTERFACE_ENTRY_IID(IID_IShellView, IShellView)
        COM_INTERFACE_ENTRY_IID(IID_IFolderView, IFolderView)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        COM_INTERFACE_ENTRY_IID(IID_IDropTarget, IDropTarget)
        COM_INTERFACE_ENTRY_IID(IID_IDropSource, IDropSource)
        COM_INTERFACE_ENTRY_IID(IID_IViewObject, IViewObject)
        COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
        END_COM_MAP()
};

/* ListView Header ID's */
#define LISTVIEW_COLUMN_NAME 0
#define LISTVIEW_COLUMN_SIZE 1
#define LISTVIEW_COLUMN_TYPE 2
#define LISTVIEW_COLUMN_TIME 3
#define LISTVIEW_COLUMN_ATTRIB 4

/*menu items */
#define IDM_VIEW_FILES  (FCIDM_SHVIEWFIRST + 0x500)
#define IDM_VIEW_IDW    (FCIDM_SHVIEWFIRST + 0x501)
#define IDM_MYFILEITEM  (FCIDM_SHVIEWFIRST + 0x502)

#define ID_LISTVIEW     1

/*windowsx.h */
#define GET_WM_COMMAND_ID(wp, lp)               LOWORD(wp)
#define GET_WM_COMMAND_HWND(wp, lp)             (HWND)(lp)
#define GET_WM_COMMAND_CMD(wp, lp)              HIWORD(wp)

/*
  Items merged into the toolbar and the filemenu
*/
typedef struct
{   int   idCommand;
    int   iImage;
    int   idButtonString;
    int   idMenuString;
    BYTE  bState;
    BYTE  bStyle;
} MYTOOLINFO, *LPMYTOOLINFO;

static const MYTOOLINFO Tools[] =
{
    { FCIDM_SHVIEW_BIGICON,    0, 0, IDS_VIEW_LARGE,   TBSTATE_ENABLED, BTNS_BUTTON },
    { FCIDM_SHVIEW_SMALLICON,  0, 0, IDS_VIEW_SMALL,   TBSTATE_ENABLED, BTNS_BUTTON },
    { FCIDM_SHVIEW_LISTVIEW,   0, 0, IDS_VIEW_LIST,    TBSTATE_ENABLED, BTNS_BUTTON },
    { FCIDM_SHVIEW_REPORTVIEW, 0, 0, IDS_VIEW_DETAILS, TBSTATE_ENABLED, BTNS_BUTTON },
    { -1, 0, 0, 0, 0, 0}
};

typedef void (CALLBACK *PFNSHGETSETTINGSPROC)(LPSHELLFLAGSTATE lpsfs, DWORD dwMask);

CDefView::CDefView()
{
    hWndList = NULL;
    hWndParent = NULL;
    FolderSettings.fFlags = 0;
    FolderSettings.ViewMode = 0;
    hMenu = NULL;
    uState = 0;
    cidl = 0;
    apidl = NULL;
    ListViewSortInfo.bIsAscending = FALSE;
    ListViewSortInfo.nHeaderID = 0;
    ListViewSortInfo.nLastHeaderID = 0;
    hNotify = 0;
    hAccel = NULL;
    dwAspects = 0;
    dwAdvf = 0;
    iDragOverItem = 0;
    cScrollDelay = 0;
    ptLastMousePos.x = 0;
    ptLastMousePos.y = 0;
}

CDefView::~CDefView()
{
    TRACE(" destroying IShellView(%p)\n", this);

    SHFree(apidl);
}

HRESULT WINAPI CDefView::Initialize(IShellFolder *shellFolder)
{
    pSFParent = shellFolder;
    shellFolder->QueryInterface(IID_IShellFolder2, (LPVOID *)&pSF2Parent);

    return S_OK;
}

/**********************************************************
 *
 * ##### helperfunctions for communication with ICommDlgBrowser #####
 */
HRESULT CDefView::IncludeObject(LPCITEMIDLIST pidl)
{
    HRESULT ret = S_OK;

    if (pCommDlgBrowser.p != NULL)
    {
        TRACE("ICommDlgBrowser::IncludeObject pidl=%p\n", pidl);
        ret = pCommDlgBrowser->IncludeObject((IShellView *)this, pidl);
        TRACE("--0x%08x\n", ret);
    }

    return ret;
}

HRESULT CDefView::OnDefaultCommand()
{
    HRESULT ret = S_FALSE;

    if (pCommDlgBrowser.p != NULL)
    {
        TRACE("ICommDlgBrowser::OnDefaultCommand\n");
        ret = pCommDlgBrowser->OnDefaultCommand((IShellView *)this);
        TRACE("-- returns %08x\n", ret);
    }

    return ret;
}

HRESULT CDefView::OnStateChange(UINT uFlags)
{
    HRESULT ret = S_FALSE;

    if (pCommDlgBrowser.p != NULL)
    {
        TRACE("ICommDlgBrowser::OnStateChange flags=%x\n", uFlags);
        ret = pCommDlgBrowser->OnStateChange((IShellView *)this, uFlags);
        TRACE("--\n");
    }

    return ret;
}
/**********************************************************
 *    set the toolbar of the filedialog buttons
 *
 * - activates the buttons from the shellbrowser according to
 *   the view state
 */
void CDefView::CheckToolbar()
{
    LRESULT result;

    TRACE("\n");

    if (pCommDlgBrowser != NULL)
    {
        pShellBrowser->SendControlMsg(FCW_TOOLBAR, TB_CHECKBUTTON,
                                      FCIDM_TB_SMALLICON, (FolderSettings.ViewMode == FVM_LIST) ? TRUE : FALSE, &result);
        pShellBrowser->SendControlMsg(FCW_TOOLBAR, TB_CHECKBUTTON,
                                      FCIDM_TB_REPORTVIEW, (FolderSettings.ViewMode == FVM_DETAILS) ? TRUE : FALSE, &result);
        pShellBrowser->SendControlMsg(FCW_TOOLBAR, TB_ENABLEBUTTON,
                                      FCIDM_TB_SMALLICON, TRUE, &result);
        pShellBrowser->SendControlMsg(FCW_TOOLBAR, TB_ENABLEBUTTON,
                                      FCIDM_TB_REPORTVIEW, TRUE, &result);
    }
}

/**********************************************************
 *
 * ##### helperfunctions for initializing the view #####
 */
/**********************************************************
 *    change the style of the listview control
 */
void CDefView::SetStyle(DWORD dwAdd, DWORD dwRemove)
{
    DWORD tmpstyle;

    TRACE("(%p)\n", this);

    tmpstyle = ::GetWindowLongPtrW(hWndList, GWL_STYLE);
    ::SetWindowLongPtrW(hWndList, GWL_STYLE, dwAdd | (tmpstyle & ~dwRemove));
}

/**********************************************************
* ShellView_CreateList()
*
* - creates the list view window
*/
BOOL CDefView::CreateList()
{   DWORD dwStyle, dwExStyle;

    TRACE("%p\n", this);

    dwStyle = WS_TABSTOP | WS_VISIBLE | WS_CHILDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
              LVS_SHAREIMAGELISTS | LVS_EDITLABELS | LVS_AUTOARRANGE;
    dwExStyle = WS_EX_CLIENTEDGE;

    if (FolderSettings.fFlags & FWF_DESKTOP)
        dwStyle |= LVS_ALIGNLEFT;
    else
        dwStyle |= LVS_ALIGNTOP;

    switch (FolderSettings.ViewMode)
    {
        case FVM_ICON:
            dwStyle |= LVS_ICON;
            break;

        case FVM_DETAILS:
            dwStyle |= LVS_REPORT;
            break;

        case FVM_SMALLICON:
            dwStyle |= LVS_SMALLICON;
            break;

        case FVM_LIST:
            dwStyle |= LVS_LIST;
            break;

        default:
            dwStyle |= LVS_LIST;
            break;
    }

    if (FolderSettings.fFlags & FWF_AUTOARRANGE)
        dwStyle |= LVS_AUTOARRANGE;

    if (FolderSettings.fFlags & FWF_DESKTOP)
        FolderSettings.fFlags |= FWF_NOCLIENTEDGE | FWF_NOSCROLL;

    if (FolderSettings.fFlags & FWF_SINGLESEL)
        dwStyle |= LVS_SINGLESEL;

    if (FolderSettings.fFlags & FWF_NOCLIENTEDGE)
        dwExStyle &= ~WS_EX_CLIENTEDGE;

    hWndList = CreateWindowExW( dwExStyle,
                                WC_LISTVIEWW,
                                NULL,
                                dwStyle,
                                0, 0, 0, 0,
                                m_hWnd,
                                (HMENU)ID_LISTVIEW,
                                shell32_hInstance,
                                NULL);

    if (!hWndList)
        return FALSE;

    ListViewSortInfo.bIsAscending = TRUE;
    ListViewSortInfo.nHeaderID = -1;
    ListViewSortInfo.nLastHeaderID = -1;

    UpdateListColors();

    /*  UpdateShellSettings(); */
    return TRUE;
}

void CDefView::UpdateListColors()
{
    if (FolderSettings.fFlags & FWF_DESKTOP)
    {
        /* Check if drop shadows option is enabled */
        BOOL bDropShadow = FALSE;
        DWORD cbDropShadow = sizeof(bDropShadow);
        WCHAR wszBuf[16] = L"";

        RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
                     L"ListviewShadow", RRF_RT_DWORD, NULL, &bDropShadow, &cbDropShadow);
        if (bDropShadow && SystemParametersInfoW(SPI_GETDESKWALLPAPER, _countof(wszBuf), wszBuf, 0) && wszBuf[0])
        {
            SendMessageW(hWndList, LVM_SETTEXTBKCOLOR, 0, CLR_NONE);
            SendMessageW(hWndList, LVM_SETBKCOLOR, 0, CLR_NONE);
            SendMessageW(hWndList, LVM_SETTEXTCOLOR, 0, RGB(255, 255, 255));
            SendMessageW(hWndList, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_TRANSPARENTSHADOWTEXT, LVS_EX_TRANSPARENTSHADOWTEXT);
        }
        else
        {
            COLORREF crDesktop = GetSysColor(COLOR_DESKTOP);
            SendMessageW(hWndList, LVM_SETTEXTBKCOLOR, 0, crDesktop);
            SendMessageW(hWndList, LVM_SETBKCOLOR, 0, crDesktop);
            if (GetRValue(crDesktop) + GetGValue(crDesktop) + GetBValue(crDesktop) > 128 * 3)
                SendMessageW(hWndList, LVM_SETTEXTCOLOR, 0, RGB(0, 0, 0));
            else
                SendMessageW(hWndList, LVM_SETTEXTCOLOR, 0, RGB(255, 255, 255));
            SendMessageW(hWndList, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_TRANSPARENTSHADOWTEXT, 0);
        }
    }
}

/**********************************************************
* ShellView_InitList()
*
* - adds all needed columns to the shellview
*/
BOOL CDefView::InitList()
{
    LVCOLUMNW    lvColumn;
    SHELLDETAILS    sd;
    WCHAR    szTemp[50];

    TRACE("%p\n", this);

    SendMessageW(hWndList, LVM_DELETEALLITEMS, 0, 0);

    lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    lvColumn.pszText = szTemp;

    if (pSF2Parent)
    {
        for (int i = 0; 1; i++)
        {
            if (FAILED(pSF2Parent->GetDetailsOf(NULL, i, &sd)))
                break;

            lvColumn.fmt = sd.fmt;
            lvColumn.cx = sd.cxChar * 8; /* chars->pixel */
            StrRetToStrNW( szTemp, 50, &sd.str, NULL);
            SendMessageW(hWndList, LVM_INSERTCOLUMNW, i, (LPARAM) &lvColumn);
        }
    }
    else
    {
        FIXME("no SF2\n");
    }

    SendMessageW(hWndList, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)ShellSmallIconList);
    SendMessageW(hWndList, LVM_SETIMAGELIST, LVSIL_NORMAL, (LPARAM)ShellBigIconList);

    return TRUE;
}

/**********************************************************
* ShellView_CompareItems()
*
* NOTES
*  internal, CALLBACK for DSA_Sort
*/
INT CALLBACK CDefView::CompareItems(LPVOID lParam1, LPVOID lParam2, LPARAM lpData)
{
    int ret;
    TRACE("pidl1=%p pidl2=%p lpsf=%p\n", lParam1, lParam2, (LPVOID) lpData);

    if (!lpData)
        return 0;

    ret = (SHORT)SCODE_CODE(((IShellFolder *)lpData)->CompareIDs(0, (LPITEMIDLIST)lParam1, (LPITEMIDLIST)lParam2));
    TRACE("ret=%i\n", ret);

    return ret;
}

/*************************************************************************
 * ShellView_ListViewCompareItems
 *
 * Compare Function for the Listview (FileOpen Dialog)
 *
 * PARAMS
 *     lParam1       [I] the first ItemIdList to compare with
 *     lParam2       [I] the second ItemIdList to compare with
 *     lpData        [I] The column ID for the header Ctrl to process
 *
 * RETURNS
 *     A negative value if the first item should precede the second,
 *     a positive value if the first item should follow the second,
 *     or zero if the two items are equivalent
 *
 * NOTES
 *    FIXME: function does what ShellView_CompareItems is supposed to do.
 *    unify it and figure out how to use the undocumented first parameter
 *    of IShellFolder_CompareIDs to do the job this function does and
 *    move this code to IShellFolder.
 *    make LISTVIEW_SORT_INFO obsolete
 *    the way this function works is only usable if we had only
 *    filesystemfolders  (25/10/99 jsch)
 */
INT CALLBACK CDefView::ListViewCompareItems(LPVOID lParam1, LPVOID lParam2, LPARAM lpData)
{
    INT nDiff = 0;
    FILETIME fd1, fd2;
    char strName1[MAX_PATH], strName2[MAX_PATH];
    BOOL bIsFolder1, bIsFolder2, bIsBothFolder;
    LPITEMIDLIST pItemIdList1 = (LPITEMIDLIST) lParam1;
    LPITEMIDLIST pItemIdList2 = (LPITEMIDLIST) lParam2;
    LISTVIEW_SORT_INFO *pSortInfo = (LPLISTVIEW_SORT_INFO) lpData;


    bIsFolder1 = _ILIsFolder(pItemIdList1);
    bIsFolder2 = _ILIsFolder(pItemIdList2);
    bIsBothFolder = bIsFolder1 && bIsFolder2;

    /* When sorting between a File and a Folder, the Folder gets sorted first */
    if ( (bIsFolder1 || bIsFolder2) && !bIsBothFolder)
    {
        nDiff = bIsFolder1 ? -1 : 1;
    }
    else
    {
        /* Sort by Time: Folders or Files can be sorted */

        if(pSortInfo->nHeaderID == LISTVIEW_COLUMN_TIME)
        {
            _ILGetFileDateTime(pItemIdList1, &fd1);
            _ILGetFileDateTime(pItemIdList2, &fd2);
            nDiff = CompareFileTime(&fd2, &fd1);
        }
        /* Sort by Attribute: Folder or Files can be sorted */
        else if(pSortInfo->nHeaderID == LISTVIEW_COLUMN_ATTRIB)
        {
            _ILGetFileAttributes(pItemIdList1, strName1, MAX_PATH);
            _ILGetFileAttributes(pItemIdList2, strName2, MAX_PATH);
            nDiff = lstrcmpiA(strName1, strName2);
        }
        /* Sort by FileName: Folder or Files can be sorted */
        else if (pSortInfo->nHeaderID == LISTVIEW_COLUMN_NAME || bIsBothFolder)
        {
            /* Sort by Text */
            _ILSimpleGetText(pItemIdList1, strName1, MAX_PATH);
            _ILSimpleGetText(pItemIdList2, strName2, MAX_PATH);
            nDiff = lstrcmpiA(strName1, strName2);
        }
        /* Sort by File Size, Only valid for Files */
        else if (pSortInfo->nHeaderID == LISTVIEW_COLUMN_SIZE)
        {
            nDiff = (INT)(_ILGetFileSize(pItemIdList1, NULL, 0) - _ILGetFileSize(pItemIdList2, NULL, 0));
        }
        /* Sort by File Type, Only valid for Files */
        else if (pSortInfo->nHeaderID == LISTVIEW_COLUMN_TYPE)
        {
            /* Sort by Type */
            _ILGetFileType(pItemIdList1, strName1, MAX_PATH);
            _ILGetFileType(pItemIdList2, strName2, MAX_PATH);
            nDiff = lstrcmpiA(strName1, strName2);
        }
    }
    /*  If the Date, FileSize, FileType, Attrib was the same, sort by FileName */

    if (nDiff == 0)
    {
        _ILSimpleGetText(pItemIdList1, strName1, MAX_PATH);
        _ILSimpleGetText(pItemIdList2, strName2, MAX_PATH);
        nDiff = lstrcmpiA(strName1, strName2);
    }

    if (!pSortInfo->bIsAscending)
    {
        nDiff = -nDiff;
    }

    return nDiff;
}

/**********************************************************
*  LV_FindItemByPidl()
*/
int CDefView::LV_FindItemByPidl(LPCITEMIDLIST pidl)
{
    LVITEMW lvItem;
    lvItem.iSubItem = 0;
    lvItem.mask = LVIF_PARAM;

    for (lvItem.iItem = 0;
            SendMessageW(hWndList, LVM_GETITEMW, 0, (LPARAM) &lvItem);
            lvItem.iItem++)
    {
        LPITEMIDLIST currentpidl = (LPITEMIDLIST) lvItem.lParam;
        HRESULT hr = pSFParent->CompareIDs(0, pidl, currentpidl);

        if (SUCCEEDED(hr) && !HRESULT_CODE(hr))
        {
            return lvItem.iItem;
        }
    }
    return -1;
}

/**********************************************************
* LV_AddItem()
*/
BOOLEAN CDefView::LV_AddItem(LPCITEMIDLIST pidl)
{
    LVITEMW    lvItem;

    TRACE("(%p)(pidl=%p)\n", this, pidl);

    lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;    /*set the mask*/
    lvItem.iItem = ListView_GetItemCount(hWndList);    /*add the item to the end of the list*/
    lvItem.iSubItem = 0;
    lvItem.lParam = (LPARAM) ILClone(ILFindLastID(pidl));                /*set the item's data*/
    lvItem.pszText = LPSTR_TEXTCALLBACKW;            /*get text on a callback basis*/
    lvItem.iImage = I_IMAGECALLBACK;            /*get the image on a callback basis*/

    if (SendMessageW(hWndList, LVM_INSERTITEMW, 0, (LPARAM)&lvItem) == -1)
        return FALSE;
    else
        return TRUE;
}

/**********************************************************
* LV_DeleteItem()
*/
BOOLEAN CDefView::LV_DeleteItem(LPCITEMIDLIST pidl)
{
    int nIndex;

    TRACE("(%p)(pidl=%p)\n", this, pidl);

    nIndex = LV_FindItemByPidl(ILFindLastID(pidl));

    return (-1 == ListView_DeleteItem(hWndList, nIndex)) ? FALSE : TRUE;
}

/**********************************************************
* LV_RenameItem()
*/
BOOLEAN CDefView::LV_RenameItem(LPCITEMIDLIST pidlOld, LPCITEMIDLIST pidlNew)
{
    int nItem;
    LVITEMW lvItem;

    TRACE("(%p)(pidlold=%p pidlnew=%p)\n", this, pidlOld, pidlNew);

    nItem = LV_FindItemByPidl(ILFindLastID(pidlOld));

    if ( -1 != nItem )
    {
        lvItem.mask = LVIF_PARAM;        /* only the pidl */
        lvItem.iItem = nItem;
        SendMessageW(hWndList, LVM_GETITEMW, 0, (LPARAM) &lvItem);

        SHFree((LPITEMIDLIST)lvItem.lParam);
        lvItem.mask = LVIF_PARAM|LVIF_IMAGE;
        lvItem.iItem = nItem;
        lvItem.lParam = (LPARAM) ILClone(ILFindLastID(pidlNew));    /* set the item's data */
        lvItem.iImage = SHMapPIDLToSystemImageListIndex(pSFParent, pidlNew, 0);
        SendMessageW(hWndList, LVM_SETITEMW, 0, (LPARAM) &lvItem);
        SendMessageW(hWndList, LVM_UPDATE, nItem, 0);
        return TRUE;                    /* FIXME: better handling */
    }

    return FALSE;
}

/**********************************************************
* ShellView_FillList()
*
* - gets the objectlist from the shellfolder
* - sorts the list
* - fills the list into the view
*/
INT CALLBACK CDefView::fill_list( LPVOID ptr, LPVOID arg )
{
    LPITEMIDLIST pidl = (LPITEMIDLIST)ptr;
    CDefView *pThis = (CDefView *)arg;
    /* in a commdlg This works as a filemask*/
    if (pThis->IncludeObject(pidl) == S_OK)
        pThis->LV_AddItem(pidl);

    SHFree(pidl);
    return TRUE;
}

HRESULT CDefView::FillList()
{
    LPENUMIDLIST    pEnumIDList;
    LPITEMIDLIST    pidl;
    DWORD        dwFetched;
    HRESULT        hRes;
    HDPA        hdpa;

    TRACE("%p\n", this);

    /* get the itemlist from the shfolder*/
    hRes = pSFParent->EnumObjects(m_hWnd, SHCONTF_NONFOLDERS | SHCONTF_FOLDERS, &pEnumIDList);
    if (hRes != S_OK)
    {
        if (hRes == S_FALSE)
            return(NOERROR);
        return(hRes);
    }

    /* create a pointer array */
    hdpa = DPA_Create(16);
    if (!hdpa)
    {
        return(E_OUTOFMEMORY);
    }

    /* copy the items into the array*/
    while((S_OK == pEnumIDList->Next(1, &pidl, &dwFetched)) && dwFetched)
    {
        if (DPA_InsertPtr(hdpa, 0x7fff, pidl) == -1)
        {
            SHFree(pidl);
        }
    }

    /* sort the array */
    DPA_Sort(hdpa, CompareItems, (LPARAM)pSFParent.p);

    /*turn the listview's redrawing off*/
    SendMessageA(hWndList, WM_SETREDRAW, FALSE, 0);

    DPA_DestroyCallback( hdpa, fill_list, (void *)this);

    /*turn the listview's redrawing back on and force it to draw*/
    SendMessageA(hWndList, WM_SETREDRAW, TRUE, 0);

    pEnumIDList->Release(); /* destroy the list*/

    return S_OK;
}

LRESULT CDefView::OnShowWindow(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    ::UpdateWindow(hWndList);
    bHandled = FALSE;
    return 0;
}

LRESULT CDefView::OnGetDlgCode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    return SendMessageW(hWndList, uMsg, 0, 0);
}

LRESULT CDefView::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    RevokeDragDrop(m_hWnd);
    SHChangeNotifyDeregister(hNotify);
    bHandled = FALSE;
    return 0;
}

LRESULT CDefView::OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (FolderSettings.fFlags & (FWF_DESKTOP | FWF_TRANSPARENT))
        return SendMessageW(GetParent(), WM_ERASEBKGND, wParam, lParam); /* redirect to parent */

    bHandled = FALSE;
    return 0;
}

LRESULT CDefView::OnSysColorChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    /* Update desktop labels color */
    UpdateListColors();

    /* Forward WM_SYSCOLORCHANGE to common controls */
    return SendMessageW(hWndList, uMsg, 0, 0);
}

LRESULT CDefView::OnGetShellBrowser(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    return (LRESULT)pShellBrowser.p;
}

/**********************************************************
*  ShellView_OnCreate()
*/
LRESULT CDefView::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    CComPtr<IDropTarget>                pdt;
    SHChangeNotifyEntry ntreg;
    CComPtr<IPersistFolder2>            ppf2;

    TRACE("%p\n", this);

    if(CreateList())
    {
        if(InitList())
        {
            FillList();
        }
    }

    if (SUCCEEDED(this->QueryInterface(IID_IDropTarget, (LPVOID*)&pdt)))
        RegisterDragDrop(m_hWnd, pdt);

    /* register for receiving notifications */
    pSFParent->QueryInterface(IID_IPersistFolder2, (LPVOID*)&ppf2);
    if (ppf2)
    {
        ppf2->GetCurFolder((LPITEMIDLIST*)&ntreg.pidl);
        ntreg.fRecursive = TRUE;
        hNotify = SHChangeNotifyRegister(m_hWnd, SHCNF_IDLIST, SHCNE_ALLEVENTS, SHV_CHANGE_NOTIFY, 1, &ntreg);
        SHFree((LPITEMIDLIST)ntreg.pidl);
    }

    hAccel = LoadAcceleratorsA(shell32_hInstance, "shv_accel");

    return S_OK;
}

/**********************************************************
 *    #### Handling of the menus ####
 */

/**********************************************************
* ShellView_BuildFileMenu()
*/
HMENU CDefView::BuildFileMenu()
{   WCHAR    szText[MAX_PATH];
    MENUITEMINFOW    mii;
    int    nTools, i;
    HMENU    hSubMenu;

    TRACE("(%p)\n", this);

    hSubMenu = CreatePopupMenu();
    if (hSubMenu)
    {
        /*get the number of items in our global array*/
        for(nTools = 0; Tools[nTools].idCommand != -1; nTools++) {}

        /*add the menu items*/
        for(i = 0; i < nTools; i++)
        {
            LoadStringW(shell32_hInstance, Tools[i].idMenuString, szText, MAX_PATH);

            ZeroMemory(&mii, sizeof(mii));
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;

            if(BTNS_SEP != Tools[i].bStyle) /* no separator*/
            {
                mii.fType = MFT_STRING;
                mii.fState = MFS_ENABLED;
                mii.dwTypeData = szText;
                mii.wID = Tools[i].idCommand;
            }
            else
            {
                mii.fType = MFT_SEPARATOR;
            }
            /* tack This item onto the end of the menu */
            InsertMenuItemW(hSubMenu, (UINT) - 1, TRUE, &mii);
        }
    }

    TRACE("-- return (menu=%p)\n", hSubMenu);
    return hSubMenu;
}

/**********************************************************
* ShellView_MergeFileMenu()
*/
void CDefView::MergeFileMenu(HMENU hSubMenu)
{
    TRACE("(%p)->(submenu=%p) stub\n", this, hSubMenu);

    if (hSubMenu)
    {   /*insert This item at the beginning of the menu */
        _InsertMenuItemW(hSubMenu, 0, TRUE, 0, MFT_SEPARATOR, NULL, MFS_ENABLED);
        _InsertMenuItemW(hSubMenu, 0, TRUE, IDM_MYFILEITEM, MFT_STRING, L"dummy45", MFS_ENABLED);
    }

    TRACE("--\n");
}

/**********************************************************
* ShellView_MergeViewMenu()
*/
void CDefView::MergeViewMenu(HMENU hSubMenu)
{
    TRACE("(%p)->(submenu=%p)\n", this, hSubMenu);

    if (hSubMenu)
    {
        /*add a separator at the correct position in the menu*/
        MENUITEMINFOW mii;
        static WCHAR view[] = L"View";

        _InsertMenuItemW(hSubMenu, FCIDM_MENU_VIEW_SEP_OPTIONS, FALSE, 0, MFT_SEPARATOR, NULL, MFS_ENABLED);

        ZeroMemory(&mii, sizeof(mii));
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_DATA;
        mii.fType = MFT_STRING;
        mii.dwTypeData = view;
        mii.hSubMenu = LoadMenuW(shell32_hInstance, L"MENU_001");
        InsertMenuItemW(hSubMenu, FCIDM_MENU_VIEW_SEP_OPTIONS, FALSE, &mii);
    }
}

/**********************************************************
*   ShellView_GetSelections()
*
* - fills the this->apidl list with the selected objects
*
* RETURNS
*  number of selected items
*/
UINT CDefView::GetSelections()
{
    LVITEMW    lvItem;
    UINT    i = 0;

    SHFree(apidl);

    cidl = ListView_GetSelectedCount(hWndList);
    apidl = (LPITEMIDLIST*)SHAlloc(cidl * sizeof(LPITEMIDLIST));

    TRACE("selected=%i\n", cidl);

    if (apidl)
    {
        TRACE("-- Items selected =%u\n", cidl);

        lvItem.mask = LVIF_STATE | LVIF_PARAM;
        lvItem.stateMask = LVIS_SELECTED;
        lvItem.iItem = 0;
        lvItem.iSubItem = 0;
        lvItem.state = 0;

        while(SendMessageW(hWndList, LVM_GETITEMW, 0, (LPARAM)&lvItem) && (i < cidl))
        {
            if(lvItem.state & LVIS_SELECTED)
            {
                apidl[i] = (LPITEMIDLIST)lvItem.lParam;
                i++;
                if (i == cidl)
                    break;
                TRACE("-- selected Item found\n");
            }
            lvItem.iItem++;
        }
    }

    return cidl;
}

/**********************************************************
 *    ShellView_OpenSelectedItems()
 */
HRESULT CDefView::OpenSelectedItems()
{
    static UINT CF_IDLIST = 0;
    HRESULT hr;
    CComPtr<IDataObject>                selection;
    CComPtr<IContextMenu>                cm;
    HMENU hmenu;
    FORMATETC fetc;
    STGMEDIUM stgm;
    LPIDA pIDList;
    LPCITEMIDLIST parent_pidl;
    WCHAR parent_path[MAX_PATH];
    LPCWSTR parent_dir = NULL;
    SFGAOF attribs;
    int i;
    CMINVOKECOMMANDINFOEX ici;
    MENUITEMINFOW info;

    if (0 == GetSelections())
    {
        return S_OK;
    }

    hr = pSFParent->GetUIObjectOf(m_hWnd, cidl,
                                  (LPCITEMIDLIST*)apidl, IID_IContextMenu,
                                  0, (LPVOID *)&cm);

    if (SUCCEEDED(hr))
    {
        hmenu = CreatePopupMenu();
        if (hmenu)
        {
            hr = IUnknown_SetSite(cm, (IShellView *)this);
            if (SUCCEEDED(cm->QueryContextMenu(hmenu, 0, 0x20, 0x7fff, CMF_DEFAULTONLY)))
            {
                INT def = -1, n = GetMenuItemCount(hmenu);

                for ( i = 0; i < n; i++ )
                {
                    memset( &info, 0, sizeof info );
                    info.cbSize = sizeof info;
                    info.fMask = MIIM_FTYPE | MIIM_STATE | MIIM_ID;
                    if (GetMenuItemInfoW( hmenu, i, TRUE, &info))
                    {
                        if (info.fState & MFS_DEFAULT)
                        {
                            def = info.wID;
                            break;
                        }
                    }
                }
                if (def != -1)
                {
                    memset( &ici, 0, sizeof ici );
                    ici.cbSize = sizeof ici;
                    ici.lpVerb = MAKEINTRESOURCEA( def );
                    ici.hwnd = m_hWnd;

                    hr = cm->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);
                    if (hr == S_OK)
                    {
                        DestroyMenu(hmenu);
                        hr = IUnknown_SetSite(cm, NULL);
                        return S_OK;
                    }
                    else
                        ERR("InvokeCommand failed: %x\n", hr);
                }
                else
                    ERR("No default context menu item\n");

            }
            DestroyMenu( hmenu );
            hr = IUnknown_SetSite(cm, NULL);
        }
    }



    hr = pSFParent->GetUIObjectOf(m_hWnd, cidl,
                                  (LPCITEMIDLIST*)apidl, IID_IDataObject,
                                  0, (LPVOID *)&selection);



    if (FAILED(hr))
        return hr;

    if (0 == CF_IDLIST)
    {
        CF_IDLIST = RegisterClipboardFormatW(CFSTR_SHELLIDLIST);
    }

    fetc.cfFormat = CF_IDLIST;
    fetc.ptd = NULL;
    fetc.dwAspect = DVASPECT_CONTENT;
    fetc.lindex = -1;
    fetc.tymed = TYMED_HGLOBAL;

    hr = selection->QueryGetData(&fetc);
    if (FAILED(hr))
        return hr;

    hr = selection->GetData(&fetc, &stgm);
    if (FAILED(hr))
        return hr;

    pIDList = (LPIDA)GlobalLock(stgm.hGlobal);

    parent_pidl = (LPCITEMIDLIST) ((LPBYTE)pIDList + pIDList->aoffset[0]);
    hr = pSFParent->GetAttributesOf(1, &parent_pidl, &attribs);
    if (SUCCEEDED(hr) && (attribs & SFGAO_FILESYSTEM) &&
            SHGetPathFromIDListW(parent_pidl, parent_path))
    {
        parent_dir = parent_path;
    }

    for (i = pIDList->cidl; i > 0; --i)
    {
        LPCITEMIDLIST pidl;

        pidl = (LPCITEMIDLIST)((LPBYTE)pIDList + pIDList->aoffset[i]);

        attribs = SFGAO_FOLDER;
        hr = pSFParent->GetAttributesOf(1, &pidl, &attribs);

        if (SUCCEEDED(hr) && ! (attribs & SFGAO_FOLDER))
        {
            SHELLEXECUTEINFOW shexinfo;

            shexinfo.cbSize = sizeof(SHELLEXECUTEINFOW);
            shexinfo.fMask = SEE_MASK_INVOKEIDLIST;    /* SEE_MASK_IDLIST is also possible. */
            shexinfo.hwnd = NULL;
            shexinfo.lpVerb = NULL;
            shexinfo.lpFile = NULL;
            shexinfo.lpParameters = NULL;
            shexinfo.lpDirectory = parent_dir;
            shexinfo.nShow = SW_NORMAL;
            shexinfo.lpIDList = ILCombine(parent_pidl, pidl);

            ShellExecuteExW(&shexinfo);    /* Discard error/success info */

            ILFree((LPITEMIDLIST)shexinfo.lpIDList);
        }
    }

    GlobalUnlock(stgm.hGlobal);
    ReleaseStgMedium(&stgm);

    return S_OK;
}

/**********************************************************
 *    ShellView_DoContextMenu()
 */
LRESULT CDefView::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    WORD                 x;
    WORD                 y;
    UINT                 uCommand;
    DWORD                wFlags;
    HMENU                hMenu;
    BOOL                 fExplore;
    HWND                 hwndTree;
    CMINVOKECOMMANDINFO  cmi;
    HRESULT              hResult;

    // for some reason I haven't figured out, we sometimes recurse into this method
    if (pCM != NULL)
        return 0;

    x = LOWORD(lParam);
    y = HIWORD(lParam);

    TRACE("(%p)->(0x%08x 0x%08x) stub\n", this, x, y);

    fExplore = FALSE;
    hwndTree = NULL;

    /* look, what's selected and create a context menu object of it*/
    if (GetSelections())
    {
        pSFParent->GetUIObjectOf(hWndParent, cidl, (LPCITEMIDLIST*)apidl, IID_IContextMenu, NULL, (LPVOID *)&pCM);

        if (pCM)
        {
            TRACE("-- pContextMenu\n");
            hMenu = CreatePopupMenu();

            if (hMenu)
            {
                hResult = IUnknown_SetSite(pCM, (IShellView *)this);

                /* See if we are in Explore or Open mode. If the browser's tree is present, we are in Explore mode.*/
                if (SUCCEEDED(pShellBrowser->GetControlWindow(FCW_TREE, &hwndTree)) && hwndTree)
                {
                    TRACE("-- explore mode\n");
                    fExplore = TRUE;
                }

                /* build the flags depending on what we can do with the selected item */
                wFlags = CMF_NORMAL | (cidl != 1 ? 0 : CMF_CANRENAME) | (fExplore ? CMF_EXPLORE : 0);

                /* let the ContextMenu merge its items in */
                if (SUCCEEDED(pCM->QueryContextMenu(hMenu, 0, FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST, wFlags )))
                {
                    if (FolderSettings.fFlags & FWF_DESKTOP)
                        SetMenuDefaultItem(hMenu, FCIDM_SHVIEW_OPEN, MF_BYCOMMAND);

                    TRACE("-- track popup\n");
                    uCommand = TrackPopupMenu(hMenu,
                                              TPM_LEFTALIGN | TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                                              x, y, 0, m_hWnd, NULL);

                    if (uCommand > 0)
                    {
                        TRACE("-- uCommand=%u\n", uCommand);

                        if (uCommand == FCIDM_SHVIEW_OPEN && pCommDlgBrowser.p != NULL)
                        {
                            TRACE("-- dlg: OnDefaultCommand\n");
                            if (OnDefaultCommand() != S_OK)
                                OpenSelectedItems();
                        }
                        else
                        {
                            TRACE("-- explore -- invoke command\n");
                            ZeroMemory(&cmi, sizeof(cmi));
                            cmi.cbSize = sizeof(cmi);
                            cmi.hwnd = hWndParent; /* this window has to answer CWM_GETISHELLBROWSER */
                            cmi.lpVerb = (LPCSTR)MAKEINTRESOURCEA(uCommand);
                            pCM->InvokeCommand(&cmi);
                        }
                    }

                    hResult = IUnknown_SetSite(pCM, NULL);
                    DestroyMenu(hMenu);
                }
            }
            pCM.Release();
        }
    }
    else    /* background context menu */
    {
        hMenu = CreatePopupMenu();

        CDefFolderMenu_Create2(NULL, NULL, cidl, (LPCITEMIDLIST*)apidl, pSFParent, NULL, 0, NULL, (IContextMenu**)&pCM);
        pCM->QueryContextMenu(hMenu, 0, FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST, 0);

        uCommand = TrackPopupMenu(hMenu,
                                  TPM_LEFTALIGN | TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                                  x, y, 0, m_hWnd, NULL);
        DestroyMenu(hMenu);

        TRACE("-- (%p)->(uCommand=0x%08x )\n", this, uCommand);

        ZeroMemory(&cmi, sizeof(cmi));
        cmi.cbSize = sizeof(cmi);
        cmi.lpVerb = (LPCSTR)MAKEINTRESOURCEA(uCommand);
        cmi.hwnd = hWndParent;
        pCM->InvokeCommand(&cmi);

        pCM.Release();
    }

    return 0;
}

/**********************************************************
 *    ##### message handling #####
 */

/**********************************************************
*  ShellView_OnSize()
*/
LRESULT CDefView::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    WORD                                wWidth;
    WORD                                wHeight;

    wWidth = LOWORD(lParam);
    wHeight = HIWORD(lParam);

    TRACE("%p width=%u height=%u\n", this, wWidth, wHeight);

    /*resize the ListView to fit our window*/
    if (hWndList)
    {
        ::MoveWindow(hWndList, 0, 0, wWidth, wHeight, TRUE);
    }

    return 0;
}

/**********************************************************
* ShellView_OnDeactivate()
*
* NOTES
*  internal
*/
void CDefView::OnDeactivate()
{
    TRACE("%p\n", this);

    if (uState != SVUIA_DEACTIVATE)
    {
        if (hMenu)
        {
            pShellBrowser->SetMenuSB(0, 0, 0);
            pShellBrowser->RemoveMenusSB(hMenu);
            DestroyMenu(hMenu);
            hMenu = 0;
        }

        uState = SVUIA_DEACTIVATE;
    }
}

void CDefView::DoActivate(UINT uState)
{
    OLEMENUGROUPWIDTHS                    omw = { {0, 0, 0, 0, 0, 0} };
    MENUITEMINFOA                        mii;
    CHAR                                szText[MAX_PATH];

    TRACE("%p uState=%x\n", this, uState);

    /*don't do anything if the state isn't really changing */
    if (this->uState == uState)
    {
        return;
    }

    OnDeactivate();

    /*only do This if we are active */
    if(uState != SVUIA_DEACTIVATE)
    {
        /*merge the menus */
        hMenu = CreateMenu();

        if(hMenu)
        {
            pShellBrowser->InsertMenusSB(hMenu, &omw);
            TRACE("-- after fnInsertMenusSB\n");

            /*build the top level menu get the menu item's text*/
            strcpy(szText, "dummy 31");

            ZeroMemory(&mii, sizeof(mii));
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_STATE;
            mii.fType = MFT_STRING;
            mii.fState = MFS_ENABLED;
            mii.dwTypeData = szText;
            mii.hSubMenu = BuildFileMenu();

            /*insert our menu into the menu bar*/
            if (mii.hSubMenu)
            {
                InsertMenuItemA(hMenu, FCIDM_MENU_HELP, FALSE, &mii);
            }

            /*get the view menu so we can merge with it*/
            ZeroMemory(&mii, sizeof(mii));
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_SUBMENU;

            if (GetMenuItemInfoA(hMenu, FCIDM_MENU_VIEW, FALSE, &mii))
            {
                MergeViewMenu(mii.hSubMenu);
            }

            /*add the items that should only be added if we have the focus*/
            if (SVUIA_ACTIVATE_FOCUS == uState)
            {
                /*get the file menu so we can merge with it */
                ZeroMemory(&mii, sizeof(mii));
                mii.cbSize = sizeof(mii);
                mii.fMask = MIIM_SUBMENU;

                if (GetMenuItemInfoA(hMenu, FCIDM_MENU_FILE, FALSE, &mii))
                {
                    MergeFileMenu(mii.hSubMenu);
                }
            }

            TRACE("-- before fnSetMenuSB\n");
            pShellBrowser->SetMenuSB(hMenu, 0, m_hWnd);
        }
    }
    this->uState = uState;
    TRACE("--\n");
}

/**********************************************************
* ShellView_OnActivate()
*/
LRESULT CDefView::OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    DoActivate(SVUIA_ACTIVATE_FOCUS);
    return 0;
}

/**********************************************************
*  ShellView_OnSetFocus()
*
*/
LRESULT CDefView::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("%p\n", this);

    /* Tell the browser one of our windows has received the focus. This
    should always be done before merging menus (OnActivate merges the
    menus) if one of our windows has the focus.*/

    pShellBrowser->OnViewWindowActive((IShellView *)this);
    DoActivate(SVUIA_ACTIVATE_FOCUS);

    /* Set the focus to the listview */
    ::SetFocus(hWndList);

    /* Notify the ICommDlgBrowser interface */
    OnStateChange(CDBOSC_SETFOCUS);

    return 0;
}

/**********************************************************
* ShellView_OnKillFocus()
*/
LRESULT CDefView::OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("(%p) stub\n", this);

    DoActivate(SVUIA_ACTIVATE_NOFOCUS);
    /* Notify the ICommDlgBrowser */
    OnStateChange(CDBOSC_KILLFOCUS);

    return 0;
}

/**********************************************************
* ShellView_OnCommand()
*
* NOTES
*    the CmdID's are the ones from the context menu
*/
LRESULT CDefView::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    DWORD                                dwCmdID;
    DWORD                                dwCmd;
    HWND                                hwndCmd;

    dwCmdID = GET_WM_COMMAND_ID(wParam, lParam);
    dwCmd = GET_WM_COMMAND_CMD(wParam, lParam);
    hwndCmd = GET_WM_COMMAND_HWND(wParam, lParam);

    TRACE("(%p)->(0x%08x 0x%08x %p) stub\n", this, dwCmdID, dwCmd, hwndCmd);

    switch (dwCmdID)
    {
        case FCIDM_SHVIEW_SMALLICON:
            FolderSettings.ViewMode = FVM_SMALLICON;
            SetStyle (LVS_SMALLICON, LVS_TYPEMASK);
            CheckToolbar();
            break;

        case FCIDM_SHVIEW_BIGICON:
            FolderSettings.ViewMode = FVM_ICON;
            SetStyle (LVS_ICON, LVS_TYPEMASK);
            CheckToolbar();
            break;

        case FCIDM_SHVIEW_LISTVIEW:
            FolderSettings.ViewMode = FVM_LIST;
            SetStyle (LVS_LIST, LVS_TYPEMASK);
            CheckToolbar();
            break;

        case FCIDM_SHVIEW_REPORTVIEW:
            FolderSettings.ViewMode = FVM_DETAILS;
            SetStyle (LVS_REPORT, LVS_TYPEMASK);
            CheckToolbar();
            break;

            /* the menu-ID's for sorting are 0x30... see shrec.rc */
        case 0x30:
        case 0x31:
        case 0x32:
        case 0x33:
            ListViewSortInfo.nHeaderID = (LPARAM) (dwCmdID - 0x30);
            ListViewSortInfo.bIsAscending = TRUE;
            ListViewSortInfo.nLastHeaderID = ListViewSortInfo.nHeaderID;
            SendMessageA(hWndList, LVM_SORTITEMS, (WPARAM) &ListViewSortInfo, (LPARAM)ListViewCompareItems);
            break;

        default:
            TRACE("-- COMMAND 0x%04x unhandled\n", dwCmdID);
    }

    return 0;
}

/**********************************************************
* ShellView_OnNotify()
*/

LRESULT CDefView::OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    UINT                                CtlID;
    LPNMHDR                                lpnmh;
    LPNMLISTVIEW                        lpnmlv;
    NMLVDISPINFOW                        *lpdi;
    LPITEMIDLIST                        pidl;
    BOOL                                unused;

    CtlID = wParam;
    lpnmh = (LPNMHDR)lParam;
    lpnmlv = (LPNMLISTVIEW)lpnmh;
    lpdi = (NMLVDISPINFOW *)lpnmh;

    TRACE("%p CtlID=%u lpnmh->code=%x\n", this, CtlID, lpnmh->code);

    switch (lpnmh->code)
    {
        case NM_SETFOCUS:
            TRACE("-- NM_SETFOCUS %p\n", this);
            OnSetFocus(0, 0, 0, unused);
            break;

        case NM_KILLFOCUS:
            TRACE("-- NM_KILLFOCUS %p\n", this);
            OnDeactivate();
            /* Notify the ICommDlgBrowser interface */
            OnStateChange(CDBOSC_KILLFOCUS);
            break;

        case NM_CUSTOMDRAW:
            TRACE("-- NM_CUSTOMDRAW %p\n", this);
            return CDRF_DODEFAULT;

        case NM_RELEASEDCAPTURE:
            TRACE("-- NM_RELEASEDCAPTURE %p\n", this);
            break;

        case NM_CLICK:
            TRACE("-- NM_CLICK %p\n", this);
            break;

        case NM_RCLICK:
            TRACE("-- NM_RCLICK %p\n", this);
            break;

        case NM_DBLCLK:
            TRACE("-- NM_DBLCLK %p\n", this);
            if (OnDefaultCommand() != S_OK) OpenSelectedItems();
            break;

        case NM_RETURN:
            TRACE("-- NM_RETURN %p\n", this);
            if (OnDefaultCommand() != S_OK) OpenSelectedItems();
            break;

        case HDN_ENDTRACKW:
            TRACE("-- HDN_ENDTRACKW %p\n", this);
            /*nColumn1 = ListView_GetColumnWidth(hWndList, 0);
            nColumn2 = ListView_GetColumnWidth(hWndList, 1);*/
            break;

        case LVN_DELETEITEM:
            TRACE("-- LVN_DELETEITEM %p\n", this);
            SHFree((LPITEMIDLIST)lpnmlv->lParam);     /*delete the pidl because we made a copy of it*/
            break;

        case LVN_DELETEALLITEMS:
            TRACE("-- LVN_DELETEALLITEMS %p\n", this);
            return FALSE;

        case LVN_INSERTITEM:
            TRACE("-- LVN_INSERTITEM (STUB)%p\n", this);
            break;

        case LVN_ITEMACTIVATE:
            TRACE("-- LVN_ITEMACTIVATE %p\n", this);
            OnStateChange(CDBOSC_SELCHANGE);  /* the browser will get the IDataObject now */
            break;

        case LVN_COLUMNCLICK:
            ListViewSortInfo.nHeaderID = lpnmlv->iSubItem;
            if (ListViewSortInfo.nLastHeaderID == ListViewSortInfo.nHeaderID)
            {
                ListViewSortInfo.bIsAscending = !ListViewSortInfo.bIsAscending;
            }
            else
            {
                ListViewSortInfo.bIsAscending = TRUE;
            }
            ListViewSortInfo.nLastHeaderID = ListViewSortInfo.nHeaderID;

            SendMessageW(lpnmlv->hdr.hwndFrom, LVM_SORTITEMS, (WPARAM) &ListViewSortInfo, (LPARAM)ListViewCompareItems);
            break;

        case LVN_GETDISPINFOA:
        case LVN_GETDISPINFOW:
            TRACE("-- LVN_GETDISPINFO %p\n", this);
            pidl = (LPITEMIDLIST)lpdi->item.lParam;

            if (lpdi->item.mask & LVIF_TEXT)    /* text requested */
            {
                if (pSF2Parent)
                {
                    SHELLDETAILS sd;
                    if (FAILED(pSF2Parent->GetDetailsOf(pidl, lpdi->item.iSubItem, &sd)))
                    {
                        FIXME("failed to get details\n");
                        break;
                    }

                    if (lpnmh->code == LVN_GETDISPINFOA)
                    {
                        /* shouldn't happen */
                        NMLVDISPINFOA *lpdiA = (NMLVDISPINFOA *)lpnmh;
                        StrRetToStrNA( lpdiA->item.pszText, lpdiA->item.cchTextMax, &sd.str, NULL);
                        TRACE("-- text=%s\n", lpdiA->item.pszText);
                    }
                    else /* LVN_GETDISPINFOW */
                    {
                        StrRetToStrNW( lpdi->item.pszText, lpdi->item.cchTextMax, &sd.str, NULL);
                        TRACE("-- text=%s\n", debugstr_w(lpdi->item.pszText));
                    }
                }
                else
                {
                    FIXME("no SF2\n");
                }
            }
            if(lpdi->item.mask & LVIF_IMAGE)    /* image requested */
            {
                lpdi->item.iImage = SHMapPIDLToSystemImageListIndex(pSFParent, pidl, 0);
            }
            lpdi->item.mask |= LVIF_DI_SETITEM;
            break;

        case LVN_ITEMCHANGED:
            TRACE("-- LVN_ITEMCHANGED %p\n", this);
            OnStateChange(CDBOSC_SELCHANGE);  /* the browser will get the IDataObject now */
            break;

        case LVN_BEGINDRAG:
        case LVN_BEGINRDRAG:
            TRACE("-- LVN_BEGINDRAG\n");

            if (GetSelections())
            {
                IDataObject * pda;
                DWORD dwAttributes = SFGAO_CANLINK;
                DWORD dwEffect = DROPEFFECT_COPY | DROPEFFECT_MOVE;

                if (SUCCEEDED(pSFParent->GetUIObjectOf(m_hWnd, cidl, (LPCITEMIDLIST*)apidl, IID_IDataObject, 0, (LPVOID *)&pda)))
                {
                    IDropSource * pds = (IDropSource *)this;    /* own DropSource interface */

                    if (SUCCEEDED(pSFParent->GetAttributesOf(cidl, (LPCITEMIDLIST*)apidl, &dwAttributes)))
                    {
                        if (dwAttributes & SFGAO_CANLINK)
                        {
                            dwEffect |= DROPEFFECT_LINK;
                        }
                    }

                    if (pds)
                    {
                        DWORD dwEffect2;
                        DoDragDrop(pda, pds, dwEffect, &dwEffect2);
                    }
                    pda->Release();
                }
            }
            break;

        case LVN_BEGINLABELEDITW:
        {
            DWORD dwAttr = SFGAO_CANRENAME;
            pidl = (LPITEMIDLIST)lpdi->item.lParam;

            TRACE("-- LVN_BEGINLABELEDITW %p\n", this);

            pSFParent->GetAttributesOf(1, (LPCITEMIDLIST*)&pidl, &dwAttr);
            if (SFGAO_CANRENAME & dwAttr)
            {
                return FALSE;
            }
            return TRUE;
        }

        case LVN_ENDLABELEDITW:
        {
            TRACE("-- LVN_ENDLABELEDITW %p\n", this);
            if (lpdi->item.pszText)
            {
                HRESULT hr;
                LVITEMW lvItem;

                lvItem.iItem = lpdi->item.iItem;
                lvItem.iSubItem = 0;
                lvItem.mask = LVIF_PARAM;
                SendMessageW(hWndList, LVM_GETITEMW, 0, (LPARAM) &lvItem);

                pidl = (LPITEMIDLIST)lpdi->item.lParam;
                hr = pSFParent->SetNameOf(0, pidl, lpdi->item.pszText, SHGDN_INFOLDER, &pidl);

                if (SUCCEEDED(hr) && pidl)
                {
                    lvItem.mask = LVIF_PARAM|LVIF_IMAGE;
                    lvItem.lParam = (LPARAM)pidl;
                    lvItem.iImage = SHMapPIDLToSystemImageListIndex(pSFParent, pidl, 0);
                    SendMessageW(hWndList, LVM_SETITEMW, 0, (LPARAM) &lvItem);
                    SendMessageW(hWndList, LVM_UPDATE, lpdi->item.iItem, 0);

                    return TRUE;
                }
            }

            return FALSE;
        }

        case LVN_KEYDOWN:
        {
            /*  MSG msg;
                msg.hwnd = m_hWnd;
                msg.message = WM_KEYDOWN;
                msg.wParam = plvKeyDown->wVKey;
                msg.lParam = 0;
                msg.time = 0;
                msg.pt = 0;*/

            LPNMLVKEYDOWN plvKeyDown = (LPNMLVKEYDOWN) lpnmh;
            SHORT ctrl = GetKeyState(VK_CONTROL) & 0x8000;

            /* initiate a rename of the selected file or directory */
            if (plvKeyDown->wVKey == VK_F2)
            {
                /* see how many files are selected */
                int i = ListView_GetSelectedCount(hWndList);

                /* get selected item */
                if (i == 1)
                {
                    /* get selected item */
                    i = ListView_GetNextItem(hWndList, -1, LVNI_SELECTED);

                    SendMessageW(hWndList, LVM_ENSUREVISIBLE, i, 0);
                    SendMessageW(hWndList, LVM_EDITLABELW, i, 0);
                }
            }
#if 0
            TranslateAccelerator(m_hWnd, hAccel, &msg)
#endif
            else if(plvKeyDown->wVKey == VK_DELETE)
            {
                UINT i;
                int item_index;
                LVITEMA item;
                LPITEMIDLIST* pItems;
                ISFHelper *psfhlp;

                pSFParent->QueryInterface(IID_ISFHelper,
                                          (LPVOID*)&psfhlp);

                if (psfhlp == NULL)
                    break;

                if (!(i = ListView_GetSelectedCount(hWndList)))
                    break;

                /* allocate memory for the pidl array */
                pItems = (LPITEMIDLIST *)HeapAlloc(GetProcessHeap(), 0,
                                                   sizeof(LPITEMIDLIST) * i);

                /* retrieve all selected items */
                i = 0;
                item_index = -1;
                while (ListView_GetSelectedCount(hWndList) > i)
                {
                    /* get selected item */
                    item_index = ListView_GetNextItem(hWndList,
                                                      item_index, LVNI_SELECTED);
                    item.iItem = item_index;
                    item.mask = LVIF_PARAM;
                    SendMessageA(hWndList, LVM_GETITEMA, 0, (LPARAM) &item);

                    /* get item pidl */
                    pItems[i] = (LPITEMIDLIST)item.lParam;

                    i++;
                }

                /* perform the item deletion */
                psfhlp->DeleteItems(i, (LPCITEMIDLIST*)pItems);

                /* free pidl array memory */
                HeapFree(GetProcessHeap(), 0, pItems);
            }
            /* Initiate a refresh */
            else if (plvKeyDown->wVKey == VK_F5)
            {
                Refresh();
            }
            else if (plvKeyDown->wVKey == VK_BACK)
            {
                LPSHELLBROWSER lpSb;
                if ((lpSb = (LPSHELLBROWSER)SendMessageW(hWndParent, CWM_GETISHELLBROWSER, 0, 0)))
                {
                    lpSb->BrowseObject(NULL, SBSP_PARENT);
                }
            }
            else if (plvKeyDown->wVKey == 'C' && ctrl)
            {
                if (GetSelections())
                {
                    CComPtr<IDataObject> pda;

                    if (SUCCEEDED(pSFParent->GetUIObjectOf(m_hWnd, cidl, (LPCITEMIDLIST*)apidl, IID_IDataObject, 0, (LPVOID *)&pda)))
                    {
                        HRESULT hr = OleSetClipboard(pda);
                        if (FAILED(hr))
                        {
                            WARN("OleSetClipboard failed");
                        }
                    }
                }
                break;
            }
            else if(plvKeyDown->wVKey == 'V' && ctrl)
            {
                CComPtr<IDataObject> pda;
                STGMEDIUM medium;
                FORMATETC formatetc;
                LPITEMIDLIST * apidl;
                LPITEMIDLIST pidl;
                CComPtr<IShellFolder> psfFrom;
                CComPtr<IShellFolder> psfDesktop;
                CComPtr<IShellFolder> psfTarget;
                LPIDA lpcida;
                CComPtr<ISFHelper> psfhlpdst;
                CComPtr<ISFHelper> psfhlpsrc;
                HRESULT hr;

                hr = OleGetClipboard(&pda);
                if (hr != S_OK)
                {
                    ERR("Failed to get clipboard with %lx\n", hr);
                    return E_FAIL;
                }

                InitFormatEtc(formatetc, RegisterClipboardFormatW(CFSTR_SHELLIDLIST), TYMED_HGLOBAL);
                hr = pda->GetData(&formatetc, &medium);

                if (FAILED(hr))
                {
                    ERR("Failed to get clipboard data with %lx\n", hr);
                    return E_FAIL;
                }

                /* lock the handle */
                lpcida = (LPIDA)GlobalLock(medium.hGlobal);
                if (!lpcida)
                {
                    ERR("failed to lock pidl\n");
                    ReleaseStgMedium(&medium);
                    return E_FAIL;
                }

                /* convert the data into pidl */
                apidl = _ILCopyCidaToaPidl(&pidl, lpcida);

                if (!apidl)
                {
                    ERR("failed to copy pidl\n");
                    return E_FAIL;
                }

                if (FAILED(SHGetDesktopFolder(&psfDesktop)))
                {
                    ERR("failed to get desktop folder\n");
                    SHFree(pidl);
                    _ILFreeaPidl(apidl, lpcida->cidl);
                    ReleaseStgMedium(&medium);
                    return E_FAIL;
                }

                if (_ILIsDesktop(pidl))
                {
                    /* use desktop shellfolder */
                    psfFrom = psfDesktop;
                }
                else if (FAILED(psfDesktop->BindToObject(pidl, NULL, IID_IShellFolder, (LPVOID*)&psfFrom)))
                {
                    ERR("no IShellFolder\n");

                    SHFree(pidl);
                    _ILFreeaPidl(apidl, lpcida->cidl);
                    ReleaseStgMedium(&medium);

                    return E_FAIL;
                }

                psfTarget = pSFParent;


                /* get source and destination shellfolder */
                if (FAILED(psfTarget->QueryInterface(IID_ISFHelper, (LPVOID*)&psfhlpdst)))
                {
                    ERR("no IID_ISFHelper for destination\n");

                    SHFree(pidl);
                    _ILFreeaPidl(apidl, lpcida->cidl);
                    ReleaseStgMedium(&medium);

                    return E_FAIL;
                }

                if (FAILED(psfFrom->QueryInterface(IID_ISFHelper, (LPVOID*)&psfhlpsrc)))
                {
                    ERR("no IID_ISFHelper for source\n");

                    SHFree(pidl);
                    _ILFreeaPidl(apidl, lpcida->cidl);
                    ReleaseStgMedium(&medium);
                    return E_FAIL;
                }

                /* FIXXME
                * do we want to perform a copy or move ???
                */
                hr = psfhlpdst->CopyItems(psfFrom, lpcida->cidl, (LPCITEMIDLIST*)apidl);

                SHFree(pidl);
                _ILFreeaPidl(apidl, lpcida->cidl);
                ReleaseStgMedium(&medium);

                TRACE("paste end hr %x\n", hr);
                break;
            }
            else
                FIXME("LVN_KEYDOWN key=0x%08x\n", plvKeyDown->wVKey);
        }
        break;

        default:
            TRACE("-- %p WM_COMMAND %x unhandled\n", this, lpnmh->code);
            break;
    }

    return 0;
}

/**********************************************************
* ShellView_OnChange()
*/
LRESULT CDefView::OnChangeNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    LPITEMIDLIST                        *Pidls;

    Pidls = (LPITEMIDLIST *)wParam;

    TRACE("(%p)(%p,%p,0x%08x)\n", this, Pidls[0], Pidls[1], lParam);

    switch (lParam)
    {
        case SHCNE_MKDIR:
        case SHCNE_CREATE:
            LV_AddItem(Pidls[0]);
            break;

        case SHCNE_RMDIR:
        case SHCNE_DELETE:
            LV_DeleteItem(Pidls[0]);
            break;

        case SHCNE_RENAMEFOLDER:
        case SHCNE_RENAMEITEM:
            LV_RenameItem(Pidls[0], Pidls[1]);
            break;

        case SHCNE_UPDATEITEM:
            break;
    }

    return TRUE;
}

/**********************************************************
*  CDefView::OnCustomItem
*/
LRESULT CDefView::OnCustomItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (!pCM.p)
    {
        /* no menu */
        ERR("no menu!!!\n");
        return FALSE;
    }

    if (pCM.p->HandleMenuMsg(uMsg, (WPARAM)m_hWnd, lParam) == S_OK)
        return TRUE;
    else
        return FALSE;
}

LRESULT CDefView::OnSettingChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    /* Wallpaper setting affects drop shadows effect */
    if (wParam == SPI_SETDESKWALLPAPER || wParam == 0)
        UpdateListColors();

    return S_OK;
}

/**********************************************************
*
*
*  The INTERFACE of the IShellView object
*
*
**********************************************************
*/

/**********************************************************
*  ShellView_GetWindow
*/
HRESULT WINAPI CDefView::GetWindow(HWND *phWnd)
{
    TRACE("(%p)\n", this);

    *phWnd = m_hWnd;

    return S_OK;
}

HRESULT WINAPI CDefView::ContextSensitiveHelp(BOOL fEnterMode)
{
    FIXME("(%p) stub\n", this);

    return E_NOTIMPL;
}

/**********************************************************
* IShellView_TranslateAccelerator
*
* FIXME:
*  use the accel functions
*/
HRESULT WINAPI CDefView::TranslateAccelerator(LPMSG lpmsg)
{
#if 0
    FIXME("(%p)->(%p: hwnd=%x msg=%x lp=%x wp=%x) stub\n", this, lpmsg, lpmsg->hwnd, lpmsg->message, lpmsg->lParam, lpmsg->wParam);
#endif

    if (lpmsg->message >= WM_KEYFIRST && lpmsg->message >= WM_KEYLAST)
    {
        TRACE("-- key=0x04%lx\n", lpmsg->wParam) ;
    }

    return S_FALSE; /* not handled */
}

HRESULT WINAPI CDefView::EnableModeless(BOOL fEnable)
{
    FIXME("(%p) stub\n", this);

    return E_NOTIMPL;
}

HRESULT WINAPI CDefView::UIActivate(UINT uState)
{
    /*
        CHAR    szName[MAX_PATH];
    */
    LRESULT    lResult;
    int    nPartArray[1] = { -1};

    TRACE("(%p)->(state=%x) stub\n", this, uState);

    /*don't do anything if the state isn't really changing*/
    if (this->uState == uState)
    {
        return S_OK;
    }

    /*OnActivate handles the menu merging and internal state*/
    DoActivate(uState);

    /*only do This if we are active*/
    if (uState != SVUIA_DEACTIVATE)
    {

        /*
            GetFolderPath is not a method of IShellFolder
            IShellFolder_GetFolderPath( pSFParent, szName, sizeof(szName) );
        */
        /* set the number of parts */
        pShellBrowser->SendControlMsg(FCW_STATUS, SB_SETPARTS, 1, (LPARAM)nPartArray, &lResult);

        /* set the text for the parts */
        /*
            pShellBrowser->SendControlMsg(FCW_STATUS, SB_SETTEXTA, 0, (LPARAM)szName, &lResult);
        */
    }

    return S_OK;
}

HRESULT WINAPI CDefView::Refresh()
{
    TRACE("(%p)\n", this);

    SendMessageW(hWndList, LVM_DELETEALLITEMS, 0, 0);
    FillList();

    return S_OK;
}

HRESULT WINAPI CDefView::CreateViewWindow(IShellView *lpPrevView, LPCFOLDERSETTINGS lpfs, IShellBrowser *psb, RECT *prcView, HWND *phWnd)
{
    *phWnd = 0;

    TRACE("(%p)->(shlview=%p set=%p shlbrs=%p rec=%p hwnd=%p) incomplete\n", this, lpPrevView, lpfs, psb, prcView, phWnd);

    if (lpfs != NULL)
        TRACE("-- vmode=%x flags=%x\n", lpfs->ViewMode, lpfs->fFlags);
    if (prcView != NULL)
        TRACE("-- left=%i top=%i right=%i bottom=%i\n", prcView->left, prcView->top, prcView->right, prcView->bottom);

    /* Validate the Shell Browser */
    if (psb == NULL)
        return E_UNEXPECTED;

    /*set up the member variables*/
    pShellBrowser = psb;
    FolderSettings = *lpfs;

    /*get our parent window*/
    pShellBrowser->GetWindow(&hWndParent);

    /* try to get the ICommDlgBrowserInterface, adds a reference !!! */
    pCommDlgBrowser = NULL;
    if (SUCCEEDED(pShellBrowser->QueryInterface(IID_ICommDlgBrowser, (LPVOID *)&pCommDlgBrowser)))
    {
        TRACE("-- CommDlgBrowser\n");
    }

    Create(hWndParent, prcView, NULL, WS_CHILD | WS_TABSTOP, 0, 0U);
    if (m_hWnd == NULL)
        return E_FAIL;

    *phWnd = m_hWnd;

    CheckToolbar();

    if (!*phWnd)
        return E_FAIL;

    SetWindowPos(HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    UpdateWindow();

    return S_OK;
}

HRESULT WINAPI CDefView::DestroyViewWindow()
{
    TRACE("(%p)\n", this);

    /*Make absolutely sure all our UI is cleaned up.*/
    UIActivate(SVUIA_DEACTIVATE);

    if (hMenu)
    {
        DestroyMenu(hMenu);
    }

    DestroyWindow();
    pShellBrowser.Release();
    pCommDlgBrowser.Release();

    return S_OK;
}

HRESULT WINAPI CDefView::GetCurrentInfo(LPFOLDERSETTINGS lpfs)
{
    TRACE("(%p)->(%p) vmode=%x flags=%x\n", this, lpfs,
          FolderSettings.ViewMode, FolderSettings.fFlags);

    if (!lpfs)
        return E_INVALIDARG;

    *lpfs = FolderSettings;
    return NOERROR;
}

HRESULT WINAPI CDefView::AddPropertySheetPages(DWORD dwReserved, LPFNADDPROPSHEETPAGE lpfn, LPARAM lparam)
{
    FIXME("(%p) stub\n", this);

    return E_NOTIMPL;
}

HRESULT WINAPI CDefView::SaveViewState()
{
    FIXME("(%p) stub\n", this);

    return S_OK;
}

HRESULT WINAPI CDefView::SelectItem(LPCITEMIDLIST pidl, UINT uFlags)
{
    int i;

    TRACE("(%p)->(pidl=%p, 0x%08x) stub\n", this, pidl, uFlags);

    i = LV_FindItemByPidl(pidl);

    if (i != -1)
    {
        LVITEMW lvItem;

        if(uFlags & SVSI_ENSUREVISIBLE)
            SendMessageW(hWndList, LVM_ENSUREVISIBLE, i, 0);

        lvItem.mask = LVIF_STATE;
        lvItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
        lvItem.iItem = 0;
        lvItem.iSubItem = 0;

        while (SendMessageW(hWndList, LVM_GETITEMW, 0, (LPARAM) &lvItem))
        {
            if (lvItem.iItem == i)
            {
                if (uFlags & SVSI_SELECT)
                    lvItem.state |= LVIS_SELECTED;
                else
                    lvItem.state &= ~LVIS_SELECTED;

                if (uFlags & SVSI_FOCUSED)
                    lvItem.state &= ~LVIS_FOCUSED;
            }
            else
            {
                if (uFlags & SVSI_DESELECTOTHERS)
                    lvItem.state &= ~LVIS_SELECTED;
            }

            SendMessageW(hWndList, LVM_SETITEMW, 0, (LPARAM) &lvItem);
            lvItem.iItem++;
        }


        if(uFlags & SVSI_EDIT)
            SendMessageW(hWndList, LVM_EDITLABELW, i, 0);
    }

    return S_OK;
}

HRESULT WINAPI CDefView::GetItemObject(UINT uItem, REFIID riid, LPVOID *ppvOut)
{
    HRESULT hr = E_NOINTERFACE;

    TRACE("(%p)->(uItem=0x%08x,\n\tIID=%s, ppv=%p)\n", this, uItem, debugstr_guid(&riid), ppvOut);

    *ppvOut = NULL;

    switch (uItem)
    {
        case SVGIO_BACKGROUND:
            if (IsEqualIID(riid, IID_IContextMenu))
            {
                //*ppvOut = ISvBgCm_Constructor(pSFParent, FALSE);
                CDefFolderMenu_Create2(NULL, NULL, cidl, (LPCITEMIDLIST*)apidl, pSFParent, NULL, 0, NULL, (IContextMenu**)ppvOut);
                if (!ppvOut)
                    hr = E_OUTOFMEMORY;
                else
                    hr = S_OK;
            }
            break;

        case SVGIO_SELECTION:
            GetSelections();
            hr = pSFParent->GetUIObjectOf(m_hWnd, cidl, (LPCITEMIDLIST*)apidl, riid, 0, ppvOut);
            break;
    }

    TRACE("-- (%p)->(interface=%p)\n", this, *ppvOut);

    return hr;
}

HRESULT STDMETHODCALLTYPE CDefView::GetCurrentViewMode(UINT *pViewMode)
{
    TRACE("(%p)->(%p), stub\n", this, pViewMode);

    if (!pViewMode)
        return E_INVALIDARG;

    *pViewMode = this->FolderSettings.ViewMode;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CDefView::SetCurrentViewMode(UINT ViewMode)
{
    DWORD dwStyle;
    TRACE("(%p)->(%u), stub\n", this, ViewMode);

    if ((ViewMode < FVM_FIRST || ViewMode > FVM_LAST) /* && (ViewMode != FVM_AUTO) */ )
        return E_INVALIDARG;

    /* Windows before Vista uses LVM_SETVIEW and possibly
       LVM_SETEXTENDEDLISTVIEWSTYLE to set the style of the listview,
       while later versions seem to accomplish this through other
       means. */
    switch (ViewMode)
    {
        case FVM_ICON:
            dwStyle = LVS_ICON;
            break;
        case FVM_DETAILS:
            dwStyle = LVS_REPORT;
            break;
        case FVM_SMALLICON:
            dwStyle = LVS_SMALLICON;
            break;
        case FVM_LIST:
            dwStyle = LVS_LIST;
            break;
        default:
        {
            FIXME("ViewMode %d not implemented\n", ViewMode);
            dwStyle = LVS_LIST;
            break;
        }
    }

    SetStyle(dwStyle, LVS_TYPEMASK);

    /* This will not necessarily be the actual mode set above.
       This mimics the behavior of Windows XP. */
    this->FolderSettings.ViewMode = ViewMode;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CDefView::GetFolder(REFIID riid, void **ppv)
{
    if (pSFParent == NULL)
        return E_FAIL;

    return pSFParent->QueryInterface(riid, ppv);
}

HRESULT STDMETHODCALLTYPE CDefView::Item(int iItemIndex, LPITEMIDLIST *ppidl)
{
    LVITEMW item;

    TRACE("(%p)->(%d %p)\n", this, iItemIndex, ppidl);

    item.mask = LVIF_PARAM;
    item.iItem = iItemIndex;

    if (SendMessageW(this->hWndList, LVM_GETITEMW, 0, (LPARAM)&item))
    {
        *ppidl = ILClone((PITEMID_CHILD)item.lParam);
        return S_OK;
    }

    *ppidl = 0;

    return E_INVALIDARG;
}

HRESULT STDMETHODCALLTYPE CDefView::ItemCount(UINT uFlags, int *pcItems)
{
    TRACE("(%p)->(%u %p)\n", this, uFlags, pcItems);

    if (uFlags != SVGIO_ALLVIEW)
        FIXME("some flags unsupported, %x\n", uFlags & ~SVGIO_ALLVIEW);

    *pcItems = SendMessageW(this->hWndList, LVM_GETITEMCOUNT, 0, 0);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CDefView::Items(UINT uFlags, REFIID riid, void **ppv)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDefView::GetSelectionMarkedItem(int *piItem)
{
    TRACE("(%p)->(%p)\n", this, piItem);

    *piItem = SendMessageW(this->hWndList, LVM_GETSELECTIONMARK, 0, 0);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CDefView::GetFocusedItem(int *piItem)
{
    TRACE("(%p)->(%p)\n", this, piItem);

    *piItem = SendMessageW(this->hWndList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CDefView::GetItemPosition(LPCITEMIDLIST pidl, POINT *ppt)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDefView::GetSpacing(POINT *ppt)
{
    TRACE("(%p)->(%p)\n", this, ppt);

    if (NULL == this->hWndList) return S_FALSE;

    if (ppt)
    {
        const DWORD ret = SendMessageW(this->hWndList, LVM_GETITEMSPACING, 0, 0);

        ppt->x = LOWORD(ret);
        ppt->y = HIWORD(ret);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CDefView::GetDefaultSpacing(POINT *ppt)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDefView::GetAutoArrange()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDefView::SelectItem(int iItem, DWORD dwFlags)
{
    LVITEMW lvItem;

    TRACE("(%p)->(%d, %x)\n", this, iItem, dwFlags);

    lvItem.state = 0;
    lvItem.stateMask = LVIS_SELECTED;

    if (dwFlags & SVSI_ENSUREVISIBLE)
        SendMessageW(this->hWndList, LVM_ENSUREVISIBLE, iItem, 0);

    /* all items */
    if (dwFlags & SVSI_DESELECTOTHERS)
        SendMessageW(this->hWndList, LVM_SETITEMSTATE, -1, (LPARAM)&lvItem);

    /* this item */
    if (dwFlags & SVSI_SELECT)
        lvItem.state |= LVIS_SELECTED;

    if (dwFlags & SVSI_FOCUSED)
        lvItem.stateMask |= LVIS_FOCUSED;

    SendMessageW(this->hWndList, LVM_SETITEMSTATE, iItem, (LPARAM)&lvItem);

    if (dwFlags & SVSI_EDIT)
        SendMessageW(this->hWndList, LVM_EDITLABELW, iItem, 0);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CDefView::SelectAndPositionItems(UINT cidl, LPCITEMIDLIST *apidl, POINT *apt, DWORD dwFlags)
{
    return E_NOTIMPL;
}

/**********************************************************
 * ISVOleCmdTarget_QueryStatus (IOleCommandTarget)
 */
HRESULT WINAPI CDefView::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD *prgCmds, OLECMDTEXT *pCmdText)
{
    FIXME("(%p)->(%p(%s) 0x%08x %p %p\n",
          this, pguidCmdGroup, debugstr_guid(pguidCmdGroup), cCmds, prgCmds, pCmdText);

    if (!prgCmds)
        return E_INVALIDARG;

    for (UINT i = 0; i < cCmds; i++)
    {
        FIXME("\tprgCmds[%d].cmdID = %d\n", i, prgCmds[i].cmdID);
        prgCmds[i].cmdf = 0;
    }

    return OLECMDERR_E_UNKNOWNGROUP;
}

/**********************************************************
 * ISVOleCmdTarget_Exec (IOleCommandTarget)
 *
 * nCmdID is the OLECMDID_* enumeration
 */
HRESULT WINAPI CDefView::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(\n\tTarget GUID:%s Command:0x%08x Opt:0x%08x %p %p)\n",
          this, debugstr_guid(pguidCmdGroup), nCmdID, nCmdexecopt, pvaIn, pvaOut);

    if (!pguidCmdGroup)
        return OLECMDERR_E_UNKNOWNGROUP;

    if (IsEqualIID(*pguidCmdGroup, CGID_Explorer) &&
            (nCmdID == 0x29) &&
            (nCmdexecopt == 4) && pvaOut)
        return S_OK;

    if (IsEqualIID(*pguidCmdGroup, CGID_ShellDocView) &&
            (nCmdID == 9) &&
            (nCmdexecopt == 0))
        return 1;

    return OLECMDERR_E_UNKNOWNGROUP;
}

/**********************************************************
 * ISVDropTarget implementation
 */

/******************************************************************************
 * drag_notify_subitem [Internal]
 *
 * Figure out the shellfolder object, which is currently under the mouse cursor
 * and notify it via the IDropTarget interface.
 */

#define SCROLLAREAWIDTH 20

HRESULT CDefView::drag_notify_subitem(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    LVHITTESTINFO htinfo;
    LVITEMW lvItem;
    LONG lResult;
    HRESULT hr;
    RECT clientRect;

    /* Map from global to client coordinates and query the index of the listview-item, which is
     * currently under the mouse cursor. */
    htinfo.pt.x = pt.x;
    htinfo.pt.y = pt.y;
    htinfo.flags = LVHT_ONITEM;
    ::ScreenToClient(hWndList, &htinfo.pt);
    lResult = SendMessageW(hWndList, LVM_HITTEST, 0, (LPARAM)&htinfo);

    /* Send WM_*SCROLL messages every 250 ms during drag-scrolling */
    ::GetClientRect(hWndList, &clientRect);
    if (htinfo.pt.x == ptLastMousePos.x && htinfo.pt.y == ptLastMousePos.y &&
            (htinfo.pt.x < SCROLLAREAWIDTH || htinfo.pt.x > clientRect.right - SCROLLAREAWIDTH ||
             htinfo.pt.y < SCROLLAREAWIDTH || htinfo.pt.y > clientRect.bottom - SCROLLAREAWIDTH ))
    {
        cScrollDelay = (cScrollDelay + 1) % 5; /* DragOver is called every 50 ms */
        if (cScrollDelay == 0)
        {
            /* Mouse did hover another 250 ms over the scroll-area */
            if (htinfo.pt.x < SCROLLAREAWIDTH)
                SendMessageW(hWndList, WM_HSCROLL, SB_LINEUP, 0);

            if (htinfo.pt.x > clientRect.right - SCROLLAREAWIDTH)
                SendMessageW(hWndList, WM_HSCROLL, SB_LINEDOWN, 0);

            if (htinfo.pt.y < SCROLLAREAWIDTH)
                SendMessageW(hWndList, WM_VSCROLL, SB_LINEUP, 0);

            if (htinfo.pt.y > clientRect.bottom - SCROLLAREAWIDTH)
                SendMessageW(hWndList, WM_VSCROLL, SB_LINEDOWN, 0);
        }
    }
    else
    {
        cScrollDelay = 0; /* Reset, if the cursor is not over the listview's scroll-area */
    }

    ptLastMousePos = htinfo.pt;

    /* If we are still over the previous sub-item, notify it via DragOver and return. */
    if (pCurDropTarget && lResult == iDragOverItem)
        return pCurDropTarget->DragOver(grfKeyState, pt, pdwEffect);

    /* We've left the previous sub-item, notify it via DragLeave and Release it. */
    if (pCurDropTarget)
    {
        pCurDropTarget->DragLeave();
        pCurDropTarget.Release();
    }

    iDragOverItem = lResult;
    if (lResult == -1)
    {
        /* We are not above one of the listview's subitems. Bind to the parent folder's
         * DropTarget interface. */
        hr = pSFParent->QueryInterface(IID_IDropTarget,
                                       (LPVOID*)&pCurDropTarget);
    }
    else
    {
        /* Query the relative PIDL of the shellfolder object represented by the currently
         * dragged over listview-item ... */
        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = lResult;
        lvItem.iSubItem = 0;
        SendMessageW(hWndList, LVM_GETITEMW, 0, (LPARAM) &lvItem);

        /* ... and bind pCurDropTarget to the IDropTarget interface of an UIObject of this object */
        hr = pSFParent->GetUIObjectOf(hWndList, 1,
                                      (LPCITEMIDLIST*)&lvItem.lParam, IID_IDropTarget, NULL, (LPVOID*)&pCurDropTarget);
    }

    /* If anything failed, pCurDropTarget should be NULL now, which ought to be a save state. */
    if (FAILED(hr))
        return hr;

    /* Notify the item just entered via DragEnter. */
    return pCurDropTarget->DragEnter(pCurDataObject, grfKeyState, pt, pdwEffect);
}

HRESULT WINAPI CDefView::DragEnter(IDataObject *pDataObject, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    /* Get a hold on the data object for later calls to DragEnter on the sub-folders */
    pCurDataObject = pDataObject;
    pDataObject->AddRef();

    return drag_notify_subitem(grfKeyState, pt, pdwEffect);
}

HRESULT WINAPI CDefView::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    return drag_notify_subitem(grfKeyState, pt, pdwEffect);
}

HRESULT WINAPI CDefView::DragLeave()
{
    if (pCurDropTarget)
    {
        pCurDropTarget->DragLeave();
        pCurDropTarget.Release();
    }

    if (pCurDataObject != NULL)
    {
        pCurDataObject.Release();
    }

    iDragOverItem = 0;

    return S_OK;
}

HRESULT WINAPI CDefView::Drop(IDataObject* pDataObject, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    if (pCurDropTarget)
    {
        pCurDropTarget->Drop(pDataObject, grfKeyState, pt, pdwEffect);
        pCurDropTarget.Release();
    }

    pCurDataObject.Release();
    iDragOverItem = 0;

    return S_OK;
}

/**********************************************************
 * ISVDropSource implementation
 */

HRESULT WINAPI CDefView::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
    TRACE("(%p)\n", this);

    if (fEscapePressed)
        return DRAGDROP_S_CANCEL;
    else if (!(grfKeyState & MK_LBUTTON) && !(grfKeyState & MK_RBUTTON))
        return DRAGDROP_S_DROP;
    else
        return NOERROR;
}

HRESULT WINAPI CDefView::GiveFeedback(DWORD dwEffect)
{
    TRACE("(%p)\n", this);

    return DRAGDROP_S_USEDEFAULTCURSORS;
}

/**********************************************************
 * ISVViewObject implementation
 */

HRESULT WINAPI CDefView::Draw(DWORD dwDrawAspect, LONG lindex, void *pvAspect, DVTARGETDEVICE *ptd, HDC hdcTargetDev, HDC hdcDraw, LPCRECTL lprcBounds, LPCRECTL lprcWBounds, BOOL (CALLBACK *pfnContinue)(ULONG_PTR dwContinue), ULONG_PTR dwContinue)
{
    FIXME("Stub: this=%p\n", this);

    return E_NOTIMPL;
}

HRESULT WINAPI CDefView::GetColorSet(DWORD dwDrawAspect, LONG lindex, void *pvAspect, DVTARGETDEVICE *ptd, HDC hicTargetDevice, LOGPALETTE **ppColorSet)
{
    FIXME("Stub: this=%p\n", this);

    return E_NOTIMPL;
}

HRESULT WINAPI CDefView::Freeze(DWORD dwDrawAspect, LONG lindex, void *pvAspect, DWORD *pdwFreeze)
{
    FIXME("Stub: this=%p\n", this);

    return E_NOTIMPL;
}

HRESULT WINAPI CDefView::Unfreeze(DWORD dwFreeze)
{
    FIXME("Stub: this=%p\n", this);

    return E_NOTIMPL;
}

HRESULT WINAPI CDefView::SetAdvise(DWORD aspects, DWORD advf, IAdviseSink *pAdvSink)
{
    FIXME("partial stub: %p %08x %08x %p\n", this, aspects, advf, pAdvSink);

    /* FIXME: we set the AdviseSink, but never use it to send any advice */
    pAdvSink = pAdvSink;
    dwAspects = aspects;
    dwAdvf = advf;

    return S_OK;
}

HRESULT WINAPI CDefView::GetAdvise(DWORD *pAspects, DWORD *pAdvf, IAdviseSink **ppAdvSink)
{
    TRACE("this=%p pAspects=%p pAdvf=%p ppAdvSink=%p\n", this, pAspects, pAdvf, ppAdvSink);

    if (ppAdvSink)
    {
        *ppAdvSink = pAdvSink;
        pAdvSink.p->AddRef();
    }

    if (pAspects)
        *pAspects = dwAspects;

    if (pAdvf)
        *pAdvf = dwAdvf;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CDefView::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    if (IsEqualIID(guidService, SID_IShellBrowser))
        return pShellBrowser->QueryInterface(riid, ppvObject);
    else if(IsEqualIID(guidService, SID_IFolderView))
        return QueryInterface(riid, ppvObject);

    return E_NOINTERFACE;
}

/**********************************************************
 *    IShellView_Constructor
 */
HRESULT WINAPI IShellView_Constructor(IShellFolder *pFolder, IShellView **newView)
{
    CComObject<CDefView>                    *theView;
    CComPtr<IShellView>                        result;
    HRESULT                                    hResult;

    if (newView == NULL)
        return E_POINTER;

    *newView = NULL;
    ATLTRY (theView = new CComObject<CDefView>);

    if (theView == NULL)
        return E_OUTOFMEMORY;

    hResult = theView->QueryInterface (IID_IShellView, (void **)&result);
    if (FAILED (hResult))
    {
        delete theView;
        return hResult;
    }

    hResult = theView->Initialize (pFolder);
    if (FAILED (hResult))
        return hResult;
    *newView = result.Detach ();

    return S_OK;
}
