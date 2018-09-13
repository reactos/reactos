/*++

    Implements IShellView.

--*/

#include <windows.h>
#include <windowsx.h>
#include <objbase.h>
#include <shlobj.h>

#include <docobj.h>
#include <shellapi.h>

#include "pstore.h"

#include "shfolder.h"

#include "shview.h"
#include "guid.h"
#include "resource.h"
#include "tools.h"

#include "utility.h"
#include "enumid.h"


#include "listu.h"


extern HINSTANCE  g_hInst;
extern LONG       g_DllRefCount;



BOOL
AddListItem(
    HWND hWnd,
    DWORD dwType,
    PST_KEY KeyType,
    LPCWSTR szName,
    LPITEMIDLIST pidl
    );



#define TOOLBAR_ID   (L"TBar")
#define INITIAL_COLUMN_POS 100

MYTOOLINFO g_Buttons[] =
   {
   IDM_MESSAGE1, 0, IDS_TB_MESSAGE1, IDS_MI_MESSAGE1, TBSTATE_ENABLED, TBSTYLE_BUTTON,
   IDM_MESSAGE2, 0, IDS_TB_MESSAGE2, IDS_MI_MESSAGE2, TBSTATE_ENABLED, TBSTYLE_BUTTON,
   -1, 0, 0, 0, 0,
   };


CShellView::CShellView(
    CShellFolder *pFolder,
    LPCITEMIDLIST pidl
    )
{

    m_hMenu = NULL;
    ZeroMemory(&m_MenuWidths, sizeof(m_MenuWidths));

    m_nColumn1 = INITIAL_COLUMN_POS;
    m_nColumn2 = INITIAL_COLUMN_POS;

    m_pSFParent = pFolder;
    if(m_pSFParent)
        m_pSFParent->AddRef();

    m_pidl = (LPITEMIDLIST)pidl;

    m_uState = SVUIA_DEACTIVATE;

    m_ObjRefCount = 1;
    InterlockedIncrement(&g_DllRefCount);
}


CShellView::~CShellView()
{
    if(m_pSFParent)
        m_pSFParent->Release();

    InterlockedDecrement(&g_DllRefCount);
}


STDMETHODIMP
CShellView::QueryInterface(
    REFIID riid,
    LPVOID *ppReturn
    )
/*++

    IUnknown::QueryInterface

--*/
{
    *ppReturn = NULL;

    if(IsEqualIID(riid, IID_IUnknown))
        *ppReturn = (IUnknown*)(IShellView*)this;
    else if(IsEqualIID(riid, IID_IOleWindow))
        *ppReturn = (IOleWindow*)this;
    else if(IsEqualIID(riid, IID_IShellView))
        *ppReturn = (CShellView*)this;
    else if(IsEqualIID(riid, IID_IOleCommandTarget))
        *ppReturn = (IOleCommandTarget*)this;

    if(*ppReturn == NULL)
        return E_NOINTERFACE;

    (*(LPUNKNOWN*)ppReturn)->AddRef();
    return S_OK;
}


STDMETHODIMP_(DWORD)
CShellView::AddRef()
/*++

    IUnknown::AddRef

--*/
{
    return InterlockedIncrement(&m_ObjRefCount);
}


STDMETHODIMP_(DWORD)
CShellView::Release()
/*++

    IUnknown::Release

--*/
{
    LONG lDecremented = InterlockedDecrement(&m_ObjRefCount);

    if(lDecremented == 0)
        delete this;

    return lDecremented;
}


STDMETHODIMP
CShellView::GetWindow(
    HWND *phWnd
    )
/*++

    IOleWindow::GetWindow

--*/
{
    *phWnd = m_hWnd;

    return S_OK;
}


STDMETHODIMP
CShellView::ContextSensitiveHelp(
    BOOL fEnterMode
    )
/*++

   Inherited from IOleWindow::ContextSensitiveHelp.

--*/
{
    return E_NOTIMPL;
}


STDMETHODIMP
CShellView::QueryStatus(
    const GUID *pguidCmdGroup,
    ULONG cCmds,
    OLECMD prgCmds[],
    OLECMDTEXT *pCmdText
    )
/*++

    IOleCommandTarget::QueryStatus

--*/
{
    ULONG i;

    //
    // only process the commands for our command group
    //

    if(pguidCmdGroup && (*pguidCmdGroup != CLSID_CmdGrp))
        return OLECMDERR_E_UNKNOWNGROUP;

    //
    // make sure prgCmds is not NULL
    //

    if(prgCmds == NULL)
        return E_POINTER;

    //
    // run through all of the commands and supply the correct information
    //

    for(i = 0; i < cCmds;i++)
    {
        switch(prgCmds[i].cmdID)
        {
            case IDM_MESSAGE1:
                prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                break;

            case IDM_MESSAGE2:
                prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                break;
        }
    }

    return S_OK;
}


STDMETHODIMP
CShellView::Exec(
    const GUID *pguidCmdGroup,
    DWORD nCmdID,
    DWORD nCmdExecOpt,
    VARIANTARG *pvaIn,
    VARIANTARG *pvaOut
    )
/*++

    IOleCommandTarget::Exec

--*/
{
    //
    // only process the commands for our command group
    //

    if(pguidCmdGroup && (*pguidCmdGroup == CLSID_CmdGrp))
    {
        OnCommand(nCmdID, 0, NULL);
        return S_OK;
    }

    return OLECMDERR_E_UNKNOWNGROUP;
}


STDMETHODIMP
CShellView::TranslateAccelerator(
    LPMSG pMsg
    )
/*++

  Same as the IOleInPlaceFrame::TranslateAccelerator, but will be
  never called because we don't support EXEs (i.e., the explorer has
  the message loop). This member function is defined here for possible
  future enhancement.

--*/
{
    return E_NOTIMPL;
}


STDMETHODIMP
CShellView::EnableModeless(
    BOOL fEnable
    )
/*++

   Same as the IOleInPlaceFrame::EnableModeless.

--*/
{
    return E_NOTIMPL;
}


STDMETHODIMP
CShellView::UIActivate(
    UINT uState
    )
/*++

    IShellView::UIActivate

--*/
{
    //
    // don't do anything if the state isn't really changing
    //

    if(m_uState == uState)
        return S_OK;

    //
    // Always do this because we will cause problems if we are going from
    // SVUIA_ACTIVATE_FOCUS to SVUIA_ACTIVATE_NOFOCUS or vice versa.
    //

    m_uState = SVUIA_DEACTIVATE;

    m_pShellBrowser->SetMenuSB(NULL, NULL, NULL);

    if(m_hMenu) {
        DestroyMenu(m_hMenu);
        m_hMenu = NULL;
    }

    //
    // only do this if we are active
    //

    if(uState != SVUIA_DEACTIVATE) {
        m_uState = uState;

        //
        // update the Explorer's menu
        //

        if(m_hMenu == NULL)
            m_hMenu = CreateMenu();

        if(m_hMenu) {
            MENUITEMINFO   mii;
            TCHAR szText[MAX_PATH];

            m_pShellBrowser->InsertMenusSB(m_hMenu, &m_MenuWidths);

            //
            // get the menu item's text
            //
            LoadString(g_hInst, IDS_MI_REGISTRY, szText, sizeof(szText));

            //
            // build our menu
            //

            ZeroMemory(&mii, sizeof(mii));
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_STATE;
            mii.fType = MFT_STRING;
            mii.fState = MFS_ENABLED;
            mii.dwTypeData = szText;
            mii.hSubMenu = BuildMenu();

            //
            // insert our menu
            //

            InsertMenuItem(m_hMenu, FCIDM_MENU_HELP, FALSE, &mii);

            m_pShellBrowser->SetMenuSB(m_hMenu, NULL, m_hWnd);
        }

        //
        // TODO: update the status bar
        //

/*

        TCHAR szName[MAX_PATH];

        LoadString(g_hInst, IDS_PSTORE_TITLE, szName, sizeof(szName));

        m_pSFParent->GetPidlFullText(m_pidl, szName + lstrlen(szName), sizeof(szName) - lstrlen(szName));

        LRESULT  lResult;
        int      nPartArray[1] = {-1};

        //set the number of parts
        m_pShellBrowser->SendControlMsg(FCW_STATUS, SB_SETPARTS, 1, (LPARAM)nPartArray, &lResult);

        //set the text for the parts
        m_pShellBrowser->SendControlMsg(FCW_STATUS, SB_SETTEXT, 0, (LPARAM)szName, &lResult);

 */
    }

    return S_OK;
}


HMENU
CShellView::BuildMenu(
    void
    )
{
    HMENU hSubMenu = CreatePopupMenu();

    if(hSubMenu == NULL)
        return NULL;

    TCHAR          szText[MAX_PATH];
    MENUITEMINFO   mii;
    int            nButtons, i;

    //
    // get the number of items in our global array
    //

    for(nButtons = 0; g_Buttons[nButtons].idCommand != -1; nButtons++) {
        ;
    }

    //
    // add the menu items
    //

    for(i = 0; i < nButtons; i++) {

        LoadString(g_hInst, g_Buttons[i].idMenuString, szText, sizeof(szText));

        ZeroMemory(&mii, sizeof(mii));
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;

        if(TBSTYLE_SEP != g_Buttons[i].bStyle) {

            mii.fType = MFT_STRING;
            mii.fState = MFS_ENABLED;
            mii.dwTypeData = szText;
            mii.wID = g_Buttons[i].idCommand;
        } else {
            mii.fType = MFT_SEPARATOR;
        }

        //
        // tack this item onto the end of the menu
        //

        InsertMenuItem(hSubMenu, (UINT)-1, TRUE, &mii);
    }

    //
    // add a couple more that aren't in the button array
    //

    LoadString(g_hInst, IDS_MI_VIEW_ISTB, szText, sizeof(szText));

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
    mii.fType = MFT_STRING;
    mii.fState = MFS_ENABLED;
    mii.dwTypeData = szText;
    mii.wID = IDM_VIEW_ISTB;

    //
    // tack this item onto the end of the menu
    //

    InsertMenuItem(hSubMenu, (UINT)-1, TRUE, &mii);

    LoadString(g_hInst, IDS_MI_VIEW_IETB, szText, sizeof(szText));

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
    mii.fType = MFT_STRING;
    mii.fState = MFS_ENABLED;
    mii.dwTypeData = szText;
    mii.wID = IDM_VIEW_IETB;

    //
    // tack this item onto the end of the menu
    //

    InsertMenuItem(hSubMenu, (UINT)-1, TRUE, &mii);

    return hSubMenu;
}


STDMETHODIMP
CShellView::Refresh(
    void
    )
/*++

    IShellView::Refresh

--*/
{
    //
    // empty the list
    //

    ListView_DeleteAllItems(m_hwndList);

    //
    // refill the list
    //

    FillList();

    return S_OK;
}

STDMETHODIMP
CShellView::CreateViewWindow(
    LPSHELLVIEW pPrevView,
    LPCFOLDERSETTINGS lpfs,
    LPSHELLBROWSER psb,
    LPRECT prcView,
    HWND *phWnd
    )
/*++

    CreateViewWindow creates a view window. This can be either the right pane
    of the Explorer or the client window of a folder window.

--*/
{
    WNDCLASS wc;

    *phWnd = NULL;

    //
    // if our window class has not been registered, then do so
    //

    if(!GetClassInfo(g_hInst, NS_CLASS_NAME, &wc)) {

        ZeroMemory(&wc, sizeof(wc));
        wc.style          = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc    = (WNDPROC)WndProc;
        wc.cbClsExtra     = 0;
        wc.cbWndExtra     = 0;
        wc.hInstance      = g_hInst;
        wc.hIcon          = NULL;
        wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszMenuName   = NULL;
        wc.lpszClassName  = NS_CLASS_NAME;

        if(!RegisterClass(&wc))
            return E_FAIL;
    }

    //
    // set up the member variables
    //

    m_pShellBrowser = psb;
    m_FolderSettings.ViewMode = lpfs->ViewMode;
    m_FolderSettings.fFlags = lpfs->fFlags;

    //
    // get our parent window
    //

    m_pShellBrowser->GetWindow(&m_hwndParent);

    GetSettings();

    *phWnd = CreateWindowEx(   0,
                               NS_CLASS_NAME,
                               NULL,
                               WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                               prcView->left,
                               prcView->top,
                               prcView->right - prcView->left,
                               prcView->bottom - prcView->top,
                               m_hwndParent,
                               NULL,
                               g_hInst,
                               (LPVOID)this);

    if(*phWnd == NULL)
        return E_FAIL;

    //
    // addref the IShellBrowser interface to allow communication with Explorer
    //

    m_pShellBrowser->AddRef();

    return S_OK;
}


STDMETHODIMP
CShellView::DestroyViewWindow(
    void
    )
/*++

    The Explorer calls this method when a folder window or the Explorer
    is being closed.

--*/
{
    if(m_hMenu)
        DestroyMenu(m_hMenu);

    DestroyWindow(m_hWnd);

    //
    // release the shell browser object
    //

    m_pShellBrowser->Release();

    return S_OK;
}

STDMETHODIMP
CShellView::GetCurrentInfo(
    LPFOLDERSETTINGS lpfs
    )
/*++

    The Explorer uses GetCurrentInfo to query the view for standard settings.

--*/
{
// TODO, check proper approach.

    *lpfs = m_FolderSettings;
//    lpfs->ViewMode = m_FolderSettings.ViewMode;
//    lpfs->fFlags = m_FolderSettings.fFlags;

    return NOERROR;
}


STDMETHODIMP
CShellView::AddPropertySheetPages(
    DWORD dwReserved,
    LPFNADDPROPSHEETPAGE lpfn,
    LPARAM lParam
    )
/*++
    The Explorer calls this method when it is opening the View.Options...
    property sheet. Views can add pages by creating them and calling the
    callback function with the page handles.
--*/
{
    OutputDebugStringA("IShellView::AddPropertySheetPages\n");
    return E_NOTIMPL;
}



STDMETHODIMP
CShellView::SaveViewState(
    void
    )
/*++

    IShellView::SaveViewState

--*/
{
    SaveSettings();
    return S_OK;
}


STDMETHODIMP
CShellView::SelectItem(
    LPCITEMIDLIST pidlItem,
    UINT uFlags
    )
/*++

    IShellView::SelectItem

--*/
{
    return E_NOTIMPL;
}


STDMETHODIMP
CShellView::GetItemObject(
    UINT uItem,
    REFIID riid,
    LPVOID *ppvOut
    )
/*++

    Used by the common dialogs to get the selected items from the view.

--*/
{
    return E_NOTIMPL;
}



LRESULT
CALLBACK
CShellView::WndProc(
    HWND hWnd,
    UINT uMessage,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CShellView  *pThis = (CShellView*)GetWindowLong(hWnd, GWL_USERDATA);

    switch (uMessage)
    {
        case WM_NCCREATE:
        {
            LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
            pThis = (CShellView*)(lpcs->lpCreateParams);
            SetWindowLong(hWnd, GWL_USERDATA, (LONG)pThis);

            //set the window handle
            pThis->m_hWnd = hWnd;
            break;
        }

        case WM_SIZE:
            return pThis->OnSize(LOWORD(lParam), HIWORD(lParam));

        case WM_CREATE:
            return pThis->OnCreate();

        case WM_SETFOCUS:
            return pThis->OnSetFocus();

        case WM_ACTIVATE:
            return pThis->OnActivate(wParam, lParam);

        case WM_COMMAND:
            return pThis->OnCommand( GET_WM_COMMAND_ID(wParam, lParam),
                                     GET_WM_COMMAND_CMD(wParam, lParam),
                                     GET_WM_COMMAND_HWND(wParam, lParam));

        case WM_INITMENUPOPUP:
            return pThis->UpdateMenu((HMENU)wParam);

        case WM_NOTIFY:
            return pThis->OnNotify((UINT)wParam, (LPNMHDR)lParam);
    }

    return DefWindowProc(hWnd, uMessage, wParam, lParam);
}


LRESULT
CShellView::OnSetFocus(
    void
    )
{
    //
    // tell the browser we got the focus
    //

    m_pShellBrowser->OnViewWindowActive(this);

    return 0;
}


LRESULT
CShellView::OnActivate(
    WPARAM wParam,
    LPARAM lParam
    )
{
    //
    // tell the browser we got the focus
    //

    if(wParam)
        m_pShellBrowser->OnViewWindowActive(this);

    return 0;
}


LRESULT
CShellView::OnCommand(
    DWORD dwCmdID,
    DWORD dwCmd,
    HWND hwndCmd
    )
{
    switch(dwCmdID)
    {
       case IDM_MESSAGE1:
            MessageBox(m_hWnd, TEXT("Message 1 was selected"), TEXT("Notice"), MB_OK | MB_ICONEXCLAMATION);
            break;

       case IDM_MESSAGE2:
            MessageBox(m_hWnd, TEXT("Message 2 was selected"), TEXT("Notice"), MB_OK | MB_ICONEXCLAMATION);
            break;

        case IDM_VIEW_ISTB:
            m_bShowISTB = ! m_bShowISTB;
            UpdateToolbar();
            break;

        case IDM_VIEW_IETB:
            m_bShowIETB = ! m_bShowIETB;
            UpdateToolbar();
            break;
    }

    return 0;
}


HRESULT
CShellView::SaveSettings(
    void
    )
/*++

    Called by IShellView::SaveViewState to store the relevant setting data
    to the browser stream.

--*/
{
    HRESULT  hr;
    LPSTREAM pStream;

    hr = m_pShellBrowser->GetViewStateStream(STGM_WRITE, &pStream);

    if(SUCCEEDED(hr))
    {
        ULONG uWritten;

        pStream->Write(&m_bShowISTB, sizeof(m_bShowISTB), &uWritten);
        pStream->Write(&m_bShowIETB, sizeof(m_bShowIETB), &uWritten);
        pStream->Write(&m_nColumn1, sizeof(m_nColumn1), &uWritten);
        pStream->Write(&m_nColumn2, sizeof(m_nColumn2), &uWritten);

        pStream->Release();
    }

    return hr;
}


HRESULT
CShellView::GetSettings(
    void
    )
{
    HRESULT  hr;
    LPSTREAM pStream;

    hr = m_pShellBrowser->GetViewStateStream(STGM_READ, &pStream);

    if(SUCCEEDED(hr)) {

        ULONG uRead;

        if(S_OK != pStream->Read(&m_bShowISTB, sizeof(m_bShowISTB), &uRead))
            m_bShowISTB = FALSE;

        if(S_OK != pStream->Read(&m_bShowIETB, sizeof(m_bShowIETB), &uRead))
            m_bShowIETB = FALSE;

        if(S_OK != pStream->Read(&m_nColumn1, sizeof(m_nColumn1), &uRead))
            m_nColumn1 = INITIAL_COLUMN_POS;

        if(S_OK != pStream->Read(&m_nColumn2, sizeof(m_nColumn2), &uRead))
            m_nColumn2 = INITIAL_COLUMN_POS;

        pStream->Release();
    }

    return hr;
}

LRESULT
CShellView::UpdateMenu(
    HMENU hMenu
    )
{
    return 0;
}


void
CShellView::UpdateToolbar()
{
    // nothing to update yet
}


LRESULT
CShellView::OnNotify(
    UINT CtlID,
    LPNMHDR pNotify
    )
{
    LPNM_LISTVIEW pNotifyLV = (LPNM_LISTVIEW)pNotify;
    LV_DISPINFO *pNotifyDI = (LV_DISPINFO *)pNotify;
    LPTBNOTIFY pNotifyTB = (LPTBNOTIFY)pNotify;
    LPTOOLTIPTEXT pNotifyTT = (LPTOOLTIPTEXT)pNotify;

    switch(pNotify->code)
    {

		//
		// HDN_BEGINTRACK
		// HDN_DIVIDERDBLCLICK
		// HDN_ENDTRACK
		// HDN_ITEMCHANGED
		// HDN_ITEMCHANGING
		// HDN_ITEMCLICK
		// HDN_ITEMDBLCLICK
		// HDN_TRACK
		//

        case HDN_ENDTRACK:
        {
            m_nColumn1 = ListView_GetColumnWidth(m_hwndList, 0);
            m_nColumn2 = ListView_GetColumnWidth(m_hwndList, 1);

            break;
        }
		
		//
		// LVN_BEGINDRAG
		// LVN_BEGINLABELEDIT
		// LVN_BEGINRDRAG
		// LVN_COLUMNCLICK
		// LVN_DELETEALLITEMS
		// LVN_DELETEITEM
		// LVN_ENDLABELEDIT
		// LVN_GETDISPINFO
		// LVN_INSERTITEM
		// LVN_ITEMCHANGED
		// LVN_ITEMCHANGING
		// LVN_KEYDOWN
		// LVN_SETDISPINFO
		//

        case LVN_DELETEITEM:
        {
            LPITEMIDLIST pidl = (LPITEMIDLIST)(pNotifyLV->lParam);

            //
            // free the memory for the pidl associated with this item.
            //

            if(pidl != NULL)
                FreePidl(pidl);

            break;
        }

		// NM_CLICK
		// NM_DBLCLK
		// NM_KILLFOCUS
		// NM_LISTVIEW
		// NM_OUTOFMEMORY
		// NM_RCLICK
		// NM_RDBLCLK
		// NM_RETURN
		// NM_SETFOCUS
		// NM_TREEVIEW
		// NM_UPDOWN
		//

		case NM_RETURN:
		case NM_DBLCLK:
		{
            UINT wFlags = SBSP_RELATIVE | SBSP_DEFMODE;
            int iItem = -1;

            //
            // if we are in explorer mode, start out in DEFBROWSER mode,
            // otherwise, start in NEWBROWSER mode.
            //

            HWND hwnd = NULL;
            m_pShellBrowser->GetControlWindow(FCW_TREE, &hwnd);

            if(hwnd != NULL) {
                wFlags |= SBSP_DEFBROWSER;
            } else {
                wFlags |= SBSP_NEWBROWSER;
            }


			//
			// loop through all the selected items.
			// TODO: if more than two are selected, we only get to view
            // two.
			//
			 
            do {
	            LV_ITEM item;

				iItem = ListView_GetNextItem(m_hwndList, iItem, LVNI_SELECTED | LVNI_ALL);
				ZeroMemory(&item, sizeof(item));
				item.mask = LVIF_PARAM;
				item.iItem = iItem;

				if(!ListView_GetItem(m_hwndList, &item))
					break;

	            LPCITEMIDLIST pidl = (LPCITEMIDLIST)(item.lParam);
				
	            if(pidl != NULL) {
				
					//
					// don't browse into items.
					//

					if(GetLastPidlType(pidl) > PIDL_TYPE_SUBTYPE)
						break;

					m_pShellBrowser->BrowseObject(pidl, wFlags);
				}
				
				//
				// if multiple items selected, bring up a new browser
				//

				wFlags &= ~SBSP_DEFBROWSER;
				wFlags |= SBSP_NEWBROWSER;
            } while (iItem != -1);

		}

		//
		// TBN_BEGINADJUST
		// TBN_BEGINDRAG
		// TBN_CUSTHELP
		// TBN_ENDADJUST
		// TBN_ENDDRAG
		// TBN_GETBUTTONINFO
		// TBN_QUERYDELETE
		// TBN_QUERYINSERT
		// TBN_RESET
		// TBN_TOOLBARCHANGE
		//

        case TBN_BEGINDRAG:
        default:
            return 0;
    }

    return 0;
}


LRESULT
CShellView::OnSize(
    WORD wWidth,
    WORD wHeight
    )
{
    //
    // resize the ListView to fit our window
    //

    if(m_hwndList)
        MoveWindow(m_hwndList, 0, 0, wWidth, wHeight, TRUE);

    return 0;
}


LRESULT
CShellView::OnCreate(
    void
    )
{
    //
    // create the ListView
    //

    if(CreateList())
    {
        if(InitList())
        {
            FillList();
        }
    }

    return 0;
}


BOOL
CShellView::CreateList(
    void
    )
{
    DWORD dwStyle;

    //
    // fetch the prior listview style and use
    // that as the new listview style.
    //


    switch (m_FolderSettings.ViewMode) {
        case FVM_ICON:
            dwStyle = LVS_ICON;
            break;
        case FVM_SMALLICON:
            dwStyle = LVS_SMALLICON;
            break;
        case FVM_LIST:
            dwStyle = LVS_LIST;
            break;
        case FVM_DETAILS:
            dwStyle = LVS_REPORT;
            break;
        default:
            dwStyle = LVS_ICON;
            break;
    }

    m_hwndList = CreateWindowEx(
                        WS_EX_CLIENTEDGE,
                        WC_LISTVIEW,
                        NULL,
                        WS_TABSTOP |
                        WS_VISIBLE |
                        WS_CHILD |
                        WS_BORDER |
                        LVS_SORTASCENDING |
                        dwStyle |
                        LVS_NOSORTHEADER |
                        0,
                        0,
                        0,
                        0,
                        0,
                        m_hWnd,
                        (HMENU)ID_LISTVIEW,
                        g_hInst,
                        NULL
                        );

    return (BOOL)(NULL != m_hwndList);
}


BOOL
CShellView::InitList(
    void
    )
{
    LV_COLUMN   lvColumn;
    HIMAGELIST  himlSmall;
    HIMAGELIST  himlLarge;
    RECT        rc;
    TCHAR       szString[MAX_PATH];

    //
    // empty the list
    //

    ListView_DeleteAllItems(m_hwndList);

    //
    // initialize the columns
    //

    lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvColumn.fmt = LVCFMT_LEFT;
    lvColumn.pszText = szString;

    lvColumn.cx = m_nColumn1;
    LoadString(g_hInst, IDS_COLUMN1, szString, sizeof(szString));
    ListView_InsertColumn(m_hwndList, 0, &lvColumn);

    GetClientRect(m_hWnd, &rc);

    lvColumn.cx = m_nColumn2;
    LoadString(g_hInst, IDS_COLUMN2, szString, sizeof(szString));
    ListView_InsertColumn(m_hwndList, 1, &lvColumn);

	int cxSmall;
	int cySmall;
	int cxLarge;
	int cyLarge;

	SHFILEINFO sfi;
	HIMAGELIST himl;

	//
	// get large and small icon size info from shell
	//

	himl = (HIMAGELIST)SHGetFileInfo(
			NULL,
			0,
			&sfi,
			sizeof(sfi),
			SHGFI_LARGEICON | SHGFI_SYSICONINDEX
			);

	if(!ImageList_GetIconSize(himl, &cxLarge, &cyLarge)) {
    	cxLarge = GetSystemMetrics(SM_CXICON);
    	cyLarge = GetSystemMetrics(SM_CYICON);
	}

	himl = (HIMAGELIST)SHGetFileInfo(
			NULL,
			0,
			&sfi,
			sizeof(sfi),
			SHGFI_SMALLICON | SHGFI_SYSICONINDEX
			);

	if(!ImageList_GetIconSize(himl, &cxSmall, &cySmall)) {
    	cxSmall = GetSystemMetrics(SM_CXSMICON);
    	cySmall = GetSystemMetrics(SM_CYSMICON);
	}


    //
    // Create the full-sized and small icon image lists.
    //

    himlLarge = ImageList_Create(
            cxLarge,
            cyLarge,
            ILC_COLORDDB | ILC_MASK,
            4,  // initial image count
            0
            );

    himlSmall = ImageList_Create(
            cxSmall,
            cySmall,
            ILC_COLORDDB | ILC_MASK,
            4,  // initial image count
            0
            );

    //
    // Add icons to each image list.
	// TODO cache icons
    //

    HICON hIconSmall;
	HICON hIconLarge;

    // icon 0: Provider icon

    hIconSmall = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_PROVIDER), IMAGE_ICON, cxSmall, cySmall, LR_DEFAULTCOLOR | LR_SHARED);
    hIconLarge = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_PROVIDER), IMAGE_ICON, cxLarge, cyLarge, LR_DEFAULTCOLOR | LR_SHARED);

    ImageList_AddIcon(himlSmall, hIconSmall);
    ImageList_AddIcon(himlLarge, hIconLarge);


    // icon 1: type icon

    hIconSmall = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_FOLDER), IMAGE_ICON, cxSmall, cySmall, LR_DEFAULTCOLOR | LR_SHARED);
    hIconLarge = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_FOLDER), IMAGE_ICON, cxLarge, cyLarge, LR_DEFAULTCOLOR | LR_SHARED);

    ImageList_AddIcon(himlSmall, hIconSmall);
    ImageList_AddIcon(himlLarge, hIconLarge);

    // icon 2: item icon

    hIconSmall = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_ITEM), IMAGE_ICON, cxSmall, cySmall, LR_DEFAULTCOLOR | LR_SHARED);
    hIconLarge = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_ITEM), IMAGE_ICON, cxLarge, cyLarge, LR_DEFAULTCOLOR | LR_SHARED);

    ImageList_AddIcon(himlSmall, hIconSmall);
    ImageList_AddIcon(himlLarge, hIconLarge);

    // icon 3: global type

    hIconSmall = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_GLOBAL), IMAGE_ICON, cxSmall, cySmall, LR_DEFAULTCOLOR | LR_SHARED);
    hIconLarge = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_GLOBAL), IMAGE_ICON, cxLarge, cyLarge, LR_DEFAULTCOLOR | LR_SHARED);

    ImageList_AddIcon(himlSmall, hIconSmall);
    ImageList_AddIcon(himlLarge, hIconLarge);


    //
    // Assign the image lists to the list view control.
    //

    ListView_SetImageList(m_hwndList, himlLarge, LVSIL_NORMAL);
    ListView_SetImageList(m_hwndList, himlSmall, LVSIL_SMALL);

    return TRUE;
}

BOOL
CShellView::FillList(
    void
    )
{
    LPENUMIDLIST pEnumIDList;
    ULONG ulFetched;
    LPITEMIDLIST pidl = NULL;

    pEnumIDList = new CEnumIDList(m_pidl, TRUE);
    if(pEnumIDList == NULL)
        return FALSE;

    //
    // enumerate the sub folders or items, based on the parent m_pidl.
    // note that storage is allocated for the pidlNew, which is a copy
    // of the current path + pidl - that storage is later freed during
    // the processing of LVN_DELETEITEM
    //

    while( NOERROR == pEnumIDList->Next(1, &pidl, &ulFetched) ) {

        LPCWSTR szText = GetPidlText(pidl);
        PST_KEY KeyType = GetPidlKeyType(pidl);
        DWORD dwPidlType = GetPidlType(pidl);

        //
        // make a fully qualified (absolute) pidl
        //

//        LPITEMIDLIST pidlNew = CopyCatPidl(m_pidl, pidl);

        //
        // free pidl associated with Next enumeration
        //

//        FreePidl(pidl);

        //
        // add item with absolute pidl to listview
        //

        AddListItem(m_hwndList, dwPidlType, KeyType, szText, pidl);
    }

    pEnumIDList->Release();

    return TRUE;
}

BOOL
AddListItem(
    HWND hWnd,
    DWORD dwType,
    PST_KEY KeyType,
    LPCWSTR szName,
    LPITEMIDLIST pidl
    )
/*++

    This function is a temporary hack to support image list add/set data.

    This will be replaced by an Unicode interface that operates on WinNT and
    Win95 in the near future.

--*/
{
    LVITEM lvItem;
    int nIndex;

    DWORD dwIconIndex;

    //
    // determine which icon to use.
    // when PST_KEY is not PST_KEY_LOCAL_MACHINE, use a different folder
    // icon at the type and subtype level.
    //

    switch(dwType) {
        case PIDL_TYPE_PROVIDER:
            dwIconIndex = 0;
            break;
        case PIDL_TYPE_TYPE:
        case PIDL_TYPE_SUBTYPE:
            if(KeyType == PST_KEY_CURRENT_USER)
                dwIconIndex = 1;
            else
                dwIconIndex = 3;    // global (shared) icon
            break;
        case PIDL_TYPE_ITEM:
            dwIconIndex = 2;
            break;
        default:
            dwIconIndex = 0;
    }

    ZeroMemory(&lvItem, sizeof(lvItem));

    lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    lvItem.iItem = 0;
    lvItem.lParam = (LPARAM)pidl;
    lvItem.pszText = (LPTSTR)szName;
    lvItem.iImage = dwIconIndex; // image index

    nIndex = ListView_InsertItemU(hWnd, &lvItem);

//    ListView_SetItemTextU(hWnd, nIndex, 1, (LPWSTR)szValue);

    return TRUE;
}

