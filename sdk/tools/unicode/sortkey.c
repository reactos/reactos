/*
 * Unicode sort key generation
 *
 * Copyright 2003 Dmitry Timoshkov
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

extern unsigned int wine_decompose( int flags, WCHAR ch, WCHAR *dst, unsigned int dstlen );
extern const unsigned int collation_table[];

/*
 * flags - normalization NORM_* flags
 *
 * FIXME: 'variable' flag not handled
 */
int wine_get_sortkey(int flags, const WCHAR *src, int srclen, char *dst, int dstlen)
{
    WCHAR dummy[4]; /* no decomposition is larger than 4 chars */
    int key_len[4];
    char *key_ptr[4];
    const WCHAR *src_save = src;
    int srclen_save = srclen;

    key_len[0] = key_len[1] = key_len[2] = key_len[3] = 0;
    for (; srclen; srclen--, src++)
    {
        unsigned int i, decomposed_len = 1;/*wine_decompose(*src, dummy, 4);*/
        dummy[0] = *src;
        if (decomposed_len)
        {
            for (i = 0; i < decomposed_len; i++)
            {
                WCHAR wch = dummy[i];
                unsigned int ce;

                /* tests show that win2k just ignores NORM_IGNORENONSPACE,
                 * and skips white space and punctuation characters for
                 * NORM_IGNORESYMBOLS.
                 */
                if ((flags & NORM_IGNORESYMBOLS) && (get_char_typeW(wch) & (C1_PUNCT | C1_SPACE)))
                    continue;

                if (flags & NORM_IGNORECASE) wch = tolowerW(wch);

                ce = collation_table[collation_table[wch >> 8] + (wch & 0xff)];
                if (ce != (unsigned int)-1)
                {
                    if (ce >> 16) key_len[0] += 2;
                    if ((ce >> 8) & 0xff) key_len[1]++;
                    if ((ce >> 4) & 0x0f) key_len[2]++;
                    if (ce & 1)
                    {
                        if (wch >> 8) key_len[3]++;
                        key_len[3]++;
                    }
                }
                else
                {
                    key_len[0] += 2;
                    if (wch >> 8) key_len[0]++;
                    if (wch & 0xff) key_len[0]++;
		}
            }
        }
    }

    if (!dstlen) /* compute length */
        /* 4 * '\1' + key length */
        return key_len[0] + key_len[1] + key_len[2] + key_len[3] + 4;

    if (dstlen < key_len[0] + key_len[1] + key_len[2] + key_len[3] + 4 + 1)
        return 0; /* overflow */

    src = src_save;
    srclen = srclen_save;

    key_ptr[0] = dst;
    key_ptr[1] = key_ptr[0] + key_len[0] + 1;
    key_ptr[2] = key_ptr[1] + key_len[1] + 1;
    key_ptr[3] = key_ptr[2] + key_len[2] + 1;

    for (; srclen; srclen--, src++)
    {
        unsigned int i, decomposed_len = 1;/*wine_decompose(*src, dummy, 4);*/
        dummy[0] = *src;
        if (decomposed_len)
        {
            for (i = 0; i < decomposed_len; i++)
            {
                WCHAR wch = dummy[i];
                unsigned int ce;

                /* tests show that win2k just ignores NORM_IGNORENONSPACE,
                 * and skips white space and punctuation characters for
                 * NORM_IGNORESYMBOLS.
                 */
                if ((flags & NORM_IGNORESYMBOLS) && (get_char_typeW(wch) & (C1_PUNCT | C1_SPACE)))
                    continue;

                if (flags & NORM_IGNORECASE) wch = tolowerW(wch);

                ce = collation_table[collation_table[wch >> 8] + (wch & 0xff)];
                if (ce != (unsigned int)-1)
                {
                    WCHAR key;
                    if ((key = ce >> 16))
                    {
                        *key_ptr[0]++ = key >> 8;
                        *key_ptr[0]++ = key & 0xff;
                    }
                    /* make key 1 start from 2 */
                    if ((key = (ce >> 8) & 0xff)) *key_ptr[1]++ = key + 1;
                    /* make key 2 start from 2 */
                    if ((key = (ce >> 4) & 0x0f)) *key_ptr[2]++ = key + 1;
                    /* key 3 is always a character code */
                    if (ce & 1)
                    {
                        if (wch >> 8) *key_ptr[3]++ = wch >> 8;
                        if (wch & 0xff) *key_ptr[3]++ = wch & 0xff;
                    }
                }
                else
                {
                    *key_ptr[0]++ = 0xff;
                    *key_ptr[0]++ = 0xfe;
                    if (wch >> 8) *key_ptr[0]++ = wch >> 8;
                    if (wch & 0xff) *key_ptr[0]++ = wch & 0xff;
                }
            }
        }
    }

    *key_ptr[0] = '\1';
    *key_ptr[1] = '\1';
    *key_ptr[2] = '\1';
    *key_ptr[3]++ = '\1';
    *key_ptr[3] = 0;

    return key_ptr[3] - dst;
}

enum weight
{
    UNICODE_WEIGHT,
    DIACRITIC_WEIGHT,
    CASE_WEIGHT
};

static unsigned int get_weight(WCHAR ch, enum weight type)
{
    unsigned int ret;

    ret = collation_table[collation_table[ch >> 8] + (ch & 0xff)];
    if (ret == (unsigned int)-1)
        return ch;

    switch(type)
    {
    case UNICODE_WEIGHT:
        return ret >> 16;
    case DIACRITIC_WEIGHT:
        return (ret >> 8) & 0xff;
    case CASE_WEIGHT:
    default:
        return (ret >> 4) & 0x0f;
    }
}

static void inc_str_pos(const WCHAR **str, int *len, int *dpos, int *dlen)
{
    (*dpos)++;
    if (*dpos == *dlen)
    {
        *dpos = *dlen = 0;
        (*str)++;
        (*len)--;
    }
}

static inline int compare_weights(int flags, const WCHAR *str1, int len1,
                                  const WCHAR *str2, int len2, enum weight type)
{
    int dpos1 = 0, dpos2 = 0, dlen1 = 0, dlen2 = 0;
    WCHAR dstr1[4], dstr2[4];
    unsigned int ce1, ce2;

    /* 32-bit collation element table format:
     * unicode weight - high 16 bit, diacritic weight - high 8 bit of low 16 bit,
     * case weight - high 4 bit of low 8 bit.
     */
    while (len1 > 0 && len2 > 0)
    {
        if (!dlen1) dlen1 = wine_decompose(0, *str1, dstr1, 4);
        if (!dlen2) dlen2 = wine_decompose(0, *str2, dstr2, 4);

        if (flags & NORM_IGNORESYMBOLS)
        {
            int skip = 0;
            /* FIXME: not tested */
            if (get_char_typeW(dstr1[dpos1]) & (C1_PUNCT | C1_SPACE))
            {
                inc_str_pos(&str1, &len1, &dpos1, &dlen1);
                skip = 1;
            }
            if (get_char_typeW(dstr2[dpos2]) & (C1_PUNCT | C1_SPACE))
            {
                inc_str_pos(&str2, &len2, &dpos2, &dlen2);
                skip = 1;
            }
            if (skip) continue;
        }

       /* hyphen and apostrophe are treated differently depending on
        * whether SORT_STRINGSORT specified or not
        */
        if (type == UNICODE_WEIGHT && !(flags & SORT_STRINGSORT))
        {
            if (dstr1[dpos1] == '-' || dstr1[dpos1] == '\'')
            {
                if (dstr2[dpos2] != '-' && dstr2[dpos2] != '\'')
                {
                    inc_str_pos(&str1, &len1, &dpos1, &dlen1);
                    continue;
                }
            }
            else if (dstr2[dpos2] == '-' || dstr2[dpos2] == '\'')
            {
                inc_str_pos(&str2, &len2, &dpos2, &dlen2);
                continue;
            }
        }

        ce1 = get_weight(dstr1[dpos1], type);
        if (!ce1)
        {
            inc_str_pos(&str1, &len1, &dpos1, &dlen1);
            continue;
        }
        ce2 = get_weight(dstr2[dpos2], type);
        if (!ce2)
        {
            inc_str_pos(&str2, &len2, &dpos2, &dlen2);
            continue;
        }

        if (ce1 - ce2) return ce1 - ce2;

        inc_str_pos(&str1, &len1, &dpos1, &dlen1);
        inc_str_pos(&str2, &len2, &dpos2, &dlen2);
    }
    while (len1)
    {
        if (!dlen1) dlen1 = wine_decompose(0, *str1, dstr1, 4);

        ce1 = get_weight(dstr1[dpos1], type);
        if (ce1) break;
        inc_str_pos(&str1, &len1, &dpos1, &dlen1);
    }
    while (len2)
    {
        if (!dlen2) dlen2 = wine_decompose(0, *str2, dstr2, 4);

        ce2 = get_weight(dstr2[dpos2], type);
        if (ce2) break;
        inc_str_pos(&str2, &len2, &dpos2, &dlen2);
    }
    return len1 - len2;
}

int wine_compare_string(int flags, const WCHAR *str1, int len1,
                        const WCHAR *str2, int len2)
{
    int ret;

    ret = compare_weights(flags, str1, len1, str2, len2, UNICODE_WEIGHT);
    if (!ret)
    {
        if (!(flags & NORM_IGNORENONSPACE))
            ret = compare_weights(flags, str1, len1, str2, len2, DIACRITIC_WEIGHT);
        if (!ret && !(flags & NORM_IGNORECASE))
            ret = compare_weights(flags, str1, len1, str2, len2, CASE_WEIGHT);
    }
    return ret;
}
