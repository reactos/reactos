/*
 * Misc unit tests for Quartz
 *
 * Copyright (C) 2007 Google (Lei Zhang)
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

#include "wine/test.h"
#include "dshow.h"

#define QI_SUCCEED(iface, riid, ppv) hr = IUnknown_QueryInterface(iface, &riid, (LPVOID*)&ppv); \
    ok(hr == S_OK, "IUnknown_QueryInterface returned %x\n", hr); \
    ok(ppv != NULL, "Pointer is NULL\n");

#define QI_FAIL(iface, riid, ppv) hr = IUnknown_QueryInterface(iface, &riid, (LPVOID*)&ppv); \
    ok(hr == E_NOINTERFACE, "IUnknown_QueryInterface returned %x\n", hr); \
    ok(ppv == NULL, "Pointer is %p\n", ppv);

#define ADDREF_EXPECT(iface, num) if (iface) { \
    refCount = IUnknown_AddRef(iface); \
    ok(refCount == num, "IUnknown_AddRef should return %d, got %d\n", num, refCount); \
}

#define RELEASE_EXPECT(iface, num) if (iface) { \
    refCount = IUnknown_Release(iface); \
    ok(refCount == num, "IUnknown_Release should return %d, got %d\n", num, refCount); \
}

static void test_aggregation(const CLSID clsidOuter, const CLSID clsidInner,
                             const IID iidOuter, const IID iidInner)
{
    HRESULT hr;
    ULONG refCount;
    IUnknown *pUnkOuter = NULL;
    IUnknown *pUnkInner = NULL;
    IUnknown *pUnkInnerFail = NULL;
    IUnknown *pUnkOuterTest = NULL;
    IUnknown *pUnkInnerTest = NULL;
    IUnknown *pUnkAggregatee = NULL;
    IUnknown *pUnkAggregator = NULL;
    IUnknown *pUnkTest = NULL;

    hr = CoCreateInstance(&clsidOuter, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IUnknown, (LPVOID*)&pUnkOuter);
    ok(hr == S_OK, "CoCreateInstance failed with %x\n", hr);
    ok(pUnkOuter != NULL, "pUnkOuter is NULL\n");

    if (!pUnkOuter)
    {
        skip("pUnkOuter is NULL\n");
        return;
    }

    /* for aggregation, we should only be able to request IUnknown */
    hr = CoCreateInstance(&clsidInner, pUnkOuter, CLSCTX_INPROC_SERVER,
                          &iidInner, (LPVOID*)&pUnkInnerFail);
    ok(hr == E_NOINTERFACE, "CoCreateInstance returned %x\n", hr);
    ok(pUnkInnerFail == NULL, "pUnkInnerFail is not NULL\n");

    /* aggregation, request IUnknown */
    hr = CoCreateInstance(&clsidInner, pUnkOuter, CLSCTX_INPROC_SERVER,
                          &IID_IUnknown, (LPVOID*)&pUnkInner);
    ok(hr == S_OK, "CoCreateInstance returned %x\n", hr);
    ok(pUnkInner != NULL, "pUnkInner is NULL\n");

    if (!pUnkInner)
    {
        skip("pUnkInner is NULL\n");
        return;
    }

    ADDREF_EXPECT(pUnkOuter, 2);
    ADDREF_EXPECT(pUnkInner, 2);
    RELEASE_EXPECT(pUnkOuter, 1);
    RELEASE_EXPECT(pUnkInner, 1);

    QI_FAIL(pUnkOuter, iidInner, pUnkAggregatee);
    QI_FAIL(pUnkInner, iidOuter, pUnkAggregator);

    /* these QueryInterface calls should work */
    QI_SUCCEED(pUnkOuter, iidOuter, pUnkAggregator);
    QI_SUCCEED(pUnkOuter, IID_IUnknown, pUnkOuterTest);
    /* IGraphConfig interface comes with DirectShow 9 */
    if(IsEqualGUID(&IID_IGraphConfig, &iidInner))
    {
        hr = IUnknown_QueryInterface(pUnkInner, &iidInner, (LPVOID*)&pUnkAggregatee);
        ok(hr == S_OK || broken(hr == E_NOINTERFACE), "IUnknown_QueryInterface returned %x\n", hr);
        ok(pUnkAggregatee != NULL || broken(!pUnkAggregatee), "Pointer is NULL\n");
    }
    else
    {
        QI_SUCCEED(pUnkInner, iidInner, pUnkAggregatee);
    }
    QI_SUCCEED(pUnkInner, IID_IUnknown, pUnkInnerTest);

    if (!pUnkAggregator || !pUnkOuterTest || !pUnkAggregatee
                    || !pUnkInnerTest)
    {
        skip("One of the required interfaces is NULL\n");
        return;
    }

    ADDREF_EXPECT(pUnkAggregator, 5);
    ADDREF_EXPECT(pUnkOuterTest, 6);
    ADDREF_EXPECT(pUnkAggregatee, 7);
    ADDREF_EXPECT(pUnkInnerTest, 3);
    RELEASE_EXPECT(pUnkAggregator, 6);
    RELEASE_EXPECT(pUnkOuterTest, 5);
    RELEASE_EXPECT(pUnkAggregatee, 4);
    RELEASE_EXPECT(pUnkInnerTest, 2);

    QI_SUCCEED(pUnkAggregator, IID_IUnknown, pUnkTest);
    QI_SUCCEED(pUnkOuterTest, IID_IUnknown, pUnkTest);
    QI_SUCCEED(pUnkAggregatee, IID_IUnknown, pUnkTest);
    QI_SUCCEED(pUnkInnerTest, IID_IUnknown, pUnkTest);

    QI_FAIL(pUnkAggregator, iidInner, pUnkTest);
    QI_FAIL(pUnkOuterTest, iidInner, pUnkTest);
    QI_FAIL(pUnkAggregatee, iidInner, pUnkTest);
    QI_SUCCEED(pUnkInnerTest, iidInner, pUnkTest);

    QI_SUCCEED(pUnkAggregator, iidOuter, pUnkTest);
    QI_SUCCEED(pUnkOuterTest, iidOuter, pUnkTest);
    QI_SUCCEED(pUnkAggregatee, iidOuter, pUnkTest);
    QI_FAIL(pUnkInnerTest, iidOuter, pUnkTest);

    RELEASE_EXPECT(pUnkAggregator, 10);
    RELEASE_EXPECT(pUnkOuterTest, 9);
    RELEASE_EXPECT(pUnkAggregatee, 8);
    RELEASE_EXPECT(pUnkInnerTest, 2);
    RELEASE_EXPECT(pUnkOuter, 7);
    RELEASE_EXPECT(pUnkInner, 1);

    do
    {
        refCount = IUnknown_Release(pUnkInner);
    } while (refCount);

    do
    {
        refCount = IUnknown_Release(pUnkOuter);
    } while (refCount);
}

static void test_video_renderer_aggregations(void)
{
    const IID * iids[] = {
        &IID_IMediaFilter, &IID_IBaseFilter, &IID_IBasicVideo, &IID_IVideoWindow
    };
    int i;

    for (i = 0; i < sizeof(iids) / sizeof(iids[0]); i++)
    {
        test_aggregation(CLSID_SystemClock, CLSID_VideoRenderer,
                         IID_IReferenceClock, *iids[i]);
    }
}

static void test_filter_graph_aggregations(void)
{
    const IID * iids[] = {
        &IID_IFilterGraph2, &IID_IMediaControl, &IID_IGraphBuilder,
        &IID_IFilterGraph, &IID_IMediaSeeking, &IID_IBasicAudio, &IID_IBasicVideo,
        &IID_IVideoWindow, &IID_IMediaEventEx, &IID_IMediaFilter,
        &IID_IMediaEventSink, &IID_IGraphConfig, &IID_IMediaPosition
    };
    int i;

    for (i = 0; i < sizeof(iids) / sizeof(iids[0]); i++)
    {
        test_aggregation(CLSID_SystemClock, CLSID_FilterGraph,
                         IID_IReferenceClock, *iids[i]);
    }
}

static void test_filter_mapper_aggregations(void)
{
    const IID * iids[] = {
        &IID_IFilterMapper2, &IID_IFilterMapper
    };
    int i;

    for (i = 0; i < sizeof(iids) / sizeof(iids[0]); i++)
    {
        test_aggregation(CLSID_SystemClock, CLSID_FilterMapper2,
                         IID_IReferenceClock, *iids[i]);
    }
}

START_TEST(misc)
{
    CoInitialize(NULL);

    test_video_renderer_aggregations();
    test_filter_graph_aggregations();
    test_filter_mapper_aggregations();

    CoUninitialize();
}
