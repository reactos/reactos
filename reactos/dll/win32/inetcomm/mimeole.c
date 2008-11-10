/*
 * MIME OLE Interfaces
 *
 * Copyright 2006 Robert Shearman for CodeWeavers
 * Copyright 2007 Huw Davies for CodeWeavers
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
#define NONAMELESSUNION

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"
#include "ole2.h"
#include "mimeole.h"

#include "wine/list.h"
#include "wine/debug.h"

#include "inetcomm_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(inetcomm);

typedef struct
{
    LPCSTR     name;
    DWORD      id;
    DWORD      flags; /* MIMEPROPFLAGS */
    VARTYPE    default_vt;
} property_t;

typedef struct
{
    struct list entry;
    property_t prop;
} property_list_entry_t;

static const property_t default_props[] =
{
    {"References",                   PID_HDR_REFS,       0,                               VT_LPSTR},
    {"Subject",                      PID_HDR_SUBJECT,    0,                               VT_LPSTR},
    {"From",                         PID_HDR_FROM,       MPF_ADDRESS,                     VT_LPSTR},
    {"Message-ID",                   PID_HDR_MESSAGEID,  0,                               VT_LPSTR},
    {"Return-Path",                  PID_HDR_RETURNPATH, MPF_ADDRESS,                     VT_LPSTR},
    {"Date",                         PID_HDR_DATE,       0,                               VT_LPSTR},
    {"Received",                     PID_HDR_RECEIVED,   0,                               VT_LPSTR},
    {"Reply-To",                     PID_HDR_REPLYTO,    MPF_ADDRESS,                     VT_LPSTR},
    {"X-Mailer",                     PID_HDR_XMAILER,    0,                               VT_LPSTR},
    {"Bcc",                          PID_HDR_BCC,        MPF_ADDRESS,                     VT_LPSTR},
    {"MIME-Version",                 PID_HDR_MIMEVER,    MPF_MIME,                        VT_LPSTR},
    {"Content-Type",                 PID_HDR_CNTTYPE,    MPF_MIME | MPF_HASPARAMS,        VT_LPSTR},
    {"Content-Transfer-Encoding",    PID_HDR_CNTXFER,    MPF_MIME,                        VT_LPSTR},
    {"Content-ID",                   PID_HDR_CNTID,      MPF_MIME,                        VT_LPSTR},
    {"Content-Disposition",          PID_HDR_CNTDISP,    MPF_MIME | MPF_HASPARAMS,        VT_LPSTR},
    {"To",                           PID_HDR_TO,         MPF_ADDRESS,                     VT_LPSTR},
    {"Cc",                           PID_HDR_CC,         MPF_ADDRESS,                     VT_LPSTR},
    {"Sender",                       PID_HDR_SENDER,     MPF_ADDRESS,                     VT_LPSTR},
    {"In-Reply-To",                  PID_HDR_INREPLYTO,  0,                               VT_LPSTR},
    {NULL,                           0,                  0,                               0}
};

typedef struct
{
    struct list entry;
    char *name;
    char *value;
} param_t;

typedef struct
{
    struct list entry;
    const property_t *prop;
    PROPVARIANT value;
    struct list params;
} header_t;

typedef struct MimeBody
{
    const IMimeBodyVtbl *lpVtbl;
    LONG refs;

    HBODY handle;

    struct list headers;
    struct list new_props; /* FIXME: This should be in a PropertySchema */
    DWORD next_prop_id;
    char *content_pri_type;
    char *content_sub_type;
    ENCODINGTYPE encoding;
    void *data;
    IID data_iid;
    BODYOFFSETS body_offsets;
} MimeBody;

static inline MimeBody *impl_from_IMimeBody( IMimeBody *iface )
{
    return (MimeBody *)((char*)iface - FIELD_OFFSET(MimeBody, lpVtbl));
}

static LPSTR strdupA(LPCSTR str)
{
    char *ret;
    int len = strlen(str);
    ret = HeapAlloc(GetProcessHeap(), 0, len + 1);
    memcpy(ret, str, len + 1);
    return ret;
}

#define PARSER_BUF_SIZE 1024

/*****************************************************
 *        copy_headers_to_buf [internal]
 *
 * Copies the headers into a '\0' terminated memory block and leave
 * the stream's current position set to after the blank line.
 */
static HRESULT copy_headers_to_buf(IStream *stm, char **ptr)
{
    char *buf = NULL;
    DWORD size = PARSER_BUF_SIZE, offset = 0, last_end = 0;
    HRESULT hr;
    int done = 0;

    *ptr = NULL;

    do
    {
        char *end;
        DWORD read;

        if(!buf)
            buf = HeapAlloc(GetProcessHeap(), 0, size + 1);
        else
        {
            size *= 2;
            buf = HeapReAlloc(GetProcessHeap(), 0, buf, size + 1);
        }
        if(!buf)
        {
            hr = E_OUTOFMEMORY;
            goto fail;
        }

        hr = IStream_Read(stm, buf + offset, size - offset, &read);
        if(FAILED(hr)) goto fail;

        offset += read;
        buf[offset] = '\0';

        if(read == 0) done = 1;

        while(!done && (end = strstr(buf + last_end, "\r\n")))
        {
            DWORD new_end = end - buf + 2;
            if(new_end - last_end == 2)
            {
                LARGE_INTEGER off;
                off.QuadPart = new_end;
                IStream_Seek(stm, off, STREAM_SEEK_SET, NULL);
                buf[new_end] = '\0';
                done = 1;
            }
            else
                last_end = new_end;
        }
    } while(!done);

    *ptr = buf;
    return S_OK;

fail:
    HeapFree(GetProcessHeap(), 0, buf);
    return hr;
}

static header_t *read_prop(MimeBody *body, char **ptr)
{
    char *colon = strchr(*ptr, ':');
    const property_t *prop;
    header_t *ret;

    if(!colon) return NULL;

    *colon = '\0';

    for(prop = default_props; prop->name; prop++)
    {
        if(!strcasecmp(*ptr, prop->name))
        {
            TRACE("%s: found match with default property id %d\n", *ptr, prop->id);
            break;
        }
    }

    if(!prop->name)
    {
        property_list_entry_t *prop_entry;
        LIST_FOR_EACH_ENTRY(prop_entry, &body->new_props, property_list_entry_t, entry)
        {
            if(!strcasecmp(*ptr, prop_entry->prop.name))
            {
                TRACE("%s: found match with already added new property id %d\n", *ptr, prop_entry->prop.id);
                prop = &prop_entry->prop;
                break;
            }
        }
        if(!prop->name)
        {
            prop_entry = HeapAlloc(GetProcessHeap(), 0, sizeof(*prop_entry));
            prop_entry->prop.name = strdupA(*ptr);
            prop_entry->prop.id = body->next_prop_id++;
            prop_entry->prop.flags = 0;
            prop_entry->prop.default_vt = VT_LPSTR;
            list_add_tail(&body->new_props, &prop_entry->entry);
            prop = &prop_entry->prop;
            TRACE("%s: allocating new prop id %d\n", *ptr, prop_entry->prop.id);
        }
    }

    ret = HeapAlloc(GetProcessHeap(), 0, sizeof(*ret));
    ret->prop = prop;
    PropVariantInit(&ret->value);
    list_init(&ret->params);
    *ptr = colon + 1;

    return ret;
}

static void unfold_header(char *header, int len)
{
    char *start = header, *cp = header;

    do {
        while(*cp == ' ' || *cp == '\t')
        {
            cp++;
            len--;
        }
        if(cp != start)
            memmove(start, cp, len + 1);

        cp = strstr(start, "\r\n");
        len -= (cp - start);
        start = cp;
        *start = ' ';
        start++;
        len--;
        cp += 2;
    } while(*cp == ' ' || *cp == '\t');

    *(start - 1) = '\0';
}

static char *unquote_string(const char *str)
{
    int quoted = 0;
    char *ret, *cp;

    while(*str == ' ' || *str == '\t') str++;

    if(*str == '"')
    {
        quoted = 1;
        str++;
    }
    ret = strdupA(str);
    for(cp = ret; *cp; cp++)
    {
        if(*cp == '\\')
            memmove(cp, cp + 1, strlen(cp + 1) + 1);
        else if(*cp == '"')
        {
            if(!quoted)
            {
                WARN("quote in unquoted string\n");
            }
            else
            {
                *cp = '\0';
                break;
            }
        }
    }
    return ret;
}

static void add_param(header_t *header, const char *p)
{
    const char *key = p, *value, *cp = p;
    param_t *param;
    char *name;

    TRACE("got param %s\n", p);

    while (*key == ' ' || *key == '\t' ) key++;

    cp = strchr(key, '=');
    if(!cp)
    {
        WARN("malformed parameter - skipping\n");
        return;
    }

    name = HeapAlloc(GetProcessHeap(), 0, cp - key + 1);
    memcpy(name, key, cp - key);
    name[cp - key] = '\0';

    value = cp + 1;

    param = HeapAlloc(GetProcessHeap(), 0, sizeof(*param));
    param->name = name;
    param->value = unquote_string(value);
    list_add_tail(&header->params, &param->entry);
}

static void split_params(header_t *header, char *value)
{
    char *cp = value, *start = value;
    int in_quote = 0;
    int done_value = 0;

    while(*cp)
    {
        if(!in_quote && *cp == ';')
        {
            *cp = '\0';
            if(done_value) add_param(header, start);
            done_value = 1;
            start = cp + 1;
        }
        else if(*cp == '"')
            in_quote = !in_quote;
        cp++;
    }
    if(done_value) add_param(header, start);
}

static void read_value(header_t *header, char **cur)
{
    char *end = *cur, *value;
    DWORD len;

    do {
        end = strstr(end, "\r\n");
        end += 2;
    } while(*end == ' ' || *end == '\t');

    len = end - *cur;
    value = HeapAlloc(GetProcessHeap(), 0, len + 1);
    memcpy(value, *cur, len);
    value[len] = '\0';

    unfold_header(value, len);
    TRACE("value %s\n", debugstr_a(value));

    if(header->prop->flags & MPF_HASPARAMS)
    {
        split_params(header, value);
        TRACE("value w/o params %s\n", debugstr_a(value));
    }

    header->value.vt = VT_LPSTR;
    header->value.u.pszVal = value;

    *cur = end;
}

static void init_content_type(MimeBody *body, header_t *header)
{
    char *slash;
    DWORD len;

    if(header->prop->id != PID_HDR_CNTTYPE)
    {
        ERR("called with header %s\n", header->prop->name);
        return;
    }

    slash = strchr(header->value.u.pszVal, '/');
    if(!slash)
    {
        WARN("malformed context type value\n");
        return;
    }
    len = slash - header->value.u.pszVal;
    body->content_pri_type = HeapAlloc(GetProcessHeap(), 0, len + 1);
    memcpy(body->content_pri_type, header->value.u.pszVal, len);
    body->content_pri_type[len] = '\0';
    body->content_sub_type = strdupA(slash + 1);
}

static HRESULT parse_headers(MimeBody *body, IStream *stm)
{
    char *header_buf, *cur_header_ptr;
    HRESULT hr;
    header_t *header;

    hr = copy_headers_to_buf(stm, &header_buf);
    if(FAILED(hr)) return hr;

    cur_header_ptr = header_buf;
    while((header = read_prop(body, &cur_header_ptr)))
    {
        read_value(header, &cur_header_ptr);
        list_add_tail(&body->headers, &header->entry);

        if(header->prop->id == PID_HDR_CNTTYPE)
            init_content_type(body, header);
    }

    HeapFree(GetProcessHeap(), 0, header_buf);
    return hr;
}

static void empty_param_list(struct list *list)
{
    param_t *param, *cursor2;

    LIST_FOR_EACH_ENTRY_SAFE(param, cursor2, list, param_t, entry)
    {
        list_remove(&param->entry);
        HeapFree(GetProcessHeap(), 0, param->name);
        HeapFree(GetProcessHeap(), 0, param->value);
        HeapFree(GetProcessHeap(), 0, param);
    }
}

static void empty_header_list(struct list *list)
{
    header_t *header, *cursor2;

    LIST_FOR_EACH_ENTRY_SAFE(header, cursor2, list, header_t, entry)
    {
        list_remove(&header->entry);
        PropVariantClear(&header->value);
        empty_param_list(&header->params);
        HeapFree(GetProcessHeap(), 0, header);
    }
}

static void empty_new_prop_list(struct list *list)
{
    property_list_entry_t *prop, *cursor2;

    LIST_FOR_EACH_ENTRY_SAFE(prop, cursor2, list, property_list_entry_t, entry)
    {
        list_remove(&prop->entry);
        HeapFree(GetProcessHeap(), 0, (char *)prop->prop.name);
        HeapFree(GetProcessHeap(), 0, prop);
    }
}

static void release_data(REFIID riid, void *data)
{
    if(!data) return;

    if(IsEqualIID(riid, &IID_IStream))
        IStream_Release((IStream *)data);
    else
        FIXME("Unhandled data format %s\n", debugstr_guid(riid));
}

static HRESULT find_prop(MimeBody *body, const char *name, header_t **prop)
{
    header_t *header;

    *prop = NULL;

    LIST_FOR_EACH_ENTRY(header, &body->headers, header_t, entry)
    {
        if(!strcasecmp(name, header->prop->name))
        {
            *prop = header;
            return S_OK;
        }
    }

    return MIME_E_NOT_FOUND;
}

static HRESULT WINAPI MimeBody_QueryInterface(IMimeBody* iface,
                                     REFIID riid,
                                     void** ppvObject)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppvObject);

    *ppvObject = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IPersist) ||
        IsEqualIID(riid, &IID_IPersistStreamInit) ||
        IsEqualIID(riid, &IID_IMimePropertySet) ||
        IsEqualIID(riid, &IID_IMimeBody))
    {
        *ppvObject = iface;
    }

    if(*ppvObject)
    {
        IUnknown_AddRef((IUnknown*)*ppvObject);
        return S_OK;
    }

    FIXME("no interface for %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI MimeBody_AddRef(IMimeBody* iface)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    TRACE("(%p)->()\n", iface);
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI MimeBody_Release(IMimeBody* iface)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    ULONG refs;

    TRACE("(%p)->()\n", iface);

    refs = InterlockedDecrement(&This->refs);
    if (!refs)
    {
        empty_header_list(&This->headers);
        empty_new_prop_list(&This->new_props);

        HeapFree(GetProcessHeap(), 0, This->content_pri_type);
        HeapFree(GetProcessHeap(), 0, This->content_sub_type);

        release_data(&This->data_iid, This->data);

        HeapFree(GetProcessHeap(), 0, This);
    }

    return refs;
}

static HRESULT WINAPI MimeBody_GetClassID(
                                 IMimeBody* iface,
                                 CLSID* pClassID)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI MimeBody_IsDirty(
                              IMimeBody* iface)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_Load(
                           IMimeBody* iface,
                           LPSTREAM pStm)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    TRACE("(%p)->(%p)\n", iface, pStm);
    return parse_headers(This, pStm);
}

static HRESULT WINAPI MimeBody_Save(
                           IMimeBody* iface,
                           LPSTREAM pStm,
                           BOOL fClearDirty)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_GetSizeMax(
                                 IMimeBody* iface,
                                 ULARGE_INTEGER* pcbSize)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_InitNew(
                              IMimeBody* iface)
{
    TRACE("%p->()\n", iface);
    return S_OK;
}

static HRESULT WINAPI MimeBody_GetPropInfo(
                                  IMimeBody* iface,
                                  LPCSTR pszName,
                                  LPMIMEPROPINFO pInfo)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_SetPropInfo(
                                  IMimeBody* iface,
                                  LPCSTR pszName,
                                  LPCMIMEPROPINFO pInfo)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_GetProp(
                              IMimeBody* iface,
                              LPCSTR pszName,
                              DWORD dwFlags,
                              LPPROPVARIANT pValue)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    TRACE("(%p)->(%s, %d, %p)\n", This, pszName, dwFlags, pValue);

    if(!strcasecmp(pszName, "att:pri-content-type"))
    {
        PropVariantClear(pValue);
        pValue->vt = VT_LPSTR;
        pValue->u.pszVal = strdupA(This->content_pri_type);
        return S_OK;
    }

    FIXME("stub!\n");
    return E_FAIL;
}

static HRESULT WINAPI MimeBody_SetProp(
                              IMimeBody* iface,
                              LPCSTR pszName,
                              DWORD dwFlags,
                              LPCPROPVARIANT pValue)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_AppendProp(
                                 IMimeBody* iface,
                                 LPCSTR pszName,
                                 DWORD dwFlags,
                                 LPPROPVARIANT pValue)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_DeleteProp(
                                 IMimeBody* iface,
                                 LPCSTR pszName)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_CopyProps(
                                IMimeBody* iface,
                                ULONG cNames,
                                LPCSTR* prgszName,
                                IMimePropertySet* pPropertySet)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_MoveProps(
                                IMimeBody* iface,
                                ULONG cNames,
                                LPCSTR* prgszName,
                                IMimePropertySet* pPropertySet)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_DeleteExcept(
                                   IMimeBody* iface,
                                   ULONG cNames,
                                   LPCSTR* prgszName)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_QueryProp(
                                IMimeBody* iface,
                                LPCSTR pszName,
                                LPCSTR pszCriteria,
                                boolean fSubString,
                                boolean fCaseSensitive)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_GetCharset(
                                 IMimeBody* iface,
                                 LPHCHARSET phCharset)
{
    FIXME("stub\n");
    *phCharset = NULL;
    return S_OK;
}

static HRESULT WINAPI MimeBody_SetCharset(
                                 IMimeBody* iface,
                                 HCHARSET hCharset,
                                 CSETAPPLYTYPE applytype)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_GetParameters(
                                    IMimeBody* iface,
                                    LPCSTR pszName,
                                    ULONG* pcParams,
                                    LPMIMEPARAMINFO* pprgParam)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    HRESULT hr;
    header_t *header;

    TRACE("(%p)->(%s, %p, %p)\n", iface, debugstr_a(pszName), pcParams, pprgParam);

    *pprgParam = NULL;
    *pcParams = 0;

    hr = find_prop(This, pszName, &header);
    if(hr != S_OK) return hr;

    *pcParams = list_count(&header->params);
    if(*pcParams)
    {
        IMimeAllocator *alloc;
        param_t *param;
        MIMEPARAMINFO *info;

        MimeOleGetAllocator(&alloc);

        *pprgParam = info = IMimeAllocator_Alloc(alloc, *pcParams * sizeof(**pprgParam));
        LIST_FOR_EACH_ENTRY(param, &header->params, param_t, entry)
        {
            int len;

            len = strlen(param->name) + 1;
            info->pszName = IMimeAllocator_Alloc(alloc, len);
            memcpy(info->pszName, param->name, len);
            len = strlen(param->value) + 1;
            info->pszData = IMimeAllocator_Alloc(alloc, len);
            memcpy(info->pszData, param->value, len);
            info++;
        }
        IMimeAllocator_Release(alloc);
    }
    return S_OK;
}

static HRESULT WINAPI MimeBody_IsContentType(
                                    IMimeBody* iface,
                                    LPCSTR pszPriType,
                                    LPCSTR pszSubType)
{
    MimeBody *This = impl_from_IMimeBody(iface);

    TRACE("(%p)->(%s, %s)\n", This, debugstr_a(pszPriType), debugstr_a(pszSubType));
    if(pszPriType)
    {
        const char *pri = This->content_pri_type;
        if(!pri) pri = "text";
        if(strcasecmp(pri, pszPriType)) return S_FALSE;
    }

    if(pszSubType)
    {
        const char *sub = This->content_sub_type;
        if(!sub) sub = "plain";
        if(strcasecmp(sub, pszSubType)) return S_FALSE;
    }

    return S_OK;
}

static HRESULT WINAPI MimeBody_BindToObject(
                                   IMimeBody* iface,
                                   REFIID riid,
                                   void** ppvObject)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_Clone(
                            IMimeBody* iface,
                            IMimePropertySet** ppPropertySet)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_SetOption(
                                IMimeBody* iface,
                                const TYPEDID oid,
                                LPCPROPVARIANT pValue)
{
    HRESULT hr = E_NOTIMPL;
    TRACE("(%p)->(%08x, %p)\n", iface, oid, pValue);

    if(pValue->vt != TYPEDID_TYPE(oid))
    {
        WARN("Called with vartype %04x and oid %08x\n", pValue->vt, oid);
        return E_INVALIDARG;
    }

    switch(oid)
    {
    case OID_SECURITY_HWND_OWNER:
        FIXME("OID_SECURITY_HWND_OWNER (value %08x): ignoring\n", pValue->u.ulVal);
        hr = S_OK;
        break;
    default:
        FIXME("Unhandled oid %08x\n", oid);
    }

    return hr;
}

static HRESULT WINAPI MimeBody_GetOption(
                                IMimeBody* iface,
                                const TYPEDID oid,
                                LPPROPVARIANT pValue)
{
    FIXME("(%p)->(%08x, %p): stub\n", iface, oid, pValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_EnumProps(
                                IMimeBody* iface,
                                DWORD dwFlags,
                                IMimeEnumProperties** ppEnum)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_IsType(
                             IMimeBody* iface,
                             IMSGBODYTYPE bodytype)
{
    MimeBody *This = impl_from_IMimeBody(iface);

    TRACE("(%p)->(%d)\n", iface, bodytype);
    switch(bodytype)
    {
    case IBT_EMPTY:
        return This->data ? S_FALSE : S_OK;
    default:
        FIXME("Unimplemented bodytype %d - returning S_OK\n", bodytype);
    }
    return S_OK;
}

static HRESULT WINAPI MimeBody_SetDisplayName(
                                     IMimeBody* iface,
                                     LPCSTR pszDisplay)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_GetDisplayName(
                                     IMimeBody* iface,
                                     LPSTR* ppszDisplay)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_GetOffsets(
                                 IMimeBody* iface,
                                 LPBODYOFFSETS pOffsets)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    TRACE("(%p)->(%p)\n", This, pOffsets);

    *pOffsets = This->body_offsets;

    if(This->body_offsets.cbBodyEnd == 0) return MIME_E_NO_DATA;
    return S_OK;
}

static HRESULT WINAPI MimeBody_GetCurrentEncoding(
                                         IMimeBody* iface,
                                         ENCODINGTYPE* pietEncoding)
{
    MimeBody *This = impl_from_IMimeBody(iface);

    TRACE("(%p)->(%p)\n", This, pietEncoding);

    *pietEncoding = This->encoding;
    return S_OK;
}

static HRESULT WINAPI MimeBody_SetCurrentEncoding(
                                         IMimeBody* iface,
                                         ENCODINGTYPE ietEncoding)
{
    MimeBody *This = impl_from_IMimeBody(iface);

    TRACE("(%p)->(%d)\n", This, ietEncoding);

    This->encoding = ietEncoding;
    return S_OK;
}

static HRESULT WINAPI MimeBody_GetEstimatedSize(
                                       IMimeBody* iface,
                                       ENCODINGTYPE ietEncoding,
                                       ULONG* pcbSize)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_GetDataHere(
                                  IMimeBody* iface,
                                  ENCODINGTYPE ietEncoding,
                                  IStream* pStream)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_GetData(
                              IMimeBody* iface,
                              ENCODINGTYPE ietEncoding,
                              IStream** ppStream)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    FIXME("(%p)->(%d, %p). Ignoring encoding type.\n", This, ietEncoding, ppStream);

    *ppStream = This->data;
    IStream_AddRef(*ppStream);
    return S_OK;
}

static HRESULT WINAPI MimeBody_SetData(
                              IMimeBody* iface,
                              ENCODINGTYPE ietEncoding,
                              LPCSTR pszPriType,
                              LPCSTR pszSubType,
                              REFIID riid,
                              LPVOID pvObject)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    TRACE("(%p)->(%d, %s, %s, %s %p)\n", This, ietEncoding, debugstr_a(pszPriType), debugstr_a(pszSubType),
          debugstr_guid(riid), pvObject);

    if(IsEqualIID(riid, &IID_IStream))
        IStream_AddRef((IStream *)pvObject);
    else
    {
        FIXME("Unhandled object type %s\n", debugstr_guid(riid));
        return E_INVALIDARG;
    }

    if(This->data)
        FIXME("release old data\n");

    This->data_iid = *riid;
    This->data = pvObject;

    IMimeBody_SetCurrentEncoding(iface, ietEncoding);

    /* FIXME: Update the content type.
       If pszPriType == NULL use 'application'
       If pszSubType == NULL use 'octet-stream' */

    return S_OK;
}

static HRESULT WINAPI MimeBody_EmptyData(
                                IMimeBody* iface)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_CopyTo(
                             IMimeBody* iface,
                             IMimeBody* pBody)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_GetTransmitInfo(
                                      IMimeBody* iface,
                                      LPTRANSMITINFO pTransmitInfo)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_SaveToFile(
                                 IMimeBody* iface,
                                 ENCODINGTYPE ietEncoding,
                                 LPCSTR pszFilePath)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_GetHandle(
                                IMimeBody* iface,
                                LPHBODY phBody)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    TRACE("(%p)->(%p)\n", iface, phBody);

    *phBody = This->handle;
    return This->handle ? S_OK : MIME_E_NO_DATA;
}

static IMimeBodyVtbl body_vtbl =
{
    MimeBody_QueryInterface,
    MimeBody_AddRef,
    MimeBody_Release,
    MimeBody_GetClassID,
    MimeBody_IsDirty,
    MimeBody_Load,
    MimeBody_Save,
    MimeBody_GetSizeMax,
    MimeBody_InitNew,
    MimeBody_GetPropInfo,
    MimeBody_SetPropInfo,
    MimeBody_GetProp,
    MimeBody_SetProp,
    MimeBody_AppendProp,
    MimeBody_DeleteProp,
    MimeBody_CopyProps,
    MimeBody_MoveProps,
    MimeBody_DeleteExcept,
    MimeBody_QueryProp,
    MimeBody_GetCharset,
    MimeBody_SetCharset,
    MimeBody_GetParameters,
    MimeBody_IsContentType,
    MimeBody_BindToObject,
    MimeBody_Clone,
    MimeBody_SetOption,
    MimeBody_GetOption,
    MimeBody_EnumProps,
    MimeBody_IsType,
    MimeBody_SetDisplayName,
    MimeBody_GetDisplayName,
    MimeBody_GetOffsets,
    MimeBody_GetCurrentEncoding,
    MimeBody_SetCurrentEncoding,
    MimeBody_GetEstimatedSize,
    MimeBody_GetDataHere,
    MimeBody_GetData,
    MimeBody_SetData,
    MimeBody_EmptyData,
    MimeBody_CopyTo,
    MimeBody_GetTransmitInfo,
    MimeBody_SaveToFile,
    MimeBody_GetHandle
};

static HRESULT MimeBody_set_offsets(MimeBody *body, const BODYOFFSETS *offsets)
{
    TRACE("setting offsets to %d, %d, %d, %d\n", offsets->cbBoundaryStart,
          offsets->cbHeaderStart, offsets->cbBodyStart, offsets->cbBodyEnd);

    body->body_offsets = *offsets;
    return S_OK;
}

#define FIRST_CUSTOM_PROP_ID 0x100

HRESULT MimeBody_create(IUnknown *outer, void **obj)
{
    MimeBody *This;
    BODYOFFSETS body_offsets;

    *obj = NULL;

    if(outer) return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &body_vtbl;
    This->refs = 1;
    This->handle = NULL;
    list_init(&This->headers);
    list_init(&This->new_props);
    This->next_prop_id = FIRST_CUSTOM_PROP_ID;
    This->content_pri_type = NULL;
    This->content_sub_type = NULL;
    This->encoding = IET_7BIT;
    This->data = NULL;
    This->data_iid = IID_NULL;

    body_offsets.cbBoundaryStart = body_offsets.cbHeaderStart = 0;
    body_offsets.cbBodyStart     = body_offsets.cbBodyEnd     = 0;
    MimeBody_set_offsets(This, &body_offsets);

    *obj = (IMimeBody *)&This->lpVtbl;
    return S_OK;
}

typedef struct
{
    IStreamVtbl *lpVtbl;
    LONG refs;

    IStream *base;
    ULARGE_INTEGER pos, start, length;
} sub_stream_t;

static inline sub_stream_t *impl_from_IStream( IStream *iface )
{
    return (sub_stream_t *)((char*)iface - FIELD_OFFSET(sub_stream_t, lpVtbl));
}

static HRESULT WINAPI sub_stream_QueryInterface(
        IStream* iface,
        REFIID riid,
        void **ppvObject)
{
    sub_stream_t *This = impl_from_IStream(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppvObject);
    *ppvObject = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
       IsEqualIID(riid, &IID_ISequentialStream) ||
       IsEqualIID(riid, &IID_IStream))
    {
        IStream_AddRef(iface);
        *ppvObject = iface;
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI sub_stream_AddRef(
         IStream* iface)
{
    sub_stream_t *This = impl_from_IStream(iface);

    TRACE("(%p)\n", This);
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI  sub_stream_Release(
        IStream* iface)
{
    sub_stream_t *This = impl_from_IStream(iface);
    LONG refs;

    TRACE("(%p)\n", This);
    refs = InterlockedDecrement(&This->refs);
    if(!refs)
    {
        IStream_Release(This->base);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return refs;
}

static HRESULT WINAPI sub_stream_Read(
        IStream* iface,
        void *pv,
        ULONG cb,
        ULONG *pcbRead)
{
    sub_stream_t *This = impl_from_IStream(iface);
    HRESULT hr;
    ULARGE_INTEGER base_pos;
    LARGE_INTEGER tmp_pos;

    TRACE("(%p, %d, %p)\n", pv, cb, pcbRead);

    tmp_pos.QuadPart = 0;
    IStream_Seek(This->base, tmp_pos, STREAM_SEEK_CUR, &base_pos);
    tmp_pos.QuadPart = This->pos.QuadPart + This->start.QuadPart;
    IStream_Seek(This->base, tmp_pos, STREAM_SEEK_SET, NULL);

    if(This->pos.QuadPart + cb > This->length.QuadPart)
        cb = This->length.QuadPart - This->pos.QuadPart;

    hr = IStream_Read(This->base, pv, cb, pcbRead);

    This->pos.QuadPart += *pcbRead;

    tmp_pos.QuadPart = base_pos.QuadPart;
    IStream_Seek(This->base, tmp_pos, STREAM_SEEK_SET, NULL);

    return hr;
}

static HRESULT WINAPI sub_stream_Write(
        IStream* iface,
        const void *pv,
        ULONG cb,
        ULONG *pcbWritten)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI sub_stream_Seek(
        IStream* iface,
        LARGE_INTEGER dlibMove,
        DWORD dwOrigin,
        ULARGE_INTEGER *plibNewPosition)
{
    sub_stream_t *This = impl_from_IStream(iface);
    LARGE_INTEGER new_pos;

    TRACE("(%08x.%08x, %x, %p)\n", dlibMove.u.HighPart, dlibMove.u.LowPart, dwOrigin, plibNewPosition);

    switch(dwOrigin)
    {
    case STREAM_SEEK_SET:
        new_pos = dlibMove;
        break;
    case STREAM_SEEK_CUR:
        new_pos.QuadPart = This->pos.QuadPart + dlibMove.QuadPart;
        break;
    case STREAM_SEEK_END:
        new_pos.QuadPart = This->length.QuadPart + dlibMove.QuadPart;
        break;
    default:
        return STG_E_INVALIDFUNCTION;
    }

    if(new_pos.QuadPart < 0) new_pos.QuadPart = 0;
    else if(new_pos.QuadPart > This->length.QuadPart) new_pos.QuadPart = This->length.QuadPart;

    This->pos.QuadPart = new_pos.QuadPart;

    if(plibNewPosition) *plibNewPosition = This->pos;
    return S_OK;
}

static HRESULT WINAPI sub_stream_SetSize(
        IStream* iface,
        ULARGE_INTEGER libNewSize)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI sub_stream_CopyTo(
        IStream* iface,
        IStream *pstm,
        ULARGE_INTEGER cb,
        ULARGE_INTEGER *pcbRead,
        ULARGE_INTEGER *pcbWritten)
{
    HRESULT        hr = S_OK;
    BYTE           tmpBuffer[128];
    ULONG          bytesRead, bytesWritten, copySize;
    ULARGE_INTEGER totalBytesRead;
    ULARGE_INTEGER totalBytesWritten;

    TRACE("(%p)->(%p, %d, %p, %p)\n", iface, pstm, cb.u.LowPart, pcbRead, pcbWritten);

    totalBytesRead.QuadPart = 0;
    totalBytesWritten.QuadPart = 0;

    while ( cb.QuadPart > 0 )
    {
        if ( cb.QuadPart >= sizeof(tmpBuffer) )
            copySize = sizeof(tmpBuffer);
        else
            copySize = cb.u.LowPart;

        hr = IStream_Read(iface, tmpBuffer, copySize, &bytesRead);
        if (FAILED(hr)) break;

        totalBytesRead.QuadPart += bytesRead;

        if (bytesRead)
        {
            hr = IStream_Write(pstm, tmpBuffer, bytesRead, &bytesWritten);
            if (FAILED(hr)) break;
            totalBytesWritten.QuadPart += bytesWritten;
        }

        if (bytesRead != copySize)
            cb.QuadPart = 0;
        else
            cb.QuadPart -= bytesRead;
    }

    if (pcbRead) pcbRead->QuadPart = totalBytesRead.QuadPart;
    if (pcbWritten) pcbWritten->QuadPart = totalBytesWritten.QuadPart;

    return hr;
}

static HRESULT WINAPI sub_stream_Commit(
        IStream* iface,
        DWORD grfCommitFlags)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI sub_stream_Revert(
        IStream* iface)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI sub_stream_LockRegion(
        IStream* iface,
        ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb,
        DWORD dwLockType)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI sub_stream_UnlockRegion(
        IStream* iface,
        ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb,
        DWORD dwLockType)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI sub_stream_Stat(
        IStream* iface,
        STATSTG *pstatstg,
        DWORD grfStatFlag)
{
    sub_stream_t *This = impl_from_IStream(iface);
    FIXME("(%p)->(%p, %08x)\n", This, pstatstg, grfStatFlag);
    memset(pstatstg, 0, sizeof(*pstatstg));
    pstatstg->cbSize = This->length;
    return S_OK;
}

static HRESULT WINAPI sub_stream_Clone(
        IStream* iface,
        IStream **ppstm)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static struct IStreamVtbl sub_stream_vtbl =
{
    sub_stream_QueryInterface,
    sub_stream_AddRef,
    sub_stream_Release,
    sub_stream_Read,
    sub_stream_Write,
    sub_stream_Seek,
    sub_stream_SetSize,
    sub_stream_CopyTo,
    sub_stream_Commit,
    sub_stream_Revert,
    sub_stream_LockRegion,
    sub_stream_UnlockRegion,
    sub_stream_Stat,
    sub_stream_Clone
};

static HRESULT create_sub_stream(IStream *stream, ULARGE_INTEGER start, ULARGE_INTEGER length, IStream **out)
{
    sub_stream_t *This;

    *out = NULL;
    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if(!This) return E_OUTOFMEMORY;

    This->lpVtbl = &sub_stream_vtbl;
    This->refs = 1;
    This->start = start;
    This->length = length;
    This->pos.QuadPart = 0;
    IStream_AddRef(stream);
    This->base = stream;

    *out = (IStream*)&This->lpVtbl;
    return S_OK;
}


typedef struct body_t
{
    struct list entry;
    HBODY hbody;
    IMimeBody *mime_body;

    struct body_t *parent;
    struct list children;
} body_t;

typedef struct MimeMessage
{
    const IMimeMessageVtbl *lpVtbl;

    LONG refs;
    IStream *stream;

    struct list body_tree;
    HBODY next_hbody;
} MimeMessage;

static HRESULT WINAPI MimeMessage_QueryInterface(IMimeMessage *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IPersist) ||
        IsEqualIID(riid, &IID_IPersistStreamInit) ||
        IsEqualIID(riid, &IID_IMimeMessageTree) ||
        IsEqualIID(riid, &IID_IMimeMessage))
    {
        *ppv = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    FIXME("no interface for %s\n", debugstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI MimeMessage_AddRef(IMimeMessage *iface)
{
    MimeMessage *This = (MimeMessage *)iface;
    TRACE("(%p)->()\n", iface);
    return InterlockedIncrement(&This->refs);
}

static void empty_body_list(struct list *list)
{
    body_t *body, *cursor2;
    LIST_FOR_EACH_ENTRY_SAFE(body, cursor2, list, body_t, entry)
    {
        empty_body_list(&body->children);
        list_remove(&body->entry);
        IMimeBody_Release(body->mime_body);
        HeapFree(GetProcessHeap(), 0, body);
    }
}

static ULONG WINAPI MimeMessage_Release(IMimeMessage *iface)
{
    MimeMessage *This = (MimeMessage *)iface;
    ULONG refs;

    TRACE("(%p)->()\n", iface);

    refs = InterlockedDecrement(&This->refs);
    if (!refs)
    {
        empty_body_list(&This->body_tree);

        if(This->stream) IStream_Release(This->stream);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refs;
}

/*** IPersist methods ***/
static HRESULT WINAPI MimeMessage_GetClassID(
    IMimeMessage *iface,
    CLSID *pClassID)
{
    FIXME("(%p)->(%p)\n", iface, pClassID);
    return E_NOTIMPL;
}

/*** IPersistStreamInit methods ***/
static HRESULT WINAPI MimeMessage_IsDirty(
    IMimeMessage *iface)
{
    FIXME("(%p)->()\n", iface);
    return E_NOTIMPL;
}

static body_t *new_body_entry(IMimeBody *mime_body, HBODY hbody, body_t *parent)
{
    body_t *body = HeapAlloc(GetProcessHeap(), 0, sizeof(*body));
    if(body)
    {
        body->mime_body = mime_body;
        body->hbody = hbody;
        list_init(&body->children);
        body->parent = parent;
    }
    return body;
}

typedef struct
{
    struct list entry;
    BODYOFFSETS offsets;
} offset_entry_t;

static HRESULT create_body_offset_list(IStream *stm, const char *boundary, struct list *body_offsets)
{
    HRESULT hr;
    DWORD read;
    int boundary_len = strlen(boundary);
    char *buf, *nl_boundary, *ptr, *overlap;
    DWORD start = 0, overlap_no;
    offset_entry_t *cur_body = NULL;
    ULARGE_INTEGER cur;
    LARGE_INTEGER zero;

    list_init(body_offsets);
    nl_boundary = HeapAlloc(GetProcessHeap(), 0, 4 + boundary_len + 1);
    memcpy(nl_boundary, "\r\n--", 4);
    memcpy(nl_boundary + 4, boundary, boundary_len + 1);

    overlap_no = boundary_len + 5;

    overlap = buf = HeapAlloc(GetProcessHeap(), 0, overlap_no + PARSER_BUF_SIZE + 1);

    zero.QuadPart = 0;
    hr = IStream_Seek(stm, zero, STREAM_SEEK_CUR, &cur);
    start = cur.u.LowPart;

    do {
        hr = IStream_Read(stm, overlap, PARSER_BUF_SIZE, &read);
        if(FAILED(hr)) goto end;
        if(read == 0) break;
        overlap[read] = '\0';

        ptr = buf;
        do {
            ptr = strstr(ptr, nl_boundary);
            if(ptr)
            {
                DWORD boundary_start = start + ptr - buf;
                char *end = ptr + boundary_len + 4;

                if(*end == '\0' || *(end + 1) == '\0')
                    break;

                if(*end == '\r' && *(end + 1) == '\n')
                {
                    if(cur_body)
                    {
                        cur_body->offsets.cbBodyEnd = boundary_start;
                        list_add_tail(body_offsets, &cur_body->entry);
                    }
                    cur_body = HeapAlloc(GetProcessHeap(), 0, sizeof(*cur_body));
                    cur_body->offsets.cbBoundaryStart = boundary_start + 2; /* doesn't including the leading \r\n */
                    cur_body->offsets.cbHeaderStart = boundary_start + boundary_len + 6;
                }
                else if(*end == '-' && *(end + 1) == '-')
                {
                    if(cur_body)
                    {
                        cur_body->offsets.cbBodyEnd = boundary_start;
                        list_add_tail(body_offsets, &cur_body->entry);
                        goto end;
                    }
                }
                ptr = end + 2;
            }
        } while(ptr);

        if(overlap == buf) /* 1st iteration */
        {
            memcpy(buf, buf + PARSER_BUF_SIZE - overlap_no, overlap_no);
            overlap = buf + overlap_no;
            start += read - overlap_no;
        }
        else
        {
            memcpy(buf, buf + PARSER_BUF_SIZE, overlap_no);
            start += read;
        }
    } while(1);

end:
    HeapFree(GetProcessHeap(), 0, buf);
    return hr;
}

static body_t *create_sub_body(MimeMessage *msg, IStream *pStm, BODYOFFSETS *offset, body_t *parent)
{
    IMimeBody *mime_body;
    HRESULT hr;
    body_t *body;
    ULARGE_INTEGER cur;
    LARGE_INTEGER zero;

    MimeBody_create(NULL, (void**)&mime_body);
    IMimeBody_Load(mime_body, pStm);
    zero.QuadPart = 0;
    hr = IStream_Seek(pStm, zero, STREAM_SEEK_CUR, &cur);
    offset->cbBodyStart = cur.u.LowPart + offset->cbHeaderStart;
    if(parent) MimeBody_set_offsets(impl_from_IMimeBody(mime_body), offset);
    IMimeBody_SetData(mime_body, IET_BINARY, NULL, NULL, &IID_IStream, pStm);
    body = new_body_entry(mime_body, msg->next_hbody, parent);
    msg->next_hbody = (HBODY)((DWORD)msg->next_hbody + 1);

    if(IMimeBody_IsContentType(mime_body, "multipart", NULL) == S_OK)
    {
        MIMEPARAMINFO *param_info;
        ULONG count, i;
        IMimeAllocator *alloc;

        hr = IMimeBody_GetParameters(mime_body, "Content-Type", &count, &param_info);
        if(hr != S_OK || count == 0) return body;

        MimeOleGetAllocator(&alloc);

        for(i = 0; i < count; i++)
        {
            if(!strcasecmp(param_info[i].pszName, "boundary"))
            {
                struct list offset_list;
                offset_entry_t *cur, *cursor2;
                hr = create_body_offset_list(pStm, param_info[i].pszData, &offset_list);
                LIST_FOR_EACH_ENTRY_SAFE(cur, cursor2, &offset_list, offset_entry_t, entry)
                {
                    body_t *sub_body;
                    IStream *sub_stream;
                    ULARGE_INTEGER start, length;

                    start.QuadPart = cur->offsets.cbHeaderStart;
                    length.QuadPart = cur->offsets.cbBodyEnd - cur->offsets.cbHeaderStart;
                    create_sub_stream(pStm, start, length, &sub_stream);
                    sub_body = create_sub_body(msg, sub_stream, &cur->offsets, body);
                    IStream_Release(sub_stream);
                    list_add_tail(&body->children, &sub_body->entry);
                    list_remove(&cur->entry);
                    HeapFree(GetProcessHeap(), 0, cur);
                }
                break;
            }
        }
        IMimeAllocator_FreeParamInfoArray(alloc, count, param_info, TRUE);
        IMimeAllocator_Release(alloc);
    }
    return body;
}

static HRESULT WINAPI MimeMessage_Load(
    IMimeMessage *iface,
    LPSTREAM pStm)
{
    MimeMessage *This = (MimeMessage *)iface;
    body_t *root_body;
    BODYOFFSETS offsets;
    ULARGE_INTEGER cur;
    LARGE_INTEGER zero;

    TRACE("(%p)->(%p)\n", iface, pStm);

    if(This->stream)
    {
        FIXME("already loaded a message\n");
        return E_FAIL;
    }

    IStream_AddRef(pStm);
    This->stream = pStm;
    offsets.cbBoundaryStart = offsets.cbHeaderStart = 0;
    offsets.cbBodyStart = offsets.cbBodyEnd = 0;

    root_body = create_sub_body(This, pStm, &offsets, NULL);

    zero.QuadPart = 0;
    IStream_Seek(pStm, zero, STREAM_SEEK_END, &cur);
    offsets.cbBodyEnd = cur.u.LowPart;
    MimeBody_set_offsets(impl_from_IMimeBody(root_body->mime_body), &offsets);

    list_add_head(&This->body_tree, &root_body->entry);

    return S_OK;
}

static HRESULT WINAPI MimeMessage_Save(
    IMimeMessage *iface,
    LPSTREAM pStm,
    BOOL fClearDirty)
{
    FIXME("(%p)->(%p, %s)\n", iface, pStm, fClearDirty ? "TRUE" : "FALSE");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetSizeMax(
    IMimeMessage *iface,
    ULARGE_INTEGER *pcbSize)
{
    FIXME("(%p)->(%p)\n", iface, pcbSize);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_InitNew(
    IMimeMessage *iface)
{
    FIXME("(%p)->()\n", iface);
    return E_NOTIMPL;
}

/*** IMimeMessageTree methods ***/
static HRESULT WINAPI MimeMessage_GetMessageSource(
    IMimeMessage *iface,
    IStream **ppStream,
    DWORD dwFlags)
{
    MimeMessage *This = (MimeMessage *)iface;
    FIXME("(%p)->(%p, 0x%x)\n", iface, ppStream, dwFlags);

    IStream_AddRef(This->stream);
    *ppStream = This->stream;
    return S_OK;
}

static HRESULT WINAPI MimeMessage_GetMessageSize(
    IMimeMessage *iface,
    ULONG *pcbSize,
    DWORD dwFlags)
{
    FIXME("(%p)->(%p, 0x%x)\n", iface, pcbSize, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_LoadOffsetTable(
    IMimeMessage *iface,
    IStream *pStream)
{
    FIXME("(%p)->(%p)\n", iface, pStream);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_SaveOffsetTable(
    IMimeMessage *iface,
    IStream *pStream,
    DWORD dwFlags)
{
    FIXME("(%p)->(%p, 0x%x)\n", iface, pStream, dwFlags);
    return E_NOTIMPL;
}


static HRESULT WINAPI MimeMessage_GetFlags(
    IMimeMessage *iface,
    DWORD *pdwFlags)
{
    FIXME("(%p)->(%p)\n", iface, pdwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_Commit(
    IMimeMessage *iface,
    DWORD dwFlags)
{
    FIXME("(%p)->(0x%x)\n", iface, dwFlags);
    return E_NOTIMPL;
}


static HRESULT WINAPI MimeMessage_HandsOffStorage(
    IMimeMessage *iface)
{
    FIXME("(%p)->()\n", iface);
    return E_NOTIMPL;
}

static HRESULT find_body(struct list *list, HBODY hbody, body_t **body)
{
    body_t *cur;
    HRESULT hr;

    if(hbody == HBODY_ROOT)
    {
        *body = LIST_ENTRY(list_head(list), body_t, entry);
        return S_OK;
    }

    LIST_FOR_EACH_ENTRY(cur, list, body_t, entry)
    {
        if(cur->hbody == hbody)
        {
            *body = cur;
            return S_OK;
        }
        hr = find_body(&cur->children, hbody, body);
        if(hr == S_OK) return S_OK;
    }
    return S_FALSE;
}

static HRESULT WINAPI MimeMessage_BindToObject(
    IMimeMessage *iface,
    const HBODY hBody,
    REFIID riid,
    void **ppvObject)
{
    MimeMessage *This = (MimeMessage *)iface;
    HRESULT hr;
    body_t *body;

    TRACE("(%p)->(%p, %s, %p)\n", iface, hBody, debugstr_guid(riid), ppvObject);

    hr = find_body(&This->body_tree, hBody, &body);

    if(hr != S_OK) return hr;

    if(IsEqualIID(riid, &IID_IMimeBody))
    {
        IMimeBody_AddRef(body->mime_body);
        *ppvObject = body->mime_body;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static HRESULT WINAPI MimeMessage_SaveBody(
    IMimeMessage *iface,
    HBODY hBody,
    DWORD dwFlags,
    IStream *pStream)
{
    FIXME("(%p)->(%p, 0x%x, %p)\n", iface, hBody, dwFlags, pStream);
    return E_NOTIMPL;
}

static HRESULT get_body(MimeMessage *msg, BODYLOCATION location, HBODY pivot, body_t **out)
{
    body_t *root = LIST_ENTRY(list_head(&msg->body_tree), body_t, entry);
    body_t *body;
    HRESULT hr;
    struct list *list;

    if(location == IBL_ROOT)
    {
        *out = root;
        return S_OK;
    }

    hr = find_body(&msg->body_tree, pivot, &body);

    if(hr == S_OK)
    {
        switch(location)
        {
        case IBL_PARENT:
            *out = body->parent;
            break;

        case IBL_FIRST:
            list = list_head(&body->children);
            if(list)
                *out = LIST_ENTRY(list, body_t, entry);
            else
                hr = MIME_E_NOT_FOUND;
            break;

        case IBL_LAST:
            list = list_tail(&body->children);
            if(list)
                *out = LIST_ENTRY(list, body_t, entry);
            else
                hr = MIME_E_NOT_FOUND;
            break;

        case IBL_NEXT:
            list = list_next(&body->parent->children, &body->entry);
            if(list)
                *out = LIST_ENTRY(list, body_t, entry);
            else
                hr = MIME_E_NOT_FOUND;
            break;

        case IBL_PREVIOUS:
            list = list_prev(&body->parent->children, &body->entry);
            if(list)
                *out = LIST_ENTRY(list, body_t, entry);
            else
                hr = MIME_E_NOT_FOUND;
            break;

        default:
            hr = E_FAIL;
            break;
        }
    }

    return hr;
}


static HRESULT WINAPI MimeMessage_InsertBody(
    IMimeMessage *iface,
    BODYLOCATION location,
    HBODY hPivot,
    LPHBODY phBody)
{
    FIXME("(%p)->(%d, %p, %p)\n", iface, location, hPivot, phBody);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetBody(
    IMimeMessage *iface,
    BODYLOCATION location,
    HBODY hPivot,
    LPHBODY phBody)
{
    MimeMessage *This = (MimeMessage *)iface;
    body_t *body;
    HRESULT hr;

    TRACE("(%p)->(%d, %p, %p)\n", iface, location, hPivot, phBody);

    hr = get_body(This, location, hPivot, &body);

    if(hr == S_OK) *phBody = body->hbody;

    return hr;
}

static HRESULT WINAPI MimeMessage_DeleteBody(
    IMimeMessage *iface,
    HBODY hBody,
    DWORD dwFlags)
{
    FIXME("(%p)->(%p, %08x)\n", iface, hBody, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_MoveBody(
    IMimeMessage *iface,
    HBODY hBody,
    BODYLOCATION location)
{
    FIXME("(%p)->(%d)\n", iface, location);
    return E_NOTIMPL;
}

static void count_children(body_t *body, boolean recurse, ULONG *count)
{
    body_t *child;

    LIST_FOR_EACH_ENTRY(child, &body->children, body_t, entry)
    {
        (*count)++;
        if(recurse) count_children(child, recurse, count);
    }
}

static HRESULT WINAPI MimeMessage_CountBodies(
    IMimeMessage *iface,
    HBODY hParent,
    boolean fRecurse,
    ULONG *pcBodies)
{
    HRESULT hr;
    MimeMessage *This = (MimeMessage *)iface;
    body_t *body;

    TRACE("(%p)->(%p, %s, %p)\n", iface, hParent, fRecurse ? "TRUE" : "FALSE", pcBodies);

    hr = find_body(&This->body_tree, hParent, &body);
    if(hr != S_OK) return hr;

    *pcBodies = 1;
    count_children(body, fRecurse, pcBodies);

    return S_OK;
}

static HRESULT find_next(IMimeMessage *msg, LPFINDBODY find_body, HBODY *out)
{
    HRESULT hr;
    IMimeBody *mime_body;
    HBODY next;

    if(find_body->dwReserved == 0)
        find_body->dwReserved = (DWORD)HBODY_ROOT;
    else
    {
        hr = IMimeMessage_GetBody(msg, IBL_FIRST, (HBODY)find_body->dwReserved, &next);
        if(hr == S_OK)
            find_body->dwReserved = (DWORD)next;
        else
        {
            hr = IMimeMessage_GetBody(msg, IBL_NEXT, (HBODY)find_body->dwReserved, &next);
            if(hr == S_OK)
                find_body->dwReserved = (DWORD)next;
            else
                return MIME_E_NOT_FOUND;
        }
    }

    hr = IMimeMessage_BindToObject(msg, (HBODY)find_body->dwReserved, &IID_IMimeBody, (void**)&mime_body);
    if(IMimeBody_IsContentType(mime_body, find_body->pszPriType, find_body->pszSubType) == S_OK)
    {
        IMimeBody_Release(mime_body);
        *out = (HBODY)find_body->dwReserved;
        return S_OK;
    }
    IMimeBody_Release(mime_body);
    return find_next(msg, find_body, out);
}

static HRESULT WINAPI MimeMessage_FindFirst(
    IMimeMessage *iface,
    LPFINDBODY pFindBody,
    LPHBODY phBody)
{
    TRACE("(%p)->(%p, %p)\n", iface, pFindBody, phBody);

    pFindBody->dwReserved = 0;
    return find_next(iface, pFindBody, phBody);
}

static HRESULT WINAPI MimeMessage_FindNext(
    IMimeMessage *iface,
    LPFINDBODY pFindBody,
    LPHBODY phBody)
{
    TRACE("(%p)->(%p, %p)\n", iface, pFindBody, phBody);

    return find_next(iface, pFindBody, phBody);
}

static HRESULT WINAPI MimeMessage_ResolveURL(
    IMimeMessage *iface,
    HBODY hRelated,
    LPCSTR pszBase,
    LPCSTR pszURL,
    DWORD dwFlags,
    LPHBODY phBody)
{
    FIXME("(%p)->(%p, %s, %s, 0x%x, %p)\n", iface, hRelated, pszBase, pszURL, dwFlags, phBody);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_ToMultipart(
    IMimeMessage *iface,
    HBODY hBody,
    LPCSTR pszSubType,
    LPHBODY phMultipart)
{
    FIXME("(%p)->(%p, %s, %p)\n", iface, hBody, pszSubType, phMultipart);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetBodyOffsets(
    IMimeMessage *iface,
    HBODY hBody,
    LPBODYOFFSETS pOffsets)
{
    FIXME("(%p)->(%p, %p)\n", iface, hBody, pOffsets);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetCharset(
    IMimeMessage *iface,
    LPHCHARSET phCharset)
{
    FIXME("(%p)->(%p)\n", iface, phCharset);
    *phCharset = NULL;
    return S_OK;
}

static HRESULT WINAPI MimeMessage_SetCharset(
    IMimeMessage *iface,
    HCHARSET hCharset,
    CSETAPPLYTYPE applytype)
{
    FIXME("(%p)->(%p, %d)\n", iface, hCharset, applytype);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_IsBodyType(
    IMimeMessage *iface,
    HBODY hBody,
    IMSGBODYTYPE bodytype)
{
    HRESULT hr;
    IMimeBody *mime_body;
    TRACE("(%p)->(%p, %d)\n", iface, hBody, bodytype);

    hr = IMimeMessage_BindToObject(iface, hBody, &IID_IMimeBody, (void**)&mime_body);
    if(hr != S_OK) return hr;

    hr = IMimeBody_IsType(mime_body, bodytype);
    MimeBody_Release(mime_body);
    return hr;
}

static HRESULT WINAPI MimeMessage_IsContentType(
    IMimeMessage *iface,
    HBODY hBody,
    LPCSTR pszPriType,
    LPCSTR pszSubType)
{
    HRESULT hr;
    IMimeBody *mime_body;
    TRACE("(%p)->(%p, %s, %s)\n", iface, hBody, pszPriType, pszSubType);

    hr = IMimeMessage_BindToObject(iface, hBody, &IID_IMimeBody, (void**)&mime_body);
    if(FAILED(hr)) return hr;

    hr = IMimeBody_IsContentType(mime_body, pszPriType, pszSubType);
    IMimeBody_Release(mime_body);
    return hr;
}

static HRESULT WINAPI MimeMessage_QueryBodyProp(
    IMimeMessage *iface,
    HBODY hBody,
    LPCSTR pszName,
    LPCSTR pszCriteria,
    boolean fSubString,
    boolean fCaseSensitive)
{
    FIXME("(%p)->(%p, %s, %s, %s, %s)\n", iface, hBody, pszName, pszCriteria, fSubString ? "TRUE" : "FALSE", fCaseSensitive ? "TRUE" : "FALSE");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetBodyProp(
    IMimeMessage *iface,
    HBODY hBody,
    LPCSTR pszName,
    DWORD dwFlags,
    LPPROPVARIANT pValue)
{
    HRESULT hr;
    IMimeBody *mime_body;

    TRACE("(%p)->(%p, %s, 0x%x, %p)\n", iface, hBody, pszName, dwFlags, pValue);

    hr = IMimeMessage_BindToObject(iface, hBody, &IID_IMimeBody, (void**)&mime_body);
    if(hr != S_OK) return hr;

    hr = IMimeBody_GetProp(mime_body, pszName, dwFlags, pValue);
    IMimeBody_Release(mime_body);

    return hr;
}

static HRESULT WINAPI MimeMessage_SetBodyProp(
    IMimeMessage *iface,
    HBODY hBody,
    LPCSTR pszName,
    DWORD dwFlags,
    LPCPROPVARIANT pValue)
{
    FIXME("(%p)->(%p, %s, 0x%x, %p)\n", iface, hBody, pszName, dwFlags, pValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_DeleteBodyProp(
    IMimeMessage *iface,
    HBODY hBody,
    LPCSTR pszName)
{
    FIXME("(%p)->(%p, %s)\n", iface, hBody, pszName);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_SetOption(
    IMimeMessage *iface,
    const TYPEDID oid,
    LPCPROPVARIANT pValue)
{
    HRESULT hr = E_NOTIMPL;
    TRACE("(%p)->(%08x, %p)\n", iface, oid, pValue);

    if(pValue->vt != TYPEDID_TYPE(oid))
    {
        WARN("Called with vartype %04x and oid %08x\n", pValue->vt, oid);
        return E_INVALIDARG;
    }

    switch(oid)
    {
    case OID_HIDE_TNEF_ATTACHMENTS:
        FIXME("OID_HIDE_TNEF_ATTACHMENTS (value %d): ignoring\n", pValue->u.boolVal);
        hr = S_OK;
        break;
    case OID_SHOW_MACBINARY:
        FIXME("OID_SHOW_MACBINARY (value %d): ignoring\n", pValue->u.boolVal);
        hr = S_OK;
        break;
    default:
        FIXME("Unhandled oid %08x\n", oid);
    }

    return hr;
}

static HRESULT WINAPI MimeMessage_GetOption(
    IMimeMessage *iface,
    const TYPEDID oid,
    LPPROPVARIANT pValue)
{
    FIXME("(%p)->(%08x, %p)\n", iface, oid, pValue);
    return E_NOTIMPL;
}

/*** IMimeMessage methods ***/
static HRESULT WINAPI MimeMessage_CreateWebPage(
    IMimeMessage *iface,
    IStream *pRootStm,
    LPWEBPAGEOPTIONS pOptions,
    IMimeMessageCallback *pCallback,
    IMoniker **ppMoniker)
{
    FIXME("(%p)->(%p, %p, %p, %p)\n", iface, pRootStm, pOptions, pCallback, ppMoniker);
    *ppMoniker = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetProp(
    IMimeMessage *iface,
    LPCSTR pszName,
    DWORD dwFlags,
    LPPROPVARIANT pValue)
{
    FIXME("(%p)->(%s, 0x%x, %p)\n", iface, pszName, dwFlags, pValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_SetProp(
    IMimeMessage *iface,
    LPCSTR pszName,
    DWORD dwFlags,
    LPCPROPVARIANT pValue)
{
    FIXME("(%p)->(%s, 0x%x, %p)\n", iface, pszName, dwFlags, pValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_DeleteProp(
    IMimeMessage *iface,
    LPCSTR pszName)
{
    FIXME("(%p)->(%s)\n", iface, pszName);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_QueryProp(
    IMimeMessage *iface,
    LPCSTR pszName,
    LPCSTR pszCriteria,
    boolean fSubString,
    boolean fCaseSensitive)
{
    FIXME("(%p)->(%s, %s, %s, %s)\n", iface, pszName, pszCriteria, fSubString ? "TRUE" : "FALSE", fCaseSensitive ? "TRUE" : "FALSE");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetTextBody(
    IMimeMessage *iface,
    DWORD dwTxtType,
    ENCODINGTYPE ietEncoding,
    IStream **pStream,
    LPHBODY phBody)
{
    HRESULT hr;
    HBODY hbody;
    FINDBODY find_struct;
    IMimeBody *mime_body;
    static char text[] = "text";
    static char plain[] = "plain";
    static char html[] = "html";

    TRACE("(%p)->(%d, %d, %p, %p)\n", iface, dwTxtType, ietEncoding, pStream, phBody);

    find_struct.pszPriType = text;

    switch(dwTxtType)
    {
    case TXT_PLAIN:
        find_struct.pszSubType = plain;
        break;
    case TXT_HTML:
        find_struct.pszSubType = html;
        break;
    default:
        return MIME_E_INVALID_TEXT_TYPE;
    }

    hr = IMimeMessage_FindFirst(iface, &find_struct, &hbody);
    if(hr != S_OK)
    {
        TRACE("not found hr %08x\n", hr);
        *phBody = NULL;
        return hr;
    }

    IMimeMessage_BindToObject(iface, hbody, &IID_IMimeBody, (void**)&mime_body);

    IMimeBody_GetData(mime_body, ietEncoding, pStream);
    *phBody = hbody;
    IMimeBody_Release(mime_body);
    return hr;
}

static HRESULT WINAPI MimeMessage_SetTextBody(
    IMimeMessage *iface,
    DWORD dwTxtType,
    ENCODINGTYPE ietEncoding,
    HBODY hAlternative,
    IStream *pStream,
    LPHBODY phBody)
{
    FIXME("(%p)->(%d, %d, %p, %p, %p)\n", iface, dwTxtType, ietEncoding, hAlternative, pStream, phBody);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_AttachObject(
    IMimeMessage *iface,
    REFIID riid,
    void *pvObject,
    LPHBODY phBody)
{
    FIXME("(%p)->(%s, %p, %p)\n", iface, debugstr_guid(riid), pvObject, phBody);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_AttachFile(
    IMimeMessage *iface,
    LPCSTR pszFilePath,
    IStream *pstmFile,
    LPHBODY phBody)
{
    FIXME("(%p)->(%s, %p, %p)\n", iface, pszFilePath, pstmFile, phBody);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_AttachURL(
    IMimeMessage *iface,
    LPCSTR pszBase,
    LPCSTR pszURL,
    DWORD dwFlags,
    IStream *pstmURL,
    LPSTR *ppszCIDURL,
    LPHBODY phBody)
{
    FIXME("(%p)->(%s, %s, 0x%x, %p, %p, %p)\n", iface, pszBase, pszURL, dwFlags, pstmURL, ppszCIDURL, phBody);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetAttachments(
    IMimeMessage *iface,
    ULONG *pcAttach,
    LPHBODY *pprghAttach)
{
    HRESULT hr;
    FINDBODY find_struct;
    HBODY hbody;
    LPHBODY array;
    ULONG size = 10;

    TRACE("(%p)->(%p, %p)\n", iface, pcAttach, pprghAttach);

    *pcAttach = 0;
    array = CoTaskMemAlloc(size * sizeof(HBODY));

    find_struct.pszPriType = find_struct.pszSubType = NULL;
    hr = IMimeMessage_FindFirst(iface, &find_struct, &hbody);
    while(hr == S_OK)
    {
        hr = IMimeMessage_IsContentType(iface, hbody, "multipart", NULL);
        TRACE("IsCT rets %08x %d\n", hr, *pcAttach);
        if(hr != S_OK)
        {
            if(*pcAttach + 1 > size)
            {
                size *= 2;
                array = CoTaskMemRealloc(array, size * sizeof(HBODY));
            }
            array[*pcAttach] = hbody;
            (*pcAttach)++;
        }
        hr = IMimeMessage_FindNext(iface, &find_struct, &hbody);
    }

    *pprghAttach = array;
    return S_OK;
}

static HRESULT WINAPI MimeMessage_GetAddressTable(
    IMimeMessage *iface,
    IMimeAddressTable **ppTable)
{
    FIXME("(%p)->(%p)\n", iface, ppTable);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetSender(
    IMimeMessage *iface,
    LPADDRESSPROPS pAddress)
{
    FIXME("(%p)->(%p)\n", iface, pAddress);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetAddressTypes(
    IMimeMessage *iface,
    DWORD dwAdrTypes,
    DWORD dwProps,
    LPADDRESSLIST pList)
{
    FIXME("(%p)->(%d, %d, %p)\n", iface, dwAdrTypes, dwProps, pList);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetAddressFormat(
    IMimeMessage *iface,
    DWORD dwAdrTypes,
    ADDRESSFORMAT format,
    LPSTR *ppszFormat)
{
    FIXME("(%p)->(%d, %d, %p)\n", iface, dwAdrTypes, format, ppszFormat);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_EnumAddressTypes(
    IMimeMessage *iface,
    DWORD dwAdrTypes,
    DWORD dwProps,
    IMimeEnumAddressTypes **ppEnum)
{
    FIXME("(%p)->(%d, %d, %p)\n", iface, dwAdrTypes, dwProps, ppEnum);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_SplitMessage(
    IMimeMessage *iface,
    ULONG cbMaxPart,
    IMimeMessageParts **ppParts)
{
    FIXME("(%p)->(%d, %p)\n", iface, cbMaxPart, ppParts);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetRootMoniker(
    IMimeMessage *iface,
    IMoniker **ppMoniker)
{
    FIXME("(%p)->(%p)\n", iface, ppMoniker);
    return E_NOTIMPL;
}

static const IMimeMessageVtbl MimeMessageVtbl =
{
    MimeMessage_QueryInterface,
    MimeMessage_AddRef,
    MimeMessage_Release,
    MimeMessage_GetClassID,
    MimeMessage_IsDirty,
    MimeMessage_Load,
    MimeMessage_Save,
    MimeMessage_GetSizeMax,
    MimeMessage_InitNew,
    MimeMessage_GetMessageSource,
    MimeMessage_GetMessageSize,
    MimeMessage_LoadOffsetTable,
    MimeMessage_SaveOffsetTable,
    MimeMessage_GetFlags,
    MimeMessage_Commit,
    MimeMessage_HandsOffStorage,
    MimeMessage_BindToObject,
    MimeMessage_SaveBody,
    MimeMessage_InsertBody,
    MimeMessage_GetBody,
    MimeMessage_DeleteBody,
    MimeMessage_MoveBody,
    MimeMessage_CountBodies,
    MimeMessage_FindFirst,
    MimeMessage_FindNext,
    MimeMessage_ResolveURL,
    MimeMessage_ToMultipart,
    MimeMessage_GetBodyOffsets,
    MimeMessage_GetCharset,
    MimeMessage_SetCharset,
    MimeMessage_IsBodyType,
    MimeMessage_IsContentType,
    MimeMessage_QueryBodyProp,
    MimeMessage_GetBodyProp,
    MimeMessage_SetBodyProp,
    MimeMessage_DeleteBodyProp,
    MimeMessage_SetOption,
    MimeMessage_GetOption,
    MimeMessage_CreateWebPage,
    MimeMessage_GetProp,
    MimeMessage_SetProp,
    MimeMessage_DeleteProp,
    MimeMessage_QueryProp,
    MimeMessage_GetTextBody,
    MimeMessage_SetTextBody,
    MimeMessage_AttachObject,
    MimeMessage_AttachFile,
    MimeMessage_AttachURL,
    MimeMessage_GetAttachments,
    MimeMessage_GetAddressTable,
    MimeMessage_GetSender,
    MimeMessage_GetAddressTypes,
    MimeMessage_GetAddressFormat,
    MimeMessage_EnumAddressTypes,
    MimeMessage_SplitMessage,
    MimeMessage_GetRootMoniker,
};

HRESULT MimeMessage_create(IUnknown *outer, void **obj)
{
    MimeMessage *This;

    TRACE("(%p, %p)\n", outer, obj);

    if (outer)
    {
        FIXME("outer unknown not supported yet\n");
        return E_NOTIMPL;
    }

    *obj = NULL;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &MimeMessageVtbl;
    This->refs = 1;
    This->stream = NULL;
    list_init(&This->body_tree);
    This->next_hbody = (HBODY)1;

    *obj = (IMimeMessage *)&This->lpVtbl;
    return S_OK;
}

/***********************************************************************
 *              MimeOleCreateMessage (INETCOMM.@)
 */
HRESULT WINAPI MimeOleCreateMessage(IUnknown *pUnkOuter, IMimeMessage **ppMessage)
{
    TRACE("(%p, %p)\n", pUnkOuter, ppMessage);
    return MimeMessage_create(NULL, (void **)ppMessage);
}

/***********************************************************************
 *              MimeOleSetCompatMode (INETCOMM.@)
 */
HRESULT WINAPI MimeOleSetCompatMode(DWORD dwMode)
{
    FIXME("(0x%x)\n", dwMode);
    return S_OK;
}

/***********************************************************************
 *              MimeOleCreateVirtualStream (INETCOMM.@)
 */
HRESULT WINAPI MimeOleCreateVirtualStream(IStream **ppStream)
{
    HRESULT hr;
    FIXME("(%p)\n", ppStream);

    hr = CreateStreamOnHGlobal(NULL, TRUE, ppStream);
    return hr;
}

typedef struct MimeSecurity
{
    const IMimeSecurityVtbl *lpVtbl;

    LONG refs;
} MimeSecurity;

static HRESULT WINAPI MimeSecurity_QueryInterface(
        IMimeSecurity* iface,
        REFIID riid,
        void** obj)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IMimeSecurity))
    {
        *obj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    FIXME("no interface for %s\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI MimeSecurity_AddRef(
        IMimeSecurity* iface)
{
    MimeSecurity *This = (MimeSecurity *)iface;
    TRACE("(%p)->()\n", iface);
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI MimeSecurity_Release(
        IMimeSecurity* iface)
{
    MimeSecurity *This = (MimeSecurity *)iface;
    ULONG refs;

    TRACE("(%p)->()\n", iface);

    refs = InterlockedDecrement(&This->refs);
    if (!refs)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refs;
}

static HRESULT WINAPI MimeSecurity_InitNew(
        IMimeSecurity* iface)
{
    FIXME("(%p)->(): stub\n", iface);
    return S_OK;
}

static HRESULT WINAPI MimeSecurity_CheckInit(
        IMimeSecurity* iface)
{
    FIXME("(%p)->(): stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeSecurity_EncodeMessage(
        IMimeSecurity* iface,
        IMimeMessageTree* pTree,
        DWORD dwFlags)
{
    FIXME("(%p)->(%p, %08x): stub\n", iface, pTree, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeSecurity_EncodeBody(
        IMimeSecurity* iface,
        IMimeMessageTree* pTree,
        HBODY hEncodeRoot,
        DWORD dwFlags)
{
    FIXME("(%p)->(%p, %p, %08x): stub\n", iface, pTree, hEncodeRoot, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeSecurity_DecodeMessage(
        IMimeSecurity* iface,
        IMimeMessageTree* pTree,
        DWORD dwFlags)
{
    FIXME("(%p)->(%p, %08x): stub\n", iface, pTree, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeSecurity_DecodeBody(
        IMimeSecurity* iface,
        IMimeMessageTree* pTree,
        HBODY hDecodeRoot,
        DWORD dwFlags)
{
    FIXME("(%p)->(%p, %p, %08x): stub\n", iface, pTree, hDecodeRoot, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeSecurity_EnumCertificates(
        IMimeSecurity* iface,
        HCAPICERTSTORE hc,
        DWORD dwUsage,
        PCX509CERT pPrev,
        PCX509CERT* ppCert)
{
    FIXME("(%p)->(%p, %08x, %p, %p): stub\n", iface, hc, dwUsage, pPrev, ppCert);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeSecurity_GetCertificateName(
        IMimeSecurity* iface,
        const PCX509CERT pX509Cert,
        const CERTNAMETYPE cn,
        LPSTR* ppszName)
{
    FIXME("(%p)->(%p, %08x, %p): stub\n", iface, pX509Cert, cn, ppszName);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeSecurity_GetMessageType(
        IMimeSecurity* iface,
        const HWND hwndParent,
        IMimeBody* pBody,
        DWORD* pdwSecType)
{
    FIXME("(%p)->(%p, %p, %p): stub\n", iface, hwndParent, pBody, pdwSecType);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeSecurity_GetCertData(
        IMimeSecurity* iface,
        const PCX509CERT pX509Cert,
        const CERTDATAID dataid,
        LPPROPVARIANT pValue)
{
    FIXME("(%p)->(%p, %x, %p): stub\n", iface, pX509Cert, dataid, pValue);
    return E_NOTIMPL;
}


static const IMimeSecurityVtbl MimeSecurityVtbl =
{
    MimeSecurity_QueryInterface,
    MimeSecurity_AddRef,
    MimeSecurity_Release,
    MimeSecurity_InitNew,
    MimeSecurity_CheckInit,
    MimeSecurity_EncodeMessage,
    MimeSecurity_EncodeBody,
    MimeSecurity_DecodeMessage,
    MimeSecurity_DecodeBody,
    MimeSecurity_EnumCertificates,
    MimeSecurity_GetCertificateName,
    MimeSecurity_GetMessageType,
    MimeSecurity_GetCertData
};

HRESULT MimeSecurity_create(IUnknown *outer, void **obj)
{
    MimeSecurity *This;

    *obj = NULL;

    if (outer) return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &MimeSecurityVtbl;
    This->refs = 1;

    *obj = (IMimeSecurity *)&This->lpVtbl;
    return S_OK;
}

/***********************************************************************
 *              MimeOleCreateSecurity (INETCOMM.@)
 */
HRESULT WINAPI MimeOleCreateSecurity(IMimeSecurity **ppSecurity)
{
    return MimeSecurity_create(NULL, (void **)ppSecurity);
}

typedef struct
{
    IMimeAllocatorVtbl *lpVtbl;
} MimeAllocator;

static HRESULT WINAPI MimeAlloc_QueryInterface(
        IMimeAllocator* iface,
        REFIID riid,
        void **obj)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IMalloc) ||
        IsEqualIID(riid, &IID_IMimeAllocator))
    {
        *obj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    FIXME("no interface for %s\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI MimeAlloc_AddRef(
        IMimeAllocator* iface)
{
    return 2;
}

static ULONG WINAPI MimeAlloc_Release(
        IMimeAllocator* iface)
{
    return 1;
}

static LPVOID WINAPI MimeAlloc_Alloc(
        IMimeAllocator* iface,
        ULONG cb)
{
    return CoTaskMemAlloc(cb);
}

static LPVOID WINAPI MimeAlloc_Realloc(
        IMimeAllocator* iface,
        LPVOID pv,
        ULONG cb)
{
    return CoTaskMemRealloc(pv, cb);
}

static void WINAPI MimeAlloc_Free(
        IMimeAllocator* iface,
        LPVOID pv)
{
    CoTaskMemFree(pv);
}

static ULONG WINAPI MimeAlloc_GetSize(
        IMimeAllocator* iface,
        LPVOID pv)
{
    FIXME("stub\n");
    return 0;
}

static int WINAPI MimeAlloc_DidAlloc(
        IMimeAllocator* iface,
        LPVOID pv)
{
    FIXME("stub\n");
    return 0;
}

static void WINAPI MimeAlloc_HeapMinimize(
        IMimeAllocator* iface)
{
    FIXME("stub\n");
    return;
}

static HRESULT WINAPI MimeAlloc_FreeParamInfoArray(
        IMimeAllocator* iface,
        ULONG cParams,
        LPMIMEPARAMINFO prgParam,
        boolean fFreeArray)
{
    ULONG i;
    TRACE("(%p)->(%d, %p, %d)\n", iface, cParams, prgParam, fFreeArray);

    for(i = 0; i < cParams; i++)
    {
        IMimeAllocator_Free(iface, prgParam[i].pszName);
        IMimeAllocator_Free(iface, prgParam[i].pszData);
    }
    if(fFreeArray) IMimeAllocator_Free(iface, prgParam);
    return S_OK;
}

static HRESULT WINAPI MimeAlloc_FreeAddressList(
        IMimeAllocator* iface,
        LPADDRESSLIST pList)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeAlloc_FreeAddressProps(
        IMimeAllocator* iface,
        LPADDRESSPROPS pAddress)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeAlloc_ReleaseObjects(
        IMimeAllocator* iface,
        ULONG cObjects,
        IUnknown **prgpUnknown,
        boolean fFreeArray)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI MimeAlloc_FreeEnumHeaderRowArray(
        IMimeAllocator* iface,
        ULONG cRows,
        LPENUMHEADERROW prgRow,
        boolean fFreeArray)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeAlloc_FreeEnumPropertyArray(
        IMimeAllocator* iface,
        ULONG cProps,
        LPENUMPROPERTY prgProp,
        boolean fFreeArray)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeAlloc_FreeThumbprint(
        IMimeAllocator* iface,
        THUMBBLOB *pthumbprint)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI MimeAlloc_PropVariantClear(
        IMimeAllocator* iface,
        LPPROPVARIANT pProp)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static IMimeAllocatorVtbl mime_alloc_vtbl =
{
    MimeAlloc_QueryInterface,
    MimeAlloc_AddRef,
    MimeAlloc_Release,
    MimeAlloc_Alloc,
    MimeAlloc_Realloc,
    MimeAlloc_Free,
    MimeAlloc_GetSize,
    MimeAlloc_DidAlloc,
    MimeAlloc_HeapMinimize,
    MimeAlloc_FreeParamInfoArray,
    MimeAlloc_FreeAddressList,
    MimeAlloc_FreeAddressProps,
    MimeAlloc_ReleaseObjects,
    MimeAlloc_FreeEnumHeaderRowArray,
    MimeAlloc_FreeEnumPropertyArray,
    MimeAlloc_FreeThumbprint,
    MimeAlloc_PropVariantClear
};

static MimeAllocator mime_allocator =
{
    &mime_alloc_vtbl
};

HRESULT MimeAllocator_create(IUnknown *outer, void **obj)
{
    if(outer) return CLASS_E_NOAGGREGATION;

    *obj = &mime_allocator;
    return S_OK;
}

HRESULT WINAPI MimeOleGetAllocator(IMimeAllocator **alloc)
{
    return MimeAllocator_create(NULL, (void**)alloc);
}
