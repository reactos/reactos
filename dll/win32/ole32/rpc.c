/*
 *	RPC Manager
 *
 * Copyright 2001  Ove Kåven, TransGaming Technologies
 * Copyright 2002  Marcus Meissner
 * Copyright 2005  Mike Hearn, Rob Shearman for CodeWeavers
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
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winsvc.h"
#include "objbase.h"
#include "ole2.h"
#include "rpc.h"
#include "winerror.h"
#include "winreg.h"
#include "wine/unicode.h"

#include "compobj_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static void __RPC_STUB dispatch_rpc(RPC_MESSAGE *msg);

/* we only use one function to dispatch calls for all methods - we use the
 * RPC_IF_OLE flag to tell the RPC runtime that this is the case */
static RPC_DISPATCH_FUNCTION rpc_dispatch_table[1] = { dispatch_rpc }; /* (RO) */
static RPC_DISPATCH_TABLE rpc_dispatch = { 1, rpc_dispatch_table }; /* (RO) */

static struct list registered_interfaces = LIST_INIT(registered_interfaces); /* (CS csRegIf) */
static CRITICAL_SECTION csRegIf;
static CRITICAL_SECTION_DEBUG csRegIf_debug =
{
    0, 0, &csRegIf,
    { &csRegIf_debug.ProcessLocksList, &csRegIf_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": dcom registered server interfaces") }
};
static CRITICAL_SECTION csRegIf = { &csRegIf_debug, -1, 0, 0, 0, 0 };

static struct list channel_hooks = LIST_INIT(channel_hooks); /* (CS csChannelHook) */
static CRITICAL_SECTION csChannelHook;
static CRITICAL_SECTION_DEBUG csChannelHook_debug =
{
    0, 0, &csChannelHook,
    { &csChannelHook_debug.ProcessLocksList, &csChannelHook_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": channel hooks") }
};
static CRITICAL_SECTION csChannelHook = { &csChannelHook_debug, -1, 0, 0, 0, 0 };

static WCHAR wszRpcTransport[] = {'n','c','a','l','r','p','c',0};


struct registered_if
{
    struct list entry;
    DWORD refs; /* ref count */
    RPC_SERVER_INTERFACE If; /* interface registered with the RPC runtime */
};

/* get the pipe endpoint specified of the specified apartment */
static inline void get_rpc_endpoint(LPWSTR endpoint, const OXID *oxid)
{
    /* FIXME: should get endpoint from rpcss */
    static const WCHAR wszEndpointFormat[] = {'\\','p','i','p','e','\\','O','L','E','_','%','0','8','l','x','%','0','8','l','x',0};
    wsprintfW(endpoint, wszEndpointFormat, (DWORD)(*oxid >> 32),(DWORD)*oxid);
}

typedef struct
{
    const IRpcChannelBufferVtbl *lpVtbl;
    LONG                  refs;
} RpcChannelBuffer;

typedef struct
{
    RpcChannelBuffer       super; /* superclass */

    RPC_BINDING_HANDLE     bind; /* handle to the remote server */
    OXID                   oxid; /* apartment in which the channel is valid */
    DWORD                  server_pid; /* id of server process */
    DWORD                  dest_context; /* returned from GetDestCtx */
    LPVOID                 dest_context_data; /* returned from GetDestCtx */
    HANDLE                 event; /* cached event handle */
} ClientRpcChannelBuffer;

struct dispatch_params
{
    RPCOLEMESSAGE     *msg; /* message */
    IRpcStubBuffer    *stub; /* stub buffer, if applicable */
    IRpcChannelBuffer *chan; /* server channel buffer, if applicable */
    IID                iid; /* ID of interface being called */
    IUnknown          *iface; /* interface being called */
    HANDLE             handle; /* handle that will become signaled when call finishes */
    BOOL               bypass_rpcrt; /* bypass RPC runtime? */
    RPC_STATUS         status; /* status (out) */
    HRESULT            hr; /* hresult (out) */
};

struct message_state
{
    RPC_BINDING_HANDLE binding_handle;
    ULONG prefix_data_len;
    SChannelHookCallInfo channel_hook_info;
    BOOL bypass_rpcrt;

    /* client only */
    HWND target_hwnd;
    DWORD target_tid;
    struct dispatch_params params;
};

typedef struct
{
    ULONG conformance; /* NDR */
    GUID id;
    ULONG size;
    /* [size_is((size+7)&~7)] */ unsigned char data[1];
} WIRE_ORPC_EXTENT;

struct channel_hook_entry
{
    struct list entry;
    GUID id;
    IChannelHook *hook;
};

struct channel_hook_buffer_data
{
    GUID id;
    ULONG extension_size;
};


static HRESULT unmarshal_ORPCTHAT(RPC_MESSAGE *msg, ORPCTHAT *orpcthat,
                                  ORPC_EXTENT_ARRAY *orpc_ext_array, WIRE_ORPC_EXTENT **first_wire_orpc_extent);

/* Channel Hook Functions */

static ULONG ChannelHooks_ClientGetSize(SChannelHookCallInfo *info,
    struct channel_hook_buffer_data **data, unsigned int *hook_count,
    ULONG *extension_count)
{
    struct channel_hook_entry *entry;
    ULONG total_size = 0;
    unsigned int hook_index = 0;

    *hook_count = 0;
    *extension_count = 0;

    EnterCriticalSection(&csChannelHook);

    LIST_FOR_EACH_ENTRY(entry, &channel_hooks, struct channel_hook_entry, entry)
        (*hook_count)++;

    if (*hook_count)
        *data = HeapAlloc(GetProcessHeap(), 0, *hook_count * sizeof(struct channel_hook_buffer_data));
    else
        *data = NULL;

    LIST_FOR_EACH_ENTRY(entry, &channel_hooks, struct channel_hook_entry, entry)
    {
        ULONG extension_size = 0;

        IChannelHook_ClientGetSize(entry->hook, &entry->id, &info->iid, &extension_size);

        TRACE("%s: extension_size = %u\n", debugstr_guid(&entry->id), extension_size);

        extension_size = (extension_size+7)&~7;
        (*data)[hook_index].id = entry->id;
        (*data)[hook_index].extension_size = extension_size;

        /* an extension is only put onto the wire if it has data to write */
        if (extension_size)
        {
            total_size += FIELD_OFFSET(WIRE_ORPC_EXTENT, data[extension_size]);
            (*extension_count)++;
        }

        hook_index++;
    }

    LeaveCriticalSection(&csChannelHook);

    return total_size;
}

static unsigned char * ChannelHooks_ClientFillBuffer(SChannelHookCallInfo *info,
    unsigned char *buffer, struct channel_hook_buffer_data *data,
    unsigned int hook_count)
{
    struct channel_hook_entry *entry;

    EnterCriticalSection(&csChannelHook);

    LIST_FOR_EACH_ENTRY(entry, &channel_hooks, struct channel_hook_entry, entry)
    {
        unsigned int i;
        ULONG extension_size = 0;
        WIRE_ORPC_EXTENT *wire_orpc_extent = (WIRE_ORPC_EXTENT *)buffer;

        for (i = 0; i < hook_count; i++)
            if (IsEqualGUID(&entry->id, &data[i].id))
                extension_size = data[i].extension_size;

        /* an extension is only put onto the wire if it has data to write */
        if (!extension_size)
            continue;

        IChannelHook_ClientFillBuffer(entry->hook, &entry->id, &info->iid,
            &extension_size, buffer + FIELD_OFFSET(WIRE_ORPC_EXTENT, data[0]));

        TRACE("%s: extension_size = %u\n", debugstr_guid(&entry->id), extension_size);

        /* FIXME: set unused portion of wire_orpc_extent->data to 0? */

        wire_orpc_extent->conformance = (extension_size+7)&~7;
        wire_orpc_extent->size = extension_size;
        wire_orpc_extent->id = entry->id;
        buffer += FIELD_OFFSET(WIRE_ORPC_EXTENT, data[wire_orpc_extent->conformance]);
    }

    LeaveCriticalSection(&csChannelHook);

    return buffer;
}

static void ChannelHooks_ServerNotify(SChannelHookCallInfo *info,
    DWORD lDataRep, WIRE_ORPC_EXTENT *first_wire_orpc_extent,
    ULONG extension_count)
{
    struct channel_hook_entry *entry;
    ULONG i;

    EnterCriticalSection(&csChannelHook);

    LIST_FOR_EACH_ENTRY(entry, &channel_hooks, struct channel_hook_entry, entry)
    {
        WIRE_ORPC_EXTENT *wire_orpc_extent;
        for (i = 0, wire_orpc_extent = first_wire_orpc_extent;
             i < extension_count;
             i++, wire_orpc_extent = (WIRE_ORPC_EXTENT *)&wire_orpc_extent->data[wire_orpc_extent->conformance])
        {
            if (IsEqualGUID(&entry->id, &wire_orpc_extent->id))
                break;
        }
        if (i == extension_count) wire_orpc_extent = NULL;

        IChannelHook_ServerNotify(entry->hook, &entry->id, &info->iid,
            wire_orpc_extent ? wire_orpc_extent->size : 0,
            wire_orpc_extent ? wire_orpc_extent->data : NULL,
            lDataRep);
    }

    LeaveCriticalSection(&csChannelHook);
}

static ULONG ChannelHooks_ServerGetSize(SChannelHookCallInfo *info,
                                        struct channel_hook_buffer_data **data, unsigned int *hook_count,
                                        ULONG *extension_count)
{
    struct channel_hook_entry *entry;
    ULONG total_size = 0;
    unsigned int hook_index = 0;

    *hook_count = 0;
    *extension_count = 0;

    EnterCriticalSection(&csChannelHook);

    LIST_FOR_EACH_ENTRY(entry, &channel_hooks, struct channel_hook_entry, entry)
        (*hook_count)++;

    if (*hook_count)
        *data = HeapAlloc(GetProcessHeap(), 0, *hook_count * sizeof(struct channel_hook_buffer_data));
    else
        *data = NULL;

    LIST_FOR_EACH_ENTRY(entry, &channel_hooks, struct channel_hook_entry, entry)
    {
        ULONG extension_size = 0;

        IChannelHook_ServerGetSize(entry->hook, &entry->id, &info->iid, S_OK,
                                   &extension_size);

        TRACE("%s: extension_size = %u\n", debugstr_guid(&entry->id), extension_size);

        extension_size = (extension_size+7)&~7;
        (*data)[hook_index].id = entry->id;
        (*data)[hook_index].extension_size = extension_size;

        /* an extension is only put onto the wire if it has data to write */
        if (extension_size)
        {
            total_size += FIELD_OFFSET(WIRE_ORPC_EXTENT, data[extension_size]);
            (*extension_count)++;
        }

        hook_index++;
    }

    LeaveCriticalSection(&csChannelHook);

    return total_size;
}

static unsigned char * ChannelHooks_ServerFillBuffer(SChannelHookCallInfo *info,
                                                     unsigned char *buffer, struct channel_hook_buffer_data *data,
                                                     unsigned int hook_count)
{
    struct channel_hook_entry *entry;

    EnterCriticalSection(&csChannelHook);

    LIST_FOR_EACH_ENTRY(entry, &channel_hooks, struct channel_hook_entry, entry)
    {
        unsigned int i;
        ULONG extension_size = 0;
        WIRE_ORPC_EXTENT *wire_orpc_extent = (WIRE_ORPC_EXTENT *)buffer;

        for (i = 0; i < hook_count; i++)
            if (IsEqualGUID(&entry->id, &data[i].id))
                extension_size = data[i].extension_size;

        /* an extension is only put onto the wire if it has data to write */
        if (!extension_size)
            continue;

        IChannelHook_ServerFillBuffer(entry->hook, &entry->id, &info->iid,
                                      &extension_size, buffer + FIELD_OFFSET(WIRE_ORPC_EXTENT, data[0]),
                                      S_OK);

        TRACE("%s: extension_size = %u\n", debugstr_guid(&entry->id), extension_size);

        /* FIXME: set unused portion of wire_orpc_extent->data to 0? */

        wire_orpc_extent->conformance = (extension_size+7)&~7;
        wire_orpc_extent->size = extension_size;
        wire_orpc_extent->id = entry->id;
        buffer += FIELD_OFFSET(WIRE_ORPC_EXTENT, data[wire_orpc_extent->conformance]);
    }

    LeaveCriticalSection(&csChannelHook);

    return buffer;
}

static void ChannelHooks_ClientNotify(SChannelHookCallInfo *info,
                                      DWORD lDataRep, WIRE_ORPC_EXTENT *first_wire_orpc_extent,
                                      ULONG extension_count, HRESULT hrFault)
{
    struct channel_hook_entry *entry;
    ULONG i;

    EnterCriticalSection(&csChannelHook);

    LIST_FOR_EACH_ENTRY(entry, &channel_hooks, struct channel_hook_entry, entry)
    {
        WIRE_ORPC_EXTENT *wire_orpc_extent;
        for (i = 0, wire_orpc_extent = first_wire_orpc_extent;
             i < extension_count;
             i++, wire_orpc_extent = (WIRE_ORPC_EXTENT *)&wire_orpc_extent->data[wire_orpc_extent->conformance])
        {
            if (IsEqualGUID(&entry->id, &wire_orpc_extent->id))
                break;
        }
        if (i == extension_count) wire_orpc_extent = NULL;

        IChannelHook_ClientNotify(entry->hook, &entry->id, &info->iid,
                                  wire_orpc_extent ? wire_orpc_extent->size : 0,
                                  wire_orpc_extent ? wire_orpc_extent->data : NULL,
                                  lDataRep, hrFault);
    }

    LeaveCriticalSection(&csChannelHook);
}

HRESULT RPC_RegisterChannelHook(REFGUID rguid, IChannelHook *hook)
{
    struct channel_hook_entry *entry;

    TRACE("(%s, %p)\n", debugstr_guid(rguid), hook);

    entry = HeapAlloc(GetProcessHeap(), 0, sizeof(*entry));
    if (!entry)
        return E_OUTOFMEMORY;

    entry->id = *rguid;
    entry->hook = hook;
    IChannelHook_AddRef(hook);

    EnterCriticalSection(&csChannelHook);
    list_add_tail(&channel_hooks, &entry->entry);
    LeaveCriticalSection(&csChannelHook);

    return S_OK;
}

void RPC_UnregisterAllChannelHooks(void)
{
    struct channel_hook_entry *cursor;
    struct channel_hook_entry *cursor2;

    EnterCriticalSection(&csChannelHook);
    LIST_FOR_EACH_ENTRY_SAFE(cursor, cursor2, &channel_hooks, struct channel_hook_entry, entry)
        HeapFree(GetProcessHeap(), 0, cursor);
    LeaveCriticalSection(&csChannelHook);
}

/* RPC Channel Buffer Functions */

static HRESULT WINAPI RpcChannelBuffer_QueryInterface(LPRPCCHANNELBUFFER iface, REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid,&IID_IRpcChannelBuffer) || IsEqualIID(riid,&IID_IUnknown))
    {
        *ppv = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI RpcChannelBuffer_AddRef(LPRPCCHANNELBUFFER iface)
{
    RpcChannelBuffer *This = (RpcChannelBuffer *)iface;
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI ServerRpcChannelBuffer_Release(LPRPCCHANNELBUFFER iface)
{
    RpcChannelBuffer *This = (RpcChannelBuffer *)iface;
    ULONG ref;

    ref = InterlockedDecrement(&This->refs);
    if (ref)
        return ref;

    HeapFree(GetProcessHeap(), 0, This);
    return 0;
}

static ULONG WINAPI ClientRpcChannelBuffer_Release(LPRPCCHANNELBUFFER iface)
{
    ClientRpcChannelBuffer *This = (ClientRpcChannelBuffer *)iface;
    ULONG ref;

    ref = InterlockedDecrement(&This->super.refs);
    if (ref)
        return ref;

    if (This->event) CloseHandle(This->event);
    RpcBindingFree(&This->bind);
    HeapFree(GetProcessHeap(), 0, This);
    return 0;
}

static HRESULT WINAPI ServerRpcChannelBuffer_GetBuffer(LPRPCCHANNELBUFFER iface, RPCOLEMESSAGE* olemsg, REFIID riid)
{
    RpcChannelBuffer *This = (RpcChannelBuffer *)iface;
    RPC_MESSAGE *msg = (RPC_MESSAGE *)olemsg;
    RPC_STATUS status;
    ORPCTHAT *orpcthat;
    struct message_state *message_state;
    ULONG extensions_size;
    struct channel_hook_buffer_data *channel_hook_data;
    unsigned int channel_hook_count;
    ULONG extension_count;

    TRACE("(%p)->(%p,%s)\n", This, olemsg, debugstr_guid(riid));

    message_state = msg->Handle;
    /* restore the binding handle and the real start of data */
    msg->Handle = message_state->binding_handle;
    msg->Buffer = (char *)msg->Buffer - message_state->prefix_data_len;

    extensions_size = ChannelHooks_ServerGetSize(&message_state->channel_hook_info,
                                                 &channel_hook_data, &channel_hook_count, &extension_count);

    msg->BufferLength += FIELD_OFFSET(ORPCTHAT, extensions) + 4;
    if (extensions_size)
    {
        msg->BufferLength += FIELD_OFFSET(ORPC_EXTENT_ARRAY, extent) + 2*sizeof(DWORD) + extensions_size;
        if (extension_count & 1)
            msg->BufferLength += FIELD_OFFSET(WIRE_ORPC_EXTENT, data[0]);
    }

    if (message_state->bypass_rpcrt)
    {
        msg->Buffer = HeapAlloc(GetProcessHeap(), 0, msg->BufferLength);
        if (msg->Buffer)
            status = RPC_S_OK;
        else
            status = ERROR_OUTOFMEMORY;
    }
    else
        status = I_RpcGetBuffer(msg);

    orpcthat = msg->Buffer;
    msg->Buffer = (char *)msg->Buffer + FIELD_OFFSET(ORPCTHAT, extensions);

    orpcthat->flags = ORPCF_NULL /* FIXME? */;

    /* NDR representation of orpcthat->extensions */
    *(DWORD *)msg->Buffer = extensions_size ? 1 : 0;
    msg->Buffer = (char *)msg->Buffer + sizeof(DWORD);

    if (extensions_size)
    {
        ORPC_EXTENT_ARRAY *orpc_extent_array = msg->Buffer;
        orpc_extent_array->size = extension_count;
        orpc_extent_array->reserved = 0;
        msg->Buffer = (char *)msg->Buffer + FIELD_OFFSET(ORPC_EXTENT_ARRAY, extent);
        /* NDR representation of orpc_extent_array->extent */
        *(DWORD *)msg->Buffer = 1;
        msg->Buffer = (char *)msg->Buffer + sizeof(DWORD);
        /* NDR representation of [size_is] attribute of orpc_extent_array->extent */
        *(DWORD *)msg->Buffer = (extension_count + 1) & ~1;
        msg->Buffer = (char *)msg->Buffer + sizeof(DWORD);

        msg->Buffer = ChannelHooks_ServerFillBuffer(&message_state->channel_hook_info,
                                                    msg->Buffer, channel_hook_data, channel_hook_count);

        /* we must add a dummy extension if there is an odd extension
         * count to meet the contract specified by the size_is attribute */
        if (extension_count & 1)
        {
            WIRE_ORPC_EXTENT *wire_orpc_extent = msg->Buffer;
            wire_orpc_extent->conformance = 0;
            wire_orpc_extent->id = GUID_NULL;
            wire_orpc_extent->size = 0;
            msg->Buffer = (char *)msg->Buffer + FIELD_OFFSET(WIRE_ORPC_EXTENT, data[0]);
        }
    }

    HeapFree(GetProcessHeap(), 0, channel_hook_data);

    /* store the prefixed data length so that we can restore the real buffer
     * later */
    message_state->prefix_data_len = (char *)msg->Buffer - (char *)orpcthat;
    msg->BufferLength -= message_state->prefix_data_len;
    /* save away the message state again */
    msg->Handle = message_state;

    TRACE("-- %d\n", status);

    return HRESULT_FROM_WIN32(status);
}

static HANDLE ClientRpcChannelBuffer_GetEventHandle(ClientRpcChannelBuffer *This)
{
    HANDLE event = InterlockedExchangePointer(&This->event, NULL);

    /* Note: must be auto-reset event so we can reuse it without a call
    * to ResetEvent */
    if (!event) event = CreateEventW(NULL, FALSE, FALSE, NULL);

    return event;
}

static void ClientRpcChannelBuffer_ReleaseEventHandle(ClientRpcChannelBuffer *This, HANDLE event)
{
    if (InterlockedCompareExchangePointer(&This->event, event, NULL))
        /* already a handle cached in This */
        CloseHandle(event);
}

static HRESULT WINAPI ClientRpcChannelBuffer_GetBuffer(LPRPCCHANNELBUFFER iface, RPCOLEMESSAGE* olemsg, REFIID riid)
{
    ClientRpcChannelBuffer *This = (ClientRpcChannelBuffer *)iface;
    RPC_MESSAGE *msg = (RPC_MESSAGE *)olemsg;
    RPC_CLIENT_INTERFACE *cif;
    RPC_STATUS status;
    ORPCTHIS *orpcthis;
    struct message_state *message_state;
    ULONG extensions_size;
    struct channel_hook_buffer_data *channel_hook_data;
    unsigned int channel_hook_count;
    ULONG extension_count;
    IPID ipid;
    HRESULT hr;
    APARTMENT *apt = NULL;

    TRACE("(%p)->(%p,%s)\n", This, olemsg, debugstr_guid(riid));

    cif = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RPC_CLIENT_INTERFACE));
    if (!cif)
        return E_OUTOFMEMORY;

    message_state = HeapAlloc(GetProcessHeap(), 0, sizeof(*message_state));
    if (!message_state)
    {
        HeapFree(GetProcessHeap(), 0, cif);
        return E_OUTOFMEMORY;
    }

    cif->Length = sizeof(RPC_CLIENT_INTERFACE);
    /* RPC interface ID = COM interface ID */
    cif->InterfaceId.SyntaxGUID = *riid;
    /* COM objects always have a version of 0.0 */
    cif->InterfaceId.SyntaxVersion.MajorVersion = 0;
    cif->InterfaceId.SyntaxVersion.MinorVersion = 0;
    msg->Handle = This->bind;
    msg->RpcInterfaceInformation = cif;

    message_state->prefix_data_len = 0;
    message_state->binding_handle = This->bind;

    message_state->channel_hook_info.iid = *riid;
    message_state->channel_hook_info.cbSize = sizeof(message_state->channel_hook_info);
    message_state->channel_hook_info.uCausality = COM_CurrentCausalityId();
    message_state->channel_hook_info.dwServerPid = This->server_pid;
    message_state->channel_hook_info.iMethod = msg->ProcNum;
    message_state->channel_hook_info.pObject = NULL; /* only present on server-side */
    message_state->target_hwnd = NULL;
    message_state->target_tid = 0;
    memset(&message_state->params, 0, sizeof(message_state->params));

    extensions_size = ChannelHooks_ClientGetSize(&message_state->channel_hook_info,
        &channel_hook_data, &channel_hook_count, &extension_count);

    msg->BufferLength += FIELD_OFFSET(ORPCTHIS, extensions) + 4;
    if (extensions_size)
    {
        msg->BufferLength += FIELD_OFFSET(ORPC_EXTENT_ARRAY, extent) + 2*sizeof(DWORD) + extensions_size;
        if (extension_count & 1)
            msg->BufferLength += FIELD_OFFSET(WIRE_ORPC_EXTENT, data[0]);
    }

    RpcBindingInqObject(message_state->binding_handle, &ipid);
    hr = ipid_get_dispatch_params(&ipid, &apt, &message_state->params.stub,
                                  &message_state->params.chan,
                                  &message_state->params.iid,
                                  &message_state->params.iface);
    if (hr == S_OK)
    {
        /* stub, chan, iface and iid are unneeded in multi-threaded case as we go
         * via the RPC runtime */
        if (apt->multi_threaded)
        {
            IRpcStubBuffer_Release(message_state->params.stub);
            message_state->params.stub = NULL;
            IRpcChannelBuffer_Release(message_state->params.chan);
            message_state->params.chan = NULL;
            message_state->params.iface = NULL;
        }
        else
        {
            message_state->params.bypass_rpcrt = TRUE;
            message_state->target_hwnd = apartment_getwindow(apt);
            message_state->target_tid = apt->tid;
            /* we assume later on that this being non-NULL is the indicator that
             * means call directly instead of going through RPC runtime */
            if (!message_state->target_hwnd)
                ERR("window for apartment %s is NULL\n", wine_dbgstr_longlong(apt->oxid));
        }
    }
    if (apt) apartment_release(apt);
    message_state->params.handle = ClientRpcChannelBuffer_GetEventHandle(This);
    /* Note: message_state->params.msg is initialised in
     * ClientRpcChannelBuffer_SendReceive */

    /* shortcut the RPC runtime */
    if (message_state->target_hwnd)
    {
        msg->Buffer = HeapAlloc(GetProcessHeap(), 0, msg->BufferLength);
        if (msg->Buffer)
            status = RPC_S_OK;
        else
            status = ERROR_OUTOFMEMORY;
    }
    else
        status = I_RpcGetBuffer(msg);

    msg->Handle = message_state;

    if (status == RPC_S_OK)
    {
        orpcthis = msg->Buffer;
        msg->Buffer = (char *)msg->Buffer + FIELD_OFFSET(ORPCTHIS, extensions);

        orpcthis->version.MajorVersion = COM_MAJOR_VERSION;
        orpcthis->version.MinorVersion = COM_MINOR_VERSION;
        orpcthis->flags = message_state->channel_hook_info.dwServerPid ? ORPCF_LOCAL : ORPCF_NULL;
        orpcthis->reserved1 = 0;
        orpcthis->cid = message_state->channel_hook_info.uCausality;

        /* NDR representation of orpcthis->extensions */
        *(DWORD *)msg->Buffer = extensions_size ? 1 : 0;
        msg->Buffer = (char *)msg->Buffer + sizeof(DWORD);

        if (extensions_size)
        {
            ORPC_EXTENT_ARRAY *orpc_extent_array = msg->Buffer;
            orpc_extent_array->size = extension_count;
            orpc_extent_array->reserved = 0;
            msg->Buffer = (char *)msg->Buffer + FIELD_OFFSET(ORPC_EXTENT_ARRAY, extent);
            /* NDR representation of orpc_extent_array->extent */
            *(DWORD *)msg->Buffer = 1;
            msg->Buffer = (char *)msg->Buffer + sizeof(DWORD);
            /* NDR representation of [size_is] attribute of orpc_extent_array->extent */
            *(DWORD *)msg->Buffer = (extension_count + 1) & ~1;
            msg->Buffer = (char *)msg->Buffer + sizeof(DWORD);

            msg->Buffer = ChannelHooks_ClientFillBuffer(&message_state->channel_hook_info,
                msg->Buffer, channel_hook_data, channel_hook_count);

            /* we must add a dummy extension if there is an odd extension
             * count to meet the contract specified by the size_is attribute */
            if (extension_count & 1)
            {
                WIRE_ORPC_EXTENT *wire_orpc_extent = msg->Buffer;
                wire_orpc_extent->conformance = 0;
                wire_orpc_extent->id = GUID_NULL;
                wire_orpc_extent->size = 0;
                msg->Buffer = (char *)msg->Buffer + FIELD_OFFSET(WIRE_ORPC_EXTENT, data[0]);
            }
        }

        /* store the prefixed data length so that we can restore the real buffer
         * pointer in ClientRpcChannelBuffer_SendReceive. */
        message_state->prefix_data_len = (char *)msg->Buffer - (char *)orpcthis;
        msg->BufferLength -= message_state->prefix_data_len;
    }

    HeapFree(GetProcessHeap(), 0, channel_hook_data);

    TRACE("-- %d\n", status);

    return HRESULT_FROM_WIN32(status);
}

static HRESULT WINAPI ServerRpcChannelBuffer_SendReceive(LPRPCCHANNELBUFFER iface, RPCOLEMESSAGE *olemsg, ULONG *pstatus)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

/* this thread runs an outgoing RPC */
static DWORD WINAPI rpc_sendreceive_thread(LPVOID param)
{
    struct dispatch_params *data = param;

    /* Note: I_RpcSendReceive doesn't raise exceptions like the higher-level
     * RPC functions do */
    data->status = I_RpcSendReceive((RPC_MESSAGE *)data->msg);

    TRACE("completed with status 0x%x\n", data->status);

    SetEvent(data->handle);

    return 0;
}

static inline HRESULT ClientRpcChannelBuffer_IsCorrectApartment(ClientRpcChannelBuffer *This, APARTMENT *apt)
{
    OXID oxid;
    if (!apt)
        return S_FALSE;
    if (apartment_getoxid(apt, &oxid) != S_OK)
        return S_FALSE;
    if (This->oxid != oxid)
        return S_FALSE;
    return S_OK;
}

static HRESULT WINAPI ClientRpcChannelBuffer_SendReceive(LPRPCCHANNELBUFFER iface, RPCOLEMESSAGE *olemsg, ULONG *pstatus)
{
    ClientRpcChannelBuffer *This = (ClientRpcChannelBuffer *)iface;
    HRESULT hr;
    RPC_MESSAGE *msg = (RPC_MESSAGE *)olemsg;
    RPC_STATUS status;
    DWORD index;
    struct message_state *message_state;
    ORPCTHAT orpcthat;
    ORPC_EXTENT_ARRAY orpc_ext_array;
    WIRE_ORPC_EXTENT *first_wire_orpc_extent = NULL;
    HRESULT hrFault = S_OK;

    TRACE("(%p) iMethod=%d\n", olemsg, olemsg->iMethod);

    hr = ClientRpcChannelBuffer_IsCorrectApartment(This, COM_CurrentApt());
    if (hr != S_OK)
    {
        ERR("called from wrong apartment, should have been 0x%s\n",
            wine_dbgstr_longlong(This->oxid));
        return RPC_E_WRONG_THREAD;
    }
    /* This situation should be impossible in multi-threaded apartments,
     * because the calling thread isn't re-enterable.
     * Note: doing a COM call during the processing of a sent message is
     * only disallowed if a client call is already being waited for
     * completion */
    if (!COM_CurrentApt()->multi_threaded &&
        COM_CurrentInfo()->pending_call_count_client &&
        InSendMessage())
    {
        ERR("can't make an outgoing COM call in response to a sent message\n");
        return RPC_E_CANTCALLOUT_ININPUTSYNCCALL;
    }

    message_state = msg->Handle;
    /* restore the binding handle and the real start of data */
    msg->Handle = message_state->binding_handle;
    msg->Buffer = (char *)msg->Buffer - message_state->prefix_data_len;
    msg->BufferLength += message_state->prefix_data_len;

    /* Note: this is an optimization in the Microsoft OLE runtime that we need
     * to copy, as shown by the test_no_couninitialize_client test. without
     * short-circuiting the RPC runtime in the case below, the test will
     * deadlock on the loader lock due to the RPC runtime needing to create
     * a thread to process the RPC when this function is called indirectly
     * from DllMain */

    message_state->params.msg = olemsg;
    if (message_state->params.bypass_rpcrt)
    {
        TRACE("Calling apartment thread 0x%08x...\n", message_state->target_tid);

        msg->ProcNum &= ~RPC_FLAGS_VALID_BIT;

        if (!PostMessageW(message_state->target_hwnd, DM_EXECUTERPC, 0,
                          (LPARAM)&message_state->params))
        {
            ERR("PostMessage failed with error %u\n", GetLastError());

            /* Note: message_state->params.iface doesn't have a reference and
             * so doesn't need to be released */

            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        /* we use a separate thread here because we need to be able to
         * pump the message loop in the application thread: if we do not,
         * any windows created by this thread will hang and RPCs that try
         * and re-enter this STA from an incoming server thread will
         * deadlock. InstallShield is an example of that.
         */
        if (!QueueUserWorkItem(rpc_sendreceive_thread, &message_state->params, WT_EXECUTEDEFAULT))
        {
            ERR("QueueUserWorkItem failed with error %u\n", GetLastError());
            hr = E_UNEXPECTED;
        }
        else
            hr = S_OK;
    }

    if (hr == S_OK)
    {
        if (WaitForSingleObject(message_state->params.handle, 0))
        {
            COM_CurrentInfo()->pending_call_count_client++;
            hr = CoWaitForMultipleHandles(0, INFINITE, 1, &message_state->params.handle, &index);
            COM_CurrentInfo()->pending_call_count_client--;
        }
    }
    ClientRpcChannelBuffer_ReleaseEventHandle(This, message_state->params.handle);

    /* for WM shortcut, faults are returned in params->hr */
    if (hr == S_OK)
        hrFault = message_state->params.hr;

    status = message_state->params.status;

    orpcthat.flags = ORPCF_NULL;
    orpcthat.extensions = NULL;

    TRACE("RPC call status: 0x%x\n", status);
    if (status != RPC_S_OK)
        hr = HRESULT_FROM_WIN32(status);

    TRACE("hrFault = 0x%08x\n", hrFault);

    /* FIXME: this condition should be
     * "hr == S_OK && (!hrFault || msg->BufferLength > FIELD_OFFSET(ORPCTHAT, extensions) + 4)"
     * but we don't currently reset the message length for PostMessage
     * dispatched calls */
    if (hr == S_OK && hrFault == S_OK)
    {
        HRESULT hr2;
        char *original_buffer = msg->Buffer;

        /* handle ORPCTHAT and client extensions */

        hr2 = unmarshal_ORPCTHAT(msg, &orpcthat, &orpc_ext_array, &first_wire_orpc_extent);
        if (FAILED(hr2))
            hr = hr2;

        message_state->prefix_data_len = (char *)msg->Buffer - original_buffer;
        msg->BufferLength -= message_state->prefix_data_len;
    }
    else
        message_state->prefix_data_len = 0;

    if (hr == S_OK)
    {
        ChannelHooks_ClientNotify(&message_state->channel_hook_info,
                                  msg->DataRepresentation,
                                  first_wire_orpc_extent,
                                  orpcthat.extensions && first_wire_orpc_extent ? orpcthat.extensions->size : 0,
                                  hrFault);
    }

    /* save away the message state again */
    msg->Handle = message_state;

    if (pstatus) *pstatus = status;

    if (hr == S_OK)
        hr = hrFault;

    TRACE("-- 0x%08x\n", hr);

    return hr;
}

static HRESULT WINAPI ServerRpcChannelBuffer_FreeBuffer(LPRPCCHANNELBUFFER iface, RPCOLEMESSAGE* olemsg)
{
    RPC_MESSAGE *msg = (RPC_MESSAGE *)olemsg;
    RPC_STATUS status;
    struct message_state *message_state;

    TRACE("(%p)\n", msg);

    message_state = msg->Handle;
    /* restore the binding handle and the real start of data */
    msg->Handle = message_state->binding_handle;
    msg->Buffer = (char *)msg->Buffer - message_state->prefix_data_len;
    msg->BufferLength += message_state->prefix_data_len;
    message_state->prefix_data_len = 0;

    if (message_state->bypass_rpcrt)
    {
        HeapFree(GetProcessHeap(), 0, msg->Buffer);
        status = RPC_S_OK;
    }
    else
        status = I_RpcFreeBuffer(msg);

    msg->Handle = message_state;

    TRACE("-- %d\n", status);

    return HRESULT_FROM_WIN32(status);
}

static HRESULT WINAPI ClientRpcChannelBuffer_FreeBuffer(LPRPCCHANNELBUFFER iface, RPCOLEMESSAGE* olemsg)
{
    RPC_MESSAGE *msg = (RPC_MESSAGE *)olemsg;
    RPC_STATUS status;
    struct message_state *message_state;

    TRACE("(%p)\n", msg);

    message_state = msg->Handle;
    /* restore the binding handle and the real start of data */
    msg->Handle = message_state->binding_handle;
    msg->Buffer = (char *)msg->Buffer - message_state->prefix_data_len;
    msg->BufferLength += message_state->prefix_data_len;

    if (message_state->params.bypass_rpcrt)
    {
        HeapFree(GetProcessHeap(), 0, msg->Buffer);
        status = RPC_S_OK;
    }
    else
        status = I_RpcFreeBuffer(msg);

    HeapFree(GetProcessHeap(), 0, msg->RpcInterfaceInformation);
    msg->RpcInterfaceInformation = NULL;

    if (message_state->params.stub)
        IRpcStubBuffer_Release(message_state->params.stub);
    if (message_state->params.chan)
        IRpcChannelBuffer_Release(message_state->params.chan);
    HeapFree(GetProcessHeap(), 0, message_state);

    TRACE("-- %d\n", status);

    return HRESULT_FROM_WIN32(status);
}

static HRESULT WINAPI ClientRpcChannelBuffer_GetDestCtx(LPRPCCHANNELBUFFER iface, DWORD* pdwDestContext, void** ppvDestContext)
{
    ClientRpcChannelBuffer *This = (ClientRpcChannelBuffer *)iface;

    TRACE("(%p,%p)\n", pdwDestContext, ppvDestContext);

    *pdwDestContext = This->dest_context;
    *ppvDestContext = This->dest_context_data;

    return S_OK;
}

static HRESULT WINAPI ServerRpcChannelBuffer_GetDestCtx(LPRPCCHANNELBUFFER iface, DWORD* pdwDestContext, void** ppvDestContext)
{
    WARN("(%p,%p), stub!\n", pdwDestContext, ppvDestContext);

    /* FIXME: implement this by storing the dwDestContext and pvDestContext
     * values passed into IMarshal_MarshalInterface and returning them here */
    *pdwDestContext = MSHCTX_DIFFERENTMACHINE;
    *ppvDestContext = NULL;
    return S_OK;
}

static HRESULT WINAPI RpcChannelBuffer_IsConnected(LPRPCCHANNELBUFFER iface)
{
    TRACE("()\n");
    /* native does nothing too */
    return S_OK;
}

static const IRpcChannelBufferVtbl ClientRpcChannelBufferVtbl =
{
    RpcChannelBuffer_QueryInterface,
    RpcChannelBuffer_AddRef,
    ClientRpcChannelBuffer_Release,
    ClientRpcChannelBuffer_GetBuffer,
    ClientRpcChannelBuffer_SendReceive,
    ClientRpcChannelBuffer_FreeBuffer,
    ClientRpcChannelBuffer_GetDestCtx,
    RpcChannelBuffer_IsConnected
};

static const IRpcChannelBufferVtbl ServerRpcChannelBufferVtbl =
{
    RpcChannelBuffer_QueryInterface,
    RpcChannelBuffer_AddRef,
    ServerRpcChannelBuffer_Release,
    ServerRpcChannelBuffer_GetBuffer,
    ServerRpcChannelBuffer_SendReceive,
    ServerRpcChannelBuffer_FreeBuffer,
    ServerRpcChannelBuffer_GetDestCtx,
    RpcChannelBuffer_IsConnected
};

/* returns a channel buffer for proxies */
HRESULT RPC_CreateClientChannel(const OXID *oxid, const IPID *ipid,
                                const OXID_INFO *oxid_info,
                                DWORD dest_context, void *dest_context_data,
                                IRpcChannelBuffer **chan)
{
    ClientRpcChannelBuffer *This;
    WCHAR                   endpoint[200];
    RPC_BINDING_HANDLE      bind;
    RPC_STATUS              status;
    LPWSTR                  string_binding;

    /* FIXME: get the endpoint from oxid_info->psa instead */
    get_rpc_endpoint(endpoint, oxid);

    TRACE("proxy pipe: connecting to endpoint: %s\n", debugstr_w(endpoint));

    status = RpcStringBindingComposeW(
        NULL,
        wszRpcTransport,
        NULL,
        endpoint,
        NULL,
        &string_binding);
        
    if (status == RPC_S_OK)
    {
        status = RpcBindingFromStringBindingW(string_binding, &bind);

        if (status == RPC_S_OK)
        {
            IPID ipid2 = *ipid; /* why can't RpcBindingSetObject take a const? */
            status = RpcBindingSetObject(bind, &ipid2);
            if (status != RPC_S_OK)
                RpcBindingFree(&bind);
        }

        RpcStringFreeW(&string_binding);
    }

    if (status != RPC_S_OK)
    {
        ERR("Couldn't get binding for endpoint %s, status = %d\n", debugstr_w(endpoint), status);
        return HRESULT_FROM_WIN32(status);
    }

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
    {
        RpcBindingFree(&bind);
        return E_OUTOFMEMORY;
    }

    This->super.lpVtbl = &ClientRpcChannelBufferVtbl;
    This->super.refs = 1;
    This->bind = bind;
    apartment_getoxid(COM_CurrentApt(), &This->oxid);
    This->server_pid = oxid_info->dwPid;
    This->dest_context = dest_context;
    This->dest_context_data = dest_context_data;
    This->event = NULL;

    *chan = (IRpcChannelBuffer*)This;

    return S_OK;
}

HRESULT RPC_CreateServerChannel(IRpcChannelBuffer **chan)
{
    RpcChannelBuffer *This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->lpVtbl = &ServerRpcChannelBufferVtbl;
    This->refs = 1;
    
    *chan = (IRpcChannelBuffer*)This;

    return S_OK;
}

/* unmarshals ORPC_EXTENT_ARRAY according to NDR rules, but doesn't allocate
 * any memory */
static HRESULT unmarshal_ORPC_EXTENT_ARRAY(RPC_MESSAGE *msg, const char *end,
                                           ORPC_EXTENT_ARRAY *extensions,
                                           WIRE_ORPC_EXTENT **first_wire_orpc_extent)
{
    DWORD pointer_id;
    DWORD i;

    memcpy(extensions, msg->Buffer, FIELD_OFFSET(ORPC_EXTENT_ARRAY, extent));
    msg->Buffer = (char *)msg->Buffer + FIELD_OFFSET(ORPC_EXTENT_ARRAY, extent);

    if ((const char *)msg->Buffer + 2 * sizeof(DWORD) > end)
        return RPC_E_INVALID_HEADER;

    pointer_id = *(DWORD *)msg->Buffer;
    msg->Buffer = (char *)msg->Buffer + sizeof(DWORD);
    extensions->extent = NULL;

    if (pointer_id)
    {
        WIRE_ORPC_EXTENT *wire_orpc_extent;

        /* conformance */
        if (*(DWORD *)msg->Buffer != ((extensions->size+1)&~1))
            return RPC_S_INVALID_BOUND;

        msg->Buffer = (char *)msg->Buffer + sizeof(DWORD);

        /* arbitrary limit for security (don't know what native does) */
        if (extensions->size > 256)
        {
            ERR("too many extensions: %d\n", extensions->size);
            return RPC_S_INVALID_BOUND;
        }

        *first_wire_orpc_extent = wire_orpc_extent = msg->Buffer;
        for (i = 0; i < ((extensions->size+1)&~1); i++)
        {
            if ((const char *)&wire_orpc_extent->data[0] > end)
                return RPC_S_INVALID_BOUND;
            if (wire_orpc_extent->conformance != ((wire_orpc_extent->size+7)&~7))
                return RPC_S_INVALID_BOUND;
            if ((const char *)&wire_orpc_extent->data[wire_orpc_extent->conformance] > end)
                return RPC_S_INVALID_BOUND;
            TRACE("size %u, guid %s\n", wire_orpc_extent->size, debugstr_guid(&wire_orpc_extent->id));
            wire_orpc_extent = (WIRE_ORPC_EXTENT *)&wire_orpc_extent->data[wire_orpc_extent->conformance];
        }
        msg->Buffer = wire_orpc_extent;
    }

    return S_OK;
}

/* unmarshals ORPCTHIS according to NDR rules, but doesn't allocate any memory */
static HRESULT unmarshal_ORPCTHIS(RPC_MESSAGE *msg, ORPCTHIS *orpcthis,
    ORPC_EXTENT_ARRAY *orpc_ext_array, WIRE_ORPC_EXTENT **first_wire_orpc_extent)
{
    const char *end = (char *)msg->Buffer + msg->BufferLength;

    *first_wire_orpc_extent = NULL;

    if (msg->BufferLength < FIELD_OFFSET(ORPCTHIS, extensions) + 4)
    {
        ERR("invalid buffer length\n");
        return RPC_E_INVALID_HEADER;
    }

    memcpy(orpcthis, msg->Buffer, FIELD_OFFSET(ORPCTHIS, extensions));
    msg->Buffer = (char *)msg->Buffer + FIELD_OFFSET(ORPCTHIS, extensions);

    if ((const char *)msg->Buffer + sizeof(DWORD) > end)
        return RPC_E_INVALID_HEADER;

    if (*(DWORD *)msg->Buffer)
        orpcthis->extensions = orpc_ext_array;
    else
        orpcthis->extensions = NULL;

    msg->Buffer = (char *)msg->Buffer + sizeof(DWORD);

    if (orpcthis->extensions)
    {
        HRESULT hr = unmarshal_ORPC_EXTENT_ARRAY(msg, end, orpc_ext_array,
                                                 first_wire_orpc_extent);
        if (FAILED(hr))
            return hr;
    }

    if ((orpcthis->version.MajorVersion != COM_MAJOR_VERSION) ||
        (orpcthis->version.MinorVersion > COM_MINOR_VERSION))
    {
        ERR("COM version {%d, %d} not supported\n",
            orpcthis->version.MajorVersion, orpcthis->version.MinorVersion);
        return RPC_E_VERSION_MISMATCH;
    }

    if (orpcthis->flags & ~(ORPCF_LOCAL|ORPCF_RESERVED1|ORPCF_RESERVED2|ORPCF_RESERVED3|ORPCF_RESERVED4))
    {
        ERR("invalid flags 0x%x\n", orpcthis->flags & ~(ORPCF_LOCAL|ORPCF_RESERVED1|ORPCF_RESERVED2|ORPCF_RESERVED3|ORPCF_RESERVED4));
        return RPC_E_INVALID_HEADER;
    }

    return S_OK;
}

static HRESULT unmarshal_ORPCTHAT(RPC_MESSAGE *msg, ORPCTHAT *orpcthat,
                                  ORPC_EXTENT_ARRAY *orpc_ext_array, WIRE_ORPC_EXTENT **first_wire_orpc_extent)
{
    const char *end = (char *)msg->Buffer + msg->BufferLength;

    *first_wire_orpc_extent = NULL;

    if (msg->BufferLength < FIELD_OFFSET(ORPCTHAT, extensions) + 4)
    {
        ERR("invalid buffer length\n");
        return RPC_E_INVALID_HEADER;
    }

    memcpy(orpcthat, msg->Buffer, FIELD_OFFSET(ORPCTHAT, extensions));
    msg->Buffer = (char *)msg->Buffer + FIELD_OFFSET(ORPCTHAT, extensions);

    if ((const char *)msg->Buffer + sizeof(DWORD) > end)
        return RPC_E_INVALID_HEADER;

    if (*(DWORD *)msg->Buffer)
        orpcthat->extensions = orpc_ext_array;
    else
        orpcthat->extensions = NULL;

    msg->Buffer = (char *)msg->Buffer + sizeof(DWORD);

    if (orpcthat->extensions)
    {
        HRESULT hr = unmarshal_ORPC_EXTENT_ARRAY(msg, end, orpc_ext_array,
                                                 first_wire_orpc_extent);
        if (FAILED(hr))
            return hr;
    }

    if (orpcthat->flags & ~(ORPCF_LOCAL|ORPCF_RESERVED1|ORPCF_RESERVED2|ORPCF_RESERVED3|ORPCF_RESERVED4))
    {
        ERR("invalid flags 0x%x\n", orpcthat->flags & ~(ORPCF_LOCAL|ORPCF_RESERVED1|ORPCF_RESERVED2|ORPCF_RESERVED3|ORPCF_RESERVED4));
        return RPC_E_INVALID_HEADER;
    }

    return S_OK;
}

void RPC_ExecuteCall(struct dispatch_params *params)
{
    struct message_state *message_state = NULL;
    RPC_MESSAGE *msg = (RPC_MESSAGE *)params->msg;
    char *original_buffer = msg->Buffer;
    ORPCTHIS orpcthis;
    ORPC_EXTENT_ARRAY orpc_ext_array;
    WIRE_ORPC_EXTENT *first_wire_orpc_extent;
    GUID old_causality_id;

    /* handle ORPCTHIS and server extensions */

    params->hr = unmarshal_ORPCTHIS(msg, &orpcthis, &orpc_ext_array, &first_wire_orpc_extent);
    if (params->hr != S_OK)
    {
        msg->Buffer = original_buffer;
        goto exit;
    }

    message_state = HeapAlloc(GetProcessHeap(), 0, sizeof(*message_state));
    if (!message_state)
    {
        params->hr = E_OUTOFMEMORY;
        msg->Buffer = original_buffer;
        goto exit;
    }

    message_state->prefix_data_len = (char *)msg->Buffer - original_buffer;
    message_state->binding_handle = msg->Handle;
    message_state->bypass_rpcrt = params->bypass_rpcrt;

    message_state->channel_hook_info.iid = params->iid;
    message_state->channel_hook_info.cbSize = sizeof(message_state->channel_hook_info);
    message_state->channel_hook_info.uCausality = orpcthis.cid;
    message_state->channel_hook_info.dwServerPid = GetCurrentProcessId();
    message_state->channel_hook_info.iMethod = msg->ProcNum;
    message_state->channel_hook_info.pObject = params->iface;

    if (orpcthis.extensions && first_wire_orpc_extent &&
        orpcthis.extensions->size)
        ChannelHooks_ServerNotify(&message_state->channel_hook_info, msg->DataRepresentation, first_wire_orpc_extent, orpcthis.extensions->size);

    msg->Handle = message_state;
    msg->BufferLength -= message_state->prefix_data_len;

    /* call message filter */

    if (COM_CurrentApt()->filter)
    {
        DWORD handlecall;
        INTERFACEINFO interface_info;
        CALLTYPE calltype;

        interface_info.pUnk = params->iface;
        interface_info.iid = params->iid;
        interface_info.wMethod = msg->ProcNum;

        if (IsEqualGUID(&orpcthis.cid, &COM_CurrentInfo()->causality_id))
            calltype = CALLTYPE_NESTED;
        else if (COM_CurrentInfo()->pending_call_count_server == 0)
            calltype = CALLTYPE_TOPLEVEL;
        else
            calltype = CALLTYPE_TOPLEVEL_CALLPENDING;

        handlecall = IMessageFilter_HandleInComingCall(COM_CurrentApt()->filter,
                                                       calltype,
                                                       (HTASK)GetCurrentProcessId(),
                                                       0 /* FIXME */,
                                                       &interface_info);
        TRACE("IMessageFilter_HandleInComingCall returned %d\n", handlecall);
        switch (handlecall)
        {
        case SERVERCALL_REJECTED:
            params->hr = RPC_E_CALL_REJECTED;
            goto exit_reset_state;
        case SERVERCALL_RETRYLATER:
#if 0 /* FIXME: handle retries on the client side before enabling this code */
            params->hr = RPC_E_RETRY;
            goto exit_reset_state;
#else
            FIXME("retry call later not implemented\n");
            break;
#endif
        case SERVERCALL_ISHANDLED:
        default:
            break;
        }
    }

    /* invoke the method */

    /* save the old causality ID - note: any calls executed while processing
     * messages received during the SendReceive will appear to originate from
     * this call - this should be checked with what Windows does */
    old_causality_id = COM_CurrentInfo()->causality_id;
    COM_CurrentInfo()->causality_id = orpcthis.cid;
    COM_CurrentInfo()->pending_call_count_server++;
    params->hr = IRpcStubBuffer_Invoke(params->stub, params->msg, params->chan);
    COM_CurrentInfo()->pending_call_count_server--;
    COM_CurrentInfo()->causality_id = old_causality_id;

    /* the invoke allocated a new buffer, so free the old one */
    if (message_state->bypass_rpcrt && original_buffer != msg->Buffer)
        HeapFree(GetProcessHeap(), 0, original_buffer);

exit_reset_state:
    message_state = msg->Handle;
    msg->Handle = message_state->binding_handle;
    msg->Buffer = (char *)msg->Buffer - message_state->prefix_data_len;
    msg->BufferLength += message_state->prefix_data_len;

exit:
    HeapFree(GetProcessHeap(), 0, message_state);
    if (params->handle) SetEvent(params->handle);
}

static void __RPC_STUB dispatch_rpc(RPC_MESSAGE *msg)
{
    struct dispatch_params *params;
    APARTMENT *apt;
    IPID ipid;
    HRESULT hr;

    RpcBindingInqObject(msg->Handle, &ipid);

    TRACE("ipid = %s, iMethod = %d\n", debugstr_guid(&ipid), msg->ProcNum);

    params = HeapAlloc(GetProcessHeap(), 0, sizeof(*params));
    if (!params)
    {
        RpcRaiseException(E_OUTOFMEMORY);
        return;
    }

    hr = ipid_get_dispatch_params(&ipid, &apt, &params->stub, &params->chan,
                                  &params->iid, &params->iface);
    if (hr != S_OK)
    {
        ERR("no apartment found for ipid %s\n", debugstr_guid(&ipid));
        HeapFree(GetProcessHeap(), 0, params);
        RpcRaiseException(hr);
        return;
    }

    params->msg = (RPCOLEMESSAGE *)msg;
    params->status = RPC_S_OK;
    params->hr = S_OK;
    params->handle = NULL;
    params->bypass_rpcrt = FALSE;

    /* Note: this is the important difference between STAs and MTAs - we
     * always execute RPCs to STAs in the thread that originally created the
     * apartment (i.e. the one that pumps messages to the window) */
    if (!apt->multi_threaded)
    {
        params->handle = CreateEventW(NULL, FALSE, FALSE, NULL);

        TRACE("Calling apartment thread 0x%08x...\n", apt->tid);

        if (PostMessageW(apartment_getwindow(apt), DM_EXECUTERPC, 0, (LPARAM)params))
            WaitForSingleObject(params->handle, INFINITE);
        else
        {
            ERR("PostMessage failed with error %u\n", GetLastError());
            IRpcChannelBuffer_Release(params->chan);
            IRpcStubBuffer_Release(params->stub);
        }
        CloseHandle(params->handle);
    }
    else
    {
        BOOL joined = FALSE;
        if (!COM_CurrentInfo()->apt)
        {
            apartment_joinmta();
            joined = TRUE;
        }
        RPC_ExecuteCall(params);
        if (joined)
        {
            apartment_release(COM_CurrentInfo()->apt);
            COM_CurrentInfo()->apt = NULL;
        }
    }

    hr = params->hr;
    if (params->chan)
        IRpcChannelBuffer_Release(params->chan);
    if (params->stub)
        IRpcStubBuffer_Release(params->stub);
    HeapFree(GetProcessHeap(), 0, params);

    apartment_release(apt);

    /* if IRpcStubBuffer_Invoke fails, we should raise an exception to tell
     * the RPC runtime that the call failed */
    if (hr) RpcRaiseException(hr);
}

/* stub registration */
HRESULT RPC_RegisterInterface(REFIID riid)
{
    struct registered_if *rif;
    BOOL found = FALSE;
    HRESULT hr = S_OK;
    
    TRACE("(%s)\n", debugstr_guid(riid));

    EnterCriticalSection(&csRegIf);
    LIST_FOR_EACH_ENTRY(rif, &registered_interfaces, struct registered_if, entry)
    {
        if (IsEqualGUID(&rif->If.InterfaceId.SyntaxGUID, riid))
        {
            rif->refs++;
            found = TRUE;
            break;
        }
    }
    if (!found)
    {
        TRACE("Creating new interface\n");

        rif = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*rif));
        if (rif)
        {
            RPC_STATUS status;

            rif->refs = 1;
            rif->If.Length = sizeof(RPC_SERVER_INTERFACE);
            /* RPC interface ID = COM interface ID */
            rif->If.InterfaceId.SyntaxGUID = *riid;
            rif->If.DispatchTable = &rpc_dispatch;
            /* all other fields are 0, including the version asCOM objects
             * always have a version of 0.0 */
            status = RpcServerRegisterIfEx(
                (RPC_IF_HANDLE)&rif->If,
                NULL, NULL,
                RPC_IF_OLE | RPC_IF_AUTOLISTEN,
                RPC_C_LISTEN_MAX_CALLS_DEFAULT,
                NULL);
            if (status == RPC_S_OK)
                list_add_tail(&registered_interfaces, &rif->entry);
            else
            {
                ERR("RpcServerRegisterIfEx failed with error %d\n", status);
                HeapFree(GetProcessHeap(), 0, rif);
                hr = HRESULT_FROM_WIN32(status);
            }
        }
        else
            hr = E_OUTOFMEMORY;
    }
    LeaveCriticalSection(&csRegIf);
    return hr;
}

/* stub unregistration */
void RPC_UnregisterInterface(REFIID riid)
{
    struct registered_if *rif;
    EnterCriticalSection(&csRegIf);
    LIST_FOR_EACH_ENTRY(rif, &registered_interfaces, struct registered_if, entry)
    {
        if (IsEqualGUID(&rif->If.InterfaceId.SyntaxGUID, riid))
        {
            if (!--rif->refs)
            {
                RpcServerUnregisterIf((RPC_IF_HANDLE)&rif->If, NULL, TRUE);
                list_remove(&rif->entry);
                HeapFree(GetProcessHeap(), 0, rif);
            }
            break;
        }
    }
    LeaveCriticalSection(&csRegIf);
}

/* get the info for an OXID, including the IPID for the rem unknown interface
 * and the string binding */
HRESULT RPC_ResolveOxid(OXID oxid, OXID_INFO *oxid_info)
{
    TRACE("%s\n", wine_dbgstr_longlong(oxid));

    oxid_info->dwTid = 0;
    oxid_info->dwPid = 0;
    oxid_info->dwAuthnHint = RPC_C_AUTHN_LEVEL_NONE;
    /* FIXME: this is a hack around not having an OXID resolver yet -
     * this function should contact the machine's OXID resolver and then it
     * should give us the IPID of the IRemUnknown interface */
    oxid_info->ipidRemUnknown.Data1 = 0xffffffff;
    oxid_info->ipidRemUnknown.Data2 = 0xffff;
    oxid_info->ipidRemUnknown.Data3 = 0xffff;
    memcpy(oxid_info->ipidRemUnknown.Data4, &oxid, sizeof(OXID));
    oxid_info->psa = NULL /* FIXME */;

    return S_OK;
}

/* make the apartment reachable by other threads and processes and create the
 * IRemUnknown object */
void RPC_StartRemoting(struct apartment *apt)
{
    if (!InterlockedExchange(&apt->remoting_started, TRUE))
    {
        WCHAR endpoint[200];
        RPC_STATUS status;

        get_rpc_endpoint(endpoint, &apt->oxid);
    
        status = RpcServerUseProtseqEpW(
            wszRpcTransport,
            RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
            endpoint,
            NULL);
        if (status != RPC_S_OK)
            ERR("Couldn't register endpoint %s\n", debugstr_w(endpoint));

        /* FIXME: move remote unknown exporting into this function */
    }
    start_apartment_remote_unknown();
}


static HRESULT create_server(REFCLSID rclsid)
{
    static const WCHAR  wszLocalServer32[] = { 'L','o','c','a','l','S','e','r','v','e','r','3','2',0 };
    static const WCHAR  embedding[] = { ' ', '-','E','m','b','e','d','d','i','n','g',0 };
    HKEY                key;
    HRESULT             hres;
    WCHAR               command[MAX_PATH+sizeof(embedding)/sizeof(WCHAR)];
    DWORD               size = (MAX_PATH+1) * sizeof(WCHAR);
    STARTUPINFOW        sinfo;
    PROCESS_INFORMATION pinfo;

    hres = COM_OpenKeyForCLSID(rclsid, wszLocalServer32, KEY_READ, &key);
    if (FAILED(hres)) {
        ERR("class %s not registered\n", debugstr_guid(rclsid));
        return hres;
    }

    hres = RegQueryValueExW(key, NULL, NULL, NULL, (LPBYTE)command, &size);
    RegCloseKey(key);
    if (hres) {
        WARN("No default value for LocalServer32 key\n");
        return REGDB_E_CLASSNOTREG; /* FIXME: check retval */
    }

    memset(&sinfo,0,sizeof(sinfo));
    sinfo.cb = sizeof(sinfo);

    /* EXE servers are started with the -Embedding switch. */

    strcatW(command, embedding);

    TRACE("activating local server %s for %s\n", debugstr_w(command), debugstr_guid(rclsid));

    /* FIXME: Win2003 supports a ServerExecutable value that is passed into
     * CreateProcess */
    if (!CreateProcessW(NULL, command, NULL, NULL, FALSE, 0, NULL, NULL, &sinfo, &pinfo)) {
        WARN("failed to run local server %s\n", debugstr_w(command));
        return HRESULT_FROM_WIN32(GetLastError());
    }
    CloseHandle(pinfo.hProcess);
    CloseHandle(pinfo.hThread);

    return S_OK;
}

/*
 * start_local_service()  - start a service given its name and parameters
 */
static DWORD start_local_service(LPCWSTR name, DWORD num, LPCWSTR *params)
{
    SC_HANDLE handle, hsvc;
    DWORD     r = ERROR_FUNCTION_FAILED;

    TRACE("Starting service %s %d params\n", debugstr_w(name), num);

    handle = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!handle)
        return r;
    hsvc = OpenServiceW(handle, name, SERVICE_START);
    if (hsvc)
    {
        if(StartServiceW(hsvc, num, params))
            r = ERROR_SUCCESS;
        else
            r = GetLastError();
        if (r == ERROR_SERVICE_ALREADY_RUNNING)
            r = ERROR_SUCCESS;
        CloseServiceHandle(hsvc);
    }
    else
        r = GetLastError();
    CloseServiceHandle(handle);

    TRACE("StartService returned error %u (%s)\n", r, (r == ERROR_SUCCESS) ? "ok":"failed");

    return r;
}

/*
 * create_local_service()  - start a COM server in a service
 *
 *   To start a Local Service, we read the AppID value under
 * the class's CLSID key, then open the HKCR\\AppId key specified
 * there and check for a LocalService value.
 *
 * Note:  Local Services are not supported under Windows 9x
 */
static HRESULT create_local_service(REFCLSID rclsid)
{
    HRESULT hres;
    WCHAR buf[CHARS_IN_GUID];
    static const WCHAR szLocalService[] = { 'L','o','c','a','l','S','e','r','v','i','c','e',0 };
    static const WCHAR szServiceParams[] = {'S','e','r','v','i','c','e','P','a','r','a','m','s',0};
    HKEY hkey;
    LONG r;
    DWORD type, sz;

    TRACE("Attempting to start Local service for %s\n", debugstr_guid(rclsid));

    hres = COM_OpenKeyForAppIdFromCLSID(rclsid, KEY_READ, &hkey);
    if (FAILED(hres))
        return hres;

    /* read the LocalService and ServiceParameters values from the AppID key */
    sz = sizeof buf;
    r = RegQueryValueExW(hkey, szLocalService, NULL, &type, (LPBYTE)buf, &sz);
    if (r==ERROR_SUCCESS && type==REG_SZ)
    {
        DWORD num_args = 0;
        LPWSTR args[1] = { NULL };

        /*
         * FIXME: I'm not really sure how to deal with the service parameters.
         *        I suspect that the string returned from RegQueryValueExW
         *        should be split into a number of arguments by spaces.
         *        It would make more sense if ServiceParams contained a
         *        REG_MULTI_SZ here, but it's a REG_SZ for the services
         *        that I'm interested in for the moment.
         */
        r = RegQueryValueExW(hkey, szServiceParams, NULL, &type, NULL, &sz);
        if (r == ERROR_SUCCESS && type == REG_SZ && sz)
        {
            args[0] = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz);
            num_args++;
            RegQueryValueExW(hkey, szServiceParams, NULL, &type, (LPBYTE)args[0], &sz);
        }
        r = start_local_service(buf, num_args, (LPCWSTR *)args);
        if (r != ERROR_SUCCESS)
            hres = REGDB_E_CLASSNOTREG; /* FIXME: check retval */
        HeapFree(GetProcessHeap(),0,args[0]);
    }
    else
    {
        WARN("No LocalService value\n");
        hres = REGDB_E_CLASSNOTREG; /* FIXME: check retval */
    }
    RegCloseKey(hkey);

    return hres;
}


static void get_localserver_pipe_name(WCHAR *pipefn, REFCLSID rclsid)
{
    static const WCHAR wszPipeRef[] = {'\\','\\','.','\\','p','i','p','e','\\',0};
    strcpyW(pipefn, wszPipeRef);
    StringFromGUID2(rclsid, pipefn + sizeof(wszPipeRef)/sizeof(wszPipeRef[0]) - 1, CHARS_IN_GUID);
}

/* FIXME: should call to rpcss instead */
HRESULT RPC_GetLocalClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    HRESULT        hres;
    HANDLE         hPipe;
    WCHAR          pipefn[100];
    DWORD          res, bufferlen;
    char           marshalbuffer[200];
    IStream       *pStm;
    LARGE_INTEGER  seekto;
    ULARGE_INTEGER newpos;
    int            tries = 0;

    static const int MAXTRIES = 30; /* 30 seconds */

    TRACE("rclsid=%s, iid=%s\n", debugstr_guid(rclsid), debugstr_guid(iid));

    get_localserver_pipe_name(pipefn, rclsid);

    while (tries++ < MAXTRIES) {
        TRACE("waiting for %s\n", debugstr_w(pipefn));

        WaitNamedPipeW( pipefn, NMPWAIT_WAIT_FOREVER );
        hPipe = CreateFileW(pipefn, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
        if (hPipe == INVALID_HANDLE_VALUE) {
            DWORD index;
            DWORD start_ticks;
            if (tries == 1) {
                if ( (hres = create_local_service(rclsid)) &&
                     (hres = create_server(rclsid)) )
                    return hres;
            } else {
                WARN("Connecting to %s, no response yet, retrying: le is %u\n", debugstr_w(pipefn), GetLastError());
            }
            /* wait for one second, even if messages arrive */
            start_ticks = GetTickCount();
            do {
                CoWaitForMultipleHandles(0, 1000, 0, NULL, &index);
            } while (GetTickCount() - start_ticks < 1000);
            continue;
        }
        bufferlen = 0;
        if (!ReadFile(hPipe,marshalbuffer,sizeof(marshalbuffer),&bufferlen,NULL)) {
            FIXME("Failed to read marshal id from classfactory of %s.\n",debugstr_guid(rclsid));
            Sleep(1000);
            continue;
        }
        TRACE("read marshal id from pipe\n");
        CloseHandle(hPipe);
        break;
    }
    
    if (tries >= MAXTRIES)
        return E_NOINTERFACE;
    
    hres = CreateStreamOnHGlobal(0,TRUE,&pStm);
    if (hres) return hres;
    hres = IStream_Write(pStm,marshalbuffer,bufferlen,&res);
    if (hres) goto out;
    seekto.u.LowPart = 0;seekto.u.HighPart = 0;
    hres = IStream_Seek(pStm,seekto,STREAM_SEEK_SET,&newpos);
    
    TRACE("unmarshalling classfactory\n");
    hres = CoUnmarshalInterface(pStm,&IID_IClassFactory,ppv);
out:
    IStream_Release(pStm);
    return hres;
}


struct local_server_params
{
    CLSID clsid;
    IStream *stream;
    HANDLE ready_event;
    HANDLE stop_event;
    HANDLE thread;
    BOOL multi_use;
};

/* FIXME: should call to rpcss instead */
static DWORD WINAPI local_server_thread(LPVOID param)
{
    struct local_server_params * lsp = param;
    HANDLE		hPipe;
    WCHAR 		pipefn[100];
    HRESULT		hres;
    IStream		*pStm = lsp->stream;
    STATSTG		ststg;
    unsigned char	*buffer;
    int 		buflen;
    LARGE_INTEGER	seekto;
    ULARGE_INTEGER	newpos;
    ULONG		res;
    BOOL multi_use = lsp->multi_use;
    OVERLAPPED ovl;
    HANDLE pipe_event;
    DWORD  bytes;

    TRACE("Starting threader for %s.\n",debugstr_guid(&lsp->clsid));

    memset(&ovl, 0, sizeof(ovl));
    get_localserver_pipe_name(pipefn, &lsp->clsid);

    hPipe = CreateNamedPipeW( pipefn, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                              PIPE_TYPE_BYTE|PIPE_WAIT, PIPE_UNLIMITED_INSTANCES,
                              4096, 4096, 500 /* 0.5 second timeout */, NULL );

    SetEvent(lsp->ready_event);

    if (hPipe == INVALID_HANDLE_VALUE)
    {
        FIXME("pipe creation failed for %s, le is %u\n", debugstr_w(pipefn), GetLastError());
        return 1;
    }

    ovl.hEvent = pipe_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    
    while (1) {
        if (!ConnectNamedPipe(hPipe, &ovl))
        {
            DWORD error = GetLastError();
            if (error == ERROR_IO_PENDING)
            {
                HANDLE handles[2] = { pipe_event, lsp->stop_event };
                DWORD ret;
                ret = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
                if (ret != WAIT_OBJECT_0)
                    break;
            }
            /* client already connected isn't an error */
            else if (error != ERROR_PIPE_CONNECTED)
            {
                ERR("ConnectNamedPipe failed with error %d\n", GetLastError());
                break;
            }
        }

        TRACE("marshalling IClassFactory to client\n");
        
        hres = IStream_Stat(pStm,&ststg,STATFLAG_NONAME);
        if (hres) return hres;

        seekto.u.LowPart = 0;
        seekto.u.HighPart = 0;
        hres = IStream_Seek(pStm,seekto,STREAM_SEEK_SET,&newpos);
        if (hres) {
            FIXME("IStream_Seek failed, %x\n",hres);
            CloseHandle(hPipe);
            CloseHandle(pipe_event);
            return hres;
        }

        buflen = ststg.cbSize.u.LowPart;
        buffer = HeapAlloc(GetProcessHeap(),0,buflen);
        
        hres = IStream_Read(pStm,buffer,buflen,&res);
        if (hres) {
            FIXME("Stream Read failed, %x\n",hres);
            CloseHandle(hPipe);
            CloseHandle(pipe_event);
            HeapFree(GetProcessHeap(),0,buffer);
            return hres;
        }
        
        WriteFile(hPipe,buffer,buflen,&res,&ovl);
        GetOverlappedResult(hPipe, &ovl, &bytes, TRUE);
        HeapFree(GetProcessHeap(),0,buffer);

        FlushFileBuffers(hPipe);
        DisconnectNamedPipe(hPipe);

        TRACE("done marshalling IClassFactory\n");

        if (!multi_use)
        {
            TRACE("single use object, shutting down pipe %s\n", debugstr_w(pipefn));
            break;
        }
    }
    CloseHandle(hPipe);
    CloseHandle(pipe_event);
    return 0;
}

/* starts listening for a local server */
HRESULT RPC_StartLocalServer(REFCLSID clsid, IStream *stream, BOOL multi_use, void **registration)
{
    DWORD tid;
    struct local_server_params *lsp;

    lsp = HeapAlloc(GetProcessHeap(), 0, sizeof(*lsp));
    if (!lsp)
        return E_OUTOFMEMORY;

    lsp->clsid = *clsid;
    lsp->stream = stream;
    IStream_AddRef(stream);
    lsp->ready_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!lsp->ready_event)
    {
        HeapFree(GetProcessHeap(), 0, lsp);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    lsp->stop_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!lsp->stop_event)
    {
        CloseHandle(lsp->ready_event);
        HeapFree(GetProcessHeap(), 0, lsp);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    lsp->multi_use = multi_use;

    lsp->thread = CreateThread(NULL, 0, local_server_thread, lsp, 0, &tid);
    if (!lsp->thread)
    {
        CloseHandle(lsp->ready_event);
        CloseHandle(lsp->stop_event);
        HeapFree(GetProcessHeap(), 0, lsp);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    WaitForSingleObject(lsp->ready_event, INFINITE);
    CloseHandle(lsp->ready_event);
    lsp->ready_event = NULL;

    *registration = lsp;
    return S_OK;
}

/* stops listening for a local server */
void RPC_StopLocalServer(void *registration)
{
    struct local_server_params *lsp = registration;

    /* signal local_server_thread to stop */
    SetEvent(lsp->stop_event);
    /* wait for it to exit */
    WaitForSingleObject(lsp->thread, INFINITE);

    IStream_Release(lsp->stream);
    CloseHandle(lsp->stop_event);
    CloseHandle(lsp->thread);
    HeapFree(GetProcessHeap(), 0, lsp);
}
