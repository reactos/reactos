/*
 * Video Renderer (Fullscreen and Windowed using Direct Draw)
 *
 * Copyright 2004 Christian Costa
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
#include "control_private.h"
#include "pin.h"

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

#include <assert.h>
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static BOOL wnd_class_registered = FALSE;

static const WCHAR wcsInputPinName[] = {'i','n','p','u','t',' ','p','i','n',0};

static const IBaseFilterVtbl VideoRenderer_Vtbl;
static const IUnknownVtbl IInner_VTable;
static const IBasicVideoVtbl IBasicVideo_VTable;
static const IVideoWindowVtbl IVideoWindow_VTable;
static const IPinVtbl VideoRenderer_InputPin_Vtbl;

typedef struct VideoRendererImpl
{
    const IBaseFilterVtbl * lpVtbl;
    const IBasicVideoVtbl * IBasicVideo_vtbl;
    const IVideoWindowVtbl * IVideoWindow_vtbl;
    const IUnknownVtbl * IInner_vtbl;

    LONG refCount;
    CRITICAL_SECTION csFilter;
    FILTER_STATE state;
    REFERENCE_TIME rtStreamStart;
    IReferenceClock * pClock;
    FILTER_INFO filterInfo;

    InputPin *pInputPin;

    BOOL init;
    HANDLE hThread;
    HANDLE blocked;

    DWORD ThreadID;
    HANDLE hEvent;
    BOOL ThreadResult;
    HWND hWnd;
    HWND hWndMsgDrain;
    BOOL AutoShow;
    RECT SourceRect;
    RECT DestRect;
    RECT WindowPos;
    long VideoWidth;
    long VideoHeight;
    IUnknown * pUnkOuter;
    BOOL bUnkOuterValid;
    BOOL bAggregatable;
    REFERENCE_TIME rtLastStop;
    MediaSeekingImpl mediaSeeking;

    /* During pause we can hold a single sample, for use in GetCurrentImage */
    IMediaSample *sample_held;
} VideoRendererImpl;

static LRESULT CALLBACK VideoWndProcA(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    VideoRendererImpl* pVideoRenderer = (VideoRendererImpl*)GetWindowLongA(hwnd, 0);
    LPRECT lprect = (LPRECT)lParam;

    if (pVideoRenderer && pVideoRenderer->hWndMsgDrain)
    {
        switch(uMsg)
        {
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_LBUTTONDBLCLK:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_MBUTTONDBLCLK:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MOUSEACTIVATE:
            case WM_MOUSEMOVE:
            case WM_NCLBUTTONDBLCLK:
            case WM_NCLBUTTONDOWN:
            case WM_NCLBUTTONUP:
            case WM_NCMBUTTONDBLCLK:
            case WM_NCMBUTTONDOWN:
            case WM_NCMBUTTONUP:
            case WM_NCMOUSEMOVE:
            case WM_NCRBUTTONDBLCLK:
            case WM_NCRBUTTONDOWN:
            case WM_NCRBUTTONUP:
            case WM_RBUTTONDBLCLK:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
                PostMessageA(pVideoRenderer->hWndMsgDrain, uMsg, wParam, lParam);
                break;
            default:
                break;
        }
    }

    switch(uMsg)
    {
        case WM_SIZING:
            /* TRACE("WM_SIZING %d %d %d %d\n", lprect->left, lprect->top, lprect->right, lprect->bottom); */
            SetWindowPos(hwnd, NULL, lprect->left, lprect->top, lprect->right - lprect->left, lprect->bottom - lprect->top, SWP_NOZORDER);
            GetClientRect(hwnd, &pVideoRenderer->DestRect);
            TRACE("WM_SIZING: DestRect=(%d,%d),(%d,%d)\n",
                pVideoRenderer->DestRect.left,
                pVideoRenderer->DestRect.top,
                pVideoRenderer->DestRect.right - pVideoRenderer->DestRect.left,
                pVideoRenderer->DestRect.bottom - pVideoRenderer->DestRect.top);
            return TRUE;
        case WM_SIZE:
            TRACE("WM_SIZE %d %d\n", LOWORD(lParam), HIWORD(lParam));
            GetClientRect(hwnd, &pVideoRenderer->DestRect);
            TRACE("WM_SIZING: DestRect=(%d,%d),(%d,%d)\n",
                pVideoRenderer->DestRect.left,
                pVideoRenderer->DestRect.top,
                pVideoRenderer->DestRect.right - pVideoRenderer->DestRect.left,
                pVideoRenderer->DestRect.bottom - pVideoRenderer->DestRect.top);
            return TRUE;
        default:
            return DefWindowProcA(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

static BOOL CreateRenderingWindow(VideoRendererImpl* This)
{
    WNDCLASSA winclass;

    TRACE("(%p)->()\n", This);
    
    winclass.style = 0;
    winclass.lpfnWndProc = VideoWndProcA;
    winclass.cbClsExtra = 0;
    winclass.cbWndExtra = sizeof(VideoRendererImpl*);
    winclass.hInstance = NULL;
    winclass.hIcon = NULL;
    winclass.hCursor = NULL;
    winclass.hbrBackground = GetStockObject(BLACK_BRUSH);
    winclass.lpszMenuName = NULL;
    winclass.lpszClassName = "Wine ActiveMovie Class";

    if (!wnd_class_registered)
    {
        if (!RegisterClassA(&winclass))
        {
            ERR("Unable to register window %u\n", GetLastError());
            return FALSE;
        }
        wnd_class_registered = TRUE;
    }

    This->hWnd = CreateWindowExA(0, "Wine ActiveMovie Class", "Wine ActiveMovie Window", WS_SIZEBOX,
                                 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL,
                                 NULL, NULL, NULL);

    if (!This->hWnd)
    {
        ERR("Unable to create window\n");
        return FALSE;
    }

    SetWindowLongA(This->hWnd, 0, (LONG)This);

    return TRUE;
}

static DWORD WINAPI MessageLoop(LPVOID lpParameter)
{
    VideoRendererImpl* This = (VideoRendererImpl*) lpParameter;
    MSG msg; 
    BOOL fGotMessage;

    TRACE("Starting message loop\n");

    if (!CreateRenderingWindow(This))
    {
        This->ThreadResult = FALSE;
        SetEvent(This->hEvent);
        return 0;
    }

    This->ThreadResult = TRUE;
    SetEvent(This->hEvent);

    while ((fGotMessage = GetMessageA(&msg, NULL, 0, 0)) != 0 && fGotMessage != -1) 
    {
        TranslateMessage(&msg); 
        DispatchMessageA(&msg); 
    }

    TRACE("End of message loop\n");

    return msg.wParam;
}

static BOOL CreateRenderingSubsystem(VideoRendererImpl* This)
{
    This->hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!This->hEvent)
        return FALSE;

    This->hThread = CreateThread(NULL, 0, MessageLoop, (LPVOID)This, 0, &This->ThreadID);
    if (!This->hThread)
    {
        CloseHandle(This->hEvent);
        return FALSE;
    }

    WaitForSingleObject(This->hEvent, INFINITE);

    if (!This->ThreadResult)
    {
        CloseHandle(This->hEvent);
        CloseHandle(This->hThread);
        return FALSE;
    }

    return TRUE;
}

static const IMemInputPinVtbl MemInputPin_Vtbl = 
{
    MemInputPin_QueryInterface,
    MemInputPin_AddRef,
    MemInputPin_Release,
    MemInputPin_GetAllocator,
    MemInputPin_NotifyAllocator,
    MemInputPin_GetAllocatorRequirements,
    MemInputPin_Receive,
    MemInputPin_ReceiveMultiple,
    MemInputPin_ReceiveCanBlock
};

static DWORD VideoRenderer_SendSampleData(VideoRendererImpl* This, LPBYTE data, DWORD size)
{
    AM_MEDIA_TYPE amt;
    HRESULT hr = S_OK;
    DDSURFACEDESC sdesc;
    int width;
    int height;
    LPBYTE palette = NULL;
    HDC hDC;
    BITMAPINFOHEADER *bmiHeader;

    TRACE("%p %p %d\n", This, data, size);

    sdesc.dwSize = sizeof(sdesc);
    hr = IPin_ConnectionMediaType((IPin *)This->pInputPin, &amt);
    if (FAILED(hr)) {
        ERR("Unable to retrieve media type\n");
        return hr;
    }

    if (IsEqualIID(&amt.formattype, &FORMAT_VideoInfo))
    {
        bmiHeader = &((VIDEOINFOHEADER *)amt.pbFormat)->bmiHeader;
    }
    else if (IsEqualIID(&amt.formattype, &FORMAT_VideoInfo2))
    {
        bmiHeader = &((VIDEOINFOHEADER2 *)amt.pbFormat)->bmiHeader;
    }
    else
    {
        FIXME("Unknown type %s\n", debugstr_guid(&amt.subtype));
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
    palette = ((LPBYTE)bmiHeader) + bmiHeader->biSize;

    if (!This->init)
    {
        if (!This->WindowPos.right || !This->WindowPos.bottom)
            This->WindowPos = This->SourceRect;

        TRACE("WindowPos: %d %d %d %d\n", This->WindowPos.left, This->WindowPos.top, This->WindowPos.right, This->WindowPos.bottom);
        SetWindowPos(This->hWnd, NULL,
            This->WindowPos.left,
            This->WindowPos.top,
            This->WindowPos.right - This->WindowPos.left,
            This->WindowPos.bottom - This->WindowPos.top,
            SWP_NOZORDER|SWP_NOMOVE);

        GetClientRect(This->hWnd, &This->DestRect);
        This->init = TRUE;
    }

    hDC = GetDC(This->hWnd);

    if (!hDC) {
        ERR("Cannot get DC from window!\n");
        return E_FAIL;
    }

    TRACE("Src Rect: %d %d %d %d\n", This->SourceRect.left, This->SourceRect.top, This->SourceRect.right, This->SourceRect.bottom);
    TRACE("Dst Rect: %d %d %d %d\n", This->DestRect.left, This->DestRect.top, This->DestRect.right, This->DestRect.bottom);

    StretchDIBits(hDC, This->DestRect.left, This->DestRect.top, This->DestRect.right -This->DestRect.left,
                  This->DestRect.bottom - This->DestRect.top, This->SourceRect.left, This->SourceRect.top,
                  This->SourceRect.right - This->SourceRect.left, This->SourceRect.bottom - This->SourceRect.top,
                  data, (BITMAPINFO *)bmiHeader, DIB_RGB_COLORS, SRCCOPY);

    ReleaseDC(This->hWnd, hDC);
    if (This->AutoShow)
        ShowWindow(This->hWnd, SW_SHOW);

    return S_OK;
}

static HRESULT VideoRenderer_Sample(LPVOID iface, IMediaSample * pSample)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;
    LPBYTE pbSrcStream = NULL;
    long cbSrcStream = 0;
    REFERENCE_TIME tStart, tStop;
    HRESULT hr;
    EnterCriticalSection(&This->csFilter);
    if (This->pInputPin->flushing || This->pInputPin->end_of_stream)
        hr = S_FALSE;

    if (This->state == State_Stopped)
    {
        LeaveCriticalSection(&This->csFilter);
        return VFW_E_WRONG_STATE;
    }

    TRACE("%p %p\n", iface, pSample);

    hr = IMediaSample_GetTime(pSample, &tStart, &tStop);
    if (FAILED(hr))
        ERR("Cannot get sample time (%x)\n", hr);

    if (This->rtLastStop != tStart)
    {
        if (IMediaSample_IsDiscontinuity(pSample) == S_FALSE)
            ERR("Unexpected discontinuity: Last: %u.%03u, tStart: %u.%03u\n",
                (DWORD)(This->rtLastStop / 10000000),
                (DWORD)((This->rtLastStop / 10000)%1000),
                (DWORD)(tStart / 10000000), (DWORD)((tStart / 10000)%1000));
        This->rtLastStop = tStart;
    }

    /* Preroll means the sample isn't shown, this is used for key frames and things like that */
    if (IMediaSample_IsPreroll(pSample) == S_OK)
    {
        This->rtLastStop = tStop;
        LeaveCriticalSection(&This->csFilter);
        return S_OK;
    }

    hr = IMediaSample_GetPointer(pSample, &pbSrcStream);
    if (FAILED(hr))
    {
        ERR("Cannot get pointer to sample data (%x)\n", hr);
        LeaveCriticalSection(&This->csFilter);
        return hr;
    }

    cbSrcStream = IMediaSample_GetActualDataLength(pSample);

    TRACE("val %p %ld\n", pbSrcStream, cbSrcStream);

#if 0 /* For debugging purpose */
    {
        int i;
        for(i = 0; i < cbSrcStream; i++)
        {
            if ((i!=0) && !(i%16))
                TRACE("\n");
                TRACE("%02x ", pbSrcStream[i]);
        }
        TRACE("\n");
    }
#endif

    SetEvent(This->hEvent);
    if (This->state == State_Paused)
    {
        This->sample_held = pSample;
        LeaveCriticalSection(&This->csFilter);
        WaitForSingleObject(This->blocked, INFINITE);
        EnterCriticalSection(&This->csFilter);
        This->sample_held = NULL;
        if (This->state == State_Paused)
        {
            /* Flushing */
            LeaveCriticalSection(&This->csFilter);
            return S_OK;
        }
        if (This->state == State_Stopped)
        {
            LeaveCriticalSection(&This->csFilter);
            return VFW_E_WRONG_STATE;
        }
    }

    if (This->pClock && This->state == State_Running)
    {
        REFERENCE_TIME time, trefstart, trefstop;
        LONG delta;

        /* Perhaps I <SHOULD> use the reference clock AdviseTime function here
         * I'm not going to! When I tried, it seemed to generate lag and
         * it caused instability.
         */
        IReferenceClock_GetTime(This->pClock, &time);

        trefstart = This->rtStreamStart;
        trefstop = (REFERENCE_TIME)((double)(tStop - tStart) / This->pInputPin->dRate) + This->rtStreamStart;
        delta = (LONG)((trefstart-time)/10000);
        This->rtStreamStart = trefstop;
        This->rtLastStop = tStop;

        if (delta > 0)
        {
            TRACE("Sleeping for %u ms\n", delta);
            Sleep(delta);
        }
        else if (time > trefstop)
        {
            TRACE("Dropping sample: Time: %u.%03u ms trefstop: %u.%03u ms!\n",
                  (DWORD)(time / 10000000), (DWORD)((time / 10000)%1000),
                  (DWORD)(trefstop / 10000000), (DWORD)((trefstop / 10000)%1000) );
            This->rtLastStop = tStop;
            LeaveCriticalSection(&This->csFilter);
            return S_OK;
        }
    }
    This->rtLastStop = tStop;

    VideoRenderer_SendSampleData(This, pbSrcStream, cbSrcStream);

    LeaveCriticalSection(&This->csFilter);
    return S_OK;
}

static HRESULT VideoRenderer_QueryAccept(LPVOID iface, const AM_MEDIA_TYPE * pmt)
{
    if (!IsEqualIID(&pmt->majortype, &MEDIATYPE_Video))
        return S_FALSE;

    if (IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_RGB32) ||
        IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_RGB24) ||
        IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_RGB565) ||
        IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_RGB8))
    {
        VideoRendererImpl* This = (VideoRendererImpl*) iface;

        if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo))
        {
            VIDEOINFOHEADER *format = (VIDEOINFOHEADER *)pmt->pbFormat;
            This->SourceRect.left = 0;
            This->SourceRect.top = 0;
            This->SourceRect.right = This->VideoWidth = format->bmiHeader.biWidth;
            This->SourceRect.bottom = This->VideoHeight = format->bmiHeader.biHeight;
        }
        else if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo2))
        {
            VIDEOINFOHEADER2 *format2 = (VIDEOINFOHEADER2 *)pmt->pbFormat;

            This->SourceRect.left = 0;
            This->SourceRect.top = 0;
            This->SourceRect.right = This->VideoWidth = format2->bmiHeader.biWidth;
            This->SourceRect.bottom = This->VideoHeight = format2->bmiHeader.biHeight;
        }
        else
        {
            WARN("Format type %s not supported\n", debugstr_guid(&pmt->formattype));
            return S_FALSE;
        }
        return S_OK;
    }
    return S_FALSE;
}

static inline VideoRendererImpl *impl_from_IMediaSeeking( IMediaSeeking *iface )
{
    return (VideoRendererImpl *)((char*)iface - FIELD_OFFSET(VideoRendererImpl, mediaSeeking.lpVtbl));
}

static HRESULT WINAPI VideoRendererImpl_Seeking_QueryInterface(IMediaSeeking * iface, REFIID riid, LPVOID * ppv)
{
    VideoRendererImpl *This = impl_from_IMediaSeeking(iface);

    return IUnknown_QueryInterface((IUnknown *)This, riid, ppv);
}

static ULONG WINAPI VideoRendererImpl_Seeking_AddRef(IMediaSeeking * iface)
{
    VideoRendererImpl *This = impl_from_IMediaSeeking(iface);

    return IUnknown_AddRef((IUnknown *)This);
}

static ULONG WINAPI VideoRendererImpl_Seeking_Release(IMediaSeeking * iface)
{
    VideoRendererImpl *This = impl_from_IMediaSeeking(iface);

    return IUnknown_Release((IUnknown *)This);
}

static const IMediaSeekingVtbl VideoRendererImpl_Seeking_Vtbl =
{
    VideoRendererImpl_Seeking_QueryInterface,
    VideoRendererImpl_Seeking_AddRef,
    VideoRendererImpl_Seeking_Release,
    MediaSeekingImpl_GetCapabilities,
    MediaSeekingImpl_CheckCapabilities,
    MediaSeekingImpl_IsFormatSupported,
    MediaSeekingImpl_QueryPreferredFormat,
    MediaSeekingImpl_GetTimeFormat,
    MediaSeekingImpl_IsUsingTimeFormat,
    MediaSeekingImpl_SetTimeFormat,
    MediaSeekingImpl_GetDuration,
    MediaSeekingImpl_GetStopPosition,
    MediaSeekingImpl_GetCurrentPosition,
    MediaSeekingImpl_ConvertTimeFormat,
    MediaSeekingImpl_SetPositions,
    MediaSeekingImpl_GetPositions,
    MediaSeekingImpl_GetAvailable,
    MediaSeekingImpl_SetRate,
    MediaSeekingImpl_GetRate,
    MediaSeekingImpl_GetPreroll
};

static HRESULT VideoRendererImpl_Change(IBaseFilter *iface)
{
    TRACE("(%p)\n", iface);
    return S_OK;
}

HRESULT VideoRenderer_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    HRESULT hr;
    PIN_INFO piInput;
    VideoRendererImpl * pVideoRenderer;

    TRACE("(%p, %p)\n", pUnkOuter, ppv);

    *ppv = NULL;

    pVideoRenderer = CoTaskMemAlloc(sizeof(VideoRendererImpl));
    pVideoRenderer->pUnkOuter = pUnkOuter;
    pVideoRenderer->bUnkOuterValid = FALSE;
    pVideoRenderer->bAggregatable = FALSE;
    pVideoRenderer->IInner_vtbl = &IInner_VTable;

    pVideoRenderer->lpVtbl = &VideoRenderer_Vtbl;
    pVideoRenderer->IBasicVideo_vtbl = &IBasicVideo_VTable;
    pVideoRenderer->IVideoWindow_vtbl = &IVideoWindow_VTable;
    
    pVideoRenderer->refCount = 1;
    InitializeCriticalSection(&pVideoRenderer->csFilter);
    pVideoRenderer->csFilter.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": VideoRendererImpl.csFilter");
    pVideoRenderer->state = State_Stopped;
    pVideoRenderer->pClock = NULL;
    pVideoRenderer->init = 0;
    pVideoRenderer->AutoShow = 1;
    pVideoRenderer->rtLastStop = -1;
    ZeroMemory(&pVideoRenderer->filterInfo, sizeof(FILTER_INFO));
    ZeroMemory(&pVideoRenderer->SourceRect, sizeof(RECT));
    ZeroMemory(&pVideoRenderer->DestRect, sizeof(RECT));
    ZeroMemory(&pVideoRenderer->WindowPos, sizeof(RECT));
    pVideoRenderer->hWndMsgDrain = NULL;

    /* construct input pin */
    piInput.dir = PINDIR_INPUT;
    piInput.pFilter = (IBaseFilter *)pVideoRenderer;
    lstrcpynW(piInput.achName, wcsInputPinName, sizeof(piInput.achName) / sizeof(piInput.achName[0]));

    hr = InputPin_Construct(&VideoRenderer_InputPin_Vtbl, &piInput, VideoRenderer_Sample, (LPVOID)pVideoRenderer, VideoRenderer_QueryAccept, NULL, &pVideoRenderer->csFilter, NULL, (IPin **)&pVideoRenderer->pInputPin);

    if (SUCCEEDED(hr))
    {
        MediaSeekingImpl_Init((IBaseFilter*)pVideoRenderer, VideoRendererImpl_Change, VideoRendererImpl_Change, VideoRendererImpl_Change, &pVideoRenderer->mediaSeeking, &pVideoRenderer->csFilter);
        pVideoRenderer->mediaSeeking.lpVtbl = &VideoRendererImpl_Seeking_Vtbl;

        pVideoRenderer->sample_held = NULL;
        *ppv = (LPVOID)pVideoRenderer;
    }
    else
    {
        pVideoRenderer->csFilter.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&pVideoRenderer->csFilter);
        CoTaskMemFree(pVideoRenderer);
    }

    if (!CreateRenderingSubsystem(pVideoRenderer))
        return E_FAIL;

    pVideoRenderer->blocked = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!pVideoRenderer->blocked)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        IUnknown_Release((IUnknown *)pVideoRenderer);
    }

    return hr;
}

HRESULT VideoRendererDefault_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    /* TODO: Attempt to use the VMR-7 renderer instead when possible */
    return VideoRenderer_create(pUnkOuter, ppv);
}

static HRESULT WINAPI VideoRendererInner_QueryInterface(IUnknown * iface, REFIID riid, LPVOID * ppv)
{
    ICOM_THIS_MULTI(VideoRendererImpl, IInner_vtbl, iface);
    TRACE("(%p/%p)->(%s, %p)\n", This, iface, qzdebugstr_guid(riid), ppv);

    if (This->bAggregatable)
        This->bUnkOuterValid = TRUE;

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = (LPVOID)&(This->IInner_vtbl);
    else if (IsEqualIID(riid, &IID_IPersist))
        *ppv = (LPVOID)This;
    else if (IsEqualIID(riid, &IID_IMediaFilter))
        *ppv = (LPVOID)This;
    else if (IsEqualIID(riid, &IID_IBaseFilter))
        *ppv = (LPVOID)This;
    else if (IsEqualIID(riid, &IID_IBasicVideo))
        *ppv = (LPVOID)&(This->IBasicVideo_vtbl);
    else if (IsEqualIID(riid, &IID_IVideoWindow))
        *ppv = (LPVOID)&(This->IVideoWindow_vtbl);
    else if (IsEqualIID(riid, &IID_IMediaSeeking))
        *ppv = &This->mediaSeeking;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    if (!IsEqualIID(riid, &IID_IPin))
        FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI VideoRendererInner_AddRef(IUnknown * iface)
{
    ICOM_THIS_MULTI(VideoRendererImpl, IInner_vtbl, iface);
    ULONG refCount = InterlockedIncrement(&This->refCount);

    TRACE("(%p/%p)->() AddRef from %d\n", This, iface, refCount - 1);

    return refCount;
}

static ULONG WINAPI VideoRendererInner_Release(IUnknown * iface)
{
    ICOM_THIS_MULTI(VideoRendererImpl, IInner_vtbl, iface);
    ULONG refCount = InterlockedDecrement(&This->refCount);

    TRACE("(%p/%p)->() Release from %d\n", This, iface, refCount + 1);

    if (!refCount)
    {
        IPin *pConnectedTo;

        DestroyWindow(This->hWnd);
        PostThreadMessageA(This->ThreadID, WM_QUIT, 0, 0);
        WaitForSingleObject(This->hThread, INFINITE);
        CloseHandle(This->hThread);
        CloseHandle(This->hEvent);

        if (This->pClock)
            IReferenceClock_Release(This->pClock);
        
        if (SUCCEEDED(IPin_ConnectedTo((IPin *)This->pInputPin, &pConnectedTo)))
        {
            IPin_Disconnect(pConnectedTo);
            IPin_Release(pConnectedTo);
        }
        IPin_Disconnect((IPin *)This->pInputPin);

        IPin_Release((IPin *)This->pInputPin);

        This->lpVtbl = NULL;
        
        This->csFilter.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->csFilter);

        TRACE("Destroying Video Renderer\n");
        CoTaskMemFree(This);
        
        return 0;
    }
    else
        return refCount;
}

static const IUnknownVtbl IInner_VTable =
{
    VideoRendererInner_QueryInterface,
    VideoRendererInner_AddRef,
    VideoRendererInner_Release
};

static HRESULT WINAPI VideoRenderer_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;

    if (This->bAggregatable)
        This->bUnkOuterValid = TRUE;

    if (This->pUnkOuter)
    {
        if (This->bAggregatable)
            return IUnknown_QueryInterface(This->pUnkOuter, riid, ppv);

        if (IsEqualIID(riid, &IID_IUnknown))
        {
            HRESULT hr;

            IUnknown_AddRef((IUnknown *)&(This->IInner_vtbl));
            hr = IUnknown_QueryInterface((IUnknown *)&(This->IInner_vtbl), riid, ppv);
            IUnknown_Release((IUnknown *)&(This->IInner_vtbl));
            This->bAggregatable = TRUE;
            return hr;
        }

        *ppv = NULL;
        return E_NOINTERFACE;
    }

    return IUnknown_QueryInterface((IUnknown *)&(This->IInner_vtbl), riid, ppv);
}

static ULONG WINAPI VideoRenderer_AddRef(IBaseFilter * iface)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;

    if (This->pUnkOuter && This->bUnkOuterValid)
        return IUnknown_AddRef(This->pUnkOuter);
    return IUnknown_AddRef((IUnknown *)&(This->IInner_vtbl));
}

static ULONG WINAPI VideoRenderer_Release(IBaseFilter * iface)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;

    if (This->pUnkOuter && This->bUnkOuterValid)
        return IUnknown_Release(This->pUnkOuter);
    return IUnknown_Release((IUnknown *)&(This->IInner_vtbl));
}

/** IPersist methods **/

static HRESULT WINAPI VideoRenderer_GetClassID(IBaseFilter * iface, CLSID * pClsid)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pClsid);

    *pClsid = CLSID_VideoRenderer;

    return S_OK;
}

/** IMediaFilter methods **/

static HRESULT WINAPI VideoRenderer_Stop(IBaseFilter * iface)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->csFilter);
    {
        This->state = State_Stopped;
        SetEvent(This->hEvent);
        SetEvent(This->blocked);
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI VideoRenderer_Pause(IBaseFilter * iface)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;
    
    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->csFilter);
    if (This->state != State_Paused)
    {
        if (This->state == State_Stopped)
        {
            This->pInputPin->end_of_stream = 0;
            ResetEvent(This->hEvent);
        }

        This->state = State_Paused;
        ResetEvent(This->blocked);
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI VideoRenderer_Run(IBaseFilter * iface, REFERENCE_TIME tStart)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;

    TRACE("(%p/%p)->(%s)\n", This, iface, wine_dbgstr_longlong(tStart));

    EnterCriticalSection(&This->csFilter);
    if (This->state != State_Running)
    {
        if (This->state == State_Stopped)
        {
            This->pInputPin->end_of_stream = 0;
            ResetEvent(This->hEvent);
        }
        SetEvent(This->blocked);

        This->rtStreamStart = tStart;
        This->state = State_Running;
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI VideoRenderer_GetState(IBaseFilter * iface, DWORD dwMilliSecsTimeout, FILTER_STATE *pState)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;
    HRESULT hr;

    TRACE("(%p/%p)->(%d, %p)\n", This, iface, dwMilliSecsTimeout, pState);

    if (WaitForSingleObject(This->hEvent, dwMilliSecsTimeout) == WAIT_TIMEOUT)
        hr = VFW_S_STATE_INTERMEDIATE;
    else
        hr = S_OK;

    EnterCriticalSection(&This->csFilter);
    {
        *pState = This->state;
    }
    LeaveCriticalSection(&This->csFilter);

    return hr;
}

static HRESULT WINAPI VideoRenderer_SetSyncSource(IBaseFilter * iface, IReferenceClock *pClock)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pClock);

    EnterCriticalSection(&This->csFilter);
    {
        if (This->pClock)
            IReferenceClock_Release(This->pClock);
        This->pClock = pClock;
        if (This->pClock)
            IReferenceClock_AddRef(This->pClock);
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI VideoRenderer_GetSyncSource(IBaseFilter * iface, IReferenceClock **ppClock)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, ppClock);

    EnterCriticalSection(&This->csFilter);
    {
        *ppClock = This->pClock;
        if (This->pClock)
            IReferenceClock_AddRef(This->pClock);
    }
    LeaveCriticalSection(&This->csFilter);
    
    return S_OK;
}

/** IBaseFilter implementation **/

static HRESULT VideoRenderer_GetPin(IBaseFilter *iface, ULONG pos, IPin **pin, DWORD *lastsynctick)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;

    /* Our pins are static, not changing so setting static tick count is ok */
    *lastsynctick = 0;

    if (pos >= 1)
        return S_FALSE;

    *pin = (IPin *)This->pInputPin;
    IPin_AddRef(*pin);
    return S_OK;
}

static HRESULT WINAPI VideoRenderer_EnumPins(IBaseFilter * iface, IEnumPins **ppEnum)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, ppEnum);

    return IEnumPinsImpl_Construct(ppEnum, VideoRenderer_GetPin, iface);
}

static HRESULT WINAPI VideoRenderer_FindPin(IBaseFilter * iface, LPCWSTR Id, IPin **ppPin)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;

    TRACE("(%p/%p)->(%p,%p)\n", This, iface, debugstr_w(Id), ppPin);

    FIXME("VideoRenderer::FindPin(...)\n");

    /* FIXME: critical section */

    return E_NOTIMPL;
}

static HRESULT WINAPI VideoRenderer_QueryFilterInfo(IBaseFilter * iface, FILTER_INFO *pInfo)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pInfo);

    strcpyW(pInfo->achName, This->filterInfo.achName);
    pInfo->pGraph = This->filterInfo.pGraph;

    if (pInfo->pGraph)
        IFilterGraph_AddRef(pInfo->pGraph);
    
    return S_OK;
}

static HRESULT WINAPI VideoRenderer_JoinFilterGraph(IBaseFilter * iface, IFilterGraph *pGraph, LPCWSTR pName)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;

    TRACE("(%p/%p)->(%p, %s)\n", This, iface, pGraph, debugstr_w(pName));

    EnterCriticalSection(&This->csFilter);
    {
        if (pName)
            strcpyW(This->filterInfo.achName, pName);
        else
            *This->filterInfo.achName = '\0';
        This->filterInfo.pGraph = pGraph; /* NOTE: do NOT increase ref. count */
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI VideoRenderer_QueryVendorInfo(IBaseFilter * iface, LPWSTR *pVendorInfo)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;
    TRACE("(%p/%p)->(%p)\n", This, iface, pVendorInfo);
    return E_NOTIMPL;
}

static const IBaseFilterVtbl VideoRenderer_Vtbl =
{
    VideoRenderer_QueryInterface,
    VideoRenderer_AddRef,
    VideoRenderer_Release,
    VideoRenderer_GetClassID,
    VideoRenderer_Stop,
    VideoRenderer_Pause,
    VideoRenderer_Run,
    VideoRenderer_GetState,
    VideoRenderer_SetSyncSource,
    VideoRenderer_GetSyncSource,
    VideoRenderer_EnumPins,
    VideoRenderer_FindPin,
    VideoRenderer_QueryFilterInfo,
    VideoRenderer_JoinFilterGraph,
    VideoRenderer_QueryVendorInfo
};

static HRESULT WINAPI VideoRenderer_InputPin_EndOfStream(IPin * iface)
{
    InputPin* This = (InputPin*)iface;
    IMediaEventSink* pEventSink;
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    hr = IFilterGraph_QueryInterface(((VideoRendererImpl*)This->pin.pinInfo.pFilter)->filterInfo.pGraph, &IID_IMediaEventSink, (LPVOID*)&pEventSink);
    if (SUCCEEDED(hr))
    {
        hr = IMediaEventSink_Notify(pEventSink, EC_COMPLETE, S_OK, 0);
        IMediaEventSink_Release(pEventSink);
    }

    return hr;
}

static HRESULT WINAPI VideoRenderer_InputPin_BeginFlush(IPin * iface)
{
    InputPin* This = (InputPin*)iface;
    VideoRendererImpl *pVideoRenderer = (VideoRendererImpl *)This->pin.pinInfo.pFilter;
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(This->pin.pCritSec);
    if (pVideoRenderer->state == State_Paused)
        SetEvent(pVideoRenderer->blocked);

    hr = InputPin_BeginFlush(iface);
    LeaveCriticalSection(This->pin.pCritSec);

    return hr;
}

static HRESULT WINAPI VideoRenderer_InputPin_EndFlush(IPin * iface)
{
    InputPin* This = (InputPin*)iface;
    VideoRendererImpl *pVideoRenderer = (VideoRendererImpl *)This->pin.pinInfo.pFilter;
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(This->pin.pCritSec);
    if (pVideoRenderer->state == State_Paused)
        ResetEvent(pVideoRenderer->blocked);

    hr = InputPin_EndFlush(iface);
    LeaveCriticalSection(This->pin.pCritSec);

    return hr;
}

static const IPinVtbl VideoRenderer_InputPin_Vtbl = 
{
    InputPin_QueryInterface,
    IPinImpl_AddRef,
    InputPin_Release,
    InputPin_Connect,
    InputPin_ReceiveConnection,
    IPinImpl_Disconnect,
    IPinImpl_ConnectedTo,
    IPinImpl_ConnectionMediaType,
    IPinImpl_QueryPinInfo,
    IPinImpl_QueryDirection,
    IPinImpl_QueryId,
    IPinImpl_QueryAccept,
    IPinImpl_EnumMediaTypes,
    IPinImpl_QueryInternalConnections,
    VideoRenderer_InputPin_EndOfStream,
    VideoRenderer_InputPin_BeginFlush,
    VideoRenderer_InputPin_EndFlush,
    InputPin_NewSegment
};

/*** IUnknown methods ***/
static HRESULT WINAPI Basicvideo_QueryInterface(IBasicVideo *iface,
						REFIID riid,
						LPVOID*ppvObj) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return VideoRenderer_QueryInterface((IBaseFilter*)This, riid, ppvObj);
}

static ULONG WINAPI Basicvideo_AddRef(IBasicVideo *iface) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return VideoRenderer_AddRef((IBaseFilter*)This);
}

static ULONG WINAPI Basicvideo_Release(IBasicVideo *iface) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return VideoRenderer_Release((IBaseFilter*)This);
}

/*** IDispatch methods ***/
static HRESULT WINAPI Basicvideo_GetTypeInfoCount(IBasicVideo *iface,
						  UINT*pctinfo) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, pctinfo);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_GetTypeInfo(IBasicVideo *iface,
					     UINT iTInfo,
					     LCID lcid,
					     ITypeInfo**ppTInfo) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    FIXME("(%p/%p)->(%d, %d, %p): stub !!!\n", This, iface, iTInfo, lcid, ppTInfo);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_GetIDsOfNames(IBasicVideo *iface,
					       REFIID riid,
					       LPOLESTR*rgszNames,
					       UINT cNames,
					       LCID lcid,
					       DISPID*rgDispId) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    FIXME("(%p/%p)->(%s (%p), %p, %d, %d, %p): stub !!!\n", This, iface, debugstr_guid(riid), riid, rgszNames, cNames, lcid, rgDispId);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_Invoke(IBasicVideo *iface,
					DISPID dispIdMember,
					REFIID riid,
					LCID lcid,
					WORD wFlags,
					DISPPARAMS*pDispParams,
					VARIANT*pVarResult,
					EXCEPINFO*pExepInfo,
					UINT*puArgErr) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    FIXME("(%p/%p)->(%d, %s (%p), %d, %04x, %p, %p, %p, %p): stub !!!\n", This, iface, dispIdMember, debugstr_guid(riid), riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    return S_OK;
}

/*** IBasicVideo methods ***/
static HRESULT WINAPI Basicvideo_get_AvgTimePerFrame(IBasicVideo *iface,
						     REFTIME *pAvgTimePerFrame) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, pAvgTimePerFrame);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_BitRate(IBasicVideo *iface,
					     long *pBitRate) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, pBitRate);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_BitErrorRate(IBasicVideo *iface,
						  long *pBitErrorRate) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, pBitErrorRate);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_VideoWidth(IBasicVideo *iface,
						long *pVideoWidth) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pVideoWidth);

    *pVideoWidth = This->VideoWidth;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_VideoHeight(IBasicVideo *iface,
						 long *pVideoHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pVideoHeight);

    *pVideoHeight = This->VideoHeight;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_SourceLeft(IBasicVideo *iface,
						long SourceLeft) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, SourceLeft);

    This->SourceRect.left = SourceLeft;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_SourceLeft(IBasicVideo *iface,
						long *pSourceLeft) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceLeft);

    *pSourceLeft = This->SourceRect.left;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_SourceWidth(IBasicVideo *iface,
						 long SourceWidth) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, SourceWidth);

    This->SourceRect.right = This->SourceRect.left + SourceWidth;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_SourceWidth(IBasicVideo *iface,
						 long *pSourceWidth) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceWidth);

    *pSourceWidth = This->SourceRect.right - This->SourceRect.left;
    
    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_SourceTop(IBasicVideo *iface,
					       long SourceTop) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, SourceTop);

    This->SourceRect.top = SourceTop;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_SourceTop(IBasicVideo *iface,
					       long *pSourceTop) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceTop);

    *pSourceTop = This->SourceRect.top;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_SourceHeight(IBasicVideo *iface,
						  long SourceHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, SourceHeight);

    This->SourceRect.bottom = This->SourceRect.top + SourceHeight;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_SourceHeight(IBasicVideo *iface,
						  long *pSourceHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceHeight);

    *pSourceHeight = This->SourceRect.bottom - This->SourceRect.top;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_DestinationLeft(IBasicVideo *iface,
						     long DestinationLeft) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, DestinationLeft);

    This->DestRect.left = DestinationLeft;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_DestinationLeft(IBasicVideo *iface,
						     long *pDestinationLeft) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationLeft);

    *pDestinationLeft = This->DestRect.left;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_DestinationWidth(IBasicVideo *iface,
						      long DestinationWidth) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, DestinationWidth);

    This->DestRect.right = This->DestRect.left + DestinationWidth;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_DestinationWidth(IBasicVideo *iface,
						      long *pDestinationWidth) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationWidth);

    *pDestinationWidth = This->DestRect.right - This->DestRect.left;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_DestinationTop(IBasicVideo *iface,
						    long DestinationTop) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, DestinationTop);

    This->DestRect.top = DestinationTop;
    
    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_DestinationTop(IBasicVideo *iface,
						    long *pDestinationTop) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationTop);

    *pDestinationTop = This->DestRect.top;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_DestinationHeight(IBasicVideo *iface,
						       long DestinationHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, DestinationHeight);

    This->DestRect.right = This->DestRect.left + DestinationHeight;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_DestinationHeight(IBasicVideo *iface,
						       long *pDestinationHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationHeight);

    *pDestinationHeight = This->DestRect.right - This->DestRect.left;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_SetSourcePosition(IBasicVideo *iface,
						   long Left,
						   long Top,
						   long Width,
						   long Height) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld, %ld, %ld, %ld)\n", This, iface, Left, Top, Width, Height);

    This->SourceRect.left = Left;
    This->SourceRect.top = Top;
    This->SourceRect.right = Left + Width;
    This->SourceRect.bottom = Top + Height;
    
    return S_OK;
}

static HRESULT WINAPI Basicvideo_GetSourcePosition(IBasicVideo *iface,
						   long *pLeft,
						   long *pTop,
						   long *pWidth,
						   long *pHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);

    *pLeft = This->SourceRect.left;
    *pTop = This->SourceRect.top;
    *pWidth = This->SourceRect.right - This->SourceRect.left;
    *pHeight = This->SourceRect.bottom - This->SourceRect.top;
    
    return S_OK;
}

static HRESULT WINAPI Basicvideo_SetDefaultSourcePosition(IBasicVideo *iface) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    This->SourceRect.left = 0;
    This->SourceRect.top = 0;
    This->SourceRect.right = This->VideoWidth;
    This->SourceRect.bottom = This->VideoHeight;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_SetDestinationPosition(IBasicVideo *iface,
							long Left,
							long Top,
							long Width,
							long Height) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld, %ld, %ld, %ld)\n", This, iface, Left, Top, Width, Height);

    This->DestRect.left = Left;
    This->DestRect.top = Top;
    This->DestRect.right = Left + Width;
    This->DestRect.bottom = Top + Height;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_GetDestinationPosition(IBasicVideo *iface,
							long *pLeft,
							long *pTop,
							long *pWidth,
							long *pHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);

    *pLeft = This->DestRect.left;
    *pTop = This->DestRect.top;
    *pWidth = This->DestRect.right - This->DestRect.left;
    *pHeight = This->DestRect.bottom - This->DestRect.top;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_SetDefaultDestinationPosition(IBasicVideo *iface) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);
    RECT rect;

    TRACE("(%p/%p)->()\n", This, iface);

    if (!GetClientRect(This->hWnd, &rect))
        return E_FAIL;
    
    This->SourceRect.left = 0;
    This->SourceRect.top = 0;
    This->SourceRect.right = rect.right;
    This->SourceRect.bottom = rect.bottom;
    
    return S_OK;
}

static HRESULT WINAPI Basicvideo_GetVideoSize(IBasicVideo *iface,
					      long *pWidth,
					      long *pHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pWidth, pHeight);

    *pWidth = This->VideoWidth;
    *pHeight = This->VideoHeight;
    
    return S_OK;
}

static HRESULT WINAPI Basicvideo_GetVideoPaletteEntries(IBasicVideo *iface,
							long StartIndex,
							long Entries,
							long *pRetrieved,
							long *pPalette) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    FIXME("(%p/%p)->(%ld, %ld, %p, %p): stub !!!\n", This, iface, StartIndex, Entries, pRetrieved, pPalette);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_GetCurrentImage(IBasicVideo *iface,
						 long *pBufferSize,
						 long *pDIBImage) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);
    BITMAPINFOHEADER *bmiHeader;
    LONG needed_size;
    AM_MEDIA_TYPE *amt = &This->pInputPin->pin.mtCurrent;
    char *ptr;

    EnterCriticalSection(&This->csFilter);

    if (!This->sample_held)
    {
         LeaveCriticalSection(&This->csFilter);
         return (This->state == State_Paused ? E_UNEXPECTED : VFW_E_NOT_PAUSED);
    }

    FIXME("(%p/%p)->(%p, %p): partial stub\n", This, iface, pBufferSize, pDIBImage);

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
        LeaveCriticalSection(&This->csFilter);
        return VFW_E_RUNTIME_ERROR;
    }

    needed_size = bmiHeader->biSize;
    needed_size += IMediaSample_GetActualDataLength(This->sample_held);

    if (!pDIBImage)
    {
        *pBufferSize = needed_size;
        LeaveCriticalSection(&This->csFilter);
        return S_OK;
    }

    if (needed_size < *pBufferSize)
    {
        ERR("Buffer too small %u/%lu\n", needed_size, *pBufferSize);
        LeaveCriticalSection(&This->csFilter);
        return E_FAIL;
    }
    *pBufferSize = needed_size;

    memcpy(pDIBImage, bmiHeader, bmiHeader->biSize);
    IMediaSample_GetPointer(This->sample_held, (BYTE **)&ptr);
    memcpy((char *)pDIBImage + bmiHeader->biSize, ptr, IMediaSample_GetActualDataLength(This->sample_held));

    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_IsUsingDefaultSource(IBasicVideo *iface) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    FIXME("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_IsUsingDefaultDestination(IBasicVideo *iface) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    FIXME("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}


static const IBasicVideoVtbl IBasicVideo_VTable =
{
    Basicvideo_QueryInterface,
    Basicvideo_AddRef,
    Basicvideo_Release,
    Basicvideo_GetTypeInfoCount,
    Basicvideo_GetTypeInfo,
    Basicvideo_GetIDsOfNames,
    Basicvideo_Invoke,
    Basicvideo_get_AvgTimePerFrame,
    Basicvideo_get_BitRate,
    Basicvideo_get_BitErrorRate,
    Basicvideo_get_VideoWidth,
    Basicvideo_get_VideoHeight,
    Basicvideo_put_SourceLeft,
    Basicvideo_get_SourceLeft,
    Basicvideo_put_SourceWidth,
    Basicvideo_get_SourceWidth,
    Basicvideo_put_SourceTop,
    Basicvideo_get_SourceTop,
    Basicvideo_put_SourceHeight,
    Basicvideo_get_SourceHeight,
    Basicvideo_put_DestinationLeft,
    Basicvideo_get_DestinationLeft,
    Basicvideo_put_DestinationWidth,
    Basicvideo_get_DestinationWidth,
    Basicvideo_put_DestinationTop,
    Basicvideo_get_DestinationTop,
    Basicvideo_put_DestinationHeight,
    Basicvideo_get_DestinationHeight,
    Basicvideo_SetSourcePosition,
    Basicvideo_GetSourcePosition,
    Basicvideo_SetDefaultSourcePosition,
    Basicvideo_SetDestinationPosition,
    Basicvideo_GetDestinationPosition,
    Basicvideo_SetDefaultDestinationPosition,
    Basicvideo_GetVideoSize,
    Basicvideo_GetVideoPaletteEntries,
    Basicvideo_GetCurrentImage,
    Basicvideo_IsUsingDefaultSource,
    Basicvideo_IsUsingDefaultDestination
};


/*** IUnknown methods ***/
static HRESULT WINAPI Videowindow_QueryInterface(IVideoWindow *iface,
						 REFIID riid,
						 LPVOID*ppvObj) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return VideoRenderer_QueryInterface((IBaseFilter*)This, riid, ppvObj);
}

static ULONG WINAPI Videowindow_AddRef(IVideoWindow *iface) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return VideoRenderer_AddRef((IBaseFilter*)This);
}

static ULONG WINAPI Videowindow_Release(IVideoWindow *iface) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return VideoRenderer_Release((IBaseFilter*)This);
}

/*** IDispatch methods ***/
static HRESULT WINAPI Videowindow_GetTypeInfoCount(IVideoWindow *iface,
						   UINT*pctinfo) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, pctinfo);

    return S_OK;
}

static HRESULT WINAPI Videowindow_GetTypeInfo(IVideoWindow *iface,
					      UINT iTInfo,
					      LCID lcid,
					      ITypeInfo**ppTInfo) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%d, %d, %p): stub !!!\n", This, iface, iTInfo, lcid, ppTInfo);

    return S_OK;
}

static HRESULT WINAPI Videowindow_GetIDsOfNames(IVideoWindow *iface,
						REFIID riid,
						LPOLESTR*rgszNames,
						UINT cNames,
						LCID lcid,
						DISPID*rgDispId) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%s (%p), %p, %d, %d, %p): stub !!!\n", This, iface, debugstr_guid(riid), riid, rgszNames, cNames, lcid, rgDispId);

    return S_OK;
}

static HRESULT WINAPI Videowindow_Invoke(IVideoWindow *iface,
					 DISPID dispIdMember,
					 REFIID riid,
					 LCID lcid,
					 WORD wFlags,
					 DISPPARAMS*pDispParams,
					 VARIANT*pVarResult,
					 EXCEPINFO*pExepInfo,
					 UINT*puArgErr) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%d, %s (%p), %d, %04x, %p, %p, %p, %p): stub !!!\n", This, iface, dispIdMember, debugstr_guid(riid), riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    return S_OK;
}

/*** IVideoWindow methods ***/
static HRESULT WINAPI Videowindow_put_Caption(IVideoWindow *iface,
					      BSTR strCaption) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p))\n", This, iface, debugstr_w(strCaption), strCaption);

    if (!SetWindowTextW(This->hWnd, strCaption))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Caption(IVideoWindow *iface,
					      BSTR *strCaption) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, strCaption);

    GetWindowTextW(This->hWnd, (LPWSTR)strCaption, 100);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_WindowStyle(IVideoWindow *iface,
						  long WindowStyle) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);
    LONG old;

    old = GetWindowLongA(This->hWnd, GWL_STYLE);
    
    TRACE("(%p/%p)->(%x -> %lx)\n", This, iface, old, WindowStyle);

    if (WindowStyle & (WS_DISABLED|WS_HSCROLL|WS_ICONIC|WS_MAXIMIZE|WS_MINIMIZE|WS_VSCROLL))
        return E_INVALIDARG;
    
    SetWindowLongA(This->hWnd, GWL_STYLE, WindowStyle);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_WindowStyle(IVideoWindow *iface,
						  long *WindowStyle) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, WindowStyle);

    *WindowStyle = GetWindowLongA(This->hWnd, GWL_STYLE);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_WindowStyleEx(IVideoWindow *iface,
						    long WindowStyleEx) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, WindowStyleEx);

    if (WindowStyleEx & (WS_DISABLED|WS_HSCROLL|WS_ICONIC|WS_MAXIMIZE|WS_MINIMIZE|WS_VSCROLL))
        return E_INVALIDARG;

    if (!SetWindowLongA(This->hWnd, GWL_EXSTYLE, WindowStyleEx))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_WindowStyleEx(IVideoWindow *iface,
						    long *WindowStyleEx) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, WindowStyleEx);

    *WindowStyleEx = GetWindowLongA(This->hWnd, GWL_EXSTYLE);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_AutoShow(IVideoWindow *iface,
					       long AutoShow) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, AutoShow);

    This->AutoShow = 1; /* FXIME: Should be AutoShow */;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_AutoShow(IVideoWindow *iface,
					       long *AutoShow) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, AutoShow);

    *AutoShow = This->AutoShow;

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_WindowState(IVideoWindow *iface,
						  long WindowState) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%ld): stub !!!\n", This, iface, WindowState);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_WindowState(IVideoWindow *iface,
						  long *WindowState) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, WindowState);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_BackgroundPalette(IVideoWindow *iface,
							long BackgroundPalette) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%ld): stub !!!\n", This, iface, BackgroundPalette);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_BackgroundPalette(IVideoWindow *iface,
							long *pBackgroundPalette) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, pBackgroundPalette);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_Visible(IVideoWindow *iface,
					      long Visible) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, Visible);

    ShowWindow(This->hWnd, Visible ? SW_SHOW : SW_HIDE);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Visible(IVideoWindow *iface,
					      long *pVisible) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pVisible);

    *pVisible = IsWindowVisible(This->hWnd);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_Left(IVideoWindow *iface,
					   long Left) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, Left);

    if (!SetWindowPos(This->hWnd, NULL, Left, This->WindowPos.top, 0, 0, SWP_NOZORDER|SWP_NOSIZE))
        return E_FAIL;

    This->WindowPos.left = Left;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Left(IVideoWindow *iface,
					   long *pLeft) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pLeft);

    *pLeft = This->WindowPos.left;

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_Width(IVideoWindow *iface,
					    long Width) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, Width);

    if (!SetWindowPos(This->hWnd, NULL, 0, 0, Width, This->WindowPos.bottom-This->WindowPos.top, SWP_NOZORDER|SWP_NOMOVE))
        return E_FAIL;

    This->WindowPos.right = This->WindowPos.left + Width;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Width(IVideoWindow *iface,
					    long *pWidth) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pWidth);

    *pWidth = This->WindowPos.right - This->WindowPos.left;

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_Top(IVideoWindow *iface,
					  long Top) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, Top);

    if (!SetWindowPos(This->hWnd, NULL, This->WindowPos.left, Top, 0, 0, SWP_NOZORDER|SWP_NOSIZE))
        return E_FAIL;

    This->WindowPos.top = Top;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Top(IVideoWindow *iface,
					  long *pTop) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pTop);

    *pTop = This->WindowPos.top;

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_Height(IVideoWindow *iface,
					     long Height) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, Height);

    if (!SetWindowPos(This->hWnd, NULL, 0, 0, This->WindowPos.right-This->WindowPos.left, Height, SWP_NOZORDER|SWP_NOMOVE))
        return E_FAIL;

    This->WindowPos.bottom = This->WindowPos.top + Height;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Height(IVideoWindow *iface,
					     long *pHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pHeight);

    *pHeight = This->WindowPos.bottom - This->WindowPos.top;

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_Owner(IVideoWindow *iface,
					    OAHWND Owner) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%08x)\n", This, iface, (DWORD) Owner);

    SetParent(This->hWnd, (HWND)Owner);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Owner(IVideoWindow *iface,
					    OAHWND *Owner) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%08x)\n", This, iface, (DWORD) Owner);

    *(HWND*)Owner = GetParent(This->hWnd);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_MessageDrain(IVideoWindow *iface,
						   OAHWND Drain) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%08x)\n", This, iface, (DWORD) Drain);

    This->hWndMsgDrain = (HWND)Drain;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_MessageDrain(IVideoWindow *iface,
						   OAHWND *Drain) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, Drain);

    *Drain = (OAHWND)This->hWndMsgDrain;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_BorderColor(IVideoWindow *iface,
						  long *Color) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, Color);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_BorderColor(IVideoWindow *iface,
						  long Color) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%ld): stub !!!\n", This, iface, Color);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_FullScreenMode(IVideoWindow *iface,
						     long *FullScreenMode) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, FullScreenMode);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_FullScreenMode(IVideoWindow *iface,
						     long FullScreenMode) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%ld): stub !!!\n", This, iface, FullScreenMode);

    return S_OK;
}

static HRESULT WINAPI Videowindow_SetWindowForeground(IVideoWindow *iface,
						      long Focus) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);
    BOOL ret;
    IPin* pPin;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, Focus);

    if ((Focus != FALSE) && (Focus != TRUE))
        return E_INVALIDARG;

    hr = IPin_ConnectedTo((IPin *)This->pInputPin, &pPin);
    if ((hr != S_OK) || !pPin)
        return VFW_E_NOT_CONNECTED;

    if (Focus)
        ret = SetForegroundWindow(This->hWnd);
    else
        ret = SetWindowPos(This->hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);

    if (!ret)
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI Videowindow_NotifyOwnerMessage(IVideoWindow *iface,
						     OAHWND hwnd,
						     long uMsg,
						     LONG_PTR wParam,
						     LONG_PTR lParam) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%08x, %ld, %08lx, %08lx)\n", This, iface, (DWORD) hwnd, uMsg, wParam, lParam);

    if (!PostMessageA(This->hWnd, uMsg, wParam, lParam))
        return E_FAIL;
    
    return S_OK;
}

static HRESULT WINAPI Videowindow_SetWindowPosition(IVideoWindow *iface,
						    long Left,
						    long Top,
						    long Width,
						    long Height) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);
    
    TRACE("(%p/%p)->(%ld, %ld, %ld, %ld)\n", This, iface, Left, Top, Width, Height);

    if (!SetWindowPos(This->hWnd, NULL, Left, Top, Width, Height, SWP_NOZORDER))
        return E_FAIL;

    This->WindowPos.left = Left;
    This->WindowPos.top = Top;
    This->WindowPos.right = Left + Width;
    This->WindowPos.bottom = Top + Height;
    
    return S_OK;
}

static HRESULT WINAPI Videowindow_GetWindowPosition(IVideoWindow *iface,
						    long *pLeft,
						    long *pTop,
						    long *pWidth,
						    long *pHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);

    *pLeft = This->WindowPos.left;
    *pTop = This->WindowPos.top;
    *pWidth = This->WindowPos.right - This->WindowPos.left;
    *pHeight = This->WindowPos.bottom - This->WindowPos.top;

    return S_OK;
}

static HRESULT WINAPI Videowindow_GetMinIdealImageSize(IVideoWindow *iface,
						       long *pWidth,
						       long *pHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%p, %p): semi stub !!!\n", This, iface, pWidth, pHeight);

    *pWidth = This->VideoWidth;
    *pHeight = This->VideoHeight;

    return S_OK;
}

static HRESULT WINAPI Videowindow_GetMaxIdealImageSize(IVideoWindow *iface,
						       long *pWidth,
						       long *pHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%p, %p): semi stub !!!\n", This, iface, pWidth, pHeight);

    *pWidth = This->VideoWidth;
    *pHeight = This->VideoHeight;

    return S_OK;
}

static HRESULT WINAPI Videowindow_GetRestorePosition(IVideoWindow *iface,
						     long *pLeft,
						     long *pTop,
						     long *pWidth,
						     long *pHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%p, %p, %p, %p): stub !!!\n", This, iface, pLeft, pTop, pWidth, pHeight);

    return S_OK;
}

static HRESULT WINAPI Videowindow_HideCursor(IVideoWindow *iface,
					     long HideCursor) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%ld): stub !!!\n", This, iface, HideCursor);

    return S_OK;
}

static HRESULT WINAPI Videowindow_IsCursorHidden(IVideoWindow *iface,
						 long *CursorHidden) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, CursorHidden);

    return S_OK;
}

static const IVideoWindowVtbl IVideoWindow_VTable =
{
    Videowindow_QueryInterface,
    Videowindow_AddRef,
    Videowindow_Release,
    Videowindow_GetTypeInfoCount,
    Videowindow_GetTypeInfo,
    Videowindow_GetIDsOfNames,
    Videowindow_Invoke,
    Videowindow_put_Caption,
    Videowindow_get_Caption,
    Videowindow_put_WindowStyle,
    Videowindow_get_WindowStyle,
    Videowindow_put_WindowStyleEx,
    Videowindow_get_WindowStyleEx,
    Videowindow_put_AutoShow,
    Videowindow_get_AutoShow,
    Videowindow_put_WindowState,
    Videowindow_get_WindowState,
    Videowindow_put_BackgroundPalette,
    Videowindow_get_BackgroundPalette,
    Videowindow_put_Visible,
    Videowindow_get_Visible,
    Videowindow_put_Left,
    Videowindow_get_Left,
    Videowindow_put_Width,
    Videowindow_get_Width,
    Videowindow_put_Top,
    Videowindow_get_Top,
    Videowindow_put_Height,
    Videowindow_get_Height,
    Videowindow_put_Owner,
    Videowindow_get_Owner,
    Videowindow_put_MessageDrain,
    Videowindow_get_MessageDrain,
    Videowindow_get_BorderColor,
    Videowindow_put_BorderColor,
    Videowindow_get_FullScreenMode,
    Videowindow_put_FullScreenMode,
    Videowindow_SetWindowForeground,
    Videowindow_NotifyOwnerMessage,
    Videowindow_SetWindowPosition,
    Videowindow_GetWindowPosition,
    Videowindow_GetMinIdealImageSize,
    Videowindow_GetMaxIdealImageSize,
    Videowindow_GetRestorePosition,
    Videowindow_HideCursor,
    Videowindow_IsCursorHidden
};
