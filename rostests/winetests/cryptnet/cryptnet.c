/*
 * Unit test suite for cryptnet.dll
 *
 * Copyright 2007 Juan Lang
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
#include <stdarg.h>
#include <stdio.h>
#define NONAMELESSUNION
#include <windef.h>
#include <winbase.h>
#include <winerror.h>
#include <wincrypt.h>
#include "wine/test.h"

static const BYTE bigCert[] = {
0x30,0x78,0x02,0x01,0x01,0x30,0x02,0x06,0x00,0x30,0x14,0x31,0x12,0x30,0x10,
0x06,0x03,0x55,0x04,0x03,0x13,0x09,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,
0x67,0x30,0x22,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,
0x30,0x30,0x30,0x30,0x5a,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,
0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x14,0x31,0x12,0x30,0x10,0x06,0x03,
0x55,0x04,0x03,0x13,0x09,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x30,
0x07,0x30,0x02,0x06,0x00,0x03,0x01,0x00,0xa3,0x16,0x30,0x14,0x30,0x12,0x06,
0x03,0x55,0x1d,0x13,0x01,0x01,0xff,0x04,0x08,0x30,0x06,0x01,0x01,0xff,0x02,
0x01,0x01};
static const BYTE certWithIssuingDistPoint[] = {
0x30,0x81,0x99,0xa0,0x03,0x02,0x01,0x02,0x02,0x01,0x01,0x30,0x0d,0x06,0x09,
0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x05,0x05,0x00,0x30,0x14,0x31,0x12,
0x30,0x10,0x06,0x03,0x55,0x04,0x03,0x13,0x09,0x4a,0x75,0x61,0x6e,0x20,0x4c,
0x61,0x6e,0x67,0x30,0x22,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,
0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,
0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x14,0x31,0x12,0x30,0x10,
0x06,0x03,0x55,0x04,0x03,0x13,0x09,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,
0x67,0x30,0x07,0x30,0x02,0x06,0x00,0x03,0x01,0x00,0xa3,0x27,0x30,0x25,0x30,
0x23,0x06,0x03,0x55,0x1d,0x1c,0x01,0x01,0xff,0x04,0x19,0x30,0x17,0xa0,0x15,
0xa0,0x13,0x86,0x11,0x68,0x74,0x74,0x70,0x3a,0x2f,0x2f,0x77,0x69,0x6e,0x65,
0x68,0x71,0x2e,0x6f,0x72,0x67, };
static const BYTE certWithCRLDistPoint[] = {
0x30,0x81,0x9b,0xa0,0x03,0x02,0x01,0x02,0x02,0x01,0x01,0x30,0x0d,0x06,0x09,
0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x05,0x05,0x00,0x30,0x14,0x31,0x12,
0x30,0x10,0x06,0x03,0x55,0x04,0x03,0x13,0x09,0x4a,0x75,0x61,0x6e,0x20,0x4c,
0x61,0x6e,0x67,0x30,0x22,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,
0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,
0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x14,0x31,0x12,0x30,0x10,
0x06,0x03,0x55,0x04,0x03,0x13,0x09,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,
0x67,0x30,0x07,0x30,0x02,0x06,0x00,0x03,0x01,0x00,0xa3,0x29,0x30,0x27,0x30,
0x25,0x06,0x03,0x55,0x1d,0x1f,0x01,0x01,0xff,0x04,0x1b,0x30,0x19,0x30,0x17,
0xa0,0x15,0xa0,0x13,0x86,0x11,0x68,0x74,0x74,0x70,0x3a,0x2f,0x2f,0x77,0x69,
0x6e,0x65,0x68,0x71,0x2e,0x6f,0x72,0x67, };

static void compareUrlArray(const CRYPT_URL_ARRAY *expected,
 const CRYPT_URL_ARRAY *got)
{
    ok(expected->cUrl == got->cUrl, "Expected %d URLs, got %d\n",
     expected->cUrl, got->cUrl);
    if (expected->cUrl == got->cUrl)
    {
        DWORD i;

        for (i = 0; i < got->cUrl; i++)
            ok(!lstrcmpiW(expected->rgwszUrl[i], got->rgwszUrl[i]),
             "%d: unexpected URL\n", i);
    }
}

static WCHAR url[] =
 { 'h','t','t','p',':','/','/','w','i','n','e','h','q','.','o','r','g',0 };

static void test_getObjectUrl(void)
{
    BOOL ret;
    DWORD urlArraySize = 0, infoSize = 0;
    PCCERT_CONTEXT cert;

    SetLastError(0xdeadbeef);
    ret = CryptGetObjectUrl(NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
    /* Crash
    ret = CryptGetObjectUrl(URL_OID_CERTIFICATE_ISSUER, NULL, 0, NULL, NULL,
     NULL, NULL, NULL);
    ret = CryptGetObjectUrl(URL_OID_CERTIFICATE_ISSUER, NULL, 0, NULL, NULL,
     NULL, &infoSize, NULL);
    ret = CryptGetObjectUrl(URL_OID_CERTIFICATE_ISSUER, NULL, 0, NULL,
     &urlArraySize, NULL, &infoSize, NULL);
     */
    /* A cert with no CRL dist point extension fails.. */
    cert = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert,
     sizeof(bigCert));
    SetLastError(0xdeadbeef);
    ret = CryptGetObjectUrl(URL_OID_CERTIFICATE_ISSUER, (void *)cert, 0, NULL,
     NULL, NULL, NULL, NULL);
    ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
     "Expected CRYPT_E_NOT_FOUND, got %08x\n", GetLastError());
    CertFreeCertificateContext(cert);

    cert = CertCreateCertificateContext(X509_ASN_ENCODING,
     certWithIssuingDistPoint, sizeof(certWithIssuingDistPoint));
    if (cert)
    {
        /* This cert has no AIA extension, so expect this to fail */
        SetLastError(0xdeadbeef);
        ret = CryptGetObjectUrl(URL_OID_CERTIFICATE_ISSUER, (void *)cert, 0,
         NULL, NULL, NULL, NULL, NULL);
        ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
         "Expected CRYPT_E_NOT_FOUND, got %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = CryptGetObjectUrl(URL_OID_CERTIFICATE_ISSUER, (void *)cert,
         CRYPT_GET_URL_FROM_PROPERTY, NULL, NULL, NULL, NULL, NULL);
        ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
         "Expected CRYPT_E_NOT_FOUND, got %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = CryptGetObjectUrl(URL_OID_CERTIFICATE_ISSUER, (void *)cert,
         CRYPT_GET_URL_FROM_EXTENSION, NULL, NULL, NULL, NULL, NULL);
        ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
         "Expected CRYPT_E_NOT_FOUND, got %08x\n", GetLastError());
        /* It does have an issuing dist point extension, but that's not what
         * this is looking for (it wants a CRL dist points extension)
         */
        SetLastError(0xdeadbeef);
        ret = CryptGetObjectUrl(URL_OID_CERTIFICATE_CRL_DIST_POINT,
         (void *)cert, 0, NULL, NULL, NULL, NULL, NULL);
        ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
         "Expected CRYPT_E_NOT_FOUND, got %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = CryptGetObjectUrl(URL_OID_CERTIFICATE_CRL_DIST_POINT,
         (void *)cert, CRYPT_GET_URL_FROM_PROPERTY, NULL, NULL, NULL, NULL,
         NULL);
        ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
         "Expected CRYPT_E_NOT_FOUND, got %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = CryptGetObjectUrl(URL_OID_CERTIFICATE_CRL_DIST_POINT,
         (void *)cert, CRYPT_GET_URL_FROM_EXTENSION, NULL, NULL, NULL, NULL,
         NULL);
        ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
         "Expected CRYPT_E_NOT_FOUND, got %08x\n", GetLastError());
        CertFreeCertificateContext(cert);
    }
    cert = CertCreateCertificateContext(X509_ASN_ENCODING,
     certWithCRLDistPoint, sizeof(certWithCRLDistPoint));
    if (cert)
    {
        PCRYPT_URL_ARRAY urlArray;

        /* This cert has no AIA extension, so expect this to fail */
        SetLastError(0xdeadbeef);
        ret = CryptGetObjectUrl(URL_OID_CERTIFICATE_ISSUER, (void *)cert, 0,
         NULL, NULL, NULL, NULL, NULL);
        ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
         "Expected CRYPT_E_NOT_FOUND, got %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = CryptGetObjectUrl(URL_OID_CERTIFICATE_ISSUER, (void *)cert,
         CRYPT_GET_URL_FROM_PROPERTY, NULL, NULL, NULL, NULL, NULL);
        ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
         "Expected CRYPT_E_NOT_FOUND, got %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = CryptGetObjectUrl(URL_OID_CERTIFICATE_ISSUER, (void *)cert,
         CRYPT_GET_URL_FROM_EXTENSION, NULL, NULL, NULL, NULL, NULL);
        ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
         "Expected CRYPT_E_NOT_FOUND, got %08x\n", GetLastError());
        /* It does have a CRL dist points extension */
        SetLastError(0xdeadbeef);
        ret = CryptGetObjectUrl(URL_OID_CERTIFICATE_CRL_DIST_POINT,
         (void *)cert, 0, NULL, NULL, NULL, NULL, NULL);
        ok(!ret && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = CryptGetObjectUrl(URL_OID_CERTIFICATE_CRL_DIST_POINT,
         (void *)cert, 0, NULL, NULL, NULL, &infoSize, NULL);
        ok(!ret && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08x\n", GetLastError());
        /* Can get it without specifying the location: */
        ret = CryptGetObjectUrl(URL_OID_CERTIFICATE_CRL_DIST_POINT,
         (void *)cert, 0, NULL, &urlArraySize, NULL, NULL, NULL);
        ok(ret, "CryptGetObjectUrl failed: %08x\n", GetLastError());
        urlArray = HeapAlloc(GetProcessHeap(), 0, urlArraySize);
        if (urlArray)
        {
            ret = CryptGetObjectUrl(URL_OID_CERTIFICATE_CRL_DIST_POINT,
             (void *)cert, 0, urlArray, &urlArraySize, NULL, NULL, NULL);
            ok(ret, "CryptGetObjectUrl failed: %08x\n", GetLastError());
            if (ret)
            {
                LPWSTR pUrl = url;
                CRYPT_URL_ARRAY expectedUrl = { 1, &pUrl };

                compareUrlArray(&expectedUrl, urlArray);
            }
            HeapFree(GetProcessHeap(), 0, urlArray);
        }
        /* or by specifying it's an extension: */
        ret = CryptGetObjectUrl(URL_OID_CERTIFICATE_CRL_DIST_POINT,
         (void *)cert, CRYPT_GET_URL_FROM_EXTENSION, NULL, &urlArraySize, NULL,
         NULL, NULL);
        ok(ret, "CryptGetObjectUrl failed: %08x\n", GetLastError());
        urlArray = HeapAlloc(GetProcessHeap(), 0, urlArraySize);
        if (urlArray)
        {
            ret = CryptGetObjectUrl(URL_OID_CERTIFICATE_CRL_DIST_POINT,
             (void *)cert, CRYPT_GET_URL_FROM_EXTENSION, urlArray,
             &urlArraySize, NULL, NULL, NULL);
            ok(ret, "CryptGetObjectUrl failed: %08x\n", GetLastError());
            if (ret)
            {
                LPWSTR pUrl = url;
                CRYPT_URL_ARRAY expectedUrl = { 1, &pUrl };

                compareUrlArray(&expectedUrl, urlArray);
            }
            HeapFree(GetProcessHeap(), 0, urlArray);
        }
        /* but it isn't contained in a property: */
        SetLastError(0xdeadbeef);
        ret = CryptGetObjectUrl(URL_OID_CERTIFICATE_CRL_DIST_POINT,
         (void *)cert, CRYPT_GET_URL_FROM_PROPERTY, NULL, &urlArraySize, NULL,
         NULL, NULL);
        ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
         "Expected CRYPT_E_NOT_FOUND, got %08x\n", GetLastError());
        CertFreeCertificateContext(cert);
    }
}

static void make_tmp_file(LPSTR path)
{
    static char curr[MAX_PATH] = { 0 };
    char temp[MAX_PATH];
    DWORD dwNumberOfBytesWritten;
    HANDLE hf;

    if (!*curr)
        GetCurrentDirectoryA(MAX_PATH, curr);
    GetTempFileNameA(curr, "net", 0, temp);
    lstrcpyA(path, temp);
    hf = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
     FILE_ATTRIBUTE_NORMAL, NULL);
    WriteFile(hf, certWithCRLDistPoint, sizeof(certWithCRLDistPoint),
     &dwNumberOfBytesWritten, NULL);
    CloseHandle(hf);
}

static void test_retrieveObjectByUrl(void)
{
    BOOL ret;
    char tmpfile[MAX_PATH * 2], *ptr, url[MAX_PATH + 8];
    CRYPT_BLOB_ARRAY *pBlobArray;
    PCCERT_CONTEXT cert;
    PCCRL_CONTEXT crl;
    HCERTSTORE store;
    CRYPT_RETRIEVE_AUX_INFO aux = { 0 };
    FILETIME ft = { 0 };

    SetLastError(0xdeadbeef);
    ret = CryptRetrieveObjectByUrlA(NULL, NULL, 0, 0, NULL, NULL, NULL, NULL, NULL);
    ok(!ret && (GetLastError() == ERROR_INVALID_PARAMETER ||
                GetLastError() == E_INVALIDARG),
       "got 0x%x/%u (expected ERROR_INVALID_PARAMETER or E_INVALIDARG)\n",
       GetLastError(), GetLastError());

    make_tmp_file(tmpfile);
    ptr = strchr(tmpfile, ':');
    if (ptr)
        ptr += 2; /* skip colon and first slash */
    else
        ptr = tmpfile;
    snprintf(url, sizeof(url), "file:///%s", ptr);
    do {
        ptr = strchr(url, '\\');
        if (ptr)
            *ptr = '/';
    } while (ptr);

    pBlobArray = (CRYPT_BLOB_ARRAY *)0xdeadbeef;
    ret = CryptRetrieveObjectByUrlA(url, NULL, 0, 0, (void **)&pBlobArray,
     NULL, NULL, NULL, NULL);
    if (!ret)
    {
        /* File URL support was apparently removed in Vista/Windows 2008 */
        win_skip("File URLs not supported\n");
        return;
    }
    ok(ret, "CryptRetrieveObjectByUrlA failed: %d\n", GetLastError());
    ok(pBlobArray && pBlobArray != (CRYPT_BLOB_ARRAY *)0xdeadbeef,
     "Expected a valid pointer\n");
    if (pBlobArray && pBlobArray != (CRYPT_BLOB_ARRAY *)0xdeadbeef)
    {
        ok(pBlobArray->cBlob == 1, "Expected 1 blob, got %d\n",
         pBlobArray->cBlob);
        ok(pBlobArray->rgBlob[0].cbData == sizeof(certWithCRLDistPoint),
         "Unexpected size %d\n", pBlobArray->rgBlob[0].cbData);
        CryptMemFree(pBlobArray);
    }
    cert = (PCCERT_CONTEXT)0xdeadbeef;
    ret = CryptRetrieveObjectByUrlA(url, CONTEXT_OID_CERTIFICATE, 0, 0,
     (void **)&cert, NULL, NULL, NULL, NULL);
    ok(cert && cert != (PCCERT_CONTEXT)0xdeadbeef, "Expected a cert\n");
    if (cert && cert != (PCCERT_CONTEXT)0xdeadbeef)
        CertFreeCertificateContext(cert);
    crl = (PCCRL_CONTEXT)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = CryptRetrieveObjectByUrlA(url, CONTEXT_OID_CRL, 0, 0, (void **)&crl,
     NULL, NULL, NULL, NULL);
    /* w2k3,XP, newer w2k: CRYPT_E_NO_MATCH, 95: OSS_DATA_ERROR */
    ok(!ret && (GetLastError() == CRYPT_E_NO_MATCH ||
                GetLastError() == CRYPT_E_ASN1_BADTAG ||
                GetLastError() == OSS_DATA_ERROR),
       "got 0x%x/%u (expected CRYPT_E_NO_MATCH or CRYPT_E_ASN1_BADTAG or "
       "OSS_DATA_ERROR)\n", GetLastError(), GetLastError());

    /* only newer versions of cryptnet do the cleanup */
    if(!ret && GetLastError() != CRYPT_E_ASN1_BADTAG &&
               GetLastError() != OSS_DATA_ERROR) {
        ok(crl == NULL, "Expected CRL to be NULL\n");
    }

    if (crl && crl != (PCCRL_CONTEXT)0xdeadbeef)
        CertFreeCRLContext(crl);
    store = (HCERTSTORE)0xdeadbeef;
    ret = CryptRetrieveObjectByUrlA(url, CONTEXT_OID_CAPI2_ANY, 0, 0,
     &store, NULL, NULL, NULL, NULL);
    ok(ret, "CryptRetrieveObjectByUrlA failed: %d\n", GetLastError());
    if (store && store != (HCERTSTORE)0xdeadbeef)
    {
        DWORD certs = 0;

        cert = NULL;
        do {
            cert = CertEnumCertificatesInStore(store, cert);
            if (cert)
                certs++;
        } while (cert);
        ok(certs == 1, "Expected 1 cert, got %d\n", certs);
        CertCloseStore(store, 0);
    }
    /* Are file URLs cached? */
    cert = (PCCERT_CONTEXT)0xdeadbeef;
    ret = CryptRetrieveObjectByUrlA(url, CONTEXT_OID_CERTIFICATE,
     CRYPT_CACHE_ONLY_RETRIEVAL, 0, (void **)&cert, NULL, NULL, NULL, NULL);
    ok(ret, "CryptRetrieveObjectByUrlA failed: %08x\n", GetLastError());
    if (cert && cert != (PCCERT_CONTEXT)0xdeadbeef)
        CertFreeCertificateContext(cert);

    cert = (PCCERT_CONTEXT)0xdeadbeef;
    ret = CryptRetrieveObjectByUrlA(url, CONTEXT_OID_CERTIFICATE, 0, 0,
     (void **)&cert, NULL, NULL, NULL, &aux);
    /* w2k: success, 9x: fail with E_INVALIDARG */
    ok(ret || (GetLastError() == E_INVALIDARG),
       "got %u with 0x%x/%u (expected '!=0' or '0' with E_INVALIDARG)\n",
       ret, GetLastError(), GetLastError());
    if (cert && cert != (PCCERT_CONTEXT)0xdeadbeef)
        CertFreeCertificateContext(cert);

    cert = (PCCERT_CONTEXT)0xdeadbeef;
    aux.cbSize = sizeof(aux);
    ret = CryptRetrieveObjectByUrlA(url, CONTEXT_OID_CERTIFICATE, 0, 0,
     (void **)&cert, NULL, NULL, NULL, &aux);
    /* w2k: success, 9x: fail with E_INVALIDARG */
    ok(ret || (GetLastError() == E_INVALIDARG),
       "got %u with 0x%x/%u (expected '!=0' or '0' with E_INVALIDARG)\n",
       ret, GetLastError(), GetLastError());
    if (!ret) {
        /* no more tests useful */
        DeleteFileA(tmpfile);
        skip("no usable CertificateContext\n");
        return;
    }

    aux.pLastSyncTime = &ft;
    ret = CryptRetrieveObjectByUrlA(url, CONTEXT_OID_CERTIFICATE, 0, 0,
     (void **)&cert, NULL, NULL, NULL, &aux);
    ok(ft.dwLowDateTime || ft.dwHighDateTime,
     "Expected last sync time to be set\n");
    DeleteFileA(tmpfile);
    /* Okay, after being deleted, are file URLs still cached? */
    SetLastError(0xdeadbeef);
    ret = CryptRetrieveObjectByUrlA(url, CONTEXT_OID_CERTIFICATE,
     CRYPT_CACHE_ONLY_RETRIEVAL, 0, (void **)&cert, NULL, NULL, NULL, NULL);
    ok(!ret && (GetLastError() == ERROR_FILE_NOT_FOUND ||
     GetLastError() == ERROR_PATH_NOT_FOUND),
     "Expected ERROR_FILE_NOT_FOUND or ERROR_PATH_NOT_FOUND, got %d\n",
     GetLastError());
}

START_TEST(cryptnet)
{
    test_getObjectUrl();
    test_retrieveObjectByUrl();
}
