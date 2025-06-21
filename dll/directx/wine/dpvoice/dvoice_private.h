/*
 * DirectPlay Voice
 *
 * Copyright (C) 2014 Alistair Leslie-Hughes
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#ifndef __DVOICE_PRIVATE_H__
#define __DVOICE_PRIVATE_H__

extern HRESULT DPVOICE_CreateDirectPlayVoiceClient(IClassFactory *iface, IUnknown *pUnkOuter, REFIID riid, void **ppobj);
extern HRESULT DPVOICE_CreateDirectPlayVoiceServer(IClassFactory *iface, IUnknown *pUnkOuter, REFIID riid, void **ppobj);
extern HRESULT DPVOICE_CreateDirectPlayVoiceTest(IClassFactory *iface, IUnknown *pUnkOuter, REFIID riid, void **ppobj);
extern HRESULT DPVOICE_GetCompressionTypes(DVCOMPRESSIONINFO *pData, DWORD *pdwDataSize, DWORD *pdwNumElements, DWORD dwFlags);

#endif
