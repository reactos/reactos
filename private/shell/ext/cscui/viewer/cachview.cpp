//////////////////////////////////////////////////////////////////////////////
/*  File: cachview.cpp

    Description: Client Side Caching cache viewer.
        This module implements the UI portions of the viewer.  This includes
        the following classes (indentation denotes inheritance):

            CacheWindow
            CacheView
                StaleView
                DetailsView
                ShareView
            ViewState

        The client instantiates a single CacheWindow object then calls 
        the Run() member.  A CacheWindow object maintains a single view
        object which is specialized to present a specific view onto the
        CSC database information.  All view-neutral functions are 
        addressed through the common base class CacheView while any
        specializations are handled virtually through StaleView, 
        DetailsView and ShareView.
        The class ViewState is used to encapsulate the state information
        associated with a view object.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#pragma hdrstop

#include <commctrl.h>
#include <commdlg.h>
#include <process.h>
#include <limits.h>
#include <htmlhelp.h>
#include <cscuiext.h>
#include "msgbox.h"
#include "strret.h"
//
// autoptr's for scalar types can generate warning C4284.
// Since overloading operator ->() for scalar type is
// meaningless, this warning is meaningless in this context.
//
#pragma warning (disable : 4284)

#include "autoptr.h"
#include "bitset.h"
#include "cscutils.h"
#include "cachview.h"
#include <resource.h>
#include "registry.h"
#include "ccinline.h" 
#include "filelist.h"
#include "filetime.h"
#include "update.h"
#include "dbgdlgs.h"
#include "progdlg.h"
#include "copydlgs.h"
#include "openfile.h"
#include "util.h"     // utils from "dll" directory.

//
// This structure is used to pass the CacheWindow object's "this" pointer
// in WM_CREATE.
//
typedef struct WndCreationData { 
    SHORT   cbExtra; 
    LPVOID  pThis;               // CacheWindow instance's "this" ptr.
    CacheWindow::ViewType eView; // Initial view to show (CacheWindow::ViewType).
    CString strShare;            // Initial share to show ("" = all).
} WNDCREATE_DATA; 

typedef UNALIGNED WNDCREATE_DATA *PWNDCREATE_DATA;

//
// Structure passed to CacheView::SortCompareFunc callback.
//
typedef struct comparestruct
{
    bool bSortAscending;         // Sort order.
    LVColumn *pColumn;           // Column being sorted.
} COMPARESTRUCT, *PCOMPARESTRUCT;
    
//
// Dimensions for the share list combo box in the toolbar.
// The CY parameter limits the total vertical dimension when
// the combo list is open.
//
const int CacheWindow::CX_SHARE_COMBO = 200;
const int CacheWindow::CY_SHARE_COMBO = 200;
//
// Maximum number of share names allowed in the "Shares" menu.
// If the number of actual shares exceeds this number, a 
// "Shares..." item is appended to the end of the menu.  Selecting
// this item invokes a selection dialog containing all shares.
// The user then selects a share from this dialog instead
// of the menu.
//
const int CacheWindow::MAX_SHARE_MENU_ITEMS = 10;
//
// Controls how often the progress bar is updated while loading the
// Csc object tree.  Value is in milliseconds.
//
const int CacheWindow::PROGRESSBAR_UPDATE_MS = 300;
//
// ID for the timer that controls the addition of items to the
// listview.  There should be no reason to change this.
//
const UINT CacheWindow::ID_TIMER_GETNEXTCSCOBJECT = 1;
//
// ID for timer that checks for toolbar button mouse hits.
// Used to detect when mouse has left the toolbar.  Since
// it's received by the subclassed toobar wnd proc, give
// it an unusual ID so it doesn't clash with some other timer
// possibly used internal to the toolbar.
//
const UINT CacheWindow::TBSubclass::ID_TIMER_TBHITTEST = 4321;
//
// Version recorded in window and view state information.  If 
// the format of state information written to the registry 
// changes, these versions should be incremented.  
//
const DWORD CacheWindow::WINDOW_STATE_VERSION = 3;
const short ViewState::VIEW_STATE_VERSION     = 2;
//
// These two parameters control the performance of the CacheWindow's
// display information cache.  The window maintains a cache of 
// display information (view text strings) for most recently
// displayed listview items.  This helps minimize the requests to
// the object tree for display information.  The CACHE_SIZE
// parameter controls the maximum number of entries in the cache.
// When this number is exceded, the least-recently-requested
// item is removed from the cache.  The CACHE_BUCKETS parameter
// is the number of buckets in the cache's hash table.  For 
// best performance, this number should be prime.
//
static const int DISP_INFO_CACHE_SIZE    = 100;
static const int DISP_INFO_CACHE_BUCKETS = 23;
//
// Custom message sent to the Cache View Window during idle periods.
// Upon receipt, the window loads another CSC object into the listview.
//
#define CVM_GETNEXTCSCOBJECT  (WM_USER + 1)
//
// Custom message sent to the Cache View Window whenever the icon
// finding thread (Icon Hound) finds an icon for a listview item.
// Upon receiving this message, the window updates the corresponding
// item in the view.
//
#define CVM_UPDATEOBJECT      (WM_USER + 2)
//
// Custom message sent to the cache view window whenever the number
// of items in the listview has changed.  
//
#define CVM_LVITEMCNTCHANGED  (WM_USER + 3)
//
// Custom message used by test apps to query status of viewer
// Status is returned in SendMessage return value.  Data is
// encoded as follows:
//
//  Bit    Description
//  ------ -------------------------------------------------------------------
//  00-02  Current view type.  0 = Unknown, 1 = Share, 2 = Details, 3 = Stale
//     03  View populated
//     04  Toolbar visible
//     05  Statusbar visible
//
#define CVM_TEST_GETVIEWERSTATUS   (WM_USER + 4)
//
// "Viewer Status" flags and values to help with manipulating the 
// return value for CVM_TEST_GETVIEWERSTATUS.
//
#define VSF_TYPEMASK    0x00000007
#define VSF_POPULATED   0x00000008
#define VSF_TOOLBAR     0x00000010
#define VSF_STATUSBAR   0x00000020

//
// Custom message used by test apps to query status of listview items.
// Item index is passed in wParam.
// Status is returned in SendMessage return value.  Data is
// encoded as follows:
//
//  Bit    Description
//  ------ -------------------------------------------------------------------
//  00-03  Item icon ID
//            0 = Generic document icon
//            1 = Share icon
//            2 = Share icon (disconnected)
//            3 = Generic folder icon
//            4 = Stale item icon
//            5 = Pinned icon
//            6 = Unpinned icon
//         7-15 = Item's native icon (may be truncated to 4 bits).
//     04  Item selection status. 0 = Not selected, 1 = selected.
//     05  Item has "pinned" icon (Details and Stale view only).
//  05-31  Unused.
//
#define CVM_TEST_GETITEMSTATUS     (WM_USER + 5)
//
// "Item Status" flags and values to help with manipulating the
// return value for CVM_TEST_GETITEMSTATUS
//
#define ISF_ICONIDMASK  0x0000000F
#define ISF_SELECTED    0x00000010
#define ISF_PINNED      0x00000020


enum ToolbarImageIndices { iIMAGE_BTN_OPEN = 0,
                           iIMAGE_BTN_SAVEAS,
                           iIMAGE_BTN_DELETE,
                           iIMAGE_BTN_PIN,
                           iIMAGE_BTN_UNPIN,
                           iIMAGE_BTN_UPDATE,
                           iIMAGE_BTN_SHARES,
                           iIMAGE_BTN_STALE,
                           iIMAGE_BTN_DETAILS,
                           cBUTTON_IMAGES };

enum MainMenuItemIndices { iMENU_ITEM_FILE = 0,
                           iMENU_ITEM_EDIT,
                           iMENU_ITEM_VIEW,
                           iMENU_ITEM_SHARES,
                           iMENU_ITEM_HELP,
                           cMENU_ITEMS };


//-----------------------------------------------------------------------------
// class CMenuItemPos
//-----------------------------------------------------------------------------
//
// This class maintains a level of indirection for main menu item
// positions.  This is required since many of the menu commands require
// a menu position and since we're removing and inserting the "Shares"
// menu item.  When a menu item is removed or added, 
// the Remove() and Restore() methods adjust the indirection array.
// The Position() method returns the true index of an item in the main menu.
// The iMENU_ITEM_XXXX constants are used as arguments to these three methods.
// -1 indicates an "unused" item in the indirection array.
//
class CMenuItemPos
{
    public:
        enum { UNUSED = -1 };

        CMenuItemPos(int cItems);

        void Restore(int iItem);
        void Remove(int iItem);
        int Position(int iItem) const
            { return m_rgItems[iItem]; }

    private:
        CArray<int> m_rgItems;

} g_MenuItemPos(cMENU_ITEMS);


//
// Fill the indirection array so that each element contains it's index
// number.  This indicates that all menu items are present.
//
CMenuItemPos::CMenuItemPos(
    int cItems
    ) : m_rgItems(cItems)
{
    for (int i = 0; i < m_rgItems.Count(); i++)
        m_rgItems[i] = i;
};


//
// Restore a menu item that was previously removed.
//
void CMenuItemPos::Restore(int iItem)
{
    if (UNUSED == m_rgItems[iItem])
    {
        int i;
        int iLastUsed = m_rgItems[0];
        //
        // Find what actual position we should use.
        //
        for (i = 0; i < iItem; i++)
        {
            if (m_rgItems[i] != UNUSED)
                iLastUsed = m_rgItems[i];
        }
        m_rgItems[iItem] = iLastUsed + 1;
        //
        // Adjust all succeeding items.
        //
        for (i = iItem + 1; i < m_rgItems.Count(); i++)
        {
            if (UNUSED != m_rgItems[i])
                m_rgItems[i] = m_rgItems[i] + 1;
        }
    }
}

//
// Remove a menu item from the indirection array.
//
void CMenuItemPos::Remove(int iItem)
{
    if (UNUSED != m_rgItems[iItem])
    {
        m_rgItems[iItem] = UNUSED;
        for (int i = iItem + 1; i < m_rgItems.Count(); i++)
        {
            if (UNUSED != m_rgItems[i])
                m_rgItems[i] = m_rgItems[i] - 1;
        }
    }
}



//-----------------------------------------------------------------------------
// class CacheWindow
//-----------------------------------------------------------------------------

CacheWindow::CacheWindow(
    HINSTANCE hInstance
    ) : m_hInstance(hInstance),
        m_pCscObjTree(new CscObjTree(DISP_INFO_CACHE_SIZE, DISP_INFO_CACHE_BUCKETS)),
        m_pView(NULL),
        m_TBSubclass(this),
        m_hwndMain(NULL),
        m_hwndStatusbar(NULL),
        m_hwndToolbar(NULL),
        m_hwndProgressbar(NULL),
        m_hwndCombo(NULL),
        m_hwndComboLabel(NULL),
        m_hmenuMain(NULL),
        m_hmenuShares(NULL),
        m_hmodHtmlHelp(NULL),
        m_hKbdAccel(NULL),
        m_strRegKey(REGSTR_KEY_VIEWSTATEINFO),
        m_eCurrentView(CacheWindow::eShareView),
        m_iLastColSorted(-1),
        m_bViewPopulated(true),
        m_bSortAscending(true),
        m_bMenuActive(false),
        m_bToolbarVisible(false),
        m_bStatusbarVisible(false),
        m_bStatusbarDisabled(false)
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("CacheWindow::CacheWindow")));
    DBGASSERT((NULL != m_hInstance));

    //
    // If the "ShowDebugInfo" reg value is 1, we'll add a "DebugInfo"
    // item to the context menus.  This menu item will display a dialog
    // containing detailed information about a selected CSC item.
    // This is intended for development and debugging only.  If the
    // value is missing or set as 0, or the user selects multiple
    // objects in the listview, the menu option is not added to the
    // context menu.
    //
    // If the "ShowSparseFiles" reg value is 1, we'll include sparse
    // files in the view.  This is a debugging option only.  We do not want
    // to display sparse files to users.  If they try to save sparse files
    // to another location they will not get a complete file if the share
    // is disconnected.
    //
    RegKey key(HKEY_CURRENT_USER, m_strRegKey);
    if (SUCCEEDED(key.Open(KEY_READ, false)))
    {
        DWORD dwRegValue = 0;
        if (SUCCEEDED(key.GetValue(CString(REGSTR_VAL_SHOWDEBUGINFO), &dwRegValue)))
        {
            Viewer::m_bShowDebugInfo = boolify(dwRegValue);
        }
//
// We have decided to ALWAYS show sparse files.  Leave this code in case
// we change our minds again. [brianau - 2/23/98]
//
//        if (key.GetValue(CString(REGSTR_VAL_SHOWSPARSEFILES), &dwRegValue))
//        {
//            Viewer::m_bShowSparseFiles = boolify(dwRegValue);
//        }
    }
    Viewer::m_bShowSparseFiles = true;
    m_pCscObjTree->AddRef();
}


CacheWindow::~CacheWindow(
    void
    )
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("CacheWindow::~CacheWindow")));

    //
    // Note that the view's icon hound and listview are destroyed in
    // response to WM_DESTROY.
    //
    delete m_pView;
    m_pView = NULL;
    m_pCscObjTree->DestroyChildren();
    m_pCscObjTree->Release();

    if (NULL != m_hmodHtmlHelp)
        FreeLibrary(m_hmodHtmlHelp);

}


//
// THE main entry point to activate the cache viewer.  Client first instantiates
// a CacheWindow object then calls Run().  
//
bool
CacheWindow::Run(
    ViewType eInitialView,
    const CString& strInitialShare // Ignored if View == CacheWindow::ViewShare.
    )
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("CacheWindow::Run")));

    bool bResult = false;
    CString strShare(strInitialShare);

    //
    // Ensure we have a valid view type.  Default to "share" view.
    //
    if (!IsValidViewType(eInitialView))
    {
        DBGERROR((TEXT("Invalid view type specified (%d).  Defaulting to \"share\" view."),
                 (int)eInitialView));

        eInitialView = eShareView;
    }

    //
    // We haven't loaded the CSC object tree yet so we need to go directly
    // to CSC to see if the specified share exists.  Note that "existance" to
    // the viewer also means the share has one or more files cached.
    //
    CscShareInformation si;
    if (!CscGetShareInformation(strShare, &si) || 0 == si.TotalCount())
    {
        //
        // If caller provided a share that doesn't exist in the CSC object
        // tree, default to "All Shares".
        //
        DBGERROR((TEXT("Invalid share specified \"%s\".  Defaulting to \"All Shares\""),
                 strShare.Cstr()));
        strShare.Empty();
    }

    m_hKbdAccel = LoadAccelerators(m_hInstance, MAKEINTRESOURCE(IDR_CACHEVIEW_ACCEL));
    if (NULL == m_hKbdAccel)
        throw CException(GetLastError());

    if (CreateMainWindow(eInitialView, strShare))
    {
        DBGASSERT((NULL != m_hwndMain));
        MSG msg;

        while (0 != GetMessage(&msg, NULL, 0, 0))
        {
            if (NULL == m_hKbdAccel || 
               !TranslateAccelerator(m_hwndMain, m_hKbdAccel, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        bResult = true;
    }

    return bResult;
}


//
// Create the main viewer window.  All of the window's children are created
// in response to WM_CREATE - See OnCreate().
//
bool
CacheWindow::CreateMainWindow(
    ViewType eInitialView,
    const CString& strInitialShare
    )
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("CacheWindow::CreateMainWindow")));
    DBGASSERT((IsValidViewType(eInitialView)));
    DBGASSERT((NULL != m_hInstance));

    bool bResult = false;
    try
    {
        WNDCLASSEX wc;

        wc.cbSize           = sizeof(WNDCLASSEX);
        wc.style            = CS_PARENTDC;
        wc.lpfnWndProc      = WndProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = m_hInstance;
        wc.hIcon            = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_CSCUI_ICON));
        wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground    = NULL;
        wc.lpszMenuName     = MAKEINTRESOURCE(IDR_CACHEVIEW_MENU);
        wc.lpszClassName    = WC_NETCACHE_VIEWER;
        wc.hIconSm          = NULL;

        RegisterClassEx(&wc);

        //
        // Need to pass "this" pointer in WM_CREATE.  We'll store "this"
        // in the window's USERDATA.  Also pass an initial view type and 
        // share name.
        //
        WNDCREATE_DATA wcd;
        wcd.cbExtra  = sizeof(WNDCREATE_DATA);
        wcd.pThis    = this;
        wcd.eView    = eInitialView;
        wcd.strShare = strInitialShare;

        //
        // Seed the "current view" and "current share" members with invalid
        // values so that ChangeView will update properly with the initial values.
        //
        m_eCurrentView    = eUnknownView;
        m_strCurrentShare = TEXT("?");

        //
        // Initial window title displayed while view is loading.  Each view
        // will modify the window title to reflect it's content.  This initial
        // value is so we don't have a blank title bar while the initial view
        // is loading.  Looks bad.
        //
        CString strTitle(m_hInstance, IDS_FMT_TITLE_CACHEVIEW_WINDOW, TEXT(""));

        HWND hwnd = CreateWindow(WC_NETCACHE_VIEWER,
                                 strTitle,
                                 WS_OVERLAPPEDWINDOW,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 GetDesktopWindow(),
                                 NULL,
                                 m_hInstance,
                                 &wcd);
        if (NULL != hwnd)
        {
            int nCmdShow;
            RestoreWindowState(&nCmdShow);
            ApplyPolicy();
            ShowWindow(hwnd, nCmdShow);
            UpdateWindow(hwnd);
            Refresh();
            bResult = true;
        }

    #ifdef __NEVER__
            //
            // BUGBUG:  For development only!
            //
            // This code throws an "out of memory" exception after window
            // creation is complete.  Used to verify proper cleanup upon
            // an exception during window creation.
            // [brianau - 11/10/97]
            // 
        DBGERROR((TEXT("Allocating BIG array.")));
        array_autoptr<BYTE> ptrBigArray(new BYTE[1000000000L]);
    #endif
    }
    catch(CException& e)
    {
        DBGERROR((TEXT("C++ exception %d caught in CacheWindow::CreateMainWindow"), e.dwError));
        CscWin32Message(NULL, e.dwError, CSCUI::SEV_ERROR);
    }
    catch(...)
    {
        DBGERROR((TEXT("Unknown exception caught in CacheWindow::CreateMainWindow")));
        CscWin32Message(NULL, DISP_E_EXCEPTION, CSCUI::SEV_ERROR);
    }

    return bResult;
}


//
// The WndProc for the cache viewer's main window.
//
LRESULT CALLBACK 
CacheWindow::WndProc(
    HWND hWnd, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam
    ) throw()
{
    //
    // Retrieve the CacheView object's "this" pointer from the window's
    // USERDATA.
    //
    CacheWindow *pThis = (CacheWindow *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    try
    {
        //
        // C++ EH is more efficient with single statement in try block.
        //
        return WndProcInternal(pThis, hWnd, message, wParam, lParam);
    }
    catch(CException& e)
    {
        DBGERROR((TEXT("C++ exception %d caught in CacheWindow::WndProc"), e.dwError));
        CscWin32Message(NULL, e.dwError, CSCUI::SEV_ERROR);
    }
    catch(...)
    {
        DBGERROR((TEXT("Unknown exception caught in CacheWindow::WndProc")));
        CscWin32Message(NULL, DISP_E_EXCEPTION, CSCUI::SEV_ERROR);
    }
}



LRESULT CALLBACK 
CacheWindow::WndProcInternal(
    CacheWindow *pThis,
    HWND hWnd, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    LRESULT lResult = 0;  // Default return value.
    switch(message)
    {
        case WM_CREATE:
        {
            //
            // Retrieve the "this" pointer from the window creation data and
            // store it in the window's USERDATA.
            //
            CREATESTRUCT *pcs = (CREATESTRUCT *)lParam;
            PWNDCREATE_DATA pcd = (PWNDCREATE_DATA)(pcs->lpCreateParams);
            DBGASSERT((NULL != pcd));

            pThis = (CacheWindow *)(pcd->pThis);
            DBGASSERT((NULL != pThis));
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (INT_PTR)pThis);

            pThis->m_hwndMain = hWnd;
            if (!pThis->OnCreate(pcd->eView, pcd->strShare))
                lResult = -1;

            break;
        }

        case WM_COMMAND:
            DBGASSERT((NULL != pThis));
            pThis->OnCommand(hWnd, message, wParam, lParam);
            break;

        case WM_QUERYENDSESSION:
            lResult = pThis->OnQueryEndSession(hWnd, wParam, lParam);
            break;

        case WM_DESTROY:
            DBGASSERT(NULL != pThis);
            pThis->OnDestroy(hWnd, message, wParam, lParam);
            break;

        case WM_SIZE:
            DBGASSERT((NULL != pThis));
            pThis->OnSize(hWnd, message, wParam, lParam);
            break;

        case WM_NOTIFY:
            DBGASSERT((NULL != pThis));
            pThis->OnNotify(hWnd, message, wParam, lParam);
            break;

        case CVM_GETNEXTCSCOBJECT:
            DBGASSERT((NULL != pThis));
            pThis->OnGetNextCscObject();
            break;

        case CVM_UPDATEOBJECT:
            DBGASSERT((NULL != pThis));
            pThis->OnUpdateObject(reinterpret_cast<CscObject *>(lParam));
            break;

        case CVM_LVITEMCNTCHANGED:
            DBGASSERT((NULL != pThis));
            pThis->OnLVItemCountChanged(lParam);
            break;

        case CVM_TEST_GETVIEWERSTATUS:
            DBGASSERT((NULL != pThis));
            lResult = pThis->OnTestGetViewerStatus();
            break;

        case CVM_TEST_GETITEMSTATUS:
            DBGASSERT((NULL != pThis));
            lResult = pThis->OnTestGetItemStatus(wParam);
            break;

        case WM_TIMER:
            DBGASSERT((NULL != pThis));
            if (ID_TIMER_GETNEXTCSCOBJECT == wParam)
                PostMessage(hWnd, CVM_GETNEXTCSCOBJECT, 0, 0);
            break;

        case WM_MENUSELECT:
            DBGASSERT((NULL != pThis));
            pThis->OnMenuSelect(hWnd, message, wParam, lParam);
            break;

        case WM_SETFOCUS:
            DBGASSERT((NULL != pThis));
            pThis->OnSetFocus(hWnd, message, wParam, lParam);
            break;

        case WM_CONTEXTMENU:
            DBGASSERT((NULL != pThis));
            pThis->OnContextMenu(hWnd, message, wParam, lParam);
            break;

        case CSCWM_DONESYNCING:
            pThis->Refresh();
            return 0;

        default:
            lResult = DefWindowProc(hWnd, message, wParam, lParam);
            break;
    }
    return lResult;
}



//
// WM_CREATE handler.
//
bool
CacheWindow::OnCreate(
     ViewType eViewType,
    const CString& strShare
    )
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("CacheWindow::OnCreate")));
    DBGASSERT((NULL != m_hwndMain));

    bool bResult = false;

    //
    // Cache the main menu handle.  We need it in several places.
    //
    m_hmenuMain = GetMenu(m_hwndMain);
    DBGASSERT((NULL != m_hmenuMain));
    //
    // Ensure comctl32 is loaded and initialized.
    //
    InitCommonControls();
    //
    // Order of these statements is important.
    // ChangeView must be called before CreateToolbar.
    // Note that this call to ChangeView() doesn't configure
    // the main window.  Window configuration must occur after
    // the toolbar and statusbar are created (See CreateMainWindow).
    // Also note that we tell ChangeView to NOT begin loading the
    // view just yet.  We first want the view to appear so that the user
    // sees a window while the view is loading.
    //
    ChangeView(eViewType, strShare, false, false);
    if (CreateToolbar() && CreateStatusbar())
    {
        bResult = true;
    }

    return bResult;
}


//
// Create a new view object to match the desired view type.
// If the view type is eDetailsView or eStaleView, the view
// is opened on the share specified in strShare.  If strShare is
// empty, the view opens displaying all shares.
// strShare is ignored if the view type is eShareView.
//
CacheView *
CacheWindow::CreateView(
    ViewType eViewType,
    const CString& strShare
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::CreateView")));
    DBGPRINT((DM_VIEW, DL_MID, TEXT("\teViewType = %d, share = \"%s\""), (int)eViewType, strShare.Cstr()));
    DBGASSERT((IsValidViewType(eViewType)));
    DBGASSERT((NULL != m_hInstance));
    DBGASSERT((NULL != m_hwndMain));

    CacheView *pView = NULL;

    RECT rcView;
    GetClientRect(m_hwndMain, &rcView);

    switch(eViewType)
    {
        case eDetailsView:
            pView = new DetailsView(m_hInstance, 
                                    m_hwndMain, 
                                    rcView, 
                                    *m_pCscObjTree,
                                    strShare);
            break;

        case eStaleView:
            pView = new StaleView(m_hInstance, 
                                  m_hwndMain, 
                                  rcView, 
                                  *m_pCscObjTree,
                                  strShare);
            break;

        case eShareView:
            pView = new ShareView(m_hInstance, 
                                  m_hwndMain, 
                                  rcView, 
                                  *m_pCscObjTree,
                                  strShare);
            break;

        default:
            //
            // If you hit this, something's wrong with IsValidViewType().
            // An invalid view type should have been caught earlier
            // by the DBGASSERT.
            //
            DBGASSERT((false));
            break;
    }

    //
    // This will trigger the filling of the listview when
    // CVM_GETNEXTCSCOBJECT is posted.
    //
    m_bViewPopulated = false;

    return pView;
}


//
// End the icon hound thread.  If it's exited then
// it's OK to end the session now.
//
LRESULT
CacheWindow::OnQueryEndSession(
    HWND hWnd, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("CacheWindow::OnQueryEndSession")));
    //
    // This call clears the icon hound thread's input queue and places
    // a sentinel value on the queue causing the thread to exit normally
    // when the sentinel is popped from the queue.  It will block for a max
    // of 2 seconds waiting to see if the icon hound thread ends.
    // If it has exited within 2 seconds, then we say the session can
    // end.  Otherwise, we're not ready to end the session.
    //
    LRESULT lOkToEnd = EndIconHoundThread(2000);
    DBGPRINT((DM_VIEW, DL_HIGH, TEXT("OnQueryEndSession result = %d"), lOkToEnd));
    return lOkToEnd;
}


//
// Centralize the function of closing down the icon hound thread.
// By default, the function doesn't wait to see if the thread has
// stopped running.  Specify a value in milliseconds (dwWait) to
// wait to see if it stops.  If it hasn't stopped, it's probably 
// down inside a call to SHGetFileInfo and hasn't returned. 
// CacheView::EndIconHoundThread will clear the thread's input queue
// and place a sentinel value on it so the thread will terminate
// it's operations as soon as it can.
//
// Returns:
//
//      true   = icon hound thread exited or was not running.
//      false  = icon hound thread still running.
//
bool
CacheWindow::EndIconHoundThread(
    DWORD dwWait   // Default is INFINITE.
    )
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("CacheWindow::EndIconHoundThread")));
    bool bThreadExited = true;
    if (NULL != m_pView)
    {
        bThreadExited = !m_pView->IsIconHoundThreadRunning(0) ||
                         m_pView->EndIconHoundThread(dwWait);
    }
    return bThreadExited;
}


//
// Validate a view type value.
//
bool
CacheWindow::IsValidViewType(
    ViewType eViewType
    ) const throw()
{
    return eViewType >= eShareView && eViewType <= eStaleView;
}


//
// Create a status DWORD to return to test apps querying
// for viewer status.  Feel free to add bits to this status
// word as needed.
//
LRESULT
CacheWindow::OnTestGetViewerStatus(
    void
    ) throw()
{
    LRESULT status = (m_eCurrentView & VSF_TYPEMASK);

    if (m_bToolbarVisible)
        status |= VSF_TOOLBAR;
    if (m_bStatusbarVisible)
        status |= VSF_STATUSBAR;
    if (m_bViewPopulated)
        status |= VSF_POPULATED;

    return status;
}


LRESULT
CacheWindow::OnTestGetItemStatus(
    int iItem
    ) throw()
{
    LRESULT status = 0;
    if (-1 != iItem)
    {
        CscObject *pObj = CacheView::GetItemObject(m_pView->GetViewWindow(), iItem);
        if (NULL != pObj)
        {
            //
            // Get item icon ID.
            //
            HWND hwndLV = m_pView->GetViewWindow();
            LVITEM item;
            item.iItem    = iItem;

            if (LVIS_SELECTED & ListView_GetItemState(hwndLV, iItem, LVIS_SELECTED))
                status |= ISF_SELECTED;

            item.mask     = LVIF_IMAGE;
            item.iSubItem = (eShareView == m_eCurrentView) ? 0 : 1;
            ListView_GetItem(hwndLV, &item);
            status |= (item.iImage & ISF_ICONIDMASK);

            //
            // Determine if the "pinned" icon is displayed.
            //
            if (eShareView != m_eCurrentView)
            {
                item.iSubItem = 0;
                ListView_GetItem(hwndLV, &item);
                if (item.iImage == iIMAGELIST_ICON_PIN)
                    status |= ISF_PINNED;
            }
        }
    }
    return status;
}


//
// Remove any items from the "File" menu or context menu restricted by 
// system policy.  This function assumes that the menu passed is the 
// "File" menu or the item context menu.
//
void
CacheWindow::ApplyPolicyToFileMenu(
    HMENU hmenu
    )
{
    bool bViewerRW = CSettings::eViewerReadWrite == g_pSettings->CacheViewerMode();
    bool bNoPin    = !bViewerRW || g_pSettings->NoMakeAvailableOffline();
    bool bNoSync   = !bViewerRW || g_pSettings->NoManualSync();
    bool bNoDelete = !bViewerRW;

    //
    // Remove menu items restricted by policy.
    //
    struct
    {
        UINT idCmd;
        bool bDelete;
    } rgMenu[] = { { IDM_FILE_PIN,    bNoPin    },
                   { IDM_FILE_UNPIN,  bNoPin    },
                   { IDM_FILE_UPDATE, bNoSync   },
                   { IDM_FILE_DELETE, bNoDelete },
                 };

    for (int i = 0; i < ARRAYSIZE(rgMenu); i++)
    {
        if (rgMenu[i].bDelete)
            DeleteMenu(hmenu, rgMenu[i].idCmd, MF_BYCOMMAND);
    }
}


//
// Apply system policy to the viewer.  This is principally the removal of
// command controls restricted by policy.
//
void
CacheWindow::ApplyPolicy(
    void
    )
{
    ApplyPolicyToFileMenu(GetSubMenu(m_hmenuMain, 
                                     g_MenuItemPos.Position(iMENU_ITEM_FILE)));

    //
    // Note: Policy is applied dynamically to toolbar buttons in 
    // UpdateButtonsAndMenuItems().
    //
}


//
// CVM_GETNEXTCSCOBJECT handler.
//
void
CacheWindow::OnGetNextCscObject(
    void
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::OnGetNextCscObject")));
    //
    // Obtain the view lock so we don't try to update while in the
    // middle of changing views.
    //
    AutoLockCs lock(m_ViewCs);
    if (NULL != m_pView)
    {
        int cObjectsAddedToView = 0;
        bool bObjectEnumerated  = false;
        //
        // LoadNextCscObject loads a single object from the object tree
        // into a small cache maintained by the view.  If the cache is 
        // full, it's items are flushed and added to the listview.
        // On return, bObjectEnumerated indicates if an object was 
        // enumerated in the object tree.  cObjectsAddedToView contains the 
        // count of objects added to the listview.
        //
        m_bViewPopulated = !m_pView->LoadNextCscObject(&bObjectEnumerated, 
                                                       &cObjectsAddedToView);

        if (0 < cObjectsAddedToView)
        {
            ShowItemCountInStatusbar();
        }
        if (m_bViewPopulated)
        {
            KillTimer(m_hwndMain, ID_TIMER_GETNEXTCSCOBJECT);
            DBGPRINT((DM_VIEW, DL_MID, TEXT("Killed GetNextCscObject timer")));
        }
        else
        {
            if (bObjectEnumerated)
            {
                //
                // We enumerated an object from the object tree.  It's likely
                // there's another one waiting.  Post another message now.
                //
                DBGPRINT((DM_VIEW, DL_MID, TEXT("Posting GetNextCscObject message")));
                PostMessage(m_hwndMain, CVM_GETNEXTCSCOBJECT, 0, 0);
            }
            else
            {
                //
                // Didn't enumerate an object this time.  Set the timer
                // to post another CVM_GETNEXTCSCOBJECT in
                // 100ms.
                //
                DBGPRINT((DM_VIEW, DL_MID, TEXT("Setting GetNextCscObject timer")));
                SetTimer(m_hwndMain, ID_TIMER_GETNEXTCSCOBJECT, 100, NULL);
            }
        }
    }
}


//
// CVM_UPDATEOBJECT handler.
// This custom message is posted to the viewer window whenever the 
// view's "icon hound" thread has located another icon asynchronously.
//
void
CacheWindow::OnUpdateObject(
    CscObject *pObject
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::OnUpdateObject")));
    DBGASSERT((NULL != pObject));
    AutoLockCs lock(m_ViewCs);
    if (NULL != m_pView)
        m_pView->ObjectChanged(pObject);    
}


void
CacheWindow::OnLVItemCountChanged(
    int cItems
    )
{
    if (0 == (-2 & cItems))
    {
        DBGASSERT((0 == cItems || 1 == cItems));
        //
        // If the item count is either 0 or 1, update the enabled-ness
        // of the toolbar buttons and menu items.  For example, if the 
        // view is empty, we don't want the "delete" button or menu
        // item enabled.
        //
        UpdateButtonsAndMenuItems();
    }
}


//
// Display help text in the status bar for a toolbar button.
//
void
CacheWindow::ShowToolbarButtonTextInStatusbar(
    int iBtn
    )
{
    int idCmd = -1;
    if (0 <= iBtn)
    {
        TBBUTTON btn;
        ToolBar_GetButton(m_hwndToolbar, iBtn, &btn);
        idCmd = btn.idCommand;
    }
    ShowMenuTextInStatusbar(idCmd);
}


//
// Update the item count information displayed in the status bar.
//
void
CacheWindow::ShowItemCountInStatusbar(
    void
    )
{
    //
    // Only display item count information if the menu isn't currently
    // active.  If the menu is active, we're displaying menu item
    // description text.
    //
    if (!m_bMenuActive && NULL != m_hwndStatusbar && !m_bStatusbarDisabled)
    {
        int cItems    = 0;
        int cSelected = 0;

        if (NULL != m_pView)
        {
            cItems    = m_pView->ObjectCount();
            cSelected = m_pView->SelectedObjectCount();
        }
        DBGASSERT((NULL != m_hInstance));

        static CString strItems;
        static CString strSelected;
        strItems.FormatNumber(cItems);
        strSelected.FormatNumber(cSelected);

        CString strStatusbar(m_hInstance, IDS_FMT_STATUSBAR, strItems.Cstr(), strSelected.Cstr());
        SetStatusbarText(0, strStatusbar);

        strItems.Empty();
        strSelected.Empty();
    }
}



//
// Display a menu item's description text in the statusbar.
//
void
CacheWindow::ShowMenuTextInStatusbar(
    DWORD idMenuOption
    )
{
    DBGASSERT((NULL != m_hInstance));
    DBGASSERT((NULL != m_hwndStatusbar));

    CString strText; // Default to blank text in the status bar.

    if (IDM_FIRST <= idMenuOption && IDM_LAST >= idMenuOption)
    {
        //
        // If the menu cmd ID is for a valid menu command, display it's
        // description in the status bar.
        //
        strText.Format(m_hInstance, idMenuOption);
    }
    else if (IDM_SHARES_FIRST <= idMenuOption && IDM_SHARES_LAST >= idMenuOption)
    {
        //
        // If the menu cmd ID is for an item in the "Shares" menu,
        // display a standard message in the status bar.
        //
        strText.Format(m_hInstance, IDM_SHARES_FIRST);
    }
    SetStatusbarText(0, strText);
}


//
// Create the toolbar object.  eViewType identifies the initial
// view selected in the "View" buttons.  These three buttons
// "Shares", "Details" and "Stale" act as radio buttons.
//
bool
CacheWindow::CreateToolbar(
    void
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::CreateToolbar")));
    DBGASSERT((NULL != m_hwndMain));
    DBGASSERT((NULL != m_hInstance));
    DBGASSERT((NULL != m_pView));

    //
    // Array describing each of the tool bar buttons.
    //
    static const TBBUTTON rgToolbarBtns[] = {
        { iIMAGE_BTN_OPEN,       IDM_FILE_OPEN,       TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { iIMAGE_BTN_SAVEAS,     IDM_FILE_SAVEAS,     TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { iIMAGE_BTN_DELETE,     IDM_FILE_DELETE,     TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { iIMAGE_BTN_PIN,        IDM_FILE_PIN,        TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { iIMAGE_BTN_UNPIN,      IDM_FILE_UNPIN,      TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { iIMAGE_BTN_UPDATE,     IDM_FILE_UPDATE,     TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { 0,                     0,                   0,               TBSTYLE_SEP,    0L, 0},
        { iIMAGE_BTN_SHARES,     IDM_VIEW_SHARES,     TBSTATE_ENABLED, TBSTYLE_GROUP | TBSTYLE_CHECK, 0L, 0},
        { iIMAGE_BTN_STALE,      IDM_VIEW_STALE,      TBSTATE_ENABLED, TBSTYLE_GROUP | TBSTYLE_CHECK, 0L, 0},
        { iIMAGE_BTN_DETAILS,    IDM_VIEW_DETAILS,    TBSTATE_ENABLED, TBSTYLE_GROUP | TBSTYLE_CHECK, 0L, 0}
        };

    m_hwndToolbar = CreateToolbarEx(m_hwndMain,
                                    WS_CHILD | WS_BORDER | WS_VISIBLE | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT,
                                    IDC_CACHEVIEW_TOOLBAR,
                                    cBUTTON_IMAGES,
                                    m_hInstance,
                                    IDB_CACHEVIEW_TOOLBAR,
                                    (LPCTBBUTTON)rgToolbarBtns,
                                    ARRAYSIZE(rgToolbarBtns),
                                    16, 16,
                                    16, 16,
                                    sizeof(TBBUTTON));

    if (NULL == m_hwndToolbar) 
    {
        DBGERROR((TEXT("Error 0x%08X creating toolbar window"), GetLastError()));
    }
    else
    {
        //
        // Subclass the toolbar window so we can intercept mouse messages and
        // display toolbar text in the statusbar.
        //
        m_TBSubclass.Initialize(m_hwndToolbar);

        CString strLabel(m_hInstance, IDS_TXT_LISTSHARES);

        //
        // Calculate the display rect for the share combo label text.
        //
        HDC hdc = GetDC(m_hwndToolbar);
        SelectObject(hdc, m_pView->GetListViewFont()); // Need font for rect calc.
        RECT rc = {0, 0, 1, 1};
        DrawText(hdc, strLabel, strLabel.Length(), &rc, DT_CALCRECT);
        ReleaseDC(m_hwndToolbar, hdc);

        m_hwndComboLabel = CreateWindowEx(0,
                                          TEXT("static"),
                                          TEXT(""),
                                          WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                          0, 0,
                                          rc.right - rc.left,
                                          rc.bottom - rc.top,
                                          m_hwndToolbar,
                                          (HMENU)IDC_CACHEVIEW_SHARE_COMBO_LABEL,
                                          m_hInstance,
                                          NULL);


        if (NULL == m_hwndComboLabel)
        {
            DBGERROR((TEXT("Error 0x%08X creating shares combo label"), GetLastError()));
        }
        else
        {
            SendMessage(m_hwndComboLabel, WM_SETFONT, (WPARAM)m_pView->GetListViewFont(), 0);
            SetWindowText(m_hwndComboLabel, strLabel);
        }

        //
        // Create the dropdown combo containing names of the available
        // shares in the CSC database.
        //
        m_hwndCombo = CreateWindowEx(0,
                                     TEXT("combobox"),
                                     TEXT(""),
                                     WS_CHILD | WS_VISIBLE | WS_BORDER | 
                                     CBS_HASSTRINGS | CBS_DROPDOWNLIST | WS_VSCROLL,
                                     0, 0, 
                                     CX_SHARE_COMBO,
                                     CY_SHARE_COMBO,
                                     m_hwndToolbar,
                                     (HMENU)IDC_CACHEVIEW_SHARE_COMBO,
                                     m_hInstance,
                                     NULL);
        if (NULL == m_hwndCombo)
        {
            DBGERROR((TEXT("Error 0x%08X creating shares combo box"), GetLastError()));
        }
        else
        {
            //
            // Set the font in the toolbar combo to be the same as that 
            // used in listview.  This assumes that the listview
            // has already been created.
            //
            SendMessage(m_hwndCombo, WM_SETFONT, (WPARAM)m_pView->GetListViewFont(), 0);
        }
    }

    return NULL != m_hwndToolbar && NULL != m_hwndComboLabel && NULL != m_hwndCombo;
}


//
// Set the visual appearance (pressed vs. raised) of the 3 view
// buttons in the toolbar.  Also checks/unchecks the corresponding
// view menu options.  The view used is the current value
// of m_eCurrentView.
//
void
CacheWindow::UpdateViewButtons(
    void
    ) throw()
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("CacheWindow::UpdateViewButtons")));
    DBGPRINT((DM_VIEW, DL_LOW, TEXT("\tView type = %d"), (int)m_eCurrentView));

    DBGASSERT((NULL != m_hwndToolbar));
    DBGASSERT((NULL != m_hmenuMain));
    DBGASSERT((IsValidViewType(m_eCurrentView)));

    static const struct
    {
        ViewType vt; // View type.
        int idCmd;   // Corresponding menu/button command ID.

    } rgMap[] = { { eShareView,   IDM_VIEW_SHARES  },
                  { eDetailsView, IDM_VIEW_DETAILS },
                  { eStaleView,   IDM_VIEW_STALE   } };

    for (int i = 0; i < ARRAYSIZE(rgMap); i++)
    {
        bool bSelected = (m_eCurrentView == rgMap[i].vt);
        int idCmd      = rgMap[i].idCmd;
        //
        // Press the toolbar button corresponding to the view.
        // Un-press all others.
        //
        ToolBar_CheckButton(m_hwndToolbar, idCmd, bSelected);
        //
        // Check the View menu item corresponding to the view.
        // Uncheck all other View menu items.
        //
        CheckMenuItem(m_hmenuMain,
                      idCmd,
                      MF_BYCOMMAND | (bSelected ? MF_CHECKED : MF_UNCHECKED));
    }
}


//
// Create the status bar control.
//
bool
CacheWindow::CreateStatusbar(
    void
    )
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("CacheWindow::CreateStatusbar")));
    DBGASSERT((NULL != m_hwndMain));
    DBGASSERT((NULL != m_hInstance));

    m_hwndStatusbar = CreateWindow(STATUSCLASSNAME,
                                   TEXT(""),
                                   WS_VISIBLE | WS_CHILD | WS_BORDER | SBS_SIZEGRIP,
                                   0, 0, 0, 0,
                                   m_hwndMain,
                                   (HMENU)NULL,
                                   m_hInstance,
                                   NULL);

    if (NULL == m_hwndStatusbar) 
    {
        DBGERROR((TEXT("Error 0x%08X creating status bar."), GetLastError()));
    }
    else
    {
        //
        // Create the dropdown combo containing names of the available
        // shares in the CSC database.
        //
        m_hwndProgressbar = CreateWindowEx(0,
                                     PROGRESS_CLASS,
                                     TEXT(""),
                                     WS_CHILD | WS_VISIBLE,
                                     0, 0, 0, 0,
                                     m_hwndStatusbar,
                                     (HMENU)IDC_CACHEVIEW_PROGRESSBAR,
                                     m_hInstance,
                                     NULL);
    }

    return NULL != m_hwndStatusbar && NULL != m_hwndProgressbar;
}


//
// WM_SIZE handler.
//
LRESULT
CacheWindow::OnSize(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    ) throw()
{
    RECT rcListView;

    //
    // Start with the main window rect.
    //
    GetClientRect(hWnd, &rcListView);

    if (IsToolbarVisible())
    {
        //
        // Adjust toolbar if it's visible.
        //
        RECT rcToolbar;
        int cyToolbar = 0;

        ToolBar_AutoSize(m_hwndToolbar);
        GetClientRect(m_hwndToolbar, &rcToolbar);

        cyToolbar = rcToolbar.bottom - rcToolbar.top;
        rcListView.top += (cyToolbar + 1);
        //
        // Position the share list combo label to the immediate right of the
        // rightmost toolbar button, then the share combo to the right of 
        // the label.
        //
        int cButtons = ToolBar_ButtonCount(m_hwndToolbar);
        if (0 < cButtons)
        {
            RECT rcButton;
            RECT rcLabel;
            RECT rcCombo;

            ToolBar_GetItemRect(m_hwndToolbar, cButtons - 1, &rcButton);
            GetClientRect(m_hwndComboLabel, &rcLabel);
            GetClientRect(m_hwndCombo, &rcCombo);

            int cyButton = rcButton.bottom - rcButton.top;
            int cxButton = rcButton.right  - rcButton.left;
            int cyLabel  = rcLabel.bottom  - rcLabel.top;
            int cxLabel  = rcLabel.right   - rcLabel.left;
            int cyCombo  = rcCombo.bottom  - rcCombo.top;
            int cxCombo  = rcCombo.right   - rcCombo.left;

            MoveWindow(m_hwndComboLabel, rcButton.right + cxButton,
                                         rcButton.top + ((cyButton - cyLabel) / 2),
                                         cxLabel,
                                         cyLabel,
                                         false);

            rcCombo.left   = rcButton.right + cxButton + cxLabel + 6;
            rcCombo.top    = rcButton.top;
            rcCombo.right  = rcCombo.left + cxCombo;
            rcCombo.bottom = rcCombo.top + cyCombo;

            MoveWindow(m_hwndCombo, rcCombo.left,
                                    rcCombo.top,
                                    cxCombo,
                                    cyCombo,
                                    false);
        }

    }
    if (IsStatusbarVisible())
    {
        //
        // Adjust status bar if it's visible.
        //
        RECT rcStatusbar;
        RECT rcProgressbar;
        int cyStatusbar = 0;
        int cxStatusbar = 0;

        SendMessage(m_hwndStatusbar, message, wParam, lParam);
        GetClientRect(m_hwndStatusbar, &rcStatusbar);

        cyStatusbar = rcStatusbar.bottom - rcStatusbar.top;
        cxStatusbar = rcStatusbar.right - rcStatusbar.left;
        rcListView.bottom -= cyStatusbar;

        int rgWidths[3];
        rgWidths[0] = cxStatusbar / 2;
        rgWidths[1] = rgWidths[0] + (cxStatusbar / 4);
        rgWidths[2] = -1;

        StatusBar_SetParts(m_hwndStatusbar, ARRAYSIZE(rgWidths), rgWidths);
        StatusBar_GetRect(m_hwndStatusbar, 2, &rcProgressbar);

        MoveWindow(m_hwndProgressbar, rcProgressbar.left,
                                      rcProgressbar.top,
                                      rcProgressbar.right - rcProgressbar.left,
                                      rcProgressbar.bottom - rcProgressbar.top,
                                      FALSE);
    }
    //
    // Adjust the listview.  Accounts for toolbar and status bar.
    //
    m_pView->Move(0, 
                  rcListView.top, 
                  rcListView.right - rcListView.left, 
                  rcListView.bottom - rcListView.top,
                  true);

    return 0;
}


//
// WM_DESTROY handler.
//
LRESULT
CacheWindow::OnDestroy(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("CacheWindow::OnDestroy")));

    m_TBSubclass.Cancel();
    m_ProgressBar.Destroy();

    SaveWindowState();

    if (NULL != m_pView)
        m_pView->SaveViewState();

    if (NULL != m_hmenuShares)
        DestroyMenu(m_hmenuShares);

    m_hwndCombo = NULL;

    if (NULL != m_pView)
    {
        //
        // The view's icon hound, and image list are all static 
        // objects.  Destroy them when the final view is destroyed.
        // I originally was destroying these in the CacheWindow dtor.
        // The problem is that by the time we've reached the dtor, 
        // the windowing elements have all been destroyed.  Therefore,
        // we need to destroy these here.
        //
        m_pView->DestroyIconHound();
        m_pView->DestroyImageList();
        delete m_pView;
        m_pView = NULL;
    }

    PostQuitMessage(0);
    return 0;
}


//
// WM_NOTIFY handler.
//
LRESULT 
CacheWindow::OnNotify(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    NMHDR *pnmhdr = (NMHDR *)lParam;

    switch(pnmhdr->code)
    {
        case LVN_GETDISPINFO:
            OnLVN_GetDispInfo((LV_DISPINFO *)lParam);
            break;

        case LVN_COLUMNCLICK:
            OnLVN_ColumnClick((NM_LISTVIEW *)lParam);
            break;

        case TTN_NEEDTEXT:
            OnTTN_NeedText((TOOLTIPTEXT *)lParam);
            break;

        case LVN_ITEMCHANGED:
            OnLVN_ItemChanged((NM_LISTVIEW *)lParam);
            break;

        case PBN_100PCT:
            OnPBN_100Pct();
            break;

        case NM_DBLCLK:
        case NM_RETURN:
            if (IDC_CACHEVIEW_LISTVIEW == pnmhdr->idFrom)
            {
                if (eShareView == m_eCurrentView)
                {
                    //
                    // If the user double-clicks or presses [Return] on 
                    // an item in Share view, open Details view on the
                    // currently-selected share.
                    //
                    ViewSelectionChanged(eDetailsView, true);
                }
                else
                {
                    //
                    // If the user double-clicks or presses [Return] on 
                    // an item in Details or Stale view, AND there is
                    // only one item selected AND that item is a file, 
                    // open the file.  CanOpen() is the logic that determines
                    // if the "Open" toolbar button or menu item is available.
                    //
                    if (m_pView->CanOpen())
                        OpenSelectedItem();
                }
            }
            break;

        default:
            break;
    }

    return 0;
}


//
// LVN_GETDISPINFO notification handler.
//
LRESULT
CacheWindow::OnLVN_GetDispInfo(
    LV_DISPINFO *pdi
    )
{
    DBGASSERT((NULL != m_pView));
    DBGASSERT((NULL != pdi));
    //
    // Ask the view to provide display information.
    // The view will pass the display info request on to the
    // appropriate view column object.
    //
    m_pView->GetDispInfo(pdi);
    return 0;
}


//
// Retrieve the share name and share display name for the currently-
// selected item in the "shares" combo box.  Both pstrShare
// and pstrDisplayName are optional depending upon which string
// is desired.
//
int
CacheWindow::GetShareComboSelection(
    CString *pstrShare,
    CString *pstrDisplayName
    ) const
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::GetShareComboSelection")));
    //
    // Ensure output strings are empty.
    //
    if (NULL != pstrShare)
        pstrShare->Empty();
    if (NULL != pstrDisplayName)
        pstrDisplayName->Empty();

    //
    // Retrieve the share display name and parsable share path from the combo box
    // for the currently selected combo item.
    //
    int iSel = ComboBox_GetCurSel(m_hwndCombo);
    if (CB_ERR != iSel)
    {
        DWORD dwItemData = ComboBox_GetItemData(m_hwndCombo, iSel);
        CscShare *pShare = reinterpret_cast<CscShare *>(dwItemData);
        if (NULL != pShare)
        {
            if (NULL != pstrShare)
                pShare->GetName(pstrShare);

            if (NULL != pstrDisplayName)
                pShare->GetLongDisplayName(pstrDisplayName);
        }
    }
    return iSel;
}        


//
// Called when the user changes the share selection in the UI.
// Destroys the current view and creates a new one for the share
// selection.
//
void
CacheWindow::ShareComboSelectionChanged(
    void
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::ShareComboSelectionChanged")));

    CString strShare;
    CString strDisplayName;
    if (CB_ERR != GetShareComboSelection(&strShare, &strDisplayName))
    {
        SetShareMenuSelection(strDisplayName);
        ChangeView(m_eCurrentView, strShare);
    }
}

//
// Selects strShare as the current item in the combo box.
//
void
CacheWindow::SetShareComboSelection(
    const CString& strShare
    ) throw()
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::SetShareComboSelection")));
    if (NULL != m_hwndCombo)
    {
        ComboBox_SelectString(m_hwndCombo, 0, strShare);
    }
}

//
// Called when the user selects a share from the "Shares" menu.
//
void
CacheWindow::ShareMenuSelectionChanged(
    int idShare
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::ShareMenuSelectionChanged")));
    DBGASSERT((NULL != m_hmenuShares));

    CString strShareDisplay;
    CString strShare;
    DBGASSERT((IDM_MORE_SHARES == IDM_SHARES_LAST));
    if (IDM_MORE_SHARES == idShare)
    {
        //
        // User selected the "Shares..." item.  Display the "Shares"
        // dialog and get user response.
        //
        CArray<CscShare *> rgpShares;
        m_pCscObjTree->GetShareList(&rgpShares);

        ShareDlg dlg(m_hInstance, m_hwndMain, rgpShares, &strShare, &strShareDisplay);
        if (!dlg.Run())
        {
            return; // User cancelled or closed without making selection.
        }
    }
    else
    {
        //
        // Get the share display name from the selected menu item.
        //
        MENUITEMINFO mii;
        mii.cbSize     = sizeof(mii);
        mii.fMask      = MIIM_DATA;
        GetMenuItemInfo(m_hmenuShares, idShare - IDM_SHARES_FIRST, true, &mii);
        CscShare *pShare = reinterpret_cast<CscShare *>(mii.dwItemData);
        if (NULL != pShare)
        {
            pShare->GetName(&strShare);
            pShare->GetLongDisplayName(&strShareDisplay);
        }
        else
        {
            strShareDisplay.Format(m_hInstance, IDS_ALL_SHARES);
        }
    }
    //
    // Check the selected item in the share's menu.
    //
    SetShareMenuSelection(strShareDisplay);
    //
    // Select the corresponding combo box item.
    //
    SetShareComboSelection(strShareDisplay);
    //
    // Make the view change and configure the window for the new view.
    //
    ChangeView(m_eCurrentView, strShare);
}


//
// Given a share display name, check the corresponding entry in the
// "Shares" menu.
//
void
CacheWindow::SetShareMenuSelection(
    const CString& strShare
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::SetShareMenuSelection")));

    if (NULL != m_hmenuShares)
    {
        CString strShareDisplay;
        MENUITEMINFO mii;
        mii.cbSize     = sizeof(mii);
        mii.fMask      = MIIM_TYPE;
        mii.dwTypeData = strShareDisplay.GetBuffer(MAX_PATH);

        int cMenuItems = GetMenuItemCount(m_hmenuShares);
        for (int i = 0; i < cMenuItems; i++)
        {
            mii.cch = strShareDisplay.Size(); // GetMenuItemInfo modifies cch.
            if (GetMenuItemInfo(m_hmenuShares, i, true, &mii))
            {
                //
                // Check the Share menu item corresponsding to the share.
                // Uncheck all other Share menu items.
                //
                CheckMenuItem(m_hmenuShares,
                              i,
                              MF_BYPOSITION | (strShare == strShareDisplay ? MF_CHECKED : MF_UNCHECKED));
            }
        }
    }
}


//
// Show/hide the share's combo.
//
void
CacheWindow::ShowShareCombo(
    bool bShow
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::ShowShareCombo")));
    DBGASSERT((NULL != m_hwndCombo));
    DBGASSERT((NULL != m_hwndComboLabel));
    DBGASSERT((NULL != m_pView));

    //
    // Show/hide the combo and combo label. 
    //
    int nCmdShow = bShow ? SW_SHOW : SW_HIDE;
    ShowWindow(m_hwndCombo, nCmdShow);
    ShowWindow(m_hwndComboLabel, nCmdShow);

    if (bShow)
    {
        //
        // Set the combo and label font to match that of the listview.
        //
        HFONT hFont = m_pView->GetListViewFont();
        SendMessage(m_hwndCombo, WM_SETFONT, (WPARAM)hFont, 0);
        SendMessage(m_hwndComboLabel, WM_SETFONT, (WPARAM)hFont, 0);
        //
        // Set the combo's current selection to match the current share
        // being viewed.
        //
        CString strDisplayName;
        int cItems = ComboBox_GetCount(m_hwndCombo);
        if (0 < cItems)
        {
            int iSel = -1;
            for (int i = 0; i < cItems; i++)
            {
                DWORD dwItemData = ComboBox_GetItemData(m_hwndCombo, i);
                CscShare *pShare = reinterpret_cast<CscShare *>(dwItemData);
                CString strShare;
                if (NULL != pShare)
                    pShare->GetName(&strShare);

                if (strShare == m_strCurrentShare)
                {
                    iSel = i;
                    break;
                }
            }
            DBGASSERT((-1 != iSel));
            ComboBox_SetCurSel(m_hwndCombo, iSel);
        }
    }
    UpdateWindow(m_hwndCombo);
}


//
// Whenever the view changes, this function is called to re-configure the window
// elements for the current view.
//
void
CacheWindow::ConfigureWindowForView(
    void
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::ConfigureWindowForView")));

    UpdateWindowTitle();
    UpdateViewButtons();
    
    bool bFillSharesMenu  = InsertOrRemoveSharesMenu();
    bool bFillSharesCombo = (NULL != m_hwndCombo);
    FillShareComboAndMenu(bFillSharesCombo, bFillSharesMenu);

    if (IsToolbarVisible())
    {
        //
        // Share combo is hidden in "share" view; visible in others.
        //
        ShowShareCombo(eShareView != m_eCurrentView);

    }

    UpdateButtonsAndMenuItems();
    InitProgressBarForView(PROGRESSBAR_UPDATE_MS);
    UpdateWindowLayout();
}


//
// Enable/disable some of the toolbar buttons and "File" menu
// items based on the current view.
//
void
CacheWindow::UpdateButtonsAndMenuItems(
    void
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::UpdateButtonsAndMenuItems")));

    bool bViewerRW = CSettings::eViewerReadWrite == g_pSettings->CacheViewerMode();
    bool bNoPin    = !bViewerRW || g_pSettings->NoMakeAvailableOffline();
    bool bNoSync   = !bViewerRW || g_pSettings->NoManualSync();
    bool bNoDelete = !bViewerRW;

    //
    // PBMF means "pointer to boolean member function".
    //
    typedef bool (CacheView::*PBMF)(void) const;

    struct
    {
        int  idCmd;      // Menu/toolbar button command ID.
        PBMF pfn;        // Boolean function to determine view capability.
        bool bRestricted;// Policy says disable?

    } rgCmdAndFunc[] = {{ IDM_FILE_DELETE, m_pView->CanDelete, bNoDelete },
                        { IDM_FILE_PIN,    m_pView->CanPin,    bNoPin    },
                        { IDM_FILE_UNPIN,  m_pView->CanUnpin,  bNoPin    },
                        { IDM_FILE_SAVEAS, m_pView->CanSaveAs, false     },
                        { IDM_FILE_UPDATE, m_pView->CanUpdate, bNoSync   },
                        { IDM_FILE_OPEN,   m_pView->CanOpen,   false     }
                       };

    static const int rgmf[] = {MF_GRAYED, MF_ENABLED};

    HMENU menu = GetSubMenu(m_hmenuMain, 0);

    for (int i = 0; i < ARRAYSIZE(rgCmdAndFunc); i++)
    {
        int idCmd        = rgCmdAndFunc[i].idCmd;
        PBMF pfn         = rgCmdAndFunc[i].pfn;
        bool bRestricted = rgCmdAndFunc[i].bRestricted;

        //
        // Enable/disable toolbar buttons based on the view capabilities and
        // policy restrictions.
        //
        ToolBar_EnableButton(m_hwndToolbar, idCmd, !bRestricted && (m_pView->*pfn)());
        //
        // Do the same for items on the "File" menu.  Menu items are 
        // actually restricted by removing them from the menu when the
        // main window is created.  However, to be consistent with the disabling
        // of toolbar buttons, we also disable them here.  This way you won't
        // have a menu item available without a toolbar button.
        //
        EnableMenuItem(menu, idCmd, rgmf[!bRestricted && (m_pView->*pfn)()]);
    }
}


//
// Add the share display names to the shares combo and "Share" menu.
// Note:  Combining the filling of the combo and menu into a single
//        function is a little unconventional.  However, both contain 
//        the same textual information.  We can leverage some
//        common processing by filling both in one function.
//
void
CacheWindow::FillShareComboAndMenu(
    bool bFillCombo,
    bool bFillMenu
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::FillShareComboAndMenu")));

    if (bFillMenu || bFillCombo)
    {
        if (bFillCombo)
            ClearSharesCombo();

        if (bFillMenu)
            ClearSharesMenu();             

        //
        // Retrieve a list of shares from the object tree and add their
        // text labels to the combo box.   Note that the combo displays
        // the long-format display name (i.e. "'worf' on ntspecs") while 
        // the itemdata for each combo item stores a pointer to the 
        // corresponding CscShare object in the object tree.  Likewise,
        // the menu displays the long-format name with a CscShare ptr
        // in the item's itemdata.
        //
        CArray<CscShare *> rgpShares;
        m_pCscObjTree->GetShareList(&rgpShares);

        int j = 0;
        int cShares = rgpShares.Count();
        for (int i = 0; i < cShares + 1; i++)
        {
            CString strName;
            CString strDisplayName;
            CscShare *pShare = NULL;

            if (0 == i)
            {
                //
                // First item in the list is "All Shares".
                //
                strName = TEXT("");
                strDisplayName.Format(m_hInstance, IDS_ALL_SHARES);
            }
            else
            {
                pShare = rgpShares[i-1];
                pShare->GetName(&strName);
                pShare->GetLongDisplayName(&strDisplayName);
            }

            DBGPRINT((DM_VIEW, DL_MID, TEXT("Adding \"%s\" to shares combo"), strDisplayName.Cstr()));
            
            if (bFillCombo)
            {
                //
                // Insert the share display name into the "Shares" combo.
                // Store pointer to CscShare object in combo item itemdata.
                // Current item in combo is set in ShowShareCombo().
                //
                ComboBox_AddString(m_hwndCombo, strDisplayName);
                ComboBox_SetItemData(m_hwndCombo, j, (LPTSTR)pShare);
            }

            if (bFillMenu)
            {
                if (MAX_SHARE_MENU_ITEMS >= j) 
                {
                    int idCmd = IDM_SHARES_FIRST + j;
                    if (MAX_SHARE_MENU_ITEMS == j)
                    {
                        //
                        // Append the "More Shares..." item that will
                        // invoke the "Shares" selection dialog.
                        //
                        strName = TEXT("..."); // An invalid share name.
                        strDisplayName.Format(m_hInstance, IDS_MORE_SHARES);
                        DBGASSERT((IDM_MORE_SHARES == IDM_SHARES_LAST));
                        idCmd = IDM_MORE_SHARES;
                    }
                    //
                    // Insert the share display name into the "Shares" menu.
                    //
                    AppendMenu(m_hmenuShares, MF_STRING, idCmd, strDisplayName);
                    //
                    // Store the share object ptr in the menu item's itemdata 
                    //
                    MENUITEMINFO mii;
                    mii.cbSize     = sizeof(mii);
                    mii.fMask      = MIIM_DATA;
                    mii.dwItemData = (DWORD)pShare;
                    SetMenuItemInfo(m_hmenuShares, j, true, &mii);
                    //
                    // Check the item corresponding to the "current" share.
                    // Uncheck all others.
                    //
                    CheckMenuItem(m_hmenuShares, 
                                  j,
                                  MF_BYPOSITION | ((m_strCurrentShare == strName) ? MF_CHECKED : MF_UNCHECKED));
                }
            }
            j++;
        }
    }
}



//
// The "Shares" menu is present only when in "Details" or "Stale" 
// view (the same time the shares combo is displayed).  This function
// either adds or removes the "Shares" menu depending on the
// current view.  Note that it only adds the menu.  It does not 
// populate the menu.  Menu population is done in FillShareComboAndMenu().
//
bool
CacheWindow::InsertOrRemoveSharesMenu(
    void
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::InsertOrRemoveSharesMenu")));
    DBGASSERT((NULL != m_hmenuMain));

    int cMenuItems  = GetMenuItemCount(m_hmenuMain);
    if (eShareView != m_eCurrentView)
    {
        if (NULL == m_hmenuShares)
        {
            //
            // If the shares menu doesn't exist, create it.
            //
            if (NULL != (m_hmenuShares = CreatePopupMenu()))
            {
                //
                // Insert the menu after the "View" menu.
                //
                CString strTitle(m_hInstance, IDS_MENU_SHARES);

                MENUITEMINFO mii;
                mii.cbSize     = sizeof(mii);
                mii.fMask      = MIIM_SUBMENU | MIIM_TYPE;
                mii.fType      = MFT_STRING;
                mii.dwTypeData = (LPTSTR)(strTitle.Cstr());
                mii.hSubMenu   = m_hmenuShares;

                g_MenuItemPos.Restore(iMENU_ITEM_SHARES);
                InsertMenuItem(m_hmenuMain, 
                               g_MenuItemPos.Position(iMENU_ITEM_SHARES), 
                               true, 
                               &mii);
            }
        }
    }
    else if (NULL != m_hmenuShares)
    {
        //
        // NOTE:  I tried using DeleteMenu to remove the menu and 
        //        free it's resources.  Unfortunately, it doesn't seem
        //        to free the resources like the docs say it should.
        //        At least BoundsChecker thinks it doesn't.
        //        Using RemoveMenu and DestroyMenu does the trick
        //        according to BoundsChecker.
        //
        int iPosShares = g_MenuItemPos.Position(iMENU_ITEM_SHARES);
        if (RemoveMenu(m_hmenuMain, iPosShares, MF_BYPOSITION) && NULL != m_hmenuShares)
        {
            DestroyMenu(m_hmenuShares);
        }
        m_hmenuShares = NULL;
        g_MenuItemPos.Remove(iMENU_ITEM_SHARES);
    }
    DrawMenuBar(m_hwndMain);

    return (NULL != m_hmenuShares);
}


//
// Empty the shares menu without destroying the menu.
//
void
CacheWindow::ClearSharesMenu(
    void
    ) throw()
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::ClearSharesMenu")));
    if (NULL != m_hmenuShares)
    {
        while(0 < GetMenuItemCount(m_hmenuShares))
            DeleteMenu(m_hmenuShares, 0, MF_BYPOSITION);
    }
}


//
// Empty the shares combo box.
//
void
CacheWindow::ClearSharesCombo(
    void
    ) throw()
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::ClearSharesCombo")));
    if (NULL != m_hwndCombo)
        ComboBox_ResetContent(m_hwndCombo);
}




//
// Update the window title to match the current view.
// Prerequisites:
//   1. Object tree is populated with share objects.
//
void
CacheWindow::UpdateWindowTitle(
    void
    )
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("CacheWindow::UpdateWindowTitle")));
    DBGASSERT((NULL != m_hwndMain));
    DBGASSERT((NULL != m_hInstance));

    CString strSubTitle;
    if (eShareView == m_eCurrentView)
    {
        strSubTitle.Format(m_hInstance, IDS_TITLE_NETSHARES);
    }
    else
    {
        CscShare *pShare = m_pCscObjTree->FindShare(UNCPath(m_strCurrentShare));
        if (NULL != pShare)
            pShare->GetLongDisplayName(&strSubTitle);
        else
            strSubTitle.Format(m_hInstance, IDS_ALL_SHARES);
    }

    //
    // Create the actual title: "Offline Folders - <subtitle>"
    //
    CString strTitle(m_hInstance, IDS_FMT_TITLE_CACHEVIEW_WINDOW, strSubTitle.Cstr());

    DBGPRINT((DM_VIEW, DL_LOW, TEXT("\ttitle = \"%s\""), strTitle.Cstr()));
    SetWindowText(m_hwndMain, strTitle);
}


//
// Called when the user changes the view type in the UI.
// Destroys the current view and creates a new one for the 
// view type selection.
//
void
CacheWindow::ViewSelectionChanged(
    ViewType eViewType,
    bool bUserSelectedViewItem
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::ViewSelectionChanged")));

    if (eShareView == m_eCurrentView && NULL != m_pView)
    {
        if (bUserSelectedViewItem)
        {
            //
            // Open the new view on the currently selected share.
            //
            CacheView::ItemIterator iter = m_pView->CreateItemIterator(true);
            CscObject *pObject;
            if (iter.Next(&pObject))
            {
                DBGASSERT((NULL != pObject));
                pObject->GetFullPath(&m_strCurrentShare);
            }
        }
        else
        {
            //
            // Open the new view on "all shares".
            //
            m_strCurrentShare.Empty();
        }
    }

    ChangeView(eViewType, m_strCurrentShare);
}




//
// Destroys the current view (after saving view state to the registry)
// then creates a new view object for the specified view type and
// share.
//
void
CacheWindow::ChangeView(
    ViewType eViewType,
    const CString& strShare,
    bool bConfigWindow,
    bool bBeginLoadingView
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::ChangeView")));
    DBGPRINT((DM_VIEW, DL_MID, TEXT("\teViewType = %d, share = \"%s\""), (int)eViewType, strShare.Cstr()));
    DBGPRINT((DM_VIEW, DL_MID, TEXT("\tbConfigWindow     = %d"), bConfigWindow));
    DBGPRINT((DM_VIEW, DL_MID, TEXT("\tbBeginLoadingView = %d"), bBeginLoadingView));


    //
    // Don't do anything if the specified view or share haven't changed.
    //
    if (m_eCurrentView != eViewType ||
        m_strCurrentShare != strShare)
    {
        //
        // Obtain the view lock so we don't try to change the view while
        // a view update is in progress.
        //
        AutoLockCs lock(m_ViewCs);
        //
        // Save view state info to the registry.
        // BUGBUG:  Might be more efficient to only save view state info
        //          if the view state has changed.  For instance, add
        //          a variable m_bViewIsDirty and only save view state
        //          if it is true.  Down side is that this would require
        //          additional code to detect view state changes.
        //
        if (NULL != m_pView)
            m_pView->SaveViewState();
        //
        // Destroy the current view.
        //
        delete m_pView;

        //
        // Update the "current view" and "current share" members then
        // direct the object tree to load the current share.  Note that
        // the share may be already loaded into the object tree.  The
        // tree handles that problem.
        //
        m_eCurrentView    = eViewType;
        m_strCurrentShare = strShare;
        if (eShareView == m_eCurrentView)
        {
            //
            // If we're switching to "share" view from "details" or "stale" 
            // view we want to suspend any loading that is in progress from
            // the previous view.  If the view was over a large share, 
            // we could end up loading several thousand files that the
            // user really doesn't want to see.  I don't want to keep eating 
            // up tree memory if the user has decided they only want to 
            // see shares.
            //
            m_pCscObjTree->PauseLoading();
        }
        else
        {
            //
            // Switching to "details" or "stale" view.
            //
            m_pCscObjTree->ResumeLoading();
            m_pCscObjTree->LoadShare(m_strCurrentShare);
        }
        //
        // Create the new view for the specified view type and share.
        // The new view will asynchronously populate the listview object
        // as time permits. 
        //
        m_pView = CreateView(eViewType, strShare);

        ShowItemCountInStatusbar();

        if (bConfigWindow)
            ConfigureWindowForView();

        //
        // Post the first CVM_GETNEXTOBJECT.  This will kick off view
        // population.
        //
        if (bBeginLoadingView)
            PostMessage(m_hwndMain, CVM_GETNEXTCSCOBJECT, 0, 0);
    }
}


//
// This is a notification from the progress bar object when the progress
// bar has reached 100%.  We clear the "Loading..." text from the statusbar.
//
void
CacheWindow::OnPBN_100Pct(
    void
    ) throw()
{
    SetStatusbarText(1, CString(""));
}



void
CacheWindow::InitProgressBarForView(
    int cUpdateIntervalMs
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::InitProgressBarForView")));

    if (eShareView == m_eCurrentView)
    {
        m_ProgressBar.Create(m_hwndProgressbar, m_hwndMain, cUpdateIntervalMs);
    }
    else
    {
        DBGASSERT((NULL != m_pView));
        if (m_strCurrentShare.IsEmpty())
        {
            //
            // Viewing all shares.  Progress bar source is the tree root.
            //
            m_ProgressBar.Create(m_hwndProgressbar, m_hwndMain, *m_pView, cUpdateIntervalMs, *m_pCscObjTree);
        }
        else
        {
            //
            // Viewing a specific share.  Progress bar source is that share.
            //
            CscShare *pShare = m_pCscObjTree->FindShare(UNCPath(m_strCurrentShare));
            DBGASSERT((NULL != pShare));
            m_ProgressBar.Create(m_hwndProgressbar, m_hwndMain, *m_pView, cUpdateIntervalMs, *pShare);
        }
        //
        // Set the "Loading..." string in the statusbar.
        //
        CString strStatus(m_hInstance, IDS_VIEWSTATUS_LOADING);
        SetStatusbarText(1, strStatus);
    }
    //
    // Kick off the progress bar updates.
    //
    m_ProgressBar.Reset();
}


//
// LVN_COLUMNCLICK notification handler.
// Called when a column header has been clicked - indicating the
// listview is to be sorted on that column's values.
//
LRESULT
CacheWindow::OnLVN_ColumnClick(
    NM_LISTVIEW *pnm
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::OnLVN_ColumnClick")));
    DBGASSERT((NULL != pnm));
    //
    // If new column selected, reset to ascending sort order.
    // If column selected more than once, toggle sort order.
    //
    m_bSortAscending = (pnm->iSubItem != m_iLastColSorted ? true : !m_bSortAscending);
    //
    // Sort the listview contents.
    // We pause loading temporarily so that sorting procedes as fast as possible
    // in cases where a load is in progress.
    //
    CAutoWaitCursor waitcursor;
    m_pCscObjTree->PauseLoading();
    m_pView->SortObjects(pnm->iSubItem, m_bSortAscending);
    m_pCscObjTree->ResumeLoading();
    //
    // Remember what column was selected as the sort key.
    //
    m_iLastColSorted = pnm->iSubItem;

    return 0;
}


//
// WM_COMMAND handler.
//
LRESULT
CacheWindow::OnCommand(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    WORD idCmd = LOWORD(wParam);
    switch(idCmd)
    {
        case IDM_FILE_OPEN:
            OpenSelectedItem();
            break;

        case IDM_FILE_CLOSE:
            DestroyWindow(m_hwndMain);
            break;

        case IDC_CACHEVIEW_SHARE_COMBO:
            if (CBN_SELENDOK == HIWORD(wParam))
            {
                ShareComboSelectionChanged();
            }
            break;

        case IDM_VIEW_SHARES:
            ViewSelectionChanged(eShareView, false);
            break;

        case IDM_VIEW_DETAILS:
            ViewSelectionChanged(eDetailsView, false);
            break;

        case IDM_VIEW_STALE:
            ViewSelectionChanged(eStaleView, false);
            break;

        case IDM_VIEW_REFRESH:
            Refresh();
            break;

        case IDM_VIEW_TOOLBAR:
            ShowToolbar(!IsToolbarVisible());
            break;

        case IDM_VIEW_STATUSBAR:
            ShowStatusbar(!IsStatusbarVisible());
            break;

        case IDM_EDIT_SELECTALL:
            SelectAllItems();
            break;

        case IDM_EDIT_INVERTSELECTION:
            InvertSelectedItems();
            break;

        case IDM_FILE_PIN:
        case IDM_FILE_UNPIN:
            PinSelectedItems(IDM_FILE_PIN == idCmd);
            break;

        case IDM_FILE_DELETE:
            DeleteSelectedItems();
            break;

        case IDM_FILE_UPDATE:
            UpdateSelectedItems();
            break;

        case IDM_FILE_SAVEAS:
            CopySelectedItemsToFolder();
            break;

        case IDM_SHOW_DEBUGINFO:
            ShowDebugInfo();
            break;

        case IDM_HELP_TOPICS:
            ShowHelp();
            break;

        default:
            if (IDM_SHARES_FIRST <= idCmd && IDM_SHARES_LAST >= idCmd)
            {
                //
                // User selected an item from the dynamic "Shares" menu.
                //
                ShareMenuSelectionChanged(idCmd);
            }

            break;
    }
    return 0;
}


typedef HWND (*PFNHTMLHELPA)(HWND, LPCSTR, UINT, DWORD);

void
CacheWindow::ShowHelp(
    void
    )
{
    if (NULL == m_hmodHtmlHelp)
        m_hmodHtmlHelp = LoadLibrary(TEXT("hhctrl.ocx"));

    if (NULL != m_hmodHtmlHelp)
    {
        PFNHTMLHELPA pfnHtmlHelpA = (PFNHTMLHELPA)GetProcAddress(m_hmodHtmlHelp,
                                                                 (LPCSTR)ATOM_HTMLHELP_API_ANSI);
        if (NULL != pfnHtmlHelpA)
        {
            (*pfnHtmlHelpA)(m_hwndMain, 
                            c_szHtmlHelpFile, 
                            HH_DISPLAY_TOPIC, 
                            (DWORD)c_szHtmlHelpTopic);
        }
    }
}


//
// Display a modal dialog showing detailed CSC database information
// for the selected item.  This is initiated by IDM_SHOW_DEBUGINFO.
//
void
CacheWindow::ShowDebugInfo(
    void
    ) const
{
    CString strSelectedItem;
    CacheView::ItemIterator iter = m_pView->CreateItemIterator(true);
    CscObject *pObject;
    if (iter.Next(&pObject))
    {
        DBGASSERT((NULL != pObject));
        pObject->GetFullPath(&strSelectedItem);
    }

    if (m_eCurrentView == eShareView)
    {
        ShareDbgDialog dlg(strSelectedItem, g_TheCnxNameCache);
        dlg.Run(m_hInstance, m_hwndMain);
    }
    else
    {
        FileDbgDialog dlg(strSelectedItem, pObject->IsFolder());
        dlg.Run(m_hInstance, m_hwndMain);
    }
}


bool
CacheWindow::CopySelectedItemsToFolder(
    void
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::CopySelectedItemsToFolder")));
    bool bResult = false;

    //
    // Ask user for the destination folder.
    //
    CPath strPathTo;
    if (BrowseForFolder(&strPathTo))
    {
        //
        // Build array of ptrs to objects to be copied.
        //
        CArray<CscObject *> rgpObj;
        if (PrepareCopyList(&rgpObj) && 0 < rgpObj.Count())
        {
            //
            // Start the progress dialog and copy the files.
            //
            int cObj = rgpObj.Count();
            CopyProgressDialog dlgCopyProgress;
            dlgCopyProgress.Run(m_hInstance, m_hwndMain, cObj);

            bool bYesToAll = false;
            for (int i = 0; i < cObj && !dlgCopyProgress.Cancelled(); i++)
            {
                CopySelectedObject(rgpObj[i], strPathTo, dlgCopyProgress, &bYesToAll);
            }
        }
    }
    return true;
}


//
// Get the destination folder name using the shell's standard 
// Browse-for-folder dialog.
//
bool
CacheWindow::BrowseForFolder(
    CPath *pstrFolder
    ) const
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::BrowseForFolder")));
    DBGASSERT((NULL != pstrFolder));
    bool bResult = false;
    BROWSEINFO bi;
    sh_autoptr<ITEMIDLIST> ptrIdl = NULL;
    BrowseForFolderCbkInfo bffci;

    bffci.pThis      = this;         // So callback can access CacheWindow members.
    bffci.pstrFolder = pstrFolder;   // So callback can set the folder name.
 
    ZeroMemory(&bi, sizeof(bi));
    CString strTitle(m_hInstance, IDS_BROWSEFORFOLDER);

    bi.hwndOwner      = m_hwndMain;
    bi.pidlRoot       = NULL;       // Start at desktop.
    bi.pszDisplayName = NULL;
    bi.lpszTitle      = strTitle.Cstr();
    bi.ulFlags        = BIF_EDITBOX | BIF_VALIDATE;
    bi.lpfn           = BrowseForFolderCallback;
    bi.lParam         = (LPARAM)&bffci;
    bi.iImage         = 0;

    bool bDone = false;
    while(!bDone)
    {
        pstrFolder->Empty();
        ptrIdl = SHBrowseForFolder(&bi);
        if (NULL == ptrIdl.get())
        {
            if (pstrFolder->IsEmpty())
            {
                bDone = true; // User cancelled SHBrowseForFolder dialog.
            }
            else
            {
                //
                // User entered name of valid but non-existant folder and
                // they want to create it.
                //
                if (CreateDirectoryPath(*pstrFolder))
                {
                    DBGPRINT((DM_VIEW, DL_LOW, TEXT("Created directory \"%s\""),
                             pstrFolder->Cstr()));
                    //
                    // Created the directory user entered in edit control.
                    // *pstrFolder contains the path string.  Life is good.
                    //
                    bDone = bResult = true;
                }
                else
                {
                    //
                    // Can't create the directory they entered.
                    // Display error to user.
                    // If they select "OK", re-open the SHBrowseForFolder dialog.
                    // If they select "Cancel", we're done.
                    //
                    DWORD dwError = GetLastError();
                    DBGERROR((TEXT("CreateDirectory error %d"), dwError));
                    switch(CscMessageBox(m_hwndMain,
                                         MB_OKCANCEL | MB_ICONWARNING,
                                         Win32Error(dwError),
                                         m_hInstance,
                                         IDS_FMT_CACHEVIEW_COPYERR_FOLDER,
                                         pstrFolder->Cstr()))
                    {
                        case IDCANCEL:
                            bDone = true;
                            //
                            // Fall through...
                            //
                        case IDOK:
                            break;
                    }
                }
            }
        }
        else
        {
            //
            // User entered or selected a valid folder that exists but it
            // may not be a valid copy destination.  For example, you can't
            // copy files to "My Computer" but the dialog still let's you select 
            // "My Computer".  
            //
            // BUGBUG:  This illustrates a problem with the browse-for-folder
            //          dialog.  If you set the BIF_RETURNONLYFSDIRS flag, the
            //          OK button is disabled when user selects something other
            //          than a filesystem folder.  However, this disabling prevents
            //          the user from clicking OK after they have entered a 
            //          path in the edit box (if BIF_EDITBOX is set) unless a 
            //          filesystem folder just happens to be selected in the tree
            //          view.  [brianau 4/27/98].
            //
            SHGetPathFromIDList(ptrIdl, pstrFolder->GetBuffer(MAX_PATH));
            pstrFolder->ReleaseBuffer();
            if (!pstrFolder->IsEmpty())
            {
                //
                // Got a valid copy destination.  
                // *pstrFolder contains the path string.  Life is good.
                //
                bResult = bDone = true;
            }
            else
            {
                //
                // Can't get a path from the IDL. Show the user the folder 
                // display name and let them know it's an invalid copy destination.
                // If they select "OK", we re-open the SHBrowseForFolder dialog.
                // If they select "Cancel", we're done.
                //
                GetShellItemDisplayName(ptrIdl, pstrFolder);
                switch(CscMessageBox(m_hwndMain,
                                     MB_OKCANCEL | MB_ICONWARNING,
                                     m_hInstance,
                                     IDS_ERR_COPYDESTINVALID,
                                     pstrFolder->Cstr()))
                {
                    case IDCANCEL:
                        bDone = true;
                        //
                        // Fall through...
                        //
                    case IDOK:
                        break;
                }
            }
        }
    }

    return bResult;
}


//
// Called to handle BFFM_VALIDATEFAILED in the SHBrowseForFolder callback.
// Returns:  0 = Close SHBrowseForFolder dialog.
//           1 = Leave open SHBrowseForFolder dialog.
//
int
CacheWindow::OnBrowseForFolderValidateFailed(
    HWND hwndParent,
    CPath *pstrFolder, 
    LPCTSTR pszPath
    ) const
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("CacheWindow::OnBrowseForFolderValidateFailed")));
    DBGASSERT((NULL != pstrFolder));
    DBGASSERT((NULL != pszPath));

    int iResult = 0;
    *pstrFolder = pszPath;
    pstrFolder->StripToRoot();
    if (pstrFolder->IsEmpty())
    {
        //
        // No root in path string so it's considered invalid.
        // If we allow a root-less path, the system will just use the
        // "current" directory and who knows what that is.
        //
        // If user selects "OK", we return to the SHBrowseForFolder dialog
        // and let them select/enter another path.
        // 
        // If user selects "Cancel", we close the SHBrowseForFolder dialog.
        // *pstrFolder is empty so BrowseForFolder will know the path
        // was invalid and not just non-existent.
        //
        DBGPRINT((DM_VIEW, DL_LOW, TEXT("\"%s\" is not a valid copy dest."), pszPath));
        switch(CscMessageBox(hwndParent,
                             MB_OKCANCEL | MB_ICONWARNING,
                             m_hInstance,
                             IDS_ERR_COPYDESTINVALID,
                             pszPath))
        {
            case IDOK:
                iResult = 1; // Don't close browse dialog.
                //
                // Fall through...
                //
            case IDCANCEL:
                break;
        }
    }
    else
    {
        //
        // Path is valid but doesn't exist.  
        //
        // If the user selects "Yes", we return and let BrowseForFolder
        // try to create the directory.  Set *pstrFolder to the 
        // path entered so that BrowseForFolder will know what the user 
        // entered.
        //
        // If the user selects "No", we return to the SHBrowseForFolder
        // dialog and let them enter/select another path.
        //
        DBGPRINT((DM_VIEW, DL_LOW, TEXT("\"%s\" is a non-existent copy dest."), pszPath));
        switch(CscMessageBox(hwndParent,
                             MB_YESNO | MB_ICONWARNING,
                             m_hInstance,
                             IDS_ERR_COPYDESTNOEXISTS,
                             pszPath))
        {
            case IDNO:
                iResult = 1; // Don't close browse dialog.
                pstrFolder->Empty();
                break;

            case IDYES:
                *pstrFolder = pszPath;
                break;
        }
    }
    return iResult;
}



//
// Callback called by SHBrowseForFolder.
//
int 
CacheWindow::BrowseForFolderCallback(
    HWND hwnd, 
    UINT uMsg, 
    LPARAM lParam, 
    LPARAM lpData
    ) throw()
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::BrowseForFolderCallback")));
    int iResult = 0;
    try
    {
        switch(uMsg)
        {
            case BFFM_INITIALIZED:
            {
                //
                // Set the title of the Browse-For-Folder dialog to better indicate
                // what the user is doing.  Title is "Copy To Folder".
                // Re-use the tooltip string resource.
                //
                DBGASSERT((NULL != lpData));
                BrowseForFolderCbkInfo *pbffci = (BrowseForFolderCbkInfo *)lpData;
                CString s(pbffci->pThis->m_hInstance, IDS_TT_FILE_SAVEAS);
                SetWindowText(hwnd, s);
                break;
            }

            case BFFM_VALIDATEFAILED:
            {
                //
                // User typed an invalid entry.  Either it is totally bogus
                // or it's a directory that just doesn't exist.
                //
                DBGASSERT((NULL != lpData));
                DBGASSERT((NULL != lParam));
                BrowseForFolderCbkInfo *pbffci = (BrowseForFolderCbkInfo *)lpData;
                iResult = pbffci->pThis->OnBrowseForFolderValidateFailed(hwnd,
                                                                         pbffci->pstrFolder,
                                                                         (LPCTSTR)lParam);
                break;
            }

            default:
                break;
        }
    }
    catch(CException& e)
    {
        DBGERROR((TEXT("C++ exception %d caught in CacheWindow::BrowseForFolderCallback"), e.dwError));
        CscWin32Message(NULL, e.dwError, CSCUI::SEV_ERROR);
    }
    catch(...)
    {
        DBGERROR((TEXT("Unknown exception caught in CacheWindow::BrowseForFolderCallback")));
        CscWin32Message(NULL, DISP_E_EXCEPTION, CSCUI::SEV_ERROR);
    }
    return iResult;
}


//
// Build an array of object pointers.  One entry for each item
// to be copied.  
//
bool
CacheWindow::PrepareCopyList(
    CArray<CscObject *> *prgpObj
    ) const
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::PrepareCopyList")));
    DBGASSERT((NULL != m_pView));
    DBGASSERT((NULL != prgpObj));

    //
    // Build array of selected obj ptrs from the listview.
    //
    int i              = 0;
    int cSelected      = m_pView->SelectedObjectCount();
    CscObject *pObject = NULL;
    CArray<CscObject *> rgpObjSel(cSelected);
    CacheView::ItemIterator iter = m_pView->CreateItemIterator(true);

    DBGPRINT((DM_VIEW, DL_LOW, TEXT("Building list of selected items...")));
    while(iter.Next(&pObject))
    {
        DBGASSERT((NULL != pObject));
        DBGASSERT((i < cSelected));
        rgpObjSel[i++] = pObject;
    }

    //
    // Scan array leaving only addresses of objects that are either
    // subtree roots or that don't have a parent included in the array.
    // Note that this has to be a M x N operation in order to ensure
    // we don't leave any redundancies.  IsDescenantOf() is a very
    // fast operation so this shouldn't be much of a perf hit.
    //
    DBGPRINT((DM_VIEW, DL_LOW, TEXT("Resolving redundancies and picking up children...")));
    for (i = 0; i < cSelected; i++)
    {
        CscObject* pObj1 = rgpObjSel[i];
        if (NULL != pObj1 && pObj1->IsParent())
        {
            for (int j = 0; j < cSelected; j++)
            {
                CscObject* pObj2 = rgpObjSel[j];
                if (NULL != pObj2 && pObj2->IsDescendantOf(pObj1))
                {
                    //
                    // Object is a descendant of another object
                    // in the array.  Replace it's ptr with NULL.
                    //
                    rgpObjSel[j] = NULL;
                }
            }
        }
    }

    //
    // Build a single array of object pointers for all objects in rgpObj[]
    // and all descendants of folders in rgpObj[].  Note that the caller
    // owns this array we're building.
    //
    DBGPRINT((DM_VIEW, DL_LOW, TEXT("Building final list of objects...")));
    prgpObj->Clear();
    for (i = 0; i < cSelected; i++)
    {
        pObject = rgpObjSel[i];
        if (NULL != pObject)
        {
            prgpObj->Append(pObject);
            if (pObject->IsFolder())
            {
                //
                // Selected folder.  Create  a tree iterator for 
                // the folder and add all of it's descendants.
                //
                CscObjTreeIterator iter(*pObject);
                CscObject *pSubObj = NULL;
                while(iter.Next(&pSubObj))
                {
                    DBGASSERT((NULL != pSubObj));
                    prgpObj->Append(pSubObj);
                }
            }
        }
    }
    return (0 < prgpObj->Count());
}


//
// Creates a directory of any depth.  If any intermediate directories
// don't exist, it will create them.
//
bool
CacheWindow::CreateDirectoryPath(
    const CPath& path
    )
{
    CPath root;            // What we'll build up as we go.
    CPath part;            // Each subdir as we walk path.
    CPath dirs(path);      // Only the directories.
    dirs.AddBackslash();
    dirs.GetRoot(&root);
    dirs.RemoveRoot();

    bool bCreate = false;  // Prevents unneeded calls to root.Exists().
    CPathIter iter(dirs);  // Used to walk the path.
    while(iter.Next(&part))
    {
        root.Append(part);
        if (bCreate || !root.Exists())
        {
            if (!CreateDirectory(root, NULL))
            {
                return false;
            }
            bCreate = true;
        }
    }
    return true;
}



//
// Copy a single object to a specified path.
//
void
CacheWindow::CopySelectedObject(
    const CscObject *pObj,
    const CString& strPathTo,
    CopyProgressDialog& dlgProgress,
    bool *pbYesToAll
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::CopySelectedObject")));
    DBGASSERT((NULL != pObj));
    DBGASSERT((NULL != pbYesToAll));

    DWORD dwError = ERROR_SUCCESS;
    //
    // Parent to use for any additional error UI.
    //
    HWND hwndParent = dlgProgress.GetWindow();
    if (NULL == hwndParent)
        hwndParent = m_hwndMain;

    //
    // Given:  Obj's path:  "\\worf\ntspecs\zaw\ui\foo.doc"
    //         strPathTo:   "c:\zaw\backup"
    // strTo will be:       "c:\zaw\backup\zaw\ui\foo.doc"
    //
    CString strFrom;
    pObj->GetFullPath(&strFrom);
    CPath strTo(strFrom);
    strTo.SetRoot(strPathTo);

    bool bIsFolder = pObj->IsFolder();
    dlgProgress.UpdateStatusText(strTo, bIsFolder);
    if (bIsFolder)
    {
        DBGPRINT((DM_VIEW, DL_LOW, TEXT("Creating directory \"%s\""), strTo.Cstr()));
        if (!CreateDirectoryPath(strTo))
        {
            //
            // We ignore "already exists" errors for directories.
            // All others we report to the user.
            //
            dwError = GetLastError();
            if (ERROR_ALREADY_EXISTS != dwError)
            {
                DBGERROR((TEXT("CreateDirectory error %d"), dwError));
                if (!ObjectCopyError(hwndParent, strPathTo, dwError, bIsFolder))
                    dlgProgress.Cancel();
            }
        }
    }
    else
    {
        //
        // Verify that the destination directory exists.  If it doesn't, try to
        // create it.
        //
        CPath strPathTo(strTo);
        strPathTo.RemoveFileSpec();
        if (!strPathTo.Exists())
        {
            if (!CreateDirectoryPath(strPathTo))
            {
                dwError = GetLastError();
                DBGERROR((TEXT("CreateDirectory error %d"), dwError));
                if (!ObjectCopyError(hwndParent, strPathTo, dwError, bIsFolder))
                    dlgProgress.Cancel();
                return;
            }
        }

        if (pObj->IsSparse())
        {
            dwError = CscFillSparseFiles(strFrom,
                                         false,     // not a full sync.
                                         (DWORD)this);
            if (ERROR_SUCCESS != dwError)
            {
                DBGERROR((TEXT("CSCFillSparseFiles failed with error %d"), dwError));
                if (!ObjectCopyError(hwndParent, strFrom, dwError, bIsFolder))
                    dlgProgress.Cancel();
                return;
            }
            pObj->ClearSparseness();
        }

        LPTSTR pszCscTemp = NULL;
        DBGPRINT((DM_VIEW, DL_LOW, TEXT("Copying file \"%s\""), strFrom.Cstr()));
        //
        // Have CSC create the temp copy of the offline file.  The temp file
        // has a unique name created by CSC.
        //
        if (CSCCopyReplica(strFrom, &pszCscTemp))
        {
            //
            // Try to move the temp file to the user's specified location,
            // renaming the file to the file's actual name.  If the first
            // move attempt fails due to an existing file, ask the user if 
            // they want to overwrite the exiting file.  If so, set the 
            // "overwrite" flag and repeat the move attempt.
            // 
            DWORD fMoveFlags = MOVEFILE_COPY_ALLOWED;
            dwError = ERROR_FILE_EXISTS;
            for (int iRetry = 0; 
                 (iRetry < 2) && (ERROR_FILE_EXISTS == dwError || ERROR_ALREADY_EXISTS == dwError); 
                 iRetry++)
            {
                dwError = ERROR_SUCCESS;
                DBGPRINT((DM_VIEW, DL_LOW, TEXT("Moving file \"%s\" to \"%s\".  Flags = 0x%08X"), pszCscTemp, strTo.Cstr(), fMoveFlags));
                if (!MoveFileEx(pszCscTemp, strTo, fMoveFlags))
                {
                    dwError = GetLastError();
                    DBGERROR((TEXT("MoveFileEx error %d"), dwError));
                    if (ERROR_FILE_EXISTS == dwError || ERROR_ALREADY_EXISTS == dwError)
                    {
                        if (*pbYesToAll || ConfirmReplaceOnCopy(hwndParent, strTo, pbYesToAll))
                        {
                            fMoveFlags |= MOVEFILE_REPLACE_EXISTING;
                        }
                        else
                            dwError = ERROR_SUCCESS; // Act like move succeeded.
                    }
                    else
                    {
                        if (!ObjectCopyError(hwndParent, strTo, dwError, bIsFolder))
                            dlgProgress.Cancel();
                    }
                }
            }

            //
            // Always try to delete the temp file created by CSCCopyReplica.  If we 
            // successfully moved it, there will be nothing to delete.
            //
            DeleteFile(pszCscTemp);
            LocalFree(pszCscTemp);
        }
        else
        {
            dwError = GetLastError();
            DBGERROR((TEXT("CSCCopyReplica failed with error %d"), dwError));
            if (!ObjectCopyError(hwndParent, strTo, dwError, bIsFolder))
                dlgProgress.Cancel();
        }
    }
    dlgProgress.Advance();
}

//
// Handle errors generated from file copy operations.
//
bool
CacheWindow::ObjectCopyError(
    HWND hwndParent,
    const CString& strName,
    DWORD dwWin32Err,
    bool bIsFolder
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::ObjectCopyError")));
    UINT idFmt = bIsFolder ? IDS_FMT_CACHEVIEW_COPYERR_FOLDER :
                             IDS_FMT_CACHEVIEW_COPYERR_FILE;

    return IDOK == CscMessageBox(hwndParent,
                          MB_OKCANCEL | MB_ICONWARNING,
                          Win32Error(dwWin32Err),
                          m_hInstance,
                          idFmt,
                          strName.Cstr());
}


//
// Ask user if they want to overwrite a file during the copy operation.
//
bool
CacheWindow::ConfirmReplaceOnCopy(
    HWND hwndParent,
    LPCTSTR pszTo,
    bool *pbYesToAll
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::ConfirmReplaceOnCopy")));
    DBGASSERT((NULL != pbYesToAll));
    DBGASSERT((NULL != pszTo));

    ConfirmCopyOverDialog dlg;
    int iResult = dlg.Run(m_hInstance, hwndParent, pszTo);

    *pbYesToAll = (IDYESTOALL == iResult);
    return !(IDNO == iResult);
}


//
// Open the selected item in read-only mode.
//
void
CacheWindow::OpenSelectedItem(
    void
    )
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("CacheWindow::OpenSelectedItems")));
    CacheView::ItemIterator iter(m_pView->CreateItemIterator(true));
    CscObject *pObject;

    DWORD dwError = ERROR_SUCCESS;
    CAutoWaitCursor waitcursor;
    if (iter.Next(&pObject))
    {
        //
        // Get the full path for the item.
        //
        CPath strFullPath;
        pObject->GetFullPath(&strFullPath);

        if (pObject->IsSparse())
        {
            dwError = CscFillSparseFiles(strFullPath,
                                         false,     // not a full sync.
                                         (DWORD)this);
            if (ERROR_SUCCESS != dwError)
            {
                DBGERROR((TEXT("CSCFillSparseFiles failed with error %d"), dwError));
                CscMessageBox(m_hwndMain,
                              MB_OK | MB_ICONERROR,
                              Win32Error(dwError),
                              m_hInstance,
                              IDS_FMT_CACHEVIEW_OPENERR,
                              strFullPath.Cstr());
                return;
            }
            pObject->ClearSparseness();
        }

        DBGPRINT((DM_VIEW, DL_LOW, TEXT("Opening \"%s\""), strFullPath.Cstr()));

        dwError = ::OpenOfflineFile(strFullPath);
        if (ERROR_SUCCESS != dwError)
        {
            //
            // Couldn't open the file for some reason.  Report error using
            // the original net path in the message text.
            //
            CscMessageBox(m_hwndMain,
                          MB_OK | MB_ICONWARNING,
                          Win32Error(dwError),
                          m_hInstance,
                          IDS_FMT_CACHEVIEW_OPENERR,
                          strFullPath.Cstr());
        }
    }
}


//
// If a user selects a share object (from share view) for delete
// or sync, AND that share hasn't been populated, we need to first
// populate the share in the object tree so that we have objects
// to refer to.  Remember that we normally don't populate shares
// until the user asks to view them in a Details view.  This lets
// us bring up the initial share view quickly and not pay for
// share population until we really need it.
//
void
CacheWindow::EnsureShareIsPopulated(
    CscShare *pShare
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::EnsureShareIsPopulated")));
    CArray<CscShare *> rgpShares;
    rgpShares.Append(pShare);
    EnsureSharesArePopulated(rgpShares);
}


//
// Same thing as EnsureShareIsPopulated but this one works for
// multiple shares.
//                                               
void
CacheWindow::EnsureSharesArePopulated(
    const CArray<CscShare *>& rgpShares
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::EnsureSharesArePopulated")));
    CArray<CString> rgstrShares;
    CString strShare;
    for (int i = 0; i < rgpShares.Count(); i++)
    {
        if (!rgpShares[i]->IsComplete())
        {
            rgpShares[i]->GetFullPath(&strShare);
            rgstrShares.Append(strShare);
        }
    }
    if (0 < rgstrShares.Count())
    {
        m_pCscObjTree->ResumeLoading();
        m_pCscObjTree->LoadShares(rgstrShares);
        int cComplete = 0;
        while(cComplete < rgpShares.Count())
        {
            cComplete = 0;
            for (int i = 0; i < rgpShares.Count(); i++)
            {
                if (rgpShares[i]->IsComplete())
                    cComplete++;
            }
            DBGPRINT((DM_VIEW, DL_LOW, TEXT("Shares not populated.  Sleeping...")));
            //
            // BUGBUG:  This isn't a great thing to do.  Could loop forever if
            //          we can never populate a share.  Should probably add a 
            //          safety valve and display UI if we can't populate all shares.
            //          
            Sleep(250);
        }
    }
}



//
// Call OneStop to update the selected items.
//
void
CacheWindow::UpdateSelectedItems(
    void
    )
{
    DBGPRINT((DM_VIEW, DL_MID, TEXT("CacheWindow::UpdateSelectedItems")));
    CAutoWaitCursor waitcursor;
    CacheView::ItemIterator iter(m_pView->CreateItemIterator(true));
    CArray<CscObject *> rgpObj;

    CscObject *pObject;
    while(iter.Next(&pObject))
    {
        DBGASSERT((NULL != pObject));
        if (pObject->IsShare())
            EnsureShareIsPopulated(static_cast<CscShare *>(pObject));

        rgpObj.Append(pObject);
    }
    BitSet bsetStale(rgpObj.Count());
    SynchronizeObjects(rgpObj, true, &bsetStale);
}


//
// Call SyncMgr (OneStop) to synchronize a set of objects in the viewer.
// Once the sync is complete, update the CSC info of each file and
// refresh the object in the view if necessary.
// bsetStale is a bitset indicating which of the objects
// in rgpObj is still stale after the update.
//
// Returns true if the CscUpdate was performed.
//
bool
CacheWindow::SynchronizeObjects(
    const CArray<CscObject *>& rgpObj,
    bool bSyncStaleOnly,
    BitSet *pbsetStale
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::SynchronizeObjects")));
    DBGASSERT((NULL != pbsetStale));

    bool bResult = false;
    CscFilenameList fnl;
    CString strPath;
    CscObject *pObject;
    pbsetStale->ClrAll();
    //
    // Build filename list for input to CscUpdateCache.
    //
    int cObjects = rgpObj.Count();
    for (int i = 0; i < cObjects; i++)
    {
        pObject = rgpObj[i];
        DBGASSERT((NULL != pObject));
        if (!bSyncStaleOnly || pObject->NeedToSync())
        {
            pObject->GetFullPath(&strPath);
            fnl.AddFile(strPath);
        }
    }

    if (0 < fnl.GetFileCount())
    {
        CscUpdateCache(CSC_UPDATE_STARTNOW | CSC_UPDATE_SELECTION | CSC_UPDATE_REINT | CSC_UPDATE_FILL_ALL,
                       m_hwndMain,
                       &fnl);

        //
        // Refresh the CSC information stored for each item in
        // the object tree.
        //
        for (int i = 0; i < cObjects; i++)
        {
            pObject = rgpObj[i];
            DBGASSERT((NULL != pObject));

            bool bNeededToSync = pObject->NeedToSync();
            pObject->UpdateCscInfo();
            bool bNeedToSync = pObject->NeedToSync();

            pbsetStale->SetBitState(i, bNeedToSync);
            //
            // Stale condition has changed.  Update the view item.
            //
            if (bNeedToSync != bNeededToSync)
                m_pView->ObjectChanged(pObject);
        }
        bResult = true;
    }

    return bResult;
}


//
// Display a message box asking the user for confirmation
// before deleting files/folders from the CSC cache.
//
bool
CacheWindow::ConfirmDeleteItems(
    void
    ) const
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::ConfirmDeleteItems")));
    MSGBOXPARAMS mbp;
    CString strText(m_hInstance, IDS_CONFIRMDELETE_TEXT);
    CString strCaption(m_hInstance, IDS_APPLICATION);

    ZeroMemory(&mbp, sizeof(mbp));

    mbp.cbSize          = sizeof(mbp);
    mbp.hwndOwner       = m_hwndMain;
    mbp.hInstance       = m_hInstance;
    mbp.lpszText        = strText;
    mbp.lpszCaption     = strCaption;
    mbp.dwStyle         = MB_OKCANCEL | MB_USERICON;
    mbp.lpszIcon        = MAKEINTRESOURCE(IDI_NUKE);

    return (IDOK == MessageBoxIndirect(&mbp));
}


//
// Delete the selected items from the CSC cache.
//
void
CacheWindow::DeleteSelectedItems(
    void
    )
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("CacheWindow::DeleteSelectedItems")));
    if (ConfirmDeleteItems())
    {
        CAutoWaitCursor waitcursor;
        m_pView->SetRedraw(false);
        try
        {
            DeleteSelectedItems2();
        }
        catch(...)
        {
            m_pView->SetRedraw(true);
            ::SetFocus(m_hwndMain);
            throw;
        }
        m_pView->SetRedraw(true);  // Reactivates redrawing.
        ::SetFocus(m_hwndMain);    // Highlights an item.
    }
}


//
// This function was broke out of DeleteSelectedItems strictly for efficiency
// of C++ EH.  
//
void
CacheWindow::DeleteSelectedItems2(
    void
    )
{
    CscShare *pCurrentShare = NULL;
    if (!m_strCurrentShare.IsEmpty())
    {
        pCurrentShare = m_pCscObjTree->FindShare(UNCPath(m_strCurrentShare));
        if (NULL != pCurrentShare)
        {
            pCurrentShare->LockAncestors();
            pCurrentShare->AddRef();
        }
    }

    //
    // A bit of a hack.
    // We're going to iterate all selected items but we need to delete
    // those items from the listview.  If we delete while we're iterating,
    // we upset the way the listview iterator works so that it only returns
    // part of the selected set of items.  We must first create a temporary
    // array of object pointers, store the selected pointers then perform
    // the actual deletions once the iteration is complete.
    //
    CArray<CscObject *>rgpObjTemp(m_pView->SelectedObjectCount());
    int i = 0;
    CacheView::ItemIterator iter(m_pView->CreateItemIterator(true));
    CscObject *pObject;
    while(iter.Next(&pObject))
    {
        DBGASSERT((NULL != pObject));
        rgpObjTemp[i++] = pObject;
        pObject->LockAncestors();
        pObject->AddRef();
    }

    //
    // Make sure that the selected shares are fully populated.
    // If we select a share in "share" view without yet having 
    // shown it in "details" view, it is not populated in the
    // object tree.  It must be populated in order for us to 
    // identify all of it's descendants and delete them.
    //
    int n = rgpObjTemp.Count();
    CArray<CscShare *> rgpShares;
    for (i = 0; i < n; i++)
    {
        if (rgpObjTemp[i]->IsShare())
            rgpShares.Append(static_cast<CscShare *>(rgpObjTemp[i]));
    }
    if (0 < rgpShares.Count())
        EnsureSharesArePopulated(rgpShares);

    //
    // Count up all of the objects that will be deleted so that
    // we can display an intelligent progress dialog.  The total
    // number is not completely accurate.  The selection set can
    // be a mixture of objects contained or not contained in 
    // a selected folder.  If an object is both selected directly
    // and selected indirectly (via inclusion in a folder) it will
    // be counted twice.  Trying to delete something that is already 
    // deleted is VERY fast so the only problem with this is that
    // the user may notice that a filename is displayed twice
    // in the progress dialog.  I think names will go by so fast users
    // won't notice.  Even if they do, it's just a progress
    // indicator.
    //
    int cObjectsTotal = 0;
    for (i = 0; i < n; i++)
    {
        CscObject *po = rgpObjTemp[i];
        cObjectsTotal += (1 + po->CountDescendants());
    }
    CProgressDialog dlgProgress;
    DBGPRINT((DM_VIEW, DL_MID, TEXT("Deleting %d total objects"), cObjectsTotal));
    dlgProgress.Run(m_hInstance, m_hwndMain, cObjectsTotal);

    CAutoWndEnable autoenable(m_hwndMain, false);
    dlgProgress.SetOperationText(CString(m_hInstance, IDS_VIEWSTATUS_DELETING));

    //
    // Get the name of the first object in the list so we can
    // flush shell notifications when we're done.
    //
    CString strObjFirst;
    if (0 < rgpObjTemp.Count())
        rgpObjTemp[0]->GetFullPath(&strObjFirst);

    //
    // Do the actual deletion of the objects.
    //
    for (i = 0; i < n && !dlgProgress.Cancelled(); i++)
    {
        CscObject *po = rgpObjTemp[i];
        DeleteObject(po, dlgProgress);
        po->ReleaseAncestors();
        po->Release();
    }
    ::ShellChangeNotify(strObjFirst, true);
    if (NULL != pCurrentShare)
    {
        if (0 == pCurrentShare->GetFileCount())
        {
            //
            // We just deleted all files and folders in the currently
            // viewed share.  Delete the share object.
            //
            DeleteShareObject(pCurrentShare, dlgProgress);
        }
        pCurrentShare->ReleaseAncestors();
        pCurrentShare->Release();
    }
}


//
// Deleting a share object requires a little more processing from the UI than
// for any other listview object.  Take care of it here.
//
HRESULT
CacheWindow::DeleteShareObject(
    CscShare *pShare,
    CProgressDialog& dlgProgress
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::DeleteShareObject")));
    DBGASSERT((NULL != pShare));

    CString strShare;
    pShare->GetFullPath(&strShare);
    //
    // Delete the share object from the CSC object tree.
    // Note that we DON'T delete it from the cache.  It's an error to 
    // call CSCDelete() on a share.
    //
    CscObjParent *pParent = static_cast<CscObjParent *>(pShare->GetParent());
    HRESULT hr = pParent->DestroyChild(pShare, 
                                       false    // Don't call CSCDelete()
                                       );
    if (SUCCEEDED(hr))
    {
        //
        // Now delete it's item from the listview and update
        // the rest of the UI.
        //
        m_pView->DeleteObject(pShare);
        ClearSharesCombo();
        ClearSharesMenu();

        if (!m_strCurrentShare.IsEmpty())
        {
            if (strShare == m_strCurrentShare)
            {
                //
                // If we just deleted the "current" share,
                // set the current share to "All Shares" and the current view to 
                // "share" view.
                //
                ChangeView(CacheWindow::eShareView, CString(TEXT("")));
            }
        }
    }
    else
    {
        DBGERROR((TEXT("Error 0x%08X deleting \"%s\" from CSC cache"), hr, strShare.Cstr()));
        CscMessageBox(dlgProgress.GetWindow() ? dlgProgress.GetWindow() : m_hwndMain,
                      MB_OK | MB_ICONERROR,
                      Win32Error(HRESULT_CODE(hr)),
                      m_hInstance,
                      IDS_FMT_ERR_DELFROMCACHE,
                      strShare.Cstr());
    }
    return hr;
}

   

//
// Delete a single object (folder or file) from the CSC cache.
// If the object is a folder, this function performs a post-order 
// traversal of the folder's subtree to delete each child.
//
HRESULT
CacheWindow::DeleteObject(
    CscObject *pObject,
    CProgressDialog& dlgProgress
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::DeleteObject")));

    DBGASSERT((NULL != pObject));
    DBGASSERT((NULL != m_pView));

    HRESULT hr = NOERROR;
    if (pObject->IsParent())
    {
        //
        // Iterate over parent's children.
        //
        CscObjParent *pParent = static_cast<CscObjParent *>(pObject);
        int iChild = 0;
        while(iChild < pParent->CountChildren() && !dlgProgress.Cancelled())
        {
            //
            // Call this function recursively.
            // If all goes well, we'll just keep deleting child 0.
            // We do it this way because as we delete children,
            // the parent's child array shrinks. An iterator wouldn't
            // work here.
            //
            hr = DeleteObject(pParent->GetChild(iChild), dlgProgress);
            if (FAILED(hr))
            {
                //
                // Can't delete this child.  Skip to the next so that
                // we're always deleting child 1, or 2, or 3... 
                // depending on how many child deletions have failed.
                // If we've hit this statement, we won't be able to
                // delete all of the parent's children.
                //
                iChild++;
            }
        }
    }
    //
    // This is the recursion exit point.  From here on down we're
    // handling the object passed in as "pObject".
    //
    if (pObject->IsShare())
    {
        //
        // This object is a share object.  If it doesn't have any
        // children, we can delete it from the tree.  
        //
        CscShare *pShare = static_cast<CscShare *>(pObject);
        if (0 == pShare->GetFileCount())
        {
            //
            // Deleting a share object requires some special handling 
            // with respect to the UI.  Therefore we have a separate 
            // function to handle it.
            //
            hr = DeleteShareObject(pShare, dlgProgress);
        }
    }
    else if (!dlgProgress.Cancelled())
    {
        //
        // This object is either a file or folder.
        // Delete the object from the CSC object tree AND
        // from the CSC database.
        //
        CString strObject;
        pObject->GetFullPath(&strObject);
//
//      I pulled this out because having the net paths for files being
//      deleted was confusing users.  Even though we displayed a message
//      saying that only local copies are being deleted, they see this net
//      path and freak out thinking we're deleting their files on the server.
//      The typical suggestion was to not display the individual file names
//      so we'll give this a try.  Note also that the "filename" static text
//      control in the dialog template has been made invisible.
//      [brianau - 7/14/98]
//
//        dlgProgress.SetFilenameText(strObject);
//
        DBGPRINT((DM_VIEW, DL_MID, TEXT("Deleting \"%s\" from CSC cache"), strObject.Cstr()));
        CscObjParent *pParent = static_cast<CscObjParent *>(pObject->GetParent());
        hr = pParent->DestroyChild(pObject);
        if (SUCCEEDED(hr))
        {
            //
            // Remove the object from the view's listview.
            //
            m_pView->DeleteObject(pObject);
            ::ShellChangeNotify(strObject, false);
        }
        else
        {
            DBGERROR((TEXT("Error 0x%08X deleting \"%s\" from CSC cache"), hr, strObject.Cstr()));
            CscMessageBox(dlgProgress.GetWindow() ? dlgProgress.GetWindow() : m_hwndMain, 
                          MB_OK | MB_ICONERROR,
                          Win32Error(HRESULT_CODE(hr)),
                          m_hInstance,
                          IDS_FMT_ERR_DELFROMCACHE,
                          strObject.Cstr());
        }
        dlgProgress.Advance();
    }
    return hr;
}


//
// Pin or Unpin selected view items.
// Note that this only pins/unpins items selected in the listview.
// Unlike DeleteSelectedItems(), it will not affect children of a selected
// folder unless those children are explicitly selected in the view.
//
void 
CacheWindow::PinSelectedItems(
    bool bPin
    )
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("CacheWindow::PinSelectedItems")));
    DBGPRINT((DM_VIEW, DL_HIGH, TEXT("\tbPin = %d"), bPin));

    CscFilenameList fnl;
    CString strPath;
    CacheView::ItemIterator iter = m_pView->CreateItemIterator(true);
    CscObject *pObject;
    CAutoWaitCursor waitcursor;
    while(iter.Next(&pObject))
    {
        DBGASSERT((NULL != pObject));

        if (bPin != pObject->IsPinned())
        {
            pObject->GetFullPath(&strPath);
            HRESULT hr = pObject->Pin(bPin);
            if (SUCCEEDED(hr))
            {
                if (bPin)
                {
                    //
                    // Only add files to the filename list if we're pinning.
                    // That's the only time we're going to send information
                    // to OneStop for update.
                    //
                    fnl.AddFile(strPath);
                }
                m_pView->ObjectChanged(pObject);
                ::ShellChangeNotify(strPath, false);
            }
            else
            {
                DBGERROR((TEXT("Error 0x%08X %spinning \"%s\""),
                          hr, bPin ? TEXT("") : TEXT("un"),
                          strPath.Cstr()));

                CscMessageBox(m_hwndMain, 
                              MB_OK | MB_ICONERROR,
                              Win32Error(HRESULT_CODE(hr)),
                              m_hInstance,
                              bPin ? IDS_FMT_ERR_PIN : IDS_FMT_ERR_UNPIN,
                              strPath.Cstr());
            }
        }
    }
    ::ShellChangeNotify(strPath, true);  // Flush all shell notifications.

    if (0 < fnl.GetFileCount())
    {
        //
        // Send the file/folder name list to OneStop for update so that the
        // user sees some progress UI and so that the pinned files are 
        // updated onto the local machine.
        //
        CscUpdateCache(CSC_UPDATE_STARTNOW | CSC_UPDATE_SELECTION | CSC_UPDATE_FILL_QUICK,
                       NULL,
                       &fnl);
    }
}



//
// Tell the view to select all items.
//
void
CacheWindow::SelectAllItems(
    void
    ) throw()
{
    DBGASSERT((NULL != m_pView));

    CAutoWaitCursor waitcursor;
    m_bStatusbarDisabled = true;
    m_pView->SelectAllItems();
    m_bStatusbarDisabled = false;
    ShowItemCountInStatusbar();
}

//
// Tell the view to invert the current listview selection.
//
void
CacheWindow::InvertSelectedItems(
    void
    ) throw()
{
    DBGASSERT((NULL != m_pView));

    CAutoWaitCursor waitcursor;
    m_bStatusbarDisabled = true;
    m_pView->InvertSelectedItems();
    m_bStatusbarDisabled = false;
    ShowItemCountInStatusbar();
}


//
// Save the state of the window to the registry on a per-user basis.
// Saved info is:
//
// Window size, statusbar visibility, toolbar visibility.
//
bool 
CacheWindow::SaveWindowState(
    void
    ) const
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::SaveWindowState")));
    DBGASSERT((NULL != m_hwndMain));

    bool bResult = false;
    StateInfo si;
    //
    // Write a struct size and version number to the reg value.
    // This helps us invalidate existing reg values if necessary.
    //
    si.cbSize    = sizeof(si);
    si.dwVersion = WINDOW_STATE_VERSION;
    //
    // Collect main window size.
    //
    RECT rc;
    GetWindowRect(m_hwndMain, &rc);
    si.cxWindow = rc.right - rc.left;
    si.cyWindow = rc.bottom - rc.top;
    si.bMaximized = boolify(IsZoomed(m_hwndMain));
    //
    // Collect toolbar and statusbar visibility information.
    //
    si.bToolbar   = IsToolbarVisible();
    si.bStatusbar = IsStatusbarVisible();
    //
    // Open the reg key and write the data.
    //
    RegKey key(HKEY_CURRENT_USER, m_strRegKey);
    if (SUCCEEDED(key.Open(KEY_WRITE, true)))
    {
        bResult = SUCCEEDED(key.SetValue(REGSTR_VAL_WINDOWSTATE_MAIN, (LPBYTE)&si, sizeof(si)));
    }
    return bResult;
}



//
// Restore the window's state from the per-user registry info.  The data
// was saved to the registry by SaveWindowState.
//
bool 
CacheWindow::RestoreWindowState(
    int *pnCmdShow
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheWindow::RestoreWindowState")));
    DBGASSERT((NULL != m_hwndMain));
    DBGASSERT((NULL != pnCmdShow));

    bool bResult = false;
    StateInfo si;

    *pnCmdShow = SW_SHOWNORMAL;
    RegKey key(HKEY_CURRENT_USER, m_strRegKey);
    if (SUCCEEDED(key.Open(KEY_READ, true)))
    {
        if (SUCCEEDED(key.GetValue(CString(TEXT("0")), (LPBYTE)&si, sizeof(si))))
        {
            if (sizeof(si) != si.cbSize || WINDOW_STATE_VERSION != si.dwVersion)
            {
                DBGERROR((TEXT("Invalid window state data in registry.")));
            }
            else
            {
                //
                // Only restore window size if reg data was a valid version
                // and a valid size.
                //
                HDC hdc = GetDC(m_hwndMain);
                int cxWindow = GetDeviceCaps(hdc, HORZRES);
                int cyWindow = GetDeviceCaps(hdc, VERTRES);
                ReleaseDC(m_hwndMain, hdc);
                if (si.bMaximized)
                {
                    *pnCmdShow = SW_SHOWMAXIMIZED;
                }
                else
                {
                    cxWindow = MIN(cxWindow, si.cxWindow);
                    cyWindow = MIN(cyWindow, si.cyWindow);
                }
                //
                // Restore the window size.
                // Limit to screen size in case screen resolution has changed
                // or someone manually put bogus data into the registry.
                //
                SetWindowPos(m_hwndMain,
                             NULL,
                             0, 0,
                             cxWindow,
                             cyWindow,
                             SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW);
          
                bResult = true;
            }
        }
    }

    //
    // Show both the toolbar and statusbar unless saved state
    // says otherwise.
    //
    ShowToolbar(!bResult || si.bToolbar);
    ShowStatusbar(!bResult || si.bStatusbar);

    return bResult;
}


//
// Determine if the statusbar is visible.
//
bool
CacheWindow::IsStatusbarVisible(
    void
    ) const throw()
{
    return (NULL != m_hwndStatusbar && m_bStatusbarVisible);
}


//
// Determine if the toolbar is visible.
//
bool
CacheWindow::IsToolbarVisible(
    void
    ) const throw()
{
    return (NULL != m_hwndToolbar && m_bToolbarVisible);
}


//
// Call OnSize() to update the layout of the main window and it's children.
//
void
CacheWindow::UpdateWindowLayout(
    void
    )
{
    //
    // Window layout is all done in WM_SIZE handler - OnSize().
    //
    RECT rc;
    GetWindowRect(m_hwndMain, &rc);
    OnSize(m_hwndMain, WM_SIZE, SIZE_RESTORED, MAKELONG(rc.right-rc.left,rc.bottom-rc.top));
}


//
// Show/hide the toolbar.
//
void
CacheWindow::ShowToolbar(
    bool bShow
    )
{
    DBGASSERT((NULL != m_hmenuMain));
    DBGASSERT((NULL != m_hwndToolbar));

    m_bToolbarVisible = bShow;

    MENUITEMINFO mii;
    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_STATE;
    mii.fState = bShow ? MFS_CHECKED : MFS_UNCHECKED;

    SetMenuItemInfo(m_hmenuMain, IDM_VIEW_TOOLBAR, false, &mii);
    ShowWindow(m_hwndToolbar, bShow ? SW_SHOW : SW_HIDE);
    ShowShareCombo(eShareView != m_eCurrentView);
    UpdateWindowLayout();
}


//
// Snow/hide the statusbar.
//
void
CacheWindow::ShowStatusbar(
    bool bShow
    )
{
    DBGASSERT((NULL != m_hmenuMain));
    DBGASSERT((NULL != m_hwndStatusbar));

    m_bStatusbarVisible = bShow;

    MENUITEMINFO mii;
    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_STATE;
    mii.fState = bShow ? MFS_CHECKED : MFS_UNCHECKED;

    SetMenuItemInfo(m_hmenuMain, IDM_VIEW_STATUSBAR, false, &mii);
    ShowWindow(m_hwndStatusbar, bShow ? SW_SHOW : SW_HIDE);
    ShowItemCountInStatusbar();
    UpdateWindowLayout();
}


//
// Provide tooltip text for toolbar buttons.
//
LRESULT
CacheWindow::OnTTN_NeedText(
    TOOLTIPTEXT *pToolTipText
    )
{
    DBGASSERT((NULL != pToolTipText));


    //
    // Cross-reference tool command IDs with tooltip text IDs.
    //
    static const struct 
    {
        UINT idCmd;  // Tool cmd ID.
        UINT idTT;   // Tooltip text ID.

    } CmdTTXRef[] = {
                        { IDM_FILE_OPEN,      IDS_TT_FILE_OPEN       },
                        { IDM_FILE_SAVEAS,    IDS_TT_FILE_SAVEAS     },
                        { IDM_FILE_DELETE,    IDS_TT_FILE_DELETE     },
                        { IDM_FILE_PIN,       IDS_TT_FILE_PIN        },
                        { IDM_FILE_UNPIN,     IDS_TT_FILE_UNPIN      },
                        { IDM_FILE_UPDATE,    IDS_TT_FILE_UPDATE     },
                        { IDM_VIEW_SHARES,    IDS_TT_VIEW_SHARES     },
                        { IDM_VIEW_DETAILS,   IDS_TT_VIEW_DETAILS    },
                        { IDM_VIEW_STALE,     IDS_TT_VIEW_STALE      }
                    };

    for (INT i = 0; i < ARRAYSIZE(CmdTTXRef); i++)
    {
        if (CmdTTXRef[i].idCmd == pToolTipText->hdr.idFrom)
        {
            m_strToolTip.Format(m_hInstance, CmdTTXRef[i].idTT);
            pToolTipText->lpszText = (LPTSTR)m_strToolTip;
            break;
        }
    }

    return 0;
}



//
// LVN_ITEMCHANGED handler.
//
LRESULT
CacheWindow::OnLVN_ItemChanged(
    NM_LISTVIEW *pnmlv
    )
{
    if (LVIS_FOCUSED & pnmlv->uNewState)
    {
        ShowItemCountInStatusbar();
        UpdateButtonsAndMenuItems();
    }
    return 0;
}    


//
// WM_SETFOCUS handler.
//
LRESULT
CacheWindow::OnSetFocus(
    HWND,
    UINT,
    WPARAM,
    LPARAM
    ) throw()
{
    //
    // Always forward keyboard focus to the listview control.
    //
    if (NULL != m_pView)
        m_pView->SetFocus();
    return 0;
}


//
// WM_MENUSELECT handler.
//
LRESULT
CacheWindow::OnMenuSelect(
    HWND,
    UINT,
    WPARAM wParam,
    LPARAM lParam
    )
{
    m_bMenuActive = (0xFFFF != HIWORD(wParam) || NULL != (HMENU)lParam);

    if (m_bMenuActive)
        ShowMenuTextInStatusbar(LOWORD(wParam));
    else
        ShowItemCountInStatusbar();

    return 0;
}


//
// Refresh the view from the CSC object tree.  This re-loads the
// CSC object tree from the CSC database as well as rebuilding
// the view.
//
LRESULT
CacheWindow::Refresh(
    void
    )
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("CacheView::Refresh")));

    CAutoWaitCursor waitcursor;
    //
    // Order of execution is important here.
    //
    m_pView->DestroyIconHound();  // Destroy icon hound thread.
    m_bViewPopulated = true;      // Suspend view item updates.
    m_pCscObjTree->Refresh();     // Reload csc object tree.
    g_TheCnxNameCache.Refresh();  // Reload the net cnx name cache.
    //
    // Save the current view and share so they can be passed to 
    // ChangeView.
    //
    ViewType eView   = m_eCurrentView;
    CString strShare = m_strCurrentShare;

    if (!m_strCurrentShare.IsEmpty() && (!m_pCscObjTree->ShareExists(UNCPath(m_strCurrentShare))))
    {
        //
        // What was the "current share" no longer exists in the cache.
        // This can happen if someone has unpinned all the share's contents
        // from explorer which deletes what's unpinned.  If all the share's
        // contents were deleted, the share no longer exists.  We can't
        // refresh to that share.  Display a message box explaining the issue
        // and default back to share view.
        //
        CscMessageBox(m_hwndMain,
                      MB_OK | MB_ICONINFORMATION,
                      m_hInstance,
                      IDS_FMT_SHAREWASDELETED,
                      m_strCurrentShare.Cstr());

        strShare.Empty();
    }
    //
    // Seed the current view and share members with invalid data so
    // ChangeView does the right thing.
    //
    m_eCurrentView    = eUnknownView;
    m_strCurrentShare = TEXT("?");

    ChangeView(eView, strShare);
    return 0;
}

//
// Retrieve the bounding rectangle of the first selected item in the listview and
// convert it to screen coordinates.
//
int
CacheWindow::GetFirstSelectedItemRect(
    RECT *prc
    )
{
    HWND hwndLV = m_pView->GetViewWindow();
    int iSel = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);
    if (-1 != iSel)
    {
        if (ListView_GetItemRect(hwndLV, iSel, prc, LVIR_SELECTBOUNDS))
        {
            ClientToScreen(hwndLV, (LPPOINT)&prc->left);
            ClientToScreen(hwndLV, (LPPOINT)&prc->right);
            return iSel;
        }
    }
    return iSel;
}



//
// WM_CONTEXTMENU handler
//
LPARAM
CacheWindow::OnContextMenu(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    if (NULL != m_pView && 
        (HWND)wParam == m_pView->GetViewWindow() &&
        0 < m_pView->SelectedObjectCount())
    {
        //
        // Only display menu if the message is from the list view and there's
        // one or more objects selected in the list view.
        //
        {
            HMENU hMenu = LoadMenu(m_hInstance, MAKEINTRESOURCE(IDR_CACHEVIEW_CONTEXTMENU));
            if (NULL != hMenu)
            {
                //
                // PBMF means "pointer to boolean member function.
                //
                typedef bool (CacheView::*PBMF)(void) const;

                static struct
                {
                    int  idCmd;  // Menu command ID.
                    PBMF pfn;    // Boolean function to determine view capability.

                } rgCmdAndFunc[] = {{ IDM_FILE_DELETE, m_pView->CanDelete },
                                    { IDM_FILE_PIN,    m_pView->CanPin    },
                                    { IDM_FILE_UNPIN,  m_pView->CanUnpin  },
                                    { IDM_FILE_SAVEAS, m_pView->CanSaveAs },
                                    { IDM_FILE_UPDATE, m_pView->CanUpdate },
                                    { IDM_FILE_OPEN,   m_pView->CanOpen   }
                                   };

                static const int rgmf[] = {MF_GRAYED, MF_ENABLED};

                HMENU hMenuTrackPopup = GetSubMenu(hMenu, 0);

                int xPos = LOWORD(lParam);
                int yPos = HIWORD(lParam);

                if (-1 == lParam)
                {
                    //
                    // Adjust menu position if invoked via Shift F10.
                    // We won't have valid mouse coordinates.
                    //
                    RECT rc;
                    if (-1 != GetFirstSelectedItemRect(&rc))
                    {
                        xPos = rc.left + ((rc.right - rc.left) / 2);
                        yPos = rc.top + ((rc.bottom - rc.top) / 2);
                    }
                }

                for (int i = 0; i < ARRAYSIZE(rgCmdAndFunc); i++)
                {
                    int idCmd = rgCmdAndFunc[i].idCmd;
                    PBMF pfn  = rgCmdAndFunc[i].pfn;
                    EnableMenuItem(hMenuTrackPopup, idCmd, rgmf[(m_pView->*pfn)()]);
                }

                //
                // Apply policy restrictions to popup menu.
                //
                ApplyPolicyToFileMenu(hMenuTrackPopup);

                //
                // Hide the "DebugInfo" item if the "ShowDebugInfo" registry value
                // is missing (or defined as 0) or if the user has selected more
                // than one item in the listview.
                //
                if (!Viewer::m_bShowDebugInfo || 1 < m_pView->SelectedObjectCount())
                    DeleteMenu(hMenuTrackPopup, IDM_SHOW_DEBUGINFO, MF_BYCOMMAND);

                TrackPopupMenu(hMenuTrackPopup,
                               TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                               xPos,
                               yPos,
                               0,
                               hWnd,
                               NULL);

                DestroyMenu(hMenu);
            }
        }
    }
    return 0;
}


//-----------------------------------------------------------------------------
// class CacheWindow::ShareDlg
//-----------------------------------------------------------------------------

CacheWindow::ShareDlg::ShareDlg(
    HINSTANCE hInstance,
    HWND hwndParent,
    const CArray<CscShare *>& rgpShares,
    CString *pstrShare,
    CString *pstrDisplayName
    ) : m_hInstance(hInstance),
        m_hwndParent(hwndParent),
        m_hwndList(NULL),
        m_pstrShare(pstrShare),
        m_pstrDisplayName(pstrDisplayName),
        m_rgpShares(rgpShares)
{
    DBGASSERT((NULL != m_hInstance));
    DBGASSERT((NULL != m_hwndParent));
}


//
// Client's instantiate a ShareDlg object and call Run().
// Returns: 
//      true  = User made a selection.
//              Share name is stored in *pstrShareName.
//              Share display name is stored in *pstrDisplayName.
//
//      false = User cancelled.
//
//
bool
CacheWindow::ShareDlg::Run(
    void
    ) throw()
{
    return boolify(DialogBoxParam(m_hInstance, 
                            MAKEINTRESOURCE(IDD_CACHEVIEW_SHARES), 
                            m_hwndParent, 
                            (DLGPROC)DlgProc,
                            (LPARAM)this));
}

//
// WM_INITDIALOG handler.
//
BOOL
CacheWindow::ShareDlg::OnInitDialog(
    HWND hwnd
    )
{
    CString strDisplayName;
    m_hwndList = GetDlgItem(hwnd, IDC_LIST_SHARES);
    DBGASSERT((NULL != m_hwndList));

    //
    // Save "this" pointer in dialog's userdata.
    //
    SetWindowLongPtr(hwnd, DWLP_USER, (INT_PTR)this);

    //
    // Center the dialog in it's parent.
    //
    RECT rcParent;
    RECT rcDlg;
    GetWindowRect(m_hwndParent, &rcParent);
    GetWindowRect(hwnd, &rcDlg);

    int cxParent = rcParent.right - rcDlg.left;
    int cyParent = rcParent.bottom - rcParent.top;
    int cxDlg = rcDlg.right - rcDlg.left;
    int cyDlg = rcDlg.bottom - rcDlg.top;

    MoveWindow(hwnd,
               rcParent.left + ((cxParent - cxDlg) / 2),
               rcParent.top  + ((cyParent - cyDlg) / 2),
               cxDlg, cyDlg, false);

    //
    // Fill the dialog's list of shares.
    //
    int cShares = m_rgpShares.Count();

    for (int i = 0; i < cShares + 1; i++)
    {
        if (0 == i)
        {
            strDisplayName.Format(m_hInstance, IDS_ALL_SHARES);
        }
        else
        {
            CscShare *pShare = m_rgpShares[i-1];
            pShare->GetLongDisplayName(&strDisplayName);
        }

        SendMessage(m_hwndList, LB_ADDSTRING, 0, (LPARAM)(strDisplayName.Cstr()));
    }
    SendMessage(m_hwndList, LB_SETCURSEL, 0, 0);

    return TRUE;
}

//
// WM_COMMAND(IDOK) handler.
//
void
CacheWindow::ShareDlg::OnOk(
    void
    )
{                        
    int i = SendMessage(m_hwndList, LB_GETCURSEL, 0, 0);
    if (LB_ERR != i)
    {
        if (0 == i)
        {
            m_pstrShare->Empty();
        }
        else if (NULL != m_pstrShare)
        {
            m_rgpShares[i-1]->GetName(m_pstrShare);
        }

        if (NULL != m_pstrDisplayName)
        {
            LPTSTR pszDisplayName = m_pstrDisplayName->GetBuffer(SendMessage(m_hwndList, LB_GETTEXTLEN, i, 0) + 1);
            SendMessage(m_hwndList, LB_GETTEXT, i, (LPARAM)pszDisplayName);
            m_pstrDisplayName->ReleaseBuffer();
        }
    }
}

//
// Share selection dialog proc.
//
BOOL CALLBACK
CacheWindow::ShareDlg::DlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    try
    {
        //
        // Having only a function call within the try block is
        // more efficient from an EH standpoint than having the entire
        // DlgProc within the try block.
        //
        return DlgProcInternal(hwnd, message, wParam, lParam);
    }
    catch(CException& e)
    {
        DBGERROR((TEXT("C++ exception %d caught in ShareDlg::DlgProc"), e.dwError));
        CscWin32Message(NULL, e.dwError, CSCUI::SEV_ERROR);
    }
    catch(...)
    {
        DBGERROR((TEXT("Unknown exception caught in ShareDlg::DlgProc")));
        CscWin32Message(NULL, DISP_E_EXCEPTION, CSCUI::SEV_ERROR);
    }
}


BOOL CALLBACK
CacheWindow::ShareDlg::DlgProcInternal(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    ShareDlg *pThis = NULL;
    switch(message)
    {
        case WM_INITDIALOG:
            pThis = reinterpret_cast<ShareDlg *>(lParam);
            SetWindowLongPtr(hwnd, DWLP_USER, (INT_PTR)lParam);
            return pThis->OnInitDialog(hwnd);

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_LIST_SHARES:
                    if (HIWORD(wParam) != LBN_DBLCLK)
                        break;
                    //
                    // Double-click in list is same as pressing OK button.
                    //
                case IDOK:
                    pThis = reinterpret_cast<ShareDlg *>(GetWindowLongPtr(hwnd, DWLP_USER));
                    DBGASSERT((NULL != pThis));
                    pThis->OnOk();
                    EndDialog(hwnd, TRUE);
                    break;

                case IDCANCEL:
                    EndDialog(hwnd, FALSE);
                    break;

                default:
                    break;
            }
            break;

        case WM_CLOSE:
            EndDialog(hwnd, FALSE);
            break;

        case WM_SETFOCUS:
            //
            // Always forward focus to the list.
            //
            pThis = reinterpret_cast<ShareDlg *>(GetWindowLongPtr(hwnd, DWLP_USER));
            DBGASSERT((NULL != pThis));
            SetFocus(pThis->m_hwndList);
            break;
    }
    return FALSE;
}
    


//-----------------------------------------------------------------------------
// class CacheWindow::TBSubclass
//-----------------------------------------------------------------------------

LRESULT
CacheWindow::TBSubclass::HandleMessages(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    POINT pt;
    switch(message)
    {
        case WM_TIMER:
            //
            // Use a timer to detect when the mouse has left the toolbar.
            // 
            if (ID_TIMER_TBHITTEST == wParam)
            {
                GetCursorPos(&pt);
                ScreenToClient(hwnd, &pt);
                ButtonHitTest(hwnd, pt, false);
            }
            break;

        case WM_MOUSEMOVE:
             pt.x = LOWORD(lParam);
             pt.y = HIWORD(lParam);
             ButtonHitTest(hwnd, pt, true);
            break;
    };
    return 0;
}


//
// Perform a hit test on the toolbar to see if the mouse cursor is over
// a button.  If it is, display tool help in the statusbar.  If it isn't,
// display the normal listview item count.  Since we don't get mouse
// messages when the cursor leaves the toolbar, I use a timer to detect
// when the mouse has left the toolbar rect.
//
int
CacheWindow::TBSubclass::ButtonHitTest(
    HWND hwnd,
    const POINT& pt,
    bool bSetTimer
    )
{
    DBGASSERT((NULL != hwnd));
    int iBtnHit = -1;

    KillTimer(hwnd, ID_TIMER_TBHITTEST);
    if (IsWindowVisible(hwnd))
    {
        iBtnHit = ToolBar_HitTest(hwnd, &pt);
        if (iBtnHit != m_iLastBtnHit)
        {
            int cButtons = ToolBar_ButtonCount(hwnd);
            if (0 <= iBtnHit && iBtnHit < cButtons)
                m_pCacheWindow->ShowToolbarButtonTextInStatusbar(iBtnHit);
            else 
                m_pCacheWindow->ShowItemCountInStatusbar();

            m_iLastBtnHit = iBtnHit;
        }
        if (bSetTimer)
        {
            SetTimer(hwnd, ID_TIMER_TBHITTEST, 200, NULL);
        }
    }
    return iBtnHit;
}



//-----------------------------------------------------------------------------
// class CacheView
//-----------------------------------------------------------------------------

//
// The view's imagelist is static and only created once.
//
HIMAGELIST CacheView::m_himl;
CacheView::IconHound *CacheView::m_pIconHound;

CacheView::CacheView(
    HINSTANCE hInstance, 
    HWND hwndParent, 
    const CscObjTree& tree,
    const CString& strShare
    ) : m_hInstance(hInstance),
        m_hwndParent(hwndParent),
        m_hwndLV(NULL),
        m_hwndHeader(NULL),
        m_strShare(strShare),
        m_strRegKey(REGSTR_KEY_VIEWSTATEINFO),
        m_CscObjTree(tree),
        m_pCscObjIter(NULL),
        m_cLoadBuffer(0),
        m_cObjConsidered(0)
{ 
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("CacheView::CacheView")));

    DBGASSERT((NULL != hInstance));
    DBGASSERT((NULL != hwndParent));
}


CacheView::~CacheView(
    void
    )
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("CacheView::~CacheView")));

    if (NULL != m_hwndLV)
    {
        //
        // Clear the listview control and destroy it.
        //
        CacheView::ItemIterator iter = CreateItemIterator(false);
        CscObject *pObject;
        while(iter.Next(&pObject))
        {
            pObject->ReleaseAncestors();
            pObject->Release();
        }
        ListView_DeleteAllItems(m_hwndLV);
        DestroyWindow(m_hwndLV);
        m_hwndLV = NULL;
    }

    while(0 < m_rgpColumns.Count())
    {
        //
        // Delete each of the view's column objects.
        //
        delete m_rgpColumns[0];
        m_rgpColumns.Delete(0);
    }

    delete m_pCscObjIter;
}


//
// Create the listview control then restore any state persisted in 
// the registry.
//
bool
CacheView::CreateTheView(
    const RECT& rc
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheView::CreateTheView")));

    bool bResult = false;
    if (CreateListView(rc))
    {
        //
        // Read the view's previous state from the registry and restore
        // it.
        //
        bResult = RestoreViewState();

        if (NULL == m_pIconHound)
            m_pIconHound = new IconHound();

        if (NULL != m_pIconHound)
            m_pIconHound->Run(GetParent(m_hwndLV));
    }

    return bResult;
}


//
// Create the listview control and it's columns.  Note that each 
// column is represented by a derivative of LVColumn.
//
bool
CacheView::CreateListView(
    const RECT& rc
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheView::CreateListView")));
    DBGASSERT((NULL != m_hwndParent));
    DBGASSERT((NULL != m_hInstance));

    m_hwndLV = CreateWindowEx(WS_EX_CLIENTEDGE, 
                              WC_LISTVIEW,
                              TEXT(""),
                              WS_CHILD | WS_CLIPCHILDREN | 
                              WS_VISIBLE | WS_CLIPSIBLINGS | 
                              WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS | 
                              LVS_SHAREIMAGELISTS,
                              rc.left, rc.top,
                              rc.right - rc.left,
                              rc.bottom - rc.top,
                              m_hwndParent,
                              (HMENU)IDC_CACHEVIEW_LISTVIEW,
                              m_hInstance,
                              NULL);

    if (NULL == m_hwndLV)
    {
        DBGERROR((TEXT("Error 0x%08X creating listview control"), GetLastError()));
    }
    else
    {
        //
        // We talk to the header control so save it's handle.
        //
        m_hwndHeader = ListView_GetHeader(m_hwndLV);
        DBGASSERT((NULL != m_hwndHeader));

        if (NULL == m_himl)
        {
            //
            // Create an image list for the view.  Images are used by both 
            // the header and listview controls.  Note that m_himl is static.
            // Also note that the style bit LVS_SHAREIMAGELISTS is required
            // for us to share this image list between the two controls.
            //
            m_himl = CreateImageList();
        }
        if (NULL != m_himl)
        {
            Header_SetImageList(m_hwndHeader, m_himl);
            ListView_SetImageList(m_hwndLV, m_himl, LVSIL_SMALL);
        }
        //
        // Enable listview for images in sub-item columns, full-row select,
        // and drag-drop headers.
        //
        ListView_SetExtendedListViewStyle(m_hwndLV, 
                                          LVS_EX_SUBITEMIMAGES |
                                          LVS_EX_FULLROWSELECT |
                                          LVS_EX_HEADERDRAGDROP);
        //
        // Have the listview call us for overlay images.
        //
        ListView_SetCallbackMask(m_hwndLV, LVIS_OVERLAYMASK);
        //
        // Create the column objects placing their pointers into 
        // m_rgpColumns.  CreateListViewColumns() is a virtual
        // function implemented by each view type.  The resulting
        // column object pointers are stored in m_rgpColumns.
        //
        CreateListViewColumns();
        DBGASSERT((m_rgpColumns.Count() > 0));
        //
        // Add the columns to the list view.
        //
        for (int iSubItem = 0; iSubItem < m_rgpColumns.Count(); iSubItem++)
        {
            DBGASSERT((NULL != m_rgpColumns[iSubItem]));
            AddColumn(iSubItem, *m_rgpColumns[iSubItem]);
        }
    }
    
    return NULL != m_hwndLV && NULL != m_hwndHeader && NULL != m_himl;
}


//
// Create the image list used by the listview and header controls.
//
HIMAGELIST
CacheView::CreateImageList(
    void
    )
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("CacheView::CreateImageList")));
    DBGASSERT((NULL != m_hInstance));

    HIMAGELIST himl = NULL;

    //
    // Note:  The order of these icon ID's in this array must match with the
    //        iIMAGELIST_ICON_XXXXX enumeration.
    //        The enum values represent the image indices in the image list.
    //
    static const struct IconDef
    {
        LPTSTR szName;
        HINSTANCE hInstance;

    } rgIcons[] = { 
                    { MAKEINTRESOURCE(IDI_DOCUMENT),      m_hInstance },
                    { MAKEINTRESOURCE(IDI_SHARE),         m_hInstance },
                    { MAKEINTRESOURCE(IDI_SHARE_NOCNX),   m_hInstance },
                    { MAKEINTRESOURCE(IDI_FOLDER),        m_hInstance },
                    { MAKEINTRESOURCE(IDI_STALE),         m_hInstance },
                    { MAKEINTRESOURCE(IDI_PIN),           m_hInstance },
                    { MAKEINTRESOURCE(IDI_UNPIN),         m_hInstance },
                    { MAKEINTRESOURCE(IDI_STALE_OVERLAY), m_hInstance },
                    { MAKEINTRESOURCE(IDI_ENCRYPTED),     m_hInstance },
                    { MAKEINTRESOURCE(IDI_DECRYPTED),     m_hInstance }
                  };
    //
    // Create the image lists for the listview.
    //
    int cxIcon = GetSystemMetrics(SM_CXSMICON);
    int cyIcon = GetSystemMetrics(SM_CYSMICON);

    himl = ImageList_Create(cxIcon,
                            cyIcon,
                            ILC_MASK, 
                            ARRAYSIZE(rgIcons), 
                            10);
    if (NULL != himl)
    {
        for (UINT i = 0; i < ARRAYSIZE(rgIcons); i++)
        {
            HICON hIcon = (HICON)LoadImage(rgIcons[i].hInstance, 
                                           rgIcons[i].szName,
                                           IMAGE_ICON,
                                           cxIcon,
                                           cyIcon,
                                           0);
            if (NULL != hIcon)
            {
                ImageList_AddIcon(himl, hIcon);
                DestroyIcon(hIcon);
            }
            else
            {
                DBGERROR((TEXT("Error loading icon into image list")));
                throw CException(GetLastError());
            }
        }
        ImageList_SetBkColor(himl, CLR_NONE);  // Transparent background.
        ImageList_SetOverlayImage(himl, iIMAGELIST_ICON_OVERLAY_STALE, 1);
    }

    return himl;
}


//
// Retrieve the handle for the font used by the listview control.
//
HFONT
CacheView::GetListViewFont(
    void
    ) throw()
{
    DBGASSERT((NULL != m_hwndLV));
    return (HFONT)SendMessage(m_hwndLV, WM_GETFONT, 0, 0);
}


//
// Add a single column to the listview.
//
void
CacheView::AddColumn(
    int iSubItem,
    const LVColumn& col
    )
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("CacheView::AddColumn")));
    DBGPRINT((DM_VIEW, DL_LOW, TEXT("\tcolumn %d"), iSubItem));
    DBGASSERT((NULL != m_hwndLV));
    DBGASSERT((NULL != m_hwndHeader));
    DBGASSERT((NULL != m_hInstance));

    LV_COLUMN lvc;

    CString strColText(m_hInstance, col.GetTitleStrId());
    lvc.pszText = strColText;

    if (0 != col.GetWidth())
    {
        lvc.cx = col.GetWidth();  // Use width from col descriptor.
    }
    else
    {
        //
        // No width specified in column object.  Size column to the title.
        //
        HDC hdc = NULL;
        TEXTMETRIC tm;

        hdc = GetDC(m_hwndLV);
        GetTextMetrics(hdc, &tm);

        ReleaseDC(m_hwndLV, hdc);
        //
        // Nothing special about the +2.  Without it, we get trailing ellipsis.
        //
        lvc.cx = tm.tmAveCharWidth * (lstrlen(lvc.pszText) + 2);
    }

    lvc.iSubItem = iSubItem;
    lvc.mask     = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    lvc.fmt      = col.GetFormat();
    if (col.HasImage())
    {
        lvc.cx += GetSystemMetrics(SM_CXSMICON);
    }
    ListView_InsertColumn(m_hwndLV, iSubItem, &lvc);
    if (col.HasImage())
    {
        //
        // If the column header has an image associated with it, 
        // add it to the header.
        //
        HDITEM item;
        item.mask   = HDI_FORMAT;
        Header_GetItem(m_hwndHeader, iSubItem, &item);
        item.mask   = HDI_FORMAT | HDI_IMAGE;
        item.fmt    |= HDF_IMAGE;
        item.iImage = col.GetHeaderImageIndex();
        Header_SetItem(m_hwndHeader, iSubItem, &item);
    }
}


//
// Create an object iterator for enumerating objects in the 
// CSC object tree.  This is a virtual function.  This base
// class implementation is used by DetailsView and StaleView.
// ShareView provides it's own implementation since it only 
// displays share names.
//
CscObjIterator *
CacheView::CreateCscObjIterator(
    const CString& strShare
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheView::CreateCscObjIterator")));
    CscObjIterator *pIter = NULL;
    DWORD fExclude = (Viewer::m_bShowSparseFiles ? EXCLUDE_NONE : EXCLUDE_SPARSEFILES);
    if (strShare.IsEmpty())
    {
        //
        // Empty share name means we want to display all shares.
        //
        DBGPRINT((DM_VIEW, DL_MID, TEXT("\tCreating iterator for tree")));
        pIter = new CscObjTreeIterator(m_CscObjTree, fExclude);
    }
    else
    {
        //
        // Only display contents of share in "strShare".
        //
        CscShare *pShare = m_CscObjTree.FindShare(UNCPath(m_strShare));
        if (NULL != pShare)
        {
            DBGPRINT((DM_VIEW, DL_MID, TEXT("\tCreating iterator for subtree \"%s\""), m_strShare.Cstr()));
            pIter = new CscObjTreeIterator(*pShare, fExclude);
        }
    }
    return pIter;
}


//
// Flush the contents of the object load buffer to the listview
// control.  This batches updates to the listview into multi-object
// chunks.  Called by LoadNextCscObject() when the load buffer is
// full.
//
void
CacheView::FlushLoadBuffer(
    void
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheView::FlushLoadBuffer")));
    DBGASSERT((NULL != m_hwndLV));

    LV_ITEM item;

    item.mask       = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE | LVIF_PARAM;
    item.state      = 0;
    item.stateMask  = 0;
    item.iSubItem   = 0;
    item.pszText    = LPSTR_TEXTCALLBACK;
    item.iImage     = I_IMAGECALLBACK;

    int cItems = ListView_GetItemCount(m_hwndLV);

    //
    // Grow the listview's internal array by the number of items
    // we're going to add.  This reduces the amount of resizing
    // activity internal to the listview control.
    //
    ListView_SetItemCount(m_hwndLV, cItems + m_cLoadBuffer);

    SetRedraw(false);
    for (int i = 0; i < m_cLoadBuffer; i++)
    {
        CscObject *pObject = m_rgLoadBuffer[i];
        item.iItem  = cItems++;
        item.lParam = reinterpret_cast<LPARAM>(pObject);

        DBGPRINT((DM_VIEW, DL_LOW, TEXT("Adding item %d with lParam 0x%08X to listview"), item.iItem, item.lParam));
        ListView_InsertItem(m_hwndLV, &item);

        if (1 == cItems)
        {
            //
            // Set focus to the listview control once the first item is added.
            // This is calling CacheView::SetFocus, not the Win32 version.
            //
            SetFocus();
        }

        //
        // Must do this after the "SetFocus" call so that something is selected
        // in the listview.  Having a current selection affects the enabled-ness
        // of some menu items and toolbar buttons.  
        //
        SendMessage(GetParent(m_hwndLV), CVM_LVITEMCNTCHANGED, 0, cItems);

#ifdef __NEVER__
        //
        // BUGBUG:  For development only!
        //
        // This code throws an "out of memory" exception when we add the
        // 1233rd item to the listview.  I use it to verify that all
        // resources are properly cleaned up when an exception occurs.
        // [brianau - 11/10/97]
        // 
        if (0 == ((cItems + 1) % 1234))
        {
            DBGPRINT((TEXT("Item count = %d, Creating HUGE array"), cItems));
            SetRedraw(true);
            array_autoptr<BYTE> ptrBigArray(new BYTE[1000000000L]);

        }
#endif
        //
        // Pass file object pointers off to the icon hound if they don't
        // yet have an icon.  Don't send folder objects to the icon hound.
        // We use a fixed "folder" icon for all folders.
        //
        if (-1 == pObject->GetIconImageIndex() && 
            NULL != m_pIconHound &&
            pObject->IsFile())
        {
            m_pIconHound->AddObject(pObject);
        }
    }
    DBGASSERT((ListView_GetItemCount(m_hwndLV) == cItems));
    SetRedraw(true);

    m_cLoadBuffer = 0;
}



//
// Retrieve the next CSC object from the CSC object tree.
// Objects are initially cached in a load buffer.  Once the
// buffer is full, the objects are added to the listview
// control in FlushLoadBuffer().
// The number of objects actually added to the listview
// control is returned through *pcObjAdded.
// Returns true if there are more objects to load.
//
bool
CacheView::LoadNextCscObject(
    bool *pfObjEnum,
    int *pcObjAdded
    )
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("CacheView::LoadNextCscObject")));
    DBGASSERT((NULL != pcObjAdded));

    bool bIterResult = false;
    *pfObjEnum  = false;
    *pcObjAdded = 0;

    if (NULL == m_pCscObjIter)
    {
        m_pCscObjIter = CreateCscObjIterator(m_strShare);
    }

    if (NULL != m_pCscObjIter)
    {
        CscObject *pObject;
        if ((bIterResult = m_pCscObjIter->Next(&pObject)))
        {
            if (NULL != pObject)
            {
                *pfObjEnum = true; // Enumerated another object.
                m_cObjConsidered++;

                if (!ExcludeCscObject(*pObject))
                {
                    pObject->LockAncestors();
                    pObject->AddRef();
                    //
                    // We have an object and it's not excluded from the
                    // view.  
                    //
                    if (m_cLoadBuffer == ARRAYSIZE(m_rgLoadBuffer))
                    {
                        //
                        // Load buffer is full.  Flush it.
                        //
                        *pcObjAdded = m_cLoadBuffer;
                        FlushLoadBuffer();
                        DBGASSERT((0 == m_cLoadBuffer));
                    }
                    //
                    // Add this object ptr to the load buffer.
                    //
                    m_rgLoadBuffer[m_cLoadBuffer++] = pObject;
                }
            }
        }
        else
        {
            //
            // No more objects in enumeration.  Flush load buffer.
            //
            DBGPRINT((DM_VIEW, DL_MID, TEXT("View enumeration complete.")));
            *pcObjAdded = m_cLoadBuffer;
            FlushLoadBuffer();
            DBGASSERT((0 == m_cLoadBuffer));
        }
    }

    return bIterResult;
}


//
// The view's handler for LVN_GETDISPINFO.
// Calls GetDispInfo for the appropriate column.
//
void 
CacheView::GetDispInfo(
    LV_DISPINFO *pdi
    )
{
    DBGASSERT((NULL != pdi));
    DBGASSERT((NULL != m_rgpColumns[pdi->item.iSubItem]));

    //
    // Get the display information from the appropriate column object.
    //
    m_rgpColumns[pdi->item.iSubItem]->GetDispInfo(pdi);
}



//
// Set focus to the listview control and have it focus on an item.
//
void
CacheView::SetFocus(
    void
    ) const throw()
{
    ::SetFocus(m_hwndLV); // Win32 SetFocus().
    FocusOnSomething(); 
}


//
// Have the listview focus on something.  If an item is currently 
// selected, it will receive the focus.  Otherwise, the first
// item will receive the focus.
//
void
CacheView::FocusOnSomething(
    void
    ) const throw()
{
    INT iFocus = ListView_GetNextItem(m_hwndLV, -1, LVNI_FOCUSED);
    if (-1 == iFocus)
        iFocus = 0;

    ListView_SetItemState(m_hwndLV, iFocus, LVIS_FOCUSED | LVIS_SELECTED, 
                                            LVIS_FOCUSED | LVIS_SELECTED)
}


//
// Select all items in the view.
//
void
CacheView::SelectAllItems(
    void
    )
{
    DBGASSERT((NULL != m_hwndLV));

    ::SetFocus(m_hwndLV);  // Win32 SetFocus().
    ListView_SetItemState(m_hwndLV, -1, LVIS_SELECTED, LVIS_SELECTED);
}


//
// Invert listview selection so that all selected items are unselected and all
// unselected items are selected.
//
void
CacheView::InvertSelectedItems(
    void
    )
{
    DBGASSERT((NULL != m_hwndLV));

    INT iItem = -1;

    ::SetFocus(m_hwndLV);
    while ((iItem = ListView_GetNextItem(m_hwndLV, iItem, 0)) != -1)
    {
        DWORD dwState = ListView_GetItemState(m_hwndLV, iItem, LVIS_SELECTED);
        ListView_SetItemState(m_hwndLV, iItem, dwState ^ LVNI_SELECTED, LVIS_SELECTED);
    }
}


//
// Callback that compares two items in a view column.  Called
// from within the ListView common control in response to 
// ListView_SortItems.
//
// a          = Address of 1st object to be compared.
// b          = Address of 2nd object to be compared.
// lParamSort = Address of COMPARESTRUCT structure containing 
//              sort information.
//
// Returns:  < 0 if (a < b)
//           > 0 if (a > b)
//             0 if (a == b)
//
int CALLBACK 
CacheView::SortCompareFunc(
    LPARAM a, 
    LPARAM b,   
    LPARAM lParamSort
    ) 
{
    COMPARESTRUCT *pcs = reinterpret_cast<COMPARESTRUCT *>(lParamSort);
    DBGASSERT((NULL != pcs));
    DBGASSERT((NULL != pcs->pColumn));
    DBGASSERT((0 != a));
    DBGASSERT((0 != b));

    //
    // CompareItems() is a virtual function implemented by each column
    // object.  Each implementation knows how to compare two items
    // based on the column value.
    //
    return pcs->pColumn->CompareItems(*(reinterpret_cast<CscObject *>(a)),
                                      *(reinterpret_cast<CscObject *>(b)),
                                       pcs->bSortAscending);
}


//
// Sorts all items in the listview using a given column as a sort key.
//
LRESULT
CacheView::SortObjects(
    DWORD idColumn,
    bool bSortAscending
    ) throw()
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("CacheView::SortObjects")));
    DBGASSERT((NULL != m_hwndLV));

    LVColumn::ResetCache();

    //
    // Need to pass the sort direction and address of the appropriate
    // LVColumn object to the sort compare function.
    //
    COMPARESTRUCT cs;
    cs.bSortAscending = bSortAscending;
    cs.pColumn        = m_rgpColumns[idColumn];
    //
    // Calls CacheView::SortCompareFunc() for all item comparisons.
    //
    ListView_SortItems(m_hwndLV, SortCompareFunc, (LPARAM)&cs);
    return 0;   
}


//
// Save view state information to the registry.
//       
bool
CacheView::SaveViewState(
    void
    ) const
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheView::SaveViewState")));
    DBGASSERT((NULL != m_hwndHeader));
    DBGASSERT((NULL != m_hwndLV));

    //
    // First store the information in a ViewState object.
    //
    ViewState vs;
    int cColumns = Header_GetItemCount(m_hwndHeader);

    array_autoptr<int> ptrIndices(new int[cColumns]);

    Header_GetOrderArray(m_hwndHeader, cColumns, ptrIndices.get());

    for (int i = 0; i < cColumns; i++)
    {
        vs.AddColumnInfo(*(ptrIndices.get() + i), ListView_GetColumnWidth(m_hwndLV, i));
    }
    //
    // Now write the view state object to the registry.
    //
    return SaveViewStateToRegistry(vs);
}


//
// Restore view state from values stored in the registry.
//
bool
CacheView::RestoreViewState(
    void
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheView::RestoreViewState")));
    DBGASSERT((NULL != m_hwndHeader));
    DBGASSERT((NULL != m_hwndLV));

    bool bResult = false;
    ViewState vs;

    //
    // First read persisted view state info from the registry
    // and write it to a ViewState object.
    //
    if (LoadViewStateFromRegistry(&vs))
    {
        int cCol = vs.ColumnCount();
        int iCol;

        //
        // Restore the user's last column ordering.
        //
        array_autoptr<int> ptrIndices(new int[cCol]);
        for (iCol = 0; iCol < cCol; iCol++)
        {
            *(ptrIndices.get() + iCol) = vs.ColumnIndex(iCol);
        }
        DBGASSERT((cCol == Header_GetItemCount(m_hwndHeader)));

        Header_SetOrderArray(m_hwndHeader, 
                             Header_GetItemCount(m_hwndHeader),
                             ptrIndices.get());

        //
        // Restore the user's last column widths.
        //
        for (iCol = 0; iCol < cCol; iCol++)
        {
            ListView_SetColumnWidth(m_hwndLV, iCol, vs.ColumnWidth(iCol));
        }
        bResult = true;
    }
    return bResult;
}


//
// Save a view state object's contents to the registry.
//
bool
CacheView::SaveViewStateToRegistry(
    const ViewState& vs
    ) const
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("CacheView::SaveViewStateToRegistry")));

    bool bResult = false;
    HKEY hkey = NULL;
    //
    // Size and allocate a temporary buffer for the state information.
    //
    int cbBuffer = vs.PersistentBufferSize();
    array_autoptr<BYTE> ptrBuffer(new BYTE[cbBuffer]);

    //
    // Transfer the ViewState object's information to the
    // temporary buffer then write that buffer to the registry.
    //
    if (vs.WriteToBuffer(ptrBuffer.get(), cbBuffer))
    {
        RegKey key(HKEY_CURRENT_USER, m_strRegKey);
        if (SUCCEEDED(key.Open(KEY_WRITE, true)))
        {
            CString strValueName;
            GetViewStateRegValueName(&strValueName);

            bResult = SUCCEEDED(key.SetValue(strValueName, ptrBuffer.get(), cbBuffer));
        }
    }

    return bResult;
};


//
// Load a view's state information from the registry.
//
bool
CacheView::LoadViewStateFromRegistry(
    ViewState *pvs
    )
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("CacheView::LoadViewStateFromRegistry")));
    DBGASSERT((NULL != pvs));

    bool bResult = false;

    RegKey key(HKEY_CURRENT_USER, m_strRegKey);
    if (SUCCEEDED(key.Open(KEY_READ, true)))
    {
        CString strValueName;
        GetViewStateRegValueName(&strValueName);
        int cbBuffer = key.GetValueBufferSize(strValueName);
        array_autoptr<BYTE> ptrBuffer(new BYTE[cbBuffer]);

        bResult = SUCCEEDED(key.GetValue(strValueName, ptrBuffer.get(), cbBuffer)) &&
                  pvs->LoadFromBuffer(ptrBuffer.get(), cbBuffer);
    }
    return bResult;
}


//
// Since we destroy and create a view each time the user changes views,
// I've made the imagelist static (no sense in re-creating it each time
// a view is created).  However, this means we can't destroy the image
// list in the view's dtor.  Only the containing CacheWindow object knows
// when the last view object is being destroyed.  Therefore, I've provided
// this function that is called in CacheWindow::~CacheWindow just prior
// to destroying the view object.
//
void
CacheView::DestroyImageList(
    void
    ) throw()
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("CacheView::DestroyImageList")));
    if (NULL != m_himl)
    {
        ImageList_Destroy(m_himl);
        m_himl = NULL;
    }
}


void
CacheView::DestroyIconHound(
    void
    ) throw()
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("CacheView::DestroyIconHound")));
    delete m_pIconHound;
    m_pIconHound = NULL;
}


//
// Determine if the icon hound thread is currently running.
// By default the function checks thread status without waiting.  You
// can optionally specify a wait time in milliseconds if you're testing
// for negativity (!IsIconHoundThreadRunning()) and you want to give the 
// thread a little time to exit normally.
//
// Returns:
//
//      true  = Thread is running.
//      false = Thread has exited.
//
bool
CacheView::IsIconHoundThreadRunning(
    DWORD dwWait    // Default is 0
    )
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("CacheView::IsIconHoundThreadRunning")));

    return NULL != m_pIconHound && !m_pIconHound->WaitForThreadExit(dwWait);
}

//
// End the icon hound's thread as soon as possible.
// Waits dwWait milliseconds to see if the thread actually exits.
// If the thread is busy in a call to SHGetFileInfo, it will not be able
// to exit until the SHGetFileInfo call returns.  Under some net conditions,
// this can take a while.
//
// Returns:
//
//      true  = Thread exited.
//      false = Thread not yet exited.
// 
bool
CacheView::EndIconHoundThread(
    DWORD dwWait    // Default is INFINITE
    )
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("CacheView::EndIconHoundThread")));

    //
    // Clear out the icon hound's input queue and place a sentinel
    // value in it.  This will cause the icon hound thread to exit
    // as soon as possible in a normal fashion.
    //
    return NULL == m_pIconHound || m_pIconHound->EndThread(dwWait);
}



void
CacheView::ObjectChanged(
    const CscObject *pObject
    ) const throw()
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheView::ObjectChanged")));
    int iItem = FindObject(pObject);
    if (-1 != iItem)
    {
        DBGPRINT((DM_VIEW, DL_MID, TEXT("Redrawing LV item %d"), iItem));
        ListView_RedrawItems(m_hwndLV, iItem, iItem);
    }
}


//
// Removes an object from the listview.
// Releases the object pointer held in the listview.
//
void
CacheView::DeleteObject(
    const CscObject *pObject
    ) const
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CacheView::DeleteObject")));
    int iItem = FindObject(pObject);
    if (-1 != iItem)
    {
        DBGPRINT((DM_VIEW, DL_MID, TEXT("Deleting LV item %d"), iItem));

        UINT uState = ListView_GetItemState(m_hwndLV, iItem, LVIS_ALL);
        ListView_DeleteItem(m_hwndLV, iItem);
        if (LVIS_FOCUSED & uState)
        {
            //
            // This behavior for adjusting the focus and repainting the listview
            // following a delete was taken from shell32's defview.
            //
            int iFocus = iItem;
            if (iFocus > 0 && ListView_GetItemCount(m_hwndLV) >= iFocus)
                iFocus--;

            if (-1 != iFocus)
            {
                ListView_SetItemState(m_hwndLV, iFocus, LVIS_FOCUSED | LVIS_SELECTED, 
                                                        LVIS_FOCUSED | LVIS_SELECTED);
                ListView_EnsureVisible(m_hwndLV, iFocus, FALSE);
            }
        }
        SendMessage(GetParent(m_hwndLV), CVM_LVITEMCNTCHANGED, 0, ObjectCount());
        pObject->ReleaseAncestors();
        pObject->Release();
    }
}

int
CacheView::FindObject(
    const CscObject *pObject
    ) const throw()
{
    LV_FINDINFO fi;
    fi.flags  = LVFI_PARAM;
    fi.lParam = reinterpret_cast<LPARAM>(pObject);

    return ListView_FindItem(m_hwndLV, -1, &fi);
}
    


bool 
CacheView::CanOpen(
    void
    ) const throw()
{ 
    //
    // Can open a selection if only one item selected and
    // it's a file.
    //
    bool bCanOpen = false;
    if (1 == SelectedObjectCount())
    {
        CacheView::ItemIterator iter = const_cast<CacheView *>(this)->CreateItemIterator(true);
        CscObject *pObject;
        if (iter.Next(&pObject))
        {
            DBGASSERT((NULL != pObject));
            bCanOpen = pObject->IsFile();
        }
    }
    return bCanOpen;
}


//
// Scan the selected items and determine if any item belongs to a 
// share that is disconnected.
// Returns:
//    true  = At least one item is from disconnecte share.
//    false = No items are from a disconnected share.
//
bool
CacheView::AnyInSelectionDisconnected(
    void
    ) const throw()
{
    ItemIterator iter = const_cast<CacheView *>(this)->CreateItemIterator(true);
    CscObject *pObject;
    while (iter.Next(&pObject))
    {
        if (!pObject->IsShareOnLine())
            return true;
    }
    return false;
}



//-----------------------------------------------------------------------------
// class CacheView::ItemIterator
//-----------------------------------------------------------------------------
CacheView::ItemIterator::ItemIterator(
    const CacheView::ItemIterator& rhs
    ) throw()
       : m_hwndLV(NULL),
         m_iItem(-1),
         m_bSelected(false)
{
    *this = rhs;
}


CacheView::ItemIterator& 
CacheView::ItemIterator::operator = (
    const ItemIterator& rhs
    ) throw()
{
    if (&rhs != this)
    {
        m_hwndLV    = rhs.m_hwndLV;
        m_iItem     = rhs.m_iItem;
        m_bSelected = rhs.m_bSelected;
    }
    return *this;
}


bool
CacheView::ItemIterator::Next(
    CscObject **ppObject
    ) throw()
{
    m_iItem = ListView_GetNextItem(m_hwndLV, m_iItem, m_bSelected ? LVNI_SELECTED : 0);
    return NULL != (*ppObject = GetItemObject(m_hwndLV, m_iItem));
}


CscObject *
CacheView::GetItemObject(
    HWND hwndLV,
    int iItem
    ) throw()
{
    if (0 <= iItem)
    {
        LV_ITEM item;
        item.mask     = LVIF_PARAM;
        item.iItem    = iItem;
        item.iSubItem = 0;

        if (ListView_GetItem(hwndLV, &item))
            return reinterpret_cast<CscObject *>(item.lParam);
    }
    return NULL;
}


//-----------------------------------------------------------------------------
// class CacheView::IconHound
//-----------------------------------------------------------------------------

CArray<unsigned short> CacheView::IconHound::m_rgSysImageXref;

CacheView::IconHound::IconHound(
    void
    ) : m_hwndNotify(NULL),
        m_sem(0, LONG_MAX),
        m_hThread(NULL)
{
    DBGTRACE((DM_ICONHOUND, DL_HIGH, TEXT("CacheView::IconHound::IconHound")));
    if (m_rgSysImageXref.IsEmpty())
    {
        m_rgSysImageXref.SetAtGrow(-1, 0);
    }
}


void
CacheView::IconHound::Run(
    HWND hwndNotify
    ) throw()
{
    m_cs.Enter();
    DBGASSERT((NULL != hwndNotify));
    m_hwndNotify = hwndNotify;
    if (NULL == m_hThread)
    {
        DWORD idThread;
        m_hThread = (HANDLE)_beginthreadex(NULL,
                                   0,          // Default stack size
                                   ThreadProc,
                                   (LPVOID)this,
                                   0,
                                   (UINT *)&idThread);

        DBGPRINT((DM_ICONHOUND, DL_HIGH, TEXT("Created icon hound thread %d"), idThread));
    }
    m_cs.Leave();
}


CacheView::IconHound::~IconHound(
    void
    )
{
    DBGTRACE((DM_ICONHOUND, DL_HIGH, TEXT("CacheView::IconHound::~IconHound")));
    EndThread(INFINITE);

    if (NULL != m_hThread)
        CloseHandle(m_hThread);
}


//
// Add an object to the icon hound's input queue.
//
void 
CacheView::IconHound::AddObject(
    CscObject *pObject
    )
{
    DBGTRACE((DM_ICONHOUND, DL_MID, TEXT("CacheView::IconHound::AddObject")));
    DBGPRINT((DM_ICONHOUND, DL_MID, TEXT("\tobject = 0x%08X"), pObject));

    AutoLockCs lock(m_cs);
    if (m_oq.Add(pObject))
    {
        if (NULL != pObject)    // pObject can be NULL (sentinel)
        {
            pObject->LockAncestors();
            pObject->AddRef();
        }
        m_sem.Release();   // Incr input queue semaphore.
    }
}


//
// Clear the icon hound thread's input queue and place a sentinel value
// in the queue.  Waits dwWait milliseconds to determine if the thread proc
// has exited.  By default, it waits until the thread has exited.
//
// Returns:
//
//      true   = Thread as exited.
//      false  = Thread has not exited.
// 
bool
CacheView::IconHound::EndThread(
    DWORD dwWait    // Default is INFINITE
    )
{
    DBGTRACE((DM_ICONHOUND, DL_HIGH, TEXT("CacheView::IconHound::EndThread")));
    AutoLockCs lock(m_cs);
    CscObject *pObject;
    while(m_oq.Remove(&pObject))
    {
        if (NULL != pObject)
        {
            pObject->ReleaseAncestors();
            pObject->Release();
        }
    }
    AddObject(NULL); // Add NULL sentinel.  Will cause thread to exit.
    lock.Release();
    return WaitForThreadExit(dwWait);
}


//
// Waits dwWait milliseconds for the icon hound's thread to exit.
// By default it waits indefinitely.  To perform a simple check on
// thread status, set the dwWait arg to 0.
//
// Returns:
//    true  = Thread has exited.
//    false = Thread has not exited.  Wait timed out.
//
bool
CacheView::IconHound::WaitForThreadExit(
    DWORD dwWait    // Default == INFINITE
    )
{
    DBGTRACE((DM_ICONHOUND, DL_HIGH, TEXT("CacheView::IconHound::WaitForThreadExit")));

    return (NULL == m_hThread || WAIT_OBJECT_0 == WaitForSingleObject(m_hThread, dwWait));
}



int
CacheView::IconHound::GetCachedImageIndex(
    int iSysImage
    ) const throw()
{
    return (iSysImage <= m_rgSysImageXref.UpperBound()) ? m_rgSysImageXref[iSysImage] : -1;
}


void
CacheView::IconHound::CacheSysImageIndex(
    int iSysImage,
    int iImage
    )
{
    DBGASSERT((-1 == GetCachedImageIndex(iSysImage)));
    int iBound = m_rgSysImageXref.UpperBound() + 1;
    if (m_rgSysImageXref.SetAtGrow(iImage, iSysImage)) // Store iImage at index iSysImage.
    {
        //
        // Array grew. Fill the newly allocated array elements with -1.
        //
        while(iBound < iSysImage)
            m_rgSysImageXref[iBound++] = -1;
    }
}


//
// This thread removes CSC object pointers from an input queue then
// goes to the shell to get the object's icon.  Icon information
// is cached in the object tree.
// 
UINT
CacheView::IconHound::ThreadProc(
    LPVOID pvParam
    ) throw()
{
    DBGTRACE((DM_ICONHOUND, DL_HIGH, TEXT("CacheView::IconHound::ThreadProc")));

    //
    // Lower thread priority of the icon hound so the UI remains responsive.
    //
    if (0 == SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST))
            DBGERROR((TEXT("SetThreadPriority failed with error %d"), GetLastError()));

    CscObject *pObject = NULL;
    try
    {
        CacheView::IconHound *pThis = reinterpret_cast<CacheView::IconHound *>(pvParam);
        DBGASSERT((NULL != pThis));

        SHFILEINFO sfi;     
        HIMAGELIST hsiml;    // System image list handle.
        CString strFullPath;
        for (;;)
        {
            DBGPRINT((DM_ICONHOUND, DL_MID, TEXT("Icon hound waiting for queue")));

            pThis->m_sem.Wait();  // Wait 'til there's something in the queue.

            AutoLockCs lock(pThis->m_cs);
            pThis->m_oq.Remove(&pObject); // Remove the next item from queue.
            lock.Release();

            if (NULL == pObject)
                break; // Sentinel item.  End thread.
        
            pObject->GetFullPath(&strFullPath);
            hsiml = (HIMAGELIST)SHGetFileInfo(strFullPath,
                                              0,
                                              &sfi,
                                              sizeof(sfi),
                                              SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
            if (hsiml)
            {
                int iImage = pThis->GetCachedImageIndex(sfi.iIcon);
                if (-1 == iImage)
                {
                    HICON hIcon = ImageList_GetIcon(hsiml,
                                                    sfi.iIcon,
                                                    ILD_NORMAL);
                    if (hIcon)
                    {
                        iImage = ImageList_AddIcon(m_himl, hIcon);
                        DestroyIcon(hIcon);
                        if (-1 != iImage)
                            pThis->CacheSysImageIndex(sfi.iIcon, iImage);
                    }
                }
                if (-1 != iImage)
                {
                    pObject->SetIconImageIndex(iImage);
                    //
                    // Notify the window that the object has changed.
                    // This will cause it to display the new icon.
                    //
                    DBGASSERT((NULL != pThis->m_hwndNotify));
                    PostMessage(pThis->m_hwndNotify, CVM_UPDATEOBJECT, 0, (LPARAM)pObject);
                }
            }
            else
            {
                DBGERROR((TEXT("SHGetFileInfo failed for \"%s\""), strFullPath.Cstr()));
            }
            pObject->ReleaseAncestors();
            pObject->Release();
            pObject = NULL;
        }
    }
    //
    // BUGBUG:  Do we need to do anything special with these exceptions?
    //          Since this is merely the icon-gathering thread, I don't 
    //          believe we should bother the user.  The only thing that 
    //          will happen is that the user will see a generic "document" 
    //          icon instead of file-specific icons.  [brianau 1/17/98]
    //
    catch(CException& e)
    {
        DBGERROR((TEXT("C++ exception %d caught in IconHound::ThreadProc"), e.dwError));
    }
    catch(...)
    {
        DBGERROR((TEXT("Unknown exception caught in IconHound::ThreadProc")));
    }
    if (NULL != pObject)
    {
        pObject->ReleaseAncestors();
        pObject->Release();
    }
    return 0;
}


//-----------------------------------------------------------------------------
// class ShareView
//-----------------------------------------------------------------------------

//
// Create all of the listview columns for a ShareView object.
//
void
ShareView::CreateListViewColumns(
    void
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("ShareView::CreateListViewColumns")));
    m_rgpColumns.Append(new LVColShareWithImage(*this, 
                                                m_CscObjTree,
                                                LVCFMT_LEFT,
                                                0,
                                                IDS_TITLE_COL_SHARE));

    m_rgpColumns.Append(new LVColServer(*this,
                                        m_CscObjTree,
                                        LVCFMT_LEFT,
                                        0,
                                        IDS_TITLE_COL_SERVER));

    m_rgpColumns.Append(new LVColShareStatus(*this,
                                        m_CscObjTree,
                                        LVCFMT_LEFT,
                                        0,
                                        IDS_TITLE_COL_SHARESTATUS));

    m_rgpColumns.Append(new LVColObjectCount(*this,
                                        m_CscObjTree,
                                        LVCFMT_RIGHT,
                                        0,
                                        IDS_TITLE_COL_OBJECTCOUNT));

    m_rgpColumns.Append(new LVColPinnedCount(*this,
                                        m_CscObjTree,
                                        LVCFMT_RIGHT,
                                        0,
                                        IDS_TITLE_COL_PINNEDCOUNT,
                                        iIMAGELIST_ICON_PIN));

    m_rgpColumns.Append(new LVColUpdateCount(*this,
                                        m_CscObjTree,
                                        LVCFMT_RIGHT,
                                        0,
                                        IDS_TITLE_COL_UPDATECOUNT,
                                        iIMAGELIST_ICON_STALE));
}


//
// ShareView's specialized implementation of CreateCscObjIterator.
// A child iterator for the tree is created to enumerate only
// the direct children of the tree (i.e. the shares).
//
CscObjIterator *
ShareView::CreateCscObjIterator(
    const CString& /* unused */
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("ShareView::CreateCscObjIterator")));
    //
    // This code is sort of funky.  CreateChildIterator actually returns
    // an iterator object but what we want to return is the ADDRESS
    // of an iterator object on the heap.  Therefore, we create the
    // iterator on the heap using the copy constructor to copy the
    // iterator created by CreateChildIterator.  Normally, I would think
    // this is excessive copying.  However, copying iterators is very
    // cheap and this does what I need.
    //
    DWORD fExclude = EXCLUDE_NONEXISTING | 
                     (Viewer::m_bShowSparseFiles ? 0 : EXCLUDE_SPARSEFILES);

    return new CscObjIterator(m_CscObjTree.CreateChildIterator(fExclude));
}



//-----------------------------------------------------------------------------
// class DetailsView
//-----------------------------------------------------------------------------

//
// Create all of the listview columns for a DetailsView object.
//
void
DetailsView::CreateListViewColumns(
    void
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("DetailsView::CreateListViewColumns")));


    m_rgpColumns.Append(new LVColPinned(*this,
                                        m_CscObjTree,
                                        0,
                                        0,
                                        IDS_TITLE_COL_PINNED,
                                        iIMAGELIST_ICON_PIN));

/*
    //
    // BUGBUG: Encryption support is for beta 2.
    //
    m_rgpColumns.Append(new LVColEncrypted(*this,
                                        m_CscObjTree,
                                        0,
                                        0,
                                        IDS_TITLE_COL_ENCRYPTED,
                                        iIMAGELIST_ICON_ENCRYPTED));
*/
    m_rgpColumns.Append(new LVColName(*this,
                                        m_CscObjTree,
                                        LVCFMT_LEFT,
                                        0,
                                        IDS_TITLE_COL_NAME));

    m_rgpColumns.Append(new LVColFolder(*this,
                                        m_CscObjTree,
                                        LVCFMT_LEFT,
                                        0,
                                        IDS_TITLE_COL_FOLDER));

    m_rgpColumns.Append(new LVColShare(*this,
                                       m_CscObjTree,
                                       LVCFMT_LEFT,
                                       0,
                                       IDS_TITLE_COL_SHARE));

    m_rgpColumns.Append(new LVColServer(*this,
                                        m_CscObjTree,
                                        LVCFMT_LEFT,
                                        0,
                                        IDS_TITLE_COL_SERVER));

    m_rgpColumns.Append(new LVColSize(*this,
                                        m_CscObjTree,
                                        LVCFMT_LEFT,
                                        0,
                                        IDS_TITLE_COL_SIZE));

    m_rgpColumns.Append(new LVColModified(*this,
                                        m_CscObjTree,
                                        LVCFMT_LEFT,
                                        0,
                                        IDS_TITLE_COL_MODIFIED));
}




//-----------------------------------------------------------------------------
// class StaleView
//-----------------------------------------------------------------------------

//
// Create all of the listview columns for a StaleView object.
//
void
StaleView::CreateListViewColumns(
    void
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("StaleView::CreateListViewColumns")));
    m_rgpColumns.Append(new LVColPinned(*this,
                                        m_CscObjTree,
                                        0,
                                        0,
                                        IDS_TITLE_COL_PINNED,
                                        iIMAGELIST_ICON_PIN));

/*
    //
    // BUGBUG: Encryption support is for beta 2.
    //
    m_rgpColumns.Append(new LVColEncrypted(*this,
                                        m_CscObjTree,
                                        0,
                                        0,
                                        IDS_TITLE_COL_ENCRYPTED,
                                        iIMAGELIST_ICON_ENCRYPTED));
*/
    m_rgpColumns.Append(new LVColName(*this,
                                        m_CscObjTree,
                                        LVCFMT_LEFT,
                                        0,
                                        IDS_TITLE_COL_NAME));

    m_rgpColumns.Append(new LVColFolder(*this,
                                        m_CscObjTree,
                                        LVCFMT_LEFT,
                                        0,
                                        IDS_TITLE_COL_FOLDER));

    m_rgpColumns.Append(new LVColShare(*this,
                                       m_CscObjTree,
                                       LVCFMT_LEFT,
                                       0,
                                       IDS_TITLE_COL_SHARE));

    m_rgpColumns.Append(new LVColServer(*this,
                                        m_CscObjTree,
                                        LVCFMT_LEFT,
                                        0,
                                        IDS_TITLE_COL_SERVER));

    m_rgpColumns.Append(new LVColStaleReason(*this,
                                        m_CscObjTree,
                                        LVCFMT_LEFT,
                                        0,
                                        IDS_TITLE_COL_STALEREASON));
}


bool 
StaleView::ExcludeCscObject(
    const CscObject& object
    ) const throw()
{
    return object.IsShare() || !object.NeedToSync();
}



//-----------------------------------------------------------------------------
// class ViewState
//-----------------------------------------------------------------------------

ViewState::ViewState(
    const ViewState& rhs
    )
{
    *this = rhs;
}


ViewState& 
ViewState::operator = (
    const ViewState& rhs
    )
{
    if (this != &rhs)
    {
        m_rgColIndices = rhs.m_rgColIndices;
        m_rgColWidths  = rhs.m_rgColWidths;
    }
    return *this;
}


void
ViewState::Clear(
    void
    ) throw()
{
    m_rgColWidths.Clear();
    m_rgColIndices.Clear();
}

//
// Add a record of column information (width and position) to the viewstate
// object.
//
void 
ViewState::AddColumnInfo(
    int iIndex,
    int iWidth
    )
{
    m_rgColIndices.Append(iIndex);
    m_rgColWidths.Append(iWidth);
}


//
// Calculates the size of a buffer needed to hold the viewstate info.
//
// The format of the buffer is:
//
// --------------+
// | version (16)|
// +-------------+----------------+-----------------------------+
// | Col Cnt (16)|   Array of column index values (each 16-bit) |
// +-------------+----------------------------------------------+
// | Col Cnt (16)|   Array of column width values (each 16-bit) |
// +-------------+----------------------------------------------+
//
int 
ViewState::PersistentBufferSize(
    void
    ) const throw()
{
    return m_rgColIndices.Count() * sizeof(short) +  // column positions
           m_rgColWidths.Count() * sizeof(short) +   // column widths.
           3 * sizeof(short);                        // column counts and version
}


//
// Write the view state info to a caller-provided buffer.
// Caller should use PersistentBufferSize() to determine the
// size of the destination buffer.
//
bool
ViewState::WriteToBuffer(
    LPBYTE pbBuffer,
    int cbBuffer
    ) const
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("ViewState::WriteToBuffer")));
    DBGASSERT((NULL != pbBuffer));

    if (cbBuffer < PersistentBufferSize())
    {
        return false;  // Destination buffer too small.
    }
    int i;
    short *piBuffer = (short *)pbBuffer;
    //
    // Save a version number with the info in case we need to invalidate
    // existing registry data at some point in the future.
    //
    *piBuffer++ = (short)VIEW_STATE_VERSION;
    //
    // Save the column count followed by each of the column indices.
    // All values saved are 16-bit integers.
    //
    *piBuffer++ = (short)(m_rgColIndices.Count());
    for (i = 0; i < m_rgColIndices.Count(); i++)
    {
        *piBuffer++ = (short)(m_rgColIndices[i]);
    }
    //
    // Save the column count followed by each of the column widths.
    // All values saved are 16-bit integers.
    //
    *piBuffer++ = (short)(m_rgColWidths.Count());
    for (i = 0; i < m_rgColWidths.Count(); i++)
    {
        *piBuffer++ = (short)(m_rgColWidths[i]);
    }
    return true;
}


//
// Read the view state info from a caller-provided buffer.
//
bool
ViewState::LoadFromBuffer(
    LPBYTE pbBuffer,
    int cbBuffer
    )
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("ViewState::LoadFromBuffer")));
    DBGASSERT((NULL != pbBuffer));

    if (cbBuffer < PersistentBufferSize())
    {
        return false;
    }
    int i;
    short *piBuffer = (short *)pbBuffer;

    if (VIEW_STATE_VERSION == *piBuffer++)
    {
        //
        // Data has correct version number.
        //
        int n = (int)(*piBuffer++);
        for (i = 0; i < n; i++)
        {
            m_rgColIndices.Append((int)(*piBuffer++));
        }
        n = (int)(*piBuffer++);
        for (i = 0; i < n; i++)
        {
            m_rgColWidths.Append((int)(*piBuffer++));
        }
        return true;
    }
    return false;
}



//
// autoptr's for scalar types can generate warning C4284.
// It's meaningless.
//
#pragma warning (default : 4284)
