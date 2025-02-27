/*
 * Capture graph builder unit tests
 *
 * Copyright 2019 Zebediah Figura
 * Copyright 2020 Zebediah Figura for CodeWeavers
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
#include "dshow.h"
#include "wine/strmbase.h"
#include "wine/test.h"

static ICaptureGraphBuilder2 *create_capture_graph(void)
{
    ICaptureGraphBuilder2 *ret;
    HRESULT hr = CoCreateInstance(&CLSID_CaptureGraphBuilder2, NULL,
            CLSCTX_INPROC_SERVER, &IID_ICaptureGraphBuilder2, (void **)&ret);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    return ret;
}

static const GUID testiid = {0x11111111}, testtype = {0x22222222};

struct testsource
{
    struct strmbase_source pin;
    IKsPropertySet IKsPropertySet_iface;
    GUID category;
    BOOL has_iface;
};

struct testsink
{
    struct strmbase_sink pin;
    BOOL has_iface;
};

struct testfilter
{
    struct strmbase_filter filter;
    struct testsource source1, source2;
    struct testsink sink1, sink2;
    BOOL filter_has_iface;
    GUID source_type;
    const GUID *sink_type;
};

static inline struct testfilter *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, filter);
}

static HRESULT testfilter_query_interface(struct strmbase_filter *iface, REFIID iid, void **out)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface);

    ok(!IsEqualGUID(iid, &IID_IKsPropertySet), "Unexpected query for IKsPropertySet.\n");

    if (filter->filter_has_iface && IsEqualGUID(iid, &testiid))
        *out = &iface->IBaseFilter_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static struct strmbase_pin *testfilter_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface);

    if (!index)
        return &filter->source1.pin.pin;
    else if (index == 1)
        return &filter->sink1.pin.pin;
    else if (index == 2)
        return &filter->source2.pin.pin;
    else if (index == 3)
        return &filter->sink2.pin.pin;
    return NULL;
}

static void testfilter_destroy(struct strmbase_filter *iface)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface);
    strmbase_source_cleanup(&filter->source1.pin);
    strmbase_source_cleanup(&filter->source2.pin);
    strmbase_sink_cleanup(&filter->sink1.pin);
    strmbase_sink_cleanup(&filter->sink2.pin);
    strmbase_filter_cleanup(&filter->filter);
}

static const struct strmbase_filter_ops testfilter_ops =
{
    .filter_query_interface = testfilter_query_interface,
    .filter_get_pin = testfilter_get_pin,
    .filter_destroy = testfilter_destroy,
};

static struct testsource *impl_from_IKsPropertySet(IKsPropertySet *iface)
{
    return CONTAINING_RECORD(iface, struct testsource, IKsPropertySet_iface);
}

static HRESULT WINAPI property_set_QueryInterface(IKsPropertySet *iface, REFIID iid, void **out)
{
    struct testsource *pin = impl_from_IKsPropertySet(iface);
    return IPin_QueryInterface(&pin->pin.pin.IPin_iface, iid, out);
}

static ULONG WINAPI property_set_AddRef(IKsPropertySet *iface)
{
    struct testsource *pin = impl_from_IKsPropertySet(iface);
    return IPin_AddRef(&pin->pin.pin.IPin_iface);
}

static ULONG WINAPI property_set_Release(IKsPropertySet *iface)
{
    struct testsource *pin = impl_from_IKsPropertySet(iface);
    return IPin_Release(&pin->pin.pin.IPin_iface);
}

static HRESULT WINAPI property_set_Set(IKsPropertySet *iface, REFGUID set, DWORD id,
        void *instance_data, DWORD instance_size, void *property_data, DWORD property_size)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI property_set_Get(IKsPropertySet *iface, REFGUID set, DWORD id,
        void *instance_data, DWORD instance_size, void *property_data, DWORD property_size, DWORD *ret_size)
{
    struct testsource *pin = impl_from_IKsPropertySet(iface);

    if (winetest_debug > 1) trace("%s->Get()\n", debugstr_w(pin->pin.pin.name));

    ok(IsEqualGUID(set, &AMPROPSETID_Pin), "Got set %s.\n", debugstr_guid(set));
    ok(id == AMPROPERTY_PIN_CATEGORY, "Got id %#lx.\n", id);
    ok(!instance_data, "Got instance data %p.\n", instance_data);
    ok(!instance_size, "Got instance size %lu.\n", instance_size);
    ok(property_size == sizeof(GUID), "Got property size %lu.\n", property_size);
    ok(!!ret_size, "Expected non-NULL return size.\n");
    memcpy(property_data, &pin->category, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI property_set_QuerySupported(IKsPropertySet *iface, REFGUID set, DWORD id, DWORD *flags)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IKsPropertySetVtbl property_set_vtbl =
{
    property_set_QueryInterface,
    property_set_AddRef,
    property_set_Release,
    property_set_Set,
    property_set_Get,
    property_set_QuerySupported,
};

static struct testsource *impl_source_from_strmbase_pin(struct strmbase_pin *iface)
{
    return CONTAINING_RECORD(iface, struct testsource, pin.pin);
}

static HRESULT testsource_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct testsource *pin = impl_source_from_strmbase_pin(iface);

    if (pin->has_iface && IsEqualGUID(iid, &testiid))
        *out = &pin->pin.pin.IPin_iface;
    else if (pin->IKsPropertySet_iface.lpVtbl && IsEqualGUID(iid, &IID_IKsPropertySet))
        *out = &pin->IKsPropertySet_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT testsource_get_media_type(struct strmbase_pin *iface, unsigned int index, AM_MEDIA_TYPE *mt)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface->filter);

    if (!index)
    {
        memset(mt, 0, sizeof(*mt));
        mt->majortype = filter->source_type;
        return S_OK;
    }
    return VFW_S_NO_MORE_ITEMS;
}

static HRESULT WINAPI testsource_DecideBufferSize(struct strmbase_source *iface,
        IMemAllocator *allocator, ALLOCATOR_PROPERTIES *props)
{
    props->cBuffers = props->cbAlign = props->cbBuffer = 1;
    return S_OK;
}

static const struct strmbase_source_ops testsource_ops =
{
    .base.pin_query_interface = testsource_query_interface,
    .base.pin_get_media_type = testsource_get_media_type,
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideAllocator = BaseOutputPinImpl_DecideAllocator,
    .pfnDecideBufferSize = testsource_DecideBufferSize,
};

static struct testsink *impl_sink_from_strmbase_pin(struct strmbase_pin *iface)
{
    return CONTAINING_RECORD(iface, struct testsink, pin.pin);
}

static HRESULT testsink_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct testsink *pin = impl_sink_from_strmbase_pin(iface);

    ok(!IsEqualGUID(iid, &IID_IKsPropertySet), "Unexpected query for IKsPropertySet.\n");

    if (IsEqualGUID(iid, &IID_IMemInputPin))
        *out = &pin->pin.IMemInputPin_iface;
    else if (pin->has_iface && IsEqualGUID(iid, &testiid))
        *out = &pin->pin.pin.IPin_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT testsink_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface->filter);

    if (filter->sink_type && !IsEqualGUID(&mt->majortype, filter->sink_type))
        return S_FALSE;
    return S_OK;
}

static const struct strmbase_sink_ops testsink_ops =
{
    .base.pin_query_interface = testsink_query_interface,
    .base.pin_query_accept = testsink_query_accept,
};

static void reset_interfaces(struct testfilter *filter)
{
    filter->filter_has_iface = filter->sink1.has_iface = filter->sink2.has_iface = TRUE;
    filter->source1.has_iface = filter->source2.has_iface = TRUE;
}

static void testfilter_init(struct testfilter *filter)
{
    static const GUID clsid = {0xabacab};
    memset(filter, 0, sizeof(*filter));
    strmbase_filter_init(&filter->filter, NULL, &clsid, &testfilter_ops);
    strmbase_source_init(&filter->source1.pin, &filter->filter, L"source1", &testsource_ops);
    strmbase_source_init(&filter->source2.pin, &filter->filter, L"source2", &testsource_ops);
    strmbase_sink_init(&filter->sink1.pin, &filter->filter, L"sink1", &testsink_ops, NULL);
    strmbase_sink_init(&filter->sink2.pin, &filter->filter, L"sink2", &testsink_ops, NULL);
    reset_interfaces(filter);
}

static void test_find_interface(void)
{
    static const AM_MEDIA_TYPE mt1 =
    {
        .majortype = {0x111},
        .subtype = {0x222},
        .formattype = {0x333},
    };
    static const AM_MEDIA_TYPE mt2 =
    {
        .majortype = {0x444},
        .subtype = {0x555},
        .formattype = {0x666},
    };
    static const GUID bogus_majortype = {0x777};

    ICaptureGraphBuilder2 *capture_graph = create_capture_graph();
    struct testfilter filter1, filter2, filter3;
    IGraphBuilder *graph;
    unsigned int i;
    IUnknown *unk;
    HRESULT hr;
    ULONG ref;

    struct
    {
        BOOL *expose;
        const void *iface;
    }
    tests_from_filter2[] =
    {
        {&filter2.filter_has_iface,     &filter2.filter.IBaseFilter_iface},
        {&filter2.source1.has_iface,    &filter2.source1.pin.pin.IPin_iface},
        {&filter3.sink1.has_iface,      &filter3.sink1.pin.pin.IPin_iface},
        {&filter3.filter_has_iface,     &filter3.filter.IBaseFilter_iface},
        {&filter3.source1.has_iface,    &filter3.source1.pin.pin.IPin_iface},
        {&filter3.source2.has_iface,    &filter3.source2.pin.pin.IPin_iface},
        {&filter2.source2.has_iface,    &filter2.source2.pin.pin.IPin_iface},
        {&filter2.sink1.has_iface,      &filter2.sink1.pin.pin.IPin_iface},
        {&filter1.source1.has_iface,    &filter1.source1.pin.pin.IPin_iface},
        {&filter1.filter_has_iface,     &filter1.filter.IBaseFilter_iface},
        {&filter1.sink1.has_iface,      &filter1.sink1.pin.pin.IPin_iface},
        {&filter1.sink2.has_iface,      &filter1.sink2.pin.pin.IPin_iface},
        {&filter2.sink2.has_iface,      &filter2.sink2.pin.pin.IPin_iface},
    }, tests_from_filter1[] =
    {
        {&filter1.filter_has_iface,     &filter1.filter.IBaseFilter_iface},
        {&filter1.source1.has_iface,    &filter1.source1.pin.pin.IPin_iface},
        {&filter2.sink1.has_iface,      &filter2.sink1.pin.pin.IPin_iface},
        {&filter2.filter_has_iface,     &filter2.filter.IBaseFilter_iface},
        {&filter2.source1.has_iface,    &filter2.source1.pin.pin.IPin_iface},
        {&filter3.sink1.has_iface,      &filter3.sink1.pin.pin.IPin_iface},
        {&filter3.filter_has_iface,     &filter3.filter.IBaseFilter_iface},
        {&filter3.source1.has_iface,    &filter3.source1.pin.pin.IPin_iface},
        {&filter3.source2.has_iface,    &filter3.source2.pin.pin.IPin_iface},
        {&filter2.source2.has_iface,    &filter2.source2.pin.pin.IPin_iface},
        {&filter1.source2.has_iface,    &filter1.source2.pin.pin.IPin_iface},
        {&filter1.sink1.has_iface,      &filter1.sink1.pin.pin.IPin_iface},
        {&filter1.sink2.has_iface,      &filter1.sink2.pin.pin.IPin_iface},
    }, look_upstream_tests[] =
    {
        {&filter2.sink1.has_iface,      &filter2.sink1.pin.pin.IPin_iface},
        {&filter1.source1.has_iface,    &filter1.source1.pin.pin.IPin_iface},
        {&filter1.filter_has_iface,     &filter1.filter.IBaseFilter_iface},
        {&filter1.sink1.has_iface,      &filter1.sink1.pin.pin.IPin_iface},
        {&filter1.sink2.has_iface,      &filter1.sink2.pin.pin.IPin_iface},
        {&filter2.sink2.has_iface,      &filter2.sink2.pin.pin.IPin_iface},
    }, look_downstream_tests[] =
    {
        {&filter2.source1.has_iface,    &filter2.source1.pin.pin.IPin_iface},
        {&filter3.sink1.has_iface,      &filter3.sink1.pin.pin.IPin_iface},
        {&filter3.filter_has_iface,     &filter3.filter.IBaseFilter_iface},
        {&filter3.source1.has_iface,    &filter3.source1.pin.pin.IPin_iface},
        {&filter3.source2.has_iface,    &filter3.source2.pin.pin.IPin_iface},
        {&filter2.source2.has_iface,    &filter2.source2.pin.pin.IPin_iface},
    }, category_tests[] =
    {
        {&filter3.filter_has_iface,     &filter3.filter.IBaseFilter_iface},
        {&filter3.source1.has_iface,    &filter3.source1.pin.pin.IPin_iface},
        {&filter3.source2.has_iface,    &filter3.source2.pin.pin.IPin_iface},
        {&filter2.sink1.has_iface,      &filter2.sink1.pin.pin.IPin_iface},
        {&filter1.source1.has_iface,    &filter1.source1.pin.pin.IPin_iface},
        {&filter1.filter_has_iface,     &filter1.filter.IBaseFilter_iface},
        {&filter1.sink1.has_iface,      &filter1.sink1.pin.pin.IPin_iface},
        {&filter1.sink2.has_iface,      &filter1.sink2.pin.pin.IPin_iface},
        {&filter2.sink2.has_iface,      &filter2.sink2.pin.pin.IPin_iface},
    };

    CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IGraphBuilder, (void **)&graph);
    hr = ICaptureGraphBuilder2_SetFiltergraph(capture_graph, graph);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    testfilter_init(&filter1);
    IGraphBuilder_AddFilter(graph, &filter1.filter.IBaseFilter_iface, L"filter1");
    testfilter_init(&filter2);
    IGraphBuilder_AddFilter(graph, &filter2.filter.IBaseFilter_iface, L"filter2");
    testfilter_init(&filter3);
    IGraphBuilder_AddFilter(graph, &filter3.filter.IBaseFilter_iface, L"filter3");

    hr = IGraphBuilder_ConnectDirect(graph, &filter1.source1.pin.pin.IPin_iface, &filter2.sink1.pin.pin.IPin_iface, &mt1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IGraphBuilder_ConnectDirect(graph, &filter2.source1.pin.pin.IPin_iface, &filter3.sink1.pin.pin.IPin_iface, &mt2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Test search order without any restrictions applied. */

    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, NULL, &bogus_majortype,
                NULL, &testiid, (void **)&unk);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(tests_from_filter2); ++i)
    {
        hr = ICaptureGraphBuilder2_FindInterface(capture_graph, NULL, &bogus_majortype,
                &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
        ok(hr == S_OK, "Test %u: got hr %#lx.\n", i, hr);
        ok(unk == tests_from_filter2[i].iface, "Test %u: got wrong interface %p.\n", i, unk);
        IUnknown_Release(unk);
        *tests_from_filter2[i].expose = FALSE;
    }

    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, NULL, &bogus_majortype,
            &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    reset_interfaces(&filter1);
    reset_interfaces(&filter2);
    reset_interfaces(&filter3);

    for (i = 0; i < ARRAY_SIZE(tests_from_filter1); ++i)
    {
        hr = ICaptureGraphBuilder2_FindInterface(capture_graph, NULL, &bogus_majortype,
                &filter1.filter.IBaseFilter_iface, &testiid, (void **)&unk);
        ok(hr == S_OK, "Test %u: got hr %#lx.\n", i, hr);
        ok(unk == tests_from_filter1[i].iface, "Test %u: got wrong interface %p.\n", i, unk);
        IUnknown_Release(unk);
        *tests_from_filter1[i].expose = FALSE;
    }

    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, NULL, &bogus_majortype,
            &filter1.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    /* Test with upstream/downstream flags. */

    reset_interfaces(&filter1);
    reset_interfaces(&filter2);
    reset_interfaces(&filter3);

    for (i = 0; i < ARRAY_SIZE(look_upstream_tests); ++i)
    {
        hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &LOOK_UPSTREAM_ONLY, &bogus_majortype,
                &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
        ok(hr == S_OK, "Test %u: got hr %#lx.\n", i, hr);
        ok(unk == look_upstream_tests[i].iface, "Test %u: got wrong interface %p.\n", i, unk);
        IUnknown_Release(unk);
        *look_upstream_tests[i].expose = FALSE;
    }

    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &LOOK_UPSTREAM_ONLY, &bogus_majortype,
            &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    reset_interfaces(&filter1);
    reset_interfaces(&filter2);
    reset_interfaces(&filter3);

    for (i = 0; i < ARRAY_SIZE(look_downstream_tests); ++i)
    {
        hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &LOOK_DOWNSTREAM_ONLY, &bogus_majortype,
                &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
        ok(hr == S_OK, "Test %u: got hr %#lx.\n", i, hr);
        ok(unk == look_downstream_tests[i].iface, "Test %u: got wrong interface %p.\n", i, unk);
        IUnknown_Release(unk);
        *look_downstream_tests[i].expose = FALSE;
    }

    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &LOOK_DOWNSTREAM_ONLY, &bogus_majortype,
            &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    /* Test with a category flag. */

    reset_interfaces(&filter1);
    reset_interfaces(&filter2);
    reset_interfaces(&filter3);

    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &PIN_CATEGORY_CAPTURE, NULL,
            &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk == (IUnknown *)&filter2.filter.IBaseFilter_iface, "Got wrong interface %p.\n", unk);
    IUnknown_Release(unk);
    filter2.filter_has_iface = FALSE;

    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &PIN_CATEGORY_CAPTURE, NULL,
            &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);

    filter2.source1.IKsPropertySet_iface.lpVtbl = &property_set_vtbl;
    filter2.source1.category = PIN_CATEGORY_CAPTURE;
    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &PIN_CATEGORY_CAPTURE, NULL,
            &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk == (IUnknown *)&filter2.source1.pin.pin.IPin_iface, "Got wrong interface %p.\n", unk);
    IUnknown_Release(unk);
    filter2.source1.has_iface = FALSE;

    /* Native returns the filter3 sink next, but suffers from a bug wherein it
     * releases a reference to the wrong pin. */
    filter3.sink1.has_iface = FALSE;

    for (i = 0; i < ARRAY_SIZE(category_tests); ++i)
    {
        hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &PIN_CATEGORY_CAPTURE, NULL,
                &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
        ok(hr == S_OK, "Test %u: got hr %#lx.\n", i, hr);
        ok(unk == category_tests[i].iface, "Test %u: got wrong interface %p.\n", i, unk);
        IUnknown_Release(unk);
        *category_tests[i].expose = FALSE;
    }

    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &PIN_CATEGORY_CAPTURE, NULL,
            &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    /* Test with a media type. */

    filter1.source_type = filter2.source_type = testtype;

    reset_interfaces(&filter1);
    reset_interfaces(&filter2);
    reset_interfaces(&filter3);

    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &PIN_CATEGORY_CAPTURE, &bogus_majortype,
            &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk == (IUnknown *)&filter2.filter.IBaseFilter_iface, "Got wrong interface %p.\n", unk);
    IUnknown_Release(unk);
    filter2.filter_has_iface = FALSE;

    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &PIN_CATEGORY_CAPTURE, &bogus_majortype,
            &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);

    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &PIN_CATEGORY_CAPTURE, &mt2.majortype,
            &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);

    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &PIN_CATEGORY_CAPTURE, &testtype,
            &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk == (IUnknown *)&filter2.source1.pin.pin.IPin_iface, "Got wrong interface %p.\n", unk);
    IUnknown_Release(unk);
    filter2.source1.has_iface = FALSE;

    filter3.sink1.has_iface = FALSE;

    for (i = 0; i < ARRAY_SIZE(category_tests); ++i)
    {
        hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &PIN_CATEGORY_CAPTURE, NULL,
                &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
        ok(hr == S_OK, "Test %u: got hr %#lx.\n", i, hr);
        ok(unk == category_tests[i].iface, "Test %u: got wrong interface %p.\n", i, unk);
        IUnknown_Release(unk);
        *category_tests[i].expose = FALSE;
    }

    hr = ICaptureGraphBuilder2_FindInterface(capture_graph, &PIN_CATEGORY_CAPTURE, NULL,
            &filter2.filter.IBaseFilter_iface, &testiid, (void **)&unk);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    ref = ICaptureGraphBuilder2_Release(capture_graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IGraphBuilder_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(&filter1.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(&filter2.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(&filter3.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void test_find_pin(void)
{
    static const AM_MEDIA_TYPE mt =
    {
        .majortype = {0x111},
        .subtype = {0x222},
        .formattype = {0x333},
    };

    ICaptureGraphBuilder2 *capture_graph = create_capture_graph();
    struct testfilter filter1, filter2;
    IGraphBuilder *graph;
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IGraphBuilder, (void **)&graph);
    hr = ICaptureGraphBuilder2_SetFiltergraph(capture_graph, graph);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    testfilter_init(&filter1);
    testfilter_init(&filter2);
    IGraphBuilder_AddFilter(graph, &filter1.filter.IBaseFilter_iface, L"filter1");
    IGraphBuilder_AddFilter(graph, &filter2.filter.IBaseFilter_iface, L"filter2");

    hr = IGraphBuilder_ConnectDirect(graph, &filter1.source1.pin.pin.IPin_iface,
            &filter2.sink1.pin.pin.IPin_iface, &mt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = ICaptureGraphBuilder2_FindPin(capture_graph, (IUnknown *)&filter1.filter.IBaseFilter_iface,
            PINDIR_INPUT, NULL, NULL, FALSE, 0, &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pin == &filter1.sink1.pin.pin.IPin_iface, "Got wrong pin.\n");
    IPin_Release(pin);

    hr = ICaptureGraphBuilder2_FindPin(capture_graph, (IUnknown *)&filter1.filter.IBaseFilter_iface,
            PINDIR_INPUT, NULL, NULL, FALSE, 1, &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pin == &filter1.sink2.pin.pin.IPin_iface, "Got wrong pin.\n");
    IPin_Release(pin);

    hr = ICaptureGraphBuilder2_FindPin(capture_graph, (IUnknown *)&filter1.filter.IBaseFilter_iface,
            PINDIR_INPUT, NULL, NULL, FALSE, 2, &pin);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    hr = ICaptureGraphBuilder2_FindPin(capture_graph, (IUnknown *)&filter1.filter.IBaseFilter_iface,
            PINDIR_OUTPUT, NULL, NULL, FALSE, 0, &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pin == &filter1.source1.pin.pin.IPin_iface, "Got wrong pin.\n");
    IPin_Release(pin);

    hr = ICaptureGraphBuilder2_FindPin(capture_graph, (IUnknown *)&filter1.filter.IBaseFilter_iface,
            PINDIR_OUTPUT, NULL, NULL, FALSE, 1, &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pin == &filter1.source2.pin.pin.IPin_iface, "Got wrong pin.\n");
    IPin_Release(pin);

    hr = ICaptureGraphBuilder2_FindPin(capture_graph, (IUnknown *)&filter1.filter.IBaseFilter_iface,
            PINDIR_OUTPUT, NULL, NULL, FALSE, 2, &pin);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    /* Test the unconnected flag. */

    hr = ICaptureGraphBuilder2_FindPin(capture_graph, (IUnknown *)&filter1.filter.IBaseFilter_iface,
            PINDIR_OUTPUT, NULL, NULL, TRUE, 0, &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pin == &filter1.source2.pin.pin.IPin_iface, "Got wrong pin.\n");
    IPin_Release(pin);

    hr = ICaptureGraphBuilder2_FindPin(capture_graph, (IUnknown *)&filter1.filter.IBaseFilter_iface,
            PINDIR_OUTPUT, NULL, NULL, TRUE, 1, &pin);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    /* Test categories. */

    filter1.source1.IKsPropertySet_iface.lpVtbl = &property_set_vtbl;
    filter1.source1.category = PIN_CATEGORY_CAPTURE;

    hr = ICaptureGraphBuilder2_FindPin(capture_graph, (IUnknown *)&filter1.filter.IBaseFilter_iface,
            PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, NULL, FALSE, 0, &pin);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(pin == &filter1.source1.pin.pin.IPin_iface, "Got wrong pin.\n");
    IPin_Release(pin);

    hr = ICaptureGraphBuilder2_FindPin(capture_graph, (IUnknown *)&filter1.filter.IBaseFilter_iface,
            PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, NULL, FALSE, 1, &pin);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    ref = ICaptureGraphBuilder2_Release(capture_graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IGraphBuilder_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(&filter1.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(&filter2.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

static void disconnect_pins(IGraphBuilder *graph, struct testsource *pin)
{
    HRESULT hr;
    hr = IGraphBuilder_Disconnect(graph, pin->pin.pin.peer);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IGraphBuilder_Disconnect(graph, &pin->pin.pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
}

static void check_smart_tee_pin_(int line, IPin *pin, const WCHAR *name)
{
    PIN_INFO info;
    GUID clsid;

    IPin_QueryPinInfo(pin, &info);
    ok_(__FILE__, line)(!wcscmp(info.achName, name), "Got name %s.\n", debugstr_w(info.achName));
    IBaseFilter_GetClassID(info.pFilter, &clsid);
    ok_(__FILE__, line)(IsEqualGUID(&clsid, &CLSID_SmartTee), "Got CLSID %s.\n", debugstr_guid(&clsid));
    IBaseFilter_Release(info.pFilter);
}
#define check_smart_tee_pin(pin, name) check_smart_tee_pin_(__LINE__, pin, name)

static void test_render_stream(void)
{
    static const GUID source_type = {0x1111};
    static const GUID sink1_type = {0x8888};
    static const GUID bad_type = {0x4444};

    ICaptureGraphBuilder2 *capture_graph = create_capture_graph();
    struct testfilter source, transform, sink, identity;
    IGraphBuilder *graph;
    HRESULT hr;
    ULONG ref;

    CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IGraphBuilder, (void **)&graph);
    hr = ICaptureGraphBuilder2_SetFiltergraph(capture_graph, graph);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    testfilter_init(&source);
    testfilter_init(&transform);
    testfilter_init(&sink);
    testfilter_init(&identity);
    IGraphBuilder_AddFilter(graph, &source.filter.IBaseFilter_iface, L"source");
    IGraphBuilder_AddFilter(graph, &transform.filter.IBaseFilter_iface, L"transform");
    IGraphBuilder_AddFilter(graph, &sink.filter.IBaseFilter_iface, L"sink");

    source.source_type = source_type;
    transform.sink_type = &source_type;
    transform.source_type = sink1_type;
    sink.sink_type = &sink1_type;

    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, NULL, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source.source1.pin.pin.peer == &transform.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(transform.source1.pin.pin.peer == &sink.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(!source.source2.pin.pin.peer, "Pin should not be connected.\n");
    ok(!sink.sink2.pin.pin.peer, "Pin should not be connected.\n");

    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, NULL, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source.source1.pin.pin.peer == &transform.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(transform.source1.pin.pin.peer == &sink.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    todo_wine ok(source.source2.pin.pin.peer == &transform.sink2.pin.pin.IPin_iface, "Got wrong connection.\n");
    todo_wine ok(transform.source2.pin.pin.peer == &sink.sink2.pin.pin.IPin_iface, "Got wrong connection.\n");

    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, NULL, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    todo_wine ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    todo_wine disconnect_pins(graph, &source.source2);
    todo_wine disconnect_pins(graph, &transform.source2);

    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, NULL, NULL,
            (IUnknown *)&transform.source2.pin.pin.IPin_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source.source1.pin.pin.peer == &transform.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(transform.source1.pin.pin.peer == &sink.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(transform.source2.pin.pin.peer == &sink.sink2.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(!source.source2.pin.pin.peer, "Pin should not be connected.\n");

    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, NULL, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    disconnect_pins(graph, &transform.source2);

    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, NULL, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &transform.filter.IBaseFilter_iface);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source.source1.pin.pin.peer == &transform.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(transform.source1.pin.pin.peer == &sink.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    todo_wine ok(source.source2.pin.pin.peer == &transform.sink2.pin.pin.IPin_iface, "Got wrong connection.\n");

    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, NULL, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source.source1.pin.pin.peer == &transform.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(transform.source1.pin.pin.peer == &sink.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    todo_wine ok(source.source2.pin.pin.peer == &transform.sink2.pin.pin.IPin_iface, "Got wrong connection.\n");
    todo_wine ok(transform.source2.pin.pin.peer == &sink.sink2.pin.pin.IPin_iface, "Got wrong connection.\n");
    todo_wine disconnect_pins(graph, &source.source2);
    todo_wine disconnect_pins(graph, &transform.source2);

    /* Test from a source pin. */
    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, NULL, NULL,
            (IUnknown *)&source.source1.pin.pin.IPin_iface, NULL, &sink.filter.IBaseFilter_iface);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source.source1.pin.pin.peer == &transform.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(transform.source1.pin.pin.peer == &sink.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    todo_wine ok(transform.source2.pin.pin.peer == &sink.sink2.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(!source.source2.pin.pin.peer, "Pin should not be connected.\n");
    todo_wine disconnect_pins(graph, &transform.source2);

    /* Only the first eligible source is tried. */
    source.source_type = bad_type;
    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, NULL, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == VFW_E_CANNOT_CONNECT, "Got hr %#lx.\n", hr);
    source.source_type = source_type;

    disconnect_pins(graph, &transform.source1);
    disconnect_pins(graph, &source.source1);

    /* Test intermediate filters. */

    IGraphBuilder_AddFilter(graph, &identity.filter.IBaseFilter_iface, L"identity");
    identity.source_type = source_type;
    identity.sink_type = &source_type;
    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, NULL, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface,
            &identity.filter.IBaseFilter_iface, &sink.filter.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source.source1.pin.pin.peer == &identity.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(identity.source1.pin.pin.peer == &transform.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(transform.source1.pin.pin.peer == &sink.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    disconnect_pins(graph, &source.source1);
    disconnect_pins(graph, &identity.source1);
    disconnect_pins(graph, &transform.source1);

    identity.sink_type = &bad_type;
    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, NULL, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface,
            &identity.filter.IBaseFilter_iface, &sink.filter.IBaseFilter_iface);
    todo_wine ok(hr == E_FAIL, "Got hr %#lx.\n", hr);
    ok(!source.source1.pin.pin.peer, "Pin should not be connected.\n");
    ok(!identity.source1.pin.pin.peer, "Pin should not be connected.\n");
    ok(!transform.source1.pin.pin.peer, "Pin should not be connected.\n");

    identity.source_type = sink1_type;
    identity.sink_type = &sink1_type;
    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, NULL, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface,
            &identity.filter.IBaseFilter_iface, &sink.filter.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source.source1.pin.pin.peer == &transform.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(transform.source1.pin.pin.peer == &identity.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(identity.source1.pin.pin.peer == &sink.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    disconnect_pins(graph, &source.source1);
    disconnect_pins(graph, &transform.source1);
    disconnect_pins(graph, &identity.source1);

    identity.source_type = bad_type;
    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, NULL, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface,
            &identity.filter.IBaseFilter_iface, &sink.filter.IBaseFilter_iface);
    ok(hr == VFW_E_CANNOT_CONNECT, "Got hr %#lx.\n", hr);
    ok(source.source1.pin.pin.peer == &transform.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(transform.source1.pin.pin.peer == &identity.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(!identity.source1.pin.pin.peer, "Pin should not be connected.\n");
    disconnect_pins(graph, &source.source1);
    disconnect_pins(graph, &transform.source1);

    /* Test media types. */

    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, NULL, &bad_type,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!source.source1.pin.pin.peer, "Pin should not be connected.\n");
    ok(!source.source2.pin.pin.peer, "Pin should not be connected.\n");
    ok(!sink.sink1.pin.pin.peer, "Pin should not be connected.\n");
    ok(!sink.sink2.pin.pin.peer, "Pin should not be connected.\n");

    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, NULL, &sink1_type,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!source.source1.pin.pin.peer, "Pin should not be connected.\n");
    ok(!source.source2.pin.pin.peer, "Pin should not be connected.\n");
    ok(!sink.sink1.pin.pin.peer, "Pin should not be connected.\n");
    ok(!sink.sink2.pin.pin.peer, "Pin should not be connected.\n");

    identity.source_type = sink1_type;
    identity.sink_type = &sink1_type;
    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, NULL, &sink1_type,
            (IUnknown *)&source.filter.IBaseFilter_iface,
            &identity.filter.IBaseFilter_iface, &sink.filter.IBaseFilter_iface);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!source.source1.pin.pin.peer, "Pin should not be connected.\n");
    ok(!source.source2.pin.pin.peer, "Pin should not be connected.\n");
    ok(!sink.sink1.pin.pin.peer, "Pin should not be connected.\n");
    ok(!sink.sink2.pin.pin.peer, "Pin should not be connected.\n");

    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, NULL, &source_type,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source.source1.pin.pin.peer == &transform.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(transform.source1.pin.pin.peer == &sink.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(!source.source2.pin.pin.peer, "Pin should not be connected.\n");
    ok(!sink.sink2.pin.pin.peer, "Pin should not be connected.\n");

    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, NULL, &sink1_type,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source.source1.pin.pin.peer == &transform.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(transform.source1.pin.pin.peer == &sink.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(transform.source2.pin.pin.peer == &sink.sink2.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(!source.source2.pin.pin.peer, "Pin should not be connected.\n");

    disconnect_pins(graph, &source.source1);
    disconnect_pins(graph, &transform.source1);
    disconnect_pins(graph, &transform.source2);

    /* Test categories. */

    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, &PIN_CATEGORY_CC, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    ok(!source.source1.pin.pin.peer, "Pin should not be connected.\n");
    ok(!source.source2.pin.pin.peer, "Pin should not be connected.\n");
    ok(!sink.sink1.pin.pin.peer, "Pin should not be connected.\n");
    ok(!sink.sink2.pin.pin.peer, "Pin should not be connected.\n");

    source.source1.IKsPropertySet_iface.lpVtbl = &property_set_vtbl;
    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, &PIN_CATEGORY_CC, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    ok(!source.source1.pin.pin.peer, "Pin should not be connected.\n");
    ok(!source.source2.pin.pin.peer, "Pin should not be connected.\n");
    ok(!sink.sink1.pin.pin.peer, "Pin should not be connected.\n");
    ok(!sink.sink2.pin.pin.peer, "Pin should not be connected.\n");

    source.source1.category = PIN_CATEGORY_CC;
    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, &PIN_CATEGORY_CC, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source.source1.pin.pin.peer == &transform.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(transform.source1.pin.pin.peer == &sink.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(!source.source2.pin.pin.peer, "Pin should not be connected.\n");
    ok(!sink.sink2.pin.pin.peer, "Pin should not be connected.\n");

    disconnect_pins(graph, &transform.source1);

    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, &PIN_CATEGORY_CC, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    ok(source.source1.pin.pin.peer == &transform.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(!transform.source1.pin.pin.peer, "Pin should not be connected.\n");

    transform.source1.IKsPropertySet_iface.lpVtbl = &property_set_vtbl;
    transform.source1.category = PIN_CATEGORY_CC;
    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, &PIN_CATEGORY_CC, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    todo_wine ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    ok(source.source1.pin.pin.peer == &transform.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    todo_wine ok(!transform.source1.pin.pin.peer, "Pin should not be connected.\n");
    todo_wine ok(!sink.sink1.pin.pin.peer, "Pin should not be connected.\n");

    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, &PIN_CATEGORY_CC, NULL,
            (IUnknown *)&source.source1.pin.pin.IPin_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    disconnect_pins(graph, &source.source1);

    /* Test the CAPTURE and PREVIEW categories. */

    source.source1.IKsPropertySet_iface.lpVtbl = transform.source1.IKsPropertySet_iface.lpVtbl = NULL;
    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, &PIN_CATEGORY_CAPTURE, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, &PIN_CATEGORY_PREVIEW, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    source.source1.IKsPropertySet_iface.lpVtbl = &property_set_vtbl;
    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, &PIN_CATEGORY_CAPTURE, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, &PIN_CATEGORY_PREVIEW, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    source.source1.category = PIN_CATEGORY_PREVIEW;
    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, &PIN_CATEGORY_CAPTURE, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    ok(!source.source1.pin.pin.peer, "Pin should not be connected.\n");
    todo_wine ok(!sink.sink1.pin.pin.peer, "Pin should not be connected.\n");

    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, &PIN_CATEGORY_PREVIEW, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(source.source1.pin.pin.peer == &transform.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(transform.source1.pin.pin.peer == &sink.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    disconnect_pins(graph, &transform.source1);

    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, &PIN_CATEGORY_PREVIEW, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    todo_wine ok(hr == E_FAIL, "Got hr %#lx.\n", hr);
    todo_wine disconnect_pins(graph, &source.source1);

    source.source1.category = PIN_CATEGORY_CAPTURE;
    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, &PIN_CATEGORY_CAPTURE, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_smart_tee_pin(source.source1.pin.pin.peer, L"Input");
    check_smart_tee_pin(transform.sink1.pin.pin.peer, L"Capture");
    ok(transform.source1.pin.pin.peer == &sink.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");

    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, &PIN_CATEGORY_CAPTURE, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, &PIN_CATEGORY_PREVIEW, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == VFW_S_NOPREVIEWPIN, "Got hr %#lx.\n", hr);
    check_smart_tee_pin(source.source1.pin.pin.peer, L"Input");
    check_smart_tee_pin(transform.sink1.pin.pin.peer, L"Capture");
    check_smart_tee_pin(transform.sink2.pin.pin.peer, L"Preview");
    ok(transform.source1.pin.pin.peer == &sink.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(transform.source2.pin.pin.peer == &sink.sink2.pin.pin.IPin_iface, "Got wrong connection.\n");

    disconnect_pins(graph, &source.source1);
    IGraphBuilder_RemoveFilter(graph, &transform.filter.IBaseFilter_iface);
    IGraphBuilder_AddFilter(graph, &transform.filter.IBaseFilter_iface, L"transform");

    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, &PIN_CATEGORY_CC, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, &PIN_CATEGORY_PREVIEW, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == VFW_S_NOPREVIEWPIN, "Got hr %#lx.\n", hr);
    check_smart_tee_pin(source.source1.pin.pin.peer, L"Input");
    check_smart_tee_pin(transform.sink1.pin.pin.peer, L"Preview");
    ok(transform.source1.pin.pin.peer == &sink.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");

    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, &PIN_CATEGORY_PREVIEW, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    disconnect_pins(graph, &source.source1);
    IGraphBuilder_RemoveFilter(graph, &transform.filter.IBaseFilter_iface);
    IGraphBuilder_AddFilter(graph, &transform.filter.IBaseFilter_iface, L"transform");

    /* Test from the pin. */

    source.source1.category = PIN_CATEGORY_CAPTURE;
    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, &PIN_CATEGORY_CAPTURE, NULL,
            (IUnknown *)&source.source1.pin.pin.IPin_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_smart_tee_pin(source.source1.pin.pin.peer, L"Input");
    check_smart_tee_pin(transform.sink1.pin.pin.peer, L"Capture");
    ok(transform.source1.pin.pin.peer == &sink.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    disconnect_pins(graph, &source.source1);
    IGraphBuilder_RemoveFilter(graph, &transform.filter.IBaseFilter_iface);
    IGraphBuilder_AddFilter(graph, &transform.filter.IBaseFilter_iface, L"transform");

    /* Test when both CAPTURE and PREVIEW are available. */

    source.source2.IKsPropertySet_iface.lpVtbl = &property_set_vtbl;
    source.source2.category = PIN_CATEGORY_PREVIEW;

    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, &PIN_CATEGORY_CAPTURE, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source.source1.pin.pin.peer == &transform.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(transform.source1.pin.pin.peer == &sink.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    disconnect_pins(graph, &source.source1);
    disconnect_pins(graph, &transform.source1);

    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, &PIN_CATEGORY_PREVIEW, NULL,
            (IUnknown *)&source.filter.IBaseFilter_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(source.source2.pin.pin.peer == &transform.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    ok(transform.source1.pin.pin.peer == &sink.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    disconnect_pins(graph, &source.source2);
    disconnect_pins(graph, &transform.source1);

    hr = ICaptureGraphBuilder2_RenderStream(capture_graph, &PIN_CATEGORY_CAPTURE, NULL,
            (IUnknown *)&source.source1.pin.pin.IPin_iface, NULL, &sink.filter.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    check_smart_tee_pin(source.source1.pin.pin.peer, L"Input");
    check_smart_tee_pin(transform.sink1.pin.pin.peer, L"Capture");
    ok(transform.source1.pin.pin.peer == &sink.sink1.pin.pin.IPin_iface, "Got wrong connection.\n");
    disconnect_pins(graph, &source.source1);
    disconnect_pins(graph, &transform.source1);

    ref = ICaptureGraphBuilder2_Release(capture_graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IGraphBuilder_Release(graph);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(&source.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(&transform.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
    ref = IBaseFilter_Release(&sink.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %ld.\n", ref);
}

START_TEST(capturegraph)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    test_find_interface();
    test_find_pin();
    test_render_stream();

    CoUninitialize();
}
