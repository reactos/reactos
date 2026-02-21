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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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

#include "compobj_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define INITIAL_SINKS 10

static void release_statdata(STATDATA *data)
{
    CoTaskMemFree(data->formatetc.ptd);
    data->formatetc.ptd = NULL;

    if(data->pAdvSink)
    {
        IAdviseSink_Release(data->pAdvSink);
        data->pAdvSink = NULL;
    }
}

static HRESULT copy_statdata(STATDATA *dst, const STATDATA *src)
{
    HRESULT hr;

    hr = copy_formatetc( &dst->formatetc, &src->formatetc );
    if (FAILED(hr)) return hr;
    dst->advf = src->advf;
    dst->pAdvSink = src->pAdvSink;
    if (dst->pAdvSink) IAdviseSink_AddRef( dst->pAdvSink );
    dst->dwConnection = src->dwConnection;
    return S_OK;
}

/**************************************************************************
 *  EnumSTATDATA Implementation
 */

typedef struct
{
    IEnumSTATDATA IEnumSTATDATA_iface;
    LONG ref;

    ULONG index;
    DWORD num_of_elems;
    STATDATA *statdata;
    IUnknown *holder;
} EnumSTATDATA;

static inline EnumSTATDATA *impl_from_IEnumSTATDATA(IEnumSTATDATA *iface)
{
    return CONTAINING_RECORD(iface, EnumSTATDATA, IEnumSTATDATA_iface);
}

static HRESULT WINAPI EnumSTATDATA_QueryInterface(IEnumSTATDATA *iface, REFIID riid, void **ppv)
{
    TRACE("(%s, %p)\n", debugstr_guid(riid), ppv);
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IEnumSTATDATA))
    {
        IEnumSTATDATA_AddRef(iface);
        *ppv = iface;
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI EnumSTATDATA_AddRef(IEnumSTATDATA *iface)
{
    EnumSTATDATA *This = impl_from_IEnumSTATDATA(iface);
    TRACE("()\n");
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI EnumSTATDATA_Release(IEnumSTATDATA *iface)
{
    EnumSTATDATA *This = impl_from_IEnumSTATDATA(iface);
    LONG refs = InterlockedDecrement(&This->ref);
    TRACE("()\n");
    if (!refs)
    {
        DWORD i;
        for(i = 0; i < This->num_of_elems; i++)
            release_statdata(This->statdata + i);
        HeapFree(GetProcessHeap(), 0, This->statdata);
        if (This->holder) IUnknown_Release(This->holder);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return refs;
}

static HRESULT WINAPI EnumSTATDATA_Next(IEnumSTATDATA *iface, ULONG num, LPSTATDATA data,
                                        ULONG *fetched)
{
    EnumSTATDATA *This = impl_from_IEnumSTATDATA(iface);
    DWORD count = 0;
    HRESULT hr = S_OK;

    TRACE("%p, %lu, %p, %p.\n", iface, num, data, fetched);

    while(num--)
    {
        if (This->index >= This->num_of_elems)
        {
            hr = S_FALSE;
            break;
        }

        copy_statdata(data + count, This->statdata + This->index);

        count++;
        This->index++;
    }

    if (fetched) *fetched = count;

    return hr;
}

static HRESULT WINAPI EnumSTATDATA_Skip(IEnumSTATDATA *iface, ULONG num)
{
    EnumSTATDATA *This = impl_from_IEnumSTATDATA(iface);

    TRACE("%p, %lu.\n", iface, num);

    if(This->index + num >= This->num_of_elems)
    {
        This->index = This->num_of_elems;
        return S_FALSE;
    }

    This->index += num;
    return S_OK;
}

static HRESULT WINAPI EnumSTATDATA_Reset(IEnumSTATDATA *iface)
{
    EnumSTATDATA *This = impl_from_IEnumSTATDATA(iface);

    TRACE("()\n");

    This->index = 0;
    return S_OK;
}

static HRESULT WINAPI EnumSTATDATA_Clone(IEnumSTATDATA *iface, IEnumSTATDATA **ppenum)
{
    EnumSTATDATA *This = impl_from_IEnumSTATDATA(iface);

    return EnumSTATDATA_Construct(This->holder, This->index, This->num_of_elems, This->statdata,
                                  TRUE, ppenum);
}

static const IEnumSTATDATAVtbl EnumSTATDATA_VTable =
{
    EnumSTATDATA_QueryInterface,
    EnumSTATDATA_AddRef,
    EnumSTATDATA_Release,
    EnumSTATDATA_Next,
    EnumSTATDATA_Skip,
    EnumSTATDATA_Reset,
    EnumSTATDATA_Clone
};

HRESULT EnumSTATDATA_Construct(IUnknown *holder, ULONG index, DWORD array_len, STATDATA *data,
                               BOOL copy, IEnumSTATDATA **ppenum)
{
    EnumSTATDATA *This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    DWORD i, count;

    if (!This) return E_OUTOFMEMORY;

    This->IEnumSTATDATA_iface.lpVtbl = &EnumSTATDATA_VTable;
    This->ref = 1;
    This->index = index;

    if (copy)
    {
        This->statdata = HeapAlloc(GetProcessHeap(), 0, array_len * sizeof(*This->statdata));
        if(!This->statdata)
        {
            HeapFree(GetProcessHeap(), 0, This);
            return E_OUTOFMEMORY;
        }

        for(i = 0, count = 0; i < array_len; i++)
        {
            if(data[i].pAdvSink)
            {
                copy_statdata(This->statdata + count, data + i);
                count++;
            }
        }
    }
    else
    {
        This->statdata = data;
        count = array_len;
    }

    This->num_of_elems = count;
    This->holder = holder;
    if (holder) IUnknown_AddRef(holder);
    *ppenum = &This->IEnumSTATDATA_iface;
    return S_OK;
}

/**************************************************************************
 *  OleAdviseHolder Implementation
 */
typedef struct
{
    IOleAdviseHolder IOleAdviseHolder_iface;

    LONG ref;

    DWORD max_cons;
    STATDATA *connections;
} OleAdviseHolderImpl;

static inline OleAdviseHolderImpl *impl_from_IOleAdviseHolder(IOleAdviseHolder *iface)
{
    return CONTAINING_RECORD(iface, OleAdviseHolderImpl, IOleAdviseHolder_iface);
}

/**************************************************************************
 *  OleAdviseHolderImpl_Destructor
 */
static void OleAdviseHolderImpl_Destructor(OleAdviseHolderImpl *This)
{
    DWORD index;
    TRACE("%p\n", This);

    for (index = 0; index < This->max_cons; index++)
    {
        if (This->connections[index].pAdvSink != NULL)
            release_statdata(This->connections + index);
    }

    HeapFree(GetProcessHeap(), 0, This->connections);
    HeapFree(GetProcessHeap(), 0, This);
}

/**************************************************************************
 *  OleAdviseHolderImpl_QueryInterface
 */
static HRESULT WINAPI OleAdviseHolderImpl_QueryInterface(IOleAdviseHolder *iface,
                                                         REFIID iid, void **obj)
{
  OleAdviseHolderImpl *This = impl_from_IOleAdviseHolder(iface);
  TRACE("(%p)->(%s,%p)\n",This, debugstr_guid(iid), obj);

  if (obj == NULL)
    return E_POINTER;

  *obj = NULL;

  if (IsEqualIID(iid, &IID_IUnknown) ||
      IsEqualIID(iid, &IID_IOleAdviseHolder))
  {
    *obj = &This->IOleAdviseHolder_iface;
  }

  if(*obj == NULL)
    return E_NOINTERFACE;

  IUnknown_AddRef((IUnknown*)*obj);

  return S_OK;
}

/******************************************************************************
 * OleAdviseHolderImpl_AddRef
 */
static ULONG WINAPI OleAdviseHolderImpl_AddRef(IOleAdviseHolder *iface)
{
  OleAdviseHolderImpl *This = impl_from_IOleAdviseHolder(iface);
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("%p, refcount %lu.\n", iface, ref);

  return ref;
}

/******************************************************************************
 * OleAdviseHolderImpl_Release
 */
static ULONG WINAPI OleAdviseHolderImpl_Release(IOleAdviseHolder *iface)
{
  OleAdviseHolderImpl *This = impl_from_IOleAdviseHolder(iface);
  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("%p, refcount %lu.\n", iface, ref);

  if (ref == 0) OleAdviseHolderImpl_Destructor(This);

  return ref;
}

/******************************************************************************
 * OleAdviseHolderImpl_Advise
 */
static HRESULT WINAPI OleAdviseHolderImpl_Advise(IOleAdviseHolder *iface,
                                                 IAdviseSink *pAdvise,
                                                 DWORD *pdwConnection)
{
  DWORD index;
  OleAdviseHolderImpl *This = impl_from_IOleAdviseHolder(iface);
  STATDATA new_conn;
  static const FORMATETC empty_fmtetc = {0, NULL, 0, -1, 0};

  TRACE("(%p)->(%p, %p)\n", This, pAdvise, pdwConnection);

  if (pdwConnection==NULL)
    return E_POINTER;

  *pdwConnection = 0;

  for (index = 0; index < This->max_cons; index++)
  {
    if (This->connections[index].pAdvSink == NULL)
      break;
  }

  if (index == This->max_cons)
  {
    This->max_cons += INITIAL_SINKS;
    This->connections = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, This->connections,
                                    This->max_cons * sizeof(*This->connections));
  }

  new_conn.pAdvSink = pAdvise;
  new_conn.advf = 0;
  new_conn.formatetc = empty_fmtetc;
  new_conn.dwConnection = index + 1; /* 0 is not a valid cookie, so increment the index */

  copy_statdata(This->connections + index, &new_conn);

  *pdwConnection = new_conn.dwConnection;

  return S_OK;
}

/******************************************************************************
 * OleAdviseHolderImpl_Unadvise
 */
static HRESULT WINAPI OleAdviseHolderImpl_Unadvise(IOleAdviseHolder *iface,
                                                   DWORD dwConnection)
{
  OleAdviseHolderImpl *This = impl_from_IOleAdviseHolder(iface);
  DWORD index;

  TRACE("%p, %lu.\n", iface, dwConnection);

  /* The connection number is 1 more than the index, see OleAdviseHolder_Advise */
  index = dwConnection - 1;

  if (index >= This->max_cons || This->connections[index].pAdvSink == NULL)
     return OLE_E_NOCONNECTION;

  release_statdata(This->connections + index);

  return S_OK;
}

/******************************************************************************
 * OleAdviseHolderImpl_EnumAdvise
 */
static HRESULT WINAPI OleAdviseHolderImpl_EnumAdvise(IOleAdviseHolder *iface, IEnumSTATDATA **enum_advise)
{
    OleAdviseHolderImpl *This = impl_from_IOleAdviseHolder(iface);
    IUnknown *unk;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, enum_advise);

    IOleAdviseHolder_QueryInterface(iface, &IID_IUnknown, (void**)&unk);
    hr = EnumSTATDATA_Construct(unk, 0, This->max_cons, This->connections, TRUE, enum_advise);
    IUnknown_Release(unk);
    return hr;
}

/******************************************************************************
 * OleAdviseHolderImpl_SendOnRename
 */
static HRESULT WINAPI OleAdviseHolderImpl_SendOnRename(IOleAdviseHolder *iface, IMoniker *pmk)
{
    IEnumSTATDATA *pEnum;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", iface, pmk);

    hr = IOleAdviseHolder_EnumAdvise(iface, &pEnum);
    if (SUCCEEDED(hr))
    {
        STATDATA statdata;
        while (IEnumSTATDATA_Next(pEnum, 1, &statdata, NULL) == S_OK)
        {
            IAdviseSink_OnRename(statdata.pAdvSink, pmk);

            IAdviseSink_Release(statdata.pAdvSink);
        }
        IEnumSTATDATA_Release(pEnum);
    }

    return hr;
}

/******************************************************************************
 * OleAdviseHolderImpl_SendOnSave
 */
static HRESULT WINAPI OleAdviseHolderImpl_SendOnSave(IOleAdviseHolder *iface)
{
    IEnumSTATDATA *pEnum;
    HRESULT hr;

    TRACE("(%p)->()\n", iface);

    hr = IOleAdviseHolder_EnumAdvise(iface, &pEnum);
    if (SUCCEEDED(hr))
    {
        STATDATA statdata;
        while (IEnumSTATDATA_Next(pEnum, 1, &statdata, NULL) == S_OK)
        {
            IAdviseSink_OnSave(statdata.pAdvSink);

            IAdviseSink_Release(statdata.pAdvSink);
        }
        IEnumSTATDATA_Release(pEnum);
    }

    return hr;
}

/******************************************************************************
 * OleAdviseHolderImpl_SendOnClose
 */
static HRESULT WINAPI OleAdviseHolderImpl_SendOnClose(IOleAdviseHolder *iface)
{
    IEnumSTATDATA *pEnum;
    HRESULT hr;

    TRACE("(%p)->()\n", iface);

    hr = IOleAdviseHolder_EnumAdvise(iface, &pEnum);
    if (SUCCEEDED(hr))
    {
        STATDATA statdata;
        while (IEnumSTATDATA_Next(pEnum, 1, &statdata, NULL) == S_OK)
        {
            IAdviseSink_OnClose(statdata.pAdvSink);

            IAdviseSink_Release(statdata.pAdvSink);
        }
        IEnumSTATDATA_Release(pEnum);
    }

    return hr;
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

static IOleAdviseHolder *OleAdviseHolderImpl_Constructor(void)
{
  OleAdviseHolderImpl* lpoah;

  lpoah = HeapAlloc(GetProcessHeap(), 0, sizeof(OleAdviseHolderImpl));

  lpoah->IOleAdviseHolder_iface.lpVtbl = &oahvt;
  lpoah->ref = 1;
  lpoah->max_cons = INITIAL_SINKS;
  lpoah->connections = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                 lpoah->max_cons * sizeof(*lpoah->connections));

  TRACE("returning %p\n",  &lpoah->IOleAdviseHolder_iface);
  return &lpoah->IOleAdviseHolder_iface;
}

/**************************************************************************
 *  DataAdviseHolder Implementation
 */
typedef struct
{
  IDataAdviseHolder     IDataAdviseHolder_iface;

  LONG                  ref;
  DWORD                 maxCons;
  STATDATA*             connections;
  DWORD*                remote_connections;
  IDataObject*          delegate;
} DataAdviseHolder;

/* this connection has also has been advised to the delegate data object */
#define WINE_ADVF_REMOTE 0x80000000

static inline DataAdviseHolder *impl_from_IDataAdviseHolder(IDataAdviseHolder *iface)
{
    return CONTAINING_RECORD(iface, DataAdviseHolder, IDataAdviseHolder_iface);
}

/******************************************************************************
 * DataAdviseHolder_Destructor
 */
static void DataAdviseHolder_Destructor(DataAdviseHolder* ptrToDestroy)
{
  DWORD index;
  TRACE("%p\n", ptrToDestroy);

  for (index = 0; index < ptrToDestroy->maxCons; index++)
  {
    if (ptrToDestroy->connections[index].pAdvSink != NULL)
    {
      if (ptrToDestroy->delegate && 
          (ptrToDestroy->connections[index].advf & WINE_ADVF_REMOTE))
        IDataObject_DUnadvise(ptrToDestroy->delegate,
          ptrToDestroy->remote_connections[index]);

      release_statdata(ptrToDestroy->connections + index);
    }
  }

  HeapFree(GetProcessHeap(), 0, ptrToDestroy->remote_connections);
  HeapFree(GetProcessHeap(), 0, ptrToDestroy->connections);
  HeapFree(GetProcessHeap(), 0, ptrToDestroy);
}

/************************************************************************
 * DataAdviseHolder_QueryInterface (IUnknown)
 */
static HRESULT WINAPI DataAdviseHolder_QueryInterface(IDataAdviseHolder *iface,
                                                      REFIID riid, void **ppvObject)
{
  DataAdviseHolder *This = impl_from_IDataAdviseHolder(iface);
  TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppvObject);

  if ( (This==0) || (ppvObject==0) )
    return E_INVALIDARG;

  *ppvObject = 0;

  if ( IsEqualIID(&IID_IUnknown, riid) ||
       IsEqualIID(&IID_IDataAdviseHolder, riid)  )
  {
    *ppvObject = iface;
  }

  if ((*ppvObject)==0)
  {
    return E_NOINTERFACE;
  }

  IUnknown_AddRef((IUnknown*)*ppvObject);
  return S_OK;
}

/************************************************************************
 * DataAdviseHolder_AddRef (IUnknown)
 */
static ULONG WINAPI DataAdviseHolder_AddRef(IDataAdviseHolder *iface)
{
  DataAdviseHolder *This = impl_from_IDataAdviseHolder(iface);
  ULONG ref = InterlockedIncrement(&This->ref);
  TRACE("%p, refcount %lu.\n", iface, ref);
  return ref;
}

/************************************************************************
 * DataAdviseHolder_Release (IUnknown)
 */
static ULONG WINAPI DataAdviseHolder_Release(IDataAdviseHolder *iface)
{
  DataAdviseHolder *This = impl_from_IDataAdviseHolder(iface);
  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("%p, refcount %lu.\n", iface, ref);

  if (ref==0) DataAdviseHolder_Destructor(This);

  return ref;
}

/************************************************************************
 * DataAdviseHolder_Advise
 *
 */
static HRESULT WINAPI DataAdviseHolder_Advise(IDataAdviseHolder *iface,
                                              IDataObject *pDataObject, FORMATETC *pFetc,
                                              DWORD advf, IAdviseSink *pAdvise,
                                              DWORD *pdwConnection)
{
  DWORD index;
  STATDATA new_conn;
  DataAdviseHolder *This = impl_from_IDataAdviseHolder(iface);

  TRACE("%p, %p, %p, %#lx, %p, %p.\n", iface, pDataObject, pFetc, advf, pAdvise, pdwConnection);

  if (pdwConnection==NULL)
    return E_POINTER;

  *pdwConnection = 0;

  for (index = 0; index < This->maxCons; index++)
  {
    if (This->connections[index].pAdvSink == NULL)
      break;
  }

  if (index == This->maxCons)
  {
    This->maxCons+=INITIAL_SINKS;
    This->connections = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                    This->connections,
                                    This->maxCons * sizeof(*This->connections));
    This->remote_connections = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                           This->remote_connections,
                                           This->maxCons * sizeof(*This->remote_connections));
  }

  new_conn.pAdvSink = pAdvise;
  new_conn.advf = advf & ~WINE_ADVF_REMOTE;
  new_conn.formatetc = *pFetc;
  new_conn.dwConnection = index + 1; /* 0 is not a valid cookie, so increment the index */

  copy_statdata(This->connections + index, &new_conn);

  if (This->connections[index].pAdvSink != NULL)
  {
    /* if we are already connected advise the remote object */
    if (This->delegate)
    {
        HRESULT hr;

        hr = IDataObject_DAdvise(This->delegate, &new_conn.formatetc,
                                 new_conn.advf, new_conn.pAdvSink,
                                 &This->remote_connections[index]);
        if (FAILED(hr))
        {
            IDataAdviseHolder_Unadvise(iface, new_conn.dwConnection);
            return hr;
        }
        This->connections[index].advf |= WINE_ADVF_REMOTE;
    }
    else if(advf & ADVF_PRIMEFIRST)
      /* only do this if we have no delegate, since in the above case the
       * delegate will do the priming for us */
      IDataAdviseHolder_SendOnDataChange(iface, pDataObject, 0, advf);
  }

  *pdwConnection = new_conn.dwConnection;

  return S_OK;
}

/******************************************************************************
 * DataAdviseHolder_Unadvise
 */
static HRESULT WINAPI DataAdviseHolder_Unadvise(IDataAdviseHolder *iface,
                                                DWORD dwConnection)
{
  DataAdviseHolder *This = impl_from_IDataAdviseHolder(iface);
  DWORD index;

  TRACE("%p, %lu.\n", iface, dwConnection);

  /* The connection number is 1 more than the index, see DataAdviseHolder_Advise */
  index = dwConnection - 1;

  if (index >= This->maxCons || This->connections[index].pAdvSink == NULL)
     return OLE_E_NOCONNECTION;

  if (This->delegate && This->connections[index].advf & WINE_ADVF_REMOTE)
  {
    IDataObject_DUnadvise(This->delegate, This->remote_connections[index]);
    This->remote_connections[index] = 0;
  }

  release_statdata(This->connections + index);

  return S_OK;
}

/******************************************************************************
 * DataAdviseHolder_EnumAdvise
 */
static HRESULT WINAPI DataAdviseHolder_EnumAdvise(IDataAdviseHolder *iface,
                                                  IEnumSTATDATA **enum_advise)
{
    DataAdviseHolder *This = impl_from_IDataAdviseHolder(iface);
    IUnknown *unk;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, enum_advise);

    IDataAdviseHolder_QueryInterface(iface, &IID_IUnknown, (void**)&unk);
    hr = EnumSTATDATA_Construct(unk, 0, This->maxCons, This->connections, TRUE, enum_advise);
    IUnknown_Release(unk);
    return hr;
}

/******************************************************************************
 * DataAdviseHolder_SendOnDataChange
 */
static HRESULT WINAPI DataAdviseHolder_SendOnDataChange(IDataAdviseHolder *iface,
                                                        IDataObject *data_obj,
                                                        DWORD dwReserved, DWORD advf)
{
    IEnumSTATDATA *pEnum;
    HRESULT hr;

    TRACE("%p, %p, %#lx, %#lx.\n", iface, data_obj, dwReserved, advf);

    hr = IDataAdviseHolder_EnumAdvise(iface, &pEnum);
    if (SUCCEEDED(hr))
    {
        STATDATA statdata;
        while (IEnumSTATDATA_Next(pEnum, 1, &statdata, NULL) == S_OK)
        {
            STGMEDIUM stg;
            stg.tymed = TYMED_NULL;
            stg.pstg = NULL;
            stg.pUnkForRelease = NULL;

            if(!(statdata.advf & ADVF_NODATA))
            {
                hr = IDataObject_GetData(data_obj, &statdata.formatetc, &stg);
            }

            IAdviseSink_OnDataChange(statdata.pAdvSink, &statdata.formatetc, &stg);

            if(statdata.advf & ADVF_ONLYONCE)
            {
                IDataAdviseHolder_Unadvise(iface, statdata.dwConnection);
            }

            release_statdata(&statdata);
        }
        IEnumSTATDATA_Release(pEnum);
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

HRESULT DataAdviseHolder_OnConnect(IDataAdviseHolder *iface, IDataObject *pDelegate)
{
  DataAdviseHolder *This = impl_from_IDataAdviseHolder(iface);
  DWORD index;
  HRESULT hr = S_OK;

  for(index = 0; index < This->maxCons; index++)
  {
    if(This->connections[index].pAdvSink != NULL)
    {
      hr = IDataObject_DAdvise(pDelegate, &This->connections[index].formatetc,
                               This->connections[index].advf,
                               This->connections[index].pAdvSink,
                               &This->remote_connections[index]);
      if (FAILED(hr)) break;
      This->connections[index].advf |= WINE_ADVF_REMOTE;
    }
  }
  This->delegate = pDelegate;
  return hr;
}

void DataAdviseHolder_OnDisconnect(IDataAdviseHolder *iface)
{
  DataAdviseHolder *This = impl_from_IDataAdviseHolder(iface);
  DWORD index;

  for(index = 0; index < This->maxCons; index++)
  {
    if((This->connections[index].pAdvSink != NULL) &&
       (This->connections[index].advf & WINE_ADVF_REMOTE))
    {
      IDataObject_DUnadvise(This->delegate, This->remote_connections[index]);
      This->remote_connections[index] = 0;
      This->connections[index].advf &= ~WINE_ADVF_REMOTE;
    }
  }
  This->delegate = NULL;
}

/******************************************************************************
 * DataAdviseHolder_Constructor
 */
static IDataAdviseHolder *DataAdviseHolder_Constructor(void)
{
  DataAdviseHolder* newHolder;

  newHolder = HeapAlloc(GetProcessHeap(), 0, sizeof(DataAdviseHolder));

  newHolder->IDataAdviseHolder_iface.lpVtbl = &DataAdviseHolderImpl_VTable;
  newHolder->ref = 1;
  newHolder->maxCons = INITIAL_SINKS;
  newHolder->connections = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                     newHolder->maxCons * sizeof(*newHolder->connections));
  newHolder->remote_connections = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                            newHolder->maxCons * sizeof(*newHolder->remote_connections));
  newHolder->delegate = NULL;

  TRACE("returning %p\n", &newHolder->IDataAdviseHolder_iface);
  return &newHolder->IDataAdviseHolder_iface;
}

/***********************************************************************
 * API functions
 */

/***********************************************************************
 * CreateOleAdviseHolder [OLE32.@]
 */
HRESULT WINAPI CreateOleAdviseHolder(IOleAdviseHolder **ppOAHolder)
{
  TRACE("(%p)\n", ppOAHolder);

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
HRESULT WINAPI CreateDataAdviseHolder(IDataAdviseHolder **ppDAHolder)
{
  TRACE("(%p)\n", ppDAHolder);

  if (ppDAHolder==NULL)
    return E_POINTER;

  *ppDAHolder = DataAdviseHolder_Constructor();

  if (*ppDAHolder != NULL)
    return S_OK;

  return E_OUTOFMEMORY;
}
