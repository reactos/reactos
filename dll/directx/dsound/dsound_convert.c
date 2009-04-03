/* DirectSound format conversion and mixing routines
 *
 * Copyright 2007 Maarten Lankhorst
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

/* 8 bits is unsigned, the rest is signed.
 * First I tried to reuse existing stuff from alsa-lib, after that
 * didn't work, I gave up and just went for individual hacks.
 *
 * 24 bit is expensive to do, due to unaligned access.
 * In dlls/winex11.drv/dib_convert.c convert_888_to_0888_asis there is a way
 * around it, but I'm happy current code works, maybe something for later.
 *
 * The ^ 0x80 flips the signed bit, this is the conversion from
 * signed (-128.. 0.. 127) to unsigned (0...255)
 * This is only temporary: All 8 bit data should be converted to signed.
 * then when fed to the sound card, it should be converted to unsigned again.
 *
 * Sound is LITTLE endian
 */

#include "config.h"

#include <stdarg.h>

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"
#include "winternl.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsdriver.h"
#include "dsound_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

#ifdef WORDS_BIGENDIAN
#define le16(x) RtlUshortByteSwap((x))
#define le32(x) RtlUlongByteSwap((x))
#else
#define le16(x) (x)
#define le32(x) (x)
#endif

static inline void src_advance(const void **src, UINT stride, INT *count, UINT *freqAcc, UINT adj)
{
    *freqAcc += adj;
    if (*freqAcc >= (1 << DSOUND_FREQSHIFT))
    {
        ULONG adv = (*freqAcc >> DSOUND_FREQSHIFT);
        *freqAcc &= (1 << DSOUND_FREQSHIFT) - 1;
        *(const char **)src += adv * stride;
        *count -= adv;
    }
}

static void convert_8_to_8 (const void *src, void *dst, UINT src_stride,
        UINT dst_stride, INT count, UINT freqAcc, UINT adj)
{
    while (count > 0)
    {
        *(BYTE *)dst = *(const BYTE *)src;

        dst = (char *)dst + dst_stride;
        src_advance(&src, src_stride, &count, &freqAcc, adj);
    }
}

static void convert_8_to_16 (const void *src, void *dst, UINT src_stride,
        UINT dst_stride, INT count, UINT freqAcc, UINT adj)
{
    while (count > 0)
    {
        WORD dest = *(const BYTE *)src, *dest16 = dst;
        *dest16 = le16(dest * 257 - 32768);

        dst = (char *)dst + dst_stride;
        src_advance(&src, src_stride, &count, &freqAcc, adj);
    }
}

static void convert_8_to_24 (const void *src, void *dst, UINT src_stride,
        UINT dst_stride, INT count, UINT freqAcc, UINT adj)
{
    while (count > 0)
    {
        BYTE dest = *(const BYTE *)src;
        BYTE *dest24 = dst;
        dest24[0] = dest;
        dest24[1] = dest;
        dest24[2] = dest - 0x80;

        dst = (char *)dst + dst_stride;
        src_advance(&src, src_stride, &count, &freqAcc, adj);
    }
}

static void convert_8_to_32 (const void *src, void *dst, UINT src_stride,
        UINT dst_stride, INT count, UINT freqAcc, UINT adj)
{
    while (count > 0)
    {
        DWORD dest = *(const BYTE *)src, *dest32 = dst;
        *dest32 = le32(dest * 16843009 - 2147483648U);

        dst = (char *)dst + dst_stride;
        src_advance(&src, src_stride, &count, &freqAcc, adj);
    }
}

static void convert_16_to_8 (const void *src, void *dst, UINT src_stride,
        UINT dst_stride, INT count, UINT freqAcc, UINT adj)
{
    while (count > 0)
    {
        BYTE *dst8 = dst;
        *dst8 = (le16(*(const WORD *)src)) / 256;
        *dst8 -= 0x80;

        dst = (char *)dst + dst_stride;
        src_advance(&src, src_stride, &count, &freqAcc, adj);
    }
}

static void convert_16_to_16 (const void *src, void *dst, UINT src_stride,
        UINT dst_stride, INT count, UINT freqAcc, UINT adj)
{
    while (count > 0)
    {
        *(WORD *)dst = *(const WORD *)src;

        dst = (char *)dst + dst_stride;
        src_advance(&src, src_stride, &count, &freqAcc, adj);
    }
}

static void convert_16_to_24 (const void *src, void *dst, UINT src_stride,
        UINT dst_stride, INT count, UINT freqAcc, UINT adj)
{
    while (count > 0)
    {
        WORD dest = le16(*(const WORD *)src);
        BYTE *dest24 = dst;

        dest24[0] = dest / 256;
        dest24[1] = dest;
        dest24[2] = dest / 256;

        dst = (char *)dst + dst_stride;
        src_advance(&src, src_stride, &count, &freqAcc, adj);
    }
}

static void convert_16_to_32 (const void *src, void *dst, UINT src_stride,
        UINT dst_stride, INT count, UINT freqAcc, UINT adj)
{
    while (count > 0)
    {
        DWORD dest = *(const WORD *)src, *dest32 = dst;
        *dest32 = dest * 65537;

        dst = (char *)dst + dst_stride;
        src_advance(&src, src_stride, &count, &freqAcc, adj);
    }
}

static void convert_24_to_8 (const void *src, void *dst, UINT src_stride,
        UINT dst_stride, INT count, UINT freqAcc, UINT adj)
{
    while (count > 0)
    {
        BYTE *dst8 = dst;
        *dst8 = ((const BYTE *)src)[2];

        dst = (char *)dst + dst_stride;
        src_advance(&src, src_stride, &count, &freqAcc, adj);
    }
}

static void convert_24_to_16 (const void *src, void *dst, UINT src_stride,
        UINT dst_stride, INT count, UINT freqAcc, UINT adj)
{
    while (count > 0)
    {
        WORD *dest16 = dst;
        const BYTE *source = src;
        *dest16 = le16(source[2] * 256 + source[1]);

        dst = (char *)dst + dst_stride;
        src_advance(&src, src_stride, &count, &freqAcc, adj);
    }
}

static void convert_24_to_24 (const void *src, void *dst, UINT src_stride,
        UINT dst_stride, INT count, UINT freqAcc, UINT adj)
{
    while (count > 0)
    {
        BYTE *dest24 = dst;
        const BYTE *src24 = src;

        dest24[0] = src24[0];
        dest24[1] = src24[1];
        dest24[2] = src24[2];

        dst = (char *)dst + dst_stride;
        src_advance(&src, src_stride, &count, &freqAcc, adj);
    }
}

static void convert_24_to_32 (const void *src, void *dst, UINT src_stride,
        UINT dst_stride, INT count, UINT freqAcc, UINT adj)
{
    while (count > 0)
    {
        DWORD *dest32 = dst;
        const BYTE *source = src;
        *dest32 = le32(source[2] * 16777217 + source[1] * 65536 + source[0] * 256);

        dst = (char *)dst + dst_stride;
        src_advance(&src, src_stride, &count, &freqAcc, adj);
    }
}

static void convert_32_to_8 (const void *src, void *dst, UINT src_stride,
        UINT dst_stride, INT count, UINT freqAcc, UINT adj)
{
    while (count > 0)
    {
        BYTE *dst8 = dst;
        *dst8 = (le32(*(const DWORD *)src) / 16777216);
        *dst8 -= 0x80;

        dst = (char *)dst + dst_stride;
        src_advance(&src, src_stride, &count, &freqAcc, adj);
    }
}

static void convert_32_to_16 (const void *src, void *dst, UINT src_stride,
        UINT dst_stride, INT count, UINT freqAcc, UINT adj)
{
    while (count > 0)
    {
        WORD *dest16 = dst;
        *dest16 = le16(le32(*(const DWORD *)src) / 65536);

        dst = (char *)dst + dst_stride;
        src_advance(&src, src_stride, &count, &freqAcc, adj);
    }
}

static void convert_32_to_24 (const void *src, void *dst, UINT src_stride,
        UINT dst_stride, INT count, UINT freqAcc, UINT adj)
{
    while (count > 0)
    {
        DWORD dest = le32(*(const DWORD *)src);
        BYTE *dest24 = dst;

        dest24[0] = dest / 256;
        dest24[1] = dest / 65536;
        dest24[2] = dest / 16777216;

        dst = (char *)dst + dst_stride;
        src_advance(&src, src_stride, &count, &freqAcc, adj);
    }
}

static void convert_32_to_32 (const void *src, void *dst, UINT src_stride,
        UINT dst_stride, INT count, UINT freqAcc, UINT adj)
{
    while (count > 0)
    {
        DWORD *dest = dst;
        *dest = *(const DWORD *)src;

        dst = (char *)dst + dst_stride;
        src_advance(&src, src_stride, &count, &freqAcc, adj);
    }
}

const bitsconvertfunc convertbpp[4][4] = {
    { convert_8_to_8, convert_8_to_16, convert_8_to_24, convert_8_to_32 },
    { convert_16_to_8, convert_16_to_16, convert_16_to_24, convert_16_to_32 },
    { convert_24_to_8, convert_24_to_16, convert_24_to_24, convert_24_to_32 },
    { convert_32_to_8, convert_32_to_16, convert_32_to_24, convert_32_to_32 },
};

static void mix8(signed char *src, INT *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    while (len--)
        /* 8-bit WAV is unsigned, it's here converted to signed, normalize function will convert it back again */
        *(dst++) += (signed char)((BYTE)*(src++) - (BYTE)0x80);
}

static void mix16(SHORT *src, INT *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    len /= 2;
    while (len--)
    {
        *dst += le16(*src);
        ++dst; ++src;
    }
}

static void mix24(BYTE *src, INT *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    len /= 3;
    while (len--)
    {
        DWORD field;
        field = ((DWORD)src[2] << 16) + ((DWORD)src[1] << 8) + (DWORD)src[0];
        if (src[2] & 0x80)
            field |= 0xFF000000U;
        *(dst++) += field;
        ++src;
    }
}

static void mix32(INT *src, LONGLONG *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    len /= 4;
    while (len--)
        *(dst++) += le32(*(src++));
}

const mixfunc mixfunctions[4] = {
    (mixfunc)mix8,
    (mixfunc)mix16,
    (mixfunc)mix24,
    (mixfunc)mix32
};

static void norm8(INT *src, signed char *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    while (len--)
    {
        *dst = (*src) + 0x80;
        if (*src < -0x80)
            *dst = 0;
        else if (*src > 0x7f)
            *dst = 0xff;
        ++dst;
        ++src;
    }
}

static void norm16(INT *src, SHORT *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    len /= 2;
    while (len--)
    {
        *dst = le16(*src);
        if (*src <= -0x8000)
            *dst = le16(0x8000);
        else if (*src > 0x7fff)
            *dst = le16(0x7fff);
        ++dst;
        ++src;
    }
}

static void norm24(INT *src, BYTE *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    len /= 3;
    while (len--)
    {
        if (*src <= -0x800000)
        {
            dst[0] = 0;
            dst[1] = 0;
            dst[2] = 0x80;
        }
        else if (*src > 0x7fffff)
        {
            dst[0] = 0xff;
            dst[1] = 0xff;
            dst[2] = 0x7f;
        }
        else
        {
            dst[0] = *src;
            dst[1] = *src >> 8;
            dst[2] = *src >> 16;
        }
        ++dst;
        ++src;
    }
}

static void norm32(LONGLONG *src, INT *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    len /= 4;
    while (len--)
    {
        *dst = le32(*src);
        if (*src <= -(LONGLONG)0x80000000)
            *dst = le32(0x80000000);
        else if (*src > 0x7fffffff)
            *dst = le32(0x7fffffff);
        ++dst;
        ++src;
    }
}

const normfunc normfunctions[4] = {
    (normfunc)norm8,
    (normfunc)norm16,
    (normfunc)norm24,
    (normfunc)norm32,
};
