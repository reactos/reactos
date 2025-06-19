#ifndef __BCRYPT_INTERNAL_H
#define __BCRYPT_INTERNAL_H

#include "precomp.h"

#define MAGIC_DSS1 ('D' | ('S' << 8) | ('S' << 16) | ('1' << 24))
#define MAGIC_DSS2 ('D' | ('S' << 8) | ('S' << 16) | ('2' << 24))

#define MAGIC_ALG  (('A' << 24) | ('L' << 16) | ('G' << 8) | '0')
#define MAGIC_HASH (('H' << 24) | ('A' << 16) | ('S' << 8) | 'H')
#define MAGIC_KEY  (('K' << 24) | ('E' << 16) | ('Y' << 8) | '0')
#define MAGIC_SECRET (('S' << 24) | ('C' << 16) | ('R' << 8) | 'T')
struct object
{
    ULONG magic;
};

enum alg_id
{
    /* cipher */
    ALG_ID_3DES,
    ALG_ID_AES,
    ALG_ID_RC4,

    /* hash */
    ALG_ID_SHA256,
    ALG_ID_SHA384,
    ALG_ID_SHA512,
    ALG_ID_SHA1,
    ALG_ID_MD5,
    ALG_ID_MD4,
    ALG_ID_MD2,

    /* asymmetric encryption */
    ALG_ID_RSA,

    /* secret agreement */
    ALG_ID_DH,
    ALG_ID_ECDH_P256,
    ALG_ID_ECDH_P384,

    /* signature */
    ALG_ID_RSA_SIGN,
    ALG_ID_ECDSA_P256,
    ALG_ID_ECDSA_P384,
    ALG_ID_DSA,

    /* rng */
    ALG_ID_RNG,

    /* key derivation */
    ALG_ID_PBKDF2,
};

enum chain_mode
{
    CHAIN_MODE_CBC,
    CHAIN_MODE_ECB,
    CHAIN_MODE_CFB,
    CHAIN_MODE_CCM,
    CHAIN_MODE_GCM,
};

struct algorithm
{
    struct object   hdr;
    enum alg_id     id;
    BOOL            hmac;
    enum chain_mode mode;
    unsigned        flags;
};

struct key_symmetric
{
    enum chain_mode  mode;
    ULONG            block_size;
    UCHAR           *vector;
    ULONG            vector_len;
    UCHAR           *secret;
    unsigned         secret_len;
    CRITICAL_SECTION cs;
};

#define KEY_FLAG_LEGACY_DSA_V2  0x00000001
#define KEY_FLAG_FINALIZED      0x00000002

struct key_asymmetric
{
    ULONG             bitlen;     /* ignored for ECC keys */
    unsigned          flags;
    DSSSEED           dss_seed;
};

#define PRIVATE_DATA_SIZE 3
struct key
{
    struct object hdr;
    enum alg_id   alg_id;
    UINT64        private[PRIVATE_DATA_SIZE];  /* private data for backend */
    union
    {
        struct key_symmetric s;
        struct key_asymmetric a;
    } u;
};

struct secret
{
    struct object hdr;
    struct key *privkey;
    struct key *pubkey;
};

struct key_symmetric_set_auth_data_params
{
    struct key  *key;
    UCHAR       *auth_data;
    ULONG        len;
#ifdef __REACTOS__
    BOOL         encrypt;
#endif
};

struct key_symmetric_encrypt_params
{
    struct key  *key;
    const UCHAR *input;
    unsigned     input_len;
    UCHAR       *output;
    ULONG        output_len;
};

struct key_symmetric_decrypt_params
{
    struct key  *key;
    const UCHAR *input;
    unsigned     input_len;
    UCHAR       *output;
    ULONG        output_len;
};

struct key_symmetric_get_tag_params
{
    struct key  *key;
    UCHAR       *tag;
    ULONG        len;
#ifdef __REACTOS__
    BOOL         encrypt;
#endif
};

struct key_asymmetric_decrypt_params
{
    struct key  *key;
    UCHAR       *input;
    unsigned     input_len;
    void        *padding;
    UCHAR       *output;
    ULONG        output_len;
    ULONG       *ret_len;
    ULONG        flags;
};

struct key_asymmetric_encrypt_params
{
    struct key  *key;
    UCHAR       *input;
    unsigned     input_len;
    void        *padding;
    UCHAR       *output;
    ULONG        output_len;
    ULONG       *ret_len;
    ULONG        flags;
};

struct key_asymmetric_duplicate_params
{
    struct key *key_orig;
    struct key *key_copy;
};

struct key_asymmetric_sign_params
{
    struct key  *key;
    void        *padding;
    UCHAR       *input;
    unsigned     input_len;
    UCHAR       *output;
    ULONG        output_len;
    ULONG       *ret_len;
    unsigned     flags;
};

struct key_asymmetric_verify_params
{
    struct key *key;
    void       *padding;
    UCHAR      *hash;
    unsigned    hash_len;
    UCHAR      *signature;
    ULONG       signature_len;
    unsigned    flags;
};

#define KEY_EXPORT_FLAG_PUBLIC        0x00000001
#define KEY_EXPORT_FLAG_RSA_FULL      0x00000002
#define KEY_EXPORT_FLAG_DH_PARAMETERS 0x00000004

struct key_asymmetric_export_params
{
    struct key  *key;
    ULONG        flags;
    UCHAR       *buf;
    ULONG        len;
    ULONG       *ret_len;
};

#define KEY_IMPORT_FLAG_PUBLIC        0x00000001
#define KEY_IMPORT_FLAG_DH_PARAMETERS 0x00000002

struct key_asymmetric_import_params
{
    struct key  *key;
    ULONG        flags;
    UCHAR       *buf;
    ULONG        len;
};

struct key_asymmetric_derive_key_params
{
    struct key *privkey;
    struct key *pubkey;
    UCHAR      *output;
    ULONG       output_len;
    ULONG      *ret_len;
};

#if defined(HAVE_GNUTLS_HASH)
struct hash
{
    struct object    hdr;
    enum alg_id      alg_id;
    BOOL hmac;
    union
    {
        gnutls_hash_hd_t hash_handle;
        gnutls_hmac_hd_t hmac_handle;
    } u;
};
#elif defined(SONAME_LIBMBEDTLS)
struct hash
{
    struct object hdr;
    enum alg_id   alg_id;
    BOOL hmac;
    union
    {
        mbedtls_md5_context    md5_ctx;
        mbedtls_sha1_context   sha1_ctx;
        mbedtls_sha256_context sha256_ctx;
        mbedtls_sha512_context sha512_ctx;
        mbedtls_md_context_t   hmac_ctx;
    } u;
};
#else
struct hash
{
    struct object hdr;
    enum alg_id   alg_id;
    BOOL hmac;
    union
    {
        CC_MD5_CTX    md5_ctx;
        CC_SHA1_CTX   sha1_ctx;
        CC_SHA256_CTX sha256_ctx;
        CC_SHA512_CTX sha512_ctx;
        CCHmacContext hmac_ctx;
    } u;
};
#endif

static const struct {
    ULONG hash_length;
    const WCHAR *alg_name;
} alg_props[] = {
    /* ALG_ID_MD5    */ { 16, BCRYPT_MD5_ALGORITHM },
    /* ALG_ID_RNG    */ {  0, BCRYPT_RNG_ALGORITHM },
    /* ALG_ID_SHA1   */ { 20, BCRYPT_SHA1_ALGORITHM },
    /* ALG_ID_SHA256 */ { 32, BCRYPT_SHA256_ALGORITHM },
    /* ALG_ID_SHA384 */ { 48, BCRYPT_SHA384_ALGORITHM },
    /* ALG_ID_SHA512 */ { 64, BCRYPT_SHA512_ALGORITHM },
    /* ALG_ID_ECDSA_P256 */ { 0, BCRYPT_ECDSA_P256_ALGORITHM },
    /* ALG_ID_ECDSA_P384 */ { 0, BCRYPT_ECDSA_P384_ALGORITHM },
};

extern NTSTATUS hash_init( struct hash *hash );
extern NTSTATUS hmac_init( struct hash *hash, UCHAR *key, ULONG key_size );
extern NTSTATUS hash_update( struct hash *hash, UCHAR *input, ULONG size );
extern NTSTATUS hmac_update( struct hash *hash, UCHAR *input, ULONG size );
extern NTSTATUS hash_finish( struct hash *hash, UCHAR *output, ULONG size );
extern NTSTATUS hmac_finish( struct hash *hash, UCHAR *output, ULONG size );

extern NTSTATUS process_detach( void *args );
extern NTSTATUS process_attach( void *args );

#endif /* __BCRYPT_INTERNAL_H */
