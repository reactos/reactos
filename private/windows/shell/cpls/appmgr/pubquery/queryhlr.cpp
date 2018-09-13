/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    queryhlr.cpp

Abstract:

    This module contains the implementation for the published application
    query handler.

Author:

    Dave Hastings (daveh) creation-date 14-Nov-1997

Revision History:

--*/
#include "pubquery.h"
#include "cstore.h"

LRESULT CALLBACK
ResultViewWndProc(
    HWND Window,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    );

//
// creator/destructor
//
CPublishedApplicationQueryHandler::CPublishedApplicationQueryHandler(LPUNKNOWN IUnknown)
{
    InterlockedIncrement(&g_RefCount);
    m_IUnknown = IUnknown;
    m_QueryFrame = NULL;
    m_Flags = 0;
}

CPublishedApplicationQueryHandler::~CPublishedApplicationQueryHandler()
{
    InterlockedDecrement(&g_RefCount);
}

//
// IUnknown
//
STDMETHODIMP CPublishedApplicationQueryHandler::QueryInterface(
    REFIID riid,
    PVOID *ppvInterface
    )
/*++

Routine Description:

    This is the query interface function for the query handler.

Arguments:

    riid - Supplies the iid of the interface desired.
    ppvInterface - Returns the interface pointer.

Return Value:

--*/
{
    return m_IUnknown->QueryInterface(riid, ppvInterface);
}

STDMETHODIMP_(ULONG) CPublishedApplicationQueryHandler::AddRef(
    VOID
    )
/*++

Routine Description:

    This is the AddRef entrypoint for the query handler.

Arguments:

    None.

Return Value:

    New reference count

--*/

{
    return InterlockedIncrement(&m_RefCount);
}

STDMETHODIMP_(ULONG) CPublishedApplicationQueryHandler::Release(
    VOID
    )
/*++

Routine Description:

    This is the Release function for the query handler.

Arguments:

    None.

Return Value:

    the new ref count

--*/
{
    LONG NewRefCount;

    NewRefCount = InterlockedDecrement(&m_RefCount);

    if (NewRefCount == 0) {
        delete this;
    }

    return NewRefCount;
}

STDMETHODIMP CPublishedApplicationQueryHandler::Initialize(
    THIS_ IQueryFrame *QueryFrame, 
    DWORD Flags, 
    LPVOID Parameters
    )
/*++

Routine Description:

    This function implements the Initialize function for the query handler.

Arguments:

    QueryFram - Supplies the IQueryFrame interface.
    Flags - Supplies the flags from ICommonQuery->OpenQueryWindow.
    Parameters - Supplies the parameters from ICommonQuery->OpenQueryWindow.

Return Value:


--*/
{
    WNDCLASS wc;

    OutputDebugString(L"Initialize\n");

    //
    // Register the result view window class
    //
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = ResultViewWndProc;
    wc.hInstance = Instance;
    wc.lpszClassName = L"ResultViewWndClass"; // bugbug
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    //wc.hbrBackground = (HBRUSH)(COLOR_DESKTOP + 1);
    RegisterClass(&wc);

    m_QueryFrame = QueryFrame;
    m_Flags = Flags;
    m_ResultView = NULL;
    m_ListView = NULL;
    m_Add = NULL;
    m_About = NULL;
    m_MenuBar = NULL;
    m_ItemSelected = FALSE;

    //
    // Load the menus 
    //
    // bugbug failure?
    m_FileMenu = LoadMenu(Instance, MAKEINTRESOURCE(IDR_MENU1));


    return S_OK;
}

STDMETHODIMP CPublishedApplicationQueryHandler::GetViewInfo(
    THIS_ LPCQVIEWINFO ViewInfo
    )
/*++

Routine Description:

    This function implements the GetViewInfo method for the query handler.

Arguments:

    ViewInfo - Returns the view info.

Return Value:


--*/
{
    OutputDebugString(L"GetViewInfo\n");

    ViewInfo->dwFlags = 0;
    ViewInfo->hInstance = Instance;
    // bugbug
    ViewInfo->idLargeIcon = NULL;
    ViewInfo->idSmallIcon = NULL;
    ViewInfo->idTitle = NULL;
    ViewInfo->idAnimation = NULL;

    return S_OK;
}

STDMETHODIMP CPublishedApplicationQueryHandler::AddScopes(
   THIS
   )
/*++

Routine Description:

    This is the AddScopes method for the query handler.
    We only have one, so we just return E_NOTIMPL.

Arguments:

    None

Return Value:

    E_NOTIMPL

--*/
{
    CQSCOPE Scope;

    OutputDebugString(L"AddScopes\n");
    return E_NOTIMPL;
}

STDMETHODIMP CPublishedApplicationQueryHandler::BrowseForScope(
    THIS_ HWND Parent, 
    LPCQSCOPE CurrentScope, 
    LPCQSCOPE* ppScope
    )
/*++

Routine Description:

    This is the browse for scope method for the query handler.
    We don't have any scopes, so we didn't  implement it.

Arguments:

    argument-name - Supplies | Returns description of argument.
    .
    .

Return Value:

    E_NOTIMPL
    
--*/
{
    OutputDebugString(L"BrowseForScope\n");
    return E_NOTIMPL;
}

STDMETHODIMP CPublishedApplicationQueryHandler::CreateResultView(
    THIS_ HWND Parent, 
    HWND* WndView
    )
/*++

Routine Description:

    This function is the CreateResultsView method for the query handler.
    It creates the result pane.

Arguments:

    Parent - Supplies the parent window handle.
    WndView - Returns the handle to the result view window.

Return Value:


--*/
{
    HWND hwnd;
    RECT rc;
    LV_COLUMN Column;

    OutputDebugString(L"CreateResultView\n");

    //
    // Create the parent window for the controls
    //
    m_ResultView = CreateWindow(
        L"ResultViewWndClass",
        NULL,
        WS_TABSTOP|WS_CLIPCHILDREN|WS_CHILD|WS_VISIBLE,
        0, 0, 0, 0,
        Parent,
        NULL,
        Instance,
        this
        );

    GetClientRect(m_ResultView, &rc);

    //
    // Create the list view to display the published applications
    //
    InitCommonControls();

    m_ListView = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        WC_LISTVIEW,
        NULL,
        WS_TABSTOP|WS_CLIPCHILDREN|WS_CHILD|WS_VISIBLE | LVS_AUTOARRANGE|LVS_SHAREIMAGELISTS|LVS_SHOWSELALWAYS|LVS_REPORT,
        0, 0,
        rc.right, rc.bottom,
        m_ResultView,
        NULL,
        Instance,
        NULL
        );

    //
    // Create the columns for the list view
    //
    Column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    Column.cx = 300;
    Column.pszText = L"Application Name";
    ListView_InsertColumn(m_ListView, 0, &Column);

    Column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    Column.cx = 100;
    Column.pszText = L"Version";
    Column.iSubItem = 1;
    ListView_InsertColumn(m_ListView, 1, &Column);
    //
    // Create the Add and About buttons
    //
    m_Add = CreateWindow(
        L"BUTTON",
        L"Add",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        0,
        0,
        0,
        0,
        m_ResultView,
        NULL,
        Instance,
        NULL
        );

    m_About = CreateWindow(
        L"BUTTON",
        L"About",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        0,
        0,
        0,
        0,
        m_ResultView,
        NULL,
        Instance,
        NULL
        );

    *WndView = m_ResultView;

    return S_OK;
    
}

STDMETHODIMP CPublishedApplicationQueryHandler::ActivateView(
    THIS_ UINT State,
    WPARAM wParam, 
    LPARAM lParam
    )
/*++

Routine Description:

    This function is the ActivateView method for the query handler.

Arguments:

    argument-name - Supplies | Returns description of argument.
    .
    .

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/
{
    HRESULT hr;
    HMENU Menu;
    OLEMENUGROUPWIDTHS omgw;
    UINT MenuState;

    OutputDebugString(L"ActivateView\n");

    switch (State) {
        case CQRVA_ACTIVATE:
            OutputDebugString(L"ActivateView -- Activate\n");
            Menu = CreateMenu();

            hr = m_QueryFrame->InsertMenus(Menu, &omgw);

            Shell_MergeMenus(GetSubMenu(Menu, 0), GetSubMenu(m_FileMenu, 0), 0, 0, 0x7fff, 0);

            hr = m_QueryFrame->SetMenu(Menu, NULL);

            return S_OK;

        case CQRVA_INITMENUBAR:
            OutputDebugString(L"ActivateView -- InitMenuBar\n");

            m_MenuBar = (HMENU)wParam;

            return S_OK;

        case CQRVA_INITMENUBARPOPUP:
            OutputDebugString(L"ActivateView -- InitMenuBarPopup\n");

            //
            // bugbug this doesn't seem to cause the menu items to get grayed
            //

            //
            // Only enable our menu items if an item has been selected
            //
            if (m_ItemSelected) {
                MenuState = MF_ENABLED | MF_BYCOMMAND;
            } else {
                MenuState = MF_GRAYED | MF_BYCOMMAND;
            }

            Menu = (HMENU)wParam;
            EnableMenuItem(Menu, PAQ_FILE_ADD, m_ItemSelected);
            EnableMenuItem(Menu, PAQ_FILE_ABOUT, m_ItemSelected);

            return S_OK;
    }

    return S_OK;
}

STDMETHODIMP CPublishedApplicationQueryHandler::InvokeCommand(
    THIS_ HWND hwndParent, 
    UINT idCmd
    )
{
    OutputDebugString(L"InvokeCommand\n");
    return E_NOTIMPL;
}

STDMETHODIMP CPublishedApplicationQueryHandler::GetCommandString(
    THIS_ UINT Command, 
    DWORD dwFlags, 
    LPTSTR Buffer, 
    INT cchBuffer
    )
{
    //
    // bugbug what are the flags for?
    //
    OutputDebugString(L"GetCommandString\n");

    switch (Command) {
        case PAQ_FILE_ADD:

            if (!LoadString(Instance, IDS_FILE_ADD_EXPL, Buffer, cchBuffer)){
                return E_FAIL;
            }

            break;

        case PAQ_FILE_ABOUT:

            if (!LoadString(Instance, IDS_FILE_ABOUT_EXPL, Buffer, cchBuffer)){
                return E_FAIL;
            }

            break;

        default:

            return E_NOTIMPL;
    }

    return S_OK;
}

STDMETHODIMP CPublishedApplicationQueryHandler::IssueQuery(
    THIS_ LPCQPARAMS pQueryParams
    )
{
    PQUERYPARAMETERS QueryParameters;
    HRESULT hr;
    IEnumPackage *IEnumPackage;
    PACKAGEDISPINFO Packages[50];
    ULONG PackagesEnumed;

    OutputDebugString(L"IssueQuery\n");

    //
    // Get the parameters typed by the user
    //
    QueryParameters = (PQUERYPARAMETERS)pQueryParams->pQueryParameters;

    //
    // Create a query for those apps (consider another thread, better interface
    // management)
    //
    hr = CsEnumApps(
        QueryParameters->ApplicationName,
        NULL,
        NULL,
        APPINFO_PUBLISHED | APPINFO_VISIBLE | APPINFO_MSI,
        &IEnumPackage
        );
    
    FailGracefully(hr, "Couldn't get package enumerator\n");

    hr = IEnumPackage->Next(
        50,
        Packages,
        &PackagesEnumed
        );

    IEnumPackage->Release();

    FailGracefully(hr, "Couldn't enum packages\n");

    PopulateListView(m_ListView, Packages, PackagesEnumed);

exit_gracefully:

    return hr;
}

STDMETHODIMP CPublishedApplicationQueryHandler::StopQuery(
    THIS
    )
{
    OutputDebugString(L"StopQuery\n");
    return E_NOTIMPL;
}

STDMETHODIMP CPublishedApplicationQueryHandler::GetViewObject(
    THIS_ UINT uScope, 
    REFIID riid, 
    LPVOID* ppvOut
    )
{
    OutputDebugString(L"GetViewObject\n");
    return E_NOINTERFACE;
}

STDMETHODIMP CPublishedApplicationQueryHandler::LoadQuery(
    THIS_ IPersistQuery* pPersistQuery
    )
{
    OutputDebugString(L"LoadQuery\n");
    return E_NOTIMPL;
}

STDMETHODIMP CPublishedApplicationQueryHandler::SaveQuery(
    THIS_ IPersistQuery* pPersistQuery, 
    LPCQSCOPE pScope
    )
{
    OutputDebugString(L"SaveQuery\n");
    return E_NOTIMPL;
}

LRESULT
CPublishedApplicationQueryHandler::OnSize(
    INT Width,
    INT Height
    )
{
    RECT rc;

    // bugbug?
    GetClientRect(m_ResultView, &rc);

    SetWindowPos(
        m_ListView, 
        NULL, 
        0, 
        0, 
        Width - 100, 
        Height, 
        SWP_NOZORDER|SWP_NOMOVE
        );

    SetWindowPos(
        m_Add,
        NULL,
        rc.right - 85,
        rc.top + 25,
        75,
        25,
        SWP_NOZORDER
        );
       
    SetWindowPos(
        m_About,
        NULL,
        rc.right - 85,
        rc.top + 75,
        75,
        24,
        SWP_NOZORDER
        );

    return 0;
}


LRESULT CALLBACK
ResultViewWndProc(
    HWND Window,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    INT Width, Height;
    CPublishedApplicationQueryHandler *QueryHandler;

    if (Msg == WM_CREATE) {
        QueryHandler = (CPublishedApplicationQueryHandler *)((LPCREATESTRUCT)lParam)->lpCreateParams;
        SetWindowLong(Window, GWL_USERDATA, (ULONG)QueryHandler);
    } else {
        QueryHandler = (CPublishedApplicationQueryHandler *)GetWindowLong(
            Window,
            GWL_USERDATA
            );

        switch (Msg) {

            case WM_SIZE: 

                QueryHandler->OnSize(LOWORD(lParam), HIWORD(lParam));
                return 0;

        }
    }

    return DefWindowProc(Window, Msg, wParam, lParam);
}