/*
 *	OLE2 COM objects
 *
 *	Copyright 1998 Eric Kohl
 *      Copyright 1999 Francis Beaudet
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
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "wine/debug.h"
#include "ole2.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define INITIAL_SINKS 10

/**************************************************************************
 *  OleAdviseHolderImpl Implementation
 */
typedef struct OleAdviseHolderImpl
{
  const IOleAdviseHolderVtbl *lpVtbl;

  DWORD ref;

  DWORD         maxSinks;
  IAdviseSink** arrayOfSinks;

} OleAdviseHolderImpl;

/**************************************************************************
 *  OleAdviseHolderImpl_Destructor
 */
static void OleAdviseHolderImpl_Destructor(
  OleAdviseHolderImpl* ptrToDestroy)
{
  DWORD index;
  TRACE("%p\n", ptrToDestroy);

  for (index = 0; index < ptrToDestroy->maxSinks; index++)
  {
    if (ptrToDestroy->arrayOfSinks[index]!=0)
    {
      IAdviseSink_Release(ptrToDestroy->arrayOfSinks[index]);
      ptrToDestroy->arrayOfSinks[index] = NULL;
    }
  }

  HeapFree(GetProcessHeap(),
	   0,
	   ptrToDestroy->arrayOfSinks);


  HeapFree(GetProcessHeap(),
	   0,
	   ptrToDestroy);
}

/**************************************************************************
 *  OleAdviseHolderImpl_QueryInterface
 */
static HRESULT WINAPI OleAdviseHolderImpl_QueryInterface(
  LPOLEADVISEHOLDER iface,
  REFIID            riid,
  LPVOID*           ppvObj)
{
  OleAdviseHolderImpl *This = (OleAdviseHolderImpl *)iface;
  TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppvObj);
  /*
   * Sanity check
   */
  if (ppvObj==NULL)
    return E_POINTER;

  *ppvObj = NULL;

  if (IsEqualIID(riid, &IID_IUnknown))
  {
    /* IUnknown */
    *ppvObj = This;
  }
  else if(IsEqualIID(riid, &IID_IOleAdviseHolder))
  {
    /* IOleAdviseHolder */
    *ppvObj = (IOleAdviseHolder*) This;
  }

  if(*ppvObj == NULL)
    return E_NOINTERFACE;

  /*
   * A successful QI always increments the reference count.
   */
  IUnknown_AddRef((IUnknown*)*ppvObj);

  return S_OK;
}

/******************************************************************************
 * OleAdviseHolderImpl_AddRef
 */
static ULONG WINAPI OleAdviseHolderImpl_AddRef(
  LPOLEADVISEHOLDER iface)
{
  OleAdviseHolderImpl *This = (OleAdviseHolderImpl *)iface;
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p)->(ref=%ld)\n", This, ref - 1);

  return ref;
}

/******************************************************************************
 * OleAdviseHolderImpl_Release
 */
static ULONG WINAPI OleAdviseHolderImpl_Release(
  LPOLEADVISEHOLDER iface)
{
  OleAdviseHolderImpl *This = (OleAdviseHolderImpl *)iface;
  ULONG ref;
  TRACE("(%p)->(ref=%ld)\n", This, This->ref);
  ref = InterlockedDecrement(&This->ref);

  if (ref == 0) OleAdviseHolderImpl_Destructor(This);

  return ref;
}

/******************************************************************************
 * OleAdviseHolderImpl_Advise
 */
static HRESULT WINAPI OleAdviseHolderImpl_Advise(
  LPOLEADVISEHOLDER iface,
  IAdviseSink*      pAdvise,
  DWORD*            pdwConnection)
{
  DWORD index;

  OleAdviseHolderImpl *This = (OleAdviseHolderImpl *)iface;

  TRACE("(%p)->(%p, %p)\n", This, pAdvise, pdwConnection);

  /*
   * Sanity check
   */
  if (pdwConnection==NULL)
    return E_POINTER;

  *pdwConnection = 0;

  /*
   * Find a free spot in the array.
   */
  for (index = 0; index < This->maxSinks; index++)
  {
    if (This->arrayOfSinks[index]==NULL)
      break;
  }

  /*
   * If the array is full, we need to grow it.
   */
  if (index == This->maxSinks)
  {
    DWORD i;

    This->maxSinks+=INITIAL_SINKS;

    This->arrayOfSinks = HeapReAlloc(GetProcessHeap(),
				     0,
				     This->arrayOfSinks,
				     This->maxSinks*sizeof(IAdviseSink*));

    for (i=index;i < This->maxSinks; i++)
      This->arrayOfSinks[i]=0;
  }

  /*
   * Store the new sink
   */
  This->arrayOfSinks[index] = pAdvise;

  if (This->arrayOfSinks[index]!=NULL)
    IAdviseSink_AddRef(This->arrayOfSinks[index]);

  /*
   * Return the index as the cookie.
   * Since 0 is not a valid cookie, we will increment by
   * 1 the index in the table.
   */
  *pdwConnection = index+1;

  return S_OK;
}

/******************************************************************************
 * OleAdviseHolderImpl_Unadvise
 */
static HRESULT WINAPI OleAdviseHolderImpl_Unadvise(
  LPOLEADVISEHOLDER iface,
  DWORD             dwConnection)
{
  OleAdviseHolderImpl *This = (OleAdviseHolderImpl *)iface;

  TRACE("(%p)->(%lu)\n", This, dwConnection);

  /*
   * So we don't return 0 as a cookie, the index was
   * incremented by 1 in OleAdviseHolderImpl_Advise
   * we have to compensate.
   */
  dwConnection--;

  /*
   * Check for invalid cookies.
   */
  if (dwConnection >= This->maxSinks)
    return OLE_E_NOCONNECTION;

  if (This->arrayOfSinks[dwConnection] == NULL)
    return OLE_E_NOCONNECTION;

  /*
   * Release the sink and mark the spot in the list as free.
   */
  IAdviseSink_Release(This->arrayOfSinks[dwConnection]);
  This->arrayOfSinks[dwConnection] = NULL;

  return S_OK;
}

/******************************************************************************
 * OleAdviseHolderImpl_EnumAdvise
 */
static HRESULT WINAPI
OleAdviseHolderImpl_EnumAdvise (LPOLEADVISEHOLDER iface, IEnumSTATDATA **ppenumAdvise)
{
    OleAdviseHolderImpl *This = (OleAdviseHolderImpl *)iface;
    FIXME("(%p)->(%p)\n", This, ppenumAdvise);

    *ppenumAdvise = NULL;

    return S_OK;
}

/******************************************************************************
 * OleAdviseHolderImpl_SendOnRename
 */
static HRESULT WINAPI
OleAdviseHolderImpl_SendOnRename (LPOLEADVISEHOLDER iface, IMoniker *pmk)
{
    OleAdviseHolderImpl *This = (OleAdviseHolderImpl *)iface;
    FIXME("(%p)->(%p)\n", This, pmk);


    return S_OK;
}

/******************************************************************************
 * OleAdviseHolderImpl_SendOnSave
 */
static HRESULT WINAPI
OleAdviseHolderImpl_SendOnSave (LPOLEADVISEHOLDER iface)
{
    OleAdviseHolderImpl *This = (OleAdviseHolderImpl *)iface;
    FIXME("(%p)\n", This);

    return S_OK;
}

/******************************************************************************
 * OleAdviseHolderImpl_SendOnClose
 */
static HRESULT WINAPI
OleAdviseHolderImpl_SendOnClose (LPOLEADVISEHOLDER iface)
{
    OleAdviseHolderImpl *This = (OleAdviseHolderImpl *)iface;
    FIXME("(%p)\n", This);


    return S_OK;
}

/**************************************************************************
 *  OleAdviseHolderImpl_VTable
 */
static const IOleAdviseHolderVtbl oahvt =
{
    OleAdviseHolderImpl_QueryInterface,
    OleAdviseHolderImpl_AddRef,
    OleAdviseHolderImpl_Release,
    OleAdviseHolderImpl_Advise,
    OleAdviseHolderImpl_Unadvise,
    OleAdviseHolderImpl_EnumAdvise,
    OleAdviseHolderImpl_SendOnRename,
    OleAdviseHolderImpl_SendOnSave,
    OleAdviseHolderImpl_SendOnClose
};

/**************************************************************************
 *  OleAdviseHolderImpl_Constructor
 */

static LPOLEADVISEHOLDER OleAdviseHolderImpl_Constructor(void)
{
  OleAdviseHolderImpl* lpoah;
  DWORD                index;

  lpoah = HeapAlloc(GetProcessHeap(), 0, sizeof(OleAdviseHolderImpl));

  lpoah->lpVtbl = &oahvt;
  lpoah->ref = 1;
  lpoah->maxSinks = INITIAL_SINKS;
  lpoah->arrayOfSinks = HeapAlloc(GetProcessHeap(),
				  0,
				  lpoah->maxSinks * sizeof(IAdviseSink*));

  for (index = 0; index < lpoah->maxSinks; index++)
    lpoah->arrayOfSinks[index]=0;

  TRACE("returning %p\n", lpoah);
  return (LPOLEADVISEHOLDER)lpoah;
}

/**************************************************************************
 *  DataAdviseHolder Implementation
 */
typedef struct DataAdviseConnection {
  IAdviseSink *sink;
  FORMATETC fmat;
  DWORD advf;
} DataAdviseConnection;

typedef struct DataAdviseHolder
{
  const IDataAdviseHolderVtbl *lpVtbl;

  DWORD                 ref;
  DWORD                 maxCons;
  DataAdviseConnection* Connections;
} DataAdviseHolder;

/******************************************************************************
 * DataAdviseHolder_Destructor
 */
static void DataAdviseHolder_Destructor(DataAdviseHolder* ptrToDestroy)
{
  DWORD index;
  TRACE("%p\n", ptrToDestroy);

  for (index = 0; index < ptrToDestroy->maxCons; index++)
  {
    if (ptrToDestroy->Connections[index].sink != NULL)
    {
      IAdviseSink_Release(ptrToDestroy->Connections[index].sink);
      ptrToDestroy->Connections[index].sink = NULL;
    }
  }

  HeapFree(GetProcessHeap(), 0, ptrToDestroy->Connections);
  HeapFree(GetProcessHeap(), 0, ptrToDestroy);
}

/************************************************************************
 * DataAdviseHolder_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI DataAdviseHolder_QueryInterface(
  IDataAdviseHolder*      iface,
  REFIID                  riid,
  void**                  ppvObject)
{
  DataAdviseHolder *This = (DataAdviseHolder *)iface;
  TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppvObject);
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
  if ( (memcmp(&IID_IUnknown, riid, sizeof(IID_IUnknown)) == 0) ||
       (memcmp(&IID_IDataAdviseHolder, riid, sizeof(IID_IDataAdviseHolder)) == 0)  )
  {
    *ppvObject = iface;
  }

  /*
   * Check that we obtained an interface.
   */
  if ((*ppvObject)==0)
  {
    return E_NOINTERFACE;
  }

  /*
   * Query Interface always increases the reference count by one when it is
   * successful.
   */
  IUnknown_AddRef((IUnknown*)*ppvObject);

  return S_OK;
}

/************************************************************************
 * DataAdviseHolder_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI       DataAdviseHolder_AddRef(
  IDataAdviseHolder*      iface)
{
  DataAdviseHolder *This = (DataAdviseHolder *)iface;
  TRACE("(%p) (ref=%ld)\n", This, This->ref);
  return InterlockedIncrement(&This->ref);
}

/************************************************************************
 * DataAdviseHolder_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataAdviseHolder_Release(
  IDataAdviseHolder*      iface)
{
  DataAdviseHolder *This = (DataAdviseHolder *)iface;
  ULONG ref;
  TRACE("(%p) (ref=%ld)\n", This, This->ref);

  /*
   * Decrease the reference count on this object.
   */
  ref = InterlockedDecrement(&This->ref);

  /*
   * If the reference count goes down to 0, perform suicide.
   */
  if (ref==0) DataAdviseHolder_Destructor(This);

  return ref;
}

/************************************************************************
 * DataAdviseHolder_Advise
 *
 */
static HRESULT WINAPI DataAdviseHolder_Advise(
  IDataAdviseHolder*      iface,
  IDataObject*            pDataObject,
  FORMATETC*              pFetc,
  DWORD                   advf,
  IAdviseSink*            pAdvise,
  DWORD*                  pdwConnection)
{
  DWORD index;

  DataAdviseHolder *This = (DataAdviseHolder *)iface;

  TRACE("(%p)->(%p, %p, %08lx, %p, %p)\n", This, pDataObject, pFetc, advf,
	pAdvise, pdwConnection);
  /*
   * Sanity check
   */
  if (pdwConnection==NULL)
    return E_POINTER;

  *pdwConnection = 0;

  /*
   * Find a free spot in the array.
   */
  for (index = 0; index < This->maxCons; index++)
  {
    if (This->Connections[index].sink == NULL)
      break;
  }

  /*
   * If the array is full, we need to grow it.
   */
  if (index == This->maxCons)
  {
    This->maxCons+=INITIAL_SINKS;
    This->Connections = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				    This->Connections,
				    This->maxCons*sizeof(DataAdviseConnection));
  }
  /*
   * Store the new sink
   */
  This->Connections[index].sink = pAdvise;
  memcpy(&(This->Connections[index].fmat), pFetc, sizeof(FORMATETC));
  This->Connections[index].advf = advf;

  if (This->Connections[index].sink != NULL) {
    IAdviseSink_AddRef(This->Connections[index].sink);
    if(advf & ADVF_PRIMEFIRST) {
      IDataAdviseHolder_SendOnDataChange(iface, pDataObject, 0, advf);
    }
  }
  /*
   * Return the index as the cookie.
   * Since 0 is not a valid cookie, we will increment by
   * 1 the index in the table.
   */
  *pdwConnection = index+1;

  return S_OK;
}

/******************************************************************************
 * DataAdviseHolder_Unadvise
 */
static HRESULT WINAPI     DataAdviseHolder_Unadvise(
  IDataAdviseHolder*      iface,
  DWORD                   dwConnection)
{
  DataAdviseHolder *This = (DataAdviseHolder *)iface;

  TRACE("(%p)->(%lu)\n", This, dwConnection);

  /*
   * So we don't return 0 as a cookie, the index was
   * incremented by 1 in OleAdviseHolderImpl_Advise
   * we have to compensate.
   */
  dwConnection--;

  /*
   * Check for invalid cookies.
   */
  if (dwConnection >= This->maxCons)
    return OLE_E_NOCONNECTION;

  if (This->Connections[dwConnection].sink == NULL)
    return OLE_E_NOCONNECTION;

  /*
   * Release the sink and mark the spot in the list as free.
   */
  IAdviseSink_Release(This->Connections[dwConnection].sink);
  memset(&(This->Connections[dwConnection]), 0, sizeof(DataAdviseConnection));
  return S_OK;
}

static HRESULT WINAPI     DataAdviseHolder_EnumAdvise(
  IDataAdviseHolder*      iface,
  IEnumSTATDATA**         ppenumAdvise)
{
  DataAdviseHolder *This = (DataAdviseHolder *)iface;

  FIXME("(%p)->(%p)\n", This, ppenumAdvise);
  return E_NOTIMPL;
}

/******************************************************************************
 * DataAdviseHolder_SendOnDataChange
 */
static HRESULT WINAPI     DataAdviseHolder_SendOnDataChange(
  IDataAdviseHolder*      iface,
  IDataObject*            pDataObject,
  DWORD                   dwReserved,
  DWORD                   advf)
{
  DataAdviseHolder *This = (DataAdviseHolder *)iface;
  DWORD index;
  STGMEDIUM stg;
  HRESULT res;

  TRACE("(%p)->(%p,%08lx,%08lx)\n", This, pDataObject, dwReserved, advf);

  for(index = 0; index < This->maxCons; index++) {
    if(This->Connections[index].sink != NULL) {
      if(!(This->Connections[index].advf & ADVF_NODATA)) {
	TRACE("Calling IDataObject_GetData\n");
	res = IDataObject_GetData(pDataObject,
				  &(This->Connections[index].fmat),
				  &stg);
	TRACE("returns %08lx\n", res);
      }
      TRACE("Calling IAdviseSink_OnDataChange\n");
      IAdviseSink_OnDataChange(This->Connections[index].sink,
				     &(This->Connections[index].fmat),
				     &stg);
      TRACE("Done IAdviseSink_OnDataChange\n");
      if(This->Connections[index].advf & ADVF_ONLYONCE) {
	TRACE("Removing connection\n");
	DataAdviseHolder_Unadvise(iface, index+1);
      }
    }
  }
  return S_OK;
}

/**************************************************************************
 *  DataAdviseHolderImpl_VTable
 */
static const IDataAdviseHolderVtbl DataAdviseHolderImpl_VTable =
{
  DataAdviseHolder_QueryInterface,
  DataAdviseHolder_AddRef,
  DataAdviseHolder_Release,
  DataAdviseHolder_Advise,
  DataAdviseHolder_Unadvise,
  DataAdviseHolder_EnumAdvise,
  DataAdviseHolder_SendOnDataChange
};

/******************************************************************************
 * DataAdviseHolder_Constructor
 */
static IDataAdviseHolder* DataAdviseHolder_Constructor(void)
{
  DataAdviseHolder* newHolder;

  newHolder = HeapAlloc(GetProcessHeap(), 0, sizeof(DataAdviseHolder));

  newHolder->lpVtbl = &DataAdviseHolderImpl_VTable;
  newHolder->ref = 1;
  newHolder->maxCons = INITIAL_SINKS;
  newHolder->Connections = HeapAlloc(GetProcessHeap(),
				     HEAP_ZERO_MEMORY,
				     newHolder->maxCons *
				     sizeof(DataAdviseConnection));

  TRACE("returning %p\n", newHolder);
  return (IDataAdviseHolder*)newHolder;
}

/***********************************************************************
 * API functions
 */

/***********************************************************************
 * CreateOleAdviseHolder [OLE32.@]
 */
HRESULT WINAPI CreateOleAdviseHolder(
  LPOLEADVISEHOLDER *ppOAHolder)
{
  TRACE("(%p)\n", ppOAHolder);

  /*
   * Sanity check,
   */
  if (ppOAHolder==NULL)
    return E_POINTER;

  *ppOAHolder = OleAdviseHolderImpl_Constructor ();

  if (*ppOAHolder != NULL)
    return S_OK;

  return E_OUTOFMEMORY;
}

/******************************************************************************
 *              CreateDataAdviseHolder        [OLE32.@]
 */
HRESULT WINAPI CreateDataAdviseHolder(
  LPDATAADVISEHOLDER* ppDAHolder)
{
  TRACE("(%p)\n", ppDAHolder);

  /*
   * Sanity check,
   */
  if (ppDAHolder==NULL)
    return E_POINTER;

  *ppDAHolder = DataAdviseHolder_Constructor();

  if (*ppDAHolder != NULL)
    return S_OK;

  return E_OUTOFMEMORY;
}
