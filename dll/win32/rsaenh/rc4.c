/*
 * dlls/rsaenh/rc4.c
 * RC4 functions
 *
 * Copyright 2004 Michael Jung
 * Based on public domain code by Tom St Denis (tomstdenis@iahu.ca)
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

/*
 * This file contains code from the LibTomCrypt cryptographic 
 * library written by Tom St Denis (tomstdenis@iahu.ca). LibTomCrypt
 * is in the public domain. The code in this file is tailored to
 * special requirements. Take a look at http://libtomcrypt.org for the
 * original version. 
 */

#include "tomcrypt.h"

int rc4_start(prng_state *prng)
{
    /* set keysize to zero */
    prng->rc4.x = 0;
    
    return CRYPT_OK;
}

int rc4_add_entropy(const unsigned char *buf, unsigned long len, prng_state *prng)
{
    /* trim as required */
    if (prng->rc4.x + len > 256) {
       if (prng->rc4.x == 256) {
          /* I can't possibly accept another byte, ok maybe a mint wafer... */
          return CRYPT_OK;
       } else {
          /* only accept part of it */
          len = 256 - prng->rc4.x;
       }       
    }

    while (len--) {
       prng->rc4.buf[prng->rc4.x++] = *buf++;
    }

    return CRYPT_OK;
}

int rc4_ready(prng_state *prng)
{
    unsigned char key[256], tmp, *s;
    int keylen, x, y, j;

    /* extract the key */
    s = prng->rc4.buf;
    memcpy(key, s, 256);
    keylen = prng->rc4.x;

    /* make RC4 perm and shuffle */
    for (x = 0; x < 256; x++) {
        s[x] = x;
    }

    for (j = x = y = 0; x < 256; x++) {
        y = (y + prng->rc4.buf[x] + key[j++]) & 255;
        if (j == keylen) {
           j = 0; 
        }
        tmp = s[x]; s[x] = s[y]; s[y] = tmp;
    }
    prng->rc4.x = 0;
    prng->rc4.y = 0;

    return CRYPT_OK;
}

unsigned long rc4_read(unsigned char *buf, unsigned long len, prng_state *prng)
{
   unsigned char x, y, *s, tmp;
   unsigned long n;

   n = len;
   x = prng->rc4.x;
   y = prng->rc4.y;
   s = prng->rc4.buf;
   while (len--) {
      x = (x + 1) & 255;
      y = (y + s[x]) & 255;
      tmp = s[x]; s[x] = s[y]; s[y] = tmp;
      tmp = (s[x] + s[y]) & 255;
      *buf++ ^= s[tmp];
   }
   prng->rc4.x = x;
   prng->rc4.y = y;
   return n;
}
