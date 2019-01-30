/*
 * COM proxy definitions
 *
 * Copyright 2001 Ove Kåven, TransGaming Technologies
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

#ifndef __WINE_CPSF_H
#define __WINE_CPSF_H

typedef struct
{
    IRpcProxyBuffer IRpcProxyBuffer_iface;
    void **PVtbl;
    LONG RefCount;
    const IID *piid;
    IUnknown *pUnkOuter;
    /* offset of base_object from PVtbl must match assembly thunks; see
     * fill_delegated_proxy_table() */
    IUnknown *base_object;
    IRpcProxyBuffer *base_proxy;
    PCInterfaceName name;
    IPSFactoryBuffer *pPSFactory;
    IRpcChannelBuffer *pChannel;
} StdProxyImpl;

typedef struct
{
    IUnknownVtbl *base_obj;
    IRpcStubBuffer *base_stub;
    CStdStubBuffer stub_buffer;
} cstdstubbuffer_delegating_t;

HRESULT StdProxy_Construct(REFIID riid, LPUNKNOWN pUnkOuter, const ProxyFileInfo *ProxyInfo,
                           int Index, LPPSFACTORYBUFFER pPSFactory, LPRPCPROXYBUFFER *ppProxy,
                           LPVOID *ppvObj) DECLSPEC_HIDDEN;
HRESULT WINAPI StdProxy_QueryInterface(IRpcProxyBuffer *iface, REFIID iid, void **obj) DECLSPEC_HIDDEN;
ULONG WINAPI StdProxy_AddRef(IRpcProxyBuffer *iface) DECLSPEC_HIDDEN;
HRESULT WINAPI StdProxy_Connect(IRpcProxyBuffer *iface, IRpcChannelBuffer *channel) DECLSPEC_HIDDEN;
void WINAPI StdProxy_Disconnect(IRpcProxyBuffer *iface) DECLSPEC_HIDDEN;

HRESULT CStdStubBuffer_Construct(REFIID riid, LPUNKNOWN pUnkServer, PCInterfaceName name,
                                 CInterfaceStubVtbl *vtbl, LPPSFACTORYBUFFER pPSFactory,
                                 LPRPCSTUBBUFFER *ppStub) DECLSPEC_HIDDEN;

HRESULT CStdStubBuffer_Delegating_Construct(REFIID riid, LPUNKNOWN pUnkServer, PCInterfaceName name,
                                            CInterfaceStubVtbl *vtbl, REFIID delegating_iid,
                                            LPPSFACTORYBUFFER pPSFactory, LPRPCSTUBBUFFER *ppStub) DECLSPEC_HIDDEN;

const MIDL_SERVER_INFO *CStdStubBuffer_GetServerInfo(IRpcStubBuffer *iface) DECLSPEC_HIDDEN;

extern const IRpcStubBufferVtbl CStdStubBuffer_Vtbl DECLSPEC_HIDDEN;
extern const IRpcStubBufferVtbl CStdStubBuffer_Delegating_Vtbl DECLSPEC_HIDDEN;

BOOL fill_delegated_proxy_table(IUnknownVtbl *vtbl, DWORD num) DECLSPEC_HIDDEN;
HRESULT create_proxy(REFIID iid, IUnknown *pUnkOuter, IRpcProxyBuffer **pproxy, void **ppv) DECLSPEC_HIDDEN;
HRESULT create_stub(REFIID iid, IUnknown *pUnk, IRpcStubBuffer **ppstub) DECLSPEC_HIDDEN;
BOOL fill_stubless_table(IUnknownVtbl *vtbl, DWORD num) DECLSPEC_HIDDEN;
IUnknownVtbl *get_delegating_vtbl(DWORD num_methods) DECLSPEC_HIDDEN;
void release_delegating_vtbl(IUnknownVtbl *vtbl) DECLSPEC_HIDDEN;

#endif  /* __WINE_CPSF_H */
