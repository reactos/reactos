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
    IUnknown base_obj;
    IRpcStubBuffer *base_stub;
    CStdStubBuffer stub_buffer;
} cstdstubbuffer_delegating_t;

HRESULT StdProxy_Construct(REFIID riid, LPUNKNOWN pUnkOuter, const ProxyFileInfo *ProxyInfo,
                           int Index, LPPSFACTORYBUFFER pPSFactory, LPRPCPROXYBUFFER *ppProxy,
                           LPVOID *ppvObj);
HRESULT WINAPI StdProxy_QueryInterface(IRpcProxyBuffer *iface, REFIID iid, void **obj);
ULONG WINAPI StdProxy_AddRef(IRpcProxyBuffer *iface);
HRESULT WINAPI StdProxy_Connect(IRpcProxyBuffer *iface, IRpcChannelBuffer *channel);
void WINAPI StdProxy_Disconnect(IRpcProxyBuffer *iface);

HRESULT CStdStubBuffer_Construct(REFIID riid, LPUNKNOWN pUnkServer, PCInterfaceName name,
                                 CInterfaceStubVtbl *vtbl, LPPSFACTORYBUFFER pPSFactory,
                                 LPRPCSTUBBUFFER *ppStub);

HRESULT CStdStubBuffer_Delegating_Construct(REFIID riid, LPUNKNOWN pUnkServer, PCInterfaceName name,
                                            CInterfaceStubVtbl *vtbl, REFIID delegating_iid,
                                            LPPSFACTORYBUFFER pPSFactory, LPRPCSTUBBUFFER *ppStub);

const MIDL_SERVER_INFO *CStdStubBuffer_GetServerInfo(IRpcStubBuffer *iface);

extern const IRpcStubBufferVtbl CStdStubBuffer_Vtbl;
extern const IRpcStubBufferVtbl CStdStubBuffer_Delegating_Vtbl;

BOOL fill_delegated_proxy_table(IUnknownVtbl *vtbl, DWORD num);
HRESULT create_proxy(REFIID iid, IUnknown *pUnkOuter, IRpcProxyBuffer **pproxy, void **ppv);
HRESULT create_stub(REFIID iid, IUnknown *pUnk, IRpcStubBuffer **ppstub);
BOOL fill_stubless_table(IUnknownVtbl *vtbl, DWORD num);
const IUnknownVtbl *get_delegating_vtbl(DWORD num_methods);

#define NB_THUNK_ENTRIES 1024

struct delegating_vtbl
{
    IUnknownVtbl vtbl;
    const void *methods[NB_THUNK_ENTRIES - 3];
};

extern const struct delegating_vtbl delegating_vtbl;

#endif  /* __WINE_CPSF_H */
