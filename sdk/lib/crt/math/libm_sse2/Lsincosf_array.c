/*
 *
 * MIT License
 * -----------
 * 
 * Copyright (c) 2002-2019 Advanced Micro Devices, Inc.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this Software and associated documentaon files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 ** Defines __Lcosarray and __Lsinarray arrays.
 ** Used in sin.asm and cos.asm
 ** These coefficients are actually from Taylor series.
 */

#include <crtdefs.h>

const long long _CRT_ALIGN(16) __Lcosfarray[] =
{
    0x0bfe0000000000000ull, // -0.5              c0
    0x03fa5555555555555ull, // 0.0416667         c1
    0x0bf56c16c16c16c16ull, // -0.00138889       c2
    0x03EFA01A01A01A019ull, // 2.48016e-005      c3
    0x0be927e4fb7789f5cull, // -2.75573e-007     c4
};

const long long _CRT_ALIGN(16) __Lsinfarray[] =
{

    0x0bfc5555555555555ull, // -0.166667         s1
    0x03f81111111111111ull, // 0.00833333        s2
    0x0bf2a01a01a01a01aull, // -0.000198413      s3
    0x03ec71de3a556c734ull, // 2.75573e-006      s4
};
