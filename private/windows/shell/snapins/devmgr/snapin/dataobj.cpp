/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    dataobj.cpp

Abstract:

    header file defines CDataObject class

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "devmgr.h"
#include "DataObj.h"


unsigned int CDataObject::m_cfNodeType       = RegisterClipboardFormat(CCF_NODETYPE);
unsigned int CDataObject::m_cfNodeTypeString = RegisterClipboardFormat(CCF_SZNODETYPE);
unsigned int CDataObject::m_cfDisplayName    = RegisterClipboardFormat(CCF_DISPLAY_NAME);
unsigned int CDataObject::m_cfCoClass        = RegisterClipboardFormat(CCF_SNAPIN_CLASSID);
unsigned int CDataObject::m_cfSnapinInternal    = RegisterClipboardFormat(SNAPIN_INTERNAL);
unsigned int CDataObject::m_cfMachineName    = RegisterClipboardFormat(MMC_SNAPIN_MACHINE_NAME);
unsigned int CDataObject::m_cfClassGuid      = RegisterClipboardFormat(DEVMGR_SNAPIN_CLASS_GUID);
unsigned int CDataObject::m_cfDeviceID       = RegisterClipboardFormat(DEVMGR_SNAPIN_DEVICE_ID);

//
// IUnknown interface implementation
//

ULONG
CDataObject::AddRef()
{
    ::InterlockedIncrement((LONG*)&m_Ref);
    return m_Ref;
}

ULONG
CDataObject::Release()
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
CDataObject::QueryInterface(
    REFIID riid,
    void** ppv
    )
{
    if (!ppv)
    return E_INVALIDARG;
    HRESULT hr = S_OK;

    if (IsEqualIID(riid, IID_IUnknown))
    *ppv = (IUnknown*)this;
    else if (IsEqualIID(riid, IID_IDataObject))
    {
    *ppv = this;
    }
    else
    {
    hr = E_NOINTERFACE;
    }
    if (SUCCEEDED(hr))
    AddRef();
    else
    *ppv = NULL;
    return hr;
}

HRESULT
CDataObject::Initialize(
    DATA_OBJECT_TYPES Type,
    COOKIE_TYPE      ct,
    CCookie* pCookie,
    String& strMachineName
    )
{
    try
    {
    m_strMachineName = strMachineName;
    m_pCookie = pCookie;
    m_Type = Type;
    m_ct = ct;
    }
    catch (CMemoryException* e)
    {
    e->Delete();
    return E_OUTOFMEMORY;
    }
    return S_OK;

}


STDMETHODIMP
CDataObject::GetDataHere(
    LPFORMATETC lpFormatetc,
    LPSTGMEDIUM lpMedium
    )
{
    HRESULT hr = S_OK;


    try
    {
    const CLIPFORMAT cf = lpFormatetc->cfFormat;
    HRESULT hr = DV_E_FORMATETC;
    SafeInterfacePtr<IStream> StreamPtr;

    if (TYMED_HGLOBAL == lpMedium->tymed)
    {
        ULONG ulWritten;
        hr = CreateStreamOnHGlobal(lpMedium->hGlobal, FALSE, &StreamPtr);
        if (S_OK == hr)
        {
        const NODEINFO* pni = &NodeInfo[m_ct];
        ASSERT(pni->ct == m_ct);

        if (cf == m_cfNodeType)
        {
            const GUID* pGuid = &pni->Guid;
            hr = StreamPtr->Write(pGuid, sizeof(GUID), &ulWritten);
        }
        else if (cf == m_cfNodeTypeString)
        {
            const TCHAR *pszGuid = pni->GuidString;
            hr = StreamPtr->Write(pszGuid,
                     (wcslen(pszGuid) + 1) * sizeof(TCHAR),
                     &ulWritten
                     );
        }
        else if (cf == m_cfDisplayName)
        {
            if (pni->idsFormat)
            {
            String strDisplayName;
            TCHAR Format[LINE_LEN];
            TCHAR LocalMachine[LINE_LEN];
            ::LoadString(g_hInstance, pni->idsFormat, Format, sizeof(Format)/sizeof(TCHAR));
            LPCTSTR MachineName = m_strMachineName;
            if (m_strMachineName.IsEmpty())
            {
                ::LoadString(g_hInstance, IDS_LOCAL_MACHINE, LocalMachine,
                     sizeof(LocalMachine) / sizeof(TCHAR));
                MachineName = LocalMachine;
            }
            strDisplayName.Format(Format, MachineName);
            hr = StreamPtr->Write(strDisplayName,
                     (strDisplayName.GetLength() + 1) * sizeof(TCHAR),
                     &ulWritten
                     );
            }
        }
        else if (cf == m_cfSnapinInternal)
        {
            INTERNAL_DATA tID;
            tID.ct = m_ct;
            tID.dot = m_Type;
            tID.cookie = (MMC_COOKIE)m_pCookie;
            hr = StreamPtr->Write(&tID,
                     sizeof(INTERNAL_DATA),
                     &ulWritten
                     );
        }
        else if (cf == m_cfCoClass)
        {
            hr = StreamPtr->Write(&CLSID_DEVMGR,
                     sizeof(CLSID),
                     &ulWritten
                     );
        }
        else if (cf == m_cfMachineName)
        {
            if (!m_strMachineName.IsEmpty())
            {
            hr = StreamPtr->Write((LPCTSTR)m_strMachineName,
                      (m_strMachineName.GetLength()+1) * sizeof(TCHAR),
                      NULL);
            }
            else
            {
            TCHAR Nothing[1];
            Nothing[0] = _T('\0');
            hr = StreamPtr->Write(Nothing, sizeof(Nothing), NULL);
            }
        }
        else if (cf == m_cfClassGuid)
        {
            if (COOKIE_TYPE_RESULTITEM_CLASS == m_pCookie->GetType())
            {
            CClass* pClass = (CClass*)m_pCookie->GetResultItem();
            ASSERT(pClass);
            LPGUID pClassGuid = *pClass;
            hr = StreamPtr->Write(pClassGuid, sizeof(GUID), NULL);
            }
        }
        else if (cf == m_cfDeviceID)
        {
            if (COOKIE_TYPE_RESULTITEM_DEVICE == m_pCookie->GetType())
            {
            CDevice* pDevice = (CDevice*)m_pCookie->GetResultItem();
            ASSERT(pDevice);
            LPCTSTR DeviceID = pDevice->GetDeviceID();

            hr = StreamPtr->Write(DeviceID,
                     (wcslen(DeviceID) + 1) * sizeof(TCHAR),
                     NULL
                     );
            }

        }
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
CDataObject::GetData(
    LPFORMATETC lpFormatetc,
    LPSTGMEDIUM lpMedium
    )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CDataObject::EnumFormatEtc(
    DWORD dwDirection,
    LPENUMFORMATETC* ppEnumFormatEtc
    )
{
    return E_NOTIMPL;
}




HRESULT ExtractData(
    IDataObject* pIDataObject,
    unsigned int cfClipFormat,
    BYTE*    pBuffer,
    DWORD    cbBuffer
    )
{

    if (NULL == pIDataObject || NULL == pBuffer)
    return E_POINTER;

    HRESULT hr = S_OK;
    FORMATETC FormatEtc = {(CLIPFORMAT)cfClipFormat, NULL, DVASPECT_CONTENT, -1 , TYMED_HGLOBAL};
    STGMEDIUM StgMedium = {TYMED_HGLOBAL, NULL};

    StgMedium.hGlobal = ::GlobalAlloc(GMEM_SHARE, cbBuffer);
    if (NULL == StgMedium.hGlobal) {
    ASSERT(FALSE);
    hr = E_OUTOFMEMORY;
    }
    else {
    hr = pIDataObject->GetDataHere(&FormatEtc, &StgMedium);
    if (SUCCEEDED(hr)) {
        BYTE* pData = reinterpret_cast<BYTE*>(::GlobalLock(StgMedium.hGlobal));
        if (NULL == pData) {
        ASSERT(FALSE);
        hr = E_UNEXPECTED;
        }
        else {
        ::memcpy(pBuffer, pData, cbBuffer);
                ::GlobalUnlock(StgMedium.hGlobal);
        }
    }
        ::GlobalFree(StgMedium.hGlobal);
    }
    return hr;
}

#if 0

/////////////////////////////////////////////////////////////////////
////
/// Helper functions to extact data from the given IDataObject
///

HRESULT ExtractData(IDataObject* piDataObject,
            unsigned int cfClipFormat,
            BYTE*    pBuffer,
            DWORD    cbBuffer
            );
HRESULT ExtractString(IDataObject* piDataObject,
              unsigned int cfClipFormat,
              String* pstr,
              DWORD    cchMax
              );



HRESULT
ExtractData(
    IDataObject* piDataObject,
    unsigned int cfClipFormat,
    BYTE*    pBuffer,
    DWORD    cbBuffer
    )
{
    HRESULT hr = S_OK;
    FORMATETC Formatetc = {cfClipFormat, NULL, DVASPET_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM Stgmedium = {TYMED_HGLOBAL, NULL};
    Stgmedium.hGlobal = ::GlobalAlloc(GMEM_SHARE, cbBuffer);

    if (NULL == Stgmedium.hGlobal) {
    ASSERT(FASLE);
    AfxThrowMemoryException();
    hr = E_OUTOFMEMORY;
    return hr;
    }
    hr = piDataObject->GetDataHere(&Formatetc, &Stgmedium);
    if (FAILED(hr)) {
    ASSERT(FALSE);
    return hr;
    }
    BYTE* pData = reinterpret_cast<BYTE*>(::GlobalLock(Stgmedium.hGlobal));
    if (NULL == pData) {
    ASSERT(FALSE);
    hr = E_UNEXPECTED;
    }
    ::memcpy(pBuffer, pData, cbBuffer);
    return hr;
}

HRESULT
ExtractString(
    IDataObject* piDataObject,
    unsigned int cfClipFormat,
    String* pstr
    DWORD   cbMax
    )
{
    FORMATETC Formatetc = {cfClipFormat, NULL, DVASPET_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM Stgmedium = {TYMED_HGLOBAL, NULL};
    Stgmedium.hGlobal = ::GlobalAlloc(GMEM_SHARE, cbBuffer);

    if (NULL == Stgmedium.hGlobal) {
    ASSERT(FASLE);
    AfxThrowMemoryException();
    hr = E_OUTOFMEMORY;
    return hr;
    }
    HRESULT hr = piDataObject->GetDataHere(&Formatetc, &Stgmedium);
    if (FAILED(hr)) {
    ASSERT(FALSE);
    return hr;
    }
    LPTSTR pszData = reinterpret_cast<LPTSTR>(::GlobalLock(Stgmedium.hGlobal));
    if (NULL == pszData) {
    ASSERT(FALSE);
    return E_UNEXPECTTED;
    }
    USES_CONVERSION;
    *pstr = OLE2T(pszData);
    return S_OK;
}
#endif
