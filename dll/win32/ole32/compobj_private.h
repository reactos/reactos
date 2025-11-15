/*
 * Copyright 1995 Martin von Loewis
 * Copyright 1998 Justin Bradford
 * Copyright 1999 Francis Beaudet
 * Copyright 1999 Sylvain St-Germain
 * Copyright 2002 Marcus Meissner
 * Copyright 2003 Ove Kåven, TransGaming Technologies
 * Copyright 2004 Mike Hearn, Rob Shearman, CodeWeavers Inc
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

#ifndef __WINE_OLE_COMPOBJ_H
#define __WINE_OLE_COMPOBJ_H

/* All private prototype functions used by OLE will be added to this header file */

#include <stdarg.h>

#include "wine/list.h"
#include "wine/heap.h"

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#ifdef __REACTOS__
#include <ole2.h>
#endif
#include "dcom.h"
#include "winreg.h"
#include "winternl.h"

DEFINE_OLEGUID( CLSID_DfMarshal, 0x0000030b, 0, 0 );

/* this is what is stored in TEB->ReservedForOle */
struct oletls
{
    struct apartment *apt;
    IErrorInfo       *errorinfo;     /* see errorinfo.c */
    DWORD             thread_seqid;  /* returned with CoGetCurrentProcess */
    DWORD             flags;         /* tlsdata_flags (+0Ch on x86) */
    void             *unknown0;
    DWORD             inits;         /* number of times CoInitializeEx called */
    DWORD             ole_inits;     /* number of times OleInitialize called */
    GUID              causality_id;  /* unique identifier for each COM call */
    LONG              pending_call_count_client; /* number of client calls pending */
    LONG              pending_call_count_server; /* number of server calls pending */
    DWORD             unknown;
    IObjContext      *context_token; /* (+38h on x86) */
    IUnknown         *call_state;    /* current call context (+3Ch on x86) */
    DWORD             unknown2[46];
    IUnknown         *cancel_object; /* cancel object set by CoSetCancelObject (+F8h on x86) */
    IUnknown         *state;         /* see CoSetState */
    struct list       spies;         /* Spies installed with CoRegisterInitializeSpy */
    DWORD             spies_lock;
    DWORD             cancelcount;
    CO_MTA_USAGE_COOKIE implicit_mta_cookie; /* mta referenced by roapi from sta thread */
};

/* Global Interface Table Functions */
extern void release_std_git(void);
extern HRESULT StdGlobalInterfaceTable_GetFactory(LPVOID *ppv);

HRESULT COM_OpenKeyForCLSID(REFCLSID clsid, LPCWSTR keyname, REGSAM access, HKEY *key);
HRESULT MARSHAL_GetStandardMarshalCF(LPVOID *ppv);
HRESULT FTMarshalCF_Create(REFIID riid, LPVOID *ppv);

/* Drag and drop */
void OLEDD_UnInitialize(void);

extern HRESULT WINAPI InternalTlsAllocData(struct oletls **tlsdata);

/* will create if necessary */
static inline struct oletls *COM_CurrentInfo(void)
{
    struct oletls *oletls;

    if (!NtCurrentTeb()->ReservedForOle)
        InternalTlsAllocData(&oletls);

    return NtCurrentTeb()->ReservedForOle;
}

static inline struct apartment * COM_CurrentApt(void)
{  
    return COM_CurrentInfo()->apt;
}

#define CHARS_IN_GUID 39 /* including NULL */

/* from dlldata.c */
extern HINSTANCE hProxyDll;
extern HRESULT WINAPI OLE32_DllGetClassObject(REFCLSID rclsid, REFIID iid,LPVOID *ppv);
extern HRESULT WINAPI OLE32_DllRegisterServer(void);
extern HRESULT WINAPI OLE32_DllUnregisterServer(void);

extern HRESULT Handler_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv);
extern HRESULT HandlerCF_Create(REFCLSID rclsid, REFIID riid, LPVOID *ppv);

extern HRESULT WINAPI GlobalOptions_CreateInstance(IClassFactory *iface, IUnknown *pUnk,
                                                   REFIID riid, void **ppv);
extern IClassFactory GlobalOptionsCF;
extern HRESULT WINAPI GlobalInterfaceTable_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid,
        void **obj);
extern IClassFactory GlobalInterfaceTableCF;
extern HRESULT WINAPI ManualResetEvent_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid,
        void **obj);
extern IClassFactory ManualResetEventCF;
extern HRESULT WINAPI Ole32DllGetClassObject(REFCLSID clsid, REFIID riid, void **obj);

/* Exported non-interface Data Advise Holder functions */
HRESULT DataAdviseHolder_OnConnect(IDataAdviseHolder *iface, IDataObject *pDelegate);
void DataAdviseHolder_OnDisconnect(IDataAdviseHolder *iface);

extern UINT ownerlink_clipboard_format;
extern UINT filename_clipboard_format;
extern UINT filenameW_clipboard_format;
extern UINT dataobject_clipboard_format;
extern UINT embedded_object_clipboard_format;
extern UINT embed_source_clipboard_format;
extern UINT custom_link_source_clipboard_format;
extern UINT link_source_clipboard_format;
extern UINT object_descriptor_clipboard_format;
extern UINT link_source_descriptor_clipboard_format;
extern UINT ole_private_data_clipboard_format;

void clipbrd_destroy(void);

extern LSTATUS create_classes_key(HKEY, const WCHAR *, REGSAM, HKEY *);
extern LSTATUS open_classes_key(HKEY, const WCHAR *, REGSAM, HKEY *);

extern BOOL actctx_get_miscstatus(const CLSID*, DWORD, DWORD*);

extern const char *debugstr_formatetc(const FORMATETC *formatetc);

static inline HRESULT copy_formatetc(FORMATETC *dst, const FORMATETC *src)
{
    *dst = *src;
    if (src->ptd)
    {
        dst->ptd = CoTaskMemAlloc( src->ptd->tdSize );
        if (!dst->ptd) return E_OUTOFMEMORY;
        memcpy( dst->ptd, src->ptd, src->ptd->tdSize );
    }
    return S_OK;
}

extern HRESULT EnumSTATDATA_Construct(IUnknown *holder, ULONG index, DWORD array_len, STATDATA *data,
                                      BOOL copy, IEnumSTATDATA **ppenum);

#endif /* __WINE_OLE_COMPOBJ_H */
