/*
 * Unit tests for Direct Show functions
 *
 * Copyright (C) 2004 Christian Costa
 * Copyright (C) 2008 Alexander Dorofeyev
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

#define COBJMACROS
#define CONST_VTABLE

#include "wine/test.h"
#include "dshow.h"
#include "control.h"

typedef struct TestFilterImpl
{
    IBaseFilter IBaseFilter_iface;

    LONG refCount;
    CRITICAL_SECTION csFilter;
    FILTER_STATE state;
    FILTER_INFO filterInfo;
    CLSID clsid;
    IPin **ppPins;
    UINT nPins;
} TestFilterImpl;

static const WCHAR avifile[] = {'t','e','s','t','.','a','v','i',0};
static const WCHAR mpegfile[] = {'t','e','s','t','.','m','p','g',0};

static IGraphBuilder *pgraph;

static int createfiltergraph(void)
{
    return S_OK == CoCreateInstance(
        &CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IGraphBuilder, (LPVOID*)&pgraph);
}

static void rungraph(void)
{
    HRESULT hr;
    IMediaControl* pmc;
    IMediaEvent* pme;
    IMediaFilter* pmf;
    HANDLE hEvent;

    hr = IGraphBuilder_QueryInterface(pgraph, &IID_IMediaControl, (LPVOID*)&pmc);
    ok(hr==S_OK, "Cannot get IMediaControl interface returned: %x\n", hr);

    hr = IGraphBuilder_QueryInterface(pgraph, &IID_IMediaFilter, (LPVOID*)&pmf);
    ok(hr==S_OK, "Cannot get IMediaFilter interface returned: %x\n", hr);

    IMediaControl_Stop(pmc);

    IMediaFilter_SetSyncSource(pmf, NULL);

    IMediaFilter_Release(pmf);

    hr = IMediaControl_Run(pmc);
    ok(hr==S_FALSE, "Cannot run the graph returned: %x\n", hr);

    Sleep(10);
    /* Crash fun */
    trace("run -> stop\n");
    hr = IMediaControl_Stop(pmc);
    ok(hr==S_OK || hr == S_FALSE, "Cannot stop the graph returned: %x\n", hr);

    IGraphBuilder_SetDefaultSyncSource(pgraph);

    Sleep(10);
    trace("stop -> pause\n");
    hr = IMediaControl_Pause(pmc);
    ok(hr==S_OK || hr == S_FALSE, "Cannot pause the graph returned: %x\n", hr);

    Sleep(10);
    trace("pause -> run\n");
    hr = IMediaControl_Run(pmc);
    ok(hr==S_OK || hr == S_FALSE, "Cannot start the graph returned: %x\n", hr);

    Sleep(10);
    trace("run -> pause\n");
    hr = IMediaControl_Pause(pmc);
    ok(hr==S_OK || hr == S_FALSE, "Cannot pause the graph returned: %x\n", hr);

    Sleep(10);
    trace("pause -> stop\n");
    hr = IMediaControl_Stop(pmc);
    ok(hr==S_OK || hr == S_FALSE, "Cannot stop the graph returned: %x\n", hr);

    Sleep(10);
    trace("pause -> run\n");
    hr = IMediaControl_Run(pmc);
    ok(hr==S_OK || hr == S_FALSE, "Cannot start the graph returned: %x\n", hr);

    trace("run -> stop\n");
    hr = IMediaControl_Stop(pmc);
    ok(hr==S_OK || hr == S_FALSE, "Cannot stop the graph returned: %x\n", hr);

    trace("stop -> run\n");
    hr = IMediaControl_Run(pmc);
    ok(hr==S_OK || hr == S_FALSE, "Cannot start the graph returned: %x\n", hr);

    hr = IGraphBuilder_QueryInterface(pgraph, &IID_IMediaEvent, (LPVOID*)&pme);
    ok(hr==S_OK, "Cannot get IMediaEvent interface returned: %x\n", hr);

    hr = IMediaEvent_GetEventHandle(pme, (OAEVENT*)&hEvent);
    ok(hr==S_OK, "Cannot get event handle returned: %x\n", hr);

    /* WaitForSingleObject(hEvent, INFINITE); */
    Sleep(20000);

    hr = IMediaEvent_Release(pme);
    ok(hr==2, "Releasing mediaevent returned: %x\n", hr);

    hr = IMediaControl_Stop(pmc);
    ok(hr==S_OK, "Cannot stop the graph returned: %x\n", hr);
    
    hr = IMediaControl_Release(pmc);
    ok(hr==1, "Releasing mediacontrol returned: %x\n", hr);
}

static void releasefiltergraph(void)
{
    HRESULT hr;

    hr = IGraphBuilder_Release(pgraph);
    ok(hr==0, "Releasing filtergraph returned: %x\n", hr);
}

static void test_render_run(const WCHAR *file)
{
    HANDLE h;
    HRESULT hr;

    if (!createfiltergraph())
        return;

    h = CreateFileW(file, 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        CloseHandle(h);
        hr = IGraphBuilder_RenderFile(pgraph, file, NULL);
        ok(hr==S_OK, "RenderFile returned: %x\n", hr);
        rungraph();
    }

    releasefiltergraph();
}

static void test_graph_builder(void)
{
    HRESULT hr;
    IBaseFilter *pF = NULL;
    IBaseFilter *pF2 = NULL;
    IPin *pIn = NULL;
    IEnumPins *pEnum = NULL;
    PIN_DIRECTION dir;
    static const WCHAR testFilterW[] = {'t','e','s','t','F','i','l','t','e','r',0};
    static const WCHAR fooBarW[] = {'f','o','o','B','a','r',0};

    if (!createfiltergraph())
        return;

    /* create video filter */
    hr = CoCreateInstance(&CLSID_VideoRenderer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (LPVOID*)&pF);
    ok(hr == S_OK, "CoCreateInstance failed with %x\n", hr);
    ok(pF != NULL, "pF is NULL\n");

    hr = IGraphBuilder_AddFilter(pgraph, NULL, testFilterW);
    ok(hr == E_POINTER, "IGraphBuilder_AddFilter returned %x\n", hr);

    /* add the two filters to the graph */
    hr = IGraphBuilder_AddFilter(pgraph, pF, testFilterW);
    ok(hr == S_OK, "failed to add pF to the graph: %x\n", hr);

    /* find the pins */
    hr = IBaseFilter_EnumPins(pF, &pEnum);
    ok(hr == S_OK, "IBaseFilter_EnumPins failed for pF: %x\n", hr);
    ok(pEnum != NULL, "pEnum is NULL\n");
    hr = IEnumPins_Next(pEnum, 1, &pIn, NULL);
    ok(hr == S_OK, "IEnumPins_Next failed for pF: %x\n", hr);
    ok(pIn != NULL, "pIn is NULL\n");
    hr = IPin_QueryDirection(pIn, &dir);
    ok(hr == S_OK, "IPin_QueryDirection failed: %x\n", hr);
    ok(dir == PINDIR_INPUT, "pin has wrong direction\n");

    hr = IGraphBuilder_FindFilterByName(pgraph, fooBarW, &pF2);
    ok(hr == VFW_E_NOT_FOUND, "IGraphBuilder_FindFilterByName returned %x\n", hr);
    ok(pF2 == NULL, "IGraphBuilder_FindFilterByName returned %p\n", pF2);
    hr = IGraphBuilder_FindFilterByName(pgraph, testFilterW, &pF2);
    ok(hr == S_OK, "IGraphBuilder_FindFilterByName returned %x\n", hr);
    ok(pF2 != NULL, "IGraphBuilder_FindFilterByName returned NULL\n");
    hr = IGraphBuilder_FindFilterByName(pgraph, testFilterW, NULL);
    ok(hr == E_POINTER, "IGraphBuilder_FindFilterByName returned %x\n", hr);

    hr = IGraphBuilder_Connect(pgraph, NULL, pIn);
    ok(hr == E_POINTER, "IGraphBuilder_Connect returned %x\n", hr);

    hr = IGraphBuilder_Connect(pgraph, pIn, NULL);
    ok(hr == E_POINTER, "IGraphBuilder_Connect returned %x\n", hr);

    hr = IGraphBuilder_Connect(pgraph, pIn, pIn);
    ok(hr == VFW_E_CANNOT_CONNECT, "IGraphBuilder_Connect returned %x\n", hr);

    if (pIn) IPin_Release(pIn);
    if (pEnum) IEnumPins_Release(pEnum);
    if (pF) IBaseFilter_Release(pF);
    if (pF2) IBaseFilter_Release(pF2);

    releasefiltergraph();
}

static void test_graph_builder_addfilter(void)
{
    HRESULT hr;
    IBaseFilter *pF = NULL;
    static const WCHAR testFilterW[] = {'t','e','s','t','F','i','l','t','e','r',0};

    if (!createfiltergraph())
        return;

    hr = IGraphBuilder_AddFilter(pgraph, NULL, testFilterW);
    ok(hr == E_POINTER, "IGraphBuilder_AddFilter returned: %x\n", hr);

    /* create video filter */
    hr = CoCreateInstance(&CLSID_VideoRenderer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (LPVOID*)&pF);
    ok(hr == S_OK, "CoCreateInstance failed with %x\n", hr);
    ok(pF != NULL, "pF is NULL\n");
    if (!pF) {
        skip("failed to created filter, skipping\n");
        return;
    }

    hr = IGraphBuilder_AddFilter(pgraph, pF, NULL);
    ok(hr == S_OK, "IGraphBuilder_AddFilter returned: %x\n", hr);
    IBaseFilter_Release(pF);
}

static void test_mediacontrol(void)
{
    HRESULT hr;
    LONGLONG pos = 0xdeadbeef;
    IMediaSeeking *seeking = NULL;
    IMediaFilter *filter = NULL;
    IMediaControl *control = NULL;

    IGraphBuilder_SetDefaultSyncSource(pgraph);
    hr = IGraphBuilder_QueryInterface(pgraph, &IID_IMediaSeeking, (void**) &seeking);
    ok(hr == S_OK, "QueryInterface IMediaControl failed: %08x\n", hr);
    if (FAILED(hr))
        return;

    hr = IGraphBuilder_QueryInterface(pgraph, &IID_IMediaFilter, (void**) &filter);
    ok(hr == S_OK, "QueryInterface IMediaFilter failed: %08x\n", hr);
    if (FAILED(hr))
    {
        IMediaSeeking_Release(seeking);
        return;
    }

    hr = IGraphBuilder_QueryInterface(pgraph, &IID_IMediaControl, (void**) &control);
    ok(hr == S_OK, "QueryInterface IMediaControl failed: %08x\n", hr);
    if (FAILED(hr))
    {
        IMediaSeeking_Release(seeking);
        IMediaFilter_Release(filter);
        return;
    }

    hr = IMediaSeeking_GetCurrentPosition(seeking, &pos);
    ok(hr == S_OK, "GetCurrentPosition failed: %08x\n", hr);
    ok(pos == 0, "Position != 0 (%x%08x)\n", (DWORD)(pos >> 32), (DWORD)pos);

    hr = IMediaSeeking_SetPositions(seeking, NULL, AM_SEEKING_ReturnTime, NULL, AM_SEEKING_NoPositioning);
    ok(hr == S_OK, "SetPositions failed: %08x\n", hr);
    hr = IMediaSeeking_SetPositions(seeking, NULL, AM_SEEKING_NoPositioning, NULL, AM_SEEKING_ReturnTime);
    ok(hr == S_OK, "SetPositions failed: %08x\n", hr);

    IMediaFilter_SetSyncSource(filter, NULL);
    pos = 0xdeadbeef;
    hr = IMediaSeeking_GetCurrentPosition(seeking, &pos);
    ok(hr == S_OK, "GetCurrentPosition failed: %08x\n", hr);
    ok(pos == 0, "Position != 0 (%x%08x)\n", (DWORD)(pos >> 32), (DWORD)pos);

    hr = IMediaControl_GetState(control, 1000, NULL);
    ok(hr == E_POINTER, "GetState expected %08x, got %08x\n", E_POINTER, hr);

    IMediaControl_Release(control);
    IMediaSeeking_Release(seeking);
    IMediaFilter_Release(filter);
    releasefiltergraph();
}

static void test_filter_graph2(void)
{
    HRESULT hr;
    IFilterGraph2 *pF = NULL;

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterGraph2, (LPVOID*)&pF);
    ok(hr == S_OK, "CoCreateInstance failed with %x\n", hr);
    ok(pF != NULL, "pF is NULL\n");

    hr = IFilterGraph2_Release(pF);
    ok(hr == 0, "IFilterGraph2_Release returned: %x\n", hr);
}

/* IEnumMediaTypes implementation (supporting code for Render() test.) */
static void FreeMediaType(AM_MEDIA_TYPE * pMediaType)
{
    if (pMediaType->pbFormat)
    {
        CoTaskMemFree(pMediaType->pbFormat);
        pMediaType->pbFormat = NULL;
    }
    if (pMediaType->pUnk)
    {
        IUnknown_Release(pMediaType->pUnk);
        pMediaType->pUnk = NULL;
    }
}

static HRESULT CopyMediaType(AM_MEDIA_TYPE * pDest, const AM_MEDIA_TYPE *pSrc)
{
    *pDest = *pSrc;
    if (!pSrc->pbFormat) return S_OK;
    if (!(pDest->pbFormat = CoTaskMemAlloc(pSrc->cbFormat)))
        return E_OUTOFMEMORY;
    memcpy(pDest->pbFormat, pSrc->pbFormat, pSrc->cbFormat);
    if (pDest->pUnk)
        IUnknown_AddRef(pDest->pUnk);
    return S_OK;
}

static AM_MEDIA_TYPE * CreateMediaType(AM_MEDIA_TYPE const * pSrc)
{
    AM_MEDIA_TYPE * pDest;

    pDest = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    if (!pDest)
        return NULL;

    if (FAILED(CopyMediaType(pDest, pSrc)))
    {
        CoTaskMemFree(pDest);
	return NULL;
    }

    return pDest;
}

static BOOL CompareMediaTypes(const AM_MEDIA_TYPE * pmt1, const AM_MEDIA_TYPE * pmt2, BOOL bWildcards)
{
    return (((bWildcards && (IsEqualGUID(&pmt1->majortype, &GUID_NULL) || IsEqualGUID(&pmt2->majortype, &GUID_NULL))) || IsEqualGUID(&pmt1->majortype, &pmt2->majortype)) &&
            ((bWildcards && (IsEqualGUID(&pmt1->subtype, &GUID_NULL)   || IsEqualGUID(&pmt2->subtype, &GUID_NULL)))   || IsEqualGUID(&pmt1->subtype, &pmt2->subtype)));
}

static void DeleteMediaType(AM_MEDIA_TYPE * pMediaType)
{
    FreeMediaType(pMediaType);
    CoTaskMemFree(pMediaType);
}

typedef struct IEnumMediaTypesImpl
{
    IEnumMediaTypes IEnumMediaTypes_iface;
    LONG refCount;
    AM_MEDIA_TYPE *pMediaTypes;
    ULONG cMediaTypes;
    ULONG uIndex;
} IEnumMediaTypesImpl;

static const struct IEnumMediaTypesVtbl IEnumMediaTypesImpl_Vtbl;

static inline IEnumMediaTypesImpl *impl_from_IEnumMediaTypes(IEnumMediaTypes *iface)
{
    return CONTAINING_RECORD(iface, IEnumMediaTypesImpl, IEnumMediaTypes_iface);
}

static HRESULT IEnumMediaTypesImpl_Construct(const AM_MEDIA_TYPE * pMediaTypes, ULONG cMediaTypes, IEnumMediaTypes ** ppEnum)
{
    ULONG i;
    IEnumMediaTypesImpl * pEnumMediaTypes = CoTaskMemAlloc(sizeof(IEnumMediaTypesImpl));

    if (!pEnumMediaTypes)
    {
        *ppEnum = NULL;
        return E_OUTOFMEMORY;
    }
    pEnumMediaTypes->IEnumMediaTypes_iface.lpVtbl = &IEnumMediaTypesImpl_Vtbl;
    pEnumMediaTypes->refCount = 1;
    pEnumMediaTypes->uIndex = 0;
    pEnumMediaTypes->cMediaTypes = cMediaTypes;
    pEnumMediaTypes->pMediaTypes = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE) * cMediaTypes);
    for (i = 0; i < cMediaTypes; i++)
        if (FAILED(CopyMediaType(&pEnumMediaTypes->pMediaTypes[i], &pMediaTypes[i])))
        {
           while (i--)
              FreeMediaType(&pEnumMediaTypes->pMediaTypes[i]);
           CoTaskMemFree(pEnumMediaTypes->pMediaTypes);
           return E_OUTOFMEMORY;
        }
    *ppEnum = &pEnumMediaTypes->IEnumMediaTypes_iface;
    return S_OK;
}

static HRESULT WINAPI IEnumMediaTypesImpl_QueryInterface(IEnumMediaTypes * iface, REFIID riid, LPVOID * ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IEnumMediaTypes))
        *ppv = iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI IEnumMediaTypesImpl_AddRef(IEnumMediaTypes * iface)
{
    IEnumMediaTypesImpl *This = impl_from_IEnumMediaTypes(iface);
    ULONG refCount = InterlockedIncrement(&This->refCount);

    return refCount;
}

static ULONG WINAPI IEnumMediaTypesImpl_Release(IEnumMediaTypes * iface)
{
    IEnumMediaTypesImpl *This = impl_from_IEnumMediaTypes(iface);
    ULONG refCount = InterlockedDecrement(&This->refCount);

    if (!refCount)
    {
        int i;
        for (i = 0; i < This->cMediaTypes; i++)
            FreeMediaType(&This->pMediaTypes[i]);
        CoTaskMemFree(This->pMediaTypes);
        CoTaskMemFree(This);
    }
    return refCount;
}

static HRESULT WINAPI IEnumMediaTypesImpl_Next(IEnumMediaTypes * iface, ULONG cMediaTypes, AM_MEDIA_TYPE ** ppMediaTypes, ULONG * pcFetched)
{
    ULONG cFetched;
    IEnumMediaTypesImpl *This = impl_from_IEnumMediaTypes(iface);

    cFetched = min(This->cMediaTypes, This->uIndex + cMediaTypes) - This->uIndex;

    if (cFetched > 0)
    {
        ULONG i;
        for (i = 0; i < cFetched; i++)
            if (!(ppMediaTypes[i] = CreateMediaType(&This->pMediaTypes[This->uIndex + i])))
            {
                while (i--)
                    DeleteMediaType(ppMediaTypes[i]);
                *pcFetched = 0;
                return E_OUTOFMEMORY;
            }
    }

    if ((cMediaTypes != 1) || pcFetched)
        *pcFetched = cFetched;

    This->uIndex += cFetched;

    if (cFetched != cMediaTypes)
        return S_FALSE;
    return S_OK;
}

static HRESULT WINAPI IEnumMediaTypesImpl_Skip(IEnumMediaTypes * iface, ULONG cMediaTypes)
{
    IEnumMediaTypesImpl *This = impl_from_IEnumMediaTypes(iface);

    if (This->uIndex + cMediaTypes < This->cMediaTypes)
    {
        This->uIndex += cMediaTypes;
        return S_OK;
    }
    return S_FALSE;
}

static HRESULT WINAPI IEnumMediaTypesImpl_Reset(IEnumMediaTypes * iface)
{
    IEnumMediaTypesImpl *This = impl_from_IEnumMediaTypes(iface);

    This->uIndex = 0;
    return S_OK;
}

static HRESULT WINAPI IEnumMediaTypesImpl_Clone(IEnumMediaTypes * iface, IEnumMediaTypes ** ppEnum)
{
    HRESULT hr;
    IEnumMediaTypesImpl *This = impl_from_IEnumMediaTypes(iface);

    hr = IEnumMediaTypesImpl_Construct(This->pMediaTypes, This->cMediaTypes, ppEnum);
    if (FAILED(hr))
        return hr;
    return IEnumMediaTypes_Skip(*ppEnum, This->uIndex);
}

static const IEnumMediaTypesVtbl IEnumMediaTypesImpl_Vtbl =
{
    IEnumMediaTypesImpl_QueryInterface,
    IEnumMediaTypesImpl_AddRef,
    IEnumMediaTypesImpl_Release,
    IEnumMediaTypesImpl_Next,
    IEnumMediaTypesImpl_Skip,
    IEnumMediaTypesImpl_Reset,
    IEnumMediaTypesImpl_Clone
};

/* Implementation of a very stripped down pin for the test filter. Just enough
   functionality for connecting and Render() to work. */

static void Copy_PinInfo(PIN_INFO * pDest, const PIN_INFO * pSrc)
{
    lstrcpyW(pDest->achName, pSrc->achName);
    pDest->dir = pSrc->dir;
    pDest->pFilter = pSrc->pFilter;
}

typedef struct ITestPinImpl
{
    IPin IPin_iface;
    LONG refCount;
    LPCRITICAL_SECTION pCritSec;
    PIN_INFO pinInfo;
    IPin * pConnectedTo;
    AM_MEDIA_TYPE mtCurrent;
    LPVOID pUserData;
} ITestPinImpl;

static inline ITestPinImpl *impl_from_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, ITestPinImpl, IPin_iface);
}

static HRESULT WINAPI  TestFilter_Pin_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IPin))
        *ppv = iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI TestFilter_Pin_AddRef(IPin * iface)
{
    ITestPinImpl *This = impl_from_IPin(iface);
    ULONG refCount = InterlockedIncrement(&This->refCount);
    return refCount;
}

static ULONG WINAPI TestFilter_Pin_Release(IPin * iface)
{
    ITestPinImpl *This = impl_from_IPin(iface);
    ULONG refCount = InterlockedDecrement(&This->refCount);

    if (!refCount)
    {
        FreeMediaType(&This->mtCurrent);
        CoTaskMemFree(This);
        return 0;
    }
    else
        return refCount;
}

static HRESULT WINAPI TestFilter_InputPin_Connect(IPin * iface, IPin * pConnector, const AM_MEDIA_TYPE * pmt)
{
    return E_UNEXPECTED;
}

static HRESULT WINAPI TestFilter_InputPin_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    ITestPinImpl *This = impl_from_IPin(iface);
    PIN_DIRECTION pindirReceive;
    HRESULT hr = S_OK;

    EnterCriticalSection(This->pCritSec);
    {
        if (!(IsEqualIID(&pmt->majortype, &This->mtCurrent.majortype) && (IsEqualIID(&pmt->subtype, &This->mtCurrent.subtype) ||
                                                                          IsEqualIID(&GUID_NULL, &This->mtCurrent.subtype))))
            hr = VFW_E_TYPE_NOT_ACCEPTED;

        if (This->pConnectedTo)
            hr = VFW_E_ALREADY_CONNECTED;

        if (SUCCEEDED(hr))
        {
            IPin_QueryDirection(pReceivePin, &pindirReceive);

            if (pindirReceive != PINDIR_OUTPUT)
            {
                hr = VFW_E_INVALID_DIRECTION;
            }
        }

        if (SUCCEEDED(hr))
        {
            CopyMediaType(&This->mtCurrent, pmt);
            This->pConnectedTo = pReceivePin;
            IPin_AddRef(pReceivePin);
        }
    }
    LeaveCriticalSection(This->pCritSec);

    return hr;
}

static HRESULT WINAPI TestFilter_Pin_Disconnect(IPin * iface)
{
    HRESULT hr;
    ITestPinImpl *This = impl_from_IPin(iface);

    EnterCriticalSection(This->pCritSec);
    {
        if (This->pConnectedTo)
        {
            IPin_Release(This->pConnectedTo);
            This->pConnectedTo = NULL;
            hr = S_OK;
        }
        else
            hr = S_FALSE;
    }
    LeaveCriticalSection(This->pCritSec);

    return hr;
}

static HRESULT WINAPI TestFilter_Pin_ConnectedTo(IPin * iface, IPin ** ppPin)
{
    HRESULT hr;
    ITestPinImpl *This = impl_from_IPin(iface);

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

static HRESULT WINAPI TestFilter_Pin_ConnectionMediaType(IPin * iface, AM_MEDIA_TYPE * pmt)
{
    HRESULT hr;
    ITestPinImpl *This = impl_from_IPin(iface);

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

static HRESULT WINAPI TestFilter_Pin_QueryPinInfo(IPin * iface, PIN_INFO * pInfo)
{
    ITestPinImpl *This = impl_from_IPin(iface);

    Copy_PinInfo(pInfo, &This->pinInfo);
    IBaseFilter_AddRef(pInfo->pFilter);

    return S_OK;
}

static HRESULT WINAPI TestFilter_Pin_QueryDirection(IPin * iface, PIN_DIRECTION * pPinDir)
{
    ITestPinImpl *This = impl_from_IPin(iface);

    *pPinDir = This->pinInfo.dir;

    return S_OK;
}

static HRESULT WINAPI TestFilter_Pin_QueryId(IPin * iface, LPWSTR * Id)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI TestFilter_Pin_QueryAccept(IPin * iface, const AM_MEDIA_TYPE * pmt)
{
    ITestPinImpl *This = impl_from_IPin(iface);

    if (IsEqualIID(&pmt->majortype, &This->mtCurrent.majortype) && (IsEqualIID(&pmt->subtype, &This->mtCurrent.subtype) ||
                                                                    IsEqualIID(&GUID_NULL, &This->mtCurrent.subtype)))
        return S_OK;
    else
        return VFW_E_TYPE_NOT_ACCEPTED;
}

static HRESULT WINAPI TestFilter_Pin_EnumMediaTypes(IPin * iface, IEnumMediaTypes ** ppEnum)
{
    ITestPinImpl *This = impl_from_IPin(iface);

    return IEnumMediaTypesImpl_Construct(&This->mtCurrent, 1, ppEnum);
}

static HRESULT WINAPI  TestFilter_Pin_QueryInternalConnections(IPin * iface, IPin ** apPin, ULONG * cPin)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI TestFilter_Pin_BeginFlush(IPin * iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI TestFilter_Pin_EndFlush(IPin * iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI TestFilter_Pin_NewSegment(IPin * iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI TestFilter_Pin_EndOfStream(IPin * iface)
{
    return E_NOTIMPL;
}

static const IPinVtbl TestFilter_InputPin_Vtbl =
{
    TestFilter_Pin_QueryInterface,
    TestFilter_Pin_AddRef,
    TestFilter_Pin_Release,
    TestFilter_InputPin_Connect,
    TestFilter_InputPin_ReceiveConnection,
    TestFilter_Pin_Disconnect,
    TestFilter_Pin_ConnectedTo,
    TestFilter_Pin_ConnectionMediaType,
    TestFilter_Pin_QueryPinInfo,
    TestFilter_Pin_QueryDirection,
    TestFilter_Pin_QueryId,
    TestFilter_Pin_QueryAccept,
    TestFilter_Pin_EnumMediaTypes,
    TestFilter_Pin_QueryInternalConnections,
    TestFilter_Pin_EndOfStream,
    TestFilter_Pin_BeginFlush,
    TestFilter_Pin_EndFlush,
    TestFilter_Pin_NewSegment
};

static HRESULT WINAPI TestFilter_OutputPin_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    return E_UNEXPECTED;
}

/* Private helper function */
static HRESULT TestFilter_OutputPin_ConnectSpecific(ITestPinImpl * This, IPin * pReceivePin,
        const AM_MEDIA_TYPE * pmt)
{
    HRESULT hr;

    This->pConnectedTo = pReceivePin;
    IPin_AddRef(pReceivePin);

    hr = IPin_ReceiveConnection(pReceivePin, &This->IPin_iface, pmt);

    if (FAILED(hr))
    {
        IPin_Release(This->pConnectedTo);
        This->pConnectedTo = NULL;
    }

    return hr;
}

static HRESULT WINAPI TestFilter_OutputPin_Connect(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    ITestPinImpl *This = impl_from_IPin(iface);
    HRESULT hr;

    EnterCriticalSection(This->pCritSec);
    {
        /* if we have been a specific type to connect with, then we can either connect
         * with that or fail. We cannot choose different AM_MEDIA_TYPE */
        if (pmt && !IsEqualGUID(&pmt->majortype, &GUID_NULL) && !IsEqualGUID(&pmt->subtype, &GUID_NULL))
            hr = TestFilter_OutputPin_ConnectSpecific(This, pReceivePin, pmt);
        else
        {
            if (( !pmt || CompareMediaTypes(pmt, &This->mtCurrent, TRUE) ) &&
                (TestFilter_OutputPin_ConnectSpecific(This, pReceivePin, &This->mtCurrent) == S_OK))
                        hr = S_OK;
            else hr = VFW_E_NO_ACCEPTABLE_TYPES;
        } /* if negotiate media type */
    } /* if succeeded */
    LeaveCriticalSection(This->pCritSec);

    return hr;
}

static const IPinVtbl TestFilter_OutputPin_Vtbl =
{
    TestFilter_Pin_QueryInterface,
    TestFilter_Pin_AddRef,
    TestFilter_Pin_Release,
    TestFilter_OutputPin_Connect,
    TestFilter_OutputPin_ReceiveConnection,
    TestFilter_Pin_Disconnect,
    TestFilter_Pin_ConnectedTo,
    TestFilter_Pin_ConnectionMediaType,
    TestFilter_Pin_QueryPinInfo,
    TestFilter_Pin_QueryDirection,
    TestFilter_Pin_QueryId,
    TestFilter_Pin_QueryAccept,
    TestFilter_Pin_EnumMediaTypes,
    TestFilter_Pin_QueryInternalConnections,
    TestFilter_Pin_EndOfStream,
    TestFilter_Pin_BeginFlush,
    TestFilter_Pin_EndFlush,
    TestFilter_Pin_NewSegment
};

static HRESULT TestFilter_Pin_Construct(const IPinVtbl *Pin_Vtbl, const PIN_INFO * pPinInfo, AM_MEDIA_TYPE *pinmt,
                                        LPCRITICAL_SECTION pCritSec, IPin ** ppPin)
{
    ITestPinImpl * pPinImpl;

    *ppPin = NULL;

    pPinImpl = CoTaskMemAlloc(sizeof(ITestPinImpl));

    if (!pPinImpl)
        return E_OUTOFMEMORY;

    pPinImpl->refCount = 1;
    pPinImpl->pConnectedTo = NULL;
    pPinImpl->pCritSec = pCritSec;
    Copy_PinInfo(&pPinImpl->pinInfo, pPinInfo);
    pPinImpl->mtCurrent = *pinmt;

    pPinImpl->IPin_iface.lpVtbl = Pin_Vtbl;

    *ppPin = &pPinImpl->IPin_iface;
    return S_OK;
}

/* IEnumPins implementation */

typedef HRESULT (* FNOBTAINPIN)(TestFilterImpl *tf, ULONG pos, IPin **pin, DWORD *lastsynctick);

typedef struct IEnumPinsImpl
{
    IEnumPins IEnumPins_iface;
    LONG refCount;
    ULONG uIndex;
    TestFilterImpl *base;
    FNOBTAINPIN receive_pin;
    DWORD synctime;
} IEnumPinsImpl;

static const struct IEnumPinsVtbl IEnumPinsImpl_Vtbl;

static inline IEnumPinsImpl *impl_from_IEnumPins(IEnumPins *iface)
{
    return CONTAINING_RECORD(iface, IEnumPinsImpl, IEnumPins_iface);
}

static HRESULT createenumpins(IEnumPins ** ppEnum, FNOBTAINPIN receive_pin, TestFilterImpl *base)
{
    IEnumPinsImpl * pEnumPins;

    if (!ppEnum)
        return E_POINTER;

    pEnumPins = CoTaskMemAlloc(sizeof(IEnumPinsImpl));
    if (!pEnumPins)
    {
        *ppEnum = NULL;
        return E_OUTOFMEMORY;
    }
    pEnumPins->IEnumPins_iface.lpVtbl = &IEnumPinsImpl_Vtbl;
    pEnumPins->refCount = 1;
    pEnumPins->uIndex = 0;
    pEnumPins->receive_pin = receive_pin;
    pEnumPins->base = base;
    IBaseFilter_AddRef(&base->IBaseFilter_iface);
    *ppEnum = &pEnumPins->IEnumPins_iface;

    receive_pin(base, ~0, NULL, &pEnumPins->synctime);

    return S_OK;
}

static HRESULT WINAPI IEnumPinsImpl_QueryInterface(IEnumPins * iface, REFIID riid, LPVOID * ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IEnumPins))
        *ppv = iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI IEnumPinsImpl_AddRef(IEnumPins * iface)
{
    IEnumPinsImpl *This = impl_from_IEnumPins(iface);
    ULONG refCount = InterlockedIncrement(&This->refCount);

    return refCount;
}

static ULONG WINAPI IEnumPinsImpl_Release(IEnumPins * iface)
{
    IEnumPinsImpl *This = impl_from_IEnumPins(iface);
    ULONG refCount = InterlockedDecrement(&This->refCount);

    if (!refCount)
    {
        IBaseFilter_Release(&This->base->IBaseFilter_iface);
        CoTaskMemFree(This);
        return 0;
    }
    else
        return refCount;
}

static HRESULT WINAPI IEnumPinsImpl_Next(IEnumPins * iface, ULONG cPins, IPin ** ppPins, ULONG * pcFetched)
{
    IEnumPinsImpl *This = impl_from_IEnumPins(iface);
    DWORD synctime = This->synctime;
    HRESULT hr = S_OK;
    ULONG i = 0;

    if (!ppPins)
        return E_POINTER;

    if (cPins > 1 && !pcFetched)
        return E_INVALIDARG;

    if (pcFetched)
        *pcFetched = 0;

    while (i < cPins && hr == S_OK)
    {
        hr = This->receive_pin(This->base, This->uIndex + i, &ppPins[i], &synctime);

        if (hr == S_OK)
            ++i;

        if (synctime != This->synctime)
            break;
    }

    if (!i && synctime != This->synctime)
        return VFW_E_ENUM_OUT_OF_SYNC;

    if (pcFetched)
        *pcFetched = i;
    This->uIndex += i;

    if (i < cPins)
        return S_FALSE;
    return S_OK;
}

static HRESULT WINAPI IEnumPinsImpl_Skip(IEnumPins * iface, ULONG cPins)
{
    IEnumPinsImpl *This = impl_from_IEnumPins(iface);
    DWORD synctime = This->synctime;
    HRESULT hr;
    IPin *pin = NULL;

    hr = This->receive_pin(This->base, This->uIndex + cPins, &pin, &synctime);
    if (pin)
        IPin_Release(pin);

    if (synctime != This->synctime)
        return VFW_E_ENUM_OUT_OF_SYNC;

    if (hr == S_OK)
        This->uIndex += cPins;

    return hr;
}

static HRESULT WINAPI IEnumPinsImpl_Reset(IEnumPins * iface)
{
    IEnumPinsImpl *This = impl_from_IEnumPins(iface);

    This->receive_pin(This->base, ~0, NULL, &This->synctime);

    This->uIndex = 0;
    return S_OK;
}

static HRESULT WINAPI IEnumPinsImpl_Clone(IEnumPins * iface, IEnumPins ** ppEnum)
{
    HRESULT hr;
    IEnumPinsImpl *This = impl_from_IEnumPins(iface);

    hr = createenumpins(ppEnum, This->receive_pin, This->base);
    if (FAILED(hr))
        return hr;
    return IEnumPins_Skip(*ppEnum, This->uIndex);
}

static const IEnumPinsVtbl IEnumPinsImpl_Vtbl =
{
    IEnumPinsImpl_QueryInterface,
    IEnumPinsImpl_AddRef,
    IEnumPinsImpl_Release,
    IEnumPinsImpl_Next,
    IEnumPinsImpl_Skip,
    IEnumPinsImpl_Reset,
    IEnumPinsImpl_Clone
};

/* Test filter implementation - a filter that has few predefined pins with single media type
 * that accept only this single media type. Enough for Render(). */

typedef struct TestFilterPinData
{
PIN_DIRECTION pinDir;
const GUID *mediasubtype;
} TestFilterPinData;

static const IBaseFilterVtbl TestFilter_Vtbl;

static inline TestFilterImpl *impl_from_IBaseFilter(IBaseFilter *iface)
{
    return CONTAINING_RECORD(iface, TestFilterImpl, IBaseFilter_iface);
}

static HRESULT createtestfilter(const CLSID* pClsid, const TestFilterPinData *pinData,
        TestFilterImpl **tf)
{
    static const WCHAR wcsInputPinName[] = {'i','n','p','u','t',' ','p','i','n',0};
    static const WCHAR wcsOutputPinName[] = {'o','u','t','p','u','t',' ','p','i','n',0};
    HRESULT hr;
    PIN_INFO pinInfo;
    TestFilterImpl* pTestFilter = NULL;
    UINT nPins, i;
    AM_MEDIA_TYPE mt;

    pTestFilter = CoTaskMemAlloc(sizeof(TestFilterImpl));
    if (!pTestFilter) return E_OUTOFMEMORY;

    pTestFilter->clsid = *pClsid;
    pTestFilter->IBaseFilter_iface.lpVtbl = &TestFilter_Vtbl;
    pTestFilter->refCount = 1;
    InitializeCriticalSection(&pTestFilter->csFilter);
    pTestFilter->state = State_Stopped;

    ZeroMemory(&pTestFilter->filterInfo, sizeof(FILTER_INFO));

    nPins = 0;
    while(pinData[nPins].mediasubtype) ++nPins;

    pTestFilter->ppPins = CoTaskMemAlloc(nPins * sizeof(IPin *));
    if (!pTestFilter->ppPins)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    ZeroMemory(pTestFilter->ppPins, nPins * sizeof(IPin *));

    for (i = 0; i < nPins; i++)
    {
        ZeroMemory(&mt, sizeof(mt));
        mt.majortype = MEDIATYPE_Video;
        mt.formattype = FORMAT_None;
        mt.subtype = *pinData[i].mediasubtype;

        pinInfo.dir = pinData[i].pinDir;
        pinInfo.pFilter = &pTestFilter->IBaseFilter_iface;
        if (pinInfo.dir == PINDIR_INPUT)
        {
            lstrcpynW(pinInfo.achName, wcsInputPinName, sizeof(pinInfo.achName) / sizeof(pinInfo.achName[0]));
            hr = TestFilter_Pin_Construct(&TestFilter_InputPin_Vtbl, &pinInfo, &mt, &pTestFilter->csFilter,
                &pTestFilter->ppPins[i]);

        }
        else
        {
            lstrcpynW(pinInfo.achName, wcsOutputPinName, sizeof(pinInfo.achName) / sizeof(pinInfo.achName[0]));
            hr = TestFilter_Pin_Construct(&TestFilter_OutputPin_Vtbl, &pinInfo, &mt, &pTestFilter->csFilter,
                 &pTestFilter->ppPins[i]);
        }
        if (FAILED(hr) || !pTestFilter->ppPins[i]) goto error;
    }

    pTestFilter->nPins = nPins;
    *tf = pTestFilter;
    return S_OK;

    error:

    if (pTestFilter->ppPins)
    {
        for (i = 0; i < nPins; i++)
        {
            if (pTestFilter->ppPins[i]) IPin_Release(pTestFilter->ppPins[i]);
        }
    }
    CoTaskMemFree(pTestFilter->ppPins);
    DeleteCriticalSection(&pTestFilter->csFilter);
    CoTaskMemFree(pTestFilter);

    return hr;
}

static HRESULT WINAPI TestFilter_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv)
{
    TestFilterImpl *This = impl_from_IBaseFilter(iface);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = This;
    else if (IsEqualIID(riid, &IID_IPersist))
        *ppv = This;
    else if (IsEqualIID(riid, &IID_IMediaFilter))
        *ppv = This;
    else if (IsEqualIID(riid, &IID_IBaseFilter))
        *ppv = This;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI TestFilter_AddRef(IBaseFilter * iface)
{
    TestFilterImpl *This = impl_from_IBaseFilter(iface);
    ULONG refCount = InterlockedIncrement(&This->refCount);

    return refCount;
}

static ULONG WINAPI TestFilter_Release(IBaseFilter * iface)
{
    TestFilterImpl *This = impl_from_IBaseFilter(iface);
    ULONG refCount = InterlockedDecrement(&This->refCount);

    if (!refCount)
    {
        ULONG i;

        for (i = 0; i < This->nPins; i++)
        {
            IPin *pConnectedTo;

            if (SUCCEEDED(IPin_ConnectedTo(This->ppPins[i], &pConnectedTo)))
            {
                IPin_Disconnect(pConnectedTo);
                IPin_Release(pConnectedTo);
            }
            IPin_Disconnect(This->ppPins[i]);

            IPin_Release(This->ppPins[i]);
        }

        CoTaskMemFree(This->ppPins);

        DeleteCriticalSection(&This->csFilter);

        CoTaskMemFree(This);

        return 0;
    }
    else
        return refCount;
}
/** IPersist methods **/

static HRESULT WINAPI TestFilter_GetClassID(IBaseFilter * iface, CLSID * pClsid)
{
    TestFilterImpl *This = impl_from_IBaseFilter(iface);

    *pClsid = This->clsid;

    return S_OK;
}

/** IMediaFilter methods **/

static HRESULT WINAPI TestFilter_Stop(IBaseFilter * iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI TestFilter_Pause(IBaseFilter * iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI TestFilter_Run(IBaseFilter * iface, REFERENCE_TIME tStart)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI TestFilter_GetState(IBaseFilter * iface, DWORD dwMilliSecsTimeout, FILTER_STATE *pState)
{
    TestFilterImpl *This = impl_from_IBaseFilter(iface);

    EnterCriticalSection(&This->csFilter);
    {
        *pState = This->state;
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI TestFilter_SetSyncSource(IBaseFilter * iface, IReferenceClock *pClock)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI TestFilter_GetSyncSource(IBaseFilter * iface, IReferenceClock **ppClock)
{
    return E_NOTIMPL;
}

/** IBaseFilter implementation **/

static HRESULT getpin_callback(TestFilterImpl *tf, ULONG pos, IPin **pin, DWORD *lastsynctick)
{
    /* Our pins are static, not changing so setting static tick count is ok */
    *lastsynctick = 0;

    if (pos >= tf->nPins)
        return S_FALSE;

    *pin = tf->ppPins[pos];
    IPin_AddRef(*pin);
    return S_OK;
}

static HRESULT WINAPI TestFilter_EnumPins(IBaseFilter * iface, IEnumPins **ppEnum)
{
    TestFilterImpl *This = impl_from_IBaseFilter(iface);

    return createenumpins(ppEnum, getpin_callback, This);
}

static HRESULT WINAPI TestFilter_FindPin(IBaseFilter * iface, LPCWSTR Id, IPin **ppPin)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI TestFilter_QueryFilterInfo(IBaseFilter * iface, FILTER_INFO *pInfo)
{
    TestFilterImpl *This = impl_from_IBaseFilter(iface);

    lstrcpyW(pInfo->achName, This->filterInfo.achName);
    pInfo->pGraph = This->filterInfo.pGraph;

    if (pInfo->pGraph)
        IFilterGraph_AddRef(pInfo->pGraph);

    return S_OK;
}

static HRESULT WINAPI TestFilter_JoinFilterGraph(IBaseFilter * iface, IFilterGraph *pGraph, LPCWSTR pName)
{
    HRESULT hr = S_OK;
    TestFilterImpl *This = impl_from_IBaseFilter(iface);

    EnterCriticalSection(&This->csFilter);
    {
        if (pName)
            lstrcpyW(This->filterInfo.achName, pName);
        else
            *This->filterInfo.achName = '\0';
        This->filterInfo.pGraph = pGraph; /* NOTE: do NOT increase ref. count */
    }
    LeaveCriticalSection(&This->csFilter);

    return hr;
}

static HRESULT WINAPI TestFilter_QueryVendorInfo(IBaseFilter * iface, LPWSTR *pVendorInfo)
{
    return E_NOTIMPL;
}

static const IBaseFilterVtbl TestFilter_Vtbl =
{
    TestFilter_QueryInterface,
    TestFilter_AddRef,
    TestFilter_Release,
    TestFilter_GetClassID,
    TestFilter_Stop,
    TestFilter_Pause,
    TestFilter_Run,
    TestFilter_GetState,
    TestFilter_SetSyncSource,
    TestFilter_GetSyncSource,
    TestFilter_EnumPins,
    TestFilter_FindPin,
    TestFilter_QueryFilterInfo,
    TestFilter_JoinFilterGraph,
    TestFilter_QueryVendorInfo
};

/* IClassFactory implementation */

typedef struct TestClassFactoryImpl
{
    IClassFactory IClassFactory_iface;
    const TestFilterPinData *filterPinData;
    const CLSID *clsid;
} TestClassFactoryImpl;

static inline TestClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, TestClassFactoryImpl, IClassFactory_iface);
}

static HRESULT WINAPI Test_IClassFactory_QueryInterface(
    LPCLASSFACTORY iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IClassFactory))
    {
        *ppvObj = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Test_IClassFactory_AddRef(LPCLASSFACTORY iface)
{
    return 2; /* non-heap-based object */
}

static ULONG WINAPI Test_IClassFactory_Release(LPCLASSFACTORY iface)
{
    return 1; /* non-heap-based object */
}

static HRESULT WINAPI Test_IClassFactory_CreateInstance(
    LPCLASSFACTORY iface,
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID *ppvObj)
{
    TestClassFactoryImpl *This = impl_from_IClassFactory(iface);
    HRESULT hr;
    TestFilterImpl *testfilter;

    *ppvObj = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    hr = createtestfilter(This->clsid, This->filterPinData, &testfilter);
    if (SUCCEEDED(hr)) {
        hr = IBaseFilter_QueryInterface(&testfilter->IBaseFilter_iface, riid, ppvObj);
        IBaseFilter_Release(&testfilter->IBaseFilter_iface);
    }
    return hr;
}

static HRESULT WINAPI Test_IClassFactory_LockServer(
    LPCLASSFACTORY iface,
    BOOL fLock)
{
    return S_OK;
}

static IClassFactoryVtbl TestClassFactory_Vtbl =
{
    Test_IClassFactory_QueryInterface,
    Test_IClassFactory_AddRef,
    Test_IClassFactory_Release,
    Test_IClassFactory_CreateInstance,
    Test_IClassFactory_LockServer
};

static HRESULT get_connected_filter_name(TestFilterImpl *pFilter, char *FilterName)
{
    IPin *pin = NULL;
    PIN_INFO pinInfo;
    FILTER_INFO filterInfo;
    HRESULT hr;

    FilterName[0] = 0;

    hr = IPin_ConnectedTo(pFilter->ppPins[0], &pin);
    ok(hr == S_OK, "IPin_ConnectedTo failed with %x\n", hr);
    if (FAILED(hr)) return hr;

    hr = IPin_QueryPinInfo(pin, &pinInfo);
    ok(hr == S_OK, "IPin_QueryPinInfo failed with %x\n", hr);
    IPin_Release(pin);
    if (FAILED(hr)) return hr;

    SetLastError(0xdeadbeef);
    hr = IBaseFilter_QueryFilterInfo(pinInfo.pFilter, &filterInfo);
    if (hr == S_OK && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        IBaseFilter_Release(pinInfo.pFilter);
        return E_NOTIMPL;
    }
    ok(hr == S_OK, "IBaseFilter_QueryFilterInfo failed with %x\n", hr);
    IBaseFilter_Release(pinInfo.pFilter);
    if (FAILED(hr)) return hr;

    IFilterGraph_Release(filterInfo.pGraph);

    WideCharToMultiByte(CP_ACP, 0, filterInfo.achName, -1, FilterName, MAX_FILTER_NAME, NULL, NULL);

    return S_OK;
}

static void test_render_filter_priority(void)
{
    /* Tests filter choice priorities in Render(). */
    DWORD cookie1 = 0, cookie2 = 0, cookie3 = 0;
    HRESULT hr;
    IFilterGraph2* pgraph2 = NULL;
    IFilterMapper2 *pMapper2 = NULL;
    TestFilterImpl *ptestfilter = NULL;
    TestFilterImpl *ptestfilter2 = NULL;
    static const CLSID CLSID_TestFilter2 = {
        0x37a4edb0,
        0x4d13,
        0x11dd,
        {0xe8, 0x9b, 0x00, 0x19, 0x66, 0x2f, 0xf0, 0xce}
    };
    static const CLSID CLSID_TestFilter3 = {
        0x37a4f2d8,
        0x4d13,
        0x11dd,
        {0xe8, 0x9b, 0x00, 0x19, 0x66, 0x2f, 0xf0, 0xce}
    };
    static const CLSID CLSID_TestFilter4 = {
        0x37a4f3b4,
        0x4d13,
        0x11dd,
        {0xe8, 0x9b, 0x00, 0x19, 0x66, 0x2f, 0xf0, 0xce}
    };
    static const GUID mediasubtype1 = {
        0x37a4f51c,
        0x4d13,
        0x11dd,
        {0xe8, 0x9b, 0x00, 0x19, 0x66, 0x2f, 0xf0, 0xce}
    };
    static const GUID mediasubtype2 = {
        0x37a4f5c6,
        0x4d13,
        0x11dd,
        {0xe8, 0x9b, 0x00, 0x19, 0x66, 0x2f, 0xf0, 0xce}
    };
    static const TestFilterPinData PinData1[] = {
            { PINDIR_OUTPUT, &mediasubtype1 },
            { 0, 0 }
        };
    static const TestFilterPinData PinData2[] = {
            { PINDIR_INPUT,  &mediasubtype1 },
            { 0, 0 }
        };
    static const TestFilterPinData PinData3[] = {
            { PINDIR_INPUT,  &GUID_NULL },
            { 0, 0 }
        };
    static const TestFilterPinData PinData4[] = {
            { PINDIR_INPUT,  &mediasubtype1 },
            { PINDIR_OUTPUT, &mediasubtype2 },
            { 0, 0 }
        };
    static const TestFilterPinData PinData5[] = {
            { PINDIR_INPUT,  &mediasubtype2 },
            { 0, 0 }
        };
    TestClassFactoryImpl Filter1ClassFactory = {
            { &TestClassFactory_Vtbl },
            PinData2, &CLSID_TestFilter2
        };
    TestClassFactoryImpl Filter2ClassFactory = {
            { &TestClassFactory_Vtbl },
            PinData4, &CLSID_TestFilter3
        };
    TestClassFactoryImpl Filter3ClassFactory = {
            { &TestClassFactory_Vtbl },
            PinData5, &CLSID_TestFilter4
        };
    char ConnectedFilterName1[MAX_FILTER_NAME];
    char ConnectedFilterName2[MAX_FILTER_NAME];
    REGFILTER2 rgf2;
    REGFILTERPINS2 rgPins2[2];
    REGPINTYPES rgPinType[2];
    static const WCHAR wszFilterInstanceName1[] = {'T', 'e', 's', 't', 'f', 'i', 'l', 't', 'e', 'r', 'I',
                                                        'n', 's', 't', 'a', 'n', 'c', 'e', '1', 0 };
    static const WCHAR wszFilterInstanceName2[] = {'T', 'e', 's', 't', 'f', 'i', 'l', 't', 'e', 'r', 'I',
                                                        'n', 's', 't', 'a', 'n', 'c', 'e', '2', 0 };
    static const WCHAR wszFilterInstanceName3[] = {'T', 'e', 's', 't', 'f', 'i', 'l', 't', 'e', 'r', 'I',
                                                        'n', 's', 't', 'a', 'n', 'c', 'e', '3', 0 };
    static const WCHAR wszFilterInstanceName4[] = {'T', 'e', 's', 't', 'f', 'i', 'l', 't', 'e', 'r', 'I',
                                                        'n', 's', 't', 'a', 'n', 'c', 'e', '4', 0 };

    /* Test which renderer of two already added to the graph will be chosen
     * (one is "exact" match, other is "wildcard" match. Seems to depend
     * on the order in which filters are added to the graph, thus indicating
     * no preference given to exact match. */
    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IFilterGraph2, (LPVOID*)&pgraph2);
    ok(hr == S_OK, "CoCreateInstance failed with %08x\n", hr);
    if (!pgraph2) return;

    hr = createtestfilter(&GUID_NULL, PinData1, &ptestfilter);
    ok(hr == S_OK, "createtestfilter failed with %08x\n", hr);
    if (FAILED(hr)) goto out;

    hr = IFilterGraph2_AddFilter(pgraph2, &ptestfilter->IBaseFilter_iface, wszFilterInstanceName1);
    ok(hr == S_OK, "IFilterGraph2_AddFilter failed with %08x\n", hr);

    hr = createtestfilter(&GUID_NULL, PinData2, &ptestfilter2);
    ok(hr == S_OK, "createtestfilter failed with %08x\n", hr);
    if (FAILED(hr)) goto out;

    hr = IFilterGraph2_AddFilter(pgraph2, &ptestfilter2->IBaseFilter_iface, wszFilterInstanceName2);
    ok(hr == S_OK, "IFilterGraph2_AddFilter failed with %08x\n", hr);

    IBaseFilter_Release(&ptestfilter2->IBaseFilter_iface);
    ptestfilter2 = NULL;

    hr = createtestfilter(&GUID_NULL, PinData3, &ptestfilter2);
    ok(hr == S_OK, "createtestfilter failed with %08x\n", hr);
    if (FAILED(hr)) goto out;

    hr = IFilterGraph2_AddFilter(pgraph2, &ptestfilter2->IBaseFilter_iface, wszFilterInstanceName3);
    ok(hr == S_OK, "IFilterGraph2_AddFilter failed with %08x\n", hr);

    hr = IFilterGraph2_Render(pgraph2, ptestfilter->ppPins[0]);
    ok(hr == S_OK, "IFilterGraph2_Render failed with %08x\n", hr);

    hr = get_connected_filter_name(ptestfilter, ConnectedFilterName1);

    IFilterGraph2_Release(pgraph2);
    pgraph2 = NULL;
    IBaseFilter_Release(&ptestfilter->IBaseFilter_iface);
    ptestfilter = NULL;
    IBaseFilter_Release(&ptestfilter2->IBaseFilter_iface);
    ptestfilter2 = NULL;

    if (hr == E_NOTIMPL)
    {
        win_skip("Needed functions are not implemented\n");
        return;
    }

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IFilterGraph2, (LPVOID*)&pgraph2);
    ok(hr == S_OK, "CoCreateInstance failed with %08x\n", hr);
    if (!pgraph2) goto out;

    hr = createtestfilter(&GUID_NULL, PinData1, &ptestfilter);
    ok(hr == S_OK, "createtestfilter failed with %08x\n", hr);
    if (FAILED(hr)) goto out;

    hr = IFilterGraph2_AddFilter(pgraph2, &ptestfilter->IBaseFilter_iface, wszFilterInstanceName1);
    ok(hr == S_OK, "IFilterGraph2_AddFilter failed with %08x\n", hr);

    hr = createtestfilter(&GUID_NULL, PinData3, &ptestfilter2);
    ok(hr == S_OK, "createtestfilter failed with %08x\n", hr);
    if (FAILED(hr)) goto out;

    hr = IFilterGraph2_AddFilter(pgraph2, &ptestfilter2->IBaseFilter_iface, wszFilterInstanceName3);
    ok(hr == S_OK, "IFilterGraph2_AddFilter failed with %08x\n", hr);

    IBaseFilter_Release(&ptestfilter2->IBaseFilter_iface);
    ptestfilter2 = NULL;

    hr = createtestfilter(&GUID_NULL, PinData2, &ptestfilter2);
    ok(hr == S_OK, "createtestfilter failed with %08x\n", hr);
    if (FAILED(hr)) goto out;

    hr = IFilterGraph2_AddFilter(pgraph2, &ptestfilter2->IBaseFilter_iface, wszFilterInstanceName2);
    ok(hr == S_OK, "IFilterGraph2_AddFilter failed with %08x\n", hr);

    hr = IFilterGraph2_Render(pgraph2, ptestfilter->ppPins[0]);
    ok(hr == S_OK, "IFilterGraph2_Render failed with %08x\n", hr);

    hr = IFilterGraph2_Disconnect(pgraph2, NULL);
    ok(hr == E_POINTER, "IFilterGraph2_Disconnect failed. Expected E_POINTER, received %08x\n", hr);

    get_connected_filter_name(ptestfilter, ConnectedFilterName2);
    ok(strcmp(ConnectedFilterName1, ConnectedFilterName2),
        "expected connected filters to be different but got %s both times\n", ConnectedFilterName1);

    IFilterGraph2_Release(pgraph2);
    pgraph2 = NULL;
    IBaseFilter_Release(&ptestfilter->IBaseFilter_iface);
    ptestfilter = NULL;
    IBaseFilter_Release(&ptestfilter2->IBaseFilter_iface);
    ptestfilter2 = NULL;

    /* Test if any preference is given to existing renderer which renders the pin directly vs
       an existing renderer which renders the pin indirectly, through an additional middle filter,
       again trying different orders of creation. Native appears not to give a preference. */

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IFilterGraph2, (LPVOID*)&pgraph2);
    ok(hr == S_OK, "CoCreateInstance failed with %08x\n", hr);
    if (!pgraph2) goto out;

    hr = createtestfilter(&GUID_NULL, PinData1, &ptestfilter);
    ok(hr == S_OK, "createtestfilter failed with %08x\n", hr);
    if (FAILED(hr)) goto out;

    hr = IFilterGraph2_AddFilter(pgraph2, &ptestfilter->IBaseFilter_iface, wszFilterInstanceName1);
    ok(hr == S_OK, "IFilterGraph2_AddFilter failed with %08x\n", hr);

    hr = createtestfilter(&GUID_NULL, PinData2, &ptestfilter2);
    ok(hr == S_OK, "createtestfilter failed with %08x\n", hr);
    if (FAILED(hr)) goto out;

    hr = IFilterGraph2_AddFilter(pgraph2, &ptestfilter2->IBaseFilter_iface, wszFilterInstanceName2);
    ok(hr == S_OK, "IFilterGraph2_AddFilter failed with %08x\n", hr);

    IBaseFilter_Release(&ptestfilter2->IBaseFilter_iface);
    ptestfilter2 = NULL;

    hr = createtestfilter(&GUID_NULL, PinData4, &ptestfilter2);
    ok(hr == S_OK, "createtestfilter failed with %08x\n", hr);
    if (FAILED(hr)) goto out;

    hr = IFilterGraph2_AddFilter(pgraph2, &ptestfilter2->IBaseFilter_iface, wszFilterInstanceName3);
    ok(hr == S_OK, "IFilterGraph2_AddFilter failed with %08x\n", hr);

    IBaseFilter_Release(&ptestfilter2->IBaseFilter_iface);
    ptestfilter2 = NULL;

    hr = createtestfilter(&GUID_NULL, PinData5, &ptestfilter2);
    ok(hr == S_OK, "createtestfilter failed with %08x\n", hr);
    if (FAILED(hr)) goto out;

    hr = IFilterGraph2_AddFilter(pgraph2, &ptestfilter2->IBaseFilter_iface, wszFilterInstanceName4);
    ok(hr == S_OK, "IFilterGraph2_AddFilter failed with %08x\n", hr);

    hr = IFilterGraph2_Render(pgraph2, ptestfilter->ppPins[0]);
    ok(hr == S_OK, "IFilterGraph2_Render failed with %08x\n", hr);

    get_connected_filter_name(ptestfilter, ConnectedFilterName1);
    ok(!strcmp(ConnectedFilterName1, "TestfilterInstance3") || !strcmp(ConnectedFilterName1, "TestfilterInstance2"),
            "unexpected connected filter: %s\n", ConnectedFilterName1);

    IFilterGraph2_Release(pgraph2);
    pgraph2 = NULL;
    IBaseFilter_Release(&ptestfilter->IBaseFilter_iface);
    ptestfilter = NULL;
    IBaseFilter_Release(&ptestfilter2->IBaseFilter_iface);
    ptestfilter2 = NULL;

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IFilterGraph2, (LPVOID*)&pgraph2);
    ok(hr == S_OK, "CoCreateInstance failed with %08x\n", hr);
    if (!pgraph2) goto out;

    hr = createtestfilter(&GUID_NULL, PinData1, &ptestfilter);
    ok(hr == S_OK, "createtestfilter failed with %08x\n", hr);
    if (FAILED(hr)) goto out;

    hr = IFilterGraph2_AddFilter(pgraph2, &ptestfilter->IBaseFilter_iface, wszFilterInstanceName1);
    ok(hr == S_OK, "IFilterGraph2_AddFilter failed with %08x\n", hr);

    hr = createtestfilter(&GUID_NULL, PinData4, &ptestfilter2);
    ok(hr == S_OK, "createtestfilter failed with %08x\n", hr);
    if (FAILED(hr)) goto out;

    hr = IFilterGraph2_AddFilter(pgraph2, &ptestfilter2->IBaseFilter_iface, wszFilterInstanceName3);
    ok(hr == S_OK, "IFilterGraph2_AddFilter failed with %08x\n", hr);

    IBaseFilter_Release(&ptestfilter2->IBaseFilter_iface);
    ptestfilter2 = NULL;

    hr = createtestfilter(&GUID_NULL, PinData5, &ptestfilter2);
    ok(hr == S_OK, "createtestfilter failed with %08x\n", hr);
    if (FAILED(hr)) goto out;

    hr = IFilterGraph2_AddFilter(pgraph2, &ptestfilter2->IBaseFilter_iface, wszFilterInstanceName4);
    ok(hr == S_OK, "IFilterGraph2_AddFilter failed with %08x\n", hr);

    IBaseFilter_Release(&ptestfilter2->IBaseFilter_iface);
    ptestfilter2 = NULL;

    hr = createtestfilter(&GUID_NULL, PinData2, &ptestfilter2);
    ok(hr == S_OK, "createtestfilter failed with %08x\n", hr);
    if (FAILED(hr)) goto out;

    hr = IFilterGraph2_AddFilter(pgraph2, &ptestfilter2->IBaseFilter_iface, wszFilterInstanceName2);
    ok(hr == S_OK, "IFilterGraph2_AddFilter failed with %08x\n", hr);

    hr = IFilterGraph2_Render(pgraph2, ptestfilter->ppPins[0]);
    ok(hr == S_OK, "IFilterGraph2_Render failed with %08x\n", hr);

    get_connected_filter_name(ptestfilter, ConnectedFilterName2);
    ok(!strcmp(ConnectedFilterName2, "TestfilterInstance3") || !strcmp(ConnectedFilterName2, "TestfilterInstance2"),
            "unexpected connected filter: %s\n", ConnectedFilterName2);
    ok(strcmp(ConnectedFilterName1, ConnectedFilterName2),
        "expected connected filters to be different but got %s both times\n", ConnectedFilterName1);

    IFilterGraph2_Release(pgraph2);
    pgraph2 = NULL;
    IBaseFilter_Release(&ptestfilter->IBaseFilter_iface);
    ptestfilter = NULL;
    IBaseFilter_Release(&ptestfilter2->IBaseFilter_iface);
    ptestfilter2 = NULL;

    /* Test if renderers are tried before non-renderers (intermediary filters). */
    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IFilterGraph2, (LPVOID*)&pgraph2);
    ok(hr == S_OK, "CoCreateInstance failed with %08x\n", hr);
    if (!pgraph2) goto out;

    hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER, &IID_IFilterMapper2, (LPVOID*)&pMapper2);
    ok(hr == S_OK, "CoCreateInstance failed with %08x\n", hr);
    if (!pMapper2) goto out;

    hr = createtestfilter(&GUID_NULL, PinData1, &ptestfilter);
    ok(hr == S_OK, "createtestfilter failed with %08x\n", hr);
    if (FAILED(hr)) goto out;

    hr = IFilterGraph2_AddFilter(pgraph2, &ptestfilter->IBaseFilter_iface, wszFilterInstanceName1);
    ok(hr == S_OK, "IFilterGraph2_AddFilter failed with %08x\n", hr);

    /* Register our filters with COM and with Filtermapper. */
    hr = CoRegisterClassObject(Filter1ClassFactory.clsid,
            (IUnknown *)&Filter1ClassFactory.IClassFactory_iface, CLSCTX_INPROC_SERVER,
            REGCLS_MULTIPLEUSE, &cookie1);
    ok(hr == S_OK, "CoRegisterClassObject failed with %08x\n", hr);
    if (FAILED(hr)) goto out;
    hr = CoRegisterClassObject(Filter2ClassFactory.clsid,
            (IUnknown *)&Filter2ClassFactory.IClassFactory_iface, CLSCTX_INPROC_SERVER,
            REGCLS_MULTIPLEUSE, &cookie2);
    ok(hr == S_OK, "CoRegisterClassObject failed with %08x\n", hr);
    if (FAILED(hr)) goto out;
    hr = CoRegisterClassObject(Filter3ClassFactory.clsid,
            (IUnknown *)&Filter3ClassFactory.IClassFactory_iface, CLSCTX_INPROC_SERVER,
            REGCLS_MULTIPLEUSE, &cookie3);
    ok(hr == S_OK, "CoRegisterClassObject failed with %08x\n", hr);
    if (FAILED(hr)) goto out;

    rgf2.dwVersion = 2;
    rgf2.dwMerit = MERIT_UNLIKELY;
    S2(U(rgf2)).cPins2 = 1;
    S2(U(rgf2)).rgPins2 = rgPins2;
    rgPins2[0].dwFlags = REG_PINFLAG_B_RENDERER;
    rgPins2[0].cInstances = 1;
    rgPins2[0].nMediaTypes = 1;
    rgPins2[0].lpMediaType = &rgPinType[0];
    rgPins2[0].nMediums = 0;
    rgPins2[0].lpMedium = NULL;
    rgPins2[0].clsPinCategory = NULL;
    rgPinType[0].clsMajorType = &MEDIATYPE_Video;
    rgPinType[0].clsMinorType = &mediasubtype1;

    hr = IFilterMapper2_RegisterFilter(pMapper2, &CLSID_TestFilter2, wszFilterInstanceName2, NULL,
                    &CLSID_LegacyAmFilterCategory, NULL, &rgf2);
    if (hr == E_ACCESSDENIED)
        skip("Not authorized to register filters\n");
    else
    {
        ok(hr == S_OK, "IFilterMapper2_RegisterFilter failed with %x\n", hr);

        rgf2.dwMerit = MERIT_PREFERRED;
        rgPinType[0].clsMinorType = &mediasubtype2;

        hr = IFilterMapper2_RegisterFilter(pMapper2, &CLSID_TestFilter4, wszFilterInstanceName4, NULL,
                    &CLSID_LegacyAmFilterCategory, NULL, &rgf2);
        ok(hr == S_OK, "IFilterMapper2_RegisterFilter failed with %x\n", hr);

        S2(U(rgf2)).cPins2 = 2;
        rgPins2[0].dwFlags = 0;
        rgPinType[0].clsMinorType = &mediasubtype1;

        rgPins2[1].dwFlags = REG_PINFLAG_B_OUTPUT;
        rgPins2[1].cInstances = 1;
        rgPins2[1].nMediaTypes = 1;
        rgPins2[1].lpMediaType = &rgPinType[1];
        rgPins2[1].nMediums = 0;
        rgPins2[1].lpMedium = NULL;
        rgPins2[1].clsPinCategory = NULL;
        rgPinType[1].clsMajorType = &MEDIATYPE_Video;
        rgPinType[1].clsMinorType = &mediasubtype2;

        hr = IFilterMapper2_RegisterFilter(pMapper2, &CLSID_TestFilter3, wszFilterInstanceName3, NULL,
                    &CLSID_LegacyAmFilterCategory, NULL, &rgf2);
        ok(hr == S_OK, "IFilterMapper2_RegisterFilter failed with %x\n", hr);

        hr = IFilterGraph2_Render(pgraph2, ptestfilter->ppPins[0]);
        ok(hr == S_OK, "IFilterGraph2_Render failed with %08x\n", hr);

        get_connected_filter_name(ptestfilter, ConnectedFilterName1);
        ok(!strcmp(ConnectedFilterName1, "TestfilterInstance3"),
           "unexpected connected filter: %s\n", ConnectedFilterName1);

        hr = IFilterMapper2_UnregisterFilter(pMapper2, &CLSID_LegacyAmFilterCategory, NULL,
                &CLSID_TestFilter2);
        ok(hr == S_OK, "IFilterMapper2_UnregisterFilter failed with %x\n", hr);
        hr = IFilterMapper2_UnregisterFilter(pMapper2, &CLSID_LegacyAmFilterCategory, NULL,
                &CLSID_TestFilter3);
        ok(hr == S_OK, "IFilterMapper2_UnregisterFilter failed with %x\n", hr);
        hr = IFilterMapper2_UnregisterFilter(pMapper2, &CLSID_LegacyAmFilterCategory, NULL,
                 &CLSID_TestFilter4);
        ok(hr == S_OK, "IFilterMapper2_UnregisterFilter failed with %x\n", hr);
    }

    out:

    if (ptestfilter) IBaseFilter_Release(&ptestfilter->IBaseFilter_iface);
    if (ptestfilter2) IBaseFilter_Release(&ptestfilter2->IBaseFilter_iface);
    if (pgraph2) IFilterGraph2_Release(pgraph2);
    if (pMapper2) IFilterMapper2_Release(pMapper2);

    hr = CoRevokeClassObject(cookie1);
    ok(hr == S_OK, "CoRevokeClassObject failed with %08x\n", hr);
    hr = CoRevokeClassObject(cookie2);
    ok(hr == S_OK, "CoRevokeClassObject failed with %08x\n", hr);
    hr = CoRevokeClassObject(cookie3);
    ok(hr == S_OK, "CoRevokeClassObject failed with %08x\n", hr);
}

START_TEST(filtergraph)
{
    HRESULT hr;
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IGraphBuilder, (LPVOID*)&pgraph);
    if (FAILED(hr)) {
        skip("Creating filtergraph returned %08x, skipping tests\n", hr);
        return;
    }
    IGraphBuilder_Release(pgraph);
    test_render_run(avifile);
    test_render_run(mpegfile);
    test_graph_builder();
    test_graph_builder_addfilter();
    test_mediacontrol();
    test_filter_graph2();
    test_render_filter_priority();
    CoUninitialize();
}
