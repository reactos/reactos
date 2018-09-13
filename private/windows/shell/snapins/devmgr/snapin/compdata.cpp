/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    compdata.cpp

Abstract:

    This module implemets CComponentData class

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "devmgr.h"
#include "factory.h"
#include "genpage.h"

const WCHAR* const DM_COMPDATA_SIGNATURE = L"Device Manager";

#define USE_DM_COMPDATA_PERSISTINFO 1
CComponentData::CComponentData()
{
    m_pScope = NULL;
    m_pConsole = NULL;
    m_pCookieRoot = NULL;
    m_pScopeItemRoot = NULL;
    // static scope item default to device manager
    m_ctRoot = COOKIE_TYPE_SCOPEITEM_DEVMGR;
    m_hwndMain = NULL;
    m_pMachine = NULL;
    m_IsDirty = FALSE;
    // increment object count(used by CanUnloadNow)
    ::InterlockedIncrement(&CClassFactory::s_Objects);
    m_Ref = 1;
}
CComponentData::~CComponentData()
{

    // All QIed interfaces should be released during
    // Destroy method
    ASSERT(NULL == m_pScope);
    ASSERT(NULL == m_pConsole);
    ASSERT(NULL == m_pCookieRoot);
    // decrement object count(used by CanUnloadNow)
    ::InterlockedDecrement(&CClassFactory::s_Objects);

}


//
// IUnknown interface
//
ULONG
CComponentData::AddRef()
{
    ::InterlockedIncrement((LONG*)&m_Ref);
    return m_Ref;
}
ULONG
CComponentData::Release()
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
CComponentData::QueryInterface(
    REFIID  riid,
    void**  ppv
    )
{
    if (!ppv)
    return E_INVALIDARG;
    HRESULT hr = S_OK;


    if (IsEqualIID(riid, IID_IUnknown))
    {
    *ppv = (IUnknown*)(IComponentData*)this;
    }
    else if (IsEqualIID(riid, IID_IComponentData))
    {
    *ppv = (IComponentData*)this;
    }
    else if (IsEqualIID(riid, IID_IExtendContextMenu))
    {
    *ppv = (IExtendContextMenu*)this;
    }
    else if (IsEqualIID(riid, IID_IExtendPropertySheet))
    {
    *ppv = (IExtendPropertySheet*)this;
    }
    else if (IsEqualIID(riid, IID_IPersistStream))
    {
    *ppv = (IPersistStream*)this;
    }
    else if (IsEqualIID(riid, IID_ISnapinHelp))
    {
    *ppv = (ISnapinHelp*)this;
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
/////////////////////////////////////////////////////////////////////////////
//  IComponentData implementation
///

STDMETHODIMP
CComponentData::Initialize(
    LPUNKNOWN pUnknown
    )
{
    if (!pUnknown)
    return E_INVALIDARG;

    HRESULT hr;
    try
    {
    // This function should be called only once.
    ASSERT(NULL == m_pScope);

    // Get the IConsoleNameSpace interface
    hr = pUnknown->QueryInterface(IID_IConsoleNameSpace, (void**)&m_pScope);
    if (SUCCEEDED(hr))
    {
        hr = pUnknown->QueryInterface(IID_IConsole, (void**)&m_pConsole);
        if (SUCCEEDED(hr))
        {
        //retreive the console main window. It will be used
        // as the parent window of property sheets and
        // parent handle for setupapi calls
        m_pConsole->GetMainWindow(&m_hwndMain);
        LoadScopeIconsForScopePane();
        }
        else
        {
        // unable to the IConsole Interface
        m_pScope->Release();
        }
    }
    }
    catch (CMemoryException* e)
    {
    e->Delete();
    MsgBoxParam(m_hwndMain, 0, 0, 0);
    hr = E_OUTOFMEMORY;
    }
    return hr;
}

// This function creates a new CComponent
// A Component will be created when a new "window" is being created.
//
STDMETHODIMP
CComponentData::CreateComponent(
    LPCOMPONENT* ppComponent
    )
{
    HRESULT hr;

    if (!ppComponent)
    return E_INVALIDARG;

    try
    {
    CComponent* pComponent = new CComponent(this);
    // return the IComponent interface
    hr = pComponent->QueryInterface(IID_IComponent, (void**)ppComponent);
    pComponent->Release();
    if (SUCCEEDED(hr))
    {
        hr = CreateScopeItems();

        if (SUCCEEDED(hr))
        {
        hr = pComponent->CreateFolderList(m_pCookieRoot);
        }
        else
        {
        pComponent->Release();
        *ppComponent = NULL;
        }
    }
    }
    catch (CMemoryException* e)
    {
    e->Delete();
    MsgBoxParam(m_hwndMain, 0, 0, 0);
    hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDMETHODIMP
CComponentData::Notify(
    LPDATAOBJECT lpDataObject,
    MMC_NOTIFY_TYPE event,
    LPARAM    arg,
    LPARAM    param
    )
{
    HRESULT hr;
    try
    {
    // On MMCN_PROPERTY_CHANGE event, lpDataObject is invalid
    // Donot touch it.
    if (MMCN_PROPERTY_CHANGE == event)
    {
        PPROPERTY_CHANGE_INFO pPCI = (PPROPERTY_CHANGE_INFO) param;
        if (pPCI && PCT_STARTUP_INFODATA == pPCI->Type)
        {
        PSTARTUP_INFODATA pSI = (PSTARTUP_INFODATA)&pPCI->InfoData;
        ASSERT(pSI->Size == sizeof(STARTUP_INFODATA));
        if (pSI->MachineName[0] != _T('\0'))
            m_strMachineName = pSI->MachineName;
        m_ctRoot = pSI->ct;
        SetDirty();
        }
        return S_OK;
    }
    else if (MMCN_EXPAND == event)
    {
        return OnExpand(lpDataObject, arg, param);
    }
    else if (MMCN_REMOVE_CHILDREN == event)
    {
        //
        // This is basically a hack!!!!
        // When the target computer is switched in Computer Management
        // snapin(we are an extention to it), we basically get
        // a MMCN_REMOVE_CHILDREN followed by MMCN_EXPAND.
        // The right thing for MMC to do is to create a new IComponent
        // for each new machine so that each IComponent can maintain
        // its own states(thus, its own folders).
        // Well, it is not a perfect world and we are forced to use
        // the old IComponent. So here we notify each scope node
        // which in turns will notify all the CFolders.
        //
        // After reset, each folder does not attach to any CMachine object
        // (thus, its m_pMachine will be NULL). Each folder will attach
        // to the new machine object when its OnShow method is called
        // the very "first" time.
        if (!IsPrimarySnapin() && m_pScopeItemRoot)
        {
        ResetScopeItem(m_pScopeItemRoot);
        }
        return S_OK;
    }

    ASSERT(m_pScope);
    INTERNAL_DATA tID;
    hr = ExtractData(lpDataObject, CDataObject::m_cfSnapinInternal,
             (PBYTE)&tID, sizeof(tID));


    if (SUCCEEDED(hr))
    {
        switch (event) {
        case MMCN_DELETE:
            hr = OnDelete(tID.cookie, arg, param);
            break;
        case MMCN_RENAME:
            hr = OnRename(tID.cookie, arg, param);
            break;
        case MMCN_CONTEXTMENU:
            hr = OnContextMenu(tID.cookie, arg, param);
            break;
        case MMCN_BTN_CLICK:
            hr = OnBtnClick(tID.cookie, arg, param);
            break;
        default:
            hr = S_OK;
            break;
        }
    }
    }
    catch(CMemoryException* e)
    {
    e->Delete();
    hr = E_OUTOFMEMORY;
    }

    return hr;
}


STDMETHODIMP
CComponentData::GetDisplayInfo(
    SCOPEDATAITEM* pScopeDataItem
    )
{

    if (!pScopeDataItem)
    return E_INVALIDARG;

    try
    {
    // IComponentData::GetDisplayInfo only deals with scope pane items.
    // Snapin's IComponent::GetDisplayInfo will deal with result pane items
    CCookie* pCookie = (CCookie*) pScopeDataItem->lParam;
    ASSERT(pCookie);
    return pCookie->GetScopeItem()->GetDisplayInfo(pScopeDataItem);
    }
    catch (CMemoryException* e)
    {
    e->Delete();
    MsgBoxParam(m_hwndMain, 0, 0, 0);
    return E_OUTOFMEMORY;
    }

}

STDMETHODIMP
CComponentData::Destroy()
{


    if (m_pCookieRoot) {
    delete m_pCookieRoot;
    m_pCookieRoot = NULL;
    }
    if (m_pScopeItemRoot)
    delete m_pScopeItemRoot;

    if (m_pScope) {
    m_pScope->Release();
    m_pScope = NULL;
    }
    if (m_pConsole)
    {
    m_pConsole->Release();
    m_pConsole = NULL;
    }
    return S_OK;
}

STDMETHODIMP
CComponentData::QueryDataObject(
    MMC_COOKIE cookie,
    DATA_OBJECT_TYPES type,
    LPDATAOBJECT* ppDataObject
    )
{

    CDataObject* pDataObject;
    COOKIE_TYPE  ct;
    CCookie* pCookie;

    try
    {
    pCookie = GetActiveCookie(cookie);
    if (NULL == pCookie)
        ct = m_ctRoot;
    else
        ct = pCookie->GetType();

    pDataObject = new CDataObject;
    pDataObject->Initialize(type, ct, pCookie, m_strMachineName);
    pDataObject->AddRef();
    *ppDataObject = pDataObject;
    }
    catch (CMemoryException* e)
    {
    e->Delete();
    MsgBoxParam(m_hwndMain, 0, 0, 0);
    return E_OUTOFMEMORY;
    }
    return S_OK;

}

STDMETHODIMP
CComponentData::CompareObjects(
    LPDATAOBJECT lpDataObjectA,
    LPDATAOBJECT lpDataObjectB
    )
{
    HRESULT hr;
    try
    {
    INTERNAL_DATA tID_A, tID_B;
    hr = ExtractData(lpDataObjectA, CDataObject::m_cfSnapinInternal,
             (PBYTE)&tID_A, sizeof(tID_A));
    if (SUCCEEDED(hr))
    {
        hr = ExtractData(lpDataObjectB, CDataObject::m_cfSnapinInternal,
                 (PBYTE)&tID_B, sizeof(tID_B));
        if (SUCCEEDED(hr))
        {
        hr = (tID_A.ct == tID_B.ct && tID_A.cookie == tID_B.cookie &&
               tID_A.dot == tID_B.dot) ? S_OK : S_FALSE;
        }
    }
    }
    catch(CMemoryException* e)
    {
    e->Delete();
    MsgBoxParam(m_hwndMain, 0, 0, 0);
    hr = E_OUTOFMEMORY;
    }
    return hr;
}



///////////////////////////////////////////////////////////////////
//// IExtendPropertySheet implementation
////
STDMETHODIMP
CComponentData::QueryPagesFor(
    LPDATAOBJECT lpDataObject
    )
{
    HRESULT hr;
    if (!lpDataObject)
    return E_INVALIDARG;

    try
    {
    INTERNAL_DATA tID;
    hr = ExtractData(lpDataObject, CDataObject::m_cfSnapinInternal,
             (PBYTE)&tID, sizeof(tID));
    if (SUCCEEDED(hr))
    {
        CScopeItem* pScopeItem;
        pScopeItem = FindScopeItem(tID.cookie);
        if (CCT_SNAPIN_MANAGER == tID.dot && COOKIE_TYPE_SCOPEITEM_DEVMGR == tID.ct)
        {
        hr = S_OK;
        }
        else if (pScopeItem)
        hr = pScopeItem->QueryPagesFor();
        else
        hr = S_FALSE;
    }
    }
    catch (CMemoryException* e)
    {
    e->Delete();
    MsgBoxParam(m_hwndMain, 0, 0, 0);
    hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDMETHODIMP
CComponentData::CreatePropertyPages(
    LPPROPERTYSHEETCALLBACK lpProvider,
    LONG_PTR handle,
    LPDATAOBJECT lpDataObject
    )
{

    if (!lpProvider || !lpDataObject)
    return E_INVALIDARG;

    HRESULT hr;

    try
    {
    INTERNAL_DATA tID;
    hr = ExtractData(lpDataObject, CDataObject::m_cfSnapinInternal,
             reinterpret_cast<BYTE*>(&tID), sizeof(tID)
             );
    if (SUCCEEDED(hr))
    {
        CScopeItem* pScopeItem = FindScopeItem(tID.cookie);
        if (CCT_SNAPIN_MANAGER == tID.dot && COOKIE_TYPE_SCOPEITEM_DEVMGR == tID.ct)
        {
        hr = DoStartupProperties(lpProvider, handle, lpDataObject);
        }
        else if (pScopeItem)
        hr = pScopeItem->CreatePropertyPages(lpProvider, handle);
        else
        hr = S_OK;
    }
    }
    catch(CMemoryException* e)
    {
    e->Delete();
    MsgBoxParam(m_hwndMain, 0, 0, 0);
    hr = E_OUTOFMEMORY;
    }
    return hr;
}


////////////////////////////////////////////////////////////
//// IExtendContextMenu implemantation
////
STDMETHODIMP
CComponentData::AddMenuItems(
    LPDATAOBJECT lpDataObject,
    LPCONTEXTMENUCALLBACK pCallbackUnknown,
    long *pInsertionAllowed
    )
{


    if (!lpDataObject || !pCallbackUnknown || !pInsertionAllowed)
    return E_INVALIDARG;

    HRESULT hr;

    try
    {
    INTERNAL_DATA tID;
    hr = ExtractData(lpDataObject, CDataObject::m_cfSnapinInternal,
             reinterpret_cast<BYTE*>(&tID), sizeof(tID)
             );
    if (SUCCEEDED(hr))
            hr = FindScopeItem(tID.cookie)->AddMenuItems(pCallbackUnknown,
                                                         pInsertionAllowed
                                                         );
    }
    catch (CMemoryException* e)
    {
    e->Delete();
    MsgBoxParam(m_hwndMain, 0, 0, 0);
    hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDMETHODIMP
CComponentData::Command(
    long nCommandID,
    LPDATAOBJECT lpDataObject
    )
{
    if (!lpDataObject)
    return E_INVALIDARG;

    HRESULT hr;

    try
    {
    INTERNAL_DATA tID;
    hr = ExtractData(lpDataObject, CDataObject::m_cfSnapinInternal,
             reinterpret_cast<BYTE*>(&tID), sizeof(tID)
             );
    if (SUCCEEDED(hr))
            hr = FindScopeItem(tID.cookie)->MenuCommand(nCommandID);
    }
    catch (CMemoryException* e)
    {
    e->Delete();
    MsgBoxParam(m_hwndMain, 0, 0, 0);
    hr = E_OUTOFMEMORY;
    }
    return hr;


}

HRESULT
CComponentData::CreateCookieSubtree(
    CScopeItem* pScopeItem,
    CCookie* pCookieParent
    )
{

    ASSERT(pScopeItem);

    CScopeItem* pChild;
    CCookie* pCookieSibling;
    pCookieSibling = NULL;
    int Index = 0;

    while (pScopeItem->EnumerateChildren(Index, &pChild))
    {
    CCookie* pCookie;
    pCookie =  new CCookie(pChild->GetType());
    pCookie->SetScopeItem(pChild);
    if (!pCookieSibling)
        pCookieParent->SetChild(pCookie);
    else
        pCookieSibling->SetSibling(pCookie);
    pCookie->SetParent(pCookieParent);
    if (pChild->GetChildCount())
        CreateCookieSubtree(pChild, pCookie);
    pCookieSibling = pCookie;
    Index++;
    }
    return S_OK;
}
////////////////////////////////////////////////////////////
/// IPersistStream implementation
///
STDMETHODIMP
CComponentData::GetClassID(
    CLSID* pClassID
    )
{
    if(!pClassID)
    return E_INVALIDARG;
    *pClassID = GetCoClassID();
    return S_OK;
}
STDMETHODIMP
CComponentData::IsDirty()
{
    return m_IsDirty ? S_OK : S_FALSE;
}



STDMETHODIMP
CComponentData::Load(
    IStream* pStm
    )
{
    HRESULT hr;
    SafeInterfacePtr<IStream> StmPtr(pStm);

    //
    // Fix up the MachineName that we got from the command line if there was one.
    // We need to prepend "\\" to the MachineName if it does not start with two
    // backslashes, and then we will verify the machine name by calling CM_Connect_Machine
    // to verify that this user has access to that machine.  If they do not then we
    // will set the MachineName to NULL.
    //
    if (!g_strStartupMachineName.IsEmpty())
    {
        if (_T('\\') != g_strStartupMachineName[0])
        {
            g_strStartupMachineName = TEXT("\\\\") + g_strStartupMachineName;
        }

        if (VerifyMachineName(g_strStartupMachineName) != CR_SUCCESS)
        {
            g_strStartupMachineName.Empty();
        }
    }

#ifdef USE_DM_COMPDATA_PERSISTINFO
    COMPDATA_PERSISTINFO Info;
    ULONG BytesRead;

    ASSERT(pStm);
    
    // read the persist data and verify that we have the right data
    hr = pStm->Read(&Info, sizeof(Info), &BytesRead);
        
    if (SUCCEEDED(hr) && BytesRead >=  sizeof(Info) &&
        Info.Size >= sizeof(Info) &&
        !wcscmp(Info.Signature, DM_COMPDATA_SIGNATURE))
    {
        try
        {
            m_ctRoot = Info.RootCookie;
            m_strMachineName.Empty();
            if (UNICODE_NULL != Info.ComputerFullName[0])
            {
#ifdef UNICODE
                m_strMachineName = Info.ComputerFullName;
#else
                CHAR ComputerNameA[MAX_COMPUTERNAME_LENGTH + 3];
                WideCharToMultiByte(CP_ACP, 0, Info.ComputerFullName, -1,
                    ComputerNameA, sizeof(ComputerNameA) / sizeof(CHAR), NULL, ULL);
                m_strMachineName = ComputerNameA;
#endif
            }
    
            if (COOKIE_TYPE_SCOPEITEM_DEVMGR == m_ctRoot)
            {
                // parameters from command line has the priority
                if (!g_strStartupMachineName.IsEmpty())
                {
                    m_strMachineName = g_strStartupMachineName;
                }
                
                m_strStartupDeviceId = g_strStartupDeviceId;
                m_strStartupCommand = g_strStartupCommand;
            }
    
            hr = CreateScopeItems();
            if (SUCCEEDED(hr))
            {
                if (!m_pMachine)
                {
                    if (!g_MachineList.CreateMachine(m_hwndMain, m_strMachineName, &m_pMachine))
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                }
            }
        }

        catch(CMemoryException* e)
        {
            e->Delete();
            MsgBoxParam(m_hwndMain, 0, 0, 0);
            hr = E_OUTOFMEMORY;
        }
    }

    m_IsDirty = FALSE;

    return hr;
#else
    
    ASSERT(pStm);
    
    // read the root cookie type
    hr = pStm->Read((PVOID)&m_ctRoot, sizeof(m_ctRoot), NULL);
    if (SUCCEEDED(hr))
    {
        try
        {
            int len;
        
            // read the machine name length
            hr = pStm->Read(&len, sizeof(len), NULL);
            if (SUCCEEDED(hr))
            {
                m_strMachineName.Empty();
                if (len)
                {
                    TCHAR szName[MAX_COMPUTERNAME_LENGTH + 1];
                    ASSERT(len <= sizeof(szName));
            
                    // read the machine name
                    hr = pStm->Read(szName, len, NULL);
                    
                    if (SUCCEEDED(hr))
                        m_strMachineName = szName;
                }

                if (COOKIE_TYPE_SCOPEITEM_DEVMGR == m_ctRoot)
                {
                    // parameters from command line has the priority
                    if (!g_strStartupMachineName.IsEmpty())
                    {
                        m_strMachineName = g_strStartupMachineName;
                    }
                    
                    m_strStartupDeviceId = g_strStartupDeviceId;
                    m_strStartupCommand = g_strStartupCommand;
                }
            }
        
            hr = CreateScopeItems();
            if (SUCCEEDED(hr))
            {
                if (!m_pMachine)
                {
                    if (!g_MachineList.CreateMachine(m_hwndMain, m_strMachineName, &m_pMachine))
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                }
            }
        }
    
        catch(CMemoryException* e)
        {
            e->Delete();
            MsgBoxParam(m_hwndMain, 0, 0, 0);
            hr = E_OUTOFMEMORY;
        }
    }
    
    m_IsDirty = FALSE;

    return hr;
#endif
}

STDMETHODIMP
CComponentData::Save(
    IStream* pStm,
    BOOL fClearDirty
    )
{
    SafeInterfacePtr<IStream> StmPtr(pStm);

    HRESULT hr;

#ifdef USE_DM_COMPDATA_PERSISTINFO
    try
    {
        COMPDATA_PERSISTINFO Info;
        Info.Size = sizeof(Info);
        Info.RootCookie = m_ctRoot;
        wcscpy(Info.Signature, DM_COMPDATA_SIGNATURE);
    
        // assuming it is on local machine. The machine name is saved
        // in UNICODE
        Info.ComputerFullName[0] = UNICODE_NULL;
        if (m_strMachineName.GetLength())
#ifdef UNICODE
            wcscpy(Info.ComputerFullName, m_strMachineName);
#else
            MultiByteToWideChar((CP_ACP, 0, chBuffer, -1, Info.ComputerFullName,
                     sizeof(Info.ComputerFullName) / sizeof(WCHAR));
#endif
        hr = pStm->Write(&Info, sizeof(Info), NULL);
    }

    catch (CMemoryException* e)
    {
        e->Delete();
        MsgBoxParam(m_hwndMain, 0, 0, 0);
        hr = E_OUTOFMEMORY;
    }

    if (fClearDirty)
    {
        m_IsDirty = FALSE;
    }
    
    return hr;
#else

    try
    {
        // (1) root cookie type
        // (2) machine name
        hr = pStm->Write(&m_ctRoot, sizeof(m_ctRoot), NULL);
        if (SUCCEEDED(hr))
        {
            int Len = m_strMachineName.GetLength() * sizeof(TCHAR);
            hr = pStm->Write(&Len, sizeof(Len), NULL);
            if (SUCCEEDED(hr) && Len)
            {
                hr = pStm->Write((LPVOID)(LPCTSTR)m_strMachineName, Len, NULL);
            }
        }
    }

    catch (CMemoryException* e)
    {
        e->Delete();
        MsgBoxParam(m_hwndMain, 0, 0, 0);
        hr = E_OUTOFMEMORY;
    }

    if (fClearDirty)
    {
        m_IsDirty = FALSE;
    }
    
    return hr;
#endif

}

STDMETHODIMP
CComponentData::GetSizeMax(
    ULARGE_INTEGER* pcbSize
    )
{
    if (!pcbSize)
    return E_INVALIDARG;
    int len;
    len = sizeof(m_ctRoot) + sizeof(len) +  (m_strMachineName.GetLength() + 1) * sizeof(TCHAR);
    ULISet32(*pcbSize, len);
    return S_OK;
}

//
// Method to support html help.
//
//
STDMETHODIMP
CComponentData::GetHelpTopic(
    LPOLESTR* lpCompileHelpFile
    )
{
    if (!lpCompileHelpFile)
    return E_INVALIDARG;
    *lpCompileHelpFile = NULL;
    UINT Size;
    TCHAR HelpFile[MAX_PATH];
    Size = GetWindowsDirectory(HelpFile, ARRAYLEN(HelpFile));
    if (Size && Size < ARRAYLEN(HelpFile))
    {
    lstrcat(HelpFile, DEVMGR_HTML_HELP_FILE_NAME);
    *lpCompileHelpFile = AllocOleTaskString(HelpFile);
    }
    return S_OK;
}

CScopeItem*
CComponentData::FindScopeItem(
    MMC_COOKIE cookie
    )
{
    CCookie* pCookie = GetActiveCookie(cookie);
    if (pCookie)
    return pCookie->GetScopeItem();
    return NULL;
}

//
// This function loads icons for the scope items
//
HRESULT
CComponentData::LoadScopeIconsForScopePane()
{


    ASSERT(m_pScope);
    ASSERT(m_pConsole);

    LPIMAGELIST lpScopeImage;
    HRESULT hr;
    hr = m_pConsole->QueryScopeImageList(&lpScopeImage);
    if (SUCCEEDED(hr))
    {
    HICON hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_DEVMGR));
    if (hIcon)
    {
        hr = lpScopeImage->ImageListSetIcon((PLONG_PTR)hIcon, IMAGE_INDEX_DEVMGR);
            DestroyIcon(hIcon);
    }
    hr = lpScopeImage->Release();
    }
    return hr;

}

//
// This function create the startup wizard property sheet
//
// INPUT:
//  lpProvider -- Interface for us to add pages
//  handle     -- notify console handle
//  lpDataObject -- the data object
//
// OUTPUT:
//   standard OLE HRESULT

HRESULT
CComponentData::DoStartupProperties(
    LPPROPERTYSHEETCALLBACK lpProvider,
    LONG_PTR handle,
    LPDATAOBJECT lpDataObject
    )
{

    CGeneralPage* pGenPage;
    HPROPSHEETPAGE hPage;
    pGenPage = new CGeneralPage();
    hPage = pGenPage->Create(handle);
    if (hPage)
    {
    lpProvider->AddPage(hPage);
    // if no console handle is provided, we have to use
    // our call back function
    if(!handle)
        pGenPage->SetOutputBuffer(&m_strMachineName, &m_ctRoot);
    return S_OK;
    }
    else
    {
    throw &g_MemoryException;
    }
    return E_OUTOFMEMORY;
}

//
// This function creates all the necessary classes represent
// our scope items
//
HRESULT
CComponentData::CreateScopeItems()
{

    // all classes are linked by cookie with m_pCookieRoot
    // points to the "root" scope item
    if (!m_pScopeItemRoot)
    {

    switch (m_ctRoot)
    {
        case COOKIE_TYPE_SCOPEITEM_DEVMGR:
                m_pScopeItemRoot = new CScopeItem(COOKIE_TYPE_SCOPEITEM_DEVMGR,
                                                  IMAGE_INDEX_DEVMGR,
                                                  OPEN_IMAGE_INDEX_DEVMGR,
                                                  IDS_NAME_DEVMGR,
                                                  IDS_DESC_DEVMGR,
                                                  IDS_DISPLAYNAME_SCOPE_DEVMGR);
        break;
        default:
        ASSERT(FALSE);
        break;
    }
    if (m_pScopeItemRoot->Create())
    {
        // bind scope items and cookies together.
        // Cookies know its scopeitem.
        // Scopeitems do not know cookies.
        m_pCookieRoot = new CCookie(m_ctRoot);
        if (m_pCookieRoot)
        {
        ASSERT(m_pScopeItemRoot->GetType() == m_ctRoot);
        m_pCookieRoot->SetScopeItem(m_pScopeItemRoot);
        CreateCookieSubtree(m_pScopeItemRoot, m_pCookieRoot);
        }
    }
    }
    return S_OK;
}


//
// This function resets the given scopeitem.
//
HRESULT
CComponentData::ResetScopeItem(
    CScopeItem* pScopeItem
    )
{
    HRESULT hr = S_OK;
    if (pScopeItem)
    {
    CScopeItem* pChild;
    int Index;
    Index = 0;
    while (SUCCEEDED(hr) && pScopeItem->EnumerateChildren(Index, &pChild))
    {
        hr = ResetScopeItem(pChild);
        Index++;
    }
    if (SUCCEEDED(hr))
        return pScopeItem->Reset();
    }
    return hr;

}
