/*
 * PROJECT:         ReactOS Drivers
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         Kernel Security Support Provider Interface Driver (CNG/Bcrypt)
 *
 * PROGRAMMERS:     MuerteSeguraZ
 */

#pragma once

#include <ntifs.h>
#include <ndk/exfuncs.h>
#include <ndk/ketypes.h>
#include <md4.h>
#include <md5.h>
#include <tomcrypt.h>
typedef aes_key AES_KEY, *PAES_KEY;
typedef des3_key DES3_KEY, *PDES3_KEY;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _BCRYPT_ALG_HANDLE
{
    LPCWSTR AlgId;
    union
    {
        AES_KEY aes;
        DES3_KEY des3;
    } ctx;
} BCRYPT_ALG_HANDLE, *PBCRYPT_ALG_HANDLE;

typedef struct _BCRYPT_HASH_HANDLE
{
    union
    {
        MD5_CTX md5;
        MD4_CTX md4;
    } state;
    LPCWSTR AlgId;
} BCRYPT_HASH_HANDLE, *PBCRYPT_HASH_HANDLE;

// Basic status codes
#ifndef STATUS_NOT_SUPPORTED
#define STATUS_NOT_SUPPORTED ((NTSTATUS)0xC00000BBL)
#endif

// Functions
NTSTATUS NTAPI BCryptOpenAlgorithmProvider(
    PBCRYPT_ALG_HANDLE *phAlgorithm,
    LPCWSTR pszAlgId,
    LPCWSTR pszImplementation,
    ULONG dwFlags);

NTSTATUS NTAPI BCryptCloseAlgorithmProvider(
    PBCRYPT_ALG_HANDLE hAlgorithm,
    ULONG dwFlags);

NTSTATUS NTAPI BCryptCreateHash(
    PBCRYPT_ALG_HANDLE hAlgorithm,
    PBCRYPT_HASH_HANDLE *phHash,
    PUCHAR pbHashObject,
    ULONG cbHashObject,
    PUCHAR pbSecret,
    ULONG cbSecret,
    ULONG dwFlags);

NTSTATUS NTAPI BCryptHashData(
    PBCRYPT_HASH_HANDLE hHash,
    PUCHAR pbInput,
    ULONG cbInput,
    ULONG dwFlags);

NTSTATUS NTAPI BCryptFinishHash(
    PBCRYPT_HASH_HANDLE hHash,
    PUCHAR pbOutput,
    ULONG cbOutput,
    ULONG dwFlags);

NTSTATUS NTAPI BCryptEncrypt(
    PBCRYPT_ALG_HANDLE hAlgorithm,
    PUCHAR pbInput,
    ULONG cbInput,
    PVOID pPaddingInfo,
    PUCHAR pbIV,
    ULONG cbIV,
    PUCHAR pbOutput,
    ULONG cbOutput,
    ULONG *pcbResult,
    ULONG dwFlags);

NTSTATUS NTAPI BCryptDecrypt(
    PBCRYPT_ALG_HANDLE hAlgorithm,
    PUCHAR pbInput,
    ULONG cbInput,
    PVOID pPaddingInfo,
    PUCHAR pbIV,
    ULONG cbIV,
    PUCHAR pbOutput,
    ULONG cbOutput,
    ULONG *pcbResult,
    ULONG dwFlags);

#ifdef __cplusplus
}
#endif
