/*
 * PROJECT:     ReactOS CTF
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Internal header for msctf.dll
 * COPYRIGHT:   Copyright 2008 Aric Stewart, CodeWeavers
 *              Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

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

extern DWORD tlsIndex DECLSPEC_HIDDEN;
extern TfClientId processId DECLSPEC_HIDDEN;
extern ITfCompartmentMgr *globalCompartmentMgr DECLSPEC_HIDDEN;

extern HRESULT ThreadMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut) DECLSPEC_HIDDEN;
extern HRESULT DocumentMgr_Constructor(ITfThreadMgrEventSink*, ITfDocumentMgr **ppOut) DECLSPEC_HIDDEN;
extern HRESULT Context_Constructor(TfClientId tidOwner, IUnknown *punk, ITfDocumentMgr *mgr, ITfContext **ppOut, TfEditCookie *pecTextStore) DECLSPEC_HIDDEN;
extern HRESULT InputProcessorProfiles_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut) DECLSPEC_HIDDEN;
extern HRESULT CategoryMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut) DECLSPEC_HIDDEN;
extern HRESULT Range_Constructor(ITfContext *context, DWORD anchorStart, DWORD anchorEnd, ITfRange **ppOut) DECLSPEC_HIDDEN;
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
extern HRESULT activate_textservices(ITfThreadMgrEx *tm) DECLSPEC_HIDDEN;
extern HRESULT deactivate_textservices(void) DECLSPEC_HIDDEN;

extern CLSID get_textservice_clsid(TfClientId tid) DECLSPEC_HIDDEN;
extern HRESULT get_textservice_sink(TfClientId tid, REFCLSID iid, IUnknown** sink) DECLSPEC_HIDDEN;
extern HRESULT set_textservice_sink(TfClientId tid, REFCLSID iid, IUnknown* sink) DECLSPEC_HIDDEN;

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

HRESULT advise_sink(struct list *sink_list, REFIID riid, DWORD cookie_magic, IUnknown *unk, DWORD *cookie) DECLSPEC_HIDDEN;
HRESULT unadvise_sink(DWORD cookie) DECLSPEC_HIDDEN;
void free_sinks(struct list *sink_list) DECLSPEC_HIDDEN;

extern const WCHAR szwSystemTIPKey[] DECLSPEC_HIDDEN;
extern const WCHAR szwSystemCTFKey[] DECLSPEC_HIDDEN;

#ifdef __cplusplus
} // extern "C"
#endif
