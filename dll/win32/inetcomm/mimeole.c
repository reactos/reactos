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

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winuser.h"
#include "objbase.h"
#include "ole2.h"
#include "mimeole.h"
#include "propvarutil.h"

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
    {"X-Newsgroup",                  PID_HDR_NEWSGROUP,    0,                               VT_LPSTR},
    {"Newsgroups",                   PID_HDR_NEWSGROUPS,   0,                               VT_LPSTR},
    {"References",                   PID_HDR_REFS,         0,                               VT_LPSTR},
    {"Subject",                      PID_HDR_SUBJECT,      0,                               VT_LPSTR},
    {"From",                         PID_HDR_FROM,         MPF_ADDRESS,                     VT_LPSTR},
    {"Message-ID",                   PID_HDR_MESSAGEID,    0,                               VT_LPSTR},
    {"Return-Path",                  PID_HDR_RETURNPATH,   MPF_ADDRESS,                     VT_LPSTR},
    {"Rr",                           PID_HDR_RR,           0,                               VT_LPSTR},
    {"Return-Receipt-To",            PID_HDR_RETRCPTO,     MPF_ADDRESS,                     VT_LPSTR},
    {"Apparently-To",                PID_HDR_APPARTO,      MPF_ADDRESS,                     VT_LPSTR},
    {"Date",                         PID_HDR_DATE,         0,                               VT_LPSTR},
    {"Received",                     PID_HDR_RECEIVED,     0,                               VT_LPSTR},
    {"Reply-To",                     PID_HDR_REPLYTO,      MPF_ADDRESS,                     VT_LPSTR},
    {"X-Mailer",                     PID_HDR_XMAILER,      0,                               VT_LPSTR},
    {"Bcc",                          PID_HDR_BCC,          MPF_ADDRESS,                     VT_LPSTR},
    {"MIME-Version",                 PID_HDR_MIMEVER,      MPF_MIME,                        VT_LPSTR},
    {"Content-Type",                 PID_HDR_CNTTYPE,      MPF_MIME | MPF_HASPARAMS,        VT_LPSTR},
    {"Content-Transfer-Encoding",    PID_HDR_CNTXFER,      MPF_MIME,                        VT_LPSTR},
    {"Content-ID",                   PID_HDR_CNTID,        MPF_MIME,                        VT_LPSTR},
    {"Content-Description",          PID_HDR_CNTDESC,      MPF_MIME,                        VT_LPSTR},
    {"Content-Disposition",          PID_HDR_CNTDISP,      MPF_MIME | MPF_HASPARAMS,        VT_LPSTR},
    {"Content-Base",                 PID_HDR_CNTBASE,      MPF_MIME,                        VT_LPSTR},
    {"Content-Location",             PID_HDR_CNTLOC,       MPF_MIME,                        VT_LPSTR},
    {"To",                           PID_HDR_TO,           MPF_ADDRESS,                     VT_LPSTR},
    {"Path",                         PID_HDR_PATH,         0,                               VT_LPSTR},
    {"Followup-To",                  PID_HDR_FOLLOWUPTO,   0,                               VT_LPSTR},
    {"Expires",                      PID_HDR_EXPIRES,      0,                               VT_LPSTR},
    {"Cc",                           PID_HDR_CC,           MPF_ADDRESS,                     VT_LPSTR},
    {"Control",                      PID_HDR_CONTROL,      0,                               VT_LPSTR},
    {"Distribution",                 PID_HDR_DISTRIB,      0,                               VT_LPSTR},
    {"Keywords",                     PID_HDR_KEYWORDS,     0,                               VT_LPSTR},
    {"Summary",                      PID_HDR_SUMMARY,      0,                               VT_LPSTR},
    {"Approved",                     PID_HDR_APPROVED,     0,                               VT_LPSTR},
    {"Lines",                        PID_HDR_LINES,        0,                               VT_LPSTR},
    {"Xref",                         PID_HDR_XREF,         0,                               VT_LPSTR},
    {"Organization",                 PID_HDR_ORG,          0,                               VT_LPSTR},
    {"X-Newsreader",                 PID_HDR_XNEWSRDR,     0,                               VT_LPSTR},
    {"X-Priority",                   PID_HDR_XPRI,         0,                               VT_LPSTR},
    {"X-MSMail-Priority",            PID_HDR_XMSPRI,       0,                               VT_LPSTR},
    {"par:content-disposition:filename", PID_PAR_FILENAME, 0,                               VT_LPSTR},
    {"par:content-type:boundary",    PID_PAR_BOUNDARY,     0,                               VT_LPSTR},
    {"par:content-type:charset",     PID_PAR_CHARSET,      0,                               VT_LPSTR},
    {"par:content-type:name",        PID_PAR_NAME,         0,                               VT_LPSTR},
    {"att:filename",                 PID_ATT_FILENAME,     0,                               VT_LPSTR},
    {"att:pri-content-type",         PID_ATT_PRITYPE,      0,                               VT_LPSTR},
    {"att:sub-content-type",         PID_ATT_SUBTYPE,      0,                               VT_LPSTR},
    {"att:illegal-lines",            PID_ATT_ILLEGAL,      0,                               VT_LPSTR},
    {"att:rendered",                 PID_ATT_RENDERED,     0,                               VT_LPSTR},
    {"att:sent-time",                PID_ATT_SENTTIME,     0,                               VT_LPSTR},
    {"att:priority",                 PID_ATT_PRIORITY,     0,                               VT_LPSTR},
    {"Comment",                      PID_HDR_COMMENT,      0,                               VT_LPSTR},
    {"Encoding",                     PID_HDR_ENCODING,     0,                               VT_LPSTR},
    {"Encrypted",                    PID_HDR_ENCRYPTED,    0,                               VT_LPSTR},
    {"X-Offsets",                    PID_HDR_OFFSETS,      0,                               VT_LPSTR},
    {"X-Unsent",                     PID_HDR_XUNSENT,      0,                               VT_LPSTR},
    {"X-ArticleId",                  PID_HDR_ARTICLEID,    0,                               VT_LPSTR},
    {"Sender",                       PID_HDR_SENDER,       MPF_ADDRESS,                     VT_LPSTR},
    {"att:athena-server",            PID_ATT_SERVER,       0,                               VT_LPSTR},
    {"att:athena-account-id",        PID_ATT_ACCOUNT,      0,                               VT_LPSTR},
    {"att:athena-pop3-uidl",         PID_ATT_UIDL,         0,                               VT_LPSTR},
    {"att:athena-store-msgid",       PID_ATT_STOREMSGID,   0,                               VT_LPSTR},
    {"att:athena-user-name",         PID_ATT_USERNAME,     0,                               VT_LPSTR},
    {"att:athena-forward-to",        PID_ATT_FORWARDTO,    0,                               VT_LPSTR},
    {"att:athena-store-fdrid",       PID_ATT_STOREFOLDERID,0,                               VT_LPSTR},
    {"att:athena-ghosted",           PID_ATT_GHOSTED,      0,                               VT_LPSTR},
    {"att:athena-uncachedsize",      PID_ATT_UNCACHEDSIZE, 0,                               VT_LPSTR},
    {"att:athena-combined",          PID_ATT_COMBINED,     0,                               VT_LPSTR},
    {"att:auto-inlined",             PID_ATT_AUTOINLINED,  0,                               VT_LPSTR},
    {"Disposition-Notification-To",  PID_HDR_DISP_NOTIFICATION_TO,  0,                      VT_LPSTR},
    {"par:Content-Type:reply-type",  PID_PAR_REPLYTYPE,    0,                               VT_LPSTR},
    {"par:Content-Type:format",      PID_PAR_FORMAT ,      0,                               VT_LPSTR},
    {"att:format",                   PID_ATT_FORMAT ,      0,                               VT_LPSTR},
    {"In-Reply-To",                  PID_HDR_INREPLYTO,    0,                               VT_LPSTR},
    {"att:athena-account-name",      PID_ATT_ACCOUNTNAME,  0,                               VT_LPSTR},
    {NULL,                           0,                    0,                               0}
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
    IMimeBody IMimeBody_iface;
    LONG ref;

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

typedef struct
{
    IStream IStream_iface;
    LONG ref;
    IStream *base;
    ULARGE_INTEGER pos, start, length;
} sub_stream_t;

static inline sub_stream_t *impl_from_IStream(IStream *iface)
{
    return CONTAINING_RECORD(iface, sub_stream_t, IStream_iface);
}

static HRESULT WINAPI sub_stream_QueryInterface(IStream *iface, REFIID riid, void **ppv)
{
    sub_stream_t *This = impl_from_IStream(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);
    *ppv = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
       IsEqualIID(riid, &IID_ISequentialStream) ||
       IsEqualIID(riid, &IID_IStream))
    {
        IStream_AddRef(iface);
        *ppv = iface;
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI sub_stream_AddRef(IStream *iface)
{
    sub_stream_t *This = impl_from_IStream(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI sub_stream_Release(IStream *iface)
{
    sub_stream_t *This = impl_from_IStream(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref)
    {
        IStream_Release(This->base);
        free(This);
    }
    return ref;
}

static HRESULT WINAPI sub_stream_Read(
        IStream* iface,
        void *pv,
        ULONG cb,
        ULONG *pcbRead)
{
    sub_stream_t *This = impl_from_IStream(iface);
    HRESULT hr;
    LARGE_INTEGER tmp_pos;

    TRACE("(%p, %ld, %p)\n", pv, cb, pcbRead);

    tmp_pos.QuadPart = This->pos.QuadPart + This->start.QuadPart;
    IStream_Seek(This->base, tmp_pos, STREAM_SEEK_SET, NULL);

    if(This->pos.QuadPart + cb > This->length.QuadPart)
        cb = This->length.QuadPart - This->pos.QuadPart;

    hr = IStream_Read(This->base, pv, cb, pcbRead);

    This->pos.QuadPart += *pcbRead;

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

    TRACE("(%08lx.%08lx, %lx, %p)\n", dlibMove.HighPart, dlibMove.LowPart, dwOrigin, plibNewPosition);

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

    TRACE("(%p)->(%p, %ld, %p, %p)\n", iface, pstm, cb.LowPart, pcbRead, pcbWritten);

    totalBytesRead.QuadPart = 0;
    totalBytesWritten.QuadPart = 0;

    while ( cb.QuadPart > 0 )
    {
        if ( cb.QuadPart >= sizeof(tmpBuffer) )
            copySize = sizeof(tmpBuffer);
        else
            copySize = cb.LowPart;

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
    FIXME("(%p)->(%p, %08lx)\n", This, pstatstg, grfStatFlag);
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
    This = malloc(sizeof(*This));
    if(!This) return E_OUTOFMEMORY;

    This->IStream_iface.lpVtbl = &sub_stream_vtbl;
    This->ref = 1;
    This->start = start;
    This->length = length;
    This->pos.QuadPart = 0;
    IStream_AddRef(stream);
    This->base = stream;

    *out = &This->IStream_iface;
    return S_OK;
}

static HRESULT get_stream_size(IStream *stream, ULARGE_INTEGER *size)
{
    STATSTG statstg = {NULL};
    LARGE_INTEGER zero;
    HRESULT hres;

    hres = IStream_Stat(stream, &statstg, STATFLAG_NONAME);
    if(SUCCEEDED(hres)) {
        *size = statstg.cbSize;
        return S_OK;
    }

    zero.QuadPart = 0;
    return IStream_Seek(stream, zero, STREAM_SEEK_END, size);
}

static inline MimeBody *impl_from_IMimeBody(IMimeBody *iface)
{
    return CONTAINING_RECORD(iface, MimeBody, IMimeBody_iface);
}

typedef struct propschema
{
    IMimePropertySchema IMimePropertySchema_iface;
    LONG ref;
} propschema;

static inline propschema *impl_from_IMimePropertySchema(IMimePropertySchema *iface)
{
    return CONTAINING_RECORD(iface, propschema, IMimePropertySchema_iface);
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
    char *buf = NULL, *new_buf;
    DWORD size = PARSER_BUF_SIZE, offset = 0, last_end = 0;
    HRESULT hr;
    BOOL done = FALSE;

    *ptr = NULL;

    do
    {
        char *end;
        DWORD read;

        if(buf) size *= 2;
        new_buf = realloc(buf, size + 1);
        if(!new_buf)
        {
            hr = E_OUTOFMEMORY;
            goto fail;
        }
        buf = new_buf;

        hr = IStream_Read(stm, buf + offset, size - offset, &read);
        if(FAILED(hr)) goto fail;

        offset += read;
        buf[offset] = '\0';

        if(read == 0) done = TRUE;

        while(!done && (end = strstr(buf + last_end, "\r\n")))
        {
            DWORD new_end = end - buf + 2;
            if(new_end - last_end == 2)
            {
                LARGE_INTEGER off;
                off.QuadPart = (LONGLONG)new_end - offset;
                IStream_Seek(stm, off, STREAM_SEEK_CUR, NULL);
                buf[new_end] = '\0';
                done = TRUE;
            }
            else
                last_end = new_end;
        }
    } while(!done);

    *ptr = buf;
    return S_OK;

fail:
    free(buf);
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
        if(!lstrcmpiA(*ptr, prop->name))
        {
            TRACE("%s: found match with default property id %ld\n", *ptr, prop->id);
            break;
        }
    }

    if(!prop->name)
    {
        property_list_entry_t *prop_entry;
        LIST_FOR_EACH_ENTRY(prop_entry, &body->new_props, property_list_entry_t, entry)
        {
            if(!lstrcmpiA(*ptr, prop_entry->prop.name))
            {
                TRACE("%s: found match with already added new property id %ld\n", *ptr, prop_entry->prop.id);
                prop = &prop_entry->prop;
                break;
            }
        }
        if(!prop->name)
        {
            prop_entry = malloc(sizeof(*prop_entry));
            prop_entry->prop.name = strdup(*ptr);
            prop_entry->prop.id = body->next_prop_id++;
            prop_entry->prop.flags = 0;
            prop_entry->prop.default_vt = VT_LPSTR;
            list_add_tail(&body->new_props, &prop_entry->entry);
            prop = &prop_entry->prop;
            TRACE("%s: allocating new prop id %ld\n", *ptr, prop_entry->prop.id);
        }
    }

    ret = malloc(sizeof(*ret));
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
    BOOL quoted = FALSE;
    char *ret, *cp;

    while(*str == ' ' || *str == '\t') str++;

    if(*str == '"')
    {
        quoted = TRUE;
        str++;
    }
    ret = strdup(str);
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

    name = malloc(cp - key + 1);
    memcpy(name, key, cp - key);
    name[cp - key] = '\0';

    value = cp + 1;

    param = malloc(sizeof(*param));
    param->name = name;
    param->value = unquote_string(value);
    list_add_tail(&header->params, &param->entry);
}

static void split_params(header_t *header, char *value)
{
    char *cp = value, *start = value;
    BOOL in_quotes = FALSE, done_value = FALSE;

    while(*cp)
    {
        if(!in_quotes && *cp == ';')
        {
            *cp = '\0';
            if(done_value) add_param(header, start);
            done_value = TRUE;
            start = cp + 1;
        }
        else if(*cp == '"')
            in_quotes = !in_quotes;
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
    value = CoTaskMemAlloc(len + 1);
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
    header->value.pszVal = value;

    *cur = end;
}

static void init_content_type(MimeBody *body, header_t *header)
{
    char *slash;
    DWORD len;

    slash = strchr(header->value.pszVal, '/');
    if(!slash)
    {
        WARN("malformed context type value\n");
        return;
    }
    len = slash - header->value.pszVal;
    body->content_pri_type = malloc(len + 1);
    memcpy(body->content_pri_type, header->value.pszVal, len);
    body->content_pri_type[len] = '\0';
    body->content_sub_type = strdup(slash + 1);
}

static void init_content_encoding(MimeBody *body, header_t *header)
{
    const char *encoding = header->value.pszVal;

    if(!stricmp(encoding, "base64"))
        body->encoding = IET_BASE64;
    else if(!stricmp(encoding, "quoted-printable"))
        body->encoding = IET_QP;
    else if(!stricmp(encoding, "7bit"))
        body->encoding = IET_7BIT;
    else if(!stricmp(encoding, "8bit"))
        body->encoding = IET_8BIT;
    else
        FIXME("unknown encoding %s\n", debugstr_a(encoding));
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

        switch(header->prop->id) {
        case PID_HDR_CNTTYPE:
            init_content_type(body, header);
            break;
        case PID_HDR_CNTXFER:
            init_content_encoding(body, header);
            break;
        }
    }

    free(header_buf);
    return hr;
}

static void empty_param_list(struct list *list)
{
    param_t *param, *cursor2;

    LIST_FOR_EACH_ENTRY_SAFE(param, cursor2, list, param_t, entry)
    {
        list_remove(&param->entry);
        free(param->name);
        free(param->value);
        free(param);
    }
}

static void free_header(header_t *header)
{
    list_remove(&header->entry);
    PropVariantClear(&header->value);
    empty_param_list(&header->params);
    free(header);
}

static void empty_header_list(struct list *list)
{
    header_t *header, *cursor2;

    LIST_FOR_EACH_ENTRY_SAFE(header, cursor2, list, header_t, entry)
    {
        free_header(header);
    }
}

static void empty_new_prop_list(struct list *list)
{
    property_list_entry_t *prop, *cursor2;

    LIST_FOR_EACH_ENTRY_SAFE(prop, cursor2, list, property_list_entry_t, entry)
    {
        list_remove(&prop->entry);
        free((char *)prop->prop.name);
        free(prop);
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
        if(ISPIDSTR(name))
        {
            if(STRTOPID(name) == header->prop->id)
            {
                *prop = header;
                return S_OK;
            }
        }
        else if(!lstrcmpiA(name, header->prop->name))
        {
            *prop = header;
            return S_OK;
        }
    }

    return MIME_E_NOT_FOUND;
}

static const property_t *find_default_prop(const char *name)
{
    const property_t *prop_def = NULL;

    for(prop_def = default_props; prop_def->name; prop_def++)
    {
        if(ISPIDSTR(name))
        {
            if(STRTOPID(name) == prop_def->id)
            {
                break;
            }
        }
        else if(!lstrcmpiA(name, prop_def->name))
        {
            break;
        }
    }

    if(prop_def->id)
       TRACE("%s: found match with default property id %ld\n", prop_def->name, prop_def->id);
    else
       prop_def = NULL;

    return prop_def;
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

static ULONG WINAPI MimeBody_AddRef(IMimeBody *iface)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI MimeBody_Release(IMimeBody *iface)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref)
    {
        empty_header_list(&This->headers);
        empty_new_prop_list(&This->new_props);

        free(This->content_pri_type);
        free(This->content_sub_type);

        release_data(&This->data_iid, This->data);

        free(This);
    }

    return ref;
}

static HRESULT WINAPI MimeBody_GetClassID(
                                 IMimeBody* iface,
                                 CLSID* pClassID)
{
    MimeBody *This = impl_from_IMimeBody(iface);

    TRACE("(%p)->(%p)\n", This, pClassID);

    if(!pClassID)
        return E_INVALIDARG;

    *pClassID = IID_IMimeBody;
    return S_OK;
}

static HRESULT WINAPI MimeBody_IsDirty(
                              IMimeBody* iface)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    FIXME("(%p)->() stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_Load(IMimeBody *iface, IStream *pStm)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    TRACE("(%p)->(%p)\n", This, pStm);
    return parse_headers(This, pStm);
}

static HRESULT WINAPI MimeBody_Save(IMimeBody *iface, IStream *pStm, BOOL fClearDirty)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    FIXME("(%p)->(%p, %d)\n", This, pStm, fClearDirty);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_GetSizeMax(
                                 IMimeBody* iface,
                                 ULARGE_INTEGER* pcbSize)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    FIXME("(%p)->(%p) stub\n", This, pcbSize);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_InitNew(
                              IMimeBody* iface)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    TRACE("(%p)->()\n", This);
    return S_OK;
}

static HRESULT WINAPI MimeBody_GetPropInfo(
                                  IMimeBody* iface,
                                  LPCSTR pszName,
                                  LPMIMEPROPINFO pInfo)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    header_t *header;
    HRESULT hr;
    DWORD supported = PIM_PROPID | PIM_VTDEFAULT;

    TRACE("(%p)->(%s, %p) semi-stub\n", This, debugstr_a(pszName), pInfo);

    if(!pszName || !pInfo)
        return E_INVALIDARG;

    TRACE("mask 0x%04lx\n", pInfo->dwMask);

    if(pInfo->dwMask & ~supported)
         FIXME("Unsupported mask flags 0x%04lx\n", pInfo->dwMask & ~supported);

    hr = find_prop(This, pszName, &header);
    if(hr == S_OK)
    {
        if(pInfo->dwMask & PIM_CHARSET)
            pInfo->hCharset = 0;
        if(pInfo->dwMask & PIM_FLAGS)
            pInfo->dwFlags = 0x00000000;
        if(pInfo->dwMask & PIM_ROWNUMBER)
            pInfo->dwRowNumber = 0;
        if(pInfo->dwMask & PIM_ENCODINGTYPE)
            pInfo->ietEncoding = 0;
        if(pInfo->dwMask & PIM_VALUES)
            pInfo->cValues = 0;
        if(pInfo->dwMask & PIM_PROPID)
            pInfo->dwPropId = header->prop->id;
        if(pInfo->dwMask & PIM_VTDEFAULT)
            pInfo->vtDefault = header->prop->default_vt;
        if(pInfo->dwMask & PIM_VTCURRENT)
            pInfo->vtCurrent = 0;
    }

    return hr;
}

static HRESULT WINAPI MimeBody_SetPropInfo(
                                  IMimeBody* iface,
                                  LPCSTR pszName,
                                  LPCMIMEPROPINFO pInfo)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    FIXME("(%p)->(%s, %p) stub\n", This, debugstr_a(pszName), pInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_GetProp(
                              IMimeBody* iface,
                              LPCSTR pszName,
                              DWORD dwFlags,
                              LPPROPVARIANT pValue)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    header_t *header;
    HRESULT hr;

    TRACE("(%p)->(%s, 0x%lx, %p)\n", This, debugstr_a(pszName), dwFlags, pValue);

    if(!pszName || !pValue)
        return E_INVALIDARG;

    if(!ISPIDSTR(pszName) && !lstrcmpiA(pszName, "att:pri-content-type"))
    {
        PropVariantClear(pValue);
        pValue->vt = VT_LPSTR;
        pValue->pszVal = CoTaskMemAlloc(strlen(This->content_pri_type) + 1);
        strcpy(pValue->pszVal, This->content_pri_type);
        return S_OK;
    }

    hr = find_prop(This, pszName, &header);
    if(hr == S_OK)
    {
        TRACE("type %d->%d\n", header->value.vt, pValue->vt);

        hr = PropVariantChangeType(pValue, &header->value, 0,  pValue->vt);
        if(FAILED(hr))
            FIXME("Conversion not currently supported (%d->%d)\n", header->value.vt, pValue->vt);
    }

    return hr;
}

static HRESULT WINAPI MimeBody_SetProp(
                              IMimeBody* iface,
                              LPCSTR pszName,
                              DWORD dwFlags,
                              LPCPROPVARIANT pValue)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    header_t *header;
    HRESULT hr;

    TRACE("(%p)->(%s, 0x%lx, %p)\n", This, debugstr_a(pszName), dwFlags, pValue);

    if(!pszName || !pValue)
        return E_INVALIDARG;

    hr = find_prop(This, pszName, &header);
    if(hr != S_OK)
    {
        property_list_entry_t *prop_entry;
        const property_t *prop = NULL;

        LIST_FOR_EACH_ENTRY(prop_entry, &This->new_props, property_list_entry_t, entry)
        {
            if(ISPIDSTR(pszName))
            {
                if(STRTOPID(pszName) == prop_entry->prop.id)
                {
                    TRACE("Found match with already added new property id %ld\n", prop_entry->prop.id);
                    prop = &prop_entry->prop;
                    break;
                }
            }
            else if(!lstrcmpiA(pszName, prop_entry->prop.name))
            {
                TRACE("Found match with already added new property id %ld\n", prop_entry->prop.id);
                prop = &prop_entry->prop;
                break;
            }
        }

        header = malloc(sizeof(*header));
        if(!header)
            return E_OUTOFMEMORY;

        if(!prop)
        {
            const property_t *prop_def = NULL;
            prop_entry = malloc(sizeof(*prop_entry));
            if(!prop_entry)
            {
                free(header);
                return E_OUTOFMEMORY;
            }

            prop_def = find_default_prop(pszName);
            if(prop_def)
            {
                prop_entry->prop.name = strdup(prop_def->name);
                prop_entry->prop.id =  prop_def->id;
            }
            else
            {
                if(ISPIDSTR(pszName))
                {
                    free(prop_entry);
                    free(header);
                    return MIME_E_NOT_FOUND;
                }

                prop_entry->prop.name = strdup(pszName);
                prop_entry->prop.id = This->next_prop_id++;
            }

            prop_entry->prop.flags = 0;
            prop_entry->prop.default_vt = pValue->vt;
            list_add_tail(&This->new_props, &prop_entry->entry);
            prop = &prop_entry->prop;
            TRACE("Allocating new prop id %ld\n", prop_entry->prop.id);
        }

        header->prop = prop;
        PropVariantInit(&header->value);
        list_init(&header->params);
        list_add_tail(&This->headers, &header->entry);
    }

    PropVariantCopy(&header->value, pValue);

    return S_OK;
}

static HRESULT WINAPI MimeBody_AppendProp(
                                 IMimeBody* iface,
                                 LPCSTR pszName,
                                 DWORD dwFlags,
                                 LPPROPVARIANT pValue)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    FIXME("(%p)->(%s, 0x%lx, %p) stub\n", This, debugstr_a(pszName), dwFlags, pValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_DeleteProp(
                                 IMimeBody* iface,
                                 LPCSTR pszName)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    header_t *cursor;
    BOOL found;

    TRACE("(%p)->(%s) stub\n", This, debugstr_a(pszName));

    LIST_FOR_EACH_ENTRY(cursor, &This->headers, header_t, entry)
    {
        if(ISPIDSTR(pszName))
            found = STRTOPID(pszName) == cursor->prop->id;
        else
            found = !lstrcmpiA(pszName, cursor->prop->name);

        if(found)
        {
             free_header(cursor);
             return S_OK;
        }
    }

    return MIME_E_NOT_FOUND;
}

static HRESULT WINAPI MimeBody_CopyProps(
                                IMimeBody* iface,
                                ULONG cNames,
                                LPCSTR* prgszName,
                                IMimePropertySet* pPropertySet)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    FIXME("(%p)->(%ld, %p, %p) stub\n", This, cNames, prgszName, pPropertySet);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_MoveProps(
                                IMimeBody* iface,
                                ULONG cNames,
                                LPCSTR* prgszName,
                                IMimePropertySet* pPropertySet)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    FIXME("(%p)->(%ld, %p, %p) stub\n", This, cNames, prgszName, pPropertySet);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_DeleteExcept(
                                   IMimeBody* iface,
                                   ULONG cNames,
                                   LPCSTR* prgszName)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    FIXME("(%p)->(%ld, %p) stub\n", This, cNames, prgszName);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_QueryProp(
                                IMimeBody* iface,
                                LPCSTR pszName,
                                LPCSTR pszCriteria,
                                boolean fSubString,
                                boolean fCaseSensitive)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    FIXME("(%p)->(%s, %s, %d, %d) stub\n", This, debugstr_a(pszName), debugstr_a(pszCriteria), fSubString, fCaseSensitive);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_GetCharset(
                                 IMimeBody* iface,
                                 LPHCHARSET phCharset)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    FIXME("(%p)->(%p) stub\n", This, phCharset);
    *phCharset = NULL;
    return S_OK;
}

static HRESULT WINAPI MimeBody_SetCharset(
                                 IMimeBody* iface,
                                 HCHARSET hCharset,
                                 CSETAPPLYTYPE applytype)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    FIXME("(%p)->(%p, %d) stub\n", This, hCharset, applytype);
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
        if(lstrcmpiA(pri, pszPriType)) return S_FALSE;
    }

    if(pszSubType)
    {
        const char *sub = This->content_sub_type;
        if(!sub) sub = "plain";
        if(lstrcmpiA(sub, pszSubType)) return S_FALSE;
    }

    return S_OK;
}

static HRESULT WINAPI MimeBody_BindToObject(
                                   IMimeBody* iface,
                                   REFIID riid,
                                   void** ppvObject)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    FIXME("(%p)->(%s, %p) stub\n", This, debugstr_guid(riid), ppvObject);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_Clone(
                            IMimeBody* iface,
                            IMimePropertySet** ppPropertySet)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    FIXME("(%p)->(%p) stub\n", This, ppPropertySet);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_SetOption(
                                IMimeBody* iface,
                                const TYPEDID oid,
                                LPCPROPVARIANT pValue)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    HRESULT hr = E_NOTIMPL;
    TRACE("(%p)->(%08lx, %p)\n", This, oid, pValue);

    if(pValue->vt != TYPEDID_TYPE(oid))
    {
        WARN("Called with vartype %04x and oid %08lx\n", pValue->vt, oid);
        return E_INVALIDARG;
    }

    switch(oid)
    {
    case OID_SECURITY_HWND_OWNER:
        FIXME("OID_SECURITY_HWND_OWNER (value %08lx): ignoring\n", pValue->ulVal);
        hr = S_OK;
        break;
    case OID_TRANSMIT_BODY_ENCODING:
        FIXME("OID_TRANSMIT_BODY_ENCODING (value %08lx): ignoring\n", pValue->ulVal);
        hr = S_OK;
        break;
    default:
        FIXME("Unhandled oid %08lx\n", oid);
    }

    return hr;
}

static HRESULT WINAPI MimeBody_GetOption(
                                IMimeBody* iface,
                                const TYPEDID oid,
                                LPPROPVARIANT pValue)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    FIXME("(%p)->(%08lx, %p): stub\n", This, oid, pValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_EnumProps(
                                IMimeBody* iface,
                                DWORD dwFlags,
                                IMimeEnumProperties** ppEnum)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    FIXME("(%p)->(0x%lx, %p) stub\n", This, dwFlags, ppEnum);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_IsType(
                             IMimeBody* iface,
                             IMSGBODYTYPE bodytype)
{
    MimeBody *This = impl_from_IMimeBody(iface);

    TRACE("(%p)->(%d)\n", This, bodytype);
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
    MimeBody *This = impl_from_IMimeBody(iface);
    FIXME("(%p)->(%s) stub\n", This, debugstr_a(pszDisplay));
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_GetDisplayName(
                                     IMimeBody* iface,
                                     LPSTR* ppszDisplay)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    FIXME("(%p)->(%p) stub\n", This, ppszDisplay);
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
    MimeBody *This = impl_from_IMimeBody(iface);
    FIXME("(%p)->(%d, %p) stub\n", This, ietEncoding, pcbSize);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_GetDataHere(
                                  IMimeBody* iface,
                                  ENCODINGTYPE ietEncoding,
                                  IStream* pStream)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    FIXME("(%p)->(%d, %p) stub\n", This, ietEncoding, pStream);
    return E_NOTIMPL;
}

static const signed char base64_decode_table[] =
{
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x00 */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x10 */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, /* 0x20 */
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, /* 0x30 */
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, /* 0x40 */
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, /* 0x50 */
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, /* 0x60 */
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1  /* 0x70 */
};

static HRESULT decode_base64(IStream *input, IStream **ret_stream)
{
    const unsigned char *ptr, *end;
    unsigned char buf[1024];
    LARGE_INTEGER pos;
    unsigned char *ret;
    unsigned char in[4];
    IStream *output;
    DWORD size;
    int n = 0;
    HRESULT hres;

    pos.QuadPart = 0;
    hres = IStream_Seek(input, pos, STREAM_SEEK_SET, NULL);
    if(FAILED(hres))
        return hres;

    hres = CreateStreamOnHGlobal(NULL, TRUE, &output);
    if(FAILED(hres))
        return hres;

    while(1) {
        hres = IStream_Read(input, buf, sizeof(buf), &size);
        if(FAILED(hres) || !size)
            break;

        ptr = ret = buf;
        end = buf + size;

        while(1) {
            /* skip invalid chars */
            while(ptr < end && (*ptr >= ARRAY_SIZE(base64_decode_table)
                                || base64_decode_table[*ptr] == -1))
                ptr++;
            if(ptr == end)
                break;

            in[n++] = base64_decode_table[*ptr++];
            switch(n) {
            case 2:
                *ret++ = in[0] << 2 | in[1] >> 4;
                continue;
            case 3:
                *ret++ = in[1] << 4 | in[2] >> 2;
                continue;
            case 4:
                *ret++ = ((in[2] << 6) & 0xc0) | in[3];
                n = 0;
            }
        }

        if(ret > buf) {
            hres = IStream_Write(output, buf, ret - buf, NULL);
            if(FAILED(hres))
                break;
        }
    }

    if(SUCCEEDED(hres))
        hres = IStream_Seek(output, pos, STREAM_SEEK_SET, NULL);
    if(FAILED(hres)) {
        IStream_Release(output);
        return hres;
    }

    *ret_stream = output;
    return S_OK;
}

static int hex_digit(char c)
{
    if('0' <= c && c <= '9')
        return c - '0';
    if('A' <= c && c <= 'F')
        return c - 'A' + 10;
    if('a' <= c && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

static HRESULT decode_qp(IStream *input, IStream **ret_stream)
{
    const unsigned char *ptr, *end;
    unsigned char *ret, prev = 0;
    unsigned char buf[1024];
    LARGE_INTEGER pos;
    IStream *output;
    DWORD size;
    int n = -1;
    HRESULT hres;

    pos.QuadPart = 0;
    hres = IStream_Seek(input, pos, STREAM_SEEK_SET, NULL);
    if(FAILED(hres))
        return hres;

    hres = CreateStreamOnHGlobal(NULL, TRUE, &output);
    if(FAILED(hres))
        return hres;

    while(1) {
        hres = IStream_Read(input, buf, sizeof(buf), &size);
        if(FAILED(hres) || !size)
            break;

        ptr = ret = buf;
        end = buf + size;

        while(ptr < end) {
            unsigned char byte = *ptr++;

            switch(n) {
            case -1:
                if(byte == '=')
                    n = 0;
                else
                    *ret++ = byte;
                continue;
            case 0:
                prev = byte;
                n = 1;
                continue;
            case 1:
                if(prev != '\r' || byte != '\n') {
                    int h1 = hex_digit(prev), h2 = hex_digit(byte);
                    if(h1 != -1 && h2 != -1)
                        *ret++ = (h1 << 4) | h2;
                    else
                        *ret++ = '=';
                }
                n = -1;
                continue;
            }
        }

        if(ret > buf) {
            hres = IStream_Write(output, buf, ret - buf, NULL);
            if(FAILED(hres))
                break;
        }
    }

    if(SUCCEEDED(hres))
        hres = IStream_Seek(output, pos, STREAM_SEEK_SET, NULL);
    if(FAILED(hres)) {
        IStream_Release(output);
        return hres;
    }

    *ret_stream = output;
    return S_OK;
}

static HRESULT WINAPI MimeBody_GetData(
                              IMimeBody* iface,
                              ENCODINGTYPE ietEncoding,
                              IStream** ppStream)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    ULARGE_INTEGER start, size;
    HRESULT hres;

    TRACE("(%p)->(%d %p)\n", This, ietEncoding, ppStream);

    if(This->encoding != ietEncoding) {
        switch(This->encoding) {
        case IET_BASE64:
            hres = decode_base64(This->data, ppStream);
            break;
        case IET_QP:
            hres = decode_qp(This->data, ppStream);
            break;
        default:
            FIXME("Decoding %d is not supported.\n", This->encoding);
            hres = S_FALSE;
        }
        if(ietEncoding != IET_BINARY)
            FIXME("Encoding %d is not supported.\n", ietEncoding);
        if(hres != S_FALSE)
            return hres;
    }

    start.QuadPart = 0;
    hres = get_stream_size(This->data, &size);
    if(SUCCEEDED(hres))
        hres = create_sub_stream(This->data, start, size, ppStream);
    return hres;
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
        release_data(&This->data_iid, This->data);

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
    MimeBody *This = impl_from_IMimeBody(iface);
    FIXME("(%p)->() stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_CopyTo(
                             IMimeBody* iface,
                             IMimeBody* pBody)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    FIXME("(%p)->(%p) stub\n", This, pBody);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_GetTransmitInfo(
                                      IMimeBody* iface,
                                      LPTRANSMITINFO pTransmitInfo)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    FIXME("(%p)->(%p) stub\n", This, pTransmitInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_SaveToFile(
                                 IMimeBody* iface,
                                 ENCODINGTYPE ietEncoding,
                                 LPCSTR pszFilePath)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    FIXME("(%p)->(%d, %s) stub\n", This, ietEncoding, debugstr_a(pszFilePath));
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeBody_GetHandle(
                                IMimeBody* iface,
                                LPHBODY phBody)
{
    MimeBody *This = impl_from_IMimeBody(iface);
    TRACE("(%p)->(%p)\n", iface, phBody);

    if(!phBody)
        return E_INVALIDARG;

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
    TRACE("setting offsets to %ld, %ld, %ld, %ld\n", offsets->cbBoundaryStart,
          offsets->cbHeaderStart, offsets->cbBodyStart, offsets->cbBodyEnd);

    body->body_offsets = *offsets;
    return S_OK;
}

#define FIRST_CUSTOM_PROP_ID 0x100

static MimeBody *mimebody_create(void)
{
    MimeBody *This;
    BODYOFFSETS body_offsets;

    This = malloc(sizeof(*This));
    if (!This)
        return NULL;

    This->IMimeBody_iface.lpVtbl = &body_vtbl;
    This->ref = 1;
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

    return This;
}

HRESULT MimeBody_create(IUnknown *outer, void **ppv)
{
    MimeBody *mb;

    if(outer)
        return CLASS_E_NOAGGREGATION;

    if ((mb = mimebody_create()))
    {
        *ppv = &mb->IMimeBody_iface;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_OUTOFMEMORY;
    }
}

typedef struct body_t
{
    struct list entry;
    DWORD index;
    MimeBody *mime_body;

    struct body_t *parent;
    struct list children;
} body_t;

typedef struct MimeMessage
{
    IMimeMessage IMimeMessage_iface;
    LONG ref;
    IStream *stream;

    struct list body_tree;
    DWORD next_index;
} MimeMessage;

static inline MimeMessage *impl_from_IMimeMessage(IMimeMessage *iface)
{
    return CONTAINING_RECORD(iface, MimeMessage, IMimeMessage_iface);
}

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
        IMimeMessage_AddRef(iface);
        return S_OK;
    }

    FIXME("no interface for %s\n", debugstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI MimeMessage_AddRef(IMimeMessage *iface)
{
    MimeMessage *This = impl_from_IMimeMessage(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static void empty_body_list(struct list *list)
{
    body_t *body, *cursor2;
    LIST_FOR_EACH_ENTRY_SAFE(body, cursor2, list, body_t, entry)
    {
        empty_body_list(&body->children);
        list_remove(&body->entry);
        IMimeBody_Release(&body->mime_body->IMimeBody_iface);
        free(body);
    }
}

static ULONG WINAPI MimeMessage_Release(IMimeMessage *iface)
{
    MimeMessage *This = impl_from_IMimeMessage(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref)
    {
        empty_body_list(&This->body_tree);

        if(This->stream) IStream_Release(This->stream);
        free(This);
    }

    return ref;
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

static body_t *new_body_entry(MimeBody *mime_body, DWORD index, body_t *parent)
{
    body_t *body = malloc(sizeof(*body));
    if(body)
    {
        body->mime_body = mime_body;
        body->index = index;
        list_init(&body->children);
        body->parent = parent;

        mime_body->handle = UlongToHandle(body->index);
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
    DWORD read, boundary_start;
    int boundary_len = strlen(boundary);
    char *buf, *ptr, *overlap;
    DWORD start = 0, overlap_no;
    offset_entry_t *cur_body = NULL;
    BOOL is_first_line = TRUE;
    ULARGE_INTEGER cur;
    LARGE_INTEGER zero;

    list_init(body_offsets);

    overlap_no = boundary_len + 5;

    overlap = buf = malloc(overlap_no + PARSER_BUF_SIZE + 1);

    zero.QuadPart = 0;
    hr = IStream_Seek(stm, zero, STREAM_SEEK_CUR, &cur);
    start = cur.LowPart;

    do {
        hr = IStream_Read(stm, overlap, PARSER_BUF_SIZE, &read);
        if(FAILED(hr)) goto end;
        if(read == 0) break;
        overlap[read] = '\0';

        ptr = buf;
        while(1) {
            if(is_first_line) {
                is_first_line = FALSE;
            }else {
                ptr = strstr(ptr, "\r\n");
                if(!ptr)
                    break;
                ptr += 2;
            }

            boundary_start = start + ptr - buf;

            if(*ptr == '-' && *(ptr + 1) == '-' && !memcmp(ptr + 2, boundary, boundary_len)) {
                ptr += boundary_len + 2;

                if(*ptr == '\r' && *(ptr + 1) == '\n')
                {
                    ptr += 2;
                    if(cur_body)
                    {
                        cur_body->offsets.cbBodyEnd = boundary_start - 2;
                        list_add_tail(body_offsets, &cur_body->entry);
                    }
                    cur_body = malloc(sizeof(*cur_body));
                    cur_body->offsets.cbBoundaryStart = boundary_start;
                    cur_body->offsets.cbHeaderStart = start + ptr - buf;
                }
                else if(*ptr == '-' && *(ptr + 1) == '-')
                {
                    if(cur_body)
                    {
                        cur_body->offsets.cbBodyEnd = boundary_start - 2;
                        list_add_tail(body_offsets, &cur_body->entry);
                        goto end;
                    }
                }
            }
        }

        if(overlap == buf) /* 1st iteration */
        {
            memmove(buf, buf + PARSER_BUF_SIZE - overlap_no, overlap_no);
            overlap = buf + overlap_no;
            start += read - overlap_no;
        }
        else
        {
            memmove(buf, buf + PARSER_BUF_SIZE, overlap_no);
            start += read;
        }
    } while(1);

end:
    free(buf);
    return hr;
}

static body_t *create_sub_body(MimeMessage *msg, IStream *pStm, BODYOFFSETS *offset, body_t *parent)
{
    ULARGE_INTEGER start, length;
    MimeBody *mime_body;
    HRESULT hr;
    body_t *body;
    LARGE_INTEGER pos;

    pos.QuadPart = offset->cbHeaderStart;
    IStream_Seek(pStm, pos, STREAM_SEEK_SET, NULL);

    mime_body = mimebody_create();
    IMimeBody_Load(&mime_body->IMimeBody_iface, pStm);

    pos.QuadPart = 0;
    hr = IStream_Seek(pStm, pos, STREAM_SEEK_CUR, &start);
    offset->cbBodyStart = start.QuadPart;
    if (parent) MimeBody_set_offsets(mime_body, offset);

    length.QuadPart = offset->cbBodyEnd - offset->cbBodyStart;
    create_sub_stream(pStm, start, length, (IStream**)&mime_body->data);
    mime_body->data_iid = IID_IStream;

    body = new_body_entry(mime_body, msg->next_index++, parent);

    if(IMimeBody_IsContentType(&mime_body->IMimeBody_iface, "multipart", NULL) == S_OK)
    {
        MIMEPARAMINFO *param_info;
        ULONG count, i;
        IMimeAllocator *alloc;

        hr = IMimeBody_GetParameters(&mime_body->IMimeBody_iface, "Content-Type", &count,
                &param_info);
        if(hr != S_OK || count == 0) return body;

        MimeOleGetAllocator(&alloc);

        for(i = 0; i < count; i++)
        {
            if(!lstrcmpiA(param_info[i].pszName, "boundary"))
            {
                struct list offset_list;
                offset_entry_t *cur, *cursor2;
                hr = create_body_offset_list(pStm, param_info[i].pszData, &offset_list);
                LIST_FOR_EACH_ENTRY_SAFE(cur, cursor2, &offset_list, offset_entry_t, entry)
                {
                    body_t *sub_body;

                    sub_body = create_sub_body(msg, pStm, &cur->offsets, body);
                    list_add_tail(&body->children, &sub_body->entry);
                    list_remove(&cur->entry);
                    free(cur);
                }
                break;
            }
        }
        IMimeAllocator_FreeParamInfoArray(alloc, count, param_info, TRUE);
        IMimeAllocator_Release(alloc);
    }
    return body;
}

static HRESULT WINAPI MimeMessage_Load(IMimeMessage *iface, IStream *pStm)
{
    MimeMessage *This = impl_from_IMimeMessage(iface);
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

    empty_body_list(&This->body_tree);

    IStream_AddRef(pStm);
    This->stream = pStm;
    offsets.cbBoundaryStart = offsets.cbHeaderStart = 0;
    offsets.cbBodyStart = offsets.cbBodyEnd = 0;

    root_body = create_sub_body(This, pStm, &offsets, NULL);

    zero.QuadPart = 0;
    IStream_Seek(pStm, zero, STREAM_SEEK_END, &cur);
    offsets.cbBodyEnd = cur.LowPart;
    MimeBody_set_offsets(root_body->mime_body, &offsets);

    list_add_head(&This->body_tree, &root_body->entry);

    return S_OK;
}

static HRESULT WINAPI MimeMessage_Save(IMimeMessage *iface, IStream *pStm, BOOL fClearDirty)
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
static HRESULT WINAPI MimeMessage_GetMessageSource(IMimeMessage *iface, IStream **ppStream,
        DWORD dwFlags)
{
    MimeMessage *This = impl_from_IMimeMessage(iface);

    FIXME("(%p)->(%p, 0x%lx)\n", iface, ppStream, dwFlags);

    IStream_AddRef(This->stream);
    *ppStream = This->stream;
    return S_OK;
}

static HRESULT WINAPI MimeMessage_GetMessageSize(
    IMimeMessage *iface,
    ULONG *pcbSize,
    DWORD dwFlags)
{
    FIXME("(%p)->(%p, 0x%lx)\n", iface, pcbSize, dwFlags);
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
    FIXME("(%p)->(%p, 0x%lx)\n", iface, pStream, dwFlags);
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
    FIXME("(%p)->(0x%lx)\n", iface, dwFlags);
    return S_OK;
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
        if(cur->index == HandleToUlong(hbody))
        {
            *body = cur;
            return S_OK;
        }
        hr = find_body(&cur->children, hbody, body);
        if(hr == S_OK) return S_OK;
    }
    return S_FALSE;
}

static HRESULT WINAPI MimeMessage_BindToObject(IMimeMessage *iface, const HBODY hBody, REFIID riid,
        void **ppvObject)
{
    MimeMessage *This = impl_from_IMimeMessage(iface);
    HRESULT hr;
    body_t *body;

    TRACE("(%p)->(%p, %s, %p)\n", iface, hBody, debugstr_guid(riid), ppvObject);

    hr = find_body(&This->body_tree, hBody, &body);

    if(hr != S_OK) return hr;

    if(IsEqualIID(riid, &IID_IMimeBody))
    {
        IMimeBody_AddRef(&body->mime_body->IMimeBody_iface);
        *ppvObject = &body->mime_body->IMimeBody_iface;
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
    FIXME("(%p)->(%p, 0x%lx, %p)\n", iface, hBody, dwFlags, pStream);
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
            if(body->parent)
                *out = body->parent;
            else
                hr = MIME_E_NOT_FOUND;
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

static HRESULT WINAPI MimeMessage_GetBody(IMimeMessage *iface, BODYLOCATION location, HBODY hPivot,
        HBODY *phBody)
{
    MimeMessage *This = impl_from_IMimeMessage(iface);
    body_t *body;
    HRESULT hr;

    TRACE("(%p)->(%d, %p, %p)\n", iface, location, hPivot, phBody);

    if(!phBody)
        return E_INVALIDARG;

    *phBody = NULL;

    hr = get_body(This, location, hPivot, &body);

    if(hr == S_OK) *phBody = UlongToHandle(body->index);

    return hr;
}

static HRESULT WINAPI MimeMessage_DeleteBody(
    IMimeMessage *iface,
    HBODY hBody,
    DWORD dwFlags)
{
    FIXME("(%p)->(%p, %08lx)\n", iface, hBody, dwFlags);
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

static HRESULT WINAPI MimeMessage_CountBodies(IMimeMessage *iface, HBODY hParent, boolean fRecurse,
        ULONG *pcBodies)
{
    HRESULT hr;
    MimeMessage *This = impl_from_IMimeMessage(iface);
    body_t *body;

    TRACE("(%p)->(%p, %s, %p)\n", iface, hParent, fRecurse ? "TRUE" : "FALSE", pcBodies);

    hr = find_body(&This->body_tree, hParent, &body);
    if(hr != S_OK) return hr;

    *pcBodies = 1;
    count_children(body, fRecurse, pcBodies);

    return S_OK;
}

static HRESULT find_next(MimeMessage *This, body_t *body, FINDBODY *find, HBODY *out)
{
    struct list *ptr;
    HBODY next;

    for (;;)
    {
        if (!body) ptr = list_head( &This->body_tree );
        else
        {
            ptr = list_head( &body->children );
            while (!ptr)
            {
                if (!body->parent) return MIME_E_NOT_FOUND;
                if (!(ptr = list_next( &body->parent->children, &body->entry ))) body = body->parent;
            }
        }

        body = LIST_ENTRY( ptr, body_t, entry );
        next = UlongToHandle( body->index );
        find->dwReserved = body->index;
        if (IMimeBody_IsContentType(&body->mime_body->IMimeBody_iface, find->pszPriType,
                    find->pszSubType) == S_OK)
        {
            *out = next;
            return S_OK;
        }
    }
    return MIME_E_NOT_FOUND;
}

static HRESULT WINAPI MimeMessage_FindFirst(IMimeMessage *iface, FINDBODY *pFindBody, HBODY *phBody)
{
    MimeMessage *This = impl_from_IMimeMessage(iface);

    TRACE("(%p)->(%p, %p)\n", iface, pFindBody, phBody);

    pFindBody->dwReserved = 0;
    return find_next(This, NULL, pFindBody, phBody);
}

static HRESULT WINAPI MimeMessage_FindNext(IMimeMessage *iface, FINDBODY *pFindBody, HBODY *phBody)
{
    MimeMessage *This = impl_from_IMimeMessage(iface);
    body_t *body;
    HRESULT hr;

    TRACE("(%p)->(%p, %p)\n", iface, pFindBody, phBody);

    hr = find_body( &This->body_tree, UlongToHandle( pFindBody->dwReserved ), &body );
    if (hr != S_OK) return MIME_E_NOT_FOUND;
    return find_next(This, body, pFindBody, phBody);
}

static HRESULT WINAPI MimeMessage_ResolveURL(
    IMimeMessage *iface,
    HBODY hRelated,
    LPCSTR pszBase,
    LPCSTR pszURL,
    DWORD dwFlags,
    LPHBODY phBody)
{
    FIXME("(%p)->(%p, %s, %s, 0x%lx, %p)\n", iface, hRelated, pszBase, pszURL, dwFlags, phBody);
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
    TRACE("(%p)->(%p, %s, %s)\n", iface, hBody, debugstr_a(pszPriType),
          debugstr_a(pszSubType));

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

    TRACE("(%p)->(%p, %s, 0x%lx, %p)\n", iface, hBody, pszName, dwFlags, pValue);

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
    FIXME("(%p)->(%p, %s, 0x%lx, %p)\n", iface, hBody, pszName, dwFlags, pValue);
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
    HRESULT hr = S_OK;
    TRACE("(%p)->(%08lx, %p)\n", iface, oid, pValue);

    /* Message ID is checked before type.
     *  OID 0x4D -> 0x56 and 0x58 aren't defined but will filtered out later.
     */
    if(TYPEDID_ID(oid) < TYPEDID_ID(OID_ALLOW_8BIT_HEADER) || TYPEDID_ID(oid) > TYPEDID_ID(OID_SECURITY_2KEY_CERT_BAG_64))
    {
        WARN("oid (%08lx) out of range\n", oid);
        return MIME_E_INVALID_OPTION_ID;
    }

    if(pValue->vt != TYPEDID_TYPE(oid))
    {
        WARN("Called with vartype %04x and oid %08lx\n", pValue->vt, oid);
        return S_OK;
    }

    switch(oid)
    {
    case OID_HIDE_TNEF_ATTACHMENTS:
        FIXME("OID_HIDE_TNEF_ATTACHMENTS (value %d): ignoring\n", pValue->boolVal);
        break;
    case OID_SHOW_MACBINARY:
        FIXME("OID_SHOW_MACBINARY (value %d): ignoring\n", pValue->boolVal);
        break;
    case OID_SAVEBODY_KEEPBOUNDARY:
        FIXME("OID_SAVEBODY_KEEPBOUNDARY (value %d): ignoring\n", pValue->boolVal);
        break;
    case OID_CLEANUP_TREE_ON_SAVE:
        FIXME("OID_CLEANUP_TREE_ON_SAVE (value %d): ignoring\n", pValue->boolVal);
        break;
    default:
        FIXME("Unhandled oid %08lx\n", oid);
        hr = MIME_E_INVALID_OPTION_ID;
    }

    return hr;
}

static HRESULT WINAPI MimeMessage_GetOption(
    IMimeMessage *iface,
    const TYPEDID oid,
    LPPROPVARIANT pValue)
{
    FIXME("(%p)->(%08lx, %p)\n", iface, oid, pValue);
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
    FIXME("(%p)->(%s, 0x%lx, %p)\n", iface, pszName, dwFlags, pValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_SetProp(
    IMimeMessage *iface,
    LPCSTR pszName,
    DWORD dwFlags,
    LPCPROPVARIANT pValue)
{
    FIXME("(%p)->(%s, 0x%lx, %p)\n", iface, pszName, dwFlags, pValue);
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

    TRACE("(%p)->(%ld, %d, %p, %p)\n", iface, dwTxtType, ietEncoding, pStream, phBody);

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
        TRACE("not found hr %08lx\n", hr);
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
    FIXME("(%p)->(%ld, %d, %p, %p, %p)\n", iface, dwTxtType, ietEncoding, hAlternative, pStream, phBody);
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
    FIXME("(%p)->(%s, %s, 0x%lx, %p, %p, %p)\n", iface, pszBase, pszURL, dwFlags, pstmURL, ppszCIDURL, phBody);
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
        TRACE("IsCT rets %08lx %ld\n", hr, *pcAttach);
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
    FIXME("(%p)->(%ld, %ld, %p)\n", iface, dwAdrTypes, dwProps, pList);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_GetAddressFormat(
    IMimeMessage *iface,
    DWORD dwAdrTypes,
    ADDRESSFORMAT format,
    LPSTR *ppszFormat)
{
    FIXME("(%p)->(%ld, %d, %p)\n", iface, dwAdrTypes, format, ppszFormat);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_EnumAddressTypes(
    IMimeMessage *iface,
    DWORD dwAdrTypes,
    DWORD dwProps,
    IMimeEnumAddressTypes **ppEnum)
{
    FIXME("(%p)->(%ld, %ld, %p)\n", iface, dwAdrTypes, dwProps, ppEnum);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeMessage_SplitMessage(
    IMimeMessage *iface,
    ULONG cbMaxPart,
    IMimeMessageParts **ppParts)
{
    FIXME("(%p)->(%ld, %p)\n", iface, cbMaxPart, ppParts);
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
    MimeBody *mime_body;
    body_t *root_body;

    TRACE("(%p, %p)\n", outer, obj);

    if (outer)
    {
        FIXME("outer unknown not supported yet\n");
        return E_NOTIMPL;
    }

    *obj = NULL;

    This = malloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IMimeMessage_iface.lpVtbl = &MimeMessageVtbl;
    This->ref = 1;
    This->stream = NULL;
    list_init(&This->body_tree);
    This->next_index = 1;

    mime_body = mimebody_create();
    root_body = new_body_entry(mime_body, This->next_index++, NULL);
    list_add_head(&This->body_tree, &root_body->entry);

    *obj = &This->IMimeMessage_iface;
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
    FIXME("(0x%lx)\n", dwMode);
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
    IMimeSecurity IMimeSecurity_iface;
    LONG ref;
} MimeSecurity;

static inline MimeSecurity *impl_from_IMimeSecurity(IMimeSecurity *iface)
{
    return CONTAINING_RECORD(iface, MimeSecurity, IMimeSecurity_iface);
}

static HRESULT WINAPI MimeSecurity_QueryInterface(IMimeSecurity *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IMimeSecurity))
    {
        *ppv = iface;
        IMimeSecurity_AddRef(iface);
        return S_OK;
    }

    FIXME("no interface for %s\n", debugstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI MimeSecurity_AddRef(IMimeSecurity *iface)
{
    MimeSecurity *This = impl_from_IMimeSecurity(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI MimeSecurity_Release(IMimeSecurity *iface)
{
    MimeSecurity *This = impl_from_IMimeSecurity(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref)
        free(This);

    return ref;
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
    FIXME("(%p)->(%p, %08lx): stub\n", iface, pTree, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeSecurity_EncodeBody(
        IMimeSecurity* iface,
        IMimeMessageTree* pTree,
        HBODY hEncodeRoot,
        DWORD dwFlags)
{
    FIXME("(%p)->(%p, %p, %08lx): stub\n", iface, pTree, hEncodeRoot, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeSecurity_DecodeMessage(
        IMimeSecurity* iface,
        IMimeMessageTree* pTree,
        DWORD dwFlags)
{
    FIXME("(%p)->(%p, %08lx): stub\n", iface, pTree, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeSecurity_DecodeBody(
        IMimeSecurity* iface,
        IMimeMessageTree* pTree,
        HBODY hDecodeRoot,
        DWORD dwFlags)
{
    FIXME("(%p)->(%p, %p, %08lx): stub\n", iface, pTree, hDecodeRoot, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeSecurity_EnumCertificates(
        IMimeSecurity* iface,
        HCAPICERTSTORE hc,
        DWORD dwUsage,
        PCX509CERT pPrev,
        PCX509CERT* ppCert)
{
    FIXME("(%p)->(%p, %08lx, %p, %p): stub\n", iface, hc, dwUsage, pPrev, ppCert);
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

    This = malloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IMimeSecurity_iface.lpVtbl = &MimeSecurityVtbl;
    This->ref = 1;

    *obj = &This->IMimeSecurity_iface;
    return S_OK;
}

/***********************************************************************
 *              MimeOleCreateSecurity (INETCOMM.@)
 */
HRESULT WINAPI MimeOleCreateSecurity(IMimeSecurity **ppSecurity)
{
    return MimeSecurity_create(NULL, (void **)ppSecurity);
}

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
        IMimeAllocator_AddRef(iface);
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
        SIZE_T cb)
{
    return CoTaskMemAlloc(cb);
}

static LPVOID WINAPI MimeAlloc_Realloc(
        IMimeAllocator* iface,
        LPVOID pv,
        SIZE_T cb)
{
    return CoTaskMemRealloc(pv, cb);
}

static void WINAPI MimeAlloc_Free(
        IMimeAllocator* iface,
        LPVOID pv)
{
    CoTaskMemFree(pv);
}

static SIZE_T WINAPI MimeAlloc_GetSize(
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
    TRACE("(%p)->(%ld, %p, %d)\n", iface, cParams, prgParam, fFreeArray);

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

static IMimeAllocator mime_allocator =
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

HRESULT VirtualStream_create(IUnknown *outer, void **obj)
{
    FIXME("(%p, %p)\n", outer, obj);

    *obj = NULL;
    if (outer) return CLASS_E_NOAGGREGATION;

    return MimeOleCreateVirtualStream((IStream **)obj);
}

/* IMimePropertySchema Interface */
static HRESULT WINAPI propschema_QueryInterface(IMimePropertySchema *iface, REFIID riid, void **out)
{
    propschema *This = impl_from_IMimePropertySchema(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), out);

    *out = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IMimePropertySchema))
    {
        *out = iface;
    }
    else
    {
        FIXME("no interface for %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IMimePropertySchema_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI propschema_AddRef(IMimePropertySchema *iface)
{
    propschema *This = impl_from_IMimePropertySchema(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI propschema_Release(IMimePropertySchema *iface)
{
    propschema *This = impl_from_IMimePropertySchema(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref)
    {
        free(This);
    }

    return ref;
}

static HRESULT WINAPI propschema_RegisterProperty(IMimePropertySchema *iface, const char *name, DWORD flags,
        DWORD rownumber, VARTYPE vtdefault, DWORD *propid)
{
    propschema *This = impl_from_IMimePropertySchema(iface);
    FIXME("(%p)->(%s, %lx, %ld, %d, %p) stub\n", This, debugstr_a(name), flags, rownumber, vtdefault, propid);
    return E_NOTIMPL;
}

static HRESULT WINAPI propschema_ModifyProperty(IMimePropertySchema *iface, const char *name, DWORD flags,
        DWORD rownumber, VARTYPE vtdefault)
{
    propschema *This = impl_from_IMimePropertySchema(iface);
    FIXME("(%p)->(%s, %lx, %ld, %d) stub\n", This, debugstr_a(name), flags, rownumber, vtdefault);
    return S_OK;
}

static HRESULT WINAPI propschema_GetPropertyId(IMimePropertySchema *iface, const char *name, DWORD *propid)
{
    propschema *This = impl_from_IMimePropertySchema(iface);
    FIXME("(%p)->(%s, %p) stub\n", This, debugstr_a(name), propid);
    return E_NOTIMPL;
}

static HRESULT WINAPI propschema_GetPropertyName(IMimePropertySchema *iface, DWORD propid, char **name)
{
    propschema *This = impl_from_IMimePropertySchema(iface);
    FIXME("(%p)->(%ld, %p) stub\n", This, propid, name);
    return E_NOTIMPL;
}

static HRESULT WINAPI propschema_RegisterAddressType(IMimePropertySchema *iface, const char *name, DWORD *adrtype)
{
    propschema *This = impl_from_IMimePropertySchema(iface);
    FIXME("(%p)->(%s, %p) stub\n", This, debugstr_a(name), adrtype);
    return E_NOTIMPL;
}

static IMimePropertySchemaVtbl prop_schema_vtbl =
{
    propschema_QueryInterface,
    propschema_AddRef,
    propschema_Release,
    propschema_RegisterProperty,
    propschema_ModifyProperty,
    propschema_GetPropertyId,
    propschema_GetPropertyName,
    propschema_RegisterAddressType
};


HRESULT WINAPI MimeOleGetPropertySchema(IMimePropertySchema **schema)
{
    propschema *This;

    TRACE("(%p) stub\n", schema);

    This = malloc(sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->IMimePropertySchema_iface.lpVtbl = &prop_schema_vtbl;
    This->ref = 1;

    *schema = &This->IMimePropertySchema_iface;

    return S_OK;
}

HRESULT WINAPI MimeGetAddressFormatW(REFIID riid, void *object, DWORD addr_type,
       ADDRESSFORMAT addr_format, WCHAR **address)
{
    FIXME("(%s, %p, %ld, %d, %p) stub\n", debugstr_guid(riid), object, addr_type, addr_format, address);

    return E_NOTIMPL;
}

static HRESULT WINAPI mime_obj_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    FIXME("(%s %p)\n", debugstr_guid(riid), ppv);
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI mime_obj_AddRef(IUnknown *iface)
{
    TRACE("\n");
    return 2;
}

static ULONG WINAPI mime_obj_Release(IUnknown *iface)
{
    TRACE("\n");
    return 1;
}

static const IUnknownVtbl mime_obj_vtbl = {
    mime_obj_QueryInterface,
    mime_obj_AddRef,
    mime_obj_Release
};

static IUnknown mime_obj = { &mime_obj_vtbl };

HRESULT WINAPI MimeOleObjectFromMoniker(BINDF bindf, IMoniker *moniker, IBindCtx *binding,
       REFIID riid, void **out, IMoniker **moniker_new)
{
    WCHAR *display_name, *mhtml_url;
    size_t len;
    HRESULT hres;

    WARN("(0x%08x, %p, %p, %s, %p, %p) semi-stub\n", bindf, moniker, binding, debugstr_guid(riid), out, moniker_new);

    if(!IsEqualGUID(&IID_IUnknown, riid)) {
        FIXME("Unsupported riid %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    hres = IMoniker_GetDisplayName(moniker, NULL, NULL, &display_name);
    if(FAILED(hres))
        return hres;

    TRACE("display name %s\n", debugstr_w(display_name));

    len = lstrlenW(display_name);
    mhtml_url = malloc(len * sizeof(WCHAR) + sizeof(L"mhtml:"));
    if(!mhtml_url)
        return E_OUTOFMEMORY;

    lstrcpyW(mhtml_url, L"mhtml:");
    lstrcatW(mhtml_url, display_name);
    CoTaskMemFree(display_name);

    hres = CreateURLMoniker(NULL, mhtml_url, moniker_new);
    free(mhtml_url);
    if(FAILED(hres))
        return hres;

    /* FIXME: We most likely should start binding here and return something more meaningful as mime object. */
    *out = &mime_obj;
    return S_OK;
}
