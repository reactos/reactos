/*
 * base64 encoder/decoder
 *
 * Copyright 2005 by Kai Blin
 * Copyright 2006 Juan Lang
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wincrypt.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

#define CERT_HEADER          "-----BEGIN CERTIFICATE-----"
#define CERT_TRAILER         "-----END CERTIFICATE-----"
#define CERT_REQUEST_HEADER  "-----BEGIN NEW CERTIFICATE REQUEST-----"
#define CERT_REQUEST_TRAILER "-----END NEW CERTIFICATE REQUEST-----"
#define X509_HEADER          "-----BEGIN X509 CRL-----"
#define X509_TRAILER         "-----END X509 CRL-----"

static const char b64[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

typedef BOOL (*BinaryToStringAFunc)(const BYTE *pbBinary,
 DWORD cbBinary, DWORD dwFlags, LPSTR pszString, DWORD *pcchString);

static BOOL EncodeBinaryToBinaryA(const BYTE *pbBinary,
 DWORD cbBinary, DWORD dwFlags, LPSTR pszString, DWORD *pcchString)
{
    BOOL ret = TRUE;

    if (*pcchString < cbBinary)
    {
        if (!pszString)
            *pcchString = cbBinary;
        else
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            *pcchString = cbBinary;
            ret = FALSE;
        }
    }
    else
    {
        if (cbBinary)
            memcpy(pszString, pbBinary, cbBinary);
        *pcchString = cbBinary;
    }
    return ret;
}

static LONG encodeBase64A(const BYTE *in_buf, int in_len, LPCSTR sep,
 char* out_buf, DWORD *out_len)
{
    int div, i;
    const BYTE *d = in_buf;
    int bytes = (in_len*8 + 5)/6, pad_bytes = (bytes % 4) ? 4 - (bytes % 4) : 0;
    DWORD needed;
    LPSTR ptr;

    TRACE("bytes is %d, pad bytes is %d\n", bytes, pad_bytes);
    needed = bytes + pad_bytes + 1;
    needed += (needed / 64 + 1) * strlen(sep);

    if (needed > *out_len)
    {
        *out_len = needed;
        return ERROR_INSUFFICIENT_BUFFER;
    }
    else
        *out_len = needed;

    /* Three bytes of input give 4 chars of output */
    div = in_len / 3;

    ptr = out_buf;
    i = 0;
    while (div > 0)
    {
        if (i && i % 64 == 0)
        {
            strcpy(ptr, sep);
            ptr += strlen(sep);
        }
        /* first char is the first 6 bits of the first byte*/
        *ptr++ = b64[ ( d[0] >> 2) & 0x3f ];
        /* second char is the last 2 bits of the first byte and the first 4
         * bits of the second byte */
        *ptr++ = b64[ ((d[0] << 4) & 0x30) | (d[1] >> 4 & 0x0f)];
        /* third char is the last 4 bits of the second byte and the first 2
         * bits of the third byte */
        *ptr++ = b64[ ((d[1] << 2) & 0x3c) | (d[2] >> 6 & 0x03)];
        /* fourth char is the remaining 6 bits of the third byte */
        *ptr++ = b64[   d[2]       & 0x3f];
        i += 4;
        d += 3;
        div--;
    }

    switch(pad_bytes)
    {
        case 1:
            /* first char is the first 6 bits of the first byte*/
            *ptr++ = b64[ ( d[0] >> 2) & 0x3f ];
            /* second char is the last 2 bits of the first byte and the first 4
             * bits of the second byte */
            *ptr++ = b64[ ((d[0] << 4) & 0x30) | (d[1] >> 4 & 0x0f)];
            /* third char is the last 4 bits of the second byte padded with
             * two zeroes */
            *ptr++ = b64[ ((d[1] << 2) & 0x3c) ];
            /* fourth char is a = to indicate one byte of padding */
            *ptr++ = '=';
            break;
        case 2:
            /* first char is the first 6 bits of the first byte*/
            *ptr++ = b64[ ( d[0] >> 2) & 0x3f ];
            /* second char is the last 2 bits of the first byte padded with
             * four zeroes*/
            *ptr++ = b64[ ((d[0] << 4) & 0x30)];
            /* third char is = to indicate padding */
            *ptr++ = '=';
            /* fourth char is = to indicate padding */
            *ptr++ = '=';
            break;
    }
    strcpy(ptr, sep);

    return ERROR_SUCCESS;
}

static BOOL BinaryToBase64A(const BYTE *pbBinary,
 DWORD cbBinary, DWORD dwFlags, LPSTR pszString, DWORD *pcchString)
{
    static const char crlf[] = "\r\n", lf[] = "\n";
    BOOL ret = TRUE;
    LPCSTR header = NULL, trailer = NULL, sep = NULL;
    DWORD charsNeeded;

    if (dwFlags & CRYPT_STRING_NOCR)
        sep = lf;
    else
        sep = crlf;
    switch (dwFlags & 0x7fffffff)
    {
    case CRYPT_STRING_BASE64:
        /* no header or footer */
        break;
    case CRYPT_STRING_BASE64HEADER:
        header = CERT_HEADER;
        trailer = CERT_TRAILER;
        break;
    case CRYPT_STRING_BASE64REQUESTHEADER:
        header = CERT_REQUEST_HEADER;
        trailer = CERT_REQUEST_TRAILER;
        break;
    case CRYPT_STRING_BASE64X509CRLHEADER:
        header = X509_HEADER;
        trailer = X509_TRAILER;
        break;
    }

    charsNeeded = 0;
    encodeBase64A(pbBinary, cbBinary, sep, NULL, &charsNeeded);
    charsNeeded += strlen(sep);
    if (header)
        charsNeeded += strlen(header) + strlen(sep);
    if (trailer)
        charsNeeded += strlen(trailer) + strlen(sep);
    if (charsNeeded <= *pcchString)
    {
        LPSTR ptr = pszString;
        DWORD size = charsNeeded;

        if (header)
        {
            strcpy(ptr, header);
            ptr += strlen(ptr);
            strcpy(ptr, sep);
            ptr += strlen(sep);
        }
        encodeBase64A(pbBinary, cbBinary, sep, ptr, &size);
        ptr += size - 1;
        if (trailer)
        {
            strcpy(ptr, trailer);
            ptr += strlen(ptr);
            strcpy(ptr, sep);
            ptr += strlen(sep);
        }
        *pcchString = charsNeeded - 1;
    }
    else if (pszString)
    {
        *pcchString = charsNeeded;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        ret = FALSE;
    }
    else
        *pcchString = charsNeeded;
    return ret;
}

BOOL WINAPI CryptBinaryToStringA(const BYTE *pbBinary,
 DWORD cbBinary, DWORD dwFlags, LPSTR pszString, DWORD *pcchString)
{
    BinaryToStringAFunc encoder = NULL;

    TRACE("(%p, %d, %08x, %p, %p)\n", pbBinary, cbBinary, dwFlags, pszString,
     pcchString);

    if (!pbBinary)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!pcchString)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    switch (dwFlags & 0x7fffffff)
    {
    case CRYPT_STRING_BINARY:
        encoder = EncodeBinaryToBinaryA;
        break;
    case CRYPT_STRING_BASE64:
    case CRYPT_STRING_BASE64HEADER:
    case CRYPT_STRING_BASE64REQUESTHEADER:
    case CRYPT_STRING_BASE64X509CRLHEADER:
        encoder = BinaryToBase64A;
        break;
    case CRYPT_STRING_HEX:
    case CRYPT_STRING_HEXASCII:
    case CRYPT_STRING_HEXADDR:
    case CRYPT_STRING_HEXASCIIADDR:
        FIXME("Unimplemented type %d\n", dwFlags & 0x7fffffff);
        /* fall through */
    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return encoder(pbBinary, cbBinary, dwFlags, pszString, pcchString);
}

static inline BYTE decodeBase64Byte(char c)
{
    BYTE ret;

    if (c >= 'A' && c <= 'Z')
        ret = c - 'A';
    else if (c >= 'a' && c <= 'z')
        ret = c - 'a' + 26;
    else if (c >= '0' && c <= '9')
        ret = c - '0' + 52;
    else if (c == '+')
        ret = 62;
    else if (c == '/')
        ret = 63;
    else
        ret = 64;
    return ret;
}

static LONG decodeBase64Block(const char *in_buf, int in_len,
 const char **nextBlock, PBYTE out_buf, DWORD *out_len)
{
    int len = in_len, i;
    const char *d = in_buf;
    int  ip0, ip1, ip2, ip3;

    if (len < 4)
        return ERROR_INVALID_DATA;

    i = 0;
    if (d[2] == '=')
    {
        if ((ip0 = decodeBase64Byte(d[0])) > 63)
            return ERROR_INVALID_DATA;
        if ((ip1 = decodeBase64Byte(d[1])) > 63)
            return ERROR_INVALID_DATA;

        if (out_buf)
            out_buf[i] = (ip0 << 2) | (ip1 >> 4);
        i++;
    }
    else if (d[3] == '=')
    {
        if ((ip0 = decodeBase64Byte(d[0])) > 63)
            return ERROR_INVALID_DATA;
        if ((ip1 = decodeBase64Byte(d[1])) > 63)
            return ERROR_INVALID_DATA;
        if ((ip2 = decodeBase64Byte(d[2])) > 63)
            return ERROR_INVALID_DATA;

        if (out_buf)
        {
            out_buf[i + 0] = (ip0 << 2) | (ip1 >> 4);
            out_buf[i + 1] = (ip1 << 4) | (ip2 >> 2);
        }
        i += 2;
    }
    else
    {
        if ((ip0 = decodeBase64Byte(d[0])) > 63)
            return ERROR_INVALID_DATA;
        if ((ip1 = decodeBase64Byte(d[1])) > 63)
            return ERROR_INVALID_DATA;
        if ((ip2 = decodeBase64Byte(d[2])) > 63)
            return ERROR_INVALID_DATA;
        if ((ip3 = decodeBase64Byte(d[3])) > 63)
            return ERROR_INVALID_DATA;

        if (out_buf)
        {
            out_buf[i + 0] = (ip0 << 2) | (ip1 >> 4);
            out_buf[i + 1] = (ip1 << 4) | (ip2 >> 2);
            out_buf[i + 2] = (ip2 << 6) |  ip3;
        }
        i += 3;
    }
    if (len >= 6 && d[4] == '\r' && d[5] == '\n')
        *nextBlock = d + 6;
    else if (len >= 5 && d[4] == '\n')
        *nextBlock = d + 5;
    else if (len >= 4 && d[4])
        *nextBlock = d + 4;
    else
        *nextBlock = NULL;
    *out_len = i;
    return ERROR_SUCCESS;
}

/* Unlike CryptStringToBinaryA, cchString is guaranteed to be the length of the
 * string to convert.
 */
typedef LONG (*StringToBinaryAFunc)(LPCSTR pszString, DWORD cchString,
 BYTE *pbBinary, DWORD *pcbBinary, DWORD *pdwSkip, DWORD *pdwFlags);

static LONG Base64ToBinaryA(LPCSTR pszString, DWORD cchString,
 BYTE *pbBinary, DWORD *pcbBinary, DWORD *pdwSkip, DWORD *pdwFlags)
{
    LONG ret = ERROR_SUCCESS;
    const char *nextBlock;
    DWORD outLen = 0;

    nextBlock = pszString;
    while (nextBlock && !ret)
    {
        DWORD len = 0;

        ret = decodeBase64Block(nextBlock, cchString - (nextBlock - pszString),
         &nextBlock, pbBinary ? pbBinary + outLen : NULL, &len);
        if (!ret)
            outLen += len;
        if (cchString - (nextBlock - pszString) <= 0)
            nextBlock = NULL;
    }
    *pcbBinary = outLen;
    if (!ret)
    {
        if (pdwSkip)
            *pdwSkip = 0;
        if (pdwFlags)
            *pdwFlags = CRYPT_STRING_BASE64;
    }
    else if (ret == ERROR_INSUFFICIENT_BUFFER)
    {
        if (!pbBinary)
            ret = ERROR_SUCCESS;
    }
    return ret;
}

static LONG Base64WithHeaderAndTrailerToBinaryA(LPCSTR pszString,
 DWORD cchString, LPCSTR header, LPCSTR trailer, BYTE *pbBinary,
 DWORD *pcbBinary, DWORD *pdwSkip)
{
    LONG ret;
    LPCSTR ptr;

    if (cchString > strlen(header) + strlen(trailer)
     && (ptr = strstr(pszString, header)) != NULL)
    {
        LPCSTR trailerSpot = pszString + cchString - strlen(trailer);

        if (pszString[cchString - 1] == '\n')
        {
            cchString--;
            trailerSpot--;
        }
        if (pszString[cchString - 1] == '\r')
        {
            cchString--;
            trailerSpot--;
        }
        if (!strncmp(trailerSpot, trailer, strlen(trailer)))
        {
            if (pdwSkip)
                *pdwSkip = ptr - pszString;
            ptr += strlen(header);
            if (*ptr == '\r') ptr++;
            if (*ptr == '\n') ptr++;
            cchString -= ptr - pszString + strlen(trailer);
            ret = Base64ToBinaryA(ptr, cchString, pbBinary, pcbBinary, NULL,
             NULL);
        }
        else
            ret = ERROR_INVALID_DATA;
    }
    else
        ret = ERROR_INVALID_DATA;
    return ret;
}

static LONG Base64HeaderToBinaryA(LPCSTR pszString, DWORD cchString,
 BYTE *pbBinary, DWORD *pcbBinary, DWORD *pdwSkip, DWORD *pdwFlags)
{
    LONG ret = Base64WithHeaderAndTrailerToBinaryA(pszString, cchString,
     CERT_HEADER, CERT_TRAILER, pbBinary, pcbBinary, pdwSkip);

    if (!ret && pdwFlags)
        *pdwFlags = CRYPT_STRING_BASE64HEADER;
    return ret;
}

static LONG Base64RequestHeaderToBinaryA(LPCSTR pszString, DWORD cchString,
 BYTE *pbBinary, DWORD *pcbBinary, DWORD *pdwSkip, DWORD *pdwFlags)
{
    LONG ret = Base64WithHeaderAndTrailerToBinaryA(pszString, cchString,
     CERT_REQUEST_HEADER, CERT_REQUEST_TRAILER, pbBinary, pcbBinary, pdwSkip);

    if (!ret && pdwFlags)
        *pdwFlags = CRYPT_STRING_BASE64REQUESTHEADER;
    return ret;
}

static LONG Base64X509HeaderToBinaryA(LPCSTR pszString, DWORD cchString,
 BYTE *pbBinary, DWORD *pcbBinary, DWORD *pdwSkip, DWORD *pdwFlags)
{
    LONG ret = Base64WithHeaderAndTrailerToBinaryA(pszString, cchString,
     X509_HEADER, X509_TRAILER, pbBinary, pcbBinary, pdwSkip);

    if (!ret && pdwFlags)
        *pdwFlags = CRYPT_STRING_BASE64X509CRLHEADER;
    return ret;
}

static LONG Base64AnyToBinaryA(LPCSTR pszString, DWORD cchString,
 BYTE *pbBinary, DWORD *pcbBinary, DWORD *pdwSkip, DWORD *pdwFlags)
{
    LONG ret;

    ret = Base64HeaderToBinaryA(pszString, cchString, pbBinary, pcbBinary,
     pdwSkip, pdwFlags);
    if (ret == ERROR_INVALID_DATA)
        ret = Base64ToBinaryA(pszString, cchString, pbBinary, pcbBinary,
         pdwSkip, pdwFlags);
    return ret;
}

static LONG DecodeBinaryToBinaryA(LPCSTR pszString, DWORD cchString,
 BYTE *pbBinary, DWORD *pcbBinary, DWORD *pdwSkip, DWORD *pdwFlags)
{
    LONG ret = ERROR_SUCCESS;

    if (*pcbBinary < cchString)
    {
        if (!pbBinary)
            *pcbBinary = cchString;
        else
        {
            ret = ERROR_INSUFFICIENT_BUFFER;
            *pcbBinary = cchString;
        }
    }
    else
    {
        if (cchString)
            memcpy(pbBinary, pszString, cchString);
        *pcbBinary = cchString;
    }
    return ret;
}

static LONG DecodeAnyA(LPCSTR pszString, DWORD cchString,
 BYTE *pbBinary, DWORD *pcbBinary, DWORD *pdwSkip, DWORD *pdwFlags)
{
    LONG ret;

    ret = Base64HeaderToBinaryA(pszString, cchString, pbBinary, pcbBinary,
     pdwSkip, pdwFlags);
    if (ret == ERROR_INVALID_DATA)
        ret = Base64ToBinaryA(pszString, cchString, pbBinary, pcbBinary,
         pdwSkip, pdwFlags);
    if (ret == ERROR_INVALID_DATA)
        ret = DecodeBinaryToBinaryA(pszString, cchString, pbBinary, pcbBinary,
         pdwSkip, pdwFlags);
    return ret;
}

BOOL WINAPI CryptStringToBinaryA(LPCSTR pszString,
 DWORD cchString, DWORD dwFlags, BYTE *pbBinary, DWORD *pcbBinary,
 DWORD *pdwSkip, DWORD *pdwFlags)
{
    StringToBinaryAFunc decoder;
    LONG ret;

    TRACE("(%s, %d, %08x, %p, %p, %p, %p)\n", debugstr_a(pszString),
     cchString, dwFlags, pbBinary, pcbBinary, pdwSkip, pdwFlags);

    if (!pszString)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    /* Only the bottom byte contains valid types */
    if (dwFlags & 0xfffffff0)
    {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }
    switch (dwFlags)
    {
    case CRYPT_STRING_BASE64_ANY:
        decoder = Base64AnyToBinaryA;
        break;
    case CRYPT_STRING_BASE64:
        decoder = Base64ToBinaryA;
        break;
    case CRYPT_STRING_BASE64HEADER:
        decoder = Base64HeaderToBinaryA;
        break;
    case CRYPT_STRING_BASE64REQUESTHEADER:
        decoder = Base64RequestHeaderToBinaryA;
        break;
    case CRYPT_STRING_BASE64X509CRLHEADER:
        decoder = Base64X509HeaderToBinaryA;
        break;
    case CRYPT_STRING_BINARY:
        decoder = DecodeBinaryToBinaryA;
        break;
    case CRYPT_STRING_ANY:
        decoder = DecodeAnyA;
        break;
    case CRYPT_STRING_HEX:
    case CRYPT_STRING_HEXASCII:
    case CRYPT_STRING_HEXADDR:
    case CRYPT_STRING_HEXASCIIADDR:
        FIXME("Unimplemented type %d\n", dwFlags & 0x7fffffff);
        /* fall through */
    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!cchString)
        cchString = strlen(pszString);
    ret = decoder(pszString, cchString, pbBinary, pcbBinary, pdwSkip, pdwFlags);
    if (ret)
        SetLastError(ret);
    return (ret == ERROR_SUCCESS) ? TRUE : FALSE;
}
