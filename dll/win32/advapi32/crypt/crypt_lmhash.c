/*
 *  Copyright 2004 Hans Leidekker
 *
 *  Based on LMHash.c from libcifs
 *
 *  Copyright (C) 2004 by Christopher R. Hertel
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <advapi32.h>

static const unsigned char CRYPT_LMhash_Magic[8] =
    { 'K', 'G', 'S', '!', '@', '#', '$', '%' };

static void CRYPT_LMhash( unsigned char *dst, const unsigned char *pwd, const int len )
{
    int i, max = 14;
    unsigned char tmp_pwd[14] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

    max = len > max ? max : len;

    for (i = 0; i < max; i++)
        tmp_pwd[i] = pwd[i];

    CRYPT_DEShash( dst, tmp_pwd, CRYPT_LMhash_Magic );
    CRYPT_DEShash( &dst[8], &tmp_pwd[7], CRYPT_LMhash_Magic );
}

NTSTATUS WINAPI SystemFunction006( LPCSTR password, LPSTR hash )
{
    CRYPT_LMhash( (unsigned char*)hash, (const unsigned char*)password, strlen(password) );

    return STATUS_SUCCESS;
}

#ifndef __REACTOS__
/******************************************************************************
 * SystemFunction008  [ADVAPI32.@]
 *
 * Creates a LM response from a challenge and a password hash
 *
 * PARAMS
 *   challenge  [I] Challenge from authentication server
 *   hash       [I] NTLM hash (from SystemFunction006)
 *   response   [O] response to send back to the server
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_UNSUCCESSFUL
 *
 * NOTES
 *  see http://davenport.sourceforge.net/ntlm.html#theLmResponse
 *
 */
NTSTATUS WINAPI SystemFunction008(const BYTE *challenge, const BYTE *hash, LPBYTE response)
{
    BYTE key[7*3];

    if (!challenge || !response)
        return STATUS_UNSUCCESSFUL;

    memset(key, 0, sizeof key);
    memcpy(key, hash, 0x10);

    CRYPT_DEShash(response, key, challenge);
    CRYPT_DEShash(response+8, key+7, challenge);
    CRYPT_DEShash(response+16, key+14, challenge);

    return STATUS_SUCCESS;
}

/******************************************************************************
 * SystemFunction009  [ADVAPI32.@]
 *
 * Seems to do the same as SystemFunction008 ...
 */
NTSTATUS WINAPI SystemFunction009(const BYTE *challenge, const BYTE *hash, LPBYTE response)
{
    return SystemFunction008(challenge, hash, response);
}

/******************************************************************************
 * SystemFunction001  [ADVAPI32.@]
 *
 * Encrypts a single block of data using DES
 *
 * PARAMS
 *   data    [I] data to encrypt    (8 bytes)
 *   key     [I] key data           (7 bytes)
 *   output  [O] the encrypted data (8 bytes)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_UNSUCCESSFUL
 *
 */
NTSTATUS WINAPI SystemFunction001(const BYTE *data, const BYTE *key, LPBYTE output)
{
    if (!data || !output)
        return STATUS_UNSUCCESSFUL;
    CRYPT_DEShash(output, key, data);
    return STATUS_SUCCESS;
}

/******************************************************************************
 * SystemFunction002  [ADVAPI32.@]
 *
 * Decrypts a single block of data using DES
 *
 * PARAMS
 *   data    [I] data to decrypt    (8 bytes)
 *   key     [I] key data           (7 bytes)
 *   output  [O] the decrypted data (8 bytes)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_UNSUCCESSFUL
 *
 */
NTSTATUS WINAPI SystemFunction002(const BYTE *data, const BYTE *key, LPBYTE output)
{
    if (!data || !output)
        return STATUS_UNSUCCESSFUL;
    CRYPT_DESunhash(output, key, data);
    return STATUS_SUCCESS;
}

/******************************************************************************
 * SystemFunction003  [ADVAPI32.@]
 *
 * Hashes a key using DES and a fixed datablock
 *
 * PARAMS
 *   key     [I] key data    (7 bytes)
 *   output  [O] hashed key  (8 bytes)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_UNSUCCESSFUL
 *
 */
NTSTATUS WINAPI SystemFunction003(const BYTE *key, LPBYTE output)
{
    if (!output)
        return STATUS_UNSUCCESSFUL;
    CRYPT_DEShash(output, key, CRYPT_LMhash_Magic);
    return STATUS_SUCCESS;
}

/******************************************************************************
 * SystemFunction004  [ADVAPI32.@]
 *
 * Encrypts a block of data with DES in ECB mode, preserving the length
 *
 * PARAMS
 *   data    [I] data to encrypt
 *   key     [I] key data (up to 7 bytes)
 *   output  [O] buffer to receive encrypted data
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_BUFFER_TOO_SMALL     if the output buffer is too small
 *  Failure: STATUS_INVALID_PARAMETER_2  if the key is zero length
 *
 * NOTES
 *  Encrypt buffer size should be input size rounded up to 8 bytes
 *   plus an extra 8 bytes.
 */
NTSTATUS WINAPI SystemFunction004(const struct ustring *in,
                                  const struct ustring *key,
                                  struct ustring *out)
{
    union {
         unsigned char uc[8];
         unsigned int  ui[2];
    } data;
    unsigned char deskey[7];
    unsigned int crypt_len, ofs;

    if (key->Length<=0)
        return STATUS_INVALID_PARAMETER_2;

    crypt_len = ((in->Length+7)&~7);
    if (out->MaximumLength < (crypt_len+8))
        return STATUS_BUFFER_TOO_SMALL;

    data.ui[0] = in->Length;
    data.ui[1] = 1;

    if (key->Length<sizeof deskey)
    {
        memset(deskey, 0, sizeof deskey);
        memcpy(deskey, key->Buffer, key->Length);
    }
    else
        memcpy(deskey, key->Buffer, sizeof deskey);

    CRYPT_DEShash(out->Buffer, deskey, data.uc);

    for(ofs=0; ofs<(crypt_len-8); ofs+=8)
        CRYPT_DEShash(out->Buffer+8+ofs, deskey, in->Buffer+ofs);

    memset(data.uc, 0, sizeof data.uc);
    memcpy(data.uc, in->Buffer+ofs, in->Length +8-crypt_len);
    CRYPT_DEShash(out->Buffer+8+ofs, deskey, data.uc);

    out->Length = crypt_len+8;

    return STATUS_SUCCESS;
}

/******************************************************************************
 * SystemFunction005  [ADVAPI32.@]
 *
 * Decrypts a block of data with DES in ECB mode
 *
 * PARAMS
 *   data    [I] data to decrypt
 *   key     [I] key data (up to 7 bytes)
 *   output  [O] buffer to receive decrypted data
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_BUFFER_TOO_SMALL     if the output buffer is too small
 *  Failure: STATUS_INVALID_PARAMETER_2  if the key is zero length
 *
 */
NTSTATUS WINAPI SystemFunction005(const struct ustring *in,
                                  const struct ustring *key,
                                  struct ustring *out)
{
    union {
         unsigned char uc[8];
         unsigned int  ui[2];
    } data;
    unsigned char deskey[7];
    unsigned int ofs, crypt_len;

    if (key->Length<=0)
        return STATUS_INVALID_PARAMETER_2;

    if (key->Length<sizeof deskey)
    {
        memset(deskey, 0, sizeof deskey);
        memcpy(deskey, key->Buffer, key->Length);
    }
    else
        memcpy(deskey, key->Buffer, sizeof deskey);

    CRYPT_DESunhash(data.uc, deskey, in->Buffer);

    if (data.ui[1] != 1)
        return STATUS_UNKNOWN_REVISION;

    crypt_len = data.ui[0];
    if (crypt_len > out->MaximumLength)
        return STATUS_BUFFER_TOO_SMALL;

    for (ofs=0; (ofs+8)<crypt_len; ofs+=8)
        CRYPT_DESunhash(out->Buffer+ofs, deskey, in->Buffer+ofs+8);

    if (ofs<crypt_len)
    {
        CRYPT_DESunhash(data.uc, deskey, in->Buffer+ofs+8);
        memcpy(out->Buffer+ofs, data.uc, crypt_len-ofs);
    }

    out->Length = crypt_len;

    return STATUS_SUCCESS;
}

/******************************************************************************
 * SystemFunction012  [ADVAPI32.@]
 * SystemFunction014  [ADVAPI32.@]
 * SystemFunction016  [ADVAPI32.@]
 * SystemFunction018  [ADVAPI32.@]
 * SystemFunction020  [ADVAPI32.@]
 * SystemFunction022  [ADVAPI32.@]
 *
 * Encrypts two DES blocks with two keys
 *
 * PARAMS
 *   data    [I] data to encrypt (16 bytes)
 *   key     [I] key data (two lots of 7 bytes)
 *   output  [O] buffer to receive encrypted data (16 bytes)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_UNSUCCESSFUL  if the input or output buffer is NULL
 */
NTSTATUS WINAPI SystemFunction012(const BYTE *in, const BYTE *key, LPBYTE out)
{
    if (!in || !out)
        return STATUS_UNSUCCESSFUL;

    CRYPT_DEShash(out, key, in);
    CRYPT_DEShash(out+8, key+7, in+8);
    return STATUS_SUCCESS;
}

/******************************************************************************
 * SystemFunction013  [ADVAPI32.@]
 * SystemFunction015  [ADVAPI32.@]
 * SystemFunction017  [ADVAPI32.@]
 * SystemFunction019  [ADVAPI32.@]
 * SystemFunction021  [ADVAPI32.@]
 * SystemFunction023  [ADVAPI32.@]
 *
 * Decrypts two DES blocks with two keys
 *
 * PARAMS
 *   data    [I] data to decrypt (16 bytes)
 *   key     [I] key data (two lots of 7 bytes)
 *   output  [O] buffer to receive decrypted data (16 bytes)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_UNSUCCESSFUL  if the input or output buffer is NULL
 */
NTSTATUS WINAPI SystemFunction013(const BYTE *in, const BYTE *key, LPBYTE out)
{
    if (!in || !out)
        return STATUS_UNSUCCESSFUL;

    CRYPT_DESunhash(out, key, in);
    CRYPT_DESunhash(out+8, key+7, in+8);
    return STATUS_SUCCESS;
}

/******************************************************************************
 * SystemFunction024  [ADVAPI32.@]
 *
 * Encrypts two DES blocks with a 32 bit key...
 *
 * PARAMS
 *   data    [I] data to encrypt (16 bytes)
 *   key     [I] key data (4 bytes)
 *   output  [O] buffer to receive encrypted data (16 bytes)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 */
NTSTATUS WINAPI SystemFunction024(const BYTE *in, const BYTE *key, LPBYTE out)
{
    BYTE deskey[0x10];

    memcpy(deskey, key, 4);
    memcpy(deskey+4, key, 4);
    memcpy(deskey+8, key, 4);
    memcpy(deskey+12, key, 4);

    CRYPT_DEShash(out, deskey, in);
    CRYPT_DEShash(out+8, deskey+7, in+8);

    return STATUS_SUCCESS;
}

/******************************************************************************
 * SystemFunction025  [ADVAPI32.@]
 *
 * Decrypts two DES blocks with a 32 bit key...
 *
 * PARAMS
 *   data    [I] data to encrypt (16 bytes)
 *   key     [I] key data (4 bytes)
 *   output  [O] buffer to receive encrypted data (16 bytes)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 */
NTSTATUS WINAPI SystemFunction025(const BYTE *in, const BYTE *key, LPBYTE out)
{
    BYTE deskey[0x10];

    memcpy(deskey, key, 4);
    memcpy(deskey+4, key, 4);
    memcpy(deskey+8, key, 4);
    memcpy(deskey+12, key, 4);

    CRYPT_DESunhash(out, deskey, in);
    CRYPT_DESunhash(out+8, deskey+7, in+8);

    return STATUS_SUCCESS;
}
#endif /* !__REACTOS__ */
