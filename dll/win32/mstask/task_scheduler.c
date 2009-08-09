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

#include "corerror.h"
#include "mstask_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mstask);

static void TaskSchedulerDestructor(TaskSchedulerImpl *This)
{
    TRACE("%p\n", This);
    HeapFree(GetProcessHeap(), 0, This);
    InterlockedDecrement(&dll_ref);
}

static HRESULT WINAPI MSTASK_ITaskScheduler_QueryInterface(
        ITaskScheduler* iface,
        REFIID riid,
        void **ppvObject)
{
    TaskSchedulerImpl * This = (TaskSchedulerImpl *)iface;

    TRACE("IID: %s\n", debugstr_guid(riid));

    if (IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_ITaskScheduler))
    {
        *ppvObject = &This->lpVtbl;
        ITaskScheduler_AddRef(iface);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI MSTASK_ITaskScheduler_AddRef(
        ITaskScheduler* iface)
{
    TaskSchedulerImpl *This = (TaskSchedulerImpl *)iface;
    TRACE("\n");
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI MSTASK_ITaskScheduler_Release(
        ITaskScheduler* iface)
{
    TaskSchedulerImpl * This = (TaskSchedulerImpl *)iface;
    ULONG ref;
    TRACE("\n");
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
        TaskSchedulerDestructor(This);
    return ref;
}

static HRESULT WINAPI MSTASK_ITaskScheduler_SetTargetComputer(
        ITaskScheduler* iface,
        LPCWSTR pwszComputer)
{
    FIXME("%p, %s: stub\n", iface, debugstr_w(pwszComputer));
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITaskScheduler_GetTargetComputer(
        ITaskScheduler* iface,
        LPWSTR *ppwszComputer)
{
    FIXME("%p, %p: stub\n", iface, ppwszComputer);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITaskScheduler_Enum(
        ITaskScheduler* iface,
        IEnumWorkItems **ppEnumTasks)
{
    FIXME("%p, %p: stub\n", iface, ppEnumTasks);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITaskScheduler_Activate(
        ITaskScheduler* iface,
        LPCWSTR pwszName,
        REFIID riid,
        IUnknown **ppunk)
{
    TRACE("%p, %s, %s, %p: stub\n", iface, debugstr_w(pwszName),
            debugstr_guid(riid), ppunk);
    FIXME("Partial stub always returning COR_E_FILENOTFOUND\n");
    return COR_E_FILENOTFOUND;
}

static HRESULT WINAPI MSTASK_ITaskScheduler_Delete(
        ITaskScheduler* iface,
        LPCWSTR pwszName)
{
    FIXME("%p, %s: stub\n", iface, debugstr_w(pwszName));
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITaskScheduler_NewWorkItem(
        ITaskScheduler* iface,
        LPCWSTR pwszTaskName,
        REFCLSID rclsid,
        REFIID riid,
        IUnknown **ppunk)
{
    HRESULT hr;
    TRACE("(%p, %s, %s, %s, %p)\n", iface, debugstr_w(pwszTaskName),
            debugstr_guid(rclsid) ,debugstr_guid(riid),  ppunk);

    if (!IsEqualGUID(rclsid, &CLSID_CTask))
        return CLASS_E_CLASSNOTAVAILABLE;

    if (!IsEqualGUID(riid, &IID_ITask))
        return E_NOINTERFACE;

    hr = TaskConstructor(pwszTaskName, (LPVOID *)ppunk);
    return hr;
}

static HRESULT WINAPI MSTASK_ITaskScheduler_AddWorkItem(
        ITaskScheduler* iface,
        LPCWSTR pwszTaskName,
        IScheduledWorkItem *pWorkItem)
{
    FIXME("%p, %s, %p: stub\n", iface, debugstr_w(pwszTaskName), pWorkItem);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITaskScheduler_IsOfType(
        ITaskScheduler* iface,
        LPCWSTR pwszName,
        REFIID riid)
{
    FIXME("%p, %s, %s: stub\n", iface, debugstr_w(pwszName),
            debugstr_guid(riid));
    return E_NOTIMPL;
}

static const ITaskSchedulerVtbl MSTASK_ITaskSchedulerVtbl =
{
    MSTASK_ITaskScheduler_QueryInterface,
    MSTASK_ITaskScheduler_AddRef,
    MSTASK_ITaskScheduler_Release,
    MSTASK_ITaskScheduler_SetTargetComputer,
    MSTASK_ITaskScheduler_GetTargetComputer,
    MSTASK_ITaskScheduler_Enum,
    MSTASK_ITaskScheduler_Activate,
    MSTASK_ITaskScheduler_Delete,
    MSTASK_ITaskScheduler_NewWorkItem,
    MSTASK_ITaskScheduler_AddWorkItem,
    MSTASK_ITaskScheduler_IsOfType
};

HRESULT TaskSchedulerConstructor(LPVOID *ppObj)
{
    TaskSchedulerImpl *This;
    TRACE("(%p)\n", ppObj);

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->lpVtbl = &MSTASK_ITaskSchedulerVtbl;
    This->ref = 1;

    *ppObj = &This->lpVtbl;
    InterlockedIncrement(&dll_ref);
    return S_OK;
}
