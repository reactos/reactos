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

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wincrypt.h"
#include "wine/debug.h"

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

static const WCHAR CERT_HEADER_W[] = L"-----BEGIN CERTIFICATE-----";
static const WCHAR CERT_HEADER_START_W[] = L"-----BEGIN ";
static const WCHAR CERT_DELIMITER_W[] = L"-----";
static const WCHAR CERT_TRAILER_W[] = L"-----END CERTIFICATE-----";
static const WCHAR CERT_TRAILER_START_W[] = L"-----END ";
static const WCHAR CERT_REQUEST_HEADER_W[] = L"-----BEGIN NEW CERTIFICATE REQUEST-----";
static const WCHAR CERT_REQUEST_TRAILER_W[] = L"-----END NEW CERTIFICATE REQUEST-----";
static const WCHAR X509_HEADER_W[] = L"-----BEGIN X509 CRL-----";
static const WCHAR X509_TRAILER_W[] = L"-----END X509 CRL-----";

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

    if (pszString)
    {
        if (*pcchString < cbBinary)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            ret = FALSE;
        }
        else if (cbBinary)
            memcpy(pszString, pbBinary, cbBinary);
    }
    else
        *pcchString = cbBinary;

    return ret;
}

static DWORD stradd(LPSTR ptr, LPCSTR end, LPCSTR s, DWORD slen)
{
    if (ptr + slen > end)
        slen = end - ptr;
    memcpy(ptr, s, slen);
    return slen;
}

static DWORD encodeBase64A(const BYTE *in_buf, int in_len, LPCSTR sep,
 char* out_buf, DWORD *out_len)
{
    int div, i;
    const BYTE *d = in_buf;
    int bytes = (in_len*8 + 5)/6, pad_bytes = (bytes % 4) ? 4 - (bytes % 4) : 0;
    LPSTR ptr;
    LPCSTR end;
    char chunk[4];

    if (!out_buf)
    {
        TRACE("bytes is %d, pad bytes is %d\n", bytes, pad_bytes);
        *out_len = bytes + pad_bytes;
        *out_len += (*out_len / 64 + (*out_len % 64 ? 1 : 0)) * strlen(sep) + 1;
        return 0;
    }

    /* Three bytes of input give 4 chars of output */
    div = in_len / 3;

    ptr = out_buf;
    end = ptr + *out_len;
    i = 0;
    while (div > 0 && ptr < end)
    {
        /* first char is the first 6 bits of the first byte*/
        chunk[0] = b64[ ( d[0] >> 2) & 0x3f ];
        /* second char is the last 2 bits of the first byte and the first 4
         * bits of the second byte */
        chunk[1] = b64[ ((d[0] << 4) & 0x30) | (d[1] >> 4 & 0x0f)];
        /* third char is the last 4 bits of the second byte and the first 2
         * bits of the third byte */
        chunk[2] = b64[ ((d[1] << 2) & 0x3c) | (d[2] >> 6 & 0x03)];
        /* fourth char is the remaining 6 bits of the third byte */
        chunk[3] = b64[   d[2]       & 0x3f];
        ptr += stradd(ptr, end, chunk, 4);
        i += 4;
        d += 3;
        div--;

        if (i && i % 64 == 0)
            ptr += stradd(ptr, end, sep, strlen(sep));
    }

    switch(pad_bytes)
    {
        case 1:
            /* first char is the first 6 bits of the first byte*/
            chunk[0] = b64[ ( d[0] >> 2) & 0x3f ];
            /* second char is the last 2 bits of the first byte and the first 4
             * bits of the second byte */
            chunk[1] = b64[ ((d[0] << 4) & 0x30) | (d[1] >> 4 & 0x0f)];
            /* third char is the last 4 bits of the second byte padded with
             * two zeroes */
            chunk[2] = b64[ ((d[1] << 2) & 0x3c) ];
            /* fourth char is a = to indicate one byte of padding */
            chunk[3] = '=';
            ptr += stradd(ptr, end, chunk, 4);
            break;
        case 2:
            /* first char is the first 6 bits of the first byte*/
            chunk[0] = b64[ ( d[0] >> 2) & 0x3f ];
            /* second char is the last 2 bits of the first byte padded with
             * four zeroes*/
            chunk[1] = b64[ ((d[0] << 4) & 0x30)];
            /* third char is = to indicate padding */
            chunk[2] = '=';
            /* fourth char is = to indicate padding */
            chunk[3] = '=';
            ptr += stradd(ptr, end, chunk, 4);
            break;
    }
    ptr += stradd(ptr, end, sep, strlen(sep));

    return ptr - out_buf;
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

    if (pszString)
    {
        LPSTR ptr = pszString;
        DWORD size = *pcchString;
        LPSTR end = ptr + size;

        if (header)
        {
            ptr += stradd(ptr, end, header, strlen(header));
            ptr += stradd(ptr, end, sep, strlen(sep));
            size = end - ptr;
        }
        ptr += encodeBase64A(pbBinary, cbBinary, sep, ptr, &size);
        if (trailer)
        {
            ptr += stradd(ptr, end, trailer, strlen(trailer));
            ptr += stradd(ptr, end, sep, strlen(sep));
        }

        if (ptr < end)
            *ptr = '\0';

        if (charsNeeded <= *pcchString)
        {
            *pcchString = charsNeeded - 1;
        }
        else
        {
            *pcchString = charsNeeded;
            SetLastError(ERROR_MORE_DATA);
            ret = FALSE;
        }
    }
    else
        *pcchString = charsNeeded;

    return ret;
}

static BOOL BinaryToHexRawA(const BYTE *bin, DWORD nbin, DWORD flags, char *str, DWORD *nstr)
{
    static const char hex[] = "0123456789abcdef";
    DWORD needed;

    if (flags & CRYPT_STRING_NOCRLF)
        needed = 0;
    else if (flags & CRYPT_STRING_NOCR)
        needed = 1;
    else
        needed = 2;

    needed += nbin * 2 + 1;

    if (!str)
    {
        *nstr = needed;
        return TRUE;
    }

    if (needed > *nstr && *nstr < 3)
    {
        SetLastError(ERROR_MORE_DATA);
        return FALSE;
    }

    nbin = min(nbin, (*nstr - 1) / 2);

    while (nbin--)
    {
        *str++ = hex[(*bin >> 4) & 0xf];
        *str++ = hex[*bin & 0xf];
        bin++;
    }

    if (needed > *nstr)
    {
        *str = 0;
        SetLastError(ERROR_MORE_DATA);
        return FALSE;
    }

    if (flags & CRYPT_STRING_NOCR)
    {
        *str++ = '\n';
    }
    else if (!(flags & CRYPT_STRING_NOCRLF))
    {
        *str++ = '\r';
        *str++ = '\n';
    }

    *str = 0;
    *nstr = needed - 1;
    return TRUE;
}

BOOL WINAPI CryptBinaryToStringA(const BYTE *pbBinary,
 DWORD cbBinary, DWORD dwFlags, LPSTR pszString, DWORD *pcchString)
{
    BinaryToStringAFunc encoder = NULL;

    TRACE("(%p, %ld, %08lx, %p, %p)\n", pbBinary, cbBinary, dwFlags, pszString,
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
    case CRYPT_STRING_HEXRAW:
        encoder = BinaryToHexRawA;
        break;
    case CRYPT_STRING_HEX:
    case CRYPT_STRING_HEXASCII:
    case CRYPT_STRING_HEXADDR:
    case CRYPT_STRING_HEXASCIIADDR:
        FIXME("Unimplemented type %ld\n", dwFlags & 0x0fffffff);
        /* fall through */
    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return encoder(pbBinary, cbBinary, dwFlags, pszString, pcchString);
}

static BOOL EncodeBinaryToBinaryW(const BYTE *in_buf, DWORD in_len, DWORD flags, WCHAR *out_buf, DWORD *out_len)
{
    BOOL ret = TRUE;

    if (out_buf)
    {
        if (*out_len < in_len)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            ret = FALSE;
        }
        else if (in_len)
            memcpy(out_buf, in_buf, in_len);
    }
    else
        *out_len = in_len;

    return ret;
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
    needed = bytes + pad_bytes;
    needed += (needed / 64 + (needed % 64 ? 1 : 0)) * lstrlenW(sep);
    needed++;

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

        if (i && i % 64 == 0)
        {
            lstrcpyW(ptr, sep);
            ptr += lstrlenW(sep);
        }
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
    lstrcpyW(ptr, sep);

    return ERROR_SUCCESS;
}

static BOOL BinaryToBase64W(const BYTE *pbBinary,
 DWORD cbBinary, DWORD dwFlags, LPWSTR pszString, DWORD *pcchString)
{
    BOOL ret = TRUE;
    LPCWSTR header = NULL, trailer = NULL, sep;
    DWORD charsNeeded;

    if (dwFlags & CRYPT_STRING_NOCR)
        sep = L"\n";
    else if (dwFlags & CRYPT_STRING_NOCRLF)
        sep = L"";
    else
        sep = L"\r\n";
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
        charsNeeded += lstrlenW(header) + lstrlenW(sep);
    if (trailer)
        charsNeeded += lstrlenW(trailer) + lstrlenW(sep);

    if (pszString)
    {
        if (charsNeeded <= *pcchString)
        {
            LPWSTR ptr = pszString;
            DWORD size = charsNeeded;

            if (header)
            {
                lstrcpyW(ptr, header);
                ptr += lstrlenW(ptr);
                lstrcpyW(ptr, sep);
                ptr += lstrlenW(sep);
            }
            encodeBase64W(pbBinary, cbBinary, sep, ptr, &size);
            ptr += size - 1;
            if (trailer)
            {
                lstrcpyW(ptr, trailer);
                ptr += lstrlenW(ptr);
                lstrcpyW(ptr, sep);
            }
            *pcchString = charsNeeded - 1;
        }
        else
        {
            *pcchString = charsNeeded;
            SetLastError(ERROR_MORE_DATA);
            ret = FALSE;
        }
    }
    else
        *pcchString = charsNeeded;

    return ret;
}

static BOOL BinaryToHexRawW(const BYTE *bin, DWORD nbin, DWORD flags, LPWSTR str, DWORD *nstr)
{
    static const WCHAR hex[] = L"0123456789abcdef";
    DWORD needed;

    if (flags & CRYPT_STRING_NOCRLF)
        needed = 0;
    else if (flags & CRYPT_STRING_NOCR)
        needed = 1;
    else
        needed = 2;

    needed += nbin * 2 + 1;

    if (!str)
    {
        *nstr = needed;
        return TRUE;
    }

    if (needed > *nstr)
    {
        SetLastError(ERROR_MORE_DATA);
        return FALSE;
    }

    while (nbin--)
    {
        *str++ = hex[(*bin >> 4) & 0xf];
        *str++ = hex[*bin & 0xf];
        bin++;
    }

    if (flags & CRYPT_STRING_NOCR)
        *str++ = '\n';
    else if (!(flags & CRYPT_STRING_NOCRLF))
    {
        *str++ = '\r';
        *str++ = '\n';
    }

    *str = 0;
    *nstr = needed - 1;
    return TRUE;
}

static BOOL binary_to_hexW(const BYTE *bin, DWORD nbin, DWORD flags, LPWSTR str, DWORD *nstr)
{
    static const WCHAR hex[] = L"0123456789abcdef";
    DWORD needed, i;

    needed = nbin * 3; /* spaces + terminating \0 */

    if (flags & CRYPT_STRING_NOCR)
    {
        needed += (nbin + 7) / 16; /* space every 16 characters */
        needed += 1; /* terminating \n */
    }
    else if (!(flags & CRYPT_STRING_NOCRLF))
    {
        needed += (nbin + 7) / 16; /* space every 16 characters */
        needed += nbin / 16 + 1; /* LF every 16 characters + terminating \r */

        if (nbin % 16)
            needed += 1; /* terminating \n */
    }

    if (!str)
    {
        *nstr = needed;
        return TRUE;
    }

    if (needed > *nstr)
    {
        SetLastError(ERROR_MORE_DATA);
        return FALSE;
    }

    for (i = 0; i < nbin; i++)
    {
        *str++ = hex[(bin[i] >> 4) & 0xf];
        *str++ = hex[bin[i] & 0xf];

        if (i >= nbin - 1) break;

        if (i && !(flags & CRYPT_STRING_NOCRLF))
        {
            if (!((i + 1) % 16))
            {
                if (flags & CRYPT_STRING_NOCR)
                    *str++ = '\n';
                else
                {
                    *str++ = '\r';
                    *str++ = '\n';
                }
                continue;
            }
            else if (!((i + 1) % 8))
                *str++ = ' ';
        }

        *str++ = ' ';
    }

    if (flags & CRYPT_STRING_NOCR)
        *str++ = '\n';
    else if (!(flags & CRYPT_STRING_NOCRLF))
    {
        *str++ = '\r';
        *str++ = '\n';
    }

    *str = 0;
    *nstr = needed - 1;
    return TRUE;
}

BOOL WINAPI CryptBinaryToStringW(const BYTE *pbBinary,
 DWORD cbBinary, DWORD dwFlags, LPWSTR pszString, DWORD *pcchString)
{
    BinaryToStringWFunc encoder = NULL;

    TRACE("(%p, %ld, %08lx, %p, %p)\n", pbBinary, cbBinary, dwFlags, pszString,
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
        encoder = EncodeBinaryToBinaryW;
        break;
    case CRYPT_STRING_BASE64:
    case CRYPT_STRING_BASE64HEADER:
    case CRYPT_STRING_BASE64REQUESTHEADER:
    case CRYPT_STRING_BASE64X509CRLHEADER:
        encoder = BinaryToBase64W;
        break;
    case CRYPT_STRING_HEXRAW:
        encoder = BinaryToHexRawW;
        break;
    case CRYPT_STRING_HEX:
        encoder = binary_to_hexW;
        break;
    case CRYPT_STRING_HEXASCII:
    case CRYPT_STRING_HEXADDR:
    case CRYPT_STRING_HEXASCIIADDR:
        FIXME("Unimplemented type %ld\n", dwFlags & 0x0fffffff);
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
        TRACE("Can't find %s in %s.\n", header, debugstr_an(pszString, cchString));
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

static LONG string_to_hexA(const char *str, DWORD len, BYTE *hex, DWORD *hex_len, DWORD *skipped, DWORD *ret_flags)
{
    return string_to_hex(str, FALSE, len, hex, hex_len, skipped, ret_flags);
}

BOOL WINAPI CryptStringToBinaryA(LPCSTR pszString,
 DWORD cchString, DWORD dwFlags, BYTE *pbBinary, DWORD *pcbBinary,
 DWORD *pdwSkip, DWORD *pdwFlags)
{
    StringToBinaryAFunc decoder;
    LONG ret;

    TRACE("(%s, %ld, %08lx, %p, %p, %p, %p)\n", debugstr_an(pszString, cchString ? cchString : -1),
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
        decoder = string_to_hexA;
        break;
    case CRYPT_STRING_HEXASCII:
    case CRYPT_STRING_HEXADDR:
    case CRYPT_STRING_HEXASCIIADDR:
        FIXME("Unimplemented type %ld\n", dwFlags & 0x7fffffff);
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

    if ((lstrlenW(header) + lstrlenW(trailer)) > cchString)
    {
        return ERROR_INVALID_DATA;
    }

    if (!(headerBegins = wcsstr(pszString, header)))
    {
        TRACE("Can't find %s in %s.\n", debugstr_w(header), debugstr_wn(pszString, cchString));
        return ERROR_INVALID_DATA;
    }

    dataBegins = headerBegins + lstrlenW(header);
    if (!(dataBegins = wcsstr(dataBegins, CERT_DELIMITER_W)))
    {
        return ERROR_INVALID_DATA;
    }
    dataBegins += lstrlenW(CERT_DELIMITER_W);
    if (*dataBegins == '\r') dataBegins++;
    if (*dataBegins == '\n') dataBegins++;

    if (!(trailerBegins = wcsstr(dataBegins, trailer)))
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

static LONG string_to_hexW(const WCHAR *str, DWORD len, BYTE *hex, DWORD *hex_len, DWORD *skipped, DWORD *ret_flags)
{
    return string_to_hex(str, TRUE, len, hex, hex_len, skipped, ret_flags);
}

BOOL WINAPI CryptStringToBinaryW(LPCWSTR pszString,
 DWORD cchString, DWORD dwFlags, BYTE *pbBinary, DWORD *pcbBinary,
 DWORD *pdwSkip, DWORD *pdwFlags)
{
    StringToBinaryWFunc decoder;
    LONG ret;

    TRACE("(%s, %ld, %08lx, %p, %p, %p, %p)\n", debugstr_wn(pszString, cchString ? cchString : -1),
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
        decoder = string_to_hexW;
        break;
    case CRYPT_STRING_HEXASCII:
    case CRYPT_STRING_HEXADDR:
    case CRYPT_STRING_HEXASCIIADDR:
        FIXME("Unimplemented type %ld\n", dwFlags & 0x7fffffff);
        /* fall through */
    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!cchString)
        cchString = lstrlenW(pszString);
    ret = decoder(pszString, cchString, pbBinary, pcbBinary, pdwSkip, pdwFlags);
    if (ret)
        SetLastError(ret);
    return ret == ERROR_SUCCESS;
}
