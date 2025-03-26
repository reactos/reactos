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
 ** Defines __two_to_jby64_head_table and __two_to_jby64_tail_table tables
 ** Used in exp and pow
 **
 */

#include <crtdefs.h>

const unsigned long long _CRT_ALIGN(16) __two_to_jby64_head_table[] =
{
    0x3ff0000000000000ull,
    0x3ff02c9a30000000ull,
    0x3ff059b0d0000000ull,
    0x3ff0874510000000ull,
    0x3ff0b55860000000ull,
    0x3ff0e3ec30000000ull,
    0x3ff11301d0000000ull,
    0x3ff1429aa0000000ull,
    0x3ff172b830000000ull,
    0x3ff1a35be0000000ull,
    0x3ff1d48730000000ull,
    0x3ff2063b80000000ull,
    0x3ff2387a60000000ull,
    0x3ff26b4560000000ull,
    0x3ff29e9df0000000ull,
    0x3ff2d285a0000000ull,
    0x3ff306fe00000000ull,
    0x3ff33c08b0000000ull,
    0x3ff371a730000000ull,
    0x3ff3a7db30000000ull,
    0x3ff3dea640000000ull,
    0x3ff4160a20000000ull,
    0x3ff44e0860000000ull,
    0x3ff486a2b0000000ull,
    0x3ff4bfdad0000000ull,
    0x3ff4f9b270000000ull,
    0x3ff5342b50000000ull,
    0x3ff56f4730000000ull,
    0x3ff5ab07d0000000ull,
    0x3ff5e76f10000000ull,
    0x3ff6247eb0000000ull,
    0x3ff6623880000000ull,
    0x3ff6a09e60000000ull,
    0x3ff6dfb230000000ull,
    0x3ff71f75e0000000ull,
    0x3ff75feb50000000ull,
    0x3ff7a11470000000ull,
    0x3ff7e2f330000000ull,
    0x3ff8258990000000ull,
    0x3ff868d990000000ull,
    0x3ff8ace540000000ull,
    0x3ff8f1ae90000000ull,
    0x3ff93737b0000000ull,
    0x3ff97d8290000000ull,
    0x3ff9c49180000000ull,
    0x3ffa0c6670000000ull,
    0x3ffa5503b0000000ull,
    0x3ffa9e6b50000000ull,
    0x3ffae89f90000000ull,
    0x3ffb33a2b0000000ull,
    0x3ffb7f76f0000000ull,
    0x3ffbcc1e90000000ull,
    0x3ffc199bd0000000ull,
    0x3ffc67f120000000ull,
    0x3ffcb720d0000000ull,
    0x3ffd072d40000000ull,
    0x3ffd5818d0000000ull,
    0x3ffda9e600000000ull,
    0x3ffdfc9730000000ull,
    0x3ffe502ee0000000ull,
    0x3ffea4afa0000000ull,
    0x3ffefa1be0000000ull,
    0x3fff507650000000ull,
    0x3fffa7c180000000ull,
};

const unsigned long long _CRT_ALIGN(16) __two_to_jby64_tail_table[] =
{
    0x0000000000000000ull,
    0x3e6cef00c1dcdef9ull,
    0x3e48ac2ba1d73e2aull,
    0x3e60eb37901186beull,
    0x3e69f3121ec53172ull,
    0x3e469e8d10103a17ull,
    0x3df25b50a4ebbf1aull,
    0x3e6d525bbf668203ull,
    0x3e68faa2f5b9bef9ull,
    0x3e66df96ea796d31ull,
    0x3e368b9aa7805b80ull,
    0x3e60c519ac771dd6ull,
    0x3e6ceac470cd83f5ull,
    0x3e5789f37495e99cull,
    0x3e547f7b84b09745ull,
    0x3e5b900c2d002475ull,
    0x3e64636e2a5bd1abull,
    0x3e4320b7fa64e430ull,
    0x3e5ceaa72a9c5154ull,
    0x3e53967fdba86f24ull,
    0x3e682468446b6824ull,
    0x3e3f72e29f84325bull,
    0x3e18624b40c4dbd0ull,
    0x3e5704f3404f068eull,
    0x3e54d8a89c750e5eull,
    0x3e5a74b29ab4cf62ull,
    0x3e5a753e077c2a0full,
    0x3e5ad49f699bb2c0ull,
    0x3e6a90a852b19260ull,
    0x3e56b48521ba6f93ull,
    0x3e0d2ac258f87d03ull,
    0x3e42a91124893ecfull,
    0x3e59fcef32422cbeull,
    0x3e68ca345de441c5ull,
    0x3e61d8bee7ba46e1ull,
    0x3e59099f22fdba6aull,
    0x3e4f580c36bea881ull,
    0x3e5b3d398841740aull,
    0x3e62999c25159f11ull,
    0x3e668925d901c83bull,
    0x3e415506dadd3e2aull,
    0x3e622aee6c57304eull,
    0x3e29b8bc9e8a0387ull,
    0x3e6fbc9c9f173d24ull,
    0x3e451f8480e3e235ull,
    0x3e66bbcac96535b5ull,
    0x3e41f12ae45a1224ull,
    0x3e55e7f6fd0fac90ull,
    0x3e62b5a75abd0e69ull,
    0x3e609e2bf5ed7fa1ull,
    0x3e47daf237553d84ull,
    0x3e12f074891ee83dull,
    0x3e6b0aa538444196ull,
    0x3e6cafa29694426full,
    0x3e69df20d22a0797ull,
    0x3e640f12f71a1e45ull,
    0x3e69f7490e4bb40bull,
    0x3e4ed9942b84600dull,
    0x3e4bdcdaf5cb4656ull,
    0x3e5e2cffd89cf44cull,
    0x3e452486cc2c7b9dull,
    0x3e6cc2b44eee3fa4ull,
    0x3e66dc8a80ce9f09ull,
    0x3e39e90d82e90a7eull,
};
