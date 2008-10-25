/* wintrust asn functions
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wincrypt.h"
#include "wintrust.h"
#include "snmp.h"
#include "winternl.h"
#include "wine/debug.h"
#include "wine/exception.h"

WINE_DEFAULT_DEBUG_CHANNEL(cryptasn);

#ifdef WORDS_BIGENDIAN

#define hton16(x) (x)
#define n16toh(x) (x)

#else

#define hton16(x) RtlUshortByteSwap(x)
#define n16toh(x) RtlUshortByteSwap(x)

#endif

#define ASN_BITSTRING       (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x03)

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

static BOOL WINAPI CRYPT_AsnEncodeOctets(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    BOOL ret = TRUE;
    const CRYPT_DATA_BLOB *blob = (const CRYPT_DATA_BLOB *)pvStructInfo;
    DWORD bytesNeeded, lenBytes;

    TRACE("(%d, %p), %p, %d\n", blob->cbData, blob->pbData, pbEncoded,
     *pcbEncoded);

    CRYPT_EncodeLen(blob->cbData, NULL, &lenBytes);
    bytesNeeded = 1 + lenBytes + blob->cbData;
    if (!pbEncoded)
        *pcbEncoded = bytesNeeded;
    else if (*pcbEncoded < bytesNeeded)
    {
        *pcbEncoded = bytesNeeded;
        SetLastError(ERROR_MORE_DATA);
        ret = FALSE;
    }
    else
    {
        *pbEncoded++ = ASN_OCTETSTRING;
        CRYPT_EncodeLen(blob->cbData, pbEncoded, &lenBytes);
        pbEncoded += lenBytes;
        if (blob->cbData)
            memcpy(pbEncoded, blob->pbData, blob->cbData);
    }
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI WVTAsn1SpcLinkEncode(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    BOOL ret = FALSE;

    TRACE("(0x%08x, %s, %p, %p, %p)\n", dwCertEncodingType,
     debugstr_a(lpszStructType), pvStructInfo, pbEncoded,
     pcbEncoded);

    __TRY
    {
        const SPC_LINK *link = (const SPC_LINK *)pvStructInfo;
        DWORD bytesNeeded, lenBytes;

        switch (link->dwLinkChoice)
        {
        case SPC_FILE_LINK_CHOICE:
        {
            DWORD fileNameLen, fileNameLenBytes;
            LPWSTR ptr;

            fileNameLen = link->u.pwszFile ?
             lstrlenW(link->u.pwszFile) * sizeof(WCHAR) : 0;
            CRYPT_EncodeLen(fileNameLen, NULL, &fileNameLenBytes);
            CRYPT_EncodeLen(1 + fileNameLenBytes + fileNameLen, NULL,
             &lenBytes);
            bytesNeeded = 2 + lenBytes + fileNameLenBytes + fileNameLen;
            if (!pbEncoded)
            {
                *pcbEncoded = bytesNeeded;
                ret = TRUE;
            }
            else if (*pcbEncoded < bytesNeeded)
            {
                SetLastError(ERROR_MORE_DATA);
                *pcbEncoded = bytesNeeded;
            }
            else
            {
                *pcbEncoded = bytesNeeded;
                *pbEncoded++ = ASN_CONSTRUCTOR | ASN_CONTEXT | 2;
                CRYPT_EncodeLen(1 + fileNameLenBytes + fileNameLen, pbEncoded,
                 &lenBytes);
                pbEncoded += lenBytes;
                *pbEncoded++ = ASN_CONTEXT;
                CRYPT_EncodeLen(fileNameLen, pbEncoded, &fileNameLenBytes);
                pbEncoded += fileNameLenBytes;
                for (ptr = link->u.pwszFile; ptr && *ptr; ptr++)
                {
                    *(WCHAR *)pbEncoded = hton16(*ptr);
                    pbEncoded += sizeof(WCHAR);
                }
                ret = TRUE;
            }
            break;
        }
        case SPC_MONIKER_LINK_CHOICE:
        {
            DWORD classIdLenBytes, dataLenBytes, dataLen;
            CRYPT_DATA_BLOB classId = { sizeof(link->u.Moniker.ClassId),
             (BYTE *)link->u.Moniker.ClassId };

            CRYPT_EncodeLen(classId.cbData, NULL, &classIdLenBytes);
            CRYPT_EncodeLen(link->u.Moniker.SerializedData.cbData, NULL,
             &dataLenBytes);
            dataLen = 2 + classIdLenBytes + classId.cbData +
             dataLenBytes + link->u.Moniker.SerializedData.cbData;
            CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
            bytesNeeded = 1 + dataLen + lenBytes;
            if (!pbEncoded)
            {
                *pcbEncoded = bytesNeeded;
                ret = TRUE;
            }
            else if (*pcbEncoded < bytesNeeded)
            {
                SetLastError(ERROR_MORE_DATA);
                *pcbEncoded = bytesNeeded;
            }
            else
            {
                DWORD size;

                *pcbEncoded = bytesNeeded;
                *pbEncoded++ = ASN_CONSTRUCTOR | ASN_CONTEXT | 1;
                CRYPT_EncodeLen(dataLen, pbEncoded, &lenBytes);
                pbEncoded += lenBytes;
                size = 1 + classIdLenBytes + classId.cbData;
                CRYPT_AsnEncodeOctets(X509_ASN_ENCODING, NULL, &classId,
                 pbEncoded, &size);
                pbEncoded += size;
                size = 1 + dataLenBytes + link->u.Moniker.SerializedData.cbData;
                CRYPT_AsnEncodeOctets(X509_ASN_ENCODING, NULL,
                 &link->u.Moniker.SerializedData, pbEncoded, &size);
                pbEncoded += size;
                ret = TRUE;
            }
            break;
        }
        case SPC_URL_LINK_CHOICE:
        {
            LPWSTR ptr;
            DWORD urlLen;

            /* Check for invalid characters in URL */
            ret = TRUE;
            urlLen = 0;
            for (ptr = link->u.pwszUrl; ptr && *ptr && ret; ptr++)
                if (*ptr > 0x7f)
                {
                    *pcbEncoded = 0;
                    SetLastError(CRYPT_E_INVALID_IA5_STRING);
                    ret = FALSE;
                }
                else
                    urlLen++;
            if (ret)
            {
                CRYPT_EncodeLen(urlLen, NULL, &lenBytes);
                bytesNeeded = 1 + lenBytes + urlLen;
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
                    *pcbEncoded = bytesNeeded;
                    *pbEncoded++ = ASN_CONTEXT;
                    CRYPT_EncodeLen(urlLen, pbEncoded, &lenBytes);
                    pbEncoded += lenBytes;
                    for (ptr = link->u.pwszUrl; ptr && *ptr; ptr++)
                        *pbEncoded++ = (BYTE)*ptr;
                }
            }
            break;
        }
        default:
            SetLastError(E_INVALIDARG);
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

typedef BOOL (WINAPI *CryptEncodeObjectFunc)(DWORD, LPCSTR, const void *,
 BYTE *, DWORD *);

struct AsnEncodeSequenceItem
{
    const void           *pvStructInfo;
    CryptEncodeObjectFunc encodeFunc;
    DWORD                 size; /* used during encoding, not for your use */
};

static BOOL WINAPI CRYPT_AsnEncodeSequence(DWORD dwCertEncodingType,
 struct AsnEncodeSequenceItem items[], DWORD cItem, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    BOOL ret;
    DWORD i, dataLen = 0;

    TRACE("%p, %d, %p, %d\n", items, cItem, pbEncoded, *pcbEncoded);
    for (i = 0, ret = TRUE; ret && i < cItem; i++)
    {
        ret = items[i].encodeFunc(dwCertEncodingType, NULL,
         items[i].pvStructInfo, NULL, &items[i].size);
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
        else if (*pcbEncoded < bytesNeeded)
        {
            *pcbEncoded = bytesNeeded;
            SetLastError(ERROR_MORE_DATA);
            ret = FALSE;
        }
        else
        {
            *pcbEncoded = bytesNeeded;
            *pbEncoded++ = ASN_SEQUENCE;
            CRYPT_EncodeLen(dataLen, pbEncoded, &lenBytes);
            pbEncoded += lenBytes;
            for (i = 0; ret && i < cItem; i++)
            {
                ret = items[i].encodeFunc(dwCertEncodingType, NULL,
                 items[i].pvStructInfo, pbEncoded, &items[i].size);
                /* Some functions propagate their errors through the size */
                if (!ret)
                    *pcbEncoded = items[i].size;
                pbEncoded += items[i].size;
            }
        }
    }
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeBits(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    BOOL ret = FALSE;

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
        else if (*pcbEncoded < bytesNeeded)
        {
            *pcbEncoded = bytesNeeded;
            SetLastError(ERROR_MORE_DATA);
        }
        else
        {
            ret = TRUE;
            *pcbEncoded = bytesNeeded;
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
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    return ret;
}

struct AsnConstructedItem
{
    BYTE                  tag;
    const void           *pvStructInfo;
    CryptEncodeObjectFunc encodeFunc;
};

static BOOL WINAPI CRYPT_AsnEncodeConstructed(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    BOOL ret;
    const struct AsnConstructedItem *item =
     (const struct AsnConstructedItem *)pvStructInfo;
    DWORD len;

    if ((ret = item->encodeFunc(dwCertEncodingType, lpszStructType,
     item->pvStructInfo, NULL, &len)))
    {
        DWORD dataLen, bytesNeeded;

        CRYPT_EncodeLen(len, NULL, &dataLen);
        bytesNeeded = 1 + dataLen + len;
        if (!pbEncoded)
            *pcbEncoded = bytesNeeded;
        else if (*pcbEncoded < bytesNeeded)
        {
            *pcbEncoded = bytesNeeded;
            SetLastError(ERROR_MORE_DATA);
            ret = FALSE;
        }
        else
        {
            *pcbEncoded = bytesNeeded;
            *pbEncoded++ = ASN_CONTEXT | ASN_CONSTRUCTOR | item->tag;
            CRYPT_EncodeLen(len, pbEncoded, &dataLen);
            pbEncoded += dataLen;
            ret = item->encodeFunc(dwCertEncodingType, lpszStructType,
             item->pvStructInfo, pbEncoded, &len);
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


BOOL WINAPI WVTAsn1SpcPeImageDataEncode(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    const SPC_PE_IMAGE_DATA *imageData =
     (const SPC_PE_IMAGE_DATA *)pvStructInfo;
    BOOL ret = FALSE;

    TRACE("(0x%08x, %s, %p, %p, %p)\n", dwCertEncodingType,
     debugstr_a(lpszStructType), pvStructInfo, pbEncoded,
     pcbEncoded);

    __TRY
    {
        struct AsnEncodeSequenceItem items[2] = {
         { 0 }
        };
        struct AsnConstructedItem constructed = { 0, imageData->pFile,
         WVTAsn1SpcLinkEncode };
        DWORD cItem = 0;

        if (imageData->Flags.cbData)
        {
            items[cItem].pvStructInfo = &imageData->Flags;
            items[cItem].encodeFunc = CRYPT_AsnEncodeBits;
            cItem++;
        }
        if (imageData->pFile)
        {
            items[cItem].pvStructInfo = &constructed;
            items[cItem].encodeFunc = CRYPT_AsnEncodeConstructed;
            cItem++;
        }

        ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, cItem,
         pbEncoded, pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeOid(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, BYTE *pbEncoded,
 DWORD *pcbEncoded)
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

static BOOL WINAPI CRYPT_CopyEncodedBlob(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    const CRYPT_DER_BLOB *blob = (const CRYPT_DER_BLOB *)pvStructInfo;
    BOOL ret = TRUE;

    if (!pbEncoded)
        *pcbEncoded = blob->cbData;
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
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeAlgorithmIdWithNullParams(
 DWORD dwCertEncodingType, LPCSTR lpszStructType, const void *pvStructInfo,
 BYTE *pbEncoded, DWORD *pcbEncoded)
{
    const CRYPT_ALGORITHM_IDENTIFIER *algo =
     (const CRYPT_ALGORITHM_IDENTIFIER *)pvStructInfo;
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
     sizeof(items) / sizeof(items[0]), pbEncoded, pcbEncoded);
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeAttributeTypeValue(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    const CRYPT_ATTRIBUTE_TYPE_VALUE *typeValue =
     (const CRYPT_ATTRIBUTE_TYPE_VALUE *)pvStructInfo;
    struct AsnEncodeSequenceItem items[] = {
     { &typeValue->pszObjId, CRYPT_AsnEncodeOid, 0 },
     { &typeValue->Value,    CRYPT_CopyEncodedBlob, 0 },
    };

    return CRYPT_AsnEncodeSequence(X509_ASN_ENCODING,
     items, sizeof(items) / sizeof(items[0]), pbEncoded, pcbEncoded);
}

struct SPCDigest
{
    CRYPT_ALGORITHM_IDENTIFIER DigestAlgorithm;
    CRYPT_HASH_BLOB            Digest;
};

static BOOL WINAPI CRYPT_AsnEncodeSPCDigest(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    const struct SPCDigest *digest = (const struct SPCDigest *)pvStructInfo;
    struct AsnEncodeSequenceItem items[] = {
     { &digest->DigestAlgorithm, CRYPT_AsnEncodeAlgorithmIdWithNullParams, 0 },
     { &digest->Digest,          CRYPT_CopyEncodedBlob, 0 },
    };

    return CRYPT_AsnEncodeSequence(X509_ASN_ENCODING,
     items, sizeof(items) / sizeof(items[0]), pbEncoded, pcbEncoded);
}

BOOL WINAPI WVTAsn1SpcIndirectDataContentEncode(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    BOOL ret = FALSE;

    TRACE("(0x%08x, %s, %p, %p, %p)\n", dwCertEncodingType,
     debugstr_a(lpszStructType), pvStructInfo, pbEncoded, pcbEncoded);

    __TRY
    {
        const SPC_INDIRECT_DATA_CONTENT *data =
         (const SPC_INDIRECT_DATA_CONTENT *)pvStructInfo;
        struct AsnEncodeSequenceItem items[] = {
         { &data->Data,            CRYPT_AsnEncodeAttributeTypeValue, 0 },
         { &data->DigestAlgorithm, CRYPT_AsnEncodeSPCDigest, 0 },
        };

        ret = CRYPT_AsnEncodeSequence(X509_ASN_ENCODING,
         items, sizeof(items) / sizeof(items[0]), pbEncoded, pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    return ret;
}

/* Gets the number of length bytes from the given (leading) length byte */
#define GET_LEN_BYTES(b) ((b) <= 0x7f ? 1 : 1 + ((b) & 0x7f))

/* Helper function to get the encoded length of the data starting at pbEncoded,
 * where pbEncoded[0] is the tag.  If the data are too short to contain a
 * length or if the length is too large for cbEncoded, sets an appropriate
 * error code and returns FALSE.
 */
static BOOL CRYPT_GetLen(const BYTE *pbEncoded, DWORD cbEncoded, DWORD *len)
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
        FIXME("unimplemented for indefinite-length encoding\n");
        SetLastError(CRYPT_E_ASN1_CORRUPT);
        ret = FALSE;
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

static BOOL WINAPI CRYPT_AsnDecodeOctets(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;
    DWORD bytesNeeded, dataLen;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

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
    else if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
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

static BOOL WINAPI CRYPT_AsnDecodeSPCLinkInternal(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;
    DWORD bytesNeeded = sizeof(SPC_LINK), dataLen;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
    {
        BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);
        DWORD realDataLen;

        switch (pbEncoded[0])
        {
        case ASN_CONTEXT:
            bytesNeeded += (dataLen + 1) * sizeof(WCHAR);
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
                PSPC_LINK link = (PSPC_LINK)pvStructInfo;
                DWORD i;

                link->dwLinkChoice = SPC_URL_LINK_CHOICE;
                for (i = 0; i < dataLen; i++)
                    link->u.pwszUrl[i] =
                     *(pbEncoded + 1 + lenBytes + i);
                link->u.pwszUrl[i] = '\0';
                TRACE("returning url %s\n", debugstr_w(link->u.pwszUrl));
            }
            break;
        case ASN_CONSTRUCTOR | ASN_CONTEXT | 1:
        {
            CRYPT_DATA_BLOB classId;
            DWORD size = sizeof(classId);

            if ((ret = CRYPT_AsnDecodeOctets(dwCertEncodingType, NULL,
             pbEncoded + 1 + lenBytes, cbEncoded - 1 - lenBytes,
             CRYPT_DECODE_NOCOPY_FLAG, &classId, &size)))
            {
                if (classId.cbData != sizeof(SPC_UUID))
                {
                    SetLastError(CRYPT_E_BAD_ENCODE);
                    ret = FALSE;
                }
                else
                {
                    CRYPT_DATA_BLOB data;

                    /* The tag length for the classId must be 1 since the
                     * length is correct.
                     */
                    size = sizeof(data);
                    if ((ret = CRYPT_AsnDecodeOctets(dwCertEncodingType, NULL,
                     pbEncoded + 3 + lenBytes + classId.cbData,
                     cbEncoded - 3 - lenBytes - classId.cbData,
                     CRYPT_DECODE_NOCOPY_FLAG, &data, &size)))
                    {
                        bytesNeeded += data.cbData;
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
                            PSPC_LINK link = (PSPC_LINK)pvStructInfo;

                            link->dwLinkChoice = SPC_MONIKER_LINK_CHOICE;
                            /* pwszFile pointer was set by caller, copy it
                             * before overwriting it
                             */
                            link->u.Moniker.SerializedData.pbData =
                             (BYTE *)link->u.pwszFile;
                            memcpy(link->u.Moniker.ClassId, classId.pbData,
                             classId.cbData);
                            memcpy(link->u.Moniker.SerializedData.pbData,
                             data.pbData, data.cbData);
                            link->u.Moniker.SerializedData.cbData = data.cbData;
                        }
                    }
                }
            }
            break;
        }
        case ASN_CONSTRUCTOR | ASN_CONTEXT | 2:
            if (dataLen && pbEncoded[1 + lenBytes] != ASN_CONTEXT)
                SetLastError(CRYPT_E_ASN1_BADTAG);
            else if ((ret = CRYPT_GetLen(pbEncoded + 1 + lenBytes, dataLen,
             &realDataLen)))
            {
                BYTE realLenBytes = GET_LEN_BYTES(pbEncoded[2 + lenBytes]);

                bytesNeeded += realDataLen + sizeof(WCHAR);
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
                    PSPC_LINK link = (PSPC_LINK)pvStructInfo;
                    DWORD i;
                    const BYTE *ptr = pbEncoded + 2 + lenBytes + realLenBytes;

                    link->dwLinkChoice = SPC_FILE_LINK_CHOICE;
                    for (i = 0; i < dataLen / sizeof(WCHAR); i++)
                        link->u.pwszFile[i] =
                         hton16(*(WORD *)(ptr + i * sizeof(WCHAR)));
                    link->u.pwszFile[realDataLen / sizeof(WCHAR)] = '\0';
                    TRACE("returning file %s\n", debugstr_w(link->u.pwszFile));
                }
            }
            else
            {
                bytesNeeded += sizeof(WCHAR);
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
                    PSPC_LINK link = (PSPC_LINK)pvStructInfo;

                    link->dwLinkChoice = SPC_FILE_LINK_CHOICE;
                    link->u.pwszFile[0] = '\0';
                    ret = TRUE;
                }
            }
            break;
        default:
            SetLastError(CRYPT_E_ASN1_BADTAG);
        }
    }
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI WVTAsn1SpcLinkDecode(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    __TRY
    {
        DWORD bytesNeeded;

        ret = CRYPT_AsnDecodeSPCLinkInternal(dwCertEncodingType,
         lpszStructType, pbEncoded, cbEncoded, dwFlags, NULL, &bytesNeeded);
        if (ret)
        {
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
                SPC_LINK *link = (SPC_LINK *)pvStructInfo;

                link->u.pwszFile =
                 (LPWSTR)((BYTE *)pvStructInfo + sizeof(SPC_LINK));
                ret = CRYPT_AsnDecodeSPCLinkInternal(dwCertEncodingType,
                 lpszStructType, pbEncoded, cbEncoded, dwFlags, pvStructInfo,
                 pcbStructInfo);
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

typedef BOOL (WINAPI *CryptDecodeObjectFunc)(DWORD, LPCSTR, const BYTE *,
 DWORD, DWORD, void *, DWORD *);

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
    BYTE                  tag;
    DWORD                 offset;
    CryptDecodeObjectFunc decodeFunc;
    DWORD                 minSize;
    BOOL                  optional;
    BOOL                  hasPointer;
    DWORD                 pointerOffset;
    DWORD                 size;
};

/* Decodes the items in a sequence, where the items are described in items,
 * the encoded data are in pbEncoded with length cbEncoded.  Decodes into
 * pvStructInfo.  nextData is a pointer to the memory location at which the
 * first decoded item with a dynamic pointer should point.
 * Upon decoding, *cbDecoded is the total number of bytes decoded.
 */
static BOOL CRYPT_AsnDecodeSequenceItems(DWORD dwCertEncodingType,
 struct AsnDecodeSequenceItem items[], DWORD cItem, const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, BYTE *nextData,
 DWORD *cbDecoded)
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
                         dwFlags & ~CRYPT_DECODE_ALLOC_FLAG,
                         pvStructInfo ?  (BYTE *)pvStructInfo + items[i].offset
                         : NULL, &items[i].size);
                        if (ret)
                        {
                            /* Account for alignment padding */
                            if (items[i].size % sizeof(DWORD))
                                items[i].size += sizeof(DWORD) -
                                 items[i].size % sizeof(DWORD);
                            TRACE("item %d size: %d\n", i, items[i].size);
                            if (nextData && items[i].hasPointer &&
                             items[i].size > items[i].minSize)
                                nextData += items[i].size - items[i].minSize;
                            ptr += 1 + nextItemLenBytes + nextItemLen;
                            decoded += 1 + nextItemLenBytes + nextItemLen;
                            TRACE("item %d: decoded %d bytes\n", i,
                             1 + nextItemLenBytes + nextItemLen);
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
                    {
                        TRACE("item %d: decoded %d bytes\n", i,
                         1 + nextItemLenBytes + nextItemLen);
                        ptr += 1 + nextItemLenBytes + nextItemLen;
                        decoded += 1 + nextItemLenBytes + nextItemLen;
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
    if (ret)
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
static BOOL CRYPT_AsnDecodeSequence(DWORD dwCertEncodingType,
 struct AsnDecodeSequenceItem items[], DWORD cItem, const BYTE *pbEncoded,
 DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo,
 void *startingPointer)
{
    BOOL ret;

    TRACE("%p, %d, %p, %d, %08x, %p, %d, %p\n", items, cItem, pbEncoded,
     cbEncoded, dwFlags, pvStructInfo, *pcbStructInfo, startingPointer);

    if (pbEncoded[0] == ASN_SEQUENCE)
    {
        DWORD dataLen;

        if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
        {
            DWORD lenBytes = GET_LEN_BYTES(pbEncoded[1]), cbDecoded;
            const BYTE *ptr = pbEncoded + 1 + lenBytes;

            cbEncoded -= 1 + lenBytes;
            if (cbEncoded < dataLen)
            {
                TRACE("dataLen %d exceeds cbEncoded %d, failing\n", dataLen,
                 cbEncoded);
                SetLastError(CRYPT_E_ASN1_CORRUPT);
                ret = FALSE;
            }
            else
                ret = CRYPT_AsnDecodeSequenceItems(dwFlags, items, cItem, ptr,
                 cbEncoded, dwFlags, NULL, NULL, &cbDecoded);
            if (ret && cbDecoded != dataLen)
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
                    bytesNeeded += items[i].size;
                    structSize += items[i].minSize;
                }
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
                    BYTE *nextData;

                    *pcbStructInfo = bytesNeeded;
                    if (startingPointer)
                        nextData = (BYTE *)startingPointer;
                    else
                        nextData = (BYTE *)pvStructInfo + structSize;
                    memset(pvStructInfo, 0, structSize);
                    ret = CRYPT_AsnDecodeSequenceItems(dwFlags, items, cItem,
                     ptr, cbEncoded, dwFlags, pvStructInfo, nextData,
                     &cbDecoded);
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

static BOOL WINAPI CRYPT_AsnDecodeBitsInternal(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;

    TRACE("(%p, %d, 0x%08x, %p, %d)\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

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

static BOOL WINAPI CRYPT_AsnDecodeSPCLinkPointer(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;
    DWORD dataLen;

    if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
    {
        BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);
        DWORD size;
        SPC_LINK **pLink = (SPC_LINK **)pvStructInfo;

        ret = CRYPT_AsnDecodeSPCLinkInternal(dwCertEncodingType, lpszStructType,
         pbEncoded + 1 + lenBytes, dataLen, dwFlags, NULL, &size);
        if (ret)
        {
            if (!pvStructInfo)
                *pcbStructInfo = size + sizeof(PSPC_LINK);
            else if (*pcbStructInfo < size + sizeof(PSPC_LINK))
            {
                *pcbStructInfo = size + sizeof(PSPC_LINK);
                SetLastError(ERROR_MORE_DATA);
                ret = FALSE;
            }
            else
            {
                *pcbStructInfo = size + sizeof(PSPC_LINK);
                /* Set imageData's pointer if necessary */
                if (size > sizeof(SPC_LINK))
                {
                    (*pLink)->u.pwszUrl =
                     (LPWSTR)((BYTE *)*pLink + sizeof(SPC_LINK));
                }
                ret = CRYPT_AsnDecodeSPCLinkInternal(dwCertEncodingType,
                 lpszStructType, pbEncoded + 1 + lenBytes, dataLen, dwFlags,
                 *pLink, pcbStructInfo);
            }
        }
    }
    return ret;
}

BOOL WINAPI WVTAsn1SpcPeImageDataDecode(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    __TRY
    {
        struct AsnDecodeSequenceItem items[] = {
         { ASN_BITSTRING, offsetof(SPC_PE_IMAGE_DATA, Flags),
           CRYPT_AsnDecodeBitsInternal, sizeof(CRYPT_BIT_BLOB), TRUE, TRUE,
           offsetof(SPC_PE_IMAGE_DATA, Flags.pbData), 0 },
         { ASN_CONSTRUCTOR | ASN_CONTEXT, offsetof(SPC_PE_IMAGE_DATA, pFile),
           CRYPT_AsnDecodeSPCLinkPointer, sizeof(PSPC_LINK), TRUE, TRUE,
           offsetof(SPC_PE_IMAGE_DATA, pFile), 0 },
        };

        ret = CRYPT_AsnDecodeSequence(dwCertEncodingType, items,
         sizeof(items) / sizeof(items[0]), pbEncoded, cbEncoded, dwFlags,
         pvStructInfo, pcbStructInfo, NULL);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeOidIgnoreTag(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 void *pvStructInfo, DWORD *pcbStructInfo)
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
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeOid(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    if (!cbEncoded)
        SetLastError(CRYPT_E_ASN1_CORRUPT);
    else if (pbEncoded[0] == ASN_OBJECTIDENTIFIER)
        ret = CRYPT_AsnDecodeOidIgnoreTag(dwCertEncodingType, lpszStructType,
         pbEncoded, cbEncoded, dwFlags, pvStructInfo, pcbStructInfo);
    else
        SetLastError(CRYPT_E_ASN1_BADTAG);
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeCopyBytes(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = TRUE;
    DWORD bytesNeeded = sizeof(CRYPT_OBJID_BLOB);

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

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

static BOOL WINAPI CRYPT_AsnDecodeAttributeTypeValue(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 void *pvStructInfo, DWORD *pcbStructInfo)
{
    CRYPT_ATTRIBUTE_TYPE_VALUE *typeValue =
     (CRYPT_ATTRIBUTE_TYPE_VALUE *)pvStructInfo;
    struct AsnDecodeSequenceItem items[] = {
     { ASN_OBJECTIDENTIFIER, offsetof(CRYPT_ATTRIBUTE_TYPE_VALUE, pszObjId),
       CRYPT_AsnDecodeOid, sizeof(LPSTR), FALSE, TRUE,
       offsetof(CRYPT_ATTRIBUTE_TYPE_VALUE, pszObjId), 0 },
     { 0, offsetof(CRYPT_ATTRIBUTE_TYPE_VALUE, Value),
       CRYPT_AsnDecodeCopyBytes, sizeof(CRYPT_DATA_BLOB), TRUE, TRUE,
       offsetof(CRYPT_ATTRIBUTE_TYPE_VALUE, Value.pbData), 0 },
    };

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    return CRYPT_AsnDecodeSequence(dwCertEncodingType, items,
     sizeof(items) / sizeof(items[0]), pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, pcbStructInfo,
     typeValue ? typeValue->pszObjId : NULL);
}

static BOOL WINAPI CRYPT_AsnDecodeAlgorithmId(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 void *pvStructInfo, DWORD *pcbStructInfo)
{
    CRYPT_ALGORITHM_IDENTIFIER *algo =
     (CRYPT_ALGORITHM_IDENTIFIER *)pvStructInfo;
    BOOL ret = TRUE;
    struct AsnDecodeSequenceItem items[] = {
     { ASN_OBJECTIDENTIFIER, offsetof(CRYPT_ALGORITHM_IDENTIFIER, pszObjId),
       CRYPT_AsnDecodeOidIgnoreTag, sizeof(LPSTR), FALSE, TRUE,
       offsetof(CRYPT_ALGORITHM_IDENTIFIER, pszObjId), 0 },
     { 0, offsetof(CRYPT_ALGORITHM_IDENTIFIER, Parameters),
       CRYPT_AsnDecodeCopyBytes, sizeof(CRYPT_OBJID_BLOB), TRUE, TRUE,
       offsetof(CRYPT_ALGORITHM_IDENTIFIER, Parameters.pbData), 0 },
    };

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    ret = CRYPT_AsnDecodeSequence(dwCertEncodingType, items,
     sizeof(items) / sizeof(items[0]), pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, pcbStructInfo, algo ? algo->pszObjId : NULL);
    if (ret && pvStructInfo)
    {
        TRACE("pszObjId is %p (%s)\n", algo->pszObjId,
         debugstr_a(algo->pszObjId));
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeSPCDigest(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 void *pvStructInfo, DWORD *pcbStructInfo)
{
    struct SPCDigest *digest =
     (struct SPCDigest *)pvStructInfo;
    struct AsnDecodeSequenceItem items[] = {
     { ASN_SEQUENCEOF, offsetof(struct SPCDigest, DigestAlgorithm),
       CRYPT_AsnDecodeAlgorithmId, sizeof(CRYPT_ALGORITHM_IDENTIFIER),
       FALSE, TRUE,
       offsetof(struct SPCDigest, DigestAlgorithm.pszObjId), 0 },
     { ASN_OCTETSTRING, offsetof(struct SPCDigest, Digest),
       CRYPT_AsnDecodeOctets, sizeof(CRYPT_DER_BLOB),
       FALSE, TRUE, offsetof(struct SPCDigest, Digest.pbData), 0 },
    };

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    return CRYPT_AsnDecodeSequence(dwCertEncodingType, items,
     sizeof(items) / sizeof(items[0]), pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, pcbStructInfo,
     digest ? digest->DigestAlgorithm.pszObjId : NULL);
}

BOOL WINAPI WVTAsn1SpcIndirectDataContentDecode(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    __TRY
    {
        struct AsnDecodeSequenceItem items[] = {
         { ASN_SEQUENCEOF, offsetof(SPC_INDIRECT_DATA_CONTENT, Data),
           CRYPT_AsnDecodeAttributeTypeValue,
           sizeof(CRYPT_ATTRIBUTE_TYPE_VALUE), FALSE, TRUE,
           offsetof(SPC_INDIRECT_DATA_CONTENT, Data.pszObjId), 0 },
         { ASN_SEQUENCEOF, offsetof(SPC_INDIRECT_DATA_CONTENT, DigestAlgorithm),
           CRYPT_AsnDecodeSPCDigest, sizeof(struct SPCDigest),
           FALSE, TRUE,
           offsetof(SPC_INDIRECT_DATA_CONTENT, DigestAlgorithm.pszObjId), 0 },
        };

        ret = CRYPT_AsnDecodeSequence(dwCertEncodingType, items,
         sizeof(items) / sizeof(items[0]), pbEncoded, cbEncoded, dwFlags,
         pvStructInfo, pcbStructInfo, NULL);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI WVTAsn1SpcSpOpusInfoDecode(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 void *pvStructInfo, DWORD *pcbStructInfo)
{
    FIXME("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);
    return FALSE;
}
