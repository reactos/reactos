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

HRESULT WINAPI StdProxy_Construct(REFIID riid,
				  LPUNKNOWN pUnkOuter,
				  const ProxyFileInfo *ProxyInfo,
				  int Index,
				  LPPSFACTORYBUFFER pPSFactory,
				  LPRPCPROXYBUFFER *ppProxy,
				  LPVOID *ppvObj);

HRESULT WINAPI CStdStubBuffer_Construct(REFIID riid,
					LPUNKNOWN pUnkServer,
					PCInterfaceName name,
					CInterfaceStubVtbl *vtbl,
					LPPSFACTORYBUFFER pPSFactory,
					LPRPCSTUBBUFFER *ppStub);

HRESULT WINAPI CStdStubBuffer_Delegating_Construct(REFIID riid,
                                                   LPUNKNOWN pUnkServer,
                                                   PCInterfaceName name,
                                                   CInterfaceStubVtbl *vtbl,
                                                   REFIID delegating_iid,
                                                   LPPSFACTORYBUFFER pPSFactory,
                                                   LPRPCSTUBBUFFER *ppStub);

const MIDL_SERVER_INFO *CStdStubBuffer_GetServerInfo(IRpcStubBuffer *iface);

const IRpcStubBufferVtbl CStdStubBuffer_Vtbl;
const IRpcStubBufferVtbl CStdStubBuffer_Delegating_Vtbl;

void create_delegating_vtbl(DWORD num_methods);

HRESULT create_stub(REFIID iid, IUnknown *pUnk, IRpcStubBuffer **ppstub);

#endif  /* __WINE_CPSF_H */
