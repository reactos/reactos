
#pragma once

typedef struct _RC4_CONTEXT
{
    unsigned char state[256];
    unsigned char x, y;
} RC4_CONTEXT;

void rc4_init(RC4_CONTEXT *a4i, const unsigned char *key, unsigned int keyLen);

void rc4_crypt(RC4_CONTEXT *a4i, unsigned char *inoutString, unsigned int length);

