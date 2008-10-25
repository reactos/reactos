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
#include "pin.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "uuids.h"
#include "vfwmsgs.h"
#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static const IPinVtbl InputPin_Vtbl;
static const IPinVtbl OutputPin_Vtbl;
static const IMemInputPinVtbl MemInputPin_Vtbl;
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
HRESULT updatehres( HRESULT original, HRESULT new )
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
     hr = S_OK;

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
    if (pin_info.pFilter)
        IBaseFilter_Release( pin_info.pFilter );
    return hr;
}

static inline InputPin *impl_from_IMemInputPin( IMemInputPin *iface )
{
    return (InputPin *)((char*)iface - FIELD_OFFSET(InputPin, lpVtblMemInput));
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

/* Function called as a helper to IPin_Connect */
/* specific AM_MEDIA_TYPE - it cannot be NULL */
/* NOTE: not part of standard interface */
static HRESULT OutputPin_ConnectSpecific(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    OutputPin *This = (OutputPin *)iface;
    HRESULT hr;
    IMemAllocator * pMemAlloc = NULL;
    ALLOCATOR_PROPERTIES actual; /* FIXME: should we put the actual props back in to This? */

    TRACE("(%p, %p)\n", pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    /* FIXME: call queryacceptproc */

    This->pin.pConnectedTo = pReceivePin;
    IPin_AddRef(pReceivePin);
    CopyMediaType(&This->pin.mtCurrent, pmt);

    hr = IPin_ReceiveConnection(pReceivePin, iface, pmt);

    /* get the IMemInputPin interface we will use to deliver samples to the
     * connected pin */
    if (SUCCEEDED(hr))
    {
        This->pMemInputPin = NULL;
        hr = IPin_QueryInterface(pReceivePin, &IID_IMemInputPin, (LPVOID)&This->pMemInputPin);

        if (SUCCEEDED(hr) && !This->custom_allocator)
        {
            hr = IMemInputPin_GetAllocator(This->pMemInputPin, &pMemAlloc);

            if (hr == VFW_E_NO_ALLOCATOR)
                /* Input pin provides no allocator, use standard memory allocator */
                hr = CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER, &IID_IMemAllocator, (LPVOID*)&pMemAlloc);

            if (SUCCEEDED(hr))
                hr = IMemAllocator_SetProperties(pMemAlloc, &This->allocProps, &actual);

            if (SUCCEEDED(hr))
                hr = IMemInputPin_NotifyAllocator(This->pMemInputPin, pMemAlloc, This->readonly);

            if (pMemAlloc)
                IMemAllocator_Release(pMemAlloc);
        }
        else if (SUCCEEDED(hr))
        {
            if (This->alloc)
            {
                hr = IMemInputPin_NotifyAllocator(This->pMemInputPin, This->alloc, This->readonly);
            }
            else
                hr = VFW_E_NO_ALLOCATOR;
        }

        /* break connection if we couldn't get the allocator */
        if (FAILED(hr))
        {
            if (This->pMemInputPin)
                IMemInputPin_Release(This->pMemInputPin);
            This->pMemInputPin = NULL;

            IPin_Disconnect(pReceivePin);
        }
    }

    if (FAILED(hr))
    {
        IPin_Release(This->pin.pConnectedTo);
        This->pin.pConnectedTo = NULL;
        FreeMediaType(&This->pin.mtCurrent);
    }

    TRACE(" -- %x\n", hr);
    return hr;
}

static HRESULT InputPin_Init(const IPinVtbl *InputPin_Vtbl, const PIN_INFO * pPinInfo, SAMPLEPROC_PUSH pSampleProc, LPVOID pUserData,
                             QUERYACCEPTPROC pQueryAccept, CLEANUPPROC pCleanUp, LPCRITICAL_SECTION pCritSec, IMemAllocator *allocator, InputPin * pPinImpl)
{
    TRACE("\n");

    /* Common attributes */
    pPinImpl->pin.refCount = 1;
    pPinImpl->pin.pConnectedTo = NULL;
    pPinImpl->pin.fnQueryAccept = pQueryAccept;
    pPinImpl->pin.pUserData = pUserData;
    pPinImpl->pin.pCritSec = pCritSec;
    Copy_PinInfo(&pPinImpl->pin.pinInfo, pPinInfo);
    ZeroMemory(&pPinImpl->pin.mtCurrent, sizeof(AM_MEDIA_TYPE));

    /* Input pin attributes */
    pPinImpl->fnSampleProc = pSampleProc;
    pPinImpl->fnCleanProc = pCleanUp;
    pPinImpl->pAllocator = pPinImpl->preferred_allocator = allocator;
    if (pPinImpl->preferred_allocator)
        IMemAllocator_AddRef(pPinImpl->preferred_allocator);
    pPinImpl->tStart = 0;
    pPinImpl->tStop = 0;
    pPinImpl->dRate = 1.0;
    pPinImpl->pin.lpVtbl = InputPin_Vtbl;
    pPinImpl->lpVtblMemInput = &MemInputPin_Vtbl;
    pPinImpl->flushing = pPinImpl->end_of_stream = 0;

    return S_OK;
}

static HRESULT OutputPin_Init(const IPinVtbl *OutputPin_Vtbl, const PIN_INFO * pPinInfo, const ALLOCATOR_PROPERTIES * props, LPVOID pUserData,
                              QUERYACCEPTPROC pQueryAccept, LPCRITICAL_SECTION pCritSec, OutputPin * pPinImpl)
{
    TRACE("\n");

    /* Common attributes */
    pPinImpl->pin.lpVtbl = OutputPin_Vtbl;
    pPinImpl->pin.refCount = 1;
    pPinImpl->pin.pConnectedTo = NULL;
    pPinImpl->pin.fnQueryAccept = pQueryAccept;
    pPinImpl->pin.pUserData = pUserData;
    pPinImpl->pin.pCritSec = pCritSec;
    Copy_PinInfo(&pPinImpl->pin.pinInfo, pPinInfo);
    ZeroMemory(&pPinImpl->pin.mtCurrent, sizeof(AM_MEDIA_TYPE));

    /* Output pin attributes */
    pPinImpl->pMemInputPin = NULL;
    pPinImpl->pConnectSpecific = OutputPin_ConnectSpecific;
    /* If custom_allocator is set, you will need to specify an allocator
     * in the alloc member of the struct before an output pin can connect
     */
    pPinImpl->custom_allocator = 0;
    pPinImpl->alloc = NULL;
    pPinImpl->readonly = FALSE;
    if (props)
    {
        pPinImpl->allocProps = *props;
        if (pPinImpl->allocProps.cbAlign == 0)
            pPinImpl->allocProps.cbAlign = 1;
    }
    else
        ZeroMemory(&pPinImpl->allocProps, sizeof(pPinImpl->allocProps));

    return S_OK;
}

HRESULT InputPin_Construct(const IPinVtbl *InputPin_Vtbl, const PIN_INFO * pPinInfo, SAMPLEPROC_PUSH pSampleProc, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept, CLEANUPPROC pCleanUp, LPCRITICAL_SECTION pCritSec, IMemAllocator *allocator, IPin ** ppPin)
{
    InputPin * pPinImpl;

    *ppPin = NULL;

    if (pPinInfo->dir != PINDIR_INPUT)
    {
        ERR("Pin direction(%x) != PINDIR_INPUT\n", pPinInfo->dir);
        return E_INVALIDARG;
    }

    pPinImpl = CoTaskMemAlloc(sizeof(*pPinImpl));

    if (!pPinImpl)
        return E_OUTOFMEMORY;

    if (SUCCEEDED(InputPin_Init(InputPin_Vtbl, pPinInfo, pSampleProc, pUserData, pQueryAccept, pCleanUp, pCritSec, allocator, pPinImpl)))
    {
        *ppPin = (IPin *)pPinImpl;
        return S_OK;
    }

    CoTaskMemFree(pPinImpl);
    return E_FAIL;
}

HRESULT OutputPin_Construct(const IPinVtbl *OutputPin_Vtbl, long outputpin_size, const PIN_INFO * pPinInfo, ALLOCATOR_PROPERTIES *props, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept, LPCRITICAL_SECTION pCritSec, IPin ** ppPin)
{
    OutputPin * pPinImpl;

    *ppPin = NULL;

    if (pPinInfo->dir != PINDIR_OUTPUT)
    {
        ERR("Pin direction(%x) != PINDIR_OUTPUT\n", pPinInfo->dir);
        return E_INVALIDARG;
    }

    assert(outputpin_size >= sizeof(OutputPin));

    pPinImpl = CoTaskMemAlloc(outputpin_size);

    if (!pPinImpl)
        return E_OUTOFMEMORY;

    if (SUCCEEDED(OutputPin_Init(OutputPin_Vtbl, pPinInfo, props, pUserData, pQueryAccept, pCritSec, pPinImpl)))
    {
        *ppPin = (IPin *)(&pPinImpl->pin.lpVtbl);
        return S_OK;
    }

    CoTaskMemFree(pPinImpl);
    return E_FAIL;
}

/*** Common pin functions ***/

ULONG WINAPI IPinImpl_AddRef(IPin * iface)
{
    IPinImpl *This = (IPinImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->refCount);
    
    TRACE("(%p)->() AddRef from %d\n", iface, refCount - 1);
    
    return refCount;
}

HRESULT WINAPI IPinImpl_Disconnect(IPin * iface)
{
    HRESULT hr;
    IPinImpl *This = (IPinImpl *)iface;

    TRACE("()\n");

    EnterCriticalSection(This->pCritSec);
    {
        if (This->pConnectedTo)
        {
            IPin_Release(This->pConnectedTo);
            This->pConnectedTo = NULL;
            FreeMediaType(&This->mtCurrent);
            ZeroMemory(&This->mtCurrent, sizeof(This->mtCurrent));
            hr = S_OK;
        }
        else
            hr = S_FALSE;
    }
    LeaveCriticalSection(This->pCritSec);
    
    return hr;
}

HRESULT WINAPI IPinImpl_ConnectedTo(IPin * iface, IPin ** ppPin)
{
    HRESULT hr;
    IPinImpl *This = (IPinImpl *)iface;

    TRACE("(%p)\n", ppPin);

    EnterCriticalSection(This->pCritSec);
    {
        if (This->pConnectedTo)
        {
            *ppPin = This->pConnectedTo;
            IPin_AddRef(*ppPin);
            hr = S_OK;
        }
        else
        {
            hr = VFW_E_NOT_CONNECTED;
            *ppPin = NULL;
        }
    }
    LeaveCriticalSection(This->pCritSec);

    return hr;
}

HRESULT WINAPI IPinImpl_ConnectionMediaType(IPin * iface, AM_MEDIA_TYPE * pmt)
{
    HRESULT hr;
    IPinImpl *This = (IPinImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pmt);

    EnterCriticalSection(This->pCritSec);
    {
        if (This->pConnectedTo)
        {
            CopyMediaType(pmt, &This->mtCurrent);
            hr = S_OK;
        }
        else
        {
            ZeroMemory(pmt, sizeof(*pmt));
            hr = VFW_E_NOT_CONNECTED;
        }
    }
    LeaveCriticalSection(This->pCritSec);

    return hr;
}

HRESULT WINAPI IPinImpl_QueryPinInfo(IPin * iface, PIN_INFO * pInfo)
{
    IPinImpl *This = (IPinImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pInfo);

    Copy_PinInfo(pInfo, &This->pinInfo);
    IBaseFilter_AddRef(pInfo->pFilter);

    return S_OK;
}

HRESULT WINAPI IPinImpl_QueryDirection(IPin * iface, PIN_DIRECTION * pPinDir)
{
    IPinImpl *This = (IPinImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pPinDir);

    *pPinDir = This->pinInfo.dir;

    return S_OK;
}

HRESULT WINAPI IPinImpl_QueryId(IPin * iface, LPWSTR * Id)
{
    IPinImpl *This = (IPinImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, Id);

    *Id = CoTaskMemAlloc((strlenW(This->pinInfo.achName) + 1) * sizeof(WCHAR));
    if (!*Id)
        return E_OUTOFMEMORY;

    strcpyW(*Id, This->pinInfo.achName);

    return S_OK;
}

HRESULT WINAPI IPinImpl_QueryAccept(IPin * iface, const AM_MEDIA_TYPE * pmt)
{
    IPinImpl *This = (IPinImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pmt);

    return (This->fnQueryAccept(This->pUserData, pmt) == S_OK ? S_OK : S_FALSE);
}

HRESULT WINAPI IPinImpl_EnumMediaTypes(IPin * iface, IEnumMediaTypes ** ppEnum)
{
    IPinImpl *This = (IPinImpl *)iface;
    ENUMMEDIADETAILS emd;

    TRACE("(%p/%p)->(%p)\n", This, iface, ppEnum);

    /* override this method to allow enumeration of your types */
    emd.cMediaTypes = 0;
    emd.pMediaTypes = NULL;

    return IEnumMediaTypesImpl_Construct(&emd, ppEnum);
}

HRESULT WINAPI IPinImpl_QueryInternalConnections(IPin * iface, IPin ** apPin, ULONG * cPin)
{
    IPinImpl *This = (IPinImpl *)iface;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, apPin, cPin);

    return E_NOTIMPL; /* to tell caller that all input pins connected to all output pins */
}

/*** IPin implementation for an input pin ***/

HRESULT WINAPI InputPin_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv)
{
    InputPin *This = (InputPin *)iface;

    TRACE("(%p)->(%s, %p)\n", iface, qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = (LPVOID)iface;
    else if (IsEqualIID(riid, &IID_IPin))
        *ppv = (LPVOID)iface;
    else if (IsEqualIID(riid, &IID_IMemInputPin))
        *ppv = (LPVOID)&This->lpVtblMemInput;
    else if (IsEqualIID(riid, &IID_IMediaSeeking))
    {
        return IBaseFilter_QueryInterface(This->pin.pinInfo.pFilter, &IID_IMediaSeeking, ppv);
    }

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

ULONG WINAPI InputPin_Release(IPin * iface)
{
    InputPin *This = (InputPin *)iface;
    ULONG refCount = InterlockedDecrement(&This->pin.refCount);
    
    TRACE("(%p)->() Release from %d\n", iface, refCount + 1);
    
    if (!refCount)
    {
        FreeMediaType(&This->pin.mtCurrent);
        if (This->pAllocator)
            IMemAllocator_Release(This->pAllocator);
        This->pAllocator = NULL;
        This->pin.lpVtbl = NULL;
        CoTaskMemFree(This);
        return 0;
    }
    else
        return refCount;
}

HRESULT WINAPI InputPin_Connect(IPin * iface, IPin * pConnector, const AM_MEDIA_TYPE * pmt)
{
    ERR("Outgoing connection on an input pin! (%p, %p)\n", pConnector, pmt);

    return E_UNEXPECTED;
}


HRESULT WINAPI InputPin_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    InputPin *This = (InputPin *)iface;
    PIN_DIRECTION pindirReceive;
    HRESULT hr = S_OK;

    TRACE("(%p, %p)\n", pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (This->pin.pConnectedTo)
            hr = VFW_E_ALREADY_CONNECTED;

        if (SUCCEEDED(hr) && This->pin.fnQueryAccept(This->pin.pUserData, pmt) != S_OK)
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

        if (SUCCEEDED(hr))
        {
            CopyMediaType(&This->pin.mtCurrent, pmt);
            This->pin.pConnectedTo = pReceivePin;
            IPin_AddRef(pReceivePin);
        }
    }
    LeaveCriticalSection(This->pin.pCritSec);

    return hr;
}

static HRESULT deliver_endofstream(IPin* pin, LPVOID unused)
{
    return IPin_EndOfStream( pin );
}

HRESULT WINAPI InputPin_EndOfStream(IPin * iface)
{
    HRESULT hr = S_OK;
    InputPin *This = (InputPin *)iface;

    TRACE("(%p)\n", This);

    EnterCriticalSection(This->pin.pCritSec);
    if (This->flushing)
        hr = S_FALSE;
    else
        This->end_of_stream = 1;
    LeaveCriticalSection(This->pin.pCritSec);

    if (hr == S_OK)
        hr = SendFurther( iface, deliver_endofstream, NULL, NULL );
    return hr;
}

static HRESULT deliver_beginflush(IPin* pin, LPVOID unused)
{
    return IPin_BeginFlush( pin );
}

HRESULT WINAPI InputPin_BeginFlush(IPin * iface)
{
    InputPin *This = (InputPin *)iface;
    HRESULT hr;
    TRACE("() semi-stub\n");

    EnterCriticalSection(This->pin.pCritSec);
    This->flushing = 1;

    if (This->fnCleanProc)
        This->fnCleanProc(This->pin.pUserData);

    hr = SendFurther( iface, deliver_beginflush, NULL, NULL );
    LeaveCriticalSection(This->pin.pCritSec);

    return hr;
}

static HRESULT deliver_endflush(IPin* pin, LPVOID unused)
{
    return IPin_EndFlush( pin );
}

HRESULT WINAPI InputPin_EndFlush(IPin * iface)
{
    InputPin *This = (InputPin *)iface;
    HRESULT hr;
    TRACE("(%p)\n", This);

    EnterCriticalSection(This->pin.pCritSec);
    This->flushing = This->end_of_stream = 0;

    hr = SendFurther( iface, deliver_endflush, NULL, NULL );
    LeaveCriticalSection(This->pin.pCritSec);

    return hr;
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

HRESULT WINAPI InputPin_NewSegment(IPin * iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    InputPin *This = (InputPin *)iface;
    newsegmentargs args;

    TRACE("(%x%08x, %x%08x, %e)\n", (ULONG)(tStart >> 32), (ULONG)tStart, (ULONG)(tStop >> 32), (ULONG)tStop, dRate);

    args.tStart = This->tStart = tStart;
    args.tStop = This->tStop = tStop;
    args.rate = This->dRate = dRate;

    return SendFurther( iface, deliver_newsegment, &args, NULL );
}

static const IPinVtbl InputPin_Vtbl = 
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
    InputPin_EndOfStream,
    InputPin_BeginFlush,
    InputPin_EndFlush,
    InputPin_NewSegment
};

/*** IMemInputPin implementation ***/

HRESULT WINAPI MemInputPin_QueryInterface(IMemInputPin * iface, REFIID riid, LPVOID * ppv)
{
    InputPin *This = impl_from_IMemInputPin(iface);

    return IPin_QueryInterface((IPin *)&This->pin, riid, ppv);
}

ULONG WINAPI MemInputPin_AddRef(IMemInputPin * iface)
{
    InputPin *This = impl_from_IMemInputPin(iface);

    return IPin_AddRef((IPin *)&This->pin);
}

ULONG WINAPI MemInputPin_Release(IMemInputPin * iface)
{
    InputPin *This = impl_from_IMemInputPin(iface);

    return IPin_Release((IPin *)&This->pin);
}

HRESULT WINAPI MemInputPin_GetAllocator(IMemInputPin * iface, IMemAllocator ** ppAllocator)
{
    InputPin *This = impl_from_IMemInputPin(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, ppAllocator);

    *ppAllocator = This->pAllocator;
    if (*ppAllocator)
        IMemAllocator_AddRef(*ppAllocator);
    
    return *ppAllocator ? S_OK : VFW_E_NO_ALLOCATOR;
}

HRESULT WINAPI MemInputPin_NotifyAllocator(IMemInputPin * iface, IMemAllocator * pAllocator, BOOL bReadOnly)
{
    InputPin *This = impl_from_IMemInputPin(iface);

    TRACE("(%p/%p)->(%p, %d)\n", This, iface, pAllocator, bReadOnly);

    if (bReadOnly)
        FIXME("Read only flag not handled yet!\n");

    /* FIXME: Should we release the allocator on disconnection? */
    if (!pAllocator)
    {
        WARN("Null allocator\n");
        return E_POINTER;
    }

    if (This->preferred_allocator && pAllocator != This->preferred_allocator)
        return E_FAIL;

    if (This->pAllocator)
        IMemAllocator_Release(This->pAllocator);
    This->pAllocator = pAllocator;
    if (This->pAllocator)
        IMemAllocator_AddRef(This->pAllocator);

    return S_OK;
}

HRESULT WINAPI MemInputPin_GetAllocatorRequirements(IMemInputPin * iface, ALLOCATOR_PROPERTIES * pProps)
{
    InputPin *This = impl_from_IMemInputPin(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pProps);

    /* override this method if you have any specific requirements */

    return E_NOTIMPL;
}

HRESULT WINAPI MemInputPin_Receive(IMemInputPin * iface, IMediaSample * pSample)
{
    InputPin *This = impl_from_IMemInputPin(iface);
    HRESULT hr;

    /* this trace commented out for performance reasons */
    /*TRACE("(%p/%p)->(%p)\n", This, iface, pSample);*/
    hr = This->fnSampleProc(This->pin.pUserData, pSample);
    return hr;
}

HRESULT WINAPI MemInputPin_ReceiveMultiple(IMemInputPin * iface, IMediaSample ** pSamples, long nSamples, long *nSamplesProcessed)
{
    HRESULT hr = S_OK;
    InputPin *This = impl_from_IMemInputPin(iface);

    TRACE("(%p/%p)->(%p, %ld, %p)\n", This, iface, pSamples, nSamples, nSamplesProcessed);

    for (*nSamplesProcessed = 0; *nSamplesProcessed < nSamples; (*nSamplesProcessed)++)
    {
        hr = IMemInputPin_Receive(iface, pSamples[*nSamplesProcessed]);
        if (hr != S_OK)
            break;
    }

    return hr;
}

HRESULT WINAPI MemInputPin_ReceiveCanBlock(IMemInputPin * iface)
{
    InputPin *This = impl_from_IMemInputPin(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return S_OK;
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

HRESULT WINAPI OutputPin_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv)
{
    OutputPin *This = (OutputPin *)iface;

    TRACE("(%p/%p)->(%s, %p)\n", This, iface, qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = (LPVOID)iface;
    else if (IsEqualIID(riid, &IID_IPin))
        *ppv = (LPVOID)iface;
    else if (IsEqualIID(riid, &IID_IMediaSeeking))
    {
        return IBaseFilter_QueryInterface(This->pin.pinInfo.pFilter, &IID_IMediaSeeking, ppv);
    }

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

ULONG WINAPI OutputPin_Release(IPin * iface)
{
    OutputPin *This = (OutputPin *)iface;
    ULONG refCount = InterlockedDecrement(&This->pin.refCount);

    TRACE("(%p)->() Release from %d\n", iface, refCount + 1);

    if (!refCount)
    {
        FreeMediaType(&This->pin.mtCurrent);
        CoTaskMemFree(This);
        return 0;
    }
    return refCount;
}

HRESULT WINAPI OutputPin_Connect(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    HRESULT hr;
    OutputPin *This = (OutputPin *)iface;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    /* If we try to connect to ourself, we will definitely deadlock.
     * There are other cases where we could deadlock too, but this
     * catches the obvious case */
    assert(pReceivePin != iface);

    EnterCriticalSection(This->pin.pCritSec);
    {
        /* if we have been a specific type to connect with, then we can either connect
         * with that or fail. We cannot choose different AM_MEDIA_TYPE */
        if (pmt && !IsEqualGUID(&pmt->majortype, &GUID_NULL) && !IsEqualGUID(&pmt->subtype, &GUID_NULL))
            hr = This->pConnectSpecific(iface, pReceivePin, pmt);
        else
        {
            /* negotiate media type */

            IEnumMediaTypes * pEnumCandidates;
            AM_MEDIA_TYPE * pmtCandidate = NULL; /* Candidate media type */

            if (SUCCEEDED(hr = IPin_EnumMediaTypes(iface, &pEnumCandidates)))
            {
                hr = VFW_E_NO_ACCEPTABLE_TYPES; /* Assume the worst, but set to S_OK if connected successfully */

                /* try this filter's media types first */
                while (S_OK == IEnumMediaTypes_Next(pEnumCandidates, 1, &pmtCandidate, NULL))
                {
                    assert(pmtCandidate);
                    dump_AM_MEDIA_TYPE(pmtCandidate);
                    if (!IsEqualGUID(&FORMAT_None, &pmtCandidate->formattype)
                        && !IsEqualGUID(&GUID_NULL, &pmtCandidate->formattype))
                        assert(pmtCandidate->pbFormat);
                    if (( !pmt || CompareMediaTypes(pmt, pmtCandidate, TRUE) ) && 
                        (This->pConnectSpecific(iface, pReceivePin, pmtCandidate) == S_OK))
                    {
                        hr = S_OK;
                        DeleteMediaType(pmtCandidate);
                        break;
                    }
                    DeleteMediaType(pmtCandidate);
                    pmtCandidate = NULL;
                }
                IEnumMediaTypes_Release(pEnumCandidates);
            }

            /* then try receiver filter's media types */
            if (hr != S_OK && SUCCEEDED(hr = IPin_EnumMediaTypes(pReceivePin, &pEnumCandidates))) /* if we haven't already connected successfully */
            {
                hr = VFW_E_NO_ACCEPTABLE_TYPES; /* Assume the worst, but set to S_OK if connected successfully */

                while (S_OK == IEnumMediaTypes_Next(pEnumCandidates, 1, &pmtCandidate, NULL))
                {
                    assert(pmtCandidate);
                    dump_AM_MEDIA_TYPE(pmtCandidate);
                    if (!IsEqualGUID(&FORMAT_None, &pmtCandidate->formattype)
                        && !IsEqualGUID(&GUID_NULL, &pmtCandidate->formattype))
                        assert(pmtCandidate->pbFormat);
                    if (( !pmt || CompareMediaTypes(pmt, pmtCandidate, TRUE) ) && 
                        (This->pConnectSpecific(iface, pReceivePin, pmtCandidate) == S_OK))
                    {
                        hr = S_OK;
                        DeleteMediaType(pmtCandidate);
                        break;
                    }
                    DeleteMediaType(pmtCandidate);
                    pmtCandidate = NULL;
                } /* while */
                IEnumMediaTypes_Release(pEnumCandidates);
            } /* if not found */
        } /* if negotiate media type */
    } /* if succeeded */
    LeaveCriticalSection(This->pin.pCritSec);

    TRACE(" -- %x\n", hr);
    return hr;
}

HRESULT WINAPI OutputPin_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    ERR("Incoming connection on an output pin! (%p, %p)\n", pReceivePin, pmt);

    return E_UNEXPECTED;
}

HRESULT WINAPI OutputPin_Disconnect(IPin * iface)
{
    HRESULT hr;
    OutputPin *This = (OutputPin *)iface;

    TRACE("()\n");

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (This->pMemInputPin)
        {
            IMemInputPin_Release(This->pMemInputPin);
            This->pMemInputPin = NULL;
        }
        if (This->pin.pConnectedTo)
        {
            IPin_Release(This->pin.pConnectedTo);
            This->pin.pConnectedTo = NULL;
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

HRESULT WINAPI OutputPin_EndOfStream(IPin * iface)
{
    TRACE("()\n");

    /* not supposed to do anything in an output pin */

    return E_UNEXPECTED;
}

HRESULT WINAPI OutputPin_BeginFlush(IPin * iface)
{
    TRACE("(%p)->()\n", iface);

    /* not supposed to do anything in an output pin */

    return E_UNEXPECTED;
}

HRESULT WINAPI OutputPin_EndFlush(IPin * iface)
{
    TRACE("(%p)->()\n", iface);

    /* not supposed to do anything in an output pin */

    return E_UNEXPECTED;
}

HRESULT WINAPI OutputPin_NewSegment(IPin * iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    TRACE("(%p)->(%x%08x, %x%08x, %e)\n", iface, (ULONG)(tStart >> 32), (ULONG)tStart, (ULONG)(tStop >> 32), (ULONG)tStop, dRate);

    /* not supposed to do anything in an output pin */

    return E_UNEXPECTED;
}

static const IPinVtbl OutputPin_Vtbl = 
{
    OutputPin_QueryInterface,
    IPinImpl_AddRef,
    OutputPin_Release,
    OutputPin_Connect,
    OutputPin_ReceiveConnection,
    OutputPin_Disconnect,
    IPinImpl_ConnectedTo,
    IPinImpl_ConnectionMediaType,
    IPinImpl_QueryPinInfo,
    IPinImpl_QueryDirection,
    IPinImpl_QueryId,
    IPinImpl_QueryAccept,
    IPinImpl_EnumMediaTypes,
    IPinImpl_QueryInternalConnections,
    OutputPin_EndOfStream,
    OutputPin_BeginFlush,
    OutputPin_EndFlush,
    OutputPin_NewSegment
};

HRESULT OutputPin_GetDeliveryBuffer(OutputPin * This, IMediaSample ** ppSample, REFERENCE_TIME * tStart, REFERENCE_TIME * tStop, DWORD dwFlags)
{
    HRESULT hr;

    TRACE("(%p, %p, %p, %x)\n", ppSample, tStart, tStop, dwFlags);

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (!This->pin.pConnectedTo)
            hr = VFW_E_NOT_CONNECTED;
        else
        {
            IMemAllocator * pAlloc = NULL;
            
            hr = IMemInputPin_GetAllocator(This->pMemInputPin, &pAlloc);

            if (SUCCEEDED(hr))
                hr = IMemAllocator_GetBuffer(pAlloc, ppSample, tStart, tStop, dwFlags);

            if (SUCCEEDED(hr))
                hr = IMediaSample_SetTime(*ppSample, tStart, tStop);

            if (pAlloc)
                IMemAllocator_Release(pAlloc);
        }
    }
    LeaveCriticalSection(This->pin.pCritSec);
    
    return hr;
}

HRESULT OutputPin_SendSample(OutputPin * This, IMediaSample * pSample)
{
    HRESULT hr = S_OK;
    IMemInputPin * pMemConnected = NULL;
    PIN_INFO pinInfo;

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (!This->pin.pConnectedTo || !This->pMemInputPin)
            hr = VFW_E_NOT_CONNECTED;
        else
        {
            /* we don't have the lock held when using This->pMemInputPin,
             * so we need to AddRef it to stop it being deleted while we are
             * using it. Same with its filter. */
            pMemConnected = This->pMemInputPin;
            IMemInputPin_AddRef(pMemConnected);
            hr = IPin_QueryPinInfo(This->pin.pConnectedTo, &pinInfo);
        }
    }
    LeaveCriticalSection(This->pin.pCritSec);

    if (SUCCEEDED(hr))
    {
        /* NOTE: if we are in a critical section when Receive is called
         * then it causes some problems (most notably with the native Video
         * Renderer) if we are re-entered for whatever reason */
        hr = IMemInputPin_Receive(pMemConnected, pSample);

        /* If the filter's destroyed, tell upstream to stop sending data */
        if(IBaseFilter_Release(pinInfo.pFilter) == 0 && SUCCEEDED(hr))
            hr = S_FALSE;
    }
    if (pMemConnected)
        IMemInputPin_Release(pMemConnected);

    return hr;
}

HRESULT OutputPin_DeliverNewSegment(OutputPin * This, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    HRESULT hr;

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (!This->pin.pConnectedTo)
            hr = VFW_E_NOT_CONNECTED;
        else
            hr = IPin_NewSegment(This->pin.pConnectedTo, tStart, tStop, dRate);
    }
    LeaveCriticalSection(This->pin.pCritSec);
    
    return hr;
}

HRESULT OutputPin_CommitAllocator(OutputPin * This)
{
    HRESULT hr = S_OK;

    TRACE("(%p)->()\n", This);

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (!This->pin.pConnectedTo || !This->pMemInputPin)
            hr = VFW_E_NOT_CONNECTED;
        else
        {
            IMemAllocator * pAlloc = NULL;

            hr = IMemInputPin_GetAllocator(This->pMemInputPin, &pAlloc);

            if (SUCCEEDED(hr))
                hr = IMemAllocator_Commit(pAlloc);

            if (pAlloc)
                IMemAllocator_Release(pAlloc);
        }
    }
    LeaveCriticalSection(This->pin.pCritSec);

    TRACE("--> %08x\n", hr);
    return hr;
}

HRESULT OutputPin_DecommitAllocator(OutputPin * This)
{
    HRESULT hr = S_OK;

    TRACE("(%p)->()\n", This);

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (!This->pin.pConnectedTo || !This->pMemInputPin)
            hr = VFW_E_NOT_CONNECTED;
        else
        {
            IMemAllocator * pAlloc = NULL;

            hr = IMemInputPin_GetAllocator(This->pMemInputPin, &pAlloc);

            if (SUCCEEDED(hr))
                hr = IMemAllocator_Decommit(pAlloc);

            if (pAlloc)
                IMemAllocator_Release(pAlloc);
        }
    }
    LeaveCriticalSection(This->pin.pCritSec);

    TRACE("--> %08x\n", hr);
    return hr;
}

HRESULT OutputPin_DeliverDisconnect(OutputPin * This)
{
    HRESULT hr;

    TRACE("(%p)->()\n", This);

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (!This->pin.pConnectedTo || !This->pMemInputPin)
            hr = VFW_E_NOT_CONNECTED;
        else if (!This->custom_allocator)
        {
            IMemAllocator * pAlloc = NULL;

            hr = IMemInputPin_GetAllocator(This->pMemInputPin, &pAlloc);

            if (SUCCEEDED(hr))
                hr = IMemAllocator_Decommit(pAlloc);

            if (pAlloc)
                IMemAllocator_Release(pAlloc);

            if (SUCCEEDED(hr))
                hr = IPin_Disconnect(This->pin.pConnectedTo);
        }
        else /* Kill the allocator! */
        {
            hr = IPin_Disconnect(This->pin.pConnectedTo);
        }
        IPin_Disconnect((IPin *)This);
    }
    LeaveCriticalSection(This->pin.pCritSec);

    return hr;
}


static HRESULT PullPin_Init(const IPinVtbl *PullPin_Vtbl, const PIN_INFO * pPinInfo, SAMPLEPROC_PULL pSampleProc, LPVOID pUserData,
                            QUERYACCEPTPROC pQueryAccept, CLEANUPPROC pCleanUp, REQUESTPROC pCustomRequest, STOPPROCESSPROC pDone, LPCRITICAL_SECTION pCritSec, PullPin * pPinImpl)
{
    /* Common attributes */
    pPinImpl->pin.lpVtbl = PullPin_Vtbl;
    pPinImpl->pin.refCount = 1;
    pPinImpl->pin.pConnectedTo = NULL;
    pPinImpl->pin.fnQueryAccept = pQueryAccept;
    pPinImpl->pin.pUserData = pUserData;
    pPinImpl->pin.pCritSec = pCritSec;
    Copy_PinInfo(&pPinImpl->pin.pinInfo, pPinInfo);
    ZeroMemory(&pPinImpl->pin.mtCurrent, sizeof(AM_MEDIA_TYPE));

    /* Input pin attributes */
    pPinImpl->fnSampleProc = pSampleProc;
    pPinImpl->fnCleanProc = pCleanUp;
    pPinImpl->fnDone = pDone;
    pPinImpl->fnPreConnect = NULL;
    pPinImpl->pAlloc = NULL;
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
    pPinImpl->stop_playback = 1;

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
        *ppPin = (IPin *)(&pPinImpl->pin.lpVtbl);
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
    PullPin *This = (PullPin *)iface;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    EnterCriticalSection(This->pin.pCritSec);
    if (!This->pin.pConnectedTo)
    {
        ALLOCATOR_PROPERTIES props;

        props.cBuffers = 3;
        props.cbBuffer = 64 * 1024; /* 64k bytes */
        props.cbAlign = 1;
        props.cbPrefix = 0;

        if (SUCCEEDED(hr) && (This->pin.fnQueryAccept(This->pin.pUserData, pmt) != S_OK))
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
        if (SUCCEEDED(hr))
        {
            hr = IPin_QueryInterface(pReceivePin, &IID_IAsyncReader, (LPVOID *)&This->pReader);
        }

        if (SUCCEEDED(hr) && This->fnPreConnect)
        {
            hr = This->fnPreConnect(iface, pReceivePin, &props);
        }

        if (SUCCEEDED(hr))
        {
            hr = IAsyncReader_RequestAllocator(This->pReader, NULL, &props, &This->pAlloc);
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
    PullPin *This = (PullPin *)iface;

    TRACE("(%p/%p)->(%s, %p)\n", This, iface, qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = (LPVOID)iface;
    else if (IsEqualIID(riid, &IID_IPin))
        *ppv = (LPVOID)iface;
    else if (IsEqualIID(riid, &IID_IMediaSeeking))
    {
        return IBaseFilter_QueryInterface(This->pin.pinInfo.pFilter, &IID_IMediaSeeking, ppv);
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
    PullPin *This = (PullPin *)iface;
    ULONG refCount = InterlockedDecrement(&This->pin.refCount);

    TRACE("(%p)->() Release from %d\n", This, refCount + 1);

    if (!refCount)
    {
        WaitForSingleObject(This->hEventStateChanged, INFINITE);
        assert(!This->hThread);

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

static void CALLBACK PullPin_Flush(PullPin *This)
{
    IMediaSample *pSample;
    TRACE("Flushing!\n");

    if (This->pReader)
    {
        /* Flush outstanding samples */
        IAsyncReader_BeginFlush(This->pReader);

        for (;;)
        {
            DWORD_PTR dwUser;

            IAsyncReader_WaitForNext(This->pReader, 0, &pSample, &dwUser);

            if (!pSample)
                break;

            assert(!IMediaSample_GetActualDataLength(pSample));

            IMediaSample_Release(pSample);
        }

        IAsyncReader_EndFlush(This->pReader);
    }
}

static void CALLBACK PullPin_Thread_Process(PullPin *This)
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
        IPin_EndOfStream((IPin *)This);
        return;
    }

    /* There is no sample in our buffer */
    hr = This->fnCustomRequest(This->pin.pUserData);

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

        hr = IAsyncReader_WaitForNext(This->pReader, 10000, &pSample, &dwUser);

        /* Return an empty sample on error to the implementation in case it does custom parsing, so it knows it's gone */
        if (SUCCEEDED(hr))
        {
            hr = This->fnSampleProc(This->pin.pUserData, pSample, dwUser);
        }
        else
        {
            /* FIXME: This is not well handled yet! */
            ERR("Processing error: %x\n", hr);
        }

        if (pSample)
        {
            IMediaSample_Release(pSample);
            pSample = NULL;
        }
    } while (This->rtCurrent < This->rtStop && hr == S_OK && !This->stop_playback);

    /* Sample was rejected, and we are asked to terminate */
    if (pSample)
    {
        IMediaSample_Release(pSample);
    }

    /* Can't reset state to Sleepy here because that might race, instead PauseProcessing will do that for us
     * Flush remaining samples
     */
    if (This->fnDone)
        This->fnDone(This->pin.pUserData);

    TRACE("End: %08x, %d\n", hr, This->stop_playback);
}

static void CALLBACK PullPin_Thread_Pause(PullPin *This)
{
    PullPin_Flush(This);

    EnterCriticalSection(This->pin.pCritSec);
    This->state = Req_Sleepy;
    SetEvent(This->hEventStateChanged);
    LeaveCriticalSection(This->pin.pCritSec);
}

static void CALLBACK PullPin_Thread_Stop(PullPin *This)
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

        /* AddRef the filter to make sure it and it's pins will be around
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
        This->stop_playback = 0;
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
        This->stop_playback = 1;
        ResetEvent(This->hEventStateChanged);
        SetEvent(This->thread_sleepy);

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

    This->stop_playback = 1;
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

HRESULT WINAPI PullPin_EndOfStream(IPin * iface)
{
    FIXME("(%p)->() stub\n", iface);

    return SendFurther( iface, deliver_endofstream, NULL, NULL );
}

HRESULT WINAPI PullPin_BeginFlush(IPin * iface)
{
    PullPin *This = (PullPin *)iface;
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
        This->fnCleanProc(This->pin.pUserData);
    }
    LeaveCriticalSection(This->pin.pCritSec);

    return S_OK;
}

HRESULT WINAPI PullPin_EndFlush(IPin * iface)
{
    PullPin *This = (PullPin *)iface;

    TRACE("(%p)->()\n", iface);

    /* Send further first: Else a race condition might terminate processing early */
    EnterCriticalSection(This->pin.pCritSec);
    SendFurther( iface, deliver_endflush, NULL, NULL );
    LeaveCriticalSection(This->pin.pCritSec);

    EnterCriticalSection(&This->thread_lock);
    {
        FILTER_STATE state;
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
    PullPin *This = (PullPin *)iface;

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
    IPinImpl_AddRef,
    PullPin_Release,
    InputPin_Connect,
    PullPin_ReceiveConnection,
    PullPin_Disconnect,
    IPinImpl_ConnectedTo,
    IPinImpl_ConnectionMediaType,
    IPinImpl_QueryPinInfo,
    IPinImpl_QueryDirection,
    IPinImpl_QueryId,
    IPinImpl_QueryAccept,
    IPinImpl_EnumMediaTypes,
    IPinImpl_QueryInternalConnections,
    PullPin_EndOfStream,
    PullPin_BeginFlush,
    PullPin_EndFlush,
    PullPin_NewSegment
};
