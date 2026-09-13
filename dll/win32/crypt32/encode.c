/*
 * Copyright 2005-2008 Juan Lang
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
 * This file implements ASN.1 DER encoding of a limited set of types.
 * It isn't a full ASN.1 implementation.  Microsoft implements BER
 * encoding of many of the basic types in msasn1.dll, but that interface isn't
 * implemented, so I implement them here.
 *
 * References:
 * "A Layman's Guide to a Subset of ASN.1, BER, and DER", by Burton Kaliski
 * (available online, look for a PDF copy as the HTML versions tend to have
 * translation errors.)
 *
 * RFC3280, http://www.faqs.org/rfcs/rfc3280.html
 *
 * MSDN, especially "Constants for CryptEncodeObject and CryptDecodeObject"
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "snmp.h"
#include "wine/debug.h"
#include "wine/exception.h"
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(cryptasn);
WINE_DECLARE_DEBUG_CHANNEL(crypt);

typedef BOOL (WINAPI *CryptEncodeObjectFunc)(DWORD, LPCSTR, const void *,
 BYTE *, DWORD *);

/* Prototypes for built-in encoders.  They follow the Ex style prototypes.
 * The dwCertEncodingType and lpszStructType are ignored by the built-in
 * functions, but the parameters are retained to simplify CryptEncodeObjectEx,
 * since it must call functions in external DLLs that follow these signatures.
 */
BOOL WINAPI CRYPT_AsnEncodeOid(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeExtensions(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeSequenceOfAny(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeBool(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodePubKeyInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeBits(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeBitsSwapBytes(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeInt(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeInteger(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeUnsignedInteger(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeChoiceOfTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeEnhancedKeyUsage(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodePKCSAttributes(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);

BOOL CRYPT_EncodeEnsureSpace(DWORD dwFlags, const CRYPT_ENCODE_PARA *pEncodePara,
 BYTE *pbEncoded, DWORD *pcbEncoded, DWORD bytesNeeded)
{
    BOOL ret = TRUE;

    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
    {
        if (pEncodePara && pEncodePara->pfnAlloc)
            *(BYTE **)pbEncoded = pEncodePara->pfnAlloc(bytesNeeded);
        else
            *(BYTE **)pbEncoded = LocalAlloc(0, bytesNeeded);
        if (!*(BYTE **)pbEncoded)
            ret = FALSE;
        else
            *pcbEncoded = bytesNeeded;
    }
    else if (bytesNeeded > *pcbEncoded)
    {
        *pcbEncoded = bytesNeeded;
        SetLastError(ERROR_MORE_DATA);
        ret = FALSE;
    }
    else
        *pcbEncoded = bytesNeeded;
    return ret;
}

static void CRYPT_FreeSpace(const CRYPT_ENCODE_PARA *pEncodePara, LPVOID pv)
{
    if (pEncodePara && pEncodePara->pfnFree)
        pEncodePara->pfnFree(pv);
    else
        LocalFree(pv);
}

BOOL CRYPT_EncodeLen(DWORD len, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    DWORD bytesNeeded, significantBytes = 0;

    if (len <= 0x7f)
        bytesNeeded = 1;
    else
    {
        DWORD temp;

        for (temp = len, significantBytes = sizeof(temp); !(temp & 0xff000000);
         temp <<= 8, significantBytes--)
            ;
        bytesNeeded = significantBytes + 1;
    }
    if (!pbEncoded)
    {
        *pcbEncoded = bytesNeeded;
        return TRUE;
    }
    if (*pcbEncoded < bytesNeeded)
    {
        SetLastError(ERROR_MORE_DATA);
        return FALSE;
    }
    if (len <= 0x7f)
        *pbEncoded = (BYTE)len;
    else
    {
        DWORD i;

        *pbEncoded++ = significantBytes | 0x80;
        for (i = 0; i < significantBytes; i++)
        {
            *(pbEncoded + significantBytes - i - 1) = (BYTE)(len & 0xff);
            len >>= 8;
        }
    }
    *pcbEncoded = bytesNeeded;
    return TRUE;
}

BOOL WINAPI CRYPT_AsnEncodeSequence(DWORD dwCertEncodingType,
 struct AsnEncodeSequenceItem items[], DWORD cItem, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;
    DWORD i, dataLen = 0;

    TRACE("%p, %ld, %08lx, %p, %p, %ld\n", items, cItem, dwFlags, pEncodePara,
     pbEncoded, pbEncoded ? *pcbEncoded : 0);
    for (i = 0, ret = TRUE; ret && i < cItem; i++)
    {
        ret = items[i].encodeFunc(dwCertEncodingType, NULL,
         items[i].pvStructInfo, dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG, NULL,
         NULL, &items[i].size);
        /* Some functions propagate their errors through the size */
        if (!ret)
            *pcbEncoded = items[i].size;
        dataLen += items[i].size;
    }
    if (ret)
    {
        DWORD lenBytes, bytesNeeded;

        CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
        bytesNeeded = 1 + lenBytes + dataLen;
        if (!pbEncoded)
            *pcbEncoded = bytesNeeded;
        else
        {
            if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
             pcbEncoded, bytesNeeded)))
            {
                BYTE *out;

                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                out = pbEncoded;
                *out++ = ASN_SEQUENCE;
                CRYPT_EncodeLen(dataLen, out, &lenBytes);
                out += lenBytes;
                for (i = 0; ret && i < cItem; i++)
                {
                    ret = items[i].encodeFunc(dwCertEncodingType, NULL,
                     items[i].pvStructInfo, dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG,
                     NULL, out, &items[i].size);
                    /* Some functions propagate their errors through the size */
                    if (!ret)
                        *pcbEncoded = items[i].size;
                    out += items[i].size;
                }
                if (!ret && (dwFlags & CRYPT_ENCODE_ALLOC_FLAG))
                    CRYPT_FreeSpace(pEncodePara, pbEncoded);
            }
        }
    }
    TRACE("returning %d (%08lx)\n", ret, GetLastError());
    return ret;
}

BOOL WINAPI CRYPT_AsnEncodeConstructed(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;
    const struct AsnConstructedItem *item = pvStructInfo;
    DWORD len;

    if ((ret = item->encodeFunc(dwCertEncodingType, lpszStructType,
     item->pvStructInfo, dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG, NULL, NULL, &len)))
    {
        DWORD dataLen, bytesNeeded;

        CRYPT_EncodeLen(len, NULL, &dataLen);
        bytesNeeded = 1 + dataLen + len;
        if (!pbEncoded)
            *pcbEncoded = bytesNeeded;
        else if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
         pbEncoded, pcbEncoded, bytesNeeded)))
        {
            BYTE *out;

            if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                pbEncoded = *(BYTE **)pbEncoded;
            out = pbEncoded;
            *out++ = ASN_CONTEXT | ASN_CONSTRUCTOR | item->tag;
            CRYPT_EncodeLen(len, out, &dataLen);
            out += dataLen;
            ret = item->encodeFunc(dwCertEncodingType, lpszStructType,
             item->pvStructInfo, dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG, NULL,
             out, &len);
            if (!ret)
            {
                /* Some functions propagate their errors through the size */
                *pcbEncoded = len;
                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    CRYPT_FreeSpace(pEncodePara, pbEncoded);
            }
        }
    }
    else
    {
        /* Some functions propagate their errors through the size */
        *pcbEncoded = len;
    }
    return ret;
}

struct AsnEncodeTagSwappedItem
{
    BYTE                    tag;
    const void             *pvStructInfo;
    CryptEncodeObjectExFunc encodeFunc;
};

/* Sort of a wacky hack, it encodes something using the struct
 * AsnEncodeTagSwappedItem's encodeFunc, then replaces the tag byte with the tag
 * given in the struct AsnEncodeTagSwappedItem.
 */
static BOOL WINAPI CRYPT_AsnEncodeSwapTag(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;
    const struct AsnEncodeTagSwappedItem *item = pvStructInfo;

    ret = item->encodeFunc(dwCertEncodingType, lpszStructType,
     item->pvStructInfo, dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    if (ret && pbEncoded)
        *pbEncoded = item->tag;
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeCertVersion(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    const DWORD *ver = pvStructInfo;
    BOOL ret;

    /* CERT_V1 is not encoded */
    if (*ver == CERT_V1)
    {
        *pcbEncoded = 0;
        ret = TRUE;
    }
    else
    {
        struct AsnConstructedItem item = { 0, ver, CRYPT_AsnEncodeInt };

        ret = CRYPT_AsnEncodeConstructed(dwCertEncodingType, X509_INTEGER,
         &item, dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    }
    return ret;
}

static BOOL WINAPI CRYPT_CopyEncodedBlob(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    const CRYPT_DER_BLOB *blob = pvStructInfo;
    BOOL ret;

    if (!pbEncoded)
    {
        *pcbEncoded = blob->cbData;
        ret = TRUE;
    }
    else
    {
        if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
         pcbEncoded, blob->cbData)))
        {
            if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                pbEncoded = *(BYTE **)pbEncoded;
            if (blob->cbData)
                memcpy(pbEncoded, blob->pbData, blob->cbData);
            *pcbEncoded = blob->cbData;
        }
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeValidity(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;
    /* This has two filetimes in a row, a NotBefore and a NotAfter */
    const FILETIME *timePtr = pvStructInfo;
    struct AsnEncodeSequenceItem items[] = {
     { timePtr,     CRYPT_AsnEncodeChoiceOfTime, 0 },
     { timePtr + 1, CRYPT_AsnEncodeChoiceOfTime, 0 },
    };

    ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, 
     ARRAY_SIZE(items), dwFlags, pEncodePara, pbEncoded,
     pcbEncoded);
    return ret;
}

/* Like CRYPT_AsnEncodeAlgorithmId, but encodes parameters as an asn.1 NULL
 * if they are empty.
 */
static BOOL WINAPI CRYPT_AsnEncodeAlgorithmIdWithNullParams(
 DWORD dwCertEncodingType, LPCSTR lpszStructType, const void *pvStructInfo,
 DWORD dwFlags, PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    const CRYPT_ALGORITHM_IDENTIFIER *algo = pvStructInfo;
    static const BYTE asn1Null[] = { ASN_NULL, 0 };
    static const CRYPT_DATA_BLOB nullBlob = { sizeof(asn1Null),
     (LPBYTE)asn1Null };
    BOOL ret;
    struct AsnEncodeSequenceItem items[2] = {
     { algo->pszObjId, CRYPT_AsnEncodeOid, 0 },
     { NULL,           CRYPT_CopyEncodedBlob, 0 },
    };

    if (algo->Parameters.cbData)
        items[1].pvStructInfo = &algo->Parameters;
    else
        items[1].pvStructInfo = &nullBlob;
    ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items,
     ARRAY_SIZE(items), dwFlags, pEncodePara, pbEncoded,
     pcbEncoded);
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeAlgorithmId(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    const CRYPT_ALGORITHM_IDENTIFIER *algo = pvStructInfo;
    BOOL ret;
    struct AsnEncodeSequenceItem items[] = {
     { algo->pszObjId,    CRYPT_AsnEncodeOid, 0 },
     { &algo->Parameters, CRYPT_CopyEncodedBlob, 0 },
    };

    ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items,
     ARRAY_SIZE(items), dwFlags, pEncodePara, pbEncoded,
     pcbEncoded);
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodePubKeyInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CERT_PUBLIC_KEY_INFO *info = pvStructInfo;
        struct AsnEncodeSequenceItem items[] = {
         { &info->Algorithm, CRYPT_AsnEncodeAlgorithmIdWithNullParams, 0 },
         { &info->PublicKey, CRYPT_AsnEncodeBits, 0 },
        };

        TRACE("Encoding public key with OID %s\n",
         debugstr_a(info->Algorithm.pszObjId));
        ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items,
         ARRAY_SIZE(items), dwFlags, pEncodePara, pbEncoded,
         pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeCert(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CERT_SIGNED_CONTENT_INFO *info = pvStructInfo;
        struct AsnEncodeSequenceItem items[] = {
         { &info->ToBeSigned,         CRYPT_CopyEncodedBlob, 0 },
         { &info->SignatureAlgorithm, CRYPT_AsnEncodeAlgorithmId, 0 },
         { &info->Signature,          CRYPT_AsnEncodeBitsSwapBytes, 0 },
        };

        if (dwFlags & CRYPT_ENCODE_NO_SIGNATURE_BYTE_REVERSAL_FLAG)
            items[2].encodeFunc = CRYPT_AsnEncodeBits;
        ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, 
         ARRAY_SIZE(items), dwFlags, pEncodePara, pbEncoded,
         pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

BOOL WINAPI CRYPT_AsnEncodePubKeyInfoNoNull(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;
    const CERT_PUBLIC_KEY_INFO *info = pvStructInfo;
    struct AsnEncodeSequenceItem items[] = {
     { &info->Algorithm, CRYPT_AsnEncodeAlgorithmId, 0 },
     { &info->PublicKey, CRYPT_AsnEncodeBits, 0 },
    };

    TRACE("Encoding public key with OID %s\n",
     debugstr_a(info->Algorithm.pszObjId));
    ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items,
     ARRAY_SIZE(items), dwFlags, pEncodePara, pbEncoded,
     pcbEncoded);
    return ret;
}

/* Like in Windows, this blithely ignores the validity of the passed-in
 * CERT_INFO, and just encodes it as-is.  The resulting encoded data may not
 * decode properly, see CRYPT_AsnDecodeCertInfo.
 */
static BOOL WINAPI CRYPT_AsnEncodeCertInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CERT_INFO *info = pvStructInfo;
        struct AsnEncodeSequenceItem items[10] = {
         { &info->dwVersion,            CRYPT_AsnEncodeCertVersion, 0 },
         { &info->SerialNumber,         CRYPT_AsnEncodeInteger, 0 },
         { &info->SignatureAlgorithm,   CRYPT_AsnEncodeAlgorithmId, 0 },
         { &info->Issuer,               CRYPT_CopyEncodedBlob, 0 },
         { &info->NotBefore,            CRYPT_AsnEncodeValidity, 0 },
         { &info->Subject,              CRYPT_CopyEncodedBlob, 0 },
         { &info->SubjectPublicKeyInfo, CRYPT_AsnEncodePubKeyInfoNoNull, 0 },
         { 0 }
        };
        struct AsnConstructedItem constructed = { 0 };
        struct AsnEncodeTagSwappedItem swapped[2] = { { 0 } };
        DWORD cItem = 7, cSwapped = 0;

        if (info->IssuerUniqueId.cbData)
        {
            swapped[cSwapped].tag = ASN_CONTEXT | 1;
            swapped[cSwapped].pvStructInfo = &info->IssuerUniqueId;
            swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeBits;
            items[cItem].pvStructInfo = &swapped[cSwapped];
            items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
            cSwapped++;
            cItem++;
        }
        if (info->SubjectUniqueId.cbData)
        {
            swapped[cSwapped].tag = ASN_CONTEXT | 2;
            swapped[cSwapped].pvStructInfo = &info->SubjectUniqueId;
            swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeBits;
            items[cItem].pvStructInfo = &swapped[cSwapped];
            items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
            cSwapped++;
            cItem++;
        }
        if (info->cExtension)
        {
            constructed.tag = 3;
            constructed.pvStructInfo = &info->cExtension;
            constructed.encodeFunc = CRYPT_AsnEncodeExtensions;
            items[cItem].pvStructInfo = &constructed;
            items[cItem].encodeFunc = CRYPT_AsnEncodeConstructed;
            cItem++;
        }

        ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, cItem,
         dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL CRYPT_AsnEncodeCRLEntry(const CRL_ENTRY *entry,
 BYTE *pbEncoded, DWORD *pcbEncoded)
{
    struct AsnEncodeSequenceItem items[3] = {
     { &entry->SerialNumber,   CRYPT_AsnEncodeInteger, 0 },
     { &entry->RevocationDate, CRYPT_AsnEncodeChoiceOfTime, 0 },
     { 0 }
    };
    DWORD cItem = 2;
    BOOL ret;

    TRACE("%p, %p, %ld\n", entry, pbEncoded, pbEncoded ? *pcbEncoded : 0);

    if (entry->cExtension)
    {
        items[cItem].pvStructInfo = &entry->cExtension;
        items[cItem].encodeFunc = CRYPT_AsnEncodeExtensions;
        cItem++;
    }

    ret = CRYPT_AsnEncodeSequence(X509_ASN_ENCODING, items, cItem, 0, NULL,
     pbEncoded, pcbEncoded);

    TRACE("returning %d (%08lx)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeCRLEntries(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    DWORD bytesNeeded, dataLen, lenBytes, i;
    const CRL_INFO *info = pvStructInfo;
    const CRL_ENTRY *rgCRLEntry = info->rgCRLEntry;
    BOOL ret = TRUE;

    for (i = 0, dataLen = 0; ret && i < info->cCRLEntry; i++)
    {
        DWORD size;

        ret = CRYPT_AsnEncodeCRLEntry(&rgCRLEntry[i], NULL, &size);
        if (ret)
            dataLen += size;
    }
    if (ret)
    {
        CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
        bytesNeeded = 1 + lenBytes + dataLen;
        if (!pbEncoded)
            *pcbEncoded = bytesNeeded;
        else
        {
            if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
             pcbEncoded, bytesNeeded)))
            {
                BYTE *out;

                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                out = pbEncoded;
                *out++ = ASN_SEQUENCEOF;
                CRYPT_EncodeLen(dataLen, out, &lenBytes);
                out += lenBytes;
                for (i = 0; i < info->cCRLEntry; i++)
                {
                    DWORD size = dataLen;

                    ret = CRYPT_AsnEncodeCRLEntry(&rgCRLEntry[i], out, &size);
                    out += size;
                    dataLen -= size;
                }
                if (!ret && (dwFlags & CRYPT_ENCODE_ALLOC_FLAG))
                    CRYPT_FreeSpace(pEncodePara, pbEncoded);
            }
        }
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeCRLVersion(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    const DWORD *ver = pvStructInfo;
    BOOL ret;

    /* CRL_V1 is not encoded */
    if (*ver == CRL_V1)
    {
        *pcbEncoded = 0;
        ret = TRUE;
    }
    else
        ret = CRYPT_AsnEncodeInt(dwCertEncodingType, X509_INTEGER, ver,
         dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    return ret;
}

/* Like in Windows, this blithely ignores the validity of the passed-in
 * CRL_INFO, and just encodes it as-is.  The resulting encoded data may not
 * decode properly, see CRYPT_AsnDecodeCRLInfo.
 */
static BOOL WINAPI CRYPT_AsnEncodeCRLInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CRL_INFO *info = pvStructInfo;
        struct AsnEncodeSequenceItem items[7] = {
         { &info->dwVersion,          CRYPT_AsnEncodeCRLVersion, 0 },
         { &info->SignatureAlgorithm, CRYPT_AsnEncodeAlgorithmId, 0 },
         { &info->Issuer,             CRYPT_CopyEncodedBlob, 0 },
         { &info->ThisUpdate,         CRYPT_AsnEncodeChoiceOfTime, 0 },
         { 0 }
        };
        struct AsnConstructedItem constructed[1] = { { 0 } };
        DWORD cItem = 4, cConstructed = 0;

        if (info->NextUpdate.dwLowDateTime || info->NextUpdate.dwHighDateTime)
        {
            items[cItem].pvStructInfo = &info->NextUpdate;
            items[cItem].encodeFunc = CRYPT_AsnEncodeChoiceOfTime;
            cItem++;
        }
        if (info->cCRLEntry)
        {
            items[cItem].pvStructInfo = info;
            items[cItem].encodeFunc = CRYPT_AsnEncodeCRLEntries;
            cItem++;
        }
        if (info->cExtension)
        {
            constructed[cConstructed].tag = 0;
            constructed[cConstructed].pvStructInfo = &info->cExtension;
            constructed[cConstructed].encodeFunc = CRYPT_AsnEncodeExtensions;
            items[cItem].pvStructInfo = &constructed[cConstructed];
            items[cItem].encodeFunc = CRYPT_AsnEncodeConstructed;
            cConstructed++;
            cItem++;
        }

        ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, cItem,
         dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL CRYPT_AsnEncodeExtension(CERT_EXTENSION *ext, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    BOOL ret;
    struct AsnEncodeSequenceItem items[3] = {
     { ext->pszObjId, CRYPT_AsnEncodeOid, 0 },
     { NULL, NULL, 0 },
     { NULL, NULL, 0 },
    };
    DWORD cItem = 1;

    TRACE("%p, %p, %ld\n", ext, pbEncoded, pbEncoded ? *pcbEncoded : 0);

    if (ext->fCritical)
    {
        items[cItem].pvStructInfo = &ext->fCritical;
        items[cItem].encodeFunc = CRYPT_AsnEncodeBool;
        cItem++;
    }
    items[cItem].pvStructInfo = &ext->Value;
    items[cItem].encodeFunc = CRYPT_AsnEncodeOctets;
    cItem++;

    ret = CRYPT_AsnEncodeSequence(X509_ASN_ENCODING, items, cItem, 0, NULL,
     pbEncoded, pcbEncoded);
    TRACE("returning %d (%08lx)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeExtensions(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        DWORD bytesNeeded, dataLen, lenBytes, i;
        const CERT_EXTENSIONS *exts = pvStructInfo;

        ret = TRUE;
        for (i = 0, dataLen = 0; ret && i < exts->cExtension; i++)
        {
            DWORD size;

            ret = CRYPT_AsnEncodeExtension(&exts->rgExtension[i], NULL, &size);
            if (ret)
                dataLen += size;
        }
        if (ret)
        {
            CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
            bytesNeeded = 1 + lenBytes + dataLen;
            if (!pbEncoded)
                *pcbEncoded = bytesNeeded;
            else
            {
                if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
                 pbEncoded, pcbEncoded, bytesNeeded)))
                {
                    BYTE *out;

                    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                        pbEncoded = *(BYTE **)pbEncoded;
                    out = pbEncoded;
                    *out++ = ASN_SEQUENCEOF;
                    CRYPT_EncodeLen(dataLen, out, &lenBytes);
                    out += lenBytes;
                    for (i = 0; i < exts->cExtension; i++)
                    {
                        DWORD size = dataLen;

                        ret = CRYPT_AsnEncodeExtension(&exts->rgExtension[i],
                         out, &size);
                        out += size;
                        dataLen -= size;
                    }
                    if (!ret && (dwFlags & CRYPT_ENCODE_ALLOC_FLAG))
                        CRYPT_FreeSpace(pEncodePara, pbEncoded);
                }
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

BOOL WINAPI CRYPT_AsnEncodeOid(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    LPCSTR pszObjId = pvStructInfo;
    DWORD bytesNeeded = 0, lenBytes;
    BOOL ret = TRUE;
    int firstPos = 0;
    BYTE firstByte = 0;

    TRACE("%s\n", debugstr_a(pszObjId));

    if (pszObjId)
    {
        const char *ptr;
        int val1, val2;

        if (sscanf(pszObjId, "%d.%d%n", &val1, &val2, &firstPos) != 2)
        {
            SetLastError(CRYPT_E_ASN1_ERROR);
            return FALSE;
        }
        bytesNeeded++;
        firstByte = val1 * 40 + val2;
        ptr = pszObjId + firstPos;
        if (*ptr == '.')
        {
            ptr++;
            firstPos++;
        }
        while (ret && *ptr)
        {
            int pos;

            /* note I assume each component is at most 32-bits long in base 2 */
            if (sscanf(ptr, "%d%n", &val1, &pos) == 1)
            {
                if (val1 >= 0x10000000)
                    bytesNeeded += 5;
                else if (val1 >= 0x200000)
                    bytesNeeded += 4;
                else if (val1 >= 0x4000)
                    bytesNeeded += 3;
                else if (val1 >= 0x80)
                    bytesNeeded += 2;
                else
                    bytesNeeded += 1;
                ptr += pos;
                if (*ptr == '.')
                    ptr++;
            }
            else
            {
                SetLastError(CRYPT_E_ASN1_ERROR);
                return FALSE;
            }
        }
        CRYPT_EncodeLen(bytesNeeded, NULL, &lenBytes);
    }
    else
        lenBytes = 1;
    bytesNeeded += 1 + lenBytes;
    if (pbEncoded)
    {
        if (*pcbEncoded < bytesNeeded)
        {
            SetLastError(ERROR_MORE_DATA);
            ret = FALSE;
        }
        else
        {
            *pbEncoded++ = ASN_OBJECTIDENTIFIER;
            CRYPT_EncodeLen(bytesNeeded - 1 - lenBytes, pbEncoded, &lenBytes);
            pbEncoded += lenBytes;
            if (pszObjId)
            {
                const char *ptr;
                int val, pos;

                *pbEncoded++ = firstByte;
                ptr = pszObjId + firstPos;
                while (ret && *ptr)
                {
                    sscanf(ptr, "%d%n", &val, &pos);
                    {
                        unsigned char outBytes[5];
                        int numBytes, i;

                        if (val >= 0x10000000)
                            numBytes = 5;
                        else if (val >= 0x200000)
                            numBytes = 4;
                        else if (val >= 0x4000)
                            numBytes = 3;
                        else if (val >= 0x80)
                            numBytes = 2;
                        else
                            numBytes = 1;
                        for (i = numBytes; i > 0; i--)
                        {
                            outBytes[i - 1] = val & 0x7f;
                            val >>= 7;
                        }
                        for (i = 0; i < numBytes - 1; i++)
                            *pbEncoded++ = outBytes[i] | 0x80;
                        *pbEncoded++ = outBytes[i];
                        ptr += pos;
                        if (*ptr == '.')
                            ptr++;
                    }
                }
            }
        }
    }
    *pcbEncoded = bytesNeeded;
    return ret;
}

static BOOL CRYPT_AsnEncodeStringCoerce(const CERT_NAME_VALUE *value,
 BYTE tag, DWORD dwFlags, const CRYPT_ENCODE_PARA *pEncodePara, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    BOOL ret = TRUE;
    LPCSTR str = (LPCSTR)value->Value.pbData;
    DWORD bytesNeeded, lenBytes, encodedLen;

    encodedLen = value->Value.cbData ? value->Value.cbData : strlen(str);
    CRYPT_EncodeLen(encodedLen, NULL, &lenBytes);
    bytesNeeded = 1 + lenBytes + encodedLen;
    if (!pbEncoded)
        *pcbEncoded = bytesNeeded;
    else
    {
        if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
         pbEncoded, pcbEncoded, bytesNeeded)))
        {
            if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                pbEncoded = *(BYTE **)pbEncoded;
            *pbEncoded++ = tag;
            CRYPT_EncodeLen(encodedLen, pbEncoded, &lenBytes);
            pbEncoded += lenBytes;
            memcpy(pbEncoded, str, encodedLen);
        }
    }
    return ret;
}

static BOOL CRYPT_AsnEncodeBMPString(const CERT_NAME_VALUE *value,
 DWORD dwFlags, const CRYPT_ENCODE_PARA *pEncodePara, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    BOOL ret = TRUE;
    LPCWSTR str = (LPCWSTR)value->Value.pbData;
    DWORD bytesNeeded, lenBytes, strLen;

    if (value->Value.cbData)
        strLen = value->Value.cbData / sizeof(WCHAR);
    else if (value->Value.pbData)
        strLen = lstrlenW(str);
    else
        strLen = 0;
    CRYPT_EncodeLen(strLen * 2, NULL, &lenBytes);
    bytesNeeded = 1 + lenBytes + strLen * 2;
    if (!pbEncoded)
        *pcbEncoded = bytesNeeded;
    else
    {
        if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
         pbEncoded, pcbEncoded, bytesNeeded)))
        {
            DWORD i;

            if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                pbEncoded = *(BYTE **)pbEncoded;
            *pbEncoded++ = ASN_BMPSTRING;
            CRYPT_EncodeLen(strLen * 2, pbEncoded, &lenBytes);
            pbEncoded += lenBytes;
            for (i = 0; i < strLen; i++)
            {
                *pbEncoded++ = (str[i] & 0xff00) >> 8;
                *pbEncoded++ = str[i] & 0x00ff;
            }
        }
    }
    return ret;
}

static BOOL CRYPT_AsnEncodeUTF8String(const CERT_NAME_VALUE *value,
 DWORD dwFlags, const CRYPT_ENCODE_PARA *pEncodePara, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    BOOL ret = TRUE;
    LPCWSTR str = (LPCWSTR)value->Value.pbData;
    DWORD bytesNeeded, lenBytes, encodedLen, strLen;

    if (value->Value.cbData)
        strLen = value->Value.cbData / sizeof(WCHAR);
    else if (str)
        strLen = lstrlenW(str);
    else
        strLen = 0;
    encodedLen = WideCharToMultiByte(CP_UTF8, 0, str, strLen, NULL, 0, NULL,
     NULL);
    CRYPT_EncodeLen(encodedLen, NULL, &lenBytes);
    bytesNeeded = 1 + lenBytes + encodedLen;
    if (!pbEncoded)
        *pcbEncoded = bytesNeeded;
    else
    {
        if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
         pbEncoded, pcbEncoded, bytesNeeded)))
        {
            if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                pbEncoded = *(BYTE **)pbEncoded;
            *pbEncoded++ = ASN_UTF8STRING;
            CRYPT_EncodeLen(encodedLen, pbEncoded, &lenBytes);
            pbEncoded += lenBytes;
            WideCharToMultiByte(CP_UTF8, 0, str, strLen, (LPSTR)pbEncoded,
             bytesNeeded - lenBytes - 1, NULL, NULL);
        }
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeNameValue(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret = TRUE;

    __TRY
    {
        const CERT_NAME_VALUE *value = pvStructInfo;

        switch (value->dwValueType)
        {
        case CERT_RDN_ANY_TYPE:
            /* explicitly disallowed */
            SetLastError(E_INVALIDARG);
            ret = FALSE;
            break;
        case CERT_RDN_ENCODED_BLOB:
            ret = CRYPT_CopyEncodedBlob(dwCertEncodingType, NULL,
             &value->Value, dwFlags, pEncodePara, pbEncoded, pcbEncoded);
            break;
        case CERT_RDN_OCTET_STRING:
            ret = CRYPT_AsnEncodeStringCoerce(value, ASN_OCTETSTRING,
             dwFlags, pEncodePara, pbEncoded, pcbEncoded);
            break;
        case CERT_RDN_NUMERIC_STRING:
            ret = CRYPT_AsnEncodeStringCoerce(value, ASN_NUMERICSTRING,
             dwFlags, pEncodePara, pbEncoded, pcbEncoded);
            break;
        case CERT_RDN_PRINTABLE_STRING:
            ret = CRYPT_AsnEncodeStringCoerce(value, ASN_PRINTABLESTRING,
             dwFlags, pEncodePara, pbEncoded, pcbEncoded);
            break;
        case CERT_RDN_TELETEX_STRING:
            ret = CRYPT_AsnEncodeStringCoerce(value, ASN_T61STRING,
             dwFlags, pEncodePara, pbEncoded, pcbEncoded);
            break;
        case CERT_RDN_VIDEOTEX_STRING:
            ret = CRYPT_AsnEncodeStringCoerce(value,
             ASN_VIDEOTEXSTRING, dwFlags, pEncodePara, pbEncoded, pcbEncoded);
            break;
        case CERT_RDN_IA5_STRING:
            ret = CRYPT_AsnEncodeStringCoerce(value, ASN_IA5STRING,
             dwFlags, pEncodePara, pbEncoded, pcbEncoded);
            break;
        case CERT_RDN_GRAPHIC_STRING:
            ret = CRYPT_AsnEncodeStringCoerce(value, ASN_GRAPHICSTRING,
             dwFlags, pEncodePara, pbEncoded, pcbEncoded);
            break;
        case CERT_RDN_VISIBLE_STRING:
            ret = CRYPT_AsnEncodeStringCoerce(value, ASN_VISIBLESTRING,
             dwFlags, pEncodePara, pbEncoded, pcbEncoded);
            break;
        case CERT_RDN_GENERAL_STRING:
            ret = CRYPT_AsnEncodeStringCoerce(value, ASN_GENERALSTRING,
             dwFlags, pEncodePara, pbEncoded, pcbEncoded);
            break;
        case CERT_RDN_UNIVERSAL_STRING:
            FIXME("CERT_RDN_UNIVERSAL_STRING: unimplemented\n");
            SetLastError(CRYPT_E_ASN1_CHOICE);
            ret = FALSE;
            break;
        case CERT_RDN_BMP_STRING:
            ret = CRYPT_AsnEncodeBMPString(value, dwFlags, pEncodePara,
             pbEncoded, pcbEncoded);
            break;
        case CERT_RDN_UTF8_STRING:
            ret = CRYPT_AsnEncodeUTF8String(value, dwFlags, pEncodePara,
             pbEncoded, pcbEncoded);
            break;
        default:
            SetLastError(CRYPT_E_ASN1_CHOICE);
            ret = FALSE;
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL CRYPT_AsnEncodeRdnAttr(DWORD dwCertEncodingType,
 const CERT_RDN_ATTR *attr, CryptEncodeObjectExFunc nameValueEncodeFunc,
 BYTE *pbEncoded, DWORD *pcbEncoded)
{
    DWORD bytesNeeded = 0, lenBytes, size;
    BOOL ret;

    ret = CRYPT_AsnEncodeOid(dwCertEncodingType, NULL, attr->pszObjId,
     0, NULL, NULL, &size);
    if (ret)
    {
        bytesNeeded += size;
        /* hack: a CERT_RDN_ATTR is identical to a CERT_NAME_VALUE beginning
         * with dwValueType, so "cast" it to get its encoded size
         */
        ret = nameValueEncodeFunc(dwCertEncodingType, NULL, &attr->dwValueType,
         0, NULL, NULL, &size);
        if (ret)
        {
            bytesNeeded += size;
            CRYPT_EncodeLen(bytesNeeded, NULL, &lenBytes);
            bytesNeeded += 1 + lenBytes;
            if (pbEncoded)
            {
                if (*pcbEncoded < bytesNeeded)
                {
                    SetLastError(ERROR_MORE_DATA);
                    ret = FALSE;
                }
                else
                {
                    *pbEncoded++ = ASN_SEQUENCE;
                    CRYPT_EncodeLen(bytesNeeded - lenBytes - 1, pbEncoded,
                     &lenBytes);
                    pbEncoded += lenBytes;
                    size = bytesNeeded - 1 - lenBytes;
                    ret = CRYPT_AsnEncodeOid(dwCertEncodingType, NULL,
                     attr->pszObjId, 0, NULL, pbEncoded, &size);
                    if (ret)
                    {
                        pbEncoded += size;
                        size = bytesNeeded - 1 - lenBytes - size;
                        ret = nameValueEncodeFunc(dwCertEncodingType, NULL,
                         &attr->dwValueType, 0, NULL, pbEncoded, &size);
                        if (!ret)
                            *pcbEncoded = size;
                    }
                }
            }
            if (ret)
                *pcbEncoded = bytesNeeded;
        }
        else
        {
            /* Have to propagate index of failing character */
            *pcbEncoded = size;
        }
    }
    return ret;
}

static int __cdecl BLOBComp(const void *l, const void *r)
{
    const CRYPT_DER_BLOB *a = l, *b = r;
    int ret;

    if (!(ret = memcmp(a->pbData, b->pbData, min(a->cbData, b->cbData))))
        ret = a->cbData - b->cbData;
    return ret;
}

/* This encodes a SET OF, which in DER must be lexicographically sorted.
 */
static BOOL WINAPI CRYPT_DEREncodeSet(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    const CRYPT_BLOB_ARRAY *set = pvStructInfo;
    DWORD bytesNeeded = 0, lenBytes, i;
    BOOL ret;

    for (i = 0; i < set->cBlob; i++)
        bytesNeeded += set->rgBlob[i].cbData;
    CRYPT_EncodeLen(bytesNeeded, NULL, &lenBytes);
    bytesNeeded += 1 + lenBytes;
    if (!pbEncoded)
    {
        *pcbEncoded = bytesNeeded;
        ret = TRUE;
    }
    else if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
     pbEncoded, pcbEncoded, bytesNeeded)))
    {
        if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
            pbEncoded = *(BYTE **)pbEncoded;
        qsort(set->rgBlob, set->cBlob, sizeof(CRYPT_DER_BLOB), BLOBComp);
        *pbEncoded++ = ASN_CONSTRUCTOR | ASN_SETOF;
        CRYPT_EncodeLen(bytesNeeded - lenBytes - 1, pbEncoded, &lenBytes);
        pbEncoded += lenBytes;
        for (i = 0; i < set->cBlob; i++)
        {
            memcpy(pbEncoded, set->rgBlob[i].pbData, set->rgBlob[i].cbData);
            pbEncoded += set->rgBlob[i].cbData;
        }
    }
    return ret;
}

struct DERSetDescriptor
{
    DWORD                   cItems;
    const void             *items;
    size_t                  itemSize;
    size_t                  itemOffset;
    CryptEncodeObjectExFunc encode;
};

static BOOL WINAPI CRYPT_DEREncodeItemsAsSet(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    const struct DERSetDescriptor *desc = pvStructInfo;
    CRYPT_BLOB_ARRAY setOf = { 0, NULL };
    BOOL ret = TRUE;
    DWORD i;

    if (desc->cItems)
    {
        setOf.rgBlob = CryptMemAlloc(desc->cItems * sizeof(CRYPT_DER_BLOB));
        if (!setOf.rgBlob)
            ret = FALSE;
        else
        {
            setOf.cBlob = desc->cItems;
            memset(setOf.rgBlob, 0, setOf.cBlob * sizeof(CRYPT_DER_BLOB));
        }
    }
    for (i = 0; ret && i < setOf.cBlob; i++)
    {
        ret = desc->encode(dwCertEncodingType, lpszStructType,
         (const BYTE *)desc->items + i * desc->itemSize + desc->itemOffset,
         0, NULL, NULL, &setOf.rgBlob[i].cbData);
        if (ret)
        {
            setOf.rgBlob[i].pbData = CryptMemAlloc(setOf.rgBlob[i].cbData);
            if (!setOf.rgBlob[i].pbData)
                ret = FALSE;
            else
                ret = desc->encode(dwCertEncodingType, lpszStructType,
                 (const BYTE *)desc->items + i * desc->itemSize +
                 desc->itemOffset, 0, NULL, setOf.rgBlob[i].pbData,
                 &setOf.rgBlob[i].cbData);
        }
        /* Some functions propagate their errors through the size */
        if (!ret)
            *pcbEncoded = setOf.rgBlob[i].cbData;
    }
    if (ret)
    {
        DWORD bytesNeeded = 0, lenBytes;

        for (i = 0; i < setOf.cBlob; i++)
            bytesNeeded += setOf.rgBlob[i].cbData;
        CRYPT_EncodeLen(bytesNeeded, NULL, &lenBytes);
        bytesNeeded += 1 + lenBytes;
        if (!pbEncoded)
            *pcbEncoded = bytesNeeded;
        else if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
         pbEncoded, pcbEncoded, bytesNeeded)))
        {
            if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                pbEncoded = *(BYTE **)pbEncoded;
            qsort(setOf.rgBlob, setOf.cBlob, sizeof(CRYPT_DER_BLOB),
             BLOBComp);
            *pbEncoded++ = ASN_CONSTRUCTOR | ASN_SETOF;
            CRYPT_EncodeLen(bytesNeeded - lenBytes - 1, pbEncoded, &lenBytes);
            pbEncoded += lenBytes;
            for (i = 0; i < setOf.cBlob; i++)
            {
                memcpy(pbEncoded, setOf.rgBlob[i].pbData,
                 setOf.rgBlob[i].cbData);
                pbEncoded += setOf.rgBlob[i].cbData;
            }
        }
    }
    for (i = 0; i < setOf.cBlob; i++)
        CryptMemFree(setOf.rgBlob[i].pbData);
    CryptMemFree(setOf.rgBlob);
    return ret;
}

static BOOL CRYPT_AsnEncodeRdn(DWORD dwCertEncodingType, const CERT_RDN *rdn,
 CryptEncodeObjectExFunc nameValueEncodeFunc, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    BOOL ret;
    CRYPT_BLOB_ARRAY setOf = { 0, NULL };

    __TRY
    {
        DWORD i;

        ret = TRUE;
        if (rdn->cRDNAttr)
        {
            setOf.rgBlob = CryptMemAlloc(rdn->cRDNAttr *
             sizeof(CRYPT_DER_BLOB));
            if (!setOf.rgBlob)
                ret = FALSE;
            else
            {
                setOf.cBlob = rdn->cRDNAttr;
                memset(setOf.rgBlob, 0, setOf.cBlob * sizeof(CRYPT_DER_BLOB));
            }
        }
        for (i = 0; ret && i < rdn->cRDNAttr; i++)
        {
            setOf.rgBlob[i].cbData = 0;
            ret = CRYPT_AsnEncodeRdnAttr(dwCertEncodingType, &rdn->rgRDNAttr[i],
             nameValueEncodeFunc, NULL, &setOf.rgBlob[i].cbData);
            if (ret)
            {
                setOf.rgBlob[i].pbData = CryptMemAlloc(setOf.rgBlob[i].cbData);
                if (!setOf.rgBlob[i].pbData)
                    ret = FALSE;
                else
                    ret = CRYPT_AsnEncodeRdnAttr(dwCertEncodingType,
                     &rdn->rgRDNAttr[i], nameValueEncodeFunc,
                     setOf.rgBlob[i].pbData, &setOf.rgBlob[i].cbData);
            }
            if (!ret)
            {
                /* Have to propagate index of failing character */
                *pcbEncoded = setOf.rgBlob[i].cbData;
            }
        }
        if (ret)
            ret = CRYPT_DEREncodeSet(X509_ASN_ENCODING, NULL, &setOf, 0, NULL,
             pbEncoded, pcbEncoded);
        for (i = 0; i < setOf.cBlob; i++)
            CryptMemFree(setOf.rgBlob[i].pbData);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    CryptMemFree(setOf.rgBlob);
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeUnicodeNameValue(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);

static BOOL WINAPI CRYPT_AsnEncodeOrCopyUnicodeNameValue(
 DWORD dwCertEncodingType, LPCSTR lpszStructType, const void *pvStructInfo,
 DWORD dwFlags, PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    const CERT_NAME_VALUE *value = pvStructInfo;
    BOOL ret;

    if (value->dwValueType == CERT_RDN_ENCODED_BLOB)
        ret = CRYPT_CopyEncodedBlob(dwCertEncodingType, NULL, &value->Value,
         dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    else
        ret = CRYPT_AsnEncodeUnicodeNameValue(dwCertEncodingType, NULL, value,
         dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeUnicodeName(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret = TRUE;

    __TRY
    {
        const CERT_NAME_INFO *info = pvStructInfo;
        DWORD bytesNeeded = 0, lenBytes, size, i;

        TRACE("encoding name with %ld RDNs\n", info->cRDN);
        ret = TRUE;
        for (i = 0; ret && i < info->cRDN; i++)
        {
            ret = CRYPT_AsnEncodeRdn(dwCertEncodingType, &info->rgRDN[i],
             CRYPT_AsnEncodeOrCopyUnicodeNameValue, NULL, &size);
            if (ret)
                bytesNeeded += size;
            else
                *pcbEncoded = size;
        }
        CRYPT_EncodeLen(bytesNeeded, NULL, &lenBytes);
        bytesNeeded += 1 + lenBytes;
        if (ret)
        {
            if (!pbEncoded)
                *pcbEncoded = bytesNeeded;
            else
            {
                if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
                 pbEncoded, pcbEncoded, bytesNeeded)))
                {
                    BYTE *out;

                    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                        pbEncoded = *(BYTE **)pbEncoded;
                    out = pbEncoded;
                    *out++ = ASN_SEQUENCEOF;
                    CRYPT_EncodeLen(bytesNeeded - lenBytes - 1, out, &lenBytes);
                    out += lenBytes;
                    for (i = 0; ret && i < info->cRDN; i++)
                    {
                        size = bytesNeeded;
                        ret = CRYPT_AsnEncodeRdn(dwCertEncodingType,
                         &info->rgRDN[i], CRYPT_AsnEncodeOrCopyUnicodeNameValue,
                         out, &size);
                        if (ret)
                        {
                            out += size;
                            bytesNeeded -= size;
                        }
                        else
                            *pcbEncoded = size;
                    }
                    if (!ret && (dwFlags & CRYPT_ENCODE_ALLOC_FLAG))
                        CRYPT_FreeSpace(pEncodePara, pbEncoded);
                }
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeCTLVersion(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    const DWORD *ver = pvStructInfo;
    BOOL ret;

    /* CTL_V1 is not encoded */
    if (*ver == CTL_V1)
    {
        *pcbEncoded = 0;
        ret = TRUE;
    }
    else
        ret = CRYPT_AsnEncodeInt(dwCertEncodingType, X509_INTEGER, ver,
         dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    return ret;
}

/* Like CRYPT_AsnEncodeAlgorithmId, but encodes parameters as an asn.1 NULL
 * if they are empty and the OID is not empty (otherwise omits them.)
 */
static BOOL WINAPI CRYPT_AsnEncodeCTLSubjectAlgorithm(
 DWORD dwCertEncodingType, LPCSTR lpszStructType, const void *pvStructInfo,
 DWORD dwFlags, PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    const CRYPT_ALGORITHM_IDENTIFIER *algo = pvStructInfo;
    BOOL ret;
    struct AsnEncodeSequenceItem items[2] = {
     { algo->pszObjId, CRYPT_AsnEncodeOid, 0 },
    };
    DWORD cItem = 1;

    if (algo->pszObjId)
    {
        static const BYTE asn1Null[] = { ASN_NULL, 0 };
        static const CRYPT_DATA_BLOB nullBlob = { sizeof(asn1Null),
         (LPBYTE)asn1Null };

        if (algo->Parameters.cbData)
            items[cItem].pvStructInfo = &algo->Parameters;
        else
            items[cItem].pvStructInfo = &nullBlob;
        items[cItem].encodeFunc = CRYPT_CopyEncodedBlob;
        cItem++;
    }
    ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, cItem,
     dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    return ret;
}

static BOOL CRYPT_AsnEncodeCTLEntry(const CTL_ENTRY *entry,
 BYTE *pbEncoded, DWORD *pcbEncoded)
{
    struct AsnEncodeSequenceItem items[2] = {
     { &entry->SubjectIdentifier, CRYPT_AsnEncodeOctets, 0 },
     { &entry->cAttribute,        CRYPT_AsnEncodePKCSAttributes, 0 },
    };
    BOOL ret;

    ret = CRYPT_AsnEncodeSequence(X509_ASN_ENCODING, items,
     ARRAY_SIZE(items), 0, NULL, pbEncoded, pcbEncoded);
    return ret;
}

struct CTLEntries
{
    DWORD      cEntry;
    CTL_ENTRY *rgEntry;
};

static BOOL WINAPI CRYPT_AsnEncodeCTLEntries(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;
    DWORD bytesNeeded, dataLen, lenBytes, i;
    const struct CTLEntries *entries = pvStructInfo;

    ret = TRUE;
    for (i = 0, dataLen = 0; ret && i < entries->cEntry; i++)
    {
        DWORD size;

        ret = CRYPT_AsnEncodeCTLEntry(&entries->rgEntry[i], NULL, &size);
        if (ret)
            dataLen += size;
    }
    if (ret)
    {
        CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
        bytesNeeded = 1 + lenBytes + dataLen;
        if (!pbEncoded)
            *pcbEncoded = bytesNeeded;
        else
        {
            if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
             pbEncoded, pcbEncoded, bytesNeeded)))
            {
                BYTE *out;

                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                out = pbEncoded;
                *out++ = ASN_SEQUENCEOF;
                CRYPT_EncodeLen(dataLen, out, &lenBytes);
                out += lenBytes;
                for (i = 0; ret && i < entries->cEntry; i++)
                {
                    DWORD size = dataLen;

                    ret = CRYPT_AsnEncodeCTLEntry(&entries->rgEntry[i],
                     out, &size);
                    out += size;
                    dataLen -= size;
                }
                if (!ret && (dwFlags & CRYPT_ENCODE_ALLOC_FLAG))
                    CRYPT_FreeSpace(pEncodePara, pbEncoded);
            }
        }
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeCTL(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret = FALSE;

    __TRY
    {
        const CTL_INFO *info = pvStructInfo;
        struct AsnEncodeSequenceItem items[9] = {
         { &info->dwVersion,        CRYPT_AsnEncodeCTLVersion, 0 },
         { &info->SubjectUsage,     CRYPT_AsnEncodeEnhancedKeyUsage, 0 },
        };
        struct AsnConstructedItem constructed = { 0 };
        DWORD cItem = 2;

        if (info->ListIdentifier.cbData)
        {
            items[cItem].pvStructInfo = &info->ListIdentifier;
            items[cItem].encodeFunc = CRYPT_AsnEncodeOctets;
            cItem++;
        }
        if (info->SequenceNumber.cbData)
        {
            items[cItem].pvStructInfo = &info->SequenceNumber;
            items[cItem].encodeFunc = CRYPT_AsnEncodeInteger;
            cItem++;
        }
        items[cItem].pvStructInfo = &info->ThisUpdate;
        items[cItem].encodeFunc = CRYPT_AsnEncodeChoiceOfTime;
        cItem++;
        if (info->NextUpdate.dwLowDateTime || info->NextUpdate.dwHighDateTime)
        {
            items[cItem].pvStructInfo = &info->NextUpdate;
            items[cItem].encodeFunc = CRYPT_AsnEncodeChoiceOfTime;
            cItem++;
        }
        items[cItem].pvStructInfo = &info->SubjectAlgorithm;
        items[cItem].encodeFunc = CRYPT_AsnEncodeCTLSubjectAlgorithm;
        cItem++;
        if (info->cCTLEntry)
        {
            items[cItem].pvStructInfo = &info->cCTLEntry;
            items[cItem].encodeFunc = CRYPT_AsnEncodeCTLEntries;
            cItem++;
        }
        if (info->cExtension)
        {
            constructed.tag = 0;
            constructed.pvStructInfo = &info->cExtension;
            constructed.encodeFunc = CRYPT_AsnEncodeExtensions;
            items[cItem].pvStructInfo = &constructed;
            items[cItem].encodeFunc = CRYPT_AsnEncodeConstructed;
            cItem++;
        }
        ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, cItem,
         dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    return ret;
}

static BOOL CRYPT_AsnEncodeSMIMECapability(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret = FALSE;

    __TRY
    {
        const CRYPT_SMIME_CAPABILITY *capability = pvStructInfo;

        if (!capability->pszObjId)
            SetLastError(E_INVALIDARG);
        else
        {
            struct AsnEncodeSequenceItem items[] = {
             { capability->pszObjId, CRYPT_AsnEncodeOid, 0 },
             { &capability->Parameters, CRYPT_CopyEncodedBlob, 0 },
            };

            ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items,
             ARRAY_SIZE(items), dwFlags, pEncodePara, pbEncoded,
             pcbEncoded);
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeSMIMECapabilities(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret = FALSE;

    __TRY
    {
        DWORD bytesNeeded, dataLen, lenBytes, i;
        const CRYPT_SMIME_CAPABILITIES *capabilities = pvStructInfo;

        ret = TRUE;
        for (i = 0, dataLen = 0; ret && i < capabilities->cCapability; i++)
        {
            DWORD size;

            ret = CRYPT_AsnEncodeSMIMECapability(dwCertEncodingType, NULL,
             &capabilities->rgCapability[i], 0, NULL, NULL, &size);
            if (ret)
                dataLen += size;
        }
        if (ret)
        {
            CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
            bytesNeeded = 1 + lenBytes + dataLen;
            if (!pbEncoded)
                *pcbEncoded = bytesNeeded;
            else
            {
                if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
                 pbEncoded, pcbEncoded, bytesNeeded)))
                {
                    BYTE *out;

                    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                        pbEncoded = *(BYTE **)pbEncoded;
                    out = pbEncoded;
                    *out++ = ASN_SEQUENCEOF;
                    CRYPT_EncodeLen(dataLen, out, &lenBytes);
                    out += lenBytes;
                    for (i = 0; i < capabilities->cCapability; i++)
                    {
                        DWORD size = dataLen;

                        ret = CRYPT_AsnEncodeSMIMECapability(dwCertEncodingType,
                         NULL, &capabilities->rgCapability[i], 0, NULL,
                         out, &size);
                        out += size;
                        dataLen -= size;
                    }
                    if (!ret && (dwFlags & CRYPT_ENCODE_ALLOC_FLAG))
                        CRYPT_FreeSpace(pEncodePara, pbEncoded);
                }
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeNoticeNumbers(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    const CERT_POLICY_QUALIFIER_NOTICE_REFERENCE *noticeRef = pvStructInfo;
    DWORD bytesNeeded, dataLen, lenBytes, i;
    BOOL ret = TRUE;

    for (i = 0, dataLen = 0; ret && i < noticeRef->cNoticeNumbers; i++)
    {
        DWORD size;

        ret = CRYPT_AsnEncodeInt(dwCertEncodingType, X509_INTEGER,
         &noticeRef->rgNoticeNumbers[i], 0, NULL, NULL, &size);
        if (ret)
            dataLen += size;
    }
    if (ret)
    {
        CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
        bytesNeeded = 1 + lenBytes + dataLen;
        if (!pbEncoded)
            *pcbEncoded = bytesNeeded;
        else
        {
            if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
             pcbEncoded, bytesNeeded)))
            {
                BYTE *out;

                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                out = pbEncoded;
                *out++ = ASN_SEQUENCE;
                CRYPT_EncodeLen(dataLen, out, &lenBytes);
                out += lenBytes;
                for (i = 0; i < noticeRef->cNoticeNumbers; i++)
                {
                    DWORD size = dataLen;

                    ret = CRYPT_AsnEncodeInt(dwCertEncodingType, X509_INTEGER,
                     &noticeRef->rgNoticeNumbers[i], 0, NULL, out, &size);
                    out += size;
                    dataLen -= size;
                }
                if (!ret && (dwFlags & CRYPT_ENCODE_ALLOC_FLAG))
                    CRYPT_FreeSpace(pEncodePara, pbEncoded);
            }
        }
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeNoticeReference(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    const CERT_POLICY_QUALIFIER_NOTICE_REFERENCE *noticeRef = pvStructInfo;
    BOOL ret;
    CERT_NAME_VALUE orgValue = { CERT_RDN_IA5_STRING,
     { 0, (LPBYTE)noticeRef->pszOrganization } };
    struct AsnEncodeSequenceItem items[] = {
     { &orgValue, CRYPT_AsnEncodeNameValue, 0 },
     { noticeRef, CRYPT_AsnEncodeNoticeNumbers, 0 },
    };

    ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items,
     ARRAY_SIZE(items), dwFlags, pEncodePara, pbEncoded,
     pcbEncoded);
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodePolicyQualifierUserNotice(
 DWORD dwCertEncodingType, LPCSTR lpszStructType, const void *pvStructInfo,
 DWORD dwFlags, PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    BOOL ret = FALSE;

    __TRY
    {
        const CERT_POLICY_QUALIFIER_USER_NOTICE *notice = pvStructInfo;
        struct AsnEncodeSequenceItem items[2];
        CERT_NAME_VALUE displayTextValue;
        DWORD cItem = 0;

        if (notice->pNoticeReference)
        {
            items[cItem].encodeFunc = CRYPT_AsnEncodeNoticeReference;
            items[cItem].pvStructInfo = notice->pNoticeReference;
            cItem++;
        }
        if (notice->pszDisplayText)
        {
            displayTextValue.dwValueType = CERT_RDN_BMP_STRING;
            displayTextValue.Value.cbData = 0;
            displayTextValue.Value.pbData = (LPBYTE)notice->pszDisplayText;
            items[cItem].encodeFunc = CRYPT_AsnEncodeNameValue;
            items[cItem].pvStructInfo = &displayTextValue;
            cItem++;
        }
        ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, cItem,
         dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodePKCSAttribute(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret = FALSE;

    __TRY
    {
        const CRYPT_ATTRIBUTE *attr = pvStructInfo;

        if (!attr->pszObjId)
            SetLastError(E_INVALIDARG);
        else
        {
            struct AsnEncodeSequenceItem items[2] = {
             { attr->pszObjId, CRYPT_AsnEncodeOid, 0 },
             { &attr->cValue, CRYPT_DEREncodeSet, 0 },
            };

            ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items,
             ARRAY_SIZE(items), dwFlags, pEncodePara, pbEncoded,
             pcbEncoded);
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodePKCSAttributes(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret = FALSE;

    __TRY
    {
        const CRYPT_ATTRIBUTES *attributes = pvStructInfo;
        struct DERSetDescriptor desc = { attributes->cAttr, attributes->rgAttr,
         sizeof(CRYPT_ATTRIBUTE), 0, CRYPT_AsnEncodePKCSAttribute };

        ret = CRYPT_DEREncodeItemsAsSet(X509_ASN_ENCODING, lpszStructType,
         &desc, dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    return ret;
}

/* Like CRYPT_AsnEncodePKCSContentInfo, but allows the OID to be NULL */
static BOOL WINAPI CRYPT_AsnEncodePKCSContentInfoInternal(
 DWORD dwCertEncodingType, LPCSTR lpszStructType, const void *pvStructInfo,
 DWORD dwFlags, PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    const CRYPT_CONTENT_INFO *info = pvStructInfo;
    struct AsnEncodeSequenceItem items[2] = {
     { info->pszObjId, CRYPT_AsnEncodeOid, 0 },
     { NULL, NULL, 0 },
    };
    struct AsnConstructedItem constructed = { 0 };
    DWORD cItem = 1;

    if (info->Content.cbData)
    {
        constructed.tag = 0;
        constructed.pvStructInfo = &info->Content;
        constructed.encodeFunc = CRYPT_CopyEncodedBlob;
        items[cItem].pvStructInfo = &constructed;
        items[cItem].encodeFunc = CRYPT_AsnEncodeConstructed;
        cItem++;
    }
    return CRYPT_AsnEncodeSequence(dwCertEncodingType, items,
     cItem, dwFlags, pEncodePara, pbEncoded, pcbEncoded);
}

BOOL CRYPT_AsnEncodePKCSDigestedData(const CRYPT_DIGESTED_DATA *digestedData,
 void *pvData, DWORD *pcbData)
{
    struct AsnEncodeSequenceItem items[] = {
     { &digestedData->version, CRYPT_AsnEncodeInt, 0 },
     { &digestedData->DigestAlgorithm, CRYPT_AsnEncodeAlgorithmIdWithNullParams,
       0 },
     { &digestedData->ContentInfo, CRYPT_AsnEncodePKCSContentInfoInternal, 0 },
     { &digestedData->hash, CRYPT_AsnEncodeOctets, 0 },
    };

    return CRYPT_AsnEncodeSequence(X509_ASN_ENCODING, items, ARRAY_SIZE(items), 0, NULL, pvData, pcbData);
}

static BOOL WINAPI CRYPT_AsnEncodePKCSContentInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret = FALSE;

    __TRY
    {
        const CRYPT_CONTENT_INFO *info = pvStructInfo;

        if (!info->pszObjId)
            SetLastError(E_INVALIDARG);
        else
            ret = CRYPT_AsnEncodePKCSContentInfoInternal(dwCertEncodingType,
             lpszStructType, pvStructInfo, dwFlags, pEncodePara, pbEncoded,
             pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    return ret;
}

static BOOL CRYPT_AsnEncodeUnicodeStringCoerce(const CERT_NAME_VALUE *value,
 BYTE tag, DWORD dwFlags, const CRYPT_ENCODE_PARA *pEncodePara, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    BOOL ret = TRUE;
    LPCWSTR str = (LPCWSTR)value->Value.pbData;
    DWORD bytesNeeded, lenBytes, encodedLen;

    if (value->Value.cbData)
        encodedLen = value->Value.cbData / sizeof(WCHAR);
    else if (str)
        encodedLen = lstrlenW(str);
    else
        encodedLen = 0;
    CRYPT_EncodeLen(encodedLen, NULL, &lenBytes);
    bytesNeeded = 1 + lenBytes + encodedLen;
    if (!pbEncoded)
        *pcbEncoded = bytesNeeded;
    else
    {
        if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
         pbEncoded, pcbEncoded, bytesNeeded)))
        {
            DWORD i;

            if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                pbEncoded = *(BYTE **)pbEncoded;
            *pbEncoded++ = tag;
            CRYPT_EncodeLen(encodedLen, pbEncoded, &lenBytes);
            pbEncoded += lenBytes;
            for (i = 0; i < encodedLen; i++)
                *pbEncoded++ = (BYTE)str[i];
        }
    }
    return ret;
}

static BOOL CRYPT_AsnEncodeNumericString(const CERT_NAME_VALUE *value,
 DWORD dwFlags, const CRYPT_ENCODE_PARA *pEncodePara, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    BOOL ret = TRUE;
    LPCWSTR str = (LPCWSTR)value->Value.pbData;
    DWORD bytesNeeded, lenBytes, encodedLen;

    if (value->Value.cbData)
        encodedLen = value->Value.cbData / sizeof(WCHAR);
    else if (str)
        encodedLen = lstrlenW(str);
    else
        encodedLen = 0;
    CRYPT_EncodeLen(encodedLen, NULL, &lenBytes);
    bytesNeeded = 1 + lenBytes + encodedLen;
    if (!pbEncoded)
        *pcbEncoded = bytesNeeded;
    else
    {
        if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
         pbEncoded, pcbEncoded, bytesNeeded)))
        {
            DWORD i;
            BYTE *ptr;

            if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                ptr = *(BYTE **)pbEncoded;
            else
                ptr = pbEncoded;
            *ptr++ = ASN_NUMERICSTRING;
            CRYPT_EncodeLen(encodedLen, ptr, &lenBytes);
            ptr += lenBytes;
            for (i = 0; ret && i < encodedLen; i++)
            {
                if ('0' <= str[i] && str[i] <= '9')
                    *ptr++ = (BYTE)str[i];
                else
                {
                    *pcbEncoded = i;
                    SetLastError(CRYPT_E_INVALID_NUMERIC_STRING);
                    ret = FALSE;
                }
            }
            if (!ret && (dwFlags & CRYPT_ENCODE_ALLOC_FLAG))
                CRYPT_FreeSpace(pEncodePara, *(BYTE **)pbEncoded);
        }
    }
    return ret;
}

static inline BOOL isprintableW(WCHAR wc)
{
    return wc && wcschr( L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 '()+,-./:=?", wc );
}

static BOOL CRYPT_AsnEncodePrintableString(const CERT_NAME_VALUE *value,
 DWORD dwFlags, const CRYPT_ENCODE_PARA *pEncodePara, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    BOOL ret = TRUE;
    LPCWSTR str = (LPCWSTR)value->Value.pbData;
    DWORD bytesNeeded, lenBytes, encodedLen;

    if (value->Value.cbData)
        encodedLen = value->Value.cbData / sizeof(WCHAR);
    else if (str)
        encodedLen = lstrlenW(str);
    else
        encodedLen = 0;
    CRYPT_EncodeLen(encodedLen, NULL, &lenBytes);
    bytesNeeded = 1 + lenBytes + encodedLen;
    if (!pbEncoded)
        *pcbEncoded = bytesNeeded;
    else
    {
        if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
         pbEncoded, pcbEncoded, bytesNeeded)))
        {
            DWORD i;
            BYTE *ptr;

            if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                ptr = *(BYTE **)pbEncoded;
            else
                ptr = pbEncoded;
            *ptr++ = ASN_PRINTABLESTRING;
            CRYPT_EncodeLen(encodedLen, ptr, &lenBytes);
            ptr += lenBytes;
            for (i = 0; ret && i < encodedLen; i++)
            {
                if (isprintableW(str[i]))
                    *ptr++ = (BYTE)str[i];
                else
                {
                    *pcbEncoded = i;
                    SetLastError(CRYPT_E_INVALID_PRINTABLE_STRING);
                    ret = FALSE;
                }
            }
            if (!ret && (dwFlags & CRYPT_ENCODE_ALLOC_FLAG))
                CRYPT_FreeSpace(pEncodePara, *(BYTE **)pbEncoded);
        }
    }
    return ret;
}

static BOOL CRYPT_AsnEncodeIA5String(const CERT_NAME_VALUE *value,
 DWORD dwFlags, const CRYPT_ENCODE_PARA *pEncodePara, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    BOOL ret = TRUE;
    LPCWSTR str = (LPCWSTR)value->Value.pbData;
    DWORD bytesNeeded, lenBytes, encodedLen;

    if (value->Value.cbData)
        encodedLen = value->Value.cbData / sizeof(WCHAR);
    else if (str)
        encodedLen = lstrlenW(str);
    else
        encodedLen = 0;
    CRYPT_EncodeLen(encodedLen, NULL, &lenBytes);
    bytesNeeded = 1 + lenBytes + encodedLen;
    if (!pbEncoded)
        *pcbEncoded = bytesNeeded;
    else
    {
        if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
         pbEncoded, pcbEncoded, bytesNeeded)))
        {
            DWORD i;
            BYTE *ptr;

            if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                ptr = *(BYTE **)pbEncoded;
            else
                ptr = pbEncoded;
            *ptr++ = ASN_IA5STRING;
            CRYPT_EncodeLen(encodedLen, ptr, &lenBytes);
            ptr += lenBytes;
            for (i = 0; ret && i < encodedLen; i++)
            {
                if (str[i] <= 0x7f)
                    *ptr++ = (BYTE)str[i];
                else
                {
                    *pcbEncoded = i;
                    SetLastError(CRYPT_E_INVALID_IA5_STRING);
                    ret = FALSE;
                }
            }
            if (!ret && (dwFlags & CRYPT_ENCODE_ALLOC_FLAG))
                CRYPT_FreeSpace(pEncodePara, *(BYTE **)pbEncoded);
        }
    }
    return ret;
}

static BOOL CRYPT_AsnEncodeUniversalString(const CERT_NAME_VALUE *value,
 DWORD dwFlags, const CRYPT_ENCODE_PARA *pEncodePara, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    BOOL ret = TRUE;
    LPCWSTR str = (LPCWSTR)value->Value.pbData;
    DWORD bytesNeeded, lenBytes, strLen;

    /* FIXME: doesn't handle composite characters */
    if (value->Value.cbData)
        strLen = value->Value.cbData / sizeof(WCHAR);
    else if (str)
        strLen = lstrlenW(str);
    else
        strLen = 0;
    CRYPT_EncodeLen(strLen * 4, NULL, &lenBytes);
    bytesNeeded = 1 + lenBytes + strLen * 4;
    if (!pbEncoded)
        *pcbEncoded = bytesNeeded;
    else
    {
        if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
         pbEncoded, pcbEncoded, bytesNeeded)))
        {
            DWORD i;

            if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                pbEncoded = *(BYTE **)pbEncoded;
            *pbEncoded++ = ASN_UNIVERSALSTRING;
            CRYPT_EncodeLen(strLen * 4, pbEncoded, &lenBytes);
            pbEncoded += lenBytes;
            for (i = 0; i < strLen; i++)
            {
                *pbEncoded++ = 0;
                *pbEncoded++ = 0;
                *pbEncoded++ = (BYTE)((str[i] & 0xff00) >> 8);
                *pbEncoded++ = (BYTE)(str[i] & 0x00ff);
            }
        }
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeUnicodeNameValue(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret = FALSE;

    __TRY
    {
        const CERT_NAME_VALUE *value = pvStructInfo;

        switch (value->dwValueType)
        {
        case CERT_RDN_ANY_TYPE:
        case CERT_RDN_ENCODED_BLOB:
        case CERT_RDN_OCTET_STRING:
            SetLastError(CRYPT_E_NOT_CHAR_STRING);
            break;
        case CERT_RDN_NUMERIC_STRING:
            ret = CRYPT_AsnEncodeNumericString(value, dwFlags, pEncodePara,
             pbEncoded, pcbEncoded);
            break;
        case CERT_RDN_PRINTABLE_STRING:
            ret = CRYPT_AsnEncodePrintableString(value, dwFlags, pEncodePara,
             pbEncoded, pcbEncoded);
            break;
        case CERT_RDN_TELETEX_STRING:
            ret = CRYPT_AsnEncodeUnicodeStringCoerce(value, ASN_T61STRING,
             dwFlags, pEncodePara, pbEncoded, pcbEncoded);
            break;
        case CERT_RDN_VIDEOTEX_STRING:
            ret = CRYPT_AsnEncodeUnicodeStringCoerce(value,
             ASN_VIDEOTEXSTRING, dwFlags, pEncodePara, pbEncoded, pcbEncoded);
            break;
        case CERT_RDN_IA5_STRING:
            ret = CRYPT_AsnEncodeIA5String(value, dwFlags, pEncodePara,
             pbEncoded, pcbEncoded);
            break;
        case CERT_RDN_GRAPHIC_STRING:
            ret = CRYPT_AsnEncodeUnicodeStringCoerce(value, ASN_GRAPHICSTRING,
             dwFlags, pEncodePara, pbEncoded, pcbEncoded);
            break;
        case CERT_RDN_VISIBLE_STRING:
            ret = CRYPT_AsnEncodeUnicodeStringCoerce(value, ASN_VISIBLESTRING,
             dwFlags, pEncodePara, pbEncoded, pcbEncoded);
            break;
        case CERT_RDN_GENERAL_STRING:
            ret = CRYPT_AsnEncodeUnicodeStringCoerce(value, ASN_GENERALSTRING,
             dwFlags, pEncodePara, pbEncoded, pcbEncoded);
            break;
        case CERT_RDN_UNIVERSAL_STRING:
            ret = CRYPT_AsnEncodeUniversalString(value, dwFlags, pEncodePara,
             pbEncoded, pcbEncoded);
            break;
        case CERT_RDN_BMP_STRING:
            ret = CRYPT_AsnEncodeBMPString(value, dwFlags, pEncodePara,
             pbEncoded, pcbEncoded);
            break;
        case CERT_RDN_UTF8_STRING:
            ret = CRYPT_AsnEncodeUTF8String(value, dwFlags, pEncodePara,
             pbEncoded, pcbEncoded);
            break;
        default:
            SetLastError(CRYPT_E_ASN1_CHOICE);
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeName(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CERT_NAME_INFO *info = pvStructInfo;
        DWORD bytesNeeded = 0, lenBytes, size, i;

        TRACE("encoding name with %ld RDNs\n", info->cRDN);
        ret = TRUE;
        for (i = 0; ret && i < info->cRDN; i++)
        {
            ret = CRYPT_AsnEncodeRdn(dwCertEncodingType, &info->rgRDN[i],
             CRYPT_AsnEncodeNameValue, NULL, &size);
            if (ret)
                bytesNeeded += size;
        }
        CRYPT_EncodeLen(bytesNeeded, NULL, &lenBytes);
        bytesNeeded += 1 + lenBytes;
        if (ret)
        {
            if (!pbEncoded)
                *pcbEncoded = bytesNeeded;
            else
            {
                if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
                 pbEncoded, pcbEncoded, bytesNeeded)))
                {
                    BYTE *out;

                    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                        pbEncoded = *(BYTE **)pbEncoded;
                    out = pbEncoded;
                    *out++ = ASN_SEQUENCEOF;
                    CRYPT_EncodeLen(bytesNeeded - lenBytes - 1, out, &lenBytes);
                    out += lenBytes;
                    for (i = 0; ret && i < info->cRDN; i++)
                    {
                        size = bytesNeeded;
                        ret = CRYPT_AsnEncodeRdn(dwCertEncodingType,
                         &info->rgRDN[i], CRYPT_AsnEncodeNameValue, out, &size);
                        if (ret)
                        {
                            out += size;
                            bytesNeeded -= size;
                        }
                    }
                    if (!ret && (dwFlags & CRYPT_ENCODE_ALLOC_FLAG))
                        CRYPT_FreeSpace(pEncodePara, pbEncoded);
                }
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeBool(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL val = *(const BOOL *)pvStructInfo, ret;

    TRACE("%d\n", val);

    if (!pbEncoded)
    {
        *pcbEncoded = 3;
        ret = TRUE;
    }
    else if (*pcbEncoded < 3)
    {
        *pcbEncoded = 3;
        SetLastError(ERROR_MORE_DATA);
        ret = FALSE;
    }
    else
    {
        *pcbEncoded = 3;
        *pbEncoded++ = ASN_BOOL;
        *pbEncoded++ = 1;
        *pbEncoded++ = val ? 0xff : 0;
        ret = TRUE;
    }
    TRACE("returning %d (%08lx)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeAltNameEntry(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    const CERT_ALT_NAME_ENTRY *entry = pvStructInfo;
    BOOL ret;
    DWORD dataLen;
    BYTE tag;

    ret = TRUE;
    switch (entry->dwAltNameChoice)
    {
    case CERT_ALT_NAME_RFC822_NAME:
    case CERT_ALT_NAME_DNS_NAME:
    case CERT_ALT_NAME_URL:
        tag = ASN_CONTEXT | (entry->dwAltNameChoice - 1);
        if (entry->pwszURL)
        {
            DWORD i;

            /* Not + 1: don't encode the NULL-terminator */
            dataLen = lstrlenW(entry->pwszURL);
            for (i = 0; ret && i < dataLen; i++)
            {
                if (entry->pwszURL[i] > 0x7f)
                {
                    SetLastError(CRYPT_E_INVALID_IA5_STRING);
                    ret = FALSE;
                    *pcbEncoded = i;
                }
            }
        }
        else
            dataLen = 0;
        break;
    case CERT_ALT_NAME_DIRECTORY_NAME:
        tag = ASN_CONTEXT | ASN_CONSTRUCTOR | (entry->dwAltNameChoice - 1);
        dataLen = entry->DirectoryName.cbData;
        break;
    case CERT_ALT_NAME_IP_ADDRESS:
        tag = ASN_CONTEXT | (entry->dwAltNameChoice - 1);
        dataLen = entry->IPAddress.cbData;
        break;
    case CERT_ALT_NAME_REGISTERED_ID:
    {
        struct AsnEncodeTagSwappedItem swapped =
         { ASN_CONTEXT | (entry->dwAltNameChoice - 1), entry->pszRegisteredID,
           CRYPT_AsnEncodeOid };

        return CRYPT_AsnEncodeSwapTag(0, NULL, &swapped, 0, NULL, pbEncoded,
         pcbEncoded);
    }
    case CERT_ALT_NAME_OTHER_NAME:
        FIXME("name type %ld unimplemented\n", entry->dwAltNameChoice);
        return FALSE;
    default:
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    if (ret)
    {
        DWORD bytesNeeded, lenBytes;

        CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
        bytesNeeded = 1 + dataLen + lenBytes;
        if (!pbEncoded)
            *pcbEncoded = bytesNeeded;
        else if (*pcbEncoded < bytesNeeded)
        {
            SetLastError(ERROR_MORE_DATA);
            *pcbEncoded = bytesNeeded;
            ret = FALSE;
        }
        else
        {
            *pbEncoded++ = tag;
            CRYPT_EncodeLen(dataLen, pbEncoded, &lenBytes);
            pbEncoded += lenBytes;
            switch (entry->dwAltNameChoice)
            {
            case CERT_ALT_NAME_RFC822_NAME:
            case CERT_ALT_NAME_DNS_NAME:
            case CERT_ALT_NAME_URL:
            {
                DWORD i;

                for (i = 0; i < dataLen; i++)
                    *pbEncoded++ = (BYTE)entry->pwszURL[i];
                break;
            }
            case CERT_ALT_NAME_DIRECTORY_NAME:
                memcpy(pbEncoded, entry->DirectoryName.pbData, dataLen);
                break;
            case CERT_ALT_NAME_IP_ADDRESS:
                memcpy(pbEncoded, entry->IPAddress.pbData, dataLen);
                break;
            }
            if (ret)
                *pcbEncoded = bytesNeeded;
        }
    }
    TRACE("returning %d (%08lx)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeAuthorityKeyId(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CERT_AUTHORITY_KEY_ID_INFO *info = pvStructInfo;
        struct AsnEncodeSequenceItem items[3] = { { 0 } };
        struct AsnEncodeTagSwappedItem swapped[3] = { { 0 } };
        struct AsnConstructedItem constructed = { 0 };
        DWORD cItem = 0, cSwapped = 0;

        if (info->KeyId.cbData)
        {
            swapped[cSwapped].tag = ASN_CONTEXT | 0;
            swapped[cSwapped].pvStructInfo = &info->KeyId;
            swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeOctets;
            items[cItem].pvStructInfo = &swapped[cSwapped];
            items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
            cSwapped++;
            cItem++;
        }
        if (info->CertIssuer.cbData)
        {
            constructed.tag = 1;
            constructed.pvStructInfo = &info->CertIssuer;
            constructed.encodeFunc = CRYPT_CopyEncodedBlob;
            items[cItem].pvStructInfo = &constructed;
            items[cItem].encodeFunc = CRYPT_AsnEncodeConstructed;
            cItem++;
        }
        if (info->CertSerialNumber.cbData)
        {
            swapped[cSwapped].tag = ASN_CONTEXT | 2;
            swapped[cSwapped].pvStructInfo = &info->CertSerialNumber;
            swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeInteger;
            items[cItem].pvStructInfo = &swapped[cSwapped];
            items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
            cSwapped++;
            cItem++;
        }
        ret = CRYPT_AsnEncodeSequence(X509_ASN_ENCODING, items, cItem, dwFlags,
         pEncodePara, pbEncoded, pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeAltName(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CERT_ALT_NAME_INFO *info = pvStructInfo;
        DWORD bytesNeeded, dataLen, lenBytes, i;

        ret = TRUE;
        /* FIXME: should check that cAltEntry is not bigger than 0xff, since we
         * can't encode an erroneous entry index if it's bigger than this.
         */
        for (i = 0, dataLen = 0; ret && i < info->cAltEntry; i++)
        {
            DWORD len;

            ret = CRYPT_AsnEncodeAltNameEntry(dwCertEncodingType, NULL,
             &info->rgAltEntry[i], 0, NULL, NULL, &len);
            if (ret)
                dataLen += len;
            else if (GetLastError() == CRYPT_E_INVALID_IA5_STRING)
            {
                /* CRYPT_AsnEncodeAltNameEntry encoded the index of
                 * the bad character, now set the index of the bad
                 * entry
                 */
                *pcbEncoded = (BYTE)i <<
                 CERT_ALT_NAME_ENTRY_ERR_INDEX_SHIFT | len;
            }
        }
        if (ret)
        {
            CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
            bytesNeeded = 1 + lenBytes + dataLen;
            if (!pbEncoded)
            {
                *pcbEncoded = bytesNeeded;
                ret = TRUE;
            }
            else
            {
                if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
                 pbEncoded, pcbEncoded, bytesNeeded)))
                {
                    BYTE *out;

                    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                        pbEncoded = *(BYTE **)pbEncoded;
                    out = pbEncoded;
                    *out++ = ASN_SEQUENCEOF;
                    CRYPT_EncodeLen(dataLen, out, &lenBytes);
                    out += lenBytes;
                    for (i = 0; ret && i < info->cAltEntry; i++)
                    {
                        DWORD len = dataLen;

                        ret = CRYPT_AsnEncodeAltNameEntry(dwCertEncodingType,
                         NULL, &info->rgAltEntry[i], 0, NULL, out, &len);
                        if (ret)
                        {
                            out += len;
                            dataLen -= len;
                        }
                    }
                    if (!ret && (dwFlags & CRYPT_ENCODE_ALLOC_FLAG))
                        CRYPT_FreeSpace(pEncodePara, pbEncoded);
                }
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeAuthorityKeyId2(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CERT_AUTHORITY_KEY_ID2_INFO *info = pvStructInfo;
        struct AsnEncodeSequenceItem items[3] = { { 0 } };
        struct AsnEncodeTagSwappedItem swapped[3] = { { 0 } };
        DWORD cItem = 0, cSwapped = 0;

        if (info->KeyId.cbData)
        {
            swapped[cSwapped].tag = ASN_CONTEXT | 0;
            swapped[cSwapped].pvStructInfo = &info->KeyId;
            swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeOctets;
            items[cItem].pvStructInfo = &swapped[cSwapped];
            items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
            cSwapped++;
            cItem++;
        }
        if (info->AuthorityCertIssuer.cAltEntry)
        {
            swapped[cSwapped].tag = ASN_CONTEXT | ASN_CONSTRUCTOR | 1;
            swapped[cSwapped].pvStructInfo = &info->AuthorityCertIssuer;
            swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeAltName;
            items[cItem].pvStructInfo = &swapped[cSwapped];
            items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
            cSwapped++;
            cItem++;
        }
        if (info->AuthorityCertSerialNumber.cbData)
        {
            swapped[cSwapped].tag = ASN_CONTEXT | 2;
            swapped[cSwapped].pvStructInfo = &info->AuthorityCertSerialNumber;
            swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeInteger;
            items[cItem].pvStructInfo = &swapped[cSwapped];
            items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
            cSwapped++;
            cItem++;
        }
        ret = CRYPT_AsnEncodeSequence(X509_ASN_ENCODING, items, cItem, dwFlags,
         pEncodePara, pbEncoded, pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL CRYPT_AsnEncodeAccessDescription(
 const CERT_ACCESS_DESCRIPTION *descr, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    struct AsnEncodeSequenceItem items[] = {
     { descr->pszAccessMethod, CRYPT_AsnEncodeOid, 0 },
     { &descr->AccessLocation, CRYPT_AsnEncodeAltNameEntry, 0 },
    };

    if (!descr->pszAccessMethod)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    return CRYPT_AsnEncodeSequence(X509_ASN_ENCODING, items, ARRAY_SIZE(items), 0, NULL, pbEncoded, pcbEncoded);
}

static BOOL WINAPI CRYPT_AsnEncodeAuthorityInfoAccess(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        DWORD bytesNeeded, dataLen, lenBytes, i;
        const CERT_AUTHORITY_INFO_ACCESS *info = pvStructInfo;

        ret = TRUE;
        for (i = 0, dataLen = 0; ret && i < info->cAccDescr; i++)
        {
            DWORD size;

            ret = CRYPT_AsnEncodeAccessDescription(&info->rgAccDescr[i], NULL,
             &size);
            if (ret)
                dataLen += size;
        }
        if (ret)
        {
            CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
            bytesNeeded = 1 + lenBytes + dataLen;
            if (!pbEncoded)
                *pcbEncoded = bytesNeeded;
            else
            {
                if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
                 pbEncoded, pcbEncoded, bytesNeeded)))
                {
                    BYTE *out;

                    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                        pbEncoded = *(BYTE **)pbEncoded;
                    out = pbEncoded;
                    *out++ = ASN_SEQUENCEOF;
                    CRYPT_EncodeLen(dataLen, out, &lenBytes);
                    out += lenBytes;
                    for (i = 0; i < info->cAccDescr; i++)
                    {
                        DWORD size = dataLen;

                        ret = CRYPT_AsnEncodeAccessDescription(
                         &info->rgAccDescr[i], out, &size);
                        out += size;
                        dataLen -= size;
                    }
                    if (!ret && (dwFlags & CRYPT_ENCODE_ALLOC_FLAG))
                        CRYPT_FreeSpace(pEncodePara, pbEncoded);
                }
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeBasicConstraints(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CERT_BASIC_CONSTRAINTS_INFO *info = pvStructInfo;
        struct AsnEncodeSequenceItem items[3] = {
         { &info->SubjectType, CRYPT_AsnEncodeBits, 0 },
         { 0 }
        };
        DWORD cItem = 1;

        if (info->fPathLenConstraint)
        {
            items[cItem].pvStructInfo = &info->dwPathLenConstraint;
            items[cItem].encodeFunc = CRYPT_AsnEncodeInt;
            cItem++;
        }
        if (info->cSubtreesConstraint)
        {
            items[cItem].pvStructInfo = &info->cSubtreesConstraint;
            items[cItem].encodeFunc = CRYPT_AsnEncodeSequenceOfAny;
            cItem++;
        }
        ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, cItem,
         dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeBasicConstraints2(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CERT_BASIC_CONSTRAINTS2_INFO *info = pvStructInfo;
        struct AsnEncodeSequenceItem items[2] = { { 0 } };
        DWORD cItem = 0;

        if (info->fCA)
        {
            items[cItem].pvStructInfo = &info->fCA;
            items[cItem].encodeFunc = CRYPT_AsnEncodeBool;
            cItem++;
        }
        if (info->fPathLenConstraint)
        {
            items[cItem].pvStructInfo = &info->dwPathLenConstraint;
            items[cItem].encodeFunc = CRYPT_AsnEncodeInt;
            cItem++;
        }
        ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, cItem,
         dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeCertPolicyQualifiers(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    const CERT_POLICY_INFO *info = pvStructInfo;
    BOOL ret;

    if (!info->cPolicyQualifier)
    {
        *pcbEncoded = 0;
        ret = TRUE;
    }
    else
    {
        struct AsnEncodeSequenceItem items[2] = {
         { NULL, CRYPT_AsnEncodeOid, 0 },
         { NULL, CRYPT_CopyEncodedBlob, 0 },
        };
        DWORD bytesNeeded = 0, lenBytes, size, i;

        ret = TRUE;
        for (i = 0; ret && i < info->cPolicyQualifier; i++)
        {
            items[0].pvStructInfo =
             info->rgPolicyQualifier[i].pszPolicyQualifierId;
            items[1].pvStructInfo = &info->rgPolicyQualifier[i].Qualifier;
            ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, ARRAY_SIZE(items),
             dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG, NULL, NULL, &size);
            if (ret)
                bytesNeeded += size;
        }
        CRYPT_EncodeLen(bytesNeeded, NULL, &lenBytes);
        bytesNeeded += 1 + lenBytes;
        if (ret)
        {
            if (!pbEncoded)
                *pcbEncoded = bytesNeeded;
            else
            {
                if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
                 pbEncoded, pcbEncoded, bytesNeeded)))
                {
                    BYTE *out;

                    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                        pbEncoded = *(BYTE **)pbEncoded;
                    out = pbEncoded;
                    *out++ = ASN_SEQUENCEOF;
                    CRYPT_EncodeLen(bytesNeeded - lenBytes - 1, out, &lenBytes);
                    out += lenBytes;
                    for (i = 0; ret && i < info->cPolicyQualifier; i++)
                    {
                        items[0].pvStructInfo =
                         info->rgPolicyQualifier[i].pszPolicyQualifierId;
                        items[1].pvStructInfo =
                         &info->rgPolicyQualifier[i].Qualifier;
                        size = bytesNeeded;
                        ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, ARRAY_SIZE(items),
                         dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG, NULL, out, &size);
                        if (ret)
                        {
                            out += size;
                            bytesNeeded -= size;
                        }
                    }
                    if (!ret && (dwFlags & CRYPT_ENCODE_ALLOC_FLAG))
                        CRYPT_FreeSpace(pEncodePara, pbEncoded);
                }
            }
        }
    }
    return ret;
}

static BOOL CRYPT_AsnEncodeCertPolicy(DWORD dwCertEncodingType,
 const CERT_POLICY_INFO *info, DWORD dwFlags, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    struct AsnEncodeSequenceItem items[2] = {
     { info->pszPolicyIdentifier, CRYPT_AsnEncodeOid, 0 },
     { info,                      CRYPT_AsnEncodeCertPolicyQualifiers, 0 },
    };
    BOOL ret;

    if (!info->pszPolicyIdentifier)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, ARRAY_SIZE(items), dwFlags, NULL, pbEncoded, pcbEncoded);
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeCertPolicies(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret = FALSE;

    __TRY
    {
        const CERT_POLICIES_INFO *info = pvStructInfo;
        DWORD bytesNeeded = 0, lenBytes, size, i;

        ret = TRUE;
        for (i = 0; ret && i < info->cPolicyInfo; i++)
        {
            ret = CRYPT_AsnEncodeCertPolicy(dwCertEncodingType,
             &info->rgPolicyInfo[i], dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG, NULL,
             &size);
            if (ret)
                bytesNeeded += size;
        }
        CRYPT_EncodeLen(bytesNeeded, NULL, &lenBytes);
        bytesNeeded += 1 + lenBytes;
        if (ret)
        {
            if (!pbEncoded)
                *pcbEncoded = bytesNeeded;
            else
            {
                if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
                 pbEncoded, pcbEncoded, bytesNeeded)))
                {
                    BYTE *out;

                    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                        pbEncoded = *(BYTE **)pbEncoded;
                    out = pbEncoded;
                    *out++ = ASN_SEQUENCEOF;
                    CRYPT_EncodeLen(bytesNeeded - lenBytes - 1, out, &lenBytes);
                    out += lenBytes;
                    for (i = 0; ret && i < info->cPolicyInfo; i++)
                    {
                        size = bytesNeeded;
                        ret = CRYPT_AsnEncodeCertPolicy(dwCertEncodingType,
                         &info->rgPolicyInfo[i],
                         dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG, out, &size);
                        if (ret)
                        {
                            out += size;
                            bytesNeeded -= size;
                        }
                    }
                    if (!ret && (dwFlags & CRYPT_ENCODE_ALLOC_FLAG))
                        CRYPT_FreeSpace(pEncodePara, pbEncoded);
                }
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    return ret;
}

static BOOL CRYPT_AsnEncodeCertPolicyMapping(DWORD dwCertEncodingType,
 const CERT_POLICY_MAPPING *mapping, DWORD dwFlags, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    struct AsnEncodeSequenceItem items[] = {
     { mapping->pszIssuerDomainPolicy,  CRYPT_AsnEncodeOid, 0 },
     { mapping->pszSubjectDomainPolicy, CRYPT_AsnEncodeOid, 0 },
    };

    if (!mapping->pszIssuerDomainPolicy || !mapping->pszSubjectDomainPolicy)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    return CRYPT_AsnEncodeSequence(dwCertEncodingType, items, ARRAY_SIZE(items), dwFlags, NULL, pbEncoded, pcbEncoded);
}

static BOOL WINAPI CRYPT_AsnEncodeCertPolicyMappings(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret = FALSE;

    __TRY
    {
        const CERT_POLICY_MAPPINGS_INFO *info = pvStructInfo;
        DWORD bytesNeeded = 0, lenBytes, size, i;

        ret = TRUE;
        for (i = 0; ret && i < info->cPolicyMapping; i++)
        {
            ret = CRYPT_AsnEncodeCertPolicyMapping(dwCertEncodingType,
             &info->rgPolicyMapping[i], dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG,
             NULL, &size);
            if (ret)
                bytesNeeded += size;
        }
        CRYPT_EncodeLen(bytesNeeded, NULL, &lenBytes);
        bytesNeeded += 1 + lenBytes;
        if (ret)
        {
            if (!pbEncoded)
                *pcbEncoded = bytesNeeded;
            else
            {
                if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
                 pbEncoded, pcbEncoded, bytesNeeded)))
                {
                    BYTE *out;

                    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                        pbEncoded = *(BYTE **)pbEncoded;
                    out = pbEncoded;
                    *out++ = ASN_SEQUENCEOF;
                    CRYPT_EncodeLen(bytesNeeded - lenBytes - 1, out, &lenBytes);
                    out += lenBytes;
                    for (i = 0; ret && i < info->cPolicyMapping; i++)
                    {
                        size = bytesNeeded;
                        ret = CRYPT_AsnEncodeCertPolicyMapping(
                         dwCertEncodingType, &info->rgPolicyMapping[i],
                         dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG, out, &size);
                        if (ret)
                        {
                            out += size;
                            bytesNeeded -= size;
                        }
                    }
                    if (!ret && (dwFlags & CRYPT_ENCODE_ALLOC_FLAG))
                        CRYPT_FreeSpace(pEncodePara, pbEncoded);
                }
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeCertPolicyConstraints(
 DWORD dwCertEncodingType, LPCSTR lpszStructType, const void *pvStructInfo,
 DWORD dwFlags, PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    BOOL ret = FALSE;

    __TRY
    {
        const CERT_POLICY_CONSTRAINTS_INFO *info = pvStructInfo;
        struct AsnEncodeSequenceItem items[2];
        struct AsnEncodeTagSwappedItem swapped[2];
        DWORD cItem = 0, cSwapped = 0;

        if (info->fRequireExplicitPolicy)
        {
            swapped[cSwapped].tag = ASN_CONTEXT | 0;
            swapped[cSwapped].pvStructInfo =
             &info->dwRequireExplicitPolicySkipCerts;
            swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeInt;
            items[cItem].pvStructInfo = &swapped[cSwapped];
            items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
            cSwapped++;
            cItem++;
        }
        if (info->fInhibitPolicyMapping)
        {
            swapped[cSwapped].tag = ASN_CONTEXT | 1;
            swapped[cSwapped].pvStructInfo =
             &info->dwInhibitPolicyMappingSkipCerts;
            swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeInt;
            items[cItem].pvStructInfo = &swapped[cSwapped];
            items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
            cSwapped++;
            cItem++;
        }
        ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, cItem,
         dwFlags, NULL, pbEncoded, pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeRsaPubKey_Bcrypt(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const BCRYPT_RSAKEY_BLOB *hdr = pvStructInfo;

        BYTE *pubexp = (BYTE*)pvStructInfo + sizeof(BCRYPT_RSAKEY_BLOB);
        BYTE *modulus = (BYTE*)pvStructInfo + sizeof(BCRYPT_RSAKEY_BLOB) + hdr->cbPublicExp;
        BYTE *pubexp_be = CryptMemAlloc(hdr->cbPublicExp);
        BYTE *modulus_be = CryptMemAlloc(hdr->cbModulus);
        CRYPT_INTEGER_BLOB pubexp_int = { hdr->cbPublicExp, pubexp_be };
        CRYPT_INTEGER_BLOB modulus_int = { hdr->cbModulus, modulus_be};

        struct AsnEncodeSequenceItem items[] = {
         { &modulus_int, CRYPT_AsnEncodeUnsignedInteger, 0 },
         { &pubexp_int, CRYPT_AsnEncodeInteger, 0 },
        };

        /* CNG_RSA_PUBLIC_KEY_BLOB stores the exponent and modulus
         * in big-endian format, so we need to convert them
         * to little-endian format before encoding
         */
        CRYPT_CopyReversed(pubexp_be, pubexp, hdr->cbPublicExp);
        CRYPT_CopyReversed(modulus_be, modulus, hdr->cbModulus);

        ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items,
         ARRAY_SIZE(items), dwFlags, pEncodePara, pbEncoded, pcbEncoded);

        CryptMemFree(pubexp_be);
        CryptMemFree(modulus_be);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;

}


static BOOL WINAPI CRYPT_AsnEncodeRsaPubKey(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const BLOBHEADER *hdr = pvStructInfo;

        if (hdr->bType != PUBLICKEYBLOB)
        {
            SetLastError(E_INVALIDARG);
            ret = FALSE;
        }
        else
        {
            const RSAPUBKEY *rsaPubKey = (const RSAPUBKEY *)
             ((const BYTE *)pvStructInfo + sizeof(BLOBHEADER));
            CRYPT_INTEGER_BLOB blob = { rsaPubKey->bitlen / 8,
             (BYTE *)pvStructInfo + sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) };
            struct AsnEncodeSequenceItem items[] = { 
             { &blob, CRYPT_AsnEncodeUnsignedInteger, 0 },
             { &rsaPubKey->pubexp, CRYPT_AsnEncodeInt, 0 },
            };

            ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items,
             ARRAY_SIZE(items), dwFlags, pEncodePara, pbEncoded, pcbEncoded);
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

BOOL WINAPI CRYPT_AsnEncodeOctets(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CRYPT_DATA_BLOB *blob = pvStructInfo;
        DWORD bytesNeeded, lenBytes;

        TRACE("(%ld, %p), %08lx, %p, %p, %ld\n", blob->cbData, blob->pbData,
         dwFlags, pEncodePara, pbEncoded, pbEncoded ? *pcbEncoded : 0);

        CRYPT_EncodeLen(blob->cbData, NULL, &lenBytes);
        bytesNeeded = 1 + lenBytes + blob->cbData;
        if (!pbEncoded)
        {
            *pcbEncoded = bytesNeeded;
            ret = TRUE;
        }
        else
        {
            if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
             pcbEncoded, bytesNeeded)))
            {
                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                *pbEncoded++ = ASN_OCTETSTRING;
                CRYPT_EncodeLen(blob->cbData, pbEncoded, &lenBytes);
                pbEncoded += lenBytes;
                if (blob->cbData)
                    memcpy(pbEncoded, blob->pbData, blob->cbData);
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    TRACE("returning %d (%08lx)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeBits(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CRYPT_BIT_BLOB *blob = pvStructInfo;
        DWORD bytesNeeded, lenBytes, dataBytes;
        BYTE unusedBits;

        /* yep, MS allows cUnusedBits to be >= 8 */
        if (!blob->cUnusedBits)
        {
            dataBytes = blob->cbData;
            unusedBits = 0;
        }
        else if (blob->cbData * 8 > blob->cUnusedBits)
        {
            dataBytes = (blob->cbData * 8 - blob->cUnusedBits) / 8 + 1;
            unusedBits = blob->cUnusedBits >= 8 ? blob->cUnusedBits / 8 :
             blob->cUnusedBits;
        }
        else
        {
            dataBytes = 0;
            unusedBits = 0;
        }
        CRYPT_EncodeLen(dataBytes + 1, NULL, &lenBytes);
        bytesNeeded = 1 + lenBytes + dataBytes + 1;
        if (!pbEncoded)
        {
            *pcbEncoded = bytesNeeded;
            ret = TRUE;
        }
        else
        {
            if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
             pcbEncoded, bytesNeeded)))
            {
                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                *pbEncoded++ = ASN_BITSTRING;
                CRYPT_EncodeLen(dataBytes + 1, pbEncoded, &lenBytes);
                pbEncoded += lenBytes;
                *pbEncoded++ = unusedBits;
                if (dataBytes)
                {
                    BYTE mask = 0xff << unusedBits;

                    if (dataBytes > 1)
                    {
                        memcpy(pbEncoded, blob->pbData, dataBytes - 1);
                        pbEncoded += dataBytes - 1;
                    }
                    *pbEncoded = *(blob->pbData + dataBytes - 1) & mask;
                }
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeBitsSwapBytes(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CRYPT_BIT_BLOB *blob = pvStructInfo;
        CRYPT_BIT_BLOB newBlob = { blob->cbData, NULL, blob->cUnusedBits };

        ret = TRUE;
        if (newBlob.cbData)
        {
            newBlob.pbData = CryptMemAlloc(newBlob.cbData);
            if (newBlob.pbData)
            {
                DWORD i;

                for (i = 0; i < newBlob.cbData; i++)
                    newBlob.pbData[newBlob.cbData - i - 1] = blob->pbData[i];
            }
            else
                ret = FALSE;
        }
        if (ret)
            ret = CRYPT_AsnEncodeBits(dwCertEncodingType, lpszStructType,
             &newBlob, dwFlags, pEncodePara, pbEncoded, pcbEncoded);
        CryptMemFree(newBlob.pbData);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeInt(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    CRYPT_INTEGER_BLOB blob = { sizeof(INT), (BYTE *)pvStructInfo };

    return CRYPT_AsnEncodeInteger(dwCertEncodingType, X509_MULTI_BYTE_INTEGER,
     &blob, dwFlags, pEncodePara, pbEncoded, pcbEncoded);
}

static BOOL WINAPI CRYPT_AsnEncodeInteger(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        DWORD significantBytes, lenBytes, bytesNeeded;
        BYTE padByte = 0;
        BOOL pad = FALSE;
        const CRYPT_INTEGER_BLOB *blob = pvStructInfo;

        significantBytes = blob->cbData;
        if (significantBytes)
        {
            if (blob->pbData[significantBytes - 1] & 0x80)
            {
                /* negative, lop off leading (little-endian) 0xffs */
                for (; significantBytes > 0 &&
                 blob->pbData[significantBytes - 1] == 0xff; significantBytes--)
                    ;
                if (blob->pbData[significantBytes - 1] < 0x80)
                {
                    padByte = 0xff;
                    pad = TRUE;
                }
            }
            else
            {
                /* positive, lop off leading (little-endian) zeroes */
                for (; significantBytes > 0 &&
                 !blob->pbData[significantBytes - 1]; significantBytes--)
                    ;
                if (significantBytes == 0)
                    significantBytes = 1;
                if (blob->pbData[significantBytes - 1] > 0x7f)
                {
                    padByte = 0;
                    pad = TRUE;
                }
            }
        }
        if (pad)
            CRYPT_EncodeLen(significantBytes + 1, NULL, &lenBytes);
        else
            CRYPT_EncodeLen(significantBytes, NULL, &lenBytes);
        bytesNeeded = 1 + lenBytes + significantBytes;
        if (pad)
            bytesNeeded++;
        if (!pbEncoded)
        {
            *pcbEncoded = bytesNeeded;
            ret = TRUE;
        }
        else
        {
            if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
             pcbEncoded, bytesNeeded)))
            {
                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                *pbEncoded++ = ASN_INTEGER;
                if (pad)
                {
                    CRYPT_EncodeLen(significantBytes + 1, pbEncoded, &lenBytes);
                    pbEncoded += lenBytes;
                    *pbEncoded++ = padByte;
                }
                else
                {
                    CRYPT_EncodeLen(significantBytes, pbEncoded, &lenBytes);
                    pbEncoded += lenBytes;
                }
                for (; significantBytes > 0; significantBytes--)
                    *(pbEncoded++) = blob->pbData[significantBytes - 1];
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeUnsignedInteger(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        DWORD significantBytes, lenBytes, bytesNeeded;
        BOOL pad = FALSE;
        const CRYPT_INTEGER_BLOB *blob = pvStructInfo;

        significantBytes = blob->cbData;
        if (significantBytes)
        {
            /* positive, lop off leading (little-endian) zeroes */
            for (; significantBytes > 0 && !blob->pbData[significantBytes - 1];
             significantBytes--)
                ;
            if (significantBytes == 0)
                significantBytes = 1;
            if (blob->pbData[significantBytes - 1] > 0x7f)
                pad = TRUE;
        }
        if (pad)
            CRYPT_EncodeLen(significantBytes + 1, NULL, &lenBytes);
        else
            CRYPT_EncodeLen(significantBytes, NULL, &lenBytes);
        bytesNeeded = 1 + lenBytes + significantBytes;
        if (pad)
            bytesNeeded++;
        if (!pbEncoded)
        {
            *pcbEncoded = bytesNeeded;
            ret = TRUE;
        }
        else
        {
            if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
             pcbEncoded, bytesNeeded)))
            {
                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                *pbEncoded++ = ASN_INTEGER;
                if (pad)
                {
                    CRYPT_EncodeLen(significantBytes + 1, pbEncoded, &lenBytes);
                    pbEncoded += lenBytes;
                    *pbEncoded++ = 0;
                }
                else
                {
                    CRYPT_EncodeLen(significantBytes, pbEncoded, &lenBytes);
                    pbEncoded += lenBytes;
                }
                for (; significantBytes > 0; significantBytes--)
                    *(pbEncoded++) = blob->pbData[significantBytes - 1];
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeEnumerated(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    CRYPT_INTEGER_BLOB blob;
    BOOL ret;

    /* Encode as an unsigned integer, then change the tag to enumerated */
    blob.cbData = sizeof(DWORD);
    blob.pbData = (BYTE *)pvStructInfo;
    ret = CRYPT_AsnEncodeUnsignedInteger(dwCertEncodingType,
     X509_MULTI_BYTE_UINT, &blob, dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    if (ret && pbEncoded)
    {
        if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
            pbEncoded = *(BYTE **)pbEncoded;
        pbEncoded[0] = ASN_ENUMERATED;
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeUtcTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        SYSTEMTIME sysTime;
        /* sorry, magic number: enough for tag, len, YYMMDDHHMMSSZ.  I use a
         * temporary buffer because the output buffer is not NULL-terminated.
         */
        static const DWORD bytesNeeded = 15;
        char buf[40];

        if (!pbEncoded)
        {
            *pcbEncoded = bytesNeeded;
            ret = TRUE;
        }
        else
        {
            /* Sanity check the year, this is a two-digit year format */
            ret = FileTimeToSystemTime(pvStructInfo, &sysTime);
            if (ret && (sysTime.wYear < 1950 || sysTime.wYear > 2050))
            {
                SetLastError(CRYPT_E_BAD_ENCODE);
                ret = FALSE;
            }
            if (ret)
            {
                if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
                 pbEncoded, pcbEncoded, bytesNeeded)))
                {
                    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                        pbEncoded = *(BYTE **)pbEncoded;
                    buf[0] = ASN_UTCTIME;
                    buf[1] = bytesNeeded - 2;
                    snprintf(buf + 2, sizeof(buf) - 2,
                     "%02d%02d%02d%02d%02d%02dZ", sysTime.wYear >= 2000 ?
                     sysTime.wYear - 2000 : sysTime.wYear - 1900,
                     sysTime.wMonth, sysTime.wDay, sysTime.wHour,
                     sysTime.wMinute, sysTime.wSecond);
                    memcpy(pbEncoded, buf, bytesNeeded);
                }
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL CRYPT_AsnEncodeGeneralizedTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        SYSTEMTIME sysTime;
        /* sorry, magic number: enough for tag, len, YYYYMMDDHHMMSSZ.  I use a
         * temporary buffer because the output buffer is not NULL-terminated.
         */
        static const DWORD bytesNeeded = 17;
        char buf[40];

        if (!pbEncoded)
        {
            *pcbEncoded = bytesNeeded;
            ret = TRUE;
        }
        else
        {
            ret = FileTimeToSystemTime(pvStructInfo, &sysTime);
            if (ret)
                ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
                 pcbEncoded, bytesNeeded);
            if (ret)
            {
                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                buf[0] = ASN_GENERALTIME;
                buf[1] = bytesNeeded - 2;
                snprintf(buf + 2, sizeof(buf) - 2, "%04d%02d%02d%02d%02d%02dZ",
                 sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour,
                 sysTime.wMinute, sysTime.wSecond);
                memcpy(pbEncoded, buf, bytesNeeded);
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeChoiceOfTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        SYSTEMTIME sysTime;

        /* Check the year, if it's in the UTCTime range call that encode func */
        if (!FileTimeToSystemTime(pvStructInfo, &sysTime))
            return FALSE;
        if (sysTime.wYear >= 1950 && sysTime.wYear <= 2050)
            ret = CRYPT_AsnEncodeUtcTime(dwCertEncodingType, lpszStructType,
             pvStructInfo, dwFlags, pEncodePara, pbEncoded, pcbEncoded);
        else
            ret = CRYPT_AsnEncodeGeneralizedTime(dwCertEncodingType,
             lpszStructType, pvStructInfo, dwFlags, pEncodePara, pbEncoded,
             pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeSequenceOfAny(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        DWORD bytesNeeded, dataLen, lenBytes, i;
        const CRYPT_SEQUENCE_OF_ANY *seq = pvStructInfo;

        for (i = 0, dataLen = 0; i < seq->cValue; i++)
            dataLen += seq->rgValue[i].cbData;
        CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
        bytesNeeded = 1 + lenBytes + dataLen;
        if (!pbEncoded)
        {
            *pcbEncoded = bytesNeeded;
            ret = TRUE;
        }
        else
        {
            if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
             pcbEncoded, bytesNeeded)))
            {
                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                *pbEncoded++ = ASN_SEQUENCEOF;
                CRYPT_EncodeLen(dataLen, pbEncoded, &lenBytes);
                pbEncoded += lenBytes;
                for (i = 0; i < seq->cValue; i++)
                {
                    memcpy(pbEncoded, seq->rgValue[i].pbData,
                     seq->rgValue[i].cbData);
                    pbEncoded += seq->rgValue[i].cbData;
                }
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL CRYPT_AsnEncodeDistPoint(const CRL_DIST_POINT *distPoint,
 BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret = TRUE;
    struct AsnEncodeSequenceItem items[3] = { { 0 } };
    struct AsnConstructedItem constructed = { 0 };
    struct AsnEncodeTagSwappedItem swapped[3] = { { 0 } };
    DWORD cItem = 0, cSwapped = 0;

    switch (distPoint->DistPointName.dwDistPointNameChoice)
    {
    case CRL_DIST_POINT_NO_NAME:
        /* do nothing */
        break;
    case CRL_DIST_POINT_FULL_NAME:
        swapped[cSwapped].tag = ASN_CONTEXT | ASN_CONSTRUCTOR | 0;
        swapped[cSwapped].pvStructInfo = &distPoint->DistPointName.FullName;
        swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeAltName;
        constructed.tag = 0;
        constructed.pvStructInfo = &swapped[cSwapped];
        constructed.encodeFunc = CRYPT_AsnEncodeSwapTag;
        items[cItem].pvStructInfo = &constructed;
        items[cItem].encodeFunc = CRYPT_AsnEncodeConstructed;
        cSwapped++;
        cItem++;
        break;
    case CRL_DIST_POINT_ISSUER_RDN_NAME:
        FIXME("unimplemented for CRL_DIST_POINT_ISSUER_RDN_NAME\n");
        ret = FALSE;
        break;
    default:
        ret = FALSE;
    }
    if (ret && distPoint->ReasonFlags.cbData)
    {
        swapped[cSwapped].tag = ASN_CONTEXT | 1;
        swapped[cSwapped].pvStructInfo = &distPoint->ReasonFlags;
        swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeBits;
        items[cItem].pvStructInfo = &swapped[cSwapped];
        items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
        cSwapped++;
        cItem++;
    }
    if (ret && distPoint->CRLIssuer.cAltEntry)
    {
        swapped[cSwapped].tag = ASN_CONTEXT | ASN_CONSTRUCTOR | 2;
        swapped[cSwapped].pvStructInfo = &distPoint->CRLIssuer;
        swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeAltName;
        items[cItem].pvStructInfo = &swapped[cSwapped];
        items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
        cSwapped++;
        cItem++;
    }
    if (ret)
        ret = CRYPT_AsnEncodeSequence(X509_ASN_ENCODING, items, cItem, 0, NULL,
         pbEncoded, pcbEncoded);
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeCRLDistPoints(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CRL_DIST_POINTS_INFO *info = pvStructInfo;

        if (!info->cDistPoint)
        {
            SetLastError(E_INVALIDARG);
            ret = FALSE;
        }
        else
        {
            DWORD bytesNeeded, dataLen, lenBytes, i;

            ret = TRUE;
            for (i = 0, dataLen = 0; ret && i < info->cDistPoint; i++)
            {
                DWORD len;

                ret = CRYPT_AsnEncodeDistPoint(&info->rgDistPoint[i], NULL,
                 &len);
                if (ret)
                    dataLen += len;
                else if (GetLastError() == CRYPT_E_INVALID_IA5_STRING)
                {
                    /* Have to propagate index of failing character */
                    *pcbEncoded = len;
                }
            }
            if (ret)
            {
                CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
                bytesNeeded = 1 + lenBytes + dataLen;
                if (!pbEncoded)
                {
                    *pcbEncoded = bytesNeeded;
                    ret = TRUE;
                }
                else
                {
                    if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
                     pbEncoded, pcbEncoded, bytesNeeded)))
                    {
                        BYTE *out;

                        if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                            pbEncoded = *(BYTE **)pbEncoded;
                        out = pbEncoded;
                        *out++ = ASN_SEQUENCEOF;
                        CRYPT_EncodeLen(dataLen, out, &lenBytes);
                        out += lenBytes;
                        for (i = 0; ret && i < info->cDistPoint; i++)
                        {
                            DWORD len = dataLen;

                            ret = CRYPT_AsnEncodeDistPoint(
                             &info->rgDistPoint[i], out, &len);
                            if (ret)
                            {
                                out += len;
                                dataLen -= len;
                            }
                        }
                        if (!ret && (dwFlags & CRYPT_ENCODE_ALLOC_FLAG))
                            CRYPT_FreeSpace(pEncodePara, pbEncoded);
                    }
                }
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeEnhancedKeyUsage(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CERT_ENHKEY_USAGE *usage = pvStructInfo;
        DWORD bytesNeeded = 0, lenBytes, size, i;

        ret = TRUE;
        for (i = 0; ret && i < usage->cUsageIdentifier; i++)
        {
            ret = CRYPT_AsnEncodeOid(dwCertEncodingType, NULL,
             usage->rgpszUsageIdentifier[i],
             dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG, NULL, NULL, &size);
            if (ret)
                bytesNeeded += size;
        }
        CRYPT_EncodeLen(bytesNeeded, NULL, &lenBytes);
        bytesNeeded += 1 + lenBytes;
        if (ret)
        {
            if (!pbEncoded)
                *pcbEncoded = bytesNeeded;
            else
            {
                if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
                 pbEncoded, pcbEncoded, bytesNeeded)))
                {
                    BYTE *out;

                    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                        pbEncoded = *(BYTE **)pbEncoded;
                    out = pbEncoded;
                    *out++ = ASN_SEQUENCEOF;
                    CRYPT_EncodeLen(bytesNeeded - lenBytes - 1, out, &lenBytes);
                    out += lenBytes;
                    for (i = 0; ret && i < usage->cUsageIdentifier; i++)
                    {
                        size = bytesNeeded;
                        ret = CRYPT_AsnEncodeOid(dwCertEncodingType, NULL,
                         usage->rgpszUsageIdentifier[i],
                         dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG, NULL, out, &size);
                        if (ret)
                        {
                            out += size;
                            bytesNeeded -= size;
                        }
                    }
                    if (!ret && (dwFlags & CRYPT_ENCODE_ALLOC_FLAG))
                        CRYPT_FreeSpace(pEncodePara, pbEncoded);
                }
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeIssuingDistPoint(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CRL_ISSUING_DIST_POINT *point = pvStructInfo;
        struct AsnEncodeSequenceItem items[6] = { { 0 } };
        struct AsnConstructedItem constructed = { 0 };
        struct AsnEncodeTagSwappedItem swapped[5] = { { 0 } };
        DWORD cItem = 0, cSwapped = 0;

        ret = TRUE;
        switch (point->DistPointName.dwDistPointNameChoice)
        {
        case CRL_DIST_POINT_NO_NAME:
            /* do nothing */
            break;
        case CRL_DIST_POINT_FULL_NAME:
            swapped[cSwapped].tag = ASN_CONTEXT | ASN_CONSTRUCTOR | 0;
            swapped[cSwapped].pvStructInfo = &point->DistPointName.FullName;
            swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeAltName;
            constructed.tag = 0;
            constructed.pvStructInfo = &swapped[cSwapped];
            constructed.encodeFunc = CRYPT_AsnEncodeSwapTag;
            items[cItem].pvStructInfo = &constructed;
            items[cItem].encodeFunc = CRYPT_AsnEncodeConstructed;
            cSwapped++;
            cItem++;
            break;
        default:
            SetLastError(E_INVALIDARG);
            ret = FALSE;
        }
        if (ret && point->fOnlyContainsUserCerts)
        {
            swapped[cSwapped].tag = ASN_CONTEXT | 1;
            swapped[cSwapped].pvStructInfo = &point->fOnlyContainsUserCerts;
            swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeBool;
            items[cItem].pvStructInfo = &swapped[cSwapped];
            items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
            cSwapped++;
            cItem++;
        }
        if (ret && point->fOnlyContainsCACerts)
        {
            swapped[cSwapped].tag = ASN_CONTEXT | 2;
            swapped[cSwapped].pvStructInfo = &point->fOnlyContainsCACerts;
            swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeBool;
            items[cItem].pvStructInfo = &swapped[cSwapped];
            items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
            cSwapped++;
            cItem++;
        }
        if (ret && point->OnlySomeReasonFlags.cbData)
        {
            swapped[cSwapped].tag = ASN_CONTEXT | 3;
            swapped[cSwapped].pvStructInfo = &point->OnlySomeReasonFlags;
            swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeBits;
            items[cItem].pvStructInfo = &swapped[cSwapped];
            items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
            cSwapped++;
            cItem++;
        }
        if (ret && point->fIndirectCRL)
        {
            swapped[cSwapped].tag = ASN_CONTEXT | 4;
            swapped[cSwapped].pvStructInfo = &point->fIndirectCRL;
            swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeBool;
            items[cItem].pvStructInfo = &swapped[cSwapped];
            items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
            cSwapped++;
            cItem++;
        }
        if (ret)
            ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, cItem,
             dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL CRYPT_AsnEncodeGeneralSubtree(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;
    const CERT_GENERAL_SUBTREE *subtree = pvStructInfo;
    struct AsnEncodeSequenceItem items[3] = {
     { &subtree->Base, CRYPT_AsnEncodeAltNameEntry, 0 },
     { 0 }
    };
    struct AsnEncodeTagSwappedItem swapped[2] = { { 0 } };
    DWORD cItem = 1, cSwapped = 0;

    if (subtree->dwMinimum)
    {
        swapped[cSwapped].tag = ASN_CONTEXT | 0;
        swapped[cSwapped].pvStructInfo = &subtree->dwMinimum;
        swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeInt;
        items[cItem].pvStructInfo = &swapped[cSwapped];
        items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
        cSwapped++;
        cItem++;
    }
    if (subtree->fMaximum)
    {
        swapped[cSwapped].tag = ASN_CONTEXT | 1;
        swapped[cSwapped].pvStructInfo = &subtree->dwMaximum;
        swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeInt;
        items[cItem].pvStructInfo = &swapped[cSwapped];
        items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
        cSwapped++;
        cItem++;
    }
    ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, cItem, dwFlags,
     pEncodePara, pbEncoded, pcbEncoded);
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeNameConstraints(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret = FALSE;
    CRYPT_BLOB_ARRAY permitted = { 0, NULL }, excluded = { 0, NULL };

    TRACE("%p\n", pvStructInfo);

    __TRY
    {
        const CERT_NAME_CONSTRAINTS_INFO *constraints = pvStructInfo;
        struct AsnEncodeSequenceItem items[2] = { { 0 } };
        struct AsnEncodeTagSwappedItem swapped[2] = { { 0 } };
        DWORD i, cItem = 0, cSwapped = 0;

        ret = TRUE;
        if (constraints->cPermittedSubtree)
        {
            permitted.rgBlob = CryptMemAlloc(
             constraints->cPermittedSubtree * sizeof(CRYPT_DER_BLOB));
            if (permitted.rgBlob)
            {
                permitted.cBlob = constraints->cPermittedSubtree;
                memset(permitted.rgBlob, 0,
                 permitted.cBlob * sizeof(CRYPT_DER_BLOB));
                for (i = 0; ret && i < permitted.cBlob; i++)
                    ret = CRYPT_AsnEncodeGeneralSubtree(dwCertEncodingType,
                     NULL, &constraints->rgPermittedSubtree[i],
                     CRYPT_ENCODE_ALLOC_FLAG, NULL,
                     (BYTE *)&permitted.rgBlob[i].pbData,
                     &permitted.rgBlob[i].cbData);
                if (ret)
                {
                    swapped[cSwapped].tag = ASN_CONTEXT | ASN_CONSTRUCTOR | 0;
                    swapped[cSwapped].pvStructInfo = &permitted;
                    swapped[cSwapped].encodeFunc = CRYPT_DEREncodeSet;
                    items[cItem].pvStructInfo = &swapped[cSwapped];
                    items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
                    cSwapped++;
                    cItem++;
                }
            }
            else
                ret = FALSE;
        }
        if (constraints->cExcludedSubtree)
        {
            excluded.rgBlob = CryptMemAlloc(
             constraints->cExcludedSubtree * sizeof(CRYPT_DER_BLOB));
            if (excluded.rgBlob)
            {
                excluded.cBlob = constraints->cExcludedSubtree;
                memset(excluded.rgBlob, 0,
                 excluded.cBlob * sizeof(CRYPT_DER_BLOB));
                for (i = 0; ret && i < excluded.cBlob; i++)
                    ret = CRYPT_AsnEncodeGeneralSubtree(dwCertEncodingType,
                     NULL, &constraints->rgExcludedSubtree[i],
                     CRYPT_ENCODE_ALLOC_FLAG, NULL,
                     (BYTE *)&excluded.rgBlob[i].pbData,
                     &excluded.rgBlob[i].cbData);
                if (ret)
                {
                    swapped[cSwapped].tag = ASN_CONTEXT | ASN_CONSTRUCTOR | 1;
                    swapped[cSwapped].pvStructInfo = &excluded;
                    swapped[cSwapped].encodeFunc = CRYPT_DEREncodeSet;
                    items[cItem].pvStructInfo = &swapped[cSwapped];
                    items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
                    cSwapped++;
                    cItem++;
                }
            }
            else
                ret = FALSE;
        }
        if (ret)
            ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, cItem,
             dwFlags, pEncodePara, pbEncoded, pcbEncoded);
        for (i = 0; i < permitted.cBlob; i++)
            LocalFree(permitted.rgBlob[i].pbData);
        for (i = 0; i < excluded.cBlob; i++)
            LocalFree(excluded.rgBlob[i].pbData);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    CryptMemFree(permitted.rgBlob);
    CryptMemFree(excluded.rgBlob);
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeIssuerSerialNumber(
 DWORD dwCertEncodingType, LPCSTR lpszStructType, const void *pvStructInfo,
 DWORD dwFlags, PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    BOOL ret;
    const CERT_ISSUER_SERIAL_NUMBER *issuerSerial = pvStructInfo;
    struct AsnEncodeSequenceItem items[] = {
     { &issuerSerial->Issuer,       CRYPT_CopyEncodedBlob, 0 },
     { &issuerSerial->SerialNumber, CRYPT_AsnEncodeInteger, 0 },
    };

    ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items,
     ARRAY_SIZE(items), dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodePKCSSignerInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret = FALSE;

    if (!(dwCertEncodingType & PKCS_7_ASN_ENCODING))
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }

    __TRY
    {
        const CMSG_SIGNER_INFO *info = pvStructInfo;

        if (!info->Issuer.cbData)
            SetLastError(E_INVALIDARG);
        else
        {
            struct AsnEncodeSequenceItem items[7] = {
             { &info->dwVersion,     CRYPT_AsnEncodeInt, 0 },
             { &info->Issuer,        CRYPT_AsnEncodeIssuerSerialNumber, 0 },
             { &info->HashAlgorithm, CRYPT_AsnEncodeAlgorithmIdWithNullParams,
               0 },
            };
            struct AsnEncodeTagSwappedItem swapped[2] = { { 0 } };
            DWORD cItem = 3, cSwapped = 0;

            if (info->AuthAttrs.cAttr)
            {
                swapped[cSwapped].tag = ASN_CONTEXT | ASN_CONSTRUCTOR | 0;
                swapped[cSwapped].pvStructInfo = &info->AuthAttrs;
                swapped[cSwapped].encodeFunc = CRYPT_AsnEncodePKCSAttributes;
                items[cItem].pvStructInfo = &swapped[cSwapped];
                items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
                cSwapped++;
                cItem++;
            }
            items[cItem].pvStructInfo = &info->HashEncryptionAlgorithm;
            items[cItem].encodeFunc = CRYPT_AsnEncodeAlgorithmIdWithNullParams;
            cItem++;
            items[cItem].pvStructInfo = &info->EncryptedHash;
            items[cItem].encodeFunc = CRYPT_AsnEncodeOctets;
            cItem++;
            if (info->UnauthAttrs.cAttr)
            {
                swapped[cSwapped].tag = ASN_CONTEXT | ASN_CONSTRUCTOR | 1;
                swapped[cSwapped].pvStructInfo = &info->UnauthAttrs;
                swapped[cSwapped].encodeFunc = CRYPT_AsnEncodePKCSAttributes;
                items[cItem].pvStructInfo = &swapped[cSwapped];
                items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
                cSwapped++;
                cItem++;
            }
            ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, cItem,
             dwFlags, pEncodePara, pbEncoded, pcbEncoded);
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeCMSSignerInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret = FALSE;

    if (!(dwCertEncodingType & PKCS_7_ASN_ENCODING))
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }

    __TRY
    {
        const CMSG_CMS_SIGNER_INFO *info = pvStructInfo;

        if (info->SignerId.dwIdChoice != CERT_ID_ISSUER_SERIAL_NUMBER &&
         info->SignerId.dwIdChoice != CERT_ID_KEY_IDENTIFIER)
            SetLastError(E_INVALIDARG);
        else if (info->SignerId.dwIdChoice == CERT_ID_ISSUER_SERIAL_NUMBER &&
         !info->SignerId.IssuerSerialNumber.Issuer.cbData)
            SetLastError(E_INVALIDARG);
        else
        {
            struct AsnEncodeSequenceItem items[7] = {
             { &info->dwVersion,     CRYPT_AsnEncodeInt, 0 },
            };
            struct AsnEncodeTagSwappedItem swapped[3] = { { 0 } };
            DWORD cItem = 1, cSwapped = 0;

            if (info->SignerId.dwIdChoice == CERT_ID_ISSUER_SERIAL_NUMBER)
            {
                items[cItem].pvStructInfo =
                 &info->SignerId.IssuerSerialNumber.Issuer;
                items[cItem].encodeFunc =
                 CRYPT_AsnEncodeIssuerSerialNumber;
                cItem++;
            }
            else
            {
                swapped[cSwapped].tag = ASN_CONTEXT | 0;
                swapped[cSwapped].pvStructInfo = &info->SignerId.KeyId;
                swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeOctets;
                items[cItem].pvStructInfo = &swapped[cSwapped];
                items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
                cSwapped++;
                cItem++;
            }
            items[cItem].pvStructInfo = &info->HashAlgorithm;
            items[cItem].encodeFunc = CRYPT_AsnEncodeAlgorithmIdWithNullParams;
            cItem++;
            if (info->AuthAttrs.cAttr)
            {
                swapped[cSwapped].tag = ASN_CONTEXT | ASN_CONSTRUCTOR | 0;
                swapped[cSwapped].pvStructInfo = &info->AuthAttrs;
                swapped[cSwapped].encodeFunc = CRYPT_AsnEncodePKCSAttributes;
                items[cItem].pvStructInfo = &swapped[cSwapped];
                items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
                cSwapped++;
                cItem++;
            }
            items[cItem].pvStructInfo = &info->HashEncryptionAlgorithm;
            items[cItem].encodeFunc = CRYPT_AsnEncodeAlgorithmIdWithNullParams;
            cItem++;
            items[cItem].pvStructInfo = &info->EncryptedHash;
            items[cItem].encodeFunc = CRYPT_AsnEncodeOctets;
            cItem++;
            if (info->UnauthAttrs.cAttr)
            {
                swapped[cSwapped].tag = ASN_CONTEXT | ASN_CONSTRUCTOR | 1;
                swapped[cSwapped].pvStructInfo = &info->UnauthAttrs;
                swapped[cSwapped].encodeFunc = CRYPT_AsnEncodePKCSAttributes;
                items[cItem].pvStructInfo = &swapped[cSwapped];
                items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
                cSwapped++;
                cItem++;
            }
            ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, cItem,
             dwFlags, pEncodePara, pbEncoded, pcbEncoded);
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    return ret;
}

BOOL CRYPT_AsnEncodeCMSSignedInfo(CRYPT_SIGNED_INFO *signedInfo, void *pvData,
 DWORD *pcbData)
{
    struct DERSetDescriptor digestAlgorithmsSet = { signedInfo->cSignerInfo,
     signedInfo->rgSignerInfo, sizeof(CMSG_CMS_SIGNER_INFO),
     offsetof(CMSG_CMS_SIGNER_INFO, HashAlgorithm),
     CRYPT_AsnEncodeAlgorithmIdWithNullParams };
    struct AsnEncodeSequenceItem items[7] = {
     { &signedInfo->version, CRYPT_AsnEncodeInt, 0 },
     { &digestAlgorithmsSet, CRYPT_DEREncodeItemsAsSet, 0 },
     { &signedInfo->content, CRYPT_AsnEncodePKCSContentInfoInternal, 0 }
    };
    struct DERSetDescriptor certSet = { 0 }, crlSet = { 0 }, signerSet = { 0 };
    struct AsnEncodeTagSwappedItem swapped[2] = { { 0 } };
    DWORD cItem = 3, cSwapped = 0;

    if (signedInfo->cCertEncoded)
    {
        certSet.cItems = signedInfo->cCertEncoded;
        certSet.items = signedInfo->rgCertEncoded;
        certSet.itemSize = sizeof(CERT_BLOB);
        certSet.itemOffset = 0;
        certSet.encode = CRYPT_CopyEncodedBlob;
        swapped[cSwapped].tag = ASN_CONSTRUCTOR | ASN_CONTEXT | 0;
        swapped[cSwapped].pvStructInfo = &certSet;
        swapped[cSwapped].encodeFunc = CRYPT_DEREncodeItemsAsSet;
        items[cItem].pvStructInfo = &swapped[cSwapped];
        items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
        cSwapped++;
        cItem++;
    }
    if (signedInfo->cCrlEncoded)
    {
        crlSet.cItems = signedInfo->cCrlEncoded;
        crlSet.items = signedInfo->rgCrlEncoded;
        crlSet.itemSize = sizeof(CRL_BLOB);
        crlSet.itemOffset = 0;
        crlSet.encode = CRYPT_CopyEncodedBlob;
        swapped[cSwapped].tag = ASN_CONSTRUCTOR | ASN_CONTEXT | 1;
        swapped[cSwapped].pvStructInfo = &crlSet;
        swapped[cSwapped].encodeFunc = CRYPT_DEREncodeItemsAsSet;
        items[cItem].pvStructInfo = &swapped[cSwapped];
        items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
        cSwapped++;
        cItem++;
    }
    signerSet.cItems = signedInfo->cSignerInfo;
    signerSet.items = signedInfo->rgSignerInfo;
    signerSet.itemSize = sizeof(CMSG_CMS_SIGNER_INFO);
    signerSet.itemOffset = 0;
    signerSet.encode = CRYPT_AsnEncodeCMSSignerInfo;
    items[cItem].pvStructInfo = &signerSet;
    items[cItem].encodeFunc = CRYPT_DEREncodeItemsAsSet;
    cItem++;

    return CRYPT_AsnEncodeSequence(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
     items, cItem, 0, NULL, pvData, pcbData);
}

static BOOL WINAPI CRYPT_AsnEncodeRecipientInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    const CMSG_KEY_TRANS_RECIPIENT_INFO *info = pvStructInfo;
    struct AsnEncodeSequenceItem items[] = {
     { &info->dwVersion, CRYPT_AsnEncodeInt, 0 },
     { &info->RecipientId.IssuerSerialNumber,
       CRYPT_AsnEncodeIssuerSerialNumber, 0 },
     { &info->KeyEncryptionAlgorithm,
       CRYPT_AsnEncodeAlgorithmIdWithNullParams, 0 },
     { &info->EncryptedKey, CRYPT_AsnEncodeOctets, 0 },
    };

    return CRYPT_AsnEncodeSequence(dwCertEncodingType, items,
     ARRAY_SIZE(items), dwFlags, pEncodePara, pbEncoded, pcbEncoded);
}

static BOOL WINAPI CRYPT_AsnEncodeEncryptedContentInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    const CRYPT_ENCRYPTED_CONTENT_INFO *info = pvStructInfo;
    struct AsnEncodeTagSwappedItem swapped = { ASN_CONTEXT | 0,
     &info->encryptedContent, CRYPT_AsnEncodeOctets };
    struct AsnEncodeSequenceItem items[] = {
     { info->contentType, CRYPT_AsnEncodeOid, 0 },
     { &info->contentEncryptionAlgorithm,
       CRYPT_AsnEncodeAlgorithmIdWithNullParams, 0 },
     { &swapped, CRYPT_AsnEncodeSwapTag, 0 },
    };

    return CRYPT_AsnEncodeSequence(dwCertEncodingType, items,
     ARRAY_SIZE(items), dwFlags, pEncodePara, pbEncoded, pcbEncoded);
}

BOOL CRYPT_AsnEncodePKCSEnvelopedData(const CRYPT_ENVELOPED_DATA *envelopedData,
 void *pvData, DWORD *pcbData)
{
    struct DERSetDescriptor recipientInfosSet = { envelopedData->cRecipientInfo,
     envelopedData->rgRecipientInfo, sizeof(CMSG_KEY_TRANS_RECIPIENT_INFO), 0,
     CRYPT_AsnEncodeRecipientInfo };
    struct AsnEncodeSequenceItem items[] = {
     { &envelopedData->version, CRYPT_AsnEncodeInt, 0 },
     { &recipientInfosSet, CRYPT_DEREncodeItemsAsSet, 0 },
     { &envelopedData->encryptedContentInfo,
       CRYPT_AsnEncodeEncryptedContentInfo, 0 },
    };

    return CRYPT_AsnEncodeSequence(X509_ASN_ENCODING, items,
     ARRAY_SIZE(items), 0, NULL, pvData, pcbData);
}

static BOOL WINAPI CRYPT_AsnEncodeCertId(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const OCSP_CERT_ID *id = pvStructInfo;
        struct AsnEncodeSequenceItem items[] = {
         { &id->HashAlgorithm, CRYPT_AsnEncodeAlgorithmIdWithNullParams, 0 },
         { &id->IssuerNameHash, CRYPT_AsnEncodeOctets, 0 },
         { &id->IssuerKeyHash, CRYPT_AsnEncodeOctets, 0 },
         { &id->SerialNumber, CRYPT_AsnEncodeInteger, 0 },
        };

        ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items,
         ARRAY_SIZE(items), dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL CRYPT_AsnEncodeOCSPRequestEntry(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const OCSP_REQUEST_ENTRY *info = pvStructInfo;
        struct AsnEncodeSequenceItem items[] = {
         { &info->CertId, CRYPT_AsnEncodeCertId, 0 },
        };

        if (info->cExtension)
        {
            FIXME("extensions not supported\n");
            SetLastError(E_INVALIDARG);
            ret = FALSE;
        }
        else
        {
            ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items,
             ARRAY_SIZE(items), dwFlags, pEncodePara, pbEncoded, pcbEncoded);
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

struct ocsp_request_list
{
    DWORD count;
    OCSP_REQUEST_ENTRY *entry;
};

static BOOL WINAPI CRYPT_AsnEncodeOCSPRequestEntries(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        DWORD bytesNeeded, dataLen, lenBytes, i;
        const struct ocsp_request_list *list = pvStructInfo;

        ret = TRUE;
        for (i = 0, dataLen = 0; ret && i < list->count; i++)
        {
            DWORD size;
            ret = CRYPT_AsnEncodeOCSPRequestEntry(dwCertEncodingType, lpszStructType, &list->entry[i],
             dwFlags, pEncodePara, NULL, &size);
            if (ret)
                dataLen += size;
        }
        if (ret)
        {
            CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
            bytesNeeded = 1 + lenBytes + dataLen;
            if (!pbEncoded)
                *pcbEncoded = bytesNeeded;
            else
            {
                if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
                 pbEncoded, pcbEncoded, bytesNeeded)))
                {
                    BYTE *out;

                    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                        pbEncoded = *(BYTE **)pbEncoded;
                    out = pbEncoded;
                    *out++ = ASN_SEQUENCEOF;
                    CRYPT_EncodeLen(dataLen, out, &lenBytes);
                    out += lenBytes;
                    for (i = 0; i < list->count; i++)
                    {
                        DWORD size = dataLen;

                        ret = CRYPT_AsnEncodeOCSPRequestEntry(dwCertEncodingType, lpszStructType,
                         &list->entry[i], dwFlags, pEncodePara, out, &size);
                        out += size;
                        dataLen -= size;
                    }

                    if (!ret && (dwFlags & CRYPT_ENCODE_ALLOC_FLAG))
                        CRYPT_FreeSpace(pEncodePara, pbEncoded);
                }
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeOCSPRequest(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const OCSP_REQUEST_INFO *info = pvStructInfo;

        if (info->dwVersion != OCSP_REQUEST_V1)
        {
            FIXME("version %lu not supported\n", info->dwVersion);
            SetLastError(E_INVALIDARG);
            ret = FALSE;
        }
        else
        {
            if (info->cExtension)
            {
                FIXME("extensions not supported\n");
                SetLastError(E_INVALIDARG);
                ret = FALSE;
            }
            else
            {
                struct AsnConstructedItem name;
                struct AsnEncodeSequenceItem items[2];
                DWORD count = 0;

                if (info->pRequestorName)
                {
                    name.tag = 1;
                    name.pvStructInfo = info->pRequestorName;
                    name.encodeFunc = CRYPT_AsnEncodeAltNameEntry;
                    items[count].pvStructInfo = &name;
                    items[count].encodeFunc = CRYPT_AsnEncodeConstructed;
                    count++;
                }
                if (info->cRequestEntry)
                {
                    items[count].pvStructInfo = &info->cRequestEntry;
                    items[count].encodeFunc = CRYPT_AsnEncodeOCSPRequestEntries;
                    count++;
                }

                ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items,
                 count, dwFlags, pEncodePara, pbEncoded, pcbEncoded);
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeOCSPSignedRequest(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const OCSP_SIGNED_REQUEST_INFO *info = pvStructInfo;
        struct AsnEncodeSequenceItem items[] = {
         { &info->ToBeSigned, CRYPT_CopyEncodedBlob, 0 },
        };

        if (info->pOptionalSignatureInfo) FIXME("pOptionalSignatureInfo not supported\n");

        ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items,
         ARRAY_SIZE(items), dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static CryptEncodeObjectExFunc CRYPT_GetBuiltinEncoder(DWORD dwCertEncodingType,
 LPCSTR lpszStructType)
{
    CryptEncodeObjectExFunc encodeFunc = NULL;

    if ((dwCertEncodingType & CERT_ENCODING_TYPE_MASK) != X509_ASN_ENCODING
     && (dwCertEncodingType & CMSG_ENCODING_TYPE_MASK) != PKCS_7_ASN_ENCODING)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return NULL;
    }

    if (IS_INTOID(lpszStructType))
    {
        switch (LOWORD(lpszStructType))
        {
        case LOWORD(X509_CERT):
            encodeFunc = CRYPT_AsnEncodeCert;
            break;
        case LOWORD(X509_CERT_TO_BE_SIGNED):
            encodeFunc = CRYPT_AsnEncodeCertInfo;
            break;
        case LOWORD(X509_CERT_CRL_TO_BE_SIGNED):
            encodeFunc = CRYPT_AsnEncodeCRLInfo;
            break;
        case LOWORD(X509_EXTENSIONS):
            encodeFunc = CRYPT_AsnEncodeExtensions;
            break;
        case LOWORD(X509_NAME_VALUE):
            encodeFunc = CRYPT_AsnEncodeNameValue;
            break;
        case LOWORD(X509_NAME):
            encodeFunc = CRYPT_AsnEncodeName;
            break;
        case LOWORD(X509_PUBLIC_KEY_INFO):
            encodeFunc = CRYPT_AsnEncodePubKeyInfo;
            break;
        case LOWORD(X509_AUTHORITY_KEY_ID):
            encodeFunc = CRYPT_AsnEncodeAuthorityKeyId;
            break;
        case LOWORD(X509_ALTERNATE_NAME):
            encodeFunc = CRYPT_AsnEncodeAltName;
            break;
        case LOWORD(X509_BASIC_CONSTRAINTS):
            encodeFunc = CRYPT_AsnEncodeBasicConstraints;
            break;
        case LOWORD(X509_BASIC_CONSTRAINTS2):
            encodeFunc = CRYPT_AsnEncodeBasicConstraints2;
            break;
        case LOWORD(X509_CERT_POLICIES):
            encodeFunc = CRYPT_AsnEncodeCertPolicies;
            break;
        case LOWORD(RSA_CSP_PUBLICKEYBLOB):
            encodeFunc = CRYPT_AsnEncodeRsaPubKey;
            break;
        case LOWORD(X509_UNICODE_NAME):
            encodeFunc = CRYPT_AsnEncodeUnicodeName;
            break;
        case LOWORD(PKCS_CONTENT_INFO):
            encodeFunc = CRYPT_AsnEncodePKCSContentInfo;
            break;
        case LOWORD(PKCS_ATTRIBUTE):
            encodeFunc = CRYPT_AsnEncodePKCSAttribute;
            break;
        case LOWORD(X509_UNICODE_NAME_VALUE):
            encodeFunc = CRYPT_AsnEncodeUnicodeNameValue;
            break;
        case LOWORD(X509_OCTET_STRING):
            encodeFunc = CRYPT_AsnEncodeOctets;
            break;
        case LOWORD(X509_BITS):
        case LOWORD(X509_KEY_USAGE):
            encodeFunc = CRYPT_AsnEncodeBits;
            break;
        case LOWORD(X509_INTEGER):
            encodeFunc = CRYPT_AsnEncodeInt;
            break;
        case LOWORD(X509_MULTI_BYTE_INTEGER):
            encodeFunc = CRYPT_AsnEncodeInteger;
            break;
        case LOWORD(X509_MULTI_BYTE_UINT):
            encodeFunc = CRYPT_AsnEncodeUnsignedInteger;
            break;
        case LOWORD(X509_ENUMERATED):
            encodeFunc = CRYPT_AsnEncodeEnumerated;
            break;
        case LOWORD(X509_CHOICE_OF_TIME):
            encodeFunc = CRYPT_AsnEncodeChoiceOfTime;
            break;
        case LOWORD(X509_AUTHORITY_KEY_ID2):
            encodeFunc = CRYPT_AsnEncodeAuthorityKeyId2;
            break;
        case LOWORD(X509_AUTHORITY_INFO_ACCESS):
            encodeFunc = CRYPT_AsnEncodeAuthorityInfoAccess;
            break;
        case LOWORD(X509_SEQUENCE_OF_ANY):
            encodeFunc = CRYPT_AsnEncodeSequenceOfAny;
            break;
        case LOWORD(PKCS_UTC_TIME):
            encodeFunc = CRYPT_AsnEncodeUtcTime;
            break;
        case LOWORD(X509_CRL_DIST_POINTS):
            encodeFunc = CRYPT_AsnEncodeCRLDistPoints;
            break;
        case LOWORD(X509_ENHANCED_KEY_USAGE):
            encodeFunc = CRYPT_AsnEncodeEnhancedKeyUsage;
            break;
        case LOWORD(PKCS_CTL):
            encodeFunc = CRYPT_AsnEncodeCTL;
            break;
        case LOWORD(PKCS_SMIME_CAPABILITIES):
            encodeFunc = CRYPT_AsnEncodeSMIMECapabilities;
            break;
        case LOWORD(X509_PKIX_POLICY_QUALIFIER_USERNOTICE):
            encodeFunc = CRYPT_AsnEncodePolicyQualifierUserNotice;
            break;
        case LOWORD(PKCS_ATTRIBUTES):
            encodeFunc = CRYPT_AsnEncodePKCSAttributes;
            break;
        case LOWORD(X509_ISSUING_DIST_POINT):
            encodeFunc = CRYPT_AsnEncodeIssuingDistPoint;
            break;
        case LOWORD(X509_NAME_CONSTRAINTS):
            encodeFunc = CRYPT_AsnEncodeNameConstraints;
            break;
        case LOWORD(X509_POLICY_MAPPINGS):
            encodeFunc = CRYPT_AsnEncodeCertPolicyMappings;
            break;
        case LOWORD(X509_POLICY_CONSTRAINTS):
            encodeFunc = CRYPT_AsnEncodeCertPolicyConstraints;
            break;
        case LOWORD(PKCS7_SIGNER_INFO):
            encodeFunc = CRYPT_AsnEncodePKCSSignerInfo;
            break;
        case LOWORD(CMS_SIGNER_INFO):
            encodeFunc = CRYPT_AsnEncodeCMSSignerInfo;
            break;
        case LOWORD(CNG_RSA_PUBLIC_KEY_BLOB):
            encodeFunc = CRYPT_AsnEncodeRsaPubKey_Bcrypt;
            break;
        case LOWORD(OCSP_REQUEST):
            encodeFunc = CRYPT_AsnEncodeOCSPRequest;
            break;
        case LOWORD(OCSP_SIGNED_REQUEST):
            encodeFunc = CRYPT_AsnEncodeOCSPSignedRequest;
            break;
        default:
            FIXME("Unimplemented encoder for lpszStructType OID %d\n", LOWORD(lpszStructType));
        }
    }
    else if (!strcmp(lpszStructType, szOID_CERT_EXTENSIONS))
        encodeFunc = CRYPT_AsnEncodeExtensions;
    else if (!strcmp(lpszStructType, szOID_RSA_signingTime))
        encodeFunc = CRYPT_AsnEncodeUtcTime;
    else if (!strcmp(lpszStructType, szOID_RSA_SMIMECapabilities))
        encodeFunc = CRYPT_AsnEncodeUtcTime;
    else if (!strcmp(lpszStructType, szOID_AUTHORITY_KEY_IDENTIFIER))
        encodeFunc = CRYPT_AsnEncodeAuthorityKeyId;
    else if (!strcmp(lpszStructType, szOID_LEGACY_POLICY_MAPPINGS))
        encodeFunc = CRYPT_AsnEncodeCertPolicyMappings;
    else if (!strcmp(lpszStructType, szOID_AUTHORITY_KEY_IDENTIFIER2))
        encodeFunc = CRYPT_AsnEncodeAuthorityKeyId2;
    else if (!strcmp(lpszStructType, szOID_CRL_REASON_CODE))
        encodeFunc = CRYPT_AsnEncodeEnumerated;
    else if (!strcmp(lpszStructType, szOID_KEY_USAGE))
        encodeFunc = CRYPT_AsnEncodeBits;
    else if (!strcmp(lpszStructType, szOID_SUBJECT_KEY_IDENTIFIER))
        encodeFunc = CRYPT_AsnEncodeOctets;
    else if (!strcmp(lpszStructType, szOID_BASIC_CONSTRAINTS))
        encodeFunc = CRYPT_AsnEncodeBasicConstraints;
    else if (!strcmp(lpszStructType, szOID_BASIC_CONSTRAINTS2))
        encodeFunc = CRYPT_AsnEncodeBasicConstraints2;
    else if (!strcmp(lpszStructType, szOID_ISSUER_ALT_NAME))
        encodeFunc = CRYPT_AsnEncodeAltName;
    else if (!strcmp(lpszStructType, szOID_ISSUER_ALT_NAME2))
        encodeFunc = CRYPT_AsnEncodeAltName;
    else if (!strcmp(lpszStructType, szOID_NEXT_UPDATE_LOCATION))
        encodeFunc = CRYPT_AsnEncodeAltName;
    else if (!strcmp(lpszStructType, szOID_SUBJECT_ALT_NAME))
        encodeFunc = CRYPT_AsnEncodeAltName;
    else if (!strcmp(lpszStructType, szOID_SUBJECT_ALT_NAME2))
        encodeFunc = CRYPT_AsnEncodeAltName;
    else if (!strcmp(lpszStructType, szOID_CRL_DIST_POINTS))
        encodeFunc = CRYPT_AsnEncodeCRLDistPoints;
    else if (!strcmp(lpszStructType, szOID_CERT_POLICIES))
        encodeFunc = CRYPT_AsnEncodeCertPolicies;
    else if (!strcmp(lpszStructType, szOID_POLICY_MAPPINGS))
        encodeFunc = CRYPT_AsnEncodeCertPolicyMappings;
    else if (!strcmp(lpszStructType, szOID_POLICY_CONSTRAINTS))
        encodeFunc = CRYPT_AsnEncodeCertPolicyConstraints;
    else if (!strcmp(lpszStructType, szOID_ENHANCED_KEY_USAGE))
        encodeFunc = CRYPT_AsnEncodeEnhancedKeyUsage;
    else if (!strcmp(lpszStructType, szOID_ISSUING_DIST_POINT))
        encodeFunc = CRYPT_AsnEncodeIssuingDistPoint;
    else if (!strcmp(lpszStructType, szOID_NAME_CONSTRAINTS))
        encodeFunc = CRYPT_AsnEncodeNameConstraints;
    else if (!strcmp(lpszStructType, szOID_AUTHORITY_INFO_ACCESS))
        encodeFunc = CRYPT_AsnEncodeAuthorityInfoAccess;
    else if (!strcmp(lpszStructType, szOID_PKIX_POLICY_QUALIFIER_USERNOTICE))
        encodeFunc = CRYPT_AsnEncodePolicyQualifierUserNotice;
    else if (!strcmp(lpszStructType, szOID_CTL))
        encodeFunc = CRYPT_AsnEncodeCTL;
    else
        FIXME("Unsupported encoder for lpszStructType %s\n", lpszStructType);
    return encodeFunc;
}

static CryptEncodeObjectFunc CRYPT_LoadEncoderFunc(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, HCRYPTOIDFUNCADDR *hFunc)
{
    static HCRYPTOIDFUNCSET set = NULL;
    CryptEncodeObjectFunc encodeFunc = NULL;

    if (!set)
        set = CryptInitOIDFunctionSet(CRYPT_OID_ENCODE_OBJECT_FUNC, 0);
    CryptGetOIDFunctionAddress(set, dwCertEncodingType, lpszStructType, 0,
     (void **)&encodeFunc, hFunc);
    return encodeFunc;
}

static CryptEncodeObjectExFunc CRYPT_LoadEncoderExFunc(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, HCRYPTOIDFUNCADDR *hFunc)
{
    static HCRYPTOIDFUNCSET set = NULL;
    CryptEncodeObjectExFunc encodeFunc = NULL;

    if (!set)
        set = CryptInitOIDFunctionSet(CRYPT_OID_ENCODE_OBJECT_EX_FUNC, 0);
    CryptGetOIDFunctionAddress(set, dwCertEncodingType, lpszStructType, 0,
     (void **)&encodeFunc, hFunc);
    return encodeFunc;
}

BOOL WINAPI CryptEncodeObject(DWORD dwCertEncodingType, LPCSTR lpszStructType,
 const void *pvStructInfo, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret = FALSE;
    HCRYPTOIDFUNCADDR hFunc = NULL;
    CryptEncodeObjectFunc pCryptEncodeObject = NULL;
    CryptEncodeObjectExFunc pCryptEncodeObjectEx = NULL;

    TRACE_(crypt)("(0x%08lx, %s, %p, %p, %p)\n", dwCertEncodingType,
     debugstr_a(lpszStructType), pvStructInfo, pbEncoded,
     pcbEncoded);

    if (!pbEncoded && !pcbEncoded)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!(pCryptEncodeObjectEx = CRYPT_GetBuiltinEncoder(dwCertEncodingType,
     lpszStructType)))
    {
        TRACE_(crypt)("OID %s not found or unimplemented, looking for DLL\n",
         debugstr_a(lpszStructType));
        pCryptEncodeObject = CRYPT_LoadEncoderFunc(dwCertEncodingType,
         lpszStructType, &hFunc);
        if (!pCryptEncodeObject)
            pCryptEncodeObjectEx = CRYPT_LoadEncoderExFunc(dwCertEncodingType,
             lpszStructType, &hFunc);
    }
    if (pCryptEncodeObject)
        ret = pCryptEncodeObject(dwCertEncodingType, lpszStructType,
         pvStructInfo, pbEncoded, pcbEncoded);
    else if (pCryptEncodeObjectEx)
        ret = pCryptEncodeObjectEx(dwCertEncodingType, lpszStructType,
         pvStructInfo, 0, NULL, pbEncoded, pcbEncoded);
    if (hFunc)
        CryptFreeOIDFunctionAddress(hFunc, 0);
    TRACE_(crypt)("returning %d\n", ret);
    return ret;
}

BOOL WINAPI CryptEncodeObjectEx(DWORD dwCertEncodingType, LPCSTR lpszStructType,
 const void *pvStructInfo, DWORD dwFlags, PCRYPT_ENCODE_PARA pEncodePara,
 void *pvEncoded, DWORD *pcbEncoded)
{
    BOOL ret = FALSE;
    HCRYPTOIDFUNCADDR hFunc = NULL;
    CryptEncodeObjectExFunc encodeFunc = NULL;

    TRACE_(crypt)("(0x%08lx, %s, %p, 0x%08lx, %p, %p, %p)\n", dwCertEncodingType,
     debugstr_a(lpszStructType), pvStructInfo, dwFlags, pEncodePara,
     pvEncoded, pcbEncoded);

    if (!pvEncoded && !pcbEncoded)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    SetLastError(NOERROR);
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG) {
        if (!pvEncoded) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        *(BYTE **)pvEncoded = NULL;
    }
    encodeFunc = CRYPT_GetBuiltinEncoder(dwCertEncodingType, lpszStructType);
    if (!encodeFunc)
    {
        TRACE_(crypt)("OID %s not found or unimplemented, looking for DLL\n",
         debugstr_a(lpszStructType));
        encodeFunc = CRYPT_LoadEncoderExFunc(dwCertEncodingType, lpszStructType,
         &hFunc);
    }
    if (encodeFunc)
        ret = encodeFunc(dwCertEncodingType, lpszStructType, pvStructInfo,
         dwFlags, pEncodePara, pvEncoded, pcbEncoded);
    else
    {
        CryptEncodeObjectFunc pCryptEncodeObject =
         CRYPT_LoadEncoderFunc(dwCertEncodingType, lpszStructType, &hFunc);

        if (pCryptEncodeObject)
        {
            if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
            {
                ret = pCryptEncodeObject(dwCertEncodingType, lpszStructType,
                 pvStructInfo, NULL, pcbEncoded);
                if (ret && (ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
                 pvEncoded, pcbEncoded, *pcbEncoded)))
                    ret = pCryptEncodeObject(dwCertEncodingType,
                     lpszStructType, pvStructInfo, *(BYTE **)pvEncoded,
                     pcbEncoded);
            }
            else
                ret = pCryptEncodeObject(dwCertEncodingType, lpszStructType,
                 pvStructInfo, pvEncoded, pcbEncoded);
        }
    }
    if (hFunc)
        CryptFreeOIDFunctionAddress(hFunc, 0);
    TRACE_(crypt)("returning %d\n", ret);
    return ret;
}

BOOL WINAPI CryptExportPublicKeyInfo(HCRYPTPROV_OR_NCRYPT_KEY_HANDLE hCryptProv, DWORD dwKeySpec,
 DWORD dwCertEncodingType, PCERT_PUBLIC_KEY_INFO pInfo, DWORD *pcbInfo)
{
    return CryptExportPublicKeyInfoEx(hCryptProv, dwKeySpec, dwCertEncodingType,
     NULL, 0, NULL, pInfo, pcbInfo);
}

typedef BOOL (WINAPI *EncodePublicKeyAndParametersFunc)(DWORD dwCertEncodingType,
 LPSTR pszPublicKeyObjId, BYTE *pbPubKey, DWORD cbPubKey, DWORD dwFlags, void *pvAuxInfo,
 BYTE **ppPublicKey, DWORD *pcbPublicKey, BYTE **ppbParams, DWORD *pcbParams);

static BOOL WINAPI CRYPT_ExportPublicKeyInfoEx(HCRYPTPROV_OR_NCRYPT_KEY_HANDLE hCryptProv,
 DWORD dwKeySpec, DWORD dwCertEncodingType, LPSTR pszPublicKeyObjId,
 DWORD dwFlags, void *pvAuxInfo, PCERT_PUBLIC_KEY_INFO pInfo, DWORD *pcbInfo)
{
    BOOL ret;
    HCRYPTKEY key;
    static CHAR rsa_oid[] = szOID_RSA_RSA;

    TRACE_(crypt)("(%08Ix, %ld, %08lx, %s, %08lx, %p, %p, %ld)\n", hCryptProv,
     dwKeySpec, dwCertEncodingType, debugstr_a(pszPublicKeyObjId), dwFlags,
     pvAuxInfo, pInfo, pInfo ? *pcbInfo : 0);

    if ((ret = CryptGetUserKey(hCryptProv, dwKeySpec, &key)))
    {
        DWORD keySize = 0;

        ret = CryptExportKey(key, 0, PUBLICKEYBLOB, 0, NULL, &keySize);
        if (ret)
        {
            PUBLICKEYSTRUC *pubKey = CryptMemAlloc(keySize);

            if (pubKey)
            {
                ret = CryptExportKey(key, 0, PUBLICKEYBLOB, 0, (BYTE *)pubKey, &keySize);
                if (ret)
                {
                    DWORD encodedLen;

                    if (!pszPublicKeyObjId)
                    {
                        static HCRYPTOIDFUNCSET set;
                        EncodePublicKeyAndParametersFunc encodeFunc = NULL;
                        HCRYPTOIDFUNCADDR hFunc = NULL;

                        pszPublicKeyObjId = (LPSTR)CertAlgIdToOID(pubKey->aiKeyAlg);
                        TRACE("public key algid %#x (%s)\n", pubKey->aiKeyAlg, debugstr_a(pszPublicKeyObjId));

                        if (!set) /* FIXME: there is no a public macro */
                            set = CryptInitOIDFunctionSet("CryptDllEncodePublicKeyAndParameters", 0);

                        CryptGetOIDFunctionAddress(set, dwCertEncodingType, pszPublicKeyObjId, 0, (void **)&encodeFunc, &hFunc);
                        if (encodeFunc)
                        {
                            BYTE *key_data = NULL;
                            DWORD key_size = 0;
                            BYTE *params = NULL;
                            DWORD params_size = 0;

                            ret = encodeFunc(dwCertEncodingType, pszPublicKeyObjId, (BYTE *)pubKey, keySize,
                                             dwFlags, pvAuxInfo, &key_data, &key_size, &params, &params_size);
                            if (ret)
                            {
                                DWORD oid_size = strlen(pszPublicKeyObjId) + 1;
                                DWORD size_needed = sizeof(*pInfo) + oid_size + key_size + params_size;

                                if (!pInfo)
                                    *pcbInfo = size_needed;
                                else if (*pcbInfo < size_needed)
                                {
                                    *pcbInfo = size_needed;
                                    SetLastError(ERROR_MORE_DATA);
                                    ret = FALSE;
                                }
                                else
                                {
                                    *pcbInfo = size_needed;
                                    pInfo->Algorithm.pszObjId = (char *)(pInfo + 1);
                                    lstrcpyA(pInfo->Algorithm.pszObjId, pszPublicKeyObjId);
                                    if (params)
                                    {
                                        pInfo->Algorithm.Parameters.cbData = params_size;
                                        pInfo->Algorithm.Parameters.pbData = (BYTE *)pInfo->Algorithm.pszObjId + oid_size;
                                        memcpy(pInfo->Algorithm.Parameters.pbData, params, params_size);
                                    }
                                    else
                                    {
                                        pInfo->Algorithm.Parameters.cbData = 0;
                                        pInfo->Algorithm.Parameters.pbData = NULL;
                                    }
                                    pInfo->PublicKey.pbData = (BYTE *)pInfo->Algorithm.pszObjId + oid_size + params_size;
                                    pInfo->PublicKey.cbData = key_size;
                                    memcpy(pInfo->PublicKey.pbData, key_data, key_size);
                                    pInfo->PublicKey.cUnusedBits = 0;
                                }

                                CryptMemFree(key_data);
                                CryptMemFree(params);
                            }

                            CryptMemFree(pubKey);
                            CryptFreeOIDFunctionAddress(hFunc, 0);
                            return ret;
                        }

                        /* fallback to RSA */
                        pszPublicKeyObjId = rsa_oid;
                    }

                    encodedLen = 0;
                    ret = CryptEncodeObject(dwCertEncodingType,
                     RSA_CSP_PUBLICKEYBLOB, pubKey, NULL, &encodedLen);
                    if (ret)
                    {
                        DWORD sizeNeeded = sizeof(CERT_PUBLIC_KEY_INFO) +
                         strlen(pszPublicKeyObjId) + 1 + encodedLen;

                        if (!pInfo)
                            *pcbInfo = sizeNeeded;
                        else if (*pcbInfo < sizeNeeded)
                        {
                            SetLastError(ERROR_MORE_DATA);
                            *pcbInfo = sizeNeeded;
                            ret = FALSE;
                        }
                        else
                        {
                            *pcbInfo = sizeNeeded;
                            pInfo->Algorithm.pszObjId = (char *)pInfo +
                             sizeof(CERT_PUBLIC_KEY_INFO);
                            lstrcpyA(pInfo->Algorithm.pszObjId,
                             pszPublicKeyObjId);
                            pInfo->Algorithm.Parameters.cbData = 0;
                            pInfo->Algorithm.Parameters.pbData = NULL;
                            pInfo->PublicKey.pbData =
                             (BYTE *)pInfo->Algorithm.pszObjId
                             + lstrlenA(pInfo->Algorithm.pszObjId) + 1;
                            pInfo->PublicKey.cbData = encodedLen;
                            pInfo->PublicKey.cUnusedBits = 0;
                            ret = CryptEncodeObject(dwCertEncodingType,
                             RSA_CSP_PUBLICKEYBLOB, pubKey,
                             pInfo->PublicKey.pbData, &pInfo->PublicKey.cbData);
                        }
                    }
                }
                CryptMemFree(pubKey);
            }
            else
                ret = FALSE;
        }
        CryptDestroyKey(key);
    }
    return ret;
}

typedef BOOL (WINAPI *ExportPublicKeyInfoExFunc)(HCRYPTPROV_OR_NCRYPT_KEY_HANDLE hCryptProv,
 DWORD dwKeySpec, DWORD dwCertEncodingType, LPSTR pszPublicKeyObjId,
 DWORD dwFlags, void *pvAuxInfo, PCERT_PUBLIC_KEY_INFO pInfo, DWORD *pcbInfo);

BOOL WINAPI CryptExportPublicKeyInfoEx(HCRYPTPROV_OR_NCRYPT_KEY_HANDLE hCryptProv, DWORD dwKeySpec,
 DWORD dwCertEncodingType, LPSTR pszPublicKeyObjId, DWORD dwFlags,
 void *pvAuxInfo, PCERT_PUBLIC_KEY_INFO pInfo, DWORD *pcbInfo)
{
    static HCRYPTOIDFUNCSET set = NULL;
    BOOL ret;
    ExportPublicKeyInfoExFunc exportFunc = NULL;
    HCRYPTOIDFUNCADDR hFunc = NULL;

    TRACE_(crypt)("(%08Ix, %ld, %08lx, %s, %08lx, %p, %p, %ld)\n", hCryptProv,
     dwKeySpec, dwCertEncodingType, debugstr_a(pszPublicKeyObjId), dwFlags,
     pvAuxInfo, pInfo, pInfo ? *pcbInfo : 0);

    if (!hCryptProv)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (pszPublicKeyObjId)
    {
        if (!set)
            set = CryptInitOIDFunctionSet(CRYPT_OID_EXPORT_PUBLIC_KEY_INFO_FUNC,
             0);
        CryptGetOIDFunctionAddress(set, dwCertEncodingType, pszPublicKeyObjId,
         0, (void **)&exportFunc, &hFunc);
    }
    if (!exportFunc)
        exportFunc = CRYPT_ExportPublicKeyInfoEx;
    ret = exportFunc(hCryptProv, dwKeySpec, dwCertEncodingType,
     pszPublicKeyObjId, dwFlags, pvAuxInfo, pInfo, pcbInfo);
    if (hFunc)
        CryptFreeOIDFunctionAddress(hFunc, 0);
    return ret;
}

BOOL WINAPI CryptImportPublicKeyInfo(HCRYPTPROV hCryptProv,
 DWORD dwCertEncodingType, PCERT_PUBLIC_KEY_INFO pInfo, HCRYPTKEY *phKey)
{
    return CryptImportPublicKeyInfoEx(hCryptProv, dwCertEncodingType, pInfo,
     0, 0, NULL, phKey);
}

typedef BOOL (WINAPI *ConvertPublicKeyInfoFunc)(DWORD dwCertEncodingType,
 PCERT_PUBLIC_KEY_INFO pInfo, ALG_ID aiKeyAlg, DWORD dwFlags,
 BYTE **ppbData, DWORD *dwDataLen);

static BOOL WINAPI CRYPT_ImportPublicKeyInfoEx(HCRYPTPROV hCryptProv,
 DWORD dwCertEncodingType, PCERT_PUBLIC_KEY_INFO pInfo, ALG_ID aiKeyAlg,
 DWORD dwFlags, void *pvAuxInfo, HCRYPTKEY *phKey)
{
    static HCRYPTOIDFUNCSET set = NULL;
    ConvertPublicKeyInfoFunc convertFunc = NULL;
    HCRYPTOIDFUNCADDR hFunc = NULL;
    BOOL ret;
    DWORD pubKeySize;
    LPBYTE pubKey;

    TRACE_(crypt)("(%08Ix, %08lx, %p, %08x, %08lx, %p, %p)\n", hCryptProv,
     dwCertEncodingType, pInfo, aiKeyAlg, dwFlags, pvAuxInfo, phKey);

    if (!set)
        set = CryptInitOIDFunctionSet(CRYPT_OID_CONVERT_PUBLIC_KEY_INFO_FUNC, 0);
    CryptGetOIDFunctionAddress(set, dwCertEncodingType, pInfo->Algorithm.pszObjId,
                               0, (void **)&convertFunc, &hFunc);
    if (convertFunc)
    {
        pubKey = NULL;
        pubKeySize = 0;
        ret = convertFunc(dwCertEncodingType, pInfo, aiKeyAlg, dwFlags, &pubKey, &pubKeySize);
        if (ret)
        {
            ret = CryptImportKey(hCryptProv, pubKey, pubKeySize, 0, 0, phKey);
            CryptMemFree(pubKey);
        }

        CryptFreeOIDFunctionAddress(hFunc, 0);
        return ret;
    }

    ret = CryptDecodeObject(dwCertEncodingType, RSA_CSP_PUBLICKEYBLOB,
     pInfo->PublicKey.pbData, pInfo->PublicKey.cbData, 0, NULL, &pubKeySize);
    if (ret)
    {
        pubKey = CryptMemAlloc(pubKeySize);

        if (pubKey)
        {
            ret = CryptDecodeObject(dwCertEncodingType, RSA_CSP_PUBLICKEYBLOB,
             pInfo->PublicKey.pbData, pInfo->PublicKey.cbData, 0, pubKey,
             &pubKeySize);
            if (ret)
            {
                if(aiKeyAlg)
                  ((BLOBHEADER*)pubKey)->aiKeyAlg = aiKeyAlg;
                ret = CryptImportKey(hCryptProv, pubKey, pubKeySize, 0, 0,
                 phKey);
            }
            CryptMemFree(pubKey);
        }
        else
            ret = FALSE;
    }
    return ret;
}

typedef BOOL (WINAPI *ImportPublicKeyInfoExFunc)(HCRYPTPROV hCryptProv,
 DWORD dwCertEncodingType, PCERT_PUBLIC_KEY_INFO pInfo, ALG_ID aiKeyAlg,
 DWORD dwFlags, void *pvAuxInfo, HCRYPTKEY *phKey);

BOOL WINAPI CryptImportPublicKeyInfoEx(HCRYPTPROV hCryptProv,
 DWORD dwCertEncodingType, PCERT_PUBLIC_KEY_INFO pInfo, ALG_ID aiKeyAlg,
 DWORD dwFlags, void *pvAuxInfo, HCRYPTKEY *phKey)
{
    static HCRYPTOIDFUNCSET set = NULL;
    BOOL ret;
    ImportPublicKeyInfoExFunc importFunc = NULL;
    HCRYPTOIDFUNCADDR hFunc = NULL;

    TRACE_(crypt)("(%08Ix, %08lx, %p, %08x, %08lx, %p, %p)\n", hCryptProv,
     dwCertEncodingType, pInfo, aiKeyAlg, dwFlags, pvAuxInfo, phKey);

    if (!set)
        set = CryptInitOIDFunctionSet(CRYPT_OID_IMPORT_PUBLIC_KEY_INFO_FUNC, 0);
    CryptGetOIDFunctionAddress(set, dwCertEncodingType,
     pInfo->Algorithm.pszObjId, 0, (void **)&importFunc, &hFunc);
    if (!importFunc)
        importFunc = CRYPT_ImportPublicKeyInfoEx;
    ret = importFunc(hCryptProv, dwCertEncodingType, pInfo, aiKeyAlg, dwFlags,
     pvAuxInfo, phKey);
    if (hFunc)
        CryptFreeOIDFunctionAddress(hFunc, 0);
    return ret;
}

BOOL WINAPI CryptImportPublicKeyInfoEx2(DWORD dwCertEncodingType,
 PCERT_PUBLIC_KEY_INFO pInfo, DWORD dwFlags, void *pvAuxInfo,
 BCRYPT_KEY_HANDLE *phKey)
{
    TRACE_(crypt)("(%ld, %p, %08lx, %p, %p)\n", dwCertEncodingType, pInfo,
     dwFlags, pvAuxInfo, phKey);

    if (dwFlags)
        FIXME("flags %#lx ignored\n", dwFlags);

    return CNG_ImportPubKey(pInfo, phKey);
}
