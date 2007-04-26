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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * This file implements ASN.1 DER decoding of a limited set of types.
 * It isn't a full ASN.1 implementation.  Microsoft implements BER
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
#include "winnls.h"
#include "winreg.h"
#include "snmp.h"
#include "wine/debug.h"
#include "wine/exception.h"
#include "crypt32_private.h"

/* This is a bit arbitrary, but to set some limit: */
#define MAX_ENCODED_LEN 0x02000000

#define ASN_FLAGS_MASK 0xe0
#define ASN_TYPE_MASK  0x1f

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

struct GenericArray
{
    DWORD cItems;
    BYTE *rgItems;
};

typedef BOOL (WINAPI *CryptDecodeObjectFunc)(DWORD, LPCSTR, const BYTE *,
 DWORD, DWORD, void *, DWORD *);
typedef BOOL (WINAPI *CryptDecodeObjectExFunc)(DWORD, LPCSTR, const BYTE *,
 DWORD, DWORD, PCRYPT_DECODE_PARA, void *, DWORD *);

/* Prototypes for built-in decoders.  They follow the Ex style prototypes.
 * The dwCertEncodingType and lpszStructType are ignored by the built-in
 * functions, but the parameters are retained to simplify CryptDecodeObjectEx,
 * since it must call functions in external DLLs that follow these signatures.
 */
static BOOL WINAPI CRYPT_AsnDecodeChoiceOfTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo);
static BOOL WINAPI CRYPT_AsnDecodePubKeyInfoInternal(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo);
/* Like CRYPT_AsnDecodeExtensions, except assumes rgExtension is set ahead of
 * time, doesn't do memory allocation, and doesn't do exception handling.
 * (This isn't intended to be the externally-called one.)
 */
static BOOL WINAPI CRYPT_AsnDecodeExtensionsInternal(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo);
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
 * memory allocation.  Also doesn't check tag, assumes the caller has checked
 * it.
 */
static BOOL WINAPI CRYPT_AsnDecodeIntegerInternal(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo);
/* Like CRYPT_AsnDecodeInteger, but unsigned.  */
static BOOL WINAPI CRYPT_AsnDecodeUnsignedIntegerInternal(
 DWORD dwCertEncodingType, LPCSTR lpszStructType, const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, PCRYPT_DECODE_PARA pDecodePara,
 void *pvStructInfo, DWORD *pcbStructInfo);

BOOL WINAPI CryptDecodeObject(DWORD dwCertEncodingType, LPCSTR lpszStructType,
 const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo,
 DWORD *pcbStructInfo)
{
    static HCRYPTOIDFUNCSET set = NULL;
    BOOL ret = FALSE;
    CryptDecodeObjectFunc pCryptDecodeObject;
    HCRYPTOIDFUNCADDR hFunc;

    TRACE("(0x%08x, %s, %p, %d, 0x%08x, %p, %p)\n", dwCertEncodingType,
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
                            TRACE("decoding item %d\n", i);
                        else
                            TRACE("sizing item %d\n", i);
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
                            TRACE("skipping optional item %d\n", i);
                            items[i].size = items[i].minSize;
                            SetLastError(NOERROR);
                            ret = TRUE;
                        }
                        else
                            TRACE("item %d failed: %08x\n", i,
                             GetLastError());
                    }
                    else
                        items[i].size = items[i].minSize;
                }
                else if (items[i].optional)
                {
                    TRACE("skipping optional item %d\n", i);
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
            TRACE("missing optional item %d, skipping\n", i);
            items[i].size = items[i].minSize;
        }
        else
        {
            TRACE("not enough bytes for item %d, failing\n", i);
            SetLastError(CRYPT_E_ASN1_CORRUPT);
            ret = FALSE;
        }
    }
    if (cbEncoded - (ptr - pbEncoded) != 0)
    {
        TRACE("%d remaining bytes, failing\n", cbEncoded -
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

    TRACE("%p, %d, %p, %d, %08x, %p, %p, %d, %p\n", items, cItem, pbEncoded,
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
    TRACE("returning %d (%08x)\n", ret, GetLastError());
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

    TRACE("%p, %p, %d, %08x, %p, %p, %d, %p\n", arrayDesc, pbEncoded,
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
                            if (itemSizes)
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

    TRACE("(%p, %d, 0x%08x, %p, %p, %d)\n", pbEncoded, cbEncoded, dwFlags,
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
    TRACE("returning %d (%08x)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeCertSignedContent(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
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

    TRACE("Returning %d (%08x)\n", ret, GetLastError());
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
     { ASN_SEQUENCEOF, offsetof(CERT_INFO, SubjectPublicKeyInfo),
       CRYPT_AsnDecodePubKeyInfoInternal, sizeof(CERT_PUBLIC_KEY_INFO),
       FALSE, TRUE, offsetof(CERT_INFO,
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

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    ret = CRYPT_AsnDecodeSequence(dwCertEncodingType, items,
     sizeof(items) / sizeof(items[0]), pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, pcbStructInfo, NULL);

    TRACE("Returning %d (%08x)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeCert(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    __TRY
    {
        PCERT_SIGNED_CONTENT_INFO signedCert = NULL;
        DWORD size = 0;

        /* First try to decode it as a signed cert. */
        ret = CRYPT_AsnDecodeCertSignedContent(dwCertEncodingType, X509_CERT,
         pbEncoded, cbEncoded, CRYPT_DECODE_ALLOC_FLAG, NULL,
         (BYTE *)&signedCert, &size);
        if (ret)
        {
            size = 0;
            ret = CRYPT_AsnDecodeCertInfo(dwCertEncodingType,
             X509_CERT_TO_BE_SIGNED, signedCert->ToBeSigned.pbData,
             signedCert->ToBeSigned.cbData, dwFlags, pDecodePara, pvStructInfo,
             pcbStructInfo);
            LocalFree(signedCert);
        }
        /* Failing that, try it as an unsigned cert */
        if (!ret)
        {
            size = 0;
            ret = CRYPT_AsnDecodeCertInfo(dwCertEncodingType,
             X509_CERT_TO_BE_SIGNED, pbEncoded, cbEncoded, dwFlags,
             pDecodePara, pvStructInfo, pcbStructInfo);
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY

    TRACE("Returning %d (%08x)\n", ret, GetLastError());
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

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags, entry,
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

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, pcbStructInfo,
     entries ? entries->rgItems : NULL);
    TRACE("Returning %d (%08x)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeCRLInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    struct AsnDecodeSequenceItem items[] = {
     { ASN_INTEGER, offsetof(CRL_INFO, dwVersion),
       CRYPT_AsnDecodeInt, sizeof(DWORD), TRUE, FALSE, 0, 0 },
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
     { ASN_CONTEXT | ASN_CONSTRUCTOR | 0, offsetof(CRL_INFO, cExtension),
       CRYPT_AsnDecodeCertExtensions, sizeof(CERT_EXTENSIONS), TRUE, TRUE,
       offsetof(CRL_INFO, rgExtension), 0 },
    };
    BOOL ret = TRUE;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    ret = CRYPT_AsnDecodeSequence(dwCertEncodingType, items,
     sizeof(items) / sizeof(items[0]), pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, pcbStructInfo, NULL);

    TRACE("Returning %d (%08x)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeCRL(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    __TRY
    {
        PCERT_SIGNED_CONTENT_INFO signedCrl = NULL;
        DWORD size = 0;

        /* First try to decode it as a signed crl. */
        ret = CRYPT_AsnDecodeCertSignedContent(dwCertEncodingType, X509_CERT,
         pbEncoded, cbEncoded, CRYPT_DECODE_ALLOC_FLAG, NULL,
         (BYTE *)&signedCrl, &size);
        if (ret)
        {
            size = 0;
            ret = CRYPT_AsnDecodeCRLInfo(dwCertEncodingType,
             X509_CERT_CRL_TO_BE_SIGNED, signedCrl->ToBeSigned.pbData,
             signedCrl->ToBeSigned.cbData, dwFlags, pDecodePara,
             pvStructInfo, pcbStructInfo);
            LocalFree(signedCrl);
        }
        /* Failing that, try it as an unsigned crl */
        if (!ret)
        {
            size = 0;
            ret = CRYPT_AsnDecodeCRLInfo(dwCertEncodingType,
             X509_CERT_CRL_TO_BE_SIGNED, pbEncoded, cbEncoded,
             dwFlags, pDecodePara, pvStructInfo, pcbStructInfo);
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY

    TRACE("Returning %d (%08x)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeOidInternal(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    if (pbEncoded[0] == ASN_OBJECTIDENTIFIER)
    {
        DWORD dataLen;

        if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
        {
            BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);
            DWORD bytesNeeded = sizeof(LPSTR);

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
                bytesNeeded += strlen(firstTwo) + 1;
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
            }
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
                if (dataLen)
                {
                    const BYTE *ptr;
                    LPSTR pszObjId = *(LPSTR *)pvStructInfo;

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
                else
                    *(LPSTR *)pvStructInfo = NULL;
                *pcbStructInfo = bytesNeeded;
            }
        }
    }
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
       CRYPT_AsnDecodeOidInternal, sizeof(LPSTR), FALSE, TRUE,
       offsetof(CERT_EXTENSION, pszObjId), 0 },
     { ASN_BOOL, offsetof(CERT_EXTENSION, fCritical), CRYPT_AsnDecodeBool,
       sizeof(BOOL), TRUE, FALSE, 0, 0 },
     { ASN_OCTETSTRING, offsetof(CERT_EXTENSION, Value),
       CRYPT_AsnDecodeOctetsInternal, sizeof(CRYPT_OBJID_BLOB), FALSE, TRUE,
       offsetof(CERT_EXTENSION, Value.pbData) },
    };
    BOOL ret = TRUE;
    PCERT_EXTENSION ext = (PCERT_EXTENSION)pvStructInfo;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags, ext,
     *pcbStructInfo);

    if (ext)
        TRACE("ext->pszObjId is %p\n", ext->pszObjId);
    ret = CRYPT_AsnDecodeSequence(X509_ASN_ENCODING, items,
     sizeof(items) / sizeof(items[0]), pbEncoded, cbEncoded, dwFlags, NULL,
     ext, pcbStructInfo, ext ? ext->pszObjId : NULL);
    if (ext)
        TRACE("ext->pszObjId is %p (%s)\n", ext->pszObjId,
         debugstr_a(ext->pszObjId));
    TRACE("returning %d (%08x)\n", ret, GetLastError());
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

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
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

/* Warning: this assumes the address of value->Value.pbData is already set, in
 * order to avoid overwriting memory.  (In some cases, it may change it, if it
 * doesn't copy anything to memory.)  Be sure to set it correctly!
 */
static BOOL WINAPI CRYPT_AsnDecodeNameValueInternal(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;
    DWORD dataLen;
    CERT_NAME_VALUE *value = (CERT_NAME_VALUE *)pvStructInfo;

    if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
    {
        BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);
        DWORD bytesNeeded = sizeof(CERT_NAME_VALUE), valueType;

        switch (pbEncoded[0])
        {
        case ASN_OCTETSTRING:
            valueType = CERT_RDN_OCTET_STRING;
            if (!(dwFlags & CRYPT_DECODE_NOCOPY_FLAG))
                bytesNeeded += dataLen;
            break;
        case ASN_NUMERICSTRING:
            valueType = CERT_RDN_NUMERIC_STRING;
            if (!(dwFlags & CRYPT_DECODE_NOCOPY_FLAG))
                bytesNeeded += dataLen;
            break;
        case ASN_PRINTABLESTRING:
            valueType = CERT_RDN_PRINTABLE_STRING;
            if (!(dwFlags & CRYPT_DECODE_NOCOPY_FLAG))
                bytesNeeded += dataLen;
            break;
        case ASN_IA5STRING:
            valueType = CERT_RDN_IA5_STRING;
            if (!(dwFlags & CRYPT_DECODE_NOCOPY_FLAG))
                bytesNeeded += dataLen;
            break;
        case ASN_T61STRING:
            valueType = CERT_RDN_T61_STRING;
            if (!(dwFlags & CRYPT_DECODE_NOCOPY_FLAG))
                bytesNeeded += dataLen;
            break;
        case ASN_VIDEOTEXSTRING:
            valueType = CERT_RDN_VIDEOTEX_STRING;
            if (!(dwFlags & CRYPT_DECODE_NOCOPY_FLAG))
                bytesNeeded += dataLen;
            break;
        case ASN_GRAPHICSTRING:
            valueType = CERT_RDN_GRAPHIC_STRING;
            if (!(dwFlags & CRYPT_DECODE_NOCOPY_FLAG))
                bytesNeeded += dataLen;
            break;
        case ASN_VISIBLESTRING:
            valueType = CERT_RDN_VISIBLE_STRING;
            if (!(dwFlags & CRYPT_DECODE_NOCOPY_FLAG))
                bytesNeeded += dataLen;
            break;
        case ASN_GENERALSTRING:
            valueType = CERT_RDN_GENERAL_STRING;
            if (!(dwFlags & CRYPT_DECODE_NOCOPY_FLAG))
                bytesNeeded += dataLen;
            break;
        case ASN_UNIVERSALSTRING:
            FIXME("ASN_UNIVERSALSTRING: unimplemented\n");
            SetLastError(CRYPT_E_ASN1_BADTAG);
            return FALSE;
        case ASN_BMPSTRING:
            valueType = CERT_RDN_BMP_STRING;
            bytesNeeded += dataLen;
            break;
        case ASN_UTF8STRING:
            valueType = CERT_RDN_UTF8_STRING;
            bytesNeeded += MultiByteToWideChar(CP_UTF8, 0,
             (LPCSTR)pbEncoded + 1 + lenBytes, dataLen, NULL, 0) * 2;
            break;
        default:
            SetLastError(CRYPT_E_ASN1_BADTAG);
            return FALSE;
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
            value->dwValueType = valueType;
            if (dataLen)
            {
                DWORD i;

                assert(value->Value.pbData);
                switch (pbEncoded[0])
                {
                case ASN_OCTETSTRING:
                case ASN_NUMERICSTRING:
                case ASN_PRINTABLESTRING:
                case ASN_IA5STRING:
                case ASN_T61STRING:
                case ASN_VIDEOTEXSTRING:
                case ASN_GRAPHICSTRING:
                case ASN_VISIBLESTRING:
                case ASN_GENERALSTRING:
                    value->Value.cbData = dataLen;
                    if (dataLen)
                    {
                        if (!(dwFlags & CRYPT_DECODE_NOCOPY_FLAG))
                            memcpy(value->Value.pbData,
                             pbEncoded + 1 + lenBytes, dataLen);
                        else
                            value->Value.pbData = (LPBYTE)pbEncoded + 1 +
                             lenBytes;
                    }
                    break;
                case ASN_BMPSTRING:
                {
                    LPWSTR str = (LPWSTR)value->Value.pbData;

                    value->Value.cbData = dataLen;
                    for (i = 0; i < dataLen / 2; i++)
                        str[i] = (pbEncoded[1 + lenBytes + 2 * i] << 8) |
                         pbEncoded[1 + lenBytes + 2 * i + 1];
                    break;
                }
                case ASN_UTF8STRING:
                {
                    LPWSTR str = (LPWSTR)value->Value.pbData;

                    value->Value.cbData = MultiByteToWideChar(CP_UTF8, 0,
                     (LPCSTR)pbEncoded + 1 + lenBytes, dataLen, 
                     str, bytesNeeded - sizeof(CERT_NAME_VALUE)) * 2;
                    break;
                }
                }
            }
            else
            {
                value->Value.cbData = 0;
                value->Value.pbData = NULL;
            }
        }
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeNameValue(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    __TRY
    {
        ret = CRYPT_AsnDecodeNameValueInternal(dwCertEncodingType,
         lpszStructType, pbEncoded, cbEncoded,
         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, NULL, pcbStructInfo);
        if (ret && pvStructInfo)
        {
            ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara, pvStructInfo,
             pcbStructInfo, *pcbStructInfo);
            if (ret)
            {
                CERT_NAME_VALUE *value;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                value = (CERT_NAME_VALUE *)pvStructInfo;
                value->Value.pbData = ((BYTE *)value + sizeof(CERT_NAME_VALUE));
                ret = CRYPT_AsnDecodeNameValueInternal(dwCertEncodingType,
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

static BOOL WINAPI CRYPT_AsnDecodeUnicodeNameValueInternal(
 DWORD dwCertEncodingType, LPCSTR lpszStructType, const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, PCRYPT_DECODE_PARA pDecodePara,
 void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;
    DWORD dataLen;
    CERT_NAME_VALUE *value = (CERT_NAME_VALUE *)pvStructInfo;

    if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
    {
        BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);
        DWORD bytesNeeded = sizeof(CERT_NAME_VALUE), valueType;

        switch (pbEncoded[0])
        {
        case ASN_NUMERICSTRING:
            valueType = CERT_RDN_NUMERIC_STRING;
            bytesNeeded += dataLen * 2;
            break;
        case ASN_PRINTABLESTRING:
            valueType = CERT_RDN_PRINTABLE_STRING;
            bytesNeeded += dataLen * 2;
            break;
        case ASN_IA5STRING:
            valueType = CERT_RDN_IA5_STRING;
            bytesNeeded += dataLen * 2;
            break;
        case ASN_T61STRING:
            valueType = CERT_RDN_T61_STRING;
            bytesNeeded += dataLen * 2;
            break;
        case ASN_VIDEOTEXSTRING:
            valueType = CERT_RDN_VIDEOTEX_STRING;
            bytesNeeded += dataLen * 2;
            break;
        case ASN_GRAPHICSTRING:
            valueType = CERT_RDN_GRAPHIC_STRING;
            bytesNeeded += dataLen * 2;
            break;
        case ASN_VISIBLESTRING:
            valueType = CERT_RDN_VISIBLE_STRING;
            bytesNeeded += dataLen * 2;
            break;
        case ASN_GENERALSTRING:
            valueType = CERT_RDN_GENERAL_STRING;
            bytesNeeded += dataLen * 2;
            break;
        case ASN_UNIVERSALSTRING:
            valueType = CERT_RDN_UNIVERSAL_STRING;
            bytesNeeded += dataLen / 2;
            break;
        case ASN_BMPSTRING:
            valueType = CERT_RDN_BMP_STRING;
            bytesNeeded += dataLen;
            break;
        case ASN_UTF8STRING:
            valueType = CERT_RDN_UTF8_STRING;
            bytesNeeded += MultiByteToWideChar(CP_UTF8, 0,
             (LPCSTR)pbEncoded + 1 + lenBytes, dataLen, NULL, 0) * 2;
            break;
        default:
            SetLastError(CRYPT_E_ASN1_BADTAG);
            return FALSE;
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
            value->dwValueType = valueType;
            if (dataLen)
            {
                DWORD i;
                LPWSTR str = (LPWSTR)value->Value.pbData;

                assert(value->Value.pbData);
                switch (pbEncoded[0])
                {
                case ASN_NUMERICSTRING:
                case ASN_PRINTABLESTRING:
                case ASN_IA5STRING:
                case ASN_T61STRING:
                case ASN_VIDEOTEXSTRING:
                case ASN_GRAPHICSTRING:
                case ASN_VISIBLESTRING:
                case ASN_GENERALSTRING:
                    value->Value.cbData = dataLen * 2;
                    for (i = 0; i < dataLen; i++)
                        str[i] = pbEncoded[1 + lenBytes + i];
                    break;
                case ASN_UNIVERSALSTRING:
                    value->Value.cbData = dataLen / 2;
                    for (i = 0; i < dataLen / 4; i++)
                        str[i] = (pbEncoded[1 + lenBytes + 2 * i + 2] << 8)
                         | pbEncoded[1 + lenBytes + 2 * i + 3];
                    break;
                case ASN_BMPSTRING:
                    value->Value.cbData = dataLen;
                    for (i = 0; i < dataLen / 2; i++)
                        str[i] = (pbEncoded[1 + lenBytes + 2 * i] << 8) |
                         pbEncoded[1 + lenBytes + 2 * i + 1];
                    break;
                case ASN_UTF8STRING:
                    value->Value.cbData = MultiByteToWideChar(CP_UTF8, 0,
                     (LPCSTR)pbEncoded + 1 + lenBytes, dataLen,
                     str, bytesNeeded - sizeof(CERT_NAME_VALUE)) * 2;
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
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeUnicodeNameValue(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    __TRY
    {
        ret = CRYPT_AsnDecodeUnicodeNameValueInternal(dwCertEncodingType,
         lpszStructType, pbEncoded, cbEncoded,
         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, NULL, pcbStructInfo);
        if (ret && pvStructInfo)
        {
            ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara, pvStructInfo,
             pcbStructInfo, *pcbStructInfo);
            if (ret)
            {
                CERT_NAME_VALUE *value;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                value = (CERT_NAME_VALUE *)pvStructInfo;
                value->Value.pbData = ((BYTE *)value + sizeof(CERT_NAME_VALUE));
                ret = CRYPT_AsnDecodeUnicodeNameValueInternal(
                 dwCertEncodingType, lpszStructType, pbEncoded, cbEncoded,
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

static BOOL WINAPI CRYPT_AsnDecodeRdnAttr(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;
    struct AsnDecodeSequenceItem items[] = {
     { ASN_OBJECTIDENTIFIER, offsetof(CERT_RDN_ATTR, pszObjId),
       CRYPT_AsnDecodeOidInternal, sizeof(LPSTR), FALSE, TRUE,
       offsetof(CERT_RDN_ATTR, pszObjId), 0 },
     { 0, offsetof(CERT_RDN_ATTR, dwValueType),
       CRYPT_AsnDecodeNameValueInternal, sizeof(CERT_NAME_VALUE),
       FALSE, TRUE, offsetof(CERT_RDN_ATTR, Value.pbData), 0 },
    };
    CERT_RDN_ATTR *attr = (CERT_RDN_ATTR *)pvStructInfo;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    if (attr)
        TRACE("attr->pszObjId is %p\n", attr->pszObjId);
    ret = CRYPT_AsnDecodeSequence(X509_ASN_ENCODING, items,
     sizeof(items) / sizeof(items[0]), pbEncoded, cbEncoded, dwFlags, NULL,
     attr, pcbStructInfo, attr ? attr->pszObjId : NULL);
    if (attr)
    {
        TRACE("attr->pszObjId is %p (%s)\n", attr->pszObjId,
         debugstr_a(attr->pszObjId));
        TRACE("attr->dwValueType is %d\n", attr->dwValueType);
    }
    TRACE("returning %d (%08x)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeRdn(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;
    struct AsnArrayDescriptor arrayDesc = { ASN_CONSTRUCTOR | ASN_SETOF,
     CRYPT_AsnDecodeRdnAttr, sizeof(CERT_RDN_ATTR), TRUE,
     offsetof(CERT_RDN_ATTR, pszObjId) };
    PCERT_RDN rdn = (PCERT_RDN)pvStructInfo;

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, pcbStructInfo, rdn ? rdn->rgRDNAttr : NULL);
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

static BOOL WINAPI CRYPT_AsnDecodeUnicodeRdnAttr(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;
    struct AsnDecodeSequenceItem items[] = {
     { ASN_OBJECTIDENTIFIER, offsetof(CERT_RDN_ATTR, pszObjId),
       CRYPT_AsnDecodeOidInternal, sizeof(LPSTR), FALSE, TRUE,
       offsetof(CERT_RDN_ATTR, pszObjId), 0 },
     { 0, offsetof(CERT_RDN_ATTR, dwValueType),
       CRYPT_AsnDecodeUnicodeNameValueInternal, sizeof(CERT_NAME_VALUE),
       FALSE, TRUE, offsetof(CERT_RDN_ATTR, Value.pbData), 0 },
    };
    CERT_RDN_ATTR *attr = (CERT_RDN_ATTR *)pvStructInfo;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    if (attr)
        TRACE("attr->pszObjId is %p\n", attr->pszObjId);
    ret = CRYPT_AsnDecodeSequence(X509_ASN_ENCODING, items,
     sizeof(items) / sizeof(items[0]), pbEncoded, cbEncoded, dwFlags, NULL,
     attr, pcbStructInfo, attr ? attr->pszObjId : NULL);
    if (attr)
    {
        TRACE("attr->pszObjId is %p (%s)\n", attr->pszObjId,
         debugstr_a(attr->pszObjId));
        TRACE("attr->dwValueType is %d\n", attr->dwValueType);
    }
    TRACE("returning %d (%08x)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeUnicodeRdn(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;
    struct AsnArrayDescriptor arrayDesc = { ASN_CONSTRUCTOR | ASN_SETOF,
     CRYPT_AsnDecodeUnicodeRdnAttr, sizeof(CERT_RDN_ATTR), TRUE,
     offsetof(CERT_RDN_ATTR, pszObjId) };
    PCERT_RDN rdn = (PCERT_RDN)pvStructInfo;

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, pcbStructInfo, rdn ? rdn->rgRDNAttr : NULL);
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeUnicodeName(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    __TRY
    {
        struct AsnArrayDescriptor arrayDesc = { ASN_SEQUENCEOF,
         CRYPT_AsnDecodeUnicodeRdn, sizeof(CERT_RDN), TRUE,
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

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
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
       CRYPT_AsnDecodeOidInternal, sizeof(LPSTR), FALSE, TRUE, 
       offsetof(CRYPT_ALGORITHM_IDENTIFIER, pszObjId), 0 },
     { 0, offsetof(CRYPT_ALGORITHM_IDENTIFIER, Parameters),
       CRYPT_AsnDecodeCopyBytes, sizeof(CRYPT_OBJID_BLOB), TRUE, TRUE, 
       offsetof(CRYPT_ALGORITHM_IDENTIFIER, Parameters.pbData), 0 },
    };

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
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

static BOOL WINAPI CRYPT_AsnDecodePubKeyInfoInternal(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;
    struct AsnDecodeSequenceItem items[] = {
     { ASN_SEQUENCEOF, offsetof(CERT_PUBLIC_KEY_INFO, Algorithm),
       CRYPT_AsnDecodeAlgorithmId, sizeof(CRYPT_ALGORITHM_IDENTIFIER),
       FALSE, TRUE, offsetof(CERT_PUBLIC_KEY_INFO,
       Algorithm.pszObjId) },
     { ASN_BITSTRING, offsetof(CERT_PUBLIC_KEY_INFO, PublicKey),
       CRYPT_AsnDecodeBitsInternal, sizeof(CRYPT_BIT_BLOB), FALSE, TRUE,
       offsetof(CERT_PUBLIC_KEY_INFO, PublicKey.pbData) },
    };
    PCERT_PUBLIC_KEY_INFO info = (PCERT_PUBLIC_KEY_INFO)pvStructInfo;

    ret = CRYPT_AsnDecodeSequence(dwCertEncodingType, items,
     sizeof(items) / sizeof(items[0]), pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, pcbStructInfo, info ?
     info->Algorithm.Parameters.pbData : NULL);
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodePubKeyInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    __TRY
    {
        DWORD bytesNeeded;

        if ((ret = CRYPT_AsnDecodePubKeyInfoInternal(dwCertEncodingType,
         lpszStructType, pbEncoded, cbEncoded,
         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, NULL, &bytesNeeded)))
        {
            if (!pvStructInfo)
                *pcbStructInfo = bytesNeeded;
            else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
             pvStructInfo, pcbStructInfo, bytesNeeded)))
            {
                PCERT_PUBLIC_KEY_INFO info;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                info = (PCERT_PUBLIC_KEY_INFO)pvStructInfo;
                info->Algorithm.Parameters.pbData = (BYTE *)pvStructInfo +
                 sizeof(CERT_PUBLIC_KEY_INFO);
                ret = CRYPT_AsnDecodePubKeyInfoInternal(dwCertEncodingType,
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
    TRACE("returning %d (%08x)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeAltNameEntry(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    PCERT_ALT_NAME_ENTRY entry = (PCERT_ALT_NAME_ENTRY)pvStructInfo;
    DWORD dataLen, lenBytes, bytesNeeded = sizeof(CERT_ALT_NAME_ENTRY);
    BOOL ret;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
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

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    if (info)
        TRACE("info->rgAltEntry is %p\n", info->rgAltEntry);
    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, pcbStructInfo, info ? info->rgAltEntry : NULL);
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeAuthorityKeyId(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    __TRY
    {
        struct AsnDecodeSequenceItem items[] = {
         { ASN_CONTEXT | 0, offsetof(CERT_AUTHORITY_KEY_ID_INFO, KeyId),
           CRYPT_AsnDecodeIntegerInternal, sizeof(CRYPT_DATA_BLOB),
           TRUE, TRUE, offsetof(CERT_AUTHORITY_KEY_ID_INFO, KeyId.pbData), 0 },
         { ASN_CONTEXT | ASN_CONSTRUCTOR| 1,
           offsetof(CERT_AUTHORITY_KEY_ID_INFO, CertIssuer),
           CRYPT_AsnDecodeOctetsInternal, sizeof(CERT_NAME_BLOB), TRUE, TRUE,
           offsetof(CERT_AUTHORITY_KEY_ID_INFO, CertIssuer.pbData), 0 },
         { ASN_CONTEXT | 2, offsetof(CERT_AUTHORITY_KEY_ID_INFO,
           CertSerialNumber), CRYPT_AsnDecodeIntegerInternal,
           sizeof(CRYPT_INTEGER_BLOB), TRUE, TRUE,
           offsetof(CERT_AUTHORITY_KEY_ID_INFO, CertSerialNumber.pbData), 0 },
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

static BOOL WINAPI CRYPT_AsnDecodeAltName(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
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

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
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
                TRACE("got an int, dwPathLenConstraint is %d\n",
                 constraint->dwPathLenConstraint);
            }
        }
        else
        {
            SetLastError(CRYPT_E_ASN1_CORRUPT);
            ret = FALSE;
        }
    }
    TRACE("returning %d (%08x)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeSubtreeConstraints(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;
    struct AsnArrayDescriptor arrayDesc = { ASN_SEQUENCEOF,
     CRYPT_AsnDecodeCopyBytes, sizeof(CERT_NAME_BLOB), TRUE,
     offsetof(CERT_NAME_BLOB, pbData) };
    struct GenericArray *entries = (struct GenericArray *)pvStructInfo;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, pcbStructInfo,
     entries ? entries->rgItems : NULL);
    TRACE("Returning %d (%08x)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeBasicConstraints(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    __TRY
    {
        struct AsnDecodeSequenceItem items[] = {
         { ASN_BITSTRING, offsetof(CERT_BASIC_CONSTRAINTS_INFO, SubjectType),
           CRYPT_AsnDecodeBitsInternal, sizeof(CRYPT_BIT_BLOB), FALSE, TRUE, 
           offsetof(CERT_BASIC_CONSTRAINTS_INFO, SubjectType.pbData), 0 },
         { ASN_INTEGER, offsetof(CERT_BASIC_CONSTRAINTS_INFO,
           fPathLenConstraint), CRYPT_AsnDecodePathLenConstraint,
           sizeof(struct PATH_LEN_CONSTRAINT), TRUE, FALSE, 0, 0 },
         { ASN_SEQUENCEOF, offsetof(CERT_BASIC_CONSTRAINTS_INFO,
           cSubtreesConstraint), CRYPT_AsnDecodeSubtreeConstraints,
           sizeof(struct GenericArray), TRUE, TRUE,
           offsetof(CERT_BASIC_CONSTRAINTS_INFO, rgSubtreesConstraint), 0 },
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

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
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

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
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

    TRACE("(%p, %d, 0x%08x, %p, %p, %d)\n", pbEncoded, cbEncoded, dwFlags,
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
    TRACE("returning %d (%08x)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeBits(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    TRACE("(%p, %d, 0x%08x, %p, %p, %p)\n", pbEncoded, cbEncoded, dwFlags,
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
    TRACE("returning %d (%08x)\n", ret, GetLastError());
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
        if (pbEncoded[0] != ASN_INTEGER)
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
            ret = FALSE;
        }
        else
            ret = CRYPT_AsnDecodeIntegerInternal(dwCertEncodingType,
             X509_MULTI_BYTE_INTEGER, pbEncoded, cbEncoded, 0, NULL, &buf,
             &size);
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

        if (pbEncoded[0] != ASN_INTEGER)
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
            ret = FALSE;
        }
        else
            ret = CRYPT_AsnDecodeIntegerInternal(dwCertEncodingType,
             lpszStructType, pbEncoded, cbEncoded,
             dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG, NULL, NULL, &bytesNeeded);
        if (ret)
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

static BOOL WINAPI CRYPT_AsnDecodeDistPointName(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    if (pbEncoded[0] == (ASN_CONTEXT | ASN_CONSTRUCTOR | 0))
    {
        DWORD bytesNeeded, dataLen;

        if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
        {
            struct AsnArrayDescriptor arrayDesc = {
             ASN_CONTEXT | ASN_CONSTRUCTOR | 0, CRYPT_AsnDecodeAltNameEntry,
             sizeof(CERT_ALT_NAME_ENTRY), TRUE,
             offsetof(CERT_ALT_NAME_ENTRY, u.pwszURL) };
            BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

            if (dataLen)
            {
                DWORD nameLen;

                ret = CRYPT_AsnDecodeArray(&arrayDesc,
                 pbEncoded + 1 + lenBytes, cbEncoded - 1 - lenBytes,
                 0, NULL, NULL, &nameLen, NULL);
                bytesNeeded = sizeof(CRL_DIST_POINT_NAME) + nameLen;
            }
            else
                bytesNeeded = sizeof(CRL_DIST_POINT_NAME);
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
                CRL_DIST_POINT_NAME *name = (CRL_DIST_POINT_NAME *)pvStructInfo;

                if (dataLen)
                {
                    name->dwDistPointNameChoice = CRL_DIST_POINT_FULL_NAME;
                    ret = CRYPT_AsnDecodeArray(&arrayDesc,
                     pbEncoded + 1 + lenBytes, cbEncoded - 1 - lenBytes,
                     0, NULL, &name->u.FullName, pcbStructInfo,
                     name->u.FullName.rgAltEntry);
                }
                else
                    name->dwDistPointNameChoice = CRL_DIST_POINT_NO_NAME;
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

static BOOL WINAPI CRYPT_AsnDecodeDistPoint(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    struct AsnDecodeSequenceItem items[] = {
     { ASN_CONTEXT | ASN_CONSTRUCTOR | 0, offsetof(CRL_DIST_POINT,
       DistPointName), CRYPT_AsnDecodeDistPointName,
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

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
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

static BOOL WINAPI CRYPT_AsnDecodeEnhancedKeyUsage(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    __TRY
    {
        struct AsnArrayDescriptor arrayDesc = { ASN_SEQUENCEOF,
         CRYPT_AsnDecodeOidInternal, sizeof(LPSTR), TRUE, 0 };

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

static BOOL WINAPI CRYPT_AsnDecodeIssuingDistPoint(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    __TRY
    {
        struct AsnDecodeSequenceItem items[] = {
         { ASN_CONTEXT | ASN_CONSTRUCTOR | 0, offsetof(CRL_ISSUING_DIST_POINT,
           DistPointName), CRYPT_AsnDecodeDistPointName,
           sizeof(CRL_DIST_POINT_NAME), TRUE, TRUE,
           offsetof(CRL_ISSUING_DIST_POINT,
           DistPointName.u.FullName.rgAltEntry), 0 },
         { ASN_CONTEXT | 1, offsetof(CRL_ISSUING_DIST_POINT,
           fOnlyContainsUserCerts), CRYPT_AsnDecodeBool, sizeof(BOOL), TRUE,
           FALSE, 0 },
         { ASN_CONTEXT | 2, offsetof(CRL_ISSUING_DIST_POINT,
           fOnlyContainsCACerts), CRYPT_AsnDecodeBool, sizeof(BOOL), TRUE,
           FALSE, 0 },
         { ASN_CONTEXT | 3, offsetof(CRL_ISSUING_DIST_POINT,
           OnlySomeReasonFlags), CRYPT_AsnDecodeBitsInternal,
           sizeof(CRYPT_BIT_BLOB), TRUE, TRUE, offsetof(CRL_ISSUING_DIST_POINT,
           OnlySomeReasonFlags.pbData), 0 },
         { ASN_CONTEXT | 4, offsetof(CRL_ISSUING_DIST_POINT,
           fIndirectCRL), CRYPT_AsnDecodeBool, sizeof(BOOL), TRUE, FALSE, 0 },
        };

        ret = CRYPT_AsnDecodeSequence(dwCertEncodingType, items,
         sizeof(items) / sizeof(items[0]), pbEncoded, cbEncoded,
         dwFlags, pDecodePara, pvStructInfo, pcbStructInfo, NULL);
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

    TRACE("(0x%08x, %s, %p, %d, 0x%08x, %p, %p, %p)\n",
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
            decodeFunc = CRYPT_AsnDecodeCertSignedContent;
            break;
        case (WORD)X509_CERT_TO_BE_SIGNED:
            decodeFunc = CRYPT_AsnDecodeCert;
            break;
        case (WORD)X509_CERT_CRL_TO_BE_SIGNED:
            decodeFunc = CRYPT_AsnDecodeCRL;
            break;
        case (WORD)X509_EXTENSIONS:
            decodeFunc = CRYPT_AsnDecodeExtensions;
            break;
        case (WORD)X509_NAME_VALUE:
            decodeFunc = CRYPT_AsnDecodeNameValue;
            break;
        case (WORD)X509_NAME:
            decodeFunc = CRYPT_AsnDecodeName;
            break;
        case (WORD)X509_PUBLIC_KEY_INFO:
            decodeFunc = CRYPT_AsnDecodePubKeyInfo;
            break;
        case (WORD)X509_AUTHORITY_KEY_ID:
            decodeFunc = CRYPT_AsnDecodeAuthorityKeyId;
            break;
        case (WORD)X509_ALTERNATE_NAME:
            decodeFunc = CRYPT_AsnDecodeAltName;
            break;
        case (WORD)X509_BASIC_CONSTRAINTS:
            decodeFunc = CRYPT_AsnDecodeBasicConstraints;
            break;
        case (WORD)X509_BASIC_CONSTRAINTS2:
            decodeFunc = CRYPT_AsnDecodeBasicConstraints2;
            break;
        case (WORD)RSA_CSP_PUBLICKEYBLOB:
            decodeFunc = CRYPT_AsnDecodeRsaPubKey;
            break;
        case (WORD)X509_UNICODE_NAME:
            decodeFunc = CRYPT_AsnDecodeUnicodeName;
            break;
        case (WORD)X509_UNICODE_NAME_VALUE:
            decodeFunc = CRYPT_AsnDecodeUnicodeNameValue;
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
        case (WORD)X509_ENHANCED_KEY_USAGE:
            decodeFunc = CRYPT_AsnDecodeEnhancedKeyUsage;
            break;
        case (WORD)X509_ISSUING_DIST_POINT:
            decodeFunc = CRYPT_AsnDecodeIssuingDistPoint;
            break;
        default:
            FIXME("%d: unimplemented\n", LOWORD(lpszStructType));
        }
    }
    else if (!strcmp(lpszStructType, szOID_CERT_EXTENSIONS))
        decodeFunc = CRYPT_AsnDecodeExtensions;
    else if (!strcmp(lpszStructType, szOID_RSA_signingTime))
        decodeFunc = CRYPT_AsnDecodeUtcTime;
    else if (!strcmp(lpszStructType, szOID_AUTHORITY_KEY_IDENTIFIER))
        decodeFunc = CRYPT_AsnDecodeAuthorityKeyId;
    else if (!strcmp(lpszStructType, szOID_CRL_REASON_CODE))
        decodeFunc = CRYPT_AsnDecodeEnumerated;
    else if (!strcmp(lpszStructType, szOID_KEY_USAGE))
        decodeFunc = CRYPT_AsnDecodeBits;
    else if (!strcmp(lpszStructType, szOID_SUBJECT_KEY_IDENTIFIER))
        decodeFunc = CRYPT_AsnDecodeOctets;
    else if (!strcmp(lpszStructType, szOID_BASIC_CONSTRAINTS))
        decodeFunc = CRYPT_AsnDecodeBasicConstraints;
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
    else if (!strcmp(lpszStructType, szOID_CRL_DIST_POINTS))
        decodeFunc = CRYPT_AsnDecodeCRLDistPoints;
    else if (!strcmp(lpszStructType, szOID_ENHANCED_KEY_USAGE))
        decodeFunc = CRYPT_AsnDecodeEnhancedKeyUsage;
    else if (!strcmp(lpszStructType, szOID_ISSUING_DIST_POINT))
        decodeFunc = CRYPT_AsnDecodeIssuingDistPoint;
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
