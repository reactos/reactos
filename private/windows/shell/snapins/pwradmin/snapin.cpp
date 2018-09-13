#include "main.h"
#include <gpedit.h>


//
// Extracts the coclass guid format from the data object
//
template <class TYPE>
TYPE* Extract(LPDATAOBJECT lpDataObject, unsigned int cf)
{

    TYPE* p = NULL;

    STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
    FORMATETC formatetc = { (CLIPFORMAT)cf, NULL,
                            DVASPECT_CONTENT, -1, TYMED_HGLOBAL
                          };

    if (!lpDataObject)
        return NULL;

    // Allocate memory for the stream
    stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, sizeof(TYPE));

    // Get the data from the data object
    do
    {
        if (stgmedium.hGlobal == NULL)
            break;

        if (FAILED(lpDataObject->GetDataHere(&formatetc, &stgmedium)))
            break;

        p = reinterpret_cast<TYPE*>(stgmedium.hGlobal);

        if (p == NULL)
            break;

    } while (FALSE);

    return p;
}

// Data object extraction helpers
CLSID* ExtractClassID(LPDATAOBJECT lpDataObject)
{
    return Extract<CLSID>(lpDataObject, CDataObject::m_cfCoClass);
}

GUID* ExtractNodeType(LPDATAOBJECT lpDataObject)
{
    return Extract<GUID>(lpDataObject, CDataObject::m_cfNodeType);
}

INTERNAL* ExtractInternalFormat(LPDATAOBJECT lpDataObject)
{
    return Extract<INTERNAL>(lpDataObject, CDataObject::m_cfInternal);
}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CSnapIn object implementation                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CSnapIn::CSnapIn(CComponentData *pComponent)
{
    m_cRef++;
    InterlockedIncrement(&g_cRefThisDll);

    m_pcd = pComponent;
    m_lViewMode = LVS_ICON;
}

CSnapIn::~CSnapIn()
{
    InterlockedDecrement(&g_cRefThisDll);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CSnapIn object implementation (IUnknown)                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


HRESULT CSnapIn::QueryInterface (REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IComponent) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPCOMPONENT)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

ULONG CSnapIn::AddRef (void)
{
    return ++m_cRef;
}

ULONG CSnapIn::Release (void)
{
    if (--m_cRef == 0) {
        delete this;
        return 0;
    }

    return m_cRef;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CSnapIn object implementation (IComponent)                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CSnapIn::Initialize(LPCONSOLE lpConsole)
{
    // Save the IConsole pointer
    m_pConsole = lpConsole;
    m_pConsole->AddRef();

    LoadString(g_hInstance, IDS_NAME, m_column1, sizeof(m_column1));
    LoadString(g_hInstance, IDS_DESCRIPTION, m_description, sizeof(m_description));
    LoadString(g_hInstance, IDS_SYSTEM, m_TitleSystem, sizeof(m_TitleSystem));
    LoadString(g_hInstance, IDS_MONITOR, m_TitleMonitor, sizeof(m_TitleMonitor));

    HRESULT hr = m_pConsole->QueryInterface(IID_IHeaderCtrl,
                        reinterpret_cast<void**>(&m_pHeader));

    // Give the console the header control interface pointer
    if (SUCCEEDED(hr))
        m_pConsole->SetHeader(m_pHeader);

    m_pConsole->QueryInterface(IID_IResultData,
                        reinterpret_cast<void**>(&m_pResult));

    hr = m_pConsole->QueryResultImageList(&m_pImageResult);

    hr = m_pConsole->QueryConsoleVerb(&m_pConsoleVerb);

    return S_OK;
}

STDMETHODIMP CSnapIn::Destroy(MMC_COOKIE cookie)
{
    if (m_pConsole != NULL)
    {
        m_pConsole->SetHeader(NULL);

        if (m_pHeader != NULL)
        {
            m_pHeader->Release();
            m_pHeader = NULL;
        }
        if (m_pResult != NULL)
        {
            m_pResult->Release();
            m_pResult = NULL;
        }
        if (m_pImageResult != NULL)
        {
            m_pImageResult->Release();
            m_pImageResult = NULL;
        }

        m_pConsole->Release();
        m_pConsole = NULL;

        if (m_pComponentData != NULL)
        {
            m_pComponentData->Release();
            m_pComponentData = NULL;
        }

        if (m_pConsoleVerb != NULL)
        {
            m_pConsoleVerb->Release();
            m_pConsoleVerb = NULL;
        }
    }
    return S_OK;
}


STDMETHODIMP CSnapIn::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    HRESULT hr = S_OK;


    if (event == MMCN_PROPERTY_CHANGE)
    {
//        hr = OnPropertyChange(lpDataObject);
    }
    else if (event == MMCN_VIEW_CHANGE)
    {
//        hr = OnUpdateView(lpDataObject);
    }
    else
    {

        switch(event)
        {

        case MMCN_CLICK:
            break;

        case MMCN_DBLCLICK:
            {
             INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);

             if (pInternal && (pInternal->m_type == CCT_RESULT)) {
                RESULTDATAITEM resultItem;

                memset(&resultItem, 0, sizeof(RESULTDATAITEM));
                resultItem.mask = RDI_STATE;
                resultItem.itemID = m_itemSystem;
                m_pResult->GetItem(&resultItem);
                if (resultItem.nState & LVIS_SELECTED) {
                    DisplayPropertyPage(ITEM_SYSTEM);
                }

                memset(&resultItem, 0, sizeof(RESULTDATAITEM));
                resultItem.mask = RDI_STATE;
                resultItem.itemID = m_itemMonitor;
                m_pResult->GetItem(&resultItem);
                if (resultItem.nState & LVIS_SELECTED) {
                    DisplayPropertyPage(ITEM_MONITOR);
                }
            }
            else
            {
                hr = S_FALSE;
            }

            FREE_INTERNAL(pInternal)

            }
            break;

        case MMCN_ADD_IMAGES:
            HBITMAP hbmp16x16;
            HBITMAP hbmp32x32;

            hbmp16x16 = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_16x16));
            hbmp32x32 = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_32x32));

            // Set the images
            m_pImageResult->ImageListSetStrip(reinterpret_cast<LONG_PTR*>(hbmp16x16),
                                              reinterpret_cast<LONG_PTR*>(hbmp32x32),
                                              0, RGB(255, 0, 255));

            DeleteObject(hbmp16x16);
            DeleteObject(hbmp32x32);
            break;

        case MMCN_SHOW:
            if (arg == TRUE)
            {
                RESULTDATAITEM resultItem;
                INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);

                if (pInternal == NULL)
                {
                    // Extensions not supported by this sample yet.
                    return S_OK;
                }

                MMC_COOKIE cookie = pInternal->m_cookie;
                CFolder* pFolder = dynamic_cast<CComponentData*>(m_pComponentData)->GetFolder();

                if (cookie != (MMC_COOKIE)pFolder)
                {
                    FREE_INTERNAL(pInternal)
                    break;
                }

                m_pHeader->InsertColumn(0, m_column1, LVCFMT_LEFT, 180);     // Name
                m_pResult->SetViewMode(m_lViewMode);

                memset(&resultItem, 0, sizeof(RESULTDATAITEM));
                resultItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
                resultItem.str = MMC_CALLBACK;
                resultItem.nImage = 3;
                resultItem.lParam = reinterpret_cast<LPARAM>(&m_TitleSystem);
                m_pResult->InsertItem(&resultItem);
                m_itemSystem = resultItem.itemID;

                memset(&resultItem, 0, sizeof(RESULTDATAITEM));
                resultItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
                resultItem.str = MMC_CALLBACK;
                resultItem.nImage = 2;
                resultItem.lParam = reinterpret_cast<LPARAM>(&m_TitleMonitor);
                m_pResult->InsertItem(&resultItem);
                m_itemMonitor = resultItem.itemID;

                m_pResult->Sort(0, 0, -1);

                FREE_INTERNAL(pInternal)
            }
            else
            {
                m_pResult->GetViewMode(&m_lViewMode);
            }
            break;

        case MMCN_MINIMIZED:
            break;

        case MMCN_SELECT:

            if (m_pConsoleVerb)
            {
                m_pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);
            }
            break;

        case MMCN_BTN_CLICK:
            break;

        // Note - Future expansion of notify types possible
        default:
            hr = E_UNEXPECTED;
            break;
        }

//        FREE_INTERNAL(pInternal);
    }


    if (m_pResult)
        m_pResult->SetDescBarText(m_description);

    return hr;
}

STDMETHODIMP CSnapIn::GetDisplayInfo(LPRESULTDATAITEM pResult)
{
    if (pResult)
    {
        if (pResult->bScopeItem == TRUE)
        {
            CFolder* pFolder = reinterpret_cast<CFolder*>(pResult->lParam);
            if (pResult->mask & RDI_STR)
            {
                if (pResult->nCol == 0)
                    pResult->str = pFolder->m_pszName;

                if (pResult->str == NULL)
                    pResult->str = (LPOLESTR)_T("");
            }
        }
        else
        {
            if (pResult->mask & RDI_STR)
            {
                if (pResult->nCol == 0)
                    pResult->str = reinterpret_cast<LPOLESTR>(pResult->lParam);

                if (pResult->str == NULL)
                    pResult->str = (LPOLESTR)L"";
            }
        }
    }

    return S_OK;
}

STDMETHODIMP CSnapIn::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT *ppDataObject)
{
    return m_pComponentData->QueryDataObject(cookie, type, ppDataObject);
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CComponentData object implementation                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CComponentData::CComponentData()
{
    m_cRef++;
    InterlockedIncrement(&g_cRefThisDll);

    m_Folder = NULL;
    m_pScope = NULL;
    m_pGPTInformation = NULL;
}

CComponentData::~CComponentData()
{
    InterlockedDecrement(&g_cRefThisDll);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CComponentData object implementation (IUnknown)                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


HRESULT CComponentData::QueryInterface (REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IComponentData) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPCOMPONENT)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

ULONG CComponentData::AddRef (void)
{
    return ++m_cRef;
}

ULONG CComponentData::Release (void)
{
    if (--m_cRef == 0) {
        delete this;
        return 0;
    }

    return m_cRef;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CComponentData object implementation (IComponentData)                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CComponentData::Initialize(LPUNKNOWN pUnknown)
{
    HRESULT hr;
    LPCONSOLE pConsole;
    HBITMAP bmp16x16;
    LPIMAGELIST lpScopeImage;

    // MMC should only call ::Initialize once!
    pUnknown->QueryInterface(IID_IConsoleNameSpace,
                    reinterpret_cast<void**>(&m_pScope));

    //
    // QI for IConsole so we can get the main window's hWnd
    //

    hr = pUnknown->QueryInterface(IID_IConsole, (LPVOID *)&pConsole);

    if (SUCCEEDED(hr))
    {
        pConsole->GetMainWindow (&m_hwndFrame);
        pConsole->Release();
    }


    //
    // Query for the scope imagelist interface
    //

    hr = pConsole->QueryScopeImageList(&lpScopeImage);

    if (SUCCEEDED(hr))
    {
        // Load the bitmaps from the dll
        bmp16x16=LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_16x16));

        // Set the images
        lpScopeImage->ImageListSetStrip(reinterpret_cast<LONG_PTR*>(bmp16x16),
                          reinterpret_cast<LONG_PTR*>(bmp16x16),
                           0, RGB(255, 0, 255));

        lpScopeImage->Release();
    }

    return S_OK;
}

STDMETHODIMP CComponentData::CreateComponent(LPCOMPONENT *ppComponent)
{
    HRESULT hr;
    CSnapIn *pSnapIn;


    //
    // Initialize
    //

    *ppComponent = NULL;


    //
    // Create the snapin view
    //

    pSnapIn = new CSnapIn(this);

    if (!pSnapIn)
        return E_OUTOFMEMORY;


    //
    // QI for IComponent
    //

    hr = pSnapIn->QueryInterface(IID_IComponent, (LPVOID *)ppComponent);
    pSnapIn->Release();     // release QI

    pSnapIn->SetIComponentData(this);

    return hr;
}

STDMETHODIMP CComponentData::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    HRESULT hr = S_OK;

    // Since it's my folder it has an internal format.
    // Design Note: for extension.  I can use the fact, that the data object doesn't have
    // my internal format and I should look at the node type and see how to extend it.
    if (event == MMCN_PROPERTY_CHANGE)
    {
//        hr = OnProperties(param);
    }
    else
    {
        CLSID* pCoClassID = ExtractClassID(lpDataObject);

        if (!IsEqualCLSID(*pCoClassID, CLSID_GPESnapIn))
        {
            return hr;
        }

        switch(event)
        {
        case MMCN_SHOW:
            if (!m_pGPTInformation)
            {
                lpDataObject->QueryInterface(IID_IGPEInformation, (LPVOID *)&m_pGPTInformation);
            }
            break;

        case MMCN_DELETE:
//            hr = OnDelete(cookie, arg, param);
            break;

        case MMCN_RENAME:
//            hr = OnRename(cookie, arg, param);
            break;

        case MMCN_EXPAND:
            if (arg == TRUE)
            {
                if (!m_pGPTInformation)
                {
                    lpDataObject->QueryInterface(IID_IGPEInformation, (LPVOID *)&m_pGPTInformation);
                }

                if (m_pGPTInformation)
                {
                    EnumerateScopePane(lpDataObject, param);
                }
            }
            break;


        case MMCN_CONTEXTMENU:
//            hr = OnContextMenu(cookie, arg, param);
            break;

        case MMCN_BTN_CLICK:
//            AfxMessageBox(_T("CComponentDataImpl::MMCN_BTN_CLICK"));
            break;

        default:
            break;
        }

    }

    return hr;
}

STDMETHODIMP CComponentData::Destroy()
{
    // Delete enumerated scope items
    delete m_Folder;

    if (m_pGPTInformation)
    {
        m_pGPTInformation->Release();
        m_pGPTInformation = NULL;
    }


    if (m_pScope != NULL)
    {
        m_pScope->Release();
        m_pScope = NULL;
    }

    return S_OK;
}

STDMETHODIMP CComponentData::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                                             LPDATAOBJECT* ppDataObject)
{
    HRESULT hr = E_NOINTERFACE;
    CDataObject *pDataObject = new CDataObject();   // ref == 1

    if (!pDataObject)
        return E_OUTOFMEMORY;

    hr = pDataObject->QueryInterface(IID_IDataObject, (LPVOID *)ppDataObject);

    pDataObject->SetType(type);
    pDataObject->SetCookie(cookie);
    pDataObject->SetClsid(GetCoClassID());

    pDataObject->Release();     // release initial ref

    return hr;
}



void CComponentData::CreateFolder(LPDATAOBJECT lpDataObject)
{
    wchar_t buf[100];
    BOOL bExtension = FALSE;

    CLSID* pCoClassID = ExtractClassID(lpDataObject);

    LoadString (g_hInstance, IDS_POWER_MGMT, buf, 100);

    if (!IsEqualCLSID(*pCoClassID, GetCoClassID()))
    {
        bExtension = TRUE;
    }

    m_Folder = new CFolder();

    // Create the folder objects with static data
    m_Folder->Create(buf, 0, 1, bExtension);
    m_Folder->SetCookie(NULL);

    // Free memory from data object extraction
    if (pCoClassID)
        GlobalFree(pCoClassID);
}

void CComponentData::EnumerateScopePane(LPDATAOBJECT lpDataObject, HSCOPEITEM pParent)
{
    int i;

 #if 0
    INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);

    if (pInternal == NULL)
        return ;

    long cookie = pInternal->m_cookie;

    // Only the static node has enumerated children
    if (cookie != NULL)
        return ;

    if (pInternal != NULL)
        GlobalFree(pInternal);
#endif

    // Initialize folder list if empty
    if (m_Folder == NULL)
        CreateFolder(lpDataObject);

    // Set the parent
    m_Folder->m_pScopeItem->relativeID = pParent;

    // Set the folder as the cookie
    m_Folder->m_pScopeItem->mask |= SDI_PARAM;
    m_Folder->m_pScopeItem->lParam = reinterpret_cast<LPARAM>(m_Folder);
    m_Folder->SetCookie(reinterpret_cast<MMC_COOKIE>(m_Folder));
    m_pScope->InsertItem(m_Folder->m_pScopeItem);

}

CFolder* CComponentData::GetFolder(void)
{
    return m_Folder;
}

STDMETHODIMP CComponentData::GetDisplayInfo(SCOPEDATAITEM* pScopeDataItem)
{
    if (pScopeDataItem == NULL)
        return E_POINTER;

    CFolder* pFolder = reinterpret_cast<CFolder*>(pScopeDataItem->lParam);

    pScopeDataItem->displayname = pFolder->m_pszName;

    return S_OK;
}
