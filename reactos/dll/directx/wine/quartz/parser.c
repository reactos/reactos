/*
 * Parser (Base for parsers and splitters)
 *
 * Copyright 2003 Robert Shearman
 * Copyright 2004-2005 Christian Costa
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

static const WCHAR wcsInputPinName[] = {'i','n','p','u','t',' ','p','i','n',0};
static const IMediaSeekingVtbl Parser_Seeking_Vtbl;
static const IPinVtbl Parser_OutputPin_Vtbl;
static const IPinVtbl Parser_InputPin_Vtbl;

static HRESULT WINAPI Parser_ChangeStart(IMediaSeeking *iface);
static HRESULT WINAPI Parser_ChangeStop(IMediaSeeking *iface);
static HRESULT WINAPI Parser_ChangeRate(IMediaSeeking *iface);
static HRESULT WINAPI Parser_OutputPin_DecideBufferSize(BaseOutputPin *iface, IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest);
static HRESULT WINAPI Parser_OutputPin_GetMediaType(BasePin *iface, int iPosition, AM_MEDIA_TYPE *pmt);
static HRESULT WINAPI Parser_OutputPin_DecideAllocator(BaseOutputPin *This, IMemInputPin *pPin, IMemAllocator **pAlloc);
static HRESULT WINAPI Parser_OutputPin_BreakConnect(BaseOutputPin *This);

static inline ParserImpl *impl_from_IMediaSeeking( IMediaSeeking *iface )
{
    return CONTAINING_RECORD(iface, ParserImpl, sourceSeeking.IMediaSeeking_iface);
}

static inline ParserImpl *impl_from_IBaseFilter( IBaseFilter *iface )
{
    return CONTAINING_RECORD(iface, ParserImpl, filter.IBaseFilter_iface);
}

static inline ParserImpl *impl_from_BaseFilter( BaseFilter *iface )
{
    return CONTAINING_RECORD(iface, ParserImpl, filter);
}

/* FIXME: WRONG */
static IPin* WINAPI Parser_GetPin(BaseFilter *iface, int pos)
{
    ParserImpl *This = impl_from_BaseFilter(iface);

    TRACE("Asking for pos %x\n", pos);

    /* Input pin also has a pin, hence the > and not >= */
    if (pos > This->cStreams || pos < 0)
        return NULL;

    IPin_AddRef(This->ppPins[pos]);
    return This->ppPins[pos];
}

static LONG WINAPI Parser_GetPinCount(BaseFilter *iface)
{
    ParserImpl *This = impl_from_BaseFilter(iface);

    return This->cStreams;
}

static const BaseFilterFuncTable BaseFuncTable = {
    Parser_GetPin,
    Parser_GetPinCount
};

HRESULT Parser_Create(ParserImpl* pParser, const IBaseFilterVtbl *Parser_Vtbl, const CLSID* pClsid, PFN_PROCESS_SAMPLE fnProcessSample, PFN_QUERY_ACCEPT fnQueryAccept, PFN_PRE_CONNECT fnPreConnect, PFN_CLEANUP fnCleanup, PFN_DISCONNECT fnDisconnect, REQUESTPROC fnRequest, STOPPROCESSPROC fnDone, SourceSeeking_ChangeStop stop, SourceSeeking_ChangeStart start, SourceSeeking_ChangeRate rate)
{
    HRESULT hr;
    PIN_INFO piInput;

    /* pTransformFilter is already allocated */
    BaseFilter_Init(&pParser->filter, Parser_Vtbl, pClsid, (DWORD_PTR)(__FILE__ ": ParserImpl.csFilter"), &BaseFuncTable);

    pParser->fnDisconnect = fnDisconnect;

    pParser->cStreams = 0;
    pParser->ppPins = CoTaskMemAlloc(1 * sizeof(IPin *));

    /* construct input pin */
    piInput.dir = PINDIR_INPUT;
    piInput.pFilter = &pParser->filter.IBaseFilter_iface;
    lstrcpynW(piInput.achName, wcsInputPinName, sizeof(piInput.achName) / sizeof(piInput.achName[0]));

    if (!start)
        start = Parser_ChangeStart;

    if (!stop)
        stop = Parser_ChangeStop;

    if (!rate)
        rate = Parser_ChangeRate;

    SourceSeeking_Init(&pParser->sourceSeeking, &Parser_Seeking_Vtbl, stop, start, rate,  &pParser->filter.csFilter);

    hr = PullPin_Construct(&Parser_InputPin_Vtbl, &piInput, fnProcessSample, (LPVOID)pParser, fnQueryAccept, fnCleanup, fnRequest, fnDone, &pParser->filter.csFilter, (IPin **)&pParser->pInputPin);

    if (SUCCEEDED(hr))
    {
        pParser->ppPins[0] = &pParser->pInputPin->pin.IPin_iface;
        pParser->pInputPin->fnPreConnect = fnPreConnect;
    }
    else
    {
        CoTaskMemFree(pParser->ppPins);
        BaseFilterImpl_Release(&pParser->filter.IBaseFilter_iface);
        CoTaskMemFree(pParser);
    }

    return hr;
}

HRESULT WINAPI Parser_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv)
{
    ParserImpl *This = impl_from_IBaseFilter(iface);
    TRACE("(%s, %p)\n", qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if ( IsEqualIID(riid, &IID_IUnknown)
      || IsEqualIID(riid, &IID_IPersist)
      || IsEqualIID(riid, &IID_IMediaFilter)
      || IsEqualIID(riid, &IID_IBaseFilter) )
        *ppv = &This->filter.IBaseFilter_iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)*ppv);
        return S_OK;
    }

    if (!IsEqualIID(riid, &IID_IPin) && !IsEqualIID(riid, &IID_IVideoWindow))
        FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

ULONG WINAPI Parser_AddRef(IBaseFilter * iface)
{
    return BaseFilterImpl_AddRef(iface);
}

void Parser_Destroy(ParserImpl *This)
{
    IPin *connected = NULL;
    ULONG pinref;
    HRESULT hr;

    assert(!This->filter.refCount);
    PullPin_WaitForStateChange(This->pInputPin, INFINITE);

    /* Don't need to clean up output pins, freeing input pin will do that */
    IPin_ConnectedTo(&This->pInputPin->pin.IPin_iface, &connected);
    if (connected)
    {
        hr = IPin_Disconnect(connected);
        assert(hr == S_OK);
        IPin_Release(connected);
        hr = IPin_Disconnect(&This->pInputPin->pin.IPin_iface);
        assert(hr == S_OK);
    }
    pinref = IPin_Release(&This->pInputPin->pin.IPin_iface);
    if (pinref)
    {
        /* Valgrind could find this, if I kill it here */
        ERR("pinref should be null, is %u, destroying anyway\n", pinref);
        assert((LONG)pinref > 0);

        while (pinref)
            pinref = IPin_Release(&This->pInputPin->pin.IPin_iface);
    }

    CoTaskMemFree(This->ppPins);
    BaseFilter_Destroy(&This->filter);

    TRACE("Destroying parser\n");
    CoTaskMemFree(This);
}

ULONG WINAPI Parser_Release(IBaseFilter * iface)
{
    ParserImpl *This = impl_from_IBaseFilter(iface);
    ULONG refCount = InterlockedDecrement(&This->filter.refCount);

    TRACE("(%p)->() Release from %d\n", This, refCount + 1);

    if (!refCount)
        Parser_Destroy(This);

    return refCount;
}

/** IPersist methods **/

HRESULT WINAPI Parser_GetClassID(IBaseFilter * iface, CLSID * pClsid)
{
    ParserImpl *This = impl_from_IBaseFilter(iface);

    TRACE("(%p)\n", pClsid);

    *pClsid = This->filter.clsid;

    return S_OK;
}

/** IMediaFilter methods **/

HRESULT WINAPI Parser_Stop(IBaseFilter * iface)
{
    ParserImpl *This = impl_from_IBaseFilter(iface);
    PullPin *pin = impl_PullPin_from_IPin(This->ppPins[0]);
    ULONG i;

    TRACE("()\n");

    EnterCriticalSection(&pin->thread_lock);

    IAsyncReader_BeginFlush(This->pInputPin->pReader);
    EnterCriticalSection(&This->filter.csFilter);

    if (This->filter.state == State_Stopped)
    {
        LeaveCriticalSection(&This->filter.csFilter);
        IAsyncReader_EndFlush(This->pInputPin->pReader);
        LeaveCriticalSection(&pin->thread_lock);
        return S_OK;
    }

    This->filter.state = State_Stopped;

    for (i = 1; i < (This->cStreams + 1); i++)
    {
        BaseOutputPinImpl_Inactive((BaseOutputPin *)This->ppPins[i]);
    }

    LeaveCriticalSection(&This->filter.csFilter);

    PullPin_PauseProcessing(This->pInputPin);
    PullPin_WaitForStateChange(This->pInputPin, INFINITE);
    IAsyncReader_EndFlush(This->pInputPin->pReader);

    LeaveCriticalSection(&pin->thread_lock);
    return S_OK;
}

HRESULT WINAPI Parser_Pause(IBaseFilter * iface)
{
    HRESULT hr = S_OK;
    ParserImpl *This = impl_from_IBaseFilter(iface);
    PullPin *pin = impl_PullPin_from_IPin(This->ppPins[0]);

    TRACE("()\n");

    EnterCriticalSection(&pin->thread_lock);
    EnterCriticalSection(&This->filter.csFilter);

    if (This->filter.state == State_Paused)
    {
        LeaveCriticalSection(&This->filter.csFilter);
        LeaveCriticalSection(&pin->thread_lock);
        return S_OK;
    }

    if (This->filter.state == State_Stopped)
    {
        LeaveCriticalSection(&This->filter.csFilter);
        hr = IBaseFilter_Run(iface, -1);
        EnterCriticalSection(&This->filter.csFilter);
    }

    if (SUCCEEDED(hr))
        This->filter.state = State_Paused;

    LeaveCriticalSection(&This->filter.csFilter);
    LeaveCriticalSection(&pin->thread_lock);

    return hr;
}

HRESULT WINAPI Parser_Run(IBaseFilter * iface, REFERENCE_TIME tStart)
{
    HRESULT hr = S_OK;
    ParserImpl *This = impl_from_IBaseFilter(iface);
    PullPin *pin = impl_PullPin_from_IPin(This->ppPins[0]);

    ULONG i;

    TRACE("(%s)\n", wine_dbgstr_longlong(tStart));

    EnterCriticalSection(&pin->thread_lock);
    EnterCriticalSection(&This->filter.csFilter);
    {
        HRESULT hr_any = VFW_E_NOT_CONNECTED;

        This->filter.rtStreamStart = tStart;
        if (This->filter.state == State_Running || This->filter.state == State_Paused)
        {
            This->filter.state = State_Running;
            LeaveCriticalSection(&This->filter.csFilter);
            LeaveCriticalSection(&pin->thread_lock);
            return S_OK;
        }

        for (i = 1; i < (This->cStreams + 1); i++)
        {
            hr = BaseOutputPinImpl_Active((BaseOutputPin *)This->ppPins[i]);
            if (SUCCEEDED(hr))
                hr_any = hr;
        }

        hr = hr_any;
        if (SUCCEEDED(hr))
        {
            LeaveCriticalSection(&This->filter.csFilter);
            hr = PullPin_StartProcessing(This->pInputPin);
            EnterCriticalSection(&This->filter.csFilter);
        }

        if (SUCCEEDED(hr))
            This->filter.state = State_Running;
    }
    LeaveCriticalSection(&This->filter.csFilter);
    LeaveCriticalSection(&pin->thread_lock);

    return hr;
}

HRESULT WINAPI Parser_GetState(IBaseFilter * iface, DWORD dwMilliSecsTimeout, FILTER_STATE *pState)
{
    ParserImpl *This = impl_from_IBaseFilter(iface);
    PullPin *pin = impl_PullPin_from_IPin(This->ppPins[0]);
    HRESULT hr = S_OK;

    TRACE("(%d, %p)\n", dwMilliSecsTimeout, pState);

    EnterCriticalSection(&pin->thread_lock);
    EnterCriticalSection(&This->filter.csFilter);
    {
        *pState = This->filter.state;
    }
    LeaveCriticalSection(&This->filter.csFilter);

    if (This->pInputPin && (PullPin_WaitForStateChange(This->pInputPin, dwMilliSecsTimeout) == S_FALSE))
        hr = VFW_S_STATE_INTERMEDIATE;
    LeaveCriticalSection(&pin->thread_lock);

    return hr;
}

HRESULT WINAPI Parser_SetSyncSource(IBaseFilter * iface, IReferenceClock *pClock)
{
    ParserImpl *This = impl_from_IBaseFilter(iface);
    PullPin *pin = impl_PullPin_from_IPin(This->ppPins[0]);

    TRACE("(%p)\n", pClock);

    EnterCriticalSection(&pin->thread_lock);
    BaseFilterImpl_SetSyncSource(iface,pClock);
    LeaveCriticalSection(&pin->thread_lock);

    return S_OK;
}

HRESULT WINAPI Parser_GetSyncSource(IBaseFilter * iface, IReferenceClock **ppClock)
{
    return BaseFilterImpl_GetSyncSource(iface, ppClock);
}

/** IBaseFilter implementation **/

HRESULT WINAPI Parser_EnumPins(IBaseFilter * iface, IEnumPins **ppEnum)
{
    return BaseFilterImpl_EnumPins(iface,ppEnum);
}

HRESULT WINAPI Parser_FindPin(IBaseFilter * iface, LPCWSTR Id, IPin **ppPin)
{
    FIXME("(%p)->(%s,%p)\n", iface, debugstr_w(Id), ppPin);

    /* FIXME: critical section */

    return E_NOTIMPL;
}

HRESULT WINAPI Parser_QueryFilterInfo(IBaseFilter * iface, FILTER_INFO *pInfo)
{
    return BaseFilterImpl_QueryFilterInfo(iface, pInfo);
}

HRESULT WINAPI Parser_JoinFilterGraph(IBaseFilter * iface, IFilterGraph *pGraph, LPCWSTR pName)
{
    return BaseFilterImpl_JoinFilterGraph(iface, pGraph, pName);
}

HRESULT WINAPI Parser_QueryVendorInfo(IBaseFilter * iface, LPWSTR *pVendorInfo)
{
    return BaseFilterImpl_QueryVendorInfo(iface, pVendorInfo);
}

static const BaseOutputPinFuncTable output_BaseOutputFuncTable = {
    {
        NULL,
        BaseOutputPinImpl_AttemptConnection,
        BasePinImpl_GetMediaTypeVersion,
        Parser_OutputPin_GetMediaType
    },
    Parser_OutputPin_DecideBufferSize,
    Parser_OutputPin_DecideAllocator,
    Parser_OutputPin_BreakConnect
};

HRESULT Parser_AddPin(ParserImpl * This, const PIN_INFO * piOutput, ALLOCATOR_PROPERTIES * props, const AM_MEDIA_TYPE * amt)
{
    IPin ** ppOldPins;
    HRESULT hr;

    ppOldPins = This->ppPins;

    This->ppPins = CoTaskMemAlloc((This->cStreams + 2) * sizeof(IPin *));
    memcpy(This->ppPins, ppOldPins, (This->cStreams + 1) * sizeof(IPin *));

    hr = BaseOutputPin_Construct(&Parser_OutputPin_Vtbl, sizeof(Parser_OutputPin), piOutput, &output_BaseOutputFuncTable, &This->filter.csFilter, This->ppPins + (This->cStreams + 1));

    if (SUCCEEDED(hr))
    {
        IPin *pPin = This->ppPins[This->cStreams + 1];
        Parser_OutputPin *pin = unsafe_impl_Parser_OutputPin_from_IPin(pPin);
        pin->pmt = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
        CopyMediaType(pin->pmt, amt);
        pin->dwSamplesProcessed = 0;

        pin->pin.pin.pinInfo.pFilter = (LPVOID)This;
        pin->allocProps = *props;
        This->cStreams++;
        BaseFilterImpl_IncrementPinVersion(&This->filter);
        CoTaskMemFree(ppOldPins);
    }
    else
    {
        CoTaskMemFree(This->ppPins);
        This->ppPins = ppOldPins;
        ERR("Failed with error %x\n", hr);
    }

    return hr;
}

static HRESULT Parser_RemoveOutputPins(ParserImpl * This)
{
    /* NOTE: should be in critical section when calling this function */
    HRESULT hr;
    ULONG i;
    IPin ** ppOldPins = This->ppPins;

    TRACE("(%p)\n", This);

    /* reduce the pin array down to 1 (just our input pin) */
    This->ppPins = CoTaskMemAlloc(sizeof(IPin *) * 1);
    memcpy(This->ppPins, ppOldPins, sizeof(IPin *) * 1);

    for (i = 0; i < This->cStreams; i++)
    {
        hr = ((BaseOutputPin *)ppOldPins[i + 1])->pFuncsTable->pfnBreakConnect((BaseOutputPin *)ppOldPins[i + 1]);
        TRACE("Disconnect: %08x\n", hr);
        IPin_Release(ppOldPins[i + 1]);
    }

    BaseFilterImpl_IncrementPinVersion(&This->filter);
    This->cStreams = 0;
    CoTaskMemFree(ppOldPins);

    return S_OK;
}

static HRESULT WINAPI Parser_ChangeStart(IMediaSeeking *iface)
{
    FIXME("(%p) filter hasn't implemented start position change!\n", iface);
    return S_OK;
}

static HRESULT WINAPI Parser_ChangeStop(IMediaSeeking *iface)
{
    FIXME("(%p) filter hasn't implemented stop position change!\n", iface);
    return S_OK;
}

static HRESULT WINAPI Parser_ChangeRate(IMediaSeeking *iface)
{
    FIXME("(%p) filter hasn't implemented rate change!\n", iface);
    return S_OK;
}


static HRESULT WINAPI Parser_Seeking_QueryInterface(IMediaSeeking * iface, REFIID riid, LPVOID * ppv)
{
    ParserImpl *This = impl_from_IMediaSeeking(iface);

    return IBaseFilter_QueryInterface(&This->filter.IBaseFilter_iface, riid, ppv);
}

static ULONG WINAPI Parser_Seeking_AddRef(IMediaSeeking * iface)
{
    ParserImpl *This = impl_from_IMediaSeeking(iface);

    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI Parser_Seeking_Release(IMediaSeeking * iface)
{
    ParserImpl *This = impl_from_IMediaSeeking(iface);

    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static const IMediaSeekingVtbl Parser_Seeking_Vtbl =
{
    Parser_Seeking_QueryInterface,
    Parser_Seeking_AddRef,
    Parser_Seeking_Release,
    SourceSeekingImpl_GetCapabilities,
    SourceSeekingImpl_CheckCapabilities,
    SourceSeekingImpl_IsFormatSupported,
    SourceSeekingImpl_QueryPreferredFormat,
    SourceSeekingImpl_GetTimeFormat,
    SourceSeekingImpl_IsUsingTimeFormat,
    SourceSeekingImpl_SetTimeFormat,
    SourceSeekingImpl_GetDuration,
    SourceSeekingImpl_GetStopPosition,
    SourceSeekingImpl_GetCurrentPosition,
    SourceSeekingImpl_ConvertTimeFormat,
    SourceSeekingImpl_SetPositions,
    SourceSeekingImpl_GetPositions,
    SourceSeekingImpl_GetAvailable,
    SourceSeekingImpl_SetRate,
    SourceSeekingImpl_GetRate,
    SourceSeekingImpl_GetPreroll
};

static HRESULT WINAPI Parser_OutputPin_DecideBufferSize(BaseOutputPin *iface, IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest)
{
    Parser_OutputPin *This = (Parser_OutputPin*)iface;
    ALLOCATOR_PROPERTIES actual;

    if (ppropInputRequest->cbAlign && ppropInputRequest->cbAlign != This->allocProps.cbAlign)
        FIXME("Requested Buffer cbAlign mismatch %i,%i\n",This->allocProps.cbAlign, ppropInputRequest->cbAlign);
    if (ppropInputRequest->cbPrefix)
        FIXME("Requested Buffer cbPrefix mismatch %i,%i\n",This->allocProps.cbPrefix, ppropInputRequest->cbPrefix);
    if (ppropInputRequest->cbBuffer)
        FIXME("Requested Buffer cbBuffer mismatch %i,%i\n",This->allocProps.cbBuffer, ppropInputRequest->cbBuffer);
    if (ppropInputRequest->cBuffers)
        FIXME("Requested Buffer cBuffers mismatch %i,%i\n",This->allocProps.cBuffers, ppropInputRequest->cBuffers);

    return IMemAllocator_SetProperties(pAlloc, &This->allocProps, &actual);
}

static HRESULT WINAPI Parser_OutputPin_GetMediaType(BasePin *iface, int iPosition, AM_MEDIA_TYPE *pmt)
{
    Parser_OutputPin *This = (Parser_OutputPin*)iface;
    if (iPosition < 0)
        return E_INVALIDARG;
    if (iPosition > 0)
        return VFW_S_NO_MORE_ITEMS;
    CopyMediaType(pmt, This->pmt);
    return S_OK;
}

static HRESULT WINAPI Parser_OutputPin_DecideAllocator(BaseOutputPin *iface, IMemInputPin *pPin, IMemAllocator **pAlloc)
{
    Parser_OutputPin *This = (Parser_OutputPin*)iface;
    HRESULT hr;

    *pAlloc = NULL;

    if (This->alloc)
    {
        hr = IMemInputPin_NotifyAllocator(pPin, This->alloc, This->readonly);
        if (SUCCEEDED(hr))
        {
            *pAlloc = This->alloc;
            IMemAllocator_AddRef(*pAlloc);
        }
    }
    else
        hr = VFW_E_NO_ALLOCATOR;

    return hr;
}

static HRESULT WINAPI Parser_OutputPin_BreakConnect(BaseOutputPin *This)
{
    HRESULT hr;

    TRACE("(%p)->()\n", This);

    EnterCriticalSection(This->pin.pCritSec);
    if (!This->pin.pConnectedTo || !This->pMemInputPin)
        hr = VFW_E_NOT_CONNECTED;
    else
    {
        hr = IPin_Disconnect(This->pin.pConnectedTo);
        IPin_Disconnect(&This->pin.IPin_iface);
    }
    LeaveCriticalSection(This->pin.pCritSec);

    return hr;
}


static HRESULT WINAPI Parser_OutputPin_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv)
{
    Parser_OutputPin *This = unsafe_impl_Parser_OutputPin_from_IPin(iface);

    TRACE("(%s, %p)\n", qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IPin))
        *ppv = iface;
    /* The Parser filter does not support querying IMediaSeeking, return it directly */
    else if (IsEqualIID(riid, &IID_IMediaSeeking))
        *ppv = &impl_from_IBaseFilter(This->pin.pin.pinInfo.pFilter)->sourceSeeking;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI Parser_OutputPin_Release(IPin * iface)
{
    Parser_OutputPin *This = unsafe_impl_Parser_OutputPin_from_IPin(iface);
    ULONG refCount = InterlockedDecrement(&This->pin.pin.refCount);
    
    TRACE("(%p)->() Release from %d\n", iface, refCount + 1);

    if (!refCount)
    {
        FreeMediaType(This->pmt);
        CoTaskMemFree(This->pmt);
        FreeMediaType(&This->pin.pin.mtCurrent);
        if (This->pin.pAllocator)
            IMemAllocator_Release(This->pin.pAllocator);
        CoTaskMemFree(This);
        return 0;
    }
    return refCount;
}

static HRESULT WINAPI Parser_OutputPin_Connect(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    Parser_OutputPin *This = unsafe_impl_Parser_OutputPin_from_IPin(iface);
    ParserImpl *parser = impl_from_IBaseFilter(This->pin.pin.pinInfo.pFilter);

    /* Set the allocator to our input pin's */
    EnterCriticalSection(This->pin.pin.pCritSec);
    This->alloc = parser->pInputPin->pAlloc;
    LeaveCriticalSection(This->pin.pin.pCritSec);

    return BaseOutputPinImpl_Connect(iface, pReceivePin, pmt);
}

static HRESULT WINAPI Parser_OutputPin_QueryAccept(IPin *iface, const AM_MEDIA_TYPE * pmt)
{
    Parser_OutputPin *This = unsafe_impl_Parser_OutputPin_from_IPin(iface);

    TRACE("()\n");
    dump_AM_MEDIA_TYPE(pmt);

    return (memcmp(This->pmt, pmt, sizeof(AM_MEDIA_TYPE)) == 0);
}

static const IPinVtbl Parser_OutputPin_Vtbl = 
{
    Parser_OutputPin_QueryInterface,
    BasePinImpl_AddRef,
    Parser_OutputPin_Release,
    Parser_OutputPin_Connect,
    BaseOutputPinImpl_ReceiveConnection,
    BaseOutputPinImpl_Disconnect,
    BasePinImpl_ConnectedTo,
    BasePinImpl_ConnectionMediaType,
    BasePinImpl_QueryPinInfo,
    BasePinImpl_QueryDirection,
    BasePinImpl_QueryId,
    Parser_OutputPin_QueryAccept,
    BasePinImpl_EnumMediaTypes,
    BasePinImpl_QueryInternalConnections,
    BaseOutputPinImpl_EndOfStream,
    BaseOutputPinImpl_BeginFlush,
    BaseOutputPinImpl_EndFlush,
    BasePinImpl_NewSegment
};

static HRESULT WINAPI Parser_PullPin_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv)
{
    PullPin *This = impl_PullPin_from_IPin(iface);

    TRACE("(%p/%p)->(%s, %p)\n", This, iface, qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    /*
     * It is important to capture the request for the IMediaSeeking interface before it is passed
     * on to PullPin_QueryInterface, this is necessary since the Parser filter does not support
     * querying IMediaSeeking
     */
    if (IsEqualIID(riid, &IID_IMediaSeeking))
        *ppv = &impl_from_IBaseFilter(This->pin.pinInfo.pFilter)->sourceSeeking;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    return PullPin_QueryInterface(iface, riid, ppv);
}

static HRESULT WINAPI Parser_PullPin_Disconnect(IPin * iface)
{
    HRESULT hr;
    PullPin *This = impl_PullPin_from_IPin(iface);

    TRACE("()\n");

    EnterCriticalSection(&This->thread_lock);
    EnterCriticalSection(This->pin.pCritSec);
    {
        if (This->pin.pConnectedTo)
        {
            FILTER_STATE state;
            ParserImpl *Parser = impl_from_IBaseFilter(This->pin.pinInfo.pFilter);

            LeaveCriticalSection(This->pin.pCritSec);
            hr = IBaseFilter_GetState(This->pin.pinInfo.pFilter, INFINITE, &state);
            EnterCriticalSection(This->pin.pCritSec);

            if (SUCCEEDED(hr) && (state == State_Stopped) && SUCCEEDED(Parser->fnDisconnect(Parser)))
            {
                LeaveCriticalSection(This->pin.pCritSec);
                PullPin_Disconnect(iface);
                EnterCriticalSection(This->pin.pCritSec);
                hr = Parser_RemoveOutputPins(impl_from_IBaseFilter(This->pin.pinInfo.pFilter));
            }
            else
                hr = VFW_E_NOT_STOPPED;
        }
        else
            hr = S_FALSE;
    }
    LeaveCriticalSection(This->pin.pCritSec);
    LeaveCriticalSection(&This->thread_lock);

    return hr;
}

static HRESULT WINAPI Parser_PullPin_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    HRESULT hr;

    TRACE("()\n");

    hr = PullPin_ReceiveConnection(iface, pReceivePin, pmt);
    if (FAILED(hr))
    {
        BasePin *This = (BasePin *)iface;

        EnterCriticalSection(This->pCritSec);
        Parser_RemoveOutputPins(impl_from_IBaseFilter(This->pinInfo.pFilter));
        LeaveCriticalSection(This->pCritSec);
    }

    return hr;
}

static HRESULT WINAPI Parser_PullPin_EnumMediaTypes(IPin *iface, IEnumMediaTypes **ppEnum)
{
    BasePin *This = (BasePin *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, ppEnum);

    return EnumMediaTypes_Construct(This, BasePinImpl_GetMediaType, BasePinImpl_GetMediaTypeVersion, ppEnum);
}

static const IPinVtbl Parser_InputPin_Vtbl =
{
    Parser_PullPin_QueryInterface,
    BasePinImpl_AddRef,
    PullPin_Release,
    BaseInputPinImpl_Connect,
    Parser_PullPin_ReceiveConnection,
    Parser_PullPin_Disconnect,
    BasePinImpl_ConnectedTo,
    BasePinImpl_ConnectionMediaType,
    BasePinImpl_QueryPinInfo,
    BasePinImpl_QueryDirection,
    BasePinImpl_QueryId,
    PullPin_QueryAccept,
    Parser_PullPin_EnumMediaTypes,
    BasePinImpl_QueryInternalConnections,
    PullPin_EndOfStream,
    PullPin_BeginFlush,
    PullPin_EndFlush,
    PullPin_NewSegment
};
