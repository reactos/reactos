/*
 * String folding
 *
 * Copyright 2003 Jon Griffiths
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

#include "wine/unicode.h"

static inline WCHAR to_unicode_digit( WCHAR ch )
{
    extern const WCHAR wine_digitmap[];
    return ch + wine_digitmap[wine_digitmap[ch >> 8] + (ch & 0xff)];
}

static inline WCHAR to_unicode_native( WCHAR ch )
{
    extern const WCHAR wine_compatmap[];
    return ch + wine_compatmap[wine_compatmap[ch >> 8] + (ch & 0xff)];
}

static const WCHAR wine_ligatures[] =
{
    0x00c6, 0x00de, 0x00df, 0x00e6, 0x00fe, 0x0132, 0x0133, 0x0152,
    0x0153, 0x01c4, 0x01c5, 0x01c6, 0x01c7, 0x01c8, 0x01c9, 0x01ca,
    0x01cb, 0x01cc, 0x01e2, 0x01e3, 0x01f1, 0x01f2, 0x01f3, 0x01fc,
    0x01fd, 0x05f0, 0x05f1, 0x05f2, 0xfb00, 0xfb01, 0xfb02, 0xfb03,
    0xfb04, 0xfb05, 0xfb06
};

/* Unicode expanded ligatures */
static const WCHAR wine_expanded_ligatures[][4] =
{
    { 'A','E','\0',1 },
    { 'T','H','\0',1 },
    { 's','s','\0',1 },
    { 'a','e','\0',1 },
    { 't','h','\0',1 },
    { 'I','J','\0',1 },
    { 'i','j','\0',1 },
    { 'O','E','\0',1 },
    { 'o','e','\0',1 },
    { 'D',0x017d,'\0',1 },
    { 'D',0x017e,'\0',1 },
    { 'd',0x017e,'\0',1 },
    { 'L','J','\0',1 },
    { 'L','j','\0',1 },
    { 'l','j','\0',1 },
    { 'N','J','\0',1 },
    { 'N','j','\0',1 },
    { 'n','j','\0',1 },
    { 0x0100,0x0112,'\0',1 },
    { 0x0101,0x0113,'\0',1 },
    { 'D','Z','\0',1 },
    { 'D','z','\0',1 },
    { 'd','z','\0',1 },
    { 0x00c1,0x00c9,'\0',1 },
    { 0x00e1,0x00e9,'\0',1 },
    { 0x05d5,0x05d5,'\0',1 },
    { 0x05d5,0x05d9,'\0',1 },
    { 0x05d9,0x05d9,'\0',1 },
    { 'f','f','\0',1 },
    { 'f','i','\0',1 },
    { 'f','l','\0',1 },
    { 'f','f','i',2 },
    { 'f','f','l',2 },
    { 0x017f,'t','\0',1 },
    { 's','t','\0',1 }
};

static inline int get_ligature_len( WCHAR wc )
{
    int low = 0, high = sizeof(wine_ligatures)/sizeof(WCHAR) -1;
    while (low <= high)
    {
        int pos = (low + high) / 2;
        if (wine_ligatures[pos] < wc)
            low = pos + 1;
        else if (wine_ligatures[pos] > wc)
            high = pos - 1;
        else
            return wine_expanded_ligatures[pos][3];
    }
    return 0;
}

static inline const WCHAR* get_ligature( WCHAR wc )
{
    static const WCHAR empty_ligature[] = { '\0','\0','\0', 0 };
    int low = 0, high = sizeof(wine_ligatures)/sizeof(WCHAR) -1;
    while (low <= high)
    {
        int pos = (low + high) / 2;
        if (wine_ligatures[pos] < wc)
            low = pos + 1;
        else if (wine_ligatures[pos] > wc)
            high = pos - 1;
        else
            return wine_expanded_ligatures[pos];
    }
    return empty_ligature;
}

/* fold a unicode string */
int wine_fold_string( int flags, const WCHAR *src, int srclen, WCHAR *dst, int dstlen )
{
    WCHAR *dstbase = dst;
    const WCHAR *expand;
    int i;

    if (srclen == -1)
        srclen = strlenW(src) + 1; /* Include terminating NUL in count */

    if (!dstlen)
    {
        /* Calculate the required size for dst */
        dstlen = srclen;

        if (flags & MAP_EXPAND_LIGATURES)
        {
            while (srclen--)
            {
                dstlen += get_ligature_len(*src);
                src++;
            }
        }
        else if (flags & MAP_COMPOSITE)
        {
            /* FIXME */
        }
        else if (flags & MAP_PRECOMPOSED)
        {
            /* FIXME */
        }
        return dstlen;
    }

    if (srclen > dstlen)
        return 0;

    dstlen -= srclen;

    /* Actually perform the mapping(s) specified */
    for (i = 0; i < srclen; i++)
    {
        WCHAR ch = *src;

        if (flags & MAP_EXPAND_LIGATURES)
        {
            expand = get_ligature(ch);
            if (expand[0])
            {
                if (!dstlen--)
                    return 0;
                dst[0] = expand[0];
                if (expand[2])
                {
                    if (!dstlen--)
                        return 0;
                    *++dst = expand[1];
                    ch = expand[2];
                }
                else
                    ch = expand[1];
                dst++;
            }
        }
        else if (flags & MAP_COMPOSITE)
        {
            /* FIXME */
        }
        else if (flags & MAP_PRECOMPOSED)
        {
            /* FIXME */
        }
        if (flags & MAP_FOLDDIGITS)
            ch = to_unicode_digit(ch);
        if (flags & MAP_FOLDCZONE)
            ch = to_unicode_native(ch);

        *dst++ = ch;
        src++;
    }
    return dst - dstbase;
}
