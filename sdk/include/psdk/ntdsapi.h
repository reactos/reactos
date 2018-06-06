/*
 * Copyright (C) 2006 Dmitry Timoshkov
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

#ifndef __WINE_NTDSAPI_H
#define __WINE_NTDSAPI_H

/* FIXME: #include <schedule.h> */

#ifdef __cplusplus
extern "C" {
#endif

DWORD WINAPI DsClientMakeSpnForTargetServerA(LPCSTR, LPCSTR, DWORD*, LPSTR);
DWORD WINAPI DsClientMakeSpnForTargetServerW(LPCWSTR, LPCWSTR, DWORD*, LPWSTR);
#define DsClientMakeSpnForTargetServer WINELIB_NAME_AW(DsClientMakeSpnForTargetServer)

DWORD WINAPI DsMakeSpnA(LPCSTR, LPCSTR, LPCSTR, USHORT, LPCSTR, DWORD*, LPSTR);
DWORD WINAPI DsMakeSpnW(LPCWSTR, LPCWSTR, LPCWSTR, USHORT, LPCWSTR, DWORD*, LPWSTR);
#define DsMakeSpn WINELIB_NAME_AW(DsMakeSpn)

typedef enum
{
    DS_NAME_NO_FLAGS              = 0x0,
    DS_NAME_FLAG_SYNTACTICAL_ONLY = 0x1,
    DS_NAME_FLAG_EVAL_AT_DC       = 0x2,
    DS_NAME_FLAG_GCVERIFY         = 0x4,
    DS_NAME_FLAG_TRUST_REFERRAL   = 0x8
} DS_NAME_FLAGS;

typedef enum
{
    DS_UNKNOWN_NAME            = 0,
    DS_FQDN_1779_NAME          = 1,
    DS_NT4_ACCOUNT_NAME        = 2,
    DS_DISPLAY_NAME            = 3,
    DS_UNIQUE_ID_NAME          = 6,
    DS_CANONICAL_NAME          = 7,
    DS_USER_PRINCIPAL_NAME     = 8,
    DS_CANONICAL_NAME_EX       = 9,
    DS_SERVICE_PRINCIPAL_NAME  = 10,
    DS_SID_OR_SID_HISTORY_NAME = 11,
    DS_DNS_DOMAIN_NAME         = 12
} DS_NAME_FORMAT;

typedef enum
{
    DS_SPN_DNS_HOST    = 0,
    DS_SPN_DN_HOST     = 1,
    DS_SPN_NB_HOST     = 2,
    DS_SPN_DOMAIN      = 3,
    DS_SPN_NB_DOMAIN   = 4,
    DS_SPN_SERVICE     = 5
} DS_SPN_NAME_TYPE;

typedef enum
{
    DS_SPN_ADD_SPN_OP     = 0,
    DS_SPN_REPLACE_SPN_OP = 1,
    DS_SPN_DELETE_SPN_OP  = 2
} DS_SPN_WRITE_OP;

typedef struct
{
    DWORD status;
    LPSTR pDomain;
    LPSTR pName;
} DS_NAME_RESULT_ITEMA, *PDS_NAME_RESULT_ITEMA;

typedef struct
{
    DWORD  status;
    LPWSTR pDomain;
    LPWSTR pName;
} DS_NAME_RESULT_ITEMW, *PDS_NAME_RESULT_ITEMW;

DECL_WINELIB_TYPE_AW(DS_NAME_RESULT_ITEM)
DECL_WINELIB_TYPE_AW(PDS_NAME_RESULT_ITEM)

typedef struct
{
    DWORD                 cItems;
    PDS_NAME_RESULT_ITEMA rItems;
} DS_NAME_RESULTA, *PDS_NAME_RESULTA;

typedef struct
{
    DWORD                 cItems;
    PDS_NAME_RESULT_ITEMW rItems;
} DS_NAME_RESULTW, *PDS_NAME_RESULTW;

DECL_WINELIB_TYPE_AW(DS_NAME_RESULT)
DECL_WINELIB_TYPE_AW(PDS_NAME_RESULT)

DWORD WINAPI DsCrackNamesA(HANDLE handle, DS_NAME_FLAGS flags, DS_NAME_FORMAT offered, DS_NAME_FORMAT desired, DWORD num, const CHAR **names, PDS_NAME_RESULTA *result);
DWORD WINAPI DsCrackNamesW(HANDLE handle, DS_NAME_FLAGS flags, DS_NAME_FORMAT offered, DS_NAME_FORMAT desired, DWORD num, const WCHAR **names, PDS_NAME_RESULTW *result);
#define DsCrackNames WINELIB_NAME_AW(DsCrackNames)
DWORD WINAPI DsServerRegisterSpnA(DS_SPN_WRITE_OP operation, LPCSTR ServiceClass, LPCSTR UserObjectDN);
DWORD WINAPI DsServerRegisterSpnW(DS_SPN_WRITE_OP operation, LPCWSTR ServiceClass, LPCWSTR UserObjectDN);
#define DsServerRegisterSpn WINELIB_NAME_AW(DsServerRegisterSpn)

#ifdef __cplusplus
}
#endif

#endif /* __WINE_NTDSAPI_H */
