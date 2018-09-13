//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       idldata.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop


#include "fmtetc.h"
#include "idldata.h"
#include "shsemip.h"


CLIPFORMAT CIDLData::m_rgcfGlobal[ICF_MAX] = { CF_HDROP, 0 };
const LARGE_INTEGER CIDLData::m_LargeIntZero;

//
// For those who prefer a function (rather than a ctor) to create an object,
// this static function will return a pointer to the IDataObject interface.
// If the function fails, no object is created.
//
HRESULT 
CIDLData::CreateInstance(
    IDataObject **ppOut,
    LPCITEMIDLIST pidlFolder,
    UINT cidl,
    LPCITEMIDLIST *apidl,
    IShellFolder *psfOwner,
    IDataObject *pdtInner
    )
{
    CIDLData *pidlData;
    HRESULT hr = CreateInstance(&pidlData, pidlFolder, cidl, apidl, psfOwner, pdtInner);
    if (SUCCEEDED(hr))
    {
        pidlData->AddRef();
        hr = pidlData->QueryInterface(IID_IDataObject, (void **)ppOut);
        pidlData->Release();
    }
    else
    {
        *ppOut = NULL;
    }
    return hr;
}


//
// For those who prefer a function (rather than a ctor) to create an object,
// this static function will return a pointer to the CIDLData object.
// If the function fails, no object is created.  Note that the returned object
// has a ref count of 0.  Therefore, it acts as a normal C++ object.  If you 
// want it to participate as a COM object, QI for IDataObject or use the 
// IDataObject version of CreateInstance() above.
//
HRESULT 
CIDLData::CreateInstance(
    CIDLData **ppOut,
    LPCITEMIDLIST pidlFolder,
    UINT cidl,
    LPCITEMIDLIST *apidl,
    IShellFolder *psfOwner,
    IDataObject *pdtInner
    )
{
    HRESULT hr = E_OUTOFMEMORY;
    CIDLData *pidlData = new CIDLData(pidlFolder, cidl, apidl, psfOwner, pdtInner);
    if (NULL != pidlData)
    {
        hr = pidlData->CtorResult();
        if (SUCCEEDED(hr))
        {
            *ppOut = pidlData;
        }
        else
        {
            delete pidlData;
        }
    }
    return hr;
}


CIDLData::CIDLData(
    LPCITEMIDLIST pidlFolder, 
    UINT cidl, 
    LPCITEMIDLIST *apidl, 
    IShellFolder *psfOwner,       // Optional.  Default is NULL.
    IDataObject *pdtobjInner      // Optional.  Default is NULL.
    ) : m_cRef(0),
        m_hrCtor(NOERROR),
        m_psfOwner(NULL),
        m_dwOwnerData(0),
        m_pdtobjInner(pdtobjInner),
        m_bEnumFormatCalled(false)
{
    //
    // Initialize the global clipboard formats.
    //
    InitializeClipboardFormats();

    ZeroMemory(m_rgMedium, sizeof(m_rgMedium));
    ZeroMemory(m_rgFmtEtc, sizeof(m_rgFmtEtc));

    if (NULL != m_pdtobjInner)
        m_pdtobjInner->AddRef();

    //
    // Empty array is valid input.
    //
    if (NULL != apidl)
    {
        HIDA hida = HIDA_Create(pidlFolder, cidl, apidl);
        if (NULL != hida)
        {
            m_hrCtor = DataObject_SetGlobal(static_cast<IDataObject *>(this), g_cfHIDA, hida);
            if (SUCCEEDED(m_hrCtor))
            {
                if (NULL != psfOwner)
                {
                    m_psfOwner = psfOwner;
                    m_psfOwner->AddRef();
                }
            }
        }
        else
        {
            m_hrCtor = E_OUTOFMEMORY;
        }
    }
}


CIDLData::~CIDLData(
    void
    )
{
    for (int i = 0; i < ARRAYSIZE(m_rgMedium); i++)
    {
        if (m_rgMedium[i].hGlobal)
            ReleaseStgMedium(&(m_rgMedium[i]));
    }

    if (NULL != m_psfOwner)
        m_psfOwner->Release();

    if (NULL != m_pdtobjInner)
        m_pdtobjInner->Release();
}



STDMETHODIMP 
CIDLData::QueryInterface(
    REFIID riid, 
    void **ppv
    )
{
    static const QITAB qit[] = {
        QITABENT(CIDLData, IDataObject),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


STDMETHODIMP_(ULONG)
CIDLData::AddRef(
    void
    )
{
    return InterlockedIncrement(&m_cRef);
}



STDMETHODIMP_(ULONG) 
CIDLData::Release(
    void
    )
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


HRESULT
CIDLData::GetData(
    FORMATETC *pFmtEtc, 
    STGMEDIUM *pMedium
    )
{
    HRESULT hr = E_INVALIDARG;

    pMedium->hGlobal        = NULL;
    pMedium->pUnkForRelease = NULL;

    for (int i = 0; i < ARRAYSIZE(m_rgFmtEtc); i++)
    {
        if ((m_rgFmtEtc[i].cfFormat == pFmtEtc->cfFormat) &&
            (m_rgFmtEtc[i].tymed & pFmtEtc->tymed) &&
            (m_rgFmtEtc[i].dwAspect == pFmtEtc->dwAspect))
        {
            *pMedium = m_rgMedium[i];

            if (NULL != pMedium->hGlobal)
            {
                //
                // Indicate that the caller should not release hmem.
                //
                if (TYMED_HGLOBAL == pMedium->tymed)
                {
                    InterlockedIncrement(&m_cRef);
                    pMedium->pUnkForRelease = static_cast<IUnknown *>(this);
                    return S_OK;
                }
                //
                // If the type is stream  then clone the stream.
                //
                if (TYMED_ISTREAM == pMedium->tymed)
                {
                    hr = CreateStreamOnHGlobal(NULL, TRUE, &(pMedium->pstm));

                    if (SUCCEEDED(hr))
                    {
                        STGMEDIUM& medium = m_rgMedium[i];
                        STATSTG stat;

                        //
                        // Get the Current Stream size
                        //
                        hr = medium.pstm->Stat(&stat, STATFLAG_NONAME);

                        if (SUCCEEDED(hr))
                        {
                            //
                            // Seek the source stream to the beginning.
                            //
                            medium.pstm->Seek(m_LargeIntZero, STREAM_SEEK_SET, NULL);
                            //
                            // Copy the entire source into the destination. 
                            // Since the destination stream is created using CreateStreamOnHGlobal, 
                            // it seek pointer is at the beginning.
                            //
                            hr = medium.pstm->CopyTo(pMedium->pstm, stat.cbSize, NULL, NULL);
                            //                            
                            // Before returning Set the destination seek pointer back at the beginning.
                            //
                            pMedium->pstm->Seek(m_LargeIntZero, STREAM_SEEK_SET, NULL);
                            return hr;
                         }
                         else
                         {
                             hr = E_OUTOFMEMORY;
                         }

                    }
                }
            }
        }
    }

    if (E_INVALIDARG == hr && NULL != m_pdtobjInner) 
    {
        hr = m_pdtobjInner->GetData(pFmtEtc, pMedium);
    }

    return hr;
}



STDMETHODIMP 
CIDLData::GetDataHere(
    FORMATETC *pFmtEtc, 
    STGMEDIUM *pMedium
    )
{
    HRESULT hr = E_NOTIMPL;

    if (NULL != m_pdtobjInner) 
    {
        hr = m_pdtobjInner->GetDataHere(pFmtEtc, pMedium);
    }

    return hr;
}



HRESULT
CIDLData::QueryGetData(
    FORMATETC *pFmtEtc
    )
{
    HRESULT hr = S_FALSE;

    for (int i = 0; i < ARRAYSIZE(m_rgFmtEtc); i++)
    {
        if ((m_rgFmtEtc[i].cfFormat == pFmtEtc->cfFormat) &&
            (m_rgFmtEtc[i].tymed & pFmtEtc->tymed) &&
            (m_rgFmtEtc[i].dwAspect == pFmtEtc->dwAspect))
        {
            return S_OK;
        }
    }

    if (NULL != m_pdtobjInner)
    {
        hr = m_pdtobjInner->QueryGetData(pFmtEtc);
    }
    return hr;
}



STDMETHODIMP 
CIDLData::GetCanonicalFormatEtc(
    FORMATETC *pFmtEtcIn, 
    FORMATETC *pFmtEtcOut
    )
{
    //
    // This is the simplest implemtation. It means we always return
    // the data in the format requested.
    //
    return DATA_S_SAMEFORMATETC;
}



STDMETHODIMP 
CIDLData::SetData(
    FORMATETC *pFmtEtc, 
    STGMEDIUM *pMedium, 
    BOOL fRelease
    )
{
    HRESULT hr;

    TraceAssert(pFmtEtc->tymed == pMedium->tymed);

    if (fRelease)
    {
        int i;
        //
        // First add it if that format is already present
        // on a NULL medium (render on demand)
        //
        for (i = 0; i < ARRAYSIZE(m_rgFmtEtc); i++)
        {
            if ((m_rgFmtEtc[i].cfFormat == pFmtEtc->cfFormat) &&
                (m_rgFmtEtc[i].tymed    == pFmtEtc->tymed) &&
                (m_rgFmtEtc[i].dwAspect == pFmtEtc->dwAspect))
            {
                //
                // We are simply adding a format, ignore.
                //
                if (NULL == pMedium->hGlobal) 
                {
                    return S_OK;
                }

                //
                // If we are set twice on the same object
                //
                if (NULL != m_rgMedium[i].hGlobal)
                    ReleaseStgMedium(&m_rgMedium[i]);

                m_rgMedium[i] = *pMedium;
                return S_OK;
            }
        }
        //
        // now look for a free slot
        //
        for (i = 0; i < ARRAYSIZE(m_rgFmtEtc); i++)
        {
            if (0 == m_rgFmtEtc[i].cfFormat)
            {
                //
                // found a free slot
                //
                m_rgMedium[i] = *pMedium;
                m_rgFmtEtc[i] = *pFmtEtc;
                return S_OK;
            }
        }
        //
        // fixed size table
        //
        hr = E_OUTOFMEMORY;
    }
    else
        hr = E_INVALIDARG;

    return hr;
}


STDMETHODIMP 
CIDLData::EnumFormatEtc(
    DWORD dwDirection, 
    LPENUMFORMATETC *ppenumFormatEtc
    )
{
    HRESULT hr = NOERROR;
    //
    // If this is the first time, build the format list by calling
    // QueryGetData with each clipboard format.
    //
    if (!m_bEnumFormatCalled)
    {
        FORMATETC fmte = { 0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        STGMEDIUM medium = { TYMED_HGLOBAL, NULL, NULL };
        for (int i = 0; i < ARRAYSIZE(m_rgcfGlobal); i++)
        {
            fmte.cfFormat = m_rgcfGlobal[i];
            if (S_OK == QueryGetData(&fmte)) 
            {
                SetData(&fmte, &medium, TRUE);
            }
        }
        m_bEnumFormatCalled = true;
    }
    //
    // Get the number of formatetc
    //
    UINT cfmt;
    for (cfmt = 0; cfmt < ARRAYSIZE(m_rgFmtEtc); cfmt++)
    {
        if (0 == m_rgFmtEtc[cfmt].cfFormat)
            break;
    }
/*
    return SHCreateStdEnumFmtEtcEx(cfmt, m_rgFmtEtc, m_pdtobjInner, ppenumFormatEtc);
*/

    CEnumFormatEtc *pEnumFmtEtc = new CEnumFormatEtc(cfmt, m_rgFmtEtc);
    if (NULL != pEnumFmtEtc)
    {
        pEnumFmtEtc->AddRef();
        //
        // Ask derived classes to add their formats.
        //
        hr = ProvideFormats(pEnumFmtEtc);
        if (SUCCEEDED(hr))
        {
            hr = pEnumFmtEtc->QueryInterface(IID_IEnumFORMATETC, (void **)ppenumFormatEtc);
        }
        pEnumFmtEtc->Release();
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}


HRESULT 
CIDLData::ProvideFormats(
    CEnumFormatEtc *pEnumFmtEtc
    )
{
    //
    // Base class default does nothing.  Our formats are added to the enumerator
    // in EnumFormatEtc().
    //
    return NOERROR;
}


STDMETHODIMP 
CIDLData::DAdvise(
    FORMATETC *pFmtEtc, 
    DWORD advf, 
    LPADVISESINK pAdvSink, 
    DWORD *pdwConnection
    )
{
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP 
CIDLData::DUnadvise(
    DWORD dwConnection
    )
{
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP 
CIDLData::EnumDAdvise(
    LPENUMSTATDATA *ppenumAdvise
    )
{
    return OLE_E_ADVISENOTSUPPORTED;
}

BOOL
CIDLData::IsOurs(
    IDataObject *pdtobj
    ) const
{ 
    if (NULL != pdtobj)
    {
        CIDLData *pNonConstThis = const_cast<CIDLData *>(this);
        return ((static_cast<IUnknown *>(pNonConstThis))->QueryInterface) == 
               ((static_cast<IUnknown *>(pdtobj))->QueryInterface);
    }

    return FALSE;
}


IShellFolder *
CIDLData::GetFolder(
    void
    ) const
{ 
    return m_psfOwner; 
}



//
// Clone DataObject only for MOVE/COPY operation
//
HRESULT
CIDLData::Clone(
    UINT *acf, 
    UINT ccf, 
    IDataObject **ppdtobjOut
    )
{
    HRESULT hr = NOERROR;
    CIDLData *pidlData = new CIDLData(NULL, 0, NULL);
    if (NULL == pidlData)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        FORMATETC fmte = { 0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        for (UINT i = 0; i < ccf; i++)
        {
            HRESULT hrT;
            STGMEDIUM medium;
            fmte.cfFormat = (CLIPFORMAT) acf[i];
            hrT = GetData(&fmte, &medium);
            if (SUCCEEDED(hrT))
            {
                HGLOBAL hmem;
                if (NULL != medium.pUnkForRelease)
                {
                    //
                    // We need to clone the hGlobal.
                    //
                    SIZE_T cbMem =  GlobalSize(medium.hGlobal);
                    hmem = GlobalAlloc(GPTR, cbMem);
                    if (NULL != hmem)
                    {
                        hmemcpy((LPVOID)hmem, GlobalLock(medium.hGlobal), cbMem);
                        GlobalUnlock(medium.hGlobal);
                    }
                    ReleaseStgMedium(&medium);
                }
                else
                {
                    //
                    // We don't need to clone the hGlobal.
                    //
                    hmem = medium.hGlobal;
                }

                if (hmem)
                    DataObject_SetGlobal(*ppdtobjOut, (CLIPFORMAT)acf[i], hmem);
            }
        }
    }
    return hr;
}


HRESULT 
CIDLData::CloneForMoveCopy(
    IDataObject **ppdtobjOut
    )
{
    return E_NOTIMPL;
    /*
    UINT acf[] = { g_cfHIDA, g_cfOFFSETS, CF_HDROP, g_cfFileNameMapW, g_cfFileNameMap };

    return Clone(acf, ARRAYSIZE(acf), ppdtobjOut);
    */
}


#define RCF(x)  (CLIPFORMAT) RegisterClipboardFormat(x)

void 
CIDLData::InitializeClipboardFormats(
    void
    )
{
    if (g_cfHIDA == 0)
    {
        g_cfHIDA                 = RCF(CFSTR_SHELLIDLIST);
        g_cfOFFSETS              = RCF(CFSTR_SHELLIDLISTOFFSET);
        g_cfNetResource          = RCF(CFSTR_NETRESOURCES);
        g_cfFileContents         = RCF(CFSTR_FILECONTENTS);         // "FileContents"
        g_cfFileGroupDescriptorA = RCF(CFSTR_FILEDESCRIPTORA);      // "FileGroupDescriptor"
        g_cfFileGroupDescriptorW = RCF(CFSTR_FILEDESCRIPTORW);      // "FileGroupDescriptor"
        g_cfPrivateShellData     = RCF(CFSTR_SHELLIDLISTP);
        g_cfFileName             = RCF(CFSTR_FILENAMEA);            // "FileName"
        g_cfFileNameW            = RCF(CFSTR_FILENAMEW);            // "FileNameW"
        g_cfFileNameMap          = RCF(CFSTR_FILENAMEMAP);          // "FileNameMap"
        g_cfFileNameMapW         = RCF(CFSTR_FILENAMEMAPW);         // "FileNameMapW"
        g_cfPrinterFriendlyName  = RCF(CFSTR_PRINTERGROUP);
        g_cfHTML                 = RCF(TEXT("HTML Format"));
        g_cfPreferredDropEffect  = RCF(CFSTR_PREFERREDDROPEFFECT);  // "Preferred DropEffect"
        g_cfPerformedDropEffect  = RCF(CFSTR_PERFORMEDDROPEFFECT);  // "Performed DropEffect"
        g_cfLogicalPerformedDropEffect = RCF(CFSTR_LOGICALPERFORMEDDROPEFFECT);
        g_cfShellURL             = RCF(CFSTR_SHELLURL);             // "Uniform Resource Locator"
        g_cfInDragLoop           = RCF(CFSTR_INDRAGLOOP);           // "InShellDragLoop"
        g_cfDragContext          = RCF(CFSTR_DRAGCONTEXT);          // "DragContext"
        g_cfTargetCLSID          = RCF(TEXT("TargetCLSID"));        // who the drag drop went to
    }
}


//
// This is normally a private shell function.
//
#define HIDA_GetPIDLItem(pida, i)       (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])

CIDLData::HIDA
CIDLData::HIDA_Create(
    LPCITEMIDLIST pidlFolder, 
    UINT cidl, 
    LPCITEMIDLIST *apidl
    )
{
    HIDA hida;
#if _MSC_VER == 1100
// BUGBUG: Workaround code generate bug in VC5 X86 compiler (12/30 version).
    volatile
#endif
    UINT i;
    UINT offset = SIZEOF(CIDA) + SIZEOF(UINT)*cidl;
    UINT cbTotal = offset + ILGetSize(pidlFolder);
    for (i=0; i<cidl ; i++) {
        cbTotal += ILGetSize(apidl[i]);
    }

    hida = GlobalAlloc(GPTR, cbTotal);  // This MUST be GlobalAlloc!!!
    if (hida)
    {
        LPIDA pida = (LPIDA)hida;       // no need to lock

        LPCITEMIDLIST pidlNext;
        pida->cidl = cidl;

        for (i=0, pidlNext=pidlFolder; ; pidlNext=apidl[i++])
        {
            UINT cbSize = ILGetSize(pidlNext);
            pida->aoffset[i] = offset;
            MoveMemory(((LPBYTE)pida)+offset, pidlNext, cbSize);
            offset += cbSize;

            TraceAssert(ILGetSize(HIDA_GetPIDLItem(pida,i-1)) == cbSize);

            if (i==cidl)
                break;
        }

        TraceAssert(offset == cbTotal);
    }

    return hida;
}

