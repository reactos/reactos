/*
 * Implementation of a generic ConnectionPoint object.
 *
 * Copyright 2000 Huw D M Davies for CodeWeavers
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
 *
 * NOTES:
 * See one exported function here is CreateConnectionPoint, see
 * comments just above that function for information.
 */

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "ole2.h"
#include "olectl.h"
#include "connpt.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define MAXSINKS 10

/************************************************************************
 * Implementation of IConnectionPoint
 */
typedef struct ConnectionPointImpl {

  ICOM_VTABLE(IConnectionPoint)       *lpvtbl;

  /* IUnknown of our main object*/
  IUnknown *Obj;

  /* Reference count */
  DWORD ref;

  /* IID of sink interface */
  IID iid;

  /* Array of sink IUnknowns */
  IUnknown **sinks;
  DWORD maxSinks;

  DWORD nSinks;
} ConnectionPointImpl;

static ICOM_VTABLE(IConnectionPoint) ConnectionPointImpl_VTable;


/************************************************************************
 * Implementation of IEnumConnections
 */
typedef struct EnumConnectionsImpl {

  ICOM_VTABLE(IEnumConnections)       *lpvtbl;

  DWORD ref;

  /* IUnknown of ConnectionPoint, used for ref counting */
  IUnknown *pUnk;

  /* Connection Data */
  CONNECTDATA *pCD;
  DWORD nConns;

  /* Next connection to enumerate from */
  DWORD nCur;

} EnumConnectionsImpl;

static EnumConnectionsImpl *EnumConnectionsImpl_Construct(IUnknown *pUnk,
							  DWORD nSinks,
							  CONNECTDATA *pCD);


/************************************************************************
 * ConnectionPointImpl_Construct
 */
static ConnectionPointImpl *ConnectionPointImpl_Construct(IUnknown *pUnk,
							  REFIID riid)
{
  ConnectionPointImpl *Obj;

  Obj = HeapAlloc(GetProcessHeap(), 0, sizeof(*Obj));
  Obj->lpvtbl = &ConnectionPointImpl_VTable;
  Obj->Obj = pUnk;
  Obj->ref = 1;
  Obj->iid =  *riid;
  Obj->maxSinks = MAXSINKS;
  Obj->sinks = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			 sizeof(IUnknown*) * MAXSINKS);
  Obj->nSinks = 0;
  return Obj;
}

/************************************************************************
 * ConnectionPointImpl_Destroy
 */
static void ConnectionPointImpl_Destroy(ConnectionPointImpl *Obj)
{
  DWORD i;
  for(i = 0; i < Obj->maxSinks; i++) {
    if(Obj->sinks[i]) {
      IUnknown_Release(Obj->sinks[i]);
      Obj->sinks[i] = NULL;
    }
  }
  HeapFree(GetProcessHeap(), 0, Obj->sinks);
  HeapFree(GetProcessHeap(), 0, Obj);
  return;
}

static ULONG WINAPI ConnectionPointImpl_AddRef(IConnectionPoint* iface);
/************************************************************************
 * ConnectionPointImpl_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI ConnectionPointImpl_QueryInterface(
  IConnectionPoint*  iface,
  REFIID  riid,
  void**  ppvObject)
{
  ICOM_THIS(ConnectionPointImpl, iface);
  TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppvObject);

  /*
   * Perform a sanity check on the parameters.
   */
  if ( (This==0) || (ppvObject==0) )
    return E_INVALIDARG;

  /*
   * Initialize the return parameter.
   */
  *ppvObject = 0;

  /*
   * Compare the riid with the interface IDs implemented by this object.
   */
  if (memcmp(&IID_IUnknown, riid, sizeof(IID_IUnknown)) == 0)
  {
    *ppvObject = (IConnectionPoint*)This;
  }
  else if (memcmp(&IID_IConnectionPoint, riid, sizeof(IID_IConnectionPoint)) == 0)
  {
    *ppvObject = (IConnectionPoint*)This;
  }

  /*
   * Check that we obtained an interface.
   */
  if ((*ppvObject)==0)
  {
    FIXME("() : asking for un supported interface %s\n",debugstr_guid(riid));
    return E_NOINTERFACE;
  }

  /*
   * Query Interface always increases the reference count by one when it is
   * successful
   */
  ConnectionPointImpl_AddRef((IConnectionPoint*)This);

  return S_OK;
}


/************************************************************************
 * ConnectionPointImpl_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI ConnectionPointImpl_AddRef(IConnectionPoint* iface)
{
  ICOM_THIS(ConnectionPointImpl, iface);
  TRACE("(%p)->(ref=%ld)\n", This, This->ref);
  This->ref++;

  return This->ref;
}

/************************************************************************
 * ConnectionPointImpl_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI ConnectionPointImpl_Release(
      IConnectionPoint* iface)
{
  ICOM_THIS(ConnectionPointImpl, iface);
  TRACE("(%p)->(ref=%ld)\n", This, This->ref);

  /*
   * Decrease the reference count on this object.
   */
  This->ref--;

  /*
   * If the reference count goes down to 0, perform suicide.
   */
  if (This->ref==0)
  {
    ConnectionPointImpl_Destroy(This);

    return 0;
  }

  return This->ref;
}

/************************************************************************
 * ConnectionPointImpl_GetConnectionInterface (IConnectionPoint)
 *
 */
static HRESULT WINAPI ConnectionPointImpl_GetConnectionInterface(
					       IConnectionPoint *iface,
					       IID              *piid)
{
  ICOM_THIS(ConnectionPointImpl, iface);
  TRACE("(%p)->(%p) returning %s\n", This, piid, debugstr_guid(&(This->iid)));
  *piid = This->iid;
  return S_OK;
}

/************************************************************************
 * ConnectionPointImpl_GetConnectionPointContainer (IConnectionPoint)
 *
 */
static HRESULT WINAPI ConnectionPointImpl_GetConnectionPointContainer(
				      IConnectionPoint           *iface,
				      IConnectionPointContainer  **ppCPC)
{
  ICOM_THIS(ConnectionPointImpl, iface);
  TRACE("(%p)->(%p)\n", This, ppCPC);

  return IUnknown_QueryInterface(This->Obj,
				 &IID_IConnectionPointContainer,
				 (LPVOID)ppCPC);
}

/************************************************************************
 * ConnectionPointImpl_Advise (IConnectionPoint)
 *
 */
static HRESULT WINAPI ConnectionPointImpl_Advise(IConnectionPoint *iface,
						 IUnknown *lpUnk,
						 DWORD *pdwCookie)
{
  DWORD i;
  ICOM_THIS(ConnectionPointImpl, iface);
  IUnknown *lpSink;
  TRACE("(%p)->(%p, %p)\n", This, lpUnk, pdwCookie);

  *pdwCookie = 0;
  if(FAILED(IUnknown_QueryInterface(lpUnk, &This->iid, (LPVOID)&lpSink)))
    return CONNECT_E_CANNOTCONNECT;

  for(i = 0; i < This->maxSinks; i++) {
    if(This->sinks[i] == NULL)
      break;
  }
  if(i == This->maxSinks) {
    This->maxSinks += MAXSINKS;
    This->sinks = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, This->sinks,
			      This->maxSinks * sizeof(IUnknown *));
  }
  This->sinks[i] = lpSink;
  This->nSinks++;
  *pdwCookie = i + 1;
  return S_OK;
}


/************************************************************************
 * ConnectionPointImpl_Unadvise (IConnectionPoint)
 *
 */
static HRESULT WINAPI ConnectionPointImpl_Unadvise(IConnectionPoint *iface,
						   DWORD dwCookie)
{
  ICOM_THIS(ConnectionPointImpl, iface);
  TRACE("(%p)->(%ld)\n", This, dwCookie);

  if(dwCookie == 0 || dwCookie > This->maxSinks) return E_INVALIDARG;

  if(This->sinks[dwCookie-1] == NULL) return CONNECT_E_NOCONNECTION;

  IUnknown_Release(This->sinks[dwCookie-1]);
  This->sinks[dwCookie-1] = NULL;
  This->nSinks--;
  return S_OK;
}

/************************************************************************
 * ConnectionPointImpl_EnumConnections (IConnectionPoint)
 *
 */
static HRESULT WINAPI ConnectionPointImpl_EnumConnections(
						    IConnectionPoint *iface,
						    LPENUMCONNECTIONS *ppEnum)
{
  ICOM_THIS(ConnectionPointImpl, iface);
  CONNECTDATA *pCD;
  DWORD i, nextslot;
  EnumConnectionsImpl *EnumObj;
  HRESULT hr;

  TRACE("(%p)->(%p)\n", This, ppEnum);

  *ppEnum = NULL;

  if(This->nSinks == 0) return OLE_E_NOCONNECTION;

  pCD = HeapAlloc(GetProcessHeap(), 0, sizeof(CONNECTDATA) * This->nSinks);

  for(i = 0, nextslot = 0; i < This->maxSinks; i++) {
    if(This->sinks[i] != NULL) {
      pCD[nextslot].pUnk = This->sinks[i];
      pCD[nextslot].dwCookie = i + 1;
      nextslot++;
    }
  }
  assert(nextslot == This->nSinks);

  /* Bump the ref count of this object up by one.  It gets Released in
     IEnumConnections_Release */
  IUnknown_AddRef((IUnknown*)This);

  EnumObj = EnumConnectionsImpl_Construct((IUnknown*)This, This->nSinks, pCD);
  hr = IEnumConnections_QueryInterface((IEnumConnections*)EnumObj,
				  &IID_IEnumConnections, (LPVOID)ppEnum);
  IEnumConnections_Release((IEnumConnections*)EnumObj);

  HeapFree(GetProcessHeap(), 0, pCD);
  return hr;
}

static ICOM_VTABLE(IConnectionPoint) ConnectionPointImpl_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  ConnectionPointImpl_QueryInterface,
  ConnectionPointImpl_AddRef,
  ConnectionPointImpl_Release,
  ConnectionPointImpl_GetConnectionInterface,
  ConnectionPointImpl_GetConnectionPointContainer,
  ConnectionPointImpl_Advise,
  ConnectionPointImpl_Unadvise,
  ConnectionPointImpl_EnumConnections
};


static ICOM_VTABLE(IEnumConnections) EnumConnectionsImpl_VTable;
static ULONG WINAPI EnumConnectionsImpl_AddRef(IEnumConnections* iface);

/************************************************************************
 * EnumConnectionsImpl_Construct
 */
static EnumConnectionsImpl *EnumConnectionsImpl_Construct(IUnknown *pUnk,
							  DWORD nSinks,
							  CONNECTDATA *pCD)
{
  EnumConnectionsImpl *Obj = HeapAlloc(GetProcessHeap(), 0, sizeof(*Obj));
  DWORD i;

  Obj->lpvtbl = &EnumConnectionsImpl_VTable;
  Obj->ref = 1;
  Obj->pUnk = pUnk;
  Obj->pCD = HeapAlloc(GetProcessHeap(), 0, nSinks * sizeof(CONNECTDATA));
  Obj->nConns = nSinks;
  Obj->nCur = 0;

  for(i = 0; i < nSinks; i++) {
    Obj->pCD[i] = pCD[i];
    IUnknown_AddRef(Obj->pCD[i].pUnk);
  }
  return Obj;
}

/************************************************************************
 * EnumConnectionsImpl_Destroy
 */
static void EnumConnectionsImpl_Destroy(EnumConnectionsImpl *Obj)
{
  DWORD i;

  for(i = 0; i < Obj->nConns; i++)
    IUnknown_Release(Obj->pCD[i].pUnk);

  HeapFree(GetProcessHeap(), 0, Obj->pCD);
  HeapFree(GetProcessHeap(), 0, Obj);
  return;
}

/************************************************************************
 * EnumConnectionsImpl_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI EnumConnectionsImpl_QueryInterface(
  IEnumConnections*  iface,
  REFIID  riid,
  void**  ppvObject)
{
  ICOM_THIS(ConnectionPointImpl, iface);
  TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppvObject);

  /*
   * Perform a sanity check on the parameters.
   */
  if ( (This==0) || (ppvObject==0) )
    return E_INVALIDARG;

  /*
   * Initialize the return parameter.
   */
  *ppvObject = 0;

  /*
   * Compare the riid with the interface IDs implemented by this object.
   */
  if (memcmp(&IID_IUnknown, riid, sizeof(IID_IUnknown)) == 0)
  {
    *ppvObject = (IEnumConnections*)This;
  }
  else if (memcmp(&IID_IEnumConnections, riid, sizeof(IID_IEnumConnections)) == 0)
  {
    *ppvObject = (IEnumConnections*)This;
  }

  /*
   * Check that we obtained an interface.
   */
  if ((*ppvObject)==0)
  {
    FIXME("() : asking for un supported interface %s\n",debugstr_guid(riid));
    return E_NOINTERFACE;
  }

  /*
   * Query Interface always increases the reference count by one when it is
   * successful
   */
  EnumConnectionsImpl_AddRef((IEnumConnections*)This);

  return S_OK;
}


/************************************************************************
 * EnumConnectionsImpl_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI EnumConnectionsImpl_AddRef(IEnumConnections* iface)
{
  ICOM_THIS(EnumConnectionsImpl, iface);
  TRACE("(%p)->(ref=%ld)\n", This, This->ref);
  This->ref++;
  IUnknown_AddRef(This->pUnk);
  return This->ref;
}

/************************************************************************
 * EnumConnectionsImpl_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI EnumConnectionsImpl_Release(IEnumConnections* iface)
{
  ICOM_THIS(EnumConnectionsImpl, iface);
  TRACE("(%p)->(ref=%ld)\n", This, This->ref);

  IUnknown_Release(This->pUnk);

  /*
   * Decrease the reference count on this object.
   */
  This->ref--;

  /*
   * If the reference count goes down to 0, perform suicide.
   */
  if (This->ref==0)
  {
    EnumConnectionsImpl_Destroy(This);

    return 0;
  }

  return This->ref;
}

/************************************************************************
 * EnumConnectionsImpl_Next (IEnumConnections)
 *
 */
static HRESULT WINAPI EnumConnectionsImpl_Next(IEnumConnections* iface,
					       ULONG cConn, LPCONNECTDATA pCD,
					       ULONG *pEnum)
{
  ICOM_THIS(EnumConnectionsImpl, iface);
  DWORD nRet = 0;
  TRACE("(%p)->(%ld, %p, %p)\n", This, cConn, pCD, pEnum);

  if(pEnum == NULL) {
    if(cConn != 1)
      return E_POINTER;
  } else
    *pEnum = 0;

  if(This->nCur >= This->nConns)
    return S_FALSE;

  while(This->nCur < This->nConns && cConn) {
    *pCD++ = This->pCD[This->nCur];
    IUnknown_AddRef(This->pCD[This->nCur].pUnk);
    This->nCur++;
    cConn--;
    nRet++;
  }

  if(pEnum)
    *pEnum = nRet;

  return S_OK;
}


/************************************************************************
 * EnumConnectionsImpl_Skip (IEnumConnections)
 *
 */
static HRESULT WINAPI EnumConnectionsImpl_Skip(IEnumConnections* iface,
					       ULONG cSkip)
{
  ICOM_THIS(EnumConnectionsImpl, iface);
  TRACE("(%p)->(%ld)\n", This, cSkip);

  if(This->nCur + cSkip >= This->nConns)
    return S_FALSE;

  This->nCur += cSkip;

  return S_OK;
}


/************************************************************************
 * EnumConnectionsImpl_Reset (IEnumConnections)
 *
 */
static HRESULT WINAPI EnumConnectionsImpl_Reset(IEnumConnections* iface)
{
  ICOM_THIS(EnumConnectionsImpl, iface);
  TRACE("(%p)\n", This);

  This->nCur = 0;

  return S_OK;
}


/************************************************************************
 * EnumConnectionsImpl_Clone (IEnumConnections)
 *
 */
static HRESULT WINAPI EnumConnectionsImpl_Clone(IEnumConnections* iface,
						LPENUMCONNECTIONS *ppEnum)
{
  ICOM_THIS(EnumConnectionsImpl, iface);
  EnumConnectionsImpl *newObj;
  TRACE("(%p)->(%p)\n", This, ppEnum);

  newObj = EnumConnectionsImpl_Construct(This->pUnk, This->nConns, This->pCD);
  newObj->nCur = This->nCur;
  *ppEnum = (LPENUMCONNECTIONS)newObj;
  IUnknown_AddRef(This->pUnk);
  return S_OK;
}

static ICOM_VTABLE(IEnumConnections) EnumConnectionsImpl_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  EnumConnectionsImpl_QueryInterface,
  EnumConnectionsImpl_AddRef,
  EnumConnectionsImpl_Release,
  EnumConnectionsImpl_Next,
  EnumConnectionsImpl_Skip,
  EnumConnectionsImpl_Reset,
  EnumConnectionsImpl_Clone
};

/************************************************************************
 *
 *  The exported function to create the connection point.
 *  NB not a windows API
 *
 * PARAMS
 * pUnk [in] IUnknown of object to which the ConnectionPoint is associated.
 *           Needed to access IConnectionPointContainer.
 *
 * riid [in] IID of sink interface that this ConnectionPoint manages
 *
 * pCP [out] returns IConnectionPoint
 *
 */
HRESULT CreateConnectionPoint(IUnknown *pUnk, REFIID riid,
			      IConnectionPoint **pCP)
{
  ConnectionPointImpl *Obj;
  HRESULT hr;

  Obj = ConnectionPointImpl_Construct(pUnk, riid);
  if(!Obj) return E_OUTOFMEMORY;

  hr = IConnectionPoint_QueryInterface((IConnectionPoint *)Obj,
				       &IID_IConnectionPoint, (LPVOID)pCP);
  IConnectionPoint_Release((IConnectionPoint *)Obj);
  return hr;
}
