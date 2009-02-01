/*
 * Copyright 2005-2007 Jacek Caban for CodeWeavers
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

#include "urlmon_main.h"
#include "winreg.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

static WCHAR cbinding_contextW[] = {'C','B','i','n','d','i','n','g',' ','C','o','n','t','e','x','t',0};
static WCHAR bscb_holderW[] = { '_','B','S','C','B','_','H','o','l','d','e','r','_',0 };

typedef struct Binding Binding;

struct _task_header_t;

typedef void (*task_proc_t)(Binding*, struct _task_header_t*);

typedef struct _task_header_t {
    task_proc_t proc;
    struct _task_header_t *next;
} task_header_t;

typedef struct {
    const IUnknownVtbl *lpUnknownVtbl;

    LONG ref;

    IInternetProtocol *protocol;

    BYTE buf[1024*8];
    DWORD size;
    BOOL init;
    HRESULT hres;

    LPWSTR cache_file;
} stgmed_buf_t;

typedef struct _stgmed_obj_t stgmed_obj_t;

typedef struct {
    void (*release)(stgmed_obj_t*);
    HRESULT (*fill_stgmed)(stgmed_obj_t*,STGMEDIUM*);
    void *(*get_result)(stgmed_obj_t*);
} stgmed_obj_vtbl;

struct _stgmed_obj_t {
    const stgmed_obj_vtbl *vtbl;
};

#define STGMEDUNK(x)  ((IUnknown*) &(x)->lpUnknownVtbl)

typedef enum {
    BEFORE_DOWNLOAD,
    DOWNLOADING,
    END_DOWNLOAD
} download_state_t;

#define BINDING_LOCKED    0x0001
#define BINDING_STOPPED   0x0002
#define BINDING_OBJAVAIL  0x0004

struct Binding {
    const IBindingVtbl               *lpBindingVtbl;
    const IInternetProtocolSinkVtbl  *lpInternetProtocolSinkVtbl;
    const IInternetBindInfoVtbl      *lpInternetBindInfoVtbl;
    const IServiceProviderVtbl       *lpServiceProviderVtbl;

    LONG ref;

    IBindStatusCallback *callback;
    IInternetProtocol *protocol;
    IServiceProvider *service_provider;

    stgmed_buf_t *stgmed_buf;
    stgmed_obj_t *stgmed_obj;

    BINDINFO bindinfo;
    DWORD bindf;
    BOOL to_object;
    LPWSTR mime;
    UINT clipboard_format;
    LPWSTR url;
    IID iid;
    BOOL report_mime;
    DWORD continue_call;
    DWORD state;
    HRESULT hres;
    download_state_t download_state;
    IUnknown *obj;
    IMoniker *mon;
    IBindCtx *bctx;

    DWORD apartment_thread;
    HWND notif_hwnd;

    task_header_t *task_queue_head, *task_queue_tail;
    CRITICAL_SECTION section;
};

#define BINDING(x)   ((IBinding*)               &(x)->lpBindingVtbl)
#define PROTSINK(x)  ((IInternetProtocolSink*)  &(x)->lpInternetProtocolSinkVtbl)
#define BINDINF(x)   ((IInternetBindInfo*)      &(x)->lpInternetBindInfoVtbl)
#define SERVPROV(x)  ((IServiceProvider*)       &(x)->lpServiceProviderVtbl)

#define STREAM(x) ((IStream*) &(x)->lpStreamVtbl)
#define HTTPNEG2(x) ((IHttpNegotiate2*) &(x)->lpHttpNegotiate2Vtbl)

#define WM_MK_CONTINUE   (WM_USER+101)

static void push_task(Binding *binding, task_header_t *task, task_proc_t proc)
{
    task->proc = proc;
    task->next = NULL;

    EnterCriticalSection(&binding->section);

    if(binding->task_queue_tail) {
        binding->task_queue_tail->next = task;
        binding->task_queue_tail = task;
    }else {
        binding->task_queue_tail = binding->task_queue_head = task;
    }

    LeaveCriticalSection(&binding->section);
}

static task_header_t *pop_task(Binding *binding)
{
    task_header_t *ret;

    EnterCriticalSection(&binding->section);

    ret = binding->task_queue_head;
    if(ret) {
        binding->task_queue_head = ret->next;
        if(!binding->task_queue_head)
            binding->task_queue_tail = NULL;
    }

    LeaveCriticalSection(&binding->section);

    return ret;
}

static void fill_stgmed_buffer(stgmed_buf_t *buf)
{
    DWORD read = 0;

    if(sizeof(buf->buf) == buf->size)
        return;

    buf->hres = IInternetProtocol_Read(buf->protocol, buf->buf+buf->size,
            sizeof(buf->buf)-buf->size, &read);
    buf->size += read;
    if(read > 0)
        buf->init = TRUE;
}

static LRESULT WINAPI notif_wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if(msg == WM_MK_CONTINUE) {
        Binding *binding = (Binding*)lParam;
        task_header_t *task;

        while((task = pop_task(binding))) {
            binding->continue_call++;
            task->proc(binding, task);
            binding->continue_call--;
        }

        IBinding_Release(BINDING(binding));
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static HWND get_notif_hwnd(void)
{
    static ATOM wnd_class = 0;
    HWND hwnd;

    static const WCHAR wszURLMonikerNotificationWindow[] =
        {'U','R','L',' ','M','o','n','i','k','e','r',' ',
         'N','o','t','i','f','i','c','a','t','i','o','n',' ','W','i','n','d','o','w',0};

    if(!wnd_class) {
        static WNDCLASSEXW wndclass = {
            sizeof(wndclass), 0,
            notif_wnd_proc, 0, 0,
            NULL, NULL, NULL, NULL, NULL,
            wszURLMonikerNotificationWindow,
            NULL        
        };

        wndclass.hInstance = URLMON_hInstance;

        wnd_class = RegisterClassExW(&wndclass);
        if (!wnd_class && GetLastError() == ERROR_CLASS_ALREADY_EXISTS)
            wnd_class = 1;
    }

    hwnd = CreateWindowExW(0, wszURLMonikerNotificationWindow,
                           wszURLMonikerNotificationWindow, 0, 0, 0, 0, 0, HWND_MESSAGE,
                           NULL, URLMON_hInstance, NULL);

    TRACE("hwnd = %p\n", hwnd);

    return hwnd;
}

static void dump_BINDINFO(BINDINFO *bi)
{
    static const char * const BINDINFOF_str[] = {
        "#0",
        "BINDINFOF_URLENCODESTGMEDDATA",
        "BINDINFOF_URLENCODEDEXTRAINFO"
    };

    static const char * const BINDVERB_str[] = {
        "BINDVERB_GET",
        "BINDVERB_POST",
        "BINDVERB_PUT",
        "BINDVERB_CUSTOM"
    };

    TRACE("\n"
            "BINDINFO = {\n"
            "    %d, %s,\n"
            "    {%d, %p, %p},\n"
            "    %s,\n"
            "    %s,\n"
            "    %s,\n"
            "    %d, %08x, %d, %d\n"
            "    {%d %p %x},\n"
            "    %s\n"
            "    %p, %d\n"
            "}\n",

            bi->cbSize, debugstr_w(bi->szExtraInfo),
            bi->stgmedData.tymed, bi->stgmedData.u.hGlobal, bi->stgmedData.pUnkForRelease,
            bi->grfBindInfoF > BINDINFOF_URLENCODEDEXTRAINFO
                ? "unknown" : BINDINFOF_str[bi->grfBindInfoF],
            bi->dwBindVerb > BINDVERB_CUSTOM
                ? "unknown" : BINDVERB_str[bi->dwBindVerb],
            debugstr_w(bi->szCustomVerb),
            bi->cbstgmedData, bi->dwOptions, bi->dwOptionsFlags, bi->dwCodePage,
            bi->securityAttributes.nLength,
            bi->securityAttributes.lpSecurityDescriptor,
            bi->securityAttributes.bInheritHandle,
            debugstr_guid(&bi->iid),
            bi->pUnk, bi->dwReserved
            );
}

static void set_binding_mime(Binding *binding, LPCWSTR mime)
{
    EnterCriticalSection(&binding->section);

    if(binding->report_mime) {
        heap_free(binding->mime);
        binding->mime = heap_strdupW(mime);
    }

    LeaveCriticalSection(&binding->section);
}

static void handle_mime_available(Binding *binding, BOOL verify)
{
    BOOL report_mime;

    EnterCriticalSection(&binding->section);
    report_mime = binding->report_mime;
    binding->report_mime = FALSE;
    LeaveCriticalSection(&binding->section);

    if(!report_mime)
        return;

    if(verify) {
        LPWSTR mime = NULL;

        fill_stgmed_buffer(binding->stgmed_buf);
        FindMimeFromData(NULL, binding->url, binding->stgmed_buf->buf,
                         min(binding->stgmed_buf->size, 255), binding->mime, 0, &mime, 0);

        heap_free(binding->mime);
        binding->mime = heap_strdupW(mime);
        CoTaskMemFree(mime);
    }

    IBindStatusCallback_OnProgress(binding->callback, 0, 0, BINDSTATUS_MIMETYPEAVAILABLE, binding->mime);

    binding->clipboard_format = RegisterClipboardFormatW(binding->mime);
}

typedef struct {
    task_header_t header;
    BOOL verify;
} mime_available_task_t;

static void mime_available_proc(Binding *binding, task_header_t *t)
{
    mime_available_task_t *task = (mime_available_task_t*)t;

    handle_mime_available(binding, task->verify);

    heap_free(task);
}

static void mime_available(Binding *This, LPCWSTR mime, BOOL verify)
{
    if(mime)
        set_binding_mime(This, mime);

    if(GetCurrentThreadId() == This->apartment_thread) {
        handle_mime_available(This, verify);
    }else {
        mime_available_task_t *task = heap_alloc(sizeof(task_header_t));
        task->verify = verify;
        push_task(This, &task->header, mime_available_proc);
    }
}

static void stop_binding(Binding *binding, HRESULT hres, LPCWSTR str)
{
    if(binding->state & BINDING_LOCKED) {
        IInternetProtocol_UnlockRequest(binding->protocol);
        binding->state &= ~BINDING_LOCKED;
    }

    if(!(binding->state & BINDING_STOPPED)) {
        binding->state |= BINDING_STOPPED;

        IBindStatusCallback_OnStopBinding(binding->callback, hres, str);
        binding->hres = hres;
    }
}

static LPWSTR get_mime_clsid(LPCWSTR mime, CLSID *clsid)
{
    LPWSTR key_name, ret;
    DWORD res, type, size;
    HKEY hkey;
    int len;
    HRESULT hres;

    static const WCHAR mime_keyW[] =
        {'M','I','M','E','\\','D','a','t','a','b','a','s','e','\\',
         'C','o','n','t','e','n','t',' ','T','y','p','e','\\'};
    static const WCHAR clsidW[] = {'C','L','S','I','D',0};

    len = strlenW(mime)+1;
    key_name = heap_alloc(sizeof(mime_keyW) + len*sizeof(WCHAR));
    memcpy(key_name, mime_keyW, sizeof(mime_keyW));
    strcpyW(key_name + sizeof(mime_keyW)/sizeof(WCHAR), mime);

    res = RegOpenKeyW(HKEY_CLASSES_ROOT, key_name, &hkey);
    heap_free(key_name);
    if(res != ERROR_SUCCESS) {
        WARN("Could not open MIME key: %x\n", res);
        return NULL;
    }

    size = 50*sizeof(WCHAR);
    ret = heap_alloc(size);
    res = RegQueryValueExW(hkey, clsidW, NULL, &type, (LPBYTE)ret, &size);
    RegCloseKey(hkey);
    if(res != ERROR_SUCCESS) {
        WARN("Could not get CLSID: %08x\n", res);
        heap_free(ret);
        return NULL;
    }

    hres = CLSIDFromString(ret, clsid);
    if(FAILED(hres)) {
        WARN("Could not parse CLSID: %08x\n", hres);
        heap_free(ret);
        return NULL;
    }

    return ret;
}

static void load_doc_mon(Binding *binding, IPersistMoniker *persist)
{
    IBindCtx *bctx;
    HRESULT hres;

    hres = CreateAsyncBindCtxEx(binding->bctx, 0, NULL, NULL, &bctx, 0);
    if(FAILED(hres)) {
        WARN("CreateAsyncBindCtxEx failed: %08x\n", hres);
        return;
    }

    IBindCtx_RevokeObjectParam(bctx, bscb_holderW);
    IBindCtx_RegisterObjectParam(bctx, cbinding_contextW, (IUnknown*)BINDING(binding));

    hres = IPersistMoniker_Load(persist, binding->download_state == END_DOWNLOAD, binding->mon, bctx, 0x12);
    IBindCtx_RevokeObjectParam(bctx, cbinding_contextW);
    IBindCtx_Release(bctx);
    if(FAILED(hres))
        FIXME("Load failed: %08x\n", hres);
}

static HRESULT create_mime_object(Binding *binding, const CLSID *clsid, LPCWSTR clsid_str)
{
    IPersistMoniker *persist;
    HRESULT hres;

    hres = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
                            &binding->iid, (void**)&binding->obj);
    if(FAILED(hres)) {
        WARN("CoCreateInstance failed: %08x\n", hres);
        return INET_E_CANNOT_INSTANTIATE_OBJECT;
    }

    binding->state |= BINDING_OBJAVAIL;

    hres = IUnknown_QueryInterface(binding->obj, &IID_IPersistMoniker, (void**)&persist);
    if(SUCCEEDED(hres)) {
        IMonikerProp *prop;

        hres = IPersistMoniker_QueryInterface(persist, &IID_IMonikerProp, (void**)&prop);
        if(SUCCEEDED(hres)) {
            IMonikerProp_PutProperty(prop, MIMETYPEPROP, binding->mime);
            IMonikerProp_PutProperty(prop, CLASSIDPROP, clsid_str);
            IMonikerProp_Release(prop);
        }

        load_doc_mon(binding, persist);

        IPersistMoniker_Release(persist);
    }else {
        FIXME("Could not get IPersistMoniker: %08x\n", hres);
        /* FIXME: Try query IPersistFile */
    }

    IBindStatusCallback_OnObjectAvailable(binding->callback, &binding->iid, binding->obj);

    return S_OK;
}

static void create_object(Binding *binding)
{
    LPWSTR clsid_str;
    CLSID clsid;
    HRESULT hres;

    if(!binding->mime) {
        FIXME("MIME not available\n");
        return;
    }

    if(!(clsid_str = get_mime_clsid(binding->mime, &clsid))) {
        FIXME("Could not find object for MIME %s\n", debugstr_w(binding->mime));
        return;
    }

    IBindStatusCallback_OnProgress(binding->callback, 0, 0, BINDSTATUS_CLASSIDAVAILABLE, clsid_str);
    IBindStatusCallback_OnProgress(binding->callback, 0, 0, BINDSTATUS_BEGINSYNCOPERATION, NULL);

    hres = create_mime_object(binding, &clsid, clsid_str);
    heap_free(clsid_str);

    IBindStatusCallback_OnProgress(binding->callback, 0, 0, BINDSTATUS_ENDSYNCOPERATION, NULL);

    stop_binding(binding, hres, NULL);
    if(FAILED(hres))
        IInternetProtocol_Terminate(binding->protocol, 0);
}

#define STGMEDUNK_THIS(iface) DEFINE_THIS(stgmed_buf_t, Unknown, iface)

static HRESULT WINAPI StgMedUnk_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    stgmed_buf_t *This = STGMEDUNK_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);

        *ppv = STGMEDUNK(This);
        IUnknown_AddRef(STGMEDUNK(This));
        return S_OK;
    }

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI StgMedUnk_AddRef(IUnknown *iface)
{
    stgmed_buf_t *This = STGMEDUNK_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI StgMedUnk_Release(IUnknown *iface)
{
    stgmed_buf_t *This = STGMEDUNK_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        IInternetProtocol_Release(This->protocol);
        heap_free(This->cache_file);
        heap_free(This);

        URLMON_UnlockModule();
    }

    return ref;
}

#undef STGMEDUNK_THIS

static const IUnknownVtbl StgMedUnkVtbl = {
    StgMedUnk_QueryInterface,
    StgMedUnk_AddRef,
    StgMedUnk_Release
};

static stgmed_buf_t *create_stgmed_buf(IInternetProtocol *protocol)
{
    stgmed_buf_t *ret = heap_alloc(sizeof(*ret));

    ret->lpUnknownVtbl = &StgMedUnkVtbl;
    ret->ref = 1;
    ret->size = 0;
    ret->init = FALSE;
    ret->hres = S_OK;
    ret->cache_file = NULL;

    IInternetProtocol_AddRef(protocol);
    ret->protocol = protocol;

    URLMON_LockModule();

    return ret;
}

typedef struct {
    stgmed_obj_t stgmed_obj;
    const IStreamVtbl *lpStreamVtbl;

    LONG ref;

    stgmed_buf_t *buf;
} ProtocolStream;

#define STREAM_THIS(iface) DEFINE_THIS(ProtocolStream, Stream, iface)

static HRESULT WINAPI ProtocolStream_QueryInterface(IStream *iface,
                                                          REFIID riid, void **ppv)
{
    ProtocolStream *This = STREAM_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = STREAM(This);
    }else if(IsEqualGUID(&IID_ISequentialStream, riid)) {
        TRACE("(%p)->(IID_ISequentialStream %p)\n", This, ppv);
        *ppv = STREAM(This);
    }else if(IsEqualGUID(&IID_IStream, riid)) {
        TRACE("(%p)->(IID_IStream %p)\n", This, ppv);
        *ppv = STREAM(This);
    }

    if(*ppv) {
        IStream_AddRef(STREAM(This));
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI ProtocolStream_AddRef(IStream *iface)
{
    ProtocolStream *This = STREAM_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI ProtocolStream_Release(IStream *iface)
{
    ProtocolStream *This = STREAM_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        IUnknown_Release(STGMEDUNK(This->buf));
        heap_free(This);

        URLMON_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI ProtocolStream_Read(IStream *iface, void *pv,
                                          ULONG cb, ULONG *pcbRead)
{
    ProtocolStream *This = STREAM_THIS(iface);
    DWORD read = 0, pread = 0;
    HRESULT hres;

    TRACE("(%p)->(%p %d %p)\n", This, pv, cb, pcbRead);

    if(This->buf->size) {
        read = cb;

        if(read > This->buf->size)
            read = This->buf->size;

        memcpy(pv, This->buf->buf, read);

        if(read < This->buf->size)
            memmove(This->buf->buf, This->buf->buf+read, This->buf->size-read);
        This->buf->size -= read;
    }

    if(read == cb) {
        if (pcbRead)
            *pcbRead = read;
        return S_OK;
    }

    hres = This->buf->hres = IInternetProtocol_Read(This->buf->protocol, (PBYTE)pv+read, cb-read, &pread);
    if (pcbRead)
        *pcbRead = read + pread;

    if(hres == E_PENDING)
        return E_PENDING;
    else if(FAILED(hres))
        FIXME("Read failed: %08x\n", hres);

    return read || pread ? S_OK : S_FALSE;
}

static HRESULT WINAPI ProtocolStream_Write(IStream *iface, const void *pv,
                                          ULONG cb, ULONG *pcbWritten)
{
    ProtocolStream *This = STREAM_THIS(iface);

    TRACE("(%p)->(%p %d %p)\n", This, pv, cb, pcbWritten);

    return STG_E_ACCESSDENIED;
}

static HRESULT WINAPI ProtocolStream_Seek(IStream *iface, LARGE_INTEGER dlibMove,
                                         DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    ProtocolStream *This = STREAM_THIS(iface);
    FIXME("(%p)->(%d %08x %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolStream_SetSize(IStream *iface, ULARGE_INTEGER libNewSize)
{
    ProtocolStream *This = STREAM_THIS(iface);
    FIXME("(%p)->(%d)\n", This, libNewSize.u.LowPart);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolStream_CopyTo(IStream *iface, IStream *pstm,
        ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    ProtocolStream *This = STREAM_THIS(iface);
    FIXME("(%p)->(%p %d %p %p)\n", This, pstm, cb.u.LowPart, pcbRead, pcbWritten);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolStream_Commit(IStream *iface, DWORD grfCommitFlags)
{
    ProtocolStream *This = STREAM_THIS(iface);

    TRACE("(%p)->(%08x)\n", This, grfCommitFlags);

    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolStream_Revert(IStream *iface)
{
    ProtocolStream *This = STREAM_THIS(iface);

    TRACE("(%p)\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolStream_LockRegion(IStream *iface, ULARGE_INTEGER libOffset,
                                               ULARGE_INTEGER cb, DWORD dwLockType)
{
    ProtocolStream *This = STREAM_THIS(iface);
    FIXME("(%p)->(%d %d %d)\n", This, libOffset.u.LowPart, cb.u.LowPart, dwLockType);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolStream_UnlockRegion(IStream *iface,
        ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    ProtocolStream *This = STREAM_THIS(iface);
    FIXME("(%p)->(%d %d %d)\n", This, libOffset.u.LowPart, cb.u.LowPart, dwLockType);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolStream_Stat(IStream *iface, STATSTG *pstatstg,
                                         DWORD dwStatFlag)
{
    ProtocolStream *This = STREAM_THIS(iface);
    FIXME("(%p)->(%p %08x)\n", This, pstatstg, dwStatFlag);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolStream_Clone(IStream *iface, IStream **ppstm)
{
    ProtocolStream *This = STREAM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppstm);
    return E_NOTIMPL;
}

#undef STREAM_THIS

static const IStreamVtbl ProtocolStreamVtbl = {
    ProtocolStream_QueryInterface,
    ProtocolStream_AddRef,
    ProtocolStream_Release,
    ProtocolStream_Read,
    ProtocolStream_Write,
    ProtocolStream_Seek,
    ProtocolStream_SetSize,
    ProtocolStream_CopyTo,
    ProtocolStream_Commit,
    ProtocolStream_Revert,
    ProtocolStream_LockRegion,
    ProtocolStream_UnlockRegion,
    ProtocolStream_Stat,
    ProtocolStream_Clone
};

static void stgmed_stream_release(stgmed_obj_t *obj)
{
    ProtocolStream *stream = (ProtocolStream*)obj;
    IStream_Release(STREAM(stream));
}

static HRESULT stgmed_stream_fill_stgmed(stgmed_obj_t *obj, STGMEDIUM *stgmed)
{
    ProtocolStream *stream = (ProtocolStream*)obj;

    stgmed->tymed = TYMED_ISTREAM;
    stgmed->u.pstm = STREAM(stream);
    stgmed->pUnkForRelease = STGMEDUNK(stream->buf);

    return S_OK;
}

static void *stgmed_stream_get_result(stgmed_obj_t *obj)
{
    ProtocolStream *stream = (ProtocolStream*)obj;

    IStream_AddRef(STREAM(stream));
    return STREAM(stream);
}

static const stgmed_obj_vtbl stgmed_stream_vtbl = {
    stgmed_stream_release,
    stgmed_stream_fill_stgmed,
    stgmed_stream_get_result
};

typedef struct {
    stgmed_obj_t stgmed_obj;
    stgmed_buf_t *buf;
} stgmed_file_obj_t;

static stgmed_obj_t *create_stgmed_stream(stgmed_buf_t *buf)
{
    ProtocolStream *ret = heap_alloc(sizeof(ProtocolStream));

    ret->stgmed_obj.vtbl = &stgmed_stream_vtbl;
    ret->lpStreamVtbl = &ProtocolStreamVtbl;
    ret->ref = 1;

    IUnknown_AddRef(STGMEDUNK(buf));
    ret->buf = buf;

    URLMON_LockModule();

    return &ret->stgmed_obj;
}

static void stgmed_file_release(stgmed_obj_t *obj)
{
    stgmed_file_obj_t *file_obj = (stgmed_file_obj_t*)obj;

    IUnknown_Release(STGMEDUNK(file_obj->buf));
    heap_free(file_obj);
}

static HRESULT stgmed_file_fill_stgmed(stgmed_obj_t *obj, STGMEDIUM *stgmed)
{
    stgmed_file_obj_t *file_obj = (stgmed_file_obj_t*)obj;

    if(!file_obj->buf->cache_file) {
        WARN("cache_file not set\n");
        return INET_E_DATA_NOT_AVAILABLE;
    }

    fill_stgmed_buffer(file_obj->buf);
    if(file_obj->buf->size == sizeof(file_obj->buf->buf)) {
        BYTE buf[1024];
        DWORD read;
        HRESULT hres;

        do {
            hres = IInternetProtocol_Read(file_obj->buf->protocol, buf, sizeof(buf), &read);
        }while(hres == S_OK);
    }

    stgmed->tymed = TYMED_FILE;
    stgmed->u.lpszFileName = file_obj->buf->cache_file;
    stgmed->pUnkForRelease = STGMEDUNK(file_obj->buf);

    return S_OK;
}

static void *stgmed_file_get_result(stgmed_obj_t *obj)
{
    return NULL;
}

static const stgmed_obj_vtbl stgmed_file_vtbl = {
    stgmed_file_release,
    stgmed_file_fill_stgmed,
    stgmed_file_get_result
};

static stgmed_obj_t *create_stgmed_file(stgmed_buf_t *buf)
{
    stgmed_file_obj_t *ret = heap_alloc(sizeof(*ret));

    ret->stgmed_obj.vtbl = &stgmed_file_vtbl;

    IUnknown_AddRef(STGMEDUNK(buf));
    ret->buf = buf;

    return &ret->stgmed_obj;
}

#define BINDING_THIS(iface) DEFINE_THIS(Binding, Binding, iface)

static HRESULT WINAPI Binding_QueryInterface(IBinding *iface, REFIID riid, void **ppv)
{
    Binding *This = BINDING_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = BINDING(This);
    }else if(IsEqualGUID(&IID_IBinding, riid)) {
        TRACE("(%p)->(IID_IBinding %p)\n", This, ppv);
        *ppv = BINDING(This);
    }else if(IsEqualGUID(&IID_IInternetProtocolSink, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolSink %p)\n", This, ppv);
        *ppv = PROTSINK(This);
    }else if(IsEqualGUID(&IID_IInternetBindInfo, riid)) {
        TRACE("(%p)->(IID_IInternetBindInfo %p)\n", This, ppv);
        *ppv = BINDINF(This);
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        TRACE("(%p)->(IID_IServiceProvider %p)\n", This, ppv);
        *ppv = SERVPROV(This);
    }

    if(*ppv) {
        IBinding_AddRef(BINDING(This));
        return S_OK;
    }

    WARN("Unsupported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI Binding_AddRef(IBinding *iface)
{
    Binding *This = BINDING_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI Binding_Release(IBinding *iface)
{
    Binding *This = BINDING_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if (This->notif_hwnd)
            DestroyWindow( This->notif_hwnd );
        if(This->mon)
            IMoniker_Release(This->mon);
        if(This->callback)
            IBindStatusCallback_Release(This->callback);
        if(This->protocol)
            IInternetProtocol_Release(This->protocol);
        if(This->service_provider)
            IServiceProvider_Release(This->service_provider);
        if(This->stgmed_buf)
            IUnknown_Release(STGMEDUNK(This->stgmed_buf));
        if(This->stgmed_obj)
            This->stgmed_obj->vtbl->release(This->stgmed_obj);
        if(This->obj)
            IUnknown_Release(This->obj);
        if(This->bctx)
            IBindCtx_Release(This->bctx);

        ReleaseBindInfo(&This->bindinfo);
        This->section.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->section);
        heap_free(This->mime);
        heap_free(This->url);

        heap_free(This);

        URLMON_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI Binding_Abort(IBinding *iface)
{
    Binding *This = BINDING_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_Suspend(IBinding *iface)
{
    Binding *This = BINDING_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_Resume(IBinding *iface)
{
    Binding *This = BINDING_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_SetPriority(IBinding *iface, LONG nPriority)
{
    Binding *This = BINDING_THIS(iface);
    FIXME("(%p)->(%d)\n", This, nPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_GetPriority(IBinding *iface, LONG *pnPriority)
{
    Binding *This = BINDING_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pnPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_GetBindResult(IBinding *iface, CLSID *pclsidProtocol,
        DWORD *pdwResult, LPOLESTR *pszResult, DWORD *pdwReserved)
{
    Binding *This = BINDING_THIS(iface);
    FIXME("(%p)->(%p %p %p %p)\n", This, pclsidProtocol, pdwResult, pszResult, pdwReserved);
    return E_NOTIMPL;
}

static Binding *get_bctx_binding(IBindCtx *bctx)
{
    IBinding *binding;
    IUnknown *unk;
    HRESULT hres;

    hres = IBindCtx_GetObjectParam(bctx, cbinding_contextW, &unk);
    if(FAILED(hres))
        return NULL;

    hres = IUnknown_QueryInterface(unk, &IID_IBinding, (void*)&binding);
    IUnknown_Release(unk);
    if(FAILED(hres))
        return NULL;

    /* FIXME!!! */
    return BINDING_THIS(binding);
}

#undef BINDING_THIS

static const IBindingVtbl BindingVtbl = {
    Binding_QueryInterface,
    Binding_AddRef,
    Binding_Release,
    Binding_Abort,
    Binding_Suspend,
    Binding_Resume,
    Binding_SetPriority,
    Binding_GetPriority,
    Binding_GetBindResult
};

#define PROTSINK_THIS(iface) DEFINE_THIS(Binding, InternetProtocolSink, iface)

static HRESULT WINAPI InternetProtocolSink_QueryInterface(IInternetProtocolSink *iface,
        REFIID riid, void **ppv)
{
    Binding *This = PROTSINK_THIS(iface);
    return IBinding_QueryInterface(BINDING(This), riid, ppv);
}

static ULONG WINAPI InternetProtocolSink_AddRef(IInternetProtocolSink *iface)
{
    Binding *This = PROTSINK_THIS(iface);
    return IBinding_AddRef(BINDING(This));
}

static ULONG WINAPI InternetProtocolSink_Release(IInternetProtocolSink *iface)
{
    Binding *This = PROTSINK_THIS(iface);
    return IBinding_Release(BINDING(This));
}

typedef struct {
    task_header_t header;
    PROTOCOLDATA data;
} switch_task_t;

static void switch_proc(Binding *binding, task_header_t *t)
{
    switch_task_t *task = (switch_task_t*)t;

    IInternetProtocol_Continue(binding->protocol, &task->data);

    heap_free(task);
}

static HRESULT WINAPI InternetProtocolSink_Switch(IInternetProtocolSink *iface,
        PROTOCOLDATA *pProtocolData)
{
    Binding *This = PROTSINK_THIS(iface);
    switch_task_t *task;

    TRACE("(%p)->(%p)\n", This, pProtocolData);

    task = heap_alloc(sizeof(switch_task_t));
    task->data = *pProtocolData;

    push_task(This, &task->header, switch_proc);

    IBinding_AddRef(BINDING(This));
    PostMessageW(This->notif_hwnd, WM_MK_CONTINUE, 0, (LPARAM)This);

    return S_OK;
}

typedef struct {
    task_header_t header;

    Binding *binding;
    ULONG progress;
    ULONG progress_max;
    ULONG status_code;
    LPWSTR status_text;
} on_progress_task_t;

static void on_progress_proc(Binding *binding, task_header_t *t)
{
    on_progress_task_t *task = (on_progress_task_t*)t;

    IBindStatusCallback_OnProgress(binding->callback, task->progress,
            task->progress_max, task->status_code, task->status_text);

    heap_free(task->status_text);
    heap_free(task);
}

static void on_progress(Binding *This, ULONG progress, ULONG progress_max,
                        ULONG status_code, LPCWSTR status_text)
{
    on_progress_task_t *task;

    if(GetCurrentThreadId() == This->apartment_thread && !This->continue_call) {
        IBindStatusCallback_OnProgress(This->callback, progress, progress_max,
                                       status_code, status_text);
        return;
    }

    task = heap_alloc(sizeof(on_progress_task_t));

    task->progress = progress;
    task->progress_max = progress_max;
    task->status_code = status_code;

    if(status_text) {
        DWORD size = (strlenW(status_text)+1)*sizeof(WCHAR);

        task->status_text = heap_alloc(size);
        memcpy(task->status_text, status_text, size);
    }else {
        task->status_text = NULL;
    }

    push_task(This, &task->header, on_progress_proc);

    if(GetCurrentThreadId() != This->apartment_thread) {
        IBinding_AddRef(BINDING(This));
        PostMessageW(This->notif_hwnd, WM_MK_CONTINUE, 0, (LPARAM)This);
    }
}

static HRESULT WINAPI InternetProtocolSink_ReportProgress(IInternetProtocolSink *iface,
        ULONG ulStatusCode, LPCWSTR szStatusText)
{
    Binding *This = PROTSINK_THIS(iface);

    TRACE("(%p)->(%u %s)\n", This, ulStatusCode, debugstr_w(szStatusText));

    switch(ulStatusCode) {
    case BINDSTATUS_FINDINGRESOURCE:
        on_progress(This, 0, 0, BINDSTATUS_FINDINGRESOURCE, szStatusText);
        break;
    case BINDSTATUS_CONNECTING:
        on_progress(This, 0, 0, BINDSTATUS_CONNECTING, szStatusText);
        break;
    case BINDSTATUS_BEGINDOWNLOADDATA:
        fill_stgmed_buffer(This->stgmed_buf);
        break;
    case BINDSTATUS_MIMETYPEAVAILABLE:
        set_binding_mime(This, szStatusText);
        break;
    case BINDSTATUS_SENDINGREQUEST:
        on_progress(This, 0, 0, BINDSTATUS_SENDINGREQUEST, szStatusText);
        break;
    case BINDSTATUS_PROTOCOLCLASSID:
        break;
    case BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE:
        mime_available(This, szStatusText, FALSE);
        break;
    case BINDSTATUS_CACHEFILENAMEAVAILABLE:
        heap_free(This->stgmed_buf->cache_file);
        This->stgmed_buf->cache_file = heap_strdupW(szStatusText);
        break;
    case BINDSTATUS_DIRECTBIND:
        This->report_mime = FALSE;
        break;
    case BINDSTATUS_ACCEPTRANGES:
        break;
    default:
        FIXME("Unhandled status code %d\n", ulStatusCode);
        return E_NOTIMPL;
    };

    return S_OK;
}

static void report_data(Binding *This, DWORD bscf, ULONG progress, ULONG progress_max)
{
    FORMATETC formatetc = {0, NULL, 1, -1, TYMED_ISTREAM};
    BOOL sent_begindownloaddata = FALSE;

    TRACE("(%p)->(%d %u %u)\n", This, bscf, progress, progress_max);

    if(This->download_state == END_DOWNLOAD)
        return;

    if(GetCurrentThreadId() != This->apartment_thread)
        FIXME("called from worker thread\n");

    if(This->report_mime)
        mime_available(This, NULL, TRUE);

    if(This->download_state == BEFORE_DOWNLOAD) {
        fill_stgmed_buffer(This->stgmed_buf);

        This->download_state = DOWNLOADING;
        sent_begindownloaddata = TRUE;
        IBindStatusCallback_OnProgress(This->callback, progress, progress_max,
                BINDSTATUS_BEGINDOWNLOADDATA, This->url);

        if(This->stgmed_buf->cache_file)
            IBindStatusCallback_OnProgress(This->callback, progress, progress_max,
                    BINDSTATUS_CACHEFILENAMEAVAILABLE, This->stgmed_buf->cache_file);
    }

    if(This->stgmed_buf->hres == S_FALSE || (bscf & BSCF_LASTDATANOTIFICATION)) {
        This->download_state = END_DOWNLOAD;
        IBindStatusCallback_OnProgress(This->callback, progress, progress_max,
                BINDSTATUS_ENDDOWNLOADDATA, This->url);
    }else if(!sent_begindownloaddata) {
        IBindStatusCallback_OnProgress(This->callback, progress, progress_max,
                BINDSTATUS_DOWNLOADINGDATA, This->url);
    }

    if(This->to_object) {
        if(!(This->state & BINDING_OBJAVAIL))
            create_object(This);
    }else {
        STGMEDIUM stgmed;
        HRESULT hres;

        if(!(This->state & BINDING_LOCKED)) {
            HRESULT hres = IInternetProtocol_LockRequest(This->protocol, 0);
            if(SUCCEEDED(hres))
                This->state |= BINDING_LOCKED;
        }

        hres = This->stgmed_obj->vtbl->fill_stgmed(This->stgmed_obj, &stgmed);
        if(FAILED(hres)) {
            stop_binding(This, hres, NULL);
            return;
        }

        formatetc.tymed = stgmed.tymed;
        formatetc.cfFormat = This->clipboard_format;

        IBindStatusCallback_OnDataAvailable(This->callback, bscf, progress,
                &formatetc, &stgmed);

        if(This->download_state == END_DOWNLOAD)
            stop_binding(This, S_OK, NULL);
    }
}

typedef struct {
    task_header_t header;
    DWORD bscf;
    ULONG progress;
    ULONG progress_max;
} report_data_task_t;

static void report_data_proc(Binding *binding, task_header_t *t)
{
    report_data_task_t *task = (report_data_task_t*)t;

    report_data(binding, task->bscf, task->progress, task->progress_max);

    heap_free(task);
}

static HRESULT WINAPI InternetProtocolSink_ReportData(IInternetProtocolSink *iface,
        DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax)
{
    Binding *This = PROTSINK_THIS(iface);

    TRACE("(%p)->(%d %u %u)\n", This, grfBSCF, ulProgress, ulProgressMax);

    if(GetCurrentThreadId() != This->apartment_thread)
        FIXME("called from worked hread\n");

    if(This->continue_call) {
        report_data_task_t *task = heap_alloc(sizeof(report_data_task_t));
        task->bscf = grfBSCF;
        task->progress = ulProgress;
        task->progress_max = ulProgressMax;

        push_task(This, &task->header, report_data_proc);
    }else {
        report_data(This, grfBSCF, ulProgress, ulProgressMax);
    }

    return S_OK;
}

typedef struct {
    task_header_t header;

    HRESULT hres;
    LPWSTR str;
} report_result_task_t;

static void report_result_proc(Binding *binding, task_header_t *t)
{
    report_result_task_t *task = (report_result_task_t*)t;

    IInternetProtocol_Terminate(binding->protocol, 0);

    stop_binding(binding, task->hres, task->str);

    heap_free(task->str);
    heap_free(task);
}

static HRESULT WINAPI InternetProtocolSink_ReportResult(IInternetProtocolSink *iface,
        HRESULT hrResult, DWORD dwError, LPCWSTR szResult)
{
    Binding *This = PROTSINK_THIS(iface);

    TRACE("(%p)->(%08x %d %s)\n", This, hrResult, dwError, debugstr_w(szResult));

    if(GetCurrentThreadId() == This->apartment_thread && !This->continue_call) {
        IInternetProtocol_Terminate(This->protocol, 0);
        stop_binding(This, hrResult, szResult);
    }else {
        report_result_task_t *task = heap_alloc(sizeof(report_result_task_t));

        task->hres = hrResult;
        task->str = heap_strdupW(szResult);

        push_task(This, &task->header, report_result_proc);
    }

    return S_OK;
}

#undef PROTSINK_THIS

static const IInternetProtocolSinkVtbl InternetProtocolSinkVtbl = {
    InternetProtocolSink_QueryInterface,
    InternetProtocolSink_AddRef,
    InternetProtocolSink_Release,
    InternetProtocolSink_Switch,
    InternetProtocolSink_ReportProgress,
    InternetProtocolSink_ReportData,
    InternetProtocolSink_ReportResult
};

#define BINDINF_THIS(iface) DEFINE_THIS(Binding, InternetBindInfo, iface)

static HRESULT WINAPI InternetBindInfo_QueryInterface(IInternetBindInfo *iface,
        REFIID riid, void **ppv)
{
    Binding *This = BINDINF_THIS(iface);
    return IBinding_QueryInterface(BINDING(This), riid, ppv);
}

static ULONG WINAPI InternetBindInfo_AddRef(IInternetBindInfo *iface)
{
    Binding *This = BINDINF_THIS(iface);
    return IBinding_AddRef(BINDING(This));
}

static ULONG WINAPI InternetBindInfo_Release(IInternetBindInfo *iface)
{
    Binding *This = BINDINF_THIS(iface);
    return IBinding_Release(BINDING(This));
}

static HRESULT WINAPI InternetBindInfo_GetBindInfo(IInternetBindInfo *iface,
        DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    Binding *This = BINDINF_THIS(iface);

    TRACE("(%p)->(%p %p)\n", This, grfBINDF, pbindinfo);

    *grfBINDF = This->bindf;

    *pbindinfo = This->bindinfo;

    if(pbindinfo->szExtraInfo || pbindinfo->szCustomVerb)
        FIXME("copy strings\n");

    if(pbindinfo->stgmedData.pUnkForRelease)
        IUnknown_AddRef(pbindinfo->stgmedData.pUnkForRelease);

    if(pbindinfo->pUnk)
        IUnknown_AddRef(pbindinfo->pUnk);

    return S_OK;
}

static HRESULT WINAPI InternetBindInfo_GetBindString(IInternetBindInfo *iface,
        ULONG ulStringType, LPOLESTR *ppwzStr, ULONG cEl, ULONG *pcElFetched)
{
    Binding *This = BINDINF_THIS(iface);

    TRACE("(%p)->(%d %p %d %p)\n", This, ulStringType, ppwzStr, cEl, pcElFetched);

    switch(ulStringType) {
    case BINDSTRING_ACCEPT_MIMES: {
        static const WCHAR wszMimes[] = {'*','/','*',0};

        if(!ppwzStr || !pcElFetched)
            return E_INVALIDARG;

        ppwzStr[0] = CoTaskMemAlloc(sizeof(wszMimes));
        memcpy(ppwzStr[0], wszMimes, sizeof(wszMimes));
        *pcElFetched = 1;
        return S_OK;
    }
    case BINDSTRING_USER_AGENT: {
        IInternetBindInfo *bindinfo = NULL;
        HRESULT hres;

        hres = IBindStatusCallback_QueryInterface(This->callback, &IID_IInternetBindInfo,
                                                  (void**)&bindinfo);
        if(FAILED(hres))
            return hres;

        hres = IInternetBindInfo_GetBindString(bindinfo, ulStringType, ppwzStr,
                                               cEl, pcElFetched);
        IInternetBindInfo_Release(bindinfo);

        return hres;
    }
    }

    FIXME("not supported string type %d\n", ulStringType);
    return E_NOTIMPL;
}

#undef BINDF_THIS

static const IInternetBindInfoVtbl InternetBindInfoVtbl = {
    InternetBindInfo_QueryInterface,
    InternetBindInfo_AddRef,
    InternetBindInfo_Release,
    InternetBindInfo_GetBindInfo,
    InternetBindInfo_GetBindString
};

#define SERVPROV_THIS(iface) DEFINE_THIS(Binding, ServiceProvider, iface)

static HRESULT WINAPI ServiceProvider_QueryInterface(IServiceProvider *iface,
        REFIID riid, void **ppv)
{
    Binding *This = SERVPROV_THIS(iface);
    return IBinding_QueryInterface(BINDING(This), riid, ppv);
}

static ULONG WINAPI ServiceProvider_AddRef(IServiceProvider *iface)
{
    Binding *This = SERVPROV_THIS(iface);
    return IBinding_AddRef(BINDING(This));
}

static ULONG WINAPI ServiceProvider_Release(IServiceProvider *iface)
{
    Binding *This = SERVPROV_THIS(iface);
    return IBinding_Release(BINDING(This));
}

static HRESULT WINAPI ServiceProvider_QueryService(IServiceProvider *iface,
        REFGUID guidService, REFIID riid, void **ppv)
{
    Binding *This = SERVPROV_THIS(iface);
    HRESULT hres;

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);

    if(This->service_provider) {
        hres = IServiceProvider_QueryService(This->service_provider, guidService,
                                             riid, ppv);
        if(SUCCEEDED(hres))
            return hres;
    }

    WARN("unknown service %s\n", debugstr_guid(guidService));
    return E_NOTIMPL;
}

#undef SERVPROV_THIS

static const IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService
};

static HRESULT get_callback(IBindCtx *pbc, IBindStatusCallback **callback)
{
    IUnknown *unk;
    HRESULT hres;

    hres = IBindCtx_GetObjectParam(pbc, bscb_holderW, &unk);
    if(SUCCEEDED(hres)) {
        hres = IUnknown_QueryInterface(unk, &IID_IBindStatusCallback, (void**)callback);
        IUnknown_Release(unk);
    }

    return SUCCEEDED(hres) ? S_OK : INET_E_DATA_NOT_AVAILABLE;
}

static BOOL is_urlmon_protocol(LPCWSTR url)
{
    static const WCHAR wszCdl[] = {'c','d','l'};
    static const WCHAR wszFile[] = {'f','i','l','e'};
    static const WCHAR wszFtp[]  = {'f','t','p'};
    static const WCHAR wszGopher[] = {'g','o','p','h','e','r'};
    static const WCHAR wszHttp[] = {'h','t','t','p'};
    static const WCHAR wszHttps[] = {'h','t','t','p','s'};
    static const WCHAR wszMk[]   = {'m','k'};

    static const struct {
        LPCWSTR scheme;
        int len;
    } protocol_list[] = {
        {wszCdl,    sizeof(wszCdl)   /sizeof(WCHAR)},
        {wszFile,   sizeof(wszFile)  /sizeof(WCHAR)},
        {wszFtp,    sizeof(wszFtp)   /sizeof(WCHAR)},
        {wszGopher, sizeof(wszGopher)/sizeof(WCHAR)},
        {wszHttp,   sizeof(wszHttp)  /sizeof(WCHAR)},
        {wszHttps,  sizeof(wszHttps) /sizeof(WCHAR)},
        {wszMk,     sizeof(wszMk)    /sizeof(WCHAR)}
    };

    unsigned int i;
    int len = lstrlenW(url);

    for(i=0; i < sizeof(protocol_list)/sizeof(protocol_list[0]); i++) {
        if(len >= protocol_list[i].len
           && !memcmp(url, protocol_list[i].scheme, protocol_list[i].len*sizeof(WCHAR)))
            return TRUE;
    }

    return FALSE;
}

static HRESULT Binding_Create(IMoniker *mon, Binding *binding_ctx, LPCWSTR url, IBindCtx *pbc,
        BOOL to_obj, REFIID riid, Binding **binding)
{
    Binding *ret;
    HRESULT hres;

    URLMON_LockModule();

    ret = heap_alloc_zero(sizeof(Binding));

    ret->lpBindingVtbl              = &BindingVtbl;
    ret->lpInternetProtocolSinkVtbl = &InternetProtocolSinkVtbl;
    ret->lpInternetBindInfoVtbl     = &InternetBindInfoVtbl;
    ret->lpServiceProviderVtbl      = &ServiceProviderVtbl;

    ret->ref = 1;

    ret->to_object = to_obj;
    ret->iid = *riid;
    ret->apartment_thread = GetCurrentThreadId();
    ret->notif_hwnd = get_notif_hwnd();
    ret->report_mime = !binding_ctx;
    ret->download_state = BEFORE_DOWNLOAD;

    if(to_obj) {
        IBindCtx_AddRef(pbc);
        ret->bctx = pbc;
    }

    if(mon) {
        IMoniker_AddRef(mon);
        ret->mon = mon;
    }

    ret->bindinfo.cbSize = sizeof(BINDINFO);

    InitializeCriticalSection(&ret->section);
    ret->section.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": Binding.section");

    hres = get_callback(pbc, &ret->callback);
    if(FAILED(hres)) {
        WARN("Could not get IBindStatusCallback\n");
        IBinding_Release(BINDING(ret));
        return hres;
    }

    IBindStatusCallback_QueryInterface(ret->callback, &IID_IServiceProvider,
                                       (void**)&ret->service_provider);

    if(binding_ctx) {
        ret->protocol = binding_ctx->protocol;
        IInternetProtocol_AddRef(ret->protocol);
    }else {
        hres = create_binding_protocol(url, TRUE, &ret->protocol);
        if(FAILED(hres)) {
            WARN("Could not get protocol handler\n");
            IBinding_Release(BINDING(ret));
            return hres;
        }
    }

    hres = IBindStatusCallback_GetBindInfo(ret->callback, &ret->bindf, &ret->bindinfo);
    if(FAILED(hres)) {
        WARN("GetBindInfo failed: %08x\n", hres);
        IBinding_Release(BINDING(ret));
        return hres;
    }

    TRACE("bindf %08x\n", ret->bindf);
    dump_BINDINFO(&ret->bindinfo);

    ret->bindf |= BINDF_FROMURLMON;
    if(to_obj)
        ret->bindinfo.dwOptions |= 0x100000;

    if(!is_urlmon_protocol(url))
        ret->bindf |= BINDF_NEEDFILE;

    ret->url = heap_strdupW(url);

    if(binding_ctx) {
        ret->stgmed_buf = binding_ctx->stgmed_buf;
        IUnknown_AddRef(STGMEDUNK(ret->stgmed_buf));
        ret->clipboard_format = binding_ctx->clipboard_format;
    }else {
        ret->stgmed_buf = create_stgmed_buf(ret->protocol);
    }

    if(to_obj) {
        ret->stgmed_obj = NULL;
    }else if(IsEqualGUID(&IID_IStream, riid)) {
        ret->stgmed_obj = create_stgmed_stream(ret->stgmed_buf);
    }else if(IsEqualGUID(&IID_IUnknown, riid)) {
        ret->bindf |= BINDF_NEEDFILE;
        ret->stgmed_obj = create_stgmed_file(ret->stgmed_buf);
    }else {
        FIXME("Unsupported riid %s\n", debugstr_guid(riid));
        IBinding_Release(BINDING(ret));
        return E_NOTIMPL;
    }

    *binding = ret;
    return S_OK;
}

static HRESULT start_binding(IMoniker *mon, Binding *binding_ctx, LPCWSTR url, IBindCtx *pbc,
                             BOOL to_obj, REFIID riid, Binding **ret)
{
    Binding *binding = NULL;
    HRESULT hres;
    MSG msg;

    hres = Binding_Create(mon, binding_ctx, url, pbc, to_obj, riid, &binding);
    if(FAILED(hres))
        return hres;

    hres = IBindStatusCallback_OnStartBinding(binding->callback, 0, BINDING(binding));
    if(FAILED(hres)) {
        WARN("OnStartBinding failed: %08x\n", hres);
        stop_binding(binding, INET_E_DOWNLOAD_FAILURE, NULL);
        IBinding_Release(BINDING(binding));
        return hres;
    }

    if(binding_ctx) {
        set_binding_sink(binding->protocol, PROTSINK(binding));
        report_data(binding, 0, 0, 0);
    }else {
        hres = IInternetProtocol_Start(binding->protocol, url, PROTSINK(binding),
                 BINDINF(binding), 0, 0);

        TRACE("start ret %08x\n", hres);

        if(FAILED(hres) && hres != E_PENDING) {
            stop_binding(binding, hres, NULL);
            IBinding_Release(BINDING(binding));

            return hres;
        }
    }

    while(!(binding->bindf & BINDF_ASYNCHRONOUS) &&
          !(binding->state & BINDING_STOPPED)) {
        MsgWaitForMultipleObjects(0, NULL, FALSE, 5000, QS_POSTMESSAGE);
        while (PeekMessageW(&msg, binding->notif_hwnd, WM_USER, WM_USER+117, PM_REMOVE|PM_NOYIELD)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    *ret = binding;
    return S_OK;
}

HRESULT bind_to_storage(LPCWSTR url, IBindCtx *pbc, REFIID riid, void **ppv)
{
    Binding *binding = NULL, *binding_ctx;
    HRESULT hres;

    *ppv = NULL;

    binding_ctx = get_bctx_binding(pbc);

    hres = start_binding(NULL, binding_ctx, url, pbc, FALSE, riid, &binding);
    if(binding_ctx)
        IBinding_Release(BINDING(binding_ctx));
    if(FAILED(hres))
        return hres;

    if(binding->hres == S_OK && binding->stgmed_buf->init) {
        if((binding->state & BINDING_STOPPED) && (binding->state & BINDING_LOCKED))
            IInternetProtocol_UnlockRequest(binding->protocol);

        *ppv = binding->stgmed_obj->vtbl->get_result(binding->stgmed_obj);
    }

    IBinding_Release(BINDING(binding));

    return *ppv ? S_OK : MK_S_ASYNCHRONOUS;
}

HRESULT bind_to_object(IMoniker *mon, LPCWSTR url, IBindCtx *pbc, REFIID riid, void **ppv)
{
    Binding *binding;
    HRESULT hres;

    *ppv = NULL;

    hres = start_binding(mon, NULL, url, pbc, TRUE, riid, &binding);
    if(FAILED(hres))
        return hres;

    if(binding->hres != S_OK) {
        hres = SUCCEEDED(binding->hres) ? S_OK : binding->hres;
    }else if(binding->bindf & BINDF_ASYNCHRONOUS) {
        hres = MK_S_ASYNCHRONOUS;
    }else {
        *ppv = binding->obj;
        IUnknown_AddRef(binding->obj);
        hres = S_OK;
    }

    IBinding_Release(BINDING(binding));

    return hres;
}
