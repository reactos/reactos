/*
 * Copyright 2019 Hans Leidekker for CodeWeavers
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/stat.h>
#ifdef __APPLE__
#include <Security/Security.h>
#endif
#ifdef SONAME_LIBGNUTLS
#include <gnutls/pkcs12.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wincrypt.h"
#include "crypt32_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

#ifdef SONAME_LIBGNUTLS

WINE_DECLARE_DEBUG_CHANNEL(winediag);

/* Not present in gnutls version < 3.0 */
int gnutls_pkcs12_simple_parse(gnutls_pkcs12_t p12, const char *password,
    gnutls_x509_privkey_t *key, gnutls_x509_crt_t **chain, unsigned int *chain_len,
    gnutls_x509_crt_t **extra_certs, unsigned int *extra_certs_len,
    gnutls_x509_crl_t * crl, unsigned int flags);

int gnutls_x509_privkey_get_pk_algorithm2(gnutls_x509_privkey_t, unsigned int*);

static void *libgnutls_handle;
#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(gnutls_global_deinit);
MAKE_FUNCPTR(gnutls_global_init);
MAKE_FUNCPTR(gnutls_global_set_log_function);
MAKE_FUNCPTR(gnutls_global_set_log_level);
MAKE_FUNCPTR(gnutls_perror);
MAKE_FUNCPTR(gnutls_pkcs12_deinit);
MAKE_FUNCPTR(gnutls_pkcs12_import);
MAKE_FUNCPTR(gnutls_pkcs12_init);
MAKE_FUNCPTR(gnutls_pkcs12_simple_parse);
MAKE_FUNCPTR(gnutls_x509_crt_export);
MAKE_FUNCPTR(gnutls_x509_privkey_export_rsa_raw2);
MAKE_FUNCPTR(gnutls_x509_privkey_get_pk_algorithm2);
#undef MAKE_FUNCPTR

static void gnutls_log( int level, const char *msg )
{
    TRACE( "<%d> %s", level, msg );
}

static NTSTATUS process_attach( void *args )
{
    const char *env_str;
    int ret;

    if ((env_str = getenv("GNUTLS_SYSTEM_PRIORITY_FILE")))
    {
        WARN("GNUTLS_SYSTEM_PRIORITY_FILE is %s.\n", debugstr_a(env_str));
    }
    else
    {
        WARN("Setting GNUTLS_SYSTEM_PRIORITY_FILE to \"/dev/null\".\n");
        setenv("GNUTLS_SYSTEM_PRIORITY_FILE", "/dev/null", 0);
    }

    if (!(libgnutls_handle = dlopen( SONAME_LIBGNUTLS, RTLD_NOW )))
    {
        ERR_(winediag)( "failed to load libgnutls, no support for pfx import/export\n" );
        return STATUS_DLL_NOT_FOUND;
    }

#define LOAD_FUNCPTR(f) \
    if (!(p##f = dlsym( libgnutls_handle, #f ))) \
    { \
        ERR( "failed to load %s\n", #f ); \
        goto fail; \
    }

    LOAD_FUNCPTR(gnutls_global_deinit)
    LOAD_FUNCPTR(gnutls_global_init)
    LOAD_FUNCPTR(gnutls_global_set_log_function)
    LOAD_FUNCPTR(gnutls_global_set_log_level)
    LOAD_FUNCPTR(gnutls_perror)
    LOAD_FUNCPTR(gnutls_pkcs12_deinit)
    LOAD_FUNCPTR(gnutls_pkcs12_import)
    LOAD_FUNCPTR(gnutls_pkcs12_init)
    LOAD_FUNCPTR(gnutls_pkcs12_simple_parse)
    LOAD_FUNCPTR(gnutls_x509_crt_export)
    LOAD_FUNCPTR(gnutls_x509_privkey_export_rsa_raw2)
    LOAD_FUNCPTR(gnutls_x509_privkey_get_pk_algorithm2)
#undef LOAD_FUNCPTR

    if ((ret = pgnutls_global_init()) != GNUTLS_E_SUCCESS)
    {
        pgnutls_perror( ret );
        goto fail;
    }

    if (TRACE_ON( crypt ))
    {
        char *env = getenv("GNUTLS_DEBUG_LEVEL");
        int level = env ? atoi(env) : 4;
        pgnutls_global_set_log_level(level);
        pgnutls_global_set_log_function( gnutls_log );
    }

    return TRUE;

fail:
    dlclose( libgnutls_handle );
    libgnutls_handle = NULL;
    return STATUS_DLL_INIT_FAILED;
}

static NTSTATUS process_detach( void *args )
{
    pgnutls_global_deinit();
    dlclose( libgnutls_handle );
    libgnutls_handle = NULL;
    return STATUS_SUCCESS;
}
#define RSA_MAGIC_KEY  ('R' | ('S' << 8) | ('A' << 16) | ('2' << 24))
#define RSA_PUBEXP     65537

struct cert_store_data
{
    gnutls_pkcs12_t p12;
    gnutls_x509_privkey_t key;
    gnutls_x509_crt_t *chain;
    unsigned int key_bitlen;
    unsigned int chain_len;
};

static struct cert_store_data *get_store_data( cert_store_data_t data )
{
    return (struct cert_store_data *)(ULONG_PTR)data;
}

static NTSTATUS import_store_key( void *args )
{
    struct import_store_key_params *params = args;
    struct cert_store_data *data = get_store_data( params->data );
    int i, ret;
    unsigned int bitlen = data->key_bitlen;
    gnutls_datum_t m, e, d, p, q, u, e1, e2;
    BLOBHEADER *hdr;
    RSAPUBKEY *rsakey;
    BYTE *src, *dst;
    DWORD size;

    size = sizeof(*hdr) + sizeof(*rsakey) + (bitlen * 9 / 16);
    if (!params->buf || *params->buf_size < size)
    {
        *params->buf_size = size;
        return STATUS_BUFFER_TOO_SMALL;
    }

    if ((ret = pgnutls_x509_privkey_export_rsa_raw2( data->key, &m, &e, &d, &p, &q, &u, &e1, &e2 )) < 0)
    {
        pgnutls_perror( ret );
        return STATUS_INVALID_PARAMETER;
    }

    hdr = params->buf;
    hdr->bType    = PRIVATEKEYBLOB;
    hdr->bVersion = CUR_BLOB_VERSION;
    hdr->reserved = 0;
    hdr->aiKeyAlg = CALG_RSA_KEYX;

    rsakey = (RSAPUBKEY *)(hdr + 1);
    rsakey->magic  = RSA_MAGIC_KEY;
    rsakey->bitlen = bitlen;
    rsakey->pubexp = RSA_PUBEXP;

    dst = (BYTE *)(rsakey + 1);
    if (m.size == bitlen / 8 + 1 && !m.data[0]) src = m.data + 1;
    else if (m.size != bitlen / 8) goto done;
    else src = m.data;
    for (i = bitlen / 8 - 1; i >= 0; i--) *dst++ = src[i];

    if (p.size == bitlen / 16 + 1 && !p.data[0]) src = p.data + 1;
    else if (p.size != bitlen / 16) goto done;
    else src = p.data;
    for (i = bitlen / 16 - 1; i >= 0; i--) *dst++ = src[i];

    if (q.size == bitlen / 16 + 1 && !q.data[0]) src = q.data + 1;
    else if (q.size != bitlen / 16) goto done;
    else src = q.data;
    for (i = bitlen / 16 - 1; i >= 0; i--) *dst++ = src[i];

    if (e1.size == bitlen / 16 + 1 && !e1.data[0]) src = e1.data + 1;
    else if (e1.size != bitlen / 16) goto done;
    else src = e1.data;
    for (i = bitlen / 16 - 1; i >= 0; i--) *dst++ = src[i];

    if (e2.size == bitlen / 16 + 1 && !e2.data[0]) src = e2.data + 1;
    else if (e2.size != bitlen / 16) goto done;
    else src = e2.data;
    for (i = bitlen / 16 - 1; i >= 0; i--) *dst++ = src[i];

    if (u.size == bitlen / 16 + 1 && !u.data[0]) src = u.data + 1;
    else if (u.size != bitlen / 16) goto done;
    else src = u.data;
    for (i = bitlen / 16 - 1; i >= 0; i--) *dst++ = src[i];

    if (d.size == bitlen / 8 + 1 && !d.data[0]) src = d.data + 1;
    else if (d.size != bitlen / 8) goto done;
    else src = d.data;
    for (i = bitlen / 8 - 1; i >= 0; i--) *dst++ = src[i];

done:
    free( m.data );
    free( e.data );
    free( d.data );
    free( p.data );
    free( q.data );
    free( u.data );
    free( e1.data );
    free( e2.data );
    return STATUS_SUCCESS;
}

static char *password_to_ascii( const WCHAR *str )
{
    char *ret;
    unsigned int i = 0;

    if (!(ret = malloc( (lstrlenW(str) + 1) * sizeof(*ret) ))) return NULL;
    while (*str)
    {
        if (*str > 0x7f) WARN( "password contains non-ascii characters\n" );
        ret[i++] = *str++;
    }
    ret[i] = 0;
    return ret;
}

static NTSTATUS open_cert_store( void *args )
{
    struct open_cert_store_params *params = args;
    gnutls_pkcs12_t p12;
    gnutls_datum_t pfx_data;
    gnutls_x509_privkey_t key;
    gnutls_x509_crt_t *chain;
    unsigned int chain_len;
    unsigned int bitlen;
    char *pwd = NULL;
    int ret;
    struct cert_store_data *store_data;

    if (!libgnutls_handle) return STATUS_DLL_NOT_FOUND;
    if (params->password && !(pwd = password_to_ascii( params->password ))) return STATUS_NO_MEMORY;

    if ((ret = pgnutls_pkcs12_init( &p12 )) < 0) goto error;

    pfx_data.data = params->pfx->pbData;
    pfx_data.size = params->pfx->cbData;
    if ((ret = pgnutls_pkcs12_import( p12, &pfx_data, GNUTLS_X509_FMT_DER, 0 )) < 0) goto error;

    if ((ret = pgnutls_pkcs12_simple_parse( p12, pwd ? pwd : "", &key, &chain, &chain_len, NULL, NULL, NULL, 0 )) < 0)
        goto error;

    if ((ret = pgnutls_x509_privkey_get_pk_algorithm2( key, &bitlen )) < 0)
        goto error;

    free( pwd );

    if (ret != GNUTLS_PK_RSA)
    {
        FIXME( "key algorithm %u not supported\n", ret );
        pgnutls_pkcs12_deinit( p12 );
        return STATUS_INVALID_PARAMETER;
    }

    store_data = malloc( sizeof(*store_data) );
    store_data->p12 = p12;
    store_data->key = key;
    store_data->chain = chain;
    store_data->key_bitlen = bitlen;
    store_data->chain_len = chain_len;
    *params->data_ret = (ULONG_PTR)store_data;
    return STATUS_SUCCESS;

error:
    pgnutls_perror( ret );
    pgnutls_pkcs12_deinit( p12 );
    free( pwd );
    return STATUS_INVALID_PARAMETER;
}

static NTSTATUS import_store_cert( void *args )
{
    struct import_store_cert_params *params = args;
    struct cert_store_data *data = get_store_data( params->data );
    size_t size = 0;
    int ret;

    if (params->index >= data->chain_len) return STATUS_NO_MORE_ENTRIES;

    if ((ret = pgnutls_x509_crt_export( data->chain[params->index], GNUTLS_X509_FMT_DER, NULL, &size )) != GNUTLS_E_SHORT_MEMORY_BUFFER)
        return STATUS_INVALID_PARAMETER;

    if (!params->buf || *params->buf_size < size)
    {
        *params->buf_size = size;
        return STATUS_BUFFER_TOO_SMALL;
    }
    if ((ret = pgnutls_x509_crt_export( data->chain[params->index], GNUTLS_X509_FMT_DER, params->buf, &size )) < 0)
        return STATUS_INVALID_PARAMETER;

    return STATUS_SUCCESS;
}

static NTSTATUS close_cert_store( void *args )
{
    struct close_cert_store_params *params = args;
    struct cert_store_data *data = get_store_data( params->data );

    if (params->data)
    {
        pgnutls_pkcs12_deinit( data->p12 );
        free( data );
    }
    return STATUS_SUCCESS;
}

#else /* SONAME_LIBGNUTLS */

static NTSTATUS process_attach( void *args ) { return STATUS_SUCCESS; }
static NTSTATUS process_detach( void *args ) { return STATUS_SUCCESS; }
static NTSTATUS open_cert_store( void *args ) { return STATUS_DLL_NOT_FOUND; }
static NTSTATUS import_store_key( void *args ) { return STATUS_DLL_NOT_FOUND; }
static NTSTATUS import_store_cert( void *args ) { return STATUS_DLL_NOT_FOUND; }
static NTSTATUS close_cert_store( void *args ) { return STATUS_DLL_NOT_FOUND; }

#endif /* SONAME_LIBGNUTLS */

struct root_cert
{
    struct list entry;
    SIZE_T      size;
    BYTE        data[1];
};

static struct list root_cert_list = LIST_INIT(root_cert_list);

static BYTE *add_cert( SIZE_T size )
{
    struct root_cert *cert = malloc( offsetof( struct root_cert, data[size] ));

    if (!cert) return NULL;
    cert->size = size;
    list_add_tail( &root_cert_list, &cert->entry );
    return cert->data;
}

struct DynamicBuffer
{
    DWORD allocated;
    DWORD used;
    char *data;
};

static inline void reset_buffer(struct DynamicBuffer *buffer)
{
    buffer->used = 0;
    if (buffer->data) buffer->data[0] = 0;
}

static void add_line_to_buffer(struct DynamicBuffer *buffer, LPCSTR line)
{
    if (buffer->used + strlen(line) + 1 > buffer->allocated)
    {
        DWORD new_size = max( max( buffer->allocated * 2, 1024 ), buffer->used + strlen(line) + 1 );
        void *ptr = realloc( buffer->data, new_size );
        if (!ptr) return;
        buffer->data = ptr;
        buffer->allocated = new_size;
        if (!buffer->used) buffer->data[0] = 0;
    }
    strcpy( buffer->data + buffer->used, line );
    buffer->used += strlen(line);
}

#define BASE64_DECODE_PADDING    0x100
#define BASE64_DECODE_WHITESPACE 0x200
#define BASE64_DECODE_INVALID    0x300

static inline int decodeBase64Byte(char c)
{
    int ret = BASE64_DECODE_INVALID;

    if (c >= 'A' && c <= 'Z')
        ret = c - 'A';
    else if (c >= 'a' && c <= 'z')
        ret = c - 'a' + 26;
    else if (c >= '0' && c <= '9')
        ret = c - '0' + 52;
    else if (c == '+')
        ret = 62;
    else if (c == '/')
        ret = 63;
    else if (c == '=')
        ret = BASE64_DECODE_PADDING;
    else if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
        ret = BASE64_DECODE_WHITESPACE;
    return ret;
}

static BOOL base64_to_cert( const char *str )
{
    DWORD i, valid, out, hasPadding;
    BYTE block[4], *data;

    for (i = valid = out = hasPadding = 0; str[i]; i++)
    {
        int d = decodeBase64Byte( str[i] );
        if (d == BASE64_DECODE_INVALID) return FALSE;
        if (d == BASE64_DECODE_WHITESPACE) continue;

        /* When padding starts, data is not acceptable */
        if (hasPadding && d != BASE64_DECODE_PADDING) return FALSE;

        /* Padding after a full block (like "VVVV=") is ok and stops decoding */
        if (d == BASE64_DECODE_PADDING && (valid & 3) == 0) break;

        valid++;
        if (d == BASE64_DECODE_PADDING)
        {
            hasPadding = 1;
            /* When padding reaches a full block, stop decoding */
            if ((valid & 3) == 0) break;
            continue;
        }

        /* out is incremented in the 4-char block as follows: "1-23" */
        if ((valid & 3) != 2) out++;
    }
    /* Fail if the block has bad padding; omitting padding is fine */
    if ((valid & 3) != 0 && hasPadding) return FALSE;

    if (!(data = add_cert( out ))) return FALSE;
    for (i = valid = out = 0; str[i]; i++)
    {
        int d = decodeBase64Byte( str[i] );
        if (d == BASE64_DECODE_WHITESPACE) continue;
        if (d == BASE64_DECODE_PADDING) break;
        block[valid & 3] = d;
        valid += 1;
        switch (valid & 3)
        {
        case 1:
            data[out++] = (block[0] << 2);
            break;
        case 2:
            data[out-1] = (block[0] << 2) | (block[1] >> 4);
            break;
        case 3:
            data[out++] = (block[1] << 4) | (block[2] >> 2);
            break;
        case 0:
            data[out++] = (block[2] << 6) | (block[3] >> 0);
            break;
        }
    }
    return TRUE;
}

/* Reads the file fd, and imports any certificates in it into store. */
static void import_certs_from_file( int fd )
{
    FILE *fp = fdopen(fd, "r");
    char line[1024];
    BOOL in_cert = FALSE;
    struct DynamicBuffer saved_cert = { 0, 0, NULL };
    int num_certs = 0;

    if (!fp) return;
    TRACE("\n");
    while (fgets(line, sizeof(line), fp))
    {
        static const char header[] = "-----BEGIN CERTIFICATE-----";
        static const char trailer[] = "-----END CERTIFICATE-----";

        if (!strncmp(line, header, strlen(header)))
        {
            TRACE("begin new certificate\n");
            in_cert = TRUE;
            reset_buffer(&saved_cert);
        }
        else if (!strncmp(line, trailer, strlen(trailer)))
        {
            TRACE("end of certificate, adding cert\n");
            in_cert = FALSE;
            if (base64_to_cert( saved_cert.data )) num_certs++;
        }
        else if (in_cert) add_line_to_buffer(&saved_cert, line);
    }
    free( saved_cert.data );
    TRACE("Read %d certs\n", num_certs);
    fclose(fp);
}

static void import_certs_from_path(LPCSTR path, BOOL allow_dir);

static BOOL check_buffer_resize(char **ptr_buf, size_t *buf_size, size_t check_size)
{
    if (check_size > *buf_size)
    {
        void *ptr = realloc(*ptr_buf, check_size);

        if (!ptr) return FALSE;
        *buf_size = check_size;
        *ptr_buf = ptr;
    }
    return TRUE;
}

/* Opens path, which must be a directory, and imports certificates from every
 * file in the directory into store.
 * Returns TRUE if any certificates were successfully imported.
 */
static void import_certs_from_dir( LPCSTR path )
{
    DIR *dir;

    dir = opendir(path);
    if (dir)
    {
        size_t path_len = strlen(path), bufsize = 0;
        char *filebuf = NULL;

        struct dirent *entry;
        while ((entry = readdir(dir)))
        {
            if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
            {
                size_t name_len = strlen(entry->d_name);

                if (!check_buffer_resize(&filebuf, &bufsize, path_len + 1 + name_len + 1)) break;
                snprintf(filebuf, bufsize, "%s/%s", path, entry->d_name);
                import_certs_from_path(filebuf, FALSE);
            }
        }
        free(filebuf);
        closedir(dir);
    }
}

/* Opens path, which may be a file or a directory, and imports any certificates
 * it finds into store.
 * Returns TRUE if any certificates were successfully imported.
 */
static void import_certs_from_path(LPCSTR path, BOOL allow_dir)
{
    int fd;

    TRACE("(%s, %d)\n", debugstr_a(path), allow_dir);

    fd = open(path, O_RDONLY);
    if (fd != -1)
    {
        struct stat st;

        if (fstat(fd, &st) == 0)
        {
            if (S_ISREG(st.st_mode))
                import_certs_from_file(fd);
            else if (S_ISDIR(st.st_mode))
            {
                if (allow_dir)
                    import_certs_from_dir(path);
                else
                    WARN("%s is a directory and directories are disallowed\n",
                     debugstr_a(path));
            }
            else
                ERR("%s: invalid file type\n", path);
        }
        close(fd);
    }
}

static const char * const CRYPT_knownLocations[] = {
 "/etc/ssl/certs/ca-certificates.crt",
 "/etc/ssl/certs",
 "/etc/pki/tls/certs/ca-bundle.crt",
 "/usr/share/ca-certificates/ca-bundle.crt",
 "/usr/local/share/certs/",
 "/etc/sfw/openssl/certs",
 "/etc/security/cacerts",  /* Android */
};

static void load_root_certs(void)
{
    unsigned int i;

#ifdef __APPLE__
    const SecTrustSettingsDomain domains[] = {
        kSecTrustSettingsDomainSystem,
        kSecTrustSettingsDomainAdmin,
        kSecTrustSettingsDomainUser
    };
    OSStatus status;
    CFArrayRef certs;
    DWORD domain;

    for (domain = 0; domain < ARRAY_SIZE(domains); domain++)
    {
        status = SecTrustSettingsCopyCertificates(domains[domain], &certs);
        if (status == noErr)
        {
            for (i = 0; i < CFArrayGetCount(certs); i++)
            {
                SecCertificateRef cert = (SecCertificateRef)CFArrayGetValueAtIndex(certs, i);
                CFDataRef certData;
                if ((status = SecItemExport(cert, kSecFormatX509Cert, 0, NULL, &certData)) == noErr)
                {
                    BYTE *data = add_cert( CFDataGetLength(certData) );
                    if (data) memcpy( data, CFDataGetBytePtr(certData), CFDataGetLength(certData) );
                    CFRelease(certData);
                }
                else
                    WARN("could not export certificate %u to X509 format: 0x%08x\n", i, (unsigned int)status);
            }
            CFRelease(certs);
        }
    }
#endif

    for (i = 0; i < ARRAY_SIZE(CRYPT_knownLocations) && list_empty(&root_cert_list); i++)
        import_certs_from_path( CRYPT_knownLocations[i], TRUE );
}

static NTSTATUS enum_root_certs( void *args )
{
    struct enum_root_certs_params *params = args;
    static BOOL loaded;
    struct list *ptr;
    struct root_cert *cert;

    if (!loaded) load_root_certs();
    loaded = TRUE;

    if (!(ptr = list_head( &root_cert_list ))) return STATUS_NO_MORE_ENTRIES;
    cert = LIST_ENTRY( ptr, struct root_cert, entry );
    *params->needed = cert->size;
    if (cert->size <= params->size)
    {
        memcpy( params->buffer, cert->data, cert->size );
        list_remove( &cert->entry );
        free( cert );
    }
    return STATUS_SUCCESS;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    process_attach,
    process_detach,
    open_cert_store,
    import_store_key,
    import_store_cert,
    close_cert_store,
    enum_root_certs,
};

C_ASSERT( ARRAYSIZE(__wine_unix_call_funcs) == unix_funcs_count );

#ifdef _WIN64

typedef ULONG PTR32;

typedef struct
{
    DWORD cbData;
    PTR32 pbData;
} CRYPT_DATA_BLOB32;

static NTSTATUS wow64_open_cert_store( void *args )
{
    struct
    {
        PTR32 pfx;
        PTR32 password;
        PTR32 data_ret;
    } const *params32 = args;

    const CRYPT_DATA_BLOB32 *pfx32 = ULongToPtr( params32->pfx );
    CRYPT_DATA_BLOB pfx = { pfx32->cbData, ULongToPtr( pfx32->pbData ) };
    struct open_cert_store_params params =
    {
        &pfx,
        ULongToPtr( params32->password ),
        ULongToPtr( params32->data_ret )
    };

    return open_cert_store( &params );
}

static NTSTATUS wow64_import_store_key( void *args )
{
    struct
    {
        cert_store_data_t data;
        PTR32 buf;
        PTR32 buf_size;
    } const *params32 = args;

    struct import_store_key_params params =
    {
        params32->data,
        ULongToPtr( params32->buf ),
        ULongToPtr( params32->buf_size )
    };

    return import_store_key( &params );
}

static NTSTATUS wow64_import_store_cert( void *args )
{
    struct
    {
        cert_store_data_t data;
        unsigned int index;
        PTR32 buf;
        PTR32 buf_size;
    } const *params32 = args;

    struct import_store_cert_params params =
    {
        params32->data,
        params32->index,
        ULongToPtr( params32->buf ),
        ULongToPtr( params32->buf_size )
    };

    return import_store_cert( &params );
}

static NTSTATUS wow64_enum_root_certs( void *args )
{
    struct
    {
        PTR32  buffer;
        DWORD  size;
        PTR32  needed;
    } const *params32 = args;

    struct enum_root_certs_params params =
    {
        ULongToPtr( params32->buffer ),
        params32->size,
        ULongToPtr( params32->needed )
    };

    return enum_root_certs( &params );
}

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    process_attach,
    process_detach,
    wow64_open_cert_store,
    wow64_import_store_key,
    wow64_import_store_cert,
    close_cert_store,
    wow64_enum_root_certs,
};

C_ASSERT( ARRAYSIZE(__wine_unix_call_wow64_funcs) == unix_funcs_count );

#endif  /* _WIN64 */
