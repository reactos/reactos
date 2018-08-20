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
#define CONST_VTABLE

#include "wine/test.h"
#include "qedit.h"

static void test_timeline(void)
{
    HRESULT hr;
    IAMTimeline *timeline = NULL;
    IAMTimeline *timeline2 = (IAMTimeline *)(ULONG_PTR)0xdeadbeefdeadbeef;
    IAMTimelineObj *obj = (IAMTimelineObj *)(ULONG_PTR)0xdeadbeefdeadbeef;
    IAMTimelineObj obj_iface;
    TIMELINE_MAJOR_TYPE type;

    hr = CoCreateInstance(&CLSID_AMTimeline, NULL, CLSCTX_INPROC_SERVER, &IID_IAMTimeline, (void **)&timeline);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "CoCreateInstance failed: %08x\n", hr);
    if (!timeline) return;

    hr = IAMTimeline_QueryInterface(timeline, &IID_IAMTimelineObj, NULL);
    ok(hr == E_POINTER, "Expected E_POINTER got %08x\n", hr);

    hr = IAMTimeline_QueryInterface(timeline, &IID_IAMTimelineObj, (void **)&obj);
    ok(hr == E_NOINTERFACE, "Expected E_NOINTERFACE got %08x\n", hr);
    ok(!obj, "Expected NULL got %p\n", obj);

    hr = IAMTimeline_CreateEmptyNode(timeline, NULL, 0);
    ok(hr == E_POINTER, "Expected E_POINTER got %08x\n", hr);

    hr = IAMTimeline_CreateEmptyNode(timeline, NULL, TIMELINE_MAJOR_TYPE_COMPOSITE);
    ok(hr == E_POINTER, "Expected E_POINTER got %08x\n", hr);

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
                ok(hr == S_OK, "CreateEmptyNode failed: %08x\n", hr);
                if (obj != &obj_iface) IAMTimelineObj_Release(obj);
                break;
            default:
                ok(hr == E_INVALIDARG, "Expected E_INVALIDARG got %08x\n", hr);
                ok(obj == &obj_iface, "Expected %p got %p\n", &obj_iface, obj);
        }
    }

    obj = NULL;
    hr = IAMTimeline_CreateEmptyNode(timeline, &obj, TIMELINE_MAJOR_TYPE_COMPOSITE);
    ok(hr == S_OK, "CreateEmptyNode failed: %08x\n", hr);
    if (!obj) return;

    hr = IAMTimelineObj_QueryInterface(obj, &IID_IAMTimeline, NULL);
    ok(hr == E_POINTER, "Expected E_POINTER got %08x\n", hr);

    hr = IAMTimelineObj_QueryInterface(obj, &IID_IAMTimeline, (void **)&timeline2);
    ok(hr == E_NOINTERFACE, "Expected E_NOINTERFACE got %08x\n", hr);
    ok(!timeline2, "Expected NULL got %p\n", timeline2);

    hr = IAMTimelineObj_GetTimelineType(obj, NULL);
    ok(hr == E_POINTER, "Expected E_POINTER got %08x\n", hr);

    hr = IAMTimelineObj_GetTimelineType(obj, &type);
    ok(hr == S_OK, "GetTimelineType failed: %08x\n", hr);
    ok(type == TIMELINE_MAJOR_TYPE_COMPOSITE, "Expected TIMELINE_MAJOR_TYPE_COMPOSITE got %d\n", type);

    for (type = 0; type < 256; type++)
    {
        hr = IAMTimelineObj_SetTimelineType(obj, type);
        if (type == TIMELINE_MAJOR_TYPE_COMPOSITE)
            ok(hr == S_OK, "SetTimelineType failed: %08x\n", hr);
        else
            ok(hr == E_INVALIDARG, "Expected E_INVALIDARG got %08x\n", hr);
    }

    hr = IAMTimelineObj_GetTimelineNoRef(obj, NULL);
    ok(hr == E_POINTER, "Expected E_POINTER got %08x\n", hr);

    timeline2 = (IAMTimeline *)(ULONG_PTR)0xdeadbeefdeadbeef;
    hr = IAMTimelineObj_GetTimelineNoRef(obj, &timeline2);
    ok(hr == E_NOINTERFACE, "Expected E_NOINTERFACE got %08x\n", hr);
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
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "CoCreateInstance failed: %08x\n", hr);
    if (!timeline)
        return;

    hr = IAMTimeline_CreateEmptyNode(timeline, &obj, TIMELINE_MAJOR_TYPE_GROUP);
    ok(hr == S_OK, "CreateEmptyNode failed: %08x\n", hr);
    if(hr == S_OK)
    {
        IAMTimelineGroup *group;
        IAMTimelineObj *obj2;

        hr = IAMTimelineObj_QueryInterface(obj, &IID_IAMTimelineGroup, (void **)&group);
        ok(hr == S_OK, "got %08x\n", hr);

        hr = IAMTimelineGroup_QueryInterface(group, &IID_IAMTimelineObj, (void **)&obj2);
        ok(hr == S_OK, "got %08x\n", hr);
        ok(obj == obj2, "Different pointers\n");
        IAMTimelineObj_Release(obj2);

        IAMTimelineGroup_Release(group);

        IAMTimelineObj_Release(obj);
    }

    IAMTimeline_Release(timeline);
}

START_TEST(timeline)
{
    CoInitialize(NULL);
    test_timeline();
    test_timelineobj_interfaces();
    CoUninitialize();
}
