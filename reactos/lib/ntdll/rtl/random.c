/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: random.c,v 1.1 2003/06/07 11:32:03 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Random number generator functions
 * FILE:              lib/ntdll/rtl/random.c
 */


#define NTOS_MODE_USER
#include <ntos.h>


static ULONG SavedValue[128] =
{
  0x4c8bc0aa, 0x4c022957, 0x2232827a, 0x2f1e7626,  /*   0 */
  0x7f8bdafb, 0x5c37d02a, 0x0ab48f72, 0x2f0c4ffa,  /*   4 */
  0x290e1954, 0x6b635f23, 0x5d3885c0, 0x74b49ff8,  /*   8 */
  0x5155fa54, 0x6214ad3f, 0x111e9c29, 0x242a3a09,  /*  12 */
  0x75932ae1, 0x40ac432e, 0x54f7ba7a, 0x585ccbd5,  /*  16 */
  0x6df5c727, 0x0374dad1, 0x7112b3f1, 0x735fc311,  /*  20 */
  0x404331a9, 0x74d97781, 0x64495118, 0x323e04be,  /*  24 */
  0x5974b425, 0x4862e393, 0x62389c1d, 0x28a68b82,  /*  28 */
  0x0f95da37, 0x7a50bbc6, 0x09b0091c, 0x22cdb7b4,  /*  32 */
  0x4faaed26, 0x66417ccd, 0x189e4bfa, 0x1ce4e8dd,  /*  36 */
  0x5274c742, 0x3bdcf4dc, 0x2d94e907, 0x32eac016,  /*  40 */
  0x26d33ca3, 0x60415a8a, 0x31f57880, 0x68c8aa52,  /*  44 */
  0x23eb16da, 0x6204f4a1, 0x373927c1, 0x0d24eb7c,  /*  48 */
  0x06dd7379, 0x2b3be507, 0x0f9c55b1, 0x2c7925eb,  /*  52 */
  0x36d67c9a, 0x42f831d9, 0x5e3961cb, 0x65d637a8,  /*  56 */
  0x24bb3820, 0x4d08e33d, 0x2188754f, 0x147e409e,  /*  60 */
  0x6a9620a0, 0x62e26657, 0x7bd8ce81, 0x11da0abb,  /*  64 */
  0x5f9e7b50, 0x23e444b6, 0x25920c78, 0x5fc894f0,  /*  68 */
  0x5e338cbb, 0x404237fd, 0x1d60f80f, 0x320a1743,  /*  72 */
  0x76013d2b, 0x070294ee, 0x695e243b, 0x56b177fd,  /*  76 */
  0x752492e1, 0x6decd52f, 0x125f5219, 0x139d2e78,  /*  80 */
  0x1898d11e, 0x2f7ee785, 0x4db405d8, 0x1a028a35,  /*  84 */
  0x63f6f323, 0x1f6d0078, 0x307cfd67, 0x3f32a78a,  /*  88 */
  0x6980796c, 0x462b3d83, 0x34b639f2, 0x53fce379,  /*  92 */
  0x74ba50f4, 0x1abc2c4b, 0x5eeaeb8d, 0x335a7a0d,  /*  96 */
  0x3973dd20, 0x0462d66b, 0x159813ff, 0x1e4643fd,  /* 100 */
  0x06bc5c62, 0x3115e3fc, 0x09101613, 0x47af2515,  /* 104 */
  0x4f11ec54, 0x78b99911, 0x3db8dd44, 0x1ec10b9b,  /* 108 */
  0x5b5506ca, 0x773ce092, 0x567be81a, 0x5475b975,  /* 112 */
  0x7a2cde1a, 0x494536f5, 0x34737bb4, 0x76d9750b,  /* 116 */
  0x2a1f6232, 0x2e49644d, 0x7dddcbe7, 0x500cebdb,  /* 120 */
  0x619dab9e, 0x48c626fe, 0x1cda3193, 0x52dabe9d   /* 124 */
};


/* FUNCTIONS ***************************************************************/

/*************************************************************************
 * RtlRandom   [NTDLL.@]
 *
 * Generates a random number
 *
 * PARAMS
 *  Seed [O] The seed of the Random function
 *
 * RETURNS
 *  It returns a random number distributed over [0..MAXLONG-1].
 */
ULONG STDCALL
RtlRandom (IN OUT PULONG Seed)
{
  ULONG Rand;
  int Pos;
  ULONG Result;

  Rand = (*Seed * 0x7fffffed + 0x7fffffc3) % 0x7fffffff;
  *Seed = (Rand * 0x7fffffed + 0x7fffffc3) % 0x7fffffff;
  Pos = *Seed & 0x7f;
  Result = SavedValue[Pos];
  SavedValue[Pos] = Rand;

  return Result;
}


/*************************************************************************
 * RtlUniform   [NTDLL.@]
 *
 * Generates an uniform random number
 *
 * PARAMS
 *  Seed [O] The seed of the Random function
 *
 * RETURNS
 *  It returns a random number uniformly distributed over [0..MAXLONG-1].
 *
 * NOTES
 *  Generates an uniform random number using D.H. Lehmer's 1948 algorithm.
 *  In our case the algorithm is:
 *
 *  Result = (*Seed * 0x7fffffed + 0x7fffffc3) % MAXLONG;
 *
 *  *Seed = Result;
 *
 * DIFFERENCES
 *  The native documentation states that the random number is
 *  uniformly distributed over [0..MAXLONG]. In reality the native
 *  function and our function return a random number uniformly
 *  distributed over [0..MAXLONG-1].
 */
ULONG STDCALL
RtlUniform (PULONG Seed)
{
  ULONG Result;

  /*
   * Instead of the algorithm stated above, we use the algorithm
   * below, which is totally equivalent (see the tests), but does
   * not use a division and therefore is faster.
   */
  Result = *Seed * 0xffffffed + 0x7fffffc3;

  if (Result == 0xffffffff || Result == 0x7ffffffe)
    {
      Result = (Result + 2) & MAXLONG;
    }
  else if (Result == 0x7fffffff)
    {
      Result = 0;
    }
  else if ((Result & 0x80000000) == 0)
    {
      Result = Result + (~Result & 1);
    }
  else
    {
      Result = (Result + (Result & 1)) & MAXLONG;
    }

  *Seed = Result;

  return Result;
}

/* EOF */
