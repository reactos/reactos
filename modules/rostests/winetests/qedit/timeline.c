/*
 * Unit tests for Timeline
 *
 * Copyright (C) 2016 Alex Henrie
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
#include "qedit.h"
#include "wine/test.h"

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

static const GUID test_iid = {0x33333333};
static LONG outer_ref = 1;

static HRESULT WINAPI outer_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IAMTimeline)
            || IsEqualGUID(iid, &test_iid))
    {
        *out = (IUnknown *)0xdeadbeef;
        return S_OK;
    }
    ok(0, "unexpected call %s\n", wine_dbgstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI outer_AddRef(IUnknown *iface)
{
    return InterlockedIncrement(&outer_ref);
}

static ULONG WINAPI outer_Release(IUnknown *iface)
{
    return InterlockedDecrement(&outer_ref);
}

static const IUnknownVtbl outer_vtbl =
{
    outer_QueryInterface,
    outer_AddRef,
    outer_Release,
};

static IUnknown test_outer = {&outer_vtbl};

static void test_aggregation(void)
{
    IAMTimeline *timeline, *timeline2;
    IUnknown *unk, *unk2;
    HRESULT hr;
    ULONG ref;

    timeline = (IAMTimeline *)0xdeadbeef;
    hr = CoCreateInstance(&CLSID_AMTimeline, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IAMTimeline, (void **)&timeline);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!timeline, "Got interface %p.\n", timeline);

    hr = CoCreateInstance(&CLSID_AMTimeline, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(outer_ref == 1, "Got unexpected refcount %ld.\n", outer_ref);
    ok(unk != &test_outer, "Returned IUnknown should not be outer IUnknown.\n");
    ref = get_refcount(unk);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);

    ref = IUnknown_AddRef(unk);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %ld.\n", outer_ref);

    ref = IUnknown_Release(unk);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %ld.\n", outer_ref);

    hr = IUnknown_QueryInterface(unk, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk2 == unk, "Got unexpected IUnknown %p.\n", unk2);
    IUnknown_Release(unk2);

    hr = IUnknown_QueryInterface(unk, &IID_IAMTimeline, (void **)&timeline);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IAMTimeline_QueryInterface(timeline, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "Got unexpected IUnknown %p.\n", unk2);

    hr = IAMTimeline_QueryInterface(timeline, &IID_IAMTimeline, (void **)&timeline2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(timeline2 == (IAMTimeline *)0xdeadbeef, "Got unexpected IAMTimeline %p.\n", timeline2);

    hr = IUnknown_QueryInterface(unk, &test_iid, (void **)&unk2);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!unk2, "Got unexpected IUnknown %p.\n", unk2);

    hr = IAMTimeline_QueryInterface(timeline, &test_iid, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "Got unexpected IUnknown %p.\n", unk2);

    IAMTimeline_Release(timeline);
    ref = IUnknown_Release(unk);
    ok(!ref, "Got unexpected refcount %ld.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %ld.\n", outer_ref);
}

static void test_timeline(void)
{
    HRESULT hr;
    IAMTimeline *timeline = NULL;
    IAMTimeline *timeline2 = (IAMTimeline *)0xdeadbeef;
    IAMTimelineObj *obj = (IAMTimelineObj *)0xdeadbeef;
    IAMTimelineObj obj_iface;
    TIMELINE_MAJOR_TYPE type;

    hr = CoCreateInstance(&CLSID_AMTimeline, NULL, CLSCTX_INPROC_SERVER, &IID_IAMTimeline, (void **)&timeline);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "Got hr %#lx.\n", hr);
    if (!timeline) return;

    hr = IAMTimeline_QueryInterface(timeline, &IID_IAMTimelineObj, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IAMTimeline_QueryInterface(timeline, &IID_IAMTimelineObj, (void **)&obj);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!obj, "Expected NULL got %p\n", obj);

    hr = IAMTimeline_CreateEmptyNode(timeline, NULL, 0);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IAMTimeline_CreateEmptyNode(timeline, NULL, TIMELINE_MAJOR_TYPE_COMPOSITE);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    for (type = 0; type < 256; type++)
    {
        obj = &obj_iface;
        hr = IAMTimeline_CreateEmptyNode(timeline, &obj, type);
        switch (type)
        {
            case TIMELINE_MAJOR_TYPE_COMPOSITE:
            case TIMELINE_MAJOR_TYPE_TRACK:
            case TIMELINE_MAJOR_TYPE_SOURCE:
            case TIMELINE_MAJOR_TYPE_TRANSITION:
            case TIMELINE_MAJOR_TYPE_EFFECT:
            case TIMELINE_MAJOR_TYPE_GROUP:
                ok(hr == S_OK, "Got hr %#lx.\n", hr);
                if (obj != &obj_iface) IAMTimelineObj_Release(obj);
                break;
            default:
                ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
                ok(obj == &obj_iface, "Expected %p got %p\n", &obj_iface, obj);
        }
    }

    obj = NULL;
    hr = IAMTimeline_CreateEmptyNode(timeline, &obj, TIMELINE_MAJOR_TYPE_COMPOSITE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (!obj) return;

    hr = IAMTimelineObj_QueryInterface(obj, &IID_IAMTimeline, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IAMTimelineObj_QueryInterface(obj, &IID_IAMTimeline, (void **)&timeline2);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!timeline2, "Expected NULL got %p\n", timeline2);

    hr = IAMTimelineObj_GetTimelineType(obj, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    hr = IAMTimelineObj_GetTimelineType(obj, &type);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(type == TIMELINE_MAJOR_TYPE_COMPOSITE, "Expected TIMELINE_MAJOR_TYPE_COMPOSITE got %d\n", type);

    for (type = 0; type < 256; type++)
    {
        hr = IAMTimelineObj_SetTimelineType(obj, type);
        if (type == TIMELINE_MAJOR_TYPE_COMPOSITE)
            ok(hr == S_OK, "Got hr %#lx.\n", hr);
        else
            ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    }

    hr = IAMTimelineObj_GetTimelineNoRef(obj, NULL);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

    timeline2 = (IAMTimeline *)0xdeadbeef;
    hr = IAMTimelineObj_GetTimelineNoRef(obj, &timeline2);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!timeline2, "Expected NULL got %p\n", timeline2);

    IAMTimelineObj_Release(obj);
    IAMTimeline_Release(timeline);
}

static void test_timelineobj_interfaces(void)
{
    HRESULT hr;
    IAMTimeline *timeline = NULL;
    IAMTimelineObj *obj;

    hr = CoCreateInstance(&CLSID_AMTimeline, NULL, CLSCTX_INPROC_SERVER, &IID_IAMTimeline, (void **)&timeline);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "Got hr %#lx.\n", hr);
    if (!timeline)
        return;

    hr = IAMTimeline_CreateEmptyNode(timeline, &obj, TIMELINE_MAJOR_TYPE_GROUP);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if(hr == S_OK)
    {
        IAMTimelineGroup *group;
        IAMTimelineObj *obj2;

        hr = IAMTimelineObj_QueryInterface(obj, &IID_IAMTimelineGroup, (void **)&group);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = IAMTimelineGroup_QueryInterface(group, &IID_IAMTimelineObj, (void **)&obj2);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(obj == obj2, "Different pointers\n");
        IAMTimelineObj_Release(obj2);

        IAMTimelineGroup_Release(group);

        IAMTimelineObj_Release(obj);
    }

    IAMTimeline_Release(timeline);
}

START_TEST(timeline)
{
    IAMTimeline *timeline;
    HRESULT hr;

    CoInitialize(NULL);

    if (FAILED(hr = CoCreateInstance(&CLSID_AMTimeline, NULL, CLSCTX_INPROC_SERVER,
            &IID_IAMTimeline, (void **)&timeline)))
    {
        /* qedit.dll does not exist on 2003. */
        win_skip("Failed to create timeline object, hr %#lx.\n", hr);
        return;
    }
    IAMTimeline_Release(timeline);

    test_aggregation();
    test_timeline();
    test_timelineobj_interfaces();

    CoUninitialize();
}
