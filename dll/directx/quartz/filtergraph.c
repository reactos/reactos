/*              DirectShow FilterGraph object (QUARTZ.DLL)
 *
 * Copyright 2002 Lionel Ulmer
 * Copyright 2004 Christian Costa
 *
 * This file contains the (internal) driver registration functions,
 * driver enumeration APIs and DirectDraw creation functions.
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
#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "shlwapi.h"
#include "dshow.h"
#include "wine/debug.h"
#include "quartz_private.h"
#include "ole2.h"
#include "olectl.h"
#include "strmif.h"
#include "vfwmsgs.h"
#include "evcode.h"
#include "wine/unicode.h"


WINE_DEFAULT_DEBUG_CHANNEL(quartz);

typedef struct {
    HWND hWnd;      /* Target window */
    long msg;       /* User window message */
    long instance;  /* User data */
    int  disabled;  /* Disabled messages posting */
} WndNotify;

typedef struct {
    long lEventCode;   /* Event code */
    LONG_PTR lParam1;  /* Param1 */
    LONG_PTR lParam2;  /* Param2 */
} Event;

/* messages ring implementation for queuing events (taken from winmm) */
#define EVENTS_RING_BUFFER_INCREMENT      64
typedef struct {
    Event* messages;
    int ring_buffer_size;
    int msg_tosave;
    int msg_toget;
    CRITICAL_SECTION msg_crst;
    HANDLE msg_event; /* Signaled for no empty queue */
} EventsQueue;

static int EventsQueue_Init(EventsQueue* omr)
{
    omr->msg_toget = 0;
    omr->msg_tosave = 0;
    omr->msg_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    omr->ring_buffer_size = EVENTS_RING_BUFFER_INCREMENT;
    omr->messages = CoTaskMemAlloc(omr->ring_buffer_size * sizeof(Event));
    ZeroMemory(omr->messages, omr->ring_buffer_size * sizeof(Event));

    InitializeCriticalSection(&omr->msg_crst);
    omr->msg_crst.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": EventsQueue.msg_crst");
    return TRUE;
}

static int EventsQueue_Destroy(EventsQueue* omr)
{
    CloseHandle(omr->msg_event);
    CoTaskMemFree(omr->messages);
    omr->msg_crst.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&omr->msg_crst);
    return TRUE;
}

static int EventsQueue_PutEvent(EventsQueue* omr, const Event* evt)
{
    EnterCriticalSection(&omr->msg_crst);
    if ((omr->msg_toget == ((omr->msg_tosave + 1) % omr->ring_buffer_size)))
    {
	int old_ring_buffer_size = omr->ring_buffer_size;
	omr->ring_buffer_size += EVENTS_RING_BUFFER_INCREMENT;
	TRACE("omr->ring_buffer_size=%d\n",omr->ring_buffer_size);
	omr->messages = HeapReAlloc(GetProcessHeap(),0,omr->messages, omr->ring_buffer_size * sizeof(Event));
	/* Now we need to rearrange the ring buffer so that the new
	   buffers just allocated are in between omr->msg_tosave and
	   omr->msg_toget.
	*/
	if (omr->msg_tosave < omr->msg_toget)
	{
	    memmove(&(omr->messages[omr->msg_toget + EVENTS_RING_BUFFER_INCREMENT]),
		    &(omr->messages[omr->msg_toget]),
		    sizeof(Event)*(old_ring_buffer_size - omr->msg_toget)
		    );
	    omr->msg_toget += EVENTS_RING_BUFFER_INCREMENT;
	}
    }
    omr->messages[omr->msg_tosave] = *evt;
    SetEvent(omr->msg_event);
    omr->msg_tosave = (omr->msg_tosave + 1) % omr->ring_buffer_size;
    LeaveCriticalSection(&omr->msg_crst);
    return TRUE;
}

static int EventsQueue_GetEvent(EventsQueue* omr, Event* evt, long msTimeOut)
{
    if (WaitForSingleObject(omr->msg_event, msTimeOut) != WAIT_OBJECT_0)
	return FALSE;
	
    EnterCriticalSection(&omr->msg_crst);

    if (omr->msg_toget == omr->msg_tosave) /* buffer empty ? */
    {
        LeaveCriticalSection(&omr->msg_crst);
	return FALSE;
    }

    *evt = omr->messages[omr->msg_toget];
    omr->msg_toget = (omr->msg_toget + 1) % omr->ring_buffer_size;

    /* Mark the buffer as empty if needed */
    if (omr->msg_toget == omr->msg_tosave) /* buffer empty ? */
	ResetEvent(omr->msg_event);

    LeaveCriticalSection(&omr->msg_crst);
    return TRUE;
}

#define MAX_ITF_CACHE_ENTRIES 3
typedef struct _ITF_CACHE_ENTRY {
   const IID* riid;
   IBaseFilter* filter;
   IUnknown* iface;
} ITF_CACHE_ENTRY;

typedef struct _IFilterGraphImpl {
    const IFilterGraph2Vtbl *IFilterGraph2_vtbl;
    const IMediaControlVtbl *IMediaControl_vtbl;
    const IMediaSeekingVtbl *IMediaSeeking_vtbl;
    const IBasicAudioVtbl *IBasicAudio_vtbl;
    const IBasicVideo2Vtbl *IBasicVideo_vtbl;
    const IVideoWindowVtbl *IVideoWindow_vtbl;
    const IMediaEventExVtbl *IMediaEventEx_vtbl;
    const IMediaFilterVtbl *IMediaFilter_vtbl;
    const IMediaEventSinkVtbl *IMediaEventSink_vtbl;
    const IGraphConfigVtbl *IGraphConfig_vtbl;
    const IMediaPositionVtbl *IMediaPosition_vtbl;
    const IUnknownVtbl * IInner_vtbl;
    /* IAMGraphStreams */
    /* IAMStats */
    /* IFilterChain */
    /* IFilterMapper2 */
    /* IGraphVersion */
    /* IQueueCommand */
    /* IRegisterServiceProvider */
    /* IResourceMananger */
    /* IServiceProvider */
    /* IVideoFrameStep */

    LONG ref;
    IUnknown *punkFilterMapper2;
    IFilterMapper2 * pFilterMapper2;
    IBaseFilter ** ppFiltersInGraph;
    LPWSTR * pFilterNames;
    int nFilters;
    int filterCapacity;
    long nameIndex;
    IReferenceClock *refClock;
    EventsQueue evqueue;
    HANDLE hEventCompletion;
    int CompletionStatus;
    WndNotify notif;
    int nRenderers;
    int EcCompleteCount;
    int HandleEcComplete;
    int HandleEcRepaint;
    int HandleEcClockChanged;
    OAFilterState state;
    CRITICAL_SECTION cs;
    ITF_CACHE_ENTRY ItfCacheEntries[MAX_ITF_CACHE_ENTRIES];
    int nItfCacheEntries;
    IUnknown * pUnkOuter;
    BOOL bUnkOuterValid;
    BOOL bAggregatable;
    GUID timeformatseek;
    LONGLONG start_time;
    LONGLONG position;
    LONGLONG stop_position;
    LONG recursioncount;
} IFilterGraphImpl;

static HRESULT Filtergraph_QueryInterface(IFilterGraphImpl *This,
                                          REFIID riid, LPVOID * ppv);
static ULONG Filtergraph_AddRef(IFilterGraphImpl *This);
static ULONG Filtergraph_Release(IFilterGraphImpl *This);

static HRESULT WINAPI FilterGraphInner_QueryInterface(IUnknown * iface,
					  REFIID riid,
					  LPVOID *ppvObj) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IInner_vtbl, iface);
    TRACE("(%p)->(%s (%p), %p)\n", This, debugstr_guid(riid), riid, ppvObj);
    
    if (This->bAggregatable)
        This->bUnkOuterValid = TRUE;

    if (IsEqualGUID(&IID_IUnknown, riid)) {
        *ppvObj = &(This->IInner_vtbl);
        TRACE("   returning IUnknown interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IFilterGraph, riid) ||
	IsEqualGUID(&IID_IFilterGraph2, riid) ||
	IsEqualGUID(&IID_IGraphBuilder, riid)) {
        *ppvObj = &(This->IFilterGraph2_vtbl);
        TRACE("   returning IGraphBuilder interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaControl, riid)) {
        *ppvObj = &(This->IMediaControl_vtbl);
        TRACE("   returning IMediaControl interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaSeeking, riid)) {
        *ppvObj = &(This->IMediaSeeking_vtbl);
        TRACE("   returning IMediaSeeking interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IBasicAudio, riid)) {
        *ppvObj = &(This->IBasicAudio_vtbl);
        TRACE("   returning IBasicAudio interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IBasicVideo, riid) ||
               IsEqualGUID(&IID_IBasicVideo2, riid)) {
        *ppvObj = &(This->IBasicVideo_vtbl);
        TRACE("   returning IBasicVideo2 interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IVideoWindow, riid)) {
        *ppvObj = &(This->IVideoWindow_vtbl);
        TRACE("   returning IVideoWindow interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaEvent, riid) ||
	   IsEqualGUID(&IID_IMediaEventEx, riid)) {
        *ppvObj = &(This->IMediaEventEx_vtbl);
        TRACE("   returning IMediaEvent(Ex) interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaFilter, riid) ||
          IsEqualGUID(&IID_IPersist, riid)) {
        *ppvObj = &(This->IMediaFilter_vtbl);
        TRACE("   returning IMediaFilter interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaEventSink, riid)) {
        *ppvObj = &(This->IMediaEventSink_vtbl);
        TRACE("   returning IMediaEventSink interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IGraphConfig, riid)) {
        *ppvObj = &(This->IGraphConfig_vtbl);
        TRACE("   returning IGraphConfig interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaPosition, riid)) {
        *ppvObj = &(This->IMediaPosition_vtbl);
        TRACE("   returning IMediaPosition interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IFilterMapper, riid)) {
        TRACE("   requesting IFilterMapper interface from aggregated filtermapper (%p)\n", *ppvObj);
        return IUnknown_QueryInterface(This->punkFilterMapper2, riid, ppvObj);
    } else if (IsEqualGUID(&IID_IFilterMapper2, riid)) {
        *ppvObj = This->pFilterMapper2;
        TRACE("   returning IFilterMapper2 interface from aggregated filtermapper (%p)\n", *ppvObj);
    } else {
        *ppvObj = NULL;
	FIXME("unknown interface %s\n", debugstr_guid(riid));
	return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)(*ppvObj));
    return S_OK;
}

static ULONG WINAPI FilterGraphInner_AddRef(IUnknown * iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IInner_vtbl, iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", This, ref);
    
    return ref;
}

static ULONG WINAPI FilterGraphInner_Release(IUnknown * iface)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IInner_vtbl, iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", This, ref);

    if (ref == 0) {
        int i;

        This->ref = 1; /* guard against reentrancy (aggregation). */

        IMediaControl_Stop((IMediaControl*)&(This->IMediaControl_vtbl));

        while (This->nFilters)
            IFilterGraph2_RemoveFilter((IFilterGraph2*)This, This->ppFiltersInGraph[0]);

        if (This->refClock)
            IReferenceClock_Release(This->refClock);

        for (i = 0; i < This->nItfCacheEntries; i++)
        {
            if (This->ItfCacheEntries[i].iface)
                IUnknown_Release(This->ItfCacheEntries[i].iface);
        }

        /* AddRef on controlling IUnknown, to compensate for Release of cached IFilterMapper2 interface below.

         * NOTE: Filtergraph_AddRef isn't suitable, because bUnkOuterValid may be FALSE but punkOuter non-NULL
         * and already passed as punkOuter to filtermapper in FilterGraph_create - this will happen in case of
         * CoCreateInstance of filtergraph with non-null pUnkOuter and REFIID other than IID_Unknown that is
         * cleaning up after error. */
        if (This->pUnkOuter) IUnknown_AddRef(This->pUnkOuter);
        else IUnknown_AddRef((IUnknown*)&This->IInner_vtbl);

        IFilterMapper2_Release(This->pFilterMapper2);
        IUnknown_Release(This->punkFilterMapper2);

	CloseHandle(This->hEventCompletion);
	EventsQueue_Destroy(&This->evqueue);
        This->cs.DebugInfo->Spare[0] = 0;
	DeleteCriticalSection(&This->cs);
	CoTaskMemFree(This->ppFiltersInGraph);
	CoTaskMemFree(This->pFilterNames);
	CoTaskMemFree(This);
    }
    return ref;
}


/*** IUnknown methods ***/
static HRESULT WINAPI FilterGraph2_QueryInterface(IFilterGraph2 *iface,
						  REFIID riid,
						  LPVOID*ppvObj) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IFilterGraph2_vtbl, iface);
    
    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);
    return Filtergraph_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI FilterGraph2_AddRef(IFilterGraph2 *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IFilterGraph2_vtbl, iface);
    
    TRACE("(%p/%p)->() calling FilterGraph AddRef\n", This, iface);
    
    return Filtergraph_AddRef(This);
}

static ULONG WINAPI FilterGraph2_Release(IFilterGraph2 *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IFilterGraph2_vtbl, iface);
    
    TRACE("(%p/%p)->() calling FilterGraph Release\n", This, iface);

    return Filtergraph_Release(This);
}

/*** IFilterGraph methods ***/
static HRESULT WINAPI FilterGraph2_AddFilter(IFilterGraph2 *iface,
					     IBaseFilter *pFilter,
					     LPCWSTR pName) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IFilterGraph2_vtbl, iface);
    HRESULT hr;
    int i,j;
    WCHAR* wszFilterName = NULL;
    int duplicate_name = FALSE;

    TRACE("(%p/%p)->(%p, %s (%p))\n", This, iface, pFilter, debugstr_w(pName), pName);

    if (!pFilter)
        return E_POINTER;

    wszFilterName = CoTaskMemAlloc( (pName ? strlenW(pName) + 6 : 5) * sizeof(WCHAR) );

    if (pName)
    {
	/* Check if name already exists */
        for(i = 0; i < This->nFilters; i++)
	    if (!strcmpW(This->pFilterNames[i], pName))
	    {
		duplicate_name = TRUE;
		break;
	    }
    }

    /* If no name given or name already existing, generate one */
    if (!pName || duplicate_name)
    {
	static const WCHAR wszFmt1[] = {'%','s',' ','%','0','4','d',0};
	static const WCHAR wszFmt2[] = {'%','0','4','d',0};

	for (j = 0; j < 10000 ; j++)
	{
	    /* Create name */
	    if (pName)
		sprintfW(wszFilterName, wszFmt1, pName, This->nameIndex);
	    else
		sprintfW(wszFilterName, wszFmt2, This->nameIndex);
	    TRACE("Generated name %s\n", debugstr_w(wszFilterName));

	    /* Check if the generated name already exists */
	    for(i = 0; i < This->nFilters; i++)
	    	if (!strcmpW(This->pFilterNames[i], wszFilterName))
		    break;

	    /* Compute next index and exit if generated name is suitable */
	    if (This->nameIndex++ == 10000)
		This->nameIndex = 1;
	    if (i == This->nFilters)
		break;
	}
	/* Unable to find a suitable name */
	if (j == 10000)
	{
	    CoTaskMemFree(wszFilterName);
	    return VFW_E_DUPLICATE_NAME;
	}
    }
    else
	memcpy(wszFilterName, pName, (strlenW(pName) + 1) * sizeof(WCHAR));

    if (This->nFilters + 1 > This->filterCapacity)
    {
        int newCapacity = This->filterCapacity ? 2 * This->filterCapacity : 1;
        IBaseFilter ** ppNewFilters = CoTaskMemAlloc(newCapacity * sizeof(IBaseFilter*));
        LPWSTR * pNewNames = CoTaskMemAlloc(newCapacity * sizeof(LPWSTR));
        memcpy(ppNewFilters, This->ppFiltersInGraph, This->nFilters * sizeof(IBaseFilter*));
        memcpy(pNewNames, This->pFilterNames, This->nFilters * sizeof(LPWSTR));
        if (This->filterCapacity)
        {
            CoTaskMemFree(This->ppFiltersInGraph);
            CoTaskMemFree(This->pFilterNames);
        }
        This->ppFiltersInGraph = ppNewFilters;
        This->pFilterNames = pNewNames;
        This->filterCapacity = newCapacity;
    }

    hr = IBaseFilter_JoinFilterGraph(pFilter, (IFilterGraph *)This, wszFilterName);

    if (SUCCEEDED(hr))
    {
        IBaseFilter_AddRef(pFilter);
        This->ppFiltersInGraph[This->nFilters] = pFilter;
        This->pFilterNames[This->nFilters] = wszFilterName;
        This->nFilters++;
        IBaseFilter_SetSyncSource(pFilter, This->refClock);
    }
    else
	CoTaskMemFree(wszFilterName);

    if (SUCCEEDED(hr) && duplicate_name)
	return VFW_S_DUPLICATE_NAME;
	
    return hr;
}

static HRESULT WINAPI FilterGraph2_RemoveFilter(IFilterGraph2 *iface, IBaseFilter *pFilter)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IFilterGraph2_vtbl, iface);
    int i;
    HRESULT hr = E_FAIL;

    TRACE("(%p/%p)->(%p)\n", This, iface, pFilter);

    /* FIXME: check graph is stopped */

    for (i = 0; i < This->nFilters; i++)
    {
        if (This->ppFiltersInGraph[i] == pFilter)
        {
            IEnumPins *penumpins = NULL;
            FILTER_STATE state;

            TRACE("Removing filter %s\n", debugstr_w(This->pFilterNames[i]));
            IBaseFilter_GetState(pFilter, 0, &state);
            if (state == State_Running)
                IBaseFilter_Pause(pFilter);
            if (state != State_Stopped)
                IBaseFilter_Stop(pFilter);

            hr = IBaseFilter_EnumPins(pFilter, &penumpins);
            if (SUCCEEDED(hr)) {
                IPin *ppin;
                while(IEnumPins_Next(penumpins, 1, &ppin, NULL) == S_OK)
                {
                    IPin *victim = NULL;
                    HRESULT h;
                    IPin_ConnectedTo(ppin, &victim);
                    if (victim)
                    {
                        h = IPin_Disconnect(victim);
                        TRACE("Disconnect other side: %08x\n", h);
                        if (h == VFW_E_NOT_STOPPED)
                        {
                            PIN_INFO pinfo;
                            IPin_QueryPinInfo(victim, &pinfo);

                            IBaseFilter_GetState(pinfo.pFilter, 0, &state);
                            if (state == State_Running)
                                IBaseFilter_Pause(pinfo.pFilter);
                            IBaseFilter_Stop(pinfo.pFilter);
                            IBaseFilter_Release(pinfo.pFilter);
                            h = IPin_Disconnect(victim);
                            TRACE("Disconnect retry: %08x\n", h);
                        }
                        IPin_Release(victim);
                    }
                    h = IPin_Disconnect(ppin);
                    TRACE("Disconnect 2: %08x\n", h);

                    IPin_Release(ppin);
                }
                IEnumPins_Release(penumpins);
            }

            hr = IBaseFilter_JoinFilterGraph(pFilter, NULL, This->pFilterNames[i]);
            if (SUCCEEDED(hr))
            {
                IBaseFilter_SetSyncSource(pFilter, NULL);
                IBaseFilter_Release(pFilter);
                CoTaskMemFree(This->pFilterNames[i]);
                memmove(This->ppFiltersInGraph+i, This->ppFiltersInGraph+i+1, sizeof(IBaseFilter*)*(This->nFilters - 1 - i));
                memmove(This->pFilterNames+i, This->pFilterNames+i+1, sizeof(LPWSTR)*(This->nFilters - 1 - i));
                This->nFilters--;
                /* Invalidate interfaces in the cache */
                for (i = 0; i < This->nItfCacheEntries; i++)
                    if (pFilter == This->ItfCacheEntries[i].filter)
                    {
                        IUnknown_Release(This->ItfCacheEntries[i].iface);
                        This->ItfCacheEntries[i].iface = NULL;
                        This->ItfCacheEntries[i].filter = NULL;
                    }
                return S_OK;
            }
            break;
        }
    }

    return hr; /* FIXME: check this error code */
}

static HRESULT WINAPI FilterGraph2_EnumFilters(IFilterGraph2 *iface,
					      IEnumFilters **ppEnum) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IFilterGraph2_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, ppEnum);

    return IEnumFiltersImpl_Construct(This->ppFiltersInGraph, This->nFilters, ppEnum);
}

static HRESULT WINAPI FilterGraph2_FindFilterByName(IFilterGraph2 *iface,
						    LPCWSTR pName,
						    IBaseFilter **ppFilter) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IFilterGraph2_vtbl, iface);
    int i;

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_w(pName), pName, ppFilter);

    if (!ppFilter)
        return E_POINTER;

    for (i = 0; i < This->nFilters; i++)
    {
        if (!strcmpW(pName, This->pFilterNames[i]))
        {
            *ppFilter = This->ppFiltersInGraph[i];
            IBaseFilter_AddRef(*ppFilter);
            return S_OK;
        }
    }

    *ppFilter = NULL;
    return VFW_E_NOT_FOUND;
}

/* Don't allow a circular connection to form, return VFW_E_CIRCULAR_GRAPH if this would be the case.
 * A circular connection will be formed if from the filter of the output pin, the input pin can be reached
 */
static HRESULT CheckCircularConnection(IFilterGraphImpl *This, IPin *out, IPin *in)
{
#if 1
    HRESULT hr;
    PIN_INFO info_out, info_in;

    hr = IPin_QueryPinInfo(out, &info_out);
    if (FAILED(hr))
        return hr;
    if (info_out.dir != PINDIR_OUTPUT)
    {
        IBaseFilter_Release(info_out.pFilter);
        return E_UNEXPECTED;
    }

    hr = IPin_QueryPinInfo(in, &info_in);
    if (SUCCEEDED(hr))
        IBaseFilter_Release(info_in.pFilter);
    if (FAILED(hr))
        goto out;
    if (info_in.dir != PINDIR_INPUT)
    {
        hr = E_UNEXPECTED;
        goto out;
    }

    if (info_out.pFilter == info_in.pFilter)
        hr = VFW_E_CIRCULAR_GRAPH;
    else
    {
        IEnumPins *enumpins;
        IPin *test;

        hr = IBaseFilter_EnumPins(info_out.pFilter, &enumpins);
        if (FAILED(hr))
            goto out;

        IEnumPins_Reset(enumpins);
        while ((hr = IEnumPins_Next(enumpins, 1, &test, NULL)) == S_OK)
        {
            PIN_DIRECTION dir = PINDIR_OUTPUT;
            IPin_QueryDirection(test, &dir);
            if (dir == PINDIR_INPUT)
            {
                IPin *victim = NULL;
                IPin_ConnectedTo(test, &victim);
                if (victim)
                {
                    hr = CheckCircularConnection(This, victim, in);
                    IPin_Release(victim);
                    if (FAILED(hr))
                    {
                        IPin_Release(test);
                        break;
                    }
                }
            }
            IPin_Release(test);
        }
        IEnumPins_Release(enumpins);
    }

out:
    IBaseFilter_Release(info_out.pFilter);
    if (FAILED(hr))
        ERR("Checking filtergraph returned %08x, something's not right!\n", hr);
    return hr;
#else
    /* Debugging filtergraphs not enabled */
    return S_OK;
#endif
}


/* NOTE: despite the implication, it doesn't matter which
 * way round you put in the input and output pins */
static HRESULT WINAPI FilterGraph2_ConnectDirect(IFilterGraph2 *iface,
						 IPin *ppinIn,
						 IPin *ppinOut,
						 const AM_MEDIA_TYPE *pmt) {
    PIN_DIRECTION dir;
    HRESULT hr;

    ICOM_THIS_MULTI(IFilterGraphImpl, IFilterGraph2_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p, %p)\n", This, iface, ppinIn, ppinOut, pmt);

    /* FIXME: check pins are in graph */

    if (TRACE_ON(quartz))
    {
        PIN_INFO PinInfo;

        hr = IPin_QueryPinInfo(ppinIn, &PinInfo);
        if (FAILED(hr))
            return hr;

        TRACE("Filter owning first pin => %p\n", PinInfo.pFilter);
        IBaseFilter_Release(PinInfo.pFilter);

        hr = IPin_QueryPinInfo(ppinOut, &PinInfo);
        if (FAILED(hr))
            return hr;

        TRACE("Filter owning second pin => %p\n", PinInfo.pFilter);
        IBaseFilter_Release(PinInfo.pFilter);
    }

    hr = IPin_QueryDirection(ppinIn, &dir);
    if (SUCCEEDED(hr))
    {
        if (dir == PINDIR_INPUT)
        {
            hr = CheckCircularConnection(This, ppinOut, ppinIn);
            if (SUCCEEDED(hr))
                hr = IPin_Connect(ppinOut, ppinIn, pmt);
        }
        else
        {
            hr = CheckCircularConnection(This, ppinIn, ppinOut);
            if (SUCCEEDED(hr))
                hr = IPin_Connect(ppinIn, ppinOut, pmt);
        }
    }

    return hr;
}

static HRESULT WINAPI FilterGraph2_Reconnect(IFilterGraph2 *iface,
					     IPin *ppin) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IFilterGraph2_vtbl, iface);
    IPin *pConnectedTo = NULL;
    HRESULT hr;
    PIN_DIRECTION pindir;

    IPin_QueryDirection(ppin, &pindir);
    hr = IPin_ConnectedTo(ppin, &pConnectedTo);
    if (FAILED(hr)) {
        TRACE("Querying connected to failed: %x\n", hr);
        return hr; 
    }
    IPin_Disconnect(ppin);
    IPin_Disconnect(pConnectedTo);
    if (pindir == PINDIR_INPUT)
        hr = IPin_Connect(pConnectedTo, ppin, NULL);
    else
        hr = IPin_Connect(ppin, pConnectedTo, NULL);
    IPin_Release(pConnectedTo);
    if (FAILED(hr))
        WARN("Reconnecting pins failed, pins are not connected now..\n");
    TRACE("(%p->%p) -- %p %p -> %x\n", iface, This, ppin, pConnectedTo, hr);
    return hr;
}

static HRESULT WINAPI FilterGraph2_Disconnect(IFilterGraph2 *iface, IPin *ppin)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IFilterGraph2_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, ppin);

    if (!ppin)
       return E_POINTER;

    return IPin_Disconnect(ppin);
}

static HRESULT WINAPI FilterGraph2_SetDefaultSyncSource(IFilterGraph2 *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IFilterGraph2_vtbl, iface);
    IReferenceClock *pClock = NULL;
    HRESULT hr;

    TRACE("(%p/%p)->() semi-stub\n", iface, This);

    hr = CoCreateInstance(&CLSID_SystemClock, NULL, CLSCTX_INPROC_SERVER, &IID_IReferenceClock, (LPVOID*)&pClock);

    if (SUCCEEDED(hr))
    {
        hr = IMediaFilter_SetSyncSource((IMediaFilter*)&(This->IMediaFilter_vtbl), pClock);
        IReferenceClock_Release(pClock);
    }

    return hr;
}

static HRESULT GetFilterInfo(IMoniker* pMoniker, GUID* pclsid, VARIANT* pvar)
{
    static const WCHAR wszClsidName[] = {'C','L','S','I','D',0};
    static const WCHAR wszFriendlyName[] = {'F','r','i','e','n','d','l','y','N','a','m','e',0};
    IPropertyBag * pPropBagCat = NULL;
    HRESULT hr;

    VariantInit(pvar);

    hr = IMoniker_BindToStorage(pMoniker, NULL, NULL, &IID_IPropertyBag, (LPVOID*)&pPropBagCat);

    if (SUCCEEDED(hr))
        hr = IPropertyBag_Read(pPropBagCat, wszClsidName, pvar, NULL);

    if (SUCCEEDED(hr))
        hr = CLSIDFromString(V_UNION(pvar, bstrVal), pclsid);

    VariantClear(pvar);

    if (SUCCEEDED(hr))
        hr = IPropertyBag_Read(pPropBagCat, wszFriendlyName, pvar, NULL);

    if (SUCCEEDED(hr))
        TRACE("Moniker = %s - %s\n", debugstr_guid(pclsid), debugstr_w(V_UNION(pvar, bstrVal)));

    if (pPropBagCat)
        IPropertyBag_Release(pPropBagCat);

    return hr;
}

static HRESULT GetInternalConnections(IBaseFilter* pfilter, IPin* pinputpin, IPin*** pppins, ULONG* pnb)
{
    HRESULT hr;
    ULONG nb = 0;

    TRACE("(%p, %p, %p, %p)\n", pfilter, pinputpin, pppins, pnb);
    hr = IPin_QueryInternalConnections(pinputpin, NULL, &nb);
    if (hr == S_OK) {
        /* Rendered input */
    } else if (hr == S_FALSE) {
        *pppins = CoTaskMemAlloc(sizeof(IPin*)*nb);
        hr = IPin_QueryInternalConnections(pinputpin, *pppins, &nb);
        if (hr != S_OK) {
            WARN("Error (%x)\n", hr);
        }
    } else if (hr == E_NOTIMPL) {
        /* Input connected to all outputs */
        IEnumPins* penumpins;
        IPin* ppin;
        int i = 0;
        TRACE("E_NOTIMPL\n");
        hr = IBaseFilter_EnumPins(pfilter, &penumpins);
        if (FAILED(hr)) {
            WARN("filter Enumpins failed (%x)\n", hr);
            return hr;
        }
        i = 0;
        /* Count output pins */
        while(IEnumPins_Next(penumpins, 1, &ppin, &nb) == S_OK) {
            PIN_DIRECTION pindir;
            IPin_QueryDirection(ppin, &pindir);
            if (pindir == PINDIR_OUTPUT)
                i++;
            IPin_Release(ppin);
        }
        *pppins = CoTaskMemAlloc(sizeof(IPin*)*i);
        /* Retrieve output pins */
        IEnumPins_Reset(penumpins);
        i = 0;
        while(IEnumPins_Next(penumpins, 1, &ppin, &nb) == S_OK) {
            PIN_DIRECTION pindir;
            IPin_QueryDirection(ppin, &pindir);
            if (pindir == PINDIR_OUTPUT)
                (*pppins)[i++] = ppin;
            else
                IPin_Release(ppin);
        }
        IEnumPins_Release(penumpins);
        nb = i;
        if (FAILED(hr)) {
            WARN("Next failed (%x)\n", hr);
            return hr;
        }
    } else if (FAILED(hr)) {
        WARN("Cannot get internal connection (%x)\n", hr);
        return hr;
    }

    *pnb = nb;
    return S_OK;
}

/*** IGraphBuilder methods ***/
static HRESULT WINAPI FilterGraph2_Connect(IFilterGraph2 *iface, IPin *ppinOut, IPin *ppinIn)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IFilterGraph2_vtbl, iface);
    HRESULT hr;
    AM_MEDIA_TYPE* mt = NULL;
    IEnumMediaTypes* penummt = NULL;
    ULONG nbmt;
    IEnumPins* penumpins;
    IEnumMoniker* pEnumMoniker;
    GUID tab[2];
    ULONG nb;
    IMoniker* pMoniker;
    ULONG pin;
    PIN_INFO PinInfo;
    CLSID FilterCLSID;
    PIN_DIRECTION dir;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, ppinOut, ppinIn);

    if (TRACE_ON(quartz))
    {
        hr = IPin_QueryPinInfo(ppinIn, &PinInfo);
        if (FAILED(hr))
            return hr;

        TRACE("Filter owning first pin => %p\n", PinInfo.pFilter);
        IBaseFilter_Release(PinInfo.pFilter);

        hr = IPin_QueryPinInfo(ppinOut, &PinInfo);
        if (FAILED(hr))
            return hr;

        TRACE("Filter owning second pin => %p\n", PinInfo.pFilter);
        IBaseFilter_Release(PinInfo.pFilter);
    }

    EnterCriticalSection(&This->cs);
    ++This->recursioncount;
    if (This->recursioncount >= 5)
    {
        WARN("Recursion count has reached %d\n", This->recursioncount);
        hr = VFW_E_CANNOT_CONNECT;
        goto out;
    }

    hr = IPin_QueryDirection(ppinOut, &dir);
    if (FAILED(hr))
        goto out;

    if (dir == PINDIR_INPUT)
    {
        IPin *temp;

        temp = ppinIn;
        ppinIn = ppinOut;
        ppinOut = temp;
    }

    hr = CheckCircularConnection(This, ppinOut, ppinIn);
    if (FAILED(hr))
        goto out;

    /* Try direct connection first */
    hr = IPin_Connect(ppinOut, ppinIn, NULL);
    if (SUCCEEDED(hr))
        goto out;

    TRACE("Direct connection failed, trying to render using extra filters\n");

    hr = IPin_QueryPinInfo(ppinIn, &PinInfo);
    if (FAILED(hr))
        goto out;

    hr = IBaseFilter_GetClassID(PinInfo.pFilter, &FilterCLSID);
    IBaseFilter_Release(PinInfo.pFilter);
    if (FAILED(hr))
        goto out;

    /* Find the appropriate transform filter than can transform the minor media type of output pin of the upstream 
     * filter to the minor mediatype of input pin of the renderer */
    hr = IPin_EnumMediaTypes(ppinOut, &penummt);
    if (FAILED(hr))
    {
        WARN("EnumMediaTypes (%x)\n", hr);
        goto out;
    }

    hr = IEnumMediaTypes_Next(penummt, 1, &mt, &nbmt);
    if (FAILED(hr)) {
        WARN("IEnumMediaTypes_Next (%x)\n", hr);
        goto out;
    }

    if (!nbmt)
    {
        WARN("No media type found!\n");
        hr = VFW_E_INVALIDMEDIATYPE;
        goto out;
    }
    TRACE("MajorType %s\n", debugstr_guid(&mt->majortype));
    TRACE("SubType %s\n", debugstr_guid(&mt->subtype));

    /* Try to find a suitable filter that can connect to the pin to render */
    tab[0] = mt->majortype;
    tab[1] = mt->subtype;
    hr = IFilterMapper2_EnumMatchingFilters(This->pFilterMapper2, &pEnumMoniker, 0, FALSE, MERIT_UNLIKELY, TRUE, 1, tab, NULL, NULL, FALSE, FALSE, 0, NULL, NULL, NULL);
    if (FAILED(hr)) {
        WARN("Unable to enum filters (%x)\n", hr);
        goto out;
    }

    hr = VFW_E_CANNOT_RENDER;
    while(IEnumMoniker_Next(pEnumMoniker, 1, &pMoniker, &nb) == S_OK)
    {
        VARIANT var;
        GUID clsid;
        IPin** ppins;
        IPin* ppinfilter = NULL;
        IBaseFilter* pfilter = NULL;

        hr = GetFilterInfo(pMoniker, &clsid, &var);
        IMoniker_Release(pMoniker);
        if (FAILED(hr)) {
            WARN("Unable to retrieve filter info (%x)\n", hr);
            goto error;
        }

        if (IsEqualGUID(&clsid, &FilterCLSID)) {
            /* Skip filter (same as the one the output pin belongs to) */
            goto error;
        }

        hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (LPVOID*)&pfilter);
        if (FAILED(hr)) {
            WARN("Unable to create filter (%x), trying next one\n", hr);
            goto error;
        }

        hr = IFilterGraph2_AddFilter(iface, pfilter, V_UNION(&var, bstrVal));
        if (FAILED(hr)) {
            WARN("Unable to add filter (%x)\n", hr);
            IBaseFilter_Release(pfilter);
            pfilter = NULL;
            goto error;
        }

        VariantClear(&var);

        hr = IBaseFilter_EnumPins(pfilter, &penumpins);
        if (FAILED(hr)) {
            WARN("Enumpins (%x)\n", hr);
            goto error;
        }

        hr = IEnumPins_Next(penumpins, 1, &ppinfilter, &pin);
        IEnumPins_Release(penumpins);

        if (FAILED(hr)) {
            WARN("Obtaining next pin: (%x)\n", hr);
            goto error;
        }
        if (pin == 0) {
            WARN("Cannot use this filter: no pins\n");
            goto error;
        }

        hr = IPin_Connect(ppinOut, ppinfilter, NULL);
        if (FAILED(hr)) {
            TRACE("Cannot connect to filter (%x), trying next one\n", hr);
            goto error;
        }
        TRACE("Successfully connected to filter, follow chain...\n");

        /* Render all output pins of the filter by calling IFilterGraph2_Connect on each of them */
        hr = GetInternalConnections(pfilter, ppinfilter, &ppins, &nb);

        if (SUCCEEDED(hr)) {
            unsigned int i;
            if (nb == 0) {
                IPin_Disconnect(ppinfilter);
                IPin_Disconnect(ppinOut);
                goto error;
            }
            TRACE("pins to consider: %d\n", nb);
            for(i = 0; i < nb; i++)
            {
                LPWSTR pinname = NULL;

                TRACE("Processing pin %u\n", i);

                hr = IPin_QueryId(ppins[i], &pinname);
                if (SUCCEEDED(hr))
                {
                    if (pinname[0] == '~')
                    {
                        TRACE("Pinname=%s, skipping\n", debugstr_w(pinname));
                        hr = E_FAIL;
                    }
                    else
                        hr = IFilterGraph2_Connect(iface, ppins[i], ppinIn);
                    CoTaskMemFree(pinname);
                }

                if (FAILED(hr)) {
                   TRACE("Cannot connect pin %p (%x)\n", ppinfilter, hr);
                }
                IPin_Release(ppins[i]);
                if (SUCCEEDED(hr)) break;
            }
            while (++i < nb) IPin_Release(ppins[i]);
            CoTaskMemFree(ppins);
            IPin_Release(ppinfilter);
            IBaseFilter_Release(pfilter);
            if (FAILED(hr))
            {
                IPin_Disconnect(ppinfilter);
                IPin_Disconnect(ppinOut);
                IFilterGraph2_RemoveFilter(iface, pfilter);
                continue;
            }
            break;
        }

error:
        VariantClear(&var);
        if (ppinfilter) IPin_Release(ppinfilter);
        if (pfilter) {
            IFilterGraph2_RemoveFilter(iface, pfilter);
            IBaseFilter_Release(pfilter);
        }
    }

out:
    if (penummt)
        IEnumMediaTypes_Release(penummt);
    if (mt)
        DeleteMediaType(mt);
    --This->recursioncount;
    LeaveCriticalSection(&This->cs);
    TRACE("--> %08x\n", hr);
    return SUCCEEDED(hr) ? S_OK : hr;
}

static HRESULT FilterGraph2_RenderRecurse(IFilterGraphImpl *This, IPin *ppinOut)
{
    /* This pin has been connected now, try to call render on all pins that aren't connected */
    IPin *to = NULL;
    PIN_INFO info;
    IEnumPins *enumpins = NULL;
    BOOL renderany = FALSE;
    BOOL renderall = TRUE;

    IPin_QueryPinInfo(ppinOut, &info);

    IBaseFilter_EnumPins(info.pFilter, &enumpins);
    /* Don't need to hold a reference, IEnumPins does */
    IBaseFilter_Release(info.pFilter);

    IEnumPins_Reset(enumpins);
    while (IEnumPins_Next(enumpins, 1, &to, NULL) == S_OK)
    {
        PIN_DIRECTION dir = PINDIR_INPUT;

        IPin_QueryDirection(to, &dir);

        if (dir == PINDIR_OUTPUT)
        {
            IPin *out = NULL;

            IPin_ConnectedTo(to, &out);
            if (!out)
            {
                HRESULT hr;
                hr = IFilterGraph2_Render((IFilterGraph2 *)&This->IFilterGraph2_vtbl, to);
                if (SUCCEEDED(hr))
                    renderany = TRUE;
                else
                    renderall = FALSE;
            }
            else
                IPin_Release(out);
        }

        IPin_Release(to);
    }

    IEnumPins_Release(enumpins);

    if (renderall)
        return S_OK;

    if (renderany)
        return VFW_S_PARTIAL_RENDER;

    return VFW_E_CANNOT_RENDER;
}

/* Ogg hates me if I create a direct rendering method
 *
 * It can only connect to a pin properly once, so use a recursive method that does
 *
 *  +----+ --- (PIN 1) (Render is called on this pin)
 *  |    |
 *  +----+ --- (PIN 2)
 *
 *  Enumerate possible renderers that EXACTLY match the requested type
 *
 *  If none is available, try to add intermediate filters that can connect to the input pin
 *  then call Render on that intermediate pin's output pins
 *  if it succeeds: Render returns success, if it doesn't, the intermediate filter is removed,
 *  and another filter that can connect to the input pin is tried
 *  if we run out of filters that can, give up and return VFW_E_CANNOT_RENDER
 *  It's recursive, but fun!
 */

static HRESULT WINAPI FilterGraph2_Render(IFilterGraph2 *iface, IPin *ppinOut)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IFilterGraph2_vtbl, iface);
    IEnumMediaTypes* penummt;
    AM_MEDIA_TYPE* mt;
    ULONG nbmt;
    HRESULT hr;

    IEnumMoniker* pEnumMoniker;
    GUID tab[4];
    ULONG nb;
    IMoniker* pMoniker;
    INT x;

    TRACE("(%p/%p)->(%p)\n", This, iface, ppinOut);

    if (TRACE_ON(quartz))
    {
        PIN_INFO PinInfo;

        hr = IPin_QueryPinInfo(ppinOut, &PinInfo);
        if (FAILED(hr))
            return hr;

        TRACE("Filter owning pin => %p\n", PinInfo.pFilter);
        IBaseFilter_Release(PinInfo.pFilter);
    }

    /* Try to find out if there is a renderer for the specified subtype already, and use that
     */
    EnterCriticalSection(&This->cs);
    for (x = 0; x < This->nFilters; ++x)
    {
        IEnumPins *enumpins = NULL;
        IPin *pin = NULL;

        hr = IBaseFilter_EnumPins(This->ppFiltersInGraph[x], &enumpins);

        if (FAILED(hr) || !enumpins)
            continue;

        IEnumPins_Reset(enumpins);
        while (IEnumPins_Next(enumpins, 1, &pin, NULL) == S_OK)
        {
            IPin *to = NULL;
            PIN_DIRECTION dir = PINDIR_OUTPUT;

            IPin_QueryDirection(pin, &dir);
            if (dir != PINDIR_INPUT)
            {
                IPin_Release(pin);
                continue;
            }
            IPin_ConnectedTo(pin, &to);

            if (to == NULL)
            {
                hr = IPin_Connect(ppinOut, pin, NULL);
                if (SUCCEEDED(hr))
                {
                    TRACE("Connected successfully %p/%p, %08x look if we should render more!\n", ppinOut, pin, hr);
                    IPin_Release(pin);

                    hr = FilterGraph2_RenderRecurse(This, pin);
                    if (FAILED(hr))
                    {
                        IPin_Disconnect(ppinOut);
                        IPin_Disconnect(pin);
                        continue;
                    }
                    IEnumPins_Release(enumpins);
                    LeaveCriticalSection(&This->cs);
                    return hr;
                }
                WARN("Could not connect!\n");
            }
            else
                IPin_Release(to);

            IPin_Release(pin);
        }
        IEnumPins_Release(enumpins);
    }

    LeaveCriticalSection(&This->cs);

    hr = IPin_EnumMediaTypes(ppinOut, &penummt);
    if (FAILED(hr)) {
        WARN("EnumMediaTypes (%x)\n", hr);
        return hr;
    }

    IEnumMediaTypes_Reset(penummt);

    /* Looks like no existing renderer of the kind exists
     * Try adding new ones
     */
    tab[0] = tab[1] = GUID_NULL;
    while (SUCCEEDED(hr))
    {
        hr = IEnumMediaTypes_Next(penummt, 1, &mt, &nbmt);
        if (FAILED(hr)) {
            WARN("IEnumMediaTypes_Next (%x)\n", hr);
            break;
        }
        if (!nbmt)
        {
            hr = VFW_E_CANNOT_RENDER;
            break;
        }
        else
        {
            TRACE("MajorType %s\n", debugstr_guid(&mt->majortype));
            TRACE("SubType %s\n", debugstr_guid(&mt->subtype));

            /* Only enumerate once, this doesn't account for all previous ones, but this should be enough nonetheless */
            if (IsEqualIID(&tab[0], &mt->majortype) && IsEqualIID(&tab[1], &mt->subtype))
            {
                DeleteMediaType(mt);
                continue;
            }

            /* Try to find a suitable renderer with the same media type */
            tab[0] = mt->majortype;
            tab[1] = mt->subtype;
            hr = IFilterMapper2_EnumMatchingFilters(This->pFilterMapper2, &pEnumMoniker, 0, FALSE, MERIT_UNLIKELY, TRUE, 1, tab, NULL, NULL, FALSE, FALSE, 0, NULL, NULL, NULL);
            if (FAILED(hr))
            {
                WARN("Unable to enum filters (%x)\n", hr);
                break;
            }
        }
        hr = E_FAIL;

        while (IEnumMoniker_Next(pEnumMoniker, 1, &pMoniker, &nb) == S_OK)
        {
            VARIANT var;
            GUID clsid;
            IPin* ppinfilter;
            IBaseFilter* pfilter = NULL;
            IEnumPins* penumpins = NULL;
            ULONG pin;

            hr = GetFilterInfo(pMoniker, &clsid, &var);
            IMoniker_Release(pMoniker);
            if (FAILED(hr)) {
                WARN("Unable to retrieve filter info (%x)\n", hr);
                goto error;
            }

            hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (LPVOID*)&pfilter);
            if (FAILED(hr))
            {
                WARN("Unable to create filter (%x), trying next one\n", hr);
                goto error;
            }

            hr = IFilterGraph2_AddFilter(iface, pfilter, V_UNION(&var, bstrVal));
            if (FAILED(hr)) {
                WARN("Unable to add filter (%x)\n", hr);
                IBaseFilter_Release(pfilter);
                pfilter = NULL;
                goto error;
            }

            hr = IBaseFilter_EnumPins(pfilter, &penumpins);
            if (FAILED(hr)) {
                WARN("Splitter Enumpins (%x)\n", hr);
                goto error;
            }

            while ((hr = IEnumPins_Next(penumpins, 1, &ppinfilter, &pin)) == S_OK)
            {
                PIN_DIRECTION dir;

                if (pin == 0) {
                    WARN("No Pin\n");
                    hr = E_FAIL;
                    goto error;
                }

                hr = IPin_QueryDirection(ppinfilter, &dir);
                if (FAILED(hr)) {
                    IPin_Release(ppinfilter);
                    WARN("QueryDirection failed (%x)\n", hr);
                    goto error;
                }
                if (dir != PINDIR_INPUT) {
                    IPin_Release(ppinfilter);
                    continue; /* Wrong direction */
                }

                /* Connect the pin to the "Renderer" */
                hr = IPin_Connect(ppinOut, ppinfilter, NULL);
                IPin_Release(ppinfilter);

                if (FAILED(hr)) {
                    WARN("Unable to connect %s to renderer (%x)\n", debugstr_w(V_UNION(&var, bstrVal)), hr);
                    goto error;
                }
                TRACE("Connected, recursing %s\n",  debugstr_w(V_UNION(&var, bstrVal)));

                VariantClear(&var);

                hr = FilterGraph2_RenderRecurse(This, ppinfilter);
                if (FAILED(hr)) {
                    WARN("Unable to connect recursively (%x)\n", hr);
                    goto error;
                }
                IBaseFilter_Release(pfilter);
                break;
            }
            if (SUCCEEDED(hr)) {
                IEnumPins_Release(penumpins);
                break; /* out of IEnumMoniker_Next loop */
            }

            /* IEnumPins_Next failed, all other failure case caught by goto error */
            WARN("IEnumPins_Next (%x)\n", hr);
            /* goto error */

error:
            VariantClear(&var);
            if (penumpins)
                IEnumPins_Release(penumpins);
            if (pfilter) {
                IFilterGraph2_RemoveFilter(iface, pfilter);
                IBaseFilter_Release(pfilter);
            }
            if (SUCCEEDED(hr)) DebugBreak();
        }

        IEnumMoniker_Release(pEnumMoniker);
        if (nbmt)
            DeleteMediaType(mt);
        if (SUCCEEDED(hr))
            break;
        hr = S_OK;
    }

    IEnumMediaTypes_Release(penummt);
    return hr;
}

static HRESULT WINAPI FilterGraph2_RenderFile(IFilterGraph2 *iface,
                                              LPCWSTR lpcwstrFile,
                                              LPCWSTR lpcwstrPlayList)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IFilterGraph2_vtbl, iface);
    static const WCHAR string[] = {'R','e','a','d','e','r',0};
    IBaseFilter* preader = NULL;
    IPin* ppinreader = NULL;
    IEnumPins* penumpins = NULL;
    HRESULT hr;
    BOOL partial = FALSE;
    HRESULT any = FALSE;

    TRACE("(%p/%p)->(%s, %s)\n", This, iface, debugstr_w(lpcwstrFile), debugstr_w(lpcwstrPlayList));

    if (lpcwstrPlayList != NULL)
        return E_INVALIDARG;

    hr = IFilterGraph2_AddSourceFilter(iface, lpcwstrFile, string, &preader);
    if (FAILED(hr))
        return hr;

    if (SUCCEEDED(hr))
        hr = IBaseFilter_EnumPins(preader, &penumpins);
    if (SUCCEEDED(hr))
    {
        while (IEnumPins_Next(penumpins, 1, &ppinreader, NULL) == S_OK)
        {
            PIN_DIRECTION dir;

            IPin_QueryDirection(ppinreader, &dir);
            if (dir == PINDIR_OUTPUT)
            {
                INT i;

                hr = IFilterGraph2_Render(iface, ppinreader);
                TRACE("Render %08x\n", hr);

                for (i = 0; i < This->nFilters; ++i)
                    TRACE("Filters in chain: %s\n", debugstr_w(This->pFilterNames[i]));

                if (SUCCEEDED(hr))
                    any = TRUE;
                if (hr != S_OK)
                    partial = TRUE;
            }
            IPin_Release(ppinreader);
        }
        IEnumPins_Release(penumpins);

        if (!any)
            hr = VFW_E_CANNOT_RENDER;
        else if (partial)
            hr = VFW_S_PARTIAL_RENDER;
        else
            hr = S_OK;
    }
    IBaseFilter_Release(preader);

    TRACE("--> %08x\n", hr);
    return hr;
}

/* Some filters implement their own asynchronous reader (Theoretically they all should, try to load it first */
static HRESULT GetFileSourceFilter(LPCOLESTR pszFileName, IBaseFilter **filter)
{
    static const WCHAR wszReg[] = {'M','e','d','i','a',' ','T','y','p','e','\\','E','x','t','e','n','s','i','o','n','s',0};
    HRESULT hr = S_OK;
    HKEY extkey;
    LONG lRet;

    lRet = RegOpenKeyExW(HKEY_CLASSES_ROOT, wszReg, 0, KEY_READ, &extkey);
    hr = HRESULT_FROM_WIN32(lRet);

    if (SUCCEEDED(hr))
    {
        static const WCHAR filtersource[] = {'S','o','u','r','c','e',' ','F','i','l','t','e','r',0};
        WCHAR *ext = PathFindExtensionW(pszFileName);
        WCHAR clsid_key[39];
        GUID clsid;
        DWORD size = sizeof(clsid_key);
        HKEY pathkey;

        if (!ext)
        {
            CloseHandle(extkey);
            return E_FAIL;
        }

        lRet = RegOpenKeyExW(extkey, ext, 0, KEY_READ, &pathkey);
        hr = HRESULT_FROM_WIN32(lRet);
        CloseHandle(extkey);
        if (FAILED(hr))
            return hr;

        lRet = RegQueryValueExW(pathkey, filtersource, NULL, NULL, (LPBYTE)clsid_key, &size);
        hr = HRESULT_FROM_WIN32(lRet);
        CloseHandle(pathkey);
        if (FAILED(hr))
            return hr;

        CLSIDFromString(clsid_key, &clsid);

        TRACE("CLSID: %s\n", debugstr_guid(&clsid));
        hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (LPVOID*)filter);
        if (SUCCEEDED(hr))
        {
            IFileSourceFilter *source = NULL;
            hr = IBaseFilter_QueryInterface(*filter, &IID_IFileSourceFilter, (LPVOID*)&source);
            if (SUCCEEDED(hr))
                IFileSourceFilter_Release(source);
            else
                IBaseFilter_Release(*filter);
        }
    }
    if (FAILED(hr))
        *filter = NULL;
    return hr;
}

static HRESULT WINAPI FilterGraph2_AddSourceFilter(IFilterGraph2 *iface,
						   LPCWSTR lpcwstrFileName,
						   LPCWSTR lpcwstrFilterName,
						   IBaseFilter **ppFilter) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IFilterGraph2_vtbl, iface);
    HRESULT hr;
    IBaseFilter* preader;
    IFileSourceFilter* pfile = NULL;
    AM_MEDIA_TYPE mt;
    WCHAR* filename;

    TRACE("(%p/%p)->(%s, %s, %p)\n", This, iface, debugstr_w(lpcwstrFileName), debugstr_w(lpcwstrFilterName), ppFilter);

    /* Try from file name first, then fall back to default asynchronous reader */
    hr = GetFileSourceFilter(lpcwstrFileName, &preader);

    if (FAILED(hr))
        hr = CoCreateInstance(&CLSID_AsyncReader, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (LPVOID*)&preader);
    if (FAILED(hr)) {
        WARN("Unable to create file source filter (%x)\n", hr);
        return hr;
    }

    hr = IFilterGraph2_AddFilter(iface, preader, lpcwstrFilterName);
    if (FAILED(hr)) {
        WARN("Unable add filter (%x)\n", hr);
        IBaseFilter_Release(preader);
        return hr;
    }

    hr = IBaseFilter_QueryInterface(preader, &IID_IFileSourceFilter, (LPVOID*)&pfile);
    if (FAILED(hr)) {
        WARN("Unable to get IFileSourceInterface (%x)\n", hr);
        goto error;
    }

    /* Load the file in the file source filter */
    hr = IFileSourceFilter_Load(pfile, lpcwstrFileName, NULL);
    if (FAILED(hr)) {
        WARN("Load (%x)\n", hr);
        goto error;
    }

    IFileSourceFilter_GetCurFile(pfile, &filename, &mt);
    if (FAILED(hr)) {
        WARN("GetCurFile (%x)\n", hr);
        goto error;
    }

    TRACE("File %s\n", debugstr_w(filename));
    TRACE("MajorType %s\n", debugstr_guid(&mt.majortype));
    TRACE("SubType %s\n", debugstr_guid(&mt.subtype));

    if (ppFilter)
        *ppFilter = preader;
    IFileSourceFilter_Release(pfile);

    return S_OK;
    
error:
    if (pfile)
        IFileSourceFilter_Release(pfile);
    IFilterGraph2_RemoveFilter(iface, preader);
    IBaseFilter_Release(preader);
       
    return hr;
}

static HRESULT WINAPI FilterGraph2_SetLogFile(IFilterGraph2 *iface,
					      DWORD_PTR hFile) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IFilterGraph2_vtbl, iface);

    TRACE("(%p/%p)->(%08x): stub !!!\n", This, iface, (DWORD) hFile);

    return S_OK;
}

static HRESULT WINAPI FilterGraph2_Abort(IFilterGraph2 *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IFilterGraph2_vtbl, iface);

    TRACE("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}

static HRESULT WINAPI FilterGraph2_ShouldOperationContinue(IFilterGraph2 *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IFilterGraph2_vtbl, iface);

    TRACE("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}

/*** IFilterGraph2 methods ***/
static HRESULT WINAPI FilterGraph2_AddSourceFilterForMoniker(IFilterGraph2 *iface,
                                                             IMoniker *pMoniker,
                                                             IBindCtx *pCtx,
                                                             LPCWSTR lpcwstrFilterName,
                                                             IBaseFilter **ppFilter) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IFilterGraph2_vtbl, iface);

    TRACE("(%p/%p)->(%p %p %s %p): stub !!!\n", This, iface, pMoniker, pCtx, debugstr_w(lpcwstrFilterName), ppFilter);

    return S_OK;
}

static HRESULT WINAPI FilterGraph2_ReconnectEx(IFilterGraph2 *iface,
                                               IPin *ppin,
                                               const AM_MEDIA_TYPE *pmt) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IFilterGraph2_vtbl, iface);

    TRACE("(%p/%p)->(%p %p): stub !!!\n", This, iface, ppin, pmt);

    return S_OK;
}

static HRESULT WINAPI FilterGraph2_RenderEx(IFilterGraph2 *iface,
                                            IPin *pPinOut,
                                            DWORD dwFlags,
                                            DWORD *pvContext) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IFilterGraph2_vtbl, iface);

    TRACE("(%p/%p)->(%p %08x %p): stub !!!\n", This, iface, pPinOut, dwFlags, pvContext);

    return S_OK;
}


static const IFilterGraph2Vtbl IFilterGraph2_VTable =
{
    FilterGraph2_QueryInterface,
    FilterGraph2_AddRef,
    FilterGraph2_Release,
    FilterGraph2_AddFilter,
    FilterGraph2_RemoveFilter,
    FilterGraph2_EnumFilters,
    FilterGraph2_FindFilterByName,
    FilterGraph2_ConnectDirect,
    FilterGraph2_Reconnect,
    FilterGraph2_Disconnect,
    FilterGraph2_SetDefaultSyncSource,
    FilterGraph2_Connect,
    FilterGraph2_Render,
    FilterGraph2_RenderFile,
    FilterGraph2_AddSourceFilter,
    FilterGraph2_SetLogFile,
    FilterGraph2_Abort,
    FilterGraph2_ShouldOperationContinue,
    FilterGraph2_AddSourceFilterForMoniker,
    FilterGraph2_ReconnectEx,
    FilterGraph2_RenderEx
};

/*** IUnknown methods ***/
static HRESULT WINAPI MediaControl_QueryInterface(IMediaControl *iface,
						  REFIID riid,
						  LPVOID*ppvObj) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return Filtergraph_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI MediaControl_AddRef(IMediaControl *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI MediaControl_Release(IMediaControl *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_Release(This);

}

/*** IDispatch methods ***/
static HRESULT WINAPI MediaControl_GetTypeInfoCount(IMediaControl *iface,
						    UINT*pctinfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pctinfo);

    return S_OK;
}

static HRESULT WINAPI MediaControl_GetTypeInfo(IMediaControl *iface,
					       UINT iTInfo,
					       LCID lcid,
					       ITypeInfo**ppTInfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%d, %d, %p): stub !!!\n", This, iface, iTInfo, lcid, ppTInfo);

    return S_OK;
}

static HRESULT WINAPI MediaControl_GetIDsOfNames(IMediaControl *iface,
						 REFIID riid,
						 LPOLESTR*rgszNames,
						 UINT cNames,
						 LCID lcid,
						 DISPID*rgDispId) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p, %d, %d, %p): stub !!!\n", This, iface, debugstr_guid(riid), riid, rgszNames, cNames, lcid, rgDispId);

    return S_OK;
}

static HRESULT WINAPI MediaControl_Invoke(IMediaControl *iface,
					  DISPID dispIdMember,
					  REFIID riid,
					  LCID lcid,
					  WORD wFlags,
					  DISPPARAMS*pDispParams,
					  VARIANT*pVarResult,
					  EXCEPINFO*pExepInfo,
					  UINT*puArgErr) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%d, %s (%p), %d, %04x, %p, %p, %p, %p): stub !!!\n", This, iface, dispIdMember, debugstr_guid(riid), riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    return S_OK;
}

typedef HRESULT(WINAPI *fnFoundFilter)(IBaseFilter *, DWORD_PTR data);

static HRESULT ExploreGraph(IFilterGraphImpl* pGraph, IPin* pOutputPin, fnFoundFilter FoundFilter, DWORD_PTR data)
{
    HRESULT hr;
    IPin* pInputPin;
    IPin** ppPins;
    ULONG nb;
    ULONG i;
    PIN_INFO PinInfo;

    TRACE("%p %p\n", pGraph, pOutputPin);
    PinInfo.pFilter = NULL;

    hr = IPin_ConnectedTo(pOutputPin, &pInputPin);

    if (SUCCEEDED(hr))
    {
        hr = IPin_QueryPinInfo(pInputPin, &PinInfo);
        if (SUCCEEDED(hr))
            hr = GetInternalConnections(PinInfo.pFilter, pInputPin, &ppPins, &nb);
        IPin_Release(pInputPin);
    }

    if (SUCCEEDED(hr))
    {
        if (nb == 0)
        {
            TRACE("Reached a renderer\n");
            /* Count renderers for end of stream notification */
            pGraph->nRenderers++;
        }
        else
        {
            for(i = 0; i < nb; i++)
            {
                /* Explore the graph downstream from this pin
		 * FIXME: We should prevent exploring from a pin more than once. This can happens when
		 * several input pins are connected to the same output (a MUX for instance). */
                ExploreGraph(pGraph, ppPins[i], FoundFilter, data);
                IPin_Release(ppPins[i]);
            }

            CoTaskMemFree(ppPins);
        }
        TRACE("Doing stuff with filter %p\n", PinInfo.pFilter);

        FoundFilter(PinInfo.pFilter, data);
    }

    if (PinInfo.pFilter) IBaseFilter_Release(PinInfo.pFilter);
    return hr;
}

static HRESULT WINAPI SendRun(IBaseFilter *pFilter, DWORD_PTR data)
{
    LONGLONG time = 0;
    IReferenceClock *clock = NULL;

    IBaseFilter_GetSyncSource(pFilter, &clock);
    if (clock)
    {
        IReferenceClock_GetTime(clock, &time);
        if (time)
            /* Add 50 ms */
            time += 500000;
        if (time < 0)
            time = 0;
        IReferenceClock_Release(clock);
    }

    return IBaseFilter_Run(pFilter, time);
}

static HRESULT WINAPI SendPause(IBaseFilter *pFilter, DWORD_PTR data)
{
    return IBaseFilter_Pause(pFilter);
}

static HRESULT WINAPI SendStop(IBaseFilter *pFilter, DWORD_PTR data)
{
    return IBaseFilter_Stop(pFilter);
}

static HRESULT WINAPI SendGetState(IBaseFilter *pFilter, DWORD_PTR data)
{
    FILTER_STATE state;
    DWORD time_end = data;
    DWORD time_now = GetTickCount();
    LONG wait;

    if (time_end == INFINITE)
    {
        wait = INFINITE;
    }
    else if (time_end > time_now)
    {
        wait = time_end - time_now;
    }
    else
        wait = 0;

    return IBaseFilter_GetState(pFilter, wait, &state);
}


static HRESULT SendFilterMessage(IMediaControl *iface, fnFoundFilter FoundFilter, DWORD_PTR data)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);
    int i;
    IBaseFilter* pfilter;
    IEnumPins* pEnum;
    HRESULT hr;
    IPin* pPin;
    DWORD dummy;
    PIN_DIRECTION dir;
    TRACE("(%p/%p)->()\n", This, iface);

    /* Explorer the graph from source filters to renderers, determine renderers
     * number and run filters from renderers to source filters */
    This->nRenderers = 0;
    ResetEvent(This->hEventCompletion);

    for(i = 0; i < This->nFilters; i++)
    {
        BOOL source = TRUE;
        pfilter = This->ppFiltersInGraph[i];
        hr = IBaseFilter_EnumPins(pfilter, &pEnum);
        if (hr != S_OK)
        {
            WARN("Enum pins failed %x\n", hr);
            continue;
        }
        /* Check if it is a source filter */
        while(IEnumPins_Next(pEnum, 1, &pPin, &dummy) == S_OK)
        {
            IPin_QueryDirection(pPin, &dir);
            IPin_Release(pPin);
            if (dir == PINDIR_INPUT)
            {
                source = FALSE;
                break;
            }
        }
        if (source)
        {
            TRACE("Found a source filter %p\n", pfilter);
            IEnumPins_Reset(pEnum);
            while(IEnumPins_Next(pEnum, 1, &pPin, &dummy) == S_OK)
            {
                /* Explore the graph downstream from this pin */
                ExploreGraph(This, pPin, FoundFilter, data);
                IPin_Release(pPin);
            }
            FoundFilter(pfilter, data);
        }
        IEnumPins_Release(pEnum);
    }

    return S_FALSE;
}

/*** IMediaControl methods ***/
static HRESULT WINAPI MediaControl_Run(IMediaControl *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);
    TRACE("(%p/%p)->()\n", This, iface);

    if (This->state == State_Running) return S_OK;

    EnterCriticalSection(&This->cs);
    if (This->state == State_Stopped)
        This->EcCompleteCount = 0;

    if (This->refClock)
    {
        IReferenceClock_GetTime(This->refClock, &This->start_time);
        This->start_time += 500000;
    }
    else This->position = This->start_time = 0;

    SendFilterMessage(iface, SendRun, 0);
    This->state = State_Running;
    LeaveCriticalSection(&This->cs);
    return S_FALSE;
}

static HRESULT WINAPI MediaControl_Pause(IMediaControl *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);
    TRACE("(%p/%p)->()\n", This, iface);

    if (This->state == State_Paused) return S_OK;

    EnterCriticalSection(&This->cs);
    if (This->state == State_Stopped)
        This->EcCompleteCount = 0;

    if (This->state == State_Running && This->refClock)
    {
        LONGLONG time = This->start_time;
        IReferenceClock_GetTime(This->refClock, &time);
        This->position += time - This->start_time;
    }

    SendFilterMessage(iface, SendPause, 0);
    This->state = State_Paused;
    LeaveCriticalSection(&This->cs);
    return S_FALSE;
}

static HRESULT WINAPI MediaControl_Stop(IMediaControl *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);
    TRACE("(%p/%p)->()\n", This, iface);

    if (This->state == State_Stopped) return S_OK;

    EnterCriticalSection(&This->cs);
    if (This->state == State_Running && This->refClock)
    {
        LONGLONG time = This->start_time;
        IReferenceClock_GetTime(This->refClock, &time);
        This->position += time - This->start_time;
    }

    if (This->state == State_Running) SendFilterMessage(iface, SendPause, 0);
    SendFilterMessage(iface, SendStop, 0);
    This->state = State_Stopped;
    LeaveCriticalSection(&This->cs);
    return S_OK;
}

static HRESULT WINAPI MediaControl_GetState(IMediaControl *iface,
					    LONG msTimeout,
					    OAFilterState *pfs) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);
    DWORD end;

    TRACE("(%p/%p)->(%d, %p)\n", This, iface, msTimeout, pfs);

    if (!pfs)
        return E_POINTER;

    EnterCriticalSection(&This->cs);

    *pfs = This->state;
    if (msTimeout > 0)
    {
        end = GetTickCount() + msTimeout;
    }
    else if (msTimeout < 0)
    {
        end = INFINITE;
    }
    else
    {
        end = 0;
    }
    if (end)
        SendFilterMessage(iface, SendGetState, end);

    LeaveCriticalSection(&This->cs);

    return S_OK;
}

static HRESULT WINAPI MediaControl_RenderFile(IMediaControl *iface,
					      BSTR strFilename) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    FIXME("(%p/%p)->(%s (%p)): stub !!!\n", This, iface, debugstr_w(strFilename), strFilename);

    return S_OK;
}

static HRESULT WINAPI MediaControl_AddSourceFilter(IMediaControl *iface,
						   BSTR strFilename,
						   IDispatch **ppUnk) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    FIXME("(%p/%p)->(%s (%p), %p): stub !!!\n", This, iface, debugstr_w(strFilename), strFilename, ppUnk);

    return S_OK;
}

static HRESULT WINAPI MediaControl_get_FilterCollection(IMediaControl *iface,
							IDispatch **ppUnk) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, ppUnk);

    return S_OK;
}

static HRESULT WINAPI MediaControl_get_RegFilterCollection(IMediaControl *iface,
							   IDispatch **ppUnk) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, ppUnk);

    return S_OK;
}

static HRESULT WINAPI MediaControl_StopWhenReady(IMediaControl *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    FIXME("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}


static const IMediaControlVtbl IMediaControl_VTable =
{
    MediaControl_QueryInterface,
    MediaControl_AddRef,
    MediaControl_Release,
    MediaControl_GetTypeInfoCount,
    MediaControl_GetTypeInfo,
    MediaControl_GetIDsOfNames,
    MediaControl_Invoke,
    MediaControl_Run,
    MediaControl_Pause,
    MediaControl_Stop,
    MediaControl_GetState,
    MediaControl_RenderFile,
    MediaControl_AddSourceFilter,
    MediaControl_get_FilterCollection,
    MediaControl_get_RegFilterCollection,
    MediaControl_StopWhenReady
};


/*** IUnknown methods ***/
static HRESULT WINAPI MediaSeeking_QueryInterface(IMediaSeeking *iface,
						  REFIID riid,
						  LPVOID*ppvObj) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return Filtergraph_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI MediaSeeking_AddRef(IMediaSeeking *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI MediaSeeking_Release(IMediaSeeking *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_Release(This);
}

typedef HRESULT (WINAPI *fnFoundSeek)(IFilterGraphImpl *This, IMediaSeeking*, DWORD_PTR arg);

static HRESULT all_renderers_seek(IFilterGraphImpl *This, fnFoundSeek FoundSeek, DWORD_PTR arg) {
    BOOL allnotimpl = TRUE;
    int i;
    IBaseFilter* pfilter;
    IEnumPins* pEnum;
    HRESULT hr, hr_return = S_OK;
    IPin* pPin;
    DWORD dummy;
    PIN_DIRECTION dir;

    TRACE("(%p)->(%p %08lx)\n", This, FoundSeek, arg);
    /* Send a message to all renderers, they are responsible for broadcasting it further */

    for(i = 0; i < This->nFilters; i++)
    {
        BOOL renderer = TRUE;
        pfilter = This->ppFiltersInGraph[i];
        hr = IBaseFilter_EnumPins(pfilter, &pEnum);
        if (hr != S_OK)
        {
            WARN("Enum pins failed %x\n", hr);
            continue;
        }
        /* Check if it is a source filter */
        while(IEnumPins_Next(pEnum, 1, &pPin, &dummy) == S_OK)
        {
            IPin_QueryDirection(pPin, &dir);
            IPin_Release(pPin);
            if (dir != PINDIR_INPUT)
            {
                renderer = FALSE;
                break;
            }
        }
        IEnumPins_Release(pEnum);
        if (renderer)
        {
            IMediaSeeking *seek = NULL;
            IBaseFilter_QueryInterface(pfilter, &IID_IMediaSeeking, (void**)&seek);
            if (!seek)
                continue;

            hr = FoundSeek(This, seek, arg);

            IMediaSeeking_Release(seek);
            if (hr_return != E_NOTIMPL)
                allnotimpl = FALSE;
            if (hr_return == S_OK || (FAILED(hr) && hr != E_NOTIMPL && SUCCEEDED(hr_return)))
                hr_return = hr;
        }
    }

    if (allnotimpl)
        return E_NOTIMPL;
    return hr_return;
}

static HRESULT WINAPI FoundCapabilities(IFilterGraphImpl *This, IMediaSeeking *seek, DWORD_PTR pcaps)
{
    HRESULT hr;
    DWORD caps = 0;

    hr = IMediaSeeking_GetCapabilities(seek, &caps);
    if (FAILED(hr))
        return hr;

    /* Only add common capabilities everything supports */
    *(DWORD*)pcaps &= caps;

    return hr;
}

/*** IMediaSeeking methods ***/
static HRESULT WINAPI MediaSeeking_GetCapabilities(IMediaSeeking *iface,
						   DWORD *pCapabilities) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);
    HRESULT hr;
    TRACE("(%p/%p)->(%p)\n", This, iface, pCapabilities);

    if (!pCapabilities)
        return E_POINTER;

    EnterCriticalSection(&This->cs);
    *pCapabilities = 0xffffffff;

    hr = all_renderers_seek(This, FoundCapabilities, (DWORD_PTR)pCapabilities);
    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI MediaSeeking_CheckCapabilities(IMediaSeeking *iface,
						     DWORD *pCapabilities) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);
    DWORD originalcaps;
    HRESULT hr;
    TRACE("(%p/%p)->(%p)\n", This, iface, pCapabilities);

    if (!pCapabilities)
        return E_POINTER;

    EnterCriticalSection(&This->cs);
    originalcaps = *pCapabilities;
    hr = all_renderers_seek(This, FoundCapabilities, (DWORD_PTR)pCapabilities);
    LeaveCriticalSection(&This->cs);

    if (FAILED(hr))
        return hr;

    if (!*pCapabilities)
        return E_FAIL;
    if (*pCapabilities != originalcaps)
        return S_FALSE;
    return S_OK;
}

static HRESULT WINAPI MediaSeeking_IsFormatSupported(IMediaSeeking *iface,
						     const GUID *pFormat) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    if (!pFormat)
        return E_POINTER;

    TRACE("(%p/%p)->(%s)\n", This, iface, debugstr_guid(pFormat));

    if (!IsEqualGUID(&TIME_FORMAT_MEDIA_TIME, pFormat))
    {
        FIXME("Unhandled time format %s\n", debugstr_guid(pFormat));
        return S_FALSE;
    }

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_QueryPreferredFormat(IMediaSeeking *iface,
							GUID *pFormat) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    if (!pFormat)
        return E_POINTER;

    FIXME("(%p/%p)->(%p): semi-stub !!!\n", This, iface, pFormat);
    memcpy(pFormat, &TIME_FORMAT_MEDIA_TIME, sizeof(GUID));

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_GetTimeFormat(IMediaSeeking *iface,
						 GUID *pFormat) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    if (!pFormat)
        return E_POINTER;

    TRACE("(%p/%p)->(%p)\n", This, iface, pFormat);
    memcpy(pFormat, &This->timeformatseek, sizeof(GUID));

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_IsUsingTimeFormat(IMediaSeeking *iface,
						     const GUID *pFormat) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pFormat);
    if (!pFormat)
        return E_POINTER;

    if (memcmp(pFormat, &This->timeformatseek, sizeof(GUID)))
        return S_FALSE;

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_SetTimeFormat(IMediaSeeking *iface,
						 const GUID *pFormat) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    if (!pFormat)
        return E_POINTER;

    TRACE("(%p/%p)->(%s)\n", This, iface, debugstr_guid(pFormat));

    if (This->state != State_Stopped)
        return VFW_E_WRONG_STATE;

    if (!IsEqualGUID(&TIME_FORMAT_MEDIA_TIME, pFormat))
    {
        FIXME("Unhandled time format %s\n", debugstr_guid(pFormat));
        return E_INVALIDARG;
    }

    return S_OK;
}

static HRESULT WINAPI FoundDuration(IFilterGraphImpl *This, IMediaSeeking *seek, DWORD_PTR pduration)
{
    HRESULT hr;
    LONGLONG duration = 0, *pdur = (LONGLONG*)pduration;

    hr = IMediaSeeking_GetDuration(seek, &duration);
    if (FAILED(hr))
        return hr;

    /* FIXME: Minimum or maximum duration? Assuming minimum */
    if (duration > 0 && *pdur < duration)
        *pdur = duration;

    return hr;
}

static HRESULT WINAPI MediaSeeking_GetDuration(IMediaSeeking *iface,
					       LONGLONG *pDuration) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pDuration);

    if (!pDuration)
        return E_POINTER;

    EnterCriticalSection(&This->cs);
    *pDuration = -1;
    hr = all_renderers_seek(This, FoundDuration, (DWORD_PTR)pDuration);
    LeaveCriticalSection(&This->cs);

    TRACE("--->%08x\n", hr);
    return hr;
}

static HRESULT WINAPI MediaSeeking_GetStopPosition(IMediaSeeking *iface,
						   LONGLONG *pStop) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);
    HRESULT hr = S_OK;

    TRACE("(%p/%p)->(%p)\n", This, iface, pStop);

    if (!pStop)
        return E_POINTER;

    EnterCriticalSection(&This->cs);
    if (This->stop_position < 0)
        /* Stop position not set, use duration instead */
        hr = IMediaSeeking_GetDuration(iface, pStop);
    else
        *pStop = This->stop_position;

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI MediaSeeking_GetCurrentPosition(IMediaSeeking *iface,
						      LONGLONG *pCurrent) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);
    LONGLONG time = 0;

    if (!pCurrent)
        return E_POINTER;

    EnterCriticalSection(&This->cs);
    if (This->state == State_Running && This->refClock)
    {
        IReferenceClock_GetTime(This->refClock, &time);
        if (time)
            time += This->position - This->start_time;
        if (time < This->position)
            time = This->position;
        *pCurrent = time;
    }
    else
        *pCurrent = This->position;
    LeaveCriticalSection(&This->cs);

    TRACE("Time: %u.%03u\n", (DWORD)(*pCurrent / 10000000), (DWORD)((*pCurrent / 10000)%1000));

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_ConvertTimeFormat(IMediaSeeking *iface,
						     LONGLONG *pTarget,
						     const GUID *pTargetFormat,
						     LONGLONG Source,
						     const GUID *pSourceFormat) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    FIXME("(%p/%p)->(%p, %p, 0x%s, %p): stub !!!\n", This, iface, pTarget,
        pTargetFormat, wine_dbgstr_longlong(Source), pSourceFormat);

    return S_OK;
}

struct pos_args {
    LONGLONG* current, *stop;
    DWORD curflags, stopflags;
};

static HRESULT WINAPI found_setposition(IFilterGraphImpl *This, IMediaSeeking *seek, DWORD_PTR pargs)
{
    struct pos_args *args = (void*)pargs;

    return IMediaSeeking_SetPositions(seek, args->current, args->curflags, args->stop, args->stopflags);
}

static HRESULT WINAPI MediaSeeking_SetPositions(IMediaSeeking *iface,
						LONGLONG *pCurrent,
						DWORD dwCurrentFlags,
						LONGLONG *pStop,
						DWORD dwStopFlags) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);
    HRESULT hr = S_OK;
    FILTER_STATE state;
    struct pos_args args;

    TRACE("(%p/%p)->(%p, %08x, %p, %08x)\n", This, iface, pCurrent, dwCurrentFlags, pStop, dwStopFlags);

    EnterCriticalSection(&This->cs);
    state = This->state;
    TRACE("State: %s\n", state == State_Running ? "Running" : (state == State_Paused ? "Paused" : (state == State_Stopped ? "Stopped" : "UNKNOWN")));

    if ((dwCurrentFlags & 0x7) == AM_SEEKING_AbsolutePositioning)
    {
        This->position = *pCurrent;
    }
    else if ((dwCurrentFlags & 0x7) != AM_SEEKING_NoPositioning)
        FIXME("Adjust method %x not handled yet!\n", dwCurrentFlags & 0x7);

    if ((dwStopFlags & 0x7) == AM_SEEKING_AbsolutePositioning)
        This->stop_position = *pStop;
    else if ((dwStopFlags & 0x7) != AM_SEEKING_NoPositioning)
        FIXME("Stop position not handled yet!\n");

    args.current = pCurrent;
    args.stop = pStop;
    args.curflags = dwCurrentFlags;
    args.stopflags = dwStopFlags;
    hr = all_renderers_seek(This, found_setposition, (DWORD_PTR)&args);

    if (This->refClock && ((dwCurrentFlags & 0x7) != AM_SEEKING_NoPositioning))
    {
        /* Update start time, prevents weird jumps */
        IReferenceClock_GetTime(This->refClock, &This->start_time);
    }
    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI MediaSeeking_GetPositions(IMediaSeeking *iface,
						LONGLONG *pCurrent,
						LONGLONG *pStop) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pCurrent, pStop);
    hr = IMediaSeeking_GetCurrentPosition(iface, pCurrent);
    if (SUCCEEDED(hr))
        hr = IMediaSeeking_GetStopPosition(iface, pStop);

    return hr;
}

static HRESULT WINAPI MediaSeeking_GetAvailable(IMediaSeeking *iface,
						LONGLONG *pEarliest,
						LONGLONG *pLatest) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    FIXME("(%p/%p)->(%p, %p): stub !!!\n", This, iface, pEarliest, pLatest);

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_SetRate(IMediaSeeking *iface,
					   double dRate) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    FIXME("(%p/%p)->(%f): stub !!!\n", This, iface, dRate);

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_GetRate(IMediaSeeking *iface,
					   double *pdRate) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, pdRate);

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_GetPreroll(IMediaSeeking *iface,
					      LONGLONG *pllPreroll) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, pllPreroll);

    return S_OK;
}


static const IMediaSeekingVtbl IMediaSeeking_VTable =
{
    MediaSeeking_QueryInterface,
    MediaSeeking_AddRef,
    MediaSeeking_Release,
    MediaSeeking_GetCapabilities,
    MediaSeeking_CheckCapabilities,
    MediaSeeking_IsFormatSupported,
    MediaSeeking_QueryPreferredFormat,
    MediaSeeking_GetTimeFormat,
    MediaSeeking_IsUsingTimeFormat,
    MediaSeeking_SetTimeFormat,
    MediaSeeking_GetDuration,
    MediaSeeking_GetStopPosition,
    MediaSeeking_GetCurrentPosition,
    MediaSeeking_ConvertTimeFormat,
    MediaSeeking_SetPositions,
    MediaSeeking_GetPositions,
    MediaSeeking_GetAvailable,
    MediaSeeking_SetRate,
    MediaSeeking_GetRate,
    MediaSeeking_GetPreroll
};

/*** IUnknown methods ***/
static HRESULT WINAPI MediaPosition_QueryInterface(IMediaPosition* iface, REFIID riid, void** ppvObj){
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaPosition_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return Filtergraph_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI MediaPosition_AddRef(IMediaPosition *iface){
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaPosition_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI MediaPosition_Release(IMediaPosition *iface){
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaPosition_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_Release(This);
}

/*** IDispatch methods ***/
static HRESULT WINAPI MediaPosition_GetTypeInfoCount(IMediaPosition *iface, UINT* pctinfo){
    FIXME("(%p) stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_GetTypeInfo(IMediaPosition *iface, UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo){
    FIXME("(%p) stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_GetIDsOfNames(IMediaPosition* iface, REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId){
    FIXME("(%p) stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_Invoke(IMediaPosition* iface, DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr){
    FIXME("(%p) stub!\n", iface);
    return E_NOTIMPL;
}

/*** IMediaPosition methods ***/
static HRESULT WINAPI MediaPosition_get_Duration(IMediaPosition * iface, REFTIME *plength){
    FIXME("(%p)->(%p) stub!\n", iface, plength);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_put_CurrentPosition(IMediaPosition * iface, REFTIME llTime){
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaPosition_vtbl, iface);
    LONGLONG reftime = llTime;

    return IMediaSeeking_SetPositions((IMediaSeeking *)&This->IMediaSeeking_vtbl, &reftime, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
}

static HRESULT WINAPI MediaPosition_get_CurrentPosition(IMediaPosition * iface, REFTIME *pllTime){
    FIXME("(%p)->(%p) stub!\n", iface, pllTime);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_get_StopTime(IMediaPosition * iface, REFTIME *pllTime){
    FIXME("(%p)->(%p) stub!\n", iface, pllTime);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_put_StopTime(IMediaPosition * iface, REFTIME llTime){
    FIXME("(%p)->(%f) stub!\n", iface, llTime);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_get_PrerollTime(IMediaPosition * iface, REFTIME *pllTime){
    FIXME("(%p)->(%p) stub!\n", iface, pllTime);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_put_PrerollTime(IMediaPosition * iface, REFTIME llTime){
    FIXME("(%p)->(%f) stub!\n", iface, llTime);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_put_Rate(IMediaPosition * iface, double dRate){
    FIXME("(%p)->(%f) stub!\n", iface, dRate);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_get_Rate(IMediaPosition * iface, double *pdRate){
    FIXME("(%p)->(%p) stub!\n", iface, pdRate);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_CanSeekForward(IMediaPosition * iface, LONG *pCanSeekForward){
    FIXME("(%p)->(%p) stub!\n", iface, pCanSeekForward);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_CanSeekBackward(IMediaPosition * iface, LONG *pCanSeekBackward){
    FIXME("(%p)->(%p) stub!\n", iface, pCanSeekBackward);
    return E_NOTIMPL;
}


static const IMediaPositionVtbl IMediaPosition_VTable =
{
    MediaPosition_QueryInterface,
    MediaPosition_AddRef,
    MediaPosition_Release,
    MediaPosition_GetTypeInfoCount,
    MediaPosition_GetTypeInfo,
    MediaPosition_GetIDsOfNames,
    MediaPosition_Invoke,
    MediaPosition_get_Duration,
    MediaPosition_put_CurrentPosition,
    MediaPosition_get_CurrentPosition,
    MediaPosition_get_StopTime,
    MediaPosition_put_StopTime,
    MediaPosition_get_PrerollTime,
    MediaPosition_put_PrerollTime,
    MediaPosition_put_Rate,
    MediaPosition_get_Rate,
    MediaPosition_CanSeekForward,
    MediaPosition_CanSeekBackward
};

static HRESULT GetTargetInterface(IFilterGraphImpl* pGraph, REFIID riid, LPVOID* ppvObj)
{
    HRESULT hr = E_NOINTERFACE;
    int i;
    int entry;

    /* Check if the interface type is already registered */
    for (entry = 0; entry < pGraph->nItfCacheEntries; entry++)
        if (riid == pGraph->ItfCacheEntries[entry].riid)
        {
            if (pGraph->ItfCacheEntries[entry].iface)
            {
                /* Return the interface if available */
                *ppvObj = pGraph->ItfCacheEntries[entry].iface;
                return S_OK;
            }
            break;
        }

    if (entry >= MAX_ITF_CACHE_ENTRIES)
    {
        FIXME("Not enough space to store interface in the cache\n");
        return E_OUTOFMEMORY;
    }

    /* Find a filter supporting the requested interface */
    for (i = 0; i < pGraph->nFilters; i++)
    {
        hr = IBaseFilter_QueryInterface(pGraph->ppFiltersInGraph[i], riid, ppvObj);
        if (hr == S_OK)
        {
            pGraph->ItfCacheEntries[entry].riid = riid;
            pGraph->ItfCacheEntries[entry].filter = pGraph->ppFiltersInGraph[i];
            pGraph->ItfCacheEntries[entry].iface = *ppvObj;
            if (entry >= pGraph->nItfCacheEntries)
                pGraph->nItfCacheEntries++;
            return S_OK;
        }
        if (hr != E_NOINTERFACE)
            return hr;
    }

    return hr;
}

/*** IUnknown methods ***/
static HRESULT WINAPI BasicAudio_QueryInterface(IBasicAudio *iface,
						REFIID riid,
						LPVOID*ppvObj) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return Filtergraph_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI BasicAudio_AddRef(IBasicAudio *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI BasicAudio_Release(IBasicAudio *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_Release(This);
}

/*** IDispatch methods ***/
static HRESULT WINAPI BasicAudio_GetTypeInfoCount(IBasicAudio *iface,
						  UINT*pctinfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);
    IBasicAudio* pBasicAudio;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pctinfo);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicAudio, (LPVOID*)&pBasicAudio);

    if (hr == S_OK)
        hr = IBasicAudio_GetTypeInfoCount(pBasicAudio, pctinfo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicAudio_GetTypeInfo(IBasicAudio *iface,
					     UINT iTInfo,
					     LCID lcid,
					     ITypeInfo**ppTInfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);
    IBasicAudio* pBasicAudio;
    HRESULT hr;

    TRACE("(%p/%p)->(%d, %d, %p)\n", This, iface, iTInfo, lcid, ppTInfo);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicAudio, (LPVOID*)&pBasicAudio);

    if (hr == S_OK)
        hr = IBasicAudio_GetTypeInfo(pBasicAudio, iTInfo, lcid, ppTInfo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicAudio_GetIDsOfNames(IBasicAudio *iface,
					       REFIID riid,
					       LPOLESTR*rgszNames,
					       UINT cNames,
					       LCID lcid,
					       DISPID*rgDispId) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);
    IBasicAudio* pBasicAudio;
    HRESULT hr;

    TRACE("(%p/%p)->(%s (%p), %p, %d, %d, %p)\n", This, iface, debugstr_guid(riid), riid, rgszNames, cNames, lcid, rgDispId);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicAudio, (LPVOID*)&pBasicAudio);

    if (hr == S_OK)
        hr = IBasicAudio_GetIDsOfNames(pBasicAudio, riid, rgszNames, cNames, lcid, rgDispId);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicAudio_Invoke(IBasicAudio *iface,
					DISPID dispIdMember,
					REFIID riid,
					LCID lcid,
					WORD wFlags,
					DISPPARAMS*pDispParams,
					VARIANT*pVarResult,
					EXCEPINFO*pExepInfo,
					UINT*puArgErr) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);
    IBasicAudio* pBasicAudio;
    HRESULT hr;

    TRACE("(%p/%p)->(%d, %s (%p), %d, %04x, %p, %p, %p, %p)\n", This, iface, dispIdMember, debugstr_guid(riid), riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicAudio, (LPVOID*)&pBasicAudio);

    if (hr == S_OK)
        hr = IBasicAudio_Invoke(pBasicAudio, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    LeaveCriticalSection(&This->cs);

    return hr;
}

/*** IBasicAudio methods ***/
static HRESULT WINAPI BasicAudio_put_Volume(IBasicAudio *iface,
                                            LONG lVolume) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);
    IBasicAudio* pBasicAudio;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, lVolume);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicAudio, (LPVOID*)&pBasicAudio);

    if (hr == S_OK)
        hr = IBasicAudio_put_Volume(pBasicAudio, lVolume);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicAudio_get_Volume(IBasicAudio *iface,
                                            LONG *plVolume) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);
    IBasicAudio* pBasicAudio;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, plVolume);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicAudio, (LPVOID*)&pBasicAudio);

    if (hr == S_OK)
        hr = IBasicAudio_get_Volume(pBasicAudio, plVolume);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicAudio_put_Balance(IBasicAudio *iface,
                                             LONG lBalance) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);
    IBasicAudio* pBasicAudio;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, lBalance);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicAudio, (LPVOID*)&pBasicAudio);

    if (hr == S_OK)
        hr = IBasicAudio_put_Balance(pBasicAudio, lBalance);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicAudio_get_Balance(IBasicAudio *iface,
                                             LONG *plBalance) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);
    IBasicAudio* pBasicAudio;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, plBalance);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicAudio, (LPVOID*)&pBasicAudio);

    if (hr == S_OK)
        hr = IBasicAudio_get_Balance(pBasicAudio, plBalance);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static const IBasicAudioVtbl IBasicAudio_VTable =
{
    BasicAudio_QueryInterface,
    BasicAudio_AddRef,
    BasicAudio_Release,
    BasicAudio_GetTypeInfoCount,
    BasicAudio_GetTypeInfo,
    BasicAudio_GetIDsOfNames,
    BasicAudio_Invoke,
    BasicAudio_put_Volume,
    BasicAudio_get_Volume,
    BasicAudio_put_Balance,
    BasicAudio_get_Balance
};

/*** IUnknown methods ***/
static HRESULT WINAPI BasicVideo_QueryInterface(IBasicVideo2 *iface,
						REFIID riid,
						LPVOID*ppvObj) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return Filtergraph_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI BasicVideo_AddRef(IBasicVideo2 *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI BasicVideo_Release(IBasicVideo2 *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_Release(This);
}

/*** IDispatch methods ***/
static HRESULT WINAPI BasicVideo_GetTypeInfoCount(IBasicVideo2 *iface,
						  UINT*pctinfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pctinfo);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetTypeInfoCount(pBasicVideo, pctinfo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_GetTypeInfo(IBasicVideo2 *iface,
					     UINT iTInfo,
					     LCID lcid,
					     ITypeInfo**ppTInfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d, %d, %p)\n", This, iface, iTInfo, lcid, ppTInfo);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetTypeInfo(pBasicVideo, iTInfo, lcid, ppTInfo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_GetIDsOfNames(IBasicVideo2 *iface,
					       REFIID riid,
					       LPOLESTR*rgszNames,
					       UINT cNames,
					       LCID lcid,
					       DISPID*rgDispId) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%s (%p), %p, %d, %d, %p)\n", This, iface, debugstr_guid(riid), riid, rgszNames, cNames, lcid, rgDispId);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetIDsOfNames(pBasicVideo, riid, rgszNames, cNames, lcid, rgDispId);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_Invoke(IBasicVideo2 *iface,
					DISPID dispIdMember,
					REFIID riid,
					LCID lcid,
					WORD wFlags,
					DISPPARAMS*pDispParams,
					VARIANT*pVarResult,
					EXCEPINFO*pExepInfo,
					UINT*puArgErr) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d, %s (%p), %d, %04x, %p, %p, %p, %p)\n", This, iface, dispIdMember, debugstr_guid(riid), riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_Invoke(pBasicVideo, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    LeaveCriticalSection(&This->cs);

    return hr;
}

/*** IBasicVideo methods ***/
static HRESULT WINAPI BasicVideo_get_AvgTimePerFrame(IBasicVideo2 *iface,
						     REFTIME *pAvgTimePerFrame) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pAvgTimePerFrame);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_AvgTimePerFrame(pBasicVideo, pAvgTimePerFrame);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_BitRate(IBasicVideo2 *iface,
                                             LONG *pBitRate) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pBitRate);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_BitRate(pBasicVideo, pBitRate);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_BitErrorRate(IBasicVideo2 *iface,
                                                  LONG *pBitErrorRate) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pBitErrorRate);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_BitErrorRate(pBasicVideo, pBitErrorRate);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_VideoWidth(IBasicVideo2 *iface,
                                                LONG *pVideoWidth) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pVideoWidth);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_VideoWidth(pBasicVideo, pVideoWidth);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_VideoHeight(IBasicVideo2 *iface,
                                                 LONG *pVideoHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pVideoHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_VideoHeight(pBasicVideo, pVideoHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_SourceLeft(IBasicVideo2 *iface,
                                                LONG SourceLeft) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, SourceLeft);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_SourceLeft(pBasicVideo, SourceLeft);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_SourceLeft(IBasicVideo2 *iface,
                                                LONG *pSourceLeft) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceLeft);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_SourceLeft(pBasicVideo, pSourceLeft);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_SourceWidth(IBasicVideo2 *iface,
                                                 LONG SourceWidth) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, SourceWidth);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_SourceWidth(pBasicVideo, SourceWidth);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_SourceWidth(IBasicVideo2 *iface,
                                                 LONG *pSourceWidth) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceWidth);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_SourceWidth(pBasicVideo, pSourceWidth);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_SourceTop(IBasicVideo2 *iface,
                                               LONG SourceTop) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, SourceTop);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_SourceTop(pBasicVideo, SourceTop);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_SourceTop(IBasicVideo2 *iface,
                                               LONG *pSourceTop) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceTop);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_SourceTop(pBasicVideo, pSourceTop);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_SourceHeight(IBasicVideo2 *iface,
                                                  LONG SourceHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, SourceHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_SourceHeight(pBasicVideo, SourceHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_SourceHeight(IBasicVideo2 *iface,
                                                  LONG *pSourceHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_SourceHeight(pBasicVideo, pSourceHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_DestinationLeft(IBasicVideo2 *iface,
                                                     LONG DestinationLeft) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, DestinationLeft);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_DestinationLeft(pBasicVideo, DestinationLeft);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_DestinationLeft(IBasicVideo2 *iface,
                                                     LONG *pDestinationLeft) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationLeft);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_DestinationLeft(pBasicVideo, pDestinationLeft);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_DestinationWidth(IBasicVideo2 *iface,
                                                      LONG DestinationWidth) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, DestinationWidth);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_DestinationWidth(pBasicVideo, DestinationWidth);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_DestinationWidth(IBasicVideo2 *iface,
                                                      LONG *pDestinationWidth) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationWidth);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_DestinationWidth(pBasicVideo, pDestinationWidth);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_DestinationTop(IBasicVideo2 *iface,
                                                    LONG DestinationTop) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, DestinationTop);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_DestinationTop(pBasicVideo, DestinationTop);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_DestinationTop(IBasicVideo2 *iface,
                                                    LONG *pDestinationTop) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationTop);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_DestinationTop(pBasicVideo, pDestinationTop);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_DestinationHeight(IBasicVideo2 *iface,
                                                       LONG DestinationHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, DestinationHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_DestinationHeight(pBasicVideo, DestinationHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_DestinationHeight(IBasicVideo2 *iface,
                                                       LONG *pDestinationHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_DestinationHeight(pBasicVideo, pDestinationHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_SetSourcePosition(IBasicVideo2 *iface,
                                                   LONG Left,
                                                   LONG Top,
                                                   LONG Width,
                                                   LONG Height) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d, %d, %d, %d)\n", This, iface, Left, Top, Width, Height);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_SetSourcePosition(pBasicVideo, Left, Top, Width, Height);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_GetSourcePosition(IBasicVideo2 *iface,
                                                   LONG *pLeft,
                                                   LONG *pTop,
                                                   LONG *pWidth,
                                                   LONG *pHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetSourcePosition(pBasicVideo, pLeft, pTop, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_SetDefaultSourcePosition(IBasicVideo2 *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_SetDefaultSourcePosition(pBasicVideo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_SetDestinationPosition(IBasicVideo2 *iface,
                                                        LONG Left,
                                                        LONG Top,
                                                        LONG Width,
                                                        LONG Height) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d, %d, %d, %d)\n", This, iface, Left, Top, Width, Height);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_SetDestinationPosition(pBasicVideo, Left, Top, Width, Height);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_GetDestinationPosition(IBasicVideo2 *iface,
                                                        LONG *pLeft,
                                                        LONG *pTop,
                                                        LONG *pWidth,
                                                        LONG *pHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetDestinationPosition(pBasicVideo, pLeft, pTop, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_SetDefaultDestinationPosition(IBasicVideo2 *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_SetDefaultDestinationPosition(pBasicVideo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_GetVideoSize(IBasicVideo2 *iface,
                                              LONG *pWidth,
                                              LONG *pHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetVideoSize(pBasicVideo, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_GetVideoPaletteEntries(IBasicVideo2 *iface,
                                                        LONG StartIndex,
                                                        LONG Entries,
                                                        LONG *pRetrieved,
                                                        LONG *pPalette) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d, %d, %p, %p)\n", This, iface, StartIndex, Entries, pRetrieved, pPalette);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetVideoPaletteEntries(pBasicVideo, StartIndex, Entries, pRetrieved, pPalette);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_GetCurrentImage(IBasicVideo2 *iface,
                                                 LONG *pBufferSize,
                                                 LONG *pDIBImage) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pBufferSize, pDIBImage);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetCurrentImage(pBasicVideo, pBufferSize, pDIBImage);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_IsUsingDefaultSource(IBasicVideo2 *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_IsUsingDefaultSource(pBasicVideo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_IsUsingDefaultDestination(IBasicVideo2 *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_IsUsingDefaultDestination(pBasicVideo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo2_GetPreferredAspectRatio(IBasicVideo2 *iface, LONG *plAspectX, LONG *plAspectY) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo2 *pBasicVideo2;
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo2, (LPVOID*)&pBasicVideo2);

    if (hr == S_OK)
        hr = BasicVideo2_GetPreferredAspectRatio(iface, plAspectX, plAspectY);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static const IBasicVideo2Vtbl IBasicVideo_VTable =
{
    BasicVideo_QueryInterface,
    BasicVideo_AddRef,
    BasicVideo_Release,
    BasicVideo_GetTypeInfoCount,
    BasicVideo_GetTypeInfo,
    BasicVideo_GetIDsOfNames,
    BasicVideo_Invoke,
    BasicVideo_get_AvgTimePerFrame,
    BasicVideo_get_BitRate,
    BasicVideo_get_BitErrorRate,
    BasicVideo_get_VideoWidth,
    BasicVideo_get_VideoHeight,
    BasicVideo_put_SourceLeft,
    BasicVideo_get_SourceLeft,
    BasicVideo_put_SourceWidth,
    BasicVideo_get_SourceWidth,
    BasicVideo_put_SourceTop,
    BasicVideo_get_SourceTop,
    BasicVideo_put_SourceHeight,
    BasicVideo_get_SourceHeight,
    BasicVideo_put_DestinationLeft,
    BasicVideo_get_DestinationLeft,
    BasicVideo_put_DestinationWidth,
    BasicVideo_get_DestinationWidth,
    BasicVideo_put_DestinationTop,
    BasicVideo_get_DestinationTop,
    BasicVideo_put_DestinationHeight,
    BasicVideo_get_DestinationHeight,
    BasicVideo_SetSourcePosition,
    BasicVideo_GetSourcePosition,
    BasicVideo_SetDefaultSourcePosition,
    BasicVideo_SetDestinationPosition,
    BasicVideo_GetDestinationPosition,
    BasicVideo_SetDefaultDestinationPosition,
    BasicVideo_GetVideoSize,
    BasicVideo_GetVideoPaletteEntries,
    BasicVideo_GetCurrentImage,
    BasicVideo_IsUsingDefaultSource,
    BasicVideo_IsUsingDefaultDestination,
    BasicVideo2_GetPreferredAspectRatio
};


/*** IUnknown methods ***/
static HRESULT WINAPI VideoWindow_QueryInterface(IVideoWindow *iface,
						 REFIID riid,
						 LPVOID*ppvObj) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return Filtergraph_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI VideoWindow_AddRef(IVideoWindow *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI VideoWindow_Release(IVideoWindow *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_Release(This);
}

/*** IDispatch methods ***/
static HRESULT WINAPI VideoWindow_GetTypeInfoCount(IVideoWindow *iface,
						   UINT*pctinfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pctinfo);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_GetTypeInfoCount(pVideoWindow, pctinfo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_GetTypeInfo(IVideoWindow *iface,
					      UINT iTInfo,
					      LCID lcid,
					      ITypeInfo**ppTInfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d, %d, %p)\n", This, iface, iTInfo, lcid, ppTInfo);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_GetTypeInfo(pVideoWindow, iTInfo, lcid, ppTInfo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_GetIDsOfNames(IVideoWindow *iface,
						REFIID riid,
						LPOLESTR*rgszNames,
						UINT cNames,
						LCID lcid,
						DISPID*rgDispId) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%s (%p), %p, %d, %d, %p)\n", This, iface, debugstr_guid(riid), riid, rgszNames, cNames, lcid, rgDispId);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_GetIDsOfNames(pVideoWindow, riid, rgszNames, cNames, lcid, rgDispId);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_Invoke(IVideoWindow *iface,
					 DISPID dispIdMember,
					 REFIID riid,
					 LCID lcid,
					 WORD wFlags,
					 DISPPARAMS*pDispParams,
					 VARIANT*pVarResult,
					 EXCEPINFO*pExepInfo,
					 UINT*puArgErr) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d, %s (%p), %d, %04x, %p, %p, %p, %p)\n", This, iface, dispIdMember, debugstr_guid(riid), riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_Invoke(pVideoWindow, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    LeaveCriticalSection(&This->cs);

    return hr;
}


/*** IVideoWindow methods ***/
static HRESULT WINAPI VideoWindow_put_Caption(IVideoWindow *iface,
					      BSTR strCaption) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;
    
    TRACE("(%p/%p)->(%s (%p))\n", This, iface, debugstr_w(strCaption), strCaption);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Caption(pVideoWindow, strCaption);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Caption(IVideoWindow *iface,
					      BSTR *strCaption) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, strCaption);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Caption(pVideoWindow, strCaption);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_WindowStyle(IVideoWindow *iface,
                                                  LONG WindowStyle) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, WindowStyle);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_WindowStyle(pVideoWindow, WindowStyle);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_WindowStyle(IVideoWindow *iface,
                                                  LONG *WindowStyle) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, WindowStyle);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_WindowStyle(pVideoWindow, WindowStyle);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_WindowStyleEx(IVideoWindow *iface,
                                                    LONG WindowStyleEx) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, WindowStyleEx);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_WindowStyleEx(pVideoWindow, WindowStyleEx);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_WindowStyleEx(IVideoWindow *iface,
                                                    LONG *WindowStyleEx) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, WindowStyleEx);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_WindowStyleEx(pVideoWindow, WindowStyleEx);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_AutoShow(IVideoWindow *iface,
                                               LONG AutoShow) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, AutoShow);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_AutoShow(pVideoWindow, AutoShow);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_AutoShow(IVideoWindow *iface,
                                               LONG *AutoShow) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, AutoShow);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_AutoShow(pVideoWindow, AutoShow);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_WindowState(IVideoWindow *iface,
                                                  LONG WindowState) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, WindowState);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_WindowState(pVideoWindow, WindowState);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_WindowState(IVideoWindow *iface,
                                                  LONG *WindowState) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, WindowState);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_WindowState(pVideoWindow, WindowState);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_BackgroundPalette(IVideoWindow *iface,
                                                        LONG BackgroundPalette) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, BackgroundPalette);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_BackgroundPalette(pVideoWindow, BackgroundPalette);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_BackgroundPalette(IVideoWindow *iface,
                                                        LONG *pBackgroundPalette) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pBackgroundPalette);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_BackgroundPalette(pVideoWindow, pBackgroundPalette);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_Visible(IVideoWindow *iface,
                                              LONG Visible) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, Visible);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Visible(pVideoWindow, Visible);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Visible(IVideoWindow *iface,
                                              LONG *pVisible) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pVisible);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Visible(pVideoWindow, pVisible);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_Left(IVideoWindow *iface,
                                           LONG Left) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, Left);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Left(pVideoWindow, Left);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Left(IVideoWindow *iface,
                                           LONG *pLeft) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pLeft);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Left(pVideoWindow, pLeft);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_Width(IVideoWindow *iface,
                                            LONG Width) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, Width);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Width(pVideoWindow, Width);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Width(IVideoWindow *iface,
                                            LONG *pWidth) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pWidth);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Width(pVideoWindow, pWidth);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_Top(IVideoWindow *iface,
                                          LONG Top) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, Top);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Top(pVideoWindow, Top);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Top(IVideoWindow *iface,
                                          LONG *pTop) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pTop);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Top(pVideoWindow, pTop);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_Height(IVideoWindow *iface,
                                             LONG Height) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, Height);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Height(pVideoWindow, Height);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Height(IVideoWindow *iface,
                                             LONG *pHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Height(pVideoWindow, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_Owner(IVideoWindow *iface,
					    OAHWND Owner) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%08x)\n", This, iface, (DWORD) Owner);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Owner(pVideoWindow, Owner);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Owner(IVideoWindow *iface,
					    OAHWND *Owner) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, Owner);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Owner(pVideoWindow, Owner);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_MessageDrain(IVideoWindow *iface,
						   OAHWND Drain) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%08x)\n", This, iface, (DWORD) Drain);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_MessageDrain(pVideoWindow, Drain);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_MessageDrain(IVideoWindow *iface,
						   OAHWND *Drain) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, Drain);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_MessageDrain(pVideoWindow, Drain);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_BorderColor(IVideoWindow *iface,
                                                  LONG *Color) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, Color);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_BorderColor(pVideoWindow, Color);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_BorderColor(IVideoWindow *iface,
                                                  LONG Color) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, Color);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_BorderColor(pVideoWindow, Color);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_FullScreenMode(IVideoWindow *iface,
                                                     LONG *FullScreenMode) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, FullScreenMode);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_FullScreenMode(pVideoWindow, FullScreenMode);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_FullScreenMode(IVideoWindow *iface,
                                                     LONG FullScreenMode) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, FullScreenMode);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_FullScreenMode(pVideoWindow, FullScreenMode);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_SetWindowForeground(IVideoWindow *iface,
                                                      LONG Focus) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, Focus);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_SetWindowForeground(pVideoWindow, Focus);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_NotifyOwnerMessage(IVideoWindow *iface,
						     OAHWND hwnd,
                                                     LONG uMsg,
						     LONG_PTR wParam,
						     LONG_PTR lParam) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%08lx, %d, %08lx, %08lx)\n", This, iface, hwnd, uMsg, wParam, lParam);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_NotifyOwnerMessage(pVideoWindow, hwnd, uMsg, wParam, lParam);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_SetWindowPosition(IVideoWindow *iface,
                                                    LONG Left,
                                                    LONG Top,
                                                    LONG Width,
                                                    LONG Height) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d, %d, %d, %d)\n", This, iface, Left, Top, Width, Height);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_SetWindowPosition(pVideoWindow, Left, Top, Width, Height);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_GetWindowPosition(IVideoWindow *iface,
                                                    LONG *pLeft,
                                                    LONG *pTop,
                                                    LONG *pWidth,
                                                    LONG *pHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_GetWindowPosition(pVideoWindow, pLeft, pTop, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_GetMinIdealImageSize(IVideoWindow *iface,
                                                       LONG *pWidth,
                                                       LONG *pHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_GetMinIdealImageSize(pVideoWindow, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_GetMaxIdealImageSize(IVideoWindow *iface,
                                                       LONG *pWidth,
                                                       LONG *pHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_GetMaxIdealImageSize(pVideoWindow, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_GetRestorePosition(IVideoWindow *iface,
                                                     LONG *pLeft,
                                                     LONG *pTop,
                                                     LONG *pWidth,
                                                     LONG *pHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_GetRestorePosition(pVideoWindow, pLeft, pTop, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_HideCursor(IVideoWindow *iface,
                                             LONG HideCursor) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, HideCursor);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_HideCursor(pVideoWindow, HideCursor);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_IsCursorHidden(IVideoWindow *iface,
                                                 LONG *CursorHidden) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, CursorHidden);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_IsCursorHidden(pVideoWindow, CursorHidden);

    LeaveCriticalSection(&This->cs);

    return hr;
}


static const IVideoWindowVtbl IVideoWindow_VTable =
{
    VideoWindow_QueryInterface,
    VideoWindow_AddRef,
    VideoWindow_Release,
    VideoWindow_GetTypeInfoCount,
    VideoWindow_GetTypeInfo,
    VideoWindow_GetIDsOfNames,
    VideoWindow_Invoke,
    VideoWindow_put_Caption,
    VideoWindow_get_Caption,
    VideoWindow_put_WindowStyle,
    VideoWindow_get_WindowStyle,
    VideoWindow_put_WindowStyleEx,
    VideoWindow_get_WindowStyleEx,
    VideoWindow_put_AutoShow,
    VideoWindow_get_AutoShow,
    VideoWindow_put_WindowState,
    VideoWindow_get_WindowState,
    VideoWindow_put_BackgroundPalette,
    VideoWindow_get_BackgroundPalette,
    VideoWindow_put_Visible,
    VideoWindow_get_Visible,
    VideoWindow_put_Left,
    VideoWindow_get_Left,
    VideoWindow_put_Width,
    VideoWindow_get_Width,
    VideoWindow_put_Top,
    VideoWindow_get_Top,
    VideoWindow_put_Height,
    VideoWindow_get_Height,
    VideoWindow_put_Owner,
    VideoWindow_get_Owner,
    VideoWindow_put_MessageDrain,
    VideoWindow_get_MessageDrain,
    VideoWindow_get_BorderColor,
    VideoWindow_put_BorderColor,
    VideoWindow_get_FullScreenMode,
    VideoWindow_put_FullScreenMode,
    VideoWindow_SetWindowForeground,
    VideoWindow_NotifyOwnerMessage,
    VideoWindow_SetWindowPosition,
    VideoWindow_GetWindowPosition,
    VideoWindow_GetMinIdealImageSize,
    VideoWindow_GetMaxIdealImageSize,
    VideoWindow_GetRestorePosition,
    VideoWindow_HideCursor,
    VideoWindow_IsCursorHidden
};


/*** IUnknown methods ***/
static HRESULT WINAPI MediaEvent_QueryInterface(IMediaEventEx *iface,
						REFIID riid,
						LPVOID*ppvObj) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return Filtergraph_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI MediaEvent_AddRef(IMediaEventEx *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI MediaEvent_Release(IMediaEventEx *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_Release(This);
}

/*** IDispatch methods ***/
static HRESULT WINAPI MediaEvent_GetTypeInfoCount(IMediaEventEx *iface,
						  UINT*pctinfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pctinfo);

    return S_OK;
}

static HRESULT WINAPI MediaEvent_GetTypeInfo(IMediaEventEx *iface,
					     UINT iTInfo,
					     LCID lcid,
					     ITypeInfo**ppTInfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%d, %d, %p): stub !!!\n", This, iface, iTInfo, lcid, ppTInfo);

    return S_OK;
}

static HRESULT WINAPI MediaEvent_GetIDsOfNames(IMediaEventEx *iface,
					       REFIID riid,
					       LPOLESTR*rgszNames,
					       UINT cNames,
					       LCID lcid,
					       DISPID*rgDispId) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p, %d, %d, %p): stub !!!\n", This, iface, debugstr_guid(riid), riid, rgszNames, cNames, lcid, rgDispId);

    return S_OK;
}

static HRESULT WINAPI MediaEvent_Invoke(IMediaEventEx *iface,
					DISPID dispIdMember,
					REFIID riid,
					LCID lcid,
					WORD wFlags,
					DISPPARAMS*pDispParams,
					VARIANT*pVarResult,
					EXCEPINFO*pExepInfo,
					UINT*puArgErr) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%d, %s (%p), %d, %04x, %p, %p, %p, %p): stub !!!\n", This, iface, dispIdMember, debugstr_guid(riid), riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    return S_OK;
}

/*** IMediaEvent methods ***/
static HRESULT WINAPI MediaEvent_GetEventHandle(IMediaEventEx *iface,
						OAEVENT *hEvent) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, hEvent);

    *hEvent = (OAEVENT)This->evqueue.msg_event;

    return S_OK;
}

static HRESULT WINAPI MediaEvent_GetEvent(IMediaEventEx *iface,
                                          LONG *lEventCode,
					  LONG_PTR *lParam1,
					  LONG_PTR *lParam2,
                                          LONG msTimeout) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);
    Event evt;

    TRACE("(%p/%p)->(%p, %p, %p, %d)\n", This, iface, lEventCode, lParam1, lParam2, msTimeout);

    if (EventsQueue_GetEvent(&This->evqueue, &evt, msTimeout))
    {
	*lEventCode = evt.lEventCode;
	*lParam1 = evt.lParam1;
	*lParam2 = evt.lParam2;
	return S_OK;
    }

    *lEventCode = 0;
    return E_ABORT;
}

static HRESULT WINAPI MediaEvent_WaitForCompletion(IMediaEventEx *iface,
                                                   LONG msTimeout,
                                                   LONG *pEvCode) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%d, %p)\n", This, iface, msTimeout, pEvCode);

    if (WaitForSingleObject(This->hEventCompletion, msTimeout) == WAIT_OBJECT_0)
    {
	*pEvCode = This->CompletionStatus;
	return S_OK;
    }

    *pEvCode = 0;
    return E_ABORT;
}

static HRESULT WINAPI MediaEvent_CancelDefaultHandling(IMediaEventEx *iface,
                                                       LONG lEvCode) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, lEvCode);

    if (lEvCode == EC_COMPLETE)
	This->HandleEcComplete = FALSE;
    else if (lEvCode == EC_REPAINT)
	This->HandleEcRepaint = FALSE;
    else if (lEvCode == EC_CLOCK_CHANGED)
        This->HandleEcClockChanged = FALSE;
    else
	return S_FALSE;

    return S_OK;
}

static HRESULT WINAPI MediaEvent_RestoreDefaultHandling(IMediaEventEx *iface,
                                                        LONG lEvCode) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, lEvCode);

    if (lEvCode == EC_COMPLETE)
	This->HandleEcComplete = TRUE;
    else if (lEvCode == EC_REPAINT)
	This->HandleEcRepaint = TRUE;
    else if (lEvCode == EC_CLOCK_CHANGED)
        This->HandleEcClockChanged = TRUE;
    else
	return S_FALSE;

    return S_OK;
}

static HRESULT WINAPI MediaEvent_FreeEventParams(IMediaEventEx *iface,
                                                 LONG lEvCode,
						 LONG_PTR lParam1,
						 LONG_PTR lParam2) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%d, %08lx, %08lx): stub !!!\n", This, iface, lEvCode, lParam1, lParam2);

    return S_OK;
}

/*** IMediaEventEx methods ***/
static HRESULT WINAPI MediaEvent_SetNotifyWindow(IMediaEventEx *iface,
						 OAHWND hwnd,
                                                 LONG lMsg,
						 LONG_PTR lInstanceData) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%08lx, %d, %08lx)\n", This, iface, hwnd, lMsg, lInstanceData);

    This->notif.hWnd = (HWND)hwnd;
    This->notif.msg = lMsg;
    This->notif.instance = (long) lInstanceData;

    return S_OK;
}

static HRESULT WINAPI MediaEvent_SetNotifyFlags(IMediaEventEx *iface,
                                                LONG lNoNotifyFlags) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, lNoNotifyFlags);

    if ((lNoNotifyFlags != 0) && (lNoNotifyFlags != 1))
	return E_INVALIDARG;

    This->notif.disabled = lNoNotifyFlags;

    return S_OK;
}

static HRESULT WINAPI MediaEvent_GetNotifyFlags(IMediaEventEx *iface,
                                                LONG *lplNoNotifyFlags) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, lplNoNotifyFlags);

    if (!lplNoNotifyFlags)
	return E_POINTER;

    *lplNoNotifyFlags = This->notif.disabled;

    return S_OK;
}


static const IMediaEventExVtbl IMediaEventEx_VTable =
{
    MediaEvent_QueryInterface,
    MediaEvent_AddRef,
    MediaEvent_Release,
    MediaEvent_GetTypeInfoCount,
    MediaEvent_GetTypeInfo,
    MediaEvent_GetIDsOfNames,
    MediaEvent_Invoke,
    MediaEvent_GetEventHandle,
    MediaEvent_GetEvent,
    MediaEvent_WaitForCompletion,
    MediaEvent_CancelDefaultHandling,
    MediaEvent_RestoreDefaultHandling,
    MediaEvent_FreeEventParams,
    MediaEvent_SetNotifyWindow,
    MediaEvent_SetNotifyFlags,
    MediaEvent_GetNotifyFlags
};


static HRESULT WINAPI MediaFilter_QueryInterface(IMediaFilter *iface, REFIID riid, LPVOID *ppv)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaFilter_vtbl, iface);

    return Filtergraph_QueryInterface(This, riid, ppv);
}

static ULONG WINAPI MediaFilter_AddRef(IMediaFilter *iface)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaFilter_vtbl, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI MediaFilter_Release(IMediaFilter *iface)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaFilter_vtbl, iface);

    return Filtergraph_Release(This);
}

static HRESULT WINAPI MediaFilter_GetClassID(IMediaFilter *iface, CLSID * pClassID)
{
    FIXME("(%p): stub\n", pClassID);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaFilter_Stop(IMediaFilter *iface)
{
    FIXME("(): stub\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaFilter_Pause(IMediaFilter *iface)
{
    FIXME("(): stub\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaFilter_Run(IMediaFilter *iface, REFERENCE_TIME tStart)
{
    FIXME("(0x%s): stub\n", wine_dbgstr_longlong(tStart));

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaFilter_GetState(IMediaFilter *iface, DWORD dwMsTimeout, FILTER_STATE * pState)
{
    FIXME("(%d, %p): stub\n", dwMsTimeout, pState);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaFilter_SetSyncSource(IMediaFilter *iface, IReferenceClock *pClock)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaFilter_vtbl, iface);
    HRESULT hr = S_OK;
    int i;

    TRACE("(%p/%p)->(%p)\n", iface, This, pClock);

    EnterCriticalSection(&This->cs);
    {
        for (i = 0;i < This->nFilters;i++)
        {
            hr = IBaseFilter_SetSyncSource(This->ppFiltersInGraph[i], pClock);
            if (FAILED(hr))
                break;
        }

        if (FAILED(hr))
        {
            for(;i >= 0;i--)
                IBaseFilter_SetSyncSource(This->ppFiltersInGraph[i], This->refClock);
        }
        else
        {
            if (This->refClock)
                IReferenceClock_Release(This->refClock);
            This->refClock = pClock;
            if (This->refClock)
                IReferenceClock_AddRef(This->refClock);

            if (This->HandleEcClockChanged)
            {
                IMediaEventSink *pEventSink;
                HRESULT eshr;

                eshr = IMediaFilter_QueryInterface(iface, &IID_IMediaEventSink, (LPVOID)&pEventSink);
                if (SUCCEEDED(eshr))
                {
                    IMediaEventSink_Notify(pEventSink, EC_CLOCK_CHANGED, 0, 0);
                    IMediaEventSink_Release(pEventSink);
                }
            }
        }
    }
    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI MediaFilter_GetSyncSource(IMediaFilter *iface, IReferenceClock **ppClock)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaFilter_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", iface, This, ppClock);

    if (!ppClock)
        return E_POINTER;

    EnterCriticalSection(&This->cs);
    {
        *ppClock = This->refClock;
        if (*ppClock)
            IReferenceClock_AddRef(*ppClock);
    }
    LeaveCriticalSection(&This->cs);

    return S_OK;
}

static const IMediaFilterVtbl IMediaFilter_VTable =
{
    MediaFilter_QueryInterface,
    MediaFilter_AddRef,
    MediaFilter_Release,
    MediaFilter_GetClassID,
    MediaFilter_Stop,
    MediaFilter_Pause,
    MediaFilter_Run,
    MediaFilter_GetState,
    MediaFilter_SetSyncSource,
    MediaFilter_GetSyncSource
};

static HRESULT WINAPI MediaEventSink_QueryInterface(IMediaEventSink *iface, REFIID riid, LPVOID *ppv)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventSink_vtbl, iface);

    return Filtergraph_QueryInterface(This, riid, ppv);
}

static ULONG WINAPI MediaEventSink_AddRef(IMediaEventSink *iface)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventSink_vtbl, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI MediaEventSink_Release(IMediaEventSink *iface)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventSink_vtbl, iface);

    return Filtergraph_Release(This);
}

static HRESULT WINAPI MediaEventSink_Notify(IMediaEventSink *iface, LONG EventCode, LONG_PTR EventParam1, LONG_PTR EventParam2)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventSink_vtbl, iface);
    Event evt;

    TRACE("(%p/%p)->(%d, %ld, %ld)\n", This, iface, EventCode, EventParam1, EventParam2);

    /* We need thread safety here, let's use the events queue's one */
    EnterCriticalSection(&This->evqueue.msg_crst);

    if ((EventCode == EC_COMPLETE) && This->HandleEcComplete)
    {
        TRACE("Process EC_COMPLETE notification\n");
        if (++This->EcCompleteCount == This->nRenderers)
        {
            evt.lEventCode = EC_COMPLETE;
            evt.lParam1 = S_OK;
            evt.lParam2 = 0;
            TRACE("Send EC_COMPLETE to app\n");
            EventsQueue_PutEvent(&This->evqueue, &evt);
            if (!This->notif.disabled && This->notif.hWnd)
	    {
                TRACE("Send Window message\n");
                PostMessageW(This->notif.hWnd, This->notif.msg, 0, This->notif.instance);
            }
            This->CompletionStatus = EC_COMPLETE;
            SetEvent(This->hEventCompletion);
        }
    }
    else if ((EventCode == EC_REPAINT) && This->HandleEcRepaint)
    {
        /* FIXME: Not handled yet */
    }
    else
    {
        evt.lEventCode = EventCode;
        evt.lParam1 = EventParam1;
        evt.lParam2 = EventParam2;
        EventsQueue_PutEvent(&This->evqueue, &evt);
        if (!This->notif.disabled && This->notif.hWnd)
            PostMessageW(This->notif.hWnd, This->notif.msg, 0, This->notif.instance);
    }

    LeaveCriticalSection(&This->evqueue.msg_crst);
    return S_OK;
}

static const IMediaEventSinkVtbl IMediaEventSink_VTable =
{
    MediaEventSink_QueryInterface,
    MediaEventSink_AddRef,
    MediaEventSink_Release,
    MediaEventSink_Notify
};

static HRESULT WINAPI GraphConfig_QueryInterface(IGraphConfig *iface, REFIID riid, LPVOID *ppv)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphConfig_vtbl, iface);

    return Filtergraph_QueryInterface(This, riid, ppv);
}

static ULONG WINAPI GraphConfig_AddRef(IGraphConfig *iface)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphConfig_vtbl, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI GraphConfig_Release(IGraphConfig *iface)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphConfig_vtbl, iface);

    return Filtergraph_Release(This);
}

static HRESULT WINAPI GraphConfig_Reconnect(IGraphConfig *iface,
					    IPin* pOutputPin,
					    IPin* pInputPin,
					    const AM_MEDIA_TYPE* pmtFirstConnection,
					    IBaseFilter* pUsingFilter,
					    HANDLE hAbortEvent,
					    DWORD dwFlags)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphConfig_vtbl, iface);

    FIXME("(%p)->(%p, %p, %p, %p, %p, %x): stub!\n", This, pOutputPin, pInputPin, pmtFirstConnection, pUsingFilter, hAbortEvent, dwFlags);
    
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_Reconfigure(IGraphConfig *iface,
					      IGraphConfigCallback* pCallback,
					      PVOID pvContext,
					      DWORD dwFlags,
					      HANDLE hAbortEvent)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphConfig_vtbl, iface);
    HRESULT hr;

    WARN("(%p)->(%p, %p, %x, %p): partial stub!\n", This, pCallback, pvContext, dwFlags, hAbortEvent);

    if (hAbortEvent)
        FIXME("The parameter hAbortEvent is not handled!\n");

    EnterCriticalSection(&This->cs);

    hr = IGraphConfigCallback_Reconfigure(pCallback, pvContext, dwFlags);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI GraphConfig_AddFilterToCache(IGraphConfig *iface,
						   IBaseFilter* pFilter)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphConfig_vtbl, iface);

    FIXME("(%p)->(%p): stub!\n", This, pFilter);
    
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_EnumCacheFilter(IGraphConfig *iface,
						  IEnumFilters** pEnum)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphConfig_vtbl, iface);

    FIXME("(%p)->(%p): stub!\n", This, pEnum);
    
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_RemoveFilterFromCache(IGraphConfig *iface,
							IBaseFilter* pFilter)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphConfig_vtbl, iface);

    FIXME("(%p)->(%p): stub!\n", This, pFilter);
    
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_GetStartTime(IGraphConfig *iface,
					       REFERENCE_TIME* prtStart)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphConfig_vtbl, iface);

    FIXME("(%p)->(%p): stub!\n", This, prtStart);
    
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_PushThroughData(IGraphConfig *iface,
						  IPin* pOutputPin,
						  IPinConnection* pConnection,
						  HANDLE hEventAbort)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphConfig_vtbl, iface);

    FIXME("(%p)->(%p, %p, %p): stub!\n", This, pOutputPin, pConnection, hEventAbort);
    
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_SetFilterFlags(IGraphConfig *iface,
						 IBaseFilter* pFilter,
						 DWORD dwFlags)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphConfig_vtbl, iface);

    FIXME("(%p)->(%p, %x): stub!\n", This, pFilter, dwFlags);
    
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_GetFilterFlags(IGraphConfig *iface,
						 IBaseFilter* pFilter,
						 DWORD* dwFlags)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphConfig_vtbl, iface);

    FIXME("(%p)->(%p, %p): stub!\n", This, pFilter, dwFlags);
    
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_RemoveFilterEx(IGraphConfig *iface,
					  	 IBaseFilter* pFilter,
						 DWORD dwFlags)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphConfig_vtbl, iface);

    FIXME("(%p)->(%p, %x): stub!\n", This, pFilter, dwFlags);
    
    return E_NOTIMPL;
}

static const IGraphConfigVtbl IGraphConfig_VTable =
{
    GraphConfig_QueryInterface,
    GraphConfig_AddRef,
    GraphConfig_Release,
    GraphConfig_Reconnect,
    GraphConfig_Reconfigure,
    GraphConfig_AddFilterToCache,
    GraphConfig_EnumCacheFilter,
    GraphConfig_RemoveFilterFromCache,
    GraphConfig_GetStartTime,
    GraphConfig_PushThroughData,
    GraphConfig_SetFilterFlags,
    GraphConfig_GetFilterFlags,
    GraphConfig_RemoveFilterEx
};

static const IUnknownVtbl IInner_VTable =
{
    FilterGraphInner_QueryInterface,
    FilterGraphInner_AddRef,
    FilterGraphInner_Release
};

static HRESULT Filtergraph_QueryInterface(IFilterGraphImpl *This,
                                          REFIID riid,
                                          LPVOID * ppv) {
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

static ULONG Filtergraph_AddRef(IFilterGraphImpl *This) {
    if (This->pUnkOuter && This->bUnkOuterValid)
        return IUnknown_AddRef(This->pUnkOuter);
    return IUnknown_AddRef((IUnknown *)&(This->IInner_vtbl));
}

static ULONG Filtergraph_Release(IFilterGraphImpl *This) {
    if (This->pUnkOuter && This->bUnkOuterValid)
        return IUnknown_Release(This->pUnkOuter);
    return IUnknown_Release((IUnknown *)&(This->IInner_vtbl));
}

/* This is the only function that actually creates a FilterGraph class... */
HRESULT FilterGraph_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    IFilterGraphImpl *fimpl;
    HRESULT hr;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    *ppObj = NULL;

    fimpl = CoTaskMemAlloc(sizeof(*fimpl));
    fimpl->pUnkOuter = pUnkOuter;
    fimpl->bUnkOuterValid = FALSE;
    fimpl->bAggregatable = FALSE;
    fimpl->IInner_vtbl = &IInner_VTable;
    fimpl->IFilterGraph2_vtbl = &IFilterGraph2_VTable;
    fimpl->IMediaControl_vtbl = &IMediaControl_VTable;
    fimpl->IMediaSeeking_vtbl = &IMediaSeeking_VTable;
    fimpl->IBasicAudio_vtbl = &IBasicAudio_VTable;
    fimpl->IBasicVideo_vtbl = &IBasicVideo_VTable;
    fimpl->IVideoWindow_vtbl = &IVideoWindow_VTable;
    fimpl->IMediaEventEx_vtbl = &IMediaEventEx_VTable;
    fimpl->IMediaFilter_vtbl = &IMediaFilter_VTable;
    fimpl->IMediaEventSink_vtbl = &IMediaEventSink_VTable;
    fimpl->IGraphConfig_vtbl = &IGraphConfig_VTable;
    fimpl->IMediaPosition_vtbl = &IMediaPosition_VTable;
    fimpl->ref = 1;
    fimpl->ppFiltersInGraph = NULL;
    fimpl->pFilterNames = NULL;
    fimpl->nFilters = 0;
    fimpl->filterCapacity = 0;
    fimpl->nameIndex = 1;
    fimpl->refClock = NULL;
    fimpl->hEventCompletion = CreateEventW(0, TRUE, FALSE, 0);
    fimpl->HandleEcComplete = TRUE;
    fimpl->HandleEcRepaint = TRUE;
    fimpl->HandleEcClockChanged = TRUE;
    fimpl->notif.hWnd = 0;
    fimpl->notif.disabled = FALSE;
    fimpl->nRenderers = 0;
    fimpl->EcCompleteCount = 0;
    fimpl->state = State_Stopped;
    EventsQueue_Init(&fimpl->evqueue);
    InitializeCriticalSection(&fimpl->cs);
    fimpl->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": IFilterGraphImpl.cs");
    fimpl->nItfCacheEntries = 0;
    memcpy(&fimpl->timeformatseek, &TIME_FORMAT_MEDIA_TIME, sizeof(GUID));
    fimpl->start_time = fimpl->position = 0;
    fimpl->stop_position = -1;
    fimpl->punkFilterMapper2 = NULL;
    fimpl->recursioncount = 0;

    /* create Filtermapper aggregated. */
    hr = CoCreateInstance(&CLSID_FilterMapper2, pUnkOuter ? pUnkOuter : (IUnknown*)&fimpl->IInner_vtbl, CLSCTX_INPROC_SERVER,
        &IID_IUnknown, (LPVOID*)&fimpl->punkFilterMapper2);

    if (SUCCEEDED(hr)) {
        hr = IUnknown_QueryInterface(fimpl->punkFilterMapper2, &IID_IFilterMapper2,  (LPVOID*)&fimpl->pFilterMapper2);
    }

    if (SUCCEEDED(hr)) {
        /* Release controlling IUnknown - compensate refcount increase from caching IFilterMapper2 interface. */
        if (pUnkOuter) IUnknown_Release(pUnkOuter);
        else IUnknown_Release((IUnknown*)&fimpl->IInner_vtbl);
    }

    if (FAILED(hr)) {
        ERR("Unable to create filter mapper (%x)\n", hr);
        if (fimpl->punkFilterMapper2) IUnknown_Release(fimpl->punkFilterMapper2);
        CloseHandle(fimpl->hEventCompletion);
        EventsQueue_Destroy(&fimpl->evqueue);
        fimpl->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&fimpl->cs);
        CoTaskMemFree(fimpl);
        return hr;
    }
    IFilterGraph2_SetDefaultSyncSource((IFilterGraph2*)fimpl);

    *ppObj = fimpl;
    return S_OK;
}

HRESULT FilterGraphNoThread_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    FIXME("CLSID_FilterGraphNoThread partially implemented - Forwarding to CLSID_FilterGraph\n");
    return FilterGraph_create(pUnkOuter, ppObj);
}
