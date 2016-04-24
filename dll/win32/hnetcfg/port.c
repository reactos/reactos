/*
 * Copyright 2009 Hans Leidekker for CodeWeavers
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

#include "hnetcfg_private.h"

#include <ole2.h>

typedef struct fw_port
{
    INetFwOpenPort INetFwOpenPort_iface;
    LONG refs;
    BSTR name;
    NET_FW_IP_PROTOCOL protocol;
    LONG port;
} fw_port;

static inline fw_port *impl_from_INetFwOpenPort( INetFwOpenPort *iface )
{
    return CONTAINING_RECORD(iface, fw_port, INetFwOpenPort_iface);
}

static ULONG WINAPI fw_port_AddRef(
    INetFwOpenPort *iface )
{
    fw_port *fw_port = impl_from_INetFwOpenPort( iface );
    return InterlockedIncrement( &fw_port->refs );
}

static ULONG WINAPI fw_port_Release(
    INetFwOpenPort *iface )
{
    fw_port *fw_port = impl_from_INetFwOpenPort( iface );
    LONG refs = InterlockedDecrement( &fw_port->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", fw_port);
        SysFreeString( fw_port->name );
        HeapFree( GetProcessHeap(), 0, fw_port );
    }
    return refs;
}

static HRESULT WINAPI fw_port_QueryInterface(
    INetFwOpenPort *iface,
    REFIID riid,
    void **ppvObject )
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_INetFwOpenPort ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    INetFwOpenPort_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI fw_port_GetTypeInfoCount(
    INetFwOpenPort *iface,
    UINT *pctinfo )
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    TRACE("%p %p\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI fw_port_GetTypeInfo(
    INetFwOpenPort *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTInfo )
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    TRACE("%p %u %u %p\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo( INetFwOpenPort_tid, ppTInfo );
}

static HRESULT WINAPI fw_port_GetIDsOfNames(
    INetFwOpenPort *iface,
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId )
{
    fw_port *This = impl_from_INetFwOpenPort( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %s %p %u %u %p\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo( INetFwOpenPort_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, rgszNames, cNames, rgDispId );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI fw_port_Invoke(
    INetFwOpenPort *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr )
{
    fw_port *This = impl_from_INetFwOpenPort( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %d %s %d %d %p %p %p %p\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo( INetFwOpenPort_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &This->INetFwOpenPort_iface, dispIdMember,
                               wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI fw_port_get_Name(
    INetFwOpenPort *iface,
    BSTR *name)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %p\n", This, name);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_put_Name(
    INetFwOpenPort *iface,
    BSTR name)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    TRACE("%p %s\n", This, debugstr_w(name));

    if (!(name = SysAllocString( name )))
        return E_OUTOFMEMORY;

    SysFreeString( This->name );
    This->name = name;
    return S_OK;
}

static HRESULT WINAPI fw_port_get_IpVersion(
    INetFwOpenPort *iface,
    NET_FW_IP_VERSION *ipVersion)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %p\n", This, ipVersion);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_put_IpVersion(
    INetFwOpenPort *iface,
    NET_FW_IP_VERSION ipVersion)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %u\n", This, ipVersion);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_get_Protocol(
    INetFwOpenPort *iface,
    NET_FW_IP_PROTOCOL *ipProtocol)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %p\n", This, ipProtocol);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_put_Protocol(
    INetFwOpenPort *iface,
    NET_FW_IP_PROTOCOL ipProtocol)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    TRACE("%p %u\n", This, ipProtocol);

    if (ipProtocol != NET_FW_IP_PROTOCOL_TCP && ipProtocol != NET_FW_IP_PROTOCOL_UDP)
        return E_INVALIDARG;

    This->protocol = ipProtocol;
    return S_OK;
}

static HRESULT WINAPI fw_port_get_Port(
    INetFwOpenPort *iface,
    LONG *portNumber)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %p\n", This, portNumber);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_put_Port(
    INetFwOpenPort *iface,
    LONG portNumber)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    TRACE("%p %d\n", This, portNumber);
    This->port = portNumber;
    return S_OK;
}

static HRESULT WINAPI fw_port_get_Scope(
    INetFwOpenPort *iface,
    NET_FW_SCOPE *scope)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %p\n", This, scope);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_put_Scope(
    INetFwOpenPort *iface,
    NET_FW_SCOPE scope)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %u\n", This, scope);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_get_RemoteAddresses(
    INetFwOpenPort *iface,
    BSTR *remoteAddrs)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %p\n", This, remoteAddrs);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_put_RemoteAddresses(
    INetFwOpenPort *iface,
    BSTR remoteAddrs)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %s\n", This, debugstr_w(remoteAddrs));
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_get_Enabled(
    INetFwOpenPort *iface,
    VARIANT_BOOL *enabled)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %p\n", This, enabled);

    *enabled = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI fw_port_put_Enabled(
    INetFwOpenPort *iface,
    VARIANT_BOOL enabled)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %d\n", This, enabled);
    return S_OK;
}

static HRESULT WINAPI fw_port_get_BuiltIn(
    INetFwOpenPort *iface,
    VARIANT_BOOL *builtIn)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %p\n", This, builtIn);
    return E_NOTIMPL;
}

static const struct INetFwOpenPortVtbl fw_port_vtbl =
{
    fw_port_QueryInterface,
    fw_port_AddRef,
    fw_port_Release,
    fw_port_GetTypeInfoCount,
    fw_port_GetTypeInfo,
    fw_port_GetIDsOfNames,
    fw_port_Invoke,
    fw_port_get_Name,
    fw_port_put_Name,
    fw_port_get_IpVersion,
    fw_port_put_IpVersion,
    fw_port_get_Protocol,
    fw_port_put_Protocol,
    fw_port_get_Port,
    fw_port_put_Port,
    fw_port_get_Scope,
    fw_port_put_Scope,
    fw_port_get_RemoteAddresses,
    fw_port_put_RemoteAddresses,
    fw_port_get_Enabled,
    fw_port_put_Enabled,
    fw_port_get_BuiltIn
};

HRESULT NetFwOpenPort_create( IUnknown *pUnkOuter, LPVOID *ppObj )
{
    fw_port *fp;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    fp = HeapAlloc( GetProcessHeap(), 0, sizeof(*fp) );
    if (!fp) return E_OUTOFMEMORY;

    fp->INetFwOpenPort_iface.lpVtbl = &fw_port_vtbl;
    fp->refs = 1;
    fp->name = NULL;
    fp->protocol = NET_FW_IP_PROTOCOL_TCP;
    fp->port = 0;

    *ppObj = &fp->INetFwOpenPort_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}

typedef struct fw_ports
{
    INetFwOpenPorts INetFwOpenPorts_iface;
    LONG refs;
} fw_ports;

static inline fw_ports *impl_from_INetFwOpenPorts( INetFwOpenPorts *iface )
{
    return CONTAINING_RECORD(iface, fw_ports, INetFwOpenPorts_iface);
}

static ULONG WINAPI fw_ports_AddRef(
    INetFwOpenPorts *iface )
{
    fw_ports *fw_ports = impl_from_INetFwOpenPorts( iface );
    return InterlockedIncrement( &fw_ports->refs );
}

static ULONG WINAPI fw_ports_Release(
    INetFwOpenPorts *iface )
{
    fw_ports *fw_ports = impl_from_INetFwOpenPorts( iface );
    LONG refs = InterlockedDecrement( &fw_ports->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", fw_ports);
        HeapFree( GetProcessHeap(), 0, fw_ports );
    }
    return refs;
}

static HRESULT WINAPI fw_ports_QueryInterface(
    INetFwOpenPorts *iface,
    REFIID riid,
    void **ppvObject )
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_INetFwOpenPorts ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    INetFwOpenPorts_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI fw_ports_GetTypeInfoCount(
    INetFwOpenPorts *iface,
    UINT *pctinfo )
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );

    TRACE("%p %p\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI fw_ports_GetTypeInfo(
    INetFwOpenPorts *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTInfo )
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );

    TRACE("%p %u %u %p\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo( INetFwOpenPorts_tid, ppTInfo );
}

static HRESULT WINAPI fw_ports_GetIDsOfNames(
    INetFwOpenPorts *iface,
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId )
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %s %p %u %u %p\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo( INetFwOpenPorts_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, rgszNames, cNames, rgDispId );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI fw_ports_Invoke(
    INetFwOpenPorts *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr )
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %d %s %d %d %p %p %p %p\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo( INetFwOpenPorts_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &This->INetFwOpenPorts_iface, dispIdMember,
                               wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI fw_ports_get_Count(
    INetFwOpenPorts *iface,
    LONG *count)
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );

    FIXME("%p, %p\n", This, count);

    *count = 0;
    return S_OK;
}

static HRESULT WINAPI fw_ports_Add(
    INetFwOpenPorts *iface,
    INetFwOpenPort *port)
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );

    FIXME("%p, %p\n", This, port);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_ports_Remove(
    INetFwOpenPorts *iface,
    LONG portNumber,
    NET_FW_IP_PROTOCOL ipProtocol)
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );

    FIXME("%p, %d, %u\n", This, portNumber, ipProtocol);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_ports_Item(
    INetFwOpenPorts *iface,
    LONG portNumber,
    NET_FW_IP_PROTOCOL ipProtocol,
    INetFwOpenPort **openPort)
{
    HRESULT hr;
    fw_ports *This = impl_from_INetFwOpenPorts( iface );

    FIXME("%p, %d, %u, %p\n", This, portNumber, ipProtocol, openPort);

    hr = NetFwOpenPort_create( NULL, (void **)openPort );
    if (SUCCEEDED(hr))
    {
        INetFwOpenPort_put_Protocol( *openPort, ipProtocol );
        INetFwOpenPort_put_Port( *openPort, portNumber );
    }
    return hr;
}

static HRESULT WINAPI fw_ports_get__NewEnum(
    INetFwOpenPorts *iface,
    IUnknown **newEnum)
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );

    FIXME("%p, %p\n", This, newEnum);
    return E_NOTIMPL;
}

static const struct INetFwOpenPortsVtbl fw_ports_vtbl =
{
    fw_ports_QueryInterface,
    fw_ports_AddRef,
    fw_ports_Release,
    fw_ports_GetTypeInfoCount,
    fw_ports_GetTypeInfo,
    fw_ports_GetIDsOfNames,
    fw_ports_Invoke,
    fw_ports_get_Count,
    fw_ports_Add,
    fw_ports_Remove,
    fw_ports_Item,
    fw_ports_get__NewEnum
};

HRESULT NetFwOpenPorts_create( IUnknown *pUnkOuter, LPVOID *ppObj )
{
    fw_ports *fp;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    fp = HeapAlloc( GetProcessHeap(), 0, sizeof(*fp) );
    if (!fp) return E_OUTOFMEMORY;

    fp->INetFwOpenPorts_iface.lpVtbl = &fw_ports_vtbl;
    fp->refs = 1;

    *ppObj = &fp->INetFwOpenPorts_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}
