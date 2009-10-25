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
    LPMAPISENDDOCUMENTS  MAPISendDocuments;
    LPMAPIUNINITIALIZE   MAPIUninitialize;

    HRESULT (WINAPI *DllGetClassObject)(REFCLSID, REFIID, LPVOID *);
} MAPI_FUNCTIONS;

extern MAPI_FUNCTIONS mapiFunctions;

#endif
