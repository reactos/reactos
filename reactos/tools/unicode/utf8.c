/*
 * UTF-8 support routines
 *
 * Copyright 2000 Alexandre Julliard
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

#include <string.h>

#include "wine/unicode.h"

/* number of following bytes in sequence based on first byte value (for bytes above 0x7f) */
static const char utf8_length[128] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x80-0x8f */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x90-0x9f */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xa0-0xaf */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xb0-0xbf */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 0xc0-0xcf */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 0xd0-0xdf */
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, /* 0xe0-0xef */
    3,3,3,3,3,3,3,3,4,4,4,4,5,5,0,0  /* 0xf0-0xff */
};

/* first byte mask depending on UTF-8 sequence length */
static const unsigned char utf8_mask[6] = { 0x7f, 0x1f, 0x0f, 0x07, 0x03, 0x01 };

/* minimum Unicode value depending on UTF-8 sequence length */
static const unsigned int utf8_minval[6] = { 0x0, 0x80, 0x800, 0x10000, 0x200000, 0x4000000 };


/* query necessary dst length for src string */
inline static int get_length_wcs_utf8( const WCHAR *src, unsigned int srclen )
{
    int len;
    for (len = 0; srclen; srclen--, src++, len++)
    {
        if (*src >= 0x80)
        {
            len++;
            if (*src >= 0x800) len++;
        }
    }
    return len;
}

/* wide char to UTF-8 string conversion */
/* return -1 on dst buffer overflow */
int wine_utf8_wcstombs( const WCHAR *src, int srclen, char *dst, int dstlen )
{
    int len;

    if (!dstlen) return get_length_wcs_utf8( src, srclen );

    for (len = dstlen; srclen; srclen--, src++)
    {
        WCHAR ch = *src;

        if (ch < 0x80)  /* 0x00-0x7f: 1 byte */
        {
            if (!len--) return -1;  /* overflow */
            *dst++ = ch;
            continue;
        }

        if (ch < 0x800)  /* 0x80-0x7ff: 2 bytes */
        {
            if ((len -= 2) < 0) return -1;  /* overflow */
            dst[1] = 0x80 | (ch & 0x3f);
            ch >>= 6;
            dst[0] = 0xc0 | ch;
            dst += 2;
            continue;
        }

        /* 0x800-0xffff: 3 bytes */

        if ((len -= 3) < 0) return -1;  /* overflow */
        dst[2] = 0x80 | (ch & 0x3f);
        ch >>= 6;
        dst[1] = 0x80 | (ch & 0x3f);
        ch >>= 6;
        dst[0] = 0xe0 | ch;
        dst += 3;
    }
    return dstlen - len;
}

/* query necessary dst length for src string */
inline static int get_length_mbs_utf8( const unsigned char *src, int srclen )
{
    int ret;
    const unsigned char *srcend = src + srclen;

    for (ret = 0; src < srcend; ret++)
    {
        unsigned char ch = *src++;
        if (ch < 0xc0) continue;

        switch(utf8_length[ch-0x80])
        {
        case 5:
            if (src >= srcend) return ret;  /* ignore partial char */
            if ((ch = *src ^ 0x80) >= 0x40) continue;
            src++;
        case 4:
            if (src >= srcend) return ret;  /* ignore partial char */
            if ((ch = *src ^ 0x80) >= 0x40) continue;
            src++;
        case 3:
            if (src >= srcend) return ret;  /* ignore partial char */
            if ((ch = *src ^ 0x80) >= 0x40) continue;
            src++;
        case 2:
            if (src >= srcend) return ret;  /* ignore partial char */
            if ((ch = *src ^ 0x80) >= 0x40) continue;
            src++;
        case 1:
            if (src >= srcend) return ret;  /* ignore partial char */
            if ((ch = *src ^ 0x80) >= 0x40) continue;
            src++;
        }
    }
    return ret;
}

/* UTF-8 to wide char string conversion */
/* return -1 on dst buffer overflow, -2 on invalid input char */
int wine_utf8_mbstowcs( int flags, const char *src, int srclen, WCHAR *dst, int dstlen )
{
    int len, count;
    unsigned int res;
    const char *srcend = src + srclen;

    if (!dstlen) return get_length_mbs_utf8( (const unsigned char*)src, srclen );

    for (count = dstlen; count && (src < srcend); count--, dst++)
    {
        unsigned char ch = *src++;
        if (ch < 0x80)  /* special fast case for 7-bit ASCII */
        {
            *dst = ch;
            continue;
        }
        len = utf8_length[ch-0x80];
        res = ch & utf8_mask[len];

        switch(len)
        {
        case 5:
            if (src >= srcend) goto done;  /* ignore partial char */
            if ((ch = *src ^ 0x80) >= 0x40) goto bad;
            res = (res << 6) | ch;
            src++;
        case 4:
            if (src >= srcend) goto done;  /* ignore partial char */
            if ((ch = *src ^ 0x80) >= 0x40) goto bad;
            res = (res << 6) | ch;
            src++;
        case 3:
            if (src >= srcend) goto done;  /* ignore partial char */
            if ((ch = *src ^ 0x80) >= 0x40) goto bad;
            res = (res << 6) | ch;
            src++;
        case 2:
            if (src >= srcend) goto done;  /* ignore partial char */
            if ((ch = *src ^ 0x80) >= 0x40) goto bad;
            res = (res << 6) | ch;
            src++;
        case 1:
            if (src >= srcend) goto done;  /* ignore partial char */
            if ((ch = *src ^ 0x80) >= 0x40) goto bad;
            res = (res << 6) | ch;
            src++;
            if (res < utf8_minval[len]) goto bad;
            if (res >= 0x10000) goto bad;  /* FIXME: maybe we should do surrogates here */
            *dst = res;
            continue;
        }
    bad:
        if (flags & MB_ERR_INVALID_CHARS) return -2;  /* bad char */
        *dst = (WCHAR)'?';
    }
    if (src < srcend) return -1;  /* overflow */
done:
    return dstlen - count;
}
