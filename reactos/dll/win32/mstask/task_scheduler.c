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

#include "mstask_private.h"

#include <corerror.h>

typedef struct
{
    ITaskScheduler ITaskScheduler_iface;
    LONG ref;
} TaskSchedulerImpl;

static inline TaskSchedulerImpl *impl_from_ITaskScheduler(ITaskScheduler *iface)
{
    return CONTAINING_RECORD(iface, TaskSchedulerImpl, ITaskScheduler_iface);
}

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
    TaskSchedulerImpl * This = impl_from_ITaskScheduler(iface);

    TRACE("IID: %s\n", debugstr_guid(riid));

    if (IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_ITaskScheduler))
    {
        *ppvObject = &This->ITaskScheduler_iface;
        ITaskScheduler_AddRef(iface);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI MSTASK_ITaskScheduler_AddRef(
        ITaskScheduler* iface)
{
    TaskSchedulerImpl *This = impl_from_ITaskScheduler(iface);
    TRACE("\n");
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI MSTASK_ITaskScheduler_Release(
        ITaskScheduler* iface)
{
    TaskSchedulerImpl * This = impl_from_ITaskScheduler(iface);
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
    TaskSchedulerImpl *This = impl_from_ITaskScheduler(iface);
    WCHAR buffer[MAX_COMPUTERNAME_LENGTH + 3];  /* extra space for two '\' and a zero */
    DWORD len = MAX_COMPUTERNAME_LENGTH + 1;    /* extra space for a zero */

    TRACE("(%p)->(%s)\n", This, debugstr_w(pwszComputer));

    /* NULL is an alias for the local computer */
    if (!pwszComputer)
        return S_OK;

    buffer[0] = '\\';
    buffer[1] = '\\';
    if (GetComputerNameW(buffer + 2, &len))
    {
        if (!lstrcmpiW(buffer, pwszComputer) ||    /* full unc name */
            !lstrcmpiW(buffer + 2, pwszComputer))  /* name without backslash */
            return S_OK;
    }

    FIXME("remote computer %s not supported\n", debugstr_w(pwszComputer));
    return HRESULT_FROM_WIN32(ERROR_BAD_NETPATH);
}

static HRESULT WINAPI MSTASK_ITaskScheduler_GetTargetComputer(
        ITaskScheduler* iface,
        LPWSTR *ppwszComputer)
{
    TaskSchedulerImpl *This = impl_from_ITaskScheduler(iface);
    LPWSTR buffer;
    DWORD len = MAX_COMPUTERNAME_LENGTH + 1; /* extra space for the zero */

    TRACE("(%p)->(%p)\n", This, ppwszComputer);

    if (!ppwszComputer)
        return E_INVALIDARG;

    /* extra space for two '\' and a zero */
    buffer = CoTaskMemAlloc((MAX_COMPUTERNAME_LENGTH + 3) * sizeof(WCHAR));
    if (buffer)
    {
        buffer[0] = '\\';
        buffer[1] = '\\';
        if (GetComputerNameW(buffer + 2, &len))
        {
            *ppwszComputer = buffer;
            return S_OK;
        }
        CoTaskMemFree(buffer);
    }
    *ppwszComputer = NULL;
    return HRESULT_FROM_WIN32(GetLastError());
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

    This->ITaskScheduler_iface.lpVtbl = &MSTASK_ITaskSchedulerVtbl;
    This->ref = 1;

    *ppObj = &This->ITaskScheduler_iface;
    InterlockedIncrement(&dll_ref);
    return S_OK;
}
