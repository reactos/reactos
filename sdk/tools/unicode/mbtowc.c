/*
 * MultiByteToWideChar implementation
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

extern unsigned int wine_decompose( int flags, WCHAR ch, WCHAR *dst, unsigned int dstlen ) DECLSPEC_HIDDEN;

/* check the code whether it is in Unicode Private Use Area (PUA). */
/* MB_ERR_INVALID_CHARS raises an error converting from 1-byte character to PUA. */
static inline int is_private_use_area_char(WCHAR code)
{
    return (code >= 0xe000 && code <= 0xf8ff);
}

/* check src string for invalid chars; return non-zero if invalid char found */
static inline int check_invalid_chars_sbcs( const struct sbcs_table *table, int flags,
                                            const unsigned char *src, unsigned int srclen )
{
    const WCHAR * const cp2uni = (flags & MB_USEGLYPHCHARS) ? table->cp2uni_glyphs : table->cp2uni;
    const WCHAR def_unicode_char = table->info.def_unicode_char;
    const unsigned char def_char = table->uni2cp_low[table->uni2cp_high[def_unicode_char >> 8]
                                                     + (def_unicode_char & 0xff)];
    while (srclen)
    {
        if ((cp2uni[*src] == def_unicode_char && *src != def_char) ||
            is_private_use_area_char(cp2uni[*src])) break;
        src++;
        srclen--;
    }
    return srclen;
}

/* mbstowcs for single-byte code page */
/* all lengths are in characters, not bytes */
static inline int mbstowcs_sbcs( const struct sbcs_table *table, int flags,
                                 const unsigned char *src, unsigned int srclen,
                                 WCHAR *dst, unsigned int dstlen )
{
    const WCHAR * const cp2uni = (flags & MB_USEGLYPHCHARS) ? table->cp2uni_glyphs : table->cp2uni;
    int ret = srclen;

    if (dstlen < srclen)
    {
        /* buffer too small: fill it up to dstlen and return error */
        srclen = dstlen;
        ret = -1;
    }

    while (srclen >= 16)
    {
        dst[0]  = cp2uni[src[0]];
        dst[1]  = cp2uni[src[1]];
        dst[2]  = cp2uni[src[2]];
        dst[3]  = cp2uni[src[3]];
        dst[4]  = cp2uni[src[4]];
        dst[5]  = cp2uni[src[5]];
        dst[6]  = cp2uni[src[6]];
        dst[7]  = cp2uni[src[7]];
        dst[8]  = cp2uni[src[8]];
        dst[9]  = cp2uni[src[9]];
        dst[10] = cp2uni[src[10]];
        dst[11] = cp2uni[src[11]];
        dst[12] = cp2uni[src[12]];
        dst[13] = cp2uni[src[13]];
        dst[14] = cp2uni[src[14]];
        dst[15] = cp2uni[src[15]];
        src += 16;
        dst += 16;
        srclen -= 16;
    }

    /* now handle the remaining characters */
    src += srclen;
    dst += srclen;
    switch (srclen)
    {
    case 15: dst[-15] = cp2uni[src[-15]];
    case 14: dst[-14] = cp2uni[src[-14]];
    case 13: dst[-13] = cp2uni[src[-13]];
    case 12: dst[-12] = cp2uni[src[-12]];
    case 11: dst[-11] = cp2uni[src[-11]];
    case 10: dst[-10] = cp2uni[src[-10]];
    case 9:  dst[-9]  = cp2uni[src[-9]];
    case 8:  dst[-8]  = cp2uni[src[-8]];
    case 7:  dst[-7]  = cp2uni[src[-7]];
    case 6:  dst[-6]  = cp2uni[src[-6]];
    case 5:  dst[-5]  = cp2uni[src[-5]];
    case 4:  dst[-4]  = cp2uni[src[-4]];
    case 3:  dst[-3]  = cp2uni[src[-3]];
    case 2:  dst[-2]  = cp2uni[src[-2]];
    case 1:  dst[-1]  = cp2uni[src[-1]];
    case 0: break;
    }
    return ret;
}

/* mbstowcs for single-byte code page with char decomposition */
static int mbstowcs_sbcs_decompose( const struct sbcs_table *table, int flags,
                                    const unsigned char *src, unsigned int srclen,
                                    WCHAR *dst, unsigned int dstlen )
{
    const WCHAR * const cp2uni = (flags & MB_USEGLYPHCHARS) ? table->cp2uni_glyphs : table->cp2uni;
    unsigned int len;

    if (!dstlen)  /* compute length */
    {
        WCHAR dummy[4]; /* no decomposition is larger than 4 chars */
        for (len = 0; srclen; srclen--, src++)
            len += wine_decompose( 0, cp2uni[*src], dummy, 4 );
        return len;
    }

    for (len = dstlen; srclen && len; srclen--, src++)
    {
        unsigned int res = wine_decompose( 0, cp2uni[*src], dst, len );
        if (!res) break;
        len -= res;
        dst += res;
    }
    if (srclen) return -1;  /* overflow */
    return dstlen - len;
}

/* query necessary dst length for src string */
static inline int get_length_dbcs( const struct dbcs_table *table,
                                   const unsigned char *src, unsigned int srclen )
{
    const unsigned char * const cp2uni_lb = table->cp2uni_leadbytes;
    int len;

    for (len = 0; srclen; srclen--, src++, len++)
    {
        if (cp2uni_lb[*src] && srclen > 1 && src[1])
        {
            src++;
            srclen--;
        }
    }
    return len;
}

/* check src string for invalid chars; return non-zero if invalid char found */
static inline int check_invalid_chars_dbcs( const struct dbcs_table *table,
                                            const unsigned char *src, unsigned int srclen )
{
    const WCHAR * const cp2uni = table->cp2uni;
    const unsigned char * const cp2uni_lb = table->cp2uni_leadbytes;
    const WCHAR def_unicode_char = table->info.def_unicode_char;
    const unsigned short def_char = table->uni2cp_low[table->uni2cp_high[def_unicode_char >> 8]
                                                      + (def_unicode_char & 0xff)];
    while (srclen)
    {
        unsigned char off = cp2uni_lb[*src];
        if (off)  /* multi-byte char */
        {
            if (srclen == 1) break;  /* partial char, error */
            if (cp2uni[(off << 8) + src[1]] == def_unicode_char &&
                ((src[0] << 8) | src[1]) != def_char) break;
            src++;
            srclen--;
        }
        else if ((cp2uni[*src] == def_unicode_char && *src != def_char) ||
                 is_private_use_area_char(cp2uni[*src])) break;
        src++;
        srclen--;
    }
    return srclen;
}

/* mbstowcs for double-byte code page */
/* all lengths are in characters, not bytes */
static inline int mbstowcs_dbcs( const struct dbcs_table *table,
                                 const unsigned char *src, unsigned int srclen,
                                 WCHAR *dst, unsigned int dstlen )
{
    const WCHAR * const cp2uni = table->cp2uni;
    const unsigned char * const cp2uni_lb = table->cp2uni_leadbytes;
    unsigned int len;

    if (!dstlen) return get_length_dbcs( table, src, srclen );

    for (len = dstlen; srclen && len; len--, srclen--, src++, dst++)
    {
        unsigned char off = cp2uni_lb[*src];
        if (off && srclen > 1 && src[1])
        {
            src++;
            srclen--;
            *dst = cp2uni[(off << 8) + *src];
        }
        else *dst = cp2uni[*src];
    }
    if (srclen) return -1;  /* overflow */
    return dstlen - len;
}


/* mbstowcs for double-byte code page with character decomposition */
static int mbstowcs_dbcs_decompose( const struct dbcs_table *table,
                                    const unsigned char *src, unsigned int srclen,
                                    WCHAR *dst, unsigned int dstlen )
{
    const WCHAR * const cp2uni = table->cp2uni;
    const unsigned char * const cp2uni_lb = table->cp2uni_leadbytes;
    unsigned int len, res;
    WCHAR ch;

    if (!dstlen)  /* compute length */
    {
        WCHAR dummy[4]; /* no decomposition is larger than 4 chars */
        for (len = 0; srclen; srclen--, src++)
        {
            unsigned char off = cp2uni_lb[*src];
            if (off && srclen > 1 && src[1])
            {
                src++;
                srclen--;
                ch = cp2uni[(off << 8) + *src];
            }
            else ch = cp2uni[*src];
            len += wine_decompose( 0, ch, dummy, 4 );
        }
        return len;
    }

    for (len = dstlen; srclen && len; srclen--, src++)
    {
        unsigned char off = cp2uni_lb[*src];
        if (off && srclen > 1 && src[1])
        {
            src++;
            srclen--;
            ch = cp2uni[(off << 8) + *src];
        }
        else ch = cp2uni[*src];
        if (!(res = wine_decompose( 0, ch, dst, len ))) break;
        dst += res;
        len -= res;
    }
    if (srclen) return -1;  /* overflow */
    return dstlen - len;
}


/* return -1 on dst buffer overflow, -2 on invalid input char */
int wine_cp_mbstowcs( const union cptable *table, int flags,
                      const char *s, int srclen,
                      WCHAR *dst, int dstlen )
{
    const unsigned char *src = (const unsigned char*) s;

    if (table->info.char_size == 1)
    {
        if (flags & MB_ERR_INVALID_CHARS)
        {
            if (check_invalid_chars_sbcs( &table->sbcs, flags, src, srclen )) return -2;
        }
        if (!(flags & MB_COMPOSITE))
        {
            if (!dstlen) return srclen;
            return mbstowcs_sbcs( &table->sbcs, flags, src, srclen, dst, dstlen );
        }
        return mbstowcs_sbcs_decompose( &table->sbcs, flags, src, srclen, dst, dstlen );
    }
    else /* mbcs */
    {
        if (flags & MB_ERR_INVALID_CHARS)
        {
            if (check_invalid_chars_dbcs( &table->dbcs, src, srclen )) return -2;
        }
        if (!(flags & MB_COMPOSITE))
            return mbstowcs_dbcs( &table->dbcs, src, srclen, dst, dstlen );
        else
            return mbstowcs_dbcs_decompose( &table->dbcs, src, srclen, dst, dstlen );
    }
}
