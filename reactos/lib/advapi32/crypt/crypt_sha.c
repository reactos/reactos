/*
 * Copyright 2004 Filip Navara
 * Based on public domain SHA code by Steve Reid <steve@edmweb.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <advapi32.h>
#include "crypt.h"


/* SHA Context Structure Declaration */

typedef struct {
   ULONG Unknown[6];
   ULONG State[5];
   ULONG Count[2];
   UCHAR Buffer[64];
} SHA_CTX, *PSHA_CTX;

/* SHA1 Helper Macros */

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))
/* FIXME: This definition of DWORD2BE is little endian specific! */
#define DWORD2BE(x) (((x) >> 24) & 0xff) | (((x) >> 8) & 0xff00) | (((x) << 8) & 0xff0000) | (((x) << 24) & 0xff000000);
/* FIXME: This definition of blk0 is little endian specific! */
#define blk0(i) (Block[i] = (rol(Block[i],24)&0xFF00FF00)|(rol(Block[i],8)&0x00FF00FF))
#define blk1(i) (Block[i&15] = rol(Block[(i+13)&15]^Block[(i+8)&15]^Block[(i+2)&15]^Block[i&15],1))
#define f1(x,y,z) (z^(x&(y^z)))
#define f2(x,y,z) (x^y^z)
#define f3(x,y,z) ((x&y)|(z&(x|y)))
#define f4(x,y,z) (x^y^z)
/* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
#define R0(v,w,x,y,z,i) z+=f1(w,x,y)+blk0(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R1(v,w,x,y,z,i) z+=f1(w,x,y)+blk1(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R2(v,w,x,y,z,i) z+=f2(w,x,y)+blk1(i)+0x6ED9EBA1+rol(v,5);w=rol(w,30);
#define R3(v,w,x,y,z,i) z+=f3(w,x,y)+blk1(i)+0x8F1BBCDC+rol(v,5);w=rol(w,30);
#define R4(v,w,x,y,z,i) z+=f4(w,x,y)+blk1(i)+0xCA62C1D6+rol(v,5);w=rol(w,30);

/* Hash a single 512-bit block. This is the core of the algorithm. */
static VOID
SHA1Transform(ULONG State[5], UCHAR Buffer[64])
{
   ULONG a, b, c, d, e;
   ULONG *Block;

   Block = (ULONG*)Buffer;

   /* Copy Context->State[] to working variables */
   a = State[0];
   b = State[1];
   c = State[2];
   d = State[3];
   e = State[4];

   /* 4 rounds of 20 operations each. Loop unrolled. */
   R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
   R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
   R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
   R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
   R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
   R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
   R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
   R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
   R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
   R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
   R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
   R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
   R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
   R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
   R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
   R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
   R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
   R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
   R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
   R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);

   /* Add the working variables back into Context->State[] */
   State[0] += a;
   State[1] += b;
   State[2] += c;
   State[3] += d;
   State[4] += e;

   /* Wipe variables */
   a = b = c = d = e = 0;
}


/******************************************************************************
 * A_SHAInit [ADVAPI32.@]
 *
 * Initialize a SHA context structure.
 */
VOID WINAPI
A_SHAInit(PSHA_CTX Context)
{
   /* SHA1 initialization constants */
   Context->State[0] = 0x67452301;
   Context->State[1] = 0xEFCDAB89;
   Context->State[2] = 0x98BADCFE;
   Context->State[3] = 0x10325476;
   Context->State[4] = 0xC3D2E1F0;
   Context->Count[0] =
   Context->Count[1] = 0;
}

/******************************************************************************
 * A_SHAUpdate [ADVAPI32.@]
 *
 * Update a SHA context with a hashed data from supplied buffer.
 */
VOID WINAPI
A_SHAUpdate(PSHA_CTX Context, PCHAR Buffer, UINT BufferSize)
{
   ULONG BufferContentSize;

   BufferContentSize = Context->Count[1] & 63;
   Context->Count[1] += BufferSize;
   if (Context->Count[1] < BufferSize)
      Context->Count[0]++;
   Context->Count[0] += (BufferSize >> 29);

   if (BufferContentSize + BufferSize < 64)
   {
      RtlCopyMemory(&Context->Buffer[BufferContentSize], Buffer,
                    BufferSize);
   }
   else
   {
      while (BufferContentSize + BufferSize >= 64)
      {
         RtlCopyMemory(Context->Buffer + BufferContentSize, Buffer,
                       64 - BufferContentSize);
         Buffer += 64 - BufferContentSize;
         BufferSize -= 64 - BufferContentSize;
         SHA1Transform(Context->State, Context->Buffer);
         BufferContentSize = 0;
      }
      RtlCopyMemory(Context->Buffer + BufferContentSize, Buffer, BufferSize);
   }
}

/******************************************************************************
 * A_SHAFinal [ADVAPI32.@]
 *
 * Finalize SHA context and return the resulting hash.
 */
VOID WINAPI
A_SHAFinal(PSHA_CTX Context, PULONG Result)
{
   INT Pad, Index;
   CHAR Buffer[72];
   ULONG *Count;
   ULONG BufferContentSize, LengthHi, LengthLo;

   BufferContentSize = Context->Count[1] & 63;
   if (BufferContentSize >= 56)
      Pad = 56 + 64 - BufferContentSize;
   else
      Pad = 56 - BufferContentSize;

   LengthHi = (Context->Count[0] << 3) | (Context->Count[1] >> (32 - 3));
   LengthLo = (Context->Count[1] << 3);

   RtlZeroMemory(Buffer + 1, Pad - 1);
   Buffer[0] = 0x80;
   Count = (ULONG*)(Buffer + Pad);
   Count[0] = DWORD2BE(LengthHi);
   Count[1] = DWORD2BE(LengthLo);
   A_SHAUpdate(Context, Buffer, Pad + 8);

   for (Index = 0; Index < 5; Index++)
      Result[Index] = DWORD2BE(Context->State[Index]);

   A_SHAInit(Context);
}
