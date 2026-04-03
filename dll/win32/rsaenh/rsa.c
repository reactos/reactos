/*
 * dlls/rsaenh/rsa.c
 * RSA public key cryptographic functions
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
#include "windef.h"

static const struct {
    int mpi_code, ltc_code;
} mpi_to_ltc_codes[] = {
   { MP_OKAY ,  CRYPT_OK},
   { MP_MEM  ,  CRYPT_MEM},
   { MP_VAL  ,  CRYPT_INVALID_ARG},
};

/* convert a MPI error to a LTC error (Possibly the most powerful function ever!  Oh wait... no) */
static int mpi_to_ltc_error(int err)
{
   unsigned int x;

   for (x = 0; x < ARRAY_SIZE(mpi_to_ltc_codes); x++) {
       if (err == mpi_to_ltc_codes[x].mpi_code) { 
          return mpi_to_ltc_codes[x].ltc_code;
       }
   }
   return CRYPT_ERROR;
}

extern int gen_rand_impl(unsigned char *dst, unsigned int len);

static int rand_prime_helper(unsigned char *dst, int len, void *dat)
{
    return gen_rand_impl(dst, len) ? len : 0;
}

static int rand_prime(mp_int *N, long len)
{
   int type;

   /* get type */
   if (len < 0) {
      type = LTM_PRIME_BBS;
      len = -len;
   } else {
      /* This seems to be what MS CSP's do: */
      type = LTM_PRIME_2MSB_ON;
      /* Original LibTomCrypt: type = 0; */
   }

   /* allow sizes between 2 and 256 bytes for a prime size */
   if (len < 16 || len > 8192) {
      printf("Invalid prime size!\n");
      return CRYPT_INVALID_PRIME_SIZE;
   }
   
   /* New prime generation makes the code even more cryptoish-insane.  Do you know what this means!!!
      -- Gir:  Yeah, oh wait, er, no.
    */
   return mpi_to_ltc_error(mp_prime_random_ex(N, mp_prime_rabin_miller_trials(len), len, type, rand_prime_helper, NULL));
}
      
int rsa_make_key(int size, long e, rsa_key *key)
{
   mp_int p, q, tmp1, tmp2, tmp3;
   int    err;

   if ((size < (MIN_RSA_SIZE/8)) || (size > (MAX_RSA_SIZE/8))) {
      return CRYPT_INVALID_KEYSIZE;
   }

   if ((e < 3) || ((e & 1) == 0)) {
      return CRYPT_INVALID_ARG;
   }

   if ((err = mp_init_multi(&p, &q, &tmp1, &tmp2, &tmp3, NULL)) != MP_OKAY) {
      return mpi_to_ltc_error(err);
   }

    /* make primes p and q (optimization provided by Wayne Scott) */
   if ((err = mp_set_int(&tmp3, e)) != MP_OKAY) { goto error; }            /* tmp3 = e */

    /* make prime "p" */
   do {
       if ((err = rand_prime(&p, size*4)) != CRYPT_OK) { goto done; }
       if ((err = mp_sub_d(&p, 1, &tmp1)) != MP_OKAY)               { goto error; }  /* tmp1 = p-1 */
       if ((err = mp_gcd(&tmp1, &tmp3, &tmp2)) != MP_OKAY)          { goto error; }  /* tmp2 = gcd(p-1, e) */
   } while (mp_cmp_d(&tmp2, 1) != 0);                                                /* while e divides p-1 */

   /* make prime "q" */
   do {
       if ((err = rand_prime(&q, size*4)) != CRYPT_OK) { goto done; }
       if ((err = mp_sub_d(&q, 1, &tmp1)) != MP_OKAY)               { goto error; } /* tmp1 = q-1 */
       if ((err = mp_gcd(&tmp1, &tmp3, &tmp2)) != MP_OKAY)          { goto error; } /* tmp2 = gcd(q-1, e) */
   } while (mp_cmp_d(&tmp2, 1) != 0);                                               /* while e divides q-1 */

   /* tmp1 = lcm(p-1, q-1) */
   if ((err = mp_sub_d(&p, 1, &tmp2)) != MP_OKAY)                  { goto error; } /* tmp2 = p-1 */
                                                                   /* tmp1 = q-1 (previous do/while loop) */
   if ((err = mp_lcm(&tmp1, &tmp2, &tmp1)) != MP_OKAY)             { goto error; } /* tmp1 = lcm(p-1, q-1) */

   /* make key */
   if ((err = mp_init_multi(&key->e, &key->d, &key->N, &key->dQ, &key->dP,
                     &key->qP, &key->p, &key->q, NULL)) != MP_OKAY) {
      goto error;
   }

   if ((err = mp_set_int(&key->e, e)) != MP_OKAY)                     { goto error2; } /* key->e =  e */
   if ((err = mp_invmod(&key->e, &tmp1, &key->d)) != MP_OKAY)         { goto error2; } /* key->d = 1/e mod lcm(p-1,q-1) */
   if ((err = mp_mul(&p, &q, &key->N)) != MP_OKAY)                    { goto error2; } /* key->N = pq */

   /* optimize for CRT now */
   /* find d mod q-1 and d mod p-1 */
   if ((err = mp_sub_d(&p, 1, &tmp1)) != MP_OKAY)                     { goto error2; } /* tmp1 = q-1 */
   if ((err = mp_sub_d(&q, 1, &tmp2)) != MP_OKAY)                     { goto error2; } /* tmp2 = p-1 */
   if ((err = mp_mod(&key->d, &tmp1, &key->dP)) != MP_OKAY)           { goto error2; } /* dP = d mod p-1 */
   if ((err = mp_mod(&key->d, &tmp2, &key->dQ)) != MP_OKAY)           { goto error2; } /* dQ = d mod q-1 */
   if ((err = mp_invmod(&q, &p, &key->qP)) != MP_OKAY)                { goto error2; } /* qP = 1/q mod p */

   if ((err = mp_copy(&p, &key->p)) != MP_OKAY)                       { goto error2; }
   if ((err = mp_copy(&q, &key->q)) != MP_OKAY)                       { goto error2; }

   /* shrink ram required  */
   if ((err = mp_shrink(&key->e)) != MP_OKAY)                         { goto error2; }
   if ((err = mp_shrink(&key->d)) != MP_OKAY)                         { goto error2; }
   if ((err = mp_shrink(&key->N)) != MP_OKAY)                         { goto error2; }
   if ((err = mp_shrink(&key->dQ)) != MP_OKAY)                        { goto error2; }
   if ((err = mp_shrink(&key->dP)) != MP_OKAY)                        { goto error2; }
   if ((err = mp_shrink(&key->qP)) != MP_OKAY)                        { goto error2; }
   if ((err = mp_shrink(&key->p)) != MP_OKAY)                         { goto error2; }
   if ((err = mp_shrink(&key->q)) != MP_OKAY)                         { goto error2; }

   /* set key type (in this case it's CRT optimized) */
   key->type = PK_PRIVATE;

   /* return ok and free temps */
   err       = CRYPT_OK;
   goto done;
error2:
   mp_clear_multi(&key->d, &key->e, &key->N, &key->dQ, &key->dP,
                  &key->qP, &key->p, &key->q, NULL);
error:
   err = mpi_to_ltc_error(err);
done:
   mp_clear_multi(&tmp3, &tmp2, &tmp1, &p, &q, NULL);
   return err;
}

void rsa_free(rsa_key *key)
{
   mp_clear_multi(&key->e, &key->d, &key->N, &key->dQ, &key->dP,
                  &key->qP, &key->p, &key->q, NULL);
}

/* compute an RSA modular exponentiation */
int rsa_exptmod(const unsigned char *in,   unsigned long inlen,
                      unsigned char *out,  unsigned long *outlen, int which,
                      rsa_key *key)
{
   mp_int        tmp, tmpa, tmpb;
   unsigned long x;
   int           err;

   /* is the key of the right type for the operation? */
   if (which == PK_PRIVATE && (key->type != PK_PRIVATE)) {
      return CRYPT_PK_NOT_PRIVATE;
   }

   /* must be a private or public operation */
   if (which != PK_PRIVATE && which != PK_PUBLIC) {
      return CRYPT_PK_INVALID_TYPE;
   }

   /* init and copy into tmp */
   if ((err = mp_init_multi(&tmp, &tmpa, &tmpb, NULL)) != MP_OKAY)    { return mpi_to_ltc_error(err); }
   if ((err = mp_read_unsigned_bin(&tmp, in, (int)inlen)) != MP_OKAY) { goto error; }

   /* sanity check on the input */
   if (mp_cmp(&key->N, &tmp) == MP_LT) {
      err = CRYPT_PK_INVALID_SIZE;
      goto done;
   }

   /* are we using the private exponent and is the key optimized? */
   if (which == PK_PRIVATE) {
      /* tmpa = tmp^dP mod p */
      if ((err = mpi_to_ltc_error(mp_exptmod(&tmp, &key->dP, &key->p, &tmpa))) != MP_OKAY)    { goto error; }
      
      /* tmpb = tmp^dQ mod q */
      if ((err = mpi_to_ltc_error(mp_exptmod(&tmp, &key->dQ, &key->q, &tmpb))) != MP_OKAY)    { goto error; }

      /* tmp = (tmpa - tmpb) * qInv (mod p) */
      if ((err = mp_sub(&tmpa, &tmpb, &tmp)) != MP_OKAY)                    { goto error; }
      if ((err = mp_mulmod(&tmp, &key->qP, &key->p, &tmp)) != MP_OKAY)      { goto error; }

      /* tmp = tmpb + q * tmp */
      if ((err = mp_mul(&tmp, &key->q, &tmp)) != MP_OKAY)                   { goto error; }
      if ((err = mp_add(&tmp, &tmpb, &tmp)) != MP_OKAY)                     { goto error; }
   } else {
      /* exptmod it */
      if ((err = mp_exptmod(&tmp, &key->e, &key->N, &tmp)) != MP_OKAY) { goto error; }
   }

   /* read it back */
   x = (unsigned long)mp_unsigned_bin_size(&key->N);
   if (x > *outlen) {
      err = CRYPT_BUFFER_OVERFLOW;
      goto done;
   }
   *outlen = x;

   /* convert it */
   memset(out, 0, x);
   if ((err = mp_to_unsigned_bin(&tmp, out+(x-mp_unsigned_bin_size(&tmp)))) != MP_OKAY) { goto error; }

   /* clean up and return */
   err = CRYPT_OK;
   goto done;
error:
   err = mpi_to_ltc_error(err);
done:
   mp_clear_multi(&tmp, &tmpa, &tmpb, NULL);
   return err;
}
