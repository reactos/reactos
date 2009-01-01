/*
 * Copyright (C) 2004 Francois Gouget
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

#ifndef __WINE_MSCAT_H
#define __WINE_MSCAT_H

#include <mssip.h>

typedef HANDLE HCATADMIN;
typedef HANDLE HCATINFO;

#ifdef __cplusplus
extern "C" {
#endif


#define CRYPTCAT_OPEN_CREATENEW  1
#define CRYPTCAT_OPEN_ALWAYS     2
#define CRYPTCAT_OPEN_EXISTING   4



#include <pshpack8.h>

typedef struct CRYPTCATMEMBER_ {
    DWORD cbStruct;
    LPWSTR pwszReferenceTag;
    LPWSTR pwszFileName;
    GUID gSubjectType;
    DWORD fdwMemberFlags;
    struct SIP_INDIRECT_DATA_* pIndirectData;
    DWORD dwCertVersion;
    DWORD dwReserved;
    HANDLE hReserved;
    CRYPT_ATTR_BLOB sEncodedIndirectData;
    CRYPT_ATTR_BLOB sEncodedMemberInfo;
} CRYPTCATMEMBER;

typedef struct CRYPTCATATTRIBUTE_
{
    DWORD cbStruct;
    LPWSTR pwszReferenceTag;
    DWORD dwAttrTypeAndAction;
    DWORD cbValue;
    BYTE *pbValue;
    DWORD dwReserved;
}CRYPTCATATTRIBUTE;

typedef struct CATALOG_INFO_ 
{
    DWORD cbStruct;
    WCHAR wszCatalogFile[MAX_PATH];
} CATALOG_INFO;


#include <poppack.h>

BOOL      WINAPI CryptCATAdminAcquireContext(HCATADMIN*,const GUID*,DWORD);
HCATINFO  WINAPI CryptCATAdminAddCatalog(HCATADMIN,PWSTR,PWSTR,DWORD);
BOOL      WINAPI CryptCATAdminCalcHashFromFileHandle(HANDLE,DWORD*,BYTE*,DWORD);
HCATINFO  WINAPI CryptCATAdminEnumCatalogFromHash(HCATADMIN,BYTE*,DWORD,DWORD,HCATINFO*);
BOOL      WINAPI CryptCATAdminReleaseCatalogContext(HCATADMIN,HCATINFO,DWORD);
BOOL      WINAPI CryptCATAdminReleaseContext(HCATADMIN,DWORD);
BOOL      WINAPI CryptCATAdminRemoveCatalog(HCATADMIN,LPCWSTR,DWORD);
BOOL      WINAPI CryptCATClose(HANDLE);
BOOL      WINAPI CryptCATCatalogInfoFromContext(HCATINFO,CATALOG_INFO*,DWORD);

CRYPTCATMEMBER* WINAPI CryptCATEnumerateMember(HANDLE,CRYPTCATMEMBER*);
HANDLE    WINAPI CryptCATOpen(LPWSTR,DWORD,HCRYPTPROV,DWORD,DWORD);
CRYPTCATATTRIBUTE* WINAPI CryptCATEnumerateAttr(HANDLE hCatalog, CRYPTCATMEMBER*,CRYPTCATATTRIBUTE*);
CRYPTCATATTRIBUTE* WINAPI CryptCATEnumerateCatAttr(HANDLE,CRYPTCATATTRIBUTE*);

#ifdef __cplusplus
}
#endif

#endif
