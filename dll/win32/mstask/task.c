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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mstask);

static inline TaskImpl *impl_from_IPersistFile( IPersistFile *iface )
{
    return (TaskImpl*) ((char*)(iface) - FIELD_OFFSET(TaskImpl, persistVtbl));
}

static void TaskDestructor(TaskImpl *This)
{
    TRACE("%p\n", This);
    HeapFree(GetProcessHeap(), 0, This->accountName);
    HeapFree(GetProcessHeap(), 0, This->comment);
    HeapFree(GetProcessHeap(), 0, This->parameters);
    HeapFree(GetProcessHeap(), 0, This->taskName);
    HeapFree(GetProcessHeap(), 0, This);
    InterlockedDecrement(&dll_ref);
}

static HRESULT WINAPI MSTASK_ITask_QueryInterface(
        ITask* iface,
        REFIID riid,
        void **ppvObject)
{
    TaskImpl * This = (TaskImpl *)iface;

    TRACE("IID: %s\n", debugstr_guid(riid));
    if (ppvObject == NULL)
        return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_ITask))
    {
        *ppvObject = &This->lpVtbl;
        ITask_AddRef(iface);
        return S_OK;
    }
    else if (IsEqualGUID(riid, &IID_IPersistFile))
    {
        *ppvObject = &This->persistVtbl;
        ITask_AddRef(iface);
        return S_OK;
    }

    WARN("Unknown interface: %s\n", debugstr_guid(riid));
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI MSTASK_ITask_AddRef(
        ITask* iface)
{
    TaskImpl *This = (TaskImpl *)iface;
    ULONG ref;
    TRACE("\n");
    ref = InterlockedIncrement(&This->ref);
    return ref;
}

static ULONG WINAPI MSTASK_ITask_Release(
        ITask* iface)
{
    TaskImpl * This = (TaskImpl *)iface;
    ULONG ref;
    TRACE("\n");
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
        TaskDestructor(This);
    return ref;
}

static HRESULT WINAPI MSTASK_ITask_CreateTrigger(
        ITask* iface,
        WORD *piNewTrigger,
        ITaskTrigger **ppTrigger)
{
    TRACE("(%p, %p, %p)\n", iface, piNewTrigger, ppTrigger);
    return TaskTriggerConstructor((LPVOID *)ppTrigger);
}

static HRESULT WINAPI MSTASK_ITask_DeleteTrigger(
        ITask* iface,
        WORD iTrigger)
{
    FIXME("(%p, %d): stub\n", iface, iTrigger);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetTriggerCount(
        ITask* iface,
        WORD *plCount)
{
    FIXME("(%p, %p): stub\n", iface, plCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetTrigger(
        ITask* iface,
        WORD iTrigger,
        ITaskTrigger **ppTrigger)
{
    FIXME("(%p, %d, %p): stub\n", iface, iTrigger, ppTrigger);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetTriggerString(
        ITask* iface,
        WORD iTrigger,
        LPWSTR *ppwszTrigger)
{
    FIXME("(%p, %d, %p): stub\n", iface, iTrigger, ppwszTrigger);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetRunTimes(
        ITask* iface,
        const LPSYSTEMTIME pstBegin,
        const LPSYSTEMTIME pstEnd,
        WORD *pCount,
        LPSYSTEMTIME *rgstTaskTimes)
{
    FIXME("(%p, %p, %p, %p, %p): stub\n", iface, pstBegin, pstEnd, pCount,
            rgstTaskTimes);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetNextRunTime(
        ITask* iface,
        SYSTEMTIME *pstNextRun)
{
    FIXME("(%p, %p): stub\n", iface, pstNextRun);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_SetIdleWait(
        ITask* iface,
        WORD wIdleMinutes,
        WORD wDeadlineMinutes)
{
    FIXME("(%p, %d, %d): stub\n", iface, wIdleMinutes, wDeadlineMinutes);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetIdleWait(
        ITask* iface,
        WORD *pwIdleMinutes,
        WORD *pwDeadlineMinutes)
{
    FIXME("(%p, %p, %p): stub\n", iface, pwIdleMinutes, pwDeadlineMinutes);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_Run(
        ITask* iface)
{
    FIXME("(%p): stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_Terminate(
        ITask* iface)
{
    FIXME("(%p): stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_EditWorkItem(
        ITask* iface,
        HWND hParent,
        DWORD dwReserved)
{
    FIXME("(%p, %p, %d): stub\n", iface, hParent, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetMostRecentRunTime(
        ITask* iface,
        SYSTEMTIME *pstLastRun)
{
    FIXME("(%p, %p): stub\n", iface, pstLastRun);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetStatus(
        ITask* iface,
        HRESULT *phrStatus)
{
    FIXME("(%p, %p): stub\n", iface, phrStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetExitCode(
        ITask* iface,
        DWORD *pdwExitCode)
{
    FIXME("(%p, %p): stub\n", iface, pdwExitCode);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_SetComment(
        ITask* iface,
        LPCWSTR pwszComment)
{
    DWORD n;
    TaskImpl *This = (TaskImpl *)iface;
    LPWSTR tmp_comment;

    TRACE("(%p, %s)\n", iface, debugstr_w(pwszComment));

    /* Empty comment */
    if (pwszComment[0] == 0)
    {
        HeapFree(GetProcessHeap(), 0, This->comment);
        This->comment = NULL;
        return S_OK;
    }

    /* Set to pwszComment */
    n = (lstrlenW(pwszComment) + 1);
    tmp_comment = HeapAlloc(GetProcessHeap(), 0, n * sizeof(WCHAR));
    if (!tmp_comment)
        return E_OUTOFMEMORY;
    lstrcpyW(tmp_comment, pwszComment);
    HeapFree(GetProcessHeap(), 0, This->comment);
    This->comment = tmp_comment;

    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_GetComment(
        ITask* iface,
        LPWSTR *ppwszComment)
{
    DWORD n;
    TaskImpl *This = (TaskImpl *)iface;

    TRACE("(%p, %p)\n", iface, ppwszComment);

    n = This->comment ? lstrlenW(This->comment) + 1 : 1;
    *ppwszComment = CoTaskMemAlloc(n * sizeof(WCHAR));
    if (!*ppwszComment)
        return E_OUTOFMEMORY;

    if (!This->comment)
        *ppwszComment[0] = 0;
    else
        lstrcpyW(*ppwszComment, This->comment);

    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_SetCreator(
        ITask* iface,
        LPCWSTR pwszCreator)
{
    FIXME("(%p, %p): stub\n", iface, pwszCreator);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetCreator(
        ITask* iface,
        LPWSTR *ppwszCreator)
{
    FIXME("(%p, %p): stub\n", iface, ppwszCreator);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_SetWorkItemData(
        ITask* iface,
        WORD cBytes,
        BYTE rgbData[])
{
    FIXME("(%p, %d, %p): stub\n", iface, cBytes, rgbData);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetWorkItemData(
        ITask* iface,
        WORD *pcBytes,
        BYTE **ppBytes)
{
    FIXME("(%p, %p, %p): stub\n", iface, pcBytes, ppBytes);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_SetErrorRetryCount(
        ITask* iface,
        WORD wRetryCount)
{
    FIXME("(%p, %d): stub\n", iface, wRetryCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetErrorRetryCount(
        ITask* iface,
        WORD *pwRetryCount)
{
    FIXME("(%p, %p): stub\n", iface, pwRetryCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_SetErrorRetryInterval(
        ITask* iface,
        WORD wRetryInterval)
{
    FIXME("(%p, %d): stub\n", iface, wRetryInterval);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetErrorRetryInterval(
        ITask* iface,
        WORD *pwRetryInterval)
{
    FIXME("(%p, %p): stub\n", iface, pwRetryInterval);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_SetFlags(
        ITask* iface,
        DWORD dwFlags)
{
    FIXME("(%p, 0x%08x): stub\n", iface, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetFlags(
        ITask* iface,
        DWORD *pdwFlags)
{
    FIXME("(%p, %p): stub\n", iface, pdwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_SetAccountInformation(
        ITask* iface,
        LPCWSTR pwszAccountName,
        LPCWSTR pwszPassword)
{
    DWORD n;
    TaskImpl *This = (TaskImpl *)iface;
    LPWSTR tmp_account_name;

    TRACE("(%p, %s, %s): partial stub\n", iface, debugstr_w(pwszAccountName),
            debugstr_w(pwszPassword));

    if (pwszPassword)
        FIXME("Partial stub ignores passwords\n");

    n = (lstrlenW(pwszAccountName) + 1);
    tmp_account_name = HeapAlloc(GetProcessHeap(), 0, n * sizeof(WCHAR));
    if (!tmp_account_name)
        return E_OUTOFMEMORY;
    lstrcpyW(tmp_account_name, pwszAccountName);
    HeapFree(GetProcessHeap(), 0, This->accountName);
    This->accountName = tmp_account_name;
    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_GetAccountInformation(
        ITask* iface,
        LPWSTR *ppwszAccountName)
{
    DWORD n;
    TaskImpl *This = (TaskImpl *)iface;

    TRACE("(%p, %p): partial stub\n", iface, ppwszAccountName);

    /* This implements the WinXP behavior when accountName has not yet
     * set.  Win2K behaves differently, returning SCHED_E_CANNOT_OPEN_TASK */
    if (!This->accountName)
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    n = (lstrlenW(This->accountName) + 1);
    *ppwszAccountName = CoTaskMemAlloc(n * sizeof(WCHAR));
    if (!*ppwszAccountName)
        return E_OUTOFMEMORY;
    lstrcpyW(*ppwszAccountName, This->accountName);
    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_SetApplicationName(
        ITask* iface,
        LPCWSTR pwszApplicationName)
{
    DWORD n;
    TaskImpl *This = (TaskImpl *)iface;
    LPWSTR tmp_name;

    TRACE("(%p, %s)\n", iface, debugstr_w(pwszApplicationName));

    /* Empty application name */
    if (pwszApplicationName[0] == 0)
    {
        HeapFree(GetProcessHeap(), 0, This->applicationName);
        This->applicationName = NULL;
        return S_OK;
    }

    /* Attempt to set pwszApplicationName to a path resolved application name */
    n = SearchPathW(NULL, pwszApplicationName, NULL, 0, NULL, NULL);
    if (n)
    {
        tmp_name = HeapAlloc(GetProcessHeap(), 0, n * sizeof(WCHAR));
        if (!tmp_name)
            return E_OUTOFMEMORY;
        n = SearchPathW(NULL, pwszApplicationName, NULL, n, tmp_name, NULL);
        if (n)
        {
            HeapFree(GetProcessHeap(), 0, This->applicationName);
            This->applicationName = tmp_name;
            return S_OK;
        }
        else
            HeapFree(GetProcessHeap(), 0, tmp_name);
    }

    /* If unable to path resolve name, simply set to pwszApplicationName */
    n = (lstrlenW(pwszApplicationName) + 1);
    tmp_name = HeapAlloc(GetProcessHeap(), 0, n * sizeof(WCHAR));
    if (!tmp_name)
        return E_OUTOFMEMORY;
    lstrcpyW(tmp_name, pwszApplicationName);
    HeapFree(GetProcessHeap(), 0, This->applicationName);
    This->applicationName = tmp_name;
    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_GetApplicationName(
        ITask* iface,
        LPWSTR *ppwszApplicationName)
{
    DWORD n;
    TaskImpl *This = (TaskImpl *)iface;

    TRACE("(%p, %p)\n", iface, ppwszApplicationName);

    n = This->applicationName ? lstrlenW(This->applicationName) + 1 : 1;
    *ppwszApplicationName = CoTaskMemAlloc(n * sizeof(WCHAR));
    if (!*ppwszApplicationName)
        return E_OUTOFMEMORY;

    if (!This->applicationName)
        *ppwszApplicationName[0] = 0;
    else
        lstrcpyW(*ppwszApplicationName, This->applicationName);

    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_SetParameters(
        ITask* iface,
        LPCWSTR pwszParameters)
{
    DWORD n;
    TaskImpl *This = (TaskImpl *)iface;
    LPWSTR tmp_parameters;

    TRACE("(%p, %s)\n", iface, debugstr_w(pwszParameters));

    /* Empty parameter list */
    if (pwszParameters[0] == 0)
    {
        HeapFree(GetProcessHeap(), 0, This->parameters);
        This->parameters = NULL;
        return S_OK;
    }

    /* Set to pwszParameters */
    n = (lstrlenW(pwszParameters) + 1);
    tmp_parameters = HeapAlloc(GetProcessHeap(), 0, n * sizeof(WCHAR));
    if (!tmp_parameters)
        return E_OUTOFMEMORY;
    lstrcpyW(tmp_parameters, pwszParameters);
    HeapFree(GetProcessHeap(), 0, This->parameters);
    This->parameters = tmp_parameters;
    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_GetParameters(
        ITask* iface,
        LPWSTR *ppwszParameters)
{
    DWORD n;
    TaskImpl *This = (TaskImpl *)iface;

    TRACE("(%p, %p)\n", iface, ppwszParameters);

    n = This->parameters ? lstrlenW(This->parameters) + 1 : 1;
    *ppwszParameters = CoTaskMemAlloc(n * sizeof(WCHAR));
    if (!*ppwszParameters)
        return E_OUTOFMEMORY;

    if (!This->parameters)
        *ppwszParameters[0] = 0;
    else
        lstrcpyW(*ppwszParameters, This->parameters);

    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_SetWorkingDirectory(
        ITask* iface,
        LPCWSTR pwszWorkingDirectory)
{
    FIXME("(%p, %s): stub\n", iface, debugstr_w(pwszWorkingDirectory));
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetWorkingDirectory(
        ITask* iface,
        LPWSTR *ppwszWorkingDirectory)
{
    FIXME("(%p, %p): stub\n", iface, ppwszWorkingDirectory);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_SetPriority(
        ITask* iface,
        DWORD dwPriority)
{
    FIXME("(%p, 0x%08x): stub\n", iface, dwPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetPriority(
        ITask* iface,
        DWORD *pdwPriority)
{
    FIXME("(%p, %p): stub\n", iface, pdwPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_SetTaskFlags(
        ITask* iface,
        DWORD dwFlags)
{
    FIXME("(%p, 0x%08x): stub\n", iface, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetTaskFlags(
        ITask* iface,
        DWORD *pdwFlags)
{
    FIXME("(%p, %p): stub\n", iface, pdwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_SetMaxRunTime(
        ITask* iface,
        DWORD dwMaxRunTime)
{
    TaskImpl *This = (TaskImpl *)iface;

    TRACE("(%p, %d)\n", iface, dwMaxRunTime);

    This->maxRunTime = dwMaxRunTime;
    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_GetMaxRunTime(
        ITask* iface,
        DWORD *pdwMaxRunTime)
{
    TaskImpl *This = (TaskImpl *)iface;

    TRACE("(%p, %p)\n", iface, pdwMaxRunTime);

    *pdwMaxRunTime = This->maxRunTime;
    return S_OK;
}

static HRESULT WINAPI MSTASK_IPersistFile_QueryInterface(
        IPersistFile* iface,
        REFIID riid,
        void **ppvObject)
{
    TaskImpl *This = impl_from_IPersistFile(iface);
    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(riid), ppvObject);
    return ITask_QueryInterface((ITask *) This, riid, ppvObject);
}

static ULONG WINAPI MSTASK_IPersistFile_AddRef(
        IPersistFile* iface)
{
    TaskImpl *This = impl_from_IPersistFile(iface);
    ULONG ref;
    TRACE("\n");
    ref = InterlockedIncrement(&This->ref);
    return ref;
}

static ULONG WINAPI MSTASK_IPersistFile_Release(
        IPersistFile* iface)
{
    TaskImpl *This = impl_from_IPersistFile(iface);
    ULONG ref;
    TRACE("\n");
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
        TaskDestructor(This);
    return ref;
}

static HRESULT WINAPI MSTASK_IPersistFile_GetClassID(
        IPersistFile* iface,
        CLSID *pClassID)
{
    FIXME("(%p, %p): stub\n", iface, pClassID);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_IPersistFile_IsDirty(
        IPersistFile* iface)
{
    FIXME("(%p): stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_IPersistFile_Load(
        IPersistFile* iface,
        LPCOLESTR pszFileName,
        DWORD dwMode)
{
    FIXME("(%p, %p, 0x%08x): stub\n", iface, pszFileName, dwMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_IPersistFile_Save(
        IPersistFile* iface,
        LPCOLESTR pszFileName,
        BOOL fRemember)
{
    FIXME("(%p, %p, %d): stub\n", iface, pszFileName, fRemember);
    WARN("Returning S_OK but not writing to disk: %s %d\n",
            debugstr_w(pszFileName), fRemember);
    return S_OK;
}

static HRESULT WINAPI MSTASK_IPersistFile_SaveCompleted(
        IPersistFile* iface,
        LPCOLESTR pszFileName)
{
    FIXME("(%p, %p): stub\n", iface, pszFileName);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_IPersistFile_GetCurFile(
        IPersistFile* iface,
        LPOLESTR *ppszFileName)
{
    FIXME("(%p, %p): stub\n", iface, ppszFileName);
    return E_NOTIMPL;
}


static const ITaskVtbl MSTASK_ITaskVtbl =
{
    MSTASK_ITask_QueryInterface,
    MSTASK_ITask_AddRef,
    MSTASK_ITask_Release,
    MSTASK_ITask_CreateTrigger,
    MSTASK_ITask_DeleteTrigger,
    MSTASK_ITask_GetTriggerCount,
    MSTASK_ITask_GetTrigger,
    MSTASK_ITask_GetTriggerString,
    MSTASK_ITask_GetRunTimes,
    MSTASK_ITask_GetNextRunTime,
    MSTASK_ITask_SetIdleWait,
    MSTASK_ITask_GetIdleWait,
    MSTASK_ITask_Run,
    MSTASK_ITask_Terminate,
    MSTASK_ITask_EditWorkItem,
    MSTASK_ITask_GetMostRecentRunTime,
    MSTASK_ITask_GetStatus,
    MSTASK_ITask_GetExitCode,
    MSTASK_ITask_SetComment,
    MSTASK_ITask_GetComment,
    MSTASK_ITask_SetCreator,
    MSTASK_ITask_GetCreator,
    MSTASK_ITask_SetWorkItemData,
    MSTASK_ITask_GetWorkItemData,
    MSTASK_ITask_SetErrorRetryCount,
    MSTASK_ITask_GetErrorRetryCount,
    MSTASK_ITask_SetErrorRetryInterval,
    MSTASK_ITask_GetErrorRetryInterval,
    MSTASK_ITask_SetFlags,
    MSTASK_ITask_GetFlags,
    MSTASK_ITask_SetAccountInformation,
    MSTASK_ITask_GetAccountInformation,
    MSTASK_ITask_SetApplicationName,
    MSTASK_ITask_GetApplicationName,
    MSTASK_ITask_SetParameters,
    MSTASK_ITask_GetParameters,
    MSTASK_ITask_SetWorkingDirectory,
    MSTASK_ITask_GetWorkingDirectory,
    MSTASK_ITask_SetPriority,
    MSTASK_ITask_GetPriority,
    MSTASK_ITask_SetTaskFlags,
    MSTASK_ITask_GetTaskFlags,
    MSTASK_ITask_SetMaxRunTime,
    MSTASK_ITask_GetMaxRunTime
};

static const IPersistFileVtbl MSTASK_IPersistFileVtbl =
{
    MSTASK_IPersistFile_QueryInterface,
    MSTASK_IPersistFile_AddRef,
    MSTASK_IPersistFile_Release,
    MSTASK_IPersistFile_GetClassID,
    MSTASK_IPersistFile_IsDirty,
    MSTASK_IPersistFile_Load,
    MSTASK_IPersistFile_Save,
    MSTASK_IPersistFile_SaveCompleted,
    MSTASK_IPersistFile_GetCurFile
};

HRESULT TaskConstructor(LPCWSTR pwszTaskName, LPVOID *ppObj)
{
    TaskImpl *This;
    int n;

    TRACE("(%s, %p)\n", debugstr_w(pwszTaskName), ppObj);

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->lpVtbl = &MSTASK_ITaskVtbl;
    This->persistVtbl = &MSTASK_IPersistFileVtbl;
    This->ref = 1;
    n = (lstrlenW(pwszTaskName) + 1) * sizeof(WCHAR);
    This->taskName = HeapAlloc(GetProcessHeap(), 0, n);
    if (!This->taskName)
    {
        HeapFree(GetProcessHeap(), 0, This);
        return E_OUTOFMEMORY;
    }
    lstrcpyW(This->taskName, pwszTaskName);
    This->applicationName = NULL;
    This->parameters = NULL;
    This->comment = NULL;
    This->accountName = NULL;

    /* Default time is 3 days = 259200000 ms */
    This->maxRunTime = 259200000;

    *ppObj = &This->lpVtbl;
    InterlockedIncrement(&dll_ref);
    return S_OK;
}
