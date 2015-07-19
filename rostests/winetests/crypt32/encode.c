/*
 * Unit test suite for crypt32.dll's CryptEncodeObjectEx/CryptDecodeObjectEx
 *
 * Copyright 2005 Juan Lang
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
//#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
//#include <winerror.h>
#include <wincrypt.h>

#include <wine/test.h>


static BOOL (WINAPI *pCryptDecodeObjectEx)(DWORD,LPCSTR,const BYTE*,DWORD,DWORD,PCRYPT_DECODE_PARA,void*,DWORD*);
static BOOL (WINAPI *pCryptEncodeObjectEx)(DWORD,LPCSTR,const void*,DWORD,PCRYPT_ENCODE_PARA,void*,DWORD*);

struct encodedInt
{
    int val;
    const BYTE *encoded;
};

static const BYTE bin1[] = {0x02,0x01,0x01};
static const BYTE bin2[] = {0x02,0x01,0x7f};
static const BYTE bin3[] = {0x02,0x02,0x00,0x80};
static const BYTE bin4[] = {0x02,0x02,0x01,0x00};
static const BYTE bin5[] = {0x02,0x01,0x80};
static const BYTE bin6[] = {0x02,0x02,0xff,0x7f};
static const BYTE bin7[] = {0x02,0x04,0xba,0xdd,0xf0,0x0d};

static const struct encodedInt ints[] = {
 { 1,          bin1 },
 { 127,        bin2 },
 { 128,        bin3 },
 { 256,        bin4 },
 { -128,       bin5 },
 { -129,       bin6 },
 { 0xbaddf00d, bin7 },
};

struct encodedBigInt
{
    const BYTE *val;
    const BYTE *encoded;
    const BYTE *decoded;
};

static const BYTE bin8[] = {0xff,0xff,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0};
static const BYTE bin9[] = {0x02,0x0a,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0xff,0xff,0};
static const BYTE bin10[] = {0xff,0xff,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0};

static const BYTE bin11[] = {0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0xff,0xff,0xff,0};
static const BYTE bin12[] = {0x02,0x09,0xff,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0};
static const BYTE bin13[] = {0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0xff,0};

static const struct encodedBigInt bigInts[] = {
 { bin8, bin9, bin10 },
 { bin11, bin12, bin13 },
};

static const BYTE bin14[] = {0xff,0xff,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0};
static const BYTE bin15[] = {0x02,0x0a,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0xff,0xff,0};
static const BYTE bin16[] = {0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0xff,0xff,0xff,0};
static const BYTE bin17[] = {0x02,0x0c,0x00,0xff,0xff,0xff,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0};

/* Decoded is the same as original, so don't bother storing a separate copy */
static const struct encodedBigInt bigUInts[] = {
 { bin14, bin15, NULL },
 { bin16, bin17, NULL },
};

static void test_encodeInt(DWORD dwEncoding)
{
    DWORD bufSize = 0;
    int i;
    BOOL ret;
    CRYPT_INTEGER_BLOB blob;
    BYTE *buf = NULL;

    /* CryptEncodeObjectEx with NULL bufSize crashes..
    ret = pCryptEncodeObjectEx(3, X509_INTEGER, &ints[0].val, 0, NULL, NULL,
     NULL);
     */
    /* check bogus encoding */
    ret = pCryptEncodeObjectEx(0, X509_INTEGER, &ints[0].val, 0, NULL, NULL,
     &bufSize);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
    if (0)
    {
        /* check with NULL integer buffer.  Windows XP incorrectly returns an
         * NTSTATUS (crashes on win9x).
         */
        ret = pCryptEncodeObjectEx(dwEncoding, X509_INTEGER, NULL, 0, NULL, NULL,
         &bufSize);
        ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
         "Expected STATUS_ACCESS_VIOLATION, got %08x\n", GetLastError());
    }
    for (i = 0; i < sizeof(ints) / sizeof(ints[0]); i++)
    {
        /* encode as normal integer */
        ret = pCryptEncodeObjectEx(dwEncoding, X509_INTEGER, &ints[i].val, 0,
         NULL, NULL, &bufSize);
        ok(ret, "Expected success, got %d\n", GetLastError());
        ret = pCryptEncodeObjectEx(dwEncoding, X509_INTEGER, &ints[i].val,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &bufSize);
        ok(ret, "CryptEncodeObjectEx failed: %d\n", GetLastError());
        if (ret)
        {
            ok(buf[0] == 2, "Got unexpected type %d for integer (expected 2)\n",
             buf[0]);
            ok(buf[1] == ints[i].encoded[1], "Got length %d, expected %d\n",
             buf[1], ints[i].encoded[1]);
            ok(!memcmp(buf + 1, ints[i].encoded + 1, ints[i].encoded[1] + 1),
             "Encoded value of 0x%08x didn't match expected\n", ints[i].val);
            LocalFree(buf);
        }
        /* encode as multibyte integer */
        blob.cbData = sizeof(ints[i].val);
        blob.pbData = (BYTE *)&ints[i].val;
        ret = pCryptEncodeObjectEx(dwEncoding, X509_MULTI_BYTE_INTEGER, &blob,
         0, NULL, NULL, &bufSize);
        ok(ret, "Expected success, got %d\n", GetLastError());
        ret = pCryptEncodeObjectEx(dwEncoding, X509_MULTI_BYTE_INTEGER, &blob,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &bufSize);
        ok(ret, "CryptEncodeObjectEx failed: %d\n", GetLastError());
        if (ret)
        {
            ok(buf[0] == 2, "Got unexpected type %d for integer (expected 2)\n",
             buf[0]);
            ok(buf[1] == ints[i].encoded[1], "Got length %d, expected %d\n",
             buf[1], ints[i].encoded[1]);
            ok(!memcmp(buf + 1, ints[i].encoded + 1, ints[i].encoded[1] + 1),
             "Encoded value of 0x%08x didn't match expected\n", ints[i].val);
            LocalFree(buf);
        }
    }
    /* encode a couple bigger ints, just to show it's little-endian and leading
     * sign bytes are dropped
     */
    for (i = 0; i < sizeof(bigInts) / sizeof(bigInts[0]); i++)
    {
        blob.cbData = strlen((const char*)bigInts[i].val);
        blob.pbData = (BYTE *)bigInts[i].val;
        ret = pCryptEncodeObjectEx(dwEncoding, X509_MULTI_BYTE_INTEGER, &blob,
         0, NULL, NULL, &bufSize);
        ok(ret, "Expected success, got %d\n", GetLastError());
        ret = pCryptEncodeObjectEx(dwEncoding, X509_MULTI_BYTE_INTEGER, &blob,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &bufSize);
        ok(ret, "CryptEncodeObjectEx failed: %d\n", GetLastError());
        if (ret)
        {
            ok(buf[0] == 2, "Got unexpected type %d for integer (expected 2)\n",
             buf[0]);
            ok(buf[1] == bigInts[i].encoded[1], "Got length %d, expected %d\n",
             buf[1], bigInts[i].encoded[1]);
            ok(!memcmp(buf + 1, bigInts[i].encoded + 1,
             bigInts[i].encoded[1] + 1),
             "Encoded value didn't match expected\n");
            LocalFree(buf);
        }
    }
    /* and, encode some uints */
    for (i = 0; i < sizeof(bigUInts) / sizeof(bigUInts[0]); i++)
    {
        blob.cbData = strlen((const char*)bigUInts[i].val);
        blob.pbData = (BYTE*)bigUInts[i].val;
        ret = pCryptEncodeObjectEx(dwEncoding, X509_MULTI_BYTE_UINT, &blob,
         0, NULL, NULL, &bufSize);
        ok(ret, "Expected success, got %d\n", GetLastError());
        ret = pCryptEncodeObjectEx(dwEncoding, X509_MULTI_BYTE_UINT, &blob,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &bufSize);
        ok(ret, "CryptEncodeObjectEx failed: %d\n", GetLastError());
        if (ret)
        {
            ok(buf[0] == 2, "Got unexpected type %d for integer (expected 2)\n",
             buf[0]);
            ok(buf[1] == bigUInts[i].encoded[1], "Got length %d, expected %d\n",
             buf[1], bigUInts[i].encoded[1]);
            ok(!memcmp(buf + 1, bigUInts[i].encoded + 1,
             bigUInts[i].encoded[1] + 1),
             "Encoded value didn't match expected\n");
            LocalFree(buf);
        }
    }
}

static void test_decodeInt(DWORD dwEncoding)
{
    static const BYTE bigInt[] = { 2, 5, 0xff, 0xfe, 0xff, 0xfe, 0xff };
    static const BYTE testStr[] = { 0x16, 4, 't', 'e', 's', 't' };
    static const BYTE longForm[] = { 2, 0x81, 0x01, 0x01 };
    static const BYTE bigBogus[] = { 0x02, 0x84, 0x01, 0xff, 0xff, 0xf9 };
    static const BYTE extraBytes[] = { 2, 1, 1, 0, 0, 0, 0 };
    BYTE *buf = NULL;
    DWORD bufSize = 0;
    int i;
    BOOL ret;

    /* CryptDecodeObjectEx with NULL bufSize crashes..
    ret = pCryptDecodeObjectEx(3, X509_INTEGER, &ints[0].encoded,
     ints[0].encoded[1] + 2, 0, NULL, NULL, NULL);
     */
    /* check bogus encoding */
    ret = pCryptDecodeObjectEx(3, X509_INTEGER, (BYTE *)&ints[0].encoded,
     ints[0].encoded[1] + 2, 0, NULL, NULL, &bufSize);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
    /* check with NULL integer buffer */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_INTEGER, NULL, 0, 0, NULL, NULL,
     &bufSize);
    ok(!ret && (GetLastError() == CRYPT_E_ASN1_EOD ||
     GetLastError() == OSS_BAD_ARG /* Win9x */),
     "Expected CRYPT_E_ASN1_EOD or OSS_BAD_ARG, got %08x\n", GetLastError());
    /* check with a valid, but too large, integer */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_INTEGER, bigInt, bigInt[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok((!ret && GetLastError() == CRYPT_E_ASN1_LARGE) ||
     broken(ret) /* Win9x */,
     "Expected CRYPT_E_ASN1_LARGE, got %d\n", GetLastError());
    /* check with a DER-encoded string */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_INTEGER, testStr, testStr[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(!ret && (GetLastError() == CRYPT_E_ASN1_BADTAG ||
     GetLastError() == OSS_PDU_MISMATCH /* Win9x */ ),
     "Expected CRYPT_E_ASN1_BADTAG or OSS_PDU_MISMATCH, got %08x\n",
     GetLastError());
    for (i = 0; i < sizeof(ints) / sizeof(ints[0]); i++)
    {
        /* When the output buffer is NULL, this always succeeds */
        SetLastError(0xdeadbeef);
        ret = pCryptDecodeObjectEx(dwEncoding, X509_INTEGER,
         ints[i].encoded, ints[i].encoded[1] + 2, 0, NULL, NULL,
         &bufSize);
        ok(ret && GetLastError() == NOERROR,
         "Expected success and NOERROR, got %d\n", GetLastError());
        ret = pCryptDecodeObjectEx(dwEncoding, X509_INTEGER,
         ints[i].encoded, ints[i].encoded[1] + 2,
         CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
        ok(ret, "CryptDecodeObjectEx failed: %d\n", GetLastError());
        ok(bufSize == sizeof(int), "Wrong size %d\n", bufSize);
        ok(buf != NULL, "Expected allocated buffer\n");
        if (ret)
        {
            ok(!memcmp(buf, &ints[i].val, bufSize), "Expected %d, got %d\n",
             ints[i].val, *(int *)buf);
            LocalFree(buf);
        }
    }
    for (i = 0; i < sizeof(bigInts) / sizeof(bigInts[0]); i++)
    {
        ret = pCryptDecodeObjectEx(dwEncoding, X509_MULTI_BYTE_INTEGER,
         bigInts[i].encoded, bigInts[i].encoded[1] + 2, 0, NULL, NULL,
         &bufSize);
        ok(ret && GetLastError() == NOERROR,
         "Expected success and NOERROR, got %d\n", GetLastError());
        ret = pCryptDecodeObjectEx(dwEncoding, X509_MULTI_BYTE_INTEGER,
         bigInts[i].encoded, bigInts[i].encoded[1] + 2,
         CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
        ok(ret, "CryptDecodeObjectEx failed: %d\n", GetLastError());
        ok(bufSize >= sizeof(CRYPT_INTEGER_BLOB), "Wrong size %d\n", bufSize);
        ok(buf != NULL, "Expected allocated buffer\n");
        if (ret)
        {
            CRYPT_INTEGER_BLOB *blob = (CRYPT_INTEGER_BLOB *)buf;

            ok(blob->cbData == strlen((const char*)bigInts[i].decoded),
             "Expected len %d, got %d\n", lstrlenA((const char*)bigInts[i].decoded),
             blob->cbData);
            ok(!memcmp(blob->pbData, bigInts[i].decoded, blob->cbData),
             "Unexpected value\n");
            LocalFree(buf);
        }
    }
    for (i = 0; i < sizeof(bigUInts) / sizeof(bigUInts[0]); i++)
    {
        ret = pCryptDecodeObjectEx(dwEncoding, X509_MULTI_BYTE_UINT,
         bigUInts[i].encoded, bigUInts[i].encoded[1] + 2, 0, NULL, NULL,
         &bufSize);
        ok(ret && GetLastError() == NOERROR,
         "Expected success and NOERROR, got %d\n", GetLastError());
        ret = pCryptDecodeObjectEx(dwEncoding, X509_MULTI_BYTE_UINT,
         bigUInts[i].encoded, bigUInts[i].encoded[1] + 2,
         CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
        ok(ret, "CryptDecodeObjectEx failed: %d\n", GetLastError());
        ok(bufSize >= sizeof(CRYPT_INTEGER_BLOB), "Wrong size %d\n", bufSize);
        ok(buf != NULL, "Expected allocated buffer\n");
        if (ret)
        {
            CRYPT_INTEGER_BLOB *blob = (CRYPT_INTEGER_BLOB *)buf;

            ok(blob->cbData == strlen((const char*)bigUInts[i].val),
             "Expected len %d, got %d\n", lstrlenA((const char*)bigUInts[i].val),
             blob->cbData);
            ok(!memcmp(blob->pbData, bigUInts[i].val, blob->cbData),
             "Unexpected value\n");
            LocalFree(buf);
        }
    }
    /* Decode the value 1 with long-form length */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_MULTI_BYTE_INTEGER, longForm,
     sizeof(longForm), CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(*(int *)buf == 1, "Expected 1, got %d\n", *(int *)buf);
        LocalFree(buf);
    }
    /* check with extra bytes at the end */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_INTEGER, extraBytes,
     sizeof(extraBytes), CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(*(int *)buf == 1, "Expected 1, got %d\n", *(int *)buf);
        LocalFree(buf);
    }
    /* Try to decode some bogus large items */
    /* The buffer size is smaller than the encoded length, so this should fail
     * with CRYPT_E_ASN1_EOD if it's being decoded.
     * Under XP it fails with CRYPT_E_ASN1_LARGE, which means there's a limit
     * on the size decoded, but in ME it fails with CRYPT_E_ASN1_EOD or crashes.
     * So this test unfortunately isn't useful.
    ret = pCryptDecodeObjectEx(dwEncoding, X509_MULTI_BYTE_INTEGER, tooBig,
     0x7fffffff, CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(!ret && GetLastError() == CRYPT_E_ASN1_LARGE,
     "Expected CRYPT_E_ASN1_LARGE, got %08x\n", GetLastError());
     */
    /* This will try to decode the buffer and overflow it, check that it's
     * caught.
     */
    if (0)
    {
    /* a large buffer isn't guaranteed to crash, it depends on memory allocation order */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_MULTI_BYTE_INTEGER, bigBogus,
     0x01ffffff, CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
     "Expected STATUS_ACCESS_VIOLATION, got %08x\n", GetLastError());
    }
}

static const BYTE bin18[] = {0x0a,0x01,0x01};
static const BYTE bin19[] = {0x0a,0x05,0x00,0xff,0xff,0xff,0x80};

/* These are always encoded unsigned, and aren't constrained to be any
 * particular value
 */
static const struct encodedInt enums[] = {
 { 1,    bin18 },
 { -128, bin19 },
};

/* X509_CRL_REASON_CODE is also an enumerated type, but it's #defined to
 * X509_ENUMERATED.
 */
static const LPCSTR enumeratedTypes[] = { X509_ENUMERATED,
 szOID_CRL_REASON_CODE };

static void test_encodeEnumerated(DWORD dwEncoding)
{
    DWORD i, j;

    for (i = 0; i < sizeof(enumeratedTypes) / sizeof(enumeratedTypes[0]); i++)
    {
        for (j = 0; j < sizeof(enums) / sizeof(enums[0]); j++)
        {
            BOOL ret;
            BYTE *buf = NULL;
            DWORD bufSize = 0;

            ret = pCryptEncodeObjectEx(dwEncoding, enumeratedTypes[i],
             &enums[j].val, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf,
             &bufSize);
            ok(ret, "CryptEncodeObjectEx failed: %d\n", GetLastError());
            if (ret)
            {
                ok(buf[0] == 0xa,
                 "Got unexpected type %d for enumerated (expected 0xa)\n",
                 buf[0]);
                ok(buf[1] == enums[j].encoded[1],
                 "Got length %d, expected %d\n", buf[1], enums[j].encoded[1]);
                ok(!memcmp(buf + 1, enums[j].encoded + 1,
                 enums[j].encoded[1] + 1),
                 "Encoded value of 0x%08x didn't match expected\n",
                 enums[j].val);
                LocalFree(buf);
            }
        }
    }
}

static void test_decodeEnumerated(DWORD dwEncoding)
{
    DWORD i, j;

    for (i = 0; i < sizeof(enumeratedTypes) / sizeof(enumeratedTypes[0]); i++)
    {
        for (j = 0; j < sizeof(enums) / sizeof(enums[0]); j++)
        {
            BOOL ret;
            DWORD bufSize = sizeof(int);
            int val;

            ret = pCryptDecodeObjectEx(dwEncoding, enumeratedTypes[i],
             enums[j].encoded, enums[j].encoded[1] + 2, 0, NULL,
             &val, &bufSize);
            ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
            ok(bufSize == sizeof(int),
             "Got unexpected size %d for enumerated\n", bufSize);
            ok(val == enums[j].val, "Unexpected value %d, expected %d\n",
             val, enums[j].val);
        }
    }
}

struct encodedFiletime
{
    SYSTEMTIME sysTime;
    const BYTE *encodedTime;
};

static void testTimeEncoding(DWORD dwEncoding, LPCSTR structType,
 const struct encodedFiletime *time)
{
    FILETIME ft = { 0 };
    BYTE *buf = NULL;
    DWORD bufSize = 0;
    BOOL ret;

    ret = SystemTimeToFileTime(&time->sysTime, &ft);
    ok(ret, "SystemTimeToFileTime failed: %d\n", GetLastError());
    ret = pCryptEncodeObjectEx(dwEncoding, structType, &ft,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    /* years other than 1950-2050 are not allowed for encodings other than
     * X509_CHOICE_OF_TIME.
     */
    if (structType == X509_CHOICE_OF_TIME ||
     (time->sysTime.wYear >= 1950 && time->sysTime.wYear <= 2050))
    {
        ok(ret, "CryptEncodeObjectEx failed: %d (0x%08x)\n", GetLastError(),
         GetLastError());
        ok(buf != NULL, "Expected an allocated buffer\n");
        if (ret)
        {
            ok(buf[0] == time->encodedTime[0],
             "Expected type 0x%02x, got 0x%02x\n", time->encodedTime[0],
             buf[0]);
            ok(buf[1] == time->encodedTime[1], "Expected %d bytes, got %d\n",
             time->encodedTime[1], bufSize);
            ok(!memcmp(time->encodedTime + 2, buf + 2, time->encodedTime[1]),
             "Got unexpected value for time encoding\n");
            LocalFree(buf);
        }
    }
    else
        ok((!ret && GetLastError() == CRYPT_E_BAD_ENCODE) ||
         broken(GetLastError() == ERROR_SUCCESS),
         "Expected CRYPT_E_BAD_ENCODE, got 0x%08x\n", GetLastError());
}

static const char *printSystemTime(const SYSTEMTIME *st)
{
    static char buf[25];

    sprintf(buf, "%02d-%02d-%04d %02d:%02d:%02d.%03d", st->wMonth, st->wDay,
     st->wYear, st->wHour, st->wMinute, st->wSecond, st->wMilliseconds);
    return buf;
}

static const char *printFileTime(const FILETIME *ft)
{
    static char buf[25];
    SYSTEMTIME st;

    FileTimeToSystemTime(ft, &st);
    sprintf(buf, "%02d-%02d-%04d %02d:%02d:%02d.%03d", st.wMonth, st.wDay,
     st.wYear, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    return buf;
}

static void compareTime(const SYSTEMTIME *expected, const FILETIME *got)
{
    SYSTEMTIME st;

    FileTimeToSystemTime(got, &st);
    ok((expected->wYear == st.wYear &&
     expected->wMonth == st.wMonth &&
     expected->wDay == st.wDay &&
     expected->wHour == st.wHour &&
     expected->wMinute == st.wMinute &&
     expected->wSecond == st.wSecond &&
     abs(expected->wMilliseconds - st.wMilliseconds) <= 1) ||
     /* Some Windows systems only seem to be accurate in their time decoding to
      * within about an hour.
      */
     broken(expected->wYear == st.wYear &&
     expected->wMonth == st.wMonth &&
     expected->wDay == st.wDay &&
     abs(expected->wHour - st.wHour) <= 1),
     "Got unexpected value for time decoding:\nexpected %s, got %s\n",
     printSystemTime(expected), printFileTime(got));
}

static void testTimeDecoding(DWORD dwEncoding, LPCSTR structType,
 const struct encodedFiletime *time)
{
    FILETIME ft = { 0 };
    DWORD size = sizeof(ft);
    BOOL ret;

    ret = pCryptDecodeObjectEx(dwEncoding, structType, time->encodedTime,
     time->encodedTime[1] + 2, 0, NULL, &ft, &size);
    /* years other than 1950-2050 are not allowed for encodings other than
     * X509_CHOICE_OF_TIME.
     */
    if (structType == X509_CHOICE_OF_TIME ||
     (time->sysTime.wYear >= 1950 && time->sysTime.wYear <= 2050))
    {
        ok(ret || broken(GetLastError() == OSS_DATA_ERROR),
         "CryptDecodeObjectEx failed: %d (0x%08x)\n", GetLastError(),
         GetLastError());
        if (ret)
            compareTime(&time->sysTime, &ft);
    }
    else
        ok(!ret && (GetLastError() == CRYPT_E_ASN1_BADTAG ||
         GetLastError() == OSS_PDU_MISMATCH /* Win9x */ ),
         "Expected CRYPT_E_ASN1_BADTAG or OSS_PDU_MISMATCH, got %08x\n",
         GetLastError());
}

static const BYTE bin20[] = {
    0x17,0x0d,'0','5','0','6','0','6','1','6','1','0','0','0','Z'};
static const BYTE bin21[] = {
    0x18,0x0f,'1','9','4','5','0','6','0','6','1','6','1','0','0','0','Z'};
static const BYTE bin22[] = {
    0x18,0x0f,'2','1','4','5','0','6','0','6','1','6','1','0','0','0','Z'};

static const struct encodedFiletime times[] = {
 { { 2005, 6, 1, 6, 16, 10, 0, 0 }, bin20 },
 { { 1945, 6, 1, 6, 16, 10, 0, 0 }, bin21 },
 { { 2145, 6, 1, 6, 16, 10, 0, 0 }, bin22 },
};

static void test_encodeFiletime(DWORD dwEncoding)
{
    DWORD i;

    for (i = 0; i < sizeof(times) / sizeof(times[0]); i++)
    {
        testTimeEncoding(dwEncoding, X509_CHOICE_OF_TIME, &times[i]);
        testTimeEncoding(dwEncoding, PKCS_UTC_TIME, &times[i]);
        testTimeEncoding(dwEncoding, szOID_RSA_signingTime, &times[i]);
    }
}

static const BYTE bin23[] = {
    0x18,0x13,'1','9','4','5','0','6','0','6','1','6','1','0','0','0','.','0','0','0','Z'};
static const BYTE bin24[] = {
    0x18,0x13,'1','9','4','5','0','6','0','6','1','6','1','0','0','0','.','9','9','9','Z'};
static const BYTE bin25[] = {
    0x18,0x13,'1','9','4','5','0','6','0','6','1','6','1','0','0','0','+','0','1','0','0'};
static const BYTE bin26[] = {
    0x18,0x13,'1','9','4','5','0','6','0','6','1','6','1','0','0','0','-','0','1','0','0'};
static const BYTE bin27[] = {
    0x18,0x13,'1','9','4','5','0','6','0','6','1','6','1','0','0','0','-','0','1','1','5'};
static const BYTE bin28[] = {
    0x18,0x0a,'2','1','4','5','0','6','0','6','1','6'};
static const BYTE bin29[] = {
    0x17,0x0a,'4','5','0','6','0','6','1','6','1','0'};
static const BYTE bin30[] = {
    0x17,0x0b,'4','5','0','6','0','6','1','6','1','0','Z'};
static const BYTE bin31[] = {
    0x17,0x0d,'4','5','0','6','0','6','1','6','1','0','+','0','1'};
static const BYTE bin32[] = {
    0x17,0x0d,'4','5','0','6','0','6','1','6','1','0','-','0','1'};
static const BYTE bin33[] = {
    0x17,0x0f,'4','5','0','6','0','6','1','6','1','0','+','0','1','0','0'};
static const BYTE bin34[] = {
    0x17,0x0f,'4','5','0','6','0','6','1','6','1','0','-','0','1','0','0'};
static const BYTE bin35[] = {
    0x17,0x08, '4','5','0','6','0','6','1','6'};
static const BYTE bin36[] = {
    0x18,0x0f, 'a','a','a','a','a','a','a','a','a','a','a','a','a','a','Z'};
static const BYTE bin37[] = {
    0x18,0x04, '2','1','4','5'};
static const BYTE bin38[] = {
    0x18,0x08, '2','1','4','5','0','6','0','6'};

static void test_decodeFiletime(DWORD dwEncoding)
{
    static const struct encodedFiletime otherTimes[] = {
     { { 1945, 6, 1, 6, 16, 10, 0, 0 },   bin23 },
     { { 1945, 6, 1, 6, 16, 10, 0, 999 }, bin24 },
     { { 1945, 6, 1, 6, 17, 10, 0, 0 },   bin25 },
     { { 1945, 6, 1, 6, 15, 10, 0, 0 },   bin26 },
     { { 1945, 6, 1, 6, 14, 55, 0, 0 },   bin27 },
     { { 2145, 6, 1, 6, 16,  0, 0, 0 },   bin28 },
     { { 2045, 6, 1, 6, 16, 10, 0, 0 },   bin29 },
     { { 2045, 6, 1, 6, 16, 10, 0, 0 },   bin30 },
     { { 2045, 6, 1, 6, 17, 10, 0, 0 },   bin31 },
     { { 2045, 6, 1, 6, 15, 10, 0, 0 },   bin32 },
     { { 2045, 6, 1, 6, 17, 10, 0, 0 },   bin33 },
     { { 2045, 6, 1, 6, 15, 10, 0, 0 },   bin34 },
    };
    /* An oddball case that succeeds in Windows, but doesn't seem correct
     { { 2145, 6, 1, 2, 11, 31, 0, 0 },   "\x18" "\x13" "21450606161000-9999" },
     */
    static const unsigned char *bogusTimes[] = {
     /* oddly, this succeeds on Windows, with year 2765
     "\x18" "\x0f" "21r50606161000Z",
      */
     bin35,
     bin36,
     bin37,
     bin38,
    };
    DWORD i, size;
    FILETIME ft1 = { 0 }, ft2 = { 0 };
    BOOL ret;

    /* Check bogus length with non-NULL buffer */
    ret = SystemTimeToFileTime(&times[0].sysTime, &ft1);
    ok(ret, "SystemTimeToFileTime failed: %d\n", GetLastError());
    size = 1;
    ret = pCryptDecodeObjectEx(dwEncoding, X509_CHOICE_OF_TIME,
     times[0].encodedTime, times[0].encodedTime[1] + 2, 0, NULL, &ft2, &size);
    ok(!ret && GetLastError() == ERROR_MORE_DATA,
     "Expected ERROR_MORE_DATA, got %d\n", GetLastError());
    /* Normal tests */
    for (i = 0; i < sizeof(times) / sizeof(times[0]); i++)
    {
        testTimeDecoding(dwEncoding, X509_CHOICE_OF_TIME, &times[i]);
        testTimeDecoding(dwEncoding, PKCS_UTC_TIME, &times[i]);
        testTimeDecoding(dwEncoding, szOID_RSA_signingTime, &times[i]);
    }
    for (i = 0; i < sizeof(otherTimes) / sizeof(otherTimes[0]); i++)
    {
        testTimeDecoding(dwEncoding, X509_CHOICE_OF_TIME, &otherTimes[i]);
        testTimeDecoding(dwEncoding, PKCS_UTC_TIME, &otherTimes[i]);
        testTimeDecoding(dwEncoding, szOID_RSA_signingTime, &otherTimes[i]);
    }
    for (i = 0; i < sizeof(bogusTimes) / sizeof(bogusTimes[0]); i++)
    {
        size = sizeof(ft1);
        ret = pCryptDecodeObjectEx(dwEncoding, X509_CHOICE_OF_TIME,
         bogusTimes[i], bogusTimes[i][1] + 2, 0, NULL, &ft1, &size);
        ok((!ret && (GetLastError() == CRYPT_E_ASN1_CORRUPT ||
                     GetLastError() == OSS_DATA_ERROR /* Win9x */)) ||
           broken(ret), /* Win9x and NT4 for bin38 */
         "Expected CRYPT_E_ASN1_CORRUPT or OSS_DATA_ERROR, got %08x\n",
         GetLastError());
    }
}

static const char commonName[] = "Juan Lang";
static const char surName[] = "Lang";

static const BYTE emptySequence[] = { 0x30, 0 };
static const BYTE emptyRDNs[] = { 0x30, 0x02, 0x31, 0 };
static const BYTE twoRDNs[] = {
    0x30,0x23,0x31,0x21,0x30,0x0c,0x06,0x03,0x55,0x04,0x04,
    0x13,0x05,0x4c,0x61,0x6e,0x67,0x00,0x30,0x11,0x06,0x03,0x55,0x04,0x03,
    0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0};
static const BYTE encodedTwoRDNs[] = {
0x30,0x2e,0x31,0x2c,0x30,0x2a,0x06,0x03,0x55,0x04,0x03,0x30,0x23,0x31,0x21,
0x30,0x0c,0x06,0x03,0x55,0x04,0x04,0x13,0x05,0x4c,0x61,0x6e,0x67,0x00,0x30,
0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,
0x6e,0x67,0x00,
};

static const BYTE us[] = { 0x55, 0x53 };
static const BYTE minnesota[] = { 0x4d, 0x69, 0x6e, 0x6e, 0x65, 0x73, 0x6f,
 0x74, 0x61 };
static const BYTE minneapolis[] = { 0x4d, 0x69, 0x6e, 0x6e, 0x65, 0x61, 0x70,
 0x6f, 0x6c, 0x69, 0x73 };
static const BYTE codeweavers[] = { 0x43, 0x6f, 0x64, 0x65, 0x57, 0x65, 0x61,
 0x76, 0x65, 0x72, 0x73 };
static const BYTE wine[] = { 0x57, 0x69, 0x6e, 0x65, 0x20, 0x44, 0x65, 0x76,
 0x65, 0x6c, 0x6f, 0x70, 0x6d, 0x65, 0x6e, 0x74 };
static const BYTE localhostAttr[] = { 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f,
 0x73, 0x74 };
static const BYTE aric[] = { 0x61, 0x72, 0x69, 0x63, 0x40, 0x63, 0x6f, 0x64,
 0x65, 0x77, 0x65, 0x61, 0x76, 0x65, 0x72, 0x73, 0x2e, 0x63, 0x6f, 0x6d };

#define RDNA(arr)   oid_ ## arr, CERT_RDN_PRINTABLE_STRING, { sizeof(arr), (LPBYTE)arr }
#define RDNIA5(arr) oid_ ## arr, CERT_RDN_IA5_STRING,       { sizeof(arr), (LPBYTE)arr }

static CHAR oid_us[]            = "2.5.4.6",
            oid_minnesota[]     = "2.5.4.8",
            oid_minneapolis[]   = "2.5.4.7",
            oid_codeweavers[]   = "2.5.4.10",
            oid_wine[]          = "2.5.4.11",
            oid_localhostAttr[] = "2.5.4.3",
            oid_aric[]          = "1.2.840.113549.1.9.1";
static CERT_RDN_ATTR rdnAttrs[] = { { RDNA(us) },
                                    { RDNA(minnesota) },
                                    { RDNA(minneapolis) },
                                    { RDNA(codeweavers) },
                                    { RDNA(wine) },
                                    { RDNA(localhostAttr) },
                                    { RDNIA5(aric) } };
static CERT_RDN_ATTR decodedRdnAttrs[] = { { RDNA(us) },
                                           { RDNA(localhostAttr) },
                                           { RDNA(minnesota) },
                                           { RDNA(minneapolis) },
                                           { RDNA(codeweavers) },
                                           { RDNA(wine) },
                                           { RDNIA5(aric) } };

#undef RDNIA5
#undef RDNA

static const BYTE encodedRDNAttrs[] = {
0x30,0x81,0x96,0x31,0x81,0x93,0x30,0x09,0x06,0x03,0x55,0x04,0x06,0x13,0x02,0x55,
0x53,0x30,0x10,0x06,0x03,0x55,0x04,0x03,0x13,0x09,0x6c,0x6f,0x63,0x61,0x6c,0x68,
0x6f,0x73,0x74,0x30,0x10,0x06,0x03,0x55,0x04,0x08,0x13,0x09,0x4d,0x69,0x6e,0x6e,
0x65,0x73,0x6f,0x74,0x61,0x30,0x12,0x06,0x03,0x55,0x04,0x07,0x13,0x0b,0x4d,0x69,
0x6e,0x6e,0x65,0x61,0x70,0x6f,0x6c,0x69,0x73,0x30,0x12,0x06,0x03,0x55,0x04,0x0a,
0x13,0x0b,0x43,0x6f,0x64,0x65,0x57,0x65,0x61,0x76,0x65,0x72,0x73,0x30,0x17,0x06,
0x03,0x55,0x04,0x0b,0x13,0x10,0x57,0x69,0x6e,0x65,0x20,0x44,0x65,0x76,0x65,0x6c,
0x6f,0x70,0x6d,0x65,0x6e,0x74,0x30,0x21,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,
0x01,0x09,0x01,0x16,0x14,0x61,0x72,0x69,0x63,0x40,0x63,0x6f,0x64,0x65,0x77,0x65,
0x61,0x76,0x65,0x72,0x73,0x2e,0x63,0x6f,0x6d
};

static void test_encodeName(DWORD dwEncoding)
{
    CERT_RDN_ATTR attrs[2];
    CERT_RDN rdn;
    CERT_NAME_INFO info;
    static CHAR oid_common_name[] = szOID_COMMON_NAME,
                oid_sur_name[]    = szOID_SUR_NAME;
    BYTE *buf = NULL;
    DWORD size = 0;
    BOOL ret;

    if (0)
    {
        /* Test with NULL pvStructInfo (crashes on win9x) */
        ret = pCryptEncodeObjectEx(dwEncoding, X509_NAME, NULL,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
        ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
         "Expected STATUS_ACCESS_VIOLATION, got %08x\n", GetLastError());
    }
    /* Test with empty CERT_NAME_INFO */
    info.cRDN = 0;
    info.rgRDN = NULL;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(!memcmp(buf, emptySequence, sizeof(emptySequence)),
         "Got unexpected encoding for empty name\n");
        LocalFree(buf);
    }
    if (0)
    {
        /* Test with bogus CERT_RDN (crashes on win9x) */
        info.cRDN = 1;
        ret = pCryptEncodeObjectEx(dwEncoding, X509_NAME, &info,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
        ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
         "Expected STATUS_ACCESS_VIOLATION, got %08x\n", GetLastError());
    }
    /* Test with empty CERT_RDN */
    rdn.cRDNAttr = 0;
    rdn.rgRDNAttr = NULL;
    info.cRDN = 1;
    info.rgRDN = &rdn;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(!memcmp(buf, emptyRDNs, sizeof(emptyRDNs)),
         "Got unexpected encoding for empty RDN array\n");
        LocalFree(buf);
    }
    if (0)
    {
        /* Test with bogus attr array (crashes on win9x) */
        rdn.cRDNAttr = 1;
        rdn.rgRDNAttr = NULL;
        ret = pCryptEncodeObjectEx(dwEncoding, X509_NAME, &info,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
        ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
         "Expected STATUS_ACCESS_VIOLATION, got %08x\n", GetLastError());
    }
    /* oddly, a bogus OID is accepted by Windows XP; not testing.
    attrs[0].pszObjId = "bogus";
    attrs[0].dwValueType = CERT_RDN_PRINTABLE_STRING;
    attrs[0].Value.cbData = sizeof(commonName);
    attrs[0].Value.pbData = commonName;
    rdn.cRDNAttr = 1;
    rdn.rgRDNAttr = attrs;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret, "Expected failure, got success\n");
     */
    /* Check with two CERT_RDN_ATTRs.  Note DER encoding forces the order of
     * the encoded attributes to be swapped.
     */
    attrs[0].pszObjId = oid_common_name;
    attrs[0].dwValueType = CERT_RDN_PRINTABLE_STRING;
    attrs[0].Value.cbData = sizeof(commonName);
    attrs[0].Value.pbData = (BYTE *)commonName;
    attrs[1].pszObjId = oid_sur_name;
    attrs[1].dwValueType = CERT_RDN_PRINTABLE_STRING;
    attrs[1].Value.cbData = sizeof(surName);
    attrs[1].Value.pbData = (BYTE *)surName;
    rdn.cRDNAttr = 2;
    rdn.rgRDNAttr = attrs;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(!memcmp(buf, twoRDNs, sizeof(twoRDNs)),
         "Got unexpected encoding for two RDN array\n");
        LocalFree(buf);
    }
    /* A name can be "encoded" with previously encoded RDN attrs. */
    attrs[0].dwValueType = CERT_RDN_ENCODED_BLOB;
    attrs[0].Value.pbData = (LPBYTE)twoRDNs;
    attrs[0].Value.cbData = sizeof(twoRDNs);
    rdn.cRDNAttr = 1;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(encodedTwoRDNs), "Unexpected size %d\n", size);
        ok(!memcmp(buf, encodedTwoRDNs, size),
         "Unexpected value for re-encoded two RDN array\n");
        LocalFree(buf);
    }
    /* CERT_RDN_ANY_TYPE is too vague for X509_NAMEs, check the return */
    rdn.cRDNAttr = 1;
    attrs[0].dwValueType = CERT_RDN_ANY_TYPE;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* Test a more complex name */
    rdn.cRDNAttr = sizeof(rdnAttrs) / sizeof(rdnAttrs[0]);
    rdn.rgRDNAttr = rdnAttrs;
    info.cRDN = 1;
    info.rgRDN = &rdn;
    buf = NULL;
    size = 0;
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, X509_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(encodedRDNAttrs), "Wrong size %d\n", size);
        ok(!memcmp(buf, encodedRDNAttrs, size), "Unexpected value\n");
        LocalFree(buf);
    }
}

static WCHAR commonNameW[] = { 'J','u','a','n',' ','L','a','n','g',0 };
static WCHAR surNameW[] = { 'L','a','n','g',0 };

static const BYTE twoRDNsNoNull[] = {
 0x30,0x21,0x31,0x1f,0x30,0x0b,0x06,0x03,0x55,0x04,0x04,0x13,0x04,0x4c,0x61,
 0x6e,0x67,0x30,0x10,0x06,0x03,0x55,0x04,0x03,0x13,0x09,0x4a,0x75,0x61,0x6e,
 0x20,0x4c,0x61,0x6e,0x67 };
static const BYTE anyType[] = {
 0x30,0x2f,0x31,0x2d,0x30,0x2b,0x06,0x03,0x55,0x04,0x03,0x1e,0x24,0x23,0x30,
 0x21,0x31,0x0c,0x30,0x03,0x06,0x04,0x55,0x13,0x04,0x4c,0x05,0x6e,0x61,0x00,
 0x67,0x11,0x30,0x03,0x06,0x04,0x55,0x13,0x03,0x4a,0x0a,0x61,0x75,0x20,0x6e,
 0x61,0x4c,0x67,0x6e };

static void test_encodeUnicodeName(DWORD dwEncoding)
{
    CERT_RDN_ATTR attrs[2];
    CERT_RDN rdn;
    CERT_NAME_INFO info;
    static CHAR oid_common_name[] = szOID_COMMON_NAME,
                oid_sur_name[]    = szOID_SUR_NAME;
    BYTE *buf = NULL;
    DWORD size = 0;
    BOOL ret;

    if (0)
    {
        /* Test with NULL pvStructInfo (crashes on win9x) */
        ret = pCryptEncodeObjectEx(dwEncoding, X509_UNICODE_NAME, NULL,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
        ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
         "Expected STATUS_ACCESS_VIOLATION, got %08x\n", GetLastError());
    }
    /* Test with empty CERT_NAME_INFO */
    info.cRDN = 0;
    info.rgRDN = NULL;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_UNICODE_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(!memcmp(buf, emptySequence, sizeof(emptySequence)),
         "Got unexpected encoding for empty name\n");
        LocalFree(buf);
    }
    /* Check with one CERT_RDN_ATTR, that has an invalid character for the
     * encoding (the NULL).
     */
    attrs[0].pszObjId = oid_common_name;
    attrs[0].dwValueType = CERT_RDN_PRINTABLE_STRING;
    attrs[0].Value.cbData = sizeof(commonNameW);
    attrs[0].Value.pbData = (BYTE *)commonNameW;
    rdn.cRDNAttr = 1;
    rdn.rgRDNAttr = attrs;
    info.cRDN = 1;
    info.rgRDN = &rdn;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_UNICODE_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_PRINTABLE_STRING,
     "Expected CRYPT_E_INVALID_PRINTABLE_STRING, got %08x\n", GetLastError());
    ok(size == 9, "Unexpected error index %08x\n", size);
    /* Check with two NULL-terminated CERT_RDN_ATTRs.  Note DER encoding
     * forces the order of the encoded attributes to be swapped.
     */
    attrs[0].pszObjId = oid_common_name;
    attrs[0].dwValueType = CERT_RDN_PRINTABLE_STRING;
    attrs[0].Value.cbData = 0;
    attrs[0].Value.pbData = (BYTE *)commonNameW;
    attrs[1].pszObjId = oid_sur_name;
    attrs[1].dwValueType = CERT_RDN_PRINTABLE_STRING;
    attrs[1].Value.cbData = 0;
    attrs[1].Value.pbData = (BYTE *)surNameW;
    rdn.cRDNAttr = 2;
    rdn.rgRDNAttr = attrs;
    info.cRDN = 1;
    info.rgRDN = &rdn;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_UNICODE_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(!memcmp(buf, twoRDNsNoNull, sizeof(twoRDNsNoNull)),
         "Got unexpected encoding for two RDN array\n");
        LocalFree(buf);
    }
    /* A name can be "encoded" with previously encoded RDN attrs. */
    attrs[0].dwValueType = CERT_RDN_ENCODED_BLOB;
    attrs[0].Value.pbData = (LPBYTE)twoRDNs;
    attrs[0].Value.cbData = sizeof(twoRDNs);
    rdn.cRDNAttr = 1;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_UNICODE_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(encodedTwoRDNs), "Unexpected size %d\n", size);
        ok(!memcmp(buf, encodedTwoRDNs, size),
         "Unexpected value for re-encoded two RDN array\n");
        LocalFree(buf);
    }
    /* Unicode names infer the type for CERT_RDN_ANY_TYPE */
    rdn.cRDNAttr = 1;
    attrs[0].dwValueType = CERT_RDN_ANY_TYPE;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_UNICODE_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    todo_wine ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(anyType), "Unexpected size %d\n", size);
        ok(!memcmp(buf, anyType, size), "Unexpected value\n");
        LocalFree(buf);
    }
}

static void compareNameValues(const CERT_NAME_VALUE *expected,
 const CERT_NAME_VALUE *got)
{
    if (expected->dwValueType == CERT_RDN_UTF8_STRING &&
        got->dwValueType == CERT_RDN_ENCODED_BLOB)
    {
        win_skip("Can't handle CERT_RDN_UTF8_STRING\n");
        return;
    }

    ok(got->dwValueType == expected->dwValueType,
     "Expected string type %d, got %d\n", expected->dwValueType,
     got->dwValueType);
    ok(got->Value.cbData == expected->Value.cbData ||
     got->Value.cbData == expected->Value.cbData - sizeof(WCHAR) /* Win8 */,
     "String type %d: unexpected data size, got %d, expected %d\n",
     expected->dwValueType, got->Value.cbData, expected->Value.cbData);
    if (got->Value.cbData && got->Value.pbData)
        ok(!memcmp(got->Value.pbData, expected->Value.pbData,
         min(got->Value.cbData, expected->Value.cbData)),
         "String type %d: unexpected value\n", expected->dwValueType);
}

static void compareRDNAttrs(const CERT_RDN_ATTR *expected,
 const CERT_RDN_ATTR *got)
{
    if (expected->pszObjId && strlen(expected->pszObjId))
    {
        ok(got->pszObjId != NULL, "Expected OID %s, got NULL\n",
         expected->pszObjId);
        if (got->pszObjId)
        {
            ok(!strcmp(got->pszObjId, expected->pszObjId),
             "Got unexpected OID %s, expected %s\n", got->pszObjId,
             expected->pszObjId);
        }
    }
    compareNameValues((const CERT_NAME_VALUE *)&expected->dwValueType,
     (const CERT_NAME_VALUE *)&got->dwValueType);
}

static void compareRDNs(const CERT_RDN *expected, const CERT_RDN *got)
{
    ok(got->cRDNAttr == expected->cRDNAttr,
     "Expected %d RDN attrs, got %d\n", expected->cRDNAttr, got->cRDNAttr);
    if (got->cRDNAttr)
    {
        DWORD i;

        for (i = 0; i < got->cRDNAttr; i++)
            compareRDNAttrs(&expected->rgRDNAttr[i], &got->rgRDNAttr[i]);
    }
}

static void compareNames(const CERT_NAME_INFO *expected,
 const CERT_NAME_INFO *got)
{
    ok(got->cRDN == expected->cRDN, "Expected %d RDNs, got %d\n",
     expected->cRDN, got->cRDN);
    if (got->cRDN)
    {
        DWORD i;

        for (i = 0; i < got->cRDN; i++)
            compareRDNs(&expected->rgRDN[i], &got->rgRDN[i]);
    }
}

static const BYTE emptyIndefiniteSequence[] = { 0x30,0x80,0x00,0x00 };
static const BYTE twoRDNsExtraBytes[] = {
    0x30,0x23,0x31,0x21,0x30,0x0c,0x06,0x03,0x55,0x04,0x04,
    0x13,0x05,0x4c,0x61,0x6e,0x67,0x00,0x30,0x11,0x06,0x03,0x55,0x04,0x03,
    0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0,0,0,0,0,0};

static void test_decodeName(DWORD dwEncoding)
{
    BYTE *buf = NULL;
    DWORD bufSize = 0;
    BOOL ret;
    CERT_RDN rdn;
    CERT_NAME_INFO info = { 1, &rdn };

    /* test empty name */
    bufSize = 0;
    ret = pCryptDecodeObjectEx(dwEncoding, X509_NAME, emptySequence,
     emptySequence[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_SHARE_OID_STRING_FLAG, NULL,
     &buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    /* Interestingly, in Windows, if cRDN is 0, rgRGN may not be NULL.  My
     * decoder works the same way, so only test the count.
     */
    if (ret)
    {
        ok(bufSize == sizeof(CERT_NAME_INFO), "Wrong bufSize %d\n", bufSize);
        ok(((CERT_NAME_INFO *)buf)->cRDN == 0,
         "Expected 0 RDNs in empty info, got %d\n",
         ((CERT_NAME_INFO *)buf)->cRDN);
        LocalFree(buf);
    }
    /* test empty name with indefinite-length encoding */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_NAME, emptyIndefiniteSequence,
     sizeof(emptyIndefiniteSequence), CRYPT_DECODE_ALLOC_FLAG, NULL,
     &buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(bufSize == sizeof(CERT_NAME_INFO), "Wrong bufSize %d\n", bufSize);
        ok(((CERT_NAME_INFO *)buf)->cRDN == 0,
         "Expected 0 RDNs in empty info, got %d\n",
         ((CERT_NAME_INFO *)buf)->cRDN);
        LocalFree(buf);
    }
    /* test empty RDN */
    bufSize = 0;
    ret = pCryptDecodeObjectEx(dwEncoding, X509_NAME, emptyRDNs,
     emptyRDNs[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_SHARE_OID_STRING_FLAG, NULL,
     &buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CERT_NAME_INFO *info = (CERT_NAME_INFO *)buf;

        ok(bufSize == sizeof(CERT_NAME_INFO) + sizeof(CERT_RDN) &&
         info->cRDN == 1 && info->rgRDN && info->rgRDN[0].cRDNAttr == 0,
         "Got unexpected value for empty RDN\n");
        LocalFree(buf);
    }
    /* test two RDN attrs */
    bufSize = 0;
    ret = pCryptDecodeObjectEx(dwEncoding, X509_NAME, twoRDNs,
     twoRDNs[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_SHARE_OID_STRING_FLAG, NULL,
     &buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        static CHAR oid_sur_name[]    = szOID_SUR_NAME,
                    oid_common_name[] = szOID_COMMON_NAME;

        CERT_RDN_ATTR attrs[] = {
         { oid_sur_name, CERT_RDN_PRINTABLE_STRING, { sizeof(surName),
          (BYTE *)surName } },
         { oid_common_name, CERT_RDN_PRINTABLE_STRING, { sizeof(commonName),
          (BYTE *)commonName } },
        };

        rdn.cRDNAttr = sizeof(attrs) / sizeof(attrs[0]);
        rdn.rgRDNAttr = attrs;
        compareNames(&info, (CERT_NAME_INFO *)buf);
        LocalFree(buf);
    }
    /* test that two RDN attrs with extra bytes succeeds */
    bufSize = 0;
    ret = pCryptDecodeObjectEx(dwEncoding, X509_NAME, twoRDNsExtraBytes,
     sizeof(twoRDNsExtraBytes), 0, NULL, NULL, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    /* And, a slightly more complicated name */
    buf = NULL;
    bufSize = 0;
    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, X509_NAME, encodedRDNAttrs,
     sizeof(encodedRDNAttrs), CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        rdn.cRDNAttr = sizeof(decodedRdnAttrs) / sizeof(decodedRdnAttrs[0]);
        rdn.rgRDNAttr = decodedRdnAttrs;
        compareNames(&info, (CERT_NAME_INFO *)buf);
        LocalFree(buf);
    }
}

static void test_decodeUnicodeName(DWORD dwEncoding)
{
    BYTE *buf = NULL;
    DWORD bufSize = 0;
    BOOL ret;
    CERT_RDN rdn;
    CERT_NAME_INFO info = { 1, &rdn };

    /* test empty name */
    bufSize = 0;
    ret = pCryptDecodeObjectEx(dwEncoding, X509_UNICODE_NAME, emptySequence,
     emptySequence[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_SHARE_OID_STRING_FLAG, NULL,
     &buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(bufSize == sizeof(CERT_NAME_INFO),
         "Got wrong bufSize %d\n", bufSize);
        ok(((CERT_NAME_INFO *)buf)->cRDN == 0,
         "Expected 0 RDNs in empty info, got %d\n",
         ((CERT_NAME_INFO *)buf)->cRDN);
        LocalFree(buf);
    }
    /* test empty RDN */
    bufSize = 0;
    ret = pCryptDecodeObjectEx(dwEncoding, X509_UNICODE_NAME, emptyRDNs,
     emptyRDNs[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_SHARE_OID_STRING_FLAG, NULL,
     &buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CERT_NAME_INFO *info = (CERT_NAME_INFO *)buf;

        ok(bufSize == sizeof(CERT_NAME_INFO) + sizeof(CERT_RDN) &&
         info->cRDN == 1 && info->rgRDN && info->rgRDN[0].cRDNAttr == 0,
         "Got unexpected value for empty RDN\n");
        LocalFree(buf);
    }
    /* test two RDN attrs */
    bufSize = 0;
    ret = pCryptDecodeObjectEx(dwEncoding, X509_UNICODE_NAME, twoRDNsNoNull,
     sizeof(twoRDNsNoNull),
     CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_SHARE_OID_STRING_FLAG, NULL,
     &buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        static CHAR oid_sur_name[]    = szOID_SUR_NAME,
                    oid_common_name[] = szOID_COMMON_NAME;

        CERT_RDN_ATTR attrs[] = {
         { oid_sur_name, CERT_RDN_PRINTABLE_STRING,
         { lstrlenW(surNameW) * sizeof(WCHAR), (BYTE *)surNameW } },
         { oid_common_name, CERT_RDN_PRINTABLE_STRING,
         { lstrlenW(commonNameW) * sizeof(WCHAR), (BYTE *)commonNameW } },
        };

        rdn.cRDNAttr = sizeof(attrs) / sizeof(attrs[0]);
        rdn.rgRDNAttr = attrs;
        compareNames(&info, (CERT_NAME_INFO *)buf);
        LocalFree(buf);
    }
}

struct EncodedNameValue
{
    CERT_NAME_VALUE value;
    const BYTE *encoded;
    DWORD encodedSize;
};

static const char bogusIA5[] = "\x80";
static const char bogusPrintable[] = "~";
static const char bogusNumeric[] = "A";
static const BYTE bin42[] = { 0x16,0x02,0x80,0x00 };
static const BYTE bin43[] = { 0x13,0x02,0x7e,0x00 };
static const BYTE bin44[] = { 0x12,0x02,0x41,0x00 };
static BYTE octetCommonNameValue[] = {
 0x04,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00 };
static BYTE numericCommonNameValue[] = {
 0x12,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00 };
static BYTE printableCommonNameValue[] = {
 0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00 };
static BYTE t61CommonNameValue[] = {
 0x14,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00 };
static BYTE videotexCommonNameValue[] = {
 0x15,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00 };
static BYTE ia5CommonNameValue[] = {
 0x16,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00 };
static BYTE graphicCommonNameValue[] = {
 0x19,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00 };
static BYTE visibleCommonNameValue[] = {
 0x1a,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00 };
static BYTE generalCommonNameValue[] = {
 0x1b,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00 };
static BYTE bmpCommonNameValue[] = {
 0x1e,0x14,0x00,0x4a,0x00,0x75,0x00,0x61,0x00,0x6e,0x00,0x20,0x00,0x4c,0x00,
 0x61,0x00,0x6e,0x00,0x67,0x00,0x00 };
static BYTE utf8CommonNameValue[] = {
 0x0c,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00 };
static char embedded_null[] = "foo\0com";
static BYTE ia5EmbeddedNull[] = {
 0x16,0x07,0x66,0x6f,0x6f,0x00,0x63,0x6f,0x6d };

static struct EncodedNameValue nameValues[] = {
 { { CERT_RDN_OCTET_STRING, { sizeof(commonName), (BYTE *)commonName } },
     octetCommonNameValue, sizeof(octetCommonNameValue) },
 { { CERT_RDN_NUMERIC_STRING, { sizeof(commonName), (BYTE *)commonName } },
     numericCommonNameValue, sizeof(numericCommonNameValue) },
 { { CERT_RDN_PRINTABLE_STRING, { sizeof(commonName), (BYTE *)commonName } },
     printableCommonNameValue, sizeof(printableCommonNameValue) },
 { { CERT_RDN_T61_STRING, { sizeof(commonName), (BYTE *)commonName } },
     t61CommonNameValue, sizeof(t61CommonNameValue) },
 { { CERT_RDN_VIDEOTEX_STRING, { sizeof(commonName), (BYTE *)commonName } },
     videotexCommonNameValue, sizeof(videotexCommonNameValue) },
 { { CERT_RDN_IA5_STRING, { sizeof(commonName), (BYTE *)commonName } },
     ia5CommonNameValue, sizeof(ia5CommonNameValue) },
 { { CERT_RDN_GRAPHIC_STRING, { sizeof(commonName), (BYTE *)commonName } },
     graphicCommonNameValue, sizeof(graphicCommonNameValue) },
 { { CERT_RDN_VISIBLE_STRING, { sizeof(commonName), (BYTE *)commonName } },
     visibleCommonNameValue, sizeof(visibleCommonNameValue) },
 { { CERT_RDN_GENERAL_STRING, { sizeof(commonName), (BYTE *)commonName } },
     generalCommonNameValue, sizeof(generalCommonNameValue) },
 { { CERT_RDN_BMP_STRING, { sizeof(commonNameW), (BYTE *)commonNameW } },
     bmpCommonNameValue, sizeof(bmpCommonNameValue) },
 { { CERT_RDN_UTF8_STRING, { sizeof(commonNameW), (BYTE *)commonNameW } },
     utf8CommonNameValue, sizeof(utf8CommonNameValue) },
 /* The following tests succeed under Windows, but really should fail,
  * they contain characters that are illegal for the encoding.  I'm
  * including them to justify my lazy encoding.
  */
 { { CERT_RDN_IA5_STRING, { sizeof(bogusIA5), (BYTE *)bogusIA5 } }, bin42,
     sizeof(bin42) },
 { { CERT_RDN_PRINTABLE_STRING, { sizeof(bogusPrintable),
     (BYTE *)bogusPrintable } }, bin43, sizeof(bin43) },
 { { CERT_RDN_NUMERIC_STRING, { sizeof(bogusNumeric), (BYTE *)bogusNumeric } },
     bin44, sizeof(bin44) },
};
/* This is kept separate, because the decoding doesn't return to the original
 * value.
 */
static struct EncodedNameValue embeddedNullNameValue = {
 { CERT_RDN_IA5_STRING, { sizeof(embedded_null) - 1, (BYTE *)embedded_null } },
   ia5EmbeddedNull, sizeof(ia5EmbeddedNull) };

static void test_encodeNameValue(DWORD dwEncoding)
{
    BYTE *buf = NULL;
    DWORD size = 0, i;
    BOOL ret;
    CERT_NAME_VALUE value = { 0, { 0, NULL } };

    value.dwValueType = CERT_RDN_ENCODED_BLOB;
    value.Value.pbData = printableCommonNameValue;
    value.Value.cbData = sizeof(printableCommonNameValue);
    ret = pCryptEncodeObjectEx(dwEncoding, X509_NAME_VALUE, &value,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(printableCommonNameValue), "Unexpected size %d\n",
         size);
        ok(!memcmp(buf, printableCommonNameValue, size),
         "Unexpected encoding\n");
        LocalFree(buf);
    }
    for (i = 0; i < sizeof(nameValues) / sizeof(nameValues[0]); i++)
    {
        ret = pCryptEncodeObjectEx(dwEncoding, X509_NAME_VALUE,
         &nameValues[i].value, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
        ok(ret || broken(GetLastError() == OSS_PDU_MISMATCH) /* NT4/Win9x */,
         "Type %d: CryptEncodeObjectEx failed: %08x\n",
         nameValues[i].value.dwValueType, GetLastError());
        if (ret)
        {
            ok(size == nameValues[i].encodedSize,
             "Expected size %d, got %d\n", nameValues[i].encodedSize, size);
            ok(!memcmp(buf, nameValues[i].encoded, size),
             "Got unexpected encoding\n");
            LocalFree(buf);
        }
    }
    ret = pCryptEncodeObjectEx(dwEncoding, X509_NAME_VALUE,
     &embeddedNullNameValue.value, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret || broken(GetLastError() == OSS_PDU_MISMATCH) /* NT4/Win9x */,
     "Type %d: CryptEncodeObjectEx failed: %08x\n",
     embeddedNullNameValue.value.dwValueType, GetLastError());
    if (ret)
    {
        ok(size == embeddedNullNameValue.encodedSize,
         "Expected size %d, got %d\n", embeddedNullNameValue.encodedSize, size);
        ok(!memcmp(buf, embeddedNullNameValue.encoded, size),
         "Got unexpected encoding\n");
        LocalFree(buf);
    }
}

static void test_decodeNameValue(DWORD dwEncoding)
{
    int i;
    BYTE *buf = NULL;
    DWORD bufSize = 0;
    BOOL ret;

    for (i = 0; i < sizeof(nameValues) / sizeof(nameValues[0]); i++)
    {
        ret = pCryptDecodeObjectEx(dwEncoding, X509_NAME_VALUE,
         nameValues[i].encoded, nameValues[i].encoded[1] + 2,
         CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_SHARE_OID_STRING_FLAG, NULL,
         &buf, &bufSize);
        ok(ret, "Value type %d: CryptDecodeObjectEx failed: %08x\n",
         nameValues[i].value.dwValueType, GetLastError());
        if (ret)
        {
            compareNameValues(&nameValues[i].value,
             (const CERT_NAME_VALUE *)buf);
            LocalFree(buf);
        }
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_NAME_VALUE,
     embeddedNullNameValue.encoded, embeddedNullNameValue.encodedSize,
     CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_SHARE_OID_STRING_FLAG, NULL,
     &buf, &bufSize);
    /* Some Windows versions disallow name values with embedded NULLs, so
     * either success or failure is acceptable.
     */
    if (ret)
    {
        CERT_NAME_VALUE rdnEncodedValue = { CERT_RDN_ENCODED_BLOB,
         { sizeof(ia5EmbeddedNull), ia5EmbeddedNull } };
        CERT_NAME_VALUE embeddedNullValue = { CERT_RDN_IA5_STRING,
         { sizeof(embedded_null) - 1, (BYTE *)embedded_null } };
        const CERT_NAME_VALUE *got = (const CERT_NAME_VALUE *)buf,
         *expected = NULL;

        /* Some Windows versions decode name values with embedded NULLs,
         * others leave them encoded, even with the same version of crypt32.
         * Accept either.
         */
        ok(got->dwValueType == CERT_RDN_ENCODED_BLOB ||
         got->dwValueType == CERT_RDN_IA5_STRING,
         "Expected CERT_RDN_ENCODED_BLOB or CERT_RDN_IA5_STRING, got %d\n",
         got->dwValueType);
        if (got->dwValueType == CERT_RDN_ENCODED_BLOB)
            expected = &rdnEncodedValue;
        else if (got->dwValueType == CERT_RDN_IA5_STRING)
            expected = &embeddedNullValue;
        if (expected)
        {
            ok(got->Value.cbData == expected->Value.cbData,
             "String type %d: unexpected data size, got %d, expected %d\n",
             got->dwValueType, got->Value.cbData, expected->Value.cbData);
            if (got->Value.cbData && got->Value.pbData)
                ok(!memcmp(got->Value.pbData, expected->Value.pbData,
                 min(got->Value.cbData, expected->Value.cbData)),
                 "String type %d: unexpected value\n", expected->dwValueType);
        }
        LocalFree(buf);
    }
}

static const BYTE emptyURL[] = { 0x30, 0x02, 0x86, 0x00 };
static const BYTE emptyURLExtraBytes[] = { 0x30, 0x02, 0x86, 0x00, 0, 0, 0 };
static const WCHAR url[] = { 'h','t','t','p',':','/','/','w','i','n','e',
 'h','q','.','o','r','g',0 };
static const BYTE encodedURL[] = { 0x30, 0x13, 0x86, 0x11, 0x68, 0x74,
 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x77, 0x69, 0x6e, 0x65, 0x68, 0x71, 0x2e,
 0x6f, 0x72, 0x67 };
static const WCHAR nihongoURL[] = { 'h','t','t','p',':','/','/',0x226f,
 0x575b, 0 };
static const WCHAR dnsName[] = { 'w','i','n','e','h','q','.','o','r','g',0 };
static const BYTE encodedDnsName[] = { 0x30, 0x0c, 0x82, 0x0a, 0x77, 0x69,
 0x6e, 0x65, 0x68, 0x71, 0x2e, 0x6f, 0x72, 0x67 };
static const BYTE localhost[] = { 127, 0, 0, 1 };
static const BYTE encodedIPAddr[] = { 0x30, 0x06, 0x87, 0x04, 0x7f, 0x00, 0x00,
 0x01 };
static const unsigned char encodedCommonName[] = {
    0x30,0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,'J','u','a','n',' ','L','a','n','g',0};
static const BYTE encodedOidName[] = { 0x30,0x04,0x88,0x02,0x2a,0x03 };
static const BYTE encodedDirectoryName[] = {
0x30,0x19,0xa4,0x17,0x30,0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,
0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00 };

static void test_encodeAltName(DWORD dwEncoding)
{
    CERT_ALT_NAME_INFO info = { 0 };
    CERT_ALT_NAME_ENTRY entry = { 0 };
    BYTE *buf = NULL;
    DWORD size = 0;
    BOOL ret;
    char oid[] = "1.2.3";

    /* Test with empty info */
    ret = pCryptEncodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (ret)
    {
        ok(size == sizeof(emptySequence), "Wrong size %d\n", size);
        ok(!memcmp(buf, emptySequence, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* Test with an empty entry */
    info.cAltEntry = 1;
    info.rgAltEntry = &entry;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* Test with an empty pointer */
    entry.dwAltNameChoice = CERT_ALT_NAME_URL;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (ret)
    {
        ok(size == sizeof(emptyURL), "Wrong size %d\n", size);
        ok(!memcmp(buf, emptyURL, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* Test with a real URL */
    U(entry).pwszURL = (LPWSTR)url;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (ret)
    {
        ok(size == sizeof(encodedURL), "Wrong size %d\n", size);
        ok(!memcmp(buf, encodedURL, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* Now with the URL containing an invalid IA5 char */
    U(entry).pwszURL = (LPWSTR)nihongoURL;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_IA5_STRING,
     "Expected CRYPT_E_INVALID_IA5_STRING, got %08x\n", GetLastError());
    /* The first invalid character is at index 7 */
    ok(GET_CERT_ALT_NAME_VALUE_ERR_INDEX(size) == 7,
     "Expected invalid char at index 7, got %d\n",
     GET_CERT_ALT_NAME_VALUE_ERR_INDEX(size));
    /* Now with the URL missing a scheme */
    U(entry).pwszURL = (LPWSTR)dnsName;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        /* This succeeds, but it shouldn't, so don't worry about conforming */
        LocalFree(buf);
    }
    /* Now with a DNS name */
    entry.dwAltNameChoice = CERT_ALT_NAME_DNS_NAME;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(encodedDnsName), "Wrong size %d\n", size);
        ok(!memcmp(buf, encodedDnsName, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* Test with an IP address */
    entry.dwAltNameChoice = CERT_ALT_NAME_IP_ADDRESS;
    U(entry).IPAddress.cbData = sizeof(localhost);
    U(entry).IPAddress.pbData = (LPBYTE)localhost;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (ret)
    {
        ok(size == sizeof(encodedIPAddr), "Wrong size %d\n", size);
        ok(!memcmp(buf, encodedIPAddr, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* Test with OID */
    entry.dwAltNameChoice = CERT_ALT_NAME_REGISTERED_ID;
    U(entry).pszRegisteredID = oid;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (ret)
    {
        ok(size == sizeof(encodedOidName), "Wrong size %d\n", size);
        ok(!memcmp(buf, encodedOidName, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* Test with directory name */
    entry.dwAltNameChoice = CERT_ALT_NAME_DIRECTORY_NAME;
    U(entry).DirectoryName.cbData = sizeof(encodedCommonName);
    U(entry).DirectoryName.pbData = (LPBYTE)encodedCommonName;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (ret)
    {
        ok(size == sizeof(encodedDirectoryName), "Wrong size %d\n", size);
        ok(!memcmp(buf, encodedDirectoryName, size), "Unexpected value\n");
        LocalFree(buf);
    }
}

static void test_decodeAltName(DWORD dwEncoding)
{
    static const BYTE unimplementedType[] = { 0x30, 0x06, 0x85, 0x04, 0x7f,
     0x00, 0x00, 0x01 };
    static const BYTE bogusType[] = { 0x30, 0x06, 0x89, 0x04, 0x7f, 0x00, 0x00,
     0x01 };
    static const BYTE dns_embedded_null[] = { 0x30,0x10,0x82,0x0e,0x66,0x6f,
     0x6f,0x2e,0x63,0x6f,0x6d,0x00,0x62,0x61,0x64,0x64,0x69,0x65 };
    static const BYTE dns_embedded_bell[] = { 0x30,0x10,0x82,0x0e,0x66,0x6f,
     0x6f,0x2e,0x63,0x6f,0x6d,0x07,0x62,0x61,0x64,0x64,0x69,0x65 };
    static const BYTE url_embedded_null[] = { 0x30,0x10,0x86,0x0e,0x66,0x6f,
     0x6f,0x2e,0x63,0x6f,0x6d,0x00,0x62,0x61,0x64,0x64,0x69,0x65 };
    BOOL ret;
    BYTE *buf = NULL;
    DWORD bufSize = 0;
    CERT_ALT_NAME_INFO *info;

    /* Test some bogus ones first */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_ALTERNATE_NAME,
     unimplementedType, sizeof(unimplementedType), CRYPT_DECODE_ALLOC_FLAG,
     NULL, &buf, &bufSize);
    ok(!ret && (GetLastError() == CRYPT_E_ASN1_BADTAG ||
     GetLastError() == OSS_DATA_ERROR /* Win9x */),
     "Expected CRYPT_E_ASN1_BADTAG or OSS_DATA_ERROR, got %08x\n",
     GetLastError());
    ret = pCryptDecodeObjectEx(dwEncoding, X509_ALTERNATE_NAME,
     bogusType, sizeof(bogusType), CRYPT_DECODE_ALLOC_FLAG, NULL, &buf,
     &bufSize);
    ok(!ret && (GetLastError() == CRYPT_E_ASN1_CORRUPT ||
     GetLastError() == OSS_DATA_ERROR /* Win9x */),
     "Expected CRYPT_E_ASN1_CORRUPT or OSS_DATA_ERROR, got %08x\n",
     GetLastError());
    /* Now expected cases */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, emptySequence,
     emptySequence[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        info = (CERT_ALT_NAME_INFO *)buf;

        ok(info->cAltEntry == 0, "Expected 0 entries, got %d\n",
         info->cAltEntry);
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, emptyURL,
     emptyURL[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        info = (CERT_ALT_NAME_INFO *)buf;

        ok(info->cAltEntry == 1, "Expected 1 entries, got %d\n",
         info->cAltEntry);
        ok(info->rgAltEntry[0].dwAltNameChoice == CERT_ALT_NAME_URL,
         "Expected CERT_ALT_NAME_URL, got %d\n",
         info->rgAltEntry[0].dwAltNameChoice);
        ok(U(info->rgAltEntry[0]).pwszURL == NULL || !*U(info->rgAltEntry[0]).pwszURL,
         "Expected empty URL\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_ALTERNATE_NAME,
     emptyURLExtraBytes, sizeof(emptyURLExtraBytes), 0, NULL, NULL, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    ret = pCryptDecodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, encodedURL,
     encodedURL[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        info = (CERT_ALT_NAME_INFO *)buf;

        ok(info->cAltEntry == 1, "Expected 1 entries, got %d\n",
         info->cAltEntry);
        ok(info->rgAltEntry[0].dwAltNameChoice == CERT_ALT_NAME_URL,
         "Expected CERT_ALT_NAME_URL, got %d\n",
         info->rgAltEntry[0].dwAltNameChoice);
        ok(!lstrcmpW(U(info->rgAltEntry[0]).pwszURL, url), "Unexpected URL\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, encodedDnsName,
     encodedDnsName[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        info = (CERT_ALT_NAME_INFO *)buf;

        ok(info->cAltEntry == 1, "Expected 1 entries, got %d\n",
         info->cAltEntry);
        ok(info->rgAltEntry[0].dwAltNameChoice == CERT_ALT_NAME_DNS_NAME,
         "Expected CERT_ALT_NAME_DNS_NAME, got %d\n",
         info->rgAltEntry[0].dwAltNameChoice);
        ok(!lstrcmpW(U(info->rgAltEntry[0]).pwszDNSName, dnsName),
         "Unexpected DNS name\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, encodedIPAddr,
     encodedIPAddr[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        info = (CERT_ALT_NAME_INFO *)buf;

        ok(info->cAltEntry == 1, "Expected 1 entries, got %d\n",
         info->cAltEntry);
        ok(info->rgAltEntry[0].dwAltNameChoice == CERT_ALT_NAME_IP_ADDRESS,
         "Expected CERT_ALT_NAME_IP_ADDRESS, got %d\n",
         info->rgAltEntry[0].dwAltNameChoice);
        ok(U(info->rgAltEntry[0]).IPAddress.cbData == sizeof(localhost),
         "Unexpected IP address length %d\n",
          U(info->rgAltEntry[0]).IPAddress.cbData);
        ok(!memcmp(U(info->rgAltEntry[0]).IPAddress.pbData, localhost,
         sizeof(localhost)), "Unexpected IP address value\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, encodedOidName,
     sizeof(encodedOidName), CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        info = (CERT_ALT_NAME_INFO *)buf;

        ok(info->cAltEntry == 1, "Expected 1 entries, got %d\n",
         info->cAltEntry);
        ok(info->rgAltEntry[0].dwAltNameChoice == CERT_ALT_NAME_REGISTERED_ID,
         "Expected CERT_ALT_NAME_REGISTERED_ID, got %d\n",
         info->rgAltEntry[0].dwAltNameChoice);
        ok(!strcmp(U(info->rgAltEntry[0]).pszRegisteredID, "1.2.3"),
           "Expected OID 1.2.3, got %s\n", U(info->rgAltEntry[0]).pszRegisteredID);
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_ALTERNATE_NAME,
     encodedDirectoryName, sizeof(encodedDirectoryName),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        info = (CERT_ALT_NAME_INFO *)buf;

        ok(info->cAltEntry == 1, "Expected 1 entries, got %d\n",
         info->cAltEntry);
        ok(info->rgAltEntry[0].dwAltNameChoice == CERT_ALT_NAME_DIRECTORY_NAME,
         "Expected CERT_ALT_NAME_DIRECTORY_NAME, got %d\n",
         info->rgAltEntry[0].dwAltNameChoice);
        ok(U(info->rgAltEntry[0]).DirectoryName.cbData ==
         sizeof(encodedCommonName), "Unexpected directory name length %d\n",
          U(info->rgAltEntry[0]).DirectoryName.cbData);
        ok(!memcmp(U(info->rgAltEntry[0]).DirectoryName.pbData,
         encodedCommonName, sizeof(encodedCommonName)),
         "Unexpected directory name value\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_ALTERNATE_NAME,
     dns_embedded_null, sizeof(dns_embedded_null), CRYPT_DECODE_ALLOC_FLAG,
     NULL, &buf, &bufSize);
    /* Fails on WinXP with CRYPT_E_ASN1_RULE.  I'm not too concerned about the
     * particular failure, just that it doesn't decode.
     * It succeeds on (broken) Windows versions that haven't addressed
     * embedded NULLs in alternate names.
     */
    ok(!ret || broken(ret), "expected failure\n");
    /* An embedded bell character is allowed, however. */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_ALTERNATE_NAME,
     dns_embedded_bell, sizeof(dns_embedded_bell), CRYPT_DECODE_ALLOC_FLAG,
     NULL, &buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        info = (CERT_ALT_NAME_INFO *)buf;

        ok(info->cAltEntry == 1, "Expected 1 entries, got %d\n",
         info->cAltEntry);
        ok(info->rgAltEntry[0].dwAltNameChoice == CERT_ALT_NAME_DNS_NAME,
         "Expected CERT_ALT_NAME_DNS_NAME, got %d\n",
         info->rgAltEntry[0].dwAltNameChoice);
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_ALTERNATE_NAME,
     url_embedded_null, sizeof(dns_embedded_null), CRYPT_DECODE_ALLOC_FLAG,
     NULL, &buf, &bufSize);
    /* Again, fails on WinXP with CRYPT_E_ASN1_RULE.  I'm not too concerned
     * about the particular failure, just that it doesn't decode.
     * It succeeds on (broken) Windows versions that haven't addressed
     * embedded NULLs in alternate names.
     */
    ok(!ret || broken(ret), "expected failure\n");
}

struct UnicodeExpectedError
{
    DWORD   valueType;
    LPCWSTR str;
    DWORD   errorIndex;
    DWORD   error;
};

static const WCHAR oneW[] = { '1',0 };
static const WCHAR aW[] = { 'a',0 };
static const WCHAR quoteW[] = { '"', 0 };

static struct UnicodeExpectedError unicodeErrors[] = {
 { CERT_RDN_ANY_TYPE,         oneW,       0, CRYPT_E_NOT_CHAR_STRING },
 { CERT_RDN_ENCODED_BLOB,     oneW,       0, CRYPT_E_NOT_CHAR_STRING },
 { CERT_RDN_OCTET_STRING,     oneW,       0, CRYPT_E_NOT_CHAR_STRING },
 { CERT_RDN_NUMERIC_STRING,   aW,         0, CRYPT_E_INVALID_NUMERIC_STRING },
 { CERT_RDN_PRINTABLE_STRING, quoteW,     0, CRYPT_E_INVALID_PRINTABLE_STRING },
 { CERT_RDN_IA5_STRING,       nihongoURL, 7, CRYPT_E_INVALID_IA5_STRING },
};

struct UnicodeExpectedResult
{
    DWORD           valueType;
    LPCWSTR         str;
    CRYPT_DATA_BLOB encoded;
};

static BYTE oneNumeric[] = { 0x12, 0x01, 0x31 };
static BYTE onePrintable[] = { 0x13, 0x01, 0x31 };
static BYTE oneTeletex[] = { 0x14, 0x01, 0x31 };
static BYTE oneVideotex[] = { 0x15, 0x01, 0x31 };
static BYTE oneIA5[] = { 0x16, 0x01, 0x31 };
static BYTE oneGraphic[] = { 0x19, 0x01, 0x31 };
static BYTE oneVisible[] = { 0x1a, 0x01, 0x31 };
static BYTE oneUniversal[] = { 0x1c, 0x04, 0x00, 0x00, 0x00, 0x31 };
static BYTE oneGeneral[] = { 0x1b, 0x01, 0x31 };
static BYTE oneBMP[] = { 0x1e, 0x02, 0x00, 0x31 };
static BYTE oneUTF8[] = { 0x0c, 0x01, 0x31 };
static BYTE nihongoT61[] = { 0x14,0x09,0x68,0x74,0x74,0x70,0x3a,0x2f,0x2f,0x6f,
 0x5b };
static BYTE nihongoGeneral[] = { 0x1b,0x09,0x68,0x74,0x74,0x70,0x3a,0x2f,0x2f,
 0x6f,0x5b };
static BYTE nihongoBMP[] = { 0x1e,0x12,0x00,0x68,0x00,0x74,0x00,0x74,0x00,0x70,
 0x00,0x3a,0x00,0x2f,0x00,0x2f,0x22,0x6f,0x57,0x5b };
static BYTE nihongoUTF8[] = { 0x0c,0x0d,0x68,0x74,0x74,0x70,0x3a,0x2f,0x2f,
 0xe2,0x89,0xaf,0xe5,0x9d,0x9b };

static struct UnicodeExpectedResult unicodeResults[] = {
 { CERT_RDN_NUMERIC_STRING,   oneW, { sizeof(oneNumeric), oneNumeric } },
 { CERT_RDN_PRINTABLE_STRING, oneW, { sizeof(onePrintable), onePrintable } },
 { CERT_RDN_TELETEX_STRING,   oneW, { sizeof(oneTeletex), oneTeletex } },
 { CERT_RDN_VIDEOTEX_STRING,  oneW, { sizeof(oneVideotex), oneVideotex } },
 { CERT_RDN_IA5_STRING,       oneW, { sizeof(oneIA5), oneIA5 } },
 { CERT_RDN_GRAPHIC_STRING,   oneW, { sizeof(oneGraphic), oneGraphic } },
 { CERT_RDN_VISIBLE_STRING,   oneW, { sizeof(oneVisible), oneVisible } },
 { CERT_RDN_UNIVERSAL_STRING, oneW, { sizeof(oneUniversal), oneUniversal } },
 { CERT_RDN_GENERAL_STRING,   oneW, { sizeof(oneGeneral), oneGeneral } },
 { CERT_RDN_BMP_STRING,       oneW, { sizeof(oneBMP), oneBMP } },
 { CERT_RDN_UTF8_STRING,      oneW, { sizeof(oneUTF8), oneUTF8 } },
 { CERT_RDN_BMP_STRING,     nihongoURL, { sizeof(nihongoBMP), nihongoBMP } },
 { CERT_RDN_UTF8_STRING,    nihongoURL, { sizeof(nihongoUTF8), nihongoUTF8 } },
};

static struct UnicodeExpectedResult unicodeWeirdness[] = {
 { CERT_RDN_TELETEX_STRING, nihongoURL, { sizeof(nihongoT61), nihongoT61 } },
 { CERT_RDN_GENERAL_STRING, nihongoURL, { sizeof(nihongoGeneral), nihongoGeneral } },
};

static void test_encodeUnicodeNameValue(DWORD dwEncoding)
{
    BYTE *buf = NULL;
    DWORD size = 0, i;
    BOOL ret;
    CERT_NAME_VALUE value;

    if (0)
    {
        /* Crashes on win9x */
        ret = pCryptEncodeObjectEx(dwEncoding, X509_UNICODE_NAME_VALUE, NULL,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
        ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
         "Expected STATUS_ACCESS_VIOLATION, got %08x\n", GetLastError());
    }
    /* Have to have a string of some sort */
    value.dwValueType = 0; /* aka CERT_RDN_ANY_TYPE */
    value.Value.pbData = NULL;
    value.Value.cbData = 0;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_UNICODE_NAME_VALUE, &value,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && GetLastError() == CRYPT_E_NOT_CHAR_STRING,
     "Expected CRYPT_E_NOT_CHAR_STRING, got %08x\n", GetLastError());
    value.dwValueType = CERT_RDN_ENCODED_BLOB;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_UNICODE_NAME_VALUE, &value,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && GetLastError() == CRYPT_E_NOT_CHAR_STRING,
     "Expected CRYPT_E_NOT_CHAR_STRING, got %08x\n", GetLastError());
    value.dwValueType = CERT_RDN_ANY_TYPE;
    value.Value.pbData = (LPBYTE)oneW;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_UNICODE_NAME_VALUE, &value,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && GetLastError() == CRYPT_E_NOT_CHAR_STRING,
     "Expected CRYPT_E_NOT_CHAR_STRING, got %08x\n", GetLastError());
    value.Value.cbData = sizeof(oneW);
    ret = pCryptEncodeObjectEx(dwEncoding, X509_UNICODE_NAME_VALUE, &value,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && GetLastError() == CRYPT_E_NOT_CHAR_STRING,
     "Expected CRYPT_E_NOT_CHAR_STRING, got %08x\n", GetLastError());
    /* An encoded string with specified length isn't good enough either */
    value.dwValueType = CERT_RDN_ENCODED_BLOB;
    value.Value.pbData = oneUniversal;
    value.Value.cbData = sizeof(oneUniversal);
    ret = pCryptEncodeObjectEx(dwEncoding, X509_UNICODE_NAME_VALUE, &value,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && GetLastError() == CRYPT_E_NOT_CHAR_STRING,
     "Expected CRYPT_E_NOT_CHAR_STRING, got %08x\n", GetLastError());
    /* More failure checking */
    value.Value.cbData = 0;
    for (i = 0; i < sizeof(unicodeErrors) / sizeof(unicodeErrors[0]); i++)
    {
        value.Value.pbData = (LPBYTE)unicodeErrors[i].str;
        value.dwValueType = unicodeErrors[i].valueType;
        ret = pCryptEncodeObjectEx(dwEncoding, X509_UNICODE_NAME_VALUE, &value,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
        ok(!ret && GetLastError() == unicodeErrors[i].error,
         "Value type %d: expected %08x, got %08x\n", value.dwValueType,
         unicodeErrors[i].error, GetLastError());
        ok(size == unicodeErrors[i].errorIndex,
         "Expected error index %d, got %d\n", unicodeErrors[i].errorIndex,
         size);
    }
    /* cbData can be zero if the string is NULL-terminated */
    value.Value.cbData = 0;
    for (i = 0; i < sizeof(unicodeResults) / sizeof(unicodeResults[0]); i++)
    {
        value.Value.pbData = (LPBYTE)unicodeResults[i].str;
        value.dwValueType = unicodeResults[i].valueType;
        ret = pCryptEncodeObjectEx(dwEncoding, X509_UNICODE_NAME_VALUE, &value,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
        ok(ret || broken(GetLastError() == OSS_PDU_MISMATCH /* Win9x */),
         "CryptEncodeObjectEx failed: %08x\n", GetLastError());
        if (ret)
        {
            ok(size == unicodeResults[i].encoded.cbData,
             "Value type %d: expected size %d, got %d\n",
             value.dwValueType, unicodeResults[i].encoded.cbData, size);
            ok(!memcmp(unicodeResults[i].encoded.pbData, buf, size),
             "Value type %d: unexpected value\n", value.dwValueType);
            LocalFree(buf);
        }
    }
    /* These "encode," but they do so by truncating each unicode character
     * rather than properly encoding it.  Kept separate from the proper results,
     * because the encoded forms won't decode to their original strings.
     */
    for (i = 0; i < sizeof(unicodeWeirdness) / sizeof(unicodeWeirdness[0]); i++)
    {
        value.Value.pbData = (LPBYTE)unicodeWeirdness[i].str;
        value.dwValueType = unicodeWeirdness[i].valueType;
        ret = pCryptEncodeObjectEx(dwEncoding, X509_UNICODE_NAME_VALUE, &value,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
        ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
        if (ret)
        {
            ok(size == unicodeWeirdness[i].encoded.cbData,
             "Value type %d: expected size %d, got %d\n",
             value.dwValueType, unicodeWeirdness[i].encoded.cbData, size);
            ok(!memcmp(unicodeWeirdness[i].encoded.pbData, buf, size),
             "Value type %d: unexpected value\n", value.dwValueType);
            LocalFree(buf);
        }
    }
}

static inline int strncmpW( const WCHAR *str1, const WCHAR *str2, int n )
{
    if (n <= 0) return 0;
    while ((--n > 0) && *str1 && (*str1 == *str2)) { str1++; str2++; }
    return *str1 - *str2;
}

static void test_decodeUnicodeNameValue(DWORD dwEncoding)
{
    DWORD i;

    for (i = 0; i < sizeof(unicodeResults) / sizeof(unicodeResults[0]); i++)
    {
        BYTE *buf = NULL;
        BOOL ret;
        DWORD size = 0;

        ret = pCryptDecodeObjectEx(dwEncoding, X509_UNICODE_NAME_VALUE,
         unicodeResults[i].encoded.pbData, unicodeResults[i].encoded.cbData,
         CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
        ok(ret || broken(GetLastError() == CRYPT_E_NOT_CHAR_STRING /* Win9x */),
         "CryptDecodeObjectEx failed: %08x\n", GetLastError());
        if (ret && buf)
        {
            PCERT_NAME_VALUE value = (PCERT_NAME_VALUE)buf;

            ok(value->dwValueType == unicodeResults[i].valueType,
             "Expected value type %d, got %d\n", unicodeResults[i].valueType,
             value->dwValueType);
            ok(!strncmpW((LPWSTR)value->Value.pbData, unicodeResults[i].str,
             value->Value.cbData / sizeof(WCHAR)),
             "Unexpected decoded value for index %d (value type %d)\n", i,
             unicodeResults[i].valueType);
            LocalFree(buf);
        }
    }
}

struct encodedOctets
{
    const BYTE *val;
    const BYTE *encoded;
};

static const unsigned char bin46[] = { 'h','i',0 };
static const unsigned char bin47[] = { 0x04,0x02,'h','i',0 };
static const unsigned char bin48[] = {
     's','o','m','e','l','o','n','g',0xff,'s','t','r','i','n','g',0 };
static const unsigned char bin49[] = {
     0x04,0x0f,'s','o','m','e','l','o','n','g',0xff,'s','t','r','i','n','g',0 };
static const unsigned char bin50[] = { 0 };
static const unsigned char bin51[] = { 0x04,0x00,0 };

static const struct encodedOctets octets[] = {
    { bin46, bin47 },
    { bin48, bin49 },
    { bin50, bin51 },
};

static void test_encodeOctets(DWORD dwEncoding)
{
    CRYPT_DATA_BLOB blob;
    DWORD i;

    for (i = 0; i < sizeof(octets) / sizeof(octets[0]); i++)
    {
        BYTE *buf = NULL;
        BOOL ret;
        DWORD bufSize = 0;

        blob.cbData = strlen((const char*)octets[i].val);
        blob.pbData = (BYTE*)octets[i].val;
        ret = pCryptEncodeObjectEx(dwEncoding, X509_OCTET_STRING, &blob,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &bufSize);
        ok(ret, "CryptEncodeObjectEx failed: %d\n", GetLastError());
        if (ret)
        {
            ok(buf[0] == 4,
             "Got unexpected type %d for octet string (expected 4)\n", buf[0]);
            ok(buf[1] == octets[i].encoded[1], "Got length %d, expected %d\n",
             buf[1], octets[i].encoded[1]);
            ok(!memcmp(buf + 1, octets[i].encoded + 1,
             octets[i].encoded[1] + 1), "Got unexpected value\n");
            LocalFree(buf);
        }
    }
}

static void test_decodeOctets(DWORD dwEncoding)
{
    DWORD i;

    for (i = 0; i < sizeof(octets) / sizeof(octets[0]); i++)
    {
        BYTE *buf = NULL;
        BOOL ret;
        DWORD bufSize = 0;

        ret = pCryptDecodeObjectEx(dwEncoding, X509_OCTET_STRING,
         octets[i].encoded, octets[i].encoded[1] + 2,
         CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
        ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
        ok(bufSize >= sizeof(CRYPT_DATA_BLOB) + octets[i].encoded[1],
         "Expected size >= %d, got %d\n",
           (int)sizeof(CRYPT_DATA_BLOB) + octets[i].encoded[1], bufSize);
        ok(buf != NULL, "Expected allocated buffer\n");
        if (ret)
        {
            CRYPT_DATA_BLOB *blob = (CRYPT_DATA_BLOB *)buf;

            if (blob->cbData)
                ok(!memcmp(blob->pbData, octets[i].val, blob->cbData),
                 "Unexpected value\n");
            LocalFree(buf);
        }
    }
}

static const BYTE bytesToEncode[] = { 0xff, 0xff };

struct encodedBits
{
    DWORD cUnusedBits;
    const BYTE *encoded;
    DWORD cbDecoded;
    const BYTE *decoded;
};

static const unsigned char bin52[] = { 0x03,0x03,0x00,0xff,0xff };
static const unsigned char bin53[] = { 0xff,0xff };
static const unsigned char bin54[] = { 0x03,0x03,0x01,0xff,0xfe };
static const unsigned char bin55[] = { 0xff,0xfe };
static const unsigned char bin56[] = { 0x03,0x02,0x01,0xfe };
static const unsigned char bin57[] = { 0xfe };

static const struct encodedBits bits[] = {
    /* normal test cases */
    { 0, bin52, 2, bin53 },
    { 1, bin54, 2, bin55 },
    /* strange test case, showing cUnusedBits >= 8 is allowed */
    { 9, bin56, 1, bin57 },
};

static void test_encodeBits(DWORD dwEncoding)
{
    DWORD i;

    for (i = 0; i < sizeof(bits) / sizeof(bits[0]); i++)
    {
        CRYPT_BIT_BLOB blob;
        BOOL ret;
        BYTE *buf = NULL;
        DWORD bufSize = 0;

        blob.cbData = sizeof(bytesToEncode);
        blob.pbData = (BYTE *)bytesToEncode;
        blob.cUnusedBits = bits[i].cUnusedBits;
        ret = pCryptEncodeObjectEx(dwEncoding, X509_BITS, &blob,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &bufSize);
        ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
        if (ret)
        {
            ok(bufSize == bits[i].encoded[1] + 2,
             "%d: Got unexpected size %d, expected %d\n", i, bufSize,
             bits[i].encoded[1] + 2);
            ok(!memcmp(buf, bits[i].encoded, bits[i].encoded[1] + 2),
             "%d: Unexpected value\n", i);
            LocalFree(buf);
        }
    }
}

static void test_decodeBits(DWORD dwEncoding)
{
    static const BYTE ber[] = "\x03\x02\x01\xff";
    static const BYTE berDecoded = 0xfe;
    DWORD i;
    BOOL ret;
    BYTE *buf = NULL;
    DWORD bufSize = 0;

    /* normal cases */
    for (i = 0; i < sizeof(bits) / sizeof(bits[0]); i++)
    {
        ret = pCryptDecodeObjectEx(dwEncoding, X509_BITS, bits[i].encoded,
         bits[i].encoded[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL, &buf,
         &bufSize);
        ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
        if (ret)
        {
            CRYPT_BIT_BLOB *blob;

            ok(bufSize >= sizeof(CRYPT_BIT_BLOB) + bits[i].cbDecoded,
               "Got unexpected size %d\n", bufSize);
            blob = (CRYPT_BIT_BLOB *)buf;
            ok(blob->cbData == bits[i].cbDecoded,
             "Got unexpected length %d, expected %d\n", blob->cbData,
             bits[i].cbDecoded);
            if (blob->cbData && bits[i].cbDecoded)
                ok(!memcmp(blob->pbData, bits[i].decoded, bits[i].cbDecoded),
                 "Unexpected value\n");
            LocalFree(buf);
        }
    }
    /* special case: check that something that's valid in BER but not in DER
     * decodes successfully
     */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_BITS, ber, ber[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CRYPT_BIT_BLOB *blob;

        ok(bufSize >= sizeof(CRYPT_BIT_BLOB) + sizeof(berDecoded),
           "Got unexpected size %d\n", bufSize);
        blob = (CRYPT_BIT_BLOB *)buf;
        ok(blob->cbData == sizeof(berDecoded),
           "Got unexpected length %d\n", blob->cbData);
        if (blob->cbData)
            ok(*blob->pbData == berDecoded, "Unexpected value\n");
        LocalFree(buf);
    }
}

struct Constraints2
{
    CERT_BASIC_CONSTRAINTS2_INFO info;
    const BYTE *encoded;
};

static const unsigned char bin59[] = { 0x30,0x00 };
static const unsigned char bin60[] = { 0x30,0x03,0x01,0x01,0xff };
static const unsigned char bin61[] = { 0x30,0x03,0x02,0x01,0x00 };
static const unsigned char bin62[] = { 0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01 };
static const struct Constraints2 constraints2[] = {
 /* empty constraints */
 { { FALSE, FALSE, 0}, bin59 },
 /* can be a CA */
 { { TRUE,  FALSE, 0}, bin60 },
 /* has path length constraints set (MSDN implies fCA needs to be TRUE as well,
  * but that's not the case
  */
 { { FALSE, TRUE,  0}, bin61 },
 /* can be a CA and has path length constraints set */
 { { TRUE,  TRUE,  1}, bin62 },
};

static const BYTE emptyConstraint[] = { 0x30, 0x03, 0x03, 0x01, 0x00 };
static const BYTE encodedDomainName[] = { 0x30, 0x2b, 0x31, 0x29, 0x30, 0x11,
 0x06, 0x0a, 0x09, 0x92, 0x26, 0x89, 0x93, 0xf2, 0x2c, 0x64, 0x01, 0x19, 0x16,
 0x03, 0x6f, 0x72, 0x67, 0x30, 0x14, 0x06, 0x0a, 0x09, 0x92, 0x26, 0x89, 0x93,
 0xf2, 0x2c, 0x64, 0x01, 0x19, 0x16, 0x06, 0x77, 0x69, 0x6e, 0x65, 0x68, 0x71 };
static const BYTE constraintWithDomainName[] = { 0x30, 0x32, 0x03, 0x01, 0x00,
 0x30, 0x2d, 0x30, 0x2b, 0x31, 0x29, 0x30, 0x11, 0x06, 0x0a, 0x09, 0x92, 0x26,
 0x89, 0x93, 0xf2, 0x2c, 0x64, 0x01, 0x19, 0x16, 0x03, 0x6f, 0x72, 0x67, 0x30,
 0x14, 0x06, 0x0a, 0x09, 0x92, 0x26, 0x89, 0x93, 0xf2, 0x2c, 0x64, 0x01, 0x19,
 0x16, 0x06, 0x77, 0x69, 0x6e, 0x65, 0x68, 0x71 };

static void test_encodeBasicConstraints(DWORD dwEncoding)
{
    DWORD i, bufSize = 0;
    CERT_BASIC_CONSTRAINTS_INFO info = { { 0 } };
    CERT_NAME_BLOB nameBlob = { sizeof(encodedDomainName),
     (LPBYTE)encodedDomainName };
    BOOL ret;
    BYTE *buf = NULL;

    /* First test with the simpler info2 */
    for (i = 0; i < sizeof(constraints2) / sizeof(constraints2[0]); i++)
    {
        ret = pCryptEncodeObjectEx(dwEncoding, X509_BASIC_CONSTRAINTS2,
         &constraints2[i].info, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf,
         &bufSize);
        ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
        if (ret)
        {
            ok(bufSize == constraints2[i].encoded[1] + 2,
             "Expected %d bytes, got %d\n", constraints2[i].encoded[1] + 2,
             bufSize);
            ok(!memcmp(buf, constraints2[i].encoded,
             constraints2[i].encoded[1] + 2), "Unexpected value\n");
            LocalFree(buf);
        }
    }
    /* Now test with more complex basic constraints */
    info.SubjectType.cbData = 0;
    info.fPathLenConstraint = FALSE;
    info.cSubtreesConstraint = 0;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_BASIC_CONSTRAINTS, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(ret || GetLastError() == OSS_BAD_PTR /* Win9x */,
     "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(bufSize == sizeof(emptyConstraint), "Wrong size %d\n", bufSize);
        ok(!memcmp(buf, emptyConstraint, sizeof(emptyConstraint)),
         "Unexpected value\n");
        LocalFree(buf);
    }
    /* None of the certs I examined had any subtree constraint, but I test one
     * anyway just in case.
     */
    info.cSubtreesConstraint = 1;
    info.rgSubtreesConstraint = &nameBlob;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_BASIC_CONSTRAINTS, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(ret || GetLastError() == OSS_BAD_PTR /* Win9x */,
     "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(bufSize == sizeof(constraintWithDomainName), "Wrong size %d\n", bufSize);
        ok(!memcmp(buf, constraintWithDomainName,
         sizeof(constraintWithDomainName)), "Unexpected value\n");
        LocalFree(buf);
    }
    /* FIXME: test encoding with subject type. */
}

static const unsigned char bin63[] = { 0x30,0x06,0x01,0x01,0x01,0x02,0x01,0x01 };

static void test_decodeBasicConstraints(DWORD dwEncoding)
{
    static const BYTE inverted[] = { 0x30, 0x06, 0x02, 0x01, 0x01, 0x01, 0x01,
     0xff };
    static const struct Constraints2 badBool = { { TRUE, TRUE, 1 }, bin63 };
    DWORD i;
    BOOL ret;
    BYTE *buf = NULL;
    DWORD bufSize = 0;

    /* First test with simpler info2 */
    for (i = 0; i < sizeof(constraints2) / sizeof(constraints2[0]); i++)
    {
        ret = pCryptDecodeObjectEx(dwEncoding, X509_BASIC_CONSTRAINTS2,
         constraints2[i].encoded, constraints2[i].encoded[1] + 2,
         CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
        ok(ret, "CryptDecodeObjectEx failed for item %d: %08x\n", i,
         GetLastError());
        if (ret)
        {
            CERT_BASIC_CONSTRAINTS2_INFO *info =
             (CERT_BASIC_CONSTRAINTS2_INFO *)buf;

            ok(!memcmp(info, &constraints2[i].info, sizeof(*info)),
             "Unexpected value for item %d\n", i);
            LocalFree(buf);
        }
    }
    /* Check with the order of encoded elements inverted */
    buf = (PBYTE)1;
    ret = pCryptDecodeObjectEx(dwEncoding, X509_BASIC_CONSTRAINTS2,
     inverted, inverted[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL, &buf,
     &bufSize);
    ok(!ret && (GetLastError() == CRYPT_E_ASN1_CORRUPT ||
     GetLastError() == OSS_DATA_ERROR /* Win9x */),
     "Expected CRYPT_E_ASN1_CORRUPT or OSS_DATA_ERROR, got %08x\n",
     GetLastError());
    ok(!buf, "Expected buf to be set to NULL\n");
    /* Check with a non-DER bool */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_BASIC_CONSTRAINTS2,
     badBool.encoded, badBool.encoded[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL,
     &buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CERT_BASIC_CONSTRAINTS2_INFO *info =
         (CERT_BASIC_CONSTRAINTS2_INFO *)buf;

        ok(!memcmp(info, &badBool.info, sizeof(*info)), "Unexpected value\n");
        LocalFree(buf);
    }
    /* Check with a non-basic constraints value */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_BASIC_CONSTRAINTS2,
     encodedCommonName, encodedCommonName[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(!ret && (GetLastError() == CRYPT_E_ASN1_CORRUPT ||
     GetLastError() == OSS_DATA_ERROR /* Win9x */),
     "Expected CRYPT_E_ASN1_CORRUPT or OSS_DATA_ERROR, got %08x\n",
     GetLastError());
    /* Now check with the more complex CERT_BASIC_CONSTRAINTS_INFO */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_BASIC_CONSTRAINTS,
     emptyConstraint, sizeof(emptyConstraint), CRYPT_DECODE_ALLOC_FLAG, NULL,
     &buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CERT_BASIC_CONSTRAINTS_INFO *info = (CERT_BASIC_CONSTRAINTS_INFO *)buf;

        ok(info->SubjectType.cbData == 0, "Expected no subject type\n");
        ok(!info->fPathLenConstraint, "Expected no path length constraint\n");
        ok(info->cSubtreesConstraint == 0, "Expected no subtree constraints\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_BASIC_CONSTRAINTS,
     constraintWithDomainName, sizeof(constraintWithDomainName),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CERT_BASIC_CONSTRAINTS_INFO *info = (CERT_BASIC_CONSTRAINTS_INFO *)buf;

        ok(info->SubjectType.cbData == 0, "Expected no subject type\n");
        ok(!info->fPathLenConstraint, "Expected no path length constraint\n");
        ok(info->cSubtreesConstraint == 1, "Expected a subtree constraint\n");
        if (info->cSubtreesConstraint && info->rgSubtreesConstraint)
        {
            ok(info->rgSubtreesConstraint[0].cbData ==
             sizeof(encodedDomainName), "Wrong size %d\n",
             info->rgSubtreesConstraint[0].cbData);
            ok(!memcmp(info->rgSubtreesConstraint[0].pbData, encodedDomainName,
             sizeof(encodedDomainName)), "Unexpected value\n");
        }
        LocalFree(buf);
    }
}

/* These are terrible public keys of course, I'm just testing encoding */
static const BYTE modulus1[] = { 0,0,0,1,1,1,1,1 };
static const BYTE modulus2[] = { 1,1,1,1,1,0,0,0 };
static const BYTE modulus3[] = { 0x80,1,1,1,1,0,0,0 };
static const BYTE modulus4[] = { 1,1,1,1,1,0,0,0x80 };
static const BYTE mod1_encoded[] = { 0x30,0x0f,0x02,0x08,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x02,0x03,0x01,0x00,0x01 };
static const BYTE mod2_encoded[] = { 0x30,0x0c,0x02,0x05,0x01,0x01,0x01,0x01,0x01,0x02,0x03,0x01,0x00,0x01 };
static const BYTE mod3_encoded[] = { 0x30,0x0c,0x02,0x05,0x01,0x01,0x01,0x01,0x80,0x02,0x03,0x01,0x00,0x01 };
static const BYTE mod4_encoded[] = { 0x30,0x10,0x02,0x09,0x00,0x80,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x02,0x03,0x01,0x00,0x01 };

struct EncodedRSAPubKey
{
    const BYTE *modulus;
    size_t modulusLen;
    const BYTE *encoded;
    size_t decodedModulusLen;
};

static const struct EncodedRSAPubKey rsaPubKeys[] = {
    { modulus1, sizeof(modulus1), mod1_encoded, sizeof(modulus1) },
    { modulus2, sizeof(modulus2), mod2_encoded, 5 },
    { modulus3, sizeof(modulus3), mod3_encoded, 5 },
    { modulus4, sizeof(modulus4), mod4_encoded, 8 },
};

static void test_encodeRsaPublicKey(DWORD dwEncoding)
{
    BYTE toEncode[sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) + sizeof(modulus1)];
    BLOBHEADER *hdr = (BLOBHEADER *)toEncode;
    RSAPUBKEY *rsaPubKey = (RSAPUBKEY *)(toEncode + sizeof(BLOBHEADER));
    BOOL ret;
    BYTE *buf = NULL;
    DWORD bufSize = 0, i;

    /* Try with a bogus blob type */
    hdr->bType = 2;
    hdr->bVersion = CUR_BLOB_VERSION;
    hdr->reserved = 0;
    hdr->aiKeyAlg = CALG_RSA_KEYX;
    rsaPubKey->magic = 0x31415352;
    rsaPubKey->bitlen = sizeof(modulus1) * 8;
    rsaPubKey->pubexp = 65537;
    memcpy(toEncode + sizeof(BLOBHEADER) + sizeof(RSAPUBKEY), modulus1,
     sizeof(modulus1));

    ret = pCryptEncodeObjectEx(dwEncoding, RSA_CSP_PUBLICKEYBLOB,
     toEncode, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* Now with a bogus reserved field */
    hdr->bType = PUBLICKEYBLOB;
    hdr->reserved = 1;
    ret = pCryptEncodeObjectEx(dwEncoding, RSA_CSP_PUBLICKEYBLOB,
     toEncode, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    if (ret)
    {
        ok(bufSize == rsaPubKeys[0].encoded[1] + 2,
         "Expected size %d, got %d\n", rsaPubKeys[0].encoded[1] + 2, bufSize);
        ok(!memcmp(buf, rsaPubKeys[0].encoded, bufSize), "Unexpected value\n");
        LocalFree(buf);
    }
    /* Now with a bogus blob version */
    hdr->reserved = 0;
    hdr->bVersion = 0;
    ret = pCryptEncodeObjectEx(dwEncoding, RSA_CSP_PUBLICKEYBLOB,
     toEncode, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    if (ret)
    {
        ok(bufSize == rsaPubKeys[0].encoded[1] + 2,
         "Expected size %d, got %d\n", rsaPubKeys[0].encoded[1] + 2, bufSize);
        ok(!memcmp(buf, rsaPubKeys[0].encoded, bufSize), "Unexpected value\n");
        LocalFree(buf);
    }
    /* And with a bogus alg ID */
    hdr->bVersion = CUR_BLOB_VERSION;
    hdr->aiKeyAlg = CALG_DES;
    ret = pCryptEncodeObjectEx(dwEncoding, RSA_CSP_PUBLICKEYBLOB,
     toEncode, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    if (ret)
    {
        ok(bufSize == rsaPubKeys[0].encoded[1] + 2,
         "Expected size %d, got %d\n", rsaPubKeys[0].encoded[1] + 2, bufSize);
        ok(!memcmp(buf, rsaPubKeys[0].encoded, bufSize), "Unexpected value\n");
        LocalFree(buf);
    }
    /* Check a couple of RSA-related OIDs */
    hdr->aiKeyAlg = CALG_RSA_KEYX;
    ret = pCryptEncodeObjectEx(dwEncoding, szOID_RSA_RSA,
     toEncode, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
    ret = pCryptEncodeObjectEx(dwEncoding, szOID_RSA_SHA1RSA,
     toEncode, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
    /* Finally, all valid */
    hdr->aiKeyAlg = CALG_RSA_KEYX;
    for (i = 0; i < sizeof(rsaPubKeys) / sizeof(rsaPubKeys[0]); i++)
    {
        memcpy(toEncode + sizeof(BLOBHEADER) + sizeof(RSAPUBKEY),
         rsaPubKeys[i].modulus, rsaPubKeys[i].modulusLen);
        ret = pCryptEncodeObjectEx(dwEncoding, RSA_CSP_PUBLICKEYBLOB,
         toEncode, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &bufSize);
        ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
        if (ret)
        {
            ok(bufSize == rsaPubKeys[i].encoded[1] + 2,
             "Expected size %d, got %d\n", rsaPubKeys[i].encoded[1] + 2,
             bufSize);
            ok(!memcmp(buf, rsaPubKeys[i].encoded, bufSize),
             "Unexpected value\n");
            LocalFree(buf);
        }
    }
}

static void test_decodeRsaPublicKey(DWORD dwEncoding)
{
    DWORD i;
    LPBYTE buf = NULL;
    DWORD bufSize = 0;
    BOOL ret;

    /* Try with a bad length */
    ret = pCryptDecodeObjectEx(dwEncoding, RSA_CSP_PUBLICKEYBLOB,
     rsaPubKeys[0].encoded, rsaPubKeys[0].encoded[1],
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(!ret && (GetLastError() == CRYPT_E_ASN1_EOD ||
     GetLastError() == OSS_MORE_INPUT /* Win9x/NT4 */),
     "Expected CRYPT_E_ASN1_EOD or OSS_MORE_INPUT, got %08x\n",
     GetLastError());
    /* Try with a couple of RSA-related OIDs */
    ret = pCryptDecodeObjectEx(dwEncoding, szOID_RSA_RSA,
     rsaPubKeys[0].encoded, rsaPubKeys[0].encoded[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
    ret = pCryptDecodeObjectEx(dwEncoding, szOID_RSA_SHA1RSA,
     rsaPubKeys[0].encoded, rsaPubKeys[0].encoded[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
    /* Now try success cases */
    for (i = 0; i < sizeof(rsaPubKeys) / sizeof(rsaPubKeys[0]); i++)
    {
        bufSize = 0;
        ret = pCryptDecodeObjectEx(dwEncoding, RSA_CSP_PUBLICKEYBLOB,
         rsaPubKeys[i].encoded, rsaPubKeys[i].encoded[1] + 2,
         CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
        ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
        if (ret)
        {
            BLOBHEADER *hdr = (BLOBHEADER *)buf;
            RSAPUBKEY *rsaPubKey = (RSAPUBKEY *)(buf + sizeof(BLOBHEADER));

            ok(bufSize >= sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) +
             rsaPubKeys[i].decodedModulusLen,
             "Wrong size %d\n", bufSize);
            ok(hdr->bType == PUBLICKEYBLOB,
             "Expected type PUBLICKEYBLOB (%d), got %d\n", PUBLICKEYBLOB,
             hdr->bType);
            ok(hdr->bVersion == CUR_BLOB_VERSION,
             "Expected version CUR_BLOB_VERSION (%d), got %d\n",
             CUR_BLOB_VERSION, hdr->bVersion);
            ok(hdr->reserved == 0, "Expected reserved 0, got %d\n",
             hdr->reserved);
            ok(hdr->aiKeyAlg == CALG_RSA_KEYX,
             "Expected CALG_RSA_KEYX, got %08x\n", hdr->aiKeyAlg);
            ok(rsaPubKey->magic == 0x31415352,
             "Expected magic RSA1, got %08x\n", rsaPubKey->magic);
            ok(rsaPubKey->bitlen == rsaPubKeys[i].decodedModulusLen * 8,
             "Wrong bit len %d\n", rsaPubKey->bitlen);
            ok(rsaPubKey->pubexp == 65537, "Expected pubexp 65537, got %d\n",
             rsaPubKey->pubexp);
            ok(!memcmp(buf + sizeof(BLOBHEADER) + sizeof(RSAPUBKEY),
             rsaPubKeys[i].modulus, rsaPubKeys[i].decodedModulusLen),
             "Unexpected modulus\n");
            LocalFree(buf);
        }
    }
}

static const BYTE intSequence[] = { 0x30, 0x1b, 0x02, 0x01, 0x01, 0x02, 0x01,
 0x7f, 0x02, 0x02, 0x00, 0x80, 0x02, 0x02, 0x01, 0x00, 0x02, 0x01, 0x80, 0x02,
 0x02, 0xff, 0x7f, 0x02, 0x04, 0xba, 0xdd, 0xf0, 0x0d };

static const BYTE mixedSequence[] = { 0x30, 0x27, 0x17, 0x0d, 0x30, 0x35, 0x30,
 0x36, 0x30, 0x36, 0x31, 0x36, 0x31, 0x30, 0x30, 0x30, 0x5a, 0x02, 0x01, 0x7f,
 0x02, 0x02, 0x00, 0x80, 0x02, 0x02, 0x01, 0x00, 0x02, 0x01, 0x80, 0x02, 0x02,
 0xff, 0x7f, 0x02, 0x04, 0xba, 0xdd, 0xf0, 0x0d };

static void test_encodeSequenceOfAny(DWORD dwEncoding)
{
    CRYPT_DER_BLOB blobs[sizeof(ints) / sizeof(ints[0])];
    CRYPT_SEQUENCE_OF_ANY seq;
    DWORD i;
    BOOL ret;
    BYTE *buf = NULL;
    DWORD bufSize = 0;

    /* Encode a homogeneous sequence */
    for (i = 0; i < sizeof(ints) / sizeof(ints[0]); i++)
    {
        blobs[i].cbData = ints[i].encoded[1] + 2;
        blobs[i].pbData = (BYTE *)ints[i].encoded;
    }
    seq.cValue = sizeof(ints) / sizeof(ints[0]);
    seq.rgValue = blobs;

    ret = pCryptEncodeObjectEx(dwEncoding, X509_SEQUENCE_OF_ANY, &seq,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(bufSize == sizeof(intSequence), "Wrong size %d\n", bufSize);
        ok(!memcmp(buf, intSequence, intSequence[1] + 2), "Unexpected value\n");
        LocalFree(buf);
    }
    /* Change the type of the first element in the sequence, and give it
     * another go
     */
    blobs[0].cbData = times[0].encodedTime[1] + 2;
    blobs[0].pbData = (BYTE *)times[0].encodedTime;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_SEQUENCE_OF_ANY, &seq,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(bufSize == sizeof(mixedSequence), "Wrong size %d\n", bufSize);
        ok(!memcmp(buf, mixedSequence, mixedSequence[1] + 2),
         "Unexpected value\n");
        LocalFree(buf);
    }
}

static void test_decodeSequenceOfAny(DWORD dwEncoding)
{
    BOOL ret;
    BYTE *buf = NULL;
    DWORD bufSize = 0;

    ret = pCryptDecodeObjectEx(dwEncoding, X509_SEQUENCE_OF_ANY, intSequence,
     intSequence[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CRYPT_SEQUENCE_OF_ANY *seq = (CRYPT_SEQUENCE_OF_ANY *)buf;
        DWORD i;

        ok(seq->cValue == sizeof(ints) / sizeof(ints[0]),
         "Wrong elements %d\n", seq->cValue);
        for (i = 0; i < min(seq->cValue, sizeof(ints) / sizeof(ints[0])); i++)
        {
            ok(seq->rgValue[i].cbData == ints[i].encoded[1] + 2,
             "Expected %d bytes, got %d\n", ints[i].encoded[1] + 2,
             seq->rgValue[i].cbData);
            ok(!memcmp(seq->rgValue[i].pbData, ints[i].encoded,
             ints[i].encoded[1] + 2), "Unexpected value\n");
        }
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_SEQUENCE_OF_ANY, mixedSequence,
     mixedSequence[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL, &buf,
     &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CRYPT_SEQUENCE_OF_ANY *seq = (CRYPT_SEQUENCE_OF_ANY *)buf;

        ok(seq->cValue == sizeof(ints) / sizeof(ints[0]),
         "Wrong elements %d\n", seq->cValue);
        /* Just check the first element since it's all that changed */
        ok(seq->rgValue[0].cbData == times[0].encodedTime[1] + 2,
         "Expected %d bytes, got %d\n", times[0].encodedTime[1] + 2,
         seq->rgValue[0].cbData);
        ok(!memcmp(seq->rgValue[0].pbData, times[0].encodedTime,
         times[0].encodedTime[1] + 2), "Unexpected value\n");
        LocalFree(buf);
    }
}

struct encodedExtensions
{
    CERT_EXTENSIONS exts;
    const BYTE *encoded;
};

static BYTE crit_ext_data[] = { 0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01 };
static BYTE noncrit_ext_data[] = { 0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01 };
static CHAR oid_basic_constraints2[] = szOID_BASIC_CONSTRAINTS2;
static CERT_EXTENSION criticalExt =
 { oid_basic_constraints2, TRUE, { 8, crit_ext_data } };
static CERT_EXTENSION nonCriticalExt =
 { oid_basic_constraints2, FALSE, { 8, noncrit_ext_data } };
static CHAR oid_short[] = "1.1";
static CERT_EXTENSION extWithShortOid =
 { oid_short, FALSE, { 0, NULL } };

static const BYTE ext0[] = { 0x30,0x00 };
static const BYTE ext1[] = { 0x30,0x14,0x30,0x12,0x06,0x03,0x55,0x1d,0x13,0x01,0x01,
                             0xff,0x04,0x08,0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01 };
static const BYTE ext2[] = { 0x30,0x11,0x30,0x0f,0x06,0x03,0x55,0x1d,0x13,0x04,
                             0x08,0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01 };
static const BYTE ext3[] = { 0x30,0x07,0x30,0x05,0x06,0x01,0x29,0x04,0x00 };

static const struct encodedExtensions exts[] = {
 { { 0, NULL }, ext0 },
 { { 1, &criticalExt }, ext1 },
 { { 1, &nonCriticalExt }, ext2 },
 { { 1, &extWithShortOid }, ext3 }
};

static void test_encodeExtensions(DWORD dwEncoding)
{
    DWORD i;

    for (i = 0; i < sizeof(exts) / sizeof(exts[i]); i++)
    {
        BOOL ret;
        BYTE *buf = NULL;
        DWORD bufSize = 0;

        ret = pCryptEncodeObjectEx(dwEncoding, X509_EXTENSIONS, &exts[i].exts,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &bufSize);
        ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
        if (ret)
        {
            ok(bufSize == exts[i].encoded[1] + 2,
             "Expected %d bytes, got %d\n", exts[i].encoded[1] + 2, bufSize);
            ok(!memcmp(buf, exts[i].encoded, exts[i].encoded[1] + 2),
             "Unexpected value\n");
            LocalFree(buf);
        }
    }
}

static void test_decodeExtensions(DWORD dwEncoding)
{
    DWORD i;

    for (i = 0; i < sizeof(exts) / sizeof(exts[i]); i++)
    {
        BOOL ret;
        BYTE *buf = NULL;
        DWORD bufSize = 0;

        ret = pCryptDecodeObjectEx(dwEncoding, X509_EXTENSIONS,
         exts[i].encoded, exts[i].encoded[1] + 2, CRYPT_DECODE_ALLOC_FLAG,
         NULL, &buf, &bufSize);
        ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
        if (ret)
        {
            CERT_EXTENSIONS *ext = (CERT_EXTENSIONS *)buf;
            DWORD j;

            ok(ext->cExtension == exts[i].exts.cExtension,
             "Expected %d extensions, see %d\n", exts[i].exts.cExtension,
             ext->cExtension);
            for (j = 0; j < min(ext->cExtension, exts[i].exts.cExtension); j++)
            {
                ok(!strcmp(ext->rgExtension[j].pszObjId,
                 exts[i].exts.rgExtension[j].pszObjId),
                 "Expected OID %s, got %s\n",
                 exts[i].exts.rgExtension[j].pszObjId,
                 ext->rgExtension[j].pszObjId);
                ok(!memcmp(ext->rgExtension[j].Value.pbData,
                 exts[i].exts.rgExtension[j].Value.pbData,
                 exts[i].exts.rgExtension[j].Value.cbData),
                 "Unexpected value\n");
            }
            LocalFree(buf);
        }
        ret = pCryptDecodeObjectEx(dwEncoding, X509_EXTENSIONS,
         exts[i].encoded, exts[i].encoded[1] + 2, 0, NULL, NULL, &bufSize);
        ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
        buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bufSize);
        if (buf)
        {
            ret = pCryptDecodeObjectEx(dwEncoding, X509_EXTENSIONS,
             exts[i].encoded, exts[i].encoded[1] + 2, 0, NULL, buf, &bufSize);
            ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
            HeapFree(GetProcessHeap(), 0, buf);
        }
    }
}

/* MS encodes public key info with a NULL if the algorithm identifier's
 * parameters are empty.  However, when encoding an algorithm in a CERT_INFO,
 * it encodes them by omitting the algorithm parameters.  It accepts either
 * form for decoding.
 */
struct encodedPublicKey
{
    CERT_PUBLIC_KEY_INFO info;
    const BYTE *encoded;
    const BYTE *encodedNoNull;
    CERT_PUBLIC_KEY_INFO decoded;
};

static const BYTE aKey[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xa, 0xb, 0xc, 0xd,
 0xe, 0xf };
static const BYTE params[] = { 0x02, 0x01, 0x01 };

static const unsigned char bin64[] = {
    0x30,0x0b,0x30,0x06,0x06,0x02,0x2a,0x03,0x05,0x00,0x03,0x01,0x00};
static const unsigned char bin65[] = {
    0x30,0x09,0x30,0x04,0x06,0x02,0x2a,0x03,0x03,0x01,0x00};
static const unsigned char bin66[] = {
    0x30,0x0f,0x30,0x0a,0x06,0x06,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x05,0x00,0x03,0x01,0x00};
static const unsigned char bin67[] = {
    0x30,0x0d,0x30,0x08,0x06,0x06,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x03,0x01,0x00};
static const unsigned char bin68[] = {
    0x30,0x1f,0x30,0x0a,0x06,0x06,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x05,0x00,0x03,0x11,0x00,0x00,0x01,
    0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
static const unsigned char bin69[] = {
    0x30,0x1d,0x30,0x08,0x06,0x06,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x03,0x11,0x00,0x00,0x01,
    0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
static const unsigned char bin70[] = {
    0x30,0x20,0x30,0x0b,0x06,0x06,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x02,0x01,0x01,
    0x03,0x11,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,
    0x0f};
static const unsigned char bin71[] = {
    0x30,0x20,0x30,0x0b,0x06,0x06,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x02,0x01,0x01,
    0x03,0x11,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,
    0x0f};
static unsigned char bin72[] = { 0x05,0x00};

static CHAR oid_bogus[] = "1.2.3",
            oid_rsa[]   = szOID_RSA;

static const struct encodedPublicKey pubKeys[] = {
 /* with a bogus OID */
 { { { oid_bogus, { 0, NULL } }, { 0, NULL, 0 } },
  bin64, bin65,
  { { oid_bogus, { 2, bin72 } }, { 0, NULL, 0 } } },
 /* some normal keys */
 { { { oid_rsa, { 0, NULL } }, { 0, NULL, 0} },
  bin66, bin67,
  { { oid_rsa, { 2, bin72 } }, { 0, NULL, 0 } } },
 { { { oid_rsa, { 0, NULL } }, { sizeof(aKey), (BYTE *)aKey, 0} },
  bin68, bin69,
  { { oid_rsa, { 2, bin72 } }, { sizeof(aKey), (BYTE *)aKey, 0} } },
 /* with add'l parameters--note they must be DER-encoded */
 { { { oid_rsa, { sizeof(params), (BYTE *)params } }, { sizeof(aKey),
  (BYTE *)aKey, 0 } },
  bin70, bin71,
  { { oid_rsa, { sizeof(params), (BYTE *)params } }, { sizeof(aKey),
  (BYTE *)aKey, 0 } } },
};

static void test_encodePublicKeyInfo(DWORD dwEncoding)
{
    DWORD i;

    for (i = 0; i < sizeof(pubKeys) / sizeof(pubKeys[0]); i++)
    {
        BOOL ret;
        BYTE *buf = NULL;
        DWORD bufSize = 0;

        ret = pCryptEncodeObjectEx(dwEncoding, X509_PUBLIC_KEY_INFO,
         &pubKeys[i].info, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf,
         &bufSize);
        ok(ret || GetLastError() == OSS_BAD_PTR /* Win9x */,
         "CryptEncodeObjectEx failed: %08x\n", GetLastError());
        if (ret)
        {
            ok(bufSize == pubKeys[i].encoded[1] + 2,
             "Expected %d bytes, got %d\n", pubKeys[i].encoded[1] + 2, bufSize);
            if (bufSize == pubKeys[i].encoded[1] + 2)
                ok(!memcmp(buf, pubKeys[i].encoded, pubKeys[i].encoded[1] + 2),
                 "Unexpected value\n");
            LocalFree(buf);
        }
    }
}

static void comparePublicKeyInfo(const CERT_PUBLIC_KEY_INFO *expected,
 const CERT_PUBLIC_KEY_INFO *got)
{
    ok(!strcmp(expected->Algorithm.pszObjId, got->Algorithm.pszObjId),
     "Expected OID %s, got %s\n", expected->Algorithm.pszObjId,
     got->Algorithm.pszObjId);
    ok(expected->Algorithm.Parameters.cbData ==
     got->Algorithm.Parameters.cbData,
     "Expected parameters of %d bytes, got %d\n",
     expected->Algorithm.Parameters.cbData, got->Algorithm.Parameters.cbData);
    if (expected->Algorithm.Parameters.cbData)
        ok(!memcmp(expected->Algorithm.Parameters.pbData,
         got->Algorithm.Parameters.pbData, got->Algorithm.Parameters.cbData),
         "Unexpected algorithm parameters\n");
    ok(expected->PublicKey.cbData == got->PublicKey.cbData,
     "Expected public key of %d bytes, got %d\n",
     expected->PublicKey.cbData, got->PublicKey.cbData);
    if (expected->PublicKey.cbData)
        ok(!memcmp(expected->PublicKey.pbData, got->PublicKey.pbData,
         got->PublicKey.cbData), "Unexpected public key value\n");
}

static void test_decodePublicKeyInfo(DWORD dwEncoding)
{
    static const BYTE bogusPubKeyInfo[] = { 0x30, 0x22, 0x30, 0x0d, 0x06, 0x06,
     0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03,
     0x11, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
     0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
    DWORD i;
    BOOL ret;
    BYTE *buf = NULL;
    DWORD bufSize = 0;

    for (i = 0; i < sizeof(pubKeys) / sizeof(pubKeys[0]); i++)
    {
        /* The NULL form decodes to the decoded member */
        ret = pCryptDecodeObjectEx(dwEncoding, X509_PUBLIC_KEY_INFO,
         pubKeys[i].encoded, pubKeys[i].encoded[1] + 2, CRYPT_DECODE_ALLOC_FLAG,
         NULL, &buf, &bufSize);
        ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
        if (ret)
        {
            comparePublicKeyInfo(&pubKeys[i].decoded,
             (CERT_PUBLIC_KEY_INFO *)buf);
            LocalFree(buf);
        }
        /* The non-NULL form decodes to the original */
        ret = pCryptDecodeObjectEx(dwEncoding, X509_PUBLIC_KEY_INFO,
         pubKeys[i].encodedNoNull, pubKeys[i].encodedNoNull[1] + 2,
         CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
        ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
        if (ret)
        {
            comparePublicKeyInfo(&pubKeys[i].info, (CERT_PUBLIC_KEY_INFO *)buf);
            LocalFree(buf);
        }
    }
    /* Test with bogus (not valid DER) parameters */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_PUBLIC_KEY_INFO,
     bogusPubKeyInfo, bogusPubKeyInfo[1] + 2, CRYPT_DECODE_ALLOC_FLAG,
     NULL, &buf, &bufSize);
    ok(!ret && (GetLastError() == CRYPT_E_ASN1_CORRUPT ||
     GetLastError() == OSS_DATA_ERROR /* Win9x */),
     "Expected CRYPT_E_ASN1_CORRUPT or OSS_DATA_ERROR, got %08x\n",
     GetLastError());
}

static const BYTE v1Cert[] = { 0x30, 0x33, 0x02, 0x00, 0x30, 0x02, 0x06, 0x00,
 0x30, 0x22, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30,
 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30,
 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x30, 0x07, 0x30,
 0x02, 0x06, 0x00, 0x03, 0x01, 0x00 };
static const BYTE v2Cert[] = { 0x30, 0x38, 0xa0, 0x03, 0x02, 0x01, 0x01, 0x02,
 0x00, 0x30, 0x02, 0x06, 0x00, 0x30, 0x22, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31,
 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x18, 0x0f,
 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30,
 0x30, 0x5a, 0x30, 0x07, 0x30, 0x02, 0x06, 0x00, 0x03, 0x01, 0x00 };
static const BYTE v3Cert[] = { 0x30, 0x38, 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02,
 0x00, 0x30, 0x02, 0x06, 0x00, 0x30, 0x22, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31,
 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x18, 0x0f,
 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30,
 0x30, 0x5a, 0x30, 0x07, 0x30, 0x02, 0x06, 0x00, 0x03, 0x01, 0x00 };
static const BYTE v4Cert[] = {
0x30,0x38,0xa0,0x03,0x02,0x01,0x03,0x02,0x00,0x30,0x02,0x06,0x00,0x30,0x22,
0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,
0x30,0x5a,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,
0x30,0x30,0x30,0x5a,0x30,0x07,0x30,0x02,0x06,0x00,0x03,0x01,0x00 };
static const BYTE v1CertWithConstraints[] = { 0x30, 0x4b, 0x02, 0x00, 0x30,
 0x02, 0x06, 0x00, 0x30, 0x22, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31,
 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x18, 0x0f, 0x31, 0x36,
 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a,
 0x30, 0x07, 0x30, 0x02, 0x06, 0x00, 0x03, 0x01, 0x00, 0xa3, 0x16, 0x30, 0x14,
 0x30, 0x12, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04, 0x08, 0x30,
 0x06, 0x01, 0x01, 0xff, 0x02, 0x01, 0x01 };
static const BYTE v1CertWithSerial[] = { 0x30, 0x4c, 0x02, 0x01, 0x01, 0x30,
 0x02, 0x06, 0x00, 0x30, 0x22, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31,
 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x18, 0x0f, 0x31, 0x36,
 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a,
 0x30, 0x07, 0x30, 0x02, 0x06, 0x00, 0x03, 0x01, 0x00, 0xa3, 0x16, 0x30, 0x14,
 0x30, 0x12, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04, 0x08, 0x30,
 0x06, 0x01, 0x01, 0xff, 0x02, 0x01, 0x01 };
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
static const BYTE v1CertWithPubKey[] = {
0x30,0x81,0x95,0x02,0x01,0x01,0x30,0x02,0x06,0x00,0x30,0x15,0x31,0x13,0x30,
0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,
0x6e,0x67,0x00,0x30,0x22,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,
0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,
0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x15,0x31,0x13,0x30,0x11,
0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,
0x67,0x00,0x30,0x22,0x30,0x0d,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,
0x01,0x01,0x05,0x00,0x03,0x11,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0xa3,0x16,0x30,0x14,0x30,0x12,0x06,
0x03,0x55,0x1d,0x13,0x01,0x01,0xff,0x04,0x08,0x30,0x06,0x01,0x01,0xff,0x02,
0x01,0x01 };
static const BYTE v1CertWithPubKeyNoNull[] = {
0x30,0x81,0x93,0x02,0x01,0x01,0x30,0x02,0x06,0x00,0x30,0x15,0x31,0x13,0x30,
0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,
0x6e,0x67,0x00,0x30,0x22,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,
0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,
0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x15,0x31,0x13,0x30,0x11,
0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,
0x67,0x00,0x30,0x20,0x30,0x0b,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,
0x01,0x01,0x03,0x11,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,
0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0xa3,0x16,0x30,0x14,0x30,0x12,0x06,0x03,0x55,
0x1d,0x13,0x01,0x01,0xff,0x04,0x08,0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01 };
static const BYTE v1CertWithSubjectKeyId[] = {
0x30,0x7b,0x02,0x01,0x01,0x30,0x02,0x06,0x00,0x30,0x15,0x31,0x13,0x30,0x11,
0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,
0x67,0x00,0x30,0x22,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,
0x30,0x30,0x30,0x30,0x30,0x5a,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,
0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x15,0x31,0x13,0x30,0x11,0x06,
0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,
0x00,0x30,0x07,0x30,0x02,0x06,0x00,0x03,0x01,0x00,0xa3,0x17,0x30,0x15,0x30,
0x13,0x06,0x03,0x55,0x1d,0x0e,0x04,0x0c,0x04,0x0a,0x4a,0x75,0x61,0x6e,0x20,
0x4c,0x61,0x6e,0x67,0x00 };
static const BYTE v1CertWithIssuerUniqueId[] = {
0x30,0x38,0x02,0x01,0x01,0x30,0x02,0x06,0x00,0x30,0x22,0x18,0x0f,0x31,0x36,
0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x18,0x0f,
0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,
0x30,0x07,0x30,0x02,0x06,0x00,0x03,0x01,0x00,0x81,0x02,0x00,0x01 };
static const BYTE v1CertWithSubjectIssuerSerialAndIssuerUniqueId[] = {
0x30,0x81,0x99,0x02,0x01,0x01,0x30,0x02,0x06,0x00,0x30,0x15,0x31,0x13,0x30,
0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,
0x6e,0x67,0x00,0x30,0x22,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,
0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,
0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x15,0x31,0x13,0x30,0x11,
0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,
0x67,0x00,0x30,0x22,0x30,0x0d,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,
0x01,0x01,0x05,0x00,0x03,0x11,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x81,0x02,0x00,0x01,0xa3,0x16,0x30,
0x14,0x30,0x12,0x06,0x03,0x55,0x1d,0x13,0x01,0x01,0xff,0x04,0x08,0x30,0x06,
0x01,0x01,0xff,0x02,0x01,0x01 };
static const BYTE v1CertWithSubjectIssuerSerialAndIssuerUniqueIdNoNull[] = {
0x30,0x81,0x97,0x02,0x01,0x01,0x30,0x02,0x06,0x00,0x30,0x15,0x31,0x13,0x30,
0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,
0x6e,0x67,0x00,0x30,0x22,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,
0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,
0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x15,0x31,0x13,0x30,0x11,
0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,
0x67,0x00,0x30,0x20,0x30,0x0b,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,
0x01,0x01,0x03,0x11,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,
0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x81,0x02,0x00,0x01,0xa3,0x16,0x30,0x14,0x30,
0x12,0x06,0x03,0x55,0x1d,0x13,0x01,0x01,0xff,0x04,0x08,0x30,0x06,0x01,0x01,
0xff,0x02,0x01,0x01 };

static const BYTE serialNum[] = { 0x01 };

static void test_encodeCertToBeSigned(DWORD dwEncoding)
{
    BOOL ret;
    BYTE *buf = NULL;
    DWORD size = 0;
    CERT_INFO info = { 0 };
    static char oid_rsa_rsa[] = szOID_RSA_RSA;
    static char oid_subject_key_identifier[] = szOID_SUBJECT_KEY_IDENTIFIER;
    CERT_EXTENSION ext;

    if (0)
    {
        /* Test with NULL pvStructInfo (crashes on win9x) */
        ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, NULL,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
        ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
         "Expected STATUS_ACCESS_VIOLATION, got %08x\n", GetLastError());
    }
    /* Test with a V1 cert */
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret || GetLastError() == OSS_BAD_PTR /* Win9x */,
     "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == v1Cert[1] + 2, "Expected size %d, got %d\n",
         v1Cert[1] + 2, size);
        ok(!memcmp(buf, v1Cert, size), "Got unexpected value\n");
        LocalFree(buf);
    }
    /* Test v2 cert */
    info.dwVersion = CERT_V2;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret || GetLastError() == OSS_BAD_PTR /* Win9x */,
     "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(v2Cert), "Wrong size %d\n", size);
        ok(!memcmp(buf, v2Cert, size), "Got unexpected value\n");
        LocalFree(buf);
    }
    /* Test v3 cert */
    info.dwVersion = CERT_V3;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret || GetLastError() == OSS_BAD_PTR /* Win9x */,
     "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(v3Cert), "Wrong size %d\n", size);
        ok(!memcmp(buf, v3Cert, size), "Got unexpected value\n");
        LocalFree(buf);
    }
    /* A v4 cert? */
    info.dwVersion = 3; /* Not a typo, CERT_V3 is 2 */
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (ret)
    {
        ok(size == sizeof(v4Cert), "Wrong size %d\n", size);
        ok(!memcmp(buf, v4Cert, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* see if a V1 cert can have basic constraints set (RFC3280 says no, but
     * API doesn't prevent it)
     */
    info.dwVersion = CERT_V1;
    info.cExtension = 1;
    info.rgExtension = &criticalExt;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret || GetLastError() == OSS_BAD_PTR /* Win9x */,
     "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(v1CertWithConstraints), "Wrong size %d\n", size);
        ok(!memcmp(buf, v1CertWithConstraints, size), "Got unexpected value\n");
        LocalFree(buf);
    }
    /* test v1 cert with a serial number */
    info.SerialNumber.cbData = sizeof(serialNum);
    info.SerialNumber.pbData = (BYTE *)serialNum;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (ret)
    {
        ok(size == sizeof(v1CertWithSerial), "Wrong size %d\n", size);
        ok(!memcmp(buf, v1CertWithSerial, size), "Got unexpected value\n");
        LocalFree(buf);
    }
    /* Test v1 cert with an issuer name, serial number, and issuer unique id */
    info.dwVersion = CERT_V1;
    info.cExtension = 0;
    info.IssuerUniqueId.cbData = sizeof(serialNum);
    info.IssuerUniqueId.pbData = (BYTE *)serialNum;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret || broken(GetLastError() == OSS_BAD_PTR /* Win98 */),
     "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(v1CertWithIssuerUniqueId), "Wrong size %d\n", size);
        ok(!memcmp(buf, v1CertWithIssuerUniqueId, size),
         "Got unexpected value\n");
        LocalFree(buf);
    }
    /* Test v1 cert with an issuer name, a subject name, and a serial number */
    info.IssuerUniqueId.cbData = 0;
    info.IssuerUniqueId.pbData = NULL;
    info.cExtension = 1;
    info.rgExtension = &criticalExt;
    info.Issuer.cbData = sizeof(encodedCommonName);
    info.Issuer.pbData = (BYTE *)encodedCommonName;
    info.Subject.cbData = sizeof(encodedCommonName);
    info.Subject.pbData = (BYTE *)encodedCommonName;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (ret)
    {
        ok(size == sizeof(bigCert), "Wrong size %d\n", size);
        ok(!memcmp(buf, bigCert, size), "Got unexpected value\n");
        LocalFree(buf);
    }
    /* Add a public key */
    info.SubjectPublicKeyInfo.Algorithm.pszObjId = oid_rsa_rsa;
    info.SubjectPublicKeyInfo.PublicKey.cbData = sizeof(aKey);
    info.SubjectPublicKeyInfo.PublicKey.pbData = (LPBYTE)aKey;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (ret)
    {
        ok(size == sizeof(v1CertWithPubKey) ||
         size == sizeof(v1CertWithPubKeyNoNull), "Wrong size %d\n", size);
        if (size == sizeof(v1CertWithPubKey))
            ok(!memcmp(buf, v1CertWithPubKey, size), "Got unexpected value\n");
        else if (size == sizeof(v1CertWithPubKeyNoNull))
            ok(!memcmp(buf, v1CertWithPubKeyNoNull, size),
             "Got unexpected value\n");
        LocalFree(buf);
    }
    /* Again add an issuer unique id */
    info.IssuerUniqueId.cbData = sizeof(serialNum);
    info.IssuerUniqueId.pbData = (BYTE *)serialNum;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(v1CertWithSubjectIssuerSerialAndIssuerUniqueId) ||
         size == sizeof(v1CertWithSubjectIssuerSerialAndIssuerUniqueIdNoNull),
         "Wrong size %d\n", size);
        if (size == sizeof(v1CertWithSubjectIssuerSerialAndIssuerUniqueId))
            ok(!memcmp(buf, v1CertWithSubjectIssuerSerialAndIssuerUniqueId,
             size), "unexpected value\n");
        else if (size ==
         sizeof(v1CertWithSubjectIssuerSerialAndIssuerUniqueIdNoNull))
            ok(!memcmp(buf,
             v1CertWithSubjectIssuerSerialAndIssuerUniqueIdNoNull, size),
             "unexpected value\n");
        LocalFree(buf);
    }
    /* Remove the public key, and add a subject key identifier extension */
    info.IssuerUniqueId.cbData = 0;
    info.IssuerUniqueId.pbData = NULL;
    info.SubjectPublicKeyInfo.Algorithm.pszObjId = NULL;
    info.SubjectPublicKeyInfo.PublicKey.cbData = 0;
    info.SubjectPublicKeyInfo.PublicKey.pbData = NULL;
    ext.pszObjId = oid_subject_key_identifier;
    ext.fCritical = FALSE;
    ext.Value.cbData = sizeof(octetCommonNameValue);
    ext.Value.pbData = octetCommonNameValue;
    info.cExtension = 1;
    info.rgExtension = &ext;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (ret)
    {
        ok(size == sizeof(v1CertWithSubjectKeyId), "Wrong size %d\n", size);
        ok(!memcmp(buf, v1CertWithSubjectKeyId, size), "Unexpected value\n");
        LocalFree(buf);
    }
}

static void test_decodeCertToBeSigned(DWORD dwEncoding)
{
    static const BYTE *corruptCerts[] = { v1Cert, v2Cert, v3Cert, v4Cert,
     v1CertWithConstraints, v1CertWithSerial, v1CertWithIssuerUniqueId };
    BOOL ret;
    BYTE *buf = NULL;
    DWORD size = 0, i;

    /* Test with NULL pbEncoded */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, NULL, 0,
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && (GetLastError() == CRYPT_E_ASN1_EOD ||
     GetLastError() == OSS_BAD_ARG /* Win9x */),
     "Expected CRYPT_E_ASN1_EOD or OSS_BAD_ARG, got %08x\n", GetLastError());
    if (0)
    {
        /* Crashes on win9x */
        ret = pCryptDecodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, NULL, 1,
         CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
        ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
         "Expected STATUS_ACCESS_VIOLATION, got %08x\n", GetLastError());
    }
    /* The following certs all fail with CRYPT_E_ASN1_CORRUPT or
     * CRYPT_E_ASN1_BADTAG, because at a minimum a cert must have a non-zero
     * serial number, an issuer, a subject, and a public key.
     */
    for (i = 0; i < sizeof(corruptCerts) / sizeof(corruptCerts[0]); i++)
    {
        ret = pCryptDecodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED,
         corruptCerts[i], corruptCerts[i][1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL,
         &buf, &size);
        ok(!ret, "Expected failure\n");
    }
    /* The following succeeds, even though v1 certs are not allowed to have
     * extensions.
     */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED,
     v1CertWithSubjectKeyId, sizeof(v1CertWithSubjectKeyId),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CERT_INFO *info = (CERT_INFO *)buf;

        ok(size >= sizeof(CERT_INFO), "Wrong size %d\n", size);
        ok(info->dwVersion == CERT_V1, "expected CERT_V1, got %d\n",
         info->dwVersion);
        ok(info->cExtension == 1, "expected 1 extension, got %d\n",
         info->cExtension);
        LocalFree(buf);
    }
    /* The following also succeeds, even though V1 certs are not allowed to
     * have issuer unique ids.
     */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED,
     v1CertWithSubjectIssuerSerialAndIssuerUniqueId,
     sizeof(v1CertWithSubjectIssuerSerialAndIssuerUniqueId),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CERT_INFO *info = (CERT_INFO *)buf;

        ok(size >= sizeof(CERT_INFO), "Wrong size %d\n", size);
        ok(info->dwVersion == CERT_V1, "expected CERT_V1, got %d\n",
         info->dwVersion);
        ok(info->IssuerUniqueId.cbData == sizeof(serialNum),
         "unexpected issuer unique id size %d\n", info->IssuerUniqueId.cbData);
        ok(!memcmp(info->IssuerUniqueId.pbData, serialNum, sizeof(serialNum)),
         "unexpected issuer unique id value\n");
        LocalFree(buf);
    }
    /* Now check with serial number, subject and issuer specified */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, bigCert,
     sizeof(bigCert), CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CERT_INFO *info = (CERT_INFO *)buf;

        ok(size >= sizeof(CERT_INFO), "Wrong size %d\n", size);
        ok(info->SerialNumber.cbData == 1,
         "Expected serial number size 1, got %d\n", info->SerialNumber.cbData);
        ok(*info->SerialNumber.pbData == *serialNum,
         "Expected serial number %d, got %d\n", *serialNum,
         *info->SerialNumber.pbData);
        ok(info->Issuer.cbData == sizeof(encodedCommonName),
         "Wrong size %d\n", info->Issuer.cbData);
        ok(!memcmp(info->Issuer.pbData, encodedCommonName, info->Issuer.cbData),
         "Unexpected issuer\n");
        ok(info->Subject.cbData == sizeof(encodedCommonName),
         "Wrong size %d\n", info->Subject.cbData);
        ok(!memcmp(info->Subject.pbData, encodedCommonName,
         info->Subject.cbData), "Unexpected subject\n");
        LocalFree(buf);
    }
    /* Check again with pub key specified */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED,
     v1CertWithPubKey, sizeof(v1CertWithPubKey), CRYPT_DECODE_ALLOC_FLAG, NULL,
     &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CERT_INFO *info = (CERT_INFO *)buf;

        ok(size >= sizeof(CERT_INFO), "Wrong size %d\n", size);
        ok(info->SerialNumber.cbData == 1,
         "Expected serial number size 1, got %d\n", info->SerialNumber.cbData);
        ok(*info->SerialNumber.pbData == *serialNum,
         "Expected serial number %d, got %d\n", *serialNum,
         *info->SerialNumber.pbData);
        ok(info->Issuer.cbData == sizeof(encodedCommonName),
         "Wrong size %d\n", info->Issuer.cbData);
        ok(!memcmp(info->Issuer.pbData, encodedCommonName, info->Issuer.cbData),
         "Unexpected issuer\n");
        ok(info->Subject.cbData == sizeof(encodedCommonName),
         "Wrong size %d\n", info->Subject.cbData);
        ok(!memcmp(info->Subject.pbData, encodedCommonName,
         info->Subject.cbData), "Unexpected subject\n");
        ok(!strcmp(info->SubjectPublicKeyInfo.Algorithm.pszObjId,
         szOID_RSA_RSA), "Expected szOID_RSA_RSA, got %s\n",
         info->SubjectPublicKeyInfo.Algorithm.pszObjId);
        ok(info->SubjectPublicKeyInfo.PublicKey.cbData == sizeof(aKey),
         "Wrong size %d\n", info->SubjectPublicKeyInfo.PublicKey.cbData);
        ok(!memcmp(info->SubjectPublicKeyInfo.PublicKey.pbData, aKey,
         sizeof(aKey)), "Unexpected public key\n");
        LocalFree(buf);
    }
}

static const BYTE hash[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xa, 0xb, 0xc, 0xd,
 0xe, 0xf };

static const BYTE signedBigCert[] = {
 0x30, 0x81, 0x93, 0x30, 0x7a, 0x02, 0x01, 0x01, 0x30, 0x02, 0x06, 0x00, 0x30,
 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x4a,
 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x30, 0x22, 0x18, 0x0f,
 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30,
 0x30, 0x5a, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30,
 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06,
 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61,
 0x6e, 0x67, 0x00, 0x30, 0x07, 0x30, 0x02, 0x06, 0x00, 0x03, 0x01, 0x00, 0xa3,
 0x16, 0x30, 0x14, 0x30, 0x12, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff,
 0x04, 0x08, 0x30, 0x06, 0x01, 0x01, 0xff, 0x02, 0x01, 0x01, 0x30, 0x02, 0x06,
 0x00, 0x03, 0x11, 0x00, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07,
 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00 };

static void test_encodeCert(DWORD dwEncoding)
{
    /* Note the SignatureAlgorithm must match that in the encoded cert.  Note
     * also that bigCert is a NULL-terminated string, so don't count its
     * last byte (otherwise the signed cert won't decode.)
     */
    CERT_SIGNED_CONTENT_INFO info = { { sizeof(bigCert), (BYTE *)bigCert },
     { NULL, { 0, NULL } }, { sizeof(hash), (BYTE *)hash, 0 } };
    BOOL ret;
    BYTE *buf = NULL;
    DWORD bufSize = 0;

    ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(bufSize == sizeof(signedBigCert), "Wrong size %d\n", bufSize);
        ok(!memcmp(buf, signedBigCert, bufSize), "Unexpected cert\n");
        LocalFree(buf);
    }
}

static void test_decodeCert(DWORD dwEncoding)
{
    BOOL ret;
    BYTE *buf = NULL;
    DWORD size = 0;

    ret = pCryptDecodeObjectEx(dwEncoding, X509_CERT, signedBigCert,
     sizeof(signedBigCert), CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CERT_SIGNED_CONTENT_INFO *info = (CERT_SIGNED_CONTENT_INFO *)buf;

        ok(info->ToBeSigned.cbData == sizeof(bigCert),
         "Wrong cert size %d\n", info->ToBeSigned.cbData);
        ok(!memcmp(info->ToBeSigned.pbData, bigCert, info->ToBeSigned.cbData),
         "Unexpected cert\n");
        ok(info->Signature.cbData == sizeof(hash),
         "Wrong signature size %d\n", info->Signature.cbData);
        ok(!memcmp(info->Signature.pbData, hash, info->Signature.cbData),
         "Unexpected signature\n");
        LocalFree(buf);
    }
    /* A signed cert decodes as a CERT_INFO too */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, signedBigCert,
     sizeof(signedBigCert), CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CERT_INFO *info = (CERT_INFO *)buf;

        ok(size >= sizeof(CERT_INFO), "Wrong size %d\n", size);
        ok(info->SerialNumber.cbData == 1,
         "Expected serial number size 1, got %d\n", info->SerialNumber.cbData);
        ok(*info->SerialNumber.pbData == *serialNum,
         "Expected serial number %d, got %d\n", *serialNum,
         *info->SerialNumber.pbData);
        ok(info->Issuer.cbData == sizeof(encodedCommonName),
         "Wrong size %d\n", info->Issuer.cbData);
        ok(!memcmp(info->Issuer.pbData, encodedCommonName, info->Issuer.cbData),
         "Unexpected issuer\n");
        ok(info->Subject.cbData == sizeof(encodedCommonName),
         "Wrong size %d\n", info->Subject.cbData);
        ok(!memcmp(info->Subject.pbData, encodedCommonName,
         info->Subject.cbData), "Unexpected subject\n");
        LocalFree(buf);
    }
}

static const BYTE emptyDistPoint[] = { 0x30, 0x02, 0x30, 0x00 };
static const BYTE distPointWithUrl[] = { 0x30, 0x19, 0x30, 0x17, 0xa0, 0x15,
 0xa0, 0x13, 0x86, 0x11, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x77, 0x69,
 0x6e, 0x65, 0x68, 0x71, 0x2e, 0x6f, 0x72, 0x67 };
static const BYTE distPointWithReason[] = { 0x30, 0x06, 0x30, 0x04, 0x81, 0x02,
 0x00, 0x03 };
static const BYTE distPointWithIssuer[] = { 0x30, 0x17, 0x30, 0x15, 0xa2, 0x13,
 0x86, 0x11, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x77, 0x69, 0x6e, 0x65,
 0x68, 0x71, 0x2e, 0x6f, 0x72, 0x67 };
static const BYTE distPointWithUrlAndIssuer[] = { 0x30, 0x2e, 0x30, 0x2c, 0xa0,
 0x15, 0xa0, 0x13, 0x86, 0x11, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x77,
 0x69, 0x6e, 0x65, 0x68, 0x71, 0x2e, 0x6f, 0x72, 0x67, 0xa2, 0x13, 0x86, 0x11,
 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x77, 0x69, 0x6e, 0x65, 0x68, 0x71,
 0x2e, 0x6f, 0x72, 0x67 };
static const BYTE crlReason = CRL_REASON_KEY_COMPROMISE |
 CRL_REASON_AFFILIATION_CHANGED;

static void test_encodeCRLDistPoints(DWORD dwEncoding)
{
    CRL_DIST_POINTS_INFO info = { 0 };
    CRL_DIST_POINT point = { { 0 } };
    CERT_ALT_NAME_ENTRY entry = { 0 };
    BOOL ret;
    BYTE *buf = NULL;
    DWORD size = 0;

    /* Test with an empty info */
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CRL_DIST_POINTS, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* Test with one empty dist point */
    info.cDistPoint = 1;
    info.rgDistPoint = &point;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CRL_DIST_POINTS, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptyDistPoint), "Wrong size %d\n", size);
        ok(!memcmp(buf, emptyDistPoint, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* A dist point with an invalid name */
    point.DistPointName.dwDistPointNameChoice = CRL_DIST_POINT_FULL_NAME;
    entry.dwAltNameChoice = CERT_ALT_NAME_URL;
    U(entry).pwszURL = (LPWSTR)nihongoURL;
    U(point.DistPointName).FullName.cAltEntry = 1;
    U(point.DistPointName).FullName.rgAltEntry = &entry;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CRL_DIST_POINTS, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_IA5_STRING,
     "Expected CRYPT_E_INVALID_IA5_STRING, got %08x\n", GetLastError());
    /* The first invalid character is at index 7 */
    ok(GET_CERT_ALT_NAME_VALUE_ERR_INDEX(size) == 7,
     "Expected invalid char at index 7, got %d\n",
     GET_CERT_ALT_NAME_VALUE_ERR_INDEX(size));
    /* A dist point with (just) a valid name */
    U(entry).pwszURL = (LPWSTR)url;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CRL_DIST_POINTS, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(distPointWithUrl), "Wrong size %d\n", size);
        ok(!memcmp(buf, distPointWithUrl, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* A dist point with (just) reason flags */
    point.DistPointName.dwDistPointNameChoice = CRL_DIST_POINT_NO_NAME;
    point.ReasonFlags.cbData = sizeof(crlReason);
    point.ReasonFlags.pbData = (LPBYTE)&crlReason;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CRL_DIST_POINTS, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(distPointWithReason), "Wrong size %d\n", size);
        ok(!memcmp(buf, distPointWithReason, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* A dist point with just an issuer */
    point.ReasonFlags.cbData = 0;
    point.CRLIssuer.cAltEntry = 1;
    point.CRLIssuer.rgAltEntry = &entry;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CRL_DIST_POINTS, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(distPointWithIssuer), "Wrong size %d\n", size);
        ok(!memcmp(buf, distPointWithIssuer, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* A dist point with both a name and an issuer */
    point.DistPointName.dwDistPointNameChoice = CRL_DIST_POINT_FULL_NAME;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CRL_DIST_POINTS, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(distPointWithUrlAndIssuer),
         "Wrong size %d\n", size);
        ok(!memcmp(buf, distPointWithUrlAndIssuer, size), "Unexpected value\n");
        LocalFree(buf);
    }
}

static void test_decodeCRLDistPoints(DWORD dwEncoding)
{
    BOOL ret;
    BYTE *buf = NULL;
    DWORD size = 0;
    PCRL_DIST_POINTS_INFO info;
    PCRL_DIST_POINT point;
    PCERT_ALT_NAME_ENTRY entry;

    ret = pCryptDecodeObjectEx(dwEncoding, X509_CRL_DIST_POINTS,
     emptyDistPoint, emptyDistPoint[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL,
     &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        info = (PCRL_DIST_POINTS_INFO)buf;
        ok(size >= sizeof(CRL_DIST_POINTS_INFO) + sizeof(CRL_DIST_POINT),
         "Wrong size %d\n", size);
        ok(info->cDistPoint == 1, "Expected 1 dist points, got %d\n",
         info->cDistPoint);
        point = info->rgDistPoint;
        ok(point->DistPointName.dwDistPointNameChoice == CRL_DIST_POINT_NO_NAME,
         "Expected CRL_DIST_POINT_NO_NAME, got %d\n",
         point->DistPointName.dwDistPointNameChoice);
        ok(point->ReasonFlags.cbData == 0, "Expected no reason\n");
        ok(point->CRLIssuer.cAltEntry == 0, "Expected no issuer\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_CRL_DIST_POINTS,
     distPointWithUrl, distPointWithUrl[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL,
     &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        info = (PCRL_DIST_POINTS_INFO)buf;
        ok(size >= sizeof(CRL_DIST_POINTS_INFO) + sizeof(CRL_DIST_POINT),
         "Wrong size %d\n", size);
        ok(info->cDistPoint == 1, "Expected 1 dist points, got %d\n",
         info->cDistPoint);
        point = info->rgDistPoint;
        ok(point->DistPointName.dwDistPointNameChoice ==
         CRL_DIST_POINT_FULL_NAME,
         "Expected CRL_DIST_POINT_FULL_NAME, got %d\n",
         point->DistPointName.dwDistPointNameChoice);
        ok(U(point->DistPointName).FullName.cAltEntry == 1,
         "Expected 1 name entry, got %d\n",
         U(point->DistPointName).FullName.cAltEntry);
        entry = U(point->DistPointName).FullName.rgAltEntry;
        ok(entry->dwAltNameChoice == CERT_ALT_NAME_URL,
         "Expected CERT_ALT_NAME_URL, got %d\n", entry->dwAltNameChoice);
        ok(!lstrcmpW(U(*entry).pwszURL, url), "Unexpected name\n");
        ok(point->ReasonFlags.cbData == 0, "Expected no reason\n");
        ok(point->CRLIssuer.cAltEntry == 0, "Expected no issuer\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_CRL_DIST_POINTS,
     distPointWithReason, distPointWithReason[1] + 2, CRYPT_DECODE_ALLOC_FLAG,
     NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        info = (PCRL_DIST_POINTS_INFO)buf;
        ok(size >= sizeof(CRL_DIST_POINTS_INFO) + sizeof(CRL_DIST_POINT),
         "Wrong size %d\n", size);
        ok(info->cDistPoint == 1, "Expected 1 dist points, got %d\n",
         info->cDistPoint);
        point = info->rgDistPoint;
        ok(point->DistPointName.dwDistPointNameChoice ==
         CRL_DIST_POINT_NO_NAME,
         "Expected CRL_DIST_POINT_NO_NAME, got %d\n",
         point->DistPointName.dwDistPointNameChoice);
        ok(point->ReasonFlags.cbData == sizeof(crlReason),
         "Expected reason length\n");
        ok(!memcmp(point->ReasonFlags.pbData, &crlReason, sizeof(crlReason)),
         "Unexpected reason\n");
        ok(point->CRLIssuer.cAltEntry == 0, "Expected no issuer\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_CRL_DIST_POINTS,
     distPointWithUrlAndIssuer, distPointWithUrlAndIssuer[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        info = (PCRL_DIST_POINTS_INFO)buf;
        ok(size >= sizeof(CRL_DIST_POINTS_INFO) + sizeof(CRL_DIST_POINT),
         "Wrong size %d\n", size);
        ok(info->cDistPoint == 1, "Expected 1 dist points, got %d\n",
         info->cDistPoint);
        point = info->rgDistPoint;
        ok(point->DistPointName.dwDistPointNameChoice ==
         CRL_DIST_POINT_FULL_NAME,
         "Expected CRL_DIST_POINT_FULL_NAME, got %d\n",
         point->DistPointName.dwDistPointNameChoice);
        ok(U(point->DistPointName).FullName.cAltEntry == 1,
         "Expected 1 name entry, got %d\n",
         U(point->DistPointName).FullName.cAltEntry);
        entry = U(point->DistPointName).FullName.rgAltEntry;
        ok(entry->dwAltNameChoice == CERT_ALT_NAME_URL,
         "Expected CERT_ALT_NAME_URL, got %d\n", entry->dwAltNameChoice);
        ok(!lstrcmpW(U(*entry).pwszURL, url), "Unexpected name\n");
        ok(point->ReasonFlags.cbData == 0, "Expected no reason\n");
        ok(point->CRLIssuer.cAltEntry == 1,
         "Expected 1 issuer entry, got %d\n", point->CRLIssuer.cAltEntry);
        entry = point->CRLIssuer.rgAltEntry;
        ok(entry->dwAltNameChoice == CERT_ALT_NAME_URL,
         "Expected CERT_ALT_NAME_URL, got %d\n", entry->dwAltNameChoice);
        ok(!lstrcmpW(U(*entry).pwszURL, url), "Unexpected name\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_CRL_DIST_POINTS,
     distPointWithUrlAndIssuer, distPointWithUrlAndIssuer[1] + 2, 0,
     NULL, NULL, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    if (buf)
    {
        ret = pCryptDecodeObjectEx(dwEncoding, X509_CRL_DIST_POINTS,
         distPointWithUrlAndIssuer, distPointWithUrlAndIssuer[1] + 2, 0,
         NULL, buf, &size);
        ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
        HeapFree(GetProcessHeap(), 0, buf);
    }
}

static const BYTE badFlagsIDP[] = { 0x30,0x06,0x81,0x01,0xff,0x82,0x01,0xff };
static const BYTE emptyNameIDP[] = { 0x30,0x04,0xa0,0x02,0xa0,0x00 };
static const BYTE urlIDP[] = { 0x30,0x17,0xa0,0x15,0xa0,0x13,0x86,0x11,0x68,
 0x74,0x74,0x70,0x3a,0x2f,0x2f,0x77,0x69,0x6e,0x65,0x68,0x71,0x2e,0x6f,0x72,
 0x67 };

static void test_encodeCRLIssuingDistPoint(DWORD dwEncoding)
{
    BOOL ret;
    BYTE *buf = NULL;
    DWORD size = 0;
    CRL_ISSUING_DIST_POINT point = { { 0 } };
    CERT_ALT_NAME_ENTRY entry;

    ret = pCryptEncodeObjectEx(dwEncoding, X509_ISSUING_DIST_POINT, NULL,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (!ret && GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        skip("no X509_ISSUING_DIST_POINT encode support\n");
        return;
    }
    ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
     "Expected STATUS_ACCESS_VIOLATION, got %08x\n", GetLastError());
    ret = pCryptEncodeObjectEx(dwEncoding, X509_ISSUING_DIST_POINT, &point,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptySequence), "Unexpected size %d\n", size);
        ok(!memcmp(buf, emptySequence, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* nonsensical flags */
    point.fOnlyContainsUserCerts = TRUE;
    point.fOnlyContainsCACerts = TRUE;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_ISSUING_DIST_POINT, &point,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(badFlagsIDP), "Unexpected size %d\n", size);
        ok(!memcmp(buf, badFlagsIDP, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* unimplemented name type */
    point.fOnlyContainsCACerts = point.fOnlyContainsUserCerts = FALSE;
    point.DistPointName.dwDistPointNameChoice = CRL_DIST_POINT_ISSUER_RDN_NAME;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_ISSUING_DIST_POINT, &point,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* empty name */
    point.DistPointName.dwDistPointNameChoice = CRL_DIST_POINT_FULL_NAME;
    U(point.DistPointName).FullName.cAltEntry = 0;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_ISSUING_DIST_POINT, &point,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptyNameIDP), "Unexpected size %d\n", size);
        ok(!memcmp(buf, emptyNameIDP, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* name with URL entry */
    entry.dwAltNameChoice = CERT_ALT_NAME_URL;
    U(entry).pwszURL = (LPWSTR)url;
    U(point.DistPointName).FullName.cAltEntry = 1;
    U(point.DistPointName).FullName.rgAltEntry = &entry;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_ISSUING_DIST_POINT, &point,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(urlIDP), "Unexpected size %d\n", size);
        ok(!memcmp(buf, urlIDP, size), "Unexpected value\n");
        LocalFree(buf);
    }
}

static void compareAltNameEntry(const CERT_ALT_NAME_ENTRY *expected,
 const CERT_ALT_NAME_ENTRY *got)
{
    ok(expected->dwAltNameChoice == got->dwAltNameChoice,
     "Expected name choice %d, got %d\n", expected->dwAltNameChoice,
     got->dwAltNameChoice);
    if (expected->dwAltNameChoice == got->dwAltNameChoice)
    {
        switch (got->dwAltNameChoice)
        {
        case CERT_ALT_NAME_RFC822_NAME:
        case CERT_ALT_NAME_DNS_NAME:
        case CERT_ALT_NAME_EDI_PARTY_NAME:
        case CERT_ALT_NAME_URL:
        case CERT_ALT_NAME_REGISTERED_ID:
            ok((!U(*expected).pwszURL && !U(*got).pwszURL) ||
             (!U(*expected).pwszURL && !lstrlenW(U(*got).pwszURL)) ||
             (!U(*got).pwszURL && !lstrlenW(U(*expected).pwszURL)) ||
             !lstrcmpW(U(*expected).pwszURL, U(*got).pwszURL),
             "Unexpected name\n");
            break;
        case CERT_ALT_NAME_X400_ADDRESS:
        case CERT_ALT_NAME_DIRECTORY_NAME:
        case CERT_ALT_NAME_IP_ADDRESS:
            ok(U(*got).IPAddress.cbData == U(*expected).IPAddress.cbData,
               "Unexpected IP address length %d\n", U(*got).IPAddress.cbData);
            ok(!memcmp(U(*got).IPAddress.pbData, U(*expected).IPAddress.pbData,
                       U(*got).IPAddress.cbData), "Unexpected value\n");
            break;
        }
    }
}

static void compareAltNameInfo(const CERT_ALT_NAME_INFO *expected,
 const CERT_ALT_NAME_INFO *got)
{
    DWORD i;

    ok(expected->cAltEntry == got->cAltEntry, "Expected %d entries, got %d\n",
     expected->cAltEntry, got->cAltEntry);
    for (i = 0; i < min(expected->cAltEntry, got->cAltEntry); i++)
        compareAltNameEntry(&expected->rgAltEntry[i], &got->rgAltEntry[i]);
}

static void compareDistPointName(const CRL_DIST_POINT_NAME *expected,
 const CRL_DIST_POINT_NAME *got)
{
    ok(got->dwDistPointNameChoice == expected->dwDistPointNameChoice,
     "Unexpected name choice %d\n", got->dwDistPointNameChoice);
    if (got->dwDistPointNameChoice == CRL_DIST_POINT_FULL_NAME)
        compareAltNameInfo(&(U(*expected).FullName), &(U(*got).FullName));
}

static void compareCRLIssuingDistPoints(const CRL_ISSUING_DIST_POINT *expected,
 const CRL_ISSUING_DIST_POINT *got)
{
    compareDistPointName(&expected->DistPointName, &got->DistPointName);
    ok(got->fOnlyContainsUserCerts == expected->fOnlyContainsUserCerts,
     "Unexpected fOnlyContainsUserCerts\n");
    ok(got->fOnlyContainsCACerts == expected->fOnlyContainsCACerts,
     "Unexpected fOnlyContainsCACerts\n");
    ok(got->OnlySomeReasonFlags.cbData == expected->OnlySomeReasonFlags.cbData,
     "Unexpected reason flags\n");
    ok(got->fIndirectCRL == expected->fIndirectCRL,
     "Unexpected fIndirectCRL\n");
}

static void test_decodeCRLIssuingDistPoint(DWORD dwEncoding)
{
    BOOL ret;
    BYTE *buf = NULL;
    DWORD size = 0;
    CRL_ISSUING_DIST_POINT point = { { 0 } };

    ret = pCryptDecodeObjectEx(dwEncoding, X509_ISSUING_DIST_POINT,
     emptySequence, emptySequence[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL,
     &buf, &size);
    if (!ret && GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        skip("no X509_ISSUING_DIST_POINT decode support\n");
        return;
    }
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        compareCRLIssuingDistPoints(&point, (PCRL_ISSUING_DIST_POINT)buf);
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_ISSUING_DIST_POINT,
     badFlagsIDP, badFlagsIDP[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL,
     &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        point.fOnlyContainsUserCerts = point.fOnlyContainsCACerts = TRUE;
        compareCRLIssuingDistPoints(&point, (PCRL_ISSUING_DIST_POINT)buf);
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_ISSUING_DIST_POINT,
     emptyNameIDP, emptyNameIDP[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL,
     &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        point.fOnlyContainsCACerts = point.fOnlyContainsUserCerts = FALSE;
        point.DistPointName.dwDistPointNameChoice = CRL_DIST_POINT_FULL_NAME;
        U(point.DistPointName).FullName.cAltEntry = 0;
        compareCRLIssuingDistPoints(&point, (PCRL_ISSUING_DIST_POINT)buf);
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_ISSUING_DIST_POINT,
     urlIDP, urlIDP[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CERT_ALT_NAME_ENTRY entry;

        entry.dwAltNameChoice = CERT_ALT_NAME_URL;
        U(entry).pwszURL = (LPWSTR)url;
        U(point.DistPointName).FullName.cAltEntry = 1;
        U(point.DistPointName).FullName.rgAltEntry = &entry;
        compareCRLIssuingDistPoints(&point, (PCRL_ISSUING_DIST_POINT)buf);
        LocalFree(buf);
    }
}

static const BYTE v1CRL[] = { 0x30, 0x15, 0x30, 0x02, 0x06, 0x00, 0x18, 0x0f,
 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30,
 0x30, 0x5a };
static const BYTE v2CRL[] = { 0x30, 0x18, 0x02, 0x01, 0x01, 0x30, 0x02, 0x06,
 0x00, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30,
 0x30, 0x30, 0x30, 0x30, 0x5a };
static const BYTE v1CRLWithIssuer[] = { 0x30, 0x2c, 0x30, 0x02, 0x06, 0x00,
 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a,
 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x18, 0x0f, 0x31,
 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
 0x5a };
static const BYTE v1CRLWithIssuerAndEmptyEntry[] = { 0x30, 0x43, 0x30, 0x02,
 0x06, 0x00, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03,
 0x13, 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x18,
 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30,
 0x30, 0x30, 0x5a, 0x30, 0x15, 0x30, 0x13, 0x02, 0x00, 0x18, 0x0f, 0x31, 0x36,
 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a };
static const BYTE v1CRLWithIssuerAndEntry[] = { 0x30, 0x44, 0x30, 0x02, 0x06,
 0x00, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13,
 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x18, 0x0f,
 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30,
 0x30, 0x5a, 0x30, 0x16, 0x30, 0x14, 0x02, 0x01, 0x01, 0x18, 0x0f, 0x31, 0x36,
 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a };
static const BYTE v1CRLWithEntryExt[] = { 0x30,0x5a,0x30,0x02,0x06,0x00,0x30,
 0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,
 0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,
 0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x2c,0x30,0x2a,0x02,0x01,
 0x01,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,
 0x30,0x30,0x5a,0x30,0x14,0x30,0x12,0x06,0x03,0x55,0x1d,0x13,0x01,0x01,0xff,
 0x04,0x08,0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01 };
static const BYTE v1CRLWithExt[] = { 0x30,0x5c,0x30,0x02,0x06,0x00,0x30,0x15,
 0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,
 0x20,0x4c,0x61,0x6e,0x67,0x00,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,
 0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x16,0x30,0x14,0x02,0x01,0x01,
 0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,
 0x30,0x5a,0xa0,0x16,0x30,0x14,0x30,0x12,0x06,0x03,0x55,0x1d,0x13,0x01,0x01,
 0xff,0x04,0x08,0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01 };
static const BYTE v2CRLWithExt[] = { 0x30,0x5c,0x02,0x01,0x01,0x30,0x02,0x06,
 0x00,0x30,0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,
 0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00,0x18,0x0f,0x31,0x36,0x30,0x31,
 0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x16,0x30,0x14,
 0x02,0x01,0x01,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,
 0x30,0x30,0x30,0x30,0x5a,0xa0,0x13,0x30,0x11,0x30,0x0f,0x06,0x03,0x55,0x1d,
 0x13,0x04,0x08,0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01 };

static void test_encodeCRLToBeSigned(DWORD dwEncoding)
{
    BOOL ret;
    BYTE *buf = NULL;
    DWORD size = 0;
    CRL_INFO info = { 0 };
    CRL_ENTRY entry = { { 0 }, { 0 }, 0, 0 };

    /* Test with a V1 CRL */
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret || broken(GetLastError() == OSS_DATA_ERROR /* Win9x */),
     "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(v1CRL), "Wrong size %d\n", size);
        ok(!memcmp(buf, v1CRL, size), "Got unexpected value\n");
        LocalFree(buf);
    }
    /* Test v2 CRL */
    info.dwVersion = CRL_V2;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret || broken(GetLastError() == OSS_DATA_ERROR /* Win9x */),
     "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == v2CRL[1] + 2, "Expected size %d, got %d\n",
         v2CRL[1] + 2, size);
        ok(!memcmp(buf, v2CRL, size), "Got unexpected value\n");
        LocalFree(buf);
    }
    /* v1 CRL with a name */
    info.dwVersion = CRL_V1;
    info.Issuer.cbData = sizeof(encodedCommonName);
    info.Issuer.pbData = (BYTE *)encodedCommonName;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(v1CRLWithIssuer), "Wrong size %d\n", size);
        ok(!memcmp(buf, v1CRLWithIssuer, size), "Got unexpected value\n");
        LocalFree(buf);
    }
    if (0)
    {
        /* v1 CRL with a name and a NULL entry pointer (crashes on win9x) */
        info.cCRLEntry = 1;
        ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED, &info,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
        ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
         "Expected STATUS_ACCESS_VIOLATION, got %08x\n", GetLastError());
    }
    /* now set an empty entry */
    info.cCRLEntry = 1;
    info.rgCRLEntry = &entry;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (ret)
    {
        ok(size == sizeof(v1CRLWithIssuerAndEmptyEntry),
         "Wrong size %d\n", size);
        ok(!memcmp(buf, v1CRLWithIssuerAndEmptyEntry, size),
         "Got unexpected value\n");
        LocalFree(buf);
    }
    /* an entry with a serial number */
    entry.SerialNumber.cbData = sizeof(serialNum);
    entry.SerialNumber.pbData = (BYTE *)serialNum;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (ret)
    {
        ok(size == sizeof(v1CRLWithIssuerAndEntry),
         "Wrong size %d\n", size);
        ok(!memcmp(buf, v1CRLWithIssuerAndEntry, size),
         "Got unexpected value\n");
        LocalFree(buf);
    }
    /* an entry with an extension */
    entry.cExtension = 1;
    entry.rgExtension = &criticalExt;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(v1CRLWithEntryExt), "Wrong size %d\n", size);
        ok(!memcmp(buf, v1CRLWithEntryExt, size), "Got unexpected value\n");
        LocalFree(buf);
    }
    /* a CRL with an extension */
    entry.cExtension = 0;
    info.cExtension = 1;
    info.rgExtension = &criticalExt;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(v1CRLWithExt), "Wrong size %d\n", size);
        ok(!memcmp(buf, v1CRLWithExt, size), "Got unexpected value\n");
        LocalFree(buf);
    }
    /* a v2 CRL with an extension, this time non-critical */
    info.dwVersion = CRL_V2;
    info.rgExtension = &nonCriticalExt;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(v2CRLWithExt), "Wrong size %d\n", size);
        ok(!memcmp(buf, v2CRLWithExt, size), "Got unexpected value\n");
        LocalFree(buf);
    }
}

static const BYTE verisignCRL[] = { 0x30, 0x82, 0x01, 0xb1, 0x30, 0x82, 0x01,
 0x1a, 0x02, 0x01, 0x01, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7,
 0x0d, 0x01, 0x01, 0x02, 0x05, 0x00, 0x30, 0x61, 0x31, 0x11, 0x30, 0x0f, 0x06,
 0x03, 0x55, 0x04, 0x07, 0x13, 0x08, 0x49, 0x6e, 0x74, 0x65, 0x72, 0x6e, 0x65,
 0x74, 0x31, 0x17, 0x30, 0x15, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x13, 0x0e, 0x56,
 0x65, 0x72, 0x69, 0x53, 0x69, 0x67, 0x6e, 0x2c, 0x20, 0x49, 0x6e, 0x63, 0x2e,
 0x31, 0x33, 0x30, 0x31, 0x06, 0x03, 0x55, 0x04, 0x0b, 0x13, 0x2a, 0x56, 0x65,
 0x72, 0x69, 0x53, 0x69, 0x67, 0x6e, 0x20, 0x43, 0x6f, 0x6d, 0x6d, 0x65, 0x72,
 0x63, 0x69, 0x61, 0x6c, 0x20, 0x53, 0x6f, 0x66, 0x74, 0x77, 0x61, 0x72, 0x65,
 0x20, 0x50, 0x75, 0x62, 0x6c, 0x69, 0x73, 0x68, 0x65, 0x72, 0x73, 0x20, 0x43,
 0x41, 0x17, 0x0d, 0x30, 0x31, 0x30, 0x33, 0x32, 0x34, 0x30, 0x30, 0x30, 0x30,
 0x30, 0x30, 0x5a, 0x17, 0x0d, 0x30, 0x34, 0x30, 0x31, 0x30, 0x37, 0x32, 0x33,
 0x35, 0x39, 0x35, 0x39, 0x5a, 0x30, 0x69, 0x30, 0x21, 0x02, 0x10, 0x1b, 0x51,
 0x90, 0xf7, 0x37, 0x24, 0x39, 0x9c, 0x92, 0x54, 0xcd, 0x42, 0x46, 0x37, 0x99,
 0x6a, 0x17, 0x0d, 0x30, 0x31, 0x30, 0x31, 0x33, 0x30, 0x30, 0x30, 0x30, 0x31,
 0x32, 0x34, 0x5a, 0x30, 0x21, 0x02, 0x10, 0x75, 0x0e, 0x40, 0xff, 0x97, 0xf0,
 0x47, 0xed, 0xf5, 0x56, 0xc7, 0x08, 0x4e, 0xb1, 0xab, 0xfd, 0x17, 0x0d, 0x30,
 0x31, 0x30, 0x31, 0x33, 0x31, 0x30, 0x30, 0x30, 0x30, 0x34, 0x39, 0x5a, 0x30,
 0x21, 0x02, 0x10, 0x77, 0xe6, 0x5a, 0x43, 0x59, 0x93, 0x5d, 0x5f, 0x7a, 0x75,
 0x80, 0x1a, 0xcd, 0xad, 0xc2, 0x22, 0x17, 0x0d, 0x30, 0x30, 0x30, 0x38, 0x33,
 0x31, 0x30, 0x30, 0x30, 0x30, 0x35, 0x36, 0x5a, 0xa0, 0x1a, 0x30, 0x18, 0x30,
 0x09, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x04, 0x02, 0x30, 0x00, 0x30, 0x0b, 0x06,
 0x03, 0x55, 0x1d, 0x0f, 0x04, 0x04, 0x03, 0x02, 0x05, 0xa0, 0x30, 0x0d, 0x06,
 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x02, 0x05, 0x00, 0x03,
 0x81, 0x81, 0x00, 0x18, 0x2c, 0xe8, 0xfc, 0x16, 0x6d, 0x91, 0x4a, 0x3d, 0x88,
 0x54, 0x48, 0x5d, 0xb8, 0x11, 0xbf, 0x64, 0xbb, 0xf9, 0xda, 0x59, 0x19, 0xdd,
 0x0e, 0x65, 0xab, 0xc0, 0x0c, 0xfa, 0x67, 0x7e, 0x21, 0x1e, 0x83, 0x0e, 0xcf,
 0x9b, 0x89, 0x8a, 0xcf, 0x0c, 0x4b, 0xc1, 0x39, 0x9d, 0xe7, 0x6a, 0xac, 0x46,
 0x74, 0x6a, 0x91, 0x62, 0x22, 0x0d, 0xc4, 0x08, 0xbd, 0xf5, 0x0a, 0x90, 0x7f,
 0x06, 0x21, 0x3d, 0x7e, 0xa7, 0xaa, 0x5e, 0xcd, 0x22, 0x15, 0xe6, 0x0c, 0x75,
 0x8e, 0x6e, 0xad, 0xf1, 0x84, 0xe4, 0x22, 0xb4, 0x30, 0x6f, 0xfb, 0x64, 0x8f,
 0xd7, 0x80, 0x43, 0xf5, 0x19, 0x18, 0x66, 0x1d, 0x72, 0xa3, 0xe3, 0x94, 0x82,
 0x28, 0x52, 0xa0, 0x06, 0x4e, 0xb1, 0xc8, 0x92, 0x0c, 0x97, 0xbe, 0x15, 0x07,
 0xab, 0x7a, 0xc9, 0xea, 0x08, 0x67, 0x43, 0x4d, 0x51, 0x63, 0x3b, 0x9c, 0x9c,
 0xcd };
static const BYTE verisignCRLWithLotsOfEntries[] = {
0x30,0x82,0x1d,0xbd,0x30,0x82,0x1d,0x26,0x30,0x0d,0x06,0x09,0x2a,0x86,0x48,
0x86,0xf7,0x0d,0x01,0x01,0x04,0x05,0x00,0x30,0x61,0x31,0x11,0x30,0x0f,0x06,
0x03,0x55,0x04,0x07,0x13,0x08,0x49,0x6e,0x74,0x65,0x72,0x6e,0x65,0x74,0x31,
0x17,0x30,0x15,0x06,0x03,0x55,0x04,0x0a,0x13,0x0e,0x56,0x65,0x72,0x69,0x53,
0x69,0x67,0x6e,0x2c,0x20,0x49,0x6e,0x63,0x2e,0x31,0x33,0x30,0x31,0x06,0x03,
0x55,0x04,0x0b,0x13,0x2a,0x56,0x65,0x72,0x69,0x53,0x69,0x67,0x6e,0x20,0x43,
0x6f,0x6d,0x6d,0x65,0x72,0x63,0x69,0x61,0x6c,0x20,0x53,0x6f,0x66,0x74,0x77,
0x61,0x72,0x65,0x20,0x50,0x75,0x62,0x6c,0x69,0x73,0x68,0x65,0x72,0x73,0x20,
0x43,0x41,0x17,0x0d,0x30,0x34,0x30,0x33,0x33,0x31,0x30,0x30,0x30,0x30,0x30,
0x30,0x5a,0x17,0x0d,0x30,0x34,0x30,0x35,0x33,0x31,0x32,0x33,0x35,0x39,0x35,
0x39,0x5a,0x30,0x82,0x1c,0x92,0x30,0x21,0x02,0x10,0x01,0x22,0xb8,0xb2,0xf3,
0x76,0x42,0xcc,0x48,0x71,0xb6,0x11,0xbf,0xd1,0xcf,0xda,0x17,0x0d,0x30,0x32,
0x30,0x34,0x31,0x35,0x31,0x35,0x34,0x30,0x32,0x34,0x5a,0x30,0x21,0x02,0x10,
0x01,0x83,0x93,0xfb,0x96,0xde,0x1d,0x89,0x4e,0xc3,0x47,0x9c,0xe1,0x60,0x13,
0x63,0x17,0x0d,0x30,0x32,0x30,0x35,0x30,0x39,0x31,0x33,0x35,0x37,0x35,0x38,
0x5a,0x30,0x21,0x02,0x10,0x01,0xdc,0xdb,0x63,0xd4,0xc9,0x9f,0x31,0xb8,0x16,
0xf9,0x2c,0xf5,0xb1,0x08,0x8e,0x17,0x0d,0x30,0x32,0x30,0x34,0x31,0x38,0x31,
0x37,0x34,0x36,0x31,0x34,0x5a,0x30,0x21,0x02,0x10,0x02,0x1a,0xa6,0xaf,0x94,
0x71,0xf0,0x07,0x6e,0xf1,0x17,0xe4,0xd4,0x17,0x82,0xdb,0x17,0x0d,0x30,0x32,
0x30,0x37,0x31,0x39,0x32,0x31,0x32,0x38,0x33,0x31,0x5a,0x30,0x21,0x02,0x10,
0x02,0x4c,0xe8,0x9d,0xfd,0x5f,0x77,0x4d,0x4b,0xf5,0x79,0x8b,0xb1,0x08,0x67,
0xac,0x17,0x0d,0x30,0x32,0x30,0x32,0x31,0x32,0x30,0x36,0x31,0x36,0x35,0x30,
0x5a,0x30,0x21,0x02,0x10,0x02,0x59,0xae,0x6c,0x4c,0x21,0xf1,0x59,0x49,0x87,
0xb0,0x95,0xf9,0x65,0xf3,0x20,0x17,0x0d,0x30,0x33,0x30,0x36,0x31,0x39,0x30,
0x38,0x30,0x34,0x34,0x37,0x5a,0x30,0x21,0x02,0x10,0x03,0x3c,0x41,0x0e,0x2f,
0x42,0x5c,0x32,0x2c,0xb1,0x35,0xfe,0xe7,0x61,0x97,0xa5,0x17,0x0d,0x30,0x32,
0x30,0x34,0x32,0x34,0x31,0x39,0x34,0x37,0x30,0x32,0x5a,0x30,0x21,0x02,0x10,
0x03,0x4e,0x68,0xfa,0x8b,0xb2,0x8e,0xb9,0x72,0xea,0x72,0xe5,0x3b,0x15,0xac,
0x8b,0x17,0x0d,0x30,0x32,0x30,0x39,0x32,0x36,0x32,0x31,0x35,0x31,0x35,0x31,
0x5a,0x30,0x21,0x02,0x10,0x03,0xc9,0xa8,0xe3,0x48,0xb0,0x5f,0xcf,0x08,0xee,
0xb9,0x93,0xf9,0xe9,0xaf,0x0c,0x17,0x0d,0x30,0x32,0x30,0x34,0x31,0x38,0x31,
0x33,0x34,0x39,0x32,0x32,0x5a,0x30,0x21,0x02,0x10,0x04,0x9b,0x23,0x6a,0x37,
0x5c,0x06,0x98,0x0a,0x31,0xc8,0x86,0xdc,0x3a,0x95,0xcc,0x17,0x0d,0x30,0x32,
0x31,0x30,0x30,0x31,0x32,0x32,0x31,0x30,0x35,0x36,0x5a,0x30,0x21,0x02,0x10,
0x06,0x08,0xba,0xc7,0xac,0xf8,0x5a,0x7c,0xa1,0xf4,0x25,0x85,0xbb,0x4e,0x8c,
0x4f,0x17,0x0d,0x30,0x33,0x30,0x31,0x30,0x33,0x30,0x37,0x35,0x37,0x31,0x34,
0x5a,0x30,0x21,0x02,0x10,0x07,0x66,0x22,0x4a,0x4a,0x9d,0xff,0x6e,0xb5,0x11,
0x0b,0xa9,0x94,0xfc,0x68,0x20,0x17,0x0d,0x30,0x32,0x30,0x38,0x32,0x32,0x30,
0x31,0x34,0x30,0x31,0x32,0x5a,0x30,0x21,0x02,0x10,0x07,0x8f,0xa1,0x4d,0xb5,
0xfc,0x0c,0xc6,0x42,0x72,0x88,0x37,0x76,0x29,0x44,0x31,0x17,0x0d,0x30,0x32,
0x30,0x33,0x31,0x35,0x32,0x30,0x31,0x39,0x34,0x39,0x5a,0x30,0x21,0x02,0x10,
0x07,0xb9,0xd9,0x42,0x19,0x81,0xc4,0xfd,0x49,0x4f,0x72,0xce,0xf2,0xf8,0x6d,
0x76,0x17,0x0d,0x30,0x32,0x30,0x32,0x31,0x35,0x31,0x35,0x33,0x37,0x31,0x39,
0x5a,0x30,0x21,0x02,0x10,0x08,0x6e,0xf9,0x6c,0x7f,0xbf,0xbc,0xc8,0x86,0x70,
0x62,0x3f,0xe9,0xc4,0x2f,0x2b,0x17,0x0d,0x30,0x32,0x31,0x31,0x32,0x38,0x30,
0x30,0x32,0x38,0x31,0x34,0x5a,0x30,0x21,0x02,0x10,0x09,0x08,0xe4,0xaa,0xf5,
0x2d,0x2b,0xc0,0x15,0x9e,0x00,0x8b,0x3f,0x97,0x93,0xf9,0x17,0x0d,0x30,0x33,
0x30,0x32,0x31,0x32,0x32,0x32,0x30,0x30,0x32,0x33,0x5a,0x30,0x21,0x02,0x10,
0x09,0x13,0x0a,0x4f,0x0f,0x88,0xe5,0x50,0x05,0xc3,0x5f,0xf4,0xff,0x15,0x39,
0xdd,0x17,0x0d,0x30,0x32,0x30,0x33,0x30,0x36,0x30,0x38,0x31,0x31,0x33,0x30,
0x5a,0x30,0x21,0x02,0x10,0x09,0x8d,0xdd,0x37,0xda,0xe7,0x84,0x03,0x9d,0x98,
0x96,0xf8,0x88,0x3a,0x55,0xca,0x17,0x0d,0x30,0x32,0x30,0x32,0x32,0x31,0x32,
0x33,0x33,0x35,0x32,0x36,0x5a,0x30,0x21,0x02,0x10,0x0a,0x35,0x0c,0xd7,0xf4,
0x53,0xe6,0xc1,0x4e,0xf2,0x2a,0xd3,0xce,0xf8,0x7c,0xe7,0x17,0x0d,0x30,0x32,
0x30,0x38,0x30,0x32,0x32,0x32,0x32,0x34,0x32,0x38,0x5a,0x30,0x21,0x02,0x10,
0x0b,0x9c,0xb8,0xf8,0xfb,0x35,0x38,0xf2,0x91,0xfd,0xa1,0xe9,0x69,0x4a,0xb1,
0x24,0x17,0x0d,0x30,0x33,0x30,0x34,0x30,0x38,0x30,0x31,0x30,0x32,0x32,0x32,
0x5a,0x30,0x21,0x02,0x10,0x0c,0x2f,0x7f,0x32,0x15,0xe0,0x2f,0x74,0xfa,0x05,
0x22,0x67,0xbc,0x8a,0x2d,0xd0,0x17,0x0d,0x30,0x32,0x30,0x32,0x32,0x36,0x31,
0x39,0x30,0x37,0x35,0x34,0x5a,0x30,0x21,0x02,0x10,0x0c,0x32,0x5b,0x78,0x32,
0xc6,0x7c,0xd8,0xdd,0x25,0x91,0x22,0x4d,0x84,0x0a,0x94,0x17,0x0d,0x30,0x32,
0x30,0x33,0x31,0x38,0x31,0x32,0x33,0x39,0x30,0x33,0x5a,0x30,0x21,0x02,0x10,
0x0d,0x76,0x36,0xb9,0x1c,0x72,0xb7,0x9d,0xdf,0xa5,0x35,0x82,0xc5,0xa8,0xf7,
0xbb,0x17,0x0d,0x30,0x32,0x30,0x38,0x32,0x37,0x32,0x31,0x34,0x32,0x31,0x31,
0x5a,0x30,0x21,0x02,0x10,0x0f,0x28,0x79,0x98,0x56,0xb8,0xa5,0x5e,0xeb,0x79,
0x5f,0x1b,0xed,0x0b,0x86,0x76,0x17,0x0d,0x30,0x32,0x30,0x33,0x31,0x33,0x30,
0x31,0x31,0x30,0x34,0x37,0x5a,0x30,0x21,0x02,0x10,0x0f,0x80,0x3c,0x24,0xf4,
0x62,0x27,0x24,0xbe,0x6a,0x74,0x9c,0x18,0x8e,0x4b,0x3b,0x17,0x0d,0x30,0x32,
0x31,0x31,0x32,0x30,0x31,0x37,0x31,0x31,0x33,0x35,0x5a,0x30,0x21,0x02,0x10,
0x0f,0xf2,0xa7,0x8c,0x80,0x9c,0xbe,0x2f,0xc8,0xa9,0xeb,0xfe,0x94,0x86,0x5a,
0x5c,0x17,0x0d,0x30,0x32,0x30,0x36,0x32,0x30,0x31,0x39,0x35,0x38,0x34,0x35,
0x5a,0x30,0x21,0x02,0x10,0x10,0x45,0x13,0x35,0x45,0xf3,0xc6,0x02,0x8d,0x8d,
0x18,0xb1,0xc4,0x0a,0x7a,0x18,0x17,0x0d,0x30,0x32,0x30,0x34,0x32,0x36,0x31,
0x37,0x33,0x32,0x35,0x39,0x5a,0x30,0x21,0x02,0x10,0x10,0x79,0xb1,0x71,0x1b,
0x26,0x98,0x92,0x08,0x1e,0x3c,0xe4,0x8b,0x29,0x37,0xf9,0x17,0x0d,0x30,0x32,
0x30,0x33,0x32,0x38,0x31,0x36,0x33,0x32,0x35,0x35,0x5a,0x30,0x21,0x02,0x10,
0x11,0x38,0x80,0x77,0xcb,0x6b,0xe5,0xd6,0xa7,0xf2,0x99,0xa1,0xc8,0xe9,0x40,
0x25,0x17,0x0d,0x30,0x32,0x30,0x34,0x31,0x39,0x31,0x32,0x32,0x34,0x31,0x37,
0x5a,0x30,0x21,0x02,0x10,0x11,0x7a,0xc3,0x82,0xfe,0x74,0x36,0x11,0x21,0xd6,
0x92,0x86,0x09,0xdf,0xe6,0xf3,0x17,0x0d,0x30,0x32,0x30,0x32,0x31,0x39,0x31,
0x35,0x31,0x31,0x33,0x36,0x5a,0x30,0x21,0x02,0x10,0x11,0xab,0x8e,0x21,0x28,
0x7f,0x6d,0xf2,0xc1,0xc8,0x40,0x3e,0xa5,0xde,0x98,0xd3,0x17,0x0d,0x30,0x32,
0x30,0x35,0x30,0x32,0x31,0x38,0x34,0x34,0x33,0x31,0x5a,0x30,0x21,0x02,0x10,
0x12,0x3c,0x38,0xae,0x3f,0x64,0x53,0x3a,0xf7,0xbc,0x6c,0x27,0xe2,0x9c,0x65,
0x75,0x17,0x0d,0x30,0x32,0x30,0x32,0x31,0x33,0x32,0x33,0x30,0x38,0x35,0x39,
0x5a,0x30,0x21,0x02,0x10,0x12,0x88,0xb6,0x6c,0x9b,0xcf,0xe7,0x50,0x92,0xd2,
0x87,0x63,0x8f,0xb7,0xa6,0xe3,0x17,0x0d,0x30,0x32,0x30,0x37,0x30,0x32,0x32,
0x30,0x35,0x35,0x30,0x33,0x5a,0x30,0x21,0x02,0x10,0x12,0x95,0x4e,0xb6,0x8f,
0x3a,0x19,0x6a,0x16,0x73,0x4f,0x6e,0x15,0xba,0xa5,0xe7,0x17,0x0d,0x30,0x32,
0x30,0x36,0x31,0x37,0x31,0x38,0x35,0x36,0x30,0x31,0x5a,0x30,0x21,0x02,0x10,
0x13,0x37,0x0b,0x41,0x8c,0x31,0x43,0x1c,0x27,0xaa,0xe1,0x83,0x0f,0x99,0x21,
0xcd,0x17,0x0d,0x30,0x32,0x30,0x37,0x32,0x32,0x31,0x32,0x31,0x37,0x31,0x36,
0x5a,0x30,0x21,0x02,0x10,0x14,0x7a,0x29,0x0a,0x09,0x38,0xf4,0x53,0x28,0x33,
0x6f,0x37,0x07,0x23,0x12,0x10,0x17,0x0d,0x30,0x32,0x30,0x32,0x32,0x32,0x30,
0x32,0x30,0x30,0x31,0x34,0x5a,0x30,0x21,0x02,0x10,0x15,0x04,0x81,0x1e,0xe2,
0x6f,0xf0,0xd8,0xdd,0x12,0x55,0x05,0x66,0x51,0x6e,0x1a,0x17,0x0d,0x30,0x32,
0x30,0x33,0x31,0x33,0x31,0x30,0x35,0x33,0x30,0x38,0x5a,0x30,0x21,0x02,0x10,
0x15,0x30,0x0d,0x8a,0xbd,0x0e,0x89,0x0e,0x66,0x4f,0x49,0x93,0xa2,0x8f,0xbc,
0x2e,0x17,0x0d,0x30,0x32,0x30,0x34,0x30,0x34,0x30,0x36,0x34,0x32,0x32,0x33,
0x5a,0x30,0x21,0x02,0x10,0x16,0xbe,0x64,0xd6,0x4f,0x90,0xf4,0xf7,0x2b,0xc8,
0xca,0x67,0x5c,0x82,0x13,0xe8,0x17,0x0d,0x30,0x32,0x30,0x36,0x30,0x36,0x31,
0x39,0x30,0x39,0x30,0x37,0x5a,0x30,0x21,0x02,0x10,0x18,0x51,0x9c,0xe4,0x48,
0x62,0x06,0xfe,0xb8,0x2d,0x93,0xb7,0xc9,0xc9,0x1b,0x4e,0x17,0x0d,0x30,0x32,
0x30,0x34,0x31,0x37,0x30,0x35,0x30,0x30,0x34,0x34,0x5a,0x30,0x21,0x02,0x10,
0x19,0x82,0xdb,0x39,0x74,0x00,0x38,0x36,0x59,0xf6,0xcc,0xc1,0x23,0x8d,0x40,
0xe9,0x17,0x0d,0x30,0x32,0x30,0x33,0x30,0x36,0x30,0x37,0x35,0x34,0x35,0x34,
0x5a,0x30,0x21,0x02,0x10,0x1b,0x51,0x90,0xf7,0x37,0x24,0x39,0x9c,0x92,0x54,
0xcd,0x42,0x46,0x37,0x99,0x6a,0x17,0x0d,0x30,0x31,0x30,0x31,0x33,0x30,0x30,
0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x21,0x02,0x10,0x1b,0xe4,0xb2,0xbb,0xb6,
0x74,0x5d,0x6b,0x8b,0x04,0xb6,0xa0,0x1b,0x35,0xeb,0x29,0x17,0x0d,0x30,0x32,
0x30,0x39,0x32,0x35,0x32,0x30,0x31,0x34,0x35,0x36,0x5a,0x30,0x21,0x02,0x10,
0x1c,0x1d,0xd5,0x2a,0xf6,0xaa,0xfd,0xbb,0x47,0xc2,0x73,0x36,0xcf,0x53,0xbd,
0x81,0x17,0x0d,0x30,0x32,0x30,0x32,0x31,0x33,0x31,0x39,0x30,0x33,0x34,0x32,
0x5a,0x30,0x21,0x02,0x10,0x1c,0xb0,0x5a,0x1f,0xfd,0xa6,0x98,0xf6,0x46,0xf9,
0x32,0x10,0x9e,0xef,0x52,0x8e,0x17,0x0d,0x30,0x32,0x30,0x36,0x32,0x37,0x31,
0x33,0x30,0x33,0x32,0x32,0x5a,0x30,0x21,0x02,0x10,0x1d,0x01,0xfc,0xa7,0xdd,
0xb4,0x0c,0x64,0xbd,0x65,0x45,0xe6,0xbf,0x1c,0x7e,0x90,0x17,0x0d,0x30,0x32,
0x30,0x32,0x32,0x31,0x30,0x34,0x32,0x30,0x30,0x36,0x5a,0x30,0x21,0x02,0x10,
0x1e,0x4d,0xc9,0xc6,0x6e,0x57,0xda,0x8a,0x07,0x97,0x70,0xfa,0xee,0x9c,0xc5,
0x58,0x17,0x0d,0x30,0x32,0x30,0x32,0x31,0x39,0x32,0x32,0x33,0x34,0x32,0x31,
0x5a,0x30,0x21,0x02,0x10,0x1e,0xbb,0x9b,0x28,0x61,0x50,0x7f,0x12,0x30,0xfb,
0x02,0xb5,0xe1,0xb0,0x7e,0x9d,0x17,0x0d,0x30,0x32,0x30,0x33,0x30,0x36,0x30,
0x30,0x30,0x34,0x32,0x30,0x5a,0x30,0x21,0x02,0x10,0x1f,0x5a,0x64,0xc9,0xa5,
0x51,0x8c,0xe2,0x2d,0x50,0x83,0xc2,0x4c,0x7c,0xe7,0x85,0x17,0x0d,0x30,0x32,
0x30,0x38,0x32,0x34,0x30,0x36,0x33,0x31,0x32,0x38,0x5a,0x30,0x21,0x02,0x10,
0x1f,0xc2,0x4e,0xd0,0xac,0x52,0xd3,0x39,0x18,0x6d,0xd0,0x0f,0x23,0xd7,0x45,
0x72,0x17,0x0d,0x30,0x32,0x30,0x32,0x32,0x38,0x31,0x39,0x31,0x35,0x34,0x32,
0x5a,0x30,0x20,0x02,0x0f,0x24,0x60,0x7a,0x8e,0x0e,0x86,0xa4,0x88,0x68,0xaf,
0xd9,0x0c,0x6b,0xba,0xff,0x17,0x0d,0x30,0x32,0x30,0x32,0x32,0x38,0x30,0x35,
0x31,0x38,0x32,0x34,0x5a,0x30,0x21,0x02,0x10,0x20,0x41,0x73,0xbb,0x72,0x88,
0x6e,0x4b,0x1c,0xb6,0x70,0x02,0x67,0xaa,0x3b,0x3d,0x17,0x0d,0x30,0x32,0x30,
0x39,0x30,0x33,0x31,0x37,0x30,0x36,0x32,0x31,0x5a,0x30,0x21,0x02,0x10,0x20,
0x6e,0x0d,0xdc,0x8c,0xa4,0xac,0xf7,0x08,0x77,0x5c,0x80,0xf9,0xa3,0x68,0x92,
0x17,0x0d,0x30,0x32,0x30,0x34,0x31,0x30,0x32,0x30,0x35,0x37,0x31,0x36,0x5a,
0x30,0x21,0x02,0x10,0x21,0xe4,0x6b,0x98,0x47,0x91,0xe6,0x02,0xdf,0xb2,0x45,
0xbc,0x31,0x37,0xa0,0x7c,0x17,0x0d,0x30,0x32,0x30,0x33,0x30,0x38,0x32,0x33,
0x32,0x33,0x31,0x33,0x5a,0x30,0x21,0x02,0x10,0x22,0x00,0x95,0x70,0x79,0xf9,
0x9c,0x34,0x91,0xbb,0x84,0xb9,0x91,0xde,0x22,0x55,0x17,0x0d,0x30,0x32,0x30,
0x32,0x31,0x33,0x30,0x36,0x35,0x39,0x33,0x39,0x5a,0x30,0x21,0x02,0x10,0x22,
0xf9,0x67,0x4f,0xcd,0x29,0xc6,0xdc,0xc8,0x22,0x6e,0xe9,0x0a,0xa1,0x48,0x5a,
0x17,0x0d,0x30,0x32,0x30,0x34,0x30,0x33,0x30,0x30,0x34,0x33,0x32,0x36,0x5a,
0x30,0x21,0x02,0x10,0x24,0xa3,0xa7,0xd0,0xb8,0x1d,0x1c,0xf7,0xe6,0x1f,0x6e,
0xba,0xc9,0x98,0x59,0xed,0x17,0x0d,0x30,0x33,0x30,0x37,0x32,0x34,0x32,0x30,
0x35,0x38,0x30,0x32,0x5a,0x30,0x21,0x02,0x10,0x24,0xef,0x89,0xa1,0x30,0x4f,
0x51,0x63,0xfe,0xdb,0xdb,0x64,0x6e,0x4c,0x5a,0x81,0x17,0x0d,0x30,0x32,0x30,
0x37,0x30,0x33,0x30,0x39,0x32,0x31,0x31,0x37,0x5a,0x30,0x21,0x02,0x10,0x25,
0x08,0xe5,0xac,0xdd,0x6f,0x74,0x44,0x51,0x1a,0xf5,0xdb,0xf8,0xba,0x25,0xe0,
0x17,0x0d,0x30,0x32,0x30,0x34,0x30,0x39,0x30,0x34,0x31,0x36,0x32,0x32,0x5a,
0x30,0x21,0x02,0x10,0x25,0x81,0xe8,0x18,0x60,0x88,0xbc,0x1a,0xe9,0x14,0x84,
0xed,0xd4,0x62,0xf5,0x47,0x17,0x0d,0x30,0x32,0x30,0x38,0x32,0x33,0x30,0x31,
0x35,0x37,0x31,0x39,0x5a,0x30,0x21,0x02,0x10,0x26,0xe5,0x5c,0xab,0x16,0xec,
0x61,0x38,0x49,0x2c,0xd2,0xb1,0x48,0x89,0xd5,0x47,0x17,0x0d,0x30,0x32,0x30,
0x33,0x31,0x33,0x31,0x38,0x30,0x30,0x33,0x38,0x5a,0x30,0x21,0x02,0x10,0x27,
0xbe,0xda,0x7f,0x4f,0x1f,0x6c,0x76,0x09,0xc0,0x9a,0xaf,0xd4,0x68,0xe2,0x16,
0x17,0x0d,0x30,0x32,0x30,0x35,0x31,0x30,0x31,0x38,0x33,0x32,0x33,0x30,0x5a,
0x30,0x21,0x02,0x10,0x28,0x89,0xd0,0xb3,0xb5,0xc4,0x56,0x36,0x9b,0x3e,0x81,
0x1a,0x21,0x56,0xaa,0x42,0x17,0x0d,0x30,0x32,0x31,0x31,0x30,0x34,0x31,0x31,
0x30,0x33,0x30,0x38,0x5a,0x30,0x21,0x02,0x10,0x28,0xab,0x93,0x06,0xb1,0x1e,
0x05,0xe0,0xe1,0x25,0x75,0xc7,0x74,0xcb,0x55,0xa6,0x17,0x0d,0x30,0x33,0x30,
0x31,0x32,0x34,0x31,0x39,0x34,0x38,0x32,0x33,0x5a,0x30,0x21,0x02,0x10,0x29,
0xe9,0x3b,0x44,0x8d,0xc3,0x4b,0x80,0x17,0xda,0xe4,0x1c,0x43,0x96,0x83,0x59,
0x17,0x0d,0x30,0x32,0x30,0x36,0x30,0x37,0x32,0x31,0x34,0x33,0x33,0x39,0x5a,
0x30,0x21,0x02,0x10,0x2a,0x08,0x64,0x2b,0x48,0xe2,0x17,0x89,0x6a,0x0c,0xf9,
0x7e,0x10,0x66,0x8f,0xe7,0x17,0x0d,0x30,0x32,0x30,0x38,0x31,0x39,0x31,0x38,
0x33,0x35,0x32,0x39,0x5a,0x30,0x21,0x02,0x10,0x2a,0x44,0xee,0x91,0x5d,0xe3,
0xa5,0x2b,0x09,0xf3,0x56,0x59,0xe0,0x8f,0x25,0x22,0x17,0x0d,0x30,0x32,0x30,
0x32,0x32,0x31,0x31,0x39,0x33,0x31,0x32,0x34,0x5a,0x30,0x21,0x02,0x10,0x2a,
0x8b,0x4e,0xa5,0xb6,0x06,0xc8,0x48,0x3b,0x0e,0x71,0x1e,0x6b,0xf4,0x16,0xc1,
0x17,0x0d,0x30,0x32,0x30,0x34,0x33,0x30,0x30,0x39,0x32,0x31,0x31,0x38,0x5a,
0x30,0x21,0x02,0x10,0x2b,0x03,0xfc,0x2f,0xc2,0x8e,0x38,0x29,0x6f,0xa1,0x0f,
0xe9,0x47,0x1b,0x35,0xd7,0x17,0x0d,0x30,0x32,0x31,0x31,0x31,0x34,0x32,0x30,
0x31,0x38,0x33,0x33,0x5a,0x30,0x21,0x02,0x10,0x2c,0x48,0xf7,0xd6,0xd5,0x71,
0xc0,0xd1,0xbd,0x6a,0x00,0x65,0x1d,0x2d,0xa9,0xdd,0x17,0x0d,0x30,0x32,0x30,
0x33,0x30,0x36,0x31,0x37,0x32,0x30,0x34,0x33,0x5a,0x30,0x21,0x02,0x10,0x2c,
0xbf,0x84,0x1d,0xe4,0x58,0x32,0x79,0x32,0x10,0x37,0xde,0xd7,0x94,0xff,0x85,
0x17,0x0d,0x30,0x32,0x30,0x32,0x32,0x32,0x31,0x39,0x30,0x32,0x32,0x35,0x5a,
0x30,0x21,0x02,0x10,0x2d,0x03,0x54,0x35,0x54,0x45,0x2c,0x6d,0x39,0xf0,0x1b,
0x74,0x68,0xde,0xcf,0x93,0x17,0x0d,0x30,0x32,0x30,0x39,0x32,0x33,0x31,0x33,
0x32,0x33,0x33,0x37,0x5a,0x30,0x21,0x02,0x10,0x2d,0x24,0x94,0x34,0x19,0x92,
0xb1,0xf2,0x37,0x9d,0x6e,0xc5,0x35,0x93,0xdd,0xf0,0x17,0x0d,0x30,0x32,0x30,
0x33,0x31,0x35,0x31,0x37,0x31,0x37,0x32,0x37,0x5a,0x30,0x21,0x02,0x10,0x2d,
0x47,0x24,0x61,0x87,0x91,0xba,0x2e,0xf2,0xf7,0x92,0x21,0xf3,0x1b,0x8b,0x1e,
0x17,0x0d,0x30,0x32,0x30,0x35,0x31,0x34,0x32,0x33,0x30,0x38,0x32,0x32,0x5a,
0x30,0x21,0x02,0x10,0x2d,0x84,0xc2,0xb1,0x01,0xa1,0x3a,0x6f,0xb0,0x30,0x13,
0x76,0x5a,0x69,0xec,0x41,0x17,0x0d,0x30,0x32,0x30,0x37,0x31,0x35,0x31,0x37,
0x32,0x39,0x32,0x33,0x5a,0x30,0x21,0x02,0x10,0x2d,0xd5,0x26,0xc3,0xcd,0x01,
0xce,0xfd,0x67,0xb8,0x08,0xac,0x5a,0x70,0xc4,0x34,0x17,0x0d,0x30,0x32,0x30,
0x32,0x32,0x37,0x30,0x34,0x34,0x36,0x31,0x34,0x5a,0x30,0x21,0x02,0x10,0x2e,
0x2b,0x0a,0x94,0x4d,0xf1,0xa4,0x37,0xb7,0xa3,0x9b,0x4b,0x96,0x26,0xa8,0xe3,
0x17,0x0d,0x30,0x33,0x30,0x31,0x30,0x39,0x30,0x36,0x32,0x38,0x32,0x38,0x5a,
0x30,0x21,0x02,0x10,0x2e,0x31,0x30,0xc1,0x2e,0x16,0x31,0xd9,0x2b,0x0a,0x70,
0xca,0x3f,0x31,0x73,0x62,0x17,0x0d,0x30,0x33,0x30,0x31,0x32,0x39,0x30,0x31,
0x34,0x39,0x32,0x37,0x5a,0x30,0x21,0x02,0x10,0x2e,0xbd,0x6d,0xdf,0xce,0x20,
0x6f,0xe7,0xa8,0xf4,0xf3,0x25,0x9c,0xc3,0xc1,0x12,0x17,0x0d,0x30,0x32,0x30,
0x39,0x32,0x30,0x31,0x33,0x35,0x34,0x34,0x32,0x5a,0x30,0x21,0x02,0x10,0x2f,
0x56,0x16,0x22,0xba,0x87,0xd5,0xfd,0xff,0xe6,0xb0,0xdd,0x3c,0x08,0x26,0x2c,
0x17,0x0d,0x30,0x32,0x30,0x33,0x31,0x33,0x31,0x37,0x35,0x33,0x31,0x31,0x5a,
0x30,0x21,0x02,0x10,0x30,0x3e,0x77,0x7b,0xec,0xcb,0x89,0x2c,0x15,0x55,0x7f,
0x20,0xf2,0x33,0xc1,0x1e,0x17,0x0d,0x30,0x32,0x30,0x32,0x32,0x31,0x32,0x33,
0x35,0x30,0x34,0x39,0x5a,0x30,0x21,0x02,0x10,0x30,0x59,0x6c,0xaa,0x5f,0xd3,
0xac,0x50,0x86,0x2c,0xc4,0xfa,0x3c,0x48,0x50,0xd1,0x17,0x0d,0x30,0x32,0x30,
0x32,0x32,0x31,0x30,0x34,0x31,0x39,0x33,0x35,0x5a,0x30,0x21,0x02,0x10,0x30,
0xce,0x9a,0xf1,0xfa,0x17,0xfa,0xf5,0x4c,0xbc,0x52,0x8a,0xf4,0x26,0x2b,0x7b,
0x17,0x0d,0x30,0x32,0x30,0x33,0x30,0x31,0x31,0x39,0x31,0x32,0x33,0x39,0x5a,
0x30,0x21,0x02,0x10,0x31,0x16,0x4a,0x6a,0x2e,0x6d,0x34,0x4d,0xd2,0x40,0xf0,
0x5f,0x47,0xe6,0x5b,0x47,0x17,0x0d,0x30,0x32,0x30,0x32,0x31,0x32,0x31,0x37,
0x33,0x38,0x35,0x32,0x5a,0x30,0x21,0x02,0x10,0x31,0xdb,0x97,0x5b,0x06,0x63,
0x0b,0xd8,0xfe,0x06,0xb3,0xf5,0xf9,0x64,0x0a,0x59,0x17,0x0d,0x30,0x32,0x30,
0x32,0x31,0x32,0x31,0x35,0x35,0x39,0x32,0x33,0x5a,0x30,0x21,0x02,0x10,0x32,
0xbc,0xeb,0x0c,0xca,0x65,0x06,0x3f,0xa4,0xd5,0x4a,0x56,0x46,0x7c,0x22,0x09,
0x17,0x0d,0x30,0x32,0x30,0x38,0x31,0x36,0x30,0x37,0x33,0x33,0x35,0x35,0x5a,
0x30,0x21,0x02,0x10,0x33,0x17,0xef,0xe1,0x89,0xec,0x11,0x25,0x15,0x8f,0x3b,
0x67,0x7a,0x64,0x0b,0x50,0x17,0x0d,0x30,0x32,0x30,0x39,0x31,0x38,0x31,0x37,
0x30,0x33,0x34,0x36,0x5a,0x30,0x21,0x02,0x10,0x34,0x24,0xa0,0xd2,0x00,0x61,
0xeb,0xd3,0x9a,0xa7,0x2a,0x66,0xb4,0x82,0x23,0x77,0x17,0x0d,0x30,0x32,0x30,
0x33,0x31,0x35,0x32,0x32,0x34,0x33,0x33,0x39,0x5a,0x30,0x21,0x02,0x10,0x34,
0xa8,0x16,0x67,0xa5,0x1b,0xa3,0x31,0x11,0x5e,0x26,0xc8,0x3f,0x21,0x38,0xbe,
0x17,0x0d,0x30,0x32,0x30,0x33,0x32,0x31,0x32,0x31,0x31,0x36,0x32,0x31,0x5a,
0x30,0x21,0x02,0x10,0x36,0x3a,0xbe,0x05,0x55,0x52,0x93,0x4f,0x32,0x5f,0x30,
0x63,0xc0,0xd4,0x50,0xdf,0x17,0x0d,0x30,0x32,0x30,0x33,0x30,0x38,0x31,0x31,
0x34,0x36,0x31,0x34,0x5a,0x30,0x21,0x02,0x10,0x37,0x19,0xcc,0xa5,0x9d,0x85,
0x05,0x56,0xe1,0x63,0x42,0x4b,0x0d,0x3c,0xbf,0xd6,0x17,0x0d,0x30,0x33,0x30,
0x31,0x30,0x38,0x31,0x38,0x35,0x38,0x32,0x34,0x5a,0x30,0x21,0x02,0x10,0x37,
0x2f,0xfd,0x2b,0xec,0x4d,0x94,0x35,0x51,0xf4,0x07,0x2a,0xf5,0x0b,0x97,0xc4,
0x17,0x0d,0x30,0x32,0x30,0x32,0x31,0x33,0x31,0x39,0x31,0x38,0x30,0x31,0x5a,
0x30,0x21,0x02,0x10,0x37,0x83,0xf5,0x1e,0x7e,0xf4,0x5f,0xad,0x1f,0x0c,0x55,
0x86,0x30,0x02,0x54,0xc1,0x17,0x0d,0x30,0x33,0x30,0x31,0x30,0x38,0x32,0x30,
0x30,0x33,0x34,0x34,0x5a,0x30,0x21,0x02,0x10,0x38,0x32,0x3e,0x50,0x2b,0x36,
0x93,0x01,0x32,0x0a,0x59,0x8c,0xce,0xad,0xa0,0xeb,0x17,0x0d,0x30,0x32,0x30,
0x34,0x33,0x30,0x32,0x31,0x32,0x34,0x30,0x38,0x5a,0x30,0x21,0x02,0x10,0x3a,
0x62,0xd8,0x64,0xd3,0x85,0xd5,0x61,0x1d,0x9d,0x3f,0x61,0x25,0xe9,0x3a,0x1d,
0x17,0x0d,0x30,0x32,0x30,0x36,0x31,0x37,0x31,0x35,0x31,0x39,0x31,0x36,0x5a,
0x30,0x21,0x02,0x10,0x3a,0x97,0x36,0xb1,0x26,0x14,0x73,0x50,0xa3,0xcc,0x3f,
0xd0,0x3b,0x83,0x99,0xc9,0x17,0x0d,0x30,0x32,0x30,0x39,0x31,0x31,0x30,0x33,
0x32,0x39,0x33,0x30,0x5a,0x30,0x21,0x02,0x10,0x3b,0x87,0x3e,0x20,0xbe,0x97,
0xff,0xa7,0x6b,0x2b,0x5f,0xff,0x9a,0x7f,0x4c,0x95,0x17,0x0d,0x30,0x32,0x30,
0x37,0x30,0x33,0x30,0x30,0x33,0x31,0x34,0x37,0x5a,0x30,0x21,0x02,0x10,0x3b,
0xba,0xe5,0xf2,0x23,0x99,0xc6,0xd7,0xae,0xe2,0x98,0x0d,0xa4,0x13,0x5c,0xd4,
0x17,0x0d,0x30,0x32,0x30,0x35,0x32,0x34,0x31,0x39,0x32,0x38,0x34,0x35,0x5a,
0x30,0x21,0x02,0x10,0x3b,0xc2,0x7c,0xf0,0xbd,0xd2,0x9a,0x6f,0x97,0xdd,0x76,
0xbc,0xa9,0x6c,0x45,0x0d,0x17,0x0d,0x30,0x32,0x30,0x33,0x30,0x38,0x31,0x30,
0x34,0x32,0x30,0x33,0x5a,0x30,0x21,0x02,0x10,0x3b,0xc5,0xda,0x41,0x64,0x7a,
0x37,0x8e,0x9f,0x7f,0x1f,0x9b,0x25,0x0a,0xb4,0xda,0x17,0x0d,0x30,0x32,0x30,
0x33,0x30,0x36,0x31,0x33,0x32,0x34,0x34,0x38,0x5a,0x30,0x21,0x02,0x10,0x3c,
0x1b,0xf1,0x9a,0x48,0xb0,0xb8,0xa0,0x45,0xd5,0x8f,0x0f,0x57,0x90,0xc2,0xcd,
0x17,0x0d,0x30,0x32,0x30,0x33,0x31,0x38,0x30,0x36,0x34,0x33,0x32,0x33,0x5a,
0x30,0x21,0x02,0x10,0x3d,0x15,0x48,0x80,0xb4,0xfe,0x51,0x7e,0xed,0x46,0xae,
0x51,0xfd,0x47,0x73,0xde,0x17,0x0d,0x30,0x32,0x30,0x38,0x32,0x37,0x30,0x39,
0x32,0x30,0x30,0x38,0x5a,0x30,0x21,0x02,0x10,0x3d,0x61,0x4e,0x87,0xea,0x39,
0x02,0xf3,0x1e,0x3e,0x56,0x5c,0x0e,0x3b,0xa7,0xe3,0x17,0x0d,0x30,0x32,0x31,
0x30,0x32,0x39,0x31,0x39,0x35,0x34,0x31,0x32,0x5a,0x30,0x21,0x02,0x10,0x3d,
0xdd,0x61,0x92,0x82,0x69,0x6b,0x01,0x79,0x0e,0xef,0x96,0x12,0xa3,0x76,0x80,
0x17,0x0d,0x30,0x32,0x30,0x35,0x30,0x31,0x32,0x32,0x32,0x34,0x31,0x36,0x5a,
0x30,0x21,0x02,0x10,0x3e,0x0e,0x14,0x71,0x55,0xf3,0x48,0x09,0x1b,0x56,0x3b,
0x91,0x7a,0x7d,0xec,0xc9,0x17,0x0d,0x30,0x32,0x30,0x33,0x31,0x31,0x32,0x31,
0x34,0x35,0x35,0x31,0x5a,0x30,0x21,0x02,0x10,0x3e,0x23,0x00,0x1f,0x9b,0xbd,
0xe8,0xb1,0xf0,0x06,0x67,0xa6,0x70,0x42,0x2e,0xc3,0x17,0x0d,0x30,0x32,0x30,
0x38,0x30,0x38,0x31,0x32,0x32,0x31,0x33,0x32,0x5a,0x30,0x21,0x02,0x10,0x41,
0x91,0x1a,0x8c,0xde,0x2d,0xb3,0xeb,0x79,0x1d,0xc7,0x99,0x99,0xbe,0x0c,0x0e,
0x17,0x0d,0x30,0x32,0x30,0x32,0x32,0x35,0x31,0x39,0x31,0x38,0x35,0x34,0x5a,
0x30,0x21,0x02,0x10,0x41,0xa8,0xd7,0x9c,0x10,0x5e,0x5a,0xac,0x16,0x7f,0x93,
0xaa,0xd1,0x83,0x34,0x55,0x17,0x0d,0x30,0x32,0x30,0x34,0x31,0x30,0x31,0x32,
0x35,0x33,0x34,0x30,0x5a,0x30,0x21,0x02,0x10,0x42,0x88,0x96,0xb0,0x7b,0x28,
0xa2,0xfa,0x2f,0x91,0x73,0x58,0xa7,0x1e,0x53,0x7c,0x17,0x0d,0x30,0x33,0x30,
0x33,0x30,0x31,0x30,0x39,0x34,0x33,0x33,0x31,0x5a,0x30,0x21,0x02,0x10,0x42,
0x93,0x2f,0xd2,0x54,0xd3,0x94,0xd0,0x41,0x6a,0x2e,0x33,0x8b,0x81,0xb4,0x3c,
0x17,0x0d,0x30,0x32,0x30,0x38,0x30,0x38,0x30,0x30,0x34,0x38,0x34,0x36,0x5a,
0x30,0x21,0x02,0x10,0x44,0x24,0xdd,0xba,0x85,0xfd,0x3e,0xb2,0xb8,0x17,0x74,
0xfd,0x9d,0x5c,0x0c,0xbd,0x17,0x0d,0x30,0x32,0x30,0x39,0x32,0x31,0x31,0x36,
0x30,0x39,0x31,0x32,0x5a,0x30,0x21,0x02,0x10,0x45,0x02,0x18,0x7d,0x39,0x9c,
0xb9,0x14,0xfb,0x10,0x37,0x96,0xf4,0xc1,0xdd,0x2f,0x17,0x0d,0x30,0x32,0x30,
0x32,0x31,0x31,0x31,0x31,0x31,0x31,0x30,0x36,0x5a,0x30,0x21,0x02,0x10,0x45,
0x16,0xbc,0x31,0x0b,0x4e,0x87,0x0a,0xcc,0xe3,0xd5,0x14,0x16,0x33,0x11,0x83,
0x17,0x0d,0x30,0x32,0x30,0x34,0x30,0x32,0x30,0x32,0x32,0x30,0x31,0x37,0x5a,
0x30,0x21,0x02,0x10,0x46,0x16,0x36,0xde,0x3f,0xef,0x8c,0xfa,0x67,0x53,0x12,
0xcc,0x76,0x63,0xd6,0xdd,0x17,0x0d,0x30,0x32,0x30,0x32,0x31,0x34,0x31,0x36,
0x35,0x39,0x34,0x33,0x5a,0x30,0x21,0x02,0x10,0x46,0x5f,0x85,0xa3,0xa4,0x98,
0x3c,0x40,0x63,0xf6,0x1c,0xf7,0xc2,0xbe,0xfd,0x0e,0x17,0x0d,0x30,0x32,0x30,
0x34,0x30,0x39,0x31,0x35,0x33,0x30,0x30,0x35,0x5a,0x30,0x21,0x02,0x10,0x47,
0x20,0xc2,0xd8,0x85,0x85,0x54,0x39,0xcd,0xf2,0x10,0xf0,0xa7,0x88,0x52,0x75,
0x17,0x0d,0x30,0x32,0x30,0x39,0x31,0x30,0x32,0x32,0x32,0x35,0x32,0x37,0x5a,
0x30,0x21,0x02,0x10,0x47,0x42,0x6e,0xa2,0xab,0xc5,0x33,0x5d,0x50,0x44,0x0b,
0x88,0x97,0x84,0x59,0x4c,0x17,0x0d,0x30,0x32,0x30,0x33,0x30,0x35,0x31,0x34,
0x30,0x35,0x31,0x39,0x5a,0x30,0x21,0x02,0x10,0x49,0x20,0x3f,0xa8,0x6e,0x81,
0xc8,0x3b,0x26,0x05,0xf4,0xa7,0x9b,0x5a,0x81,0x60,0x17,0x0d,0x30,0x32,0x30,
0x37,0x31,0x31,0x31,0x37,0x35,0x30,0x34,0x38,0x5a,0x30,0x21,0x02,0x10,0x49,
0x8b,0x6f,0x05,0xfb,0xcb,0xf4,0x5a,0xaf,0x09,0x47,0xb1,0x04,0xc5,0xe3,0x51,
0x17,0x0d,0x30,0x32,0x30,0x34,0x31,0x32,0x31,0x37,0x34,0x38,0x30,0x38,0x5a,
0x30,0x21,0x02,0x10,0x49,0xb2,0xc3,0x7a,0xbf,0x75,0x2a,0xb3,0x13,0xae,0x53,
0xc6,0xcb,0x45,0x5a,0x3e,0x17,0x0d,0x30,0x32,0x31,0x31,0x31,0x35,0x32,0x31,
0x33,0x35,0x33,0x37,0x5a,0x30,0x21,0x02,0x10,0x4b,0xca,0xc3,0xab,0x0a,0xc5,
0xcd,0x90,0xa2,0xbe,0x43,0xfe,0xdd,0x06,0xe1,0x45,0x17,0x0d,0x30,0x32,0x30,
0x37,0x32,0x30,0x31,0x37,0x33,0x32,0x31,0x32,0x5a,0x30,0x21,0x02,0x10,0x4c,
0x00,0xcc,0x73,0xd5,0x74,0x61,0x62,0x92,0x52,0xff,0xde,0x5b,0xc1,0x55,0xbd,
0x17,0x0d,0x30,0x32,0x30,0x38,0x32,0x36,0x31,0x34,0x30,0x31,0x35,0x31,0x5a,
0x30,0x21,0x02,0x10,0x4c,0x59,0xc1,0xc3,0x56,0x40,0x27,0xd4,0x22,0x0e,0x37,
0xf6,0x5f,0x26,0x50,0xc5,0x17,0x0d,0x30,0x32,0x30,0x32,0x32,0x36,0x30,0x39,
0x35,0x37,0x34,0x34,0x5a,0x30,0x21,0x02,0x10,0x4c,0xca,0x12,0x59,0x46,0xf9,
0x2b,0xc6,0x7d,0x33,0x78,0x40,0x2c,0x3b,0x7a,0x0c,0x17,0x0d,0x30,0x32,0x30,
0x35,0x33,0x30,0x32,0x30,0x32,0x34,0x35,0x38,0x5a,0x30,0x21,0x02,0x10,0x4d,
0x57,0x51,0x35,0x9b,0xe5,0x41,0x2c,0x69,0x66,0xc7,0x21,0xec,0xc6,0x29,0x32,
0x17,0x0d,0x30,0x32,0x30,0x39,0x32,0x36,0x30,0x34,0x33,0x35,0x35,0x36,0x5a,
0x30,0x21,0x02,0x10,0x4e,0x85,0xab,0x9e,0x17,0x54,0xe7,0x42,0x0f,0x8c,0xa1,
0x65,0x96,0x88,0x53,0x54,0x17,0x0d,0x30,0x32,0x30,0x33,0x32,0x38,0x30,0x30,
0x31,0x38,0x35,0x33,0x5a,0x30,0x21,0x02,0x10,0x50,0x3d,0xed,0xac,0x21,0x86,
0x66,0x5d,0xa5,0x1a,0x13,0xee,0xfc,0xa7,0x0b,0xc6,0x17,0x0d,0x30,0x32,0x30,
0x32,0x31,0x38,0x31,0x33,0x35,0x35,0x34,0x39,0x5a,0x30,0x21,0x02,0x10,0x50,
0xa3,0x81,0x9c,0xcb,0x22,0xe4,0x0f,0x80,0xcb,0x7a,0xec,0x35,0xf8,0x73,0x82,
0x17,0x0d,0x30,0x32,0x31,0x30,0x30,0x35,0x31,0x36,0x35,0x39,0x35,0x39,0x5a,
0x30,0x21,0x02,0x10,0x51,0x28,0x73,0x26,0x17,0xcf,0x10,0x6e,0xeb,0x4a,0x03,
0x74,0xa3,0x35,0xe5,0x60,0x17,0x0d,0x30,0x33,0x30,0x36,0x31,0x33,0x31,0x30,
0x30,0x39,0x32,0x39,0x5a,0x30,0x21,0x02,0x10,0x51,0x52,0xff,0xdc,0x69,0x6b,
0x1f,0x1f,0xff,0x7c,0xb1,0x7f,0x03,0x90,0xa9,0x6b,0x17,0x0d,0x30,0x32,0x30,
0x36,0x31,0x34,0x31,0x36,0x30,0x34,0x30,0x32,0x5a,0x30,0x21,0x02,0x10,0x52,
0xd9,0x53,0x69,0x9f,0xec,0xab,0xdd,0x5d,0x2a,0x2f,0xaa,0x57,0x86,0xb9,0x1f,
0x17,0x0d,0x30,0x32,0x30,0x38,0x33,0x30,0x32,0x33,0x34,0x36,0x34,0x33,0x5a,
0x30,0x21,0x02,0x10,0x54,0x46,0xa8,0x8f,0x69,0x2e,0x02,0xf4,0xb4,0xb2,0x69,
0xda,0xbd,0x40,0x02,0xe0,0x17,0x0d,0x30,0x32,0x30,0x33,0x32,0x36,0x30,0x31,
0x35,0x36,0x35,0x38,0x5a,0x30,0x21,0x02,0x10,0x54,0xb5,0x81,0x73,0xb5,0x7c,
0x6d,0xba,0x5c,0x99,0x0d,0xff,0x0a,0x4d,0xee,0xef,0x17,0x0d,0x30,0x32,0x30,
0x37,0x32,0x34,0x31,0x36,0x33,0x39,0x35,0x31,0x5a,0x30,0x21,0x02,0x10,0x57,
0x91,0x41,0x20,0x9f,0x57,0x6f,0x42,0x53,0x4e,0x19,0xcc,0xe4,0xc8,0x52,0x4a,
0x17,0x0d,0x30,0x32,0x30,0x35,0x32,0x38,0x32,0x33,0x32,0x34,0x30,0x30,0x5a,
0x30,0x21,0x02,0x10,0x57,0xc6,0xdc,0xa0,0xed,0xbf,0x77,0xdd,0x7e,0x18,0x68,
0x83,0x57,0x0c,0x2a,0x4f,0x17,0x0d,0x30,0x32,0x30,0x35,0x32,0x31,0x31,0x34,
0x30,0x36,0x31,0x31,0x5a,0x30,0x21,0x02,0x10,0x57,0xed,0xe2,0x5b,0xe2,0x62,
0x3f,0x98,0xe1,0xf5,0x4d,0x30,0xa4,0x0e,0xdf,0xdf,0x17,0x0d,0x30,0x32,0x30,
0x36,0x30,0x39,0x30,0x31,0x34,0x37,0x31,0x38,0x5a,0x30,0x21,0x02,0x10,0x58,
0x47,0xd9,0xbd,0x83,0x1a,0x63,0x6f,0xb7,0x63,0x7f,0x4a,0x56,0x5e,0x8e,0x4d,
0x17,0x0d,0x30,0x32,0x30,0x34,0x31,0x35,0x31,0x37,0x32,0x33,0x30,0x33,0x5a,
0x30,0x21,0x02,0x10,0x58,0xc6,0x62,0x99,0x80,0xe6,0x0c,0x4f,0x00,0x8b,0x25,
0x38,0x93,0xe6,0x18,0x10,0x17,0x0d,0x30,0x32,0x30,0x36,0x30,0x36,0x30,0x37,
0x30,0x39,0x34,0x37,0x5a,0x30,0x21,0x02,0x10,0x59,0x52,0x09,0x0e,0x99,0xf3,
0xa9,0xe5,0x2f,0xed,0xa9,0xb2,0xd8,0x61,0xe7,0xea,0x17,0x0d,0x30,0x32,0x30,
0x36,0x32,0x36,0x31,0x34,0x31,0x38,0x33,0x36,0x5a,0x30,0x21,0x02,0x10,0x59,
0x5c,0xaa,0xfb,0xbe,0xfb,0x73,0xd1,0xf4,0xab,0xc8,0xe3,0x3d,0x01,0x04,0xdd,
0x17,0x0d,0x30,0x32,0x30,0x39,0x32,0x37,0x32,0x32,0x32,0x30,0x31,0x30,0x5a,
0x30,0x21,0x02,0x10,0x59,0x97,0x59,0xa7,0x3d,0xb0,0xd9,0x7e,0xff,0x2a,0xcb,
0x31,0xcc,0x66,0xf3,0x85,0x17,0x0d,0x30,0x32,0x30,0x38,0x32,0x32,0x30,0x30,
0x35,0x35,0x35,0x38,0x5a,0x30,0x21,0x02,0x10,0x59,0xdd,0x45,0x36,0x61,0xd9,
0x3e,0xe9,0xff,0xbd,0xad,0x2e,0xbf,0x9a,0x5d,0x98,0x17,0x0d,0x30,0x32,0x30,
0x37,0x30,0x32,0x32,0x30,0x34,0x30,0x30,0x33,0x5a,0x30,0x21,0x02,0x10,0x5a,
0x4b,0x48,0x18,0xa9,0x2a,0x9c,0xd5,0x91,0x2f,0x4f,0xa4,0xf8,0xb3,0x1b,0x4d,
0x17,0x0d,0x30,0x32,0x30,0x34,0x30,0x34,0x32,0x33,0x33,0x33,0x31,0x32,0x5a,
0x30,0x21,0x02,0x10,0x5a,0xdf,0x32,0x0d,0x64,0xeb,0x9b,0xd2,0x11,0xe2,0x58,
0x50,0xbe,0x93,0x0c,0x65,0x17,0x0d,0x30,0x32,0x30,0x34,0x30,0x35,0x31,0x37,
0x30,0x37,0x32,0x31,0x5a,0x30,0x21,0x02,0x10,0x5b,0x23,0xbf,0xbb,0xc4,0xb3,
0xf4,0x02,0xe9,0xcb,0x10,0x9e,0xee,0xa5,0x3f,0xcd,0x17,0x0d,0x30,0x32,0x30,
0x33,0x32,0x39,0x31,0x36,0x32,0x36,0x35,0x39,0x5a,0x30,0x21,0x02,0x10,0x5b,
0x51,0xbc,0x38,0xbf,0xaf,0x9f,0x27,0xa9,0xc7,0xed,0x25,0xd0,0x8d,0xec,0x2e,
0x17,0x0d,0x30,0x32,0x30,0x33,0x30,0x38,0x31,0x30,0x32,0x35,0x32,0x30,0x5a,
0x30,0x21,0x02,0x10,0x5c,0x29,0x7f,0x46,0x61,0xdd,0x47,0x90,0x82,0x91,0xbd,
0x79,0x22,0x6a,0x98,0x38,0x17,0x0d,0x30,0x32,0x31,0x31,0x30,0x38,0x31,0x35,
0x35,0x34,0x32,0x36,0x5a,0x30,0x21,0x02,0x10,0x5e,0x38,0xf7,0x5b,0x00,0xf1,
0xef,0x1c,0xb6,0xff,0xd5,0x5c,0x74,0xfb,0x95,0x5d,0x17,0x0d,0x30,0x32,0x31,
0x31,0x32,0x33,0x30,0x31,0x34,0x39,0x32,0x39,0x5a,0x30,0x21,0x02,0x10,0x5e,
0x88,0xbe,0xb6,0xb4,0xb2,0xaa,0xb0,0x92,0xf3,0xf6,0xc2,0xbc,0x72,0x21,0xca,
0x17,0x0d,0x30,0x32,0x30,0x32,0x31,0x34,0x30,0x37,0x31,0x32,0x31,0x30,0x5a,
0x30,0x21,0x02,0x10,0x5f,0x59,0xa0,0xbb,0xaf,0x26,0xc8,0xc1,0xb4,0x04,0x3a,
0xbb,0xfc,0x4c,0x75,0xa5,0x17,0x0d,0x30,0x32,0x30,0x34,0x31,0x36,0x31,0x35,
0x35,0x31,0x32,0x33,0x5a,0x30,0x21,0x02,0x10,0x5f,0x81,0x08,0x0f,0xa0,0xcd,
0x44,0x73,0x23,0x58,0x8e,0x49,0x9f,0xb5,0x08,0x35,0x17,0x0d,0x30,0x32,0x30,
0x36,0x31,0x39,0x31,0x34,0x31,0x37,0x34,0x33,0x5a,0x30,0x21,0x02,0x10,0x5f,
0xba,0x1f,0x8f,0xb2,0x23,0x56,0xdd,0xbc,0xa6,0x72,0xb0,0x99,0x13,0xb5,0xb2,
0x17,0x0d,0x30,0x32,0x30,0x35,0x30,0x36,0x30,0x38,0x34,0x37,0x31,0x30,0x5a,
0x30,0x21,0x02,0x10,0x60,0x09,0xd5,0xb7,0x6b,0xf1,0x16,0x4a,0xfa,0xd0,0xa5,
0x4c,0x8e,0xdd,0x02,0xcb,0x17,0x0d,0x30,0x32,0x30,0x36,0x31,0x37,0x31,0x36,
0x31,0x32,0x32,0x39,0x5a,0x30,0x21,0x02,0x10,0x60,0x1d,0x19,0xd8,0x55,0xd5,
0x14,0xd5,0xff,0x03,0x0d,0xad,0x5c,0x07,0x4c,0xe7,0x17,0x0d,0x30,0x32,0x30,
0x37,0x31,0x35,0x32,0x33,0x30,0x31,0x31,0x31,0x5a,0x30,0x21,0x02,0x10,0x60,
0x24,0x67,0xc3,0x0b,0xad,0x53,0x8f,0xce,0x89,0x05,0xb5,0x87,0xaf,0x7c,0xe4,
0x17,0x0d,0x30,0x32,0x31,0x30,0x30,0x38,0x32,0x30,0x33,0x38,0x35,0x32,0x5a,
0x30,0x21,0x02,0x10,0x60,0x5c,0xf3,0x3d,0x22,0x23,0x39,0x3f,0xe6,0x21,0x09,
0xfd,0xdd,0x77,0xc2,0x8f,0x17,0x0d,0x30,0x32,0x30,0x37,0x30,0x32,0x31,0x37,
0x32,0x37,0x35,0x38,0x5a,0x30,0x21,0x02,0x10,0x60,0xa2,0x5e,0xbf,0x07,0x83,
0xa3,0x18,0x56,0x18,0x48,0x63,0xa7,0xfd,0xc7,0x63,0x17,0x0d,0x30,0x32,0x30,
0x35,0x30,0x39,0x31,0x39,0x35,0x32,0x32,0x37,0x5a,0x30,0x21,0x02,0x10,0x60,
0xc2,0xad,0xa8,0x0e,0xf9,0x9a,0x66,0x5d,0xa2,0x75,0x04,0x5e,0x5c,0x71,0xc2,
0x17,0x0d,0x30,0x32,0x31,0x31,0x31,0x32,0x31,0x33,0x33,0x36,0x31,0x37,0x5a,
0x30,0x21,0x02,0x10,0x60,0xdb,0x1d,0x37,0x34,0xf6,0x02,0x9d,0x68,0x1b,0x70,
0xf1,0x13,0x00,0x2f,0x80,0x17,0x0d,0x30,0x32,0x30,0x32,0x32,0x38,0x30,0x39,
0x35,0x35,0x33,0x33,0x5a,0x30,0x21,0x02,0x10,0x61,0xf0,0x38,0xea,0xbc,0x17,
0x0d,0x11,0xd2,0x89,0xee,0x87,0x50,0x57,0xa0,0xed,0x17,0x0d,0x30,0x33,0x30,
0x31,0x32,0x39,0x31,0x37,0x34,0x31,0x34,0x34,0x5a,0x30,0x21,0x02,0x10,0x61,
0xfa,0x9b,0xeb,0x58,0xf9,0xe5,0xa5,0x9e,0x79,0xa8,0x3d,0x79,0xac,0x35,0x97,
0x17,0x0d,0x30,0x32,0x31,0x30,0x31,0x30,0x32,0x30,0x31,0x36,0x33,0x37,0x5a,
0x30,0x21,0x02,0x10,0x62,0x44,0x57,0x24,0x41,0xc0,0x89,0x3f,0x5b,0xd2,0xbd,
0xe7,0x2f,0x75,0x41,0xfa,0x17,0x0d,0x30,0x32,0x30,0x38,0x30,0x38,0x31,0x38,
0x33,0x30,0x31,0x35,0x5a,0x30,0x21,0x02,0x10,0x62,0x51,0x3a,0x2d,0x8d,0x82,
0x39,0x65,0xfe,0xf6,0x8a,0xc8,0x4e,0x29,0x91,0xfd,0x17,0x0d,0x30,0x32,0x30,
0x39,0x32,0x36,0x30,0x30,0x35,0x34,0x33,0x34,0x5a,0x30,0x21,0x02,0x10,0x62,
0x52,0x49,0x49,0xf2,0x51,0x67,0x7a,0xe2,0xee,0xc9,0x0c,0x23,0x11,0x3d,0xb2,
0x17,0x0d,0x30,0x32,0x30,0x34,0x31,0x37,0x31,0x38,0x30,0x36,0x35,0x35,0x5a,
0x30,0x21,0x02,0x10,0x63,0x52,0xbd,0xdc,0xb7,0xbf,0xbb,0x90,0x6c,0x82,0xee,
0xb5,0xa3,0x9f,0xd8,0xc9,0x17,0x0d,0x30,0x32,0x30,0x32,0x32,0x31,0x31,0x36,
0x33,0x30,0x35,0x38,0x5a,0x30,0x21,0x02,0x10,0x63,0x5e,0x6b,0xe9,0xea,0x3d,
0xd6,0x3b,0xc3,0x4d,0x09,0xc3,0x13,0xdb,0xdd,0xbc,0x17,0x0d,0x30,0x33,0x30,
0x36,0x30,0x32,0x31,0x34,0x34,0x37,0x33,0x36,0x5a,0x30,0x21,0x02,0x10,0x63,
0xda,0x0b,0xd5,0x13,0x1e,0x98,0x83,0x32,0xa2,0x3a,0x4b,0xdf,0x8c,0x89,0x86,
0x17,0x0d,0x30,0x32,0x30,0x39,0x32,0x35,0x30,0x38,0x30,0x38,0x31,0x33,0x5a,
0x30,0x21,0x02,0x10,0x64,0xfe,0xf0,0x1a,0x3a,0xed,0x89,0xf8,0xb5,0x34,0xd3,
0x1e,0x0f,0xce,0x0d,0xce,0x17,0x0d,0x30,0x32,0x30,0x34,0x30,0x38,0x32,0x31,
0x30,0x36,0x32,0x34,0x5a,0x30,0x21,0x02,0x10,0x65,0xa7,0x49,0xd8,0x37,0x22,
0x4b,0x4a,0xe5,0xcf,0xa3,0xfe,0xd6,0x3b,0xc0,0x67,0x17,0x0d,0x30,0x32,0x31,
0x32,0x30,0x34,0x31,0x37,0x31,0x34,0x31,0x36,0x5a,0x30,0x21,0x02,0x10,0x65,
0xc9,0x9e,0x47,0x76,0x98,0x0d,0x9e,0x57,0xe4,0xae,0xc5,0x1c,0x3e,0xf2,0xe7,
0x17,0x0d,0x30,0x32,0x30,0x39,0x32,0x33,0x31,0x34,0x30,0x38,0x31,0x38,0x5a,
0x30,0x21,0x02,0x10,0x65,0xe0,0x7b,0xc5,0x74,0xe4,0xab,0x01,0x4f,0xa3,0x5e,
0xd6,0xeb,0xcd,0xd5,0x69,0x17,0x0d,0x30,0x32,0x30,0x34,0x30,0x33,0x31,0x37,
0x32,0x34,0x30,0x36,0x5a,0x30,0x21,0x02,0x10,0x66,0x51,0xb7,0xe5,0x62,0xb7,
0xe3,0x31,0xc0,0xee,0xf2,0xe8,0xfe,0x84,0x6a,0x4e,0x17,0x0d,0x30,0x32,0x30,
0x39,0x30,0x36,0x31,0x33,0x32,0x33,0x33,0x33,0x5a,0x30,0x21,0x02,0x10,0x67,
0x7c,0x76,0xac,0x66,0x5a,0x6b,0x41,0x5c,0x07,0x83,0x02,0xd6,0xd9,0x63,0xc0,
0x17,0x0d,0x30,0x32,0x30,0x32,0x31,0x38,0x31,0x33,0x35,0x35,0x31,0x30,0x5a,
0x30,0x21,0x02,0x10,0x68,0x67,0xde,0xb3,0xaa,0x20,0xcf,0x4b,0x34,0xa5,0xe0,
0xc8,0xc0,0xc5,0xc9,0xa4,0x17,0x0d,0x30,0x32,0x30,0x33,0x31,0x32,0x30,0x31,
0x30,0x39,0x32,0x36,0x5a,0x30,0x21,0x02,0x10,0x69,0x23,0x34,0x5d,0x75,0x04,
0xdc,0x99,0xbd,0xce,0x8d,0x21,0xb4,0x6b,0x10,0xfc,0x17,0x0d,0x30,0x32,0x30,
0x39,0x30,0x33,0x31,0x33,0x31,0x39,0x32,0x30,0x5a,0x30,0x21,0x02,0x10,0x69,
0x9f,0x20,0x31,0xd1,0x3f,0xfa,0x1e,0x70,0x2e,0x37,0xd5,0x9a,0x8c,0x0a,0x16,
0x17,0x0d,0x30,0x32,0x30,0x32,0x32,0x30,0x30,0x39,0x30,0x31,0x33,0x35,0x5a,
0x30,0x21,0x02,0x10,0x6a,0x94,0xd6,0x25,0xd0,0x67,0xe4,0x4d,0x79,0x2b,0xc6,
0xd5,0xc9,0x4a,0x7f,0xc6,0x17,0x0d,0x30,0x32,0x30,0x32,0x31,0x31,0x31,0x39,
0x31,0x35,0x34,0x30,0x5a,0x30,0x21,0x02,0x10,0x6b,0x5c,0xa4,0x45,0x5b,0xe9,
0xcf,0xe7,0x3b,0x29,0xb1,0x32,0xd7,0xa1,0x04,0x3d,0x17,0x0d,0x30,0x32,0x31,
0x30,0x31,0x38,0x31,0x35,0x34,0x33,0x34,0x38,0x5a,0x30,0x21,0x02,0x10,0x6b,
0xc0,0x7d,0x4f,0x18,0xfe,0xb7,0x07,0xe8,0x56,0x9a,0x6c,0x40,0x0f,0x36,0x53,
0x17,0x0d,0x30,0x32,0x30,0x39,0x32,0x36,0x32,0x31,0x30,0x31,0x32,0x36,0x5a,
0x30,0x21,0x02,0x10,0x6b,0xe1,0xdd,0x36,0x3b,0xec,0xe0,0xa9,0xf5,0x92,0x7e,
0x33,0xbf,0xed,0x48,0x46,0x17,0x0d,0x30,0x32,0x30,0x34,0x31,0x37,0x31,0x34,
0x34,0x32,0x33,0x31,0x5a,0x30,0x21,0x02,0x10,0x6c,0xac,0xeb,0x37,0x2b,0x6a,
0x42,0xe2,0xca,0xc8,0xd2,0xda,0xb8,0xb9,0x82,0x6a,0x17,0x0d,0x30,0x32,0x30,
0x33,0x30,0x31,0x31,0x34,0x32,0x38,0x33,0x34,0x5a,0x30,0x21,0x02,0x10,0x6d,
0x98,0x1b,0xb4,0x76,0xd1,0x62,0x59,0xa1,0x3c,0xee,0xd2,0x21,0xd8,0xdf,0x4c,
0x17,0x0d,0x30,0x32,0x30,0x35,0x31,0x34,0x31,0x37,0x35,0x36,0x31,0x32,0x5a,
0x30,0x21,0x02,0x10,0x6d,0xdd,0x0b,0x5a,0x3c,0x9c,0xab,0xd3,0x3b,0xd9,0x16,
0xec,0x69,0x74,0xfb,0x9a,0x17,0x0d,0x30,0x32,0x30,0x32,0x32,0x32,0x31,0x32,
0x32,0x36,0x33,0x38,0x5a,0x30,0x21,0x02,0x10,0x6e,0xde,0xfd,0x89,0x36,0xae,
0xa0,0x41,0x8d,0x5c,0xec,0x2e,0x90,0x31,0xf8,0x9a,0x17,0x0d,0x30,0x32,0x30,
0x34,0x30,0x38,0x32,0x32,0x33,0x36,0x31,0x32,0x5a,0x30,0x21,0x02,0x10,0x6f,
0xb2,0x6b,0x4c,0x48,0xca,0xfe,0xe6,0x69,0x9a,0x06,0x63,0xc4,0x32,0x96,0xc1,
0x17,0x0d,0x30,0x33,0x30,0x31,0x31,0x37,0x31,0x37,0x32,0x37,0x32,0x35,0x5a,
0x30,0x21,0x02,0x10,0x70,0x0b,0xe1,0xee,0x44,0x89,0x51,0x52,0x65,0x27,0x2c,
0x2d,0x34,0x7c,0xe0,0x8d,0x17,0x0d,0x30,0x32,0x30,0x39,0x31,0x38,0x30,0x30,
0x33,0x36,0x30,0x30,0x5a,0x30,0x21,0x02,0x10,0x70,0x2d,0xc0,0xa6,0xb8,0xa5,
0xa0,0xda,0x48,0x59,0xb3,0x96,0x34,0x80,0xc8,0x25,0x17,0x0d,0x30,0x32,0x30,
0x38,0x33,0x30,0x31,0x34,0x30,0x31,0x30,0x31,0x5a,0x30,0x21,0x02,0x10,0x70,
0xe1,0xd9,0x92,0xcd,0x76,0x42,0x63,0x51,0x6e,0xcd,0x8c,0x09,0x29,0x17,0x48,
0x17,0x0d,0x30,0x32,0x30,0x35,0x31,0x37,0x31,0x31,0x31,0x30,0x34,0x31,0x5a,
0x30,0x21,0x02,0x10,0x72,0x38,0xe4,0x91,0x6a,0x7a,0x8a,0xf3,0xbf,0xf0,0xd8,
0xe0,0xa4,0x70,0x8d,0xa8,0x17,0x0d,0x30,0x32,0x30,0x33,0x30,0x34,0x31,0x39,
0x30,0x36,0x34,0x30,0x5a,0x30,0x21,0x02,0x10,0x72,0x97,0xa1,0xd8,0x9c,0x3b,
0x00,0xc2,0xc4,0x26,0x2d,0x06,0x2b,0x29,0x76,0x4e,0x17,0x0d,0x30,0x32,0x30,
0x36,0x31,0x38,0x31,0x35,0x30,0x39,0x34,0x37,0x5a,0x30,0x21,0x02,0x10,0x72,
0xd2,0x23,0x9b,0xf2,0x33,0xe9,0x7c,0xcf,0xb6,0xa9,0x41,0xd5,0x0e,0x5c,0x39,
0x17,0x0d,0x30,0x33,0x30,0x34,0x30,0x39,0x31,0x37,0x30,0x32,0x32,0x39,0x5a,
0x30,0x21,0x02,0x10,0x74,0x5c,0x9c,0xf9,0xaa,0xc3,0xfa,0x94,0x3c,0x25,0x39,
0x65,0x44,0x95,0x13,0xf1,0x17,0x0d,0x30,0x32,0x30,0x37,0x30,0x39,0x32,0x33,
0x35,0x33,0x32,0x30,0x5a,0x30,0x21,0x02,0x10,0x74,0x98,0x7f,0x68,0xad,0x17,
0x92,0x93,0xf2,0x65,0x94,0x0c,0x33,0xe6,0xbd,0x49,0x17,0x0d,0x30,0x32,0x30,
0x34,0x32,0x33,0x30,0x37,0x34,0x34,0x31,0x38,0x5a,0x30,0x21,0x02,0x10,0x75,
0x0e,0x40,0xff,0x97,0xf0,0x47,0xed,0xf5,0x56,0xc7,0x08,0x4e,0xb1,0xab,0xfd,
0x17,0x0d,0x30,0x31,0x30,0x31,0x33,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,
0x30,0x21,0x02,0x10,0x75,0x26,0x51,0x59,0x65,0xb7,0x33,0x32,0x5f,0xe6,0xcd,
0xaa,0x30,0x65,0x78,0xe0,0x17,0x0d,0x30,0x32,0x30,0x35,0x31,0x36,0x31,0x38,
0x32,0x34,0x35,0x36,0x5a,0x30,0x21,0x02,0x10,0x76,0x13,0x6f,0xbf,0xc8,0xde,
0xd9,0x36,0x30,0x39,0xcc,0x85,0x8f,0x00,0x2f,0x19,0x17,0x0d,0x30,0x32,0x30,
0x33,0x31,0x34,0x30,0x39,0x34,0x38,0x32,0x34,0x5a,0x30,0x21,0x02,0x10,0x76,
0x52,0x78,0x89,0x44,0xfa,0xc1,0xb3,0xd7,0xc9,0x4c,0xb3,0x32,0x95,0xaf,0x03,
0x17,0x0d,0x30,0x32,0x31,0x31,0x31,0x34,0x31,0x39,0x31,0x35,0x34,0x33,0x5a,
0x30,0x21,0x02,0x10,0x77,0x5d,0x4c,0x40,0xd9,0x8d,0xfa,0xc8,0x9a,0x24,0x8d,
0x47,0x10,0x90,0x4a,0x0a,0x17,0x0d,0x30,0x32,0x30,0x35,0x30,0x39,0x30,0x31,
0x31,0x33,0x30,0x32,0x5a,0x30,0x21,0x02,0x10,0x77,0xe6,0x5a,0x43,0x59,0x93,
0x5d,0x5f,0x7a,0x75,0x80,0x1a,0xcd,0xad,0xc2,0x22,0x17,0x0d,0x30,0x30,0x30,
0x38,0x33,0x31,0x31,0x38,0x32,0x32,0x35,0x30,0x5a,0x30,0x21,0x02,0x10,0x78,
0x19,0xf1,0xb6,0x87,0x83,0xaf,0xdf,0x60,0x8d,0x9a,0x64,0x0d,0xec,0xe0,0x51,
0x17,0x0d,0x30,0x32,0x30,0x35,0x32,0x30,0x31,0x37,0x32,0x38,0x31,0x36,0x5a,
0x30,0x21,0x02,0x10,0x78,0x64,0x65,0x8f,0x82,0x79,0xdb,0xa5,0x1c,0x47,0x10,
0x1d,0x72,0x23,0x66,0x52,0x17,0x0d,0x30,0x33,0x30,0x31,0x32,0x34,0x31,0x38,
0x34,0x35,0x34,0x37,0x5a,0x30,0x21,0x02,0x10,0x78,0x64,0xe1,0xc0,0x69,0x8f,
0x3a,0xc7,0x8b,0x23,0xe3,0x29,0xb1,0xee,0xa9,0x41,0x17,0x0d,0x30,0x32,0x30,
0x35,0x30,0x38,0x31,0x37,0x34,0x36,0x32,0x36,0x5a,0x30,0x21,0x02,0x10,0x78,
0x79,0x89,0x61,0x12,0x67,0x64,0x14,0xfd,0x08,0xcc,0xb3,0x05,0x55,0xc0,0x67,
0x17,0x0d,0x30,0x32,0x30,0x34,0x30,0x32,0x31,0x33,0x31,0x38,0x35,0x33,0x5a,
0x30,0x21,0x02,0x10,0x78,0x8a,0x56,0x22,0x08,0xce,0x42,0xee,0xd1,0xa3,0x79,
0x10,0x14,0xfd,0x3a,0x36,0x17,0x0d,0x30,0x33,0x30,0x32,0x30,0x35,0x31,0x36,
0x35,0x33,0x32,0x39,0x5a,0x30,0x21,0x02,0x10,0x7a,0xa0,0x6c,0xba,0x33,0x02,
0xac,0x5f,0xf5,0x0b,0xb6,0x77,0x61,0xef,0x77,0x09,0x17,0x0d,0x30,0x32,0x30,
0x32,0x32,0x38,0x31,0x37,0x35,0x35,0x31,0x31,0x5a,0x30,0x21,0x02,0x10,0x7b,
0x91,0x33,0x66,0x6c,0xf0,0xd4,0xe3,0x9d,0xf6,0x88,0x29,0x9b,0xf7,0xd0,0xea,
0x17,0x0d,0x30,0x32,0x31,0x31,0x32,0x30,0x32,0x32,0x31,0x36,0x34,0x39,0x5a,
0x30,0x21,0x02,0x10,0x7c,0xef,0xf2,0x0a,0x08,0xae,0x10,0x57,0x1e,0xde,0xdc,
0xd6,0x63,0x76,0xb0,0x5d,0x17,0x0d,0x30,0x32,0x30,0x32,0x32,0x36,0x31,0x30,
0x32,0x32,0x33,0x30,0x5a,0x30,0x21,0x02,0x10,0x7f,0x76,0xef,0x69,0xeb,0xf5,
0x3f,0x53,0x2e,0xaa,0xa5,0xed,0xde,0xc0,0xb4,0x06,0x17,0x0d,0x30,0x32,0x30,
0x35,0x30,0x31,0x30,0x33,0x33,0x33,0x30,0x37,0x5a,0x30,0x21,0x02,0x10,0x7f,
0xcb,0x6b,0x99,0x91,0xd0,0x76,0xe1,0x3c,0x0e,0x67,0x15,0xc4,0xd4,0x4d,0x7b,
0x17,0x0d,0x30,0x32,0x30,0x34,0x31,0x30,0x32,0x31,0x31,0x38,0x34,0x30,0x5a,
0x30,0x0d,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x04,0x05,0x00,
0x03,0x81,0x81,0x00,0x5c,0xb9,0xb3,0xbe,0xd3,0xd6,0x73,0xa3,0xfe,0x4a,0xb2,
0x21,0x80,0xea,0xaa,0x05,0x61,0x14,0x1d,0x67,0xb1,0xdf,0xa6,0xf9,0x42,0x08,
0x0d,0x59,0x62,0x9c,0x11,0x5f,0x0e,0x92,0xc5,0xc6,0xae,0x74,0x64,0xc7,0x84,
0x3e,0x64,0x43,0xd2,0xec,0xbb,0xe1,0x9b,0x52,0x74,0x57,0xcf,0x96,0xef,0x68,
0x02,0x7a,0x7b,0x36,0xb7,0xc6,0x9a,0x5f,0xca,0x9c,0x37,0x47,0xc8,0x3a,0x5c,
0x34,0x35,0x3b,0x4b,0xca,0x20,0x77,0x44,0x68,0x07,0x02,0x34,0x46,0xaa,0x0f,
0xd0,0x4d,0xd9,0x47,0xf4,0xb3,0x2d,0xb1,0x44,0xa5,0x69,0xa9,0x85,0x13,0x43,
0xcd,0xcc,0x1d,0x9a,0xe6,0x2d,0xfd,0x9f,0xdc,0x2f,0x83,0xbb,0x8c,0xe2,0x8c,
0x61,0xc0,0x99,0x16,0x71,0x05,0xb6,0x25,0x14,0x64,0x4f,0x30 };

static void test_decodeCRLToBeSigned(DWORD dwEncoding)
{
    static const BYTE *corruptCRLs[] = { v1CRL, v2CRL };
    BOOL ret;
    BYTE *buf = NULL;
    DWORD size = 0, i;

    for (i = 0; i < sizeof(corruptCRLs) / sizeof(corruptCRLs[0]); i++)
    {
        ret = pCryptDecodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED,
         corruptCRLs[i], corruptCRLs[i][1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL,
         &buf, &size);
        ok(!ret && (GetLastError() == CRYPT_E_ASN1_CORRUPT ||
         GetLastError() == OSS_DATA_ERROR /* Win9x */),
         "Expected CRYPT_E_ASN1_CORRUPT or OSS_DATA_ERROR, got %08x\n",
         GetLastError());
    }
    /* at a minimum, a CRL must contain an issuer: */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED,
     v1CRLWithIssuer, v1CRLWithIssuer[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL,
     &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CRL_INFO *info = (CRL_INFO *)buf;

        ok(size >= sizeof(CRL_INFO), "Wrong size %d\n", size);
        ok(info->cCRLEntry == 0, "Expected 0 CRL entries, got %d\n",
         info->cCRLEntry);
        ok(info->Issuer.cbData == sizeof(encodedCommonName),
         "Wrong issuer size %d\n", info->Issuer.cbData);
        ok(!memcmp(info->Issuer.pbData, encodedCommonName, info->Issuer.cbData),
         "Unexpected issuer\n");
        LocalFree(buf);
    }
    /* check decoding with an empty CRL entry */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED,
     v1CRLWithIssuerAndEmptyEntry, v1CRLWithIssuerAndEmptyEntry[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && (GetLastError() == CRYPT_E_ASN1_CORRUPT ||
     GetLastError() == OSS_DATA_ERROR /* Win9x */ ||
     GetLastError() == CRYPT_E_BAD_ENCODE /* Win8 */),
     "Expected CRYPT_E_ASN1_CORRUPT or OSS_DATA_ERROR, got %08x\n",
     GetLastError());
    /* with a real CRL entry */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED,
     v1CRLWithIssuerAndEntry, v1CRLWithIssuerAndEntry[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CRL_INFO *info = (CRL_INFO *)buf;
        CRL_ENTRY *entry;

        ok(size >= sizeof(CRL_INFO), "Wrong size %d\n", size);
        ok(info->cCRLEntry == 1, "Expected 1 CRL entries, got %d\n",
         info->cCRLEntry);
        ok(info->rgCRLEntry != NULL, "Expected a valid CRL entry array\n");
        entry = info->rgCRLEntry;
        ok(entry->SerialNumber.cbData == 1,
         "Expected serial number size 1, got %d\n",
         entry->SerialNumber.cbData);
        ok(*entry->SerialNumber.pbData == *serialNum,
         "Expected serial number %d, got %d\n", *serialNum,
         *entry->SerialNumber.pbData);
        ok(info->Issuer.cbData == sizeof(encodedCommonName),
         "Wrong issuer size %d\n", info->Issuer.cbData);
        ok(!memcmp(info->Issuer.pbData, encodedCommonName, info->Issuer.cbData),
         "Unexpected issuer\n");
        LocalFree(buf);
    }
    /* a real CRL from verisign that has extensions */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED,
     verisignCRL, sizeof(verisignCRL), CRYPT_DECODE_ALLOC_FLAG,
     NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CRL_INFO *info = (CRL_INFO *)buf;

        ok(size >= sizeof(CRL_INFO), "Wrong size %d\n", size);
        ok(info->cCRLEntry == 3, "Expected 3 CRL entries, got %d\n",
         info->cCRLEntry);
        ok(info->rgCRLEntry != NULL, "Expected a valid CRL entry array\n");
        ok(info->cExtension == 2, "Expected 2 extensions, got %d\n",
         info->cExtension);
        LocalFree(buf);
    }
    /* another real CRL from verisign that has lots of entries */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED,
     verisignCRLWithLotsOfEntries, sizeof(verisignCRLWithLotsOfEntries),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CRL_INFO *info = (CRL_INFO *)buf;

        ok(size >= sizeof(CRL_INFO), "Got size %d\n", size);
        ok(info->cCRLEntry == 209, "Expected 209 CRL entries, got %d\n",
         info->cCRLEntry);
        ok(info->cExtension == 0, "Expected 0 extensions, got %d\n",
         info->cExtension);
        LocalFree(buf);
    }
    /* and finally, with an extension */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED,
     v1CRLWithExt, sizeof(v1CRLWithExt), CRYPT_DECODE_ALLOC_FLAG,
     NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CRL_INFO *info = (CRL_INFO *)buf;
        CRL_ENTRY *entry;

        ok(size >= sizeof(CRL_INFO), "Wrong size %d\n", size);
        ok(info->cCRLEntry == 1, "Expected 1 CRL entries, got %d\n",
         info->cCRLEntry);
        ok(info->rgCRLEntry != NULL, "Expected a valid CRL entry array\n");
        entry = info->rgCRLEntry;
        ok(entry->SerialNumber.cbData == 1,
         "Expected serial number size 1, got %d\n",
         entry->SerialNumber.cbData);
        ok(*entry->SerialNumber.pbData == *serialNum,
         "Expected serial number %d, got %d\n", *serialNum,
         *entry->SerialNumber.pbData);
        ok(info->Issuer.cbData == sizeof(encodedCommonName),
         "Wrong issuer size %d\n", info->Issuer.cbData);
        ok(!memcmp(info->Issuer.pbData, encodedCommonName, info->Issuer.cbData),
         "Unexpected issuer\n");
        ok(info->cExtension == 1, "Expected 1 extensions, got %d\n",
         info->cExtension);
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED,
     v2CRLWithExt, sizeof(v2CRLWithExt), CRYPT_DECODE_ALLOC_FLAG,
     NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CRL_INFO *info = (CRL_INFO *)buf;

        ok(info->cExtension == 1, "Expected 1 extensions, got %d\n",
         info->cExtension);
        LocalFree(buf);
    }
}

static const LPCSTR keyUsages[] = { szOID_PKIX_KP_CODE_SIGNING,
 szOID_PKIX_KP_CLIENT_AUTH, szOID_RSA_RSA };
static const BYTE encodedUsage[] = {
 0x30, 0x1f, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x03,
 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x02, 0x06, 0x09,
 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01 };

static void test_encodeEnhancedKeyUsage(DWORD dwEncoding)
{
    BOOL ret;
    BYTE *buf = NULL;
    DWORD size = 0;
    CERT_ENHKEY_USAGE usage;

    /* Test with empty usage */
    usage.cUsageIdentifier = 0;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_ENHANCED_KEY_USAGE, &usage,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptySequence), "Wrong size %d\n", size);
        ok(!memcmp(buf, emptySequence, size), "Got unexpected value\n");
        LocalFree(buf);
    }
    /* Test with a few usages */
    usage.cUsageIdentifier = sizeof(keyUsages) / sizeof(keyUsages[0]);
    usage.rgpszUsageIdentifier = (LPSTR *)keyUsages;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_ENHANCED_KEY_USAGE, &usage,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(encodedUsage), "Wrong size %d\n", size);
        ok(!memcmp(buf, encodedUsage, size), "Got unexpected value\n");
        LocalFree(buf);
    }
}

static void test_decodeEnhancedKeyUsage(DWORD dwEncoding)
{
    BOOL ret;
    LPBYTE buf = NULL;
    DWORD size = 0;

    ret = pCryptDecodeObjectEx(dwEncoding, X509_ENHANCED_KEY_USAGE,
     emptySequence, sizeof(emptySequence), CRYPT_DECODE_ALLOC_FLAG, NULL,
     &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CERT_ENHKEY_USAGE *usage = (CERT_ENHKEY_USAGE *)buf;

        ok(size >= sizeof(CERT_ENHKEY_USAGE),
         "Wrong size %d\n", size);
        ok(usage->cUsageIdentifier == 0, "Expected 0 CRL entries, got %d\n",
         usage->cUsageIdentifier);
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_ENHANCED_KEY_USAGE,
     encodedUsage, sizeof(encodedUsage), CRYPT_DECODE_ALLOC_FLAG, NULL,
     &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CERT_ENHKEY_USAGE *usage = (CERT_ENHKEY_USAGE *)buf;
        DWORD i;

        ok(size >= sizeof(CERT_ENHKEY_USAGE),
         "Wrong size %d\n", size);
        ok(usage->cUsageIdentifier == sizeof(keyUsages) / sizeof(keyUsages[0]),
         "Wrong CRL entries count %d\n", usage->cUsageIdentifier);
        for (i = 0; i < usage->cUsageIdentifier; i++)
            ok(!strcmp(usage->rgpszUsageIdentifier[i], keyUsages[i]),
             "Expected OID %s, got %s\n", keyUsages[i],
             usage->rgpszUsageIdentifier[i]);
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_ENHANCED_KEY_USAGE,
     encodedUsage, sizeof(encodedUsage), 0, NULL, NULL, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    if (buf)
    {
        ret = pCryptDecodeObjectEx(dwEncoding, X509_ENHANCED_KEY_USAGE,
         encodedUsage, sizeof(encodedUsage), 0, NULL, buf, &size);
        ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
        HeapFree(GetProcessHeap(), 0, buf);
    }
}

static BYTE keyId[] = { 1,2,3,4 };
static const BYTE authorityKeyIdWithId[] = {
 0x30,0x06,0x80,0x04,0x01,0x02,0x03,0x04 };
static const BYTE authorityKeyIdWithIssuer[] = { 0x30,0x19,0xa1,0x17,0x30,0x15,
 0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,
 0x20,0x4c,0x61,0x6e,0x67,0x00 };
static const BYTE authorityKeyIdWithSerial[] = { 0x30,0x03,0x82,0x01,0x01 };

static void test_encodeAuthorityKeyId(DWORD dwEncoding)
{
    CERT_AUTHORITY_KEY_ID_INFO info = { { 0 } };
    BOOL ret;
    BYTE *buf = NULL;
    DWORD size = 0;

    /* Test with empty id */
    ret = pCryptEncodeObjectEx(dwEncoding, X509_AUTHORITY_KEY_ID, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptySequence), "Unexpected size %d\n", size);
        ok(!memcmp(buf, emptySequence, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* With just a key id */
    info.KeyId.cbData = sizeof(keyId);
    info.KeyId.pbData = keyId;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_AUTHORITY_KEY_ID, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(authorityKeyIdWithId), "Unexpected size %d\n", size);
        ok(!memcmp(buf, authorityKeyIdWithId, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* With just an issuer */
    info.KeyId.cbData = 0;
    info.CertIssuer.cbData = sizeof(encodedCommonName);
    info.CertIssuer.pbData = (BYTE *)encodedCommonName;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_AUTHORITY_KEY_ID, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(authorityKeyIdWithIssuer), "Unexpected size %d\n",
         size);
        ok(!memcmp(buf, authorityKeyIdWithIssuer, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* With just a serial number */
    info.CertIssuer.cbData = 0;
    info.CertSerialNumber.cbData = sizeof(serialNum);
    info.CertSerialNumber.pbData = (BYTE *)serialNum;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_AUTHORITY_KEY_ID, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(authorityKeyIdWithSerial), "Unexpected size %d\n",
         size);
        ok(!memcmp(buf, authorityKeyIdWithSerial, size), "Unexpected value\n");
        LocalFree(buf);
    }
}

static void test_decodeAuthorityKeyId(DWORD dwEncoding)
{
    BOOL ret;
    LPBYTE buf = NULL;
    DWORD size = 0;

    ret = pCryptDecodeObjectEx(dwEncoding, X509_AUTHORITY_KEY_ID,
     emptySequence, sizeof(emptySequence), CRYPT_DECODE_ALLOC_FLAG, NULL,
     &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CERT_AUTHORITY_KEY_ID_INFO *info = (CERT_AUTHORITY_KEY_ID_INFO *)buf;

        ok(size >= sizeof(CERT_AUTHORITY_KEY_ID_INFO), "Unexpected size %d\n",
         size);
        ok(info->KeyId.cbData == 0, "Expected no key id\n");
        ok(info->CertIssuer.cbData == 0, "Expected no issuer name\n");
        ok(info->CertSerialNumber.cbData == 0, "Expected no serial number\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_AUTHORITY_KEY_ID,
     authorityKeyIdWithId, sizeof(authorityKeyIdWithId),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CERT_AUTHORITY_KEY_ID_INFO *info = (CERT_AUTHORITY_KEY_ID_INFO *)buf;

        ok(size >= sizeof(CERT_AUTHORITY_KEY_ID_INFO), "Unexpected size %d\n",
         size);
        ok(info->KeyId.cbData == sizeof(keyId), "Unexpected key id len\n");
        ok(!memcmp(info->KeyId.pbData, keyId, sizeof(keyId)),
         "Unexpected key id\n");
        ok(info->CertIssuer.cbData == 0, "Expected no issuer name\n");
        ok(info->CertSerialNumber.cbData == 0, "Expected no serial number\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_AUTHORITY_KEY_ID,
     authorityKeyIdWithIssuer, sizeof(authorityKeyIdWithIssuer),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CERT_AUTHORITY_KEY_ID_INFO *info = (CERT_AUTHORITY_KEY_ID_INFO *)buf;

        ok(size >= sizeof(CERT_AUTHORITY_KEY_ID_INFO), "Unexpected size %d\n",
         size);
        ok(info->KeyId.cbData == 0, "Expected no key id\n");
        ok(info->CertIssuer.cbData == sizeof(encodedCommonName),
         "Unexpected issuer len\n");
        ok(!memcmp(info->CertIssuer.pbData, encodedCommonName,
         sizeof(encodedCommonName)), "Unexpected issuer\n");
        ok(info->CertSerialNumber.cbData == 0, "Expected no serial number\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_AUTHORITY_KEY_ID,
     authorityKeyIdWithSerial, sizeof(authorityKeyIdWithSerial),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CERT_AUTHORITY_KEY_ID_INFO *info = (CERT_AUTHORITY_KEY_ID_INFO *)buf;

        ok(size >= sizeof(CERT_AUTHORITY_KEY_ID_INFO), "Unexpected size %d\n",
         size);
        ok(info->KeyId.cbData == 0, "Expected no key id\n");
        ok(info->CertIssuer.cbData == 0, "Expected no issuer name\n");
        ok(info->CertSerialNumber.cbData == sizeof(serialNum),
         "Unexpected serial number len\n");
        ok(!memcmp(info->CertSerialNumber.pbData, serialNum, sizeof(serialNum)),
         "Unexpected serial number\n");
        LocalFree(buf);
    }
}

static const BYTE authorityKeyIdWithIssuerUrl[] = { 0x30,0x15,0xa1,0x13,0x86,
 0x11,0x68,0x74,0x74,0x70,0x3a,0x2f,0x2f,0x77,0x69,0x6e,0x65,0x68,0x71,0x2e,
 0x6f,0x72,0x67 };

static void test_encodeAuthorityKeyId2(DWORD dwEncoding)
{
    CERT_AUTHORITY_KEY_ID2_INFO info = { { 0 } };
    CERT_ALT_NAME_ENTRY entry = { 0 };
    BOOL ret;
    BYTE *buf = NULL;
    DWORD size = 0;

    /* Test with empty id */
    ret = pCryptEncodeObjectEx(dwEncoding, X509_AUTHORITY_KEY_ID2, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptySequence), "Unexpected size %d\n", size);
        ok(!memcmp(buf, emptySequence, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* With just a key id */
    info.KeyId.cbData = sizeof(keyId);
    info.KeyId.pbData = keyId;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_AUTHORITY_KEY_ID2, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(authorityKeyIdWithId), "Unexpected size %d\n",
         size);
        ok(!memcmp(buf, authorityKeyIdWithId, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* With a bogus issuer name */
    info.KeyId.cbData = 0;
    info.AuthorityCertIssuer.cAltEntry = 1;
    info.AuthorityCertIssuer.rgAltEntry = &entry;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_AUTHORITY_KEY_ID2, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* With an issuer name */
    entry.dwAltNameChoice = CERT_ALT_NAME_URL;
    U(entry).pwszURL = (LPWSTR)url;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_AUTHORITY_KEY_ID2, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(authorityKeyIdWithIssuerUrl), "Unexpected size %d\n",
         size);
        ok(!memcmp(buf, authorityKeyIdWithIssuerUrl, size),
         "Unexpected value\n");
        LocalFree(buf);
    }
    /* With just a serial number */
    info.AuthorityCertIssuer.cAltEntry = 0;
    info.AuthorityCertSerialNumber.cbData = sizeof(serialNum);
    info.AuthorityCertSerialNumber.pbData = (BYTE *)serialNum;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_AUTHORITY_KEY_ID2, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(authorityKeyIdWithSerial), "Unexpected size %d\n",
         size);
        ok(!memcmp(buf, authorityKeyIdWithSerial, size), "Unexpected value\n");
        LocalFree(buf);
    }
}

static void test_decodeAuthorityKeyId2(DWORD dwEncoding)
{
    BOOL ret;
    LPBYTE buf = NULL;
    DWORD size = 0;

    ret = pCryptDecodeObjectEx(dwEncoding, X509_AUTHORITY_KEY_ID2,
     emptySequence, sizeof(emptySequence), CRYPT_DECODE_ALLOC_FLAG, NULL,
     &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CERT_AUTHORITY_KEY_ID2_INFO *info = (CERT_AUTHORITY_KEY_ID2_INFO *)buf;

        ok(size >= sizeof(CERT_AUTHORITY_KEY_ID2_INFO), "Unexpected size %d\n",
         size);
        ok(info->KeyId.cbData == 0, "Expected no key id\n");
        ok(info->AuthorityCertIssuer.cAltEntry == 0,
         "Expected no issuer name entries\n");
        ok(info->AuthorityCertSerialNumber.cbData == 0,
         "Expected no serial number\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_AUTHORITY_KEY_ID2,
     authorityKeyIdWithId, sizeof(authorityKeyIdWithId),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CERT_AUTHORITY_KEY_ID2_INFO *info = (CERT_AUTHORITY_KEY_ID2_INFO *)buf;

        ok(size >= sizeof(CERT_AUTHORITY_KEY_ID2_INFO), "Unexpected size %d\n",
         size);
        ok(info->KeyId.cbData == sizeof(keyId), "Unexpected key id len\n");
        ok(!memcmp(info->KeyId.pbData, keyId, sizeof(keyId)),
         "Unexpected key id\n");
        ok(info->AuthorityCertIssuer.cAltEntry == 0,
         "Expected no issuer name entries\n");
        ok(info->AuthorityCertSerialNumber.cbData == 0,
         "Expected no serial number\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_AUTHORITY_KEY_ID2,
     authorityKeyIdWithIssuerUrl, sizeof(authorityKeyIdWithIssuerUrl),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CERT_AUTHORITY_KEY_ID2_INFO *info = (CERT_AUTHORITY_KEY_ID2_INFO *)buf;

        ok(size >= sizeof(CERT_AUTHORITY_KEY_ID2_INFO), "Unexpected size %d\n",
         size);
        ok(info->KeyId.cbData == 0, "Expected no key id\n");
        ok(info->AuthorityCertIssuer.cAltEntry == 1,
         "Expected 1 issuer entry, got %d\n",
         info->AuthorityCertIssuer.cAltEntry);
        ok(info->AuthorityCertIssuer.rgAltEntry[0].dwAltNameChoice ==
         CERT_ALT_NAME_URL, "Expected CERT_ALT_NAME_URL, got %d\n",
         info->AuthorityCertIssuer.rgAltEntry[0].dwAltNameChoice);
        ok(!lstrcmpW(U(info->AuthorityCertIssuer.rgAltEntry[0]).pwszURL,
         url), "Unexpected URL\n");
        ok(info->AuthorityCertSerialNumber.cbData == 0,
         "Expected no serial number\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_AUTHORITY_KEY_ID2,
     authorityKeyIdWithSerial, sizeof(authorityKeyIdWithSerial),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        CERT_AUTHORITY_KEY_ID2_INFO *info = (CERT_AUTHORITY_KEY_ID2_INFO *)buf;

        ok(size >= sizeof(CERT_AUTHORITY_KEY_ID2_INFO), "Unexpected size %d\n",
         size);
        ok(info->KeyId.cbData == 0, "Expected no key id\n");
        ok(info->AuthorityCertIssuer.cAltEntry == 0,
         "Expected no issuer name entries\n");
        ok(info->AuthorityCertSerialNumber.cbData == sizeof(serialNum),
         "Unexpected serial number len\n");
        ok(!memcmp(info->AuthorityCertSerialNumber.pbData, serialNum,
         sizeof(serialNum)), "Unexpected serial number\n");
        LocalFree(buf);
    }
}

static const BYTE authorityInfoAccessWithUrl[] = {
0x30,0x19,0x30,0x17,0x06,0x02,0x2a,0x03,0x86,0x11,0x68,0x74,0x74,0x70,0x3a,
0x2f,0x2f,0x77,0x69,0x6e,0x65,0x68,0x71,0x2e,0x6f,0x72,0x67 };
static const BYTE authorityInfoAccessWithUrlAndIPAddr[] = {
0x30,0x29,0x30,0x17,0x06,0x02,0x2a,0x03,0x86,0x11,0x68,0x74,0x74,0x70,0x3a,
0x2f,0x2f,0x77,0x69,0x6e,0x65,0x68,0x71,0x2e,0x6f,0x72,0x67,0x30,0x0e,0x06,
0x02,0x2d,0x06,0x87,0x08,0x30,0x06,0x87,0x04,0x7f,0x00,0x00,0x01 };

static void test_encodeAuthorityInfoAccess(DWORD dwEncoding)
{
    static char oid1[] = "1.2.3";
    static char oid2[] = "1.5.6";
    BOOL ret;
    BYTE *buf = NULL;
    DWORD size = 0;
    CERT_ACCESS_DESCRIPTION accessDescription[2];
    CERT_AUTHORITY_INFO_ACCESS aia;

    memset(accessDescription, 0, sizeof(accessDescription));
    aia.cAccDescr = 0;
    aia.rgAccDescr = NULL;
    /* Having no access descriptions is allowed */
    ret = pCryptEncodeObjectEx(dwEncoding, X509_AUTHORITY_INFO_ACCESS, &aia,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptySequence), "unexpected size %d\n", size);
        ok(!memcmp(buf, emptySequence, size), "unexpected value\n");
        LocalFree(buf);
        buf = NULL;
    }
    /* It can't have an empty access method */
    aia.cAccDescr = 1;
    aia.rgAccDescr = accessDescription;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_AUTHORITY_INFO_ACCESS, &aia,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && (GetLastError() == E_INVALIDARG ||
     GetLastError() == OSS_LIMITED /* Win9x */),
     "expected E_INVALIDARG or OSS_LIMITED, got %08x\n", GetLastError());
    /* It can't have an empty location */
    accessDescription[0].pszAccessMethod = oid1;
    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(dwEncoding, X509_AUTHORITY_INFO_ACCESS, &aia,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "expected E_INVALIDARG, got %08x\n", GetLastError());
    accessDescription[0].AccessLocation.dwAltNameChoice = CERT_ALT_NAME_URL;
    U(accessDescription[0].AccessLocation).pwszURL = (LPWSTR)url;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_AUTHORITY_INFO_ACCESS, &aia,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(authorityInfoAccessWithUrl), "unexpected size %d\n",
         size);
        ok(!memcmp(buf, authorityInfoAccessWithUrl, size),
         "unexpected value\n");
        LocalFree(buf);
        buf = NULL;
    }
    accessDescription[1].pszAccessMethod = oid2;
    accessDescription[1].AccessLocation.dwAltNameChoice =
     CERT_ALT_NAME_IP_ADDRESS;
    U(accessDescription[1].AccessLocation).IPAddress.cbData =
     sizeof(encodedIPAddr);
    U(accessDescription[1].AccessLocation).IPAddress.pbData =
     (LPBYTE)encodedIPAddr;
    aia.cAccDescr = 2;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_AUTHORITY_INFO_ACCESS, &aia,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(authorityInfoAccessWithUrlAndIPAddr),
         "unexpected size %d\n", size);
        ok(!memcmp(buf, authorityInfoAccessWithUrlAndIPAddr, size),
         "unexpected value\n");
        LocalFree(buf);
        buf = NULL;
    }
}

static void compareAuthorityInfoAccess(LPCSTR header,
 const CERT_AUTHORITY_INFO_ACCESS *expected,
 const CERT_AUTHORITY_INFO_ACCESS *got)
{
    DWORD i;

    ok(expected->cAccDescr == got->cAccDescr,
     "%s: expected %d access descriptions, got %d\n", header,
     expected->cAccDescr, got->cAccDescr);
    for (i = 0; i < expected->cAccDescr; i++)
    {
        ok(!strcmp(expected->rgAccDescr[i].pszAccessMethod,
         got->rgAccDescr[i].pszAccessMethod), "%s[%d]: expected %s, got %s\n",
         header, i, expected->rgAccDescr[i].pszAccessMethod,
         got->rgAccDescr[i].pszAccessMethod);
        compareAltNameEntry(&expected->rgAccDescr[i].AccessLocation,
         &got->rgAccDescr[i].AccessLocation);
    }
}

static void test_decodeAuthorityInfoAccess(DWORD dwEncoding)
{
    static char oid1[] = "1.2.3";
    static char oid2[] = "1.5.6";
    BOOL ret;
    LPBYTE buf = NULL;
    DWORD size = 0;

    ret = pCryptDecodeObjectEx(dwEncoding, X509_AUTHORITY_INFO_ACCESS,
     emptySequence, sizeof(emptySequence), CRYPT_DECODE_ALLOC_FLAG, NULL,
     &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %x\n", GetLastError());
    if (ret)
    {
        CERT_AUTHORITY_INFO_ACCESS aia = { 0, NULL };

        compareAuthorityInfoAccess("empty AIA", &aia,
         (CERT_AUTHORITY_INFO_ACCESS *)buf);
        LocalFree(buf);
        buf = NULL;
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_AUTHORITY_INFO_ACCESS,
     authorityInfoAccessWithUrl, sizeof(authorityInfoAccessWithUrl),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %x\n", GetLastError());
    if (ret)
    {
        CERT_ACCESS_DESCRIPTION accessDescription;
        CERT_AUTHORITY_INFO_ACCESS aia;

        accessDescription.pszAccessMethod = oid1;
        accessDescription.AccessLocation.dwAltNameChoice = CERT_ALT_NAME_URL;
        U(accessDescription.AccessLocation).pwszURL = (LPWSTR)url;
        aia.cAccDescr = 1;
        aia.rgAccDescr = &accessDescription;
        compareAuthorityInfoAccess("AIA with URL", &aia,
         (CERT_AUTHORITY_INFO_ACCESS *)buf);
        LocalFree(buf);
        buf = NULL;
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_AUTHORITY_INFO_ACCESS,
     authorityInfoAccessWithUrlAndIPAddr,
     sizeof(authorityInfoAccessWithUrlAndIPAddr), CRYPT_DECODE_ALLOC_FLAG,
     NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %x\n", GetLastError());
    if (ret)
    {
        CERT_ACCESS_DESCRIPTION accessDescription[2];
        CERT_AUTHORITY_INFO_ACCESS aia;

        accessDescription[0].pszAccessMethod = oid1;
        accessDescription[0].AccessLocation.dwAltNameChoice = CERT_ALT_NAME_URL;
        U(accessDescription[0].AccessLocation).pwszURL = (LPWSTR)url;
        accessDescription[1].pszAccessMethod = oid2;
        accessDescription[1].AccessLocation.dwAltNameChoice =
         CERT_ALT_NAME_IP_ADDRESS;
        U(accessDescription[1].AccessLocation).IPAddress.cbData =
         sizeof(encodedIPAddr);
        U(accessDescription[1].AccessLocation).IPAddress.pbData =
         (LPBYTE)encodedIPAddr;
        aia.cAccDescr = 2;
        aia.rgAccDescr = accessDescription;
        compareAuthorityInfoAccess("AIA with URL and IP addr", &aia,
         (CERT_AUTHORITY_INFO_ACCESS *)buf);
        LocalFree(buf);
        buf = NULL;
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_AUTHORITY_INFO_ACCESS,
     authorityInfoAccessWithUrlAndIPAddr,
     sizeof(authorityInfoAccessWithUrlAndIPAddr), 0, NULL, NULL, &size);
    ok(ret, "CryptDecodeObjectEx failed: %x\n", GetLastError());
    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    if (buf)
    {
        ret = pCryptDecodeObjectEx(dwEncoding, X509_AUTHORITY_INFO_ACCESS,
         authorityInfoAccessWithUrlAndIPAddr,
         sizeof(authorityInfoAccessWithUrlAndIPAddr), 0, NULL, buf, &size);
        ok(ret, "CryptDecodeObjectEx failed: %x\n", GetLastError());
        HeapFree(GetProcessHeap(), 0, buf);
    }
}

static const BYTE emptyCTL[] = {
0x30,0x17,0x30,0x00,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,
0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x02,0x06,0x00 };
static const BYTE emptyCTLWithVersion1[] = {
0x30,0x1a,0x02,0x01,0x01,0x30,0x00,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,
0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x02,0x06,0x00 };
static const BYTE ctlWithUsageIdentifier[] = {
0x30,0x1b,0x30,0x04,0x06,0x02,0x2a,0x03,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,
0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x02,0x06,0x00 };
static const BYTE ctlWithListIdentifier[] = {
0x30,0x1a,0x30,0x00,0x04,0x01,0x01,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,
0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x02,0x06,0x00 };
static const BYTE ctlWithSequenceNumber[] = {
0x30,0x1a,0x30,0x00,0x02,0x01,0x01,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,
0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x02,0x06,0x00 };
static const BYTE ctlWithThisUpdate[] = {
0x30,0x15,0x30,0x00,0x17,0x0d,0x30,0x35,0x30,0x36,0x30,0x36,0x31,0x36,0x31,
0x30,0x30,0x30,0x5a,0x30,0x02,0x06,0x00 };
static const BYTE ctlWithThisAndNextUpdate[] = {
0x30,0x24,0x30,0x00,0x17,0x0d,0x30,0x35,0x30,0x36,0x30,0x36,0x31,0x36,0x31,
0x30,0x30,0x30,0x5a,0x17,0x0d,0x30,0x35,0x30,0x36,0x30,0x36,0x31,0x36,0x31,
0x30,0x30,0x30,0x5a,0x30,0x02,0x06,0x00 };
static const BYTE ctlWithAlgId[] = {
0x30,0x1b,0x30,0x00,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,
0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x06,0x06,0x02,0x2d,0x06,0x05,0x00 };
static const BYTE ctlWithBogusEntry[] = {
0x30,0x29,0x30,0x00,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,
0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x02,0x06,0x00,0x30,0x10,0x30,0x0e,0x04,
0x01,0x01,0x31,0x09,0x30,0x07,0x06,0x02,0x2a,0x03,0x31,0x01,0x01 };
static const BYTE ctlWithOneEntry[] = {
0x30,0x2a,0x30,0x00,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,
0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x02,0x06,0x00,0x30,0x11,0x30,0x0f,0x04,
0x01,0x01,0x31,0x0a,0x30,0x08,0x06,0x02,0x2a,0x03,0x31,0x02,0x30,0x00 };
static const BYTE ctlWithTwoEntries[] = {
0x30,0x41,0x30,0x00,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,
0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x02,0x06,0x00,0x30,0x28,0x30,0x0f,0x04,
0x01,0x01,0x31,0x0a,0x30,0x08,0x06,0x02,0x2a,0x03,0x31,0x02,0x30,0x00,0x30,
0x15,0x04,0x01,0x01,0x31,0x10,0x30,0x0e,0x06,0x02,0x2d,0x06,0x31,0x08,0x30,
0x06,0x87,0x04,0x7f,0x00,0x00,0x01 };

static void test_encodeCTL(DWORD dwEncoding)
{
    static char oid1[] = "1.2.3";
    static char oid2[] = "1.5.6";
    char *pOid1 = oid1;
    BOOL ret;
    BYTE *buf = NULL;
    DWORD size = 0;
    CTL_INFO info;
    SYSTEMTIME thisUpdate = { 2005, 6, 1, 6, 16, 10, 0, 0 };
    CTL_ENTRY ctlEntry[2];
    CRYPT_ATTRIBUTE attr1, attr2;
    CRYPT_ATTR_BLOB value1, value2;

    memset(&info, 0, sizeof(info));
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_CTL, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptyCTL), "unexpected size %d\n", size);
        ok(!memcmp(buf, emptyCTL, size), "unexpected value\n");
        LocalFree(buf);
        buf = NULL;
    }
    info.dwVersion = 1;
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_CTL, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptyCTLWithVersion1), "unexpected size %d\n", size);
        ok(!memcmp(buf, emptyCTLWithVersion1, size), "unexpected value\n");
        LocalFree(buf);
        buf = NULL;
    }
    info.dwVersion = 0;
    info.SubjectUsage.cUsageIdentifier = 1;
    info.SubjectUsage.rgpszUsageIdentifier = &pOid1;
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_CTL, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(ctlWithUsageIdentifier), "unexpected size %d\n",
         size);
        ok(!memcmp(buf, ctlWithUsageIdentifier, size), "unexpected value\n");
        LocalFree(buf);
        buf = NULL;
    }
    info.SubjectUsage.cUsageIdentifier = 0;
    info.ListIdentifier.cbData = sizeof(serialNum);
    info.ListIdentifier.pbData = (LPBYTE)serialNum;
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_CTL, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(ctlWithListIdentifier), "unexpected size %d\n", size);
        ok(!memcmp(buf, ctlWithListIdentifier, size), "unexpected value\n");
        LocalFree(buf);
        buf = NULL;
    }
    info.ListIdentifier.cbData = 0;
    info.SequenceNumber.cbData = sizeof(serialNum);
    info.SequenceNumber.pbData = (LPBYTE)serialNum;
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_CTL, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(ctlWithSequenceNumber), "unexpected size %d\n",
         size);
        ok(!memcmp(buf, ctlWithSequenceNumber, size), "unexpected value\n");
        LocalFree(buf);
        buf = NULL;
    }
    info.SequenceNumber.cbData = 0;
    SystemTimeToFileTime(&thisUpdate, &info.ThisUpdate);
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_CTL, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(ctlWithThisUpdate), "unexpected size %d\n", size);
        ok(!memcmp(buf, ctlWithThisUpdate, size), "unexpected value\n");
        LocalFree(buf);
        buf = NULL;
    }
    SystemTimeToFileTime(&thisUpdate, &info.NextUpdate);
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_CTL, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(ctlWithThisAndNextUpdate), "unexpected size %d\n",
         size);
        ok(!memcmp(buf, ctlWithThisAndNextUpdate, size), "unexpected value\n");
        LocalFree(buf);
        buf = NULL;
    }
    info.ThisUpdate.dwLowDateTime = info.ThisUpdate.dwHighDateTime = 0;
    info.NextUpdate.dwLowDateTime = info.NextUpdate.dwHighDateTime = 0;
    info.SubjectAlgorithm.pszObjId = oid2;
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_CTL, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(ctlWithAlgId), "unexpected size %d\n", size);
        ok(!memcmp(buf, ctlWithAlgId, size), "unexpected value\n");
        LocalFree(buf);
        buf = NULL;
    }
    /* The value is supposed to be asn.1 encoded, so this'll fail to decode
     * (see tests below) but it'll encode fine.
     */
    info.SubjectAlgorithm.pszObjId = NULL;
    value1.cbData = sizeof(serialNum);
    value1.pbData = (LPBYTE)serialNum;
    attr1.pszObjId = oid1;
    attr1.cValue = 1;
    attr1.rgValue = &value1;
    ctlEntry[0].SubjectIdentifier.cbData = sizeof(serialNum);
    ctlEntry[0].SubjectIdentifier.pbData = (LPBYTE)serialNum;
    ctlEntry[0].cAttribute = 1;
    ctlEntry[0].rgAttribute = &attr1;
    info.cCTLEntry = 1;
    info.rgCTLEntry = ctlEntry;
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_CTL, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(ctlWithBogusEntry), "unexpected size %d\n", size);
        ok(!memcmp(buf, ctlWithBogusEntry, size), "unexpected value\n");
        LocalFree(buf);
        buf = NULL;
    }
    value1.cbData = sizeof(emptySequence);
    value1.pbData = (LPBYTE)emptySequence;
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_CTL, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(ctlWithOneEntry), "unexpected size %d\n", size);
        ok(!memcmp(buf, ctlWithOneEntry, size), "unexpected value\n");
        LocalFree(buf);
        buf = NULL;
    }
    value2.cbData = sizeof(encodedIPAddr);
    value2.pbData = (LPBYTE)encodedIPAddr;
    attr2.pszObjId = oid2;
    attr2.cValue = 1;
    attr2.rgValue = &value2;
    ctlEntry[1].SubjectIdentifier.cbData = sizeof(serialNum);
    ctlEntry[1].SubjectIdentifier.pbData = (LPBYTE)serialNum;
    ctlEntry[1].cAttribute = 1;
    ctlEntry[1].rgAttribute = &attr2;
    info.cCTLEntry = 2;
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_CTL, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(ctlWithTwoEntries), "unexpected size %d\n", size);
        ok(!memcmp(buf, ctlWithTwoEntries, size), "unexpected value\n");
        LocalFree(buf);
        buf = NULL;
    }
}

static void compareCTLInfo(LPCSTR header, const CTL_INFO *expected,
 const CTL_INFO *got)
{
    DWORD i, j, k;

    ok(expected->dwVersion == got->dwVersion,
     "%s: expected version %d, got %d\n", header, expected->dwVersion,
     got->dwVersion);
    ok(expected->SubjectUsage.cUsageIdentifier ==
     got->SubjectUsage.cUsageIdentifier,
     "%s: expected %d usage identifiers, got %d\n", header,
     expected->SubjectUsage.cUsageIdentifier,
     got->SubjectUsage.cUsageIdentifier);
    for (i = 0; i < expected->SubjectUsage.cUsageIdentifier; i++)
        ok(!strcmp(expected->SubjectUsage.rgpszUsageIdentifier[i],
         got->SubjectUsage.rgpszUsageIdentifier[i]),
         "%s[%d]: expected %s, got %s\n", header, i,
         expected->SubjectUsage.rgpszUsageIdentifier[i],
         got->SubjectUsage.rgpszUsageIdentifier[i]);
    ok(expected->ListIdentifier.cbData == got->ListIdentifier.cbData,
     "%s: expected list identifier of %d bytes, got %d\n", header,
     expected->ListIdentifier.cbData, got->ListIdentifier.cbData);
    if (expected->ListIdentifier.cbData)
        ok(!memcmp(expected->ListIdentifier.pbData, got->ListIdentifier.pbData,
         expected->ListIdentifier.cbData),
         "%s: unexpected list identifier value\n", header);
    ok(expected->SequenceNumber.cbData == got->SequenceNumber.cbData,
     "%s: expected sequence number of %d bytes, got %d\n", header,
     expected->SequenceNumber.cbData, got->SequenceNumber.cbData);
    if (expected->SequenceNumber.cbData)
        ok(!memcmp(expected->SequenceNumber.pbData, got->SequenceNumber.pbData,
         expected->SequenceNumber.cbData),
         "%s: unexpected sequence number value\n", header);
    ok(!memcmp(&expected->ThisUpdate, &got->ThisUpdate, sizeof(FILETIME)),
     "%s: expected this update = (%d, %d), got (%d, %d)\n", header,
     expected->ThisUpdate.dwLowDateTime, expected->ThisUpdate.dwHighDateTime,
     got->ThisUpdate.dwLowDateTime, got->ThisUpdate.dwHighDateTime);
    ok(!memcmp(&expected->NextUpdate, &got->NextUpdate, sizeof(FILETIME)),
     "%s: expected next update = (%d, %d), got (%d, %d)\n", header,
     expected->NextUpdate.dwLowDateTime, expected->NextUpdate.dwHighDateTime,
     got->NextUpdate.dwLowDateTime, got->NextUpdate.dwHighDateTime);
    if (expected->SubjectAlgorithm.pszObjId &&
     *expected->SubjectAlgorithm.pszObjId && !got->SubjectAlgorithm.pszObjId)
        ok(0, "%s: expected subject algorithm %s, got NULL\n", header,
         expected->SubjectAlgorithm.pszObjId);
    if (expected->SubjectAlgorithm.pszObjId && got->SubjectAlgorithm.pszObjId)
        ok(!strcmp(expected->SubjectAlgorithm.pszObjId,
         got->SubjectAlgorithm.pszObjId),
         "%s: expected subject algorithm %s, got %s\n", header,
         expected->SubjectAlgorithm.pszObjId, got->SubjectAlgorithm.pszObjId);
    ok(expected->SubjectAlgorithm.Parameters.cbData ==
     got->SubjectAlgorithm.Parameters.cbData,
     "%s: expected subject algorithm parameters of %d bytes, got %d\n", header,
     expected->SubjectAlgorithm.Parameters.cbData,
     got->SubjectAlgorithm.Parameters.cbData);
    if (expected->SubjectAlgorithm.Parameters.cbData)
        ok(!memcmp(expected->SubjectAlgorithm.Parameters.pbData,
         got->SubjectAlgorithm.Parameters.pbData,
         expected->SubjectAlgorithm.Parameters.cbData),
         "%s: unexpected subject algorithm parameter value\n", header);
    ok(expected->cCTLEntry == got->cCTLEntry,
     "%s: expected %d CTL entries, got %d\n", header, expected->cCTLEntry,
     got->cCTLEntry);
    for (i = 0; i < expected->cCTLEntry; i++)
    {
        ok(expected->rgCTLEntry[i].SubjectIdentifier.cbData ==
         got->rgCTLEntry[i].SubjectIdentifier.cbData,
         "%s[%d]: expected subject identifier of %d bytes, got %d\n",
         header, i, expected->rgCTLEntry[i].SubjectIdentifier.cbData,
         got->rgCTLEntry[i].SubjectIdentifier.cbData);
        if (expected->rgCTLEntry[i].SubjectIdentifier.cbData)
            ok(!memcmp(expected->rgCTLEntry[i].SubjectIdentifier.pbData,
             got->rgCTLEntry[i].SubjectIdentifier.pbData,
             expected->rgCTLEntry[i].SubjectIdentifier.cbData),
             "%s[%d]: unexpected subject identifier value\n",
             header, i);
        for (j = 0; j < expected->rgCTLEntry[i].cAttribute; j++)
        {
            ok(!strcmp(expected->rgCTLEntry[i].rgAttribute[j].pszObjId,
             got->rgCTLEntry[i].rgAttribute[j].pszObjId),
             "%s[%d][%d]: expected attribute OID %s, got %s\n", header, i, j,
             expected->rgCTLEntry[i].rgAttribute[j].pszObjId,
             got->rgCTLEntry[i].rgAttribute[j].pszObjId);
            for (k = 0; k < expected->rgCTLEntry[i].rgAttribute[j].cValue; k++)
            {
                ok(expected->rgCTLEntry[i].rgAttribute[j].rgValue[k].cbData ==
                 got->rgCTLEntry[i].rgAttribute[j].rgValue[k].cbData,
                 "%s[%d][%d][%d]: expected value of %d bytes, got %d\n",
                 header, i, j, k,
                 expected->rgCTLEntry[i].rgAttribute[j].rgValue[k].cbData,
                 got->rgCTLEntry[i].rgAttribute[j].rgValue[k].cbData);
                if (expected->rgCTLEntry[i].rgAttribute[j].rgValue[k].cbData)
                    ok(!memcmp(
                     expected->rgCTLEntry[i].rgAttribute[j].rgValue[k].pbData,
                     got->rgCTLEntry[i].rgAttribute[j].rgValue[k].pbData,
                     expected->rgCTLEntry[i].rgAttribute[j].rgValue[k].cbData),
                     "%s[%d][%d][%d]: unexpected value\n",
                     header, i, j, k);
            }
        }
    }
    ok(expected->cExtension == got->cExtension,
     "%s: expected %d extensions, got %d\n", header, expected->cExtension,
     got->cExtension);
    for (i = 0; i < expected->cExtension; i++)
    {
        ok(!strcmp(expected->rgExtension[i].pszObjId,
         got->rgExtension[i].pszObjId), "%s[%d]: expected %s, got %s\n",
         header, i, expected->rgExtension[i].pszObjId,
         got->rgExtension[i].pszObjId);
        ok(expected->rgExtension[i].fCritical == got->rgExtension[i].fCritical,
         "%s[%d]: expected fCritical = %d, got %d\n", header, i,
         expected->rgExtension[i].fCritical, got->rgExtension[i].fCritical);
        ok(expected->rgExtension[i].Value.cbData ==
         got->rgExtension[i].Value.cbData,
         "%s[%d]: expected extension value to have %d bytes, got %d\n",
         header, i, expected->rgExtension[i].Value.cbData,
         got->rgExtension[i].Value.cbData);
        if (expected->rgExtension[i].Value.cbData)
            ok(!memcmp(expected->rgExtension[i].Value.pbData,
             got->rgExtension[i].Value.pbData,
             expected->rgExtension[i].Value.cbData),
             "%s[%d]: unexpected extension value\n", header, i);
    }
}

static const BYTE signedCTL[] = {
0x30,0x81,0xc7,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x07,0x02,0xa0,
0x81,0xb9,0x30,0x81,0xb6,0x02,0x01,0x01,0x31,0x0e,0x30,0x0c,0x06,0x08,0x2a,
0x86,0x48,0x86,0xf7,0x0d,0x02,0x05,0x05,0x00,0x30,0x28,0x06,0x09,0x2a,0x86,
0x48,0x86,0xf7,0x0d,0x01,0x07,0x01,0xa0,0x1b,0x04,0x19,0x30,0x17,0x30,0x00,
0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,
0x30,0x5a,0x30,0x02,0x06,0x00,0x31,0x77,0x30,0x75,0x02,0x01,0x01,0x30,0x1a,
0x30,0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,
0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00,0x02,0x01,0x01,0x30,0x0c,0x06,0x08,
0x2a,0x86,0x48,0x86,0xf7,0x0d,0x02,0x05,0x05,0x00,0x30,0x04,0x06,0x00,0x05,
0x00,0x04,0x40,0xca,0xd8,0x32,0xd1,0xbd,0x97,0x61,0x54,0xd6,0x80,0xcf,0x0d,
0xbd,0xa2,0x42,0xc7,0xca,0x37,0x91,0x7d,0x9d,0xac,0x8c,0xdf,0x05,0x8a,0x39,
0xc6,0x07,0xc1,0x37,0xe6,0xb9,0xd1,0x0d,0x26,0xec,0xa5,0xb0,0x8a,0x51,0x26,
0x2b,0x4f,0x73,0x44,0x86,0x83,0x5e,0x2b,0x6e,0xcc,0xf8,0x1b,0x85,0x53,0xe9,
0x7a,0x80,0x8f,0x6b,0x42,0x19,0x93 };
static const BYTE signedCTLWithCTLInnerContent[] = {
0x30,0x82,0x01,0x0f,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x07,0x02,
0xa0,0x82,0x01,0x00,0x30,0x81,0xfd,0x02,0x01,0x01,0x31,0x0e,0x30,0x0c,0x06,
0x08,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x02,0x05,0x05,0x00,0x30,0x30,0x06,0x09,
0x2b,0x06,0x01,0x04,0x01,0x82,0x37,0x0a,0x01,0xa0,0x23,0x30,0x21,0x30,0x00,
0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,
0x30,0x5a,0x30,0x0c,0x06,0x08,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x02,0x05,0x05,
0x00,0x31,0x81,0xb5,0x30,0x81,0xb2,0x02,0x01,0x01,0x30,0x1a,0x30,0x15,0x31,
0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,
0x4c,0x61,0x6e,0x67,0x00,0x02,0x01,0x01,0x30,0x0c,0x06,0x08,0x2a,0x86,0x48,
0x86,0xf7,0x0d,0x02,0x05,0x05,0x00,0xa0,0x3b,0x30,0x18,0x06,0x09,0x2a,0x86,
0x48,0x86,0xf7,0x0d,0x01,0x09,0x03,0x31,0x0b,0x06,0x09,0x2b,0x06,0x01,0x04,
0x01,0x82,0x37,0x0a,0x01,0x30,0x1f,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,
0x01,0x09,0x04,0x31,0x12,0x04,0x10,0x54,0x71,0xbc,0xe1,0x56,0x31,0xa2,0xf9,
0x65,0x70,0x34,0xf8,0xe2,0xe9,0xb4,0xf4,0x30,0x04,0x06,0x00,0x05,0x00,0x04,
0x40,0x2f,0x1b,0x9f,0x5a,0x4a,0x15,0x73,0xfa,0xb1,0x93,0x3d,0x09,0x52,0xdf,
0x6b,0x98,0x4b,0x13,0x5e,0xe7,0xbf,0x65,0xf4,0x9c,0xc2,0xb1,0x77,0x09,0xb1,
0x66,0x4d,0x72,0x0d,0xb1,0x1a,0x50,0x20,0xe0,0x57,0xa2,0x39,0xc7,0xcd,0x7f,
0x8e,0xe7,0x5f,0x76,0x2b,0xd1,0x6a,0x82,0xb3,0x30,0x25,0x61,0xf6,0x25,0x23,
0x57,0x6c,0x0b,0x47,0xb8 };

static void test_decodeCTL(DWORD dwEncoding)
{
    static char oid1[] = "1.2.3";
    static char oid2[] = "1.5.6";
    static BYTE nullData[] = { 5,0 };
    char *pOid1 = oid1;
    BOOL ret;
    BYTE *buf = NULL;
    DWORD size = 0;
    CTL_INFO info;
    SYSTEMTIME thisUpdate = { 2005, 6, 1, 6, 16, 10, 0, 0 };
    CTL_ENTRY ctlEntry[2];
    CRYPT_ATTRIBUTE attr1, attr2;
    CRYPT_ATTR_BLOB value1, value2;

    memset(&info, 0, sizeof(info));
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_CTL, emptyCTL, sizeof(emptyCTL),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        compareCTLInfo("empty CTL", &info, (CTL_INFO *)buf);
        LocalFree(buf);
        buf = NULL;
    }
    info.dwVersion = 1;
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_CTL, emptyCTLWithVersion1,
     sizeof(emptyCTLWithVersion1), CRYPT_DECODE_ALLOC_FLAG, NULL, &buf,
     &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        compareCTLInfo("v1 CTL", &info, (CTL_INFO *)buf);
        LocalFree(buf);
        buf = NULL;
    }
    info.dwVersion = 0;
    info.SubjectUsage.cUsageIdentifier = 1;
    info.SubjectUsage.rgpszUsageIdentifier = &pOid1;
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_CTL, ctlWithUsageIdentifier,
     sizeof(ctlWithUsageIdentifier), CRYPT_DECODE_ALLOC_FLAG, NULL,
     &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        compareCTLInfo("CTL with usage identifier", &info, (CTL_INFO *)buf);
        LocalFree(buf);
        buf = NULL;
    }
    info.SubjectUsage.cUsageIdentifier = 0;
    info.ListIdentifier.cbData = sizeof(serialNum);
    info.ListIdentifier.pbData = (LPBYTE)serialNum;
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_CTL, ctlWithListIdentifier,
     sizeof(ctlWithListIdentifier), CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        compareCTLInfo("CTL with list identifier", &info, (CTL_INFO *)buf);
        LocalFree(buf);
        buf = NULL;
    }
    info.ListIdentifier.cbData = 0;
    info.SequenceNumber.cbData = sizeof(serialNum);
    info.SequenceNumber.pbData = (LPBYTE)serialNum;
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_CTL, ctlWithSequenceNumber,
     sizeof(ctlWithSequenceNumber), CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        compareCTLInfo("CTL with sequence number", &info, (CTL_INFO *)buf);
        LocalFree(buf);
        buf = NULL;
    }
    info.SequenceNumber.cbData = 0;
    SystemTimeToFileTime(&thisUpdate, &info.ThisUpdate);
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_CTL, ctlWithThisUpdate,
     sizeof(ctlWithThisUpdate), CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        compareCTLInfo("CTL with this update", &info, (CTL_INFO *)buf);
        LocalFree(buf);
        buf = NULL;
    }
    SystemTimeToFileTime(&thisUpdate, &info.NextUpdate);
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_CTL, ctlWithThisAndNextUpdate,
     sizeof(ctlWithThisAndNextUpdate), CRYPT_DECODE_ALLOC_FLAG, NULL,
     &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        compareCTLInfo("CTL with this and next update", &info, (CTL_INFO *)buf);
        LocalFree(buf);
        buf = NULL;
    }
    info.ThisUpdate.dwLowDateTime = info.ThisUpdate.dwHighDateTime = 0;
    info.NextUpdate.dwLowDateTime = info.NextUpdate.dwHighDateTime = 0;
    info.SubjectAlgorithm.pszObjId = oid2;
    info.SubjectAlgorithm.Parameters.cbData = sizeof(nullData);
    info.SubjectAlgorithm.Parameters.pbData = nullData;
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_CTL, ctlWithAlgId,
     sizeof(ctlWithAlgId), CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        compareCTLInfo("CTL with algorithm identifier", &info, (CTL_INFO *)buf);
        LocalFree(buf);
        buf = NULL;
    }
    SetLastError(0xdeadbeef);
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_CTL, ctlWithBogusEntry,
     sizeof(ctlWithBogusEntry), CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret &&
     (GetLastError() == CRYPT_E_ASN1_EOD ||
      GetLastError() == CRYPT_E_ASN1_CORRUPT ||
      GetLastError() == OSS_MORE_INPUT), /* Win9x */
     "expected CRYPT_E_ASN1_EOD or CRYPT_E_ASN1_CORRUPT, got %08x\n",
     GetLastError());
    info.SubjectAlgorithm.Parameters.cbData = 0;
    info.ThisUpdate.dwLowDateTime = info.ThisUpdate.dwHighDateTime = 0;
    info.NextUpdate.dwLowDateTime = info.NextUpdate.dwHighDateTime = 0;
    info.SubjectAlgorithm.pszObjId = NULL;
    value1.cbData = sizeof(emptySequence);
    value1.pbData = (LPBYTE)emptySequence;
    attr1.pszObjId = oid1;
    attr1.cValue = 1;
    attr1.rgValue = &value1;
    ctlEntry[0].SubjectIdentifier.cbData = sizeof(serialNum);
    ctlEntry[0].SubjectIdentifier.pbData = (LPBYTE)serialNum;
    ctlEntry[0].cAttribute = 1;
    ctlEntry[0].rgAttribute = &attr1;
    info.cCTLEntry = 1;
    info.rgCTLEntry = ctlEntry;
    SetLastError(0xdeadbeef);
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_CTL, ctlWithOneEntry,
     sizeof(ctlWithOneEntry), CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        compareCTLInfo("CTL with one entry", &info, (CTL_INFO *)buf);
        LocalFree(buf);
        buf = NULL;
    }
    value2.cbData = sizeof(encodedIPAddr);
    value2.pbData = (LPBYTE)encodedIPAddr;
    attr2.pszObjId = oid2;
    attr2.cValue = 1;
    attr2.rgValue = &value2;
    ctlEntry[1].SubjectIdentifier.cbData = sizeof(serialNum);
    ctlEntry[1].SubjectIdentifier.pbData = (LPBYTE)serialNum;
    ctlEntry[1].cAttribute = 1;
    ctlEntry[1].rgAttribute = &attr2;
    info.cCTLEntry = 2;
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_CTL, ctlWithTwoEntries,
     sizeof(ctlWithTwoEntries), CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        compareCTLInfo("CTL with two entries", &info, (CTL_INFO *)buf);
        LocalFree(buf);
        buf = NULL;
    }
    /* A signed CTL isn't decodable, even if the inner content is a CTL */
    SetLastError(0xdeadbeef);
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_CTL, signedCTL,
     sizeof(signedCTL), CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && (GetLastError() == CRYPT_E_ASN1_BADTAG ||
     GetLastError() == OSS_DATA_ERROR /* Win9x */),
     "expected CRYPT_E_ASN1_BADTAG or OSS_DATA_ERROR, got %08x\n",
     GetLastError());
    SetLastError(0xdeadbeef);
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_CTL,
     signedCTLWithCTLInnerContent, sizeof(signedCTLWithCTLInnerContent),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && (GetLastError() == CRYPT_E_ASN1_BADTAG ||
     GetLastError() == OSS_DATA_ERROR /* Win9x */),
     "expected CRYPT_E_ASN1_BADTAG or OSS_DATA_ERROR, got %08x\n",
     GetLastError());
}

static const BYTE emptyPKCSContentInfo[] = { 0x30,0x04,0x06,0x02,0x2a,0x03 };
static const BYTE emptyPKCSContentInfoExtraBytes[] = { 0x30,0x04,0x06,0x02,0x2a,
 0x03,0,0,0,0,0,0 };
static const BYTE bogusPKCSContentInfo[] = { 0x30,0x07,0x06,0x02,0x2a,0x03,
 0xa0,0x01,0x01 };
static const BYTE intPKCSContentInfo[] = { 0x30,0x09,0x06,0x02,0x2a,0x03,0xa0,
 0x03,0x02,0x01,0x01 };
static BYTE bogusDER[] = { 1 };

static void test_encodePKCSContentInfo(DWORD dwEncoding)
{
    BOOL ret;
    BYTE *buf = NULL;
    DWORD size = 0;
    CRYPT_CONTENT_INFO info = { 0 };
    char oid1[] = "1.2.3";

    if (0)
    {
        /* Crashes on win9x */
        SetLastError(0xdeadbeef);
        ret = pCryptEncodeObjectEx(dwEncoding, PKCS_CONTENT_INFO, NULL,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
        ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
         "Expected STATUS_ACCESS_VIOLATION, got %x\n", GetLastError());
    }
    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_CONTENT_INFO, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && (GetLastError() == E_INVALIDARG ||
     GetLastError() == OSS_LIMITED /* Win9x */),
     "Expected E_INVALIDARG or OSS_LIMITED, got %x\n", GetLastError());
    info.pszObjId = oid1;
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_CONTENT_INFO, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptyPKCSContentInfo), "Unexpected size %d\n", size);
        ok(!memcmp(buf, emptyPKCSContentInfo, size), "Unexpected value\n");
        LocalFree(buf);
    }
    info.Content.pbData = bogusDER;
    info.Content.cbData = sizeof(bogusDER);
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_CONTENT_INFO, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed; %x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(bogusPKCSContentInfo), "Unexpected size %d\n", size);
        ok(!memcmp(buf, bogusPKCSContentInfo, size), "Unexpected value\n");
        LocalFree(buf);
    }
    info.Content.pbData = (BYTE *)ints[0].encoded;
    info.Content.cbData = ints[0].encoded[1] + 2;
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_CONTENT_INFO, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (ret)
    {
        ok(size == sizeof(intPKCSContentInfo), "Unexpected size %d\n", size);
        ok(!memcmp(buf, intPKCSContentInfo, size), "Unexpected value\n");
        LocalFree(buf);
    }
}

static const BYTE indefiniteSignedPKCSContent[] = {
0x30,0x80,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x07,0x02,0xa0,0x80,
0x30,0x80,0x02,0x01,0x01,0x31,0x0e,0x30,0x0c,0x06,0x08,0x2a,0x86,0x48,0x86,
0xf7,0x0d,0x02,0x05,0x05,0x00,0x30,0x80,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,
0x0d,0x01,0x07,0x01,0xa0,0x80,0x24,0x80,0x04,0x04,0x01,0x02,0x03,0x04,0x04,
0x04,0x01,0x02,0x03,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0xa0,0x81,0xd2,0x30,
0x81,0xcf,0x02,0x01,0x01,0x30,0x02,0x06,0x00,0x30,0x15,0x31,0x13,0x30,0x11,
0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,
0x67,0x00,0x30,0x22,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,
0x30,0x30,0x30,0x30,0x30,0x5a,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,
0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x15,0x31,0x13,0x30,0x11,0x06,
0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,
0x00,0x30,0x5c,0x30,0x0d,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,
0x01,0x05,0x00,0x03,0x4b,0x00,0x30,0x48,0x02,0x41,0x00,0xe2,0x54,0x3a,0xa7,
0x83,0xb1,0x27,0x14,0x3e,0x59,0xbb,0xb4,0x53,0xe6,0x1f,0xe7,0x5d,0xf1,0x21,
0x68,0xad,0x85,0x53,0xdb,0x6b,0x1e,0xeb,0x65,0x97,0x03,0x86,0x60,0xde,0xf3,
0x6c,0x38,0x75,0xe0,0x4c,0x61,0xbb,0xbc,0x62,0x17,0xa9,0xcd,0x79,0x3f,0x21,
0x4e,0x96,0xcb,0x0e,0xdc,0x61,0x94,0x30,0x18,0x10,0x6b,0xd0,0x1c,0x10,0x79,
0x02,0x03,0x01,0x00,0x01,0xa3,0x16,0x30,0x14,0x30,0x12,0x06,0x03,0x55,0x1d,
0x13,0x01,0x01,0xff,0x04,0x08,0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01,0x31,
0x77,0x30,0x75,0x02,0x01,0x01,0x30,0x1a,0x30,0x15,0x31,0x13,0x30,0x11,0x06,
0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,
0x00,0x02,0x01,0x01,0x30,0x0c,0x06,0x08,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x02,
0x05,0x05,0x00,0x30,0x04,0x06,0x00,0x05,0x00,0x04,0x40,0x57,0xba,0xe0,0xad,
0xfe,0x36,0x8d,0xb3,0x88,0xa2,0x8d,0x84,0x82,0x52,0x09,0x09,0xd9,0xf0,0xb8,
0x04,0xfa,0xb5,0x51,0x0b,0x2b,0x2e,0xd5,0x72,0x3e,0x3d,0x13,0x8a,0x51,0xc3,
0x71,0x65,0x9a,0x52,0xf2,0x8f,0xb2,0x5b,0x39,0x28,0xb3,0x29,0x36,0xa5,0x8d,
0xe3,0x55,0x71,0x91,0xf9,0x2a,0xd1,0xb8,0xaa,0x52,0xb8,0x22,0x3a,0xeb,0x61,
0x00,0x00,0x00,0x00,0x00,0x00 };

static void test_decodePKCSContentInfo(DWORD dwEncoding)
{
    BOOL ret;
    LPBYTE buf = NULL;
    DWORD size = 0;
    CRYPT_CONTENT_INFO *info;

    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_CONTENT_INFO,
     emptyPKCSContentInfo, sizeof(emptyPKCSContentInfo),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %x\n", GetLastError());
    if (ret)
    {
        info = (CRYPT_CONTENT_INFO *)buf;

        ok(!strcmp(info->pszObjId, "1.2.3"), "Expected 1.2.3, got %s\n",
         info->pszObjId);
        ok(info->Content.cbData == 0, "Expected no data, got %d\n",
         info->Content.cbData);
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_CONTENT_INFO,
     emptyPKCSContentInfoExtraBytes, sizeof(emptyPKCSContentInfoExtraBytes),
     0, NULL, NULL, &size);
    ok(ret, "CryptDecodeObjectEx failed: %x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_CONTENT_INFO,
     bogusPKCSContentInfo, sizeof(bogusPKCSContentInfo),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    /* Native fails with CRYPT_E_ASN1_EOD, accept also CRYPT_E_ASN1_CORRUPT as
     * I doubt an app depends on that.
     */
    ok((!ret && (GetLastError() == CRYPT_E_ASN1_EOD ||
     GetLastError() == CRYPT_E_ASN1_CORRUPT)) || broken(ret),
     "Expected CRYPT_E_ASN1_EOD or CRYPT_E_ASN1_CORRUPT, got %x\n",
     GetLastError());
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_CONTENT_INFO,
     intPKCSContentInfo, sizeof(intPKCSContentInfo),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %x\n", GetLastError());
    if (ret)
    {
        info = (CRYPT_CONTENT_INFO *)buf;

        ok(!strcmp(info->pszObjId, "1.2.3"), "Expected 1.2.3, got %s\n",
         info->pszObjId);
        ok(info->Content.cbData == ints[0].encoded[1] + 2,
         "Unexpected size %d\n", info->Content.cbData);
        ok(!memcmp(info->Content.pbData, ints[0].encoded,
         info->Content.cbData), "Unexpected value\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_CONTENT_INFO,
     indefiniteSignedPKCSContent, sizeof(indefiniteSignedPKCSContent),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %x\n", GetLastError());
    if (ret)
    {
        info = (CRYPT_CONTENT_INFO *)buf;

        ok(!strcmp(info->pszObjId, szOID_RSA_signedData),
         "Expected %s, got %s\n", szOID_RSA_signedData, info->pszObjId);
        ok(info->Content.cbData == 392, "Expected 392, got %d\n",
         info->Content.cbData);
        LocalFree(buf);
    }
}

static const BYTE emptyPKCSAttr[] = { 0x30,0x06,0x06,0x02,0x2a,0x03,0x31,
 0x00 };
static const BYTE bogusPKCSAttr[] = { 0x30,0x07,0x06,0x02,0x2a,0x03,0x31,0x01,
 0x01 };
static const BYTE intPKCSAttr[] = { 0x30,0x09,0x06,0x02,0x2a,0x03,0x31,0x03,
 0x02,0x01,0x01 };

static void test_encodePKCSAttribute(DWORD dwEncoding)
{
    CRYPT_ATTRIBUTE attr = { 0 };
    BOOL ret;
    LPBYTE buf = NULL;
    DWORD size = 0;
    CRYPT_ATTR_BLOB blob;
    char oid[] = "1.2.3";

    if (0)
    {
        /* Crashes on win9x */
        SetLastError(0xdeadbeef);
        ret = pCryptEncodeObjectEx(dwEncoding, PKCS_ATTRIBUTE, NULL,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
        ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
         "Expected STATUS_ACCESS_VIOLATION, got %x\n", GetLastError());
    }
    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_ATTRIBUTE, &attr,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && (GetLastError() == E_INVALIDARG ||
     GetLastError() == OSS_LIMITED /* Win9x */),
     "Expected E_INVALIDARG or OSS_LIMITED, got %x\n", GetLastError());
    attr.pszObjId = oid;
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_ATTRIBUTE, &attr,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptyPKCSAttr), "Unexpected size %d\n", size);
        ok(!memcmp(buf, emptyPKCSAttr, size), "Unexpected value\n");
        LocalFree(buf);
    }
    blob.cbData = sizeof(bogusDER);
    blob.pbData = bogusDER;
    attr.cValue = 1;
    attr.rgValue = &blob;
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_ATTRIBUTE, &attr,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(bogusPKCSAttr), "Unexpected size %d\n", size);
        ok(!memcmp(buf, bogusPKCSAttr, size), "Unexpected value\n");
        LocalFree(buf);
    }
    blob.pbData = (BYTE *)ints[0].encoded;
    blob.cbData = ints[0].encoded[1] + 2;
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_ATTRIBUTE, &attr,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (ret)
    {
        ok(size == sizeof(intPKCSAttr), "Unexpected size %d\n", size);
        ok(!memcmp(buf, intPKCSAttr, size), "Unexpected value\n");
        LocalFree(buf);
    }
}

static void test_decodePKCSAttribute(DWORD dwEncoding)
{
    BOOL ret;
    LPBYTE buf = NULL;
    DWORD size = 0;
    CRYPT_ATTRIBUTE *attr;

    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_ATTRIBUTE,
     emptyPKCSAttr, sizeof(emptyPKCSAttr),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %x\n", GetLastError());
    if (ret)
    {
        attr = (CRYPT_ATTRIBUTE *)buf;

        ok(!strcmp(attr->pszObjId, "1.2.3"), "Expected 1.2.3, got %s\n",
         attr->pszObjId);
        ok(attr->cValue == 0, "Expected no value, got %d\n", attr->cValue);
        LocalFree(buf);
    }
    SetLastError(0xdeadbeef);
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_ATTRIBUTE,
     bogusPKCSAttr, sizeof(bogusPKCSAttr),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    /* Native fails with CRYPT_E_ASN1_EOD, accept also CRYPT_E_ASN1_CORRUPT as
     * I doubt an app depends on that.
     */
    ok(!ret && (GetLastError() == CRYPT_E_ASN1_EOD ||
     GetLastError() == CRYPT_E_ASN1_CORRUPT ||
     GetLastError() == OSS_MORE_INPUT /* Win9x */),
     "Expected CRYPT_E_ASN1_EOD, CRYPT_E_ASN1_CORRUPT, or OSS_MORE_INPUT, got %x\n",
     GetLastError());
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_ATTRIBUTE,
     intPKCSAttr, sizeof(intPKCSAttr),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %x\n", GetLastError());
    if (ret)
    {
        attr = (CRYPT_ATTRIBUTE *)buf;

        ok(!strcmp(attr->pszObjId, "1.2.3"), "Expected 1.2.3, got %s\n",
         attr->pszObjId);
        ok(attr->cValue == 1, "Expected 1 value, got %d\n", attr->cValue);
        ok(attr->rgValue[0].cbData == ints[0].encoded[1] + 2,
         "Unexpected size %d\n", attr->rgValue[0].cbData);
        ok(!memcmp(attr->rgValue[0].pbData, ints[0].encoded,
         attr->rgValue[0].cbData), "Unexpected value\n");
        LocalFree(buf);
    }
}

static const BYTE emptyPKCSAttributes[] = { 0x31,0x00 };
static const BYTE singlePKCSAttributes[] = { 0x31,0x08,0x30,0x06,0x06,0x02,
 0x2a,0x03,0x31,0x00 };
static const BYTE doublePKCSAttributes[] = { 0x31,0x13,0x30,0x06,0x06,0x02,
 0x2a,0x03,0x31,0x00,0x30,0x09,0x06,0x02,0x2d,0x06,0x31,0x03,0x02,0x01,0x01 };

static void test_encodePKCSAttributes(DWORD dwEncoding)
{
    CRYPT_ATTRIBUTES attributes = { 0 };
    CRYPT_ATTRIBUTE attr[2] = { { 0 } };
    CRYPT_ATTR_BLOB blob;
    BOOL ret;
    LPBYTE buf = NULL;
    DWORD size = 0;
    char oid1[] = "1.2.3", oid2[] = "1.5.6";

    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_ATTRIBUTES, &attributes,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptyPKCSAttributes), "Unexpected size %d\n", size);
        ok(!memcmp(buf, emptyPKCSAttributes, size), "Unexpected value\n");
        LocalFree(buf);
    }
    attributes.cAttr = 1;
    attributes.rgAttr = attr;
    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_ATTRIBUTES, &attributes,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && (GetLastError() == E_INVALIDARG ||
     GetLastError() == OSS_LIMITED /* Win9x */),
     "Expected E_INVALIDARG or OSS_LIMITED, got %08x\n", GetLastError());
    attr[0].pszObjId = oid1;
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_ATTRIBUTES, &attributes,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (ret)
    {
        ok(size == sizeof(singlePKCSAttributes), "Unexpected size %d\n", size);
        ok(!memcmp(buf, singlePKCSAttributes, size), "Unexpected value\n");
        LocalFree(buf);
    }
    attr[1].pszObjId = oid2;
    attr[1].cValue = 1;
    attr[1].rgValue = &blob;
    blob.pbData = (BYTE *)ints[0].encoded;
    blob.cbData = ints[0].encoded[1] + 2;
    attributes.cAttr = 2;
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_ATTRIBUTES, &attributes,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(doublePKCSAttributes), "Unexpected size %d\n", size);
        ok(!memcmp(buf, doublePKCSAttributes, size), "Unexpected value\n");
        LocalFree(buf);
    }
}

static void test_decodePKCSAttributes(DWORD dwEncoding)
{
    BOOL ret;
    LPBYTE buf = NULL;
    DWORD size = 0;
    CRYPT_ATTRIBUTES *attributes;

    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_ATTRIBUTES,
     emptyPKCSAttributes, sizeof(emptyPKCSAttributes),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %x\n", GetLastError());
    if (ret)
    {
        attributes = (CRYPT_ATTRIBUTES *)buf;
        ok(attributes->cAttr == 0, "Expected no attributes, got %d\n",
         attributes->cAttr);
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_ATTRIBUTES,
     singlePKCSAttributes, sizeof(singlePKCSAttributes),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %x\n", GetLastError());
    if (ret)
    {
        attributes = (CRYPT_ATTRIBUTES *)buf;
        ok(attributes->cAttr == 1, "Expected 1 attribute, got %d\n",
         attributes->cAttr);
        ok(!strcmp(attributes->rgAttr[0].pszObjId, "1.2.3"),
         "Expected 1.2.3, got %s\n", attributes->rgAttr[0].pszObjId);
        ok(attributes->rgAttr[0].cValue == 0,
         "Expected no attributes, got %d\n", attributes->rgAttr[0].cValue);
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_ATTRIBUTES,
     doublePKCSAttributes, sizeof(doublePKCSAttributes),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %x\n", GetLastError());
    if (ret)
    {
        attributes = (CRYPT_ATTRIBUTES *)buf;
        ok(attributes->cAttr == 2, "Expected 2 attributes, got %d\n",
         attributes->cAttr);
        ok(!strcmp(attributes->rgAttr[0].pszObjId, "1.2.3"),
         "Expected 1.2.3, got %s\n", attributes->rgAttr[0].pszObjId);
        ok(attributes->rgAttr[0].cValue == 0,
         "Expected no attributes, got %d\n", attributes->rgAttr[0].cValue);
        ok(!strcmp(attributes->rgAttr[1].pszObjId, "1.5.6"),
         "Expected 1.5.6, got %s\n", attributes->rgAttr[1].pszObjId);
        ok(attributes->rgAttr[1].cValue == 1,
         "Expected 1 attribute, got %d\n", attributes->rgAttr[1].cValue);
        ok(attributes->rgAttr[1].rgValue[0].cbData == ints[0].encoded[1] + 2,
         "Unexpected size %d\n", attributes->rgAttr[1].rgValue[0].cbData);
        ok(!memcmp(attributes->rgAttr[1].rgValue[0].pbData, ints[0].encoded,
         attributes->rgAttr[1].rgValue[0].cbData), "Unexpected value\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_ATTRIBUTES,
     doublePKCSAttributes, sizeof(doublePKCSAttributes), 0, NULL, NULL, &size);
    ok(ret, "CryptDecodeObjectEx failed: %x\n", GetLastError());
    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    if (buf)
    {
        ret = pCryptDecodeObjectEx(dwEncoding, PKCS_ATTRIBUTES,
         doublePKCSAttributes, sizeof(doublePKCSAttributes), 0, NULL, buf, &size);
        ok(ret, "CryptDecodeObjectEx failed: %x\n", GetLastError());
        HeapFree(GetProcessHeap(), 0, buf);
    }
}

static const BYTE singleCapability[] = {
0x30,0x06,0x30,0x04,0x06,0x02,0x2d,0x06 };
static const BYTE twoCapabilities[] = {
0x30,0x0c,0x30,0x04,0x06,0x02,0x2d,0x06,0x30,0x04,0x06,0x02,0x2a,0x03 };
static const BYTE singleCapabilitywithNULL[] = {
0x30,0x08,0x30,0x06,0x06,0x02,0x2d,0x06,0x05,0x00 };

static void test_encodePKCSSMimeCapabilities(DWORD dwEncoding)
{
    static char oid1[] = "1.5.6", oid2[] = "1.2.3";
    BOOL ret;
    LPBYTE buf = NULL;
    DWORD size = 0;
    CRYPT_SMIME_CAPABILITY capability[2];
    CRYPT_SMIME_CAPABILITIES capabilities;

    /* An empty capabilities is allowed */
    capabilities.cCapability = 0;
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_SMIME_CAPABILITIES,
     &capabilities, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptySequence), "unexpected size %d\n", size);
        ok(!memcmp(buf, emptySequence, size), "unexpected value\n");
        LocalFree(buf);
    }
    /* A non-empty capabilities with an empty capability (lacking an OID) is
     * not allowed
     */
    capability[0].pszObjId = NULL;
    capability[0].Parameters.cbData = 0;
    capabilities.cCapability = 1;
    capabilities.rgCapability = capability;
    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_SMIME_CAPABILITIES,
     &capabilities, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && (GetLastError() == E_INVALIDARG ||
     GetLastError() == OSS_LIMITED /* Win9x */),
     "Expected E_INVALIDARG or OSS_LIMITED, got %08x\n", GetLastError());
    capability[0].pszObjId = oid1;
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_SMIME_CAPABILITIES,
     &capabilities, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(singleCapability), "unexpected size %d\n", size);
        ok(!memcmp(buf, singleCapability, size), "unexpected value\n");
        LocalFree(buf);
    }
    capability[1].pszObjId = oid2;
    capability[1].Parameters.cbData = 0;
    capabilities.cCapability = 2;
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS_SMIME_CAPABILITIES,
     &capabilities, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(twoCapabilities), "unexpected size %d\n", size);
        ok(!memcmp(buf, twoCapabilities, size), "unexpected value\n");
        LocalFree(buf);
    }
}

static void compareSMimeCapabilities(LPCSTR header,
 const CRYPT_SMIME_CAPABILITIES *expected, const CRYPT_SMIME_CAPABILITIES *got)
{
    DWORD i;

    ok(got->cCapability == expected->cCapability,
     "%s: expected %d capabilities, got %d\n", header, expected->cCapability,
     got->cCapability);
    for (i = 0; i < expected->cCapability; i++)
    {
        ok(!strcmp(expected->rgCapability[i].pszObjId,
         got->rgCapability[i].pszObjId), "%s[%d]: expected %s, got %s\n",
         header, i, expected->rgCapability[i].pszObjId,
         got->rgCapability[i].pszObjId);
        ok(expected->rgCapability[i].Parameters.cbData ==
         got->rgCapability[i].Parameters.cbData,
         "%s[%d]: expected %d bytes, got %d\n", header, i,
         expected->rgCapability[i].Parameters.cbData,
         got->rgCapability[i].Parameters.cbData);
        if (expected->rgCapability[i].Parameters.cbData)
            ok(!memcmp(expected->rgCapability[i].Parameters.pbData,
             got->rgCapability[i].Parameters.pbData,
             expected->rgCapability[i].Parameters.cbData),
             "%s[%d]: unexpected value\n", header, i);
    }
}

static void test_decodePKCSSMimeCapabilities(DWORD dwEncoding)
{
    static char oid1[] = "1.5.6", oid2[] = "1.2.3";
    BOOL ret;
    DWORD size = 0;
    CRYPT_SMIME_CAPABILITY capability[2];
    CRYPT_SMIME_CAPABILITIES capabilities, *ptr;

    SetLastError(0xdeadbeef);
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_SMIME_CAPABILITIES,
     emptySequence, sizeof(emptySequence),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &ptr, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        capabilities.cCapability = 0;
        compareSMimeCapabilities("empty capabilities", &capabilities, ptr);
        LocalFree(ptr);
    }
    SetLastError(0xdeadbeef);
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_SMIME_CAPABILITIES,
     singleCapability, sizeof(singleCapability), CRYPT_DECODE_ALLOC_FLAG, NULL,
     &ptr, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        capability[0].pszObjId = oid1;
        capability[0].Parameters.cbData = 0;
        capabilities.cCapability = 1;
        capabilities.rgCapability = capability;
        compareSMimeCapabilities("single capability", &capabilities, ptr);
        LocalFree(ptr);
    }
    SetLastError(0xdeadbeef);
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_SMIME_CAPABILITIES,
     singleCapabilitywithNULL, sizeof(singleCapabilitywithNULL),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &ptr, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        BYTE NULLparam[] = {0x05, 0x00};
        capability[0].pszObjId = oid1;
        capability[0].Parameters.cbData = 2;
        capability[0].Parameters.pbData = NULLparam;
        capabilities.cCapability = 1;
        capabilities.rgCapability = capability;
        compareSMimeCapabilities("single capability with NULL", &capabilities,
         ptr);
        LocalFree(ptr);
    }
    SetLastError(0xdeadbeef);
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_SMIME_CAPABILITIES,
    twoCapabilities, sizeof(twoCapabilities), CRYPT_DECODE_ALLOC_FLAG, NULL,
    &ptr, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        capability[0].Parameters.cbData = 0;
        capability[1].pszObjId = oid2;
        capability[1].Parameters.cbData = 0;
        capabilities.cCapability = 2;
        compareSMimeCapabilities("two capabilities", &capabilities, ptr);
        LocalFree(ptr);
    }
    SetLastError(0xdeadbeef);
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_SMIME_CAPABILITIES,
     twoCapabilities, sizeof(twoCapabilities), 0, NULL, NULL, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    ptr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    if (ptr)
    {
        SetLastError(0xdeadbeef);
        ret = pCryptDecodeObjectEx(dwEncoding, PKCS_SMIME_CAPABILITIES,
         twoCapabilities, sizeof(twoCapabilities), 0, NULL, ptr, &size);
        ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
        HeapFree(GetProcessHeap(), 0, ptr);
    }
}

static BYTE encodedCommonNameNoNull[] = { 0x30,0x14,0x31,0x12,0x30,0x10,
 0x06,0x03,0x55,0x04,0x03,0x13,0x09,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,
 0x67 };
static const BYTE minimalPKCSSigner[] = {
 0x30,0x2b,0x02,0x01,0x00,0x30,0x18,0x30,0x14,0x31,0x12,0x30,0x10,0x06,0x03,
 0x55,0x04,0x03,0x13,0x09,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x02,
 0x00,0x30,0x04,0x06,0x00,0x05,0x00,0x30,0x04,0x06,0x00,0x05,0x00,0x04,0x00 };
static const BYTE PKCSSignerWithSerial[] = {
 0x30,0x2c,0x02,0x01,0x00,0x30,0x19,0x30,0x14,0x31,0x12,0x30,0x10,0x06,0x03,
 0x55,0x04,0x03,0x13,0x09,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x02,
 0x01,0x01,0x30,0x04,0x06,0x00,0x05,0x00,0x30,0x04,0x06,0x00,0x05,0x00,0x04,
 0x00 };
static const BYTE PKCSSignerWithHashAlgo[] = {
 0x30,0x2e,0x02,0x01,0x00,0x30,0x19,0x30,0x14,0x31,0x12,0x30,0x10,0x06,0x03,
 0x55,0x04,0x03,0x13,0x09,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x02,
 0x01,0x01,0x30,0x06,0x06,0x02,0x2a,0x03,0x05,0x00,0x30,0x04,0x06,0x00,0x05,
 0x00,0x04,0x00 };
static const BYTE PKCSSignerWithHashAndEncryptionAlgo[] = {
 0x30,0x30,0x02,0x01,0x00,0x30,0x19,0x30,0x14,0x31,0x12,0x30,0x10,0x06,0x03,
 0x55,0x04,0x03,0x13,0x09,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x02,
 0x01,0x01,0x30,0x06,0x06,0x02,0x2a,0x03,0x05,0x00,0x30,0x06,0x06,0x02,0x2d,
 0x06,0x05,0x00,0x04,0x00 };
static const BYTE PKCSSignerWithHash[] = {
 0x30,0x40,0x02,0x01,0x00,0x30,0x19,0x30,0x14,0x31,0x12,0x30,0x10,0x06,0x03,
 0x55,0x04,0x03,0x13,0x09,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x02,
 0x01,0x01,0x30,0x06,0x06,0x02,0x2a,0x03,0x05,0x00,0x30,0x06,0x06,0x02,0x2d,
 0x06,0x05,0x00,0x04,0x10,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,
 0x0a,0x0b,0x0c,0x0d,0x0e,0x0f };
static const BYTE PKCSSignerWithAuthAttr[] = {
0x30,0x62,0x02,0x01,0x00,0x30,0x19,0x30,0x14,0x31,0x12,0x30,0x10,0x06,0x03,
0x55,0x04,0x03,0x13,0x09,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x02,
0x01,0x01,0x30,0x06,0x06,0x02,0x2a,0x03,0x05,0x00,0xa0,0x20,0x30,0x1e,0x06,
0x03,0x55,0x04,0x03,0x31,0x17,0x30,0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,
0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00,0x30,
0x06,0x06,0x02,0x2d,0x06,0x05,0x00,0x04,0x10,0x00,0x01,0x02,0x03,0x04,0x05,
0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f };

static void test_encodePKCSSignerInfo(DWORD dwEncoding)
{
    static char oid1[] = "1.2.3", oid2[] = "1.5.6";
    BOOL ret;
    LPBYTE buf = NULL;
    DWORD size = 0;
    CMSG_SIGNER_INFO info = { 0 };
    char oid_common_name[] = szOID_COMMON_NAME;
    CRYPT_ATTR_BLOB commonName = { sizeof(encodedCommonName),
     (LPBYTE)encodedCommonName };
    CRYPT_ATTRIBUTE attr = { oid_common_name, 1, &commonName };

    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS7_SIGNER_INFO, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (!ret && GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        skip("no PKCS7_SIGNER_INFO encode support\n");
        return;
    }
    ok(!ret && (GetLastError() == E_INVALIDARG ||
     GetLastError() == OSS_LIMITED /* Win9x */),
     "Expected E_INVALIDARG or OSS_LIMITED, got %08x\n", GetLastError());
    /* To be encoded, a signer must have an issuer at least, and the encoding
     * must include PKCS_7_ASN_ENCODING.  (That isn't enough to be decoded,
     * see decoding tests.)
     */
    info.Issuer.cbData = sizeof(encodedCommonNameNoNull);
    info.Issuer.pbData = encodedCommonNameNoNull;
    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS7_SIGNER_INFO, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (!(dwEncoding & PKCS_7_ASN_ENCODING))
        ok(!ret && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08x\n", GetLastError());
    else
    {
        ok(ret || broken(GetLastError() == OSS_LIMITED /* Win9x */),
         "CryptEncodeObjectEx failed: %x\n", GetLastError());
        if (ret)
        {
            ok(size == sizeof(minimalPKCSSigner), "Unexpected size %d\n", size);
            if (size == sizeof(minimalPKCSSigner))
                ok(!memcmp(buf, minimalPKCSSigner, size), "Unexpected value\n");
            else
                ok(0, "Unexpected value\n");
            LocalFree(buf);
        }
    }
    info.SerialNumber.cbData = sizeof(serialNum);
    info.SerialNumber.pbData = (BYTE *)serialNum;
    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS7_SIGNER_INFO, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (!(dwEncoding & PKCS_7_ASN_ENCODING))
        ok(!ret && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08x\n", GetLastError());
    else
    {
        ok(ret || broken(GetLastError() == OSS_LIMITED /* Win9x */),
         "CryptEncodeObjectEx failed: %x\n", GetLastError());
        if (ret)
        {
            ok(size == sizeof(PKCSSignerWithSerial), "Unexpected size %d\n",
             size);
            if (size == sizeof(PKCSSignerWithSerial))
                ok(!memcmp(buf, PKCSSignerWithSerial, size),
                 "Unexpected value\n");
            else
                ok(0, "Unexpected value\n");
            LocalFree(buf);
        }
    }
    info.HashAlgorithm.pszObjId = oid1;
    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS7_SIGNER_INFO, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (!(dwEncoding & PKCS_7_ASN_ENCODING))
        ok(!ret && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08x\n", GetLastError());
    else
    {
        ok(ret || broken(GetLastError() == OSS_LIMITED /* Win9x */),
         "CryptEncodeObjectEx failed: %x\n", GetLastError());
        if (ret)
        {
            ok(size == sizeof(PKCSSignerWithHashAlgo), "Unexpected size %d\n",
             size);
            if (size == sizeof(PKCSSignerWithHashAlgo))
                ok(!memcmp(buf, PKCSSignerWithHashAlgo, size),
                 "Unexpected value\n");
            else
                ok(0, "Unexpected value\n");
            LocalFree(buf);
        }
    }
    info.HashEncryptionAlgorithm.pszObjId = oid2;
    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS7_SIGNER_INFO, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (!(dwEncoding & PKCS_7_ASN_ENCODING))
        ok(!ret && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08x\n", GetLastError());
    else
    {
        ok(ret, "CryptEncodeObjectEx failed: %x\n", GetLastError());
        if (ret)
        {
            ok(size == sizeof(PKCSSignerWithHashAndEncryptionAlgo),
             "Unexpected size %d\n", size);
            if (size == sizeof(PKCSSignerWithHashAndEncryptionAlgo))
                ok(!memcmp(buf, PKCSSignerWithHashAndEncryptionAlgo, size),
                 "Unexpected value\n");
            else
                ok(0, "Unexpected value\n");
            LocalFree(buf);
        }
    }
    info.EncryptedHash.cbData = sizeof(hash);
    info.EncryptedHash.pbData = (BYTE *)hash;
    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS7_SIGNER_INFO, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (!(dwEncoding & PKCS_7_ASN_ENCODING))
        ok(!ret && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08x\n", GetLastError());
    else
    {
        ok(ret, "CryptEncodeObjectEx failed: %x\n", GetLastError());
        if (ret)
        {
            ok(size == sizeof(PKCSSignerWithHash), "Unexpected size %d\n",
             size);
            if (size == sizeof(PKCSSignerWithHash))
                ok(!memcmp(buf, PKCSSignerWithHash, size),
                 "Unexpected value\n");
            else
                ok(0, "Unexpected value\n");
            LocalFree(buf);
        }
    }
    info.AuthAttrs.cAttr = 1;
    info.AuthAttrs.rgAttr = &attr;
    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(dwEncoding, PKCS7_SIGNER_INFO, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (!(dwEncoding & PKCS_7_ASN_ENCODING))
        ok(!ret && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08x\n", GetLastError());
    else
    {
        ok(ret, "CryptEncodeObjectEx failed: %x\n", GetLastError());
        if (ret)
        {
            ok(size == sizeof(PKCSSignerWithAuthAttr), "Unexpected size %d\n",
             size);
            if (size == sizeof(PKCSSignerWithAuthAttr))
                ok(!memcmp(buf, PKCSSignerWithAuthAttr, size),
                 "Unexpected value\n");
            else
                ok(0, "Unexpected value\n");
            LocalFree(buf);
        }
    }
}

static void test_decodePKCSSignerInfo(DWORD dwEncoding)
{
    BOOL ret;
    LPBYTE buf = NULL;
    DWORD size = 0;
    CMSG_SIGNER_INFO *info;

    /* A PKCS signer can't be decoded without a serial number. */
    SetLastError(0xdeadbeef);
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS7_SIGNER_INFO,
     minimalPKCSSigner, sizeof(minimalPKCSSigner),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && (GetLastError() == CRYPT_E_ASN1_CORRUPT ||
     GetLastError() == OSS_DATA_ERROR /* Win9x */),
     "Expected CRYPT_E_ASN1_CORRUPT or OSS_DATA_ERROR, got %x\n",
     GetLastError());
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS7_SIGNER_INFO,
     PKCSSignerWithSerial, sizeof(PKCSSignerWithSerial),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret || broken(GetLastError() == OSS_DATA_ERROR),
     "CryptDecodeObjectEx failed: %x\n", GetLastError());
    if (ret)
    {
        info = (CMSG_SIGNER_INFO *)buf;
        ok(info->dwVersion == 0, "Expected version 0, got %d\n",
         info->dwVersion);
        ok(info->Issuer.cbData == sizeof(encodedCommonNameNoNull),
         "Unexpected size %d\n", info->Issuer.cbData);
        ok(!memcmp(info->Issuer.pbData, encodedCommonNameNoNull,
         info->Issuer.cbData), "Unexpected value\n");
        ok(info->SerialNumber.cbData == sizeof(serialNum),
         "Unexpected size %d\n", info->SerialNumber.cbData);
        ok(!memcmp(info->SerialNumber.pbData, serialNum, sizeof(serialNum)),
         "Unexpected value\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS7_SIGNER_INFO,
     PKCSSignerWithHashAlgo, sizeof(PKCSSignerWithHashAlgo),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    if (ret)
    {
        info = (CMSG_SIGNER_INFO *)buf;
        ok(info->dwVersion == 0, "Expected version 0, got %d\n",
         info->dwVersion);
        ok(info->Issuer.cbData == sizeof(encodedCommonNameNoNull),
         "Unexpected size %d\n", info->Issuer.cbData);
        ok(!memcmp(info->Issuer.pbData, encodedCommonNameNoNull,
         info->Issuer.cbData), "Unexpected value\n");
        ok(info->SerialNumber.cbData == sizeof(serialNum),
         "Unexpected size %d\n", info->SerialNumber.cbData);
        ok(!memcmp(info->SerialNumber.pbData, serialNum, sizeof(serialNum)),
         "Unexpected value\n");
        ok(!strcmp(info->HashAlgorithm.pszObjId, "1.2.3"),
         "Expected 1.2.3, got %s\n", info->HashAlgorithm.pszObjId);
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS7_SIGNER_INFO,
     PKCSSignerWithHashAndEncryptionAlgo,
     sizeof(PKCSSignerWithHashAndEncryptionAlgo), CRYPT_DECODE_ALLOC_FLAG,
     NULL, &buf, &size);
    if (ret)
    {
        info = (CMSG_SIGNER_INFO *)buf;
        ok(info->dwVersion == 0, "Expected version 0, got %d\n",
         info->dwVersion);
        ok(info->Issuer.cbData == sizeof(encodedCommonNameNoNull),
         "Unexpected size %d\n", info->Issuer.cbData);
        ok(!memcmp(info->Issuer.pbData, encodedCommonNameNoNull,
         info->Issuer.cbData), "Unexpected value\n");
        ok(info->SerialNumber.cbData == sizeof(serialNum),
         "Unexpected size %d\n", info->SerialNumber.cbData);
        ok(!memcmp(info->SerialNumber.pbData, serialNum, sizeof(serialNum)),
         "Unexpected value\n");
        ok(!strcmp(info->HashAlgorithm.pszObjId, "1.2.3"),
         "Expected 1.2.3, got %s\n", info->HashAlgorithm.pszObjId);
        ok(!strcmp(info->HashEncryptionAlgorithm.pszObjId, "1.5.6"),
         "Expected 1.5.6, got %s\n", info->HashEncryptionAlgorithm.pszObjId);
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS7_SIGNER_INFO,
     PKCSSignerWithHash, sizeof(PKCSSignerWithHash),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    if (ret)
    {
        info = (CMSG_SIGNER_INFO *)buf;
        ok(info->dwVersion == 0, "Expected version 0, got %d\n",
         info->dwVersion);
        ok(info->Issuer.cbData == sizeof(encodedCommonNameNoNull),
         "Unexpected size %d\n", info->Issuer.cbData);
        ok(!memcmp(info->Issuer.pbData, encodedCommonNameNoNull,
         info->Issuer.cbData), "Unexpected value\n");
        ok(info->SerialNumber.cbData == sizeof(serialNum),
         "Unexpected size %d\n", info->SerialNumber.cbData);
        ok(!memcmp(info->SerialNumber.pbData, serialNum, sizeof(serialNum)),
         "Unexpected value\n");
        ok(!strcmp(info->HashAlgorithm.pszObjId, "1.2.3"),
         "Expected 1.2.3, got %s\n", info->HashAlgorithm.pszObjId);
        ok(!strcmp(info->HashEncryptionAlgorithm.pszObjId, "1.5.6"),
         "Expected 1.5.6, got %s\n", info->HashEncryptionAlgorithm.pszObjId);
        ok(info->EncryptedHash.cbData == sizeof(hash), "Unexpected size %d\n",
         info->EncryptedHash.cbData);
        ok(!memcmp(info->EncryptedHash.pbData, hash, sizeof(hash)),
         "Unexpected value\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS7_SIGNER_INFO,
     PKCSSignerWithAuthAttr, sizeof(PKCSSignerWithAuthAttr),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    if (ret)
    {
        info = (CMSG_SIGNER_INFO *)buf;
        ok(info->AuthAttrs.cAttr == 1, "Expected 1 attribute, got %d\n",
         info->AuthAttrs.cAttr);
        ok(!strcmp(info->AuthAttrs.rgAttr[0].pszObjId, szOID_COMMON_NAME),
         "Expected %s, got %s\n", szOID_COMMON_NAME,
         info->AuthAttrs.rgAttr[0].pszObjId);
        ok(info->AuthAttrs.rgAttr[0].cValue == 1, "Expected 1 value, got %d\n",
         info->AuthAttrs.rgAttr[0].cValue);
        ok(info->AuthAttrs.rgAttr[0].rgValue[0].cbData ==
         sizeof(encodedCommonName), "Unexpected size %d\n",
         info->AuthAttrs.rgAttr[0].rgValue[0].cbData);
        ok(!memcmp(info->AuthAttrs.rgAttr[0].rgValue[0].pbData,
         encodedCommonName, sizeof(encodedCommonName)), "Unexpected value\n");
        LocalFree(buf);
    }
}

static const BYTE CMSSignerWithKeyId[] = {
0x30,0x14,0x02,0x01,0x00,0x80,0x01,0x01,0x30,0x04,0x06,0x00,0x05,0x00,0x30,
0x04,0x06,0x00,0x05,0x00,0x04,0x00 };

static void test_encodeCMSSignerInfo(DWORD dwEncoding)
{
    BOOL ret;
    LPBYTE buf = NULL;
    DWORD size = 0;
    CMSG_CMS_SIGNER_INFO info = { 0 };
    static char oid1[] = "1.2.3", oid2[] = "1.5.6";

    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(dwEncoding, CMS_SIGNER_INFO, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret, "Expected failure, got %d\n", ret);
    if (!ret && GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        skip("no CMS_SIGNER_INFO encode support\n");
        return;
    }
    ok(GetLastError() == E_INVALIDARG,
       "Expected E_INVALIDARG, got %08x\n", GetLastError());
    info.SignerId.dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;
    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(dwEncoding, CMS_SIGNER_INFO, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret, "Expected failure, got %d\n", ret);
    if (!ret && GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        skip("no CMS_SIGNER_INFO encode support\n");
        return;
    }
    ok(GetLastError() == E_INVALIDARG,
       "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* To be encoded, a signer must have a valid cert ID, where a valid ID may
     * be a key id or an issuer serial number with at least the issuer set, and
     * the encoding must include PKCS_7_ASN_ENCODING.
     * (That isn't enough to be decoded, see decoding tests.)
     */
    U(info.SignerId).IssuerSerialNumber.Issuer.cbData =
     sizeof(encodedCommonNameNoNull);
    U(info.SignerId).IssuerSerialNumber.Issuer.pbData = encodedCommonNameNoNull;
    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(dwEncoding, CMS_SIGNER_INFO, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (!(dwEncoding & PKCS_7_ASN_ENCODING))
        ok(!ret && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08x\n", GetLastError());
    else
    {
        ok(ret, "CryptEncodeObjectEx failed: %x\n", GetLastError());
        if (ret)
        {
            ok(size == sizeof(minimalPKCSSigner), "Unexpected size %d\n", size);
            ok(!memcmp(buf, minimalPKCSSigner, size), "Unexpected value\n");
            LocalFree(buf);
        }
    }
    U(info.SignerId).IssuerSerialNumber.SerialNumber.cbData = sizeof(serialNum);
    U(info.SignerId).IssuerSerialNumber.SerialNumber.pbData = (BYTE *)serialNum;
    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(dwEncoding, CMS_SIGNER_INFO, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (!(dwEncoding & PKCS_7_ASN_ENCODING))
        ok(!ret && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08x\n", GetLastError());
    else
    {
        ok(ret, "CryptEncodeObjectEx failed: %x\n", GetLastError());
        if (ret)
        {
            ok(size == sizeof(PKCSSignerWithSerial), "Unexpected size %d\n",
             size);
            ok(!memcmp(buf, PKCSSignerWithSerial, size), "Unexpected value\n");
            LocalFree(buf);
        }
    }
    info.SignerId.dwIdChoice = CERT_ID_KEY_IDENTIFIER;
    U(info.SignerId).KeyId.cbData = sizeof(serialNum);
    U(info.SignerId).KeyId.pbData = (BYTE *)serialNum;
    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(dwEncoding, CMS_SIGNER_INFO, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (!(dwEncoding & PKCS_7_ASN_ENCODING))
        ok(!ret && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08x\n", GetLastError());
    else
    {
        ok(ret, "CryptEncodeObjectEx failed: %x\n", GetLastError());
        if (ret)
        {
            ok(size == sizeof(CMSSignerWithKeyId), "Unexpected size %d\n",
             size);
            ok(!memcmp(buf, CMSSignerWithKeyId, size), "Unexpected value\n");
            LocalFree(buf);
        }
    }
    /* While a CERT_ID can have a hash type, that's not allowed in CMS, where
     * only the IssuerAndSerialNumber and SubjectKeyIdentifier types are allowed
     * (see RFC 3852, section 5.3.)
     */
    info.SignerId.dwIdChoice = CERT_ID_SHA1_HASH;
    U(info.SignerId).HashId.cbData = sizeof(hash);
    U(info.SignerId).HashId.pbData = (BYTE *)hash;
    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(dwEncoding, CMS_SIGNER_INFO, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* Now with a hash algo */
    info.SignerId.dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;
    U(info.SignerId).IssuerSerialNumber.Issuer.cbData =
     sizeof(encodedCommonNameNoNull);
    U(info.SignerId).IssuerSerialNumber.Issuer.pbData = encodedCommonNameNoNull;
    info.HashAlgorithm.pszObjId = oid1;
    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(dwEncoding, CMS_SIGNER_INFO, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (!(dwEncoding & PKCS_7_ASN_ENCODING))
        ok(!ret && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08x\n", GetLastError());
    else
    {
        ok(ret, "CryptEncodeObjectEx failed: %x\n", GetLastError());
        if (ret)
        {
            ok(size == sizeof(PKCSSignerWithHashAlgo), "Unexpected size %d\n",
             size);
            ok(!memcmp(buf, PKCSSignerWithHashAlgo, size),
             "Unexpected value\n");
            LocalFree(buf);
        }
    }
    info.HashEncryptionAlgorithm.pszObjId = oid2;
    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(dwEncoding, CMS_SIGNER_INFO, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (!(dwEncoding & PKCS_7_ASN_ENCODING))
        ok(!ret && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08x\n", GetLastError());
    else
    {
        ok(ret, "CryptEncodeObjectEx failed: %x\n", GetLastError());
        if (ret)
        {
            ok(size == sizeof(PKCSSignerWithHashAndEncryptionAlgo),
             "Unexpected size %d\n", size);
            ok(!memcmp(buf, PKCSSignerWithHashAndEncryptionAlgo, size),
             "Unexpected value\n");
            LocalFree(buf);
        }
    }
    info.EncryptedHash.cbData = sizeof(hash);
    info.EncryptedHash.pbData = (BYTE *)hash;
    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(dwEncoding, CMS_SIGNER_INFO, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (!(dwEncoding & PKCS_7_ASN_ENCODING))
        ok(!ret && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08x\n", GetLastError());
    else
    {
        ok(ret, "CryptEncodeObjectEx failed: %x\n", GetLastError());
        if (ret)
        {
            ok(size == sizeof(PKCSSignerWithHash), "Unexpected size %d\n",
             size);
            ok(!memcmp(buf, PKCSSignerWithHash, size), "Unexpected value\n");
            LocalFree(buf);
        }
    }
}

static void test_decodeCMSSignerInfo(DWORD dwEncoding)
{
    BOOL ret;
    LPBYTE buf = NULL;
    DWORD size = 0;
    CMSG_CMS_SIGNER_INFO *info;
    static const char oid1[] = "1.2.3", oid2[] = "1.5.6";

    /* A CMS signer can't be decoded without a serial number. */
    SetLastError(0xdeadbeef);
    ret = pCryptDecodeObjectEx(dwEncoding, CMS_SIGNER_INFO,
     minimalPKCSSigner, sizeof(minimalPKCSSigner),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret, "expected failure\n");
    if (!ret && GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        skip("no CMS_SIGNER_INFO decode support\n");
        return;
    }
    ok(GetLastError() == CRYPT_E_ASN1_CORRUPT,
     "Expected CRYPT_E_ASN1_CORRUPT, got %x\n", GetLastError());
    ret = pCryptDecodeObjectEx(dwEncoding, CMS_SIGNER_INFO,
     PKCSSignerWithSerial, sizeof(PKCSSignerWithSerial),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %x\n", GetLastError());
    if (ret)
    {
        info = (CMSG_CMS_SIGNER_INFO *)buf;
        ok(info->dwVersion == 0, "Expected version 0, got %d\n",
         info->dwVersion);
        ok(info->SignerId.dwIdChoice == CERT_ID_ISSUER_SERIAL_NUMBER,
         "Expected CERT_ID_ISSUER_SERIAL_NUMBER, got %d\n",
         info->SignerId.dwIdChoice);
        ok(U(info->SignerId).IssuerSerialNumber.Issuer.cbData ==
         sizeof(encodedCommonNameNoNull), "Unexpected size %d\n",
         U(info->SignerId).IssuerSerialNumber.Issuer.cbData);
        ok(!memcmp(U(info->SignerId).IssuerSerialNumber.Issuer.pbData,
         encodedCommonNameNoNull,
         U(info->SignerId).IssuerSerialNumber.Issuer.cbData),
         "Unexpected value\n");
        ok(U(info->SignerId).IssuerSerialNumber.SerialNumber.cbData ==
         sizeof(serialNum), "Unexpected size %d\n",
         U(info->SignerId).IssuerSerialNumber.SerialNumber.cbData);
        ok(!memcmp(U(info->SignerId).IssuerSerialNumber.SerialNumber.pbData,
         serialNum, sizeof(serialNum)), "Unexpected value\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, CMS_SIGNER_INFO,
     PKCSSignerWithHashAlgo, sizeof(PKCSSignerWithHashAlgo),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %x\n", GetLastError());
    if (ret)
    {
        info = (CMSG_CMS_SIGNER_INFO *)buf;
        ok(info->dwVersion == 0, "Expected version 0, got %d\n",
         info->dwVersion);
        ok(info->SignerId.dwIdChoice == CERT_ID_ISSUER_SERIAL_NUMBER,
         "Expected CERT_ID_ISSUER_SERIAL_NUMBER, got %d\n",
         info->SignerId.dwIdChoice);
        ok(U(info->SignerId).IssuerSerialNumber.Issuer.cbData ==
         sizeof(encodedCommonNameNoNull), "Unexpected size %d\n",
         U(info->SignerId).IssuerSerialNumber.Issuer.cbData);
        ok(!memcmp(U(info->SignerId).IssuerSerialNumber.Issuer.pbData,
         encodedCommonNameNoNull,
         U(info->SignerId).IssuerSerialNumber.Issuer.cbData),
         "Unexpected value\n");
        ok(U(info->SignerId).IssuerSerialNumber.SerialNumber.cbData ==
         sizeof(serialNum), "Unexpected size %d\n",
         U(info->SignerId).IssuerSerialNumber.SerialNumber.cbData);
        ok(!memcmp(U(info->SignerId).IssuerSerialNumber.SerialNumber.pbData,
         serialNum, sizeof(serialNum)), "Unexpected value\n");
        ok(!strcmp(info->HashAlgorithm.pszObjId, oid1),
         "Expected %s, got %s\n", oid1, info->HashAlgorithm.pszObjId);
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, CMS_SIGNER_INFO,
     PKCSSignerWithHashAndEncryptionAlgo,
     sizeof(PKCSSignerWithHashAndEncryptionAlgo), CRYPT_DECODE_ALLOC_FLAG,
     NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %x\n", GetLastError());
    if (ret)
    {
        info = (CMSG_CMS_SIGNER_INFO *)buf;
        ok(info->dwVersion == 0, "Expected version 0, got %d\n",
         info->dwVersion);
        ok(info->SignerId.dwIdChoice == CERT_ID_ISSUER_SERIAL_NUMBER,
         "Expected CERT_ID_ISSUER_SERIAL_NUMBER, got %d\n",
         info->SignerId.dwIdChoice);
        ok(U(info->SignerId).IssuerSerialNumber.Issuer.cbData ==
         sizeof(encodedCommonNameNoNull), "Unexpected size %d\n",
         U(info->SignerId).IssuerSerialNumber.Issuer.cbData);
        ok(!memcmp(U(info->SignerId).IssuerSerialNumber.Issuer.pbData,
         encodedCommonNameNoNull,
         U(info->SignerId).IssuerSerialNumber.Issuer.cbData),
         "Unexpected value\n");
        ok(U(info->SignerId).IssuerSerialNumber.SerialNumber.cbData ==
         sizeof(serialNum), "Unexpected size %d\n",
         U(info->SignerId).IssuerSerialNumber.SerialNumber.cbData);
        ok(!memcmp(U(info->SignerId).IssuerSerialNumber.SerialNumber.pbData,
         serialNum, sizeof(serialNum)), "Unexpected value\n");
        ok(!strcmp(info->HashAlgorithm.pszObjId, oid1),
         "Expected %s, got %s\n", oid1, info->HashAlgorithm.pszObjId);
        ok(!strcmp(info->HashEncryptionAlgorithm.pszObjId, oid2),
         "Expected %s, got %s\n", oid2, info->HashEncryptionAlgorithm.pszObjId);
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, CMS_SIGNER_INFO,
     PKCSSignerWithHash, sizeof(PKCSSignerWithHash),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %x\n", GetLastError());
    if (ret)
    {
        info = (CMSG_CMS_SIGNER_INFO *)buf;
        ok(info->dwVersion == 0, "Expected version 0, got %d\n",
         info->dwVersion);
        ok(info->SignerId.dwIdChoice == CERT_ID_ISSUER_SERIAL_NUMBER,
         "Expected CERT_ID_ISSUER_SERIAL_NUMBER, got %d\n",
         info->SignerId.dwIdChoice);
        ok(U(info->SignerId).IssuerSerialNumber.Issuer.cbData ==
         sizeof(encodedCommonNameNoNull), "Unexpected size %d\n",
         U(info->SignerId).IssuerSerialNumber.Issuer.cbData);
        ok(!memcmp(U(info->SignerId).IssuerSerialNumber.Issuer.pbData,
         encodedCommonNameNoNull,
         U(info->SignerId).IssuerSerialNumber.Issuer.cbData),
         "Unexpected value\n");
        ok(U(info->SignerId).IssuerSerialNumber.SerialNumber.cbData ==
         sizeof(serialNum), "Unexpected size %d\n",
         U(info->SignerId).IssuerSerialNumber.SerialNumber.cbData);
        ok(!memcmp(U(info->SignerId).IssuerSerialNumber.SerialNumber.pbData,
         serialNum, sizeof(serialNum)), "Unexpected value\n");
        ok(!strcmp(info->HashAlgorithm.pszObjId, oid1),
         "Expected %s, got %s\n", oid1, info->HashAlgorithm.pszObjId);
        ok(!strcmp(info->HashEncryptionAlgorithm.pszObjId, oid2),
         "Expected %s, got %s\n", oid2, info->HashEncryptionAlgorithm.pszObjId);
        ok(info->EncryptedHash.cbData == sizeof(hash), "Unexpected size %d\n",
         info->EncryptedHash.cbData);
        ok(!memcmp(info->EncryptedHash.pbData, hash, sizeof(hash)),
         "Unexpected value\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, CMS_SIGNER_INFO,
     CMSSignerWithKeyId, sizeof(CMSSignerWithKeyId),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %x\n", GetLastError());
    if (ret)
    {
        info = (CMSG_CMS_SIGNER_INFO *)buf;
        ok(info->dwVersion == 0, "Expected version 0, got %d\n",
         info->dwVersion);
        ok(info->SignerId.dwIdChoice == CERT_ID_KEY_IDENTIFIER,
         "Expected CERT_ID_KEY_IDENTIFIER, got %d\n",
         info->SignerId.dwIdChoice);
        ok(U(info->SignerId).KeyId.cbData == sizeof(serialNum),
         "Unexpected size %d\n", U(info->SignerId).KeyId.cbData);
        ok(!memcmp(U(info->SignerId).KeyId.pbData, serialNum, sizeof(serialNum)),
         "Unexpected value\n");
        LocalFree(buf);
    }
}

static BYTE emptyDNSPermittedConstraints[] = {
0x30,0x06,0xa0,0x04,0x30,0x02,0x82,0x00 };
static BYTE emptyDNSExcludedConstraints[] = {
0x30,0x06,0xa1,0x04,0x30,0x02,0x82,0x00 };
static BYTE DNSExcludedConstraints[] = {
0x30,0x17,0xa1,0x15,0x30,0x13,0x82,0x11,0x68,0x74,0x74,0x70,0x3a,0x2f,0x2f,
0x77,0x69,0x6e,0x65,0x68,0x71,0x2e,0x6f,0x72,0x67 };
static BYTE permittedAndExcludedConstraints[] = {
0x30,0x25,0xa0,0x0c,0x30,0x0a,0x87,0x08,0x30,0x06,0x87,0x04,0x7f,0x00,0x00,
0x01,0xa1,0x15,0x30,0x13,0x82,0x11,0x68,0x74,0x74,0x70,0x3a,0x2f,0x2f,0x77,
0x69,0x6e,0x65,0x68,0x71,0x2e,0x6f,0x72,0x67 };
static BYTE permittedAndExcludedWithMinConstraints[] = {
0x30,0x28,0xa0,0x0f,0x30,0x0d,0x87,0x08,0x30,0x06,0x87,0x04,0x7f,0x00,0x00,
0x01,0x80,0x01,0x05,0xa1,0x15,0x30,0x13,0x82,0x11,0x68,0x74,0x74,0x70,0x3a,
0x2f,0x2f,0x77,0x69,0x6e,0x65,0x68,0x71,0x2e,0x6f,0x72,0x67 };
static BYTE permittedAndExcludedWithMinMaxConstraints[] = {
0x30,0x2b,0xa0,0x12,0x30,0x10,0x87,0x08,0x30,0x06,0x87,0x04,0x7f,0x00,0x00,
0x01,0x80,0x01,0x05,0x81,0x01,0x03,0xa1,0x15,0x30,0x13,0x82,0x11,0x68,0x74,
0x74,0x70,0x3a,0x2f,0x2f,0x77,0x69,0x6e,0x65,0x68,0x71,0x2e,0x6f,0x72,0x67 };

static void test_encodeNameConstraints(DWORD dwEncoding)
{
    BOOL ret;
    CERT_NAME_CONSTRAINTS_INFO constraints = { 0 };
    CERT_GENERAL_SUBTREE permitted = { { 0 } };
    CERT_GENERAL_SUBTREE excluded = { { 0 } };
    LPBYTE buf;
    DWORD size;

    ret = pCryptEncodeObjectEx(dwEncoding, X509_NAME_CONSTRAINTS, &constraints,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (!ret && GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        skip("no X509_NAME_CONSTRAINTS encode support\n");
        return;
    }
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptySequence), "Unexpected size\n");
        ok(!memcmp(buf, emptySequence, size), "Unexpected value\n");
        LocalFree(buf);
    }
    constraints.cPermittedSubtree = 1;
    constraints.rgPermittedSubtree = &permitted;
    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(dwEncoding, X509_NAME_CONSTRAINTS, &constraints,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    permitted.Base.dwAltNameChoice = CERT_ALT_NAME_DNS_NAME;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_NAME_CONSTRAINTS, &constraints,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptyDNSPermittedConstraints), "Unexpected size\n");
        ok(!memcmp(buf, emptyDNSPermittedConstraints, size),
         "Unexpected value\n");
        LocalFree(buf);
    }
    constraints.cPermittedSubtree = 0;
    constraints.cExcludedSubtree = 1;
    constraints.rgExcludedSubtree = &excluded;
    excluded.Base.dwAltNameChoice = CERT_ALT_NAME_DNS_NAME;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_NAME_CONSTRAINTS, &constraints,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptyDNSExcludedConstraints), "Unexpected size\n");
        ok(!memcmp(buf, emptyDNSExcludedConstraints, size),
         "Unexpected value\n");
        LocalFree(buf);
    }
    U(excluded.Base).pwszURL = (LPWSTR)url;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_NAME_CONSTRAINTS, &constraints,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(DNSExcludedConstraints), "Unexpected size\n");
        ok(!memcmp(buf, DNSExcludedConstraints, size),
         "Unexpected value\n");
        LocalFree(buf);
    }
    permitted.Base.dwAltNameChoice = CERT_ALT_NAME_IP_ADDRESS;
    U(permitted.Base).IPAddress.cbData = sizeof(encodedIPAddr);
    U(permitted.Base).IPAddress.pbData = (LPBYTE)encodedIPAddr;
    constraints.cPermittedSubtree = 1;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_NAME_CONSTRAINTS, &constraints,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(permittedAndExcludedConstraints),
         "Unexpected size\n");
        ok(!memcmp(buf, permittedAndExcludedConstraints, size),
         "Unexpected value\n");
        LocalFree(buf);
    }
    permitted.dwMinimum = 5;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_NAME_CONSTRAINTS, &constraints,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(permittedAndExcludedWithMinConstraints),
         "Unexpected size\n");
        ok(!memcmp(buf, permittedAndExcludedWithMinConstraints, size),
         "Unexpected value\n");
        LocalFree(buf);
    }
    permitted.fMaximum = TRUE;
    permitted.dwMaximum = 3;
    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(dwEncoding, X509_NAME_CONSTRAINTS, &constraints,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(permittedAndExcludedWithMinMaxConstraints),
         "Unexpected size\n");
        ok(!memcmp(buf, permittedAndExcludedWithMinMaxConstraints, size),
         "Unexpected value\n");
        LocalFree(buf);
    }
}

struct EncodedNameConstraints
{
    CRYPT_DATA_BLOB            encoded;
    CERT_NAME_CONSTRAINTS_INFO constraints;
};

static CERT_GENERAL_SUBTREE emptyDNSSubtree = {
 { CERT_ALT_NAME_DNS_NAME, { 0 } }, 0 };
static CERT_GENERAL_SUBTREE DNSSubtree = {
 { CERT_ALT_NAME_DNS_NAME, { 0 } }, 0 };
static CERT_GENERAL_SUBTREE IPAddressSubtree = {
 { CERT_ALT_NAME_IP_ADDRESS, { 0 } }, 0 };
static CERT_GENERAL_SUBTREE IPAddressWithMinSubtree = {
 { CERT_ALT_NAME_IP_ADDRESS, { 0 } }, 5, 0 };
static CERT_GENERAL_SUBTREE IPAddressWithMinMaxSubtree = {
 { CERT_ALT_NAME_IP_ADDRESS, { 0 } }, 5, TRUE, 3 };

static const struct EncodedNameConstraints encodedNameConstraints[] = {
 { { sizeof(emptySequence), (LPBYTE)emptySequence }, { 0 } },
 { { sizeof(emptyDNSPermittedConstraints), emptyDNSPermittedConstraints },
   { 1, &emptyDNSSubtree, 0, NULL } },
 { { sizeof(emptyDNSExcludedConstraints), emptyDNSExcludedConstraints },
   { 0, NULL, 1, &emptyDNSSubtree } },
 { { sizeof(DNSExcludedConstraints), DNSExcludedConstraints },
   { 0, NULL, 1, &DNSSubtree } },
 { { sizeof(permittedAndExcludedConstraints), permittedAndExcludedConstraints },
   { 1, &IPAddressSubtree, 1, &DNSSubtree } },
 { { sizeof(permittedAndExcludedWithMinConstraints),
     permittedAndExcludedWithMinConstraints },
   { 1, &IPAddressWithMinSubtree, 1, &DNSSubtree } },
 { { sizeof(permittedAndExcludedWithMinMaxConstraints),
     permittedAndExcludedWithMinMaxConstraints },
   { 1, &IPAddressWithMinMaxSubtree, 1, &DNSSubtree } },
};

static void test_decodeNameConstraints(DWORD dwEncoding)
{
    BOOL ret;
    DWORD i;
    CERT_NAME_CONSTRAINTS_INFO *constraints;

    U(DNSSubtree.Base).pwszURL = (LPWSTR)url;
    U(IPAddressSubtree.Base).IPAddress.cbData = sizeof(encodedIPAddr);
    U(IPAddressSubtree.Base).IPAddress.pbData = (LPBYTE)encodedIPAddr;
    U(IPAddressWithMinSubtree.Base).IPAddress.cbData = sizeof(encodedIPAddr);
    U(IPAddressWithMinSubtree.Base).IPAddress.pbData = (LPBYTE)encodedIPAddr;
    U(IPAddressWithMinMaxSubtree.Base).IPAddress.cbData = sizeof(encodedIPAddr);
    U(IPAddressWithMinMaxSubtree.Base).IPAddress.pbData = (LPBYTE)encodedIPAddr;
    for (i = 0;
     i < sizeof(encodedNameConstraints) / sizeof(encodedNameConstraints[0]);
     i++)
    {
        DWORD size;

        ret = pCryptDecodeObjectEx(dwEncoding, X509_NAME_CONSTRAINTS,
         encodedNameConstraints[i].encoded.pbData,
         encodedNameConstraints[i].encoded.cbData,
         CRYPT_DECODE_ALLOC_FLAG, NULL, &constraints, &size);
        if (!ret && GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            skip("no X509_NAME_CONSTRAINTS decode support\n");
            return;
        }
        ok(ret, "%d: CryptDecodeObjectEx failed: %08x\n", i, GetLastError());
        if (ret)
        {
            DWORD j;

            if (constraints->cPermittedSubtree !=
             encodedNameConstraints[i].constraints.cPermittedSubtree)
                fprintf(stderr, "%d: expected %d permitted, got %d\n", i,
                 encodedNameConstraints[i].constraints.cPermittedSubtree,
                 constraints->cPermittedSubtree);
            if (constraints->cPermittedSubtree ==
             encodedNameConstraints[i].constraints.cPermittedSubtree)
            {
                for (j = 0; j < constraints->cPermittedSubtree; j++)
                {
                    compareAltNameEntry(&constraints->rgPermittedSubtree[j].Base,
                     &encodedNameConstraints[i].constraints.rgPermittedSubtree[j].Base);
                }
            }
            if (constraints->cExcludedSubtree !=
             encodedNameConstraints[i].constraints.cExcludedSubtree)
                fprintf(stderr, "%d: expected %d excluded, got %d\n", i,
                 encodedNameConstraints[i].constraints.cExcludedSubtree,
                 constraints->cExcludedSubtree);
            if (constraints->cExcludedSubtree ==
             encodedNameConstraints[i].constraints.cExcludedSubtree)
            {
                for (j = 0; j < constraints->cExcludedSubtree; j++)
                {
                    compareAltNameEntry(&constraints->rgExcludedSubtree[j].Base,
                     &encodedNameConstraints[i].constraints.rgExcludedSubtree[j].Base);
                }
            }
            LocalFree(constraints);
        }
    }
}

static WCHAR noticeText[] = { 'T','h','i','s',' ','i','s',' ','a',' ',
 'n','o','t','i','c','e',0 };
static const BYTE noticeWithDisplayText[] = {
 0x30,0x22,0x1e,0x20,0x00,0x54,0x00,0x68,0x00,0x69,0x00,0x73,0x00,0x20,0x00,
 0x69,0x00,0x73,0x00,0x20,0x00,0x61,0x00,0x20,0x00,0x6e,0x00,0x6f,0x00,0x74,
 0x00,0x69,0x00,0x63,0x00,0x65
};
static char org[] = "Wine";
static int noticeNumbers[] = { 2,3 };
static BYTE noticeWithReference[] = {
 0x30,0x32,0x30,0x0e,0x16,0x04,0x57,0x69,0x6e,0x65,0x30,0x06,0x02,0x01,0x02,
 0x02,0x01,0x03,0x1e,0x20,0x00,0x54,0x00,0x68,0x00,0x69,0x00,0x73,0x00,0x20,
 0x00,0x69,0x00,0x73,0x00,0x20,0x00,0x61,0x00,0x20,0x00,0x6e,0x00,0x6f,0x00,
 0x74,0x00,0x69,0x00,0x63,0x00,0x65
};

static void test_encodePolicyQualifierUserNotice(DWORD dwEncoding)
{
    BOOL ret;
    LPBYTE buf;
    DWORD size;
    CERT_POLICY_QUALIFIER_USER_NOTICE notice;
    CERT_POLICY_QUALIFIER_NOTICE_REFERENCE reference;

    memset(&notice, 0, sizeof(notice));
    ret = pCryptEncodeObjectEx(dwEncoding,
     X509_PKIX_POLICY_QUALIFIER_USERNOTICE, &notice, CRYPT_ENCODE_ALLOC_FLAG,
     NULL, &buf, &size);
    if (!ret && GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        skip("no X509_PKIX_POLICY_QUALIFIER_USERNOTICE encode support\n");
        return;
    }
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(sizeof(emptySequence) == size, "unexpected size %d\n", size);
        ok(!memcmp(buf, emptySequence, size), "unexpected value\n");
        LocalFree(buf);
    }
    notice.pszDisplayText = noticeText;
    ret = pCryptEncodeObjectEx(dwEncoding,
     X509_PKIX_POLICY_QUALIFIER_USERNOTICE, &notice, CRYPT_ENCODE_ALLOC_FLAG,
     NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(sizeof(noticeWithDisplayText) == size, "unexpected size %d\n", size);
        ok(!memcmp(buf, noticeWithDisplayText, size), "unexpected value\n");
        LocalFree(buf);
    }
    reference.pszOrganization = org;
    reference.cNoticeNumbers = 2;
    reference.rgNoticeNumbers = noticeNumbers;
    notice.pNoticeReference = &reference;
    ret = pCryptEncodeObjectEx(dwEncoding,
     X509_PKIX_POLICY_QUALIFIER_USERNOTICE, &notice, CRYPT_ENCODE_ALLOC_FLAG,
     NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(sizeof(noticeWithReference) == size, "unexpected size %d\n", size);
        ok(!memcmp(buf, noticeWithReference, size), "unexpected value\n");
        LocalFree(buf);
    }
}

static void test_decodePolicyQualifierUserNotice(DWORD dwEncoding)
{
    BOOL ret;
    CERT_POLICY_QUALIFIER_USER_NOTICE *notice;
    DWORD size;

    ret = pCryptDecodeObjectEx(dwEncoding,
     X509_PKIX_POLICY_QUALIFIER_USERNOTICE,
     emptySequence, sizeof(emptySequence), CRYPT_DECODE_ALLOC_FLAG, NULL,
     &notice, &size);
    if (!ret && GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        skip("no X509_PKIX_POLICY_QUALIFIER_USERNOTICE decode support\n");
        return;
    }
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(notice->pszDisplayText == NULL, "unexpected display text\n");
        ok(notice->pNoticeReference == NULL, "unexpected notice reference\n");
        LocalFree(notice);
    }
    ret = pCryptDecodeObjectEx(dwEncoding,
     X509_PKIX_POLICY_QUALIFIER_USERNOTICE,
     noticeWithDisplayText, sizeof(noticeWithDisplayText),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &notice, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(!lstrcmpW(notice->pszDisplayText, noticeText),
         "unexpected display text\n");
        ok(notice->pNoticeReference == NULL, "unexpected notice reference\n");
        LocalFree(notice);
    }
    ret = pCryptDecodeObjectEx(dwEncoding,
     X509_PKIX_POLICY_QUALIFIER_USERNOTICE,
     noticeWithReference, sizeof(noticeWithReference),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &notice, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(!lstrcmpW(notice->pszDisplayText, noticeText),
         "unexpected display text\n");
        ok(notice->pNoticeReference != NULL, "expected a notice reference\n");
        if (notice->pNoticeReference)
        {
            ok(!strcmp(notice->pNoticeReference->pszOrganization, org),
             "unexpected organization %s\n",
             notice->pNoticeReference->pszOrganization);
            ok(notice->pNoticeReference->cNoticeNumbers == 2,
             "expected 2 notice numbers, got %d\n",
             notice->pNoticeReference->cNoticeNumbers);
            ok(notice->pNoticeReference->rgNoticeNumbers[0] == noticeNumbers[0],
             "unexpected notice number %d\n",
             notice->pNoticeReference->rgNoticeNumbers[0]);
            ok(notice->pNoticeReference->rgNoticeNumbers[1] == noticeNumbers[1],
             "unexpected notice number %d\n",
             notice->pNoticeReference->rgNoticeNumbers[1]);
        }
        LocalFree(notice);
    }
}

static char oid_any_policy[] = "2.5.29.32.0";
static const BYTE policiesWithAnyPolicy[] = {
 0x30,0x08,0x30,0x06,0x06,0x04,0x55,0x1d,0x20,0x00
};
static char oid1[] = "1.2.3";
static char oid_user_notice[] = "1.3.6.1.5.5.7.2.2";
static const BYTE twoPolicies[] = {
 0x30,0x50,0x30,0x06,0x06,0x04,0x55,0x1d,0x20,0x00,0x30,0x46,0x06,0x02,0x2a,
 0x03,0x30,0x40,0x30,0x3e,0x06,0x08,0x2b,0x06,0x01,0x05,0x05,0x07,0x02,0x02,
 0x30,0x32,0x30,0x0e,0x16,0x04,0x57,0x69,0x6e,0x65,0x30,0x06,0x02,0x01,0x02,
 0x02,0x01,0x03,0x1e,0x20,0x00,0x54,0x00,0x68,0x00,0x69,0x00,0x73,0x00,0x20,
 0x00,0x69,0x00,0x73,0x00,0x20,0x00,0x61,0x00,0x20,0x00,0x6e,0x00,0x6f,0x00,
 0x74,0x00,0x69,0x00,0x63,0x00,0x65
};

static void test_encodeCertPolicies(DWORD dwEncoding)
{
    BOOL ret;
    CERT_POLICIES_INFO info;
    CERT_POLICY_INFO policy[2];
    CERT_POLICY_QUALIFIER_INFO qualifier;
    LPBYTE buf;
    DWORD size;

    memset(&info, 0, sizeof(info));
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT_POLICIES, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(sizeof(emptySequence) == size, "unexpected size %d\n", size);
        ok(!memcmp(buf, emptySequence, size), "unexpected value\n");
        LocalFree(buf);
    }
    memset(policy, 0, sizeof(policy));
    info.cPolicyInfo = 1;
    info.rgPolicyInfo = policy;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT_POLICIES, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && (GetLastError() == E_INVALIDARG ||
     GetLastError() == OSS_LIMITED /* Win9x/NT4 */),
     "expected E_INVALIDARG or OSS_LIMITED, got %08x\n", GetLastError());
    policy[0].pszPolicyIdentifier = oid_any_policy;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT_POLICIES, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(sizeof(policiesWithAnyPolicy) == size, "unexpected size %d\n", size);
        ok(!memcmp(buf, policiesWithAnyPolicy, size), "unexpected value\n");
        LocalFree(buf);
    }
    policy[1].pszPolicyIdentifier = oid1;
    memset(&qualifier, 0, sizeof(qualifier));
    qualifier.pszPolicyQualifierId = oid_user_notice;
    qualifier.Qualifier.cbData = sizeof(noticeWithReference);
    qualifier.Qualifier.pbData = noticeWithReference;
    policy[1].cPolicyQualifier = 1;
    policy[1].rgPolicyQualifier = &qualifier;
    info.cPolicyInfo = 2;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_CERT_POLICIES, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(sizeof(twoPolicies) == size, "unexpected size %d\n", size);
        ok(!memcmp(buf, twoPolicies, size), "unexpected value\n");
        LocalFree(buf);
    }
}

static void test_decodeCertPolicies(DWORD dwEncoding)
{
    BOOL ret;
    CERT_POLICIES_INFO *info;
    DWORD size;

    ret = pCryptDecodeObjectEx(dwEncoding, X509_CERT_POLICIES,
     emptySequence, sizeof(emptySequence), CRYPT_DECODE_ALLOC_FLAG, NULL,
     &info, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(info->cPolicyInfo == 0, "unexpected policy info %d\n",
         info->cPolicyInfo);
        LocalFree(info);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_CERT_POLICIES,
     policiesWithAnyPolicy, sizeof(policiesWithAnyPolicy),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &info, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(info->cPolicyInfo == 1, "unexpected policy info %d\n",
         info->cPolicyInfo);
        ok(!strcmp(info->rgPolicyInfo[0].pszPolicyIdentifier, oid_any_policy),
         "unexpected policy id %s\n",
         info->rgPolicyInfo[0].pszPolicyIdentifier);
        ok(info->rgPolicyInfo[0].cPolicyQualifier == 0,
         "unexpected policy qualifier count %d\n",
         info->rgPolicyInfo[0].cPolicyQualifier);
        LocalFree(info);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_CERT_POLICIES,
     twoPolicies, sizeof(twoPolicies),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &info, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(info->cPolicyInfo == 2, "unexpected policy info %d\n",
         info->cPolicyInfo);
        ok(!strcmp(info->rgPolicyInfo[0].pszPolicyIdentifier, oid_any_policy),
         "unexpected policy id %s\n",
         info->rgPolicyInfo[0].pszPolicyIdentifier);
        ok(info->rgPolicyInfo[0].cPolicyQualifier == 0,
         "unexpected policy qualifier count %d\n",
         info->rgPolicyInfo[0].cPolicyQualifier);
        ok(!strcmp(info->rgPolicyInfo[1].pszPolicyIdentifier, oid1),
         "unexpected policy id %s\n",
         info->rgPolicyInfo[1].pszPolicyIdentifier);
        ok(info->rgPolicyInfo[1].cPolicyQualifier == 1,
         "unexpected policy qualifier count %d\n",
         info->rgPolicyInfo[1].cPolicyQualifier);
        ok(!strcmp(
         info->rgPolicyInfo[1].rgPolicyQualifier[0].pszPolicyQualifierId,
         oid_user_notice), "unexpected policy qualifier id %s\n",
         info->rgPolicyInfo[1].rgPolicyQualifier[0].pszPolicyQualifierId);
        ok(info->rgPolicyInfo[1].rgPolicyQualifier[0].Qualifier.cbData ==
         sizeof(noticeWithReference), "unexpected qualifier size %d\n",
         info->rgPolicyInfo[1].rgPolicyQualifier[0].Qualifier.cbData);
        ok(!memcmp(
         info->rgPolicyInfo[1].rgPolicyQualifier[0].Qualifier.pbData,
         noticeWithReference, sizeof(noticeWithReference)),
         "unexpected qualifier value\n");
        LocalFree(info);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_CERT_POLICIES,
     twoPolicies, sizeof(twoPolicies), 0, NULL, NULL, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    info = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    if (info)
    {
        ret = pCryptDecodeObjectEx(dwEncoding, X509_CERT_POLICIES,
         twoPolicies, sizeof(twoPolicies), 0, NULL, info, &size);
        ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
        HeapFree(GetProcessHeap(), 0, info);
    }
}

static const BYTE policyMappingWithOneMapping[] = {
0x30,0x0a,0x30,0x08,0x06,0x02,0x2a,0x03,0x06,0x02,0x53,0x04 };
static const BYTE policyMappingWithTwoMappings[] = {
0x30,0x14,0x30,0x08,0x06,0x02,0x2a,0x03,0x06,0x02,0x53,0x04,0x30,0x08,0x06,
0x02,0x2b,0x04,0x06,0x02,0x55,0x06 };
static const LPCSTR mappingOids[] = { X509_POLICY_MAPPINGS,
 szOID_POLICY_MAPPINGS, szOID_LEGACY_POLICY_MAPPINGS };

static void test_encodeCertPolicyMappings(DWORD dwEncoding)
{
    static char oid2[] = "2.3.4";
    static char oid3[] = "1.3.4";
    static char oid4[] = "2.5.6";
    BOOL ret;
    CERT_POLICY_MAPPINGS_INFO info = { 0 };
    CERT_POLICY_MAPPING mapping[2];
    LPBYTE buf;
    DWORD size, i;

    /* Each of the mapping OIDs is equivalent, so check with all of them */
    for (i = 0; i < sizeof(mappingOids) / sizeof(mappingOids[0]); i++)
    {
        memset(&info, 0, sizeof(info));
        ret = pCryptEncodeObjectEx(dwEncoding, mappingOids[i], &info,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
        ok(ret || broken(GetLastError() == ERROR_FILE_NOT_FOUND),
         "CryptEncodeObjectEx failed: %08x\n", GetLastError());
        if (!ret && GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            win_skip("no policy mappings support\n");
            return;
        }
        if (ret)
        {
            ok(size == sizeof(emptySequence), "unexpected size %d\n", size);
            ok(!memcmp(buf, emptySequence, sizeof(emptySequence)),
             "unexpected value\n");
            LocalFree(buf);
        }
        mapping[0].pszIssuerDomainPolicy = NULL;
        mapping[0].pszSubjectDomainPolicy = NULL;
        info.cPolicyMapping = 1;
        info.rgPolicyMapping = mapping;
        SetLastError(0xdeadbeef);
        ret = pCryptEncodeObjectEx(dwEncoding, mappingOids[i], &info,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
        ok(!ret && GetLastError() == E_INVALIDARG,
         "expected E_INVALIDARG, got %08x\n", GetLastError());
        mapping[0].pszIssuerDomainPolicy = oid1;
        mapping[0].pszSubjectDomainPolicy = oid2;
        ret = pCryptEncodeObjectEx(dwEncoding, mappingOids[i], &info,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
        ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
        if (ret)
        {
            ok(size == sizeof(policyMappingWithOneMapping),
             "unexpected size %d\n", size);
            ok(!memcmp(buf, policyMappingWithOneMapping, size),
             "unexpected value\n");
            LocalFree(buf);
        }
        mapping[1].pszIssuerDomainPolicy = oid3;
        mapping[1].pszSubjectDomainPolicy = oid4;
        info.cPolicyMapping = 2;
        ret = pCryptEncodeObjectEx(dwEncoding, mappingOids[i], &info,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
        ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
        if (ret)
        {
            ok(size == sizeof(policyMappingWithTwoMappings),
             "unexpected size %d\n", size);
            ok(!memcmp(buf, policyMappingWithTwoMappings, size),
             "unexpected value\n");
            LocalFree(buf);
        }
    }
}

static void test_decodeCertPolicyMappings(DWORD dwEncoding)
{
    DWORD size, i;
    CERT_POLICY_MAPPINGS_INFO *info;
    BOOL ret;

    /* Each of the mapping OIDs is equivalent, so check with all of them */
    for (i = 0; i < sizeof(mappingOids) / sizeof(mappingOids[0]); i++)
    {
        ret = pCryptDecodeObjectEx(dwEncoding, mappingOids[i],
         emptySequence, sizeof(emptySequence), CRYPT_DECODE_ALLOC_FLAG, NULL,
         &info, &size);
        ok(ret || broken(GetLastError() == ERROR_FILE_NOT_FOUND),
         "CryptDecodeObjectEx failed: %08x\n", GetLastError());
        if (!ret && GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            win_skip("no policy mappings support\n");
            return;
        }
        if (ret)
        {
            ok(info->cPolicyMapping == 0,
             "expected 0 policy mappings, got %d\n", info->cPolicyMapping);
            LocalFree(info);
        }
        ret = pCryptDecodeObjectEx(dwEncoding, mappingOids[i],
         policyMappingWithOneMapping, sizeof(policyMappingWithOneMapping),
         CRYPT_DECODE_ALLOC_FLAG, NULL, &info, &size);
        ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
        if (ret)
        {
            ok(info->cPolicyMapping == 1,
             "expected 1 policy mappings, got %d\n", info->cPolicyMapping);
            ok(!strcmp(info->rgPolicyMapping[0].pszIssuerDomainPolicy, "1.2.3"),
             "unexpected issuer policy %s\n",
             info->rgPolicyMapping[0].pszIssuerDomainPolicy);
            ok(!strcmp(info->rgPolicyMapping[0].pszSubjectDomainPolicy,
             "2.3.4"), "unexpected subject policy %s\n",
             info->rgPolicyMapping[0].pszSubjectDomainPolicy);
            LocalFree(info);
        }
        ret = pCryptDecodeObjectEx(dwEncoding, mappingOids[i],
         policyMappingWithTwoMappings, sizeof(policyMappingWithTwoMappings),
         CRYPT_DECODE_ALLOC_FLAG, NULL, &info, &size);
        ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
        if (ret)
        {
            ok(info->cPolicyMapping == 2,
             "expected 2 policy mappings, got %d\n", info->cPolicyMapping);
            ok(!strcmp(info->rgPolicyMapping[0].pszIssuerDomainPolicy, "1.2.3"),
             "unexpected issuer policy %s\n",
             info->rgPolicyMapping[0].pszIssuerDomainPolicy);
            ok(!strcmp(info->rgPolicyMapping[0].pszSubjectDomainPolicy,
             "2.3.4"), "unexpected subject policy %s\n",
             info->rgPolicyMapping[0].pszSubjectDomainPolicy);
            ok(!strcmp(info->rgPolicyMapping[1].pszIssuerDomainPolicy, "1.3.4"),
             "unexpected issuer policy %s\n",
             info->rgPolicyMapping[1].pszIssuerDomainPolicy);
            ok(!strcmp(info->rgPolicyMapping[1].pszSubjectDomainPolicy,
             "2.5.6"), "unexpected subject policy %s\n",
             info->rgPolicyMapping[1].pszSubjectDomainPolicy);
            LocalFree(info);
        }
        ret = pCryptDecodeObjectEx(dwEncoding, mappingOids[i],
         policyMappingWithTwoMappings, sizeof(policyMappingWithTwoMappings), 0,
         NULL, NULL, &size);
        ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
        info = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
        if (info)
        {
            ret = pCryptDecodeObjectEx(dwEncoding, mappingOids[i],
             policyMappingWithTwoMappings, sizeof(policyMappingWithTwoMappings), 0,
             NULL, info, &size);
            ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
            HeapFree(GetProcessHeap(), 0, info);
        }
    }
}

static const BYTE policyConstraintsWithRequireExplicit[] = {
0x30,0x03,0x80,0x01,0x00 };
static const BYTE policyConstraintsWithInhibitMapping[] = {
0x30,0x03,0x81,0x01,0x01 };
static const BYTE policyConstraintsWithBoth[] = {
0x30,0x06,0x80,0x01,0x01,0x81,0x01,0x01 };

static void test_encodeCertPolicyConstraints(DWORD dwEncoding)
{
    CERT_POLICY_CONSTRAINTS_INFO info = { 0 };
    LPBYTE buf;
    DWORD size;
    BOOL ret;

    /* Even though RFC 5280 explicitly states CAs must not issue empty
     * policy constraints (section 4.2.1.11), the API doesn't prevent it.
     */
    ret = pCryptEncodeObjectEx(dwEncoding, X509_POLICY_CONSTRAINTS, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret || broken(GetLastError() == ERROR_FILE_NOT_FOUND),
     "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (!ret && GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        win_skip("no policy constraints support\n");
        return;
    }
    if (ret)
    {
        ok(size == sizeof(emptySequence), "unexpected size %d\n", size);
        ok(!memcmp(buf, emptySequence, sizeof(emptySequence)),
         "unexpected value\n");
        LocalFree(buf);
    }
    /* If fRequireExplicitPolicy is set but dwRequireExplicitPolicySkipCerts
     * is not, then a skip of 0 is encoded.
     */
    info.fRequireExplicitPolicy = TRUE;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_POLICY_CONSTRAINTS, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(policyConstraintsWithRequireExplicit),
         "unexpected size %d\n", size);
        ok(!memcmp(buf, policyConstraintsWithRequireExplicit,
         sizeof(policyConstraintsWithRequireExplicit)), "unexpected value\n");
        LocalFree(buf);
    }
    /* With inhibit policy mapping */
    info.fRequireExplicitPolicy = FALSE;
    info.dwRequireExplicitPolicySkipCerts = 0;
    info.fInhibitPolicyMapping = TRUE;
    info.dwInhibitPolicyMappingSkipCerts = 1;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_POLICY_CONSTRAINTS, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(policyConstraintsWithInhibitMapping),
         "unexpected size %d\n", size);
        ok(!memcmp(buf, policyConstraintsWithInhibitMapping,
         sizeof(policyConstraintsWithInhibitMapping)), "unexpected value\n");
        LocalFree(buf);
    }
    /* And with both */
    info.fRequireExplicitPolicy = TRUE;
    info.dwRequireExplicitPolicySkipCerts = 1;
    ret = pCryptEncodeObjectEx(dwEncoding, X509_POLICY_CONSTRAINTS, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(policyConstraintsWithBoth), "unexpected size %d\n",
         size);
        ok(!memcmp(buf, policyConstraintsWithBoth,
         sizeof(policyConstraintsWithBoth)), "unexpected value\n");
        LocalFree(buf);
    }
}

static void test_decodeCertPolicyConstraints(DWORD dwEncoding)
{
    CERT_POLICY_CONSTRAINTS_INFO *info;
    DWORD size;
    BOOL ret;

    /* Again, even though CAs must not issue such constraints, they can be
     * decoded.
     */
    ret = pCryptDecodeObjectEx(dwEncoding, X509_POLICY_CONSTRAINTS,
     emptySequence, sizeof(emptySequence), CRYPT_DECODE_ALLOC_FLAG, NULL,
     &info, &size);
    ok(ret || broken(GetLastError() == ERROR_FILE_NOT_FOUND),
     "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (!ret && GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        win_skip("no policy mappings support\n");
        return;
    }
    if (ret)
    {
        ok(!info->fRequireExplicitPolicy,
         "expected require explicit = FALSE\n");
        ok(!info->fInhibitPolicyMapping,
         "expected implicit mapping = FALSE\n");
        LocalFree(info);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_POLICY_CONSTRAINTS,
     policyConstraintsWithRequireExplicit,
     sizeof(policyConstraintsWithRequireExplicit), CRYPT_DECODE_ALLOC_FLAG,
     NULL, &info, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(info->fRequireExplicitPolicy,
         "expected require explicit = TRUE\n");
        ok(info->dwRequireExplicitPolicySkipCerts == 0, "expected 0, got %d\n",
         info->dwRequireExplicitPolicySkipCerts);
        ok(!info->fInhibitPolicyMapping,
         "expected implicit mapping = FALSE\n");
        LocalFree(info);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_POLICY_CONSTRAINTS,
     policyConstraintsWithInhibitMapping,
     sizeof(policyConstraintsWithInhibitMapping), CRYPT_DECODE_ALLOC_FLAG,
     NULL, &info, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(!info->fRequireExplicitPolicy,
         "expected require explicit = FALSE\n");
        ok(info->fInhibitPolicyMapping,
         "expected implicit mapping = TRUE\n");
        ok(info->dwInhibitPolicyMappingSkipCerts == 1, "expected 1, got %d\n",
         info->dwInhibitPolicyMappingSkipCerts);
        LocalFree(info);
    }
    ret = pCryptDecodeObjectEx(dwEncoding, X509_POLICY_CONSTRAINTS,
     policyConstraintsWithBoth, sizeof(policyConstraintsWithBoth),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &info, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(info->fRequireExplicitPolicy,
         "expected require explicit = TRUE\n");
        ok(info->dwRequireExplicitPolicySkipCerts == 1, "expected 1, got %d\n",
         info->dwRequireExplicitPolicySkipCerts);
        ok(info->fInhibitPolicyMapping,
         "expected implicit mapping = TRUE\n");
        ok(info->dwInhibitPolicyMappingSkipCerts == 1, "expected 1, got %d\n",
         info->dwInhibitPolicyMappingSkipCerts);
        LocalFree(info);
    }
}

static const BYTE rsaPrivKeyDer[] = {
0x30,0x82,0x04,0xa5,0x02,0x01,0x00,0x02,0x82,0x01,0x01,0x00,
0xae,0xba,0x3c,0x41,0xeb,0x25,0x41,0xb0,0x1c,0x41,0xd4,0x26,
0xf9,0xf8,0x31,0x64,0x7e,0x97,0x65,0x54,0x9c,0x90,0xdf,0x34,
0x07,0xfb,0xb0,0x69,0x99,0x3b,0x45,0x39,0x06,0xe4,0x3a,0x7a,
0x01,0xe0,0xeb,0x3f,0xe1,0xd5,0x91,0xe0,0x16,0xe0,0xf2,0x35,
0x59,0xdf,0x32,0x2d,0x69,0x3a,0x4a,0xbc,0xd1,0xba,0x1b,0x3b,
0x7a,0x55,0x76,0xba,0x11,0xdd,0x2f,0xc7,0x58,0x66,0xf2,0x6c,
0xd1,0x68,0x27,0x6c,0x85,0x74,0x0b,0xc9,0x7b,0x1a,0xde,0x3c,
0x62,0x73,0xe2,0x9e,0x36,0x3a,0x29,0x3b,0x91,0x85,0x3d,0xd2,
0xe1,0xe5,0x61,0x84,0x1e,0x28,0xfd,0xb7,0x97,0x68,0xc1,0xbb,
0x0f,0x93,0x14,0xc2,0x03,0x60,0x41,0x11,0x7a,0xda,0x76,0x01,
0x65,0x08,0xe6,0x0c,0xf6,0xfc,0x1d,0x64,0x12,0x7b,0x42,0xb0,
0xb8,0xfe,0x61,0xe5,0xe2,0xe5,0x61,0x44,0xcc,0x94,0xe8,0xc0,
0x4f,0x58,0x9a,0xea,0x99,0xaf,0x9c,0xa4,0xf2,0xd7,0x2b,0x31,
0x90,0x3b,0x41,0x2e,0x4a,0x74,0x7c,0x1a,0xfc,0x42,0xa9,0x17,
0xff,0x53,0x20,0x76,0xa7,0xf0,0x2c,0xb9,0xd5,0x1f,0xa9,0x8a,
0x77,0xa8,0x09,0x5c,0x0e,0xd1,0x54,0xc5,0xf2,0x86,0xf1,0x2f,
0x23,0xd6,0x63,0xba,0xe9,0x2b,0x73,0xf9,0xf0,0xdc,0xcb,0xf9,
0xcb,0xe8,0x40,0x62,0x47,0x09,0x85,0xe1,0x9c,0xfd,0xcf,0x75,
0x5a,0x65,0xfd,0x86,0x1c,0x50,0xfa,0x24,0x36,0x0f,0x54,0x5e,
0x81,0xe7,0xf6,0x63,0x2d,0x87,0x0c,0x50,0x03,0x25,0x49,0xe7,
0xc5,0x20,0xaa,0xbc,0x6c,0xf9,0xbe,0x49,0x8f,0x4f,0xb8,0x9a,
0x73,0x9f,0x55,0x43,0x02,0x03,0x01,0x00,0x01,0x02,0x82,0x01,
0x01,0x00,0x99,0x03,0xcd,0x5b,0x69,0x03,0x32,0x98,0x78,0xd6,
0x89,0x65,0x2c,0xc9,0xd6,0xef,0x8c,0x11,0x27,0x93,0x46,0x9d,
0x74,0x6a,0xcb,0x86,0xf6,0x02,0x34,0x47,0xfc,0xa2,0x29,0x4f,
0xdb,0x8a,0x17,0x75,0x12,0x6f,0xda,0x65,0x3f,0x1f,0xc0,0xc9,
0x74,0x33,0x96,0xa5,0xe8,0xfa,0x6d,0xc9,0xb7,0xc3,0xcd,0xe3,
0x2e,0x90,0x12,0xdd,0x1f,0x61,0x69,0xdd,0x8b,0x47,0x07,0x3a,
0xf8,0x98,0xa5,0x76,0x91,0xf7,0xee,0x93,0x26,0xf3,0x66,0x54,
0xac,0x44,0xb3,0x6f,0x8b,0x09,0x44,0xb2,0x00,0x84,0x03,0x37,
0x6d,0x61,0xed,0xa4,0x04,0x97,0x40,0x16,0x63,0xc2,0xd0,0xdc,
0xd3,0xb3,0xee,0xba,0xbe,0x95,0xfd,0x80,0xe0,0xda,0xde,0xfc,
0xcc,0x15,0x02,0x97,0x1d,0x68,0x43,0x2f,0x9c,0xc8,0x20,0x23,
0xeb,0x00,0x4c,0x74,0x3d,0x27,0x20,0x14,0x23,0x95,0xfc,0x8c,
0xb7,0x7e,0x7f,0xb0,0xdb,0xaf,0x8a,0x48,0x1b,0xfe,0x59,0xab,
0x75,0xe2,0xbf,0x69,0xf2,0x73,0xe3,0xb9,0x92,0xa9,0x90,0x03,
0xe5,0xd4,0x2d,0x86,0xff,0x12,0x54,0xb3,0xbb,0xe2,0xce,0x81,
0x58,0x71,0xa4,0xde,0x45,0x05,0xf8,0x2d,0x45,0xf5,0xd8,0x5e,
0x4c,0x5d,0x06,0x69,0x0c,0x86,0x9f,0x66,0x9f,0xb1,0x60,0xfd,
0xf2,0x33,0x85,0x15,0xd5,0x18,0xf7,0xba,0x99,0x65,0x15,0x1d,
0xfa,0xaa,0x76,0xdd,0x25,0xed,0xdf,0x90,0x6e,0xba,0x61,0x96,
0x79,0xde,0xd2,0xda,0x66,0x03,0x74,0x3b,0x13,0x39,0x68,0xbc,
0x94,0x01,0x00,0x2d,0xf8,0xf0,0x8c,0xbd,0x4c,0x9c,0x7e,0x87,
0x9c,0x62,0x9f,0xb6,0x90,0x11,0x02,0x81,0x81,0x00,0xe3,0x5e,
0xfe,0xdd,0xed,0x76,0xb6,0x4e,0xfc,0x5b,0xe0,0x20,0x99,0x7b,
0x48,0x3b,0x1e,0x5f,0x7f,0x9f,0xa4,0x68,0xbe,0xc3,0x7f,0xb8,
0x62,0x98,0xb0,0x95,0x8a,0xfa,0x0d,0xa3,0x79,0x63,0x39,0xf7,
0xdb,0x76,0x3d,0x53,0x4a,0x0a,0x33,0xdf,0xe0,0x47,0x22,0xd5,
0x96,0x80,0xc7,0xcd,0x24,0xef,0xac,0x49,0x46,0x37,0x6c,0x25,
0xcf,0x6c,0x4d,0xe5,0x31,0xf8,0x2f,0xd2,0x59,0x74,0x00,0x38,
0xdb,0xce,0xd1,0x72,0xc3,0xa8,0x30,0x70,0xd8,0x02,0x20,0xe7,
0x56,0xe7,0xca,0xf0,0x3b,0x52,0x5d,0x11,0xbe,0x53,0x4e,0xd0,
0xd9,0x2e,0xa6,0xb8,0xe2,0xd9,0xbf,0xb9,0x77,0xe7,0x3b,0xed,
0x5e,0xd7,0x16,0x4a,0x3a,0xc5,0x86,0xd7,0x74,0x20,0xa7,0x8e,
0xbf,0xb7,0x33,0xdb,0x51,0xe9,0x02,0x81,0x81,0x00,0xc4,0xba,
0x57,0xf0,0x6e,0xcf,0xe8,0xce,0xce,0x9d,0x4a,0xe9,0x0f,0xe1,
0xab,0x91,0x62,0xaa,0x66,0x5d,0x82,0x66,0x1c,0x72,0x18,0x6f,
0x68,0x9c,0x7d,0x5e,0xfc,0xaf,0x4a,0xd6,0x8e,0xc6,0xae,0x40,
0xf2,0x40,0x84,0x93,0xee,0x7c,0x87,0xa9,0xa6,0xcd,0x2b,0xc3,
0xe6,0x29,0x3a,0xe2,0x4a,0xed,0xb0,0x4d,0x9f,0xc0,0xe9,0xd6,
0xa3,0xca,0x97,0xee,0xac,0xab,0xa4,0x32,0x05,0x40,0x4d,0xf2,
0x95,0x99,0xaf,0xa0,0xe1,0xe1,0xe7,0x3a,0x64,0xa4,0x70,0x6b,
0x3d,0x1d,0x7b,0xf1,0x53,0xfa,0xb0,0xe0,0xe2,0x68,0x1a,0x61,
0x2c,0x37,0xa5,0x39,0x7b,0xb2,0xcf,0xe6,0x5f,0x9b,0xc6,0x64,
0xaf,0x48,0x86,0xfb,0xc1,0xf3,0x39,0x97,0x10,0x36,0xf5,0xa9,
0x3d,0x08,0xa5,0x2f,0xe6,0x4b,0x02,0x81,0x81,0x00,0x86,0xe7,
0x02,0x08,0xe2,0xaf,0xa0,0x93,0x54,0x9f,0x9e,0x67,0x39,0x29,
0x30,0x3e,0x03,0x53,0x5e,0x01,0x76,0x26,0xbf,0xa8,0x76,0xcb,
0x0b,0x94,0xd4,0x90,0xa5,0x98,0x9f,0x26,0xf3,0x0a,0xb0,0x86,
0x22,0xac,0x10,0xce,0xae,0x0b,0x47,0xa3,0xf9,0x09,0xbb,0xdd,
0x46,0x22,0xba,0x69,0x39,0x15,0x0a,0xff,0x9e,0xad,0x9b,0x79,
0x03,0x8c,0x9a,0xda,0xf5,0xbe,0xef,0x80,0xba,0x9a,0x5c,0xd7,
0x5f,0x73,0x62,0x49,0xd9,0x54,0x9d,0x09,0x16,0xe0,0x8c,0x6d,
0x35,0xde,0xe9,0x45,0x87,0xac,0xe2,0x93,0x78,0x7d,0x2d,0x32,
0x34,0xe9,0xbc,0xf9,0xcd,0x7e,0xac,0x86,0x7a,0x61,0xb3,0xe8,
0xae,0x70,0xa7,0x44,0xfb,0x81,0xde,0xf3,0x4e,0x6f,0x61,0x7b,
0x0c,0xbc,0xc2,0x03,0xca,0xa1,0x02,0x81,0x80,0x69,0x5b,0x4a,
0xa1,0x4f,0x17,0x35,0x9d,0x1b,0xf6,0x0d,0x1a,0x48,0x11,0x19,
0xab,0x20,0xe6,0x15,0x30,0x5b,0x17,0x88,0x80,0x6a,0x29,0xb0,
0x22,0xae,0xd9,0xe2,0x05,0x96,0xd4,0xd5,0x5d,0xfe,0x10,0x76,
0x2c,0xab,0x53,0xf6,0x52,0xe6,0xec,0xaa,0x92,0x12,0xb0,0x35,
0x61,0x3b,0x51,0xd9,0xc2,0xf5,0xba,0x7c,0xa5,0xfa,0x15,0xa3,
0x5e,0x6a,0x83,0xbe,0x21,0xa6,0x2b,0xcb,0xb8,0x26,0x86,0x96,
0x2b,0xda,0x6d,0x14,0xcb,0xc0,0xe3,0xfa,0xe6,0x3d,0xf6,0x90,
0xa2,0x6b,0xb0,0x50,0xc3,0x5f,0x5a,0xf0,0xa5,0xc4,0x0a,0xea,
0x7d,0x5a,0x95,0x30,0x74,0x10,0xf7,0x55,0x98,0xbd,0x65,0x4a,
0xa2,0x52,0xf8,0x1d,0x64,0xbf,0x20,0xf1,0xe4,0x1d,0x28,0x67,
0xb1,0x6b,0x95,0xfd,0x85,0x02,0x81,0x81,0x00,0xda,0xb4,0x31,
0x34,0xe1,0xec,0x9a,0x1e,0x07,0xd7,0xda,0x20,0x46,0xbf,0x6b,
0xf0,0x45,0xbd,0x50,0xa2,0x0f,0x8a,0x14,0x51,0x52,0x83,0x7c,
0x47,0xc8,0x9c,0x4e,0x68,0x6b,0xae,0x00,0x25,0x63,0xdd,0x13,
0x2a,0x66,0x65,0xb6,0x74,0x91,0x5b,0xb6,0x47,0x3e,0x8e,0x46,
0x62,0xcd,0x9d,0xc1,0xf7,0x14,0x14,0xbc,0x60,0xd6,0x3c,0x7c,
0x3a,0xce,0xff,0x96,0x04,0x84,0xf6,0x44,0x1a,0xf8,0xdb,0x40,
0x1c,0xf2,0xf1,0x4d,0xb2,0x68,0x3e,0xa3,0x0b,0xc6,0xb1,0xd0,
0xa6,0x88,0x18,0x68,0xa1,0x05,0x2a,0xfc,0x2b,0x3a,0xa1,0xe6,
0x31,0x4a,0x46,0x88,0x39,0x1e,0x44,0x11,0x6c,0xc5,0x8b,0xb6,
0x8b,0xce,0x3d,0xd5,0xcb,0xbd,0xf0,0xd4,0xd9,0xfb,0x02,0x35,
0x96,0x39,0x26,0x85,0xf9 };
static const BYTE rsaPrivKeyModulus[] = {
0x43,0x55,0x9f,0x73,0x9a,0xb8,0x4f,0x8f,0x49,0xbe,0xf9,0x6c,
0xbc,0xaa,0x20,0xc5,0xe7,0x49,0x25,0x03,0x50,0x0c,0x87,0x2d,
0x63,0xf6,0xe7,0x81,0x5e,0x54,0x0f,0x36,0x24,0xfa,0x50,0x1c,
0x86,0xfd,0x65,0x5a,0x75,0xcf,0xfd,0x9c,0xe1,0x85,0x09,0x47,
0x62,0x40,0xe8,0xcb,0xf9,0xcb,0xdc,0xf0,0xf9,0x73,0x2b,0xe9,
0xba,0x63,0xd6,0x23,0x2f,0xf1,0x86,0xf2,0xc5,0x54,0xd1,0x0e,
0x5c,0x09,0xa8,0x77,0x8a,0xa9,0x1f,0xd5,0xb9,0x2c,0xf0,0xa7,
0x76,0x20,0x53,0xff,0x17,0xa9,0x42,0xfc,0x1a,0x7c,0x74,0x4a,
0x2e,0x41,0x3b,0x90,0x31,0x2b,0xd7,0xf2,0xa4,0x9c,0xaf,0x99,
0xea,0x9a,0x58,0x4f,0xc0,0xe8,0x94,0xcc,0x44,0x61,0xe5,0xe2,
0xe5,0x61,0xfe,0xb8,0xb0,0x42,0x7b,0x12,0x64,0x1d,0xfc,0xf6,
0x0c,0xe6,0x08,0x65,0x01,0x76,0xda,0x7a,0x11,0x41,0x60,0x03,
0xc2,0x14,0x93,0x0f,0xbb,0xc1,0x68,0x97,0xb7,0xfd,0x28,0x1e,
0x84,0x61,0xe5,0xe1,0xd2,0x3d,0x85,0x91,0x3b,0x29,0x3a,0x36,
0x9e,0xe2,0x73,0x62,0x3c,0xde,0x1a,0x7b,0xc9,0x0b,0x74,0x85,
0x6c,0x27,0x68,0xd1,0x6c,0xf2,0x66,0x58,0xc7,0x2f,0xdd,0x11,
0xba,0x76,0x55,0x7a,0x3b,0x1b,0xba,0xd1,0xbc,0x4a,0x3a,0x69,
0x2d,0x32,0xdf,0x59,0x35,0xf2,0xe0,0x16,0xe0,0x91,0xd5,0xe1,
0x3f,0xeb,0xe0,0x01,0x7a,0x3a,0xe4,0x06,0x39,0x45,0x3b,0x99,
0x69,0xb0,0xfb,0x07,0x34,0xdf,0x90,0x9c,0x54,0x65,0x97,0x7e,
0x64,0x31,0xf8,0xf9,0x26,0xd4,0x41,0x1c,0xb0,0x41,0x25,0xeb,
0x41,0x3c,0xba,0xae };
static const BYTE rsaPrivKeyPrime1[] = {
0xe9,0x51,0xdb,0x33,0xb7,0xbf,0x8e,0xa7,0x20,0x74,0xd7,0x86,
0xc5,0x3a,0x4a,0x16,0xd7,0x5e,0xed,0x3b,0xe7,0x77,0xb9,0xbf,
0xd9,0xe2,0xb8,0xa6,0x2e,0xd9,0xd0,0x4e,0x53,0xbe,0x11,0x5d,
0x52,0x3b,0xf0,0xca,0xe7,0x56,0xe7,0x20,0x02,0xd8,0x70,0x30,
0xa8,0xc3,0x72,0xd1,0xce,0xdb,0x38,0x00,0x74,0x59,0xd2,0x2f,
0xf8,0x31,0xe5,0x4d,0x6c,0xcf,0x25,0x6c,0x37,0x46,0x49,0xac,
0xef,0x24,0xcd,0xc7,0x80,0x96,0xd5,0x22,0x47,0xe0,0xdf,0x33,
0x0a,0x4a,0x53,0x3d,0x76,0xdb,0xf7,0x39,0x63,0x79,0xa3,0x0d,
0xfa,0x8a,0x95,0xb0,0x98,0x62,0xb8,0x7f,0xc3,0xbe,0x68,0xa4,
0x9f,0x7f,0x5f,0x1e,0x3b,0x48,0x7b,0x99,0x20,0xe0,0x5b,0xfc,
0x4e,0xb6,0x76,0xed,0xdd,0xfe,0x5e,0xe3 };
static const BYTE rsaPrivKeyPrime2[] = {
0x4b,0xe6,0x2f,0xa5,0x08,0x3d,0xa9,0xf5,0x36,0x10,0x97,0x39,
0xf3,0xc1,0xfb,0x86,0x48,0xaf,0x64,0xc6,0x9b,0x5f,0xe6,0xcf,
0xb2,0x7b,0x39,0xa5,0x37,0x2c,0x61,0x1a,0x68,0xe2,0xe0,0xb0,
0xfa,0x53,0xf1,0x7b,0x1d,0x3d,0x6b,0x70,0xa4,0x64,0x3a,0xe7,
0xe1,0xe1,0xa0,0xaf,0x99,0x95,0xf2,0x4d,0x40,0x05,0x32,0xa4,
0xab,0xac,0xee,0x97,0xca,0xa3,0xd6,0xe9,0xc0,0x9f,0x4d,0xb0,
0xed,0x4a,0xe2,0x3a,0x29,0xe6,0xc3,0x2b,0xcd,0xa6,0xa9,0x87,
0x7c,0xee,0x93,0x84,0x40,0xf2,0x40,0xae,0xc6,0x8e,0xd6,0x4a,
0xaf,0xfc,0x5e,0x7d,0x9c,0x68,0x6f,0x18,0x72,0x1c,0x66,0x82,
0x5d,0x66,0xaa,0x62,0x91,0xab,0xe1,0x0f,0xe9,0x4a,0x9d,0xce,
0xce,0xe8,0xcf,0x6e,0xf0,0x57,0xba,0xc4 };
static const BYTE rsaPrivKeyExponent1[] = {
0xa1,0xca,0x03,0xc2,0xbc,0x0c,0x7b,0x61,0x6f,0x4e,0xf3,0xde,
0x81,0xfb,0x44,0xa7,0x70,0xae,0xe8,0xb3,0x61,0x7a,0x86,0xac,
0x7e,0xcd,0xf9,0xbc,0xe9,0x34,0x32,0x2d,0x7d,0x78,0x93,0xe2,
0xac,0x87,0x45,0xe9,0xde,0x35,0x6d,0x8c,0xe0,0x16,0x09,0x9d,
0x54,0xd9,0x49,0x62,0x73,0x5f,0xd7,0x5c,0x9a,0xba,0x80,0xef,
0xbe,0xf5,0xda,0x9a,0x8c,0x03,0x79,0x9b,0xad,0x9e,0xff,0x0a,
0x15,0x39,0x69,0xba,0x22,0x46,0xdd,0xbb,0x09,0xf9,0xa3,0x47,
0x0b,0xae,0xce,0x10,0xac,0x22,0x86,0xb0,0x0a,0xf3,0x26,0x9f,
0x98,0xa5,0x90,0xd4,0x94,0x0b,0xcb,0x76,0xa8,0xbf,0x26,0x76,
0x01,0x5e,0x53,0x03,0x3e,0x30,0x29,0x39,0x67,0x9e,0x9f,0x54,
0x93,0xa0,0xaf,0xe2,0x08,0x02,0xe7,0x86 };
static const BYTE rsaPrivKeyExponent2[] = {
0x85,0xfd,0x95,0x6b,0xb1,0x67,0x28,0x1d,0xe4,0xf1,0x20,0xbf,
0x64,0x1d,0xf8,0x52,0xa2,0x4a,0x65,0xbd,0x98,0x55,0xf7,0x10,
0x74,0x30,0x95,0x5a,0x7d,0xea,0x0a,0xc4,0xa5,0xf0,0x5a,0x5f,
0xc3,0x50,0xb0,0x6b,0xa2,0x90,0xf6,0x3d,0xe6,0xfa,0xe3,0xc0,
0xcb,0x14,0x6d,0xda,0x2b,0x96,0x86,0x26,0xb8,0xcb,0x2b,0xa6,
0x21,0xbe,0x83,0x6a,0x5e,0xa3,0x15,0xfa,0xa5,0x7c,0xba,0xf5,
0xc2,0xd9,0x51,0x3b,0x61,0x35,0xb0,0x12,0x92,0xaa,0xec,0xe6,
0x52,0xf6,0x53,0xab,0x2c,0x76,0x10,0xfe,0x5d,0xd5,0xd4,0x96,
0x05,0xe2,0xd9,0xae,0x22,0xb0,0x29,0x6a,0x80,0x88,0x17,0x5b,
0x30,0x15,0xe6,0x20,0xab,0x19,0x11,0x48,0x1a,0x0d,0xf6,0x1b,
0x9d,0x35,0x17,0x4f,0xa1,0x4a,0x5b,0x69 };
static const BYTE rsaPrivKeyCoefficient[] = {
0xf9,0x85,0x26,0x39,0x96,0x35,0x02,0xfb,0xd9,0xd4,0xf0,0xbd,
0xcb,0xd5,0x3d,0xce,0x8b,0xb6,0x8b,0xc5,0x6c,0x11,0x44,0x1e,
0x39,0x88,0x46,0x4a,0x31,0xe6,0xa1,0x3a,0x2b,0xfc,0x2a,0x05,
0xa1,0x68,0x18,0x88,0xa6,0xd0,0xb1,0xc6,0x0b,0xa3,0x3e,0x68,
0xb2,0x4d,0xf1,0xf2,0x1c,0x40,0xdb,0xf8,0x1a,0x44,0xf6,0x84,
0x04,0x96,0xff,0xce,0x3a,0x7c,0x3c,0xd6,0x60,0xbc,0x14,0x14,
0xf7,0xc1,0x9d,0xcd,0x62,0x46,0x8e,0x3e,0x47,0xb6,0x5b,0x91,
0x74,0xb6,0x65,0x66,0x2a,0x13,0xdd,0x63,0x25,0x00,0xae,0x6b,
0x68,0x4e,0x9c,0xc8,0x47,0x7c,0x83,0x52,0x51,0x14,0x8a,0x0f,
0xa2,0x50,0xbd,0x45,0xf0,0x6b,0xbf,0x46,0x20,0xda,0xd7,0x07,
0x1e,0x9a,0xec,0xe1,0x34,0x31,0xb4,0xda };
static const BYTE rsaPrivKeyPrivateExponent[] = {
0x11,0x90,0xb6,0x9f,0x62,0x9c,0x87,0x7e,0x9c,0x4c,0xbd,0x8c,
0xf0,0xf8,0x2d,0x00,0x01,0x94,0xbc,0x68,0x39,0x13,0x3b,0x74,
0x03,0x66,0xda,0xd2,0xde,0x79,0x96,0x61,0xba,0x6e,0x90,0xdf,
0xed,0x25,0xdd,0x76,0xaa,0xfa,0x1d,0x15,0x65,0x99,0xba,0xf7,
0x18,0xd5,0x15,0x85,0x33,0xf2,0xfd,0x60,0xb1,0x9f,0x66,0x9f,
0x86,0x0c,0x69,0x06,0x5d,0x4c,0x5e,0xd8,0xf5,0x45,0x2d,0xf8,
0x05,0x45,0xde,0xa4,0x71,0x58,0x81,0xce,0xe2,0xbb,0xb3,0x54,
0x12,0xff,0x86,0x2d,0xd4,0xe5,0x03,0x90,0xa9,0x92,0xb9,0xe3,
0x73,0xf2,0x69,0xbf,0xe2,0x75,0xab,0x59,0xfe,0x1b,0x48,0x8a,
0xaf,0xdb,0xb0,0x7f,0x7e,0xb7,0x8c,0xfc,0x95,0x23,0x14,0x20,
0x27,0x3d,0x74,0x4c,0x00,0xeb,0x23,0x20,0xc8,0x9c,0x2f,0x43,
0x68,0x1d,0x97,0x02,0x15,0xcc,0xfc,0xde,0xda,0xe0,0x80,0xfd,
0x95,0xbe,0xba,0xee,0xb3,0xd3,0xdc,0xd0,0xc2,0x63,0x16,0x40,
0x97,0x04,0xa4,0xed,0x61,0x6d,0x37,0x03,0x84,0x00,0xb2,0x44,
0x09,0x8b,0x6f,0xb3,0x44,0xac,0x54,0x66,0xf3,0x26,0x93,0xee,
0xf7,0x91,0x76,0xa5,0x98,0xf8,0x3a,0x07,0x47,0x8b,0xdd,0x69,
0x61,0x1f,0xdd,0x12,0x90,0x2e,0xe3,0xcd,0xc3,0xb7,0xc9,0x6d,
0xfa,0xe8,0xa5,0x96,0x33,0x74,0xc9,0xc0,0x1f,0x3f,0x65,0xda,
0x6f,0x12,0x75,0x17,0x8a,0xdb,0x4f,0x29,0xa2,0xfc,0x47,0x34,
0x02,0xf6,0x86,0xcb,0x6a,0x74,0x9d,0x46,0x93,0x27,0x11,0x8c,
0xef,0xd6,0xc9,0x2c,0x65,0x89,0xd6,0x78,0x98,0x32,0x03,0x69,
0x5b,0xcd,0x03,0x99 };

static void test_decodeRsaPrivateKey(DWORD dwEncoding)
{
    LPBYTE buf = NULL;
    DWORD bufSize = 0;
    BOOL ret;

    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_RSA_PRIVATE_KEY,
     rsaPrivKeyDer, sizeof(rsaPrivKeyDer)-10,
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(!ret && (GetLastError() == CRYPT_E_ASN1_EOD),
     "Expected CRYPT_E_ASN1_EOD, got %08x\n",
     GetLastError());

    buf = NULL;
    bufSize = 0;
    ret = pCryptDecodeObjectEx(dwEncoding, PKCS_RSA_PRIVATE_KEY,
     rsaPrivKeyDer, sizeof(rsaPrivKeyDer),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());

    if (ret)
    {
        BLOBHEADER *hdr = (BLOBHEADER *)buf;
        RSAPUBKEY *rsaPubKey = (RSAPUBKEY *)(buf + sizeof(BLOBHEADER));
        static const int bitlen = 2048;
        BYTE *modulus = (BYTE*)(rsaPubKey + 1);
        BYTE *prime1 = modulus + bitlen/8;
        BYTE *prime2 = prime1 + bitlen/16;
        BYTE *exponent1 = prime2 + bitlen/16;
        BYTE *exponent2 = exponent1 + bitlen/16;
        BYTE *coefficient = exponent2 + bitlen/16;
        BYTE *privateExponent = coefficient + bitlen/16;

        ok(bufSize >= sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) +
            (bitlen * 9 / 16),
         "Wrong size %d\n", bufSize);

        ok(hdr->bType == PRIVATEKEYBLOB,
         "Expected type PRIVATEKEYBLOB (%d), got %d\n", PRIVATEKEYBLOB,
         hdr->bType);
        ok(hdr->bVersion == CUR_BLOB_VERSION,
         "Expected version CUR_BLOB_VERSION (%d), got %d\n",
         CUR_BLOB_VERSION, hdr->bVersion);
        ok(hdr->reserved == 0, "Expected reserved 0, got %d\n",
         hdr->reserved);
        ok(hdr->aiKeyAlg == CALG_RSA_KEYX,
         "Expected CALG_RSA_KEYX, got %08x\n", hdr->aiKeyAlg);

        ok(rsaPubKey->magic == 0x32415352,
         "Expected magic 0x32415352, got 0x%x\n", rsaPubKey->magic);
        ok(rsaPubKey->bitlen == bitlen,
         "Expected bitlen %d, got %d\n", bitlen, rsaPubKey->bitlen);
        ok(rsaPubKey->pubexp == 65537,
         "Expected pubexp 65537, got %d\n", rsaPubKey->pubexp);

        ok(!memcmp(modulus, rsaPrivKeyModulus, bitlen/8),
         "unexpected modulus\n");
        ok(!memcmp(prime1, rsaPrivKeyPrime1, bitlen/16),
         "unexpected prime1\n");
        ok(!memcmp(prime2, rsaPrivKeyPrime2, bitlen/16),
         "unexpected prime2\n");
        ok(!memcmp(exponent1, rsaPrivKeyExponent1, bitlen/16),
         "unexpected exponent1\n");
        ok(!memcmp(exponent2, rsaPrivKeyExponent2, bitlen/16),
         "unexpected exponent2\n");
        ok(!memcmp(coefficient, rsaPrivKeyCoefficient, bitlen/16),
         "unexpected coefficient\n");
        ok(!memcmp(privateExponent, rsaPrivKeyPrivateExponent, bitlen/8),
         "unexpected privateExponent\n");

        LocalFree(buf);
    }
}

/* Free *pInfo with HeapFree */
static void testExportPublicKey(HCRYPTPROV csp, PCERT_PUBLIC_KEY_INFO *pInfo)
{
    BOOL ret;
    DWORD size = 0;
    HCRYPTKEY key;

    /* This crashes
    ret = CryptExportPublicKeyInfoEx(0, 0, 0, NULL, 0, NULL, NULL, NULL);
     */
    ret = CryptExportPublicKeyInfoEx(0, 0, 0, NULL, 0, NULL, NULL, &size);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());
    ret = CryptExportPublicKeyInfoEx(0, AT_SIGNATURE, 0, NULL, 0, NULL, NULL,
     &size);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());
    ret = CryptExportPublicKeyInfoEx(0, 0, X509_ASN_ENCODING, NULL, 0, NULL,
     NULL, &size);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());
    ret = CryptExportPublicKeyInfoEx(0, AT_SIGNATURE, X509_ASN_ENCODING, NULL,
     0, NULL, NULL, &size);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());
    /* Test with no key */
    ret = CryptExportPublicKeyInfoEx(csp, AT_SIGNATURE, X509_ASN_ENCODING, NULL,
     0, NULL, NULL, &size);
    ok(!ret && GetLastError() == NTE_NO_KEY, "Expected NTE_NO_KEY, got %08x\n",
     GetLastError());
    ret = CryptGenKey(csp, AT_SIGNATURE, 0, &key);
    ok(ret, "CryptGenKey failed: %08x\n", GetLastError());
    if (ret)
    {
        ret = CryptExportPublicKeyInfoEx(csp, AT_SIGNATURE, X509_ASN_ENCODING,
         NULL, 0, NULL, NULL, &size);
        ok(ret, "CryptExportPublicKeyInfoEx failed: %08x\n", GetLastError());
        *pInfo = HeapAlloc(GetProcessHeap(), 0, size);
        if (*pInfo)
        {
            ret = CryptExportPublicKeyInfoEx(csp, AT_SIGNATURE,
             X509_ASN_ENCODING, NULL, 0, NULL, *pInfo, &size);
            ok(ret, "CryptExportPublicKeyInfoEx failed: %08x\n",
             GetLastError());
            if (ret)
            {
                /* By default (we passed NULL as the OID) the OID is
                 * szOID_RSA_RSA.
                 */
                ok(!strcmp((*pInfo)->Algorithm.pszObjId, szOID_RSA_RSA),
                 "Expected %s, got %s\n", szOID_RSA_RSA,
                 (*pInfo)->Algorithm.pszObjId);
            }
        }
    }
    CryptDestroyKey(key);
}

static const BYTE expiredCert[] = { 0x30, 0x82, 0x01, 0x33, 0x30, 0x81, 0xe2,
 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x10, 0xc4, 0xd7, 0x7f, 0x0e, 0x6f, 0xa6,
 0x8c, 0xaa, 0x47, 0x47, 0x40, 0xe7, 0xb7, 0x0b, 0x4a, 0x7f, 0x30, 0x09, 0x06,
 0x05, 0x2b, 0x0e, 0x03, 0x02, 0x1d, 0x05, 0x00, 0x30, 0x1f, 0x31, 0x1d, 0x30,
 0x1b, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x14, 0x61, 0x72, 0x69, 0x63, 0x40,
 0x63, 0x6f, 0x64, 0x65, 0x77, 0x65, 0x61, 0x76, 0x65, 0x72, 0x73, 0x2e, 0x63,
 0x6f, 0x6d, 0x30, 0x1e, 0x17, 0x0d, 0x36, 0x39, 0x30, 0x31, 0x30, 0x31, 0x30,
 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x17, 0x0d, 0x37, 0x30, 0x30, 0x31, 0x30,
 0x31, 0x30, 0x36, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x30, 0x1f, 0x31, 0x1d, 0x30,
 0x1b, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x14, 0x61, 0x72, 0x69, 0x63, 0x40,
 0x63, 0x6f, 0x64, 0x65, 0x77, 0x65, 0x61, 0x76, 0x65, 0x72, 0x73, 0x2e, 0x63,
 0x6f, 0x6d, 0x30, 0x5c, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7,
 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x4b, 0x00, 0x30, 0x48, 0x02, 0x41,
 0x00, 0xa1, 0xaf, 0x4a, 0xea, 0xa7, 0x83, 0x57, 0xc0, 0x37, 0x33, 0x7e, 0x29,
 0x5e, 0x0d, 0xfc, 0x44, 0x74, 0x3a, 0x1d, 0xc3, 0x1b, 0x1d, 0x96, 0xed, 0x4e,
 0xf4, 0x1b, 0x98, 0xec, 0x69, 0x1b, 0x04, 0xea, 0x25, 0xcf, 0xb3, 0x2a, 0xf5,
 0xd9, 0x22, 0xd9, 0x8d, 0x08, 0x39, 0x81, 0xc6, 0xe0, 0x4f, 0x12, 0x37, 0x2a,
 0x3f, 0x80, 0xa6, 0x6c, 0x67, 0x43, 0x3a, 0xdd, 0x95, 0x0c, 0xbb, 0x2f, 0x6b,
 0x02, 0x03, 0x01, 0x00, 0x01, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e, 0x03, 0x02,
 0x1d, 0x05, 0x00, 0x03, 0x41, 0x00, 0x8f, 0xa2, 0x5b, 0xd6, 0xdf, 0x34, 0xd0,
 0xa2, 0xa7, 0x47, 0xf1, 0x13, 0x79, 0xd3, 0xf3, 0x39, 0xbd, 0x4e, 0x2b, 0xa3,
 0xf4, 0x63, 0x37, 0xac, 0x5a, 0x0c, 0x5e, 0x4d, 0x0d, 0x54, 0x87, 0x4f, 0x31,
 0xfb, 0xa0, 0xce, 0x8f, 0x9a, 0x2f, 0x4d, 0x48, 0xc6, 0x84, 0x8d, 0xf5, 0x70,
 0x74, 0x17, 0xa5, 0xf3, 0x66, 0x47, 0x06, 0xd6, 0x64, 0x45, 0xbc, 0x52, 0xef,
 0x49, 0xe5, 0xf9, 0x65, 0xf3 };

static void testImportPublicKey(HCRYPTPROV csp, PCERT_PUBLIC_KEY_INFO info)
{
    BOOL ret;
    HCRYPTKEY key;
    PCCERT_CONTEXT context;
    DWORD dwSize;
    ALG_ID ai;

    /* These crash
    ret = CryptImportPublicKeyInfoEx(0, 0, NULL, 0, 0, NULL, NULL);
    ret = CryptImportPublicKeyInfoEx(0, 0, NULL, 0, 0, NULL, &key);
    ret = CryptImportPublicKeyInfoEx(0, 0, info, 0, 0, NULL, NULL);
    ret = CryptImportPublicKeyInfoEx(csp, X509_ASN_ENCODING, info, 0, 0, NULL,
     NULL);
     */
    ret = CryptImportPublicKeyInfoEx(0, 0, info, 0, 0, NULL, &key);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
    ret = CryptImportPublicKeyInfoEx(csp, 0, info, 0, 0, NULL, &key);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
    ret = CryptImportPublicKeyInfoEx(0, X509_ASN_ENCODING, info, 0, 0, NULL,
     &key);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());

    /* Export key with standard algorithm (CALG_RSA_KEYX) */
    ret = CryptImportPublicKeyInfoEx(csp, X509_ASN_ENCODING, info, 0, 0, NULL,
     &key);
    ok(ret, "CryptImportPublicKeyInfoEx failed: %08x\n", GetLastError());

    dwSize = sizeof(ai);
    CryptGetKeyParam(key, KP_ALGID, (LPVOID)&ai, &dwSize, 0);
    ok(ret, "CryptGetKeyParam failed: %08x\n", GetLastError());
    if(ret)
    {
      ok(dwSize == sizeof(ai), "CryptGetKeyParam returned size %d\n",dwSize);
      ok(ai == CALG_RSA_KEYX, "Default ALG_ID is %04x (expected CALG_RSA_KEYX)\n", ai);
    }

    CryptDestroyKey(key);

    /* Repeat with forced algorithm */
    ret = CryptImportPublicKeyInfoEx(csp, X509_ASN_ENCODING, info, CALG_RSA_SIGN, 0, NULL,
     &key);
    ok(ret, "CryptImportPublicKeyInfoEx failed: %08x\n", GetLastError());

    dwSize = sizeof(ai);
    CryptGetKeyParam(key, KP_ALGID, (LPVOID)&ai, &dwSize, 0);
    ok(ret, "CryptGetKeyParam failed: %08x\n", GetLastError());
    if(ret)
    {
      ok(dwSize == sizeof(ai), "CryptGetKeyParam returned size %d\n",dwSize);
      ok(ai == CALG_RSA_SIGN, "ALG_ID is %04x (expected CALG_RSA_SIGN)\n", ai);
    }

    CryptDestroyKey(key);

    /* Test importing a public key from a certificate context */
    context = CertCreateCertificateContext(X509_ASN_ENCODING, expiredCert,
     sizeof(expiredCert));
    ok(context != NULL, "CertCreateCertificateContext failed: %08x\n",
     GetLastError());
    if (context)
    {
        ok(!strcmp(szOID_RSA_RSA,
         context->pCertInfo->SubjectPublicKeyInfo.Algorithm.pszObjId),
         "Expected %s, got %s\n", szOID_RSA_RSA,
         context->pCertInfo->SubjectPublicKeyInfo.Algorithm.pszObjId);
        ret = CryptImportPublicKeyInfoEx(csp, X509_ASN_ENCODING,
         &context->pCertInfo->SubjectPublicKeyInfo, 0, 0, NULL, &key);
        ok(ret, "CryptImportPublicKeyInfoEx failed: %08x\n", GetLastError());
        CryptDestroyKey(key);
        CertFreeCertificateContext(context);
    }
}

static const char cspName[] = "WineCryptTemp";

static void testPortPublicKeyInfo(void)
{
    HCRYPTPROV csp;
    BOOL ret;
    PCERT_PUBLIC_KEY_INFO info = NULL;

    /* Just in case a previous run failed, delete this thing */
    CryptAcquireContextA(&csp, cspName, MS_DEF_PROV_A, PROV_RSA_FULL,
     CRYPT_DELETEKEYSET);
    ret = CryptAcquireContextA(&csp, cspName, MS_DEF_PROV_A, PROV_RSA_FULL,
     CRYPT_NEWKEYSET);
    ok(ret,"CryptAcquireContextA failed\n");

    testExportPublicKey(csp, &info);
    testImportPublicKey(csp, info);

    HeapFree(GetProcessHeap(), 0, info);
    CryptReleaseContext(csp, 0);
    ret = CryptAcquireContextA(&csp, cspName, MS_DEF_PROV_A, PROV_RSA_FULL,
     CRYPT_DELETEKEYSET);
    ok(ret,"CryptAcquireContextA failed\n");
}

START_TEST(encode)
{
    static const DWORD encodings[] = { X509_ASN_ENCODING, PKCS_7_ASN_ENCODING,
     X509_ASN_ENCODING | PKCS_7_ASN_ENCODING };
    HMODULE hCrypt32;
    DWORD i;

    hCrypt32 = GetModuleHandleA("crypt32.dll");
    pCryptDecodeObjectEx = (void*)GetProcAddress(hCrypt32, "CryptDecodeObjectEx");
    pCryptEncodeObjectEx = (void*)GetProcAddress(hCrypt32, "CryptEncodeObjectEx");
    if (!pCryptDecodeObjectEx || !pCryptEncodeObjectEx)
    {
        win_skip("CryptDecodeObjectEx() is not available\n");
        return;
    }

    for (i = 0; i < sizeof(encodings) / sizeof(encodings[0]); i++)
    {
        test_encodeInt(encodings[i]);
        test_decodeInt(encodings[i]);
        test_encodeEnumerated(encodings[i]);
        test_decodeEnumerated(encodings[i]);
        test_encodeFiletime(encodings[i]);
        test_decodeFiletime(encodings[i]);
        test_encodeName(encodings[i]);
        test_decodeName(encodings[i]);
        test_encodeUnicodeName(encodings[i]);
        test_decodeUnicodeName(encodings[i]);
        test_encodeNameValue(encodings[i]);
        test_decodeNameValue(encodings[i]);
        test_encodeUnicodeNameValue(encodings[i]);
        test_decodeUnicodeNameValue(encodings[i]);
        test_encodeAltName(encodings[i]);
        test_decodeAltName(encodings[i]);
        test_encodeOctets(encodings[i]);
        test_decodeOctets(encodings[i]);
        test_encodeBits(encodings[i]);
        test_decodeBits(encodings[i]);
        test_encodeBasicConstraints(encodings[i]);
        test_decodeBasicConstraints(encodings[i]);
        test_encodeRsaPublicKey(encodings[i]);
        test_decodeRsaPublicKey(encodings[i]);
        test_encodeSequenceOfAny(encodings[i]);
        test_decodeSequenceOfAny(encodings[i]);
        test_encodeExtensions(encodings[i]);
        test_decodeExtensions(encodings[i]);
        test_encodePublicKeyInfo(encodings[i]);
        test_decodePublicKeyInfo(encodings[i]);
        test_encodeCertToBeSigned(encodings[i]);
        test_decodeCertToBeSigned(encodings[i]);
        test_encodeCert(encodings[i]);
        test_decodeCert(encodings[i]);
        test_encodeCRLDistPoints(encodings[i]);
        test_decodeCRLDistPoints(encodings[i]);
        test_encodeCRLIssuingDistPoint(encodings[i]);
        test_decodeCRLIssuingDistPoint(encodings[i]);
        test_encodeCRLToBeSigned(encodings[i]);
        test_decodeCRLToBeSigned(encodings[i]);
        test_encodeEnhancedKeyUsage(encodings[i]);
        test_decodeEnhancedKeyUsage(encodings[i]);
        test_encodeAuthorityKeyId(encodings[i]);
        test_decodeAuthorityKeyId(encodings[i]);
        test_encodeAuthorityKeyId2(encodings[i]);
        test_decodeAuthorityKeyId2(encodings[i]);
        test_encodeAuthorityInfoAccess(encodings[i]);
        test_decodeAuthorityInfoAccess(encodings[i]);
        test_encodeCTL(encodings[i]);
        test_decodeCTL(encodings[i]);
        test_encodePKCSContentInfo(encodings[i]);
        test_decodePKCSContentInfo(encodings[i]);
        test_encodePKCSAttribute(encodings[i]);
        test_decodePKCSAttribute(encodings[i]);
        test_encodePKCSAttributes(encodings[i]);
        test_decodePKCSAttributes(encodings[i]);
        test_encodePKCSSMimeCapabilities(encodings[i]);
        test_decodePKCSSMimeCapabilities(encodings[i]);
        test_encodePKCSSignerInfo(encodings[i]);
        test_decodePKCSSignerInfo(encodings[i]);
        test_encodeCMSSignerInfo(encodings[i]);
        test_decodeCMSSignerInfo(encodings[i]);
        test_encodeNameConstraints(encodings[i]);
        test_decodeNameConstraints(encodings[i]);
        test_encodePolicyQualifierUserNotice(encodings[i]);
        test_decodePolicyQualifierUserNotice(encodings[i]);
        test_encodeCertPolicies(encodings[i]);
        test_decodeCertPolicies(encodings[i]);
        test_encodeCertPolicyMappings(encodings[i]);
        test_decodeCertPolicyMappings(encodings[i]);
        test_encodeCertPolicyConstraints(encodings[i]);
        test_decodeCertPolicyConstraints(encodings[i]);
        test_decodeRsaPrivateKey(encodings[i]);
    }
    testPortPublicKeyInfo();
}
