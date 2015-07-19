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

#include <wine/config.h>

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <objbase.h>
#include <olectl.h>
#include <msctf.h>
#include <shlwapi.h>

#include <wine/list.h>
#include <wine/unicode.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(msctf);

#define COOKIE_MAGIC_TMSINK  0x0010
#define COOKIE_MAGIC_CONTEXTSINK 0x0020
#define COOKIE_MAGIC_GUIDATOM 0x0030
#define COOKIE_MAGIC_IPPSINK 0x0040
#define COOKIE_MAGIC_EDITCOOKIE 0x0050
#define COOKIE_MAGIC_COMPARTMENTSINK 0x0060

extern DWORD tlsIndex DECLSPEC_HIDDEN;
extern TfClientId processId DECLSPEC_HIDDEN;
extern ITfCompartmentMgr *globalCompartmentMgr DECLSPEC_HIDDEN;

extern HRESULT ThreadMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut) DECLSPEC_HIDDEN;
extern HRESULT DocumentMgr_Constructor(ITfThreadMgrEventSink*, ITfDocumentMgr **ppOut) DECLSPEC_HIDDEN;
extern HRESULT Context_Constructor(TfClientId tidOwner, IUnknown *punk, ITfDocumentMgr *mgr, ITfContext **ppOut, TfEditCookie *pecTextStore) DECLSPEC_HIDDEN;
extern HRESULT InputProcessorProfiles_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut) DECLSPEC_HIDDEN;
extern HRESULT CategoryMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut) DECLSPEC_HIDDEN;
extern HRESULT Range_Constructor(ITfContext *context, ITextStoreACP *textstore, DWORD lockType, DWORD anchorStart, DWORD anchorEnd, ITfRange **ppOut) DECLSPEC_HIDDEN;
extern HRESULT CompartmentMgr_Constructor(IUnknown *pUnkOuter, REFIID riid, IUnknown **ppOut) DECLSPEC_HIDDEN;
extern HRESULT CompartmentMgr_Destructor(ITfCompartmentMgr *This) DECLSPEC_HIDDEN;
extern HRESULT LangBarMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut) DECLSPEC_HIDDEN;
extern HRESULT DisplayAttributeMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut) DECLSPEC_HIDDEN;

extern HRESULT Context_Initialize(ITfContext *cxt, ITfDocumentMgr *manager) DECLSPEC_HIDDEN;
extern HRESULT Context_Uninitialize(ITfContext *cxt) DECLSPEC_HIDDEN;
extern void    ThreadMgr_OnDocumentMgrDestruction(ITfThreadMgr *tm, ITfDocumentMgr *mgr) DECLSPEC_HIDDEN;
extern HRESULT TF_SELECTION_to_TS_SELECTION_ACP(const TF_SELECTION *tf, TS_SELECTION_ACP *tsAcp) DECLSPEC_HIDDEN;

/* cookie function */
extern DWORD  generate_Cookie(DWORD magic, LPVOID data) DECLSPEC_HIDDEN;
extern DWORD  get_Cookie_magic(DWORD id) DECLSPEC_HIDDEN;
extern LPVOID get_Cookie_data(DWORD id) DECLSPEC_HIDDEN;
extern LPVOID remove_Cookie(DWORD id) DECLSPEC_HIDDEN;
extern DWORD enumerate_Cookie(DWORD magic, DWORD *index) DECLSPEC_HIDDEN;

/* activated text services functions */
extern HRESULT add_active_textservice(TF_LANGUAGEPROFILE *lp) DECLSPEC_HIDDEN;
extern BOOL get_active_textservice(REFCLSID rclsid, TF_LANGUAGEPROFILE *lp) DECLSPEC_HIDDEN;
extern HRESULT activate_textservices(ITfThreadMgr *tm) DECLSPEC_HIDDEN;
extern HRESULT deactivate_textservices(void) DECLSPEC_HIDDEN;

extern CLSID get_textservice_clsid(TfClientId tid) DECLSPEC_HIDDEN;
extern HRESULT get_textservice_sink(TfClientId tid, REFCLSID iid, IUnknown** sink) DECLSPEC_HIDDEN;
extern HRESULT set_textservice_sink(TfClientId tid, REFCLSID iid, IUnknown* sink) DECLSPEC_HIDDEN;

extern const WCHAR szwSystemTIPKey[] DECLSPEC_HIDDEN;
extern const WCHAR szwSystemCTFKey[] DECLSPEC_HIDDEN;

#endif /* __WINE_MSCTF_I_H */
