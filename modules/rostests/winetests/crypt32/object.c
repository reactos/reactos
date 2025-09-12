/*
 * Unit test suite for crypt32.dll's Crypt*Object functions
 *
 * Copyright 2008 Juan Lang
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
#include <winerror.h>
#include <wincrypt.h>

#include "wine/test.h"

static BOOL (WINAPI * pCryptQueryObject)(DWORD, const void *, DWORD, DWORD,
 DWORD, DWORD *, DWORD *, DWORD *, HCERTSTORE *, HCRYPTMSG *, const void **);

static BYTE bigCert[] = {
0x30,0x7a,0x02,0x01,0x01,0x30,0x02,0x06,0x00,0x30,0x15,0x31,
0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,
0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00,0x30,0x22,0x18,0x0f,
0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,
0x30,0x30,0x5a,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,
0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x15,0x31,0x13,
0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,
0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00,0x30,0x07,0x30,0x02,0x06,
0x00,0x03,0x01,0x00,0xa3,0x16,0x30,0x14,0x30,0x12,0x06,0x03,
0x55,0x1d,0x13,0x01,0x01,0xff,0x04,0x08,0x30,0x06,0x01,0x01,
0xff,0x02,0x01,0x01 };
static char bigCertBase64[] =
"MHoCAQEwAgYAMBUxEzARBgNVBAMTCkp1YW4gTGFuZwAwIhgPMTYwMTAxMDEwMDAw\n"
"MDBaGA8xNjAxMDEwMTAwMDAwMFowFTETMBEGA1UEAxMKSnVhbiBMYW5nADAHMAIG\n"
"AAMBAKMWMBQwEgYDVR0TAQH/BAgwBgEB/wIBAQ==\n";
static WCHAR bigCertBase64W[] = {
'M','H','o','C','A','Q','E','w','A','g','Y','A','M','B','U','x','E','z','A',
'R','B','g','N','V','B','A','M','T','C','k','p','1','Y','W','4','g','T','G',
'F','u','Z','w','A','w','I','h','g','P','M','T','Y','w','M','T','A','x','M',
'D','E','w',',','D','A','w','\n',
'M','D','B','a','G','A','8','x','N','j','A','x','M','D','E','w','M','T','A',
'w','M','D','A','w','M','F','o','w','F','T','E','T','M','B','E','G','A','1',
'U','E','A','x','M','K','S','n','V','h','b','i','B','M','Y','W','5','n','A',
'D','A','H','M','A','I','G','\n',
'A','A','M','B','A','K','M','W','M','B','Q','w','E','g','Y','D','V','R','0',
'T','A','Q','H','/','B','A','g','w','B','g','E','B','/','w','I','B','A','Q',
'=','=','\n',0 };
static BYTE signedWithCertWithValidPubKeyContent[] = {
0x30,0x82,0x01,0x89,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x07,0x02,
0xa0,0x82,0x01,0x7a,0x30,0x82,0x01,0x76,0x02,0x01,0x01,0x31,0x0e,0x30,0x0c,
0x06,0x08,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x02,0x05,0x05,0x00,0x30,0x13,0x06,
0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x07,0x01,0xa0,0x06,0x04,0x04,0x01,
0x02,0x03,0x04,0xa0,0x81,0xd2,0x30,0x81,0xcf,0x02,0x01,0x01,0x30,0x02,0x06,
0x00,0x30,0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,
0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00,0x30,0x22,0x18,0x0f,0x31,0x36,
0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x18,0x0f,
0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,
0x30,0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,
0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00,0x30,0x5c,0x30,0x0d,0x06,0x09,0x2a,
0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x01,0x05,0x00,0x03,0x4b,0x00,0x30,0x48,
0x02,0x41,0x00,0xe2,0x54,0x3a,0xa7,0x83,0xb1,0x27,0x14,0x3e,0x59,0xbb,0xb4,
0x53,0xe6,0x1f,0xe7,0x5d,0xf1,0x21,0x68,0xad,0x85,0x53,0xdb,0x6b,0x1e,0xeb,
0x65,0x97,0x03,0x86,0x60,0xde,0xf3,0x6c,0x38,0x75,0xe0,0x4c,0x61,0xbb,0xbc,
0x62,0x17,0xa9,0xcd,0x79,0x3f,0x21,0x4e,0x96,0xcb,0x0e,0xdc,0x61,0x94,0x30,
0x18,0x10,0x6b,0xd0,0x1c,0x10,0x79,0x02,0x03,0x01,0x00,0x01,0xa3,0x16,0x30,
0x14,0x30,0x12,0x06,0x03,0x55,0x1d,0x13,0x01,0x01,0xff,0x04,0x08,0x30,0x06,
0x01,0x01,0xff,0x02,0x01,0x01,0x31,0x77,0x30,0x75,0x02,0x01,0x01,0x30,0x1a,
0x30,0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,
0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00,0x02,0x01,0x01,0x30,0x0c,0x06,0x08,
0x2a,0x86,0x48,0x86,0xf7,0x0d,0x02,0x05,0x05,0x00,0x30,0x04,0x06,0x00,0x05,
0x00,0x04,0x40,0x81,0xa6,0x70,0xb3,0xef,0x59,0xd1,0x66,0xd1,0x9b,0xc0,0x9a,
0xb6,0x9a,0x5e,0x6d,0x6f,0x6d,0x0d,0x59,0xa9,0xaa,0x6e,0xe9,0x2c,0xa0,0x1e,
0xee,0xc2,0x60,0xbc,0x59,0xbe,0x3f,0x63,0x06,0x8d,0xc9,0x11,0x1d,0x23,0x64,
0x92,0xef,0x2e,0xfc,0x57,0x29,0xa4,0xaf,0xe0,0xee,0x93,0x19,0x39,0x51,0xe4,
0x44,0xb8,0x0b,0x28,0xf4,0xa8,0x0d };
static char signedWithCertWithValidPubKeyContentBase64[] =
"MIIBiQYJKoZIhvcNAQcCoIIBejCCAXYCAQExDjAMBggqhkiG9w0CBQUAMBMGCSqG"
"SIb3DQEHAaAGBAQBAgMEoIHSMIHPAgEBMAIGADAVMRMwEQYDVQQDEwpKdWFuIExh"
"bmcAMCIYDzE2MDEwMTAxMDAwMDAwWhgPMTYwMTAxMDEwMDAwMDBaMBUxEzARBgNV"
"BAMTCkp1YW4gTGFuZwAwXDANBgkqhkiG9w0BAQEFAANLADBIAkEA4lQ6p4OxJxQ+"
"Wbu0U+Yf513xIWithVPbax7rZZcDhmDe82w4deBMYbu8YhepzXk/IU6Wyw7cYZQw"
"GBBr0BwQeQIDAQABoxYwFDASBgNVHRMBAf8ECDAGAQH/AgEBMXcwdQIBATAaMBUx"
"EzARBgNVBAMTCkp1YW4gTGFuZwACAQEwDAYIKoZIhvcNAgUFADAEBgAFAARAgaZw"
"s+9Z0WbRm8CatppebW9tDVmpqm7pLKAe7sJgvFm+P2MGjckRHSNkku8u/FcppK/g"
"7pMZOVHkRLgLKPSoDQ==";
static WCHAR signedWithCertWithValidPubKeyContentBase64W[] = {
'M','I','I','B','i','Q','Y','J','K','o','Z','I','h','v','c','N','A','Q','c','C',
'o','I','I','B','e','j','C','C','A','X','Y','C','A','Q','E','x','D','j','A','M',
'B','g','g','q','h','k','i','G','9','w','0','C','B','Q','U','A','M','B','M','G',
'C','S','q','G','S','I','b','3','D','Q','E','H','A','a','A','G','B','A','Q','B',
'A','g','M','E','o','I','H','S','M','I','H','P','A','g','E','B','M','A','I','G',
'A','D','A','V','M','R','M','w','E','Q','Y','D','V','Q','Q','D','E','w','p','K',
'd','W','F','u','I','E','x','h','b','m','c','A','M','C','I','Y','D','z','E','2',
'M','D','E','w','M','T','A','x','M','D','A','w','M','D','A','w','W','h','g','P',
'M','T','Y','w','M','T','A','x','M','D','E','w','M','D','A','w','M','D','B','a',
'M','B','U','x','E','z','A','R','B','g','N','V','B','A','M','T','C','k','p','1',
'Y','W','4','g','T','G','F','u','Z','w','A','w','X','D','A','N','B','g','k','q',
'h','k','i','G','9','w','0','B','A','Q','E','F','A','A','N','L','A','D','B','I',
'A','k','E','A','4','l','Q','6','p','4','O','x','J','x','Q','+','W','b','u','0',
'U','+','Y','f','5','1','3','x','I','W','i','t','h','V','P','b','a','x','7','r',
'Z','Z','c','D','h','m','D','e','8','2','w','4','d','e','B','M','Y','b','u','8',
'Y','h','e','p','z','X','k','/','I','U','6','W','y','w','7','c','Y','Z','Q','w',
'G','B','B','r','0','B','w','Q','e','Q','I','D','A','Q','A','B','o','x','Y','w',
'F','D','A','S','B','g','N','V','H','R','M','B','A','f','8','E','C','D','A','G',
'A','Q','H','/','A','g','E','B','M','X','c','w','d','Q','I','B','A','T','A','a',
'M','B','U','x','E','z','A','R','B','g','N','V','B','A','M','T','C','k','p','1',
'Y','W','4','g','T','G','F','u','Z','w','A','C','A','Q','E','w','D','A','Y','I',
'K','o','Z','I','h','v','c','N','A','g','U','F','A','D','A','E','B','g','A','F',
'A','A','R','A','g','a','Z','w','s','+','9','Z','0','W','b','R','m','8','C','a',
't','p','p','e','b','W','9','t','D','V','m','p','q','m','7','p','L','K','A','e',
'7','s','J','g','v','F','m','+','P','2','M','G','j','c','k','R','H','S','N','k',
'k','u','8','u','/','F','c','p','p','K','/','g','7','p','M','Z','O','V','H','k',
'R','L','g','L','K','P','S','o','D','Q','=','=',0 };

static void test_query_object(void)
{
    BOOL ret;
    CRYPT_DATA_BLOB blob;

    /* Test the usual invalid arguments */
    SetLastError(0xdeadbeef);
    ret = pCryptQueryObject(0, NULL, 0, 0, 0, NULL, NULL, NULL, NULL, NULL,
     NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "expected E_INVALIDARG, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pCryptQueryObject(CERT_QUERY_OBJECT_BLOB, NULL, 0, 0, 0, NULL, NULL,
     NULL, NULL, NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "expected E_INVALIDARG, got %08x\n", GetLastError());
    /* Test with a simple cert */
    blob.pbData = bigCert;
    blob.cbData = sizeof(bigCert);
    SetLastError(0xdeadbeef);
    ret = pCryptQueryObject(CERT_QUERY_OBJECT_BLOB, &blob,
     CERT_QUERY_CONTENT_FLAG_ALL, CERT_QUERY_FORMAT_FLAG_ALL, 0, NULL, NULL,
     NULL, NULL, NULL, NULL);
    ok(ret, "CryptQueryObject failed: %08x\n", GetLastError());
    /* The same cert, base64-encoded */
    blob.pbData = (BYTE *)bigCertBase64;
    blob.cbData = sizeof(bigCertBase64);
    SetLastError(0xdeadbeef);
    ret = pCryptQueryObject(CERT_QUERY_OBJECT_BLOB, &blob,
     CERT_QUERY_CONTENT_FLAG_ALL, CERT_QUERY_FORMAT_FLAG_ALL, 0, NULL, NULL,
     NULL, NULL, NULL, NULL);
    ok(ret, "CryptQueryObject failed: %08x\n", GetLastError());
    /* The same base64-encoded cert, restricting the format types */
    SetLastError(0xdeadbeef);
    ret = pCryptQueryObject(CERT_QUERY_OBJECT_BLOB, &blob,
     CERT_QUERY_CONTENT_FLAG_ALL, CERT_QUERY_FORMAT_FLAG_BINARY, 0, NULL, NULL,
     NULL, NULL, NULL, NULL);
    ok(!ret && GetLastError() == CRYPT_E_NO_MATCH,
     "expected CRYPT_E_NO_MATCH, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pCryptQueryObject(CERT_QUERY_OBJECT_BLOB, &blob,
     CERT_QUERY_CONTENT_FLAG_ALL, CERT_QUERY_FORMAT_FLAG_BASE64_ENCODED, 0,
     NULL, NULL, NULL, NULL, NULL, NULL);
    ok(ret, "CryptQueryObject failed: %08x\n", GetLastError());
    /* The same cert, base64-encoded but as a wide character string */
    blob.pbData = (BYTE *)bigCertBase64W;
    blob.cbData = sizeof(bigCertBase64W);
    SetLastError(0xdeadbeef);
    ret = pCryptQueryObject(CERT_QUERY_OBJECT_BLOB, &blob,
     CERT_QUERY_CONTENT_FLAG_ALL, CERT_QUERY_FORMAT_FLAG_ALL, 0, NULL, NULL,
     NULL, NULL, NULL, NULL);
    ok(!ret && GetLastError() == CRYPT_E_NO_MATCH,
     "expected CRYPT_E_NO_MATCH, got %08x\n", GetLastError());
    /* For brevity, not tested here, but tested on Windows:  same failure
     * (CRYPT_E_NO_MATCH) when the wide character base64-encoded cert
     * is written to a file and queried.
     */
    /* Test with a valid signed message */
    blob.pbData = signedWithCertWithValidPubKeyContent;
    blob.cbData = sizeof(signedWithCertWithValidPubKeyContent);
    SetLastError(0xdeadbeef);
    ret = pCryptQueryObject(CERT_QUERY_OBJECT_BLOB, &blob,
     CERT_QUERY_CONTENT_FLAG_ALL, CERT_QUERY_FORMAT_FLAG_ALL, 0, NULL, NULL,
     NULL, NULL, NULL, NULL);
    ok(ret, "CryptQueryObject failed: %08x\n", GetLastError());
    blob.pbData = (BYTE *)signedWithCertWithValidPubKeyContentBase64;
    blob.cbData = sizeof(signedWithCertWithValidPubKeyContentBase64);
    SetLastError(0xdeadbeef);
    ret = pCryptQueryObject(CERT_QUERY_OBJECT_BLOB, &blob,
     CERT_QUERY_CONTENT_FLAG_ALL, CERT_QUERY_FORMAT_FLAG_ALL, 0, NULL, NULL,
     NULL, NULL, NULL, NULL);
    ok(ret, "CryptQueryObject failed: %08x\n", GetLastError());
    /* A valid signed message, encoded as a wide character base64 string, can
     * be queried successfully.
     */
    blob.pbData = (BYTE *)signedWithCertWithValidPubKeyContentBase64W;
    blob.cbData = sizeof(signedWithCertWithValidPubKeyContentBase64W);
    SetLastError(0xdeadbeef);
    ret = pCryptQueryObject(CERT_QUERY_OBJECT_BLOB, &blob,
     CERT_QUERY_CONTENT_FLAG_ALL, CERT_QUERY_FORMAT_FLAG_ALL, 0, NULL, NULL,
     NULL, NULL, NULL, NULL);
    ok(ret, "CryptQueryObject failed: %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pCryptQueryObject(CERT_QUERY_OBJECT_BLOB, &blob,
     CERT_QUERY_CONTENT_FLAG_ALL, CERT_QUERY_FORMAT_FLAG_BINARY, 0, NULL, NULL,
     NULL, NULL, NULL, NULL);
    ok(!ret && GetLastError() == CRYPT_E_NO_MATCH,
     "expected CRYPT_E_NO_MATCH, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pCryptQueryObject(CERT_QUERY_OBJECT_BLOB, &blob,
     CERT_QUERY_CONTENT_FLAG_ALL, CERT_QUERY_FORMAT_FLAG_BASE64_ENCODED, 0,
     NULL, NULL, NULL, NULL, NULL, NULL);
    ok(ret, "CryptQueryObject failed: %08x\n", GetLastError());
}

START_TEST(object)
{
    HMODULE mod = GetModuleHandleA("crypt32.dll");

    pCryptQueryObject = (void *)GetProcAddress(mod, "CryptQueryObject");

    if (!pCryptQueryObject)
    {
        win_skip("CryptQueryObject is not available\n");
        return;
    }

    test_query_object();
}
