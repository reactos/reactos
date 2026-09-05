/*
 * Copyright (C) 2008 Google (Roy Shea)
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "taskschd.h"
#include "mstask.h"
#include "mstask_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mstask);

typedef struct
{
    ITaskTrigger ITaskTrigger_iface;
    LONG ref;
    ITask *parent_task;
    WORD trigger_index;
} TaskTriggerImpl;

static inline TaskTriggerImpl *impl_from_ITaskTrigger(ITaskTrigger *iface)
{
    return CONTAINING_RECORD(iface, TaskTriggerImpl, ITaskTrigger_iface);
}

static HRESULT WINAPI MSTASK_ITaskTrigger_QueryInterface(
        ITaskTrigger* iface,
        REFIID riid,
        void **ppvObject)
{
    TaskTriggerImpl *This = impl_from_ITaskTrigger(iface);

    TRACE("IID: %s\n", debugstr_guid(riid));
    if (ppvObject == NULL)
        return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_ITaskTrigger))
    {
        *ppvObject = &This->ITaskTrigger_iface;
        ITaskTrigger_AddRef(iface);
        return S_OK;
    }

    WARN("Unknown interface: %s\n", debugstr_guid(riid));
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI MSTASK_ITaskTrigger_AddRef(
        ITaskTrigger* iface)
{
    TaskTriggerImpl *This = impl_from_ITaskTrigger(iface);
    ULONG ref;
    TRACE("\n");
    ref = InterlockedIncrement(&This->ref);
    return ref;
}

static ULONG WINAPI MSTASK_ITaskTrigger_Release(
        ITaskTrigger* iface)
{
    TaskTriggerImpl *This = impl_from_ITaskTrigger(iface);
    ULONG ref;
    TRACE("\n");
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
    {
        ITask_Release(This->parent_task);
        free(This);
        InterlockedDecrement(&dll_ref);
    }
    return ref;
}

static HRESULT WINAPI MSTASK_ITaskTrigger_SetTrigger(ITaskTrigger *iface, const PTASK_TRIGGER trigger)
{
    TaskTriggerImpl *This = impl_from_ITaskTrigger(iface);

    TRACE("(%p, %p)\n", iface, trigger);

    if (!trigger) return E_INVALIDARG;

    return task_set_trigger(This->parent_task, This->trigger_index, trigger);
}

static HRESULT WINAPI MSTASK_ITaskTrigger_GetTrigger(ITaskTrigger *iface, TASK_TRIGGER *trigger)
{
    TaskTriggerImpl *This = impl_from_ITaskTrigger(iface);

    TRACE("(%p, %p)\n", iface, trigger);

    if (!trigger) return E_INVALIDARG;

    return task_get_trigger(This->parent_task, This->trigger_index, trigger);
}

static HRESULT WINAPI MSTASK_ITaskTrigger_GetTriggerString(
        ITaskTrigger* iface,
        LPWSTR *ppwszTrigger)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static const ITaskTriggerVtbl MSTASK_ITaskTriggerVtbl =
{
    MSTASK_ITaskTrigger_QueryInterface,
    MSTASK_ITaskTrigger_AddRef,
    MSTASK_ITaskTrigger_Release,
    MSTASK_ITaskTrigger_SetTrigger,
    MSTASK_ITaskTrigger_GetTrigger,
    MSTASK_ITaskTrigger_GetTriggerString
};

HRESULT TaskTriggerConstructor(ITask *task, WORD idx, ITaskTrigger **trigger)
{
    TaskTriggerImpl *This;

    TRACE("(%p, %u, %p)\n", task, idx, trigger);

    This = malloc(sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->ITaskTrigger_iface.lpVtbl = &MSTASK_ITaskTriggerVtbl;
    This->ref = 1;

    ITask_AddRef(task);
    This->parent_task = task;
    This->trigger_index = idx;

    *trigger = &This->ITaskTrigger_iface;
    InterlockedIncrement(&dll_ref);
    return S_OK;
}
