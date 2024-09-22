/* Unit test suite for wintrust asn functions
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
 *
 */
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wincrypt.h"
#include "wintrust.h"

#include "wine/test.h"

static BOOL (WINAPI *pCryptDecodeObjectEx)(DWORD,LPCSTR,const BYTE*,DWORD,DWORD,PCRYPT_DECODE_PARA,void*,DWORD*);
static BOOL (WINAPI *pCryptEncodeObjectEx)(DWORD,LPCSTR,const void*,DWORD,PCRYPT_ENCODE_PARA,void*,DWORD*);

static const BYTE falseCriteria[] = { 0x30,0x06,0x01,0x01,0x00,0x01,0x01,0x00 };
static const BYTE trueCriteria[] = { 0x30,0x06,0x01,0x01,0xff,0x01,0x01,0xff };

static void test_encodeSPCFinancialCriteria(void)
{
    BOOL ret;
    DWORD size = 0;
    LPBYTE buf;
    SPC_FINANCIAL_CRITERIA criteria = { FALSE, FALSE };

    if (!pCryptEncodeObjectEx)
    {
        win_skip("CryptEncodeObjectEx() is not available. Skipping the encodeFinancialCriteria tests\n");
        return;
    }
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_FINANCIAL_CRITERIA_STRUCT,
     &criteria, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(falseCriteria), "Unexpected size %ld\n", size);
        ok(!memcmp(buf, falseCriteria, size), "Unexpected value\n");
        LocalFree(buf);
    }
    criteria.fFinancialInfoAvailable = criteria.fMeetsCriteria = TRUE;
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_FINANCIAL_CRITERIA_STRUCT,
     &criteria, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(trueCriteria), "Unexpected size %ld\n", size);
        ok(!memcmp(buf, trueCriteria, size), "Unexpected value\n");
        LocalFree(buf);
    }
}

static void test_decodeSPCFinancialCriteria(void)
{
    BOOL ret;
    SPC_FINANCIAL_CRITERIA criteria;
    DWORD size = sizeof(criteria);

    if (!pCryptDecodeObjectEx)
    {
        win_skip("CryptDecodeObjectEx() is not available. Skipping the decodeSPCFinancialCriteria tests\n");
        return;
    }

    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, SPC_FINANCIAL_CRITERIA_STRUCT,
     falseCriteria, sizeof(falseCriteria), 0, NULL, &criteria, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(!criteria.fFinancialInfoAvailable, "expected FALSE\n");
        ok(!criteria.fMeetsCriteria, "expected FALSE\n");
    }
    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, SPC_FINANCIAL_CRITERIA_STRUCT,
     trueCriteria, sizeof(trueCriteria), 0, NULL, &criteria, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(criteria.fFinancialInfoAvailable, "expected TRUE\n");
        ok(criteria.fMeetsCriteria, "expected TRUE\n");
    }
}

static WCHAR url[] = { 'h','t','t','p',':','/','/','w','i','n','e','h','q','.',
 'o','r','g',0 };
static const WCHAR nihongoURL[] = { 'h','t','t','p',':','/','/',0x226f,
 0x575b, 0 };
static const BYTE emptyURLSPCLink[] = { 0x80,0x00 };
static const BYTE urlSPCLink[] = {
0x80,0x11,0x68,0x74,0x74,0x70,0x3a,0x2f,0x2f,0x77,0x69,0x6e,0x65,0x68,0x71,
0x2e,0x6f,0x72,0x67};
static const BYTE fileSPCLink[] = {
0xa2,0x14,0x80,0x12,0x00,0x68,0x00,0x74,0x00,0x74,0x00,0x70,0x00,0x3a,0x00,
0x2f,0x00,0x2f,0x22,0x6f,0x57,0x5b };
static const BYTE emptyMonikerSPCLink[] = {
0xa1,0x14,0x04,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x04,0x00 };
static BYTE data[] = { 0xba, 0xad, 0xf0, 0x0d };
static const BYTE monikerSPCLink[] = {
0xa1,0x18,0x04,0x10,0xea,0xea,0xea,0xea,0xea,0xea,0xea,0xea,0xea,0xea,0xea,
0xea,0xea,0xea,0xea,0xea,0x04,0x04,0xba,0xad,0xf0,0x0d };

static void test_encodeSPCLink(void)
{
    BOOL ret;
    DWORD size = 0;
    LPBYTE buf;
    SPC_LINK link = { 0 };

    if (!pCryptEncodeObjectEx)
    {
        win_skip("CryptEncodeObjectEx() is not available. Skipping the encodeSPCLink tests\n");
        return;
    }

    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT, &link,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    link.dwLinkChoice = SPC_URL_LINK_CHOICE;
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT, &link,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptyURLSPCLink), "Unexpected size %ld\n", size);
        ok(!memcmp(buf, emptyURLSPCLink, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* With an invalid char: */
    link.pwszUrl = (LPWSTR)nihongoURL;
    size = 1;
    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT, &link,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret &&
     (GetLastError() == CRYPT_E_INVALID_IA5_STRING ||
     GetLastError() == OSS_BAD_PTR /* WinNT */),
     "Expected CRYPT_E_INVALID_IA5_STRING, got %08lx\n", GetLastError());
    /* Unlike the crypt32 string encoding routines, size is not set to the
     * index of the first invalid character.
     */
    ok(size == 0, "Expected size 0, got %ld\n", size);
    link.pwszUrl = url;
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT, &link,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(urlSPCLink), "Unexpected size %ld\n", size);
        ok(!memcmp(buf, urlSPCLink, size), "Unexpected value\n");
        LocalFree(buf);
    }
    link.dwLinkChoice = SPC_FILE_LINK_CHOICE;
    link.pwszFile = (LPWSTR)nihongoURL;
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT, &link,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(fileSPCLink), "Unexpected size %ld\n", size);
        ok(!memcmp(buf, fileSPCLink, size), "Unexpected value\n");
        LocalFree(buf);
    }
    link.dwLinkChoice = SPC_MONIKER_LINK_CHOICE;
    memset(&link.Moniker, 0, sizeof(link.Moniker));
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT, &link,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptyMonikerSPCLink), "Unexpected size %ld\n", size);
        ok(!memcmp(buf, emptyMonikerSPCLink, size), "Unexpected value\n");
        LocalFree(buf);
    }
    memset(&link.Moniker.ClassId, 0xea, sizeof(link.Moniker.ClassId));
    link.Moniker.SerializedData.pbData = data;
    link.Moniker.SerializedData.cbData = sizeof(data);
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT, &link,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(monikerSPCLink), "Unexpected size %ld\n", size);
        ok(!memcmp(buf, monikerSPCLink, size), "Unexpected value\n");
        LocalFree(buf);
    }
}

static const BYTE badMonikerSPCLink[] = {
0xa1,0x19,0x04,0x11,0xea,0xea,0xea,0xea,0xea,0xea,0xea,0xea,0xea,0xea,0xea,
0xea,0xea,0xea,0xea,0xea,0xea,0x04,0x04,0xba,0xad,0xf0,0x0d };

static void test_decodeSPCLink(void)
{
    BOOL ret;
    LPBYTE buf = NULL;
    DWORD size = 0;
    SPC_LINK *link;

    if (!pCryptDecodeObjectEx)
    {
        win_skip("CryptDecodeObjectEx() is not available. Skipping the decodeSPCLink tests\n");
        return;
    }

    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT,
     emptyURLSPCLink, sizeof(emptyURLSPCLink), CRYPT_DECODE_ALLOC_FLAG, NULL,
     &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        link = (SPC_LINK *)buf;
        ok(link->dwLinkChoice == SPC_URL_LINK_CHOICE,
         "Expected SPC_URL_LINK_CHOICE, got %ld\n", link->dwLinkChoice);
        ok(lstrlenW(link->pwszUrl) == 0, "Expected empty string\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT,
     urlSPCLink, sizeof(urlSPCLink), CRYPT_DECODE_ALLOC_FLAG, NULL,
     &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        link = (SPC_LINK *)buf;
        ok(link->dwLinkChoice == SPC_URL_LINK_CHOICE,
         "Expected SPC_URL_LINK_CHOICE, got %ld\n", link->dwLinkChoice);
        ok(!lstrcmpW(link->pwszUrl, url), "Unexpected URL\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT,
     fileSPCLink, sizeof(fileSPCLink), CRYPT_DECODE_ALLOC_FLAG, NULL,
     &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        link = (SPC_LINK *)buf;
        ok(link->dwLinkChoice == SPC_FILE_LINK_CHOICE,
         "Expected SPC_FILE_LINK_CHOICE, got %ld\n", link->dwLinkChoice);
        ok(!lstrcmpW(link->pwszFile, nihongoURL), "Unexpected file\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT,
     emptyMonikerSPCLink, sizeof(emptyMonikerSPCLink), CRYPT_DECODE_ALLOC_FLAG,
     NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        SPC_SERIALIZED_OBJECT emptyMoniker = { { 0 } };

        link = (SPC_LINK *)buf;
        ok(link->dwLinkChoice == SPC_MONIKER_LINK_CHOICE,
         "Expected SPC_MONIKER_LINK_CHOICE, got %ld\n", link->dwLinkChoice);
        ok(!memcmp(&link->Moniker.ClassId, &emptyMoniker.ClassId,
         sizeof(emptyMoniker.ClassId)), "Unexpected value\n");
        ok(link->Moniker.SerializedData.cbData == 0,
         "Expected no serialized data\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT,
     monikerSPCLink, sizeof(monikerSPCLink), CRYPT_DECODE_ALLOC_FLAG, NULL,
     &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        SPC_UUID id;

        link = (SPC_LINK *)buf;
        ok(link->dwLinkChoice == SPC_MONIKER_LINK_CHOICE,
         "Expected SPC_MONIKER_LINK_CHOICE, got %ld\n", link->dwLinkChoice);
        memset(&id, 0xea, sizeof(id));
        ok(!memcmp(&link->Moniker.ClassId, &id, sizeof(id)),
         "Unexpected value\n");
        ok(link->Moniker.SerializedData.cbData == sizeof(data),
           "Unexpected data size %ld\n", link->Moniker.SerializedData.cbData);
        ok(!memcmp(link->Moniker.SerializedData.pbData, data, sizeof(data)),
         "Unexpected value\n");
        LocalFree(buf);
    }
    SetLastError(0xdeadbeef);
    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT,
     badMonikerSPCLink, sizeof(badMonikerSPCLink), CRYPT_DECODE_ALLOC_FLAG,
     NULL, &buf, &size);
    ok(!ret &&
     (GetLastError() == CRYPT_E_BAD_ENCODE ||
     GetLastError() == OSS_DATA_ERROR /* WinNT */),
     "Expected CRYPT_E_BAD_ENCODE, got %08lx\n", GetLastError());
}

static const BYTE emptySequence[] = { 0x30,0x00 };
static BYTE flags[] = { 1 };
static const BYTE onlyFlagsPEImage[] = { 0x30,0x04,0x03,0x02,0x00,0x01 };
static const BYTE onlyEmptyFilePEImage[] = {
0x30,0x06,0xa0,0x04,0xa2,0x02,0x80,0x00 };
static const BYTE flagsAndEmptyFilePEImage[] = {
0x30,0x0a,0x03,0x02,0x00,0x01,0xa0,0x04,0xa2,0x02,0x80,0x00 };
static const BYTE flagsAndFilePEImage[] = {
0x30,0x1c,0x03,0x02,0x00,0x01,0xa0,0x16,0xa2,0x14,0x80,0x12,0x00,0x68,0x00,
0x74,0x00,0x74,0x00,0x70,0x00,0x3a,0x00,0x2f,0x00,0x2f,0x22,0x6f,0x57,0x5b };

static void test_encodeSPCPEImage(void)
{
    BOOL ret;
    DWORD size = 0;
    LPBYTE buf;
    SPC_PE_IMAGE_DATA imageData = { { 0 } };
    SPC_LINK link = { 0 };

    if (!pCryptEncodeObjectEx)
    {
        win_skip("CryptEncodeObjectEx() is not available. Skipping the encodeSPCPEImage tests\n");
        return;
    }

    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_PE_IMAGE_DATA_STRUCT,
     &imageData, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptySequence), "Unexpected size %ld\n", size);
        ok(!memcmp(buf, emptySequence, sizeof(emptySequence)),
         "Unexpected value\n");
        LocalFree(buf);
    }
    /* With an invalid link: */
    imageData.pFile = &link;
    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_PE_IMAGE_DATA_STRUCT,
     &imageData, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && GetLastError () == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    /* With just unused bits field set: */
    imageData.pFile = NULL;
    imageData.Flags.cUnusedBits = 1;
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_PE_IMAGE_DATA_STRUCT,
     &imageData, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptySequence), "Unexpected size %ld\n", size);
        ok(!memcmp(buf, emptySequence, sizeof(emptySequence)),
         "Unexpected value\n");
        LocalFree(buf);
    }
    /* With flags set: */
    imageData.Flags.cUnusedBits = 0;
    imageData.Flags.pbData = flags;
    imageData.Flags.cbData = sizeof(flags);
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_PE_IMAGE_DATA_STRUCT,
     &imageData, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    if (!ret && GetLastError() == OSS_TOO_LONG)
    {
        skip("SPC_PE_IMAGE_DATA_STRUCT not supported\n");
        return;
    }
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(onlyFlagsPEImage), "Unexpected size %ld\n", size);
        ok(!memcmp(buf, onlyFlagsPEImage, sizeof(onlyFlagsPEImage)),
         "Unexpected value\n");
        LocalFree(buf);
    }
    /* With just an empty file: */
    imageData.Flags.cbData = 0;
    link.dwLinkChoice = SPC_FILE_LINK_CHOICE;
    imageData.pFile = &link;
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_PE_IMAGE_DATA_STRUCT,
     &imageData, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(onlyEmptyFilePEImage), "Unexpected size %ld\n", size);
        ok(!memcmp(buf, onlyEmptyFilePEImage, sizeof(onlyEmptyFilePEImage)),
         "Unexpected value\n");
        LocalFree(buf);
    }
    /* With flags and an empty file: */
    imageData.Flags.pbData = flags;
    imageData.Flags.cbData = sizeof(flags);
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_PE_IMAGE_DATA_STRUCT,
     &imageData, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(flagsAndEmptyFilePEImage), "Unexpected size %ld\n",
         size);
        ok(!memcmp(buf, flagsAndEmptyFilePEImage,
         sizeof(flagsAndEmptyFilePEImage)), "Unexpected value\n");
        LocalFree(buf);
    }
    /* Finally, a non-empty file: */
    link.pwszFile = (LPWSTR)nihongoURL;
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_PE_IMAGE_DATA_STRUCT,
     &imageData, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(flagsAndFilePEImage), "Unexpected size %ld\n", size);
        ok(!memcmp(buf, flagsAndFilePEImage, sizeof(flagsAndFilePEImage)),
         "Unexpected value\n");
        LocalFree(buf);
    }
}

static void test_decodeSPCPEImage(void)
{
    static const WCHAR emptyString[] = { 0 };
    BOOL ret;
    LPBYTE buf = NULL;
    DWORD size = 0;
    SPC_PE_IMAGE_DATA *imageData;

    if (!pCryptDecodeObjectEx)
    {
        win_skip("CryptDecodeObjectEx() is not available. Skipping the decodeSPCPEImage tests\n");
        return;
    }

    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, SPC_PE_IMAGE_DATA_STRUCT,
     emptySequence, sizeof(emptySequence),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        imageData = (SPC_PE_IMAGE_DATA *)buf;
        ok(imageData->Flags.cbData == 0, "Expected empty flags, got %ld\n",
         imageData->Flags.cbData);
        ok(imageData->pFile == NULL, "Expected no file\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, SPC_PE_IMAGE_DATA_STRUCT,
     onlyFlagsPEImage, sizeof(onlyFlagsPEImage),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        imageData = (SPC_PE_IMAGE_DATA *)buf;
        ok(imageData->Flags.cbData == sizeof(flags),
         "Unexpected flags size %ld\n", imageData->Flags.cbData);
        if (imageData->Flags.cbData)
            ok(!memcmp(imageData->Flags.pbData, flags, sizeof(flags)),
             "Unexpected flags\n");
        ok(imageData->pFile == NULL, "Expected no file\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, SPC_PE_IMAGE_DATA_STRUCT,
     onlyEmptyFilePEImage, sizeof(onlyEmptyFilePEImage),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        imageData = (SPC_PE_IMAGE_DATA *)buf;
        ok(imageData->Flags.cbData == 0, "Expected empty flags, got %ld\n",
         imageData->Flags.cbData);
        ok(imageData->pFile != NULL, "Expected a file\n");
        if (imageData->pFile)
        {
            ok(imageData->pFile->dwLinkChoice == SPC_FILE_LINK_CHOICE,
             "Expected SPC_FILE_LINK_CHOICE, got %ld\n",
             imageData->pFile->dwLinkChoice);
            ok(!lstrcmpW(imageData->pFile->pwszFile, emptyString),
             "Unexpected file\n");
        }
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, SPC_PE_IMAGE_DATA_STRUCT,
     flagsAndEmptyFilePEImage, sizeof(flagsAndEmptyFilePEImage),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        imageData = (SPC_PE_IMAGE_DATA *)buf;
        ok(imageData->Flags.cbData == sizeof(flags),
         "Unexpected flags size %ld\n", imageData->Flags.cbData);
        if (imageData->Flags.cbData)
            ok(!memcmp(imageData->Flags.pbData, flags, sizeof(flags)),
             "Unexpected flags\n");
        ok(imageData->pFile != NULL, "Expected a file\n");
        if (imageData->pFile)
        {
            ok(imageData->pFile->dwLinkChoice == SPC_FILE_LINK_CHOICE,
             "Expected SPC_FILE_LINK_CHOICE, got %ld\n",
             imageData->pFile->dwLinkChoice);
            ok(!lstrcmpW(imageData->pFile->pwszFile, emptyString),
             "Unexpected file\n");
        }
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, SPC_PE_IMAGE_DATA_STRUCT,
     flagsAndFilePEImage, sizeof(flagsAndFilePEImage),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        imageData = (SPC_PE_IMAGE_DATA *)buf;
        ok(imageData->Flags.cbData == sizeof(flags),
         "Unexpected flags size %ld\n", imageData->Flags.cbData);
        if (imageData->Flags.cbData)
            ok(!memcmp(imageData->Flags.pbData, flags, sizeof(flags)),
             "Unexpected flags\n");
        ok(imageData->pFile != NULL, "Expected a file\n");
        if (imageData->pFile)
        {
            ok(imageData->pFile->dwLinkChoice == SPC_FILE_LINK_CHOICE,
             "Expected SPC_FILE_LINK_CHOICE, got %ld\n",
             imageData->pFile->dwLinkChoice);
            ok(!lstrcmpW(imageData->pFile->pwszFile, nihongoURL),
             "Unexpected file\n");
        }
        LocalFree(buf);
    }
}

static const BYTE emptyIndirectData[] = {
 0x30,0x0c,0x30,0x02,0x06,0x00,0x30,0x06,0x30,0x02,0x06,0x00,0x04,0x00 };
static const BYTE spcidcWithOnlyDigest[] = {
 0x30,0x20,0x30,0x02,0x06,0x00,0x30,0x1a,0x30,0x02,0x06,0x00,0x04,0x14,0x01,0x02,
 0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,
 0x13,0x14 };
static const BYTE spcidcWithDigestAndAlgorithm[] = {
 0x30,0x27,0x30,0x02,0x06,0x00,0x30,0x21,0x30,0x09,0x06,0x05,0x2b,0x0e,0x03,0x02,
 0x1a,0x05,0x00,0x04,0x14,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,
 0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14 };
static const BYTE spcidcWithDigestAndAlgorithmParams[] = {
 0x30,0x29,0x30,0x02,0x06,0x00,0x30,0x23,0x30,0x0b,0x06,0x05,0x2b,0x0e,0x03,0x02,
 0x1a,0x74,0x65,0x73,0x74,0x04,0x14,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,
 0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14 };
static const BYTE spcidcWithEverythingExceptDataValue[] = {
 0x30,0x33,0x30,0x0c,0x06,0x0a,0x2b,0x06,0x01,0x04,0x01,0x82,0x37,0x02,0x01,0x0f,
 0x30,0x23,0x30,0x0b,0x06,0x05,0x2b,0x0e,0x03,0x02,0x1a,0x74,0x65,0x73,0x74,0x04,
 0x14,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
 0x10,0x11,0x12,0x13,0x14 };
static const BYTE spcidcWithEverything[] = {
 0x30,0x35,0x30,0x0e,0x06,0x0a,0x2b,0x06,0x01,0x04,0x01,0x82,0x37,0x02,0x01,0x0f,
 0x30,0x00,0x30,0x23,0x30,0x0b,0x06,0x05,0x2b,0x0e,0x03,0x02,0x1a,0x74,0x65,0x73,
 0x74,0x04,0x14,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,
 0x0e,0x0f,0x10,0x11,0x12,0x13,0x14 };

static BYTE fakeDigest[] = {
 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,
 0x11,0x12,0x13,0x14 };
static BYTE parameters[] = { 't','e','s','t' };

static void test_encodeSPCIndirectDataContent(void)
{
    static char SPC_PE_IMAGE_DATA_OBJID_[] = SPC_PE_IMAGE_DATA_OBJID;
    static char szOID_OIWSEC_sha1_[] = szOID_OIWSEC_sha1;

    SPC_INDIRECT_DATA_CONTENT indirectData = { { 0 } };
    DWORD size = 0;
    LPBYTE buf;
    BOOL ret;

    if (!pCryptEncodeObjectEx)
    {
        win_skip("CryptEncodeObjectEx() is not available. Skipping the encodeSPCIndirectDataContent tests\n");
        return;
    }

    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_INDIRECT_DATA_CONTENT_STRUCT,
     &indirectData, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptyIndirectData), "Unexpected size %ld\n", size);
        if (size == sizeof(emptyIndirectData))
            ok(!memcmp(buf, emptyIndirectData, sizeof(emptyIndirectData)),
             "Unexpected value\n");
        LocalFree(buf);
    }
    /* With only digest */
    indirectData.Digest.cbData = sizeof(fakeDigest);
    indirectData.Digest.pbData = fakeDigest;
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_INDIRECT_DATA_CONTENT_STRUCT,
     &indirectData, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(spcidcWithOnlyDigest), "Unexpected size %ld\n", size);
        if (size == sizeof(spcidcWithOnlyDigest))
            ok(!memcmp(buf, spcidcWithOnlyDigest, sizeof(spcidcWithOnlyDigest)),
             "Unexpected value\n");
        LocalFree(buf);
    }
    /* With digest and digest algorithm */
    indirectData.DigestAlgorithm.pszObjId = szOID_OIWSEC_sha1_;
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_INDIRECT_DATA_CONTENT_STRUCT,
     &indirectData, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(spcidcWithDigestAndAlgorithm), "Unexpected size %ld\n",
         size);
        if (size == sizeof(spcidcWithDigestAndAlgorithm))
            ok(!memcmp(buf, spcidcWithDigestAndAlgorithm,
                       sizeof(spcidcWithDigestAndAlgorithm)),
             "Unexpected value\n");
        LocalFree(buf);
    }
    /* With digest algorithm parameters */
    indirectData.DigestAlgorithm.Parameters.cbData = sizeof(parameters);
    indirectData.DigestAlgorithm.Parameters.pbData = parameters;
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_INDIRECT_DATA_CONTENT_STRUCT,
     &indirectData, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(spcidcWithDigestAndAlgorithmParams),
         "Unexpected size %ld\n", size);
        if (size == sizeof(spcidcWithDigestAndAlgorithmParams))
            ok(!memcmp(buf, spcidcWithDigestAndAlgorithmParams,
                       sizeof(spcidcWithDigestAndAlgorithmParams)),
             "Unexpected value\n");
        LocalFree(buf);
    }
    /* With everything except Data.Value */
    indirectData.Data.pszObjId = SPC_PE_IMAGE_DATA_OBJID_;
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_INDIRECT_DATA_CONTENT_STRUCT,
     &indirectData, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(spcidcWithEverythingExceptDataValue),
         "Unexpected size %ld\n", size);
        if (size == sizeof(spcidcWithEverythingExceptDataValue))
            ok(!memcmp(buf, spcidcWithEverythingExceptDataValue,
                       sizeof(spcidcWithEverythingExceptDataValue)),
             "Unexpected value\n");
        LocalFree(buf);
    }
    /* With everything */
    indirectData.Data.Value.cbData = sizeof(emptySequence);
    indirectData.Data.Value.pbData = (void*)emptySequence;
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_INDIRECT_DATA_CONTENT_STRUCT,
     &indirectData, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(spcidcWithEverything), "Unexpected size %ld\n", size);
        if (size == sizeof(spcidcWithEverything))
            ok(!memcmp(buf, spcidcWithEverything, sizeof(spcidcWithEverything)),
             "Unexpected value\n");
        LocalFree(buf);
    }
}

static void test_decodeSPCIndirectDataContent(void)
{
    static const BYTE asn1Null[] = { 0x05,0x00 };

    BOOL ret;
    LPBYTE buf = NULL;
    DWORD size = 0;
    SPC_INDIRECT_DATA_CONTENT *indirectData;

    if (!pCryptDecodeObjectEx)
    {
        win_skip("CryptDecodeObjectEx() is not available. Skipping the decodeSPCIndirectDataContent tests\n");
        return;
    }

    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, SPC_INDIRECT_DATA_CONTENT_STRUCT,
     emptyIndirectData, sizeof(emptyIndirectData),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        indirectData = (SPC_INDIRECT_DATA_CONTENT *)buf;
        ok(indirectData->Data.pszObjId != NULL, "Expected non-NULL data objid\n");
        if (indirectData->Data.pszObjId)
            ok(!strcmp(indirectData->Data.pszObjId, ""),
             "Expected empty data objid\n");
        ok(indirectData->Data.Value.cbData == 0, "Expected no data value\n");
        ok(indirectData->DigestAlgorithm.pszObjId != NULL,
         "Expected non-NULL digest algorithm objid\n");
        if (indirectData->DigestAlgorithm.pszObjId)
            ok(!strcmp(indirectData->DigestAlgorithm.pszObjId, ""),
             "Expected empty digest algorithm objid\n");
        ok(indirectData->DigestAlgorithm.Parameters.cbData == 0,
         "Expected no digest algorithm parameters\n");
        ok(indirectData->Digest.cbData == 0, "Expected no digest\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, SPC_INDIRECT_DATA_CONTENT_STRUCT,
     spcidcWithOnlyDigest, sizeof(spcidcWithOnlyDigest),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        indirectData = (SPC_INDIRECT_DATA_CONTENT *)buf;
        ok(indirectData->Digest.cbData == sizeof(fakeDigest),
         "Unexpected digest size %ld\n", indirectData->Digest.cbData);
        if (indirectData->Digest.cbData == sizeof(fakeDigest))
            ok(!memcmp(indirectData->Digest.pbData, fakeDigest,
                       sizeof(fakeDigest)),
             "Unexpected flags\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, SPC_INDIRECT_DATA_CONTENT_STRUCT,
     spcidcWithDigestAndAlgorithm, sizeof(spcidcWithDigestAndAlgorithm),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        indirectData = (SPC_INDIRECT_DATA_CONTENT *)buf;
        ok(indirectData->DigestAlgorithm.pszObjId != NULL,
         "Expected non-NULL digest algorithm objid\n");
        if (indirectData->DigestAlgorithm.pszObjId)
            ok(!strcmp(indirectData->DigestAlgorithm.pszObjId, szOID_OIWSEC_sha1),
             "Expected szOID_OIWSEC_sha1, got \"%s\"\n",
             indirectData->DigestAlgorithm.pszObjId);
        ok(indirectData->DigestAlgorithm.Parameters.cbData == sizeof(asn1Null),
         "Unexpected digest algorithm parameters size %ld\n",
         indirectData->DigestAlgorithm.Parameters.cbData);
        if (indirectData->DigestAlgorithm.Parameters.cbData == sizeof(asn1Null))
            ok(!memcmp(indirectData->DigestAlgorithm.Parameters.pbData,
                       asn1Null, sizeof(asn1Null)),
             "Unexpected digest algorithm parameters\n");
        ok(indirectData->Digest.cbData == sizeof(fakeDigest),
         "Unexpected digest size %ld\n", indirectData->Digest.cbData);
        if (indirectData->Digest.cbData == sizeof(fakeDigest))
            ok(!memcmp(indirectData->Digest.pbData, fakeDigest,
                       sizeof(fakeDigest)),
             "Unexpected flags\n");
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, SPC_INDIRECT_DATA_CONTENT_STRUCT,
     spcidcWithDigestAndAlgorithmParams, sizeof(spcidcWithDigestAndAlgorithmParams),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    /* what? */
    ok(!ret && GetLastError () == CRYPT_E_ASN1_EOD,
     "Expected CRYPT_E_ASN1_EOD, got %08lx\n", GetLastError());
    if (ret)
        LocalFree(buf);
    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, SPC_INDIRECT_DATA_CONTENT_STRUCT,
     spcidcWithEverythingExceptDataValue, sizeof(spcidcWithEverythingExceptDataValue),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    /* huh??? */
    ok(!ret && GetLastError () == CRYPT_E_ASN1_EOD,
     "Expected CRYPT_E_ASN1_EOD, got %08lx\n", GetLastError());
    if (ret)
        LocalFree(buf);
    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, SPC_INDIRECT_DATA_CONTENT_STRUCT,
     spcidcWithEverything, sizeof(spcidcWithEverything),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    /* ?????? */
    ok(!ret && GetLastError () == CRYPT_E_ASN1_EOD,
     "Expected CRYPT_E_ASN1_EOD, got %08lx\n", GetLastError());
    if (ret)
        LocalFree(buf);
}

static WCHAR foo[] = { 'f','o','o',0 };
static WCHAR guidStr[] = { '{','8','b','c','9','6','b','0','0','-',
 '8','d','a','1','-','1','1','c','f','-','8','7','3','6','-','0','0',
 'a','a','0','0','a','4','8','5','e','b','}',0 };

static const BYTE emptyCatMemberInfo[] = { 0x30,0x05,0x1e,0x00,0x02,0x01,0x00 };
static const BYTE catMemberInfoWithSillyGuid[] = {
0x30,0x0b,0x1e,0x06,0x00,0x66,0x00,0x6f,0x00,0x6f,0x02,0x01,0x00 };
static const BYTE catMemberInfoWithGuid[] = {
0x30,0x51,0x1e,0x4c,0x00,0x7b,0x00,0x38,0x00,0x62,0x00,0x63,0x00,0x39,0x00,0x36,
0x00,0x62,0x00,0x30,0x00,0x30,0x00,0x2d,0x00,0x38,0x00,0x64,0x00,0x61,0x00,0x31,
0x00,0x2d,0x00,0x31,0x00,0x31,0x00,0x63,0x00,0x66,0x00,0x2d,0x00,0x38,0x00,0x37,
0x00,0x33,0x00,0x36,0x00,0x2d,0x00,0x30,0x00,0x30,0x00,0x61,0x00,0x61,0x00,0x30,
0x00,0x30,0x00,0x61,0x00,0x34,0x00,0x38,0x00,0x35,0x00,0x65,0x00,0x62,0x00,0x7d,
0x02,0x01,0x00 };

static void test_encodeCatMemberInfo(void)
{
    CAT_MEMBERINFO info;
    BOOL ret;
    DWORD size = 0;
    LPBYTE buf;

    memset(&info, 0, sizeof(info));

    if (!pCryptEncodeObjectEx)
    {
        win_skip("CryptEncodeObjectEx() is not available. Skipping the encodeCatMemberInfo tests\n");
        return;
    }

    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, CAT_MEMBERINFO_STRUCT,
     &info, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptyCatMemberInfo), "Unexpected size %ld\n", size);
        ok(!memcmp(buf, emptyCatMemberInfo, sizeof(emptyCatMemberInfo)),
         "Unexpected value\n");
        LocalFree(buf);
    }
    info.pwszSubjGuid = foo;
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, CAT_MEMBERINFO_STRUCT,
     &info, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(catMemberInfoWithSillyGuid), "Unexpected size %ld\n",
         size);
        ok(!memcmp(buf, catMemberInfoWithSillyGuid,
         sizeof(catMemberInfoWithSillyGuid)), "Unexpected value\n");
        LocalFree(buf);
    }
    info.pwszSubjGuid = guidStr;
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, CAT_MEMBERINFO_STRUCT,
     &info, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(catMemberInfoWithGuid), "Unexpected size %ld\n",
         size);
        ok(!memcmp(buf, catMemberInfoWithGuid, sizeof(catMemberInfoWithGuid)),
         "Unexpected value\n");
        LocalFree(buf);
    }
}

static void test_decodeCatMemberInfo(void)
{
   BOOL ret;
   LPBYTE buf;
   DWORD size;
   CAT_MEMBERINFO *info;

    if (!pCryptDecodeObjectEx)
    {
        win_skip("CryptDecodeObjectEx() is not available. Skipping the decodeCatMemberInfo tests\n");
        return;
    }

    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, CAT_MEMBERINFO_STRUCT,
     emptyCatMemberInfo, sizeof(emptyCatMemberInfo),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        info = (CAT_MEMBERINFO *)buf;
        ok(!info->pwszSubjGuid || !info->pwszSubjGuid[0],
         "expected empty pwszSubjGuid\n");
        ok(info->dwCertVersion == 0, "expected dwCertVersion == 0, got %ld\n",
         info->dwCertVersion);
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, CAT_MEMBERINFO_STRUCT,
     catMemberInfoWithSillyGuid, sizeof(catMemberInfoWithSillyGuid),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        info = (CAT_MEMBERINFO *)buf;
        ok(info->pwszSubjGuid && !lstrcmpW(info->pwszSubjGuid, foo),
         "unexpected pwszSubjGuid\n");
        ok(info->dwCertVersion == 0, "expected dwCertVersion == 0, got %ld\n",
         info->dwCertVersion);
        LocalFree(buf);
    }
    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, CAT_MEMBERINFO_STRUCT,
     catMemberInfoWithGuid, sizeof(catMemberInfoWithGuid),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        info = (CAT_MEMBERINFO *)buf;
        ok(info->pwszSubjGuid && !lstrcmpW(info->pwszSubjGuid, guidStr),
         "unexpected pwszSubjGuid\n");
        ok(info->dwCertVersion == 0, "expected dwCertVersion == 0, got %ld\n",
         info->dwCertVersion);
        LocalFree(buf);
    }
}

static const BYTE emptyCatNameValue[] = {
0x30,0x07,0x1e,0x00,0x02,0x01,0x00,0x04,0x00 };
static const BYTE catNameValueWithTag[] = {
0x30,0x0d,0x1e,0x06,0x00,0x66,0x00,0x6f,0x00,0x6f,0x02,0x01,0x00,0x04,0x00 };
static const BYTE catNameValueWithFlags[] = {
0x30,0x0a,0x1e,0x00,0x02,0x04,0xf0,0x0d,0xd0,0x0d,0x04,0x00 };
static const BYTE catNameValueWithValue[] = {
0x30,0x0b,0x1e,0x00,0x02,0x01,0x00,0x04,0x04,0x01,0x02,0x03,0x04 };

static BYTE aVal[] = { 1,2,3,4 };

static void test_encodeCatNameValue(void)
{
    static WCHAR foo[] = { 'f','o','o',0 };
    BOOL ret;
    LPBYTE buf;
    DWORD size;
    CAT_NAMEVALUE value;

    memset(&value, 0, sizeof(value));
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, CAT_NAMEVALUE_STRUCT,
     &value, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptyCatNameValue), "Unexpected size %ld\n", size);
        ok(!memcmp(buf, emptyCatNameValue, sizeof(emptyCatNameValue)),
         "Unexpected value\n");
        LocalFree(buf);
    }
    value.pwszTag = foo;
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, CAT_NAMEVALUE_STRUCT,
     &value, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(catNameValueWithTag), "Unexpected size %ld\n", size);
        ok(!memcmp(buf, catNameValueWithTag, sizeof(catNameValueWithTag)),
         "Unexpected value\n");
        LocalFree(buf);
    }
    value.pwszTag = NULL;
    value.fdwFlags = 0xf00dd00d;
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, CAT_NAMEVALUE_STRUCT,
     &value, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(catNameValueWithFlags), "Unexpected size %ld\n", size);
        ok(!memcmp(buf, catNameValueWithFlags, sizeof(catNameValueWithFlags)),
         "Unexpected value\n");
        LocalFree(buf);
    }
    value.fdwFlags = 0;
    value.Value.cbData = sizeof(aVal);
    value.Value.pbData = aVal;
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, CAT_NAMEVALUE_STRUCT,
     &value, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(catNameValueWithValue), "Unexpected size %ld\n", size);
        ok(!memcmp(buf, catNameValueWithValue, sizeof(catNameValueWithValue)),
         "Unexpected value\n");
        LocalFree(buf);
    }
}

static void test_decodeCatNameValue(void)
{
    BOOL ret;
    LPBYTE buf;
    DWORD size;
    CAT_NAMEVALUE *value;

    buf = NULL;
    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, CAT_NAMEVALUE_STRUCT,
     emptyCatNameValue, sizeof(emptyCatNameValue),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        value = (CAT_NAMEVALUE *)buf;
        ok(!value->pwszTag || !value->pwszTag[0], "expected empty pwszTag\n");
        ok(value->fdwFlags == 0, "expected fdwFlags == 0, got %08lx\n",
         value->fdwFlags);
        ok(value->Value.cbData == 0, "expected 0-length value, got %ld\n",
         value->Value.cbData);
        LocalFree(buf);
    }
    buf = NULL;
    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, CAT_NAMEVALUE_STRUCT,
     catNameValueWithTag, sizeof(catNameValueWithTag),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        value = (CAT_NAMEVALUE *)buf;
        ok(value->pwszTag && !lstrcmpW(value->pwszTag, foo),
         "unexpected pwszTag\n");
        ok(value->fdwFlags == 0, "expected fdwFlags == 0, got %08lx\n",
         value->fdwFlags);
        ok(value->Value.cbData == 0, "expected 0-length value, got %ld\n",
         value->Value.cbData);
        LocalFree(buf);
    }
    buf = NULL;
    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, CAT_NAMEVALUE_STRUCT,
     catNameValueWithFlags, sizeof(catNameValueWithFlags),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        value = (CAT_NAMEVALUE *)buf;
        ok(!value->pwszTag || !value->pwszTag[0], "expected empty pwszTag\n");
        ok(value->fdwFlags == 0xf00dd00d,
         "expected fdwFlags == 0xf00dd00d, got %08lx\n", value->fdwFlags);
        ok(value->Value.cbData == 0, "expected 0-length value, got %ld\n",
         value->Value.cbData);
        LocalFree(buf);
    }
    buf = NULL;
    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, CAT_NAMEVALUE_STRUCT,
     catNameValueWithValue, sizeof(catNameValueWithValue),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        value = (CAT_NAMEVALUE *)buf;
        ok(!value->pwszTag || !value->pwszTag[0], "expected empty pwszTag\n");
        ok(value->fdwFlags == 0, "expected fdwFlags == 0, got %08lx\n",
         value->fdwFlags);
        ok(value->Value.cbData == sizeof(aVal), "unexpected size %ld\n",
         value->Value.cbData);
        ok(!memcmp(value->Value.pbData, aVal, value->Value.cbData),
         "unexpected value\n");
        LocalFree(buf);
    }
}

static const WCHAR progName[] = { 'A',' ','p','r','o','g','r','a','m',0 };
static const BYTE spOpusInfoWithProgramName[] = {
0x30,0x16,0xa0,0x14,0x80,0x12,0x00,0x41,0x00,0x20,0x00,0x70,0x00,0x72,0x00,0x6f,
0x00,0x67,0x00,0x72,0x00,0x61,0x00,0x6d };
static WCHAR winehq[] = { 'h','t','t','p',':','/','/','w','i','n','e','h','q',
 '.','o','r','g','/',0 };
static const BYTE spOpusInfoWithMoreInfo[] = {
0x30,0x16,0xa1,0x14,0x80,0x12,0x68,0x74,0x74,0x70,0x3a,0x2f,0x2f,0x77,0x69,0x6e,
0x65,0x68,0x71,0x2e,0x6f,0x72,0x67,0x2f };
static const BYTE spOpusInfoWithPublisherInfo[] = {
0x30,0x16,0xa2,0x14,0x80,0x12,0x68,0x74,0x74,0x70,0x3a,0x2f,0x2f,0x77,0x69,0x6e,
0x65,0x68,0x71,0x2e,0x6f,0x72,0x67,0x2f };

static void test_encodeSpOpusInfo(void)
{
    BOOL ret;
    LPBYTE buf;
    DWORD size;
    SPC_SP_OPUS_INFO info;
    SPC_LINK moreInfo;

    memset(&info, 0, sizeof(info));
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_SP_OPUS_INFO_STRUCT,
     &info, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptySequence), "unexpected size %ld\n", size);
        ok(!memcmp(buf, emptySequence, size), "unexpected value\n");
        LocalFree(buf);
    }
    info.pwszProgramName = progName;
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_SP_OPUS_INFO_STRUCT,
     &info, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(spOpusInfoWithProgramName), "unexpected size %ld\n",
         size);
        ok(!memcmp(buf, spOpusInfoWithProgramName, size),
         "unexpected value\n");
        LocalFree(buf);
    }
    info.pwszProgramName = NULL;
    memset(&moreInfo, 0, sizeof(moreInfo));
    info.pMoreInfo = &moreInfo;
    SetLastError(0xdeadbeef);
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_SP_OPUS_INFO_STRUCT,
     &info, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "expected E_INVALIDARG, got %08lx\n", GetLastError());
    moreInfo.dwLinkChoice = SPC_URL_LINK_CHOICE;
    moreInfo.pwszUrl = winehq;
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_SP_OPUS_INFO_STRUCT,
     &info, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(spOpusInfoWithMoreInfo),
         "unexpected size %ld\n", size);
        ok(!memcmp(buf, spOpusInfoWithMoreInfo, size),
         "unexpected value\n");
        LocalFree(buf);
    }
    info.pMoreInfo = NULL;
    info.pPublisherInfo = &moreInfo;
    ret = pCryptEncodeObjectEx(X509_ASN_ENCODING, SPC_SP_OPUS_INFO_STRUCT,
     &info, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(spOpusInfoWithPublisherInfo),
         "unexpected size %ld\n", size);
        ok(!memcmp(buf, spOpusInfoWithPublisherInfo, size),
         "unexpected value\n");
        LocalFree(buf);
    }
}

static void test_decodeSpOpusInfo(void)
{
    BOOL ret;
    DWORD size;
    SPC_SP_OPUS_INFO *info;

    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, SPC_SP_OPUS_INFO_STRUCT,
     emptySequence, sizeof(emptySequence), CRYPT_DECODE_ALLOC_FLAG, NULL,
     &info, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(!info->pwszProgramName, "expected NULL\n");
        ok(!info->pMoreInfo, "expected NULL\n");
        ok(!info->pPublisherInfo, "expected NULL\n");
        LocalFree(info);
    }
    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, SPC_SP_OPUS_INFO_STRUCT,
     spOpusInfoWithProgramName, sizeof(spOpusInfoWithProgramName),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &info, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(info->pwszProgramName && !lstrcmpW(info->pwszProgramName,
         progName), "unexpected program name\n");
        ok(!info->pMoreInfo, "expected NULL\n");
        ok(!info->pPublisherInfo, "expected NULL\n");
        LocalFree(info);
    }
    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, SPC_SP_OPUS_INFO_STRUCT,
     spOpusInfoWithMoreInfo, sizeof(spOpusInfoWithMoreInfo),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &info, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(!info->pwszProgramName, "expected NULL\n");
        ok(info->pMoreInfo != NULL, "expected a value for pMoreInfo\n");
        if (info->pMoreInfo)
        {
            ok(info->pMoreInfo->dwLinkChoice == SPC_URL_LINK_CHOICE,
             "unexpected link choice %ld\n", info->pMoreInfo->dwLinkChoice);
            ok(!lstrcmpW(info->pMoreInfo->pwszUrl, winehq),
             "unexpected link value\n");
        }
        ok(!info->pPublisherInfo, "expected NULL\n");
        LocalFree(info);
    }
    ret = pCryptDecodeObjectEx(X509_ASN_ENCODING, SPC_SP_OPUS_INFO_STRUCT,
     spOpusInfoWithPublisherInfo, sizeof(spOpusInfoWithPublisherInfo),
     CRYPT_DECODE_ALLOC_FLAG, NULL, &info, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(!info->pwszProgramName, "expected NULL\n");
        ok(!info->pMoreInfo, "expected NULL\n");
        ok(info->pPublisherInfo != NULL,
         "expected a value for pPublisherInfo\n");
        if (info->pPublisherInfo)
        {
            ok(info->pPublisherInfo->dwLinkChoice == SPC_URL_LINK_CHOICE,
             "unexpected link choice %ld\n",
             info->pPublisherInfo->dwLinkChoice);
            ok(!lstrcmpW(info->pPublisherInfo->pwszUrl, winehq),
             "unexpected link value\n");
        }
        LocalFree(info);
    }
}

START_TEST(asn)
{
    HMODULE hCrypt32 = LoadLibraryA("crypt32.dll");
    pCryptDecodeObjectEx = (void*)GetProcAddress(hCrypt32, "CryptDecodeObjectEx");
    pCryptEncodeObjectEx = (void*)GetProcAddress(hCrypt32, "CryptEncodeObjectEx");

    test_encodeSPCFinancialCriteria();
    test_decodeSPCFinancialCriteria();
    test_encodeSPCLink();
    test_decodeSPCLink();
    test_encodeSPCPEImage();
    test_decodeSPCPEImage();
    test_encodeSPCIndirectDataContent();
    test_decodeSPCIndirectDataContent();
    test_encodeCatMemberInfo();
    test_decodeCatMemberInfo();
    test_encodeCatNameValue();
    test_decodeCatNameValue();
    test_encodeSpOpusInfo();
    test_decodeSpOpusInfo();

    FreeLibrary(hCrypt32);
}
