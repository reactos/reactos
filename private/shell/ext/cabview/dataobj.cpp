//*******************************************************************************************
//
// Filename : DataObj.cpp
//    
// Implementation file for CObjFormats and CCabObj
//
// Copyright (c) 1994 - 1996 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************

#include "pch.h"

#include "thisdll.h"

#include "folder.h"
#include "dataobj.h"

#include "cabitms.h"

UINT CCabObj::s_uFileGroupDesc = 0;
UINT CCabObj::s_uFileContents = 0;
UINT CCabObj::s_uPersistedDataObject = 0;

// {dfe49cfe-cd09-11d2-9643-00c04f79adf0}
const GUID CLSID_CabViewDataObject = {0xdfe49cfe, 0xcd09, 0x11d2, 0x96, 0x43, 0x00, 0xc0, 0x4f, 0x79, 0xad, 0xf0};

#define    MAX_CHUNK    (60*1024)    /* max mouthful to CopyTo from CCabStream */

class CCabStream : public IStream
{
public:
    CCabStream(HGLOBAL hStream,DWORD dwStreamLength);
    ~CCabStream(void);

    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // *** IStream methods ***
    STDMETHODIMP Read(void *pv,ULONG cb,ULONG *pcbRead);
    STDMETHODIMP Write(const void *pv,ULONG cb,ULONG *pcbWritten);
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER * plibNewPosition);
    STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize);
    STDMETHODIMP CopyTo(IStream * pstm,    ULARGE_INTEGER cb, ULARGE_INTEGER * pcbRead, ULARGE_INTEGER * pcbWritten);
    STDMETHODIMP Commit(DWORD grfCommitFlags);
    STDMETHODIMP Revert(void);
    STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHODIMP Stat(STATSTG * pstatstg, DWORD grfStatFlag);
    STDMETHODIMP Clone(IStream ** ppstm);

private:
    CRefCount m_cRef;
    CRefDll m_cRefDll;
    HGLOBAL m_hStream;
    DWORD m_dwStreamLength;
    DWORD m_dwStreamPosition;
};

CCabStream::CCabStream(HGLOBAL hStream,DWORD dwStreamLength)
{
    m_hStream = hStream;
    m_dwStreamLength = dwStreamLength;
    m_dwStreamPosition = 0;

    AddRef();
}

CCabStream::~CCabStream(void)
{
    GlobalFree(m_hStream);
}

STDMETHODIMP CCabStream::QueryInterface(
   REFIID riid, 
   LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    LPUNKNOWN pObj;
 
    if (riid == IID_IUnknown)
    {
        pObj = (IUnknown*)((IStream *)this); 
    }
    else if (riid == IID_IStream)
    {
        pObj = (IUnknown*)((IStream *)this); 
    }
    else
    {
           return(E_NOINTERFACE);
    }

    pObj->AddRef();
    *ppvObj = pObj;

    return(NOERROR);
}

STDMETHODIMP_(ULONG) CCabStream::AddRef(void)
{
    return(m_cRef.AddRef());
}

STDMETHODIMP_(ULONG) CCabStream::Release(void)
{
    if (!m_cRef.Release())
    {
           delete this;
        return(0);
    }

    return(m_cRef.GetRef());
}

STDMETHODIMP CCabStream::Read(void * pv, ULONG cb, ULONG * pcbRead)
{
    *pcbRead = 0;

    if (m_dwStreamPosition < m_dwStreamLength)
    {
        if (cb > (m_dwStreamLength - m_dwStreamPosition))
        {
            *pcbRead = (m_dwStreamLength - m_dwStreamPosition);
        }
        else
        {
            *pcbRead = cb;
        }

        CopyMemory(pv,(char *) m_hStream + m_dwStreamPosition,*pcbRead);
        m_dwStreamPosition += *pcbRead;
    }

    return(S_OK);
}

STDMETHODIMP CCabStream::Write(const void * pv, ULONG cb, ULONG * pcbWritten)
{
    return(E_NOTIMPL);
}

STDMETHODIMP CCabStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER * plibNewPosition)
{
    switch (dwOrigin)
    {
    case STREAM_SEEK_SET:
        m_dwStreamPosition = dlibMove.LowPart;
        break;

    case STREAM_SEEK_CUR:
        m_dwStreamPosition += dlibMove.LowPart;
        break;

    case STREAM_SEEK_END:
        m_dwStreamPosition = m_dwStreamLength + dlibMove.LowPart;
        break;

    default:
        return(STG_E_INVALIDFUNCTION);
    }

    if (plibNewPosition)
    {
        (*plibNewPosition).LowPart = m_dwStreamPosition;
        (*plibNewPosition).HighPart = 0;
    }

    return(S_OK);
}

STDMETHODIMP CCabStream::SetSize(ULARGE_INTEGER libNewSize)
{
    return(E_NOTIMPL);
}

STDMETHODIMP CCabStream::CopyTo(IStream * pstm,    ULARGE_INTEGER cb, ULARGE_INTEGER * pcbRead, ULARGE_INTEGER * pcbWritten)
{
    HRESULT hRes;
    unsigned long cbActual = cb.LowPart;
    unsigned long cbWritten;
    unsigned long cbChunk;

    if (pcbRead)
    {
        (*pcbRead).LowPart = 0;
        (*pcbRead).HighPart = 0;
    }

    if (pcbWritten)
    {
        (*pcbWritten).LowPart = 0;
        (*pcbWritten).HighPart = 0;
    }

    hRes = S_OK;

    if (m_dwStreamPosition < m_dwStreamLength)
    {
        if (cbActual > (m_dwStreamLength - m_dwStreamPosition))
        {
            cbActual = (m_dwStreamLength - m_dwStreamPosition);
        }

        while (cbActual)
        {
            if (cbActual > MAX_CHUNK)
            {
                cbChunk = MAX_CHUNK;
            }
            else
            {
                cbChunk = cbActual;
            }

            hRes = pstm->Write((char *) m_hStream + m_dwStreamPosition,cbChunk,&cbWritten);
            if (hRes)
            {
                break;
            }

            m_dwStreamPosition += cbChunk;

            if (pcbRead)
            {
                (*pcbRead).LowPart += cbChunk;
                (*pcbRead).HighPart = 0;
            }

            if (pcbWritten)
            {
                (*pcbWritten).LowPart += cbWritten;
                (*pcbWritten).HighPart = 0;
            }

            cbActual -= cbChunk;
        }
    }

    return(hRes);
}

STDMETHODIMP CCabStream::Commit(DWORD grfCommitFlags)
{
    return(E_NOTIMPL);
}

STDMETHODIMP CCabStream::Revert(void)
{
    return(E_NOTIMPL);
}

STDMETHODIMP CCabStream::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return(E_NOTIMPL);
}

STDMETHODIMP CCabStream::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return(E_NOTIMPL);
}

STDMETHODIMP CCabStream::Stat(STATSTG * pstatstg, DWORD grfStatFlag)
{
    return(E_NOTIMPL);
}

STDMETHODIMP CCabStream::Clone(IStream ** ppstm)
{
    return(E_NOTIMPL);
}



class CObjFormats : public IEnumFORMATETC
{
public:
    CObjFormats(UINT cfmt, const FORMATETC afmt[]);
    ~CObjFormats();

    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // *** IEnumFORMATETC methods ***
    STDMETHODIMP Next(ULONG celt, FORMATETC *rgelt, ULONG *pceltFethed);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumFORMATETC ** ppenum);

private:
    CRefCount m_cRef;

    CRefDll m_cRefDll;

    UINT m_iFmt;
    UINT m_cFmt;
    FORMATETC *m_aFmt;
} ;


CObjFormats::CObjFormats(UINT cfmt, const FORMATETC afmt[])
{
    m_iFmt = 0;
    m_cFmt = cfmt;
    m_aFmt = new FORMATETC[cfmt];

    if (m_aFmt)
    {
        CopyMemory(m_aFmt, afmt, cfmt*sizeof(afmt[0]));
    }
}


CObjFormats::~CObjFormats()
{
    if (m_aFmt)
    {
        delete m_aFmt;
    }
}


// *** IUnknown methods ***
STDMETHODIMP CObjFormats::QueryInterface(
   REFIID riid, 
   LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    if (!m_aFmt)
    {
        return(E_OUTOFMEMORY);
    }

    LPUNKNOWN pObj;
 
    if (riid == IID_IUnknown)
    {
        pObj = (IUnknown*)((IEnumFORMATETC*)this); 
    }
    else if (riid == IID_IEnumFORMATETC)
    {
        pObj = (IUnknown*)((IEnumFORMATETC*)this); 
    }
    else
    {
           return(E_NOINTERFACE);
    }

    pObj->AddRef();
    *ppvObj = pObj;

    return(NOERROR);
}


STDMETHODIMP_(ULONG) CObjFormats::AddRef(void)
{
    return(m_cRef.AddRef());
}


STDMETHODIMP_(ULONG) CObjFormats::Release(void)
{
    if (!m_cRef.Release())
    {
           delete this;
        return(0);
    }

    return(m_cRef.GetRef());
}


STDMETHODIMP CObjFormats::Next(ULONG celt, FORMATETC *rgelt, ULONG *pceltFethed)
{
    UINT cfetch;
    HRESULT hres = S_FALSE;    // assume less numbers

    if (m_iFmt < m_cFmt)
    {
        cfetch = m_cFmt - m_iFmt;
        if (cfetch >= celt)
        {
            cfetch = celt;
            hres = S_OK;
        }

        CopyMemory(rgelt, &m_aFmt[m_iFmt], cfetch * sizeof(FORMATETC));
        m_iFmt += cfetch;
    }
    else
    {
        cfetch = 0;
    }

    if (pceltFethed)
    {
        *pceltFethed = cfetch;
    }

    return hres;
}

STDMETHODIMP CObjFormats::Skip(ULONG celt)
{
    m_iFmt += celt;
    if (m_iFmt > m_cFmt)
    {
        m_iFmt = m_cFmt;
        return S_FALSE;
    }
    return S_OK;
}

STDMETHODIMP CObjFormats::Reset()
{
    m_iFmt = 0;
    return S_OK;
}

STDMETHODIMP CObjFormats::Clone(IEnumFORMATETC ** ppenum)
{
    return(E_NOTIMPL);
}

HRESULT CabViewDataObject_CreateInstance(REFIID riid, LPVOID *ppvObj)
{
    HRESULT hr;
    CCabObj* pco = new CCabObj();
    if (NULL != pco)
    {
        pco->AddRef();
        hr = pco->QueryInterface(riid, ppvObj);
        pco->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

CCabObj::CCabObj(HWND hwndOwner, CCabFolder *pcf, LPCABITEM *apit, UINT cpit)
: m_lSel(8), m_lContents(NULL)
{
    m_pcfHere = pcf;
    pcf->AddRef();

    m_hwndOwner = hwndOwner;
    m_lSel.AddItems(apit, cpit);
}

// constructor used when we're co-created:
CCabObj::CCabObj() : m_pcfHere(NULL), m_hwndOwner(NULL), m_lSel(8), m_lContents(NULL)
{
};

CCabObj::~CCabObj()
{
    if (m_lContents != NULL)
    {
        int cItems = m_lSel.GetCount();

        while (cItems--)
        {
            if (m_lContents[cItems] != NULL)
            {
                GlobalFree(m_lContents[cItems]);
                m_lContents[cItems] = NULL;
            }
        }

        GlobalFree(m_lContents);
        m_lContents = NULL;
    }

    if (m_pcfHere)
    {
        m_pcfHere->Release();
    }
}


// *** IUnknown methods ***
STDMETHODIMP CCabObj::QueryInterface(
   REFIID riid, 
   LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    if (m_lSel.GetState() == CCabItemList::State_OutOfMem)
    {
        return(E_OUTOFMEMORY);
    }

    LPUNKNOWN pObj;
 
    if (riid == IID_IUnknown)
    {
        pObj = (IUnknown*)((IDataObject*)this); 
    }
    else if (riid == IID_IDataObject)
    {
        pObj = (IUnknown*)((IDataObject*)this); 
    }
    else if (riid == IID_IPersist)
    {
        pObj = (IUnknown*)((IPersist*)this); 
    }
    else if (riid == IID_IPersistStream)
    {
        pObj = (IUnknown*)((IPersistStream*)this); 
    }
    else
    {
           return(E_NOINTERFACE);
    }

    pObj->AddRef();
    *ppvObj = pObj;

    return(NOERROR);
}


STDMETHODIMP_(ULONG) CCabObj::AddRef(void)
{
    return(m_cRef.AddRef());
}


STDMETHODIMP_(ULONG) CCabObj::Release(void)
{
    if (!m_cRef.Release())
    {
           delete this;
        return(0);
    }

    return(m_cRef.GetRef());
}

/////////////////////////////////
////// IPersistStream Impl
/////////////////////////////////


typedef struct
{
    DWORD dwVersion;
    DWORD dwExtraSize;   // After pidl list
    DWORD dwReserved1;
    DWORD dwReserved2;
} CABVIEWDATAOBJ_PERSISTSTRUCT;


/*****************************************************************************\
    FUNCTION: IPersistStream::Load

    DESCRIPTION:
        See IPersistStream::Save() for the layout of the stream.
\*****************************************************************************/
HRESULT CCabObj::Load(IStream *pStm)
{
    HRESULT hr = E_INVALIDARG;

    if (pStm)
    {
        CABVIEWDATAOBJ_PERSISTSTRUCT cvdops;
        DWORD dwNumPidls;

        hr = pStm->Read(&cvdops, SIZEOF(cvdops), NULL);   // #1
        // If we rev the version, read it now (cvdops.dwVersion)

        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidl = NULL;       // ILLoadFromStream frees the param

            // ASSERT(!m_pff);

            hr = ILLoadFromStream(pStm, &pidl); // #2
            if (SUCCEEDED(hr))
            {
                IShellFolder* psf;
                hr = SHGetDesktopFolder(&psf);

                if (SUCCEEDED(hr))
                {
                    if (m_pcfHere)
                    {
                        m_pcfHere->Release();
                        m_pcfHere = NULL;
                    }
                    hr = psf->BindToObject(pidl, NULL, CLSID_CabFolder, (void **)&m_pcfHere);
                    psf->Release();
                }
                
                ILFree(pidl);
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = pStm->Read(&dwNumPidls, SIZEOF(dwNumPidls), NULL);  // #3
        }

        if (SUCCEEDED(hr))
        {
            for (int nIndex = 0; (nIndex < (int)dwNumPidls) && SUCCEEDED(hr); nIndex++)
            {
                LPITEMIDLIST pidl = NULL;       // ILLoadFromStream frees the param

                hr = ILLoadFromStream(pStm, &pidl); // #4
                if (SUCCEEDED(hr))
                {
                    hr = m_lSel.AddItems((LPCABITEM*) &pidl, 1);
                    ILFree(pidl);
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            // We may be reading a version newer than us, so skip their data.
            if (0 != cvdops.dwExtraSize)
            {
                LARGE_INTEGER li = {0};
                
                li.LowPart = cvdops.dwExtraSize;
                hr = pStm->Seek(li, STREAM_SEEK_CUR, NULL);
            }
        }
    }

    // don't return success codes other than S_OK:
    return SUCCEEDED(hr) ? S_OK : hr;
}


/*****************************************************************************\
    FUNCTION: IPersistStream::Save

    DESCRIPTION:
        The stream will be layed out in the following way:

    Version 1:
        1. CABVIEWDATAOBJ_PERSISTSTRUCT - Constant sized data.
        <PidlList BEGIN>
            2. PIDL pidl - Pidl for m_pcfHere.  It will be a public pidl (fully qualified
                        from the shell root)
            3. DWORD dwNumPidls - Number of pidls coming.
            4. PIDL pidl(n) - Pidl in slot (n) of m_lSel
        <PidlList END>
\*****************************************************************************/
HRESULT CCabObj::Save(IStream *pStm, BOOL fClearDirty)
{
    HRESULT hr = E_INVALIDARG;

    if (pStm)
    {
        CABVIEWDATAOBJ_PERSISTSTRUCT cvdops = {0};
        DWORD dwNumPidls = m_lSel.GetCount();

        cvdops.dwVersion = 1;
        hr = pStm->Write(&cvdops, SIZEOF(cvdops), NULL);  // #1
        if (SUCCEEDED(hr))
        {
            if (m_pcfHere)
            {
                LPITEMIDLIST pidl;
                hr = m_pcfHere->GetCurFolder(&pidl);
                if (SUCCEEDED(hr))
                {
                    hr = ILSaveToStream(pStm, pidl); // #2
                    ILFree(pidl);
                }
            }
            else
            {
                hr = E_FAIL;
            }
        }

        if (SUCCEEDED(hr))
            hr = pStm->Write(&dwNumPidls, SIZEOF(dwNumPidls), NULL);  // #3

        if (SUCCEEDED(hr))
        {
            for (int nIndex = 0; (nIndex < (int)dwNumPidls) && SUCCEEDED(hr); nIndex++)
            {
                hr = ILSaveToStream(pStm, (LPCITEMIDLIST) m_lSel[nIndex]); // #4
            }
        }
    }

    return hr;
}


#define MAX_STREAM_SIZE    (500 * 1024) // 500k
/*****************************************************************************\
    FUNCTION: IPersistStream::GetSizeMax

    DESCRIPTION:
        Now this is tough.  I can't calculate the real value because I don't know
    how big the hglobals are going to be for the user provided data.  I will
    assume everything fits in
\*****************************************************************************/
HRESULT CCabObj::GetSizeMax(ULARGE_INTEGER * pcbSize)
{
    if (pcbSize)
    {
        pcbSize->HighPart = 0;
        pcbSize->LowPart = MAX_STREAM_SIZE;
    }
    
    return E_NOTIMPL;
}


/*****************************************************************************\
    DESCRIPTION:
        Allocate an hglobal of the indicated size, initialized from the
    specified buffer.
\*****************************************************************************/
HRESULT StgMediumWriteHGlobal(HGLOBAL *phglob, LPVOID pv, SIZE_T cb)
{
    HRESULT hres = E_OUTOFMEMORY;

    *phglob = 0;            // Rules are rules
    if (cb)
    {
        *phglob = (HGLOBAL) LocalAlloc(LPTR, cb);
        if (phglob)
        {
            hres = S_OK;
            CopyMemory(*phglob, pv, cb);
        }
    }
    else
        hres = E_INVALIDARG;    // Can't clone a discardable block

    return hres;
}


/*****************************************************************************\
    DESCRIPTION:
        When the copy source goes away (the process shuts down), it calls
    OleFlushClipboard.  OLE will then copy our data, release us, and then
    give out our data later.  This works for most things except for:
    1. When lindex needs to very.  This doesn't work because ole doesn't know
       how to ask us how may lindexs they need to copy.
    2. If this object has a private interface OLE doesn't know about.  For us,
       it's IAsyncOperation.

   To get around this problem, we want OLE to recreate us when some possible
   paste target calls OleGetClipboard.  We want OLE to call OleLoadFromStream()
   to have us CoCreated and reload our persisted data via IPersistStream.
   OLE doesn't want to do this by default or they may have backward compat
   problems so they want a sign from the heavens, or at least from us, that
   we will work.  They ping our "OleClipboardPersistOnFlush" clipboard format
   to ask this.
\*****************************************************************************/
HRESULT _RenderOlePersist(STGMEDIUM * pStgMedium)
{
    // The actual cookie value is opaque to the outside world.  Since
    // we don't use it either, we just leave it at zero in case we use
    // it in the future.  It's mere existence will cause OLE to do the
    // use our IPersistStream, which is what we want.
    DWORD dwCookie = 0;
    return StgMediumWriteHGlobal(&pStgMedium->hGlobal, &dwCookie, sizeof(dwCookie));
}


STDMETHODIMP CCabObj::GetData(FORMATETC *pfmt, STGMEDIUM *pmedium)
{
    if (!InitFileGroupDesc())
    {
        return(E_UNEXPECTED);
    }

    if (pfmt->cfFormat == s_uFileGroupDesc)
    {
        if (pfmt->ptd==NULL && (pfmt->dwAspect&DVASPECT_CONTENT) && pfmt->lindex==-1
            && (pfmt->tymed&TYMED_HGLOBAL))
        {
            int cItems = m_lSel.GetCount();
            if (cItems < 1)
            {
                return(E_UNEXPECTED);
            }

            FILEGROUPDESCRIPTOR *pfgd = (FILEGROUPDESCRIPTOR *)GlobalAlloc(GMEM_FIXED,
                sizeof(FILEGROUPDESCRIPTOR) + (cItems-1)*sizeof(FILEDESCRIPTOR));
            if (!pfgd)
            {
                return(E_OUTOFMEMORY);
            }

            pfgd->cItems = cItems;
            for (--cItems; cItems>=0; --cItems)
            {
                LPCABITEM pItem = m_lSel[cItems];
                FILETIME ft;

                pfgd->fgd[cItems].dwFlags = FD_ATTRIBUTES|FD_WRITESTIME|FD_FILESIZE|FD_PROGRESSUI;
                pfgd->fgd[cItems].dwFileAttributes = pItem->uFileAttribs;
                DosDateTimeToFileTime(pItem->uFileDate, pItem->uFileTime,&ft);
                LocalFileTimeToFileTime(&ft, &pfgd->fgd[cItems].ftLastWriteTime);
                pfgd->fgd[cItems].nFileSizeHigh = 0;
                pfgd->fgd[cItems].nFileSizeLow  = pItem->dwFileSize;
                lstrcpyn(pfgd->fgd[cItems].cFileName, PathFindFileName(pItem->szName),
                    ARRAYSIZE(pfgd->fgd[cItems].cFileName));
            }

            pmedium->tymed = TYMED_HGLOBAL;
            pmedium->hGlobal = (HGLOBAL)pfgd;
            pmedium->pUnkForRelease = NULL;

            return(NOERROR);
        }

        return(E_INVALIDARG);
    }

    if (!InitFileContents())
    {
        return(E_UNEXPECTED);
    }

    if (pfmt->cfFormat == s_uFileContents)
    {
        if (pfmt->ptd==NULL && (pfmt->dwAspect&DVASPECT_CONTENT))
        {
            if (pfmt->tymed & TYMED_ISTREAM)
            {
                int cItems = m_lSel.GetCount();

                if ((-1 == pfmt->lindex) || (pfmt->lindex >= cItems))
                {
                    return(E_INVALIDARG);
                }

                HRESULT hRes = InitContents();
                if (FAILED(hRes))
                {
                    return(hRes);
                }

                if (!m_lContents[pfmt->lindex])
                {
                    return(E_OUTOFMEMORY);
                }

                CCabStream *stream = new CCabStream(m_lContents[pfmt->lindex],m_lSel[pfmt->lindex]->dwFileSize);
                if (!stream)
                {
                    return(E_OUTOFMEMORY);
                }

                pmedium->tymed = TYMED_ISTREAM;
                pmedium->pstm = stream;
                pmedium->pUnkForRelease = NULL;

                m_lContents[pfmt->lindex] = NULL;

                return(NOERROR);
            }

            if (pfmt->tymed&TYMED_HGLOBAL)
            {
                int cItems = m_lSel.GetCount();
                if (pfmt->lindex >= cItems)
                {
                    return(E_INVALIDARG);
                }

                HRESULT hRes = InitContents();
                if (FAILED(hRes))
                {
                    return(hRes);
                }

                if (!m_lContents[pfmt->lindex])
                {
                    return(E_OUTOFMEMORY);
                }

                pmedium->tymed = TYMED_HGLOBAL;
                pmedium->hGlobal = m_lContents[pfmt->lindex];
                pmedium->pUnkForRelease = NULL;

                m_lContents[pfmt->lindex] = NULL;

                return(NOERROR);
            }
        }

        return(E_INVALIDARG);
    }

    if (!InitPersistedDataObject())
    {
        return E_UNEXPECTED;
    }

    if (pfmt->cfFormat == s_uPersistedDataObject)
    {
        if ((pfmt->ptd == NULL) &&
            (pfmt->dwAspect & DVASPECT_CONTENT) &&
            (pfmt->lindex == -1) &&
            (pfmt->tymed & TYMED_HGLOBAL))
        {
            return _RenderOlePersist(pmedium);
        }
        else
        {
            return E_INVALIDARG;
        }
    }

    return(E_NOTIMPL);
}


STDMETHODIMP CCabObj::GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
    return(E_NOTIMPL);
}


STDMETHODIMP CCabObj::QueryGetData(FORMATETC *pfmt)
{
    if (!InitFileGroupDesc())
    {
        return(E_UNEXPECTED);
    }

    if (pfmt->cfFormat == s_uFileGroupDesc)
    {
        if (pfmt->ptd==NULL && (pfmt->dwAspect&DVASPECT_CONTENT) && pfmt->lindex==-1
            && (pfmt->tymed&TYMED_HGLOBAL))
        {
            return(S_OK);
        }

        return(E_INVALIDARG);
    }

    if (!InitFileContents())
    {
        return(E_UNEXPECTED);
    }

    if (pfmt->cfFormat == s_uFileContents)
    {
        if (pfmt->ptd==NULL && (pfmt->dwAspect&DVASPECT_CONTENT)
            && (pfmt->tymed & (TYMED_ISTREAM | TYMED_HGLOBAL)))
        {
            return(S_OK);
        }

        return(E_INVALIDARG);
    }

    if (!InitPersistedDataObject())
    {
        return(E_UNEXPECTED);
    }

    if (pfmt->cfFormat == s_uPersistedDataObject)
    {
        if (pfmt->ptd==NULL && (pfmt->dwAspect & DVASPECT_CONTENT)
            && (pfmt->tymed & TYMED_HGLOBAL))
        {
            return(S_OK);
        }

        return(E_INVALIDARG);
    }
    
    return(E_NOTIMPL);
}


STDMETHODIMP CCabObj::GetCanonicalFormatEtc(FORMATETC *pformatetcIn, FORMATETC *pformatetcOut)
{
    return(E_NOTIMPL);
}


STDMETHODIMP CCabObj::SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease)
{
    return(E_NOTIMPL);
}


STDMETHODIMP CCabObj::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc)
{
    if (!InitFileGroupDesc() || !InitFileContents() || !InitPersistedDataObject())
    {
        return(E_UNEXPECTED);
    }

    FORMATETC fmte[] = {
        {(USHORT)s_uFileContents,        NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL|TYMED_ISTREAM },
        {(USHORT)s_uFileGroupDesc,       NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        {(USHORT)s_uPersistedDataObject, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
    };

    CObjFormats *pFmts = new CObjFormats(ARRAYSIZE(fmte), fmte);
    if (!pFmts)
    {
        return(E_OUTOFMEMORY);
    }

    pFmts->AddRef();
    HRESULT hRes = pFmts->QueryInterface(IID_IEnumFORMATETC, (LPVOID *)ppenumFormatEtc);
    pFmts->Release();

    return(hRes);
}


STDMETHODIMP CCabObj::DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink,
    DWORD *pdwConnection)
{
    return(E_NOTIMPL);
}


STDMETHODIMP CCabObj::DUnadvise(DWORD dwConnection)
{
    return(E_NOTIMPL);
}


STDMETHODIMP CCabObj::EnumDAdvise(IEnumSTATDATA **ppenumAdvise)
{
    return(E_NOTIMPL);
}


BOOL CCabObj::InitFileGroupDesc()
{
    if (s_uFileGroupDesc)
    {
        return(TRUE);
    }

    s_uFileGroupDesc = RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR);
    return(s_uFileGroupDesc != 0);
}


BOOL CCabObj::InitFileContents()
{
    if (s_uFileContents)
    {
        return(TRUE);
    }

    s_uFileContents = RegisterClipboardFormat(CFSTR_FILECONTENTS);
    return(s_uFileContents != 0);
}

#define CFSTR_OLECLIPBOARDPERSISTONFLUSH           TEXT("OleClipboardPersistOnFlush")
BOOL CCabObj::InitPersistedDataObject()
{
    if (s_uPersistedDataObject)
    {
        return(TRUE);
    }

    s_uPersistedDataObject = RegisterClipboardFormat(CFSTR_OLECLIPBOARDPERSISTONFLUSH);
    return(s_uPersistedDataObject != 0);
}


HGLOBAL * CALLBACK CCabObj::ShouldExtract(LPCTSTR pszFile, DWORD dwSize, UINT date,
        UINT time, UINT attribs, LPARAM lParam)
{
    CCabObj *pThis = (CCabObj*)lParam;

    int iItem = pThis->m_lSel.FindInList(pszFile, dwSize, date, time, attribs);
    if (iItem < 0)
    {
        return(EXTRACT_FALSE);
    }

    // Copy nothing for now
    return(&(pThis->m_lContents[iItem]));
}


HRESULT CCabObj::InitContents()
{
    if (m_lContents)
    {
        return(NOERROR);
    }

    int iCount = m_lSel.GetCount();
    if (iCount < 1)
    {
        return(E_UNEXPECTED);
    }

    m_lContents = (HGLOBAL *)GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT,
        sizeof(HGLOBAL)*m_lSel.GetCount());
    if (!m_lContents)
    {
        return(E_OUTOFMEMORY);
    }

    TCHAR szHere[MAX_PATH];
    if ((NULL == m_pcfHere) || (!m_pcfHere->GetPath(szHere)))
    {
        return(E_UNEXPECTED);
    }

    CCabExtract ceHere(szHere);

    ceHere.ExtractItems(m_hwndOwner, DIR_MEM, ShouldExtract, (LPARAM)this);

    return(TRUE);
}
