/*
 * Copyright (C) 2008 Google (Roy Shea)
 * Copyright (C) 2018 Dmitry Timoshkov
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
#include "initguid.h"
#include "objbase.h"
#include "taskschd.h"
#include "mstask.h"
#include "mstask_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mstask);

typedef struct
{
    ITaskScheduler ITaskScheduler_iface;
    LONG ref;
    ITaskService *service;
} TaskSchedulerImpl;

typedef struct
{
    IEnumWorkItems IEnumWorkItems_iface;
    LONG ref;
    HANDLE handle;
} EnumWorkItemsImpl;

static inline TaskSchedulerImpl *impl_from_ITaskScheduler(ITaskScheduler *iface)
{
    return CONTAINING_RECORD(iface, TaskSchedulerImpl, ITaskScheduler_iface);
}

static inline EnumWorkItemsImpl *impl_from_IEnumWorkItems(IEnumWorkItems *iface)
{
    return CONTAINING_RECORD(iface, EnumWorkItemsImpl, IEnumWorkItems_iface);
}

static void TaskSchedulerDestructor(TaskSchedulerImpl *This)
{
    TRACE("%p\n", This);
    ITaskService_Release(This->service);
    free(This);
    InterlockedDecrement(&dll_ref);
}

static HRESULT WINAPI EnumWorkItems_QueryInterface(IEnumWorkItems *iface, REFIID riid, void **obj)
{
    EnumWorkItemsImpl *This = impl_from_IEnumWorkItems(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IEnumWorkItems) || IsEqualGUID(riid, &IID_IUnknown))
    {
        *obj = &This->IEnumWorkItems_iface;
        IEnumWorkItems_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI EnumWorkItems_AddRef(IEnumWorkItems *iface)
{
    EnumWorkItemsImpl *This = impl_from_IEnumWorkItems(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%lu)\n", This, ref);
    return ref;
}

static ULONG WINAPI EnumWorkItems_Release(IEnumWorkItems *iface)
{
    EnumWorkItemsImpl *This = impl_from_IEnumWorkItems(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%lu)\n", This, ref);

    if (ref == 0)
    {
        if (This->handle != INVALID_HANDLE_VALUE)
            FindClose(This->handle);
        free(This);
        InterlockedDecrement(&dll_ref);
    }

    return ref;
}

static void free_list(LPWSTR *list, LONG count)
{
    LONG i;

    for (i = 0; i < count; i++)
        CoTaskMemFree(list[i]);

    CoTaskMemFree(list);
}

static inline BOOL is_file(const WIN32_FIND_DATAW *data)
{
    return !(data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

static HRESULT WINAPI EnumWorkItems_Next(IEnumWorkItems *iface, ULONG count, LPWSTR **names, ULONG *fetched)
{
    EnumWorkItemsImpl *This = impl_from_IEnumWorkItems(iface);
    WCHAR path[MAX_PATH];
    WIN32_FIND_DATAW data;
    ULONG enumerated, allocated, dummy;
    LPWSTR *list;
    HRESULT hr = S_FALSE;

    TRACE("(%p)->(%lu %p %p)\n", This, count, names, fetched);

    if (!count || !names || (!fetched && count > 1)) return E_INVALIDARG;

    if (!fetched) fetched = &dummy;

    *names = NULL;
    *fetched = 0;
    enumerated = 0;
    list = NULL;

    if (This->handle == INVALID_HANDLE_VALUE)
    {
        GetWindowsDirectoryW(path, MAX_PATH);
        lstrcatW(path, L"\\Tasks\\*");
        This->handle = FindFirstFileW(path, &data);
        if (This->handle == INVALID_HANDLE_VALUE)
            return S_FALSE;
    }
    else
    {
        if (!FindNextFileW(This->handle, &data))
            return S_FALSE;
    }

    allocated = 64;
    list = CoTaskMemAlloc(allocated * sizeof(list[0]));
    if (!list) return E_OUTOFMEMORY;

    do
    {
        if (is_file(&data))
        {
            if (enumerated >= allocated)
            {
                LPWSTR *new_list;
                allocated *= 2;
                new_list = CoTaskMemRealloc(list, allocated * sizeof(list[0]));
                if (!new_list)
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }
                list = new_list;
            }

            list[enumerated] = CoTaskMemAlloc((lstrlenW(data.cFileName) + 1) * sizeof(WCHAR));
            if (!list[enumerated])
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            lstrcpyW(list[enumerated], data.cFileName);
            enumerated++;

            if (enumerated >= count)
            {
                hr = S_OK;
                break;
            }
        }
    } while (FindNextFileW(This->handle, &data));

    if (FAILED(hr))
        free_list(list, enumerated);
    else
    {
        *fetched = enumerated;
        *names = list;
    }

    return hr;
}

static HRESULT WINAPI EnumWorkItems_Skip(IEnumWorkItems *iface, ULONG count)
{
    LPWSTR *names;
    ULONG fetched;
    HRESULT hr;

    TRACE("(%p)->(%lu)\n", iface, count);

    hr = EnumWorkItems_Next(iface, count, &names, &fetched);
    if (SUCCEEDED(hr))
        free_list(names, fetched);

    return hr;
}

static HRESULT WINAPI EnumWorkItems_Reset(IEnumWorkItems *iface)
{
    EnumWorkItemsImpl *This = impl_from_IEnumWorkItems(iface);

    TRACE("(%p)\n", This);

    if (This->handle != INVALID_HANDLE_VALUE)
    {
        FindClose(This->handle);
        This->handle = INVALID_HANDLE_VALUE;
    }

    return S_OK;
}

static HRESULT WINAPI EnumWorkItems_Clone(IEnumWorkItems *iface, IEnumWorkItems **cloned)
{
    EnumWorkItemsImpl *This = impl_from_IEnumWorkItems(iface);
    FIXME("(%p)->(%p): stub\n", This, cloned);
    return E_NOTIMPL;
}

static const IEnumWorkItemsVtbl EnumWorkItemsVtbl = {
    EnumWorkItems_QueryInterface,
    EnumWorkItems_AddRef,
    EnumWorkItems_Release,
    EnumWorkItems_Next,
    EnumWorkItems_Skip,
    EnumWorkItems_Reset,
    EnumWorkItems_Clone
};

static HRESULT create_task_enum(IEnumWorkItems **ret)
{
    EnumWorkItemsImpl *tasks;

    *ret = NULL;

    tasks = malloc(sizeof(*tasks));
    if (!tasks)
        return E_OUTOFMEMORY;

    tasks->IEnumWorkItems_iface.lpVtbl = &EnumWorkItemsVtbl;
    tasks->ref = 1;
    tasks->handle = INVALID_HANDLE_VALUE;

    *ret = &tasks->IEnumWorkItems_iface;
    InterlockedIncrement(&dll_ref);
    return S_OK;
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
        ITaskScheduler *iface, LPCWSTR comp_name)
{
    TaskSchedulerImpl *This = impl_from_ITaskScheduler(iface);
    VARIANT v_null, v_comp;
    HRESULT hr;

    TRACE("(%p)->(%s)\n", This, debugstr_w(comp_name));

    V_VT(&v_null) = VT_NULL;
    V_VT(&v_comp) = VT_BSTR;
    V_BSTR(&v_comp) = SysAllocString(comp_name);
    hr = ITaskService_Connect(This->service, v_comp, v_null, v_null, v_null);
    SysFreeString(V_BSTR(&v_comp));
    return hr;
}

static HRESULT WINAPI MSTASK_ITaskScheduler_GetTargetComputer(
        ITaskScheduler *iface, LPWSTR *comp_name)
{
    TaskSchedulerImpl *This = impl_from_ITaskScheduler(iface);
    BSTR bstr;
    WCHAR *buffer;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, comp_name);

    if (!comp_name)
        return E_INVALIDARG;

    hr = ITaskService_get_TargetServer(This->service, &bstr);
    if (hr != S_OK) return hr;

    /* extra space for two '\' and a zero */
    buffer = CoTaskMemAlloc((SysStringLen(bstr) + 3) * sizeof(WCHAR));
    if (buffer)
    {
        buffer[0] = '\\';
        buffer[1] = '\\';
        lstrcpyW(buffer + 2, bstr);
        *comp_name = buffer;
        hr = S_OK;
    }
    else
    {
        *comp_name = NULL;
        hr = E_OUTOFMEMORY;
    }

    SysFreeString(bstr);
    return hr;
}

static HRESULT WINAPI MSTASK_ITaskScheduler_Enum(
        ITaskScheduler* iface,
        IEnumWorkItems **tasks)
{
    TaskSchedulerImpl *This = impl_from_ITaskScheduler(iface);

    TRACE("(%p)->(%p)\n", This, tasks);

    if (!tasks)
        return E_INVALIDARG;

    return create_task_enum(tasks);
}

static HRESULT WINAPI MSTASK_ITaskScheduler_Activate(ITaskScheduler *iface,
        LPCWSTR task_name, REFIID riid, IUnknown **unknown)
{
    ITask *task;
    IPersistFile *pfile;
    HRESULT hr;

    TRACE("%p, %s, %s, %p\n", iface, debugstr_w(task_name), debugstr_guid(riid), unknown);

    hr = ITaskScheduler_NewWorkItem(iface, task_name, &CLSID_CTask, riid, (IUnknown **)&task);
    if (hr != S_OK) return hr;

    hr = ITask_QueryInterface(task, &IID_IPersistFile, (void **)&pfile);
    if (hr == S_OK)
    {
        WCHAR *curfile;

        hr = IPersistFile_GetCurFile(pfile, &curfile);
        if (hr == S_OK)
        {
            hr = IPersistFile_Load(pfile, curfile, STGM_READ | STGM_SHARE_DENY_WRITE);
            CoTaskMemFree(curfile);
        }

        IPersistFile_Release(pfile);
    }

    if (hr == S_OK)
        *unknown = (IUnknown *)task;
    else
        ITask_Release(task);
    return hr;
}

static HRESULT WINAPI MSTASK_ITaskScheduler_Delete(ITaskScheduler *iface, LPCWSTR name)
{
    WCHAR task_name[MAX_PATH];

    TRACE("%p, %s\n", iface, debugstr_w(name));

    if (wcschr(name, '.')) return E_INVALIDARG;

    GetWindowsDirectoryW(task_name, MAX_PATH);
    lstrcatW(task_name, L"\\Tasks\\");
    lstrcatW(task_name, name);
    lstrcatW(task_name, L".job");

    if (!DeleteFileW(task_name))
        return HRESULT_FROM_WIN32(GetLastError());

    return S_OK;
}

static HRESULT WINAPI MSTASK_ITaskScheduler_NewWorkItem(
        ITaskScheduler* iface,
        LPCWSTR task_name,
        REFCLSID rclsid,
        REFIID riid,
        IUnknown **task)
{
    TaskSchedulerImpl *This = impl_from_ITaskScheduler(iface);

    TRACE("(%p, %s, %s, %s, %p)\n", iface, debugstr_w(task_name),
            debugstr_guid(rclsid), debugstr_guid(riid), task);

    if (!IsEqualGUID(rclsid, &CLSID_CTask))
        return CLASS_E_CLASSNOTAVAILABLE;

    if (!IsEqualGUID(riid, &IID_ITask))
        return E_NOINTERFACE;

    return TaskConstructor(This->service, task_name, (ITask **)task);
}

static HRESULT WINAPI MSTASK_ITaskScheduler_AddWorkItem(ITaskScheduler *iface, LPCWSTR name, IScheduledWorkItem *item)
{
    WCHAR task_name[MAX_PATH];
    IPersistFile *pfile;
    HRESULT hr;

    TRACE("%p, %s, %p\n", iface, debugstr_w(name), item);

    if (wcschr(name, '.')) return E_INVALIDARG;

    GetWindowsDirectoryW(task_name, MAX_PATH);
    lstrcatW(task_name, L"\\Tasks\\");
    lstrcatW(task_name, name);
    lstrcatW(task_name, L".job");

    hr = IScheduledWorkItem_QueryInterface(item, &IID_IPersistFile, (void **)&pfile);
    if (hr == S_OK)
    {
        hr = IPersistFile_Save(pfile, task_name, TRUE);
        IPersistFile_Release(pfile);
    }
    return hr;
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
    ITaskService *service;
    VARIANT v_null;
    HRESULT hr;

    TRACE("(%p)\n", ppObj);

    hr = CoCreateInstance(&CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, &IID_ITaskService, (void **)&service);
    if (hr != S_OK) return hr;

    V_VT(&v_null) = VT_NULL;
    hr = ITaskService_Connect(service, v_null, v_null, v_null, v_null);
    if (hr != S_OK)
    {
        ITaskService_Release(service);
        return hr;
    }

    This = malloc(sizeof(*This));
    if (!This)
    {
        ITaskService_Release(service);
        return E_OUTOFMEMORY;
    }

    This->ITaskScheduler_iface.lpVtbl = &MSTASK_ITaskSchedulerVtbl;
    This->service = service;
    This->ref = 1;

    *ppObj = &This->ITaskScheduler_iface;
    InterlockedIncrement(&dll_ref);
    return S_OK;
}
