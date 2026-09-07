// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------

//
//  File: exports.cpp
//------------------------------------------------------------------------------

#include "precomp.hpp"


ULONG
MILAddRef(
    __inout_ecount(1) IUnknown* pIUnknown
    )
{
    return pIUnknown->AddRef();
}

ULONG
MILRelease(
    __inout_ecount(1) IUnknown* pIUnknown
    )
{
    return pIUnknown->Release();
}

HRESULT
MILQueryInterface(
    __inout_ecount(1) IUnknown* pIUnknown,
    __in_ecount(1) REFIID riid,
    __deref_out void** ppvObject
    )
{
    HRESULT hr = S_OK;
    CHECKPTRARG(pIUnknown);
    CHECKPTRARG(ppvObject);

    // We initialize this here because we treat it in managed code
    // as an out param and can't therefore intialize it from managed code.

    (*ppvObject) = NULL;

    hr = pIUnknown->QueryInterface(riid, ppvObject);

Cleanup:
    RRETURN(hr);
}

HRESULT
MILFactoryCreateBitmapRenderTarget(
    __in_ecount(1) IMILCoreFactory* THIS_PTR,
    UINT width,
    UINT height,
    MilPixelFormat::Enum format,
    FLOAT dpiX,
    FLOAT dpiY,
    MilRTInitialization::Flags dwFlags,
    __deref_out_ecount(1) IMILRenderTargetBitmap ** const ppIRenderTargetBitmap
    )
{
    if (THIS_PTR == NULL) RRETURN(E_INVALIDARG);
    RRETURN(THIS_PTR->CreateBitmapRenderTarget(
        width,
        height,
        format,
        dpiX,
        dpiY,
        dwFlags,
        ppIRenderTargetBitmap));
}

HRESULT
MILFactoryCreateMediaPlayer(
    __in_ecount(1) IMILCoreFactory* THIS_PTR,
    __inout_ecount(1) IUnknown *pEventProxy,
    bool canOpenAnyMedia,
    __deref_out_ecount(1) IMILMedia ** const ppMedia
    )
{
    if (THIS_PTR == NULL) RRETURN(E_INVALIDARG);
    RRETURN(THIS_PTR->CreateMediaPlayer(
        pEventProxy,
        canOpenAnyMedia,
        ppMedia));
}



HRESULT
MILFactoryCreateSWRenderTargetForBitmap(
    __in_ecount(1) IMILCoreFactory* THIS_PTR,
    __inout_ecount(1) IWICBitmap *pIBitmap,
    __deref_out_ecount(1) IMILRenderTargetBitmap ** const ppIRenderTargetBitmap
    )
{
    if (THIS_PTR == NULL) RRETURN(E_INVALIDARG);
    RRETURN(THIS_PTR->CreateSWRenderTargetForBitmap(pIBitmap, ppIRenderTargetBitmap));
}

/***************************************************************************************
    We currently support loading images from resources linked to the executable.
    Support to load resources from other dlls in the app to come later.
    The image resources should be specified with resource type IMAGE and
    the resourceID should contain the correct file extension.
    Support for loading using any resource type and using ordinals will be added later
***************************************************************************************/
HRESULT
MILLoadResource(
    __in LPCWSTR src,
    __deref_out_bcount(*size) LPBYTE *memPtr,
    __out_ecount(1) long *size
    )
{
    HRESULT hr      = S_OK;
    HGLOBAL hGlobal = NULL;

    HRSRC hRes = FindResource(NULL , src, L"IMAGE");
    if (hRes == NULL)
        RRETURN(E_FAIL);

    hGlobal = LoadResource(NULL, hRes);
    if (hGlobal == NULL)
        RRETURN(E_FAIL);

    *memPtr = (BYTE*)LockResource(hGlobal);
    *size = SizeofResource(NULL, hRes);

    RRETURN(hr);
}

//------------------------------------------------------------------------------
//  IMILRenderTargetBitmap
//------------------------------------------------------------------------------

HRESULT
MILRenderTargetBitmapGetBitmap(
    __in_ecount(1) IMILRenderTargetBitmap* THIS_PTR,
    __deref_out_ecount(1) IWICBitmap ** const ppIBitmap)
{
    HRESULT hr = S_OK;
    IWGXBitmap *pWGXBitmap = NULL;

    CHECKPTRARG(THIS_PTR);
    
    IFC(THIS_PTR->GetBitmap(&pWGXBitmap));
    IFC(CWGXWrapperBitmap::Create(
        pWGXBitmap,
        ppIBitmap
        ));

Cleanup:  
    ReleaseInterface(pWGXBitmap);
    
    RRETURN(hr);
}

//------------------------------------------------------------------------------
//  IMILRenderTargetBitmap
//------------------------------------------------------------------------------

HRESULT
MILRenderTargetBitmapClear(
    __inout_ecount(1) IMILRenderTargetBitmap* THIS_PTR)
{
    // This routine involves rendering so we need standard
    // FPU precision setting (24 bits).
    CFloatFPU oGuard;

    if (THIS_PTR == NULL) RRETURN(E_INVALIDARG);
    HRESULT hr = S_OK;

    IMILRenderTarget* pRenderTarget = NULL;
    if (SUCCEEDED(hr))
    {
        MIL_THR(THIS_PTR->QueryInterface(IID_IMILRenderTarget, (void **)&pRenderTarget));
    }

    if (SUCCEEDED(hr))
    {
        CAliasedClip aliasedClip(NULL);
        MilColorF colTransparent = {0.0f, 0.0f, 0.0f, 0.0f};
        MIL_THR(pRenderTarget->Clear(&colTransparent, &aliasedClip));
    }

    if (pRenderTarget)
    {
        pRenderTarget->Release();
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------------
//  IMILMedia
//------------------------------------------------------------------------------
HRESULT
MILMediaOpen(
    __inout_ecount(1) IMILMedia* THIS_PTR,
    __in LPOLESTR src
    )
{
    if (THIS_PTR == NULL) RRETURN(E_INVALIDARG);
    RRETURN(THIS_PTR->Open(src));
}


HRESULT
MILMediaStop(
    __inout_ecount(1) IMILMedia* THIS_PTR
    )
{
    if (THIS_PTR == NULL) RRETURN(E_INVALIDARG);
    RRETURN(THIS_PTR->Stop());
}

HRESULT
MILMediaClose(
    __inout_ecount(1) IMILMedia   *THIS_PTR
    )
{
    if (THIS_PTR == NULL) RRETURN(E_INVALIDARG);
    RRETURN(THIS_PTR->Close());
}

HRESULT
MILMediaGetPosition(
    __inout_ecount(1) IMILMedia* THIS_PTR,
    __out_ecount(1) LONGLONG *pllTime
    )
{
    if (THIS_PTR == NULL) RRETURN(E_INVALIDARG);
    RRETURN(THIS_PTR->GetPosition(pllTime));
}


HRESULT
MILMediaSetPosition(
    __inout_ecount(1) IMILMedia* THIS_PTR,
    LONGLONG llTime
    )
{
    if (THIS_PTR == NULL) RRETURN(E_INVALIDARG);
    RRETURN(THIS_PTR->SetPosition(llTime));
}


HRESULT
MILMediaSetRate(
    __inout_ecount(1) IMILMedia* THIS_PTR,
    double dblRate
    )
{
    if (THIS_PTR == NULL) RRETURN(E_INVALIDARG);
    RRETURN(THIS_PTR->SetRate(dblRate));
}


HRESULT
MILMediaSetVolume(
    __inout_ecount(1) IMILMedia* THIS_PTR,
    double dblVolume
    )
{
    if (THIS_PTR == NULL) RRETURN(E_INVALIDARG);
    RRETURN(THIS_PTR->SetVolume(dblVolume));
}


HRESULT
MILMediaSetBalance(
    __inout_ecount(1) IMILMedia* THIS_PTR,
    double dblBalance
    )
{
    if (THIS_PTR == NULL) RRETURN(E_INVALIDARG);
    RRETURN(THIS_PTR->SetBalance(dblBalance));
}

HRESULT
MILMediaSetIsScrubbingEnabled(
    __inout_ecount(1) IMILMedia* THIS_PTR,
    bool isScrubbingEnabled
    )
{
    if (THIS_PTR == NULL) RRETURN(E_INVALIDARG);
    RRETURN(THIS_PTR->SetIsScrubbingEnabled(isScrubbingEnabled));
}


HRESULT
MILMediaIsBuffering(
    __inout_ecount(1) IMILMedia* THIS_PTR,
    __out_ecount(1) bool *pIsBuffering
    )
{
    if (THIS_PTR == NULL) RRETURN(E_INVALIDARG);
    RRETURN(THIS_PTR->IsBuffering(pIsBuffering));
}


HRESULT
MILMediaCanPause(
    __inout_ecount(1) IMILMedia* THIS_PTR,
    __out_ecount(1) bool *pCanPause
    )
{
    if (THIS_PTR == NULL) RRETURN(E_INVALIDARG);
    RRETURN(THIS_PTR->CanPause(pCanPause));
}


HRESULT
MILMediaGetDownloadProgress(
    __inout_ecount(1) IMILMedia* THIS_PTR,
    __out_ecount(1) double *pProgress
    )
{
    if (THIS_PTR == NULL) RRETURN(E_INVALIDARG);
    RRETURN(THIS_PTR->GetDownloadProgress(pProgress));
}


HRESULT
MILMediaGetBufferingProgress(
    __inout_ecount(1) IMILMedia* THIS_PTR,
    __out_ecount(1) double *pProgress
    )
{
    if (THIS_PTR == NULL) RRETURN(E_INVALIDARG);
    RRETURN(THIS_PTR->GetBufferingProgress(pProgress));
}


HRESULT
MILMediaHasVideo(
    __inout_ecount(1) IMILMedia* THIS_PTR,
    __out_ecount(1) bool *pfHasVideo
    )
{
    if (THIS_PTR == NULL) RRETURN(E_INVALIDARG);
    RRETURN(THIS_PTR->HasVideo(pfHasVideo));
}


HRESULT
MILMediaHasAudio(
    __inout_ecount(1) IMILMedia* THIS_PTR,
    __out_ecount(1) bool *pfHasAudio
    )
{
    if (THIS_PTR == NULL) RRETURN(E_INVALIDARG);
    RRETURN(THIS_PTR->HasAudio(pfHasAudio));
}


HRESULT
MILMediaGetNaturalHeight(
    __inout_ecount(1) IMILMedia* THIS_PTR,
    __out_ecount(1) UINT *puiHeight
    )
{
    if (THIS_PTR == NULL) RRETURN(E_INVALIDARG);
    RRETURN(THIS_PTR->GetNaturalHeight(puiHeight));
}


HRESULT
MILMediaGetNaturalWidth(
    __inout_ecount(1) IMILMedia* THIS_PTR,
    __out_ecount(1) UINT *puiWidth
    )
{
    if (THIS_PTR == NULL) RRETURN(E_INVALIDARG);
    RRETURN(THIS_PTR->GetNaturalWidth(puiWidth));
}


HRESULT
MILMediaGetMediaLength(
    __inout_ecount(1) IMILMedia* THIS_PTR,
    __out_ecount(1) LONGLONG *pllLength
    )
{
    if (THIS_PTR == NULL) RRETURN(E_INVALIDARG);
    RRETURN(THIS_PTR->GetMediaLength(pllLength));
}

HRESULT
MILMediaNeedUIFrameUpdate(
                        IMILMedia* THIS_PTR
                        )
{
    if (THIS_PTR == NULL) RRETURN(E_INVALIDARG);
    RRETURN(THIS_PTR->NeedUIFrameUpdate());
}


HRESULT
MILMediaShutdown(
    __inout_ecount(1) IMILMedia* THIS_PTR
    )
{
    if (THIS_PTR == NULL) RRETURN(E_INVALIDARG);
    RRETURN(THIS_PTR->Shutdown());
}

HRESULT
MILMediaProcessExitHandler(
    IMILMedia* THIS_PTR
    )
{
    if (THIS_PTR == NULL) RRETURN(E_INVALIDARG);
    RRETURN(THIS_PTR->ProcessExitHandler());
}

//------------------------------------------------------------------------------
//  IMILSwDoubleBufferedBitmap
//------------------------------------------------------------------------------

HRESULT
MILSwDoubleBufferedBitmapCreate(
    UINT width,
    UINT height,
    double dpiX,
    double dpiY,
    __in REFWICPixelFormatGUID pixelFormat,
    __in_opt IWICPalette *pPalette,
    __deref_out CSwDoubleBufferedBitmap ** const ppSwDoubleBufferedBitmap
    )
{
    HRESULT hr = S_OK;
    MilPixelFormat::Enum fmtMIL;

    CHECKPTR(ppSwDoubleBufferedBitmap);

    IFC(WicPfToMil(pixelFormat, &fmtMIL));

    IFC(CSwDoubleBufferedBitmap::Create(
        width,
        height,
        dpiX,
        dpiY,
        fmtMIL,
        pPalette,
        ppSwDoubleBufferedBitmap
        ));

Cleanup:

    RRETURN(hr);
}

HRESULT
MILSwDoubleBufferedBitmapGetBackBuffer(
    __in CSwDoubleBufferedBitmap const * THIS_PTR,
    __deref_out IWICBitmap **ppBackBuffer,
    __out UINT * pBackBufferSize
    )
{
    HRESULT hr = S_OK;

    CHECKPTR(THIS_PTR);
    CHECKPTR(ppBackBuffer);

    THIS_PTR->GetBackBuffer(ppBackBuffer, pBackBufferSize);

Cleanup:

    RRETURN(hr);
}

HRESULT
MILSwDoubleBufferedBitmapAddDirtyRect(
    __in CSwDoubleBufferedBitmap * THIS_PTR,
    __in const MILRect *pRect
    )
{
    HRESULT hr = S_OK;
    UINT x = 0;
    UINT y = 0;
    UINT width = 0;
    UINT height = 0;
    CMilRectU rcDirty;

    CHECKPTR(THIS_PTR);
    CHECKPTR(pRect);

    IFC(IntToUInt(pRect->X, &x));
    IFC(IntToUInt(pRect->Y, &y));
    IFC(IntToUInt(pRect->Width, &width));
    IFC(IntToUInt(pRect->Height, &height));

    // Since we converted x, y, width, and height from ints, we can add them
    // together and remain within a UINT.
    rcDirty = CMilRectU(x, y, width, height, XYWH_Parameters);

    IFC(THIS_PTR->AddDirtyRect(&rcDirty));

Cleanup:

    RRETURN(hr);
}


HRESULT
MILSwDoubleBufferedBitmapProtectBackBuffer(
    __in CSwDoubleBufferedBitmap * THIS_PTR
    )
{
    HRESULT hr = S_OK;

    CHECKPTR(THIS_PTR);

    IFC(THIS_PTR->ProtectBackBuffer());

Cleanup:

    RRETURN(hr);
}

//------------------------------------------------------------------------------
//  IStream Wrapper for System.IO.Stream
//------------------------------------------------------------------------------

// CStreamDescriptor -----------------------------------------------------------

class CStreamDescriptor
{
public:
    void (__stdcall *pfnDispose)(
        __typefix(CStreamDescriptor) __in_ecount(1) void* pSD
        );

    HRESULT (__stdcall *pfnRead)(
        __typefix(CStreamDescriptor) __in_ecount(1) void* pSD,
        __in_bcount(cb) VOID* buf,
        ULONG cb,
        __out_ecount(1) ULONG* cbRead
        );

    HRESULT (__stdcall *pfnSeek)(
        __typefix(CStreamDescriptor) __in_ecount(1) void* pSD,
        LARGE_INTEGER offset,
        DWORD origin,
        __out_ecount(1) ULARGE_INTEGER* newPos
        );

    HRESULT (__stdcall *pfnStat)(
        __typefix(CStreamDescriptor) __in_ecount(1) void* pSD,
        __out_ecount(1) STATSTG* statstg,
        DWORD statFlag
        );

    HRESULT (__stdcall *pfnWrite)(
        __typefix(CStreamDescriptor) __in_ecount(1) void* pSD,
        __in_bcount(cb) const VOID* buf,
        ULONG cb,
        __out_ecount(1) ULONG* cbWritten
        );

    HRESULT (__stdcall *pfnCopyTo)(
        __typefix(CStreamDescriptor) __in_ecount(1) void* pSD,
        __inout_ecount(1) IStream* stream,
        ULARGE_INTEGER cb,
        __out_ecount(1) ULARGE_INTEGER* cbRead,
        __out_ecount(1) ULARGE_INTEGER* cbWritten
        );

    HRESULT (__stdcall *pfnSetSize)(
        __typefix(CStreamDescriptor) __in_ecount(1) void* pSD,
        ULARGE_INTEGER newSize
        );

    HRESULT (__stdcall *pfnCommit)(
        __typefix(CStreamDescriptor) __in_ecount(1) void* pSD,
        DWORD commitFlags
        );

    HRESULT (__stdcall *pfnRevert)(
        __typefix(CStreamDescriptor) __in_ecount(1) void* pSD
        );

    HRESULT (__stdcall *pfnLockRegion)(
        __typefix(CStreamDescriptor) __in_ecount(1) void* pSD,
        ULARGE_INTEGER offset,
        ULARGE_INTEGER cb,
        DWORD lockType
        );

    HRESULT (__stdcall *pfnUnlockRegion)(
        __typefix(CStreamDescriptor) __in_ecount(1) void* pSD,
        ULARGE_INTEGER offset,
        ULARGE_INTEGER cb,
        DWORD lockType
        );

    HRESULT (__stdcall *pfnClone)(
        __typefix(CStreamDescriptor) __in_ecount(1) void* pSD,
        __deref_out_ecount(1) IStream** const stream
        );

    HRESULT (__stdcall *pfnCanWrite)(
        __typefix(CStreamDescriptor) __in_ecount(1) void* pSD,
        __out_ecount(1) BOOL *canWrite
        );

    HRESULT (__stdcall *pfnCanSeek)(
        __typefix(CStreamDescriptor) __in_ecount(1) void* pSD,
        __out_ecount(1) BOOL *canSeek
        );

    DWORD_PTR m_handle;
};

// CManagedStreamWrapper -----------------------------------------------------------

MtDefine(CManagedStreamWrapper, MILApi, "CManagedStreamWrapper");

class CManagedStreamWrapper :
    public CMILCOMBase,
    public IManagedStream
{
public:
    DECLARE_MIL_OBJECT
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CManagedStreamWrapper));

    CManagedStreamWrapper(
        __in_ecount(1) const CStreamDescriptor& sd
        );

    virtual ~CManagedStreamWrapper();

    STDMETHOD(Read)(
        __out_bcount_part(cb, *cbRead) VOID* buf,
        ULONG cb,
        __out_ecount(1) ULONG* cbRead
        );

    STDMETHOD(Seek)(
        LARGE_INTEGER offset,
        DWORD origin,
        __out_ecount(1) ULARGE_INTEGER* newPos
        );

    STDMETHOD(Stat)(
        __out_ecount(1) STATSTG* statstg,
        DWORD statFlag
        );

    STDMETHOD(Write)(
        __in_ecount(cb) const VOID* buf,
        ULONG cb,
        __out_ecount(1) ULONG* cbWritten
        );

    STDMETHOD(CopyTo)(
        __inout_ecount(1) IStream* stream,
        ULARGE_INTEGER cb,
        __out_ecount(1) ULARGE_INTEGER* cbRead,
        __out_ecount(1) ULARGE_INTEGER* cbWritten
        );

    STDMETHOD(SetSize)(
        ULARGE_INTEGER newSize
        );

    STDMETHOD(Commit)(
        DWORD commitFlags
        );

    STDMETHOD(Revert)();

    STDMETHOD(LockRegion)(
        ULARGE_INTEGER offset,
        ULARGE_INTEGER cb,
        DWORD lockType
        );

    STDMETHOD(UnlockRegion)(
        ULARGE_INTEGER offset,
        ULARGE_INTEGER cb,
        DWORD lockType
        );

    STDMETHOD(Clone)(
        __deref_out_ecount(1) IStream** stream
        );

    STDMETHOD(CanWrite)(
        __out_ecount(1) BOOL *pfCanWrite
        );

    STDMETHOD(CanSeek)(
        __out_ecount(1) BOOL *pfCanSeek
        );

private:
    CStreamDescriptor m_sd;
};

CManagedStreamWrapper::CManagedStreamWrapper(
    __in_ecount(1) const CStreamDescriptor& sd
    ) : m_sd(sd), CMILCOMBase()
{
}

CManagedStreamWrapper::~CManagedStreamWrapper()
{
    m_sd.pfnDispose(&m_sd);
}

STDMETHODIMP
CManagedStreamWrapper::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void ** const ppvObject
    )
{
    HRESULT hr = E_INVALIDARG;

    if (ppvObject)
    {
        if (riid == IID_IStream)
        {
            (*ppvObject) = static_cast<IStream *>(this);
            hr = S_OK;
        }
        else if (riid == IID_IManagedStream)
        {
            (*ppvObject) = static_cast<IManagedStream *>(this);
            hr = S_OK;
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }

    return hr;
}

STDMETHODIMP
CManagedStreamWrapper::Read(
    __out_bcount_part(cb, *cbRead) VOID* buf,
    ULONG cb,
    __out_ecount(1) ULONG* cbRead
    )
{
    RRETURN(m_sd.pfnRead(&m_sd, buf, cb, cbRead));
}

STDMETHODIMP
CManagedStreamWrapper::Seek(
    LARGE_INTEGER offset,
    DWORD origin,
    __out_ecount(1) ULARGE_INTEGER* newPos
    )
{
    RRETURN(m_sd.pfnSeek(&m_sd, offset, origin, newPos));
}

STDMETHODIMP
CManagedStreamWrapper::Stat(
    __out_ecount(1) STATSTG* statstg,
    DWORD statFlag
    )
{
    RRETURN(m_sd.pfnStat(&m_sd, statstg, statFlag));
}

STDMETHODIMP
CManagedStreamWrapper::Write(
    __in_ecount(cb) const VOID* buf,
    ULONG cb,
    __out_ecount(1) ULONG* cbWritten
    )
{
    RRETURN(m_sd.pfnWrite(&m_sd, buf, cb, cbWritten));
}

STDMETHODIMP
CManagedStreamWrapper::CopyTo(
    __inout_ecount(1) IStream* stream,
    ULARGE_INTEGER cb,
    __out_ecount(1) ULARGE_INTEGER* cbRead,
    __out_ecount(1) ULARGE_INTEGER* cbWritten
    )
{
    RRETURN(m_sd.pfnCopyTo(&m_sd, stream,cb, cbRead, cbWritten));
}

STDMETHODIMP
CManagedStreamWrapper::SetSize(
    ULARGE_INTEGER newSize
    )
{
    RRETURN(m_sd.pfnSetSize(&m_sd, newSize));
}

STDMETHODIMP
CManagedStreamWrapper::Commit(
    DWORD commitFlags
    )
{
    RRETURN(m_sd.pfnCommit(&m_sd, commitFlags));
}

STDMETHODIMP
CManagedStreamWrapper::Revert()
{
    RRETURN(m_sd.pfnRevert(&m_sd));
}

STDMETHODIMP
CManagedStreamWrapper::LockRegion(
    ULARGE_INTEGER offset,
    ULARGE_INTEGER cb,
    DWORD lockType
    )
{
    RRETURN(m_sd.pfnLockRegion(&m_sd, offset, cb, lockType));
}

STDMETHODIMP
CManagedStreamWrapper::UnlockRegion(
    ULARGE_INTEGER offset,
    ULARGE_INTEGER cb,
    DWORD lockType
    )
{
    RRETURN(m_sd.pfnUnlockRegion(&m_sd, offset, cb, lockType));
}

STDMETHODIMP
CManagedStreamWrapper::Clone(
    __deref_out_ecount(1) IStream** const stream
    )
{
    RRETURN(m_sd.pfnClone(&m_sd, stream));
}

STDMETHODIMP
CManagedStreamWrapper::CanWrite(
    __out_ecount(1) BOOL *pfCanWrite
    )
{
    RRETURN(m_sd.pfnCanWrite(&m_sd, pfCanWrite));
}

STDMETHODIMP
CManagedStreamWrapper::CanSeek(
    __out_ecount(1) BOOL *pfCanSeek
    )
{
    RRETURN(m_sd.pfnCanSeek(&m_sd, pfCanSeek));
}

// MILCreateStreamFromStreamDescriptor -----------------------------------------------------------


HRESULT
MILCreateStreamFromStreamDescriptor(
    __inout_ecount(1) CStreamDescriptor* pSD,
    __deref_out_ecount(1) IStream** const ppStream
    )
{
    HRESULT hr = S_OK;

    CHECKPTRARG(ppStream);
    CHECKPTRARG(pSD);

    IStream* pStream = new CManagedStreamWrapper(*pSD);
    IFCOOM(pStream);

    pStream->AddRef();
    *ppStream = pStream;
Cleanup:
    RRETURN(hr);
}

// MILCreateEventProxy -----------------------------------------------------------

HRESULT
MILCreateEventProxy(
    __inout_ecount(1) CEventProxyDescriptor* pEPD,
    __deref_out_ecount(1) CEventProxy** const ppEventProxy
    )
{
    HRESULT hr = S_OK;

    CHECKPTRARG(ppEventProxy);
    CHECKPTRARG(pEPD);

    IFC(CEventProxy::Create(*pEPD, ppEventProxy));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------------
//  IStream
//------------------------------------------------------------------------------


HRESULT
MILIStreamWrite(
    __inout_ecount(1) IStream* pStream,
    __in_bcount(cb) const VOID* buf,
    ULONG cb,
    __out_ecount(1) ULONG* cbWritten
    )
{
    HRESULT hr = S_OK;
    CHECKPTRARG(pStream);
    IFC(pStream->Write(buf, cb, cbWritten));
Cleanup:
    RRETURN(hr);
}

HRESULT
MILUpdateSystemParametersInfo()
{
    g_DisplayManager.ScheduleUpdate();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      IWICColorContext_GetProfileBytes_Proxy
//
//  Synopsis:
//      Calls GetProfileBytes on the incoming IWICColorContext*
//
//----------------------------------------------------------------------------
HRESULT
IWICColorContext_GetProfileBytes_Proxy(
    __in_ecount(1) IWICColorContext *pICC,
    UINT cbBuffer,
    __inout_bcount_part_opt(cbBuffer, *pcbActual) BYTE *pbBuffer,
    __out_ecount(1) UINT *pcbActual
    )
{
    HRESULT hr = S_OK;

    CHECKPTRARG(pICC);
    CHECKPTRARG(pcbActual);

    IFC(pICC->GetProfileBytes(cbBuffer, pbBuffer, pcbActual));

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:
//      IWICColorContext_GetType_Proxy
//
//  Synopsis:
//      Calls GetType on the incoming IWICColorContext*
//
//----------------------------------------------------------------------------
HRESULT
IWICColorContext_GetType_Proxy(
    __in_ecount(1) IWICColorContext *pICC,
    __out_ecount(1) WICColorContextType *pType
    )
{
    HRESULT hr = S_OK;

    CHECKPTRARG(pICC);
    CHECKPTRARG(pType);

    IFC(pICC->GetType(pType));

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:
//      IWICColorContext_GetExifColorSpace_Proxy
//
//  Synopsis:
//      Calls GetExifColorSpace on the incoming IWICColorContext*
//
//----------------------------------------------------------------------------
HRESULT
IWICColorContext_GetExifColorSpace_Proxy(
    __in_ecount(1) IWICColorContext *pICC,
    __out_ecount(1) UINT *pValue
    )
{
    HRESULT hr = S_OK;

    CHECKPTRARG(pICC);
    CHECKPTRARG(pValue);

    IFC(pICC->GetExifColorSpace(pValue));

Cleanup:
    RRETURN(hr);
}




