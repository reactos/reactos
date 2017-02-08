
#pragma once

#include <ntdef.h>

typedef struct
{
    unsigned int i[2];
    unsigned int buf[4];
    unsigned char in[64];
    unsigned char digest[16];
} MD5_CTX;

VOID NTAPI MD5Init( MD5_CTX *ctx );

VOID NTAPI MD5Update( MD5_CTX *ctx, const unsigned char *buf, unsigned int len );

VOID NTAPI MD5Final( MD5_CTX *ctx );

