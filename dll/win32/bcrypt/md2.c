/*
 * MD2 (RFC 1319) hash function implementation by Tom St Denis
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

#include <assert.h>
#include "bcrypt_internal.h"

static const unsigned char PI_SUBST[256] = {
  41, 46, 67, 201, 162, 216, 124, 1, 61, 54, 84, 161, 236, 240, 6,
  19, 98, 167, 5, 243, 192, 199, 115, 140, 152, 147, 43, 217, 188,
  76, 130, 202, 30, 155, 87, 60, 253, 212, 224, 22, 103, 66, 111, 24,
  138, 23, 229, 18, 190, 78, 196, 214, 218, 158, 222, 73, 160, 251,
  245, 142, 187, 47, 238, 122, 169, 104, 121, 145, 21, 178, 7, 63,
  148, 194, 16, 137, 11, 34, 95, 33, 128, 127, 93, 154, 90, 144, 50,
  39, 53, 62, 204, 231, 191, 247, 151, 3, 255, 25, 48, 179, 72, 165,
  181, 209, 215, 94, 146, 42, 172, 86, 170, 198, 79, 184, 56, 210,
  150, 164, 125, 182, 118, 252, 107, 226, 156, 116, 4, 241, 69, 157,
  112, 89, 100, 113, 135, 32, 134, 91, 207, 101, 230, 45, 168, 2, 27,
  96, 37, 173, 174, 176, 185, 246, 28, 70, 97, 105, 52, 64, 126, 15,
  85, 71, 163, 35, 221, 81, 175, 58, 195, 92, 249, 206, 186, 197,
  234, 38, 44, 83, 13, 110, 133, 40, 132, 9, 211, 223, 205, 244, 65,
  129, 77, 82, 106, 220, 55, 200, 108, 193, 171, 250, 36, 225, 123,
  8, 12, 189, 177, 74, 120, 136, 149, 139, 227, 99, 232, 109, 233,
  203, 213, 254, 59, 0, 29, 57, 242, 239, 183, 14, 102, 88, 208, 228,
  166, 119, 114, 248, 235, 117, 75, 10, 49, 68, 80, 180, 143, 237,
  31, 26, 219, 153, 141, 51, 159, 17, 131, 20
};

/* adds 16 bytes to the checksum */
static void md2_update_chksum(MD2_CTX *md2)
{
    int j;
    unsigned char L;
    L = md2->chksum[15];
    for (j = 0; j < 16; j++) {

/* caution, the RFC says its "C[j] = S[M[i*16+j] xor L]" but the reference source code [and test vectors] say
   otherwise.
*/
        L = (md2->chksum[j] ^= PI_SUBST[(int)(md2->buf[j] ^ L)] & 255);
    }
}

static void md2_compress(MD2_CTX *md2)
{
    int j, k;
    unsigned char t;

    /* copy block */
    for (j = 0; j < 16; j++) {
        md2->X[16+j] = md2->buf[j];
        md2->X[32+j] = md2->X[j] ^ md2->X[16+j];
    }

    t = 0;

    /* do 18 rounds */
    for (j = 0; j < 18; j++) {
        for (k = 0; k < 48; k++) {
            t = (md2->X[k] ^= PI_SUBST[(int)(t & 255)]);
        }
        t = (t + (unsigned char)j) & 255;
    }
}

void md2_init(MD2_CTX *md2)
{
    /* MD2 uses a zero'ed state... */
    memset(md2->X, 0, sizeof(md2->X));
    memset(md2->chksum, 0, sizeof(md2->chksum));
    memset(md2->buf, 0, sizeof(md2->buf));
    md2->curlen = 0;
}

void md2_update(MD2_CTX *md2, const unsigned char *buf, ULONG len)
{
    unsigned long n;

    assert(md2->curlen <= sizeof(md2->buf));

    while (len > 0) {
        n = min(len, (16 - md2->curlen));
        memcpy(md2->buf + md2->curlen, buf, (size_t)n);
        md2->curlen += n;
        buf         += n;
        len         -= n;

        /* is 16 bytes full? */
        if (md2->curlen == 16) {
            md2_compress(md2);
            md2_update_chksum(md2);
            md2->curlen = 0;
        }
    }
}

void md2_finalize(MD2_CTX * md2, unsigned char *hash)
{
    unsigned long i, k;

    assert(md2->curlen <= sizeof(md2->buf));

    /* pad the message */
    k = 16 - md2->curlen;
    for (i = md2->curlen; i < 16; i++) {
        md2->buf[i] = (unsigned char)k;
    }

    /* hash and update */
    md2_compress(md2);
    md2_update_chksum(md2);

    /* hash checksum */
    memcpy(md2->buf, md2->chksum, 16);
    md2_compress(md2);

    /* output is lower 16 bytes of X */
    memcpy(hash, md2->X, 16);
}
