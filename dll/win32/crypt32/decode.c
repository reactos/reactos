/*
 * Copyright 2005-2009 Juan Lang
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

#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "winnls.h"
#include "snmp.h"
#include "wine/debug.h"
#include "wine/exception.h"
#include "crypt32_private.h"

/* This is a bit arbitrary, but to set some limit: */
#define MAX_ENCODED_LEN 0x02000000

#define ASN_FLAGS_MASK 0xe0
#define ASN_TYPE_MASK  0x1f

WINE_DEFAULT_DEBUG_CHANNEL(cryptasn);
WINE_DECLARE_DEBUG_CHANNEL(crypt);

typedef BOOL (WINAPI *CryptDecodeObjectFunc)(DWORD, LPCSTR, const BYTE *,
 DWORD, DWORD, void *, DWORD *);
typedef BOOL (WINAPI *CryptDecodeObjectExFunc)(DWORD, LPCSTR, const BYTE *,
 DWORD, DWORD, PCRYPT_DECODE_PARA, void *, DWORD *);

/* Internal decoders don't do memory allocation or exception handling, and
 * they report how many bytes they decoded.
 */
typedef BOOL (*InternalDecodeFunc)(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded);

static BOOL CRYPT_AsnDecodeChoiceOfTimeInternal(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded);
static BOOL CRYPT_AsnDecodePubKeyInfoInternal(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded);
/* Assumes pvStructInfo is a CERT_EXTENSION whose pszObjId is set ahead of time.
 */
static BOOL CRYPT_AsnDecodeExtension(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded);
/* Assumes algo->Parameters.pbData is set ahead of time. */
static BOOL CRYPT_AsnDecodeAlgorithmId(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded);
static BOOL CRYPT_AsnDecodeBool(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded);
/* Assumes the CRYPT_DATA_BLOB's pbData member has been initialized */
static BOOL CRYPT_AsnDecodeOctets(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded);
/* Doesn't check the tag, assumes the caller does so */
static BOOL CRYPT_AsnDecodeBitsInternal(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded);
static BOOL CRYPT_AsnDecodeIntInternal(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded);
/* Like CRYPT_AsnDecodeInteger, but assumes the CRYPT_INTEGER_BLOB's pbData
 * member has been initialized, doesn't do exception handling, and doesn't do
 * memory allocation.  Also doesn't check tag, assumes the caller has checked
 * it.
 */
static BOOL CRYPT_AsnDecodeIntegerInternal(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded);
/* Like CRYPT_AsnDecodeInteger, but unsigned.  */
static BOOL CRYPT_AsnDecodeUnsignedIntegerInternal(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded);
static BOOL CRYPT_AsnDecodePKCSAttributeInternal(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded);

/* Gets the number of length bytes from the given (leading) length byte */
#define GET_LEN_BYTES(b) ((b) <= 0x80 ? 1 : 1 + ((b) & 0x7f))

/* Helper function to get the encoded length of the data starting at pbEncoded,
 * where pbEncoded[0] is the tag.  If the data are too short to contain a
 * length or if the length is too large for cbEncoded, sets an appropriate
 * error code and returns FALSE.  If the encoded length is unknown due to
 * indefinite length encoding, *len is set to CMSG_INDEFINITE_LENGTH.
 */
static BOOL CRYPT_GetLengthIndefinite(const BYTE *pbEncoded, DWORD cbEncoded,
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
    else if (pbEncoded[1] == 0x80)
    {
        *len = CMSG_INDEFINITE_LENGTH;
        ret = TRUE;
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

/* Like CRYPT_GetLengthIndefinite, but disallows indefinite-length encoding. */
static BOOL CRYPT_GetLen(const BYTE *pbEncoded, DWORD cbEncoded, DWORD *len)
{
    BOOL ret;

    if ((ret = CRYPT_GetLengthIndefinite(pbEncoded, cbEncoded, len)) &&
     *len == CMSG_INDEFINITE_LENGTH)
    {
        SetLastError(CRYPT_E_ASN1_CORRUPT);
        ret = FALSE;
    }
    return ret;
}

/* Helper function to check *pcbStructInfo, set it to the required size, and
 * optionally to allocate memory.  Assumes pvStructInfo is not NULL.
 * If CRYPT_DECODE_ALLOC_FLAG is set in dwFlags, *pvStructInfo will be set to a
 * pointer to the newly allocated memory.
 */
static BOOL CRYPT_DecodeEnsureSpace(DWORD dwFlags,
 const CRYPT_DECODE_PARA *pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD bytesNeeded)
{
    BOOL ret = TRUE;

    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
    {
        if (pDecodePara && pDecodePara->pfnAlloc)
            *(BYTE **)pvStructInfo = pDecodePara->pfnAlloc(bytesNeeded);
        else
            *(BYTE **)pvStructInfo = LocalAlloc(LPTR, bytesNeeded);
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
    else
        *pcbStructInfo = bytesNeeded;
    return ret;
}

static void CRYPT_FreeSpace(const CRYPT_DECODE_PARA *pDecodePara, LPVOID pv)
{
    if (pDecodePara && pDecodePara->pfnFree)
        pDecodePara->pfnFree(pv);
    else
        LocalFree(pv);
}

/* Helper function to check *pcbStructInfo and set it to the required size.
 * Assumes pvStructInfo is not NULL.
 */
static BOOL CRYPT_DecodeCheckSpace(DWORD *pcbStructInfo, DWORD bytesNeeded)
{
    BOOL ret;

    if (*pcbStructInfo < bytesNeeded)
    {
        *pcbStructInfo = bytesNeeded;
        SetLastError(ERROR_MORE_DATA);
        ret = FALSE;
    }
    else
    {
        *pcbStructInfo = bytesNeeded;
        ret = TRUE;
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
 * hasPointer, pointerOffset:
 *     If the item has dynamic data, set hasPointer to TRUE, pointerOffset to
 *     the offset within the struct of the data pointer (or to the
 *     first data pointer, if more than one exist).
 * size:
 *     Used by CRYPT_AsnDecodeSequence, not for your use.
 */
struct AsnDecodeSequenceItem
{
    BYTE               tag;
    DWORD              offset;
    InternalDecodeFunc decodeFunc;
    DWORD              minSize;
    BOOL               optional;
    BOOL               hasPointer;
    DWORD              pointerOffset;
    DWORD              size;
};

#define FINALMEMBERSIZE(s, member) (sizeof(s) - offsetof(s, member))
#define MEMBERSIZE(s, member, nextmember) \
    (offsetof(s, nextmember) - offsetof(s, member))

/* Decodes the items in a sequence, where the items are described in items,
 * the encoded data are in pbEncoded with length cbEncoded.  Decodes into
 * pvStructInfo.  nextData is a pointer to the memory location at which the
 * first decoded item with a dynamic pointer should point.
 * Upon decoding, *cbDecoded is the total number of bytes decoded.
 * Each item decoder is never called with CRYPT_DECODE_ALLOC_FLAG set.
 */
static BOOL CRYPT_AsnDecodeSequenceItems(struct AsnDecodeSequenceItem items[],
 DWORD cItem, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 void *pvStructInfo, BYTE *nextData, DWORD *cbDecoded)
{
    BOOL ret;
    DWORD i, decoded = 0;
    const BYTE *ptr = pbEncoded;

    TRACE("%p, %d, %p, %d, %08x, %p, %p, %p\n", items, cItem, pbEncoded,
     cbEncoded, dwFlags, pvStructInfo, nextData, cbDecoded);

    for (i = 0, ret = TRUE; ret && i < cItem; i++)
    {
        if (cbEncoded - (ptr - pbEncoded) != 0)
        {
            DWORD itemLen;

            if ((ret = CRYPT_GetLengthIndefinite(ptr,
             cbEncoded - (ptr - pbEncoded), &itemLen)))
            {
                BYTE itemLenBytes = GET_LEN_BYTES(ptr[1]);

                if (ptr[0] == items[i].tag || !items[i].tag)
                {
                    DWORD itemEncodedLen;

                    if (itemLen == CMSG_INDEFINITE_LENGTH)
                        itemEncodedLen = cbEncoded - (ptr - pbEncoded);
                    else
                        itemEncodedLen = 1 + itemLenBytes + itemLen;
                    if (nextData && pvStructInfo && items[i].hasPointer)
                    {
                        TRACE("Setting next pointer to %p\n",
                         nextData);
                        *(BYTE **)((BYTE *)pvStructInfo +
                         items[i].pointerOffset) = nextData;
                    }
                    if (items[i].decodeFunc)
                    {
                        DWORD itemDecoded;

                        if (pvStructInfo)
                            TRACE("decoding item %d\n", i);
                        else
                            TRACE("sizing item %d\n", i);
                        ret = items[i].decodeFunc(ptr, itemEncodedLen,
                         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG,
                         pvStructInfo ?  (BYTE *)pvStructInfo + items[i].offset
                         : NULL, &items[i].size, &itemDecoded);
                        if (ret)
                        {
                            if (items[i].size < items[i].minSize)
                                items[i].size = items[i].minSize;
                            else if (items[i].size > items[i].minSize)
                            {
                                /* Account for alignment padding */
                                items[i].size = ALIGN_DWORD_PTR(items[i].size);
                            }
                            TRACE("item %d size: %d\n", i, items[i].size);
                            if (nextData && items[i].hasPointer &&
                             items[i].size > items[i].minSize)
                                nextData += items[i].size - items[i].minSize;
                            if (itemDecoded > itemEncodedLen)
                            {
                                WARN("decoded length %d exceeds encoded %d\n",
                                 itemDecoded, itemEncodedLen);
                                SetLastError(CRYPT_E_ASN1_CORRUPT);
                                ret = FALSE;
                            }
                            else
                            {
                                ptr += itemDecoded;
                                decoded += itemDecoded;
                                TRACE("item %d: decoded %d bytes\n", i,
                                 itemDecoded);
                            }
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
                    else if (itemLen == CMSG_INDEFINITE_LENGTH)
                    {
                        ERR("can't use indefinite length encoding without a decoder\n");
                        SetLastError(CRYPT_E_ASN1_CORRUPT);
                        ret = FALSE;
                    }
                    else
                    {
                        TRACE("item %d: decoded %d bytes\n", i, itemEncodedLen);
                        ptr += itemEncodedLen;
                        decoded += itemEncodedLen;
                        items[i].size = items[i].minSize;
                    }
                }
                else if (items[i].optional)
                {
                    TRACE("skipping optional item %d\n", i);
                    items[i].size = items[i].minSize;
                }
                else
                {
                    TRACE("item %d: tag %02x doesn't match expected %02x\n",
                     i, ptr[0], items[i].tag);
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
    if (cbDecoded)
        *cbDecoded = decoded;
    TRACE("returning %d\n", ret);
    return ret;
}

/* This decodes an arbitrary sequence into a contiguous block of memory
 * (basically, a struct.)  Each element being decoded is described by a struct
 * AsnDecodeSequenceItem, see above.
 * startingPointer is an optional pointer to the first place where dynamic
 * data will be stored.  If you know the starting offset, you may pass it
 * here.  Otherwise, pass NULL, and one will be inferred from the items.
 */
static BOOL CRYPT_AsnDecodeSequence(struct AsnDecodeSequenceItem items[],
 DWORD cItem, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded, void *startingPointer)
{
    BOOL ret;

    TRACE("%p, %d, %p, %d, %08x, %p, %p, %d, %p\n", items, cItem, pbEncoded,
     cbEncoded, dwFlags, pDecodePara, pvStructInfo, *pcbStructInfo,
     startingPointer);

    if (!cbEncoded)
    {
        SetLastError(CRYPT_E_ASN1_EOD);
        return FALSE;
    }
    if (pbEncoded[0] == ASN_SEQUENCE)
    {
        DWORD dataLen;

        if ((ret = CRYPT_GetLengthIndefinite(pbEncoded, cbEncoded, &dataLen)))
        {
            DWORD lenBytes = GET_LEN_BYTES(pbEncoded[1]), cbDecoded;
            const BYTE *ptr = pbEncoded + 1 + lenBytes;
            BOOL indefinite = FALSE;

            cbEncoded -= 1 + lenBytes;
            if (dataLen == CMSG_INDEFINITE_LENGTH)
            {
                dataLen = cbEncoded;
                indefinite = TRUE;
                lenBytes += 2;
            }
            else if (cbEncoded < dataLen)
            {
                TRACE("dataLen %d exceeds cbEncoded %d, failing\n", dataLen,
                 cbEncoded);
                SetLastError(CRYPT_E_ASN1_CORRUPT);
                ret = FALSE;
            }
            if (ret)
            {
                ret = CRYPT_AsnDecodeSequenceItems(items, cItem,
                 ptr, dataLen, dwFlags, NULL, NULL, &cbDecoded);
                if (ret && dataLen == CMSG_INDEFINITE_LENGTH)
                {
                    if (cbDecoded > cbEncoded - 2)
                    {
                        /* Not enough space for 0 TLV */
                        SetLastError(CRYPT_E_ASN1_CORRUPT);
                        ret = FALSE;
                    }
                    else if (*(ptr + cbDecoded) != 0 ||
                     *(ptr + cbDecoded + 1) != 0)
                    {
                        TRACE("expected 0 TLV\n");
                        SetLastError(CRYPT_E_ASN1_CORRUPT);
                        ret = FALSE;
                    }
                    else
                        cbDecoded += 2;
                }
            }
            if (ret && !indefinite && cbDecoded != dataLen)
            {
                TRACE("expected %d decoded, got %d, failing\n", dataLen,
                 cbDecoded);
                SetLastError(CRYPT_E_ASN1_CORRUPT);
                ret = FALSE;
            }
            if (ret)
            {
                DWORD i, bytesNeeded = 0, structSize = 0;

                for (i = 0; i < cItem; i++)
                {
                    if (items[i].size > items[i].minSize)
                        bytesNeeded += items[i].size - items[i].minSize;
                    structSize = max( structSize, items[i].offset + items[i].minSize );
                }
                bytesNeeded += structSize;
                if (pcbDecoded)
                    *pcbDecoded = 1 + lenBytes + cbDecoded;
                if (!pvStructInfo)
                    *pcbStructInfo = bytesNeeded;
                else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags,
                 pDecodePara, pvStructInfo, pcbStructInfo, bytesNeeded)))
                {
                    BYTE *nextData;

                    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                        pvStructInfo = *(BYTE **)pvStructInfo;
                    if (startingPointer)
                        nextData = startingPointer;
                    else
                        nextData = (BYTE *)pvStructInfo + structSize;
                    memset(pvStructInfo, 0, structSize);
                    ret = CRYPT_AsnDecodeSequenceItems(items, cItem,
                     ptr, dataLen, dwFlags, pvStructInfo, nextData,
                     &cbDecoded);
                    if (!ret && (dwFlags & CRYPT_DECODE_ALLOC_FLAG))
                        CRYPT_FreeSpace(pDecodePara, pvStructInfo);
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
 *     of ASN_SETOF or ASN_SEQUENCEOF.)  If tag is 0, decodeFunc is called
 *     regardless of the tag seen.
 * countOffset:
 *     The offset within the outer structure at which the count exists.
 *     For example, a structure such as CRYPT_ATTRIBUTES has countOffset == 0,
 *     while CRYPT_ATTRIBUTE has countOffset ==
 *     offsetof(CRYPT_ATTRIBUTE, cValue).
 * arrayOffset:
 *     The offset within the outer structure at which the array pointer exists.
 *     For example, CRYPT_ATTRIBUTES has arrayOffset ==
 *     offsetof(CRYPT_ATTRIBUTES, rgAttr).
 * minArraySize:
 *     The minimum size of the decoded array.  On WIN32, this is always 8:
 *     sizeof(DWORD) + sizeof(void *).  On WIN64, it can be larger due to
 *     alignment.
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
    BYTE               tag;
    DWORD              countOffset;
    DWORD              arrayOffset;
    DWORD              minArraySize;
    InternalDecodeFunc decodeFunc;
    DWORD              itemSize;
    BOOL               hasPointer;
    DWORD              pointerOffset;
};

struct AsnArrayItemSize
{
    DWORD encodedLen;
    DWORD size;
};

/* Decodes an array of like types into a structure described by a struct
 * AsnArrayDescriptor.
 */
static BOOL CRYPT_AsnDecodeArray(const struct AsnArrayDescriptor *arrayDesc,
 const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 const CRYPT_DECODE_PARA *pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret = TRUE;

    TRACE("%p, %p, %d, %p, %d\n", arrayDesc, pbEncoded,
     cbEncoded, pvStructInfo, pvStructInfo ? *pcbStructInfo : 0);

    if (!cbEncoded)
    {
        SetLastError(CRYPT_E_ASN1_EOD);
        ret = FALSE;
    }
    else if (!arrayDesc->tag || pbEncoded[0] == arrayDesc->tag)
    {
        DWORD dataLen;

        if ((ret = CRYPT_GetLengthIndefinite(pbEncoded, cbEncoded, &dataLen)))
        {
            DWORD bytesNeeded = arrayDesc->minArraySize, cItems = 0, decoded;
            BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);
            /* There can be arbitrarily many items, but there is often only one.
             */
            struct AsnArrayItemSize itemSize = { 0 }, *itemSizes = &itemSize;

            decoded = 1 + lenBytes;
            if (dataLen)
            {
                const BYTE *ptr;
                BOOL doneDecoding = FALSE;

                for (ptr = pbEncoded + 1 + lenBytes; ret && !doneDecoding; )
                {
                    if (dataLen == CMSG_INDEFINITE_LENGTH)
                    {
                        if (ptr[0] == 0)
                        {
                            doneDecoding = TRUE;
                            if (ptr[1] != 0)
                            {
                                SetLastError(CRYPT_E_ASN1_CORRUPT);
                                ret = FALSE;
                            }
                            else
                                decoded += 2;
                        }
                    }
                    else if (ptr - pbEncoded - 1 - lenBytes >= dataLen)
                        doneDecoding = TRUE;
                    if (!doneDecoding)
                    {
                        DWORD itemEncoded, itemDataLen, itemDecoded, size = 0;

                        /* Each item decoded may not tolerate extraneous bytes,
                         * so get the length of the next element if known.
                         */
                        if ((ret = CRYPT_GetLengthIndefinite(ptr,
                         cbEncoded - (ptr - pbEncoded), &itemDataLen)))
                        {
                            if (itemDataLen == CMSG_INDEFINITE_LENGTH)
                                itemEncoded = cbEncoded - (ptr - pbEncoded);
                            else
                                itemEncoded = 1 + GET_LEN_BYTES(ptr[1]) +
                                 itemDataLen;
                        }
                        if (ret)
                            ret = arrayDesc->decodeFunc(ptr, itemEncoded,
                             dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, &size,
                             &itemDecoded);
                        if (ret)
                        {
                            /* Ignore an item that failed to decode but the decoder doesn't want to fail the whole process */
                            if (!size)
                            {
                                ptr += itemEncoded;
                                continue;
                            }

                            cItems++;
                            if (itemSizes != &itemSize)
                                itemSizes = CryptMemRealloc(itemSizes,
                                 cItems * sizeof(struct AsnArrayItemSize));
                            else if (cItems > 1)
                            {
                                itemSizes =
                                 CryptMemAlloc(
                                 cItems * sizeof(struct AsnArrayItemSize));
                                if (itemSizes)
                                    *itemSizes = itemSize;
                            }
                            if (itemSizes)
                            {
                                decoded += itemDecoded;
                                itemSizes[cItems - 1].encodedLen = itemEncoded;
                                itemSizes[cItems - 1].size = size;
                                bytesNeeded += size;
                                ptr += itemEncoded;
                            }
                            else
                                ret = FALSE;
                        }
                    }
                }
            }
            if (ret)
            {
                if (pcbDecoded)
                    *pcbDecoded = decoded;
                if (!pvStructInfo)
                    *pcbStructInfo = bytesNeeded;
                else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
                 pvStructInfo, pcbStructInfo, bytesNeeded)))
                {
                    DWORD i, *pcItems;
                    BYTE *nextData;
                    const BYTE *ptr;
                    void *rgItems;

                    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                        pvStructInfo = *(void **)pvStructInfo;
                    pcItems = pvStructInfo;
                    *pcItems = cItems;
                    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    {
                        rgItems = (BYTE *)pvStructInfo +
                         arrayDesc->minArraySize;
                        *(void **)((BYTE *)pcItems -
                         arrayDesc->countOffset + arrayDesc->arrayOffset) =
                         rgItems;
                    }
                    else
                        rgItems = *(void **)((BYTE *)pcItems -
                         arrayDesc->countOffset + arrayDesc->arrayOffset);
                    nextData = (BYTE *)rgItems + cItems * arrayDesc->itemSize;
                    for (i = 0, ptr = pbEncoded + 1 + lenBytes; ret &&
                     i < cItems && ptr - pbEncoded - 1 - lenBytes <
                     dataLen; i++)
                    {
                        DWORD itemDecoded;

                        if (arrayDesc->hasPointer)
                            *(BYTE **)((BYTE *)rgItems + i * arrayDesc->itemSize
                             + arrayDesc->pointerOffset) = nextData;
                        ret = arrayDesc->decodeFunc(ptr,
                         itemSizes[i].encodedLen,
                         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG,
                         (BYTE *)rgItems + i * arrayDesc->itemSize,
                         &itemSizes[i].size, &itemDecoded);
                        if (ret)
                        {
                            nextData += itemSizes[i].size - arrayDesc->itemSize;
                            ptr += itemDecoded;
                        }
                    }
                    if (!ret && (dwFlags & CRYPT_DECODE_ALLOC_FLAG))
                        CRYPT_FreeSpace(pDecodePara, pvStructInfo);
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
static BOOL CRYPT_AsnDecodeDerBlob(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded)
{
    BOOL ret;
    DWORD dataLen;

    if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
    {
        BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);
        DWORD bytesNeeded = sizeof(CRYPT_DER_BLOB);
       
        if (!(dwFlags & CRYPT_DECODE_NOCOPY_FLAG))
            bytesNeeded += 1 + lenBytes + dataLen;

        if (pcbDecoded)
            *pcbDecoded = 1 + lenBytes + dataLen;
        if (!pvStructInfo)
            *pcbStructInfo = bytesNeeded;
        else if ((ret = CRYPT_DecodeCheckSpace(pcbStructInfo, bytesNeeded)))
        {
            CRYPT_DER_BLOB *blob;

            if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                pvStructInfo = *(BYTE **)pvStructInfo;
            blob = pvStructInfo;
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
static BOOL CRYPT_AsnDecodeBitsSwapBytes(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;

    TRACE("(%p, %d, 0x%08x, %p, %d, %p)\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo, pcbDecoded);

    /* Can't use the CRYPT_DECODE_NOCOPY_FLAG, because we modify the bytes in-
     * place.
     */
    ret = CRYPT_AsnDecodeBitsInternal(pbEncoded, cbEncoded,
     dwFlags & ~CRYPT_DECODE_NOCOPY_FLAG, pvStructInfo, pcbStructInfo,
     pcbDecoded);
    if (ret && pvStructInfo)
    {
        CRYPT_BIT_BLOB *blob = pvStructInfo;

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
        ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
         pbEncoded, cbEncoded, dwFlags, pDecodePara, pvStructInfo,
         pcbStructInfo, NULL, NULL);
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

static BOOL CRYPT_AsnDecodeCertVersion(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded)
{
    BOOL ret;
    DWORD dataLen;

    if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
    {
        BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

        ret = CRYPT_AsnDecodeIntInternal(pbEncoded + 1 + lenBytes, dataLen,
         dwFlags, pvStructInfo, pcbStructInfo, NULL);
        if (pcbDecoded)
            *pcbDecoded = 1 + lenBytes + dataLen;
    }
    return ret;
}

static BOOL CRYPT_AsnDecodeValidity(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded)
{
    BOOL ret;

    struct AsnDecodeSequenceItem items[] = {
     { 0, offsetof(CERT_PRIVATE_KEY_VALIDITY, NotBefore),
       CRYPT_AsnDecodeChoiceOfTimeInternal, sizeof(FILETIME), FALSE, FALSE, 0 },
     { 0, offsetof(CERT_PRIVATE_KEY_VALIDITY, NotAfter),
       CRYPT_AsnDecodeChoiceOfTimeInternal, sizeof(FILETIME), FALSE, FALSE, 0 },
    };

    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, NULL, pvStructInfo, pcbStructInfo,
     pcbDecoded, NULL);
    return ret;
}

static BOOL CRYPT_AsnDecodeCertExtensionsInternal(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret = TRUE;
    struct AsnArrayDescriptor arrayDesc = { ASN_SEQUENCEOF,
     offsetof(CERT_INFO, cExtension), offsetof(CERT_INFO, rgExtension),
     FINALMEMBERSIZE(CERT_INFO, cExtension),
     CRYPT_AsnDecodeExtension, sizeof(CERT_EXTENSION), TRUE,
     offsetof(CERT_EXTENSION, pszObjId) };

    TRACE("%p, %d, %08x, %p, %d, %p\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo, pcbDecoded);

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
     dwFlags, NULL, pvStructInfo, pcbStructInfo, pcbDecoded);
    return ret;
}

static BOOL CRYPT_AsnDecodeCertExtensions(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;
    DWORD dataLen;

    if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
    {
        BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

        ret = CRYPT_AsnDecodeCertExtensionsInternal(pbEncoded + 1 + lenBytes,
         dataLen, dwFlags, pvStructInfo, pcbStructInfo, NULL);
        if (ret && pcbDecoded)
            *pcbDecoded = 1 + lenBytes + dataLen;
    }
    return ret;
}

static BOOL CRYPT_AsnDecodeCertInfo(DWORD dwCertEncodingType,
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
     { ASN_CONTEXT | 1, offsetof(CERT_INFO, IssuerUniqueId),
       CRYPT_AsnDecodeBitsInternal, sizeof(CRYPT_BIT_BLOB), TRUE, TRUE,
       offsetof(CERT_INFO, IssuerUniqueId.pbData), 0 },
     { ASN_CONTEXT | 2, offsetof(CERT_INFO, SubjectUniqueId),
       CRYPT_AsnDecodeBitsInternal, sizeof(CRYPT_BIT_BLOB), TRUE, TRUE,
       offsetof(CERT_INFO, SubjectUniqueId.pbData), 0 },
     { ASN_CONTEXT | ASN_CONSTRUCTOR | 3, offsetof(CERT_INFO, cExtension),
       CRYPT_AsnDecodeCertExtensions, FINALMEMBERSIZE(CERT_INFO, cExtension),
       TRUE, TRUE, offsetof(CERT_INFO, rgExtension), 0 },
    };

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, pDecodePara, pvStructInfo, pcbStructInfo,
     NULL, NULL);
    if (ret && pvStructInfo)
    {
        CERT_INFO *info;

        if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
            info = *(CERT_INFO **)pvStructInfo;
        else
            info = pvStructInfo;
        if (!info->SerialNumber.cbData || !info->Issuer.cbData ||
         !info->Subject.cbData)
        {
            SetLastError(CRYPT_E_ASN1_CORRUPT);
            /* Don't need to deallocate, because it should have failed on the
             * first pass (and no memory was allocated.)
             */
            ret = FALSE;
        }
    }

    TRACE("Returning %d (%08x)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeCert(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    __TRY
    {
        DWORD size = 0;

        /* Unless told not to, first try to decode it as a signed cert. */
        if (!(dwFlags & CRYPT_DECODE_TO_BE_SIGNED_FLAG))
        {
            PCERT_SIGNED_CONTENT_INFO signedCert = NULL;

            ret = CRYPT_AsnDecodeCertSignedContent(dwCertEncodingType,
             X509_CERT, pbEncoded, cbEncoded, CRYPT_DECODE_ALLOC_FLAG, NULL,
             &signedCert, &size);
            if (ret)
            {
                size = 0;
                ret = CRYPT_AsnDecodeCertInfo(dwCertEncodingType,
                 X509_CERT_TO_BE_SIGNED, signedCert->ToBeSigned.pbData,
                 signedCert->ToBeSigned.cbData, dwFlags, pDecodePara,
                 pvStructInfo, pcbStructInfo);
                LocalFree(signedCert);
            }
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
    }
    __ENDTRY

    TRACE("Returning %d (%08x)\n", ret, GetLastError());
    return ret;
}

static BOOL CRYPT_AsnDecodeCRLEntryExtensions(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret = TRUE;
    struct AsnArrayDescriptor arrayDesc = { ASN_SEQUENCEOF,
     offsetof(CRL_ENTRY, cExtension), offsetof(CRL_ENTRY, rgExtension),
     FINALMEMBERSIZE(CRL_ENTRY, cExtension),
     CRYPT_AsnDecodeExtension, sizeof(CERT_EXTENSION), TRUE,
     offsetof(CERT_EXTENSION, pszObjId) };

    TRACE("%p, %d, %08x, %p, %d, %p\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo, pcbDecoded);

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
     dwFlags, NULL, pvStructInfo, pcbStructInfo, pcbDecoded);
    return ret;
}

static BOOL CRYPT_AsnDecodeCRLEntry(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded)
{
    BOOL ret;
    struct AsnDecodeSequenceItem items[] = {
     { ASN_INTEGER, offsetof(CRL_ENTRY, SerialNumber),
       CRYPT_AsnDecodeIntegerInternal, sizeof(CRYPT_INTEGER_BLOB), FALSE, TRUE,
       offsetof(CRL_ENTRY, SerialNumber.pbData), 0 },
     { 0, offsetof(CRL_ENTRY, RevocationDate),
       CRYPT_AsnDecodeChoiceOfTimeInternal, sizeof(FILETIME), FALSE, FALSE, 0 },
     { ASN_SEQUENCEOF, offsetof(CRL_ENTRY, cExtension),
       CRYPT_AsnDecodeCRLEntryExtensions,
       FINALMEMBERSIZE(CRL_ENTRY, cExtension), TRUE, TRUE,
       offsetof(CRL_ENTRY, rgExtension), 0 },
    };
    PCRL_ENTRY entry = pvStructInfo;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags, entry,
     *pcbStructInfo);

    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, NULL, entry, pcbStructInfo, pcbDecoded,
     entry ? entry->SerialNumber.pbData : NULL);
    if (ret && entry && !entry->SerialNumber.cbData)
    {
        WARN("empty CRL entry serial number\n");
        SetLastError(CRYPT_E_ASN1_CORRUPT);
        ret = FALSE;
    }
    return ret;
}

/* Warning: assumes pvStructInfo points to the cCRLEntry member of a CRL_INFO
 * whose rgCRLEntry member has been set prior to calling.
 */
static BOOL CRYPT_AsnDecodeCRLEntries(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded)
{
    BOOL ret;
    struct AsnArrayDescriptor arrayDesc = { ASN_SEQUENCEOF,
     offsetof(CRL_INFO, cCRLEntry), offsetof(CRL_INFO, rgCRLEntry),
     MEMBERSIZE(CRL_INFO, cCRLEntry, cExtension),
     CRYPT_AsnDecodeCRLEntry, sizeof(CRL_ENTRY), TRUE,
     offsetof(CRL_ENTRY, SerialNumber.pbData) };

    TRACE("%p, %d, %08x, %p, %d, %p\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo, pcbDecoded);

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
     dwFlags, NULL, pvStructInfo, pcbStructInfo, pcbDecoded);
    TRACE("Returning %d (%08x)\n", ret, GetLastError());
    return ret;
}

static BOOL CRYPT_AsnDecodeCRLExtensionsInternal(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret = TRUE;
    struct AsnArrayDescriptor arrayDesc = { ASN_SEQUENCEOF,
     offsetof(CRL_INFO, cExtension), offsetof(CRL_INFO, rgExtension),
     FINALMEMBERSIZE(CRL_INFO, cExtension),
     CRYPT_AsnDecodeExtension, sizeof(CERT_EXTENSION), TRUE,
     offsetof(CERT_EXTENSION, pszObjId) };

    TRACE("%p, %d, %08x, %p, %d, %p\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo, pcbDecoded);

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
     dwFlags, NULL, pvStructInfo, pcbStructInfo, pcbDecoded);
    return ret;
}

static BOOL CRYPT_AsnDecodeCRLExtensions(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;
    DWORD dataLen;

    if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
    {
        BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

        ret = CRYPT_AsnDecodeCRLExtensionsInternal(pbEncoded + 1 + lenBytes,
         dataLen, dwFlags, pvStructInfo, pcbStructInfo, NULL);
        if (ret && pcbDecoded)
            *pcbDecoded = 1 + lenBytes + dataLen;
    }
    return ret;
}

static BOOL CRYPT_AsnDecodeCRLInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    struct AsnDecodeSequenceItem items[] = {
     { ASN_INTEGER, offsetof(CRL_INFO, dwVersion),
       CRYPT_AsnDecodeIntInternal, sizeof(DWORD), TRUE, FALSE, 0, 0 },
     { ASN_SEQUENCEOF, offsetof(CRL_INFO, SignatureAlgorithm),
       CRYPT_AsnDecodeAlgorithmId, sizeof(CRYPT_ALGORITHM_IDENTIFIER),
       FALSE, TRUE, offsetof(CRL_INFO, SignatureAlgorithm.pszObjId), 0 },
     { 0, offsetof(CRL_INFO, Issuer), CRYPT_AsnDecodeDerBlob,
       sizeof(CRYPT_DER_BLOB), FALSE, TRUE, offsetof(CRL_INFO,
       Issuer.pbData) },
     { 0, offsetof(CRL_INFO, ThisUpdate), CRYPT_AsnDecodeChoiceOfTimeInternal,
       sizeof(FILETIME), FALSE, FALSE, 0 },
     { 0, offsetof(CRL_INFO, NextUpdate), CRYPT_AsnDecodeChoiceOfTimeInternal,
       sizeof(FILETIME), TRUE, FALSE, 0 },
     { ASN_SEQUENCEOF, offsetof(CRL_INFO, cCRLEntry),
       CRYPT_AsnDecodeCRLEntries, MEMBERSIZE(CRL_INFO, cCRLEntry, cExtension),
       TRUE, TRUE, offsetof(CRL_INFO, rgCRLEntry), 0 },
     { ASN_CONTEXT | ASN_CONSTRUCTOR | 0, offsetof(CRL_INFO, cExtension),
       CRYPT_AsnDecodeCRLExtensions, FINALMEMBERSIZE(CRL_INFO, cExtension),
       TRUE, TRUE, offsetof(CRL_INFO, rgExtension), 0 },
    };
    BOOL ret = TRUE;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items), pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, pcbStructInfo, NULL, NULL);

    TRACE("Returning %d (%08x)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeCRL(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    __TRY
    {
        DWORD size = 0;

        /* Unless told not to, first try to decode it as a signed crl. */
        if (!(dwFlags & CRYPT_DECODE_TO_BE_SIGNED_FLAG))
        {
            PCERT_SIGNED_CONTENT_INFO signedCrl = NULL;

            ret = CRYPT_AsnDecodeCertSignedContent(dwCertEncodingType,
             X509_CERT, pbEncoded, cbEncoded, CRYPT_DECODE_ALLOC_FLAG, NULL,
             &signedCrl, &size);
            if (ret)
            {
                size = 0;
                ret = CRYPT_AsnDecodeCRLInfo(dwCertEncodingType,
                 X509_CERT_CRL_TO_BE_SIGNED, signedCrl->ToBeSigned.pbData,
                 signedCrl->ToBeSigned.cbData, dwFlags, pDecodePara,
                 pvStructInfo, pcbStructInfo);
                LocalFree(signedCrl);
            }
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
    }
    __ENDTRY

    TRACE("Returning %d (%08x)\n", ret, GetLastError());
    return ret;
}

static BOOL CRYPT_AsnDecodeOidIgnoreTag(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded)
{
    BOOL ret = TRUE;
    DWORD dataLen;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
    {
        BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);
        DWORD bytesNeeded = sizeof(LPSTR);

        if (dataLen)
        {
            const BYTE *ptr;
            char str[32];

            snprintf(str, sizeof(str), "%d.%d",
             pbEncoded[1 + lenBytes] / 40,
             pbEncoded[1 + lenBytes] - (pbEncoded[1 + lenBytes] / 40)
             * 40);
            bytesNeeded += strlen(str) + 1;
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
        if (pcbDecoded)
            *pcbDecoded = 1 + lenBytes + dataLen;
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
    return ret;
}

static BOOL CRYPT_AsnDecodeOidInternal(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded)
{
    BOOL ret;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    if (pbEncoded[0] == ASN_OBJECTIDENTIFIER)
        ret = CRYPT_AsnDecodeOidIgnoreTag(pbEncoded, cbEncoded, dwFlags,
         pvStructInfo, pcbStructInfo, pcbDecoded);
    else
    {
        SetLastError(CRYPT_E_ASN1_BADTAG);
        ret = FALSE;
    }
    return ret;
}

static BOOL CRYPT_AsnDecodeExtension(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded)
{
    struct AsnDecodeSequenceItem items[] = {
     { ASN_OBJECTIDENTIFIER, offsetof(CERT_EXTENSION, pszObjId),
       CRYPT_AsnDecodeOidIgnoreTag, sizeof(LPSTR), FALSE, TRUE,
       offsetof(CERT_EXTENSION, pszObjId), 0 },
     { ASN_BOOL, offsetof(CERT_EXTENSION, fCritical), CRYPT_AsnDecodeBool,
       sizeof(BOOL), TRUE, FALSE, 0, 0 },
     { ASN_OCTETSTRING, offsetof(CERT_EXTENSION, Value),
       CRYPT_AsnDecodeOctets, sizeof(CRYPT_OBJID_BLOB), FALSE, TRUE,
       offsetof(CERT_EXTENSION, Value.pbData) },
    };
    BOOL ret = TRUE;
    PCERT_EXTENSION ext = pvStructInfo;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags, ext,
     *pcbStructInfo);

    if (ext)
        TRACE("ext->pszObjId is %p\n", ext->pszObjId);
    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, NULL, ext, pcbStructInfo,
     pcbDecoded, ext ? ext->pszObjId : NULL);
    if (ext)
        TRACE("ext->pszObjId is %p (%s)\n", ext->pszObjId,
         debugstr_a(ext->pszObjId));
    TRACE("returning %d (%08x)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeExtensions(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, pvStructInfo ? *pcbStructInfo : 0);

    __TRY
    {
        struct AsnArrayDescriptor arrayDesc = { ASN_SEQUENCEOF,
         offsetof(CERT_EXTENSIONS, cExtension),
         offsetof(CERT_EXTENSIONS, rgExtension),
         sizeof(CERT_EXTENSIONS),
         CRYPT_AsnDecodeExtension, sizeof(CERT_EXTENSION), TRUE,
         offsetof(CERT_EXTENSION, pszObjId) };
        CERT_EXTENSIONS *exts = pvStructInfo;

        if (pvStructInfo && !(dwFlags & CRYPT_DECODE_ALLOC_FLAG))
            exts->rgExtension = (CERT_EXTENSION *)(exts + 1);
        ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
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

/* Warning: this assumes the address of value->Value.pbData is already set, in
 * order to avoid overwriting memory.  (In some cases, it may change it, if it
 * doesn't copy anything to memory.)  Be sure to set it correctly!
 */
static BOOL CRYPT_AsnDecodeNameValueInternal(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret = TRUE;
    DWORD dataLen;
    CERT_NAME_VALUE *value = pvStructInfo;

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
             (LPCSTR)pbEncoded + 1 + lenBytes, dataLen, NULL, 0) * sizeof(WCHAR);
            break;
        default:
            SetLastError(CRYPT_E_ASN1_BADTAG);
            return FALSE;
        }

        if (pcbDecoded)
            *pcbDecoded = 1 + lenBytes + dataLen;
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
                    if (!(dwFlags & CRYPT_DECODE_NOCOPY_FLAG))
                        memcpy(value->Value.pbData,
                         pbEncoded + 1 + lenBytes, dataLen);
                    else
                        value->Value.pbData = (LPBYTE)pbEncoded + 1 +
                         lenBytes;
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
        ret = CRYPT_AsnDecodeNameValueInternal(pbEncoded, cbEncoded,
         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, pcbStructInfo, NULL);
        if (ret && pvStructInfo)
        {
            ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara, pvStructInfo,
             pcbStructInfo, *pcbStructInfo);
            if (ret)
            {
                CERT_NAME_VALUE *value;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                value = pvStructInfo;
                value->Value.pbData = ((BYTE *)value + sizeof(CERT_NAME_VALUE));
                ret = CRYPT_AsnDecodeNameValueInternal( pbEncoded, cbEncoded,
                 dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, pvStructInfo,
                 pcbStructInfo, NULL);
                if (!ret && (dwFlags & CRYPT_DECODE_ALLOC_FLAG))
                    CRYPT_FreeSpace(pDecodePara, value);
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

static BOOL CRYPT_AsnDecodeUnicodeNameValueInternal(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret = TRUE;
    DWORD dataLen;
    CERT_NAME_VALUE *value = pvStructInfo;

    if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
    {
        BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);
        DWORD bytesNeeded = sizeof(CERT_NAME_VALUE), valueType;

        switch (pbEncoded[0])
        {
        case ASN_NUMERICSTRING:
            valueType = CERT_RDN_NUMERIC_STRING;
            if (dataLen)
                bytesNeeded += (dataLen + 1) * 2;
            break;
        case ASN_PRINTABLESTRING:
            valueType = CERT_RDN_PRINTABLE_STRING;
            if (dataLen)
                bytesNeeded += (dataLen + 1) * 2;
            break;
        case ASN_IA5STRING:
            valueType = CERT_RDN_IA5_STRING;
            if (dataLen)
                bytesNeeded += (dataLen + 1) * 2;
            break;
        case ASN_T61STRING:
            valueType = CERT_RDN_T61_STRING;
            if (dataLen)
                bytesNeeded += (dataLen + 1) * 2;
            break;
        case ASN_VIDEOTEXSTRING:
            valueType = CERT_RDN_VIDEOTEX_STRING;
            if (dataLen)
                bytesNeeded += (dataLen + 1) * 2;
            break;
        case ASN_GRAPHICSTRING:
            valueType = CERT_RDN_GRAPHIC_STRING;
            if (dataLen)
                bytesNeeded += (dataLen + 1) * 2;
            break;
        case ASN_VISIBLESTRING:
            valueType = CERT_RDN_VISIBLE_STRING;
            if (dataLen)
                bytesNeeded += (dataLen + 1) * 2;
            break;
        case ASN_GENERALSTRING:
            valueType = CERT_RDN_GENERAL_STRING;
            if (dataLen)
                bytesNeeded += (dataLen + 1) * 2;
            break;
        case ASN_UNIVERSALSTRING:
            valueType = CERT_RDN_UNIVERSAL_STRING;
            if (dataLen)
                bytesNeeded += dataLen / 2 + sizeof(WCHAR);
            break;
        case ASN_BMPSTRING:
            valueType = CERT_RDN_BMP_STRING;
            if (dataLen)
                bytesNeeded += dataLen + sizeof(WCHAR);
            break;
        case ASN_UTF8STRING:
            valueType = CERT_RDN_UTF8_STRING;
            if (dataLen)
                bytesNeeded += (MultiByteToWideChar(CP_UTF8, 0,
                 (LPCSTR)pbEncoded + 1 + lenBytes, dataLen, NULL, 0) + 1) * 2;
            break;
        default:
            SetLastError(CRYPT_E_ASN1_BADTAG);
            return FALSE;
        }

        if (pcbDecoded)
            *pcbDecoded = 1 + lenBytes + dataLen;
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
                    str[i] = 0;
                    break;
                case ASN_UNIVERSALSTRING:
                    value->Value.cbData = dataLen / 2;
                    for (i = 0; i < dataLen / 4; i++)
                        str[i] = (pbEncoded[1 + lenBytes + 2 * i + 2] << 8)
                         | pbEncoded[1 + lenBytes + 2 * i + 3];
                    str[i] = 0;
                    break;
                case ASN_BMPSTRING:
                    value->Value.cbData = dataLen;
                    for (i = 0; i < dataLen / 2; i++)
                        str[i] = (pbEncoded[1 + lenBytes + 2 * i] << 8) |
                         pbEncoded[1 + lenBytes + 2 * i + 1];
                    str[i] = 0;
                    break;
                case ASN_UTF8STRING:
                    value->Value.cbData = MultiByteToWideChar(CP_UTF8, 0,
                     (LPCSTR)pbEncoded + 1 + lenBytes, dataLen,
                     str, bytesNeeded - sizeof(CERT_NAME_VALUE)) * sizeof(WCHAR);
                    *(WCHAR *)(value->Value.pbData + value->Value.cbData) = 0;
                    value->Value.cbData += sizeof(WCHAR);
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
        ret = CRYPT_AsnDecodeUnicodeNameValueInternal(pbEncoded, cbEncoded,
         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, pcbStructInfo, NULL);
        if (ret && pvStructInfo)
        {
            ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara, pvStructInfo,
             pcbStructInfo, *pcbStructInfo);
            if (ret)
            {
                CERT_NAME_VALUE *value;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                value = pvStructInfo;
                value->Value.pbData = ((BYTE *)value + sizeof(CERT_NAME_VALUE));
                ret = CRYPT_AsnDecodeUnicodeNameValueInternal(pbEncoded,
                 cbEncoded, dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, pvStructInfo,
                 pcbStructInfo, NULL);
                if (!ret && (dwFlags & CRYPT_DECODE_ALLOC_FLAG))
                    CRYPT_FreeSpace(pDecodePara, value);
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

static BOOL CRYPT_AsnDecodeRdnAttr(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded)
{
    BOOL ret;
    struct AsnDecodeSequenceItem items[] = {
     { ASN_OBJECTIDENTIFIER, offsetof(CERT_RDN_ATTR, pszObjId),
       CRYPT_AsnDecodeOidIgnoreTag, sizeof(LPSTR), FALSE, TRUE,
       offsetof(CERT_RDN_ATTR, pszObjId), 0 },
     { 0, offsetof(CERT_RDN_ATTR, dwValueType),
       CRYPT_AsnDecodeNameValueInternal, sizeof(CERT_NAME_VALUE),
       FALSE, TRUE, offsetof(CERT_RDN_ATTR, Value.pbData), 0 },
    };
    CERT_RDN_ATTR *attr = pvStructInfo;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    if (attr)
        TRACE("attr->pszObjId is %p\n", attr->pszObjId);
    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, NULL, attr, pcbStructInfo, pcbDecoded,
     attr ? attr->pszObjId : NULL);
    if (attr)
    {
        TRACE("attr->pszObjId is %p (%s)\n", attr->pszObjId,
         debugstr_a(attr->pszObjId));
        TRACE("attr->dwValueType is %d\n", attr->dwValueType);
    }
    TRACE("returning %d (%08x)\n", ret, GetLastError());
    return ret;
}

static BOOL CRYPT_AsnDecodeRdn(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags,  void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded)
{
    BOOL ret = TRUE;
    struct AsnArrayDescriptor arrayDesc = { ASN_CONSTRUCTOR | ASN_SETOF,
     offsetof(CERT_RDN, cRDNAttr), offsetof(CERT_RDN, rgRDNAttr),
     sizeof(CERT_RDN),
     CRYPT_AsnDecodeRdnAttr, sizeof(CERT_RDN_ATTR), TRUE,
     offsetof(CERT_RDN_ATTR, pszObjId) };

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded, dwFlags,
     NULL, pvStructInfo, pcbStructInfo, pcbDecoded);
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
         offsetof(CERT_NAME_INFO, cRDN), offsetof(CERT_NAME_INFO, rgRDN),
         sizeof(CERT_NAME_INFO),
         CRYPT_AsnDecodeRdn, sizeof(CERT_RDN), TRUE,
         offsetof(CERT_RDN, rgRDNAttr) };
        DWORD bytesNeeded = 0;

        ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, NULL, &bytesNeeded,
         NULL);
        if (ret)
        {
            if (!pvStructInfo)
                *pcbStructInfo = bytesNeeded;
            else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
             pvStructInfo, pcbStructInfo, bytesNeeded)))
            {
                CERT_NAME_INFO *info;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                info = pvStructInfo;
                info->rgRDN = (CERT_RDN *)((BYTE *)pvStructInfo +
                 sizeof(CERT_NAME_INFO));
                ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
                 dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, pvStructInfo,
                 &bytesNeeded, NULL);
                if (!ret && (dwFlags & CRYPT_DECODE_ALLOC_FLAG))
                    CRYPT_FreeSpace(pDecodePara, info);
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

static BOOL CRYPT_AsnDecodeUnicodeRdnAttr(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;
    struct AsnDecodeSequenceItem items[] = {
     { ASN_OBJECTIDENTIFIER, offsetof(CERT_RDN_ATTR, pszObjId),
       CRYPT_AsnDecodeOidIgnoreTag, sizeof(LPSTR), FALSE, TRUE,
       offsetof(CERT_RDN_ATTR, pszObjId), 0 },
     { 0, offsetof(CERT_RDN_ATTR, dwValueType),
       CRYPT_AsnDecodeUnicodeNameValueInternal, sizeof(CERT_NAME_VALUE),
       FALSE, TRUE, offsetof(CERT_RDN_ATTR, Value.pbData), 0 },
    };
    CERT_RDN_ATTR *attr = pvStructInfo;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    if (attr)
        TRACE("attr->pszObjId is %p\n", attr->pszObjId);
    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, NULL, attr, pcbStructInfo, pcbDecoded,
     attr ? attr->pszObjId : NULL);
    if (attr)
    {
        TRACE("attr->pszObjId is %p (%s)\n", attr->pszObjId,
         debugstr_a(attr->pszObjId));
        TRACE("attr->dwValueType is %d\n", attr->dwValueType);
    }
    TRACE("returning %d (%08x)\n", ret, GetLastError());
    return ret;
}

static BOOL CRYPT_AsnDecodeUnicodeRdn(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded)
{
    BOOL ret = TRUE;
    struct AsnArrayDescriptor arrayDesc = { ASN_CONSTRUCTOR | ASN_SETOF,
     offsetof(CERT_RDN, cRDNAttr), offsetof(CERT_RDN, rgRDNAttr),
     sizeof(CERT_RDN),
     CRYPT_AsnDecodeUnicodeRdnAttr, sizeof(CERT_RDN_ATTR), TRUE,
     offsetof(CERT_RDN_ATTR, pszObjId) };

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded, dwFlags,
     NULL, pvStructInfo, pcbStructInfo, pcbDecoded);
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
         offsetof(CERT_NAME_INFO, cRDN), offsetof(CERT_NAME_INFO, rgRDN),
         sizeof(CERT_NAME_INFO),
         CRYPT_AsnDecodeUnicodeRdn, sizeof(CERT_RDN), TRUE,
         offsetof(CERT_RDN, rgRDNAttr) };
        DWORD bytesNeeded = 0;

        ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, NULL, &bytesNeeded,
         NULL);
        if (ret)
        {
            if (!pvStructInfo)
                *pcbStructInfo = bytesNeeded;
            else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
             pvStructInfo, pcbStructInfo, bytesNeeded)))
            {
                CERT_NAME_INFO *info;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                info = pvStructInfo;
                info->rgRDN = (CERT_RDN *)((BYTE *)pvStructInfo +
                 sizeof(CERT_NAME_INFO));
                ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
                 dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, pvStructInfo,
                 &bytesNeeded, NULL);
                if (!ret && (dwFlags & CRYPT_DECODE_ALLOC_FLAG))
                    CRYPT_FreeSpace(pDecodePara, info);
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

static BOOL CRYPT_FindEncodedLen(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD *pcbDecoded)
{
    BOOL ret = TRUE, done = FALSE;
    DWORD indefiniteNestingLevels = 0, decoded = 0;

    TRACE("(%p, %d)\n", pbEncoded, cbEncoded);

    do {
        DWORD dataLen;

        if (!cbEncoded)
            done = TRUE;
        else if ((ret = CRYPT_GetLengthIndefinite(pbEncoded, cbEncoded,
         &dataLen)))
        {
            BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

            if (dataLen == CMSG_INDEFINITE_LENGTH)
            {
                indefiniteNestingLevels++;
                pbEncoded += 1 + lenBytes;
                cbEncoded -= 1 + lenBytes;
                decoded += 1 + lenBytes;
                TRACE("indefiniteNestingLevels = %d\n",
                 indefiniteNestingLevels);
            }
            else
            {
                if (pbEncoded[0] == 0 && pbEncoded[1] == 0 &&
                 indefiniteNestingLevels)
                {
                    indefiniteNestingLevels--;
                    TRACE("indefiniteNestingLevels = %d\n",
                     indefiniteNestingLevels);
                }
                pbEncoded += 1 + lenBytes + dataLen;
                cbEncoded -= 1 + lenBytes + dataLen;
                decoded += 1 + lenBytes + dataLen;
                if (!indefiniteNestingLevels)
                    done = TRUE;
            }
        }
    } while (ret && !done);
    /* If we haven't found all 0 TLVs, we haven't found the end */
    if (ret && indefiniteNestingLevels)
    {
        SetLastError(CRYPT_E_ASN1_EOD);
        ret = FALSE;
    }
    if (ret)
        *pcbDecoded = decoded;
    TRACE("returning %d (%d)\n", ret, ret ? *pcbDecoded : 0);
    return ret;
}

static BOOL CRYPT_AsnDecodeCopyBytes(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret = TRUE;
    DWORD bytesNeeded = sizeof(CRYPT_OBJID_BLOB), encodedLen = 0;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    if ((ret = CRYPT_FindEncodedLen(pbEncoded, cbEncoded, &encodedLen)))
    {
        if (!(dwFlags & CRYPT_DECODE_NOCOPY_FLAG))
            bytesNeeded += encodedLen;
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
            PCRYPT_OBJID_BLOB blob = pvStructInfo;

            *pcbStructInfo = bytesNeeded;
            blob->cbData = encodedLen;
            if (encodedLen)
            {
                if (dwFlags & CRYPT_DECODE_NOCOPY_FLAG)
                    blob->pbData = (LPBYTE)pbEncoded;
                else
                {
                    assert(blob->pbData);
                    memcpy(blob->pbData, pbEncoded, blob->cbData);
                }
            }
            else
                blob->pbData = NULL;
        }
        if (pcbDecoded)
            *pcbDecoded = encodedLen;
    }
    return ret;
}

static BOOL CRYPT_AsnDecodeCTLUsage(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded)
{
    BOOL ret;
    struct AsnArrayDescriptor arrayDesc = { ASN_SEQUENCEOF,
     offsetof(CTL_USAGE, cUsageIdentifier),
     offsetof(CTL_USAGE, rgpszUsageIdentifier),
     sizeof(CTL_USAGE),
     CRYPT_AsnDecodeOidInternal, sizeof(LPSTR), TRUE, 0 };

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded, dwFlags,
     NULL, pvStructInfo, pcbStructInfo, pcbDecoded);
    return ret;
}

static BOOL CRYPT_AsnDecodeCTLEntryAttributes(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    struct AsnArrayDescriptor arrayDesc = { 0,
     offsetof(CTL_ENTRY, cAttribute), offsetof(CTL_ENTRY, rgAttribute),
     FINALMEMBERSIZE(CTL_ENTRY, cAttribute),
     CRYPT_AsnDecodePKCSAttributeInternal, sizeof(CRYPT_ATTRIBUTE), TRUE,
     offsetof(CRYPT_ATTRIBUTE, pszObjId) };
    BOOL ret;

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
     dwFlags, NULL, pvStructInfo, pcbStructInfo, pcbDecoded);
    return ret;
}

static BOOL CRYPT_AsnDecodeCTLEntry(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded)
{
    struct AsnDecodeSequenceItem items[] = {
     { ASN_OCTETSTRING, offsetof(CTL_ENTRY, SubjectIdentifier),
       CRYPT_AsnDecodeOctets, sizeof(CRYPT_DATA_BLOB), FALSE, TRUE,
       offsetof(CTL_ENTRY, SubjectIdentifier.pbData), 0 },
     { ASN_CONSTRUCTOR | ASN_SETOF, offsetof(CTL_ENTRY, cAttribute),
       CRYPT_AsnDecodeCTLEntryAttributes,
       FINALMEMBERSIZE(CTL_ENTRY, cAttribute), FALSE, TRUE,
       offsetof(CTL_ENTRY, rgAttribute), 0 },
    };
    BOOL ret = TRUE;
    CTL_ENTRY *entry = pvStructInfo;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags, entry,
     *pcbStructInfo);

    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, NULL, entry, pcbStructInfo,
     pcbDecoded, entry ? entry->SubjectIdentifier.pbData : NULL);
    return ret;
}

static BOOL CRYPT_AsnDecodeCTLEntries(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded)
{
    BOOL ret;
    struct AsnArrayDescriptor arrayDesc = { ASN_SEQUENCEOF,
     offsetof(CTL_INFO, cCTLEntry), offsetof(CTL_INFO, rgCTLEntry),
     FINALMEMBERSIZE(CTL_INFO, cExtension),
     CRYPT_AsnDecodeCTLEntry, sizeof(CTL_ENTRY), TRUE,
     offsetof(CTL_ENTRY, SubjectIdentifier.pbData) };

    TRACE("%p, %d, %08x, %p, %d, %p\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo, pcbDecoded);

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
     dwFlags, NULL, pvStructInfo, pcbStructInfo, pcbDecoded);
    return ret;
}

static BOOL CRYPT_AsnDecodeCTLExtensionsInternal(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret = TRUE;
    struct AsnArrayDescriptor arrayDesc = { ASN_SEQUENCEOF,
     offsetof(CTL_INFO, cExtension), offsetof(CTL_INFO, rgExtension),
     FINALMEMBERSIZE(CTL_INFO, cExtension),
     CRYPT_AsnDecodeExtension, sizeof(CERT_EXTENSION), TRUE,
     offsetof(CERT_EXTENSION, pszObjId) };

    TRACE("%p, %d, %08x, %p, %d, %p\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo, pcbDecoded);

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
     dwFlags, NULL, pvStructInfo, pcbStructInfo, pcbDecoded);
    return ret;
}

static BOOL CRYPT_AsnDecodeCTLExtensions(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;
    DWORD dataLen;

    if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
    {
        BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

        ret = CRYPT_AsnDecodeCTLExtensionsInternal(pbEncoded + 1 + lenBytes,
         dataLen, dwFlags, pvStructInfo, pcbStructInfo, NULL);
        if (ret && pcbDecoded)
            *pcbDecoded = 1 + lenBytes + dataLen;
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeCTL(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    __TRY
    {
        struct AsnDecodeSequenceItem items[] = {
         { ASN_INTEGER, offsetof(CTL_INFO, dwVersion),
           CRYPT_AsnDecodeIntInternal, sizeof(DWORD), TRUE, FALSE, 0, 0 },
         { ASN_SEQUENCEOF, offsetof(CTL_INFO, SubjectUsage),
           CRYPT_AsnDecodeCTLUsage, sizeof(CTL_USAGE), FALSE, TRUE,
           offsetof(CTL_INFO, SubjectUsage.rgpszUsageIdentifier), 0 },
         { ASN_OCTETSTRING, offsetof(CTL_INFO, ListIdentifier),
           CRYPT_AsnDecodeOctets, sizeof(CRYPT_DATA_BLOB), TRUE,
           TRUE, offsetof(CTL_INFO, ListIdentifier.pbData), 0 },
         { ASN_INTEGER, offsetof(CTL_INFO, SequenceNumber),
           CRYPT_AsnDecodeIntegerInternal, sizeof(CRYPT_INTEGER_BLOB),
           TRUE, TRUE, offsetof(CTL_INFO, SequenceNumber.pbData), 0 },
         { 0, offsetof(CTL_INFO, ThisUpdate),
           CRYPT_AsnDecodeChoiceOfTimeInternal, sizeof(FILETIME), FALSE, FALSE,
           0 },
         { 0, offsetof(CTL_INFO, NextUpdate),
           CRYPT_AsnDecodeChoiceOfTimeInternal, sizeof(FILETIME), TRUE, FALSE,
           0 },
         { ASN_SEQUENCEOF, offsetof(CTL_INFO, SubjectAlgorithm),
           CRYPT_AsnDecodeAlgorithmId, sizeof(CRYPT_ALGORITHM_IDENTIFIER),
           FALSE, TRUE, offsetof(CTL_INFO, SubjectAlgorithm.pszObjId), 0 },
         { ASN_SEQUENCEOF, offsetof(CTL_INFO, cCTLEntry),
           CRYPT_AsnDecodeCTLEntries,
           MEMBERSIZE(CTL_INFO, cCTLEntry, cExtension),
           TRUE, TRUE, offsetof(CTL_INFO, rgCTLEntry), 0 },
         { ASN_CONTEXT | ASN_CONSTRUCTOR | 0, offsetof(CTL_INFO, cExtension),
           CRYPT_AsnDecodeCTLExtensions, FINALMEMBERSIZE(CTL_INFO, cExtension),
           TRUE, TRUE, offsetof(CTL_INFO, rgExtension), 0 },
        };

        ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
         pbEncoded, cbEncoded, dwFlags, pDecodePara, pvStructInfo,
         pcbStructInfo, NULL, NULL);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    return ret;
}

static BOOL CRYPT_AsnDecodeSMIMECapability(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;
    struct AsnDecodeSequenceItem items[] = {
     { ASN_OBJECTIDENTIFIER, offsetof(CRYPT_SMIME_CAPABILITY, pszObjId),
       CRYPT_AsnDecodeOidIgnoreTag, sizeof(LPSTR), FALSE, TRUE,
       offsetof(CRYPT_SMIME_CAPABILITY, pszObjId), 0 },
     { 0, offsetof(CRYPT_SMIME_CAPABILITY, Parameters),
       CRYPT_AsnDecodeCopyBytes, sizeof(CRYPT_OBJID_BLOB), TRUE, TRUE,
       offsetof(CRYPT_SMIME_CAPABILITY, Parameters.pbData), 0 },
    };
    PCRYPT_SMIME_CAPABILITY capability = pvStructInfo;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, NULL, pvStructInfo, pcbStructInfo,
     pcbDecoded, capability ? capability->pszObjId : NULL);
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeSMIMECapabilities(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    __TRY
    {
        struct AsnArrayDescriptor arrayDesc = { ASN_SEQUENCEOF,
         offsetof(CRYPT_SMIME_CAPABILITIES, cCapability),
         offsetof(CRYPT_SMIME_CAPABILITIES, rgCapability),
         sizeof(CRYPT_SMIME_CAPABILITIES),
         CRYPT_AsnDecodeSMIMECapability, sizeof(CRYPT_SMIME_CAPABILITY), TRUE,
         offsetof(CRYPT_SMIME_CAPABILITY, pszObjId) };
        CRYPT_SMIME_CAPABILITIES *capabilities = pvStructInfo;

        if (pvStructInfo && !(dwFlags & CRYPT_DECODE_ALLOC_FLAG))
            capabilities->rgCapability = (CRYPT_SMIME_CAPABILITY *)(capabilities + 1);
        ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
         dwFlags, pDecodePara, pvStructInfo, pcbStructInfo, NULL);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL CRYPT_AsnDecodeIA5String(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret = TRUE;
    DWORD dataLen;
    LPSTR *pStr = pvStructInfo;

    if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
    {
        BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);
        DWORD bytesNeeded = sizeof(LPSTR) + sizeof(char);

        if (pbEncoded[0] != ASN_IA5STRING)
        {
            SetLastError(CRYPT_E_ASN1_CORRUPT);
            ret = FALSE;
        }
        else
        {
            bytesNeeded += dataLen;
            if (pcbDecoded)
                *pcbDecoded = 1 + lenBytes + dataLen;
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
                *pcbStructInfo = bytesNeeded;
                if (dataLen)
                {
                    LPSTR str = *pStr;

                    assert(str);
                    memcpy(str, pbEncoded + 1 + lenBytes, dataLen);
                    str[dataLen] = 0;
                }
                else
                    *pStr = NULL;
            }
        }
    }
    return ret;
}

static BOOL CRYPT_AsnDecodeNoticeNumbers(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    struct AsnArrayDescriptor arrayDesc = { ASN_SEQUENCEOF,
     offsetof(CERT_POLICY_QUALIFIER_NOTICE_REFERENCE, cNoticeNumbers),
     offsetof(CERT_POLICY_QUALIFIER_NOTICE_REFERENCE, rgNoticeNumbers),
     FINALMEMBERSIZE(CERT_POLICY_QUALIFIER_NOTICE_REFERENCE, cNoticeNumbers),
     CRYPT_AsnDecodeIntInternal, sizeof(int), FALSE, 0 };
    BOOL ret;

    TRACE("(%p, %d, %08x, %p, %d)\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, pvStructInfo ? *pcbDecoded : 0);

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
     dwFlags, NULL, pvStructInfo, pcbStructInfo, pcbDecoded);
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL CRYPT_AsnDecodeNoticeReference(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;
    struct AsnDecodeSequenceItem items[] = {
     { ASN_IA5STRING, offsetof(CERT_POLICY_QUALIFIER_NOTICE_REFERENCE,
       pszOrganization), CRYPT_AsnDecodeIA5String, sizeof(LPSTR), FALSE, TRUE,
       offsetof(CERT_POLICY_QUALIFIER_NOTICE_REFERENCE, pszOrganization), 0 },
     { ASN_SEQUENCEOF, offsetof(CERT_POLICY_QUALIFIER_NOTICE_REFERENCE,
       cNoticeNumbers), CRYPT_AsnDecodeNoticeNumbers,
       FINALMEMBERSIZE(CERT_POLICY_QUALIFIER_NOTICE_REFERENCE, cNoticeNumbers),
       FALSE, TRUE, offsetof(CERT_POLICY_QUALIFIER_NOTICE_REFERENCE,
       rgNoticeNumbers), 0 },
    };
    DWORD bytesNeeded = 0;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, pvStructInfo ? *pcbStructInfo : 0);

    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, NULL, NULL, &bytesNeeded, pcbDecoded,
     NULL);
    if (ret)
    {
        /* The caller is expecting a pointer to a
         * CERT_POLICY_QUALIFIER_NOTICE_REFERENCE to be decoded, whereas
         * CRYPT_AsnDecodeSequence is decoding a
         * CERT_POLICY_QUALIFIER_NOTICE_REFERENCE.  Increment the bytes
         * needed, and decode again if the requisite space is available.
         */
        bytesNeeded += sizeof(PCERT_POLICY_QUALIFIER_NOTICE_REFERENCE);
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
            PCERT_POLICY_QUALIFIER_NOTICE_REFERENCE noticeRef;

            *pcbStructInfo = bytesNeeded;
            /* The pointer (pvStructInfo) passed in points to the first dynamic
             * pointer, so use it as the pointer to the
             * CERT_POLICY_QUALIFIER_NOTICE_REFERENCE, and create the
             * appropriate offset for the first dynamic pointer within the
             * notice reference by pointing to the first memory location past
             * the CERT_POLICY_QUALIFIER_NOTICE_REFERENCE.
             */
            noticeRef =
             *(PCERT_POLICY_QUALIFIER_NOTICE_REFERENCE *)pvStructInfo;
            noticeRef->pszOrganization = (LPSTR)((LPBYTE)noticeRef +
             sizeof(CERT_POLICY_QUALIFIER_NOTICE_REFERENCE));
            ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items), pbEncoded, cbEncoded, dwFlags,
             NULL, noticeRef, &bytesNeeded, pcbDecoded, noticeRef->pszOrganization);
        }
    }
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL CRYPT_AsnDecodeUnicodeString(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret = TRUE;
    DWORD dataLen;

    if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
    {
        BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);
        DWORD bytesNeeded = sizeof(LPWSTR);

        switch (pbEncoded[0])
        {
        case ASN_NUMERICSTRING:
            if (dataLen)
                bytesNeeded += (dataLen + 1) * 2;
            break;
        case ASN_PRINTABLESTRING:
            if (dataLen)
                bytesNeeded += (dataLen + 1) * 2;
            break;
        case ASN_IA5STRING:
            if (dataLen)
                bytesNeeded += (dataLen + 1) * 2;
            break;
        case ASN_T61STRING:
            if (dataLen)
                bytesNeeded += (dataLen + 1) * 2;
            break;
        case ASN_VIDEOTEXSTRING:
            if (dataLen)
                bytesNeeded += (dataLen + 1) * 2;
            break;
        case ASN_GRAPHICSTRING:
            if (dataLen)
                bytesNeeded += (dataLen + 1) * 2;
            break;
        case ASN_VISIBLESTRING:
            if (dataLen)
                bytesNeeded += (dataLen + 1) * 2;
            break;
        case ASN_GENERALSTRING:
            if (dataLen)
                bytesNeeded += (dataLen + 1) * 2;
            break;
        case ASN_UNIVERSALSTRING:
            if (dataLen)
                bytesNeeded += dataLen / 2 + sizeof(WCHAR);
            break;
        case ASN_BMPSTRING:
            if (dataLen)
                bytesNeeded += dataLen + sizeof(WCHAR);
            break;
        case ASN_UTF8STRING:
            if (dataLen)
                bytesNeeded += (MultiByteToWideChar(CP_UTF8, 0,
                 (LPCSTR)pbEncoded + 1 + lenBytes, dataLen, NULL, 0) + 1) * 2;
            break;
        default:
            SetLastError(CRYPT_E_ASN1_BADTAG);
            return FALSE;
        }

        if (pcbDecoded)
            *pcbDecoded = 1 + lenBytes + dataLen;
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
            LPWSTR *pStr = pvStructInfo;

            *pcbStructInfo = bytesNeeded;
            if (dataLen)
            {
                DWORD i;
                LPWSTR str = *pStr;

                assert(str);
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
                    for (i = 0; i < dataLen; i++)
                        str[i] = pbEncoded[1 + lenBytes + i];
                    str[i] = 0;
                    break;
                case ASN_UNIVERSALSTRING:
                    for (i = 0; i < dataLen / 4; i++)
                        str[i] = (pbEncoded[1 + lenBytes + 2 * i + 2] << 8)
                         | pbEncoded[1 + lenBytes + 2 * i + 3];
                    str[i] = 0;
                    break;
                case ASN_BMPSTRING:
                    for (i = 0; i < dataLen / 2; i++)
                        str[i] = (pbEncoded[1 + lenBytes + 2 * i] << 8) |
                         pbEncoded[1 + lenBytes + 2 * i + 1];
                    str[i] = 0;
                    break;
                case ASN_UTF8STRING:
                {
                    int len = MultiByteToWideChar(CP_UTF8, 0,
                     (LPCSTR)pbEncoded + 1 + lenBytes, dataLen,
                     str, bytesNeeded - sizeof(CERT_NAME_VALUE)) * 2;
                    str[len] = 0;
                    break;
                }
                }
            }
            else
                *pStr = NULL;
        }
    }
    return ret;
}

static BOOL CRYPT_AsnDecodePolicyQualifierUserNoticeInternal(
 const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo,
 DWORD *pcbStructInfo, DWORD *pcbDecoded)
{
    BOOL ret;
    struct AsnDecodeSequenceItem items[] = {
     { ASN_SEQUENCE, offsetof(CERT_POLICY_QUALIFIER_USER_NOTICE,
       pNoticeReference), CRYPT_AsnDecodeNoticeReference,
       sizeof(PCERT_POLICY_QUALIFIER_NOTICE_REFERENCE), TRUE, TRUE,
       offsetof(CERT_POLICY_QUALIFIER_USER_NOTICE, pNoticeReference), 0 },
     { 0, offsetof(CERT_POLICY_QUALIFIER_USER_NOTICE, pszDisplayText),
       CRYPT_AsnDecodeUnicodeString, sizeof(LPWSTR), TRUE, TRUE,
       offsetof(CERT_POLICY_QUALIFIER_USER_NOTICE, pszDisplayText), 0 },
    };
    PCERT_POLICY_QUALIFIER_USER_NOTICE notice = pvStructInfo;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, NULL, pvStructInfo, pcbStructInfo,
     pcbDecoded, notice ? notice->pNoticeReference : NULL);
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodePolicyQualifierUserNotice(
 DWORD dwCertEncodingType, LPCSTR lpszStructType, const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, PCRYPT_DECODE_PARA pDecodePara,
 void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    __TRY
    {
        DWORD bytesNeeded = 0;

        ret = CRYPT_AsnDecodePolicyQualifierUserNoticeInternal(pbEncoded,
         cbEncoded, dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, &bytesNeeded,
         NULL);
        if (ret)
        {
            if (!pvStructInfo)
                *pcbStructInfo = bytesNeeded;
            else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
             pvStructInfo, pcbStructInfo, bytesNeeded)))
            {
                PCERT_POLICY_QUALIFIER_USER_NOTICE notice;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                notice = pvStructInfo;
                notice->pNoticeReference =
                 (PCERT_POLICY_QUALIFIER_NOTICE_REFERENCE)
                 ((BYTE *)pvStructInfo +
                 sizeof(CERT_POLICY_QUALIFIER_USER_NOTICE));
                ret = CRYPT_AsnDecodePolicyQualifierUserNoticeInternal(
                 pbEncoded, cbEncoded, dwFlags & ~CRYPT_DECODE_ALLOC_FLAG,
                 pvStructInfo, &bytesNeeded, NULL);
                if (!ret && (dwFlags & CRYPT_DECODE_ALLOC_FLAG))
                    CRYPT_FreeSpace(pDecodePara, notice);
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL CRYPT_AsnDecodePKCSAttributeValue(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;
    struct AsnArrayDescriptor arrayDesc = { 0,
     offsetof(CRYPT_ATTRIBUTE, cValue), offsetof(CRYPT_ATTRIBUTE, rgValue),
     FINALMEMBERSIZE(CRYPT_ATTRIBUTE, cValue),
     CRYPT_AsnDecodeCopyBytes,
     sizeof(CRYPT_DER_BLOB), TRUE, offsetof(CRYPT_DER_BLOB, pbData) };

    TRACE("%p, %d, %08x, %p, %d, %p\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, pvStructInfo ? *pcbStructInfo : 0, pcbDecoded);

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
     dwFlags, NULL, pvStructInfo, pcbStructInfo, pcbDecoded);
    return ret;
}

static BOOL CRYPT_AsnDecodePKCSAttributeInternal(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;
    struct AsnDecodeSequenceItem items[] = {
     { ASN_OBJECTIDENTIFIER, offsetof(CRYPT_ATTRIBUTE, pszObjId),
       CRYPT_AsnDecodeOidIgnoreTag, sizeof(LPSTR), FALSE, TRUE,
       offsetof(CRYPT_ATTRIBUTE, pszObjId), 0 },
     { ASN_CONSTRUCTOR | ASN_SETOF, offsetof(CRYPT_ATTRIBUTE, cValue),
       CRYPT_AsnDecodePKCSAttributeValue,
       FINALMEMBERSIZE(CRYPT_ATTRIBUTE, cValue), FALSE,
       TRUE, offsetof(CRYPT_ATTRIBUTE, rgValue), 0 },
    };
    PCRYPT_ATTRIBUTE attr = pvStructInfo;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, NULL, pvStructInfo, pcbStructInfo,
     pcbDecoded, attr ? attr->pszObjId : NULL);
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodePKCSAttribute(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    __TRY
    {
        DWORD bytesNeeded = 0;

        ret = CRYPT_AsnDecodePKCSAttributeInternal(pbEncoded, cbEncoded,
         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, &bytesNeeded, NULL);
        if (ret)
        {
            if (!pvStructInfo)
                *pcbStructInfo = bytesNeeded;
            else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
             pvStructInfo, pcbStructInfo, bytesNeeded)))
            {
                PCRYPT_ATTRIBUTE attr;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                attr = pvStructInfo;
                attr->pszObjId = (LPSTR)((BYTE *)pvStructInfo +
                 sizeof(CRYPT_ATTRIBUTE));
                ret = CRYPT_AsnDecodePKCSAttributeInternal(pbEncoded, cbEncoded,
                 dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, pvStructInfo, &bytesNeeded,
                 NULL);
                if (!ret && (dwFlags & CRYPT_DECODE_ALLOC_FLAG))
                    CRYPT_FreeSpace(pDecodePara, attr);
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL CRYPT_AsnDecodePKCSAttributesInternal(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    struct AsnArrayDescriptor arrayDesc = { 0,
     offsetof(CRYPT_ATTRIBUTES, cAttr), offsetof(CRYPT_ATTRIBUTES, rgAttr),
     sizeof(CRYPT_ATTRIBUTES),
     CRYPT_AsnDecodePKCSAttributeInternal, sizeof(CRYPT_ATTRIBUTE), TRUE,
     offsetof(CRYPT_ATTRIBUTE, pszObjId) };
    BOOL ret;

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded, dwFlags,
     NULL, pvStructInfo, pcbStructInfo, pcbDecoded);
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodePKCSAttributes(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    __TRY
    {
        struct AsnArrayDescriptor arrayDesc = { ASN_CONSTRUCTOR | ASN_SETOF,
         offsetof(CRYPT_ATTRIBUTES, cAttr), offsetof(CRYPT_ATTRIBUTES, rgAttr),
         sizeof(CRYPT_ATTRIBUTES),
         CRYPT_AsnDecodePKCSAttributeInternal, sizeof(CRYPT_ATTRIBUTE),
         TRUE, offsetof(CRYPT_ATTRIBUTE, pszObjId) };
        CRYPT_ATTRIBUTES *attrs = pvStructInfo;

        if (pvStructInfo && !(dwFlags & CRYPT_DECODE_ALLOC_FLAG))
            attrs->rgAttr = (CRYPT_ATTRIBUTE *)(attrs + 1);
        ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
         dwFlags, pDecodePara, pvStructInfo, pcbStructInfo, NULL);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL CRYPT_AsnDecodeAlgorithmId(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded)
{
    CRYPT_ALGORITHM_IDENTIFIER *algo = pvStructInfo;
    BOOL ret = TRUE;
    struct AsnDecodeSequenceItem items[] = {
     { ASN_OBJECTIDENTIFIER, offsetof(CRYPT_ALGORITHM_IDENTIFIER, pszObjId),
       CRYPT_AsnDecodeOidIgnoreTag, sizeof(LPSTR), FALSE, TRUE,
       offsetof(CRYPT_ALGORITHM_IDENTIFIER, pszObjId), 0 },
     { 0, offsetof(CRYPT_ALGORITHM_IDENTIFIER, Parameters),
       CRYPT_AsnDecodeCopyBytes, sizeof(CRYPT_OBJID_BLOB), TRUE, TRUE, 
       offsetof(CRYPT_ALGORITHM_IDENTIFIER, Parameters.pbData), 0 },
    };

    TRACE("%p, %d, %08x, %p, %d, %p\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo, pcbDecoded);

    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, NULL, pvStructInfo, pcbStructInfo,
     pcbDecoded, algo ? algo->pszObjId : NULL);
    if (ret && pvStructInfo)
    {
        TRACE("pszObjId is %p (%s)\n", algo->pszObjId,
         debugstr_a(algo->pszObjId));
    }
    return ret;
}

static BOOL CRYPT_AsnDecodePubKeyInfoInternal(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
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
    PCERT_PUBLIC_KEY_INFO info = pvStructInfo;

    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, NULL, pvStructInfo, pcbStructInfo,
     pcbDecoded, info ? info->Algorithm.Parameters.pbData : NULL);
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodePubKeyInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;

    __TRY
    {
        DWORD bytesNeeded = 0;

        if ((ret = CRYPT_AsnDecodePubKeyInfoInternal(pbEncoded, cbEncoded,
         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, &bytesNeeded, NULL)))
        {
            if (!pvStructInfo)
                *pcbStructInfo = bytesNeeded;
            else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
             pvStructInfo, pcbStructInfo, bytesNeeded)))
            {
                PCERT_PUBLIC_KEY_INFO info;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                info = pvStructInfo;
                info->Algorithm.Parameters.pbData = (BYTE *)pvStructInfo +
                 sizeof(CERT_PUBLIC_KEY_INFO);
                ret = CRYPT_AsnDecodePubKeyInfoInternal(pbEncoded, cbEncoded,
                 dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, pvStructInfo,
                 &bytesNeeded, NULL);
                if (!ret && (dwFlags & CRYPT_DECODE_ALLOC_FLAG))
                    CRYPT_FreeSpace(pDecodePara, info);
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

static BOOL CRYPT_AsnDecodeBool(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded)
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
    if (pcbDecoded)
        *pcbDecoded = 3;
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
        *pcbStructInfo = sizeof(BOOL);
        *(BOOL *)pvStructInfo = pbEncoded[2] != 0;
        ret = TRUE;
    }
    TRACE("returning %d (%08x)\n", ret, GetLastError());
    return ret;
}

static BOOL CRYPT_AsnDecodeAltNameEntry(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded)
{
    PCERT_ALT_NAME_ENTRY entry = pvStructInfo;
    DWORD dataLen, lenBytes, bytesNeeded = sizeof(CERT_ALT_NAME_ENTRY);
    BOOL ret;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    if (cbEncoded < 2)
    {
        SetLastError(CRYPT_E_ASN1_CORRUPT);
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
            if (memchr(pbEncoded + 1 + lenBytes, 0, dataLen))
            {
                SetLastError(CRYPT_E_ASN1_RULE);
                ret = FALSE;
            }
            else
                bytesNeeded += (dataLen + 1) * sizeof(WCHAR);
            break;
        case 4: /* directoryName */
        case 7: /* iPAddress */
            bytesNeeded += dataLen;
            break;
        case 8: /* registeredID */
            ret = CRYPT_AsnDecodeOidIgnoreTag(pbEncoded, cbEncoded, 0, NULL,
             &dataLen, NULL);
            if (ret)
            {
                /* FIXME: ugly, shouldn't need to know internals of OID decode
                 * function to use it.
                 */
                bytesNeeded += dataLen - sizeof(LPSTR);
            }
            break;
        case 0: /* otherName */
            FIXME("%d: stub\n", pbEncoded[0] & ASN_TYPE_MASK);
            SetLastError(CRYPT_E_ASN1_BADTAG);
            ret = FALSE;
            break;
        case 3: /* x400Address, unimplemented */
        case 5: /* ediPartyName, unimplemented */
            TRACE("type %d unimplemented\n", pbEncoded[0] & ASN_TYPE_MASK);
            SetLastError(CRYPT_E_ASN1_BADTAG);
            ret = FALSE;
            break;
        default:
            TRACE("type %d bad\n", pbEncoded[0] & ASN_TYPE_MASK);
            SetLastError(CRYPT_E_ASN1_CORRUPT);
            ret = FALSE;
        }
        if (ret)
        {
            if (pcbDecoded)
                *pcbDecoded = 1 + lenBytes + dataLen;
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
                entry->dwAltNameChoice = (pbEncoded[0] & ASN_TYPE_MASK) + 1;
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
                case 4: /* directoryName */
                    /* The data are memory-equivalent with the IPAddress case,
                     * fall-through
                     */
                case 7: /* iPAddress */
                    /* The next data pointer is in the pwszURL spot, that is,
                     * the first 4 bytes.  Need to move it to the next spot.
                     */
                    entry->u.IPAddress.pbData = (LPBYTE)entry->u.pwszURL;
                    entry->u.IPAddress.cbData = dataLen;
                    memcpy(entry->u.IPAddress.pbData, pbEncoded + 1 + lenBytes,
                     dataLen);
                    break;
                case 8: /* registeredID */
                    ret = CRYPT_AsnDecodeOidIgnoreTag(pbEncoded, cbEncoded, 0,
                     &entry->u.pszRegisteredID, &dataLen, NULL);
                    break;
                }
            }
        }
    }
    return ret;
}

static BOOL CRYPT_AsnDecodeAltNameInternal(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;
    struct AsnArrayDescriptor arrayDesc = { 0,
     offsetof(CERT_ALT_NAME_INFO, cAltEntry),
     offsetof(CERT_ALT_NAME_INFO, rgAltEntry),
     sizeof(CERT_ALT_NAME_INFO),
     CRYPT_AsnDecodeAltNameEntry, sizeof(CERT_ALT_NAME_ENTRY), TRUE,
     offsetof(CERT_ALT_NAME_ENTRY, u.pwszURL) };

    TRACE("%p, %d, %08x, %p, %d, %p\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo, pcbDecoded);

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded, dwFlags,
     NULL, pvStructInfo, pcbStructInfo, pcbDecoded);
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
           CRYPT_AsnDecodeOctets, sizeof(CRYPT_DATA_BLOB),
           TRUE, TRUE, offsetof(CERT_AUTHORITY_KEY_ID_INFO, KeyId.pbData), 0 },
         { ASN_CONTEXT | ASN_CONSTRUCTOR| 1,
           offsetof(CERT_AUTHORITY_KEY_ID_INFO, CertIssuer),
           CRYPT_AsnDecodeOctets, sizeof(CERT_NAME_BLOB), TRUE, TRUE,
           offsetof(CERT_AUTHORITY_KEY_ID_INFO, CertIssuer.pbData), 0 },
         { ASN_CONTEXT | 2, offsetof(CERT_AUTHORITY_KEY_ID_INFO,
           CertSerialNumber), CRYPT_AsnDecodeIntegerInternal,
           sizeof(CRYPT_INTEGER_BLOB), TRUE, TRUE,
           offsetof(CERT_AUTHORITY_KEY_ID_INFO, CertSerialNumber.pbData), 0 },
        };

        ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
         pbEncoded, cbEncoded, dwFlags, pDecodePara, pvStructInfo,
         pcbStructInfo, NULL, NULL);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeAuthorityKeyId2(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    __TRY
    {
        struct AsnDecodeSequenceItem items[] = {
         { ASN_CONTEXT | 0, offsetof(CERT_AUTHORITY_KEY_ID2_INFO, KeyId),
           CRYPT_AsnDecodeOctets, sizeof(CRYPT_DATA_BLOB),
           TRUE, TRUE, offsetof(CERT_AUTHORITY_KEY_ID2_INFO, KeyId.pbData), 0 },
         { ASN_CONTEXT | ASN_CONSTRUCTOR| 1,
           offsetof(CERT_AUTHORITY_KEY_ID2_INFO, AuthorityCertIssuer),
           CRYPT_AsnDecodeAltNameInternal, sizeof(CERT_ALT_NAME_INFO), TRUE,
           TRUE, offsetof(CERT_AUTHORITY_KEY_ID2_INFO,
           AuthorityCertIssuer.rgAltEntry), 0 },
         { ASN_CONTEXT | 2, offsetof(CERT_AUTHORITY_KEY_ID2_INFO,
           AuthorityCertSerialNumber), CRYPT_AsnDecodeIntegerInternal,
           sizeof(CRYPT_INTEGER_BLOB), TRUE, TRUE,
           offsetof(CERT_AUTHORITY_KEY_ID2_INFO,
           AuthorityCertSerialNumber.pbData), 0 },
        };

        ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
         pbEncoded, cbEncoded, dwFlags, pDecodePara, pvStructInfo,
         pcbStructInfo, NULL, NULL);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL CRYPT_AsnDecodeAccessDescription(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    struct AsnDecodeSequenceItem items[] = {
     { 0, offsetof(CERT_ACCESS_DESCRIPTION, pszAccessMethod),
       CRYPT_AsnDecodeOidInternal, sizeof(LPSTR), FALSE, TRUE,
       offsetof(CERT_ACCESS_DESCRIPTION, pszAccessMethod), 0 },
     { 0, offsetof(CERT_ACCESS_DESCRIPTION, AccessLocation),
       CRYPT_AsnDecodeAltNameEntry, sizeof(CERT_ALT_NAME_ENTRY), FALSE,
       TRUE, offsetof(CERT_ACCESS_DESCRIPTION, AccessLocation.u.pwszURL), 0 },
    };
    CERT_ACCESS_DESCRIPTION *descr = pvStructInfo;

    return CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, NULL, pvStructInfo, pcbStructInfo,
     pcbDecoded, descr ? descr->pszAccessMethod : NULL);
}

static BOOL WINAPI CRYPT_AsnDecodeAuthorityInfoAccess(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    __TRY
    {
        struct AsnArrayDescriptor arrayDesc = { ASN_SEQUENCEOF,
         offsetof(CERT_AUTHORITY_INFO_ACCESS, cAccDescr),
         offsetof(CERT_AUTHORITY_INFO_ACCESS, rgAccDescr),
         sizeof(CERT_AUTHORITY_INFO_ACCESS),
         CRYPT_AsnDecodeAccessDescription, sizeof(CERT_ACCESS_DESCRIPTION),
         TRUE, offsetof(CERT_ACCESS_DESCRIPTION, pszAccessMethod) };
        CERT_AUTHORITY_INFO_ACCESS *info = pvStructInfo;

        if (pvStructInfo && !(dwFlags & CRYPT_DECODE_ALLOC_FLAG))
            info->rgAccDescr = (CERT_ACCESS_DESCRIPTION *)(info + 1);
        ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
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

static BOOL CRYPT_AsnDecodePKCSContent(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded)
{
    BOOL ret;
    DWORD dataLen;

    TRACE("%p, %d, %08x, %p, %d, %p\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo, pcbDecoded);

    /* The caller has already checked the tag, no need to check it again.
     * Check the outer length is valid:
     */
    if ((ret = CRYPT_GetLengthIndefinite(pbEncoded, cbEncoded, &dataLen)))
    {
        BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);
        DWORD innerLen;

        pbEncoded += 1 + lenBytes;
        cbEncoded -= 1 + lenBytes;
        if (dataLen == CMSG_INDEFINITE_LENGTH)
            cbEncoded -= 2; /* space for 0 TLV */
        /* Check the inner length is valid: */
        if ((ret = CRYPT_GetLengthIndefinite(pbEncoded, cbEncoded, &innerLen)))
        {
            DWORD decodedLen;

            ret = CRYPT_AsnDecodeCopyBytes(pbEncoded, cbEncoded, dwFlags,
             pvStructInfo, pcbStructInfo, &decodedLen);
            if (dataLen == CMSG_INDEFINITE_LENGTH)
            {
                if (*(pbEncoded + decodedLen) != 0 ||
                 *(pbEncoded + decodedLen + 1) != 0)
                {
                    TRACE("expected 0 TLV, got {%02x,%02x}\n",
                     *(pbEncoded + decodedLen),
                     *(pbEncoded + decodedLen + 1));
                    SetLastError(CRYPT_E_ASN1_CORRUPT);
                    ret = FALSE;
                }
                else
                    decodedLen += 2;
            }
            if (ret && pcbDecoded)
            {
                *pcbDecoded = 1 + lenBytes + decodedLen;
                TRACE("decoded %d bytes\n", *pcbDecoded);
            }
        }
    }
    return ret;
}

static BOOL CRYPT_AsnDecodePKCSContentInfoInternal(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    CRYPT_CONTENT_INFO *info = pvStructInfo;
    struct AsnDecodeSequenceItem items[] = {
     { ASN_OBJECTIDENTIFIER, offsetof(CRYPT_CONTENT_INFO, pszObjId),
       CRYPT_AsnDecodeOidIgnoreTag, sizeof(LPSTR), FALSE, TRUE,
       offsetof(CRYPT_CONTENT_INFO, pszObjId), 0 },
     { ASN_CONTEXT | ASN_CONSTRUCTOR | 0,
       offsetof(CRYPT_CONTENT_INFO, Content), CRYPT_AsnDecodePKCSContent,
       sizeof(CRYPT_DER_BLOB), TRUE, TRUE,
       offsetof(CRYPT_CONTENT_INFO, Content.pbData), 0 },
    };
    BOOL ret;

    TRACE("%p, %d, %08x, %p, %d, %p\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo, pcbDecoded);

    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, NULL, pvStructInfo, pcbStructInfo,
     pcbDecoded, info ? info->pszObjId : NULL);
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodePKCSContentInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    __TRY
    {
        ret = CRYPT_AsnDecodePKCSContentInfoInternal(pbEncoded, cbEncoded,
         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, pcbStructInfo, NULL);
        if (ret && pvStructInfo)
        {
            ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara, pvStructInfo,
             pcbStructInfo, *pcbStructInfo);
            if (ret)
            {
                CRYPT_CONTENT_INFO *info;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                info = pvStructInfo;
                info->pszObjId = (LPSTR)((BYTE *)info +
                 sizeof(CRYPT_CONTENT_INFO));
                ret = CRYPT_AsnDecodePKCSContentInfoInternal(pbEncoded,
                 cbEncoded, dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, pvStructInfo,
                 pcbStructInfo, NULL);
                if (!ret && (dwFlags & CRYPT_DECODE_ALLOC_FLAG))
                    CRYPT_FreeSpace(pDecodePara, info);
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

BOOL CRYPT_AsnDecodePKCSDigestedData(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, PCRYPT_DECODE_PARA pDecodePara,
 CRYPT_DIGESTED_DATA *digestedData, DWORD *pcbDigestedData)
{
    BOOL ret;
    struct AsnDecodeSequenceItem items[] = {
     { ASN_INTEGER, offsetof(CRYPT_DIGESTED_DATA, version),
       CRYPT_AsnDecodeIntInternal, sizeof(DWORD), FALSE, FALSE, 0, 0 },
     { ASN_SEQUENCEOF, offsetof(CRYPT_DIGESTED_DATA, DigestAlgorithm),
       CRYPT_AsnDecodeAlgorithmId, sizeof(CRYPT_ALGORITHM_IDENTIFIER),
       FALSE, TRUE, offsetof(CRYPT_DIGESTED_DATA, DigestAlgorithm.pszObjId),
       0 },
     { ASN_SEQUENCEOF, offsetof(CRYPT_DIGESTED_DATA, ContentInfo),
       CRYPT_AsnDecodePKCSContentInfoInternal,
       sizeof(CRYPT_CONTENT_INFO), FALSE, TRUE, offsetof(CRYPT_DIGESTED_DATA,
       ContentInfo.pszObjId), 0 },
     { ASN_OCTETSTRING, offsetof(CRYPT_DIGESTED_DATA, hash),
       CRYPT_AsnDecodeOctets, sizeof(CRYPT_HASH_BLOB), FALSE, TRUE,
       offsetof(CRYPT_DIGESTED_DATA, hash.pbData), 0 },
    };

    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, pDecodePara, digestedData, pcbDigestedData,
     NULL, NULL);
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
        DWORD bytesNeeded = 0;

        if ((ret = CRYPT_AsnDecodeAltNameInternal(pbEncoded, cbEncoded,
         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, &bytesNeeded, NULL)))
        {
            if (!pvStructInfo)
                *pcbStructInfo = bytesNeeded;
            else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
             pvStructInfo, pcbStructInfo, bytesNeeded)))
            {
                CERT_ALT_NAME_INFO *name;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                name = pvStructInfo;
                name->rgAltEntry = (PCERT_ALT_NAME_ENTRY)
                 ((BYTE *)pvStructInfo + sizeof(CERT_ALT_NAME_INFO));
                ret = CRYPT_AsnDecodeAltNameInternal(pbEncoded, cbEncoded,
                 dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, pvStructInfo,
                 &bytesNeeded, NULL);
                if (!ret && (dwFlags & CRYPT_DECODE_ALLOC_FLAG))
                    CRYPT_FreeSpace(pDecodePara, name);
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

struct PATH_LEN_CONSTRAINT
{
    BOOL  fPathLenConstraint;
    DWORD dwPathLenConstraint;
};

static BOOL CRYPT_AsnDecodePathLenConstraint(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret = TRUE;
    DWORD bytesNeeded = sizeof(struct PATH_LEN_CONSTRAINT), size;

    TRACE("%p, %d, %08x, %p, %d, %p\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo, pcbDecoded);

    if (!pvStructInfo)
    {
        ret = CRYPT_AsnDecodeIntInternal(pbEncoded, cbEncoded, dwFlags, NULL,
         &size, pcbDecoded);
        *pcbStructInfo = bytesNeeded;
    }
    else if (*pcbStructInfo < bytesNeeded)
    {
        SetLastError(ERROR_MORE_DATA);
        *pcbStructInfo = bytesNeeded;
        ret = FALSE;
    }
    else
    {
        struct PATH_LEN_CONSTRAINT *constraint = pvStructInfo;

        *pcbStructInfo = bytesNeeded;
        size = sizeof(constraint->dwPathLenConstraint);
        ret = CRYPT_AsnDecodeIntInternal(pbEncoded, cbEncoded, dwFlags,
         &constraint->dwPathLenConstraint, &size, pcbDecoded);
        if (ret)
            constraint->fPathLenConstraint = TRUE;
        TRACE("got an int, dwPathLenConstraint is %d\n",
         constraint->dwPathLenConstraint);
    }
    TRACE("returning %d (%08x)\n", ret, GetLastError());
    return ret;
}

static BOOL CRYPT_AsnDecodeSubtreeConstraints(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;
    struct AsnArrayDescriptor arrayDesc = { ASN_SEQUENCEOF,
     offsetof(CERT_BASIC_CONSTRAINTS_INFO, cSubtreesConstraint),
     offsetof(CERT_BASIC_CONSTRAINTS_INFO, rgSubtreesConstraint),
     FINALMEMBERSIZE(CERT_BASIC_CONSTRAINTS_INFO, cSubtreesConstraint),
     CRYPT_AsnDecodeCopyBytes, sizeof(CERT_NAME_BLOB), TRUE,
     offsetof(CERT_NAME_BLOB, pbData) };

    TRACE("%p, %d, %08x, %p, %d, %p\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo, pcbDecoded);

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
     dwFlags, NULL, pvStructInfo, pcbStructInfo, pcbDecoded);
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
           FINALMEMBERSIZE(CERT_BASIC_CONSTRAINTS_INFO, cSubtreesConstraint),
           TRUE, TRUE,
           offsetof(CERT_BASIC_CONSTRAINTS_INFO, rgSubtreesConstraint), 0 },
        };

        ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
         pbEncoded, cbEncoded, dwFlags, pDecodePara, pvStructInfo,
         pcbStructInfo, NULL, NULL);
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

        ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
         pbEncoded, cbEncoded, dwFlags, pDecodePara, pvStructInfo,
         pcbStructInfo, NULL, NULL);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL CRYPT_AsnDecodePolicyQualifier(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    struct AsnDecodeSequenceItem items[] = {
     { ASN_OBJECTIDENTIFIER, offsetof(CERT_POLICY_QUALIFIER_INFO,
       pszPolicyQualifierId), CRYPT_AsnDecodeOidInternal, sizeof(LPSTR),
       FALSE, TRUE, offsetof(CERT_POLICY_QUALIFIER_INFO, pszPolicyQualifierId),
       0 },
     { 0, offsetof(CERT_POLICY_QUALIFIER_INFO, Qualifier),
       CRYPT_AsnDecodeDerBlob, sizeof(CRYPT_OBJID_BLOB), TRUE, TRUE,
       offsetof(CERT_POLICY_QUALIFIER_INFO, Qualifier.pbData), 0 },
    };
    BOOL ret;
    CERT_POLICY_QUALIFIER_INFO *qualifier = pvStructInfo;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, pvStructInfo ? *pcbStructInfo : 0);

    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, NULL, pvStructInfo, pcbStructInfo,
     pcbDecoded, qualifier ? qualifier->pszPolicyQualifierId : NULL);
    return ret;
}

static BOOL CRYPT_AsnDecodePolicyQualifiers(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;
    struct AsnArrayDescriptor arrayDesc = { ASN_SEQUENCEOF,
     offsetof(CERT_POLICY_INFO, cPolicyQualifier),
     offsetof(CERT_POLICY_INFO, rgPolicyQualifier),
     FINALMEMBERSIZE(CERT_POLICY_INFO, cPolicyQualifier),
     CRYPT_AsnDecodePolicyQualifier, sizeof(CERT_POLICY_QUALIFIER_INFO), TRUE,
     offsetof(CERT_POLICY_QUALIFIER_INFO, pszPolicyQualifierId) };

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, pvStructInfo ? *pcbStructInfo : 0);

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
     dwFlags, NULL, pvStructInfo, pcbStructInfo, pcbDecoded);
    TRACE("Returning %d (%08x)\n", ret, GetLastError());
    return ret;
}

static BOOL CRYPT_AsnDecodeCertPolicy(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded)
{
    struct AsnDecodeSequenceItem items[] = {
     { ASN_OBJECTIDENTIFIER, offsetof(CERT_POLICY_INFO, pszPolicyIdentifier),
       CRYPT_AsnDecodeOidInternal, sizeof(LPSTR), FALSE, TRUE,
       offsetof(CERT_POLICY_INFO, pszPolicyIdentifier), 0 },
     { ASN_SEQUENCEOF, offsetof(CERT_POLICY_INFO, cPolicyQualifier),
       CRYPT_AsnDecodePolicyQualifiers,
       FINALMEMBERSIZE(CERT_POLICY_INFO, cPolicyQualifier), TRUE,
       TRUE, offsetof(CERT_POLICY_INFO, rgPolicyQualifier), 0 },
    };
    CERT_POLICY_INFO *info = pvStructInfo;
    BOOL ret;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, pvStructInfo ? *pcbStructInfo : 0);

    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, NULL, pvStructInfo, pcbStructInfo,
     pcbDecoded, info ? info->pszPolicyIdentifier : NULL);
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeCertPolicies(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, pvStructInfo ? *pcbStructInfo : 0);

    __TRY
    {
        struct AsnArrayDescriptor arrayDesc = { ASN_SEQUENCEOF,
         offsetof(CERT_POLICIES_INFO, cPolicyInfo),
         offsetof(CERT_POLICIES_INFO, rgPolicyInfo),
         sizeof(CERT_POLICIES_INFO),
         CRYPT_AsnDecodeCertPolicy, sizeof(CERT_POLICY_INFO), TRUE,
         offsetof(CERT_POLICY_INFO, pszPolicyIdentifier) };
        CERT_POLICIES_INFO *info = pvStructInfo;

        if (pvStructInfo && !(dwFlags & CRYPT_DECODE_ALLOC_FLAG))
            info->rgPolicyInfo = (CERT_POLICY_INFO *)(info + 1);

        ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
         dwFlags, pDecodePara, pvStructInfo, pcbStructInfo, NULL);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    return ret;
}

static BOOL CRYPT_AsnDecodeCertPolicyMapping(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    struct AsnDecodeSequenceItem items[] = {
     { ASN_OBJECTIDENTIFIER, offsetof(CERT_POLICY_MAPPING,
       pszIssuerDomainPolicy), CRYPT_AsnDecodeOidInternal, sizeof(LPSTR),
       FALSE, TRUE, offsetof(CERT_POLICY_MAPPING, pszIssuerDomainPolicy), 0 },
     { ASN_OBJECTIDENTIFIER, offsetof(CERT_POLICY_MAPPING,
       pszSubjectDomainPolicy), CRYPT_AsnDecodeOidInternal, sizeof(LPSTR),
       FALSE, TRUE, offsetof(CERT_POLICY_MAPPING, pszSubjectDomainPolicy), 0 },
    };
    CERT_POLICY_MAPPING *mapping = pvStructInfo;
    BOOL ret;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, pvStructInfo ? *pcbStructInfo : 0);

    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, NULL, pvStructInfo, pcbStructInfo,
     pcbDecoded, mapping ? mapping->pszIssuerDomainPolicy : NULL);
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeCertPolicyMappings(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, pvStructInfo ? *pcbStructInfo : 0);

    __TRY
    {
        struct AsnArrayDescriptor arrayDesc = { ASN_SEQUENCEOF,
         offsetof(CERT_POLICY_MAPPINGS_INFO, cPolicyMapping),
         offsetof(CERT_POLICY_MAPPINGS_INFO, rgPolicyMapping),
         sizeof(CERT_POLICY_MAPPING),
         CRYPT_AsnDecodeCertPolicyMapping, sizeof(CERT_POLICY_MAPPING), TRUE,
         offsetof(CERT_POLICY_MAPPING, pszIssuerDomainPolicy) };
        CERT_POLICY_MAPPINGS_INFO *info = pvStructInfo;

        if (pvStructInfo && !(dwFlags & CRYPT_DECODE_ALLOC_FLAG))
            info->rgPolicyMapping = (CERT_POLICY_MAPPING *)(info + 1);
        ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
         dwFlags, pDecodePara, pvStructInfo, pcbStructInfo, NULL);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    return ret;
}

static BOOL CRYPT_AsnDecodeRequireExplicit(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;
    DWORD skip, size = sizeof(skip);

    if (!cbEncoded)
    {
        SetLastError(CRYPT_E_ASN1_EOD);
        return FALSE;
    }
    if (pbEncoded[0] != (ASN_CONTEXT | 0))
    {
        SetLastError(CRYPT_E_ASN1_BADTAG);
        return FALSE;
    }
    if ((ret = CRYPT_AsnDecodeIntInternal(pbEncoded, cbEncoded, dwFlags,
     &skip, &size, pcbDecoded)))
    {
        DWORD bytesNeeded = MEMBERSIZE(CERT_POLICY_CONSTRAINTS_INFO,
         fRequireExplicitPolicy, fInhibitPolicyMapping);

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
            CERT_POLICY_CONSTRAINTS_INFO *info = CONTAINING_RECORD(pvStructInfo,
              CERT_POLICY_CONSTRAINTS_INFO, fRequireExplicitPolicy);

            *pcbStructInfo = bytesNeeded;
            /* The BOOL is implicit:  if the integer is present, then it's
             * TRUE.
             */
            info->fRequireExplicitPolicy = TRUE;
            info->dwRequireExplicitPolicySkipCerts = skip;
        }
    }
    return ret;
}

static BOOL CRYPT_AsnDecodeInhibitMapping(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;
    DWORD skip, size = sizeof(skip);

    if (!cbEncoded)
    {
        SetLastError(CRYPT_E_ASN1_EOD);
        return FALSE;
    }
    if (pbEncoded[0] != (ASN_CONTEXT | 1))
    {
        SetLastError(CRYPT_E_ASN1_BADTAG);
        return FALSE;
    }
    if ((ret = CRYPT_AsnDecodeIntInternal(pbEncoded, cbEncoded, dwFlags,
     &skip, &size, pcbDecoded)))
    {
        DWORD bytesNeeded = FINALMEMBERSIZE(CERT_POLICY_CONSTRAINTS_INFO,
         fInhibitPolicyMapping);

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
            CERT_POLICY_CONSTRAINTS_INFO *info = CONTAINING_RECORD(pvStructInfo,
              CERT_POLICY_CONSTRAINTS_INFO, fInhibitPolicyMapping);

            *pcbStructInfo = bytesNeeded;
            /* The BOOL is implicit:  if the integer is present, then it's
             * TRUE.
             */
            info->fInhibitPolicyMapping = TRUE;
            info->dwInhibitPolicyMappingSkipCerts = skip;
        }
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeCertPolicyConstraints(
 DWORD dwCertEncodingType, LPCSTR lpszStructType, const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, PCRYPT_DECODE_PARA pDecodePara,
 void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, pvStructInfo ? *pcbStructInfo : 0);

    __TRY
    {
        struct AsnDecodeSequenceItem items[] = {
         { ASN_CONTEXT | 0,
           offsetof(CERT_POLICY_CONSTRAINTS_INFO, fRequireExplicitPolicy),
           CRYPT_AsnDecodeRequireExplicit,
           MEMBERSIZE(CERT_POLICY_CONSTRAINTS_INFO, fRequireExplicitPolicy,
           fInhibitPolicyMapping), TRUE, FALSE, 0, 0 },
         { ASN_CONTEXT | 1,
           offsetof(CERT_POLICY_CONSTRAINTS_INFO, fInhibitPolicyMapping),
           CRYPT_AsnDecodeInhibitMapping,
           FINALMEMBERSIZE(CERT_POLICY_CONSTRAINTS_INFO, fInhibitPolicyMapping),
           TRUE, FALSE, 0, 0 },
        };

        ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
         pbEncoded, cbEncoded, dwFlags, pDecodePara, pvStructInfo,
         pcbStructInfo, NULL, NULL);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
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
           CRYPT_AsnDecodeIntInternal, sizeof(DWORD), FALSE, FALSE, 0, 0 },
        };
        struct DECODED_RSA_PUB_KEY *decodedKey = NULL;
        DWORD size = 0;

        ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
         pbEncoded, cbEncoded, CRYPT_DECODE_ALLOC_FLAG, NULL, &decodedKey,
         &size, NULL, NULL);
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
                hdr = pvStructInfo;
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

#define RSA2_MAGIC 0x32415352

struct DECODED_RSA_PRIV_KEY
{
    DWORD              version;
    DWORD              pubexp;
    CRYPT_INTEGER_BLOB modulus;
    CRYPT_INTEGER_BLOB privexp;
    CRYPT_INTEGER_BLOB prime1;
    CRYPT_INTEGER_BLOB prime2;
    CRYPT_INTEGER_BLOB exponent1;
    CRYPT_INTEGER_BLOB exponent2;
    CRYPT_INTEGER_BLOB coefficient;
};

static BOOL WINAPI CRYPT_AsnDecodeRsaPrivKey(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;
    DWORD halflen;

    __TRY
    {
        struct AsnDecodeSequenceItem items[] = {
         { ASN_INTEGER, offsetof(struct DECODED_RSA_PRIV_KEY, version),
           CRYPT_AsnDecodeIntInternal, sizeof(DWORD), FALSE, FALSE, 0, 0 },
         { ASN_INTEGER, offsetof(struct DECODED_RSA_PRIV_KEY, modulus),
           CRYPT_AsnDecodeUnsignedIntegerInternal, sizeof(CRYPT_INTEGER_BLOB),
           FALSE, TRUE, offsetof(struct DECODED_RSA_PRIV_KEY, modulus.pbData),
           0 },
         { ASN_INTEGER, offsetof(struct DECODED_RSA_PRIV_KEY, pubexp),
           CRYPT_AsnDecodeIntInternal, sizeof(DWORD), FALSE, FALSE, 0, 0 },
         { ASN_INTEGER, offsetof(struct DECODED_RSA_PRIV_KEY, privexp),
           CRYPT_AsnDecodeUnsignedIntegerInternal, sizeof(CRYPT_INTEGER_BLOB),
           FALSE, TRUE, offsetof(struct DECODED_RSA_PRIV_KEY, privexp.pbData),
           0 },
         { ASN_INTEGER, offsetof(struct DECODED_RSA_PRIV_KEY, prime1),
           CRYPT_AsnDecodeUnsignedIntegerInternal, sizeof(CRYPT_INTEGER_BLOB),
           FALSE, TRUE, offsetof(struct DECODED_RSA_PRIV_KEY, prime1.pbData),
           0 },
         { ASN_INTEGER, offsetof(struct DECODED_RSA_PRIV_KEY, prime2),
           CRYPT_AsnDecodeUnsignedIntegerInternal, sizeof(CRYPT_INTEGER_BLOB),
           FALSE, TRUE, offsetof(struct DECODED_RSA_PRIV_KEY, prime2.pbData),
           0 },
         { ASN_INTEGER, offsetof(struct DECODED_RSA_PRIV_KEY, exponent1),
           CRYPT_AsnDecodeUnsignedIntegerInternal, sizeof(CRYPT_INTEGER_BLOB),
           FALSE, TRUE, offsetof(struct DECODED_RSA_PRIV_KEY, exponent1.pbData),
           0 },
         { ASN_INTEGER, offsetof(struct DECODED_RSA_PRIV_KEY, exponent2),
           CRYPT_AsnDecodeUnsignedIntegerInternal, sizeof(CRYPT_INTEGER_BLOB),
           FALSE, TRUE, offsetof(struct DECODED_RSA_PRIV_KEY, exponent2.pbData),
           0 },
         { ASN_INTEGER, offsetof(struct DECODED_RSA_PRIV_KEY, coefficient),
           CRYPT_AsnDecodeUnsignedIntegerInternal, sizeof(CRYPT_INTEGER_BLOB),
           FALSE, TRUE, offsetof(struct DECODED_RSA_PRIV_KEY, coefficient.pbData),
           0 },
        };
        struct DECODED_RSA_PRIV_KEY *decodedKey = NULL;
        DWORD size = 0;

        ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
         pbEncoded, cbEncoded, CRYPT_DECODE_ALLOC_FLAG, NULL, &decodedKey,
         &size, NULL, NULL);
        if (ret)
        {
            halflen = decodedKey->prime1.cbData;
            if (halflen < decodedKey->prime2.cbData)
                halflen = decodedKey->prime2.cbData;
            if (halflen < decodedKey->exponent1.cbData)
                halflen = decodedKey->exponent1.cbData;
            if (halflen < decodedKey->exponent2.cbData)
                halflen = decodedKey->exponent2.cbData;
            if (halflen < decodedKey->coefficient.cbData)
                halflen = decodedKey->coefficient.cbData;
            if (halflen * 2 < decodedKey->modulus.cbData)
                halflen = decodedKey->modulus.cbData / 2 + decodedKey->modulus.cbData % 2;
            if (halflen * 2 < decodedKey->privexp.cbData)
                halflen = decodedKey->privexp.cbData / 2 + decodedKey->privexp.cbData % 2;

            if (ret)
            {
                DWORD bytesNeeded = sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) +
                 (halflen * 9);

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
                    BYTE *vardata;

                    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                        pvStructInfo = *(BYTE **)pvStructInfo;

                    hdr = pvStructInfo;
                    hdr->bType = PRIVATEKEYBLOB;
                    hdr->bVersion = CUR_BLOB_VERSION;
                    hdr->reserved = 0;
                    hdr->aiKeyAlg = CALG_RSA_KEYX;

                    rsaPubKey = (RSAPUBKEY *)((BYTE *)pvStructInfo +
                     sizeof(BLOBHEADER));
                    rsaPubKey->magic = RSA2_MAGIC;
                    rsaPubKey->pubexp = decodedKey->pubexp;
                    rsaPubKey->bitlen = halflen * 16;

                    vardata = (BYTE*)(rsaPubKey + 1);
                    memset(vardata, 0, halflen * 9);
                    memcpy(vardata,
                     decodedKey->modulus.pbData, decodedKey->modulus.cbData);
                    memcpy(vardata + halflen * 2,
                     decodedKey->prime1.pbData, decodedKey->prime1.cbData);
                    memcpy(vardata + halflen * 3,
                     decodedKey->prime2.pbData, decodedKey->prime2.cbData);
                    memcpy(vardata + halflen * 4,
                     decodedKey->exponent1.pbData, decodedKey->exponent1.cbData);
                    memcpy(vardata + halflen * 5,
                     decodedKey->exponent2.pbData, decodedKey->exponent2.cbData);
                    memcpy(vardata + halflen * 6,
                     decodedKey->coefficient.pbData, decodedKey->coefficient.cbData);
                    memcpy(vardata + halflen * 7,
                     decodedKey->privexp.pbData, decodedKey->privexp.cbData);
                }
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

static BOOL CRYPT_AsnDecodeOctets(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;
    DWORD bytesNeeded, dataLen;

    TRACE("%p, %d, %08x, %p, %d, %p\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo, pcbDecoded);

    if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
    {
        BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

        if (dwFlags & CRYPT_DECODE_NOCOPY_FLAG)
            bytesNeeded = sizeof(CRYPT_DATA_BLOB);
        else
            bytesNeeded = dataLen + sizeof(CRYPT_DATA_BLOB);
        if (pcbDecoded)
            *pcbDecoded = 1 + lenBytes + dataLen;
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

            *pcbStructInfo = bytesNeeded;
            blob = pvStructInfo;
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

static BOOL CRYPT_AsnDecodeOctetStringInternal(const BYTE *encoded, DWORD encoded_size,
    DWORD flags, void *buf, DWORD *buf_size, DWORD *ret_decoded)
{
    DWORD decoded = 0, indefinite_len_depth = 0, len_size, len, bytes_needed;
    CRYPT_DATA_BLOB *blob;
    const BYTE *string;

    while (encoded[0] == (ASN_CONSTRUCTOR | ASN_OCTETSTRING))
    {
        if (!CRYPT_GetLengthIndefinite(encoded, encoded_size, &len))
            return FALSE;

        len_size = GET_LEN_BYTES(encoded[1]);
        encoded += 1 + len_size;
        encoded_size -= 1 + len_size;
        decoded += 1 + len_size;

        if (len == CMSG_INDEFINITE_LENGTH)
        {
            indefinite_len_depth++;
            if (encoded_size < 2)
            {
                SetLastError(CRYPT_E_ASN1_EOD);
                return FALSE;
            }
            encoded_size -= 2;
            decoded += 2;
        }
    }

    if (encoded[0] != ASN_OCTETSTRING)
    {
        WARN("Unexpected tag %02x\n", encoded[0]);
        SetLastError(CRYPT_E_ASN1_BADTAG);
        return FALSE;
    }

    if (!CRYPT_GetLen(encoded, encoded_size, &len))
        return FALSE;
    len_size = GET_LEN_BYTES(encoded[1]);
    decoded += 1 + len_size + len;
    encoded_size -= 1 + len_size;

    if (len > encoded_size)
    {
        SetLastError(CRYPT_E_ASN1_EOD);
        return FALSE;
    }
    if (ret_decoded)
        *ret_decoded = decoded;

    encoded += 1 + len_size;
    string = encoded;
    encoded += len;

    while (indefinite_len_depth--)
    {
        if (encoded[0] || encoded[1])
        {
            TRACE("expected 0 TLV, got %02x %02x\n", encoded[0], encoded[1]);
            SetLastError(CRYPT_E_ASN1_CORRUPT);
            return FALSE;
        }
    }

    bytes_needed = sizeof(*blob);
    if (!(flags & CRYPT_DECODE_NOCOPY_FLAG)) bytes_needed += len;
    if (!buf)
    {
        *buf_size = bytes_needed;
        return TRUE;
    }
    if (*buf_size < bytes_needed)
    {
        SetLastError(ERROR_MORE_DATA);
        *buf_size = bytes_needed;
        return FALSE;
    }

    *buf_size = bytes_needed;
    blob = buf;
    blob->cbData = len;
    if (flags & CRYPT_DECODE_NOCOPY_FLAG)
        blob->pbData = (BYTE*)string;
    else if (blob->cbData)
        memcpy(blob->pbData, string, blob->cbData);

    if (ret_decoded)
        *ret_decoded = decoded;
    return TRUE;
}

static BOOL WINAPI CRYPT_AsnDecodeOctetString(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    if (!cbEncoded)
    {
        SetLastError(CRYPT_E_ASN1_CORRUPT);
        return FALSE;
    }

    __TRY
    {
        DWORD bytesNeeded = 0;

        if ((ret = CRYPT_AsnDecodeOctetStringInternal(pbEncoded, cbEncoded,
         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, &bytesNeeded, NULL)))
        {
            if (!pvStructInfo)
                *pcbStructInfo = bytesNeeded;
            else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
             pvStructInfo, pcbStructInfo, bytesNeeded)))
            {
                CRYPT_DATA_BLOB *blob;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                blob = pvStructInfo;
                blob->pbData = (BYTE *)pvStructInfo + sizeof(CRYPT_DATA_BLOB);
                ret = CRYPT_AsnDecodeOctetStringInternal(pbEncoded, cbEncoded,
                 dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, pvStructInfo,
                 &bytesNeeded, NULL);
                if (!ret && (dwFlags & CRYPT_DECODE_ALLOC_FLAG))
                    CRYPT_FreeSpace(pDecodePara, blob);
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

static BOOL CRYPT_AsnDecodeBitsInternal(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded)
{
    BOOL ret;
    DWORD bytesNeeded, dataLen;
    BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

    TRACE("(%p, %d, 0x%08x, %p, %d, %p)\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo, pcbDecoded);

    if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
    {
        if (dwFlags & CRYPT_DECODE_NOCOPY_FLAG)
            bytesNeeded = sizeof(CRYPT_BIT_BLOB);
        else
            bytesNeeded = dataLen - 1 + sizeof(CRYPT_BIT_BLOB);
        if (pcbDecoded)
            *pcbDecoded = 1 + lenBytes + dataLen;
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

            *pcbStructInfo = bytesNeeded;
            blob = pvStructInfo;
            blob->cbData = dataLen - 1;
            blob->cUnusedBits = *(pbEncoded + 1 + lenBytes);
            if (dwFlags & CRYPT_DECODE_NOCOPY_FLAG)
            {
                blob->pbData = (BYTE *)pbEncoded + 2 + lenBytes;
            }
            else
            {
                assert(blob->pbData);
                if (blob->cbData)
                {
                    BYTE mask = 0xff << blob->cUnusedBits;

                    memcpy(blob->pbData, pbEncoded + 2 + lenBytes,
                     blob->cbData);
                    blob->pbData[blob->cbData - 1] &= mask;
                }
            }
        }
    }
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
        DWORD bytesNeeded = 0;

        if (!cbEncoded)
        {
            SetLastError(CRYPT_E_ASN1_CORRUPT);
            ret = FALSE;
        }
        else if (pbEncoded[0] != ASN_BITSTRING)
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
            ret = FALSE;
        }
        else if ((ret = CRYPT_AsnDecodeBitsInternal(pbEncoded, cbEncoded,
         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, &bytesNeeded, NULL)))
        {
            if (!pvStructInfo)
                *pcbStructInfo = bytesNeeded;
            else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
             pvStructInfo, pcbStructInfo, bytesNeeded)))
            {
                CRYPT_BIT_BLOB *blob;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                blob = pvStructInfo;
                blob->pbData = (BYTE *)pvStructInfo + sizeof(CRYPT_BIT_BLOB);
                ret = CRYPT_AsnDecodeBitsInternal(pbEncoded, cbEncoded,
                 dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, pvStructInfo,
                 &bytesNeeded, NULL);
                if (!ret && (dwFlags & CRYPT_DECODE_ALLOC_FLAG))
                    CRYPT_FreeSpace(pDecodePara, blob);
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

/* Ignores tag.  Only allows integers 4 bytes or smaller in size. */
static BOOL CRYPT_AsnDecodeIntInternal(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded)
{
    BOOL ret;
    DWORD dataLen;

    if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
    {
        BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

        if (pcbDecoded)
            *pcbDecoded = 1 + lenBytes + dataLen;
        if (dataLen > sizeof(int))
        {
            SetLastError(CRYPT_E_ASN1_LARGE);
            ret = FALSE;
        }
        else if (!pvStructInfo)
            *pcbStructInfo = sizeof(int);
        else if ((ret = CRYPT_DecodeCheckSpace(pcbStructInfo, sizeof(int))))
        {
            int val, i;

            if (dataLen && pbEncoded[1 + lenBytes] & 0x80)
            {
                /* initialize to a negative value to sign-extend */
                val = -1;
            }
            else
                val = 0;
            for (i = 0; i < dataLen; i++)
            {
                val <<= 8;
                val |= pbEncoded[1 + lenBytes + i];
            }
            memcpy(pvStructInfo, &val, sizeof(int));
        }
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeInt(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    __TRY
    {
        DWORD bytesNeeded = 0;

        if (!cbEncoded)
        {
            SetLastError(CRYPT_E_ASN1_EOD);
            ret = FALSE;
        }
        else if (pbEncoded[0] != ASN_INTEGER)
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
            ret = FALSE;
        }
        else
            ret = CRYPT_AsnDecodeIntInternal(pbEncoded, cbEncoded,
             dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, &bytesNeeded, NULL);
        if (ret)
        {
            if (!pvStructInfo)
                *pcbStructInfo = bytesNeeded;
            else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
             pvStructInfo, pcbStructInfo, bytesNeeded)))
            {
                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                ret = CRYPT_AsnDecodeIntInternal(pbEncoded, cbEncoded,
                 dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, pvStructInfo,
                 &bytesNeeded, NULL);
                if (!ret && (dwFlags & CRYPT_DECODE_ALLOC_FLAG))
                    CRYPT_FreeSpace(pDecodePara, pvStructInfo);
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

static BOOL CRYPT_AsnDecodeIntegerInternal(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;
    DWORD bytesNeeded, dataLen;

    if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
    {
        BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

        bytesNeeded = dataLen + sizeof(CRYPT_INTEGER_BLOB);
        if (pcbDecoded)
            *pcbDecoded = 1 + lenBytes + dataLen;
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
            CRYPT_INTEGER_BLOB *blob = pvStructInfo;

            *pcbStructInfo = bytesNeeded;
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
        DWORD bytesNeeded = 0;

        if (pbEncoded[0] != ASN_INTEGER)
        {
            SetLastError(CRYPT_E_ASN1_BADTAG);
            ret = FALSE;
        }
        else
            ret = CRYPT_AsnDecodeIntegerInternal(pbEncoded, cbEncoded,
             dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG, NULL, &bytesNeeded, NULL);
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
                blob = pvStructInfo;
                blob->pbData = (BYTE *)pvStructInfo +
                 sizeof(CRYPT_INTEGER_BLOB);
                ret = CRYPT_AsnDecodeIntegerInternal(pbEncoded, cbEncoded,
                 dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG, pvStructInfo,
                 &bytesNeeded, NULL);
                if (!ret && (dwFlags & CRYPT_DECODE_ALLOC_FLAG))
                    CRYPT_FreeSpace(pDecodePara, blob);
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

static BOOL CRYPT_AsnDecodeUnsignedIntegerInternal(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;

    if (pbEncoded[0] == ASN_INTEGER)
    {
        DWORD bytesNeeded, dataLen;

        if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
        {
            BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

            if (pcbDecoded)
                *pcbDecoded = 1 + lenBytes + dataLen;
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
                CRYPT_INTEGER_BLOB *blob = pvStructInfo;

                *pcbStructInfo = bytesNeeded;
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
        DWORD bytesNeeded = 0;

        if ((ret = CRYPT_AsnDecodeUnsignedIntegerInternal(pbEncoded, cbEncoded,
         dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG, NULL, &bytesNeeded, NULL)))
        {
            if (!pvStructInfo)
                *pcbStructInfo = bytesNeeded;
            else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
             pvStructInfo, pcbStructInfo, bytesNeeded)))
            {
                CRYPT_INTEGER_BLOB *blob;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                blob = pvStructInfo;
                blob->pbData = (BYTE *)pvStructInfo +
                 sizeof(CRYPT_INTEGER_BLOB);
                ret = CRYPT_AsnDecodeUnsignedIntegerInternal(pbEncoded,
                 cbEncoded, dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG, pvStructInfo,
                 &bytesNeeded, NULL);
                if (!ret && (dwFlags & CRYPT_DECODE_ALLOC_FLAG))
                    CRYPT_FreeSpace(pDecodePara, blob);
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
    BOOL ret = TRUE;

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
    return ret;
}

#define MIN_ENCODED_TIME_LENGTH 10

static BOOL CRYPT_AsnDecodeUtcTimeInternal(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret = FALSE;

    if (pbEncoded[0] == ASN_UTCTIME)
    {
        if (cbEncoded <= 1)
            SetLastError(CRYPT_E_ASN1_EOD);
        else if (pbEncoded[1] > 0x7f)
        {
            /* long-form date strings really can't be valid */
            SetLastError(CRYPT_E_ASN1_CORRUPT);
        }
        else
        {
            SYSTEMTIME sysTime = { 0 };
            BYTE len = pbEncoded[1];

            if (len < MIN_ENCODED_TIME_LENGTH)
                SetLastError(CRYPT_E_ASN1_CORRUPT);
            else
            {
                ret = TRUE;
                if (pcbDecoded)
                    *pcbDecoded = 2 + len;
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
                if (ret)
                {
                    if (!pvStructInfo)
                        *pcbStructInfo = sizeof(FILETIME);
                    else if ((ret = CRYPT_DecodeCheckSpace(pcbStructInfo,
                     sizeof(FILETIME))))
                        ret = SystemTimeToFileTime(&sysTime, pvStructInfo);
                }
            }
        }
    }
    else
        SetLastError(CRYPT_E_ASN1_BADTAG);
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeUtcTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;

    __TRY
    {
        DWORD bytesNeeded = 0;

        ret = CRYPT_AsnDecodeUtcTimeInternal(pbEncoded, cbEncoded,
         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, &bytesNeeded, NULL);
        if (ret)
        {
            if (!pvStructInfo)
                *pcbStructInfo = bytesNeeded;
            else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags,
             pDecodePara, pvStructInfo, pcbStructInfo, bytesNeeded)))
            {
                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                ret = CRYPT_AsnDecodeUtcTimeInternal(pbEncoded, cbEncoded,
                 dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, pvStructInfo,
                 &bytesNeeded, NULL);
                if (!ret && (dwFlags & CRYPT_DECODE_ALLOC_FLAG))
                    CRYPT_FreeSpace(pDecodePara, pvStructInfo);
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

static BOOL CRYPT_AsnDecodeGeneralizedTime(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret = FALSE;

    if (pbEncoded[0] == ASN_GENERALTIME)
    {
        if (cbEncoded <= 1)
            SetLastError(CRYPT_E_ASN1_EOD);
        else if (pbEncoded[1] > 0x7f)
        {
            /* long-form date strings really can't be valid */
            SetLastError(CRYPT_E_ASN1_CORRUPT);
        }
        else
        {
            BYTE len = pbEncoded[1];

            if (len < MIN_ENCODED_TIME_LENGTH)
                SetLastError(CRYPT_E_ASN1_CORRUPT);
            else
            {
                SYSTEMTIME sysTime = { 0 };

                ret = TRUE;
                if (pcbDecoded)
                    *pcbDecoded = 2 + len;
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
                if (ret)
                {
                    if (!pvStructInfo)
                        *pcbStructInfo = sizeof(FILETIME);
                    else if ((ret = CRYPT_DecodeCheckSpace(pcbStructInfo,
                     sizeof(FILETIME))))
                        ret = SystemTimeToFileTime(&sysTime, pvStructInfo);
                }
            }
        }
    }
    else
        SetLastError(CRYPT_E_ASN1_BADTAG);
    return ret;
}

static BOOL CRYPT_AsnDecodeChoiceOfTimeInternal(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;
    InternalDecodeFunc decode = NULL;

    if (pbEncoded[0] == ASN_UTCTIME)
        decode = CRYPT_AsnDecodeUtcTimeInternal;
    else if (pbEncoded[0] == ASN_GENERALTIME)
        decode = CRYPT_AsnDecodeGeneralizedTime;
    if (decode)
        ret = decode(pbEncoded, cbEncoded, dwFlags, pvStructInfo,
         pcbStructInfo, pcbDecoded);
    else
    {
        SetLastError(CRYPT_E_ASN1_BADTAG);
        ret = FALSE;
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeChoiceOfTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    __TRY
    {
        DWORD bytesNeeded = 0;

        ret = CRYPT_AsnDecodeChoiceOfTimeInternal(pbEncoded, cbEncoded,
         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, &bytesNeeded, NULL);
        if (ret)
        {
            if (!pvStructInfo)
                *pcbStructInfo = bytesNeeded;
            else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
             pvStructInfo, pcbStructInfo, bytesNeeded)))
            {
                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                ret = CRYPT_AsnDecodeChoiceOfTimeInternal(pbEncoded, cbEncoded,
                 dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, pvStructInfo,
                 &bytesNeeded, NULL);
                if (!ret && (dwFlags & CRYPT_DECODE_ALLOC_FLAG))
                    CRYPT_FreeSpace(pDecodePara, pvStructInfo);
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

                    if (!pvStructInfo)
                        *pcbStructInfo = bytesNeeded;
                    else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
                     pvStructInfo, pcbStructInfo, bytesNeeded)))
                    {
                        if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                            pvStructInfo = *(BYTE **)pvStructInfo;
                        seq = pvStructInfo;
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
                        if (!ret && (dwFlags & CRYPT_DECODE_ALLOC_FLAG))
                            CRYPT_FreeSpace(pDecodePara, seq);
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

static BOOL CRYPT_AsnDecodeDistPointName(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;

    if (pbEncoded[0] == (ASN_CONTEXT | ASN_CONSTRUCTOR | 0))
    {
        DWORD bytesNeeded = 0, dataLen;

        if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
        {
            struct AsnArrayDescriptor arrayDesc = {
             ASN_CONTEXT | ASN_CONSTRUCTOR | 0,
             offsetof(CRL_DIST_POINT_NAME, u.FullName.cAltEntry),
             offsetof(CRL_DIST_POINT_NAME, u.FullName.rgAltEntry),
             FINALMEMBERSIZE(CRL_DIST_POINT_NAME, u),
             CRYPT_AsnDecodeAltNameEntry, sizeof(CERT_ALT_NAME_ENTRY), TRUE,
             offsetof(CERT_ALT_NAME_ENTRY, u.pwszURL) };
            BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);
            DWORD nameLen;

            if (dataLen)
            {
                ret = CRYPT_AsnDecodeArray(&arrayDesc,
                 pbEncoded + 1 + lenBytes, cbEncoded - 1 - lenBytes,
                 dwFlags, NULL, NULL, &nameLen, NULL);
                bytesNeeded = sizeof(CRL_DIST_POINT_NAME) + nameLen -
                 FINALMEMBERSIZE(CRL_DIST_POINT_NAME, u);
            }
            else
                bytesNeeded = sizeof(CRL_DIST_POINT_NAME);
            if (pcbDecoded)
                *pcbDecoded = 1 + lenBytes + dataLen;
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
                CRL_DIST_POINT_NAME *name = pvStructInfo;

                *pcbStructInfo = bytesNeeded;
                if (dataLen)
                {
                    name->dwDistPointNameChoice = CRL_DIST_POINT_FULL_NAME;
                    ret = CRYPT_AsnDecodeArray(&arrayDesc,
                     pbEncoded + 1 + lenBytes, cbEncoded - 1 - lenBytes,
                     dwFlags, NULL, &name->u.FullName.cAltEntry, &nameLen,
                     NULL);
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

static BOOL CRYPT_AsnDecodeDistPoint(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded)
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
    CRL_DIST_POINT *point = pvStructInfo;
    BOOL ret;

    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, NULL, pvStructInfo, pcbStructInfo,
     pcbDecoded, point ? point->DistPointName.u.FullName.rgAltEntry : NULL);
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
         offsetof(CRL_DIST_POINTS_INFO, cDistPoint),
         offsetof(CRL_DIST_POINTS_INFO, rgDistPoint),
         sizeof(CRL_DIST_POINTS_INFO),
         CRYPT_AsnDecodeDistPoint, sizeof(CRL_DIST_POINT), TRUE,
         offsetof(CRL_DIST_POINT, DistPointName.u.FullName.rgAltEntry) };
        CRL_DIST_POINTS_INFO *info = pvStructInfo;

        if (pvStructInfo && !(dwFlags & CRYPT_DECODE_ALLOC_FLAG))
            info->rgDistPoint = (CRL_DIST_POINT *)(info + 1);
        ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
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
         offsetof(CERT_ENHKEY_USAGE, cUsageIdentifier),
         offsetof(CERT_ENHKEY_USAGE, rgpszUsageIdentifier),
         sizeof(CERT_ENHKEY_USAGE),
         CRYPT_AsnDecodeOidInternal, sizeof(LPSTR), TRUE, 0 };
        CERT_ENHKEY_USAGE *usage = pvStructInfo;

        if (pvStructInfo && !(dwFlags & CRYPT_DECODE_ALLOC_FLAG))
            usage->rgpszUsageIdentifier = (LPSTR *)(usage + 1);
        ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
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

        ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
         pbEncoded, cbEncoded, dwFlags, pDecodePara, pvStructInfo,
         pcbStructInfo, NULL, NULL);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL CRYPT_AsnDecodeMaximum(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;
    DWORD max, size = sizeof(max);

    TRACE("%p, %d, %08x, %p, %d, %p\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo, pcbDecoded);

    if (!cbEncoded)
    {
        SetLastError(CRYPT_E_ASN1_EOD);
        return FALSE;
    }
    if (pbEncoded[0] != (ASN_CONTEXT | 1))
    {
        SetLastError(CRYPT_E_ASN1_BADTAG);
        return FALSE;
    }
    if ((ret = CRYPT_AsnDecodeIntInternal(pbEncoded, cbEncoded, dwFlags,
     &max, &size, pcbDecoded)))
    {
        DWORD bytesNeeded = FINALMEMBERSIZE(CERT_GENERAL_SUBTREE, fMaximum);

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
            CERT_GENERAL_SUBTREE *subtree = CONTAINING_RECORD(pvStructInfo,
             CERT_GENERAL_SUBTREE, fMaximum);

            *pcbStructInfo = bytesNeeded;
            /* The BOOL is implicit:  if the integer is present, then it's
             * TRUE.
             */
            subtree->fMaximum = TRUE;
            subtree->dwMaximum = max;
        }
    }
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL CRYPT_AsnDecodeSubtree(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;
    struct AsnDecodeSequenceItem items[] = {
     { 0, offsetof(CERT_GENERAL_SUBTREE, Base),
       CRYPT_AsnDecodeAltNameEntry, sizeof(CERT_ALT_NAME_ENTRY), TRUE, TRUE,
       offsetof(CERT_ALT_NAME_ENTRY, u.pwszURL), 0 },
     { ASN_CONTEXT | 0, offsetof(CERT_GENERAL_SUBTREE, dwMinimum),
       CRYPT_AsnDecodeIntInternal, sizeof(DWORD), TRUE, FALSE, 0, 0 },
     { ASN_CONTEXT | 1, offsetof(CERT_GENERAL_SUBTREE, fMaximum),
       CRYPT_AsnDecodeMaximum, FINALMEMBERSIZE(CERT_GENERAL_SUBTREE, fMaximum),
       TRUE, FALSE, 0, 0 },
    };
    CERT_GENERAL_SUBTREE *subtree = pvStructInfo;

    TRACE("%p, %d, %08x, %p, %d, %p\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo, pcbDecoded);

    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, NULL, pvStructInfo, pcbStructInfo,
     pcbDecoded, subtree ? subtree->Base.u.pwszURL : NULL);
    if (pcbDecoded)
    {
        TRACE("%d\n", *pcbDecoded);
        if (*pcbDecoded < cbEncoded)
            TRACE("%02x %02x\n", *(pbEncoded + *pcbDecoded),
             *(pbEncoded + *pcbDecoded + 1));
    }
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL CRYPT_AsnDecodePermittedSubtree(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret = TRUE;
    struct AsnArrayDescriptor arrayDesc = { 0,
     offsetof(CERT_NAME_CONSTRAINTS_INFO, cPermittedSubtree),
     offsetof(CERT_NAME_CONSTRAINTS_INFO, rgPermittedSubtree),
     MEMBERSIZE(CERT_NAME_CONSTRAINTS_INFO, cPermittedSubtree,
                cExcludedSubtree),
     CRYPT_AsnDecodeSubtree, sizeof(CERT_GENERAL_SUBTREE), TRUE,
     offsetof(CERT_GENERAL_SUBTREE, Base.u.pwszURL) };

    TRACE("%p, %d, %08x, %p, %d, %p\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo, pcbDecoded);

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
     dwFlags, NULL, pvStructInfo, pcbStructInfo, pcbDecoded);
    return ret;
}

static BOOL CRYPT_AsnDecodeExcludedSubtree(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret = TRUE;
    struct AsnArrayDescriptor arrayDesc = { 0,
     offsetof(CERT_NAME_CONSTRAINTS_INFO, cExcludedSubtree),
     offsetof(CERT_NAME_CONSTRAINTS_INFO, rgExcludedSubtree),
     FINALMEMBERSIZE(CERT_NAME_CONSTRAINTS_INFO, cExcludedSubtree),
     CRYPT_AsnDecodeSubtree, sizeof(CERT_GENERAL_SUBTREE), TRUE,
     offsetof(CERT_GENERAL_SUBTREE, Base.u.pwszURL) };

    TRACE("%p, %d, %08x, %p, %d, %p\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo, pcbDecoded);

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
     dwFlags, NULL, pvStructInfo, pcbStructInfo, pcbDecoded);
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeNameConstraints(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    __TRY
    {
        struct AsnDecodeSequenceItem items[] = {
         { ASN_CONTEXT | ASN_CONSTRUCTOR | 0,
           offsetof(CERT_NAME_CONSTRAINTS_INFO, cPermittedSubtree),
           CRYPT_AsnDecodePermittedSubtree,
           MEMBERSIZE(CERT_NAME_CONSTRAINTS_INFO, cPermittedSubtree,
           cExcludedSubtree), TRUE, TRUE,
           offsetof(CERT_NAME_CONSTRAINTS_INFO, rgPermittedSubtree), 0 },
         { ASN_CONTEXT | ASN_CONSTRUCTOR | 1,
           offsetof(CERT_NAME_CONSTRAINTS_INFO, cExcludedSubtree),
           CRYPT_AsnDecodeExcludedSubtree,
           FINALMEMBERSIZE(CERT_NAME_CONSTRAINTS_INFO, cExcludedSubtree),
           TRUE, TRUE,
           offsetof(CERT_NAME_CONSTRAINTS_INFO, rgExcludedSubtree), 0 },
        };

        ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
         pbEncoded, cbEncoded, dwFlags, pDecodePara, pvStructInfo,
         pcbStructInfo, NULL, NULL);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    return ret;
}

static BOOL CRYPT_AsnDecodeIssuerSerialNumber(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;
    struct AsnDecodeSequenceItem items[] = {
     { 0, offsetof(CERT_ISSUER_SERIAL_NUMBER, Issuer), CRYPT_AsnDecodeDerBlob,
       sizeof(CRYPT_DER_BLOB), FALSE, TRUE, offsetof(CERT_ISSUER_SERIAL_NUMBER,
       Issuer.pbData) },
     { ASN_INTEGER, offsetof(CERT_ISSUER_SERIAL_NUMBER, SerialNumber),
       CRYPT_AsnDecodeIntegerInternal, sizeof(CRYPT_INTEGER_BLOB), FALSE,
       TRUE, offsetof(CERT_ISSUER_SERIAL_NUMBER, SerialNumber.pbData), 0 },
    };
    CERT_ISSUER_SERIAL_NUMBER *issuerSerial = pvStructInfo;

    TRACE("%p, %d, %08x, %p, %d, %p\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo, pcbDecoded);

    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, NULL, pvStructInfo, pcbStructInfo,
     pcbDecoded, issuerSerial ? issuerSerial->Issuer.pbData : NULL);
    if (ret && issuerSerial && !issuerSerial->SerialNumber.cbData)
    {
        SetLastError(CRYPT_E_ASN1_CORRUPT);
        ret = FALSE;
    }
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL CRYPT_AsnDecodePKCSSignerInfoInternal(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    CMSG_SIGNER_INFO *info = pvStructInfo;
    struct AsnDecodeSequenceItem items[] = {
     { ASN_INTEGER, offsetof(CMSG_SIGNER_INFO, dwVersion),
       CRYPT_AsnDecodeIntInternal, sizeof(DWORD), FALSE, FALSE, 0, 0 },
     { ASN_SEQUENCEOF, offsetof(CMSG_SIGNER_INFO, Issuer),
       CRYPT_AsnDecodeIssuerSerialNumber, sizeof(CERT_ISSUER_SERIAL_NUMBER),
       FALSE, TRUE, offsetof(CMSG_SIGNER_INFO, Issuer.pbData), 0 },
     { ASN_SEQUENCEOF, offsetof(CMSG_SIGNER_INFO, HashAlgorithm),
       CRYPT_AsnDecodeAlgorithmId, sizeof(CRYPT_ALGORITHM_IDENTIFIER),
       FALSE, TRUE, offsetof(CMSG_SIGNER_INFO, HashAlgorithm.pszObjId), 0 },
     { ASN_CONSTRUCTOR | ASN_CONTEXT | 0,
       offsetof(CMSG_SIGNER_INFO, AuthAttrs),
       CRYPT_AsnDecodePKCSAttributesInternal, sizeof(CRYPT_ATTRIBUTES),
       TRUE, TRUE, offsetof(CMSG_SIGNER_INFO, AuthAttrs.rgAttr), 0 },
     { ASN_SEQUENCEOF, offsetof(CMSG_SIGNER_INFO, HashEncryptionAlgorithm),
       CRYPT_AsnDecodeAlgorithmId, sizeof(CRYPT_ALGORITHM_IDENTIFIER),
       FALSE, TRUE, offsetof(CMSG_SIGNER_INFO,
       HashEncryptionAlgorithm.pszObjId), 0 },
     { ASN_OCTETSTRING, offsetof(CMSG_SIGNER_INFO, EncryptedHash),
       CRYPT_AsnDecodeOctets, sizeof(CRYPT_DER_BLOB),
       FALSE, TRUE, offsetof(CMSG_SIGNER_INFO, EncryptedHash.pbData), 0 },
     { ASN_CONSTRUCTOR | ASN_CONTEXT | 1,
       offsetof(CMSG_SIGNER_INFO, UnauthAttrs),
       CRYPT_AsnDecodePKCSAttributesInternal, sizeof(CRYPT_ATTRIBUTES),
       TRUE, TRUE, offsetof(CMSG_SIGNER_INFO, UnauthAttrs.rgAttr), 0 },
    };
    BOOL ret;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, NULL, pvStructInfo, pcbStructInfo,
     pcbDecoded, info ? info->Issuer.pbData : NULL);
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodePKCSSignerInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    __TRY
    {
        ret = CRYPT_AsnDecodePKCSSignerInfoInternal(pbEncoded, cbEncoded,
         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, pcbStructInfo, NULL);
        if (ret && pvStructInfo)
        {
            ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara, pvStructInfo,
             pcbStructInfo, *pcbStructInfo);
            if (ret)
            {
                CMSG_SIGNER_INFO *info;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                info = pvStructInfo;
                info->Issuer.pbData = ((BYTE *)info +
                 sizeof(CMSG_SIGNER_INFO));
                ret = CRYPT_AsnDecodePKCSSignerInfoInternal(pbEncoded,
                 cbEncoded, dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, pvStructInfo,
                 pcbStructInfo, NULL);
                if (!ret && (dwFlags & CRYPT_DECODE_ALLOC_FLAG))
                    CRYPT_FreeSpace(pDecodePara, info);
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL verify_and_copy_certificate(const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
                                        void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded)
{
    PCCERT_CONTEXT cert;

    cert = CertCreateCertificateContext(X509_ASN_ENCODING, pbEncoded, cbEncoded);
    if (!cert)
    {
        WARN("CertCreateCertificateContext error %#x\n", GetLastError());
        *pcbStructInfo = 0;
        *pcbDecoded = 0;
        return TRUE;
    }

    CertFreeCertificateContext(cert);

    return CRYPT_AsnDecodeCopyBytes(pbEncoded, cbEncoded, dwFlags, pvStructInfo, pcbStructInfo, pcbDecoded);
}

static BOOL CRYPT_AsnDecodeCMSCertEncoded(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;
    struct AsnArrayDescriptor arrayDesc = { 0,
     offsetof(CRYPT_SIGNED_INFO, cCertEncoded),
     offsetof(CRYPT_SIGNED_INFO, rgCertEncoded),
     MEMBERSIZE(CRYPT_SIGNED_INFO, cCertEncoded, cCrlEncoded),
     verify_and_copy_certificate,
     sizeof(CRYPT_DER_BLOB), TRUE, offsetof(CRYPT_DER_BLOB, pbData) };

    TRACE("%p, %d, %08x, %p, %d, %p\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, pvStructInfo ? *pcbStructInfo : 0, pcbDecoded);

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
     dwFlags, NULL, pvStructInfo, pcbStructInfo, pcbDecoded);
    return ret;
}

static BOOL CRYPT_AsnDecodeCMSCrlEncoded(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;
    struct AsnArrayDescriptor arrayDesc = { 0,
     offsetof(CRYPT_SIGNED_INFO, cCrlEncoded),
     offsetof(CRYPT_SIGNED_INFO, rgCrlEncoded),
     MEMBERSIZE(CRYPT_SIGNED_INFO, cCrlEncoded, content),
     CRYPT_AsnDecodeCopyBytes, sizeof(CRYPT_DER_BLOB),
     TRUE, offsetof(CRYPT_DER_BLOB, pbData) };

    TRACE("%p, %d, %08x, %p, %d, %p\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, pvStructInfo ? *pcbStructInfo : 0, pcbDecoded);

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
     dwFlags, NULL, pvStructInfo, pcbStructInfo, pcbDecoded);
    return ret;
}

static BOOL CRYPT_AsnDecodeCMSSignerId(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    CERT_ID *id = pvStructInfo;
    BOOL ret = FALSE;

    if (*pbEncoded == ASN_SEQUENCEOF)
    {
        ret = CRYPT_AsnDecodeIssuerSerialNumber(pbEncoded, cbEncoded, dwFlags,
         id ? &id->u.IssuerSerialNumber : NULL, pcbStructInfo, pcbDecoded);
        if (ret)
        {
            if (id)
                id->dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;
            if (*pcbStructInfo > sizeof(CERT_ISSUER_SERIAL_NUMBER))
                *pcbStructInfo = sizeof(CERT_ID) + *pcbStructInfo -
                 sizeof(CERT_ISSUER_SERIAL_NUMBER);
            else
                *pcbStructInfo = sizeof(CERT_ID);
        }
    }
    else if (*pbEncoded == (ASN_CONTEXT | 0))
    {
        ret = CRYPT_AsnDecodeOctets(pbEncoded, cbEncoded, dwFlags,
         id ? &id->u.KeyId : NULL, pcbStructInfo, pcbDecoded);
        if (ret)
        {
            if (id)
                id->dwIdChoice = CERT_ID_KEY_IDENTIFIER;
            if (*pcbStructInfo > sizeof(CRYPT_DATA_BLOB))
                *pcbStructInfo = sizeof(CERT_ID) + *pcbStructInfo -
                 sizeof(CRYPT_DATA_BLOB);
            else
                *pcbStructInfo = sizeof(CERT_ID);
        }
    }
    else
        SetLastError(CRYPT_E_ASN1_BADTAG);
    return ret;
}

static BOOL CRYPT_AsnDecodeCMSSignerInfoInternal(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    CMSG_CMS_SIGNER_INFO *info = pvStructInfo;
    struct AsnDecodeSequenceItem items[] = {
     { ASN_INTEGER, offsetof(CMSG_CMS_SIGNER_INFO, dwVersion),
       CRYPT_AsnDecodeIntInternal, sizeof(DWORD), FALSE, FALSE, 0, 0 },
     { 0, offsetof(CMSG_CMS_SIGNER_INFO, SignerId),
       CRYPT_AsnDecodeCMSSignerId, sizeof(CERT_ID), FALSE, TRUE,
       offsetof(CMSG_CMS_SIGNER_INFO, SignerId.u.KeyId.pbData), 0 },
     { ASN_SEQUENCEOF, offsetof(CMSG_CMS_SIGNER_INFO, HashAlgorithm),
       CRYPT_AsnDecodeAlgorithmId, sizeof(CRYPT_ALGORITHM_IDENTIFIER),
       FALSE, TRUE, offsetof(CMSG_CMS_SIGNER_INFO, HashAlgorithm.pszObjId), 0 },
     { ASN_CONSTRUCTOR | ASN_CONTEXT | 0,
       offsetof(CMSG_CMS_SIGNER_INFO, AuthAttrs),
       CRYPT_AsnDecodePKCSAttributesInternal, sizeof(CRYPT_ATTRIBUTES),
       TRUE, TRUE, offsetof(CMSG_CMS_SIGNER_INFO, AuthAttrs.rgAttr), 0 },
     /* FIXME: Tests show that CertOpenStore accepts such certificates, but
      * how exactly should they be interpreted? */
     { ASN_CONSTRUCTOR | ASN_UNIVERSAL | 0x11, 0, NULL, 0, TRUE, FALSE, 0, 0 },
     { ASN_SEQUENCEOF, offsetof(CMSG_CMS_SIGNER_INFO, HashEncryptionAlgorithm),
       CRYPT_AsnDecodeAlgorithmId, sizeof(CRYPT_ALGORITHM_IDENTIFIER),
       FALSE, TRUE, offsetof(CMSG_CMS_SIGNER_INFO,
       HashEncryptionAlgorithm.pszObjId), 0 },
     { ASN_OCTETSTRING, offsetof(CMSG_CMS_SIGNER_INFO, EncryptedHash),
       CRYPT_AsnDecodeOctets, sizeof(CRYPT_DER_BLOB),
       FALSE, TRUE, offsetof(CMSG_CMS_SIGNER_INFO, EncryptedHash.pbData), 0 },
     { ASN_CONSTRUCTOR | ASN_CONTEXT | 1,
       offsetof(CMSG_CMS_SIGNER_INFO, UnauthAttrs),
       CRYPT_AsnDecodePKCSAttributesInternal, sizeof(CRYPT_ATTRIBUTES),
       TRUE, TRUE, offsetof(CMSG_CMS_SIGNER_INFO, UnauthAttrs.rgAttr), 0 },
    };
    BOOL ret;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, NULL, pvStructInfo, pcbStructInfo,
     pcbDecoded, info ? info->SignerId.u.KeyId.pbData : NULL);
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeCMSSignerInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;

    TRACE("%p, %d, %08x, %p, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, *pcbStructInfo);

    __TRY
    {
        ret = CRYPT_AsnDecodeCMSSignerInfoInternal(pbEncoded, cbEncoded,
         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, NULL, pcbStructInfo, NULL);
        if (ret && pvStructInfo)
        {
            ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara, pvStructInfo,
             pcbStructInfo, *pcbStructInfo);
            if (ret)
            {
                CMSG_CMS_SIGNER_INFO *info;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;
                info = pvStructInfo;
                info->SignerId.u.KeyId.pbData = ((BYTE *)info +
                 sizeof(CMSG_CMS_SIGNER_INFO));
                ret = CRYPT_AsnDecodeCMSSignerInfoInternal(pbEncoded,
                 cbEncoded, dwFlags & ~CRYPT_DECODE_ALLOC_FLAG, pvStructInfo,
                 pcbStructInfo, NULL);
                if (!ret && (dwFlags & CRYPT_DECODE_ALLOC_FLAG))
                    CRYPT_FreeSpace(pDecodePara, info);
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL CRYPT_DecodeSignerArray(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded)
{
    BOOL ret;
    struct AsnArrayDescriptor arrayDesc = { ASN_CONSTRUCTOR | ASN_SETOF,
     offsetof(CRYPT_SIGNED_INFO, cSignerInfo),
     offsetof(CRYPT_SIGNED_INFO, rgSignerInfo),
     FINALMEMBERSIZE(CRYPT_SIGNED_INFO, cSignerInfo),
     CRYPT_AsnDecodeCMSSignerInfoInternal, sizeof(CMSG_CMS_SIGNER_INFO), TRUE,
     offsetof(CMSG_CMS_SIGNER_INFO, SignerId.u.KeyId.pbData) };

    TRACE("%p, %d, %08x, %p, %d, %p\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo, pcbDecoded);

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
     dwFlags, NULL, pvStructInfo, pcbStructInfo, pcbDecoded);
    return ret;
}

BOOL CRYPT_AsnDecodeCMSSignedInfo(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, PCRYPT_DECODE_PARA pDecodePara,
 CRYPT_SIGNED_INFO *signedInfo, DWORD *pcbSignedInfo)
{
    BOOL ret = FALSE;
    struct AsnDecodeSequenceItem items[] = {
     { ASN_INTEGER, offsetof(CRYPT_SIGNED_INFO, version),
       CRYPT_AsnDecodeIntInternal, sizeof(DWORD), FALSE, FALSE, 0, 0 },
     /* Placeholder for the hash algorithms - redundant with those in the
      * signers, so just ignore them.
      */
     { ASN_CONSTRUCTOR | ASN_SETOF, 0, NULL, 0, TRUE, FALSE, 0, 0 },
     { ASN_SEQUENCE, offsetof(CRYPT_SIGNED_INFO, content),
       CRYPT_AsnDecodePKCSContentInfoInternal, sizeof(CRYPT_CONTENT_INFO),
       FALSE, TRUE, offsetof(CRYPT_SIGNED_INFO, content.pszObjId), 0 },
     { ASN_CONSTRUCTOR | ASN_CONTEXT | 0,
       offsetof(CRYPT_SIGNED_INFO, cCertEncoded), CRYPT_AsnDecodeCMSCertEncoded,
       MEMBERSIZE(CRYPT_SIGNED_INFO, cCertEncoded, cCrlEncoded), TRUE, TRUE,
       offsetof(CRYPT_SIGNED_INFO, rgCertEncoded), 0 },
     { ASN_CONSTRUCTOR | ASN_CONTEXT | 1,
       offsetof(CRYPT_SIGNED_INFO, cCrlEncoded), CRYPT_AsnDecodeCMSCrlEncoded,
       MEMBERSIZE(CRYPT_SIGNED_INFO, cCrlEncoded, content), TRUE, TRUE,
       offsetof(CRYPT_SIGNED_INFO, rgCrlEncoded), 0 },
     { ASN_CONSTRUCTOR | ASN_SETOF, offsetof(CRYPT_SIGNED_INFO, cSignerInfo),
       CRYPT_DecodeSignerArray,
       FINALMEMBERSIZE(CRYPT_SIGNED_INFO, cSignerInfo), TRUE, TRUE,
       offsetof(CRYPT_SIGNED_INFO, rgSignerInfo), 0 },
    };

    TRACE("%p, %d, %08x, %p, %p, %p\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, signedInfo, pcbSignedInfo);

    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, pDecodePara, signedInfo, pcbSignedInfo,
     NULL, NULL);
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL CRYPT_AsnDecodeRecipientInfo(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo, DWORD *pcbDecoded)
{
    BOOL ret;
    CMSG_KEY_TRANS_RECIPIENT_INFO *info = pvStructInfo;
    struct AsnDecodeSequenceItem items[] = {
     { ASN_INTEGER, offsetof(CMSG_KEY_TRANS_RECIPIENT_INFO, dwVersion),
       CRYPT_AsnDecodeIntInternal, sizeof(DWORD), FALSE, FALSE, 0, 0 },
     { ASN_SEQUENCEOF, offsetof(CMSG_KEY_TRANS_RECIPIENT_INFO,
       RecipientId.u.IssuerSerialNumber), CRYPT_AsnDecodeIssuerSerialNumber,
       sizeof(CERT_ISSUER_SERIAL_NUMBER), FALSE, TRUE,
       offsetof(CMSG_KEY_TRANS_RECIPIENT_INFO,
       RecipientId.u.IssuerSerialNumber.Issuer.pbData), 0 },
     { ASN_SEQUENCEOF, offsetof(CMSG_KEY_TRANS_RECIPIENT_INFO,
       KeyEncryptionAlgorithm), CRYPT_AsnDecodeAlgorithmId,
       sizeof(CRYPT_ALGORITHM_IDENTIFIER), FALSE, TRUE,
       offsetof(CMSG_KEY_TRANS_RECIPIENT_INFO,
       KeyEncryptionAlgorithm.pszObjId), 0 },
     { ASN_OCTETSTRING, offsetof(CMSG_KEY_TRANS_RECIPIENT_INFO, EncryptedKey),
       CRYPT_AsnDecodeOctets, sizeof(CRYPT_DATA_BLOB), FALSE, TRUE,
       offsetof(CMSG_KEY_TRANS_RECIPIENT_INFO, EncryptedKey.pbData), 0 },
    };

    TRACE("%p, %d, %08x, %p, %d, %p\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo, pcbDecoded);

    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, NULL, pvStructInfo, pcbStructInfo,
     pcbDecoded, info ? info->RecipientId.u.IssuerSerialNumber.Issuer.pbData :
     NULL);
    if (info)
        info->RecipientId.dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL CRYPT_DecodeRecipientInfoArray(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;
    struct AsnArrayDescriptor arrayDesc = { ASN_CONSTRUCTOR | ASN_SETOF,
     offsetof(CRYPT_ENVELOPED_DATA, cRecipientInfo),
     offsetof(CRYPT_ENVELOPED_DATA, rgRecipientInfo),
     MEMBERSIZE(CRYPT_ENVELOPED_DATA, cRecipientInfo, encryptedContentInfo),
     CRYPT_AsnDecodeRecipientInfo, sizeof(CMSG_KEY_TRANS_RECIPIENT_INFO), TRUE,
     offsetof(CMSG_KEY_TRANS_RECIPIENT_INFO,
     RecipientId.u.IssuerSerialNumber.Issuer.pbData) };

    TRACE("%p, %d, %08x, %p, %d, %p\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo, pcbDecoded);

    ret = CRYPT_AsnDecodeArray(&arrayDesc, pbEncoded, cbEncoded,
     dwFlags, NULL, pvStructInfo, pcbStructInfo, pcbDecoded);
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL CRYPT_AsnDecodeEncryptedContentInfo(const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 DWORD *pcbDecoded)
{
    BOOL ret;
    CRYPT_ENCRYPTED_CONTENT_INFO *info = pvStructInfo;
    struct AsnDecodeSequenceItem items[] = {
     { ASN_OBJECTIDENTIFIER, offsetof(CRYPT_ENCRYPTED_CONTENT_INFO,
       contentType), CRYPT_AsnDecodeOidInternal, sizeof(LPSTR),
       FALSE, TRUE, offsetof(CRYPT_ENCRYPTED_CONTENT_INFO,
       contentType), 0 },
     { ASN_SEQUENCEOF, offsetof(CRYPT_ENCRYPTED_CONTENT_INFO,
       contentEncryptionAlgorithm), CRYPT_AsnDecodeAlgorithmId,
       sizeof(CRYPT_ALGORITHM_IDENTIFIER), FALSE, TRUE,
       offsetof(CRYPT_ENCRYPTED_CONTENT_INFO,
       contentEncryptionAlgorithm.pszObjId), 0 },
     { ASN_CONTEXT | 0, offsetof(CRYPT_ENCRYPTED_CONTENT_INFO,
       encryptedContent), CRYPT_AsnDecodeOctets,
       sizeof(CRYPT_DATA_BLOB), TRUE, TRUE,
       offsetof(CRYPT_ENCRYPTED_CONTENT_INFO, encryptedContent.pbData) },
    };

    TRACE("%p, %d, %08x, %p, %d, %p\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo, pcbDecoded);

    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, NULL, pvStructInfo, pcbStructInfo,
     pcbDecoded, info ? info->contentType : NULL);
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL CRYPT_AsnDecodePKCSEnvelopedData(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, PCRYPT_DECODE_PARA pDecodePara,
 CRYPT_ENVELOPED_DATA *envelopedData, DWORD *pcbEnvelopedData)
{
    BOOL ret;
    struct AsnDecodeSequenceItem items[] = {
     { ASN_INTEGER, offsetof(CRYPT_ENVELOPED_DATA, version),
       CRYPT_AsnDecodeIntInternal, sizeof(DWORD), FALSE, FALSE, 0, 0 },
     { ASN_CONSTRUCTOR | ASN_SETOF, offsetof(CRYPT_ENVELOPED_DATA,
       cRecipientInfo), CRYPT_DecodeRecipientInfoArray,
       MEMBERSIZE(CRYPT_ENVELOPED_DATA, cRecipientInfo, encryptedContentInfo),
       FALSE, TRUE, offsetof(CRYPT_ENVELOPED_DATA, rgRecipientInfo), 0 },
     { ASN_SEQUENCEOF, offsetof(CRYPT_ENVELOPED_DATA, encryptedContentInfo),
       CRYPT_AsnDecodeEncryptedContentInfo,
       sizeof(CRYPT_ENCRYPTED_CONTENT_INFO), FALSE, TRUE,
       offsetof(CRYPT_ENVELOPED_DATA, encryptedContentInfo.contentType), 0 },
    };

    TRACE("%p, %d, %08x, %p, %p, %p\n", pbEncoded, cbEncoded, dwFlags,
     pDecodePara, envelopedData, pcbEnvelopedData);

    ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
     pbEncoded, cbEncoded, dwFlags, pDecodePara, envelopedData,
     pcbEnvelopedData, NULL, NULL);
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeObjectIdentifier(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 CRYPT_DECODE_PARA *pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    DWORD bytesNeeded = 0;
    BOOL ret;

    __TRY
    {
        ret = CRYPT_AsnDecodeOidInternal(pbEncoded, cbEncoded, dwFlags & ~CRYPT_DECODE_ALLOC_FLAG,
                                         NULL, &bytesNeeded, NULL);
        if (ret)
        {
            if (!pvStructInfo)
                *pcbStructInfo = bytesNeeded;
            else if ((ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara, pvStructInfo, pcbStructInfo, bytesNeeded)))
            {
                LPSTR *info;

                if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
                    pvStructInfo = *(BYTE **)pvStructInfo;

                info = pvStructInfo;
                *info = (void *)((BYTE *)info + sizeof(*info));
                ret = CRYPT_AsnDecodeOidInternal(pbEncoded, cbEncoded, dwFlags & ~CRYPT_DECODE_ALLOC_FLAG,
                                                 pvStructInfo, &bytesNeeded, NULL);
                if (!ret && (dwFlags & CRYPT_DECODE_ALLOC_FLAG))
                    CRYPT_FreeSpace(pDecodePara, info);
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

static BOOL WINAPI CRYPT_AsnDecodeEccSignature(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 CRYPT_DECODE_PARA *pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;
    struct AsnDecodeSequenceItem items[] = {
     { ASN_INTEGER, offsetof(CERT_ECC_SIGNATURE, r),
       CRYPT_AsnDecodeUnsignedIntegerInternal, sizeof(CRYPT_UINT_BLOB), FALSE,
       TRUE, offsetof(CERT_ECC_SIGNATURE, r.pbData), 0 },
     { ASN_INTEGER, offsetof(CERT_ECC_SIGNATURE, s),
       CRYPT_AsnDecodeUnsignedIntegerInternal, sizeof(CRYPT_UINT_BLOB), FALSE,
       TRUE, offsetof(CERT_ECC_SIGNATURE, s.pbData), 0 },
    };

    __TRY
    {
        ret = CRYPT_AsnDecodeSequence(items, ARRAY_SIZE(items),
         pbEncoded, cbEncoded, dwFlags, pDecodePara, pvStructInfo,
         pcbStructInfo, NULL, NULL);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static CryptDecodeObjectExFunc CRYPT_GetBuiltinDecoder(DWORD dwCertEncodingType,
 LPCSTR lpszStructType)
{
    CryptDecodeObjectExFunc decodeFunc = NULL;

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
            decodeFunc = CRYPT_AsnDecodeCertSignedContent;
            break;
        case LOWORD(X509_CERT_TO_BE_SIGNED):
            decodeFunc = CRYPT_AsnDecodeCert;
            break;
        case LOWORD(X509_CERT_CRL_TO_BE_SIGNED):
            decodeFunc = CRYPT_AsnDecodeCRL;
            break;
        case LOWORD(X509_EXTENSIONS):
            decodeFunc = CRYPT_AsnDecodeExtensions;
            break;
        case LOWORD(X509_NAME_VALUE):
            decodeFunc = CRYPT_AsnDecodeNameValue;
            break;
        case LOWORD(X509_NAME):
            decodeFunc = CRYPT_AsnDecodeName;
            break;
        case LOWORD(X509_PUBLIC_KEY_INFO):
            decodeFunc = CRYPT_AsnDecodePubKeyInfo;
            break;
        case LOWORD(X509_AUTHORITY_KEY_ID):
            decodeFunc = CRYPT_AsnDecodeAuthorityKeyId;
            break;
        case LOWORD(X509_ALTERNATE_NAME):
            decodeFunc = CRYPT_AsnDecodeAltName;
            break;
        case LOWORD(X509_BASIC_CONSTRAINTS):
            decodeFunc = CRYPT_AsnDecodeBasicConstraints;
            break;
        case LOWORD(X509_BASIC_CONSTRAINTS2):
            decodeFunc = CRYPT_AsnDecodeBasicConstraints2;
            break;
        case LOWORD(X509_CERT_POLICIES):
            decodeFunc = CRYPT_AsnDecodeCertPolicies;
            break;
        case LOWORD(RSA_CSP_PUBLICKEYBLOB):
            decodeFunc = CRYPT_AsnDecodeRsaPubKey;
            break;
        case LOWORD(PKCS_RSA_PRIVATE_KEY):
            decodeFunc = CRYPT_AsnDecodeRsaPrivKey;
            break;
        case LOWORD(X509_UNICODE_NAME):
            decodeFunc = CRYPT_AsnDecodeUnicodeName;
            break;
        case LOWORD(PKCS_ATTRIBUTE):
            decodeFunc = CRYPT_AsnDecodePKCSAttribute;
            break;
        case LOWORD(X509_UNICODE_NAME_VALUE):
            decodeFunc = CRYPT_AsnDecodeUnicodeNameValue;
            break;
        case LOWORD(X509_OCTET_STRING):
            decodeFunc = CRYPT_AsnDecodeOctetString;
            break;
        case LOWORD(X509_BITS):
        case LOWORD(X509_KEY_USAGE):
            decodeFunc = CRYPT_AsnDecodeBits;
            break;
        case LOWORD(X509_INTEGER):
            decodeFunc = CRYPT_AsnDecodeInt;
            break;
        case LOWORD(X509_MULTI_BYTE_INTEGER):
            decodeFunc = CRYPT_AsnDecodeInteger;
            break;
        case LOWORD(X509_MULTI_BYTE_UINT):
            decodeFunc = CRYPT_AsnDecodeUnsignedInteger;
            break;
        case LOWORD(X509_ENUMERATED):
            decodeFunc = CRYPT_AsnDecodeEnumerated;
            break;
        case LOWORD(X509_CHOICE_OF_TIME):
            decodeFunc = CRYPT_AsnDecodeChoiceOfTime;
            break;
        case LOWORD(X509_AUTHORITY_KEY_ID2):
            decodeFunc = CRYPT_AsnDecodeAuthorityKeyId2;
            break;
        case LOWORD(X509_AUTHORITY_INFO_ACCESS):
            decodeFunc = CRYPT_AsnDecodeAuthorityInfoAccess;
            break;
        case LOWORD(PKCS_CONTENT_INFO):
            decodeFunc = CRYPT_AsnDecodePKCSContentInfo;
            break;
        case LOWORD(X509_SEQUENCE_OF_ANY):
            decodeFunc = CRYPT_AsnDecodeSequenceOfAny;
            break;
        case LOWORD(PKCS_UTC_TIME):
            decodeFunc = CRYPT_AsnDecodeUtcTime;
            break;
        case LOWORD(X509_CRL_DIST_POINTS):
            decodeFunc = CRYPT_AsnDecodeCRLDistPoints;
            break;
        case LOWORD(X509_ENHANCED_KEY_USAGE):
            decodeFunc = CRYPT_AsnDecodeEnhancedKeyUsage;
            break;
        case LOWORD(PKCS_CTL):
            decodeFunc = CRYPT_AsnDecodeCTL;
            break;
        case LOWORD(PKCS_SMIME_CAPABILITIES):
            decodeFunc = CRYPT_AsnDecodeSMIMECapabilities;
            break;
        case LOWORD(X509_PKIX_POLICY_QUALIFIER_USERNOTICE):
            decodeFunc = CRYPT_AsnDecodePolicyQualifierUserNotice;
            break;
        case LOWORD(PKCS_ATTRIBUTES):
            decodeFunc = CRYPT_AsnDecodePKCSAttributes;
            break;
        case LOWORD(X509_ISSUING_DIST_POINT):
            decodeFunc = CRYPT_AsnDecodeIssuingDistPoint;
            break;
        case LOWORD(X509_NAME_CONSTRAINTS):
            decodeFunc = CRYPT_AsnDecodeNameConstraints;
            break;
        case LOWORD(X509_POLICY_MAPPINGS):
            decodeFunc = CRYPT_AsnDecodeCertPolicyMappings;
            break;
        case LOWORD(X509_POLICY_CONSTRAINTS):
            decodeFunc = CRYPT_AsnDecodeCertPolicyConstraints;
            break;
        case LOWORD(PKCS7_SIGNER_INFO):
            decodeFunc = CRYPT_AsnDecodePKCSSignerInfo;
            break;
        case LOWORD(CMS_SIGNER_INFO):
            decodeFunc = CRYPT_AsnDecodeCMSSignerInfo;
            break;
        case LOWORD(X509_OBJECT_IDENTIFIER):
            decodeFunc = CRYPT_AsnDecodeObjectIdentifier;
            break;
        case LOWORD(X509_ECC_SIGNATURE):
            decodeFunc = CRYPT_AsnDecodeEccSignature;
            break;
        }
    }
    else if (!strcmp(lpszStructType, szOID_CERT_EXTENSIONS))
        decodeFunc = CRYPT_AsnDecodeExtensions;
    else if (!strcmp(lpszStructType, szOID_RSA_signingTime))
        decodeFunc = CRYPT_AsnDecodeUtcTime;
    else if (!strcmp(lpszStructType, szOID_RSA_SMIMECapabilities))
        decodeFunc = CRYPT_AsnDecodeSMIMECapabilities;
    else if (!strcmp(lpszStructType, szOID_AUTHORITY_KEY_IDENTIFIER))
        decodeFunc = CRYPT_AsnDecodeAuthorityKeyId;
    else if (!strcmp(lpszStructType, szOID_LEGACY_POLICY_MAPPINGS))
        decodeFunc = CRYPT_AsnDecodeCertPolicyMappings;
    else if (!strcmp(lpszStructType, szOID_AUTHORITY_KEY_IDENTIFIER2))
        decodeFunc = CRYPT_AsnDecodeAuthorityKeyId2;
    else if (!strcmp(lpszStructType, szOID_CRL_REASON_CODE))
        decodeFunc = CRYPT_AsnDecodeEnumerated;
    else if (!strcmp(lpszStructType, szOID_KEY_USAGE))
        decodeFunc = CRYPT_AsnDecodeBits;
    else if (!strcmp(lpszStructType, szOID_SUBJECT_KEY_IDENTIFIER))
        decodeFunc = CRYPT_AsnDecodeOctetString;
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
    else if (!strcmp(lpszStructType, szOID_CERT_POLICIES))
        decodeFunc = CRYPT_AsnDecodeCertPolicies;
    else if (!strcmp(lpszStructType, szOID_POLICY_MAPPINGS))
        decodeFunc = CRYPT_AsnDecodeCertPolicyMappings;
    else if (!strcmp(lpszStructType, szOID_POLICY_CONSTRAINTS))
        decodeFunc = CRYPT_AsnDecodeCertPolicyConstraints;
    else if (!strcmp(lpszStructType, szOID_ENHANCED_KEY_USAGE))
        decodeFunc = CRYPT_AsnDecodeEnhancedKeyUsage;
    else if (!strcmp(lpszStructType, szOID_ISSUING_DIST_POINT))
        decodeFunc = CRYPT_AsnDecodeIssuingDistPoint;
    else if (!strcmp(lpszStructType, szOID_NAME_CONSTRAINTS))
        decodeFunc = CRYPT_AsnDecodeNameConstraints;
    else if (!strcmp(lpszStructType, szOID_AUTHORITY_INFO_ACCESS))
        decodeFunc = CRYPT_AsnDecodeAuthorityInfoAccess;
    else if (!strcmp(lpszStructType, szOID_PKIX_POLICY_QUALIFIER_USERNOTICE))
        decodeFunc = CRYPT_AsnDecodePolicyQualifierUserNotice;
    else if (!strcmp(lpszStructType, szOID_CTL))
        decodeFunc = CRYPT_AsnDecodeCTL;
    else if (!strcmp(lpszStructType, szOID_ECC_PUBLIC_KEY))
        decodeFunc = CRYPT_AsnDecodeObjectIdentifier;
    return decodeFunc;
}

static CryptDecodeObjectFunc CRYPT_LoadDecoderFunc(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, HCRYPTOIDFUNCADDR *hFunc)
{
    static HCRYPTOIDFUNCSET set = NULL;
    CryptDecodeObjectFunc decodeFunc = NULL;

    if (!set)
        set = CryptInitOIDFunctionSet(CRYPT_OID_DECODE_OBJECT_FUNC, 0);
    CryptGetOIDFunctionAddress(set, dwCertEncodingType, lpszStructType, 0,
     (void **)&decodeFunc, hFunc);
    return decodeFunc;
}

static CryptDecodeObjectExFunc CRYPT_LoadDecoderExFunc(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, HCRYPTOIDFUNCADDR *hFunc)
{
    static HCRYPTOIDFUNCSET set = NULL;
    CryptDecodeObjectExFunc decodeFunc = NULL;

    if (!set)
        set = CryptInitOIDFunctionSet(CRYPT_OID_DECODE_OBJECT_EX_FUNC, 0);
    CryptGetOIDFunctionAddress(set, dwCertEncodingType, lpszStructType, 0,
     (void **)&decodeFunc, hFunc);
    return decodeFunc;
}

BOOL WINAPI CryptDecodeObject(DWORD dwCertEncodingType, LPCSTR lpszStructType,
 const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo,
 DWORD *pcbStructInfo)
{
    return CryptDecodeObjectEx(dwCertEncodingType, lpszStructType,
        pbEncoded, cbEncoded, dwFlags, NULL, pvStructInfo, pcbStructInfo);
}

BOOL WINAPI CryptDecodeObjectEx(DWORD dwCertEncodingType, LPCSTR lpszStructType,
 const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;
    CryptDecodeObjectExFunc decodeFunc;
    HCRYPTOIDFUNCADDR hFunc = NULL;

    TRACE_(crypt)("(0x%08x, %s, %p, %d, 0x%08x, %p, %p, %p)\n",
     dwCertEncodingType, debugstr_a(lpszStructType), pbEncoded,
     cbEncoded, dwFlags, pDecodePara, pvStructInfo, pcbStructInfo);

    if (!pvStructInfo && !pcbStructInfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (cbEncoded > MAX_ENCODED_LEN)
    {
        SetLastError(CRYPT_E_ASN1_LARGE);
        return FALSE;
    }

    SetLastError(NOERROR);
    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
    {
        if (!pvStructInfo)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        *(BYTE **)pvStructInfo = NULL;
    }
    decodeFunc = CRYPT_GetBuiltinDecoder(dwCertEncodingType, lpszStructType);
    if (!decodeFunc)
    {
        TRACE_(crypt)("OID %s not found or unimplemented, looking for DLL\n",
         debugstr_a(lpszStructType));
        decodeFunc = CRYPT_LoadDecoderExFunc(dwCertEncodingType, lpszStructType,
         &hFunc);
    }
    if (decodeFunc)
        ret = decodeFunc(dwCertEncodingType, lpszStructType, pbEncoded,
         cbEncoded, dwFlags, pDecodePara, pvStructInfo, pcbStructInfo);
    else
    {
        CryptDecodeObjectFunc pCryptDecodeObject =
         CRYPT_LoadDecoderFunc(dwCertEncodingType, lpszStructType, &hFunc);

        /* Try CryptDecodeObject function.  Don't call CryptDecodeObject
         * directly, as that could cause an infinite loop.
         */
        if (pCryptDecodeObject)
        {
            if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
            {
                ret = pCryptDecodeObject(dwCertEncodingType, lpszStructType,
                 pbEncoded, cbEncoded, dwFlags, NULL, pcbStructInfo);
                if (ret && (ret = CRYPT_DecodeEnsureSpace(dwFlags, pDecodePara,
                 pvStructInfo, pcbStructInfo, *pcbStructInfo)))
                {
                    ret = pCryptDecodeObject(dwCertEncodingType,
                     lpszStructType, pbEncoded, cbEncoded, dwFlags,
                     *(BYTE **)pvStructInfo, pcbStructInfo);
                    if (!ret)
                        CRYPT_FreeSpace(pDecodePara, *(BYTE **)pvStructInfo);
                }
            }
            else
                ret = pCryptDecodeObject(dwCertEncodingType, lpszStructType,
                 pbEncoded, cbEncoded, dwFlags, pvStructInfo, pcbStructInfo);
        }
    }
    if (hFunc)
        CryptFreeOIDFunctionAddress(hFunc, 0);
    TRACE_(crypt)("returning %d\n", ret);
    return ret;
}

BOOL WINAPI PFXIsPFXBlob(CRYPT_DATA_BLOB *pPFX)
{
    BOOL ret;

    TRACE_(crypt)("(%p)\n", pPFX);

    /* A PFX blob is an asn.1-encoded sequence, consisting of at least a
     * version integer of length 1 (3 encoded byes) and at least one other
     * datum (two encoded bytes), plus at least two bytes for the outer
     * sequence.  Thus, even an empty PFX blob is at least 7 bytes in length.
     */
    if (pPFX->cbData < 7)
        ret = FALSE;
    else if (pPFX->pbData[0] == ASN_SEQUENCE)
    {
        DWORD len;

        if ((ret = CRYPT_GetLengthIndefinite(pPFX->pbData, pPFX->cbData, &len)))
        {
            BYTE lenLen = GET_LEN_BYTES(pPFX->pbData[1]);

            /* Need at least three bytes for the integer version */
            if (pPFX->cbData < 1 + lenLen + 3)
                ret = FALSE;
            else if (pPFX->pbData[1 + lenLen] != ASN_INTEGER || /* Tag */
             pPFX->pbData[1 + lenLen + 1] != 1 ||          /* Definite length */
             pPFX->pbData[1 + lenLen + 2] != 3)            /* PFX version */
                ret = FALSE;
        }
    }
    else
        ret = FALSE;
    return ret;
}
