//
// selftest.c
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

const BYTE SymCryptTestMsg3 [ 3] = { 'a', 'b', 'c' };

const BYTE SymCryptTestKey32[32] = {
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
};

const BYTE SymCryptTestMsg16[16] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
};
