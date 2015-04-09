/*
 * Schannel tests
 *
 * Copyright 2006 Juan Lang
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
#include <stdio.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#define SECURITY_WIN32
#include <security.h>
#include <schannel.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "wine/test.h"

static HMODULE secdll, crypt32dll;

static ACQUIRE_CREDENTIALS_HANDLE_FN_A pAcquireCredentialsHandleA;
static ENUMERATE_SECURITY_PACKAGES_FN_A pEnumerateSecurityPackagesA;
static FREE_CONTEXT_BUFFER_FN pFreeContextBuffer;
static FREE_CREDENTIALS_HANDLE_FN pFreeCredentialsHandle;
static QUERY_CREDENTIALS_ATTRIBUTES_FN_A pQueryCredentialsAttributesA;
static INITIALIZE_SECURITY_CONTEXT_FN_A pInitializeSecurityContextA;
static QUERY_CONTEXT_ATTRIBUTES_FN_A pQueryContextAttributesA;
static DELETE_SECURITY_CONTEXT_FN pDeleteSecurityContext;
static DECRYPT_MESSAGE_FN pDecryptMessage;
static ENCRYPT_MESSAGE_FN pEncryptMessage;

static PCCERT_CONTEXT (WINAPI *pCertCreateCertificateContext)(DWORD,const BYTE*,DWORD);
static BOOL (WINAPI *pCertFreeCertificateContext)(PCCERT_CONTEXT);
static BOOL (WINAPI *pCertSetCertificateContextProperty)(PCCERT_CONTEXT,DWORD,DWORD,const void*);
static PCCERT_CONTEXT (WINAPI *pCertEnumCertificatesInStore)(HCERTSTORE,PCCERT_CONTEXT);

static BOOL (WINAPI *pCryptAcquireContextW)(HCRYPTPROV*, LPCWSTR, LPCWSTR, DWORD, DWORD);
static BOOL (WINAPI *pCryptDestroyKey)(HCRYPTKEY);
static BOOL (WINAPI *pCryptImportKey)(HCRYPTPROV,const BYTE*,DWORD,HCRYPTKEY,DWORD,HCRYPTKEY*);
static BOOL (WINAPI *pCryptReleaseContext)(HCRYPTPROV,ULONG_PTR);

static const BYTE bigCert[] = { 0x30, 0x7a, 0x02, 0x01, 0x01, 0x30, 0x02, 0x06,
 0x00, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13,
 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x30, 0x22,
 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30,
 0x30, 0x30, 0x30, 0x5a, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30,
 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x30, 0x15, 0x31, 0x13, 0x30,
 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20,
 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x30, 0x07, 0x30, 0x02, 0x06, 0x00, 0x03, 0x01,
 0x00, 0xa3, 0x16, 0x30, 0x14, 0x30, 0x12, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01,
 0x01, 0xff, 0x04, 0x08, 0x30, 0x06, 0x01, 0x01, 0xff, 0x02, 0x01, 0x01 };
static WCHAR cspNameW[] = { 'W','i','n','e','C','r','y','p','t','T','e',
 'm','p',0 };
static BYTE privKey[] = {
 0x07, 0x02, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x52, 0x53, 0x41, 0x32, 0x00,
 0x02, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x79, 0x10, 0x1c, 0xd0, 0x6b, 0x10,
 0x18, 0x30, 0x94, 0x61, 0xdc, 0x0e, 0xcb, 0x96, 0x4e, 0x21, 0x3f, 0x79, 0xcd,
 0xa9, 0x17, 0x62, 0xbc, 0xbb, 0x61, 0x4c, 0xe0, 0x75, 0x38, 0x6c, 0xf3, 0xde,
 0x60, 0x86, 0x03, 0x97, 0x65, 0xeb, 0x1e, 0x6b, 0xdb, 0x53, 0x85, 0xad, 0x68,
 0x21, 0xf1, 0x5d, 0xe7, 0x1f, 0xe6, 0x53, 0xb4, 0xbb, 0x59, 0x3e, 0x14, 0x27,
 0xb1, 0x83, 0xa7, 0x3a, 0x54, 0xe2, 0x8f, 0x65, 0x8e, 0x6a, 0x4a, 0xcf, 0x3b,
 0x1f, 0x65, 0xff, 0xfe, 0xf1, 0x31, 0x3a, 0x37, 0x7a, 0x8b, 0xcb, 0xc6, 0xd4,
 0x98, 0x50, 0x36, 0x67, 0xe4, 0xa1, 0xe8, 0x7e, 0x8a, 0xc5, 0x23, 0xf2, 0x77,
 0xf5, 0x37, 0x61, 0x49, 0x72, 0x59, 0xe8, 0x3d, 0xf7, 0x60, 0xb2, 0x77, 0xca,
 0x78, 0x54, 0x6d, 0x65, 0x9e, 0x03, 0x97, 0x1b, 0x61, 0xbd, 0x0c, 0xd8, 0x06,
 0x63, 0xe2, 0xc5, 0x48, 0xef, 0xb3, 0xe2, 0x6e, 0x98, 0x7d, 0xbd, 0x4e, 0x72,
 0x91, 0xdb, 0x31, 0x57, 0xe3, 0x65, 0x3a, 0x49, 0xca, 0xec, 0xd2, 0x02, 0x4e,
 0x22, 0x7e, 0x72, 0x8e, 0xf9, 0x79, 0x84, 0x82, 0xdf, 0x7b, 0x92, 0x2d, 0xaf,
 0xc9, 0xe4, 0x33, 0xef, 0x89, 0x5c, 0x66, 0x99, 0xd8, 0x80, 0x81, 0x47, 0x2b,
 0xb1, 0x66, 0x02, 0x84, 0x59, 0x7b, 0xc3, 0xbe, 0x98, 0x45, 0x4a, 0x3d, 0xdd,
 0xea, 0x2b, 0xdf, 0x4e, 0xb4, 0x24, 0x6b, 0xec, 0xe7, 0xd9, 0x0c, 0x45, 0xb8,
 0xbe, 0xca, 0x69, 0x37, 0x92, 0x4c, 0x38, 0x6b, 0x96, 0x6d, 0xcd, 0x86, 0x67,
 0x5c, 0xea, 0x54, 0x94, 0xa4, 0xca, 0xa4, 0x02, 0xa5, 0x21, 0x4d, 0xae, 0x40,
 0x8f, 0x9d, 0x51, 0x83, 0xf2, 0x3f, 0x33, 0xc1, 0x72, 0xb4, 0x1d, 0x94, 0x6e,
 0x7d, 0xe4, 0x27, 0x3f, 0xea, 0xff, 0xe5, 0x9b, 0xa7, 0x5e, 0x55, 0x8e, 0x0d,
 0x69, 0x1c, 0x7a, 0xff, 0x81, 0x9d, 0x53, 0x52, 0x97, 0x9a, 0x76, 0x79, 0xda,
 0x93, 0x32, 0x16, 0xec, 0x69, 0x51, 0x1a, 0x4e, 0xc3, 0xf1, 0x72, 0x80, 0x78,
 0x5e, 0x66, 0x4a, 0x8d, 0x85, 0x2f, 0x3f, 0xb2, 0xa7 };

static const BYTE selfSignedCert[] = {
 0x30, 0x82, 0x01, 0x1f, 0x30, 0x81, 0xce, 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02,
 0x10, 0xeb, 0x0d, 0x57, 0x2a, 0x9c, 0x09, 0xba, 0xa4, 0x4a, 0xb7, 0x25, 0x49,
 0xd9, 0x3e, 0xb5, 0x73, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e, 0x03, 0x02, 0x1d,
 0x05, 0x00, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03,
 0x13, 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x30,
 0x1e, 0x17, 0x0d, 0x30, 0x36, 0x30, 0x36, 0x32, 0x39, 0x30, 0x35, 0x30, 0x30,
 0x34, 0x36, 0x5a, 0x17, 0x0d, 0x30, 0x37, 0x30, 0x36, 0x32, 0x39, 0x31, 0x31,
 0x30, 0x30, 0x34, 0x36, 0x5a, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03,
 0x55, 0x04, 0x03, 0x13, 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e,
 0x67, 0x00, 0x30, 0x5c, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7,
 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x4b, 0x00, 0x30, 0x48, 0x02, 0x41,
 0x00, 0xe2, 0x54, 0x3a, 0xa7, 0x83, 0xb1, 0x27, 0x14, 0x3e, 0x59, 0xbb, 0xb4,
 0x53, 0xe6, 0x1f, 0xe7, 0x5d, 0xf1, 0x21, 0x68, 0xad, 0x85, 0x53, 0xdb, 0x6b,
 0x1e, 0xeb, 0x65, 0x97, 0x03, 0x86, 0x60, 0xde, 0xf3, 0x6c, 0x38, 0x75, 0xe0,
 0x4c, 0x61, 0xbb, 0xbc, 0x62, 0x17, 0xa9, 0xcd, 0x79, 0x3f, 0x21, 0x4e, 0x96,
 0xcb, 0x0e, 0xdc, 0x61, 0x94, 0x30, 0x18, 0x10, 0x6b, 0xd0, 0x1c, 0x10, 0x79,
 0x02, 0x03, 0x01, 0x00, 0x01, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e, 0x03, 0x02,
 0x1d, 0x05, 0x00, 0x03, 0x41, 0x00, 0x25, 0x90, 0x53, 0x34, 0xd9, 0x56, 0x41,
 0x5e, 0xdb, 0x7e, 0x01, 0x36, 0xec, 0x27, 0x61, 0x5e, 0xb7, 0x4d, 0x90, 0x66,
 0xa2, 0xe1, 0x9d, 0x58, 0x76, 0xd4, 0x9c, 0xba, 0x2c, 0x84, 0xc6, 0x83, 0x7a,
 0x22, 0x0d, 0x03, 0x69, 0x32, 0x1a, 0x6d, 0xcb, 0x0c, 0x15, 0xb3, 0x6b, 0xc7,
 0x0a, 0x8c, 0xb4, 0x5c, 0x34, 0x78, 0xe0, 0x3c, 0x9c, 0xe9, 0xf3, 0x30, 0x9f,
 0xa8, 0x76, 0x57, 0x92, 0x36 };

static void InitFunctionPtrs(void)
{
    HMODULE advapi32dll;

    crypt32dll = LoadLibraryA("crypt32.dll");
    secdll = LoadLibraryA("secur32.dll");
    if(!secdll)
        secdll = LoadLibraryA("security.dll");
    advapi32dll = LoadLibraryA("advapi32.dll");

#define GET_PROC(h, func)  p ## func = (void*)GetProcAddress(h, #func)

    if(secdll)
    {
        GET_PROC(secdll, AcquireCredentialsHandleA);
        GET_PROC(secdll, EnumerateSecurityPackagesA);
        GET_PROC(secdll, FreeContextBuffer);
        GET_PROC(secdll, FreeCredentialsHandle);
        GET_PROC(secdll, QueryCredentialsAttributesA);
        GET_PROC(secdll, InitializeSecurityContextA);
        GET_PROC(secdll, QueryContextAttributesA);
        GET_PROC(secdll, DeleteSecurityContext);
        GET_PROC(secdll, DecryptMessage);
        GET_PROC(secdll, EncryptMessage);
    }

    GET_PROC(advapi32dll, CryptAcquireContextW);
    GET_PROC(advapi32dll, CryptDestroyKey);
    GET_PROC(advapi32dll, CryptImportKey);
    GET_PROC(advapi32dll, CryptReleaseContext);

    GET_PROC(crypt32dll, CertFreeCertificateContext);
    GET_PROC(crypt32dll, CertSetCertificateContextProperty);
    GET_PROC(crypt32dll, CertCreateCertificateContext);
    GET_PROC(crypt32dll, CertEnumCertificatesInStore);

#undef GET_PROC
}

static void test_strength(PCredHandle handle)
{
    SecPkgCred_CipherStrengths strength = {-1,-1};
    SECURITY_STATUS st;

    st = pQueryCredentialsAttributesA(handle, SECPKG_ATTR_CIPHER_STRENGTHS, &strength);
    ok(st == SEC_E_OK, "QueryCredentialsAttributesA failed: %u\n", GetLastError());
    ok(strength.dwMinimumCipherStrength, "dwMinimumCipherStrength not changed\n");
    ok(strength.dwMaximumCipherStrength, "dwMaximumCipherStrength not changed\n");
    trace("strength %d - %d\n", strength.dwMinimumCipherStrength, strength.dwMaximumCipherStrength);
}

static void test_supported_protocols(CredHandle *handle, unsigned exprots)
{
    SecPkgCred_SupportedProtocols protocols;
    SECURITY_STATUS status;

    status = pQueryCredentialsAttributesA(handle, SECPKG_ATTR_SUPPORTED_PROTOCOLS, &protocols);
    ok(status == SEC_E_OK, "QueryCredentialsAttributes failed: %08x\n", status);

    if(exprots)
        ok(protocols.grbitProtocol == exprots, "protocols.grbitProtocol = %x, expected %x\n", protocols.grbitProtocol, exprots);

    trace("Supported protocols:\n");

#define X(flag, name) do { if(protocols.grbitProtocol & flag) { trace(name "\n"); protocols.grbitProtocol &= ~flag; } }while(0)
    X(SP_PROT_SSL2_CLIENT, "SSL 2 client");
    X(SP_PROT_SSL3_CLIENT, "SSL 3 client");
    X(SP_PROT_TLS1_0_CLIENT, "TLS 1.0 client");
    X(SP_PROT_TLS1_1_CLIENT, "TLS 1.1 client");
    X(SP_PROT_TLS1_2_CLIENT, "TLS 1.2 client");
#undef X

    if(protocols.grbitProtocol)
        trace("Unknown flags: %x\n", protocols.grbitProtocol);
}

static void testAcquireSecurityContext(void)
{
    BOOL has_schannel = FALSE;
    SecPkgInfoA *package_info;
    ULONG i;
    SECURITY_STATUS st;
    CredHandle cred;
    SecPkgCredentials_NamesA names;
    TimeStamp exp;
    SCHANNEL_CRED schanCred;
    PCCERT_CONTEXT certs[2];
    HCRYPTPROV csp;
    static CHAR unisp_name_a[] = UNISP_NAME_A;
    WCHAR ms_def_prov_w[MAX_PATH];
    BOOL ret;
    HCRYPTKEY key;
    CRYPT_KEY_PROV_INFO keyProvInfo;

    if (!pAcquireCredentialsHandleA || !pCertCreateCertificateContext ||
        !pEnumerateSecurityPackagesA || !pFreeContextBuffer ||
        !pFreeCredentialsHandle || !pCryptAcquireContextW)
    {
        win_skip("Needed functions are not available\n");
        return;
    }

    if (SUCCEEDED(pEnumerateSecurityPackagesA(&i, &package_info)))
    {
        while(i--)
        {
            if (!strcmp(package_info[i].Name, unisp_name_a))
            {
                has_schannel = TRUE;
                break;
            }
        }
        pFreeContextBuffer(package_info);
    }
    if (!has_schannel)
    {
        skip("Schannel not available\n");
        return;
    }

    lstrcpyW(ms_def_prov_w, MS_DEF_PROV_W);

    keyProvInfo.pwszContainerName = cspNameW;
    keyProvInfo.pwszProvName = ms_def_prov_w;
    keyProvInfo.dwProvType = PROV_RSA_FULL;
    keyProvInfo.dwFlags = 0;
    keyProvInfo.cProvParam = 0;
    keyProvInfo.rgProvParam = NULL;
    keyProvInfo.dwKeySpec = AT_SIGNATURE;

    certs[0] = pCertCreateCertificateContext(X509_ASN_ENCODING, bigCert,
     sizeof(bigCert));
    certs[1] = pCertCreateCertificateContext(X509_ASN_ENCODING, selfSignedCert,
     sizeof(selfSignedCert));

    SetLastError(0xdeadbeef);
    ret = pCryptAcquireContextW(&csp, cspNameW, MS_DEF_PROV_W, PROV_RSA_FULL,
     CRYPT_DELETEKEYSET);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        /* WinMe would crash on some tests */
        win_skip("CryptAcquireContextW is not implemented\n");
        return;
    }

    st = pAcquireCredentialsHandleA(NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL,
     NULL);
    ok(st == SEC_E_SECPKG_NOT_FOUND,
     "Expected SEC_E_SECPKG_NOT_FOUND, got %08x\n", st);
    if (0)
    {
        /* Crashes on Win2K */
        st = pAcquireCredentialsHandleA(NULL, unisp_name_a, 0, NULL, NULL, NULL,
         NULL, NULL, NULL);
        ok(st == SEC_E_NO_CREDENTIALS, "Expected SEC_E_NO_CREDENTIALS, got %08x\n", st);

        /* Crashes on WinNT */
        st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_BOTH, NULL,
         NULL, NULL, NULL, NULL, NULL);
        ok(st == SEC_E_NO_CREDENTIALS, "Expected SEC_E_NO_CREDENTIALS, got %08x\n", st);

        st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
         NULL, NULL, NULL, NULL, NULL, NULL);
        ok(st == SEC_E_NO_CREDENTIALS, "Expected SEC_E_NO_CREDENTIALS, got %08x\n", st);

        /* Crashes */
        pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
         NULL, NULL, NULL, NULL, NULL, NULL);
    }
    st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
     NULL, NULL, NULL, NULL, &cred, NULL);
    ok(st == SEC_E_OK, "AcquireCredentialsHandleA failed: %08x\n", st);
    if(st == SEC_E_OK) {
        st = pQueryCredentialsAttributesA(&cred, SECPKG_ATTR_SUPPORTED_PROTOCOLS, NULL);
        ok(st == SEC_E_INTERNAL_ERROR, "QueryCredentialsAttributes failed: %08x, expected SEC_E_INTERNAL_ERROR\n", st);

        test_supported_protocols(&cred, 0);
        pFreeCredentialsHandle(&cred);
    }
    memset(&cred, 0, sizeof(cred));
    st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
     NULL, NULL, NULL, NULL, &cred, &exp);
    ok(st == SEC_E_OK, "AcquireCredentialsHandleA failed: %08x\n", st);
    /* expriy is indeterminate in win2k3 */
    trace("expiry: %08x%08x\n", exp.HighPart, exp.LowPart);

    st = pQueryCredentialsAttributesA(&cred, SECPKG_CRED_ATTR_NAMES, &names);
    ok(st == SEC_E_NO_CREDENTIALS || st == SEC_E_UNSUPPORTED_FUNCTION /* before Vista */, "expected SEC_E_NO_CREDENTIALS, got %08x\n", st);

    pFreeCredentialsHandle(&cred);

    /* Bad version in SCHANNEL_CRED */
    memset(&schanCred, 0, sizeof(schanCred));
    st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
     NULL, &schanCred, NULL, NULL, NULL, NULL);
    ok(st == SEC_E_INTERNAL_ERROR ||
       st == SEC_E_UNKNOWN_CREDENTIALS /* Vista/win2k8 */ ||
       st == SEC_E_INVALID_TOKEN /* WinNT */, "st = %08x\n", st);
    st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
     NULL, &schanCred, NULL, NULL, NULL, NULL);
    ok(st == SEC_E_INTERNAL_ERROR ||
       st == SEC_E_UNKNOWN_CREDENTIALS /* Vista/win2k8 */ ||
       st == SEC_E_INVALID_TOKEN /* WinNT */, "st = %08x\n", st);

    /* No cert in SCHANNEL_CRED succeeds for outbound.. */
    schanCred.dwVersion = SCHANNEL_CRED_VERSION;
    st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
     NULL, &schanCred, NULL, NULL, &cred, NULL);
    ok(st == SEC_E_OK, "AcquireCredentialsHandleA failed: %08x\n", st);
    pFreeCredentialsHandle(&cred);
    /* but fails for inbound. */
    st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
     NULL, &schanCred, NULL, NULL, &cred, NULL);
    ok(st == SEC_E_NO_CREDENTIALS ||
       st == SEC_E_OK /* Vista/win2k8 */,
       "Expected SEC_E_NO_CREDENTIALS or SEC_E_OK, got %08x\n", st);

    if (0)
    {
        /* Crashes with bad paCred pointer */
        schanCred.cCreds = 1;
        pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
         NULL, &schanCred, NULL, NULL, NULL, NULL);
    }

    /* Bogus cert in SCHANNEL_CRED. Windows fails with
     * SEC_E_UNKNOWN_CREDENTIALS, but I'll accept SEC_E_NO_CREDENTIALS too.
     */
    schanCred.cCreds = 1;
    schanCred.paCred = &certs[0];
    st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
     NULL, &schanCred, NULL, NULL, NULL, NULL);
    ok(st == SEC_E_UNKNOWN_CREDENTIALS ||
       st == SEC_E_NO_CREDENTIALS ||
       st == SEC_E_INVALID_TOKEN /* WinNT */, "st = %08x\n", st);
    st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
     NULL, &schanCred, NULL, NULL, NULL, NULL);
    ok(st == SEC_E_UNKNOWN_CREDENTIALS ||
       st == SEC_E_NO_CREDENTIALS ||
       st == SEC_E_INVALID_TOKEN /* WinNT */, "st = %08x\n", st);

    /* Good cert, but missing private key. Windows fails with
     * SEC_E_NO_CREDENTIALS, but I'll accept SEC_E_UNKNOWN_CREDENTIALS too.
     */
    schanCred.cCreds = 1;
    schanCred.paCred = &certs[1];
    st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
     NULL, &schanCred, NULL, NULL, &cred, NULL);
    ok(st == SEC_E_UNKNOWN_CREDENTIALS || st == SEC_E_NO_CREDENTIALS ||
       st == SEC_E_INTERNAL_ERROR, /* win2k */
     "Expected SEC_E_UNKNOWN_CREDENTIALS, SEC_E_NO_CREDENTIALS "
     "or SEC_E_INTERNAL_ERROR, got %08x\n", st);
    st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
     NULL, &schanCred, NULL, NULL, NULL, NULL);
    ok(st == SEC_E_UNKNOWN_CREDENTIALS || st == SEC_E_NO_CREDENTIALS ||
       st == SEC_E_INTERNAL_ERROR, /* win2k */
     "Expected SEC_E_UNKNOWN_CREDENTIALS, SEC_E_NO_CREDENTIALS "
     "or SEC_E_INTERNAL_ERROR, got %08x\n", st);

    /* Good cert, with CRYPT_KEY_PROV_INFO set before it's had a key loaded. */
    if (pCertSetCertificateContextProperty)
    {
        ret = pCertSetCertificateContextProperty(certs[1],
              CERT_KEY_PROV_INFO_PROP_ID, 0, &keyProvInfo);
        schanCred.dwVersion = SCH_CRED_V3;
        ok(ret, "CertSetCertificateContextProperty failed: %08x\n", GetLastError());
        st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
             NULL, &schanCred, NULL, NULL, &cred, NULL);
        ok(st == SEC_E_UNKNOWN_CREDENTIALS || st == SEC_E_INTERNAL_ERROR /* WinNT */,
           "Expected SEC_E_UNKNOWN_CREDENTIALS or SEC_E_INTERNAL_ERROR, got %08x\n", st);
        st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
             NULL, &schanCred, NULL, NULL, &cred, NULL);
        ok(st == SEC_E_UNKNOWN_CREDENTIALS || st == SEC_E_INTERNAL_ERROR /* WinNT */,
           "Expected SEC_E_UNKNOWN_CREDENTIALS or SEC_E_INTERNAL_ERROR, got %08x\n", st);
    }

    ret = pCryptAcquireContextW(&csp, cspNameW, MS_DEF_PROV_W, PROV_RSA_FULL,
     CRYPT_NEWKEYSET);
    ok(ret, "CryptAcquireContextW failed: %08x\n", GetLastError());
    ret = 0;
    if (pCryptImportKey)
    {
        ret = pCryptImportKey(csp, privKey, sizeof(privKey), 0, 0, &key);
        ok(ret, "CryptImportKey failed: %08x\n", GetLastError());
    }
    if (ret)
    {
        PCCERT_CONTEXT tmp;

        if (0)
        {
            /* Crashes */
            pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
             NULL, &schanCred, NULL, NULL, NULL, NULL);

            /* Crashes on WinNT */
            /* Good cert with private key, bogus version */
            schanCred.dwVersion = SCH_CRED_V1;
            st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
                NULL, &schanCred, NULL, NULL, &cred, NULL);
            ok(st == SEC_E_INTERNAL_ERROR ||
                st == SEC_E_UNKNOWN_CREDENTIALS /* Vista/win2k8 */,
                "Expected SEC_E_INTERNAL_ERROR or SEC_E_UNKNOWN_CREDENTIALS, got %08x\n", st);
            st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
                NULL, &schanCred, NULL, NULL, &cred, NULL);
            ok(st == SEC_E_INTERNAL_ERROR ||
                st == SEC_E_UNKNOWN_CREDENTIALS /* Vista/win2k8 */,
                "Expected SEC_E_INTERNAL_ERROR or SEC_E_UNKNOWN_CREDENTIALS, got %08x\n", st);
            schanCred.dwVersion = SCH_CRED_V2;
            st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
                NULL, &schanCred, NULL, NULL, &cred, NULL);
            ok(st == SEC_E_INTERNAL_ERROR ||
                st == SEC_E_UNKNOWN_CREDENTIALS /* Vista/win2k8 */,
                "Expected SEC_E_INTERNAL_ERROR or SEC_E_UNKNOWN_CREDENTIALS, got %08x\n", st);
            st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
                NULL, &schanCred, NULL, NULL, &cred, NULL);
            ok(st == SEC_E_INTERNAL_ERROR ||
                st == SEC_E_UNKNOWN_CREDENTIALS /* Vista/win2k8 */,
                "Expected SEC_E_INTERNAL_ERROR or SEC_E_UNKNOWN_CREDENTIALS, got %08x\n", st);
        }

        /* Succeeds on V3 or higher */
        schanCred.dwVersion = SCH_CRED_V3;
        st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
         NULL, &schanCred, NULL, NULL, &cred, NULL);
        ok(st == SEC_E_OK, "AcquireCredentialsHandleA failed: %08x\n", st);
        pFreeCredentialsHandle(&cred);
        st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
         NULL, &schanCred, NULL, NULL, &cred, NULL);
        ok(st == SEC_E_OK ||
           st == SEC_E_UNKNOWN_CREDENTIALS, /* win2k3 */
           "AcquireCredentialsHandleA failed: %08x\n", st);
        pFreeCredentialsHandle(&cred);
        schanCred.dwVersion = SCHANNEL_CRED_VERSION;
        st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
         NULL, &schanCred, NULL, NULL, &cred, NULL);
        ok(st == SEC_E_OK, "AcquireCredentialsHandleA failed: %08x\n", st);
        pFreeCredentialsHandle(&cred);
        st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
         NULL, &schanCred, NULL, NULL, &cred, NULL);
        ok(st == SEC_E_OK ||
           st == SEC_E_UNKNOWN_CREDENTIALS, /* win2k3 */
           "AcquireCredentialsHandleA failed: %08x\n", st);
        if (st == SEC_E_OK) test_strength(&cred);
        pFreeCredentialsHandle(&cred);

        /* How about more than one cert? */
        schanCred.cCreds = 2;
        schanCred.paCred = certs;
        st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
         NULL, &schanCred, NULL, NULL, &cred, NULL);
        ok(st == SEC_E_UNKNOWN_CREDENTIALS ||
           st == SEC_E_NO_CREDENTIALS /* Vista/win2k8 */ ||
           st == SEC_E_INVALID_TOKEN /* WinNT */, "st = %08x\n", st);
        st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
         NULL, &schanCred, NULL, NULL, &cred, NULL);
        ok(st == SEC_E_UNKNOWN_CREDENTIALS ||
           st == SEC_E_NO_CREDENTIALS ||
           st == SEC_E_INVALID_TOKEN /* WinNT */, "st = %08x\n", st);
        tmp = certs[0];
        certs[0] = certs[1];
        certs[1] = tmp;
        st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
         NULL, &schanCred, NULL, NULL, &cred, NULL);
        ok(st == SEC_E_UNKNOWN_CREDENTIALS ||
           st == SEC_E_NO_CREDENTIALS ||
           st == SEC_E_INVALID_TOKEN /* WinNT */, "st = %08x\n", st);
        st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
         NULL, &schanCred, NULL, NULL, &cred, NULL);
        ok(st == SEC_E_UNKNOWN_CREDENTIALS,
         "Expected SEC_E_UNKNOWN_CREDENTIALS, got %08x\n", st);
        /* FIXME: what about two valid certs? */

        if (pCryptDestroyKey)
            pCryptDestroyKey(key);
    }

    if (pCryptReleaseContext)
        pCryptReleaseContext(csp, 0);
    pCryptAcquireContextW(&csp, cspNameW, MS_DEF_PROV_W, PROV_RSA_FULL,
     CRYPT_DELETEKEYSET);

    if (pCertFreeCertificateContext)
    {
        pCertFreeCertificateContext(certs[0]);
        pCertFreeCertificateContext(certs[1]);
    }
}

static void test_remote_cert(PCCERT_CONTEXT remote_cert)
{
    PCCERT_CONTEXT iter = NULL;
    BOOL incl_remote = FALSE;
    unsigned cert_cnt = 0;

    ok(remote_cert->hCertStore != NULL, "hCertStore == NULL\n");

    while((iter = pCertEnumCertificatesInStore(remote_cert->hCertStore, iter))) {
        if(iter == remote_cert)
            incl_remote = TRUE;
        cert_cnt++;
    }

    ok(cert_cnt == 3, "cert_cnt = %u\n", cert_cnt);
    ok(incl_remote, "context does not contain cert itself\n");
}

static const char http_request[] = "HEAD /test.html HTTP/1.1\r\nHost: www.winehq.org\r\nConnection: close\r\n\r\n";

static void init_cred(SCHANNEL_CRED *cred)
{
    cred->dwVersion = SCHANNEL_CRED_VERSION;
    cred->cCreds = 0;
    cred->paCred = 0;
    cred->hRootStore = NULL;
    cred->cMappers = 0;
    cred->aphMappers = NULL;
    cred->cSupportedAlgs = 0;
    cred->palgSupportedAlgs = NULL;
    cred->grbitEnabledProtocols = SP_PROT_TLS1_CLIENT;
    cred->dwMinimumCipherStrength = 0;
    cred->dwMaximumCipherStrength = 0;
    cred->dwSessionLifespan = 0;
    cred->dwFlags = 0;
}

static void init_buffers(SecBufferDesc *desc, unsigned count, unsigned size)
{
    desc->ulVersion = SECBUFFER_VERSION;
    desc->cBuffers = count;
    desc->pBuffers = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, count*sizeof(SecBuffer));

    desc->pBuffers[0].cbBuffer = size;
    desc->pBuffers[0].pvBuffer = HeapAlloc(GetProcessHeap(), 0, size);
}

static void reset_buffers(SecBufferDesc *desc)
{
    unsigned i;

    for (i = 0; i < desc->cBuffers; ++i)
    {
        desc->pBuffers[i].BufferType = SECBUFFER_EMPTY;
        if (i > 0)
        {
            desc->pBuffers[i].cbBuffer = 0;
            desc->pBuffers[i].pvBuffer = NULL;
        }
    }
}

static void free_buffers(SecBufferDesc *desc)
{
    HeapFree(GetProcessHeap(), 0, desc->pBuffers[0].pvBuffer);
    HeapFree(GetProcessHeap(), 0, desc->pBuffers);
}

static int receive_data(SOCKET sock, SecBuffer *buf)
{
    unsigned received = 0;

    while (1)
    {
        unsigned char *data = buf->pvBuffer;
        unsigned expected = 0;
        int ret;

        ret = recv(sock, (char *)data+received, buf->cbBuffer-received, 0);
        if (ret == -1)
        {
            skip("recv failed\n");
            return -1;
        }
        else if(ret == 0)
        {
            skip("connection closed\n");
            return -1;
        }
        received += ret;

        while (expected < received)
        {
            unsigned frame_size = 5 + ((data[3]<<8) | data[4]);
            expected += frame_size;
            data += frame_size;
        }

        if (expected == received)
            break;
    }

    buf->cbBuffer = received;

    return received;
}

static void test_communication(void)
{
    int ret;

    WSADATA wsa_data;
    SOCKET sock;
    struct hostent *host;
    struct sockaddr_in addr;

    SECURITY_STATUS status;
    ULONG attrs;

    SCHANNEL_CRED cred;
    CredHandle cred_handle;
    CtxtHandle context;
    SecPkgCredentials_NamesA names;
    SecPkgContext_StreamSizes sizes;
    SecPkgContext_ConnectionInfo conn_info;
    CERT_CONTEXT *cert;

    SecBufferDesc buffers[2];
    SecBuffer *buf;
    unsigned buf_size = 4000;
    unsigned char *data;
    unsigned data_size;

    if (!pAcquireCredentialsHandleA || !pFreeCredentialsHandle ||
        !pInitializeSecurityContextA || !pDeleteSecurityContext ||
        !pQueryContextAttributesA || !pDecryptMessage || !pEncryptMessage)
    {
        skip("Required secur32 functions not available\n");
        return;
    }

    /* Create a socket and connect to www.winehq.org */
    ret = WSAStartup(0x0202, &wsa_data);
    if (ret)
    {
        skip("Can't init winsock 2.2\n");
        return;
    }

    host = gethostbyname("www.winehq.org");
    if (!host)
    {
        skip("Can't resolve www.winehq.org\n");
        return;
    }

    addr.sin_family = host->h_addrtype;
    addr.sin_addr = *(struct in_addr *)host->h_addr_list[0];
    addr.sin_port = htons(443);
    sock = socket(host->h_addrtype, SOCK_STREAM, 0);
    if (sock == SOCKET_ERROR)
    {
        skip("Can't create socket\n");
        return;
    }

    ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == SOCKET_ERROR)
    {
        skip("Can't connect to www.winehq.org\n");
        return;
    }

    /* Create client credentials */
    init_cred(&cred);
    cred.dwFlags = SCH_CRED_NO_DEFAULT_CREDS|SCH_CRED_MANUAL_CRED_VALIDATION;

    status = pAcquireCredentialsHandleA(NULL, (SEC_CHAR *)UNISP_NAME_A, SECPKG_CRED_OUTBOUND, NULL,
        &cred, NULL, NULL, &cred_handle, NULL);
    ok(status == SEC_E_OK, "AcquireCredentialsHandleA failed: %08x\n", status);
    if (status != SEC_E_OK) return;

    test_supported_protocols(&cred_handle, SP_PROT_TLS1_CLIENT);

    /* Initialize the connection */
    init_buffers(&buffers[0], 4, buf_size);
    init_buffers(&buffers[1], 4, buf_size);

    buffers[0].pBuffers[0].BufferType = SECBUFFER_TOKEN;
    status = pInitializeSecurityContextA(&cred_handle, NULL, (SEC_CHAR *)"localhost",
        ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM,
        0, 0, NULL, 0, &context, &buffers[0], &attrs, NULL);
    ok(status == SEC_I_CONTINUE_NEEDED, "Expected SEC_I_CONTINUE_NEEDED, got %08x\n", status);

    buffers[1].cBuffers = 1;
    buffers[1].pBuffers[0].BufferType = SECBUFFER_TOKEN;
    buffers[0].pBuffers[0].cbBuffer = 1;
    memset(buffers[1].pBuffers[0].pvBuffer, 0xfa, buf_size);
    status = pInitializeSecurityContextA(&cred_handle, &context, (SEC_CHAR *)"localhost",
            ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM,
            0, 0, &buffers[1], 0, NULL, &buffers[0], &attrs, NULL);
todo_wine
    ok(status == SEC_E_INVALID_TOKEN, "Expected SEC_E_INVALID_TOKEN, got %08x\n", status);
todo_wine
    ok(buffers[0].pBuffers[0].cbBuffer == 0, "Output buffer size was not set to 0.\n");

    buffers[1].cBuffers = 1;
    buffers[1].pBuffers[0].BufferType = SECBUFFER_TOKEN;
    buffers[0].pBuffers[0].cbBuffer = 1;
    memset(buffers[1].pBuffers[0].pvBuffer, 0, buf_size);
    status = pInitializeSecurityContextA(&cred_handle, &context, (SEC_CHAR *)"localhost",
            ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM,
            0, 0, &buffers[1], 0, NULL, &buffers[0], &attrs, NULL);
    ok(status == SEC_E_INVALID_TOKEN, "Expected SEC_E_INVALID_TOKEN, got %08x\n", status);
todo_wine
    ok(buffers[0].pBuffers[0].cbBuffer == 0, "Output buffer size was not set to 0.\n");

    buffers[0].pBuffers[0].cbBuffer = 0;
    status = pInitializeSecurityContextA(&cred_handle, &context, (SEC_CHAR *)"localhost",
            ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM,
            0, 0, &buffers[1], 0, NULL, &buffers[0], &attrs, NULL);
todo_wine
    ok(status == SEC_E_INSUFFICIENT_MEMORY || status == SEC_E_INVALID_TOKEN,
       "Expected SEC_E_INSUFFICIENT_MEMORY or SEC_E_INVALID_TOKEN, got %08x\n", status);

    buffers[0].pBuffers[0].cbBuffer = buf_size;

    status = pInitializeSecurityContextA(&cred_handle, NULL, (SEC_CHAR *)"localhost",
            ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM,
            0, 0, NULL, 0, &context, &buffers[0], &attrs, NULL);
    ok(status == SEC_I_CONTINUE_NEEDED, "Expected SEC_I_CONTINUE_NEEDED, got %08x\n", status);

    buf = &buffers[0].pBuffers[0];
    send(sock, buf->pvBuffer, buf->cbBuffer, 0);
    buf->cbBuffer = buf_size;

    status = pInitializeSecurityContextA(&cred_handle, &context, (SEC_CHAR *)"localhost",
            ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM,
            0, 0, NULL, 0, NULL, &buffers[0], &attrs, NULL);
    ok(status == SEC_E_INCOMPLETE_MESSAGE, "Got unexpected status %#x.\n", status);
    ok(buffers[0].pBuffers[0].cbBuffer == buf_size, "Output buffer size changed.\n");
    ok(buffers[0].pBuffers[0].BufferType == SECBUFFER_TOKEN, "Output buffer type changed.\n");

    buffers[1].cBuffers = 4;
    buffers[1].pBuffers[0].cbBuffer = 0;

    status = pInitializeSecurityContextA(&cred_handle, &context, (SEC_CHAR *)"localhost",
            ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM,
            0, 0, &buffers[1], 0, NULL, &buffers[0], &attrs, NULL);
    ok(status == SEC_E_INCOMPLETE_MESSAGE, "Got unexpected status %#x.\n", status);
    ok(buffers[0].pBuffers[0].cbBuffer == buf_size, "Output buffer size changed.\n");
    ok(buffers[0].pBuffers[0].BufferType == SECBUFFER_TOKEN, "Output buffer type changed.\n");

    buf = &buffers[1].pBuffers[0];
    buf->cbBuffer = buf_size;
    ret = receive_data(sock, buf);
    if (ret == -1)
        return;

    buffers[1].pBuffers[0].cbBuffer = 4;
    status = pInitializeSecurityContextA(&cred_handle, &context, (SEC_CHAR *)"localhost",
            ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM,
            0, 0, &buffers[1], 0, NULL, &buffers[0], &attrs, NULL);
    ok(status == SEC_E_INCOMPLETE_MESSAGE, "Got unexpected status %#x.\n", status);
    ok(buffers[0].pBuffers[0].cbBuffer == buf_size, "Output buffer size changed.\n");
    ok(buffers[0].pBuffers[0].BufferType == SECBUFFER_TOKEN, "Output buffer type changed.\n");

    buffers[1].pBuffers[0].cbBuffer = 5;
    status = pInitializeSecurityContextA(&cred_handle, &context, (SEC_CHAR *)"localhost",
            ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM,
            0, 0, &buffers[1], 0, NULL, &buffers[0], &attrs, NULL);
    ok(status == SEC_E_INCOMPLETE_MESSAGE, "Got unexpected status %#x.\n", status);
    ok(buffers[0].pBuffers[0].cbBuffer == buf_size, "Output buffer size changed.\n");
    ok(buffers[0].pBuffers[0].BufferType == SECBUFFER_TOKEN, "Output buffer type changed.\n");

    buffers[1].pBuffers[0].cbBuffer = ret;
    status = pInitializeSecurityContextA(&cred_handle, &context, (SEC_CHAR *)"localhost",
            ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM,
            0, 0, &buffers[1], 0, NULL, &buffers[0], &attrs, NULL);
    buffers[1].pBuffers[0].cbBuffer = buf_size;
    while (status == SEC_I_CONTINUE_NEEDED)
    {
        buf = &buffers[0].pBuffers[0];
        send(sock, buf->pvBuffer, buf->cbBuffer, 0);
        buf->cbBuffer = buf_size;

        buf = &buffers[1].pBuffers[0];
        ret = receive_data(sock, buf);
        if (ret == -1)
            return;

        buf->BufferType = SECBUFFER_TOKEN;

        status = pInitializeSecurityContextA(&cred_handle, &context, (SEC_CHAR *)"localhost",
            ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM,
            0, 0, &buffers[1], 0, NULL, &buffers[0], &attrs, NULL);
        buffers[1].pBuffers[0].cbBuffer = buf_size;
    }

    ok(status == SEC_E_OK || broken(status == SEC_E_INVALID_TOKEN) /* WinNT */,
        "InitializeSecurityContext failed: %08x\n", status);
    if(status != SEC_E_OK) {
        win_skip("Handshake failed\n");
        return;
    }

    status = pQueryCredentialsAttributesA(&cred_handle, SECPKG_CRED_ATTR_NAMES, &names);
    ok(status == SEC_E_NO_CREDENTIALS || status == SEC_E_UNSUPPORTED_FUNCTION /* before Vista */, "expected SEC_E_NO_CREDENTIALS, got %08x\n", status);

    status = pQueryContextAttributesA(&context, SECPKG_ATTR_REMOTE_CERT_CONTEXT, (void*)&cert);
    ok(status == SEC_E_OK, "QueryContextAttributesW(SECPKG_ATTR_REMOTE_CERT_CONTEXT) failed: %08x\n", status);
    if(status == SEC_E_OK) {
        test_remote_cert(cert);
        pCertFreeCertificateContext(cert);
    }

    status = pQueryContextAttributesA(&context, SECPKG_ATTR_CONNECTION_INFO, (void*)&conn_info);
    ok(status == SEC_E_OK, "QueryContextAttributesW(SECPKG_ATTR_CONNECTION_INFO) failed: %08x\n", status);
    if(status == SEC_E_OK) {
        ok(conn_info.dwCipherStrength == 128 || conn_info.dwCipherStrength == 168,
           "conn_info.dwCipherStrength = %d\n", conn_info.dwCipherStrength);
        ok(conn_info.dwHashStrength >= 128, "conn_info.dwHashStrength = %d\n", conn_info.dwHashStrength);
    }

    status = pQueryContextAttributesA(&context, SECPKG_ATTR_STREAM_SIZES, &sizes);
    ok(status == SEC_E_OK, "QueryContextAttributesW(SECPKG_ATTR_STREAM_SIZES) failed: %08x\n", status);

    reset_buffers(&buffers[0]);

    /* Send a simple request so we get data for testing DecryptMessage */
    buf = &buffers[0].pBuffers[0];
    data = buf->pvBuffer;
    buf->BufferType = SECBUFFER_STREAM_HEADER;
    buf->cbBuffer = sizes.cbHeader;
    ++buf;
    buf->BufferType = SECBUFFER_DATA;
    buf->pvBuffer = data + sizes.cbHeader;
    buf->cbBuffer = sizeof(http_request) - 1;
    memcpy(buf->pvBuffer, http_request, sizeof(http_request) - 1);
    ++buf;
    buf->BufferType = SECBUFFER_STREAM_TRAILER;
    buf->pvBuffer = data + sizes.cbHeader + sizeof(http_request) -1;
    buf->cbBuffer = sizes.cbTrailer;

    status = pEncryptMessage(&context, 0, &buffers[0], 0);
    ok(status == SEC_E_OK, "EncryptMessage failed: %08x\n", status);
    if (status != SEC_E_OK)
        return;

    buf = &buffers[0].pBuffers[0];
    send(sock, buf->pvBuffer, buffers[0].pBuffers[0].cbBuffer + buffers[0].pBuffers[1].cbBuffer + buffers[0].pBuffers[2].cbBuffer, 0);

    reset_buffers(&buffers[0]);
    buf->cbBuffer = buf_size;
    data_size = receive_data(sock, buf);

    /* Too few buffers */
    --buffers[0].cBuffers;
    status = pDecryptMessage(&context, &buffers[0], 0, NULL);
    ok(status == SEC_E_INVALID_TOKEN, "Expected SEC_E_INVALID_TOKEN, got %08x\n", status);

    /* No data buffer */
    ++buffers[0].cBuffers;
    status = pDecryptMessage(&context, &buffers[0], 0, NULL);
    ok(status == SEC_E_INVALID_TOKEN, "Expected SEC_E_INVALID_TOKEN, got %08x\n", status);

    /* Two data buffers */
    buffers[0].pBuffers[0].BufferType = SECBUFFER_DATA;
    buffers[0].pBuffers[1].BufferType = SECBUFFER_DATA;
    status = pDecryptMessage(&context, &buffers[0], 0, NULL);
    ok(status == SEC_E_INVALID_TOKEN, "Expected SEC_E_INVALID_TOKEN, got %08x\n", status);

    /* Too few empty buffers */
    buffers[0].pBuffers[1].BufferType = SECBUFFER_EXTRA;
    status = pDecryptMessage(&context, &buffers[0], 0, NULL);
    ok(status == SEC_E_INVALID_TOKEN, "Expected SEC_E_INVALID_TOKEN, got %08x\n", status);

    /* Incomplete data */
    buffers[0].pBuffers[1].BufferType = SECBUFFER_EMPTY;
    buffers[0].pBuffers[0].cbBuffer = (data[3]<<8) | data[4];
    status = pDecryptMessage(&context, &buffers[0], 0, NULL);
    ok(status == SEC_E_INCOMPLETE_MESSAGE, "Expected SEC_E_INCOMPLETE_MESSAGE, got %08x\n", status);
    ok(buffers[0].pBuffers[0].BufferType == SECBUFFER_MISSING, "Expected first buffer to be SECBUFFER_MISSING\n");
    ok(buffers[0].pBuffers[0].cbBuffer == 5, "Expected first buffer to be a five bytes\n");

    buffers[0].pBuffers[0].cbBuffer = data_size;
    buffers[0].pBuffers[0].BufferType = SECBUFFER_DATA;
    buffers[0].pBuffers[1].BufferType = SECBUFFER_EMPTY;
    status = pDecryptMessage(&context, &buffers[0], 0, NULL);
    ok(status == SEC_E_OK, "DecryptMessage failed: %08x\n", status);
    if (status == SEC_E_OK)
    {
        ok(buffers[0].pBuffers[0].BufferType == SECBUFFER_STREAM_HEADER, "Expected first buffer to be SECBUFFER_STREAM_HEADER\n");
        ok(buffers[0].pBuffers[1].BufferType == SECBUFFER_DATA, "Expected second buffer to be SECBUFFER_DATA\n");
        ok(buffers[0].pBuffers[2].BufferType == SECBUFFER_STREAM_TRAILER, "Expected third buffer to be SECBUFFER_STREAM_TRAILER\n");

        data = buffers[0].pBuffers[1].pvBuffer;
        data[buffers[0].pBuffers[1].cbBuffer] = 0;
    }

    pDeleteSecurityContext(&context);
    pFreeCredentialsHandle(&cred_handle);

    free_buffers(&buffers[0]);
    free_buffers(&buffers[1]);

    closesocket(sock);
}

START_TEST(schannel)
{
    InitFunctionPtrs();

    testAcquireSecurityContext();
    test_communication();

    if(secdll)
        FreeLibrary(secdll);
    if(crypt32dll)
        FreeLibrary(crypt32dll);
}
