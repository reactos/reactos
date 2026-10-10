/*
 *             MAPI utility header file
 *
 * Copyright 2009 Owen Rudge for CodeWeavers
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

#ifndef _MAPI_UTIL_H

#define _MAPI_UTIL_H

#include <mapi.h>
#include <mapix.h>

extern void load_mapi_providers(void);
extern void unload_mapi_providers(void);

typedef struct MAPI_FUNCTIONS {
    LPMAPIADDRESS        MAPIAddress;
    LPMAPIDELETEMAIL     MAPIDeleteMail;
    LPMAPIDETAILS        MAPIDetails;
    LPMAPIFINDNEXT       MAPIFindNext;
    LPMAPIINITIALIZE     MAPIInitialize;
    LPMAPILOGOFF         MAPILogoff;
    LPMAPILOGON          MAPILogon;
    LPMAPILOGONEX        MAPILogonEx;
    LPMAPIREADMAIL       MAPIReadMail;
    LPMAPIRESOLVENAME    MAPIResolveName;
    LPMAPISAVEMAIL       MAPISaveMail;
    LPMAPISENDMAIL       MAPISendMail;
    LPMAPISENDMAILW      MAPISendMailW;
    LPMAPISENDDOCUMENTS  MAPISendDocuments;
    LPMAPIUNINITIALIZE   MAPIUninitialize;

    VOID     (WINAPI *DeinitMapiUtil)             (void);
    HRESULT  (WINAPI *DllCanUnloadNow)            (void);
    HRESULT  (WINAPI *DllGetClassObject)          (REFCLSID, REFIID, LPVOID *);
    BOOL     (WINAPI *FGetComponentPath)          (LPCSTR, LPCSTR, LPSTR, DWORD, BOOL);
    HRESULT  (WINAPI *MAPIAdminProfiles)          (ULONG, LPPROFADMIN *);
    SCODE    (WINAPI *MAPIAllocateBuffer)         (ULONG, LPVOID *);
    SCODE    (WINAPI *MAPIAllocateMore)           (ULONG, LPVOID, LPVOID *);
    ULONG    (WINAPI *MAPIFreeBuffer)             (LPVOID);
    LPMALLOC (WINAPI *MAPIGetDefaultMalloc)       (void);
    HRESULT  (WINAPI *MAPIOpenLocalFormContainer) (LPVOID *);
    HRESULT  (WINAPI *HrThisThreadAdviseSink)     (LPMAPIADVISESINK, LPMAPIADVISESINK*);
    HRESULT  (WINAPI *HrQueryAllRows)             (LPMAPITABLE, LPSPropTagArray, LPSRestriction, LPSSortOrderSet, LONG, LPSRowSet *);
    HRESULT  (WINAPI *OpenStreamOnFile)           (LPALLOCATEBUFFER, LPFREEBUFFER, ULONG, LPWSTR, LPWSTR, LPSTREAM *);
    SCODE    (WINAPI *ScInitMapiUtil)             (ULONG ulReserved);
    HRESULT  (WINAPI *WrapCompressedRTFStream)    (LPSTREAM, ULONG, LPSTREAM *);
} MAPI_FUNCTIONS;

extern MAPI_FUNCTIONS mapiFunctions;
extern HINSTANCE hInstMAPI32;

#endif
