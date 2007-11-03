/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   xrdp: A Remote Desktop Protocol server.
   Copyright (C) Jay Sorg 2004-2005

   ssl calls

*/

#include "rdesktop.h"

#define APP_CC

/*****************************************************************************/
static void * g_malloc(int size, int zero)
{
  void * p;

  p = xmalloc(size);
  if (zero)
  {
    memset(p, 0, size);
  }
  return p;
}

/*****************************************************************************/
static void g_free(void * in)
{
  xfree(in);
}

/*****************************************************************************/
/*****************************************************************************/
/* rc4 stuff */
/* An implementation of the ARC4 algorithm
 *
 * Copyright (C) 2001-2003  Christophe Devine
 */
struct rc4_state
{
  int x;
  int y;
  int m[256];
};

/*****************************************************************************/
void* APP_CC
ssl_rc4_info_create(void)
{
  return g_malloc(sizeof(struct rc4_state), 1);;
}

/*****************************************************************************/
void APP_CC
ssl_rc4_info_delete(void* rc4_info)
{
  g_free(rc4_info);
}

/*****************************************************************************/
void APP_CC
ssl_rc4_set_key(void* rc4_info, char* key, int len)
{
  int i;
  int j;
  int k;
  int a;
  int* m;
  struct rc4_state* s;

  s = (struct rc4_state*)rc4_info;
  s->x = 0;
  s->y = 0;
  m = s->m;
  for (i = 0; i < 256; i++)
  {
    m[i] = i;
  }
  j = 0;
  k = 0;
  for (i = 0; i < 256; i++)
  {
    a = m[i];
    j = (unsigned char)(j + a + key[k]);
    m[i] = m[j];
    m[j] = a;
    k++;
    if (k >= len)
    {
      k = 0;
    }
  }
}

/*****************************************************************************/
void APP_CC
ssl_rc4_crypt(void* rc4_info, char* in_data, char* out_data, int len)
{
  int i;
  int x;
  int y;
  int a;
  int b;
  int* m;
  struct rc4_state* s;

  s = (struct rc4_state*)rc4_info;
  x = s->x;
  y = s->y;
  m = s->m;
  for (i = 0; i < len; i++)
  {
    x = (unsigned char)(x + 1);
    a = m[x];
    y = (unsigned char)(y + a);
    b = m[y];
    m[x] = b;
    m[y] = a;
    out_data[i] = in_data[i] ^ (m[(unsigned char)(a + b)]);
  }
  s->x = x;
  s->y = y;
}

/*****************************************************************************/
/*****************************************************************************/
/* sha1 stuff */
/* FIPS-180-1 compliant SHA-1 implementation
 *
 * Copyright (C) 2001-2003  Christophe Devine
 */
struct sha1_context
{
  int total[2];
  int state[5];
  char buffer[64];
};

/*****************************************************************************/
void* APP_CC
ssl_sha1_info_create(void)
{
  return g_malloc(sizeof(struct sha1_context), 1);
}

/*****************************************************************************/
void APP_CC
ssl_sha1_info_delete(void* sha1_info)
{
  g_free(sha1_info);
}

/*****************************************************************************/
void APP_CC
ssl_sha1_clear(void* sha1_info)
{
  struct sha1_context* ctx;

  ctx = (struct sha1_context*)sha1_info;
  memset(ctx, 0, sizeof(struct sha1_context));
  ctx->state[0] = 0x67452301;
  ctx->state[1] = 0xEFCDAB89;
  ctx->state[2] = 0x98BADCFE;
  ctx->state[3] = 0x10325476;
  ctx->state[4] = 0xC3D2E1F0;
}

#undef GET_UINT32
#define GET_UINT32(n, b, i)          \
{                                    \
  (n) = ((b)[(i) + 0] << 24) |       \
        ((b)[(i) + 1] << 16) |       \
        ((b)[(i) + 2] << 8) |        \
        ((b)[(i) + 3] << 0);         \
}

#undef PUT_UINT32
#define PUT_UINT32(n, b, i)         \
{                                   \
  (b)[(i) + 0] = ((n) >> 24);       \
  (b)[(i) + 1] = ((n) >> 16);       \
  (b)[(i) + 2] = ((n) >> 8);        \
  (b)[(i) + 3] = ((n) >> 0);        \
}

/*****************************************************************************/
static void APP_CC
sha1_process(struct sha1_context* ctx, char* in_data)
{
  int temp;
  int W[16];
  int A;
  int B;
  int C;
  int D;
  int E;
  unsigned char* data;

  data = (unsigned char*)in_data;

  GET_UINT32(W[0], data, 0);
  GET_UINT32(W[1], data, 4);
  GET_UINT32(W[2], data, 8);
  GET_UINT32(W[3], data, 12);
  GET_UINT32(W[4], data, 16);
  GET_UINT32(W[5], data, 20);
  GET_UINT32(W[6], data, 24);
  GET_UINT32(W[7], data, 28);
  GET_UINT32(W[8], data, 32);
  GET_UINT32(W[9], data, 36);
  GET_UINT32(W[10], data, 40);
  GET_UINT32(W[11], data, 44);
  GET_UINT32(W[12], data, 48);
  GET_UINT32(W[13], data, 52);
  GET_UINT32(W[14], data, 56);
  GET_UINT32(W[15], data, 60);

#define S(x, n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define R(t)                        \
(                                   \
  temp = W[(t - 3) & 0x0F] ^        \
         W[(t - 8) & 0x0F] ^        \
         W[(t - 14) & 0x0F] ^       \
         W[(t - 0) & 0x0F],         \
         (W[t & 0x0F] = S(temp, 1)) \
)

#undef P
#define P(a, b, c, d, e, x)          \
{                                    \
  e += S(a, 5) + F(b, c, d) + K + x; \
  b = S(b, 30);                      \
}

  A = ctx->state[0];
  B = ctx->state[1];
  C = ctx->state[2];
  D = ctx->state[3];
  E = ctx->state[4];

#define F(x, y, z) (z ^ (x & (y ^ z)))
#define K 0x5A827999

  P(A, B, C, D, E, W[0]);
  P(E, A, B, C, D, W[1]);
  P(D, E, A, B, C, W[2]);
  P(C, D, E, A, B, W[3]);
  P(B, C, D, E, A, W[4]);
  P(A, B, C, D, E, W[5]);
  P(E, A, B, C, D, W[6]);
  P(D, E, A, B, C, W[7]);
  P(C, D, E, A, B, W[8]);
  P(B, C, D, E, A, W[9]);
  P(A, B, C, D, E, W[10]);
  P(E, A, B, C, D, W[11]);
  P(D, E, A, B, C, W[12]);
  P(C, D, E, A, B, W[13]);
  P(B, C, D, E, A, W[14]);
  P(A, B, C, D, E, W[15]);
  P(E, A, B, C, D, R(16));
  P(D, E, A, B, C, R(17));
  P(C, D, E, A, B, R(18));
  P(B, C, D, E, A, R(19));

#undef K
#undef F

#define F(x, y, z) (x ^ y ^ z)
#define K 0x6ED9EBA1

  P(A, B, C, D, E, R(20));
  P(E, A, B, C, D, R(21));
  P(D, E, A, B, C, R(22));
  P(C, D, E, A, B, R(23));
  P(B, C, D, E, A, R(24));
  P(A, B, C, D, E, R(25));
  P(E, A, B, C, D, R(26));
  P(D, E, A, B, C, R(27));
  P(C, D, E, A, B, R(28));
  P(B, C, D, E, A, R(29));
  P(A, B, C, D, E, R(30));
  P(E, A, B, C, D, R(31));
  P(D, E, A, B, C, R(32));
  P(C, D, E, A, B, R(33));
  P(B, C, D, E, A, R(34));
  P(A, B, C, D, E, R(35));
  P(E, A, B, C, D, R(36));
  P(D, E, A, B, C, R(37));
  P(C, D, E, A, B, R(38));
  P(B, C, D, E, A, R(39));

#undef K
#undef F

#define F(x, y, z) ((x & y) | (z & (x | y)))
#define K 0x8F1BBCDC

  P(A, B, C, D, E, R(40));
  P(E, A, B, C, D, R(41));
  P(D, E, A, B, C, R(42));
  P(C, D, E, A, B, R(43));
  P(B, C, D, E, A, R(44));
  P(A, B, C, D, E, R(45));
  P(E, A, B, C, D, R(46));
  P(D, E, A, B, C, R(47));
  P(C, D, E, A, B, R(48));
  P(B, C, D, E, A, R(49));
  P(A, B, C, D, E, R(50));
  P(E, A, B, C, D, R(51));
  P(D, E, A, B, C, R(52));
  P(C, D, E, A, B, R(53));
  P(B, C, D, E, A, R(54));
  P(A, B, C, D, E, R(55));
  P(E, A, B, C, D, R(56));
  P(D, E, A, B, C, R(57));
  P(C, D, E, A, B, R(58));
  P(B, C, D, E, A, R(59));

#undef K
#undef F

#define F(x, y, z) (x ^ y ^ z)
#define K 0xCA62C1D6

  P(A, B, C, D, E, R(60));
  P(E, A, B, C, D, R(61));
  P(D, E, A, B, C, R(62));
  P(C, D, E, A, B, R(63));
  P(B, C, D, E, A, R(64));
  P(A, B, C, D, E, R(65));
  P(E, A, B, C, D, R(66));
  P(D, E, A, B, C, R(67));
  P(C, D, E, A, B, R(68));
  P(B, C, D, E, A, R(69));
  P(A, B, C, D, E, R(70));
  P(E, A, B, C, D, R(71));
  P(D, E, A, B, C, R(72));
  P(C, D, E, A, B, R(73));
  P(B, C, D, E, A, R(74));
  P(A, B, C, D, E, R(75));
  P(E, A, B, C, D, R(76));
  P(D, E, A, B, C, R(77));
  P(C, D, E, A, B, R(78));
  P(B, C, D, E, A, R(79));

#undef K
#undef F

  ctx->state[0] += A;
  ctx->state[1] += B;
  ctx->state[2] += C;
  ctx->state[3] += D;
  ctx->state[4] += E;
}

/*****************************************************************************/
void APP_CC
ssl_sha1_transform(void* sha1_info, char* data, int len)
{
  int left;
  int fill;
  struct sha1_context* ctx;

  ctx = (struct sha1_context*)sha1_info;
  if (len == 0)
  {
    return;
  }
  left = ctx->total[0] & 0x3F;
  fill = 64 - left;
  ctx->total[0] += len;
  ctx->total[0] &= 0xFFFFFFFF;
  if (ctx->total[0] < len)
  {
    ctx->total[1]++;
  }
  if (left && (len >= fill))
  {
    memcpy(ctx->buffer + left, data, fill);
    sha1_process(ctx, ctx->buffer);
    len -= fill;
    data += fill;
    left = 0;
  }
  while (len >= 64)
  {
    sha1_process(ctx, data);
    len -= 64;
    data += 64;
  }
  if (len != 0)
  {
    memcpy(ctx->buffer + left, data, len);
  }
}

static unsigned char sha1_padding[64] =
{
  0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*****************************************************************************/
void APP_CC
ssl_sha1_complete(void* sha1_info, char* data)
{
  int last;
  int padn;
  int high;
  int low;
  char msglen[8];
  struct sha1_context* ctx;

  ctx = (struct sha1_context*)sha1_info;
  high = (ctx->total[0] >> 29) | (ctx->total[1] << 3);
  low = (ctx->total[0] << 3);
  PUT_UINT32(high, msglen, 0);
  PUT_UINT32(low, msglen, 4);
  last = ctx->total[0] & 0x3F;
  padn = (last < 56) ? (56 - last) : (120 - last);
  ssl_sha1_transform(ctx, sha1_padding, padn);
  ssl_sha1_transform(ctx, msglen, 8);
  PUT_UINT32(ctx->state[0], data, 0);
  PUT_UINT32(ctx->state[1], data, 4);
  PUT_UINT32(ctx->state[2], data, 8);
  PUT_UINT32(ctx->state[3], data, 12);
  PUT_UINT32(ctx->state[4], data, 16);
}

/*****************************************************************************/
/*****************************************************************************/
/* md5 stuff */
/* RFC 1321 compliant MD5 implementation
 *
 * Copyright (C) 2001-2003  Christophe Devine
 */

struct md5_context
{
  int total[2];
  int state[4];
  char buffer[64];
};

/*****************************************************************************/
void* APP_CC
ssl_md5_info_create(void)
{
  return g_malloc(sizeof(struct md5_context), 1);
}

/*****************************************************************************/
void APP_CC
ssl_md5_info_delete(void* md5_info)
{
  g_free(md5_info);
}

/*****************************************************************************/
void APP_CC
ssl_md5_clear(void* md5_info)
{
  struct md5_context* ctx;

  ctx = (struct md5_context*)md5_info;
  memset(ctx, 0, sizeof(struct md5_context));
  ctx->state[0] = 0x67452301;
  ctx->state[1] = 0xEFCDAB89;
  ctx->state[2] = 0x98BADCFE;
  ctx->state[3] = 0x10325476;
}

#undef GET_UINT32
#define GET_UINT32(n, b, i)          \
{                                    \
  (n) = ((b)[(i) + 0] << 0) |        \
        ((b)[(i) + 1] << 8) |        \
        ((b)[(i) + 2] << 16) |       \
        ((b)[(i) + 3] << 24);        \
}

#undef PUT_UINT32
#define PUT_UINT32(n, b, i)          \
{                                    \
  (b)[(i) + 0] = ((n) >> 0);         \
  (b)[(i) + 1] = ((n) >> 8);         \
  (b)[(i) + 2] = ((n) >> 16);        \
  (b)[(i) + 3] = ((n) >> 24);        \
}

/*****************************************************************************/
static void
md5_process(struct md5_context* ctx, char* in_data)
{
  int X[16];
  int A;
  int B;
  int C;
  int D;
  unsigned char* data;

  data = (unsigned char*)in_data;
  GET_UINT32(X[0], data, 0);
  GET_UINT32(X[1], data, 4);
  GET_UINT32(X[2], data, 8);
  GET_UINT32(X[3], data, 12);
  GET_UINT32(X[4], data, 16);
  GET_UINT32(X[5], data, 20);
  GET_UINT32(X[6], data, 24);
  GET_UINT32(X[7], data, 28);
  GET_UINT32(X[8], data, 32);
  GET_UINT32(X[9], data, 36);
  GET_UINT32(X[10], data, 40);
  GET_UINT32(X[11], data, 44);
  GET_UINT32(X[12], data, 48);
  GET_UINT32(X[13], data, 52);
  GET_UINT32(X[14], data, 56);
  GET_UINT32(X[15], data, 60);

#define S(x, n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#undef P
#define P(a, b, c, d, k, s, t) \
{                              \
  a += F(b, c, d) + X[k] + t;  \
  a = S(a, s) + b;             \
}

  A = ctx->state[0];
  B = ctx->state[1];
  C = ctx->state[2];
  D = ctx->state[3];

#define F(x, y, z) (z ^ (x & (y ^ z)))

  P(A, B, C, D,  0,  7, 0xD76AA478);
  P(D, A, B, C,  1, 12, 0xE8C7B756);
  P(C, D, A, B,  2, 17, 0x242070DB);
  P(B, C, D, A,  3, 22, 0xC1BDCEEE);
  P(A, B, C, D,  4,  7, 0xF57C0FAF);
  P(D, A, B, C,  5, 12, 0x4787C62A);
  P(C, D, A, B,  6, 17, 0xA8304613);
  P(B, C, D, A,  7, 22, 0xFD469501);
  P(A, B, C, D,  8,  7, 0x698098D8);
  P(D, A, B, C,  9, 12, 0x8B44F7AF);
  P(C, D, A, B, 10, 17, 0xFFFF5BB1);
  P(B, C, D, A, 11, 22, 0x895CD7BE);
  P(A, B, C, D, 12,  7, 0x6B901122);
  P(D, A, B, C, 13, 12, 0xFD987193);
  P(C, D, A, B, 14, 17, 0xA679438E);
  P(B, C, D, A, 15, 22, 0x49B40821);

#undef F

#define F(x, y, z) (y ^ (z & (x ^ y)))

  P(A, B, C, D,  1,  5, 0xF61E2562);
  P(D, A, B, C,  6,  9, 0xC040B340);
  P(C, D, A, B, 11, 14, 0x265E5A51);
  P(B, C, D, A,  0, 20, 0xE9B6C7AA);
  P(A, B, C, D,  5,  5, 0xD62F105D);
  P(D, A, B, C, 10,  9, 0x02441453);
  P(C, D, A, B, 15, 14, 0xD8A1E681);
  P(B, C, D, A,  4, 20, 0xE7D3FBC8);
  P(A, B, C, D,  9,  5, 0x21E1CDE6);
  P(D, A, B, C, 14,  9, 0xC33707D6);
  P(C, D, A, B,  3, 14, 0xF4D50D87);
  P(B, C, D, A,  8, 20, 0x455A14ED);
  P(A, B, C, D, 13,  5, 0xA9E3E905);
  P(D, A, B, C,  2,  9, 0xFCEFA3F8);
  P(C, D, A, B,  7, 14, 0x676F02D9);
  P(B, C, D, A, 12, 20, 0x8D2A4C8A);

#undef F

#define F(x, y, z) (x ^ y ^ z)

  P(A, B, C, D,  5,  4, 0xFFFA3942);
  P(D, A, B, C,  8, 11, 0x8771F681);
  P(C, D, A, B, 11, 16, 0x6D9D6122);
  P(B, C, D, A, 14, 23, 0xFDE5380C);
  P(A, B, C, D,  1,  4, 0xA4BEEA44);
  P(D, A, B, C,  4, 11, 0x4BDECFA9);
  P(C, D, A, B,  7, 16, 0xF6BB4B60);
  P(B, C, D, A, 10, 23, 0xBEBFBC70);
  P(A, B, C, D, 13,  4, 0x289B7EC6);
  P(D, A, B, C,  0, 11, 0xEAA127FA);
  P(C, D, A, B,  3, 16, 0xD4EF3085);
  P(B, C, D, A,  6, 23, 0x04881D05);
  P(A, B, C, D,  9,  4, 0xD9D4D039);
  P(D, A, B, C, 12, 11, 0xE6DB99E5);
  P(C, D, A, B, 15, 16, 0x1FA27CF8);
  P(B, C, D, A,  2, 23, 0xC4AC5665);

#undef F

#define F(x, y, z) (y ^ (x | ~z))

  P(A, B, C, D,  0,  6, 0xF4292244);
  P(D, A, B, C,  7, 10, 0x432AFF97);
  P(C, D, A, B, 14, 15, 0xAB9423A7);
  P(B, C, D, A,  5, 21, 0xFC93A039);
  P(A, B, C, D, 12,  6, 0x655B59C3);
  P(D, A, B, C,  3, 10, 0x8F0CCC92);
  P(C, D, A, B, 10, 15, 0xFFEFF47D);
  P(B, C, D, A,  1, 21, 0x85845DD1);
  P(A, B, C, D,  8,  6, 0x6FA87E4F);
  P(D, A, B, C, 15, 10, 0xFE2CE6E0);
  P(C, D, A, B,  6, 15, 0xA3014314);
  P(B, C, D, A, 13, 21, 0x4E0811A1);
  P(A, B, C, D,  4,  6, 0xF7537E82);
  P(D, A, B, C, 11, 10, 0xBD3AF235);
  P(C, D, A, B,  2, 15, 0x2AD7D2BB);
  P(B, C, D, A,  9, 21, 0xEB86D391);

#undef F

  ctx->state[0] += A;
  ctx->state[1] += B;
  ctx->state[2] += C;
  ctx->state[3] += D;
}

/*****************************************************************************/
void APP_CC
ssl_md5_transform(void* md5_info, char* data, int len)
{
  int left;
  int fill;
  struct md5_context* ctx;

  ctx = (struct md5_context*)md5_info;
  if (len == 0)
  {
    return;
  }
  left = ctx->total[0] & 0x3F;
  fill = 64 - left;
  ctx->total[0] += len;
  ctx->total[0] &= 0xFFFFFFFF;
  if (ctx->total[0] < len)
  {
    ctx->total[1]++;
  }
  if (left && (len >= fill))
  {
    memcpy(ctx->buffer + left, data, fill);
    md5_process(ctx, ctx->buffer);
    len -= fill;
    data += fill;
    left = 0;
  }
  while (len >= 64)
  {
    md5_process(ctx, data);
    len -= 64;
    data += 64;
  }
  if (len != 0)
  {
    memcpy(ctx->buffer + left, data, len);
  }
}

static unsigned char md5_padding[64] =
{
  0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*****************************************************************************/
void APP_CC
ssl_md5_complete(void* md5_info, char* data)
{
  int last;
  int padn;
  int high;
  int low;
  char msglen[8];
  struct md5_context* ctx;

  ctx = (struct md5_context*)md5_info;
  high = (ctx->total[0] >> 29) | (ctx->total[1] << 3);
  low = (ctx->total[0] << 3);
  PUT_UINT32(low, msglen, 0);
  PUT_UINT32(high, msglen, 4);
  last = ctx->total[0] & 0x3F;
  padn = (last < 56) ? (56 - last) : (120 - last);
  ssl_md5_transform(ctx, md5_padding, padn);
  ssl_md5_transform(ctx, msglen, 8);
  PUT_UINT32(ctx->state[0], data, 0);
  PUT_UINT32(ctx->state[1], data, 4);
  PUT_UINT32(ctx->state[2], data, 8);
  PUT_UINT32(ctx->state[3], data, 12);
}

/*****************************************************************************/
/*****************************************************************************/
/* big number stuff */
/******************* SHORT COPYRIGHT NOTICE*************************
This source code is part of the BigDigits multiple-precision
arithmetic library Version 1.0 originally written by David Ireland,
copyright (c) 2001 D.I. Management Services Pty Limited, all rights
reserved. It is provided "as is" with no warranties. You may use
this software under the terms of the full copyright notice
"bigdigitsCopyright.txt" that should have been included with
this library. To obtain a copy send an email to
<code@di-mgt.com.au> or visit <www.di-mgt.com.au/crypto.html>.
This notice must be retained in any copy.
****************** END OF COPYRIGHT NOTICE*************************/
/************************* COPYRIGHT NOTICE*************************
This source code is part of the BigDigits multiple-precision
arithmetic library Version 1.0 originally written by David Ireland,
copyright (c) 2001 D.I. Management Services Pty Limited, all rights
reserved. You are permitted to use compiled versions of this code as
part of your own executable files and to distribute unlimited copies
of such executable files for any purposes including commercial ones
provided you keep the copyright notices intact in the source code
and that you ensure that the following characters remain in any
object or executable files you distribute:

"Contains multiple-precision arithmetic code originally written
by David Ireland, copyright (c) 2001 by D.I. Management Services
Pty Limited <www.di-mgt.com.au>, and is used with permission."

David Ireland and DI Management Services Pty Limited make no
representations concerning either the merchantability of this
software or the suitability of this software for any particular
purpose. It is provided "as is" without express or implied warranty
of any kind.

Please forward any comments and bug reports to <code@di-mgt.com.au>.
The latest version of the source code can be downloaded from
www.di-mgt.com.au/crypto.html.
****************** END OF COPYRIGHT NOTICE*************************/

typedef unsigned int DIGIT_T;
#define HIBITMASK 0x80000000
#define MAX_DIG_LEN 51
#define MAX_DIGIT 0xffffffff
#define BITS_PER_DIGIT 32
#define MAX_HALF_DIGIT 0xffff
#define B_J (MAX_HALF_DIGIT + 1)
#define LOHALF(x) ((DIGIT_T)((x) & 0xffff))
#define HIHALF(x) ((DIGIT_T)((x) >> 16 & 0xffff))
#define TOHIGH(x) ((DIGIT_T)((x) << 16))

#define mpNEXTBITMASK(mask, n) \
{ \
  if (mask == 1) \
  { \
    mask = HIBITMASK; \
    n--; \
  } \
  else \
  { \
    mask >>= 1; \
  } \
}

/*****************************************************************************/
static DIGIT_T APP_CC
mpAdd(DIGIT_T* w, DIGIT_T* u, DIGIT_T* v, unsigned int ndigits)
{
  /* Calculates w = u + v
     where w, u, v are multiprecision integers of ndigits each
     Returns carry if overflow. Carry = 0 or 1.

     Ref: Knuth Vol 2 Ch 4.3.1 p 266 Algorithm A. */
  DIGIT_T k;
  unsigned int j;

  /* Step A1. Initialise */
  k = 0;
  for (j = 0; j < ndigits; j++)
  {
    /* Step A2. Add digits w_j = (u_j + v_j + k)
       Set k = 1 if carry (overflow) occurs */
    w[j] = u[j] + k;
    if (w[j] < k)
    {
      k = 1;
    }
    else
    {
      k = 0;
    }
    w[j] += v[j];
    if (w[j] < v[j])
    {
      k++;
    }
  } /* Step A3. Loop on j */
  return k; /* w_n = k */
}

/*****************************************************************************/
static void APP_CC
mpSetDigit(DIGIT_T* a, DIGIT_T d, unsigned int ndigits)
{ /* Sets a = d where d is a single digit */
  unsigned int i;

  for (i = 1; i < ndigits; i++)
  {
    a[i] = 0;
  }
  a[0] = d;
}

/*****************************************************************************/
static int APP_CC
mpCompare(DIGIT_T* a, DIGIT_T* b, unsigned int ndigits)
{
  /* Returns sign of (a - b) */
  if (ndigits == 0)
  {
    return 0;
  }
  while (ndigits--)
  {
    if (a[ndigits] > b[ndigits])
    {
      return 1; /* GT */
    }
    if (a[ndigits] < b[ndigits])
    {
      return -1; /* LT */
    }
  }
  return 0; /* EQ */
}

/*****************************************************************************/
static void APP_CC
mpSetZero(DIGIT_T* a, unsigned int ndigits)
{ /* Sets a = 0 */
  unsigned int i;

  for (i = 0; i < ndigits; i++)
  {
    a[i] = 0;
  }
}

/*****************************************************************************/
static void APP_CC
mpSetEqual(DIGIT_T* a, DIGIT_T* b, unsigned int ndigits)
{  /* Sets a = b */
  unsigned int i;

  for (i = 0; i < ndigits; i++)
  {
    a[i] = b[i];
  }
}

/*****************************************************************************/
static unsigned int APP_CC
mpSizeof(DIGIT_T* a, unsigned int ndigits)
{  /* Returns size of significant digits in a */
  while (ndigits--)
  {
    if (a[ndigits] != 0)
    {
      return (++ndigits);
    }
  }
  return 0;
}

/*****************************************************************************/
static DIGIT_T APP_CC
mpShiftLeft(DIGIT_T* a, DIGIT_T* b, unsigned int x, unsigned int ndigits)
{ /* Computes a = b << x */
  unsigned int i;
  unsigned int y;
  DIGIT_T mask;
  DIGIT_T carry;
  DIGIT_T nextcarry;

  /* Check input - NB unspecified result */
  if (x >= BITS_PER_DIGIT)
  {
    return 0;
  }
  /* Construct mask */
  mask = HIBITMASK;
  for (i = 1; i < x; i++)
  {
    mask = (mask >> 1) | mask;
  }
  if (x == 0)
  {
    mask = 0x0;
  }
  y = BITS_PER_DIGIT - x;
  carry = 0;
  for (i = 0; i < ndigits; i++)
  {
    nextcarry = (b[i] & mask) >> y;
    a[i] = b[i] << x | carry;
    carry = nextcarry;
  }
  return carry;
}

/*****************************************************************************/
static DIGIT_T APP_CC
mpShiftRight(DIGIT_T* a, DIGIT_T* b, unsigned int x, unsigned int ndigits)
{ /* Computes a = b >> x */
  unsigned int i;
  unsigned int y;
  DIGIT_T mask;
  DIGIT_T carry;
  DIGIT_T nextcarry;

  /* Check input  - NB unspecified result */
  if (x >= BITS_PER_DIGIT)
  {
    return 0;
  }
  /* Construct mask */
  mask = 0x1;
  for (i = 1; i < x; i++)
  {
    mask = (mask << 1) | mask;
  }
  if (x == 0)
  {
    mask = 0x0;
  }
  y = BITS_PER_DIGIT - x;
  carry = 0;
  i = ndigits;
  while (i--)
  {
    nextcarry = (b[i] & mask) << y;
    a[i] = b[i] >> x | carry;
    carry = nextcarry;
  }
  return carry;
}

/*****************************************************************************/
static void APP_CC
spMultSub(DIGIT_T* uu, DIGIT_T qhat, DIGIT_T v1, DIGIT_T v0)
{
  /* Compute uu = uu - q(v1v0)
     where uu = u3u2u1u0, u3 = 0
     and u_n, v_n are all half-digits
     even though v1, v2 are passed as full digits. */
  DIGIT_T p0;
  DIGIT_T p1;
  DIGIT_T t;

  p0 = qhat * v0;
  p1 = qhat * v1;
  t = p0 + TOHIGH(LOHALF(p1));
  uu[0] -= t;
  if (uu[0] > MAX_DIGIT - t)
  {
    uu[1]--; /* Borrow */
  }
  uu[1] -= HIHALF(p1);
}

/*****************************************************************************/
static int APP_CC
spMultiply(DIGIT_T* p, DIGIT_T x, DIGIT_T y)
{ /* Computes p = x * y */
  /* Ref: Arbitrary Precision Computation
     http://numbers.computation.free.fr/Constants/constants.html

         high    p1                p0     low
        +--------+--------+--------+--------+
        |      x1*y1      |      x0*y0      |
        +--------+--------+--------+--------+
               +-+--------+--------+
               |1| (x0*y1 + x1*y1) |
               +-+--------+--------+
                ^carry from adding (x0*y1+x1*y1) together
                        +-+
                        |1|< carry from adding LOHALF t
                        +-+  to high half of p0 */
  DIGIT_T x0;
  DIGIT_T y0;
  DIGIT_T x1;
  DIGIT_T y1;
  DIGIT_T t;
  DIGIT_T u;
  DIGIT_T carry;

  /* Split each x,y into two halves
     x = x0 + B * x1
     y = y0 + B * y1
     where B = 2^16, half the digit size
     Product is
        xy = x0y0 + B(x0y1 + x1y0) + B^2(x1y1) */

  x0 = LOHALF(x);
  x1 = HIHALF(x);
  y0 = LOHALF(y);
  y1 = HIHALF(y);

  /* Calc low part - no carry */
  p[0] = x0 * y0;

  /* Calc middle part */
  t = x0 * y1;
  u = x1 * y0;
  t += u;
  if (t < u)
  {
    carry = 1;
  }
  else
  {
    carry = 0;
  }
  /* This carry will go to high half of p[1]
     + high half of t into low half of p[1] */
  carry = TOHIGH(carry) + HIHALF(t);

  /* Add low half of t to high half of p[0] */
  t = TOHIGH(t);
  p[0] += t;
  if (p[0] < t)
  {
    carry++;
  }

  p[1] = x1 * y1;
  p[1] += carry;

  return 0;
}

/*****************************************************************************/
static DIGIT_T APP_CC
spDivide(DIGIT_T* q, DIGIT_T* r, DIGIT_T* u, DIGIT_T v)
{ /* Computes quotient q = u / v, remainder r = u mod v
     where u is a double digit
     and q, v, r are single precision digits.
     Returns high digit of quotient (max value is 1)
     Assumes normalised such that v1 >= b/2
     where b is size of HALF_DIGIT
     i.e. the most significant bit of v should be one

     In terms of half-digits in Knuth notation:
     (q2q1q0) = (u4u3u2u1u0) / (v1v0)
     (r1r0) = (u4u3u2u1u0) mod (v1v0)
     for m = 2, n = 2 where u4 = 0
     q2 is either 0 or 1.
     We set q = (q1q0) and return q2 as "overflow' */
  DIGIT_T qhat;
  DIGIT_T rhat;
  DIGIT_T t;
  DIGIT_T v0;
  DIGIT_T v1;
  DIGIT_T u0;
  DIGIT_T u1;
  DIGIT_T u2;
  DIGIT_T u3;
  DIGIT_T uu[2];
  DIGIT_T q2;

  /* Check for normalisation */
  if (!(v & HIBITMASK))
  {
    *q = *r = 0;
    return MAX_DIGIT;
  }

  /* Split up into half-digits */
  v0 = LOHALF(v);
  v1 = HIHALF(v);
  u0 = LOHALF(u[0]);
  u1 = HIHALF(u[0]);
  u2 = LOHALF(u[1]);
  u3 = HIHALF(u[1]);

  /* Do three rounds of Knuth Algorithm D Vol 2 p272 */

  /* ROUND 1. Set j = 2 and calculate q2 */
  /* Estimate qhat = (u4u3)/v1  = 0 or 1
     then set (u4u3u2) -= qhat(v1v0)
     where u4 = 0. */
  qhat = u3 / v1;
  if (qhat > 0)
  {
    rhat = u3 - qhat * v1;
    t = TOHIGH(rhat) | u2;
    if (qhat * v0 > t)
    {
      qhat--;
    }
  }
  uu[1] = 0; /* (u4) */
  uu[0] = u[1]; /* (u3u2) */
  if (qhat > 0)
  {
    /* (u4u3u2) -= qhat(v1v0) where u4 = 0 */
    spMultSub(uu, qhat, v1, v0);
    if (HIHALF(uu[1]) != 0)
    { /* Add back */
      qhat--;
      uu[0] += v;
      uu[1] = 0;
    }
  }
  q2 = qhat;
  /* ROUND 2. Set j = 1 and calculate q1 */
  /* Estimate qhat = (u3u2) / v1
     then set (u3u2u1) -= qhat(v1v0) */
  t = uu[0];
  qhat = t / v1;
  rhat = t - qhat * v1;
  /* Test on v0 */
  t = TOHIGH(rhat) | u1;
  if ((qhat == B_J) || (qhat * v0 > t))
  {
    qhat--;
    rhat += v1;
    t = TOHIGH(rhat) | u1;
    if ((rhat < B_J) && (qhat * v0 > t))
    {
      qhat--;
    }
  }
  /* Multiply and subtract
     (u3u2u1)' = (u3u2u1) - qhat(v1v0) */
  uu[1] = HIHALF(uu[0]); /* (0u3) */
  uu[0] = TOHIGH(LOHALF(uu[0])) | u1; /* (u2u1) */
  spMultSub(uu, qhat, v1, v0);
  if (HIHALF(uu[1]) != 0)
  { /* Add back */
    qhat--;
    uu[0] += v;
    uu[1] = 0;
  }
  /* q1 = qhat */
  *q = TOHIGH(qhat);
  /* ROUND 3. Set j = 0 and calculate q0 */
  /* Estimate qhat = (u2u1) / v1
     then set (u2u1u0) -= qhat(v1v0) */
  t = uu[0];
  qhat = t / v1;
  rhat = t - qhat * v1;
  /* Test on v0 */
  t = TOHIGH(rhat) | u0;
  if ((qhat == B_J) || (qhat * v0 > t))
  {
    qhat--;
    rhat += v1;
    t = TOHIGH(rhat) | u0;
    if ((rhat < B_J) && (qhat * v0 > t))
    {
      qhat--;
    }
  }
  /* Multiply and subtract
     (u2u1u0)" = (u2u1u0)' - qhat(v1v0) */
  uu[1] = HIHALF(uu[0]); /* (0u2) */
  uu[0] = TOHIGH(LOHALF(uu[0])) | u0; /* (u1u0) */
  spMultSub(uu, qhat, v1, v0);
  if (HIHALF(uu[1]) != 0)
  { /* Add back */
    qhat--;
    uu[0] += v;
    uu[1] = 0;
  }
  /* q0 = qhat */
  *q |= LOHALF(qhat);
  /* Remainder is in (u1u0) i.e. uu[0] */
  *r = uu[0];
  return q2;
}

/*****************************************************************************/
static int APP_CC
QhatTooBig(DIGIT_T qhat, DIGIT_T rhat, DIGIT_T vn2, DIGIT_T ujn2)
{ /* Returns true if Qhat is too big
     i.e. if (Qhat * Vn-2) > (b.Rhat + Uj+n-2) */
  DIGIT_T t[2];

  spMultiply(t, qhat, vn2);
  if (t[1] < rhat)
  {
    return 0;
  }
  else if (t[1] > rhat)
  {
    return 1;
  }
  else if (t[0] > ujn2)
  {
    return 1;
  }
  return 0;
}

/*****************************************************************************/
static DIGIT_T APP_CC
mpShortDiv(DIGIT_T* q, DIGIT_T* u, DIGIT_T v, unsigned int ndigits)
{
  /* Calculates quotient q = u div v
     Returns remainder r = u mod v
     where q, u are multiprecision integers of ndigits each
     and d, v are single precision digits.

     Makes no assumptions about normalisation.

     Ref: Knuth Vol 2 Ch 4.3.1 Exercise 16 p625 */
  unsigned int j;
  unsigned int shift;
  DIGIT_T t[2];
  DIGIT_T r;
  DIGIT_T bitmask;
  DIGIT_T overflow;
  DIGIT_T* uu;

  if (ndigits == 0)
  {
    return 0;
  }
  if (v == 0)
  {
    return 0; /* Divide by zero error */
  }
  /* Normalise first */
  /* Requires high bit of V
     to be set, so find most signif. bit then shift left,
     i.e. d = 2^shift, u' = u * d, v' = v * d. */
  bitmask = HIBITMASK;
  for (shift = 0; shift < BITS_PER_DIGIT; shift++)
  {
    if (v & bitmask)
    {
      break;
    }
    bitmask >>= 1;
  }
  v <<= shift;
  overflow = mpShiftLeft(q, u, shift, ndigits);
  uu = q;
  /* Step S1 - modified for extra digit. */
  r = overflow; /* New digit Un */
  j = ndigits;
  while (j--)
  {
    /* Step S2. */
    t[1] = r;
    t[0] = uu[j];
    overflow = spDivide(&q[j], &r, t, v);
  }
  /* Unnormalise */
  r >>= shift;
  return r;
}

/*****************************************************************************/
static DIGIT_T APP_CC
mpMultSub(DIGIT_T wn, DIGIT_T* w, DIGIT_T* v, DIGIT_T q, unsigned int n)
{ /* Compute w = w - qv
     where w = (WnW[n-1]...W[0])
     return modified Wn. */
  DIGIT_T k;
  DIGIT_T t[2];
  unsigned int i;

  if (q == 0) /* No change */
  {
    return wn;
  }
  k = 0;
  for (i = 0; i < n; i++)
  {
    spMultiply(t, q, v[i]);
    w[i] -= k;
    if (w[i] > MAX_DIGIT - k)
    {
      k = 1;
    }
    else
    {
      k = 0;
    }
    w[i] -= t[0];
    if (w[i] > MAX_DIGIT - t[0])
    {
      k++;
    }
    k += t[1];
  }
  /* Cope with Wn not stored in array w[0..n-1] */
  wn -= k;
  return wn;
}

/*****************************************************************************/
static int APP_CC
mpDivide(DIGIT_T* q, DIGIT_T* r, DIGIT_T* u, unsigned int udigits,
         DIGIT_T* v, unsigned int vdigits)
{ /* Computes quotient q = u / v and remainder r = u mod v
     where q, r, u are multiple precision digits
     all of udigits and the divisor v is vdigits.

     Ref: Knuth Vol 2 Ch 4.3.1 p 272 Algorithm D.

     Do without extra storage space, i.e. use r[] for
     normalised u[], unnormalise v[] at end, and cope with
     extra digit Uj+n added to u after normalisation.

     WARNING: this trashes q and r first, so cannot do
     u = u / v or v = u mod v. */
  unsigned int shift;
  int n;
  int m;
  int j;
  int qhatOK;
  int cmp;
  DIGIT_T bitmask;
  DIGIT_T overflow;
  DIGIT_T qhat;
  DIGIT_T rhat;
  DIGIT_T t[2];
  DIGIT_T* uu;
  DIGIT_T* ww;

  /* Clear q and r */
  mpSetZero(q, udigits);
  mpSetZero(r, udigits);
  /* Work out exact sizes of u and v */
  n = (int)mpSizeof(v, vdigits);
  m = (int)mpSizeof(u, udigits);
  m -= n;
  /* Catch special cases */
  if (n == 0)
  {
    return -1;	/* Error: divide by zero */
  }
  if (n == 1)
  { /* Use short division instead */
    r[0] = mpShortDiv(q, u, v[0], udigits);
    return 0;
  }
  if (m < 0)
  { /* v > u, so just set q = 0 and r = u */
    mpSetEqual(r, u, udigits);
    return 0;
  }
  if (m == 0)
  { /* u and v are the same length */
    cmp = mpCompare(u, v, (unsigned int)n);
    if (cmp < 0)
    { /* v > u, as above */
      mpSetEqual(r, u, udigits);
      return 0;
    }
    else if (cmp == 0)
    { /* v == u, so set q = 1 and r = 0 */
      mpSetDigit(q, 1, udigits);
      return 0;
    }
  }
  /* In Knuth notation, we have:
     Given
     u = (Um+n-1 ... U1U0)
     v = (Vn-1 ... V1V0)
     Compute
     q = u/v = (QmQm-1 ... Q0)
     r = u mod v = (Rn-1 ... R1R0) */
  /* Step D1. Normalise */
  /* Requires high bit of Vn-1
     to be set, so find most signif. bit then shift left,
     i.e. d = 2^shift, u' = u * d, v' = v * d. */
  bitmask = HIBITMASK;
  for (shift = 0; shift < BITS_PER_DIGIT; shift++)
  {
    if (v[n - 1] & bitmask)
    {
      break;
    }
    bitmask >>= 1;
  }
  /* Normalise v in situ - NB only shift non-zero digits */
  overflow = mpShiftLeft(v, v, shift, n);
  /* Copy normalised dividend u*d into r */
  overflow = mpShiftLeft(r, u, shift, n + m);
  uu = r; /* Use ptr to keep notation constant */
  t[0] = overflow; /* New digit Um+n */
  /* Step D2. Initialise j. Set j = m */
  for (j = m; j >= 0; j--)
  {
    /* Step D3. Calculate Qhat = (b.Uj+n + Uj+n-1)/Vn-1 */
    qhatOK = 0;
    t[1] = t[0]; /* This is Uj+n */
    t[0] = uu[j+n-1];
    overflow = spDivide(&qhat, &rhat, t, v[n - 1]);
    /* Test Qhat */
    if (overflow)
    { /* Qhat = b */
      qhat = MAX_DIGIT;
      rhat = uu[j + n - 1];
      rhat += v[n - 1];
      if (rhat < v[n - 1]) /* Overflow */
      {
        qhatOK = 1;
      }
    }
    if (!qhatOK && QhatTooBig(qhat, rhat, v[n - 2], uu[j + n - 2]))
    { /* Qhat.Vn-2 > b.Rhat + Uj+n-2 */
      qhat--;
      rhat += v[n - 1];
      if (!(rhat < v[n - 1]))
      {
        if (QhatTooBig(qhat, rhat, v[n - 2], uu[j + n - 2]))
        {
          qhat--;
        }
      }
    }
    /* Step D4. Multiply and subtract */
    ww = &uu[j];
    overflow = mpMultSub(t[1], ww, v, qhat, (unsigned int)n);
    /* Step D5. Test remainder. Set Qj = Qhat */
    q[j] = qhat;
    if (overflow)
    { /* Step D6. Add back if D4 was negative */
      q[j]--;
      overflow = mpAdd(ww, ww, v, (unsigned int)n);
    }
    t[0] = uu[j + n - 1]; /* Uj+n on next round */
  } /* Step D7. Loop on j */
  /* Clear high digits in uu */
  for (j = n; j < m+n; j++)
  {
    uu[j] = 0;
  }
  /* Step D8. Unnormalise. */
  mpShiftRight(r, r, shift, n);
  mpShiftRight(v, v, shift, n);
  return 0;
}

/*****************************************************************************/
static int APP_CC
mpModulo(DIGIT_T* r, DIGIT_T* u, unsigned int udigits,
         DIGIT_T* v, unsigned int vdigits)
{
  /* Calculates r = u mod v
     where r, v are multiprecision integers of length vdigits
     and u is a multiprecision integer of length udigits.
     r may overlap v.

     Note that r here is only vdigits long,
     whereas in mpDivide it is udigits long.

     Use remainder from mpDivide function. */
  /* Double-length temp variable for divide fn */
  DIGIT_T qq[MAX_DIG_LEN * 2];
  /* Use a double-length temp for r to allow overlap of r and v */
  DIGIT_T rr[MAX_DIG_LEN * 2];

  /* rr[2n] = u[2n] mod v[n] */
  mpDivide(qq, rr, u, udigits, v, vdigits);
  mpSetEqual(r, rr, vdigits);
  mpSetZero(rr, udigits);
  mpSetZero(qq, udigits);
  return 0;
}

/*****************************************************************************/
static int APP_CC
mpMultiply(DIGIT_T* w, DIGIT_T* u, DIGIT_T* v, unsigned int ndigits)
{
  /* Computes product w = u * v
     where u, v are multiprecision integers of ndigits each
     and w is a multiprecision integer of 2*ndigits
     Ref: Knuth Vol 2 Ch 4.3.1 p 268 Algorithm M. */
  DIGIT_T k;
  DIGIT_T t[2];
  unsigned int i;
  unsigned int j;
  unsigned int m;
  unsigned int n;

  n = ndigits;
  m = n;
  /* Step M1. Initialise */
  for (i = 0; i < 2 * m; i++)
  {
    w[i] = 0;
  }
  for (j = 0; j < n; j++)
  {
    /* Step M2. Zero multiplier? */
    if (v[j] == 0)
    {
      w[j + m] = 0;
    }
    else
    {
      /* Step M3. Initialise i */
      k = 0;
      for (i = 0; i < m; i++)
      {
        /* Step M4. Multiply and add */
        /* t = u_i * v_j + w_(i+j) + k */
        spMultiply(t, u[i], v[j]);
        t[0] += k;
        if (t[0] < k)
        {
          t[1]++;
        }
        t[0] += w[i + j];
        if (t[0] < w[i+j])
        {
          t[1]++;
        }
        w[i + j] = t[0];
        k = t[1];
      }
      /* Step M5. Loop on i, set w_(j+m) = k */
      w[j + m] = k;
    }
  } /* Step M6. Loop on j */
  return 0;
}

/*****************************************************************************/
static int APP_CC
mpModMult(DIGIT_T* a, DIGIT_T* x, DIGIT_T* y,
          DIGIT_T* m, unsigned int ndigits)
{ /* Computes a = (x * y) mod m */
  /* Double-length temp variable */
  DIGIT_T p[MAX_DIG_LEN * 2];

  /* Calc p[2n] = x * y */
  mpMultiply(p, x, y, ndigits);
  /* Then modulo */
  mpModulo(a, p, ndigits * 2, m, ndigits);
  mpSetZero(p, ndigits * 2);
  return 0;
}

/*****************************************************************************/
int APP_CC
ssl_mod_exp(char* out, int out_len, char* in, int in_len,
            char* mod, int mod_len, char* exp, int exp_len)
{
  /* Computes y = x ^ e mod m */
  /* Binary left-to-right method */
  DIGIT_T mask;
  DIGIT_T* e;
  DIGIT_T* x;
  DIGIT_T* y;
  DIGIT_T* m;
  unsigned int n;
  int max_size;
  char* l_out;
  char* l_in;
  char* l_mod;
  char* l_exp;

  if (in_len > out_len || in_len == 0 ||
      out_len == 0 || mod_len == 0 || exp_len == 0)
  {
    return 0;
  }
  max_size = out_len;
  if (in_len > max_size)
  {
    max_size = in_len;
  }
  if (mod_len > max_size)
  {
    max_size = mod_len;
  }
  if (exp_len > max_size)
  {
    max_size = exp_len;
  }
  l_out = (char*)g_malloc(max_size, 1);
  l_in = (char*)g_malloc(max_size, 1);
  l_mod = (char*)g_malloc(max_size, 1);
  l_exp = (char*)g_malloc(max_size, 1);
  memcpy(l_in, in, in_len);
  memcpy(l_mod, mod, mod_len);
  memcpy(l_exp, exp, exp_len);
  e = (DIGIT_T*)l_exp;
  x = (DIGIT_T*)l_in;
  y = (DIGIT_T*)l_out;
  m = (DIGIT_T*)l_mod;
  /* Find second-most significant bit in e */
  n = mpSizeof(e, max_size / 4);
  for (mask = HIBITMASK; mask > 0; mask >>= 1)
  {
    if (e[n - 1] & mask)
    {
      break;
    }
  }
  mpNEXTBITMASK(mask, n);
  /* Set y = x */
  mpSetEqual(y, x, max_size / 4);
  /* For bit j = k - 2 downto 0 step -1 */
  while (n)
  {
    mpModMult(y, y, y, m, max_size / 4); /* Square */
    if (e[n - 1] & mask)
    {
      mpModMult(y, y, x, m, max_size / 4); /* Multiply */
    }
    /* Move to next bit */
    mpNEXTBITMASK(mask, n);
  }
  memcpy(out, l_out, out_len);
  g_free(l_out);
  g_free(l_in);
  g_free(l_mod);
  g_free(l_exp);
  return out_len;
}

