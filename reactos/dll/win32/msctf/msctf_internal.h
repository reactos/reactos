/*
 * Internal header for msctf.dll
 *
 * Copyright 2008 Aric Stewart, CodeWeavers
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

#ifndef __WINE_MSCTF_I_H
#define __WINE_MSCTF_I_H
extern DWORD tlsIndex;

extern HRESULT ThreadMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut);
extern HRESULT DocumentMgr_Constructor(ITfThreadMgrEventSink*, ITfDocumentMgr **ppOut);
extern HRESULT Context_Constructor(TfClientId tidOwner, IUnknown *punk, ITfContext **ppOut, TfEditCookie *pecTextStore);
extern HRESULT InputProcessorProfiles_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut);
extern HRESULT CategoryMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut);

extern const WCHAR szwSystemTIPKey[];
#endif /* __WINE_MSCTF_I_H */
