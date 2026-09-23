/*
 * Copyright 2012 Jacek Caban for CodeWeavers
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
#include "winuser.h"
#include "ole2.h"
#include "shlobj.h"

#include "wine/debug.h"

#include "mshtml_private.h"
#include "htmlevent.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

/* Native defaults to 5 million chars per origin */
enum { MAX_QUOTA = 5000000 };

typedef struct {
    DispatchEx dispex;
    IHTMLStorage IHTMLStorage_iface;
    unsigned num_props;
    BSTR *props;
    HTMLInnerWindow *window;
    struct session_map_entry *session_storage;
    WCHAR *filename;
    HANDLE mutex;
} HTMLStorage;

struct session_map_entry {
    struct wine_rb_entry entry;
    struct wine_rb_tree data_map;
    struct list data_list;        /* for key() */
    UINT ref;
    UINT quota;
    UINT num_keys;
    UINT origin_len;
    WCHAR origin[1];
};

int session_storage_map_cmp(const void *key, const struct wine_rb_entry *entry)
{
    struct session_map_entry *p = WINE_RB_ENTRY_VALUE(entry, struct session_map_entry, entry);
    UINT len = SysStringLen((BSTR)key);

    return (len != p->origin_len) ? (len - p->origin_len) : memcmp(key, p->origin, len * sizeof(WCHAR));
}

struct session_entry
{
    struct wine_rb_entry entry;
    struct list list_entry;
    BSTR value;
    WCHAR key[1];
};

static int session_entry_cmp(const void *key, const struct wine_rb_entry *entry)
{
    struct session_entry *data = WINE_RB_ENTRY_VALUE(entry, struct session_entry, entry);
    return wcscmp(key, data->key);
}

static struct session_map_entry *grab_session_map_entry(BSTR origin)
{
    struct session_map_entry *entry;
    struct wine_rb_entry *rb_entry;
    thread_data_t *thread_data;
    UINT origin_len;

    thread_data = get_thread_data(TRUE);
    if(!thread_data)
        return NULL;

    rb_entry = wine_rb_get(&thread_data->session_storage_map, origin);
    if(rb_entry) {
        entry = WINE_RB_ENTRY_VALUE(rb_entry, struct session_map_entry, entry);
        entry->ref++;
        return entry;
    }

    origin_len = SysStringLen(origin);
    entry = malloc(FIELD_OFFSET(struct session_map_entry, origin[origin_len]));
    if(!entry)
        return NULL;
    wine_rb_init(&entry->data_map, session_entry_cmp);
    list_init(&entry->data_list);
    entry->ref = 1;
    entry->quota = MAX_QUOTA;
    entry->num_keys = 0;
    entry->origin_len = origin_len;
    memcpy(entry->origin, origin, origin_len * sizeof(WCHAR));

    wine_rb_put(&thread_data->session_storage_map, origin, &entry->entry);
    return entry;
}

static void release_session_map_entry(struct session_map_entry *entry)
{
    if(!entry || --entry->ref || entry->num_keys)
        return;

    wine_rb_remove(&get_thread_data(FALSE)->session_storage_map, &entry->entry);
    free(entry);
}

static HRESULT get_session_entry(struct session_map_entry *entry, const WCHAR *name, BOOL create, struct session_entry **ret)
{
    const WCHAR *key = name ? name : L"";
    struct wine_rb_entry *rb_entry;
    struct session_entry *data;
    UINT key_len;

    rb_entry = wine_rb_get(&entry->data_map, key);
    if(rb_entry) {
        *ret = WINE_RB_ENTRY_VALUE(rb_entry, struct session_entry, entry);
        return S_OK;
    }

    if(!create) {
        *ret = NULL;
        return S_OK;
    }

    key_len = wcslen(key);
    if(entry->quota < key_len)
        return E_OUTOFMEMORY;  /* native returns this when quota is exceeded */
    if(!(data = malloc(FIELD_OFFSET(struct session_entry, key[key_len + 1]))))
        return E_OUTOFMEMORY;
    data->value = NULL;
    memcpy(data->key, key, (key_len + 1) * sizeof(WCHAR));

    entry->quota -= key_len;
    entry->num_keys++;
    list_add_tail(&entry->data_list, &data->list_entry);
    wine_rb_put(&entry->data_map, key, &data->entry);
    *ret = data;
    return S_OK;
}

static void clear_session_storage(struct session_map_entry *entry)
{
    struct session_entry *iter, *iter2;

    LIST_FOR_EACH_ENTRY_SAFE(iter, iter2, &entry->data_list, struct session_entry, list_entry) {
        SysFreeString(iter->value);
        free(iter);
    }
    wine_rb_destroy(&entry->data_map, NULL, NULL);
    list_init(&entry->data_list);
    entry->quota = MAX_QUOTA;
    entry->num_keys = 0;
}

void destroy_session_storage(thread_data_t *thread_data)
{
    struct session_map_entry *iter, *iter2;

    WINE_RB_FOR_EACH_ENTRY_DESTRUCTOR(iter, iter2, &thread_data->session_storage_map, struct session_map_entry, entry) {
        clear_session_storage(iter);
        free(iter);
    }
}

static void release_props(HTMLStorage *This)
{
    BSTR *prop = This->props, *end = prop + This->num_props;
    while(prop != end) {
        SysFreeString(*prop);
        prop++;
    }
    free(This->props);
}

static inline HTMLStorage *impl_from_IHTMLStorage(IHTMLStorage *iface)
{
    return CONTAINING_RECORD(iface, HTMLStorage, IHTMLStorage_iface);
}

static HRESULT build_session_origin(IUri*,BSTR,BSTR*);

struct storage_event_task {
    event_task_t header;
    DOMEvent *event;
};

static void storage_event_proc(event_task_t *_task)
{
    struct storage_event_task *task = (struct storage_event_task*)_task;
    HTMLInnerWindow *window = task->header.window;
    DOMEvent *event = task->event;
    compat_mode_t compat_mode;
    VARIANT_BOOL cancelled;
    HRESULT hres;
    VARIANT var;

    if(event->event_id == EVENTID_STORAGE && (compat_mode = dispex_compat_mode(&window->event_target.dispex)) >= COMPAT_MODE_IE9) {
        dispatch_event(&window->event_target, event);
        if(window->doc) {
            hres = create_event_obj(event, window->doc, (IHTMLEventObj**)&V_DISPATCH(&var));
            if(SUCCEEDED(hres)) {
                V_VT(&var) = VT_DISPATCH;
                fire_event(&window->doc->node, L"onstorage", &var, &cancelled);
                IDispatch_Release(V_DISPATCH(&var));
            }
        }
    }else if(window->doc) {
        dispatch_event(&window->doc->node.event_target, event);
    }
}

static void storage_event_destr(event_task_t *_task)
{
    struct storage_event_task *task = (struct storage_event_task*)_task;
    IDOMEvent_Release(&task->event->IDOMEvent_iface);
}

struct send_storage_event_ctx {
    HTMLInnerWindow *skip_window;
    const WCHAR *origin;
    UINT origin_len;
    BSTR key;
    BSTR old_value;
    BSTR new_value;
    BSTR url;
};

static HRESULT push_storage_event_task(struct send_storage_event_ctx *ctx, HTMLInnerWindow *window, BOOL commit)
{
    struct storage_event_task *task;
    DOMEvent *event;
    HRESULT hres;

    hres = create_storage_event(window->doc, ctx->key, ctx->old_value, ctx->new_value, ctx->url, commit, &event);
    if(FAILED(hres))
        return hres;

    if(!(task = malloc(sizeof(*task)))) {
        IDOMEvent_Release(&event->IDOMEvent_iface);
        return E_OUTOFMEMORY;
    }

    task->event = event;
    return push_event_task(&task->header, window, storage_event_proc, storage_event_destr, window->task_magic);
}

static HRESULT send_storage_event_impl(struct send_storage_event_ctx *ctx, HTMLInnerWindow *window)
{
    HTMLOuterWindow *child;
    const WCHAR *origin;
    UINT origin_len;
    BOOL matches;
    HRESULT hres;
    BSTR bstr;

    if(!window)
        return S_OK;

    LIST_FOR_EACH_ENTRY(child, &window->children, HTMLOuterWindow, sibling_entry) {
        hres = send_storage_event_impl(ctx, child->base.inner_window);
        if(FAILED(hres))
            return hres;
    }

    if(window == ctx->skip_window)
        return S_OK;

    /* Try it quick from session storage first, if available */
    if(window->session_storage) {
        HTMLStorage *storage = impl_from_IHTMLStorage(window->session_storage);
        origin = storage->session_storage->origin;
        origin_len = ctx->skip_window ? wcslen(origin) : storage->session_storage->origin_len;
        bstr = NULL;
    }else if(!window->base.outer_window->uri) {
        return S_OK;
    }else {
        hres = IUri_GetHost(window->base.outer_window->uri, &bstr);
        if(hres != S_OK) {
            if(SUCCEEDED(hres))
                SysFreeString(bstr);
            return S_OK;
        }
        if(ctx->skip_window)
            _wcslwr(bstr);
        else {
            BSTR tmp = bstr;
            hres = build_session_origin(window->base.outer_window->uri, tmp, &bstr);
            SysFreeString(tmp);
            if(hres != S_OK) {
                if(SUCCEEDED(hres))
                    SysFreeString(bstr);
                return S_OK;
            }
        }
        origin = bstr;
        origin_len = SysStringLen(bstr);
    }

    matches = (origin_len == ctx->origin_len && !memcmp(origin, ctx->origin, origin_len * sizeof(WCHAR)));
    SysFreeString(bstr);

    return matches ? push_storage_event_task(ctx, window, FALSE) : S_OK;
}

/* Takes ownership of old_value */
static HRESULT send_storage_event(HTMLStorage *storage, BSTR key, BSTR old_value, BSTR new_value)
{
    HTMLInnerWindow *window = storage->window;
    struct send_storage_event_ctx ctx;
    HTMLOuterWindow *top_window;
    BSTR hostname = NULL;
    HRESULT hres = S_OK;

    ctx.url = NULL;

    /* FIXME: Events are actually sent to the current window on native, even if we're detached. */
    if(!window->base.outer_window)
        goto done;

    if(window->base.outer_window->uri_nofrag) {
        hres = IUri_GetDisplayUri(window->base.outer_window->uri_nofrag, &ctx.url);
        if(hres != S_OK)
            goto done;
    }

    get_top_window(window->base.outer_window, &top_window);
    ctx.key = key;
    ctx.old_value = old_value;
    ctx.new_value = new_value;
    if(!storage->filename) {
        ctx.origin = storage->session_storage->origin;
        ctx.origin_len = storage->session_storage->origin_len;
        ctx.skip_window = NULL;
    }else {
        hres = IUri_GetHost(window->base.outer_window->uri, &hostname);
        if(hres != S_OK)
            goto done;
        _wcslwr(hostname);
        ctx.origin = hostname;
        ctx.origin_len = SysStringLen(hostname);
        ctx.skip_window = top_window->base.inner_window;  /* localStorage on native skips top window */
    }
    hres = send_storage_event_impl(&ctx, top_window->base.inner_window);

    if(ctx.skip_window && hres == S_OK)
        hres = push_storage_event_task(&ctx, window, TRUE);

done:
    SysFreeString(ctx.url);
    SysFreeString(hostname);
    SysFreeString(old_value);
    return FAILED(hres) ? hres : S_OK;
}

DISPEX_IDISPATCH_IMPL(HTMLStorage, IHTMLStorage, impl_from_IHTMLStorage(iface)->dispex)

static BOOL create_path(const WCHAR *path)
{
    BOOL ret = TRUE;
    WCHAR *new_path;
    int len;

    new_path = malloc((wcslen(path) + 1) * sizeof(WCHAR));
    if(!new_path)
        return FALSE;
    wcscpy(new_path, path);

    while((len = wcslen(new_path)) && new_path[len - 1] == '\\')
    new_path[len - 1] = 0;

    while(!CreateDirectoryW(new_path, NULL)) {
        WCHAR *slash;
        DWORD error = GetLastError();
        if(error == ERROR_ALREADY_EXISTS) break;
        if(error != ERROR_PATH_NOT_FOUND) {
            ret = FALSE;
            break;
        }
        slash = wcsrchr(new_path, '\\');
        if(!slash) {
            ret = FALSE;
            break;
        }
        len = slash - new_path;
        new_path[len] = 0;
        if(!create_path(new_path)) {
            ret = FALSE;
            break;
        }
        new_path[len] = '\\';
    }
    free(new_path);
    return ret;
}

static HRESULT open_document(const WCHAR *filename, IXMLDOMDocument **ret)
{
    IXMLDOMDocument *doc = NULL;
    HRESULT hres = E_OUTOFMEMORY;
    WCHAR *ptr, *path;
    VARIANT var;
    VARIANT_BOOL success;

    path = wcsdup(filename);
    if(!path)
        return E_OUTOFMEMORY;

    *(ptr = wcsrchr(path, '\\')) = 0;
    if(!create_path(path))
        goto done;

    if(GetFileAttributesW(filename) == INVALID_FILE_ATTRIBUTES) {
        DWORD count;
        HANDLE file = CreateFileW(filename, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
        if(file == INVALID_HANDLE_VALUE) {
            hres = HRESULT_FROM_WIN32(GetLastError());
            goto done;
        }
        if(!WriteFile(file, "<root/>", sizeof("<root/>") - 1, &count, NULL)) {
            CloseHandle(file);
            hres = HRESULT_FROM_WIN32(GetLastError());
            goto done;
        }
        CloseHandle(file);
    }

    hres = CoCreateInstance(&CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (void**)&doc);
    if(hres != S_OK)
        goto done;

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(filename);
    if(!V_BSTR(&var)) {
        hres = E_OUTOFMEMORY;
        goto done;
    }

    hres = IXMLDOMDocument_load(doc, var, &success);
    if(hres == S_FALSE || success == VARIANT_FALSE)
        hres = E_FAIL;

    SysFreeString(V_BSTR(&var));

done:
    free(path);
    if(hres == S_OK)
        *ret = doc;
    else if(doc)
        IXMLDOMDocument_Release(doc);

    return hres;
}

static HRESULT get_root_node(IXMLDOMDocument *doc, IXMLDOMNode **root)
{
    HRESULT hres;
    BSTR str;

    str = SysAllocString(L"root");
    if(!str)
        return E_OUTOFMEMORY;

    hres = IXMLDOMDocument_selectSingleNode(doc, str, root);
    SysFreeString(str);
    return hres;
}

static HRESULT get_node_list(const WCHAR *filename, IXMLDOMNodeList **node_list)
{
    IXMLDOMDocument *doc;
    IXMLDOMNode *root;
    HRESULT hres;
    BSTR query;

    hres = open_document(filename, &doc);
    if(hres != S_OK)
        return hres;

    hres = get_root_node(doc, &root);
    IXMLDOMDocument_Release(doc);
    if(hres != S_OK)
        return hres;

    if(!(query = SysAllocString(L"item")))
        hres = E_OUTOFMEMORY;
    else {
        hres = IXMLDOMNode_selectNodes(root, query, node_list);
        SysFreeString(query);
    }
    IXMLDOMNode_Release(root);
    return hres;
}

static HRESULT WINAPI HTMLStorage_get_length(IHTMLStorage *iface, LONG *p)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    IXMLDOMNodeList *node_list;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->filename) {
        *p = This->session_storage->num_keys;
        return S_OK;
    }

    WaitForSingleObject(This->mutex, INFINITE);
    hres = get_node_list(This->filename, &node_list);
    if(SUCCEEDED(hres)) {
        hres = IXMLDOMNodeList_get_length(node_list, p);
        IXMLDOMNodeList_Release(node_list);
    }
    ReleaseMutex(This->mutex);

    return hres;
}

static HRESULT WINAPI HTMLStorage_get_remainingSpace(IHTMLStorage *iface, LONG *p)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->filename) {
        *p = This->session_storage->quota;
        return S_OK;
    }

    FIXME("local storage not supported\n");
    return E_NOTIMPL;
}

static HRESULT get_key(const WCHAR *filename, LONG index, BSTR *ret)
{
    IXMLDOMNodeList *node_list;
    IXMLDOMElement *elem;
    IXMLDOMNode *node;
    HRESULT hres;
    VARIANT key;

    hres = get_node_list(filename, &node_list);
    if(FAILED(hres))
        return hres;

    hres = IXMLDOMNodeList_get_item(node_list, index, &node);
    IXMLDOMNodeList_Release(node_list);
    if(hres != S_OK)
        return FAILED(hres) ? hres : E_INVALIDARG;

    hres = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMElement, (void**)&elem);
    IXMLDOMNode_Release(node);
    if(hres != S_OK)
        return E_INVALIDARG;

    hres = IXMLDOMElement_getAttribute(elem, (BSTR)L"name", &key);
    IXMLDOMElement_Release(elem);
    if(FAILED(hres))
        return hres;

    if(V_VT(&key) != VT_BSTR) {
        FIXME("non-string key %s\n", debugstr_variant(&key));
        VariantClear(&key);
        return E_NOTIMPL;
    }

    *ret = V_BSTR(&key);
    return S_OK;
}

static HRESULT WINAPI HTMLStorage_key(IHTMLStorage *iface, LONG lIndex, BSTR *p)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    struct session_entry *session_entry;
    HRESULT hres;

    TRACE("(%p)->(%ld %p)\n", This, lIndex, p);

    if(!This->filename) {
        struct list *entry = &This->session_storage->data_list;
        unsigned i = 0;

        if(lIndex >= This->session_storage->num_keys)
            return E_INVALIDARG;

        do entry = entry->next; while(i++ < lIndex);
        session_entry = LIST_ENTRY(entry, struct session_entry, list_entry);

        *p = SysAllocString(session_entry->key);
        return *p ? S_OK : E_OUTOFMEMORY;
    }

    WaitForSingleObject(This->mutex, INFINITE);
    hres = get_key(This->filename, lIndex, p);
    ReleaseMutex(This->mutex);

    return hres;
}

static BSTR build_query(const WCHAR *key)
{
    static const WCHAR fmt[] = L"item[@name='%s']";
    const WCHAR *str = key ? key : L"";
    UINT len = ARRAY_SIZE(fmt) + wcslen(str);
    BSTR ret = SysAllocStringLen(NULL, len);

    if(ret) swprintf(ret, len, fmt, str);
    return ret;
}

static HRESULT get_item(const WCHAR *filename, BSTR key, VARIANT *value)
{
    IXMLDOMDocument *doc;
    BSTR query = NULL;
    IXMLDOMNode *root = NULL, *node = NULL;
    IXMLDOMElement *elem = NULL;
    HRESULT hres;

    hres = open_document(filename, &doc);
    if(hres != S_OK)
        return hres;

    hres = get_root_node(doc, &root);
    if(hres != S_OK)
        goto done;

    query = build_query(key);
    if(!query) {
        hres = E_OUTOFMEMORY;
        goto done;
    }

    hres = IXMLDOMNode_selectSingleNode(root, query, &node);
    if(hres == S_OK) {
        hres = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMElement, (void**)&elem);
        if(hres != S_OK)
            goto done;

        hres = IXMLDOMElement_getAttribute(elem, (BSTR)L"value", value);
    }else {
        V_VT(value) = VT_NULL;
        hres = S_OK;
    }

done:
    SysFreeString(query);
    if(root)
        IXMLDOMNode_Release(root);
    if(node)
        IXMLDOMNode_Release(node);
    if(elem)
        IXMLDOMElement_Release(elem);
    IXMLDOMDocument_Release(doc);
    return hres;
}

static HRESULT WINAPI HTMLStorage_getItem(IHTMLStorage *iface, BSTR bstrKey, VARIANT *value)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    struct session_entry *session_entry;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(bstrKey), value);

    if(!value)
        return E_POINTER;

    if(!This->filename) {
        hres = get_session_entry(This->session_storage, bstrKey, FALSE, &session_entry);
        if(SUCCEEDED(hres)) {
            if(!session_entry || !session_entry->value)
                V_VT(value) = VT_NULL;
            else {
                V_VT(value) = VT_BSTR;
                V_BSTR(value) = SysAllocStringLen(session_entry->value, SysStringLen(session_entry->value));
                hres = V_BSTR(value) ? S_OK : E_OUTOFMEMORY;
            }
        }
        return hres;
    }

    WaitForSingleObject(This->mutex, INFINITE);
    hres = get_item(This->filename, bstrKey, value);
    ReleaseMutex(This->mutex);

    return hres;
}

static HRESULT set_attribute(IXMLDOMElement *elem, const WCHAR *name, BSTR value)
{
    BSTR str;
    VARIANT var;
    HRESULT hres;

    str = SysAllocString(name);
    if(!str)
        return E_OUTOFMEMORY;
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = value;

    hres = IXMLDOMElement_setAttribute(elem, str, var);
    SysFreeString(str);
    return hres;
}

static HRESULT save_document(IXMLDOMDocument *doc, const WCHAR *filename)
{
    VARIANT var;
    HRESULT hres;

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(filename);
    if(!V_BSTR(&var))
        return E_OUTOFMEMORY;

    hres = IXMLDOMDocument_save(doc, var);
    SysFreeString(V_BSTR(&var));
    return hres;
}

static HRESULT set_item(const WCHAR *filename, BSTR key, BSTR value, BSTR *old_value)
{
    IXMLDOMDocument *doc;
    IXMLDOMNode *root = NULL, *node = NULL;
    IXMLDOMElement *elem = NULL;
    BSTR query = NULL;
    HRESULT hres;

    *old_value = NULL;
    hres = open_document(filename, &doc);
    if(hres != S_OK)
        return hres;

    hres = get_root_node(doc, &root);
    if(hres != S_OK)
        goto done;

    query = build_query(key);
    if(!query) {
        hres = E_OUTOFMEMORY;
        goto done;
    }

    hres = IXMLDOMNode_selectSingleNode(root, query, &node);
    if(hres == S_OK) {
        VARIANT old;

        hres = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMElement, (void**)&elem);
        if(hres != S_OK)
            goto done;

        hres = IXMLDOMElement_getAttribute(elem, (BSTR)L"value", &old);
        if(hres == S_OK) {
            if(V_VT(&old) == VT_BSTR)
                *old_value = V_BSTR(&old);
            else
                VariantClear(&old);
        }

        hres = set_attribute(elem, L"value", value);
        if(hres != S_OK)
            goto done;
    }else {
        BSTR str = SysAllocString(L"item");
        hres = IXMLDOMDocument_createElement(doc, str, &elem);
        SysFreeString(str);
        if(hres != S_OK)
            goto done;

        hres = set_attribute(elem, L"name", key);
        if(hres != S_OK)
            goto done;

        hres = set_attribute(elem, L"value", value);
        if(hres != S_OK)
            goto done;

        hres = IXMLDOMNode_appendChild(root, (IXMLDOMNode*)elem, NULL);
        if(hres != S_OK)
            goto done;
    }

    hres = save_document(doc, filename);

done:
    SysFreeString(query);
    if(root)
        IXMLDOMNode_Release(root);
    if(node)
        IXMLDOMNode_Release(node);
    if(elem)
        IXMLDOMElement_Release(elem);
    IXMLDOMDocument_Release(doc);
    if(hres != S_OK)
        SysFreeString(*old_value);
    return hres;
}

static HRESULT WINAPI HTMLStorage_setItem(IHTMLStorage *iface, BSTR bstrKey, BSTR bstrValue)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    struct session_entry *session_entry;
    BSTR old_value;
    HRESULT hres;

    TRACE("(%p)->(%s %s)\n", This, debugstr_w(bstrKey), debugstr_w(bstrValue));

    if(!This->filename) {
        UINT value_len = bstrValue ? wcslen(bstrValue) : 0;
        BSTR value = SysAllocStringLen(bstrValue, value_len);
        if(!value)
            return E_OUTOFMEMORY;

        hres = get_session_entry(This->session_storage, bstrKey, TRUE, &session_entry);
        if(FAILED(hres))
            SysFreeString(value);
        else {
            UINT old_len = SysStringLen(session_entry->value);
            if(old_len < value_len && This->session_storage->quota < value_len - old_len) {
                SysFreeString(value);
                return E_OUTOFMEMORY;  /* native returns this when quota is exceeded */
            }
            This->session_storage->quota -= value_len - old_len;
            old_value = session_entry->value;
            session_entry->value = value;

            hres = send_storage_event(This, bstrKey, old_value, value);
        }
        return hres;
    }

    WaitForSingleObject(This->mutex, INFINITE);
    hres = set_item(This->filename, bstrKey, bstrValue, &old_value);
    ReleaseMutex(This->mutex);

    if(hres == S_OK)
        hres = send_storage_event(This, bstrKey, old_value, bstrValue);
    return hres;
}

static HRESULT remove_item(const WCHAR *filename, BSTR key, BSTR *old_value, BOOL *changed)
{
    IXMLDOMDocument *doc;
    IXMLDOMNode *root = NULL, *node = NULL;
    BSTR query = NULL;
    HRESULT hres;

    *old_value = NULL;
    *changed = FALSE;
    hres = open_document(filename, &doc);
    if(hres != S_OK)
        return hres;

    hres = get_root_node(doc, &root);
    if(hres != S_OK)
        goto done;

    query = build_query(key);
    if(!query) {
        hres = E_OUTOFMEMORY;
        goto done;
    }

    hres = IXMLDOMNode_selectSingleNode(root, query, &node);
    if(hres == S_OK) {
        IXMLDOMElement *elem;
        VARIANT old;

        hres = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMElement, (void**)&elem);
        if(hres == S_OK) {
            hres = IXMLDOMElement_getAttribute(elem, (BSTR)L"value", &old);
            if(hres == S_OK) {
                if(V_VT(&old) == VT_BSTR)
                    *old_value = V_BSTR(&old);
                else
                    VariantClear(&old);
            }
            IXMLDOMElement_Release(elem);
            *changed = TRUE;
        }

        hres = IXMLDOMNode_removeChild(root, node, NULL);
        if(hres != S_OK)
            goto done;
    }

    hres = save_document(doc, filename);

done:
    SysFreeString(query);
    if(root)
        IXMLDOMNode_Release(root);
    if(node)
        IXMLDOMNode_Release(node);
    IXMLDOMDocument_Release(doc);
    if(hres != S_OK || !changed)
        SysFreeString(*old_value);
    return hres;
}

static HRESULT WINAPI HTMLStorage_removeItem(IHTMLStorage *iface, BSTR bstrKey)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    struct session_entry *session_entry;
    BSTR old_value;
    HRESULT hres;
    BOOL changed;

    TRACE("(%p)->(%s)\n", This, debugstr_w(bstrKey));

    if(!This->filename) {
        hres = get_session_entry(This->session_storage, bstrKey, FALSE, &session_entry);
        if(SUCCEEDED(hres) && session_entry) {
            This->session_storage->quota += wcslen(session_entry->key) + SysStringLen(session_entry->value);
            This->session_storage->num_keys--;
            list_remove(&session_entry->list_entry);
            wine_rb_remove(&This->session_storage->data_map, &session_entry->entry);
            old_value = session_entry->value;
            free(session_entry);

            hres = send_storage_event(This, bstrKey, old_value, NULL);
        }
        return hres;
    }

    WaitForSingleObject(This->mutex, INFINITE);
    hres = remove_item(This->filename, bstrKey, &old_value, &changed);
    ReleaseMutex(This->mutex);

    if(hres == S_OK && changed)
        hres = send_storage_event(This, bstrKey, old_value, NULL);
    return hres;
}

static HRESULT WINAPI HTMLStorage_clear(IHTMLStorage *iface)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    HRESULT hres = S_OK;

    if(!This->filename) {
        UINT num_keys = This->session_storage->num_keys;
        clear_session_storage(This->session_storage);
        return num_keys ? send_storage_event(This, NULL, NULL, NULL) : S_OK;
    }

    WaitForSingleObject(This->mutex, INFINITE);
    if(!DeleteFileW(This->filename)) {
        DWORD error = GetLastError();
        if(error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND)
            hres = HRESULT_FROM_WIN32(error);
    }
    ReleaseMutex(This->mutex);
    if(hres == S_OK)
        hres = send_storage_event(This, NULL, NULL, NULL);
    return hres;
}

static const IHTMLStorageVtbl HTMLStorageVtbl = {
    HTMLStorage_QueryInterface,
    HTMLStorage_AddRef,
    HTMLStorage_Release,
    HTMLStorage_GetTypeInfoCount,
    HTMLStorage_GetTypeInfo,
    HTMLStorage_GetIDsOfNames,
    HTMLStorage_Invoke,
    HTMLStorage_get_length,
    HTMLStorage_get_remainingSpace,
    HTMLStorage_key,
    HTMLStorage_getItem,
    HTMLStorage_setItem,
    HTMLStorage_removeItem,
    HTMLStorage_clear
};

static inline HTMLStorage *impl_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLStorage, dispex);
}

static void *HTMLStorage_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLStorage *This = impl_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLStorage, riid))
        return &This->IHTMLStorage_iface;

    return NULL;
}

static void HTMLStorage_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLStorage *This = impl_from_DispatchEx(dispex);

    if(This->window)
        note_cc_edge((nsISupports*)&This->window->base.IHTMLWindow2_iface, "window", cb);
}

static void HTMLStorage_unlink(DispatchEx *dispex)
{
    HTMLStorage *This = impl_from_DispatchEx(dispex);

    if(This->window) {
        HTMLInnerWindow *window = This->window;
        This->window = NULL;
        IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);
    }
}

static void HTMLStorage_destructor(DispatchEx *dispex)
{
    HTMLStorage *This = impl_from_DispatchEx(dispex);
    release_session_map_entry(This->session_storage);
    free(This->filename);
    CloseHandle(This->mutex);
    release_props(This);
    free(This);
}

static HRESULT check_item(HTMLStorage *This, const WCHAR *key)
{
    struct session_entry *session_entry;
    IXMLDOMNode *root, *node;
    IXMLDOMDocument *doc;
    HRESULT hres;
    BSTR query;

    if(!This->filename) {
        hres = get_session_entry(This->session_storage, key, FALSE, &session_entry);
        if(SUCCEEDED(hres))
            hres = (session_entry && session_entry->value) ? S_OK : S_FALSE;
        return hres;
    }

    WaitForSingleObject(This->mutex, INFINITE);

    hres = open_document(This->filename, &doc);
    if(hres == S_OK) {
        hres = get_root_node(doc, &root);
        IXMLDOMDocument_Release(doc);
        if(hres == S_OK) {
            if(!(query = build_query(key)))
                hres = E_OUTOFMEMORY;
            else {
                hres = IXMLDOMNode_selectSingleNode(root, query, &node);
                SysFreeString(query);
                if(hres == S_OK)
                    IXMLDOMNode_Release(node);
            }
            IXMLDOMNode_Release(root);
        }
    }

    ReleaseMutex(This->mutex);

    return hres;
}

static HRESULT get_prop(HTMLStorage *This, const WCHAR *name, DISPID *dispid)
{
    UINT name_len = wcslen(name);
    BSTR p, *prop, *end;

    for(prop = This->props, end = prop + This->num_props; prop != end; prop++) {
        if(SysStringLen(*prop) == name_len && !memcmp(*prop, name, name_len * sizeof(WCHAR))) {
            *dispid = MSHTML_DISPID_CUSTOM_MIN + (prop - This->props);
            return S_OK;
        }
    }

    if(is_power_of_2(This->num_props)) {
        BSTR *new_props = realloc(This->props, max(This->num_props * 2, 1) * sizeof(*This->props));
        if(!new_props)
            return E_OUTOFMEMORY;
        This->props = new_props;
    }

    if(!(p = SysAllocStringLen(name, name_len)))
        return E_OUTOFMEMORY;

    This->props[This->num_props] = p;
    *dispid = MSHTML_DISPID_CUSTOM_MIN + This->num_props++;
    return S_OK;
}

static HRESULT HTMLStorage_get_dispid(DispatchEx *dispex, const WCHAR *name, DWORD flags, DISPID *dispid)
{
    HTMLStorage *This = impl_from_DispatchEx(dispex);
    HRESULT hres;

    if(flags & fdexNameCaseInsensitive)
        FIXME("case insensitive not supported\n");

    if(!(flags & fdexNameEnsure)) {
        hres = check_item(This, name);
        if(hres != S_OK)
            return FAILED(hres) ? hres : DISP_E_UNKNOWNNAME;
    }

    return get_prop(This, name, dispid);
}

static HRESULT HTMLStorage_invoke(DispatchEx *dispex, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLStorage *This = impl_from_DispatchEx(dispex);
    DWORD idx = id - MSHTML_DISPID_CUSTOM_MIN;
    HRESULT hres;
    BSTR bstr;

    if(idx >= This->num_props)
        return DISP_E_MEMBERNOTFOUND;

    switch(flags) {
    case DISPATCH_PROPERTYGET:
        hres = HTMLStorage_getItem(&This->IHTMLStorage_iface, This->props[idx], res);
        if(FAILED(hres))
            return hres;
        if(V_VT(res) == VT_NULL)
            return DISP_E_MEMBERNOTFOUND;
        break;
    case DISPATCH_PROPERTYPUTREF:
    case DISPATCH_PROPERTYPUT:
        if(params->cArgs != 1 || (params->cNamedArgs && params->rgdispidNamedArgs[0] != DISPID_PROPERTYPUT)) {
            FIXME("unimplemented args %u %u\n", params->cArgs, params->cNamedArgs);
            return E_NOTIMPL;
        }

        bstr = V_BSTR(params->rgvarg);
        if(V_VT(params->rgvarg) != VT_BSTR) {
            VARIANT var;
            hres = change_type(&var, params->rgvarg, VT_BSTR, caller);
            if(FAILED(hres))
                return hres;
            bstr = V_BSTR(&var);
        }

        hres = HTMLStorage_setItem(&This->IHTMLStorage_iface, This->props[idx], bstr);

        if(V_VT(params->rgvarg) != VT_BSTR)
            SysFreeString(bstr);
        return hres;

    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT HTMLStorage_delete(DispatchEx *dispex, DISPID id)
{
    HTMLStorage *This = impl_from_DispatchEx(dispex);
    DWORD idx = id - MSHTML_DISPID_CUSTOM_MIN;

    if(idx >= This->num_props)
        return DISP_E_MEMBERNOTFOUND;

    if(dispex_compat_mode(dispex) < COMPAT_MODE_IE8)
        return MSHTML_E_INVALID_ACTION;

    return HTMLStorage_removeItem(&This->IHTMLStorage_iface, This->props[idx]);
}

static HRESULT HTMLStorage_next_dispid(DispatchEx *dispex, DISPID id, DISPID *pid)
{
    DWORD idx = (id == DISPID_STARTENUM) ? 0 : id - MSHTML_DISPID_CUSTOM_MIN + 1;
    HTMLStorage *This = impl_from_DispatchEx(dispex);
    HRESULT hres;
    DISPID tmp;

    if(idx > MSHTML_CUSTOM_DISPID_CNT)
        return S_FALSE;

    while(idx < This->num_props) {
        hres = check_item(This, This->props[idx]);
        if(hres == S_OK) {
            *pid = idx + MSHTML_DISPID_CUSTOM_MIN;
            return S_OK;
        }
        if(FAILED(hres))
            return hres;
        idx++;
    }

    /* Populate possibly missing DISPIDs */
    if(!This->filename) {
        struct session_entry *session_entry;

        LIST_FOR_EACH_ENTRY(session_entry, &This->session_storage->data_list, struct session_entry, list_entry) {
            hres = get_prop(This, session_entry->key, &tmp);
            if(FAILED(hres))
                return hres;
        }
    }else {
        IXMLDOMNodeList *node_list;
        IXMLDOMElement *elem;
        IXMLDOMNode *node;
        LONG index = 0;
        HRESULT hres;
        VARIANT key;

        hres = get_node_list(This->filename, &node_list);
        if(FAILED(hres))
            return hres;

        for(;;) {
            hres = IXMLDOMNodeList_get_item(node_list, index++, &node);
            if(hres != S_OK) {
                IXMLDOMNodeList_Release(node_list);
                if(FAILED(hres))
                    return hres;
                break;
            }

            hres = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMElement, (void**)&elem);
            IXMLDOMNode_Release(node);
            if(hres != S_OK)
                continue;

            hres = IXMLDOMElement_getAttribute(elem, (BSTR)L"name", &key);
            IXMLDOMElement_Release(elem);
            if(hres != S_OK) {
                if(SUCCEEDED(hres))
                    continue;
                IXMLDOMNodeList_Release(node_list);
                return hres;
            }

            if(V_VT(&key) != VT_BSTR) {
                FIXME("non-string key %s\n", debugstr_variant(&key));
                VariantClear(&key);
                continue;
            }

            hres = get_prop(This, V_BSTR(&key), &tmp);
            SysFreeString(V_BSTR(&key));
            if(FAILED(hres)) {
                IXMLDOMNodeList_Release(node_list);
                return hres;
            }
        }
    }

    if(idx >= This->num_props)
        return S_FALSE;

    *pid = idx + MSHTML_DISPID_CUSTOM_MIN;
    return S_OK;
}

static HRESULT HTMLStorage_get_prop_desc(DispatchEx *dispex, DISPID id, struct property_info *desc)
{
    HTMLStorage *This = impl_from_DispatchEx(dispex);

    desc->name = This->props[id - MSHTML_DISPID_CUSTOM_MIN];
    desc->id = id;
    desc->flags = PROPF_WRITABLE | PROPF_CONFIGURABLE | PROPF_ENUMERABLE;
    desc->iid = 0;
    return S_OK;
}

static const dispex_static_data_vtbl_t Storage_dispex_vtbl = {
    .query_interface  = HTMLStorage_query_interface,
    .destructor       = HTMLStorage_destructor,
    .traverse         = HTMLStorage_traverse,
    .unlink           = HTMLStorage_unlink,
    .get_dispid       = HTMLStorage_get_dispid,
    .invoke           = HTMLStorage_invoke,
    .delete           = HTMLStorage_delete,
    .next_dispid      = HTMLStorage_next_dispid,
    .get_prop_desc    = HTMLStorage_get_prop_desc,
};

static const tid_t HTMLStorage_iface_tids[] = {
    IHTMLStorage_tid,
    0
};
dispex_static_data_t Storage_dispex = {
    .id         = PROT_Storage,
    .vtbl       = &Storage_dispex_vtbl,
    .disp_tid   = IHTMLStorage_tid,
    .iface_tids = HTMLStorage_iface_tids,
};

static HRESULT build_session_origin(IUri *uri, BSTR hostname, BSTR *ret)
{
    UINT host_len, scheme_len;
    BSTR scheme, origin;
    HRESULT hres;

    hres = IUri_GetSchemeName(uri, &scheme);
    if(FAILED(hres))
        return hres;
    if(hres != S_OK) {
        SysFreeString(scheme);
        scheme = NULL;
    }

    /* Since it's only used for lookup, we can apply transformations to
       keep the lookup itself simple and fast. First, we convert `https`
       to `http` because they are equal for lookup. Next, we place the
       scheme after the hostname, separated by NUL, to compare the host
       first, since it tends to differ more often. Lastly, we lowercase
       the whole thing since lookup must be case-insensitive. */
    scheme_len = SysStringLen(scheme);
    host_len = SysStringLen(hostname);

    if(scheme_len == 5 && !wcsicmp(scheme, L"https"))
        scheme_len--;

    origin = SysAllocStringLen(NULL, host_len + 1 + scheme_len);
    if(origin) {
        WCHAR *p = origin;
        memcpy(p, hostname, host_len * sizeof(WCHAR));
        p += host_len;
        *p = ' ';    /* for wcslwr */
        memcpy(p + 1, scheme, scheme_len * sizeof(WCHAR));
        p[1 + scheme_len] = '\0';
        _wcslwr(origin);
        *p = '\0';
    }
    SysFreeString(scheme);

    if(!origin)
        return E_OUTOFMEMORY;

    *ret = origin;
    return S_OK;
}

static WCHAR *build_filename(BSTR hostname)
{
    static const WCHAR store[] = L"\\Microsoft\\Internet Explorer\\DOMStore\\";
    WCHAR path[MAX_PATH], *ret;
    int len;

    if(!SHGetSpecialFolderPathW(NULL, path, CSIDL_LOCAL_APPDATA, TRUE)) {
        ERR("Can't get folder path %lu\n", GetLastError());
        return NULL;
    }

    len = wcslen(path);
    if(len + ARRAY_SIZE(store) > ARRAY_SIZE(path)) {
        ERR("Path too long\n");
        return NULL;
    }
    memcpy(path + len, store, sizeof(store));

    len += ARRAY_SIZE(store);
    ret = malloc((len + wcslen(hostname) + ARRAY_SIZE(L".xml")) * sizeof(WCHAR));
    if(!ret) {
        return NULL;
    }

    wcscpy(ret, path);
    wcscat(ret, hostname);
    wcscat(ret, L".xml");

    return ret;
}

static WCHAR *build_mutexname(const WCHAR *filename)
{
    WCHAR *ret, *ptr;
    ret = wcsdup(filename);
    if(!ret)
        return NULL;
    for(ptr = ret; *ptr; ptr++)
        if(*ptr == '\\') *ptr = '_';
    return ret;
}

HRESULT create_html_storage(HTMLInnerWindow *window, BOOL local, IHTMLStorage **p)
{
    IUri *uri = window->base.outer_window->uri;
    BSTR origin, hostname = NULL;
    HTMLStorage *storage;
    HRESULT hres;

    if(!uri)
        return S_FALSE;

    hres = IUri_GetHost(uri, &hostname);
    if(hres != S_OK) {
        SysFreeString(hostname);
        return hres;
    }

    storage = calloc(1, sizeof(*storage));
    if(!storage) {
        SysFreeString(hostname);
        return E_OUTOFMEMORY;
    }

    if(local) {
        WCHAR *mutexname;
        storage->filename = build_filename(hostname);
        SysFreeString(hostname);
        if(!storage->filename) {
            free(storage);
            return E_OUTOFMEMORY;
        }
        mutexname = build_mutexname(storage->filename);
        if(!mutexname) {
            free(storage->filename);
            free(storage);
            return E_OUTOFMEMORY;
        }
        storage->mutex = CreateMutexW(NULL, FALSE, mutexname);
        free(mutexname);
        if(!storage->mutex) {
            free(storage->filename);
            free(storage);
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }else {
        hres = build_session_origin(uri, hostname, &origin);
        SysFreeString(hostname);
        if(hres != S_OK) {
            free(storage);
            return hres;
        }
        storage->session_storage = grab_session_map_entry(origin);
        SysFreeString(origin);
        if(!storage->session_storage) {
            free(storage);
            return E_OUTOFMEMORY;
        }
    }

    storage->IHTMLStorage_iface.lpVtbl = &HTMLStorageVtbl;
    storage->window = window;
    IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);

    init_dispatch(&storage->dispex, &Storage_dispex, window,
                  dispex_compat_mode(&window->event_target.dispex));

    *p = &storage->IHTMLStorage_iface;
    return S_OK;
}
