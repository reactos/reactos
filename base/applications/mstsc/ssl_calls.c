/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   xrdp: A Remote Desktop Protocol server.
   Copyright (C) Jay Sorg 2004-2005

   ssl calls

*/

#include "precomp.h"

/*****************************************************************************/
static void * g_malloc(int size, int zero)
{
    void * p;

    p = CryptMemAlloc(size);
    if (zero)
    {
        memset(p, 0, size);
    }
    return p;
}

/*****************************************************************************/
static void g_free(void * in)
{
    CryptMemFree(in);
}

struct rc4_state
{
    HCRYPTPROV hCryptProv;
    HCRYPTKEY hKey;
};
/*****************************************************************************/
void*
rdssl_rc4_info_create(void)
{
    struct rc4_state *info = g_malloc(sizeof(struct rc4_state), 1);
    BOOL ret;
    DWORD dwErr;
    if (!info)
    {
        error("rdssl_rc4_info_create no memory\n");
        return NULL;
    }
    ret = CryptAcquireContext(&info->hCryptProv,
                              L"MSTSC",
                              MS_ENHANCED_PROV,
                              PROV_RSA_FULL,
                              0);
    if (!ret)
    {
        dwErr = GetLastError();
        if (dwErr == NTE_BAD_KEYSET)
        {
            ret = CryptAcquireContext(&info->hCryptProv,
                                      L"MSTSC",
                                      MS_ENHANCED_PROV,
                                      PROV_RSA_FULL,
                                      CRYPT_NEWKEYSET);
        }
    }
    if (!ret)
    {
        dwErr = GetLastError();
        error("CryptAcquireContext failed with %lx\n", dwErr);
        g_free(info);
        return NULL;
    }
    return info;
}

/*****************************************************************************/
void
rdssl_rc4_info_delete(void* rc4_info)
{
    struct rc4_state *info = rc4_info;
    BOOL ret = TRUE;
    DWORD dwErr;
    if (!info)
    {
        //error("rdssl_rc4_info_delete rc4_info is null\n");
        return;
    }
    if (info->hKey)
    {
        ret = CryptDestroyKey(info->hKey);
        if (!ret)
        {
            dwErr = GetLastError();
            error("CryptDestroyKey failed with %lx\n", dwErr);
        }
    }
    if (info->hCryptProv)
    {
        ret = CryptReleaseContext(info->hCryptProv, 0);
        if (!ret)
        {
            dwErr = GetLastError();
            error("CryptReleaseContext failed with %lx\n", dwErr);
        }
    }
    g_free(rc4_info);
}

/*****************************************************************************/
void
rdssl_rc4_set_key(void* rc4_info, char* key, int len)
{
    struct rc4_state *info = rc4_info;
    BOOL ret;
    DWORD dwErr;
    BYTE * blob;
    PUBLICKEYSTRUC *desc;
    DWORD * keySize;
    BYTE * keyBuf;
    if (!rc4_info || !key || !len || !info->hCryptProv)
    {
        error("rdssl_rc4_set_key %p %p %d\n", rc4_info, key, len);
        return;
    }
    blob = g_malloc(sizeof(PUBLICKEYSTRUC) + sizeof(DWORD) + len, 0);
    if (!blob)
    {
        error("rdssl_rc4_set_key no memory\n");
        return;
    }
    desc = (PUBLICKEYSTRUC *)blob;
    keySize = (DWORD *)(blob + sizeof(PUBLICKEYSTRUC));
    keyBuf = blob + sizeof(PUBLICKEYSTRUC) + sizeof(DWORD);
    desc->aiKeyAlg = CALG_RC4;
    desc->bType = PLAINTEXTKEYBLOB;
    desc->bVersion = CUR_BLOB_VERSION;
    desc->reserved = 0;
    *keySize = len;
    memcpy(keyBuf, key, len);
    if (info->hKey)
    {
        CryptDestroyKey(info->hKey);
        info->hKey = 0;
    }
    ret = CryptImportKey(info->hCryptProv,
                         blob,
                         sizeof(PUBLICKEYSTRUC) + sizeof(DWORD) + len,
                         0,
                         CRYPT_EXPORTABLE,
                         &info->hKey);
    g_free(blob);
    if (!ret)
    {
        dwErr = GetLastError();
        error("CryptImportKey failed with %lx\n", dwErr);
    }
}

/*****************************************************************************/
void
rdssl_rc4_crypt(void* rc4_info, char* in_data, char* out_data, int len)
{
    struct rc4_state *info = rc4_info;
    BOOL ret;
    DWORD dwErr;
    BYTE * intermediate_data;
    DWORD dwLen = len;
    if (!rc4_info || !in_data || !out_data || !len || !info->hKey)
    {
        error("rdssl_rc4_crypt %p %p %p %d\n", rc4_info, in_data, out_data, len);
        return;
    }
    intermediate_data = g_malloc(len, 0);
    if (!intermediate_data)
    {
        error("rdssl_rc4_set_key no memory\n");
        return;
    }
    memcpy(intermediate_data, in_data, len);
    ret = CryptEncrypt(info->hKey,
                       0,
                       FALSE,
                       0,
                       intermediate_data,
                       &dwLen,
                       dwLen);
    if (!ret)
    {
        dwErr = GetLastError();
        g_free(intermediate_data);
        error("CryptEncrypt failed with %lx\n", dwErr);
        return;
    }
    memcpy(out_data, intermediate_data, len);
    g_free(intermediate_data);
}

struct hash_context
{
    HCRYPTPROV hCryptProv;
    HCRYPTKEY hHash;
};

/*****************************************************************************/
void*
rdssl_hash_info_create(ALG_ID id)
{
    struct hash_context *info = g_malloc(sizeof(struct hash_context), 1);
    BOOL ret;
    DWORD dwErr;
    if (!info)
    {
        error("rdssl_hash_info_create %d no memory\n", id);
        return NULL;
    }
    ret = CryptAcquireContext(&info->hCryptProv,
                              L"MSTSC",
                              MS_ENHANCED_PROV,
                              PROV_RSA_FULL,
                              0);
    if (!ret)
    {
        dwErr = GetLastError();
        if (dwErr == NTE_BAD_KEYSET)
        {
            ret = CryptAcquireContext(&info->hCryptProv,
                                      L"MSTSC",
                                      MS_ENHANCED_PROV,
                                      PROV_RSA_FULL,
                                      CRYPT_NEWKEYSET);
        }
    }
    if (!ret)
    {
        dwErr = GetLastError();
        g_free(info);
        error("CryptAcquireContext failed with %lx\n", dwErr);
        return NULL;
    }
    ret = CryptCreateHash(info->hCryptProv,
                          id,
                          0,
                          0,
                          &info->hHash);
    if (!ret)
    {
        dwErr = GetLastError();
        CryptReleaseContext(info->hCryptProv, 0);
        g_free(info);
        error("CryptCreateHash failed with %lx\n", dwErr);
        return NULL;
    }
    return info;
}

/*****************************************************************************/
void
rdssl_hash_info_delete(void* hash_info)
{
    struct hash_context *info = hash_info;
    if (!info)
    {
        //error("ssl_hash_info_delete hash_info is null\n");
        return;
    }
    if (info->hHash)
    {
        CryptDestroyHash(info->hHash);
    }
    if (info->hCryptProv)
    {
        CryptReleaseContext(info->hCryptProv, 0);
    }
    g_free(hash_info);
}

/*****************************************************************************/
void
rdssl_hash_clear(void* hash_info, ALG_ID id)
{
    struct hash_context *info = hash_info;
    BOOL ret;
    DWORD dwErr;
    if (!info || !info->hHash || !info->hCryptProv)
    {
        error("rdssl_hash_clear %p\n", info);
        return;
    }
    ret = CryptDestroyHash(info->hHash);
    if (!ret)
    {
        dwErr = GetLastError();
        error("CryptDestroyHash failed with %lx\n", dwErr);
        return;
    }
    ret = CryptCreateHash(info->hCryptProv,
                          id,
                          0,
                          0,
                          &info->hHash);
    if (!ret)
    {
        dwErr = GetLastError();
        error("CryptCreateHash failed with %lx\n", dwErr);
    }
}

void
rdssl_hash_transform(void* hash_info, char* data, int len)
{
    struct hash_context *info = hash_info;
    BOOL ret;
    DWORD dwErr;
    if (!info || !info->hHash || !info->hCryptProv || !data || !len)
    {
        error("rdssl_hash_transform %p %p %d\n", hash_info, data, len);
        return;
    }
    ret = CryptHashData(info->hHash,
                        (BYTE *)data,
                        len,
                        0);
    if (!ret)
    {
        dwErr = GetLastError();
        error("CryptHashData failed with %lx\n", dwErr);
    }
}

/*****************************************************************************/
void
rdssl_hash_complete(void* hash_info, char* data)
{
    struct hash_context *info = hash_info;
    BOOL ret;
    DWORD dwErr, dwDataLen;
    if (!info || !info->hHash || !info->hCryptProv || !data)
    {
        error("rdssl_hash_complete %p %p\n", hash_info, data);
        return;
    }
    ret = CryptGetHashParam(info->hHash,
                            HP_HASHVAL,
                            NULL,
                            &dwDataLen,
                            0);
    if (!ret)
    {
        dwErr = GetLastError();
        error("CryptGetHashParam failed with %lx\n", dwErr);
        return;
    }
    ret = CryptGetHashParam(info->hHash,
                            HP_HASHVAL,
                            (BYTE *)data,
                            &dwDataLen,
                            0);
    if (!ret)
    {
        dwErr = GetLastError();
        error("CryptGetHashParam failed with %lx\n", dwErr);
    }
}

/*****************************************************************************/
void*
rdssl_sha1_info_create(void)
{
    return rdssl_hash_info_create(CALG_SHA1);
}

/*****************************************************************************/
void
rdssl_sha1_info_delete(void* sha1_info)
{
    rdssl_hash_info_delete(sha1_info);
}

/*****************************************************************************/
void
rdssl_sha1_clear(void* sha1_info)
{
    rdssl_hash_clear(sha1_info, CALG_SHA1);
}

/*****************************************************************************/
void
rdssl_sha1_transform(void* sha1_info, char* data, int len)
{
    rdssl_hash_transform(sha1_info, data, len);
}

/*****************************************************************************/
void
rdssl_sha1_complete(void* sha1_info, char* data)
{
    rdssl_hash_complete(sha1_info, data);
}

/*****************************************************************************/
void*
rdssl_md5_info_create(void)
{
    return rdssl_hash_info_create(CALG_MD5);
}

/*****************************************************************************/
void
rdssl_md5_info_delete(void* md5_info)
{
    rdssl_hash_info_delete(md5_info);
}

/*****************************************************************************/
void
rdssl_md5_clear(void* md5_info)
{
    rdssl_hash_clear(md5_info, CALG_MD5);
}

/*****************************************************************************/
void
rdssl_md5_transform(void* md5_info, char* data, int len)
{
    rdssl_hash_transform(md5_info, data, len);
}

/*****************************************************************************/
void
rdssl_md5_complete(void* md5_info, char* data)
{
    rdssl_hash_complete(md5_info, data);
}

/*****************************************************************************/
void
rdssl_hmac_md5(char* key, int keylen, char* data, int len, char* output)
{
    HCRYPTPROV hCryptProv;
    HCRYPTKEY hKey;
    HCRYPTKEY hHash;
    BOOL ret;
    DWORD dwErr, dwDataLen;
    HMAC_INFO info;
    BYTE * blob;
    PUBLICKEYSTRUC *desc;
    DWORD * keySize;
    BYTE * keyBuf;
    BYTE sum[16];

    if (!key || !keylen || !data || !len ||!output)
    {
        error("rdssl_hmac_md5 %p %d %p %d %p\n", key, keylen, data, len, output);
        return;
    }
    blob = g_malloc(sizeof(PUBLICKEYSTRUC) + sizeof(DWORD) + keylen, 0);
    desc = (PUBLICKEYSTRUC *)blob;
    keySize = (DWORD *)(blob + sizeof(PUBLICKEYSTRUC));
    keyBuf = blob + sizeof(PUBLICKEYSTRUC) + sizeof(DWORD);
    if (!blob)
    {
        error("rdssl_hmac_md5 %d no memory\n");
        return;
    }
    ret = CryptAcquireContext(&hCryptProv,
                              L"MSTSC",
                              MS_ENHANCED_PROV,
                              PROV_RSA_FULL,
                              0);
    if (!ret)
    {
        dwErr = GetLastError();
        if (dwErr == NTE_BAD_KEYSET)
        {
            ret = CryptAcquireContext(&hCryptProv,
                                      L"MSTSC",
                                      MS_ENHANCED_PROV,
                                      PROV_RSA_FULL,
                                      CRYPT_NEWKEYSET);
        }
    }
    if (!ret)
    {
        dwErr = GetLastError();
        g_free(blob);
        error("CryptAcquireContext failed with %lx\n", dwErr);
        return;
    }
    desc->aiKeyAlg = CALG_RC4;
    desc->bType = PLAINTEXTKEYBLOB;
    desc->bVersion = CUR_BLOB_VERSION;
    desc->reserved = 0;
    if (keylen > 64)
    {
        HCRYPTKEY hHash;
        ret = CryptCreateHash(hCryptProv,
                              CALG_MD5,
                              0,
                              0,
                              &hHash);
        if (!ret)
        {
            dwErr = GetLastError();
            g_free(blob);
            error("CryptCreateHash failed with %lx\n", dwErr);
            return;
        }
        ret = CryptHashData(hHash,
                            (BYTE *)key,
                            keylen,
                            0);
        if (!ret)
        {
            dwErr = GetLastError();
            g_free(blob);
            error("CryptHashData failed with %lx\n", dwErr);
            return;
        }
        ret = CryptGetHashParam(hHash,
                                HP_HASHVAL,
                                NULL,
                                &dwDataLen,
                                0);
        if (!ret)
        {
            dwErr = GetLastError();
            g_free(blob);
            error("CryptGetHashParam failed with %lx\n", dwErr);
            return;
        }
        ret = CryptGetHashParam(hHash,
                                HP_HASHVAL,
                                sum,
                                &dwDataLen,
                                0);
        if (!ret)
        {
            dwErr = GetLastError();
            g_free(blob);
            error("CryptGetHashParam failed with %lx\n", dwErr);
            return;
        }
        keylen = dwDataLen;
        key = (char *)sum;
    }
    *keySize = keylen;
    memcpy(keyBuf, key, keylen);
    ret = CryptImportKey(hCryptProv,
                         blob,
                         sizeof(PUBLICKEYSTRUC) + sizeof(DWORD) + keylen,
                         0,
                         CRYPT_EXPORTABLE,
                         &hKey);
    g_free(blob);
    if (!ret)
    {
        dwErr = GetLastError();
        error("CryptImportKey failed with %lx\n", dwErr);
        return;
    }
    ret = CryptCreateHash(hCryptProv,
                          CALG_HMAC,
                          hKey,
                          0,
                          &hHash);
    if (!ret)
    {
        dwErr = GetLastError();
        error("CryptCreateHash failed with %lx\n", dwErr);
        return;
    }
    info.HashAlgid = CALG_MD5;
    info.cbInnerString = 0;
    info.cbOuterString = 0;
    ret = CryptSetHashParam(hHash,
                            HP_HMAC_INFO,
                            (BYTE *)&info,
                            0);
    if (!ret)
    {
        dwErr = GetLastError();
        error("CryptSetHashParam failed with %lx\n", dwErr);
        return;
    }
    ret = CryptHashData(hHash,
                        (BYTE *)data,
                        len,
                        0);
    if (!ret)
    {
        dwErr = GetLastError();
        error("CryptHashData failed with %lx\n", dwErr);
        return;
    }
    ret = CryptGetHashParam(hHash,
                            HP_HASHVAL,
                            NULL,
                            &dwDataLen,
                            0);
    if (!ret)
    {
        dwErr = GetLastError();
        error("CryptGetHashParam failed with %lx\n", dwErr);
        return;
    }
    ret = CryptGetHashParam(hHash,
                            HP_HASHVAL,
                            (BYTE *)output,
                            &dwDataLen,
                            0);
    if (!ret)
    {
        dwErr = GetLastError();
        error("CryptGetHashParam failed with %lx\n", dwErr);
        return;
    }
    CryptDestroyHash(hHash);
    ret = CryptReleaseContext(hCryptProv, 0);
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
static DIGIT_T
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
static void
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
static int
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
static void
mpSetZero(DIGIT_T* a, unsigned int ndigits)
{ /* Sets a = 0 */
  unsigned int i;

  for (i = 0; i < ndigits; i++)
  {
    a[i] = 0;
  }
}

/*****************************************************************************/
static void
mpSetEqual(DIGIT_T* a, DIGIT_T* b, unsigned int ndigits)
{  /* Sets a = b */
  unsigned int i;

  for (i = 0; i < ndigits; i++)
  {
    a[i] = b[i];
  }
}

/*****************************************************************************/
static unsigned int
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
static DIGIT_T
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
static DIGIT_T
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
static void
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
static int
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
static DIGIT_T
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
static int
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
static DIGIT_T
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
static DIGIT_T
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
static int
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
static int
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
static int
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
static int
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
int
rdssl_mod_exp(char* out, int out_len, char* in, int in_len,
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

static uint8 g_ppk_n[72] =
{
    0x3D, 0x3A, 0x5E, 0xBD, 0x72, 0x43, 0x3E, 0xC9,
    0x4D, 0xBB, 0xC1, 0x1E, 0x4A, 0xBA, 0x5F, 0xCB,
    0x3E, 0x88, 0x20, 0x87, 0xEF, 0xF5, 0xC1, 0xE2,
    0xD7, 0xB7, 0x6B, 0x9A, 0xF2, 0x52, 0x45, 0x95,
    0xCE, 0x63, 0x65, 0x6B, 0x58, 0x3A, 0xFE, 0xEF,
    0x7C, 0xE7, 0xBF, 0xFE, 0x3D, 0xF6, 0x5C, 0x7D,
    0x6C, 0x5E, 0x06, 0x09, 0x1A, 0xF5, 0x61, 0xBB,
    0x20, 0x93, 0x09, 0x5F, 0x05, 0x6D, 0xEA, 0x87,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static uint8 g_ppk_d[108] =
{
    0x87, 0xA7, 0x19, 0x32, 0xDA, 0x11, 0x87, 0x55,
    0x58, 0x00, 0x16, 0x16, 0x25, 0x65, 0x68, 0xF8,
    0x24, 0x3E, 0xE6, 0xFA, 0xE9, 0x67, 0x49, 0x94,
    0xCF, 0x92, 0xCC, 0x33, 0x99, 0xE8, 0x08, 0x60,
    0x17, 0x9A, 0x12, 0x9F, 0x24, 0xDD, 0xB1, 0x24,
    0x99, 0xC7, 0x3A, 0xB8, 0x0A, 0x7B, 0x0D, 0xDD,
    0x35, 0x07, 0x79, 0x17, 0x0B, 0x51, 0x9B, 0xB3,
    0xC7, 0x10, 0x01, 0x13, 0xE7, 0x3F, 0xF3, 0x5F,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

int
rdssl_sign_ok(char* e_data, int e_len, char* n_data, int n_len,
    char* sign_data, int sign_len, char* sign_data2, int sign_len2, char* testkey)
{
    char* key;
    char* md5_final;
    void* md5;

    if ((e_len != 4) || (n_len != 64) || (sign_len != 64) || (sign_len2 != 64))
    {
        return 1;
    }
    md5 = rdssl_md5_info_create();
    if (!md5)
    {
        return 1;
    }
    key = (char*)xmalloc(176);
    md5_final = (char*)xmalloc(64);
    // copy the test key
    memcpy(key, testkey, 176);
    // replace e and n
    memcpy(key + 32, e_data, 4);
    memcpy(key + 36, n_data, 64);
    rdssl_md5_clear(md5);
    // the first 108 bytes
    rdssl_md5_transform(md5, key, 108);
    // set the whole thing with 0xff
    memset(md5_final, 0xff, 64);
    // digest 16 bytes
    rdssl_md5_complete(md5, md5_final);
    // set non 0xff array items
    md5_final[16] = 0;
    md5_final[62] = 1;
    md5_final[63] = 0;
    // encrypt
    rdssl_mod_exp(sign_data, 64, md5_final, 64, (char*)g_ppk_n, 64,
        (char*)g_ppk_d, 64);
    // cleanup
    rdssl_md5_info_delete(md5);
    xfree(key);
    xfree(md5_final);
    return memcmp(sign_data, sign_data2, sign_len2);
}

/*****************************************************************************/
PCCERT_CONTEXT rdssl_cert_read(uint8 * data, uint32 len)
{
    PCCERT_CONTEXT res;
    if (!data || !len)
    {
        error("rdssl_cert_read %p %ld\n", data, len);
        return NULL;
    }
    res = CertCreateCertificateContext(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, data, len);
    if (!res)
    {
        error("CertCreateCertificateContext call failed with %lx\n", GetLastError());
    }
    return res;
}

/*****************************************************************************/
void rdssl_cert_free(PCCERT_CONTEXT context)
{
    if (context)
        CertFreeCertificateContext(context);
}

/*****************************************************************************/
uint8 *rdssl_cert_to_rkey(PCCERT_CONTEXT cert, uint32 * key_len)
{
    HCRYPTPROV hCryptProv;
    HCRYPTKEY hKey;
    BOOL ret;
    BYTE * rkey;
    DWORD dwSize, dwErr;
    ret = CryptAcquireContext(&hCryptProv,
                              NULL,
                              MS_ENHANCED_PROV,
                              PROV_RSA_FULL,
                              0);
    if (!ret)
    {
        dwErr = GetLastError();
        if (dwErr == NTE_BAD_KEYSET)
        {
            ret = CryptAcquireContext(&hCryptProv,
                                      L"MSTSC",
                                      MS_ENHANCED_PROV,
                                      PROV_RSA_FULL,
                                      CRYPT_NEWKEYSET);
        }
    }
    if (!ret)
    {
        dwErr = GetLastError();
        error("CryptAcquireContext call failed with %lx\n", dwErr);
        return NULL;
    }
    ret = CryptImportPublicKeyInfoEx(hCryptProv,
                                     X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                     &(cert->pCertInfo->SubjectPublicKeyInfo),
                                     0,
                                     0,
                                     NULL,
                                     &hKey);
    if (!ret)
    {
        dwErr = GetLastError();
        CryptReleaseContext(hCryptProv, 0);
        error("CryptImportPublicKeyInfoEx call failed with %lx\n", dwErr);
        return NULL;
    }
    ret = CryptExportKey(hKey,
                         0,
                         PUBLICKEYBLOB,
                         0,
                         NULL,
                         &dwSize);
    if (!ret)
    {
        dwErr = GetLastError();
        CryptDestroyKey(hKey);
        CryptReleaseContext(hCryptProv, 0);
        error("CryptExportKey call failed with %lx\n", dwErr);
        return NULL;
    }
    rkey = g_malloc(dwSize, 0);
    ret = CryptExportKey(hKey,
                         0,
                         PUBLICKEYBLOB,
                         0,
                         rkey,
                         &dwSize);
    if (!ret)
    {
        dwErr = GetLastError();
        g_free(rkey);
        CryptDestroyKey(hKey);
        CryptReleaseContext(hCryptProv, 0);
        error("CryptExportKey call failed with %lx\n", dwErr);
        return NULL;
    }
    CryptDestroyKey(hKey);
    CryptReleaseContext(hCryptProv, 0);
    return rkey;
}

/*****************************************************************************/
RD_BOOL rdssl_certs_ok(PCCERT_CONTEXT server_cert, PCCERT_CONTEXT cacert)
{
    /* FIXME should we check for expired certificates??? */
    DWORD dwFlags = CERT_STORE_SIGNATURE_FLAG; /* CERT_STORE_TIME_VALIDITY_FLAG */
    BOOL ret = CertVerifySubjectCertificateContext(server_cert,
                                                   cacert,
                                                   &dwFlags);
    if (!ret)
    {
        error("CertVerifySubjectCertificateContext call failed with %lx\n", GetLastError());
    }
    if (dwFlags)
    {
        error("CertVerifySubjectCertificateContext check failed %lx\n", dwFlags);
    }
    return (dwFlags == 0);
}

/*****************************************************************************/
int rdssl_rkey_get_exp_mod(uint8 * rkey, uint8 * exponent, uint32 max_exp_len, uint8 * modulus,
    uint32 max_mod_len)
{
    RSAPUBKEY *desc = (RSAPUBKEY *)(rkey + sizeof(PUBLICKEYSTRUC));
    if (!rkey || !exponent || !max_exp_len || !modulus || !max_mod_len)
    {
        error("rdssl_rkey_get_exp_mod %p %p %ld %p %ld\n", rkey, exponent, max_exp_len, modulus, max_mod_len);
        return -1;
    }
    memcpy (exponent, &desc->pubexp, max_exp_len);
    memcpy (modulus, rkey + sizeof(PUBLICKEYSTRUC) + sizeof(RSAPUBKEY), max_mod_len);
    return 0;
}

/*****************************************************************************/
void rdssl_rkey_free(uint8 * rkey)
{
    if (!rkey)
    {
        error("rdssl_rkey_free rkey is null\n");
        return;
    }
    g_free(rkey);
}
