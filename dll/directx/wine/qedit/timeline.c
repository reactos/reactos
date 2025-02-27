/*              DirectShow Timeline object (QEDIT.DLL)
 *
 * Copyright 2016 Alex Henrie
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

#include <assert.h>
#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "qedit_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

typedef struct {
    IUnknown IUnknown_inner;
    IAMTimeline IAMTimeline_iface;
    IUnknown *outer_unk;
    LONG ref;
} TimelineImpl;

static inline TimelineImpl *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, TimelineImpl, IUnknown_inner);
}

static inline TimelineImpl *impl_from_IAMTimeline(IAMTimeline *iface)
{
    return CONTAINING_RECORD(iface, TimelineImpl, IAMTimeline_iface);
}

typedef struct {
    IAMTimelineObj IAMTimelineObj_iface;
    IAMTimelineGroup IAMTimelineGroup_iface;
    LONG ref;
    TIMELINE_MAJOR_TYPE timeline_type;
} TimelineObjImpl;

static inline TimelineObjImpl *impl_from_IAMTimelineObj(IAMTimelineObj *iface)
{
    return CONTAINING_RECORD(iface, TimelineObjImpl, IAMTimelineObj_iface);
}

static inline TimelineObjImpl *impl_from_IAMTimelineGroup(IAMTimelineGroup *iface)
{
    return CONTAINING_RECORD(iface, TimelineObjImpl, IAMTimelineGroup_iface);
}

static const IAMTimelineObjVtbl IAMTimelineObj_VTable;
static const IAMTimelineGroupVtbl IAMTimelineGroup_VTable;

/* Timeline inner IUnknown */

static HRESULT WINAPI Timeline_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    TimelineImpl *This = impl_from_IUnknown(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;

    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = &This->IUnknown_inner;
    else if (IsEqualIID(riid, &IID_IAMTimeline))
        *ppv = &This->IAMTimeline_iface;
    else
        WARN("(%p, %s,%p): not found\n", This, debugstr_guid(riid), ppv);

    if (!*ppv)
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI Timeline_AddRef(IUnknown *iface)
{
    TimelineImpl *timeline = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&timeline->ref);

    TRACE("%p increasing refcount to %lu.\n", timeline, refcount);

    return refcount;
}

static ULONG WINAPI Timeline_Release(IUnknown *iface)
{
    TimelineImpl *timeline = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&timeline->ref);

    TRACE("%p decreasing refcount to %lu.\n", timeline, refcount);

    if (!refcount)
        CoTaskMemFree(timeline);

    return refcount;
}

static const IUnknownVtbl timeline_vtbl =
{
    Timeline_QueryInterface,
    Timeline_AddRef,
    Timeline_Release,
};

/* IAMTimeline implementation */

static HRESULT WINAPI Timeline_IAMTimeline_QueryInterface(IAMTimeline *iface, REFIID riid, void **ppv)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI Timeline_IAMTimeline_AddRef(IAMTimeline *iface)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI Timeline_IAMTimeline_Release(IAMTimeline *iface)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI Timeline_IAMTimeline_CreateEmptyNode(IAMTimeline *iface, IAMTimelineObj **obj,
                                                           TIMELINE_MAJOR_TYPE type)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    TimelineObjImpl* obj_impl;

    TRACE("(%p)->(%p,%d)\n", This, obj, type);

    if (!obj)
        return E_POINTER;

    switch (type)
    {
        case TIMELINE_MAJOR_TYPE_COMPOSITE:
        case TIMELINE_MAJOR_TYPE_TRACK:
        case TIMELINE_MAJOR_TYPE_SOURCE:
        case TIMELINE_MAJOR_TYPE_TRANSITION:
        case TIMELINE_MAJOR_TYPE_EFFECT:
        case TIMELINE_MAJOR_TYPE_GROUP:
            break;
        default:
            return E_INVALIDARG;
    }

    obj_impl = CoTaskMemAlloc(sizeof(TimelineObjImpl));
    if (!obj_impl) {
        *obj = NULL;
        return E_OUTOFMEMORY;
    }

    obj_impl->ref = 1;
    obj_impl->IAMTimelineObj_iface.lpVtbl = &IAMTimelineObj_VTable;
    obj_impl->IAMTimelineGroup_iface.lpVtbl = &IAMTimelineGroup_VTable;
    obj_impl->timeline_type = type;

    *obj = &obj_impl->IAMTimelineObj_iface;
    return S_OK;
}

static HRESULT WINAPI Timeline_IAMTimeline_AddGroup(IAMTimeline *iface, IAMTimelineObj *group)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, group);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_RemGroupFromList(IAMTimeline *iface, IAMTimelineObj *group)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, group);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_GetGroup(IAMTimeline *iface, IAMTimelineObj **group, LONG index)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p,%ld): not implemented!\n", This, group, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_GetGroupCount(IAMTimeline *iface, LONG *count)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_ClearAllGroups(IAMTimeline *iface)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p): not implemented!\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_GetInsertMode(IAMTimeline *iface, LONG *mode)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, mode);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_SetInsertMode(IAMTimeline *iface, LONG mode)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%ld): not implemented!\n", This, mode);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_EnableTransitions(IAMTimeline *iface, BOOL enabled)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%d): not implemented!\n", This, enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_TransitionsEnabled(IAMTimeline *iface, BOOL *enabled)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_EnableEffects(IAMTimeline *iface, BOOL enabled)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%d): not implemented!\n", This, enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_EffectsEnabled(IAMTimeline *iface, BOOL *enabled)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_SetInterestRange(IAMTimeline *iface, REFERENCE_TIME start,
                                                            REFERENCE_TIME stop)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%s,%s): not implemented!\n", This, wine_dbgstr_longlong(start),
          wine_dbgstr_longlong(stop));
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_GetDuration(IAMTimeline *iface, REFERENCE_TIME *duration)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, duration);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_GetDuration2(IAMTimeline *iface, double *duration)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, duration);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_SetDefaultFPS(IAMTimeline *iface, double fps)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%f): not implemented!\n", This, fps);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_GetDefaultFPS(IAMTimeline *iface, double *fps)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, fps);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_IsDirty(IAMTimeline *iface, BOOL *dirty)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, dirty);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_GetDirtyRange(IAMTimeline *iface, REFERENCE_TIME *start,
                                                         REFERENCE_TIME *stop)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p,%p): not implemented!\n", This, start, stop);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_GetCountOfType(IAMTimeline *iface, LONG group, LONG *value,
                                                          LONG *value_with_comps, TIMELINE_MAJOR_TYPE type)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%ld,%p,%p,%#x): not implemented!\n", This, group, value, value_with_comps, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_ValidateSourceNames(IAMTimeline *iface, LONG flags, IMediaLocator *override,
                                                               LONG_PTR notify_event)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%ld,%p,%#Ix): not implemented!\n", This, flags, override, notify_event);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_SetDefaultTransition(IAMTimeline *iface, GUID *guid)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%s): not implemented!\n", This, wine_dbgstr_guid(guid));
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_GetDefaultTransition(IAMTimeline *iface, GUID *guid)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%s): not implemented!\n", This, wine_dbgstr_guid(guid));
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_SetDefaultEffect(IAMTimeline *iface, GUID *guid)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%s): not implemented!\n", This, wine_dbgstr_guid(guid));
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_GetDefaultEffect(IAMTimeline *iface, GUID *guid)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%s): not implemented!\n", This, wine_dbgstr_guid(guid));
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_SetDefaultTransitionB(IAMTimeline *iface, BSTR guidb)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, guidb);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_GetDefaultTransitionB(IAMTimeline *iface, BSTR *guidb)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, guidb);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_SetDefaultEffectB(IAMTimeline *iface, BSTR guidb)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, guidb);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_GetDefaultEffectB(IAMTimeline *iface, BSTR *guidb)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, guidb);
    return E_NOTIMPL;
}

static const IAMTimelineVtbl IAMTimeline_VTable =
{
    Timeline_IAMTimeline_QueryInterface,
    Timeline_IAMTimeline_AddRef,
    Timeline_IAMTimeline_Release,
    Timeline_IAMTimeline_CreateEmptyNode,
    Timeline_IAMTimeline_AddGroup,
    Timeline_IAMTimeline_RemGroupFromList,
    Timeline_IAMTimeline_GetGroup,
    Timeline_IAMTimeline_GetGroupCount,
    Timeline_IAMTimeline_ClearAllGroups,
    Timeline_IAMTimeline_GetInsertMode,
    Timeline_IAMTimeline_SetInsertMode,
    Timeline_IAMTimeline_EnableTransitions,
    Timeline_IAMTimeline_TransitionsEnabled,
    Timeline_IAMTimeline_EnableEffects,
    Timeline_IAMTimeline_EffectsEnabled,
    Timeline_IAMTimeline_SetInterestRange,
    Timeline_IAMTimeline_GetDuration,
    Timeline_IAMTimeline_GetDuration2,
    Timeline_IAMTimeline_SetDefaultFPS,
    Timeline_IAMTimeline_GetDefaultFPS,
    Timeline_IAMTimeline_IsDirty,
    Timeline_IAMTimeline_GetDirtyRange,
    Timeline_IAMTimeline_GetCountOfType,
    Timeline_IAMTimeline_ValidateSourceNames,
    Timeline_IAMTimeline_SetDefaultTransition,
    Timeline_IAMTimeline_GetDefaultTransition,
    Timeline_IAMTimeline_SetDefaultEffect,
    Timeline_IAMTimeline_GetDefaultEffect,
    Timeline_IAMTimeline_SetDefaultTransitionB,
    Timeline_IAMTimeline_GetDefaultTransitionB,
    Timeline_IAMTimeline_SetDefaultEffectB,
    Timeline_IAMTimeline_GetDefaultEffectB,
};

HRESULT timeline_create(IUnknown *pUnkOuter, IUnknown **ppv)
{
    TimelineImpl* obj = NULL;

    TRACE("(%p,%p)\n", pUnkOuter, ppv);

    obj = CoTaskMemAlloc(sizeof(TimelineImpl));
    if (NULL == obj) {
        *ppv = NULL;
        return E_OUTOFMEMORY;
    }
    ZeroMemory(obj, sizeof(TimelineImpl));

    obj->ref = 1;
    obj->IUnknown_inner.lpVtbl = &timeline_vtbl;
    obj->IAMTimeline_iface.lpVtbl = &IAMTimeline_VTable;

    if (pUnkOuter)
        obj->outer_unk = pUnkOuter;
    else
        obj->outer_unk = &obj->IUnknown_inner;

    *ppv = &obj->IUnknown_inner;
    return S_OK;
}

/* IAMTimelineObj implementation */

static HRESULT WINAPI TimelineObj_QueryInterface(IAMTimelineObj *iface, REFIID riid, void **ppv)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;

    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IAMTimelineObj))
        *ppv = &This->IAMTimelineObj_iface;
    else if (IsEqualIID(riid, &IID_IAMTimelineGroup))
        *ppv = &This->IAMTimelineGroup_iface;
    else
        WARN("(%p, %s,%p): not found\n", This, debugstr_guid(riid), ppv);

    if (!*ppv)
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI TimelineObj_AddRef(IAMTimelineObj *iface)
{
    TimelineObjImpl *obj = impl_from_IAMTimelineObj(iface);
    ULONG refcount = InterlockedIncrement(&obj->ref);

    TRACE("%p increasing refcount to %lu.\n", obj, refcount);

    return refcount;
}

static ULONG WINAPI TimelineObj_Release(IAMTimelineObj *iface)
{
    TimelineObjImpl *obj = impl_from_IAMTimelineObj(iface);
    ULONG refcount = InterlockedDecrement(&obj->ref);

    TRACE("%p decreasing refcount to %lu.\n", obj, refcount);

    if (!refcount)
        CoTaskMemFree(obj);

    return refcount;
}

static HRESULT WINAPI TimelineObj_GetStartStop(IAMTimelineObj *iface, REFERENCE_TIME *start, REFERENCE_TIME *stop)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%p,%p): not implemented!\n", This, start, stop);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_GetStartStop2(IAMTimelineObj *iface, REFTIME *start, REFTIME *stop)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%p,%p): not implemented!\n", This, start, stop);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_FixTimes(IAMTimelineObj *iface, REFERENCE_TIME *start, REFERENCE_TIME *stop)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%p,%p): not implemented!\n", This, start, stop);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_FixTimes2(IAMTimelineObj *iface, REFTIME *start, REFTIME *stop)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%p,%p): not implemented!\n", This, start, stop);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_SetStartStop(IAMTimelineObj *iface, REFERENCE_TIME start, REFERENCE_TIME stop)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%s,%s): not implemented!\n", This, wine_dbgstr_longlong(start), wine_dbgstr_longlong(stop));
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_SetStartStop2(IAMTimelineObj *iface, REFTIME start, REFTIME stop)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%f,%f): not implemented!\n", This, start, stop);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_GetPropertySetter(IAMTimelineObj *iface, IPropertySetter **setter)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, setter);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_SetPropertySetter(IAMTimelineObj *iface, IPropertySetter *setter)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, setter);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_GetSubObject(IAMTimelineObj *iface, IUnknown **obj)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, obj);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_SetSubObject(IAMTimelineObj *iface, IUnknown *obj)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, obj);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_SetSubObjectGUID(IAMTimelineObj *iface, GUID guid)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%s): not implemented!\n", This, wine_dbgstr_guid(&guid));
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_SetSubObjectGUIDB(IAMTimelineObj *iface, BSTR guidb)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%s): not implemented!\n", This, wine_dbgstr_w(guidb));
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_GetSubObjectGUID(IAMTimelineObj *iface, GUID *guid)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, guid);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_GetSubObjectGUIDB(IAMTimelineObj *iface, BSTR *guidb)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, guidb);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_GetSubObjectLoaded(IAMTimelineObj *iface, BOOL *loaded)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, loaded);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_GetTimelineType(IAMTimelineObj *iface, TIMELINE_MAJOR_TYPE *type)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    TRACE("(%p)->(%p)\n", This, type);
    if (!type) return E_POINTER;
    *type = This->timeline_type;
    return S_OK;
}

static HRESULT WINAPI TimelineObj_SetTimelineType(IAMTimelineObj *iface, TIMELINE_MAJOR_TYPE type)
{
    /* MSDN says that this function is "not supported" */
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    TRACE("(%p)->(%d)\n", This, type);
    if (type != This->timeline_type) return E_INVALIDARG;
    return S_OK;
}

static HRESULT WINAPI TimelineObj_GetUserID(IAMTimelineObj *iface, LONG *id)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_SetUserID(IAMTimelineObj *iface, LONG id)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%ld): not implemented!\n", This, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_GetGenID(IAMTimelineObj *iface, LONG *id)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_GetUserName(IAMTimelineObj *iface, BSTR *name)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, name);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_SetUserName(IAMTimelineObj *iface, BSTR name)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%s): not implemented!\n", This, wine_dbgstr_w(name));
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_GetUserData(IAMTimelineObj *iface, BYTE *data, LONG *size)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%p,%p): not implemented!\n", This, data, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_SetUserData(IAMTimelineObj *iface, BYTE *data, LONG size)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%p,%ld): not implemented!\n", This, data, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_GetMuted(IAMTimelineObj *iface, BOOL *muted)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, muted);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_SetMuted(IAMTimelineObj *iface, BOOL muted)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%d): not implemented!\n", This, muted);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_GetLocked(IAMTimelineObj *iface, BOOL *locked)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, locked);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_SetLocked(IAMTimelineObj *iface, BOOL locked)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%d): not implemented!\n", This, locked);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_GetDirtyRange(IAMTimelineObj *iface, REFERENCE_TIME *start, REFERENCE_TIME *stop)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%p,%p): not implemented!\n", This, start, stop);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_GetDirtyRange2(IAMTimelineObj *iface, REFTIME *start, REFTIME *stop)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%p,%p): not implemented!\n", This, start, stop);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_SetDirtyRange(IAMTimelineObj *iface, REFERENCE_TIME start, REFERENCE_TIME stop)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%s,%s): not implemented!\n", This, wine_dbgstr_longlong(start), wine_dbgstr_longlong(stop));
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_SetDirtyRange2(IAMTimelineObj *iface, REFTIME start, REFTIME stop)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%f,%f): not implemented!\n", This, start, stop);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_ClearDirty(IAMTimelineObj *iface)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p): not implemented!\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_Remove(IAMTimelineObj *iface)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p): not implemented!\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_RemoveAll(IAMTimelineObj *iface)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p): not implemented!\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_GetTimelineNoRef(IAMTimelineObj *iface, IAMTimeline **timeline)
{
    /* MSDN says that this function is "not supported" */
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    TRACE("(%p)->(%p)\n", This, timeline);
    if (!timeline) return E_POINTER;
    *timeline = NULL;
    return E_NOINTERFACE;
}

static HRESULT WINAPI TimelineObj_GetGroupIBelongTo(IAMTimelineObj *iface, IAMTimelineGroup **group)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, group);
    return E_NOTIMPL;
}

static HRESULT WINAPI TimelineObj_GetEmbedDepth(IAMTimelineObj *iface, LONG *depth)
{
    TimelineObjImpl *This = impl_from_IAMTimelineObj(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, depth);
    return E_NOTIMPL;
}

static const IAMTimelineObjVtbl IAMTimelineObj_VTable =
{
    TimelineObj_QueryInterface,
    TimelineObj_AddRef,
    TimelineObj_Release,
    TimelineObj_GetStartStop,
    TimelineObj_GetStartStop2,
    TimelineObj_FixTimes,
    TimelineObj_FixTimes2,
    TimelineObj_SetStartStop,
    TimelineObj_SetStartStop2,
    TimelineObj_GetPropertySetter,
    TimelineObj_SetPropertySetter,
    TimelineObj_GetSubObject,
    TimelineObj_SetSubObject,
    TimelineObj_SetSubObjectGUID,
    TimelineObj_SetSubObjectGUIDB,
    TimelineObj_GetSubObjectGUID,
    TimelineObj_GetSubObjectGUIDB,
    TimelineObj_GetSubObjectLoaded,
    TimelineObj_GetTimelineType,
    TimelineObj_SetTimelineType,
    TimelineObj_GetUserID,
    TimelineObj_SetUserID,
    TimelineObj_GetGenID,
    TimelineObj_GetUserName,
    TimelineObj_SetUserName,
    TimelineObj_GetUserData,
    TimelineObj_SetUserData,
    TimelineObj_GetMuted,
    TimelineObj_SetMuted,
    TimelineObj_GetLocked,
    TimelineObj_SetLocked,
    TimelineObj_GetDirtyRange,
    TimelineObj_GetDirtyRange2,
    TimelineObj_SetDirtyRange,
    TimelineObj_SetDirtyRange2,
    TimelineObj_ClearDirty,
    TimelineObj_Remove,
    TimelineObj_RemoveAll,
    TimelineObj_GetTimelineNoRef,
    TimelineObj_GetGroupIBelongTo,
    TimelineObj_GetEmbedDepth,
};

static HRESULT WINAPI timelinegrp_QueryInterface(IAMTimelineGroup *iface, REFIID riid, void **object)
{
    TimelineObjImpl *This = impl_from_IAMTimelineGroup(iface);
    return IAMTimelineObj_QueryInterface(&This->IAMTimelineObj_iface, riid, object);
}

static ULONG WINAPI timelinegrp_AddRef(IAMTimelineGroup *iface)
{
    TimelineObjImpl *This = impl_from_IAMTimelineGroup(iface);
    return IAMTimelineObj_AddRef(&This->IAMTimelineObj_iface);
}

static ULONG WINAPI timelinegrp_Release(IAMTimelineGroup *iface)
{
    TimelineObjImpl *This = impl_from_IAMTimelineGroup(iface);
    return IAMTimelineObj_Release(&This->IAMTimelineObj_iface);
}

static HRESULT WINAPI timelinegrp_SetTimeline(IAMTimelineGroup *iface, IAMTimeline *timeline)
{
    TimelineObjImpl *This = impl_from_IAMTimelineGroup(iface);
    FIXME("(%p)->(%p)\n", This, timeline);
    return E_NOTIMPL;
}

static HRESULT WINAPI timelinegrp_GetTimeline(IAMTimelineGroup *iface, IAMTimeline **timeline)
{
    TimelineObjImpl *This = impl_from_IAMTimelineGroup(iface);
    FIXME("(%p)->(%p)\n", This, timeline);
    return E_NOTIMPL;
}

static HRESULT WINAPI timelinegrp_GetPriority(IAMTimelineGroup *iface, LONG *priority)
{
    TimelineObjImpl *This = impl_from_IAMTimelineGroup(iface);
    FIXME("(%p)->(%p)\n", This, priority);
    return E_NOTIMPL;
}

static HRESULT WINAPI timelinegrp_GetMediaType(IAMTimelineGroup *iface, AM_MEDIA_TYPE *mediatype)
{
    TimelineObjImpl *This = impl_from_IAMTimelineGroup(iface);
    FIXME("(%p)->(%p)\n", This, mediatype);
    return E_NOTIMPL;
}

static HRESULT WINAPI timelinegrp_SetMediaType(IAMTimelineGroup *iface, AM_MEDIA_TYPE *mediatype)
{
    TimelineObjImpl *This = impl_from_IAMTimelineGroup(iface);
    FIXME("(%p)->(%p)\n", This, mediatype);
    return E_NOTIMPL;
}

static HRESULT WINAPI timelinegrp_SetOutputFPS(IAMTimelineGroup *iface, double fps)
{
    TimelineObjImpl *This = impl_from_IAMTimelineGroup(iface);
    FIXME("(%p)->(%f)\n", This, fps);
    return E_NOTIMPL;
}

static HRESULT WINAPI timelinegrp_GetOutputFPS(IAMTimelineGroup *iface, double *fps)
{
    TimelineObjImpl *This = impl_from_IAMTimelineGroup(iface);
    FIXME("(%p)->(%p)\n", This, fps);
    return E_NOTIMPL;
}

static HRESULT WINAPI timelinegrp_SetGroupName(IAMTimelineGroup *iface, BSTR name)
{
    TimelineObjImpl *This = impl_from_IAMTimelineGroup(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(name));
    return E_NOTIMPL;
}

static HRESULT WINAPI timelinegrp_GetGroupName(IAMTimelineGroup *iface, BSTR *name)
{
    TimelineObjImpl *This = impl_from_IAMTimelineGroup(iface);
    FIXME("(%p)->(%p)\n", This, name);
    return E_NOTIMPL;
}

static HRESULT WINAPI timelinegrp_SetPreviewMode(IAMTimelineGroup *iface, BOOL preview)
{
    TimelineObjImpl *This = impl_from_IAMTimelineGroup(iface);
    FIXME("(%p)->(%d)\n", This, preview);
    return E_NOTIMPL;
}

static HRESULT WINAPI timelinegrp_GetPreviewMode(IAMTimelineGroup *iface, BOOL *preview)
{
    TimelineObjImpl *This = impl_from_IAMTimelineGroup(iface);
    FIXME("(%p)->(%p)\n", This, preview);
    return E_NOTIMPL;
}

static HRESULT WINAPI timelinegrp_SetMediaTypeForVB(IAMTimelineGroup *iface, LONG type)
{
    TimelineObjImpl *This = impl_from_IAMTimelineGroup(iface);
    FIXME("(%p)->(%ld)\n", This, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI timelinegrp_GetOutputBuffering(IAMTimelineGroup *iface, int *buffer)
{
    TimelineObjImpl *This = impl_from_IAMTimelineGroup(iface);
    FIXME("(%p)->(%p)\n", This, buffer);
    return E_NOTIMPL;
}

static HRESULT WINAPI timelinegrp_SetOutputBuffering(IAMTimelineGroup *iface, int buffer)
{
    TimelineObjImpl *This = impl_from_IAMTimelineGroup(iface);
    FIXME("(%p)->(%d)\n", This, buffer);
    return E_NOTIMPL;
}

static HRESULT WINAPI timelinegrp_SetSmartRecompressFormat(IAMTimelineGroup *iface, LONG *format)
{
    TimelineObjImpl *This = impl_from_IAMTimelineGroup(iface);
    FIXME("(%p)->(%p)\n", This, format);
    return E_NOTIMPL;
}

static HRESULT WINAPI timelinegrp_GetSmartRecompressFormat(IAMTimelineGroup *iface, LONG **format)
{
    TimelineObjImpl *This = impl_from_IAMTimelineGroup(iface);
    FIXME("(%p)->(%p)\n", This, format);
    return E_NOTIMPL;
}

static HRESULT WINAPI timelinegrp_IsSmartRecompressFormatSet(IAMTimelineGroup *iface, BOOL *set)
{
    TimelineObjImpl *This = impl_from_IAMTimelineGroup(iface);
    FIXME("(%p)->(%p)\n", This, set);
    return E_NOTIMPL;
}

static HRESULT WINAPI timelinegrp_IsRecompressFormatDirty(IAMTimelineGroup *iface, BOOL *dirty)
{
    TimelineObjImpl *This = impl_from_IAMTimelineGroup(iface);
    FIXME("(%p)->(%p)\n", This, dirty);
    return E_NOTIMPL;
}

static HRESULT WINAPI timelinegrp_ClearRecompressFormatDirty(IAMTimelineGroup *iface)
{
    TimelineObjImpl *This = impl_from_IAMTimelineGroup(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI timelinegrp_SetRecompFormatFromSource(IAMTimelineGroup *iface, IAMTimelineSrc *source)
{
    TimelineObjImpl *This = impl_from_IAMTimelineGroup(iface);
    FIXME("(%p)->(%p)\n", This, source);
    return E_NOTIMPL;
}

static const IAMTimelineGroupVtbl IAMTimelineGroup_VTable =
{
    timelinegrp_QueryInterface,
    timelinegrp_AddRef,
    timelinegrp_Release,
    timelinegrp_SetTimeline,
    timelinegrp_GetTimeline,
    timelinegrp_GetPriority,
    timelinegrp_GetMediaType,
    timelinegrp_SetMediaType,
    timelinegrp_SetOutputFPS,
    timelinegrp_GetOutputFPS,
    timelinegrp_SetGroupName,
    timelinegrp_GetGroupName,
    timelinegrp_SetPreviewMode,
    timelinegrp_GetPreviewMode,
    timelinegrp_SetMediaTypeForVB,
    timelinegrp_GetOutputBuffering,
    timelinegrp_SetOutputBuffering,
    timelinegrp_SetSmartRecompressFormat,
    timelinegrp_GetSmartRecompressFormat,
    timelinegrp_IsSmartRecompressFormatSet,
    timelinegrp_IsRecompressFormatDirty,
    timelinegrp_ClearRecompressFormatDirty,
    timelinegrp_SetRecompFormatFromSource
};
