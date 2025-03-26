/*
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
 **
 ** Defines __two_to_jby64_table table
 ** Used by exp and expf
 **
 */

#include <crtdefs.h>

const unsigned long long _CRT_ALIGN(16) __two_to_jby64_table[] =
{
    0x3ff0000000000000ull,
    0x3ff02c9a3e778061ull,
    0x3ff059b0d3158574ull,
    0x3ff0874518759bc8ull,
    0x3ff0b5586cf9890full,
    0x3ff0e3ec32d3d1a2ull,
    0x3ff11301d0125b51ull,
    0x3ff1429aaea92de0ull,
    0x3ff172b83c7d517bull,
    0x3ff1a35beb6fcb75ull,
    0x3ff1d4873168b9aaull,
    0x3ff2063b88628cd6ull,
    0x3ff2387a6e756238ull,
    0x3ff26b4565e27cddull,
    0x3ff29e9df51fdee1ull,
    0x3ff2d285a6e4030bull,
    0x3ff306fe0a31b715ull,
    0x3ff33c08b26416ffull,
    0x3ff371a7373aa9cbull,
    0x3ff3a7db34e59ff7ull,
    0x3ff3dea64c123422ull,
    0x3ff4160a21f72e2aull,
    0x3ff44e086061892dull,
    0x3ff486a2b5c13cd0ull,
    0x3ff4bfdad5362a27ull,
    0x3ff4f9b2769d2ca7ull,
    0x3ff5342b569d4f82ull,
    0x3ff56f4736b527daull,
    0x3ff5ab07dd485429ull,
    0x3ff5e76f15ad2148ull,
    0x3ff6247eb03a5585ull,
    0x3ff6623882552225ull,
    0x3ff6a09e667f3bcdull,
    0x3ff6dfb23c651a2full,
    0x3ff71f75e8ec5f74ull,
    0x3ff75feb564267c9ull,
    0x3ff7a11473eb0187ull,
    0x3ff7e2f336cf4e62ull,
    0x3ff82589994cce13ull,
    0x3ff868d99b4492edull,
    0x3ff8ace5422aa0dbull,
    0x3ff8f1ae99157736ull,
    0x3ff93737b0cdc5e5ull,
    0x3ff97d829fde4e50ull,
    0x3ff9c49182a3f090ull,
    0x3ffa0c667b5de565ull,
    0x3ffa5503b23e255dull,
    0x3ffa9e6b5579fdbfull,
    0x3ffae89f995ad3adull,
    0x3ffb33a2b84f15fbull,
    0x3ffb7f76f2fb5e47ull,
    0x3ffbcc1e904bc1d2ull,
    0x3ffc199bdd85529cull,
    0x3ffc67f12e57d14bull,
    0x3ffcb720dcef9069ull,
    0x3ffd072d4a07897cull,
    0x3ffd5818dcfba487ull,
    0x3ffda9e603db3285ull,
    0x3ffdfc97337b9b5full,
    0x3ffe502ee78b3ff6ull,
    0x3ffea4afa2a490daull,
    0x3ffefa1bee615a27ull,
    0x3fff50765b6e4540ull,
    0x3fffa7c1819e90d8ull,
};
