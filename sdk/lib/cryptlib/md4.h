
#pragma once

#include <ntdef.h>

typedef struct
{
    unsigned int buf[4];
    unsigned int i[2];
    unsigned char in[64];
    unsigned char digest[16];
} MD4_CTX;

VOID NTAPI MD4Init( MD4_CTX *ctx );

VOID NTAPI MD4Update( MD4_CTX *ctx, const unsigned char *buf, unsigned int len );

VOID NTAPI MD4Final( MD4_CTX *ctx );

