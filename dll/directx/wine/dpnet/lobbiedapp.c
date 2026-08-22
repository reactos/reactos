/*
 * DirectPlay8 LobbiedApplication
 *
 * Copyright 2007 Jason Edmeades
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
 */


#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "objbase.h"
#include "wine/debug.h"

#include "dpnet_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dpnet);

static inline IDirectPlay8LobbiedApplicationImpl *impl_from_IDirectPlay8LobbiedApplication(IDirectPlay8LobbiedApplication *iface)
{
    return CONTAINING_RECORD(iface, IDirectPlay8LobbiedApplicationImpl,
            IDirectPlay8LobbiedApplication_iface);
}

/* IDirectPlay8LobbiedApplication IUnknown parts follow: */
static HRESULT WINAPI IDirectPlay8LobbiedApplicationImpl_QueryInterface(IDirectPlay8LobbiedApplication *iface,
        REFIID riid, void **ret_iface)
{
    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectPlay8LobbiedApplication)) {
        IDirectPlay8LobbiedApplication_AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }

    WARN("(%p)->(%s,%p): not found\n", iface, debugstr_guid(riid), ret_iface);
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectPlay8LobbiedApplicationImpl_AddRef(IDirectPlay8LobbiedApplication *iface)
{
    IDirectPlay8LobbiedApplicationImpl *This = impl_from_IDirectPlay8LobbiedApplication(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before=%lu)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IDirectPlay8LobbiedApplicationImpl_Release(IDirectPlay8LobbiedApplication *iface)
{
    IDirectPlay8LobbiedApplicationImpl *This = impl_from_IDirectPlay8LobbiedApplication(iface);
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before=%lu)\n", This, refCount + 1);

    if (!refCount) {
        HeapFree(GetProcessHeap(), 0, This);
    }
    return refCount;
}

/* IDirectPlay8LobbiedApplication Interface follow: */

static HRESULT WINAPI IDirectPlay8LobbiedApplicationImpl_Initialize(IDirectPlay8LobbiedApplication *iface,
        void * const pvUserContext, const PFNDPNMESSAGEHANDLER pfn,
        DPNHANDLE * const pdpnhConnection, const DWORD dwFlags)
{
    IDirectPlay8LobbiedApplicationImpl *This = impl_from_IDirectPlay8LobbiedApplication(iface);

    TRACE("(%p)->(%p %p %p %lx)\n", This, pvUserContext, pfn, pdpnhConnection, dwFlags);

    if(!pfn)
        return DPNERR_INVALIDPOINTER;

    This->msghandler = pfn;
    This->flags = dwFlags;
    This->usercontext = pvUserContext;
    This->connection = pdpnhConnection;

    init_winsock();

    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8LobbiedApplicationImpl_RegisterProgram(IDirectPlay8LobbiedApplication *iface,
        PDPL_PROGRAM_DESC pdplProgramDesc, const DWORD dwFlags)
{
  IDirectPlay8LobbiedApplicationImpl *This = impl_from_IDirectPlay8LobbiedApplication(iface);
  FIXME("(%p): stub\n", This);
  return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8LobbiedApplicationImpl_UnRegisterProgram(IDirectPlay8LobbiedApplication *iface,
        GUID *pguidApplication, const DWORD dwFlags)
{
  IDirectPlay8LobbiedApplicationImpl *This = impl_from_IDirectPlay8LobbiedApplication(iface);
  FIXME("(%p): stub\n", This);
  return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8LobbiedApplicationImpl_Send(IDirectPlay8LobbiedApplication *iface,
        const DPNHANDLE hConnection, BYTE * const pBuffer, const DWORD pBufferSize,
        const DWORD dwFlags)
{
  IDirectPlay8LobbiedApplicationImpl *This = impl_from_IDirectPlay8LobbiedApplication(iface);
  FIXME("(%p): stub\n", This);
  return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8LobbiedApplicationImpl_SetAppAvailable(IDirectPlay8LobbiedApplication *iface,
        const BOOL fAvailable, const DWORD dwFlags)
{
  IDirectPlay8LobbiedApplicationImpl *This = impl_from_IDirectPlay8LobbiedApplication(iface);
  FIXME("(%p): stub\n", This);
  return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8LobbiedApplicationImpl_UpdateStatus(IDirectPlay8LobbiedApplication *iface,
        const DPNHANDLE hConnection, const DWORD dwStatus, const DWORD dwFlags)
{
  IDirectPlay8LobbiedApplicationImpl *This = impl_from_IDirectPlay8LobbiedApplication(iface);
  FIXME("(%p): stub\n", This);
  return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8LobbiedApplicationImpl_Close(IDirectPlay8LobbiedApplication *iface,
        const DWORD dwFlags)
{
  IDirectPlay8LobbiedApplicationImpl *This = impl_from_IDirectPlay8LobbiedApplication(iface);
  FIXME("(%p): stub\n", This);
  return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8LobbiedApplicationImpl_GetConnectionSettings(IDirectPlay8LobbiedApplication *iface,
        const DPNHANDLE hConnection, DPL_CONNECTION_SETTINGS * const pdplSessionInfo,
        DWORD *pdwInfoSize, const DWORD dwFlags)
{
  IDirectPlay8LobbiedApplicationImpl *This = impl_from_IDirectPlay8LobbiedApplication(iface);
  FIXME("(%p): stub\n", This);
  return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8LobbiedApplicationImpl_SetConnectionSettings(IDirectPlay8LobbiedApplication *iface,
        const DPNHANDLE hConnection, const DPL_CONNECTION_SETTINGS * const pdplSessionInfo,
        const DWORD dwFlags)
{
  IDirectPlay8LobbiedApplicationImpl *This = impl_from_IDirectPlay8LobbiedApplication(iface);
  FIXME("(%p): stub\n", This);
  return DPN_OK;
}

static const IDirectPlay8LobbiedApplicationVtbl DirectPlay8LobbiedApplication_Vtbl =
{
    IDirectPlay8LobbiedApplicationImpl_QueryInterface,
    IDirectPlay8LobbiedApplicationImpl_AddRef,
    IDirectPlay8LobbiedApplicationImpl_Release,
    IDirectPlay8LobbiedApplicationImpl_Initialize,
    IDirectPlay8LobbiedApplicationImpl_RegisterProgram,
    IDirectPlay8LobbiedApplicationImpl_UnRegisterProgram,
    IDirectPlay8LobbiedApplicationImpl_Send,
    IDirectPlay8LobbiedApplicationImpl_SetAppAvailable,
    IDirectPlay8LobbiedApplicationImpl_UpdateStatus,
    IDirectPlay8LobbiedApplicationImpl_Close,
    IDirectPlay8LobbiedApplicationImpl_GetConnectionSettings,
    IDirectPlay8LobbiedApplicationImpl_SetConnectionSettings
};



HRESULT DPNET_CreateDirectPlay8LobbiedApp(LPCLASSFACTORY iface, LPUNKNOWN punkOuter, REFIID riid, LPVOID *ppobj) {
  IDirectPlay8LobbiedApplicationImpl* app;

  TRACE("(%p, %s, %p)\n", punkOuter, debugstr_guid(riid), ppobj);

  app = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectPlay8LobbiedApplicationImpl));
  if (NULL == app) {
    *ppobj = NULL;
    return E_OUTOFMEMORY;
  }
  app->IDirectPlay8LobbiedApplication_iface.lpVtbl = &DirectPlay8LobbiedApplication_Vtbl;
  app->ref = 0;
  return IDirectPlay8LobbiedApplicationImpl_QueryInterface(&app->IDirectPlay8LobbiedApplication_iface,
          riid, ppobj);
}
