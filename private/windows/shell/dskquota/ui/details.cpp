///////////////////////////////////////////////////////////////////////////////
/*  File: details.cpp

    Description: Contains definition for class DetailsView.
        This class implements a list view containing quota information about
        the various accounts in a volume's quota information file.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
    05/28/97    Major changes.                                       BrianAu
                - Added "User Finder".
                - Added promotion of selected item to front of
                  name resolution queue.
                - Improved name resolution status reporting through
                  listview.
                - Moved drag/drop and report generation code
                  from dragdrop.cpp and reptgen.cpp into the
                  DetailsView class.  DetailsView now implements
                  IDataObject, IDropSource and IDropTarget instead
                  of deferring implementation to secondary objects.
                  dragdrop.cpp and reptgen.cpp have been dropped
                  from the project.
                - Added support for CF_HDROP and private import/
                  export clipboard formats.
                - Added import/export functionality.
    07/28/97    Removed export support for CF_HDROP.  Replaced       BrianAu
                with FileContents and FileGroupDescriptor.  Import
                from CF_HDROP is still supported.
                Added Import Source object hierarchy.
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"  // PCH
#pragma hdrstop

#include <htmlhelp.h>
#include <commctrl.h>
#include <commdlg.h>

#include "uihelp.h"
#include "uiutils.h"
#include "dskquota.h"
#include "registry.h"
#include "resource.h"
#include "shellinc.h"
#include "userprop.h"
#include "details.h"
#include "guidsp.h"
#include "mapfile.h"
#include "progress.h"
#include "yntoall.h"
#include "ownerlst.h"
#include "ownerdlg.h"
#include "adusrdlg.h"
//
// Constant text strings.
//
TCHAR c_szWndClassDetailsView[]   = TEXT("DetailsView");

//
// Bitmap dimension constants.
//
const UINT BITMAP_WIDTH     = 16;
const UINT BITMAP_HEIGHT    = 16;
const UINT LG_BITMAP_WIDTH  = 32;
const UINT LG_BITMAP_HEIGHT = 32;

//
// How much to grow the user object list whenever expansion is required.
//
const INT USER_LIST_GROW_AMT = 100;

//
// This structure is used to pass the DetailsView object's "this" pointer
// in WM_CREATE.
//
typedef struct WndCreationData {
    SHORT   cbExtra;
    LPVOID  pThis;
} WNDCREATE_DATA;

typedef UNALIGNED WNDCREATE_DATA *PWNDCREATE_DATA;

//
// Structure passed to CompareItems callback.
//
typedef struct comparestruct
{
    DWORD idColumn;
    DWORD dwDirection;
    DetailsView *pThis;
} COMPARESTRUCT, *PCOMPARESTRUCT;


//
// Define some names for indexes into the listview's image list.
//
#define iIMAGELIST_ICON_NOIMAGE       (-1)
#define iIMAGELIST_ICON_OK              0
#define iIMAGELIST_ICON_WARNING         1
#define iIMAGELIST_ICON_LIMIT           2

//
// The 0-based index of the "View" item in the main menu and of the
// "Arrange" item in the view menu.
// WARNING:  If you change menu items, these may need updating.
//
#define iMENUITEM_VIEW                  2
#define iMENUITEM_VIEW_ARRANGE          4
//
// Same thing for the "Edit" menu.
//
#define iMENUITEM_EDIT                  1

//
// Add/remove from this array to change the columns in the list view.
// IMPORTANT:
//     The ordering of these items is very important (sort of).
//     Because of a bug in commctrl.h, they don't paint under the bitmap
//     if it's the only thing in the column (or if it's the bitmap of the primary
//     item).  Also, the behavior of the listview is such that the text in
//     subitem 0 is always shifted right the width of a small bitmap.  When
//     I had the status column NOT as item 0, there were two display problems.
//     1) First, the text in column 0 was always shifted right to allow for the
//        bitmap we weren't using.  This looked funny.
//     2) The full-row-select highlight didn't properly paint the background
//        of the status bitmap.
//
//     By placing the status column as subitem 0, we eliminate problem 1 since
//     we're using a bitmap in subitem 0 (listview's default behavior).
//     If we drag the status column out of the leftmost position, they still don't
//     paint under the bitmap but at least it will work like any other explorer
//     view.  When/if they fix listview, we'll be fixed automatically.
//
const DV_COLDATA g_rgColumns[] = {
    { LVCFMT_LEFT |
      LVCFMT_COL_HAS_IMAGES,
                     0, IDS_TITLE_COL_STATUS,    DetailsView::idCol_Status      },
    { LVCFMT_LEFT,   0, IDS_TITLE_COL_FOLDER,    DetailsView::idCol_Folder      },
    { LVCFMT_LEFT,   0, IDS_TITLE_COL_USERNAME,  DetailsView::idCol_Name        },
    { LVCFMT_LEFT,   0, IDS_TITLE_COL_LOGONNAME, DetailsView::idCol_LogonName   },
    { LVCFMT_RIGHT,  0, IDS_TITLE_COL_AMTUSED,   DetailsView::idCol_AmtUsed     },
    { LVCFMT_RIGHT,  0, IDS_TITLE_COL_LIMIT,     DetailsView::idCol_Limit       },
    { LVCFMT_RIGHT,  0, IDS_TITLE_COL_THRESHOLD, DetailsView::idCol_Threshold   },
    { LVCFMT_RIGHT,  0, IDS_TITLE_COL_PCTUSED,   DetailsView::idCol_PctUsed     },
    };

//
// User quota state constants.
// used for identifying which icon to display in "Status" column.
//
const INT iUSERSTATE_OK        = 0;
const INT iUSERSTATE_WARNING   = 1;
const INT iUSERSTATE_OVERLIMIT = 2;

//
// Maximum number of entries allowed in the "Find User" MRU list.
//
const INT DetailsView::MAX_FINDMRU_ENTRIES = 10;

//
// Dimensions for the "Find User" combo box in the toolbar.
//
const INT DetailsView::CX_TOOLBAR_COMBO    = 200;
const INT DetailsView::CY_TOOLBAR_COMBO    = 200;


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::DetailsView

    Description: Class constructor.

    Arguments: None.

    Returns: Nothing.

    Exceptions: OutOfMemory

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
    02/21/97    Ownerdata listview. Added m_UserList.                BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
DetailsView::DetailsView(
    VOID
    ) : m_cRef(0),
        m_UserList(USER_LIST_GROW_AMT),
        m_hwndMain(NULL),
        m_hwndListView(NULL),
        m_hwndStatusBar(NULL),
        m_hwndToolBar(NULL),
        m_hwndListViewToolTip(NULL),
        m_hwndHeader(NULL),
        m_hKbdAccel(NULL),
        m_lpfnLVWndProc(NULL),
        m_pQuotaControl(NULL),
        m_pUserFinder(NULL),
        m_DropSource(MK_LBUTTON),
        m_DropTarget(MK_LBUTTON),
        m_pDataObject(NULL),
        m_pUndoList(NULL),
        m_ColMap(ARRAYSIZE(g_rgColumns)),
        m_strAccountUnresolved(g_hInstDll, IDS_USER_ACCOUNT_UNRESOLVED),
        m_strAccountUnavailable(g_hInstDll, IDS_USER_ACCOUNT_UNAVAILABLE),
        m_strAccountUnknown(g_hInstDll, IDS_USER_ACCOUNT_UNKNOWN),
        m_strAccountDeleted(g_hInstDll, IDS_USER_ACCOUNT_DELETED),
        m_strAccountInvalid(g_hInstDll, IDS_USER_ACCOUNT_INVALID),
        m_strNoLimit(g_hInstDll, IDS_NO_LIMIT),
        m_strNotApplicable(g_hInstDll, IDS_NOT_APPLICABLE),
        m_strStatusOK(g_hInstDll, IDS_STATUS_OK),
        m_strStatusWarning(g_hInstDll, IDS_STATUS_WARNING),
        m_strStatusOverlimit(g_hInstDll, IDS_STATUS_OVERLIMIT),
        m_pIDataObjectOnClipboard(NULL),
        m_dwEventCookie(0),
        m_iLastItemHit(-1),
        m_iLastColSorted(-1),
        m_fSortDirection(0),
        m_bMenuActive(FALSE),
        m_bWaitCursor(FALSE),
        m_bStopLoadingObjects(FALSE),
        m_bDestroyingView(FALSE)
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("DetailsView::DetailsView")));

    //
    // Make sure the idCol_XXX constants agree
    // with the size of g_rgColumns.
    //
    DBGASSERT((ARRAYSIZE(g_rgColumns) == DetailsView::idCol_Last));

    ZeroMemory(&m_lvsi, sizeof(m_lvsi));
    m_ptMouse.x     = 0;
    m_ptMouse.y     = 0;

    InitializeCriticalSection(&m_csAsyncUpdate);
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::Initialize

    Description: Initializes a new details view object.

    Arguments:
        idVolume - Ref to a const CVolumeID object containing both the
            parsable and displayable names for the volume.

    Returns: TRUE  = Success.
             FALSE = Out of memory or couldn't create thread.
                     Either way, we can't run the view.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    12/06/96    Initial creation.  Moved this code out of the ctor.  BrianAu
    02/25/97    Removed m_hwndPropPage from DetailsView.             BrianAu
    05/20/97    Added user finder object.                            BrianAu
    06/28/98    Added support for mounted volumes.                   BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
DetailsView::Initialize(
    const CVolumeID& idVolume
    )
{
    BOOL bResult   = FALSE;
    HANDLE hThread = NULL;

    try
    {
        //
        // Create the reg parameter objects we'll be using in the UI.
        // The RegParamTable functions will not add a duplicate entry.
        //
        //
        // Parameter: Preferences
        //
        LV_STATE_INFO lvsi;
        InitLVStateInfo(&lvsi);

        //
        // Create a private copy of the file sys object name string.
        // This can throw OutOfMemory.
        //
        m_idVolume = idVolume;
        if (FAILED(CreateVolumeDisplayName(m_idVolume, &m_strVolumeDisplayName)))
        {
            m_strVolumeDisplayName = m_idVolume.ForDisplay();
        }

        //
        // Read saved state of listview from registry.
        // Saved info includes window ht/wd, column widths and
        // toolbar/status bar visibility.  Need this info before we start thread.
        //
        RegKey keyPref(HKEY_CURRENT_USER, REGSTR_KEY_DISKQUOTA);
        if (FAILED(keyPref.Open(KEY_READ)) ||
            FAILED(keyPref.GetValue(REGSTR_VAL_PREFERENCES, (LPBYTE)&m_lvsi, sizeof(m_lvsi))) ||
            !DetailsView::IsValidLVStateInfo(&m_lvsi))
        {
            //
            // Protect us from truly bogus data.  If it's bad, or obsolete,
            // just re-initialize it.
            //
            DBGERROR((TEXT("Listview persist state info invalid.  Re-initializing.")));
            DetailsView::InitLVStateInfo(&m_lvsi);
        }

        //
        // Transfer sorting information to member variables.
        // These can be changed by user-initiated events.
        //
        m_iLastColSorted  = m_lvsi.iLastColSorted;
        m_fSortDirection  = m_lvsi.fSortDirection;

        //
        // Create the user finder object.
        // This is used to locate users through the toolbar combo box and
        // the "Find User" dialog.  The finder object maintains a MRU list for
        // both the toolbar and dialog combos.
        //
        m_pUserFinder = new Finder(*this, MAX_FINDMRU_ENTRIES);

        //
        // Create the data object we use to control data transfers.
        //
        m_pDataObject = new DataObject(*this);

        //
        // Create a new thread on which to run the details view window.
        // This is so that the details view will remain alive if the
        // property page is destroyed.  This must be done last in this method
        // so that if we return FALSE, the caller is assured there is no thread
        // running loose.  If we return FALSE, they'll have to call "delete"
        // to release any string allocations done above.  If we return TRUE,
        // the caller must not call delete on the object.  The object will
        // destroy itself when the user closes the view window.
        //
        hThread = CreateThread(NULL,        // No security attributes.
                               0,           // Default stack size.
                               (LPTHREAD_START_ROUTINE)&ThreadProc,
                               this,        // Static thread proc needs this.
                               0,           // Not suspended.
                               NULL);
        if (NULL != hThread)
        {
            CloseHandle(hThread);
            //
            // Everything succeeded.
            //
            bResult = TRUE;
        }
    }
    catch(CAllocException& e)
    {
        //
        // Catch an allocation exception here.
        // We'll return FALSE indicating initialization failure.
        //
    }
    return bResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::~DetailsView

    Description: Class destructor.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
    02/21/97    Ownerdata listview. Added m_UserList.                BrianAu
    05/20/97    Added user finder object.                            BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
DetailsView::~DetailsView(
    VOID
    )
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("DetailsView::~DetailsView")));

    //
    // Destroy the user object list if it still has some objects.
    //
    ReleaseObjects();

    delete m_pUserFinder;
    delete m_pUndoList;
    delete m_pDataObject;

    DeleteCriticalSection(&m_csAsyncUpdate);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::QueryInterface

    Description: Returns an interface pointer to the object's supported
        interfaces.

    Arguments:
        riid - Reference to requested interface ID.

        ppvOut - Address of interface pointer variable to accept interface ptr.

    Returns:
        NO_ERROR        - Success.
        E_NOINTERFACE   - Requested interface not supported.
        E_INVALIDARG    - ppvOut argument was NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DetailsView::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    HRESULT hResult = NO_ERROR;

    if (NULL != ppv)
    {
        *ppv = NULL;

        if (IID_IUnknown == riid || IID_IDiskQuotaEvents == riid)
        {
            *ppv = static_cast<IDiskQuotaEvents *>(this);
        }
        else if (IID_IDataObject == riid)
        {
            *ppv = static_cast<IDataObject *>(this);
        }
        else if (IID_IDropSource == riid)
        {
            *ppv = static_cast<IDropSource *>(this);
        }
        else if (IID_IDropTarget == riid)
        {
            *ppv = static_cast<IDropTarget *>(this);
        }
        else
            hResult = E_NOINTERFACE;

        if (NULL != *ppv)
        {
            ((LPUNKNOWN)*ppv)->AddRef();
        }
    }
    else
        hResult = E_INVALIDARG;

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::AddRef

    Description: Increments object reference count.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
DetailsView::AddRef(
    VOID
    )
{
    ULONG ulReturn = m_cRef + 1;

    DBGPRINT((DM_COM, DL_HIGH, TEXT("DetailsView::AddRef, 0x%08X  %d -> %d\n"),
             this, ulReturn - 1, ulReturn));

    InterlockedIncrement(&m_cRef);

    return ulReturn;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::Release

    Description: Decrements object reference count.  If count drops to 0,
        object is deleted.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
DetailsView::Release(
    VOID
    )
{
    ULONG ulReturn = m_cRef - 1;

    DBGPRINT((DM_COM, DL_HIGH, TEXT("DetailsView::Release, 0x%08X  %d -> %d\n"),
             this, ulReturn + 1, ulReturn));

    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        ulReturn = 0;
    }
    return ulReturn;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::ThreadProc

    Description: Thread procedure for the details view window.  Creates the
        quota control object and the main window.  Then it just sits
        processing messages until it receives a WM_QUIT message.

    Arguments:
        dwParam - Address of DetailsView instance.

    Returns:
        Always returns 0.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
DWORD
DetailsView::ThreadProc(
    DWORD dwParam
    )
{
    HRESULT hResult = NO_ERROR;
    DetailsView *pThis = (DetailsView *)dwParam;

    DBGPRINT((DM_VIEW, DL_HIGH, TEXT("LISTVIEW - New thread %d"), GetCurrentThreadId()));

    DBGASSERT((NULL != pThis));

    //
    // Need to ensure DLL stays loaded while this thread is active.
    //
    InterlockedIncrement(&g_cRefThisDll);
    //
    // This will keep the view object alive while the thread is alive.
    // We call Release when the thread terminates.
    //
    pThis->AddRef();

    //
    // Must call OleInitialize() for new thread.
    //
    try
    {
        if (SUCCEEDED(OleInitialize(NULL)))
        {
            //
            // Create the quota control object.
            // Why don't we just use the same quota controller as the
            // volume property page?  Good question.
            // Since we're on a separate thread, we either need a new
            // object or marshal the IDiskQuotaControl interface.
            // I chose to create a new object rather than take the
            // performance hit of the additional marshaling.  The quota
            // controller object is used heavily by the details view.
            //
            hResult = CoCreateInstance(CLSID_DiskQuotaControl,
                                       NULL,
                                       CLSCTX_INPROC_SERVER,
                                       IID_IDiskQuotaControl,
                                       (LPVOID *)&(pThis->m_pQuotaControl));

            if (SUCCEEDED(hResult))
            {
                hResult = pThis->m_pQuotaControl->Initialize(pThis->m_idVolume.ForParsing(),
                                                             TRUE); // Read-write.

                if (SUCCEEDED(hResult))
                {
                    //
                    // Create the main window.
                    //
                    hResult = pThis->CreateMainWindow();
                    if (SUCCEEDED(hResult))
                    {
                        MSG msg;
                        DBGASSERT((NULL != pThis->m_hwndMain));
                        //
                        // Place a message in the queue that the window has been
                        // created.  Now creation of the other controls can procede.
                        //
                        // It is VERY important that once we receive a WM_QUIT message,
                        // no members of the DetailsView instance are referenced.
                        // Posting WM_QUIT is the last thing done by the WM_DESTROY handler.
                        //
                        PostMessage(pThis->m_hwndMain, WM_MAINWINDOW_CREATED, 0, 0);

                        while (0 != GetMessage(&msg, NULL, 0, 0))
                        {
                            if (NULL == pThis->m_hKbdAccel ||
                               !TranslateAccelerator(pThis->m_hwndMain,
                                                     pThis->m_hKbdAccel,
                                                     &msg))
                            {
                                TranslateMessage(&msg);
                                DispatchMessage(&msg);
                            }
                        }
                    }
                }
            }
            else
            {
                DBGERROR((TEXT("LISTVIEW - OleInitialize failed for thread %d."),
                         GetCurrentThreadId()));
            }
            OleUninitialize();
        }
    }
    catch(CAllocException& e)
    {
        DiskQuotaMsgBox(GetDesktopWindow(),
                        IDS_OUTOFMEMORY,
                        IDS_TITLE_DISK_QUOTA,
                        MB_ICONERROR | MB_OK);

        pThis->CleanupAfterAbnormalTermination();
    }

    DBGPRINT((DM_VIEW, DL_HIGH, TEXT("LISTVIEW - Exit thread %d"), GetCurrentThreadId()));

    //
    // Release the view object since it's no longer required.
    // This will call the destructor.
    //
    pThis->Release();
    InterlockedDecrement(&g_cRefThisDll);
    CoFreeUnusedLibraries();

    return 0;
}




///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::CleanupAfterAbnormalTermination

    Description: Perform operations required after the thread has terminated
        abnormally.  This function assumes that the thread's message pump is
        no longer active.  Any operations performed must not generate messages
        that require processing by the thread.

        This method does almost the same things as OnDestroy().

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    12/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
DetailsView::CleanupAfterAbnormalTermination(
    VOID
    )
{
    //
    // Cancel subclassing of the listview control.
    //
    if (NULL != m_lpfnLVWndProc)
        SetWindowLongPtr(m_hwndListView, GWLP_WNDPROC, (INT_PTR)m_lpfnLVWndProc);

    DisconnectEventSink();
    //
    // NOTE:  We can't call ReleaseObjects() because that method
    //        requires an active listview.  Our thread is finished and the
    //        window is gone.
    //
    if (NULL != m_pQuotaControl)
    {
        m_pQuotaControl->Release();
        m_pQuotaControl = NULL;
    }

    //
    // If we have a data object on the clipboard, clear the clipboard.
    // Note that the clipboard holds the reference to the data object.
    // When we clear the clipboard, the data object will be released.
    //
    if (NULL != m_pIDataObjectOnClipboard &&
       S_OK == OleIsCurrentClipboard(m_pIDataObjectOnClipboard))
    {
        OleFlushClipboard();
    }
}




///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnUserNameChanged

    Description: Called by the event source (SidNameResolver) whenever a disk
        quota user object's name has changed.  The user object's folder and
        account name strings are updated in the list view.

    Arguments:
        pUser - Address of IDiskQuotaUser interface for user object that has
            a new name.

    Returns:
        NO_ERROR     - Success.
        E_INVALIDARG - User object pointer received from event source was invalid.
        E_FAIL       - User not found in listview list.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
    12/10/96    Use free-threading OLE apartment model.              BrianAu
    02/05/98    Changed ListView_RedrawItems to use                  BrianAu
                SendMessageTimeout().

*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DetailsView::OnUserNameChanged(
    PDISKQUOTA_USER pUser
    )
{
    HRESULT hResult = E_FAIL;

    //
    // Ensure the DetailsView object stay's alive while the view is updated.
    // Remember, this code is being run on the SID/Name resolver's thread.
    //
    AddRef();

    //
    // We don't want to perform a user-name-changed update if the
    // view is being or has been destroyed.  Likewise, we don't want to
    // destroy the view window while a user-name-changed update is in
    // progress.  The crit sec m_csAsyncUpdate and the flag m_bDestroyingView
    // work together to ensure this.
    //
    EnterCriticalSection(&m_csAsyncUpdate);
    if (!m_bDestroyingView)
    {
        try
        {
            if (NULL != pUser)
            {
                INT iItem = -1;

                if (m_UserList.FindIndex((LPVOID)pUser, &iItem))
                {
                    //
                    // Send message to listview to redraw the item
                    // that changed.  Use the "timeout" version of
                    // SendMessage because the main window thread
                    // could be blocked waiting for m_csAsyncUpdate
                    // which is now owned by the resolver thread.
                    // If the main thread is blocked (waiting to
                    // process WM_DESTROY), this call will return 0
                    // after 5 seconds.  If this happens, we leave the
                    // CS without generating a window update, releasing
                    // the CS and letting the main window thread continue
                    // with WM_DESTROY processing.
                    //
                    DWORD_PTR dwResult;
                    LRESULT lResult = SendMessageTimeout(m_hwndListView,
                                                         LVM_REDRAWITEMS,
                                                         (WPARAM)iItem,
                                                         (LPARAM)iItem,
                                                         SMTO_BLOCK,
                                                         5000,
                                                         &dwResult);
                    if (lResult)
                        UpdateWindow(m_hwndListView);
                    else
                        DBGERROR((TEXT("ListView update timed out after 5 seconds")));

                    hResult = NO_ERROR;
                }
            }
            else
                hResult = E_INVALIDARG;
        }
        catch(CAllocException& e)
        {
            //
            // Catch allocation exceptions and do nothing.
            // Resolver doesn't care about return value.
            // Want to ensure that Release() is called no matter what.
            //
            hResult = E_OUTOFMEMORY;
        }
    }
    LeaveCriticalSection(&m_csAsyncUpdate);

    Release();

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::CreateMainWindow

    Description: Creates the main window for the details view.

    Arguments: None.

    Returns:
        NO_ERROR    - Success.
        E_FAIL      - Couldn't create window.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::CreateMainWindow(
    VOID
    )
{
    HRESULT hResult = NO_ERROR;
    WNDCLASSEX wc;
    DWORD dwExStyle;
    LANGID LangID;

    wc.cbSize           = sizeof(WNDCLASSEX);
    wc.style            = CS_PARENTDC;
    wc.lpfnWndProc      = WndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = g_hInstDll;
    wc.hIcon            = LoadIcon(g_hInstDll, MAKEINTRESOURCE(IDI_QUOTA));
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = NULL;
    wc.lpszMenuName     = MAKEINTRESOURCE(IDM_LISTVIEW_MENU);
    wc.lpszClassName    = c_szWndClassDetailsView;
    wc.hIconSm          = NULL;

    RegisterClassEx(&wc);

    //
    // Need to pass "this" pointer in WM_CREATE.  We'll store "this"
    // in the window's USERDATA.
    //
    WNDCREATE_DATA wcd;
    wcd.cbExtra = sizeof(WNDCREATE_DATA);
    wcd.pThis   = this;

    //
    // Create the window title string.
    // "Quota Details for My Disk (X:)"
    //
    CString strWndTitle(g_hInstDll, IDS_TITLE_MAINWINDOW, (LPCTSTR)m_strVolumeDisplayName);

    HWND hwndDesktop   = GetDesktopWindow();
    HDC hdc            = GetDC(hwndDesktop);

    //
    // Get current screen resolution.
    //
    if ((m_lvsi.cxScreen != (WORD)GetDeviceCaps(hdc, HORZRES)) ||
        (m_lvsi.cyScreen != (WORD)GetDeviceCaps(hdc, VERTRES)))
    {
        //
        // Screen resolution has changed since listview state data was
        // last saved to registry.  Use the default window ht/wd.
        //
        m_lvsi.cx = 0;
        m_lvsi.cy = 0;
    }
    ReleaseDC(hwndDesktop, hdc);


    // Check if we are running on BiDi Localized build. we need to create the Main Window 
    // mirrored (WS_EX_LAYOUTRTL).
    dwExStyle = 0;
    LangID = GetUserDefaultUILanguage();
    if( LangID )
    {
       LCID   iLCID;
       WCHAR wchLCIDFontSignature[16];
       iLCID = MAKELCID( LangID , SORT_DEFAULT );

        if( GetLocaleInfoW( iLCID ,
                           LOCALE_FONTSIGNATURE ,
                           (WCHAR *) &wchLCIDFontSignature[0] ,
                           (sizeof(wchLCIDFontSignature)/sizeof(WCHAR))) )
        {
           if( wchLCIDFontSignature[7] & (WCHAR)0x0800 )
           {
              dwExStyle = WS_EX_LAYOUTRTL;
           }
        }
    }


    m_hwndMain = CreateWindowEx(dwExStyle,
                              c_szWndClassDetailsView,
                              strWndTitle,
                              WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              m_lvsi.cx ? m_lvsi.cx : CW_USEDEFAULT,
                              m_lvsi.cy ? m_lvsi.cy : CW_USEDEFAULT,
                              hwndDesktop,
                              NULL,
                              g_hInstDll,
                              &wcd);
    if (NULL != m_hwndMain)
    {
        //
        // Register the main window as an OLE drop target.
        //
        RegisterAsDropTarget(TRUE);
    }
    else
    {
        hResult = E_FAIL;
    }

#if DBG
    if (FAILED(hResult))
        DBGERROR((TEXT("LISTVIEW - Failed creating main window."), hResult));
#endif

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::CreateListView

    Description: Create the list view control.

    Arguments: None.

    Returns:
        NO_ERROR    - Success.
        E_FAIL      - Failed to create listview or load icons.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
    02/21/97    Modified to use virtual listview (LVS_OWNERDATA)     BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::CreateListView(
    VOID
    )
{
    HRESULT hResult = NO_ERROR;
    RECT rc;

    DBGASSERT((NULL != m_hwndMain));

    GetClientRect(m_hwndMain, &rc);

    m_hwndListView = CreateWindowEx(WS_EX_CLIENTEDGE,
                                    WC_LISTVIEW,
                                    TEXT(""),
                                    WS_CHILD | WS_CLIPCHILDREN |
                                    WS_VISIBLE | WS_CLIPSIBLINGS |
                                    WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS |
                                    LVS_OWNERDATA,
                                    0, 0,
                                    rc.right - rc.left,
                                    rc.bottom - rc.top,
                                    m_hwndMain,
                                    (HMENU)NULL,
                                    g_hInstDll,
                                    NULL);
    if (NULL != m_hwndListView)
    {
        //
        // Store "this" ptr so subclass WndProc can access members.
        //
        SetWindowLongPtr(m_hwndListView, GWLP_USERDATA, (INT_PTR)this);

        //
        // We talk to the header control so save it's handle.
        //
        m_hwndHeader = ListView_GetHeader(m_hwndListView);

        //
        // Subclass the listview control so we can monitor mouse position.
        // This is used for listview tooltip management.
        //
        m_lpfnLVWndProc = (WNDPROC)GetWindowLongPtr(m_hwndListView, GWLP_WNDPROC);
        SetWindowLongPtr(m_hwndListView, GWLP_WNDPROC, (INT_PTR)LVSubClassWndProc);

        //
        // Enable listview for images in sub-item columns and full-row select.
        //
        ListView_SetExtendedListViewStyle(m_hwndListView,
                                          LVS_EX_SUBITEMIMAGES |
                                          LVS_EX_FULLROWSELECT |
                                          LVS_EX_HEADERDRAGDROP);

        //
        // Add all columns to the listview.
        // Adjust for showing/hiding the Folder column.
        //
        INT iColId = 0; // Start with 1st col.
        for (INT iSubItem = 0;
             iSubItem < (m_lvsi.fShowFolder ? DetailsView::idCol_Last : DetailsView::idCol_Last - 1);
             iSubItem++)
        {
            AddColumn(iSubItem, g_rgColumns[iColId]);
            iColId++;

            //
            // Skip over the Folder column if it's hidden.
            //
            if (!m_lvsi.fShowFolder && DetailsView::idCol_Folder == iColId)
                iColId++;
        }

        //
        // Restore column widths to where the user left them last time
        // the details view was used.
        //
        if (m_lvsi.cb == sizeof(LV_STATE_INFO))
        {
            for (UINT i = 0; i < DetailsView::idCol_Last; i++)
            {
                if (0 != m_lvsi.rgcxCol[i])
                {
                    ListView_SetColumnWidth(m_hwndListView, i, m_lvsi.rgcxCol[i]);
                }
            }
        }

        //
        // Restore the user's last column ordering.
        //
        DBGASSERT((Header_GetItemCount(m_hwndHeader) <= ARRAYSIZE(m_lvsi.rgColIndices)));

        Header_SetOrderArray(m_hwndHeader, Header_GetItemCount(m_hwndHeader),
                             m_lvsi.rgColIndices);

        //
        // Check the "Show Folder" menu item to indicate the current visibility state
        // of the Folder column.
        //
        CheckMenuItem(GetMenu(m_hwndMain),
                      IDM_VIEW_SHOWFOLDER,
                      MF_BYCOMMAND | (m_lvsi.fShowFolder ? MF_CHECKED : MF_UNCHECKED));

        //
        // Set the sensitivity of the "by Folder" item arrangement menu option.
        //
        EnableMenuItem_ArrangeByFolder(m_lvsi.fShowFolder);

        //
        // Create and activate the listview tooltip window.
        // Even though the standard listview has a tooltip window, we need more
        // control that it provides.  i.e.:  We need to be able to enable/disable
        // the tooltip as well as notify the control when a new listview item
        // has been hit.  Therefore, we need our own tooltip window.
        //
        if (SUCCEEDED(CreateListViewToolTip()))
            ActivateListViewToolTip(!m_lvsi.fShowFolder);
        else
            DBGERROR((TEXT("LISTVIEW, Failed creating tooltip window.")));

        //
        // Add WARNING and ERROR images to the listview's image list.
        // These are used for the "Status" column.
        //
        if (FAILED(hResult = AddImages()))
            DBGERROR((TEXT("LISTVIEW, Failed adding images to image list.")));
    }
    else
        hResult = E_FAIL;

#if DBG
    if (FAILED(hResult))
        DBGERROR((TEXT("LISTVIEW - Failed creating list view with result 0x%08X"),
                 hResult));
#endif

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::RemoveColumn

    Description: Removes a specified column from the list view.

    Arguments:
        iColId - 0-based index of the column in the list view.
            i.e. idCol_Folder, idCol_Name etc.

    Returns:
        NO_ERROR    - Success.
        E_FAIL      - Column could not be removed.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/06/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::RemoveColumn(
    INT iColId
    )
{
    INT iSubItem = m_ColMap.IdToSubItem(iColId);
    if (-1 != iSubItem && ListView_DeleteColumn(m_hwndListView, iSubItem))
    {
        m_ColMap.RemoveId(iSubItem);
        return NO_ERROR;
    }

    return E_FAIL;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::AddColumn

    Description: Adds a column to the list view.  The caller specifies which
        0-based position the column is to occupy and a reference to a column
        descriptor record containing information that defines the column.

    Arguments:
        iSubItem - 0-based index of the column in the list view.

        ColDesc - Reference to column descriptor record.

    Returns:
        NO_ERROR    - Success.
        E_FAIL      - One or more columns could not be inserted.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/06/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::AddColumn(
    INT iSubItem,
    const DV_COLDATA& ColDesc
    )
{
    LV_COLUMN col;
    HRESULT hResult = NO_ERROR;

    CString strColText(g_hInstDll, ColDesc.idMsgText);
    col.pszText = strColText;

    if (0 == ColDesc.cx)
    {
        //
        // No width specified in col desc record.  Size column to the title.
        //
        HDC hdc = NULL;
        TEXTMETRIC tm;

        hdc = GetDC(m_hwndListView);
        GetTextMetrics(hdc, &tm);
        ReleaseDC(m_hwndListView, hdc);
        //
        // Nothing special about the +2.  Without it, we get trailing ellipsis.
        //
        col.cx = tm.tmAveCharWidth * (lstrlen(col.pszText) + 2);
    }
    else
        col.cx = ColDesc.cx;  // Use width from col descriptor.


    col.iSubItem = iSubItem;
    col.fmt      = ColDesc.fmt;
    col.mask     = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    if (-1 != ListView_InsertColumn(m_hwndListView, iSubItem, &col))
        m_ColMap.InsertId(iSubItem, ColDesc.iColId);
    else
        hResult = E_FAIL;

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::AddImages

    Description: Adds icon images to the list view's image lists.  These
        icons are used in the status column to indicate overrun of the
        quota threshold and limit.

    Arguments: None.

    Returns:
        NO_ERROR    - Success.
        E_FAIL      - One or more icons could not be loaded.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/06/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::AddImages(
    VOID
    )
{
    HRESULT hResult         = NO_ERROR;
    HIMAGELIST hSmallImages = NULL;

    //
    // Create the image lists for the listview.
    //
    hSmallImages = ImageList_Create(BITMAP_WIDTH, BITMAP_HEIGHT, ILC_MASK, 3, 0);

    //
    // Note:  The order of these icon ID's in this array must match with the
    //        iIMAGELIST_ICON_XXXXX macros defined at the top of this file.
    //        The macro values represent the image indices in the image list.
    //
    struct IconDef
    {
        LPTSTR szName;
        HINSTANCE hInstDll;
    } rgIcons[] = {
                    { MAKEINTRESOURCE(IDI_OKBUBBLE), g_hInstDll },
                    { IDI_WARNING,                   NULL       },
                    { MAKEINTRESOURCE(IDI_WARNERR),  g_hInstDll }
                  };

    for (UINT i = 0; i < ARRAYSIZE(rgIcons) && SUCCEEDED(hResult); i++)
    {
        HICON hIcon = LoadIcon(rgIcons[i].hInstDll, rgIcons[i].szName);

        if (NULL != hIcon)
        {
            ImageList_AddIcon(hSmallImages, hIcon);
            DestroyIcon(hIcon);
        }
        else
        {
            DBGERROR((TEXT("LISTVIEW - Error loading icon")));
            hResult = E_FAIL;
        }
    }
    ImageList_SetBkColor(hSmallImages, CLR_NONE);  // Transparent background.

    ListView_SetImageList(m_hwndListView, hSmallImages, LVSIL_SMALL);

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::CreateListViewToolTip

    Description: Creates a tooltip window for displaying the user's folder
        name when the Folder column is hidden.  The entire listview
        is defined as a single tool.  We make the tooltip control think
        each listview item is a separate tool by intercepting WM_MOUSEMOVE,
        and performing a hit test to determine which listview item is hit.
        If the cursor has moved over a new item, the tooltip control is sent
        a WM_MOUSEMOVE(0,0).  The next real WM_MOUSEMOVE that we relay to the
        tooltip makes it think that it is on a new tool.  This is required
        so that tooltips popup and hide appropriately.

    Arguments: None.

    Returns:
        NO_ERROR    - Success.
        E_FAIL      - Failed to create tooltip window.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/09/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::CreateListViewToolTip(
    VOID
    )
{
    HRESULT hResult = E_FAIL;

    m_hwndListViewToolTip = CreateWindowEx(0,
                                           TOOLTIPS_CLASS,
                                           (LPTSTR)NULL,
                                           TTS_ALWAYSTIP,
                                           CW_USEDEFAULT,
                                           CW_USEDEFAULT,
                                           CW_USEDEFAULT,
                                           CW_USEDEFAULT,
                                           m_hwndListView,
                                           (HMENU)NULL,
                                           g_hInstDll,
                                           NULL);
    if (NULL != m_hwndListViewToolTip)
    {
        TOOLINFO ti;

        //
        // Set tooltip timing parameter so that it pops up after
        // 1/2 second of no-mouse-movement.
        //
        SendMessage(m_hwndListViewToolTip,
                    TTM_SETDELAYTIME,
                    TTDT_INITIAL,
                    (LPARAM)500);

        ti.cbSize      = sizeof(TOOLINFO);
        ti.uFlags      = TTF_IDISHWND;
        ti.hwnd        = m_hwndListView;
        ti.hinst       = g_hInstDll;
        ti.uId         = (UINT_PTR)m_hwndListView;  // Treat entire LV as a tool.
        ti.lpszText    = LPSTR_TEXTCALLBACK;

        if (SendMessage(m_hwndListViewToolTip,
                        TTM_ADDTOOL,
                        0,
                        (LPARAM)&ti))
        {
            hResult = NO_ERROR;
        }
    }

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::CreateStatusBar

    Description: Creates the status bar.

    Arguments: None.

    Returns:
        NO_ERROR    - Success.
        E_FAIL      - Failed to create status bar.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::CreateStatusBar(
    VOID
    )
{
    HRESULT hResult = NO_ERROR;

    DBGASSERT((NULL != m_hwndMain));
    m_hwndStatusBar = CreateWindow(STATUSCLASSNAME,
                                   TEXT(""),
                                   WS_VISIBLE | WS_CHILD | WS_BORDER | SBS_SIZEGRIP,
                                   0, 0, 0, 0,
                                   m_hwndMain,
                                   (HMENU)NULL,
                                   g_hInstDll,
                                   NULL);
    if (NULL != m_hwndStatusBar)
    {
        //
        // Show/hide status bar according to registry setting.
        //
        if (!m_lvsi.fStatusBar)
            ShowWindow(m_hwndStatusBar, SW_HIDE);

        //
        // Check the menu item to indicate the current status bar state.
        //
        CheckMenuItem(GetMenu(m_hwndMain),
                      IDM_VIEW_STATUSBAR,
                      MF_BYCOMMAND | (m_lvsi.fStatusBar ? MF_CHECKED : MF_UNCHECKED));
    }
    else
    {
        hResult = E_FAIL;
        DBGERROR((TEXT("LISTVIEW - Failed creating status bar."), hResult));
    }

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::CreateToolBar

    Description: Creates the tool bar.

    Arguments: None.

    Returns:
        NO_ERROR    - Success.
        E_FAIL      - Failed to create status bar.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
    02/26/97    Changed to flat toolbar buttons.                     BrianAu
    05/20/97    Added "Find User" button and combo box.              BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::CreateToolBar(
    VOID
    )
{
    HRESULT hResult = NO_ERROR;

    //
    // Array describing each of the tool bar buttons.
    //
    TBBUTTON rgToolBarBtns[] = {
        { STD_FILENEW,     IDM_QUOTA_NEW,        TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { STD_DELETE,      IDM_QUOTA_DELETE,     TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { STD_PROPERTIES,  IDM_QUOTA_PROPERTIES, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { STD_UNDO,        IDM_EDIT_UNDO,        TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { STD_FIND,        IDM_EDIT_FIND,        TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
        };

    DBGASSERT((NULL != m_hwndMain));

    m_hwndToolBar = CreateToolbarEx(m_hwndMain,
                                    WS_CHILD | WS_BORDER | WS_VISIBLE | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT,
                                    IDC_TOOLBAR,
                                    15,
                                    (HINSTANCE)HINST_COMMCTRL,
                                    IDB_STD_SMALL_COLOR,
                                    (LPCTBBUTTON)rgToolBarBtns,
                                    ARRAYSIZE(rgToolBarBtns),
                                    0,
                                    0,
                                    100,
                                    30,
                                    sizeof(TBBUTTON));

    if (NULL != m_hwndToolBar)
    {
        //
        // BUGBUG:  I'm creating this combo without the WS_VISIBLE
        //          attribute set.  I originally coded this to have a
        //          "find" dropdown combo in the toolbar similar to that
        //          found in MS Dev Studio.  Later we decided that this
        //          was unnecessarily redundant with the "find" dialog
        //          and it's dropdown combo.  I'm leaving the code for
        //          two reasons.
        //             1. I don't want to break the existing implementation.
        //             2. If we decide later to re-enable the feature it will
        //                be easy to reactivate.
        //
        //          [brianau - 1/20/98]
        //
        m_hwndToolbarCombo = CreateWindowEx(0,
                                            TEXT("COMBOBOX"),
                                            TEXT(""),
                                            WS_CHILD | WS_BORDER |
                                            CBS_HASSTRINGS | CBS_DROPDOWN |
                                            CBS_AUTOHSCROLL,
                                            0, 0,
                                            CX_TOOLBAR_COMBO,
                                            CY_TOOLBAR_COMBO,
                                            m_hwndToolBar,
                                            (HMENU)IDC_TOOLBAR_COMBO,
                                            g_hInstDll,
                                            NULL);
        if (NULL != m_hwndToolbarCombo)
        {
            //
            // Set the font in the toolbar combo to be the same as that
            // used in listview.  This assumes that the listview
            // has already been created.
            //
            DBGASSERT((NULL != m_hwndListView));
            HFONT hfontMain = (HFONT)SendMessage(m_hwndListView, WM_GETFONT, 0, 0);
            SendMessage(m_hwndToolbarCombo, WM_SETFONT, (WPARAM)hfontMain, 0);

            //
            // Initialize the "user finder" object so that it knows
            // how to communicate with the toolbar combo box.
            //
            m_pUserFinder->ConnectToolbarCombo(m_hwndToolbarCombo);

            //
            // Retrieve the finder's MRU list contents from the registry and
            // load the toolbar's combo box.
            // The check for cMruEntries < MAX_FINDMRU_ENTRIES is to prevent
            // this loop from running wild if someone trashes the registry entry.
            //
            RegKey keyPref(HKEY_CURRENT_USER, REGSTR_KEY_DISKQUOTA);
            if (SUCCEEDED(keyPref.Open(KEY_READ)))
            {
                CArray<CString> rgstrMRU;
                if (SUCCEEDED(keyPref.GetValue(REGSTR_VAL_FINDMRU, &rgstrMRU)))
                {
                    int n = rgstrMRU.Count();
                    for (int i = 0; i < n; i++)
                    {
                        SendMessage(m_hwndToolbarCombo,
                                    CB_ADDSTRING,
                                    0,
                                    (LPARAM)(rgstrMRU[i].Cstr()));
                    }
                }
            }
        }

        //
        // Show/hide tool bar according to registry setting.
        //
        if (!m_lvsi.fToolBar)
            ShowWindow(m_hwndToolBar, SW_HIDE);

        //
        // Check the menu item to indicate the current tool bar state.
        //
        CheckMenuItem(GetMenu(m_hwndMain),
                      IDM_VIEW_TOOLBAR,
                      MF_BYCOMMAND | (m_lvsi.fToolBar ? MF_CHECKED : MF_UNCHECKED));
        //
        // Initially, we have nothing in the undo list.
        //
        EnableMenuItem_Undo(FALSE);
    }
    else
    {
        hResult = E_FAIL;
        DBGERROR((TEXT("LISTVIEW - Failed creating tool bar."), hResult));
    }

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::LoadObjects

    Description: Loads the user object list with quota record objects.

    Arguments: None.

    Returns:
        NO_ERROR    - Success.
        E_FAIL      - Failed enumerating users or adding objects to listview.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::LoadObjects(
    VOID
    )
{
    HRESULT hResult = NO_ERROR;

    DBGASSERT((NULL != m_pQuotaControl));


    //
    // Use a user enumerator object for obtaining all of the quota users.
    //
    IEnumDiskQuotaUsers *pEnumUsers = NULL;

    hResult = m_pQuotaControl->CreateEnumUsers(
                            NULL,                             // All entries.
                            0,                                // All entries.
                            DISKQUOTA_USERNAME_RESOLVE_ASYNC, // Asynchronous operation.
                            &pEnumUsers);
    if (SUCCEEDED(hResult))
    {
        IDiskQuotaUser *pUser = NULL;
        hResult = S_OK;

        //
        // m_bStopLoadingObjects is sort of a hack so that we can interrupt
        // object loading if the user closes the view while loading is in progress.
        //
        // This is probably the most speed-critical loop in the disk quota UI.
        // The faster it is, the less time the user must wait for the listview
        // to be populated with user objects.
        //
        try
        {
            //
            // Go ahead and take a lock on the user list during the entire loading
            // process.  This will let the list locking code in m_UserList.Append
            // proceded without having to obtain the lock each time.
            //
            m_UserList.Lock();
            while(!m_bStopLoadingObjects)
            {
                DWORD cUsers = 1;

                hResult = pEnumUsers->Next(cUsers, &pUser, &cUsers);
                if (S_OK == hResult)
                {
                    m_UserList.Append(pUser);
                }
                else
                {
                    break;
                }

                pUser = NULL;
            }
            pEnumUsers->Release();  // Release the enumerator.
            m_UserList.ReleaseLock();
        }
        catch(CAllocException& e)
        {
            //
            // Clean up before re-throwing exception.
            // Leave m_UserList in the pre-exception state.
            //
            if (NULL != pUser)
                pUser->Release();
            if (NULL != pEnumUsers)
                pEnumUsers->Release();
            m_UserList.ReleaseLock();

            m_bStopLoadingObjects = FALSE;
            hResult = E_OUTOFMEMORY;
        }
    }

    if (S_FALSE == hResult)     // Means no-more-users.
        hResult = NO_ERROR;

#if DBG
    if (FAILED(hResult))
    {
        DBGERROR((TEXT("LISTVIEW - Failed loading objects.  Result = 0x%08X"),
                 hResult));
    }
#endif

    m_bStopLoadingObjects = FALSE;

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::ReleaseObjects

    Description: Releases all objects from the user object list (listview).

    Arguments: None.

    Returns: Always returns NO_ERROR.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
    02/21/97    Ownerdata listview. Added m_UserList.                BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::ReleaseObjects(
    VOID
    )
{
    //
    // Destroy the user objects in the list.
    //
    PDISKQUOTA_USER pUser = NULL;
    m_UserList.Lock();
    while(m_UserList.RemoveLast((LPVOID *)&pUser))
    {
        if (NULL != pUser)
            pUser->Release();
    }
    m_UserList.ReleaseLock();

    return NO_ERROR;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::SortObjects

    Description: Sort objects in the list view using a given column as the key.

    Arguments:
        idColumn - Number of the column (0-based) to use as the key.

        dwDirection - 0 = Ascending sort, 1 = Descending sort.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
    02/24/97    Added m_UserList.  Ownerdata listview.               BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::SortObjects(
    DWORD idColumn,
    DWORD dwDirection
    )
{
    DBGASSERT((idColumn >= 0 && idColumn < DetailsView::idCol_Last));

    CAutoWaitCursor waitcursor;
    COMPARESTRUCT cs;

    cs.idColumn    = idColumn;
    cs.dwDirection = dwDirection;
    cs.pThis       = this;

    m_UserList.Lock();
    m_UserList.Sort(CompareItems, (LPARAM)&cs);
    InvalidateRect(m_hwndListView, NULL, TRUE);
    UpdateWindow(m_hwndListView);
    m_UserList.ReleaseLock();

    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::CompareItems [static]

    Description: Compares two items from the details view.
        Note that it's a static method so there's no "this" pointer.

    Arguments:
        lParam1 - Address of first user object.

        lParam2 - Address of second user object.

        lParam3 - Address of a COMPARESTRUCT structure.

    Returns:
        < 0 = User 1 is "less than" user 2.
          0 = Users are "equivalent".
        > 0 = User 1 is "greater than" user 2.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
    09/05/96    Added domain name string.                            BrianAu
    05/19/97    Fixed overflow in difference calculations.           BrianAu
                Changed type of "diff" from INT to __int64.
    07/18/97    Use CompareString for name comparisons.              BrianAu
                Need to be locale-sensitive.
*/
///////////////////////////////////////////////////////////////////////////////
INT
DetailsView::CompareItems(
    LPVOID lParam1,
    LPVOID lParam2,
    LPARAM lParam3
    )
{
    INT i[2];
    __int64 diff = 0;
    PDISKQUOTA_USER pUser[2];
    LONGLONG llValue[2];
    PCOMPARESTRUCT pcs = (PCOMPARESTRUCT)lParam3;
    DBGASSERT((NULL != pcs));
    DetailsView *pThis = pcs->pThis;
    pUser[0] = (PDISKQUOTA_USER)lParam1;
    pUser[1] = (PDISKQUOTA_USER)lParam2;

    i[0] = pcs->dwDirection; // Sort direction (0 = ascending, 1 = descending)
    i[1] = i[0] ^ 1;         // Opposite of i[0].

    DBGASSERT((NULL != pUser[0]));
    DBGASSERT((NULL != pUser[1]));

    switch(pcs->idColumn)
    {
        case DetailsView::idCol_Name:
        case DetailsView::idCol_LogonName:
        case DetailsView::idCol_Folder:
        {
            DWORD dwAccountStatus[2];
            pUser[0]->GetAccountStatus(&dwAccountStatus[0]);
            pUser[1]->GetAccountStatus(&dwAccountStatus[1]);

            if (DISKQUOTA_USER_ACCOUNT_RESOLVED == dwAccountStatus[0] &&
                DISKQUOTA_USER_ACCOUNT_RESOLVED == dwAccountStatus[1])
            {
                //
                // Both users have valid logon name  strings.
                //
                INT iCompareResult;         // For CompareString.
                TCHAR szContainer[2][MAX_DOMAIN];
                TCHAR szName[2][MAX_USERNAME];
                TCHAR szLogonName[2][MAX_USERNAME];
                pUser[0]->GetName(szContainer[0], ARRAYSIZE(szContainer[0]),
                                  szLogonName[0], ARRAYSIZE(szLogonName[0]),
                                  szName[0],      ARRAYSIZE(szName[0]));

                pUser[1]->GetName(szContainer[1], ARRAYSIZE(szContainer[1]),
                                  szLogonName[1], ARRAYSIZE(szLogonName[1]),
                                  szName[1],      ARRAYSIZE(szName[1]));

                if (DetailsView::idCol_Folder == pcs->idColumn)
                {
                    //
                    // Sort by container + logon name.
                    // Use CompareString so we're locale-sensitive.
                    //
                    iCompareResult = CompareString(LOCALE_USER_DEFAULT,
                                                   NORM_IGNORECASE,
                                                   szContainer[ i[0] ], -1,
                                                   szContainer[ i[1] ], -1);
                    if (CSTR_EQUAL == iCompareResult)
                    {
                        iCompareResult = CompareString(LOCALE_USER_DEFAULT,
                                                       NORM_IGNORECASE,
                                                       szLogonName[ i[0] ], -1,
                                                       szLogonName[ i[1] ], -1);
                    }
                }
                else if (DetailsView::idCol_Name == pcs->idColumn)
                {
                    //
                    // Sort by display name + container.
                    // Use CompareString so we're locale-sensitive.
                    //
                    iCompareResult = CompareString(LOCALE_USER_DEFAULT,
                                                   NORM_IGNORECASE,
                                                   szName[ i[0] ], -1,
                                                   szName[ i[1] ], -1);

                    if (CSTR_EQUAL == iCompareResult)
                    {
                        iCompareResult = CompareString(LOCALE_USER_DEFAULT,
                                                       NORM_IGNORECASE,
                                                       szContainer[ i[0] ], -1,
                                                       szContainer[ i[1] ], -1);
                    }
                }
                else if (DetailsView::idCol_LogonName == pcs->idColumn)
                {
                    //
                    // Sort by logon name + container.
                    // Use CompareString so we're locale-sensitive.
                    //
                    iCompareResult = CompareString(LOCALE_USER_DEFAULT,
                                                   NORM_IGNORECASE,
                                                   szLogonName[ i[0] ], -1,
                                                   szLogonName[ i[1] ], -1);

                    if (CSTR_EQUAL == iCompareResult)
                    {
                        iCompareResult = CompareString(LOCALE_USER_DEFAULT,
                                                       NORM_IGNORECASE,
                                                       szContainer[ i[0] ], -1,
                                                       szContainer[ i[1] ], -1);
                    }
                }
                //
                // Convert iCompareResult [1,2,3] to [-1,0,1].
                //
                diff = iCompareResult - 2;
            }
            else
            {
                //
                // At least one of the users hasn't been or can't be resolved.
                // Compare by account status alone.  Status values are such
                // that a resolved name will sort before an unresolved name.
                // Cast to (INT) is required for proper ordering.
                //
                diff = (INT)dwAccountStatus[ i[0] ] - (INT)dwAccountStatus[ i[1] ];
            }
            break;
        }
        case DetailsView::idCol_Status:
        {
            //
            // The status image is based on the quota "state" of the user.
            // This expression effectively compares user records by status image.
            //
            diff = (pThis->GetUserQuotaState(pUser[ i[0] ]) - pThis->GetUserQuotaState(pUser[ i[1] ]));
            break;
        }
        case DetailsView::idCol_AmtUsed:
        {
            pUser[0]->GetQuotaUsed(&llValue[0]);
            pUser[1]->GetQuotaUsed(&llValue[1]);
            diff = llValue[ i[0] ] - llValue[ i[1] ];
            break;
        }
        case DetailsView::idCol_Limit:
        {
            pUser[0]->GetQuotaLimit(&llValue[0]);
            pUser[1]->GetQuotaLimit(&llValue[1]);

            if (NOLIMIT == llValue[ i[0] ])
                diff = 1;
            else if (NOLIMIT == llValue[ i[1] ])
                diff = -1;
            else
                diff = llValue[ i[0] ] - llValue[ i[1] ];
            break;
        }
        case DetailsView::idCol_Threshold:
        {
            pUser[0]->GetQuotaThreshold(&llValue[0]);
            pUser[1]->GetQuotaThreshold(&llValue[1]);

            if (NOLIMIT == llValue[ i[0] ])
                diff = 1;
            else if (NOLIMIT == llValue[ i[1] ])
                diff = -1;
            else
                diff = llValue[ i[0] ] - llValue[ i[1] ];
            break;
        }
        case DetailsView::idCol_PctUsed:
        {
            DWORD dwPct[2];
            CalcPctQuotaUsed(pUser[0], &dwPct[0]);
            CalcPctQuotaUsed(pUser[1], &dwPct[1]);
            diff = (INT)dwPct[ i[0] ] - (INT)dwPct[ i[1] ];
            break;
        }

        default:
            break;
    }

    //
    // Translate return value to -1, 0 or 1.
    //
    INT iReturn = 0;
    if (0 != diff)
    {
        if (0 < diff)
            iReturn = 1;
        else
            iReturn = -1;
    }

    return iReturn;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::WndProc

    Description: Window procedure for the details view main window.  This
        method merely dispatches the messages to other methods that do the
        actual work.  These work methods should be declared "inline".

    Arguments: Standard WndProc arguments.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK
DetailsView::WndProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    //
    // Retrieve the DetailsView object's "this" pointer from the window's
    // USERDATA.
    //
    DetailsView *pThis = (DetailsView *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    try
    {
        switch(message)
        {
            case WM_CREATE:
            {
                CREATESTRUCT *pcs = (CREATESTRUCT *)lParam;
                PWNDCREATE_DATA pCreateData = (PWNDCREATE_DATA)(pcs->lpCreateParams);
                DBGASSERT((NULL != pCreateData));

                pThis = (DetailsView *)(pCreateData->pThis);
                DBGASSERT((NULL != pThis));
                SetWindowLongPtr(hWnd, GWLP_USERDATA, (INT_PTR)pThis);

                InitCommonControls();
                return 0;
            }

            case WM_COMMAND:
                DBGASSERT((NULL != pThis));
                pThis->OnCommand(hWnd, message, wParam, lParam);
                //
                // Enable the "Undo" menu if the undo list is not empty.
                //
                pThis->EnableMenuItem_Undo(0 != pThis->m_pUndoList->Count());
                return 0;

            case WM_CONTEXTMENU:
                DBGASSERT((NULL != pThis));
                pThis->OnContextMenu(hWnd, message, wParam, lParam);
                return 0;

            case WM_CLOSE:
            case WM_ENDSESSION:
                DestroyWindow(hWnd);
                return 0;

            case WM_DESTROY:
                DBGASSERT((NULL != pThis));
                pThis->OnDestroy(hWnd, message, wParam, lParam);
                return 0;

            case WM_ADD_USER_TO_DETAILS_VIEW:  // This is DSKQUOTA-specific.
                DBGASSERT((NULL != pThis));
                pThis->AddUser((PDISKQUOTA_USER)lParam);
                return 0;

            case WM_MAINWINDOW_CREATED:  // This is DSKQUOTA-specific.
                DBGASSERT((NULL != pThis));
                pThis->OnMainWindowCreated(hWnd, message, wParam, lParam);
                return 0;

            case WM_MENUSELECT:
                DBGASSERT((NULL != pThis));
                pThis->OnMenuSelect(hWnd, message, wParam, lParam);
                return 0;

            case WM_NOTIFY:
                DBGASSERT((NULL != pThis));
                pThis->OnNotify(hWnd, message, wParam, lParam);
                return 0;

            case WM_SETFOCUS:
                DBGASSERT((NULL != pThis));
                pThis->OnSetFocus(hWnd, message, wParam, lParam);
                return 0;

            case WM_SIZE:
                DBGASSERT((NULL != pThis));
                pThis->OnSize(hWnd, message, wParam, lParam);
                return 0;

            case WM_SYSCOLORCHANGE:
            case WM_SETTINGCHANGE:
                DBGASSERT((NULL != pThis));
                pThis->OnSettingChange(hWnd, message, wParam, lParam);
                return 0;

            default:
                break;
        }
    }
    catch(CAllocException& e)
    {
        //
        // Handle out-of-memory errors here.  Any other exceptions
        // can be thrown to caller.  Let ThreadProc handle them.
        //
        DiskQuotaMsgBox(GetDesktopWindow(),
                        IDS_OUTOFMEMORY,
                        IDS_TITLE_DISK_QUOTA,
                        MB_ICONERROR | MB_OK);
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::LVSubClassWndProc

    Description: Window proc for the sub-classed listview control.
        This is required so that we can intecept mouse messages and respond
        to the request for tooltip text.

    Arguments: Std windows WndProc args.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/09/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK
DetailsView::LVSubClassWndProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    DetailsView *pThis = (DetailsView *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch(message)
    {
        case WM_NOTIFY:
            {
                //
                // Only return ToolTip text if TTN_NEEDTEXT is being
                // sent from our tooltip.  Don't respond to the listview's
                // internal tooltip's request for text.
                //
                LV_DISPINFO *pDispInfo  = (LV_DISPINFO *)lParam;
                if (pDispInfo->hdr.hwndFrom == pThis->m_hwndListViewToolTip)
                {
                    switch(pDispInfo->hdr.code)
                    {
                        case TTN_NEEDTEXT:
//
// BUGBUG:  With the removal of the "domain" term from the UI, I
//          decided we don't need this tooltip any more.
//          However, I'm making this change in the last hour before
//          "code complete" and I don't want to break something else.
//          Therefore I'm just commenting this out and leaving the
//          subclassing in place.  If there's time later, this subclassing
//          of the listview should be removed. [brianau - 03/19/98]
//
//                            pThis->LV_OnTTN_NeedText((TOOLTIPTEXT *)lParam);
                            return 0;

                        default:
                            break;
                    }
                }
            }
            break;

        case WM_MOUSEMOVE:
            DBGASSERT((NULL != pThis));
            pThis->LV_OnMouseMessages(hWnd, message, wParam, lParam);
            break;

        case WM_ADD_USER_TO_DETAILS_VIEW:  // This is DSKQUOTA-specific.
            DBGASSERT((NULL != pThis));
            pThis->AddUser((PDISKQUOTA_USER)lParam);
            break;

        default:
            break;
    }
    DBGASSERT((NULL != pThis->m_lpfnLVWndProc));
    return CallWindowProc(pThis->m_lpfnLVWndProc, hWnd, message, wParam, lParam);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnCommand

    Description: Handler for WM_COMMAND.

    Arguments: Standard WndProc arguments.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
    09/06/96    Added "Show Domain" menu option.                     BrianAu
    05/20/97    Added IDM_EDIT_FIND and IDM_EDIT_FIND_LIST.          BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnCommand(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch(LOWORD(wParam))
    {
        case IDM_EDIT_INVERTSELECTION:
            InvertSelectedItems();
            break;

        case IDM_EDIT_UNDO:
            OnCmdUndo();
            break;

        case IDM_EDIT_FIND:
            OnCmdFind();
            break;

        case IDM_EDIT_FIND_LIST:
            SetFocus(m_hwndToolbarCombo);
            break;

        case IDM_EDIT_SELECTALL:
            SelectAllItems();
            break;

        case IDM_EDIT_COPY:
            OnCmdEditCopy();
            break;

        case IDM_HELP_ABOUT:
            OnHelpAbout(hWnd);
            break;

        case IDM_HELP_TOPICS:
            OnHelpTopics(hWnd);
            break;

        case IDM_QUOTA_CLOSE:
            DestroyWindow(m_hwndMain);
            break;

        case IDM_QUOTA_DELETE:
            OnCmdDelete();
            FocusOnSomething();      // Needed if DEL key pressed.
            break;

        case IDM_QUOTA_NEW:
            OnCmdNew();
            break;

        case IDM_QUOTA_PROPERTIES:
            OnCmdProperties();
            break;

        case IDM_QUOTA_IMPORT:
            OnCmdImport();
            break;

        case IDM_QUOTA_EXPORT:
            OnCmdExport();
            break;

        case IDM_VIEW_ARRANGE_BYFOLDER:
            SortObjects(DetailsView::idCol_Folder, m_fSortDirection);
            break;

        case IDM_VIEW_ARRANGE_BYLIMIT:
            SortObjects(DetailsView::idCol_Limit, m_fSortDirection);
            break;

        case IDM_VIEW_ARRANGE_BYNAME:
            SortObjects(DetailsView::idCol_Name, m_fSortDirection);
            break;

        case IDM_VIEW_ARRANGE_BYLOGONNAME:
            SortObjects(DetailsView::idCol_LogonName, m_fSortDirection);
            break;

        case IDM_VIEW_ARRANGE_BYPERCENT:
            SortObjects(DetailsView::idCol_PctUsed, m_fSortDirection);
            break;

        case IDM_VIEW_ARRANGE_BYTHRESHOLD:
            SortObjects(DetailsView::idCol_Threshold, m_fSortDirection);
            break;

        case IDM_VIEW_ARRANGE_BYSTATUS:
            SortObjects(DetailsView::idCol_Status, m_fSortDirection);
            break;

        case IDM_VIEW_ARRANGE_BYUSED:
            SortObjects(DetailsView::idCol_AmtUsed, m_fSortDirection);
            break;

        case IDM_VIEW_REFRESH:
            Refresh(true);
            break;

        case IDM_VIEW_STATUSBAR:
            OnCmdViewStatusBar();
            break;

        case IDM_VIEW_TOOLBAR:
            OnCmdViewToolBar();
            break;
        case IDM_VIEW_SHOWFOLDER:
            OnCmdViewShowFolder();
            break;
//
// These are just for development.
//
//      case IDM_DEBUG_DUMP:
//          g_pSidCache->Dump();
//          break;
//
//      case IDM_CLEAR_CACHE:
//          m_pQuotaControl->InvalidateSidNameCache();
//          break;

        default:
            break;
    }
    return 0;
}


LRESULT
DetailsView::OnSettingChange(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HWND rghwnd[] = { m_hwndListView,
                      m_hwndStatusBar,
                      m_hwndToolBar,
                      m_hwndToolbarCombo,
                      m_hwndListViewToolTip,
                      m_hwndHeader };

    for (int i = 0; i < ARRAYSIZE(rghwnd); i++)
    {
        SendMessage(rghwnd[i], uMsg, wParam, lParam);
    }
    return 0;
}


//
// Is an x,y screen position in the LV header control?
//
BOOL
DetailsView::HitTestHeader(
    int xPos,
    int yPos
    )
{
    RECT rcHdr;
    POINT pt = { xPos, yPos };

    GetWindowRect(m_hwndHeader, &rcHdr);
    return PtInRect(&rcHdr, pt);
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnContextMenu

    Description: Handler for WM_CONTEXTMENU.
        Creates and tracks a popup context menu for deleting
        selected object(s) and showing their properties.


    Arguments: Standard WndProc arguments.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnContextMenu(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    //
    // Only display menu if the message is from the list view and there's
    // one or more objects selected in the list view.
    //
    if ((HWND)wParam == m_hwndListView &&
        !HitTestHeader(LOWORD(lParam), HIWORD(lParam)) &&
        ListView_GetSelectedCount(m_hwndListView) > 0)
    {
        HMENU hMenu = LoadMenu(g_hInstDll, MAKEINTRESOURCE(IDM_CONTEXT_MENU));
        if (NULL != hMenu)
        {
            HMENU hMenuTrackPopup = GetSubMenu(hMenu, 0);

            SetMenuDefaultItem(hMenuTrackPopup, IDM_QUOTA_PROPERTIES, MF_BYCOMMAND);

            if (LPARAM(-1) == lParam)
            {
                //
                // Invoked from keyboard.  Place menu at focused item.
                //
                POINT pt = { -1, -1 };
                int i = ListView_GetNextItem(m_hwndListView, -1, LVNI_FOCUSED);
                if (i != -1)
                {
                    ListView_GetItemPosition(m_hwndListView, i, &pt);
                    ClientToScreen(m_hwndListView, &pt);
                }

                lParam = MAKELPARAM(pt.x, pt.y);
            }
            if (LPARAM(-1) != lParam)
            {
                TrackPopupMenu(hMenuTrackPopup,
                               TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                               LOWORD(lParam),
                               HIWORD(lParam),
                               0,
                               hWnd,
                               NULL);
            }
        }
        DestroyMenu(hMenu);
    }

    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnDestroy

    Description: Handler for WM_DESTROY.

    Arguments: Standard WndProc arguments.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnDestroy(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    //
    // We don't want to destroy the view window while a user-name-changed
    // update is in progress.  Likewise, we don't want to perform a name
    // update if the view is being (or has been) destroyed.  The crit sec
    // m_csAsyncUpdate and the flag m_bDestroyingView work together to
    // ensure this.
    //
    EnterCriticalSection(&m_csAsyncUpdate);

    m_bDestroyingView     = TRUE;  // Destruction of DetailsView in progress.
    m_bStopLoadingObjects = TRUE;  // Will terminate in-progress loading.

    //
    // Unregister the main window as an OLE drop target.
    //
    if (NULL != hWnd)
    {
        RegisterAsDropTarget(FALSE);
    }

    //
    // Cancel subclassing of the listview control.
    //
    if (NULL != m_lpfnLVWndProc)
        SetWindowLongPtr(m_hwndListView, GWLP_WNDPROC, (INT_PTR)m_lpfnLVWndProc);

    DisconnectEventSink();

    if (NULL != m_pQuotaControl)
    {
        m_pQuotaControl->Release();
        m_pQuotaControl = NULL;
    }

    //
    // Save the view dimensions and column widths to the registry.
    // We want the user to be able to configure the view and leave it.
    //
    SaveViewStateToRegistry();

    //
    // If we have a data object on the clipboard, clear the clipboard.
    // Note that the clipboard holds the reference to the data object.
    // When we clear the clipboard, the data object will be released.
    //
    if (NULL != m_pIDataObjectOnClipboard &&
       S_OK == OleIsCurrentClipboard(m_pIDataObjectOnClipboard))
    {
        OleFlushClipboard();
    }

    //
    // All done now.  Post a WM_QUIT message to the thread to tell
    // it to exit.  On termination, the thread proc will release
    // the view object, calling the destructor.
    //
    PostMessage(hWnd, WM_QUIT, 0, 0);

    LeaveCriticalSection(&m_csAsyncUpdate);
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::RegisterAsDropTarget

    Description: Registers or De-Registers the details view window as
        an OLE drop target.

    Arguments:
        bActive - If TRUE, registers as a drop target.
                  If FALSE, un-registers as a drop target.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
DetailsView::RegisterAsDropTarget(
    BOOL bActive
    )
{
    if (bActive)
    {
        //
        // Register as a drop target.
        //
        CoLockObjectExternal(static_cast<IDropTarget *>(this), TRUE, FALSE);
        RegisterDragDrop(m_hwndMain, static_cast<IDropTarget *>(this));
    }
    else
    {
        //
        // Un-register as a drop target.
        //
        RevokeDragDrop(m_hwndMain);
        CoLockObjectExternal(static_cast<IDropTarget *>(this), FALSE, TRUE);
    }
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::SaveViewStateToRegistry

    Description: Saves the listview height/width and the column widths to
        the registry.  When the listview is created, these values are used
        to size it so that the user doesn't always have to re-configure the
        view every time they open it.  Also saves the visibility state of the
        toolbar, statusbar and folder column.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/25/96    Initial creation.                                    BrianAu
    05/20/97    Added FindMRU list to persistent reg data.           BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
DetailsView::SaveViewStateToRegistry(
    VOID
    )
{
    RECT rc;
    HDC hdc = GetDC(m_hwndMain);

    RegKey keyPref(HKEY_CURRENT_USER, REGSTR_KEY_DISKQUOTA);
    if (FAILED(keyPref.Open(KEY_WRITE, true)))
    {
        DBGERROR((TEXT("Error opening reg key \"%s\""), REGSTR_KEY_DISKQUOTA));
        return;
    }

    m_lvsi.cb = sizeof(LV_STATE_INFO);

    //
    // Save current screen resolution.
    //
    m_lvsi.cxScreen = (WORD)GetDeviceCaps(hdc, HORZRES);
    m_lvsi.cyScreen = (WORD)GetDeviceCaps(hdc, VERTRES);
    ReleaseDC(m_hwndMain, hdc);

    //
    // Save current listview window size.
    //
    GetWindowRect(m_hwndMain, &rc);
    m_lvsi.cx = rc.right - rc.left;
    m_lvsi.cy = rc.bottom - rc.top;

    //
    // Save listview column widths.
    //
    UINT cColumns = Header_GetItemCount(m_hwndHeader);
    for (UINT i = 0; i < cColumns; i++)
    {
        m_lvsi.rgcxCol[i] = ListView_GetColumnWidth(m_hwndListView, i);
    }

    //
    // Save the current order of the columns in the listview.
    //
    DBGASSERT(cColumns <=  ARRAYSIZE(m_lvsi.rgColIndices));

    Header_GetOrderArray(m_hwndHeader, cColumns, m_lvsi.rgColIndices);

    //
    // Save column sorting state.
    // Casts are because we use a WORD bit field in the LVSI structure.
    //
    m_lvsi.iLastColSorted = (WORD)(m_iLastColSorted & 0xF);  // Uses only lower 4 bits.
    m_lvsi.fSortDirection = (WORD)m_fSortDirection;

    //
    // Write preference data to registry.
    //
    keyPref.SetValue(REGSTR_VAL_PREFERENCES, (LPBYTE)&m_lvsi, m_lvsi.cb);

    //
    // Save the contents of the Find MRU list.
    //
    UINT cNames = (UINT)SendMessage(m_hwndToolbarCombo, CB_GETCOUNT, 0, 0);
    if (CB_ERR != cNames && 0 < cNames)
    {
        CArray<CString> rgstrNames(cNames);
        for (i = 0; i < cNames; i++)
        {
            INT cchName = (INT)SendMessage(m_hwndToolbarCombo, CB_GETLBTEXTLEN, i, 0);
            if (CB_ERR != cchName && 0 < cchName)
            {
                CString s;
                cchName = (INT)SendMessage(m_hwndToolbarCombo, CB_GETLBTEXT, i, (LPARAM)s.GetBuffer(cchName + 1));
                s.ReleaseBuffer();
                if (CB_ERR != cchName)
                {
                    rgstrNames[i] = s;
                }
            }
        }
        keyPref.SetValue(REGSTR_VAL_FINDMRU, rgstrNames);
    }
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnMainWindowCreated

    Description: Handles WM_MAIN_WINDOW_CREATED.
        This message is posted by ThreadProc after main window creation is
        complete.  It does all the stuff to get the window up and running.

    Arguments: Standard WndProc arguments.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnMainWindowCreated(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    DBGASSERT((NULL != m_hwndMain));

    //
    // The accelerator table is automatically freed by Windows
    // when the app terminates.
    //
    m_hKbdAccel = LoadAccelerators(g_hInstDll,
                                   MAKEINTRESOURCE(IDR_KBDACCEL));

    CreateListView();
    CreateStatusBar();
    CreateToolBar();
    ConnectEventSink();
    ShowWindow(m_hwndMain, SW_SHOWNORMAL);
    UpdateWindow(m_hwndMain);
    //
    // Create the UNDO object.
    //
    m_pUndoList = new UndoList(&m_UserList, m_hwndListView);

    ShowItemCountInStatusBar();
    Refresh();
    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnMenuSelect

    Description: Handles WM_MENUSELECT.
        If a menu item is currently selected AND the status bar is visible,
        the menu item's description is displayed in the status bar.  When
        the menu is closed, the status bar reverts back to an item count.

    Arguments: Standard WndProc arguments.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnMenuSelect(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    if (0xFFFF == HIWORD(wParam) && NULL == (HMENU)lParam)
    {
        //
        // Menu closed.
        //
        m_bMenuActive = FALSE;
        ShowItemCountInStatusBar();
    }
    else
    {
        //
        // Item selected.
        //
        m_bMenuActive = TRUE;
        ShowMenuTextInStatusBar(LOWORD(wParam));
    }
    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnNotify

    Description: Handles all LVN_XXXXXX list view control notifications.
        Dispatches specific notifications to other handlers.

    Arguments: Standard WndProc arguments.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnNotify(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    NMHDR *pnmhdr = (NMHDR *)lParam;

    switch(pnmhdr->code)
    {
        case NM_DBLCLK:
        case NM_RETURN:
            OnCmdProperties( );
            //
            // Enable/disable Undo menu item.
            //
            EnableMenuItem_Undo(0 != m_pUndoList->Count());
            break;

        case NM_SETFOCUS:
            FocusOnSomething(); // Something should always be highlighted.
            break;

        case LVN_ODFINDITEM:
            OnLVN_OwnerDataFindItem((NMLVFINDITEM *)lParam);
            break;

        case LVN_GETDISPINFO:
            OnLVN_GetDispInfo((LV_DISPINFO *)lParam);
            break;

        case LVN_BEGINDRAG:
            OnLVN_BeginDrag((NM_LISTVIEW *)lParam);
            break;

        case LVN_COLUMNCLICK:
            OnLVN_ColumnClick((NM_LISTVIEW *)lParam);
            break;

        case LVN_ITEMCHANGED:
            OnLVN_ItemChanged((NM_LISTVIEW *)lParam);
            break;

        case TTN_NEEDTEXT:
            OnTTN_NeedText((TOOLTIPTEXT *)lParam);
            //
            // Fall through.
            //

        default:
            break;
    }

    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::LV_OnMouseMessages

    Description: Handles mouse messages for the subclassed listview control.
        These must be intercepted so that we can...

        a) Tell the tooltip when we've hit another listview item.
    and b) Forward all mouse messages to the tooltip window.

    Arguments: Standard WndProc args.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/09/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::LV_OnMouseMessages(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch(message)
    {
        case WM_MOUSEMOVE:
            {
                //
                // If we've moved the mouse to another listview item,
                // make the tooltip window think we're over another tool.
                // The tooltip window thinks the entire listview is a single
                // tool but we want to treat each item as a separate tool.
                // Note that m_ptMouse.x and .y are recorded when the main
                // window receives WM_MOUSEMOVE.
                //
                LV_HITTESTINFO hti;
                INT iItem = 0;

                hti.pt.x = m_ptMouse.x = LOWORD(lParam);
                hti.pt.y = m_ptMouse.y = HIWORD(lParam);

                if (-1 != (iItem = ListView_HitTest(m_hwndListView, &hti)))
                {
                    if (iItem != m_iLastItemHit)
                    {
                        SendMessage(m_hwndListViewToolTip, WM_MOUSEMOVE, 0, 0);
                        m_iLastItemHit = iItem;
                    }
                }
                else
                {
                    ShowWindow(m_hwndListViewToolTip, SW_HIDE);
                    m_iLastItemHit = iItem;
                }
            }

            //
            // Fall through.
            //
        case WM_LBUTTONUP:
        case WM_LBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_RBUTTONDOWN:
            {
                //
                // Relay all mouse messages to the listview's tooltip control.
                //
                MSG msg;
                msg.hwnd    = hWnd;
                msg.message = message;
                msg.wParam  = wParam;
                msg.lParam  = lParam;

                SendMessage(m_hwndListViewToolTip, TTM_RELAYEVENT, 0, (LPARAM)&msg);
            }
            break;

        default:
            break;
    }
    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnCmdEditCopy

    Description: Handles WM_COMMAND, IDM_EDIT_COPY.  This is invoked whenever
        a user selects the Copy menu item or presses Ctrl + C.  The method
        creates a DataObject (same used in drag-drop) and places it on the
        OLE clipboard.  The data is rendered when OLE asks for it via
        IDataObject::GetData.

    Arguments: None.

    Returns: Always 0.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/09/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnCmdEditCopy(
    VOID
    )
{
    HRESULT hResult = NO_ERROR;

    hResult = QueryInterface(IID_IDataObject, (LPVOID *)&m_pIDataObjectOnClipboard);
    if (SUCCEEDED(hResult))
    {
        OleSetClipboard(m_pIDataObjectOnClipboard);
        //
        // OLE calls AddRef() so we can release the count added in QI.
        //
        m_pIDataObjectOnClipboard->Release();
    }
    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnLVN_BeginDrag

    Description: Called when the user has selected one or more items in the
        listview and begins a drag operation.  Creates a DropSource object
        and a DataObject then calls DoDragDrop() to execute the drag-drop
        operation.

        Note that we don't store the IDataObject pointer in
        m_IDataObjectOnClipboard.  That member is only for clipboard copy
        operations, not drag/drop.

    Arguments:
        pNm - Address of notification message structure.

    Returns: TRUE  = Succeeded.
             FALSE = Failed.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/09/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnLVN_BeginDrag(
    NM_LISTVIEW *pNm
    )
{
    DBGTRACE((DM_DRAGDROP, DL_HIGH, TEXT("DetailsView::OnLVN_BeginDragDrop")));
    HRESULT hResult           = NO_ERROR;
    IDataObject *pIDataObject = NULL;
    IDropSource *pIDropSource = NULL;

    DBGPRINT((DM_DRAGDROP, DL_HIGH, TEXT("DRAGDROP - Beginning Drag/Drop")));
    try
    {
        hResult = QueryInterface(IID_IDataObject, (LPVOID *)&pIDataObject);
        if (SUCCEEDED(hResult))
        {
            hResult = QueryInterface(IID_IDropSource, (LPVOID *)&pIDropSource);
            if (SUCCEEDED(hResult))
            {
                DWORD dwEffect = 0;

                //
                // Unregister our window as a drop target while we're acting as a drop
                // source.  Don't want to drop our own data onto our own window.
                //
                RegisterAsDropTarget(FALSE);

                hResult = DoDragDrop(pIDataObject,
                                     pIDropSource,
                                     DROPEFFECT_COPY | DROPEFFECT_MOVE,
                                     &dwEffect);

                //
                // BUGBUG:  Should probably display some error UI here.
                //          The shell doesn't indicate any error if the
                //          destination volume is full or if there's a write
                //          error.  The only indication of a failure is that
                //          dwEffect will contain 0.  We could display something
                //          like "An error occured while transferring the selected
                //          items."  The big problem is that only the shell knows
                //          where the data was stored so only it could delete
                //          the created file.  Displaying a message but leaving
                //          the file is also confusing. [brianau 7/29/97]
                //          NT Bug 96282 will fix the shell not deleting the file.
                //
                RegisterAsDropTarget(TRUE);

                DBGPRINT((TEXT("DRAGDROP - Drag/Drop complete.\n\t hResult = 0x%08X  Effect = 0x%08X"),
                         hResult, dwEffect));

                pIDropSource->Release();
                pIDropSource = NULL;
            }
            pIDataObject->Release();
            pIDataObject = NULL;
        }
    }
    catch(CAllocException& e)
    {
        if (NULL != pIDropSource)
            pIDropSource->Release();
        if (NULL != pIDataObject)
            pIDataObject->Release();
        RegisterAsDropTarget(TRUE);
        throw;
    }
    return SUCCEEDED(hResult);
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnTTN_NeedText

    Description: Handles requests for tooltip text for the main window's tool
        bar buttons.

    Arguments:
        pToolTipText - Address of tooltip text notification information.

    Returns: Always returns 0.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/09/96    Initial creation.                                    BrianAu
    07/09/97    Added cmd/TT cross reference.                        BrianAu
                Previously used the tool status text in tooltip.
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnTTN_NeedText(
    TOOLTIPTEXT *pToolTipText
    )
{
    //
    // Cross-reference tool command IDs with tooltip text IDs.
    //
    const struct
    {
        UINT idCmd;  // Tool cmd ID.
        UINT idTT;   // Tooltip text ID.

    } CmdTTXRef[] = {
                        { IDM_QUOTA_NEW,        IDS_TT_QUOTA_NEW        },
                        { IDM_QUOTA_DELETE,     IDS_TT_QUOTA_DELETE     },
                        { IDM_QUOTA_PROPERTIES, IDS_TT_QUOTA_PROPERTIES },
                        { IDM_EDIT_UNDO,        IDS_TT_EDIT_UNDO        },
                        { IDM_EDIT_FIND,        IDS_TT_EDIT_FIND        }
                    };
    INT idTT = -1;
    for (INT i = 0; i < ARRAYSIZE(CmdTTXRef) && -1 == idTT; i++)
    {
        if (CmdTTXRef[i].idCmd == pToolTipText->hdr.idFrom)
            idTT = CmdTTXRef[i].idTT;
    }

    if (-1 != idTT)
    {
        m_strDispText.Format(g_hInstDll, idTT);
        pToolTipText->lpszText = (LPTSTR)m_strDispText;
    }
    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::LV_OnTTN_NeedText

    Description: Handles requests for tooltip text for the listview tooltip
        window.  This is where we get the foldername DOMAIN/USERNAME text for when the
        "domain name" column is hidden.

    Arguments:
        pToolTipText - Address of tooltip text notification information.

    Returns: Always returns 0.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/09/96    Initial creation.                                    BrianAu
    10/11/96    Added support for draggable columns.                 BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::LV_OnTTN_NeedText(
    TOOLTIPTEXT *pToolTipText
    )
{
    //
    // Only provide text when the mouse is over the "User Name" column.
    //
    if (-1 != m_iLastItemHit)
    {
        INT cxMin    = 0;
        INT cxMax    = 0;
        INT cHdrs    = Header_GetItemCount(m_hwndHeader);

        for (INT i = 0; i < cHdrs; i++)
        {
            //
            // Find the left and right X coordinate for the "Name" column.
            //
            INT iCol  = Header_OrderToIndex(m_hwndHeader, i);
            INT cxCol = ListView_GetColumnWidth(m_hwndListView, iCol);
            if (DetailsView::idCol_Name == m_ColMap.SubItemToId(iCol))
            {
                cxMax = cxMin + cxCol;
                break;
            }
            else
            {
                cxMin += cxCol;
            }
        }
        //
        // cxMin now contains left edge of Name column.
        // cxMax now contains right edge of Name column.
        //
        if (m_ptMouse.x >= cxMin && m_ptMouse.x <= cxMax)
        {
            PDISKQUOTA_USER pUser = NULL;

            if (m_UserList.Retrieve((LPVOID *)&pUser, m_iLastItemHit))
            {
                TCHAR szContainer[MAX_DOMAIN]          = { TEXT('\0') };
                TCHAR szLogonName[MAX_USERNAME]        = { TEXT('\0') };
                TCHAR szDisplayName[MAX_FULL_USERNAME] = { TEXT('\0') };

                pUser->GetName(szContainer,   ARRAYSIZE(szContainer),
                               szLogonName, ARRAYSIZE(szLogonName),
                               szDisplayName, ARRAYSIZE(szDisplayName));

                if (TEXT('\0') != szContainer[0] && TEXT('\0') != szLogonName[0])
                {
                    if (TEXT('\0') != szDisplayName[0])
                        m_strDispText.Format(g_hInstDll,
                                      IDS_FMT_DISPLAY_LOGON_CONTAINER,
                                      szDisplayName,
                                      szLogonName,
                                      szContainer);
                    else
                        m_strDispText.Format(g_hInstDll,
                                      IDS_FMT_LOGON_CONTAINER,
                                      szLogonName,
                                      szContainer);

                    pToolTipText->lpszText = (LPTSTR)m_strDispText;
                }
                else
                {
                    pToolTipText->lpszText = NULL;
                    pToolTipText->szText[0] = TEXT('\0');
                }
            }
        }
    }
    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnLVN_OwnerDataFindItem

    Description: Handles LVN_ODFINDITEM for the listview control.

    Arguments:
        pFindInfo - Address of NMLVFINDITEM structure associated with the
            notification.

    Returns: 0-based index of found item.  -1 if not found.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    02/21/97    Initial creation.  Ownerdraw listview.               BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnLVN_OwnerDataFindItem(
    NMLVFINDITEM *pFindInfo
    )
{
    INT iItem = -1;
    switch(pFindInfo->lvfi.flags)
    {
        case LVFI_PARAM:
        {
            LPVOID pvUser = NULL;

            m_UserList.Lock();
            INT cUsers = m_UserList.Count();
            for (INT i = 0; i < cUsers; i++)
            {
                if (m_UserList.Retrieve(&pvUser, i) &&
                    pvUser == (LPVOID)pFindInfo->lvfi.lParam)
                {
                    iItem = i;
                    break;
                }
            }
            m_UserList.ReleaseLock();
            break;
        }

        default:
            //
            // This app only uses lParam for locating items.
            //
            break;
    }
    return iItem;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnLVN_GetDispInfo

    Description: Handles LVN_GETDISPINFO for the listview control.

    Arguments:
        pDispInfo - Address of LV_DISPINFO structure associated with the
            notification.

    Returns: Always 0.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    02/21/97    Initial creation.  Ownerdraw listview.               BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnLVN_GetDispInfo(
    LV_DISPINFO * pDispInfo
    )
{
    PDISKQUOTA_USER pUser = NULL;

    m_UserList.Retrieve((LPVOID *)&pUser, pDispInfo->item.iItem);
    if (NULL != pUser)
    {
        if (LVIF_TEXT & pDispInfo->item.mask)
            OnLVN_GetDispInfo_Text(pDispInfo, pUser);

        if ((LVIF_IMAGE & pDispInfo->item.mask) &&
           (m_ColMap.SubItemToId(pDispInfo->item.iSubItem) == DetailsView::idCol_Status))

        {
            OnLVN_GetDispInfo_Image(pDispInfo, pUser);
        }
    }
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnLVN_GetDispInfo_Text

    Description: Handles LVN_GETDISPINFO - LVIF_TEXT for the listview control.

    Arguments:
        pDispInfo - Address of LV_DISPINFO structure associated with the
            notification.

        pUser - Address of user object for listview item.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
    09/22/96    Added user "full name" support.                      BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnLVN_GetDispInfo_Text(
    LV_DISPINFO *pDispInfo,
    PDISKQUOTA_USER pUser
    )
{
    HRESULT hResult = NO_ERROR;
    LONGLONG llValue;
    NUMBERFMT NumFmt;

    DBGASSERT((NULL != pDispInfo));
    DBGASSERT((NULL != pUser));

    NumFmt.NumDigits = 0;
    m_strDispText.Empty();

    switch(m_ColMap.SubItemToId(pDispInfo->item.iSubItem))
    {
        case DetailsView::idCol_Status:
            DBGASSERT((NULL != pUser));
            switch(GetUserQuotaState(pUser))
            {
                case iUSERSTATE_OK:
                    m_strDispText = m_strStatusOK;
                    break;
                case iUSERSTATE_WARNING:
                    m_strDispText = m_strStatusWarning;
                    break;
                default:
                    DBGASSERT((0));
                    //
                    // Fall through.
                    //
                case iUSERSTATE_OVERLIMIT:
                    m_strDispText = m_strStatusOverlimit;
                    break;
            }
            break;

        case DetailsView::idCol_Folder:
        {
            DWORD dwAccountStatus = 0;
            DBGASSERT((NULL != pUser));
            pUser->GetAccountStatus(&dwAccountStatus);
            if (DISKQUOTA_USER_ACCOUNT_RESOLVED == dwAccountStatus)
            {
                pUser->GetName(m_strDispText.GetBuffer(MAX_PATH),
                               MAX_PATH,
                               NULL,
                               0,
                               NULL,
                               0);
            }
            else
            {
                //
                // Non-normal account status.  Leave domain column
                // blank.  Account name column will contain status information.
                //
            }
            break;
        }

        case DetailsView::idCol_Name:
        {
            DWORD dwAccountStatus = 0;
            DBGASSERT((NULL != pUser));
            pUser->GetAccountStatus(&dwAccountStatus);
            switch(dwAccountStatus)
            {
                case DISKQUOTA_USER_ACCOUNT_RESOLVED:
                    pUser->GetName(NULL,       0,
                                   NULL,       0,
                                   m_strDispText.GetBuffer(MAX_USERNAME), MAX_USERNAME);

                    m_strDispText.ReleaseBuffer();
                    break;

                case DISKQUOTA_USER_ACCOUNT_UNRESOLVED:
                    m_strDispText = m_strAccountUnresolved;
                    break;

                case DISKQUOTA_USER_ACCOUNT_UNKNOWN:
                    m_strDispText = m_strAccountUnknown;
                    break;

                case DISKQUOTA_USER_ACCOUNT_INVALID:
                    m_strDispText = m_strAccountInvalid;
                    break;

                case DISKQUOTA_USER_ACCOUNT_DELETED:
                    m_strDispText = m_strAccountDeleted;
                    break;

                case DISKQUOTA_USER_ACCOUNT_UNAVAILABLE:
                    m_strDispText = m_strAccountUnavailable;
                    break;
            }
            break;
        }

        case DetailsView::idCol_LogonName:
        {
            DBGASSERT((NULL != pUser));

            DWORD dwAccountStatus = 0;
            pUser->GetAccountStatus(&dwAccountStatus);
            if (DISKQUOTA_USER_ACCOUNT_RESOLVED == dwAccountStatus)
            {
                //
                // If the account SID has been resolved to a name,
                // display the name.
                //
                pUser->GetName(NULL,       0,
                               m_strDispText.GetBuffer(MAX_USERNAME), MAX_USERNAME,
                               NULL,       0);
                m_strDispText.ReleaseBuffer();
            }
            else
            {
                //
                // If the account SID has NOT been resolved to a name, display
                // the SID as a string.
                //
                BYTE Sid[MAX_SID_LEN];
                DWORD cchSidStr = MAX_PATH;
                if (SUCCEEDED(pUser->GetSid(Sid, ARRAYSIZE(Sid))))
                {
                    SidToString(Sid, m_strDispText.GetBuffer(cchSidStr), &cchSidStr);
                    m_strDispText.ReleaseBuffer();
                }
            }
            break;
        }

        case DetailsView::idCol_AmtUsed:
            pUser->GetQuotaUsed(&llValue);
            XBytes::FormatByteCountForDisplay(llValue,
                                              m_strDispText.GetBuffer(40), 40);
            break;

        case DetailsView::idCol_Limit:
            pUser->GetQuotaLimit(&llValue);

            if (NOLIMIT == llValue)
                m_strDispText = m_strNoLimit;
            else
                XBytes::FormatByteCountForDisplay(llValue,
                                                  m_strDispText.GetBuffer(40), 40);
            break;

        case DetailsView::idCol_Threshold:
            pUser->GetQuotaThreshold(&llValue);

            if (NOLIMIT == llValue)
                m_strDispText = m_strNoLimit;
            else
                XBytes::FormatByteCountForDisplay(llValue,
                                      m_strDispText.GetBuffer(40), 40);
            break;

        case DetailsView::idCol_PctUsed:
        {
            DWORD dwPct = 0;
            hResult = CalcPctQuotaUsed(pUser, &dwPct);

            if (SUCCEEDED(hResult))
                m_strDispText.Format(TEXT("%1!d!"), dwPct);
            else
                m_strDispText = m_strNotApplicable; // Not a number.

            break;
        }

        default:
            break;
    }
    pDispInfo->item.pszText = (LPTSTR)m_strDispText;  // Used by all text callbacks.
    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnLVN_GetDispInfo_Image

    Description: Handles LVN_GETDISPINFO - LVIF_IMAGE for the listview control.

    Arguments:
        pDispInfo - Address of LV_DISPINFO structure associated with the
            notification.

        pUser - Address of user object for listview item.


    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
    09/12/96    Added CheckMark icon.                                BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnLVN_GetDispInfo_Image(
    LV_DISPINFO *pDispInfo,
    PDISKQUOTA_USER pUser
    )
{
    switch(GetUserQuotaState(pUser))
    {
        case iUSERSTATE_OK:
            pDispInfo->item.iImage = iIMAGELIST_ICON_OK;
            break;
        case iUSERSTATE_WARNING:
            pDispInfo->item.iImage = iIMAGELIST_ICON_WARNING;
            break;
        default:
            DBGASSERT((0));
            //
            // Fall through.
            //
        case iUSERSTATE_OVERLIMIT:
            pDispInfo->item.iImage = iIMAGELIST_ICON_LIMIT;
            break;
    }
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::GetUserQuotaState

    Description: Determines which of 3 states the user's quota values place
        the user in.  This is mainly used to determine what icon to display
        in the "Status" column.  It is also used to determine what text
        to display in the "Status" column in a drag-drop report.

    Arguments:
        pUser - Address of user object for listview item.


    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/10/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT
DetailsView::GetUserQuotaState(
    PDISKQUOTA_USER pUser
    )
{
    LONGLONG llUsed;
    LONGLONG llLimit;
    INT iState = iUSERSTATE_OK;

    DBGASSERT((NULL != pUser));

    pUser->GetQuotaUsed(&llUsed);
    pUser->GetQuotaLimit(&llLimit);

    if (NOLIMIT != llLimit && llUsed > llLimit)
    {
        iState = iUSERSTATE_OVERLIMIT;
    }
    else
    {
        LONGLONG llThreshold;
        pUser->GetQuotaThreshold(&llThreshold);

        if (NOLIMIT != llThreshold && llUsed > llThreshold)
            iState = iUSERSTATE_WARNING;
    }

    return iState;
}




///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnLVN_ColumnClick

    Description: Handles LVN_COLUMNCLICK list view notifications.
        This is received when the user selects a column's label.

    Arguments:
        pNm - Address of listview notification message structure.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnLVN_ColumnClick(
    NM_LISTVIEW *pNm
    )
{
    INT idCol = m_ColMap.SubItemToId(pNm->iSubItem);

    if (idCol != m_iLastColSorted)
    {
        //
        // New column selected.  Reset to ascending sort order.
        //
        m_fSortDirection = 0;
    }
    else
    {
        //
        // Column selected more than once.  Toggle sort order.
        //
        m_fSortDirection ^= 1;
    }

    SortObjects(idCol, m_fSortDirection);

    //
    // Remember what column was selected.
    //
    m_iLastColSorted = idCol;

    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnLVN_ItemChanged

    Description: Handles LVN_ITEMCHANGED listview notifications.
        Updates the selected-item-count in the status bar.

    Arguments:
        pNm - Address of listview notification structure.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
    05/18/97    Added promotion of user object in name resolution    BrianAu
                queue.
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnLVN_ItemChanged(
    NM_LISTVIEW *pNm
    )
{
    if (LVIS_FOCUSED & pNm->uNewState)
    {
        if (!m_bMenuActive)
        {
            //
            // Only update the item count if a menu item is not active.
            // This method is called WHENEVER an item is updated.  This includes
            // asynchronous notifications following name resolution.  Without
            // this check, a menu's descriptive text can be overwritten while
            // the user is walking through menu items.
            //
            ShowItemCountInStatusBar();
        }

        PDISKQUOTA_USER pUser = NULL;
        m_UserList.Lock();
        m_UserList.Retrieve((LPVOID *)&pUser, pNm->iItem);

        if (NULL != pUser &&
            NULL != m_pQuotaControl)
        {
            DWORD dwAccountStatus = 0;
            pUser->GetAccountStatus(&dwAccountStatus);

            if (DISKQUOTA_USER_ACCOUNT_UNRESOLVED == dwAccountStatus)
            {
                //
                // If the user object hasn't been resolved yet, promote it to the
                // head of the quota controller's SID-Name resolver queue.
                // This will speed up the name resolution for this user without
                // performing a blocking operation.
                //
                m_pQuotaControl->GiveUserNameResolutionPriority(pUser);
            }
        }
        m_UserList.ReleaseLock();
    }
    else if ((0 == pNm->uNewState) || (LVIS_SELECTED & pNm->uNewState))
    {
        ShowItemCountInStatusBar();
    }

    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::SetFocus

    Description: Called whenever the main window receives focus.  Immediately
        transfers focus to the listview control.  The listview in turn
        ensures that one or more items are highlighted.

    Arguments: Std WndProc arguments.

    Returns: Always returns 0.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/09/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnSetFocus(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    if (NULL != m_hwndListView)
        SetFocus(m_hwndListView);
    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnSize

    Description: Handles WM_SIZE message.

    Arguments: Standard WndProc arguments.

    Returns: Always returns 0.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
    05/20/97    Added positioning of "Find User" combo in toolbar.   BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnSize(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    RECT rcMain;
    RECT rcListView;

    GetClientRect(hWnd, &rcMain);  // How big's the main window?

    rcListView = rcMain;

    if (m_lvsi.fToolBar)
    {
        //
        // Adjust toolbar if it's visible.
        //
        RECT rcToolBar;
        INT cyToolBar = 0;

        SendMessage(m_hwndToolBar, message, wParam, lParam);
        GetClientRect(m_hwndToolBar, &rcToolBar);

        cyToolBar = rcToolBar.bottom - rcToolBar.top;

        rcListView.top += (cyToolBar + 1);

        //
        // Position the "Find User" combo box to the immediate right of the
        // "Find" toolbar button.
        // This code assumes that the "Find" toolbar button is the right-most
        // button in the toolbar.
        //
        INT cButtons = (INT)SendMessage(m_hwndToolBar, TB_BUTTONCOUNT, 0, 0);
        if (0 < cButtons)
        {
            RECT rcButton;
            SendMessage(m_hwndToolBar, TB_GETITEMRECT, cButtons - 1, (LPARAM)&rcButton);

            RECT rcCombo;
            GetWindowRect(m_hwndToolbarCombo, &rcCombo);

            SetWindowPos(m_hwndToolbarCombo,
                         NULL,
                         rcButton.right + 1,
                         rcButton.top + 1,
                         0, 0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE);

        }
    }


    if (m_lvsi.fStatusBar)
    {
        //
        // Adjust status bar if it's visible.
        //
        RECT rcStatusBar;
        INT cyStatusBar = 0;

        SendMessage(m_hwndStatusBar, message, wParam, lParam);
        GetClientRect(m_hwndStatusBar, &rcStatusBar);

        cyStatusBar = rcStatusBar.bottom - rcStatusBar.top;

        rcListView.bottom -= cyStatusBar;
    }

    //
    // Adjust the listview.  Accounts for toolbar and status bar.
    //
    MoveWindow(m_hwndListView,
               0,
               rcListView.top,
               rcListView.right - rcListView.left,
               rcListView.bottom - rcListView.top,
               TRUE);

    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::SelectAllItems

    Description: Highlights all items in the listview for selection.

    Arguments: None.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::SelectAllItems(
    VOID
    )
{
    INT cItems    = ListView_GetItemCount(m_hwndListView);
    DWORD dwState = 0;

    CAutoWaitCursor waitcursor;
    SetFocus(m_hwndListView);
    //
    // This isn't documented but it's the way the shell does it for DefView.
    //
    ListView_SetItemState(m_hwndListView, -1, LVIS_SELECTED, LVIS_SELECTED);
    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::InvertSelection

    Description: Selects all items that are not selected and unselects all
        items that are.

    Arguments: None.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::InvertSelectedItems(
    VOID
    )
{
    INT iItem = -1;

    CAutoWaitCursor waitcursor;
    SetFocus(m_hwndListView);
    while ((iItem = ListView_GetNextItem(m_hwndListView, iItem, 0)) != -1)
    {
        DWORD dwState;

        dwState = ListView_GetItemState(m_hwndListView, iItem, LVIS_SELECTED);
        dwState ^= LVNI_SELECTED;
        ListView_SetItemState(m_hwndListView, iItem, dwState, LVIS_SELECTED);
    }
    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnHelpAbout

    Description: Handler for "About Windows NT" menu option.

    Arguments:
        hWnd - Handle of parent window for "about" dialog.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnHelpAbout(
    HWND hWnd
    )
{
    TCHAR szOpSysName[MAX_PATH];

    LoadString(g_hInstDll, IDS_WINDOWS, szOpSysName, ARRAYSIZE(szOpSysName));
    ShellAbout(hWnd, szOpSysName, NULL, NULL);
    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnHelpTopics

    Description: Handler for "Help Topics" menu option.

    Arguments:
        hWnd - Handle of parent window for the help UI.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnHelpTopics(
    HWND hWnd
    )
{
    const char szHtmlHelpFileA[]  = "DSKQUOUI.CHM > windefault";
    const char szHtmlHelpTopicA[] = "nt_diskquota_overview.htm";

    HtmlHelpA(hWnd,
             szHtmlHelpFileA,
             HH_DISPLAY_TOPIC,
             (DWORD_PTR)szHtmlHelpTopicA);
    return 0;
}



bool
DetailsView::SingleSelectionIsAdmin(
    void
    )
{
    bool bResult = false;

    if (1 == ListView_GetSelectedCount(m_hwndListView))
    {
        INT iItem = ListView_GetNextItem(m_hwndListView, -1, LVNI_SELECTED);
        if (-1 != iItem)
        {
            PDISKQUOTA_USER pUser = NULL;
            if (m_UserList.Retrieve((LPVOID *)&pUser, iItem) && NULL != pUser)
            {
                bResult = !!UserIsAdministrator(pUser);
            }
        }
    }
    return bResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnCmdDelete

    Description: Called whenever the user presses DEL or selects the "Delete"
        option from the main menu, context menu or toolbar.
        The method attempts to delete the selected records.  Any records that
        have 1+ bytes charged to them will not be deleted.  A message box
        is displayed informing the user if any selected records have 1+ bytes.

    Arguments: None.

    Returns: Nothing.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/11/96    Initial creation.                                    BrianAu
    03/11/98    Added code for resolving "owned" files.              BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnCmdDelete(
    VOID
    )
{
    HRESULT hResult = NO_ERROR;

    //
    // Make sure they really want to do this.
    // Early return if they don't.
    // Don't ask if it's a single selection and the selected user
    // is BUILTIN/Administrators.  This user can't be deleted anyway so
    // we don't want to ask for confirmation.  The deletion attempt will
    // fail later and we'll display a "can't be deleted" msgbox.
    //
    if (!SingleSelectionIsAdmin() && IDNO == DiskQuotaMsgBox(m_hwndListView,
                                                             IDS_CONFIRM_DELETE_USER,
                                                             IDS_TITLE_DISK_QUOTA,
                                                             MB_ICONWARNING | MB_YESNO))
    {
        return 0;
    }

    //
    // Clear any previous undo actions.
    // Only allow undo for a single delete (single or multi-user) operation.
    //
    m_pUndoList->Clear();

    CAutoWndEnable autoenable(m_hwndListView);
    PDISKQUOTA_USER pUser = NULL;
    INT iItem             = -1;
    INT cItemsToDelete    = ListView_GetSelectedCount(m_hwndListView);
    ProgressDialog dlgProgress(IDD_PROGRESS,
                               IDC_PROGRESS_BAR,
                               IDC_TXT_PROGRESS_DESCRIPTION,
                               IDC_TXT_PROGRESS_FILENAME);

    if (2 < cItemsToDelete)
    {
        //
        // Create and display a progress dialog if we're deleting more than 2
        // user quota records.
        //
        if (dlgProgress.Create(g_hInstDll,
                               m_hwndMain))
        {
            autoenable.Enable(false);
            dlgProgress.ProgressBarInit(0, cItemsToDelete, 1);
            dlgProgress.SetDescription(MAKEINTRESOURCE(IDS_PROGRESS_DELETING));
            dlgProgress.Show();
        }
    }

    //
    // Set each user's threshold and limit to -2 (MARK4DEL) and remove the
    // item from the listview.
    // A limit of -2 tells the quota system (NTFS) that the record should be
    // removed from the quota file.  However, if the user still has quota
    // charged, the record will be restored.
    //
    CAutoSetRedraw autoredraw(m_hwndListView, false);
    CArray<IDiskQuotaUser *> rgpUsersWithFiles;
    LONGLONG Threshold;
    LONGLONG Limit;


    while(-1 != (iItem = ListView_GetNextItem(m_hwndListView, iItem, LVNI_SELECTED)) &&
          !dlgProgress.UserCancelled())
    {
        if (m_UserList.Retrieve((LPVOID *)&pUser, iItem))
        {
            if (UserIsAdministrator(pUser))
            {
                //
                // Deletion of the BUILTINS\Administrators quota record is not
                // allowed.
                //
                CString strText(g_hInstDll, IDS_CANT_DELETE_ADMIN_RECORD);
                DiskQuotaMsgBox(dlgProgress.m_hWnd ? dlgProgress.m_hWnd : m_hwndListView,
                                strText,
                                IDS_TITLE_DISK_QUOTA,
                                MB_ICONWARNING | MB_OK);
            }
            else
            {
                //
                // Get threshold and limit values for undo action.
                //
                pUser->GetQuotaThreshold(&Threshold);
                pUser->GetQuotaLimit(&Limit);

                //
                // Remove the user from the quota file.
                //
                hResult = m_pQuotaControl->DeleteUser(pUser);
                if (SUCCEEDED(hResult))
                {
                    pUser->AddRef(); // Giving ptr to UNDO list.
                    try
                    {
                        m_pUndoList->Add(new UndoDelete(pUser, Threshold, Limit));
                    }
                    catch(CAllocException& e)
                    {
                        pUser->Release(); // Release from Undo list.
                        EnableWindow(m_hwndMain, TRUE);
                        throw;
                    }

                    ListView_DeleteItem(m_hwndListView, iItem);
                    //
                    // Deletion is successful.  Now actually remove the user from
                    // the user list.
                    //
                    m_UserList.Remove((LPVOID *)&pUser, iItem);

                    pUser->Release();  // Release from listview.
                    //
                    // Decrement the search index by 1 since what was index + 1
                    // is now index.  ListView_GetNextItem ignores the item at "index".
                    //
                    iItem--;
                    dlgProgress.ProgressBarAdvance();
                }
                else if (ERROR_FILE_EXISTS == HRESULT_CODE(hResult))
                {
                    //
                    // One more we couldn't delete.
                    //
                    rgpUsersWithFiles.Append(pUser);
                }
            }
        }
    }

    if (0 < rgpUsersWithFiles.Count())
    {
        //
        // Display a dialog listing users selected for deletion and
        // and the files owned by those users on this volume.  From the dialog,
        // the admin can Delete, Move or Take Ownership of the files.
        //
        dlgProgress.SetDescription(MAKEINTRESOURCE(IDS_PROGRESS_SEARCHINGFORFILES));
        CFileOwnerDialog dlg(g_hInstDll,
                             dlgProgress.m_hWnd ? dlgProgress.m_hWnd : m_hwndListView,
                             m_idVolume.FSPath(),
                             rgpUsersWithFiles);
        dlg.Run();
        dlgProgress.SetDescription(MAKEINTRESOURCE(IDS_PROGRESS_DELETING));

        int cUsers = rgpUsersWithFiles.Count();
        int cCannotDelete = 0;
        for (int i = 0; i < cUsers; i++)
        {
            pUser = rgpUsersWithFiles[i];
            //
            // Get threshold and limit values for undo action.
            //
            pUser->GetQuotaThreshold(&Threshold);
            pUser->GetQuotaLimit(&Limit);

            //
            // Try to remove the user from the quota file.
            //
            hResult = m_pQuotaControl->DeleteUser(pUser);
            if (SUCCEEDED(hResult))
            {
                pUser->AddRef(); // Giving ptr to UNDO list.
                try
                {
                    m_pUndoList->Add(new UndoDelete(pUser, Threshold, Limit));
                }
                catch(CAllocException& e)
                {
                    pUser->Release(); // Release from Undo list.
                    throw;
                }

                iItem = FindUserByObjPtr(pUser);
                if (-1 != iItem)
                {
                    ListView_DeleteItem(m_hwndListView, iItem);
                    //
                    // Deletion is successful.  Now actually remove the user from
                    // the user list.
                    //
                    m_UserList.Remove((LPVOID *)&pUser, iItem);
                    pUser->Release();  // Release from listview.
                }
                dlgProgress.ProgressBarAdvance();
            }
            else if (ERROR_FILE_EXISTS == HRESULT_CODE(hResult))
            {
                cCannotDelete++;
            }
        }
        if (0 < cCannotDelete)
        {
            //
            // One or more records could not be deleted because they have
            // disk space charged to them.
            //
            CString strText;

            if (1 == cCannotDelete)
                strText.Format(g_hInstDll, IDS_CANNOT_DELETE_USER);
            else
                strText.Format(g_hInstDll, IDS_CANNOT_DELETE_USERS, cCannotDelete);

            DiskQuotaMsgBox(m_hwndListView,
                            strText,
                            IDS_TITLE_DISK_QUOTA,
                            MB_ICONINFORMATION | MB_OK);
        }
    }

    ShowItemCountInStatusBar();

    if (FAILED(hResult) && ERROR_FILE_EXISTS != HRESULT_CODE(hResult))
    {
        //
        // Something bad happened.
        // BUGBUG: Do we need to discriminate between a general failure and
        //         a quota file write error?
        //
        DiskQuotaMsgBox(m_hwndListView,
                        IDS_ERROR_DELETE_USER,
                        IDS_TITLE_DISK_QUOTA,
                        MB_ICONERROR | MB_OK);
    }

    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnCmdUndo

    Description: Called whenever the user presses Ctrl + Z or selects
        the "Undo" option from the main menu, context menu or toolbar.
        The method invokes the current undo list to "undo" its actions.

    Arguments: None.

    Returns: Always 0.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/01/96    Initial creation.                                    BrianAu
    02/26/97    Added call to update status bar.                     BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnCmdUndo(
    VOID
    )
{
    if (NULL != m_pUndoList)
    {
        CAutoWaitCursor waitcursor;
        m_pUndoList->Undo();
        ShowItemCountInStatusBar();
    }
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnCmdFind

    Description: Called whenever the user presses Ctrl + F or selects
        the "Find" option from the main menu, context menu or toolbar.
        The method invokes the "Find User" dialog.

    Arguments: None.

    Returns: Always 0.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnCmdFind(
    VOID
    )
{
    if (NULL != m_pUserFinder)
    {
        m_pUserFinder->InvokeFindDialog(m_hwndListView);
    }
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnCmdProperties

    Description: Displays a properties dialog for one or more selected objects.
        Invoked when the user selects a "Properties" menu option, dbl clicks
        a selection or presses Return for a selection.

    Arguments: None.

    Returns: Always returns 0.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
    09/10/96    Added passing of LVSelection to prop sheet ctor.     BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnCmdProperties(
    VOID
    )
{
    LVSelection lvs(m_hwndListView);

    INT iItem = -1;

    //
    // Fill in arrays of user pointers and item indices.
    //
    while(-1 != (iItem = ListView_GetNextItem(m_hwndListView, iItem, LVNI_SELECTED)))
    {
        LPVOID pvUser = 0;

        if (m_UserList.Retrieve(&pvUser, iItem))
        {
            //
            // Add user object pointer and item index to the selection object.
            // We'll use this container to communicate the selected items to the
            // property sheet object.
            // This can throw OutOfMemory.
            //
            lvs.Add((PDISKQUOTA_USER)pvUser, iItem);
        }
    }

    if (0 < lvs.Count())
    {
        //
        // Create and run the property sheet.  It's modal.
        // There's a condition where the user can select in the listview and
        // nothing is actually selected (i.e. select below the last item).
        // Therefore, we need the (0 < count) check.
        //
        m_pQuotaControl->AddRef();
        UserPropSheet ups(m_pQuotaControl,
                          m_idVolume,
                          m_hwndListView,
                          lvs,
                          *m_pUndoList);
        ups.Run();
    }

    return 0;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnCmdNew

    Description: Displays a properties dialog for adding a new user to the
        quota information file.
        Invoked when the user selects the "New" menu option.

    Arguments: None.

    Returns: Always returns 0.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/27/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnCmdNew(
    VOID
    )
{
    //
    // Create and run the AddUser dialog.
    // Note that it first launches the DS Object Picker dialog.
    //
    m_pQuotaControl->AddRef();
    AddUserDialog dlg(m_pQuotaControl,
                      m_idVolume,
                      g_hInstDll,
                      m_hwndListView,
                      m_hwndListView,
                      *m_pUndoList);
    dlg.Run();

    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::CreateVolumeDisplayName [static]

    Description: Obtains the display name used by the shell for a given
        volume.

    Arguments:
        pszDrive - Address of string containing drive name (i.e. "C:\").

        pstrDisplayName - Address of CString object to receive the
            display name.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/30/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::CreateVolumeDisplayName(
    const CVolumeID& idVolume, // [in] - "C:\" or "\\?\Volume{ <guid }\"
    CString *pstrDisplayName   // [out] - "My Disk (C:)"
    )
{
    HRESULT hr = E_FAIL;

    if (idVolume.IsMountedVolume())
    {
        //
        // If it's a mounted volume thingy "\\?\Volume{ <guid> }\", the shell won't
        // understand it.  Just use the default display name provided by the
        // CVolumeID object.
        //
        *pstrDisplayName = idVolume.ForDisplay();
    }
    else
    {
        //
        // It's a normal volume.  Get the display name the shell uses.
        //
        com_autoptr<IShellFolder> ptrDesktop;
        //
        // Bind to the desktop folder.
        //
        hr = SHGetDesktopFolder(ptrDesktop.getaddr());
        if (SUCCEEDED(hr))
        {
            sh_autoptr<ITEMIDLIST> ptrIdlDrives;
            hr = SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, ptrIdlDrives.getaddr());
            if (SUCCEEDED(hr))
            {
                //
                // Bind to the "Drives" folder.
                //
                com_autoptr<IShellFolder> ptrDrives;
                hr = ptrDesktop->BindToObject(ptrIdlDrives, NULL, IID_IShellFolder, (LPVOID *)ptrDrives.getaddr());
                if (SUCCEEDED(hr))
                {
                    com_autoptr<IEnumIDList> ptrEnum;

                    //
                    // Enumerate all of the non-folder objects in the drives folder.
                    //
                    hr = ptrDrives->EnumObjects(NULL, SHCONTF_NONFOLDERS, ptrEnum.getaddr());
                    if (SUCCEEDED(hr))
                    {
                        sh_autoptr<ITEMIDLIST> ptrIdlItem;
                        ULONG ulFetched = 0;
                        LPCTSTR pszDrive = idVolume.ForParsing();
                        //
                        // For each item in the drives folder...
                        //
                        while(S_OK == ptrEnum->Next(1, ptrIdlItem.getaddr(), &ulFetched))
                        {
                            TCHAR szName[MAX_PATH];
                            StrRet strretName((LPITEMIDLIST)ptrIdlItem);
                            StrRet strretDisplayName((LPITEMIDLIST)ptrIdlItem);

                            //
                            // Get the non-display name form; i.e. "G:\"
                            //
                            hr = ptrDrives->GetDisplayNameOf(ptrIdlItem, SHGDN_FORPARSING, &strretName);
                            if (SUCCEEDED(hr))
                            {
                                strretName.GetString(szName, ARRAYSIZE(szName));
                                if (TEXT(':') == szName[1] &&
                                    *pszDrive == szName[0])
                                {
                                    //
                                    // Get the display name form; i.e. "My Disk (G:)"
                                    //
                                    ptrDrives->GetDisplayNameOf(ptrIdlItem, SHGDN_NORMAL, &strretDisplayName);
                                    strretDisplayName.GetString(pstrDisplayName->GetBuffer(MAX_PATH), MAX_PATH);
                                    pstrDisplayName->ReleaseBuffer();
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnCmdImport

    Description: Called when the user selects the "Import" option on the
        "Quota" menu.  Presents the "Open File" common dialog to get the
        name for the file containing the import information.  Then passes
        the path off to an Importer object to do the actual import.

    Arguments: None.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnCmdImport(
    VOID
    )
{
    HRESULT hResult            = NO_ERROR;
    TCHAR szFileName[MAX_PATH] = { TEXT('\0') };
    TCHAR szTitle[80] = { TEXT('\0') };
    LoadString(g_hInstDll, IDS_DLGTITLE_IMPORT, szTitle, ARRAYSIZE(szTitle));

    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = m_hwndMain;
    ofn.hInstance   = g_hInstDll;
    ofn.lpstrFile   = szFileName;
    ofn.lpstrTitle  = szTitle;
    ofn.nMaxFile    = ARRAYSIZE(szFileName);
    ofn.Flags       = OFN_HIDEREADONLY | OFN_NOCHANGEDIR |
                      OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    //
    // Get name of import file from user and import the files.
    //
    if (GetOpenFileName(&ofn))
    {
        Importer importer(*this);
        hResult = importer.Import(ofn.lpstrFile);
    }

    return SUCCEEDED(hResult);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnCmdExport

    Description: Called when the user selects the "Export" option on the
        "Quota" menu or listview context menu.  Presents the "Save File"
        common dialog to get the name for the output file. It then creates
        the doc file, the stream within the doc file and then calls the
        DetailsView's IDataObject implementation to render the data on the
        stream.

    Arguments: None.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnCmdExport(
    VOID
    )
{
    HRESULT hResult           = NO_ERROR;
    IDataObject *pIDataObject = NULL;

    try
    {
        hResult = QueryInterface(IID_IDataObject, (LPVOID *)&pIDataObject);
        if (SUCCEEDED(hResult))
        {
            FORMATETC fmt;
            DataObject::SetFormatEtc(fmt,
                                     DataObject::m_CF_NtDiskQuotaExport,
                                     TYMED_ISTREAM);

            hResult = pIDataObject->QueryGetData(&fmt);
            if (SUCCEEDED(hResult))
            {
                TCHAR szFileName[MAX_PATH] = { TEXT('\0') };
                TCHAR szTitle[80] = { TEXT('\0') };

                LoadString(g_hInstDll, IDS_DLGTITLE_EXPORT, szTitle, ARRAYSIZE(szTitle));

                OPENFILENAME ofn;
                ZeroMemory(&ofn, sizeof(ofn));
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner   = m_hwndMain;
                ofn.hInstance   = g_hInstDll;
                ofn.lpstrFile   = szFileName;
                ofn.lpstrTitle  = szTitle;
                ofn.nMaxFile    = ARRAYSIZE(szFileName);
                ofn.Flags       = OFN_HIDEREADONLY | OFN_NOCHANGEDIR |
                                  OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
                //
                // Get output file name from user.
                //
                if (GetSaveFileName(&ofn))
                {
                    DWORD grfMode = STGM_DIRECT | STGM_READWRITE |
                                    STGM_CREATE | STGM_SHARE_EXCLUSIVE;
                    IStorage *pStg;

                    //
                    // Create the output doc file.
                    //
                    hResult = StgCreateDocfile(ofn.lpstrFile,
                                               grfMode,
                                               0,
                                               &pStg);
                    if (SUCCEEDED(hResult))
                    {
                        //
                        // Create the stream in the doc file.
                        //
                        IStream *pStm;
                        hResult = pStg->CreateStream(DataObject::SZ_EXPORT_STREAM_NAME,
                                                     grfMode,
                                                     0, 0,
                                                     &pStm);
                        if (SUCCEEDED(hResult))
                        {
                            CStgMedium medium;

                            //
                            // Render the quota information onto the file stream.
                            //
                            hResult = pIDataObject->GetData(&fmt, &medium);
                            if (SUCCEEDED(hResult))
                            {
                                ULARGE_INTEGER cb = {0xFFFFFFFF, 0xFFFFFFFF};
                                medium.pstm->CopyTo(pStm, cb, NULL, NULL);
                            }
                            pStm->Release();
                        }
                        pStg->Release();
                    }
                    if (FAILED(hResult))
                    {
                        UINT iMsg = IDS_EXPORT_STREAM_FAILED;

                        switch(hResult)
                        {
                            case STG_E_ACCESSDENIED:
                                iMsg = IDS_EXPORT_STREAM_NOACCESS;
                                break;

                            case E_OUTOFMEMORY:
                            case STG_E_INSUFFICIENTMEMORY:
                                iMsg = IDS_EXPORT_STREAM_OUTOFMEMORY;
                                break;

                            case STG_E_INVALIDNAME:
                                iMsg = IDS_EXPORT_STREAM_INVALIDNAME;
                                break;

                            case STG_E_TOOMANYOPENFILES:
                                iMsg = IDS_EXPORT_STREAM_TOOMANYFILES;
                                break;

                            default:
                                break;
                        }
                        DiskQuotaMsgBox(m_hwndMain,
                                        iMsg,
                                        IDS_TITLE_DISK_QUOTA,
                                        MB_ICONERROR | MB_OK);

                    }
                }
            }
            else
            {
                DBGERROR((TEXT("Export: Error 0x%08X returned from QueryGetData."), hResult));
            }
            pIDataObject->Release();
            pIDataObject = NULL;
        }
        else
        {
            DBGERROR((TEXT("Export: Error 0x%08X getting IDataObject."), hResult));
        }
    }
    catch(CAllocException& e)
    {
        if (NULL != pIDataObject)
            pIDataObject->Release();
        throw;
    }
    return SUCCEEDED(hResult);
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnCmdViewStatusBar

    Description:  Toggles the visibility of the status bar.  Invoked when the
        user selects the "Status Bar" menu option.

    Arguments: None.

    Returns: Always returns 0.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnCmdViewStatusBar(
    VOID
    )
{
    RECT rc;

    m_lvsi.fStatusBar ^= TRUE;
    ShowWindow(m_hwndStatusBar, m_lvsi.fStatusBar ? SW_SHOW : SW_HIDE);

    //
    // Adjust the main window.
    //
    GetWindowRect(m_hwndMain, &rc);
    OnSize(m_hwndMain, WM_SIZE, SIZE_RESTORED, MAKELONG(rc.right-rc.left,rc.bottom-rc.top));

    //
    // Check the menu item to indicate the current status bar state.
    //
    CheckMenuItem(GetMenu(m_hwndMain),
                  IDM_VIEW_STATUSBAR,
                  MF_BYCOMMAND | (m_lvsi.fStatusBar ? MF_CHECKED : MF_UNCHECKED));
    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnCmdViewToolBar

    Description:  Toggles the visibility of the tool bar.  Invoked when the
        user selects the "Tool Bar" menu option.

    Arguments: None.

    Returns: Always returns 0.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnCmdViewToolBar(
    VOID
    )
{
    RECT rc;

    m_lvsi.fToolBar ^= TRUE;
    ShowWindow(m_hwndToolBar, m_lvsi.fToolBar ? SW_SHOW : SW_HIDE);

    //
    // Adjust the main window.
    //
    GetWindowRect(m_hwndMain, &rc);
    OnSize(m_hwndMain, WM_SIZE, SIZE_RESTORED, MAKELONG(rc.right-rc.left,rc.bottom-rc.top));

    //
    // Check the menu item to indicate the current tool bar state.
    //
    CheckMenuItem(GetMenu(m_hwndMain),
                  IDM_VIEW_TOOLBAR,
                  MF_BYCOMMAND | (m_lvsi.fToolBar ? MF_CHECKED : MF_UNCHECKED));
    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::OnCmdViewShowFolder

    Description:  Toggles the visibility of the Domain Name column.  Invoked
        when the user selects the "Show Domain" menu option.

    Arguments: None.

    Returns: Always returns 0.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/06/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::OnCmdViewShowFolder(
    VOID
    )
{
    m_lvsi.fShowFolder ^= TRUE;

    if (m_lvsi.fShowFolder)
    {
        //
        // Insert the folder column and DEACTIVATE listview tooltip.
        // Always add at index 1 then shift it to position 0.
        // User can drag it elsewhere if they like.
        // Because of the listview's icon painting behavior, we only let the
        // "status" column be index 0.
        //
        AddColumn(1, g_rgColumns[DetailsView::idCol_Folder]);
        INT cCols = Header_GetItemCount(m_hwndHeader);
        INT rgColIndicies[DetailsView::idCol_Last];
        INT iTemp = 0;

        DBGASSERT((DetailsView::idCol_Last >= cCols));
        Header_GetOrderArray(m_hwndHeader, cCols, rgColIndicies);
        //
        // Swap the column we just added with column 0.
        //
        iTemp = rgColIndicies[0];
        rgColIndicies[0] = rgColIndicies[1];
        rgColIndicies[1] = iTemp;
        Header_SetOrderArray(m_hwndHeader, cCols, rgColIndicies);

        ActivateListViewToolTip(FALSE);
    }
    else
    {
        //
        // Remove the folder column and ACTIVATE listview tooltip.
        // With the column hidden, users can view a user's folder by hovering
        // over the user's name.
        //
        ActivateListViewToolTip(TRUE);
        RemoveColumn(DetailsView::idCol_Folder);
    }

    //
    // Check/Uncheck the "Show Folder" menu item.
    //
    CheckMenuItem(GetMenu(m_hwndMain),
                  IDM_VIEW_SHOWFOLDER,
                  MF_BYCOMMAND | (m_lvsi.fShowFolder ? MF_CHECKED : MF_UNCHECKED));

    //
    // If the folder column is hidden, the "Arrange by Folder" menu option
    // is disabled.
    //
    EnableMenuItem_ArrangeByFolder(m_lvsi.fShowFolder);

    //
    // I haven't found a way to do this without unloading and reloading the
    // objects following the new column configuration.
    //
    Refresh();

    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::EnableMenuItem_ArrangeByFolder

    Description:  Sets the sensitivity of the "by Folder" menu item
        in the "Arrange Items" submenu.

    Arguments:
        bEnable - TRUE = Enable menu item, FALSE = Disable and gray item.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/08/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
DetailsView::EnableMenuItem_ArrangeByFolder(
    BOOL bEnable
    )
{
    HMENU hMainMenu        = GetMenu(m_hwndMain);
    HMENU hViewMenu        = GetSubMenu(hMainMenu, iMENUITEM_VIEW);
    HMENU hViewArrangeMenu = GetSubMenu(hViewMenu, iMENUITEM_VIEW_ARRANGE);

    //
    // If these assert, it probably means somebody's changed the
    // menus so that the iMENUITEM_XXXXX constants are no longer correct.
    //
    DBGASSERT((NULL != hViewMenu));
    DBGASSERT((NULL != hViewArrangeMenu));

    EnableMenuItem(hViewArrangeMenu,
                   IDM_VIEW_ARRANGE_BYFOLDER,
                   MF_BYCOMMAND | (bEnable ? MF_ENABLED : MF_GRAYED));
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::EnableMenuItem_Undo

    Description:  Sets the sensitivity of the "Undo" menu item
        in the "Edit" submenu.

    Arguments:
        bEnable - TRUE = Enable menu item, FALSE = Disable and gray item.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/08/96    Initial creation.                                    BrianAu
    10/22/96    Replaced Assert() with nested if's.                  BrianAu
                Tester hit assert via WM_COMMAND when closing
                details view.
*/
///////////////////////////////////////////////////////////////////////////////
VOID
DetailsView::EnableMenuItem_Undo(
    BOOL bEnable
    )
{
    HMENU hMainMenu = GetMenu(m_hwndMain);

    if (NULL != hMainMenu)
    {
        HMENU hEditMenu  = GetSubMenu(hMainMenu, iMENUITEM_EDIT);
        if (NULL != hEditMenu)
        {
            EnableMenuItem(hEditMenu,
                           IDM_EDIT_UNDO,
                           MF_BYCOMMAND | (bEnable ? MF_ENABLED : MF_GRAYED));

            SendMessage(m_hwndToolBar, TB_ENABLEBUTTON, IDM_EDIT_UNDO, MAKELONG(bEnable, 0));
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::ShowItemCountInStatusBar

    Description: Displays the current count of selected items in the status bar.
        This is what is displayed in the status bar when a menu item is not
        currently selected.

    Arguments: None.

    Returns: Always returns 0.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
    12/16/96    Appended "incorrect data" warning to status bar      BrianAu
                text when quotas are disabled on system.
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::ShowItemCountInStatusBar(
    VOID
    )
{
    DWORD cTotalItems    = m_UserList.Count();
    DWORD cSelectedItems = ListView_GetSelectedCount(m_hwndListView);

    CString strText(g_hInstDll, IDS_STATUSBAR_ITEMCOUNT, cTotalItems, cSelectedItems);
    SendMessage(m_hwndStatusBar, SB_SETTEXT, 0, (LPARAM)((LPCTSTR)strText));

    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::ShowMenuTextInStatusBar

    Description: Displays the description of the currently-selected menu
        item in the status bar.

    Arguments: None.

    Returns: Always returns 0.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::ShowMenuTextInStatusBar(
    DWORD idMenuOption
    )
{
    TCHAR szText[MAX_PATH];

    LoadString(g_hInstDll, idMenuOption, szText, ARRAYSIZE(szText));
    SendMessage(m_hwndStatusBar, SB_SETTEXT, 0, (LPARAM)szText);
    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::Refresh

    Description: Refreshes the view by re-loading the objects.

    Arguments: bInvalidateCache - true == invalidate all entries in the
                    SID-name cache.  Default is false.

    Returns: Always returns 0.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
    02/21/97    Ownerdata listview.                                  BrianAu
    10/10/98    Added bInvalidateCache argument.                     BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT
DetailsView::Refresh(
    bool bInvalidateCache    // optional.  default is false.
    )
{
    CAutoWaitCursor waitcursor;
    if (bInvalidateCache)
        m_pQuotaControl->InvalidateSidNameCache();

    InvalidateRect(m_hwndListView, NULL, TRUE);

    CAutoSetRedraw autoredraw(m_hwndListView, false);
    ReleaseObjects();
    autoredraw.Set(true);

    LoadObjects();
    ListView_SetItemCountEx(m_hwndListView,
                            m_UserList.Count(),
                            LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);

    SortObjects(m_iLastColSorted, m_fSortDirection);
    ShowItemCountInStatusBar();
    FocusOnSomething();
    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::FocusOnSomething

    Description: Ensures that one or more listview items have the focus
        highlighting.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/09/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
DetailsView::FocusOnSomething(
    VOID
    )
{
    INT iFocus;

    iFocus = ListView_GetNextItem(m_hwndListView, -1, LVNI_FOCUSED);
    if (-1 == iFocus)
        iFocus = 0;

    ListView_SetItemState(m_hwndListView, iFocus, LVIS_FOCUSED | LVIS_SELECTED,
                                                  LVIS_FOCUSED | LVIS_SELECTED)
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::CalcPctQuotaUsed

    Description: Calculates the percent quota used for a given user.  The
        value returned is an integer.

    Arguments:
        pUser - Address of IDiskQuotaUser interface for user object.

        pdwPct - Address of DWORD to receive the percentage value.
            If the method returns div-by-zero, this value is set to ~0.
            This lets a caller sort erroneous values from valid values.
            The (~0 - 1) return value is used so that NOLIMIT users
            are grouped separate from 0 limit users when sorted on % used.
            Both are using 0% of their quota but it looks better if
            they are each grouped separately.

    Returns:
        NO_ERROR    - Success.
        STATUS_INTEGER_DIVIDE_BY_ZERO - The user's quota limit was 0.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::CalcPctQuotaUsed(
    PDISKQUOTA_USER pUser,
    LPDWORD pdwPct
    )
{
    LONGLONG llUsed;
    LONGLONG llLimit;
    HRESULT hResult = E_FAIL;

    DBGASSERT((NULL != pUser));
    DBGASSERT((NULL != pdwPct));

    pUser->GetQuotaUsed(&llUsed);
    pUser->GetQuotaLimit(&llLimit);

    if (NOLIMIT == llLimit)
    {
        *pdwPct = (DWORD)~0 - 1;  // No quota limit for user.
    }
    else if (0 < llLimit)
    {
        *pdwPct = (INT)((llUsed * 100) / llLimit);
        hResult = NO_ERROR;
    }
    else
    {
        //
        // Limit is 0.  Would produce div-by-zero.
        //
        *pdwPct = (DWORD)~0;
    }

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::AddUser

    Description: Adds a user object to the listview.  Note that this is used
        for adding a single user object such as in an "add user" operation.
        The method LoadObjects is used to load the whole listview.  It's
        more efficient than calling this for each user.

    Arguments:
        pUser - Address of user's IDiskQuotaUser interface.

    Returns:
        TRUE  - User was added.
        FALSE - User wasn't added.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
DetailsView::AddUser(
    PDISKQUOTA_USER pUser
    )
{
    BOOL bResult    = FALSE;
    LV_ITEM item;

    item.mask       = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE;
    item.state      = 0;
    item.stateMask  = 0;
    item.iSubItem   = 0;
    item.pszText    = LPSTR_TEXTCALLBACK;
    item.iImage     = I_IMAGECALLBACK;
    item.iItem      = 0;

    m_UserList.Insert((LPVOID)pUser);
    if (-1 != ListView_InsertItem(m_hwndListView, &item))
    {
        bResult = TRUE;
    }

    return bResult;
}

///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::FindUserByName

    Description: Locate a specified user in the listview by account name.
        Name comparison is case-insensitive.

    Arguments:
        pszUserName - Account name for user.

        ppIUser [optional] - Address of IDiskQuotaUser pointer variable to
            receive the address of the user object.

    Returns:
        -1 = account name not found.
        Otherwise, returns index of item in listview.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/23/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT
DetailsView::FindUserByName(
    LPCTSTR pszLogonName,
    PDISKQUOTA_USER *ppIUser // [optional]
    )
{
    INT iItem = -1;

    m_UserList.Lock();
    try
    {
        INT cUsers = m_UserList.Count();
        PDISKQUOTA_USER pUser = NULL;
        //
        // Find the user that matches pszUserName.
        //
        for (INT i = 0; i < cUsers && -1 == iItem; i++)
        {
            if (m_UserList.Retrieve((LPVOID *)&pUser, i))
            {
                //
                // Get name from listview item.
                //
                if (NULL != pUser)
                {
                    TCHAR szLogonName[MAX_USERNAME];
                    pUser->GetName(NULL, 0,
                                   szLogonName, ARRAYSIZE(szLogonName),
                                   NULL, 0);

                    if (CSTR_EQUAL == CompareString(LOCALE_USER_DEFAULT,
                                                    NORM_IGNORECASE,
                                                    szLogonName, -1,
                                                    pszLogonName, -1))
                    {
                        iItem = i;
                        if (NULL != ppIUser)
                            *ppIUser = pUser;
                    }
                }
            }
        }
    }
    catch(CAllocException& e)
    {
        m_UserList.ReleaseLock();
        throw;
    }
    m_UserList.ReleaseLock();
    return iItem;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::FindUserBySid

    Description: Locate a specified user in the listview by SID.

    Arguments:
        pSid - Address of buffer containing key SID.

        ppIUser [optional] - Address of IDiskQuotaUser pointer variable to
            receive the address of the user object.

    Returns:
        -1 = Record not found.
        Otherwise, returns index of item in listview.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/23/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT
DetailsView::FindUserBySid(
    LPBYTE pbSid,
    PDISKQUOTA_USER *ppIUser  // [optional]
    )
{
    INT iItem = -1;

    m_UserList.Lock();
    try
    {
        INT cUsers = m_UserList.Count();
        PDISKQUOTA_USER pUser = NULL;
        //
        // Find the user that matches pszUserName.
        //
        for (INT i = 0; i < cUsers && -1 == iItem; i++)
        {
            if (m_UserList.Retrieve((LPVOID *)&pUser, i))
            {
                //
                // Get SID from listview item.
                //
                if (NULL != pUser)
                {
                    BYTE Sid[MAX_SID_LEN];
                    pUser->GetSid((LPBYTE)&Sid, ARRAYSIZE(Sid));

                    if (EqualSid((LPBYTE)&Sid, pbSid))
                    {
                        iItem = i;
                        if (NULL != ppIUser)
                            *ppIUser = pUser;
                    }
                }
            }
        }
    }
    catch(CAllocException& e)
    {
        m_UserList.ReleaseLock();
        throw;
    }
    m_UserList.ReleaseLock();
    return iItem;
}

//
// Locate a user in the listview based on it's object pointer.
//
INT
DetailsView::FindUserByObjPtr(
    PDISKQUOTA_USER pUserKey
    )
{
    INT iItem = -1;

    m_UserList.Lock();
    try
    {
        INT cUsers = m_UserList.Count();
        PDISKQUOTA_USER pUser = NULL;
        //
        // Find the user that matches pszUserName.
        //
        for (INT i = 0; i < cUsers && -1 == iItem; i++)
        {
            if (m_UserList.Retrieve((LPVOID *)&pUser, i))
            {
                if (NULL != pUser && pUserKey == pUser)
                {
                    iItem = i;
                }
            }
        }
    }
    catch(CAllocException& e)
    {
        m_UserList.ReleaseLock();
        throw;
    }
    m_UserList.ReleaseLock();
    return iItem;
}

///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::GotoUserName

    Description: Locate a specified user in the listview.  If found, highlight
        the row.  The search is case-insensitive.  This function was originally
        designed to work with the "Find User" feature so that when a record
        is located, it is made visible in the view and highlighted.

    Arguments:
        pszUserName - Account name for user.

    Returns:
        TRUE  = Record found.
        FALSE = Not found.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
DetailsView::GotoUserName(
    LPCTSTR pszUserName
    )
{
    INT iUser = FindUserByName(pszUserName);
    if (-1 != iUser)
    {
        //
        // Found a match (case-insensitive).
        //
        // Select the item specified by the user.
        // Note that we leave any selected items selected.
        // Users may use the find feature to select a set of
        // non-contiguous quota records in the listview.
        //
        ListView_EnsureVisible(m_hwndListView, iUser, FALSE);
        ListView_SetItemState(m_hwndListView, iUser, LVIS_FOCUSED | LVIS_SELECTED,
                                                     LVIS_FOCUSED | LVIS_SELECTED);
    }
    return (-1 != iUser);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::GetConnectionPoint

    Description: Retrieves the IDiskQuotaEvents connection point from
        the quota control object.  This is the connection point through which
        the asynchronous user name change events are delivered as names
        are resolved by the network DC.

    Arguments: None.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
IConnectionPoint *
DetailsView::GetConnectionPoint(
    VOID
    )
{
    HRESULT hResult       = NO_ERROR;
    IConnectionPoint *pCP = NULL;
    if (NULL != m_pQuotaControl)
    {
        IConnectionPointContainer *pCPC = NULL;
        hResult = m_pQuotaControl->QueryInterface(IID_IConnectionPointContainer,
                                                  (LPVOID *)&pCPC);
        if (SUCCEEDED(hResult))
        {
            hResult = pCPC->FindConnectionPoint(IID_IDiskQuotaEvents, &pCP);
            pCPC->Release();
            if (FAILED(hResult))
                pCP = NULL;
        }
    }
    return pCP;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::ConnectEventSink

    Description: Connects the event sink (DetailsView) from the quota
        controller's IDiskQuotaEvents connection point object.

    Arguments: None.

    Returns:
        NO_ERROR    - Success.
        E_FAIL      - Failed.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::ConnectEventSink(
    VOID
    )
{
    HRESULT hResult = NO_ERROR;

    IConnectionPoint *pConnPt = GetConnectionPoint();
    if (NULL != pConnPt)
    {
        hResult = pConnPt->Advise((LPUNKNOWN)static_cast<IDataObject *>(this), &m_dwEventCookie);
        pConnPt->Release();
        DBGPRINT((DM_VIEW, DL_MID, TEXT("LISTVIEW - Connected event sink.  Cookie = %d"), m_dwEventCookie));
    }
    else
        hResult = E_FAIL;

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::DisconnectEventSink

    Description: Disconnects the event sink (DetailsView) from the quota
        controller's IDiskQuotaEvents connection point object.

    Arguments: None.

    Returns:
        NO_ERROR    - Success.
        E_FAIL      - Failed.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::DisconnectEventSink(
    VOID
    )
{
    HRESULT hResult = NO_ERROR;

    DBGPRINT((DM_VIEW, DL_MID, TEXT("LISTVIEW - Disconnecting event sink.  Cookie = %d"), m_dwEventCookie));

    if (0 != m_dwEventCookie)
    {
        IConnectionPoint *pConnPt = GetConnectionPoint();
        if (NULL != pConnPt)
        {
            hResult = pConnPt->Unadvise(m_dwEventCookie);
            if (SUCCEEDED(hResult))
            {
                m_dwEventCookie = 0;
            }
            pConnPt->Release();
        }
        else
            hResult = E_FAIL;
    }
    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::InitLVStateInfo

    Description: Initializes an LV_STATE_INFO structure to default values.
        NOTE: This method is declared static so that it can be called
            without a DetailsView object (not needed).

            If you want to change any listview state defaults, this is the
            place to do it.

    Arguments:
        plvsi - Address of an LV_STATE_INFO structure to be initialized.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/08/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
DetailsView::InitLVStateInfo(
    PLV_STATE_INFO plvsi
    )
{
    ZeroMemory(plvsi, sizeof(*plvsi));

    plvsi->cb         = sizeof(*plvsi);

    plvsi->wVersion       = wLV_STATE_INFO_VERSION;
    plvsi->fToolBar       = 1;  // Default to toolbar visible.
    plvsi->fStatusBar     = 1;  // Default to statusbar visible.
    plvsi->iLastColSorted = 0;  // Default to sort first col.
    plvsi->fSortDirection = 1;  // Default to ascending sort.
    for (UINT i = 0; i < DetailsView::idCol_Last; i++)
        plvsi->rgColIndices[i] = i;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::IsValidLVStateInfo

    Description: Validates the contents of a LV_STATE_INFO structure.
        NOTE: This method is declared static so that it can be called
            without a DetailsView object (not needed).

    Arguments:
        plvsi - Address of an LV_STATE_INFO structure to be validated.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    12/10/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
DetailsView::IsValidLVStateInfo(
    PLV_STATE_INFO plvsi
    )
{
    BOOL bResult = FALSE;
    INT i = 0;

    //
    // Validate structure size member.
    //
    if (plvsi->cb != sizeof(LV_STATE_INFO))
        goto info_invalid;
    //
    // Validate version.
    //
    if (wLV_STATE_INFO_VERSION != plvsi->wVersion)
        goto info_invalid;
    //
    // Validate iLastSorted member.
    //
    if (!(plvsi->iLastColSorted >= 0 && plvsi->iLastColSorted < DetailsView::idCol_Last))
        goto info_invalid;
    //
    // Validate each of the column index members.  Used for ordering columns.
    //
    for (i = 0; i < DetailsView::idCol_Last; i++)
    {
        if (!(plvsi->rgColIndices[i] >= 0 && plvsi->rgColIndices[i] < DetailsView::idCol_Last))
            goto info_invalid;
    }

    bResult = TRUE;

info_invalid:

    return bResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::GetColumnIds

    Description: Retrieves a list of IDs for the visible columns in the list.
        A client can use this list to request report items from the
        GetReportXXXXX methods below.

    Arguments:
        prgColIds - Pointer to an array of INTs to receive the column IDs.

        cColIds - Size of the destination array.

    Returns: Number of IDs written to destination array.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/08/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
UINT
DetailsView::GetColumnIds(
    INT *prgColIds,
    INT cColIds
    )
{
    INT cHdrCols = Header_GetItemCount(m_hwndHeader);
    INT rgiSubItem[DetailsView::idCol_Last];
    INT i = 0;

    if (Header_GetOrderArray(m_hwndHeader, cHdrCols, rgiSubItem))
    {
        for (i = 0; i < cHdrCols && i < cColIds; i++)
        {
            *(prgColIds + i) = m_ColMap.SubItemToId(rgiSubItem[i]);
        }
    }
    return i;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::GetNextSelectedItemIndex

    Description: Retrieves the index of a selected item.  The search starts
        with the index supplied in the iRow argument.  Therefore, the following
        loop will find all selected items:

        INT iItem = -1;

        while(1)
        {
            iItem = GetNextSelectedItemIndex(iItem);
            if (-1 == iItem)
                break;

            //
            // Do something with item.
            //
        }


    Arguments:
        iRow - Row where to start search.  The row itself is exluded from
            the search.  -1 starts search from the head of the listview.

    Returns:
        0-based index of next selected item if found.
        -1 if no more selected items.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT
DetailsView::GetNextSelectedItemIndex(INT iRow)
{
    return ListView_GetNextItem(m_hwndListView, iRow, LVNI_ALL | LVNI_SELECTED);
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::GetReportItem

    Description: Retrieve a data item for a drag-drop/clipboard report.
        This method is patterned after the GetDispInfo_XXX methods but is
        taylored to placing data on a Stream object.

    Arguments:
        iRow - Row where to begin search for next selected item in listview.
            For the first call, specify -1 to begin the search with the
            first item.  Subsequent calls should specify the value returned
            from the previous call to GetReportItem.

        iColId - Item's column ID (idCol_Folder, idCol_Name etc).

        pItem - Address of an LV_REPORT_ITEM structure.  This structure
            is used to return the data to the caller and also to specify the
            desired format for numeric values.  Some report formats want
            all data in text format (i.e. CF_TEXT) while other binary formats
            want numeric data in numeric format (i.e. XlTable).

    Returns: TRUE  = Retrieved row/col data.
             FALSE = Invalid row or column index.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/08/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
DetailsView::GetReportItem(
    UINT iRow,
    UINT iColId,
    PLV_REPORT_ITEM pItem
    )
{
    DBGASSERT((NULL != pItem));
    BOOL bResult          = FALSE;
    PDISKQUOTA_USER pUser = NULL;

    if (m_UserList.Retrieve((LPVOID *)&pUser, iRow))
    {
        LONGLONG llValue;
        DBGASSERT((NULL != pUser));
        bResult = TRUE;

        switch(iColId)
        {
            case DetailsView::idCol_Folder:
            {
                DWORD dwAccountStatus = 0;
                pUser->GetAccountStatus(&dwAccountStatus);

                if (DISKQUOTA_USER_ACCOUNT_RESOLVED == dwAccountStatus)
                {
                    pUser->GetName(pItem->pszText, pItem->cchMaxText,
                                   NULL, 0,
                                   NULL, 0);
                }
                else
                {
                    //
                    // Folder name not resolved.  User name column will
                    // contain status text.
                    //
                    lstrcpyn(pItem->pszText, TEXT(""), pItem->cchMaxText);
                }
                pItem->fType = LVRI_TEXT;
                break;
            }

            case DetailsView::idCol_Name:
            {
                DWORD dwAccountStatus = 0;
                CString strNameText;
                pUser->GetAccountStatus(&dwAccountStatus);

                if (DISKQUOTA_USER_ACCOUNT_RESOLVED == dwAccountStatus)
                {
                    //
                    // User's name has been resolved.
                    //
                    pItem->fType = LVRI_TEXT;
                    pUser->GetName(NULL, 0,
                                   NULL, 0,
                                   strNameText.GetBuffer(MAX_USERNAME), MAX_USERNAME);
                    strNameText.ReleaseBuffer();
                }
                else
                {
                    //
                    // User's name not resolved.  Use a status message.
                    //
                    switch(dwAccountStatus)
                    {
                        case DISKQUOTA_USER_ACCOUNT_UNRESOLVED:
                            strNameText = m_strAccountUnresolved;
                            break;

                        case DISKQUOTA_USER_ACCOUNT_UNKNOWN:
                            strNameText = m_strAccountUnknown;
                            break;

                        case DISKQUOTA_USER_ACCOUNT_INVALID:
                            strNameText = m_strAccountInvalid;
                            break;

                        case DISKQUOTA_USER_ACCOUNT_DELETED:
                            strNameText = m_strAccountDeleted;
                            break;

                        case DISKQUOTA_USER_ACCOUNT_UNAVAILABLE:
                            strNameText = m_strAccountUnavailable;
                            break;
                    }
                }
                lstrcpyn(pItem->pszText, strNameText, pItem->cchMaxText);

                break;
            }

            case DetailsView::idCol_LogonName:
            {
                DBGASSERT((NULL != pUser));
                DWORD dwAccountStatus = 0;
                CString strNameText;
                pUser->GetAccountStatus(&dwAccountStatus);

                if (DISKQUOTA_USER_ACCOUNT_RESOLVED == dwAccountStatus)
                {
                    //
                    // User's name has been resolved.
                    //
                    pItem->fType = LVRI_TEXT;
                    pUser->GetName(NULL, 0,
                                   strNameText.GetBuffer(MAX_USERNAME), MAX_USERNAME,
                                   NULL, 0);
                    strNameText.ReleaseBuffer();
                }

                lstrcpyn(pItem->pszText, strNameText, pItem->cchMaxText);
                break;
            }

            case DetailsView::idCol_Status:
                //
                // Return a text string to represent the icon shown
                // in the "Status" column.
                //
                DBGASSERT((NULL != pUser));
                switch(GetUserQuotaState(pUser))
                {
                    case iUSERSTATE_OK:
                        lstrcpyn(pItem->pszText, m_strStatusOK, pItem->cchMaxText);
                        break;
                    case iUSERSTATE_WARNING:
                        lstrcpyn(pItem->pszText, m_strStatusWarning, pItem->cchMaxText);
                        break;
                    default:
                        DBGASSERT((0));
                        //
                        // Fall through.
                        //
                    case iUSERSTATE_OVERLIMIT:
                        lstrcpyn(pItem->pszText, m_strStatusOverlimit, pItem->cchMaxText);
                        break;
                }
                pItem->fType = LVRI_TEXT;
                break;

            //
            // For the following numeric columns, first get the data then
            // jump to fmt_byte_count to format it as requested.  Note that
            // all numeric values are expressed in megabytes.  This is so they
            // all have the same units to help with ordering in a spreadsheet.
            // Otherwise, sorting would not be possible.  This is also why we
            // include the "(MB)" in the report column titles.
            //
            case DetailsView::idCol_AmtUsed:
                pUser->GetQuotaUsed(&llValue);
                goto fmt_byte_count;

            case DetailsView::idCol_Threshold:
                pUser->GetQuotaThreshold(&llValue);
                goto fmt_byte_count;

            case DetailsView::idCol_Limit:
                pUser->GetQuotaLimit(&llValue);
fmt_byte_count:
                //
                // Format the byte count for the requested data type (text vs. numeric).
                //
                switch(pItem->fType)
                {
                    case LVRI_NUMBER:
                        pItem->fType = LVRI_REAL;
                        if (NOLIMIT == llValue)
                            pItem->dblValue = -1.0;        // Indicates to caller "No Limit".
                        else
                            pItem->dblValue = XBytes::ConvertFromBytes(llValue, XBytes::e_Mega);
                        break;
                    case LVRI_TEXT:
                        if (NOLIMIT == llValue)
                            lstrcpyn(pItem->pszText, m_strNoLimit, pItem->cchMaxText);
                        else
                            XBytes::FormatByteCountForDisplay(llValue,
                                                              pItem->pszText,
                                                              pItem->cchMaxText,
                                                              XBytes::e_Mega);
                        //
                        // Fall through.
                        //
                    default:
                        break;
                }
                break;

            case DetailsView::idCol_PctUsed:
            {
                HRESULT hResult = CalcPctQuotaUsed(pUser, &pItem->dwValue);
                //
                // Format the percent value for the requested data type (text vs. numeric).
                // If a percentage can't be calculated (0 denominator), return -2 as an
                // INT value or "N/A" as a text value.
                //
                switch(pItem->fType)
                {
                    case LVRI_NUMBER:
                        pItem->fType = LVRI_INT;
                        if (FAILED(hResult))
                            pItem->dwValue = (DWORD)-2; // Indicates to caller "N/A".
                        break;
                    case LVRI_TEXT:
                        if (FAILED(hResult))
                            lstrcpyn(pItem->pszText, m_strNotApplicable, pItem->cchMaxText);
                        else
                            wsprintf(pItem->pszText, TEXT("%d"), pItem->dwValue);

                    default:
                        break;
                }
                break;
            }

            default:
                bResult = FALSE;
                break;
        }
    }
    return bResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::GetReportTitle

    Description: Retrieves a title for a report.  Uses the listview window
        title.

    Arguments:
        pszDest - Address of destination character buffer.

        cchDest - Size of destination buffer in characters.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/08/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
DetailsView::GetReportTitle(
    LPTSTR pszDest,
    UINT cchDest
    )
{
    //
    // This is simple.  Just use the details view title.
    // BUGBUG: Could be enhanced to include the date/time but that will
    //         require localization considerations.
    //
    GetWindowText(m_hwndMain, pszDest, cchDest);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::GetReportColHeader

    Description: Retrieves a title for a report column.  Note that the titles
        may differ from those used in the listview.  Specifically for the
        numeric columns.  In the listview, numeric column entries include
        units (bytes, KB, MB etc.).  In a report, these numeric values are
        all expressed in MB.  Therefore, the units must be included in the
        title string.

    Arguments:
        iColId - ID of column requested (idCol_Folder, idCol_Name etc.)

        pszDest - Address of destination character buffer.

        cchDest - Size of destination buffer in characters.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/08/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
DetailsView::GetReportColHeader(
    UINT iColId,
    LPTSTR pszDest,
    UINT cchDest
    )
{
    //
    // WARNING:  The order of these must match that of the idCol_XXX enumeration
    //           constants in DetailsView.
    //
    UINT rgTitles[] = { IDS_REPORT_HEADER_STATUS,
                        IDS_REPORT_HEADER_FOLDER,
                        IDS_REPORT_HEADER_USERNAME,
                        IDS_REPORT_HEADER_LOGONNAME,
                        IDS_REPORT_HEADER_AMTUSED,
                        IDS_REPORT_HEADER_LIMIT,
                        IDS_REPORT_HEADER_THRESHOLD,
                        IDS_REPORT_HEADER_PCTUSED };

    DBGASSERT((NULL != pszDest));
    DBGASSERT((1 < cchDest));
    CString strHeader(TEXT("..."));

    if (iColId < ARRAYSIZE(rgTitles))
    {
        strHeader.Format(g_hInstDll, rgTitles[iColId]);
    }
    else
    {
        DBGERROR((TEXT("LISTVIEW - Invalid idCol (%d) on header request"), iColId));
    }
    lstrcpyn(pszDest, strHeader, cchDest);
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::GetReportRowCount

    Description: Retrieves the number of data rows in the listview.

    Arguments: None.

    Returns: Number of rows in the listview.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/08/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
UINT
DetailsView::GetReportRowCount(VOID)
{
    return ListView_GetSelectedCount(m_hwndListView);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::GetReportBinaryRecordSize

    Description: Retrieves the number of bytes in a record formatted as
        binary data.  This should be called before GetReportBinaryRecord to
        determine how to size the destination buffer.

    Arguments:
        iRow - 0-based index of the row in question.

    Returns: Number of bytes required to store the record.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
UINT
DetailsView::GetReportBinaryRecordSize(
    UINT iRow
    )
{
    INT cbRecord = 0;

    PDISKQUOTA_USER pUser = NULL;

    DBGASSERT((0 <= iRow && iRow < m_UserList.Count()));

    if (m_UserList.Retrieve((LPVOID *)&pUser, iRow))
    {
        if (NULL != pUser)
        {
            pUser->GetSidLength((LPDWORD)&cbRecord); // Length of SID field.

            cbRecord += sizeof(DWORD)    +     // Sid-Length field.
                        sizeof(LONGLONG) +     // Quota used field.
                        sizeof(LONGLONG) +     // Quota threshold field.
                        sizeof(LONGLONG);      // Quota limit field.
        }
    }
    return cbRecord;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::GetReportBinaryRecord

    Description: Retrieves the information for a single row in the
        details view formatted as binary data.
        The format of the returned record is as follows:

        +------------+---------------------------------------+
        | cbSid (32) |       SID (variable length)           |
        +------------+------------+-------------+------------+
        |    Quota Used (64)      |  Quota Threshold (64)    |
        +------------+------------+-------------+------------+
        |    Quota Limit (64)     |
        +------------+------------+

        (*) The size of each field (bits) is shown in parentheses.


    Arguments:
        iRow - 0-based index of the row in question.

        pbRecord - Address of destination buffer.

        cbRecord - Number of bytes in destination buffer.

    Returns:
        TRUE  = Destination buffer was sufficiently large.
        FALSE = Destination buffer too small or record was invalid.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
DetailsView::GetReportBinaryRecord(
    UINT iRow,
    LPBYTE pbRecord,
    UINT cbRecord
    )
{
    //
    // Create "PMF" (pointer to member function) as a type of pointer
    // to the IDiskQuotaUser::GetQuotaXXXXXX functions.  This allows us
    // to build an array of function pointers and reduce the amount of
    // code required.
    //
    typedef HRESULT(_stdcall IDiskQuotaUser::*PMF)(PLONGLONG);

    PDISKQUOTA_USER pUser = NULL;
    BOOL bResult = FALSE;

    DBGASSERT((0 <= iRow && iRow < m_UserList.Count()));
    DBGASSERT((NULL != pbRecord));

    if (m_UserList.Retrieve((LPVOID *)&pUser, iRow))
    {
        DWORD cbSid = 0;
        if (NULL != pUser && cbRecord >= sizeof(cbSid))
        {
            //
            // Store the SID-length value first in the record.
            //
            pUser->GetSidLength((LPDWORD)&cbSid);
            *((LPDWORD)pbRecord) = cbSid;

            pbRecord += sizeof(cbSid);
            cbRecord -= sizeof(cbSid);

            //
            // Store the SID value next.
            //
            if (cbRecord >= cbSid && SUCCEEDED(pUser->GetSid(pbRecord, cbRecord)))
            {
                pbRecord += cbSid;
                cbRecord -= cbSid;
                //
                // An array of member function pointers.  Each function
                // retrieves a LONGLONG value from the quota user object.
                // This places the redundant code in a loop.
                //
                // The value order is Quota Used, Quota Threshold, Quota Limit.
                //
                PMF rgpfnQuotaValue[] = {
                    &IDiskQuotaUser::GetQuotaUsed,
                    &IDiskQuotaUser::GetQuotaThreshold,
                    &IDiskQuotaUser::GetQuotaLimit
                    };

                for (INT i = 0; i < ARRAYSIZE(rgpfnQuotaValue); i++)
                {
                    bResult = TRUE;
                    if (cbRecord >= sizeof(LONGLONG))
                    {
                        (pUser->*(rgpfnQuotaValue[i]))((PLONGLONG)pbRecord);
                        pbRecord += sizeof(LONGLONG);
                        cbRecord -= sizeof(LONGLONG);
                    }
                    else
                    {
                        //
                        // Insufficient buffer.
                        //
                        bResult = FALSE;
                        break;
                    }
                }
            }
        }
    }
    return bResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::GiveFeedback

    Description: Implementation for IDropSource::GiveFeedback.

    Arguments: See IDropSource::GiveFeedback in SDK.

    Returns: Always returns DRAGDROP_S_USEDEFAULTS.
        We don't have any special cursors for drag/drop.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::GiveFeedback(
    DWORD dwEffect
    )
{
    DBGPRINT((DM_DRAGDROP, DL_MID, TEXT("DRAGDROP - DropSource::GiveFeedback")));
    return DRAGDROP_S_USEDEFAULTCURSORS;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::QueryContinueDrag

    Description: Implementation for IDropSource::QueryContinueDrag

    Arguments: See IDropSource::QueryContinueDrag in SDK.

    Returns:
        DRAGDROP_S_CANCEL = User pressed ESC during drag.
        DRAGDROP_S_DROP   = User releases left mouse button.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::QueryContinueDrag(
    BOOL fEscapePressed,
    DWORD grfKeyState
    )
{
    HRESULT hResult = S_OK;

    DBGPRINT((DM_DRAGDROP, DL_MID, TEXT("DRAGDROP - DropSource::QueryContinueDrag")));
    if (fEscapePressed)
        hResult = DRAGDROP_S_CANCEL;
    else if (!(m_DropSource.m_grfKeyState & grfKeyState))
        hResult = DRAGDROP_S_DROP;

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::DragEnter

    Description: Implementation for IDropTarget::DragEnter

    Arguments: See IDropTarget::DragEnter in SDK.

    Returns: See IDropTarget::DragEnter in SDK.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::DragEnter(
    IDataObject *pDataObject,
    DWORD grfKeyState,
    POINTL pt,
    DWORD *pdwEffect
    )
{
    BOOL bWillAcceptDrop = FALSE;
    HRESULT hResult = NO_ERROR;
    IEnumFORMATETC *pEnum = NULL;

    *pdwEffect = DROPEFFECT_NONE;

    //
    // Enumerate formats supported by our data object.
    //
    hResult = pDataObject->EnumFormatEtc(DATADIR_GET, &pEnum);
    if (SUCCEEDED(hResult))
    {
        ULONG ulFetched = 0;
        FORMATETC fmt;

        //
        // Search the formats until we find an acceptable match.
        // We only accept our private export format along with
        // CF_HDROP in stream and HGLOBAL media types.
        //
        while(!bWillAcceptDrop && S_OK == pEnum->Next(1, &fmt, &ulFetched))
        {
            if (fmt.cfFormat == DataObject::m_CF_NtDiskQuotaExport || fmt.cfFormat == CF_HDROP)
            {
                if (fmt.tymed & (TYMED_HGLOBAL | TYMED_ISTREAM))
                {
                    bWillAcceptDrop = TRUE;
                }
            }
        }
        pEnum->Release();
    }
    if (SUCCEEDED(hResult))
    {
        hResult = NO_ERROR;
        if (bWillAcceptDrop)
        {
            *pdwEffect = (grfKeyState & MK_CONTROL ? DROPEFFECT_COPY :
                                                     DROPEFFECT_MOVE);
            m_DropTarget.m_pIDataObject = pDataObject;
            m_DropTarget.m_pIDataObject->AddRef();
        }
    }
    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::DragOver

    Description: Implementation for IDropTarget::DragOver

    Arguments: See IDropTarget::DragOver in SDK.

    Returns: See IDropTarget::DragOver in SDK.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::DragOver(
    DWORD grfKeyState,
    POINTL pt,
    DWORD *pdwEffect
    )
{
    if (NULL != m_DropTarget.m_pIDataObject)
    {
        *pdwEffect = (grfKeyState & MK_CONTROL ? DROPEFFECT_COPY :
                                                 DROPEFFECT_MOVE);
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
    }

    return NO_ERROR;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::DragLeave

    Description: Implementation for IDropTarget::DragLeave

    Arguments: See IDropTarget::DragLeave in SDK.

    Returns: See IDropTarget::DragLeave in SDK.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::DragLeave(
    VOID
    )
{
    if (NULL != m_DropTarget.m_pIDataObject)
    {
        m_DropTarget.m_pIDataObject->Release();
    }

    return NO_ERROR;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::Drop

    Description: Implementation for IDropTarget::Drop

    Arguments: See IDropTarget::Drop in SDK.

    Returns: See IDropTarget::Drop in SDK.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::Drop(
    IDataObject *pDataObject,
    DWORD grfKeyState,
    POINTL pt,
    DWORD *pdwEffect
    )
{
    HRESULT hResult = E_FAIL;

    *pdwEffect = DROPEFFECT_NONE;

    if (NULL != m_DropTarget.m_pIDataObject)
    {
        DragLeave();

        //
        // Import the quota data from the data object.
        //
        Importer importer(*this);
        hResult = importer.Import(pDataObject);

        if (SUCCEEDED(hResult))
        {
            if (grfKeyState & MK_CONTROL)
            {
                *pdwEffect = DROPEFFECT_COPY;
            }
        }
    }

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::GetData

    Description: Implementation of IDataObject::GetData

    Arguments: See IDataObject::GetData in SDK.

    Returns: See IDataObject::GetData in SDK.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DetailsView::GetData(
    FORMATETC *pFormatEtc,
    STGMEDIUM *pMedium
    )
{
    HRESULT hResult = E_INVALIDARG;

#if DBG

    TCHAR szCFName[MAX_PATH] = { TEXT('\0') };

    GetClipboardFormatName(pFormatEtc->cfFormat, szCFName, ARRAYSIZE(szCFName));
    DBGPRINT((DM_DRAGDROP, DL_MID, TEXT("DRAGDROP - DataObject::GetData\nDVA = %d  CF = %d (%s) tymed = %d"),
           pFormatEtc->dwAspect, pFormatEtc->cfFormat,
           szCFName,
           pFormatEtc->tymed));

#endif // DEBUG

    if (NULL != pFormatEtc && NULL != pMedium)
    {
        //
        // See if we support the requested format.
        //
        hResult = m_pDataObject->IsFormatSupported(pFormatEtc);
        if (SUCCEEDED(hResult))
        {
            //
            // Yep, we support it.  Render the data.
            //
            hResult = m_pDataObject->RenderData(pFormatEtc, pMedium);
        }
    }

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::GetDataHere

    Description: Implementation of IDataObject::GetDataHere

    Arguments: See IDataObject::GetData in SDK.

    Returns: E_NOTIMPL

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DetailsView::GetDataHere(
    FORMATETC *pFormatEtc,
    STGMEDIUM *pMedium
    )
{
    DBGTRACE((DM_DRAGDROP, DL_MID, TEXT("DRAGDROP - DataObject::GetDataHere")));
    return E_NOTIMPL;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::QueryGetData

    Description: Implementation of IDataObject::QueryGetData

    Arguments: See IDataObject::QueryGetData in SDK.

    Returns: See IDataObject::QueryGetData in SDK.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DetailsView::QueryGetData(
    FORMATETC *pFormatEtc
    )
{
    HRESULT hResult = E_UNEXPECTED;

#if DBG

    TCHAR szCFName[MAX_PATH] = { TEXT('\0') };

    GetClipboardFormatName(pFormatEtc->cfFormat, szCFName, ARRAYSIZE(szCFName));
    DBGPRINT((DM_DRAGDROP, DL_MID, TEXT("DRAGDROP - DataObject::QueryGetData\nDVA = %d  CF = %d (%s) tymed = %d"),
           pFormatEtc->dwAspect, pFormatEtc->cfFormat,
           szCFName,
           pFormatEtc->tymed));

#endif // DEBUG

    if (NULL != pFormatEtc)
    {
        hResult = m_pDataObject->IsFormatSupported(pFormatEtc);
    }
    else
        hResult = E_INVALIDARG;

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::GetCanonicalFormatEtc

    Description: Implementation of IDataObject::GetCanonicalFormatEtc

    Arguments: See IDataObject::GetCanonicalFormatEtc in SDK.

    Returns: See IDataObject::GetCanonicalFormatEtc in SDK.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DetailsView::GetCanonicalFormatEtc(
    FORMATETC *pFormatEtcIn,
    FORMATETC *pFormatEtcOut
    )
{
    DBGTRACE((DM_DRAGDROP, DL_MID, TEXT("DRAGDROP - DataObject::GetCanonicalFormatEtc")));

    HRESULT hResult = E_INVALIDARG;

    if (NULL != pFormatEtcIn && NULL != pFormatEtcOut)
    {
        CopyMemory(pFormatEtcOut, pFormatEtcIn, sizeof(*pFormatEtcOut));
        pFormatEtcOut->ptd = NULL;
        hResult = DATA_S_SAMEFORMATETC;
    }

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::SetData

    Description: Implementation of IDataObject::SetData

    Arguments: See IDataObject::SetData in SDK.

    Returns: E_NOTIMPL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DetailsView::SetData(
    FORMATETC *pFormatEtc,
    STGMEDIUM *pMedium,
    BOOL fRelease
    )
{
    DBGTRACE((DM_DRAGDROP, DL_MID, TEXT("DRAGDROP - DataObject::SetData")));
    return E_NOTIMPL;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::EnumFormatEtc

    Description: Implementation of IDataObject::EnumFormatEtc

    Arguments: See IDataObject::EnumFormatEtc in SDK.

    Returns: See IDataObject::GetCanonicalFormatEtc in SDK.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DetailsView::EnumFormatEtc(
    DWORD dwDirection,
    IEnumFORMATETC **ppenumFormatEtc
    )
{
    DBGTRACE((DM_DRAGDROP, DL_MID, TEXT("DRAGDROP - DataObject::EnumFormatEtc")));

    HRESULT hResult      = E_FAIL;
    EnumFORMATETC *pEnum = NULL;

    switch(dwDirection)
    {
        case DATADIR_GET:
            try
            {
                pEnum = new EnumFORMATETC(m_pDataObject->m_cFormats, m_pDataObject->m_rgFormats);
                hResult = pEnum->QueryInterface(IID_IEnumFORMATETC, (LPVOID *)ppenumFormatEtc);
            }
            catch(CAllocException& e)
            {
                *ppenumFormatEtc = NULL;
                hResult = E_OUTOFMEMORY;
            }
            break;

        case DATADIR_SET:
            //
            // SetData not implemented.
            //
        default:
            *ppenumFormatEtc = NULL;
            break;
    }
    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::DAdvise

    Description: Implementation of IDataObject::DAdvise

    Arguments: See IDataObject::DAdvise in SDK.

    Returns: E_NOTIMPL

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DetailsView::DAdvise(
    FORMATETC *pFormatEtc,
    DWORD advf,
    IAdviseSink *pAdvSink,
    DWORD *pdwConnection
    )
{
    DBGTRACE((DM_DRAGDROP, DL_MID, TEXT("DRAGDROP - DataObject::DAdvise")));
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::DUnadvise

    Description: Implementation of IDataObject::DUnadvise

    Arguments: See IDataObject::DUnadvise in SDK.

    Returns: E_NOTIMPL

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DetailsView::DUnadvise(
    DWORD dwConnection
    )
{
    DBGTRACE((DM_DRAGDROP, DL_MID, TEXT("DRAGDROP - DataObject::DUnadvise")));
    return E_NOTIMPL;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::EnumDAdvise

    Description: Implementation of IDataObject::EnumDAdvise

    Arguments: See IDataObject::EnumDAdvise in SDK.

    Returns: E_NOTIMPL

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DetailsView::EnumDAdvise(
    IEnumSTATDATA **ppenumAdvise
    )
{
    DBGTRACE((DM_DRAGDROP, DL_MID, TEXT("DRAGDROP - DataObject::EnumDAdvise")));
    return E_NOTIMPL;
}


//
// Number of clipboard formats supported by our data object.
// Change this if you add/remove clipboard formats.  There's an assert
// in the DataObject ctor to ensure this.
//
const INT DetailsView::DataObject::CF_FORMATS_SUPPORTED = 14;
//
// Name of data stream in import/export and dragdrop streams.
//
LPCWSTR DetailsView::DataObject::SZ_EXPORT_STREAM_NAME = L"NT DISKQUOTA IMPORTEXPORT";
LPCTSTR DetailsView::DataObject::SZ_EXPORT_CF_NAME     = TEXT("NT DISKQUTOA IMPORTEXPORT");

//
// The version of export data produced by this module.  This value
// is written into the stream immediately following the GUID.  If the
// format of the export stream is changed, this value should be incremented.
//
const DWORD DetailsView::DataObject::EXPORT_STREAM_VERSION = 1;

CLIPFORMAT DetailsView::DataObject::m_CF_Csv                 = 0; // Comma-separated fields format.
CLIPFORMAT DetailsView::DataObject::m_CF_RichText            = 0; // RTF format.
CLIPFORMAT DetailsView::DataObject::m_CF_NtDiskQuotaExport   = 0; // Internal fmt for import/export.
CLIPFORMAT DetailsView::DataObject::m_CF_FileGroupDescriptor = 0;
CLIPFORMAT DetailsView::DataObject::m_CF_FileContents        = 0;


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::DataObject::DataObject

    Description: Constructor for implementation of IDataObject.

    Arguments:
        DV - Reference to details view object that contains the data object.

    Returns: Nothing

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
DetailsView::DataObject::DataObject(
    DetailsView& DV
    ) : m_pStg(NULL),
        m_pStm(NULL),
        m_rgFormats(NULL),
        m_cFormats(CF_FORMATS_SUPPORTED),
        m_DV(DV)
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("DetailsView::DataObject::DataObject")));
    DBGPRINT((DM_VIEW, DL_HIGH, TEXT("\tthis = 0x%08X"), this));

    //
    // Get additional clipboard formats we support.
    //
    if (0 == m_CF_Csv)
    {
        m_CF_Csv               = (CLIPFORMAT)RegisterClipboardFormat(TEXT("Csv"));
        DBGPRINT((DM_DRAGDROP, DL_MID, TEXT("DRAGDROP - Csv CF            = %d"), m_CF_Csv));
    }

    if (0 == m_CF_RichText)
    {
        m_CF_RichText          = (CLIPFORMAT)RegisterClipboardFormat(TEXT("Rich Text Format"));
        DBGPRINT((DM_DRAGDROP, DL_MID, TEXT("DRAGDROP - RTF CF            = %d"), m_CF_RichText));
    }

    if (0 == m_CF_NtDiskQuotaExport)
    {
        m_CF_NtDiskQuotaExport = (CLIPFORMAT)RegisterClipboardFormat(DataObject::SZ_EXPORT_CF_NAME);
        DBGPRINT((DM_DRAGDROP, DL_MID, TEXT("DRAGDROP - NtDiskQuotaExport = %d"), m_CF_NtDiskQuotaExport));
    }

    if (0 == m_CF_FileGroupDescriptor)
    {
        m_CF_FileGroupDescriptor = (CLIPFORMAT)RegisterClipboardFormat(TEXT("FileGroupDescriptorW"));
        DBGPRINT((DM_DRAGDROP, DL_MID, TEXT("DRAGDROP - FileGroupDescriptorW = %d"), m_CF_FileGroupDescriptor));
    }

    if (0 == m_CF_FileContents)
    {
        m_CF_FileContents = (CLIPFORMAT)RegisterClipboardFormat(TEXT("FileContents"));
        DBGPRINT((DM_DRAGDROP, DL_MID, TEXT("DRAGDROP - FileContents = %d"), m_CF_FileContents));
    }

    //
    // Create the array to hold the FORMATETC structures that describe the
    // formats we support.
    //
    m_rgFormats = new FORMATETC[m_cFormats];

    //
    // Specify all formats and media we support.
    // Place the richest formats first in the array.
    // These are used to initialize the format enumerator when it
    // is requested.
    //
    UINT iFmt = 0;

    SetFormatEtc(m_rgFormats[iFmt++], m_CF_FileGroupDescriptor, TYMED_ISTREAM);
    SetFormatEtc(m_rgFormats[iFmt++], m_CF_FileGroupDescriptor, TYMED_HGLOBAL);
    SetFormatEtc(m_rgFormats[iFmt++], m_CF_FileContents,        TYMED_ISTREAM);
    SetFormatEtc(m_rgFormats[iFmt++], m_CF_FileContents,        TYMED_HGLOBAL);
    SetFormatEtc(m_rgFormats[iFmt++], m_CF_NtDiskQuotaExport,   TYMED_ISTREAM);
    SetFormatEtc(m_rgFormats[iFmt++], m_CF_NtDiskQuotaExport,   TYMED_HGLOBAL);
    SetFormatEtc(m_rgFormats[iFmt++], m_CF_RichText,            TYMED_ISTREAM);
    SetFormatEtc(m_rgFormats[iFmt++], m_CF_RichText,            TYMED_HGLOBAL);
    SetFormatEtc(m_rgFormats[iFmt++], m_CF_Csv,                 TYMED_ISTREAM);
    SetFormatEtc(m_rgFormats[iFmt++], m_CF_Csv,                 TYMED_HGLOBAL);
    SetFormatEtc(m_rgFormats[iFmt++], CF_UNICODETEXT,           TYMED_ISTREAM);
    SetFormatEtc(m_rgFormats[iFmt++], CF_UNICODETEXT,           TYMED_HGLOBAL);
    SetFormatEtc(m_rgFormats[iFmt++], CF_TEXT,                  TYMED_ISTREAM);
    SetFormatEtc(m_rgFormats[iFmt++], CF_TEXT,                  TYMED_HGLOBAL);

    //
    // If you hit this, you need to adjust CF_FORMATS_SUPPORTED to match
    // the number of SetFormatEtc statements above.
    // Otherwise, you just overwrote the m_rgFormats[] allocation.
    //
    DBGASSERT((iFmt == m_cFormats));
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::DataObject::~DataObject

    Description: Destructor for implementation of IDataObject.

    Arguments: None.

    Returns: Nothing

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
DetailsView::DataObject::~DataObject(
    VOID
    )
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("DetailsView::DataObject::~DataObject")));
    DBGPRINT((DM_VIEW, DL_HIGH, TEXT("\tthis = 0x%08X"), this));

    delete[] m_rgFormats;
    if (NULL != m_pStg)
        m_pStg->Release();
    //
    // NOTE:  m_pStm is released by the data object's recipient
    //        through ReleaseStgMedium.
    //
}

///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::DataObject::IsFormatSupported

    Description: Determines if a given format is supported by our implementation.

    Arguments:
        pFormatEtc - Address of FORMATETC structure containing request info.

    Returns:
        NO_ERROR       - Supported.
        DV_E_TYMED     - Medium type not supported.
        DV_E_FORMATETC - Clipboard format not supported.
        DV_E_DVASPECT  - Device aspect not supported.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/10/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::DataObject::IsFormatSupported(
    FORMATETC *pFormatEtc
    )
{
    DBGASSERT((NULL != pFormatEtc));
    HRESULT hResult = E_FAIL;

    if (DVASPECT_CONTENT == pFormatEtc->dwAspect)
    {
        if (CF_TEXT                  == pFormatEtc->cfFormat ||
            CF_UNICODETEXT           == pFormatEtc->cfFormat ||
            m_CF_RichText            == pFormatEtc->cfFormat ||
            m_CF_Csv                 == pFormatEtc->cfFormat ||
            m_CF_NtDiskQuotaExport   == pFormatEtc->cfFormat ||
            m_CF_FileGroupDescriptor == pFormatEtc->cfFormat ||
            m_CF_FileContents        == pFormatEtc->cfFormat)
        {
            if (pFormatEtc->tymed & (TYMED_ISTREAM | TYMED_HGLOBAL))
            {
                hResult = NO_ERROR;
            }
            else
            {
                hResult = DV_E_TYMED;
            }
        }
        else
            hResult = DV_E_FORMATETC;
    }
    else
        hResult = DV_E_DVASPECT;

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::DataObject::CreateRenderStream

    Description: Creates the OLE stream on which the data is to be rendered.

    Arguments:
        tymed - Desired medium type.

        ppStm - Address of IStream pointer variable to receive the stream ptr.

    Returns:
        NO_ERROR       - Success.
        E_INVALIDARG   - Invalid medium type.
        E_OUTOFMEMORY  - Insufficient memory.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/30/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::DataObject::CreateRenderStream(
    DWORD tymed,
    IStream **ppStm
    )
{
    HRESULT hResult = NOERROR;

    //
    // Create the Stream.
    //
    if (TYMED_ISTREAM & tymed)
    {
        DBGPRINT((DM_DRAGDROP, DL_MID, TEXT("DRAGDROP - CreateRenderStream for ISTREAM")));
        hResult = CreateStreamOnHGlobal(NULL,       // Block of 0 bytes.
                                        TRUE,       // Delete on release.
                                        ppStm);
    }
    else if (TYMED_HGLOBAL & tymed)
    {
        DBGPRINT((DM_DRAGDROP, DL_MID, TEXT("DRAGDROP - CreateRenderStream for HGLOBAL")));
        hResult = CreateStreamOnHGlobal(NULL,       // Block of 0 bytes.
                                        TRUE,       // Delete on release.
                                        ppStm);
    }
    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::DataObject::RenderData [private]

    Description: Renders the data in the Details View onto the provided
        stream using the requested clipboard format.

    Arguments:
        pStm - Pointer to output stream.

        cf - Desired clipboard format.

    Returns:
        NO_ERROR         - Success.
        E_FAIL           - General failure.
        STG_E_WRITEFAULT - Media write error.
        STG_E_MEDIUMFULL - Insufficient space on medium.
        E_ACCESSDENIED   - Write access denied.
        E_OUTOFMEMORY    - Insufficient memory.
        E_UNEXPECTED     - Unexpected exception.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/30/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::DataObject::RenderData(
    IStream *pStm,
    CLIPFORMAT cf
    )
{
    HRESULT hResult     = NOERROR;
    Renderer *pRenderer = NULL;

    DBGPRINT((DM_DRAGDROP, DL_MID, TEXT("DetailsView::DataObject::RenderData on stream")));
    try
    {
        //
        // Create the properly-typed rendering object for the requested format.
        //
        switch(cf)
        {
            case CF_TEXT:
                DBGPRINT((DM_DRAGDROP, DL_MID, TEXT("Format is CF_TEXT")));
                pRenderer = new Renderer_TEXT(m_DV);
                break;

            case CF_UNICODETEXT:
                DBGPRINT((DM_DRAGDROP, DL_MID, TEXT("Format is CF_UNICODETEXT")));
                pRenderer = new Renderer_UNICODETEXT(m_DV);
                break;

            default:
                if (m_CF_RichText == cf)
                {
                    DBGPRINT((DM_DRAGDROP, DL_MID, TEXT("Format is RTF")));
                    pRenderer = new Renderer_RTF(m_DV);
                }
                else if (m_CF_Csv == cf)
                {
                    DBGPRINT((DM_DRAGDROP, DL_MID, TEXT("Format is Csv")));
                    pRenderer = new Renderer_Csv(m_DV);
                }
                else if (m_CF_NtDiskQuotaExport == cf)
                {
                    DBGPRINT((DM_DRAGDROP, DL_MID, TEXT("Format is Windows NT Disk Quota ImportExport Format")));
                    pRenderer = new Renderer_Export(m_DV);
                }
                else if (m_CF_FileGroupDescriptor == cf)
                {
                    DBGPRINT((DM_DRAGDROP, DL_MID, TEXT("Format is FileGroupDescriptor")));
                    pRenderer = new Renderer_FileGroupDescriptor(m_DV);
                }
                else if (m_CF_FileContents == cf)
                {
                    DBGPRINT((DM_DRAGDROP, DL_MID, TEXT("Format is FileContents")));
                    pRenderer = new Renderer_FileContents(m_DV);
                }
                else
                {
                    DBGPRINT((DM_DRAGDROP, DL_MID, TEXT("Unknown CF format (%d) requested"), cf));
                }
                break;
        }

        if (NULL != pRenderer)
        {
            m_pStm->AddRef();      // Giving stream to renderer.
                                   // Will be released when renderer is destroyed.
            //
            // Render the information onto the stream.
            // This can throw FileError exceptions if we run out of disk
            // space or there's a disk write error.
            //
            pRenderer->Render(m_pStm);
        }
    }
    catch(CFileException& fe)
    {
        switch(fe.Reason())
        {
            case CFileException::write:
                DBGERROR((TEXT("FileWrite error")));
                hResult = E_FAIL;
                break;
            case CFileException::device:
                DBGERROR((TEXT("Disk error")));
                hResult = STG_E_WRITEFAULT;
                break;
            case CFileException::diskfull:
                DBGERROR((TEXT("Disk Full error")));
                hResult = STG_E_MEDIUMFULL;
                break;
            case CFileException::access:
                DBGERROR((TEXT("Access Denied error")));
                hResult = E_ACCESSDENIED;
                break;
            default:
                DBGERROR((TEXT("Other error")));
                hResult = E_FAIL;
                break;
        }
    }
    catch(CAllocException& e)
    {
        DBGERROR((TEXT("Insufficient memory")));
        hResult = E_OUTOFMEMORY;
    }

    delete pRenderer;

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::DataObject::RenderData [public]

    Description: Renders the data in the Details View onto the requested
        medium using the requested format.

    Arguments:
        pFormatEtc - Address of FORMATETC structure containing request info.

        pMedium - Address of STGMEDIUM structure containing requested
            medium info.

    Returns:
        NO_ERROR       - Success.
        Can return many other OLE drag/drop error codes.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/10/96    Initial creation.                                    BrianAu
    07/30/97    Reworked.  Moved some code into CreateRenderStream   BrianAu
                and CreateAndRunRenderer.  Makes the function
                more understandable.
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::DataObject::RenderData(
    FORMATETC *pFormatEtc,
    STGMEDIUM *pMedium
    )
{
    DBGASSERT((NULL != pFormatEtc));
    DBGASSERT((NULL != pMedium));
    DBGASSERT((SUCCEEDED(IsFormatSupported(pFormatEtc))));

    HRESULT hResult     = NOERROR;
    Renderer *pRenderer = NULL;

    DBGPRINT((DM_DRAGDROP, DL_HIGH, TEXT("DetailsView::DataObject::RenderData on medium")));

    //
    // Create the stream we'll render the data onto.
    //
    hResult = CreateRenderStream(pFormatEtc->tymed, &m_pStm);
    if (SUCCEEDED(hResult))
    {
        //
        // Render the data on the stream.
        //
        hResult = RenderData(m_pStm, pFormatEtc->cfFormat);

        if (SUCCEEDED(hResult))
        {
            //
            // Position the stream's read/write ptr to the
            // start of the stream so whoever get's it as a data object can start
            // reading from the beginning.
            //
            LARGE_INTEGER liSeek = {0, 0};
            m_pStm->Seek(liSeek, STREAM_SEEK_SET, NULL);

            //
            // If we've made it here, we have a valid drag-drop report on m_pStm.
            // Now set up the stg medium to transfer the rendering.
            //
            if (TYMED_ISTREAM & pFormatEtc->tymed)
            {
                pMedium->pstm           = m_pStm;
                pMedium->tymed          = TYMED_ISTREAM;
                pMedium->pUnkForRelease = NULL;          // Target will free the Stream.
            }
            else if (TYMED_HGLOBAL & pFormatEtc->tymed)
            {
                pMedium->tymed          = TYMED_HGLOBAL;
                pMedium->pUnkForRelease = NULL;          // Target will free the mem.
                hResult = GetHGlobalFromStream(m_pStm,
                                               &pMedium->hGlobal);
            }
            else
            {
                //
                // Call to CreateRenderStream() should have failed if we
                // hit this.
                //
                DBGASSERT((0));
            }
        }
        if (FAILED(hResult))
        {
            DBGERROR((TEXT("DRAGDROP - Error 0x%08X rendering data"), hResult));

            //
            // Something failed after the stream was created.
            // The DetailsView::DataObject dtor DOES NOT release it.
            // It assumes success and assumes the recipient will release it.
            // Release the stream.
            //
            m_pStm->Release();
            //
            // These two statements are redundant since pMedium contains a union.
            // I didn't want any more if(STREAM) else if (HGLOBAL) logic.  In case
            // there's ever a change in structure, this will ensure both possible
            // medium types are null'd out.
            //
            pMedium->pstm    = NULL;
            pMedium->hGlobal = NULL;
        }
    }
    else
    {
        DBGERROR((TEXT("DRAGDROP - Error 0x%08X creating stream"), hResult);)
    }

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::DataObject::SetFormatEtc [static]

    Description: Helper function to set the members of a FORMATETC
        structure.  Uses defaults for least used members.

    Arguments: See SDK description of FORMATETC.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
DetailsView::DataObject::SetFormatEtc(
    FORMATETC& fe,
    CLIPFORMAT cfFormat,
    DWORD tymed,
    DWORD dwAspect,
    DVTARGETDEVICE *ptd,
    LONG lindex
    )
{
    fe.cfFormat = cfFormat;
    fe.dwAspect = dwAspect;
    fe.ptd      = ptd;
    fe.tymed    = tymed;
    fe.lindex   = lindex;
};


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::DataObject::WideToAnsi

    Description: Helper function to convert a wide character string to ANSI.
        The caller must delete the return buffer.

    Arguments:
        pszTextW - UNICODE string to convert.

    Returns: Address of ANSI string.  Caller must delete this.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LPSTR
DetailsView::DataObject::WideToAnsi(
    LPCWSTR pszTextW
    )
{
    DBGASSERT((NULL != pszTextW));

    INT cchTextA = WideCharToMultiByte(CP_ACP,
                                       0,
                                       pszTextW,
                                       -1,
                                       NULL,
                                       0,
                                       NULL,
                                       NULL);

    LPSTR pszTextA = new CHAR[cchTextA + 1];

    WideCharToMultiByte(CP_ACP,
                        0,
                        pszTextW,
                        -1,
                        pszTextA,
                        cchTextA + 1,
                        NULL,
                        NULL);
    return pszTextA;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::DataObject::Renderer::Render

    Description: Render the selected items in the listview on a stream.
        Calls virtual functions defined by derived classes to produce the
        required format.

    Arguments:
        pStm - Address of IStream on which to write output.
            Assumes that this pointer has been AddRef'd by the caller.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
DetailsView::DataObject::Renderer::Render(
    IStream *pStm
    )
{
    HRESULT hResult = NO_ERROR;
    WCHAR szText[MAX_PATH];
    INT rgColIds[DetailsView::idCol_Last];
    INT cCols = m_DV.GetColumnIds(rgColIds, ARRAYSIZE(rgColIds));
    INT cRows = m_DV.GetReportRowCount();
    INT i, j;
    INT iRow = -1;

    DBGASSERT((NULL != pStm));
    m_Stm.SetStream(pStm);

    //
    // Start the report.
    //
    Begin(cRows, cCols);

    //
    // Add the report title.
    //
    m_DV.GetReportTitle(szText, ARRAYSIZE(szText));
    AddTitle(szText);

    //
    // Add the report column headers.
    //
    BeginHeaders();
    for (i = 0; i < cCols; i++)
    {
        m_DV.GetReportColHeader(rgColIds[i], szText, ARRAYSIZE(szText));
        AddHeader(szText);
        AddHeaderSep();
    }
    EndHeaders();

    //
    // Add the report row/col data.
    //
    for (i = 0; i < cRows; i++)
    {
        iRow = m_DV.GetNextSelectedItemIndex(iRow);
        DBGASSERT((-1 != iRow));
        BeginRow();
        for (j = 0; j < cCols; j++)
        {
            AddRowColData(iRow, rgColIds[j]);
            AddRowColSep();
        }
        EndRow();
    }

    //
    // Terminate the report.
    //
    End();
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::DataObject::Renderer::Stream::Stream

    Description: Constructor for the renderer's private stream object.
        The object is used to encapsulate stream write operations in overloaded
        type-sensitive member functions.

    Arguments:
        pStm - Address of IStream associated with the object.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
DetailsView::DataObject::Renderer::Stream::Stream(
    IStream *pStm
    ) : m_pStm(pStm)
{

#ifdef CLIPBOARD_DEBUG_OUTPUT
    m_pStgDbgOut = NULL;
    m_pStmDbgOut = NULL;
    StgCreateDocfile(TEXT("\\DskquotaClipboard.Out"),
                     STGM_CREATE |
                     STGM_READWRITE |
                     STGM_SHARE_EXCLUSIVE,
                     0,
                     &m_pStgDbgOut);
    if (NULL != m_pStgDbgOut)
    {
        m_pStgDbgOut->CreateStream(TEXT("Clipboard Data"),
                                   STGM_CREATE |
                                   STGM_READWRITE |
                                   STGM_SHARE_EXCLUSIVE,
                                   0, 0,
                                   &m_pStmDbgOut);
    }
#endif // CLIPBOARD_DEBUG_OUTPUT
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::DataObject::Renderer::Stream::~Stream

    Description: Destructor for the renderer's private stream object.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
DetailsView::DataObject::Renderer::Stream::~Stream(VOID)
{
    if (NULL != m_pStm)
        m_pStm->Release();

#ifdef CLIPBOARD_DEBUG_OUTPUT

    if (NULL != m_pStmDbgOut)
        m_pStmDbgOut->Release();
    if (NULL != m_pStgDbgOut)
        m_pStgDbgOut->Release();

#endif // CLIPBOARD_DEBUG_OUTPUT
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::DataObject::Renderer::Stream::SetStream

    Description: Associates an IStream pointer with the stream object.
        Releases an existing pointer if one was already assigned.

    Arguments:
        pStm - Address of new IStream to associate with stream object.
            Caller must AddRef IStream pointer before passing to this function.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
DetailsView::DataObject::Renderer::Stream::SetStream(
    IStream *pStm
    )
{
    DBGASSERT((NULL != pStm));

    if (NULL != m_pStm)
        m_pStm->Release();
    m_pStm = pStm;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::DataObject::Renderer::Stream::Write

    Description: Set of overloaded functions to handle
        the writing of various types of data to the stream.

    Arguments:
        pbData - Address of BYTE buffer for source data.

        cbData - Number of bytes in pbData[]

        pszTextA - Ansi text string for source data.

        pszTextW - Wide character text string for source data.

        bData - Byte to write to stream.

        chDataW - Wide character to write to stream.

        chDataA - Ansi character to write to stream.

        dwData - DWORD-type data to write to stream.

        dblData - double-type data to write to stream.


    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
DetailsView::DataObject::Renderer::Stream::Write(
    LPBYTE pbData,
    UINT cbData
    )
{
    DBGASSERT((NULL != pbData));

    ULONG cbWritten = 0;
    HRESULT hr;

    hr = m_pStm->Write(pbData, cbData, &cbWritten);
    if (S_OK != hr)
    {
        DBGERROR((TEXT("Error 0x%08X writing to output stream."), hr));
        CFileException::reason reason = CFileException::write;
        switch(hr)
        {
            case STG_E_ACCESSDENIED:
                reason = CFileException::access;
                break;
            case STG_E_MEDIUMFULL:
                reason = CFileException::diskfull;
                break;
            case STG_E_WRITEFAULT:
                reason = CFileException::device;
                break;
            default:
                //
                // Use default value.
                //
                break;
        }
        throw CFileException(reason, TEXT(""), 0);
    }

#ifdef CLIPBOARD_DEBUG_OUTPUT

    cbWritten = 0;
    if (S_OK != m_pStmDbgOut->Write(pbData, cbData, &cbWritten))
        throw CFileException(CFileException::write, TEXT(""), 0);

#endif  // CLIPBOARD_DEBUG_OUTPUT
}


VOID
DetailsView::DataObject::Renderer::Stream::Write(
    LPCWSTR pszTextW
    )
{
    Write((LPBYTE)pszTextW, lstrlenW(pszTextW) * sizeof(WCHAR));
}

VOID
DetailsView::DataObject::Renderer::Stream::Write(
    LPCSTR pszTextA
    )
{
    Write((LPBYTE)pszTextA, lstrlenA(pszTextA) * sizeof(CHAR));
}


VOID
DetailsView::DataObject::Renderer::Stream::Write(
    BYTE bData
    )
{
    Write((LPBYTE)&bData, sizeof(bData));
}

VOID
DetailsView::DataObject::Renderer::Stream::Write(
    WCHAR chDataW
    )
{
    Write((LPBYTE)&chDataW, sizeof(chDataW));
}


VOID
DetailsView::DataObject::Renderer::Stream::Write(
    CHAR chDataA
    )
{
    Write((LPBYTE)&chDataA, sizeof(chDataA));
}


VOID
DetailsView::DataObject::Renderer::Stream::Write(
    DWORD dwData
    )
{
    Write((LPBYTE)&dwData, sizeof(dwData));
}


VOID
DetailsView::DataObject::Renderer::Stream::Write(
    double dblData
    )
{
    Write((LPBYTE)&dblData, sizeof(dblData));
}


///////////////////////////////////////////////////////////////////////////////
//
// The following section of code contains the different implementations of
// the virtual rendering functions that make each type of rendering object
// unique.  Since they're pretty self-explanatory, I haven't commented each
// function.  It should be obvious as to what they do.
// I have separated each rendering-type section with a banner comment for
// readability.  [brianau]
//
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// CF_UNICODETEXT
///////////////////////////////////////////////////////////////////////////////
VOID
DetailsView::DataObject::Renderer_UNICODETEXT::AddTitle(
    LPCTSTR pszTitle
    )
{
    m_Stm.Write(pszTitle);
    m_Stm.Write(TEXT('\n'));
    m_Stm.Write(TEXT('\n'));
}


VOID
DetailsView::DataObject::Renderer_UNICODETEXT::AddRowColData(
    INT iRow,
    INT idCol
    )
{
    WCHAR szText[MAX_PATH];
    LV_REPORT_ITEM item;
    item.fType      = LVRI_TEXT;  // Want text data.
    item.pszText    = szText;
    item.cchMaxText = ARRAYSIZE(szText);

    m_DV.GetReportItem(iRow, idCol, &item);
    m_Stm.Write(szText);
}


///////////////////////////////////////////////////////////////////////////////
// CF_TEXT
///////////////////////////////////////////////////////////////////////////////
VOID
DetailsView::DataObject::Renderer_TEXT::AddTitle(
    LPCWSTR pszTitleW
    )
{
    array_autoptr<CHAR> ptrTitleA(DataObject::WideToAnsi(pszTitleW));
    m_Stm.Write(ptrTitleA);
    m_Stm.Write('\n');
    m_Stm.Write('\n');
}


VOID
DetailsView::DataObject::Renderer_TEXT::AddHeader(
    LPCWSTR pszHeaderW
    )
{
    array_autoptr<CHAR> ptrHeaderA(DataObject::WideToAnsi(pszHeaderW));
    m_Stm.Write(ptrHeaderA);
}


VOID
DetailsView::DataObject::Renderer_TEXT::AddRowColData(
    INT iRow,
    INT idCol
    )
{
    WCHAR szTextW[MAX_PATH];
    LV_REPORT_ITEM item;
    item.fType      = LVRI_TEXT;  // Want text data.
    item.pszText    = szTextW;
    item.cchMaxText = ARRAYSIZE(szTextW);

    m_DV.GetReportItem(iRow, idCol, &item);

    array_autoptr<CHAR> ptrTextA(DataObject::WideToAnsi(szTextW));
    m_Stm.Write(ptrTextA);
}


///////////////////////////////////////////////////////////////////////////////
// RTF (Rich Text)
///////////////////////////////////////////////////////////////////////////////
static const INT TWIPS_PER_PT    = 20;
static const INT PTS_PER_INCH    = 72;
static const INT TWIPS_PER_INCH  = PTS_PER_INCH * TWIPS_PER_PT;
static const INT COL_WIDTH_TWIPS = TWIPS_PER_INCH * 5 / 4;       // 1 1/4 inches.

//
// Converts all single backslashes to double backslashes.
// Literal backslashes in RTF must be "\\".
// Caller must delete[] the returned buffer.
//
LPSTR
DetailsView::DataObject::Renderer_RTF::DoubleBackslashes(
    LPSTR pszSrc
    )
{
    DBGASSERT((NULL != pszSrc));

    //
    // Create new string for output.  Size must be double.  Every char
    // could be '\'.
    //
    LPSTR pszFormatted = new CHAR[(lstrlenA(pszSrc) * 2) + 1];
    LPSTR pszDest      = pszFormatted;

    while('\0' != *pszSrc)
    {
        if ('\\' == *pszSrc)
            *pszDest++ = *pszSrc;
        *pszDest++ = *pszSrc++;
    }

    *pszDest = *pszSrc; // Pick up NUL terminator.
    return pszFormatted;
}


VOID
DetailsView::DataObject::Renderer_RTF::Begin(
    INT cRows,
    INT cCols
    )
{
    m_cCols = cCols;
    m_Stm.Write("{\\rtf1 \\sect\\sectd\\lndscpsxn \\par\\pard\\plain ");
}



VOID
DetailsView::DataObject::Renderer_RTF::AddTitle(
    LPCWSTR pszTitleW
    )
{
    array_autoptr<CHAR> ptrTempA(DataObject::WideToAnsi(pszTitleW));
    array_autoptr<CHAR> ptrTitleA(DoubleBackslashes(ptrTempA));      // cvt '\' to "\\"
    m_Stm.Write(ptrTitleA);
}



VOID DetailsView::DataObject::Renderer_RTF::BeginHeaders(
    VOID
    )
{
    m_Stm.Write(" \\par \\par ");   // Hdr preceded by empty row.
    BeginHeaderOrRow();             // Add stuff common to hdr and data rows.
    m_Stm.Write(" \\trhdr ");       // Hdr at top of each page.
    AddCellDefs();                  // Cell size definitions.
}



VOID DetailsView::DataObject::Renderer_RTF::AddCellDefs(
    VOID
    )
{
    char szText[80];
    INT cxTwips = 0;

    for (INT i = 0; i < m_cCols; i++)
    {
        cxTwips += COL_WIDTH_TWIPS;
        wsprintfA(szText, "\\cellx%d", cxTwips);
        m_Stm.Write(szText);
    }
    m_Stm.Write(' ');
}


//
// Stuff common to both header row and data rows.
//
VOID DetailsView::DataObject::Renderer_RTF::BeginHeaderOrRow(
    VOID
    )
{
    m_Stm.Write("\\trowd \\pard \\intbl ");
}



VOID
DetailsView::DataObject::Renderer_RTF::AddHeader(
    LPCWSTR pszHeaderW
    )
{
    array_autoptr<CHAR> ptrHeaderA(DataObject::WideToAnsi(pszHeaderW));
    //
    // No need to convert '\' to "\\".  No
    // backslashes in our header text.
    //
    m_Stm.Write(ptrHeaderA);
}


VOID
DetailsView::DataObject::Renderer_RTF::AddRowColData(
    INT iRow,
    INT idCol
    )
{
    WCHAR szTextW[MAX_PATH];
    LV_REPORT_ITEM item;
    item.fType      = LVRI_TEXT;  // Want text data.
    item.pszText    = szTextW;
    item.cchMaxText = ARRAYSIZE(szTextW);

    m_DV.GetReportItem(iRow, idCol, &item);

    array_autoptr<CHAR> ptrTempA(DataObject::WideToAnsi(szTextW));
    array_autoptr<CHAR> ptrTextA(DoubleBackslashes(ptrTempA));      // cvt '\' to "\\"
    m_Stm.Write(ptrTextA);
}


///////////////////////////////////////////////////////////////////////////////
// Private import/export format
///////////////////////////////////////////////////////////////////////////////
//
// Assumes that caller AddRef'd IStream pointer.
//
VOID
DetailsView::DataObject::Renderer_Export::Render(
    IStream *pStm
    )
{
    HRESULT hResult = NO_ERROR;
    INT cRows = m_DV.GetReportRowCount();
    INT iRow = -1;

    DBGASSERT((NULL != pStm));
    m_Stm.SetStream(pStm);

    Begin(cRows, 0);

    //
    // Add the export data records.
    //
    for (INT i = 0; i < cRows; i++)
    {
        iRow = m_DV.GetNextSelectedItemIndex(iRow);
        DBGASSERT((-1 != iRow));
        AddBinaryRecord(iRow);
    }

    //
    // Terminate the report.
    //
    End();
}

VOID
DetailsView::DataObject::Renderer_Export::Begin(
    INT cRows,
    INT cCols
    )
{
    //
    // The stream header contains a GUID as a unique identifier followed
    // by a version number.
    //
    m_Stm.Write((LPBYTE)&GUID_NtDiskQuotaStream, sizeof(GUID_NtDiskQuotaStream));
    m_Stm.Write(DataObject::EXPORT_STREAM_VERSION);
    m_Stm.Write((DWORD)cRows);
}


VOID
DetailsView::DataObject::Renderer_Export::AddBinaryRecord(
    INT iRow
    )
{
    INT cbRecord    = m_DV.GetReportBinaryRecordSize(iRow);
    array_autoptr<BYTE> ptrRecord(new BYTE[cbRecord]);
    if (NULL != ptrRecord.get())
    {
        if (m_DV.GetReportBinaryRecord(iRow, ptrRecord, cbRecord))
        {
            m_Stm.Write(ptrRecord, cbRecord);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// CF "FileGroupDescriptor"
//
//
///////////////////////////////////////////////////////////////////////////////

VOID
DetailsView::DataObject::Renderer_FileGroupDescriptor::Begin(
    INT cRows,
    INT cCols
    )
{
    //
    // Build a name for the file we'll create.
    //
    // Vol label?  Filename
    // ----------  -------------------------------------------------------
    //  Yes        "Disk Quota Settings for Volume 'VOL_LABEL'"
    //  No         "Disk Quota Settings for Unlabeled Volume SN 8AB1-DE23"
    //
    // The serial-number format is gross but without a label, we don't have
    // any other distinguishing feature for the volume.  I'd use the
    // display name from the CVolumeID object but in the mounted volume
    // case, it contains backslashes and a colon; both invalid buried in
    // a filename.
    //
    TCHAR szLabel[MAX_VOL_LABEL] = { TEXT('\0') };
    DWORD dwSerialNumber = 0;
    GetVolumeInformation(m_DV.GetVolumeID().ForParsing(),
                         szLabel,
                         ARRAYSIZE(szLabel),
                         &dwSerialNumber,
                         NULL,
                         NULL,
                         NULL,
                         0);

    CString strFileName;
    if (TEXT('\0') != szLabel[0])
    {
        //
        // Volume has a label.
        //
        strFileName.Format(g_hInstDll,
                           IDS_EXPORT_STREAM_FILENAME_TEMPLATE,
                           szLabel);
    }
    else
    {
        //
        // No volume label.
        //
        strFileName.Format(g_hInstDll,
                           IDS_EXPORT_STREAM_FILENAME_TEMPLATE_VOLSN,
                           HIWORD(dwSerialNumber),
                           LOWORD(dwSerialNumber));
    }

    //
    // Create a file group descriptor containing the name we want the
    // shell to use for the file.  The descriptor contains one file
    // description.  That description just contains the file name.
    // All other members are initialized to 0.
    //
    FILEGROUPDESCRIPTORW desc;

    ZeroMemory(&desc, sizeof(desc));
    desc.cItems = 1;
    lstrcpyn(desc.fgd[0].cFileName, strFileName, ARRAYSIZE(desc.fgd[0].cFileName));

    //
    // Write the file group descriptor to the renderer's stream.
    //
    m_Stm.Write((LPBYTE)&desc, sizeof(desc));
}





///////////////////////////////////////////////////////////////////////////////
/*  Function: LVSelection::Add

    Description: Add a user pointer and listview item index to a listview
        selection object.  This object is used to transfer the notion of a
        "selection" to some function.

    Arguments:
        pUser - Address of IDiskQuotaUser interface for a selected user object.

        iItem - Index of selected item in the listview.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/10/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
LVSelection::Add(
    PDISKQUOTA_USER pUser,
    INT iItem
    )
{
    DBGASSERT((NULL != pUser));

    ListEntry entry;
    entry.pUser = pUser;
    entry.iItem = iItem;

    m_List.Append((LPVOID)&entry);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: LVSelection::Retrieve

    Description: Retrieves a user pointer and listview item index from a
        listview selection object.

    Arguments:
        i - Index of item.  Use the Count() method to determine how many
            items are in the selection object.

        ppUser - Address of an interface pointer variable to receive the
            IDiskQuotaUser interface for the user object at index 'i'.

        piItem - Address of integer variable to receive the Listview item index
            of the object at index 'i'.

    Returns: TRUE  = Returned information is valid.
             FALSE = Couldn't retrieve entry 'i'.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/10/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
LVSelection::Retrieve(
    INT i,
    PDISKQUOTA_USER *ppUser,
    INT *piItem
    )
{
    ListEntry entry;
    if (m_List.Retrieve((LPVOID)&entry, i))
    {
        if (NULL != ppUser)
            *ppUser = entry.pUser;
        if (NULL != piItem)
            *piItem = entry.iItem;
        return TRUE;
    }
    return FALSE;
}




///////////////////////////////////////////////////////////////////////////////
/*  Function: ColumnMap::ColumnMap
    Function: ColumnMap::~ColumnMap

    Description: Constructor and Destructor.
        Creates/Destroys a column map.  The column map is used
        to map column ID's (known to the Details View) to listview subitem
        indices.  It is needed to support the addition and deletion of the
        folder name column.

    Arguments:
        cMapSize - Number of entries in the map.  Should be the max number
            of columns possible in the listview.

    Returns: Nothing.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/09/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
ColumnMap::ColumnMap(
    UINT cMapSize
    ) : m_pMap(NULL),
        m_cMapSize(cMapSize)
{
    //
    // Can throw OutOfMemory.
    //
    m_pMap     = new INT[m_cMapSize];
    FillMemory(m_pMap, m_cMapSize * sizeof(m_pMap[0]), (BYTE)-1);
}

ColumnMap::~ColumnMap(
    VOID
    )
{
    if (NULL != m_pMap)
        delete[] m_pMap;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ColumnMap::SubItemToId

    Description: Returns a column ID given a listview subitem index.

    Arguments:
        iSubItem - 0-based subitem index of the item to be mapped.

    Returns: Column ID corresponding to subitem.  -1 if the subitem is invalid.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/09/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT
ColumnMap::SubItemToId(
    INT iSubItem
    ) const
{
    DBGASSERT((iSubItem >= 0 && iSubItem < (INT)m_cMapSize));
    return *(m_pMap + iSubItem);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: ColumnMap::IdToSubItem

    Description: Returns a listview subitem index given a column ID.

    Arguments:
        iColId - ID of column. i.e. idCol_Name, idCol_Folder etc.

    Returns: Listview subitem index.  -1 if the column is not currently
        visible.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/09/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT
ColumnMap::IdToSubItem(
    INT iColId
    ) const
{
    for (INT i = 0; i < (INT)m_cMapSize; i++)
    {
        if (SubItemToId(i) == iColId)
            return i;
    }
    return -1;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ColumnMap::RemoveId

    Description: Removes a mapping for a given listview subitem index.

    Arguments:
        iSubItem - 0-based subitem index of the item to be removed.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/09/96    Initial creation.                                    BrianAu
    11/30/96    Fixed off-by-one error.                              BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
ColumnMap::RemoveId(
    INT iSubItem
    )
{
    DBGASSERT((iSubItem >= 0 && iSubItem < (INT)m_cMapSize));
    for (INT i = iSubItem; i < (INT)m_cMapSize - 1; i++)
        *(m_pMap + i) = *(m_pMap + i + 1);
    *(m_pMap + m_cMapSize - 1) = -1;
}

///////////////////////////////////////////////////////////////////////////////
/*  Function: ColumnMap::InsertId

    Description: Adds a mapping for a given listview subitem index.
        The mapping is added at the iSubItem location in the map.  All subsequent
        item mappings are shifted down one place.  This is analogous to
        inserting a column into the listview.

    Arguments:
        iSubItem - 0-based subitem index of the item to be removed.

        iColId - ID of column. i.e. idCol_Name, idCol_Folder etc.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/09/96    Initial creation.                                    BrianAu
    11/30/96    Fixed off-by-one error.                              BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
ColumnMap::InsertId(
    INT iSubItem,
    INT iColId
    )
{
    DBGASSERT((iSubItem >= 0 && iSubItem < (INT)m_cMapSize));
    for (INT i = m_cMapSize-1; i > iSubItem; i--)
        *(m_pMap + i) = *(m_pMap + i - 1);
    *(m_pMap + iSubItem) = iColId;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::Finder::Finder

    Description: Constructs a user finder object.
        The user finder coordinates the activities of finding an item in
        the details listview.

    Arguments:
        DetailsView - Reference to the details view object.

        cMaxMru - Maximum entries allowed in the most-recently-used list.
            This list is maintained in the dropdown combo box in the
            view's toolbar.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
DetailsView::Finder::Finder(
    DetailsView& DetailsView,
    INT cMaxMru
    ) : m_DetailsView(DetailsView),
        m_hwndToolbarCombo(NULL),
        m_cMaxComboEntries(cMaxMru),
        m_pfnOldToolbarComboWndProc(NULL)
{
    //
    // Nothing more to do.
    //
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::Finder::ConnectToolbarCombo

    Description: Connects the finder object to the combo box in the view's
        toolbar.  This is necessary because the finder object coordinates
        the contents of the toolbar combo box with the contents of the
        combo box in the "Find User" dialog.  When you enter a name in
        one of the boxes, it is automatically added to the other so they
        appear to be in sync.

        Also subclasses the edit control within the combo box.  This is
        required so that we can intercept VK_RETURN and find the record
        when the user presses [Return].

        Also adds the toolbar combo box as a "tool" to the toolbar.  This
        is so we can get a tooltip for the combo.

    Arguments:
        hwndToolbarCombo - Hwnd of combo box in view's toolbar.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
DetailsView::Finder::ConnectToolbarCombo(
    HWND hwndToolbarCombo
    )
{
    m_hwndToolbarCombo = hwndToolbarCombo;

    //
    // Add the combo box to the toolbar's list of "tools".
    // This will allow us to get a tooltip for the combo box.
    // This code assumes that the combo is a child of the toolbar.
    //
    HWND hwndToolbar = GetParent(hwndToolbarCombo);
    HWND hwndMain    = GetParent(hwndToolbar);
    HWND hwndTooltip = (HWND)SendMessage(hwndToolbar,
                                         TB_GETTOOLTIPS,
                                         0, 0);
    if (NULL != hwndTooltip)
    {
        TOOLINFO ti;

        ti.cbSize   = sizeof(ti);
        ti.uFlags   = TTF_IDISHWND | TTF_CENTERTIP | TTF_SUBCLASS;
        ti.lpszText = (LPTSTR)IDS_TOOLBAR_COMBO;
        ti.hwnd     = hwndMain;
        ti.uId      = (UINT_PTR)hwndToolbarCombo;
        ti.hinst    = g_hInstDll;

        SendMessage(hwndTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
    }

    //
    // Subclass the combo box so we can intercept VK_ENTER.
    // This is done so we can respond to VK_ENTER.  Normally combo boxes
    // don't respond to this keystroke.
    //
    HWND hwndComboEdit = NULL;

    //
    // The combo box has two children... an edit control and a listbox control.
    // Find the edit control.
    //
    for (HWND hwndChild =  GetTopWindow(m_hwndToolbarCombo);
         hwndChild      != NULL;
         hwndChild      =  GetNextWindow(hwndChild, GW_HWNDNEXT))
    {
        TCHAR szClassName[20] = { TEXT('\0') };
        GetClassName(hwndChild, szClassName, ARRAYSIZE(szClassName));

        if (0 == lstrcmpi(szClassName, TEXT("Edit")))
        {
            hwndComboEdit = hwndChild;
            break;
        }
    }

    if (NULL != hwndComboEdit)
    {
        //
        // Store the address of the Finder object in the combo box's
        // userdata.  This is so the subclass WndProc (a static function)
        // can access the finder object.
        //
        SetWindowLongPtr(hwndComboEdit, GWLP_USERDATA, (INT_PTR)this);

        //
        // Subclass the combo box's edit control.
        //
        m_pfnOldToolbarComboWndProc = (WNDPROC)GetWindowLongPtr(hwndComboEdit,
                                                                GWLP_WNDPROC);
        SetWindowLongPtr(hwndComboEdit,
                        GWLP_WNDPROC,
                        (INT_PTR)DetailsView::Finder::ToolbarComboSubClassWndProc);
    }
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::Finder::InvokeFindDialog

    Description: Display the "Find User" dialog.

    Arguments:
        hwndParent - Parent for the dialog.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
DetailsView::Finder::InvokeFindDialog(
    HWND hwndParent
    )
{
    DialogBoxParam(g_hInstDll,
                   MAKEINTRESOURCE(IDD_FINDUSER),
                   hwndParent,
                   (DLGPROC)DetailsView::Finder::DlgProc,
                   (LPARAM)this);
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::Finder::DlgProc

    Description: DlgProc for the "Find User" dialog.

    Arguments:
        Standard DlgProc arguments.

    Returns:
        Standard DlgProc return values.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK
DetailsView::Finder::DlgProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    //
    // Get the finder object's "this" ptr from the window's userdata.
    //
    Finder *pThis = (Finder *)GetWindowLongPtr(hwnd, DWLP_USER);
    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            //
            // Save the "this" ptr in the window's userdata.
            //
            pThis = (Finder *)lParam;
            SetWindowLongPtr(hwnd, DWLP_USER, (INT_PTR)pThis);

            //
            // Set the height of the combo in the dialog.
            // Not sure why, but DevStudio's dialog editor won't let me
            // do this.   Use the same height value we use for the combo
            // in the toolbar.  It's the same contents so the height
            // should be the same.
            //
            HWND hwndCombo = GetDlgItem(hwnd, IDC_CMB_FINDUSER);
            RECT rcCombo;

            GetClientRect(hwndCombo, &rcCombo);
            SetWindowPos(hwndCombo,
                         NULL,
                         0, 0,
                         rcCombo.right - rcCombo.left,
                         CY_TOOLBAR_COMBO,
                         SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE);

            //
            // Fill the dialog's combo with entries from the toolbar
            // combo.  The toolbar's combo box contains the MRU for finding users.
            //
            pThis->FillDialogCombo(pThis->m_hwndToolbarCombo, GetDlgItem(hwnd, IDC_CMB_FINDUSER));

            return 1;
        }

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    //
                    // User pressed OK button or [Enter].
                    //
                    DBGASSERT((NULL != pThis));
                    if (!pThis->UserNameEntered(GetDlgItem(hwnd, IDC_CMB_FINDUSER)))
                    {
                        //
                        // Record not found so don't close dialog.
                        // UserNameEntered() will display UI to tell the user
                        // that the name wasn't found.  Leave the dialog open
                        // so user can retry with a new name.
                        //
                        break;
                    }

                    //
                    // Fall through...
                    //
                case IDCANCEL:
                    //
                    // User pressed Cancel button or [ESC].
                    //
                    EndDialog(hwnd, 0);
                    break;

                default:
                    break;
            }
            break;
    };
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::Finder::FillDialogCombo

    Description: Fill the combo box in the dialog with the contents
        from a second combo box.

    Arguments:
        hwndComboSrc - Hwnd of source combo containing text strings.

        hwndComboDest - Hwnd of combo where strings will be copied to.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
DetailsView::Finder::FillDialogCombo(
    HWND hwndComboSrc,
    HWND hwndComboDest
    )
{
    //
    // Clear out the destination combo.
    //
    SendMessage(hwndComboDest, CB_RESETCONTENT, 0, 0);

    //
    // Copy all contents of the source combo to the destination combo.
    //
    INT cItems = (INT)SendMessage(hwndComboSrc, CB_GETCOUNT, 0, 0);
    if (CB_ERR != cItems)
    {
        for (INT i = 0; i < cItems; i++)
        {
            LPTSTR pszName = NULL;
            INT cchName = (INT)SendMessage(hwndComboSrc, CB_GETLBTEXTLEN, i, 0);
            pszName = new TCHAR[cchName + 1];

            if (NULL != pszName)
            {
                //
                // Remove item from the source combo at index [i] and append
                // it to the destination combo.
                //
                SendMessage(hwndComboSrc,  CB_GETLBTEXT, i, (LPARAM)pszName);
                SendMessage(hwndComboDest, CB_ADDSTRING, 0, (LPARAM)pszName);
                delete[] pszName;
            }
        }
    }
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::Finder::ToolbarComboSubclassWndProc

    Description: Subclass window procedure for the "Edit" control that is part
        of the combo contained in the view's toolbar.  We subclass this control
        so that we can intercept VK_RETURN and handle it.  The standard combo
        box just beeps when you press [Enter] in it's edit control.

    Arguments:
        Standard WndProc arguments.

    Returns:
        Standard WndProc return values.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK
DetailsView::Finder::ToolbarComboSubClassWndProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    //
    // Get finder object's "this" ptr from window's userdata.
    //
    Finder *pThis = (Finder *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch(uMsg)
    {
        case WM_CHAR:
            switch(wParam)
            {
                case VK_RETURN:
                {
                    //
                    // Tell the finder that a user name was entered in the
                    // combo box.  Pass the hwnd of the combo from which the
                    // name was entered.  Since this message is for the
                    // subclassed edit control (child of the combo), the
                    // parent is the combo box itself.
                    //
                    DBGASSERT((NULL != pThis));
                    HWND hwndCombo = GetParent(hwnd);
                    if (pThis->UserNameEntered(hwndCombo))
                    {
                        //
                        // Record found in view.
                        // Set focus to the main view.
                        // If not found, focus should just stay with the combo
                        // so user can enter another name.
                        //
                        HWND hwndToolbar = GetParent(hwndCombo);
                        SetFocus(GetParent(hwndToolbar));
                    }
                    else
                    {
                        //
                        // Not found in listview.  Focus remains in the combo box
                        // so user can try again with a new name.
                        //
                        SetFocus(hwndCombo);
                    }

                    //
                    // Swallow up the VK_RETURN.
                    // Otherwise, the combo box control beeps.
                    //
                    return 0;
                }

                case VK_ESCAPE:
                {
                    //
                    // Set focus to the main window which will set focus to the
                    // listview.  This gives the keyboard-only user a way to
                    // get back out of the combo box.
                    //
                    HWND hwndCombo = GetParent(hwnd);
                    HWND hwndToolbar = GetParent(hwndCombo);
                    SetFocus(GetParent(hwndToolbar));
                    //
                    // Swallow VK_ESCAPE so combo box doesn't beep.
                    //
                    return 0;
                }
            }
            break;

        default:
            break;
    }
    return CallWindowProc(pThis->m_pfnOldToolbarComboWndProc,
                          hwnd, uMsg, wParam, lParam);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::Finder::AddNameToCombo

    Description: Add a name string to one of the Find User combo boxes.
        If the item already exists in the list, it is moved to the top of the
        list.  If the item is not in the list, it is added at the top of the list.
        If the addition of the new item causes the list's entry count to exceed
        a specified maximum value, the last item in the list is removed.

    Arguments:
        hwndCombo - Hwnd for the combo box to which the name is added.

        pszName - Address of name string to add.

        cMaxEntries - Maximum number of entries allowed in combo box.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
DetailsView::Finder::AddNameToCombo(
    HWND hwndCombo,
    LPCTSTR pszName,
    INT cMaxEntries
    )
{
    if (NULL != pszName && TEXT('\0') != *pszName)
    {
        //
        // See if the item already exists in the list.
        //
        INT iItemToDelete = (INT)SendMessage(hwndCombo,
                                             CB_FINDSTRING,
                                             (WPARAM)-1,
                                             (LPARAM)pszName);

        if (CB_ERR == iItemToDelete)
        {
            //
            // Item is not already in the list.  Need to add it.
            // If the list is full, we'll have to drop one off of the end.
            //
            INT cItems = (INT)SendMessage(hwndCombo, CB_GETCOUNT, 0, 0);

            if (CB_ERR != cItems && 0 < cItems && cItems >= cMaxEntries)
            {
                iItemToDelete = cItems - 1;
            }
        }
        if (-1 != iItemToDelete)
        {
            //
            // Need to delete an existing item for one of these reasons:
            //
            //  1. Promoting an existing item to the head of the list.
            //     Delete it from it's previous location.
            //  2. Dropping last item from list.
            //
            SendMessage(hwndCombo, CB_DELETESTRING, iItemToDelete, 0);
        }
        //
        // Add the new item at the head of the list.
        //
        SendMessage(hwndCombo, CB_INSERTSTRING, 0, (LPARAM)pszName);
    }
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::Finder::UserNameEntered

    Description: A name has been entered from one of the combo boxes.
        1. Retrieve the name from the combo.
        2. See if it's in the listview and if it is, the listview ensures the
           item is visible and selects it.
        3. Update the toobar combo's list with the new item.  This is our MRU
           list.

        Add a name string to one of the Find User combo boxes.
        If the item already exists in the list, it is moved to the top of the
        list.  If the item is not in the list, it is added at the top of the list.
        If the addition of the new item causes the list's entry count to exceed
        a specified maximum value, the last item in the list is removed.

    Arguments:
        hwndCombo - Hwnd for the combo box to which the name is added.

        pszName - Address of name string to add.

        cMaxEntries - Maximum number of entries allowed in combo box.

    Returns:
        TRUE  = User was found in listview.
        FALSE = User was not found in listview.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
DetailsView::Finder::UserNameEntered(
    HWND hwndCombo
    )
{
    TCHAR szName[MAX_PATH]    = { TEXT('\0') };
    BOOL bUserFoundInListView = FALSE;

    //
    // Get the name from the combo edit control.
    //
    if (0 < SendMessage(hwndCombo, WM_GETTEXT, (WPARAM)ARRAYSIZE(szName), (LPARAM)szName))
    {
        //
        // Tell the details view object to highlight this name.
        //
        bUserFoundInListView = m_DetailsView.GotoUserName(szName);

        if (bUserFoundInListView)
        {
            //
            // Add the name to the toolbar combo's listbox.  This becomes
            // our MRU list.  Also make sure the visible name in the combo's
            // edit control is the one last entered.  May have been entered
            // through the "Find User" dialog.
            //
            AddNameToCombo(m_hwndToolbarCombo, szName, m_cMaxComboEntries);
            SendMessage(m_hwndToolbarCombo, WM_SETTEXT, 0, (LPARAM)szName);
        }
        else
        {
            //
            // Display a message box to the user stating that the user couldn't
            // be found in the listview.
            //
            CString strMsg(g_hInstDll, IDS_USER_NOT_FOUND_IN_LISTVIEW, szName);

            DiskQuotaMsgBox(hwndCombo,
                            (LPCTSTR)strMsg,
                            IDS_TITLE_DISK_QUOTA,
                            MB_ICONEXCLAMATION);
        }
    }
    return bUserFoundInListView;
}



DetailsView::Importer::Importer(
    DetailsView& DV
    ) : m_DV(DV),
        m_bUserCancelled(FALSE),
        m_bPromptOnReplace(TRUE),
        m_dlgProgress(IDD_PROGRESS,
                      IDC_PROGRESS_BAR,
                      IDC_TXT_PROGRESS_DESCRIPTION,
                      IDC_TXT_PROGRESS_FILENAME),
        m_hwndParent(m_DV.m_hwndMain),
        m_cImported(0)
{
    if (m_dlgProgress.Create(g_hInstDll, m_hwndParent))
    {
        EnableWindow(m_hwndParent, FALSE);
        m_dlgProgress.SetDescription(MAKEINTRESOURCE(IDS_PROGRESS_IMPORTING));
        m_dlgProgress.Show();
    }
    //
    // Clear any previous undo actions from the view's undo list.
    //
    m_DV.m_pUndoList->Clear();
}

DetailsView::Importer::~Importer(
    VOID
    )
{
    Destroy();
}


VOID
DetailsView::Importer::Destroy(
    VOID
    )
{
    if (NULL != m_hwndParent && !IsWindowEnabled(m_hwndParent))
    {
        EnableWindow(m_hwndParent, TRUE);
    }

    m_dlgProgress.Destroy();

    //
    // Update the view's "Undo" menu and toolbar button based on the current
    // contents of the undo list.
    //
    m_DV.EnableMenuItem_Undo(0 != m_DV.m_pUndoList->Count());
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::Importer::Import [IDataObject *]

    Description: Imports user quota records given an IDataObject pointer.
        Called from DetailsView::Drop().

    Arguments:
        pIDataObject - Pointer to IDataObject interface of object containing
            import data.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::Importer::Import(
    IDataObject *pIDataObject
    )
{
    HRESULT hResult = NO_ERROR;
    FORMATETC fmt;
    CStgMedium medium;

    //
    // Array to specify the clipboard formats and media types that
    // we can import from.  Ordered by preference.
    //
    struct
    {
        CLIPFORMAT fmt;
        DWORD tymed;
    } rgFmtMedia[] = {{ DataObject::m_CF_NtDiskQuotaExport, TYMED_ISTREAM},
                      { DataObject::m_CF_NtDiskQuotaExport, TYMED_HGLOBAL},
                      { CF_HDROP,                           TYMED_ISTREAM},
                      { CF_HDROP,                           TYMED_HGLOBAL}};

    DBGASSERT((NULL != pIDataObject));

    hResult = E_FAIL;
    for (INT i = 0; i < ARRAYSIZE(rgFmtMedia); i++)
    {
        //
        // See which of our supported formats/media types the drop
        // source supports.
        //
        DataObject::SetFormatEtc(fmt, rgFmtMedia[i].fmt, rgFmtMedia[i].tymed);

        //
        // NOTE:  I wanted to call QueryGetData to verify a source's support
        //        for a format.  However, it didn't work properly when
        //        pasting an HDROP from the shell.  Calling GetData()
        //        directly results in the proper behavior.
        //
        hResult = pIDataObject->GetData(&fmt, &medium);
        if (SUCCEEDED(hResult))
        {
            break;
        }
    }

    if (SUCCEEDED(hResult))
    {
        //
        // Successfully have dropped data from the source.
        // Import users from it.
        //
        hResult = Import(fmt, medium);
    }
    else
    {
        DBGERROR((TEXT("PasteFromData: Error 0x%08X, Drop source doesn't support our format/media"), hResult));
    }
    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::Importer::Import [FORMATETC&, STGMEDIUM&]

    Description: Imports one or more users from a storage medium.

    Arguments:
        fmt - Reference to the FORMATETC structure describing the data format.

        medium - Reference to the STGMEDIUM structure describing the medium.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::Importer::Import(
    const FORMATETC& fmt,
    const STGMEDIUM& medium
    )
{
    HRESULT hResult   = E_FAIL;
    IStream *pIStream = NULL;

    if (TYMED_HGLOBAL == medium.tymed)
    {
        //
        // Medium type is an HGLOBAL but our import functions need
        // a stream.
        //
        hResult = CreateStreamOnHGlobal(medium.hGlobal, FALSE, &pIStream);
    }
    else if (TYMED_ISTREAM == medium.tymed)
    {
        pIStream = medium.pstm;
        hResult  = NO_ERROR;
    }
    //
    // OK.  The source can render data in one of our acceptable
    // formats and medium types.  Go ahead and have them render
    // it onto our stream.
    //
    if (NULL != pIStream)
    {
        if (DetailsView::DataObject::m_CF_NtDiskQuotaExport == fmt.cfFormat)
        {
            //
            // Stream contains quota record information directly.
            // Import the records.
            //
            Source src(pIStream);
            Import(src);
        }
        else if (CF_HDROP == fmt.cfFormat)
        {
            //
            // Stream contains names of files that potentially
            // contain quota record information.
            //
            HGLOBAL hDrop;
            hResult = GetHGlobalFromStream(pIStream, &hDrop);
            if (SUCCEEDED(hResult))
            {
                hResult = Import((HDROP)hDrop);
            }
        }
    }
    else
    {
        DBGERROR((TEXT("PasteFromData: GetData failed with error 0x%08X"), hResult));
    }
    return hResult;
}

///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::Importer::Import [LPCTSTR]

    Description: Imports settings for one or more users from a doc file on
        disk.  The doc file contains the import data directly in the stream.
        After opening and validating the storage and stream, it passes the
        stream to ImportUsersFromStream.

    Arguments:
        pszFilePath - Path to doc file containing import information stream.

        bUserCancelled - Reference to variable that is returned status
            indicating if the user cancelled the import operation.

    Returns:
        NO_ERROR = Success.
        S_FALSE  = Not a doc file.
        STG_E_FILENOTFOUND
        STG_E_OUTOFMEMORY
        STG_E_ACCESSDENIED
        STG_E_INVALIDNAME
        STG_E_TOOMANYOPENFILES

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::Importer::Import(
    LPCTSTR pszFilePath
    )
{
    HRESULT hResult     = NO_ERROR;
    BOOL bStreamFailure = FALSE; // FALSE = Storage failure.

    //
    // Display the filename in the progress dialog.
    //
    m_dlgProgress.SetFileName(pszFilePath);

    //
    // Validate and open the file.
    //
    if (S_OK != StgIsStorageFile(pszFilePath))
    {
        //
        // Not a doc file.  Assume it was created using drag/drop.
        // Map the file into memory and import from that.
        // Contents will be validated during the import process.
        //
        MappedFile file;
        hResult = file.Open(pszFilePath);
        if (SUCCEEDED(hResult))
        {
            //
            // This typecast from __int64 to ULONG is OK. Truncation
            // will not be a problem.  There will be no quota import
            // storages larger than 4GB.
            //
            Source src(file.Base(), (ULONG)file.Size());
            hResult = Import(src);
        }
    }
    else
    {
        IStorage *pStg = NULL;
        //
        // It's a doc file.  Assume it's one created using OnCmdExport().
        // Contents will be validated during the import process.
        //
        hResult = StgOpenStorage(pszFilePath,
                                 NULL,
                                 STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE,
                                 NULL, 0,
                                 &pStg);

        if (SUCCEEDED(hResult))
        {
            //
            // Open the import stream.
            //
            IStream *pIStream;
            hResult = pStg->OpenStream(DetailsView::DataObject::SZ_EXPORT_STREAM_NAME,
                                       NULL,
                                       STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE,
                                       0,
                                       &pIStream);
            if (SUCCEEDED(hResult))
            {

                //
                // Import information contained in the stream.
                //
                Source src(pIStream);
                hResult = Import(src);
                pIStream->Release();
            }
            else
            {
                DBGERROR((TEXT("Import: Error 0x%08X opening stream \"%s\""), hResult, DataObject::SZ_EXPORT_STREAM_NAME));
                //
                // Reporting logic below needs to know if it was a stream or storage
                // failure.
                //
                bStreamFailure = TRUE;
            }
            pStg->Release();
        }
        else
        {
            DBGERROR((TEXT("Import: Error 0x%08X opening storage \"%s\""), hResult, pszFilePath));
        }
    }

    if (FAILED(hResult))
    {
        UINT iMsg = IDS_IMPORT_STREAM_READ_ERROR; // Generic message.

        switch(hResult)
        {
            case STG_E_FILENOTFOUND:
                //
                // Both OpenStream and StgOpenStorage can return
                // STG_E_FILENOTFOUND.  However, they have two completely
                // different meanings from the user's perspective.
                //
                iMsg = bStreamFailure ? IDS_IMPORT_STREAM_INVALID_STREAM :
                                        IDS_IMPORT_STREAM_FILENOTFOUND;
                break;

            case STG_E_ACCESSDENIED:
                iMsg = IDS_IMPORT_STREAM_NOACCESS;
                break;

            case E_OUTOFMEMORY:
            case STG_E_INSUFFICIENTMEMORY:
                iMsg = IDS_IMPORT_STREAM_OUTOFMEMORY;
                break;

            case STG_E_INVALIDNAME:
                iMsg = IDS_IMPORT_STREAM_INVALIDNAME;
                break;

            case STG_E_TOOMANYOPENFILES:
                iMsg = IDS_IMPORT_STREAM_TOOMANYFILES;
                break;

            default:
                break;
        }
        DiskQuotaMsgBox(GetTopmostWindow(),
                        iMsg,
                        IDS_TITLE_DISK_QUOTA,
                        MB_ICONERROR | MB_OK);

    }
    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::Importer::Import [HDROP]

    Description: Imports settings from one or more doc files specified
        in a DROPFILES buffer.  This is used when someone drops an export
        file onto the listview.  The doc file names are extracted from
        the HDROP buffer then handed off to ImportUsersFromFile.

    Arguments:
        pIStream - Pointer to IStream containing DROPFILES info.

        bUserCancelled - Reference to variable that is returned status
            indicating if the user cancelled the import operation.


    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::Importer::Import(
    HDROP hDrop
    )
{
    HRESULT hResult = NO_ERROR;
    TCHAR szFile[MAX_PATH];

    DBGASSERT((NULL != hDrop));

    //
    // Get the count of files in the HDROP buffer.
    //
    UINT cFiles = DragQueryFile((HDROP)hDrop, (UINT)-1, NULL, 0);
    if ((UINT)-1 != cFiles)
    {
        //
        // Import users from each file in the HDROP buffer.
        // Bail out if user cancels operation.
        //
        for (INT i = 0; i < (INT)cFiles && !m_bUserCancelled; i++)
        {
            DragQueryFile(hDrop, i, szFile, ARRAYSIZE(szFile));
            hResult = Import(szFile);
        }
    }
    else
    {
        DBGERROR((TEXT("DragQueryFile returned -1")));
    }
    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::Importer::Import [Source&]

    Description: Imports settings for one or more users from a Source object.
        All import functions eventually get their information into a Source
        object format and call this function.  It then separates out the
        individual user information and calls ImportOneUser() to do the
        actual import.

    Arguments:
        source - Reference to Source containing user import info.

    Returns: Number of users imported.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::Importer::Import(
    Source& source
    )
{
    ULONG cbRead;
    HRESULT hResult = E_FAIL;

    try
    {
        //
        // Read and validate the stream signature.
        // This signature consists of a GUID so that we can validate any
        // stream used for import of quota information.
        //
        GUID guidStreamSignature;
        if (S_OK != source.Read(&guidStreamSignature, sizeof(guidStreamSignature), &cbRead))
            throw CFileException(CFileException::read, TEXT(""), 0);

        if (guidStreamSignature == GUID_NtDiskQuotaStream)
        {
            //
            // Read and validate the stream version.
            // Currently there is only 1 version of stream generated so validation
            // is simple.  If we ever rev the stream format and bump the version
            // to 2, we should still be able to handle version 1 streams.  The
            // only reason to display an error is if we encounter a totally bogus
            // stream version number.
            //
            DWORD nVersion;
            if (S_OK != source.Read(&nVersion, sizeof(nVersion), &cbRead))
                throw CFileException(CFileException::read, TEXT(""), 0);

            if (1 == nVersion)
            {
                INT cRecords;
                //
                // Read the count of records in the stream.
                //
                if (S_OK != source.Read(&cRecords, sizeof(cRecords), &cbRead))
                    throw CFileException(CFileException::read, TEXT(""), 0);

                //
                // Set up the progress bar to represent this stream.
                //
                m_dlgProgress.ProgressBarInit(0, cRecords, 1);

                for (INT i = 0; !m_bUserCancelled && i < cRecords; i++)
                {
                    //
                    // Read each record from the stream.
                    // A record consists of a SID-Length value followed by a SID
                    // then followed by the user's quota amount used, threshold
                    // and limit values.  Abort loop if user cancels the
                    // operation.
                    //
                    DWORD cbSid;
                    LPBYTE pbSid;
                    if (S_OK != source.Read(&cbSid, sizeof(cbSid), &cbRead))
                        throw CFileException(CFileException::read, TEXT(""), 0);

                    pbSid = new BYTE[cbSid];
                    try
                    {
                        if (NULL != pbSid)
                        {
                            PDISKQUOTA_USER pIUser = NULL;
                            LONGLONG llQuotaThreshold;
                            LONGLONG llQuotaLimit;
                            //
                            // Read in the user's SID.
                            //
                            if (S_OK != source.Read(pbSid, cbSid, &cbRead))
                                throw CFileException(CFileException::read, TEXT(""), 0);

                            //
                            // Read in the user's quota amount used.
                            // This isn't used in the import process but it's in
                            // the stream.  Therefore we just dump it into the
                            // threshold buffer.  It will be overwritten.
                            //
                            if (S_OK != source.Read(&llQuotaThreshold, sizeof(llQuotaThreshold), &cbRead))
                                throw CFileException(CFileException::read, TEXT(""), 0);

                            //
                            // Read in the user's quota threshold.
                            //
                            if (S_OK != source.Read(&llQuotaThreshold, sizeof(llQuotaThreshold), &cbRead))
                                throw CFileException(CFileException::read, TEXT(""), 0);

                            //
                            // Read in the user's quota limit.
                            //
                            if (S_OK != source.Read(&llQuotaLimit, sizeof(llQuotaLimit), &cbRead))
                                throw CFileException(CFileException::read, TEXT(""), 0);

                            //
                            // We have one record of data for a user.
                            // Now import it.
                            //
                            hResult = Import(pbSid, llQuotaThreshold, llQuotaLimit);
                            delete[] pbSid;
                        }
                    }
                    catch(CFileException& fe)
                    {
                        DBGERROR((TEXT("Import: File exception caught while reading import data.")));
                        delete[] pbSid;
                        throw;
                    }
                    catch(CAllocException& ae)
                    {
                        DBGERROR((TEXT("Import: Alloc exception caught while reading import data.")));
                        delete[] pbSid;
                        throw;
                    }
                }
            }
            else
            {
                //
                // Invalid stream version.
                // Our code should always be able to handle any version
                // we produce.  This code branch should only handle BOGUS
                // version numbers.  In other words, a message like "Can't
                // understand this version" is not acceptable.
                //
                DBGERROR((TEXT("Import: Invalid stream version (%d)."), nVersion));
                DiskQuotaMsgBox(GetTopmostWindow(),
                                IDS_IMPORT_STREAM_INVALID_STREAM,
                                IDS_TITLE_DISK_QUOTA,
                                MB_ICONERROR | MB_OK);
            }
        }
        else
        {
            //
            // Invalid stream signature.
            //
            DBGERROR((TEXT("Import: Invalid stream signature.")));
            DiskQuotaMsgBox(GetTopmostWindow(),
                            IDS_IMPORT_STREAM_INVALID_STREAM,
                            IDS_TITLE_DISK_QUOTA,
                            MB_ICONERROR | MB_OK);
        }
    }
    catch(CFileException& fe)
    {
        DBGERROR((TEXT("Import: File exception caught while reading import data.")));

        DiskQuotaMsgBox(GetTopmostWindow(),
                        IDS_IMPORT_STREAM_READ_ERROR,
                        IDS_TITLE_DISK_QUOTA,
                        MB_ICONERROR | MB_OK);

        hResult = HRESULT_FROM_WIN32(ERROR_READ_FAULT);
    }
    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::Importer::Import [LPBYTE, LONGLONG, LONGLONG]

    Description: Imports a single user into the system given the user's SID
        and quota settings.  This is the single function where all import
        mechanisms end up.  It does the actual importing of the user.

    Arguments:
        pbSid - Address of buffer containing user's SID.

        llQuotaThreshold - User's quota warning threshold setting.

        llQuotaLimit - User's quota limit setting.

    Returns:
        -1 = User pressed "Cancel" in either the "Replace User" dialog or
            in the progress dialog.

         0 = Failed to import user.

         1 = User imported.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DetailsView::Importer::Import(
    LPBYTE pbSid,
    LONGLONG llQuotaThreshold,
    LONGLONG llQuotaLimit
    )
{
    INT iResult     = 0;
    HRESULT hResult = NO_ERROR;
    PDISKQUOTA_USER pIUser = NULL;
    static BOOL bReplaceExistingUser = FALSE;

    DBGASSERT((NULL != pbSid));

    if (m_bPromptOnReplace)
    {
        //
        // We'll be prompting the user if a record needs replacement.
        // They'll make a choice through the UI.
        // Assume for now that we won't be replacing the record.
        //
        bReplaceExistingUser = FALSE;
    }

    //
    // Add user to volume's quota file.
    //
    hResult = m_DV.m_pQuotaControl->AddUserSid(pbSid,
                                               DISKQUOTA_USERNAME_RESOLVE_SYNC,
                                               &pIUser);
    if (SUCCEEDED(hResult))
    {
        //
        // Either the user was added or already exists.
        //
        BOOL bAddNewUser = (S_FALSE != hResult);

        if (!bAddNewUser)
        {
            //
            // User already exists in the quota file.  Find it's entry
            // in the listview.
            //
            DBGASSERT((NULL != pIUser));
            pIUser->Release();
            INT iItem = m_DV.FindUserBySid(pbSid, &pIUser);

            if (m_bPromptOnReplace)
            {
                TCHAR szLogonName[MAX_USERNAME]   = { TEXT('\0') };
                TCHAR szDisplayName[MAX_USERNAME] = { TEXT('\0') };

                if (-1 != iItem)
                {
                    //
                    // Listview item found.
                    // Get the account's name string so we can ask the user
                    // if they want to replace it's quota settings.
                    //
                    DBGASSERT((NULL != pIUser));
                    pIUser->GetName(NULL, 0,
                                    szLogonName,   ARRAYSIZE(szLogonName),
                                    szDisplayName, ARRAYSIZE(szDisplayName));

                }

                CString strTitle(g_hInstDll, IDS_TITLE_DISK_QUOTA);
                CString strMsg(g_hInstDll,
                               IDS_IMPORT_REPLACE_RECORD,
                               szDisplayName,
                               szLogonName);

                //
                // Ask the user if they want to replace the record's
                // quota settings.
                //
                YesNoToAllDialog ynToAllDlg(IDD_YNTOALL);
                INT_PTR iResponse = ynToAllDlg.CreateAndRun(g_hInstDll,
                                                            GetTopmostWindow(),
                                                            strTitle,
                                                            strMsg);
                //
                // If the "Apply to All" checkbox was selected, we set this flag
                // so that the dialog isn't displayed again until the caller resets
                // m_bPromptOnReplace to TRUE.
                //
                m_bPromptOnReplace = !ynToAllDlg.ApplyToAll();

                switch(iResponse)
                {
                    case IDYES:
                        bReplaceExistingUser = TRUE;
                        break;

                    case IDCANCEL:
                        m_bUserCancelled = TRUE;
                        break;

                    default:
                        break;
                }
            }
        }
        if (bAddNewUser || bReplaceExistingUser)
        {
            DBGASSERT((NULL != pIUser));

            //
            // Write the new quota values because...
            //
            // 1. Added a new user record and setting initial values or...
            // 2. Replacing settings for an existing user.
            //
            if (NULL != pIUser)
            {
                LONGLONG llQuotaThresholdUndo;
                LONGLONG llQuotaLimitUndo;

                if (!bAddNewUser && bReplaceExistingUser)
                {
                    //
                    // Save the current threshold and limit values for "undo".
                    // Only need information for undo if replacing an existing
                    // user's settings.  For performance, only call when we need
                    // the info.
                    //
                    pIUser->GetQuotaThreshold(&llQuotaThresholdUndo);
                    pIUser->GetQuotaLimit(&llQuotaLimitUndo);
                }

                //
                // Set the new threshold and limit values.
                //
                pIUser->SetQuotaThreshold(llQuotaThreshold, TRUE);
                pIUser->SetQuotaLimit(llQuotaLimit, TRUE);

                if (bAddNewUser)
                {
                    //
                    // Add the user to the listview and create an UNDO object for the operation.
                    //
                    m_DV.AddUser(pIUser);
                    pIUser->AddRef();
                    m_DV.m_pQuotaControl->AddRef();
                    m_DV.m_pUndoList->Add(new UndoAdd(pIUser, m_DV.m_pQuotaControl));
                }
                if (!bAddNewUser && bReplaceExistingUser)
                {
                    //
                    // This will update the record to display any changed quota values.
                    // Create an UNDO object for the operation.
                    //
                    m_DV.OnUserNameChanged(pIUser);
                    pIUser->AddRef();
                    m_DV.m_pUndoList->Add(new UndoModify(pIUser, llQuotaThresholdUndo, llQuotaLimitUndo));
                }
            }
        }
    }

    if (!m_bUserCancelled)
        m_bUserCancelled = m_dlgProgress.UserCancelled();

    if (SUCCEEDED(hResult))
    {
        m_cImported++;
        m_dlgProgress.ProgressBarAdvance();
    }

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DetailsView::Importer::GetTopmostWindow

    Description: Returns the HWND of the topmost window in the importer UI.
        If the UI's progress dialog is visible, the dialog's HWND is returned.
        Otherwise, the value of m_hwndParent is returned.
        The Importer uses this function to identify what window should be parent
        to any error message boxes.

    Arguments: None.

    Returns: HWND to use for parent of any messages boxes created by the
        Importer.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HWND
DetailsView::Importer::GetTopmostWindow(
    VOID
    )
{
   return m_dlgProgress.m_hWnd ? m_dlgProgress.m_hWnd : m_hwndParent;
}



///////////////////////////////////////////////////////////////////////////////
// The following StreamSource functions implement a layer of abstraction
// between the import function and the source of the import data.  This allows
// me to centralize the actual import processing in a single function
// without consideration of input source.
// There are several Import() overloads but they eventually all call down
// to Import(Source&).  These functions are very simple so I'm not going
// go bother documenting each.  I think it's pretty obvious what they do.
// Do note the use of the virtual constructor technique allowing the user
// to deal only with Source objects and not AnySource, StreamSource or
// MemorySource objects.  This may be unfamiliar to some.
//
// [brianau 7/25/97]
//
///////////////////////////////////////////////////////////////////////////////
//
// Source ---------------------------------------------------------------------
//
DetailsView::Importer::Source::Source(
    IStream *pStm
    ) : m_pTheSource(NULL)
{
    //
    // Create a stream source type object.
    //
    m_pTheSource = new StreamSource(pStm);
}


DetailsView::Importer::Source::Source(
    LPBYTE pb,
    ULONG cbMax
    ) : m_pTheSource(NULL)
{
    //
    // Create a memory source type object.
    //
    m_pTheSource = new MemorySource(pb, cbMax);
}

DetailsView::Importer::Source::~Source(
    VOID
    )
{
    //
    // Note:  Destructors must be virtual for this to work.
    //
    delete m_pTheSource;
}


HRESULT
DetailsView::Importer::Source::Read(
    LPVOID pvOut,
    ULONG cb,
    ULONG *pcbRead
    )
{
    HRESULT hr = E_OUTOFMEMORY;
    if (NULL != m_pTheSource)
    {
        //
        // Delegate the read operation to the properly-typed
        // subobject.
        //
        hr = m_pTheSource->Read(pvOut, cb, pcbRead);
    }
    return hr;
}


//
// StreamSource ---------------------------------------------------------------
//
DetailsView::Importer::StreamSource::StreamSource(
    IStream *pStm
    ) : m_pStm(pStm)
{
    //
    // AddRef the stream pointer.
    //
    if (NULL != m_pStm)
        m_pStm->AddRef();
}

DetailsView::Importer::StreamSource::~StreamSource(
    VOID
    )
{
    //
    // Release the stream pointer.
    //
    if (NULL != m_pStm)
        m_pStm->Release();
}

HRESULT
DetailsView::Importer::StreamSource::Read(
    LPVOID pvOut,
    ULONG cb,
    ULONG *pcbRead
    )
{
    HRESULT hr = E_FAIL;
    if (NULL != m_pStm)
    {
        //
        // Read data from the stream.
        //
        hr = m_pStm->Read(pvOut, cb, pcbRead);
    }
    return hr;
}


//
// MemorySource ---------------------------------------------------------------
//
DetailsView::Importer::MemorySource::MemorySource(
    LPBYTE pb,
    ULONG cbMax
    ) : m_pb(pb),
        m_cbMax(cbMax)
{

}


HRESULT
DetailsView::Importer::MemorySource::Read(
    LPVOID pvOut,
    ULONG cb,
    ULONG *pcbRead
    )
{
    HRESULT hr = E_FAIL;

    if (m_cbMax >= cb)
    {
        //
        // Read data from the memory block.
        //
        CopyMemory(pvOut, m_pb, cb);
        m_pb    += cb;
        m_cbMax -= cb;
        if (NULL != pcbRead)
        {
            *pcbRead = cb;
        }
        hr = S_OK;
    }
    return hr;
}


