/*
 * Status codes returned by MAPI
 *
 * Copyright (C) 2002 Aric Stewart
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

#ifndef MAPICODE_H
#define MAPICODE_H

#include <winerror.h>

#define MAKE_MAPI_SCODE(sev,fac,code) \
    ((SCODE)(((ULONG)(sev)<<31)|((ULONG)(fac)<<16)|((ULONG)(code))))

#define MAKE_MAPI_E(e) (MAKE_MAPI_SCODE(1,FACILITY_ITF,(e)))
#define MAKE_MAPI_S(e) (MAKE_MAPI_SCODE(0,FACILITY_ITF,(e)))

#ifndef SUCCESS_SUCCESS
#define SUCCESS_SUCCESS     0L
#endif

/* Errors */
#define MAPI_E_ACCOUNT_DISABLED            ((SCODE)0x80040124)
#define MAPI_E_AMBIGUOUS_RECIP             ((SCODE)0x80040700)
#define MAPI_E_BAD_CHARWIDTH               ((SCODE)0x80040103)
#define MAPI_E_BAD_COLUMN                  ((SCODE)0x80040118)
#define MAPI_E_BAD_VALUE                   ((SCODE)0x80040301)
#define MAPI_E_BUSY                        ((SCODE)0x8004010B)
#define MAPI_E_CALL_FAILED                 E_FAIL
#define MAPI_E_CANCEL                      ((SCODE)0x80040501)
#define MAPI_E_COLLISION                   ((SCODE)0x80040604)
#define MAPI_E_COMPUTED                    ((SCODE)0x8004011A)
#define MAPI_E_CORRUPT_DATA                ((SCODE)0x8004011B)
#define MAPI_E_CORRUPT_STORE               ((SCODE)0x80040600)
#define MAPI_E_DECLINE_COPY                ((SCODE)0x80040306)
#define MAPI_E_DISK_ERROR                  ((SCODE)0x80040116)
#define MAPI_E_END_OF_SESSION              ((SCODE)0x80040200)
#define MAPI_E_EXTENDED_ERROR              ((SCODE)0x80040119)
#define MAPI_E_FAILONEPROVIDER             ((SCODE)0x8004011D)
#define MAPI_E_FOLDER_CYCLE                ((SCODE)0x8004060B)
#define MAPI_E_HAS_FOLDERS                 ((SCODE)0x80040609)
#define MAPI_E_HAS_MESSAGES                ((SCODE)0x8004060A)
#define MAPI_E_INTERFACE_NOT_SUPPORTED     E_NOINTERFACE
#define MAPI_E_INVALID_ACCESS_TIME         ((SCODE)0x80040123)
#define MAPI_E_INVALID_BOOKMARK            ((SCODE)0x80040405)
#define MAPI_E_INVALID_ENTRYID             ((SCODE)0x80040107)
#define MAPI_E_INVALID_OBJECT              ((SCODE)0x80040108)
#define MAPI_E_INVALID_PARAMETER           E_INVALIDARG
#define MAPI_E_INVALID_TYPE                ((SCODE)0x80040302)
#define MAPI_E_INVALID_WORKSTATION_ACCOUNT ((SCODE)0x80040122)
#define MAPI_E_LOGON_FAILED                ((SCODE)0x80040111)
#define MAPI_E_MISSING_REQUIRED_COLUMN     ((SCODE)0x80040202)
#define MAPI_E_NETWORK_ERROR               ((SCODE)0x80040115)
#define MAPI_E_NO_ACCESS                   E_ACCESSDENIED
#define MAPI_E_NON_STANDARD                ((SCODE)0x80040606)
#define MAPI_E_NO_RECIPIENTS               ((SCODE)0x80040607)
#define MAPI_E_NO_SUPPORT                  ((SCODE)0x80040102)
#define MAPI_E_NO_SUPPRESS                 ((SCODE)0x80040602)
#define MAPI_E_NOT_ENOUGH_DISK             ((SCODE)0x8004010D)
#define MAPI_E_NOT_ENOUGH_MEMORY           E_OUTOFMEMORY
#define MAPI_E_NOT_ENOUGH_RESOURCES        ((SCODE)0x8004010E)
#define MAPI_E_NOT_FOUND                   ((SCODE)0x8004010F)
#define MAPI_E_NOT_INITIALIZED             ((SCODE)0x80040605)
#define MAPI_E_NOT_IN_QUEUE                ((SCODE)0x80040601)
#define MAPI_E_NOT_ME                      ((SCODE)0x80040502)
#define MAPI_E_OBJECT_CHANGED              ((SCODE)0x80040109)
#define MAPI_E_OBJECT_DELETED              ((SCODE)0x8004010A)
#define MAPI_E_PASSWORD_CHANGE_REQUIRED    ((SCODE)0x80040120)
#define MAPI_E_PASSWORD_EXPIRED            ((SCODE)0x80040121)
#define MAPI_E_SESSION_LIMIT               ((SCODE)0x80040112)
#define MAPI_E_STRING_TOO_LONG             ((SCODE)0x80040105)
#define MAPI_E_SUBMITTED                   ((SCODE)0x80040608)
#define MAPI_E_TABLE_EMPTY                 ((SCODE)0x80040402)
#define MAPI_E_TABLE_TOO_BIG               ((SCODE)0x80040403)
#define MAPI_E_TIMEOUT                     ((SCODE)0x80040401)
#define MAPI_E_TOO_BIG                     ((SCODE)0x80040305)
#define MAPI_E_TOO_COMPLEX                 ((SCODE)0x80040117)
#define MAPI_E_TYPE_NO_SUPPORT             ((SCODE)0x80040303)
#define MAPI_E_UNABLE_TO_ABORT             ((SCODE)0x80040114)
#define MAPI_E_UNABLE_TO_COMPLETE          ((SCODE)0x80040400)
#define MAPI_E_UNCONFIGURED                ((SCODE)0x8004011C)
#define MAPI_E_UNEXPECTED_ID               ((SCODE)0x80040307)
#define MAPI_E_UNEXPECTED_TYPE             ((SCODE)0x80040304)
#define MAPI_E_UNKNOWN_CPID                ((SCODE)0x8004011E)
#define MAPI_E_UNKNOWN_ENTRYID             ((SCODE)0x80040201)
#define MAPI_E_UNKNOWN_FLAGS               ((SCODE)0x80040106)
#define MAPI_E_UNKNOWN_LCID                ((SCODE)0x8004011F)
#define MAPI_E_USER_CANCEL                 ((SCODE)0x80040113)
#define MAPI_E_VERSION                     ((SCODE)0x80040110)
#define MAPI_E_WAIT                        ((SCODE)0x80040500)

/* Warnings */
#define MAPI_W_APPROX_COUNT                ((SCODE)0x00040482)
#define MAPI_W_CANCEL_MESSAGE              ((SCODE)0x00040580)
#define MAPI_W_ERRORS_RETURNED             ((SCODE)0x00040380)
#define MAPI_W_NO_SERVICE                  ((SCODE)0x00040203)
#define MAPI_W_PARTIAL_COMPLETION          ((SCODE)0x00040680)
#define MAPI_W_POSITION_CHANGED            ((SCODE)0x00040481)

#endif /* MAPICODE_H */
