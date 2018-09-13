#if defined(UNIX) && defined(_HPUX_SOURCE)
#  define _MODULE_IS_NOT_DEFINED
#endif // UNIX
#include "headers.hxx"

#ifndef X_IMG_HXX_
#define X_IMG_HXX_
#include "img.hxx"
#endif

#ifndef X_ATLBASE_H_
#define X_ATLBASE_H_
#include "atlbase.h"
#endif

#define _ATL_NO_CONNECTION_POINTS

#ifndef X_ATLCOM_H_
#define X_ATLCOM_H_
#include "atlcom.h"
#endif

#ifdef UNIX
#ifndef X_DDRAW_H_
#define X_DDRAW_H_
#include "ddraw.h"
#endif
#endif

#ifndef X_IMGUTIL_H_
#define X_IMGUTIL_H_
#include "imgutil.h"
#endif

#ifndef X_DDRAW_H_
#define X_DDRAW_H_
#include "ddraw.h"
#endif

MtDefine(CImgTaskPlug, Dwn, "CImgTaskPlug")
MtDefine(CPlugStream, CImgTaskPlug, "CPlugStream")
MtDefine(CImageDecodeFilter, CImgTaskPlug, "CImageDecodeFilter")
MtDefine(CImageDecodeEventSink, CImgTaskPlug, "CImageDecodeEventSink")

#undef  DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	EXTERN_C const GUID name \
		= { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

IUnknown* WINAPI _AtlComPtrAssign(IUnknown** pp, IUnknown* lp)
{
    if (lp != NULL)
	lp->AddRef();
    if (*pp)
	(*pp)->Release();
    *pp = lp;
    return lp;
}

class CImgTaskPlug : public CImgTask
{
    typedef CImgTask super;
    friend class CImageDecodeEventSink;
    friend class CPlugStream;

public:
    
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CImgTaskPlug))

    virtual void Decode(BOOL *pfNonProgressive);
    virtual void BltDib(HDC hdc, RECT * prcDst, RECT * prcSrc, DWORD dwRop, DWORD dwFlags);

protected:
    HRESULT SetColorTable(IDithererImpl* pDitherer);
    HRESULT GetTransIndex();
    
// Data members
public:
    UINT                                _nColors;
    CComPtr< IImageDecodeFilter >       m_pFilter;
    CComPtr< IImageDecodeEventSink >    m_pEventSink;
    CComPtr< IStream >                  m_pStream;
    CComPtr< IDirectDrawSurface >       m_pSurface;
    LONG                                m_nPitch;
    ULONG                               m_nBytesPerPixel;
    BYTE*                               m_pbFirstScanLine;
    BYTE*                               m_pbBits;
    BOOL                                m_bGotTrans;
};

class CPlugStream :
    public IStream
{

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CPlugStream))

    CPlugStream(CImgTaskPlug* pFilter);
    ~CPlugStream();

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    STDMETHOD(QueryInterface)(REFIID iid, void** ppInterface);

// IStream
public:
    STDMETHOD(Clone)(IStream** ppStream);
    STDMETHOD(Commit)(DWORD dwFlags);
    STDMETHOD(CopyTo)(IStream* pStream, ULARGE_INTEGER nBytes, 
	ULARGE_INTEGER* pnBytesRead, ULARGE_INTEGER* pnBytesWritten);
    STDMETHOD(LockRegion)(ULARGE_INTEGER iOffset, ULARGE_INTEGER nBytes,
	DWORD dwLockType);
    STDMETHOD(Read)(void* pBuffer, ULONG nBytes, ULONG* pnBytesRead);
    STDMETHOD(Revert)();
    STDMETHOD(Seek)(LARGE_INTEGER nDisplacement, DWORD dwOrigin, 
	ULARGE_INTEGER* piNewPosition);
    STDMETHOD(SetSize)(ULARGE_INTEGER nNewSize);
    STDMETHOD(Stat)(STATSTG* pStatStg, DWORD dwFlags);
    STDMETHOD(UnlockRegion)(ULARGE_INTEGER iOffset, ULARGE_INTEGER nBytes,
	DWORD dwLockType);
    STDMETHOD(Write)(const void* pBuffer, ULONG nBytes, 
	ULONG* pnBytesWritten);

protected:
    LONG            m_nRefCount;
    CImgTaskPlug*   m_pFilter;
};


// CImageDecodeEventSink (Private) -----------------------------------------------------------

class CImageDecodeEventSink :
    public IImageDecodeEventSink
{

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CImageDecodeEventSink))

    CImageDecodeEventSink( CImgTaskPlug* pFilter );
    ~CImageDecodeEventSink();

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    STDMETHOD(QueryInterface)(REFIID iid, void** ppInterface);

    STDMETHOD(GetSurface)(LONG nWidth, LONG nHeight, REFGUID bfid, 
	ULONG nPasses, DWORD dwHints, IUnknown** ppSurface);
    STDMETHOD(OnBeginDecode)(DWORD* pdwEvents, ULONG* pnFormats, 
	GUID** ppFormats);
    STDMETHOD(OnBitsComplete)();
    STDMETHOD(OnDecodeComplete)(HRESULT hrStatus);
    STDMETHOD(OnPalette)();
    STDMETHOD(OnProgress)(RECT* pBounds, BOOL bFinal);

protected:
    ULONG                       m_nRefCount;
    CImgTaskPlug*               m_pFilter;
    CComPtr< IDirectDrawSurface > m_pSurface;
};

CImageDecodeEventSink::CImageDecodeEventSink(CImgTaskPlug* pFilter) :
    m_nRefCount(0),
    m_pFilter(pFilter),
    m_pSurface(NULL)
{
}

CImageDecodeEventSink::~CImageDecodeEventSink()
{
}

ULONG CImageDecodeEventSink::AddRef()
{
    m_nRefCount++;

    return (m_nRefCount);
}

ULONG CImageDecodeEventSink::Release()
{
    m_nRefCount--;
    if (m_nRefCount == 0)
    {
	delete this;
	return (0);
    }

    return (m_nRefCount);
}

STDMETHODIMP CImageDecodeEventSink::QueryInterface(REFIID iid, 
   void** ppInterface)
{
    if (ppInterface == NULL)
    {
	return (E_POINTER);
    }

    *ppInterface = NULL;

    if (IsEqualGUID(iid, IID_IUnknown))
    {
	*ppInterface = (IUnknown*)this;
    }
    else if (IsEqualGUID(iid, IID_IImageDecodeEventSink))
    {
	*ppInterface = (IImageDecodeEventSink*)this;
    }
    else
    {
	return (E_NOINTERFACE);
    }

    //  If we're going to return an interface, AddRef it first
    if (*ppInterface)
    {
	((LPUNKNOWN)*ppInterface)->AddRef();
	return S_OK;
    }

    return (S_OK);
}

STDMETHODIMP CImageDecodeEventSink::OnBeginDecode(DWORD* pdwEvents, 
   ULONG* pnFormats, GUID** ppFormats)
{
    GUID* pFormats;

    if (pdwEvents != NULL)
    {
	*pdwEvents = 0;
    }
    if (pnFormats != NULL)
    {
	*pnFormats = 0;
    }
    if (ppFormats != NULL)
    {
	*ppFormats = NULL;
    }
    if (pdwEvents == NULL)
    {
	return (E_POINTER);
    }
    if (pnFormats == NULL)
    {
	return (E_POINTER);
    }
    if (ppFormats == NULL)
    {
	return (E_POINTER);
    }

    if (m_pFilter->_colorMode == 8)
    {
	pFormats = (GUID*)CoTaskMemAlloc(1*sizeof(GUID));
	if(pFormats == NULL)
	{
	    return (E_OUTOFMEMORY);
	}
	
	pFormats[0] = BFID_INDEXED_RGB_8;
	*pnFormats = 1;
    }
    else
    {
	pFormats = (GUID*)CoTaskMemAlloc(2*sizeof(GUID));
	if(pFormats == NULL)
	{
	    return (E_OUTOFMEMORY);
	}
	
	pFormats[0] = BFID_INDEXED_RGB_8;
	pFormats[1] = BFID_RGB_24;
	*pnFormats = 2;
    }

    *ppFormats = pFormats;
    *pdwEvents = IMGDECODE_EVENT_PALETTE|IMGDECODE_EVENT_BITSCOMPLETE
		    |IMGDECODE_EVENT_PROGRESS|IMGDECODE_EVENT_USEDDRAW;

    return (S_OK);
}

STDMETHODIMP CImageDecodeEventSink::OnBitsComplete()
{
    return (S_OK);
}

STDMETHODIMP CImageDecodeEventSink::OnDecodeComplete(HRESULT hrStatus)
{
    return (S_OK);
}

#define LINEBYTES(_wid,_bits) ((((_wid)*(_bits) + 31) / 32) * 4)

STDMETHODIMP CImageDecodeEventSink::GetSurface(LONG nWidth, LONG nHeight, 
    REFGUID bfid, ULONG nPasses, DWORD dwHints, IUnknown** ppSurface)
{
    HRESULT hResult;
    ULONG nBufferSize;
    CImgBitsDIB *pibd;

    (void)nPasses;
    (void)dwHints;
    
    if (ppSurface != NULL)
    {
	*ppSurface = NULL;
    }
    if (ppSurface == NULL)
    {
	return (E_POINTER);
    }

    if (IsEqualGUID(bfid, BFID_INDEXED_RGB_8))
    {
	m_pFilter->m_nBytesPerPixel = 1;
    }
    else if (IsEqualGUID(bfid, BFID_RGB_24))
    {
	m_pFilter->m_nBytesPerPixel = 3;
    }
    else
    {
	return (E_NOINTERFACE);
    }

    m_pFilter->_xWid = nWidth;
    m_pFilter->_yHei = nHeight;
    m_pFilter->m_nPitch = -LONG( LINEBYTES( m_pFilter->_xWid, 
	m_pFilter->m_nBytesPerPixel*8 ) );
    nBufferSize = -(m_pFilter->m_nPitch*(m_pFilter->_yHei));

    pibd = new CImgBitsDIB();
    if (!pibd)
        return E_OUTOFMEMORY;
        
    m_pFilter->_pImgBits = pibd;

    hResult = pibd->AllocDIBSection(m_pFilter->_colorMode == 8 ? 8 : (m_pFilter->m_nBytesPerPixel == 3 ? 24 : 8),
            m_pFilter->_xWid, m_pFilter->_yHei, NULL, 0, -1);

    if (hResult)
        return(hResult);
        
	m_pFilter->m_pbBits = (BYTE *)pibd->GetBits();;

    if (!m_pFilter->m_pbBits)
    {
	return (E_OUTOFMEMORY);
    }

    m_pFilter->m_pbFirstScanLine = m_pFilter->m_pbBits+nBufferSize+
	m_pFilter->m_nPitch;

    hResult = CreateDDrawSurfaceOnDIB(pibd->GetHbm(), &m_pSurface);
    if (FAILED(hResult))
    {
	return( hResult );
    }

    *ppSurface = (IUnknown *)m_pSurface;
    (*ppSurface)->AddRef();

    m_pFilter->m_pSurface = m_pSurface;
    
    m_pFilter->OnSize(nWidth, nHeight, -1);

    if (m_pFilter->_colorMode == 8 && m_pFilter->m_nBytesPerPixel == 1)
    {
	DDCOLORKEY  ddKey;

	ddKey.dwColorSpaceLowValue = ddKey.dwColorSpaceHighValue = g_wIdxTrans;

	m_pSurface->SetColorKey(DDCKEY_SRCBLT, &ddKey);
    }

    return (S_OK);
}

STDMETHODIMP CImageDecodeEventSink::OnPalette()
{
    return (S_OK);
}

STDMETHODIMP CImageDecodeEventSink::OnProgress(RECT* pBounds, BOOL bComplete)
{
    if (pBounds == NULL)
    {
	return (E_INVALIDARG);
    }

    m_pFilter->_yBot = pBounds->bottom-1;
    m_pFilter->OnProg(FALSE,
	m_pFilter->_yBot == m_pFilter->_yHei ? IMGBITS_TOTAL : IMGBITS_PARTIAL,
	FALSE, m_pFilter->_yBot);

    return (S_OK);
}

// CImgTaskPlug (Private) -----------------------------------------------------------

void CImgTaskPlug::Decode(BOOL *pfNonProgressive)
{
    USES_CONVERSION;
    HRESULT hResult;
    CComPtr< IImageDecodeEventSink > pDitherer;
    CComPtr< IDithererImpl > pDithererImpl;
    CComPtr< IStream > pStream;
    CComPtr< IStream > pOutStream;
    CComPtr< IMapMIMEToCLSID > pMap;
    CComPtr< IImageDecodeEventSink > pEventSink;

    m_pEventSink = (IImageDecodeEventSink*)new CImageDecodeEventSink(this);
    if (m_pEventSink == NULL)
 
    {
	goto Cleanup;
    }

    pStream = (IStream*)new CPlugStream(this);
    if (pStream == NULL)
    {
	goto Cleanup;
    }

    hResult = GetImgTaskExec()->RequestCoInit();
    if (FAILED(hResult))
    {
	goto Cleanup;
    }

    if (_colorMode == 8)
    {
	hResult = CoCreateInstance(CLSID_CoDitherToRGB8, NULL, 
	    CLSCTX_INPROC_SERVER, IID_IDithererImpl, (void**)&pDithererImpl);
	if (FAILED(hResult))
	{
	    goto Cleanup;
	}

	pDithererImpl->SetEventSink(m_pEventSink);

	hResult = SetColorTable(pDithererImpl);
	if (FAILED(hResult))
	{
	    goto Cleanup;
	}

	pDithererImpl->QueryInterface(IID_IImageDecodeEventSink, 
	    (void**)&pEventSink);
    }
    else
    {
	pEventSink = m_pEventSink;
    }

    hResult = DecodeImage(pStream, NULL, pEventSink);
    if (FAILED(hResult))
    {
	goto Cleanup;
    }

    _yBot = _yHei;
    _ySrcBot = -1;

    GetTransIndex();

    OnProg(TRUE, IMGBITS_TOTAL, FALSE, _yBot);

Cleanup:
    return;
}

void
CImgTaskPlug::BltDib(HDC hdc, RECT * prcDst, RECT * prcSrc, DWORD dwRop, DWORD dwFlags)
{
    if (!m_bGotTrans)
	GetTransIndex();
	
    _pImgBits->StretchBlt(hdc, prcDst, prcSrc, dwRop, dwFlags);
}

HRESULT CImgTaskPlug::GetTransIndex()
{
    // If we dithered the image the transparent index has changed to
    // match our destination palette.  Otherwise it is still the 
    // transparent index in the surface.  If the surface is not 8bpp
    // don't bother asking for the transparent index - ImgBltDib() doesn't
    // handle that case.

    if (m_bGotTrans)
	return S_OK;

    m_bGotTrans = TRUE;
    
    if (_colorMode == 8)
    {
        _lTrans = g_wIdxTrans;
        ((CImgBitsDIB *)_pImgBits)->SetTransIndex(_lTrans);
    }
    else if (m_nBytesPerPixel == 1 && m_pSurface)
    {
        DDCOLORKEY ddKey;

        m_pSurface->GetColorKey(DDCKEY_SRCBLT, &ddKey);
        _lTrans = (LONG)ddKey.dwColorSpaceLowValue;
        ((CImgBitsDIB *)_pImgBits)->SetTransIndex(_lTrans);
    }

    return S_OK;
}

BOOL
IsPluginImgFormat(BYTE * pb, UINT cb)
{
    UINT nFormat;
    return(IdentifyMIMEType(pb, cb, &nFormat) == S_OK);
}

CImgTask* NewImgTaskPlug()
{
    return(new CImgTaskPlug);
}

HRESULT CImgTaskPlug::SetColorTable(IDithererImpl* pDitherer)
{
    HRESULT hResult;

    _nColors = g_lpHalftone.wCnt;
    memcpy(_ape, g_lpHalftone.ape, _nColors * sizeof(PALETTEENTRY));

    hResult = pDitherer->SetDestColorTable(_nColors, g_rgbHalftone);
    if (FAILED(hResult))
    {
	return (hResult);
    }

    return (S_OK);
}

// CPlugStream (Private) -----------------------------------------------------------

CPlugStream::CPlugStream(CImgTaskPlug* pFilter) :
    m_pFilter(pFilter),
    m_nRefCount(0)
{
}

CPlugStream::~CPlugStream()
{
}

ULONG CPlugStream::AddRef()
{
    m_nRefCount++;

    return (m_nRefCount);
}

ULONG CPlugStream::Release()
{
    m_nRefCount--;
    if (m_nRefCount == 0)
    {
	delete this;
	return (0);
    }

    return (m_nRefCount);
}

STDMETHODIMP CPlugStream::QueryInterface(REFIID iid, void** ppInterface)
{
    if (ppInterface == NULL)
    {
	return (E_POINTER);
    }

    *ppInterface = NULL;

    if (IsEqualGUID(iid, IID_IUnknown))
    {
	*ppInterface = (IUnknown*)this;
    }
    else if (IsEqualGUID(iid, IID_IStream))
    {
	*ppInterface = (IStream*)this;
    }
    else
    {
	return (E_NOINTERFACE);
    }

    ((LPUNKNOWN)*ppInterface)->AddRef();
    return (S_OK);
}

STDMETHODIMP CPlugStream::Clone(IStream** ppStream)
{
    if (ppStream == NULL)
    {
	return (E_POINTER);
    }

    *ppStream = NULL;

    return (E_NOTIMPL);
}

STDMETHODIMP CPlugStream::Commit(DWORD dwFlags)
{
    return (E_NOTIMPL);
}

STDMETHODIMP CPlugStream::CopyTo(IStream* pStream, ULARGE_INTEGER nBytes,
    ULARGE_INTEGER* pnBytesRead, ULARGE_INTEGER* pnBytesWritten)
{
    return (E_NOTIMPL);
}

STDMETHODIMP CPlugStream::LockRegion(ULARGE_INTEGER iOffset, 
    ULARGE_INTEGER nBytes, DWORD dwLockType)
{
    return (E_NOTIMPL);
}

STDMETHODIMP CPlugStream::Read(void* pBuffer, ULONG nBytes, 
   ULONG* pnBytesRead)
{
    HRESULT hResult;

    if (pnBytesRead != NULL)
    {
	*pnBytesRead = 0;
    }
    if (pBuffer == NULL)
    {
	return (E_POINTER);
    }
    if (nBytes == 0)
    {
	return (E_INVALIDARG);
    }

    hResult = m_pFilter->Read(pBuffer, nBytes, pnBytesRead);
    if (FAILED(hResult))
    {
	return (hResult);
    }

    if (*pnBytesRead < nBytes)
    {
	return (S_FALSE);
    }

    return (S_OK);
}

STDMETHODIMP CPlugStream::Revert()
{
    return (E_NOTIMPL);
}

STDMETHODIMP CPlugStream::Seek(LARGE_INTEGER nDisplacement, DWORD dwOrigin,
    ULARGE_INTEGER* piNewPosition)
{
    return (E_NOTIMPL);
}

STDMETHODIMP CPlugStream::SetSize(ULARGE_INTEGER nNewSize)
{
    return (E_NOTIMPL);
}

STDMETHODIMP CPlugStream::Stat(STATSTG* pStatStg, DWORD dwFlags)
{
    return (E_NOTIMPL);
}

STDMETHODIMP CPlugStream::UnlockRegion(ULARGE_INTEGER iOffset, 
    ULARGE_INTEGER nBytes, DWORD dwLockType)
{
    return (E_NOTIMPL);
}

STDMETHODIMP CPlugStream::Write(const void* pBuffer, ULONG nBytes, 
    ULONG* pnBytesWritten)
{
    return (E_NOTIMPL);
}

// IImageDecodeFilter support -----------------------------------------------------

struct BFID_ENTRY {
    int bpp;
    const GUID *pBFID;
};

BFID_ENTRY  BFIDInfo[] =
{
    { 1, &BFID_MONOCHROME },
    { 4, &BFID_RGB_4 },
    { 8, &BFID_RGB_8 },
    { 15, &BFID_RGB_555 },
    { 16, &BFID_RGB_565 },
    { 24, &BFID_RGB_24 },
    { 32, &BFID_RGB_32 }
};

class CImageDecodeFilter : public CBaseFT, public IImageDecodeFilter
{
    typedef CBaseFT super;
    
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CImageDecodeFilter))

    // IUnknown members

    STDMETHOD(QueryInterface)(REFIID, void **);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();
    
    // IImageDecodeFilter methods
    STDMETHOD( Initialize )( IImageDecodeEventSink* pEventSink );
    STDMETHOD( Process )( IStream* pStream );
    STDMETHOD( Terminate )( HRESULT hrStatus );

    // Worker methods

    BFID_ENTRY * MatchBFID(int bpp);
    BFID_ENTRY * MatchBFIDtoBFID(GUID *pBFID);
    HRESULT LockBits(RECT *prcBounds, DWORD dwLockFlags, void **ppBits, long *pPitch);
    HRESULT UnlockBits(RECT *prcBounds, void *pBits);
    HRESULT EnsureSurface();
    HRESULT CopyRect(RECT *prcBounds);
    void OnComplete(HRESULT hr);
    void ImageCallback();

    // Worker thread methods
    
    static DWORD WINAPI ThreadProc(LPVOID lpData);
    STDMETHOD( ProcessOnThread )( );
        
    CComPtr < IImageDecodeEventSink > m_pEventSink;
    DWORD                   m_dwEvents;
    ULONG                   m_nFormats;
    GUID *                  m_pFormats;
    ULONG                   m_ulState;
    CComPtr < IDirectDrawSurface >    m_pSurface;
    CImgCtx *               m_pImgCtx;
    LONG                    m_xWidth;
    LONG                    m_yHeight;
    LONG                    m_iBitCount;
    void *                  m_pvBits;
    RECT                    m_rcUpdate;
    HANDLE                  m_hMainEvent;
    HANDLE                  m_hWorkerEvent;
    enum { Ready, Callback, Done } m_State;
};

BFID_ENTRY * CImageDecodeFilter::MatchBFID(int bpp)
{
    BFID_ENTRY *pBFIDEntry = NULL;
    ULONG i;

    for (i = 0; i < ARRAY_SIZE(BFIDInfo); ++i)
    {
        if (BFIDInfo[i].bpp == bpp)
        {
            pBFIDEntry = BFIDInfo + i;
            break;
        }
    }

    if (pBFIDEntry)
    {
        for (i = 0; i < m_nFormats; ++i)
        {
            if (IsEqualGUID(*pBFIDEntry->pBFID, m_pFormats[i]))
            return pBFIDEntry;
        }
    }
    
    return NULL;
}

BFID_ENTRY * CImageDecodeFilter::MatchBFIDtoBFID(GUID *pBFID)
{
    ULONG i;

    for (i = 0; i < ARRAY_SIZE(BFIDInfo); ++i)
    {
        if (IsEqualGUID(*BFIDInfo[i].pBFID, *pBFID))
            return BFIDInfo + i;
    }

    return NULL;
}

HRESULT CImageDecodeFilter::LockBits(RECT *prcBounds, DWORD dwLockFlags, void **ppBits
, long *pPitch)
{
    HRESULT hResult;
    
    DDSURFACEDESC   ddsd;

    ddsd.dwSize = sizeof(ddsd);
    hResult = m_pSurface->Lock(prcBounds, &ddsd, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
    if (FAILED(hResult))
        return hResult;

    *ppBits = ddsd.lpSurface;
    *pPitch = ddsd.lPitch;

    return S_OK;
}

HRESULT CImageDecodeFilter::UnlockBits(RECT *prcBounds, void *pBits)
{
    return m_pSurface->Unlock(pBits);
}


HRESULT CImageDecodeFilter::EnsureSurface()
{
    HRESULT hr;
    LPDIRECTDRAWPALETTE pDDPalette;
    HRESULT             hResult;
    CImgBits *pImgBits;
    
    if (m_pSurface == NULL)
    {
        CImgInfo *pImgInfo = (CImgInfo *)m_pImgCtx->GetImgId();

        pImgInfo->GetImageAndMask(&pImgBits);

        BFID_ENTRY  *pBFIDEntry;

        // we can only create a surface if we have DIB-formatted bits
        m_pvBits = pImgBits->GetBits();
        if (!m_pvBits)
            return E_FAIL;
            
        IUnknown *pUnk;
        DDCOLORKEY ddkey;
    
        pBFIDEntry = MatchBFID(pImgBits->Depth());
        if (pBFIDEntry == NULL)
            return E_FAIL;

        m_iBitCount = pImgBits->Depth();
        if (m_iBitCount == 15)
            m_iBitCount = 16;

        m_xWidth = pImgBits->Width();
        m_yHeight = pImgBits->Height();
    
        hr = m_pEventSink->GetSurface(m_xWidth, 
                m_yHeight, *pBFIDEntry->pBFID,
                1, IMGDECODE_HINT_TOPDOWN | IMGDECODE_HINT_FULLWIDTH,
                &pUnk);
        if (FAILED(hr))
            return hr;
    
        pUnk->QueryInterface(IID_IDirectDrawSurface, (void **)&m_pSurface);
        pUnk->Release();

        if (m_pSurface == NULL)
            return E_FAIL;

        // Start the transparent index out of range (opaque)
        ddkey.dwColorSpaceLowValue = 0xffffffff;
        ddkey.dwColorSpaceHighValue = 0xffffffff;
        m_pSurface->SetColorKey(DDCKEY_SRCBLT, &ddkey);
    
        // Set the palette if there is one
        hResult = m_pSurface->GetPalette(&pDDPalette);
        if (SUCCEEDED(hResult))
        {
            RGBQUAD             argb[256];
            PALETTEENTRY        ape[256];
            LONG                cColors;

            cColors = pImgBits->Colors();
            if (cColors > 0)
            {
                if (cColors > 256)
                    cColors = 256;

                pImgBits->GetColors(0, cColors, argb);
                CopyPaletteEntriesFromColors(ape, argb, cColors);
                pDDPalette->SetEntries(0, 0, cColors, ape);
                pDDPalette->Release();
            }
        }

        // Notify the caller if necessary
        
        if (m_dwEvents & IMGDECODE_EVENT_PALETTE)
            m_pEventSink->OnPalette();
    }

    return S_OK;
}

void CImageDecodeFilter::OnComplete(HRESULT hr)
{
    if (SUCCEEDED(hr) && (m_dwEvents & IMGDECODE_EVENT_BITSCOMPLETE))
        m_pEventSink->OnBitsComplete();

    m_pEventSink->OnDecodeComplete(hr);
    SetEvent(m_hWorkerEvent);
}

HRESULT CImageDecodeFilter::CopyRect(RECT *prcBounds)
{
    BYTE    *pbBits, *pbSrc, *pbDst;
    long     cbRowDst, cbRowSrc, i, y1, y2;
    HRESULT hr;

    hr = EnsureSurface();
    if (FAILED(hr))
        return hr;

    hr = LockBits(prcBounds, 0, (void **)&pbBits, &cbRowDst);
    if (FAILED(hr))
        return hr;

    y1 = prcBounds->top;
    y2 = prcBounds->bottom;
    if (y1 > y2)
    {
        long t;

        t = y1;
        y1 = y2 + 1;        // maintain inclusive/exclusive relationship
        y2 = t + 1;
    }

    cbRowSrc = -(((m_xWidth * m_iBitCount + 31) & ~31) / 8);
    pbSrc = (BYTE *)m_pvBits - cbRowSrc * (m_yHeight - y1 - 1);
    
    pbDst = pbBits;
    if (cbRowDst < 0)
        pbDst -= cbRowDst * (m_yHeight - y1 - 1);

    for (i = y1; i < y2; ++i)
    {
        memcpy(pbDst, pbSrc, -cbRowSrc);    
        pbSrc += cbRowSrc;
        pbDst += cbRowDst;
    }
    
    UnlockBits(prcBounds, pbBits);

    return S_OK;
}

void IImageDecodeFilter_CallbackOnThread(void *pv1, void *pv2)
{
    CImageDecodeFilter *pImgFilter = (CImageDecodeFilter *)pv2;

    pImgFilter->m_State = CImageDecodeFilter::Callback;
    SetEvent(pImgFilter->m_hMainEvent);
}

void CImageDecodeFilter::ImageCallback()
{
    RECT    rcBounds;
    HRESULT hr;
    DDCOLORKEY ddKey;

    m_pImgCtx->GetStateInfo(&m_ulState, NULL, TRUE);

    // First see if there is an error or the image is done decoding.
    
    if (m_ulState & IMGLOAD_COMPLETE)
    {
        CImgInfo *pImgInfo = (CImgInfo *)m_pImgCtx->GetImgId();
	
        // Transfer remaining data to buffer since last view change
        hr = EnsureSurface();
        if (SUCCEEDED(hr))
        {
            rcBounds.left = 0;
            rcBounds.top = 0;
            rcBounds.right = m_xWidth;
            rcBounds.bottom = m_yHeight;
            SubtractRect(&rcBounds, &rcBounds, &m_rcUpdate);

            hr = CopyRect(&rcBounds);

            if (pImgInfo->GetTrans())
            {
                ddKey.dwColorSpaceLowValue = ddKey.dwColorSpaceHighValue = pImgInfo->GetTrans();
                m_pSurface->SetColorKey(DDCKEY_SRCBLT, &ddKey);
            }
        }

        OnComplete(hr);
        return;
    }
    else if (m_ulState & IMGLOAD_ERROR)
    {
        OnComplete(E_FAIL);
        return;
    }

    // If we haven't made the surface yet do it now (responding to IMGCHG_VIEW)

    if (FAILED(EnsureSurface()))
    {
        OnComplete(E_FAIL);
        return;
    }
    
    // Transfer image data and send progress event 

    if ((m_dwEvents & IMGDECODE_EVENT_PROGRESS) && m_pSurface)
    {
        RECT    rcBounds[2], rcImg;
        LONG    nRects, i;

        rcImg.left = 0;
        rcImg.top = 0;
        rcImg.right = m_xWidth;
        rcImg.bottom = m_yHeight;

        m_pImgCtx->GetUpdateRects(rcBounds, &rcImg, &nRects);

        // If we call GetUpdateRects before the decoder has filled anything in
        // (_yTop == -1) the function returns the entire image rectangle.  We 
        // need to detect this case and skip over this notification.
	
        if (IsRectEmpty(&m_rcUpdate) 
            && nRects > 0 
            && EqualRect(&rcImg, rcBounds))
            return;
	    
        for (i = 0; i < nRects; ++i)
        {
            hr = CopyRect(&rcBounds[i]);
            if (FAILED(hr))
            {
                OnComplete(hr);
                return;
            }
            
            m_pEventSink->OnProgress(&rcBounds[i], TRUE);
            UnionRect(&m_rcUpdate, &m_rcUpdate, &rcBounds[i]);
        }
    }
}

STDMETHODIMP
CImageDecodeFilter::QueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IImageDecodeFilter || riid == IID_IUnknown)
    {
        *ppv = (IUnknown *)this;
        ((LPUNKNOWN)*ppv)->AddRef();
        return(S_OK);
    }
    else
    {
        *ppv = NULL;
        return(E_NOINTERFACE);
    }
}

STDMETHODIMP_(ULONG)
CImageDecodeFilter::AddRef()
{
    return(super::AddRef());
}

STDMETHODIMP_(ULONG)
CImageDecodeFilter::Release()
{
    return(super::Release());
}

STDMETHODIMP
CImageDecodeFilter::Initialize(IImageDecodeEventSink* pEventSink)
{
    HRESULT hr;
    
    if (pEventSink == NULL)
        return E_INVALIDARG;

    m_pEventSink = pEventSink;

    hr = pEventSink->OnBeginDecode(&m_dwEvents, &m_nFormats, &m_pFormats);

    return hr;
}

STDMETHODIMP
CImageDecodeFilter::Process(IStream* pStream)
{
    DWORD dwTID;
    HRESULT hr = E_FAIL;
    HANDLE hThread;
    CDwnDoc *pDwnDoc;
    CDwnCtx *pDwnCtx;
    CDwnLoad *pDwnLoad;
    BFID_ENTRY  *pBFIDEntry = NULL;
    DWNLOADINFO dwnInfo;
    ULONG i;

    ZeroMemory(&dwnInfo, sizeof(dwnInfo));
    dwnInfo.pstm = pStream;

    pDwnDoc = new CDwnDoc;
    if (pDwnDoc == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup1;
    }

    dwnInfo.pDwnDoc = pDwnDoc;
    dwnInfo.pchUrl = TEXT("");

    for (i = 0; i < m_nFormats; ++i)
    {
        pBFIDEntry = MatchBFIDtoBFID(&m_pFormats[i]);
        if (pBFIDEntry)
            break;
    }
    if (pBFIDEntry == NULL)
    {
        hr = E_FAIL;
        goto cleanup2;
    }
	
    pDwnDoc->SetDownf(pBFIDEntry->bpp | DWNF_RAWIMAGE);

    hr = NewDwnCtx(DWNCTX_IMG, TRUE, &dwnInfo, &pDwnCtx);
    if (FAILED(hr))
        goto cleanup2;

    m_pImgCtx = (CImgCtx *)pDwnCtx;

    // Keep the CDwnLoad around so it can passivate on this
    // thread.  If you don't, it has to post a message to
    // get back to this thread and we don't work well in
    // console mode apps

    // Returns addref'd CDwnLoad
    pDwnLoad = m_pImgCtx->GetDwnLoad();
    
    m_hMainEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!m_hMainEvent)
        goto cleanup3;

    m_hWorkerEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!m_hWorkerEvent)
        goto cleanup4;
        
    m_State = Ready;
    
    hThread = CreateThread(NULL, 0, ThreadProc, this, 0, &dwTID);
    if (hThread == NULL)
        goto cleanup5;

    while (m_State != Done)
    {
        WaitForSingleObject(m_hMainEvent, INFINITE);

        while (m_State == Callback)
        {
            m_State = Ready;
            ImageCallback();
        }
    }
    
    GetExitCodeThread(hThread, (DWORD *)&hr);
    
    CloseHandle(hThread);

cleanup5:
    CloseHandle(m_hWorkerEvent);

cleanup4:
    CloseHandle(m_hMainEvent);

cleanup3:    
    if (pDwnLoad)
    {
        pDwnLoad->Release();
    }
    if (pDwnCtx)
    {
        pDwnCtx->Release();
    }
    
cleanup2:    
    pDwnDoc->Release();
    
cleanup1:
    return hr;
}

DWORD WINAPI CImageDecodeFilter::ThreadProc(LPVOID lpData)
{
    DWORD dwReturn;
    CImageDecodeFilter *pFilter = (CImageDecodeFilter *)lpData;

    dwReturn = CoInitialize(NULL);
    if (SUCCEEDED(dwReturn))
    {    
        dwReturn = pFilter->ProcessOnThread();
        CoUninitialize();
    }

    return dwReturn;
}

STDMETHODIMP
CImageDecodeFilter::ProcessOnThread()
{
    DWORD   dwResult;
    BOOL    fActive = TRUE;
    
    m_pImgCtx->SetCallback(IImageDecodeFilter_CallbackOnThread, this);
    m_pImgCtx->SelectChanges(IMGCHG_VIEW | IMGCHG_COMPLETE, 0, TRUE);

    while (fActive)
    {
        dwResult = MsgWaitForMultipleObjects(1, &m_hWorkerEvent, FALSE, INFINITE, 
                        QS_ALLINPUT);
        if (dwResult == WAIT_OBJECT_0 + 1)
        {
            MSG msg;

            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    fActive = FALSE;
                    break;
                }
                else
                    DispatchMessage(&msg);
            }
        }
        else
        {
            fActive = FALSE;
        }
    }

    m_pImgCtx->Disconnect();
    
    m_State = Done;
    SetEvent(m_hMainEvent);
    
    return S_OK;
}

STDMETHODIMP
CImageDecodeFilter::Terminate(HRESULT hrStatus)
{
	if (m_pFormats)
		CoTaskMemFree(m_pFormats);

    return S_OK;
}

STDMETHODIMP
CreateIImageDecodeFilter(IUnknown * pUnkOuter, IUnknown **ppUnk)
{
    if (pUnkOuter != NULL)
    {
        *ppUnk = NULL;
        return(CLASS_E_NOAGGREGATION);
    }

    CImageDecodeFilter * pImgFilter = new CImageDecodeFilter;

    *ppUnk = pImgFilter;

    RRETURN(pImgFilter ? S_OK : E_OUTOFMEMORY);
}
