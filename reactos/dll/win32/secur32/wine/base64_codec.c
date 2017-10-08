/*
 * base64 encoder/decoder
 *
 * Copyright 2005 by Kai Blin
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

#include "precomp.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

static const char b64[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

SECURITY_STATUS encodeBase64(PBYTE in_buf, int in_len, char* out_buf,
        int max_len, int *out_len)
{
    int div, i;
    PBYTE d = in_buf;
    int bytes = (in_len*8 + 5)/6, pad_bytes = (bytes % 4) ? 4 - (bytes % 4) : 0;

    TRACE("bytes is %d, pad bytes is %d\n", bytes, pad_bytes);
    *out_len = bytes + pad_bytes;

    if(bytes + pad_bytes + 1 > max_len)
        return SEC_E_BUFFER_TOO_SMALL;

    /* Three bytes of input give 4 chars of output */
    div = in_len / 3;

    i = 0;
    while(div > 0)
    {
        /* first char is the first 6 bits of the first byte*/
        out_buf[i + 0] = b64[ ( d[0] >> 2) & 0x3f ];
        /* second char is the last 2 bits of the first byte and the first 4
         * bits of the second byte */
        out_buf[i + 1] = b64[ ((d[0] << 4) & 0x30) | (d[1] >> 4 & 0x0f)];
        /* third char is the last 4 bits of the second byte and the first 2
         * bits of the third byte */
        out_buf[i + 2] = b64[ ((d[1] << 2) & 0x3c) | (d[2] >> 6 & 0x03)];
        /* fourth char is the remaining 6 bits of the third byte */
        out_buf[i + 3] = b64[   d[2]       & 0x3f];
        i += 4;
        d += 3;
        div--;
    }

    switch(pad_bytes)
    {
        case 1:
            /* first char is the first 6 bits of the first byte*/
            out_buf[i + 0] = b64[ ( d[0] >> 2) & 0x3f ];
            /* second char is the last 2 bits of the first byte and the first 4
             * bits of the second byte */
            out_buf[i + 1] = b64[ ((d[0] << 4) & 0x30) | (d[1] >> 4 & 0x0f)];
            /* third char is the last 4 bits of the second byte padded with
             * two zeroes */
            out_buf[i + 2] = b64[ ((d[1] << 2) & 0x3c) ];
            /* fourth char is a = to indicate one byte of padding */
            out_buf[i + 3] = '=';
            out_buf[i + 4] = 0;
            break;
        case 2:
            /* first char is the first 6 bits of the first byte*/
            out_buf[i + 0] = b64[ ( d[0] >> 2) & 0x3f ];
            /* second char is the last 2 bits of the first byte padded with
             * four zeroes*/
            out_buf[i + 1] = b64[ ((d[0] << 4) & 0x30)];
            /* third char is = to indicate padding */
            out_buf[i + 2] = '=';
            /* fourth char is = to indicate padding */
            out_buf[i + 3] = '=';
            out_buf[i + 4] = 0;
            break;
        default:
            out_buf[i] = 0;
    }

    return SEC_E_OK;
}

static inline BYTE decode(char c)
{
    if( c >= 'A' && c <= 'Z')
        return c - 'A';
    if( c >= 'a' && c <= 'z')
        return c - 'a' + 26;
    if( c >= '0' && c <= '9')
        return c - '0' + 52;
    if( c == '+')
        return 62;
    if( c == '/')
        return 63;
    else
        return 64;
}

SECURITY_STATUS decodeBase64(char *in_buf, int in_len, PBYTE out_buf,
        int max_len, int *out_len)
{
    int len = in_len, i;
    char *d = in_buf;
    int  ip0, ip1, ip2, ip3;

    TRACE("in_len: %d\n", in_len);

    if((in_len % 4) != 0)
        return SEC_E_INVALID_TOKEN;

    if(in_len > max_len)
        return SEC_E_BUFFER_TOO_SMALL;

    i = 0;
    while(len > 4)
    {
        if((ip0 = decode(d[0])) > 63)
            return SEC_E_INVALID_TOKEN;
        if((ip1 = decode(d[1])) > 63)
            return SEC_E_INVALID_TOKEN;
        if((ip2 = decode(d[2])) > 63)
            return SEC_E_INVALID_TOKEN;
        if((ip3 = decode(d[3])) > 63)
            return SEC_E_INVALID_TOKEN;

        out_buf[i + 0] = (ip0 << 2) | (ip1 >> 4);
        out_buf[i + 1] = (ip1 << 4) | (ip2 >> 2);
        out_buf[i + 2] = (ip2 << 6) |  ip3;
        len -= 4;
        i += 3;
        d += 4;
    }

    if(d[2] == '=')
    {
        if((ip0 = decode(d[0])) > 63)
            return SEC_E_INVALID_TOKEN;
        if((ip1 = decode(d[1])) > 63)
            return SEC_E_INVALID_TOKEN;

        out_buf[i] = (ip0 << 2) | (ip1 >> 4);
        i++;
    }
    else if(d[3] == '=')
    {
        if((ip0 = decode(d[0])) > 63)
            return SEC_E_INVALID_TOKEN;
        if((ip1 = decode(d[1])) > 63)
            return SEC_E_INVALID_TOKEN;
        if((ip2 = decode(d[2])) > 63)
            return SEC_E_INVALID_TOKEN;

        out_buf[i + 0] = (ip0 << 2) | (ip1 >> 4);
        out_buf[i + 1] = (ip1 << 4) | (ip2 >> 2);
        i += 2;
    }
    else
    {
        if((ip0 = decode(d[0])) > 63)
            return SEC_E_INVALID_TOKEN;
        if((ip1 = decode(d[1])) > 63)
            return SEC_E_INVALID_TOKEN;
        if((ip2 = decode(d[2])) > 63)
            return SEC_E_INVALID_TOKEN;
        if((ip3 = decode(d[3])) > 63)
            return SEC_E_INVALID_TOKEN;


        out_buf[i + 0] = (ip0 << 2) | (ip1 >> 4);
        out_buf[i + 1] = (ip1 << 4) | (ip2 >> 2);
        out_buf[i + 2] = (ip2 << 6) |  ip3;
        i += 3;
    }
    *out_len = i;
    return SEC_E_OK;
}
