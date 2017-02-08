
#include "ntlmssp.h"
#include "ciphers.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

/* not in any header, DES HASH! */
LONG
WINAPI SystemFunction001(const BYTE *data, const BYTE *key, LPBYTE output);

static UINT32 crc32_table[256];
static int crc32_initialized = 0;

//CRC-32-IEEE 802.3
static void
crc32_make_table()
{
    UINT32 h = 1;
    unsigned int i, j;

    memset(crc32_table, 0, sizeof(crc32_table));

    for (i = 128; i; i >>= 1)
    {
        h = (h >> 1) ^ ((h & 1) ? 0xedb88320L : 0);

        for (j = 0; j < 256; j += 2 * i)
            crc32_table[i + j] = crc32_table[j] ^ h;
    }

    crc32_initialized = 1;
}

static UINT32
crc32(UINT32 crc, const unsigned char *buf, int len)
{
    if (!crc32_initialized)
        crc32_make_table();

    if (!buf || len < 0)
        return crc;

    crc ^= 0xffffffffL;

    while (len--)
        crc = (crc >> 8) ^ crc32_table[(crc ^ *buf++) & 0xff];

    return crc ^ 0xffffffffL;
}

static inline void
swap_bytes(unsigned char *a, unsigned char *b)
{
    unsigned char swapByte;

    swapByte = *a;
    *a = *b;
    *b = swapByte;
}

void
rc4_init(rc4_key *const state, const unsigned char *key, int keylen)
{
    unsigned char j;
    int i;

    /* Initialize state with identity permutation */
    for (i = 0; i < 256; i++)
        state->perm[i] = (unsigned char)i; 
    state->index1 = 0;
    state->index2 = 0;
  
    /* Randomize the permutation using key data */
    for (j = i = 0; i < 256; i++) {
        j += state->perm[i] + key[i % keylen]; 
        swap_bytes(&state->perm[i], &state->perm[j]);
    }
}

void
rc4_crypt(rc4_key *const state, const unsigned char *inbuf, unsigned char *outbuf, int buflen)
{
    int i;
    unsigned char j;

    for (i = 0; i < buflen; i++)
    {
        /* Update modification indicies */
        state->index1++;
        state->index2 += state->perm[state->index1];

        /* Modify permutation */
        swap_bytes(&state->perm[state->index1],
            &state->perm[state->index2]);

        /* Encrypt/decrypt next byte */
        j = state->perm[state->index1] + state->perm[state->index2];
        outbuf[i] = inbuf[i] ^ state->perm[j];
    }
}

void
HMACMD5Init(HMAC_MD5_CTX *ctx, const unsigned char *key, unsigned int key_len)
{
    int i;
    unsigned char inner_padding[64];
    unsigned char temp_key[16];

    if(key_len > 64)
    {
        MD5_CTX temp_ctx;

        MD5Init(&temp_ctx);
        MD5Update(&temp_ctx, key, key_len);
        MD5Final(&temp_ctx);
        memcpy(temp_key, temp_ctx.digest, 16);

        key = temp_key;
        key_len = 16;
    }

    memset(inner_padding, 0, 64);
    memset(ctx->outer_padding, 0, 64);
    memcpy(inner_padding, key, key_len);
    memcpy(ctx->outer_padding, key, key_len);

    for(i = 0; i < 64; ++i)
    {
        inner_padding[i] ^= 0x36;
        ctx->outer_padding[i] ^= 0x5c;
    }

    MD5Init(&(ctx->ctx));
    MD5Update(&(ctx->ctx), inner_padding, 64);
}

void
HMACMD5Update(HMAC_MD5_CTX *ctx, const unsigned char *data, unsigned int data_len)
{
    MD5Update(&(ctx->ctx), data, data_len);
}

void
HMACMD5Final(HMAC_MD5_CTX *ctx, unsigned char *digest)
{
    MD5_CTX outer_ctx;
    unsigned char inner_digest[16];

    MD5Final(&(ctx->ctx));
    memcpy(inner_digest, ctx->ctx.digest, 16);

    MD5Init(&outer_ctx);
    MD5Update(&outer_ctx, ctx->outer_padding, 64);
    MD5Update(&outer_ctx, inner_digest, 16);
    MD5Final(&outer_ctx);

    memcpy(digest, outer_ctx.digest, 16);
}

/* high level */

void
RC4K (const unsigned char * k, unsigned long key_len, const unsigned char * d, int len, unsigned char * result)
{
    rc4_key rc4key;

    rc4_init(&rc4key, k, key_len);
    rc4_crypt(&rc4key, d, result, len);
}

UINT32
CRC32(const char *msg, int len)
{
    UINT32 crc = 0L;
    crc = crc32(crc, (unsigned char *) msg, len);
    return crc;
}

// (k = 7 byte key, d = 8 byte data) returns 8 bytes in results
void
DES(const unsigned char *k, const unsigned char *d, unsigned char * results)
{
    SystemFunction001(d, k, (LPBYTE)results);
}

// (K = 21 byte key, D = 8 bytes of data) returns 24 bytes in results:
void
DESL(unsigned char *k, const unsigned char *d, unsigned char * results)
{
    unsigned char keys[21];

    /* copy the first 16 bytes */
    memcpy(keys, k, 16);

    /* zero out the last 5 bytes of the key */
    memset(keys + 16, 0, 5);

    DES(keys,      d, results);
    DES(keys + 7,  d, results + 8);
    DES(keys + 14, d, results + 16);
}

void
MD4(const unsigned char * d, int len, unsigned char * result)
{
    MD4_CTX ctx;

    MD4Init(&ctx);
    MD4Update(&ctx, d, len);
    MD4Final(&ctx);
    memcpy(result, ctx.digest, len);
}

void
MD5(const unsigned char * d, int len, unsigned char * result)
{
    MD5_CTX ctx;

    MD5Init(&ctx);
    MD5Update(&ctx, d, len);
    MD5Final(&ctx);
    memcpy(result, ctx.digest, 0x10);
}

void
HMAC_MD5(const unsigned char *key, int key_len, const unsigned char *data, int data_len, unsigned char *result)
{
    HMAC_MD5_CTX ctx;

    HMACMD5Init(&ctx, key, key_len);
    HMACMD5Update(&ctx, data, data_len);
    HMACMD5Final(&ctx, result);
}

