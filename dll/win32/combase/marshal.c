/*
 *     Copyright 2002 Juergen Schmied
 *     Copyright 2002 Marcus Meissner
 *     Copyright 2004 Mike Hearn, for CodeWeavers
 *     Copyright 2004 Rob Shearman, for CodeWeavers
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

#include <assert.h>

#define COBJMACROS
#include "objbase.h"

#include "dcom.h"
#include "combase_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

HRESULT WINAPI RPC_CreateClientChannel(const OXID *oxid, const IPID *ipid,
                                const OXID_INFO *oxid_info, const IID *iid,
                                DWORD dest_context, void *dest_context_data,
                                IRpcChannelBuffer **chan, struct apartment *apt);

static HRESULT unmarshal_object(const STDOBJREF *stdobjref, struct apartment *apt,
                                MSHCTX dest_context, void *dest_context_data,
                                REFIID riid, const OXID_INFO *oxid_info,
                                void **object);

/* number of refs given out for normal marshaling */
#define NORMALEXTREFS 5

/* private flag indicating that the object was marshaled as table-weak */
#define SORFP_TABLEWEAK SORF_OXRES1
/* private flag indicating that the caller does not want to notify the stub
 * when the proxy disconnects or is destroyed */
#define SORFP_NOLIFETIMEMGMT SORF_OXRES2

/* imported interface proxy */
struct ifproxy
{
    struct list entry;       /* entry in proxy_manager list (CS parent->cs) */
    struct proxy_manager *parent; /* owning proxy_manager (RO) */
    void *iface;             /* interface pointer (RO) */
    STDOBJREF stdobjref;     /* marshal data that represents this object (RO) */
    IID iid;                 /* interface ID (RO) */
    IRpcProxyBuffer *proxy;  /* interface proxy (RO) */
    ULONG refs;              /* imported (public) references (LOCK) */
    IRpcChannelBuffer *chan; /* channel to object (CS parent->cs) */
};

/* imported object / proxy manager */
struct proxy_manager
{
    IMultiQI IMultiQI_iface;
    IMarshal IMarshal_iface;
    IClientSecurity IClientSecurity_iface;
    struct apartment *parent; /* owning apartment (RO) */
    struct list entry;        /* entry in apartment (CS parent->cs) */
    OXID oxid;                /* object exported ID (RO) */
    OXID_INFO oxid_info;      /* string binding, ipid of rem unknown and other information (RO) */
    OID oid;                  /* object ID (RO) */
    struct list interfaces;   /* imported interfaces (CS cs) */
    LONG refs;                /* proxy reference count (LOCK); 0 if about to be removed from list */
    CRITICAL_SECTION cs;      /* thread safety for this object and children */
    ULONG sorflags;           /* STDOBJREF flags (RO) */
    IRemUnknown *remunk;      /* proxy to IRemUnknown used for lifecycle management (CS cs) */
    HANDLE remoting_mutex;    /* mutex used for synchronizing access to IRemUnknown */
    MSHCTX dest_context;      /* context used for activating optimisations (LOCK) */
    void *dest_context_data;  /* reserved context value (LOCK) */
};

static inline struct proxy_manager *impl_from_IMultiQI(IMultiQI *iface)
{
    return CONTAINING_RECORD(iface, struct proxy_manager, IMultiQI_iface);
}

static inline struct proxy_manager *impl_from_IMarshal(IMarshal *iface)
{
    return CONTAINING_RECORD(iface, struct proxy_manager, IMarshal_iface);
}

static inline struct proxy_manager *impl_from_IClientSecurity(IClientSecurity *iface)
{
    return CONTAINING_RECORD(iface, struct proxy_manager, IClientSecurity_iface);
}

struct ftmarshaler
{
    IUnknown IUnknown_inner;
    IMarshal IMarshal_iface;
    IUnknown *outer_unk;
    LONG refcount;
};

static struct ftmarshaler *impl_ft_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct ftmarshaler, IUnknown_inner);
}

static struct ftmarshaler *impl_ft_from_IMarshal(IMarshal *iface)
{
    return CONTAINING_RECORD(iface, struct ftmarshaler, IMarshal_iface);
}

/***********************************************************************
 *            CoMarshalHresult        (combase.@)
 */
HRESULT WINAPI CoMarshalHresult(IStream *stream, HRESULT hresult)
{
    return IStream_Write(stream, &hresult, sizeof(hresult), NULL);
}

/***********************************************************************
 *            CoUnmarshalHresult      (combase.@)
 */
HRESULT WINAPI CoUnmarshalHresult(IStream *stream, HRESULT *phresult)
{
    return IStream_Read(stream, phresult, sizeof(*phresult), NULL);
}

/***********************************************************************
 *            CoGetInterfaceAndReleaseStream    (combase.@)
 */
HRESULT WINAPI CoGetInterfaceAndReleaseStream(IStream *stream, REFIID riid, void **obj)
{
    HRESULT hr;

    TRACE("%p, %s, %p\n", stream, debugstr_guid(riid), obj);

    if (!stream) return E_INVALIDARG;
    hr = CoUnmarshalInterface(stream, riid, obj);
    IStream_Release(stream);
    return hr;
}

/***********************************************************************
 *            CoMarshalInterThreadInterfaceInStream    (combase.@)
 */
HRESULT WINAPI CoMarshalInterThreadInterfaceInStream(REFIID riid, IUnknown *unk, IStream **stream)
{
    ULARGE_INTEGER xpos;
    LARGE_INTEGER seekto;
    HRESULT hr;

    TRACE("%s, %p, %p\n", debugstr_guid(riid), unk, stream);

    hr = CreateStreamOnHGlobal(NULL, TRUE, stream);
    if (FAILED(hr)) return hr;
    hr = CoMarshalInterface(*stream, riid, unk, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);

    if (SUCCEEDED(hr))
    {
        memset(&seekto, 0, sizeof(seekto));
        IStream_Seek(*stream, seekto, STREAM_SEEK_SET, &xpos);
    }
    else
    {
        IStream_Release(*stream);
        *stream = NULL;
    }

    return hr;
}

static HRESULT WINAPI ftmarshaler_inner_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
{
    struct ftmarshaler *marshaler = impl_ft_from_IUnknown(iface);

    TRACE("%p, %s, %p\n", iface, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualIID(&IID_IUnknown, riid))
        *obj = &marshaler->IUnknown_inner;
    else if (IsEqualIID(&IID_IMarshal, riid))
        *obj = &marshaler->IMarshal_iface;
    else
    {
        FIXME("No interface for %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);

    return S_OK;
}

static ULONG WINAPI ftmarshaler_inner_AddRef(IUnknown *iface)
{
    struct ftmarshaler *marshaler = impl_ft_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&marshaler->refcount);

    TRACE("%p, refcount %lu\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI ftmarshaler_inner_Release(IUnknown *iface)
{
    struct ftmarshaler *marshaler = impl_ft_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&marshaler->refcount);

    TRACE("%p, refcount %lu\n", iface, refcount);

    if (!refcount)
        free(marshaler);

    return refcount;
}

static const IUnknownVtbl ftmarshaler_inner_vtbl =
{
    ftmarshaler_inner_QueryInterface,
    ftmarshaler_inner_AddRef,
    ftmarshaler_inner_Release
};

static HRESULT WINAPI ftmarshaler_QueryInterface(IMarshal *iface, REFIID riid, void **obj)
{
    struct ftmarshaler *marshaler = impl_ft_from_IMarshal(iface);

    TRACE("%p, %s, %p\n", iface, debugstr_guid(riid), obj);

    return IUnknown_QueryInterface(marshaler->outer_unk, riid, obj);
}

static ULONG WINAPI ftmarshaler_AddRef(IMarshal *iface)
{
    struct ftmarshaler *marshaler = impl_ft_from_IMarshal(iface);

    TRACE("%p\n", iface);

    return IUnknown_AddRef(marshaler->outer_unk);
}

static ULONG WINAPI ftmarshaler_Release(IMarshal *iface)
{
    struct ftmarshaler *marshaler = impl_ft_from_IMarshal(iface);

    TRACE("%p\n", iface);

    return IUnknown_Release(marshaler->outer_unk);
}

static HRESULT WINAPI ftmarshaler_GetUnmarshalClass(IMarshal *iface, REFIID riid, void *pv,
        DWORD dest_context, void *pvDestContext, DWORD mshlflags, CLSID *clsid)
{
    TRACE("%s, %p, %#lx, %p, %#lx, %p\n", debugstr_guid(riid), pv, dest_context, pvDestContext, mshlflags, clsid);

    if (dest_context == MSHCTX_INPROC || dest_context == MSHCTX_CROSSCTX)
        *clsid = CLSID_InProcFreeMarshaler;
    else
        *clsid = CLSID_StdMarshal;

    return S_OK;
}

union ftmarshaler_data
{
    UINT64 value;
    IUnknown *object;
};

static HRESULT WINAPI ftmarshaler_GetMarshalSizeMax(IMarshal *iface, REFIID riid, void *pv,
        DWORD dest_context, void *pvDestContext, DWORD mshlflags, DWORD *size)
{
    IMarshal *marshal = NULL;
    HRESULT hr;

    TRACE("%s, %p, %#lx, %p, %#lx, %p\n", debugstr_guid(riid), pv, dest_context, pvDestContext, mshlflags, size);

    /* If the marshalling happens inside the same process the interface pointer is
       copied between the apartments */
    if (dest_context == MSHCTX_INPROC || dest_context == MSHCTX_CROSSCTX)
    {
        *size = sizeof(mshlflags) + sizeof(union ftmarshaler_data) + sizeof(GUID);
        return S_OK;
    }

    /* Use the standard marshaller to handle all other cases */
    CoGetStandardMarshal(riid, pv, dest_context, pvDestContext, mshlflags, &marshal);
    hr = IMarshal_GetMarshalSizeMax(marshal, riid, pv, dest_context, pvDestContext, mshlflags, size);
    IMarshal_Release(marshal);
    return hr;
}

static HRESULT WINAPI ftmarshaler_MarshalInterface(IMarshal *iface, IStream *stream, REFIID riid,
        void *pv, DWORD dest_context, void *pvDestContext, DWORD mshlflags)
{
    IMarshal *marshal = NULL;
    HRESULT hr;

    TRACE("%p, %s, %p, %#lx, %p, %#lx\n", stream, debugstr_guid(riid), pv,
            dest_context, pvDestContext, mshlflags);

    /* If the marshalling happens inside the same process the interface pointer is
       copied between the apartments */
    if (dest_context == MSHCTX_INPROC || dest_context == MSHCTX_CROSSCTX)
    {
        union ftmarshaler_data data;
        GUID unknown_guid = { 0 };

        data.value = 0;
        hr = IUnknown_QueryInterface((IUnknown *)pv, riid, (void **)&data.object);
        if (FAILED(hr))
            return hr;

        /* don't hold a reference to table-weak marshaled interfaces */
        if (mshlflags & MSHLFLAGS_TABLEWEAK)
            IUnknown_Release(data.object);

        hr = IStream_Write(stream, &mshlflags, sizeof(mshlflags), NULL);
        if (hr != S_OK) return STG_E_MEDIUMFULL;

        hr = IStream_Write(stream, &data, sizeof(data), NULL);
        if (hr != S_OK) return STG_E_MEDIUMFULL;

        hr = IStream_Write(stream, &unknown_guid, sizeof(unknown_guid), NULL);
        if (hr != S_OK) return STG_E_MEDIUMFULL;

        return S_OK;
    }

    /* Use the standard marshaler to handle all other cases */
    CoGetStandardMarshal(riid, pv, dest_context, pvDestContext, mshlflags, &marshal);
    hr = IMarshal_MarshalInterface(marshal, stream, riid, pv, dest_context, pvDestContext, mshlflags);
    IMarshal_Release(marshal);
    return hr;
}

static HRESULT WINAPI ftmarshaler_UnmarshalInterface(IMarshal *iface, IStream *stream, REFIID riid, void **ppv)
{
    union ftmarshaler_data data;
    DWORD mshlflags;
    GUID unknown_guid;
    HRESULT hr;

    TRACE("%p, %s, %p\n", stream, debugstr_guid(riid), ppv);

    hr = IStream_Read(stream, &mshlflags, sizeof(mshlflags), NULL);
    if (hr != S_OK) return STG_E_READFAULT;

    hr = IStream_Read(stream, &data, sizeof(data), NULL);
    if (hr != S_OK) return STG_E_READFAULT;

    hr = IStream_Read(stream, &unknown_guid, sizeof(unknown_guid), NULL);
    if (hr != S_OK) return STG_E_READFAULT;

    hr = IUnknown_QueryInterface(data.object, riid, ppv);
    if (!(mshlflags & (MSHLFLAGS_TABLEWEAK | MSHLFLAGS_TABLESTRONG)))
        IUnknown_Release(data.object);

    return hr;
}

static HRESULT WINAPI ftmarshaler_ReleaseMarshalData(IMarshal *iface, IStream *stream)
{
    union ftmarshaler_data data;
    DWORD mshlflags;
    GUID unknown_guid;
    HRESULT hr;

    TRACE("%p\n", stream);

    hr = IStream_Read(stream, &mshlflags, sizeof(mshlflags), NULL);
    if (hr != S_OK) return STG_E_READFAULT;

    hr = IStream_Read(stream, &data, sizeof(data), NULL);
    if (hr != S_OK) return STG_E_READFAULT;

    hr = IStream_Read(stream, &unknown_guid, sizeof(unknown_guid), NULL);
    if (hr != S_OK) return STG_E_READFAULT;

    IUnknown_Release(data.object);
    return S_OK;
}

static HRESULT WINAPI ftmarshaler_DisconnectObject(IMarshal *iface, DWORD reserved)
{
    TRACE("\n");

    /* nothing to do */
    return S_OK;
}

static const IMarshalVtbl ftmarshaler_vtbl =
{
    ftmarshaler_QueryInterface,
    ftmarshaler_AddRef,
    ftmarshaler_Release,
    ftmarshaler_GetUnmarshalClass,
    ftmarshaler_GetMarshalSizeMax,
    ftmarshaler_MarshalInterface,
    ftmarshaler_UnmarshalInterface,
    ftmarshaler_ReleaseMarshalData,
    ftmarshaler_DisconnectObject
};

/***********************************************************************
 *          CoCreateFreeThreadedMarshaler    (combase.@)
 */
HRESULT WINAPI CoCreateFreeThreadedMarshaler(IUnknown *outer, IUnknown **marshaler)
{
    struct ftmarshaler *object;

    TRACE("%p, %p\n", outer, marshaler);

    object = malloc(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IUnknown_inner.lpVtbl = &ftmarshaler_inner_vtbl;
    object->IMarshal_iface.lpVtbl = &ftmarshaler_vtbl;
    object->refcount = 1;
    object->outer_unk = outer ? outer : &object->IUnknown_inner;

    *marshaler = &object->IUnknown_inner;

    return S_OK;
}

/***********************************************************************
 *            CoGetMarshalSizeMax        (combase.@)
 */
HRESULT WINAPI CoGetMarshalSizeMax(ULONG *size, REFIID riid, IUnknown *unk,
         DWORD dest_context, void *pvDestContext, DWORD mshlFlags)
{
    BOOL std_marshal = FALSE;
    IMarshal *marshal;
    HRESULT hr;

    if (!unk)
        return E_POINTER;

    hr = IUnknown_QueryInterface(unk, &IID_IMarshal, (void **)&marshal);
    if (hr != S_OK)
    {
        std_marshal = TRUE;
        hr = CoGetStandardMarshal(riid, unk, dest_context, pvDestContext, mshlFlags, &marshal);
    }
    if (hr != S_OK)
        return hr;

    hr = IMarshal_GetMarshalSizeMax(marshal, riid, unk, dest_context, pvDestContext, mshlFlags, size);
    if (!std_marshal)
        /* add on the size of the whole OBJREF structure like native does */
        *size += sizeof(OBJREF);

    IMarshal_Release(marshal);
    return hr;
}

static void dump_mshflags(MSHLFLAGS flags)
{
    if (flags & MSHLFLAGS_TABLESTRONG)
        TRACE(" MSHLFLAGS_TABLESTRONG");
    if (flags & MSHLFLAGS_TABLEWEAK)
        TRACE(" MSHLFLAGS_TABLEWEAK");
    if (!(flags & (MSHLFLAGS_TABLESTRONG|MSHLFLAGS_TABLEWEAK)))
        TRACE(" MSHLFLAGS_NORMAL");
    if (flags & MSHLFLAGS_NOPING)
        TRACE(" MSHLFLAGS_NOPING");
}

/***********************************************************************
 *        CoMarshalInterface    (combase.@)
 */
HRESULT WINAPI CoMarshalInterface(IStream *stream, REFIID riid, IUnknown *unk,
        DWORD dest_context, void *pvDestContext, DWORD mshlFlags)
{
    CLSID marshaler_clsid;
    IMarshal *marshal;
    HRESULT hr;

    TRACE("%p, %s, %p, %lx, %p, ", stream, debugstr_guid(riid), unk, dest_context, pvDestContext);
    dump_mshflags(mshlFlags);
    TRACE("\n");

    if (!unk || !stream)
        return E_INVALIDARG;

    hr = IUnknown_QueryInterface(unk, &IID_IMarshal, (void **)&marshal);
    if (hr != S_OK)
        hr = CoGetStandardMarshal(riid, unk, dest_context, pvDestContext, mshlFlags, &marshal);
    if (hr != S_OK)
    {
        ERR("Failed to get marshaller, %#lx\n", hr);
        return hr;
    }

    hr = IMarshal_GetUnmarshalClass(marshal, riid, unk, dest_context, pvDestContext, mshlFlags,
            &marshaler_clsid);
    if (hr != S_OK)
    {
        ERR("IMarshal::GetUnmarshalClass failed, %#lx\n", hr);
        goto cleanup;
    }

    /* FIXME: implement handler marshaling too */
    if (IsEqualCLSID(&marshaler_clsid, &CLSID_StdMarshal))
    {
        TRACE("Using standard marshaling\n");
    }
    else
    {
        OBJREF objref;

        TRACE("Using custom marshaling\n");
        objref.signature = OBJREF_SIGNATURE;
        objref.iid = *riid;
        objref.flags = OBJREF_CUSTOM;
        objref.u_objref.u_custom.clsid = marshaler_clsid;
        objref.u_objref.u_custom.cbExtension = 0;
        objref.u_objref.u_custom.size = 0;
        hr = IMarshal_GetMarshalSizeMax(marshal, riid, unk, dest_context, pvDestContext, mshlFlags,
                &objref.u_objref.u_custom.size);
        if (hr != S_OK)
        {
            ERR("Failed to get max size of marshal data, error %#lx\n", hr);
            goto cleanup;
        }
        /* write constant sized common header and OR_CUSTOM data into stream */
        hr = IStream_Write(stream, &objref, FIELD_OFFSET(OBJREF, u_objref.u_custom.pData), NULL);
        if (hr != S_OK)
        {
            ERR("Failed to write OR_CUSTOM header to stream with %#lx\n", hr);
            goto cleanup;
        }
    }

    TRACE("Calling IMarshal::MarshalInterface\n");

    hr = IMarshal_MarshalInterface(marshal, stream, riid, unk, dest_context, pvDestContext, mshlFlags);
    if (hr != S_OK)
    {
        ERR("Failed to marshal the interface %s, hr %#lx\n", debugstr_guid(riid), hr);
        goto cleanup;
    }

cleanup:
    IMarshal_Release(marshal);

    TRACE("completed with hr %#lx\n", hr);

    return hr;
}

/* Creates an IMarshal* object according to the data marshaled to the stream.
 * The function leaves the stream pointer at the start of the data written
 * to the stream by the IMarshal* object.
 */
static HRESULT get_unmarshaler_from_stream(IStream *stream, IMarshal **marshal, IID *iid)
{
    OBJREF objref;
    HRESULT hr;
    ULONG res;

    /* read common OBJREF header */
    hr = IStream_Read(stream, &objref, FIELD_OFFSET(OBJREF, u_objref), &res);
    if (hr != S_OK || (res != FIELD_OFFSET(OBJREF, u_objref)))
    {
        ERR("Failed to read common OBJREF header, %#lx\n", hr);
        return STG_E_READFAULT;
    }

    /* sanity check on header */
    if (objref.signature != OBJREF_SIGNATURE)
    {
        ERR("Bad OBJREF signature %#lx\n", objref.signature);
        return RPC_E_INVALID_OBJREF;
    }

    if (iid) *iid = objref.iid;

    /* FIXME: handler marshaling */
    if (objref.flags & OBJREF_STANDARD)
    {
        TRACE("Using standard unmarshaling\n");
        *marshal = NULL;
        return S_FALSE;
    }
    else if (objref.flags & OBJREF_CUSTOM)
    {
        ULONG custom_header_size = FIELD_OFFSET(OBJREF, u_objref.u_custom.pData) -
                                   FIELD_OFFSET(OBJREF, u_objref.u_custom);
        TRACE("Using custom unmarshaling\n");
        /* read constant sized OR_CUSTOM data from stream */
        hr = IStream_Read(stream, &objref.u_objref.u_custom,
                          custom_header_size, &res);
        if (hr != S_OK || (res != custom_header_size))
        {
            ERR("Failed to read OR_CUSTOM header, %#lx\n", hr);
            return STG_E_READFAULT;
        }
        /* now create the marshaler specified in the stream */
        hr = CoCreateInstance(&objref.u_objref.u_custom.clsid, NULL,
                              CLSCTX_INPROC_SERVER, &IID_IMarshal,
                              (LPVOID*)marshal);
    }
    else
    {
        FIXME("Invalid or unimplemented marshaling type specified: %lx\n", objref.flags);
        return RPC_E_INVALID_OBJREF;
    }

    if (hr != S_OK)
        ERR("Failed to create marshal, %#lx\n", hr);

    return hr;
}

static HRESULT std_release_marshal_data(IStream *stream)
{
    struct stub_manager *stubmgr;
    struct OR_STANDARD  obj;
    struct apartment *apt;
    ULONG res;
    HRESULT hr;

    hr = IStream_Read(stream, &obj, FIELD_OFFSET(struct OR_STANDARD, saResAddr.aStringArray), &res);
    if (hr != S_OK) return STG_E_READFAULT;

    if (obj.saResAddr.wNumEntries)
    {
        ERR("unsupported size of DUALSTRINGARRAY\n");
        return E_NOTIMPL;
    }

    TRACE("oxid = %s, oid = %s, ipid = %s\n", wine_dbgstr_longlong(obj.std.oxid),
            wine_dbgstr_longlong(obj.std.oid), wine_dbgstr_guid(&obj.std.ipid));

    if (!(apt = apartment_findfromoxid(obj.std.oxid)))
    {
        WARN("Could not map OXID %s to apartment object\n",
            wine_dbgstr_longlong(obj.std.oxid));
        return RPC_E_INVALID_OBJREF;
    }

    if (!(stubmgr = get_stub_manager(apt, obj.std.oid)))
    {
        apartment_release(apt);
        ERR("could not map object ID to stub manager, oxid=%s, oid=%s\n",
            wine_dbgstr_longlong(obj.std.oxid), wine_dbgstr_longlong(obj.std.oid));
        return RPC_E_INVALID_OBJREF;
    }

    stub_manager_release_marshal_data(stubmgr, obj.std.cPublicRefs, &obj.std.ipid, obj.std.flags & SORFP_TABLEWEAK);

    stub_manager_int_release(stubmgr);
    apartment_release(apt);

    return S_OK;
}

/***********************************************************************
 *            CoReleaseMarshalData        (combase.@)
 */
HRESULT WINAPI CoReleaseMarshalData(IStream *stream)
{
    IMarshal *marshal;
    HRESULT hr;

    TRACE("%p\n", stream);

    hr = get_unmarshaler_from_stream(stream, &marshal, NULL);
    if (hr == S_FALSE)
    {
        hr = std_release_marshal_data(stream);
        if (hr != S_OK)
            ERR("StdMarshal ReleaseMarshalData failed with error %#lx\n", hr);
        return hr;
    }
    if (hr != S_OK)
        return hr;

    /* call the helper object to do the releasing of marshal data */
    hr = IMarshal_ReleaseMarshalData(marshal, stream);
    if (hr != S_OK)
        ERR("IMarshal::ReleaseMarshalData failed with error %#lx\n", hr);

    IMarshal_Release(marshal);
    return hr;
}

static HRESULT std_unmarshal_interface(MSHCTX dest_context, void *dest_context_data,
        IStream *stream, REFIID riid, void **ppv, BOOL dest_context_known)
{
    struct stub_manager *stubmgr = NULL;
    struct OR_STANDARD obj;
    ULONG res;
    HRESULT hres;
    struct apartment *apt, *stub_apt;

    TRACE("(...,%s,....)\n", debugstr_guid(riid));

    /* we need an apartment to unmarshal into */
    if (!(apt = apartment_get_current_or_mta()))
    {
        ERR("Apartment not initialized\n");
        return CO_E_NOTINITIALIZED;
    }

    /* read STDOBJREF from wire */
    hres = IStream_Read(stream, &obj, FIELD_OFFSET(struct OR_STANDARD, saResAddr.aStringArray), &res);
    if (hres != S_OK)
    {
        apartment_release(apt);
        return STG_E_READFAULT;
    }

    if (obj.saResAddr.wNumEntries)
    {
        ERR("unsupported size of DUALSTRINGARRAY\n");
        return E_NOTIMPL;
    }

    /* check if we're marshalling back to ourselves */
    if ((apartment_getoxid(apt) == obj.std.oxid) && (stubmgr = get_stub_manager(apt, obj.std.oid)))
    {
        TRACE("Unmarshalling object marshalled in same apartment for iid %s, "
              "returning original object %p\n", debugstr_guid(riid), stubmgr->object);

        hres = IUnknown_QueryInterface(stubmgr->object, riid, ppv);

        /* unref the ifstub. FIXME: only do this on success? */
        if (!stub_manager_is_table_marshaled(stubmgr, &obj.std.ipid))
            stub_manager_ext_release(stubmgr, obj.std.cPublicRefs, obj.std.flags & SORFP_TABLEWEAK, FALSE);

        stub_manager_int_release(stubmgr);
        apartment_release(apt);
        return hres;
    }

    /* notify stub manager about unmarshal if process-local object.
     * note: if the oxid is not found then we and native will quite happily
     * ignore table marshaling and normal marshaling rules regarding number of
     * unmarshals, etc, but if you abuse these rules then your proxy could end
     * up returning RPC_E_DISCONNECTED. */
    if ((stub_apt = apartment_findfromoxid(obj.std.oxid)))
    {
        if ((stubmgr = get_stub_manager(stub_apt, obj.std.oid)))
        {
            if (!stub_manager_notify_unmarshal(stubmgr, &obj.std.ipid))
                hres = CO_E_OBJNOTCONNECTED;
            if (SUCCEEDED(hres) && !dest_context_known)
                hres = ipid_get_dest_context(&obj.std.ipid, &dest_context, &dest_context_data);
        }
        else
        {
            WARN("Couldn't find object for OXID %s, OID %s, assuming disconnected\n",
                wine_dbgstr_longlong(obj.std.oxid),
                wine_dbgstr_longlong(obj.std.oid));
            hres = CO_E_OBJNOTCONNECTED;
        }
    }
    else
        TRACE("Treating unmarshal from OXID %s as inter-process\n",
            wine_dbgstr_longlong(obj.std.oxid));

    if (hres == S_OK)
        hres = unmarshal_object(&obj.std, apt, dest_context,
                                dest_context_data, riid,
                                stubmgr ? &stubmgr->oxid_info : NULL, ppv);

    if (stubmgr) stub_manager_int_release(stubmgr);
    if (stub_apt) apartment_release(stub_apt);

    if (hres != S_OK) WARN("Failed with error %#lx\n", hres);
    else TRACE("Successfully created proxy %p\n", *ppv);

    apartment_release(apt);
    return hres;
}

/***********************************************************************
 *            CoUnmarshalInterface        (combase.@)
 */
HRESULT WINAPI CoUnmarshalInterface(IStream *stream, REFIID riid, void **ppv)
{
    IMarshal *marshal;
    IUnknown *object;
    HRESULT hr;
    IID iid;

    TRACE("%p, %s, %p\n", stream, debugstr_guid(riid), ppv);

    if (!stream || !ppv)
        return E_INVALIDARG;

    hr = get_unmarshaler_from_stream(stream, &marshal, &iid);
    if (hr == S_FALSE)
    {
        hr = std_unmarshal_interface(0, NULL, stream, &iid, (void **)&object, FALSE);
        if (hr != S_OK)
            ERR("StdMarshal UnmarshalInterface failed, hr %#lx\n", hr);
    }
    else if (hr == S_OK)
    {
        /* call the helper object to do the actual unmarshaling */
        hr = IMarshal_UnmarshalInterface(marshal, stream, &iid, (void **)&object);
        IMarshal_Release(marshal);
        if (hr != S_OK)
            ERR("IMarshal::UnmarshalInterface failed, hr %#lx\n", hr);
    }

    if (hr == S_OK)
    {
        /* IID_NULL means use the interface ID of the marshaled object */
        if (!IsEqualIID(riid, &IID_NULL) && !IsEqualIID(riid, &iid))
        {
            TRACE("requested interface != marshalled interface, additional QI needed\n");
            hr = IUnknown_QueryInterface(object, riid, ppv);
            if (hr != S_OK)
                ERR("Couldn't query for interface %s, hr %#lx\n", debugstr_guid(riid), hr);
            IUnknown_Release(object);
        }
        else
        {
            *ppv = object;
        }
    }

    TRACE("completed with hr %#lx\n", hr);

    return hr;
}

/* Marshalling just passes a unique identifier to the remote client,
 * that makes it possible to find the passed interface again.
 *
 * So basically we need a set of values that make it unique.
 *
 * Note that the IUnknown_QI(ob,xiid,&ppv) always returns the SAME ppv value!
 *
 * A triple is used: OXID (apt id), OID (stub manager id),
 * IPID (interface ptr/stub id).
 *
 * OXIDs identify an apartment and are network scoped
 * OIDs identify a stub manager and are apartment scoped
 * IPIDs identify an interface stub and are apartment scoped
 */

static inline HRESULT get_facbuf_for_iid(REFIID riid, IPSFactoryBuffer **facbuf)
{
    HRESULT       hr;
    CLSID         clsid;

    hr = CoGetPSClsid(riid, &clsid);
    if (hr != S_OK)
        return hr;
    return CoGetClassObject(&clsid, CLSCTX_INPROC_SERVER | CLSCTX_PS_DLL, NULL, &IID_IPSFactoryBuffer, (void **)facbuf);
}

/* marshals an object into a STDOBJREF structure */
HRESULT marshal_object(struct apartment *apt, STDOBJREF *stdobjref, REFIID riid, IUnknown *object,
    DWORD dest_context, void *dest_context_data, MSHLFLAGS mshlflags)
{
    struct stub_manager *manager;
    struct ifstub       *ifstub;
    BOOL                 tablemarshal;
    HRESULT              hr;

    stdobjref->oxid = apartment_getoxid(apt);

    hr = apartment_createwindowifneeded(apt);
    if (hr != S_OK)
        return hr;

    if (!(manager = get_stub_manager_from_object(apt, object, TRUE)))
        return E_OUTOFMEMORY;

    stdobjref->flags = SORF_NULL;
    if (mshlflags & MSHLFLAGS_TABLEWEAK)
        stdobjref->flags |= SORFP_TABLEWEAK;
    if (mshlflags & MSHLFLAGS_NOPING)
        stdobjref->flags |= SORF_NOPING;
    stdobjref->oid = manager->oid;

    tablemarshal = ((mshlflags & MSHLFLAGS_TABLESTRONG) || (mshlflags & MSHLFLAGS_TABLEWEAK));

    /* make sure ifstub that we are creating is unique */
    ifstub = stub_manager_find_ifstub(manager, riid, mshlflags);
    if (!ifstub) {
        IRpcStubBuffer *stub = NULL;

        /* IUnknown doesn't require a stub buffer, because it never goes out on
         * the wire */
        if (!IsEqualIID(riid, &IID_IUnknown))
        {
            IPSFactoryBuffer *psfb;

            hr = get_facbuf_for_iid(riid, &psfb);
            if (hr == S_OK) {
                hr = IPSFactoryBuffer_CreateStub(psfb, riid, manager->object, &stub);
                IPSFactoryBuffer_Release(psfb);
                if (hr != S_OK)
                    ERR("Failed to create an IRpcStubBuffer from IPSFactory for %s with error %#lx\n",
                        debugstr_guid(riid), hr);
            }else {
                WARN("couldn't get IPSFactory buffer for interface %s\n", debugstr_guid(riid));
                hr = E_NOINTERFACE;
            }

        }

        if (hr == S_OK) {
            ifstub = stub_manager_new_ifstub(manager, stub, riid, dest_context, dest_context_data, mshlflags);
            if (!ifstub)
                hr = E_OUTOFMEMORY;
        }
        if (stub) IRpcStubBuffer_Release(stub);

        if (hr != S_OK) {
            stub_manager_int_release(manager);
            /* destroy the stub manager if it has no ifstubs by releasing
             * zero external references */
            stub_manager_ext_release(manager, 0, FALSE, TRUE);
            return hr;
        }
    }

    if (!tablemarshal)
    {
        stdobjref->cPublicRefs = NORMALEXTREFS;
        stub_manager_ext_addref(manager, stdobjref->cPublicRefs, FALSE);
    }
    else
    {
        stdobjref->cPublicRefs = 0;
        if (mshlflags & MSHLFLAGS_TABLESTRONG)
            stub_manager_ext_addref(manager, 1, FALSE);
        else
            stub_manager_ext_addref(manager, 0, TRUE);
    }

    /* FIXME: check return value */
    rpc_register_interface(riid);

    stdobjref->ipid = ifstub->ipid;

    stub_manager_int_release(manager);
    return S_OK;
}

/* Client-side identity of the server object */

static HRESULT proxy_manager_get_remunknown(struct proxy_manager * This, IRemUnknown **remunk);
static void proxy_manager_destroy(struct proxy_manager * This);
static HRESULT proxy_manager_find_ifproxy(struct proxy_manager * This, REFIID riid, struct ifproxy ** ifproxy_found);
static HRESULT proxy_manager_query_local_interface(struct proxy_manager * This, REFIID riid, void ** ppv);

static HRESULT WINAPI ClientIdentity_QueryInterface(IMultiQI * iface, REFIID riid, void ** ppv)
{
    HRESULT hr;
    MULTI_QI mqi;

    TRACE("%s\n", debugstr_guid(riid));

    mqi.pIID = riid;
    hr = IMultiQI_QueryMultipleInterfaces(iface, 1, &mqi);
    *ppv = mqi.pItf;

    return hr;
}

static ULONG WINAPI ClientIdentity_AddRef(IMultiQI *iface)
{
    struct proxy_manager *This = impl_from_IMultiQI(iface);
    TRACE("%p - before %ld\n", iface, This->refs);
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI ClientIdentity_Release(IMultiQI *iface)
{
    struct proxy_manager *This = impl_from_IMultiQI(iface);
    ULONG refs = InterlockedDecrement(&This->refs);
    TRACE("%p - after %ld\n", iface, refs);
    if (!refs)
        proxy_manager_destroy(This);
    return refs;
}

static HRESULT WINAPI ClientIdentity_QueryMultipleInterfaces(IMultiQI *iface, ULONG cMQIs, MULTI_QI *pMQIs)
{
    struct proxy_manager *This = impl_from_IMultiQI(iface);
    REMQIRESULT *qiresults = NULL;
    ULONG nonlocal_mqis = 0;
    ULONG i;
    ULONG successful_mqis = 0;
    IID *iids = malloc(cMQIs * sizeof(*iids));
    /* mapping of RemQueryInterface index to QueryMultipleInterfaces index */
    ULONG *mapping = malloc(cMQIs * sizeof(*mapping));

    TRACE("cMQIs: %ld\n", cMQIs);

    /* try to get a local interface - this includes already active proxy
     * interfaces and also interfaces exposed by the proxy manager */
    for (i = 0; i < cMQIs; i++)
    {
        TRACE("iid[%ld] = %s\n", i, debugstr_guid(pMQIs[i].pIID));
        pMQIs[i].hr = proxy_manager_query_local_interface(This, pMQIs[i].pIID, (void **)&pMQIs[i].pItf);
        if (pMQIs[i].hr == S_OK)
            successful_mqis++;
        else
        {
            iids[nonlocal_mqis] = *pMQIs[i].pIID;
            mapping[nonlocal_mqis] = i;
            nonlocal_mqis++;
        }
    }

    TRACE("%ld interfaces not found locally\n", nonlocal_mqis);

    /* if we have more than one interface not found locally then we must try
     * to query the remote object for it */
    if (nonlocal_mqis != 0)
    {
        IRemUnknown *remunk;
        HRESULT hr;
        IPID *ipid;

        /* get the ipid of the first entry */
        /* FIXME: should we implement ClientIdentity on the ifproxies instead
         * of the proxy_manager so we use the correct ipid here? */
        ipid = &LIST_ENTRY(list_head(&This->interfaces), struct ifproxy, entry)->stdobjref.ipid;

        /* get IRemUnknown proxy so we can communicate with the remote object */
        hr = proxy_manager_get_remunknown(This, &remunk);

        if (SUCCEEDED(hr))
        {
            hr = IRemUnknown_RemQueryInterface(remunk, ipid, NORMALEXTREFS,
                                               nonlocal_mqis, iids, &qiresults);
            IRemUnknown_Release(remunk);
            if (FAILED(hr))
                WARN("IRemUnknown_RemQueryInterface failed with error %#lx\n", hr);
        }

        /* IRemUnknown_RemQueryInterface can return S_FALSE if only some of
         * the interfaces were returned */
        if (SUCCEEDED(hr))
        {
            struct apartment *apt = apartment_get_current_or_mta();

            /* try to unmarshal each object returned to us */
            for (i = 0; i < nonlocal_mqis; i++)
            {
                ULONG index = mapping[i];
                HRESULT hrobj = qiresults[i].hResult;
                if (hrobj == S_OK)
                    hrobj = unmarshal_object(&qiresults[i].std, apt,
                                             This->dest_context,
                                             This->dest_context_data,
                                             pMQIs[index].pIID, &This->oxid_info,
                                             (void **)&pMQIs[index].pItf);

                if (hrobj == S_OK)
                    successful_mqis++;
                else
                    ERR("Failed to get pointer to interface %s\n", debugstr_guid(pMQIs[index].pIID));
                pMQIs[index].hr = hrobj;
            }

            apartment_release(apt);
        }

        /* free the memory allocated by the proxy */
        CoTaskMemFree(qiresults);
    }

    TRACE("%ld/%ld successfully queried\n", successful_mqis, cMQIs);

    free(iids);
    free(mapping);

    if (successful_mqis == cMQIs)
        return S_OK; /* we got all requested interfaces */
    else if (successful_mqis == 0)
        return E_NOINTERFACE; /* we didn't get any interfaces */
    else
        return S_FALSE; /* we got some interfaces */
}

static const IMultiQIVtbl ClientIdentity_Vtbl =
{
    ClientIdentity_QueryInterface,
    ClientIdentity_AddRef,
    ClientIdentity_Release,
    ClientIdentity_QueryMultipleInterfaces
};

static HRESULT StdMarshalImpl_Construct(REFIID, DWORD, void*, void**);

static HRESULT WINAPI Proxy_QueryInterface(IMarshal *iface, REFIID riid, void **ppvObject)
{
    struct proxy_manager *This = impl_from_IMarshal( iface );
    return IMultiQI_QueryInterface(&This->IMultiQI_iface, riid, ppvObject);
}

static ULONG WINAPI Proxy_AddRef(IMarshal *iface)
{
    struct proxy_manager *This = impl_from_IMarshal( iface );
    return IMultiQI_AddRef(&This->IMultiQI_iface);
}

static ULONG WINAPI Proxy_Release(IMarshal *iface)
{
    struct proxy_manager *This = impl_from_IMarshal( iface );
    return IMultiQI_Release(&This->IMultiQI_iface);
}

static HRESULT WINAPI Proxy_GetUnmarshalClass(
    IMarshal *iface, REFIID riid, void* pv, DWORD dwDestContext,
    void* pvDestContext, DWORD mshlflags, CLSID* pCid)
{
    *pCid = CLSID_StdMarshal;
    return S_OK;
}

static HRESULT WINAPI Proxy_GetMarshalSizeMax(
    IMarshal *iface, REFIID riid, void* pv, DWORD dwDestContext,
    void* pvDestContext, DWORD mshlflags, DWORD* pSize)
{
    *pSize = FIELD_OFFSET(OBJREF, u_objref.u_standard.saResAddr.aStringArray);
    return S_OK;
}

static void fill_std_objref(OBJREF *objref, const GUID *iid, STDOBJREF *std)
{
    objref->signature = OBJREF_SIGNATURE;
    objref->flags = OBJREF_STANDARD;
    objref->iid = *iid;
    if(std)
        objref->u_objref.u_standard.std = *std;
    memset(&objref->u_objref.u_standard.saResAddr, 0,
            sizeof(objref->u_objref.u_standard.saResAddr));
}

static HRESULT WINAPI Proxy_MarshalInterface(
    LPMARSHAL iface, IStream *pStm, REFIID riid, void* pv, DWORD dwDestContext,
    void* pvDestContext, DWORD mshlflags)
{
    struct proxy_manager *This = impl_from_IMarshal( iface );
    HRESULT hr;
    struct ifproxy *ifproxy;

    TRACE("(...,%s,...)\n", debugstr_guid(riid));

    hr = proxy_manager_find_ifproxy(This, riid, &ifproxy);
    if (SUCCEEDED(hr))
    {
        STDOBJREF stdobjref = ifproxy->stdobjref;

        stdobjref.cPublicRefs = 0;

        if ((mshlflags != MSHLFLAGS_TABLEWEAK) &&
            (mshlflags != MSHLFLAGS_TABLESTRONG))
        {
            ULONG cPublicRefs = ifproxy->refs;
            ULONG cPublicRefsOld;
            /* optimization - share out proxy's public references if possible
             * instead of making new proxy do a roundtrip through the server */
            do
            {
                ULONG cPublicRefsNew;
                cPublicRefsOld = cPublicRefs;
                stdobjref.cPublicRefs = cPublicRefs / 2;
                cPublicRefsNew = cPublicRefs - stdobjref.cPublicRefs;
                cPublicRefs = InterlockedCompareExchange(
                    (LONG *)&ifproxy->refs, cPublicRefsNew, cPublicRefsOld);
            } while (cPublicRefs != cPublicRefsOld);
        }

        /* normal and table-strong marshaling need at least one reference */
        if (!stdobjref.cPublicRefs && (mshlflags != MSHLFLAGS_TABLEWEAK))
        {
            IRemUnknown *remunk;
            hr = proxy_manager_get_remunknown(This, &remunk);
            if (hr == S_OK)
            {
                HRESULT hrref = S_OK;
                REMINTERFACEREF rif;
                rif.ipid = ifproxy->stdobjref.ipid;
                rif.cPublicRefs = (mshlflags == MSHLFLAGS_TABLESTRONG) ? 1 : NORMALEXTREFS;
                rif.cPrivateRefs = 0;
                hr = IRemUnknown_RemAddRef(remunk, 1, &rif, &hrref);
                IRemUnknown_Release(remunk);
                if (hr == S_OK && hrref == S_OK)
                {
                    /* table-strong marshaling doesn't give the refs to the
                     * client that unmarshals the STDOBJREF */
                    if (mshlflags != MSHLFLAGS_TABLESTRONG)
                        stdobjref.cPublicRefs = rif.cPublicRefs;
                }
                else
                    ERR("IRemUnknown_RemAddRef returned with %#lx, hrref = %#lx\n", hr, hrref);
            }
        }

        if (SUCCEEDED(hr))
        {
            OBJREF objref;

            TRACE("writing stdobjref: flags = %#lx cPublicRefs = %ld oxid = %s oid = %s ipid = %s\n",
                stdobjref.flags, stdobjref.cPublicRefs,
                wine_dbgstr_longlong(stdobjref.oxid),
                wine_dbgstr_longlong(stdobjref.oid),
                debugstr_guid(&stdobjref.ipid));
            fill_std_objref(&objref, riid, &stdobjref);
            hr = IStream_Write(pStm, &objref, FIELD_OFFSET(OBJREF,
                        u_objref.u_standard.saResAddr.aStringArray), NULL);
        }
    }
    else
    {
        /* we don't have the interface already unmarshaled so we have to
         * request the object from the server */
        IRemUnknown *remunk;
        IPID *ipid;
        REMQIRESULT *qiresults = NULL;
        IID iid = *riid;

        /* get the ipid of the first entry */
        /* FIXME: should we implement ClientIdentity on the ifproxies instead
         * of the proxy_manager so we use the correct ipid here? */
        ipid = &LIST_ENTRY(list_head(&This->interfaces), struct ifproxy, entry)->stdobjref.ipid;

        /* get IRemUnknown proxy so we can communicate with the remote object */
        hr = proxy_manager_get_remunknown(This, &remunk);

        if (hr == S_OK)
        {
            hr = IRemUnknown_RemQueryInterface(remunk, ipid, NORMALEXTREFS,
                                               1, &iid, &qiresults);
            if (SUCCEEDED(hr))
            {
                OBJREF objref;

                fill_std_objref(&objref, riid, &qiresults->std);
                hr = IStream_Write(pStm, &objref, FIELD_OFFSET(OBJREF,
                            u_objref.u_standard.saResAddr.aStringArray), NULL);
                if (FAILED(hr))
                {
                    REMINTERFACEREF rif;
                    rif.ipid = qiresults->std.ipid;
                    rif.cPublicRefs = qiresults->std.cPublicRefs;
                    rif.cPrivateRefs = 0;
                    IRemUnknown_RemRelease(remunk, 1, &rif);
                }
                CoTaskMemFree(qiresults);
            }
            else
                ERR("IRemUnknown_RemQueryInterface failed with error %#lx\n", hr);
            IRemUnknown_Release(remunk);
        }
    }

    return hr;
}

static HRESULT WINAPI Proxy_UnmarshalInterface(
        IMarshal *iface, IStream *pStm, REFIID riid, void **ppv)
{
    struct proxy_manager *This = impl_from_IMarshal( iface );
    IMarshal *marshal;
    HRESULT hr;

    TRACE("(%p, %p, %s, %p)\n", This, pStm, wine_dbgstr_guid(riid), ppv);

    hr = StdMarshalImpl_Construct(&IID_IMarshal, This->dest_context,
            This->dest_context_data, (void**)&marshal);
    if(FAILED(hr))
        return hr;

    hr = IMarshal_UnmarshalInterface(marshal, pStm, riid, ppv);
    IMarshal_Release(marshal);
    return hr;
}

static HRESULT WINAPI Proxy_ReleaseMarshalData(IMarshal *iface, IStream *pStm)
{
    struct proxy_manager *This = impl_from_IMarshal( iface );
    IMarshal *marshal;
    HRESULT hr;

    TRACE("(%p, %p)\n", This, pStm);

    hr = StdMarshalImpl_Construct(&IID_IMarshal, This->dest_context,
            This->dest_context_data, (void**)&marshal);
    if(FAILED(hr))
        return hr;

    hr = IMarshal_ReleaseMarshalData(marshal, pStm);
    IMarshal_Release(marshal);
    return hr;
}

static HRESULT WINAPI Proxy_DisconnectObject(IMarshal *iface, DWORD dwReserved)
{
    struct proxy_manager *This = impl_from_IMarshal( iface );
    IMarshal *marshal;
    HRESULT hr;

    TRACE("%p, %#lx\n", This, dwReserved);

    hr = StdMarshalImpl_Construct(&IID_IMarshal, This->dest_context,
            This->dest_context_data, (void**)&marshal);
    if(FAILED(hr))
        return hr;

    hr = IMarshal_DisconnectObject(marshal, dwReserved);
    IMarshal_Release(marshal);
    return hr;
}

static const IMarshalVtbl ProxyMarshal_Vtbl =
{
    Proxy_QueryInterface,
    Proxy_AddRef,
    Proxy_Release,
    Proxy_GetUnmarshalClass,
    Proxy_GetMarshalSizeMax,
    Proxy_MarshalInterface,
    Proxy_UnmarshalInterface,
    Proxy_ReleaseMarshalData,
    Proxy_DisconnectObject
};

static HRESULT WINAPI ProxyCliSec_QueryInterface(IClientSecurity *iface, REFIID riid, void **ppvObject)
{
    struct proxy_manager *This = impl_from_IClientSecurity( iface );
    return IMultiQI_QueryInterface(&This->IMultiQI_iface, riid, ppvObject);
}

static ULONG WINAPI ProxyCliSec_AddRef(IClientSecurity *iface)
{
    struct proxy_manager *This = impl_from_IClientSecurity( iface );
    return IMultiQI_AddRef(&This->IMultiQI_iface);
}

static ULONG WINAPI ProxyCliSec_Release(IClientSecurity *iface)
{
    struct proxy_manager *This = impl_from_IClientSecurity( iface );
    return IMultiQI_Release(&This->IMultiQI_iface);
}

static HRESULT WINAPI ProxyCliSec_QueryBlanket(IClientSecurity *iface,
                                               IUnknown *pProxy,
                                               DWORD *pAuthnSvc,
                                               DWORD *pAuthzSvc,
                                               OLECHAR **ppServerPrincName,
                                               DWORD *pAuthnLevel,
                                               DWORD *pImpLevel,
                                               void **pAuthInfo,
                                               DWORD *pCapabilities)
{
    FIXME("(%p, %p, %p, %p, %p, %p, %p, %p): stub\n", pProxy, pAuthnSvc,
          pAuthzSvc, ppServerPrincName, pAuthnLevel, pImpLevel, pAuthInfo,
          pCapabilities);

    if (pAuthnSvc)
        *pAuthnSvc = 0;
    if (pAuthzSvc)
        *pAuthzSvc = 0;
    if (ppServerPrincName)
        *ppServerPrincName = NULL;
    if (pAuthnLevel)
        *pAuthnLevel = RPC_C_AUTHN_LEVEL_DEFAULT;
    if (pImpLevel)
        *pImpLevel = RPC_C_IMP_LEVEL_DEFAULT;
    if (pAuthInfo)
        *pAuthInfo = NULL;
    if (pCapabilities)
        *pCapabilities = EOAC_NONE;

    return E_NOTIMPL;
}

static HRESULT WINAPI ProxyCliSec_SetBlanket(IClientSecurity *iface,
                                             IUnknown *pProxy, DWORD AuthnSvc,
                                             DWORD AuthzSvc,
                                             OLECHAR *pServerPrincName,
                                             DWORD AuthnLevel, DWORD ImpLevel,
                                             void *pAuthInfo,
                                             DWORD Capabilities)
{
    FIXME("%p, %ld, %ld, %s, %ld, %ld, %p, %#lx: stub\n", pProxy, AuthnSvc, AuthzSvc,
          pServerPrincName == COLE_DEFAULT_PRINCIPAL ? "<default principal>" : debugstr_w(pServerPrincName),
          AuthnLevel, ImpLevel, pAuthInfo, Capabilities);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProxyCliSec_CopyProxy(IClientSecurity *iface,
                                            IUnknown *pProxy, IUnknown **ppCopy)
{
    FIXME("(%p, %p): stub\n", pProxy, ppCopy);
    *ppCopy = NULL;
    return E_NOTIMPL;
}

static const IClientSecurityVtbl ProxyCliSec_Vtbl =
{
    ProxyCliSec_QueryInterface,
    ProxyCliSec_AddRef,
    ProxyCliSec_Release,
    ProxyCliSec_QueryBlanket,
    ProxyCliSec_SetBlanket,
    ProxyCliSec_CopyProxy
};

static HRESULT ifproxy_get_public_ref(struct ifproxy * This)
{
    HRESULT hr = S_OK;

    if (WAIT_OBJECT_0 != WaitForSingleObject(This->parent->remoting_mutex, INFINITE))
    {
        ERR("Wait failed for ifproxy %p\n", This);
        return E_UNEXPECTED;
    }

    if (This->refs == 0)
    {
        IRemUnknown *remunk = NULL;

        TRACE("getting public ref for ifproxy %p\n", This);

        hr = proxy_manager_get_remunknown(This->parent, &remunk);
        if (hr == S_OK)
        {
            HRESULT hrref = S_OK;
            REMINTERFACEREF rif;
            rif.ipid = This->stdobjref.ipid;
            rif.cPublicRefs = NORMALEXTREFS;
            rif.cPrivateRefs = 0;
            hr = IRemUnknown_RemAddRef(remunk, 1, &rif, &hrref);
            IRemUnknown_Release(remunk);
            if (hr == S_OK && hrref == S_OK)
                InterlockedExchangeAdd((LONG *)&This->refs, NORMALEXTREFS);
            else
                ERR("IRemUnknown_RemAddRef returned with %#lx, hrref = %#lx\n", hr, hrref);
        }
    }
    ReleaseMutex(This->parent->remoting_mutex);

    return hr;
}

static HRESULT ifproxy_release_public_refs(struct ifproxy * This)
{
    HRESULT hr = S_OK;
    LONG public_refs;

    if (WAIT_OBJECT_0 != WaitForSingleObject(This->parent->remoting_mutex, INFINITE))
    {
        ERR("Wait failed for ifproxy %p\n", This);
        return E_UNEXPECTED;
    }

    public_refs = This->refs;
    if (public_refs > 0)
    {
        IRemUnknown *remunk = NULL;

        TRACE("releasing %ld refs\n", public_refs);

        hr = proxy_manager_get_remunknown(This->parent, &remunk);
        if (hr == S_OK)
        {
            REMINTERFACEREF rif;
            rif.ipid = This->stdobjref.ipid;
            rif.cPublicRefs = public_refs;
            rif.cPrivateRefs = 0;
            hr = IRemUnknown_RemRelease(remunk, 1, &rif);
            IRemUnknown_Release(remunk);
            if (hr == S_OK)
                InterlockedExchangeAdd((LONG *)&This->refs, -public_refs);
            else if (hr == RPC_E_DISCONNECTED)
                WARN("couldn't release references because object was "
                     "disconnected: oxid = %s, oid = %s\n",
                     wine_dbgstr_longlong(This->parent->oxid),
                     wine_dbgstr_longlong(This->parent->oid));
            else
                ERR("IRemUnknown_RemRelease failed with error %#lx\n", hr);
        }
    }
    ReleaseMutex(This->parent->remoting_mutex);

    return hr;
}

/* should be called inside This->parent->cs critical section */
static void ifproxy_disconnect(struct ifproxy * This)
{
    ifproxy_release_public_refs(This);
    if (This->proxy) IRpcProxyBuffer_Disconnect(This->proxy);

    IRpcChannelBuffer_Release(This->chan);
    This->chan = NULL;
}

/* should be called in This->parent->cs critical section if it is an entry in parent's list */
static void ifproxy_destroy(struct ifproxy * This)
{
    TRACE("%p\n", This);

    /* release public references to this object so that the stub can know
     * when to destroy itself */
    ifproxy_release_public_refs(This);

    list_remove(&This->entry);

    if (This->chan)
    {
        IRpcChannelBuffer_Release(This->chan);
        This->chan = NULL;
    }

    if (This->proxy) IRpcProxyBuffer_Release(This->proxy);

    free(This);
}

static HRESULT proxy_manager_construct(
    struct apartment * apt, ULONG sorflags, OXID oxid, OID oid,
    const OXID_INFO *oxid_info, struct proxy_manager ** proxy_manager)
{
    struct proxy_manager * This = malloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->remoting_mutex = CreateMutexW(NULL, FALSE, NULL);
    if (!This->remoting_mutex)
    {
        free(This);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (oxid_info)
    {
        This->oxid_info.dwPid = oxid_info->dwPid;
        This->oxid_info.dwTid = oxid_info->dwTid;
        This->oxid_info.ipidRemUnknown = oxid_info->ipidRemUnknown;
        This->oxid_info.dwAuthnHint = oxid_info->dwAuthnHint;
        This->oxid_info.psa = NULL /* FIXME: copy from oxid_info */;
    }
    else
    {
        HRESULT hr = rpc_resolve_oxid(oxid, &This->oxid_info);
        if (FAILED(hr))
        {
            CloseHandle(This->remoting_mutex);
            free(This);
            return hr;
        }
    }

    This->IMultiQI_iface.lpVtbl = &ClientIdentity_Vtbl;
    This->IMarshal_iface.lpVtbl = &ProxyMarshal_Vtbl;
    This->IClientSecurity_iface.lpVtbl = &ProxyCliSec_Vtbl;

    list_init(&This->entry);
    list_init(&This->interfaces);

    InitializeCriticalSectionEx(&This->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    This->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": proxy_manager");

    /* the apartment the object was unmarshaled into */
    This->parent = apt;

    /* the source apartment and id of the object */
    This->oxid = oxid;
    This->oid = oid;

    This->refs = 1;

    /* the DCOM draft specification states that the SORF_NOPING flag is
     * proxy manager specific, not ifproxy specific, so this implies that we
     * should store the STDOBJREF flags here in the proxy manager. */
    This->sorflags = sorflags;

    /* we create the IRemUnknown proxy on demand */
    This->remunk = NULL;

    /* initialise these values to the weakest values and they will be
     * overwritten in proxy_manager_set_context */
    This->dest_context = MSHCTX_INPROC;
    This->dest_context_data = NULL;

    EnterCriticalSection(&apt->cs);
    /* FIXME: we are dependent on the ordering in here to make sure a proxy's
     * IRemUnknown proxy doesn't get destroyed before the regular proxy does
     * because we need the IRemUnknown proxy during the destruction of the
     * regular proxy. Ideally, we should maintain a separate list for the
     * IRemUnknown proxies that need late destruction */
    list_add_tail(&apt->proxies, &This->entry);
    LeaveCriticalSection(&apt->cs);

    TRACE("%p created for OXID %s, OID %s\n", This,
        wine_dbgstr_longlong(oxid), wine_dbgstr_longlong(oid));

    *proxy_manager = This;
    return S_OK;
}

static inline void proxy_manager_set_context(struct proxy_manager *This, MSHCTX dest_context, void *dest_context_data)
{
    MSHCTX old_dest_context;
    MSHCTX new_dest_context;

    do
    {
        old_dest_context = This->dest_context;
        new_dest_context = old_dest_context;
        /* "stronger" values overwrite "weaker" values. stronger values are
         * ones that disable more optimisations */
        switch (old_dest_context)
        {
        case MSHCTX_INPROC:
            new_dest_context = dest_context;
            break;
        case MSHCTX_CROSSCTX:
            switch (dest_context)
            {
            case MSHCTX_INPROC:
                break;
            default:
                new_dest_context = dest_context;
            }
            break;
        case MSHCTX_LOCAL:
            switch (dest_context)
            {
            case MSHCTX_INPROC:
            case MSHCTX_CROSSCTX:
                break;
            default:
                new_dest_context = dest_context;
            }
            break;
        case MSHCTX_NOSHAREDMEM:
            switch (dest_context)
            {
            case MSHCTX_DIFFERENTMACHINE:
                new_dest_context = dest_context;
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }

        if (old_dest_context == new_dest_context) break;

        new_dest_context = InterlockedCompareExchange((PLONG)&This->dest_context, new_dest_context, old_dest_context);
    } while (new_dest_context != old_dest_context);

    if (dest_context_data)
        InterlockedExchangePointer(&This->dest_context_data, dest_context_data);
}

static HRESULT proxy_manager_query_local_interface(struct proxy_manager * This, REFIID riid, void ** ppv)
{
    HRESULT hr;
    struct ifproxy * ifproxy;

    TRACE("%s\n", debugstr_guid(riid));

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IMultiQI))
    {
        *ppv = &This->IMultiQI_iface;
        IMultiQI_AddRef(&This->IMultiQI_iface);
        return S_OK;
    }
    if (IsEqualIID(riid, &IID_IMarshal))
    {
        *ppv = &This->IMarshal_iface;
        IMarshal_AddRef(&This->IMarshal_iface);
        return S_OK;
    }
    if (IsEqualIID(riid, &IID_IClientSecurity))
    {
        *ppv = &This->IClientSecurity_iface;
        IClientSecurity_AddRef(&This->IClientSecurity_iface);
        return S_OK;
    }

    hr = proxy_manager_find_ifproxy(This, riid, &ifproxy);
    if (hr == S_OK)
    {
        *ppv = ifproxy->iface;
        IUnknown_AddRef((IUnknown *)*ppv);
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static HRESULT proxy_manager_create_ifproxy(
    struct proxy_manager * This, const STDOBJREF *stdobjref, REFIID riid,
    IRpcChannelBuffer * channel, struct ifproxy ** iif_out)
{
    HRESULT hr;
    IPSFactoryBuffer * psfb;
    struct ifproxy * ifproxy = malloc(sizeof(*ifproxy));
    if (!ifproxy) return E_OUTOFMEMORY;

    list_init(&ifproxy->entry);

    ifproxy->parent = This;
    ifproxy->stdobjref = *stdobjref;
    ifproxy->iid = *riid;
    ifproxy->refs = 0;
    ifproxy->proxy = NULL;

    assert(channel);
    ifproxy->chan = channel; /* FIXME: we should take the binding strings and construct the channel in this function */

    /* the IUnknown interface is special because it does not have a
     * proxy associated with the ifproxy as we handle IUnknown ourselves */
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        ifproxy->iface = &This->IMultiQI_iface;
        IMultiQI_AddRef(&This->IMultiQI_iface);
        hr = S_OK;
    }
    else
    {
        hr = get_facbuf_for_iid(riid, &psfb);
        if (hr == S_OK)
        {
            /* important note: the outer unknown is set to the proxy manager.
             * This ensures the COM identity rules are not violated, by having a
             * one-to-one mapping of objects on the proxy side to objects on the
             * stub side, no matter which interface you view the object through */
            hr = IPSFactoryBuffer_CreateProxy(psfb, (IUnknown*)&This->IMultiQI_iface, riid,
                                              &ifproxy->proxy, &ifproxy->iface);
            IPSFactoryBuffer_Release(psfb);
            if (hr != S_OK)
                ERR("Could not create proxy for interface %s, error %#lx\n", debugstr_guid(riid), hr);
        }
        else
            ERR("Could not get IPSFactoryBuffer for interface %s, error %#lx\n", debugstr_guid(riid), hr);

        if (hr == S_OK)
            hr = IRpcProxyBuffer_Connect(ifproxy->proxy, ifproxy->chan);
    }

    if (hr == S_OK)
    {
        EnterCriticalSection(&This->cs);
        list_add_tail(&This->interfaces, &ifproxy->entry);
        LeaveCriticalSection(&This->cs);

        *iif_out = ifproxy;
        TRACE("ifproxy %p created for IPID %s, interface %s with %lu public refs\n",
              ifproxy, debugstr_guid(&stdobjref->ipid), debugstr_guid(riid), stdobjref->cPublicRefs);
    }
    else
        ifproxy_destroy(ifproxy);

    return hr;
}

static HRESULT proxy_manager_find_ifproxy(struct proxy_manager * This, REFIID riid, struct ifproxy ** ifproxy_found)
{
    HRESULT hr = E_NOINTERFACE; /* assume not found */
    struct ifproxy *ifproxy;

    EnterCriticalSection(&This->cs);
    LIST_FOR_EACH_ENTRY(ifproxy, &This->interfaces, struct ifproxy, entry)
    {
        if (IsEqualIID(riid, &ifproxy->iid))
        {
            *ifproxy_found = ifproxy;
            hr = S_OK;
            break;
        }
    }
    LeaveCriticalSection(&This->cs);

    return hr;
}

static void proxy_manager_disconnect(struct proxy_manager * This)
{
    struct ifproxy *ifproxy;

    TRACE("oxid = %s, oid = %s\n", wine_dbgstr_longlong(This->oxid),
        wine_dbgstr_longlong(This->oid));

    EnterCriticalSection(&This->cs);

    /* SORFP_NOLIFTIMEMGMT proxies (for IRemUnknown) shouldn't be
     * disconnected - it won't do anything anyway, except cause
     * problems for other objects that depend on this proxy always
     * working */
    if (!(This->sorflags & SORFP_NOLIFETIMEMGMT))
    {
        LIST_FOR_EACH_ENTRY(ifproxy, &This->interfaces, struct ifproxy, entry)
        {
            ifproxy_disconnect(ifproxy);
        }
    }

    /* apartment is being destroyed so don't keep a pointer around to it */
    This->parent = NULL;

    LeaveCriticalSection(&This->cs);
}

static HRESULT proxy_manager_get_remunknown(struct proxy_manager * This, IRemUnknown **remunk)
{
    HRESULT hr = S_OK;
    struct apartment *apt;
    BOOL called_in_original_apt;

    /* we don't want to try and unmarshal or use IRemUnknown if we don't want
     * lifetime management */
    if (This->sorflags & SORFP_NOLIFETIMEMGMT)
        return S_FALSE;

    if (!(apt = apartment_get_current_or_mta()))
        return CO_E_NOTINITIALIZED;

    called_in_original_apt = This->parent && (This->parent->oxid == apt->oxid);

    EnterCriticalSection(&This->cs);
    /* only return the cached object if called from the original apartment.
     * in future, we might want to make the IRemUnknown proxy callable from any
     * apartment to avoid these checks */
    if (This->remunk && called_in_original_apt)
    {
        /* already created - return existing object */
        *remunk = This->remunk;
        IRemUnknown_AddRef(*remunk);
    }
    else if (!This->parent)
    {
        /* disconnected - we can't create IRemUnknown */
        *remunk = NULL;
        hr = S_FALSE;
    }
    else
    {
        STDOBJREF stdobjref;
        /* Don't want IRemUnknown lifetime management as this is IRemUnknown!
         * We also don't care about whether or not the stub is still alive */
        stdobjref.flags = SORFP_NOLIFETIMEMGMT | SORF_NOPING;
        stdobjref.cPublicRefs = 1;
        /* oxid of destination object */
        stdobjref.oxid = This->oxid;
        /* FIXME: what should be used for the oid? The DCOM draft doesn't say */
        stdobjref.oid = (OID)-1;
        stdobjref.ipid = This->oxid_info.ipidRemUnknown;

        /* do the unmarshal */
        hr = unmarshal_object(&stdobjref, apt, This->dest_context,
                              This->dest_context_data, &IID_IRemUnknown,
                              &This->oxid_info, (void**)remunk);
        if (hr == S_OK && called_in_original_apt)
        {
            This->remunk = *remunk;
            IRemUnknown_AddRef(This->remunk);
        }
    }
    LeaveCriticalSection(&This->cs);
    apartment_release(apt);

    TRACE("got IRemUnknown* pointer %p, hr = %#lx\n", *remunk, hr);

    return hr;
}

/*
 * Safely increment the reference count of a proxy manager obtained from an
 * apartment proxy list.
 *
 * This function shall be called inside the apartment's critical section.
 */
static LONG proxy_manager_addref_if_alive(struct proxy_manager * This)
{
    LONG refs = ReadNoFence(&This->refs);
    LONG old_refs, new_refs;

    do
    {
        if (refs == 0)
        {
            /* This proxy manager is about to be destroyed */
            return 0;
        }

        old_refs = refs;
        new_refs = refs + 1;
    } while ((refs = InterlockedCompareExchange(&This->refs, new_refs, old_refs)) != old_refs);

    return new_refs;
}

/* destroys a proxy manager, freeing the memory it used.
 * Note: this function should not be called from a list iteration in the
 * apartment, due to the fact that it removes itself from the apartment and
 * it could add a proxy to IRemUnknown into the apartment. */
static void proxy_manager_destroy(struct proxy_manager * This)
{
    struct list * cursor;

    TRACE("oxid = %s, oid = %s\n", wine_dbgstr_longlong(This->oxid),
        wine_dbgstr_longlong(This->oid));

    if (This->parent)
    {
        EnterCriticalSection(&This->parent->cs);

        /* remove ourself from the list of proxy objects in the apartment */
        LIST_FOR_EACH(cursor, &This->parent->proxies)
        {
            if (cursor == &This->entry)
            {
                list_remove(&This->entry);
                break;
            }
        }

        LeaveCriticalSection(&This->parent->cs);
    }

    /* destroy all of the interface proxies */
    while ((cursor = list_head(&This->interfaces)))
    {
        struct ifproxy * ifproxy = LIST_ENTRY(cursor, struct ifproxy, entry);
        ifproxy_destroy(ifproxy);
    }

    if (This->remunk) IRemUnknown_Release(This->remunk);
    CoTaskMemFree(This->oxid_info.psa);

    This->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&This->cs);

    CloseHandle(This->remoting_mutex);

    free(This);
}

/* finds the proxy manager corresponding to a given OXID and OID that has
 * been unmarshaled in the specified apartment. The caller must release the
 * reference to the proxy_manager when the object is no longer used. */
static BOOL find_proxy_manager(struct apartment * apt, OXID oxid, OID oid, struct proxy_manager ** proxy_found)
{
    struct proxy_manager *proxy;
    BOOL found = FALSE;

    EnterCriticalSection(&apt->cs);
    LIST_FOR_EACH_ENTRY(proxy, &apt->proxies, struct proxy_manager, entry)
    {
        if ((oxid == proxy->oxid) && (oid == proxy->oid))
        {
            /* be careful of a race with ClientIdentity_Release, which would
             * cause us to return a proxy which is in the process of being
             * destroyed */
            if (proxy_manager_addref_if_alive(proxy) != 0)
            {
                *proxy_found = proxy;
                found = TRUE;
                break;
            }
        }
    }
    LeaveCriticalSection(&apt->cs);
    return found;
}

HRESULT apartment_disconnectproxies(struct apartment *apt)
{
    struct proxy_manager *proxy;

    LIST_FOR_EACH_ENTRY(proxy, &apt->proxies, struct proxy_manager, entry)
    {
        proxy_manager_disconnect(proxy);
    }

    return S_OK;
}

/********************** StdMarshal implementation ****************************/

struct stdmarshal
{
    IMarshal IMarshal_iface;
    LONG refcount;
    DWORD dest_context;
    void *dest_context_data;
};

static inline struct stdmarshal *impl_from_StdMarshal(IMarshal *iface)
{
    return CONTAINING_RECORD(iface, struct stdmarshal, IMarshal_iface);
}

static HRESULT WINAPI StdMarshalImpl_QueryInterface(IMarshal *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(&IID_IUnknown, riid) || IsEqualIID(&IID_IMarshal, riid))
    {
        *ppv = iface;
        IMarshal_AddRef(iface);
        return S_OK;
    }
    FIXME("No interface for %s.\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI StdMarshalImpl_AddRef(IMarshal *iface)
{
    struct stdmarshal *marshal = impl_from_StdMarshal(iface);
    return InterlockedIncrement(&marshal->refcount);
}

static ULONG WINAPI StdMarshalImpl_Release(IMarshal *iface)
{
    struct stdmarshal *marshal = impl_from_StdMarshal(iface);
    ULONG refcount = InterlockedDecrement(&marshal->refcount);

    if (!refcount)
        free(marshal);

    return refcount;
}

static HRESULT WINAPI StdMarshalImpl_GetUnmarshalClass(IMarshal *iface, REFIID riid, void *pv,
        DWORD dwDestContext, void *pvDestContext, DWORD mshlflags, CLSID *pCid)
{
    *pCid = CLSID_StdMarshal;
    return S_OK;
}

static HRESULT WINAPI StdMarshalImpl_GetMarshalSizeMax(IMarshal *iface, REFIID riid, void *pv,
        DWORD dwDestContext, void *pvDestContext, DWORD mshlflags, DWORD *pSize)
{
    *pSize = FIELD_OFFSET(OBJREF, u_objref.u_standard.saResAddr.aStringArray);
    return S_OK;
}

static HRESULT WINAPI StdMarshalImpl_MarshalInterface(IMarshal *iface, IStream *stream, REFIID riid, void *pv,
        DWORD dest_context, void *dest_context_data, DWORD mshlflags)
{
    ULONG res;
    HRESULT hr;
    OBJREF objref;
    struct apartment *apt;

    TRACE("(...,%s,...)\n", debugstr_guid(riid));

    if (!(apt = apartment_get_current_or_mta()))
    {
        ERR("Apartment not initialized\n");
        return CO_E_NOTINITIALIZED;
    }

    /* make sure this apartment can be reached from other threads / processes */
    rpc_start_remoting(apt);

    fill_std_objref(&objref, riid, NULL);
    hr = marshal_object(apt, &objref.u_objref.u_standard.std, riid, pv, dest_context,
            dest_context_data, mshlflags);
    apartment_release(apt);
    if (hr != S_OK)
    {
        ERR("Failed to create ifstub, hr %#lx\n", hr);
        return hr;
    }

    return IStream_Write(stream, &objref, FIELD_OFFSET(OBJREF, u_objref.u_standard.saResAddr.aStringArray), &res);
}

/* helper for StdMarshalImpl_UnmarshalInterface - does the unmarshaling with
 * no questions asked about the rules surrounding same-apartment unmarshals
 * and table marshaling */
static HRESULT unmarshal_object(const STDOBJREF *stdobjref, struct apartment *apt, MSHCTX dest_context,
        void *dest_context_data, REFIID riid, const OXID_INFO *oxid_info, void **object)
{
    struct proxy_manager *proxy_manager = NULL;
    HRESULT hr = S_OK;

    assert(apt);

    TRACE("stdobjref: flags = %#lx cPublicRefs = %ld oxid = %s oid = %s ipid = %s\n",
        stdobjref->flags, stdobjref->cPublicRefs,
        wine_dbgstr_longlong(stdobjref->oxid),
        wine_dbgstr_longlong(stdobjref->oid),
        debugstr_guid(&stdobjref->ipid));

    /* create a new proxy manager if one doesn't already exist for the
     * object */
    if (!find_proxy_manager(apt, stdobjref->oxid, stdobjref->oid, &proxy_manager))
    {
        hr = proxy_manager_construct(apt, stdobjref->flags,
                                     stdobjref->oxid, stdobjref->oid, oxid_info,
                                     &proxy_manager);
    }
    else
        TRACE("proxy manager already created, using\n");

    if (hr == S_OK)
    {
        struct ifproxy * ifproxy;

        proxy_manager_set_context(proxy_manager, dest_context, dest_context_data);

        hr = proxy_manager_find_ifproxy(proxy_manager, riid, &ifproxy);
        if (hr == E_NOINTERFACE)
        {
            IRpcChannelBuffer *chanbuf;
            hr = rpc_create_clientchannel(&stdobjref->oxid, &stdobjref->ipid,
                    &proxy_manager->oxid_info, riid, proxy_manager->dest_context,
                    proxy_manager->dest_context_data, &chanbuf, apt);
            if (hr == S_OK)
                hr = proxy_manager_create_ifproxy(proxy_manager, stdobjref, riid, chanbuf, &ifproxy);
        }
        else
            IUnknown_AddRef((IUnknown *)ifproxy->iface);

        if (hr == S_OK)
        {
            InterlockedExchangeAdd((LONG *)&ifproxy->refs, stdobjref->cPublicRefs);
            /* get at least one external reference to the object to keep it alive */
            hr = ifproxy_get_public_ref(ifproxy);
            if (FAILED(hr))
                ifproxy_destroy(ifproxy);
        }

        if (hr == S_OK)
            *object = ifproxy->iface;
    }

    /* release our reference to the proxy manager - the client/apartment
     * will hold on to the remaining reference for us */
    if (proxy_manager) IMultiQI_Release(&proxy_manager->IMultiQI_iface);

    return hr;
}

static HRESULT WINAPI StdMarshalImpl_UnmarshalInterface(IMarshal *iface, IStream *stream, REFIID riid, void **ppv)
{
    struct stdmarshal *marshal = impl_from_StdMarshal(iface);
    OBJREF objref;
    HRESULT hr;
    ULONG res;

    hr = IStream_Read(stream, &objref, FIELD_OFFSET(OBJREF, u_objref), &res);
    if (hr != S_OK || (res != FIELD_OFFSET(OBJREF, u_objref)))
    {
        ERR("Failed to read common OBJREF header, %#lx\n", hr);
        return STG_E_READFAULT;
    }

    if (objref.signature != OBJREF_SIGNATURE)
    {
        ERR("Bad OBJREF signature %#lx\n", objref.signature);
        return RPC_E_INVALID_OBJREF;
    }

    if (!(objref.flags & OBJREF_STANDARD))
    {
        FIXME("unsupported objref.flags = %lx\n", objref.flags);
        return E_NOTIMPL;
    }

    return std_unmarshal_interface(marshal->dest_context, marshal->dest_context_data, stream, riid, ppv, TRUE);
}

static HRESULT WINAPI StdMarshalImpl_ReleaseMarshalData(IMarshal *iface, IStream *stream)
{
    OBJREF objref;
    HRESULT hr;
    ULONG res;

    TRACE("%p, %p\n", iface, stream);

    hr = IStream_Read(stream, &objref, FIELD_OFFSET(OBJREF, u_objref), &res);
    if (hr != S_OK || (res != FIELD_OFFSET(OBJREF, u_objref)))
    {
        ERR("Failed to read common OBJREF header, %#lx\n", hr);
        return STG_E_READFAULT;
    }

    if (objref.signature != OBJREF_SIGNATURE)
    {
        ERR("Bad OBJREF signature %#lx\n", objref.signature);
        return RPC_E_INVALID_OBJREF;
    }

    if (!(objref.flags & OBJREF_STANDARD))
    {
        FIXME("unsupported objref.flags = %lx\n", objref.flags);
        return E_NOTIMPL;
    }

    return std_release_marshal_data(stream);
}

static HRESULT WINAPI StdMarshalImpl_DisconnectObject(IMarshal *iface, DWORD reserved)
{
    FIXME("(), stub!\n");
    return S_OK;
}

static const IMarshalVtbl StdMarshalVtbl =
{
    StdMarshalImpl_QueryInterface,
    StdMarshalImpl_AddRef,
    StdMarshalImpl_Release,
    StdMarshalImpl_GetUnmarshalClass,
    StdMarshalImpl_GetMarshalSizeMax,
    StdMarshalImpl_MarshalInterface,
    StdMarshalImpl_UnmarshalInterface,
    StdMarshalImpl_ReleaseMarshalData,
    StdMarshalImpl_DisconnectObject
};

static HRESULT StdMarshalImpl_Construct(REFIID riid, DWORD dest_context, void *dest_context_data, void **ppvObject)
{
    struct stdmarshal *object;
    HRESULT hr;

    object = malloc(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMarshal_iface.lpVtbl = &StdMarshalVtbl;
    object->refcount = 1;
    object->dest_context = dest_context;
    object->dest_context_data = dest_context_data;

    hr = IMarshal_QueryInterface(&object->IMarshal_iface, riid, ppvObject);
    IMarshal_Release(&object->IMarshal_iface);

    return hr;
}

HRESULT WINAPI InternalCoStdMarshalObject(REFIID riid, DWORD dest_context, void *dest_context_data, void **ppvObject)
{
    return StdMarshalImpl_Construct(riid, dest_context, dest_context_data, ppvObject);
}

/***********************************************************************
 *            CoGetStandardMarshal        (combase.@)
 */
HRESULT WINAPI CoGetStandardMarshal(REFIID riid, IUnknown *pUnk, DWORD dwDestContext,
        void *dest_context, DWORD flags, IMarshal **marshal)
{
    if (pUnk == NULL)
    {
        FIXME("%s, NULL, %lx, %p, %lx, %p, unimplemented yet.\n", debugstr_guid(riid), dwDestContext,
                dest_context, flags, marshal);
        return E_NOTIMPL;
    }
    TRACE("%s, %p, %lx, %p, %lx, %p\n", debugstr_guid(riid), pUnk, dwDestContext, dest_context, flags, marshal);

    return StdMarshalImpl_Construct(&IID_IMarshal, dwDestContext, dest_context, (void **)marshal);
}
