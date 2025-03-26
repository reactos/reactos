#pragma once

#include "rapps.h"
#include "rosui.h"
#include "crichedit.h"
#include "asyncinet.h"

#include <shlobj_undoc.h>
#include <shlguid_undoc.h>

#include <atlbase.h>
#include <atlcom.h>
#include <atltypes.h>
#include <atlwin.h>
#include <wininet.h>
#include <shellutils.h>
#include <ui/rosctrls.h>
#include <gdiplus.h>
#include <math.h>

extern HICON g_hDefaultPackageIcon;

// default broken-image icon size
#define BROKENIMG_ICON_SIZE 96

// the boundary of w/h ratio of scrnshot preview window
#define SCRNSHOT_MAX_ASPECT_RAT 2.5

// padding between scrnshot preview and richedit (in pixel)
#define INFO_DISPLAY_PADDING 10

// minimum width of richedit
#define RICHEDIT_MIN_WIDTH 160

// padding between controls in toolbar
#define TOOLBAR_PADDING 6

// user-defined window message
#define WM_RAPPS_DOWNLOAD_COMPLETE                                                                                     \
    (WM_USER + 1) // notify download complete. wParam is error code, and lParam is a pointer to ScrnshotDownloadParam
#define WM_RAPPS_RESIZE_CHILDREN (WM_USER + 2) // ask parent window to resize children.
#define WM_RAPPSLIST_ASYNCICON (WM_APP + 0)

enum SCRNSHOT_STATUS
{
    SCRNSHOT_PREV_EMPTY,   // show nothing
    SCRNSHOT_PREV_LOADING, // image is loading (most likely downloading)
    SCRNSHOT_PREV_IMAGE,   // display image from a file
    SCRNSHOT_PREV_FAILED   // image can not be shown (download failure or wrong image)
};

#define TIMER_LOADING_ANIMATION 1 // Timer ID

#define LOADING_ANIMATION_PERIOD 3 // Animation cycling period (in seconds)
#define LOADING_ANIMATION_FPS 18   // Animation Frame Per Second

#define PI 3.1415927

// retrieve the value using a mask
#define STATEIMAGETOINDEX(x) (((x)&LVIS_STATEIMAGEMASK) >> 12)

// for listview with extend style LVS_EX_CHECKBOXES, State image 1 is the unchecked box, and state image 2 is the
// checked box. see this: https://docs.microsoft.com/en-us/windows/win32/controls/extended-list-view-styles
#define STATEIMAGE_UNCHECKED 1
#define STATEIMAGE_CHECKED 2

class CMainWindow;

enum APPLICATION_VIEW_TYPE
{
    AppViewTypeAvailableApps,
    AppViewTypeInstalledApps
};

typedef struct __ScrnshotDownloadParam
{
    LONGLONG ID;
    HANDLE hFile;
    HWND hwndNotify;
    CStringW DownloadFileName;
} ScrnshotDownloadParam;

class CAppRichEdit : public CUiWindow<CRichEdit>
{
  public:
    VOID
    LoadAndInsertText(UINT uStringID, const CStringW &szText, DWORD TextFlags);
    VOID
    LoadAndInsertText(UINT uStringID, DWORD StringFlags);
    VOID
    InsertTextWithString(UINT StringID, const CStringW &Text, DWORD TextFlags);
};

int
ScrnshotDownloadCallback(pASYNCINET AsyncInet, ASYNC_EVENT Event, WPARAM wParam, LPARAM lParam, VOID *Extension);

class CAppScrnshotPreview : public CWindowImpl<CAppScrnshotPreview>
{
  private:
    CStringW m_BasePath;
    SCRNSHOT_STATUS ScrnshotPrevStauts = SCRNSHOT_PREV_EMPTY;
    Gdiplus::Image *pImage = NULL;
    HICON hBrokenImgIcon = NULL;
    BOOL bLoadingTimerOn = FALSE;
    int LoadingAnimationFrame = 0;
    int BrokenImgSize = BROKENIMG_ICON_SIZE;
    pASYNCINET AsyncInet = NULL;
    LONGLONG ContentID = 0; // used to determine whether image has been switched when download complete. Increase by 1
                            // each time the content of this window changed
    CStringW TempImagePath; // currently displayed temp file

    BOOL
    ProcessWindowMessage(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT &theResult, DWORD dwMapId);

    VOID
    DisplayLoading();
    VOID
    DisplayFailed();
    BOOL
    DisplayFile(LPCWSTR lpszFileName);
    VOID
    SetStatus(SCRNSHOT_STATUS Status);

    VOID
    PaintOnDC(HDC hdc, int width, int height, BOOL bDrawBkgnd);
    float
    GetLoadingDotWidth(int width, int height);
    float
    GetFrameDotShift(int Frame, int width, int height);

  public:
    static ATL::CWndClassInfo &
    GetWndClassInfo();

    HWND
    Create(HWND hParent);
    VOID
    PreviousDisplayCleanup();
    VOID
    DisplayEmpty();
    BOOL
    DisplayImage(LPCWSTR lpszLocation);

    // calculate requested window width by given height
    int
    GetRequestedWidth(int Height);

    CAppScrnshotPreview(const CStringW &BasePath);
    ~CAppScrnshotPreview();
};

class CAppInfoDisplay : public CUiWindow<CWindowImpl<CAppInfoDisplay>>
{
    LPWSTR pLink = NULL;

  private:
    BOOL
    ProcessWindowMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT &theResult, DWORD dwMapId);
    VOID
    OnLink(ENLINK *Link);

    VOID
    ResizeChildren();
    VOID
    ResizeChildren(int Width, int Height);

  public:
    CAppRichEdit *RichEdit = NULL;
    CAppScrnshotPreview *ScrnshotPrev = NULL;

    static ATL::CWndClassInfo &
    GetWndClassInfo();

    HWND
    Create(HWND hwndParent);

    VOID
    ShowAppInfo(CAppInfo &Info, bool OnlyUpdateText = false);
    void
    SetWelcomeText(bool bAppwiz);
    VOID
    OnCommand(WPARAM wParam, LPARAM lParam);

    ~CAppInfoDisplay();
};

class CAppsListView : public CUiWindow<CWindowImpl<CAppsListView, CListView>>
{
    struct SortContext
    {
        CAppsListView *lvw;
        INT iSubItem;
    };

    BOOL bIsAscending = TRUE;
    bool bHasCheckboxes = false;

    INT ItemCount = 0;
    INT CheckedItemCount = 0;
    INT ColumnCount = 0;

    INT nLastHeaderID = -1;

    APPLICATION_VIEW_TYPE ApplicationViewType = AppViewTypeAvailableApps;

    HIMAGELIST m_hImageListView = NULL;
    CStringW m_Watermark;

    BEGIN_MSG_MAP(CAppsListView)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
    MESSAGE_HANDLER(WM_RAPPSLIST_ASYNCICON, OnAsyncIcon)
    END_MSG_MAP()

    LRESULT
    OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT
    OnAsyncIcon(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

  public:
    CAppsListView();
    ~CAppsListView();

    VOID
    SetWatermark(const CStringW &Text);

    void
    ShowCheckboxes(bool bShow);

    VOID
    ColumnClick(LPNMLISTVIEW pnmv);

    BOOL
    AddColumn(INT Index, CStringW &Text, INT Width, INT Format);
    void
    DeleteColumn(INT Index);

    INT
    AddItem(INT ItemIndex, INT IconIndex, LPCWSTR lpText, LPARAM lParam);

    HIMAGELIST
    GetImageList(int iImageList);

    static INT CALLBACK
    s_CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

    INT
    CompareFunc(LPARAM lParam1, LPARAM lParam2, INT iSubItem);

    HWND
    Create(HWND hwndParent);

    BOOL
    GetCheckState(INT item);
    VOID
    SetCheckState(INT item, BOOL fCheck);
    VOID
    CheckAll();

    PVOID
    GetFocusedItemData();

    BOOL
    SetDisplayAppType(APPLICATION_VIEW_TYPE AppType);
    BOOL
    SetViewMode(DWORD ViewMode);

    BOOL
    AddApplication(CAppInfo *AppInfo, BOOL InitialCheckState);

    // this function is called when parent window receiving an notification about checkstate changing
    VOID
    ItemCheckStateNotify(int iItem, BOOL bCheck);
};

class CMainToolbar : public CUiWindow<CToolbar<>>
{
    const INT m_iToolbarHeight;
    DWORD m_dButtonsWidthMax;

    VOID
    AddImageToImageList(HIMAGELIST hImageList, UINT ImageIndex);

    HIMAGELIST
    InitImageList();

  public:
    CMainToolbar();

    VOID
    OnGetDispInfo(LPTOOLTIPTEXT lpttt);

    HWND
    Create(HWND hwndParent);

    void
    ShowButtonCaption(bool bShow);

    void
    UpdateMaxButtonsWidth();

    DWORD
    GetMaxButtonsWidth() const;
};

class CSearchBar : public CWindow
{
  public:
    const INT m_Width;
    const INT m_Height;

    CSearchBar();

    VOID
    SetText(LPCWSTR lpszText);

    HWND
    Create(HWND hwndParent);
};

class CComboBox : public CWindow
{
    // ID refers to different types of view
    enum
    {
        m_AppDisplayTypeDetails,
        m_AppDisplayTypeList,
        m_AppDisplayTypeTile
    };

    // string ID for different. this should correspond with the enum above.
    const UINT m_TypeStringID[3] = {IDS_APP_DISPLAY_DETAILS, IDS_APP_DISPLAY_LIST, IDS_APP_DISPLAY_TILE};

    const int m_DefaultSelectType = m_AppDisplayTypeDetails;

  public:
    int m_Width;
    int m_Height;

    CComboBox();

    HWND
    Create(HWND hwndParent);
};

class CApplicationView : public CUiWindow<CWindowImpl<CApplicationView>>
{
  private:
    CUiPanel *m_Panel = NULL;
    CMainToolbar *m_Toolbar = NULL;
    CUiWindow<CComboBox> *m_ComboBox = NULL;
    CUiWindow<CSearchBar> *m_SearchBar = NULL;
    CAppsListView *m_ListView = NULL;
    CAppInfoDisplay *m_AppsInfo = NULL;
    CUiSplitPanel *m_HSplitter = NULL;
    CMainWindow *m_MainWindow = NULL;
    APPLICATION_VIEW_TYPE ApplicationViewType = AppViewTypeAvailableApps;

    BOOL
    ProcessWindowMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT &theResult, DWORD dwMapId);

    BOOL
    CreateToolbar();
    BOOL
    CreateSearchBar();
    BOOL
    CreateComboBox();
    BOOL
    CreateHSplitter();
    BOOL
    CreateListView();
    BOOL
    CreateAppInfoDisplay();

    VOID
    OnSize(HWND hwnd, WPARAM wParam, LPARAM lParam);
    VOID
    OnCommand(WPARAM wParam, LPARAM lParam);

  public:
    CApplicationView(CMainWindow *MainWindow);
    ~CApplicationView();

    static ATL::CWndClassInfo &
    GetWndClassInfo();

    HWND
    Create(HWND hwndParent);
    void
    SetRedraw(BOOL bRedraw);
    void
    RefreshAvailableItem(PCWSTR PackageName);
    void
    SetFocusOnSearchBar();
    BOOL
    SetDisplayAppType(APPLICATION_VIEW_TYPE AppType);

    BOOL
    AddApplication(CAppInfo *InstAppInfo, BOOL InitialCheckState);
    VOID
    SetWatermark(const CStringW &Text);

    void
    CheckAll();
    PVOID
    GetFocusedItemData();
    int
    GetItemCount();
    VOID
    AppendTabOrderWindow(int Direction, ATL::CSimpleArray<HWND> &TabOrderList);

    struct RESTORELISTSELECTION {
        LVITEMW Item;
        WCHAR Name[MAX_PATH];
    };
    VOID
    GetRestoreListSelectionData(RESTORELISTSELECTION &Restore);
    VOID
    RestoreListSelection(const RESTORELISTSELECTION &Restore);

    VOID
    RefreshDetailsPane(CAppInfo &Info, bool OnlyUpdateText = false);

    // this function is called when a item of listview get focus.
    // CallbackParam is the param passed to listview when adding the item (the one getting focus now).
    VOID
    ItemGetFocus(LPVOID CallbackParam);

    // this function is called when a item of listview is checked/unchecked
    // CallbackParam is the param passed to listview when adding the item (the one getting focus now).
    VOID
    ItemCheckStateChanged(BOOL bChecked, LPVOID CallbackParam);
};
