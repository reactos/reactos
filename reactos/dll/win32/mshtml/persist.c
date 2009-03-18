/*
 * Copyright 2005 Jacek Caban
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

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "shlguid.h"
#include "idispids.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define USER_AGENT "User-Agent:"
#define CONTENT_TYPE "Content-Type:"

static int fix_headers(char *buf, DWORD post_len)
{
    char *ptr = buf, *ptr2;

    while(*ptr && (ptr[0] != '\r' || ptr[1] != '\n')) {
        for(ptr2=ptr+1; *ptr2 && (ptr2[0] != '\r' || ptr2[1] != '\n'); ptr2++);

        if(*ptr2)
            ptr2 += 2;

        if(!strncasecmp(ptr, USER_AGENT, sizeof(USER_AGENT)-1)) {
            FIXME("Ignoring User-Agent header\n");
            memmove(ptr, ptr2, strlen(ptr2)+1);
        }else if(!post_len && !strncasecmp(ptr, CONTENT_TYPE, sizeof(CONTENT_TYPE)-1)) {
            TRACE("Ignoring Content-Type header\n");
            memmove(ptr, ptr2, strlen(ptr2)+1);
        }else {
            ptr = ptr2;
        }
    }

    *ptr = 0;
    return ptr-buf;
}

static nsIInputStream *get_post_data_stream(IBindCtx *bctx)
{
    nsIInputStream *ret = NULL;
    IUnknown *unk;
    IBindStatusCallback *callback;
    IServiceProvider *service_provider;
    BINDINFO bindinfo;
    DWORD bindf = 0;
    DWORD post_len = 0, headers_len = 0;
    LPWSTR headers = NULL;
    WCHAR emptystr[] = {0};
    char *data;
    HRESULT hres;

    static WCHAR _BSCB_Holder_[] =
        {'_','B','S','C','B','_','H','o','l','d','e','r','_',0};


    /* FIXME: This should be done in URLMoniker */
    if(!bctx)
        return NULL;

    hres = IBindCtx_GetObjectParam(bctx, _BSCB_Holder_, &unk);
    if(FAILED(hres))
        return NULL;

    hres = IUnknown_QueryInterface(unk, &IID_IBindStatusCallback, (void**)&callback);
    if(FAILED(hres)) {
        IUnknown_Release(unk);
        return NULL;
    }

    hres = IUnknown_QueryInterface(unk, &IID_IServiceProvider, (void**)&service_provider);
    IUnknown_Release(unk);
    if(SUCCEEDED(hres)) {
        IHttpNegotiate *http_negotiate;

        hres = IServiceProvider_QueryService(service_provider, &IID_IHttpNegotiate, &IID_IHttpNegotiate,
                                             (void**)&http_negotiate);
        if(SUCCEEDED(hres)) {
            hres = IHttpNegotiate_BeginningTransaction(http_negotiate, emptystr,
                                                       emptystr, 0, &headers);
            IHttpNegotiate_Release(http_negotiate);

            if(SUCCEEDED(hres) && headers)
                headers_len = WideCharToMultiByte(CP_ACP, 0, headers, -1, NULL, 0, NULL, NULL);
        }

        IServiceProvider_Release(service_provider);
    }

    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(bindinfo);

    hres = IBindStatusCallback_GetBindInfo(callback, &bindf, &bindinfo);

    if(SUCCEEDED(hres) && bindinfo.dwBindVerb == BINDVERB_POST)
        post_len = bindinfo.cbstgmedData;

    if(headers_len || post_len) {
        int len = 0;

        static const char content_length[] = "Content-Length: %u\r\n\r\n";

        data = heap_alloc(headers_len+post_len+sizeof(content_length)+8);

        if(headers_len) {
            WideCharToMultiByte(CP_ACP, 0, headers, -1, data, headers_len, NULL, NULL);
            len = fix_headers(data, post_len);
        }

        if(post_len) {
            sprintf(data+len, content_length, post_len);
            len += strlen(data+len);

            memcpy(data+len, bindinfo.stgmedData.u.hGlobal, post_len);
        }

        TRACE("data = %s\n", debugstr_an(data, len+post_len));

        if(len)
            ret = create_nsstream(data, len+post_len);
    }

    CoTaskMemFree(headers);
    ReleaseBindInfo(&bindinfo);
    IBindStatusCallback_Release(callback);

    return ret;
}

static BOOL use_gecko_script(LPCWSTR url)
{
    static const WCHAR fileW[] = {'f','i','l','e',':'};
    return strncmpiW(fileW, url, sizeof(fileW)/sizeof(WCHAR));
}

void set_current_mon(HTMLDocument *This, IMoniker *mon)
{
    HRESULT hres;

    if(This->mon) {
        IMoniker_Release(This->mon);
        This->mon = NULL;
    }

    if(This->url) {
        CoTaskMemFree(This->url);
        This->url = NULL;
    }

    if(!mon)
        return;

    IMoniker_AddRef(mon);
    This->mon = mon;

    hres = IMoniker_GetDisplayName(mon, NULL, NULL, &This->url);
    if(FAILED(hres))
        WARN("GetDisplayName failed: %08x\n", hres);

    set_script_mode(This, use_gecko_script(This->url) ? SCRIPTMODE_GECKO : SCRIPTMODE_ACTIVESCRIPT);
}

static HRESULT set_moniker(HTMLDocument *This, IMoniker *mon, IBindCtx *pibc, BOOL *bind_complete)
{
    nsChannelBSC *bscallback;
    LPOLESTR url = NULL;
    task_t *task;
    HRESULT hres;
    nsresult nsres;

    if(pibc) {
        IUnknown *unk = NULL;

        /* FIXME:
         * Use params:
         * "__PrecreatedObject"
         * "BIND_CONTEXT_PARAM"
         * "__HTMLLOADOPTIONS"
         * "__DWNBINDINFO"
         * "URL Context"
         * "CBinding Context"
         * "_ITransData_Object_"
         * "_EnumFORMATETC_"
         */

        IBindCtx_GetObjectParam(pibc, (LPOLESTR)SZ_HTML_CLIENTSITE_OBJECTPARAM, &unk);
        if(unk) {
            IOleClientSite *client = NULL;

            hres = IUnknown_QueryInterface(unk, &IID_IOleClientSite, (void**)&client);
            if(SUCCEEDED(hres)) {
                TRACE("Got client site %p\n", client);
                IOleObject_SetClientSite(OLEOBJ(This), client);
                IOleClientSite_Release(client);
            }

            IUnknown_Release(unk);
        }
    }

    This->readystate = READYSTATE_LOADING;
    call_property_onchanged(&This->cp_propnotif, DISPID_READYSTATE);
    update_doc(This, UPDATE_TITLE);

    HTMLDocument_LockContainer(This, TRUE);
    
    hres = IMoniker_GetDisplayName(mon, pibc, NULL, &url);
    if(FAILED(hres)) {
        WARN("GetDiaplayName failed: %08x\n", hres);
        return hres;
    }

    TRACE("got url: %s\n", debugstr_w(url));

    set_current_mon(This, mon);

    if(This->client) {
        VARIANT silent, offline;
        IOleCommandTarget *cmdtrg = NULL;

        hres = get_client_disp_property(This->client, DISPID_AMBIENT_SILENT, &silent);
        if(SUCCEEDED(hres)) {
            if(V_VT(&silent) != VT_BOOL)
                WARN("V_VT(silent) = %d\n", V_VT(&silent));
            else if(V_BOOL(&silent))
                FIXME("silent == true\n");
        }

        hres = get_client_disp_property(This->client,
                DISPID_AMBIENT_OFFLINEIFNOTCONNECTED, &offline);
        if(SUCCEEDED(hres)) {
            if(V_VT(&silent) != VT_BOOL)
                WARN("V_VT(offline) = %d\n", V_VT(&silent));
            else if(V_BOOL(&silent))
                FIXME("offline == true\n");
        }

        hres = IOleClientSite_QueryInterface(This->client, &IID_IOleCommandTarget,
                (void**)&cmdtrg);
        if(SUCCEEDED(hres)) {
            VARIANT var;

            V_VT(&var) = VT_I4;
            V_I4(&var) = 0;
            IOleCommandTarget_Exec(cmdtrg, &CGID_ShellDocView, 37, 0, &var, NULL);

            IOleCommandTarget_Release(cmdtrg);
        }
    }

    bscallback = create_channelbsc(mon);

    if(This->frame) {
        task = heap_alloc(sizeof(task_t));

        task->doc = This;
        task->task_id = TASK_SETPROGRESS;
        task->next = NULL;

        push_task(task);
    }

    task = heap_alloc(sizeof(task_t));

    task->doc = This;
    task->task_id = TASK_SETDOWNLOADSTATE;
    task->next = NULL;

    push_task(task);

    if(This->nscontainer) {
        nsIInputStream *post_data_stream = get_post_data_stream(pibc);

        This->nscontainer->bscallback = bscallback;
        nsres = nsIWebNavigation_LoadURI(This->nscontainer->navigation, url,
                LOAD_FLAGS_NONE, NULL, post_data_stream, NULL);
        This->nscontainer->bscallback = NULL;

        if(post_data_stream)
            nsIInputStream_Release(post_data_stream);

        if(NS_SUCCEEDED(nsres)) {
            /* FIXME: don't return here (URL Moniker needs to be good enough) */

            IUnknown_Release((IUnknown*)bscallback);
            CoTaskMemFree(url);

            if(bind_complete)
                *bind_complete = TRUE;
            return S_OK;
        }else if(nsres != WINE_NS_LOAD_FROM_MONIKER) {
            WARN("LoadURI failed: %08x\n", nsres);
        }
    }

    set_document_bscallback(This, bscallback);
    IUnknown_Release((IUnknown*)bscallback);
    CoTaskMemFree(url);

    if(bind_complete)
        *bind_complete = FALSE;
    return S_OK;
}

static HRESULT get_doc_string(HTMLDocument *This, char **str, DWORD *len)
{
    nsIDOMNode *nsnode;
    LPCWSTR strw;
    nsAString nsstr;
    nsresult nsres;

    if(!This->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMHTMLDocument_QueryInterface(This->nsdoc, &IID_nsIDOMNode, (void**)&nsnode);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMNode failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsAString_Init(&nsstr, NULL);
    nsnode_to_nsstring(nsnode, &nsstr);
    nsIDOMNode_Release(nsnode);

    nsAString_GetData(&nsstr, &strw);
    TRACE("%s\n", debugstr_w(strw));

    *len = WideCharToMultiByte(CP_ACP, 0, strw, -1, NULL, 0, NULL, NULL);
    *str = heap_alloc(*len);
    WideCharToMultiByte(CP_ACP, 0, strw, -1, *str, *len, NULL, NULL);

    nsAString_Finish(&nsstr);

    return S_OK;
}


/**********************************************************
 * IPersistMoniker implementation
 */

#define PERSISTMON_THIS(iface) DEFINE_THIS(HTMLDocument, PersistMoniker, iface)

static HRESULT WINAPI PersistMoniker_QueryInterface(IPersistMoniker *iface, REFIID riid,
                                                            void **ppvObject)
{
    HTMLDocument *This = PERSISTMON_THIS(iface);
    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppvObject);
}

static ULONG WINAPI PersistMoniker_AddRef(IPersistMoniker *iface)
{
    HTMLDocument *This = PERSISTMON_THIS(iface);
    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI PersistMoniker_Release(IPersistMoniker *iface)
{
    HTMLDocument *This = PERSISTMON_THIS(iface);
    return IHTMLDocument2_Release(HTMLDOC(This));
}

static HRESULT WINAPI PersistMoniker_GetClassID(IPersistMoniker *iface, CLSID *pClassID)
{
    HTMLDocument *This = PERSISTMON_THIS(iface);
    return IPersist_GetClassID(PERSIST(This), pClassID);
}

static HRESULT WINAPI PersistMoniker_IsDirty(IPersistMoniker *iface)
{
    HTMLDocument *This = PERSISTMON_THIS(iface);

    TRACE("(%p)\n", This);

    return IPersistStreamInit_IsDirty(PERSTRINIT(This));
}

static HRESULT WINAPI PersistMoniker_Load(IPersistMoniker *iface, BOOL fFullyAvailable,
        IMoniker *pimkName, LPBC pibc, DWORD grfMode)
{
    HTMLDocument *This = PERSISTMON_THIS(iface);
    BOOL bind_complete = FALSE;
    HRESULT hres;

    TRACE("(%p)->(%x %p %p %08x)\n", This, fFullyAvailable, pimkName, pibc, grfMode);

    hres = set_moniker(This, pimkName, pibc, &bind_complete);
    if(FAILED(hres))
        return hres;

    if(!bind_complete)
        return start_binding(This, (BSCallback*)This->bscallback, pibc);

    return S_OK;
}

static HRESULT WINAPI PersistMoniker_Save(IPersistMoniker *iface, IMoniker *pimkName,
        LPBC pbc, BOOL fRemember)
{
    HTMLDocument *This = PERSISTMON_THIS(iface);
    FIXME("(%p)->(%p %p %x)\n", This, pimkName, pbc, fRemember);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistMoniker_SaveCompleted(IPersistMoniker *iface, IMoniker *pimkName, LPBC pibc)
{
    HTMLDocument *This = PERSISTMON_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pimkName, pibc);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistMoniker_GetCurMoniker(IPersistMoniker *iface, IMoniker **ppimkName)
{
    HTMLDocument *This = PERSISTMON_THIS(iface);

    TRACE("(%p)->(%p)\n", This, ppimkName);

    if(!This->mon)
        return E_UNEXPECTED;

    IMoniker_AddRef(This->mon);
    *ppimkName = This->mon;
    return S_OK;
}

static const IPersistMonikerVtbl PersistMonikerVtbl = {
    PersistMoniker_QueryInterface,
    PersistMoniker_AddRef,
    PersistMoniker_Release,
    PersistMoniker_GetClassID,
    PersistMoniker_IsDirty,
    PersistMoniker_Load,
    PersistMoniker_Save,
    PersistMoniker_SaveCompleted,
    PersistMoniker_GetCurMoniker
};

/**********************************************************
 * IMonikerProp implementation
 */

#define MONPROP_THIS(iface) DEFINE_THIS(HTMLDocument, MonikerProp, iface)

static HRESULT WINAPI MonikerProp_QueryInterface(IMonikerProp *iface, REFIID riid, void **ppvObject)
{
    HTMLDocument *This = MONPROP_THIS(iface);
    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppvObject);
}

static ULONG WINAPI MonikerProp_AddRef(IMonikerProp *iface)
{
    HTMLDocument *This = MONPROP_THIS(iface);
    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI MonikerProp_Release(IMonikerProp *iface)
{
    HTMLDocument *This = MONPROP_THIS(iface);
    return IHTMLDocument_Release(HTMLDOC(This));
}

static HRESULT WINAPI MonikerProp_PutProperty(IMonikerProp *iface, MONIKERPROPERTY mkp, LPCWSTR val)
{
    HTMLDocument *This = MONPROP_THIS(iface);

    TRACE("(%p)->(%d %s)\n", This, mkp, debugstr_w(val));

    switch(mkp) {
    case MIMETYPEPROP:
        heap_free(This->mime);
        This->mime = heap_strdupW(val);
        break;

    case CLASSIDPROP:
        break;

    default:
        FIXME("mkp %d\n", mkp);
        return E_NOTIMPL;
    }

    return S_OK;
}

static const IMonikerPropVtbl MonikerPropVtbl = {
    MonikerProp_QueryInterface,
    MonikerProp_AddRef,
    MonikerProp_Release,
    MonikerProp_PutProperty
};

/**********************************************************
 * IPersistFile implementation
 */

#define PERSISTFILE_THIS(iface) DEFINE_THIS(HTMLDocument, PersistFile, iface)

static HRESULT WINAPI PersistFile_QueryInterface(IPersistFile *iface, REFIID riid, void **ppvObject)
{
    HTMLDocument *This = PERSISTFILE_THIS(iface);
    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppvObject);
}

static ULONG WINAPI PersistFile_AddRef(IPersistFile *iface)
{
    HTMLDocument *This = PERSISTFILE_THIS(iface);
    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI PersistFile_Release(IPersistFile *iface)
{
    HTMLDocument *This = PERSISTFILE_THIS(iface);
    return IHTMLDocument2_Release(HTMLDOC(This));
}

static HRESULT WINAPI PersistFile_GetClassID(IPersistFile *iface, CLSID *pClassID)
{
    HTMLDocument *This = PERSISTFILE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pClassID);

    if(!pClassID)
        return E_INVALIDARG;

    *pClassID = CLSID_HTMLDocument;
    return S_OK;
}

static HRESULT WINAPI PersistFile_IsDirty(IPersistFile *iface)
{
    HTMLDocument *This = PERSISTFILE_THIS(iface);

    TRACE("(%p)\n", This);

    return IPersistStreamInit_IsDirty(PERSTRINIT(This));
}

static HRESULT WINAPI PersistFile_Load(IPersistFile *iface, LPCOLESTR pszFileName, DWORD dwMode)
{
    HTMLDocument *This = PERSISTFILE_THIS(iface);
    FIXME("(%p)->(%s %08x)\n", This, debugstr_w(pszFileName), dwMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistFile_Save(IPersistFile *iface, LPCOLESTR pszFileName, BOOL fRemember)
{
    HTMLDocument *This = PERSISTFILE_THIS(iface);
    char *str;
    DWORD len, written=0;
    HANDLE file;
    HRESULT hres;

    TRACE("(%p)->(%s %x)\n", This, debugstr_w(pszFileName), fRemember);

    file = CreateFileW(pszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    if(file == INVALID_HANDLE_VALUE) {
        WARN("Could not create file: %u\n", GetLastError());
        return E_FAIL;
    }

    hres = get_doc_string(This, &str, &len);
    if(SUCCEEDED(hres))
        WriteFile(file, str, len, &written, NULL);

    CloseHandle(file);
    return hres;
}

static HRESULT WINAPI PersistFile_SaveCompleted(IPersistFile *iface, LPCOLESTR pszFileName)
{
    HTMLDocument *This = PERSISTFILE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pszFileName));
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistFile_GetCurFile(IPersistFile *iface, LPOLESTR *pszFileName)
{
    HTMLDocument *This = PERSISTFILE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pszFileName);
    return E_NOTIMPL;
}

static const IPersistFileVtbl PersistFileVtbl = {
    PersistFile_QueryInterface,
    PersistFile_AddRef,
    PersistFile_Release,
    PersistFile_GetClassID,
    PersistFile_IsDirty,
    PersistFile_Load,
    PersistFile_Save,
    PersistFile_SaveCompleted,
    PersistFile_GetCurFile
};

#define PERSTRINIT_THIS(iface) DEFINE_THIS(HTMLDocument, PersistStreamInit, iface)

static HRESULT WINAPI PersistStreamInit_QueryInterface(IPersistStreamInit *iface,
                                                       REFIID riid, void **ppv)
{
    HTMLDocument *This = PERSTRINIT_THIS(iface);
    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppv);
}

static ULONG WINAPI PersistStreamInit_AddRef(IPersistStreamInit *iface)
{
    HTMLDocument *This = PERSTRINIT_THIS(iface);
    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI PersistStreamInit_Release(IPersistStreamInit *iface)
{
    HTMLDocument *This = PERSTRINIT_THIS(iface);
    return IHTMLDocument2_Release(HTMLDOC(This));
}

static HRESULT WINAPI PersistStreamInit_GetClassID(IPersistStreamInit *iface, CLSID *pClassID)
{
    HTMLDocument *This = PERSTRINIT_THIS(iface);
    return IPersist_GetClassID(PERSIST(This), pClassID);
}

static HRESULT WINAPI PersistStreamInit_IsDirty(IPersistStreamInit *iface)
{
    HTMLDocument *This = PERSTRINIT_THIS(iface);

    TRACE("(%p)\n", This);

    if(This->usermode == EDITMODE)
        return editor_is_dirty(This);

    return S_FALSE;
}

static HRESULT WINAPI PersistStreamInit_Load(IPersistStreamInit *iface, LPSTREAM pStm)
{
    HTMLDocument *This = PERSTRINIT_THIS(iface);
    IMoniker *mon;
    HRESULT hres;

    static const WCHAR about_blankW[] = {'a','b','o','u','t',':','b','l','a','n','k',0};

    TRACE("(%p)->(%p)\n", This, pStm);

    hres = CreateURLMoniker(NULL, about_blankW, &mon);
    if(FAILED(hres)) {
        WARN("CreateURLMoniker failed: %08x\n", hres);
        return hres;
    }

    hres = set_moniker(This, mon, NULL, NULL);
    IMoniker_Release(mon);
    if(FAILED(hres))
        return hres;

    return channelbsc_load_stream(This->bscallback, pStm);
}

static HRESULT WINAPI PersistStreamInit_Save(IPersistStreamInit *iface, LPSTREAM pStm,
                                             BOOL fClearDirty)
{
    HTMLDocument *This = PERSTRINIT_THIS(iface);
    char *str;
    DWORD len, written=0;
    HRESULT hres;

    TRACE("(%p)->(%p %x)\n", This, pStm, fClearDirty);

    hres = get_doc_string(This, &str, &len);
    if(FAILED(hres))
        return hres;

    hres = IStream_Write(pStm, str, len, &written);
    if(FAILED(hres))
        FIXME("Write failed: %08x\n", hres);

    heap_free(str);

    if(fClearDirty)
        set_dirty(This, VARIANT_FALSE);

    return S_OK;
}

static HRESULT WINAPI PersistStreamInit_GetSizeMax(IPersistStreamInit *iface,
                                                   ULARGE_INTEGER *pcbSize)
{
    HTMLDocument *This = PERSTRINIT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pcbSize);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStreamInit_InitNew(IPersistStreamInit *iface)
{
    HTMLDocument *This = PERSTRINIT_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

#undef PERSTRINIT_THIS

static const IPersistStreamInitVtbl PersistStreamInitVtbl = {
    PersistStreamInit_QueryInterface,
    PersistStreamInit_AddRef,
    PersistStreamInit_Release,
    PersistStreamInit_GetClassID,
    PersistStreamInit_IsDirty,
    PersistStreamInit_Load,
    PersistStreamInit_Save,
    PersistStreamInit_GetSizeMax,
    PersistStreamInit_InitNew
};

void HTMLDocument_Persist_Init(HTMLDocument *This)
{
    This->lpPersistMonikerVtbl = &PersistMonikerVtbl;
    This->lpPersistFileVtbl = &PersistFileVtbl;
    This->lpMonikerPropVtbl = &MonikerPropVtbl;
    This->lpPersistStreamInitVtbl = &PersistStreamInitVtbl;

    This->bscallback = NULL;
    This->mon = NULL;
    This->url = NULL;
    This->mime = NULL;
}
