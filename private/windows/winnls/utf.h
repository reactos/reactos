/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    utf.h

Abstract:

    This file contains the header information for the UTF module of NLS.

Revision History:

    02-06-96    JulieB    Created.

--*/



//
//  Constant Declarations.
//

#define ASCII                 0x007f

#define SHIFT_IN              '+'     // beginning of a shift sequence
#define SHIFT_OUT             '-'     // end       of a shift sequence

#define UTF8_2_MAX            0x07ff  // max UTF8 2-byte sequence (32 * 64 = 2048)
#define UTF8_1ST_OF_2         0xc0    // 110x xxxx
#define UTF8_1ST_OF_3         0xe0    // 1110 xxxx
#define UTF8_1ST_OF_4         0xf0    // 1111 xxxx
#define UTF8_TRAIL            0x80    // 10xx xxxx

#define HIGHER_6_BIT(u)       ((u) >> 12)
#define MIDDLE_6_BIT(u)       (((u) & 0x0fc0) >> 6)
#define LOWER_6_BIT(u)        ((u) & 0x003f)

#define BIT7(a)               ((a) & 0x80)
#define BIT6(a)               ((a) & 0x40)

#define HIGH_SURROGATE_START  0xd800
#define HIGH_SURROGATE_END    0xdbff
#define LOW_SURROGATE_START   0xdc00
#define LOW_SURROGATE_END     0xdfff

/////////////////////////
//                     //
//  Unicode -> UTF-7   //
//                     //
/////////////////////////

//
//  Convert one Unicode to 2 2/3 Base64 chars in a shifted sequence.
//  Each char represents a 6-bit portion of the 16-bit Unicode char.
//
CONST char cBase64[] =

  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"  // A : 000000 .... 011001  ( 0 - 25)
  "abcdefghijklmnopqrstuvwxyz"  // a : 011010 .... 110011  (26 - 51)
  "0123456789"                  // 0 : 110100 .... 111101  (52 - 61)
  "+/";                         // + : 111110, / : 111111  (62 - 63)

//
//  To determine if an ASCII char needs to be shifted.
//    1 :     to be shifted
//    0 : not to be shifted
//
CONST BOOLEAN fShiftChar[] =
{
  0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1,    // Null, Tab, LF, CR
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0,    // Space '() +,-./
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0,    // 0123456789:    ?
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //  ABCDEFGHIJKLMNO
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,    // PQRSTUVWXYZ
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //  abcdefghijklmno
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1     // pqrstuvwxyz
};




/////////////////////////
//                     //
//  UTF-7 -> Unicode   //
//                     //
/////////////////////////

//
//  Convert a Base64 char in a shifted sequence to a 6-bit portion of a
//  Unicode char.
//  -1 means it is not a Base64
//
CONST char nBitBase64[] =
{
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,   //            +   /
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,   // 0123456789
  -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,   //  ABCDEFGHIJKLMNO
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,   // PQRSTUVWXYZ
  -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,   //  abcdefghijklmno
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1    // pqrstuvwxyz
};
