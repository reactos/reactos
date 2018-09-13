//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       dwnpost.cxx
//
//  Contents:   CDwnPost
//              CDwnPostStm
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DWN_HXX_
#define X_DWN_HXX_
#include "dwn.hxx"
#endif

#ifndef X_POSTDATA_HXX_
#define X_POSTDATA_HXX_
#include "postdata.hxx"
#endif

#define _hxx_
#include "element.hdl"

#ifdef WIN16
#define MB_PRECOMPOSED 0
#endif

// Debugging ------------------------------------------------------------------

PerfDbgTag(tagDwnPost,       "Dwn", "Trace CDwnPost")
PerfDbgTag(tagDwnPostStm,    "Dwn", "Trace CDwnPostStm")
PerfDbgTag(tagDwnPostAsStm,  "Dwn", "! Force posting via IStream")
PerfDbgTag(tagDwnPostNoRefs, "Dwn", "! One-shot HGLOBALS for posting");
MtDefine(CDwnPost, Dwn, "CDwnPost")
MtDefine(CDwnPostStm, Dwn, "CDwnPostStm")

// Types ----------------------------------------------------------------------

class CDwnPostStm : public CBaseFT, public IStream
{
    typedef CBaseFT super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDwnPostStm))

    CDwnPostStm(CDwnPost * pDwnPost);

    HRESULT ComputeSize(ULONG * pcb);

    // IUnknown methods

    STDMETHOD (QueryInterface)(REFIID riid, void ** ppv);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // IStream

    STDMETHOD(Clone) (IStream** ppStream);
    STDMETHOD(Commit)(DWORD dwFlags);
    STDMETHOD(CopyTo)(IStream* pStream, ULARGE_INTEGER nBytes, ULARGE_INTEGER* pnBytesRead, ULARGE_INTEGER* pnBytesWritten);
    STDMETHOD(LockRegion)(ULARGE_INTEGER iOffset, ULARGE_INTEGER nBytes, DWORD dwLockType);
    STDMETHOD(Read)(void HUGEP * pBuffer, ULONG nBytes, ULONG* pnBytesRead);
    STDMETHOD(Revert)();
    STDMETHOD(Seek)(LARGE_INTEGER nDisplacement, DWORD dwOrigin, ULARGE_INTEGER* piNewPosition);
    STDMETHOD(SetSize)(ULARGE_INTEGER nNewSize);
    STDMETHOD(Stat)(STATSTG* pStatStg, DWORD dwFlags);
    STDMETHOD(UnlockRegion)(ULARGE_INTEGER iOffset, ULARGE_INTEGER nBytes, DWORD dwLockType);
    STDMETHOD(Write)(const void HUGEP * pBuffer, ULONG nBytes, ULONG* pnBytesWritten);

protected:

    void    Passivate(void);

    CDwnPost *  _pDwnPost;
    HANDLE      _hFile;
    UINT        _uItem;
    ULONG       _ulOffset;
    ULONG       _cbSize;
};

// CDwnPost (IUnknown) --------------------------------------------------------

STDMETHODIMP
CDwnPost::QueryInterface(REFIID riid, void ** ppv)
{
    PerfDbgLog(tagDwnPost, this, "+CDwnPost::QueryInterface");

    HRESULT hr;

    if (riid == IID_IUnknown)
    {
        *ppv = (IUnknown *)this;
        AddRef();
        hr = S_OK;
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    PerfDbgLog1(tagDwnPost, this, "-CDwnPost::QueryInterface (hr=%lX)", hr);
    return(hr);
}

STDMETHODIMP_(ULONG)
CDwnPost::AddRef()
{
    PerfDbgLog(tagDwnPost, this, "+CDwnPost::AddRef");

    ULONG ulRefs = super::AddRef();

    PerfDbgLog1(tagDwnPost, this, "-CDwnPost::AddRef (cRefs=%ld)", ulRefs);
    return(ulRefs);
}

STDMETHODIMP_(ULONG)
CDwnPost::Release()
{
    PerfDbgLog(tagDwnPost, this, "+CDwnPost::Release");

    ULONG ulRefs = super::Release();

    PerfDbgLog1(tagDwnPost, this, "-CDwnPost::Release (cRefs=%ld)", ulRefs);
    return(ulRefs);
}

//+--------------------------------------------------------------------------//
//
//  Method:     CDwnPost::~CDwnPost
//
//  Synopsis:   frees the strings and the descriptor list
//              
//---------------------------------------------------------------------------//

CDwnPost::~CDwnPost()
{
    PerfDbgLog(tagDwnPost, this, "+CDwnPost::~CDwnPost");

    CPostItem * pItem;

    if (_pItems)
    {
        UINT i;
        for (pItem = _pItems, i = _cItems; i > 0; i--, pItem++)
        {
            switch (pItem->_ePostDataType)
            {
                case POSTDATA_LITERAL:
                    delete [] pItem->_pszAnsi;
                    break;

                case POSTDATA_FILENAME:
                    delete [] pItem->_pszWide;
                    break;
            }
        }
        delete [] _pItems;
    }

    if (_hGlobal)
    {
        GlobalFree(_hGlobal);
    }

    PerfDbgLog(tagDwnPost, this, "-CDwnPost::~CDwnPost");
}

//+--------------------------------------------------------------------------//
//
//  Method:     CDwnPost::Create, static
//
//  Arguments:  cItems    - number of items for descriptor array allocation
//              ppDwnPost - return the object here
//
//  Synopsis:   Create a skeleton CDwnPost object
//              having cItems sections
//
//---------------------------------------------------------------------------//
HRESULT
CDwnPost::Create(UINT cItems, CDwnPost ** ppDwnPost)
{
    PerfDbgLog1(tagDwnPost, NULL, "CDwnPost::Create (cItems=%ld)", cItems);

    CDwnPost * pDwnPost = new CDwnPost;
    HRESULT hr;

    if (pDwnPost == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (cItems > 0)
    {
        pDwnPost->_pItems = new(Mt(CPostItem)) CPostItem[cItems];

        if (pDwnPost->_pItems == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        memset(pDwnPost->_pItems, 0, cItems * sizeof(CPostItem));

        pDwnPost->_cItems = cItems;
    }

    hr = THR(CoFileTimeNow(&pDwnPost->_ftCreated));
    if (hr)
        goto Cleanup;

Cleanup:

    if (hr && pDwnPost)
    {
        pDwnPost->Release();
        *ppDwnPost = NULL;
    }
    else
    {
        *ppDwnPost = pDwnPost;
    }

    PerfDbgLog1(tagDwnPost, NULL, "CDwnPost::Create (hr=%lX)", hr);
    RRETURN(hr);
}

//+--------------------------------------------------------------------------//
//
//  Method:     CDwnPost::Create, static
//
//  Synopsis:   Create a DownPost object from a SubmitData object
//              
//  Arguments:  pSubmitData - the object to init from
//              ppDwnPost   - return the object here
//
//  Note::      DownPost takes ownership of the strings passed
//              in in SubmitData
//
//---------------------------------------------------------------------------//

HRESULT
CDwnPost::Create(CPostData * pSubmitData, CDwnPost ** ppDwnPost)
{
    PerfDbgLog1(tagDwnPost, NULL, "CDwnPost::Create (pSubmitData=%lx)", pSubmitData);

    HRESULT hr;

    hr = THR(Create((UINT)0, ppDwnPost));

    if (hr == S_OK)
    {
        TCHAR achEncoding[ENCODING_SIZE];

        achEncoding[0] = 0;

        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
            pSubmitData->GetEncodingString(), -1,
            achEncoding, ARRAY_SIZE(achEncoding));

        // copy the string and remove the CRLF
        hr = THR((*ppDwnPost)->_cstrEncoding.Set(achEncoding, _tcslen(achEncoding) - 2));

        if (hr)
        {
            (*ppDwnPost)->Release();
            *ppDwnPost = NULL;
            goto Cleanup;
        }

        // Take ownership of the string list
        (*ppDwnPost)->_cItems  = pSubmitData->_cItems;
        (*ppDwnPost)->_pItems  = pSubmitData->_pItems;

        //  Clear the SubmitData object
        pSubmitData->_cItems = 0;
        pSubmitData->_pItems = 0;
    }

    #if DBG==1
    (*ppDwnPost)->VerifySaveLoad();
    #endif

Cleanup:
    PerfDbgLog1(tagDwnPost, NULL, "CDwnPost::Create (hr=%lX)", hr);
    RRETURN(hr);
}

//+--------------------------------------------------------------------------//
//
//  Method:     CDwnPost::Save, static
//
//  Arguments:  pstm - the stream
//              pDwnPost - the object to save. Can be NULL.
//
//  Synopsis:   Save private format into a stream
//              
//---------------------------------------------------------------------------//

HRESULT
CDwnPost::Save(CDwnPost * pDwnPost, IStream * pstm)
{
    PerfDbgLog(tagDwnPost, pDwnPost, "+CDwnPost::Save");

    DWORD   cItems = pDwnPost ? pDwnPost->_cItems : 0xFFFFFFFF;
    HRESULT hr;

    hr = THR(pstm->Write(&cItems, sizeof(DWORD), NULL));
    if (hr)
        goto Cleanup;

    if (pDwnPost && cItems)
    {
        CPostItem * pItem;

        hr = THR(pstm->Write(&pDwnPost->_ftCreated, sizeof(FILETIME), NULL));
        if (hr)
            goto Cleanup;

        hr = THR(pDwnPost->_cstrEncoding.Save(pstm));
        if (hr)
            goto Cleanup;

        for ( pItem = pDwnPost->_pItems, cItems = pDwnPost->_cItems;
              cItems > 0;
              cItems--, pItem++ )
        {
            hr = THR(pstm->Write(&pItem->_ePostDataType,
                        sizeof(POSTDATA_KIND), NULL));

            switch (pItem->_ePostDataType)
            {
                case POSTDATA_LITERAL:
                {
                    char * psz = pItem->_pszAnsi;
                    ULONG cb = psz ? 1 + strlen(psz) : 0;

                    hr = THR(pstm->Write(&cb, sizeof(ULONG), NULL));
                    if (hr)
                        goto Cleanup;

                    if (cb)
                    {
                        hr = THR(pstm->Write(psz, cb, NULL));
                        if (hr)
                            goto Cleanup;
                    }
                }
                break;

                case POSTDATA_FILENAME:
                {
                    TCHAR * psz = pItem->_pszWide;
                    ULONG cb = psz ? sizeof (TCHAR) * (1 + _tcslen(psz)) : 0;

                    hr = THR(pstm->Write(&cb, sizeof(ULONG), NULL));
                    if (hr)
                        goto Cleanup;

                    if (cb)
                    {
                        hr = THR(pstm->Write(psz, cb, NULL));
                        if (hr)
                            goto Cleanup;
                    }
                }
                break;
            }
        }
    }

Cleanup:
    PerfDbgLog1(tagDwnPost, pDwnPost, "-CDwnPost::Save (hr=%lX)", hr);
    RRETURN(hr);
}

//+--------------------------------------------------------------------------//
//
//  Method:     CDwnPost::Load, static
//
//  Arguments:  pstm - the stream
//              ppDwnPost - return the DownPost object here. Can become NULL.
//
//  Synopsis:   Load private format from a stream
//              
//---------------------------------------------------------------------------//

HRESULT
CDwnPost::Load(IStream * pstm, CDwnPost ** ppDwnPost)
{
    PerfDbgLog(tagDwnPost, NULL, "+CDwnPost::Load");

    CDwnPost *  pDwnPost = NULL;
    CPostItem * pItem;
    DWORD       cItems;
    ULONG       cb;
    HRESULT     hr;

    hr = THR(pstm->Read(&cItems, sizeof(DWORD), NULL));
    if (hr)
        goto Cleanup;
    
    if (cItems != 0xFFFFFFFF)
    {
        hr = THR(Create(cItems, &pDwnPost));
        if (hr)
            goto Cleanup;

        hr = THR(pstm->Read(&pDwnPost->_ftCreated, sizeof(FILETIME), NULL));
        if (hr)
            goto Cleanup;

        hr = THR(pDwnPost->_cstrEncoding.Load(pstm));
        if (hr)
            goto Cleanup;

        if (cItems == 0)
            goto Cleanup;

        for ( pItem = pDwnPost->_pItems;
              cItems > 0;
              cItems--, pItem++ )
        {
            hr = THR(pstm->Read(&pItem->_ePostDataType, sizeof(POSTDATA_KIND), NULL));
            if (hr)
                goto Cleanup;

            switch (pItem->_ePostDataType)
            {
                case POSTDATA_LITERAL:
                case POSTDATA_FILENAME:
                {
                    hr = THR(pstm->Read(&cb, sizeof(cb), NULL));
                    if (hr)
                        goto Cleanup;

                    if (cb)
                    {
                        pItem->_pszAnsi = (char *)MemAlloc(Mt(CPostItem_psz), cb);

                        if (!pItem->_pszAnsi)
                        {
                            hr = E_OUTOFMEMORY;
                            goto Cleanup;
                        }

                        hr = THR(pstm->Read(pItem->_pszAnsi, cb, NULL));
                        if (hr)
                            goto Cleanup;
                    }
                    break;
                }

                default:
                    AssertSz(FALSE, "Unrecognized POSTDATA type");
                    hr = E_FAIL;
                    goto Cleanup;
            }
        }
    }

Cleanup:
    if (hr && pDwnPost)
    {
        pDwnPost->Release();
        *ppDwnPost = NULL;
    }
    else
    {
        *ppDwnPost = pDwnPost;
    }

    PerfDbgLog1(tagDwnPost, NULL, "-CDwnPost::Load (hr=%lX)", hr);
    RRETURN(hr);
}

//+--------------------------------------------------------------------------//
//
//  Method:     CDwnPost::GetSaveSize, static
//
//  Arguments:  pDwnPost
//
//  Synopsis:   Computes the maximum size needed by CDwnPost::Save
//
//---------------------------------------------------------------------------//

ULONG
CDwnPost::GetSaveSize(CDwnPost * pDwnPost)
{
    PerfDbgLog(tagDwnPost, pDwnPost, "+CDwnPost::GetSaveSize");

    ULONG cb = sizeof(DWORD);

    if (pDwnPost)
    {
        cb += sizeof(FILETIME) + pDwnPost->_cstrEncoding.GetSaveSize();

        CPostItem * pItem = pDwnPost->_pItems;
        ULONG       cItem = pDwnPost->_cItems;

        for (; cItem > 0; --cItem, ++pItem)
        {
            cb += sizeof(POSTDATA_KIND);

            switch (pItem->_ePostDataType)
            {
                case POSTDATA_LITERAL:
                    cb += sizeof(ULONG);
                    if (pItem->_pszAnsi)
                        cb += strlen(pItem->_pszAnsi) + 1;
                    break;
                case POSTDATA_FILENAME:
                    cb += sizeof(ULONG);
                    if (pItem->_pszWide)
                        cb += (_tcslen(pItem->_pszWide) + 1) * sizeof(TCHAR);
                    break;
            }
        }
    }

    PerfDbgLog1(tagDwnPost, pDwnPost, "-CDwnPost::GetSaveSize (cb=%ld)", cb);
    return(cb);
}

//+--------------------------------------------------------------------------//
//
//  Method:     CDwnPost::VerifySaveLoad
//
//  Arguments:  
//
//  Synopsis:   Debugging checks to make sure save/load sequence works
//
//---------------------------------------------------------------------------//

#if DBG==1

void
CDwnPost::VerifySaveLoad()
{
    IStream * pstm = NULL;
    CDwnPost * pDwnPost = NULL;
    LARGE_INTEGER li = { 0 };
    ULARGE_INTEGER uli;
    ULONG cbSave;
    CPostItem * pItemSrc, * pItemDst;
    ULONG cItem;
    HRESULT hr;

    cbSave = GetSaveSize(this);

    hr = THR(CreateStreamOnHGlobal(NULL, TRUE, &pstm));
    if (hr)
        goto Cleanup;

    hr = THR(Save(this, pstm));
    if (hr)
        goto Cleanup;

    hr = THR(pstm->Seek(li, STREAM_SEEK_CUR, &uli));
    if (hr)
        goto Cleanup;
    
    Assert(uli.LowPart == cbSave && uli.HighPart == 0);

    hr = THR(pstm->Seek(li, STREAM_SEEK_SET, &uli));
    if (hr)
        goto Cleanup;

    Assert(uli.LowPart == 0 && uli.HighPart == 0);

    hr = THR(Load(pstm, &pDwnPost));
    if (hr)
        goto Cleanup;

    Assert(GetItemCount() == pDwnPost->GetItemCount());

    Assert(!!_cstrEncoding == !!pDwnPost->_cstrEncoding);

    Assert(!_cstrEncoding || StrCmpC(_cstrEncoding, pDwnPost->_cstrEncoding) == 0);

    Assert(memcmp(&_ftCreated, &pDwnPost->_ftCreated, sizeof(FILETIME)) == 0);

    pItemSrc = GetItems();
    pItemDst = pDwnPost->GetItems();
    cItem    = GetItemCount();

    for (; cItem > 0; --cItem, ++pItemSrc, ++pItemDst)
    {
        Assert(pItemSrc->_ePostDataType == pItemDst->_ePostDataType);

        if (pItemSrc->_ePostDataType == POSTDATA_LITERAL)
        {
            Assert(strcmp(pItemSrc->_pszAnsi, pItemDst->_pszAnsi) == 0);
        }
        else if (pItemSrc->_ePostDataType == POSTDATA_FILENAME)
        {
            Assert(StrCmpC(pItemSrc->_pszWide, pItemDst->_pszWide) == 0);
        }
    }

Cleanup:
    if (pDwnPost)
        pDwnPost->Release();
    ReleaseInterface(pstm);
}

#endif

DWORD HashBytes(void *pb, DWORD cb, DWORD hash)
{
    while (cb)
    {
        hash = (hash >> 7) | (hash << (32-7));
        hash += *(BYTE*)pb; // Case-sensitive hash
        pb = (BYTE*)pb+1;
        cb--;
    }

    return hash;
}

//+--------------------------------------------------------------------------//
//
//  Method:     CDwnPost::GetBindInfo
//
//  Arguments:  
//
//  Synopsis:   Fills out a STGMEDIUM structure for accessing the post data
//
//---------------------------------------------------------------------------//

HRESULT
CDwnPost::GetBindInfo(BINDINFO * pbindinfo)
{
    PerfDbgLog(tagDwnPost, this, "+CDwnPost::GetBindInfo");

    HGLOBAL hGlobal = _hGlobal;
    ULONG   cb      = _cbGlobal;
    HRESULT hr      = S_OK;

    if (    hGlobal == NULL
        &&  (   _cItems == 0
            ||  (_cItems == 1 && _pItems->_ePostDataType == POSTDATA_LITERAL)) )
    {
        cb      = _cItems ? strlen(_pItems->_pszAnsi) : 0;
        hGlobal = GlobalAlloc(GMEM_FIXED, cb);

        if (hGlobal == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        if (cb)
        {
            memcpy((void *)hGlobal, _pItems->_pszAnsi, cb);
        }

        // It is possible that two threads just did the same allocation, so
        // we enter a critical section and pick the first one.

        g_csDwnPost.Enter();

        if (_hGlobal == NULL)
        {
            _hGlobal  = hGlobal;
            _cbGlobal = cb;
        }
        else
        {
            GlobalFree(hGlobal);
            hGlobal = _hGlobal;
        }

        g_csDwnPost.Leave();

        Assert(_hGlobal != NULL);
        Assert(hGlobal == _hGlobal);
    }

    #if DBG==1
    if (IsPerfDbgEnabled(tagDwnPostAsStm))
        hGlobal = NULL;
    #endif

    if (hGlobal)
    {
        pbindinfo->stgmedData.tymed = TYMED_HGLOBAL;
        pbindinfo->stgmedData.hGlobal = _hGlobal;
        pbindinfo->cbstgmedData = cb;

    #if DBG==1
        if (IsPerfDbgEnabled(tagDwnPostNoRefs))
        {
            // This is not a thread-safe thing to do, but it is only used
            // for debugging when we suspect that the caller of GetBindInfo
            // is not properly releasing the STGMEDIUM and therefore leaking
            // the CDwnPost.  Turning this tag on allows us to isolate this
            // condition.

            _hGlobal = NULL;
        }
        else
    #endif
        {
            pbindinfo->stgmedData.pUnkForRelease = this;
            AddRef();
        }
    }
    else
    {
        CDwnPostStm * pDwnPostStm = new CDwnPostStm(this);

        if (pDwnPostStm == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        if (_cbGlobal == 0)
        {
            hr = THR(pDwnPostStm->ComputeSize(&_cbGlobal));

            if (hr)
            {
                pDwnPostStm->Release();
                goto Cleanup;
            }
        }

        pbindinfo->stgmedData.tymed = TYMED_ISTREAM;
        pbindinfo->stgmedData.pUnkForRelease = NULL;    //Release will be handled by pstm
        pbindinfo->stgmedData.pstm = pDwnPostStm;
        pbindinfo->cbstgmedData = _cbGlobal;
    }


Cleanup:
    PerfDbgLog(tagDwnPost, this, "-CDwnPost::GetBindInfo");
    RRETURN(hr);
}

HRESULT
CDwnPost::GetHashString(LPOLESTR *ppchHashString)
{
    TCHAR achBuf[9];
    HRESULT hr;
    
    hr = THR(Format(0, achBuf, ARRAY_SIZE(achBuf), _T("<0x>"), ComputeHash()));
    if (hr)
        goto Cleanup;

    hr = TaskAllocString(achBuf, ppchHashString);
    if (hr)
        goto Cleanup;
        
Cleanup:
    RRETURN(hr);
}

DWORD
CDwnPost::ComputeHash()
{
    PerfDbgLog(tagDwnPost, this, "+CDwnPostStm::ComputeHash");

    ULONG i;
    CPostItem * pItem;
    DWORD dwHash = 0;

    for (i = _cItems, pItem = _pItems; i>0; i--, pItem++)
    {
        //  Iterate
        Assert(pItem);
        switch ( pItem->_ePostDataType )
        {
        case POSTDATA_LITERAL:
            dwHash = HashBytes(pItem->_pszAnsi, strlen(pItem->_pszAnsi), dwHash);
            break;

        case POSTDATA_FILENAME:
            dwHash = HashBytes(pItem->_pszWide, _tcslen(pItem->_pszWide), dwHash);
            break;
        }
    }


    PerfDbgLog1(tagDwnPost, this, "-CDwnPostStm::ComputeHash (dw=%ld)", dwHash);
    
    return(dwHash);
}

//+--------------------------------------------------------------------------//
//
//  Class:  CDwnPostStm
//
//---------------------------------------------------------------------------//

CDwnPostStm::CDwnPostStm(CDwnPost * pDwnPost)
{
    PerfDbgLog(tagDwnPostStm, this, "+CDwnPostStm::CDwnPostStm");

    _pDwnPost = pDwnPost;
    _pDwnPost->AddRef();
    _hFile = INVALID_HANDLE_VALUE;

    PerfDbgLog(tagDwnPostStm, this, "-CDwnPostStm::CDwnPostStm");
}

//+--------------------------------------------------------------------------//
//
//  Method:     CDwnPostStm::Passivate, protected
//
//  Synopsis:   clean up the stream provider
//
//---------------------------------------------------------------------------//

void
CDwnPostStm::Passivate(void)
{
    PerfDbgLog(tagDwnPostStm, this, "+CDwnPostStm::Passivate");

    if ( _hFile != INVALID_HANDLE_VALUE )
    {
        Verify(CloseHandle(_hFile));
    }

    ReleaseInterface(_pDwnPost);

    super::Passivate();

    PerfDbgLog(tagDwnPostStm, this, "-CDwnPostStm::Passivate");
}

// CDwnPostStm (IUnknown) -----------------------------------------------------

STDMETHODIMP
CDwnPostStm::QueryInterface(REFIID iid, void ** ppv)
{
    PerfDbgLog(tagDwnPostStm, this, "+CDwnPostStm::QueryInterface");

    HRESULT hr;

    if (iid == IID_IUnknown || iid == IID_IStream)
    {
        *ppv = (IStream *)this;
        AddRef();
        hr = S_OK;
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    PerfDbgLog1(tagDwnPostStm, this, "-CDwnPostStm::QueryInterface (hr=%lX)", hr);
    return(hr);
}

STDMETHODIMP_(ULONG)
CDwnPostStm::AddRef()
{
    PerfDbgLog(tagDwnPostStm, this, "+CDwnPostStm::AddRef");

    ULONG ulRefs = super::AddRef();

    PerfDbgLog1(tagDwnPostStm, this, "-CDwnPostStm::AddRef (cRefs=%ld)", ulRefs);
    return(ulRefs);
}

STDMETHODIMP_(ULONG)
CDwnPostStm::Release()
{
    PerfDbgLog(tagDwnPostStm, this, "+CDwnPostStm::Release");

    ULONG ulRefs = super::Release();

    PerfDbgLog1(tagDwnPostStm, this, "-CDwnPostStm::Release (cRefs=%ld)", ulRefs);
    return(ulRefs);
}

// CDwnPostStm (IStream) ------------------------------------------------------

STDMETHODIMP CDwnPostStm::Clone(IStream ** ppStream)
{
    PerfDbgLog(tagDwnPostStm, this, "+CDwnPostStm::Clone");

    HRESULT hr = E_NOTIMPL;
    *ppStream = NULL;

    PerfDbgLog1(tagDwnPostStm, this, "-CDwnPostStm::Clone (hr=%lX)", hr);
    RRETURN(hr);
}

STDMETHODIMP CDwnPostStm::Commit(DWORD dwFlags)
{
    PerfDbgLog(tagDwnPostStm, this, "+CDwnPostStm::Commit");

    HRESULT hr = S_OK;

    PerfDbgLog1(tagDwnPostStm, this, "-CDwnPostStm::Commit (hr=%lX)", hr);
    RRETURN(hr);
}

STDMETHODIMP CDwnPostStm::CopyTo(IStream* pStream, ULARGE_INTEGER nBytes,
    ULARGE_INTEGER* pnBytesRead, ULARGE_INTEGER* pnBytesWritten)
{
    PerfDbgLog(tagDwnPostStm, this, "+CDwnPostStm::CopyTo");

    HRESULT hr = E_NOTIMPL;

    PerfDbgLog1(tagDwnPostStm, this, "-CDwnPostStm::CopyTo (hr=%lX)", hr);
    RRETURN(hr);
}

STDMETHODIMP CDwnPostStm::LockRegion(ULARGE_INTEGER iOffset, 
    ULARGE_INTEGER nBytes, DWORD dwLockType)
{
    PerfDbgLog(tagDwnPostStm, this, "+CDwnPostStm::LockRegion");

    HRESULT hr = E_NOTIMPL;

    PerfDbgLog1(tagDwnPostStm, this, "-CDwnPostStm::LockRegion (hr=%lX)", hr);
    RRETURN(hr);
}

STDMETHODIMP CDwnPostStm::Read(void HUGEP * pBuffer, ULONG nBytes, ULONG* pnBytesRead)
{
    PerfDbgLog1(tagDwnPostStm, this, "+CDwnPostStm::Read (cbReq=%ld)", nBytes);

    HRESULT hr = S_OK;
    BYTE * pbBuffer = (BYTE *)pBuffer;
    ULONG cbWritten = 0;

    if (pnBytesRead)
    {
        *pnBytesRead = 0;
    }

    if (pBuffer == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (nBytes == 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    while ( _uItem < _pDwnPost->_cItems && cbWritten < nBytes)
    {
        CPostItem * pItem;
        ULONG cb;
        ULONG cbStrLen;
        BOOL f;

        pItem = _pDwnPost->_pItems + _uItem;

        switch ( pItem->_ePostDataType )
        {
        case POSTDATA_LITERAL:
            cbStrLen = strlen(pItem->_pszAnsi) - _ulOffset;
            cb = min(cbStrLen, nBytes - cbWritten);
            memcpy(pbBuffer+cbWritten, pItem->_pszAnsi + _ulOffset, cb);

            cbWritten += cb;
            _ulOffset += cb;

            if ( cb == cbStrLen ) // or _ulOffset == strlen(pItem->_pszAnsi) ??
            {
                //  We consumed this section, switch to the next
                _ulOffset = 0;
                _uItem++;
            }
            break;

        case POSTDATA_FILENAME:

            cb = 0;

            //  Note: NetScape 3 doesn't mime64 encode the
            //        uploaded file, even if it is NOT text
            
            if ( _hFile == INVALID_HANDLE_VALUE )
            {
                DWORD dw;

                //  Open the file and get its size.
                //  If it fails, bail out of this section
                _hFile = CreateFile(pItem->_pszWide,
                                    GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);

                if ( _hFile == INVALID_HANDLE_VALUE )
                    goto FileError;

#ifndef WIN16
                dw = GetFileType(_hFile);
                if ( dw != FILE_TYPE_DISK )
                    goto FileError;
#endif // ndef WIN16

                cb = GetFileSize(_hFile, NULL);  //  Don't care about high DWORD, we don't handle
                                                //  that large files anyway.
                if ( cb == 0xFFFFFFFF )
                    goto FileError;
            }

            //  Here the file should be open and ripe for consumption
            f = ReadFile(_hFile, pbBuffer+cbWritten, nBytes - cbWritten, &cb, NULL);
            if ( !f )
                goto Win32Error;

            if ( cb < nBytes - cbWritten )
            {
                goto FileError;
            }

FinishSection:
            cbWritten += cb;
            break;

Win32Error:
            hr = GetLastWin32Error();
            //  Fall through

FileError:
            if ( _hFile != INVALID_HANDLE_VALUE )
            {
                Verify(CloseHandle(_hFile));
            }
            _hFile = INVALID_HANDLE_VALUE;
            _ulOffset = 0;
            _uItem++;

            goto FinishSection;
        }
    }

    Assert(cbWritten <= nBytes);

    if (pnBytesRead)
    {
        *pnBytesRead = cbWritten;
    }

    if (SUCCEEDED(hr) && cbWritten == 0)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

Cleanup:
    PerfDbgLog2(tagDwnPostStm, this, "-CDwnPostStm::Read (cbRead=%ld,hr=%lX)", cbWritten, hr);
    RRETURN1(hr, S_FALSE);
}


STDMETHODIMP CDwnPostStm::Revert()
{
    PerfDbgLog(tagDwnPostStm, this, "+CDwnPostStm::Revert");

    HRESULT hr = E_NOTIMPL;

    PerfDbgLog1(tagDwnPostStm, this, "-CDwnPostStm::Revert (hr=%lX)", hr);
    RRETURN(hr);
}

STDMETHODIMP CDwnPostStm::Seek(LARGE_INTEGER nDisplacement, DWORD dwOrigin,
    ULARGE_INTEGER* piNewPosition)
{
    PerfDbgLog(tagDwnPostStm, this, "+CDwnPostStm::Seek");

    HRESULT hr = E_FAIL;

    // We can only seek back to the beginning of the stream.
    if ( nDisplacement.LowPart == 0 && nDisplacement.HighPart == 0 && 
        dwOrigin == STREAM_SEEK_SET )
    {
        hr = S_OK;
        
        if ( _hFile != INVALID_HANDLE_VALUE )
        {
            Verify(CloseHandle(_hFile));
            _hFile = INVALID_HANDLE_VALUE;
        }

        _uItem = 0;
        _ulOffset = 0;
    }

    PerfDbgLog1(tagDwnPostStm, this, "-CDwnPostStm::Seek (hr=%lX)", hr);
    RRETURN(hr);
}

STDMETHODIMP CDwnPostStm::SetSize(ULARGE_INTEGER nNewSize)
{
    PerfDbgLog(tagDwnPostStm, this, "+CDwnPostStm::SetSize");

    HRESULT hr = E_NOTIMPL;

    PerfDbgLog1(tagDwnPostStm, this, "-CDwnPostStm::SetSize (hr=%lX)", hr);
    RRETURN(hr);
}

STDMETHODIMP
CDwnPostStm::Stat(STATSTG * pStatStg, DWORD dwFlags)
{
    PerfDbgLog(tagDwnPostStm, this, "+CDwnPostStm::Stat");

    memset(pStatStg, 0, sizeof(STATSTG));

    pStatStg->type           = STGTY_STREAM;
    pStatStg->mtime          =
    pStatStg->ctime          =
    pStatStg->atime          = _pDwnPost->_ftCreated;
    pStatStg->grfMode        = STGM_READ;
    pStatStg->cbSize.LowPart = _pDwnPost->_cbGlobal;

    PerfDbgLog(tagDwnPostStm, this, "-CDwnPostStm::Stat (hr=0)");
    return(S_OK);
}

STDMETHODIMP CDwnPostStm::UnlockRegion(ULARGE_INTEGER iOffset, 
    ULARGE_INTEGER nBytes, DWORD dwLockType)
{
    PerfDbgLog(tagDwnPostStm, this, "+CDwnPostStm::UnlockRegion");

    HRESULT hr = E_NOTIMPL;

    PerfDbgLog1(tagDwnPostStm, this, "-CDwnPostStm::UnlockRegion (hr=%lX)", hr);
    RRETURN(hr);
}

STDMETHODIMP CDwnPostStm::Write(const void HUGEP * pBuffer, ULONG nBytes, 
    ULONG* pnBytesWritten)
{
    PerfDbgLog(tagDwnPostStm, this, "+CDwnPostStm::Write");

    HRESULT hr = E_NOTIMPL;

    PerfDbgLog1(tagDwnPostStm, this, "-CDwnPostStm::Write (hr=%lX)", hr);
    RRETURN(hr);
}

// CDwnPostStm (Internal) -----------------------------------------------------

HRESULT
CDwnPostStm::ComputeSize(ULONG * pcb)
{
    PerfDbgLog(tagDwnPostStm, this, "+CDwnPostStm::ComputeSize");

    HRESULT hr = S_OK;
    ULONG i;
    CPostItem * pItem;
    ULONG cbSize = 0;

    Assert(_pDwnPost);

    for ( i = _pDwnPost->_cItems, pItem = _pDwnPost->_pItems;
          i > 0;
          i--, pItem++ )
    {
        //  Iterate
        Assert(pItem);
        switch ( pItem->_ePostDataType )
        {
        case POSTDATA_LITERAL:
            cbSize += strlen(pItem->_pszAnsi);
            break;

        case POSTDATA_FILENAME:
            {
                HANDLE hFile;
                DWORD dw;

                hFile = CreateFile( pItem->_pszWide,
                                    GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);

                if ( hFile == INVALID_HANDLE_VALUE )
                    goto CloseFile;

#ifndef WIN16
                dw = GetFileType(hFile);
                if ( dw != FILE_TYPE_DISK )
                    goto CloseFile;
#endif // ndef WIN16

                dw = GetFileSize(hFile, NULL);  //  Don't care about high DWORD, we don't handle
                                                //  that large files anyway.
                if ( dw != 0xFFFFFFFF )
                {
#if NEVER
                    //  This is the way to compute the MIME64 encoded file size
                    //  We don't encode on fil eupload, so don't fudge the size either
                    DWORDLONG dwl = dw + 2;     //  padding
                    dwl /= 3;
                    dwl *= 4;
                    dwlSize += dwl;             //  BUGBUG(laszlog):This doesn't count the newlines!
#else
                    cbSize += dw;
#endif
                }
CloseFile:
                if ( hFile != INVALID_HANDLE_VALUE )
                {
                    Verify(CloseHandle(hFile));
                }
            }
            break;
        }
    }

    *pcb = cbSize;

    PerfDbgLog2(tagDwnPostStm, this, "-CDwnPostStm::ComputeSize (cb=%ld,hr=%lX)", cbSize, hr);
    RRETURN(hr);
}

