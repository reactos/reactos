/*
 * Fast486 386/486 CPU Emulation Library
 * fpu.c
 *
 * Copyright (C) 2015 Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* INCLUDES *******************************************************************/

#include <windef.h>

// #define NDEBUG
#include <debug.h>

#include <fast486.h>
#include "common.h"
#include "fpu.h"

/* CONSTANTS ******************************************************************/

/* 0.00 */
static const FAST486_FPU_DATA_REG FpuZero = {0ULL, 0, FALSE};

/* 1.00 */
static const FAST486_FPU_DATA_REG FpuOne = {0x8000000000000000ULL, FPU_REAL10_BIAS, FALSE};

/* Pi */
static const FAST486_FPU_DATA_REG FpuPi = {0xC90FDAA22168C235ULL, FPU_REAL10_BIAS + 1, FALSE};

/* lb(10) */
static const FAST486_FPU_DATA_REG FpuL2Ten = {0xD49A784BCD1B8AFEULL, FPU_REAL10_BIAS + 1, FALSE};

/* lb(e) */
static const FAST486_FPU_DATA_REG FpuL2E = {0xB8AA3B295C17F0BCULL, FPU_REAL10_BIAS, FALSE};

/* lg(2) */
static const FAST486_FPU_DATA_REG FpuLgTwo = {0x9A209A84FBCFF799ULL, FPU_REAL10_BIAS - 2, FALSE};

/* ln(2) */
static const FAST486_FPU_DATA_REG FpuLnTwo = {0xB17217F7D1CF79ACULL, FPU_REAL10_BIAS - 1, FALSE};

/* 2.00 */
static const FAST486_FPU_DATA_REG FpuTwo = {0x8000000000000000ULL, FPU_REAL10_BIAS + 1, FALSE};

/* Pi / 2 */
static const FAST486_FPU_DATA_REG FpuHalfPi = {0xC90FDAA22168C235ULL, FPU_REAL10_BIAS, FALSE};

static const FAST486_FPU_DATA_REG FpuInverseNumber[INVERSE_NUMBERS_COUNT] =
{
    {0x8000000000000000ULL, FPU_REAL10_BIAS,     FALSE}, /* 1 / 1 */
    {0x8000000000000000ULL, FPU_REAL10_BIAS - 1, FALSE}, /* 1 / 2 */
    {0xAAAAAAAAAAAAAAABULL, FPU_REAL10_BIAS - 2, FALSE}, /* 1 / 3 */
    {0x8000000000000000ULL, FPU_REAL10_BIAS - 2, FALSE}, /* 1 / 4 */
    {0xCCCCCCCCCCCCCCCDULL, FPU_REAL10_BIAS - 3, FALSE}, /* 1 / 5 */
    {0xAAAAAAAAAAAAAAAAULL, FPU_REAL10_BIAS - 3, FALSE}, /* 1 / 6 */
    {0x9249249249249249ULL, FPU_REAL10_BIAS - 3, FALSE}, /* 1 / 7 */
    {0x8000000000000000ULL, FPU_REAL10_BIAS - 3, FALSE}, /* 1 / 8 */
    {0xE38E38E38E38E38EULL, FPU_REAL10_BIAS - 4, FALSE}, /* 1 / 9 */
    {0xCCCCCCCCCCCCCCCDULL, FPU_REAL10_BIAS - 4, FALSE}, /* 1 / 10 */
    {0xBA2E8BA2E8BA2E8CULL, FPU_REAL10_BIAS - 4, FALSE}, /* 1 / 11 */
    {0xAAAAAAAAAAAAAAABULL, FPU_REAL10_BIAS - 4, FALSE}, /* 1 / 12 */
    {0x9D89D89D89D89D8AULL, FPU_REAL10_BIAS - 4, FALSE}, /* 1 / 13 */
    {0x9249249249249249ULL, FPU_REAL10_BIAS - 4, FALSE}, /* 1 / 14 */
    {0x8888888888888889ULL, FPU_REAL10_BIAS - 4, FALSE}, /* 1 / 15 */
    {0x8000000000000000ULL, FPU_REAL10_BIAS - 4, FALSE}, /* 1 / 16 */
    {0xF0F0F0F0F0F0F0F0ULL, FPU_REAL10_BIAS - 5, FALSE}, /* 1 / 17 */
    {0xE38E38E38E38E38EULL, FPU_REAL10_BIAS - 5, FALSE}, /* 1 / 18 */
    {0xD79435E50D79435EULL, FPU_REAL10_BIAS - 5, FALSE}, /* 1 / 19 */
    {0xCCCCCCCCCCCCCCCDULL, FPU_REAL10_BIAS - 5, FALSE}, /* 1 / 20 */
    {0xC30C30C30C30C30CULL, FPU_REAL10_BIAS - 5, FALSE}, /* 1 / 21 */
    {0xBA2E8BA2E8BA2E8BULL, FPU_REAL10_BIAS - 5, FALSE}, /* 1 / 22 */
    {0xB21642C8590B2164ULL, FPU_REAL10_BIAS - 5, FALSE}, /* 1 / 23 */
    {0xAAAAAAAAAAAAAAABULL, FPU_REAL10_BIAS - 5, FALSE}, /* 1 / 24 */
    {0xA3D70A3D70A3D70AULL, FPU_REAL10_BIAS - 5, FALSE}, /* 1 / 25 */
    {0x9D89D89D89D89D8AULL, FPU_REAL10_BIAS - 5, FALSE}, /* 1 / 26 */
    {0x97B425ED097B425FULL, FPU_REAL10_BIAS - 5, FALSE}, /* 1 / 27 */
    {0x9249249249249249ULL, FPU_REAL10_BIAS - 5, FALSE}, /* 1 / 28 */
    {0x8D3DCB08D3DCB08DULL, FPU_REAL10_BIAS - 5, FALSE}, /* 1 / 29 */
    {0x8888888888888889ULL, FPU_REAL10_BIAS - 5, FALSE}, /* 1 / 30 */
    {0x8421084210842108ULL, FPU_REAL10_BIAS - 5, FALSE}, /* 1 / 31 */
    {0x8000000000000000ULL, FPU_REAL10_BIAS - 5, FALSE}, /* 1 / 32 */
    {0xF83E0F83E0F83E0FULL, FPU_REAL10_BIAS - 6, FALSE}, /* 1 / 33 */
    {0xF0F0F0F0F0F0F0F0ULL, FPU_REAL10_BIAS - 6, FALSE}, /* 1 / 34 */
    {0xEA0EA0EA0EA0EA0EULL, FPU_REAL10_BIAS - 6, FALSE}, /* 1 / 35 */
    {0xE38E38E38E38E38EULL, FPU_REAL10_BIAS - 6, FALSE}, /* 1 / 36 */
    {0xDD67C8A60DD67C8AULL, FPU_REAL10_BIAS - 6, FALSE}, /* 1 / 37 */
    {0xD79435E50D79435EULL, FPU_REAL10_BIAS - 6, FALSE}, /* 1 / 38 */
    {0xD20D20D20D20D20DULL, FPU_REAL10_BIAS - 6, FALSE}, /* 1 / 39 */
    {0xCCCCCCCCCCCCCCCDULL, FPU_REAL10_BIAS - 6, FALSE}, /* 1 / 40 */
    {0xC7CE0C7CE0C7CE0CULL, FPU_REAL10_BIAS - 6, FALSE}, /* 1 / 41 */
    {0xC30C30C30C30C30CULL, FPU_REAL10_BIAS - 6, FALSE}, /* 1 / 42 */
    {0xBE82FA0BE82FA0BFULL, FPU_REAL10_BIAS - 6, FALSE}, /* 1 / 43 */
    {0xBA2E8BA2E8BA2E8BULL, FPU_REAL10_BIAS - 6, FALSE}, /* 1 / 44 */
    {0xB60B60B60B60B60BULL, FPU_REAL10_BIAS - 6, FALSE}, /* 1 / 45 */
    {0xB21642C8590B2164ULL, FPU_REAL10_BIAS - 6, FALSE}, /* 1 / 46 */
    {0xAE4C415C9882B931ULL, FPU_REAL10_BIAS - 6, FALSE}, /* 1 / 47 */
    {0xAAAAAAAAAAAAAAABULL, FPU_REAL10_BIAS - 6, FALSE}, /* 1 / 48 */
    {0xA72F05397829CBC1ULL, FPU_REAL10_BIAS - 6, FALSE}, /* 1 / 49 */
    {0xA3D70A3D70A3D70AULL, FPU_REAL10_BIAS - 6, FALSE}, /* 1 / 50 */
};

static const FAST486_FPU_DATA_REG FpuInverseNumberSine[INVERSE_NUMBERS_COUNT] =
{
    {0xAAAAAAAAAAAAAAAAULL, FPU_REAL10_BIAS - 3, FALSE}, /* 1 / 6 */
    {0xCCCCCCCCCCCCCCCCULL, FPU_REAL10_BIAS - 5, FALSE}, /* 1 / 20 */
    {0xC30C30C30C30C30CULL, FPU_REAL10_BIAS - 6, FALSE}, /* 1 / 42 */
    {0xE38E38E38E38E38EULL, FPU_REAL10_BIAS - 7, FALSE}, /* 1 / 72 */
    {0x94F2094F2094F209ULL, FPU_REAL10_BIAS - 7, FALSE}, /* 1 / 110 */
    {0xD20D20D20D20D20DULL, FPU_REAL10_BIAS - 8, FALSE}, /* 1 / 156 */
    {0x9C09C09C09C09C09ULL, FPU_REAL10_BIAS - 8, FALSE}, /* 1 / 210 */
    {0xF0F0F0F0F0F0F0F0ULL, FPU_REAL10_BIAS - 9, FALSE}, /* 1 / 272 */
    {0xBFA02FE80BFA02FEULL, FPU_REAL10_BIAS - 9, FALSE}, /* 1 / 342 */
    {0x9C09C09C09C09C09ULL, FPU_REAL10_BIAS - 9, FALSE}, /* 1 / 420 */
    {0x81848DA8FAF0D277ULL, FPU_REAL10_BIAS - 9, FALSE}, /* 1 / 506 */
    {0xDA740DA740DA740DULL, FPU_REAL10_BIAS - 10, FALSE}, /* 1 / 600 */
    {0xBAB656100BAB6561ULL, FPU_REAL10_BIAS - 10, FALSE}, /* 1 / 702 */
    {0xA16B312EA8FC377CULL, FPU_REAL10_BIAS - 10, FALSE}, /* 1 / 812 */
    {0x8CF008CF008CF008ULL, FPU_REAL10_BIAS - 10, FALSE}, /* 1 / 930 */
    {0xF83E0F83E0F83E0FULL, FPU_REAL10_BIAS - 11, FALSE}, /* 1 / 1056 */
    {0xDC4A00DC4A00DC4AULL, FPU_REAL10_BIAS - 11, FALSE}, /* 1 / 1190 */
    {0xC4CE07B00C4CE07BULL, FPU_REAL10_BIAS - 11, FALSE}, /* 1 / 1332 */
    {0xB0E2A2600B0E2A26ULL, FPU_REAL10_BIAS - 11, FALSE}, /* 1 / 1482 */
    {0x9FD809FD809FD809ULL, FPU_REAL10_BIAS - 11, FALSE}, /* 1 / 1640 */
    {0x9126D6E4802449B5ULL, FPU_REAL10_BIAS - 11, FALSE}, /* 1 / 1806 */
    {0x84655D9BAB2F1008ULL, FPU_REAL10_BIAS - 11, FALSE}, /* 1 / 1980 */
    {0xF2805AF0221A0CC9ULL, FPU_REAL10_BIAS - 12, FALSE}, /* 1 / 2162 */
    {0xDEE95C4CA037BA57ULL, FPU_REAL10_BIAS - 12, FALSE}, /* 1 / 2352 */
    {0xCD9A673400CD9A67ULL, FPU_REAL10_BIAS - 12, FALSE}, /* 1 / 2550 */
    {0xBE3C310B84A4F832ULL, FPU_REAL10_BIAS - 12, FALSE}, /* 1 / 2756 */
    {0xB087277A39941560ULL, FPU_REAL10_BIAS - 12, FALSE}, /* 1 / 2970 */
    {0xA44029100A440291ULL, FPU_REAL10_BIAS - 12, FALSE}, /* 1 / 3192 */
    {0x9936034AA9121AA1ULL, FPU_REAL10_BIAS - 12, FALSE}, /* 1 / 3422 */
    {0x8F3F82A86DACA008ULL, FPU_REAL10_BIAS - 12, FALSE}, /* 1 / 3660 */
    {0x8639F00218E7C008ULL, FPU_REAL10_BIAS - 12, FALSE}, /* 1 / 3906 */
    {0xFC0FC0FC0FC0FC0FULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 4160 */
    {0xED208916CF412FD1ULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 4422 */
    {0xDF7B4EC93886702DULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 4692 */
    {0xD2FB287C7224E167ULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 4970 */
    {0xC78031E00C78031EULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 5256 */
    {0xBCEEBFB33F021F2EULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 5550 */
    {0xB32EB86E96D5D441ULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 5852 */
    {0xAA2B0A62E08248F3ULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 6162 */
    {0xA1D139855F7268EDULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 6480 */
    {0x9A1100604AA03C2EULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 6806 */
    {0x92DC0092DC0092DCULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 7140 */
    {0x8C258008C258008CULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 7482 */
    {0x85E230A32BAB46DDULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 7832 */
    {0x8008008008008008ULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 8190 */
    {0xF51BE2CC2D7AAC94ULL, FPU_REAL10_BIAS - 14, FALSE}, /* 1 / 8556 */
    {0xEAD7EC46DDA80C62ULL, FPU_REAL10_BIAS - 14, FALSE}, /* 1 / 8930 */
    {0xE135A9C97500E135ULL, FPU_REAL10_BIAS - 14, FALSE}, /* 1 / 9312 */
    {0xD8281B71177BDB7BULL, FPU_REAL10_BIAS - 14, FALSE}, /* 1 / 9702 */
    {0xCFA3892CE9FFCC17ULL, FPU_REAL10_BIAS - 14, FALSE}, /* 1 / 10100 */
};

static const FAST486_FPU_DATA_REG FpuInverseNumberCosine[INVERSE_NUMBERS_COUNT] =
{
    {0x8000000000000000ULL, FPU_REAL10_BIAS - 1, FALSE}, /* 1 / 2 */
    {0xAAAAAAAAAAAAAAAAULL, FPU_REAL10_BIAS - 4, FALSE}, /* 1 / 12 */
    {0x8888888888888888ULL, FPU_REAL10_BIAS - 5, FALSE}, /* 1 / 30 */
    {0x9249249249249249ULL, FPU_REAL10_BIAS - 6, FALSE}, /* 1 / 56 */
    {0xB60B60B60B60B60BULL, FPU_REAL10_BIAS - 7, FALSE}, /* 1 / 90 */
    {0xF83E0F83E0F83E0FULL, FPU_REAL10_BIAS - 8, FALSE}, /* 1 / 132 */
    {0xB40B40B40B40B40BULL, FPU_REAL10_BIAS - 8, FALSE}, /* 1 / 182 */
    {0x8888888888888888ULL, FPU_REAL10_BIAS - 8, FALSE}, /* 1 / 240 */
    {0xD62B80D62B80D62BULL, FPU_REAL10_BIAS - 9, FALSE}, /* 1 / 306 */
    {0xAC7691840AC76918ULL, FPU_REAL10_BIAS - 9, FALSE}, /* 1 / 380 */
    {0x8DDA520237694808ULL, FPU_REAL10_BIAS - 9, FALSE}, /* 1 / 462 */
    {0xED7303B5CC0ED730ULL, FPU_REAL10_BIAS - 10, FALSE}, /* 1 / 552 */
    {0xC9A633FCD967300CULL, FPU_REAL10_BIAS - 10, FALSE}, /* 1 / 650 */
    {0xAD602B580AD602B5ULL, FPU_REAL10_BIAS - 10, FALSE}, /* 1 / 756 */
    {0x96A850096A850096ULL, FPU_REAL10_BIAS - 10, FALSE}, /* 1 / 870 */
    {0x8421084210842108ULL, FPU_REAL10_BIAS - 10, FALSE}, /* 1 / 992 */
    {0xE9A3D25E00E9A3D2ULL, FPU_REAL10_BIAS - 11, FALSE}, /* 1 / 1122 */
    {0xD00D00D00D00D00DULL, FPU_REAL10_BIAS - 11, FALSE}, /* 1 / 1260 */
    {0xBA7258200BA72582ULL, FPU_REAL10_BIAS - 11, FALSE}, /* 1 / 1406 */
    {0xA80A80A80A80A80AULL, FPU_REAL10_BIAS - 11, FALSE}, /* 1 / 1560 */
    {0x983B773A92E16009ULL, FPU_REAL10_BIAS - 11, FALSE}, /* 1 / 1722 */
    {0x8A8DCD1FEEAE465CULL, FPU_REAL10_BIAS - 11, FALSE}, /* 1 / 1892 */
    {0xFD477B6C956529CDULL, FPU_REAL10_BIAS - 12, FALSE}, /* 1 / 2070 */
    {0xE865AC7B7603A196ULL, FPU_REAL10_BIAS - 12, FALSE}, /* 1 / 2256 */
    {0xD5FEBF01E17D2DC4ULL, FPU_REAL10_BIAS - 12, FALSE}, /* 1 / 2450 */
    {0xC5B200C5B200C5B2ULL, FPU_REAL10_BIAS - 12, FALSE}, /* 1 / 2652 */
    {0xB7307B1492B1D28FULL, FPU_REAL10_BIAS - 12, FALSE}, /* 1 / 2862 */
    {0xAA392F35DC17F00AULL, FPU_REAL10_BIAS - 12, FALSE}, /* 1 / 3080 */
    {0x9E96394D47B46C68ULL, FPU_REAL10_BIAS - 12, FALSE}, /* 1 / 3306 */
    {0x941A9CC82BF7E68BULL, FPU_REAL10_BIAS - 12, FALSE}, /* 1 / 3540 */
    {0x8AA08EF5936D4008ULL, FPU_REAL10_BIAS - 12, FALSE}, /* 1 / 3782 */
    {0x8208208208208208ULL, FPU_REAL10_BIAS - 12, FALSE}, /* 1 / 4032 */
    {0xF46C5E0BB22F800FULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 4290 */
    {0xE6271BA5329217D3ULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 4556 */
    {0xD918B2EF5B7B4866ULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 4830 */
    {0xCD1ED923A7DCBEB2ULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 5112 */
    {0xC21BDD800C21BDD8ULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 5402 */
    {0xB7F5F08CD84C2BD5ULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 5700 */
    {0xAE968C517F46800AULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 6006 */
    {0xA5E9F6ED347F0721ULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 6320 */
    {0x9DDEDA75A1CD4726ULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 6642 */
    {0x9665EE14DB2283E4ULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 6972 */
    {0x8F71AD362448008FULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 7310 */
    {0x88F61A371B048C2BULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 7656 */
    {0x82E88A942AB2D933ULL, FPU_REAL10_BIAS - 13, FALSE}, /* 1 / 8010 */
    {0xFA7EF5D91AC9538AULL, FPU_REAL10_BIAS - 14, FALSE}, /* 1 / 8372 */
    {0xEFE4D31416B96CFEULL, FPU_REAL10_BIAS - 14, FALSE}, /* 1 / 8742 */
    {0xE5F36CB00E5F36CBULL, FPU_REAL10_BIAS - 14, FALSE}, /* 1 / 9120 */
    {0xDC9D0ECFCB6E9378ULL, FPU_REAL10_BIAS - 14, FALSE}, /* 1 / 9506 */
    {0xD3D56292AB7E800DULL, FPU_REAL10_BIAS - 14, FALSE}, /* 1 / 9900 */
};

/* PRIVATE FUNCTIONS **********************************************************/

#ifndef FAST486_NO_FPU

static ULONGLONG
UnsignedMult128(ULONGLONG Multiplicand,
                ULONGLONG Multiplier,
                ULONGLONG *HighProduct)
{
    ULONG MultiplicandLow, MultiplicandHigh, MultiplierLow, MultiplierHigh;
    ULONG IntermediateLow, IntermediateHigh;
    ULONGLONG LowProduct, Intermediate, Intermediate1, Intermediate2;

    MultiplicandLow = (ULONG)(Multiplicand & 0xFFFFFFFFULL);
    MultiplicandHigh = (ULONG)(Multiplicand >> 32);
    MultiplierLow = (ULONG)(Multiplier & 0xFFFFFFFFULL);
    MultiplierHigh = (ULONG)(Multiplier >> 32);

    LowProduct = (ULONGLONG)MultiplicandLow * (ULONGLONG)MultiplierLow;
    Intermediate1 = (ULONGLONG)MultiplicandLow * (ULONGLONG)MultiplierHigh;
    Intermediate2 = (ULONGLONG)MultiplicandHigh * (ULONGLONG)MultiplierLow;
    *HighProduct = (ULONGLONG)MultiplicandHigh * (ULONGLONG)MultiplierHigh;

    Intermediate = Intermediate1 + Intermediate2;
    if (Intermediate < Intermediate1) *HighProduct += 1ULL << 32;

    IntermediateLow = (ULONG)(Intermediate & 0xFFFFFFFFULL);
    IntermediateHigh = (ULONG)(Intermediate >> 32);

    LowProduct += (ULONGLONG)IntermediateLow << 32;
    if ((ULONG)(LowProduct >> 32) < IntermediateLow) (*HighProduct)++;

    *HighProduct += IntermediateHigh;
    return LowProduct;
}

static ULONGLONG
UnsignedDivMod128(ULONGLONG DividendLow,
                  ULONGLONG DividendHigh,
                  ULONGLONG Divisor,
                  PULONGLONG QuotientLow,
                  PULONGLONG QuotientHigh)
{
    ULONGLONG ValueLow = DividendLow;
    ULONGLONG ValueHigh = DividendHigh;
    ULONGLONG CurrentLow = 0ULL;
    ULONGLONG CurrentHigh = Divisor;
    ULONG Bits;

    ASSERT(Divisor != 0ULL);

    /* Initialize the quotient */
    *QuotientLow = *QuotientHigh = 0ULL;

    /* Exit early if the dividend is lower than the divisor */
    if ((DividendHigh == 0ULL) && (DividendLow < Divisor)) return ValueLow;

    /* Normalize the current divisor */
    Bits = CountLeadingZeros64(CurrentHigh);
    CurrentHigh <<= Bits;

    while (TRUE)
    {
        /* Shift the quotient left by one bit */
        *QuotientHigh <<= 1;
        *QuotientHigh |= *QuotientLow >> 63;
        *QuotientLow <<= 1;

        /* Check if the value is higher than or equal to the current divisor */
        if ((ValueHigh > CurrentHigh)
            || ((ValueHigh == CurrentHigh) && (ValueLow >= CurrentLow)))
        {
            BOOLEAN Carry = ValueLow < CurrentLow;

            /* Subtract the current divisor from the value */
            ValueHigh -= CurrentHigh;
            ValueLow -= CurrentLow;
            if (Carry) ValueHigh--;

            /* Set the lowest bit of the quotient */
            *QuotientLow |= 1;

            /* Stop if the value is lower than the original divisor */
            if ((ValueHigh == 0ULL) && (ValueLow < Divisor)) break;
        }

        /* Shift the current divisor right by one bit */
        CurrentLow >>= 1;
        CurrentLow |= (CurrentHigh & 1) << 63;
        CurrentHigh >>= 1;
    }

    /*
     * Calculate the number of significant bits the current
     * divisor has more than the original divisor
     */
    Bits = CountLeadingZeros64(Divisor);
    if (CurrentHigh > 0ULL) Bits += 64 - CountLeadingZeros64(CurrentHigh);
    else Bits -= CountLeadingZeros64(CurrentLow);

    if (Bits >= 64)
    {
        *QuotientHigh = *QuotientLow;
        *QuotientLow = 0ULL;
        Bits -= 64;
    }

    if (Bits)
    {
        /* Shift the quotient left by that amount */
        *QuotientHigh <<= Bits;
        *QuotientHigh |= *QuotientLow >> (64 - Bits);
        *QuotientLow <<= Bits;
    }

    /* Return the remainder */
    return ValueLow;
}

static inline VOID FASTCALL
Fast486FpuRound(PFAST486_STATE State,
                PULONGLONG Result,
                BOOLEAN Sign,
                ULONGLONG Remainder,
                INT RemainderHighBit)
{
    switch (State->FpuControl.Rc)
    {
        case FPU_ROUND_NEAREST:
        {
            /* Check if the highest bit of the remainder is set */
            if (Remainder & (1ULL << RemainderHighBit))
            {
                (*Result)++;

                /* Check if all the other bits are clear */
                if (!(Remainder & ((1ULL << RemainderHighBit) - 1ULL)))
                {
                    /* Round to even */
                    *Result &= ~1ULL;
                }
            }

            break;
        }

        case FPU_ROUND_DOWN:
        {
            if ((Remainder != 0ULL) && Sign) (*Result)++;
            break;
        }

        case FPU_ROUND_UP:
        {
            if ((Remainder != 0ULL) && !Sign) (*Result)++;
            break;
        }

        default:
        {
            /* Leave it truncated */
        }
    }
}

static inline VOID FASTCALL
Fast486FpuFromInteger(PFAST486_STATE State,
                      LONGLONG Value,
                      PFAST486_FPU_DATA_REG Result)
{
    ULONG ZeroCount;

    Result->Sign = Result->Exponent = Result->Mantissa = 0;
    if (Value == 0LL) return;

    if (Value < 0LL)
    {
        Result->Sign = TRUE;
        Value = -Value;
    }

    Result->Mantissa = (ULONGLONG)Value;
    ZeroCount = CountLeadingZeros64(Result->Mantissa);

    Result->Mantissa <<= ZeroCount;
    Result->Exponent = FPU_REAL10_BIAS + 63 - ZeroCount;
}

static inline BOOLEAN FASTCALL
Fast486FpuToInteger(PFAST486_STATE State,
                    PCFAST486_FPU_DATA_REG Value,
                    PLONGLONG Result)
{
    ULONG Bits;
    ULONGLONG Remainder;
    SHORT UnbiasedExp = (SHORT)Value->Exponent - FPU_REAL10_BIAS;

    if (FPU_IS_ZERO(Value))
    {
        *Result = 0LL;
        return TRUE;
    }

    if (FPU_IS_NAN(Value) || !FPU_IS_NORMALIZED(Value) || (UnbiasedExp >= 63))
    {
        /* Raise an invalid operation exception */
        State->FpuStatus.Ie = TRUE;

        if (State->FpuControl.Im)
        {
            *Result = 0x8000000000000000LL;
            return TRUE;
        }
        else
        {
            Fast486FpuException(State);
            return FALSE;
        }
    }

    if (UnbiasedExp >= 0)
    {
        Bits = 63 - UnbiasedExp;

        /* Calculate the result and the remainder */
        *Result = (LONGLONG)(Value->Mantissa >> Bits);
        Remainder = Value->Mantissa & ((1ULL << Bits) - 1);
    }
    else
    {
        /* The result is zero */
        *Result = 0LL;
        Bits = 64;

        if (UnbiasedExp >= -64)
        {
            Remainder = Value->Mantissa >> (-1 - UnbiasedExp);
        }
        else
        {
            /* Too small to even have a remainder */
            Remainder = 0ULL;
        }
    }

    /* The result must be positive here */
    ASSERT(*Result >= 0LL);

    /* Perform rounding */
    Fast486FpuRound(State, (PULONGLONG)Result, Value->Sign, Remainder, Bits - 1);

    if (Value->Sign) *Result = -*Result;
    return TRUE;
}

static inline VOID FASTCALL
Fast486FpuFromSingleReal(PFAST486_STATE State,
                         ULONG Value,
                         PFAST486_FPU_DATA_REG Result)
{
    /* Extract the sign, exponent and mantissa */
    Result->Sign = (UCHAR)(Value >> 31);
    Result->Exponent = (USHORT)((Value >> 23) & 0xFF);
    Result->Mantissa = (((ULONGLONG)Value & 0x7FFFFFULL) | 0x800000ULL) << 40;

    /* If this is a zero, we're done */
    if (Value == 0) return;

    if (Result->Exponent == 0xFF) Result->Exponent = FPU_MAX_EXPONENT + 1;
    else
    {
        /* Adjust the exponent bias */
        Result->Exponent += (FPU_REAL10_BIAS - FPU_REAL4_BIAS);
    }
}

static inline BOOLEAN FASTCALL
Fast486FpuToSingleReal(PFAST486_STATE State,
                       PCFAST486_FPU_DATA_REG Value,
                       PULONG Result)
{
    ULONGLONG Remainder;
    SHORT UnbiasedExp = (SHORT)Value->Exponent - FPU_REAL10_BIAS;
    ULONGLONG Result64;

    if (FPU_IS_ZERO(Value))
    {
        *Result = 0;
        return TRUE;
    }

    /* Calculate the mantissa */
    *Result = (ULONG)(Value->Mantissa >> 40) & 0x7FFFFF;

    if (FPU_IS_NAN(Value))
    {
        *Result |= FPU_REAL4_INFINITY;
        goto SetSign;
    }

    /* Check for underflow */
    if (!FPU_IS_NORMALIZED(Value) || (UnbiasedExp < -127))
    {
        /* Raise the underflow exception */
        State->FpuStatus.Ue = TRUE;

        if (State->FpuControl.Um)
        {
            /* The result is zero due to underflow */
            *Result = 0ULL;
            return TRUE;
        }
        else
        {
            Fast486FpuException(State);
            return FALSE;
        }
    }

    /* Check for overflow */
    if (UnbiasedExp > 127)
    {
        /* Raise the overflow exception */
        State->FpuStatus.Oe = TRUE;

        if (State->FpuControl.Om)
        {
            /* The result is infinity due to overflow */
            *Result = FPU_REAL4_INFINITY;
            goto SetSign;
        }
        else
        {
            Fast486FpuException(State);
            return FALSE;
        }
    }

    /* Calculate the remainder */
    Remainder = Value->Mantissa & ((1ULL << 40) - 1);

    /* Store the biased exponent */
    *Result |= (ULONG)(UnbiasedExp + FPU_REAL4_BIAS) << 23;

    /* Perform rounding */
    Result64 = (ULONGLONG)*Result;
    Fast486FpuRound(State, &Result64, Value->Sign, Remainder, 39);
    *Result = (ULONG)Result64;

SetSign:

    if (Value->Sign) *Result |= 0x80000000;
    return TRUE;
}

static inline VOID FASTCALL
Fast486FpuFromDoubleReal(PFAST486_STATE State,
                         ULONGLONG Value,
                         PFAST486_FPU_DATA_REG Result)
{
    /* Extract the sign, exponent and mantissa */
    Result->Sign = (UCHAR)(Value >> 63);
    Result->Exponent = (USHORT)((Value >> 52) & 0x7FF);
    Result->Mantissa = (((ULONGLONG)Value & 0xFFFFFFFFFFFFFULL) | 0x10000000000000ULL) << 11;

    /* If this is a zero, we're done */
    if (Value == 0) return;

    if (Result->Exponent == 0x7FF) Result->Exponent = FPU_MAX_EXPONENT + 1;
    else
    {
        /* Adjust the exponent bias */
        Result->Exponent += (FPU_REAL10_BIAS - FPU_REAL8_BIAS);
    }
}

static inline BOOLEAN FASTCALL
Fast486FpuToDoubleReal(PFAST486_STATE State,
                       PCFAST486_FPU_DATA_REG Value,
                       PULONGLONG Result)
{
    ULONGLONG Remainder;
    SHORT UnbiasedExp = (SHORT)Value->Exponent - FPU_REAL10_BIAS;

    if (FPU_IS_ZERO(Value))
    {
        *Result = 0LL;
        return TRUE;
    }

    /* Calculate the mantissa */
    *Result = (Value->Mantissa >> 11) & ((1ULL << 52) - 1);

    if (FPU_IS_NAN(Value))
    {
        *Result |= FPU_REAL8_INFINITY;
        goto SetSign;
    }

    /* Check for underflow */
    if (!FPU_IS_NORMALIZED(Value) || (UnbiasedExp < -1023))
    {
        /* Raise the underflow exception */
        State->FpuStatus.Ue = TRUE;

        if (State->FpuControl.Um)
        {
            /* The result is zero due to underflow */
            *Result = 0ULL;
            return TRUE;
        }
        else
        {
            Fast486FpuException(State);
            return FALSE;
        }
    }

    /* Check for overflow */
    if (UnbiasedExp > 1023)
    {
        /* Raise the overflow exception */
        State->FpuStatus.Oe = TRUE;

        if (State->FpuControl.Om)
        {
            /* The result is infinity due to overflow */
            *Result = FPU_REAL8_INFINITY;
            goto SetSign;
        }
        else
        {
            Fast486FpuException(State);
            return FALSE;
        }
    }

    /* Calculate the remainder */
    Remainder = Value->Mantissa & ((1ULL << 11) - 1ULL);

    /* Store the biased exponent */
    *Result |= (ULONGLONG)(UnbiasedExp + FPU_REAL8_BIAS) << 52;

    /* Perform rounding */
    Fast486FpuRound(State, Result, Value->Sign, Remainder, 10);

SetSign:

    if (Value->Sign) *Result |= 1ULL << 63;
    return TRUE;
}

static inline VOID FASTCALL
Fast486FpuFromPackedBcd(PFAST486_STATE State,
                        PUCHAR Value,
                        PFAST486_FPU_DATA_REG Result)
{
    INT i;
    LONGLONG IntVal = 0LL;

    for (i = 8; i >= 0; i--)
    {
        IntVal *= 100LL;
        IntVal += (Value[i] >> 4) * 10 + (Value[i] & 0x0F);
    }

    /* Apply the sign */
    if (Value[9] & 0x80) IntVal = -IntVal;

    /* Now convert the integer to FP80 */
    Fast486FpuFromInteger(State, IntVal, Result);
}

static inline BOOLEAN FASTCALL
Fast486FpuToPackedBcd(PFAST486_STATE State,
                      PCFAST486_FPU_DATA_REG Value,
                      PUCHAR Result)
{
    INT i;
    LONGLONG IntVal;

    /* Convert it to an integer first */
    if (!Fast486FpuToInteger(State, Value, &IntVal)) return FALSE;

    if (IntVal < 0LL)
    {
        IntVal = -IntVal;
        Result[9] = 0x80;
    }

    for (i = 0; i < 9; i++)
    {
        Result[i] = (UCHAR)((IntVal % 10) + (((IntVal / 10) % 10) << 4));
        IntVal /= 100LL;
    }

    return TRUE;
}

static inline BOOLEAN FASTCALL
Fast486FpuAdd(PFAST486_STATE State,
              PCFAST486_FPU_DATA_REG FirstOperand,
              PCFAST486_FPU_DATA_REG SecondOperand,
              PFAST486_FPU_DATA_REG Result)
{
    FAST486_FPU_DATA_REG FirstAdjusted = *FirstOperand;
    FAST486_FPU_DATA_REG SecondAdjusted = *SecondOperand;
    FAST486_FPU_DATA_REG TempResult;

    if (FPU_IS_INDEFINITE(FirstOperand)
        || FPU_IS_INDEFINITE(SecondOperand)
        || (FPU_IS_POS_INF(FirstOperand) && FPU_IS_NEG_INF(SecondOperand))
        || (FPU_IS_NEG_INF(FirstOperand) && FPU_IS_POS_INF(SecondOperand)))
    {
        /* The result will be indefinite */
        Result->Sign = TRUE;
        Result->Exponent = FPU_MAX_EXPONENT + 1;
        Result->Mantissa = FPU_INDEFINITE_MANTISSA;
        return TRUE;
    }

    if ((!FPU_IS_NORMALIZED(FirstOperand) || !FPU_IS_NORMALIZED(SecondOperand)))
    {
        /* Raise the denormalized exception */
        State->FpuStatus.De = TRUE;

        if (!State->FpuControl.Dm)
        {
            Fast486FpuException(State);
            return FALSE;
        }
    }

    if (FPU_IS_ZERO(FirstOperand) || FPU_IS_INFINITY(SecondOperand))
    {
        /* The second operand is the result */
        *Result = *SecondOperand;
        return TRUE;
    }

    if (FPU_IS_ZERO(SecondOperand) || FPU_IS_INFINITY(FirstOperand))
    {
        /* The first operand is the result */
        *Result = *FirstOperand;
        return TRUE;
    }

    /* Find the largest exponent */
    TempResult.Exponent = max(FirstOperand->Exponent, SecondOperand->Exponent);

    /* Adjust the first operand to it... */
    if (FirstAdjusted.Exponent < TempResult.Exponent)
    {
        if ((TempResult.Exponent - FirstAdjusted.Exponent) < 64)
        {
            FirstAdjusted.Mantissa >>= (TempResult.Exponent - FirstAdjusted.Exponent);
            FirstAdjusted.Exponent = TempResult.Exponent;
        }
        else
        {
            /* The second operand is the result */
            *Result = *SecondOperand;
            return TRUE;
        }
    }

    /* ... and the second one too */
    if (SecondAdjusted.Exponent < TempResult.Exponent)
    {
        if ((TempResult.Exponent - SecondAdjusted.Exponent) < 64)
        {
            SecondAdjusted.Mantissa >>= (TempResult.Exponent - SecondAdjusted.Exponent);
            SecondAdjusted.Exponent = TempResult.Exponent;
        }
        else
        {
            /* The first operand is the result */
            *Result = *FirstOperand;
            return TRUE;
        }
    }

    if (FirstAdjusted.Sign == SecondAdjusted.Sign)
    {
        /* Calculate the mantissa and sign of the result */
        TempResult.Mantissa = FirstAdjusted.Mantissa + SecondAdjusted.Mantissa;
        TempResult.Sign = FirstAdjusted.Sign;
    }
    else
    {
        /* Calculate the sign of the result */
        if (FirstAdjusted.Mantissa > SecondAdjusted.Mantissa) TempResult.Sign = FirstAdjusted.Sign;
        else if (FirstAdjusted.Mantissa < SecondAdjusted.Mantissa) TempResult.Sign = SecondAdjusted.Sign;
        else TempResult.Sign = FALSE;

        /* Invert the negative mantissa */
        if (FirstAdjusted.Sign) FirstAdjusted.Mantissa = -(LONGLONG)FirstAdjusted.Mantissa;
        if (SecondAdjusted.Sign) SecondAdjusted.Mantissa = -(LONGLONG)SecondAdjusted.Mantissa;

        /* Calculate the mantissa of the result */
        TempResult.Mantissa = FirstAdjusted.Mantissa + SecondAdjusted.Mantissa;
    }

    /* Did it overflow? */
    if (FirstAdjusted.Sign == SecondAdjusted.Sign)
    {
        if (TempResult.Mantissa < FirstAdjusted.Mantissa
            || TempResult.Mantissa < SecondAdjusted.Mantissa)
        {
            if (TempResult.Exponent == FPU_MAX_EXPONENT)
            {
                /* Raise the overflow exception */
                State->FpuStatus.Oe = TRUE;

                if (State->FpuControl.Om)
                {
                    /* Total overflow, return infinity */
                    TempResult.Mantissa = FPU_MANTISSA_HIGH_BIT;
                    TempResult.Exponent = FPU_MAX_EXPONENT + 1;
                }
                else
                {
                    Fast486FpuException(State);
                    return FALSE;
                }
            }
            else
            {
                /* Lose the LSB in favor of the carry */
                TempResult.Mantissa >>= 1;
                TempResult.Mantissa |= FPU_MANTISSA_HIGH_BIT;
                TempResult.Exponent++;
            }
        }
    }
    else
    {
        if (TempResult.Mantissa >= FirstAdjusted.Mantissa
            && TempResult.Mantissa >= SecondAdjusted.Mantissa)
        {
            /* Reverse the mantissa */
            TempResult.Mantissa = -(LONGLONG)TempResult.Mantissa;
        }
    }

    /* Normalize the result and return it */
    if (!Fast486FpuNormalize(State, &TempResult))
    {
        /* Exception occurred */
        return FALSE;
    }

    *Result = TempResult;
    return TRUE;
}

static inline BOOLEAN FASTCALL
Fast486FpuSubtract(PFAST486_STATE State,
                   PCFAST486_FPU_DATA_REG FirstOperand,
                   PCFAST486_FPU_DATA_REG SecondOperand,
                   PFAST486_FPU_DATA_REG Result)
{
    FAST486_FPU_DATA_REG NegativeSecondOperand = *SecondOperand;

    /* Invert the sign */
    NegativeSecondOperand.Sign = !NegativeSecondOperand.Sign;

    /* And perform an addition instead */
    return Fast486FpuAdd(State, FirstOperand, &NegativeSecondOperand, Result);
}

static inline VOID FASTCALL
Fast486FpuCompare(PFAST486_STATE State,
                  PCFAST486_FPU_DATA_REG FirstOperand,
                  PCFAST486_FPU_DATA_REG SecondOperand)
{
    if (FPU_IS_NAN(FirstOperand) || FPU_IS_NAN(SecondOperand))
    {
        if ((FPU_IS_POS_INF(FirstOperand)
            && (!FPU_IS_NAN(SecondOperand) || FPU_IS_NEG_INF(SecondOperand)))
            || (!FPU_IS_NAN(FirstOperand) && FPU_IS_NEG_INF(SecondOperand)))
        {
            State->FpuStatus.Code0 = FALSE;
            State->FpuStatus.Code2 = FALSE;
            State->FpuStatus.Code3 = FALSE;
        }
        else if ((FPU_IS_POS_INF(SecondOperand)
                 && (!FPU_IS_NAN(FirstOperand) || FPU_IS_NEG_INF(FirstOperand)))
                 || (!FPU_IS_NAN(SecondOperand) && FPU_IS_NEG_INF(FirstOperand)))
        {
            State->FpuStatus.Code0 = TRUE;
            State->FpuStatus.Code2 = FALSE;
            State->FpuStatus.Code3 = FALSE;
        }
        else
        {
            State->FpuStatus.Code0 = TRUE;
            State->FpuStatus.Code2 = TRUE;
            State->FpuStatus.Code3 = TRUE;
        }
    }
    else
    {
        FAST486_FPU_DATA_REG TempResult;

        Fast486FpuSubtract(State, FirstOperand, SecondOperand, &TempResult);

        if (FPU_IS_ZERO(&TempResult))
        {
            State->FpuStatus.Code0 = FALSE;
            State->FpuStatus.Code2 = FALSE;
            State->FpuStatus.Code3 = TRUE;
        }
        else if (TempResult.Sign)
        {
            State->FpuStatus.Code0 = TRUE;
            State->FpuStatus.Code2 = FALSE;
            State->FpuStatus.Code3 = FALSE;
        }
        else
        {
            State->FpuStatus.Code0 = FALSE;
            State->FpuStatus.Code2 = FALSE;
            State->FpuStatus.Code3 = FALSE;
        }
    }
}

static inline BOOLEAN FASTCALL
Fast486FpuMultiply(PFAST486_STATE State,
                   PCFAST486_FPU_DATA_REG FirstOperand,
                   PCFAST486_FPU_DATA_REG SecondOperand,
                   PFAST486_FPU_DATA_REG Result)
{
    FAST486_FPU_DATA_REG TempResult;
    LONG Exponent;

    if (FPU_IS_INDEFINITE(FirstOperand)
        || FPU_IS_INDEFINITE(SecondOperand)
        || (FPU_IS_ZERO(FirstOperand) && FPU_IS_INFINITY(SecondOperand))
        || (FPU_IS_INFINITY(FirstOperand) && FPU_IS_ZERO(SecondOperand)))
    {
        /* The result will be indefinite */
        Result->Sign = TRUE;
        Result->Exponent = FPU_MAX_EXPONENT + 1;
        Result->Mantissa = FPU_INDEFINITE_MANTISSA;
        return TRUE;
    }

    if (FPU_IS_ZERO(FirstOperand) || FPU_IS_ZERO(SecondOperand))
    {
        /* The result will be zero */
        Result->Sign = FirstOperand->Sign ^ SecondOperand->Sign;
        Result->Exponent = 0;
        Result->Mantissa = 0ULL;
        return TRUE;
    }

    if (FPU_IS_INFINITY(FirstOperand) || FPU_IS_INFINITY(SecondOperand))
    {
        /* The result will be infinity */
        Result->Sign = FirstOperand->Sign ^ SecondOperand->Sign;
        Result->Exponent = FPU_MAX_EXPONENT + 1;
        Result->Mantissa = FPU_MANTISSA_HIGH_BIT;
        return TRUE;
    }

    if ((!FPU_IS_NORMALIZED(FirstOperand) || !FPU_IS_NORMALIZED(SecondOperand)))
    {
        /* Raise the denormalized exception */
        State->FpuStatus.De = TRUE;

        if (!State->FpuControl.Dm)
        {
            Fast486FpuException(State);
            return FALSE;
        }
    }

    /* Calculate the sign */
    TempResult.Sign = FirstOperand->Sign ^ SecondOperand->Sign;

    /* Calculate the exponent */
    Exponent = (LONG)FirstOperand->Exponent + (LONG)SecondOperand->Exponent - FPU_REAL10_BIAS + 1;

    /* Calculate the mantissa */
    UnsignedMult128(FirstOperand->Mantissa,
                    SecondOperand->Mantissa,
                    &TempResult.Mantissa);

    if (Exponent < 0)
    {
        /* Raise the underflow exception */
        State->FpuStatus.Ue = TRUE;

        if (!State->FpuControl.Um)
        {
            Fast486FpuException(State);
            return FALSE;
        }

        /* The exponent will be zero */
        TempResult.Exponent = 0;

        /* If possible, denormalize the result, otherwise make it zero */
        if (Exponent > -64) TempResult.Mantissa >>= (-Exponent);
        else TempResult.Mantissa = 0ULL;
    }
    else if (Exponent > FPU_MAX_EXPONENT)
    {
        /* Raise the overflow exception */
        State->FpuStatus.Oe = TRUE;

        if (!State->FpuControl.Om)
        {
            Fast486FpuException(State);
            return FALSE;
        }

        /* Make the result infinity */
        TempResult.Exponent = FPU_MAX_EXPONENT + 1;
        TempResult.Mantissa = FPU_MANTISSA_HIGH_BIT;
    }
    else TempResult.Exponent = (USHORT)Exponent;

    /* Normalize the result */
    if (!Fast486FpuNormalize(State, &TempResult))
    {
        /* Exception occurred */
        return FALSE;
    }

    *Result = TempResult;
    return TRUE;
}

static inline BOOLEAN FASTCALL
Fast486FpuDivide(PFAST486_STATE State,
                 PCFAST486_FPU_DATA_REG FirstOperand,
                 PCFAST486_FPU_DATA_REG SecondOperand,
                 PFAST486_FPU_DATA_REG Result)
{
    FAST486_FPU_DATA_REG TempResult;
    ULONGLONG QuotientLow, QuotientHigh, Remainder;
    LONG Exponent;

    if (FPU_IS_INDEFINITE(FirstOperand)
        || FPU_IS_INDEFINITE(SecondOperand)
        || (FPU_IS_INFINITY(FirstOperand) && FPU_IS_INFINITY(SecondOperand))
        || (FPU_IS_ZERO(FirstOperand) && FPU_IS_ZERO(SecondOperand)))
    {
        /* Raise the invalid operation exception */
        State->FpuStatus.Ie = TRUE;

        if (State->FpuControl.Im)
        {
            /* Return the indefinite NaN */
            Result->Sign = TRUE;
            Result->Exponent = FPU_MAX_EXPONENT + 1;
            Result->Mantissa = FPU_INDEFINITE_MANTISSA;
            return TRUE;
        }
        else
        {
            Fast486FpuException(State);
            return FALSE;
        }
    }

    if (FPU_IS_ZERO(SecondOperand) || FPU_IS_INFINITY(FirstOperand))
    {
        /* Raise the division by zero exception */
        State->FpuStatus.Ze = TRUE;

        if (State->FpuControl.Zm)
        {
            /* Return infinity */
            Result->Sign = FirstOperand->Sign;
            Result->Exponent = FPU_MAX_EXPONENT + 1;
            Result->Mantissa = FPU_MANTISSA_HIGH_BIT;
            return TRUE;
        }
        else
        {
            Fast486FpuException(State);
            return FALSE;
        }
    }

    /* Calculate the sign of the result */
    TempResult.Sign = FirstOperand->Sign ^ SecondOperand->Sign;

    if (FPU_IS_ZERO(FirstOperand) || FPU_IS_INFINITY(SecondOperand))
    {
        /* Return zero */
        Result->Sign = TempResult.Sign;
        Result->Mantissa = 0ULL;
        Result->Exponent = 0;
        return TRUE;
    }

    /* Calculate the exponent of the result */
    Exponent = (LONG)FirstOperand->Exponent - (LONG)SecondOperand->Exponent - 1;

    /* Divide the two mantissas */
    Remainder = UnsignedDivMod128(0ULL,
                                  FirstOperand->Mantissa,
                                  SecondOperand->Mantissa,
                                  &QuotientLow,
                                  &QuotientHigh);
    UNREFERENCED_PARAMETER(Remainder); // TODO: Rounding

    TempResult.Mantissa = QuotientLow;

    if (QuotientHigh > 0ULL)
    {
        ULONG BitsToShift = 64 - CountLeadingZeros64(QuotientHigh);

        TempResult.Mantissa >>= BitsToShift;
        TempResult.Mantissa |= QuotientHigh << (64 - BitsToShift);
        Exponent += BitsToShift;

        // TODO: Rounding
    }

    if (Exponent < -FPU_REAL10_BIAS)
    {
        TempResult.Mantissa >>= -(Exponent + FPU_REAL10_BIAS);
        Exponent = -FPU_REAL10_BIAS;

        // TODO: Rounding
    }

    TempResult.Exponent = (USHORT)(Exponent + FPU_REAL10_BIAS);

    /* Normalize the result */
    if (!Fast486FpuNormalize(State, &TempResult))
    {
        /* Exception occurred */
        return FALSE;
    }

    *Result = TempResult;
    return TRUE;
}

/*
 * Calculates using the identity:
 * 2 ^ x - 1 = 1 + sum { 2 * (((x - 1) * ln(2)) ^ n) / n! }
 */
static inline VOID FASTCALL
Fast486FpuCalculateTwoPowerMinusOne(PFAST486_STATE State,
                                    PFAST486_FPU_DATA_REG Operand,
                                    PFAST486_FPU_DATA_REG Result)
{
    INT i;
    FAST486_FPU_DATA_REG TempResult = FpuOne;
    FAST486_FPU_DATA_REG Value;
    FAST486_FPU_DATA_REG SeriesElement;

    /* Calculate the first series element, which is 2 * (x - 1) * ln(2) */
    if (!Fast486FpuSubtract(State, Operand, &FpuOne, &Value)) return;
    if (!Fast486FpuMultiply(State, &Value, &FpuLnTwo, &Value)) return;
    if (!Fast486FpuAdd(State, &Value, &Value, &SeriesElement)) return;

    for (i = 2; i <= INVERSE_NUMBERS_COUNT; i++)
    {
        /* Add the series element to the final sum */
        if (!Fast486FpuAdd(State, &TempResult, &SeriesElement, &TempResult))
        {
            /* An exception occurred */
            return;
        }

        /*
         * Calculate the next series element (partially) by multiplying
         * it with (x - 1) * ln(2)
         */
        if (!Fast486FpuMultiply(State, &SeriesElement, &Value, &SeriesElement))
        {
            /* An exception occurred */
            return;
        }

        /* And now multiply the series element by the inverse counter */
        if (!Fast486FpuMultiply(State,
                                &SeriesElement,
                                &FpuInverseNumber[i - 1],
                                &SeriesElement))
        {
            /* An exception occurred */
            return;
        }
    }

    *Result = TempResult;
}

static inline BOOLEAN FASTCALL
Fast486FpuCalculateLogBase2(PFAST486_STATE State,
                            PFAST486_FPU_DATA_REG Operand,
                            PFAST486_FPU_DATA_REG Result)
{
    INT i;
    FAST486_FPU_DATA_REG Value = *Operand;
    FAST486_FPU_DATA_REG TempResult;
    FAST486_FPU_DATA_REG TempValue;
    LONGLONG UnbiasedExp = (LONGLONG)Operand->Exponent - FPU_REAL10_BIAS;

    if (Operand->Sign)
    {
        /* Raise the invalid operation exception */
        State->FpuStatus.Ie = TRUE;

        if (State->FpuControl.Im)
        {
            /* Return the indefinite NaN */
            Result->Sign = TRUE;
            Result->Exponent = FPU_MAX_EXPONENT + 1;
            Result->Mantissa = FPU_INDEFINITE_MANTISSA;
            return TRUE;
        }
        else
        {
            Fast486FpuException(State);
            return FALSE;
        }
    }

    /* Get only the mantissa as a floating-pointer number between 1 and 2 */
    Value.Exponent = FPU_REAL10_BIAS;

    /* Check if it's denormalized */
    if (!FPU_IS_NORMALIZED(&Value))
    {
        ULONG Bits;
        State->FpuStatus.De = TRUE;

        if (!State->FpuControl.Dm)
        {
            Fast486FpuException(State);
            return FALSE;
        }

        /* Normalize the number */
        Bits = CountLeadingZeros64(Value.Mantissa);
        UnbiasedExp -= Bits;
        Value.Mantissa <<= Bits;
    }

    TempResult.Sign = FALSE;
    TempResult.Exponent = FPU_REAL10_BIAS - 1;
    TempResult.Mantissa = 0ULL;

    for (i = 63; i >= 0; i--)
    {
        /* Square the value */
        if (!Fast486FpuMultiply(State, &Value, &Value, &Value)) return FALSE;

        /* Subtract two from it */
        if (!Fast486FpuSubtract(State, &Value, &FpuTwo, &TempValue)) return FALSE;

        /* Is the result positive? */
        if (!TempValue.Sign)
        {
            /* Yes, set the appropriate bit in the mantissa */
            TempResult.Mantissa |= 1ULL << i;

            /* Halve the value */
            if (!Fast486FpuMultiply(State, &Value, &FpuInverseNumber[1], &Value)) return FALSE;
        }
    }

    /* Normalize the result */
    if (!Fast486FpuNormalize(State, &TempResult)) return FALSE;

    /*
     * Add the exponent to the result
     * log2(x * 2^y) = log2(x) + log2(2^y) = log2(x) + y
     */
    Fast486FpuFromInteger(State, UnbiasedExp, &TempValue);
    if (!Fast486FpuAdd(State, &TempValue, &TempResult, &TempResult)) return FALSE;

    *Result = TempResult;
    return TRUE;
}

static inline BOOLEAN FASTCALL
Fast486FpuRemainder(PFAST486_STATE State,
                    PCFAST486_FPU_DATA_REG FirstOperand,
                    PCFAST486_FPU_DATA_REG SecondOperand,
                    BOOLEAN RoundToNearest,
                    PFAST486_FPU_DATA_REG Result,
                    PLONGLONG Quotient OPTIONAL)
{
    BOOLEAN Success = FALSE;
    INT OldRoundingMode = State->FpuControl.Rc;
    LONGLONG Integer;
    FAST486_FPU_DATA_REG Temp;

    if (!Fast486FpuDivide(State, FirstOperand, SecondOperand, &Temp)) return FALSE;

    State->FpuControl.Rc = RoundToNearest ? FPU_ROUND_NEAREST : FPU_ROUND_TRUNCATE;

    if (!Fast486FpuToInteger(State, &Temp, &Integer)) goto Cleanup;
    Fast486FpuFromInteger(State, Integer, &Temp);

    if (!Fast486FpuMultiply(State, &Temp, SecondOperand, &Temp)) goto Cleanup;
    if (!Fast486FpuSubtract(State, FirstOperand, &Temp, Result)) goto Cleanup;

    if (Quotient) *Quotient = Integer;
    Success = TRUE;

Cleanup:
    State->FpuControl.Rc = OldRoundingMode;
    return Success;
}

/*
 * Calculates using the identity:
 * sin(x) = sum { -1^n * x^(2n + 1) / (2n + 1)!, n >= 0 }
 */
static inline BOOLEAN FASTCALL
Fast486FpuCalculateSine(PFAST486_STATE State,
                        PFAST486_FPU_DATA_REG Operand,
                        PFAST486_FPU_DATA_REG Result)
{
    INT i;
    ULONGLONG Quadrant;
    FAST486_FPU_DATA_REG Normalized = *Operand;
    FAST486_FPU_DATA_REG TempResult;
    FAST486_FPU_DATA_REG OperandSquared;
    FAST486_FPU_DATA_REG SeriesElement;
    PCFAST486_FPU_DATA_REG Inverse;

    if (!Fast486FpuRemainder(State,
                             Operand,
                             &FpuHalfPi,
                             FALSE,
                             &Normalized,
                             (PLONGLONG)&Quadrant))
    {
        return FALSE;
    }

    /* Normalize the quadrant number */
    Quadrant &= 3;

    if (!(Quadrant & 1))
    {
        /* This is a sine */
        Inverse = FpuInverseNumberSine;
        TempResult = SeriesElement = Normalized;
    }
    else
    {
        /* This is a cosine */
        Inverse = FpuInverseNumberCosine;
        TempResult = SeriesElement = FpuOne;
    }

    /* Calculate the square of the operand */
    if (!Fast486FpuMultiply(State, &Normalized, &Normalized, &OperandSquared)) return FALSE;

    for (i = 0; i < INVERSE_NUMBERS_COUNT; i++)
    {
        if (!Fast486FpuMultiply(State, &SeriesElement, &OperandSquared, &SeriesElement))
        {
            /* An exception occurred */
            return FALSE;
        }

        if (!Fast486FpuMultiply(State,
                                &SeriesElement,
                                &Inverse[i],
                                &SeriesElement))
        {
            /* An exception occurred */
            return FALSE;
        }

        /* Toggle the sign of the series element */
        SeriesElement.Sign = !SeriesElement.Sign;

        if (!Fast486FpuAdd(State, &TempResult, &SeriesElement, &TempResult))
        {
            /* An exception occurred */
            return FALSE;
        }
    }

    /* Flip the sign for the third and fourth quadrant */
    if (Quadrant >= 2) TempResult.Sign = !TempResult.Sign;

    *Result = TempResult;
    return TRUE;
}

/*
 * Calculates using the identity:
 * cos(x) = sum { -1^n * x^(2n) / (2n)!, n >= 0 }
 */
static inline BOOLEAN FASTCALL
Fast486FpuCalculateCosine(PFAST486_STATE State,
                          PFAST486_FPU_DATA_REG Operand,
                          PFAST486_FPU_DATA_REG Result)
{
    FAST486_FPU_DATA_REG Value = *Operand;

    /* Add pi / 2 */
    if (!Fast486FpuAdd(State, &Value, &FpuHalfPi, &Value)) return FALSE;

    /* Calculate the sine */
    return Fast486FpuCalculateSine(State, &Value, Result);
}

static inline VOID FASTCALL
Fast486FpuArithmeticOperation(PFAST486_STATE State,
                              INT Operation,
                              PFAST486_FPU_DATA_REG Operand,
                              BOOLEAN TopDestination)
{
    PFAST486_FPU_DATA_REG DestOperand = TopDestination ? &FPU_ST(0) : Operand;

    ASSERT(!(Operation & ~7));

    /* Check the operation */
    switch (Operation)
    {
        /* FADD */
        case 0:
        {
            Fast486FpuAdd(State, &FPU_ST(0), Operand, DestOperand);
            break;
        }

        /* FMUL */
        case 1:
        {
            Fast486FpuMultiply(State, &FPU_ST(0), Operand, DestOperand);
            break;
        }

        /* FCOM */
        case 2:
        /* FCOMP */
        case 3:
        {
            Fast486FpuCompare(State, &FPU_ST(0), Operand);
            if (Operation == 3) Fast486FpuPop(State);

            break;
        }

        /* FSUB */
        case 4:
        {
            Fast486FpuSubtract(State, &FPU_ST(0), Operand, DestOperand);
            break;
        }

        /* FSUBR */
        case 5:
        {
            Fast486FpuSubtract(State, Operand, &FPU_ST(0), DestOperand);
            break;
        }

        /* FDIV */
        case 6:
        {
            Fast486FpuDivide(State, &FPU_ST(0), Operand, DestOperand);
            break;
        }

        /* FDIVR */
        case 7:
        {
            Fast486FpuDivide(State, Operand, &FPU_ST(0), DestOperand);
            break;
        }
    }
}

/*
 * Calculates using:
 * x[0] = s
 * x[n + 1] = (x[n] + s / x[n]) / 2
 */
static inline BOOLEAN FASTCALL
Fast486FpuCalculateSquareRoot(PFAST486_STATE State,
                              PCFAST486_FPU_DATA_REG Operand,
                              PFAST486_FPU_DATA_REG Result)
{
    FAST486_FPU_DATA_REG Value = *Operand;
    FAST486_FPU_DATA_REG PrevValue = FpuZero;

    if (Operand->Sign)
    {
        /* Raise the invalid operation exception */
        State->FpuStatus.Ie = TRUE;

        if (State->FpuControl.Im)
        {
            /* Return the indefinite NaN */
            Result->Sign = TRUE;
            Result->Exponent = FPU_MAX_EXPONENT + 1;
            Result->Mantissa = FPU_INDEFINITE_MANTISSA;
            return TRUE;
        }
        else
        {
            Fast486FpuException(State);
            return FALSE;
        }
    }

    /* Loop until it converges */
    while (Value.Sign != PrevValue.Sign
           || Value.Exponent != PrevValue.Exponent
           || Value.Mantissa != PrevValue.Mantissa)
    {
        FAST486_FPU_DATA_REG Temp;

        /* Save the current value */
        PrevValue = Value;

        /* Divide the operand by the current value */
        if (!Fast486FpuDivide(State, Operand, &Value, &Temp)) return FALSE;

        /* Add the result of that division to the current value */
        if (!Fast486FpuAdd(State, &Value, &Temp, &Value)) return FALSE;

        /* Halve the current value */
        if (!Fast486FpuMultiply(State, &Value, &FpuInverseNumber[1], &Value)) return FALSE;
    }

    *Result = Value;
    return TRUE;
}

static inline BOOLEAN FASTCALL
Fast486FpuLoadEnvironment(PFAST486_STATE State,
                          INT Segment,
                          ULONG Address,
                          BOOLEAN Size)
{
    UCHAR Buffer[28];

    if (!Fast486ReadMemory(State, Segment, Address, FALSE, Buffer, (Size + 1) * 14))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Check if this is a 32-bit save or a 16-bit save */
    if (Size)
    {
        PULONG Data = (PULONG)Buffer;

        State->FpuControl.Value = (USHORT)Data[0];
        State->FpuStatus.Value = (USHORT)Data[1];
        State->FpuTag = (USHORT)Data[2];
        State->FpuLastInstPtr.Long = Data[3];
        State->FpuLastCodeSel = (USHORT)Data[4];
        State->FpuLastOpPtr.Long = Data[5];
        State->FpuLastDataSel = (USHORT)Data[6];
    }
    else
    {
        PUSHORT Data = (PUSHORT)Buffer;

        State->FpuControl.Value = Data[0];
        State->FpuStatus.Value = Data[1];
        State->FpuTag = Data[2];
        State->FpuLastInstPtr.LowWord = Data[3];
        State->FpuLastCodeSel = Data[4];
        State->FpuLastOpPtr.LowWord = Data[5];
        State->FpuLastDataSel = Data[6];
    }

    return TRUE;
}

static inline BOOLEAN FASTCALL
Fast486FpuSaveEnvironment(PFAST486_STATE State,
                          INT Segment,
                          ULONG Address,
                          BOOLEAN Size)
{
    UCHAR Buffer[28];

    /* Check if this is a 32-bit save or a 16-bit save */
    if (Size)
    {
        PULONG Data = (PULONG)Buffer;

        Data[0] = (ULONG)State->FpuControl.Value;
        Data[1] = (ULONG)State->FpuStatus.Value;
        Data[2] = (ULONG)State->FpuTag;
        Data[3] = State->FpuLastInstPtr.Long;
        Data[4] = (ULONG)State->FpuLastCodeSel;
        Data[5] = State->FpuLastOpPtr.Long;
        Data[6] = (ULONG)State->FpuLastDataSel;
    }
    else
    {
        PUSHORT Data = (PUSHORT)Buffer;

        Data[0] = State->FpuControl.Value;
        Data[1] = State->FpuStatus.Value;
        Data[2] = State->FpuTag;
        Data[3] = State->FpuLastInstPtr.LowWord;
        Data[4] = State->FpuLastCodeSel;
        Data[5] = State->FpuLastOpPtr.LowWord;
        Data[6] = State->FpuLastDataSel;
    }

    return Fast486WriteMemory(State, Segment, Address, Buffer, (Size + 1) * 14);
}

#endif

/* PUBLIC FUNCTIONS ***********************************************************/

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeD8)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
#ifndef FAST486_NO_FPU
    PFAST486_FPU_DATA_REG Operand;
    FAST486_FPU_DATA_REG MemoryData;
#endif

    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    FPU_CHECK();

#ifndef FAST486_NO_FPU

    FPU_SAVE_LAST_INST();

    if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
    {
        /* Raise the invalid operation exception */
        State->FpuStatus.Ie = TRUE;

        if (State->FpuControl.Im)
        {
            /* Return the indefinite NaN */
            FPU_ST(0).Sign = TRUE;
            FPU_ST(0).Exponent = FPU_MAX_EXPONENT + 1;
            FPU_ST(0).Mantissa = FPU_INDEFINITE_MANTISSA;

            FPU_SET_TAG(0, FPU_TAG_SPECIAL);
        }
        else Fast486FpuException(State);

        return;
    }

    if (ModRegRm.Memory)
    {
        /* Load the source operand from memory */
        ULONG Value;

        if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }

        Fast486FpuFromSingleReal(State, Value, &MemoryData);
        Operand = &MemoryData;

        FPU_SAVE_LAST_OPERAND();
    }
    else
    {
        if (FPU_GET_TAG(ModRegRm.SecondRegister) == FPU_TAG_EMPTY)
        {
            /* Raise the invalid operation exception */
            State->FpuStatus.Ie = TRUE;

            if (State->FpuControl.Im)
            {
                /* Return the indefinite NaN */
                FPU_ST(0).Sign = TRUE;
                FPU_ST(0).Exponent = FPU_MAX_EXPONENT + 1;
                FPU_ST(0).Mantissa = FPU_INDEFINITE_MANTISSA;

                FPU_SET_TAG(0, FPU_TAG_SPECIAL);
            }
            else Fast486FpuException(State);

            return;
        }

        /* Load the source operand from an FPU register */
        Operand = &FPU_ST(ModRegRm.SecondRegister);
    }

    /* Perform the requested operation */
    Fast486FpuArithmeticOperation(State, ModRegRm.Register, Operand, TRUE);

#endif
}

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeD9)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    FPU_CHECK();

#ifndef FAST486_NO_FPU

    if (ModRegRm.Memory)
    {
        switch (ModRegRm.Register)
        {
            /* FLD */
            case 0:
            {
                ULONG Value;
                FAST486_FPU_DATA_REG MemoryData;

                FPU_SAVE_LAST_INST();
                FPU_SAVE_LAST_OPERAND();

                if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, NULL, &Value))
                {
                    /* Exception occurred */
                    return;
                }

                Fast486FpuFromSingleReal(State, Value, &MemoryData);
                Fast486FpuPush(State, &MemoryData);

                break;
            }

            /* FST */
            case 2:
            /* FSTP */
            case 3:
            {
                ULONG Value = FPU_REAL4_INDEFINITE;

                FPU_SAVE_LAST_INST();
                FPU_SAVE_LAST_OPERAND();

                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                {
                    /* Raise the invalid operation exception */
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im)
                    {
                        Fast486FpuException(State);
                        return;
                    }
                }
                else if (!Fast486FpuToSingleReal(State, &FPU_ST(0), &Value))
                {
                    /* Exception occurred */
                    return;
                }

                if (!Fast486WriteModrmDwordOperands(State, &ModRegRm, FALSE, Value))
                {
                    /* Exception occurred */
                    return;
                }

                if (ModRegRm.Register == 3) Fast486FpuPop(State);
                break;
            }

            /* FLDENV */
            case 4:
            {
                Fast486FpuLoadEnvironment(State,
                                          (State->PrefixFlags & FAST486_PREFIX_SEG)
                                          ? FAST486_REG_DS : State->SegmentOverride,
                                          ModRegRm.MemoryAddress,
                                          OperandSize);
                break;
            }

            /* FLDCW */
            case 5:
            {
                Fast486ReadModrmWordOperands(State, &ModRegRm, NULL, &State->FpuControl.Value);
                break;
            }

            /* FSTENV */
            case 6:
            {
                Fast486FpuSaveEnvironment(State,
                                          (State->PrefixFlags & FAST486_PREFIX_SEG)
                                          ? FAST486_REG_DS : State->SegmentOverride,
                                          ModRegRm.MemoryAddress,
                                          OperandSize);
                break;
            }

            /* FSTCW */
            case 7:
            {
                Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, State->FpuControl.Value);
                break;
            }

            /* Invalid */
            default:
            {
                Fast486Exception(State, FAST486_EXCEPTION_UD);
                return;
            }
        }
    }
    else
    {
        switch ((ModRegRm.Register << 3) | ModRegRm.SecondRegister)
        {
            /* FLD */
            case 0x00:
            case 0x01:
            case 0x02:
            case 0x03:
            case 0x04:
            case 0x05:
            case 0x06:
            case 0x07:
            {
                if (FPU_GET_TAG(ModRegRm.SecondRegister) == FPU_TAG_EMPTY)
                {
                    /* Raise the invalid operation exception */
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im)
                    {
                        Fast486FpuException(State);
                        return;
                    }
                }

                Fast486FpuPush(State, &FPU_ST(ModRegRm.SecondRegister));
                break;
            }

            /* FXCH */
            case 0x08:
            case 0x09:
            case 0x0A:
            case 0x0B:
            case 0x0C:
            case 0x0D:
            case 0x0E:
            case 0x0F:
            {
                FAST486_FPU_DATA_REG Temp;

                if ((FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                    || FPU_GET_TAG(ModRegRm.SecondRegister) == FPU_TAG_EMPTY)
                {
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im) Fast486FpuException(State);
                    break;
                }

                /* Exchange */
                Temp = FPU_ST(0);
                FPU_ST(0) = FPU_ST(ModRegRm.SecondRegister);
                FPU_ST(ModRegRm.SecondRegister) = Temp;

                FPU_UPDATE_TAG(0);
                FPU_UPDATE_TAG(ModRegRm.SecondRegister);

                break;
            }

            /* FNOP */
            case 0x10:
            {
                /* Do nothing */
                break;
            }

            /* FSTP */
            case 0x18:
            case 0x19:
            case 0x1A:
            case 0x1B:
            case 0x1C:
            case 0x1D:
            case 0x1E:
            case 0x1F:
            {
                FPU_ST(ModRegRm.SecondRegister) = FPU_ST(0);
                FPU_UPDATE_TAG(ModRegRm.SecondRegister);

                Fast486FpuPop(State);
                break;
            }

            /* FCHS */
            case 0x20:
            {
                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                {
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im) Fast486FpuException(State);
                    break;
                }

                /* Invert the sign */
                FPU_ST(0).Sign = !FPU_ST(0).Sign;

                break;
            }

            /* FABS */
            case 0x21:
            {
                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                {
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im) Fast486FpuException(State);
                    break;
                }

                /* Set the sign to positive */
                FPU_ST(0).Sign = FALSE;

                break;
            }

            /* FTST */
            case 0x24:
            {
                Fast486FpuCompare(State, &FPU_ST(0), &FpuZero);
                break;
            }

            /* FXAM */
            case 0x25:
            {
                /* The sign bit goes in C1, even if the register's empty */
                State->FpuStatus.Code1 = FPU_ST(0).Sign;

                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                {
                    State->FpuStatus.Code0 = 1;
                    State->FpuStatus.Code2 = 0;
                    State->FpuStatus.Code3 = 1;
                }
                else if (FPU_GET_TAG(0) == FPU_TAG_SPECIAL)
                {
                    if (FPU_IS_INFINITY(&FPU_ST(0)))
                    {
                        State->FpuStatus.Code0 = 1;
                        State->FpuStatus.Code2 = 1;
                        State->FpuStatus.Code3 = 0;
                    }
                    else
                    {
                        State->FpuStatus.Code0 = 1;
                        State->FpuStatus.Code2 = 0;
                        State->FpuStatus.Code3 = 0;
                    }
                }
                else if (FPU_GET_TAG(0) == FPU_TAG_ZERO)
                {
                    State->FpuStatus.Code0 = 0;
                    State->FpuStatus.Code2 = 0;
                    State->FpuStatus.Code3 = 1;
                }
                else
                {
                    if (FPU_IS_NORMALIZED(&FPU_ST(0)))
                    {
                        State->FpuStatus.Code0 = 0;
                        State->FpuStatus.Code2 = 1;
                        State->FpuStatus.Code3 = 0;
                    }
                    else
                    {
                        State->FpuStatus.Code0 = 0;
                        State->FpuStatus.Code2 = 1;
                        State->FpuStatus.Code3 = 1;
                    }
                }

                break;
            }

            /* FLD1 */
            case 0x28:
            /* FLDL2T */
            case 0x29:
            /* FLDL2E */
            case 0x2A:
            /* FLDPI */
            case 0x2B:
            /* FLDLG2 */
            case 0x2C:
            /* FLDLN2 */
            case 0x2D:
            /* FLDZ */
            case 0x2E:
            {
                PCFAST486_FPU_DATA_REG Constants[] =
                {
                    &FpuOne,
                    &FpuL2Ten,
                    &FpuL2E,
                    &FpuPi,
                    &FpuLgTwo,
                    &FpuLnTwo,
                    &FpuZero
                };

                Fast486FpuPush(State, Constants[ModRegRm.SecondRegister]);
                break;
            }

            /* F2XM1 */
            case 0x30:
            {
                FPU_SAVE_LAST_INST();

                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                {
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im) Fast486FpuException(State);
                    break;
                }

                if (!FPU_IS_NORMALIZED(&FPU_ST(0)))
                {
                    State->FpuStatus.De = TRUE;

                    if (!State->FpuControl.Dm)
                    {
                        Fast486FpuException(State);
                        break;
                    }
                }

                Fast486FpuCalculateTwoPowerMinusOne(State, &FPU_ST(0), &FPU_ST(0));
                FPU_UPDATE_TAG(0);

                break;
            }

            /* FYL2X */
            case 0x31:
            {
                FAST486_FPU_DATA_REG Logarithm;

                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY || FPU_GET_TAG(1) == FPU_TAG_EMPTY)
                {
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im) Fast486FpuException(State);
                    break;
                }

                if (!Fast486FpuCalculateLogBase2(State, &FPU_ST(0), &Logarithm))
                {
                    /* Exception occurred */
                    break;
                }

                if (!Fast486FpuMultiply(State, &Logarithm, &FPU_ST(1), &FPU_ST(1)))
                {
                    /* Exception occurred */
                    break;
                }

                /* Pop the stack so that the result ends up in ST0 */
                Fast486FpuPop(State);
                FPU_UPDATE_TAG(0);

                break;
            }

            /* FPTAN */
            case 0x32:
            {
                // TODO: NOT IMPLEMENTED
                UNIMPLEMENTED;

                break;
            }

            /* FPATAN */
            case 0x33:
            {
                // TODO: NOT IMPLEMENTED
                UNIMPLEMENTED;

                break;
            }

            /* FXTRACT */
            case 0x34:
            {
                FAST486_FPU_DATA_REG Value = FPU_ST(0);

                if ((FPU_GET_TAG(0) == FPU_TAG_EMPTY) || FPU_IS_INDEFINITE(&Value))
                {
                    State->FpuStatus.Ie = TRUE;
                    if (FPU_GET_TAG(0) == FPU_TAG_EMPTY) State->FpuStatus.Sf = TRUE;

                    if (!State->FpuControl.Im) Fast486FpuException(State);
                    break;
                }

                if (FPU_IS_ZERO(&Value))
                {
                    /* The exponent of zero is negative infinity */
                    FPU_ST(0).Sign = TRUE;
                    FPU_ST(0).Exponent = FPU_MAX_EXPONENT + 1;
                    FPU_ST(0).Mantissa = FPU_MANTISSA_HIGH_BIT;
                }
                else if (FPU_IS_INFINITY(&Value))
                {
                    /* The exponent of infinity is positive infinity */
                    FPU_ST(0).Sign = FALSE;
                    FPU_ST(0).Exponent = FPU_MAX_EXPONENT + 1;
                    FPU_ST(0).Mantissa = FPU_MANTISSA_HIGH_BIT;
                }
                else
                {
                    /* Store the unbiased exponent in ST0 */
                    Fast486FpuFromInteger(State,
                                          (LONGLONG)Value.Exponent - (LONGLONG)FPU_REAL10_BIAS,
                                          &FPU_ST(0));
                }

                /* Now push the mantissa as a real number, with the original sign */
                Value.Exponent = FPU_REAL10_BIAS;
                Fast486FpuPush(State, &Value);

                break;
            }

            /* FPREM1 */
            case 0x35:
            {
                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY || FPU_GET_TAG(1) == FPU_TAG_EMPTY)
                {
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im) Fast486FpuException(State);
                    break;
                }

                Fast486FpuRemainder(State, &FPU_ST(0), &FPU_ST(1), TRUE, &FPU_ST(0), NULL);
                FPU_UPDATE_TAG(0);

                break;
            }

            /* FDECSTP */
            case 0x36:
            {
                State->FpuStatus.Top--;
                break;
            }

            /* FINCSTP */
            case 0x37:
            {
                State->FpuStatus.Top++;
                break;
            }

            /* FPREM */
            case 0x38:
            {
                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY || FPU_GET_TAG(1) == FPU_TAG_EMPTY)
                {
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im) Fast486FpuException(State);
                    break;
                }

                Fast486FpuRemainder(State, &FPU_ST(0), &FPU_ST(1), FALSE, &FPU_ST(0), NULL);
                FPU_UPDATE_TAG(0);

                break;
            }

            /* FYL2XP1 */
            case 0x39:
            {
                FAST486_FPU_DATA_REG Value, Logarithm;

                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY || FPU_GET_TAG(1) == FPU_TAG_EMPTY)
                {
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im) Fast486FpuException(State);
                    break;
                }

                if (!Fast486FpuAdd(State, &FPU_ST(0), &FpuOne, &Value))
                {
                    /* Exception occurred */
                    break;
                }

                if (!Fast486FpuCalculateLogBase2(State, &Value, &Logarithm))
                {
                    /* Exception occurred */
                    break;
                }

                if (!Fast486FpuMultiply(State, &Logarithm, &FPU_ST(1), &FPU_ST(1)))
                {
                    /* Exception occurred */
                    break;
                }

                /* Pop the stack so that the result ends up in ST0 */
                Fast486FpuPop(State);
                FPU_UPDATE_TAG(0);

                break;
            }

            /* FSQRT */
            case 0x3A:
            {
                Fast486FpuCalculateSquareRoot(State, &FPU_ST(0), &FPU_ST(0));
                FPU_UPDATE_TAG(0);

                break;
            }

            /* FSINCOS */
            case 0x3B:
            {
                FAST486_FPU_DATA_REG Number = FPU_ST(0);
                FPU_SAVE_LAST_INST();

                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                {
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im) Fast486FpuException(State);
                    break;
                }

                if (!FPU_IS_NORMALIZED(&FPU_ST(0)))
                {
                    State->FpuStatus.De = TRUE;

                    if (!State->FpuControl.Dm)
                    {
                        Fast486FpuException(State);
                        break;
                    }
                }

                /* Replace FP0 with the sine */
                if (!Fast486FpuCalculateSine(State, &Number, &FPU_ST(0))) break;
                FPU_UPDATE_TAG(0);

                /* Push the cosine */
                if (!Fast486FpuCalculateCosine(State, &Number, &Number)) break;
                Fast486FpuPush(State, &Number);

                break;
            }

            /* FRNDINT */
            case 0x3C:
            {
                LONGLONG Result = 0LL;

                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                {
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im) Fast486FpuException(State);
                    break;
                }

                if (!FPU_IS_NORMALIZED(&FPU_ST(0)))
                {
                    State->FpuStatus.De = TRUE;

                    if (!State->FpuControl.Dm)
                    {
                        Fast486FpuException(State);
                        break;
                    }
                }

                /* Do nothing if it's too big to not be an integer */
                if (FPU_ST(0).Exponent >= FPU_REAL10_BIAS + 63) break;

                /* Perform the rounding */
                Fast486FpuToInteger(State, &FPU_ST(0), &Result);
                Fast486FpuFromInteger(State, Result, &FPU_ST(0));

                State->FpuStatus.Pe = TRUE;
                if (!State->FpuControl.Pm) Fast486FpuException(State);

                break;
            }

            /* FSCALE */
            case 0x3D:
            {
                LONGLONG Scale;
                LONGLONG UnbiasedExp = (LONGLONG)((SHORT)FPU_ST(0).Exponent) - FPU_REAL10_BIAS;
                INT OldRoundingMode = State->FpuControl.Rc;

                FPU_SAVE_LAST_INST();

                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY || FPU_GET_TAG(1) == FPU_TAG_EMPTY)
                {
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im) Fast486FpuException(State);
                    break;
                }

                if (!FPU_IS_NORMALIZED(&FPU_ST(0)))
                {
                    State->FpuStatus.De = TRUE;

                    if (!State->FpuControl.Dm)
                    {
                        Fast486FpuException(State);
                        break;
                    }
                }

                State->FpuControl.Rc = FPU_ROUND_TRUNCATE;

                if (!Fast486FpuToInteger(State, &FPU_ST(1), &Scale))
                {
                    /* Exception occurred */
                    State->FpuControl.Rc = OldRoundingMode;
                    break;
                }

                State->FpuControl.Rc = OldRoundingMode;

                /* Adjust the unbiased exponent */
                UnbiasedExp += Scale;

                /* Check for underflow */
                if (UnbiasedExp < -1023)
                {
                    /* Raise the underflow exception */
                    State->FpuStatus.Ue = TRUE;

                    if (State->FpuControl.Um)
                    {
                        /* Make the result zero */
                        FPU_ST(0) = FpuZero;
                        FPU_UPDATE_TAG(0);
                    }
                    else
                    {
                        Fast486FpuException(State);
                    }

                    break;
                }

                /* Check for overflow */
                if (UnbiasedExp > 1023)
                {
                    /* Raise the overflow exception */
                    State->FpuStatus.Oe = TRUE;

                    if (State->FpuControl.Om)
                    {
                        /* Make the result infinity */
                        FPU_ST(0).Mantissa = FPU_MANTISSA_HIGH_BIT;
                        FPU_ST(0).Exponent = FPU_MAX_EXPONENT + 1;
                        FPU_UPDATE_TAG(0);
                    }
                    else
                    {
                        Fast486FpuException(State);
                    }

                    break;
                }

                FPU_ST(0).Exponent = (USHORT)(UnbiasedExp + FPU_REAL10_BIAS);
                FPU_UPDATE_TAG(0);

                break;
            }

            /* FSIN */
            case 0x3E:
            {
                FPU_SAVE_LAST_INST();

                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                {
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im) Fast486FpuException(State);
                    break;
                }

                if (!FPU_IS_NORMALIZED(&FPU_ST(0)))
                {
                    State->FpuStatus.De = TRUE;

                    if (!State->FpuControl.Dm)
                    {
                        Fast486FpuException(State);
                        break;
                    }
                }

                Fast486FpuCalculateSine(State, &FPU_ST(0), &FPU_ST(0));
                FPU_UPDATE_TAG(0);

                break;
            }

            /* FCOS */
            case 0x3F:
            {
                FPU_SAVE_LAST_INST();

                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                {
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im) Fast486FpuException(State);
                    break;
                }

                if (!FPU_IS_NORMALIZED(&FPU_ST(0)))
                {
                    State->FpuStatus.De = TRUE;

                    if (!State->FpuControl.Dm)
                    {
                        Fast486FpuException(State);
                        break;
                    }
                }

                Fast486FpuCalculateCosine(State, &FPU_ST(0), &FPU_ST(0));
                FPU_UPDATE_TAG(0);

                break;
            }

            /* Invalid */
            default:
            {
                Fast486Exception(State, FAST486_EXCEPTION_UD);
                return;
            }
        }
    }

#endif
}

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDA)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
#ifndef FAST486_NO_FPU
    LONG Value;
    FAST486_FPU_DATA_REG MemoryData;
#endif

    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    FPU_CHECK();

#ifndef FAST486_NO_FPU

    FPU_SAVE_LAST_INST();

    if (!ModRegRm.Memory)
    {
        /* The only valid opcode in this case is FUCOMPP (0xDA 0xE9) */
        if ((ModRegRm.Register != 5) && (ModRegRm.SecondRegister != 1))
        {
            Fast486Exception(State, FAST486_EXCEPTION_UD);
            return;
        }

        if ((FPU_GET_TAG(0) == FPU_TAG_EMPTY) || (FPU_GET_TAG(1) == FPU_TAG_EMPTY))
        {
            /* Raise the invalid operation exception*/
            State->FpuStatus.Ie = TRUE;

            if (!State->FpuControl.Im) Fast486FpuException(State);
            return;
        }

        /* Compare */
        Fast486FpuCompare(State, &FPU_ST(0), &FPU_ST(1));

        /* Pop twice */
        Fast486FpuPop(State);
        Fast486FpuPop(State);

        return;
    }

    FPU_SAVE_LAST_OPERAND();

    /* Load the source operand from memory */
    if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, NULL, (PULONG)&Value))
    {
        /* Exception occurred */
        return;
    }

    if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
    {
        /* Raise the invalid operation exception */
        State->FpuStatus.Ie = TRUE;

        if (State->FpuControl.Im)
        {
            /* Return the indefinite NaN */
            FPU_ST(0).Sign = TRUE;
            FPU_ST(0).Exponent = FPU_MAX_EXPONENT + 1;
            FPU_ST(0).Mantissa = FPU_INDEFINITE_MANTISSA;

            FPU_SET_TAG(0, FPU_TAG_SPECIAL);
        }
        else Fast486FpuException(State);

        return;
    }

    Fast486FpuFromInteger(State, (LONGLONG)Value, &MemoryData);

    /* Perform the requested operation */
    Fast486FpuArithmeticOperation(State, ModRegRm.Register, &MemoryData, TRUE);

#endif
}

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDB)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    FPU_CHECK();

#ifndef FAST486_NO_FPU

    if (ModRegRm.Memory)
    {
        FPU_SAVE_LAST_INST();
        FPU_SAVE_LAST_OPERAND();

        switch (ModRegRm.Register)
        {
            /* FILD */
            case 0:
            {
                LONG Value;
                FAST486_FPU_DATA_REG Temp;

                if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, NULL, (PULONG)&Value))
                {
                    /* Exception occurred */
                    return;
                }

                Fast486FpuFromInteger(State, (LONGLONG)Value, &Temp);
                Fast486FpuPush(State, &Temp);

                break;
            }

            /* FIST */
            case 2:
            /* FISTP */
            case 3:
            {
                LONGLONG Temp = 0;

                if ((FPU_GET_TAG(0) == FPU_TAG_EMPTY) || (FPU_GET_TAG(0) == FPU_TAG_SPECIAL))
                {
                    /* Raise the invalid operation exception */
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im)
                    {
                        Fast486FpuException(State);
                        return;
                    }
                }

                if (!Fast486FpuToInteger(State, &FPU_ST(0), &Temp))
                {
                    /* Exception occurred */
                    return;
                }

                /* Check if it can fit in a signed 32-bit integer */
                if ((LONGLONG)((LONG)Temp) != Temp)
                {
                    State->FpuStatus.Ie = TRUE;

                    if (State->FpuControl.Im)
                    {
                        Temp = 0x80000000LL;
                    }
                    else
                    {
                        Fast486FpuException(State);
                        return;
                    }
                }

                if (!Fast486WriteModrmDwordOperands(State, &ModRegRm, FALSE, (ULONG)((LONG)Temp)))
                {
                    /* Exception occurred */
                    return;
                }

                if (ModRegRm.Register == 3)
                {
                    /* Pop the FPU stack too */
                    Fast486FpuPop(State);
                }

                break;
            }

            /* FLD */
            case 5:
            {
                FAST486_FPU_DATA_REG Value;
                UCHAR Buffer[10];

                if (!Fast486ReadMemory(State,
                                       (State->PrefixFlags & FAST486_PREFIX_SEG)
                                       ? State->SegmentOverride : FAST486_REG_DS,
                                       ModRegRm.MemoryAddress,
                                       FALSE,
                                       Buffer,
                                       sizeof(Buffer)))
                {
                    /* Exception occurred */
                    return;
                }

                Value.Mantissa = *((PULONGLONG)Buffer);
                Value.Exponent = *((PUSHORT)&Buffer[8]) & (FPU_MAX_EXPONENT + 1);
                Value.Sign = *((PUCHAR)&Buffer[9]) >> 7;

                Fast486FpuPush(State, &Value);
                break;
            }

            /* FSTP */
            case 7:
            {
                UCHAR Buffer[10];

                if (FPU_GET_TAG(0) != FPU_TAG_EMPTY)
                {
                    *((PULONGLONG)Buffer) = FPU_ST(0).Mantissa;
                    *((PUSHORT)&Buffer[sizeof(ULONGLONG)]) = FPU_ST(0).Exponent
                                                             | (FPU_ST(0).Sign ? 0x8000 : 0);
                }
                else
                {
                    /* Raise the invalid operation exception */
                    State->FpuStatus.Ie = TRUE;

                    if (State->FpuControl.Im)
                    {
                        *((PULONGLONG)Buffer) = FPU_INDEFINITE_MANTISSA;
                        *((PUSHORT)&Buffer[sizeof(ULONGLONG)]) = 0x8000 | (FPU_MAX_EXPONENT + 1);
                    }
                    else
                    {
                        Fast486FpuException(State);
                        return;
                    }
                }

                if (!Fast486WriteMemory(State,
                                        (State->PrefixFlags & FAST486_PREFIX_SEG)
                                        ? State->SegmentOverride : FAST486_REG_DS,
                                        ModRegRm.MemoryAddress,
                                        Buffer,
                                        sizeof(Buffer)))
                {
                    /* Exception occurred */
                    return;
                }

                Fast486FpuPop(State);
                break;
            }

            /* Invalid */
            default:
            {
                Fast486Exception(State, FAST486_EXCEPTION_UD);
                return;
            }
        }
    }
    else
    {
        /* Only a few of these instructions have any meaning on a 487 */
        switch ((ModRegRm.Register << 3) | ModRegRm.SecondRegister)
        {
            /* FCLEX */
            case 0x22:
            {
                /* Clear exception data */
                State->FpuStatus.Ie =
                State->FpuStatus.De =
                State->FpuStatus.Ze =
                State->FpuStatus.Oe =
                State->FpuStatus.Ue =
                State->FpuStatus.Pe =
                State->FpuStatus.Sf =
                State->FpuStatus.Es =
                State->FpuStatus.Busy = FALSE;

                break;
            }

            /* FINIT */
            case 0x23:
            {
                /* Restore the state */
                State->FpuControl.Value = FAST486_FPU_DEFAULT_CONTROL;
                State->FpuStatus.Value = 0;
                State->FpuTag = 0xFFFF;

                break;
            }

            /* FENI */
            case 0x20:
            /* FDISI */
            case 0x21:
            /* FSETPM */
            case 0x24:
            /* FRSTPM */
            case 0x25:
            {
                /* These do nothing */
                break;
            }

            /* Invalid */
            default:
            {
                Fast486Exception(State, FAST486_EXCEPTION_UD);
                return;
            }
        }
    }

#endif
}

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDC)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
#ifndef FAST486_NO_FPU
    PFAST486_FPU_DATA_REG Operand;
    FAST486_FPU_DATA_REG MemoryData;
#endif

    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    FPU_CHECK();

#ifndef FAST486_NO_FPU

    FPU_SAVE_LAST_INST();

    if (ModRegRm.Memory)
    {
        ULONGLONG Value;

        if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
        {
            /* Raise the invalid operation exception */
            State->FpuStatus.Ie = TRUE;

            if (State->FpuControl.Im)
            {
                /* Return the indefinite NaN */
                FPU_ST(0).Sign = TRUE;
                FPU_ST(0).Exponent = FPU_MAX_EXPONENT + 1;
                FPU_ST(0).Mantissa = FPU_INDEFINITE_MANTISSA;

                FPU_SET_TAG(0, FPU_TAG_SPECIAL);
            }
            else Fast486FpuException(State);

            return;
        }

        /* Load the source operand from memory */
        if (!Fast486ReadMemory(State,
                               (State->PrefixFlags & FAST486_PREFIX_SEG)
                               ? State->SegmentOverride : FAST486_REG_DS,
                               ModRegRm.MemoryAddress,
                               FALSE,
                               &Value,
                               sizeof(ULONGLONG)))
        {
            /* Exception occurred */
            return;
        }

        Fast486FpuFromDoubleReal(State, Value, &MemoryData);
        Operand = &MemoryData;

        FPU_SAVE_LAST_OPERAND();
    }
    else
    {
        /* Load the destination operand from an FPU register */
        Operand = &FPU_ST(ModRegRm.SecondRegister);

        if ((FPU_GET_TAG(0) == FPU_TAG_EMPTY)
            || (FPU_GET_TAG(ModRegRm.SecondRegister) == FPU_TAG_EMPTY))
        {
            /* Raise the invalid operation exception */
            State->FpuStatus.Ie = TRUE;

            if (State->FpuControl.Im)
            {
                /* Return the indefinite NaN */
                Operand->Sign = TRUE;
                Operand->Exponent = FPU_MAX_EXPONENT + 1;
                Operand->Mantissa = FPU_INDEFINITE_MANTISSA;

                FPU_SET_TAG(ModRegRm.SecondRegister, FPU_TAG_SPECIAL);
            }
            else Fast486FpuException(State);

            return;
        }
    }

    /* Perform the requested operation */
    Fast486FpuArithmeticOperation(State, ModRegRm.Register, Operand, ModRegRm.Memory);

#endif
}

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDD)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    FPU_CHECK();

#ifndef FAST486_NO_FPU

    if (ModRegRm.Memory)
    {
        switch (ModRegRm.Register)
        {
            /* FLD */
            case 0:
            {
                ULONGLONG Value;
                FAST486_FPU_DATA_REG MemoryData;

                FPU_SAVE_LAST_INST();
                FPU_SAVE_LAST_OPERAND();

                if (!Fast486ReadMemory(State,
                                       (State->PrefixFlags & FAST486_PREFIX_SEG)
                                       ? State->SegmentOverride : FAST486_REG_DS,
                                       ModRegRm.MemoryAddress,
                                       FALSE,
                                       &Value,
                                       sizeof(ULONGLONG)))
                {
                    /* Exception occurred */
                    return;
                }

                Fast486FpuFromDoubleReal(State, Value, &MemoryData);
                Fast486FpuPush(State, &MemoryData);

                break;
            }

            /* FST */
            case 2:
            /* FSTP */
            case 3:
            {
                ULONGLONG Value = FPU_REAL8_INDEFINITE;

                FPU_SAVE_LAST_INST();
                FPU_SAVE_LAST_OPERAND();

                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                {
                    /* Raise the invalid operation exception */
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im)
                    {
                        Fast486FpuException(State);
                        return;
                    }
                }
                else if (!Fast486FpuToDoubleReal(State, &FPU_ST(0), &Value))
                {
                    /* Exception occurred */
                    return;
                }

                if (!Fast486WriteMemory(State,
                                        (State->PrefixFlags & FAST486_PREFIX_SEG)
                                        ? State->SegmentOverride : FAST486_REG_DS,
                                        ModRegRm.MemoryAddress,
                                        &Value,
                                        sizeof(ULONGLONG)))
                {
                    /* Exception occurred */
                    return;
                }

                if (ModRegRm.Register == 3) Fast486FpuPop(State);
                break;
            }

            /* FRSTOR */
            case 4:
            {
                INT i;
                UCHAR AllRegs[80];

                /* Save the environment */
                if (!Fast486FpuLoadEnvironment(State,
                                               (State->PrefixFlags & FAST486_PREFIX_SEG)
                                               ? FAST486_REG_DS : State->SegmentOverride,
                                               ModRegRm.MemoryAddress,
                                               OperandSize))
                {
                    /* Exception occurred */
                    return;
                }

                /* Load the registers */
                if (!Fast486ReadMemory(State,
                                       (State->PrefixFlags & FAST486_PREFIX_SEG)
                                       ? FAST486_REG_DS : State->SegmentOverride,
                                       ModRegRm.MemoryAddress + (OperandSize + 1) * 14,
                                       FALSE,
                                       AllRegs,
                                       sizeof(AllRegs)))
                {
                    /* Exception occurred */
                    return;
                }

                for (i = 0; i < FAST486_NUM_FPU_REGS; i++)
                {
                    State->FpuRegisters[i].Mantissa = *((PULONGLONG)&AllRegs[i * 10]);
                    State->FpuRegisters[i].Exponent = *((PUSHORT)&AllRegs[(i * 10) + sizeof(ULONGLONG)]) & 0x7FFF;

                    if (*((PUSHORT)&AllRegs[(i * 10) + sizeof(ULONGLONG)]) & 0x8000)
                    {
                        State->FpuRegisters[i].Sign = TRUE;
                    }
                    else
                    {
                        State->FpuRegisters[i].Sign = FALSE;
                    }
                }

                break;
            }

            /* FSAVE */
            case 6:
            {
                INT i;
                UCHAR AllRegs[80];

                /* Save the environment */
                if (!Fast486FpuSaveEnvironment(State,
                                               (State->PrefixFlags & FAST486_PREFIX_SEG)
                                               ? FAST486_REG_DS : State->SegmentOverride,
                                               ModRegRm.MemoryAddress,
                                               OperandSize))
                {
                    /* Exception occurred */
                    return;
                }

                /* Save the registers */
                for (i = 0; i < FAST486_NUM_FPU_REGS; i++)
                {
                    *((PULONGLONG)&AllRegs[i * 10]) = State->FpuRegisters[i].Mantissa;
                    *((PUSHORT)&AllRegs[(i * 10) + sizeof(ULONGLONG)]) = State->FpuRegisters[i].Exponent;

                    if (State->FpuRegisters[i].Sign)
                    {
                        *((PUSHORT)&AllRegs[(i * 10) + sizeof(ULONGLONG)]) |= 0x8000;
                    }
                }

                Fast486WriteMemory(State,
                                   (State->PrefixFlags & FAST486_PREFIX_SEG)
                                   ? FAST486_REG_DS : State->SegmentOverride,
                                   ModRegRm.MemoryAddress + (OperandSize + 1) * 14,
                                   AllRegs,
                                   sizeof(AllRegs));

                break;
            }

            /* FSTSW */
            case 7:
            {
                Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, State->FpuStatus.Value);
                break;
            }

            /* Invalid */
            default:
            {
                Fast486Exception(State, FAST486_EXCEPTION_UD);
            }
        }
    }
    else
    {
        FPU_SAVE_LAST_INST();

        switch (ModRegRm.Register)
        {
            /* FFREE */
            case 0:
            {
                FPU_SET_TAG(ModRegRm.SecondRegister, FPU_TAG_EMPTY);
                break;
            }

            /* FXCH */
            case 1:
            {
                FAST486_FPU_DATA_REG Temp;

                if ((FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                    || FPU_GET_TAG(ModRegRm.SecondRegister) == FPU_TAG_EMPTY)
                {
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im) Fast486FpuException(State);
                    break;
                }

                /* Exchange */
                Temp = FPU_ST(0);
                FPU_ST(0) = FPU_ST(ModRegRm.SecondRegister);
                FPU_ST(ModRegRm.SecondRegister) = Temp;

                FPU_UPDATE_TAG(0);
                FPU_UPDATE_TAG(ModRegRm.SecondRegister);

                break;
            }

            /* FST */
            case 2:
            /* FSTP */
            case 3:
            {
                FPU_ST(ModRegRm.SecondRegister) = FPU_ST(0);
                FPU_UPDATE_TAG(ModRegRm.SecondRegister);

                if (ModRegRm.Register == 3) Fast486FpuPop(State);
                break;
            }

            /* FUCOM */
            case 4:
            /* FUCOMP */
            case 5:
            {
                if ((FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                    || (FPU_GET_TAG(ModRegRm.SecondRegister) == FPU_TAG_EMPTY))
                {
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im) Fast486FpuException(State);
                    return;
                }

                Fast486FpuCompare(State, &FPU_ST(0), &FPU_ST(ModRegRm.SecondRegister));
                if (ModRegRm.Register == 5) Fast486FpuPop(State);

                break;
            }

            /* Invalid */
            default:
            {
                Fast486Exception(State, FAST486_EXCEPTION_UD);
            }
        }
    }

#endif
}

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDE)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
#ifndef FAST486_NO_FPU
    PFAST486_FPU_DATA_REG Operand;
    FAST486_FPU_DATA_REG MemoryData;
#endif

    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    FPU_CHECK();

#ifndef FAST486_NO_FPU

    FPU_SAVE_LAST_INST();

    if (ModRegRm.Memory)
    {
        SHORT Value;

        if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
        {
            /* Raise the invalid operation exception */
            State->FpuStatus.Ie = TRUE;

            if (State->FpuControl.Im)
            {
                /* Return the indefinite NaN */
                FPU_ST(0).Sign = TRUE;
                FPU_ST(0).Exponent = FPU_MAX_EXPONENT + 1;
                FPU_ST(0).Mantissa = FPU_INDEFINITE_MANTISSA;

                FPU_SET_TAG(0, FPU_TAG_SPECIAL);
            }
            else Fast486FpuException(State);

            return;
        }

        /* Load the source operand from memory */
        if (!Fast486ReadModrmWordOperands(State, &ModRegRm, NULL, (PUSHORT)&Value))
        {
            /* Exception occurred */
            return;
        }

        Fast486FpuFromInteger(State, (LONGLONG)Value, &MemoryData);
        Operand = &MemoryData;

        FPU_SAVE_LAST_OPERAND();
    }
    else
    {
        /* FCOMPP check */
        if ((ModRegRm.Register == 3) && (ModRegRm.SecondRegister != 1))
        {
            /* Invalid */
            Fast486Exception(State, FAST486_EXCEPTION_UD);
            return;
        }

        /* Load the destination operand from a register */
        Operand = &FPU_ST(ModRegRm.SecondRegister);

        if ((FPU_GET_TAG(0) == FPU_TAG_EMPTY)
            || (FPU_GET_TAG(ModRegRm.SecondRegister) == FPU_TAG_EMPTY))
        {
            /* Raise the invalid operation exception, if unmasked */
            State->FpuStatus.Ie = TRUE;

            if (!State->FpuControl.Im) Fast486FpuException(State);
            return;
        }
    }

    /* Perform the requested operation */
    Fast486FpuArithmeticOperation(State, ModRegRm.Register, Operand, ModRegRm.Memory);
    if (!ModRegRm.Memory) Fast486FpuPop(State);

#endif
}

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDF)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    FPU_CHECK();

#ifndef FAST486_NO_FPU

    FPU_SAVE_LAST_INST();

    if (ModRegRm.Memory)
    {
        FPU_SAVE_LAST_OPERAND();

        switch (ModRegRm.Register)
        {
            /* FILD */
            case 0:
            {
                SHORT Value;
                FAST486_FPU_DATA_REG Temp;

                if (!Fast486ReadModrmWordOperands(State, &ModRegRm, NULL, (PUSHORT)&Value))
                {
                    /* Exception occurred */
                    return;
                }

                Fast486FpuFromInteger(State, (LONGLONG)Value, &Temp);
                Fast486FpuPush(State, &Temp);

                break;
            }

            /* FIST */
            case 2:
            /* FISTP */
            case 3:
            {
                LONGLONG Temp = 0LL;

                if ((FPU_GET_TAG(0) == FPU_TAG_EMPTY) || (FPU_GET_TAG(0) == FPU_TAG_SPECIAL))
                {
                    /* Raise the invalid operation exception */
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im)
                    {
                        Fast486FpuException(State);
                        return;
                    }
                }

                if (!Fast486FpuToInteger(State, &FPU_ST(0), &Temp))
                {
                    /* Exception occurred */
                    return;
                }

                /* Check if it can fit in a signed 16-bit integer */
                if ((LONGLONG)((SHORT)Temp) != Temp)
                {
                    /* Raise the invalid operation exception */
                    State->FpuStatus.Ie = TRUE;

                    if (State->FpuControl.Im)
                    {
                        Temp = 0x8000LL;
                    }
                    else
                    {
                        Fast486FpuException(State);
                        return;
                    }
                }

                if (!Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, (USHORT)((SHORT)Temp)))
                {
                    /* Exception occurred */
                    return;
                }

                if (ModRegRm.Register == 3)
                {
                    /* Pop the FPU stack too */
                    Fast486FpuPop(State);
                }

                break;
            }

            /* FBLD */
            case 4:
            {
                FAST486_FPU_DATA_REG Value;
                UCHAR Buffer[10];

                if (!Fast486ReadMemory(State,
                                       (State->PrefixFlags & FAST486_PREFIX_SEG)
                                       ? State->SegmentOverride : FAST486_REG_DS,
                                       ModRegRm.MemoryAddress,
                                       FALSE,
                                       Buffer,
                                       sizeof(Buffer)))
                {
                    /* Exception occurred */
                    return;
                }

                Fast486FpuFromPackedBcd(State, Buffer, &Value);
                Fast486FpuPush(State, &Value);

                break;
            }

            /* FILD (64-bit int) */
            case 5:
            {
                LONGLONG Value;
                FAST486_FPU_DATA_REG Temp;

                if (!Fast486ReadMemory(State,
                                       (State->PrefixFlags & FAST486_PREFIX_SEG)
                                       ? State->SegmentOverride : FAST486_REG_DS,
                                       ModRegRm.MemoryAddress,
                                       FALSE,
                                       &Value,
                                       sizeof(LONGLONG)))
                {
                    /* Exception occurred */
                    return;
                }

                Fast486FpuFromInteger(State, (LONGLONG)Value, &Temp);
                Fast486FpuPush(State, &Temp);

                break;
            }

            /* FBSTP */
            case 6:
            {
                UCHAR Buffer[10] = {0};

                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                {
                    /* Raise the invalid operation exception */
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im)
                    {
                        Fast486FpuException(State);
                        return;
                    }
                }
                else if (!Fast486FpuToPackedBcd(State, &FPU_ST(0), Buffer))
                {
                    /* Exception occurred */
                    return;
                }

                if (!Fast486WriteMemory(State,
                                        (State->PrefixFlags & FAST486_PREFIX_SEG)
                                        ? State->SegmentOverride : FAST486_REG_DS,
                                        ModRegRm.MemoryAddress,
                                        Buffer,
                                        sizeof(Buffer)))
                {
                    /* Exception occurred */
                    return;
                }

                Fast486FpuPop(State);
                break;
            }

            /* FISTP (64-bit int) */
            case 7:
            {
                LONGLONG Temp = 0LL;

                if ((FPU_GET_TAG(0) == FPU_TAG_EMPTY) || (FPU_GET_TAG(0) == FPU_TAG_SPECIAL))
                {
                    /* Raise the invalid operation exception */
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im)
                    {
                        Fast486FpuException(State);
                        return;
                    }
                }

                if (!Fast486FpuToInteger(State, &FPU_ST(0), &Temp))
                {
                    /* Exception occurred */
                    return;
                }

                if (!Fast486WriteMemory(State,
                                        (State->PrefixFlags & FAST486_PREFIX_SEG)
                                        ? State->SegmentOverride : FAST486_REG_DS,
                                        ModRegRm.MemoryAddress,
                                        &Temp,
                                        sizeof(LONGLONG)))
                {
                    /* Exception occurred */
                    return;
                }

                /* Pop the FPU stack too */
                Fast486FpuPop(State);

                break;
            }

            /* Invalid */
            default:
            {
                Fast486Exception(State, FAST486_EXCEPTION_UD);
            }
        }
    }
    else
    {
        switch (ModRegRm.Register)
        {
            /* FFREEP */
            case 0:
            {
                FPU_SET_TAG(ModRegRm.SecondRegister, FPU_TAG_EMPTY);
                Fast486FpuPop(State);

                break;
            }

            /* FXCH */
            case 1:
            {
                FAST486_FPU_DATA_REG Temp;

                if ((FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                    || FPU_GET_TAG(ModRegRm.SecondRegister) == FPU_TAG_EMPTY)
                {
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im) Fast486FpuException(State);
                    break;
                }

                /* Exchange */
                Temp = FPU_ST(0);
                FPU_ST(0) = FPU_ST(ModRegRm.SecondRegister);
                FPU_ST(ModRegRm.SecondRegister) = Temp;

                FPU_UPDATE_TAG(0);
                FPU_UPDATE_TAG(ModRegRm.SecondRegister);

                break;
            }

            /* FSTP */
            case 2:
            case 3:
            {
                FPU_ST(ModRegRm.SecondRegister) = FPU_ST(0);
                FPU_UPDATE_TAG(ModRegRm.SecondRegister);
                Fast486FpuPop(State);

                break;
            }

            /* FSTSW */
            case 4:
            {
                if (ModRegRm.SecondRegister != 0)
                {
                    /* Invalid */
                    Fast486Exception(State, FAST486_EXCEPTION_UD);
                    return;
                }

                /* Store the status word in AX */
                State->GeneralRegs[FAST486_REG_EAX].LowWord = State->FpuStatus.Value;

                break;
            }

            /* Invalid */
            default:
            {
                Fast486Exception(State, FAST486_EXCEPTION_UD);
                return;
            }
        }
    }

#endif
}

/* EOF */
