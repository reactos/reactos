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

BOOL transitionJobState(BackgroundCopyJobImpl *job, BG_JOB_STATE from, BG_JOB_STATE to)
{
    BOOL ret = FALSE;

    EnterCriticalSection(&globalMgr.cs);
    if (job->state == from)
    {
        job->state = to;
        ret = TRUE;
    }
    LeaveCriticalSection(&globalMgr.cs);
    return ret;
}

struct copy_error
{
    IBackgroundCopyError  IBackgroundCopyError_iface;
    LONG                  refs;
    BG_ERROR_CONTEXT      context;
    HRESULT               code;
    IBackgroundCopyFile2 *file;
};

static inline struct copy_error *impl_from_IBackgroundCopyError(IBackgroundCopyError *iface)
{
    return CONTAINING_RECORD(iface, struct copy_error, IBackgroundCopyError_iface);
}

static HRESULT WINAPI copy_error_QueryInterface(
    IBackgroundCopyError *iface,
    REFIID riid,
    void **obj)
{
    struct copy_error *error = impl_from_IBackgroundCopyError(iface);

    TRACE("(%p)->(%s %p)\n", error, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IBackgroundCopyError))
    {
        *obj = &error->IBackgroundCopyError_iface;
    }
    else
    {
        *obj = NULL;
        WARN("interface %s not supported\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IBackgroundCopyError_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI copy_error_AddRef(
    IBackgroundCopyError *iface)
{
    struct copy_error *error = impl_from_IBackgroundCopyError(iface);
    LONG refs = InterlockedIncrement(&error->refs);
    TRACE("(%p)->(%d)\n", error, refs);
    return refs;
}

static ULONG WINAPI copy_error_Release(
    IBackgroundCopyError *iface)
{
    struct copy_error *error = impl_from_IBackgroundCopyError(iface);
    LONG refs = InterlockedDecrement(&error->refs);

    TRACE("(%p)->(%d)\n", error, refs);

    if (!refs)
    {
        if (error->file) IBackgroundCopyFile2_Release(error->file);
        HeapFree(GetProcessHeap(), 0, error);
    }
    return refs;
}

static HRESULT WINAPI copy_error_GetError(
    IBackgroundCopyError *iface,
    BG_ERROR_CONTEXT *pContext,
    HRESULT *pCode)
{
    struct copy_error *error = impl_from_IBackgroundCopyError(iface);

    TRACE("(%p)->(%p %p)\n", error, pContext, pCode);

    *pContext = error->context;
    *pCode = error->code;

    TRACE("returning context %u error code 0x%08x\n", error->context, error->code);
    return S_OK;
}

static HRESULT WINAPI copy_error_GetFile(
    IBackgroundCopyError *iface,
    IBackgroundCopyFile **pVal)
{
    struct copy_error *error = impl_from_IBackgroundCopyError(iface);

    TRACE("(%p)->(%p)\n", error, pVal);

    if (error->file)
    {
        IBackgroundCopyFile2_AddRef(error->file);
        *pVal = (IBackgroundCopyFile *)error->file;
        return S_OK;
    }
    *pVal = NULL;
    return BG_E_FILE_NOT_AVAILABLE;
}

static HRESULT WINAPI copy_error_GetErrorDescription(
    IBackgroundCopyError *iface,
    DWORD LanguageId,
    LPWSTR *pErrorDescription)
{
    struct copy_error *error = impl_from_IBackgroundCopyError(iface);
    FIXME("(%p)->(%p)\n", error, pErrorDescription);
    return E_NOTIMPL;
}

static HRESULT WINAPI copy_error_GetErrorContextDescription(
    IBackgroundCopyError *iface,
    DWORD LanguageId,
    LPWSTR *pContextDescription)
{
    struct copy_error *error = impl_from_IBackgroundCopyError(iface);
    FIXME("(%p)->(%p)\n", error, pContextDescription);
    return E_NOTIMPL;
}

static HRESULT WINAPI copy_error_GetProtocol(
    IBackgroundCopyError *iface,
    LPWSTR *pProtocol)
{
    struct copy_error *error = impl_from_IBackgroundCopyError(iface);
    FIXME("(%p)->(%p)\n", error, pProtocol);
    return E_NOTIMPL;
}

static const IBackgroundCopyErrorVtbl copy_error_vtbl =
{
    copy_error_QueryInterface,
    copy_error_AddRef,
    copy_error_Release,
    copy_error_GetError,
    copy_error_GetFile,
    copy_error_GetErrorDescription,
    copy_error_GetErrorContextDescription,
    copy_error_GetProtocol
};

static HRESULT create_copy_error(
    BG_ERROR_CONTEXT context,
    HRESULT code,
    IBackgroundCopyFile2 *file,
    IBackgroundCopyError **obj)
{
    struct copy_error *error;

    TRACE("context %u code %08x file %p\n", context, code, file);

    if (!(error = HeapAlloc(GetProcessHeap(), 0, sizeof(*error) ))) return E_OUTOFMEMORY;
    error->IBackgroundCopyError_iface.lpVtbl = &copy_error_vtbl;
    error->refs    = 1;
    error->context = context;
    error->code    = code;
    error->file    = file;
    if (error->file) IBackgroundCopyFile2_AddRef(error->file);

    *obj = &error->IBackgroundCopyError_iface;
    TRACE("returning iface %p\n", *obj);
    return S_OK;
}

static inline BOOL is_job_done(const BackgroundCopyJobImpl *job)
{
    return job->state == BG_JOB_STATE_CANCELLED || job->state == BG_JOB_STATE_ACKNOWLEDGED;
}

static inline BackgroundCopyJobImpl *impl_from_IBackgroundCopyJob3(IBackgroundCopyJob3 *iface)
{
    return CONTAINING_RECORD(iface, BackgroundCopyJobImpl, IBackgroundCopyJob3_iface);
}

static HRESULT WINAPI BackgroundCopyJob_QueryInterface(
    IBackgroundCopyJob3 *iface, REFIID riid, void **obj)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IBackgroundCopyJob) ||
        IsEqualGUID(riid, &IID_IBackgroundCopyJob2) ||
        IsEqualGUID(riid, &IID_IBackgroundCopyJob3))
    {
        *obj = &This->IBackgroundCopyJob3_iface;
    }
    else if (IsEqualGUID(riid, &IID_IBackgroundCopyJobHttpOptions))
    {
        *obj = &This->IBackgroundCopyJobHttpOptions_iface;
    }
    else
    {
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IBackgroundCopyJob3_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI BackgroundCopyJob_AddRef(IBackgroundCopyJob3 *iface)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI BackgroundCopyJob_Release(IBackgroundCopyJob3 *iface)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    ULONG i, j, ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (ref == 0)
    {
        This->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->cs);
        if (This->callback)
            IBackgroundCopyCallback2_Release(This->callback);
        HeapFree(GetProcessHeap(), 0, This->displayName);
        HeapFree(GetProcessHeap(), 0, This->description);
        HeapFree(GetProcessHeap(), 0, This->http_options.headers);
        for (i = 0; i < BG_AUTH_TARGET_PROXY; i++)
        {
            for (j = 0; j < BG_AUTH_SCHEME_PASSPORT; j++)
            {
                BG_AUTH_CREDENTIALS *cred = &This->http_options.creds[i][j];
                HeapFree(GetProcessHeap(), 0, cred->Credentials.Basic.UserName);
                HeapFree(GetProcessHeap(), 0, cred->Credentials.Basic.Password);
            }
        }
        CloseHandle(This->wait);
        CloseHandle(This->cancel);
        CloseHandle(This->done);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** IBackgroundCopyJob methods ***/

static HRESULT WINAPI BackgroundCopyJob_AddFileSet(
    IBackgroundCopyJob3 *iface,
    ULONG cFileCount,
    BG_FILE_INFO *pFileSet)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
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
    IBackgroundCopyJob3 *iface,
    LPCWSTR RemoteUrl,
    LPCWSTR LocalName)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    BG_FILE_INFO file;

    TRACE("(%p)->(%s %s)\n", This, debugstr_w(RemoteUrl), debugstr_w(LocalName));

    file.RemoteName = (LPWSTR)RemoteUrl;
    file.LocalName = (LPWSTR)LocalName;
    return IBackgroundCopyJob3_AddFileSet(iface, 1, &file);
}

static HRESULT WINAPI BackgroundCopyJob_EnumFiles(
    IBackgroundCopyJob3 *iface,
    IEnumBackgroundCopyFiles **enum_files)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    TRACE("(%p)->(%p)\n", This, enum_files);
    return EnumBackgroundCopyFilesConstructor(This, enum_files);
}

static HRESULT WINAPI BackgroundCopyJob_Suspend(
    IBackgroundCopyJob3 *iface)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_Resume(
    IBackgroundCopyJob3 *iface)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
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
    IBackgroundCopyJob3 *iface)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
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

        if (This->state == BG_JOB_STATE_CONNECTING || This->state == BG_JOB_STATE_TRANSFERRING)
        {
            This->state = BG_JOB_STATE_CANCELLED;
            SetEvent(This->cancel);

            LeaveCriticalSection(&This->cs);
            WaitForSingleObject(This->done, INFINITE);
            EnterCriticalSection(&This->cs);
        }

        LIST_FOR_EACH_ENTRY(file, &This->files, BackgroundCopyFileImpl, entryFromJob)
        {
            if (file->tempFileName[0] && !DeleteFileW(file->tempFileName))
            {
                WARN("Couldn't delete %s (%u)\n", debugstr_w(file->tempFileName), GetLastError());
                rv = BG_S_UNABLE_TO_DELETE_FILES;
            }
            if (file->info.LocalName && !DeleteFileW(file->info.LocalName))
            {
                WARN("Couldn't delete %s (%u)\n", debugstr_w(file->info.LocalName), GetLastError());
                rv = BG_S_UNABLE_TO_DELETE_FILES;
            }
        }
        This->state = BG_JOB_STATE_CANCELLED;
    }

    LeaveCriticalSection(&This->cs);
    return rv;
}

static HRESULT WINAPI BackgroundCopyJob_Complete(
    IBackgroundCopyJob3 *iface)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
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
    IBackgroundCopyJob3 *iface,
    GUID *pVal)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    TRACE("(%p)->(%p)\n", This, pVal);
    *pVal = This->jobId;
    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_GetType(
    IBackgroundCopyJob3 *iface,
    BG_JOB_TYPE *pVal)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);

    TRACE("(%p)->(%p)\n", This, pVal);

    if (!pVal)
        return E_INVALIDARG;

    *pVal = This->type;
    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_GetProgress(
    IBackgroundCopyJob3 *iface,
    BG_JOB_PROGRESS *pVal)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);

    TRACE("(%p)->(%p)\n", This, pVal);

    if (!pVal)
        return E_INVALIDARG;

    EnterCriticalSection(&This->cs);
    *pVal = This->jobProgress;
    LeaveCriticalSection(&This->cs);

    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_GetTimes(
    IBackgroundCopyJob3 *iface,
    BG_JOB_TIMES *pVal)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    FIXME("(%p)->(%p): stub\n", This, pVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_GetState(
    IBackgroundCopyJob3 *iface,
    BG_JOB_STATE *pVal)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);

    TRACE("(%p)->(%p)\n", This, pVal);

    if (!pVal)
        return E_INVALIDARG;

    /* Don't think we need a critical section for this */
    *pVal = This->state;
    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_GetError(
    IBackgroundCopyJob3 *iface,
    IBackgroundCopyError **ppError)
{
    BackgroundCopyJobImpl *job = impl_from_IBackgroundCopyJob3(iface);

    TRACE("(%p)->(%p)\n", job, ppError);

    if (!job->error.context) return BG_E_ERROR_INFORMATION_UNAVAILABLE;

    return create_copy_error(job->error.context, job->error.code, job->error.file, ppError);
}

static HRESULT WINAPI BackgroundCopyJob_GetOwner(
    IBackgroundCopyJob3 *iface,
    LPWSTR *pVal)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    FIXME("(%p)->(%p): stub\n", This, pVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_SetDisplayName(
    IBackgroundCopyJob3 *iface,
    LPCWSTR Val)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    FIXME("(%p)->(%s): stub\n", This, debugstr_w(Val));
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_GetDisplayName(
    IBackgroundCopyJob3 *iface,
    LPWSTR *pVal)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);

    TRACE("(%p)->(%p)\n", This, pVal);

    return return_strval(This->displayName, pVal);
}

static HRESULT WINAPI BackgroundCopyJob_SetDescription(
    IBackgroundCopyJob3 *iface,
    LPCWSTR Val)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
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
    IBackgroundCopyJob3 *iface,
    LPWSTR *pVal)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);

    TRACE("(%p)->(%p)\n", This, pVal);

    return return_strval(This->description, pVal);
}

static HRESULT WINAPI BackgroundCopyJob_SetPriority(
    IBackgroundCopyJob3 *iface,
    BG_JOB_PRIORITY Val)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    FIXME("(%p)->(%d): stub\n", This, Val);
    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_GetPriority(
    IBackgroundCopyJob3 *iface,
    BG_JOB_PRIORITY *pVal)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    FIXME("(%p)->(%p): stub\n", This, pVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_SetNotifyFlags(
    IBackgroundCopyJob3 *iface,
    ULONG Val)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
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
    IBackgroundCopyJob3 *iface,
    ULONG *pVal)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);

    TRACE("(%p)->(%p)\n", This, pVal);

    if (!pVal) return E_INVALIDARG;

    *pVal = This->notify_flags;

    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_SetNotifyInterface(
    IBackgroundCopyJob3 *iface,
    IUnknown *Val)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
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
    IBackgroundCopyJob3 *iface,
    IUnknown **pVal)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);

    TRACE("(%p)->(%p)\n", This, pVal);

    if (!pVal) return E_INVALIDARG;

    *pVal = (IUnknown*)This->callback;
    if (*pVal)
        IUnknown_AddRef(*pVal);

    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_SetMinimumRetryDelay(
    IBackgroundCopyJob3 *iface,
    ULONG Seconds)
{
    FIXME("%u\n", Seconds);
    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_GetMinimumRetryDelay(
    IBackgroundCopyJob3 *iface,
    ULONG *Seconds)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    FIXME("(%p)->(%p): stub\n", This, Seconds);
    *Seconds = 30;
    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_SetNoProgressTimeout(
    IBackgroundCopyJob3 *iface,
    ULONG Seconds)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    FIXME("(%p)->(%d): stub\n", This, Seconds);
    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_GetNoProgressTimeout(
    IBackgroundCopyJob3 *iface,
    ULONG *Seconds)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    FIXME("(%p)->(%p): stub\n", This, Seconds);
    *Seconds = 900;
    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_GetErrorCount(
    IBackgroundCopyJob3 *iface,
    ULONG *Errors)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    FIXME("(%p)->(%p): stub\n", This, Errors);
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_SetProxySettings(
    IBackgroundCopyJob3 *iface,
    BG_JOB_PROXY_USAGE ProxyUsage,
    const WCHAR *ProxyList,
    const WCHAR *ProxyBypassList)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    FIXME("(%p)->(%d %s %s): stub\n", This, ProxyUsage, debugstr_w(ProxyList), debugstr_w(ProxyBypassList));
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_GetProxySettings(
    IBackgroundCopyJob3 *iface,
    BG_JOB_PROXY_USAGE *pProxyUsage,
    LPWSTR *pProxyList,
    LPWSTR *pProxyBypassList)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    FIXME("(%p)->(%p %p %p): stub\n", This, pProxyUsage, pProxyList, pProxyBypassList);
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_TakeOwnership(
    IBackgroundCopyJob3 *iface)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_SetNotifyCmdLine(
    IBackgroundCopyJob3 *iface,
    LPCWSTR prog,
    LPCWSTR params)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    FIXME("(%p)->(%s %s): stub\n", This, debugstr_w(prog), debugstr_w(params));
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_GetNotifyCmdLine(
    IBackgroundCopyJob3 *iface,
    LPWSTR *prog,
    LPWSTR *params)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    FIXME("(%p)->(%p %p): stub\n", This, prog, params);
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_GetReplyProgress(
    IBackgroundCopyJob3 *iface,
    BG_JOB_REPLY_PROGRESS *progress)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    FIXME("(%p)->(%p): stub\n", This, progress);
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_GetReplyData(
    IBackgroundCopyJob3 *iface,
    byte **pBuffer,
    UINT64 *pLength)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    FIXME("(%p)->(%p %p): stub\n", This, pBuffer, pLength);
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_SetReplyFileName(
    IBackgroundCopyJob3 *iface,
    LPCWSTR filename)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    FIXME("(%p)->(%s): stub\n", This, debugstr_w(filename));
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyJob_GetReplyFileName(
    IBackgroundCopyJob3 *iface,
    LPWSTR *pFilename)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    FIXME("(%p)->(%p): stub\n", This, pFilename);
    return E_NOTIMPL;
}

static int index_from_target(BG_AUTH_TARGET target)
{
    if (!target || target > BG_AUTH_TARGET_PROXY) return -1;
    return target - 1;
}

static int index_from_scheme(BG_AUTH_SCHEME scheme)
{
    if (!scheme || scheme > BG_AUTH_SCHEME_PASSPORT) return -1;
    return scheme - 1;
}

static HRESULT WINAPI BackgroundCopyJob_SetCredentials(
    IBackgroundCopyJob3 *iface,
    BG_AUTH_CREDENTIALS *cred)
{
    BackgroundCopyJobImpl *job = impl_from_IBackgroundCopyJob3(iface);
    BG_AUTH_CREDENTIALS *new_cred;
    int idx_target, idx_scheme;

    TRACE("(%p)->(%p)\n", job, cred);

    if ((idx_target = index_from_target(cred->Target)) < 0) return BG_E_INVALID_AUTH_TARGET;
    if ((idx_scheme = index_from_scheme(cred->Scheme)) < 0) return BG_E_INVALID_AUTH_SCHEME;
    new_cred = &job->http_options.creds[idx_target][idx_scheme];

    EnterCriticalSection(&job->cs);

    new_cred->Target = cred->Target;
    new_cred->Scheme = cred->Scheme;

    if (cred->Credentials.Basic.UserName)
    {
        HeapFree(GetProcessHeap(), 0, new_cred->Credentials.Basic.UserName);
        new_cred->Credentials.Basic.UserName = strdupW(cred->Credentials.Basic.UserName);
    }
    if (cred->Credentials.Basic.Password)
    {
        HeapFree(GetProcessHeap(), 0, new_cred->Credentials.Basic.Password);
        new_cred->Credentials.Basic.Password = strdupW(cred->Credentials.Basic.Password);
    }

    LeaveCriticalSection(&job->cs);
    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_RemoveCredentials(
    IBackgroundCopyJob3 *iface,
    BG_AUTH_TARGET target,
    BG_AUTH_SCHEME scheme)
{
    BackgroundCopyJobImpl *job = impl_from_IBackgroundCopyJob3(iface);
    BG_AUTH_CREDENTIALS *new_cred;
    int idx_target, idx_scheme;

    TRACE("(%p)->(%u %u)\n", job, target, scheme);

    if ((idx_target = index_from_target(target)) < 0) return BG_E_INVALID_AUTH_TARGET;
    if ((idx_scheme = index_from_scheme(scheme)) < 0) return BG_E_INVALID_AUTH_SCHEME;
    new_cred = &job->http_options.creds[idx_target][idx_scheme];

    EnterCriticalSection(&job->cs);

    new_cred->Target = new_cred->Scheme = 0;
    HeapFree(GetProcessHeap(), 0, new_cred->Credentials.Basic.UserName);
    new_cred->Credentials.Basic.UserName = NULL;
    HeapFree(GetProcessHeap(), 0, new_cred->Credentials.Basic.Password);
    new_cred->Credentials.Basic.Password = NULL;

    LeaveCriticalSection(&job->cs);
    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_ReplaceRemotePrefix(
    IBackgroundCopyJob3 *iface,
    LPCWSTR OldPrefix,
    LPCWSTR NewPrefix)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    FIXME("(%p)->(%s %s): stub\n", This, debugstr_w(OldPrefix), debugstr_w(NewPrefix));
    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_AddFileWithRanges(
    IBackgroundCopyJob3 *iface,
    LPCWSTR RemoteUrl,
    LPCWSTR LocalName,
    DWORD RangeCount,
    BG_FILE_RANGE Ranges[])
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    FIXME("(%p)->(%s %s %u %p): stub\n", This, debugstr_w(RemoteUrl), debugstr_w(LocalName), RangeCount, Ranges);
    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_SetFileACLFlags(
    IBackgroundCopyJob3 *iface,
    DWORD Flags)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    FIXME("(%p)->(%x): stub\n", This, Flags);
    return S_OK;
}

static HRESULT WINAPI BackgroundCopyJob_GetFileACLFlags(
    IBackgroundCopyJob3 *iface,
    DWORD *Flags)
{
    BackgroundCopyJobImpl *This = impl_from_IBackgroundCopyJob3(iface);
    FIXME("(%p)->(%p): stub\n", This, Flags);
    return S_OK;
}

static const IBackgroundCopyJob3Vtbl BackgroundCopyJob3Vtbl =
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
    BackgroundCopyJob_RemoveCredentials,
    BackgroundCopyJob_ReplaceRemotePrefix,
    BackgroundCopyJob_AddFileWithRanges,
    BackgroundCopyJob_SetFileACLFlags,
    BackgroundCopyJob_GetFileACLFlags
};

static inline BackgroundCopyJobImpl *impl_from_IBackgroundCopyJobHttpOptions(
    IBackgroundCopyJobHttpOptions *iface)
{
    return CONTAINING_RECORD(iface, BackgroundCopyJobImpl, IBackgroundCopyJobHttpOptions_iface);
}

static HRESULT WINAPI http_options_QueryInterface(
    IBackgroundCopyJobHttpOptions *iface,
    REFIID riid,
    void **ppvObject)
{
    BackgroundCopyJobImpl *job = impl_from_IBackgroundCopyJobHttpOptions(iface);
    return IBackgroundCopyJob3_QueryInterface(&job->IBackgroundCopyJob3_iface, riid, ppvObject);
}

static ULONG WINAPI http_options_AddRef(
    IBackgroundCopyJobHttpOptions *iface)
{
    BackgroundCopyJobImpl *job = impl_from_IBackgroundCopyJobHttpOptions(iface);
    return IBackgroundCopyJob3_AddRef(&job->IBackgroundCopyJob3_iface);
}

static ULONG WINAPI http_options_Release(
    IBackgroundCopyJobHttpOptions *iface)
{
    BackgroundCopyJobImpl *job = impl_from_IBackgroundCopyJobHttpOptions(iface);
    return IBackgroundCopyJob3_Release(&job->IBackgroundCopyJob3_iface);
}

static HRESULT WINAPI http_options_SetClientCertificateByID(
    IBackgroundCopyJobHttpOptions *iface,
    BG_CERT_STORE_LOCATION StoreLocation,
    LPCWSTR StoreName,
    BYTE *pCertHashBlob)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI http_options_SetClientCertificateByName(
    IBackgroundCopyJobHttpOptions *iface,
    BG_CERT_STORE_LOCATION StoreLocation,
    LPCWSTR StoreName,
    LPCWSTR SubjectName)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI http_options_RemoveClientCertificate(
    IBackgroundCopyJobHttpOptions *iface)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI http_options_GetClientCertificate(
    IBackgroundCopyJobHttpOptions *iface,
    BG_CERT_STORE_LOCATION *pStoreLocation,
    LPWSTR *pStoreName,
    BYTE **ppCertHashBlob,
    LPWSTR *pSubjectName)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI http_options_SetCustomHeaders(
    IBackgroundCopyJobHttpOptions *iface,
    LPCWSTR RequestHeaders)
{
    BackgroundCopyJobImpl *job = impl_from_IBackgroundCopyJobHttpOptions(iface);

    TRACE("(%p)->(%s)\n", iface, debugstr_w(RequestHeaders));

    EnterCriticalSection(&job->cs);

    if (RequestHeaders)
    {
        WCHAR *headers = strdupW(RequestHeaders);
        if (!headers)
        {
            LeaveCriticalSection(&job->cs);
            return E_OUTOFMEMORY;
        }
        HeapFree(GetProcessHeap(), 0, job->http_options.headers);
        job->http_options.headers = headers;
    }
    else
    {
        HeapFree(GetProcessHeap(), 0, job->http_options.headers);
        job->http_options.headers = NULL;
    }

    LeaveCriticalSection(&job->cs);
    return S_OK;
}

static HRESULT WINAPI http_options_GetCustomHeaders(
    IBackgroundCopyJobHttpOptions *iface,
    LPWSTR *pRequestHeaders)
{
    BackgroundCopyJobImpl *job = impl_from_IBackgroundCopyJobHttpOptions(iface);

    TRACE("(%p)->(%p)\n", iface, pRequestHeaders);

    EnterCriticalSection(&job->cs);

    if (job->http_options.headers)
    {
        WCHAR *headers = co_strdupW(job->http_options.headers);
        if (!headers)
        {
            LeaveCriticalSection(&job->cs);
            return E_OUTOFMEMORY;
        }
        *pRequestHeaders = headers;
        LeaveCriticalSection(&job->cs);
        return S_OK;
    }

    *pRequestHeaders = NULL;
    LeaveCriticalSection(&job->cs);
    return S_FALSE;
}

static HRESULT WINAPI http_options_SetSecurityFlags(
    IBackgroundCopyJobHttpOptions *iface,
    ULONG Flags)
{
    BackgroundCopyJobImpl *job = impl_from_IBackgroundCopyJobHttpOptions(iface);

    TRACE("(%p)->(0x%08x)\n", iface, Flags);

    job->http_options.flags = Flags;
    return S_OK;
}

static HRESULT WINAPI http_options_GetSecurityFlags(
    IBackgroundCopyJobHttpOptions *iface,
    ULONG *pFlags)
{
    BackgroundCopyJobImpl *job = impl_from_IBackgroundCopyJobHttpOptions(iface);

    TRACE("(%p)->(%p)\n", iface, pFlags);

    *pFlags = job->http_options.flags;
    return S_OK;
}

static const IBackgroundCopyJobHttpOptionsVtbl http_options_vtbl =
{
    http_options_QueryInterface,
    http_options_AddRef,
    http_options_Release,
    http_options_SetClientCertificateByID,
    http_options_SetClientCertificateByName,
    http_options_RemoveClientCertificate,
    http_options_GetClientCertificate,
    http_options_SetCustomHeaders,
    http_options_GetCustomHeaders,
    http_options_SetSecurityFlags,
    http_options_GetSecurityFlags
};

HRESULT BackgroundCopyJobConstructor(LPCWSTR displayName, BG_JOB_TYPE type, GUID *job_id, BackgroundCopyJobImpl **job)
{
    HRESULT hr;
    BackgroundCopyJobImpl *This;

    TRACE("(%s,%d,%p)\n", debugstr_w(displayName), type, job);

    This = HeapAlloc(GetProcessHeap(), 0, sizeof *This);
    if (!This)
        return E_OUTOFMEMORY;

    This->IBackgroundCopyJob3_iface.lpVtbl = &BackgroundCopyJob3Vtbl;
    This->IBackgroundCopyJobHttpOptions_iface.lpVtbl = &http_options_vtbl;
    InitializeCriticalSection(&This->cs);
    This->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": BackgroundCopyJobImpl.cs");

    This->ref = 1;
    This->type = type;

    This->displayName = strdupW(displayName);
    if (!This->displayName)
    {
        This->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->cs);
        HeapFree(GetProcessHeap(), 0, This);
        return E_OUTOFMEMORY;
    }

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

    This->error.context = 0;
    This->error.code = 0;
    This->error.file = NULL;

    memset(&This->http_options, 0, sizeof(This->http_options));

    This->wait   = CreateEventW(NULL, FALSE, FALSE, NULL);
    This->cancel = CreateEventW(NULL, FALSE, FALSE, NULL);
    This->done   = CreateEventW(NULL, FALSE, FALSE, NULL);

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
