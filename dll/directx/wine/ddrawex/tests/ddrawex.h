/*
 * Copyright 2006 Ulrich Czekalla
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

#ifndef __WINE_DDRAWEX_DDRAWEX_H
#define __WINE_DDRAWEX_DDRAWEX_H

DEFINE_GUID(CLSID_DirectDrawFactory, 0x4fd2a832, 0x86c8, 0x11d0, 0x8f, 0xca, 0x0, 0xc0, 0x4f, 0xd9, 0x18, 0x9d);
DEFINE_GUID(IID_IDirectDrawFactory, 0x4fd2a833, 0x86c8, 0x11d0, 0x8f, 0xca, 0x0, 0xc0, 0x4f, 0xd9, 0x18, 0x9d);

#define INTERFACE IDirectDrawFactory
DECLARE_INTERFACE_(IDirectDrawFactory, IUnknown)
{
    STDMETHOD_(HRESULT, QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(CreateDirectDraw)(THIS_ GUID * pGUID, HWND hWnd, DWORD dwCoopLevelFlags,
              DWORD dwReserved, IUnknown *pUnkOuter, IDirectDraw **ppDirectDraw) PURE;
    STDMETHOD(_DirectDrawEnumerate)(THIS_ LPDDENUMCALLBACKW cb, void *ctx) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IDirectDrawFactory_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectDrawFactory_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define IDirectDrawFactory_Release(p)            (p)->lpVtbl->Release(p)
#define IDirectDrawFactory_CreateDirectDraw(p,a,b,c,d,e,f)  (p)->lpVtbl->CreateDirectDraw(p,a,b,c,d,e,f)
#define IDirectDrawFactory_DirectDrawEnumerate(p,a,b)  (p)->lpVtbl->_DirectDrawEnumerate(p,a,b)
#endif

#endif /* __WINE_DDRAWEX_DDRAWEX_H */
