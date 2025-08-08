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
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <wincrypt.h>

#include "wine/test.h"

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
static const BYTE toEncode5[] =
 "abcdefghijlkmnopqrstuvwxyz01234567890ABCDEFGHI";

static const BYTE toEncode6[] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

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
 { toEncode5, sizeof(toEncode5),
   "YWJjZGVmZ2hpamxrbW5vcHFyc3R1dnd4eXowMTIzNDU2Nzg5MEFCQ0RFRkdISQA=\r\n" },
 { toEncode6, sizeof(toEncode6),
   "YWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFh\r\n"
   "YQA=\r\n" },
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
 { toEncode5, sizeof(toEncode5),
   "YWJjZGVmZ2hpamxrbW5vcHFyc3R1dnd4eXowMTIzNDU2Nzg5MEFCQ0RFRkdISQA=\n" },
 { toEncode6, sizeof(toEncode6),
   "YWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFh\n"
   "YQA=\n" },
};

static WCHAR *strdupAtoW(const char *str)
{
    WCHAR *ret = NULL;
    DWORD len;

    if (!str) return ret;
    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    ret = malloc(len * sizeof(WCHAR));
    if (ret)
        MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    return ret;
}

static void encodeAndCompareBase64_A(const BYTE *toEncode, DWORD toEncodeLen,
 DWORD format, const char *expected, const char *header, const char *trailer)
{
    DWORD strLen, strLen2, required;
    const char *ptr;
    LPSTR str = NULL;
    BOOL ret;

    required = strlen(expected) + 1;
    if (header)
        required += strlen(header);
    if (trailer)
        required += strlen(trailer);

    strLen = 0;
    ret = CryptBinaryToStringA(toEncode, toEncodeLen, format, NULL, &strLen);
    ok(ret, "CryptBinaryToStringA failed: %ld\n", GetLastError());
    ok(strLen == required, "Unexpected required length %lu, expected %lu.\n", required, strLen);

    strLen2 = strLen;
    ret = CryptBinaryToStringA(toEncode, toEncodeLen, format, NULL, &strLen2);
    ok(ret, "CryptBinaryToStringA failed: %ld\n", GetLastError());
    ok(strLen == strLen2, "Unexpected required length %lu, expected %lu.\n", strLen2, strLen);

    strLen2 = strLen - 1;
    ret = CryptBinaryToStringA(toEncode, toEncodeLen, format, NULL, &strLen2);
    ok(ret, "CryptBinaryToStringA failed: %ld\n", GetLastError());
    ok(strLen == strLen2, "Unexpected required length %lu, expected %lu.\n", strLen2, strLen);

    str = malloc(strLen);

    /* Partially filled output buffer. */
    strLen2 = strLen - 1;
    str[0] = 0x12;
    ret = CryptBinaryToStringA(toEncode, toEncodeLen, format, str, &strLen2);
    ok((!ret && GetLastError() == ERROR_MORE_DATA) || broken(ret) /* XP */, "CryptBinaryToStringA failed %d, error %ld.\n",
        ret, GetLastError());
    ok(strLen2 == strLen || broken(strLen2 == strLen - 1), "Expected length %ld, got %ld\n", strLen, strLen2);
    if (header)
        ok(str[0] == header[0], "Unexpected buffer contents %#x.\n", str[0]);
    else
        ok(str[0] == expected[0], "Unexpected buffer contents %#x.\n", str[0]);
    strLen2 = strLen;
    ret = CryptBinaryToStringA(toEncode, toEncodeLen, format, str, &strLen2);
    ok(ret, "CryptBinaryToStringA failed: %ld\n", GetLastError());
    ok(strLen2 == strLen - 1, "Expected length %ld, got %ld\n", strLen - 1, strLen2);

    ptr = str;
    if (header)
    {
        ok(!strncmp(header, ptr, strlen(header)), "Expected header %s, got %s\n", header, ptr);
        ptr += strlen(header);
    }
    ok(!strncmp(expected, ptr, strlen(expected)), "Expected %s, got %s\n", expected, ptr);
    ptr += strlen(expected);
    if (trailer)
        ok(!strncmp(trailer, ptr, strlen(trailer)), "Expected trailer %s, got %s\n", trailer, ptr);

    free(str);
}

static void encode_compare_base64_W(const BYTE *toEncode, DWORD toEncodeLen, DWORD format,
        const WCHAR *expected, const char *header, const char *trailer)
{
    WCHAR *headerW, *trailerW, required;
    DWORD strLen, strLen2;
    WCHAR *strW = NULL;
    const WCHAR *ptr;
    BOOL ret;

    required = lstrlenW(expected) + 1;
    if (header)
        required += strlen(header);
    if (trailer)
        required += strlen(trailer);

    strLen = 0;
    ret = CryptBinaryToStringW(toEncode, toEncodeLen, format, NULL, &strLen);
    ok(ret, "CryptBinaryToStringW failed: %ld\n", GetLastError());
    ok(strLen == required, "Unexpected required length %lu, expected %u.\n", strLen, required);

    /* Same call with non-zero length value. */
    strLen2 = strLen;
    ret = CryptBinaryToStringW(toEncode, toEncodeLen, format, NULL, &strLen2);
    ok(ret, "CryptBinaryToStringW failed: %ld\n", GetLastError());
    ok(strLen == strLen2, "Unexpected required length.\n");

    strLen2 = strLen - 1;
    ret = CryptBinaryToStringW(toEncode, toEncodeLen, format, NULL, &strLen2);
    ok(ret, "CryptBinaryToStringW failed: %ld\n", GetLastError());
    ok(strLen == strLen2, "Unexpected required length.\n");

    strLen2 = strLen - 1;
    ret = CryptBinaryToStringW(toEncode, toEncodeLen, format, NULL, &strLen2);
    ok(ret, "CryptBinaryToStringW failed: %ld\n", GetLastError());
    ok(strLen == strLen2, "Unexpected required length.\n");

    strW = malloc(strLen * sizeof(WCHAR));

    headerW = strdupAtoW(header);
    trailerW = strdupAtoW(trailer);

    strLen2 = strLen - 1;
    strW[0] = 0x1234;
    ret = CryptBinaryToStringW(toEncode, toEncodeLen, format, strW, &strLen2);
    ok((!ret && GetLastError() == ERROR_MORE_DATA) || broken(ret) /* XP */, "CryptBinaryToStringW failed, %d, error %ld\n",
        ret, GetLastError());
    if (headerW)
        ok(strW[0] == 0x1234, "Unexpected buffer contents %#x.\n", strW[0]);
    else
        ok(strW[0] == 0x1234 || broken(strW[0] != 0x1234) /* XP */, "Unexpected buffer contents %#x.\n", strW[0]);

    strLen2 = strLen;
    ret = CryptBinaryToStringW(toEncode, toEncodeLen, format, strW, &strLen2);
    ok(ret, "CryptBinaryToStringW failed: %ld\n", GetLastError());

    ok(strLen2 == strLen - 1, "Expected length %ld, got %ld\n", strLen - 1, strLen);

    ptr = strW;
    if (headerW)
    {
        ok(!memcmp(headerW, ptr, lstrlenW(headerW)), "Expected header %s, got %s.\n", wine_dbgstr_w(headerW),
                wine_dbgstr_w(ptr));
        ptr += lstrlenW(headerW);
    }
    ok(!memcmp(expected, ptr, lstrlenW(expected)), "Expected %s, got %s.\n", wine_dbgstr_w(expected),
            wine_dbgstr_w(ptr));
    ptr += lstrlenW(expected);
    if (trailerW)
        ok(!memcmp(trailerW, ptr, lstrlenW(trailerW)), "Expected trailer %s, got %s.\n", wine_dbgstr_w(trailerW),
                wine_dbgstr_w(ptr));

    free(strW);
    free(headerW);
    free(trailerW);
}

static DWORD binary_to_hex_len(DWORD binary_len, DWORD flags)
{
    DWORD strLen2;

    strLen2 = binary_len * 3; /* spaces + terminating \0 */

    if (flags & CRYPT_STRING_NOCR)
    {
        strLen2 += (binary_len + 7) / 16; /* space every 16 characters */
        strLen2 += 1; /* terminating \n */
    }
    else if (!(flags & CRYPT_STRING_NOCRLF))
    {
        strLen2 += (binary_len + 7) / 16; /* space every 16 characters */
        strLen2 += binary_len / 16 + 1; /* LF every 16 characters + terminating \r */

        if (binary_len % 16)
            strLen2 += 1; /* terminating \n */
    }

    return strLen2;
}

static void test_CryptBinaryToString(void)
{
    static const DWORD flags[] = { 0, CRYPT_STRING_NOCR, CRYPT_STRING_NOCRLF };
    static const DWORD sizes[] = { 3, 4, 7, 8, 12, 15, 16, 17, 256 };
    static const WCHAR hexdig[] = L"0123456789abcdef";
    BYTE input[256 * sizeof(WCHAR)];
    DWORD strLen, strLen2, i, j, k;
    WCHAR *hex, *cmp, *ptr;
    char *hex_a, *cmp_a;
    BOOL ret;

    ret = CryptBinaryToStringA(NULL, 0, 0, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    strLen = 123;
    ret = CryptBinaryToStringA(NULL, 0, 0, NULL, &strLen);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
    ok(strLen == 123, "Unexpected length.\n");

    if (0)
        ret = CryptBinaryToStringW(NULL, 0, 0, NULL, NULL);

    strLen = 123;
    ret = CryptBinaryToStringW(NULL, 0, 0, NULL, &strLen);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "Unexpected error %ld\n", GetLastError());
    ok(strLen == 123, "Unexpected length.\n");

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        WCHAR *strW, *encodedW;
        LPSTR str = NULL;
        BOOL ret;

        strLen = 0;
        ret = CryptBinaryToStringA(tests[i].toEncode, tests[i].toEncodeLen, CRYPT_STRING_BINARY, NULL, &strLen);
        ok(ret, "CryptBinaryToStringA failed: %ld\n", GetLastError());
        ok(strLen == tests[i].toEncodeLen, "Unexpected required length %lu.\n", strLen);

        strLen2 = strLen;
        str = malloc(strLen);
        ret = CryptBinaryToStringA(tests[i].toEncode, tests[i].toEncodeLen, CRYPT_STRING_BINARY, str, &strLen2);
        ok(ret, "CryptBinaryToStringA failed: %ld\n", GetLastError());
        ok(strLen == strLen2, "Expected length %lu, got %lu\n", strLen, strLen2);
        ok(!memcmp(str, tests[i].toEncode, tests[i].toEncodeLen), "Unexpected value\n");
        free(str);

        strLen = 0;
        ret = CryptBinaryToStringW(tests[i].toEncode, tests[i].toEncodeLen, CRYPT_STRING_BINARY, NULL, &strLen);
        ok(ret, "CryptBinaryToStringW failed: %ld\n", GetLastError());
        ok(strLen == tests[i].toEncodeLen, "Unexpected required length %lu.\n", strLen);

        strLen2 = strLen;
        strW = malloc(strLen);
        ret = CryptBinaryToStringW(tests[i].toEncode, tests[i].toEncodeLen, CRYPT_STRING_BINARY, strW, &strLen2);
        ok(ret, "CryptBinaryToStringW failed: %ld\n", GetLastError());
        ok(strLen == strLen2, "Expected length %lu, got %lu\n", strLen, strLen2);
        ok(!memcmp(strW, tests[i].toEncode, tests[i].toEncodeLen), "Unexpected value\n");
        free(strW);

        encodeAndCompareBase64_A(tests[i].toEncode, tests[i].toEncodeLen, CRYPT_STRING_BASE64,
            tests[i].base64, NULL, NULL);
        encodeAndCompareBase64_A(tests[i].toEncode, tests[i].toEncodeLen, CRYPT_STRING_BASE64HEADER,
            tests[i].base64, CERT_HEADER, CERT_TRAILER);
        encodeAndCompareBase64_A(tests[i].toEncode, tests[i].toEncodeLen, CRYPT_STRING_BASE64REQUESTHEADER,
            tests[i].base64, CERT_REQUEST_HEADER, CERT_REQUEST_TRAILER);
        encodeAndCompareBase64_A(tests[i].toEncode, tests[i].toEncodeLen, CRYPT_STRING_BASE64X509CRLHEADER,
            tests[i].base64, X509_HEADER, X509_TRAILER);

        encodedW = strdupAtoW(tests[i].base64);

        encode_compare_base64_W(tests[i].toEncode, tests[i].toEncodeLen, CRYPT_STRING_BASE64, encodedW, NULL, NULL);
        encode_compare_base64_W(tests[i].toEncode, tests[i].toEncodeLen, CRYPT_STRING_BASE64HEADER, encodedW,
            CERT_HEADER, CERT_TRAILER);
        encode_compare_base64_W(tests[i].toEncode, tests[i].toEncodeLen, CRYPT_STRING_BASE64REQUESTHEADER,
            encodedW, CERT_REQUEST_HEADER, CERT_REQUEST_TRAILER);
        encode_compare_base64_W(tests[i].toEncode, tests[i].toEncodeLen, CRYPT_STRING_BASE64X509CRLHEADER, encodedW,
            X509_HEADER, X509_TRAILER);

        free(encodedW);
    }

    for (i = 0; i < ARRAY_SIZE(testsNoCR); i++)
    {
        LPSTR str = NULL;
        WCHAR *encodedW;
        BOOL ret;

        ret = CryptBinaryToStringA(testsNoCR[i].toEncode, testsNoCR[i].toEncodeLen,
            CRYPT_STRING_BINARY | CRYPT_STRING_NOCR, NULL, &strLen);
        ok(ret, "CryptBinaryToStringA failed: %ld\n", GetLastError());

        strLen2 = strLen;
        str = malloc(strLen);
        ret = CryptBinaryToStringA(testsNoCR[i].toEncode, testsNoCR[i].toEncodeLen,
            CRYPT_STRING_BINARY | CRYPT_STRING_NOCR, str, &strLen2);
        ok(ret, "CryptBinaryToStringA failed: %ld\n", GetLastError());
        ok(strLen == strLen2, "Expected length %ld, got %ld\n", strLen, strLen2);
        ok(!memcmp(str, testsNoCR[i].toEncode, testsNoCR[i].toEncodeLen), "Unexpected value\n");
        free(str);

        encodeAndCompareBase64_A(testsNoCR[i].toEncode, testsNoCR[i].toEncodeLen, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCR,
            testsNoCR[i].base64, NULL, NULL);
        encodeAndCompareBase64_A(testsNoCR[i].toEncode, testsNoCR[i].toEncodeLen,
            CRYPT_STRING_BASE64HEADER | CRYPT_STRING_NOCR, testsNoCR[i].base64, CERT_HEADER_NOCR, CERT_TRAILER_NOCR);
        encodeAndCompareBase64_A(testsNoCR[i].toEncode, testsNoCR[i].toEncodeLen,
            CRYPT_STRING_BASE64REQUESTHEADER | CRYPT_STRING_NOCR, testsNoCR[i].base64, CERT_REQUEST_HEADER_NOCR,
            CERT_REQUEST_TRAILER_NOCR);
        encodeAndCompareBase64_A(testsNoCR[i].toEncode, testsNoCR[i].toEncodeLen,
            CRYPT_STRING_BASE64X509CRLHEADER | CRYPT_STRING_NOCR, testsNoCR[i].base64, X509_HEADER_NOCR, X509_TRAILER_NOCR);

        encodedW = strdupAtoW(testsNoCR[i].base64);

        encode_compare_base64_W(testsNoCR[i].toEncode, testsNoCR[i].toEncodeLen,
            CRYPT_STRING_BASE64 | CRYPT_STRING_NOCR, encodedW, NULL, NULL);
        encode_compare_base64_W(testsNoCR[i].toEncode, testsNoCR[i].toEncodeLen,
            CRYPT_STRING_BASE64HEADER | CRYPT_STRING_NOCR, encodedW, CERT_HEADER_NOCR, CERT_TRAILER_NOCR);
        encode_compare_base64_W(testsNoCR[i].toEncode, testsNoCR[i].toEncodeLen,
            CRYPT_STRING_BASE64REQUESTHEADER | CRYPT_STRING_NOCR, encodedW, CERT_REQUEST_HEADER_NOCR,
            CERT_REQUEST_TRAILER_NOCR);
        encode_compare_base64_W(testsNoCR[i].toEncode, testsNoCR[i].toEncodeLen,
            CRYPT_STRING_BASE64X509CRLHEADER | CRYPT_STRING_NOCR, encodedW,
            X509_HEADER_NOCR, X509_TRAILER_NOCR);

        free(encodedW);
    }

    /* Systems that don't support HEXRAW format convert to BASE64 instead - 3 bytes in -> 4 chars + crlf + 1 null out. */
    strLen = 0;
    ret = CryptBinaryToStringW(input, 3, CRYPT_STRING_HEXRAW, NULL, &strLen);
    ok(ret, "Failed to get string length.\n");
    ok(strLen == 9 || broken(strLen == 7), "Unexpected string length %ld.\n", strLen);
    if (strLen == 7)
    {
        win_skip("CryptBinaryToString(HEXRAW) not supported\n");
        return;
    }

    for (i = 0; i < sizeof(input) / sizeof(WCHAR); i++)
        ((WCHAR *)input)[i] = i;

    for (i = 0; i < ARRAY_SIZE(flags); i++)
    {
        winetest_push_context("i %lu", i);
        strLen = 0;
        ret = CryptBinaryToStringW(input, sizeof(input), CRYPT_STRING_HEXRAW|flags[i], NULL, &strLen);
        ok(ret, "CryptBinaryToStringW failed: %ld\n", GetLastError());
        ok(strLen > 0, "Unexpected string length.\n");

        strLen = 0;
        ret = CryptBinaryToStringA(input, sizeof(input), CRYPT_STRING_HEXRAW|flags[i], NULL, &strLen);
        ok(ret, "failed, error %ld.\n", GetLastError());
        ok(strLen > 0, "Unexpected string length.\n");

        strLen = ~0;
        ret = CryptBinaryToStringW(input, sizeof(input), CRYPT_STRING_HEXRAW|flags[i],
                                   NULL, &strLen);
        ok(ret, "CryptBinaryToStringW failed: %ld\n", GetLastError());
        if (flags[i] & CRYPT_STRING_NOCRLF)
            strLen2 = 0;
        else if (flags[i] & CRYPT_STRING_NOCR)
            strLen2 = 1;
        else
            strLen2 = 2;
        strLen2 += sizeof(input) * 2 + 1;
        ok(strLen == strLen2, "Expected length %ld, got %ld\n", strLen2, strLen);

        hex = malloc(strLen * sizeof(WCHAR));
        hex_a = malloc(strLen);

        memset(hex, 0xcc, strLen * sizeof(WCHAR));
        ptr = cmp = malloc(strLen * sizeof(WCHAR));
        cmp_a = malloc(strLen);
        for (j = 0; j < ARRAY_SIZE(input); j++)
        {
            *ptr++ = hexdig[(input[j] >> 4) & 0xf];
            *ptr++ = hexdig[input[j] & 0xf];
        }
        if (flags[i] & CRYPT_STRING_NOCR)
        {
            *ptr++ = '\n';
        }
        else if (!(flags[i] & CRYPT_STRING_NOCRLF))
        {
            *ptr++ = '\r';
            *ptr++ = '\n';
        }
        *ptr++ = 0;

        for (j = 0; cmp[j]; ++j)
            cmp_a[j] = cmp[j];
        cmp_a[j] = 0;

        ret = CryptBinaryToStringW(input, sizeof(input), CRYPT_STRING_HEXRAW|flags[i],
                                   hex, &strLen);
        ok(ret, "CryptBinaryToStringW failed: %ld\n", GetLastError());
        strLen2--;
        ok(strLen == strLen2, "Expected length %ld, got %ld\n", strLen, strLen2);
        ok(!memcmp(hex, cmp, strLen * sizeof(WCHAR)), "Unexpected value\n");

        ++strLen;
        ret = CryptBinaryToStringA(input, sizeof(input), CRYPT_STRING_HEXRAW | flags[i],
                                   hex_a, &strLen);
        ok(ret, "failed, error %ld.\n", GetLastError());
        ok(strLen == strLen2, "Expected length %ld, got %ld.\n", strLen, strLen2);
        ok(!memcmp(hex_a, cmp_a, strLen), "Unexpected value.\n");

        /* adjusts size if buffer too big */
        strLen *= 2;
        ret = CryptBinaryToStringW(input, sizeof(input), CRYPT_STRING_HEXRAW|flags[i],
                                   hex, &strLen);
        ok(ret, "CryptBinaryToStringW failed: %ld\n", GetLastError());
        ok(strLen == strLen2, "Expected length %ld, got %ld\n", strLen, strLen2);

        strLen *= 2;
        ret = CryptBinaryToStringA(input, sizeof(input), CRYPT_STRING_HEXRAW|flags[i],
                                   hex_a, &strLen);
        ok(ret, "failed, error %ld.\n", GetLastError());
        ok(strLen == strLen2, "Expected length %ld, got %ld.\n", strLen, strLen2);

        /* no writes if buffer too small */
        strLen /= 2;
        strLen2 /= 2;
        memset(hex, 0xcc, strLen * sizeof(WCHAR));
        memset(cmp, 0xcc, strLen * sizeof(WCHAR));
        SetLastError(0xdeadbeef);
        ret = CryptBinaryToStringW(input, sizeof(input), CRYPT_STRING_HEXRAW|flags[i],
                                   hex, &strLen);
        ok(!ret && GetLastError() == ERROR_MORE_DATA,"Expected ERROR_MORE_DATA, got ret=%d le=%lu\n",
           ret, GetLastError());
        ok(strLen == strLen2, "Expected length %ld, got %ld\n", strLen, strLen2);
        ok(!memcmp(hex, cmp, strLen * sizeof(WCHAR)), "Unexpected value\n");

        SetLastError(0xdeadbeef);
        memset(hex_a, 0xcc, strLen + 3);
        ret = CryptBinaryToStringA(input, sizeof(input), CRYPT_STRING_HEXRAW | flags[i],
                                   hex_a, &strLen);
        ok(!ret && GetLastError() == ERROR_MORE_DATA,"got ret %d, error %lu.\n", ret, GetLastError());
        ok(strLen == strLen2, "Expected length %ld, got %ld.\n", strLen2, strLen);
        /* Output consists of the number of full bytes which fit in plus terminating 0. */
        strLen = (strLen - 1) & ~1;
        ok(!memcmp(hex_a, cmp_a, strLen), "Unexpected value\n");
        ok(!hex_a[strLen], "got %#x.\n", (unsigned char)hex_a[strLen]);
        ok((unsigned char)hex_a[strLen + 1] == 0xcc, "got %#x.\n", (unsigned char)hex_a[strLen + 1]);

        /* Output is not filled if string length is less than 3. */
        strLen = 1;
        memset(hex_a, 0xcc, strLen2);
        ret = CryptBinaryToStringA(input, sizeof(input), CRYPT_STRING_HEXRAW | flags[i],
                                   hex_a, &strLen);
        ok(strLen == 1, "got %ld.\n", strLen);
        ok((unsigned char)hex_a[0] == 0xcc, "got %#x.\n", (unsigned char)hex_a[strLen - 1]);

        strLen = 2;
        memset(hex_a, 0xcc, strLen2);
        ret = CryptBinaryToStringA(input, sizeof(input), CRYPT_STRING_HEXRAW | flags[i],
                                   hex_a, &strLen);
        ok(strLen == 2, "got %ld.\n", strLen);
        ok((unsigned char)hex_a[0] == 0xcc, "got %#x.\n", (unsigned char)hex_a[0]);
        ok((unsigned char)hex_a[1] == 0xcc, "got %#x.\n", (unsigned char)hex_a[1]);

        strLen = 3;
        memset(hex_a, 0xcc, strLen2);
        ret = CryptBinaryToStringA(input, sizeof(input), CRYPT_STRING_HEXRAW | flags[i],
                                   hex_a, &strLen);
        ok(strLen == 3, "got %ld.\n", strLen);
        ok(hex_a[0] == 0x30, "got %#x.\n", (unsigned char)hex_a[0]);
        ok(hex_a[1] == 0x30, "got %#x.\n", (unsigned char)hex_a[1]);
        ok(!hex_a[2], "got %#x.\n", (unsigned char)hex_a[2]);

        free(hex);
        free(hex_a);
        free(cmp);
        free(cmp_a);

        winetest_pop_context();
    }

    for (k = 0; k < ARRAY_SIZE(sizes); k++)
    for (i = 0; i < ARRAY_SIZE(flags); i++)
    {
        strLen = 0;
        ret = CryptBinaryToStringW(input, sizes[k], CRYPT_STRING_HEX | flags[i], NULL, &strLen);
        ok(ret, "CryptBinaryToStringW failed: %ld\n", GetLastError());
        ok(strLen > 0, "Unexpected string length.\n");

        strLen = ~0;
        ret = CryptBinaryToStringW(input, sizes[k], CRYPT_STRING_HEX | flags[i], NULL, &strLen);
        ok(ret, "CryptBinaryToStringW failed: %ld\n", GetLastError());
        strLen2 = binary_to_hex_len(sizes[k], CRYPT_STRING_HEX | flags[i]);
        ok(strLen == strLen2, "%lu: Expected length %ld, got %ld\n", i, strLen2, strLen);

        hex = malloc(strLen * sizeof(WCHAR) + 256);
        memset(hex, 0xcc, strLen * sizeof(WCHAR));

        ptr = cmp = malloc(strLen * sizeof(WCHAR) + 256);
        for (j = 0; j < sizes[k]; j++)
        {
            *ptr++ = hexdig[(input[j] >> 4) & 0xf];
            *ptr++ = hexdig[input[j] & 0xf];

            if (j >= sizes[k] - 1) break;

            if (j && !(flags[i] & CRYPT_STRING_NOCRLF))
            {

                if (!((j + 1) % 16))
                {
                    if (flags[i] & CRYPT_STRING_NOCR)
                    {
                        *ptr++ = '\n';
                    }
                    else
                    {
                        *ptr++ = '\r';
                        *ptr++ = '\n';
                    }
                    continue;
                }
                else if (!((j + 1) % 8))
                    *ptr++ = ' ';
            }

            *ptr++ = ' ';
        }

        if (flags[i] & CRYPT_STRING_NOCR)
        {
            *ptr++ = '\n';
        }
        else if (!(flags[i] & CRYPT_STRING_NOCRLF))
        {
            *ptr++ = '\r';
            *ptr++ = '\n';
        }
        *ptr++ = 0;

        ret = CryptBinaryToStringW(input, sizes[k], CRYPT_STRING_HEX | flags[i], hex, &strLen);
        ok(ret, "CryptBinaryToStringW failed: %ld\n", GetLastError());
        strLen2--;
        ok(strLen == strLen2, "%lu: Expected length %ld, got %ld\n", i, strLen, strLen2);
        ok(!memcmp(hex, cmp, strLen * sizeof(WCHAR)), "%lu: got %s\n", i, wine_dbgstr_wn(hex, strLen));

        /* adjusts size if buffer too big */
        strLen *= 2;
        ret = CryptBinaryToStringW(input, sizes[k], CRYPT_STRING_HEX | flags[i], hex, &strLen);
        ok(ret, "CryptBinaryToStringW failed: %ld\n", GetLastError());
        ok(strLen == strLen2, "%lu: Expected length %ld, got %ld\n", i, strLen, strLen2);

        /* no writes if buffer too small */
        strLen /= 2;
        strLen2 /= 2;
        memset(hex, 0xcc, strLen * sizeof(WCHAR));
        memset(cmp, 0xcc, strLen * sizeof(WCHAR));
        SetLastError(0xdeadbeef);
        ret = CryptBinaryToStringW(input, sizes[k], CRYPT_STRING_HEX | flags[i], hex, &strLen);
        ok(!ret && GetLastError() == ERROR_MORE_DATA,"Expected ERROR_MORE_DATA, got ret=%d le=%lu\n",
           ret, GetLastError());
        ok(strLen == strLen2, "%lu: Expected length %ld, got %ld\n", i, strLen, strLen2);
        ok(!memcmp(hex, cmp, strLen * sizeof(WCHAR)), "%lu: got %s\n", i, wine_dbgstr_wn(hex, strLen));

        free(hex);
        free(cmp);
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
    str = malloc(len);
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
        ret = CryptStringToBinaryA(str, 0, useFormat, NULL, &bufLen, NULL,
         NULL);
        ok(ret, "CryptStringToBinaryA failed: %ld\n", GetLastError());
        buf = malloc(bufLen);
        if (buf)
        {
            DWORD skipped, usedFormat;

            /* check as normal, make sure last two parameters are optional */
            ret = CryptStringToBinaryA(str, 0, useFormat, buf, &bufLen, NULL,
             NULL);
            ok(ret, "CryptStringToBinaryA failed: %ld\n", GetLastError());
            ok(bufLen == expectedLen,
             "Expected length %ld, got %ld\n", expectedLen, bufLen);
            ok(!memcmp(buf, expected, bufLen), "Unexpected value\n");
            /* check last two params */
            ret = CryptStringToBinaryA(str, 0, useFormat, buf, &bufLen,
             &skipped, &usedFormat);
            ok(ret, "CryptStringToBinaryA failed: %ld\n", GetLastError());
            ok(skipped == 0, "Expected skipped 0, got %ld\n", skipped);
            ok(usedFormat == expectedFormat, "Expected format %ld, got %ld\n",
             expectedFormat, usedFormat);
            free(buf);
        }

        /* Check again, but with garbage up front */
        strcpy(str, garbage);
        if (header)
            strcat(str, header);
        strcat(str, toDecode);
        if (trailer)
            strcat(str, trailer);
        ret = CryptStringToBinaryA(str, 0, useFormat, NULL, &bufLen, NULL,
         NULL);
        /* expect failure with no header, and success with one */
        if (header)
            ok(ret, "CryptStringToBinaryA failed: %ld\n", GetLastError());
        else
            ok(!ret && GetLastError() == ERROR_INVALID_DATA,
             "Expected !ret and last error ERROR_INVALID_DATA, got ret=%d, error=%ld\n", ret, GetLastError());
        if (ret)
        {
            buf = malloc(bufLen);
            if (buf)
            {
                DWORD skipped, usedFormat;

                ret = CryptStringToBinaryA(str, 0, useFormat, buf, &bufLen,
                 &skipped, &usedFormat);
                ok(ret, "CryptStringToBinaryA failed: %ld\n", GetLastError());
                ok(skipped == strlen(garbage),
                 "Expected %d characters of \"%s\" skipped when trying format %08lx, got %ld (used format is %08lx)\n",
                 lstrlenA(garbage), str, useFormat, skipped, usedFormat);
                free(buf);
            }
        }
        free(str);
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
    ret = CryptStringToBinaryW(strW, len, fmt, buf, &bufLen, NULL, &fmtUsed);
    ok(ret == retA && bufLen == bufLenA && memcmp(bufA, buf, bufLen) == 0
     && fmtUsed == fmtUsedA, "base64 \"%s\" len %d: W and A differ\n", strA, len);
}

static void decodeBase64WithLenFmt(LPCSTR str, int len, DWORD fmt, LPCSTR expected, int le, BOOL isBroken)
{
    BYTE buf[8] = {0};
    DWORD bufLen = sizeof(buf)-1, fmtUsed = 0xdeadbeef;
    BOOL ret;
    SetLastError(0xdeadbeef);
    ret = CryptStringToBinaryA(str, len, fmt, buf, &bufLen, NULL, &fmtUsed);
    buf[bufLen] = 0;
    if (expected) {
        BOOL correct = ret && strcmp(expected, (char*)buf) == 0;
        ok(correct || (isBroken && broken(!ret)),
         "base64 \"%s\" len %d: expected \"%s\", got \"%s\" (ret %d, le %ld)\n",
         str, len, expected, (char*)buf, ret, GetLastError());
        if (correct)
            ok(fmtUsed == fmt, "base64 \"%s\" len %d: expected fmt %ld, used %ld\n",
             str, len, fmt, fmtUsed);
    } else {
        ok(!ret && GetLastError() == le,
         "base64 \"%s\" len %d: expected failure, got \"%s\" (ret %d, le %ld)\n",
         str, len, (char*)buf, ret, GetLastError());
    }

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

static BOOL is_hex_string_special_char(WCHAR c)
{
    switch (c)
    {
        case '-':
        case ',':
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            return TRUE;

        default:
            return FALSE;
    }
}

static WCHAR wchar_from_str(BOOL wide, const void **str, DWORD *len)
{
    WCHAR c;

    if (!*len)
        return 0;

    --*len;
    if (wide)
        c = *(*(const WCHAR **)str)++;
    else
        c = *(*(const char **)str)++;

    return c ? c : 0xffff;
}

static BYTE digit_from_char(WCHAR c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    c = towlower(c);
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 0xa;
    return 0xff;
}

static LONG string_to_hex(const void* str, BOOL wide, DWORD len, BYTE *hex, DWORD *hex_len,
        DWORD *skipped, DWORD *ret_flags)
{
    unsigned int byte_idx = 0;
    BYTE d1, d2;
    WCHAR c;

    if (!str || !hex_len)
        return ERROR_INVALID_PARAMETER;

    if (!len)
        len = wide ? wcslen(str) : strlen(str);

    if (wide && !len)
        return ERROR_INVALID_PARAMETER;

    if (skipped)
        *skipped = 0;
    if (ret_flags)
        *ret_flags = 0;

    while ((c = wchar_from_str(wide, &str, &len)) && is_hex_string_special_char(c))
        ;

    while ((d1 = digit_from_char(c)) != 0xff)
    {
        if ((d2 = digit_from_char(wchar_from_str(wide, &str, &len))) == 0xff)
        {
            if (!hex)
                *hex_len = 0;
            return ERROR_INVALID_DATA;
        }

        if (hex && byte_idx < *hex_len)
            hex[byte_idx] = (d1 << 4) | d2;

        ++byte_idx;

        do
        {
            c = wchar_from_str(wide, &str, &len);
        } while (c == '-' || c == ',');
    }

    while (c)
    {
        if (!is_hex_string_special_char(c))
        {
            if (!hex)
                *hex_len = 0;
            return ERROR_INVALID_DATA;
        }
        c = wchar_from_str(wide, &str, &len);
    }

    if (hex && byte_idx > *hex_len)
        return ERROR_MORE_DATA;

    if (ret_flags)
        *ret_flags = CRYPT_STRING_HEX;

    *hex_len = byte_idx;

    return ERROR_SUCCESS;
}

static void test_CryptStringToBinary(void)
{
    static const char *string_hex_tests[] =
    {
        "",
        "-",
        ",-",
        "0",
        "00",
        "000",
        "11220",
        "1122q",
        "q1122",
        " aE\t\n\r\n",
        "01-02",
        "-,01-02",
        "01-02-",
        "aa,BB-ff,-,",
        "1-2",
        "010-02",
        "aa,BBff,-,",
        "aa,,-BB---ff,-,",
        "010203040506070809q",
    };

    DWORD skipped, flags, expected_err, expected_len, expected_skipped, expected_flags;
    BYTE buf[8], expected[8];
    DWORD bufLen = 0, i;
    WCHAR str_w[64];
    BOOL ret, wide;

    ret = CryptStringToBinaryA(NULL, 0, 0, NULL, NULL, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got ret=%d le=%lu\n", ret, GetLastError());
    ret = CryptStringToBinaryA(NULL, 0, 0, NULL, &bufLen, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got ret=%d le=%lu\n", ret, GetLastError());
    /* Bogus format */
    ret = CryptStringToBinaryA(tests[0].base64, 0, 0, NULL, &bufLen, NULL,
     NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_DATA,
     "Expected ERROR_INVALID_DATA, got ret=%d le=%lu\n", ret, GetLastError());
    /* Decoding doesn't expect the NOCR flag to be specified */
    ret = CryptStringToBinaryA(tests[0].base64, 1,
     CRYPT_STRING_BASE64 | CRYPT_STRING_NOCR, NULL, &bufLen, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_DATA,
     "Expected ERROR_INVALID_DATA, got ret=%d le=%lu\n", ret, GetLastError());
    /* Bad strings */
    for (i = 0; i < ARRAY_SIZE(badStrings); i++)
    {
        bufLen = 0;
        ret = CryptStringToBinaryA(badStrings[i].str, 0, badStrings[i].format,
         NULL, &bufLen, NULL, NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_DATA,
           "%ld: Expected ERROR_INVALID_DATA, got ret=%d le=%lu\n", i, ret, GetLastError());
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
    ret = CryptStringToBinaryA("VVVVVVVV", 8, CRYPT_STRING_BASE64, (BYTE*)buf, &bufLen, NULL, NULL);
    ok(!ret && bufLen == 4 && buf[0] == 0,
     "Expected ret 0, bufLen 4, buf[0] '\\0', got ret %d, bufLen %ld, buf[0] '%c'\n",
     ret, bufLen, buf[0]);

    /* Good strings */
    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        bufLen = 0;
        /* Bogus length--oddly enough, that succeeds, even though it's not
         * properly padded.
         */
        ret = CryptStringToBinaryA(tests[i].base64, 1, CRYPT_STRING_BASE64,
         NULL, &bufLen, NULL, NULL);
        ok(ret, "CryptStringToBinaryA failed: %ld\n", GetLastError());
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
    for (i = 0; i < ARRAY_SIZE(testsNoCR); i++)
    {
        bufLen = 0;
        /* Bogus length--oddly enough, that succeeds, even though it's not
         * properly padded.
         */
        ret = CryptStringToBinaryA(testsNoCR[i].base64, 1, CRYPT_STRING_BASE64,
         NULL, &bufLen, NULL, NULL);
        ok(ret, "CryptStringToBinaryA failed: %ld\n", GetLastError());
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

    /* CRYPT_STRING_HEX */

    ret = CryptStringToBinaryW(L"01", 2, CRYPT_STRING_HEX, NULL, NULL, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "got ret %d, error %lu.\n", ret, GetLastError());
    if (0)
    {
        /* access violation on Windows. */
        CryptStringToBinaryA("01", 2, CRYPT_STRING_HEX, NULL, NULL, NULL, NULL);
    }

    bufLen = 8;
    ret = CryptStringToBinaryW(L"0102", 2, CRYPT_STRING_HEX, NULL, &bufLen, NULL, NULL);
    ok(ret, "got error %lu.\n", GetLastError());
    ok(bufLen == 1, "got length %lu.\n", bufLen);

    bufLen = 8;
    ret = CryptStringToBinaryW(NULL, 0, CRYPT_STRING_HEX, NULL, &bufLen, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "got ret %d, error %lu.\n", ret, GetLastError());
    ok(bufLen == 8, "got length %lu.\n", bufLen);

    bufLen = 8;
    ret = CryptStringToBinaryA(NULL, 0, CRYPT_STRING_HEX, NULL, &bufLen, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "got ret %d, error %lu.\n", ret, GetLastError());
    ok(bufLen == 8, "got length %lu.\n", bufLen);

    bufLen = 8;
    ret = CryptStringToBinaryW(L"0102", 3, CRYPT_STRING_HEX, NULL, &bufLen, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_DATA, "got ret %d, error %lu.\n", ret, GetLastError());
    ok(!bufLen, "got length %lu.\n", bufLen);

    bufLen = 8;
    buf[0] = 0xcc;
    ret = CryptStringToBinaryW(L"0102", 3, CRYPT_STRING_HEX, buf, &bufLen, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_DATA, "got ret %d, error %lu.\n", ret, GetLastError());
    ok(bufLen == 8, "got length %lu.\n", bufLen);
    ok(buf[0] == 1, "got buf[0] %#x.\n", buf[0]);

    bufLen = 8;
    buf[0] = 0xcc;
    ret = CryptStringToBinaryW(L"0102", 2, CRYPT_STRING_HEX, buf, &bufLen, NULL, NULL);
    ok(ret, "got error %lu.\n", GetLastError());
    ok(bufLen == 1, "got length %lu.\n", bufLen);
    ok(buf[0] == 1, "got buf[0] %#x.\n", buf[0]);

    bufLen = 8;
    buf[0] = buf[1] = 0xcc;
    ret = CryptStringToBinaryA("01\0 02", 4, CRYPT_STRING_HEX, buf, &bufLen, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_DATA, "got ret %d, error %lu.\n", ret, GetLastError());
    ok(bufLen == 8, "got length %lu.\n", bufLen);
    ok(buf[0] == 1, "got buf[0] %#x.\n", buf[0]);
    ok(buf[1] == 0xcc, "got buf[1] %#x.\n", buf[1]);

    bufLen = 8;
    buf[0] = buf[1] = 0xcc;
    ret = CryptStringToBinaryW(L"01\0 02", 4, CRYPT_STRING_HEX, buf, &bufLen, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_DATA, "got ret %d, error %lu.\n", ret, GetLastError());
    ok(bufLen == 8, "got length %lu.\n", bufLen);
    ok(buf[0] == 1, "got buf[0] %#x.\n", buf[0]);
    ok(buf[1] == 0xcc, "got buf[1] %#x.\n", buf[1]);

    bufLen = 1;
    buf[0] = 0xcc;
    skipped = 0xdeadbeef;
    flags = 0xdeadbeef;
    ret = CryptStringToBinaryW(L"0102", 4, CRYPT_STRING_HEX, buf, &bufLen, &skipped, &flags);
    ok(!ret && GetLastError() == ERROR_MORE_DATA, "got ret %d, error %lu.\n", ret, GetLastError());
    ok(bufLen == 1, "got length %lu.\n", bufLen);
    ok(buf[0] == 1, "got buf[0] %#x.\n", buf[0]);
    ok(!flags, "got flags %lu.\n", flags);
    ok(!skipped, "got skipped %lu.\n", skipped);

    for (i = 0; i < ARRAY_SIZE(string_hex_tests); ++i)
    {
        for (wide = 0; wide < 2; ++wide)
        {
            if (wide)
            {
                unsigned int j = 0;

                while ((str_w[j] = string_hex_tests[i][j]))
                    ++j;
            }
            winetest_push_context("test %lu, %s", i, wide ? debugstr_w(str_w)
                    : debugstr_a(string_hex_tests[i]));

            expected_len = 0xdeadbeef;
            expected_skipped = 0xdeadbeef;
            expected_flags = 0xdeadbeef;
            expected_err = string_to_hex(wide ? (void *)str_w : (void *)string_hex_tests[i], wide, 0, NULL,
                    &expected_len, &expected_skipped, &expected_flags);

            bufLen = 0xdeadbeef;
            skipped = 0xdeadbeef;
            flags = 0xdeadbeef;
            SetLastError(0xdeadbeef);
            if (wide)
                ret = CryptStringToBinaryW(str_w, 0, CRYPT_STRING_HEX, NULL, &bufLen, &skipped, &flags);
            else
                ret = CryptStringToBinaryA(string_hex_tests[i], 0, CRYPT_STRING_HEX, NULL, &bufLen, &skipped, &flags);

            ok(bufLen == expected_len, "got length %lu.\n", bufLen);
            ok(skipped == expected_skipped, "got skipped %lu.\n", skipped);
            ok(flags == expected_flags, "got flags %lu.\n", flags);

            if (expected_err)
                ok(!ret && GetLastError() == expected_err, "got ret %d, error %lu.\n", ret, GetLastError());
            else
                ok(ret, "got error %lu.\n", GetLastError());

            memset(expected, 0xcc, sizeof(expected));
            expected_len = 8;
            expected_skipped = 0xdeadbeef;
            expected_flags = 0xdeadbeef;
            expected_err = string_to_hex(wide ? (void *)str_w : (void *)string_hex_tests[i], wide, 0, expected,
                    &expected_len, &expected_skipped, &expected_flags);

            memset(buf, 0xcc, sizeof(buf));
            bufLen = 8;
            skipped = 0xdeadbeef;
            flags = 0xdeadbeef;
            SetLastError(0xdeadbeef);
            if (wide)
                ret = CryptStringToBinaryW(str_w, 0, CRYPT_STRING_HEX, buf, &bufLen, &skipped, &flags);
            else
                ret = CryptStringToBinaryA(string_hex_tests[i], 0, CRYPT_STRING_HEX, buf, &bufLen, &skipped, &flags);

            ok(!memcmp(buf, expected, sizeof(buf)), "data does not match, buf[0] %#x, buf[1] %#x.\n", buf[0], buf[1]);
            ok(bufLen == expected_len, "got length %lu.\n", bufLen);
            if (expected_err)
                ok(!ret && GetLastError() == expected_err, "got ret %d, error %lu.\n", ret, GetLastError());
            else
                ok(ret, "got error %lu.\n", GetLastError());

            ok(bufLen == expected_len, "got length %lu.\n", bufLen);
            ok(skipped == expected_skipped, "got skipped %lu.\n", skipped);
            ok(flags == expected_flags, "got flags %lu.\n", flags);

            winetest_pop_context();
        }
    }

    bufLen = 1;
    SetLastError(0xdeadbeef);
    skipped = 0xdeadbeef;
    flags = 0xdeadbeef;
    memset(buf, 0xcc, sizeof(buf));
    ret = CryptStringToBinaryA("0102", 0, CRYPT_STRING_HEX, buf, &bufLen, &skipped, &flags);
    ok(!ret && GetLastError() == ERROR_MORE_DATA, "got ret %d, error %lu.\n", ret, GetLastError());
    ok(bufLen == 1, "got length %lu.\n", bufLen);
    ok(!skipped, "got skipped %lu.\n", skipped);
    ok(!flags, "got flags %lu.\n", flags);
    ok(buf[0] == 1, "got buf[0] %#x.\n", buf[0]);
    ok(buf[1] == 0xcc, "got buf[1] %#x.\n", buf[1]);

    bufLen = 1;
    SetLastError(0xdeadbeef);
    skipped = 0xdeadbeef;
    flags = 0xdeadbeef;
    memset(buf, 0xcc, sizeof(buf));
    ret = CryptStringToBinaryA("0102q", 0, CRYPT_STRING_HEX, buf, &bufLen, &skipped, &flags);
    ok(!ret && GetLastError() == ERROR_INVALID_DATA, "got ret %d, error %lu.\n", ret, GetLastError());
    ok(bufLen == 1, "got length %lu.\n", bufLen);
    ok(!skipped, "got skipped %lu.\n", skipped);
    ok(!flags, "got flags %lu.\n", flags);
    ok(buf[0] == 1, "got buf[0] %#x.\n", buf[0]);
    ok(buf[1] == 0xcc, "got buf[1] %#x.\n", buf[1]);

    bufLen = 1;
    SetLastError(0xdeadbeef);
    skipped = 0xdeadbeef;
    flags = 0xdeadbeef;
    memset(buf, 0xcc, sizeof(buf));
    ret = CryptStringToBinaryW(L"0102q", 0, CRYPT_STRING_HEX, buf, &bufLen, &skipped, &flags);
    ok(!ret && GetLastError() == ERROR_INVALID_DATA, "got ret %d, error %lu.\n", ret, GetLastError());
    ok(bufLen == 1, "got length %lu.\n", bufLen);
    ok(!skipped, "got skipped %lu.\n", skipped);
    ok(!flags, "got flags %lu.\n", flags);
    ok(buf[0] == 1, "got buf[0] %#x.\n", buf[0]);
    ok(buf[1] == 0xcc, "got buf[1] %#x.\n", buf[1]);

    bufLen = 1;
    SetLastError(0xdeadbeef);
    skipped = 0xdeadbeef;
    flags = 0xdeadbeef;
    memset(buf, 0xcc, sizeof(buf));
    ret = CryptStringToBinaryW(L"0102", 0, CRYPT_STRING_HEX, buf, &bufLen, &skipped, &flags);
    ok(bufLen == 1, "got length %lu.\n", bufLen);
    ok(!ret && GetLastError() == ERROR_MORE_DATA, "got ret %d, error %lu.\n", ret, GetLastError());
    ok(buf[0] == 1, "got buf[0] %#x.\n", buf[0]);
    ok(buf[1] == 0xcc, "got buf[1] %#x.\n", buf[1]);

    /* It looks like Windows is normalizing Unicode strings in some way which depending on locale may result in
     * some invalid characters in 128-255 range being converted into sequences starting with valid hex numbers.
     * Just avoiding characters in the 128-255 range in test. */
    for (i = 1; i < 128; ++i)
    {
        char str_a[16];

        for (wide = 0; wide < 2; ++wide)
        {
            if (wide)
            {
                str_w[0] = i;
                wcscpy(str_w + 1, L"00");
            }
            else
            {
                str_a[0] = i;
                strcpy(str_a + 1, "00");
            }

            winetest_push_context("char %#lx, %s", i, wide ? debugstr_w(str_w) : debugstr_a(str_a));

            bufLen = 1;
            buf[0] = buf[1] = 0xcc;
            SetLastError(0xdeadbeef);
            if (wide)
                ret = CryptStringToBinaryW(str_w, 0, CRYPT_STRING_HEX, buf, &bufLen, &skipped, &flags);
            else
                ret = CryptStringToBinaryA(str_a, 0, CRYPT_STRING_HEX, buf, &bufLen, &skipped, &flags);
            ok(bufLen == 1, "got length %lu.\n", bufLen);
            if (is_hex_string_special_char(i))
            {
                ok(ret, "got error %lu.\n", GetLastError());
                ok(!buf[0], "got buf[0] %#x.\n", buf[0]);
                ok(buf[1] == 0xcc, "got buf[1] %#x.\n", buf[1]);
            }
            else
            {
                ok(!ret && GetLastError() == ERROR_INVALID_DATA, "got ret %d, error %lu.\n", ret, GetLastError());
                if (isdigit(i) || (tolower(i) >= 'a' && tolower(i) <= 'f'))
                {
                    ok(buf[0] == (digit_from_char(i) << 4), "got buf[0] %#x.\n", buf[0]);
                    ok(buf[1] == 0xcc, "got buf[0] %#x.\n", buf[1]);
                }
                else
                {
                    ok(buf[0] == 0xcc, "got buf[0] %#x.\n", buf[0]);
                }
            }
            winetest_pop_context();
        }
    }
}

START_TEST(base64)
{
    test_CryptBinaryToString();
    test_CryptStringToBinary();
}
