/*
 * Monikers
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1999 Noomen Hamza
 * Copyright 2005 Robert Shearman (for CodeWeavers)
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

#ifndef __WINE_MONIKER_H__
#define __WINE_MONIKER_H__

DEFINE_OLEGUID( CLSID_FileMoniker,      0x303, 0, 0 );
DEFINE_OLEGUID( CLSID_ItemMoniker,      0x304, 0, 0 );
DEFINE_OLEGUID( CLSID_AntiMoniker,      0x305, 0, 0 );
DEFINE_OLEGUID( CLSID_PointerMoniker,   0x306, 0, 0 );
DEFINE_OLEGUID( CLSID_CompositeMoniker, 0x309, 0, 0 );
DEFINE_OLEGUID( CLSID_ClassMoniker,     0x31a, 0, 0 );
DEFINE_OLEGUID( CLSID_ObjrefMoniker,    0x327, 0, 0 );

HRESULT WINAPI FileMoniker_CreateInstance(IClassFactory *iface, IUnknown *pUnk, REFIID riid, void **ppv);
HRESULT WINAPI ItemMoniker_CreateInstance(IClassFactory *iface, IUnknown *pUnk, REFIID riid, void **ppv);
HRESULT WINAPI AntiMoniker_CreateInstance(IClassFactory *iface, IUnknown *pUnk, REFIID riid, void **ppv);
HRESULT WINAPI CompositeMoniker_CreateInstance(IClassFactory *iface, IUnknown *pUnk, REFIID riid, void **ppv);
HRESULT WINAPI ClassMoniker_CreateInstance(IClassFactory *iface, IUnknown *pUnk, REFIID riid, void **ppv);
HRESULT WINAPI ObjrefMoniker_CreateInstance(IClassFactory *iface, IUnknown *pUnk, REFIID riid, void **ppv);
HRESULT WINAPI PointerMoniker_CreateInstance(IClassFactory *iface, IUnknown *pUnk, REFIID riid, void **ppv);
HRESULT WINAPI ComCat_CreateInstance(IClassFactory *iface, IUnknown *pUnk, REFIID riid, void **ppv);

HRESULT FileMoniker_CreateFromDisplayName(LPBC pbc, LPCOLESTR szDisplayName,
                                          LPDWORD pchEaten, LPMONIKER *ppmk);
HRESULT ClassMoniker_CreateFromDisplayName(LPBC pbc, LPCOLESTR szDisplayName,
                                           LPDWORD pchEaten, LPMONIKER *ppmk);

HRESULT MonikerMarshal_Create(IMoniker *inner, IUnknown **outer);

BOOL is_anti_moniker(IMoniker *iface, DWORD *order);
HRESULT create_anti_moniker(DWORD order, IMoniker **ret);

#endif /* __WINE_MONIKER_H__ */
