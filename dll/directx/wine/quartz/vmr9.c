/*
 * Video Mixing Renderer for dx9
 *
 * Copyright 2004 Christian Costa
 * Copyright 2008 Maarten Lankhorst
 * Copyright 2012 Aric Stewart
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include "quartz_private.h"

#include "uuids.h"
#include "vfwmsgs.h"
#include "amvideo.h"
#include "windef.h"
#include "winbase.h"
#include "dshow.h"
#include "evcode.h"
#include "strmif.h"
#include "ddraw.h"
#include "dvdmedia.h"
#include "d3d9.h"
#include "vmr9.h"
#include "pin.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

typedef struct
{
    BaseRenderer renderer;
    BaseControlWindow baseControlWindow;
    BaseControlVideo baseControlVideo;

    IUnknown IUnknown_inner;
    IAMFilterMiscFlags IAMFilterMiscFlags_iface;
    IVMRFilterConfig9 IVMRFilterConfig9_iface;
    IVMRWindowlessControl9 IVMRWindowlessControl9_iface;
    IVMRSurfaceAllocatorNotify9 IVMRSurfaceAllocatorNotify9_iface;

    IVMRSurfaceAllocatorEx9 *allocator;
    IVMRImagePresenter9 *presenter;
    BOOL allocator_is_ex;

    /*
     * The Video Mixing Renderer supports 3 modes, renderless, windowless and windowed
     * What I do is implement windowless as a special case of renderless, and then
     * windowed also as a special case of windowless. This is probably the easiest way.
     */
    VMR9Mode mode;
    BITMAPINFOHEADER bmiheader;
    IUnknown * outer_unk;
    BOOL bUnkOuterValid;
    BOOL bAggregatable;

    HMODULE hD3d9;

    /* Presentation related members */
    IDirect3DDevice9 *allocator_d3d9_dev;
    HMONITOR allocator_mon;
    DWORD num_surfaces;
    DWORD cur_surface;
    DWORD_PTR cookie;

    /* for Windowless Mode */
    HWND hWndClippingWindow;

    RECT source_rect;
    RECT target_rect;
    LONG VideoWidth;
    LONG VideoHeight;
} VMR9Impl;

static inline VMR9Impl *impl_from_inner_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, VMR9Impl, IUnknown_inner);
}

static inline VMR9Impl *impl_from_BaseWindow( BaseWindow *wnd )
{
    return CONTAINING_RECORD(wnd, VMR9Impl, baseControlWindow.baseWindow);
}

static inline VMR9Impl *impl_from_IVideoWindow( IVideoWindow *iface)
{
    return CONTAINING_RECORD(iface, VMR9Impl, baseControlWindow.IVideoWindow_iface);
}

static inline VMR9Impl *impl_from_BaseControlVideo( BaseControlVideo *cvid )
{
    return CONTAINING_RECORD(cvid, VMR9Impl, baseControlVideo);
}

static inline VMR9Impl *impl_from_IBasicVideo( IBasicVideo *iface)
{
    return CONTAINING_RECORD(iface, VMR9Impl, baseControlVideo.IBasicVideo_iface);
}

static inline VMR9Impl *impl_from_IAMFilterMiscFlags( IAMFilterMiscFlags *iface)
{
    return CONTAINING_RECORD(iface, VMR9Impl, IAMFilterMiscFlags_iface);
}

static inline VMR9Impl *impl_from_IVMRFilterConfig9( IVMRFilterConfig9 *iface)
{
    return CONTAINING_RECORD(iface, VMR9Impl, IVMRFilterConfig9_iface);
}

static inline VMR9Impl *impl_from_IVMRWindowlessControl9( IVMRWindowlessControl9 *iface)
{
    return CONTAINING_RECORD(iface, VMR9Impl, IVMRWindowlessControl9_iface);
}

static inline VMR9Impl *impl_from_IVMRSurfaceAllocatorNotify9( IVMRSurfaceAllocatorNotify9 *iface)
{
    return CONTAINING_RECORD(iface, VMR9Impl, IVMRSurfaceAllocatorNotify9_iface);
}

typedef struct
{
    IVMRImagePresenter9 IVMRImagePresenter9_iface;
    IVMRSurfaceAllocatorEx9 IVMRSurfaceAllocatorEx9_iface;

    LONG refCount;

    HANDLE ack;
    DWORD tid;
    HANDLE hWndThread;

    IDirect3DDevice9 *d3d9_dev;
    IDirect3D9 *d3d9_ptr;
    IDirect3DSurface9 **d3d9_surfaces;
    IDirect3DVertexBuffer9 *d3d9_vertex;
    HMONITOR hMon;
    DWORD num_surfaces;

    BOOL reset;
    VMR9AllocationInfo info;

    VMR9Impl* pVMR9;
    IVMRSurfaceAllocatorNotify9 *SurfaceAllocatorNotify;
} VMR9DefaultAllocatorPresenterImpl;

static inline VMR9DefaultAllocatorPresenterImpl *impl_from_IVMRImagePresenter9( IVMRImagePresenter9 *iface)
{
    return CONTAINING_RECORD(iface, VMR9DefaultAllocatorPresenterImpl, IVMRImagePresenter9_iface);
}

static inline VMR9DefaultAllocatorPresenterImpl *impl_from_IVMRSurfaceAllocatorEx9( IVMRSurfaceAllocatorEx9 *iface)
{
    return CONTAINING_RECORD(iface, VMR9DefaultAllocatorPresenterImpl, IVMRSurfaceAllocatorEx9_iface);
}

static HRESULT VMR9DefaultAllocatorPresenterImpl_create(VMR9Impl *parent, LPVOID * ppv);

static DWORD VMR9_SendSampleData(VMR9Impl *This, VMR9PresentationInfo *info, LPBYTE data, DWORD size)
{
    AM_MEDIA_TYPE *amt;
    HRESULT hr = S_OK;
    int width;
    int height;
    BITMAPINFOHEADER *bmiHeader;
    D3DLOCKED_RECT lock;

    TRACE("%p %p %d\n", This, data, size);

    amt = &This->renderer.pInputPin->pin.mtCurrent;

    if (IsEqualIID(&amt->formattype, &FORMAT_VideoInfo))
    {
        bmiHeader = &((VIDEOINFOHEADER *)amt->pbFormat)->bmiHeader;
    }
    else if (IsEqualIID(&amt->formattype, &FORMAT_VideoInfo2))
    {
        bmiHeader = &((VIDEOINFOHEADER2 *)amt->pbFormat)->bmiHeader;
    }
    else
    {
        FIXME("Unknown type %s\n", debugstr_guid(&amt->subtype));
        return VFW_E_RUNTIME_ERROR;
    }

    TRACE("biSize = %d\n", bmiHeader->biSize);
    TRACE("biWidth = %d\n", bmiHeader->biWidth);
    TRACE("biHeight = %d\n", bmiHeader->biHeight);
    TRACE("biPlanes = %d\n", bmiHeader->biPlanes);
    TRACE("biBitCount = %d\n", bmiHeader->biBitCount);
    TRACE("biCompression = %s\n", debugstr_an((LPSTR)&(bmiHeader->biCompression), 4));
    TRACE("biSizeImage = %d\n", bmiHeader->biSizeImage);

    width = bmiHeader->biWidth;
    height = bmiHeader->biHeight;

    TRACE("Src Rect: %d %d %d %d\n", This->source_rect.left, This->source_rect.top, This->source_rect.right, This->source_rect.bottom);
    TRACE("Dst Rect: %d %d %d %d\n", This->target_rect.left, This->target_rect.top, This->target_rect.right, This->target_rect.bottom);

    hr = IDirect3DSurface9_LockRect(info->lpSurf, &lock, NULL, D3DLOCK_DISCARD);
    if (FAILED(hr))
    {
        ERR("IDirect3DSurface9_LockRect failed (%x)\n",hr);
        return hr;
    }

    if (lock.Pitch != width * bmiHeader->biBitCount / 8)
    {
        WARN("Slow path! %u/%u\n", lock.Pitch, width * bmiHeader->biBitCount/8);

        while (height--)
        {
            memcpy(lock.pBits, data, width * bmiHeader->biBitCount / 8);
            data = data + width * bmiHeader->biBitCount / 8;
            lock.pBits = (char *)lock.pBits + lock.Pitch;
        }
    }
    else memcpy(lock.pBits, data, size);

    IDirect3DSurface9_UnlockRect(info->lpSurf);

    hr = IVMRImagePresenter9_PresentImage(This->presenter, This->cookie, info);
    return hr;
}

static HRESULT WINAPI VMR9_DoRenderSample(BaseRenderer *iface, IMediaSample * pSample)
{
    VMR9Impl *This = (VMR9Impl *)iface;
    LPBYTE pbSrcStream = NULL;
    long cbSrcStream = 0;
    REFERENCE_TIME tStart, tStop;
    VMR9PresentationInfo info;
    HRESULT hr;

    TRACE("%p %p\n", iface, pSample);

    /* It is possible that there is no device at this point */

    if (!This->allocator || !This->presenter)
    {
        ERR("NO PRESENTER!!\n");
        return S_FALSE;
    }

    hr = IMediaSample_GetTime(pSample, &tStart, &tStop);
    if (FAILED(hr))
        info.dwFlags = VMR9Sample_SrcDstRectsValid;
    else
        info.dwFlags = VMR9Sample_SrcDstRectsValid | VMR9Sample_TimeValid;

    if (IMediaSample_IsDiscontinuity(pSample) == S_OK)
        info.dwFlags |= VMR9Sample_Discontinuity;

    if (IMediaSample_IsPreroll(pSample) == S_OK)
        info.dwFlags |= VMR9Sample_Preroll;

    if (IMediaSample_IsSyncPoint(pSample) == S_OK)
        info.dwFlags |= VMR9Sample_SyncPoint;

    /* If we render ourselves, and this is a preroll sample, discard it */
    if (This->baseControlWindow.baseWindow.hWnd && (info.dwFlags & VMR9Sample_Preroll))
    {
        return S_OK;
    }

    hr = IMediaSample_GetPointer(pSample, &pbSrcStream);
    if (FAILED(hr))
    {
        ERR("Cannot get pointer to sample data (%x)\n", hr);
        return hr;
    }

    cbSrcStream = IMediaSample_GetActualDataLength(pSample);

    info.rtStart = tStart;
    info.rtEnd = tStop;
    info.szAspectRatio.cx = This->bmiheader.biWidth;
    info.szAspectRatio.cy = This->bmiheader.biHeight;

    hr = IVMRSurfaceAllocatorEx9_GetSurface(This->allocator, This->cookie, (++This->cur_surface)%This->num_surfaces, 0, &info.lpSurf);

    if (FAILED(hr))
        return hr;

    VMR9_SendSampleData(This, &info, pbSrcStream, cbSrcStream);
    IDirect3DSurface9_Release(info.lpSurf);

    return hr;
}

static HRESULT WINAPI VMR9_CheckMediaType(BaseRenderer *iface, const AM_MEDIA_TYPE * pmt)
{
    VMR9Impl *This = (VMR9Impl*)iface;

    if (!IsEqualIID(&pmt->majortype, &MEDIATYPE_Video) || !pmt->pbFormat)
        return S_FALSE;

    /* Ignore subtype, test for bicompression instead */
    if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo))
    {
        VIDEOINFOHEADER *format = (VIDEOINFOHEADER *)pmt->pbFormat;

        This->bmiheader = format->bmiHeader;
        TRACE("Resolution: %dx%d\n", format->bmiHeader.biWidth, format->bmiHeader.biHeight);
        This->source_rect.right = This->VideoWidth = format->bmiHeader.biWidth;
        This->source_rect.bottom = This->VideoHeight = format->bmiHeader.biHeight;
        This->source_rect.top = This->source_rect.left = 0;
    }
    else if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo2))
    {
        VIDEOINFOHEADER2 *format = (VIDEOINFOHEADER2 *)pmt->pbFormat;

        This->bmiheader = format->bmiHeader;

        TRACE("Resolution: %dx%d\n", format->bmiHeader.biWidth, format->bmiHeader.biHeight);
        This->source_rect.right = This->VideoWidth = format->bmiHeader.biWidth;
        This->source_rect.bottom = This->VideoHeight = format->bmiHeader.biHeight;
        This->source_rect.top = This->source_rect.left = 0;
    }
    else
    {
        ERR("Format type %s not supported\n", debugstr_guid(&pmt->formattype));
        return S_FALSE;
    }
    if (This->bmiheader.biCompression)
        return S_FALSE;
    return S_OK;
}

static HRESULT VMR9_maybe_init(VMR9Impl *This, BOOL force)
{
    VMR9AllocationInfo info;
    DWORD buffers;
    HRESULT hr;

    TRACE("my mode: %u, my window: %p, my last window: %p\n", This->mode, This->baseControlWindow.baseWindow.hWnd, This->hWndClippingWindow);
    if (This->baseControlWindow.baseWindow.hWnd || !This->renderer.pInputPin->pin.pConnectedTo)
        return S_OK;

    if (This->mode == VMR9Mode_Windowless && !This->hWndClippingWindow)
        return (force ? VFW_E_RUNTIME_ERROR : S_OK);

    TRACE("Initializing\n");
    info.dwFlags = VMR9AllocFlag_TextureSurface;
    info.dwHeight = This->source_rect.bottom;
    info.dwWidth = This->source_rect.right;
    info.Pool = D3DPOOL_DEFAULT;
    info.MinBuffers = 2;
    FIXME("Reduce ratio to least common denominator\n");
    info.szAspectRatio.cx = info.dwWidth;
    info.szAspectRatio.cy = info.dwHeight;
    info.szNativeSize.cx = This->bmiheader.biWidth;
    info.szNativeSize.cy = This->bmiheader.biHeight;
    buffers = 2;

    switch (This->bmiheader.biBitCount)
    {
    case 8:  info.Format = D3DFMT_R3G3B2; break;
    case 15: info.Format = D3DFMT_X1R5G5B5; break;
    case 16: info.Format = D3DFMT_R5G6B5; break;
    case 24: info.Format = D3DFMT_R8G8B8; break;
    case 32: info.Format = D3DFMT_X8R8G8B8; break;
    default:
        FIXME("Unknown bpp %u\n", This->bmiheader.biBitCount);
        hr = E_INVALIDARG;
    }

    This->cur_surface = 0;
    if (This->num_surfaces)
    {
        ERR("num_surfaces or d3d9_surfaces not 0\n");
        return E_FAIL;
    }

    hr = IVMRSurfaceAllocatorEx9_InitializeDevice(This->allocator, This->cookie, &info, &buffers);
    if (SUCCEEDED(hr))
    {
        This->source_rect.left = This->source_rect.top = 0;
        This->source_rect.right = This->bmiheader.biWidth;
        This->source_rect.bottom = This->bmiheader.biHeight;

        This->num_surfaces = buffers;
    }
    return hr;
}

static VOID WINAPI VMR9_OnStartStreaming(BaseRenderer* iface)
{
    VMR9Impl *This = (VMR9Impl*)iface;

    TRACE("(%p)\n", This);

    VMR9_maybe_init(This, TRUE);
    IVMRImagePresenter9_StartPresenting(This->presenter, This->cookie);
    SetWindowPos(This->baseControlWindow.baseWindow.hWnd, NULL,
        This->source_rect.left,
        This->source_rect.top,
        This->source_rect.right - This->source_rect.left,
        This->source_rect.bottom - This->source_rect.top,
        SWP_NOZORDER|SWP_NOMOVE|SWP_DEFERERASE);
    ShowWindow(This->baseControlWindow.baseWindow.hWnd, SW_SHOW);
    GetClientRect(This->baseControlWindow.baseWindow.hWnd, &This->target_rect);
}

static VOID WINAPI VMR9_OnStopStreaming(BaseRenderer* iface)
{
    VMR9Impl *This = (VMR9Impl*)iface;

    TRACE("(%p)\n", This);

    if (This->renderer.filter.state == State_Running)
        IVMRImagePresenter9_StopPresenting(This->presenter, This->cookie);
}

static HRESULT WINAPI VMR9_ShouldDrawSampleNow(BaseRenderer *This, IMediaSample *pSample, REFERENCE_TIME *pStartTime, REFERENCE_TIME *pEndTime)
{
    /* Preroll means the sample isn't shown, this is used for key frames and things like that */
    if (IMediaSample_IsPreroll(pSample) == S_OK)
        return E_FAIL;
    return S_FALSE;
}

static HRESULT WINAPI VMR9_CompleteConnect(BaseRenderer *This, IPin *pReceivePin)
{
    VMR9Impl *pVMR9 = (VMR9Impl*)This;
    HRESULT hr = S_OK;

    TRACE("(%p)\n", This);

    if (!pVMR9->mode && SUCCEEDED(hr))
        hr = IVMRFilterConfig9_SetRenderingMode(&pVMR9->IVMRFilterConfig9_iface, VMR9Mode_Windowed);

    if (SUCCEEDED(hr))
        hr = VMR9_maybe_init(pVMR9, FALSE);

    return hr;
}

static HRESULT WINAPI VMR9_BreakConnect(BaseRenderer *This)
{
    VMR9Impl *pVMR9 = (VMR9Impl*)This;
    HRESULT hr = S_OK;

    if (!pVMR9->mode)
        return S_FALSE;
     if (This->pInputPin->pin.pConnectedTo && pVMR9->allocator && pVMR9->presenter)
    {
        if (pVMR9->renderer.filter.state != State_Stopped)
        {
            ERR("Disconnecting while not stopped! UNTESTED!!\n");
        }
        if (pVMR9->renderer.filter.state == State_Running)
            hr = IVMRImagePresenter9_StopPresenting(pVMR9->presenter, pVMR9->cookie);
        IVMRSurfaceAllocatorEx9_TerminateDevice(pVMR9->allocator, pVMR9->cookie);
        pVMR9->num_surfaces = 0;
    }
    return hr;
}

static const BaseRendererFuncTable BaseFuncTable = {
    VMR9_CheckMediaType,
    VMR9_DoRenderSample,
    /**/
    NULL,
    NULL,
    NULL,
    VMR9_OnStartStreaming,
    VMR9_OnStopStreaming,
    NULL,
    NULL,
    NULL,
    VMR9_ShouldDrawSampleNow,
    NULL,
    /**/
    VMR9_CompleteConnect,
    VMR9_BreakConnect,
    NULL,
    NULL,
    NULL,
};

static LPWSTR WINAPI VMR9_GetClassWindowStyles(BaseWindow *This, DWORD *pClassStyles, DWORD *pWindowStyles, DWORD *pWindowStylesEx)
{
    static WCHAR classnameW[] = { 'I','V','M','R','9',' ','C','l','a','s','s', 0 };

    *pClassStyles = 0;
    *pWindowStyles = WS_SIZEBOX;
    *pWindowStylesEx = 0;

    return classnameW;
}

static RECT WINAPI VMR9_GetDefaultRect(BaseWindow *This)
{
    VMR9Impl* pVMR9 = impl_from_BaseWindow(This);
    static RECT defRect;

    defRect.left = defRect.top = 0;
    defRect.right = pVMR9->VideoWidth;
    defRect.bottom = pVMR9->VideoHeight;

    return defRect;
}

static BOOL WINAPI VMR9_OnSize(BaseWindow *This, LONG Width, LONG Height)
{
    VMR9Impl* pVMR9 = impl_from_BaseWindow(This);

    TRACE("WM_SIZE %d %d\n", Width, Height);
    GetClientRect(This->hWnd, &pVMR9->target_rect);
    TRACE("WM_SIZING: DestRect=(%d,%d),(%d,%d)\n",
        pVMR9->target_rect.left,
        pVMR9->target_rect.top,
        pVMR9->target_rect.right - pVMR9->target_rect.left,
        pVMR9->target_rect.bottom - pVMR9->target_rect.top);
    return BaseWindowImpl_OnSize(This, Width, Height);
}

static const BaseWindowFuncTable renderer_BaseWindowFuncTable = {
    VMR9_GetClassWindowStyles,
    VMR9_GetDefaultRect,
    NULL,
    BaseControlWindowImpl_PossiblyEatMessage,
    VMR9_OnSize,
};

static HRESULT WINAPI VMR9_GetSourceRect(BaseControlVideo* This, RECT *pSourceRect)
{
    VMR9Impl* pVMR9 = impl_from_BaseControlVideo(This);
    CopyRect(pSourceRect,&pVMR9->source_rect);
    return S_OK;
}

static HRESULT WINAPI VMR9_GetStaticImage(BaseControlVideo* This, LONG *pBufferSize, LONG *pDIBImage)
{
    VMR9Impl* pVMR9 = impl_from_BaseControlVideo(This);
    BITMAPINFOHEADER *bmiHeader;
    LONG needed_size;
    AM_MEDIA_TYPE *amt = &pVMR9->renderer.pInputPin->pin.mtCurrent;
    char *ptr;

    FIXME("(%p/%p)->(%p, %p): partial stub\n", pVMR9, This, pBufferSize, pDIBImage);

    EnterCriticalSection(&pVMR9->renderer.filter.csFilter);

    if (!pVMR9->renderer.pMediaSample)
    {
         LeaveCriticalSection(&pVMR9->renderer.filter.csFilter);
         return (pVMR9->renderer.filter.state == State_Paused ? E_UNEXPECTED : VFW_E_NOT_PAUSED);
    }

    if (IsEqualIID(&amt->formattype, &FORMAT_VideoInfo))
    {
        bmiHeader = &((VIDEOINFOHEADER *)amt->pbFormat)->bmiHeader;
    }
    else if (IsEqualIID(&amt->formattype, &FORMAT_VideoInfo2))
    {
        bmiHeader = &((VIDEOINFOHEADER2 *)amt->pbFormat)->bmiHeader;
    }
    else
    {
        FIXME("Unknown type %s\n", debugstr_guid(&amt->subtype));
        LeaveCriticalSection(&pVMR9->renderer.filter.csFilter);
        return VFW_E_RUNTIME_ERROR;
    }

    needed_size = bmiHeader->biSize;
    needed_size += IMediaSample_GetActualDataLength(pVMR9->renderer.pMediaSample);

    if (!pDIBImage)
    {
        *pBufferSize = needed_size;
        LeaveCriticalSection(&pVMR9->renderer.filter.csFilter);
        return S_OK;
    }

    if (needed_size < *pBufferSize)
    {
        ERR("Buffer too small %u/%u\n", needed_size, *pBufferSize);
        LeaveCriticalSection(&pVMR9->renderer.filter.csFilter);
        return E_FAIL;
    }
    *pBufferSize = needed_size;

    memcpy(pDIBImage, bmiHeader, bmiHeader->biSize);
    IMediaSample_GetPointer(pVMR9->renderer.pMediaSample, (BYTE **)&ptr);
    memcpy((char *)pDIBImage + bmiHeader->biSize, ptr, IMediaSample_GetActualDataLength(pVMR9->renderer.pMediaSample));

    LeaveCriticalSection(&pVMR9->renderer.filter.csFilter);
    return S_OK;
}

static HRESULT WINAPI VMR9_GetTargetRect(BaseControlVideo* This, RECT *pTargetRect)
{
    VMR9Impl* pVMR9 = impl_from_BaseControlVideo(This);
    CopyRect(pTargetRect,&pVMR9->target_rect);
    return S_OK;
}

static VIDEOINFOHEADER* WINAPI VMR9_GetVideoFormat(BaseControlVideo* This)
{
    VMR9Impl* pVMR9 = impl_from_BaseControlVideo(This);
    AM_MEDIA_TYPE *pmt;

    TRACE("(%p/%p)\n", pVMR9, This);

    pmt = &pVMR9->renderer.pInputPin->pin.mtCurrent;
    if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo)) {
        return (VIDEOINFOHEADER*)pmt->pbFormat;
    } else if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo2)) {
        static VIDEOINFOHEADER vih;
        VIDEOINFOHEADER2 *vih2 = (VIDEOINFOHEADER2*)pmt->pbFormat;
        memcpy(&vih,vih2,sizeof(VIDEOINFOHEADER));
        memcpy(&vih.bmiHeader, &vih2->bmiHeader, sizeof(BITMAPINFOHEADER));
        return &vih;
    } else {
        ERR("Unknown format type %s\n", qzdebugstr_guid(&pmt->formattype));
        return NULL;
    }
}

static HRESULT WINAPI VMR9_IsDefaultSourceRect(BaseControlVideo* This)
{
    VMR9Impl* pVMR9 = impl_from_BaseControlVideo(This);
    FIXME("(%p/%p)->(): stub !!!\n", pVMR9, This);

    return S_OK;
}

static HRESULT WINAPI VMR9_IsDefaultTargetRect(BaseControlVideo* This)
{
    VMR9Impl* pVMR9 = impl_from_BaseControlVideo(This);
    FIXME("(%p/%p)->(): stub !!!\n", pVMR9, This);

    return S_OK;
}

static HRESULT WINAPI VMR9_SetDefaultSourceRect(BaseControlVideo* This)
{
    VMR9Impl* pVMR9 = impl_from_BaseControlVideo(This);

    pVMR9->source_rect.left = 0;
    pVMR9->source_rect.top = 0;
    pVMR9->source_rect.right = pVMR9->VideoWidth;
    pVMR9->source_rect.bottom = pVMR9->VideoHeight;

    return S_OK;
}

static HRESULT WINAPI VMR9_SetDefaultTargetRect(BaseControlVideo* This)
{
    RECT rect;
    VMR9Impl* pVMR9 = impl_from_BaseControlVideo(This);

    if (!GetClientRect(pVMR9->baseControlWindow.baseWindow.hWnd, &rect))
        return E_FAIL;

    pVMR9->target_rect.left = 0;
    pVMR9->target_rect.top = 0;
    pVMR9->target_rect.right = rect.right;
    pVMR9->target_rect.bottom = rect.bottom;

    return S_OK;
}

static HRESULT WINAPI VMR9_SetSourceRect(BaseControlVideo* This, RECT *pSourceRect)
{
    VMR9Impl* pVMR9 = impl_from_BaseControlVideo(This);
    CopyRect(&pVMR9->source_rect,pSourceRect);
    return S_OK;
}

static HRESULT WINAPI VMR9_SetTargetRect(BaseControlVideo* This, RECT *pTargetRect)
{
    VMR9Impl* pVMR9 = impl_from_BaseControlVideo(This);
    CopyRect(&pVMR9->target_rect,pTargetRect);
    return S_OK;
}

static const BaseControlVideoFuncTable renderer_BaseControlVideoFuncTable = {
    VMR9_GetSourceRect,
    VMR9_GetStaticImage,
    VMR9_GetTargetRect,
    VMR9_GetVideoFormat,
    VMR9_IsDefaultSourceRect,
    VMR9_IsDefaultTargetRect,
    VMR9_SetDefaultSourceRect,
    VMR9_SetDefaultTargetRect,
    VMR9_SetSourceRect,
    VMR9_SetTargetRect
};

static HRESULT WINAPI VMR9Inner_QueryInterface(IUnknown * iface, REFIID riid, LPVOID * ppv)
{
    VMR9Impl *This = impl_from_inner_IUnknown(iface);
    TRACE("(%p/%p)->(%s, %p)\n", This, iface, qzdebugstr_guid(riid), ppv);

    if (This->bAggregatable)
        This->bUnkOuterValid = TRUE;

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = &This->IUnknown_inner;
    else if (IsEqualIID(riid, &IID_IVideoWindow))
        *ppv = &This->baseControlWindow.IVideoWindow_iface;
    else if (IsEqualIID(riid, &IID_IBasicVideo))
        *ppv = &This->baseControlVideo.IBasicVideo_iface;
    else if (IsEqualIID(riid, &IID_IAMFilterMiscFlags))
        *ppv = &This->IAMFilterMiscFlags_iface;
    else if (IsEqualIID(riid, &IID_IVMRFilterConfig9))
        *ppv = &This->IVMRFilterConfig9_iface;
    else if (IsEqualIID(riid, &IID_IVMRWindowlessControl9) && This->mode == VMR9Mode_Windowless)
        *ppv = &This->IVMRWindowlessControl9_iface;
    else if (IsEqualIID(riid, &IID_IVMRSurfaceAllocatorNotify9) && This->mode == VMR9Mode_Renderless)
        *ppv = &This->IVMRSurfaceAllocatorNotify9_iface;
    else
    {
        HRESULT hr;
        hr = BaseRendererImpl_QueryInterface(&This->renderer.filter.IBaseFilter_iface, riid, ppv);
        if (SUCCEEDED(hr))
            return hr;
    }

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    else if (IsEqualIID(riid, &IID_IBasicVideo2))
        FIXME("No interface for IID_IBasicVideo2\n");
    else if (IsEqualIID(riid, &IID_IVMRWindowlessControl9))
        ;
    else if (IsEqualIID(riid, &IID_IVMRSurfaceAllocatorNotify9))
        ;
    else if (IsEqualIID(riid, &IID_IMediaPosition))
        FIXME("No interface for IID_IMediaPosition\n");
    else if (IsEqualIID(riid, &IID_IQualProp))
        FIXME("No interface for IID_IQualProp\n");
    else if (IsEqualIID(riid, &IID_IVMRAspectRatioControl9))
        FIXME("No interface for IID_IVMRAspectRatioControl9\n");
    else if (IsEqualIID(riid, &IID_IVMRDeinterlaceControl9))
        FIXME("No interface for IID_IVMRDeinterlaceControl9\n");
    else if (IsEqualIID(riid, &IID_IVMRMixerBitmap9))
        FIXME("No interface for IID_IVMRMixerBitmap9\n");
    else if (IsEqualIID(riid, &IID_IVMRMonitorConfig9))
        FIXME("No interface for IID_IVMRMonitorConfig9\n");
    else if (IsEqualIID(riid, &IID_IVMRMixerControl9))
        FIXME("No interface for IID_IVMRMixerControl9\n");
    else
        FIXME("No interface for %s\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI VMR9Inner_AddRef(IUnknown * iface)
{
    VMR9Impl *This = impl_from_inner_IUnknown(iface);
    ULONG refCount = BaseFilterImpl_AddRef(&This->renderer.filter.IBaseFilter_iface);

    TRACE("(%p/%p)->() AddRef from %d\n", This, iface, refCount - 1);

    return refCount;
}

static ULONG WINAPI VMR9Inner_Release(IUnknown * iface)
{
    VMR9Impl *This = impl_from_inner_IUnknown(iface);
    ULONG refCount = BaseRendererImpl_Release(&This->renderer.filter.IBaseFilter_iface);

    TRACE("(%p/%p)->() Release from %d\n", This, iface, refCount + 1);

    if (!refCount)
    {
        TRACE("Destroying\n");
        BaseControlWindow_Destroy(&This->baseControlWindow);
        CloseHandle(This->hD3d9);

        if (This->allocator)
            IVMRSurfaceAllocatorEx9_Release(This->allocator);
        if (This->presenter)
            IVMRImagePresenter9_Release(This->presenter);

        This->num_surfaces = 0;
        if (This->allocator_d3d9_dev)
        {
            IDirect3DDevice9_Release(This->allocator_d3d9_dev);
            This->allocator_d3d9_dev = NULL;
        }

        CoTaskMemFree(This);
    }
    return refCount;
}

static const IUnknownVtbl IInner_VTable =
{
    VMR9Inner_QueryInterface,
    VMR9Inner_AddRef,
    VMR9Inner_Release
};

static HRESULT WINAPI VMR9_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv)
{
    VMR9Impl *This = (VMR9Impl *)iface;

    if (This->bAggregatable)
        This->bUnkOuterValid = TRUE;

    if (This->outer_unk)
    {
        if (This->bAggregatable)
            return IUnknown_QueryInterface(This->outer_unk, riid, ppv);

        if (IsEqualIID(riid, &IID_IUnknown))
        {
            HRESULT hr;

            IUnknown_AddRef(&This->IUnknown_inner);
            hr = IUnknown_QueryInterface(&This->IUnknown_inner, riid, ppv);
            IUnknown_Release(&This->IUnknown_inner);
            This->bAggregatable = TRUE;
            return hr;
        }

        *ppv = NULL;
        return E_NOINTERFACE;
    }

    return IUnknown_QueryInterface(&This->IUnknown_inner, riid, ppv);
}

static ULONG WINAPI VMR9_AddRef(IBaseFilter * iface)
{
    VMR9Impl *This = (VMR9Impl *)iface;
    LONG ret;

    if (This->outer_unk && This->bUnkOuterValid)
        ret = IUnknown_AddRef(This->outer_unk);
    else
        ret = IUnknown_AddRef(&This->IUnknown_inner);

    TRACE("(%p)->AddRef from %d\n", iface, ret - 1);

    return ret;
}

static ULONG WINAPI VMR9_Release(IBaseFilter * iface)
{
    VMR9Impl *This = (VMR9Impl *)iface;
    LONG ret;

    if (This->outer_unk && This->bUnkOuterValid)
        ret = IUnknown_Release(This->outer_unk);
    else
        ret = IUnknown_Release(&This->IUnknown_inner);

    TRACE("(%p)->Release from %d\n", iface, ret + 1);

    if (ret)
        return ret;
    return 0;
}

static const IBaseFilterVtbl VMR9_Vtbl =
{
    VMR9_QueryInterface,
    VMR9_AddRef,
    VMR9_Release,
    BaseFilterImpl_GetClassID,
    BaseRendererImpl_Stop,
    BaseRendererImpl_Pause,
    BaseRendererImpl_Run,
    BaseRendererImpl_GetState,
    BaseRendererImpl_SetSyncSource,
    BaseFilterImpl_GetSyncSource,
    BaseFilterImpl_EnumPins,
    BaseRendererImpl_FindPin,
    BaseFilterImpl_QueryFilterInfo,
    BaseFilterImpl_JoinFilterGraph,
    BaseFilterImpl_QueryVendorInfo
};

/*** IUnknown methods ***/
static HRESULT WINAPI Videowindow_QueryInterface(IVideoWindow *iface, REFIID riid, LPVOID*ppvObj)
{
    VMR9Impl *This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return VMR9_QueryInterface(&This->renderer.filter.IBaseFilter_iface, riid, ppvObj);
}

static ULONG WINAPI Videowindow_AddRef(IVideoWindow *iface)
{
    VMR9Impl *This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return VMR9_AddRef(&This->renderer.filter.IBaseFilter_iface);
}

static ULONG WINAPI Videowindow_Release(IVideoWindow *iface)
{
    VMR9Impl *This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return VMR9_Release(&This->renderer.filter.IBaseFilter_iface);
}

static const IVideoWindowVtbl IVideoWindow_VTable =
{
    Videowindow_QueryInterface,
    Videowindow_AddRef,
    Videowindow_Release,
    BaseControlWindowImpl_GetTypeInfoCount,
    BaseControlWindowImpl_GetTypeInfo,
    BaseControlWindowImpl_GetIDsOfNames,
    BaseControlWindowImpl_Invoke,
    BaseControlWindowImpl_put_Caption,
    BaseControlWindowImpl_get_Caption,
    BaseControlWindowImpl_put_WindowStyle,
    BaseControlWindowImpl_get_WindowStyle,
    BaseControlWindowImpl_put_WindowStyleEx,
    BaseControlWindowImpl_get_WindowStyleEx,
    BaseControlWindowImpl_put_AutoShow,
    BaseControlWindowImpl_get_AutoShow,
    BaseControlWindowImpl_put_WindowState,
    BaseControlWindowImpl_get_WindowState,
    BaseControlWindowImpl_put_BackgroundPalette,
    BaseControlWindowImpl_get_BackgroundPalette,
    BaseControlWindowImpl_put_Visible,
    BaseControlWindowImpl_get_Visible,
    BaseControlWindowImpl_put_Left,
    BaseControlWindowImpl_get_Left,
    BaseControlWindowImpl_put_Width,
    BaseControlWindowImpl_get_Width,
    BaseControlWindowImpl_put_Top,
    BaseControlWindowImpl_get_Top,
    BaseControlWindowImpl_put_Height,
    BaseControlWindowImpl_get_Height,
    BaseControlWindowImpl_put_Owner,
    BaseControlWindowImpl_get_Owner,
    BaseControlWindowImpl_put_MessageDrain,
    BaseControlWindowImpl_get_MessageDrain,
    BaseControlWindowImpl_get_BorderColor,
    BaseControlWindowImpl_put_BorderColor,
    BaseControlWindowImpl_get_FullScreenMode,
    BaseControlWindowImpl_put_FullScreenMode,
    BaseControlWindowImpl_SetWindowForeground,
    BaseControlWindowImpl_NotifyOwnerMessage,
    BaseControlWindowImpl_SetWindowPosition,
    BaseControlWindowImpl_GetWindowPosition,
    BaseControlWindowImpl_GetMinIdealImageSize,
    BaseControlWindowImpl_GetMaxIdealImageSize,
    BaseControlWindowImpl_GetRestorePosition,
    BaseControlWindowImpl_HideCursor,
    BaseControlWindowImpl_IsCursorHidden
};

/*** IUnknown methods ***/
static HRESULT WINAPI Basicvideo_QueryInterface(IBasicVideo *iface, REFIID riid, LPVOID * ppvObj)
{
    VMR9Impl *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return VMR9_QueryInterface(&This->renderer.filter.IBaseFilter_iface, riid, ppvObj);
}

static ULONG WINAPI Basicvideo_AddRef(IBasicVideo *iface)
{
    VMR9Impl *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return VMR9_AddRef(&This->renderer.filter.IBaseFilter_iface);
}

static ULONG WINAPI Basicvideo_Release(IBasicVideo *iface)
{
    VMR9Impl *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return VMR9_Release(&This->renderer.filter.IBaseFilter_iface);
}

static const IBasicVideoVtbl IBasicVideo_VTable =
{
    Basicvideo_QueryInterface,
    Basicvideo_AddRef,
    Basicvideo_Release,
    BaseControlVideoImpl_GetTypeInfoCount,
    BaseControlVideoImpl_GetTypeInfo,
    BaseControlVideoImpl_GetIDsOfNames,
    BaseControlVideoImpl_Invoke,
    BaseControlVideoImpl_get_AvgTimePerFrame,
    BaseControlVideoImpl_get_BitRate,
    BaseControlVideoImpl_get_BitErrorRate,
    BaseControlVideoImpl_get_VideoWidth,
    BaseControlVideoImpl_get_VideoHeight,
    BaseControlVideoImpl_put_SourceLeft,
    BaseControlVideoImpl_get_SourceLeft,
    BaseControlVideoImpl_put_SourceWidth,
    BaseControlVideoImpl_get_SourceWidth,
    BaseControlVideoImpl_put_SourceTop,
    BaseControlVideoImpl_get_SourceTop,
    BaseControlVideoImpl_put_SourceHeight,
    BaseControlVideoImpl_get_SourceHeight,
    BaseControlVideoImpl_put_DestinationLeft,
    BaseControlVideoImpl_get_DestinationLeft,
    BaseControlVideoImpl_put_DestinationWidth,
    BaseControlVideoImpl_get_DestinationWidth,
    BaseControlVideoImpl_put_DestinationTop,
    BaseControlVideoImpl_get_DestinationTop,
    BaseControlVideoImpl_put_DestinationHeight,
    BaseControlVideoImpl_get_DestinationHeight,
    BaseControlVideoImpl_SetSourcePosition,
    BaseControlVideoImpl_GetSourcePosition,
    BaseControlVideoImpl_SetDefaultSourcePosition,
    BaseControlVideoImpl_SetDestinationPosition,
    BaseControlVideoImpl_GetDestinationPosition,
    BaseControlVideoImpl_SetDefaultDestinationPosition,
    BaseControlVideoImpl_GetVideoSize,
    BaseControlVideoImpl_GetVideoPaletteEntries,
    BaseControlVideoImpl_GetCurrentImage,
    BaseControlVideoImpl_IsUsingDefaultSource,
    BaseControlVideoImpl_IsUsingDefaultDestination
};

static HRESULT WINAPI AMFilterMiscFlags_QueryInterface(IAMFilterMiscFlags *iface, REFIID riid, void **ppv) {
    VMR9Impl *This = impl_from_IAMFilterMiscFlags(iface);
    return VMR9_QueryInterface(&This->renderer.filter.IBaseFilter_iface, riid, ppv);
}

static ULONG WINAPI AMFilterMiscFlags_AddRef(IAMFilterMiscFlags *iface) {
    VMR9Impl *This = impl_from_IAMFilterMiscFlags(iface);
    return VMR9_AddRef(&This->renderer.filter.IBaseFilter_iface);
}

static ULONG WINAPI AMFilterMiscFlags_Release(IAMFilterMiscFlags *iface) {
    VMR9Impl *This = impl_from_IAMFilterMiscFlags(iface);
    return VMR9_Release(&This->renderer.filter.IBaseFilter_iface);
}

static ULONG WINAPI AMFilterMiscFlags_GetMiscFlags(IAMFilterMiscFlags *iface) {
    return AM_FILTER_MISC_FLAGS_IS_RENDERER;
}

static const IAMFilterMiscFlagsVtbl IAMFilterMiscFlags_Vtbl = {
    AMFilterMiscFlags_QueryInterface,
    AMFilterMiscFlags_AddRef,
    AMFilterMiscFlags_Release,
    AMFilterMiscFlags_GetMiscFlags
};

static HRESULT WINAPI VMR9FilterConfig_QueryInterface(IVMRFilterConfig9 *iface, REFIID riid, LPVOID * ppv)
{
    VMR9Impl *This = impl_from_IVMRFilterConfig9(iface);
    return VMR9_QueryInterface(&This->renderer.filter.IBaseFilter_iface, riid, ppv);
}

static ULONG WINAPI VMR9FilterConfig_AddRef(IVMRFilterConfig9 *iface)
{
    VMR9Impl *This = impl_from_IVMRFilterConfig9(iface);
    return VMR9_AddRef(&This->renderer.filter.IBaseFilter_iface);
}

static ULONG WINAPI VMR9FilterConfig_Release(IVMRFilterConfig9 *iface)
{
    VMR9Impl *This = impl_from_IVMRFilterConfig9(iface);
    return VMR9_Release(&This->renderer.filter.IBaseFilter_iface);
}

static HRESULT WINAPI VMR9FilterConfig_SetImageCompositor(IVMRFilterConfig9 *iface, IVMRImageCompositor9 *compositor)
{
    VMR9Impl *This = impl_from_IVMRFilterConfig9(iface);

    FIXME("(%p/%p)->(%p) stub\n", iface, This, compositor);
    return E_NOTIMPL;
}

static HRESULT WINAPI VMR9FilterConfig_SetNumberOfStreams(IVMRFilterConfig9 *iface, DWORD max)
{
    VMR9Impl *This = impl_from_IVMRFilterConfig9(iface);

    FIXME("(%p/%p)->(%u) stub\n", iface, This, max);
    return E_NOTIMPL;
}

static HRESULT WINAPI VMR9FilterConfig_GetNumberOfStreams(IVMRFilterConfig9 *iface, DWORD *max)
{
    VMR9Impl *This = impl_from_IVMRFilterConfig9(iface);

    FIXME("(%p/%p)->(%p) stub\n", iface, This, max);
    return E_NOTIMPL;
}

static HRESULT WINAPI VMR9FilterConfig_SetRenderingPrefs(IVMRFilterConfig9 *iface, DWORD renderflags)
{
    VMR9Impl *This = impl_from_IVMRFilterConfig9(iface);

    FIXME("(%p/%p)->(%u) stub\n", iface, This, renderflags);
    return E_NOTIMPL;
}

static HRESULT WINAPI VMR9FilterConfig_GetRenderingPrefs(IVMRFilterConfig9 *iface, DWORD *renderflags)
{
    VMR9Impl *This = impl_from_IVMRFilterConfig9(iface);

    FIXME("(%p/%p)->(%p) stub\n", iface, This, renderflags);
    return E_NOTIMPL;
}

static HRESULT WINAPI VMR9FilterConfig_SetRenderingMode(IVMRFilterConfig9 *iface, DWORD mode)
{
    HRESULT hr = S_OK;
    VMR9Impl *This = impl_from_IVMRFilterConfig9(iface);

    TRACE("(%p/%p)->(%u)\n", iface, This, mode);

    EnterCriticalSection(&This->renderer.filter.csFilter);
    if (This->mode)
    {
        LeaveCriticalSection(&This->renderer.filter.csFilter);
        return VFW_E_WRONG_STATE;
    }

    if (This->allocator)
        IVMRSurfaceAllocatorEx9_Release(This->allocator);
    if (This->presenter)
        IVMRImagePresenter9_Release(This->presenter);

    This->allocator = NULL;
    This->presenter = NULL;

    switch (mode)
    {
    case VMR9Mode_Windowed:
    case VMR9Mode_Windowless:
        This->allocator_is_ex = 0;
        This->cookie = ~0;

        hr = VMR9DefaultAllocatorPresenterImpl_create(This, (LPVOID*)&This->presenter);
        if (SUCCEEDED(hr))
            hr = IVMRImagePresenter9_QueryInterface(This->presenter, &IID_IVMRSurfaceAllocatorEx9, (LPVOID*)&This->allocator);
        if (FAILED(hr))
        {
            ERR("Unable to find Presenter interface\n");
            IVMRImagePresenter9_Release(This->presenter);
            This->allocator = NULL;
            This->presenter = NULL;
        }
        else
            hr = IVMRSurfaceAllocatorEx9_AdviseNotify(This->allocator, &This->IVMRSurfaceAllocatorNotify9_iface);
        break;
    case VMR9Mode_Renderless:
        break;
    default:
        LeaveCriticalSection(&This->renderer.filter.csFilter);
        return E_INVALIDARG;
    }

    This->mode = mode;
    LeaveCriticalSection(&This->renderer.filter.csFilter);
    return hr;
}

static HRESULT WINAPI VMR9FilterConfig_GetRenderingMode(IVMRFilterConfig9 *iface, DWORD *mode)
{
    VMR9Impl *This = impl_from_IVMRFilterConfig9(iface);

    TRACE("(%p/%p)->(%p) stub\n", iface, This, mode);
    if (!mode)
        return E_POINTER;

    if (This->mode)
        *mode = This->mode;
    else
        *mode = VMR9Mode_Windowed;

    return S_OK;
}

static const IVMRFilterConfig9Vtbl VMR9_FilterConfig_Vtbl =
{
    VMR9FilterConfig_QueryInterface,
    VMR9FilterConfig_AddRef,
    VMR9FilterConfig_Release,
    VMR9FilterConfig_SetImageCompositor,
    VMR9FilterConfig_SetNumberOfStreams,
    VMR9FilterConfig_GetNumberOfStreams,
    VMR9FilterConfig_SetRenderingPrefs,
    VMR9FilterConfig_GetRenderingPrefs,
    VMR9FilterConfig_SetRenderingMode,
    VMR9FilterConfig_GetRenderingMode
};

static HRESULT WINAPI VMR9WindowlessControl_QueryInterface(IVMRWindowlessControl9 *iface, REFIID riid, LPVOID * ppv)
{
    VMR9Impl *This = impl_from_IVMRWindowlessControl9(iface);
    return VMR9_QueryInterface(&This->renderer.filter.IBaseFilter_iface, riid, ppv);
}

static ULONG WINAPI VMR9WindowlessControl_AddRef(IVMRWindowlessControl9 *iface)
{
    VMR9Impl *This = impl_from_IVMRWindowlessControl9(iface);
    return VMR9_AddRef(&This->renderer.filter.IBaseFilter_iface);
}

static ULONG WINAPI VMR9WindowlessControl_Release(IVMRWindowlessControl9 *iface)
{
    VMR9Impl *This = impl_from_IVMRWindowlessControl9(iface);
    return VMR9_Release(&This->renderer.filter.IBaseFilter_iface);
}

static HRESULT WINAPI VMR9WindowlessControl_GetNativeVideoSize(IVMRWindowlessControl9 *iface, LONG *width, LONG *height, LONG *arwidth, LONG *arheight)
{
    VMR9Impl *This = impl_from_IVMRWindowlessControl9(iface);
    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", iface, This, width, height, arwidth, arheight);

    if (!width || !height || !arwidth || !arheight)
    {
        ERR("Got no pointer\n");
        return E_POINTER;
    }

    *width = This->bmiheader.biWidth;
    *height = This->bmiheader.biHeight;
    *arwidth = This->bmiheader.biWidth;
    *arheight = This->bmiheader.biHeight;

    return S_OK;
}

static HRESULT WINAPI VMR9WindowlessControl_GetMinIdealVideoSize(IVMRWindowlessControl9 *iface, LONG *width, LONG *height)
{
    VMR9Impl *This = impl_from_IVMRWindowlessControl9(iface);

    FIXME("(%p/%p)->(...) stub\n", iface, This);
    return E_NOTIMPL;
}

static HRESULT WINAPI VMR9WindowlessControl_GetMaxIdealVideoSize(IVMRWindowlessControl9 *iface, LONG *width, LONG *height)
{
    VMR9Impl *This = impl_from_IVMRWindowlessControl9(iface);

    FIXME("(%p/%p)->(...) stub\n", iface, This);
    return E_NOTIMPL;
}

static HRESULT WINAPI VMR9WindowlessControl_SetVideoPosition(IVMRWindowlessControl9 *iface, const RECT *source, const RECT *dest)
{
    VMR9Impl *This = impl_from_IVMRWindowlessControl9(iface);

    TRACE("(%p/%p)->(%p, %p)\n", iface, This, source, dest);

    EnterCriticalSection(&This->renderer.filter.csFilter);

    if (source)
        This->source_rect = *source;
    if (dest)
    {
        This->target_rect = *dest;
        if (This->baseControlWindow.baseWindow.hWnd)
        {
            FIXME("Output rectangle: starting at %dx%d, up to point %dx%d\n", dest->left, dest->top, dest->right, dest->bottom);
            SetWindowPos(This->baseControlWindow.baseWindow.hWnd, NULL, dest->left, dest->top, dest->right - dest->left,
                         dest->bottom-dest->top, SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_NOOWNERZORDER|SWP_NOREDRAW);
        }
    }

    LeaveCriticalSection(&This->renderer.filter.csFilter);

    return S_OK;
}

static HRESULT WINAPI VMR9WindowlessControl_GetVideoPosition(IVMRWindowlessControl9 *iface, RECT *source, RECT *dest)
{
    VMR9Impl *This = impl_from_IVMRWindowlessControl9(iface);

    if (source)
        *source = This->source_rect;

    if (dest)
        *dest = This->target_rect;

    FIXME("(%p/%p)->(%p/%p) stub\n", iface, This, source, dest);
    return S_OK;
}

static HRESULT WINAPI VMR9WindowlessControl_GetAspectRatioMode(IVMRWindowlessControl9 *iface, DWORD *mode)
{
    VMR9Impl *This = impl_from_IVMRWindowlessControl9(iface);

    FIXME("(%p/%p)->(...) stub\n", iface, This);
    return E_NOTIMPL;
}

static HRESULT WINAPI VMR9WindowlessControl_SetAspectRatioMode(IVMRWindowlessControl9 *iface, DWORD mode)
{
    VMR9Impl *This = impl_from_IVMRWindowlessControl9(iface);

    FIXME("(%p/%p)->(...) stub\n", iface, This);
    return E_NOTIMPL;
}

static HRESULT WINAPI VMR9WindowlessControl_SetVideoClippingWindow(IVMRWindowlessControl9 *iface, HWND hwnd)
{
    VMR9Impl *This = impl_from_IVMRWindowlessControl9(iface);

    TRACE("(%p/%p)->(%p)\n", iface, This, hwnd);

    EnterCriticalSection(&This->renderer.filter.csFilter);
    This->hWndClippingWindow = hwnd;
    VMR9_maybe_init(This, FALSE);
    if (!hwnd)
        IVMRSurfaceAllocatorEx9_TerminateDevice(This->allocator, This->cookie);
    LeaveCriticalSection(&This->renderer.filter.csFilter);
    return S_OK;
}

static HRESULT WINAPI VMR9WindowlessControl_RepaintVideo(IVMRWindowlessControl9 *iface, HWND hwnd, HDC hdc)
{
    VMR9Impl *This = impl_from_IVMRWindowlessControl9(iface);
    HRESULT hr;

    FIXME("(%p/%p)->(...) semi-stub\n", iface, This);

    EnterCriticalSection(&This->renderer.filter.csFilter);
    if (hwnd != This->hWndClippingWindow && hwnd != This->baseControlWindow.baseWindow.hWnd)
    {
        ERR("Not handling changing windows yet!!!\n");
        LeaveCriticalSection(&This->renderer.filter.csFilter);
        return S_OK;
    }

    if (!This->allocator_d3d9_dev)
    {
        ERR("No d3d9 device!\n");
        LeaveCriticalSection(&This->renderer.filter.csFilter);
        return VFW_E_WRONG_STATE;
    }

    /* Windowless extension */
    hr = IDirect3DDevice9_Present(This->allocator_d3d9_dev, NULL, NULL, This->baseControlWindow.baseWindow.hWnd, NULL);
    LeaveCriticalSection(&This->renderer.filter.csFilter);

    return hr;
}

static HRESULT WINAPI VMR9WindowlessControl_DisplayModeChanged(IVMRWindowlessControl9 *iface)
{
    VMR9Impl *This = impl_from_IVMRWindowlessControl9(iface);

    FIXME("(%p/%p)->(...) stub\n", iface, This);
    return E_NOTIMPL;
}

static HRESULT WINAPI VMR9WindowlessControl_GetCurrentImage(IVMRWindowlessControl9 *iface, BYTE **dib)
{
    VMR9Impl *This = impl_from_IVMRWindowlessControl9(iface);

    FIXME("(%p/%p)->(...) stub\n", iface, This);
    return E_NOTIMPL;
}

static HRESULT WINAPI VMR9WindowlessControl_SetBorderColor(IVMRWindowlessControl9 *iface, COLORREF color)
{
    VMR9Impl *This = impl_from_IVMRWindowlessControl9(iface);

    FIXME("(%p/%p)->(...) stub\n", iface, This);
    return E_NOTIMPL;
}

static HRESULT WINAPI VMR9WindowlessControl_GetBorderColor(IVMRWindowlessControl9 *iface, COLORREF *color)
{
    VMR9Impl *This = impl_from_IVMRWindowlessControl9(iface);

    FIXME("(%p/%p)->(...) stub\n", iface, This);
    return E_NOTIMPL;
}

static const IVMRWindowlessControl9Vtbl VMR9_WindowlessControl_Vtbl =
{
    VMR9WindowlessControl_QueryInterface,
    VMR9WindowlessControl_AddRef,
    VMR9WindowlessControl_Release,
    VMR9WindowlessControl_GetNativeVideoSize,
    VMR9WindowlessControl_GetMinIdealVideoSize,
    VMR9WindowlessControl_GetMaxIdealVideoSize,
    VMR9WindowlessControl_SetVideoPosition,
    VMR9WindowlessControl_GetVideoPosition,
    VMR9WindowlessControl_GetAspectRatioMode,
    VMR9WindowlessControl_SetAspectRatioMode,
    VMR9WindowlessControl_SetVideoClippingWindow,
    VMR9WindowlessControl_RepaintVideo,
    VMR9WindowlessControl_DisplayModeChanged,
    VMR9WindowlessControl_GetCurrentImage,
    VMR9WindowlessControl_SetBorderColor,
    VMR9WindowlessControl_GetBorderColor
};

static HRESULT WINAPI VMR9SurfaceAllocatorNotify_QueryInterface(IVMRSurfaceAllocatorNotify9 *iface, REFIID riid, LPVOID * ppv)
{
    VMR9Impl *This = impl_from_IVMRSurfaceAllocatorNotify9(iface);
    return VMR9_QueryInterface(&This->renderer.filter.IBaseFilter_iface, riid, ppv);
}

static ULONG WINAPI VMR9SurfaceAllocatorNotify_AddRef(IVMRSurfaceAllocatorNotify9 *iface)
{
    VMR9Impl *This = impl_from_IVMRSurfaceAllocatorNotify9(iface);
    return VMR9_AddRef(&This->renderer.filter.IBaseFilter_iface);
}

static ULONG WINAPI VMR9SurfaceAllocatorNotify_Release(IVMRSurfaceAllocatorNotify9 *iface)
{
    VMR9Impl *This = impl_from_IVMRSurfaceAllocatorNotify9(iface);
    return VMR9_Release(&This->renderer.filter.IBaseFilter_iface);
}

static HRESULT WINAPI VMR9SurfaceAllocatorNotify_AdviseSurfaceAllocator(IVMRSurfaceAllocatorNotify9 *iface, DWORD_PTR id, IVMRSurfaceAllocator9 *alloc)
{
    VMR9Impl *This = impl_from_IVMRSurfaceAllocatorNotify9(iface);

    /* FIXME: This code is not tested!!! */
    FIXME("(%p/%p)->(...) stub\n", iface, This);
    This->cookie = id;

    if (This->presenter)
        return VFW_E_WRONG_STATE;

    if (FAILED(IVMRSurfaceAllocator9_QueryInterface(alloc, &IID_IVMRImagePresenter9, (void **)&This->presenter)))
        return E_NOINTERFACE;

    if (SUCCEEDED(IVMRSurfaceAllocator9_QueryInterface(alloc, &IID_IVMRSurfaceAllocatorEx9, (void **)&This->allocator)))
        This->allocator_is_ex = 1;
    else
    {
        This->allocator = (IVMRSurfaceAllocatorEx9 *)alloc;
        IVMRSurfaceAllocator9_AddRef(alloc);
        This->allocator_is_ex = 0;
    }

    return S_OK;
}

static HRESULT WINAPI VMR9SurfaceAllocatorNotify_SetD3DDevice(IVMRSurfaceAllocatorNotify9 *iface, IDirect3DDevice9 *device, HMONITOR monitor)
{
    VMR9Impl *This = impl_from_IVMRSurfaceAllocatorNotify9(iface);

    FIXME("(%p/%p)->(...) semi-stub\n", iface, This);
    if (This->allocator_d3d9_dev)
        IDirect3DDevice9_Release(This->allocator_d3d9_dev);
    This->allocator_d3d9_dev = device;
    IDirect3DDevice9_AddRef(This->allocator_d3d9_dev);
    This->allocator_mon = monitor;

    return S_OK;
}

static HRESULT WINAPI VMR9SurfaceAllocatorNotify_ChangeD3DDevice(IVMRSurfaceAllocatorNotify9 *iface, IDirect3DDevice9 *device, HMONITOR monitor)
{
    VMR9Impl *This = impl_from_IVMRSurfaceAllocatorNotify9(iface);

    FIXME("(%p/%p)->(...) semi-stub\n", iface, This);
    if (This->allocator_d3d9_dev)
        IDirect3DDevice9_Release(This->allocator_d3d9_dev);
    This->allocator_d3d9_dev = device;
    IDirect3DDevice9_AddRef(This->allocator_d3d9_dev);
    This->allocator_mon = monitor;

    return S_OK;
}

static HRESULT WINAPI VMR9SurfaceAllocatorNotify_AllocateSurfaceHelper(IVMRSurfaceAllocatorNotify9 *iface, VMR9AllocationInfo *allocinfo, DWORD *numbuffers, IDirect3DSurface9 **surface)
{
    VMR9Impl *This = impl_from_IVMRSurfaceAllocatorNotify9(iface);
    DWORD i;
    HRESULT hr = S_OK;

    FIXME("(%p/%p)->(%p, %p => %u, %p) semi-stub\n", iface, This, allocinfo, numbuffers, (numbuffers ? *numbuffers : 0), surface);

    if (!allocinfo || !numbuffers || !surface)
        return E_POINTER;

    if (!*numbuffers || *numbuffers < allocinfo->MinBuffers)
    {
        ERR("Invalid number of buffers?\n");
        return E_INVALIDARG;
    }

    if (!This->allocator_d3d9_dev)
    {
        ERR("No direct3d device when requested to allocate a surface!\n");
        return VFW_E_WRONG_STATE;
    }

    if (allocinfo->dwFlags & VMR9AllocFlag_OffscreenSurface)
    {
        ERR("Creating offscreen surface\n");
        for (i = 0; i < *numbuffers; ++i)
        {
            hr = IDirect3DDevice9_CreateOffscreenPlainSurface(This->allocator_d3d9_dev,  allocinfo->dwWidth, allocinfo->dwHeight,
                                                             allocinfo->Format, allocinfo->Pool, &surface[i], NULL);
            if (FAILED(hr))
                break;
        }
    }
    else if (allocinfo->dwFlags & VMR9AllocFlag_TextureSurface)
    {
        TRACE("Creating texture surface\n");
        for (i = 0; i < *numbuffers; ++i)
        {
            IDirect3DTexture9 *texture;

            hr = IDirect3DDevice9_CreateTexture(This->allocator_d3d9_dev, allocinfo->dwWidth, allocinfo->dwHeight, 1, 0,
                                                allocinfo->Format, allocinfo->Pool, &texture, NULL);
            if (FAILED(hr))
                break;
            IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface[i]);
            IDirect3DTexture9_Release(texture);
        }
    }
    else
    {
         FIXME("Could not allocate for type %08x\n", allocinfo->dwFlags);
         return E_NOTIMPL;
    }

    if (i >= allocinfo->MinBuffers)
    {
        hr = S_OK;
        *numbuffers = i;
    }
    else
    {
        for ( ; i > 0; --i) IDirect3DSurface9_Release(surface[i - 1]);
        *numbuffers = 0;
    }
    return hr;
}

static HRESULT WINAPI VMR9SurfaceAllocatorNotify_NotifyEvent(IVMRSurfaceAllocatorNotify9 *iface, LONG code, LONG_PTR param1, LONG_PTR param2)
{
    VMR9Impl *This = impl_from_IVMRSurfaceAllocatorNotify9(iface);

    FIXME("(%p/%p)->(...) stub\n", iface, This);
    return E_NOTIMPL;
}

static const IVMRSurfaceAllocatorNotify9Vtbl IVMRSurfaceAllocatorNotify9_Vtbl =
{
    VMR9SurfaceAllocatorNotify_QueryInterface,
    VMR9SurfaceAllocatorNotify_AddRef,
    VMR9SurfaceAllocatorNotify_Release,
    VMR9SurfaceAllocatorNotify_AdviseSurfaceAllocator,
    VMR9SurfaceAllocatorNotify_SetD3DDevice,
    VMR9SurfaceAllocatorNotify_ChangeD3DDevice,
    VMR9SurfaceAllocatorNotify_AllocateSurfaceHelper,
    VMR9SurfaceAllocatorNotify_NotifyEvent
};

HRESULT VMR9Impl_create(IUnknown * outer_unk, LPVOID * ppv)
{
    HRESULT hr;
    VMR9Impl * pVMR9;

    TRACE("(%p, %p)\n", outer_unk, ppv);

    *ppv = NULL;

    pVMR9 = CoTaskMemAlloc(sizeof(VMR9Impl));

    pVMR9->hD3d9 = LoadLibraryA("d3d9.dll");
    if (!pVMR9->hD3d9 )
    {
        WARN("Could not load d3d9.dll\n");
        CoTaskMemFree(pVMR9);
        return VFW_E_DDRAW_CAPS_NOT_SUITABLE;
    }

    pVMR9->outer_unk = outer_unk;
    pVMR9->bUnkOuterValid = FALSE;
    pVMR9->bAggregatable = FALSE;
    pVMR9->IUnknown_inner.lpVtbl = &IInner_VTable;
    pVMR9->IAMFilterMiscFlags_iface.lpVtbl = &IAMFilterMiscFlags_Vtbl;

    pVMR9->mode = 0;
    pVMR9->allocator_d3d9_dev = NULL;
    pVMR9->allocator_mon= NULL;
    pVMR9->num_surfaces = pVMR9->cur_surface = 0;
    pVMR9->allocator = NULL;
    pVMR9->presenter = NULL;
    pVMR9->hWndClippingWindow = NULL;
    pVMR9->IVMRFilterConfig9_iface.lpVtbl = &VMR9_FilterConfig_Vtbl;
    pVMR9->IVMRWindowlessControl9_iface.lpVtbl = &VMR9_WindowlessControl_Vtbl;
    pVMR9->IVMRSurfaceAllocatorNotify9_iface.lpVtbl = &IVMRSurfaceAllocatorNotify9_Vtbl;

    hr = BaseRenderer_Init(&pVMR9->renderer, &VMR9_Vtbl, outer_unk, &CLSID_VideoMixingRenderer9, (DWORD_PTR)(__FILE__ ": VMR9Impl.csFilter"), &BaseFuncTable);
    if (FAILED(hr))
        goto fail;

    hr = BaseControlWindow_Init(&pVMR9->baseControlWindow, &IVideoWindow_VTable, &pVMR9->renderer.filter, &pVMR9->renderer.filter.csFilter, &pVMR9->renderer.pInputPin->pin, &renderer_BaseWindowFuncTable);
    if (FAILED(hr))
        goto fail;

    hr = BaseControlVideo_Init(&pVMR9->baseControlVideo, &IBasicVideo_VTable, &pVMR9->renderer.filter, &pVMR9->renderer.filter.csFilter, &pVMR9->renderer.pInputPin->pin, &renderer_BaseControlVideoFuncTable);
    if (FAILED(hr))
        goto fail;

    *ppv = (LPVOID)pVMR9;
    ZeroMemory(&pVMR9->source_rect, sizeof(RECT));
    ZeroMemory(&pVMR9->target_rect, sizeof(RECT));
    TRACE("Created at %p\n", pVMR9);
    return hr;

fail:
    BaseRendererImpl_Release(&pVMR9->renderer.filter.IBaseFilter_iface);
    CloseHandle(pVMR9->hD3d9);
    CoTaskMemFree(pVMR9);
    return hr;
}



static HRESULT WINAPI VMR9_ImagePresenter_QueryInterface(IVMRImagePresenter9 *iface, REFIID riid, LPVOID * ppv)
{
    VMR9DefaultAllocatorPresenterImpl *This = impl_from_IVMRImagePresenter9(iface);
    TRACE("(%p/%p)->(%s, %p)\n", This, iface, qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = (LPVOID)&(This->IVMRImagePresenter9_iface);
    else if (IsEqualIID(riid, &IID_IVMRImagePresenter9))
        *ppv = &This->IVMRImagePresenter9_iface;
    else if (IsEqualIID(riid, &IID_IVMRSurfaceAllocatorEx9))
        *ppv = &This->IVMRSurfaceAllocatorEx9_iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI VMR9_ImagePresenter_AddRef(IVMRImagePresenter9 *iface)
{
    VMR9DefaultAllocatorPresenterImpl *This = impl_from_IVMRImagePresenter9(iface);
    ULONG refCount = InterlockedIncrement(&This->refCount);

    TRACE("(%p)->() AddRef from %d\n", iface, refCount - 1);

    return refCount;
}

static ULONG WINAPI VMR9_ImagePresenter_Release(IVMRImagePresenter9 *iface)
{
    VMR9DefaultAllocatorPresenterImpl *This = impl_from_IVMRImagePresenter9(iface);
    ULONG refCount = InterlockedDecrement(&This->refCount);

    TRACE("(%p)->() Release from %d\n", iface, refCount + 1);

    if (!refCount)
    {
        DWORD i;
        TRACE("Destroying\n");
        CloseHandle(This->ack);
        IDirect3D9_Release(This->d3d9_ptr);

        TRACE("Number of surfaces: %u\n", This->num_surfaces);
        for (i = 0; i < This->num_surfaces; ++i)
        {
            IDirect3DSurface9 *surface = This->d3d9_surfaces[i];
            TRACE("Releasing surface %p\n", surface);
            if (surface)
                IDirect3DSurface9_Release(surface);
        }

        CoTaskMemFree(This->d3d9_surfaces);
        This->d3d9_surfaces = NULL;
        This->num_surfaces = 0;
        if (This->d3d9_vertex)
        {
            IDirect3DVertexBuffer9_Release(This->d3d9_vertex);
            This->d3d9_vertex = NULL;
        }
        CoTaskMemFree(This);
        return 0;
    }
    return refCount;
}

static HRESULT WINAPI VMR9_ImagePresenter_StartPresenting(IVMRImagePresenter9 *iface, DWORD_PTR id)
{
    VMR9DefaultAllocatorPresenterImpl *This = impl_from_IVMRImagePresenter9(iface);

    TRACE("(%p/%p/%p)->(...) stub\n", iface, This,This->pVMR9);
    return S_OK;
}

static HRESULT WINAPI VMR9_ImagePresenter_StopPresenting(IVMRImagePresenter9 *iface, DWORD_PTR id)
{
    VMR9DefaultAllocatorPresenterImpl *This = impl_from_IVMRImagePresenter9(iface);

    TRACE("(%p/%p/%p)->(...) stub\n", iface, This,This->pVMR9);
    return S_OK;
}

#define USED_FVF (D3DFVF_XYZRHW | D3DFVF_TEX1)
struct VERTEX { float x, y, z, rhw, u, v; };

static HRESULT VMR9_ImagePresenter_PresentTexture(VMR9DefaultAllocatorPresenterImpl *This, IDirect3DSurface9 *surface)
{
    IDirect3DTexture9 *texture = NULL;
    HRESULT hr;

    hr = IDirect3DDevice9_SetFVF(This->d3d9_dev, USED_FVF);
    if (FAILED(hr))
    {
        FIXME("SetFVF: %08x\n", hr);
        return hr;
    }

    hr = IDirect3DDevice9_SetStreamSource(This->d3d9_dev, 0, This->d3d9_vertex, 0, sizeof(struct VERTEX));
    if (FAILED(hr))
    {
        FIXME("SetStreamSource: %08x\n", hr);
        return hr;
    }

    hr = IDirect3DSurface9_GetContainer(surface, &IID_IDirect3DTexture9, (void **) &texture);
    if (FAILED(hr))
    {
        FIXME("IDirect3DSurface9_GetContainer failed\n");
        return hr;
    }
    hr = IDirect3DDevice9_SetTexture(This->d3d9_dev, 0, (IDirect3DBaseTexture9 *)texture);
    IDirect3DTexture9_Release(texture);
    if (FAILED(hr))
    {
        FIXME("SetTexture: %08x\n", hr);
        return hr;
    }

    hr = IDirect3DDevice9_DrawPrimitive(This->d3d9_dev, D3DPT_TRIANGLESTRIP, 0, 2);
    if (FAILED(hr))
    {
        FIXME("DrawPrimitive: %08x\n", hr);
        return hr;
    }

    return S_OK;
}

static HRESULT VMR9_ImagePresenter_PresentOffscreenSurface(VMR9DefaultAllocatorPresenterImpl *This, IDirect3DSurface9 *surface)
{
    HRESULT hr;
    IDirect3DSurface9 *target = NULL;
    RECT target_rect;

    hr = IDirect3DDevice9_GetBackBuffer(This->d3d9_dev, 0, 0, D3DBACKBUFFER_TYPE_MONO, &target);
    if (FAILED(hr))
    {
        ERR("IDirect3DDevice9_GetBackBuffer -- %08x\n", hr);
        return hr;
    }

    target_rect = This->pVMR9->target_rect;
    target_rect.right -= target_rect.left;
    target_rect.bottom -= target_rect.top;
    target_rect.left = target_rect.top = 0;

    /* Flip */
    target_rect.top = target_rect.bottom;
    target_rect.bottom = 0;

    hr = IDirect3DDevice9_StretchRect(This->d3d9_dev, surface, &This->pVMR9->source_rect, target, &target_rect, D3DTEXF_LINEAR);
    if (FAILED(hr))
        ERR("IDirect3DDevice9_StretchRect -- %08x\n", hr);
    IDirect3DSurface9_Release(target);

    return hr;
}

static HRESULT WINAPI VMR9_ImagePresenter_PresentImage(IVMRImagePresenter9 *iface, DWORD_PTR id, VMR9PresentationInfo *info)
{
    VMR9DefaultAllocatorPresenterImpl *This = impl_from_IVMRImagePresenter9(iface);
    HRESULT hr;
    RECT output;
    BOOL render = FALSE;

    TRACE("(%p/%p/%p)->(...) stub\n", iface, This, This->pVMR9);
    GetWindowRect(This->pVMR9->baseControlWindow.baseWindow.hWnd, &output);
    TRACE("Output rectangle: starting at %dx%d, up to point %dx%d\n", output.left, output.top, output.right, output.bottom);

    /* This might happen if we don't have active focus (eg on a different virtual desktop) */
    if (!This->d3d9_dev)
        return S_OK;

    /* Display image here */
    hr = IDirect3DDevice9_Clear(This->d3d9_dev, 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    if (FAILED(hr))
        FIXME("hr: %08x\n", hr);
    hr = IDirect3DDevice9_BeginScene(This->d3d9_dev);
    if (SUCCEEDED(hr))
    {
        if (This->d3d9_vertex)
            hr = VMR9_ImagePresenter_PresentTexture(This, info->lpSurf);
        else
            hr = VMR9_ImagePresenter_PresentOffscreenSurface(This, info->lpSurf);
        render = SUCCEEDED(hr);
    }
    else
        FIXME("BeginScene: %08x\n", hr);
    hr = IDirect3DDevice9_EndScene(This->d3d9_dev);
    if (render && SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_Present(This->d3d9_dev, NULL, NULL, This->pVMR9->baseControlWindow.baseWindow.hWnd, NULL);
        if (FAILED(hr))
            FIXME("Presenting image: %08x\n", hr);
    }

    return S_OK;
}

static const IVMRImagePresenter9Vtbl VMR9_ImagePresenter =
{
    VMR9_ImagePresenter_QueryInterface,
    VMR9_ImagePresenter_AddRef,
    VMR9_ImagePresenter_Release,
    VMR9_ImagePresenter_StartPresenting,
    VMR9_ImagePresenter_StopPresenting,
    VMR9_ImagePresenter_PresentImage
};

static HRESULT WINAPI VMR9_SurfaceAllocator_QueryInterface(IVMRSurfaceAllocatorEx9 *iface, REFIID riid, LPVOID * ppv)
{
    VMR9DefaultAllocatorPresenterImpl *This = impl_from_IVMRSurfaceAllocatorEx9(iface);

    return VMR9_ImagePresenter_QueryInterface(&This->IVMRImagePresenter9_iface, riid, ppv);
}

static ULONG WINAPI VMR9_SurfaceAllocator_AddRef(IVMRSurfaceAllocatorEx9 *iface)
{
    VMR9DefaultAllocatorPresenterImpl *This = impl_from_IVMRSurfaceAllocatorEx9(iface);

    return VMR9_ImagePresenter_AddRef(&This->IVMRImagePresenter9_iface);
}

static ULONG WINAPI VMR9_SurfaceAllocator_Release(IVMRSurfaceAllocatorEx9 *iface)
{
    VMR9DefaultAllocatorPresenterImpl *This = impl_from_IVMRSurfaceAllocatorEx9(iface);

    return VMR9_ImagePresenter_Release(&This->IVMRImagePresenter9_iface);
}

static HRESULT VMR9_SurfaceAllocator_SetAllocationSettings(VMR9DefaultAllocatorPresenterImpl *This, VMR9AllocationInfo *allocinfo)
{
    D3DCAPS9 caps;
    UINT width, height;
    HRESULT hr;

    if (!(allocinfo->dwFlags & VMR9AllocFlag_TextureSurface))
        /* Only needed for texture surfaces */
        return S_OK;

    hr = IDirect3DDevice9_GetDeviceCaps(This->d3d9_dev, &caps);
    if (FAILED(hr))
        return hr;

    if (!(caps.TextureCaps & D3DPTEXTURECAPS_POW2) || (caps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY))
    {
        width = allocinfo->dwWidth;
        height = allocinfo->dwHeight;
    }
    else
    {
        width = height = 1;
        while (width < allocinfo->dwWidth)
            width *= 2;

        while (height < allocinfo->dwHeight)
            height *= 2;
        FIXME("NPOW2 support missing, not using proper surfaces!\n");
    }

    if (caps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY)
    {
        if (height > width)
            width = height;
        else
            height = width;
        FIXME("Square texture support required..\n");
    }

    hr = IDirect3DDevice9_CreateVertexBuffer(This->d3d9_dev, 4 * sizeof(struct VERTEX), D3DUSAGE_WRITEONLY, USED_FVF, allocinfo->Pool, &This->d3d9_vertex, NULL);
    if (FAILED(hr))
    {
        ERR("Couldn't create vertex buffer: %08x\n", hr);
        return hr;
    }

    This->reset = TRUE;
    allocinfo->dwHeight = height;
    allocinfo->dwWidth = width;

    return hr;
}

static DWORD WINAPI MessageLoop(LPVOID lpParameter)
{
    MSG msg;
    BOOL fGotMessage;
    VMR9DefaultAllocatorPresenterImpl *This = lpParameter;

    TRACE("Starting message loop\n");

    if (FAILED(BaseWindowImpl_PrepareWindow(&This->pVMR9->baseControlWindow.baseWindow)))
    {
        FIXME("Failed to prepare window\n");
        return FALSE;
    }

    SetEvent(This->ack);
    while ((fGotMessage = GetMessageW(&msg, NULL, 0, 0)) != 0 && fGotMessage != -1)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    TRACE("End of message loop\n");

    return 0;
}

static UINT d3d9_adapter_from_hwnd(IDirect3D9 *d3d9, HWND hwnd, HMONITOR *mon_out)
{
    UINT d3d9_adapter;
    HMONITOR mon;

    mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL);
    if (!mon)
        d3d9_adapter = 0;
    else
    {
        for (d3d9_adapter = 0; d3d9_adapter < IDirect3D9_GetAdapterCount(d3d9); ++d3d9_adapter)
        {
            if (mon == IDirect3D9_GetAdapterMonitor(d3d9, d3d9_adapter))
                break;
        }
        if (d3d9_adapter >= IDirect3D9_GetAdapterCount(d3d9))
            d3d9_adapter = 0;
    }
    if (mon_out)
        *mon_out = mon;
    return d3d9_adapter;
}

static BOOL CreateRenderingWindow(VMR9DefaultAllocatorPresenterImpl *This, VMR9AllocationInfo *info, DWORD *numbuffers)
{
    D3DPRESENT_PARAMETERS d3dpp;
    DWORD d3d9_adapter;
    HRESULT hr;

    TRACE("(%p)->()\n", This);

    This->hWndThread = CreateThread(NULL, 0, MessageLoop, This, 0, &This->tid);
    if (!This->hWndThread)
        return FALSE;

    WaitForSingleObject(This->ack, INFINITE);

    if (!This->pVMR9->baseControlWindow.baseWindow.hWnd) return FALSE;

    /* Obtain a monitor and d3d9 device */
    d3d9_adapter = d3d9_adapter_from_hwnd(This->d3d9_ptr, This->pVMR9->baseControlWindow.baseWindow.hWnd, &This->hMon);

    /* Now try to create the d3d9 device */
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.hDeviceWindow = This->pVMR9->baseControlWindow.baseWindow.hWnd;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferHeight = This->pVMR9->target_rect.bottom - This->pVMR9->target_rect.top;
    d3dpp.BackBufferWidth = This->pVMR9->target_rect.right - This->pVMR9->target_rect.left;

    hr = IDirect3D9_CreateDevice(This->d3d9_ptr, d3d9_adapter, D3DDEVTYPE_HAL, NULL, D3DCREATE_MIXED_VERTEXPROCESSING, &d3dpp, &This->d3d9_dev);
    if (FAILED(hr))
    {
        ERR("Could not create device: %08x\n", hr);
        BaseWindowImpl_DoneWithWindow(&This->pVMR9->baseControlWindow.baseWindow);
        return FALSE;
    }
    IVMRSurfaceAllocatorNotify9_SetD3DDevice(This->SurfaceAllocatorNotify, This->d3d9_dev, This->hMon);

    This->d3d9_surfaces = CoTaskMemAlloc(*numbuffers * sizeof(IDirect3DSurface9 *));
    ZeroMemory(This->d3d9_surfaces, *numbuffers * sizeof(IDirect3DSurface9 *));

    hr = VMR9_SurfaceAllocator_SetAllocationSettings(This, info);
    if (FAILED(hr))
        ERR("Setting allocation settings failed: %08x\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IVMRSurfaceAllocatorNotify9_AllocateSurfaceHelper(This->SurfaceAllocatorNotify, info, numbuffers, This->d3d9_surfaces);
        if (FAILED(hr))
            ERR("Allocating surfaces failed: %08x\n", hr);
    }

    if (FAILED(hr))
    {
        IVMRSurfaceAllocatorEx9_TerminateDevice(This->pVMR9->allocator, This->pVMR9->cookie);
        BaseWindowImpl_DoneWithWindow(&This->pVMR9->baseControlWindow.baseWindow);
        return FALSE;
    }

    This->num_surfaces = *numbuffers;

    return TRUE;
}

static HRESULT WINAPI VMR9_SurfaceAllocator_InitializeDevice(IVMRSurfaceAllocatorEx9 *iface, DWORD_PTR id, VMR9AllocationInfo *allocinfo, DWORD *numbuffers)
{
    VMR9DefaultAllocatorPresenterImpl *This = impl_from_IVMRSurfaceAllocatorEx9(iface);

    if (This->pVMR9->mode != VMR9Mode_Windowed && !This->pVMR9->hWndClippingWindow)
    {
        ERR("No window set\n");
        return VFW_E_WRONG_STATE;
    }

    This->info = *allocinfo;

    if (!CreateRenderingWindow(This, allocinfo, numbuffers))
    {
        ERR("Failed to create rendering window, expect no output!\n");
        return VFW_E_WRONG_STATE;
    }

    return S_OK;
}

static HRESULT WINAPI VMR9_SurfaceAllocator_TerminateDevice(IVMRSurfaceAllocatorEx9 *iface, DWORD_PTR id)
{
    VMR9DefaultAllocatorPresenterImpl *This = impl_from_IVMRSurfaceAllocatorEx9(iface);

    if (!This->pVMR9->baseControlWindow.baseWindow.hWnd)
    {
        return S_OK;
    }

    SendMessageW(This->pVMR9->baseControlWindow.baseWindow.hWnd, WM_CLOSE, 0, 0);
    PostThreadMessageW(This->tid, WM_QUIT, 0, 0);
    WaitForSingleObject(This->hWndThread, INFINITE);
    This->hWndThread = NULL;
    BaseWindowImpl_DoneWithWindow(&This->pVMR9->baseControlWindow.baseWindow);

    return S_OK;
}

/* Recreate all surfaces (If allocated as D3DPOOL_DEFAULT) and survive! */
static HRESULT VMR9_SurfaceAllocator_UpdateDeviceReset(VMR9DefaultAllocatorPresenterImpl *This)
{
    struct VERTEX t_vert[4];
    UINT width, height;
    unsigned int i;
    void *bits = NULL;
    D3DPRESENT_PARAMETERS d3dpp;
    HRESULT hr;

    if (!This->pVMR9->baseControlWindow.baseWindow.hWnd)
    {
        ERR("No window\n");
        return E_FAIL;
    }

    if (!This->d3d9_surfaces || !This->reset)
        return S_OK;

    This->reset = FALSE;
    TRACE("RESETTING\n");
    if (This->d3d9_vertex)
    {
        IDirect3DVertexBuffer9_Release(This->d3d9_vertex);
        This->d3d9_vertex = NULL;
    }

    for (i = 0; i < This->num_surfaces; ++i)
    {
        IDirect3DSurface9 *surface = This->d3d9_surfaces[i];
        TRACE("Releasing surface %p\n", surface);
        if (surface)
            IDirect3DSurface9_Release(surface);
    }
    ZeroMemory(This->d3d9_surfaces, sizeof(IDirect3DSurface9 *) * This->num_surfaces);

    /* Now try to create the d3d9 device */
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.hDeviceWindow = This->pVMR9->baseControlWindow.baseWindow.hWnd;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;

    if (This->d3d9_dev)
        IDirect3DDevice9_Release(This->d3d9_dev);
    This->d3d9_dev = NULL;
    hr = IDirect3D9_CreateDevice(This->d3d9_ptr, d3d9_adapter_from_hwnd(This->d3d9_ptr, This->pVMR9->baseControlWindow.baseWindow.hWnd, &This->hMon), D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &This->d3d9_dev);
    if (FAILED(hr))
    {
        hr = IDirect3D9_CreateDevice(This->d3d9_ptr, d3d9_adapter_from_hwnd(This->d3d9_ptr, This->pVMR9->baseControlWindow.baseWindow.hWnd, &This->hMon), D3DDEVTYPE_HAL, NULL, D3DCREATE_MIXED_VERTEXPROCESSING, &d3dpp, &This->d3d9_dev);
        if (FAILED(hr))
        {
            ERR("--> Creating device: %08x\n", hr);
            return S_OK;
        }
    }
    IVMRSurfaceAllocatorNotify9_ChangeD3DDevice(This->SurfaceAllocatorNotify, This->d3d9_dev, This->hMon);

    IVMRSurfaceAllocatorNotify9_AllocateSurfaceHelper(This->SurfaceAllocatorNotify, &This->info, &This->num_surfaces, This->d3d9_surfaces);

    This->reset = FALSE;

    if (!(This->info.dwFlags & VMR9AllocFlag_TextureSurface))
        return S_OK;

    hr = IDirect3DDevice9_CreateVertexBuffer(This->d3d9_dev, 4 * sizeof(struct VERTEX), D3DUSAGE_WRITEONLY, USED_FVF,
                                             This->info.Pool, &This->d3d9_vertex, NULL);

    width = This->info.dwWidth;
    height = This->info.dwHeight;

    for (i = 0; i < sizeof(t_vert) / sizeof(t_vert[0]); ++i)
    {
        if (i % 2)
        {
            t_vert[i].x = (float)This->pVMR9->target_rect.right - (float)This->pVMR9->target_rect.left - 0.5f;
            t_vert[i].u = (float)This->pVMR9->source_rect.right / (float)width;
        }
        else
        {
            t_vert[i].x = -0.5f;
            t_vert[i].u = (float)This->pVMR9->source_rect.left / (float)width;
        }

        if (i % 4 < 2)
        {
            t_vert[i].y = -0.5f;
            t_vert[i].v = (float)This->pVMR9->source_rect.bottom / (float)height;
        }
        else
        {
            t_vert[i].y = (float)This->pVMR9->target_rect.bottom - (float)This->pVMR9->target_rect.top - 0.5f;
            t_vert[i].v = (float)This->pVMR9->source_rect.top / (float)height;
        }
        t_vert[i].z = 0.0f;
        t_vert[i].rhw = 1.0f;
    }

    FIXME("Vertex rectangle:\n");
    FIXME("X, Y: %f, %f\n", t_vert[0].x, t_vert[0].y);
    FIXME("X, Y: %f, %f\n", t_vert[3].x, t_vert[3].y);
    FIXME("TOP, LEFT: %f, %f\n", t_vert[0].u, t_vert[0].v);
    FIXME("DOWN, BOTTOM: %f, %f\n", t_vert[3].u, t_vert[3].v);

    IDirect3DVertexBuffer9_Lock(This->d3d9_vertex, 0, sizeof(t_vert), &bits, 0);
    memcpy(bits, t_vert, sizeof(t_vert));
    IDirect3DVertexBuffer9_Unlock(This->d3d9_vertex);

    return S_OK;
}

static HRESULT WINAPI VMR9_SurfaceAllocator_GetSurface(IVMRSurfaceAllocatorEx9 *iface, DWORD_PTR id, DWORD surfaceindex, DWORD flags, IDirect3DSurface9 **surface)
{
    VMR9DefaultAllocatorPresenterImpl *This = impl_from_IVMRSurfaceAllocatorEx9(iface);

    /* Update everything first, this is needed because the surface might be destroyed in the reset */
    if (!This->d3d9_dev)
    {
        TRACE("Device has left me!\n");
        return E_FAIL;
    }

    VMR9_SurfaceAllocator_UpdateDeviceReset(This);

    if (surfaceindex >= This->num_surfaces)
    {
        ERR("surfaceindex is greater than num_surfaces\n");
        return E_FAIL;
    }
    *surface = This->d3d9_surfaces[surfaceindex];
    IDirect3DSurface9_AddRef(*surface);

    return S_OK;
}

static HRESULT WINAPI VMR9_SurfaceAllocator_AdviseNotify(IVMRSurfaceAllocatorEx9 *iface, IVMRSurfaceAllocatorNotify9 *allocnotify)
{
    VMR9DefaultAllocatorPresenterImpl *This = impl_from_IVMRSurfaceAllocatorEx9(iface);

    TRACE("(%p/%p)->(...)\n", iface, This);

    /* No AddRef taken here or the base VMR9 filter would never be destroied */
    This->SurfaceAllocatorNotify = allocnotify;
    return S_OK;
}

static const IVMRSurfaceAllocatorEx9Vtbl VMR9_SurfaceAllocator =
{
    VMR9_SurfaceAllocator_QueryInterface,
    VMR9_SurfaceAllocator_AddRef,
    VMR9_SurfaceAllocator_Release,
    VMR9_SurfaceAllocator_InitializeDevice,
    VMR9_SurfaceAllocator_TerminateDevice,
    VMR9_SurfaceAllocator_GetSurface,
    VMR9_SurfaceAllocator_AdviseNotify,
    NULL /* This isn't the SurfaceAllocatorEx type yet, working on it */
};

static IDirect3D9 *init_d3d9(HMODULE d3d9_handle)
{
    IDirect3D9 * (__stdcall * d3d9_create)(UINT SDKVersion);

    d3d9_create = (void *)GetProcAddress(d3d9_handle, "Direct3DCreate9");
    if (!d3d9_create) return NULL;

    return d3d9_create(D3D_SDK_VERSION);
}

static HRESULT VMR9DefaultAllocatorPresenterImpl_create(VMR9Impl *parent, LPVOID * ppv)
{
    HRESULT hr = S_OK;
    int i;
    VMR9DefaultAllocatorPresenterImpl* This;

    This = CoTaskMemAlloc(sizeof(VMR9DefaultAllocatorPresenterImpl));
    if (!This)
        return E_OUTOFMEMORY;

    This->d3d9_ptr = init_d3d9(parent->hD3d9);
    if (!This->d3d9_ptr)
    {
        WARN("Could not initialize d3d9.dll\n");
        CoTaskMemFree(This);
        return VFW_E_DDRAW_CAPS_NOT_SUITABLE;
    }

    i = 0;
    do
    {
        D3DDISPLAYMODE mode;

        hr = IDirect3D9_EnumAdapterModes(This->d3d9_ptr, i++, D3DFMT_X8R8G8B8, 0, &mode);
    } while (FAILED(hr));
    if (FAILED(hr))
        ERR("HR: %08x\n", hr);
    if (hr == D3DERR_NOTAVAILABLE)
    {
        ERR("Format not supported\n");
        IDirect3D9_Release(This->d3d9_ptr);
        CoTaskMemFree(This);
        return VFW_E_DDRAW_CAPS_NOT_SUITABLE;
    }

    This->IVMRImagePresenter9_iface.lpVtbl = &VMR9_ImagePresenter;
    This->IVMRSurfaceAllocatorEx9_iface.lpVtbl = &VMR9_SurfaceAllocator;

    This->refCount = 1;
    This->pVMR9 = parent;
    This->d3d9_surfaces = NULL;
    This->d3d9_dev = NULL;
    This->hMon = 0;
    This->d3d9_vertex = NULL;
    This->num_surfaces = 0;
    This->hWndThread = NULL;
    This->ack = CreateEventW(NULL, 0, 0, NULL);
    This->SurfaceAllocatorNotify = NULL;
    This->reset = FALSE;

    *ppv = This;
    return S_OK;
}
