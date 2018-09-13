/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    componet.cpp

Abstract:

    This module implemets CComponent class

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "devmgr.h"
#include "factory.h"



//
// ctor and dtor
//

CComponent::CComponent(
    CComponentData* pComponentData
    )
{
    m_pComponentData = pComponentData;
    m_pHeader = NULL;
    m_pConsole = NULL;
    m_pResult = NULL;
    m_pConsoleVerb = NULL;
    m_pCurFolder = NULL;
    m_pPropSheetProvider = NULL;
    m_pDisplayHelp = NULL;
    m_MachineAttached = FALSE;
    m_Dirty = FALSE;
    m_pControlbar = NULL;
    m_pToolbar = NULL;
    // increment object count(used by CanUnloadNow)
    ::InterlockedIncrement(&CClassFactory::s_Objects);
    m_Ref = 1;
}

CComponent::~CComponent()
{
    // decrement object count(used by CanUnloadNow)
    ::InterlockedDecrement(&CClassFactory::s_Objects);
}

//
// IUNKNOWN interface
//

ULONG
CComponent::AddRef()
{
    ::InterlockedIncrement((LONG*)&m_Ref);
    return m_Ref;
}

ULONG
CComponent::Release()
{
    ::InterlockedDecrement((LONG*)&m_Ref);
    if (!m_Ref)
    {
        delete this;
        return 0;
    }
    return m_Ref;
}

STDMETHODIMP
CComponent::QueryInterface(
    REFIID  riid,
    void**  ppv
    )
{
    if (!ppv)
        return E_INVALIDARG;
    HRESULT hr = S_OK;


    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (IUnknown*)(IComponent*)this;
    }
    else if (IsEqualIID(riid, IID_IComponent))
    {
        *ppv = (IComponent*)this;
    }
    else if (IsEqualIID(riid, IID_IResultDataCompare))
    {
        *ppv = (IResultDataCompare*)this;
    }
    else if (IsEqualIID(riid, IID_IExtendContextMenu))
    {
        *ppv = (IExtendContextMenu*)this;
    }
    else if (IsEqualIID(riid, IID_IExtendControlbar))
    {
        *ppv = (IExtendControlbar*)this;
    }
    else if (IsEqualIID(riid, IID_IExtendPropertySheet))
    {
        *ppv = (IExtendPropertySheet*)this;
    }
#ifdef PERSIST_VIEW
    else if (IsEqualIID(riid, IID_IPersistStream))
    {
        *ppv = (IPersistStream*)this;
    }
#endif

    else if (IsEqualIID(riid, IID_ISnapinCallback))
    {
        *ppv = (ISnapinCallback*)this;
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }
    if (SUCCEEDED(hr))
    {
        AddRef();
    }
    return hr;
}


//
// IComponent interface implementation
//
STDMETHODIMP
CComponent::GetResultViewType(
    MMC_COOKIE cookie,
    LPOLESTR* ppViewType,
    long* pViewOptions
    )
{

    if (!ppViewType || !pViewOptions)
        return E_INVALIDARG;

    HRESULT hr;

    try
    {
        CFolder* pFolder;
        pFolder = FindFolder(cookie);
        if (pFolder)
            return pFolder->GetResultViewType(ppViewType, pViewOptions);
        else
            return S_OK;
    }
    catch (CMemoryException* e)
    {
        e->Delete();
        MsgBoxParam(m_pComponentData->m_hwndMain, 0, 0, 0);
        return S_FALSE;
    }
}

STDMETHODIMP
CComponent::Initialize(
    LPCONSOLE lpConsole
    )
{
    HRESULT hr;

    if (!lpConsole)
        return E_INVALIDARG;

    m_pConsole = lpConsole;
    lpConsole->AddRef();

    hr = lpConsole->QueryInterface(IID_IHeaderCtrl, (void**)&m_pHeader);

    if (SUCCEEDED(hr))
    {
        lpConsole->SetHeader(m_pHeader);
        hr = lpConsole->QueryInterface(IID_IResultData, (void**)&m_pResult);
    }
    if (SUCCEEDED(hr))
    {
        hr = lpConsole->QueryConsoleVerb(&m_pConsoleVerb);
    }
    if (SUCCEEDED(hr))
    {
        hr = lpConsole->QueryInterface(IID_IPropertySheetProvider,
                                       (void**)&m_pPropSheetProvider);
    }
    if (SUCCEEDED(hr))
    {
        hr = lpConsole->QueryInterface(IID_IDisplayHelp, (void**)&m_pDisplayHelp);
    }

    if (FAILED(hr))
        OutputDebugString(TEXT("CComponent::Initialize failed\n"));
    return hr;
}

#if DBG
TCHAR *mmcNotifyStr[] = {
    TEXT("UNKNOWN"),
    TEXT("ACTIVATE"),
    TEXT("ADD_IMAGES"),
    TEXT("BTN_CLICK"),
    TEXT("CLICK"),
    TEXT("COLUMN_CLICK"),
    TEXT("CONTEXTMENU"),
    TEXT("CUTORMOVE"),
    TEXT("DBLCLICK"),
    TEXT("DELETE"),
    TEXT("DESELECT_ALL"),
    TEXT("EXPAND"),
    TEXT("HELP"),
    TEXT("MENU_BTNCLICK"),
    TEXT("MINIMIZED"),
    TEXT("PASTE"),
    TEXT("PROPERTY_CHANGE"),
    TEXT("QUERY_PASTE"),
    TEXT("REFRESH"),
    TEXT("REMOVE_CHILDREN"),
    TEXT("RENAME"),
    TEXT("SELECT"),
    TEXT("SHOW"),
    TEXT("VIEW_CHANGE"),
    TEXT("SNAPINHELP"),
    TEXT("CONTEXTHELP"),
    TEXT("INITOCX"),
    TEXT("FILTER_CHANGE"),
    TEXT("FILTERBTN_CLICK"),
    TEXT("RESTORE_VIEW"),
    TEXT("PRINT"),
    TEXT("PRELOAD"),
    TEXT("LISTPAD"),
    TEXT("EXPANDSYNC")
    };
#endif

STDMETHODIMP
CComponent::Notify(
    LPDATAOBJECT lpDataObject,
    MMC_NOTIFY_TYPE event,
    LPARAM arg,
    LPARAM param
    )
{
    HRESULT hr;

    INTERNAL_DATA tID;

#if DBG
    UINT i = event - MMCN_ACTIVATE + 1;
    if (event > MMCN_EXPANDSYNC || event < MMCN_ACTIVATE)
    {
        i = 0;
    }
    //TRACE2(TEXT("Componet:Notify, event = %lx %s\n"), event, mmcNotifyStr[i]);
#endif

    try
    {
        if (DOBJ_CUSTOMOCX == lpDataObject)
        {
            return OnOcxNotify(event, arg, param);
        }
        hr = ExtractData(lpDataObject, CDataObject::m_cfSnapinInternal,
                         (PBYTE)&tID, sizeof(tID));

        if (SUCCEEDED(hr))
        {
            switch(event)
            {
                case MMCN_ACTIVATE:
                    hr = OnActivate(tID.cookie, arg, param);
                    break;

                case MMCN_VIEW_CHANGE:
                    hr = OnViewChange(tID.cookie, arg, param);
                    break;

                case MMCN_SHOW:
                    hr = OnShow(tID.cookie, arg, param);
                    break;

                case MMCN_CLICK:
                    hr = OnResultItemClick(tID.cookie, arg, param);
                    break;
                case MMCN_DBLCLICK:
                    hr = OnResultItemDblClick(tID.cookie, arg, param);
                    break;
                case MMCN_MINIMIZED:
                    hr = OnMinimize(tID.cookie, arg, param);
                    break;
                case MMCN_BTN_CLICK:
                    hr = OnBtnClick(tID.cookie, arg, param);
                    break;
                case MMCN_SELECT:
                    hr = OnSelect(tID.cookie, arg, param);
                    break;
                case MMCN_ADD_IMAGES:
                    hr = OnAddImages(tID.cookie, (IImageList*)arg, param);
                    break;
                case MMCN_RESTORE_VIEW:
                    hr = OnRestoreView(tID.cookie, arg, param);
                    break;
                case MMCN_CONTEXTHELP:
                    hr = OnContextHelp(tID.cookie, arg, param);
                    break;
                default:
                    hr = S_OK;
                    break;
            }
        }
        else
        {
            if (MMCN_ADD_IMAGES == event)
            {
                OnAddImages(0, (IImageList*)arg, (HSCOPEITEM)param);
            }
        }
    }
    catch (CMemoryException* e)
    {
        e->Delete();
        MsgBoxParam(m_pComponentData->m_hwndMain, 0, 0, 0);
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDMETHODIMP
CComponent::Destroy(
    MMC_COOKIE cookie
    )
{
    // cookie must point to the static node

    ASSERT(0 == cookie);


    try
    {
        DetachAllFoldersFromMachine();
        DestroyFolderList(cookie);

        if (m_pToolbar)
        {
            m_pToolbar->Release();
        }
        if (m_pControlbar)
        {
            m_pControlbar->Release();
        }

        // Release the interfaces that we QI'ed
        if (m_pConsole != NULL)
        {
            // Tell the console to release the header control interface
            m_pConsole->SetHeader(NULL);
            m_pHeader->Release();

            m_pResult->Release();

            m_pConsoleVerb->Release();

            m_pDisplayHelp->Release();

            // Release the IFrame interface last
            m_pConsole->Release();
        }

        if (m_pPropSheetProvider)
            m_pPropSheetProvider->Release();
    }
    catch (CMemoryException* e)
    {
        e->Delete();
        MsgBoxParam(m_pComponentData->m_hwndMain, 0, 0, 0);
    }
    return S_OK;
}

STDMETHODIMP
CComponent::QueryDataObject(
    MMC_COOKIE cookie,
    DATA_OBJECT_TYPES type,
    LPDATAOBJECT* ppDataObject
    )
{

    try
    {
        ASSERT(m_pComponentData);
        // delegate to IComponentData
        return m_pComponentData->QueryDataObject(cookie, type, ppDataObject);
    }
    catch (CMemoryException* e)
    {
        e->Delete();
        MsgBoxParam(m_pComponentData->m_hwndMain, 0, 0, 0);
        return E_OUTOFMEMORY;
    }
}

STDMETHODIMP
CComponent::GetDisplayInfo(
    LPRESULTDATAITEM pResultDataItem
    )
{
    try
    {
        CFolder* pFolder = FindFolder(pResultDataItem->lParam);
        if (pFolder)
            return pFolder->GetDisplayInfo(pResultDataItem);
        else
            return S_OK;
    }
    catch (CMemoryException* e)
    {
        e->Delete();
        MsgBoxParam(m_pComponentData->m_hwndMain, 0, 0, 0);
        return E_OUTOFMEMORY;
    }
}

STDMETHODIMP
CComponent::CompareObjects(
    LPDATAOBJECT lpDataObjectA,
    LPDATAOBJECT lpDataObjectB
    )
{

    try
    {
        ASSERT(m_pComponentData);
        //delegate to ComponentData
        return m_pComponentData->CompareObjects(lpDataObjectA, lpDataObjectB);
    }
    catch (CMemoryException* e)
    {
        e->Delete();
        MsgBoxParam(m_pComponentData->m_hwndMain, 0, 0, 0);
        return E_OUTOFMEMORY;
    }
}

///////////////////////////////////////////////////////////////////////////
/// IResultDataCompare implementation
///

// This compare is used to sort the item's in the listview.
// lUserParam - user param passed in when IResultData::Sort() was called.
// cookieA    -- first item
// cookieB    -- second item
// pnResult contains the column on entry. This function has the compared
// result in the location pointed by this parameter.
// the valid compare results are:
// -1 if cookieA  "<" cookieB
// 0  if cookieA "==" cookieB
// 1 if cookieA ">" cookieB
//
//

STDMETHODIMP
CComponent::Compare(
    LPARAM lUserParam,
    MMC_COOKIE cookieA,
    MMC_COOKIE cookieB,
    int* pnResult
    )
{


    if (!pnResult)
        return E_INVALIDARG;

    HRESULT hr;
    try
    {
        int nCol = *pnResult;
        CFolder* pFolder = (CFolder*)lUserParam;
        if (pFolder)
            hr = pFolder->Compare(cookieA, cookieB, nCol, pnResult);
        else
            hr = m_pCurFolder->Compare(cookieA, cookieB, nCol, pnResult);
    }
    catch (CMemoryException* e)
    {
        e->Delete();
        MsgBoxParam(m_pComponentData->m_hwndMain, 0, 0, 0);
        hr =  E_OUTOFMEMORY;
    }
    return hr;
}

////////////////////////////////////////////////////////////////////////////
/// Snapin's IExtendContextMenu implementation -- delegate to IComponentData
////
// Note that IComponentData also has its own IExtendContextMenu
// interface implementation. The difference is that
// IComponentData only deals with scope items while we only
// deal with result item except for cutomer view menu.
//
//
STDMETHODIMP
CComponent::AddMenuItems(
    LPDATAOBJECT lpDataObject,
    LPCONTEXTMENUCALLBACK pCallback,
    long*   pInsertionAllowed
    )
{
    HRESULT hr;
    INTERNAL_DATA tID;
    
    try
    {
        //
        // If lpDataObject is DOBJ_CUSTOMOCX then the user is viewing
        // the Action menu.
        //
        if (DOBJ_CUSTOMOCX == lpDataObject) 
        {
            ASSERT(m_pCurFolder);

            hr = m_pCurFolder->m_pScopeItem->AddMenuItems(pCallback, pInsertionAllowed);
        }

        //
        // If we have a valid cookie then the user is using the context menu
        // or the View menu
        //
        else
        {
            hr = ExtractData(lpDataObject, CDataObject::m_cfSnapinInternal,
                             reinterpret_cast<BYTE*>(&tID), sizeof(tID)
                             );

            if (SUCCEEDED(hr))
            {
                ASSERT(m_pCurFolder);

                hr = m_pCurFolder->AddMenuItems(GetActiveCookie(tID.cookie),
                                            pCallback, pInsertionAllowed
                                            );
            }
        }
    }

    catch (CMemoryException* e)
    {
        e->Delete();
        MsgBoxParam(m_pComponentData->m_hwndMain, 0, 0, 0);
        hr  = E_OUTOFMEMORY;
    }
    
    return hr;
}

STDMETHODIMP
CComponent::Command(
    long nCommandID,
    LPDATAOBJECT lpDataObject
    )
{

    INTERNAL_DATA tID;

    HRESULT hr;
    try
    {
        //
        // Menu item from the Action menu
        //
        if (DOBJ_CUSTOMOCX == lpDataObject) 
        {
            ASSERT(m_pCurFolder);

            hr = m_pCurFolder->m_pScopeItem->MenuCommand(nCommandID);
        }

        //
        // Context menu item or View menu item
        //
        else
        {
            hr = ExtractData(lpDataObject, CDataObject::m_cfSnapinInternal,
                              (PBYTE)&tID, sizeof(tID));
            if (SUCCEEDED(hr))
            {
                ASSERT(m_pCurFolder);
                
                hr = m_pCurFolder->MenuCommand(GetActiveCookie(tID.cookie), nCommandID);
            }
        }
    }

    catch (CMemoryException* e)
    {
        e->Delete();
        MsgBoxParam(m_pComponentData->m_hwndMain, 0, 0, 0);
        hr =  E_OUTOFMEMORY;
    }
    
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
// IExtendControlbar implementation
//

MMCBUTTON g_SnapinButtons[] =
{
    { 0, IDM_REFRESH, TBSTATE_ENABLED, TBSTYLE_BUTTON, (BSTR)IDS_BUTTON_REFRESH, (BSTR)IDS_TOOLTIP_REFRESH },
    { 0, 0,           TBSTATE_ENABLED, TBSTYLE_SEP,    NULL,                     NULL },
    { 1, IDM_ENABLE,  TBSTATE_ENABLED, TBSTYLE_BUTTON, (BSTR)IDS_BUTTON_ENABLE,  (BSTR)IDS_TOOLTIP_ENABLE },
    { 2, IDM_REMOVE,  TBSTATE_ENABLED, TBSTYLE_BUTTON, (BSTR)IDS_BUTTON_REMOVE,  (BSTR)IDS_TOOLTIP_REMOVE },
    { 3, IDM_DISABLE, TBSTATE_ENABLED, TBSTYLE_BUTTON, (BSTR)IDS_BUTTON_DISABLE, (BSTR)IDS_TOOLTIP_DISABLE },
};

// The Enable and Disable buttons are interchanged dependent on the state of
// the device.  The button array is used to initialize the buttons.  Then
// either the enable or disable button is inserted at the ENABLE_INDEX location.

#define SEPARATOR_INDEX         1
#define ENABLE_INDEX            2
#define DISABLE_INDEX           4
#define CBUTTONS_ARRAY          ARRAYLEN(g_SnapinButtons)
#define CBUTTONS_VISIBLE        CBUTTONS_ARRAY-1

String* g_astrButtonStrings = NULL;    // dynamic array of Strings
BOOL g_bLoadedStrings = FALSE;


STDMETHODIMP
CComponent::SetControlbar(
    LPCONTROLBAR pControlbar
    )
{
    if (pControlbar != NULL)
    {
        // Hold on to the controlbar interface.
        if (m_pControlbar)
        {
            m_pControlbar->Release();
        }

        m_pControlbar = pControlbar;
        m_pControlbar->AddRef();

        HRESULT hr = S_FALSE;

        if (!m_pToolbar)
        {
            // Create the Toolbar
            hr = m_pControlbar->Create(TOOLBAR, this,
                                       reinterpret_cast<LPUNKNOWN*>(&m_pToolbar));
            ASSERT(SUCCEEDED(hr));

            // Add the bitmap
            HBITMAP hBitmap = ::LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_TOOLBAR));
            hr = m_pToolbar->AddBitmap(4, hBitmap, 16, 16, RGB(255, 0, 255));
            ASSERT(SUCCEEDED(hr));

            if (!g_bLoadedStrings)
            {
                // load strings
                g_astrButtonStrings = new String[2*CBUTTONS_ARRAY];
                for (UINT i = 0; i < CBUTTONS_ARRAY; i++)
                {
                    if (NULL != g_SnapinButtons[i].lpButtonText &&
                        g_astrButtonStrings[i*2].LoadString(g_hInstance,
                                (UINT)((ULONG_PTR)g_SnapinButtons[i].lpButtonText)))
                    {
                        g_SnapinButtons[i].lpButtonText =
                            const_cast<BSTR>((LPCTSTR)(g_astrButtonStrings[i*2]));
                    }
                    else
                    {
                        g_SnapinButtons[i].lpButtonText = NULL;
                    }

                    if (NULL != g_SnapinButtons[i].lpTooltipText &&
                        g_astrButtonStrings[(i*2)+1].LoadString(g_hInstance,
                                (UINT)((ULONG_PTR)g_SnapinButtons[i].lpTooltipText)))
                    {
                        g_SnapinButtons[i].lpTooltipText =
                                const_cast<BSTR>((LPCTSTR)(g_astrButtonStrings[(i*2)+1]));
                    }
                    else
                    {
                        g_SnapinButtons[i].lpTooltipText = NULL;
                    }
                }

                g_bLoadedStrings = TRUE;
            }

            // Add the buttons to the toolbar
            hr = m_pToolbar->AddButtons(CBUTTONS_VISIBLE, g_SnapinButtons);
            ASSERT(SUCCEEDED(hr));
        }
    }

    return S_OK;
}

STDMETHODIMP
CComponent::ControlbarNotify(
    MMC_NOTIFY_TYPE event,
    LPARAM arg,
    LPARAM param
    )
{
    switch (event)
    {
    case MMCN_BTN_CLICK:
        // arg - Data object of the currently selected scope or result pane item.
        // param - CmdID of the button.
        switch (param)
        {
            case IDM_REFRESH:
            case IDM_ENABLE:
            case IDM_REMOVE:
            case IDM_DISABLE:
                // The arg parameter is supposed to be the data object of the
                // currently selected scope or result pane item, but it seems
                // to always passes 0xFFFFFFFF. So the ScopeItem MenuCommand is
                // used because it uses the selected cookie instead.

                // Handle toolbar button requests.
                return m_pCurFolder->m_pScopeItem->MenuCommand((LONG)param);

            default:
                break;
        }

        break;

    case MMCN_SELECT:
        // param - Data object of the item being selected.
        // For select, if the cookie has toolbar items attach the toolbar.
        // Otherwise detach the toolbar.

        HRESULT hr;

        if (LOWORD(arg))
        {
            // LOWORD(arg) being set indicated this is for the scope pane item.
            if (HIWORD(arg))
            {
                // Detach the Controlbar.
                hr = m_pControlbar->Detach(reinterpret_cast<LPUNKNOWN>(m_pToolbar));
                ASSERT(SUCCEEDED(hr));
            }
            else
            {
                // Attach the Controlbar.
                hr = m_pControlbar->Attach(TOOLBAR,
                                           reinterpret_cast<LPUNKNOWN>(m_pToolbar));
                ASSERT(SUCCEEDED(hr));
            }
        }
        break;

    default:
        break;
    }

    return S_OK;
}

//
// This function updates the toolbar buttons based on the selected cookie type.
//
HRESULT
CComponent::UpdateToolbar(
    CCookie* pCookie
    )
{
    if (!m_pToolbar)
    {
        return S_OK;
    }

    BOOL fRemoveHidden = TRUE;
    BOOL fRefreshHidden = TRUE;

    switch (pCookie->GetType())
    {
        case COOKIE_TYPE_RESULTITEM_DEVICE:
            if(m_pComponentData->m_pMachine->IsLocal() && g_HasLoadDriverNamePrivilege)
            {
                CDevice* pDevice;
                UINT index;

                pDevice = (CDevice*)pCookie->GetResultItem();

                //
                // Device can be disabled
                //
                if (pDevice->IsDisableable()) {
                
                    index = pDevice->IsStateDisabled() ? ENABLE_INDEX : DISABLE_INDEX;
    
                    // Delete the current button.
                    m_pToolbar->DeleteButton(ENABLE_INDEX);
    
                    // Insert the enable/disable button based on the device's state.
                    m_pToolbar->InsertButton(ENABLE_INDEX, &g_SnapinButtons[index]);
                }

                //
                // Device cannot be disabled
                //
                else
                {
                    // Hide both the enable and disable buttons in case the
                    // previously selected node was a device.
                    m_pToolbar->SetButtonState(IDM_ENABLE, HIDDEN, TRUE);
                    m_pToolbar->SetButtonState(IDM_DISABLE, HIDDEN, TRUE);
                }

                //
                // Only show the uninstall button if the device can be uninstalled.
                //
                if (pDevice->IsUninstallable()) {
                
                    fRemoveHidden = FALSE;
                }

                // Display refresh (Scan...) button.
                fRefreshHidden = FALSE;

                break;
            }
            else
            {
                // Must be an admin and on the local machine to remove or
                // enable/disable a device.

                // Fall through to hide the remove and enable/disable buttons.
            }


        case COOKIE_TYPE_RESULTITEM_RESOURCE_IRQ:
        case COOKIE_TYPE_RESULTITEM_RESOURCE_DMA:
        case COOKIE_TYPE_RESULTITEM_RESOURCE_IO:
        case COOKIE_TYPE_RESULTITEM_RESOURCE_MEMORY:
        case COOKIE_TYPE_RESULTITEM_COMPUTER:
        case COOKIE_TYPE_RESULTITEM_CLASS:
        case COOKIE_TYPE_RESULTITEM_RESTYPE:
            // Hide both the enable and disable buttons in case the
            // previously selected node was a device.
            m_pToolbar->SetButtonState(IDM_ENABLE, HIDDEN, TRUE);
            m_pToolbar->SetButtonState(IDM_DISABLE, HIDDEN, TRUE);

            // Display refresh (enumerate) button.
            fRefreshHidden = FALSE;

            break;

        default:
            break;
    }
    m_pToolbar->SetButtonState(IDM_REMOVE, HIDDEN, fRemoveHidden);
    m_pToolbar->SetButtonState(IDM_REFRESH, HIDDEN, fRefreshHidden);

    if (!fRemoveHidden)
    {
        // The separator appears to be automatically hidden if there are no
        // buttons following it.  It must be unhidden when additional buttons
        // are shown.
        m_pToolbar->SetButtonState(0, HIDDEN, FALSE);
    }

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//// Snapin's IExtendPropertySheet implementation
////

STDMETHODIMP
CComponent::QueryPagesFor(
    LPDATAOBJECT lpDataObject
    )
{
    HRESULT hr;

    if (!lpDataObject)
        return E_INVALIDARG;

    INTERNAL_DATA tID;
    try
    {
        hr = ExtractData(lpDataObject, CDataObject::m_cfSnapinInternal,
                         reinterpret_cast<BYTE*>(&tID), sizeof(tID)
                         );
        if (SUCCEEDED(hr))
        {
            ASSERT(m_pCurFolder);
            hr = m_pCurFolder->QueryPagesFor(GetActiveCookie(tID.cookie));
        }
    }
    catch (CMemoryException* e)
    {
        e->Delete();
        MsgBoxParam(m_pComponentData->m_hwndMain, 0, 0, 0);
        hr  = S_FALSE;
    }
    return hr;
}
STDMETHODIMP
CComponent::CreatePropertyPages(
    LPPROPERTYSHEETCALLBACK lpProvider,
    LONG_PTR handle,
    LPDATAOBJECT lpDataObject
    )
{

    HRESULT hr;

    if (!lpProvider || !lpDataObject)
        return E_INVALIDARG;

    INTERNAL_DATA tID;
    try
    {
        hr = ExtractData(lpDataObject, CDataObject::m_cfSnapinInternal,
                         reinterpret_cast<BYTE*>(&tID), sizeof(tID)
                         );
        if (SUCCEEDED(hr))
        {
            ASSERT(m_pCurFolder);
            hr = m_pCurFolder->CreatePropertyPages(GetActiveCookie(tID.cookie),
                                           lpProvider, handle
                                           );
        }
    }
    catch (CMemoryException* e)
    {
        e->Delete();
        MsgBoxParam(m_pComponentData->m_hwndMain, 0, 0, 0);
        hr =  E_OUTOFMEMORY;
    }
    return hr;
}

#ifdef PERSIST_VIEW

/////////////////////////////////////////////////////////////////////////////
// Snapin's IPersistStream implementation

STDMETHODIMP
CComponent::GetClassID(
    CLSID* pClassID
    )
{
    if(!pClassID)
        return E_POINTER;
    *pClassID = m_pComponentData->GetCoClassID();
    return S_OK;
}
inline
STDMETHODIMP
CComponent::IsDirty()
{
    return m_Dirty ? S_OK : S_FALSE;
}

STDMETHODIMP
CComponent::GetSizeMax(
    ULARGE_INTEGER* pcbSize
    )
{

    if (!pcbSize)
        return E_INVALIDARG;

    //         total folders        folder signature
    int Size =  sizeof(int) + m_listFolder.GetCount() * sizeof(FOLDER_SIGNATURE)
                + sizeof(CLSID);
    CFolder* pFolder;
    POSITION pos = m_listFolder.GetHeadPosition();
    while (NULL != pos)
    {
        pFolder = m_listFolder.GetNext(pos);
        ASSERT(pFolder);
        Size += pFolder->GetPersistDataSize();
    }
    ULISet32(*pcbSize, Size);
    return S_OK;
}


// save data format

STDMETHODIMP
CComponent::Save(
    IStream* pStm,
    BOOL fClearDirty
    )
{

    HRESULT hr = S_OK;

    SafeInterfacePtr<IStream> StmPtr(pStm);

    int Count, Index;
    POSITION pos;
    CLSID clsid;
    try
    {
        // write out CLSID
        hr = pStm->Write(&CLSID_DEVMGR, sizeof(CLSID), NULL);
        if (SUCCEEDED(hr))
        {
            Count = m_listFolder.GetCount();
            CFolder* pFolder;
            // write out folder count
            hr = pStm->Write(&Count, sizeof(Count), NULL);
            if (SUCCEEDED(hr) && Count)
            {
                pos = m_listFolder.GetHeadPosition();
                while (NULL != pos)
                {
                    pFolder = m_listFolder.GetNext(pos);
                    // write folder signature
                    FOLDER_SIGNATURE Signature = pFolder->GetSignature();
                    hr = pStm->Write(&Signature, sizeof(Signature), NULL);
                    if (SUCCEEDED(hr))
                        hr = SaveFolderPersistData(pFolder, pStm, fClearDirty);
                    if (FAILED(hr))
                        break;
                }
            }
        }
    }
    catch (CMemoryException* e)
    {
        e->Delete();
        MsgBoxParam(m_pComponentData->m_hwndMain, 0, 0, 0);
        hr =  E_OUTOFMEMORY;
    }
    if (fClearDirty)
        m_Dirty = FALSE;
    return hr;
}

STDMETHODIMP
CComponent::Load(
    IStream* pStm
    )
{

    HRESULT hr = S_OK;

    CLSID clsid;
    SafeInterfacePtr<IStream> StmPtr(pStm);

    ASSERT(pStm);

    // read the clsid
    try
    {
        hr = pStm->Read(&clsid, sizeof(clsid), NULL);
        if (SUCCEEDED(hr) && clsid ==  CLSID_DEVMGR)
        {
            CFolder* pFolder;
            int FolderCount;

            // folder list must be create before Load.
            // DO NOT rely on that IComponent::Initialize comes before IStream::Load
            //
            ASSERT(m_listFolder.GetCount());

            // load folder count
            hr = pStm->Read(&FolderCount, sizeof(FolderCount), NULL);
            if (SUCCEEDED(hr))
            {
                ASSERT(m_listFolder.GetCount() == FolderCount);
                // get folder signature
                // go through every folder
                for (int i = 0; i < FolderCount; i++)
                {
                    FOLDER_SIGNATURE Signature;
                    hr = pStm->Read(&Signature, sizeof(Signature), NULL);
                    if (SUCCEEDED(hr))
                    {
                        POSITION pos;
                        pos = m_listFolder.GetHeadPosition();
                        while (NULL != pos)
                        {
                            pFolder = m_listFolder.GetNext(pos);
                            if (pFolder->GetSignature() == Signature)
                            {
                                hr = LoadFolderPersistData(pFolder, pStm);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    catch (CMemoryException* e)
    {
        e->Delete();
        MsgBoxParam(m_pComponentData->m_hwndMain, 0, 0, 0);
        hr = E_OUTOFMEMORY;
    }
    m_Dirty = FALSE;
    return hr;
}
HRESULT
CComponent::SaveFolderPersistData(
    CFolder* pFolder,
    IStream* pStm,
    BOOL fClearDirty
    )
{
    HRESULT hr = S_OK;
    PBYTE Buffer;
    int Size;
    SafeInterfacePtr<IStream> StmPtr(pStm);

    try
    {
        Size = pFolder->GetPersistDataSize();
        // always write the length even though it can be 0.
        hr = pStm->Write(&Size, sizeof(Size), NULL);
        if (SUCCEEDED(hr) && Size)
        {
            BufferPtr<BYTE> Buffer(Size);
            pFolder->GetPersistData(Buffer, Size);
            hr = pStm->Write(Buffer, Size, NULL);
        }
    }
    catch (CMemoryException* e)
    {
        e->Delete();
        MsgBoxParam(m_pComponentData->m_hwndMain, 0, 0, 0);
        hr = E_OUTOFMEMORY;
    }
    return hr;

}

HRESULT
CComponent::LoadFolderPersistData(
    CFolder* pFolder,
    IStream* pStm
    )
{
    HRESULT hr = S_OK;

    PBYTE Buffer;

    SafeInterfacePtr<IStream> StmPtr(pStm);

    int Size = 0;
    hr = pStm->Read(&Size, sizeof(Size), NULL);
    if (SUCCEEDED(hr) && Size)
    {
        BufferPtr<BYTE> Buffer(Size);
        hr = pStm->Read(Buffer, Size, NULL);
        if (SUCCEEDED(hr))
        {
            hr = pFolder->SetPersistData(Buffer, Size);
        }
    }
    return hr;
}

#endif  // ifdef PERSIST_VIEW

//
// This function attaches the given folder the the machine created
// by the component data. The machine notifies every attached folder
// when there are state changes in the machine.
//
// INPUT:
//      pFolder     -- the folder to be attached
//      ppMachind   -- to receive a pointer to the machine
// OUTPUT:
//      TRUE if the folder is attached successfully.
//      FALSE if the attachment failed.
//
//
BOOL
CComponent::AttachFolderToMachine(
    CFolder* pFolder,
    CMachine** ppMachine
    )
{
    if (!pFolder)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    // Initialize the machine.
    if (m_pComponentData->InitializeMachine())
    {
        *ppMachine = m_pComponentData->m_pMachine;
        (*ppMachine)->AttachFolder(pFolder);
        return TRUE;
    }
    return FALSE;
}

//
// This function detaches all the component's folders from the machine
//
void
CComponent::DetachAllFoldersFromMachine()
{
    if (m_pComponentData->m_pMachine)
    {
        CMachine*       pMachine = m_pComponentData->m_pMachine;

        CFolder* pFolder;
        POSITION pos = m_listFolder.GetHeadPosition();
        while (NULL != pos)
        {
            pFolder = m_listFolder.GetNext(pos);
            pMachine->DetachFolder(pFolder);
        }
    }
}

HRESULT
CComponent::CreateFolderList(
    CCookie* pCookie
    )
{
    CCookie* pCookieChild;
    CScopeItem* pScopeItem;
    CFolder* pFolder;

    ASSERT(pCookie);

    HRESULT hr = S_OK;
    do
    {
        pScopeItem = pCookie->GetScopeItem();
        ASSERT(pScopeItem);
        pFolder =  pScopeItem->CreateFolder(this);
        if (pFolder)
        {
            m_listFolder.AddTail(pFolder);
            pFolder->AddRef();
            pCookieChild = pCookie->GetChild();
            if (pCookieChild)
                hr = CreateFolderList(pCookieChild);
            pCookie = pCookie->GetSibling();
        }
        else
        {
           hr = E_OUTOFMEMORY;
        }
    } while (SUCCEEDED(hr) && pCookie);
    return hr;
}

BOOL
CComponent::DestroyFolderList(
    MMC_COOKIE cookie
    )
{
    if (!m_listFolder.IsEmpty())
    {
        POSITION pos = m_listFolder.GetHeadPosition();
        while (NULL != pos)
        {
            CFolder* pFolder = m_listFolder.GetNext(pos);
            //
            // DONOT delete it!!!!!!!
            //
            pFolder->Release();
        }
        m_listFolder.RemoveAll();
    }
    return TRUE;
}

CFolder*
CComponent::FindFolder(
    MMC_COOKIE cookie
    )
{
    CCookie* pCookie = GetActiveCookie(cookie);
    CFolder* pFolder;
    POSITION pos = m_listFolder.GetHeadPosition();
    while (NULL != pos)
    {
        pFolder = m_listFolder.GetNext(pos);
        if (pCookie->GetScopeItem() == pFolder->m_pScopeItem)
            return pFolder;
    }
    return NULL;
}


int
CComponent::MessageBox(
    LPCTSTR Msg,
    LPCTSTR Caption,
    DWORD Flags
    )
{
    int Result;
    ASSERT(m_pConsole);
    if (SUCCEEDED(m_pConsole->MessageBox(Msg, Caption, Flags, &Result)))
        return Result;
    else
        return IDCANCEL;
}
