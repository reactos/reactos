/*
 * Copyright 2011 Piotr Caban for CodeWeavers
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

#include "bnum.h"

#ifdef PRINTF_WIDE
#define APICHAR wchar_t
#define CONVCHAR char
#define FUNC_NAME(func) func ## _w
#else
#define APICHAR char
#define CONVCHAR wchar_t
#define FUNC_NAME(func) func ## _a
#endif

struct FUNC_NAME(_str_ctx) {
    size_t len;
    APICHAR *buf;
};

static int FUNC_NAME(puts_clbk_str)(void *ctx, int len, const APICHAR *str)
{
    struct FUNC_NAME(_str_ctx) *out = ctx;

    if(!out->buf)
        return len;

    if(out->len < len) {
        memmove(out->buf, str, out->len*sizeof(APICHAR));
        out->buf += out->len;
        out->len = 0;
        return -1;
    }

    memmove(out->buf, str, len*sizeof(APICHAR));
    out->buf += len;
    out->len -= len;
    return len;
}

static inline const APICHAR* FUNC_NAME(pf_parse_int)(const APICHAR *fmt, int *val)
{
    *val = 0;

    while (*fmt >= '0' && *fmt <= '9') {
        *val *= 10;
        *val += *fmt++ - '0';
    }

    return fmt;
}

/* pf_fill: takes care of signs, alignment, zero and field padding */
static inline int FUNC_NAME(pf_fill)(FUNC_NAME(puts_clbk) pf_puts, void *puts_ctx,
        int len, pf_flags *flags, BOOL left)
{
    int i, r = 0, written;

    if(flags->Sign && !strchr("diaAeEfFgG", flags->Format))
        flags->Sign = 0;

    if(left && flags->Sign) {
        APICHAR ch = flags->Sign;
        flags->FieldLength--;
        if(flags->PadZero)
            r = pf_puts(puts_ctx, 1, &ch);
    }
    written = r;

    if((!left && flags->LeftAlign) || (left && !flags->LeftAlign)) {
        APICHAR ch;

        if(left && flags->PadZero)
            ch = '0';
        else
            ch = ' ';

        for(i=0; i<flags->FieldLength-len && r>=0; i++) {
            r = pf_puts(puts_ctx, 1, &ch);
            written += r;
        }
    }


    if(r>=0 && left && flags->Sign && !flags->PadZero) {
        APICHAR ch = flags->Sign;
        r = pf_puts(puts_ctx, 1, &ch);
        written += r;
    }

    return r>=0 ? written : r;
}

#ifndef PRINTF_HELPERS
#define PRINTF_HELPERS
static inline int wcstombs_len(char *mbstr, const wchar_t *wcstr,
        int len, _locale_t locale)
{
    char buf[MB_LEN_MAX];
    int i, r, mblen = 0;

    for(i=0; i<len; i++) {
        r = _wctomb_l(mbstr ? mbstr+mblen : buf, wcstr[i], locale);
        if(r < 0) return r;
        mblen += r;
    }
    return mblen;
}

static inline int mbstowcs_len(wchar_t *wcstr, const char *mbstr,
        int len, _locale_t locale)
{
    int i, r, wlen = 0;
    WCHAR buf;

    for(i=0; i<len; wlen++) {
        r = _mbtowc_l(wcstr ? wcstr+wlen : &buf, mbstr+i, len-i, locale);
        if(r < 0) return r;
        i += r ? r : 1;
    }
    return wlen;
}

static inline unsigned int log2i(unsigned int x)
{
    ULONG result;
    _BitScanReverse(&result, x);
    return result;
}

static inline unsigned int log10i(unsigned int x)
{
    unsigned int t = ((log2i(x) + 1) * 1233) / 4096;
    return t - (x < p10s[t]);
}

#endif

static inline int FUNC_NAME(pf_output_wstr)(FUNC_NAME(puts_clbk) pf_puts, void *puts_ctx,
        const wchar_t *str, int len, _locale_t locale)
{
#ifdef PRINTF_WIDE
    return pf_puts(puts_ctx, len, str);
#else
    LPSTR out;
    int len_a = wcstombs_len(NULL, str, len, locale);
    if(len_a < 0)
        return -1;

    out = HeapAlloc(GetProcessHeap(), 0, len_a);
    if(!out)
        return -1;

    wcstombs_len(out, str, len, locale);
    len = pf_puts(puts_ctx, len_a, out);
    HeapFree(GetProcessHeap(), 0, out);
    return len;
#endif
}

static inline int FUNC_NAME(pf_output_str)(FUNC_NAME(puts_clbk) pf_puts, void *puts_ctx,
        const char *str, int len, _locale_t locale)
{
#ifdef PRINTF_WIDE
    LPWSTR out;
    int len_w = mbstowcs_len(NULL, str, len, locale);
    if(len_w < 0)
        return -1;

    out = HeapAlloc(GetProcessHeap(), 0, len_w*sizeof(WCHAR));
    if(!out)
        return -1;

    mbstowcs_len(out, str, len, locale);
    len = pf_puts(puts_ctx, len_w, out);
    HeapFree(GetProcessHeap(), 0, out);
    return len;
#else
    return pf_puts(puts_ctx, len, str);
#endif
}

static inline int FUNC_NAME(pf_output_format_wstr)(FUNC_NAME(puts_clbk) pf_puts, void *puts_ctx,
        const wchar_t *str, int len, pf_flags *flags, _locale_t locale)
{
    int r, ret;

    if(len < 0) {
        /* Do not search past the length specified by the precision. */
        if(flags->Precision>=0)
            len = wcsnlen(str, flags->Precision);
        else
            len = wcslen(str);
    }

    if(flags->Precision>=0 && flags->Precision<len)
        len = flags->Precision;

    r = FUNC_NAME(pf_fill)(pf_puts, puts_ctx, len, flags, TRUE);
    ret = r;
    if(r >= 0) {
        r = FUNC_NAME(pf_output_wstr)(pf_puts, puts_ctx, str, len, locale);
        ret += r;
    }
    if(r >= 0) {
        r = FUNC_NAME(pf_fill)(pf_puts, puts_ctx, len, flags, FALSE);
        ret += r;
    }

    return r>=0 ? ret : r;
}

static inline int FUNC_NAME(pf_output_format_str)(FUNC_NAME(puts_clbk) pf_puts, void *puts_ctx,
        const char *str, int len, pf_flags *flags, _locale_t locale)
{
    int r, ret;

    if(len < 0) {
        /* Do not search past the length specified by the precision. */
        if(flags->Precision>=0)
            len = strnlen(str, flags->Precision);
        else
            len = strlen(str);
    }

    if(flags->Precision>=0 && flags->Precision<len)
        len = flags->Precision;

    r = FUNC_NAME(pf_fill)(pf_puts, puts_ctx, len, flags, TRUE);
    ret = r;
    if(r >= 0) {
        r = FUNC_NAME(pf_output_str)(pf_puts, puts_ctx, str, len, locale);
        ret += r;
    }
    if(r >= 0) {
        r = FUNC_NAME(pf_fill)(pf_puts, puts_ctx, len, flags, FALSE);
        ret += r;
    }

    return r>=0 ? ret : r;
}

static inline int FUNC_NAME(pf_handle_string)(FUNC_NAME(puts_clbk) pf_puts, void *puts_ctx,
        const void *str, int len, pf_flags *flags, _locale_t locale, BOOL legacy_wide)
{
    BOOL api_is_wide = sizeof(APICHAR) == sizeof(wchar_t);
    BOOL complement_is_narrow = legacy_wide ? api_is_wide : FALSE;
#ifdef PRINTF_WIDE

    if(!str)
        return FUNC_NAME(pf_output_format_wstr)(pf_puts, puts_ctx, L"(null)", 6, flags, locale);
#else
    if(!str)
        return FUNC_NAME(pf_output_format_str)(pf_puts, puts_ctx, "(null)", 6, flags, locale);
#endif

    if((flags->NaturalString && api_is_wide) || flags->WideString || flags->IntegerLength == LEN_LONG)
        return FUNC_NAME(pf_output_format_wstr)(pf_puts, puts_ctx, str, len, flags, locale);
    if((flags->NaturalString && !api_is_wide) || flags->IntegerLength == LEN_SHORT)
        return FUNC_NAME(pf_output_format_str)(pf_puts, puts_ctx, str, len, flags, locale);

    if((flags->Format=='S' || flags->Format=='C') == complement_is_narrow)
        return FUNC_NAME(pf_output_format_str)(pf_puts, puts_ctx, str, len, flags, locale);
    else
        return FUNC_NAME(pf_output_format_wstr)(pf_puts, puts_ctx, str, len, flags, locale);
}

static inline int FUNC_NAME(pf_output_special_fp)(FUNC_NAME(puts_clbk) pf_puts, void *puts_ctx,
        double v, pf_flags *flags, _locale_t locale,
        BOOL legacy_msvcrt_compat, BOOL three_digit_exp)
{
    APICHAR pfx[16], sfx[8], *p;
    int len = 0, r, frac_len, pfx_len, sfx_len;

    if(!legacy_msvcrt_compat) {
        const char *str;

        if(isinf(v)) {
            if(strchr("AEFG", flags->Format)) str = "INF";
            else str = "inf";
        }else {
            if(strchr("AEFG", flags->Format)) str = (flags->Sign == '-' ? "NAN(IND)" : "NAN");
            else str = (flags->Sign == '-' ? "nan(ind)" : "nan");
        }

        flags->Precision = -1;
        flags->PadZero = FALSE;
        return FUNC_NAME(pf_output_format_str)(pf_puts, puts_ctx, str, -1, flags, locale);
    }

    /* workaround a bug in native implementation */
    if(flags->Format=='g' || flags->Format=='G')
        flags->Precision--;

    p = pfx;
    if(flags->PadZero && (flags->Format=='a' || flags->Format=='A')) {
        if (flags->Sign) *p++ = flags->Sign;
        *p++ = '0';
        *p++ = (flags->Format=='a' ? 'x' : 'X');
        r = pf_puts(puts_ctx, p-pfx, pfx);
        if(r < 0) return r;
        len += r;

        flags->FieldLength -= p-pfx;
    }

    p = pfx;
    if(!flags->PadZero && (flags->Format=='a' || flags->Format=='A')) {
        *p++ = '0';
        *p++ = (flags->Format=='a' ? 'x' : 'X');
    }

    *p++ = '1';
    *p++ = *(locale ? locale->locinfo : get_locinfo())->lconv->decimal_point;
    *p++ = '#';
    frac_len = 1;

    if(isinf(v)) {
        *p++ = 'I';
        *p++ = 'N';
        *p++ = 'F';
        frac_len += 3;
    }else if(flags->Sign == '-') {
        *p++ = 'I';
        *p++ = 'N';
        *p++ = 'D';
        frac_len += 3;
    }else {
        *p++ = 'Q';
        *p++ = 'N';
        *p++ = 'A';
        *p++ = 'N';
        frac_len += 4;
    }
    *p = 0;
    pfx_len = p - pfx;

    if(len) flags->Sign = 0;

    if(flags->Precision>=0 && flags->Precision<frac_len)
        p[flags->Precision - frac_len - 1]++;

    p = sfx;
    if(strchr("aAeE", flags->Format)) {
        if(flags->Format == 'a') *p++ = 'p';
        else if(flags->Format == 'A') *p++ = 'P';
        else if(flags->Format == 'e') *p++ = 'e';
        else *p++ = 'E';

        *p++ = '+';
        *p++ = '0';

        if(flags->Format == 'e' || flags->Format == 'E') {
            *p++ = '0';
            if(three_digit_exp) *p++ = '0';
        }
    }
    *p = 0;

    if(!flags->Alternate && (flags->Format == 'g' || flags->Format == 'G')) sfx_len = frac_len;
    else sfx_len = flags->Precision;

    if(sfx_len == -1) {
        if(strchr("fFeE", flags->Format)) sfx_len = 6;
        else if(flags->Format == 'a' || flags->Format == 'A') sfx_len = 13;
    }
    sfx_len += p - sfx - frac_len;

    if(sfx_len > 0) flags->FieldLength -= sfx_len;
    if(flags->Precision >= 0) {
        if(!flags->Precision) flags->Precision--;
        flags->Precision += pfx_len - frac_len;
    }
#ifdef PRINTF_WIDE
    r = FUNC_NAME(pf_output_format_wstr)(pf_puts, puts_ctx, pfx, -1, flags, locale);
#else
    r = FUNC_NAME(pf_output_format_str)(pf_puts, puts_ctx, pfx, -1, flags, locale);
#endif
    if(r < 0) return r;
    len += r;

    flags->FieldLength = sfx_len;
    flags->PadZero = TRUE;
    flags->Precision = -1;
    flags->Sign = 0;
#ifdef PRINTF_WIDE
    r = FUNC_NAME(pf_output_format_wstr)(pf_puts, puts_ctx, sfx, -1, flags, locale);
#else
    r = FUNC_NAME(pf_output_format_str)(pf_puts, puts_ctx, sfx, -1, flags, locale);
#endif
    if(r < 0) return r;
    len += r;

    return len;
}

static inline int FUNC_NAME(pf_output_hex_fp)(FUNC_NAME(puts_clbk) pf_puts, void *puts_ctx,
        double v, pf_flags *flags, _locale_t locale, BOOL standard_rounding)
{
    const APICHAR digits[2][16] = {
        { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' },
        { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' }
    };

    APICHAR pfx[4+MANT_BITS/4+1], sfx[8], *p;
    ULONGLONG mant;
    int len = 0, sfx_len = 0, r, exp;

    mant = (*(ULONGLONG*)&v) << 1;
    exp = (mant >> MANT_BITS);
    exp -= (1 << (EXP_BITS - 1)) - 1;
    mant = (mant << EXP_BITS) >> (EXP_BITS+1);

    p = pfx;
    if(flags->PadZero) {
        if(flags->Sign) *p++ = flags->Sign;
        *p++ = '0';
        *p++ = (flags->Format=='a' ? 'x' : 'X');
        r = pf_puts(puts_ctx, p-pfx, pfx);
        if(r < 0) return r;
        len += r;

        flags->FieldLength -= p-pfx;
        flags->Sign = 0;
        p = pfx;
    }else {
        *p++ = '0';
        *p++ = (flags->Format=='a' ? 'x' : 'X');
    }
    if(exp == -(1 << (EXP_BITS-1))+1) {
        if(!mant) exp = 0;
        else exp++;
        *p++ = '0';
    }else {
        *p++ = '1';
    }
    *p++ = *(locale ? locale->locinfo : get_locinfo())->lconv->decimal_point;
    for(r=MANT_BITS/4-1; r>=0; r--) {
        p[r] = digits[flags->Format == 'A'][mant & 15];
        mant >>= 4;
    }
    if(!flags->Precision) {
        if(p[0] >= '8') p[-2]++;
        if(!flags->Alternate) p--;
    }else if(flags->Precision>0 && flags->Precision<MANT_BITS/4) {
        BOOL round_up = FALSE;

        if(!standard_rounding) round_up = (p[flags->Precision] >= '8');
        else if(p[flags->Precision] > '8') round_up = TRUE;
        else if(p[flags->Precision] == '8') {
            for(r = flags->Precision+1; r<MANT_BITS/4; r++) {
                if(p[r] != '0') {
                    round_up = TRUE;
                    break;
                }
            }

            if(!round_up) {
                if(p[flags->Precision-1] <= '9') round_up = (p[flags->Precision-1] - '0') & 1;
                else if(p[flags->Precision-1] <= 'F') round_up = (p[flags->Precision-1] - 'A') & 1;
                else round_up = (p[flags->Precision-1] - 'a') & 1;
            }
        }

        for(r=flags->Precision-1; r>=0 && round_up; r--) {
            round_up = FALSE;
            if(p[r]=='f' || p[r]=='F') {
                p[r] = '0';
                round_up = TRUE;
            }else if(p[r] == '9') {
                p[r] = (flags->Format == 'a' ? 'a' : 'A');
            }else {
                p[r]++;
            }
        }
        if(round_up) p[-2]++;
        p += flags->Precision;
    }else {
        p += MANT_BITS/4;
        if(flags->Precision > MANT_BITS/4) sfx_len += flags->Precision - MANT_BITS/4;
    }
    *p = 0;

    p = sfx;
    *p++ = (flags->Format == 'a' ? 'p' : 'P');
    if(exp < 0) {
        *p++ = '-';
        exp = -exp;
    }else {
        *p++ = '+';
    }
    for(r=3; r>=0; r--) {
        p[r] = exp%10 + '0';
        exp /= 10;
        if(!exp) break;
    }
    for(exp=0; exp<4-r; exp++)
        p[exp] = p[exp+r];
    p += exp;
    *p = 0;
    sfx_len += p - sfx;

    flags->FieldLength -= sfx_len;
    flags->Precision = -1;
#ifdef PRINTF_WIDE
    r = FUNC_NAME(pf_output_format_wstr)(pf_puts, puts_ctx, pfx, -1, flags, locale);
#else
    r = FUNC_NAME(pf_output_format_str)(pf_puts, puts_ctx, pfx, -1, flags, locale);
#endif
    if(r < 0) return r;
    len += r;

    flags->FieldLength = sfx_len;
    flags->PadZero = TRUE;
    flags->Sign = 0;
#ifdef PRINTF_WIDE
    r = FUNC_NAME(pf_output_format_wstr)(pf_puts, puts_ctx, sfx, -1, flags, locale);
#else
    r = FUNC_NAME(pf_output_format_str)(pf_puts, puts_ctx, sfx, -1, flags, locale);
#endif
    if(r < 0) return r;
    len += r;

    return len;
}

/* pf_integer_conv:  prints x to buf, including alternate formats and
   additional precision digits, but not field characters or the sign */
static inline void FUNC_NAME(pf_integer_conv)(APICHAR *buf, pf_flags *flags, LONGLONG x)
{
    unsigned int base;
    const char *digits;
    int i, j, k;

    if(flags->Format == 'o')
        base = 8;
    else if(flags->Format=='x' || flags->Format=='X')
        base = 16;
    else
        base = 10;

    if(flags->Format == 'X')
        digits = "0123456789ABCDEFX";
    else
        digits = "0123456789abcdefx";

    if(x<0 && (flags->Format=='d' || flags->Format=='i')) {
        x = -x;
        flags->Sign = '-';
    }

    i = 0;
    if(x == 0) {
        flags->Alternate = FALSE;
        if(flags->Precision)
            buf[i++] = '0';
    } else {
        while(x != 0) {
            j = (ULONGLONG)x%base;
            x = (ULONGLONG)x/base;
            buf[i++] = digits[j];
        }
    }
    k = flags->Precision-i;
    while(k-- > 0)
        buf[i++] = '0';
    if(flags->Alternate) {
        if(base == 16) {
            buf[i++] = digits[16];
            buf[i++] = '0';
        } else if(base==8 && buf[i-1]!='0')
            buf[i++] = '0';
    }

    /* Adjust precision so pf_fill won't truncate the number later */
    flags->Precision = i;

    buf[i] = '\0';
    j = 0;
    while(--i > j) {
        APICHAR tmp = buf[j];
        buf[j] = buf[i];
        buf[i] = tmp;
        j++;
    }
}

static inline int FUNC_NAME(pf_output_fp)(FUNC_NAME(puts_clbk) pf_puts, void *puts_ctx,
        double v, pf_flags *flags, _locale_t locale, BOOL three_digit_exp,
        BOOL standard_rounding)
{
    int e2, e10 = 0, round_pos, round_limb, radix_pos, first_limb_len, i, len, r, ret;
    BYTE bnum_data[FIELD_OFFSET(struct bnum, data[BNUM_PREC64])];
    struct bnum *b = (struct bnum*)bnum_data;
    APICHAR buf[LIMB_DIGITS + 1];
    BOOL trim_tail = FALSE, round_up = FALSE;
    pf_flags f;
    int limb_len, prec;
    ULONGLONG m;
    DWORD l;

    if(flags->Precision == -1)
        flags->Precision = 6;

    v = frexp(v, &e2);
    if(v) {
        m = (ULONGLONG)1 << (MANT_BITS - 1);
        m |= (*(ULONGLONG*)&v & (((ULONGLONG)1 << (MANT_BITS - 1)) - 1));
        b->b = 0;
        b->e = 2;
        b->size = BNUM_PREC64;
        b->data[0] = m % LIMB_MAX;
        b->data[1] = m / LIMB_MAX;
        e2 -= MANT_BITS;

        while(e2 > 0) {
            int shift = e2 > 29 ? 29 : e2;
            if(bnum_lshift(b, shift)) e10 += LIMB_DIGITS;
            e2 -= shift;
        }
        while(e2 < 0) {
            int shift = -e2 > 9 ? 9 : -e2;
            if(bnum_rshift(b, shift)) e10 -= LIMB_DIGITS;
            e2 += shift;
        }
    } else {
        b->b = 0;
        b->e = 1;
        b->size = BNUM_PREC64;
        b->data[0] = 0;
        e10 = -LIMB_DIGITS;
    }

    if(!b->data[bnum_idx(b, b->e-1)])
        first_limb_len = 1;
    else
        first_limb_len = log10i(b->data[bnum_idx(b, b->e - 1)]) + 1;
    radix_pos = first_limb_len + LIMB_DIGITS + e10;

    round_pos = flags->Precision;
    if(flags->Format=='f' || flags->Format=='F')
        round_pos += radix_pos;
    else if(!flags->Precision || flags->Format=='e' || flags->Format=='E')
        round_pos++;
    if (round_pos <= first_limb_len)
        round_limb = b->e + (first_limb_len - round_pos) / LIMB_DIGITS - 1;
    else
        round_limb = b->e - (round_pos - first_limb_len - 1) / LIMB_DIGITS - 2;

    if (b->b<=round_limb && round_limb<b->e) {
        if (round_pos <= first_limb_len) {
            round_pos = first_limb_len - round_pos;
        } else {
            round_pos = LIMB_DIGITS - (round_pos - first_limb_len) % LIMB_DIGITS;
            if (round_pos == LIMB_DIGITS) round_pos = 0;
        }

        if (round_pos) {
            l = b->data[bnum_idx(b, round_limb)] % p10s[round_pos];
            b->data[bnum_idx(b, round_limb)] -= l;
            if(!standard_rounding) round_up = (2*l >= p10s[round_pos]);
            else if(2*l > p10s[round_pos]) round_up = TRUE;
            else if(2*l == p10s[round_pos]) {
                for(r = round_limb-1; r >= b->b; r--) {
                    if(b->data[bnum_idx(b, r)]) {
                        round_up = TRUE;
                        break;
                    }
                }

                if(!round_up) round_up = b->data[bnum_idx(b, round_limb)] / p10s[round_pos] & 1;
            }
        } else if(round_limb - 1 >= b->b) {
            if(!standard_rounding) round_up = (2*b->data[bnum_idx(b, round_limb-1)] >= LIMB_MAX);
            else if(2*b->data[bnum_idx(b, round_limb-1)] > LIMB_MAX) round_up = TRUE;
            else if(2*b->data[bnum_idx(b, round_limb-1)] == LIMB_MAX) {
                for(r = round_limb-2; r >= b->b; r--) {
                    if(b->data[bnum_idx(b, r)]) {
                        round_up = TRUE;
                        break;
                    }
                }

                if(!round_up) round_up = b->data[bnum_idx(b, round_limb)] & 1;
            }
        }
        b->b = round_limb;

        if(round_up) {
            b->data[bnum_idx(b, b->b)] += p10s[round_pos];
            for(i = b->b; i < b->e; i++) {
                if(b->data[bnum_idx(b, i)] < LIMB_MAX) break;

                b->data[bnum_idx(b, i)] -= LIMB_MAX;
                if(i+1 < b->e) b->data[bnum_idx(b, i+1)]++;
                else b->data[bnum_idx(b, i+1)] = 1;
            }
            if(i == b->e-1) {
                if(!b->data[bnum_idx(b, b->e-1)])
                    i = 1;
                else
                    i = log10i(b->data[bnum_idx(b, b->e-1)]) + 1;
                if(i != first_limb_len) {
                    first_limb_len = i;
                    radix_pos++;

                    round_pos++;
                    if (round_pos == LIMB_DIGITS)
                    {
                        round_pos = 0;
                        round_limb++;
                    }
                }
            } else if(i == b->e) {
                first_limb_len = 1;
                radix_pos++;
                b->e++;

                round_pos++;
                if (round_pos == LIMB_DIGITS)
                {
                    round_pos = 0;
                    round_limb++;
                }
            }
        }
    }
    else if(b->e <= round_limb) { /* got 0 or 1 after rounding */
        if(b->e == round_limb) {
            if(!standard_rounding) round_up = b->data[bnum_idx(b, b->e-1)] >= LIMB_MAX/2;
            else if(b->data[bnum_idx(b, b->e-1)] > LIMB_MAX/2) round_up = TRUE;
            else if(b->data[bnum_idx(b, b->e-1)] == LIMB_MAX/2) {
                for(r = b->e-2; r >= b->b; r--) {
                    if(b->data[bnum_idx(b, r)]) {
                        round_up = TRUE;
                        break;
                    }
                }
            }
        }

        b->data[bnum_idx(b, round_limb)] = round_up;
        b->b = round_limb;
        b->e = b->b + 1;
        first_limb_len = 1;
        radix_pos++;
    }

    if(flags->Format=='g' || flags->Format=='G') {
        trim_tail = TRUE;

        if(radix_pos>=-3 && radix_pos<=flags->Precision) {
            flags->Format -= 1;
            if(!flags->Precision) flags->Precision++;
            flags->Precision -= radix_pos;
        } else {
            flags->Format -= 2;
            if(flags->Precision > 0) flags->Precision--;
        }
    }

    if(trim_tail && !flags->Alternate) {
        for(i=round_limb; flags->Precision>0 && i<b->e; i++) {
            if(i>=b->b)
                l = b->data[bnum_idx(b, i)];
            else
                l = 0;

            if(i == round_limb) {
                if(flags->Format=='f' || flags->Format=='F')
                    r = radix_pos + flags->Precision;
                else
                    r = flags->Precision + 1;
                r = first_limb_len + LIMB_DIGITS * (b->e-1 - b->b) - r;
                r %= LIMB_DIGITS;
                if(r < 0) r += LIMB_DIGITS;
                l /= p10s[r];
                limb_len = LIMB_DIGITS - r;
            } else {
                limb_len = LIMB_DIGITS;
            }

            if(!l) {
                flags->Precision -= limb_len;
            } else {
                while(l % 10 == 0) {
                    flags->Precision--;
                    l /= 10;
                }
            }

            if(flags->Precision <= 0) {
                flags->Precision = 0;
                break;
            }
            if(l)
                break;
        }
    }

    len = flags->Precision;
    if(flags->Precision || flags->Alternate) len++;
    if(flags->Format=='f' || flags->Format=='F') {
        len += (radix_pos > 0 ? radix_pos : 1);
    } else if(flags->Format=='e' || flags->Format=='E') {
        radix_pos--;
        if(!trim_tail || radix_pos) {
            len += 3; /* strlen("1e+") */
            if(three_digit_exp || radix_pos<-99 || radix_pos>99) len += 3;
            else len += 2;
        } else {
            len++;
        }
    }

    r = FUNC_NAME(pf_fill)(pf_puts, puts_ctx, len, flags, TRUE);
    if(r < 0) return r;
    ret = r;

    f.Format = 'd';
    f.PadZero = TRUE;
    if(flags->Format=='f' || flags->Format=='F') {
        if(radix_pos <= 0) {
            buf[0] = '0';
            r = pf_puts(puts_ctx, 1, buf);
            if(r < 0) return r;
            ret += r;
        }

        limb_len = LIMB_DIGITS;
        for(i=b->e-1; radix_pos>0 && i>=b->b; i--) {
            limb_len = (i == b->e-1 ? first_limb_len : LIMB_DIGITS);
            l = b->data[bnum_idx(b, i)];
            if(limb_len > radix_pos) {
                f.Precision = radix_pos;
                l /= p10s[limb_len - radix_pos];
                limb_len = limb_len - radix_pos;
            } else {
                f.Precision = limb_len;
                limb_len = LIMB_DIGITS;
            }
            radix_pos -= f.Precision;
            FUNC_NAME(pf_integer_conv)(buf, &f, l);

            r = pf_puts(puts_ctx, f.Precision, buf);
            if(r < 0) return r;
            ret += r;
        }

        buf[0] = '0';
        for(; radix_pos>0; radix_pos--) {
            r = pf_puts(puts_ctx, 1, buf);
            if(r < 0) return r;
            ret += r;
        }

        if(flags->Precision || flags->Alternate) {
            buf[0] = *(locale ? locale->locinfo : get_locinfo())->lconv->decimal_point;
            r = pf_puts(puts_ctx, 1, buf);
            if(r < 0) return r;
            ret += r;
        }

        prec = flags->Precision;
        buf[0] = '0';
        for(; prec>0 && radix_pos+LIMB_DIGITS-first_limb_len<0; radix_pos++, prec--) {
            r = pf_puts(puts_ctx, 1, buf);
            if(r < 0) return r;
            ret += r;
        }

        for(; prec>0 && i>=b->b; i--) {
            l = b->data[bnum_idx(b, i)];
            if(limb_len != LIMB_DIGITS)
                l %= p10s[limb_len];
            if(limb_len > prec) {
                f.Precision = prec;
                l /= p10s[limb_len - prec];
            } else {
                f.Precision = limb_len;
                limb_len = LIMB_DIGITS;
            }
            prec -= f.Precision;
            FUNC_NAME(pf_integer_conv)(buf, &f, l);

            r = pf_puts(puts_ctx, f.Precision, buf);
            if(r < 0) return r;
            ret += r;
        }

        buf[0] = '0';
        for(; prec>0; prec--) {
            r = pf_puts(puts_ctx, 1, buf);
            if(r < 0) return r;
            ret += r;
        }
    } else {
        l = b->data[bnum_idx(b, b->e - 1)];
        l /= p10s[first_limb_len - 1];

        buf[0] = '0' + l;
        r = pf_puts(puts_ctx, 1, buf);
        if(r < 0) return r;
        ret += r;

        if(flags->Precision || flags->Alternate) {
            buf[0] = *(locale ? locale->locinfo : get_locinfo())->lconv->decimal_point;
            r = pf_puts(puts_ctx, 1, buf);
            if(r < 0) return r;
            ret += r;
        }

        prec = flags->Precision;
        limb_len = LIMB_DIGITS;
        for(i=b->e-1; prec>0 && i>=b->b; i--) {
            l = b->data[bnum_idx(b, i)];
            if(i == b->e-1) {
                limb_len = first_limb_len - 1;
                l %= p10s[limb_len];
            }

            if(limb_len > prec) {
                f.Precision = prec;
                l /= p10s[limb_len - prec];
            } else {
                f.Precision = limb_len;
                limb_len = LIMB_DIGITS;
            }
            prec -= f.Precision;
            FUNC_NAME(pf_integer_conv)(buf, &f, l);

            r = pf_puts(puts_ctx, f.Precision, buf);
            if(r < 0) return r;
            ret += r;
        }

        buf[0] = '0';
        for(; prec>0; prec--) {
            r = pf_puts(puts_ctx, 1, buf);
            if(r < 0) return r;
            ret += r;
        }

        if(!trim_tail || radix_pos) {
            buf[0] = flags->Format;
            buf[1] = radix_pos < 0 ? '-' : '+';
            r = pf_puts(puts_ctx, 2, buf);
            if(r < 0) return r;
            ret += r;

            f.Precision = three_digit_exp ? 3 : 2;
            FUNC_NAME(pf_integer_conv)(buf, &f, radix_pos);
            r = pf_puts(puts_ctx, f.Precision, buf);
            if(r < 0) return r;
            ret += r;
        }
    }

    r = FUNC_NAME(pf_fill)(pf_puts, puts_ctx, len, flags, FALSE);
    if(r < 0) return r;
    ret += r;
    return ret;
}

int FUNC_NAME(pf_printf)(FUNC_NAME(puts_clbk) pf_puts, void *puts_ctx, const APICHAR *fmt,
        _locale_t locale, DWORD options,
        args_clbk pf_args, void *args_ctx, va_list *valist)
{
    const APICHAR *q, *p = fmt;
    APICHAR buf[32];
    int written = 0, pos, i;
    pf_flags flags;
    BOOL positional_params = options & MSVCRT_PRINTF_POSITIONAL_PARAMS;
    BOOL invoke_invalid_param_handler = options & MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER;
#if _MSVCR_VER >= 140
    BOOL legacy_wide = options & _CRT_INTERNAL_PRINTF_LEGACY_WIDE_SPECIFIERS;
    BOOL legacy_msvcrt_compat = options & _CRT_INTERNAL_PRINTF_LEGACY_MSVCRT_COMPATIBILITY;
    BOOL three_digit_exp = options & _CRT_INTERNAL_PRINTF_LEGACY_THREE_DIGIT_EXPONENTS;
    BOOL standard_rounding = options & _CRT_INTERNAL_PRINTF_STANDARD_ROUNDING;
#else
    BOOL legacy_wide = TRUE, legacy_msvcrt_compat = TRUE;
    BOOL three_digit_exp = _get_output_format() != _TWO_DIGIT_EXPONENT;
    BOOL standard_rounding = FALSE;
#endif

    if (!MSVCRT_CHECK_PMT(fmt != NULL))
        return -1;

    while(*p) {
        /* output characters before '%' */
        for(q=p; *q && *q!='%'; q++);
        if(p != q) {
            i = pf_puts(puts_ctx, q-p, p);
            if(i < 0)
                return i;

            written += i;
            p = q;
            continue;
        }

        /* *p == '%' here */
        p++;

        /* output a single '%' character */
        if(*p == '%') {
            i = pf_puts(puts_ctx, 1, p++);
            if(i < 0)
                return i;

            written += i;
            continue;
        }

        /* check parameter position */
        if(positional_params && (q = FUNC_NAME(pf_parse_int)(p, &pos)) && *q=='$')
            p = q+1;
        else
            pos = -1;

        /* parse the flags */
        memset(&flags, 0, sizeof(flags));
        while(*p) {
            if(*p=='+' || *p==' ') {
                if(flags.Sign != '+')
                    flags.Sign = *p;
            } else if(*p == '-')
                flags.LeftAlign = TRUE;
            else if(*p == '0')
                flags.PadZero = TRUE;
            else if(*p == '#')
                flags.Alternate = TRUE;
            else
                break;

            p++;
        }

        /* parse the width */
        if(*p == '*') {
            p++;
            if(positional_params && (q = FUNC_NAME(pf_parse_int)(p, &i)) && *q=='$')
                p = q+1;
            else
                i = -1;

            flags.FieldLength = pf_args(args_ctx, i, VT_INT, valist).get_int;
            if(flags.FieldLength < 0) {
                flags.LeftAlign = TRUE;
                flags.FieldLength = -flags.FieldLength;
            }
        }

#if _MSVCR_VER >= 140
        if (*p >= '0' && *p <= '9')
            flags.FieldLength = 0;
#endif

        while (*p >= '0' && *p <= '9') {
            flags.FieldLength *= 10;
            flags.FieldLength += *p++ - '0';
        }

        /* parse the precision */
        flags.Precision = -1;
        if(*p == '.') {
            flags.Precision = 0;
            p++;
            if(*p == '*') {
                p++;
                if(positional_params && (q = FUNC_NAME(pf_parse_int)(p, &i)) && *q=='$')
                    p = q+1;
                else
                    i = -1;

                flags.Precision = pf_args(args_ctx, i, VT_INT, valist).get_int;
            } else while (*p >= '0' && *p <= '9') {
                flags.Precision *= 10;
                flags.Precision += *p++ - '0';
            }
        }

        /* parse argument size modifier */
        while(*p) {
            if(*p=='l' && *(p+1)=='l') {
                flags.IntegerDouble = TRUE;
                p++;
            } else if(*p=='l') {
                flags.IntegerLength = LEN_LONG;
            } else if(*p == 'h') {
                flags.IntegerLength = LEN_SHORT;
            } else if(*p == 'I') {
                if(*(p+1)=='6' && *(p+2)=='4') {
                    flags.IntegerDouble = TRUE;
                    p += 2;
                } else if(*(p+1)=='3' && *(p+2)=='2')
                    p += 2;
                else if(p[1] && strchr("diouxX", p[1]))
                    flags.IntegerNative = TRUE;
                else
                    break;
            } else if(*p == 'w')
                flags.WideString = TRUE;
#if _MSVCR_VER == 0 || _MSVCR_VER >= 140
            else if((*p == 'z' || *p == 't') && p[1] && strchr("diouxX", p[1]))
                flags.IntegerNative = TRUE;
            else if(*p == 'j')
                flags.IntegerDouble = TRUE;
#endif
#if _MSVCR_VER >= 140
            else if(*p == 'T')
                flags.NaturalString = TRUE;
#endif
            else if(*p != 'L' && ((*p != 'F' && *p != 'N') || !legacy_msvcrt_compat))
                break;
            p++;
        }

        flags.Format = *p;

        if(flags.Format == 's' || flags.Format == 'S') {
            i = FUNC_NAME(pf_handle_string)(pf_puts, puts_ctx,
                    pf_args(args_ctx, pos, VT_PTR, valist).get_ptr,
                    -1,  &flags, locale, legacy_wide);
        } else if(flags.Format == 'c' || flags.Format == 'C') {
            int ch = pf_args(args_ctx, pos, VT_INT, valist).get_int;

            i = FUNC_NAME(pf_handle_string)(pf_puts, puts_ctx, &ch, 1, &flags, locale, legacy_wide);
            if(i < 0) i = 0; /* ignore conversion error */
        } else if(flags.Format == 'p') {
            flags.Format = 'X';
            flags.PadZero = TRUE;
            i = flags.Precision;
            flags.Precision = 2*sizeof(void*);
            FUNC_NAME(pf_integer_conv)(buf, &flags,
                    (ULONG_PTR)pf_args(args_ctx, pos, VT_PTR, valist).get_ptr);
            flags.PadZero = FALSE;
            flags.Precision = i;

#ifdef PRINTF_WIDE
            i = FUNC_NAME(pf_output_format_wstr)(pf_puts, puts_ctx, buf, -1, &flags, locale);
#else
            i = FUNC_NAME(pf_output_format_str)(pf_puts, puts_ctx, buf, -1, &flags, locale);
#endif
        } else if(flags.Format == 'n') {
            int *used;

            if(!n_format_enabled) {
                MSVCRT_INVALID_PMT("\'n\' format specifier disabled", EINVAL);
                return -1;
            }

            used = pf_args(args_ctx, pos, VT_PTR, valist).get_ptr;
            *used = written;
            i = 0;
        } else if(flags.Format && strchr("diouxX", flags.Format)) {
            APICHAR *tmp = buf;
            int max_len;

            /* 0 padding is added after '0x' if Alternate flag is in use */
            if((flags.Format=='x' || flags.Format=='X') && flags.PadZero && flags.Alternate
                    && !flags.LeftAlign && flags.Precision<flags.FieldLength-2)
                flags.Precision = flags.FieldLength - 2;

            max_len = (flags.FieldLength>flags.Precision ? flags.FieldLength : flags.Precision) + 10;
            if(max_len > ARRAY_SIZE(buf))
                tmp = HeapAlloc(GetProcessHeap(), 0, max_len);
            if(!tmp)
                return -1;

            if(flags.IntegerDouble || (flags.IntegerNative && sizeof(void*) == 8))
                FUNC_NAME(pf_integer_conv)(tmp, &flags, pf_args(args_ctx, pos,
                            VT_I8, valist).get_longlong);
            else if(flags.Format=='d' || flags.Format=='i')
                FUNC_NAME(pf_integer_conv)(tmp, &flags,
                        flags.IntegerLength != LEN_SHORT ?
                        pf_args(args_ctx, pos, VT_INT, valist).get_int :
                        (short)pf_args(args_ctx, pos, VT_INT, valist).get_int);
            else
                FUNC_NAME(pf_integer_conv)(tmp, &flags,
                        flags.IntegerLength != LEN_SHORT ?
                        (unsigned)pf_args(args_ctx, pos, VT_INT, valist).get_int :
                        (unsigned short)pf_args(args_ctx, pos, VT_INT, valist).get_int);

#ifdef PRINTF_WIDE
            i = FUNC_NAME(pf_output_format_wstr)(pf_puts, puts_ctx, tmp, -1, &flags, locale);
#else
            i = FUNC_NAME(pf_output_format_str)(pf_puts, puts_ctx, tmp, -1, &flags, locale);
#endif
            if(tmp != buf)
                HeapFree(GetProcessHeap(), 0, tmp);
        } else if(flags.Format && strchr("aAeEfFgG", flags.Format)) {
            double val = pf_args(args_ctx, pos, VT_R8, valist).get_double;

            if(signbit(val)) {
                flags.Sign = '-';
                val = -val;
            }

            if(isinf(val) || isnan(val))
                i = FUNC_NAME(pf_output_special_fp)(pf_puts, puts_ctx, val, &flags,
                        locale, legacy_msvcrt_compat, three_digit_exp);
            else if(flags.Format=='a' || flags.Format=='A')
                i = FUNC_NAME(pf_output_hex_fp)(pf_puts, puts_ctx, val, &flags,
                        locale, standard_rounding);
            else
                i = FUNC_NAME(pf_output_fp)(pf_puts, puts_ctx, val, &flags,
                        locale, three_digit_exp, standard_rounding);
        } else {
            if(invoke_invalid_param_handler) {
                _invalid_parameter(NULL, NULL, NULL, 0, 0);
                *_errno() = EINVAL;
                return -1;
            }

            continue;
        }

        if(i < 0)
            return i;
        written += i;
        p++;
    }

    return written;
}

#ifndef PRINTF_WIDE
enum types_clbk_flags {
    TYPE_CLBK_VA_LIST = 1,
    TYPE_CLBK_POSITIONAL = 2,
    TYPE_CLBK_ERROR_POS = 4,
    TYPE_CLBK_ERROR_TYPE = 8
};

/* This functions stores types of arguments. It uses args[0] internally */
static printf_arg arg_clbk_type(void *ctx, int pos, int type, va_list *valist)
{
    static const printf_arg ret;
    printf_arg *args = ctx;

    if(pos == -1) {
        args[0].get_int |= TYPE_CLBK_VA_LIST;
        return ret;
    } else
        args[0].get_int |= TYPE_CLBK_POSITIONAL;

    if(pos<1 || pos>_ARGMAX)
        args[0].get_int |= TYPE_CLBK_ERROR_POS;
    else if(args[pos].get_int && args[pos].get_int!=type)
        args[0].get_int |= TYPE_CLBK_ERROR_TYPE;
    else
        args[pos].get_int = type;

    return ret;
}
#endif

int FUNC_NAME(create_positional_ctx)(void *args_ctx, const APICHAR *format, va_list valist)
{
    struct FUNC_NAME(_str_ctx) puts_ctx = {INT_MAX, NULL};
    printf_arg *args = args_ctx;
    int i, j;

    i = FUNC_NAME(pf_printf)(FUNC_NAME(puts_clbk_str), &puts_ctx, format, NULL,
            MSVCRT_PRINTF_POSITIONAL_PARAMS, arg_clbk_type, args_ctx, NULL);
    if(i < 0)
        return i;

    if(args[0].get_int==0 || args[0].get_int==TYPE_CLBK_VA_LIST)
        return 0;
    if(args[0].get_int != TYPE_CLBK_POSITIONAL)
        return -1;

    for(i=_ARGMAX; i>0; i--)
        if(args[i].get_int)
            break;

    for(j=1; j<=i; j++) {
        switch(args[j].get_int) {
        case VT_I8:
            args[j].get_longlong = va_arg(valist, LONGLONG);
            break;
        case VT_INT:
            args[j].get_int = va_arg(valist, int);
            break;
        case VT_R8:
            args[j].get_double = va_arg(valist, double);
            break;
        case VT_PTR:
            args[j].get_ptr = va_arg(valist, void*);
            break;
        default:
            return -1;
        }
    }

    return j;
}

#undef APICHAR
#undef CONVCHAR
#undef FUNC_NAME
