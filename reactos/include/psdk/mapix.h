/*
 * Copyright 2004 Jon Griffiths
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef MAPIX_H
#define MAPIX_H

#ifndef MAPIDEFS_H
#include <mapidefs.h>
#endif
#ifndef MAPICODE_H
#include <mapicode.h>
#endif
#ifndef MAPIGUID_H
#include <mapiguid.h>
#endif
#ifndef MAPITAGS_H
#include <mapitags.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct IProfAdmin IProfAdmin;
typedef IProfAdmin *LPPROFADMIN;
typedef struct IMsgServiceAdmin IMsgServiceAdmin;
typedef IMsgServiceAdmin *LPSERVICEADMIN;
typedef struct IMAPISession *LPMAPISESSION;

#ifndef WINE_FLAGS_DEFINED
#define WINE_FLAGS_DEFINED
typedef unsigned long           FLAGS;
#endif

/* Flags for MAPILogon and MAPILogonEx */
#ifndef MAPI_LOGON_UI
#define MAPI_LOGON_UI           0x00000001
#endif
#ifndef MAPI_NEW_SESSION
#define MAPI_NEW_SESSION        0x00000002
#endif
#define MAPI_ALLOW_OTHERS       0x00000008
#define MAPI_EXPLICIT_PROFILE   0x00000010
#ifndef MAPI_EXTENDED
#define MAPI_EXTENDED           0x00000020
#endif
#ifndef MAPI_FORCE_DOWNLOAD
#define MAPI_FORCE_DOWNLOAD     0x00001000
#endif
#ifndef MAPI_PASSWORD_UI
#define MAPI_PASSWORD_UI        0x00020000
#endif
#define MAPI_SERVICE_UI_ALWAYS  0x00002000
#define MAPI_NO_MAIL            0x00008000
#define MAPI_NT_SERVICE         0x00010000
#define MAPI_TIMEOUT_SHORT      0x00100000

#define MAPI_SIMPLE_DEFAULT  (MAPI_LOGON_UI|MAPI_ALLOW_OTHERS|MAPI_FORCE_DOWNLOAD)
#define MAPI_SIMPLE_EXPLICIT (MAPI_NEW_SESSION|MAPI_EXPLICIT_PROFILE|MAPI_FORCE_DOWNLOAD)

typedef struct tagMAPIINIT_0
{
    ULONG ulVersion;
    ULONG ulFlags;
} MAPIINIT_0, *LPMAPIINIT_0;

typedef MAPIINIT_0 MAPIINIT, *LPMAPIINIT;

#define MAPI_INIT_VERSION 0U

typedef HRESULT (WINAPI MAPIINITIALIZE)(void*);
typedef MAPIINITIALIZE *LPMAPIINITIALIZE;
MAPIINITIALIZE MAPIInitialize;

typedef void (WINAPI MAPIUNINITIALIZE)(void);
typedef MAPIUNINITIALIZE *LPMAPIUNINITIALIZE;
MAPIUNINITIALIZE MAPIUninitialize;

#if defined (UNICODE) || defined (__WINESRC__)
typedef HRESULT (STDMETHODCALLTYPE MAPILOGONEX)(ULONG_PTR,LPWSTR,LPWSTR,ULONG,LPMAPISESSION*);
#else
typedef HRESULT (STDMETHODCALLTYPE MAPILOGONEX)(ULONG_PTR,LPSTR,LPSTR,ULONG,LPMAPISESSION *);
#endif
typedef MAPILOGONEX *LPMAPILOGONEX;
MAPILOGONEX MAPILogonEx;

typedef SCODE (WINAPI MAPIALLOCATEBUFFER)(ULONG,LPVOID*);
typedef MAPIALLOCATEBUFFER *LPMAPIALLOCATEBUFFER;
MAPIALLOCATEBUFFER MAPIAllocateBuffer;

typedef SCODE (WINAPI MAPIALLOCATEMORE)(ULONG,LPVOID,LPVOID*);
typedef MAPIALLOCATEMORE *LPMAPIALLOCATEMORE;
MAPIALLOCATEMORE MAPIAllocateMore;

#ifndef MAPIFREEBUFFER_DEFINED
#define MAPIFREEBUFFER_DEFINED
typedef ULONG (WINAPI MAPIFREEBUFFER)(LPVOID);
typedef MAPIFREEBUFFER *LPMAPIFREEBUFFER;
MAPIFREEBUFFER MAPIFreeBuffer;
#endif

/*****************************************************************************
 * IMAPISession interface
 */
#define INTERFACE IMAPISession
DECLARE_INTERFACE_(IMAPISession,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IMAPISession methods ***/
    STDMETHOD(GetLastError)(THIS_ HRESULT hResult, ULONG ulFlags, LPMAPIERROR *lppMAPIError) PURE;
    STDMETHOD(GetMsgStoresTable)(THIS_ ULONG ulFlags, LPMAPITABLE *lppTable) PURE;
    STDMETHOD(OpenMsgStore)(THIS_ ULONG_PTR ulUIParam, ULONG cbId,
                            LPENTRYID lpId, LPCIID lpIFace, ULONG ulFlags, LPMDB *lppMDB) PURE;
    STDMETHOD(OpenAddressBook)(THIS_ ULONG_PTR ulUIParam, LPCIID iid, ULONG ulFlags, LPADRBOOK *lppAdrBook) PURE;
    STDMETHOD(OpenProfileSection)(THIS_ LPMAPIUID lpUID, LPCIID iid, ULONG ulFlags, LPPROFSECT *lppProf) PURE;
    STDMETHOD(GetStatusTable)(THIS_ ULONG ulFlags, LPMAPITABLE *lppTable) PURE;
    STDMETHOD(OpenEntry)(THIS_ ULONG cbId, LPENTRYID lpId, LPCIID iid,
                         ULONG ulFlags, ULONG *lpType, LPUNKNOWN *lppUnk) PURE;
    STDMETHOD(CompareEntryIDs)(THIS_ ULONG cbLID, LPENTRYID lpLID, ULONG cbRID,
                               LPENTRYID lpRID, ULONG ulFlags, ULONG *lpRes) PURE;
    STDMETHOD(Advise)(THIS_ ULONG cbId, LPENTRYID lpId, ULONG ulMask,
                      LPMAPIADVISESINK lpSink, ULONG *lpCxn) PURE;
    STDMETHOD(Unadvise)(THIS_ ULONG ulConnection) PURE;
    STDMETHOD(MessageOptions)(THIS_ ULONG_PTR ulUIParam, ULONG ulFlags, LPSTR lpszAddr, LPMESSAGE lpMsg) PURE;
    STDMETHOD(QueryDefaultMessageOpt)(THIS_ LPSTR lpszAddr, ULONG ulFlags,
                                      ULONG *lpcVals, LPSPropValue *lppOpts) PURE;
    STDMETHOD(EnumAdrTypes)(THIS_ ULONG ulFlags, ULONG *lpcTypes, LPSTR **lpppszTypes) PURE;
    STDMETHOD(QueryIdentity)(THIS_ ULONG *lpcbId, LPENTRYID *lppEntryID) PURE;
    STDMETHOD(Logoff)(THIS_ ULONG_PTR ulUIParam, ULONG ulFlags, ULONG ulReserved) PURE;
    STDMETHOD(SetDefaultStore)(THIS_ ULONG ulFlags, ULONG cbId, LPENTRYID lpId) PURE;
    STDMETHOD(AdminServices)(THIS_ ULONG ulFlags, LPSERVICEADMIN *lppAdmin) PURE;
    STDMETHOD(ShowForm)(THIS_ ULONG_PTR ulUIParam, LPMDB lpStore,
                        LPMAPIFOLDER lpParent, LPCIID iid, ULONG ulToken,
                        LPMESSAGE lpSent, ULONG ulFlags, ULONG ulStatus,
                        ULONG ulMsgFlags, ULONG ulAccess, LPSTR lpszClass) PURE;
    STDMETHOD(PrepareForm)(THIS_ LPCIID lpIFace, LPMESSAGE lpMsg, ULONG *lpToken) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
        /*** IUnknown methods ***/
#define IMAPISession_QueryInterface(p,a,b)          (p)->lpVtbl->QueryInterface(p,a,b)
#define IMAPISession_AddRef(p)                      (p)->lpVtbl->AddRef(p)
#define IMAPISession_Release(p)                     (p)->lpVtbl->Release(p)
        /*** IMAPISession methods ***/
#define IMAPISession_GetLastError(p,a,b,c)          (p)->lpVtbl->GetLastError(p,a,b,c)
#define IMAPISession_GetMsgStoresTable(p,a,b)       (p)->lpVtbl->GetMsgStoresTable(p,a,b)
#define IMAPISession_OpenMsgStore(p,a,b,c,d,e,f)    (p)->lpVtbl->OpenMsgStore(p,a,b,c,d,e,f)
#define IMAPISession_OpenAddressBook(p,a,b,c,d)     (p)->lpVtbl->OpenAddressBook(p,a,b,c,d)
#define IMAPISession_OpenProfileSection(p,a,b,c,d)  (p)->lpVtbl->OpenProfileSection(p,a,b,c,d)
#define IMAPISession_GetStatusTable(p,a,b)          (p)->lpVtbl->GetStatusTable(p,a,b)
#define IMAPISession_OpenEntry(p,a,b,c,d,e,f)       (p)->lpVtbl->OpenEntry(p,a,b,c,d,e,f)
#define IMAPISession_CompareEntryIDs(p,a,b,c,d,e,f) (p)->lpVtbl->CompareEntryIDs(p,a,b,c,d,e,f)
#define IMAPISession_Advise(p,a,b,c,d,e)            (p)->lpVtbl->Advise(p,a,b,c,d,e)
#define IMAPISession_Unadvise(p,a)                  (p)->lpVtbl->Unadvise(p,a)
#define IMAPISession_MessageOptions(p,a,b,c,d)      (p)->lpVtbl->MessageOptions)(p,a,b,c,d)
#define IMAPISession_QueryDefaultMessageOpt(p,a,b,c,d) \
                                                    (p)->lpVtbl->QueryDefaultMessageOpt(p,a,b,c,d)
#define IMAPISession_EnumAdrTypes(p,a,b,c)          (p)->lpVtbl->EnumAdrTypes(p,a,b,c)
#define IMAPISession_QueryIdentity(p,a,b)           (p)->lpVtbl->QueryIdentity(p,a,b)
#define IMAPISession_Logoff(p,a,b,c)                (p)->lpVtbl->Logoff(p,a,b,c)
#define IMAPISession_SetDefaultStore(p,a,b,c)       (p)->lpVtbl->SetDefaultStore(p,a,b,c)
#define IMAPISession_AdminServices(p,a,b)           (p)->lpVtbl->AdminServices(p,a,b)
#define IMAPISession_ShowForm(p,a,b,c,d,e,f,g,h,i,j,k) \
                                                    (p)->lpVtbl->ShowForm(p,a,b,c,d,e,f,g,h,i,j,k)
#define IMAPISession_PrepareForm(p,a,b,c)           (p)->lpVtbl->PrepareForm(p,a,b,c)
#endif

#ifdef __cplusplus
}
#endif

#endif /* MAPIX_H */
