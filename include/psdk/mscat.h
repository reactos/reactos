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

#include <poppack.h>


BOOL      WINAPI CryptCATAdminAcquireContext(HCATADMIN*,const GUID*,DWORD);
BOOL      WINAPI CryptCATAdminCalcHashFromFileHandle(HANDLE,DWORD*,BYTE*,DWORD);
HCATINFO  WINAPI CryptCATAdminEnumCatalogFromHash(HCATADMIN,BYTE*,DWORD,DWORD,HCATINFO*);
BOOL      WINAPI CryptCATAdminReleaseContext(HCATADMIN,DWORD);
BOOL      WINAPI CryptCATClose(HANDLE);
CRYPTCATMEMBER* WINAPI CryptCATEnumerateMember(HANDLE,CRYPTCATMEMBER*);
HANDLE    WINAPI CryptCATOpen(LPWSTR,DWORD,HCRYPTPROV,DWORD,DWORD);

#ifdef __cplusplus
}
#endif

#endif
