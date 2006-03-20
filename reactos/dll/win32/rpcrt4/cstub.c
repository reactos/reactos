/*
 * COM stub (CStdStubBuffer) implementation
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"

#include "objbase.h"

#include "rpcproxy.h"

#include "wine/debug.h"

#include "cpsf.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define STUB_HEADER(This) (((CInterfaceStubHeader*)((This)->lpVtbl))[-1])

HRESULT WINAPI CStdStubBuffer_Construct(REFIID riid,
                                       LPUNKNOWN pUnkServer,
                                       PCInterfaceName name,
                                       CInterfaceStubVtbl *vtbl,
                                       LPPSFACTORYBUFFER pPSFactory,
                                       LPRPCSTUBBUFFER *ppStub)
{
  CStdStubBuffer *This;

  TRACE("(%p,%p,%p,%p) %s\n", pUnkServer, vtbl, pPSFactory, ppStub, name);
  TRACE("iid=%s\n", debugstr_guid(vtbl->header.piid));
  TRACE("vtbl=%p\n", &vtbl->Vtbl);

  if (!IsEqualGUID(vtbl->header.piid, riid)) {
    ERR("IID mismatch during stub creation\n");
    return RPC_E_UNEXPECTED;
  }

  This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(CStdStubBuffer));
  if (!This) return E_OUTOFMEMORY;

  This->lpVtbl = &vtbl->Vtbl;
  This->RefCount = 1;
  This->pvServerObject = pUnkServer;
  This->pPSFactory = pPSFactory;
  *ppStub = (LPRPCSTUBBUFFER)This;

  IUnknown_AddRef(This->pvServerObject);
  IPSFactoryBuffer_AddRef(pPSFactory);
  return S_OK;
}

HRESULT WINAPI CStdStubBuffer_QueryInterface(LPRPCSTUBBUFFER iface,
                                            REFIID riid,
                                            LPVOID *obj)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  TRACE("(%p)->QueryInterface(%s,%p)\n",This,debugstr_guid(riid),obj);

  if (IsEqualGUID(&IID_IUnknown,riid) ||
      IsEqualGUID(&IID_IRpcStubBuffer,riid)) {
    *obj = This;
    This->RefCount++;
    return S_OK;
  }
  return E_NOINTERFACE;
}

ULONG WINAPI CStdStubBuffer_AddRef(LPRPCSTUBBUFFER iface)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  TRACE("(%p)->AddRef()\n",This);
  return ++(This->RefCount);
}

ULONG WINAPI NdrCStdStubBuffer_Release(LPRPCSTUBBUFFER iface,
                                      LPPSFACTORYBUFFER pPSF)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  TRACE("(%p)->Release()\n",This);

  if (!--(This->RefCount)) {
    IRpcStubBuffer_Disconnect(iface);
    if(This->pPSFactory)
        IPSFactoryBuffer_Release(This->pPSFactory);
    HeapFree(GetProcessHeap(),0,This);
    return 0;
  }
  return This->RefCount;
}

ULONG WINAPI NdrCStdStubBuffer2_Release(LPRPCSTUBBUFFER iface,
                                        LPPSFACTORYBUFFER pPSF)
{
    FIXME("Not implemented\n");
    return 0;
}

HRESULT WINAPI CStdStubBuffer_Connect(LPRPCSTUBBUFFER iface,
                                     LPUNKNOWN lpUnkServer)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  TRACE("(%p)->Connect(%p)\n",This,lpUnkServer);
  This->pvServerObject = lpUnkServer;
  return S_OK;
}

void WINAPI CStdStubBuffer_Disconnect(LPRPCSTUBBUFFER iface)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  TRACE("(%p)->Disconnect()\n",This);
  if (This->pvServerObject)
  {
    IUnknown_Release(This->pvServerObject);
    This->pvServerObject = NULL;
  }
}

HRESULT WINAPI CStdStubBuffer_Invoke(LPRPCSTUBBUFFER iface,
                                    PRPCOLEMESSAGE pMsg,
                                    LPRPCCHANNELBUFFER pChannel)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  DWORD dwPhase = STUB_UNMARSHAL;
  TRACE("(%p)->Invoke(%p,%p)\n",This,pMsg,pChannel);

  if (STUB_HEADER(This).pDispatchTable)
    STUB_HEADER(This).pDispatchTable[pMsg->iMethod](iface, pChannel, (PRPC_MESSAGE)pMsg, &dwPhase);
  else /* pure interpreted */
    NdrStubCall2(iface, pChannel, (PRPC_MESSAGE)pMsg, &dwPhase);
  return S_OK;
}

LPRPCSTUBBUFFER WINAPI CStdStubBuffer_IsIIDSupported(LPRPCSTUBBUFFER iface,
                                                    REFIID riid)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  TRACE("(%p)->IsIIDSupported(%s)\n",This,debugstr_guid(riid));
  return IsEqualGUID(STUB_HEADER(This).piid, riid) ? iface : NULL;
}

ULONG WINAPI CStdStubBuffer_CountRefs(LPRPCSTUBBUFFER iface)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  TRACE("(%p)->CountRefs()\n",This);
  return This->RefCount;
}

HRESULT WINAPI CStdStubBuffer_DebugServerQueryInterface(LPRPCSTUBBUFFER iface,
                                                       LPVOID *ppv)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  TRACE("(%p)->DebugServerQueryInterface(%p)\n",This,ppv);
  return S_OK;
}

void WINAPI CStdStubBuffer_DebugServerRelease(LPRPCSTUBBUFFER iface,
                                             LPVOID pv)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  TRACE("(%p)->DebugServerRelease(%p)\n",This,pv);
}

const IRpcStubBufferVtbl CStdStubBuffer_Vtbl =
{
    CStdStubBuffer_QueryInterface,
    CStdStubBuffer_AddRef,
    NULL,
    CStdStubBuffer_Connect,
    CStdStubBuffer_Disconnect,
    CStdStubBuffer_Invoke,
    CStdStubBuffer_IsIIDSupported,
    CStdStubBuffer_CountRefs,
    CStdStubBuffer_DebugServerQueryInterface,
    CStdStubBuffer_DebugServerRelease
};

const MIDL_SERVER_INFO *CStdStubBuffer_GetServerInfo(IRpcStubBuffer *iface)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  return STUB_HEADER(This).pServerInfo;
}
