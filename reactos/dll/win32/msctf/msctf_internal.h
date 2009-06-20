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

#define COOKIE_MAGIC_TMSINK  0x0010
#define COOKIE_MAGIC_CONTEXTSINK 0x0020
#define COOKIE_MAGIC_GUIDATOM 0x0030
#define COOKIE_MAGIC_IPPSINK 0x0040
#define COOKIE_MAGIC_EDITCOOKIE 0x0050

extern DWORD tlsIndex;
extern TfClientId processId;

extern HRESULT ThreadMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut);
extern HRESULT DocumentMgr_Constructor(ITfThreadMgrEventSink*, ITfDocumentMgr **ppOut);
extern HRESULT Context_Constructor(TfClientId tidOwner, IUnknown *punk, ITfContext **ppOut, TfEditCookie *pecTextStore);
extern HRESULT InputProcessorProfiles_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut);
extern HRESULT CategoryMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut);
extern HRESULT Range_Constructor(ITfContext *context, ITextStoreACP *textstore, DWORD lockType, DWORD anchorStart, DWORD anchorEnd, ITfRange **ppOut);

extern HRESULT Context_Initialize(ITfContext *cxt);
extern HRESULT Context_Uninitialize(ITfContext *cxt);

/* cookie function */
extern DWORD  generate_Cookie(DWORD magic, LPVOID data);
extern DWORD  get_Cookie_magic(DWORD id);
extern LPVOID get_Cookie_data(DWORD id);
extern LPVOID remove_Cookie(DWORD id);
extern DWORD enumerate_Cookie(DWORD magic, DWORD *index);

/* activated text services functions */
extern HRESULT add_active_textservice(TF_LANGUAGEPROFILE *lp);
extern BOOL get_active_textservice(REFCLSID rclsid, TF_LANGUAGEPROFILE *lp);
extern HRESULT activate_textservices(ITfThreadMgr *tm);
extern HRESULT deactivate_textservices(void);

extern CLSID get_textservice_clsid(TfClientId tid);
extern HRESULT get_textservice_sink(TfClientId tid, REFCLSID iid, IUnknown** sink);
extern HRESULT set_textservice_sink(TfClientId tid, REFCLSID iid, IUnknown* sink);

extern const WCHAR szwSystemTIPKey[];
#endif /* __WINE_MSCTF_I_H */
