/*
 * winsafer.h
 *
 * This file is part of the ReactOS PSDK package.
 *
 * Contributors:
 *   Thomas Faber (thomas.faber@reactos.org)
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#pragma once

#ifndef _WINSAFER_H
#define _WINSAFER_H

#include <guiddef.h>
#include <wincrypt.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

DECLARE_HANDLE(SAFER_LEVEL_HANDLE);

#define SAFER_SCOPEID_MACHINE 1
#define SAFER_SCOPEID_USER    2

#define SAFER_LEVELID_DISALLOWED   0x00000
#define SAFER_LEVELID_UNTRUSTED    0x01000
#define SAFER_LEVELID_CONSTRAINED  0x10000
#define SAFER_LEVELID_NORMALUSER   0x20000
#define SAFER_LEVELID_FULLYTRUSTED 0x40000

#define SAFER_LEVEL_OPEN 1

#define SAFER_MAX_HASH_SIZE          64
#define SAFER_MAX_DESCRIPTION_SIZE  256
#define SAFER_MAX_FRIENDLYNAME_SIZE 256

#define SAFER_TOKEN_NULL_IF_EQUAL 0x1
#define SAFER_TOKEN_COMPARE_ONLY  0x2
#define SAFER_TOKEN_MAKE_INERT    0x4
#define SAFER_TOKEN_WANT_FLAGS    0x8

#define SAFER_CRITERIA_IMAGEPATH    0x0001
#define SAFER_CRITERIA_NOSIGNEDHASH 0x0002
#define SAFER_CRITERIA_IMAGEHASH    0x0004
#define SAFER_CRITERIA_AUTHENTICODE 0x0008
#define SAFER_CRITERIA_URLZONE      0x0010
#define SAFER_CRITERIA_APPX_PACKAGE 0x0020
#define SAFER_CRITERIA_IMAGEPATH_NT 0x1000

#define SAFER_POLICY_JOBID_UNTRUSTED            0x03000000
#define SAFER_POLICY_JOBID_CONSTRAINED          0x04000000
#define SAFER_POLICY_JOBID_MASK                 0xFF000000
#define SAFER_POLICY_ONLY_EXES                  0x00010000
#define SAFER_POLICY_SANDBOX_INERT              0x00020000
#define SAFER_POLICY_HASH_DUPLICATE             0x00040000
#define SAFER_POLICY_ONLY_AUDIT                 0x00001000
#define SAFER_POLICY_BLOCK_CLIENT_UI            0x00002000
#define SAFER_POLICY_UIFLAGS_INFORMATION_PROMPT 0x00000001
#define SAFER_POLICY_UIFLAGS_OPTION_PROMPT      0x00000002
#define SAFER_POLICY_UIFLAGS_HIDDEN             0x00000004
#define SAFER_POLICY_UIFLAGS_MASK               0x000000FF


#include <pshpack8.h>

typedef struct _SAFER_CODE_PROPERTIES_V1
{
    DWORD cbSize;
    DWORD dwCheckFlags;
    PCWSTR ImagePath;
    HANDLE hImageFileHandle;
    DWORD UrlZoneId;
    BYTE ImageHash[SAFER_MAX_HASH_SIZE];
    DWORD dwImageHashSize;
    LARGE_INTEGER ImageSize;
    ALG_ID HashAlgorithm;
    PBYTE pByteBlock;
    HWND hWndParent;
    DWORD dwWVTUIChoice;
} SAFER_CODE_PROPERTIES_V1, *PSAFER_CODE_PROPERTIES_V1;

typedef struct _SAFER_CODE_PROPERTIES_V2
{
    SAFER_CODE_PROPERTIES_V1;
    PCWSTR PackageMoniker;
    PCWSTR PackagePublisher;
    PCWSTR PackageName;
    ULONG64 PackageVersion;
    BOOL PackageIsFramework;
} SAFER_CODE_PROPERTIES_V2, *PSAFER_CODE_PROPERTIES_V2;

#include <poppack.h>

/* NOTE: MS defines SAFER_CODE_PROPERTIES as V2 unconditionally,
 * which is... not smart */
#if _WIN32_WINNT >= 0x602
typedef SAFER_CODE_PROPERTIES_V2 SAFER_CODE_PROPERTIES, *PSAFER_CODE_PROPERTIES;
#else /* _WIN32_WINNT */
typedef SAFER_CODE_PROPERTIES_V1 SAFER_CODE_PROPERTIES, *PSAFER_CODE_PROPERTIES;
#endif /* _WIN32_WINNT */

typedef enum _SAFER_OBJECT_INFO_CLASS
{
    SaferObjectLevelId = 1,
    SaferObjectScopeId = 2,
    SaferObjectFriendlyName = 3,
    SaferObjectDescription = 4,
    SaferObjectBuiltin = 5,
    SaferObjectDisallowed = 6,
    SaferObjectDisableMaxPrivilege = 7,
    SaferObjectInvertDeletedPrivileges = 8,
    SaferObjectDeletedPrivileges = 9,
    SaferObjectDefaultOwner = 10,
    SaferObjectSidsToDisable = 11,
    SaferObjectRestrictedSidsInverted = 12,
    SaferObjectRestrictedSidsAdded = 13,
    SaferObjectAllIdentificationGuids = 14,
    SaferObjectSingleIdentification = 15,
    SaferObjectExtendedError = 16,
} SAFER_OBJECT_INFO_CLASS;

typedef enum _SAFER_POLICY_INFO_CLASS
{
    SaferPolicyLevelList = 1,
    SaferPolicyEnableTransparentEnforcement = 2,
    SaferPolicyDefaultLevel = 3,
    SaferPolicyEvaluateUserScope = 4,
    SaferPolicyScopeFlags = 5,
    SaferPolicyDefaultLevelFlags = 6,
    SaferPolicyAuthenticodeEnabled = 7,
} SAFER_POLICY_INFO_CLASS;

typedef enum _SAFER_IDENTIFICATION_TYPES
{
    SaferIdentityDefault = 0,
    SaferIdentityTypeImageName = 1,
    SaferIdentityTypeImageHash = 2,
    SaferIdentityTypeUrlZone = 3,
    SaferIdentityTypeCertificate = 4,
} SAFER_IDENTIFICATION_TYPES;

#include <pshpack8.h>

typedef struct _SAFER_IDENTIFICATION_HEADER
{
    SAFER_IDENTIFICATION_TYPES dwIdentificationType;
    DWORD cbStructSize;
    GUID IdentificationGuid;
    FILETIME lastModified;
} SAFER_IDENTIFICATION_HEADER, *PSAFER_IDENTIFICATION_HEADER;

typedef struct _SAFER_PATHNAME_IDENTIFICATION
{
    SAFER_IDENTIFICATION_HEADER header;
    WCHAR Description[SAFER_MAX_DESCRIPTION_SIZE];
    PWCHAR ImageName;
    DWORD dwSaferFlags;
} SAFER_PATHNAME_IDENTIFICATION, *PSAFER_PATHNAME_IDENTIFICATION;

typedef struct _SAFER_HASH_IDENTIFICATION
{
    SAFER_IDENTIFICATION_HEADER header;
    WCHAR Description[SAFER_MAX_DESCRIPTION_SIZE];
    WCHAR FriendlyName[SAFER_MAX_FRIENDLYNAME_SIZE];
    DWORD HashSize;
    BYTE ImageHash[SAFER_MAX_HASH_SIZE];
    ALG_ID HashAlgorithm;
    LARGE_INTEGER ImageSize;
    DWORD dwSaferFlags;
} SAFER_HASH_IDENTIFICATION, *PSAFER_HASH_IDENTIFICATION;

typedef struct _SAFER_HASH_IDENTIFICATION2
{
    SAFER_HASH_IDENTIFICATION hashIdentification;
    DWORD HashSize;
    BYTE ImageHash[SAFER_MAX_HASH_SIZE];
    ALG_ID HashAlgorithm;
} SAFER_HASH_IDENTIFICATION2, *PSAFER_HASH_IDENTIFICATION2;

typedef struct _SAFER_URLZONE_IDENTIFICATION
{
    SAFER_IDENTIFICATION_HEADER header;
    DWORD UrlZoneId;
    DWORD dwSaferFlags;
} SAFER_URLZONE_IDENTIFICATION, *PSAFER_URLZONE_IDENTIFICATION;

#include <poppack.h>


WINADVAPI
BOOL
WINAPI
SaferCloseLevel(
    _In_ SAFER_LEVEL_HANDLE hLevelHandle);

WINADVAPI
BOOL
WINAPI
SaferComputeTokenFromLevel(
    _In_ SAFER_LEVEL_HANDLE LevelHandle,
    _In_opt_ HANDLE InAccessToken,
    _Out_ PHANDLE OutAccessToken,
    _In_ DWORD dwFlags,
    _Inout_opt_ PVOID pReserved);

WINADVAPI
BOOL
WINAPI
SaferCreateLevel(
    _In_ DWORD dwScopeId,
    _In_ DWORD dwLevelId,
    _In_ DWORD OpenFlags,
    _Outptr_ SAFER_LEVEL_HANDLE *pLevelHandle,
    _Reserved_ PVOID pReserved);

WINADVAPI
BOOL
WINAPI
SaferGetLevelInformation(
    _In_ SAFER_LEVEL_HANDLE LevelHandle,
    _In_ SAFER_OBJECT_INFO_CLASS dwInfoType,
    _Out_writes_bytes_opt_(dwInBufferSize) PVOID pQueryBuffer,
    _In_ DWORD dwInBufferSize,
    _Out_ PDWORD pdwOutBufferSize);

WINADVAPI
BOOL
WINAPI
SaferGetPolicyInformation(
    _In_ DWORD dwScopeId,
    _In_ SAFER_POLICY_INFO_CLASS SaferPolicyInfoClass,
    _In_ DWORD InfoBufferSize,
    _Out_writes_bytes_opt_(InfoBufferSize) PVOID InfoBuffer,
    _Out_ PDWORD InfoBufferRetSize,
    _Reserved_ PVOID pReserved);

WINADVAPI
BOOL
WINAPI
SaferIdentifyLevel(
    _In_ DWORD dwNumProperties,
    _In_reads_opt_(dwNumProperties) PSAFER_CODE_PROPERTIES pCodeProperties,
    _Outptr_ SAFER_LEVEL_HANDLE *pLevelHandle,
    _Reserved_ PVOID pReserved);

WINADVAPI
BOOL
WINAPI
SaferiIsExecutableFileType(
    _In_ PCWSTR szFullPath,
    _In_ BOOLEAN bFromShellExecute);

WINADVAPI
BOOL
WINAPI
SaferRecordEventLogEntry(
    _In_ SAFER_LEVEL_HANDLE hLevel,
    _In_ PCWSTR szTargetPath,
    _Reserved_ PVOID pReserved);

WINADVAPI
BOOL
WINAPI
SaferSetLevelInformation(
    _In_ SAFER_LEVEL_HANDLE LevelHandle,
    _In_ SAFER_OBJECT_INFO_CLASS dwInfoType,
    _In_reads_bytes_(dwInBufferSize) PVOID pQueryBuffer,
    _In_ DWORD dwInBufferSize);

WINADVAPI
BOOL
WINAPI
SaferSetPolicyInformation(
    _In_ DWORD dwScopeId,
    _In_ SAFER_POLICY_INFO_CLASS SaferPolicyInfoClass,
    _In_ DWORD InfoBufferSize,
    _In_reads_bytes_(InfoBufferSize) PVOID InfoBuffer,
    _Reserved_ PVOID pReserved);


#define SRP_POLICY_EXE        L"EXE"
#define SRP_POLICY_DLL        L"DLL"
#define SRP_POLICY_MSI        L"MSI"
#define SRP_POLICY_SCRIPT     L"SCRIPT"
#define SRP_POLICY_SHELL      L"SHELL"
#define SRP_POLICY_NOV2       L"IGNORESRPV2"
#define SRP_POLICY_APPX       L"APPX"
#define SRP_POLICY_WLDPMSI    L"WLDPMSI"
#define SRP_POLICY_WLDPSCRIPT L"WLDPSCRIPT"

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* _WINSAFER_H */
