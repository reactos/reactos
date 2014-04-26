/*
 * Background Copy Job Interface for BITS
 *
 * Copyright 2007 Google (Roy Shea)
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

#include "qmgr.h"

static inline BOOL is_job_done(const BackgroundCopyJobImpl *job)
{
    return job->state == BG_JOB_STATE_CANCELLED || job->state == BG_JOB_STATE_ACKNOWLEDGED;
}

static inline BackgroundCopyJobImpl *impl_from_IBackgroundCopyJob2(IBackgroundCopyJob2 *iface)
{
    return CONTAINING_RECORD(iface, BackgroundCopyJobImpl, IBackgroundCopyJob2_iface);
}

static HRESULT WINAPI BackgroundCopyJob_QueryInterface(
    IBackgroundCopyJob2 *iface, REFIID riid, void **obj)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IBackgroundCopyJob)
        || IsEqualGUID(riid, &IID_IBackgroundCopyJob2))
    {
        *obj = iface;
        IBackgroundCopyJob2_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI BackgroundCopyJob_AddRef(IBackgroundCopyJob2 *iface)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI BackgroundCopyJob_Release(IBackgroundCopyJob2 *iface)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (ref == 0)
    {
        This->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->cs);
        if (This->callback)
            IBackgroundCopyCallback2_Release(This->callback);
        HeapFree(GetProcessHeap(), 0, This->displayName);
        HeapFree(GetProcessHeap(), 0, This->description);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** IBackgroundCopyJob methods ***/

static HRESULT WINAPI BackgroundCopyJob_AddFileSet(
    IBackgroundCopyJob2 *iface,
    ULONG cFileCount,
    BG_FILE_INFO *pFileSet)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    HRESULT hr = S_OK;
    ULONG i;

    TRACE("(%p)->(%d %p)\n", This, cFileCount, pFileSet);

    EnterCriticalSection(&This->cs);

    for (i = 0; i < cFileCount; ++i)
    {
        BackgroundCopyFileImpl *file;

        /* We should return E_INVALIDARG in these cases. */
        FIXME("Check for valid filenames and supported protocols\n");

        hr = BackgroundCopyFileConstructor(This, pFileSet[i].RemoteName, pFileSet[i].LocalName, &file);
        if (hr != S_OK) break;

        /* Add a reference to the file to file list */
        list_add_head(&This->files, &file->entryFromJob);
        This->jobProgress.BytesTotal = BG_SIZE_UNKNOWN;
        ++This->jobProgress.FilesTotal;
    }

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BackgroundCopyJob_AddFile(
    IBackgroundCopyJob2 *iface,
    LPCWSTR RemoteUrl,
    LPCWSTR LocalName)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    BG_FILE_INFO file;

    TRACE("(%p)->(%s %s)\n", This, debugstr_w(RemoteUrl), debugstr_w(LocalName));

    file.RemoteName = (LPWSTR)RemoteUrl;
    file.LocalName = (LPWSTR)LocalName;
    return IBackgroundCopyJob2_AddFileSet(iface, 1, &file);
}

static HRESULT WINAPI BackgroundCopyJob_EnumFiles(
    IBackgroundCopyJob2 *iface,
    IEnumBackgroundCopyFiles **enum_files)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    TRACE("(%p)->(%p)\n", This, enum_files);
    return EnumBackgroundCopyFilesConstructor(This, enum_files);
}

static HRESULT WINAPI BackgroundCopyJob_Suspend(
    IBackgroundCopyJob2 *iface)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_Resume(
    IBackgroundCopyJob2 *iface)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    HRESULT rv = S_OK;

    TRACE("(%p)\n", This);

    EnterCriticalSection(&globalMgr.cs);
    if (is_job_done(This))
    {
        rv = BG_E_INVALID_STATE;
    }
    else if (This->jobProgress.FilesTransferred == This->jobProgress.FilesTotal)
    {
        rv = BG_E_EMPTY;
    }
    else if (This->state != BG_JOB_STATE_CONNECTING
             && This->state != BG_JOB_STATE_TRANSFERRING)
    {
        This->state = BG_JOB_STATE_QUEUED;
        SetEvent(globalMgr.jobEvent);
    }
    LeaveCriticalSection(&globalMgr.cs);

    return rv;
}

static HRESULT WINAPI BackgroundCopyJob_Cancel(
    IBackgroundCopyJob2 *iface)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_Complete(
    IBackgroundCopyJob2 *iface)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    HRESULT rv = S_OK;

    TRACE("(%p)\n", This);

    EnterCriticalSection(&This->cs);

    if (is_job_done(This))
    {
        rv = BG_E_INVALID_STATE;
    }
    else
    {
        BackgroundCopyFileImpl *file;
        LIST_FOR_EACH_ENTRY(file, &This->files, BackgroundCopyFileImpl, entryFromJob)
        {
            if (file->fileProgress.Completed)
            {
                if (!MoveFileExW(file->tempFileName, file->info.LocalName,
                                 (MOVEFILE_COPY_ALLOWED
                                  | MOVEFILE_REPLACE_EXISTING
                                  | MOVEFILE_WRITE_THROUGH)))
                {
                    ERR("Couldn't rename file %s -> %s\n",
                        debugstr_w(file->tempFileName),
                        debugstr_w(file->info.LocalName));
                    rv = BG_S_PARTIAL_COMPLETE;
                }
            }
            else
                rv = BG_S_PARTIAL_COMPLETE;
        }
    }

    This->state = BG_JOB_STATE_ACKNOWLEDGED;
    LeaveCriticalSection(&This->cs);

    return rv;
}

static HRESULT WINAPI BackgroundCopyJob_GetId(
    IBackgroundCopyJob2 *iface,
    GUID *pVal)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    TRACE("(%p)->(%p)\n", This, pVal);
    *pVal = This->jobId;
    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_GetType(
    IBackgroundCopyJob2 *iface,
    BG_JOB_TYPE *pVal)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);

    TRACE("(%p)->(%p)\n", This, pVal);

    if (!pVal)
        return E_INVALIDARG;

    *pVal = This->type;
    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_GetProgress(
    IBackgroundCopyJob2 *iface,
    BG_JOB_PROGRESS *pVal)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);

    TRACE("(%p)->(%p)\n", This, pVal);

    if (!pVal)
        return E_INVALIDARG;

    EnterCriticalSection(&This->cs);
    pVal->BytesTotal = This->jobProgress.BytesTotal;
    pVal->BytesTransferred = This->jobProgress.BytesTransferred;
    pVal->FilesTotal = This->jobProgress.FilesTotal;
    pVal->FilesTransferred = This->jobProgress.FilesTransferred;
    LeaveCriticalSection(&This->cs);

    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_GetTimes(
    IBackgroundCopyJob2 *iface,
    BG_JOB_TIMES *pVal)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    FIXME("(%p)->(%p): stub\n", This, pVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_GetState(
    IBackgroundCopyJob2 *iface,
    BG_JOB_STATE *pVal)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);

    TRACE("(%p)->(%p)\n", This, pVal);

    if (!pVal)
        return E_INVALIDARG;

    /* Don't think we need a critical section for this */
    *pVal = This->state;
    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_GetError(
    IBackgroundCopyJob2 *iface,
    IBackgroundCopyError **ppError)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    FIXME("(%p)->(%p): stub\n", This, ppError);
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_GetOwner(
    IBackgroundCopyJob2 *iface,
    LPWSTR *pVal)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    FIXME("(%p)->(%p): stub\n", This, pVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_SetDisplayName(
    IBackgroundCopyJob2 *iface,
    LPCWSTR Val)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    FIXME("(%p)->(%s): stub\n", This, debugstr_w(Val));
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_GetDisplayName(
    IBackgroundCopyJob2 *iface,
    LPWSTR *pVal)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);

    TRACE("(%p)->(%p)\n", This, pVal);

    return return_strval(This->displayName, pVal);
}

static HRESULT WINAPI BackgroundCopyJob_SetDescription(
    IBackgroundCopyJob2 *iface,
    LPCWSTR Val)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    static const int max_description_len = 1024;
    HRESULT hr = S_OK;
    int len;

    TRACE("(%p)->(%s)\n", This, debugstr_w(Val));

    if (!Val) return E_INVALIDARG;

    len = strlenW(Val);
    if (len > max_description_len) return BG_E_STRING_TOO_LONG;

    EnterCriticalSection(&This->cs);

    if (is_job_done(This))
    {
        hr = BG_E_INVALID_STATE;
    }
    else
    {
        HeapFree(GetProcessHeap(), 0, This->description);
        if ((This->description = HeapAlloc(GetProcessHeap(), 0, (len+1)*sizeof(WCHAR))))
            strcpyW(This->description, Val);
        else
            hr = E_OUTOFMEMORY;
    }

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BackgroundCopyJob_GetDescription(
    IBackgroundCopyJob2 *iface,
    LPWSTR *pVal)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);

    TRACE("(%p)->(%p)\n", This, pVal);

    return return_strval(This->description, pVal);
}

static HRESULT WINAPI BackgroundCopyJob_SetPriority(
    IBackgroundCopyJob2 *iface,
    BG_JOB_PRIORITY Val)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    FIXME("(%p)->(%d): stub\n", This, Val);
    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_GetPriority(
    IBackgroundCopyJob2 *iface,
    BG_JOB_PRIORITY *pVal)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    FIXME("(%p)->(%p): stub\n", This, pVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_SetNotifyFlags(
    IBackgroundCopyJob2 *iface,
    ULONG Val)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    static const ULONG valid_flags = BG_NOTIFY_JOB_TRANSFERRED |
                                     BG_NOTIFY_JOB_ERROR |
                                     BG_NOTIFY_DISABLE |
                                     BG_NOTIFY_JOB_MODIFICATION |
                                     BG_NOTIFY_FILE_TRANSFERRED;

    TRACE("(%p)->(0x%x)\n", This, Val);

    if (is_job_done(This)) return BG_E_INVALID_STATE;
    if (Val & ~valid_flags) return E_NOTIMPL;
    This->notify_flags = Val;
    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_GetNotifyFlags(
    IBackgroundCopyJob2 *iface,
    ULONG *pVal)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);

    TRACE("(%p)->(%p)\n", This, pVal);

    if (!pVal) return E_INVALIDARG;

    *pVal = This->notify_flags;

    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_SetNotifyInterface(
    IBackgroundCopyJob2 *iface,
    IUnknown *Val)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    HRESULT hr = S_OK;

    TRACE("(%p)->(%p)\n", This, Val);

    if (is_job_done(This)) return BG_E_INVALID_STATE;

    if (This->callback)
    {
        IBackgroundCopyCallback2_Release(This->callback);
        This->callback = NULL;
        This->callback2 = FALSE;
    }

    if (Val)
    {
        hr = IUnknown_QueryInterface(Val, &IID_IBackgroundCopyCallback2, (void**)&This->callback);
        if (FAILED(hr))
            hr = IUnknown_QueryInterface(Val, &IID_IBackgroundCopyCallback, (void**)&This->callback);
        else
            This->callback2 = TRUE;
    }

    return hr;
}

static HRESULT WINAPI BackgroundCopyJob_GetNotifyInterface(
    IBackgroundCopyJob2 *iface,
    IUnknown **pVal)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);

    TRACE("(%p)->(%p)\n", This, pVal);

    if (!pVal) return E_INVALIDARG;

    *pVal = (IUnknown*)This->callback;
    if (*pVal)
        IUnknown_AddRef(*pVal);

    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_SetMinimumRetryDelay(
    IBackgroundCopyJob2 *iface,
    ULONG Seconds)
{
    FIXME("%u\n", Seconds);
    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_GetMinimumRetryDelay(
    IBackgroundCopyJob2 *iface,
    ULONG *Seconds)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    FIXME("(%p)->(%p): stub\n", This, Seconds);
    *Seconds = 30;
    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_SetNoProgressTimeout(
    IBackgroundCopyJob2 *iface,
    ULONG Seconds)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    FIXME("(%p)->(%d): stub\n", This, Seconds);
    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_GetNoProgressTimeout(
    IBackgroundCopyJob2 *iface,
    ULONG *Seconds)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    FIXME("(%p)->(%p): stub\n", This, Seconds);
    *Seconds = 900;
    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_GetErrorCount(
    IBackgroundCopyJob2 *iface,
    ULONG *Errors)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    FIXME("(%p)->(%p): stub\n", This, Errors);
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_SetProxySettings(
    IBackgroundCopyJob2 *iface,
    BG_JOB_PROXY_USAGE ProxyUsage,
    const WCHAR *ProxyList,
    const WCHAR *ProxyBypassList)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    FIXME("(%p)->(%d %s %s): stub\n", This, ProxyUsage, debugstr_w(ProxyList), debugstr_w(ProxyBypassList));
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_GetProxySettings(
    IBackgroundCopyJob2 *iface,
    BG_JOB_PROXY_USAGE *pProxyUsage,
    LPWSTR *pProxyList,
    LPWSTR *pProxyBypassList)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    FIXME("(%p)->(%p %p %p): stub\n", This, pProxyUsage, pProxyList, pProxyBypassList);
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_TakeOwnership(
    IBackgroundCopyJob2 *iface)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_SetNotifyCmdLine(
    IBackgroundCopyJob2 *iface,
    LPCWSTR prog,
    LPCWSTR params)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    FIXME("(%p)->(%s %s): stub\n", This, debugstr_w(prog), debugstr_w(params));
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_GetNotifyCmdLine(
    IBackgroundCopyJob2 *iface,
    LPWSTR *prog,
    LPWSTR *params)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    FIXME("(%p)->(%p %p): stub\n", This, prog, params);
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_GetReplyProgress(
    IBackgroundCopyJob2 *iface,
    BG_JOB_REPLY_PROGRESS *progress)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    FIXME("(%p)->(%p): stub\n", This, progress);
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_GetReplyData(
    IBackgroundCopyJob2 *iface,
    byte **pBuffer,
    UINT64 *pLength)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    FIXME("(%p)->(%p %p): stub\n", This, pBuffer, pLength);
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_SetReplyFileName(
    IBackgroundCopyJob2 *iface,
    LPCWSTR filename)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    FIXME("(%p)->(%s): stub\n", This, debugstr_w(filename));
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_GetReplyFileName(
    IBackgroundCopyJob2 *iface,
    LPWSTR *pFilename)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    FIXME("(%p)->(%p): stub\n", This, pFilename);
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_SetCredentials(
    IBackgroundCopyJob2 *iface,
    BG_AUTH_CREDENTIALS *cred)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    FIXME("(%p)->(%p): stub\n", This, cred);
    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_RemoveCredentials(
    IBackgroundCopyJob2 *iface,
    BG_AUTH_TARGET target,
    BG_AUTH_SCHEME scheme)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob2(iface);
    FIXME("(%p)->(%d %d): stub\n", This, target, scheme);
    return S_OK;
}

static const IBackgroundCopyJob2Vtbl BackgroundCopyJobVtbl =
{
    BackgroundCopyJob_QueryInterface,
    BackgroundCopyJob_AddRef,
    BackgroundCopyJob_Release,
    BackgroundCopyJob_AddFileSet,
    BackgroundCopyJob_AddFile,
    BackgroundCopyJob_EnumFiles,
    BackgroundCopyJob_Suspend,
    BackgroundCopyJob_Resume,
    BackgroundCopyJob_Cancel,
    BackgroundCopyJob_Complete,
    BackgroundCopyJob_GetId,
    BackgroundCopyJob_GetType,
    BackgroundCopyJob_GetProgress,
    BackgroundCopyJob_GetTimes,
    BackgroundCopyJob_GetState,
    BackgroundCopyJob_GetError,
    BackgroundCopyJob_GetOwner,
    BackgroundCopyJob_SetDisplayName,
    BackgroundCopyJob_GetDisplayName,
    BackgroundCopyJob_SetDescription,
    BackgroundCopyJob_GetDescription,
    BackgroundCopyJob_SetPriority,
    BackgroundCopyJob_GetPriority,
    BackgroundCopyJob_SetNotifyFlags,
    BackgroundCopyJob_GetNotifyFlags,
    BackgroundCopyJob_SetNotifyInterface,
    BackgroundCopyJob_GetNotifyInterface,
    BackgroundCopyJob_SetMinimumRetryDelay,
    BackgroundCopyJob_GetMinimumRetryDelay,
    BackgroundCopyJob_SetNoProgressTimeout,
    BackgroundCopyJob_GetNoProgressTimeout,
    BackgroundCopyJob_GetErrorCount,
    BackgroundCopyJob_SetProxySettings,
    BackgroundCopyJob_GetProxySettings,
    BackgroundCopyJob_TakeOwnership,
    BackgroundCopyJob_SetNotifyCmdLine,
    BackgroundCopyJob_GetNotifyCmdLine,
    BackgroundCopyJob_GetReplyProgress,
    BackgroundCopyJob_GetReplyData,
    BackgroundCopyJob_SetReplyFileName,
    BackgroundCopyJob_GetReplyFileName,
    BackgroundCopyJob_SetCredentials,
    BackgroundCopyJob_RemoveCredentials
};

HRESULT BackgroundCopyJobConstructor(LPCWSTR displayName, BG_JOB_TYPE type, GUID *job_id, BackgroundCopyJobImpl **job)
{
    HRESULT hr;
    BackgroundCopyJobImpl *This;
    int n;

    TRACE("(%s,%d,%p)\n", debugstr_w(displayName), type, job);

    This = HeapAlloc(GetProcessHeap(), 0, sizeof *This);
    if (!This)
        return E_OUTOFMEMORY;

    This->IBackgroundCopyJob2_iface.lpVtbl = &BackgroundCopyJobVtbl;
    InitializeCriticalSection(&This->cs);
    This->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": BackgroundCopyJobImpl.cs");

    This->ref = 1;
    This->type = type;

    n = (strlenW(displayName) + 1) *  sizeof *displayName;
    This->displayName = HeapAlloc(GetProcessHeap(), 0, n);
    if (!This->displayName)
    {
        This->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->cs);
        HeapFree(GetProcessHeap(), 0, This);
        return E_OUTOFMEMORY;
    }
    memcpy(This->displayName, displayName, n);

    hr = CoCreateGuid(&This->jobId);
    if (FAILED(hr))
    {
        This->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->cs);
        HeapFree(GetProcessHeap(), 0, This->displayName);
        HeapFree(GetProcessHeap(), 0, This);
        return hr;
    }
    *job_id = This->jobId;

    list_init(&This->files);
    This->jobProgress.BytesTotal = 0;
    This->jobProgress.BytesTransferred = 0;
    This->jobProgress.FilesTotal = 0;
    This->jobProgress.FilesTransferred = 0;

    This->state = BG_JOB_STATE_SUSPENDED;
    This->description = NULL;
    This->notify_flags = BG_NOTIFY_JOB_ERROR | BG_NOTIFY_JOB_TRANSFERRED;
    This->callback = NULL;
    This->callback2 = FALSE;

    *job = This;

    TRACE("created job %s:%p\n", debugstr_guid(&This->jobId), This);

    return S_OK;
}

void processJob(BackgroundCopyJobImpl *job)
{
    for (;;)
    {
        BackgroundCopyFileImpl *file;
        BOOL done = TRUE;

        EnterCriticalSection(&job->cs);
        LIST_FOR_EACH_ENTRY(file, &job->files, BackgroundCopyFileImpl, entryFromJob)
            if (!file->fileProgress.Completed)
            {
                done = FALSE;
                break;
            }
        LeaveCriticalSection(&job->cs);
        if (done)
        {
            transitionJobState(job, BG_JOB_STATE_QUEUED, BG_JOB_STATE_TRANSFERRED);
            return;
        }

        if (!processFile(file, job))
          return;
    }
}
