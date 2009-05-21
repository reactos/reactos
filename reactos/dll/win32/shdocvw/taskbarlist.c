/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
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
 *
 */

#include "config.h"
#include "wine/port.h"
#include "wine/debug.h"

#include "shdocvw.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

struct taskbar_list
{
    const struct ITaskbarListVtbl *lpVtbl;
    LONG refcount;
};

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE taskbar_list_QueryInterface(ITaskbarList *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ITaskbarList)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE taskbar_list_AddRef(ITaskbarList *iface)
{
    struct taskbar_list *This = (struct taskbar_list *)iface;
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE taskbar_list_Release(ITaskbarList *iface)
{
    struct taskbar_list *This = (struct taskbar_list *)iface;
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %u\n", This, refcount);

    if (!refcount)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refcount;
}

/* ITaskbarList methods */

static HRESULT STDMETHODCALLTYPE taskbar_list_HrInit(ITaskbarList *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE taskbar_list_AddTab(ITaskbarList *iface, HWND hwnd)
{
    FIXME("iface %p, hwnd %p stub!\n", iface, hwnd);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE taskbar_list_DeleteTab(ITaskbarList *iface, HWND hwnd)
{
    FIXME("iface %p, hwnd %p stub!\n", iface, hwnd);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE taskbar_list_ActivateTab(ITaskbarList *iface, HWND hwnd)
{
    FIXME("iface %p, hwnd %p stub!\n", iface, hwnd);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE taskbar_list_SetActiveAlt(ITaskbarList *iface, HWND hwnd)
{
    FIXME("iface %p, hwnd %p stub!\n", iface, hwnd);

    return E_NOTIMPL;
}

static const struct ITaskbarListVtbl taskbar_list_vtbl =
{
    /* IUnknown methods */
    taskbar_list_QueryInterface,
    taskbar_list_AddRef,
    taskbar_list_Release,
    /* ITaskbarList methods */
    taskbar_list_HrInit,
    taskbar_list_AddTab,
    taskbar_list_DeleteTab,
    taskbar_list_ActivateTab,
    taskbar_list_SetActiveAlt,
};

HRESULT TaskbarList_Create(IUnknown *outer, REFIID riid, void **taskbar_list)
{
    struct taskbar_list *object;

    TRACE("outer %p, riid %s, taskbar_list %p\n", outer, debugstr_guid(riid), taskbar_list);

    if (outer)
    {
        WARN("Aggregation not supported\n");
        return CLASS_E_NOAGGREGATION;
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate taskbar list object memory\n");
        return E_OUTOFMEMORY;
    }

    object->lpVtbl = &taskbar_list_vtbl;
    object->refcount = 1;

    *taskbar_list = object;

    TRACE("Created ITaskbarList %p\n", object);

    return S_OK;
}
