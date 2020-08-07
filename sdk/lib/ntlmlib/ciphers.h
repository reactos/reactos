/*
 * PROJECT:     ntlmlib
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ciphers for ntlmlib (headers)
 * COPYRIGHT:   Copyright 2011 Samuel Serapión
 *              Copyright 2020 Andreas Maier (staubim@quantentunnel.de)
 *
 */

#ifndef _CYPHERS_H_
#define _CYPHERS_H_

typedef struct
{
    unsigned int buf[4];
    unsigned int i[2];
    unsigned char in[64];
    unsigned char digest[16];
}MD4_CTX;

/* advapi32 */
void WINAPI MD4Init(MD4_CTX *ctx);
void WINAPI MD4Update(MD4_CTX *ctx, const unsigned char *buf, unsigned int len);
void WINAPI MD4Final(MD4_CTX *ctx);

typedef struct
{
    unsigned int i[2];
    unsigned int buf[4];
    unsigned char in[64];
    unsigned char digest[16];
}MD5_CTX;

/* advapi32 */
void WINAPI MD5Init(MD5_CTX *ctx);
void WINAPI MD5Update(MD5_CTX *ctx, const unsigned char *buf, unsigned int len);
void WINAPI MD5Final(MD5_CTX *ctx);

typedef struct
{
    MD5_CTX ctx;
    unsigned char outer_padding[64];
} HMAC_MD5_CTX;

void HMACMD5Init(HMAC_MD5_CTX *ctx, const unsigned char *key, unsigned int key_len);
void HMACMD5Update(HMAC_MD5_CTX *ctx, const unsigned char *data, unsigned int data_len);
void HMACMD5Final(HMAC_MD5_CTX *ctx, UCHAR digest[16]);

/* high level api */
UINT32
CRC32 (const char *msg, int len);

void
DES(const UCHAR k[7], const UCHAR d[8], UCHAR results[8]);

/* Calls SystemFuntion009; Computes a "NT response"
 * HINT: Params 1 + 2 swapped in comparison to SystemFunction009 */
NTSTATUS
DESL(_In_ UCHAR PasswordHash[16],
    _In_ UCHAR Challenge[8],
    _Out_ UCHAR Result[24]);

void
MD4(const unsigned char * d, int len, unsigned char * result);

void
MD5(const unsigned char * d, int len, unsigned char * result);

void
HMAC_MD5(const unsigned char *key, int key_len, const unsigned char *data, int data_len, UCHAR result[16]);
#endif
