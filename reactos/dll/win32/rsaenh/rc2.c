/*
 * dlls/rsaen/rc2.c
 * RC2 functions  
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

/* 256-entry permutation table, probably derived somehow from pi */
static const unsigned char permute[256] = {
        217,120,249,196, 25,221,181,237, 40,233,253,121, 74,160,216,157,
        198,126, 55,131, 43,118, 83,142, 98, 76,100,136, 68,139,251,162,
         23,154, 89,245,135,179, 79, 19, 97, 69,109,141,  9,129,125, 50,
        189,143, 64,235,134,183,123, 11,240,149, 33, 34, 92,107, 78,130,
         84,214,101,147,206, 96,178, 28,115, 86,192, 20,167,140,241,220,
         18,117,202, 31, 59,190,228,209, 66, 61,212, 48,163, 60,182, 38,
        111,191, 14,218, 70,105,  7, 87, 39,242, 29,155,188,148, 67,  3,
        248, 17,199,246,144,239, 62,231,  6,195,213, 47,200,102, 30,215,
          8,232,234,222,128, 82,238,247,132,170,114,172, 53, 77,106, 42,
        150, 26,210,113, 90, 21, 73,116, 75,159,208, 94,  4, 24,164,236,
        194,224, 65,110, 15, 81,203,204, 36,145,175, 80,161,244,112, 57,
        153,124, 58,133, 35,184,180,122,252,  2, 54, 91, 37, 85,151, 49,
         45, 93,250,152,227,138,146,174,  5,223, 41, 16,103,108,186,201,
        211,  0,230,207,225,158,168, 44, 99, 22,  1, 63, 88,226,137,169,
         13, 56, 52, 27,171, 51,255,176,187, 72, 12, 95,185,177,205, 46,
        197,243,219, 71,229,165,156,119, 10,166, 32,104,254,127,193,173
};

int rc2_setup(const unsigned char *key, int keylen, int bits, int rounds, rc2_key *rc2)
{
   unsigned *xkey = rc2->xkey;
   unsigned char tmp[128];
   unsigned T8, TM;
   int i;

   if (keylen < 5 || keylen > 128) {
      return CRYPT_INVALID_KEYSIZE;
   }

   if (rounds != 0 && rounds != 16) {
      return CRYPT_INVALID_ROUNDS;
   }

    /* Following comment is from Eric Young's rc2 code: */
    /* It has come to my attention that there are 2 versions of the RC2
     * key schedule.  One which is normal, and anther which has a hook to
     * use a reduced key length.
     * BSAFE uses the 'retarded' version.  What I previously shipped is
     * the same as specifying 1024 for the 'bits' parameter.  BSAFE uses
     * a version where the bits parameter is the same as len*8 */
    /* Seems like MS uses the 'retarded' version, too.
     * Adjust effective keylen bits */
   if (bits <= 0) bits = keylen << 3;
   if (bits > 1024) bits = 1024;
   
   for (i = 0; i < keylen; i++) {
       tmp[i] = key[i] & 255;
   }

    /* Phase 1: Expand input key to 128 bytes */
    if (keylen < 128) {
        for (i = keylen; i < 128; i++) {
            tmp[i] = permute[(tmp[i - 1] + tmp[i - keylen]) & 255];
        }
    }
    
    /* Phase 2 - reduce effective key size to "bits" */
    /*bits = keylen<<3; */
    T8   = (unsigned)(bits+7)>>3;
    TM   = (255 >> (unsigned)(7 & -bits));
    tmp[128 - T8] = permute[tmp[128 - T8] & TM];
    for (i = 127 - T8; i >= 0; i--) {
        tmp[i] = permute[tmp[i + 1] ^ tmp[i + T8]];
    }

    /* Phase 3 - copy to xkey in little-endian order */
    for (i = 0; i < 64; i++) {
        xkey[i] =  (unsigned)tmp[2*i] + ((unsigned)tmp[2*i+1] << 8);
    }        

    return CRYPT_OK;
}

/**********************************************************************\
* Encrypt an 8-byte block of plaintext using the given key.            *
\**********************************************************************/
void rc2_ecb_encrypt( const unsigned char *plain,
                            unsigned char *cipher,
                            rc2_key *rc2)
{
    unsigned *xkey;
    unsigned x76, x54, x32, x10, i;

    xkey = rc2->xkey;

    x76 = ((unsigned)plain[7] << 8) + (unsigned)plain[6];
    x54 = ((unsigned)plain[5] << 8) + (unsigned)plain[4];
    x32 = ((unsigned)plain[3] << 8) + (unsigned)plain[2];
    x10 = ((unsigned)plain[1] << 8) + (unsigned)plain[0];

    for (i = 0; i < 16; i++) {
        x10 = (x10 + (x32 & ~x76) + (x54 & x76) + xkey[4*i+0]) & 0xFFFF;
        x10 = ((x10 << 1) | (x10 >> 15));

        x32 = (x32 + (x54 & ~x10) + (x76 & x10) + xkey[4*i+1]) & 0xFFFF;
        x32 = ((x32 << 2) | (x32 >> 14));

        x54 = (x54 + (x76 & ~x32) + (x10 & x32) + xkey[4*i+2]) & 0xFFFF;
        x54 = ((x54 << 3) | (x54 >> 13));

        x76 = (x76 + (x10 & ~x54) + (x32 & x54) + xkey[4*i+3]) & 0xFFFF;
        x76 = ((x76 << 5) | (x76 >> 11));

        if (i == 4 || i == 10) {
            x10 = (x10 + xkey[x76 & 63]) & 0xFFFF;
            x32 = (x32 + xkey[x10 & 63]) & 0xFFFF;
            x54 = (x54 + xkey[x32 & 63]) & 0xFFFF;
            x76 = (x76 + xkey[x54 & 63]) & 0xFFFF;
        }
    }

    cipher[0] = (unsigned char)x10;
    cipher[1] = (unsigned char)(x10 >> 8);
    cipher[2] = (unsigned char)x32;
    cipher[3] = (unsigned char)(x32 >> 8);
    cipher[4] = (unsigned char)x54;
    cipher[5] = (unsigned char)(x54 >> 8);
    cipher[6] = (unsigned char)x76;
    cipher[7] = (unsigned char)(x76 >> 8);
}

/**********************************************************************\
* Decrypt an 8-byte block of ciphertext using the given key.           *
\**********************************************************************/
void rc2_ecb_decrypt( const unsigned char *cipher,
                            unsigned char *plain,
                            rc2_key *rc2)
{
    unsigned x76, x54, x32, x10;
    unsigned *xkey;
    int i;

    xkey = rc2->xkey;

    x76 = ((unsigned)cipher[7] << 8) + (unsigned)cipher[6];
    x54 = ((unsigned)cipher[5] << 8) + (unsigned)cipher[4];
    x32 = ((unsigned)cipher[3] << 8) + (unsigned)cipher[2];
    x10 = ((unsigned)cipher[1] << 8) + (unsigned)cipher[0];

    for (i = 15; i >= 0; i--) {
        if (i == 4 || i == 10) {
            x76 = (x76 - xkey[x54 & 63]) & 0xFFFF;
            x54 = (x54 - xkey[x32 & 63]) & 0xFFFF;
            x32 = (x32 - xkey[x10 & 63]) & 0xFFFF;
            x10 = (x10 - xkey[x76 & 63]) & 0xFFFF;
        }

        x76 = ((x76 << 11) | (x76 >> 5));
        x76 = (x76 - ((x10 & ~x54) + (x32 & x54) + xkey[4*i+3])) & 0xFFFF;

        x54 = ((x54 << 13) | (x54 >> 3));
        x54 = (x54 - ((x76 & ~x32) + (x10 & x32) + xkey[4*i+2])) & 0xFFFF;

        x32 = ((x32 << 14) | (x32 >> 2));
        x32 = (x32 - ((x54 & ~x10) + (x76 & x10) + xkey[4*i+1])) & 0xFFFF;

        x10 = ((x10 << 15) | (x10 >> 1));
        x10 = (x10 - ((x32 & ~x76) + (x54 & x76) + xkey[4*i+0])) & 0xFFFF;
    }

    plain[0] = (unsigned char)x10;
    plain[1] = (unsigned char)(x10 >> 8);
    plain[2] = (unsigned char)x32;
    plain[3] = (unsigned char)(x32 >> 8);
    plain[4] = (unsigned char)x54;
    plain[5] = (unsigned char)(x54 >> 8);
    plain[6] = (unsigned char)x76;
    plain[7] = (unsigned char)(x76 >> 8);
}
