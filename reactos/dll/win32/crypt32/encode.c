/*
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This file implements ASN.1 DER encoding and decoding of a limited set of
 * types.  It isn't a full ASN.1 implementation.  Microsoft implements BER
 * encoding of many of the basic types in msasn1.dll, but that interface is
 * undocumented, so I implement them here.
 *
 * References:
 * "A Layman's Guide to a Subset of ASN.1, BER, and DER", by Burton Kaliski
 * (available online, look for a PDF copy as the HTML versions tend to have
 * translation errors.)
 *
 * RFC3280, http://www.faqs.org/rfcs/rfc3280.html
 *
 * MSDN, especially:
 * http://msdn.microsoft.com/library/en-us/seccrypto/security/constants_for_cryptencodeobject_and_cryptdecodeobject.asp
 */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "excpt.h"
#include "wincrypt.h"
#include "winreg.h"
#include "snmp.h"
#include "wine/debug.h"
#include "wine/exception.h"
#include "crypt32_private.h"

/* This is a bit arbitrary, but to set some limit: */
#define MAX_ENCODED_LEN 0x02000000

/* a few asn.1 tags we need */
#define ASN_BOOL            (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x01)
#define ASN_BITSTRING       (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x03)
#define ASN_ENUMERATED      (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x0a)
#define ASN_SETOF           (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x11)
#define ASN_NUMERICSTRING   (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x12)
#define ASN_PRINTABLESTRING (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x13)
#define ASN_IA5STRING       (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x16)
#define ASN_UTCTIME         (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x17)
#define ASN_GENERALTIME     (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x18)

#define ASN_FLAGS_MASK 0xe0
#define ASN_TYPE_MASK  0x1f

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

typedef BOOL (WINAPI *CryptEncodeObjectFunc)(DWORD, LPCSTR, const void *,
 BYTE *, DWORD *);
typedef BOOL (WINAPI *CryptEncodeObjectExFunc)(DWORD, LPCSTR, const void *,
 DWORD, PCRYPT_ENCODE_PARA, BYTE *, DWORD *);
typedef BOOL (WINAPI *CryptDecodeObjectFunc)(DWORD, LPCSTR, const BYTE *,
 DWORD, DWORD, void *, DWORD *);
typedef BOOL (WINAPI *CryptDecodeObjectExFunc)(DWORD, LPCSTR, const BYTE *,
 DWORD, DWORD, PCRYPT_DECODE_PARA, void *, DWORD *);

/* Prototypes for built-in encoders/decoders.  They follow the Ex style
 * prototypes.  The dwCertEncodingType and lpszStructType are ignored by the
 * built-in functions, but the parameters are retained to simplify
 * CryptEncodeObjectEx/CryptDecodeObjectEx, since they must call functions in
 * external DLLs that follow these signatures.
 * FIXME: some built-in functions are suitable to be called directly by
 * CryptEncodeObjectEx/CryptDecodeObjectEx (they implement exception handling
 * and memory allocation if requested), others are only suitable to be called
 * internally.  Comment which are which.
 */
static BOOL WINAPI CRYPT_AsnEncodeOid(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeExtensions(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeBool(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodePubKeyInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeOctets(DWORD dwCertEncodingType,
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

static BOOL WINAPI CRYPT_AsnDecodeChoiceOfTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo);
static BOOL WINAPI CRYPT_AsnDecodePubKeyInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo);
/* Like CRYPT_AsnDecodeExtensions, except assumes rgExtension is set ahead of
 * time, doesn't do memory allocation, and doesn't do exception handling.
 * (This isn't intended to be the externally-called one.)
 */
static BOOL WINAPI CRYPT_AsnDecodeExtensionsInternal(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo);
static BOOL WINAPI CRYPT_AsnDecodeOid(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, LPSTR pszObjId, DWORD *pcbObjId);
/* Assumes algo->Parameters.pbData is set ahead of time.  Internal func. */
static BOOL WINAPI CRYPT_AsnDecodeAlgorithmId(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo);
/* Internal function */
static BOOL WINAPI CRYPT_AsnDecodeBool(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo);
/* Assumes the CRYPT_DATA_BLOB's pbData member has been initialized */
static BOOL WINAPI CRYPT_AsnDecodeOctetsInternal(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo);
/* Like CRYPT_AsnDecodeBits, but assumes the CRYPT_INTEGER_BLOB's pbData
 * member has been initialized, doesn't do exception handling, and doesn't do
 * memory allocation.
 */
static BOOL WINAPI CRYPT_AsnDecodeBitsInternal(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo);
static BOOL WINAPI CRYPT_AsnDecodeBits(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo);
static BOOL WINAPI CRYPT_AsnDecodeInt(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo);
/* Like CRYPT_AsnDecodeInteger, but assumes the CRYPT_INTEGER_BLOB's pbData
 * member has been initialized, doesn't do exception handling, and doesn't do
 * memory allocation.
 */
static BOOL WINAPI CRYPT_AsnDecodeIntegerInternal(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo);
/* Like CRYPT_AsnDecodeInteger, but unsigned.  */
static BOOL WINAPI CRYPT_AsnDecodeUnsignedIntegerInternal(
 DWORD dwCertEncodingType, LPCSTR lpszStructType, const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, PCRYPT_DECODE_PARA pDecodePara,
 void *pvStructInfo, DWORD *pcbStructInfo);

BOOL WINAPI CryptEncodeObject(DWORD dwCertEncodingType, LPCSTR lpszStructType,
 const void *pvStructInfo, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    static HCRYPTOIDFUNCSET set = NULL;
    BOOL ret = FALSE;
    HCRYPTOIDFUNCADDR hFunc;
    CryptEncodeObjectFunc pCryptEncodeObject;

    TRACE("(0x%08lx, %s, %p, %p, %p)\n", dwCertEncodingType,
     debugstr_a(lpszStructType), pvStructInfo, pbEncoded,
     pcbEncoded);

    if (!pbEncoded && !pcbEncoded)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Try registered DLL first.. */
    if (!set)
        set = CryptInitOIDFunctionSet(CRYPT_OID_ENCODE_OBJECT_FUNC, 0);
    CryptGetOIDFunctionAddress(set, dwCertEncodingType, lpszStructType, 0,
     (void **)&pCryptEncodeObject, &hFunc);
    if (pCryptEncodeObject)
    {
        ret = pCryptEncodeObject(dwCertEncodingType, lpszStructType,
         pvStructInfo, pbEncoded, pcbEncoded);
        CryptFreeOIDFunctionAddress(hFunc, 0);
    }
    else
    {
        /* If not, use CryptEncodeObjectEx */
        ret = CryptEncodeObjectEx(dwCertEncodingType, lpszStructType,
         pvStructInfo, 0, NULL, pbEncoded, pcbEncoded);
    }
    return ret;
}

/* Helper function to check *pcbEncoded, set it to the required size, and
 * optionally to allocate memory.  Assumes pbEncoded is not NULL.
 * If CRYPT_ENCODE_ALLOC_FLAG is set in dwFlags, *pbEncoded will be set to a
 * pointer to the newly allocated memory.
 */
static BOOL CRYPT_EncodeEnsureSpace(DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded,
 DWORD bytesNeeded)
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
    return ret;
}

static BOOL CRYPT_EncodeLen(DWORD len, BYTE *pbEncoded, DWORD *pcbEncoded)
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

struct AsnEncodeSequenceItem
{
    const void             *pvStructInfo;
    CryptEncodeObjectExFunc encodeFunc;
    DWORD                   size; /* used during encoding, not for your use */
};

static BOOL WINAPI CRYPT_AsnEncodeSequence(DWORD dwCertEncodingType,
 struct AsnEncodeSequenceItem items[], DWORD cItem, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;
    DWORD i, dataLen = 0;

    TRACE("%p, %ld, %08lx, %p, %p, %ld\n", items, cItem, dwFlags, pEncodePara,
     pbEncoded, *pcbEncoded);
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
                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                *pbEncoded++ = ASN_SEQUENCE;
                CRYPT_EncodeLen(dataLen, pbEncoded, &lenBytes);
                pbEncoded += lenBytes;
                for (i = 0; ret && i < cItem; i++)
                {
                    ret = items[i].encodeFunc(dwCertEncodingType, NULL,
                     items[i].pvStructInfo, dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG,
                     NULL, pbEncoded, &items[i].size);
                    /* Some functions propagate their errors through the size */
                    if (!ret)
                        *pcbEncoded = items[i].size;
                    pbEncoded += items[i].size;
                }
            }
        }
    }
    TRACE("returning %d (%08lx)\n", ret, GetLastError());
    return ret;
}

struct AsnConstructedItem
{
    BYTE                    tag;
    const void             *pvStructInfo;
    CryptEncodeObjectExFunc encodeFunc;
};

static BOOL WINAPI CRYPT_AsnEncodeConstructed(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;
    const struct AsnConstructedItem *item =
     (const struct AsnConstructedItem *)pvStructInfo;
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
            if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                pbEncoded = *(BYTE **)pbEncoded;
            *pbEncoded++ = ASN_CONTEXT | ASN_CONSTRUCTOR | item->tag;
            CRYPT_EncodeLen(len, pbEncoded, &dataLen);
            pbEncoded += dataLen;
            ret = item->encodeFunc(dwCertEncodingType, lpszStructType,
             item->pvStructInfo, dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG, NULL,
             pbEncoded, &len);
            if (!ret)
            {
                /* Some functions propagate their errors through the size */
                *pcbEncoded = len;
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
    const struct AsnEncodeTagSwappedItem *item =
     (const struct AsnEncodeTagSwappedItem *)pvStructInfo;

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
    const DWORD *ver = (const DWORD *)pvStructInfo;
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
    const CRYPT_DER_BLOB *blob = (const CRYPT_DER_BLOB *)pvStructInfo;
    BOOL ret;

    if (!pbEncoded)
    {
        *pcbEncoded = blob->cbData;
        ret = TRUE;
    }
    else if (*pcbEncoded < blob->cbData)
    {
        *pcbEncoded = blob->cbData;
        SetLastError(ERROR_MORE_DATA);
        ret = FALSE;
    }
    else
    {
        if (blob->cbData)
            memcpy(pbEncoded, blob->pbData, blob->cbData);
        *pcbEncoded = blob->cbData;
        ret = TRUE;
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeValidity(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;
    /* This has two filetimes in a row, a NotBefore and a NotAfter */
    const FILETIME *timePtr = (const FILETIME *)pvStructInfo;
    struct AsnEncodeSequenceItem items[] = {
     { timePtr++, CRYPT_AsnEncodeChoiceOfTime, 0 },
     { timePtr,   CRYPT_AsnEncodeChoiceOfTime, 0 },
    };

    ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, 
     sizeof(items) / sizeof(items[0]), dwFlags, pEncodePara, pbEncoded,
     pcbEncoded);
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeAlgorithmId(
 DWORD dwCertEncodingType, LPCSTR lpszStructType, const void *pvStructInfo,
 DWORD dwFlags, PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    const CRYPT_ALGORITHM_IDENTIFIER *algo =
     (const CRYPT_ALGORITHM_IDENTIFIER *)pvStructInfo;
    BOOL ret;
    struct AsnEncodeSequenceItem items[] = {
     { algo->pszObjId,    CRYPT_AsnEncodeOid, 0 },
     { &algo->Parameters, CRYPT_CopyEncodedBlob, 0 },
    };

    ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items,
     sizeof(items) / sizeof(items[0]), dwFlags, pEncodePara, pbEncoded,
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
        const CERT_PUBLIC_KEY_INFO *info =
         (const CERT_PUBLIC_KEY_INFO *)pvStructInfo;
        struct AsnEncodeSequenceItem items[] = {
         { &info->Algorithm, CRYPT_AsnEncodeAlgorithmId, 0 },
         { &info->PublicKey, CRYPT_AsnEncodeBits, 0 },
        };

        TRACE("Encoding public key with OID %s\n",
         debugstr_a(info->Algorithm.pszObjId));
        ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items,
         sizeof(items) / sizeof(items[0]), dwFlags, pEncodePara, pbEncoded,
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
        const CERT_SIGNED_CONTENT_INFO *info =
         (const CERT_SIGNED_CONTENT_INFO *)pvStructInfo;
        struct AsnEncodeSequenceItem items[] = {
         { &info->ToBeSigned,         CRYPT_CopyEncodedBlob, 0 },
         { &info->SignatureAlgorithm, CRYPT_AsnEncodeAlgorithmId, 0 },
         { &info->Signature,          CRYPT_AsnEncodeBitsSwapBytes, 0 },
        };

        if (dwFlags & CRYPT_ENCODE_NO_SIGNATURE_BYTE_REVERSAL_FLAG)
            items[2].encodeFunc = CRYPT_AsnEncodeBits;
        ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, 
         sizeof(items) / sizeof(items[0]), dwFlags, pEncodePara, pbEncoded,
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
        const CERT_INFO *info = (const CERT_INFO *)pvStructInfo;
        struct AsnEncodeSequenceItem items[10] = {
         { &info->dwVersion,            CRYPT_AsnEncodeCertVersion, 0 },
         { &info->SerialNumber,         CRYPT_AsnEncodeInteger, 0 },
         { &info->SignatureAlgorithm,   CRYPT_AsnEncodeAlgorithmId, 0 },
         { &info->Issuer,               CRYPT_CopyEncodedBlob, 0 },
         { &info->NotBefore,            CRYPT_AsnEncodeValidity, 0 },
         { &info->Subject,              CRYPT_CopyEncodedBlob, 0 },
         { &info->SubjectPublicKeyInfo, CRYPT_AsnEncodePubKeyInfo, 0 },
         { 0 }
        };
        struct AsnConstructedItem constructed[3] = { { 0 } };
        DWORD cItem = 7, cConstructed = 0;

        if (info->IssuerUniqueId.cbData)
        {
            constructed[cConstructed].tag = 1;
            constructed[cConstructed].pvStructInfo = &info->IssuerUniqueId;
            constructed[cConstructed].encodeFunc = CRYPT_AsnEncodeBits;
            items[cItem].pvStructInfo = &constructed[cConstructed];
            items[cItem].encodeFunc = CRYPT_AsnEncodeConstructed;
            cConstructed++;
            cItem++;
        }
        if (info->SubjectUniqueId.cbData)
        {
            constructed[cConstructed].tag = 2;
            constructed[cConstructed].pvStructInfo = &info->SubjectUniqueId;
            constructed[cConstructed].encodeFunc = CRYPT_AsnEncodeBits;
            items[cItem].pvStructInfo = &constructed[cConstructed];
            items[cItem].encodeFunc = CRYPT_AsnEncodeConstructed;
            cConstructed++;
            cItem++;
        }
        if (info->cExtension)
        {
            constructed[cConstructed].tag = 3;
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

static BOOL WINAPI CRYPT_AsnEncodeCRLEntry(const CRL_ENTRY *entry,
 BYTE *pbEncoded, DWORD *pcbEncoded)
{
    struct AsnEncodeSequenceItem items[3] = {
     { &entry->SerialNumber,   CRYPT_AsnEncodeInteger, 0 },
     { &entry->RevocationDate, CRYPT_AsnEncodeChoiceOfTime, 0 },
     { 0 }
    };
    DWORD cItem = 2;
    BOOL ret;

    TRACE("%p, %p, %p\n", entry, pbEncoded, pcbEncoded);

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
    DWORD cCRLEntry = *(const DWORD *)pvStructInfo;
    DWORD bytesNeeded, dataLen, lenBytes, i;
    const CRL_ENTRY *rgCRLEntry = *(const CRL_ENTRY **)
     ((const BYTE *)pvStructInfo + sizeof(DWORD));
    BOOL ret = TRUE;

    for (i = 0, dataLen = 0; ret && i < cCRLEntry; i++)
    {
        DWORD size;

        ret = CRYPT_AsnEncodeCRLEntry(&rgCRLEntry[i], NULL, &size);
        if (ret)
            dataLen += size;
    }
    CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
    bytesNeeded = 1 + lenBytes + dataLen;
    if (!pbEncoded)
        *pcbEncoded = bytesNeeded;
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
            for (i = 0; i < cCRLEntry; i++)
            {
                DWORD size = dataLen;

                ret = CRYPT_AsnEncodeCRLEntry(&rgCRLEntry[i], pbEncoded, &size);
                pbEncoded += size;
                dataLen -= size;
            }
        }
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeCRLVersion(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    const DWORD *ver = (const DWORD *)pvStructInfo;
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
        const CRL_INFO *info = (const CRL_INFO *)pvStructInfo;
        struct AsnEncodeSequenceItem items[7] = {
         { &info->dwVersion,          CRYPT_AsnEncodeCRLVersion, 0 },
         { &info->SignatureAlgorithm, CRYPT_AsnEncodeAlgorithmId, 0 },
         { &info->Issuer,             CRYPT_CopyEncodedBlob, 0 },
         { &info->ThisUpdate,         CRYPT_AsnEncodeChoiceOfTime, 0 },
         { 0 }
        };
        DWORD cItem = 4;

        if (info->NextUpdate.dwLowDateTime || info->NextUpdate.dwHighDateTime)
        {
            items[cItem].pvStructInfo = &info->NextUpdate;
            items[cItem].encodeFunc = CRYPT_AsnEncodeChoiceOfTime;
            cItem++;
        }
        if (info->cCRLEntry)
        {
            items[cItem].pvStructInfo = &info->cCRLEntry;
            items[cItem].encodeFunc = CRYPT_AsnEncodeCRLEntries;
            cItem++;
        }
        if (info->cExtension)
        {
            items[cItem].pvStructInfo = &info->cExtension;
            items[cItem].encodeFunc = CRYPT_AsnEncodeExtensions;
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

    TRACE("%p, %p, %ld\n", ext, pbEncoded, *pcbEncoded);

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
        const CERT_EXTENSIONS *exts = (const CERT_EXTENSIONS *)pvStructInfo;

        ret = TRUE;
        for (i = 0, dataLen = 0; ret && i < exts->cExtension; i++)
        {
            DWORD size;

            ret = CRYPT_AsnEncodeExtension(&exts->rgExtension[i], NULL, &size);
            if (ret)
                dataLen += size;
        }
        CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
        bytesNeeded = 1 + lenBytes + dataLen;
        if (!pbEncoded)
            *pcbEncoded = bytesNeeded;
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
                for (i = 0; i < exts->cExtension; i++)
                {
                    DWORD size = dataLen;

                    ret = CRYPT_AsnEncodeExtension(&exts->rgExtension[i],
                     pbEncoded, &size);
                    pbEncoded += size;
                    dataLen -= size;
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

static BOOL WINAPI CRYPT_AsnEncodeOid(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    LPCSTR pszObjId = (LPCSTR)pvStructInfo;
    DWORD bytesNeeded = 0, lenBytes;
    BOOL ret = TRUE;
    int firstPos = 0;
    BYTE firstByte = 0;

    TRACE("%s\n", debugstr_a(pszObjId));

    if (pszObjId)
    {
        const char *ptr;
        int val1, val2;

        if (sscanf(pszObjId, "%d.%d.%n", &val1, &val2, &firstPos) != 2)
        {
            SetLastError(CRYPT_E_ASN1_ERROR);
            return FALSE;
        }
        bytesNeeded++;
        firstByte = val1 * 40 + val2;
        ptr = pszObjId + firstPos;
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

static BOOL WINAPI CRYPT_AsnEncodeNameValue(DWORD dwCertEncodingType,
 CERT_NAME_VALUE *value, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BYTE tag;
    DWORD bytesNeeded, lenBytes, encodedLen;
    BOOL ret = TRUE;

    switch (value->dwValueType)
    {
    case CERT_RDN_NUMERIC_STRING:
        tag = ASN_NUMERICSTRING;
        encodedLen = value->Value.cbData;
        break;
    case CERT_RDN_PRINTABLE_STRING:
        tag = ASN_PRINTABLESTRING;
        encodedLen = value->Value.cbData;
        break;
    case CERT_RDN_IA5_STRING:
        tag = ASN_IA5STRING;
        encodedLen = value->Value.cbData;
        break;
    case CERT_RDN_ANY_TYPE:
        /* explicitly disallowed */
        SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
        return FALSE;
    default:
        FIXME("String type %ld unimplemented\n", value->dwValueType);
        return FALSE;
    }
    CRYPT_EncodeLen(encodedLen, NULL, &lenBytes);
    bytesNeeded = 1 + lenBytes + encodedLen;
    if (pbEncoded)
    {
        if (*pcbEncoded < bytesNeeded)
        {
            SetLastError(ERROR_MORE_DATA);
            ret = FALSE;
        }
        else
        {
            *pbEncoded++ = tag;
            CRYPT_EncodeLen(encodedLen, pbEncoded, &lenBytes);
            pbEncoded += lenBytes;
            switch (value->dwValueType)
            {
            case CERT_RDN_NUMERIC_STRING:
            case CERT_RDN_PRINTABLE_STRING:
            case CERT_RDN_IA5_STRING:
                memcpy(pbEncoded, value->Value.pbData, value->Value.cbData);
            }
        }
    }
    *pcbEncoded = bytesNeeded;
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeRdnAttr(DWORD dwCertEncodingType,
 CERT_RDN_ATTR *attr, BYTE *pbEncoded, DWORD *pcbEncoded)
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
        ret = CRYPT_AsnEncodeNameValue(dwCertEncodingType,
         (CERT_NAME_VALUE *)&attr->dwValueType, NULL, &size);
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
                        ret = CRYPT_AsnEncodeNameValue(dwCertEncodingType,
                         (CERT_NAME_VALUE *)&attr->dwValueType, pbEncoded,
                         &size);
                    }
                }
            }
            *pcbEncoded = bytesNeeded;
        }
    }
    return ret;
}

static int BLOBComp(const void *l, const void *r)
{
    CRYPT_DER_BLOB *a = (CRYPT_DER_BLOB *)l, *b = (CRYPT_DER_BLOB *)r;
    int ret;

    if (!(ret = memcmp(a->pbData, b->pbData, min(a->cbData, b->cbData))))
        ret = a->cbData - b->cbData;
    return ret;
}

/* This encodes as a SET OF, which in DER must be lexicographically sorted.
 */
static BOOL WINAPI CRYPT_AsnEncodeRdn(DWORD dwCertEncodingType, CERT_RDN *rdn,
 BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;
    CRYPT_DER_BLOB *blobs = NULL;

    __TRY
    {
        DWORD bytesNeeded = 0, lenBytes, i;

        blobs = NULL;
        ret = TRUE;
        if (rdn->cRDNAttr)
        {
            blobs = CryptMemAlloc(rdn->cRDNAttr * sizeof(CRYPT_DER_BLOB));
            if (!blobs)
                ret = FALSE;
            else
                memset(blobs, 0, rdn->cRDNAttr * sizeof(CRYPT_DER_BLOB));
        }
        for (i = 0; ret && i < rdn->cRDNAttr; i++)
        {
            ret = CRYPT_AsnEncodeRdnAttr(dwCertEncodingType, &rdn->rgRDNAttr[i],
             NULL, &blobs[i].cbData);
            if (ret)
                bytesNeeded += blobs[i].cbData;
        }
        if (ret)
        {
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
                    for (i = 0; ret && i < rdn->cRDNAttr; i++)
                    {
                        blobs[i].pbData = CryptMemAlloc(blobs[i].cbData);
                        if (!blobs[i].pbData)
                            ret = FALSE;
                        else
                            ret = CRYPT_AsnEncodeRdnAttr(dwCertEncodingType,
                             &rdn->rgRDNAttr[i], blobs[i].pbData,
                             &blobs[i].cbData);
                    }
                    if (ret)
                    {
                        qsort(blobs, rdn->cRDNAttr, sizeof(CRYPT_DER_BLOB),
                         BLOBComp);
                        *pbEncoded++ = ASN_CONSTRUCTOR | ASN_SETOF;
                        CRYPT_EncodeLen(bytesNeeded - lenBytes - 1, pbEncoded,
                         &lenBytes);
                        pbEncoded += lenBytes;
                        for (i = 0; ret && i < rdn->cRDNAttr; i++)
                        {
                            memcpy(pbEncoded, blobs[i].pbData, blobs[i].cbData);
                            pbEncoded += blobs[i].cbData;
                        }
                    }
                }
            }
            *pcbEncoded = bytesNeeded;
        }
        if (blobs)
        {
            for (i = 0; i < rdn->cRDNAttr; i++)
                CryptMemFree(blobs[i].pbData);
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    CryptMemFree(blobs);
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeName(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CERT_NAME_INFO *info = (const CERT_NAME_INFO *)pvStructInfo;
        DWORD bytesNeeded = 0, lenBytes, size, i;

        TRACE("encoding name with %ld RDNs\n", info->cRDN);
        ret = TRUE;
        for (i = 0; ret && i < info->cRDN; i++)
        {
            ret = CRYPT_AsnEncodeRdn(dwCertEncodingType, &info->rgRDN[i], NULL,
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
                    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                        pbEncoded = *(BYTE **)pbEncoded;
                    *pbEncoded++ = ASN_SEQUENCEOF;
                    CRYPT_EncodeLen(bytesNeeded - lenBytes - 1, pbEncoded,
                     &lenBytes);
                    pbEncoded += lenBytes;
                    for (i = 0; ret && i < info->cRDN; i++)
                    {
                        size = bytesNeeded;
                        ret = CRYPT_AsnEncodeRdn(dwCertEncodingType,
                         &info->rgRDN[i], pbEncoded, &size);
                        if (ret)
                        {
                            pbEncoded += size;
                            bytesNeeded -= size;
                        }
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

static BOOL CRYPT_AsnEncodeAltNameEntry(const CERT_ALT_NAME_ENTRY *entry,
 BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;
    DWORD dataLen;

    ret = TRUE;
    switch (entry->dwAltNameChoice)
    {
    case CERT_ALT_NAME_RFC822_NAME:
    case CERT_ALT_NAME_DNS_NAME:
    case CERT_ALT_NAME_URL:
        if (entry->u.pwszURL)
        {
            DWORD i;

            /* Not + 1: don't encode the NULL-terminator */
            dataLen = lstrlenW(entry->u.pwszURL);
            for (i = 0; ret && i < dataLen; i++)
            {
                if (entry->u.pwszURL[i] > 0x7f)
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
    case CERT_ALT_NAME_IP_ADDRESS:
        dataLen = entry->u.IPAddress.cbData;
        break;
    case CERT_ALT_NAME_REGISTERED_ID:
        /* FIXME: encode OID */
    case CERT_ALT_NAME_OTHER_NAME:
    case CERT_ALT_NAME_DIRECTORY_NAME:
        FIXME("name type %ld unimplemented\n", entry->dwAltNameChoice);
        return FALSE;
    default:
        SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
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
            *pbEncoded++ = ASN_CONTEXT | (entry->dwAltNameChoice - 1);
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
                    *pbEncoded++ = (BYTE)entry->u.pwszURL[i];
                break;
            }
            case CERT_ALT_NAME_IP_ADDRESS:
                memcpy(pbEncoded, entry->u.IPAddress.pbData, dataLen);
                break;
            }
            if (ret)
                *pcbEncoded = bytesNeeded;
        }
    }
    TRACE("returning %d (%08lx)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeAltName(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CERT_ALT_NAME_INFO *info =
         (const CERT_ALT_NAME_INFO *)pvStructInfo;
        DWORD bytesNeeded, dataLen, lenBytes, i;

        ret = TRUE;
        /* FIXME: should check that cAltEntry is not bigger than 0xff, since we
         * can't encode an erroneous entry index if it's bigger than this.
         */
        for (i = 0, dataLen = 0; ret && i < info->cAltEntry; i++)
        {
            DWORD len;

            ret = CRYPT_AsnEncodeAltNameEntry(&info->rgAltEntry[i], NULL,
             &len);
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
                    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                        pbEncoded = *(BYTE **)pbEncoded;
                    *pbEncoded++ = ASN_SEQUENCEOF;
                    CRYPT_EncodeLen(dataLen, pbEncoded, &lenBytes);
                    pbEncoded += lenBytes;
                    for (i = 0; ret && i < info->cAltEntry; i++)
                    {
                        DWORD len = dataLen;

                        ret = CRYPT_AsnEncodeAltNameEntry(&info->rgAltEntry[i],
                         pbEncoded, &len);
                        if (ret)
                        {
                            pbEncoded += len;
                            dataLen -= len;
                        }
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

static BOOL WINAPI CRYPT_AsnEncodeBasicConstraints2(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CERT_BASIC_CONSTRAINTS2_INFO *info =
         (const CERT_BASIC_CONSTRAINTS2_INFO *)pvStructInfo;
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

static BOOL WINAPI CRYPT_AsnEncodeRsaPubKey(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const BLOBHEADER *hdr =
         (const BLOBHEADER *)pvStructInfo;

        if (hdr->bType != PUBLICKEYBLOB)
        {
            SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
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
             sizeof(items) / sizeof(items[0]), dwFlags, pEncodePara, pbEncoded,
             pcbEncoded);
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

static BOOL WINAPI CRYPT_AsnEncodeOctets(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CRYPT_DATA_BLOB *blob = (const CRYPT_DATA_BLOB *)pvStructInfo;
        DWORD bytesNeeded, lenBytes;

        TRACE("(%ld, %p), %08lx, %p, %p, %ld\n", blob->cbData, blob->pbData,
         dwFlags, pEncodePara, pbEncoded, *pcbEncoded);

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
        const CRYPT_BIT_BLOB *blob = (const CRYPT_BIT_BLOB *)pvStructInfo;
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
        const CRYPT_BIT_BLOB *blob = (const CRYPT_BIT_BLOB *)pvStructInfo;
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
        DWORD significantBytes, lenBytes;
        BYTE padByte = 0, bytesNeeded;
        BOOL pad = FALSE;
        const CRYPT_INTEGER_BLOB *blob =
         (const CRYPT_INTEGER_BLOB *)pvStructInfo;

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
        DWORD significantBytes, lenBytes;
        BYTE bytesNeeded;
        BOOL pad = FALSE;
        const CRYPT_INTEGER_BLOB *blob =
         (const CRYPT_INTEGER_BLOB *)pvStructInfo;

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
        /* sorry, magic number: enough for tag, len, YYMMDDHHMMSSZ\0.  I use a
         * temporary buffer because the output buffer is not NULL-terminated.
         */
        char buf[16];
        static const DWORD bytesNeeded = sizeof(buf) - 1;

        if (!pbEncoded)
        {
            *pcbEncoded = bytesNeeded;
            ret = TRUE;
        }
        else
        {
            /* Sanity check the year, this is a two-digit year format */
            ret = FileTimeToSystemTime((const FILETIME *)pvStructInfo,
             &sysTime);
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
                     sysTime.wDay, sysTime.wMonth, sysTime.wHour,
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

static BOOL WINAPI CRYPT_AsnEncodeGeneralizedTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        SYSTEMTIME sysTime;
        /* sorry, magic number: enough for tag, len, YYYYMMDDHHMMSSZ\0.  I use a
         * temporary buffer because the output buffer is not NULL-terminated.
         */
        char buf[18];
        static const DWORD bytesNeeded = sizeof(buf) - 1;

        if (!pbEncoded)
        {
            *pcbEncoded = bytesNeeded;
            ret = TRUE;
        }
        else
        {
            ret = FileTimeToSystemTime((const FILETIME *)pvStructInfo,
             &sysTime);
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
                 sysTime.wYear, sysTime.wDay, sysTime.wMonth, sysTime.wHour,
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
        if (!FileTimeToSystemTime((const FILETIME *)pvStructInfo, &sysTime))
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
        const CRYPT_SEQUENCE_OF_ANY *seq =
         (const CRYPT_SEQUENCE_OF_ANY *)pvStructInfo;

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
        swapped[cSwapped].pvStructInfo = &distPoint->DistPointName.u.FullName;
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
        const CRL_DIST_POINTS_INFO *info =
         (const CRL_DIST_POINTS_INFO *)pvStructInfo;

        if (!info->cDistPoint)
        {
            SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
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
                        if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                            pbEncoded = *(BYTE **)pbEncoded;
                        *pbEncoded++ = ASN_SEQUENCEOF;
                        CRYPT_EncodeLen(dataLen, pbEncoded, &lenBytes);
                        pbEncoded += lenBytes;
                        for (i = 0; ret && i < info->cDistPoint; i++)
                        {
                            DWORD len = dataLen;

                            ret = CRYPT_AsnEncodeDistPoint(
                             &info->rgDistPoint[i], pbEncoded, &len);
                            if (ret)
                            {
                                pbEncoded += len;
                                dataLen -= len;
                            }
                        }
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

BOOL WINAPI CryptEncodeObjectEx(DWORD dwCertEncodingType, LPCSTR lpszStructType,
 const void *pvStructInfo, DWORD dwFlags, PCRYPT_ENCODE_PARA pEncodePara,
 void *pvEncoded, DWORD *pcbEncoded)
{
    static HCRYPTOIDFUNCSET set = NULL;
    BOOL ret = FALSE;
    CryptEncodeObjectExFunc encodeFunc = NULL;
    HCRYPTOIDFUNCADDR hFunc = NULL;

    TRACE("(0x%08lx, %s, %p, 0x%08lx, %p, %p, %p)\n", dwCertEncodingType,
     debugstr_a(lpszStructType), pvStructInfo, dwFlags, pEncodePara,
     pvEncoded, pcbEncoded);

    if (!pvEncoded && !pcbEncoded)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if ((dwCertEncodingType & CERT_ENCODING_TYPE_MASK) != X509_ASN_ENCODING
     && (dwCertEncodingType & CMSG_ENCODING_TYPE_MASK) != PKCS_7_ASN_ENCODING)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    SetLastError(NOERROR);
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG && pvEncoded)
        *(BYTE **)pvEncoded = NULL;
    if (!HIWORD(lpszStructType))
    {
        switch (LOWORD(lpszStructType))
        {
        case (WORD)X509_CERT:
            encodeFunc = CRYPT_AsnEncodeCert;
            break;
        case (WORD)X509_CERT_TO_BE_SIGNED:
            encodeFunc = CRYPT_AsnEncodeCertInfo;
            break;
        case (WORD)X509_CERT_CRL_TO_BE_SIGNED:
            encodeFunc = CRYPT_AsnEncodeCRLInfo;
            break;
        case (WORD)X509_EXTENSIONS:
            encodeFunc = CRYPT_AsnEncodeExtensions;
            break;
        case (WORD)X509_NAME:
            encodeFunc = CRYPT_AsnEncodeName;
            break;
        case (WORD)X509_PUBLIC_KEY_INFO:
            encodeFunc = CRYPT_AsnEncodePubKeyInfo;
            break;
        case (WORD)X509_ALTERNATE_NAME:
            encodeFunc = CRYPT_AsnEncodeAltName;
            break;
        case (WORD)X509_BASIC_CONSTRAINTS2:
            encodeFunc = CRYPT_AsnEncodeBasicConstraints2;
            break;
        case (WORD)RSA_CSP_PUBLICKEYBLOB:
            encodeFunc = CRYPT_AsnEncodeRsaPubKey;
            break;
        case (WORD)X509_OCTET_STRING:
            encodeFunc = CRYPT_AsnEncodeOctets;
            break;
        case (WORD)X509_BITS:
        case (WORD)X509_KEY_USAGE:
            encodeFunc = CRYPT_AsnEncodeBits;
            break;
        case (WORD)X509_INTEGER:
            encodeFunc = CRYPT_AsnEncodeInt;
            break;
        case (WORD)X509_MULTI_BYTE_INTEGER:
            encodeFunc = CRYPT_AsnEncodeInteger;
            break;
        case (WORD)X509_MULTI_BYTE_UINT:
            encodeFunc = CRYPT_AsnEncodeUnsignedInteger;
            break;
        case (WORD)X509_ENUMERATED:
            encodeFunc = CRYPT_AsnEncodeEnumerated;
            break;
        case (WORD)X509_CHOICE_OF_TIME:
            encodeFunc = CRYPT_AsnEncodeChoiceOfTime;
            break;
        case (WORD)X509_SEQUENCE_OF_ANY:
            encodeFunc = CRYPT_AsnEncodeSequenceOfAny;
            break;
        case (WORD)PKCS_UTC_TIME:
            encodeFunc = CRYPT_AsnEncodeUtcTime;
            break;
        case (WORD)X509_CRL_DIST_POINTS:
            encodeFunc = CRYPT_AsnEncodeCRLDistPoints;
            break;
        default:
            FIXME("%d: unimplemented\n", LOWORD(lpszStructType));
        }
    }
    else if (!strcmp(lpszStructType, szOID_CERT_EXTENSIONS))
        encodeFunc = CRYPT_AsnEncodeExtensions;
    else if (!strcmp(lpszStructType, szOID_RSA_signingTime))
        encodeFunc = CRYPT_AsnEncodeUtcTime;
    else if (!strcmp(lpszStructType, szOID_CRL_REASON_CODE))
        encodeFunc = CRYPT_AsnEncodeEnumerated;
    else if (!strcmp(lpszStructType, szOID_KEY_USAGE))
        encodeFunc = CRYPT_AsnEncodeBits;
    else if (!strcmp(lpszStructType, szOID_SUBJECT_KEY_IDENTIFIER))
        encodeFunc = CRYPT_AsnEncodeOctets;
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
    else
        TRACE("OID %s not found or unimplemented, looking for DLL\n",
         debugstr_a(lpszStructType));
    if (!encodeFunc)
    {
        if (!set)
            set = CryptInitOIDFunctionSet(CRYPT_OID_ENCODE_OBJECT_EX_FUNC, 0);
        CryptGetOIDFunctionAddress(set, dwCertEncodingType, lpszStructType, 0,
         (void **)&encodeFunc, &hFunc);
    }
    if (encodeFunc)
        ret = encodeFunc(dwCertEncodingType, lpszStructType, pvStructInfo,
         dwFlags, pEncodePara, pvEncoded, pcbEncoded);
    else
        SetLastError(ERROR_FILE_NOT_FOUND);
    if (hFunc)
        CryptFreeOIDFunctionAddress(hFunc, 0);
    return ret;
}

BOOL WINAPI CryptDecodeObject(DWORD dwCertEncodingType, LPCSTR lpszStructType,
 const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo,
 DWORD *pcbStructInfo)
{
    static HCRYPTOIDFUNCSET set = NULL;
    BOOL ret = FALSE;
    CryptDecodeObjectFunc pCryptDecodeObject;
    HCRYPTOIDFUNCADDR hFunc;

    TRACE("(0x%08lx, %s, %p, %ld, 0x%08lx, %p, %p)\n", dwCertEncodingType,
     debugstr_a(lpszStructType), pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, pcbStructInfo);

    if (!pvStructInfo && !pcbStructInfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Try registered DLL first.. */
    if (!set)
        set = CryptInitOIDFunctionSet(CRYPT_OID_DECODE_OBJECT_FUNC, 0);
    CryptGetOIDFunctionAddress(set, dwCertEncodingType, lpszStructType, 0,
     (void **)&pCryptDecodeObject, &hFunc);
    if (pCryptDecodeObject)
    {
        ret = pCryptDecodeObject(dwCertEncodingType, lpszStructType,
         pbEncoded, cbEncoded, dwFlags, pvStructInfo, pcbStructInfo);
        CryptFreeOIDFunctionAddress(hFunc, 0);
    }
    else
    {
        /* If not, use CryptDecodeObjectEx */
        ret = CryptDecodeObjectEx(dwCertEncodingType, lpszStructType, pbEncoded,
         cbEncoded, dwFlags, NULL, pvStructInfo, pcbStructInfo);
    }
    return ret;
}

/* Gets the number of length bytes from the given (leading) length byte */
#define GET_LEN_BYTES(b) ((b) <= 0x7f ? 1 : 1 + ((b) & 0x7f))

/* Helper function to get the encoded length of the data starting at pbEncoded,
 * where pbEncoded[0] is the tag.  If the data are too short to contain a
 * length or if the length is too large for cbEncoded, sets an appropriate
 * error code and returns FALSE.
 */
static BOOL WINAPI CRYPT_GetLen(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD *len)
{
    BOOL ret;

    if (cbEncoded <= 1)
    {
        SetLastError(CRYPT_E_ASN1_CORRUPT);
        ret = FALSE;
    }
    else if (pbEncoded[1] <= 0x7f)
    {
        if (pbEncoded[1] + 1 > cbEncoded)
        {
            SetLastError(CRYPT_E_ASN1_EOD);
            ret = FALSE;
        }
        else
        {
            *len = pbEncoded[1];
            ret = TRUE;
        }
    }
    else
    {
        BYTE lenLen = GET_LEN_BYTES(pbEncoded[1]);

        if (lenLen > sizeof(DWORD) + 1)
        {
            SetLastError(CRYPT_E_ASN1_LARGE);
            ret = FALSE;
        }
        else if (lenLen + 2 > cbEncoded)
        {
            SetLastError(CRYPT_E_ASN1_CORRUPT);
            ret = FALSE;
        }
        else
        {
            DWORD out = 0;

            pbEncoded += 2;
            while (--lenLen)
            {
                out <<= 8;
                out |= *pbEncoded++;
            }
            if (out + lenLen + 1 > cbEncoded)
            {
                SetLastError(CRYPT_E_ASN1_EOD);
                ret = FALSE;
            }
            else
            {
                *len = out;
                ret = TRUE;
            }
        }
    }
    return ret;
}

/* Helper function to check *pcbStructInfo, set it to the required size, and
 * optionally to allocate memory.  Assumes pvStructInfo is not NULL.
 * If CRYPT_DECODE_ALLOC_FLAG is set in dwFlags, *pvStructInfo will be set to a
 * pointer to the newly allocated memory.
 */
static BOOL CRYPT_DecodeEnsureSpace(DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD bytesNeeded)
{
    BOOL ret = TRUE;

    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
    {
        if (pDecodePara && pDecodePara->pfnAlloc)
            *(BYTE **)pvStructInfo = pDecodePara->pfnAlloc(bytesNeeded);
        else
            *(BYTE **)pvStructInfo = LocalAlloc(0, bytesNeeded);
        if (!*(BYTE **)pvStructInfo)
            ret = FALSE;
        else
            *pcbStructInfo = bytesNeeded;
    }
    else if (*pcbStructInfo < bytesNeeded)
    {
        *pcbStructInfo = bytesNeeded;
        SetLastError(ERROR_MORE_DATA);
        ret = FALSE;
    }
    return ret;
}

/* tag:
 *     The expected tag of the item.  If tag is 0, decodeFunc is called
 *     regardless of the tag value seen.
 * offset:
 *     A sequence is decoded into a struct.  The offset member is the
 *     offset of this item within that struct.
 * decodeFunc:
 *     The decoder function to use.  If this is NULL, then the member isn't
 *     decoded, but minSize space is reserved for it.
 * minSize:
 *     The minimum amount of space occupied after decoding.  You must set this.
 * optional:
 *     If true, and the tag doesn't match the expected tag for this item,
 *     or the decodeFunc fails with CRYPT_E_ASN1_BADTAG, then minSize space is
 *     filled with 0 for this member.
 * hasPointer, pointerOffset, minSize:
 *     If the item has dynamic data, set hasPointer to TRUE, pointerOffset to
 *     the offset within the (outer) struct of the data pointer (or to the
 *     first data pointer, if more than one exist).
 * size:
 *     Used by CRYPT_AsnDecodeSequence, not for your use.
 */
struct AsnDecodeSequenceItem
{
    BYTE                    tag;
    DWORD                   offset;
    CryptDecodeObjectExFunc decodeFunc;
    DWORD                   minSize;
    BOOL                    optional;
    BOOL                    hasPointer;
    DWORD                   pointerOffset;
    DWORD                   size;
};

static BOOL CRYPT_AsnDecodeSequenceItems(DWORD dwCertEncodingType,
 struct AsnDecodeSequenceItem items[], DWORD cItem, const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, BYTE *nextData)
{
    BOOL ret;
    DWORD i;
    const BYTE *ptr;

    ptr = pbEncoded + 1 + GET_LEN_BYTES(pbEncoded[1]);
    for (i = 0, ret = TRUE; ret && i < cItem; i++)
    {
        if (cbEncoded - (ptr - pbEncoded) != 0)
        {
            DWORD nextItemLen;

            if ((ret = CRYPT_GetLen(ptr, cbEncoded - (ptr - pbEncoded),
             &nextItemLen)))
            {
                BYTE nextItemLenBytes = GET_LEN_BYTES(ptr[1]);

                if (ptr[0] == items[i].tag || !items[i].tag)
                {
                    if (nextData && pvStructInfo && items[i].hasPointer)
                    {
                        TRACE("Setting next pointer to %p\n",
                         nextData);
                        *(BYTE **)((BYTE *)pvStructInfo +
                         items[i].pointerOffset) = nextData;
                    }
                    if (items[i].decodeFunc)
                    {
                        if (pvStructInfo)
                            TRACE("decoding item %ld\n", i);
                        else
                            TRACE("sizing item %ld\n", i);
                        ret = items[i].decodeFunc(dwCertEncodingType,
                         NULL, ptr, 1 + nextItemLenBytes + nextItemLen,
                         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL,
                         pvStructInfo ?  (BYTE *)pvStructInfo + items[i].offset
                         : NULL, &items[i].size);
                        if (ret)
                        {
                            if (nextData && items[i].hasPointer &&
                             items[i].size > items[i].minSize)
                            {
                                nextData += items[i].size - items[i].minSize;
                                /* align nextData to DWORD boundaries */
                                if (items[i].size % sizeof(DWORD))
                                    nextData += sizeof(DWORD) - items[i].size %
                                     sizeof(DWORD);
                            }
                            /* Account for alignment padding */
                            if (items[i].size % sizeof(DWORD))
                                items[i].size += sizeof(DWORD) -
                                 items[i].size % sizeof(DWORD);
                            ptr += 1 + nextItemLenBytes + nextItemLen;
                        }
                        else if (items[i].optional &&
                         GetLastError() == CRYPT_E_ASN1_BADTAG)
                        {
                            TRACE("skipping optional item %ld\n", i);
                            items[i].size = items[i].minSize;
                            SetLastError(NOERROR);
                            ret = TRUE;
                        }
                        else
                            TRACE("item %ld failed: %08lx\n", i,
                             GetLastError());
                    }
                    else
                        items[i].size = items[i].minSize;
                }
                else if (items[i].optional)
                {
                    TRACE("skipping optional item %ld\n", i);
                    items[i].size = items[i].minSize;
                }
                else
                {
                    TRACE("tag %02x doesn't match expected %02x\n",
                     ptr[0], items[i].tag);
                    SetLastError(CRYPT_E_ASN1_BADTAG);
                    ret = FALSE;
                }
            }
        }
        else if (items[i].optional)
        {
            TRACE("missing optional item %ld, skipping\n", i);
            items[i].size = items[i].minSize;
        }
        else
        {
            TRACE("not enough bytes for item %ld, failing\n", i);
            SetLastError(CRYPT_E_ASN1_CORRUPT);
            ret = FALSE;
        }
    }
    if (cbEncoded - (ptr - pbEncoded) != 0)
    {
        TRACE("%ld remaining bytes, failing\n", cbEncoded -
         (ptr - pbEncoded));
        SetLastError(CRYPT_E_ASN1_CORRUPT);
        ret = FALSE;
    }
    return ret;
}

/* This decodes an arbitrary sequence into a contiguous block of memory
 * (basically, a struct.)  Each element being decoded is described by a struct
 * AsnDecodeSequenceItem, see above.
 * startingPointer is an optional pointer to the first place where dynamic
 * data will be stored.  If you know the starting offset, you may pass it
 * here.  Otherwise, pass NULL, and one will be inferred from the items.
 * Each item decoder is never called with CRYPT_DECODE_ALLOC_FLAG set.
 * If any undecoded data are left over, fails with CRYPT_E_ASN1_CORRUPT.
 */
static BOOL CRYPT_AsnDecodeSequence(DWORD dwCertEncodingType,
 struct AsnDecodeSequenceItem items[], DWORD cItem, const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, PCRYPT_DECODE_PARA pDecodePara,
 void *pvStructInfo, DWORD *pcbStructInfo, void *startingPointer)
{
    BOOL ret;

    TRACE("%p, %ld, %p, %ld, %08lx, %p, %p, %ld, %p\n", items, cItem, pbEncoded,
     cbEncoded, dwFlags, pDecodePara, pvStructInfo, *pcbStructInfo,
     startingPointer);

    if (pbEncoded[0] == ASN_SEQUENCE)
    {
        DWORD dataLen;

        if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
        {
            DWORD i;

            ret = CRYPT_AsnDecodeSequenceItems(dwFlags, items, cItem, pbEncoded,
             cbEncoded, dwFlags, NULL, NULL);
            if (ret)
            {
                DWORD bytesNeeded = 0, structSize = 0;

                for (i = 0; i < cItem; i++)
                {
                    bytesNeeded += items[i].size;
                    structSize += items[i].minSize;
                }
                if (!pvStructInfo)
                    *pcbStructInfo = bytesNeeded;
                else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags,
                 pDecodePara, pvStructInfo, pcbStructInfo, bytesNeeded)))
                {
                    BYTE *nextData;

                    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                        pvStructInfo = *(BYTE **)pvStructInfo;
                    if (startingPointer)
                        nextData = (BYTE *)startingPointer;
                    else
                        nextData = (BYTE *)pvStructInfo + structSize;
                    memset(pvStructInfo, 0, structSize);
                    ret = CRYPT_AsnDecodeSequenceItems(dwFlags, items, cItem,
                     pbEncoded, cbEncoded, dwFlags, pvStructInfo, nextData);
                }
            }
        }
    }
    else
    {
        SetLastError(CRYPT_E_ASN1_BADTAG);
        ret = FALSE;
    }
    TRACE("returning %d (%08lx)\n", ret, GetLastError());
    return ret;
}

/* tag:
 *     The expected tag of the entire encoded array (usually a variant
 *     of ASN_SETOF or ASN_SEQUENCEOF.)
 * decodeFunc:
 *     used to decode each item in the array
 * itemSize:
 *      is the minimum size of each decoded item
 * hasPointer:
 *      indicates whether each item has a dynamic pointer
 * pointerOffset:
 *     indicates the offset within itemSize at which the pointer exists
 */
struct AsnArrayDescriptor
{
    BYTE                    tag;
    CryptDecodeObjectExFunc decodeFunc;
    DWORD                   itemSize;
    BOOL                    hasPointer;
    DWORD                   pointerOffset;
};

struct AsnArrayItemSize
{
    DWORD encodedLen;
    DWORD size;
};

struct GenericArray
{
    DWORD cItems;
    BYTE *rgItems;
};

/* Decodes an array of like types into a struct GenericArray.
 * The layout and decoding of the array are described by a struct
 * AsnArrayDescriptor.
 */
static BOOL CRYPT_AsnDecodeArray(const struct AsnArrayDescriptor *arrayDesc,
 const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo,
 void *startingPointer)
{
    BOOL ret = TRUE;

    TRACE("%p, %p, %ld, %08lx, %p, %p, %ld, %p\n", arrayDesc, pbEncoded,
     cbEncoded, dwFlags, pDecodePara, pvStructInfo, *pcbStructInfo,
     startingPointer);

    if (pbEncoded[0] == arrayDesc->tag)
    {
        DWORD dataLen;

        if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
        {
            DWORD bytesNeeded, cItems = 0;
            BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);
            /* There can be arbitrarily many items, but there is often only one.
             */
            struct AsnArrayItemSize itemSize = { 0 }, *itemSizes = &itemSize;

            bytesNeeded = sizeof(struct GenericArray);
            if (dataLen)
            {
                const BYTE *ptr;

                for (ptr = pbEncoded + 1 + lenBytes; ret &&
                 ptr - pbEncoded - 1 - lenBytes < dataLen; )
                {
                    DWORD itemLenBytes, itemDataLen, size;

                    itemLenBytes = GET_LEN_BYTES(ptr[1]);
                    /* Each item decoded may not tolerate extraneous bytes, so
                     * get the length of the next element and pass it directly.
                     */
                    ret = CRYPT_GetLen(ptr, cbEncoded - (ptr - pbEncoded),
                     &itemDataLen);
                    if (ret)
                        ret = arrayDesc->decodeFunc(X509_ASN_ENCODING, 0, ptr,
                         1 + itemLenBytes + itemDataLen,
                         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, NULL,
                         &size);
                    if (ret)
                    {
                        DWORD nextLen;

                        cItems++;
                        if (itemSizes != &itemSize)
                            itemSizes = CryptMemRealloc(itemSizes,
                             cItems * sizeof(struct AsnArrayItemSize));
                        else
                        {
                            itemSizes =
                             CryptMemAlloc(
                             cItems * sizeof(struct AsnArrayItemSize));
                            memcpy(itemSizes, &itemSize, sizeof(itemSize));
                        }
                        if (itemSizes)
                        {
                            itemSizes[cItems - 1].encodedLen = 1 + itemLenBytes
                             + itemDataLen;
                            itemSizes[cItems - 1].size = size;
                            bytesNeeded += size;
                            ret = CRYPT_GetLen(ptr,
                             cbEncoded - (ptr - pbEncoded), &nextLen);
                            if (ret)
                                ptr += nextLen + 1 + GET_LEN_BYTES(ptr[1]);
                        }
                        else
                            ret = FALSE;
                    }
                }
            }
            if (ret)
            {
                if (!pvStructInfo)
                    *pcbStructInfo = bytesNeeded;
                else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags,
                 pDecodePara, pvStructInfo, pcbStructInfo, bytesNeeded)))
                {
                    DWORD i;
                    BYTE *nextData;
                    const BYTE *ptr;
                    struct GenericArray *array;

                    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                        pvStructInfo = *(BYTE **)pvStructInfo;
                    array = (struct GenericArray *)pvStructInfo;
                    array->cItems = cItems;
                    if (startingPointer)
                        array->rgItems = startingPointer;
                    else
                        array->rgItems = (BYTE *)array +
                         sizeof(struct GenericArray);
                    nextData = (BYTE *)array->rgItems +
                     array->cItems * arrayDesc->itemSize;
                    for (i = 0, ptr = pbEncoded + 1 + lenBytes; ret &&
                     i < cItems && ptr - pbEncoded - 1 - lenBytes <
                     dataLen; i++)
                    {
                        if (arrayDesc->hasPointer)
                            *(BYTE **)(array->rgItems + i * arrayDesc->itemSize
                             + arrayDesc->pointerOffset) = nextData;
                        ret = arrayDesc->decodeFunc(X509_ASN_ENCODING, 0, ptr,
                         itemSizes[i].encodedLen,
                         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL,
                         array->rgItems + i * arrayDesc->itemSize,
                         &itemSizes[i].size);
                        if (ret)
                        {
                            DWORD nextLen;

                            nextData += itemSizes[i].size - arrayDesc->itemSize;
                            ret = CRYPT_GetLen(ptr,
                             cbEncoded - (ptr - pbEncoded), &nextLen);
                            if (ret)
                                ptr += nextLen + 1 + GET_LEN_BYTES(ptr[1]);
                        }
                    }
                }
            }
            if (itemSizes != &itemSize)
                CryptMemFree(itemSizes);
        }
    }
    else
    {
        SetLastError(CRYPT_E_ASN1_BADTAG);
        ret = FALSE;
    }
    return ret;
}

/* Decodes a DER-encoded BLOB into a CRYPT_DER_BLOB struct pointed to by
 * pvStructInfo.  The BLOB must be non-empty, otherwise the last error is set
 * to CRYPT_E_ASN1_CORRUPT.
 * Warning: assumes the CRYPT_DER_BLOB pointed to by pvStructInfo has pbData
 * set!
 */
static BOOL WINAPI CRYPT_AsnDecodeDerBlob(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;
    DWORD dataLen;

    if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
    {
        BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);
        DWORD bytesNeeded = sizeof(CRYPT_DER_BLOB);
       
        if (!(dwFlags & CRYPT_DECODE_NOCOPY_FLAG))
            bytesNeeded += 1 + lenBytes + dataLen;

        if (!pvStructInfo)
            *pcbStructInfo = bytesNeeded;
        else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara, 
         pvStructInfo, pcbStructInfo, bytesNeeded)))
        {
            CRYPT_DER_BLOB *blob;

            if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                pvStructInfo = *(BYTE **)pvStructInfo;
            blob = (CRYPT_DER_BLOB *)pvStructInfo;
            blob->cbData = 1 + lenBytes + dataLen;
            if (blob->cbData)
            {
                if (dwFlags & CRYPT_DECODE_NOCOPY_FLAG)
                    blob->pbData = (BYTE *)pbEncoded;
                else
                {
                    assert(blob->pbData);
                    memcpy(blob->pbData, pbEncoded, blob->cbData);
                }
            }
            else
            {
                SetLastError(CRYPT_E_ASN1_CORRUPT);
                ret = FALSE;
            }
        }
    }
    return ret;
}

/* Like CRYPT_AsnDecodeBitsInternal, but swaps the bytes */
static BOOL WINAPI CRYPT_AsnDecodeBitsSwapBytes(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    TRACE("(%p, %ld, 0x%08lx, %p, %p, %ld)\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    /* Can't use the CRYPT_DECODE_NOCOPY_FLAG, because we modify the bytes in-
     * place.
     */
    ret = CRYPT_AsnDecodeBitsInternal(dwCertEncodingType, lpszStructType,
     pbEncoded, cbEncoded, dwFlags & ~CRYPT_DECODE_NOCOPY_FLAG, pDecodePara,
     pvStructInfo, pcbStructInfo);
    if (ret && pvStructInfo)
    {
        CRYPT_BIT_BLOB *blob = (CRYPT_BIT_BLOB *)pvStructInfo;

        if (blob->cbData)
        {
            DWORD i;
            BYTE temp;

            for (i = 0; i < blob->cbData / 2; i++)
            {
                temp = blob->pbData[i];
                blob->pbData[i] = blob->pbData[blob->cbData - i - 1];
                blob->pbData[blob->cbData - i - 1] = temp;
            }
        }
    }
    TRACE("returning %d (%08lx)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeCert(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    TRACE("%p, %ld, %08lx, %p, %p, %ld\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    __TRY
    {
        struct AsnDecodeSequenceItem items[] = {
         { 0, offsetof(CERT_SIGNED_CONTENT_INFO, ToBeSigned),
           CRYPT_AsnDecodeDerBlob, sizeof(CRYPT_DER_BLOB), FALSE, TRUE,
           offsetof(CERT_SIGNED_CONTENT_INFO, ToBeSigned.pbData), 0 },
         { ASN_SEQUENCEOF, offsetof(CERT_SIGNED_CONTENT_INFO,
           SignatureAlgorithm), CRYPT_AsnDecodeAlgorithmId,
           sizeof(CRYPT_ALGORITHM_IDENTIFIER), FALSE, TRUE,
           offsetof(CERT_SIGNED_CONTENT_INFO, SignatureAlgorithm.pszObjId), 0 },
         { ASN_BITSTRING, offsetof(CERT_SIGNED_CONTENT_INFO, Signature),
           CRYPT_AsnDecodeBitsSwapBytes, sizeof(CRYPT_BIT_BLOB), FALSE, TRUE,
           offsetof(CERT_SIGNED_CONTENT_INFO, Signature.pbData), 0 },
        };

        if (dwFlags & CRYPT_DECODE_NO_SIGNATURE_BYTE_REVERSAL_FLAG)
            items[2].decodeFunc = CRYPT_AsnDecodeBitsInternal;
        ret = CRYPT_AsnDecodeSequence(dwCertEncodingType, items,
         sizeof(items) / sizeof(items[0]), pbEncoded, cbEncoded, dwFlags,
         pDecodePara, pvStructInfo, pcbStructInfo, NULL);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

/* Internal function */
static BOOL WINAPI CRYPT_AsnDecodeCertVersion(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;
    DWORD dataLen;

    if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
    {
        BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

        ret = CRYPT_AsnDecodeInt(dwCertEncodingType, X509_INTEGER,
         pbEncoded + 1 + lenBytes, dataLen, dwFlags, pDecodePara,
         pvStructInfo, pcbStructInfo);
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeValidity(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    struct AsnDecodeSequenceItem items[] = {
     { 0, offsetof(CERT_PRIVATE_KEY_VALIDITY, NotBefore),
       CRYPT_AsnDecodeChoiceOfTime, sizeof(FILETIME), FALSE, FALSE, 0 },
     { 0, offsetof(CERT_PRIVATE_KEY_VALIDITY, NotAfter),
       CRYPT_AsnDecodeChoiceOfTime, sizeof(FILETIME), FALSE, FALSE, 0 },
    };

    ret = CRYPT_AsnDecodeSequence(dwCertEncodingType, items,
     sizeof(items) / sizeof(items[0]), pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, pcbStructInfo, NULL);
    return ret;
}

/* Internal function */
static BOOL WINAPI CRYPT_AsnDecodeCertExtensions(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;
    DWORD dataLen;

    if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
    {
        BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

        ret = CRYPT_AsnDecodeExtensionsInternal(dwCertEncodingType,
         X509_EXTENSIONS, pbEncoded + 1 + lenBytes, dataLen, dwFlags,
         pDecodePara, pvStructInfo, pcbStructInfo);
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeCertInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    TRACE("%p, %ld, %08lx, %p, %p, %ld\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    __TRY
    {
        struct AsnDecodeSequenceItem items[] = {
         { ASN_CONTEXT | ASN_CONSTRUCTOR, offsetof(CERT_INFO, dwVersion),
           CRYPT_AsnDecodeCertVersion, sizeof(DWORD), TRUE, FALSE, 0, 0 },
         { ASN_INTEGER, offsetof(CERT_INFO, SerialNumber),
           CRYPT_AsnDecodeIntegerInternal, sizeof(CRYPT_INTEGER_BLOB), FALSE,
           TRUE, offsetof(CERT_INFO, SerialNumber.pbData), 0 },
         { ASN_SEQUENCEOF, offsetof(CERT_INFO, SignatureAlgorithm),
           CRYPT_AsnDecodeAlgorithmId, sizeof(CRYPT_ALGORITHM_IDENTIFIER),
           FALSE, TRUE, offsetof(CERT_INFO, SignatureAlgorithm.pszObjId), 0 },
         { 0, offsetof(CERT_INFO, Issuer), CRYPT_AsnDecodeDerBlob,
           sizeof(CRYPT_DER_BLOB), FALSE, TRUE, offsetof(CERT_INFO,
           Issuer.pbData) },
         { ASN_SEQUENCEOF, offsetof(CERT_INFO, NotBefore),
           CRYPT_AsnDecodeValidity, sizeof(CERT_PRIVATE_KEY_VALIDITY), FALSE,
           FALSE, 0 },
         { 0, offsetof(CERT_INFO, Subject), CRYPT_AsnDecodeDerBlob,
           sizeof(CRYPT_DER_BLOB), FALSE, TRUE, offsetof(CERT_INFO,
           Subject.pbData) },
         /* jil FIXME: shouldn't this have an internal version, which expects
          * the pbData to be set?
          */
         { ASN_SEQUENCEOF, offsetof(CERT_INFO, SubjectPublicKeyInfo),
           CRYPT_AsnDecodePubKeyInfo, sizeof(CERT_PUBLIC_KEY_INFO), FALSE,
           TRUE, offsetof(CERT_INFO,
           SubjectPublicKeyInfo.Algorithm.Parameters.pbData), 0 },
         { ASN_BITSTRING, offsetof(CERT_INFO, IssuerUniqueId),
           CRYPT_AsnDecodeBitsInternal, sizeof(CRYPT_BIT_BLOB), TRUE, TRUE,
           offsetof(CERT_INFO, IssuerUniqueId.pbData), 0 },
         { ASN_BITSTRING, offsetof(CERT_INFO, SubjectUniqueId),
           CRYPT_AsnDecodeBitsInternal, sizeof(CRYPT_BIT_BLOB), TRUE, TRUE,
           offsetof(CERT_INFO, SubjectUniqueId.pbData), 0 },
         { ASN_CONTEXT | ASN_CONSTRUCTOR | 3, offsetof(CERT_INFO, cExtension),
           CRYPT_AsnDecodeCertExtensions, sizeof(CERT_EXTENSIONS), TRUE, TRUE,
           offsetof(CERT_INFO, rgExtension), 0 },
        };

        ret = CRYPT_AsnDecodeSequence(dwCertEncodingType, items,
         sizeof(items) / sizeof(items[0]), pbEncoded, cbEncoded, dwFlags,
         pDecodePara, pvStructInfo, pcbStructInfo, NULL);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeCRLEntry(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;
    struct AsnDecodeSequenceItem items[] = {
     { ASN_INTEGER, offsetof(CRL_ENTRY, SerialNumber),
       CRYPT_AsnDecodeIntegerInternal, sizeof(CRYPT_INTEGER_BLOB), FALSE, TRUE,
       offsetof(CRL_ENTRY, SerialNumber.pbData), 0 },
     { 0, offsetof(CRL_ENTRY, RevocationDate), CRYPT_AsnDecodeChoiceOfTime,
       sizeof(FILETIME), FALSE, FALSE, 0 },
     { ASN_SEQUENCEOF, offsetof(CRL_ENTRY, cExtension),
       CRYPT_AsnDecodeExtensionsInternal, sizeof(CERT_EXTENSIONS), TRUE, TRUE,
       offsetof(CRL_ENTRY, rgExtension), 0 },
    };
    PCRL_ENTRY entry = (PCRL_ENTRY)pvStructInfo;

    TRACE("%p, %ld, %08lx, %p, %ld\n", pbEncoded, cbEncoded, dwFlags, entry,
     *pcbStructInfo);

    ret = CRYPT_AsnDecodeSequence(X509_ASN_ENCODING, items,
     sizeof(items) / sizeof(items[0]), pbEncoded, cbEncoded, dwFlags,
     NULL, entry, pcbStructInfo, entry ? entry->SerialNumber.pbData : NULL);
    return ret;
}

/* Warning: assumes pvStructInfo is a struct GenericArray whose rgItems has
 * been set prior to calling.
 */
static BOOL WINAPI CRYPT_AsnDecodeCRLEntries(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;
    struct AsnArrayDescriptor arrayDesc = { ASN_SEQUENCEOF,
     CRYPT_AsnDecodeCRLEntry, sizeof(CRL_ENTRY), TRUE,
     offsetof(CRL_ENTRY, SerialNumber.pbData) };
    struct GenericArray *entries = (struct GenericArray *)pvStructInfo;

    TRACE("%p, %ld, %08lx, %p, %p, %ld\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, pcbStructInfo,
     entries ? entries->rgItems : NULL);
    TRACE("Returning %d (%08lx)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeCRLInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    TRACE("%p, %ld, %08lx, %p, %p, %ld\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    __TRY
    {
        struct AsnDecodeSequenceItem items[] = {
         { ASN_CONTEXT | ASN_CONSTRUCTOR, offsetof(CRL_INFO, dwVersion),
           CRYPT_AsnDecodeCertVersion, sizeof(DWORD), TRUE, FALSE, 0, 0 },
         { ASN_SEQUENCEOF, offsetof(CRL_INFO, SignatureAlgorithm),
           CRYPT_AsnDecodeAlgorithmId, sizeof(CRYPT_ALGORITHM_IDENTIFIER),
           FALSE, TRUE, offsetof(CRL_INFO, SignatureAlgorithm.pszObjId), 0 },
         { 0, offsetof(CRL_INFO, Issuer), CRYPT_AsnDecodeDerBlob,
           sizeof(CRYPT_DER_BLOB), FALSE, TRUE, offsetof(CRL_INFO,
           Issuer.pbData) },
         { 0, offsetof(CRL_INFO, ThisUpdate), CRYPT_AsnDecodeChoiceOfTime,
           sizeof(FILETIME), FALSE, FALSE, 0 },
         { 0, offsetof(CRL_INFO, NextUpdate), CRYPT_AsnDecodeChoiceOfTime,
           sizeof(FILETIME), TRUE, FALSE, 0 },
         { ASN_SEQUENCEOF, offsetof(CRL_INFO, cCRLEntry),
           CRYPT_AsnDecodeCRLEntries, sizeof(struct GenericArray), TRUE, TRUE,
           offsetof(CRL_INFO, rgCRLEntry), 0 },
         /* Note that the extensions are ignored by MS, so I'll ignore them too
          */
         { 0, offsetof(CRL_INFO, cExtension), NULL,
           sizeof(CERT_EXTENSIONS), TRUE, FALSE, 0 },
        };

        ret = CRYPT_AsnDecodeSequence(dwCertEncodingType, items,
         sizeof(items) / sizeof(items[0]), pbEncoded, cbEncoded, dwFlags,
         pDecodePara, pvStructInfo, pcbStructInfo, NULL);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY

    TRACE("Returning %d (%08lx)\n", ret, GetLastError());
    return ret;
}

/* Differences between this and CRYPT_AsnDecodeOid:
 * - pvStructInfo is a LPSTR *, not an LPSTR
 * - CRYPT_AsnDecodeOid doesn't account for the size of an LPSTR in its byte
 *   count, whereas our callers (typically CRYPT_AsnDecodeSequence) expect this
 *   to
 */
static BOOL WINAPI CRYPT_AsnDecodeOidWrapper(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    TRACE("%p, %ld, %08lx, %p, %p, %ld\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    ret = CRYPT_AsnDecodeOid(pbEncoded, cbEncoded, dwFlags,
     pvStructInfo ? *(LPSTR *)pvStructInfo : NULL, pcbStructInfo);
    if (ret || GetLastError() == ERROR_MORE_DATA)
        *pcbStructInfo += sizeof(LPSTR);
    if (ret && pvStructInfo)
        TRACE("returning %s\n", debugstr_a(*(LPSTR *)pvStructInfo));
    return ret;
}

/* Warning:  assumes pvStructInfo is a CERT_EXTENSION whose pszObjId is set
 * ahead of time!
 */
static BOOL WINAPI CRYPT_AsnDecodeExtension(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    struct AsnDecodeSequenceItem items[] = {
     { ASN_OBJECTIDENTIFIER, offsetof(CERT_EXTENSION, pszObjId),
       CRYPT_AsnDecodeOidWrapper, sizeof(LPSTR), FALSE, TRUE,
       offsetof(CERT_EXTENSION, pszObjId), 0 },
     { ASN_BOOL, offsetof(CERT_EXTENSION, fCritical), CRYPT_AsnDecodeBool,
       sizeof(BOOL), TRUE, FALSE, 0, 0 },
     { ASN_OCTETSTRING, offsetof(CERT_EXTENSION, Value),
       CRYPT_AsnDecodeOctetsInternal, sizeof(CRYPT_OBJID_BLOB), FALSE, TRUE,
       offsetof(CERT_EXTENSION, Value.pbData) },
    };
    BOOL ret = TRUE;
    PCERT_EXTENSION ext = (PCERT_EXTENSION)pvStructInfo;

    TRACE("%p, %ld, %08lx, %p, %ld\n", pbEncoded, cbEncoded, dwFlags, ext,
     *pcbStructInfo);

    if (ext)
        TRACE("ext->pszObjId is %p\n", ext->pszObjId);
    ret = CRYPT_AsnDecodeSequence(X509_ASN_ENCODING, items,
     sizeof(items) / sizeof(items[0]), pbEncoded, cbEncoded, dwFlags, NULL,
     ext, pcbStructInfo, ext ? ext->pszObjId : NULL);
    if (ext)
        TRACE("ext->pszObjId is %p (%s)\n", ext->pszObjId,
         debugstr_a(ext->pszObjId));
    TRACE("returning %d (%08lx)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeExtensionsInternal(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;
    struct AsnArrayDescriptor arrayDesc = { ASN_SEQUENCEOF,
     CRYPT_AsnDecodeExtension, sizeof(CERT_EXTENSION), TRUE,
     offsetof(CERT_EXTENSION, pszObjId) };
    PCERT_EXTENSIONS exts = (PCERT_EXTENSIONS)pvStructInfo;

    TRACE("%p, %ld, %08lx, %p, %p, %ld\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, pcbStructInfo, exts ? exts->rgExtension : NULL);
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeExtensions(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    __TRY
    {
        ret = CRYPT_AsnDecodeExtensionsInternal(dwCertEncodingType,
         lpszStructType, pbEncoded, cbEncoded,
         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, NULL, pcbStructInfo);
        if (ret && pvStructInfo)
        {
            ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara, pvStructInfo,
             pcbStructInfo, *pcbStructInfo);
            if (ret)
            {
                CERT_EXTENSIONS *exts;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                exts = (CERT_EXTENSIONS *)pvStructInfo;
                exts->rgExtension = (CERT_EXTENSION *)((BYTE *)exts +
                 sizeof(CERT_EXTENSIONS));
                ret = CRYPT_AsnDecodeExtensionsInternal(dwCertEncodingType,
                 lpszStructType, pbEncoded, cbEncoded,
                 dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, pvStructInfo,
                 pcbStructInfo);
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

/* FIXME: honor the CRYPT_DECODE_SHARE_OID_STRING_FLAG. */
static BOOL WINAPI CRYPT_AsnDecodeOid(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, LPSTR pszObjId, DWORD *pcbObjId)
{
    BOOL ret = TRUE;

    TRACE("%p, %ld, %08lx, %p, %ld\n", pbEncoded, cbEncoded, dwFlags, pszObjId,
     *pcbObjId);

    __TRY
    {
        if (pbEncoded[0] == ASN_OBJECTIDENTIFIER)
        {
            DWORD dataLen;

            if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
            {
                BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);
                DWORD bytesNeeded;

                if (dataLen)
                {
                    /* The largest possible string for the first two components
                     * is 2.175 (= 2 * 40 + 175 = 255), so this is big enough.
                     */
                    char firstTwo[6];
                    const BYTE *ptr;

                    snprintf(firstTwo, sizeof(firstTwo), "%d.%d",
                     pbEncoded[1 + lenBytes] / 40,
                     pbEncoded[1 + lenBytes] - (pbEncoded[1 + lenBytes] / 40)
                     * 40);
                    bytesNeeded = strlen(firstTwo) + 1;
                    for (ptr = pbEncoded + 2 + lenBytes; ret &&
                     ptr - pbEncoded - 1 - lenBytes < dataLen; )
                    {
                        /* large enough for ".4000000" */
                        char str[9];
                        int val = 0;

                        while (ptr - pbEncoded - 1 - lenBytes < dataLen &&
                         (*ptr & 0x80))
                        {
                            val <<= 7;
                            val |= *ptr & 0x7f;
                            ptr++;
                        }
                        if (ptr - pbEncoded - 1 - lenBytes >= dataLen ||
                         (*ptr & 0x80))
                        {
                            SetLastError(CRYPT_E_ASN1_CORRUPT);
                            ret = FALSE;
                        }
                        else
                        {
                            val <<= 7;
                            val |= *ptr++;
                            snprintf(str, sizeof(str), ".%d", val);
                            bytesNeeded += strlen(str);
                        }
                    }
                    if (!pszObjId)
                        *pcbObjId = bytesNeeded;
                    else if (*pcbObjId < bytesNeeded)
                    {
                        *pcbObjId = bytesNeeded;
                        SetLastError(ERROR_MORE_DATA);
                        ret = FALSE;
                    }
                    else
                    {
                        *pszObjId = 0;
                        sprintf(pszObjId, "%d.%d", pbEncoded[1 + lenBytes] / 40,
                         pbEncoded[1 + lenBytes] - (pbEncoded[1 + lenBytes] /
                         40) * 40);
                        pszObjId += strlen(pszObjId);
                        for (ptr = pbEncoded + 2 + lenBytes; ret &&
                         ptr - pbEncoded - 1 - lenBytes < dataLen; )
                        {
                            int val = 0;

                            while (ptr - pbEncoded - 1 - lenBytes < dataLen &&
                             (*ptr & 0x80))
                            {
                                val <<= 7;
                                val |= *ptr & 0x7f;
                                ptr++;
                            }
                            val <<= 7;
                            val |= *ptr++;
                            sprintf(pszObjId, ".%d", val);
                            pszObjId += strlen(pszObjId);
                        }
                    }
                }
                else
                    bytesNeeded = 0;
                *pcbObjId = bytesNeeded;
            }
        }
        else
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
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

/* Warning: this assumes the address of value->Value.pbData is already set, in
 * order to avoid overwriting memory.  (In some cases, it may change it, if it
 * doesn't copy anything to memory.)  Be sure to set it correctly!
 */
static BOOL WINAPI CRYPT_AsnDecodeNameValue(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    __TRY
    {
        DWORD dataLen;
        CERT_NAME_VALUE *value = (CERT_NAME_VALUE *)pvStructInfo;

        if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
        {
            BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

            switch (pbEncoded[0])
            {
            case ASN_NUMERICSTRING:
            case ASN_PRINTABLESTRING:
            case ASN_IA5STRING:
                break;
            default:
                FIXME("Unimplemented string type %02x\n", pbEncoded[0]);
                SetLastError(OSS_UNIMPLEMENTED);
                ret = FALSE;
            }
            if (ret)
            {
                DWORD bytesNeeded = sizeof(CERT_NAME_VALUE);

                switch (pbEncoded[0])
                {
                case ASN_NUMERICSTRING:
                case ASN_PRINTABLESTRING:
                case ASN_IA5STRING:
                    if (!(dwFlags & CRYPT_DECODE_NOCOPY_FLAG))
                        bytesNeeded += dataLen;
                    break;
                }
                if (!value)
                    *pcbStructInfo = bytesNeeded;
                else if (*pcbStructInfo < bytesNeeded)
                {
                    *pcbStructInfo = bytesNeeded;
                    SetLastError(ERROR_MORE_DATA);
                    ret = FALSE;
                }
                else
                {
                    *pcbStructInfo = bytesNeeded;
                    switch (pbEncoded[0])
                    {
                    case ASN_NUMERICSTRING:
                        value->dwValueType = CERT_RDN_NUMERIC_STRING;
                        break;
                    case ASN_PRINTABLESTRING:
                        value->dwValueType = CERT_RDN_PRINTABLE_STRING;
                        break;
                    case ASN_IA5STRING:
                        value->dwValueType = CERT_RDN_IA5_STRING;
                        break;
                    }
                    if (dataLen)
                    {
                        switch (pbEncoded[0])
                        {
                        case ASN_NUMERICSTRING:
                        case ASN_PRINTABLESTRING:
                        case ASN_IA5STRING:
                            value->Value.cbData = dataLen;
                            if (dwFlags & CRYPT_DECODE_NOCOPY_FLAG)
                                value->Value.pbData = (BYTE *)pbEncoded + 1 +
                                 lenBytes;
                            else
                            {
                                assert(value->Value.pbData);
                                memcpy(value->Value.pbData,
                                 pbEncoded + 1 + lenBytes, dataLen);
                            }
                            break;
                        }
                    }
                    else
                    {
                        value->Value.cbData = 0;
                        value->Value.pbData = NULL;
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

static BOOL WINAPI CRYPT_AsnDecodeRdnAttr(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    TRACE("%p, %ld, %08lx, %p, %ld\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    __TRY
    {
        struct AsnDecodeSequenceItem items[] = {
         { ASN_OBJECTIDENTIFIER, offsetof(CERT_RDN_ATTR, pszObjId),
           CRYPT_AsnDecodeOidWrapper, sizeof(LPSTR), FALSE, TRUE,
           offsetof(CERT_RDN_ATTR, pszObjId), 0 },
         { 0, offsetof(CERT_RDN_ATTR, dwValueType), CRYPT_AsnDecodeNameValue,
           sizeof(CERT_NAME_VALUE), FALSE, TRUE, offsetof(CERT_RDN_ATTR,
           Value.pbData), 0 },
        };
        CERT_RDN_ATTR *attr = (CERT_RDN_ATTR *)pvStructInfo;

        if (attr)
            TRACE("attr->pszObjId is %p\n", attr->pszObjId);
        ret = CRYPT_AsnDecodeSequence(X509_ASN_ENCODING, items,
         sizeof(items) / sizeof(items[0]), pbEncoded, cbEncoded, dwFlags, NULL,
         attr, pcbStructInfo, attr ? attr->pszObjId : NULL);
        if (attr)
            TRACE("attr->pszObjId is %p (%s)\n", attr->pszObjId,
             debugstr_a(attr->pszObjId));
        TRACE("returning %d (%08lx)\n", ret, GetLastError());
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeRdn(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    __TRY
    {
        struct AsnArrayDescriptor arrayDesc = { ASN_CONSTRUCTOR | ASN_SETOF,
         CRYPT_AsnDecodeRdnAttr, sizeof(CERT_RDN_ATTR), TRUE,
         offsetof(CERT_RDN_ATTR, pszObjId) };

        ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded, dwFlags,
         pDecodePara, pvStructInfo, pcbStructInfo, NULL);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeName(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    __TRY
    {
        struct AsnArrayDescriptor arrayDesc = { ASN_SEQUENCEOF,
         CRYPT_AsnDecodeRdn, sizeof(CERT_RDN), TRUE,
         offsetof(CERT_RDN, rgRDNAttr) };

        ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded, dwFlags,
         pDecodePara, pvStructInfo, pcbStructInfo, NULL);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeCopyBytes(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;
    DWORD bytesNeeded = sizeof(CRYPT_OBJID_BLOB);

    TRACE("%p, %ld, %08lx, %p, %p, %ld\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    if (!(dwFlags & CRYPT_DECODE_NOCOPY_FLAG))
        bytesNeeded += cbEncoded;
    if (!pvStructInfo)
        *pcbStructInfo = bytesNeeded;
    else if (*pcbStructInfo < bytesNeeded)
    {
        SetLastError(ERROR_MORE_DATA);
        *pcbStructInfo = bytesNeeded;
        ret = FALSE;
    }
    else
    {
        PCRYPT_OBJID_BLOB blob = (PCRYPT_OBJID_BLOB)pvStructInfo;

        *pcbStructInfo = bytesNeeded;
        blob->cbData = cbEncoded;
        if (dwFlags & CRYPT_DECODE_NOCOPY_FLAG)
            blob->pbData = (LPBYTE)pbEncoded;
        else
        {
            assert(blob->pbData);
            memcpy(blob->pbData, pbEncoded, blob->cbData);
        }
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeAlgorithmId(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    CRYPT_ALGORITHM_IDENTIFIER *algo =
     (CRYPT_ALGORITHM_IDENTIFIER *)pvStructInfo;
    BOOL ret = TRUE;
    struct AsnDecodeSequenceItem items[] = {
     { ASN_OBJECTIDENTIFIER, offsetof(CRYPT_ALGORITHM_IDENTIFIER, pszObjId),
       CRYPT_AsnDecodeOidWrapper, sizeof(LPSTR), FALSE, TRUE, 
       offsetof(CRYPT_ALGORITHM_IDENTIFIER, pszObjId), 0 },
     { 0, offsetof(CRYPT_ALGORITHM_IDENTIFIER, Parameters),
       CRYPT_AsnDecodeCopyBytes, sizeof(CRYPT_OBJID_BLOB), TRUE, TRUE, 
       offsetof(CRYPT_ALGORITHM_IDENTIFIER, Parameters.pbData), 0 },
    };

    TRACE("%p, %ld, %08lx, %p, %p, %ld\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    ret = CRYPT_AsnDecodeSequence(dwCertEncodingType, items,
     sizeof(items) / sizeof(items[0]), pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, pcbStructInfo, algo ? algo->pszObjId : NULL);
    if (ret && pvStructInfo)
    {
        TRACE("pszObjId is %p (%s)\n", algo->pszObjId,
         debugstr_a(algo->pszObjId));
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodePubKeyInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    __TRY
    {
        struct AsnDecodeSequenceItem items[] = {
         { ASN_SEQUENCEOF, offsetof(CERT_PUBLIC_KEY_INFO, Algorithm),
           CRYPT_AsnDecodeAlgorithmId, sizeof(CRYPT_ALGORITHM_IDENTIFIER),
           FALSE, TRUE, offsetof(CERT_PUBLIC_KEY_INFO,
           Algorithm.pszObjId) },
         { ASN_BITSTRING, offsetof(CERT_PUBLIC_KEY_INFO, PublicKey),
           CRYPT_AsnDecodeBitsInternal, sizeof(CRYPT_BIT_BLOB), FALSE, TRUE,
           offsetof(CERT_PUBLIC_KEY_INFO, PublicKey.pbData) },
        };

        ret = CRYPT_AsnDecodeSequence(dwCertEncodingType, items,
         sizeof(items) / sizeof(items[0]), pbEncoded, cbEncoded, dwFlags,
         pDecodePara, pvStructInfo, pcbStructInfo, NULL);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeBool(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    if (cbEncoded < 3)
    {
        SetLastError(CRYPT_E_ASN1_CORRUPT);
        return FALSE;
    }
    if (GET_LEN_BYTES(pbEncoded[1]) > 1)
    {
        SetLastError(CRYPT_E_ASN1_CORRUPT);
        return FALSE;
    }
    if (pbEncoded[1] > 1)
    {
        SetLastError(CRYPT_E_ASN1_CORRUPT);
        return FALSE;
    }
    if (!pvStructInfo)
    {
        *pcbStructInfo = sizeof(BOOL);
        ret = TRUE;
    }
    else if (*pcbStructInfo < sizeof(BOOL))
    {
        *pcbStructInfo = sizeof(BOOL);
        SetLastError(ERROR_MORE_DATA);
        ret = FALSE;
    }
    else
    {
        *(BOOL *)pvStructInfo = pbEncoded[2] ? TRUE : FALSE;
        ret = TRUE;
    }
    TRACE("returning %d (%08lx)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeAltNameEntry(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    PCERT_ALT_NAME_ENTRY entry = (PCERT_ALT_NAME_ENTRY)pvStructInfo;
    DWORD dataLen, lenBytes, bytesNeeded = sizeof(CERT_ALT_NAME_ENTRY);
    BOOL ret;

    TRACE("%p, %ld, %08lx, %p, %p, %ld\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    if (cbEncoded < 2)
    {
        SetLastError(CRYPT_E_ASN1_CORRUPT);
        return FALSE;
    }
    if ((pbEncoded[0] & ASN_FLAGS_MASK) != ASN_CONTEXT)
    {
        SetLastError(CRYPT_E_ASN1_BADTAG);
        return FALSE;
    }
    lenBytes = GET_LEN_BYTES(pbEncoded[1]);
    if (1 + lenBytes > cbEncoded)
    {
        SetLastError(CRYPT_E_ASN1_CORRUPT);
        return FALSE;
    }
    if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
    {
        switch (pbEncoded[0] & ASN_TYPE_MASK)
        {
        case 1: /* rfc822Name */
        case 2: /* dNSName */
        case 6: /* uniformResourceIdentifier */
            bytesNeeded += (dataLen + 1) * sizeof(WCHAR);
            break;
        case 7: /* iPAddress */
            bytesNeeded += dataLen;
            break;
        case 8: /* registeredID */
            /* FIXME: decode as OID */
        case 0: /* otherName */
        case 4: /* directoryName */
            FIXME("stub\n");
            SetLastError(CRYPT_E_ASN1_BADTAG);
            ret = FALSE;
            break;
        case 3: /* x400Address, unimplemented */
        case 5: /* ediPartyName, unimplemented */
            SetLastError(CRYPT_E_ASN1_BADTAG);
            ret = FALSE;
            break;
        default:
            SetLastError(CRYPT_E_ASN1_CORRUPT);
            ret = FALSE;
        }
        if (ret)
        {
            if (!entry)
                *pcbStructInfo = bytesNeeded;
            else if (*pcbStructInfo < bytesNeeded)
            {
                *pcbStructInfo = bytesNeeded;
                SetLastError(ERROR_MORE_DATA);
                ret = FALSE;
            }
            else
            {
                *pcbStructInfo = bytesNeeded;
                /* MS used values one greater than the asn1 ones.. sigh */
                entry->dwAltNameChoice = (pbEncoded[0] & 0x7f) + 1;
                switch (pbEncoded[0] & ASN_TYPE_MASK)
                {
                case 1: /* rfc822Name */
                case 2: /* dNSName */
                case 6: /* uniformResourceIdentifier */
                {
                    DWORD i;

                    for (i = 0; i < dataLen; i++)
                        entry->u.pwszURL[i] =
                         (WCHAR)pbEncoded[1 + lenBytes + i];
                    entry->u.pwszURL[i] = 0;
                    TRACE("URL is %p (%s)\n", entry->u.pwszURL,
                     debugstr_w(entry->u.pwszURL));
                    break;
                }
                case 7: /* iPAddress */
                    /* The next data pointer is in the pwszURL spot, that is,
                     * the first 4 bytes.  Need to move it to the next spot.
                     */
                    entry->u.IPAddress.pbData = (LPBYTE)entry->u.pwszURL;
                    entry->u.IPAddress.cbData = dataLen;
                    memcpy(entry->u.IPAddress.pbData, pbEncoded + 1 + lenBytes,
                     dataLen);
                    break;
                }
            }
        }
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeAltNameInternal(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;
    struct AsnArrayDescriptor arrayDesc = { ASN_SEQUENCEOF,
     CRYPT_AsnDecodeAltNameEntry, sizeof(CERT_ALT_NAME_ENTRY), TRUE,
     offsetof(CERT_ALT_NAME_ENTRY, u.pwszURL) };
    PCERT_ALT_NAME_INFO info = (PCERT_ALT_NAME_INFO)pvStructInfo;

    TRACE("%p, %ld, %08lx, %p, %p, %ld\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    if (info)
        TRACE("info->rgAltEntry is %p\n", info->rgAltEntry);
    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, pcbStructInfo, info ? info->rgAltEntry : NULL);
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeAltName(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    TRACE("%p, %ld, %08lx, %p, %p, %ld\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    __TRY
    {
        struct AsnArrayDescriptor arrayDesc = { ASN_SEQUENCEOF,
         CRYPT_AsnDecodeAltNameEntry, sizeof(CERT_ALT_NAME_ENTRY), TRUE,
         offsetof(CERT_ALT_NAME_ENTRY, u.pwszURL) };

        ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded, dwFlags,
         pDecodePara, pvStructInfo, pcbStructInfo, NULL);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

struct PATH_LEN_CONSTRAINT
{
    BOOL  fPathLenConstraint;
    DWORD dwPathLenConstraint;
};

static BOOL WINAPI CRYPT_AsnDecodePathLenConstraint(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    TRACE("%p, %ld, %08lx, %p, %ld\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    if (cbEncoded)
    {
        if (pbEncoded[0] == ASN_INTEGER)
        {
            DWORD bytesNeeded = sizeof(struct PATH_LEN_CONSTRAINT);

            if (!pvStructInfo)
                *pcbStructInfo = bytesNeeded;
            else if (*pcbStructInfo < bytesNeeded)
            {
                SetLastError(ERROR_MORE_DATA);
                *pcbStructInfo = bytesNeeded;
                ret = FALSE;
            }
            else
            {
                struct PATH_LEN_CONSTRAINT *constraint =
                 (struct PATH_LEN_CONSTRAINT *)pvStructInfo;
                DWORD size = sizeof(constraint->dwPathLenConstraint);

                ret = CRYPT_AsnDecodeInt(dwCertEncodingType, X509_INTEGER,
                 pbEncoded, cbEncoded, 0, NULL,
                 &constraint->dwPathLenConstraint, &size);
                if (ret)
                    constraint->fPathLenConstraint = TRUE;
                TRACE("got an int, dwPathLenConstraint is %ld\n",
                 constraint->dwPathLenConstraint);
            }
        }
        else
        {
            SetLastError(CRYPT_E_ASN1_CORRUPT);
            ret = FALSE;
        }
    }
    TRACE("returning %d (%08lx)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeBasicConstraints2(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    __TRY
    {
        struct AsnDecodeSequenceItem items[] = {
         { ASN_BOOL, offsetof(CERT_BASIC_CONSTRAINTS2_INFO, fCA),
           CRYPT_AsnDecodeBool, sizeof(BOOL), TRUE, FALSE, 0, 0 },
         { ASN_INTEGER, offsetof(CERT_BASIC_CONSTRAINTS2_INFO,
           fPathLenConstraint), CRYPT_AsnDecodePathLenConstraint,
           sizeof(struct PATH_LEN_CONSTRAINT), TRUE, FALSE, 0, 0 },
        };

        ret = CRYPT_AsnDecodeSequence(dwCertEncodingType, items,
         sizeof(items) / sizeof(items[0]), pbEncoded, cbEncoded, dwFlags,
         pDecodePara, pvStructInfo, pcbStructInfo, NULL);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

#define RSA1_MAGIC 0x31415352

struct DECODED_RSA_PUB_KEY
{
    DWORD              pubexp;
    CRYPT_INTEGER_BLOB modulus;
};

static BOOL WINAPI CRYPT_AsnDecodeRsaPubKey(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    __TRY
    {
        struct AsnDecodeSequenceItem items[] = {
         { ASN_INTEGER, offsetof(struct DECODED_RSA_PUB_KEY, modulus),
           CRYPT_AsnDecodeUnsignedIntegerInternal, sizeof(CRYPT_INTEGER_BLOB),
           FALSE, TRUE, offsetof(struct DECODED_RSA_PUB_KEY, modulus.pbData),
           0 },
         { ASN_INTEGER, offsetof(struct DECODED_RSA_PUB_KEY, pubexp),
           CRYPT_AsnDecodeInt, sizeof(DWORD), FALSE, FALSE, 0, 0 },
        };
        struct DECODED_RSA_PUB_KEY *decodedKey = NULL;
        DWORD size = 0;

        ret = CRYPT_AsnDecodeSequence(dwCertEncodingType, items,
         sizeof(items) / sizeof(items[0]), pbEncoded, cbEncoded,
         CRYPT_DECODE_ALLOC_FLAG, NULL, &decodedKey, &size, NULL);
        if (ret)
        {
            DWORD bytesNeeded = sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) +
             decodedKey->modulus.cbData;

            if (!pvStructInfo)
            {
                *pcbStructInfo = bytesNeeded;
                ret = TRUE;
            }
            else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
             pvStructInfo, pcbStructInfo, bytesNeeded)))
            {
                BLOBHEADER *hdr;
                RSAPUBKEY *rsaPubKey;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                hdr = (BLOBHEADER *)pvStructInfo;
                hdr->bType = PUBLICKEYBLOB;
                hdr->bVersion = CUR_BLOB_VERSION;
                hdr->reserved = 0;
                hdr->aiKeyAlg = CALG_RSA_KEYX;
                rsaPubKey = (RSAPUBKEY *)((BYTE *)pvStructInfo +
                 sizeof(BLOBHEADER));
                rsaPubKey->magic = RSA1_MAGIC;
                rsaPubKey->pubexp = decodedKey->pubexp;
                rsaPubKey->bitlen = decodedKey->modulus.cbData * 8;
                memcpy((BYTE *)pvStructInfo + sizeof(BLOBHEADER) +
                 sizeof(RSAPUBKEY), decodedKey->modulus.pbData,
                 decodedKey->modulus.cbData);
            }
            LocalFree(decodedKey);
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

static BOOL WINAPI CRYPT_AsnDecodeOctetsInternal(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;
    DWORD bytesNeeded, dataLen;

    TRACE("%p, %ld, %08lx, %p, %p, %ld\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
    {
        if (dwFlags & CRYPT_DECODE_NOCOPY_FLAG)
            bytesNeeded = sizeof(CRYPT_DATA_BLOB);
        else
            bytesNeeded = dataLen + sizeof(CRYPT_DATA_BLOB);
        if (!pvStructInfo)
            *pcbStructInfo = bytesNeeded;
        else if (*pcbStructInfo < bytesNeeded)
        {
            SetLastError(ERROR_MORE_DATA);
            *pcbStructInfo = bytesNeeded;
            ret = FALSE;
        }
        else
        {
            CRYPT_DATA_BLOB *blob;
            BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

            blob = (CRYPT_DATA_BLOB *)pvStructInfo;
            blob->cbData = dataLen;
            if (dwFlags & CRYPT_DECODE_NOCOPY_FLAG)
                blob->pbData = (BYTE *)pbEncoded + 1 + lenBytes;
            else
            {
                assert(blob->pbData);
                if (blob->cbData)
                    memcpy(blob->pbData, pbEncoded + 1 + lenBytes,
                     blob->cbData);
            }
        }
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeOctets(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    TRACE("%p, %ld, %08lx, %p, %p, %ld\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    __TRY
    {
        DWORD bytesNeeded;

        if (!cbEncoded)
        {
            SetLastError(CRYPT_E_ASN1_CORRUPT);
            ret = FALSE;
        }
        else if (pbEncoded[0] != ASN_OCTETSTRING)
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
            ret = FALSE;
        }
        else if ((ret = CRYPT_AsnDecodeOctetsInternal(dwCertEncodingType,
         lpszStructType, pbEncoded, cbEncoded,
         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, NULL, &bytesNeeded)))
        {
            if (!pvStructInfo)
                *pcbStructInfo = bytesNeeded;
            else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
             pvStructInfo, pcbStructInfo, bytesNeeded)))
            {
                CRYPT_DATA_BLOB *blob;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                blob = (CRYPT_DATA_BLOB *)pvStructInfo;
                blob->pbData = (BYTE *)pvStructInfo + sizeof(CRYPT_DATA_BLOB);
                ret = CRYPT_AsnDecodeOctetsInternal(dwCertEncodingType,
                 lpszStructType, pbEncoded, cbEncoded,
                 dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, pvStructInfo,
                 &bytesNeeded);
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

static BOOL WINAPI CRYPT_AsnDecodeBitsInternal(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    TRACE("(%p, %ld, 0x%08lx, %p, %p, %ld)\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    if (pbEncoded[0] == ASN_BITSTRING)
    {
        DWORD bytesNeeded, dataLen;

        if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
        {
            if (dwFlags & CRYPT_DECODE_NOCOPY_FLAG)
                bytesNeeded = sizeof(CRYPT_BIT_BLOB);
            else
                bytesNeeded = dataLen - 1 + sizeof(CRYPT_BIT_BLOB);
            if (!pvStructInfo)
                *pcbStructInfo = bytesNeeded;
            else if (*pcbStructInfo < bytesNeeded)
            {
                *pcbStructInfo = bytesNeeded;
                SetLastError(ERROR_MORE_DATA);
                ret = FALSE;
            }
            else
            {
                CRYPT_BIT_BLOB *blob;

                blob = (CRYPT_BIT_BLOB *)pvStructInfo;
                blob->cbData = dataLen - 1;
                blob->cUnusedBits = *(pbEncoded + 1 +
                 GET_LEN_BYTES(pbEncoded[1]));
                if (dwFlags & CRYPT_DECODE_NOCOPY_FLAG)
                {
                    blob->pbData = (BYTE *)pbEncoded + 2 +
                     GET_LEN_BYTES(pbEncoded[1]);
                }
                else
                {
                    assert(blob->pbData);
                    if (blob->cbData)
                    {
                        BYTE mask = 0xff << blob->cUnusedBits;

                        memcpy(blob->pbData, pbEncoded + 2 +
                         GET_LEN_BYTES(pbEncoded[1]), blob->cbData);
                        blob->pbData[blob->cbData - 1] &= mask;
                    }
                }
            }
        }
    }
    else
    {
        SetLastError(CRYPT_E_ASN1_BADTAG);
        ret = FALSE;
    }
    TRACE("returning %d (%08lx)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeBits(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    TRACE("(%p, %ld, 0x%08lx, %p, %p, %p)\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, pcbStructInfo);

    __TRY
    {
        DWORD bytesNeeded;

        if ((ret = CRYPT_AsnDecodeBitsInternal(dwCertEncodingType,
         lpszStructType, pbEncoded, cbEncoded,
         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, NULL, &bytesNeeded)))
        {
            if (!pvStructInfo)
                *pcbStructInfo = bytesNeeded;
            else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
             pvStructInfo, pcbStructInfo, bytesNeeded)))
            {
                CRYPT_BIT_BLOB *blob;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                blob = (CRYPT_BIT_BLOB *)pvStructInfo;
                blob->pbData = (BYTE *)pvStructInfo + sizeof(CRYPT_BIT_BLOB);
                ret = CRYPT_AsnDecodeBitsInternal(dwCertEncodingType,
                 lpszStructType, pbEncoded, cbEncoded,
                 dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, pvStructInfo,
                 &bytesNeeded);
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

static BOOL WINAPI CRYPT_AsnDecodeInt(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    if (!pvStructInfo)
    {
        *pcbStructInfo = sizeof(int);
        return TRUE;
    }
    __TRY
    {
        BYTE buf[sizeof(CRYPT_INTEGER_BLOB) + sizeof(int)];
        CRYPT_INTEGER_BLOB *blob = (CRYPT_INTEGER_BLOB *)buf;
        DWORD size = sizeof(buf);

        blob->pbData = buf + sizeof(CRYPT_INTEGER_BLOB);
        ret = CRYPT_AsnDecodeIntegerInternal(dwCertEncodingType,
         X509_MULTI_BYTE_INTEGER, pbEncoded, cbEncoded, 0, NULL, &buf, &size);
        if (ret)
        {
            if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
             pvStructInfo, pcbStructInfo, sizeof(int))))
            {
                int val, i;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                if (blob->pbData[blob->cbData - 1] & 0x80)
                {
                    /* initialize to a negative value to sign-extend */
                    val = -1;
                }
                else
                    val = 0;
                for (i = 0; i < blob->cbData; i++)
                {
                    val <<= 8;
                    val |= blob->pbData[blob->cbData - i - 1];
                }
                memcpy(pvStructInfo, &val, sizeof(int));
            }
        }
        else if (GetLastError() == ERROR_MORE_DATA)
            SetLastError(CRYPT_E_ASN1_LARGE);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeIntegerInternal(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    if (pbEncoded[0] == ASN_INTEGER)
    {
        DWORD bytesNeeded, dataLen;

        if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
        {
            BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

            bytesNeeded = dataLen + sizeof(CRYPT_INTEGER_BLOB);
            if (!pvStructInfo)
                *pcbStructInfo = bytesNeeded;
            else if (*pcbStructInfo < bytesNeeded)
            {
                *pcbStructInfo = bytesNeeded;
                SetLastError(ERROR_MORE_DATA);
                ret = FALSE;
            }
            else
            {
                CRYPT_INTEGER_BLOB *blob = (CRYPT_INTEGER_BLOB *)pvStructInfo;

                blob->cbData = dataLen;
                assert(blob->pbData);
                if (blob->cbData)
                {
                    DWORD i;

                    for (i = 0; i < blob->cbData; i++)
                    {
                        blob->pbData[i] = *(pbEncoded + 1 + lenBytes +
                         dataLen - i - 1);
                    }
                }
            }
        }
    }
    else
    {
        SetLastError(CRYPT_E_ASN1_BADTAG);
        ret = FALSE;
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeInteger(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    __TRY
    {
        DWORD bytesNeeded;

        if ((ret = CRYPT_AsnDecodeIntegerInternal(dwCertEncodingType,
         lpszStructType, pbEncoded, cbEncoded,
         dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG, NULL, NULL, &bytesNeeded)))
        {
            if (!pvStructInfo)
                *pcbStructInfo = bytesNeeded;
            else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
             pvStructInfo, pcbStructInfo, bytesNeeded)))
            {
                CRYPT_INTEGER_BLOB *blob;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                blob = (CRYPT_INTEGER_BLOB *)pvStructInfo;
                blob->pbData = (BYTE *)pvStructInfo +
                 sizeof(CRYPT_INTEGER_BLOB);
                ret = CRYPT_AsnDecodeIntegerInternal(dwCertEncodingType,
                 lpszStructType, pbEncoded, cbEncoded,
                 dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG, NULL, pvStructInfo,
                 &bytesNeeded);
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

static BOOL WINAPI CRYPT_AsnDecodeUnsignedIntegerInternal(
 DWORD dwCertEncodingType, LPCSTR lpszStructType, const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, PCRYPT_DECODE_PARA pDecodePara,
 void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    if (pbEncoded[0] == ASN_INTEGER)
    {
        DWORD bytesNeeded, dataLen;

        if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
        {
            BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

            bytesNeeded = dataLen + sizeof(CRYPT_INTEGER_BLOB);
            if (!pvStructInfo)
                *pcbStructInfo = bytesNeeded;
            else if (*pcbStructInfo < bytesNeeded)
            {
                *pcbStructInfo = bytesNeeded;
                SetLastError(ERROR_MORE_DATA);
                ret = FALSE;
            }
            else
            {
                CRYPT_INTEGER_BLOB *blob = (CRYPT_INTEGER_BLOB *)pvStructInfo;

                blob->cbData = dataLen;
                assert(blob->pbData);
                /* remove leading zero byte if it exists */
                if (blob->cbData && *(pbEncoded + 1 + lenBytes) == 0)
                {
                    blob->cbData--;
                    blob->pbData++;
                }
                if (blob->cbData)
                {
                    DWORD i;

                    for (i = 0; i < blob->cbData; i++)
                    {
                        blob->pbData[i] = *(pbEncoded + 1 + lenBytes +
                         dataLen - i - 1);
                    }
                }
            }
        }
    }
    else
    {
        SetLastError(CRYPT_E_ASN1_BADTAG);
        ret = FALSE;
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeUnsignedInteger(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    __TRY
    {
        DWORD bytesNeeded;

        if ((ret = CRYPT_AsnDecodeUnsignedIntegerInternal(dwCertEncodingType,
         lpszStructType, pbEncoded, cbEncoded,
         dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG, NULL, NULL, &bytesNeeded)))
        {
            if (!pvStructInfo)
                *pcbStructInfo = bytesNeeded;
            else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
             pvStructInfo, pcbStructInfo, bytesNeeded)))
            {
                CRYPT_INTEGER_BLOB *blob;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                blob = (CRYPT_INTEGER_BLOB *)pvStructInfo;
                blob->pbData = (BYTE *)pvStructInfo +
                 sizeof(CRYPT_INTEGER_BLOB);
                ret = CRYPT_AsnDecodeUnsignedIntegerInternal(dwCertEncodingType,
                 lpszStructType, pbEncoded, cbEncoded,
                 dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG, NULL, pvStructInfo,
                 &bytesNeeded);
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

static BOOL WINAPI CRYPT_AsnDecodeEnumerated(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    if (!pvStructInfo)
    {
        *pcbStructInfo = sizeof(int);
        return TRUE;
    }
    __TRY
    {
        if (pbEncoded[0] == ASN_ENUMERATED)
        {
            unsigned int val = 0, i;

            if (cbEncoded <= 1)
            {
                SetLastError(CRYPT_E_ASN1_EOD);
                ret = FALSE;
            }
            else if (pbEncoded[1] == 0)
            {
                SetLastError(CRYPT_E_ASN1_CORRUPT);
                ret = FALSE;
            }
            else
            {
                /* A little strange looking, but we have to accept a sign byte:
                 * 0xffffffff gets encoded as 0a 05 00 ff ff ff ff.  Also,
                 * assuming a small length is okay here, it has to be in short
                 * form.
                 */
                if (pbEncoded[1] > sizeof(unsigned int) + 1)
                {
                    SetLastError(CRYPT_E_ASN1_LARGE);
                    return FALSE;
                }
                for (i = 0; i < pbEncoded[1]; i++)
                {
                    val <<= 8;
                    val |= pbEncoded[2 + i];
                }
                if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
                 pvStructInfo, pcbStructInfo, sizeof(unsigned int))))
                {
                    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                        pvStructInfo = *(BYTE **)pvStructInfo;
                    memcpy(pvStructInfo, &val, sizeof(unsigned int));
                }
            }
        }
        else
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
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

/* Modifies word, pbEncoded, and len, and magically sets a value ret to FALSE
 * if it fails.
 */
#define CRYPT_TIME_GET_DIGITS(pbEncoded, len, numDigits, word) \
 do { \
    BYTE i; \
 \
    (word) = 0; \
    for (i = 0; (len) > 0 && i < (numDigits); i++, (len)--) \
    { \
        if (!isdigit(*(pbEncoded))) \
        { \
            SetLastError(CRYPT_E_ASN1_CORRUPT); \
            ret = FALSE; \
        } \
        else \
        { \
            (word) *= 10; \
            (word) += *(pbEncoded)++ - '0'; \
        } \
    } \
 } while (0)

static BOOL CRYPT_AsnDecodeTimeZone(const BYTE *pbEncoded, DWORD len,
 SYSTEMTIME *sysTime)
{
    BOOL ret;

    __TRY
    {
        ret = TRUE;
        if (len >= 3 && (*pbEncoded == '+' || *pbEncoded == '-'))
        {
            WORD hours, minutes = 0;
            BYTE sign = *pbEncoded++;

            len--;
            CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, hours);
            if (ret && hours >= 24)
            {
                SetLastError(CRYPT_E_ASN1_CORRUPT);
                ret = FALSE;
            }
            else if (len >= 2)
            {
                CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, minutes);
                if (ret && minutes >= 60)
                {
                    SetLastError(CRYPT_E_ASN1_CORRUPT);
                    ret = FALSE;
                }
            }
            if (ret)
            {
                if (sign == '+')
                {
                    sysTime->wHour += hours;
                    sysTime->wMinute += minutes;
                }
                else
                {
                    if (hours > sysTime->wHour)
                    {
                        sysTime->wDay--;
                        sysTime->wHour = 24 - (hours - sysTime->wHour);
                    }
                    else
                        sysTime->wHour -= hours;
                    if (minutes > sysTime->wMinute)
                    {
                        sysTime->wHour--;
                        sysTime->wMinute = 60 - (minutes - sysTime->wMinute);
                    }
                    else
                        sysTime->wMinute -= minutes;
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

#define MIN_ENCODED_TIME_LENGTH 10

static BOOL WINAPI CRYPT_AsnDecodeUtcTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    if (!pvStructInfo)
    {
        *pcbStructInfo = sizeof(FILETIME);
        return TRUE;
    }
    __TRY
    {
        ret = TRUE;
        if (pbEncoded[0] == ASN_UTCTIME)
        {
            if (cbEncoded <= 1)
            {
                SetLastError(CRYPT_E_ASN1_EOD);
                ret = FALSE;
            }
            else if (pbEncoded[1] > 0x7f)
            {
                /* long-form date strings really can't be valid */
                SetLastError(CRYPT_E_ASN1_CORRUPT);
                ret = FALSE;
            }
            else
            {
                SYSTEMTIME sysTime = { 0 };
                BYTE len = pbEncoded[1];

                if (len < MIN_ENCODED_TIME_LENGTH)
                {
                    SetLastError(CRYPT_E_ASN1_CORRUPT);
                    ret = FALSE;
                }
                else
                {
                    pbEncoded += 2;
                    CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, sysTime.wYear);
                    if (sysTime.wYear >= 50)
                        sysTime.wYear += 1900;
                    else
                        sysTime.wYear += 2000;
                    CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, sysTime.wMonth);
                    CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, sysTime.wDay);
                    CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, sysTime.wHour);
                    CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, sysTime.wMinute);
                    if (ret && len > 0)
                    {
                        if (len >= 2 && isdigit(*pbEncoded) &&
                         isdigit(*(pbEncoded + 1)))
                            CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2,
                             sysTime.wSecond);
                        else if (isdigit(*pbEncoded))
                            CRYPT_TIME_GET_DIGITS(pbEncoded, len, 1,
                             sysTime.wSecond);
                        if (ret)
                            ret = CRYPT_AsnDecodeTimeZone(pbEncoded, len,
                             &sysTime);
                    }
                    if (ret && (ret = CRYPT_DecodeEnsureSpace(dwFlags,
                     pDecodePara, pvStructInfo, pcbStructInfo,
                     sizeof(FILETIME))))
                    {
                        if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                            pvStructInfo = *(BYTE **)pvStructInfo;
                        ret = SystemTimeToFileTime(&sysTime,
                         (FILETIME *)pvStructInfo);
                    }
                }
            }
        }
        else
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
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

static BOOL WINAPI CRYPT_AsnDecodeGeneralizedTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    if (!pvStructInfo)
    {
        *pcbStructInfo = sizeof(FILETIME);
        return TRUE;
    }
    __TRY
    {
        ret = TRUE;
        if (pbEncoded[0] == ASN_GENERALTIME)
        {
            if (cbEncoded <= 1)
            {
                SetLastError(CRYPT_E_ASN1_EOD);
                ret = FALSE;
            }
            else if (pbEncoded[1] > 0x7f)
            {
                /* long-form date strings really can't be valid */
                SetLastError(CRYPT_E_ASN1_CORRUPT);
                ret = FALSE;
            }
            else
            {
                BYTE len = pbEncoded[1];

                if (len < MIN_ENCODED_TIME_LENGTH)
                {
                    SetLastError(CRYPT_E_ASN1_CORRUPT);
                    ret = FALSE;
                }
                else
                {
                    SYSTEMTIME sysTime = { 0 };

                    pbEncoded += 2;
                    CRYPT_TIME_GET_DIGITS(pbEncoded, len, 4, sysTime.wYear);
                    CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, sysTime.wMonth);
                    CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, sysTime.wDay);
                    CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2, sysTime.wHour);
                    if (ret && len > 0)
                    {
                        CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2,
                         sysTime.wMinute);
                        if (ret && len > 0)
                            CRYPT_TIME_GET_DIGITS(pbEncoded, len, 2,
                             sysTime.wSecond);
                        if (ret && len > 0 && (*pbEncoded == '.' ||
                         *pbEncoded == ','))
                        {
                            BYTE digits;

                            pbEncoded++;
                            len--;
                            /* workaround macro weirdness */
                            digits = min(len, 3);
                            CRYPT_TIME_GET_DIGITS(pbEncoded, len, digits,
                             sysTime.wMilliseconds);
                        }
                        if (ret)
                            ret = CRYPT_AsnDecodeTimeZone(pbEncoded, len,
                             &sysTime);
                    }
                    if (ret && (ret = CRYPT_DecodeEnsureSpace(dwFlags,
                     pDecodePara, pvStructInfo, pcbStructInfo,
                     sizeof(FILETIME))))
                    {
                        if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                            pvStructInfo = *(BYTE **)pvStructInfo;
                        ret = SystemTimeToFileTime(&sysTime,
                         (FILETIME *)pvStructInfo);
                    }
                }
            }
        }
        else
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
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

static BOOL WINAPI CRYPT_AsnDecodeChoiceOfTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    __TRY
    {
        if (pbEncoded[0] == ASN_UTCTIME)
            ret = CRYPT_AsnDecodeUtcTime(dwCertEncodingType, lpszStructType,
             pbEncoded, cbEncoded, dwFlags, pDecodePara, pvStructInfo,
             pcbStructInfo);
        else if (pbEncoded[0] == ASN_GENERALTIME)
            ret = CRYPT_AsnDecodeGeneralizedTime(dwCertEncodingType,
             lpszStructType, pbEncoded, cbEncoded, dwFlags, pDecodePara,
             pvStructInfo, pcbStructInfo);
        else
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
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

static BOOL WINAPI CRYPT_AsnDecodeSequenceOfAny(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    __TRY
    {
        if (pbEncoded[0] == ASN_SEQUENCEOF)
        {
            DWORD bytesNeeded, dataLen, remainingLen, cValue;

            if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
            {
                BYTE lenBytes;
                const BYTE *ptr;

                lenBytes = GET_LEN_BYTES(pbEncoded[1]);
                bytesNeeded = sizeof(CRYPT_SEQUENCE_OF_ANY);
                cValue = 0;
                ptr = pbEncoded + 1 + lenBytes;
                remainingLen = dataLen;
                while (ret && remainingLen)
                {
                    DWORD nextLen;

                    ret = CRYPT_GetLen(ptr, remainingLen, &nextLen);
                    if (ret)
                    {
                        DWORD nextLenBytes = GET_LEN_BYTES(ptr[1]);

                        remainingLen -= 1 + nextLenBytes + nextLen;
                        ptr += 1 + nextLenBytes + nextLen;
                        bytesNeeded += sizeof(CRYPT_DER_BLOB);
                        if (!(dwFlags & CRYPT_DECODE_NOCOPY_FLAG))
                            bytesNeeded += 1 + nextLenBytes + nextLen;
                        cValue++;
                    }
                }
                if (ret)
                {
                    CRYPT_SEQUENCE_OF_ANY *seq;
                    BYTE *nextPtr;
                    DWORD i;

                    if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
                     pvStructInfo, pcbStructInfo, bytesNeeded)))
                    {
                        if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                            pvStructInfo = *(BYTE **)pvStructInfo;
                        seq = (CRYPT_SEQUENCE_OF_ANY *)pvStructInfo;
                        seq->cValue = cValue;
                        seq->rgValue = (CRYPT_DER_BLOB *)((BYTE *)seq +
                         sizeof(*seq));
                        nextPtr = (BYTE *)seq->rgValue +
                         cValue * sizeof(CRYPT_DER_BLOB);
                        ptr = pbEncoded + 1 + lenBytes;
                        remainingLen = dataLen;
                        i = 0;
                        while (ret && remainingLen)
                        {
                            DWORD nextLen;

                            ret = CRYPT_GetLen(ptr, remainingLen, &nextLen);
                            if (ret)
                            {
                                DWORD nextLenBytes = GET_LEN_BYTES(ptr[1]);

                                seq->rgValue[i].cbData = 1 + nextLenBytes +
                                 nextLen;
                                if (dwFlags & CRYPT_DECODE_NOCOPY_FLAG)
                                    seq->rgValue[i].pbData = (BYTE *)ptr;
                                else
                                {
                                    seq->rgValue[i].pbData = nextPtr;
                                    memcpy(nextPtr, ptr, 1 + nextLenBytes +
                                     nextLen);
                                    nextPtr += 1 + nextLenBytes + nextLen;
                                }
                                remainingLen -= 1 + nextLenBytes + nextLen;
                                ptr += 1 + nextLenBytes + nextLen;
                                i++;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
            return FALSE;
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

static BOOL WINAPI CRYPT_AsnDecodeDistPoint(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    struct AsnDecodeSequenceItem items[] = {
     { ASN_CONTEXT | ASN_CONSTRUCTOR | 0, offsetof(CRL_DIST_POINT,
       DistPointName), CRYPT_AsnDecodeAltNameInternal,
       sizeof(CRL_DIST_POINT_NAME), TRUE, TRUE, offsetof(CRL_DIST_POINT,
       DistPointName.u.FullName.rgAltEntry), 0 },
     { ASN_CONTEXT | 1, offsetof(CRL_DIST_POINT, ReasonFlags),
       CRYPT_AsnDecodeBitsInternal, sizeof(CRYPT_BIT_BLOB), TRUE, TRUE,
       offsetof(CRL_DIST_POINT, ReasonFlags.pbData), 0 },
     { ASN_CONTEXT | ASN_CONSTRUCTOR | 2, offsetof(CRL_DIST_POINT, CRLIssuer),
       CRYPT_AsnDecodeAltNameInternal, sizeof(CERT_ALT_NAME_INFO), TRUE, TRUE,
       offsetof(CRL_DIST_POINT, CRLIssuer.rgAltEntry), 0 },
    };
    BOOL ret;

    ret = CRYPT_AsnDecodeSequence(dwCertEncodingType, items,
     sizeof(items) / sizeof(items[0]), pbEncoded, cbEncoded,
     dwFlags, pDecodePara, pvStructInfo, pcbStructInfo, NULL);
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeCRLDistPoints(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    TRACE("%p, %ld, %08lx, %p, %p, %ld\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    __TRY
    {
        struct AsnArrayDescriptor arrayDesc = { ASN_SEQUENCEOF,
         CRYPT_AsnDecodeDistPoint, sizeof(CRL_DIST_POINT), TRUE,
         offsetof(CRL_DIST_POINT, DistPointName.u.FullName.rgAltEntry) };

        ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded, dwFlags,
         pDecodePara, pvStructInfo, pcbStructInfo, NULL);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

BOOL WINAPI CryptDecodeObjectEx(DWORD dwCertEncodingType, LPCSTR lpszStructType,
 const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    static HCRYPTOIDFUNCSET set = NULL;
    BOOL ret = FALSE;
    CryptDecodeObjectExFunc decodeFunc = NULL;
    HCRYPTOIDFUNCADDR hFunc = NULL;

    TRACE("(0x%08lx, %s, %p, %ld, 0x%08lx, %p, %p, %p)\n",
     dwCertEncodingType, debugstr_a(lpszStructType), pbEncoded,
     cbEncoded, dwFlags, pDecodePara, pvStructInfo, pcbStructInfo);

    if (!pvStructInfo && !pcbStructInfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if ((dwCertEncodingType & CERT_ENCODING_TYPE_MASK) != X509_ASN_ENCODING
     && (dwCertEncodingType & CMSG_ENCODING_TYPE_MASK) != PKCS_7_ASN_ENCODING)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }
    if (!cbEncoded)
    {
        SetLastError(CRYPT_E_ASN1_EOD);
        return FALSE;
    }
    if (cbEncoded > MAX_ENCODED_LEN)
    {
        SetLastError(CRYPT_E_ASN1_LARGE);
        return FALSE;
    }

    SetLastError(NOERROR);
    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG && pvStructInfo)
        *(BYTE **)pvStructInfo = NULL;
    if (!HIWORD(lpszStructType))
    {
        switch (LOWORD(lpszStructType))
        {
        case (WORD)X509_CERT:
            decodeFunc = CRYPT_AsnDecodeCert;
            break;
        case (WORD)X509_CERT_TO_BE_SIGNED:
            decodeFunc = CRYPT_AsnDecodeCertInfo;
            break;
        case (WORD)X509_CERT_CRL_TO_BE_SIGNED:
            decodeFunc = CRYPT_AsnDecodeCRLInfo;
            break;
        case (WORD)X509_EXTENSIONS:
            decodeFunc = CRYPT_AsnDecodeExtensions;
            break;
        case (WORD)X509_NAME:
            decodeFunc = CRYPT_AsnDecodeName;
            break;
        case (WORD)X509_PUBLIC_KEY_INFO:
            decodeFunc = CRYPT_AsnDecodePubKeyInfo;
            break;
        case (WORD)X509_ALTERNATE_NAME:
            decodeFunc = CRYPT_AsnDecodeAltName;
            break;
        case (WORD)X509_BASIC_CONSTRAINTS2:
            decodeFunc = CRYPT_AsnDecodeBasicConstraints2;
            break;
        case (WORD)RSA_CSP_PUBLICKEYBLOB:
            decodeFunc = CRYPT_AsnDecodeRsaPubKey;
            break;
        case (WORD)X509_OCTET_STRING:
            decodeFunc = CRYPT_AsnDecodeOctets;
            break;
        case (WORD)X509_BITS:
        case (WORD)X509_KEY_USAGE:
            decodeFunc = CRYPT_AsnDecodeBits;
            break;
        case (WORD)X509_INTEGER:
            decodeFunc = CRYPT_AsnDecodeInt;
            break;
        case (WORD)X509_MULTI_BYTE_INTEGER:
            decodeFunc = CRYPT_AsnDecodeInteger;
            break;
        case (WORD)X509_MULTI_BYTE_UINT:
            decodeFunc = CRYPT_AsnDecodeUnsignedInteger;
            break;
        case (WORD)X509_ENUMERATED:
            decodeFunc = CRYPT_AsnDecodeEnumerated;
            break;
        case (WORD)X509_CHOICE_OF_TIME:
            decodeFunc = CRYPT_AsnDecodeChoiceOfTime;
            break;
        case (WORD)X509_SEQUENCE_OF_ANY:
            decodeFunc = CRYPT_AsnDecodeSequenceOfAny;
            break;
        case (WORD)PKCS_UTC_TIME:
            decodeFunc = CRYPT_AsnDecodeUtcTime;
            break;
        case (WORD)X509_CRL_DIST_POINTS:
            decodeFunc = CRYPT_AsnDecodeCRLDistPoints;
            break;
        default:
            FIXME("%d: unimplemented\n", LOWORD(lpszStructType));
        }
    }
    else if (!strcmp(lpszStructType, szOID_CERT_EXTENSIONS))
        decodeFunc = CRYPT_AsnDecodeExtensions;
    else if (!strcmp(lpszStructType, szOID_RSA_signingTime))
        decodeFunc = CRYPT_AsnDecodeUtcTime;
    else if (!strcmp(lpszStructType, szOID_CRL_REASON_CODE))
        decodeFunc = CRYPT_AsnDecodeEnumerated;
    else if (!strcmp(lpszStructType, szOID_KEY_USAGE))
        decodeFunc = CRYPT_AsnDecodeBits;
    else if (!strcmp(lpszStructType, szOID_SUBJECT_KEY_IDENTIFIER))
        decodeFunc = CRYPT_AsnDecodeOctets;
    else if (!strcmp(lpszStructType, szOID_BASIC_CONSTRAINTS2))
        decodeFunc = CRYPT_AsnDecodeBasicConstraints2;
    else if (!strcmp(lpszStructType, szOID_ISSUER_ALT_NAME))
        decodeFunc = CRYPT_AsnDecodeAltName;
    else if (!strcmp(lpszStructType, szOID_ISSUER_ALT_NAME2))
        decodeFunc = CRYPT_AsnDecodeAltName;
    else if (!strcmp(lpszStructType, szOID_NEXT_UPDATE_LOCATION))
        decodeFunc = CRYPT_AsnDecodeAltName;
    else if (!strcmp(lpszStructType, szOID_SUBJECT_ALT_NAME))
        decodeFunc = CRYPT_AsnDecodeAltName;
    else if (!strcmp(lpszStructType, szOID_SUBJECT_ALT_NAME2))
        decodeFunc = CRYPT_AsnDecodeAltName;
    else
        TRACE("OID %s not found or unimplemented, looking for DLL\n",
         debugstr_a(lpszStructType));
    if (!decodeFunc)
    {
        if (!set)
            set = CryptInitOIDFunctionSet(CRYPT_OID_DECODE_OBJECT_EX_FUNC, 0);
        CryptGetOIDFunctionAddress(set, dwCertEncodingType, lpszStructType, 0,
         (void **)&decodeFunc, &hFunc);
    }
    if (decodeFunc)
        ret = decodeFunc(dwCertEncodingType, lpszStructType, pbEncoded,
         cbEncoded, dwFlags, pDecodePara, pvStructInfo, pcbStructInfo);
    else
        SetLastError(ERROR_FILE_NOT_FOUND);
    if (hFunc)
        CryptFreeOIDFunctionAddress(hFunc, 0);
    return ret;
}

BOOL WINAPI CryptExportPublicKeyInfo(HCRYPTPROV hCryptProv, DWORD dwKeySpec,
 DWORD dwCertEncodingType, PCERT_PUBLIC_KEY_INFO pInfo, DWORD *pcbInfo)
{
    return CryptExportPublicKeyInfoEx(hCryptProv, dwKeySpec, dwCertEncodingType,
     NULL, 0, NULL, pInfo, pcbInfo);
}

static BOOL WINAPI CRYPT_ExportRsaPublicKeyInfoEx(HCRYPTPROV hCryptProv,
 DWORD dwKeySpec, DWORD dwCertEncodingType, LPSTR pszPublicKeyObjId,
 DWORD dwFlags, void *pvAuxInfo, PCERT_PUBLIC_KEY_INFO pInfo, DWORD *pcbInfo)
{
    BOOL ret;
    HCRYPTKEY key;

    TRACE("(%ld, %ld, %08lx, %s, %08lx, %p, %p, %p)\n", hCryptProv, dwKeySpec,
     dwCertEncodingType, debugstr_a(pszPublicKeyObjId), dwFlags, pvAuxInfo,
     pInfo, pcbInfo);

    if (!pszPublicKeyObjId)
        pszPublicKeyObjId = szOID_RSA_RSA;
    if ((ret = CryptGetUserKey(hCryptProv, dwKeySpec, &key)))
    {
        DWORD keySize = 0;

        ret = CryptExportKey(key, 0, PUBLICKEYBLOB, 0, NULL, &keySize);
        if (ret)
        {
            LPBYTE pubKey = CryptMemAlloc(keySize);

            if (pubKey)
            {
                ret = CryptExportKey(key, 0, PUBLICKEYBLOB, 0, pubKey,
                 &keySize);
                if (ret)
                {
                    DWORD encodedLen = 0;

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

typedef BOOL (WINAPI *ExportPublicKeyInfoExFunc)(HCRYPTPROV hCryptProv,
 DWORD dwKeySpec, DWORD dwCertEncodingType, LPSTR pszPublicKeyObjId,
 DWORD dwFlags, void *pvAuxInfo, PCERT_PUBLIC_KEY_INFO pInfo, DWORD *pcbInfo);

BOOL WINAPI CryptExportPublicKeyInfoEx(HCRYPTPROV hCryptProv, DWORD dwKeySpec,
 DWORD dwCertEncodingType, LPSTR pszPublicKeyObjId, DWORD dwFlags,
 void *pvAuxInfo, PCERT_PUBLIC_KEY_INFO pInfo, DWORD *pcbInfo)
{
    static HCRYPTOIDFUNCSET set = NULL;
    BOOL ret;
    ExportPublicKeyInfoExFunc exportFunc = NULL;
    HCRYPTOIDFUNCADDR hFunc = NULL;

    TRACE("(%ld, %ld, %08lx, %s, %08lx, %p, %p, %p)\n", hCryptProv, dwKeySpec,
     dwCertEncodingType, debugstr_a(pszPublicKeyObjId), dwFlags, pvAuxInfo,
     pInfo, pcbInfo);

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
        exportFunc = CRYPT_ExportRsaPublicKeyInfoEx;
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

static BOOL WINAPI CRYPT_ImportRsaPublicKeyInfoEx(HCRYPTPROV hCryptProv,
 DWORD dwCertEncodingType, PCERT_PUBLIC_KEY_INFO pInfo, ALG_ID aiKeyAlg,
 DWORD dwFlags, void *pvAuxInfo, HCRYPTKEY *phKey)
{
    BOOL ret;
    DWORD pubKeySize = 0;

    TRACE("(%ld, %ld, %p, %d, %08lx, %p, %p)\n", hCryptProv,
     dwCertEncodingType, pInfo, aiKeyAlg, dwFlags, pvAuxInfo, phKey);

    ret = CryptDecodeObject(dwCertEncodingType, RSA_CSP_PUBLICKEYBLOB,
     pInfo->PublicKey.pbData, pInfo->PublicKey.cbData, 0, NULL, &pubKeySize);
    if (ret)
    {
        LPBYTE pubKey = CryptMemAlloc(pubKeySize);

        if (pubKey)
        {
            ret = CryptDecodeObject(dwCertEncodingType, RSA_CSP_PUBLICKEYBLOB,
             pInfo->PublicKey.pbData, pInfo->PublicKey.cbData, 0, pubKey,
             &pubKeySize);
            if (ret)
                ret = CryptImportKey(hCryptProv, pubKey, pubKeySize, 0, 0,
                 phKey);
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

    TRACE("(%ld, %ld, %p, %d, %08lx, %p, %p)\n", hCryptProv,
     dwCertEncodingType, pInfo, aiKeyAlg, dwFlags, pvAuxInfo, phKey);

    if (!set)
        set = CryptInitOIDFunctionSet(CRYPT_OID_IMPORT_PUBLIC_KEY_INFO_FUNC, 0);
    CryptGetOIDFunctionAddress(set, dwCertEncodingType,
     pInfo->Algorithm.pszObjId, 0, (void **)&importFunc, &hFunc);
    if (!importFunc)
        importFunc = CRYPT_ImportRsaPublicKeyInfoEx;
    ret = importFunc(hCryptProv, dwCertEncodingType, pInfo, aiKeyAlg, dwFlags,
     pvAuxInfo, phKey);
    if (hFunc)
        CryptFreeOIDFunctionAddress(hFunc, 0);
    return ret;
}
