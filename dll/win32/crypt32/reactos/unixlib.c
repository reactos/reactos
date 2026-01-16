/*
 * PROJECT:     ReactOS win32 DLLs
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ReactOS emulation layer for crypt32 unixlib calls
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wincrypt.h>
#include <wine/debug.h>
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

// See https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-gpef/e051aba9-c9df-4f82-a42a-c13012c9d381
typedef struct _CRYPT_CERT_PROP
{
    DWORD dwPropId;
    DWORD dwReserved;
    DWORD cbData;
    BYTE ajData[ANYSIZE_ARRAY];
} CRYPT_CERT_PROP, *PCRYPT_CERT_PROP;

static
BOOL
FindCertInRegBlob(
    _In_ const CRYPT_DATA_BLOB* RegBlob,
    _Out_ PCRYPT_DER_BLOB OutCertBlob)
{
    PCRYPT_CERT_PROP prop;
    DWORD offset = 0;

    /* Parse the registry blob */
    while ((offset + sizeof(CRYPT_CERT_PROP)) < RegBlob->cbData)
    {
        prop = (PCRYPT_CERT_PROP)(RegBlob->pbData + offset);

        if (prop->dwReserved != 0x00000001)
        {
            /* Invalid reserved field */
            return FALSE;
        }

        /* Check for the certificate property (ID 32) */
        if (prop->dwPropId == 32)
        {
            if ((offset + prop->cbData) > RegBlob->cbData)
            {
                /* Invalid data size */
                return FALSE;
            }

            OutCertBlob->cbData = prop->cbData;
            OutCertBlob->pbData = prop->ajData;
            return TRUE;
        }

        /* Move to the next property */
        offset += FIELD_OFFSET(CRYPT_CERT_PROP, ajData) + prop->cbData;
    }

    return FALSE;
}

static
BOOL
LoadCertBlobFromReg(
    _In_ HKEY hRootKey,
    _In_z_ PWSTR pwszSubkeyName,
    _Out_ PCRYPT_DATA_BLOB RegDataBlob)
{
    HKEY hCert;
    DWORD dwLength = 0;
    DWORD dwType;
    LSTATUS ret;
    PVOID pvBuffer = NULL;

    RegDataBlob->cbData = 0;
    RegDataBlob->pbData = NULL;

    /* Open the certificate subkey */
    ret = RegOpenKeyExW(hRootKey, pwszSubkeyName, 0, KEY_READ, &hCert);
    if (ret != ERROR_SUCCESS)
    {
        return FALSE;
    }

    /* Query blob size */
    ret = RegQueryValueExW(hCert, L"Blob", NULL, &dwType, NULL, &dwLength);
    if (ret != ERROR_SUCCESS || dwType != REG_BINARY)
    {
        RegCloseKey(hCert);
        return FALSE;
    }

    /* Allocate a the buffer */
    pvBuffer = HeapAlloc(GetProcessHeap(), 0, dwLength);
    if (pvBuffer == NULL)
    {
        RegCloseKey(hCert);
        return FALSE;
    }
        
    /* Fetch the registry blob */
    ret = RegQueryValueExW(hCert, L"Blob", NULL, &dwType, pvBuffer, &dwLength);
    RegCloseKey(hCert);

    if ((ret != ERROR_SUCCESS) || (dwType != REG_BINARY))
    {
        HeapFree(GetProcessHeap(), 0, pvBuffer);
        return FALSE;
    }

    RegDataBlob->pbData = pvBuffer;
    RegDataBlob->cbData = dwLength;
    return TRUE;
}

// Note: this function is not thread-safe! It is called under a lock in crypt32.
static
int
EnumerateRootCertificates(
    PVOID pvBuffer,
    DWORD cbBufferSize,
    PDWORD pcbRequired)
{
    static const WCHAR szRootKey[] =
        L"Software\\Microsoft\\SystemCertificates\\Root\\Certificates";
    static HKEY hRoot = NULL;
    static DWORD dwIndex = 0;
    WCHAR awcSubkey[64];
    CRYPT_DATA_BLOB regBlob;
    CRYPT_DER_BLOB certBlob;
    LSTATUS ret;

    if (pcbRequired == 0)
    {
        return EINVAL;
    }

    if (hRoot == NULL)
    {
        /* Open the key for the root certificates */
        ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE, szRootKey, 0, KEY_READ, &hRoot);
        if (ret != ERROR_SUCCESS)
            return ENOENT;
    }

    /* Enumeration loop */
    for (; dwIndex < MAXDWORD; dwIndex++)
    {
        /* Enumerate next subkey */
        ret = RegEnumKeyW(hRoot, dwIndex, awcSubkey, ARRAYSIZE(awcSubkey));
        if (ret != ERROR_SUCCESS)
        {
            if (ret == ERROR_NO_MORE_ITEMS)
                break;

            continue;
        }

        /* Load the registry blob */
        if (!LoadCertBlobFromReg(hRoot, awcSubkey, &regBlob))
        {
            continue;
        }

        /* Extract the certificate from the registry blob */
        if (!FindCertInRegBlob(&regBlob, &certBlob))
        {
            HeapFree(GetProcessHeap(), 0, regBlob.pbData);
            continue;
        }

        *pcbRequired = certBlob.cbData;

        /* Check if we have a usable buffer */
        if ((pvBuffer != NULL) && (cbBufferSize >= certBlob.cbData))
        {
            /* Copy the certificate data to the output buffer */
            memcpy(pvBuffer, certBlob.pbData, certBlob.cbData);

            /* Increment index for the next call */
            dwIndex++;
        }

        /* Free the registry blob buffer */
        HeapFree(GetProcessHeap(), 0, regBlob.pbData);

        /* Successfully retrieved a certificate */
        return 0;
    }

    RegCloseKey(hRoot);
    hRoot = NULL;
    dwIndex = 0;
    return ENOENT;
}

int __reactos_call_unix_enum_root_certs(void* Args)
{
    struct enum_root_certs_params* params = (struct enum_root_certs_params*)Args;
    return EnumerateRootCertificates(params->buffer,
                                     params->size,
                                     params->needed);
}

static
int
OpenCertStore(
    CRYPT_DATA_BLOB *pfx,
    const WCHAR *password,
    cert_store_data_t *data_ret)
{
    UNIMPLEMENTED;
    return -1;
}

int __reactos_call_unix_open_cert_store(PVOID Args)
{
    struct open_cert_store_params* params = (struct open_cert_store_params*)Args;
    return OpenCertStore(params->pfx,
                         params->password,
                         params->data_ret);
}

static
int
CloseCertStore(cert_store_data_t data)
{
    UNIMPLEMENTED;
    return -1;
}

int __reactos_call_unix_close_cert_store(PVOID Args)
{
    struct close_cert_store_params* params = (struct close_cert_store_params*)Args;
    return CloseCertStore(params->data);
}

static
int
ImportStoreKey(
    cert_store_data_t data,
    void *buf,
    DWORD *buf_size)
{
    UNIMPLEMENTED;
    return -1;
}

int __reactos_call_unix_import_store_key(PVOID Args)
{
    struct import_store_key_params* params = (struct import_store_key_params*)Args;
    return ImportStoreKey(params->data,
                          params->buf,
                          params->buf_size);
}

static
int
ImportStoreCert(
    cert_store_data_t data,
    unsigned int index,
    void *buf,
    DWORD *buf_size)
{
    UNIMPLEMENTED;
    return -1;
}

int __reactos_call_unix_import_store_cert(PVOID Args)
{
    struct import_store_cert_params* params = (struct import_store_cert_params*)Args;
    return ImportStoreCert(params->data,
                           params->index,
                           params->buf,
                           params->buf_size);
}
