/*
 *    INSENG Implementation
 *
 * Copyright 2006 Mike McCormack
 * Copyright 2016 Michael Müller
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

#define COBJMACROS


#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "rpcproxy.h"
#include "urlmon.h"
#ifdef __REACTOS__
#include <winreg.h>
#endif
#include "shlwapi.h"
#include "initguid.h"
#include "inseng.h"

#include "inseng_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(inseng);

static HINSTANCE instance;

enum thread_operation
{
    OP_DOWNLOAD,
    OP_INSTALL
};

struct thread_info
{
    DWORD operation;
    DWORD jobflags;
    IEnumCifComponents *enum_comp;

    DWORD download_size;
    DWORD install_size;

    DWORD downloaded_kb;
    ULONGLONG download_start;
};

struct InstallEngine {
    IInstallEngine2 IInstallEngine2_iface;
    IInstallEngineTiming IInstallEngineTiming_iface;
    LONG ref;

    IInstallEngineCallback *callback;
    char *baseurl;
    char *downloaddir;
    ICifFile *icif;
    DWORD status;

    /* used for the installation thread */
    struct thread_info thread;
};

struct downloadcb
{
    IBindStatusCallback IBindStatusCallback_iface;
    LONG ref;

    WCHAR *file_name;
    WCHAR *cache_file;

    char *id;
    char *display;

    DWORD dl_size;
    DWORD dl_previous_kb;

    InstallEngine *engine;
    HANDLE event_done;
    HRESULT hr;
};

static inline InstallEngine *impl_from_IInstallEngine2(IInstallEngine2 *iface)
{
    return CONTAINING_RECORD(iface, InstallEngine, IInstallEngine2_iface);
}

static inline struct downloadcb *impl_from_IBindStatusCallback(IBindStatusCallback *iface)
{
    return CONTAINING_RECORD(iface, struct downloadcb, IBindStatusCallback_iface);
}

static inline InstallEngine *impl_from_IInstallEngineTiming(IInstallEngineTiming *iface)
{
    return CONTAINING_RECORD(iface, InstallEngine, IInstallEngineTiming_iface);
}

static HRESULT WINAPI downloadcb_QueryInterface(IBindStatusCallback *iface, REFIID riid, void **ppv)
{
    struct downloadcb *This = impl_from_IBindStatusCallback(iface);

    if (IsEqualGUID(&IID_IUnknown, riid))
    {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IBindStatusCallback_iface;
    }
    else if (IsEqualGUID(&IID_IBindStatusCallback, riid))
    {
        TRACE("(%p)->(IID_IBindStatusCallback %p)\n", This, ppv);
        *ppv = &This->IBindStatusCallback_iface;
    }
    else
    {
        FIXME("(%p)->(%s %p) not found\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);
    return S_OK;
}

static ULONG WINAPI downloadcb_AddRef(IBindStatusCallback *iface)
{
    struct downloadcb *This = impl_from_IBindStatusCallback(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref = %d\n", This, ref);

    return ref;
}

static ULONG WINAPI downloadcb_Release(IBindStatusCallback *iface)
{
    struct downloadcb *This = impl_from_IBindStatusCallback(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref = %d\n", This, ref);

    if (!ref)
    {
        heap_free(This->file_name);
        heap_free(This->cache_file);

        IInstallEngine2_Release(&This->engine->IInstallEngine2_iface);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI downloadcb_OnStartBinding(IBindStatusCallback *iface, DWORD reserved, IBinding *pbind)
{
    struct downloadcb *This = impl_from_IBindStatusCallback(iface);

    TRACE("(%p)->(%u %p)\n", This, reserved, pbind);

    return S_OK;
}

static HRESULT WINAPI downloadcb_GetPriority(IBindStatusCallback *iface, LONG *priority)
{
    struct downloadcb *This = impl_from_IBindStatusCallback(iface);

    FIXME("(%p)->(%p): stub\n", This, priority);

    return E_NOTIMPL;
}

static HRESULT WINAPI downloadcb_OnLowResource(IBindStatusCallback *iface, DWORD reserved)
{
    struct downloadcb *This = impl_from_IBindStatusCallback(iface);

    FIXME("(%p)->(%u): stub\n", This, reserved);

    return E_NOTIMPL;
}

static HRESULT WINAPI downloadcb_OnProgress(IBindStatusCallback *iface, ULONG progress,
        ULONG progress_max, ULONG status, const WCHAR *status_text)
{
    struct downloadcb *This = impl_from_IBindStatusCallback(iface);
    HRESULT hr = S_OK;

    TRACE("%p)->(%u %u %u %s)\n", This, progress, progress_max, status, debugstr_w(status_text));

    switch(status)
    {
        case BINDSTATUS_BEGINDOWNLOADDATA:
            if (!This->engine->thread.download_start)
                This->engine->thread.download_start = GetTickCount64();
            /* fall-through */
        case BINDSTATUS_DOWNLOADINGDATA:
        case BINDSTATUS_ENDDOWNLOADDATA:
            This->engine->thread.downloaded_kb = This->dl_previous_kb + progress / 1024;
            if (This->engine->callback)
            {
                hr = IInstallEngineCallback_OnComponentProgress(This->engine->callback,
                         This->id, INSTALLSTATUS_DOWNLOADING, This->display, NULL, progress / 1024, This->dl_size);
            }
            break;

        case BINDSTATUS_CACHEFILENAMEAVAILABLE:
            This->cache_file = strdupW(status_text);
            if (!This->cache_file)
            {
                ERR("Failed to allocate memory for cache file\n");
                hr = E_OUTOFMEMORY;
            }
            break;

        case BINDSTATUS_CONNECTING:
        case BINDSTATUS_SENDINGREQUEST:
        case BINDSTATUS_MIMETYPEAVAILABLE:
        case BINDSTATUS_FINDINGRESOURCE:
            break;

        default:
            FIXME("Unsupported status %u\n", status);
    }

    return hr;
}

static HRESULT WINAPI downloadcb_OnStopBinding(IBindStatusCallback *iface, HRESULT hresult, LPCWSTR szError)
{
    struct downloadcb *This = impl_from_IBindStatusCallback(iface);

    TRACE("(%p)->(%08x %s)\n", This, hresult, debugstr_w(szError));

    if (FAILED(hresult))
    {
        This->hr = hresult;
        goto done;
    }

    if (!This->cache_file)
    {
        This->hr = E_FAIL;
        goto done;
    }

    if (CopyFileW(This->cache_file, This->file_name, FALSE))
        This->hr = S_OK;
    else
    {
        ERR("CopyFile failed: %u\n", GetLastError());
        This->hr = E_FAIL;
    }

done:
    SetEvent(This->event_done);
    return S_OK;
}

static HRESULT WINAPI downloadcb_GetBindInfo(IBindStatusCallback *iface,
        DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    struct downloadcb *This = impl_from_IBindStatusCallback(iface);

    TRACE("(%p)->(%p %p)\n", This, grfBINDF, pbindinfo);

    *grfBINDF = BINDF_PULLDATA | BINDF_NEEDFILE;
    return S_OK;
}

static HRESULT WINAPI downloadcb_OnDataAvailable(IBindStatusCallback *iface,
        DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed)
{
    struct downloadcb *This = impl_from_IBindStatusCallback(iface);

    TRACE("(%p)->(%08x %u %p %p)\n", This, grfBSCF, dwSize, pformatetc, pstgmed);

    return S_OK;
}

static HRESULT WINAPI downloadcb_OnObjectAvailable(IBindStatusCallback *iface,
        REFIID riid, IUnknown *punk)
{
    struct downloadcb *This = impl_from_IBindStatusCallback(iface);

    FIXME("(%p)->(%s %p): stub\n", This, debugstr_guid(riid), punk);

    return E_NOTIMPL;
}

static const IBindStatusCallbackVtbl BindStatusCallbackVtbl =
{
    downloadcb_QueryInterface,
    downloadcb_AddRef,
    downloadcb_Release,
    downloadcb_OnStartBinding,
    downloadcb_GetPriority,
    downloadcb_OnLowResource,
    downloadcb_OnProgress,
    downloadcb_OnStopBinding,
    downloadcb_GetBindInfo,
    downloadcb_OnDataAvailable,
    downloadcb_OnObjectAvailable
};

static HRESULT downloadcb_create(InstallEngine *engine, HANDLE event, char *file_name, char *id,
                                 char *display, DWORD dl_size, struct downloadcb **callback)
{
    struct downloadcb *cb;

    cb = heap_alloc_zero(sizeof(*cb));
    if (!cb) return E_OUTOFMEMORY;

    cb->IBindStatusCallback_iface.lpVtbl = &BindStatusCallbackVtbl;
    cb->ref = 1;
    cb->hr = E_FAIL;
    cb->id = id;
    cb->display = display;
    cb->engine = engine;
    cb->dl_size = dl_size;
    cb->dl_previous_kb = engine->thread.downloaded_kb;
    cb->event_done = event;
    cb->file_name = strAtoW(file_name);
    if (!cb->file_name)
    {
        heap_free(cb);
        return E_OUTOFMEMORY;
    }

    IInstallEngine2_AddRef(&engine->IInstallEngine2_iface);

    *callback = cb;
    return S_OK;
}

static HRESULT WINAPI InstallEngine_QueryInterface(IInstallEngine2 *iface, REFIID riid, void **ppv)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IInstallEngine2_iface;
    }else if(IsEqualGUID(&IID_IInstallEngine, riid)) {
        TRACE("(%p)->(IID_IInstallEngine %p)\n", This, ppv);
        *ppv = &This->IInstallEngine2_iface;
    }else if(IsEqualGUID(&IID_IInstallEngine2, riid)) {
        TRACE("(%p)->(IID_IInstallEngine2 %p)\n", This, ppv);
        *ppv = &This->IInstallEngine2_iface;
    }else if(IsEqualGUID(&IID_IInstallEngineTiming, riid)) {
        TRACE("(%p)->(IID_IInstallEngineTiming %p)\n", This, ppv);
        *ppv = &This->IInstallEngineTiming_iface;
    }else {
        FIXME("(%p)->(%s %p) not found\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);
    return S_OK;
}

static ULONG WINAPI InstallEngine_AddRef(IInstallEngine2 *iface)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI InstallEngine_Release(IInstallEngine2 *iface)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if (!ref)
    {
        if (This->icif)
            ICifFile_Release(This->icif);

        heap_free(This->baseurl);
        heap_free(This->downloaddir);
        heap_free(This);
    }

    return ref;
}

static void set_status(InstallEngine *This, DWORD status)
{
    This->status = status;

    if (This->callback)
        IInstallEngineCallback_OnEngineStatusChange(This->callback, status, 0);
}

static HRESULT calc_sizes(IEnumCifComponents *enum_comp, DWORD operation, DWORD *size_download, DWORD *size_install)
{
    ICifComponent *comp;
    DWORD download = 0;
    DWORD install = 0;
    HRESULT hr;

    /* FIXME: what about inactive dependencies and how does
     * INSTALLOPTIONS_FORCEDEPENDENCIES play into this ?*/

    hr = IEnumCifComponents_Reset(enum_comp);
    if (FAILED(hr)) return hr;

    while (SUCCEEDED(IEnumCifComponents_Next(enum_comp, &comp)))
    {
        if (ICifComponent_GetInstallQueueState(comp) != ActionInstall)
            continue;

        /* FIXME: handle install options and find out the default options*/
        if (operation == OP_DOWNLOAD && ICifComponent_IsComponentDownloaded(comp) == S_FALSE)
            download = ICifComponent_GetDownloadSize(comp);
        /*
        if (operation == OP_INSTALL && ICifComponent_IsComponentInstalled(comp) == S_FALSE)
            install = ICifComponent_GetInstalledSize(comp);
        */
    }

    *size_download = download;
    *size_install = install;

    return S_OK;
}

static HRESULT get_next_component(IEnumCifComponents *enum_comp, DWORD operation, ICifComponent **ret_comp)
{
    ICifComponent *comp;
    HRESULT hr;

    hr = IEnumCifComponents_Reset(enum_comp);
    if (FAILED(hr)) return hr;

    while (SUCCEEDED(IEnumCifComponents_Next(enum_comp, &comp)))
    {
        if (ICifComponent_GetInstallQueueState(comp) != ActionInstall)
            continue;

        /* FIXME: handle install options and find out the default options*/
        if (operation == OP_DOWNLOAD && ICifComponent_IsComponentDownloaded(comp) != S_FALSE)
            continue;
        if (operation == OP_INSTALL && ICifComponent_IsComponentInstalled(comp) != S_FALSE)
            continue;

        *ret_comp = comp;
        return S_OK;
    }

    return S_FALSE;
}

static HRESULT get_url(ICifComponent *comp, int index, char **url, DWORD *flags)
{
    char *url_temp = NULL;
    int size = MAX_PATH / 2;
    HRESULT hr;

    /* FIXME: should we add an internal get function to prevent this ugly code ? */

    /* check if there is an url with such an index */
    hr = ICifComponent_GetUrl(comp, index, NULL, 0, flags);
    if (FAILED(hr))
    {
        *url = NULL;
        *flags = 0;
        return S_OK;
    }

    do
    {
        size *= 2;
        heap_free(url_temp);
        url_temp = heap_alloc(size);
        if (!url_temp) return E_OUTOFMEMORY;

        hr = ICifComponent_GetUrl(comp, index, url_temp, size, flags);
        if (FAILED(hr))
        {
            heap_free(url_temp);
            return hr;
        }
    }
    while (strlen(url_temp) == size-1);

    *url = url_temp;
    return S_OK;
}

static char *combine_url(char *baseurl, char *url)
{
    int len_base = strlen(baseurl);
    int len_url = strlen(url);
    char *combined;

    combined = heap_alloc(len_base + len_url + 2);
    if (!combined) return NULL;

    strcpy(combined, baseurl);
    if (len_base && combined[len_base-1] != '/')
        strcat(combined, "/");
    strcat(combined, url);

    return combined;
}

static HRESULT generate_moniker(char *baseurl, char *url, DWORD flags, IMoniker **moniker)
{
    WCHAR *urlW;
    HRESULT hr;

    if (flags & URLF_RELATIVEURL)
    {
        char *combined;
        if (!baseurl)
            return E_FAIL;

        combined = combine_url(baseurl, url);
        if (!combined) return E_OUTOFMEMORY;

        urlW = strAtoW(combined);
        heap_free(combined);
        if (!urlW) return E_OUTOFMEMORY;
    }
    else
    {
        urlW = strAtoW(url);
        if (!urlW) return E_OUTOFMEMORY;
    }

    hr = CreateURLMoniker(NULL, urlW, moniker);
    heap_free(urlW);
    return hr;
}

static char *merge_path(char *path1, char *path2)
{
    int len = strlen(path1) + strlen(path2) + 2;
    char *combined = heap_alloc(len);

    if (!combined) return NULL;
    strcpy(combined, path1);
    strcat(combined, "\\");
    strcat(combined, path2);

    return combined;
}

static HRESULT download_url(InstallEngine *This, char *id, char *display, char *url, DWORD flags, DWORD dl_size)
{
    struct downloadcb *callback = NULL;
    char *filename    = NULL;
    IUnknown *unk     = NULL;
    IMoniker *mon     = NULL;
    IBindCtx *bindctx = NULL;
    HANDLE event      = NULL;
    HRESULT hr;

    if (!This->downloaddir)
    {
        WARN("No download directory set\n");
        return E_FAIL;
    }

    hr = generate_moniker(This->baseurl, url, flags, &mon);
    if (FAILED(hr))
    {
        FIXME("Failed to create moniker\n");
        return hr;
    }

    event = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!event)
    {
        IMoniker_Release(mon);
        return E_FAIL;
    }

    filename = strrchr(url, '/');
    if (!filename) filename = url;

    filename = merge_path(This->downloaddir, filename);
    if (!filename)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    hr = downloadcb_create(This, event, filename, id, display, dl_size, &callback);
    if (FAILED(hr)) goto error;

    hr = CreateAsyncBindCtx(0, &callback->IBindStatusCallback_iface, NULL, &bindctx);
    if(FAILED(hr)) goto error;

    hr = IMoniker_BindToStorage(mon, bindctx, NULL, &IID_IUnknown, (void**)&unk);
    if (FAILED(hr)) goto error;
    if (unk) IUnknown_Release(unk);

    heap_free(filename);
    IMoniker_Release(mon);
    IBindCtx_Release(bindctx);

    WaitForSingleObject(event, INFINITE);
    hr = callback->hr;

    CloseHandle(event);
    IBindStatusCallback_Release(&callback->IBindStatusCallback_iface);
    return hr;

error:
    if (mon) IMoniker_Release(mon);
    if (event) CloseHandle(event);
    if (callback) IBindStatusCallback_Release(&callback->IBindStatusCallback_iface);
    if (bindctx) IBindCtx_Release(bindctx);
    if (filename) heap_free(filename);
    return hr;
}

static HRESULT process_component_dependencies(InstallEngine *This, ICifComponent *comp)
{
    char id[MAX_ID_LENGTH+1], type;
    DWORD ver, build;
    HRESULT hr;
    int i;

    for (i = 0;; i++)
    {
        hr = ICifComponent_GetDependency(comp, i, id, sizeof(id), &type, &ver, &build);
        if (SUCCEEDED(hr))
            FIXME("Can't handle dependencies yet: %s\n", debugstr_a(id));
        else
            break;
    }

    return S_OK;
}

static HRESULT process_component(InstallEngine *This, ICifComponent *comp)
{
    DWORD size_dl, size_install, phase;
    char display[MAX_DISPLAYNAME_LENGTH+1];
    char id[MAX_ID_LENGTH+1];
    HRESULT hr;
    int i;

    hr = ICifComponent_GetID(comp, id, sizeof(id));
    if (FAILED(hr)) return hr;

    TRACE("processing component %s\n", debugstr_a(id));

    hr = ICifComponent_GetDescription(comp, display, sizeof(display));
    if (FAILED(hr)) return hr;

    size_dl      = (This->thread.operation == OP_DOWNLOAD) ? ICifComponent_GetDownloadSize(comp) : 0;
    size_install = 0; /* (This->thread.operation == OP_INSTALL) ? ICifComponent_GetInstalledSize(comp) : 0; */

    if (This->callback)
    {
        IInstallEngineCallback_OnStartComponent(This->callback, id, size_dl, size_install, display);
        IInstallEngineCallback_OnComponentProgress(This->callback, id, INSTALLSTATUS_INITIALIZING, display, NULL, 0, 0);
        phase = INSTALLSTATUS_INITIALIZING;
    }

    hr = process_component_dependencies(This, comp);
    if (FAILED(hr)) return hr;

    if (This->thread.operation == OP_DOWNLOAD)
    {
        for (i = 0;; i++)
        {
            DWORD flags;
            char *url;

            phase = INSTALLSTATUS_DOWNLOADING;

            hr = get_url(comp, i, &url, &flags);
            if (FAILED(hr)) goto done;
            if (!url) break;

            TRACE("processing url %s\n", debugstr_a(url));

            hr = download_url(This, id, display, url, flags, size_dl);
            heap_free(url);
            if (FAILED(hr))
            {
                DWORD retry = 0;

                if (This->callback)
                    IInstallEngineCallback_OnEngineProblem(This->callback, ENGINEPROBLEM_DOWNLOADFAIL, &retry);
                if (!retry) goto done;

                i--;
                continue;
            }

            phase = INSTALLSTATUS_CHECKINGTRUST;
            /* FIXME: check trust */
            IInstallEngineCallback_OnComponentProgress(This->callback, id, INSTALLSTATUS_CHECKINGTRUST, display, NULL, 0, 0);
        }

        component_set_downloaded(comp, TRUE);
        phase = INSTALLSTATUS_DOWNLOADFINISHED;
    }
    else
        FIXME("Installation not yet implemented\n");

done:
    IInstallEngineCallback_OnStopComponent(This->callback, id, hr, phase, display, 0);
    return hr;
}

DWORD WINAPI thread_installation(LPVOID param)
{
    InstallEngine *This = param;
    ICifComponent *comp;
    HRESULT hr;

    if (This->callback)
        IInstallEngineCallback_OnStartInstall(This->callback, This->thread.download_size, This->thread.install_size);

    for (;;)
    {
        hr = get_next_component(This->thread.enum_comp, This->thread.operation, &comp);
        if (FAILED(hr)) break;
        if (hr == S_FALSE)
        {
            hr = S_OK;
            break;
        }

        hr = process_component(This, comp);
        if (FAILED(hr)) break;
    }

    if (This->callback)
        IInstallEngineCallback_OnStopInstall(This->callback, hr, NULL, 0);

    IEnumCifComponents_Release(This->thread.enum_comp);
    IInstallEngine2_Release(&This->IInstallEngine2_iface);

    set_status(This, ENGINESTATUS_READY);
    return 0;
}

static HRESULT start_installation(InstallEngine *This, DWORD operation, DWORD jobflags)
{
    HANDLE thread;
    HRESULT hr;

    This->thread.operation = operation;
    This->thread.jobflags  = jobflags;
    This->thread.downloaded_kb = 0;
    This->thread.download_start = 0;

    /* Windows sends the OnStartInstall event from a different thread,
     * but OnStartInstall already contains the required download and install size.
     * The only way to signal an error from the thread is to send an OnStopComponent /
     * OnStopInstall signal which can only occur after OnStartInstall. We need to
     * precompute the sizes here to be able inform the application about errors while
     * calculating the required sizes. */

    hr = ICifFile_EnumComponents(This->icif, &This->thread.enum_comp, 0, NULL);
    if (FAILED(hr)) return hr;

    hr = calc_sizes(This->thread.enum_comp, operation, &This->thread.download_size, &This->thread.install_size);
    if (FAILED(hr)) goto error;

    IInstallEngine2_AddRef(&This->IInstallEngine2_iface);

    thread = CreateThread(NULL, 0, thread_installation, This, 0, NULL);
    if (!thread)
    {
        IInstallEngine2_Release(&This->IInstallEngine2_iface);
        hr = E_FAIL;
        goto error;
    }

    CloseHandle(thread);
    return S_OK;

error:
    IEnumCifComponents_Release(This->thread.enum_comp);
    return hr;
}

static HRESULT WINAPI InstallEngine_GetEngineStatus(IInstallEngine2 *iface, DWORD *status)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);

    TRACE("(%p)->(%p)\n", This, status);

    if (!status)
        return E_FAIL;

    *status = This->status;
    return S_OK;
}

static HRESULT WINAPI InstallEngine_SetCifFile(IInstallEngine2 *iface, const char *cab_name, const char *cif_name)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);

    FIXME("(%p)->(%s %s): stub\n", This, debugstr_a(cab_name), debugstr_a(cif_name));

    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_DownloadComponents(IInstallEngine2 *iface, DWORD flags)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);

    TRACE("(%p)->(%x)\n", This, flags);

    /* The interface is not really threadsafe on windows, but we can at least prevent multiple installations */
    if (InterlockedCompareExchange((LONG *)&This->status, ENGINESTATUS_INSTALLING, ENGINESTATUS_READY) != ENGINESTATUS_READY)
        return E_FAIL;

    if (This->callback)
        IInstallEngineCallback_OnEngineStatusChange(This->callback, ENGINESTATUS_INSTALLING, 0);

    return start_installation(This, OP_DOWNLOAD, flags);
}

static HRESULT WINAPI InstallEngine_InstallComponents(IInstallEngine2 *iface, DWORD flags)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);

    FIXME("(%p)->(%x): stub\n", This, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_EnumInstallIDs(IInstallEngine2 *iface, UINT index, char **id)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);

    FIXME("(%p)->(%u %p): stub\n", This, index, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_EnumDownloadIDs(IInstallEngine2 *iface, UINT index, char **id)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    IEnumCifComponents *enum_components;
    ICifComponent *comp;
    HRESULT hr;

    TRACE("(%p)->(%u %p)\n", This, index, id);

    if (!This->icif || !id)
        return E_FAIL;

    hr = ICifFile_EnumComponents(This->icif, &enum_components, 0, NULL);
    if (FAILED(hr)) return hr;

    for (;;)
    {
        hr = IEnumCifComponents_Next(enum_components, &comp);
        if (FAILED(hr)) goto done;

        if (ICifComponent_GetInstallQueueState(comp) != ActionInstall)
            continue;

        if (ICifComponent_IsComponentDownloaded(comp) != S_FALSE)
            continue;

        if (index == 0)
        {
            char *id_src = component_get_id(comp);
            *id = CoTaskMemAlloc(strlen(id_src) + 1);

            if (*id)
                strcpy(*id, id_src);
            else
                hr = E_OUTOFMEMORY;
            goto done;
        }

        index--;
    }

done:
    IEnumCifComponents_Release(enum_components);
    return hr;
}

static HRESULT WINAPI InstallEngine_IsComponentInstalled(IInstallEngine2 *iface, const char *id, DWORD *status)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);

    FIXME("(%p)->(%s %p): stub\n", This, debugstr_a(id), status);

    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_RegisterInstallEngineCallback(IInstallEngine2 *iface, IInstallEngineCallback *callback)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);

    TRACE("(%p)->(%p)\n", This, callback);

    This->callback = callback;
    return S_OK;
}

static HRESULT WINAPI InstallEngine_UnregisterInstallEngineCallback(IInstallEngine2 *iface)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);

    TRACE("(%p)\n", This);

    This->callback = NULL;
    return S_OK;
}

static HRESULT WINAPI InstallEngine_SetAction(IInstallEngine2 *iface, const char *id, DWORD action, DWORD priority)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    ICifComponent *comp;
    HRESULT hr;

    TRACE("(%p)->(%s %u %u)\n", This, debugstr_a(id), action, priority);

    if (!This->icif)
        return E_FAIL; /* FIXME: check error code */

    hr = ICifFile_FindComponent(This->icif, id, &comp);
    if (FAILED(hr)) return hr;

    hr = ICifComponent_SetInstallQueueState(comp, action);
    if (FAILED(hr)) return hr;

    hr = ICifComponent_SetCurrentPriority(comp, priority);
    return hr;
}

static HRESULT WINAPI InstallEngine_GetSizes(IInstallEngine2 *iface, const char *id, COMPONENT_SIZES *sizes)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);

    FIXME("(%p)->(%s %p): stub\n", This, debugstr_a(id), sizes);

    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_LaunchExtraCommand(IInstallEngine2 *iface, const char *inf_name, const char *section)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);

    FIXME("(%p)->(%s %s): stub\n", This, debugstr_a(inf_name), debugstr_a(section));

    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_GetDisplayName(IInstallEngine2 *iface, const char *id, const char *name)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);

    FIXME("(%p)->(%s %s): stub\n", This, debugstr_a(id), debugstr_a(name));

    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_SetBaseUrl(IInstallEngine2 *iface, const char *base_name)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_a(base_name));

    if (This->baseurl)
        heap_free(This->baseurl);

    This->baseurl = strdupA(base_name);
    return This->baseurl ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI InstallEngine_SetDownloadDir(IInstallEngine2 *iface, const char *download_dir)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_a(download_dir));

    if (This->downloaddir)
        heap_free(This->downloaddir);

    This->downloaddir = strdupA(download_dir);
    return This->downloaddir ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI InstallEngine_SetInstallDrive(IInstallEngine2 *iface, char drive)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);

    FIXME("(%p)->(%c): stub\n", This, drive);

    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_SetInstallOptions(IInstallEngine2 *iface, DWORD flags)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);

    FIXME("(%p)->(%x): stub\n", This, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_SetHWND(IInstallEngine2 *iface, HWND hwnd)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);

    FIXME("(%p)->(%p): stub\n", This, hwnd);

    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_SetIStream(IInstallEngine2 *iface, IStream *stream)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);

    FIXME("(%p)->(%p): stub\n", This, stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_Abort(IInstallEngine2 *iface, DWORD flags)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);

    FIXME("(%p)->(%x): stub\n", This, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_Suspend(IInstallEngine2 *iface)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);

    FIXME("(%p): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_Resume(IInstallEngine2 *iface)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);

    FIXME("(%p): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine2_SetLocalCif(IInstallEngine2 *iface, const char *cif)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    HRESULT hr;

    TRACE("(%p)->(%s)\n", This, debugstr_a(cif));

    if (This->icif)
        ICifFile_Release(This->icif);

    set_status(This, ENGINESTATUS_LOADING);

    hr = GetICifFileFromFile(&This->icif, cif);
    if (SUCCEEDED(hr))
        set_status(This, ENGINESTATUS_READY);
    else
    {
        This->icif = NULL;
        set_status(This, ENGINESTATUS_NOTREADY);
    }
    return hr;
}

static HRESULT WINAPI InstallEngine2_GetICifFile(IInstallEngine2 *iface, ICifFile **cif_file)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);

    TRACE("(%p)->(%p)\n", This, cif_file);

    if (!This->icif || !cif_file)
        return E_FAIL;

    ICifFile_AddRef(This->icif);
    *cif_file = This->icif;
    return S_OK;
}

static const IInstallEngine2Vtbl InstallEngine2Vtbl =
{
    InstallEngine_QueryInterface,
    InstallEngine_AddRef,
    InstallEngine_Release,
    InstallEngine_GetEngineStatus,
    InstallEngine_SetCifFile,
    InstallEngine_DownloadComponents,
    InstallEngine_InstallComponents,
    InstallEngine_EnumInstallIDs,
    InstallEngine_EnumDownloadIDs,
    InstallEngine_IsComponentInstalled,
    InstallEngine_RegisterInstallEngineCallback,
    InstallEngine_UnregisterInstallEngineCallback,
    InstallEngine_SetAction,
    InstallEngine_GetSizes,
    InstallEngine_LaunchExtraCommand,
    InstallEngine_GetDisplayName,
    InstallEngine_SetBaseUrl,
    InstallEngine_SetDownloadDir,
    InstallEngine_SetInstallDrive,
    InstallEngine_SetInstallOptions,
    InstallEngine_SetHWND,
    InstallEngine_SetIStream,
    InstallEngine_Abort,
    InstallEngine_Suspend,
    InstallEngine_Resume,
    InstallEngine2_SetLocalCif,
    InstallEngine2_GetICifFile
};

static HRESULT WINAPI InstallEngineTiming_QueryInterface(IInstallEngineTiming *iface, REFIID riid, void **ppv)
{
    InstallEngine *This = impl_from_IInstallEngineTiming(iface);
    return IInstallEngine2_QueryInterface(&This->IInstallEngine2_iface, riid, ppv);
}

static ULONG WINAPI InstallEngineTiming_AddRef(IInstallEngineTiming *iface)
{
    InstallEngine *This = impl_from_IInstallEngineTiming(iface);
    return IInstallEngine2_AddRef(&This->IInstallEngine2_iface);
}

static ULONG WINAPI InstallEngineTiming_Release(IInstallEngineTiming *iface)
{
    InstallEngine *This = impl_from_IInstallEngineTiming(iface);
    return IInstallEngine2_Release(&This->IInstallEngine2_iface);
}

static HRESULT WINAPI InstallEngineTiming_GetRates(IInstallEngineTiming *iface, DWORD *download, DWORD *install)
{
    InstallEngine *This = impl_from_IInstallEngineTiming(iface);

    FIXME("(%p)->(%p, %p): stub\n", This, download, install);

    *download = 0;
    *install = 0;

    return S_OK;
}

static HRESULT WINAPI InstallEngineTiming_GetInstallProgress(IInstallEngineTiming *iface, INSTALLPROGRESS *progress)
{
    InstallEngine *This = impl_from_IInstallEngineTiming(iface);
    ULONGLONG elapsed;
    static int once;

    if (!once)
        FIXME("(%p)->(%p): semi-stub\n", This, progress);
    else
        TRACE("(%p)->(%p): semi-stub\n", This, progress);

    progress->dwDownloadKBRemaining = max(This->thread.download_size, This->thread.downloaded_kb) - This->thread.downloaded_kb;

    elapsed = GetTickCount64() - This->thread.download_start;
    if (This->thread.download_start && This->thread.downloaded_kb && elapsed > 100)
        progress->dwDownloadSecsRemaining = (progress->dwDownloadKBRemaining * elapsed) / (This->thread.downloaded_kb * 1000);
    else
        progress->dwDownloadSecsRemaining = -1;

    progress->dwInstallKBRemaining = 0;
    progress->dwInstallSecsRemaining = -1;

    return S_OK;
}

static const IInstallEngineTimingVtbl InstallEngineTimingVtbl =
{
    InstallEngineTiming_QueryInterface,
    InstallEngineTiming_AddRef,
    InstallEngineTiming_Release,
    InstallEngineTiming_GetRates,
    InstallEngineTiming_GetInstallProgress,
};

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IClassFactory, riid)) {
        TRACE("(%p)->(IID_IClassFactory %p)\n", iface, ppv);
        *ppv = iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    FIXME("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    return S_OK;
}

static HRESULT WINAPI InstallEngineCF_CreateInstance(IClassFactory *iface, IUnknown *outer,
        REFIID riid, void **ppv)
{
    InstallEngine *engine;
    HRESULT hres;

    TRACE("(%p %s %p)\n", outer, debugstr_guid(riid), ppv);

    engine = heap_alloc_zero(sizeof(*engine));
    if(!engine)
        return E_OUTOFMEMORY;

    engine->IInstallEngine2_iface.lpVtbl = &InstallEngine2Vtbl;
    engine->IInstallEngineTiming_iface.lpVtbl = &InstallEngineTimingVtbl;
    engine->ref = 1;
    engine->status = ENGINESTATUS_NOTREADY;

    hres = IInstallEngine2_QueryInterface(&engine->IInstallEngine2_iface, riid, ppv);
    IInstallEngine2_Release(&engine->IInstallEngine2_iface);
    return hres;
}

static const IClassFactoryVtbl InstallEngineCFVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    InstallEngineCF_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory InstallEngineCF = { &InstallEngineCFVtbl };

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch(fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        instance = hInstDLL;
        DisableThreadLibraryCalls(hInstDLL);
        break;
    }
    return TRUE;
}

/***********************************************************************
 *             DllGetClassObject (INSENG.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    if(IsEqualGUID(rclsid, &CLSID_InstallEngine)) {
        TRACE("(CLSID_InstallEngine %s %p)\n", debugstr_guid(iid), ppv);
        return IClassFactory_QueryInterface(&InstallEngineCF, iid, ppv);
    }

    FIXME("(%s %s %p)\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *              DllCanUnloadNow (INSENG.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

/***********************************************************************
 *		DllRegisterServer (INSENG.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( instance );
}

/***********************************************************************
 *		DllUnregisterServer (INSENG.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( instance );
}

BOOL WINAPI CheckTrustEx( LPVOID a, LPVOID b, LPVOID c, LPVOID d, LPVOID e )
{
    FIXME("%p %p %p %p %p\n", a, b, c, d, e );
    return TRUE;
}

/***********************************************************************
 *  DllInstall (INSENG.@)
 */
HRESULT WINAPI DllInstall(BOOL bInstall, LPCWSTR cmdline)
{
    FIXME("(%s, %s): stub\n", bInstall ? "TRUE" : "FALSE", debugstr_w(cmdline));
    return S_OK;
}
