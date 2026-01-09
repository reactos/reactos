/*
 * Copyright (C) 2002 Patrik Stridvall
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

#ifndef __WINE_MSSIP_H
#define __WINE_MSSIP_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/**********************************************************************/

typedef CRYPT_HASH_BLOB CRYPT_DIGEST_DATA;

/**********************************************************************/

#define MSSIP_FLAGS_PROHIBIT_RESIZE_ON_CREATE 0x00010000
#define MSSIP_FLAGS_USE_CATALOG               0x00020000

#define SPC_EXC_PE_PAGE_HASHES_FLAG         0x010
#define SPC_INC_PE_IMPORT_ADDR_TABLE_FLAG   0x020
#define SPC_INC_PE_DEBUG_INFO_FLAG          0x040
#define SPC_INC_PE_RESOURCES_FLAG           0x080
#define SPC_INC_PE_PAGE_HASHES_FLAG         0x100

#define MSSIP_ADDINFO_NONE      0
#define MSSIP_ADDINFO_FLAT      1
#define MSSIP_ADDINFO_CATMEMBER 2
#define MSSIP_ADDINFO_BLOB      3
#define MSSIP_ADDINFO_NONMSSIP  500

#define SIP_MAX_MAGIC_NUMBER 4

/**********************************************************************/

#include <pshpack8.h>
typedef struct SIP_SUBJECTINFO_ {
    DWORD cbSize;
    GUID *pgSubjectType;
    HANDLE hFile;
    LPCWSTR pwsFileName;
    LPCWSTR pwsDisplayName;

    DWORD dwReserved1;
    DWORD dwIntVersion;

    HCRYPTPROV hProv;
    CRYPT_ALGORITHM_IDENTIFIER DigestAlgorithm;
    DWORD dwFlags;
    DWORD dwEncodingType;
    DWORD dwReserved2;
    DWORD fdwCAPISettings;
    DWORD fdwSecuritySettings;
    DWORD dwIndex;

    DWORD dwUnionChoice;
    union {
      struct MS_ADDINFO_FLAT_          *psFlat;
      struct MS_ADDINFO_CATALOGMEMBER_ *psCatMember;
      struct MS_ADDINFO_BLOB_          *psBlob;
    } DUMMYUNIONNAME;

    LPVOID pClientData;
} SIP_SUBJECTINFO, *LPSIP_SUBJECTINFO;
#include <poppack.h>

#include <pshpack8.h>
typedef struct MS_ADDINFO_FLAT_ {
  DWORD cbStruct;

  struct SIP_INDIRECT_DATA_ *pIndirectData;
} MS_ADDINFO_FLAT, *PMS_ADDINFO_FLAT;
#include <poppack.h>

#include <pshpack8.h>
typedef struct MS_ADDINFO_CATALOGMEMBER_ {
  DWORD cbStruct;

  struct CRYPTCATSTORE_  *pStore;
  struct CRYPTCATMEMBER_ *pMember;
} MS_ADDINFO_CATALOGMEMBER, *PMS_ADDINFO_CATALOGMEMBER;
#include <poppack.h>

#include <pshpack8.h>
typedef struct MS_ADDINFO_BLOB_ {
  DWORD cbStruct;

  DWORD cbMemObject;
  BYTE *pbMemObject;

  DWORD cbMemSignedMsg;
  BYTE *pbMemSignedMsg;
} MS_ADDINFO_BLOB, *PMS_ADDINFO_BLOB;
#include <poppack.h>

#include <pshpack8.h>
typedef struct SIP_INDIRECT_DATA_ {
  CRYPT_ATTRIBUTE_TYPE_VALUE Data;
  CRYPT_ALGORITHM_IDENTIFIER DigestAlgorithm;
  CRYPT_HASH_BLOB            Digest;
} SIP_INDIRECT_DATA, *PSIP_INDIRECT_DATA;
#include <poppack.h>

typedef BOOL (WINAPI * pCryptSIPGetSignedDataMsg)(SIP_SUBJECTINFO *,DWORD *,DWORD,DWORD *,BYTE *);
typedef BOOL (WINAPI * pCryptSIPPutSignedDataMsg)(SIP_SUBJECTINFO *,DWORD,DWORD *,DWORD,BYTE *);
typedef BOOL (WINAPI * pCryptSIPCreateIndirectData)(SIP_SUBJECTINFO *,DWORD *,SIP_INDIRECT_DATA *);
typedef BOOL (WINAPI * pCryptSIPVerifyIndirectData)(SIP_SUBJECTINFO *,SIP_INDIRECT_DATA *);
typedef BOOL (WINAPI * pCryptSIPRemoveSignedDataMsg)(SIP_SUBJECTINFO *,DWORD);

#include <pshpack8.h>
typedef struct SIP_DISPATCH_INFO_ {
  DWORD cbSize;

  HANDLE hSIP;

  pCryptSIPGetSignedDataMsg    pfGet;
  pCryptSIPPutSignedDataMsg    pfPut;
  pCryptSIPCreateIndirectData  pfCreate;
  pCryptSIPVerifyIndirectData  pfVerify;
  pCryptSIPRemoveSignedDataMsg pfRemove;
} SIP_DISPATCH_INFO, *LPSIP_DISPATCH_INFO;
#include <poppack.h>

typedef BOOL (WINAPI *pfnIsFileSupported)(HANDLE,GUID *);
typedef BOOL (WINAPI *pfnIsFileSupportedName)(WCHAR *,GUID *);

#include <pshpack8.h>
typedef struct SIP_ADD_NEWPROVIDER_
{
  DWORD cbStruct;

  GUID  *pgSubject;

  WCHAR *pwszDLLFileName;
  WCHAR *pwszMagicNumber;

  WCHAR *pwszIsFunctionName;

  WCHAR *pwszGetFuncName;
  WCHAR *pwszPutFuncName;
  WCHAR *pwszCreateFuncName;
  WCHAR *pwszVerifyFuncName;
  WCHAR *pwszRemoveFuncName;

  WCHAR *pwszIsFunctionNameFmt2;

  WCHAR *pwszGetCapFuncName;
} SIP_ADD_NEWPROVIDER, *PSIP_ADD_NEWPROVIDER;
#include <poppack.h>

/**********************************************************************/

BOOL WINAPI CryptSIPGetSignedDataMsg(SIP_SUBJECTINFO *,DWORD *,DWORD,DWORD *,BYTE *);
BOOL WINAPI CryptSIPPutSignedDataMsg(SIP_SUBJECTINFO *,DWORD,DWORD *,DWORD,BYTE *);
BOOL WINAPI CryptSIPCreateIndirectData(SIP_SUBJECTINFO *,DWORD *,SIP_INDIRECT_DATA *);
BOOL WINAPI CryptSIPVerifyIndirectData(SIP_SUBJECTINFO *,SIP_INDIRECT_DATA *);
BOOL WINAPI CryptSIPRemoveSignedDataMsg(SIP_SUBJECTINFO *,DWORD);

BOOL WINAPI CryptSIPLoad(const GUID *,DWORD,SIP_DISPATCH_INFO *);
BOOL WINAPI CryptSIPRetrieveSubjectGuid(LPCWSTR,HANDLE,GUID *);
BOOL WINAPI CryptSIPRetrieveSubjectGuidForCatalogFile(LPCWSTR,HANDLE,GUID *);
BOOL WINAPI CryptSIPAddProvider(SIP_ADD_NEWPROVIDER *);
BOOL WINAPI CryptSIPRemoveProvider(GUID *);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif  /* __WINE_MSSIP_H */
