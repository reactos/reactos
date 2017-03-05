/*
 * Unit test suite for crypt32.dll's CryptStringToBinary and CryptBinaryToString
 * functions.
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
//#include <stdio.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
//#include <winerror.h>
#include <wincrypt.h>

#include <wine/test.h>

#define CERT_HEADER               "-----BEGIN CERTIFICATE-----\r\n"
#define ALT_CERT_HEADER           "-----BEGIN This is some arbitrary text that goes on and on-----\r\n"
#define CERT_TRAILER              "-----END CERTIFICATE-----\r\n"
#define ALT_CERT_TRAILER          "-----END More arbitrary text------\r\n"
#define CERT_REQUEST_HEADER       "-----BEGIN NEW CERTIFICATE REQUEST-----\r\n"
#define CERT_REQUEST_TRAILER      "-----END NEW CERTIFICATE REQUEST-----\r\n"
#define X509_HEADER               "-----BEGIN X509 CRL-----\r\n"
#define X509_TRAILER              "-----END X509 CRL-----\r\n"
#define CERT_HEADER_NOCR          "-----BEGIN CERTIFICATE-----\n"
#define CERT_TRAILER_NOCR         "-----END CERTIFICATE-----\n"
#define CERT_REQUEST_HEADER_NOCR  "-----BEGIN NEW CERTIFICATE REQUEST-----\n"
#define CERT_REQUEST_TRAILER_NOCR "-----END NEW CERTIFICATE REQUEST-----\n"
#define X509_HEADER_NOCR          "-----BEGIN X509 CRL-----\n"
#define X509_TRAILER_NOCR         "-----END X509 CRL-----\n"

static BOOL (WINAPI *pCryptBinaryToStringA)(const BYTE *pbBinary,
 DWORD cbBinary, DWORD dwFlags, LPSTR pszString, DWORD *pcchString);
static BOOL (WINAPI *pCryptStringToBinaryA)(LPCSTR pszString,
 DWORD cchString, DWORD dwFlags, BYTE *pbBinary, DWORD *pcbBinary,
 DWORD *pdwSkip, DWORD *pdwFlags);
static BOOL (WINAPI *pCryptStringToBinaryW)(LPCWSTR pszString,
 DWORD cchString, DWORD dwFlags, BYTE *pbBinary, DWORD *pcbBinary,
 DWORD *pdwSkip, DWORD *pdwFlags);

struct BinTests
{
    const BYTE *toEncode;
    DWORD       toEncodeLen;
    const char *base64;
};

static const BYTE toEncode1[] = { 0 };
static const BYTE toEncode2[] = { 1,2 };
/* static const BYTE toEncode3[] = { 1,2,3 }; */
static const BYTE toEncode4[] =
 "abcdefghijlkmnopqrstuvwxyz01234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890"
 "abcdefghijlkmnopqrstuvwxyz01234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890"
 "abcdefghijlkmnopqrstuvwxyz01234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890";

static const struct BinTests tests[] = {
 { toEncode1, sizeof(toEncode1), "AA==\r\n", },
 { toEncode2, sizeof(toEncode2), "AQI=\r\n", },
 /* { toEncode3, sizeof(toEncode3), "AQID\r\n", },  This test fails on Vista. */
 { toEncode4, sizeof(toEncode4),
   "YWJjZGVmZ2hpamxrbW5vcHFyc3R1dnd4eXowMTIzNDU2Nzg5MEFCQ0RFRkdISUpL\r\n"
   "TE1OT1BRUlNUVVZXWFlaMDEyMzQ1Njc4OTBhYmNkZWZnaGlqbGttbm9wcXJzdHV2\r\n"
   "d3h5ejAxMjM0NTY3ODkwQUJDREVGR0hJSktMTU5PUFFSU1RVVldYWVowMTIzNDU2\r\n"
   "Nzg5MGFiY2RlZmdoaWpsa21ub3BxcnN0dXZ3eHl6MDEyMzQ1Njc4OTBBQkNERUZH\r\n"
   "SElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0NTY3ODkwAA==\r\n" },
};

static const struct BinTests testsNoCR[] = {
 { toEncode1, sizeof(toEncode1), "AA==\n", },
 { toEncode2, sizeof(toEncode2), "AQI=\n", },
 /* { toEncode3, sizeof(toEncode3), "AQID\n", },  This test fails on Vista. */
 { toEncode4, sizeof(toEncode4),
   "YWJjZGVmZ2hpamxrbW5vcHFyc3R1dnd4eXowMTIzNDU2Nzg5MEFCQ0RFRkdISUpL\n"
   "TE1OT1BRUlNUVVZXWFlaMDEyMzQ1Njc4OTBhYmNkZWZnaGlqbGttbm9wcXJzdHV2\n"
   "d3h5ejAxMjM0NTY3ODkwQUJDREVGR0hJSktMTU5PUFFSU1RVVldYWVowMTIzNDU2\n"
   "Nzg5MGFiY2RlZmdoaWpsa21ub3BxcnN0dXZ3eHl6MDEyMzQ1Njc4OTBBQkNERUZH\n"
   "SElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0NTY3ODkwAA==\n" },
};

static void encodeAndCompareBase64_A(const BYTE *toEncode, DWORD toEncodeLen,
 DWORD format, const char *expected, const char *header, const char *trailer)
{
    DWORD strLen = 0;
    LPSTR str = NULL;
    BOOL ret;

    ret = pCryptBinaryToStringA(toEncode, toEncodeLen, format, NULL, &strLen);
    ok(ret, "CryptBinaryToStringA failed: %d\n", GetLastError());
    str = HeapAlloc(GetProcessHeap(), 0, strLen);
    if (str)
    {
        DWORD strLen2 = strLen;
        LPCSTR ptr = str;

        ret = pCryptBinaryToStringA(toEncode, toEncodeLen, format, str,
         &strLen2);
        ok(ret, "CryptBinaryToStringA failed: %d\n", GetLastError());
        ok(strLen2 == strLen - 1, "Expected length %d, got %d\n",
         strLen - 1, strLen);
        if (header)
        {
            ok(!strncmp(header, ptr, strlen(header)),
             "Expected header %s, got %s\n", header, ptr);
            ptr += strlen(header);
        }
        ok(!strncmp(expected, ptr, strlen(expected)),
         "Expected %s, got %s\n", expected, ptr);
        ptr += strlen(expected);
        if (trailer)
            ok(!strncmp(trailer, ptr, strlen(trailer)),
             "Expected trailer %s, got %s\n", trailer, ptr);
        HeapFree(GetProcessHeap(), 0, str);
    }
}

static void testBinaryToStringA(void)
{
    BOOL ret;
    DWORD strLen = 0, i;

    ret = pCryptBinaryToStringA(NULL, 0, 0, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    ret = pCryptBinaryToStringA(NULL, 0, 0, NULL, &strLen);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
    {
        DWORD strLen = 0;
        LPSTR str = NULL;
        BOOL ret;

        ret = pCryptBinaryToStringA(tests[i].toEncode, tests[i].toEncodeLen,
         CRYPT_STRING_BINARY, NULL, &strLen);
        ok(ret, "CryptBinaryToStringA failed: %d\n", GetLastError());
        str = HeapAlloc(GetProcessHeap(), 0, strLen);
        if (str)
        {
            DWORD strLen2 = strLen;

            ret = pCryptBinaryToStringA(tests[i].toEncode, tests[i].toEncodeLen,
             CRYPT_STRING_BINARY, str, &strLen2);
            ok(ret, "CryptBinaryToStringA failed: %d\n", GetLastError());
            ok(strLen == strLen2, "Expected length %d, got %d\n", strLen,
             strLen2);
            ok(!memcmp(str, tests[i].toEncode, tests[i].toEncodeLen),
             "Unexpected value\n");
            HeapFree(GetProcessHeap(), 0, str);
        }
        encodeAndCompareBase64_A(tests[i].toEncode, tests[i].toEncodeLen,
         CRYPT_STRING_BASE64, tests[i].base64, NULL, NULL);
        encodeAndCompareBase64_A(tests[i].toEncode, tests[i].toEncodeLen,
         CRYPT_STRING_BASE64HEADER, tests[i].base64, CERT_HEADER,
         CERT_TRAILER);
        encodeAndCompareBase64_A(tests[i].toEncode, tests[i].toEncodeLen,
         CRYPT_STRING_BASE64REQUESTHEADER, tests[i].base64,
         CERT_REQUEST_HEADER, CERT_REQUEST_TRAILER);
        encodeAndCompareBase64_A(tests[i].toEncode, tests[i].toEncodeLen,
         CRYPT_STRING_BASE64X509CRLHEADER, tests[i].base64, X509_HEADER,
         X509_TRAILER);
    }
    for (i = 0; i < sizeof(testsNoCR) / sizeof(testsNoCR[0]); i++)
    {
        DWORD strLen = 0;
        LPSTR str = NULL;
        BOOL ret;

        ret = pCryptBinaryToStringA(testsNoCR[i].toEncode,
         testsNoCR[i].toEncodeLen, CRYPT_STRING_BINARY | CRYPT_STRING_NOCR,
         NULL, &strLen);
        ok(ret, "CryptBinaryToStringA failed: %d\n", GetLastError());
        str = HeapAlloc(GetProcessHeap(), 0, strLen);
        if (str)
        {
            DWORD strLen2 = strLen;

            ret = pCryptBinaryToStringA(testsNoCR[i].toEncode,
             testsNoCR[i].toEncodeLen, CRYPT_STRING_BINARY | CRYPT_STRING_NOCR,
             str, &strLen2);
            ok(ret, "CryptBinaryToStringA failed: %d\n", GetLastError());
            ok(strLen == strLen2, "Expected length %d, got %d\n", strLen,
             strLen2);
            ok(!memcmp(str, testsNoCR[i].toEncode, testsNoCR[i].toEncodeLen),
             "Unexpected value\n");
            HeapFree(GetProcessHeap(), 0, str);
        }
        encodeAndCompareBase64_A(testsNoCR[i].toEncode,
         testsNoCR[i].toEncodeLen, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCR,
         testsNoCR[i].base64, NULL, NULL);
        encodeAndCompareBase64_A(testsNoCR[i].toEncode,
         testsNoCR[i].toEncodeLen,
         CRYPT_STRING_BASE64HEADER | CRYPT_STRING_NOCR, testsNoCR[i].base64,
         CERT_HEADER_NOCR, CERT_TRAILER_NOCR);
        encodeAndCompareBase64_A(testsNoCR[i].toEncode,
         testsNoCR[i].toEncodeLen,
         CRYPT_STRING_BASE64REQUESTHEADER | CRYPT_STRING_NOCR,
         testsNoCR[i].base64, CERT_REQUEST_HEADER_NOCR,
         CERT_REQUEST_TRAILER_NOCR);
        encodeAndCompareBase64_A(testsNoCR[i].toEncode,
         testsNoCR[i].toEncodeLen,
         CRYPT_STRING_BASE64X509CRLHEADER | CRYPT_STRING_NOCR,
         testsNoCR[i].base64, X509_HEADER_NOCR, X509_TRAILER_NOCR);
    }
}

static void decodeAndCompareBase64_A(LPCSTR toDecode, LPCSTR header,
 LPCSTR trailer, DWORD useFormat, DWORD expectedFormat, const BYTE *expected,
 DWORD expectedLen)
{
    static const char garbage[] = "garbage\r\n";
    LPSTR str;
    DWORD len = strlen(toDecode) + strlen(garbage) + 1;

    if (header)
        len += strlen(header);
    if (trailer)
        len += strlen(trailer);
    str = HeapAlloc(GetProcessHeap(), 0, len);
    if (str)
    {
        LPBYTE buf;
        DWORD bufLen = 0;
        BOOL ret;

        if (header)
            strcpy(str, header);
        else
            *str = 0;
        strcat(str, toDecode);
        if (trailer)
            strcat(str, trailer);
        ret = pCryptStringToBinaryA(str, 0, useFormat, NULL, &bufLen, NULL,
         NULL);
        ok(ret, "CryptStringToBinaryA failed: %d\n", GetLastError());
        buf = HeapAlloc(GetProcessHeap(), 0, bufLen);
        if (buf)
        {
            DWORD skipped, usedFormat;

            /* check as normal, make sure last two parameters are optional */
            ret = pCryptStringToBinaryA(str, 0, useFormat, buf, &bufLen, NULL,
             NULL);
            ok(ret, "CryptStringToBinaryA failed: %d\n", GetLastError());
            ok(bufLen == expectedLen,
             "Expected length %d, got %d\n", expectedLen, bufLen);
            ok(!memcmp(buf, expected, bufLen), "Unexpected value\n");
            /* check last two params */
            ret = pCryptStringToBinaryA(str, 0, useFormat, buf, &bufLen,
             &skipped, &usedFormat);
            ok(ret, "CryptStringToBinaryA failed: %d\n", GetLastError());
            ok(skipped == 0, "Expected skipped 0, got %d\n", skipped);
            ok(usedFormat == expectedFormat, "Expected format %d, got %d\n",
             expectedFormat, usedFormat);
            HeapFree(GetProcessHeap(), 0, buf);
        }

        /* Check again, but with garbage up front */
        strcpy(str, garbage);
        if (header)
            strcat(str, header);
        strcat(str, toDecode);
        if (trailer)
            strcat(str, trailer);
        ret = pCryptStringToBinaryA(str, 0, useFormat, NULL, &bufLen, NULL,
         NULL);
        /* expect failure with no header, and success with one */
        if (header)
            ok(ret, "CryptStringToBinaryA failed: %d\n", GetLastError());
        else
            ok(!ret && GetLastError() == ERROR_INVALID_DATA,
             "Expected !ret and last error ERROR_INVALID_DATA, got ret=%d, error=%d\n", ret, GetLastError());
        if (ret)
        {
            buf = HeapAlloc(GetProcessHeap(), 0, bufLen);
            if (buf)
            {
                DWORD skipped, usedFormat;

                ret = pCryptStringToBinaryA(str, 0, useFormat, buf, &bufLen,
                 &skipped, &usedFormat);
                ok(ret, "CryptStringToBinaryA failed: %d\n", GetLastError());
                ok(skipped == strlen(garbage),
                 "Expected %d characters of \"%s\" skipped when trying format %08x, got %d (used format is %08x)\n",
                 lstrlenA(garbage), str, useFormat, skipped, usedFormat);
                HeapFree(GetProcessHeap(), 0, buf);
            }
        }
        HeapFree(GetProcessHeap(), 0, str);
    }
}

static void decodeBase64WithLenFmtW(LPCSTR strA, int len, DWORD fmt, BOOL retA,
 const BYTE *bufA, DWORD bufLenA, DWORD fmtUsedA)
{
    BYTE buf[8] = {0};
    DWORD bufLen = sizeof(buf)-1, fmtUsed = 0xdeadbeef;
    BOOL ret;
    WCHAR strW[64];
    int i;
    for (i = 0; (strW[i] = strA[i]) != 0; ++i);
    ret = pCryptStringToBinaryW(strW, len, fmt, buf, &bufLen, NULL, &fmtUsed);
    ok(ret == retA && bufLen == bufLenA && memcmp(bufA, buf, bufLen) == 0
     && fmtUsed == fmtUsedA, "base64 \"%s\" len %d: W and A differ\n", strA, len);
}

static void decodeBase64WithLenFmt(LPCSTR str, int len, DWORD fmt, LPCSTR expected, int le, BOOL isBroken)
{
    BYTE buf[8] = {0};
    DWORD bufLen = sizeof(buf)-1, fmtUsed = 0xdeadbeef;
    BOOL ret;
    SetLastError(0xdeadbeef);
    ret = pCryptStringToBinaryA(str, len, fmt, buf, &bufLen, NULL, &fmtUsed);
    buf[bufLen] = 0;
    if (expected) {
        BOOL correct = ret && strcmp(expected, (char*)buf) == 0;
        ok(correct || (isBroken && broken(!ret)),
         "base64 \"%s\" len %d: expected \"%s\", got \"%s\" (ret %d, le %d)\n",
         str, len, expected, (char*)buf, ret, GetLastError());
        if (correct)
            ok(fmtUsed == fmt, "base64 \"%s\" len %d: expected fmt %d, used %d\n",
             str, len, fmt, fmtUsed);
    } else {
        ok(!ret && GetLastError() == le,
         "base64 \"%s\" len %d: expected failure, got \"%s\" (ret %d, le %d)\n",
         str, len, (char*)buf, ret, GetLastError());
    }
    if (pCryptStringToBinaryW)
        decodeBase64WithLenFmtW(str, len, fmt, ret, buf, bufLen, fmtUsed);
}

static void decodeBase64WithLenBroken(LPCSTR str, int len, LPCSTR expected, int le)
{
    decodeBase64WithLenFmt(str, len, CRYPT_STRING_BASE64, expected, le, TRUE);
}

static void decodeBase64WithLen(LPCSTR str, int len, LPCSTR expected, int le)
{
    decodeBase64WithLenFmt(str, len, CRYPT_STRING_BASE64, expected, le, FALSE);
}

static void decodeBase64WithFmt(LPCSTR str, DWORD fmt, LPCSTR expected, int le)
{
    decodeBase64WithLenFmt(str, 0, fmt, expected, le, FALSE);
}

struct BadString
{
    const char *str;
    DWORD       format;
};

static const struct BadString badStrings[] = {
 { "-----BEGIN X509 CRL-----\r\nAA==\r\n", CRYPT_STRING_BASE64X509CRLHEADER },
};

static void testStringToBinaryA(void)
{
    BOOL ret;
    DWORD bufLen = 0, i;
    BYTE buf[8];

    ret = pCryptStringToBinaryA(NULL, 0, 0, NULL, NULL, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got ret=%d le=%u\n", ret, GetLastError());
    ret = pCryptStringToBinaryA(NULL, 0, 0, NULL, &bufLen, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got ret=%d le=%u\n", ret, GetLastError());
    /* Bogus format */
    ret = pCryptStringToBinaryA(tests[0].base64, 0, 0, NULL, &bufLen, NULL,
     NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_DATA,
     "Expected ERROR_INVALID_DATA, got ret=%d le=%u\n", ret, GetLastError());
    /* Decoding doesn't expect the NOCR flag to be specified */
    ret = pCryptStringToBinaryA(tests[0].base64, 1,
     CRYPT_STRING_BASE64 | CRYPT_STRING_NOCR, NULL, &bufLen, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_DATA,
     "Expected ERROR_INVALID_DATA, got ret=%d le=%u\n", ret, GetLastError());
    /* Bad strings */
    for (i = 0; i < sizeof(badStrings) / sizeof(badStrings[0]); i++)
    {
        bufLen = 0;
        ret = pCryptStringToBinaryA(badStrings[i].str, 0, badStrings[i].format,
         NULL, &bufLen, NULL, NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_DATA,
           "%d: Expected ERROR_INVALID_DATA, got ret=%d le=%u\n", i, ret, GetLastError());
    }
    /* Weird base64 strings (invalid padding, extra white-space etc.) */
    decodeBase64WithLen("V=", 0, 0, ERROR_INVALID_DATA);
    decodeBase64WithLen("VV=", 0, 0, ERROR_INVALID_DATA);
    decodeBase64WithLen("V==", 0, 0, ERROR_INVALID_DATA);
    decodeBase64WithLen("V=", 2, 0, ERROR_INVALID_DATA);
    decodeBase64WithLen("VV=", 3, 0, ERROR_INVALID_DATA);
    decodeBase64WithLen("V==", 3, 0, ERROR_INVALID_DATA);
    decodeBase64WithLenBroken("V", 0, "T", 0);
    decodeBase64WithLenBroken("VV", 0, "U", 0);
    decodeBase64WithLenBroken("VVV", 0, "UU", 0);
    decodeBase64WithLen("V", 1, "T", 0);
    decodeBase64WithLen("VV", 2, "U", 0);
    decodeBase64WithLen("VVV", 3, "UU", 0);
    decodeBase64WithLen("V===", 0, "T", 0);
    decodeBase64WithLen("V========", 0, "T", 0);
    decodeBase64WithLen("V===", 4, "T", 0);
    decodeBase64WithLen("V\nVVV", 0, "UUU", 0);
    decodeBase64WithLen("VV\nVV", 0, "UUU", 0);
    decodeBase64WithLen("VVV\nV", 0, "UUU", 0);
    decodeBase64WithLen("V\nVVV", 5, "UUU", 0);
    decodeBase64WithLen("VV\nVV", 5, "UUU", 0);
    decodeBase64WithLen("VVV\nV", 5, "UUU", 0);
    decodeBase64WithLen("VV    VV", 0, "UUU", 0);
    decodeBase64WithLen("V===VVVV", 0, "T", 0);
    decodeBase64WithLen("VV==VVVV", 0, "U", 0);
    decodeBase64WithLen("VVV=VVVV", 0, "UU", 0);
    decodeBase64WithLen("VVVV=VVVV", 0, "UUU", 0);
    decodeBase64WithLen("V===VVVV", 8, "T", 0);
    decodeBase64WithLen("VV==VVVV", 8, "U", 0);
    decodeBase64WithLen("VVV=VVVV", 8, "UU", 0);
    decodeBase64WithLen("VVVV=VVVV", 8, "UUU", 0);

    decodeBase64WithFmt("-----BEGIN-----VVVV-----END-----", CRYPT_STRING_BASE64HEADER, 0, ERROR_INVALID_DATA);
    decodeBase64WithFmt("-----BEGIN-----VVVV-----END -----", CRYPT_STRING_BASE64HEADER, 0, ERROR_INVALID_DATA);
    decodeBase64WithFmt("-----BEGIN -----VVVV-----END-----", CRYPT_STRING_BASE64HEADER, 0, ERROR_INVALID_DATA);
    decodeBase64WithFmt("-----BEGIN -----VVVV-----END -----", CRYPT_STRING_BASE64HEADER, "UUU", 0);

    decodeBase64WithFmt("-----BEGIN -----V-----END -----", CRYPT_STRING_BASE64HEADER, "T", 0);
    decodeBase64WithFmt("-----BEGIN foo-----V-----END -----", CRYPT_STRING_BASE64HEADER, "T", 0);
    decodeBase64WithFmt("-----BEGIN foo-----V-----END foo-----", CRYPT_STRING_BASE64HEADER, "T", 0);
    decodeBase64WithFmt("-----BEGIN -----V-----END foo-----", CRYPT_STRING_BASE64HEADER, "T", 0);
    decodeBase64WithFmt("-----BEGIN -----V-----END -----", CRYPT_STRING_BASE64X509CRLHEADER, "T", 0);
    decodeBase64WithFmt("-----BEGIN foo-----V-----END -----", CRYPT_STRING_BASE64X509CRLHEADER, "T", 0);
    decodeBase64WithFmt("-----BEGIN foo-----V-----END foo-----", CRYPT_STRING_BASE64X509CRLHEADER, "T", 0);
    decodeBase64WithFmt("-----BEGIN -----V-----END foo-----", CRYPT_STRING_BASE64X509CRLHEADER, "T", 0);
    decodeBase64WithFmt("-----BEGIN -----V-----END -----", CRYPT_STRING_BASE64REQUESTHEADER, "T", 0);
    decodeBase64WithFmt("-----BEGIN foo-----V-----END -----", CRYPT_STRING_BASE64REQUESTHEADER, "T", 0);
    decodeBase64WithFmt("-----BEGIN foo-----V-----END foo-----", CRYPT_STRING_BASE64REQUESTHEADER, "T", 0);
    decodeBase64WithFmt("-----BEGIN -----V-----END foo-----", CRYPT_STRING_BASE64REQUESTHEADER, "T", 0);

    /* Too small buffer */
    buf[0] = 0;
    bufLen = 4;
    ret = pCryptStringToBinaryA("VVVVVVVV", 8, CRYPT_STRING_BASE64, (BYTE*)buf, &bufLen, NULL, NULL);
    ok(!ret && bufLen == 4 && buf[0] == 0,
     "Expected ret 0, bufLen 4, buf[0] '\\0', got ret %d, bufLen %d, buf[0] '%c'\n",
     ret, bufLen, buf[0]);

    /* Good strings */
    for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
    {
        bufLen = 0;
        /* Bogus length--oddly enough, that succeeds, even though it's not
         * properly padded.
         */
        ret = pCryptStringToBinaryA(tests[i].base64, 1, CRYPT_STRING_BASE64,
         NULL, &bufLen, NULL, NULL);
        ok(ret, "CryptStringToBinaryA failed: %d\n", GetLastError());
        /* Check with the precise format */
        decodeAndCompareBase64_A(tests[i].base64, NULL, NULL,
         CRYPT_STRING_BASE64, CRYPT_STRING_BASE64, tests[i].toEncode,
         tests[i].toEncodeLen);
        decodeAndCompareBase64_A(tests[i].base64, CERT_HEADER, CERT_TRAILER,
         CRYPT_STRING_BASE64HEADER, CRYPT_STRING_BASE64HEADER,
         tests[i].toEncode, tests[i].toEncodeLen);
        decodeAndCompareBase64_A(tests[i].base64, ALT_CERT_HEADER, ALT_CERT_TRAILER,
         CRYPT_STRING_BASE64HEADER, CRYPT_STRING_BASE64HEADER,
         tests[i].toEncode, tests[i].toEncodeLen);
        decodeAndCompareBase64_A(tests[i].base64, CERT_REQUEST_HEADER,
         CERT_REQUEST_TRAILER, CRYPT_STRING_BASE64REQUESTHEADER,
         CRYPT_STRING_BASE64REQUESTHEADER, tests[i].toEncode,
         tests[i].toEncodeLen);
        decodeAndCompareBase64_A(tests[i].base64, X509_HEADER, X509_TRAILER,
         CRYPT_STRING_BASE64X509CRLHEADER, CRYPT_STRING_BASE64X509CRLHEADER,
         tests[i].toEncode, tests[i].toEncodeLen);
        /* And check with the "any" formats */
        decodeAndCompareBase64_A(tests[i].base64, NULL, NULL,
         CRYPT_STRING_BASE64_ANY, CRYPT_STRING_BASE64, tests[i].toEncode,
         tests[i].toEncodeLen);
        /* Don't check with no header and the string_any format, that'll
         * always succeed.
         */
        decodeAndCompareBase64_A(tests[i].base64, CERT_HEADER, CERT_TRAILER,
         CRYPT_STRING_BASE64_ANY, CRYPT_STRING_BASE64HEADER, tests[i].toEncode,
         tests[i].toEncodeLen);
        decodeAndCompareBase64_A(tests[i].base64, CERT_HEADER, CERT_TRAILER,
         CRYPT_STRING_ANY, CRYPT_STRING_BASE64HEADER, tests[i].toEncode,
         tests[i].toEncodeLen);
        /* oddly, these seem to decode using the wrong format
        decodeAndCompareBase64_A(tests[i].base64, CERT_REQUEST_HEADER,
         CERT_REQUEST_TRAILER, CRYPT_STRING_BASE64_ANY,
         CRYPT_STRING_BASE64REQUESTHEADER, tests[i].toEncode,
         tests[i].toEncodeLen);
        decodeAndCompareBase64_A(tests[i].base64, CERT_REQUEST_HEADER,
         CERT_REQUEST_TRAILER, CRYPT_STRING_ANY,
         CRYPT_STRING_BASE64REQUESTHEADER, tests[i].toEncode,
         tests[i].toEncodeLen);
        decodeAndCompareBase64_A(tests[i].base64, X509_HEADER, X509_TRAILER,
         CRYPT_STRING_BASE64_ANY, CRYPT_STRING_BASE64X509CRLHEADER,
         tests[i].toEncode, tests[i].toEncodeLen);
        decodeAndCompareBase64_A(tests[i].base64, X509_HEADER, X509_TRAILER,
         CRYPT_STRING_ANY, CRYPT_STRING_BASE64X509CRLHEADER, tests[i].toEncode,
         tests[i].toEncodeLen);
         */
    }
    /* And again, with no CR--decoding handles this automatically */
    for (i = 0; i < sizeof(testsNoCR) / sizeof(testsNoCR[0]); i++)
    {
        bufLen = 0;
        /* Bogus length--oddly enough, that succeeds, even though it's not
         * properly padded.
         */
        ret = pCryptStringToBinaryA(testsNoCR[i].base64, 1, CRYPT_STRING_BASE64,
         NULL, &bufLen, NULL, NULL);
        ok(ret, "CryptStringToBinaryA failed: %d\n", GetLastError());
        /* Check with the precise format */
        decodeAndCompareBase64_A(testsNoCR[i].base64, NULL, NULL,
         CRYPT_STRING_BASE64, CRYPT_STRING_BASE64, testsNoCR[i].toEncode,
         testsNoCR[i].toEncodeLen);
        decodeAndCompareBase64_A(testsNoCR[i].base64, CERT_HEADER, CERT_TRAILER,
         CRYPT_STRING_BASE64HEADER, CRYPT_STRING_BASE64HEADER,
         testsNoCR[i].toEncode, testsNoCR[i].toEncodeLen);
        decodeAndCompareBase64_A(testsNoCR[i].base64, CERT_REQUEST_HEADER,
         CERT_REQUEST_TRAILER, CRYPT_STRING_BASE64REQUESTHEADER,
         CRYPT_STRING_BASE64REQUESTHEADER, testsNoCR[i].toEncode,
         testsNoCR[i].toEncodeLen);
        decodeAndCompareBase64_A(testsNoCR[i].base64, X509_HEADER, X509_TRAILER,
         CRYPT_STRING_BASE64X509CRLHEADER, CRYPT_STRING_BASE64X509CRLHEADER,
         testsNoCR[i].toEncode, testsNoCR[i].toEncodeLen);
        /* And check with the "any" formats */
        decodeAndCompareBase64_A(testsNoCR[i].base64, NULL, NULL,
         CRYPT_STRING_BASE64_ANY, CRYPT_STRING_BASE64, testsNoCR[i].toEncode,
         testsNoCR[i].toEncodeLen);
        /* Don't check with no header and the string_any format, that'll
         * always succeed.
         */
        decodeAndCompareBase64_A(testsNoCR[i].base64, CERT_HEADER, CERT_TRAILER,
         CRYPT_STRING_BASE64_ANY, CRYPT_STRING_BASE64HEADER,
         testsNoCR[i].toEncode, testsNoCR[i].toEncodeLen);
        decodeAndCompareBase64_A(testsNoCR[i].base64, CERT_HEADER, CERT_TRAILER,
         CRYPT_STRING_ANY, CRYPT_STRING_BASE64HEADER, testsNoCR[i].toEncode,
         testsNoCR[i].toEncodeLen);
    }
}

START_TEST(base64)
{
    HMODULE lib = GetModuleHandleA("crypt32");

    pCryptBinaryToStringA = (void *)GetProcAddress(lib, "CryptBinaryToStringA");
    pCryptStringToBinaryA = (void *)GetProcAddress(lib, "CryptStringToBinaryA");
    pCryptStringToBinaryW = (void *)GetProcAddress(lib, "CryptStringToBinaryW");

    if (pCryptBinaryToStringA)
        testBinaryToStringA();
    else
        win_skip("CryptBinaryToStringA is not available\n");

    if (pCryptStringToBinaryA)
        testStringToBinaryA();
    else
        win_skip("CryptStringToBinaryA is not available\n");
}
