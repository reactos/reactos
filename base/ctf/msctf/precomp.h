/*
 * PROJECT:     ReactOS CTF
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Precompiled header for msctf.dll
 * COPYRIGHT:   Copyright 2008 Aric Stewart, CodeWeavers
 *              Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include <initguid.h>
#include <windef.h>
#include <winbase.h>
#include <oleauto.h>
#include <olectl.h>
#include <cguid.h>
#include <msctf.h>
#include <msctf_undoc.h>
#include <tchar.h>
#include <strsafe.h>
#include <wine/list.h>

// Cicero
#include <cicbase.h>
#include <cicarray.h>
#include <cicreg.h>
#include <cicutb.h>

#ifdef __cplusplus
extern "C" {
#endif

#define COOKIE_MAGIC_TMSINK  0x0010
#define COOKIE_MAGIC_CONTEXTSINK 0x0020
#define COOKIE_MAGIC_GUIDATOM 0x0030
#define COOKIE_MAGIC_IPPSINK 0x0040
#define COOKIE_MAGIC_EDITCOOKIE 0x0050
#define COOKIE_MAGIC_COMPARTMENTSINK 0x0060
#define COOKIE_MAGIC_DMSINK 0x0070
#define COOKIE_MAGIC_THREADFOCUSSINK 0x0080
#define COOKIE_MAGIC_KEYTRACESINK 0x0090
#define COOKIE_MAGIC_UIELEMENTSINK 0x00a0
#define COOKIE_MAGIC_INPUTPROCESSORPROFILEACTIVATIONSINK 0x00b0
#define COOKIE_MAGIC_ACTIVELANGSINK 0x00c0

extern DWORD g_dwTLSIndex;
extern TfClientId g_processId;
extern ITfCompartmentMgr *g_globalCompartmentMgr;

HRESULT ThreadMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut);
HRESULT DocumentMgr_Constructor(ITfThreadMgrEventSink*, ITfDocumentMgr **ppOut);
HRESULT Context_Constructor(TfClientId tidOwner, IUnknown *punk, ITfDocumentMgr *mgr, ITfContext **ppOut, TfEditCookie *pecTextStore);
HRESULT InputProcessorProfiles_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut);
HRESULT CategoryMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut);
HRESULT Range_Constructor(ITfContext *context, DWORD anchorStart, DWORD anchorEnd, ITfRange **ppOut);
HRESULT CompartmentMgr_Constructor(IUnknown *pUnkOuter, REFIID riid, IUnknown **ppOut);
HRESULT CompartmentMgr_Destructor(ITfCompartmentMgr *This);
HRESULT LangBarMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut);
HRESULT DisplayAttributeMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut);

HRESULT Context_Initialize(ITfContext *cxt, ITfDocumentMgr *manager);
HRESULT Context_Uninitialize(ITfContext *cxt);
void ThreadMgr_OnDocumentMgrDestruction(ITfThreadMgr *tm, ITfDocumentMgr *mgr);
HRESULT TF_SELECTION_to_TS_SELECTION_ACP(const TF_SELECTION *tf, TS_SELECTION_ACP *tsAcp);

/* cookie function */
DWORD  generate_Cookie(DWORD magic, LPVOID data);
DWORD  get_Cookie_magic(DWORD id);
LPVOID get_Cookie_data(DWORD id);
LPVOID remove_Cookie(DWORD id);
DWORD enumerate_Cookie(DWORD magic, DWORD *index);

/* activated text services functions */
HRESULT add_active_textservice(TF_LANGUAGEPROFILE *lp);
BOOL get_active_textservice(REFCLSID rclsid, TF_LANGUAGEPROFILE *lp);
HRESULT activate_textservices(ITfThreadMgrEx *tm);
HRESULT deactivate_textservices(void);

CLSID get_textservice_clsid(TfClientId tid);
HRESULT get_textservice_sink(TfClientId tid, REFCLSID iid, IUnknown** sink);
HRESULT set_textservice_sink(TfClientId tid, REFCLSID iid, IUnknown* sink);

typedef struct {
    struct list entry;
    union {
        IUnknown *pIUnknown;
        ITfThreadMgrEventSink *pITfThreadMgrEventSink;
        ITfCompartmentEventSink *pITfCompartmentEventSink;
        ITfTextEditSink *pITfTextEditSink;
        ITfLanguageProfileNotifySink *pITfLanguageProfileNotifySink;
        ITfTransitoryExtensionSink *pITfTransitoryExtensionSink;
    } interfaces;
} Sink;

#define SINK_ENTRY(cursor,type) (LIST_ENTRY(cursor,Sink,entry)->interfaces.p##type)
#define SINK_FOR_EACH(cursor,list,type,elem) \
    for ((cursor) = (list)->next, elem = SINK_ENTRY(cursor,type); \
         (cursor) != (list); \
         (cursor) = (cursor)->next, elem = SINK_ENTRY(cursor,type))

HRESULT advise_sink(struct list *sink_list, REFIID riid, DWORD cookie_magic, IUnknown *unk, DWORD *cookie);
HRESULT unadvise_sink(DWORD cookie);
void free_sinks(struct list *sink_list);

#define szwSystemTIPKey L"SOFTWARE\\Microsoft\\CTF\\TIP"
#define szwSystemCTFKey L"SOFTWARE\\Microsoft\\CTF"

HRESULT __wine_register_resources(HMODULE module);
HRESULT __wine_unregister_resources(HMODULE module);

BOOL ProcessAttach(HINSTANCE hinstDLL);
VOID ProcessDetach(HINSTANCE hinstDLL);

#ifdef __cplusplus
} // extern "C"
#endif
