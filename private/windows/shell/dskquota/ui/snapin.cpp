///////////////////////////////////////////////////////////////////////////////
/*  File: snapin.cpp

    Description: Implements the MMC snapin for disk quota policy.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    02/14/98    Initial creation.                                    BrianAu
    06/25/98    Disabled snapin code with #ifdef POLICY_MMC_SNAPIN.  BrianAu
                Switching to ADM-file approach to entering policy
                data.  Keeping snapin code available in case
                we decide to switch back at a later time.
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#pragma hdrstop

#ifdef POLICY_MMC_SNAPIN

#include "snapin.h"
#include "resource.h"
#include "guidsp.h"
#include "policy.h"



//-----------------------------------------------------------------------------
// CSnapInItem
//-----------------------------------------------------------------------------
//
// General rendering function usable by this class and any
// derived classes to render data onto a medium.  Only accepts
// TYMED_HGLOBAL.
//
HRESULT
CSnapInItem::RenderData(  // [ static ]
    LPVOID pvData,
    int cbData,
    LPSTGMEDIUM pMedium
    )
{
    DBGTRACE((DM_SNAPIN, DL_MID, TEXT("CSnapInItem::RenderData [ general ]")));
    HRESULT hr = DV_E_TYMED;

    if (NULL == pvData || NULL == pMedium)
        return E_INVALIDARG;

    //
    // Make sure the type medium is HGLOBAL
    //
    if (TYMED_HGLOBAL == pMedium->tymed)
    {
        //
        // Create the stream on the hGlobal passed in
        //
        LPSTREAM pStream;
        hr = CreateStreamOnHGlobal(pMedium->hGlobal, FALSE, &pStream);

        if (SUCCEEDED(hr))
        {
            //
            // Write to the stream the number of bytes
            //
            unsigned long written;

            hr = pStream->Write(pvData, cbData, &written);

            //
            // Because we told CreateStreamOnHGlobal with 'FALSE',
            // only the stream is released here.
            // Note - the caller (i.e. snap-in, object) will free the HGLOBAL
            // at the correct time.  This is according to the IDataObject specification.
            //
            pStream->Release();
        }
    }

    return hr;
}


//
// Format a GUID as a string.
//
void
CSnapInItem::GUIDToString(   // [ static ]
    const GUID& guid,
    CString *pstr
    )
{
    StringFromGUID2(guid, pstr->GetBuffer(50), 50);
    pstr->ReleaseBuffer();
}



//-----------------------------------------------------------------------------
// CScopeItem
//-----------------------------------------------------------------------------
UINT CScopeItem::m_cfNodeType       = RegisterClipboardFormat(CCF_NODETYPE);
UINT CScopeItem::m_cfNodeTypeString = RegisterClipboardFormat(CCF_SZNODETYPE);
UINT CScopeItem::m_cfDisplayName    = RegisterClipboardFormat(CCF_DISPLAY_NAME);
UINT CScopeItem::m_cfCoClass        = RegisterClipboardFormat(CCF_SNAPIN_CLASSID);

//
// Returns:  S_OK            = Data successfully rendered.
//           DV_E_CLIPFORMAT = Clipboard format not supported.
//           DV_E_TYMED      = Medium is not HGLOBAL.
//           Other           = Rendering error.
HRESULT
CScopeItem::RenderData(
    UINT cf,
    LPSTGMEDIUM pMedium
    ) const
{
    DBGTRACE((DM_SNAPIN, DL_MID, TEXT("CScopeItem::RenderData")));
    HRESULT hr = DV_E_CLIPFORMAT;

    if (cf == m_cfNodeType)
    {
        DBGPRINT((DM_SNAPIN, DL_LOW, TEXT("Rendering CCF_NODETYPE")));
        hr = CSnapInItem::RenderData((LPVOID)&m_idType, sizeof(m_idType), pMedium);
    }
    else if (cf == m_cfNodeTypeString)
    {
        DBGPRINT((DM_SNAPIN, DL_LOW, TEXT("Rendering CCF_SZNODETYPE")));
        CString s;
        GUIDToString(m_idType, &s);
        hr = CSnapInItem::RenderData((LPVOID)s.Cstr(), (s.Length() + 1) * sizeof(TCHAR), pMedium);
    }
    else if (cf == m_cfDisplayName)
    {
        DBGPRINT((DM_SNAPIN, DL_LOW, TEXT("Rendering CCF_DISPLAY_NAME")));
        const CString& s = m_cd.DisplayName();
        hr = CSnapInItem::RenderData((LPVOID)s.Cstr(), (s.Length() + 1) * sizeof(TCHAR), pMedium);
    }
    else if (cf == m_cfCoClass)
    {
        DBGPRINT((DM_SNAPIN, DL_LOW, TEXT("Rendering CCF_SNAPIN_CLASSID")));
        hr = CSnapInItem::RenderData((LPVOID)&m_cd.ClassId(), sizeof(GUID), pMedium);
    }
    return hr;
}


CScopeItem::~CScopeItem(
    void
    )
{
    DBGTRACE((DM_SNAPIN, DL_MID, TEXT("CScopeItem::~CScopeItem")));
    while(0 < m_rgpChildren.Count())
    {
        delete m_rgpChildren[0];
        m_rgpChildren.Delete(0);
    }
    while(0 < m_rgpResultItems.Count())
    {
        delete m_rgpResultItems[0];
        m_rgpResultItems.Delete(0);
    }
}



//-----------------------------------------------------------------------------
// CSnapInComp
//-----------------------------------------------------------------------------

CSnapInComp::CSnapInComp(
    HINSTANCE hInstance,
    CSnapInCompData& cd
    ) : m_cRef(0),
        m_hInstance(hInstance),
        m_cd(cd),
        m_pConsole(NULL),
        m_pResult(NULL),
        m_pHeader(NULL),
        m_pImageResult(NULL),
        m_strColumn(hInstance, IDS_SNAPIN_COLUMN),
        m_cxColumn(200),
        m_lViewMode(LVS_ICON)
{
    DBGTRACE((DM_SNAPIN, DL_MID, TEXT("CSnapInComp::CSnapInComp")));
}


CSnapInComp::~CSnapInComp(
    void
    )
{
    DBGTRACE((DM_SNAPIN, DL_MID, TEXT("CSnapInComp::~CSnapInComp")));

    if (NULL != m_pImageResult)
    {
        m_pImageResult->Release();
        m_pImageResult = NULL;
    }
    if (NULL != m_pResult)
    {
        m_pResult->Release();
        m_pResult = NULL;
    }
    if (NULL != m_pHeader)
    {
        m_pHeader->Release();
        m_pHeader = NULL;
    }
    if (NULL != m_pConsole)
    {
        m_pConsole->Release();
        m_pConsole = NULL;
    }
}



HRESULT
CSnapInComp::QueryInterface(
    REFIID riid, 
    LPVOID *ppvOut
    )
{
    DBGTRACE((DM_SNAPIN, DL_MID, TEXT("CSnapInComp::QueryInterface")));
    DBGPRINTIID(DM_SNAPIN, DL_MID, riid);

    HRESULT hr = E_NOINTERFACE;

    if (NULL == ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;

    if (IID_IUnknown == riid || IID_IComponent == riid)
    {
        *ppvOut = this;
    }
    else if (IID_IExtendContextMenu == riid)
    {
        *ppvOut = static_cast<IExtendContextMenu *>(this);
    }

    if (NULL != *ppvOut)
    {
        ((LPUNKNOWN)*ppvOut)->AddRef();
        hr = NOERROR;
    }

    return hr;
}



ULONG
CSnapInComp::AddRef(
    void
    )
{
    DBGTRACE((DM_SNAPIN, DL_LOW, TEXT("CSnapInComp::AddRef")));
    ULONG ulReturn = m_cRef + 1;
    InterlockedIncrement(&m_cRef);
    return ulReturn;
}


ULONG
CSnapInComp::Release(
    void
    )
{
    DBGTRACE((DM_SNAPIN, DL_LOW, TEXT("CSnapInComp::Release")));
    ULONG ulReturn = m_cRef - 1;
    if (InterlockedDecrement(&m_cRef) == 0)
    {   
        delete this;
        ulReturn = 0;
    }
    return ulReturn;
}


HRESULT
CSnapInComp::Initialize(
    LPCONSOLE lpConsole
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInComp::Initialize")));
    HRESULT hr = NOERROR;

    m_pConsole = lpConsole;
    m_pConsole->AddRef();

    //
    // Get IResultData interface to result pane.
    //
    hr = m_pConsole->QueryInterface(IID_IResultData,
                        reinterpret_cast<void **>(&m_pResult));

    if (FAILED(hr))
        return hr;
    //
    // Get IHeaderCtrl interface to header control.
    //
    hr = m_pConsole->QueryInterface(IID_IHeaderCtrl,
                        reinterpret_cast<void **>(&m_pHeader));
    if (FAILED(hr))
        return hr;

//    m_pConsole->SetHeader(m_pHeader); // BUGBUG: Needed?

    hr = m_pConsole->QueryResultImageList(&m_pImageResult);

    return hr;
}


HRESULT
CSnapInComp::Notify(
    LPDATAOBJECT lpDataObject,
    MMC_NOTIFY_TYPE event, 
    long arg, 
    long param
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInComp::Notify")));
    DBGPRINT((DM_SNAPIN, DL_LOW, TEXT("\tpObj = 0x%08X, event = %d, arg = %d, param = 0x%08X"), lpDataObject, event, arg, param));
    HRESULT hr = S_OK;

    switch(event)
    {
        case MMCN_ADD_IMAGES:
            DBGPRINT((DM_SNAPIN, DL_LOW, TEXT("MMCN_ADD_IMAGES")));
            {
                //
                // Result pane image list has only one icon.
                //
                HICON hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_QUOTA));
                m_pImageResult->ImageListSetIcon(reinterpret_cast<long *>(hIcon), 0);
            }
            break;

        case MMCN_SHOW:
            DBGPRINT((DM_SNAPIN, DL_LOW, TEXT("MMCN_SHOW, arg = %d"), arg));
            if (arg)
            {
                //
                // Showing view.  Set view information.
                //
                m_pHeader->InsertColumn(0, m_strColumn, LVCFMT_LEFT, m_cxColumn);
                m_pResult->SetViewMode(m_lViewMode);            

                CScopeItem *psi;
                hr = GetScopeItem((HSCOPEITEM)param, &psi);
                if (FAILED(hr))
                {
                    DBGERROR((TEXT("Failed getting item for MMCN_SHOW")));
                    return hr;
                }

                RESULTDATAITEM rdi;
                rdi.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
                int cResultItems = psi->NumResultItems();

                for (int i = 0; i < cResultItems; i++)
                {
                    CResultItem *pri = psi->ResultItem(i);
                    DBGASSERT((NULL != pri));
                    rdi.str    = MMC_CALLBACK;
                    rdi.nImage = pri->ImageIndex();
                    rdi.lParam = (LPARAM)pri;
                    m_pResult->InsertItem(&rdi);
                }
            }
            else
            {
                //
                // Hiding view.  Gather view information.
                //
                m_pHeader->GetColumnWidth(0, &m_cxColumn);
                m_pResult->GetViewMode(&m_lViewMode);
            }
            break;

        case MMCN_CLICK:
        case MMCN_DBLCLICK:
            DBGPRINT((DM_SNAPIN, DL_LOW, TEXT("MMCN_CLICK or MMCN_DBLCLICK")));
            m_cd.OpenVolumeQuotaProperties();
            break;

        default:
            break;
    }
    return hr;
}


HRESULT
CSnapInComp::Destroy(
    LONG cookie
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInComp::Destroy")));
    if (NULL != m_pImageResult)
    {
        m_pImageResult->Release();
        m_pImageResult = NULL;
    }
    if (NULL != m_pResult)
    {
        m_pResult->Release();
        m_pResult = NULL;
    }
    if (NULL != m_pHeader)
    {
        m_pHeader->Release();
        m_pHeader = NULL;
    }
    if (NULL != m_pConsole)
    {
        m_pConsole->Release();
        m_pConsole = NULL;
    }

    return NOERROR;
}


HRESULT
CSnapInComp::QueryDataObject(
    long cookie, 
    DATA_OBJECT_TYPES type, 
    LPDATAOBJECT *ppDataObject
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInComp::QueryDataObject")));
    //
    // Forward the data object query to the component data object.
    //
    return m_cd.QueryDataObject(cookie, type, ppDataObject);
}


HRESULT
CSnapInComp::GetResultViewType(
    long cookie, 
    LPOLESTR *ppViewType, 
    long *pViewOptions
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInComp::GetResultViewType")));
    //
    // Tell snapin mgr to use default listview.
    //
    return S_FALSE;
}


HRESULT
CSnapInComp::GetDisplayInfo(
    RESULTDATAITEM *prdi
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInComp::GetDisplayInfo")));
    CSnapInItem *psi = reinterpret_cast<CSnapInItem *>(prdi->lParam);

    if (RDI_STR & prdi->mask)
    {
        prdi->str = (LPOLESTR)psi->DisplayName().Cstr();
    }
    if (RDI_IMAGE & prdi->mask)
    {
        prdi->nImage = psi->ImageIndex();
    }
    if (RDI_STATE & prdi->mask)
    {
        prdi->nState = 0;
    }
    if (RDI_INDEX & prdi->mask)
    {
        prdi->nIndex = 0;
    }
    if (RDI_INDENT & prdi->mask)
    {
        prdi->iIndent = 0;
    }
    
    return S_OK;
}



HRESULT
CSnapInComp::CompareObjects(
    LPDATAOBJECT pdoA, 
    LPDATAOBJECT pdoB
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInComp::CompareObjects")));
    return m_cd.CompareObjects(pdoA, pdoB);
}


HRESULT
CSnapInComp::AddMenuItems(
    LPDATAOBJECT pDataObject, 
    LPCONTEXTMENUCALLBACK piCallback, 
    long *pInsertionAllowed
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInComp::AddMenuItems")));
    return m_cd.AddMenuItems(pDataObject, piCallback, pInsertionAllowed);
}

HRESULT
CSnapInComp::Command(
    long lCommandID, 
    LPDATAOBJECT pDataObject
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInComp::Command")));
    return m_cd.Command(lCommandID, pDataObject);
}

 

HRESULT
CSnapInComp::GetScopeItem(
    HSCOPEITEM hItem,
    CScopeItem **ppsi
    ) const
{
    HRESULT hr;
    SCOPEDATAITEM item;
    item.mask = SDI_PARAM;
    item.ID   = hItem;

    hr = m_cd.m_pScope->GetItem(&item);
    if (SUCCEEDED(hr))
    {
        *ppsi = reinterpret_cast<CScopeItem *>(item.lParam);
    }
    return hr;
}


//-----------------------------------------------------------------------------
// CSnapInCompData
//-----------------------------------------------------------------------------

CSnapInCompData::CSnapInCompData(
    HINSTANCE hInstance,
    LPCTSTR pszDisplayName,
    const GUID& idClass
    ) : m_cRef(0),
        m_hInstance(hInstance),
        m_strDisplayName(pszDisplayName),
        m_idClass(idClass),
        m_pConsole(NULL),
        m_pScope(NULL),
        m_pRootScopeItem(NULL),
        m_hRoot(NULL),
        m_pGPEInformation(NULL)
{
    DBGTRACE((DM_SNAPIN, DL_MID, TEXT("CSnapInCompData::CSnapInCompData")));

    InterlockedIncrement(&g_cRefThisDll);

    //
    // Root node.
    //
    m_pRootScopeItem = new CScopeItem(NODEID_DiskQuotaRoot, // Node ID
                                      *this,                // Snapin comp data ref.
                                      NULL,                 // Parent node ptr.
                                      TEXT("<root>"),       // Node name str.
                                      -1,                   // Image index.
                                      -1);                  // Image index "open".
    //
    // Add "Disk Quota Settings" as a child of the root.
    //
    CString strName(hInstance, IDS_SNAPIN_SCOPENAME);
    CScopeItem *psi = new CScopeItem(NODEID_DiskQuotaSettings,
                                     *this,
                                     m_pRootScopeItem,
                                     strName,
                                     iICON_QUOTA,
                                     iICON_QUOTA_OPEN);
    m_pRootScopeItem->AddChild(psi);

    //
    // Add single result item to "Disk Quota Settings".
    //
    strName.Format(hInstance, IDS_SNAPIN_RESULTNAME);
    psi->AddResultItem(new CResultItem(strName, 0, *psi));
}


CSnapInCompData::~CSnapInCompData(
    void
    )
{
    DBGTRACE((DM_SNAPIN, DL_MID, TEXT("CSnapInCompData::~CSnapInCompData")));

    delete m_pRootScopeItem;

    if (NULL != m_pScope)
        m_pScope->Release();

    if (NULL != m_pConsole)
        m_pConsole->Release();

    if (NULL != m_pGPEInformation)
        m_pGPEInformation->Release();

    InterlockedDecrement(&g_cRefThisDll);
}


HRESULT
CSnapInCompData::QueryInterface(
    REFIID riid, 
    LPVOID *ppvOut
    )
{
    DBGTRACE((DM_SNAPIN, DL_MID, TEXT("CSnapInCompData::QueryInterface")));
    DBGPRINTIID(DM_SNAPIN, DL_MID, riid);
    HRESULT hr = E_NOINTERFACE;

    if (NULL == ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;

    if (IID_IUnknown == riid || IID_IComponentData == riid)
    {
        *ppvOut = this;
    }
    else if (IID_IPersistStreamInit == riid)
    {
        *ppvOut = static_cast<IPersistStreamInit *>(this);
    }
    else if (IID_IExtendContextMenu == riid)
    {
        *ppvOut = static_cast<IExtendContextMenu *>(this);
    }

    if (NULL != *ppvOut)
    {
        ((LPUNKNOWN)*ppvOut)->AddRef();
        hr = NOERROR;
    }

    return hr;
}



ULONG
CSnapInCompData::AddRef(
    void
    )
{
    DBGTRACE((DM_SNAPIN, DL_LOW, TEXT("CSnapInCompData::AddRef")));
    ULONG ulReturn = m_cRef + 1;
    InterlockedIncrement(&m_cRef);
    return ulReturn;
}


ULONG
CSnapInCompData::Release(
    void
    )
{
    DBGTRACE((DM_SNAPIN, DL_LOW, TEXT("CSnapInCompData::Release")));
    ULONG ulReturn = m_cRef - 1;
    if (InterlockedDecrement(&m_cRef) == 0)
    {   
        delete this;
        ulReturn = 0;
    }
    return ulReturn;
}


HRESULT
CSnapInCompData::Initialize(
    LPUNKNOWN pUnk
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInCompData::Initialize")));
    DBGASSERT((NULL != pUnk));
    DBGASSERT((NULL == m_pConsole));

    HRESULT hr = pUnk->QueryInterface(IID_IConsole, 
                                      reinterpret_cast<void **>(&m_pConsole));

    if (FAILED(hr))
    {
        DBGERROR((TEXT("CSnapInCompData failed QI for IID_IConsole")));
        return hr;
    }

    hr = pUnk->QueryInterface(IID_IConsoleNameSpace,
                                      reinterpret_cast<void **>(&m_pScope));
    if (FAILED(hr))
    {
        DBGERROR((TEXT("CSnapInCompData failed QI for IID_IConsoleNameSpace")));
        return hr;
    }

    LPIMAGELIST pImageList = NULL;
    hr = m_pConsole->QueryScopeImageList(&pImageList);
    if (FAILED(hr))
    {
        DBGERROR((TEXT("CSnapInCompData failed to get scope image list")));
        return hr;
    }

    //
    // Set the scope pane's image list icons.
    //
    static const struct
    {
        int idIcon;
        int iIconIndex;

    } rgIcons[] = { { IDI_QUOTA,      iICON_QUOTA },
                    { IDI_QUOTA_OPEN, iICON_QUOTA_OPEN } };

    for (int i = 0; i < ARRAYSIZE(rgIcons); i++)
    {
        HICON hIcon = LoadIcon(m_hInstance, 
                               MAKEINTRESOURCE(rgIcons[i].idIcon));
        pImageList->ImageListSetIcon(reinterpret_cast<long *>(hIcon), 
                                     rgIcons[i].iIconIndex);
    }

    pImageList->Release();
        
    return hr;
}


HRESULT
CSnapInCompData::CreateComponent(
    LPCOMPONENT *ppComp
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInCompData::CreateComponent")));

    HRESULT hr = S_OK;
    try
    {
        CSnapInComp *pComp = new CSnapInComp(m_hInstance, *this);
        hr = pComp->QueryInterface(IID_IComponent, reinterpret_cast<void **>(ppComp));
    }
    catch(CAllocException& e)
    {
        DBGERROR((TEXT("Insufficient memory")));
        hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
        DBGERROR((TEXT("C++ exception caught")));
        hr = E_UNEXPECTED;
    }
    return hr;
}


HRESULT
CSnapInCompData::Destroy(
    void
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInCompData::Destroy")));

    if (NULL != m_pScope)
    {
        m_pScope->Release();
        m_pScope = NULL;
    }

    if (NULL != m_pConsole)
    {
        m_pConsole->Release();
        m_pConsole = NULL;
    }
    return S_OK;
}


HRESULT
CSnapInCompData::Notify(
    LPDATAOBJECT pDataObject, 
    MMC_NOTIFY_TYPE event, 
    long arg, 
    long param
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInCompData::Notify")));
    DBGPRINT((DM_SNAPIN, DL_LOW, TEXT("\tpDataObj = 0x%08X, event = 0x%08X, arg = %d, param = 0x%08X"), 
              pDataObject, event, arg, param));
    HRESULT hr = S_OK;
    try
    {
        switch(event)
        {
            case MMCN_EXPAND:
                if (arg)
                {
                    if (NULL == m_pGPEInformation)
                    {
                        hr = pDataObject->QueryInterface(IID_IGPEInformation, 
                                                         reinterpret_cast<void **>(&m_pGPEInformation));
                    }
                    if (NULL != m_pGPEInformation)
                    {
                        hr = EnumerateScopePane((HSCOPEITEM)param);
                    }
                }
                break;

            default:
                break;
        }
    }
    catch(CAllocException& e)
    {
        DBGERROR((TEXT("Insufficient memory")));
        hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
        DBGERROR((TEXT("C++ exception caught")));
        hr = E_UNEXPECTED;
    }

    return hr;
}


HRESULT 
CSnapInCompData::GetDisplayInfo(
    SCOPEDATAITEM *psdi
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInCompData::GetDispInfo")));
    DBGPRINT((DM_SNAPIN, DL_LOW, TEXT("\tmask = 0x%08X"), psdi->mask));

    HRESULT hr = S_OK;

    CScopeItem *pItem = reinterpret_cast<CScopeItem *>(psdi->lParam);
    if (psdi->mask & SDI_STR)
    {
        psdi->displayname = (LPOLESTR)((LPCTSTR)pItem->DisplayName());
    }
    if (psdi->mask & SDI_IMAGE)
    {
        psdi->nImage = pItem->ImageIndex();
    }
    if (psdi->mask & SDI_OPENIMAGE)
    {
        psdi->nOpenImage = pItem->OpenImageIndex();
    }
    if (psdi->mask & SDI_CHILDREN)
    {
        psdi->cChildren = pItem->NumChildren();
    }

    return hr;
}


HRESULT
CSnapInCompData::CompareObjects(
    LPDATAOBJECT pdoA, 
    LPDATAOBJECT pdoB
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInCompData::GetDispInfo")));
    LPQUOTADATAOBJECT pDataA, pDataB;

    HRESULT hr = S_FALSE;

    if (FAILED(pdoA->QueryInterface(IID_IDiskQuotaSnapInData, reinterpret_cast<void **>(&pDataA))))
        return S_FALSE;

    if (FAILED(pdoB->QueryInterface(IID_IDiskQuotaSnapInData, reinterpret_cast<void **>(&pDataB))))
    {
        pDataA->Release();
        return FALSE;
    }

    CSnapInItem *psiA, *psiB;
    pDataA->GetItem(&psiA);
    pDataB->GetItem(&psiB);

    if (psiA == psiB)
        hr = S_OK;

    pDataA->Release();
    pDataB->Release();

    return hr;
}


HRESULT
CSnapInCompData::QueryDataObject(
    long cookie, 
    DATA_OBJECT_TYPES type, 
    LPDATAOBJECT *ppDataObject
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInCompData::QueryDataObject")));

    HRESULT hr = E_NOINTERFACE;
    CDataObject *pDataObject     = NULL;
    LPQUOTADATAOBJECT pQuotaData = NULL;

    try
    {
        pDataObject = new CDataObject(*this);
        pDataObject->SetType(type);
        pDataObject->SetItem(reinterpret_cast<CSnapInItem *>(cookie));

        hr = pDataObject->QueryInterface(IID_IDataObject, (LPVOID *)ppDataObject);
    }
    catch(CAllocException& e)
    {
        DBGERROR((TEXT("Insufficient memory")));
        hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
        DBGERROR((TEXT("C++ exception caught")));
        hr = E_UNEXPECTED;
    }

    return hr;
}


HRESULT 
CSnapInCompData::EnumerateScopePane (
    HSCOPEITEM hParent
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInCompData::EnumerateScopePane")));
    HRESULT hr = S_OK;
    CScopeItem *psi = m_pRootScopeItem; // Default to enumerating from root.
    SCOPEDATAITEM item;

    if (NULL == m_hRoot)
        m_hRoot = hParent;      // Remember the root node's handle.

    if (m_hRoot != hParent)
    {
        item.mask = SDI_PARAM;
        item.ID   = hParent;

        hr = m_pScope->GetItem (&item);
        if (FAILED(hr))
        {
            DBGERROR((TEXT("Failed getting item")));
            return hr;
        }

        psi = reinterpret_cast<CScopeItem *>(item.lParam);
    }

    item.mask = SDI_STR | SDI_STATE | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM | SDI_CHILDREN;
    item.nState      = 0;
    item.relativeID  = hParent;

    int cChildren = psi->NumChildren();
    for (int iChild = 0; iChild < cChildren; iChild++)
    {
        CScopeItem *pChild = psi->Child(iChild);

        item.displayname = MMC_CALLBACK;
        item.nImage      = pChild->ImageIndex();
        item.nOpenImage  = pChild->OpenImageIndex();
        item.cChildren   = pChild->NumChildren();
        item.lParam      = reinterpret_cast<LPARAM>(pChild);

        m_pScope->InsertItem (&item);
    }
    return S_OK;
}



HRESULT
CSnapInCompData::InitNew(
    void
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInCompData::InitNew")));
    return S_OK;
}


HRESULT
CSnapInCompData::GetClassID(
    CLSID *pClassID
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInCompData::GetClassID")));
    if (!pClassID)
        return E_FAIL;

    *pClassID = ClassId();

    return S_OK;
}

HRESULT
CSnapInCompData::IsDirty(
    void
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInCompData::IsDirty")));
    return S_FALSE;
}

HRESULT
CSnapInCompData::Load(
    IStream *pStm
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInCompData::Load")));
    return S_OK;
}


HRESULT
CSnapInCompData::Save(
    IStream *pStm, 
    BOOL fClearDirty
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInCompData::Save")));
    return S_OK;
}


HRESULT 
CSnapInCompData::GetSizeMax(
    ULARGE_INTEGER *pcbSize
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInCompData::GetSizeMax")));
    DWORD dwSize = 0;

    if (NULL == pcbSize)
        return E_FAIL;

    pcbSize->QuadPart = 0;

    return S_OK;
}



HRESULT
CSnapInCompData::AddMenuItems(
    LPDATAOBJECT pDataObject, 
    LPCONTEXTMENUCALLBACK piCallback, 
    long *pInsertionAllowed
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInCompData::AddMenuItems")));
    HRESULT hr = S_OK;

    if (CCM_INSERTIONALLOWED_TOP & *pInsertionAllowed)
    {
        CString strOpen(m_hInstance, IDS_VERB_OPEN);
        CONTEXTMENUITEM ci;
        ci.strName           = strOpen;
        ci.strStatusBarText  = NULL;
        ci.lCommandID        = 0;
        ci.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
        ci.fFlags            = 0;
        ci.fSpecialFlags     = CCM_SPECIAL_DEFAULT_ITEM;

        hr = piCallback->AddItem(&ci);
        if (FAILED(hr))
        {
            DBGERROR((TEXT("Error 0x%08X adding context menu item"), hr));
        }
    }
    return hr;
}

HRESULT
CSnapInCompData::Command(
    long lCommandID, 
    LPDATAOBJECT pDataObject
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInCompData::Command")));
    DBGPRINT((DM_SNAPIN, DL_LOW, TEXT("Command ID = %d"), lCommandID));

    if (0 == lCommandID)
        OpenVolumeQuotaProperties();
    else
        DBGERROR((TEXT("Unrecognized command ID (%d)"), lCommandID));

    return S_OK;
}

const int MAX_SNAPIN_PROP_PAGES = 1;

BOOL CALLBACK 
CSnapInCompData::AddPropSheetPage(
    HPROPSHEETPAGE hpage, 
    LPARAM lParam
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInCompData::AddPropSheetPage")));
    PROPSHEETHEADER * ppsh = (PROPSHEETHEADER *)lParam;

    if (ppsh->nPages < MAX_SNAPIN_PROP_PAGES)
    {
        ppsh->phpage[ppsh->nPages++] = hpage;
        return TRUE;
    }

    return FALSE;
}


DWORD
CSnapInCompData::PropPageThreadProc(
    LPVOID pvParam
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInCompData::PropPageThreadProc")));
    MSG msg;
    HWND hwndPropSheet = NULL;
    CSnapInCompData *pThis = (CSnapInCompData *)pvParam;

    com_autoptr<IShellExtInit> psei;
    HRESULT hr = CoInitialize(NULL);
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_DiskQuotaUI,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IShellExtInit,
                              reinterpret_cast<void **>(psei.getaddr()));

        if (SUCCEEDED(hr))
        {
            com_autoptr<IShellPropSheetExt> pspse;
            hr = psei->QueryInterface(IID_ISnapInPropSheetExt,
                                     reinterpret_cast<void **>(pspse.getaddr()));
            if (SUCCEEDED(hr))
            {
                com_autoptr<IDiskQuotaPolicy> pdqp;
                hr = pspse->QueryInterface(IID_IDiskQuotaPolicy,
                                          reinterpret_cast<void **>(pdqp.getaddr()));
                if (SUCCEEDED(hr))
                {
                    pdqp->Initialize(pThis->m_pGPEInformation, NULL);

                    HPROPSHEETPAGE rghPages[1];

                    PROPSHEETHEADER psh;
                    ZeroMemory(&psh, sizeof(psh));
                    //
                    // Define sheet.
                    //
                    psh.dwSize          = sizeof(PROPSHEETHEADER);
                    psh.dwFlags         = PSH_MODELESS;
                    psh.hInstance       = pThis->m_hInstance;
                    psh.pszIcon         = NULL;
                    psh.pszCaption      = NULL;
                    psh.nPages          = 0;
                    psh.nStartPage      = 0;
                    psh.phpage          = rghPages;

                    pThis->m_pConsole->GetMainWindow(&psh.hwndParent);

                    hr = pspse->AddPages(AddPropSheetPage, (LPARAM)&psh);
                    if (SUCCEEDED(hr))
                    {
                        hwndPropSheet = (HWND)PropertySheet(&psh);
                        if (NULL != hwndPropSheet)
                        {
                            //
                            // Set the title on the property sheet.
                            //
                            CString strTitle(pThis->m_hInstance, IDS_SNAPIN_POLICYDLG_TITLE);
                            SetWindowText(hwndPropSheet, strTitle);

                            MSG msg;
                            while (0 != ::GetMessage(&msg, NULL, 0, 0))
                            {
                                if (!PropSheet_IsDialogMessage(hwndPropSheet, &msg))
                                {
                                    ::TranslateMessage(&msg);
                                    ::DispatchMessage(&msg);
                                }
                                if (NULL == PropSheet_GetCurrentPageHwnd(hwndPropSheet))
                                {
                                    DestroyWindow(hwndPropSheet);
                                    PostQuitMessage(0);
                                }
                            }
                        }
                        else if (-1 == (int)hwndPropSheet)
                        {
                            DBGERROR((TEXT("PropertySheet failed with error %d"), GetLastError()));
                        }
                    }
                    else
                    {
                        DBGERROR((TEXT("AddPages failed")));
                    }
                }
            }
        }
        else
            DBGERROR((TEXT("CoCreateInstance failed with result 0x%08X"), hr));

        CoUninitialize();
    }
    else
        DBGERROR((TEXT("CoInitialize failed with result 0x%08X"), hr));
    return 0;
}


//
// Create a new thread and run the volume properties dialog (modified for
// policy snapin).
//
HRESULT
CSnapInCompData::OpenVolumeQuotaProperties(
    void
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CSnapInCompData::OpenVolumeQuotaProperties")));
    HRESULT hr = E_FAIL;
    UINT idThread;
    HANDLE hThread = CreateThread(NULL,
                                  0,          // Default stack size
                                  (LPTHREAD_START_ROUTINE)PropPageThreadProc,
                                  (LPVOID)this,
                                  0,
                                  (LPDWORD)&idThread);
    if (INVALID_HANDLE_VALUE != hThread)
    {
        hr = NOERROR;
        CloseHandle(hThread);
    }

    return hr;
}




//-----------------------------------------------------------------------------
// Snap-in data object implementation.  CDataObject
//-----------------------------------------------------------------------------
CDataObject::CDataObject(
    CSnapInCompData& cd
    ) : m_cRef(0),
        m_cd(cd),
        m_type(CCT_UNINITIALIZED),
        m_pItem(NULL)
{
    DBGTRACE((DM_SNAPIN, DL_MID, TEXT("CDataObject::CDataObject")));

}


CDataObject::~CDataObject(
    void
    )
{
    DBGTRACE((DM_SNAPIN, DL_MID, TEXT("CDataObject::~CDataObject")));

}



HRESULT
CDataObject::QueryInterface(
    REFIID riid, 
    LPVOID *ppvOut
    )
{
    DBGTRACE((DM_SNAPIN, DL_MID, TEXT("CDataObject::QueryInterface")));
    DBGPRINTIID(DM_SNAPIN, DL_MID, riid);

    HRESULT hr = E_NOINTERFACE;

    if (NULL == ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;

    if (IID_IDiskQuotaSnapInData == riid)
    {
        *ppvOut = reinterpret_cast<IQuotaDataObject *>(this);
    }
    else if(IID_IUnknown == riid || IID_IDataObject == riid)
    {
        *ppvOut = this;
    }

    if (NULL != *ppvOut)
    {
        ((LPUNKNOWN)*ppvOut)->AddRef();
        hr = NOERROR;
    }

    return hr;
}



ULONG
CDataObject::AddRef(
    void
    )
{
    DBGTRACE((DM_SNAPIN, DL_LOW, TEXT("CDataObject::AddRef")));

    ULONG ulReturn = m_cRef + 1;
    InterlockedIncrement(&m_cRef);
    return ulReturn;
}


ULONG
CDataObject::Release(
    void
    )
{
    DBGTRACE((DM_SNAPIN, DL_LOW, TEXT("CDataObject::Release")));

    ULONG ulReturn = m_cRef - 1;
    if (InterlockedDecrement(&m_cRef) == 0)
    {   
        delete this;
        ulReturn = 0;
    }
    return ulReturn;
}



HRESULT
CDataObject::GetDataHere(
    LPFORMATETC lpFormatetc, 
    LPSTGMEDIUM lpMedium
    )
{
    DBGTRACE((DM_SNAPIN, DL_HIGH, TEXT("CDataObject::GetDataHere")));
    HRESULT hr = S_OK;
    try
    {
        //
        // Forward the rendering request to the data object's item object.
        // 
        hr = m_pItem->RenderData(lpFormatetc->cfFormat, lpMedium);
    }
    catch(CAllocException& e)
    {
        DBGERROR((TEXT("Insufficient memory")));
        hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
        DBGERROR((TEXT("C++ exception caught")));
        hr = E_UNEXPECTED;
    }
    return hr;
}


#endif // POLICY_MMC_SNAPIN
