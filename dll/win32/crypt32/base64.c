/*
 * base64 encoder/decoder
 *
 * Copyright 2005 by Kai Blin
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

#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

#define CERT_HEADER          "-----BEGIN CERTIFICATE-----"
#define CERT_HEADER_START    "-----BEGIN "
#define CERT_DELIMITER       "-----"
#define CERT_TRAILER         "-----END CERTIFICATE-----"
#define CERT_TRAILER_START   "-----END "
#define CERT_REQUEST_HEADER  "-----BEGIN NEW CERTIFICATE REQUEST-----"
#define CERT_REQUEST_TRAILER "-----END NEW CERTIFICATE REQUEST-----"
#define X509_HEADER          "-----BEGIN X509 CRL-----"
#define X509_TRAILER         "-----END X509 CRL-----"

static const WCHAR CERT_HEADER_W[] = {
'-','-','-','-','-','B','E','G','I','N',' ','C','E','R','T','I','F','I','C',
'A','T','E','-','-','-','-','-',0 };
static const WCHAR CERT_HEADER_START_W[] = {
'-','-','-','-','-','B','E','G','I','N',' ',0 };
static const WCHAR CERT_DELIMITER_W[] = {
'-','-','-','-','-',0 };
static const WCHAR CERT_TRAILER_W[] = {
'-','-','-','-','-','E','N','D',' ','C','E','R','T','I','F','I','C','A','T',
'E','-','-','-','-','-',0 };
static const WCHAR CERT_TRAILER_START_W[] = {
'-','-','-','-','-','E','N','D',' ',0 };
static const WCHAR CERT_REQUEST_HEADER_W[] = {
'-','-','-','-','-','B','E','G','I','N',' ','N','E','W',' ','C','E','R','T',
'I','F','I','C','A','T','E','R','E','Q','U','E','S','T','-','-','-','-','-',0 };
static const WCHAR CERT_REQUEST_TRAILER_W[] = {
'-','-','-','-','-','E','N','D',' ','N','E','W',' ','C','E','R','T','I','F',
'I','C','A','T','E','R','E','Q','U','E','S','T','-','-','-','-','-',0 };
static const WCHAR X509_HEADER_W[] = {
'-','-','-','-','-','B','E','G','I','N',' ','X','5','0','9',' ','C','R','L',
'-','-','-','-','-',0 };
static const WCHAR X509_TRAILER_W[] = {
'-','-','-','-','-','E','N','D',' ','X','5','0','9',' ','C','R','L','-','-',
'-','-','-',0 };

static const char b64[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

typedef BOOL (*BinaryToStringAFunc)(const BYTE *pbBinary,
 DWORD cbBinary, DWORD dwFlags, LPSTR pszString, DWORD *pcchString);
typedef BOOL (*BinaryToStringWFunc)(const BYTE *pbBinary,
 DWORD cbBinary, DWORD dwFlags, LPWSTR pszString, DWORD *pcchString);

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
    LPCSTR header = NULL, trailer = NULL, sep;
    DWORD charsNeeded;

    if (dwFlags & CRYPT_STRING_NOCR)
        sep = lf;
    else if (dwFlags & CRYPT_STRING_NOCRLF)
        sep = "";
    else
        sep = crlf;
    switch (dwFlags & 0x0fffffff)
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

    switch (dwFlags & 0x0fffffff)
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
        FIXME("Unimplemented type %d\n", dwFlags & 0x0fffffff);
        /* fall through */
    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return encoder(pbBinary, cbBinary, dwFlags, pszString, pcchString);
}

static LONG encodeBase64W(const BYTE *in_buf, int in_len, LPCWSTR sep,
 WCHAR* out_buf, DWORD *out_len)
{
    int div, i;
    const BYTE *d = in_buf;
    int bytes = (in_len*8 + 5)/6, pad_bytes = (bytes % 4) ? 4 - (bytes % 4) : 0;
    DWORD needed;
    LPWSTR ptr;

    TRACE("bytes is %d, pad bytes is %d\n", bytes, pad_bytes);
    needed = bytes + pad_bytes + 1;
    needed += (needed / 64 + 1) * strlenW(sep);

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
            strcpyW(ptr, sep);
            ptr += strlenW(sep);
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
    strcpyW(ptr, sep);

    return ERROR_SUCCESS;
}

static BOOL BinaryToBase64W(const BYTE *pbBinary,
 DWORD cbBinary, DWORD dwFlags, LPWSTR pszString, DWORD *pcchString)
{
    static const WCHAR crlf[] = { '\r','\n',0 }, lf[] = { '\n',0 }, empty[] = {0};
    BOOL ret = TRUE;
    LPCWSTR header = NULL, trailer = NULL, sep;
    DWORD charsNeeded;

    if (dwFlags & CRYPT_STRING_NOCR)
        sep = lf;
    else if (dwFlags & CRYPT_STRING_NOCRLF)
        sep = empty;
    else
        sep = crlf;
    switch (dwFlags & 0x0fffffff)
    {
    case CRYPT_STRING_BASE64:
        /* no header or footer */
        break;
    case CRYPT_STRING_BASE64HEADER:
        header = CERT_HEADER_W;
        trailer = CERT_TRAILER_W;
        break;
    case CRYPT_STRING_BASE64REQUESTHEADER:
        header = CERT_REQUEST_HEADER_W;
        trailer = CERT_REQUEST_TRAILER_W;
        break;
    case CRYPT_STRING_BASE64X509CRLHEADER:
        header = X509_HEADER_W;
        trailer = X509_TRAILER_W;
        break;
    }

    charsNeeded = 0;
    encodeBase64W(pbBinary, cbBinary, sep, NULL, &charsNeeded);
    if (header)
        charsNeeded += strlenW(header) + strlenW(sep);
    if (trailer)
        charsNeeded += strlenW(trailer) + strlenW(sep);
    if (charsNeeded <= *pcchString)
    {
        LPWSTR ptr = pszString;
        DWORD size = charsNeeded;

        if (header)
        {
            strcpyW(ptr, header);
            ptr += strlenW(ptr);
            strcpyW(ptr, sep);
            ptr += strlenW(sep);
        }
        encodeBase64W(pbBinary, cbBinary, sep, ptr, &size);
        ptr += size - 1;
        if (trailer)
        {
            strcpyW(ptr, trailer);
            ptr += strlenW(ptr);
            strcpyW(ptr, sep);
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

BOOL WINAPI CryptBinaryToStringW(const BYTE *pbBinary,
 DWORD cbBinary, DWORD dwFlags, LPWSTR pszString, DWORD *pcchString)
{
    BinaryToStringWFunc encoder = NULL;

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

    switch (dwFlags & 0x0fffffff)
    {
    case CRYPT_STRING_BASE64:
    case CRYPT_STRING_BASE64HEADER:
    case CRYPT_STRING_BASE64REQUESTHEADER:
    case CRYPT_STRING_BASE64X509CRLHEADER:
        encoder = BinaryToBase64W;
        break;
    case CRYPT_STRING_BINARY:
    case CRYPT_STRING_HEX:
    case CRYPT_STRING_HEXASCII:
    case CRYPT_STRING_HEXADDR:
    case CRYPT_STRING_HEXASCIIADDR:
        FIXME("Unimplemented type %d\n", dwFlags & 0x0fffffff);
        /* fall through */
    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return encoder(pbBinary, cbBinary, dwFlags, pszString, pcchString);
}

#define BASE64_DECODE_PADDING    0x100
#define BASE64_DECODE_WHITESPACE 0x200
#define BASE64_DECODE_INVALID    0x300

static inline int decodeBase64Byte(int c)
{
    int ret = BASE64_DECODE_INVALID;

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
    else if (c == '=')
        ret = BASE64_DECODE_PADDING;
    else if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
        ret = BASE64_DECODE_WHITESPACE;
    return ret;
}

/* Unlike CryptStringToBinaryA, cchString is guaranteed to be the length of the
 * string to convert.
 */
typedef LONG (*StringToBinaryAFunc)(LPCSTR pszString, DWORD cchString,
 BYTE *pbBinary, DWORD *pcbBinary, DWORD *pdwSkip, DWORD *pdwFlags);

static LONG Base64ToBinary(const void* pszString, BOOL wide, DWORD cchString,
 BYTE *pbBinary, DWORD *pcbBinary, DWORD *pdwSkip, DWORD *pdwFlags)
{
    DWORD cbIn, cbValid, cbOut, hasPadding;
    BYTE block[4];
    for (cbIn = cbValid = cbOut = hasPadding = 0; cbIn < cchString; ++cbIn)
    {
        int c = wide ? (int)((WCHAR*)pszString)[cbIn] : (int)((char*)pszString)[cbIn];
        int d = decodeBase64Byte(c);
        if (d == BASE64_DECODE_INVALID)
            goto invalid;
        if (d == BASE64_DECODE_WHITESPACE)
            continue;

        /* When padding starts, data is not acceptable */
        if (hasPadding && d != BASE64_DECODE_PADDING)
            goto invalid;

        /* Padding after a full block (like "VVVV=") is ok and stops decoding */
        if (d == BASE64_DECODE_PADDING && (cbValid & 3) == 0)
            break;

        cbValid += 1;

        if (d == BASE64_DECODE_PADDING)
        {
            hasPadding = 1;
            /* When padding reaches a full block, stop decoding */
            if ((cbValid & 3) == 0)
                break;
            continue;
        }

        /* cbOut is incremented in the 4-char block as follows: "1-23" */
        if ((cbValid & 3) != 2)
            cbOut += 1;
    }
    /* Fail if the block has bad padding; omitting padding is fine */
    if ((cbValid & 3) != 0 && hasPadding)
        goto invalid;
    /* Check available buffer size */
    if (pbBinary && *pcbBinary && cbOut > *pcbBinary)
        goto overflow;
    /* Convert the data; this step depends on the validity checks above! */
    if (pbBinary) for (cbIn = cbValid = cbOut = 0; cbIn < cchString; ++cbIn)
    {
        int c = wide ? (int)((WCHAR*)pszString)[cbIn] : (int)((char*)pszString)[cbIn];
        int d = decodeBase64Byte(c);
        if (d == BASE64_DECODE_WHITESPACE)
            continue;
        if (d == BASE64_DECODE_PADDING)
            break;
        block[cbValid & 3] = d;
        cbValid += 1;
        switch (cbValid & 3) {
        case 1:
            pbBinary[cbOut++] = (block[0] << 2);
            break;
        case 2:
            pbBinary[cbOut-1] = (block[0] << 2) | (block[1] >> 4);
            break;
        case 3:
            pbBinary[cbOut++] = (block[1] << 4) | (block[2] >> 2);
            break;
        case 0:
            pbBinary[cbOut++] = (block[2] << 6) | (block[3] >> 0);
            break;
        }
    }
    *pcbBinary = cbOut;
    if (pdwSkip)
        *pdwSkip = 0;
    if (pdwFlags)
        *pdwFlags = CRYPT_STRING_BASE64;
    return ERROR_SUCCESS;
overflow:
    return ERROR_INSUFFICIENT_BUFFER;
invalid:
    *pcbBinary = cbOut;
    return ERROR_INVALID_DATA;
}

static LONG Base64ToBinaryA(LPCSTR pszString, DWORD cchString,
 BYTE *pbBinary, DWORD *pcbBinary, DWORD *pdwSkip, DWORD *pdwFlags)
{
    return Base64ToBinary(pszString, FALSE, cchString, pbBinary, pcbBinary, pdwSkip, pdwFlags);
}

static LONG Base64WithHeaderAndTrailerToBinaryA(LPCSTR pszString,
 DWORD cchString, BYTE *pbBinary,
 DWORD *pcbBinary, DWORD *pdwSkip)
{
    LONG ret;
    LPCSTR header = CERT_HEADER_START;
    LPCSTR trailer = CERT_TRAILER_START;

    LPCSTR headerBegins;
    LPCSTR dataBegins;
    LPCSTR trailerBegins;
    size_t dataLength;

    if ((strlen(header) + strlen(trailer)) > cchString)
    {
        return ERROR_INVALID_DATA;
    }

    if (!(headerBegins = strstr(pszString, header)))
    {
        TRACE("Can't find %s in %s.\n", header, pszString);
        return ERROR_INVALID_DATA;
    }

    dataBegins = headerBegins + strlen(header);
    if (!(dataBegins = strstr(dataBegins, CERT_DELIMITER)))
    {
        return ERROR_INVALID_DATA;
    }
    dataBegins += strlen(CERT_DELIMITER);
    if (*dataBegins == '\r') dataBegins++;
    if (*dataBegins == '\n') dataBegins++;

    if (!(trailerBegins = strstr(dataBegins, trailer)))
    {
        return ERROR_INVALID_DATA;
    }
    if (*(trailerBegins-1) == '\n') trailerBegins--;
    if (*(trailerBegins-1) == '\r') trailerBegins--;

    if (pdwSkip)
       *pdwSkip = headerBegins - pszString;

    dataLength = trailerBegins - dataBegins;

    ret = Base64ToBinaryA(dataBegins, dataLength, pbBinary, pcbBinary, NULL,
          NULL);

    return ret;
}

static LONG Base64HeaderToBinaryA(LPCSTR pszString, DWORD cchString,
 BYTE *pbBinary, DWORD *pcbBinary, DWORD *pdwSkip, DWORD *pdwFlags)
{
    LONG ret = Base64WithHeaderAndTrailerToBinaryA(pszString, cchString,
     pbBinary, pcbBinary, pdwSkip);

    if (!ret && pdwFlags)
        *pdwFlags = CRYPT_STRING_BASE64HEADER;
    return ret;
}

static LONG Base64RequestHeaderToBinaryA(LPCSTR pszString, DWORD cchString,
 BYTE *pbBinary, DWORD *pcbBinary, DWORD *pdwSkip, DWORD *pdwFlags)
{
    LONG ret = Base64WithHeaderAndTrailerToBinaryA(pszString, cchString,
     pbBinary, pcbBinary, pdwSkip);

    if (!ret && pdwFlags)
        *pdwFlags = CRYPT_STRING_BASE64REQUESTHEADER;
    return ret;
}

static LONG Base64X509HeaderToBinaryA(LPCSTR pszString, DWORD cchString,
 BYTE *pbBinary, DWORD *pcbBinary, DWORD *pdwSkip, DWORD *pdwFlags)
{
    LONG ret = Base64WithHeaderAndTrailerToBinaryA(pszString, cchString,
     pbBinary, pcbBinary, pdwSkip);

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
    return ret == ERROR_SUCCESS;
}

/* Unlike CryptStringToBinaryW, cchString is guaranteed to be the length of the
 * string to convert.
 */
typedef LONG (*StringToBinaryWFunc)(LPCWSTR pszString, DWORD cchString,
 BYTE *pbBinary, DWORD *pcbBinary, DWORD *pdwSkip, DWORD *pdwFlags);

static LONG Base64ToBinaryW(LPCWSTR pszString, DWORD cchString,
 BYTE *pbBinary, DWORD *pcbBinary, DWORD *pdwSkip, DWORD *pdwFlags)
{
    return Base64ToBinary(pszString, TRUE, cchString, pbBinary, pcbBinary, pdwSkip, pdwFlags);
}

static LONG Base64WithHeaderAndTrailerToBinaryW(LPCWSTR pszString,
 DWORD cchString, BYTE *pbBinary,
 DWORD *pcbBinary, DWORD *pdwSkip)
{
    LONG ret;
    LPCWSTR header = CERT_HEADER_START_W;
    LPCWSTR trailer = CERT_TRAILER_START_W;

    LPCWSTR headerBegins;
    LPCWSTR dataBegins;
    LPCWSTR trailerBegins;
    size_t dataLength;

    if ((strlenW(header) + strlenW(trailer)) > cchString)
    {
        return ERROR_INVALID_DATA;
    }

    if (!(headerBegins = strstrW(pszString, header)))
    {
        TRACE("Can't find %s in %s.\n", debugstr_w(header), debugstr_w(pszString));
        return ERROR_INVALID_DATA;
    }

    dataBegins = headerBegins + strlenW(header);
    if (!(dataBegins = strstrW(dataBegins, CERT_DELIMITER_W)))
    {
        return ERROR_INVALID_DATA;
    }
    dataBegins += strlenW(CERT_DELIMITER_W);
    if (*dataBegins == '\r') dataBegins++;
    if (*dataBegins == '\n') dataBegins++;

    if (!(trailerBegins = strstrW(dataBegins, trailer)))
    {
        return ERROR_INVALID_DATA;
    }
    if (*(trailerBegins-1) == '\n') trailerBegins--;
    if (*(trailerBegins-1) == '\r') trailerBegins--;

    if (pdwSkip)
       *pdwSkip = headerBegins - pszString;

    dataLength = trailerBegins - dataBegins;

    ret = Base64ToBinaryW(dataBegins, dataLength, pbBinary, pcbBinary, NULL,
          NULL);

    return ret;
}

static LONG Base64HeaderToBinaryW(LPCWSTR pszString, DWORD cchString,
 BYTE *pbBinary, DWORD *pcbBinary, DWORD *pdwSkip, DWORD *pdwFlags)
{
    LONG ret = Base64WithHeaderAndTrailerToBinaryW(pszString, cchString,
     pbBinary, pcbBinary, pdwSkip);

    if (!ret && pdwFlags)
        *pdwFlags = CRYPT_STRING_BASE64HEADER;
    return ret;
}

static LONG Base64RequestHeaderToBinaryW(LPCWSTR pszString, DWORD cchString,
 BYTE *pbBinary, DWORD *pcbBinary, DWORD *pdwSkip, DWORD *pdwFlags)
{
    LONG ret = Base64WithHeaderAndTrailerToBinaryW(pszString, cchString,
     pbBinary, pcbBinary, pdwSkip);

    if (!ret && pdwFlags)
        *pdwFlags = CRYPT_STRING_BASE64REQUESTHEADER;
    return ret;
}

static LONG Base64X509HeaderToBinaryW(LPCWSTR pszString, DWORD cchString,
 BYTE *pbBinary, DWORD *pcbBinary, DWORD *pdwSkip, DWORD *pdwFlags)
{
    LONG ret = Base64WithHeaderAndTrailerToBinaryW(pszString, cchString,
     pbBinary, pcbBinary, pdwSkip);

    if (!ret && pdwFlags)
        *pdwFlags = CRYPT_STRING_BASE64X509CRLHEADER;
    return ret;
}

static LONG Base64AnyToBinaryW(LPCWSTR pszString, DWORD cchString,
 BYTE *pbBinary, DWORD *pcbBinary, DWORD *pdwSkip, DWORD *pdwFlags)
{
    LONG ret;

    ret = Base64HeaderToBinaryW(pszString, cchString, pbBinary, pcbBinary,
     pdwSkip, pdwFlags);
    if (ret == ERROR_INVALID_DATA)
        ret = Base64ToBinaryW(pszString, cchString, pbBinary, pcbBinary,
         pdwSkip, pdwFlags);
    return ret;
}

static LONG DecodeBinaryToBinaryW(LPCWSTR pszString, DWORD cchString,
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
            memcpy(pbBinary, pszString, cchString * sizeof(WCHAR));
        *pcbBinary = cchString * sizeof(WCHAR);
    }
    return ret;
}

static LONG DecodeAnyW(LPCWSTR pszString, DWORD cchString,
 BYTE *pbBinary, DWORD *pcbBinary, DWORD *pdwSkip, DWORD *pdwFlags)
{
    LONG ret;

    ret = Base64HeaderToBinaryW(pszString, cchString, pbBinary, pcbBinary,
     pdwSkip, pdwFlags);
    if (ret == ERROR_INVALID_DATA)
        ret = Base64ToBinaryW(pszString, cchString, pbBinary, pcbBinary,
         pdwSkip, pdwFlags);
    if (ret == ERROR_INVALID_DATA)
        ret = DecodeBinaryToBinaryW(pszString, cchString, pbBinary, pcbBinary,
         pdwSkip, pdwFlags);
    return ret;
}

BOOL WINAPI CryptStringToBinaryW(LPCWSTR pszString,
 DWORD cchString, DWORD dwFlags, BYTE *pbBinary, DWORD *pcbBinary,
 DWORD *pdwSkip, DWORD *pdwFlags)
{
    StringToBinaryWFunc decoder;
    LONG ret;

    TRACE("(%s, %d, %08x, %p, %p, %p, %p)\n", debugstr_w(pszString),
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
        decoder = Base64AnyToBinaryW;
        break;
    case CRYPT_STRING_BASE64:
        decoder = Base64ToBinaryW;
        break;
    case CRYPT_STRING_BASE64HEADER:
        decoder = Base64HeaderToBinaryW;
        break;
    case CRYPT_STRING_BASE64REQUESTHEADER:
        decoder = Base64RequestHeaderToBinaryW;
        break;
    case CRYPT_STRING_BASE64X509CRLHEADER:
        decoder = Base64X509HeaderToBinaryW;
        break;
    case CRYPT_STRING_BINARY:
        decoder = DecodeBinaryToBinaryW;
        break;
    case CRYPT_STRING_ANY:
        decoder = DecodeAnyW;
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
        cchString = strlenW(pszString);
    ret = decoder(pszString, cchString, pbBinary, pcbBinary, pdwSkip, pdwFlags);
    if (ret)
        SetLastError(ret);
    return ret == ERROR_SUCCESS;
}
