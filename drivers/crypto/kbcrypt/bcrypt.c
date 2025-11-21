/*
 * PROJECT:         ReactOS Drivers
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         Kernel Security Support Provider Interface Driver (CNG/Bcrypt)
 *
 * PROGRAMMERS:     MuerteSeguraZ
 */

/* INCLUDES *******************************************************************/
#include <md5.h>
#include <md4.h>
#include <tomcrypt.h>
#include "bcrypt.h"
#define NDEBUG
#include <debug.h>
#include <string.h>

/* HELPERS ********************************************************************/

// Not used yet.

/* Helper to initialize the AES key 
static NTSTATUS NTAPI PrepareAESKey(PBCRYPT_ALG_HANDLE hAlgorithm, const PUCHAR pbKey, ULONG cbKey) __attribute__((unused));
{
    if (!hAlgorithm || !pbKey)
        return STATUS_INVALID_PARAMETER;

    int err = aes_setup(pbKey, cbKey, 0, &hAlgorithm->ctx.aes);
    if (err != CRYPT_OK)
        return STATUS_NOT_SUPPORTED;

    return STATUS_SUCCESS;
}
*/

/* FUNCTIONS ******************************************************************/

NTSTATUS NTAPI BCryptOpenAlgorithmProvider(
    PBCRYPT_ALG_HANDLE *phAlgorithm,
    LPCWSTR pszAlgId,
    LPCWSTR pszImplementation,
    ULONG dwFlags)
{
    UNREFERENCED_PARAMETER(pszImplementation);
    UNREFERENCED_PARAMETER(dwFlags);

    if (!phAlgorithm || !pszAlgId)
        return STATUS_INVALID_PARAMETER;

    PBCRYPT_ALG_HANDLE handle = ExAllocatePoolWithTag(NonPagedPool, sizeof(BCRYPT_ALG_HANDLE), 'bcry');
    if (!handle)
        return STATUS_INSUFFICIENT_RESOURCES;

    handle->AlgId = pszAlgId;

    *phAlgorithm = handle;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI BCryptCloseAlgorithmProvider(
    PBCRYPT_ALG_HANDLE hAlgorithm,
    ULONG dwFlags)
{
    UNREFERENCED_PARAMETER(dwFlags);
    if (!hAlgorithm)
        return STATUS_INVALID_PARAMETER;

    ExFreePoolWithTag(hAlgorithm, 'bcry');
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI BCryptCreateHash(
    PBCRYPT_ALG_HANDLE hAlgorithm,
    PBCRYPT_HASH_HANDLE *phHash,
    PUCHAR pbHashObject,
    ULONG cbHashObject,
    PUCHAR pbSecret,
    ULONG cbSecret,
    ULONG dwFlags)
{
    UNREFERENCED_PARAMETER(pbHashObject);
    UNREFERENCED_PARAMETER(cbHashObject);
    UNREFERENCED_PARAMETER(pbSecret);
    UNREFERENCED_PARAMETER(cbSecret);
    UNREFERENCED_PARAMETER(dwFlags);

    if (!hAlgorithm || !phHash)
        return STATUS_INVALID_PARAMETER;

    PBCRYPT_HASH_HANDLE hash = ExAllocatePoolWithTag(NonPagedPool, sizeof(BCRYPT_HASH_HANDLE), 'bcry');
    if (!hash)
        return STATUS_INSUFFICIENT_RESOURCES;

    if (_wcsicmp(hAlgorithm->AlgId, L"MD5") == 0)
        MD5Init(&hash->state.md5);
    else if (_wcsicmp(hAlgorithm->AlgId, L"MD4") == 0)
        MD4Init(&hash->state.md4);
    else
    {
        ExFreePoolWithTag(hash, 'bcry');
        return STATUS_NOT_SUPPORTED;
    }

    hash->AlgId = hAlgorithm->AlgId;
    *phHash = hash;

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI BCryptHashData(
    PBCRYPT_HASH_HANDLE hHash,
    PUCHAR pbInput,
    ULONG cbInput,
    ULONG dwFlags)
{
    UNREFERENCED_PARAMETER(dwFlags);

    if (!hHash || !pbInput)
        return STATUS_INVALID_PARAMETER;

    if (_wcsicmp(hHash->AlgId, L"MD5") == 0)
        MD5Update(&hHash->state.md5, pbInput, cbInput);
    else if (_wcsicmp(hHash->AlgId, L"MD4") == 0)
        MD4Update(&hHash->state.md4, pbInput, cbInput);
    else
        return STATUS_NOT_SUPPORTED;

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI BCryptFinishHash(
    PBCRYPT_HASH_HANDLE hHash,
    PUCHAR pbOutput,
    ULONG cbOutput,
    ULONG dwFlags)
{
    UNREFERENCED_PARAMETER(dwFlags);

    if (!hHash || !pbOutput)
        return STATUS_INVALID_PARAMETER;

    if (_wcsicmp(hHash->AlgId, L"MD5") == 0)
    {
        MD5Final(&hHash->state.md5);
        RtlCopyMemory(pbOutput, hHash->state.md5.digest, 16);
    }
    else if (_wcsicmp(hHash->AlgId, L"MD4") == 0)
    {   
        MD4Final(&hHash->state.md4);
        RtlCopyMemory(pbOutput, hHash->state.md4.digest, 16);
    }
    else
    {
        return STATUS_NOT_SUPPORTED;
    }

    ExFreePoolWithTag(hHash, 'bcry');
    return STATUS_SUCCESS;
}

// AES encrypt/decrypt wrappers
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
    ULONG dwFlags)
{
    UNREFERENCED_PARAMETER(pPaddingInfo);
    UNREFERENCED_PARAMETER(pbIV);
    UNREFERENCED_PARAMETER(cbIV);
    UNREFERENCED_PARAMETER(dwFlags);

    if (!hAlgorithm || !pbInput || !pbOutput || !pcbResult)
        return STATUS_INVALID_PARAMETER;

    size_t processed = 0;
    for (ULONG i = 0; i < cbInput; i += 16)
    {
        size_t block_size = ((16 < (cbInput - i)) ? 16 : (cbInput - i));
        aes_ecb_encrypt(pbInput + i, pbOutput + i, &hAlgorithm->ctx.aes);
        processed += block_size;
    }

    *pcbResult = processed;
    return STATUS_SUCCESS;
}

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
    ULONG dwFlags)
{
    UNREFERENCED_PARAMETER(pPaddingInfo);
    UNREFERENCED_PARAMETER(pbIV);
    UNREFERENCED_PARAMETER(cbIV);
    UNREFERENCED_PARAMETER(dwFlags);

    if (!hAlgorithm || !pbInput || !pbOutput || !pcbResult)
        return STATUS_INVALID_PARAMETER;

    size_t processed = 0;
    for (ULONG i = 0; i < cbInput; i += 16)
    {
        size_t block_size = ((16 < (cbInput - i)) ? 16 : (cbInput - i));
        aes_ecb_decrypt(pbInput + i, pbOutput + i, &hAlgorithm->ctx.aes);
        processed += block_size;
    }

    *pcbResult = processed;
    return STATUS_SUCCESS;
}
