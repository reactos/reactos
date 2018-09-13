#include "main.h"


const wchar_t* PWRADMIN_INTERNAL = L"PWRADMIN_INTERNAL";

///////////////////////////////////////////////////////////////////////////////
//
// Snap-in NodeType in both GUID format and string format
//
///////////////////////////////////////////////////////////////////////////////

unsigned int CDataObject::m_cfNodeType       = RegisterClipboardFormat(CCF_NODETYPE);
unsigned int CDataObject::m_cfNodeTypeString = RegisterClipboardFormat(CCF_SZNODETYPE);
unsigned int CDataObject::m_cfDisplayName    = RegisterClipboardFormat(CCF_DISPLAY_NAME);
unsigned int CDataObject::m_cfCoClass        = RegisterClipboardFormat(CCF_SNAPIN_CLASSID);
unsigned int CDataObject::m_cfInternal       = RegisterClipboardFormat(PWRADMIN_INTERNAL);

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CDataObject implementation                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


CDataObject::CDataObject()
{
    m_cRef++;
    InterlockedIncrement(&g_cRefThisDll);
}

CDataObject::~CDataObject()
{
    InterlockedDecrement(&g_cRefThisDll);
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CDataObject object implementation (IUnknown)                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


HRESULT CDataObject::QueryInterface (REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IDataObject) || IsEqualIID(riid, IID_IUnknown))
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

ULONG CDataObject::AddRef (void)
{
    return ++m_cRef;
}

ULONG CDataObject::Release (void)
{
    if (--m_cRef == 0) {
        delete this;
        return 0;
    }

    return m_cRef;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CDataObject object implementation (IDataObject)                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDataObject::GetDataHere(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium)
{
    HRESULT hr = DV_E_CLIPFORMAT;

    // Based on the CLIPFORMAT write data to the stream
    const CLIPFORMAT cf = lpFormatetc->cfFormat;

    if(cf == m_cfNodeType)
    {
        hr = CreateNodeTypeData(lpMedium);
    }
    else if (cf == m_cfCoClass)
    {
        hr = CreateCoClassID(lpMedium);
    }
    else if(cf == m_cfNodeTypeString)
    {
        hr = CreateNodeTypeStringData(lpMedium);
    }
    else if (cf == m_cfDisplayName)
    {
        hr = CreateDisplayName(lpMedium);
    }
    else if (cf == m_cfInternal)
    {
        hr = CreateInternal(lpMedium);
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CDataObject object implementation (Internal functions)                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT CDataObject::Create(LPVOID pBuffer, INT len, LPSTGMEDIUM lpMedium)
{
    HRESULT hr = DV_E_TYMED;

    // Do some simple validation
    if (pBuffer == NULL || lpMedium == NULL)
        return E_POINTER;

    // Make sure the type medium is HGLOBAL
    if (lpMedium->tymed == TYMED_HGLOBAL)
    {
        // Create the stream on the hGlobal passed in
        LPSTREAM lpStream;
        hr = CreateStreamOnHGlobal(lpMedium->hGlobal, FALSE, &lpStream);

        if (SUCCEEDED(hr))
        {
            // Write to the stream the number of bytes
            unsigned long written;

            hr = lpStream->Write(pBuffer, len, &written);

            // Because we told CreateStreamOnHGlobal with 'FALSE',
            // only the stream is released here.
            // Note - the caller (i.e. snap-in, object) will free the HGLOBAL
            // at the correct time.  This is according to the IDataObject specification.
            lpStream->Release();
        }
    }

    return hr;
}

HRESULT CDataObject::CreateNodeTypeData(LPSTGMEDIUM lpMedium)
{
    // Create the node type object in GUID format
    return Create((LPVOID)&NodeType, sizeof(GUID), lpMedium);
}

HRESULT CDataObject::CreateNodeTypeStringData(LPSTGMEDIUM lpMedium)
{
    // Create the node type object in GUID string format
    return Create((LPVOID)szNodeType, ((lstrlenW(szNodeType)+1) * sizeof(WCHAR)), lpMedium);
}

HRESULT CDataObject::CreateDisplayName(LPSTGMEDIUM lpMedium)
{
    WCHAR  szDispName[50];

    //
    // This is the display named used in the scope pane and snap-in manager
    //

    LoadStringW (g_hInstance, IDS_DESCRIPTION, szDispName, 50);

    return Create((LPVOID)szDispName, (lstrlenW(szDispName) + 1) * sizeof(WCHAR), lpMedium);
}

HRESULT CDataObject::CreateInternal(LPSTGMEDIUM lpMedium)
{

    return Create(&m_internal, sizeof(INTERNAL), lpMedium);
}

HRESULT CDataObject::CreateCoClassID(LPSTGMEDIUM lpMedium)
{
    // Create the CoClass information
    return Create((LPVOID)&m_internal.m_clsid, sizeof(CLSID), lpMedium);
}
