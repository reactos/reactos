/*
 *	OLE 2 default object handler
 *
 *      Copyright 1999  Francis Beaudet
 *      Copyright 2000  Abey George
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
 *
 * NOTES:
 *    The OLE2 default object handler supports a whole whack of
 *    interfaces including:
 *       IOleObject, IDataObject, IPersistStorage, IViewObject2,
 *       IRunnableObject, IOleCache2, IOleCacheControl and much more.
 *
 *    All the implementation details are taken from: Inside OLE
 *    second edition by Kraig Brockschmidt,
 *
 * TODO
 * - This implementation of the default handler does not launch the
 *   server in the DoVerb, Update, GetData, GetDataHere and Run
 *   methods. When it is fixed to do so, all the methods will have
 *   to be  revisited to allow delegating to the running object
 *
 * - All methods in the class that use the class ID should be
 *   aware that it is possible for a class to be treated as
 *   another one and go into emulation mode. Nothing has been
 *   done in this area.
 *
 * - Some functions still return E_NOTIMPL they have to be
 *   implemented. Most of those are related to the running of the
 *   actual server.
 *
 * - All the methods related to notification and advise sinks are
 *   in place but no notifications are sent to the sinks yet.
 */
#include <assert.h>
#include <stdarg.h>
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "ole2.h"

#include "compobj_private.h"
#include "storage32.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

enum storage_state
{
    storage_state_uninitialised,
    storage_state_initialised,
    storage_state_loaded
};

enum object_state
{
    object_state_not_running,
    object_state_running,
    object_state_deferred_close
};

/****************************************************************************
 * DefaultHandler
 *
 */
struct DefaultHandler
{
  IOleObject        IOleObject_iface;
  IUnknown          IUnknown_iface;
  IDataObject       IDataObject_iface;
  IRunnableObject   IRunnableObject_iface;
  IAdviseSink       IAdviseSink_iface;
  IPersistStorage   IPersistStorage_iface;

  /* Reference count of this object */
  LONG ref;

  /* IUnknown implementation of the outer object. */
  IUnknown* outerUnknown;

  /* Class Id that this handler object represents. */
  CLSID clsid;

  /* IUnknown implementation of the datacache. */
  IUnknown* dataCache;
  /* IPersistStorage implementation of the datacache. */
  IPersistStorage* dataCache_PersistStg;

  /* Client site for the embedded object. */
  IOleClientSite* clientSite;

  /*
   * The IOleAdviseHolder maintains the connections
   * on behalf of the default handler.
   */
  IOleAdviseHolder* oleAdviseHolder;

  /*
   * The IDataAdviseHolder maintains the data
   * connections on behalf of the default handler.
   */
  IDataAdviseHolder* dataAdviseHolder;

  /* Name of the container and object contained */
  LPWSTR containerApp;
  LPWSTR containerObj;

  /* IOleObject delegate */
  IOleObject *pOleDelegate;
  /* IPersistStorage delegate */
  IPersistStorage *pPSDelegate;
  /* IDataObject delegate */
  IDataObject *pDataDelegate;
  enum object_state object_state;
  ULONG in_call;

  /* connection cookie for the advise on the delegate OLE object */
  DWORD dwAdvConn;

  /* storage passed to Load or InitNew */
  IStorage *storage;
  enum storage_state storage_state;

  /* optional class factory for object */
  IClassFactory *pCFObject;
  /* TRUE if acting as an inproc server instead of an inproc handler */
  BOOL inproc_server;
};

typedef struct DefaultHandler DefaultHandler;

static inline DefaultHandler *impl_from_IOleObject( IOleObject *iface )
{
    return CONTAINING_RECORD(iface, DefaultHandler, IOleObject_iface);
}

static inline DefaultHandler *impl_from_IUnknown( IUnknown *iface )
{
    return CONTAINING_RECORD(iface, DefaultHandler, IUnknown_iface);
}

static inline DefaultHandler *impl_from_IDataObject( IDataObject *iface )
{
    return CONTAINING_RECORD(iface, DefaultHandler, IDataObject_iface);
}

static inline DefaultHandler *impl_from_IRunnableObject( IRunnableObject *iface )
{
    return CONTAINING_RECORD(iface, DefaultHandler, IRunnableObject_iface);
}

static inline DefaultHandler *impl_from_IAdviseSink( IAdviseSink *iface )
{
    return CONTAINING_RECORD(iface, DefaultHandler, IAdviseSink_iface);
}

static inline DefaultHandler *impl_from_IPersistStorage( IPersistStorage *iface )
{
    return CONTAINING_RECORD(iface, DefaultHandler, IPersistStorage_iface);
}

static void DefaultHandler_Destroy(DefaultHandler* This);

static inline BOOL object_is_running(DefaultHandler *This)
{
    return IRunnableObject_IsRunning(&This->IRunnableObject_iface);
}

static void DefaultHandler_Stop(DefaultHandler *This);

static inline void start_object_call(DefaultHandler *This)
{
    This->in_call++;
}

static inline void end_object_call(DefaultHandler *This)
{
    This->in_call--;
    if (This->in_call == 0 && This->object_state == object_state_deferred_close)
        DefaultHandler_Stop( This );
}

/*********************************************************
 * Method implementation for the  non delegating IUnknown
 * part of the DefaultHandler class.
 */

/************************************************************************
 * DefaultHandler_NDIUnknown_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 *
 * This version of QueryInterface will not delegate its implementation
 * to the outer unknown.
 */
static HRESULT WINAPI DefaultHandler_NDIUnknown_QueryInterface(
            IUnknown*      iface,
            REFIID         riid,
            void**         ppvObject)
{
  DefaultHandler *This = impl_from_IUnknown(iface);

  if (!ppvObject)
    return E_INVALIDARG;

  *ppvObject = NULL;

  if (IsEqualIID(&IID_IUnknown, riid))
    *ppvObject = iface;
  else if (IsEqualIID(&IID_IOleObject, riid))
    *ppvObject = &This->IOleObject_iface;
  else if (IsEqualIID(&IID_IDataObject, riid))
    *ppvObject = &This->IDataObject_iface;
  else if (IsEqualIID(&IID_IRunnableObject, riid))
    *ppvObject = &This->IRunnableObject_iface;
  else if (IsEqualIID(&IID_IPersist, riid) ||
           IsEqualIID(&IID_IPersistStorage, riid))
    *ppvObject = &This->IPersistStorage_iface;
  else if (IsEqualIID(&IID_IViewObject, riid) ||
           IsEqualIID(&IID_IViewObject2, riid) ||
           IsEqualIID(&IID_IOleCache, riid) ||
           IsEqualIID(&IID_IOleCache2, riid))
  {
    HRESULT hr = IUnknown_QueryInterface(This->dataCache, riid, ppvObject);
    if (FAILED(hr)) FIXME("interface %s not implemented by data cache\n", debugstr_guid(riid));
    return hr;
  }
  else if (This->inproc_server && This->pOleDelegate)
  {
    return IOleObject_QueryInterface(This->pOleDelegate, riid, ppvObject);
  }

  /* Check that we obtained an interface. */
  if (*ppvObject == NULL)
  {
    WARN( "() : asking for unsupported interface %s\n", debugstr_guid(riid));
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
 * DefaultHandler_NDIUnknown_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 *
 * This version of QueryInterface will not delegate its implementation
 * to the outer unknown.
 */
static ULONG WINAPI DefaultHandler_NDIUnknown_AddRef(
            IUnknown*      iface)
{
  DefaultHandler *This = impl_from_IUnknown(iface);
  return InterlockedIncrement(&This->ref);
}

/************************************************************************
 * DefaultHandler_NDIUnknown_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 *
 * This version of QueryInterface will not delegate its implementation
 * to the outer unknown.
 */
static ULONG WINAPI DefaultHandler_NDIUnknown_Release(
            IUnknown*      iface)
{
  DefaultHandler *This = impl_from_IUnknown(iface);
  ULONG ref;

  ref = InterlockedDecrement(&This->ref);

  if (!ref) DefaultHandler_Destroy(This);

  return ref;
}

/*********************************************************
 * Methods implementation for the IOleObject part of
 * the DefaultHandler class.
 */

/************************************************************************
 * DefaultHandler_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI DefaultHandler_QueryInterface(
            IOleObject*      iface,
            REFIID           riid,
            void**           ppvObject)
{
  DefaultHandler *This = impl_from_IOleObject(iface);

  return IUnknown_QueryInterface(This->outerUnknown, riid, ppvObject);
}

/************************************************************************
 * DefaultHandler_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DefaultHandler_AddRef(
            IOleObject*        iface)
{
  DefaultHandler *This = impl_from_IOleObject(iface);

  return IUnknown_AddRef(This->outerUnknown);
}

/************************************************************************
 * DefaultHandler_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DefaultHandler_Release(
            IOleObject*        iface)
{
  DefaultHandler *This = impl_from_IOleObject(iface);

  return IUnknown_Release(This->outerUnknown);
}

/************************************************************************
 * DefaultHandler_SetClientSite (IOleObject)
 *
 * The default handler's implementation of this method only keeps the
 * client site pointer for future reference.
 *
 * See Windows documentation for more details on IOleObject methods.
 */
static HRESULT WINAPI DefaultHandler_SetClientSite(
	    IOleObject*        iface,
	    IOleClientSite*    pClientSite)
{
  DefaultHandler *This = impl_from_IOleObject(iface);
  HRESULT hr = S_OK;

  TRACE("(%p, %p)\n", iface, pClientSite);

  if (object_is_running(This))
  {
    start_object_call( This );
    hr = IOleObject_SetClientSite(This->pOleDelegate, pClientSite);
    end_object_call( This );
  }

  /*
   * Make sure we release the previous client site if there
   * was one.
   */
  if (This->clientSite)
    IOleClientSite_Release(This->clientSite);

  This->clientSite = pClientSite;

  if (This->clientSite)
    IOleClientSite_AddRef(This->clientSite);

  return hr;
}

/************************************************************************
 * DefaultHandler_GetClientSite (IOleObject)
 *
 * The default handler's implementation of this method returns the
 * last pointer set in IOleObject_SetClientSite.
 *
 * See Windows documentation for more details on IOleObject methods.
 */
static HRESULT WINAPI DefaultHandler_GetClientSite(
	    IOleObject*        iface,
	    IOleClientSite**   ppClientSite)
{
  DefaultHandler *This = impl_from_IOleObject(iface);

  if (!ppClientSite)
    return E_POINTER;

  *ppClientSite = This->clientSite;

  if (This->clientSite)
    IOleClientSite_AddRef(This->clientSite);

  return S_OK;
}

/************************************************************************
 * DefaultHandler_SetHostNames (IOleObject)
 *
 * The default handler's implementation of this method just stores
 * the strings and returns S_OK.
 *
 * See Windows documentation for more details on IOleObject methods.
 */
static HRESULT WINAPI DefaultHandler_SetHostNames(
	    IOleObject*        iface,
	    LPCOLESTR          szContainerApp,
	    LPCOLESTR          szContainerObj)
{
  DefaultHandler *This = impl_from_IOleObject(iface);

  TRACE("(%p, %s, %s)\n",
	iface,
	debugstr_w(szContainerApp),
	debugstr_w(szContainerObj));

  if (object_is_running(This))
  {
    start_object_call( This );
    IOleObject_SetHostNames(This->pOleDelegate, szContainerApp, szContainerObj);
    end_object_call( This );
  }

  /* Be sure to cleanup before re-assigning the strings. */
  HeapFree( GetProcessHeap(), 0, This->containerApp );
  This->containerApp = NULL;
  HeapFree( GetProcessHeap(), 0, This->containerObj );
  This->containerObj = NULL;

  if (szContainerApp)
  {
      if ((This->containerApp = HeapAlloc( GetProcessHeap(), 0,
                                           (lstrlenW(szContainerApp) + 1) * sizeof(WCHAR) )))
          lstrcpyW( This->containerApp, szContainerApp );
  }

  if (szContainerObj)
  {
      if ((This->containerObj = HeapAlloc( GetProcessHeap(), 0,
                                           (lstrlenW(szContainerObj) + 1) * sizeof(WCHAR) )))
          lstrcpyW( This->containerObj, szContainerObj );
  }
  return S_OK;
}

static void release_delegates(DefaultHandler *This)
{
    if (This->pDataDelegate)
    {
        IDataObject_Release(This->pDataDelegate);
        This->pDataDelegate = NULL;
    }
    if (This->pPSDelegate)
    {
        IPersistStorage_Release(This->pPSDelegate);
        This->pPSDelegate = NULL;
    }
    if (This->pOleDelegate)
    {
        IOleObject_Release(This->pOleDelegate);
        This->pOleDelegate = NULL;
    }
}

/* undoes the work done by DefaultHandler_Run */
static void DefaultHandler_Stop(DefaultHandler *This)
{
  IOleCacheControl *cache_ctrl;
  HRESULT hr;

  if (This->object_state == object_state_not_running)
    return;

  hr = IUnknown_QueryInterface( This->dataCache, &IID_IOleCacheControl, (void **)&cache_ctrl );
  if (SUCCEEDED(hr))
  {
    hr = IOleCacheControl_OnStop( cache_ctrl );
    IOleCacheControl_Release( cache_ctrl );
  }

  IOleObject_Unadvise(This->pOleDelegate, This->dwAdvConn);

  if (This->dataAdviseHolder)
    DataAdviseHolder_OnDisconnect(This->dataAdviseHolder);

  This->object_state = object_state_not_running;
  release_delegates( This );
}

/************************************************************************
 * DefaultHandler_Close (IOleObject)
 *
 * The default handler's implementation of this method is meaningless
 * without a running server so it does nothing.
 *
 * See Windows documentation for more details on IOleObject methods.
 */
static HRESULT WINAPI DefaultHandler_Close(
	    IOleObject*        iface,
	    DWORD              dwSaveOption)
{
  DefaultHandler *This = impl_from_IOleObject(iface);
  HRESULT hr;

  TRACE("%ld\n", dwSaveOption);

  if (!object_is_running(This))
    return S_OK;

  start_object_call( This );
  hr = IOleObject_Close(This->pOleDelegate, dwSaveOption);
  end_object_call( This );

  DefaultHandler_Stop(This);

  return hr;
}

/************************************************************************
 * DefaultHandler_SetMoniker (IOleObject)
 *
 * The default handler's implementation of this method does nothing.
 *
 * See Windows documentation for more details on IOleObject methods.
 */
static HRESULT WINAPI DefaultHandler_SetMoniker(
	    IOleObject*        iface,
	    DWORD              dwWhichMoniker,
	    IMoniker*          pmk)
{
  DefaultHandler *This = impl_from_IOleObject(iface);
  HRESULT hr = S_OK;

  TRACE("%p, %ld, %p.\n", iface, dwWhichMoniker, pmk);

  if (object_is_running(This))
  {
    start_object_call( This );
    hr = IOleObject_SetMoniker(This->pOleDelegate, dwWhichMoniker, pmk);
    end_object_call( This );
  }

  return hr;
}

/************************************************************************
 * DefaultHandler_GetMoniker (IOleObject)
 *
 * Delegate this request to the client site if we have one.
 *
 * See Windows documentation for more details on IOleObject methods.
 */
static HRESULT WINAPI DefaultHandler_GetMoniker(
	    IOleObject*        iface,
	    DWORD              dwAssign,
	    DWORD              dwWhichMoniker,
	    IMoniker**         ppmk)
{
  DefaultHandler *This = impl_from_IOleObject(iface);
  HRESULT hr;

  TRACE("%p, %ld, %ld, %p.\n", iface, dwAssign, dwWhichMoniker, ppmk);

  if (object_is_running(This))
  {
    start_object_call( This );
    hr = IOleObject_GetMoniker(This->pOleDelegate, dwAssign, dwWhichMoniker,
                               ppmk);
    end_object_call( This );
    return hr;
  }

  /* FIXME: dwWhichMoniker == OLEWHICHMK_CONTAINER only? */
  if (This->clientSite)
  {
    return IOleClientSite_GetMoniker(This->clientSite,
				     dwAssign,
				     dwWhichMoniker,
				     ppmk);

  }

  return E_FAIL;
}

/************************************************************************
 * DefaultHandler_InitFromData (IOleObject)
 *
 * This method is meaningless if the server is not running
 *
 * See Windows documentation for more details on IOleObject methods.
 */
static HRESULT WINAPI DefaultHandler_InitFromData(
	    IOleObject*        iface,
	    IDataObject*       pDataObject,
	    BOOL               fCreation,
	    DWORD              dwReserved)
{
  DefaultHandler *This = impl_from_IOleObject(iface);
  HRESULT hr = OLE_E_NOTRUNNING;

  TRACE("%p, %p, %d, %ld.\n", iface, pDataObject, fCreation, dwReserved);

  if (object_is_running(This))
  {
    start_object_call( This );
    hr = IOleObject_InitFromData(This->pOleDelegate, pDataObject, fCreation,
		                   dwReserved);
    end_object_call( This );
  }

  return hr;
}

/************************************************************************
 * DefaultHandler_GetClipboardData (IOleObject)
 *
 * This method is meaningless if the server is not running
 *
 * See Windows documentation for more details on IOleObject methods.
 */
static HRESULT WINAPI DefaultHandler_GetClipboardData(
	    IOleObject*        iface,
	    DWORD              dwReserved,
	    IDataObject**      ppDataObject)
{
  DefaultHandler *This = impl_from_IOleObject(iface);
  HRESULT hr = OLE_E_NOTRUNNING;

  TRACE("%p, %ld, %p.\n", iface, dwReserved, ppDataObject);

  if (object_is_running(This))
  {
    start_object_call( This );
    hr = IOleObject_GetClipboardData(This->pOleDelegate, dwReserved,
                                       ppDataObject);
    end_object_call( This );
  }

  return hr;
}

static HRESULT WINAPI DefaultHandler_DoVerb(
	    IOleObject*        iface,
	    LONG               iVerb,
	    struct tagMSG*     lpmsg,
	    IOleClientSite*    pActiveSite,
	    LONG               lindex,
	    HWND               hwndParent,
	    LPCRECT            lprcPosRect)
{
  DefaultHandler *This = impl_from_IOleObject(iface);
  IRunnableObject *pRunnableObj = &This->IRunnableObject_iface;
  HRESULT hr;

  TRACE("%ld, %p, %p, %ld, %p, %s.\n", iVerb, lpmsg, pActiveSite, lindex, hwndParent, wine_dbgstr_rect(lprcPosRect));

  hr = IRunnableObject_Run(pRunnableObj, NULL);
  if (FAILED(hr)) return hr;

  start_object_call( This );
  hr = IOleObject_DoVerb(This->pOleDelegate, iVerb, lpmsg, pActiveSite,
                           lindex, hwndParent, lprcPosRect);
  end_object_call( This );

  return hr;
}

/************************************************************************
 * DefaultHandler_EnumVerbs (IOleObject)
 *
 * The default handler implementation of this method simply delegates
 * to OleRegEnumVerbs
 *
 * See Windows documentation for more details on IOleObject methods.
 */
static HRESULT WINAPI DefaultHandler_EnumVerbs(
	    IOleObject*        iface,
	    IEnumOLEVERB**     ppEnumOleVerb)
{
  DefaultHandler *This = impl_from_IOleObject(iface);
  HRESULT hr = OLE_S_USEREG;

  TRACE("(%p, %p)\n", iface, ppEnumOleVerb);

  if (object_is_running(This))
  {
    start_object_call( This );
    hr = IOleObject_EnumVerbs(This->pOleDelegate, ppEnumOleVerb);
    end_object_call( This );
  }

  if (hr == OLE_S_USEREG)
    return OleRegEnumVerbs(&This->clsid, ppEnumOleVerb);
  else
    return hr;
}

static HRESULT WINAPI DefaultHandler_Update(
	    IOleObject*        iface)
{
    DefaultHandler *This = impl_from_IOleObject(iface);
    HRESULT hr;

    TRACE("(%p)\n", iface);

    if (!object_is_running(This))
    {
        FIXME("Should run object\n");
        return E_NOTIMPL;
    }

    start_object_call( This );
    hr = IOleObject_Update(This->pOleDelegate);
    end_object_call( This );

    return hr;
}

/************************************************************************
 * DefaultHandler_IsUpToDate (IOleObject)
 *
 * This method is meaningless if the server is not running
 *
 * See Windows documentation for more details on IOleObject methods.
 */
static HRESULT WINAPI DefaultHandler_IsUpToDate(
	    IOleObject*        iface)
{
    DefaultHandler *This = impl_from_IOleObject(iface);
    HRESULT hr = OLE_E_NOTRUNNING;
    TRACE("(%p)\n", iface);

    if (object_is_running(This))
    {
        start_object_call( This );
        hr = IOleObject_IsUpToDate(This->pOleDelegate);
        end_object_call( This );
    }

    return hr;
}

/************************************************************************
 * DefaultHandler_GetUserClassID (IOleObject)
 *
 * TODO: Map to a new class ID if emulation is active.
 *
 * See Windows documentation for more details on IOleObject methods.
 */
static HRESULT WINAPI DefaultHandler_GetUserClassID(
	    IOleObject*        iface,
	    CLSID*             pClsid)
{
  DefaultHandler *This = impl_from_IOleObject(iface);
  HRESULT hr;

  TRACE("(%p, %p)\n", iface, pClsid);

  if (object_is_running(This))
  {
    start_object_call( This );
    hr = IOleObject_GetUserClassID(This->pOleDelegate, pClsid);
    end_object_call( This );
    return hr;
  }

  if (!pClsid)
    return E_POINTER;

  *pClsid = This->clsid;

  return S_OK;
}

/************************************************************************
 * DefaultHandler_GetUserType (IOleObject)
 *
 * The default handler implementation of this method simply delegates
 * to OleRegGetUserType
 *
 * See Windows documentation for more details on IOleObject methods.
 */
static HRESULT WINAPI DefaultHandler_GetUserType(
	    IOleObject*        iface,
	    DWORD              dwFormOfType,
	    LPOLESTR*          pszUserType)
{
  DefaultHandler *This = impl_from_IOleObject(iface);
  HRESULT hr;

  TRACE("%p, %ld, %p.\n", iface, dwFormOfType, pszUserType);
  if (object_is_running(This))
  {
    start_object_call( This );
    hr = IOleObject_GetUserType(This->pOleDelegate, dwFormOfType, pszUserType);
    end_object_call( This );
    return hr;
  }

  return OleRegGetUserType(&This->clsid, dwFormOfType, pszUserType);
}

/************************************************************************
 * DefaultHandler_SetExtent (IOleObject)
 *
 * This method is meaningless if the server is not running
 *
 * See Windows documentation for more details on IOleObject methods.
 */
static HRESULT WINAPI DefaultHandler_SetExtent(
	    IOleObject*        iface,
	    DWORD              dwDrawAspect,
	    SIZEL*             psizel)
{
  DefaultHandler *This = impl_from_IOleObject(iface);
  HRESULT hr = OLE_E_NOTRUNNING;

  TRACE("%p, %lx, (%ld x %ld))\n", iface, dwDrawAspect, psizel->cx, psizel->cy);

  if (object_is_running(This))
  {
    start_object_call( This );
    hr = IOleObject_SetExtent(This->pOleDelegate, dwDrawAspect, psizel);
    end_object_call( This );
  }

  return hr;
}

/************************************************************************
 * DefaultHandler_GetExtent (IOleObject)
 *
 * The default handler's implementation of this method returns uses
 * the cache to locate the aspect and extract the extent from it.
 *
 * See Windows documentation for more details on IOleObject methods.
 */
static HRESULT WINAPI DefaultHandler_GetExtent(
	    IOleObject*        iface,
	    DWORD              dwDrawAspect,
	    SIZEL*             psizel)
{
  DVTARGETDEVICE* targetDevice;
  IViewObject2*   cacheView = NULL;
  HRESULT         hres;

  DefaultHandler *This = impl_from_IOleObject(iface);

  TRACE("%p, %lx, %p.\n", iface, dwDrawAspect, psizel);

  if (object_is_running(This))
  {
    start_object_call( This );
    hres = IOleObject_GetExtent(This->pOleDelegate, dwDrawAspect, psizel);
    end_object_call( This );
    return hres;
  }

  hres = IUnknown_QueryInterface(This->dataCache, &IID_IViewObject2, (void**)&cacheView);
  if (FAILED(hres))
    return E_UNEXPECTED;

  /*
   * Prepare the call to the cache's GetExtent method.
   *
   * Here we would build a valid DVTARGETDEVICE structure
   * but, since we are calling into the data cache, we
   * know its implementation and we'll skip this
   * extra work until later.
   */
  targetDevice = NULL;

  hres = IViewObject2_GetExtent(cacheView,
				dwDrawAspect,
				-1,
				targetDevice,
				psizel);

  IViewObject2_Release(cacheView);

  return hres;
}

/************************************************************************
 * DefaultHandler_Advise (IOleObject)
 *
 * The default handler's implementation of this method simply
 * delegates to the OleAdviseHolder.
 *
 * See Windows documentation for more details on IOleObject methods.
 */
static HRESULT WINAPI DefaultHandler_Advise(
	    IOleObject*        iface,
	    IAdviseSink*       pAdvSink,
	    DWORD*             pdwConnection)
{
  HRESULT hres = S_OK;
  DefaultHandler *This = impl_from_IOleObject(iface);

  TRACE("(%p, %p, %p)\n", iface, pAdvSink, pdwConnection);

  /* Make sure we have an advise holder before we start. */
  if (!This->oleAdviseHolder)
    hres = CreateOleAdviseHolder(&This->oleAdviseHolder);

  if (SUCCEEDED(hres))
    hres = IOleAdviseHolder_Advise(This->oleAdviseHolder,
				   pAdvSink,
				   pdwConnection);

  return hres;
}

/************************************************************************
 * DefaultHandler_Unadvise (IOleObject)
 *
 * The default handler's implementation of this method simply
 * delegates to the OleAdviseHolder.
 *
 * See Windows documentation for more details on IOleObject methods.
 */
static HRESULT WINAPI DefaultHandler_Unadvise(
	    IOleObject*        iface,
	    DWORD              dwConnection)
{
  DefaultHandler *This = impl_from_IOleObject(iface);

  TRACE("%p, %ld.\n", iface, dwConnection);

  /*
   * If we don't have an advise holder yet, it means we don't have
   * a connection.
   */
  if (!This->oleAdviseHolder)
    return OLE_E_NOCONNECTION;

  return IOleAdviseHolder_Unadvise(This->oleAdviseHolder,
				   dwConnection);
}

/************************************************************************
 * DefaultHandler_EnumAdvise (IOleObject)
 *
 * The default handler's implementation of this method simply
 * delegates to the OleAdviseHolder.
 *
 * See Windows documentation for more details on IOleObject methods.
 */
static HRESULT WINAPI DefaultHandler_EnumAdvise(
	    IOleObject*        iface,
	    IEnumSTATDATA**    ppenumAdvise)
{
  DefaultHandler *This = impl_from_IOleObject(iface);

  TRACE("(%p, %p)\n", iface, ppenumAdvise);

  if (!ppenumAdvise)
    return E_POINTER;

  *ppenumAdvise = NULL;

  if (!This->oleAdviseHolder)
      return S_OK;

  return IOleAdviseHolder_EnumAdvise(This->oleAdviseHolder, ppenumAdvise);
}

/************************************************************************
 * DefaultHandler_GetMiscStatus (IOleObject)
 *
 * The default handler's implementation of this method simply delegates
 * to OleRegGetMiscStatus.
 *
 * See Windows documentation for more details on IOleObject methods.
 */
static HRESULT WINAPI DefaultHandler_GetMiscStatus(
	    IOleObject*        iface,
	    DWORD              dwAspect,
	    DWORD*             pdwStatus)
{
  HRESULT hres;
  DefaultHandler *This = impl_from_IOleObject(iface);

  TRACE("%p, %lx, %p.\n", iface, dwAspect, pdwStatus);

  if (object_is_running(This))
  {
    start_object_call( This );
    hres = IOleObject_GetMiscStatus(This->pOleDelegate, dwAspect, pdwStatus);
    end_object_call( This );
    return hres;
  }

  hres = OleRegGetMiscStatus(&This->clsid, dwAspect, pdwStatus);

  if (FAILED(hres))
    *pdwStatus = 0;

  return hres;
}

/************************************************************************
 * DefaultHandler_SetColorScheme (IOleObject)
 *
 * This method is meaningless if the server is not running
 *
 * See Windows documentation for more details on IOleObject methods.
 */
static HRESULT WINAPI DefaultHandler_SetColorScheme(
	    IOleObject*           iface,
	    struct tagLOGPALETTE* pLogpal)
{
  DefaultHandler *This = impl_from_IOleObject(iface);
  HRESULT hr = OLE_E_NOTRUNNING;

  TRACE("(%p, %p))\n", iface, pLogpal);

  if (object_is_running(This))
  {
    start_object_call( This );
    hr = IOleObject_SetColorScheme(This->pOleDelegate, pLogpal);
    end_object_call( This );
  }

  return hr;
}

/*********************************************************
 * Methods implementation for the IDataObject part of
 * the DefaultHandler class.
 */

/************************************************************************
 * DefaultHandler_IDataObject_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI DefaultHandler_IDataObject_QueryInterface(
            IDataObject*     iface,
           REFIID           riid,
            void**           ppvObject)
{
  DefaultHandler *This = impl_from_IDataObject(iface);

  return IUnknown_QueryInterface(This->outerUnknown, riid, ppvObject);
}

/************************************************************************
 * DefaultHandler_IDataObject_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DefaultHandler_IDataObject_AddRef(
            IDataObject*     iface)
{
  DefaultHandler *This = impl_from_IDataObject(iface);

  return IUnknown_AddRef(This->outerUnknown);
}

/************************************************************************
 * DefaultHandler_IDataObject_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DefaultHandler_IDataObject_Release(
            IDataObject*     iface)
{
  DefaultHandler *This = impl_from_IDataObject(iface);

  return IUnknown_Release(This->outerUnknown);
}

/************************************************************************
 * DefaultHandler_GetData
 *
 * Get Data from a source dataobject using format pformatetcIn->cfFormat
 * See Windows documentation for more details on GetData.
 * Default handler's implementation of this method delegates to the cache.
 */
static HRESULT WINAPI DefaultHandler_GetData(
	    IDataObject*     iface,
	    LPFORMATETC      pformatetcIn,
	    STGMEDIUM*       pmedium)
{
  IDataObject* cacheDataObject = NULL;
  HRESULT      hres;

  DefaultHandler *This = impl_from_IDataObject(iface);

  TRACE("(%p, %p, %p)\n", iface, pformatetcIn, pmedium);

  hres = IUnknown_QueryInterface(This->dataCache,
				 &IID_IDataObject,
				 (void**)&cacheDataObject);

  if (FAILED(hres))
    return E_UNEXPECTED;

  hres = IDataObject_GetData(cacheDataObject,
			     pformatetcIn,
			     pmedium);

  IDataObject_Release(cacheDataObject);

  if (hres == S_OK) return hres;

  if (object_is_running( This ))
  {
    start_object_call(This);
    hres = IDataObject_GetData(This->pDataDelegate, pformatetcIn, pmedium);
    end_object_call(This);
    if (hres == S_OK) return hres;
  }

  /* Query running state again, as the object may have closed during _GetData call */
  if (!object_is_running( This ))
    hres = OLE_E_NOTRUNNING;

  return hres;
}

static HRESULT WINAPI DefaultHandler_GetDataHere(
	    IDataObject*     iface,
	    LPFORMATETC      pformatetc,
	    STGMEDIUM*       pmedium)
{
  FIXME(": Stub\n");
  return E_NOTIMPL;
}

/************************************************************************
 * DefaultHandler_QueryGetData (IDataObject)
 *
 * The default handler's implementation of this method delegates to
 * the cache.
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI DefaultHandler_QueryGetData(
	    IDataObject*     iface,
	    LPFORMATETC      pformatetc)
{
  IDataObject* cacheDataObject = NULL;
  HRESULT      hres;

  DefaultHandler *This = impl_from_IDataObject(iface);

  TRACE("(%p, %p)\n", iface, pformatetc);

  hres = IUnknown_QueryInterface(This->dataCache,
				 &IID_IDataObject,
				 (void**)&cacheDataObject);

  if (FAILED(hres))
    return E_UNEXPECTED;

  hres = IDataObject_QueryGetData(cacheDataObject,
				  pformatetc);

  IDataObject_Release(cacheDataObject);

  if (hres == S_OK) return hres;

  if (object_is_running( This ))
  {
    start_object_call( This );
    hres = IDataObject_QueryGetData(This->pDataDelegate, pformatetc);
    end_object_call( This );
    if (hres == S_OK) return hres;
  }

  /* Query running state again, as the object may have closed during _QueryGetData call */
  if (!object_is_running( This ))
    hres = OLE_E_NOTRUNNING;

  return hres;
}

/************************************************************************
 * DefaultHandler_GetCanonicalFormatEtc (IDataObject)
 *
 * This method is meaningless if the server is not running
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI DefaultHandler_GetCanonicalFormatEtc(
	    IDataObject*     iface,
	    LPFORMATETC      pformatetcIn,
	    LPFORMATETC      pformatetcOut)
{
  DefaultHandler *This = impl_from_IDataObject(iface);
  HRESULT hr;

  TRACE("(%p, %p, %p)\n", iface, pformatetcIn, pformatetcOut);

  if (!object_is_running( This ))
    return OLE_E_NOTRUNNING;

  start_object_call( This );
  hr = IDataObject_GetCanonicalFormatEtc(This->pDataDelegate, pformatetcIn, pformatetcOut);
  end_object_call( This );

  return hr;
}

/************************************************************************
 * DefaultHandler_SetData (IDataObject)
 *
 * The default handler's implementation of this method delegates to
 * the cache.
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI DefaultHandler_SetData(
	    IDataObject*     iface,
	    LPFORMATETC      pformatetc,
	    STGMEDIUM*       pmedium,
	    BOOL             fRelease)
{
  DefaultHandler *This = impl_from_IDataObject(iface);
  IDataObject* cacheDataObject = NULL;
  HRESULT      hres;

  TRACE("(%p, %p, %p, %d)\n", iface, pformatetc, pmedium, fRelease);

  hres = IUnknown_QueryInterface(This->dataCache,
				 &IID_IDataObject,
				 (void**)&cacheDataObject);

  if (FAILED(hres))
    return E_UNEXPECTED;

  hres = IDataObject_SetData(cacheDataObject,
			     pformatetc,
			     pmedium,
			     fRelease);

  IDataObject_Release(cacheDataObject);

  return hres;
}

/************************************************************************
 * DefaultHandler_EnumFormatEtc (IDataObject)
 *
 * The default handler's implementation of This method simply delegates
 * to OleRegEnumFormatEtc.
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI DefaultHandler_EnumFormatEtc(
	    IDataObject*     iface,
	    DWORD            dwDirection,
	    IEnumFORMATETC** ppenumFormatEtc)
{
  DefaultHandler *This = impl_from_IDataObject(iface);

  TRACE("%p, %lx, %p.\n", iface, dwDirection, ppenumFormatEtc);

  return OleRegEnumFormatEtc(&This->clsid, dwDirection, ppenumFormatEtc);
}

/************************************************************************
 * DefaultHandler_DAdvise (IDataObject)
 *
 * The default handler's implementation of this method simply
 * delegates to the DataAdviseHolder.
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI DefaultHandler_DAdvise(
	    IDataObject*     iface,
	    FORMATETC*       pformatetc,
	    DWORD            advf,
	    IAdviseSink*     pAdvSink,
	    DWORD*           pdwConnection)
{
  HRESULT hres = S_OK;
  DefaultHandler *This = impl_from_IDataObject(iface);

  TRACE("%p, %p, %ld, %p, %p.\n", iface, pformatetc, advf, pAdvSink, pdwConnection);

  /* Make sure we have a data advise holder before we start. */
  if (!This->dataAdviseHolder)
  {
    hres = CreateDataAdviseHolder(&This->dataAdviseHolder);
    if (SUCCEEDED(hres) && object_is_running( This ))
    {
      start_object_call( This );
      DataAdviseHolder_OnConnect(This->dataAdviseHolder, This->pDataDelegate);
      end_object_call( This );
    }
  }

  if (SUCCEEDED(hres))
    hres = IDataAdviseHolder_Advise(This->dataAdviseHolder,
				    iface,
				    pformatetc,
				    advf,
				    pAdvSink,
				    pdwConnection);

  return hres;
}

/************************************************************************
 * DefaultHandler_DUnadvise (IDataObject)
 *
 * The default handler's implementation of this method simply
 * delegates to the DataAdviseHolder.
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI DefaultHandler_DUnadvise(
	    IDataObject*     iface,
	    DWORD            dwConnection)
{
  DefaultHandler *This = impl_from_IDataObject(iface);

  TRACE("%p, %ld.\n", iface, dwConnection);

  /*
   * If we don't have a data advise holder yet, it means that
   * we don't have any connections..
   */
  if (!This->dataAdviseHolder)
    return OLE_E_NOCONNECTION;

  return IDataAdviseHolder_Unadvise(This->dataAdviseHolder,
				    dwConnection);
}

/************************************************************************
 * DefaultHandler_EnumDAdvise (IDataObject)
 *
 * The default handler's implementation of this method simply
 * delegates to the DataAdviseHolder.
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI DefaultHandler_EnumDAdvise(
	    IDataObject*     iface,
	    IEnumSTATDATA**  ppenumAdvise)
{
  DefaultHandler *This = impl_from_IDataObject(iface);

  TRACE("(%p, %p)\n", iface, ppenumAdvise);

  if (!ppenumAdvise)
    return E_POINTER;

  *ppenumAdvise = NULL;

  /* If we have a data advise holder object, delegate. */
  if (This->dataAdviseHolder)
    return IDataAdviseHolder_EnumAdvise(This->dataAdviseHolder,
					ppenumAdvise);

  return S_OK;
}

/*********************************************************
 * Methods implementation for the IRunnableObject part
 * of the DefaultHandler class.
 */

/************************************************************************
 * DefaultHandler_IRunnableObject_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI DefaultHandler_IRunnableObject_QueryInterface(
            IRunnableObject*     iface,
            REFIID               riid,
            void**               ppvObject)
{
  DefaultHandler *This = impl_from_IRunnableObject(iface);

  return IUnknown_QueryInterface(This->outerUnknown, riid, ppvObject);
}

/************************************************************************
 * DefaultHandler_IRunnableObject_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DefaultHandler_IRunnableObject_AddRef(
            IRunnableObject*     iface)
{
  DefaultHandler *This = impl_from_IRunnableObject(iface);

  return IUnknown_AddRef(This->outerUnknown);
}

/************************************************************************
 * DefaultHandler_IRunnableObject_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DefaultHandler_IRunnableObject_Release(
            IRunnableObject*     iface)
{
  DefaultHandler *This = impl_from_IRunnableObject(iface);

  return IUnknown_Release(This->outerUnknown);
}

/************************************************************************
 * DefaultHandler_GetRunningClass (IRunnableObject)
 *
 * See Windows documentation for more details on IRunnableObject methods.
 */
static HRESULT WINAPI DefaultHandler_GetRunningClass(
            IRunnableObject*     iface,
	    LPCLSID              lpClsid)
{
  FIXME("()\n");
  return S_OK;
}

static HRESULT WINAPI DefaultHandler_Run(
            IRunnableObject*     iface,
	    IBindCtx*            pbc)
{
  DefaultHandler *This = impl_from_IRunnableObject(iface);
  HRESULT hr;
  IOleCacheControl *cache_ctrl;

  FIXME("(%p): semi-stub\n", pbc);

  /* already running? if so nothing to do */
  if (object_is_running(This))
    return S_OK;

  release_delegates(This);

  hr = CoCreateInstance(&This->clsid, NULL, CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER,
                        &IID_IOleObject, (void **)&This->pOleDelegate);
  if (FAILED(hr))
    return hr;

  hr = IOleObject_Advise(This->pOleDelegate, &This->IAdviseSink_iface, &This->dwAdvConn);
  if (FAILED(hr)) goto fail;

  if (This->clientSite)
  {
    hr = IOleObject_SetClientSite(This->pOleDelegate, This->clientSite);
    if (FAILED(hr)) goto fail;
  }

  hr = IOleObject_QueryInterface(This->pOleDelegate, &IID_IPersistStorage,
                                 (void **)&This->pPSDelegate);
  if (FAILED(hr)) goto fail;

  if (This->storage_state == storage_state_initialised)
      hr = IPersistStorage_InitNew(This->pPSDelegate, This->storage);
  else if (This->storage_state == storage_state_loaded)
      hr = IPersistStorage_Load(This->pPSDelegate, This->storage);
  if (FAILED(hr)) goto fail;

  if (This->containerApp)
  {
    hr = IOleObject_SetHostNames(This->pOleDelegate, This->containerApp,
                                 This->containerObj);
    if (FAILED(hr)) goto fail;
  }

  /* FIXME: do more stuff here:
   * - IOleObject_GetMiscStatus
   * - IOleObject_GetMoniker
   */

  hr = IOleObject_QueryInterface(This->pOleDelegate, &IID_IDataObject,
                                 (void **)&This->pDataDelegate);
  if (FAILED(hr)) goto fail;

  This->object_state = object_state_running;

  if (This->dataAdviseHolder)
  {
    hr = DataAdviseHolder_OnConnect(This->dataAdviseHolder, This->pDataDelegate);
    if (FAILED(hr)) goto fail;
  }

  hr = IUnknown_QueryInterface( This->dataCache, &IID_IOleCacheControl, (void **)&cache_ctrl );
  if (FAILED(hr)) goto fail;
  hr = IOleCacheControl_OnRun( cache_ctrl, This->pDataDelegate );
  IOleCacheControl_Release( cache_ctrl );
  if (FAILED(hr)) goto fail;

  return hr;

fail:
  DefaultHandler_Stop(This);
  return hr;
}

/************************************************************************
 * DefaultHandler_IsRunning (IRunnableObject)
 *
 * See Windows documentation for more details on IRunnableObject methods.
 */
static BOOL    WINAPI DefaultHandler_IsRunning(
            IRunnableObject*     iface)
{
  DefaultHandler *This = impl_from_IRunnableObject(iface);

  TRACE("()\n");

  if (This->object_state == object_state_running)
    return TRUE;
  else
    return FALSE;
}

/************************************************************************
 * DefaultHandler_LockRunning (IRunnableObject)
 *
 * See Windows documentation for more details on IRunnableObject methods.
 */
static HRESULT WINAPI DefaultHandler_LockRunning(
            IRunnableObject*     iface,
	    BOOL                 fLock,
	    BOOL                 fLastUnlockCloses)
{
  FIXME("()\n");
  return S_OK;
}

/************************************************************************
 * DefaultHandler_SetContainedObject (IRunnableObject)
 *
 * See Windows documentation for more details on IRunnableObject methods.
 */
static HRESULT WINAPI DefaultHandler_SetContainedObject(
            IRunnableObject*     iface,
	    BOOL                 fContained)
{
  FIXME("()\n");
  return S_OK;
}

static HRESULT WINAPI DefaultHandler_IAdviseSink_QueryInterface(
    IAdviseSink *iface,
    REFIID riid,
    void **ppvObject)
{
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAdviseSink))
    {
        *ppvObject = iface;
        IAdviseSink_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI DefaultHandler_IAdviseSink_AddRef(
    IAdviseSink *iface)
{
    DefaultHandler *This = impl_from_IAdviseSink(iface);

    return IUnknown_AddRef(&This->IUnknown_iface);
}

static ULONG WINAPI DefaultHandler_IAdviseSink_Release(
            IAdviseSink *iface)
{
    DefaultHandler *This = impl_from_IAdviseSink(iface);

    return IUnknown_Release(&This->IUnknown_iface);
}

static void WINAPI DefaultHandler_IAdviseSink_OnDataChange(
    IAdviseSink *iface,
    FORMATETC *pFormatetc,
    STGMEDIUM *pStgmed)
{
    FIXME(": stub\n");
}

static void WINAPI DefaultHandler_IAdviseSink_OnViewChange(
    IAdviseSink *iface,
    DWORD dwAspect,
    LONG lindex)
{
    FIXME(": stub\n");
}

static void WINAPI DefaultHandler_IAdviseSink_OnRename(
    IAdviseSink *iface,
    IMoniker *pmk)
{
    DefaultHandler *This = impl_from_IAdviseSink(iface);

    TRACE("(%p)\n", pmk);

    if (This->oleAdviseHolder)
        IOleAdviseHolder_SendOnRename(This->oleAdviseHolder, pmk);
}

static void WINAPI DefaultHandler_IAdviseSink_OnSave(
    IAdviseSink *iface)
{
    DefaultHandler *This = impl_from_IAdviseSink(iface);

    TRACE("()\n");

    if (This->oleAdviseHolder)
        IOleAdviseHolder_SendOnSave(This->oleAdviseHolder);
}

static void WINAPI DefaultHandler_IAdviseSink_OnClose(
    IAdviseSink *iface)
{
    DefaultHandler *This = impl_from_IAdviseSink(iface);
    
    TRACE("()\n");

    if (This->oleAdviseHolder)
        IOleAdviseHolder_SendOnClose(This->oleAdviseHolder);

    if(!This->in_call)
        DefaultHandler_Stop(This);
    else
    {
        TRACE("OnClose during call.  Deferring shutdown\n");
        This->object_state = object_state_deferred_close;
    }
}

/************************************************************************
 * DefaultHandler_IPersistStorage_QueryInterface
 *
 */
static HRESULT WINAPI DefaultHandler_IPersistStorage_QueryInterface(
            IPersistStorage*     iface,
            REFIID               riid,
            void**               ppvObject)
{
  DefaultHandler *This = impl_from_IPersistStorage(iface);

  return IUnknown_QueryInterface(This->outerUnknown, riid, ppvObject);
}

/************************************************************************
 * DefaultHandler_IPersistStorage_AddRef
 *
 */
static ULONG WINAPI DefaultHandler_IPersistStorage_AddRef(
            IPersistStorage*     iface)
{
  DefaultHandler *This = impl_from_IPersistStorage(iface);

  return IUnknown_AddRef(This->outerUnknown);
}

/************************************************************************
 * DefaultHandler_IPersistStorage_Release
 *
 */
static ULONG WINAPI DefaultHandler_IPersistStorage_Release(
            IPersistStorage*     iface)
{
  DefaultHandler *This = impl_from_IPersistStorage(iface);

  return IUnknown_Release(This->outerUnknown);
}

/************************************************************************
 * DefaultHandler_IPersistStorage_GetClassID
 *
 */
static HRESULT WINAPI DefaultHandler_IPersistStorage_GetClassID(
            IPersistStorage*     iface,
            CLSID*               clsid)
{
    DefaultHandler *This = impl_from_IPersistStorage(iface);
    HRESULT hr;

    TRACE("(%p)->(%p)\n", iface, clsid);

    if(object_is_running(This))
    {
        start_object_call( This );
        hr = IPersistStorage_GetClassID(This->pPSDelegate, clsid);
        end_object_call( This );
    }
    else
        hr = IPersistStorage_GetClassID(This->dataCache_PersistStg, clsid);

    return hr;
}

/************************************************************************
 * DefaultHandler_IPersistStorage_IsDirty
 *
 */
static HRESULT WINAPI DefaultHandler_IPersistStorage_IsDirty(
            IPersistStorage*     iface)
{
    DefaultHandler *This = impl_from_IPersistStorage(iface);
    HRESULT hr;

    TRACE("(%p)\n", iface);

    hr = IPersistStorage_IsDirty(This->dataCache_PersistStg);
    if(hr != S_FALSE) return hr;

    if(object_is_running(This))
    {
        start_object_call( This );
        hr = IPersistStorage_IsDirty(This->pPSDelegate);
        end_object_call( This );
    }

    return hr;
}

/***********************************************************************
 *
 * The format of '\1Ole' stream is as follows:
 *
 * DWORD Version == 0x02000001
 * DWORD Flags - low bit set indicates the object is a link otherwise it's embedded.
 * DWORD LinkupdateOption - [MS-OLEDS describes this as an implementation specific hint
 *                           supplied by the app that creates the data structure.  May be
 *                           ignored on processing].
 *
 * DWORD Reserved == 0
 * DWORD MonikerStreamSize - size of the rest of the data (ie CLSID + moniker stream data).
 * CLSID clsid - class id of object capable of processing the moniker
 * BYTE  data[] - moniker data for a link
 */

typedef struct
{
    DWORD version;
    DWORD flags;
    DWORD link_update_opt;
    DWORD res;
    DWORD moniker_size;
} ole_stream_header_t;
static const DWORD ole_stream_version = 0x02000001;

static HRESULT load_ole_stream(DefaultHandler *This, IStorage *storage)
{
    IStream *stream;
    HRESULT hr;

    hr = IStorage_OpenStream(storage, L"\1Ole", NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stream);

    if(SUCCEEDED(hr))
    {
        DWORD read;
        ole_stream_header_t header;

        hr = IStream_Read(stream, &header, sizeof(header), &read);
        if(hr == S_OK && read == sizeof(header) && header.version == ole_stream_version)
        {
            if(header.flags & 1)
            {
                /* FIXME: Read the moniker and deal with the link */
                FIXME("Linked objects are not supported yet\n");
            }
        }
        else
        {
            WARN("Incorrect OleStream header\n");
            hr = DV_E_CLIPFORMAT;
        }
        IStream_Release(stream);
    }
    else
        hr = STORAGE_CreateOleStream(storage, 0);

    return hr;
}

/************************************************************************
 * DefaultHandler_IPersistStorage_InitNew
 *
 */
static HRESULT WINAPI DefaultHandler_IPersistStorage_InitNew(
           IPersistStorage*     iface,
           IStorage*            pStg)
{
    DefaultHandler *This = impl_from_IPersistStorage(iface);
    HRESULT hr;

    TRACE("(%p)->(%p)\n", iface, pStg);
    hr = STORAGE_CreateOleStream(pStg, 0);
    if (hr != S_OK) return hr;

    hr = IPersistStorage_InitNew(This->dataCache_PersistStg, pStg);

    if(SUCCEEDED(hr) && object_is_running(This))
    {
        start_object_call( This );
        hr = IPersistStorage_InitNew(This->pPSDelegate, pStg);
        end_object_call( This );
    }

    if(SUCCEEDED(hr))
    {
        IStorage_AddRef(pStg);
        This->storage = pStg;
        This->storage_state = storage_state_initialised;
    }

    return hr;
}


/************************************************************************
 * DefaultHandler_IPersistStorage_Load
 *
 */
static HRESULT WINAPI DefaultHandler_IPersistStorage_Load(
           IPersistStorage*     iface,
           IStorage*            pStg)
{
    DefaultHandler *This = impl_from_IPersistStorage(iface);
    HRESULT hr;

    TRACE("(%p)->(%p)\n", iface, pStg);

    hr = load_ole_stream(This, pStg);

    if(SUCCEEDED(hr))
        hr = IPersistStorage_Load(This->dataCache_PersistStg, pStg);

    if(SUCCEEDED(hr) && object_is_running(This))
    {
        start_object_call( This );
        hr = IPersistStorage_Load(This->pPSDelegate, pStg);
        end_object_call( This );
    }

    if(SUCCEEDED(hr))
    {
        IStorage_AddRef(pStg);
        This->storage = pStg;
        This->storage_state = storage_state_loaded;
    }
    return hr;
}


/************************************************************************
 * DefaultHandler_IPersistStorage_Save
 *
 */
static HRESULT WINAPI DefaultHandler_IPersistStorage_Save(
           IPersistStorage*     iface,
           IStorage*            pStgSave,
           BOOL                 fSameAsLoad)
{
    DefaultHandler *This = impl_from_IPersistStorage(iface);
    HRESULT hr;

    TRACE("(%p)->(%p, %d)\n", iface, pStgSave, fSameAsLoad);

    hr = IPersistStorage_Save(This->dataCache_PersistStg, pStgSave, fSameAsLoad);
    if(SUCCEEDED(hr) && object_is_running(This))
    {
        start_object_call( This );
        hr = IPersistStorage_Save(This->pPSDelegate, pStgSave, fSameAsLoad);
        end_object_call( This );
    }

    return hr;
}


/************************************************************************
 * DefaultHandler_IPersistStorage_SaveCompleted
 *
 */
static HRESULT WINAPI DefaultHandler_IPersistStorage_SaveCompleted(
           IPersistStorage*     iface,
           IStorage*            pStgNew)
{
    DefaultHandler *This = impl_from_IPersistStorage(iface);
    HRESULT hr;

    TRACE("(%p)->(%p)\n", iface, pStgNew);

    hr = IPersistStorage_SaveCompleted(This->dataCache_PersistStg, pStgNew);

    if(SUCCEEDED(hr) && object_is_running(This))
    {
        start_object_call( This );
        hr = IPersistStorage_SaveCompleted(This->pPSDelegate, pStgNew);
        end_object_call( This );
    }

    if(pStgNew)
    {
        IStorage_AddRef(pStgNew);
        if(This->storage) IStorage_Release(This->storage);
        This->storage = pStgNew;
        This->storage_state = storage_state_loaded;
    }

    return hr;
}


/************************************************************************
 * DefaultHandler_IPersistStorage_HandsOffStorage
 *
 */
static HRESULT WINAPI DefaultHandler_IPersistStorage_HandsOffStorage(
            IPersistStorage*     iface)
{
    DefaultHandler *This = impl_from_IPersistStorage(iface);
    HRESULT hr;

    TRACE("(%p)\n", iface);

    hr = IPersistStorage_HandsOffStorage(This->dataCache_PersistStg);

    if(SUCCEEDED(hr) && object_is_running(This))
    {
        start_object_call( This );
        hr = IPersistStorage_HandsOffStorage(This->pPSDelegate);
        end_object_call( This );
    }

    if(This->storage) IStorage_Release(This->storage);
    This->storage = NULL;
    This->storage_state = storage_state_uninitialised;

    return hr;
}


/*
 * Virtual function tables for the DefaultHandler class.
 */
static const IOleObjectVtbl DefaultHandler_IOleObject_VTable =
{
  DefaultHandler_QueryInterface,
  DefaultHandler_AddRef,
  DefaultHandler_Release,
  DefaultHandler_SetClientSite,
  DefaultHandler_GetClientSite,
  DefaultHandler_SetHostNames,
  DefaultHandler_Close,
  DefaultHandler_SetMoniker,
  DefaultHandler_GetMoniker,
  DefaultHandler_InitFromData,
  DefaultHandler_GetClipboardData,
  DefaultHandler_DoVerb,
  DefaultHandler_EnumVerbs,
  DefaultHandler_Update,
  DefaultHandler_IsUpToDate,
  DefaultHandler_GetUserClassID,
  DefaultHandler_GetUserType,
  DefaultHandler_SetExtent,
  DefaultHandler_GetExtent,
  DefaultHandler_Advise,
  DefaultHandler_Unadvise,
  DefaultHandler_EnumAdvise,
  DefaultHandler_GetMiscStatus,
  DefaultHandler_SetColorScheme
};

static const IUnknownVtbl DefaultHandler_NDIUnknown_VTable =
{
  DefaultHandler_NDIUnknown_QueryInterface,
  DefaultHandler_NDIUnknown_AddRef,
  DefaultHandler_NDIUnknown_Release,
};

static const IDataObjectVtbl DefaultHandler_IDataObject_VTable =
{
  DefaultHandler_IDataObject_QueryInterface,
  DefaultHandler_IDataObject_AddRef,
  DefaultHandler_IDataObject_Release,
  DefaultHandler_GetData,
  DefaultHandler_GetDataHere,
  DefaultHandler_QueryGetData,
  DefaultHandler_GetCanonicalFormatEtc,
  DefaultHandler_SetData,
  DefaultHandler_EnumFormatEtc,
  DefaultHandler_DAdvise,
  DefaultHandler_DUnadvise,
  DefaultHandler_EnumDAdvise
};

static const IRunnableObjectVtbl DefaultHandler_IRunnableObject_VTable =
{
  DefaultHandler_IRunnableObject_QueryInterface,
  DefaultHandler_IRunnableObject_AddRef,
  DefaultHandler_IRunnableObject_Release,
  DefaultHandler_GetRunningClass,
  DefaultHandler_Run,
  DefaultHandler_IsRunning,
  DefaultHandler_LockRunning,
  DefaultHandler_SetContainedObject
};

static const IAdviseSinkVtbl DefaultHandler_IAdviseSink_VTable =
{
  DefaultHandler_IAdviseSink_QueryInterface,
  DefaultHandler_IAdviseSink_AddRef,
  DefaultHandler_IAdviseSink_Release,
  DefaultHandler_IAdviseSink_OnDataChange,
  DefaultHandler_IAdviseSink_OnViewChange,
  DefaultHandler_IAdviseSink_OnRename,
  DefaultHandler_IAdviseSink_OnSave,
  DefaultHandler_IAdviseSink_OnClose
};

static const IPersistStorageVtbl DefaultHandler_IPersistStorage_VTable =
{
  DefaultHandler_IPersistStorage_QueryInterface,
  DefaultHandler_IPersistStorage_AddRef,
  DefaultHandler_IPersistStorage_Release,
  DefaultHandler_IPersistStorage_GetClassID,
  DefaultHandler_IPersistStorage_IsDirty,
  DefaultHandler_IPersistStorage_InitNew,
  DefaultHandler_IPersistStorage_Load,
  DefaultHandler_IPersistStorage_Save,
  DefaultHandler_IPersistStorage_SaveCompleted,
  DefaultHandler_IPersistStorage_HandsOffStorage
};

/*********************************************************
 * Methods implementation for the DefaultHandler class.
 */
static DefaultHandler* DefaultHandler_Construct(
  REFCLSID  clsid,
  LPUNKNOWN pUnkOuter,
  DWORD flags,
  IClassFactory *pCF)
{
  DefaultHandler* This = NULL;
  HRESULT hr;

  This = HeapAlloc(GetProcessHeap(), 0, sizeof(DefaultHandler));

  if (!This)
    return This;

  This->IOleObject_iface.lpVtbl = &DefaultHandler_IOleObject_VTable;
  This->IUnknown_iface.lpVtbl = &DefaultHandler_NDIUnknown_VTable;
  This->IDataObject_iface.lpVtbl = &DefaultHandler_IDataObject_VTable;
  This->IRunnableObject_iface.lpVtbl = &DefaultHandler_IRunnableObject_VTable;
  This->IAdviseSink_iface.lpVtbl = &DefaultHandler_IAdviseSink_VTable;
  This->IPersistStorage_iface.lpVtbl = &DefaultHandler_IPersistStorage_VTable;

  This->inproc_server = (flags & EMBDHLP_INPROC_SERVER) != 0;

  /*
   * Start with one reference count. The caller of this function
   * must release the interface pointer when it is done.
   */
  This->ref = 1;

  /*
   * Initialize the outer unknown
   * We don't keep a reference on the outer unknown since, the way
   * aggregation works, our lifetime is at least as large as its
   * lifetime.
   */
  if (!pUnkOuter)
    pUnkOuter = &This->IUnknown_iface;

  This->outerUnknown = pUnkOuter;

  /*
   * Create a datacache object.
   * We aggregate with the datacache. Make sure we pass our outer
   * unknown as the datacache's outer unknown.
   */
  hr = CreateDataCache(This->outerUnknown,
                       clsid,
                       &IID_IUnknown,
                       (void**)&This->dataCache);
  if(SUCCEEDED(hr))
  {
    hr = IUnknown_QueryInterface(This->dataCache, &IID_IPersistStorage, (void**)&This->dataCache_PersistStg);
    /* keeping a reference to This->dataCache_PersistStg causes us to keep a
     * reference on the outer object */
    if (SUCCEEDED(hr))
        IUnknown_Release(This->outerUnknown);
    else
        IUnknown_Release(This->dataCache);
  }
  if(FAILED(hr))
  {
    ERR("Unexpected error creating data cache\n");
    HeapFree(GetProcessHeap(), 0, This);
    return NULL;
  }

  This->clsid = *clsid;
  This->clientSite = NULL;
  This->oleAdviseHolder = NULL;
  This->dataAdviseHolder = NULL;
  This->containerApp = NULL;
  This->containerObj = NULL;
  This->pOleDelegate = NULL;
  This->pPSDelegate = NULL;
  This->pDataDelegate = NULL;
  This->object_state = object_state_not_running;
  This->in_call = 0;

  This->dwAdvConn = 0;
  This->storage = NULL;
  This->storage_state = storage_state_uninitialised;

  if (This->inproc_server && !(flags & EMBDHLP_DELAYCREATE))
  {
    HRESULT hr;
    This->pCFObject = NULL;
    if (pCF)
      hr = IClassFactory_CreateInstance(pCF, NULL, &IID_IOleObject, (void **)&This->pOleDelegate);
    else
      hr = CoCreateInstance(&This->clsid, NULL, CLSCTX_INPROC_SERVER,
                            &IID_IOleObject, (void **)&This->pOleDelegate);
    if (SUCCEEDED(hr))
      hr = IOleObject_QueryInterface(This->pOleDelegate, &IID_IPersistStorage, (void **)&This->pPSDelegate);
    if (SUCCEEDED(hr))
      hr = IOleObject_QueryInterface(This->pOleDelegate, &IID_IDataObject, (void **)&This->pDataDelegate);
    if (SUCCEEDED(hr))
      This->object_state = object_state_running;
    if (FAILED(hr))
      WARN("object creation failed with error %#lx\n", hr);
  }
  else
  {
    This->pCFObject = pCF;
    if (pCF) IClassFactory_AddRef(pCF);
  }

  return This;
}

static void DefaultHandler_Destroy(
  DefaultHandler* This)
{
  TRACE("(%p)\n", This);

  /* AddRef/Release may be called on this object during destruction.
   * Prevent the object being destroyed recursively by artificially raising
   * the reference count. */
  This->ref = 10000;

  /* release delegates */
  DefaultHandler_Stop(This);

  HeapFree( GetProcessHeap(), 0, This->containerApp );
  This->containerApp = NULL;
  HeapFree( GetProcessHeap(), 0, This->containerObj );
  This->containerObj = NULL;

  if (This->dataCache)
  {
    /* to balance out the release of dataCache_PersistStg which will result
     * in a reference being released from the outer unknown */
    IUnknown_AddRef(This->outerUnknown);
    IPersistStorage_Release(This->dataCache_PersistStg);
    IUnknown_Release(This->dataCache);
    This->dataCache_PersistStg = NULL;
    This->dataCache = NULL;
  }

  if (This->clientSite)
  {
    IOleClientSite_Release(This->clientSite);
    This->clientSite = NULL;
  }

  if (This->oleAdviseHolder)
  {
    IOleAdviseHolder_Release(This->oleAdviseHolder);
    This->oleAdviseHolder = NULL;
  }

  if (This->dataAdviseHolder)
  {
    IDataAdviseHolder_Release(This->dataAdviseHolder);
    This->dataAdviseHolder = NULL;
  }

  if (This->storage)
  {
    IStorage_Release(This->storage);
    This->storage = NULL;
  }

  if (This->pCFObject)
  {
    IClassFactory_Release(This->pCFObject);
    This->pCFObject = NULL;
  }

  HeapFree(GetProcessHeap(), 0, This);
}

/******************************************************************************
 * OleCreateEmbeddingHelper [OLE32.@]
 */
HRESULT WINAPI OleCreateEmbeddingHelper(
  REFCLSID  clsid,
  LPUNKNOWN pUnkOuter,
  DWORD     flags,
  IClassFactory *pCF,
  REFIID    riid,
  LPVOID*   ppvObj)
{
  DefaultHandler* newHandler = NULL;
  HRESULT         hr         = S_OK;

  TRACE("%s, %p, %#lx, %p, %s, %p.\n", debugstr_guid(clsid), pUnkOuter, flags, pCF, debugstr_guid(riid), ppvObj);

  if (!ppvObj)
    return E_POINTER;

  *ppvObj = NULL;

  /*
   * If This handler is constructed for aggregation, make sure
   * the caller is requesting the IUnknown interface.
   * This is necessary because it's the only time the non-delegating
   * IUnknown pointer can be returned to the outside.
   */
  if (pUnkOuter && !IsEqualIID(&IID_IUnknown, riid))
    return CLASS_E_NOAGGREGATION;

  /*
   * Try to construct a new instance of the class.
   */
  newHandler = DefaultHandler_Construct(clsid, pUnkOuter, flags, pCF);

  if (!newHandler)
    return E_OUTOFMEMORY;

  /*
   * Make sure it supports the interface required by the caller.
   */
  hr = IUnknown_QueryInterface(&newHandler->IUnknown_iface, riid, ppvObj);

  /*
   * Release the reference obtained in the constructor. If
   * the QueryInterface was unsuccessful, it will free the class.
   */
  IUnknown_Release(&newHandler->IUnknown_iface);

  return hr;
}


/******************************************************************************
 * OleCreateDefaultHandler [OLE32.@]
 */
HRESULT WINAPI OleCreateDefaultHandler(REFCLSID clsid, LPUNKNOWN pUnkOuter,
                                       REFIID riid, LPVOID* ppvObj)
{
    TRACE("(%s, %p, %s, %p)\n", debugstr_guid(clsid), pUnkOuter,debugstr_guid(riid), ppvObj);
    return OleCreateEmbeddingHelper(clsid, pUnkOuter, EMBDHLP_INPROC_HANDLER | EMBDHLP_CREATENOW,
                                    NULL, riid, ppvObj);
}

typedef struct HandlerCF
{
    IClassFactory IClassFactory_iface;
    LONG refs;
    CLSID clsid;
} HandlerCF;

static inline HandlerCF *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, HandlerCF, IClassFactory_iface);
}

static HRESULT WINAPI
HandlerCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid,&IID_IUnknown) ||
        IsEqualIID(riid,&IID_IClassFactory))
    {
        *ppv = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI HandlerCF_AddRef(LPCLASSFACTORY iface)
{
    HandlerCF *This = impl_from_IClassFactory(iface);
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI HandlerCF_Release(LPCLASSFACTORY iface)
{
    HandlerCF *This = impl_from_IClassFactory(iface);
    ULONG refs = InterlockedDecrement(&This->refs);
    if (!refs)
        HeapFree(GetProcessHeap(), 0, This);
    return refs;
}

static HRESULT WINAPI
HandlerCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pUnk,
                         REFIID riid, LPVOID *ppv)
{
    HandlerCF *This = impl_from_IClassFactory(iface);
    return OleCreateDefaultHandler(&This->clsid, pUnk, riid, ppv);
}

static HRESULT WINAPI HandlerCF_LockServer(LPCLASSFACTORY iface, BOOL fLock)
{
    FIXME("(%d), stub!\n",fLock);
    return S_OK;
}

static const IClassFactoryVtbl HandlerClassFactoryVtbl = {
    HandlerCF_QueryInterface,
    HandlerCF_AddRef,
    HandlerCF_Release,
    HandlerCF_CreateInstance,
    HandlerCF_LockServer
};

HRESULT HandlerCF_Create(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    HRESULT hr;
    HandlerCF *This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This) return E_OUTOFMEMORY;
    This->IClassFactory_iface.lpVtbl = &HandlerClassFactoryVtbl;
    This->refs = 0;
    This->clsid = *rclsid;

    hr = IClassFactory_QueryInterface(&This->IClassFactory_iface, riid, ppv);
    if (FAILED(hr))
        HeapFree(GetProcessHeap(), 0, This);

    return hr;
}
