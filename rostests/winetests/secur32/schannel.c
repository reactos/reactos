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

#include "wine/test.h"

static HMODULE secdll, crypt32dll;

static ACQUIRE_CREDENTIALS_HANDLE_FN_A pAcquireCredentialsHandleA;
static ENUMERATE_SECURITY_PACKAGES_FN_A pEnumerateSecurityPackagesA;
static FREE_CONTEXT_BUFFER_FN pFreeContextBuffer;
static FREE_CREDENTIALS_HANDLE_FN pFreeCredentialsHandle;
static QUERY_CREDENTIALS_ATTRIBUTES_FN_A pQueryCredentialsAttributesA;

static PCCERT_CONTEXT (WINAPI *pCertCreateCertificateContext)(DWORD,const BYTE*,DWORD);
static BOOL (WINAPI *pCertFreeCertificateContext)(PCCERT_CONTEXT);
static BOOL (WINAPI *pCertSetCertificateContextProperty)(PCCERT_CONTEXT,DWORD,DWORD,const void*);

static BOOL (WINAPI *pCryptAcquireContextW)(HCRYPTPROV*, LPCWSTR, LPCWSTR, DWORD, DWORD);
static BOOL (WINAPI *pCryptDestroyKey)(HCRYPTKEY);
static BOOL (WINAPI *pCryptImportKey)(HCRYPTPROV,CONST BYTE*,DWORD,HCRYPTKEY,DWORD,HCRYPTKEY*);
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
    advapi32dll = GetModuleHandleA("advapi32.dll");

#define GET_PROC(h, func)  p ## func = (void*)GetProcAddress(h, #func)

    if(secdll)
    {
        GET_PROC(secdll, AcquireCredentialsHandleA);
        GET_PROC(secdll, EnumerateSecurityPackagesA);
        GET_PROC(secdll, FreeContextBuffer);
        GET_PROC(secdll, FreeCredentialsHandle);
        GET_PROC(secdll, QueryCredentialsAttributesA);
    }

    GET_PROC(advapi32dll, CryptAcquireContextW);
    GET_PROC(advapi32dll, CryptDestroyKey);
    GET_PROC(advapi32dll, CryptImportKey);
    GET_PROC(advapi32dll, CryptReleaseContext);

    GET_PROC(crypt32dll, CertFreeCertificateContext);
    GET_PROC(crypt32dll, CertSetCertificateContextProperty);
    GET_PROC(crypt32dll, CertCreateCertificateContext);

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

static void testAcquireSecurityContext(void)
{
    BOOL has_schannel = FALSE;
    SecPkgInfoA *package_info;
    ULONG i;
    SECURITY_STATUS st;
    CredHandle cred;
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

    pCryptAcquireContextW(&csp, cspNameW, MS_DEF_PROV_W, PROV_RSA_FULL,
     CRYPT_DELETEKEYSET);

    st = pAcquireCredentialsHandleA(NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL,
     NULL);
    ok(st == SEC_E_SECPKG_NOT_FOUND,
     "Expected SEC_E_SECPKG_NOT_FOUND, got %08x\n", st);
    if (0)
    {
        /* Crashes on Win2K */
        st = pAcquireCredentialsHandleA(NULL, unisp_name_a, 0, NULL, NULL, NULL,
         NULL, NULL, NULL);
        ok(st == SEC_E_NO_CREDENTIALS, "Expected SEC_E_NO_CREDENTIALS, got %08x\n",
         st);
    }
    st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_BOTH, NULL,
     NULL, NULL, NULL, NULL, NULL);
    ok(st == SEC_E_NO_CREDENTIALS, "Expected SEC_E_NO_CREDENTIALS, got %08x\n",
     st);
    st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
     NULL, NULL, NULL, NULL, NULL, NULL);
    ok(st == SEC_E_NO_CREDENTIALS, "Expected SEC_E_NO_CREDENTIALS, got %08x\n",
     st);
    if (0)
    {
        /* Crashes */
        st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
         NULL, NULL, NULL, NULL, NULL, NULL);
    }
    st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
     NULL, NULL, NULL, NULL, &cred, NULL);
    ok(st == SEC_E_OK, "AcquireCredentialsHandleA failed: %08x\n", st);
    if(st == SEC_E_OK)
        pFreeCredentialsHandle(&cred);
    memset(&cred, 0, sizeof(cred));
    st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
     NULL, NULL, NULL, NULL, &cred, &exp);
    ok(st == SEC_E_OK, "AcquireCredentialsHandleA failed: %08x\n", st);
    /* expriy is indeterminate in win2k3 */
    trace("expiry: %08x%08x\n", exp.HighPart, exp.LowPart);
    pFreeCredentialsHandle(&cred);

    /* Bad version in SCHANNEL_CRED */
    memset(&schanCred, 0, sizeof(schanCred));
    st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
     NULL, &schanCred, NULL, NULL, NULL, NULL);
    ok(st == SEC_E_INTERNAL_ERROR ||
       st == SEC_E_UNKNOWN_CREDENTIALS /* Vista/win2k8 */,
       "Expected SEC_E_INTERNAL_ERROR or SEC_E_UNKNOWN_CREDENTIALS, got %08x\n", st);
    st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
     NULL, &schanCred, NULL, NULL, NULL, NULL);
    ok(st == SEC_E_INTERNAL_ERROR ||
       st == SEC_E_UNKNOWN_CREDENTIALS /* Vista/win2k8 */,
       "Expected SEC_E_INTERNAL_ERROR or SEC_E_UNKNOWN_CREDENTIALS, got %08x\n", st);

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
        st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
         NULL, &schanCred, NULL, NULL, NULL, NULL);
    }

    /* Bogus cert in SCHANNEL_CRED. Windows fails with
     * SEC_E_UNKNOWN_CREDENTIALS, but I'll accept SEC_E_NO_CREDENTIALS too.
     */
    schanCred.cCreds = 1;
    schanCred.paCred = &certs[0];
    st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
     NULL, &schanCred, NULL, NULL, NULL, NULL);
    ok(st == SEC_E_UNKNOWN_CREDENTIALS || st == SEC_E_NO_CREDENTIALS,
     "Expected SEC_E_UNKNOWN_CREDENTIALS or SEC_E_NO_CREDENTIALS, got %08x\n",
     st);
    st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
     NULL, &schanCred, NULL, NULL, NULL, NULL);
    ok(st == SEC_E_UNKNOWN_CREDENTIALS || st == SEC_E_NO_CREDENTIALS,
     "Expected SEC_E_UNKNOWN_CREDENTIALS or SEC_E_NO_CREDENTIALS, got %08x\n",
     st);

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
        st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
             NULL, &schanCred, NULL, NULL, &cred, NULL);
        ok(st == SEC_E_UNKNOWN_CREDENTIALS,
           "Expected SEC_E_UNKNOWN_CREDENTIALS, got %08x\n", st);
        st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
             NULL, &schanCred, NULL, NULL, &cred, NULL);
        ok(st == SEC_E_UNKNOWN_CREDENTIALS,
           "Expected SEC_E_UNKNOWN_CREDENTIALS, got %08x\n", st);
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
            st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
             NULL, &schanCred, NULL, NULL, NULL, NULL);
        }
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
           st == SEC_E_NO_CREDENTIALS /* Vista/win2k8 */,
           "Expected SEC_E_UNKNOWN_CREDENTIALS or SEC_E_NO_CREDENTIALS, got %08x\n", st);
        st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
         NULL, &schanCred, NULL, NULL, &cred, NULL);
        ok(st == SEC_E_UNKNOWN_CREDENTIALS ||
           st == SEC_E_NO_CREDENTIALS,
           "Expected SEC_E_UNKNOWN_CREDENTIALS, got %08x\n", st);
        tmp = certs[0];
        certs[0] = certs[1];
        certs[1] = tmp;
        st = pAcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
         NULL, &schanCred, NULL, NULL, &cred, NULL);
        ok(st == SEC_E_UNKNOWN_CREDENTIALS ||
           st == SEC_E_NO_CREDENTIALS,
           "Expected SEC_E_UNKNOWN_CREDENTIALS, got %08x\n", st);
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

START_TEST(schannel)
{
    InitFunctionPtrs();

    testAcquireSecurityContext();

    if(secdll)
        FreeLibrary(secdll);
    if(crypt32dll)
        FreeLibrary(crypt32dll);
}
