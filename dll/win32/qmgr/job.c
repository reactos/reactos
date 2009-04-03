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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#include "qmgr.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qmgr);

static void BackgroundCopyJobDestructor(BackgroundCopyJobImpl *This)
{
    DeleteCriticalSection(&This->cs);
    HeapFree(GetProcessHeap(), 0, This->displayName);
    HeapFree(GetProcessHeap(), 0, This);
}

static ULONG WINAPI BITS_IBackgroundCopyJob_AddRef(IBackgroundCopyJob2 *iface)
{
    BackgroundCopyJobImpl *This = (BackgroundCopyJobImpl *) iface;
    return InterlockedIncrement(&This->ref);
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_QueryInterface(
    IBackgroundCopyJob2 *iface, REFIID riid, LPVOID *ppvObject)
{
    BackgroundCopyJobImpl *This = (BackgroundCopyJobImpl *) iface;
    TRACE("IID: %s\n", debugstr_guid(riid));

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IBackgroundCopyJob)
        || IsEqualGUID(riid, &IID_IBackgroundCopyJob2))
    {
        *ppvObject = &This->lpVtbl;
        BITS_IBackgroundCopyJob_AddRef(iface);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI BITS_IBackgroundCopyJob_Release(IBackgroundCopyJob2 *iface)
{
    BackgroundCopyJobImpl *This = (BackgroundCopyJobImpl *) iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    if (ref == 0)
        BackgroundCopyJobDestructor(This);

    return ref;
}

/*** IBackgroundCopyJob methods ***/

static HRESULT WINAPI BITS_IBackgroundCopyJob_AddFileSet(
    IBackgroundCopyJob2 *iface,
    ULONG cFileCount,
    BG_FILE_INFO *pFileSet)
{
    ULONG i;
    for (i = 0; i < cFileCount; ++i)
    {
        HRESULT hr = IBackgroundCopyJob_AddFile(iface, pFileSet[i].RemoteName,
                                                pFileSet[i].LocalName);
        if (FAILED(hr))
            return hr;
    }
    return S_OK;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_AddFile(
    IBackgroundCopyJob2 *iface,
    LPCWSTR RemoteUrl,
    LPCWSTR LocalName)
{
    BackgroundCopyJobImpl *This = (BackgroundCopyJobImpl *) iface;
    IBackgroundCopyFile *pFile;
    BackgroundCopyFileImpl *file;
    HRESULT res;

    /* We should return E_INVALIDARG in these cases.  */
    FIXME("Check for valid filenames and supported protocols\n");

    res = BackgroundCopyFileConstructor(This, RemoteUrl, LocalName, (LPVOID *) &pFile);
    if (res != S_OK)
        return res;

    /* Add a reference to the file to file list */
    IBackgroundCopyFile_AddRef(pFile);
    file = (BackgroundCopyFileImpl *) pFile;
    EnterCriticalSection(&This->cs);
    list_add_head(&This->files, &file->entryFromJob);
    This->jobProgress.BytesTotal = BG_SIZE_UNKNOWN;
    ++This->jobProgress.FilesTotal;
    LeaveCriticalSection(&This->cs);

    return S_OK;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_EnumFiles(
    IBackgroundCopyJob2 *iface,
    IEnumBackgroundCopyFiles **ppEnum)
{
    TRACE("\n");
    return EnumBackgroundCopyFilesConstructor((LPVOID *) ppEnum, iface);
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_Suspend(
    IBackgroundCopyJob2 *iface)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_Resume(
    IBackgroundCopyJob2 *iface)
{
    BackgroundCopyJobImpl *This = (BackgroundCopyJobImpl *) iface;
    HRESULT rv = S_OK;

    EnterCriticalSection(&globalMgr.cs);
    if (This->state == BG_JOB_STATE_CANCELLED
        || This->state == BG_JOB_STATE_ACKNOWLEDGED)
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

static HRESULT WINAPI BITS_IBackgroundCopyJob_Cancel(
    IBackgroundCopyJob2 *iface)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_Complete(
    IBackgroundCopyJob2 *iface)
{
    BackgroundCopyJobImpl *This = (BackgroundCopyJobImpl *) iface;
    HRESULT rv = S_OK;

    EnterCriticalSection(&This->cs);

    if (This->state == BG_JOB_STATE_CANCELLED
        || This->state == BG_JOB_STATE_ACKNOWLEDGED)
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

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetId(
    IBackgroundCopyJob2 *iface,
    GUID *pVal)
{
    BackgroundCopyJobImpl *This = (BackgroundCopyJobImpl *) iface;
    *pVal = This->jobId;
    return S_OK;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetType(
    IBackgroundCopyJob2 *iface,
    BG_JOB_TYPE *pVal)
{
    BackgroundCopyJobImpl *This = (BackgroundCopyJobImpl *) iface;

    if (!pVal)
        return E_INVALIDARG;

    *pVal = This->type;
    return S_OK;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetProgress(
    IBackgroundCopyJob2 *iface,
    BG_JOB_PROGRESS *pVal)
{
    BackgroundCopyJobImpl *This = (BackgroundCopyJobImpl *) iface;

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

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetTimes(
    IBackgroundCopyJob2 *iface,
    BG_JOB_TIMES *pVal)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetState(
    IBackgroundCopyJob2 *iface,
    BG_JOB_STATE *pVal)
{
    BackgroundCopyJobImpl *This = (BackgroundCopyJobImpl *) iface;

    if (!pVal)
        return E_INVALIDARG;

    /* Don't think we need a critical section for this */
    *pVal = This->state;
    return S_OK;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetError(
    IBackgroundCopyJob2 *iface,
    IBackgroundCopyError **ppError)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetOwner(
    IBackgroundCopyJob2 *iface,
    LPWSTR *pVal)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_SetDisplayName(
    IBackgroundCopyJob2 *iface,
    LPCWSTR Val)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetDisplayName(
    IBackgroundCopyJob2 *iface,
    LPWSTR *pVal)
{
    BackgroundCopyJobImpl *This = (BackgroundCopyJobImpl *) iface;
    int n;

    if (!pVal)
        return E_INVALIDARG;

    n = (lstrlenW(This->displayName) + 1) * sizeof **pVal;
    *pVal = CoTaskMemAlloc(n);
    if (*pVal == NULL)
        return E_OUTOFMEMORY;
    memcpy(*pVal, This->displayName, n);
    return S_OK;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_SetDescription(
    IBackgroundCopyJob2 *iface,
    LPCWSTR Val)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetDescription(
    IBackgroundCopyJob2 *iface,
    LPWSTR *pVal)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_SetPriority(
    IBackgroundCopyJob2 *iface,
    BG_JOB_PRIORITY Val)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetPriority(
    IBackgroundCopyJob2 *iface,
    BG_JOB_PRIORITY *pVal)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_SetNotifyFlags(
    IBackgroundCopyJob2 *iface,
    ULONG Val)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetNotifyFlags(
    IBackgroundCopyJob2 *iface,
    ULONG *pVal)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_SetNotifyInterface(
    IBackgroundCopyJob2 *iface,
    IUnknown *Val)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetNotifyInterface(
    IBackgroundCopyJob2 *iface,
    IUnknown **pVal)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_SetMinimumRetryDelay(
    IBackgroundCopyJob2 *iface,
    ULONG Seconds)
{
    FIXME("%u\n", Seconds);
    return S_OK;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetMinimumRetryDelay(
    IBackgroundCopyJob2 *iface,
    ULONG *Seconds)
{
    FIXME("%p\n", Seconds);
    *Seconds = 30;
    return S_OK;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_SetNoProgressTimeout(
    IBackgroundCopyJob2 *iface,
    ULONG Seconds)
{
    FIXME("%u\n", Seconds);
    return S_OK;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetNoProgressTimeout(
    IBackgroundCopyJob2 *iface,
    ULONG *Seconds)
{
    FIXME("%p\n", Seconds);
    *Seconds = 900;
    return S_OK;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetErrorCount(
    IBackgroundCopyJob2 *iface,
    ULONG *Errors)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_SetProxySettings(
    IBackgroundCopyJob2 *iface,
    BG_JOB_PROXY_USAGE ProxyUsage,
    const WCHAR *ProxyList,
    const WCHAR *ProxyBypassList)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetProxySettings(
    IBackgroundCopyJob2 *iface,
    BG_JOB_PROXY_USAGE *pProxyUsage,
    LPWSTR *pProxyList,
    LPWSTR *pProxyBypassList)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_TakeOwnership(
    IBackgroundCopyJob2 *iface)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_SetNotifyCmdLine(
    IBackgroundCopyJob2 *iface,
    LPCWSTR prog,
    LPCWSTR params)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetNotifyCmdLine(
    IBackgroundCopyJob2 *iface,
    LPWSTR *prog,
    LPWSTR *params)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetReplyProgress(
    IBackgroundCopyJob2 *iface,
    BG_JOB_REPLY_PROGRESS *progress)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetReplyData(
    IBackgroundCopyJob2 *iface,
    byte **pBuffer,
    UINT64 *pLength)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_SetReplyFileName(
    IBackgroundCopyJob2 *iface,
    LPCWSTR filename)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetReplyFileName(
    IBackgroundCopyJob2 *iface,
    LPWSTR *pFilename)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_SetCredentials(
    IBackgroundCopyJob2 *iface,
    BG_AUTH_CREDENTIALS *cred)
{
    FIXME("Not implemented\n");
    return S_OK;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_RemoveCredentials(
    IBackgroundCopyJob2 *iface,
    BG_AUTH_TARGET target,
    BG_AUTH_SCHEME scheme)
{
    FIXME("Not implemented\n");
    return S_OK;
}

static const IBackgroundCopyJob2Vtbl BITS_IBackgroundCopyJob_Vtbl =
{
    BITS_IBackgroundCopyJob_QueryInterface,
    BITS_IBackgroundCopyJob_AddRef,
    BITS_IBackgroundCopyJob_Release,
    BITS_IBackgroundCopyJob_AddFileSet,
    BITS_IBackgroundCopyJob_AddFile,
    BITS_IBackgroundCopyJob_EnumFiles,
    BITS_IBackgroundCopyJob_Suspend,
    BITS_IBackgroundCopyJob_Resume,
    BITS_IBackgroundCopyJob_Cancel,
    BITS_IBackgroundCopyJob_Complete,
    BITS_IBackgroundCopyJob_GetId,
    BITS_IBackgroundCopyJob_GetType,
    BITS_IBackgroundCopyJob_GetProgress,
    BITS_IBackgroundCopyJob_GetTimes,
    BITS_IBackgroundCopyJob_GetState,
    BITS_IBackgroundCopyJob_GetError,
    BITS_IBackgroundCopyJob_GetOwner,
    BITS_IBackgroundCopyJob_SetDisplayName,
    BITS_IBackgroundCopyJob_GetDisplayName,
    BITS_IBackgroundCopyJob_SetDescription,
    BITS_IBackgroundCopyJob_GetDescription,
    BITS_IBackgroundCopyJob_SetPriority,
    BITS_IBackgroundCopyJob_GetPriority,
    BITS_IBackgroundCopyJob_SetNotifyFlags,
    BITS_IBackgroundCopyJob_GetNotifyFlags,
    BITS_IBackgroundCopyJob_SetNotifyInterface,
    BITS_IBackgroundCopyJob_GetNotifyInterface,
    BITS_IBackgroundCopyJob_SetMinimumRetryDelay,
    BITS_IBackgroundCopyJob_GetMinimumRetryDelay,
    BITS_IBackgroundCopyJob_SetNoProgressTimeout,
    BITS_IBackgroundCopyJob_GetNoProgressTimeout,
    BITS_IBackgroundCopyJob_GetErrorCount,
    BITS_IBackgroundCopyJob_SetProxySettings,
    BITS_IBackgroundCopyJob_GetProxySettings,
    BITS_IBackgroundCopyJob_TakeOwnership,
    BITS_IBackgroundCopyJob_SetNotifyCmdLine,
    BITS_IBackgroundCopyJob_GetNotifyCmdLine,
    BITS_IBackgroundCopyJob_GetReplyProgress,
    BITS_IBackgroundCopyJob_GetReplyData,
    BITS_IBackgroundCopyJob_SetReplyFileName,
    BITS_IBackgroundCopyJob_GetReplyFileName,
    BITS_IBackgroundCopyJob_SetCredentials,
    BITS_IBackgroundCopyJob_RemoveCredentials
};

HRESULT BackgroundCopyJobConstructor(LPCWSTR displayName, BG_JOB_TYPE type,
                                     GUID *pJobId, LPVOID *ppObj)
{
    HRESULT hr;
    BackgroundCopyJobImpl *This;
    int n;

    TRACE("(%s,%d,%p)\n", debugstr_w(displayName), type, ppObj);

    This = HeapAlloc(GetProcessHeap(), 0, sizeof *This);
    if (!This)
        return E_OUTOFMEMORY;

    This->lpVtbl = &BITS_IBackgroundCopyJob_Vtbl;
    InitializeCriticalSection(&This->cs);
    This->ref = 1;
    This->type = type;

    n = (lstrlenW(displayName) + 1) *  sizeof *displayName;
    This->displayName = HeapAlloc(GetProcessHeap(), 0, n);
    if (!This->displayName)
    {
        DeleteCriticalSection(&This->cs);
        HeapFree(GetProcessHeap(), 0, This);
        return E_OUTOFMEMORY;
    }
    memcpy(This->displayName, displayName, n);

    hr = CoCreateGuid(&This->jobId);
    if (FAILED(hr))
    {
        DeleteCriticalSection(&This->cs);
        HeapFree(GetProcessHeap(), 0, This->displayName);
        HeapFree(GetProcessHeap(), 0, This);
        return hr;
    }
    *pJobId = This->jobId;

    list_init(&This->files);
    This->jobProgress.BytesTotal = 0;
    This->jobProgress.BytesTransferred = 0;
    This->jobProgress.FilesTotal = 0;
    This->jobProgress.FilesTransferred = 0;

    This->state = BG_JOB_STATE_SUSPENDED;

    *ppObj = &This->lpVtbl;
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
