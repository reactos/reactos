/*
 * Generic Implementation of IPin Interface
 *
 * Copyright 2003 Robert Shearman
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

#include "quartz_private.h"

static const IPinVtbl PullPin_Vtbl;

#define ALIGNDOWN(value,boundary) ((value)/(boundary)*(boundary))
#define ALIGNUP(value,boundary) (ALIGNDOWN((value)+(boundary)-1, (boundary)))

typedef HRESULT (*SendPinFunc)( IPin *to, LPVOID arg );

/** Helper function, there are a lot of places where the error code is inherited
 * The following rules apply:
 *
 * Return the first received error code (E_NOTIMPL is ignored)
 * If no errors occur: return the first received non-error-code that isn't S_OK
 */
static HRESULT updatehres( HRESULT original, HRESULT new )
{
    if (FAILED( original ) || new == E_NOTIMPL)
        return original;

    if (FAILED( new ) || original == S_OK)
        return new;

    return original;
}

/** Sends a message from a pin further to other, similar pins
 * fnMiddle is called on each pin found further on the stream.
 * fnEnd (can be NULL) is called when the message can't be sent any further (this is a renderer or source)
 *
 * If the pin given is an input pin, the message will be sent downstream to other input pins
 * If the pin given is an output pin, the message will be sent upstream to other output pins
 */
static HRESULT SendFurther( IPin *from, SendPinFunc fnMiddle, LPVOID arg, SendPinFunc fnEnd )
{
    PIN_INFO pin_info;
    ULONG amount = 0;
    HRESULT hr = S_OK;
    HRESULT hr_return = S_OK;
    IEnumPins *enumpins = NULL;
    BOOL foundend = TRUE;
    PIN_DIRECTION from_dir;

    IPin_QueryDirection( from, &from_dir );

    hr = IPin_QueryInternalConnections( from, NULL, &amount );
    if (hr != E_NOTIMPL && amount)
        FIXME("Use QueryInternalConnections!\n");

    pin_info.pFilter = NULL;
    hr = IPin_QueryPinInfo( from, &pin_info );
    if (FAILED(hr))
        goto out;

    hr = IBaseFilter_EnumPins( pin_info.pFilter, &enumpins );
    if (FAILED(hr))
        goto out;

    hr = IEnumPins_Reset( enumpins );
    while (hr == S_OK) {
        IPin *pin = NULL;
        hr = IEnumPins_Next( enumpins, 1, &pin, NULL );
        if (hr == VFW_E_ENUM_OUT_OF_SYNC)
        {
            hr = IEnumPins_Reset( enumpins );
            continue;
        }
        if (pin)
        {
            PIN_DIRECTION dir;

            IPin_QueryDirection( pin, &dir );
            if (dir != from_dir)
            {
                IPin *connected = NULL;

                foundend = FALSE;
                IPin_ConnectedTo( pin, &connected );
                if (connected)
                {
                    HRESULT hr_local;

                    hr_local = fnMiddle( connected, arg );
                    hr_return = updatehres( hr_return, hr_local );
                    IPin_Release(connected);
                }
            }
            IPin_Release( pin );
        }
        else
        {
            hr = S_OK;
            break;
        }
    }

    if (!foundend)
        hr = hr_return;
    else if (fnEnd) {
        HRESULT hr_local;

        hr_local = fnEnd( from, arg );
        hr_return = updatehres( hr_return, hr_local );
    }

out:
    if (enumpins)
        IEnumPins_Release( enumpins );
    if (pin_info.pFilter)
        IBaseFilter_Release( pin_info.pFilter );
    return hr;
}


static void Copy_PinInfo(PIN_INFO * pDest, const PIN_INFO * pSrc)
{
    /* Tempting to just do a memcpy, but the name field is
       128 characters long! We will probably never exceed 10
       most of the time, so we are better off copying 
       each field manually */
    strcpyW(pDest->achName, pSrc->achName);
    pDest->dir = pSrc->dir;
    pDest->pFilter = pSrc->pFilter;
}

static HRESULT deliver_endofstream(IPin* pin, LPVOID unused)
{
    return IPin_EndOfStream( pin );
}

static HRESULT deliver_beginflush(IPin* pin, LPVOID unused)
{
    return IPin_BeginFlush( pin );
}

static HRESULT deliver_endflush(IPin* pin, LPVOID unused)
{
    return IPin_EndFlush( pin );
}

typedef struct newsegmentargs
{
    REFERENCE_TIME tStart, tStop;
    double rate;
} newsegmentargs;

static HRESULT deliver_newsegment(IPin *pin, LPVOID data)
{
    newsegmentargs *args = data;
    return IPin_NewSegment(pin, args->tStart, args->tStop, args->rate);
}

/*** PullPin implementation ***/

static HRESULT PullPin_Init(const IPinVtbl *PullPin_Vtbl, const PIN_INFO * pPinInfo, SAMPLEPROC_PULL pSampleProc, LPVOID pUserData,
                            QUERYACCEPTPROC pQueryAccept, CLEANUPPROC pCleanUp, REQUESTPROC pCustomRequest, STOPPROCESSPROC pDone, LPCRITICAL_SECTION pCritSec, PullPin * pPinImpl)
{
    /* Common attributes */
    pPinImpl->pin.IPin_iface.lpVtbl = PullPin_Vtbl;
    pPinImpl->pin.refCount = 1;
    pPinImpl->pin.pConnectedTo = NULL;
    pPinImpl->pin.pCritSec = pCritSec;
    Copy_PinInfo(&pPinImpl->pin.pinInfo, pPinInfo);
    ZeroMemory(&pPinImpl->pin.mtCurrent, sizeof(AM_MEDIA_TYPE));

    /* Input pin attributes */
    pPinImpl->pUserData = pUserData;
    pPinImpl->fnQueryAccept = pQueryAccept;
    pPinImpl->fnSampleProc = pSampleProc;
    pPinImpl->fnCleanProc = pCleanUp;
    pPinImpl->fnDone = pDone;
    pPinImpl->fnPreConnect = NULL;
    pPinImpl->pAlloc = NULL;
    pPinImpl->prefAlloc = NULL;
    pPinImpl->pReader = NULL;
    pPinImpl->hThread = NULL;
    pPinImpl->hEventStateChanged = CreateEventW(NULL, TRUE, TRUE, NULL);
    pPinImpl->thread_sleepy = CreateEventW(NULL, FALSE, FALSE, NULL);

    pPinImpl->rtStart = 0;
    pPinImpl->rtCurrent = 0;
    pPinImpl->rtStop = ((LONGLONG)0x7fffffff << 32) | 0xffffffff;
    pPinImpl->dRate = 1.0;
    pPinImpl->state = Req_Die;
    pPinImpl->fnCustomRequest = pCustomRequest;
    pPinImpl->stop_playback = TRUE;

    InitializeCriticalSection(&pPinImpl->thread_lock);
    pPinImpl->thread_lock.DebugInfo->Spare[0] = (DWORD_PTR)( __FILE__ ": PullPin.thread_lock");

    return S_OK;
}

HRESULT PullPin_Construct(const IPinVtbl *PullPin_Vtbl, const PIN_INFO * pPinInfo, SAMPLEPROC_PULL pSampleProc, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept, CLEANUPPROC pCleanUp, REQUESTPROC pCustomRequest, STOPPROCESSPROC pDone, LPCRITICAL_SECTION pCritSec, IPin ** ppPin)
{
    PullPin * pPinImpl;

    *ppPin = NULL;

    if (pPinInfo->dir != PINDIR_INPUT)
    {
        ERR("Pin direction(%x) != PINDIR_INPUT\n", pPinInfo->dir);
        return E_INVALIDARG;
    }

    pPinImpl = CoTaskMemAlloc(sizeof(*pPinImpl));

    if (!pPinImpl)
        return E_OUTOFMEMORY;

    if (SUCCEEDED(PullPin_Init(PullPin_Vtbl, pPinInfo, pSampleProc, pUserData, pQueryAccept, pCleanUp, pCustomRequest, pDone, pCritSec, pPinImpl)))
    {
        *ppPin = &pPinImpl->pin.IPin_iface;
        return S_OK;
    }

    CoTaskMemFree(pPinImpl);
    return E_FAIL;
}

static HRESULT PullPin_InitProcessing(PullPin * This);

HRESULT WINAPI PullPin_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    PIN_DIRECTION pindirReceive;
    HRESULT hr = S_OK;
    PullPin *This = impl_PullPin_from_IPin(iface);

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    EnterCriticalSection(This->pin.pCritSec);
    if (!This->pin.pConnectedTo)
    {
        ALLOCATOR_PROPERTIES props;

        props.cBuffers = 3;
        props.cbBuffer = 64 * 1024; /* 64 KB */
        props.cbAlign = 1;
        props.cbPrefix = 0;

        if (This->fnQueryAccept(This->pUserData, pmt) != S_OK)
            hr = VFW_E_TYPE_NOT_ACCEPTED; /* FIXME: shouldn't we just map common errors onto 
                                           * VFW_E_TYPE_NOT_ACCEPTED and pass the value on otherwise? */

        if (SUCCEEDED(hr))
        {
            IPin_QueryDirection(pReceivePin, &pindirReceive);

            if (pindirReceive != PINDIR_OUTPUT)
            {
                ERR("Can't connect from non-output pin\n");
                hr = VFW_E_INVALID_DIRECTION;
            }
        }

        This->pReader = NULL;
        This->pAlloc = NULL;
        This->prefAlloc = NULL;
        if (SUCCEEDED(hr))
        {
            hr = IPin_QueryInterface(pReceivePin, &IID_IAsyncReader, (LPVOID *)&This->pReader);
        }

        if (SUCCEEDED(hr) && This->fnPreConnect)
        {
            hr = This->fnPreConnect(iface, pReceivePin, &props);
        }

        /*
         * Some custom filters (such as the one used by Fallout 3
         * and Fallout: New Vegas) expect to be passed a non-NULL
         * preferred allocator.
         */
        if (SUCCEEDED(hr))
        {
            hr = StdMemAllocator_create(NULL, (LPVOID *) &This->prefAlloc);
        }

        if (SUCCEEDED(hr))
        {
            hr = IAsyncReader_RequestAllocator(This->pReader, This->prefAlloc, &props, &This->pAlloc);
        }

        if (SUCCEEDED(hr))
        {
            CopyMediaType(&This->pin.mtCurrent, pmt);
            This->pin.pConnectedTo = pReceivePin;
            IPin_AddRef(pReceivePin);
            hr = IMemAllocator_Commit(This->pAlloc);
        }

        if (SUCCEEDED(hr))
            hr = PullPin_InitProcessing(This);

        if (FAILED(hr))
        {
             if (This->pReader)
                 IAsyncReader_Release(This->pReader);
             This->pReader = NULL;
             if (This->prefAlloc)
                 IMemAllocator_Release(This->prefAlloc);
             This->prefAlloc = NULL;
             if (This->pAlloc)
                 IMemAllocator_Release(This->pAlloc);
             This->pAlloc = NULL;
        }
    }
    else
        hr = VFW_E_ALREADY_CONNECTED;
    LeaveCriticalSection(This->pin.pCritSec);
    return hr;
}

HRESULT WINAPI PullPin_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv)
{
    PullPin *This = impl_PullPin_from_IPin(iface);

    TRACE("(%p/%p)->(%s, %p)\n", This, iface, qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IPin))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IMediaSeeking) ||
             IsEqualIID(riid, &IID_IQualityControl))
    {
        return IBaseFilter_QueryInterface(This->pin.pinInfo.pFilter, riid, ppv);
    }

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

ULONG WINAPI PullPin_Release(IPin *iface)
{
    PullPin *This = impl_PullPin_from_IPin(iface);
    ULONG refCount = InterlockedDecrement(&This->pin.refCount);

    TRACE("(%p)->() Release from %d\n", This, refCount + 1);

    if (!refCount)
    {
        WaitForSingleObject(This->hEventStateChanged, INFINITE);
        assert(!This->hThread);

        if(This->prefAlloc)
            IMemAllocator_Release(This->prefAlloc);
        if(This->pAlloc)
            IMemAllocator_Release(This->pAlloc);
        if(This->pReader)
            IAsyncReader_Release(This->pReader);
        CloseHandle(This->thread_sleepy);
        CloseHandle(This->hEventStateChanged);
        This->thread_lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->thread_lock);
        CoTaskMemFree(This);
        return 0;
    }
    return refCount;
}

static void PullPin_Flush(PullPin *This)
{
    IMediaSample *pSample;
    TRACE("Flushing!\n");

    if (This->pReader)
    {
        /* Do not allow state to change while flushing */
        EnterCriticalSection(This->pin.pCritSec);

        /* Flush outstanding samples */
        IAsyncReader_BeginFlush(This->pReader);

        for (;;)
        {
            DWORD_PTR dwUser;

            pSample = NULL;
            IAsyncReader_WaitForNext(This->pReader, 0, &pSample, &dwUser);

            if (!pSample)
                break;

            assert(!IMediaSample_GetActualDataLength(pSample));

            IMediaSample_Release(pSample);
        }

        IAsyncReader_EndFlush(This->pReader);

        LeaveCriticalSection(This->pin.pCritSec);
    }
}

static void PullPin_Thread_Process(PullPin *This)
{
    HRESULT hr;
    IMediaSample * pSample = NULL;
    ALLOCATOR_PROPERTIES allocProps;

    hr = IMemAllocator_GetProperties(This->pAlloc, &allocProps);

    This->cbAlign = allocProps.cbAlign;

    if (This->rtCurrent < This->rtStart)
        This->rtCurrent = MEDIATIME_FROM_BYTES(ALIGNDOWN(BYTES_FROM_MEDIATIME(This->rtStart), This->cbAlign));

    TRACE("Start\n");

    if (This->rtCurrent >= This->rtStop)
    {
        IPin_EndOfStream(&This->pin.IPin_iface);
        return;
    }

    /* There is no sample in our buffer */
    hr = This->fnCustomRequest(This->pUserData);

    if (FAILED(hr))
        ERR("Request error: %x\n", hr);

    EnterCriticalSection(This->pin.pCritSec);
    SetEvent(This->hEventStateChanged);
    LeaveCriticalSection(This->pin.pCritSec);

    if (SUCCEEDED(hr))
    do
    {
        DWORD_PTR dwUser;

        TRACE("Process sample\n");

        pSample = NULL;
        hr = IAsyncReader_WaitForNext(This->pReader, 10000, &pSample, &dwUser);

        /* Return an empty sample on error to the implementation in case it does custom parsing, so it knows it's gone */
        if (SUCCEEDED(hr))
        {
            hr = This->fnSampleProc(This->pUserData, pSample, dwUser);
        }
        else
        {
            if (hr == VFW_E_TIMEOUT)
            {
                if (pSample != NULL)
                    WARN("Non-NULL sample returned with VFW_E_TIMEOUT.\n");
                hr = S_OK;
            }
            /* FIXME: Errors are not well handled yet! */
            else
                ERR("Processing error: %x\n", hr);
        }

        if (pSample)
        {
            IMediaSample_Release(pSample);
            pSample = NULL;
        }
    } while (This->rtCurrent < This->rtStop && hr == S_OK && !This->stop_playback);

    /*
     * Sample was rejected, and we are asked to terminate.  When there is more than one buffer
     * it is possible for a filter to have several queued samples, making it necessary to
     * release all of these pending samples.
     */
    if (This->stop_playback || FAILED(hr))
    {
        DWORD_PTR dwUser;

        do
        {
            if (pSample)
                IMediaSample_Release(pSample);
            pSample = NULL;
            IAsyncReader_WaitForNext(This->pReader, 0, &pSample, &dwUser);
        } while(pSample);
    }

    /* Can't reset state to Sleepy here because that might race, instead PauseProcessing will do that for us
     * Flush remaining samples
     */
    if (This->fnDone)
        This->fnDone(This->pUserData);

    TRACE("End: %08x, %d\n", hr, This->stop_playback);
}

static void PullPin_Thread_Pause(PullPin *This)
{
    PullPin_Flush(This);

    EnterCriticalSection(This->pin.pCritSec);
    This->state = Req_Sleepy;
    SetEvent(This->hEventStateChanged);
    LeaveCriticalSection(This->pin.pCritSec);
}

static void  PullPin_Thread_Stop(PullPin *This)
{
    TRACE("(%p)->()\n", This);

    EnterCriticalSection(This->pin.pCritSec);
    {
        CloseHandle(This->hThread);
        This->hThread = NULL;
        SetEvent(This->hEventStateChanged);
    }
    LeaveCriticalSection(This->pin.pCritSec);

    IBaseFilter_Release(This->pin.pinInfo.pFilter);

    CoUninitialize();
    ExitThread(0);
}

static DWORD WINAPI PullPin_Thread_Main(LPVOID pv)
{
    PullPin *This = pv;
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    PullPin_Flush(This);

    for (;;)
    {
        WaitForSingleObject(This->thread_sleepy, INFINITE);

        TRACE("State: %d\n", This->state);

        switch (This->state)
        {
        case Req_Die: PullPin_Thread_Stop(This); break;
        case Req_Run: PullPin_Thread_Process(This); break;
        case Req_Pause: PullPin_Thread_Pause(This); break;
        case Req_Sleepy: ERR("Should not be signalled with SLEEPY!\n"); break;
        default: ERR("Unknown state request: %d\n", This->state); break;
        }
    }
    return 0;
}

static HRESULT PullPin_InitProcessing(PullPin * This)
{
    HRESULT hr = S_OK;

    TRACE("(%p)->()\n", This);

    /* if we are connected */
    if (This->pAlloc)
    {
        DWORD dwThreadId;

        WaitForSingleObject(This->hEventStateChanged, INFINITE);
        EnterCriticalSection(This->pin.pCritSec);

        assert(!This->hThread);
        assert(This->state == Req_Die);
        assert(This->stop_playback);
        assert(WaitForSingleObject(This->thread_sleepy, 0) == WAIT_TIMEOUT);
        This->state = Req_Sleepy;

        /* AddRef the filter to make sure it and its pins will be around
         * as long as the thread */
        IBaseFilter_AddRef(This->pin.pinInfo.pFilter);


        This->hThread = CreateThread(NULL, 0, PullPin_Thread_Main, This, 0, &dwThreadId);
        if (!This->hThread)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            IBaseFilter_Release(This->pin.pinInfo.pFilter);
        }

        if (SUCCEEDED(hr))
        {
            SetEvent(This->hEventStateChanged);
            /* If assert fails, that means a command was not processed before the thread previously terminated */
        }
        LeaveCriticalSection(This->pin.pCritSec);
    }

    TRACE(" -- %x\n", hr);

    return hr;
}

HRESULT PullPin_StartProcessing(PullPin * This)
{
    /* if we are connected */
    TRACE("(%p)->()\n", This);
    if(This->pAlloc)
    {
        assert(This->hThread);

        PullPin_WaitForStateChange(This, INFINITE);

        assert(This->state == Req_Sleepy);

        /* Wake up! */
        assert(WaitForSingleObject(This->thread_sleepy, 0) == WAIT_TIMEOUT);
        This->state = Req_Run;
        This->stop_playback = FALSE;
        ResetEvent(This->hEventStateChanged);
        SetEvent(This->thread_sleepy);
    }

    return S_OK;
}

HRESULT PullPin_PauseProcessing(PullPin * This)
{
    /* if we are connected */
    TRACE("(%p)->()\n", This);
    if(This->pAlloc)
    {
        assert(This->hThread);

        PullPin_WaitForStateChange(This, INFINITE);

        EnterCriticalSection(This->pin.pCritSec);

        assert(!This->stop_playback);
        assert(This->state == Req_Run|| This->state == Req_Sleepy);

        assert(WaitForSingleObject(This->thread_sleepy, 0) == WAIT_TIMEOUT);

        This->state = Req_Pause;
        This->stop_playback = TRUE;
        ResetEvent(This->hEventStateChanged);
        SetEvent(This->thread_sleepy);

        /* Release any outstanding samples */
        if (This->pReader)
        {
            IMediaSample *pSample;
            DWORD_PTR dwUser;

            do
            {
                pSample = NULL;
                IAsyncReader_WaitForNext(This->pReader, 0, &pSample, &dwUser);
                if (pSample)
                    IMediaSample_Release(pSample);
            } while(pSample);
        }

        LeaveCriticalSection(This->pin.pCritSec);
    }

    return S_OK;
}

static HRESULT PullPin_StopProcessing(PullPin * This)
{
    TRACE("(%p)->()\n", This);

    /* if we are alive */
    assert(This->hThread);

    PullPin_WaitForStateChange(This, INFINITE);

    assert(This->state == Req_Pause || This->state == Req_Sleepy);

    This->stop_playback = TRUE;
    This->state = Req_Die;
    assert(WaitForSingleObject(This->thread_sleepy, 0) == WAIT_TIMEOUT);
    ResetEvent(This->hEventStateChanged);
    SetEvent(This->thread_sleepy);
    return S_OK;
}

HRESULT PullPin_WaitForStateChange(PullPin * This, DWORD dwMilliseconds)
{
    if (WaitForSingleObject(This->hEventStateChanged, dwMilliseconds) == WAIT_TIMEOUT)
        return S_FALSE;
    return S_OK;
}

HRESULT WINAPI PullPin_QueryAccept(IPin * iface, const AM_MEDIA_TYPE * pmt)
{
    PullPin *This = impl_PullPin_from_IPin(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pmt);

    return (This->fnQueryAccept(This->pUserData, pmt) == S_OK ? S_OK : S_FALSE);
}

HRESULT WINAPI PullPin_EndOfStream(IPin * iface)
{
    PullPin *This = impl_PullPin_from_IPin(iface);
    HRESULT hr = S_FALSE;

    TRACE("(%p)->()\n", iface);

    EnterCriticalSection(This->pin.pCritSec);
    hr = SendFurther( iface, deliver_endofstream, NULL, NULL );
    SetEvent(This->hEventStateChanged);
    LeaveCriticalSection(This->pin.pCritSec);

    return hr;
}

HRESULT WINAPI PullPin_BeginFlush(IPin * iface)
{
    PullPin *This = impl_PullPin_from_IPin(iface);
    TRACE("(%p)->()\n", This);

    EnterCriticalSection(This->pin.pCritSec);
    {
        SendFurther( iface, deliver_beginflush, NULL, NULL );
    }
    LeaveCriticalSection(This->pin.pCritSec);

    EnterCriticalSection(&This->thread_lock);
    {
        if (This->pReader)
            IAsyncReader_BeginFlush(This->pReader);
        PullPin_WaitForStateChange(This, INFINITE);

        if (This->hThread && This->state == Req_Run)
        {
            PullPin_PauseProcessing(This);
            PullPin_WaitForStateChange(This, INFINITE);
        }
    }
    LeaveCriticalSection(&This->thread_lock);

    EnterCriticalSection(This->pin.pCritSec);
    {
        This->fnCleanProc(This->pUserData);
    }
    LeaveCriticalSection(This->pin.pCritSec);

    return S_OK;
}

HRESULT WINAPI PullPin_EndFlush(IPin * iface)
{
    PullPin *This = impl_PullPin_from_IPin(iface);

    TRACE("(%p)->()\n", iface);

    /* Send further first: Else a race condition might terminate processing early */
    EnterCriticalSection(This->pin.pCritSec);
    SendFurther( iface, deliver_endflush, NULL, NULL );
    LeaveCriticalSection(This->pin.pCritSec);

    EnterCriticalSection(&This->thread_lock);
    {
        FILTER_STATE state;

        if (This->pReader)
            IAsyncReader_EndFlush(This->pReader);

        IBaseFilter_GetState(This->pin.pinInfo.pFilter, INFINITE, &state);

        if (state != State_Stopped)
            PullPin_StartProcessing(This);

        PullPin_WaitForStateChange(This, INFINITE);
    }
    LeaveCriticalSection(&This->thread_lock);

    return S_OK;
}

HRESULT WINAPI PullPin_Disconnect(IPin *iface)
{
    HRESULT hr;
    PullPin *This = impl_PullPin_from_IPin(iface);

    TRACE("()\n");

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (FAILED(hr = IMemAllocator_Decommit(This->pAlloc)))
            ERR("Allocator decommit failed with error %x. Possible memory leak\n", hr);

        if (This->pin.pConnectedTo)
        {
            IPin_Release(This->pin.pConnectedTo);
            This->pin.pConnectedTo = NULL;
            PullPin_StopProcessing(This);

            FreeMediaType(&This->pin.mtCurrent);
            ZeroMemory(&This->pin.mtCurrent, sizeof(This->pin.mtCurrent));
            hr = S_OK;
        }
        else
            hr = S_FALSE;
    }
    LeaveCriticalSection(This->pin.pCritSec);

    return hr;
}

HRESULT WINAPI PullPin_NewSegment(IPin * iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    newsegmentargs args;
    FIXME("(%p)->(%s, %s, %g) stub\n", iface, wine_dbgstr_longlong(tStart), wine_dbgstr_longlong(tStop), dRate);

    args.tStart = tStart;
    args.tStop = tStop;
    args.rate = dRate;

    return SendFurther( iface, deliver_newsegment, &args, NULL );
}

static const IPinVtbl PullPin_Vtbl = 
{
    PullPin_QueryInterface,
    BasePinImpl_AddRef,
    PullPin_Release,
    BaseInputPinImpl_Connect,
    PullPin_ReceiveConnection,
    PullPin_Disconnect,
    BasePinImpl_ConnectedTo,
    BasePinImpl_ConnectionMediaType,
    BasePinImpl_QueryPinInfo,
    BasePinImpl_QueryDirection,
    BasePinImpl_QueryId,
    PullPin_QueryAccept,
    BasePinImpl_EnumMediaTypes,
    BasePinImpl_QueryInternalConnections,
    PullPin_EndOfStream,
    PullPin_BeginFlush,
    PullPin_EndFlush,
    PullPin_NewSegment
};
