/* Copyright (C) 2005 Juan Lang
 * Copyright 2008 Henri Verbeet
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
 *
 * This file implements the schannel provider, or, the SSL/TLS implementations.
 */

#include "precomp.h"

#include <wine/config.h>

#if defined(SONAME_LIBGNUTLS) || defined (HAVE_SECURITY_SECURITY_H) || defined (SONAME_LIBMBEDTLS)

#define SCHAN_INVALID_HANDLE ~0UL

enum schan_handle_type
{
    SCHAN_HANDLE_CRED,
    SCHAN_HANDLE_CTX,
    SCHAN_HANDLE_FREE
};

struct schan_handle
{
    void *object;
    enum schan_handle_type type;
};

struct schan_context
{
    schan_imp_session session;
    ULONG req_ctx_attr;
    const CERT_CONTEXT *cert;
};

static struct schan_handle *schan_handle_table;
static struct schan_handle *schan_free_handles;
static SIZE_T schan_handle_table_size;
static SIZE_T schan_handle_count;

/* Protocols enabled, only those may be used for the connection. */
static DWORD config_enabled_protocols;

/* Protocols disabled by default. They are enabled for using, but disabled when caller asks for default settings. */
static DWORD config_default_disabled_protocols;

static ULONG_PTR schan_alloc_handle(void *object, enum schan_handle_type type)
{
    struct schan_handle *handle;

    if (schan_free_handles)
    {
        DWORD index = schan_free_handles - schan_handle_table;
        /* Use a free handle */
        handle = schan_free_handles;
        if (handle->type != SCHAN_HANDLE_FREE)
        {
            ERR("Handle %d(%p) is in the free list, but has type %#x.\n", index, handle, handle->type);
            return SCHAN_INVALID_HANDLE;
        }
        schan_free_handles = handle->object;
        handle->object = object;
        handle->type = type;

        return index;
    }
    if (!(schan_handle_count < schan_handle_table_size))
    {
        /* Grow the table */
        SIZE_T new_size = schan_handle_table_size + (schan_handle_table_size >> 1);
        struct schan_handle *new_table = HeapReAlloc(GetProcessHeap(), 0, schan_handle_table, new_size * sizeof(*schan_handle_table));
        if (!new_table)
        {
            ERR("Failed to grow the handle table\n");
            return SCHAN_INVALID_HANDLE;
        }
        schan_handle_table = new_table;
        schan_handle_table_size = new_size;
    }

    handle = &schan_handle_table[schan_handle_count++];
    handle->object = object;
    handle->type = type;

    return handle - schan_handle_table;
}

static void *schan_free_handle(ULONG_PTR handle_idx, enum schan_handle_type type)
{
    struct schan_handle *handle;
    void *object;

    if (handle_idx == SCHAN_INVALID_HANDLE) return NULL;
    if (handle_idx >= schan_handle_count) return NULL;
    handle = &schan_handle_table[handle_idx];
    if (handle->type != type)
    {
        ERR("Handle %ld(%p) is not of type %#x\n", handle_idx, handle, type);
        return NULL;
    }

    object = handle->object;
    handle->object = schan_free_handles;
    handle->type = SCHAN_HANDLE_FREE;
    schan_free_handles = handle;

    return object;
}

static void *schan_get_object(ULONG_PTR handle_idx, enum schan_handle_type type)
{
    struct schan_handle *handle;

    if (handle_idx == SCHAN_INVALID_HANDLE) return NULL;
    if (handle_idx >= schan_handle_count) return NULL;
    handle = &schan_handle_table[handle_idx];
    if (handle->type != type)
    {
        ERR("Handle %ld(%p) is not of type %#x\n", handle_idx, handle, type);
        return NULL;
    }

    return handle->object;
}

static void read_config(void)
{
    DWORD enabled = 0, default_disabled = 0;
    HKEY protocols_key, key;
    WCHAR subkey_name[64];
    unsigned i;
    DWORD res;

    static BOOL config_read = FALSE;

    static const WCHAR protocol_config_key_name[] = {
        'S','Y','S','T','E','M','\\',
        'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
        'C','o','n','t','r','o','l','\\',
        'S','e','c','u','r','i','t','y','P','r','o','v','i','d','e','r','s','\\',
        'S','C','H','A','N','N','E','L','\\',
        'P','r','o','t','o','c','o','l','s',0 };

    static const WCHAR clientW[] = {'\\','C','l','i','e','n','t',0};
    static const WCHAR enabledW[] = {'e','n','a','b','l','e','d',0};
    static const WCHAR disabledbydefaultW[] = {'D','i','s','a','b','l','e','d','B','y','D','e','f','a','u','l','t',0};

    static const struct {
        WCHAR key_name[20];
        DWORD prot_client_flag;
        BOOL enabled; /* If no config is present, enable the protocol */
        BOOL disabled_by_default; /* Disable if caller asks for default protocol set */
    } protocol_config_keys[] = {
        {{'S','S','L',' ','2','.','0',0}, SP_PROT_SSL2_CLIENT, FALSE, TRUE}, /* NOTE: TRUE, TRUE on Windows */
        {{'S','S','L',' ','3','.','0',0}, SP_PROT_SSL3_CLIENT, TRUE, FALSE},
        {{'T','L','S',' ','1','.','0',0}, SP_PROT_TLS1_0_CLIENT, TRUE, FALSE},
        {{'T','L','S',' ','1','.','1',0}, SP_PROT_TLS1_1_CLIENT, TRUE, FALSE /* NOTE: not enabled by default on Windows */ },
        {{'T','L','S',' ','1','.','2',0}, SP_PROT_TLS1_2_CLIENT, TRUE, FALSE /* NOTE: not enabled by default on Windows */ }
    };

    /* No need for thread safety */
    if(config_read)
        return;

    res = RegOpenKeyExW(HKEY_LOCAL_MACHINE, protocol_config_key_name, 0, KEY_READ, &protocols_key);
    if(res == ERROR_SUCCESS) {
        DWORD type, size, value;

        for(i=0; i < sizeof(protocol_config_keys)/sizeof(*protocol_config_keys); i++) {
            strcpyW(subkey_name, protocol_config_keys[i].key_name);
            strcatW(subkey_name, clientW);
            res = RegOpenKeyExW(protocols_key, subkey_name, 0, KEY_READ, &key);
            if(res != ERROR_SUCCESS) {
                if(protocol_config_keys[i].enabled)
                    enabled |= protocol_config_keys[i].prot_client_flag;
                if(protocol_config_keys[i].disabled_by_default)
                    default_disabled |= protocol_config_keys[i].prot_client_flag;
                continue;
            }

            size = sizeof(value);
            res = RegQueryValueExW(key, enabledW, NULL, &type, (BYTE*)&value, &size);
            if(res == ERROR_SUCCESS) {
                if(type == REG_DWORD && value)
                    enabled |= protocol_config_keys[i].prot_client_flag;
            }else if(protocol_config_keys[i].enabled) {
                enabled |= protocol_config_keys[i].prot_client_flag;
            }

            size = sizeof(value);
            res = RegQueryValueExW(key, disabledbydefaultW, NULL, &type, (BYTE*)&value, &size);
            if(res == ERROR_SUCCESS) {
                if(type != REG_DWORD || value)
                    default_disabled |= protocol_config_keys[i].prot_client_flag;
            }else if(protocol_config_keys[i].disabled_by_default) {
                default_disabled |= protocol_config_keys[i].prot_client_flag;
            }

            RegCloseKey(key);
        }
    }else {
        /* No config, enable all known protocols. */
        for(i=0; i < sizeof(protocol_config_keys)/sizeof(*protocol_config_keys); i++) {
            if(protocol_config_keys[i].enabled)
                enabled |= protocol_config_keys[i].prot_client_flag;
            if(protocol_config_keys[i].disabled_by_default)
                default_disabled |= protocol_config_keys[i].prot_client_flag;
        }
    }

    RegCloseKey(protocols_key);

    config_enabled_protocols = enabled & schan_imp_enabled_protocols();
    config_default_disabled_protocols = default_disabled;
    config_read = TRUE;

    TRACE("enabled %x, disabled by default %x\n", config_enabled_protocols, config_default_disabled_protocols);
}

static SECURITY_STATUS schan_QueryCredentialsAttributes(
 PCredHandle phCredential, ULONG ulAttribute, VOID *pBuffer)
{
    struct schan_credentials *cred;
    SECURITY_STATUS ret;

    cred = schan_get_object(phCredential->dwLower, SCHAN_HANDLE_CRED);
    if(!cred)
        return SEC_E_INVALID_HANDLE;

    switch (ulAttribute)
    {
    case SECPKG_ATTR_SUPPORTED_ALGS:
        if (pBuffer)
        {
            /* FIXME: get from CryptoAPI */
            FIXME("SECPKG_ATTR_SUPPORTED_ALGS: stub\n");
            ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INTERNAL_ERROR;
        break;
    case SECPKG_ATTR_CIPHER_STRENGTHS:
        if (pBuffer)
        {
            SecPkgCred_CipherStrengths *r = pBuffer;

            /* FIXME: get from CryptoAPI */
            FIXME("SECPKG_ATTR_CIPHER_STRENGTHS: semi-stub\n");
            r->dwMinimumCipherStrength = 40;
            r->dwMaximumCipherStrength = 168;
            ret = SEC_E_OK;
        }
        else
            ret = SEC_E_INTERNAL_ERROR;
        break;
    case SECPKG_ATTR_SUPPORTED_PROTOCOLS:
        if(pBuffer) {
            /* Regardless of MSDN documentation, tests show that this attribute takes into account
             * what protocols are enabled for given credential. */
            ((SecPkgCred_SupportedProtocols*)pBuffer)->grbitProtocol = cred->enabled_protocols;
            ret = SEC_E_OK;
        }else {
            ret = SEC_E_INTERNAL_ERROR;
        }
        break;
    default:
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    return ret;
}

static SECURITY_STATUS SEC_ENTRY schan_QueryCredentialsAttributesA(
 PCredHandle phCredential, ULONG ulAttribute, PVOID pBuffer)
{
    SECURITY_STATUS ret;

    TRACE("(%p, %d, %p)\n", phCredential, ulAttribute, pBuffer);

    switch (ulAttribute)
    {
    case SECPKG_CRED_ATTR_NAMES:
        FIXME("SECPKG_CRED_ATTR_NAMES: stub\n");
        ret = SEC_E_UNSUPPORTED_FUNCTION;
        break;
    default:
        ret = schan_QueryCredentialsAttributes(phCredential, ulAttribute,
         pBuffer);
    }
    return ret;
}

SECURITY_STATUS SEC_ENTRY schan_QueryCredentialsAttributesW(
 PCredHandle phCredential, ULONG ulAttribute, PVOID pBuffer)
{
    SECURITY_STATUS ret;

    TRACE("(%p, %d, %p)\n", phCredential, ulAttribute, pBuffer);

    switch (ulAttribute)
    {
    case SECPKG_CRED_ATTR_NAMES:
        FIXME("SECPKG_CRED_ATTR_NAMES: stub\n");
        ret = SEC_E_UNSUPPORTED_FUNCTION;
        break;
    default:
        ret = schan_QueryCredentialsAttributes(phCredential, ulAttribute,
         pBuffer);
    }
    return ret;
}

static SECURITY_STATUS schan_CheckCreds(const SCHANNEL_CRED *schanCred)
{
    SECURITY_STATUS st;
    DWORD i;

    TRACE("dwVersion = %d\n", schanCred->dwVersion);
    TRACE("cCreds = %d\n", schanCred->cCreds);
    TRACE("hRootStore = %p\n", schanCred->hRootStore);
    TRACE("cMappers = %d\n", schanCred->cMappers);
    TRACE("cSupportedAlgs = %d:\n", schanCred->cSupportedAlgs);
    for (i = 0; i < schanCred->cSupportedAlgs; i++)
        TRACE("%08x\n", schanCred->palgSupportedAlgs[i]);
    TRACE("grbitEnabledProtocols = %08x\n", schanCred->grbitEnabledProtocols);
    TRACE("dwMinimumCipherStrength = %d\n", schanCred->dwMinimumCipherStrength);
    TRACE("dwMaximumCipherStrength = %d\n", schanCred->dwMaximumCipherStrength);
    TRACE("dwSessionLifespan = %d\n", schanCred->dwSessionLifespan);
    TRACE("dwFlags = %08x\n", schanCred->dwFlags);
    TRACE("dwCredFormat = %d\n", schanCred->dwCredFormat);

    switch (schanCred->dwVersion)
    {
    case SCH_CRED_V3:
    case SCHANNEL_CRED_VERSION:
        break;
    default:
        return SEC_E_INTERNAL_ERROR;
    }

    if (schanCred->cCreds == 0)
        st = SEC_E_NO_CREDENTIALS;
    else if (schanCred->cCreds > 1)
        st = SEC_E_UNKNOWN_CREDENTIALS;
    else
    {
        DWORD keySpec;
        HCRYPTPROV csp;
        BOOL ret, freeCSP;

        ret = CryptAcquireCertificatePrivateKey(schanCred->paCred[0],
         0, /* FIXME: what flags to use? */ NULL,
         &csp, &keySpec, &freeCSP);
        if (ret)
        {
            st = SEC_E_OK;
            if (freeCSP)
                CryptReleaseContext(csp, 0);
        }
        else
            st = SEC_E_UNKNOWN_CREDENTIALS;
    }
    return st;
}

static SECURITY_STATUS schan_AcquireClientCredentials(const SCHANNEL_CRED *schanCred,
 PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    struct schan_credentials *creds;
    unsigned enabled_protocols;
    ULONG_PTR handle;
    SECURITY_STATUS st = SEC_E_OK;

    TRACE("schanCred %p, phCredential %p, ptsExpiry %p\n", schanCred, phCredential, ptsExpiry);

    if (schanCred)
    {
        st = schan_CheckCreds(schanCred);
        if (st != SEC_E_OK && st != SEC_E_NO_CREDENTIALS)
            return st;

        st = SEC_E_OK;
    }

    read_config();
    if(schanCred && schanCred->grbitEnabledProtocols)
        enabled_protocols = schanCred->grbitEnabledProtocols & config_enabled_protocols;
    else
        enabled_protocols = config_enabled_protocols & ~config_default_disabled_protocols;
    if(!enabled_protocols) {
        ERR("Could not find matching protocol\n");
        return SEC_E_NO_AUTHENTICATING_AUTHORITY;
    }

    /* For now, the only thing I'm interested in is the direction of the
     * connection, so just store it.
     */
    creds = HeapAlloc(GetProcessHeap(), 0, sizeof(*creds));
    if (!creds) return SEC_E_INSUFFICIENT_MEMORY;

    handle = schan_alloc_handle(creds, SCHAN_HANDLE_CRED);
    if (handle == SCHAN_INVALID_HANDLE) goto fail;

    creds->credential_use = SECPKG_CRED_OUTBOUND;
    if (!schan_imp_allocate_certificate_credentials(creds))
    {
        schan_free_handle(handle, SCHAN_HANDLE_CRED);
        goto fail;
    }

    creds->enabled_protocols = enabled_protocols;
    phCredential->dwLower = handle;
    phCredential->dwUpper = 0;

    /* Outbound credentials have no expiry */
    if (ptsExpiry)
    {
        ptsExpiry->LowPart = 0;
        ptsExpiry->HighPart = 0;
    }

    return st;

fail:
    HeapFree(GetProcessHeap(), 0, creds);
    return SEC_E_INTERNAL_ERROR;
}

static SECURITY_STATUS schan_AcquireServerCredentials(const SCHANNEL_CRED *schanCred,
 PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS st;

    TRACE("schanCred %p, phCredential %p, ptsExpiry %p\n", schanCred, phCredential, ptsExpiry);

    if (!schanCred) return SEC_E_NO_CREDENTIALS;

    st = schan_CheckCreds(schanCred);
    if (st == SEC_E_OK)
    {
        ULONG_PTR handle;
        struct schan_credentials *creds;

        creds = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*creds));
        if (!creds) return SEC_E_INSUFFICIENT_MEMORY;
        creds->credential_use = SECPKG_CRED_INBOUND;

        handle = schan_alloc_handle(creds, SCHAN_HANDLE_CRED);
        if (handle == SCHAN_INVALID_HANDLE)
        {
            HeapFree(GetProcessHeap(), 0, creds);
            return SEC_E_INTERNAL_ERROR;
        }

        phCredential->dwLower = handle;
        phCredential->dwUpper = 0;

        /* FIXME: get expiry from cert */
    }
    return st;
}

static SECURITY_STATUS schan_AcquireCredentialsHandle(ULONG fCredentialUse,
 const SCHANNEL_CRED *schanCred, PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;

    if (fCredentialUse == SECPKG_CRED_OUTBOUND)
        ret = schan_AcquireClientCredentials(schanCred, phCredential,
         ptsExpiry);
    else
        ret = schan_AcquireServerCredentials(schanCred, phCredential,
         ptsExpiry);
    return ret;
}

SECURITY_STATUS SEC_ENTRY schan_AcquireCredentialsHandleA(
 SEC_CHAR *pszPrincipal, SEC_CHAR *pszPackage, ULONG fCredentialUse,
 PLUID pLogonID, PVOID pAuthData, SEC_GET_KEY_FN pGetKeyFn,
 PVOID pGetKeyArgument, PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    TRACE("(%s, %s, 0x%08x, %p, %p, %p, %p, %p, %p)\n",
     debugstr_a(pszPrincipal), debugstr_a(pszPackage), fCredentialUse,
     pLogonID, pAuthData, pGetKeyFn, pGetKeyArgument, phCredential, ptsExpiry);
    return schan_AcquireCredentialsHandle(fCredentialUse,
     pAuthData, phCredential, ptsExpiry);
}

SECURITY_STATUS SEC_ENTRY schan_AcquireCredentialsHandleW(
 SEC_WCHAR *pszPrincipal, SEC_WCHAR *pszPackage, ULONG fCredentialUse,
 PLUID pLogonID, PVOID pAuthData, SEC_GET_KEY_FN pGetKeyFn,
 PVOID pGetKeyArgument, PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    TRACE("(%s, %s, 0x%08x, %p, %p, %p, %p, %p, %p)\n",
     debugstr_w(pszPrincipal), debugstr_w(pszPackage), fCredentialUse,
     pLogonID, pAuthData, pGetKeyFn, pGetKeyArgument, phCredential, ptsExpiry);
    return schan_AcquireCredentialsHandle(fCredentialUse,
     pAuthData, phCredential, ptsExpiry);
}

SECURITY_STATUS SEC_ENTRY schan_FreeCredentialsHandle(
 PCredHandle phCredential)
{
    struct schan_credentials *creds;

    TRACE("phCredential %p\n", phCredential);

    if (!phCredential) return SEC_E_INVALID_HANDLE;

    creds = schan_free_handle(phCredential->dwLower, SCHAN_HANDLE_CRED);
    if (!creds) return SEC_E_INVALID_HANDLE;

    if (creds->credential_use == SECPKG_CRED_OUTBOUND)
        schan_imp_free_certificate_credentials(creds);
    HeapFree(GetProcessHeap(), 0, creds);

    return SEC_E_OK;
}

static void init_schan_buffers(struct schan_buffers *s, const PSecBufferDesc desc,
        int (*get_next_buffer)(const struct schan_transport *, struct schan_buffers *))
{
    s->offset = 0;
    s->limit = ~0UL;
    s->desc = desc;
    s->current_buffer_idx = -1;
    s->allow_buffer_resize = FALSE;
    s->get_next_buffer = get_next_buffer;
}

static int schan_find_sec_buffer_idx(const SecBufferDesc *desc, unsigned int start_idx, ULONG buffer_type)
{
    unsigned int i;
    PSecBuffer buffer;

    for (i = start_idx; i < desc->cBuffers; ++i)
    {
        buffer = &desc->pBuffers[i];
        if (buffer->BufferType == buffer_type) return i;
    }

    return -1;
}

static void schan_resize_current_buffer(const struct schan_buffers *s, SIZE_T min_size)
{
    SecBuffer *b = &s->desc->pBuffers[s->current_buffer_idx];
    SIZE_T new_size = b->cbBuffer ? b->cbBuffer * 2 : 128;
    void *new_data;

    if (b->cbBuffer >= min_size || !s->allow_buffer_resize || min_size > UINT_MAX / 2) return;

    while (new_size < min_size) new_size *= 2;

    if (b->pvBuffer)
        new_data = HeapReAlloc(GetProcessHeap(), 0, b->pvBuffer, new_size);
    else
        new_data = HeapAlloc(GetProcessHeap(), 0, new_size);

    if (!new_data)
    {
        TRACE("Failed to resize %p from %d to %ld\n", b->pvBuffer, b->cbBuffer, new_size);
        return;
    }

    b->cbBuffer = new_size;
    b->pvBuffer = new_data;
}

char *schan_get_buffer(const struct schan_transport *t, struct schan_buffers *s, SIZE_T *count)
{
    SIZE_T max_count;
    PSecBuffer buffer;

    if (!s->desc)
    {
        TRACE("No desc\n");
        return NULL;
    }

    if (s->current_buffer_idx == -1)
    {
        /* Initial buffer */
        int buffer_idx = s->get_next_buffer(t, s);
        if (buffer_idx == -1)
        {
            TRACE("No next buffer\n");
            return NULL;
        }
        s->current_buffer_idx = buffer_idx;
    }

    buffer = &s->desc->pBuffers[s->current_buffer_idx];
    TRACE("Using buffer %d: cbBuffer %d, BufferType %#x, pvBuffer %p\n", s->current_buffer_idx, buffer->cbBuffer, buffer->BufferType, buffer->pvBuffer);

    schan_resize_current_buffer(s, s->offset + *count);
    max_count = buffer->cbBuffer - s->offset;
    if (s->limit != ~0UL && s->limit < max_count)
        max_count = s->limit;
    if (!max_count)
    {
        int buffer_idx;

        s->allow_buffer_resize = FALSE;
        buffer_idx = s->get_next_buffer(t, s);
        if (buffer_idx == -1)
        {
            TRACE("No next buffer\n");
            return NULL;
        }
        s->current_buffer_idx = buffer_idx;
        s->offset = 0;
        return schan_get_buffer(t, s, count);
    }

    if (*count > max_count)
        *count = max_count;
    if (s->limit != ~0UL)
        s->limit -= *count;

    return (char *)buffer->pvBuffer + s->offset;
}

/* schan_pull
 *      Read data from the transport input buffer.
 *
 * t - The session transport object.
 * buff - The buffer into which to store the read data.  Must be at least
 *        *buff_len bytes in length.
 * buff_len - On input, *buff_len is the desired length to read.  On successful
 *            return, *buff_len is the number of bytes actually read.
 *
 * Returns:
 *  0 on success, in which case:
 *      *buff_len == 0 indicates end of file.
 *      *buff_len > 0 indicates that some data was read.  May be less than
 *          what was requested, in which case the caller should call again if/
 *          when they want more.
 *  EAGAIN when no data could be read without blocking
 *  another errno-style error value on failure
 *
 */
int schan_pull(struct schan_transport *t, void *buff, size_t *buff_len)
{
    char *b;
    SIZE_T local_len = *buff_len;

    TRACE("Pull %lu bytes\n", local_len);

    *buff_len = 0;

    b = schan_get_buffer(t, &t->in, &local_len);
    if (!b)
        return EAGAIN;

    memcpy(buff, b, local_len);
    t->in.offset += local_len;

    TRACE("Read %lu bytes\n", local_len);

    *buff_len = local_len;
    return 0;
}

/* schan_push
 *      Write data to the transport output buffer.
 *
 * t - The session transport object.
 * buff - The buffer of data to write.  Must be at least *buff_len bytes in length.
 * buff_len - On input, *buff_len is the desired length to write.  On successful
 *            return, *buff_len is the number of bytes actually written.
 *
 * Returns:
 *  0 on success
 *      *buff_len will be > 0 indicating how much data was written.  May be less
 *          than what was requested, in which case the caller should call again
            if/when they want to write more.
 *  EAGAIN when no data could be written without blocking
 *  another errno-style error value on failure
 *
 */
int schan_push(struct schan_transport *t, const void *buff, size_t *buff_len)
{
    char *b;
    SIZE_T local_len = *buff_len;

    TRACE("Push %lu bytes\n", local_len);

    *buff_len = 0;

    b = schan_get_buffer(t, &t->out, &local_len);
    if (!b)
        return EAGAIN;

    memcpy(b, buff, local_len);
    t->out.offset += local_len;

    TRACE("Wrote %lu bytes\n", local_len);

    *buff_len = local_len;
    return 0;
}

schan_imp_session schan_session_for_transport(struct schan_transport* t)
{
    return t->ctx->session;
}

static int schan_init_sec_ctx_get_next_buffer(const struct schan_transport *t, struct schan_buffers *s)
{
    if (s->current_buffer_idx == -1)
    {
        int idx = schan_find_sec_buffer_idx(s->desc, 0, SECBUFFER_TOKEN);
        if (t->ctx->req_ctx_attr & ISC_REQ_ALLOCATE_MEMORY)
        {
            if (idx == -1)
            {
                idx = schan_find_sec_buffer_idx(s->desc, 0, SECBUFFER_EMPTY);
                if (idx != -1) s->desc->pBuffers[idx].BufferType = SECBUFFER_TOKEN;
            }
            if (idx != -1 && !s->desc->pBuffers[idx].pvBuffer)
            {
                s->desc->pBuffers[idx].cbBuffer = 0;
                s->allow_buffer_resize = TRUE;
            }
        }
        return idx;
    }

    return -1;
}

static void dump_buffer_desc(SecBufferDesc *desc)
{
    unsigned int i;

    if (!desc) return;
    TRACE("Buffer desc %p:\n", desc);
    for (i = 0; i < desc->cBuffers; ++i)
    {
        SecBuffer *b = &desc->pBuffers[i];
        TRACE("\tbuffer %u: cbBuffer %d, BufferType %#x pvBuffer %p\n", i, b->cbBuffer, b->BufferType, b->pvBuffer);
    }
}

/***********************************************************************
 *              InitializeSecurityContextW
 */
SECURITY_STATUS SEC_ENTRY schan_InitializeSecurityContextW(
 PCredHandle phCredential, PCtxtHandle phContext, SEC_WCHAR *pszTargetName,
 ULONG fContextReq, ULONG Reserved1, ULONG TargetDataRep,
 PSecBufferDesc pInput, ULONG Reserved2, PCtxtHandle phNewContext,
 PSecBufferDesc pOutput, ULONG *pfContextAttr, PTimeStamp ptsExpiry)
{
    struct schan_context *ctx;
    struct schan_buffers *out_buffers;
    struct schan_credentials *cred;
    struct schan_transport transport;
    SIZE_T expected_size = ~0UL;
    SECURITY_STATUS ret;

    TRACE("%p %p %s 0x%08x %d %d %p %d %p %p %p %p\n", phCredential, phContext,
     debugstr_w(pszTargetName), fContextReq, Reserved1, TargetDataRep, pInput,
     Reserved1, phNewContext, pOutput, pfContextAttr, ptsExpiry);

    dump_buffer_desc(pInput);
    dump_buffer_desc(pOutput);

    if (!phContext)
    {
        ULONG_PTR handle;

        if (!phCredential) return SEC_E_INVALID_HANDLE;

        cred = schan_get_object(phCredential->dwLower, SCHAN_HANDLE_CRED);
        if (!cred) return SEC_E_INVALID_HANDLE;

        if (!(cred->credential_use & SECPKG_CRED_OUTBOUND))
        {
            WARN("Invalid credential use %#x\n", cred->credential_use);
            return SEC_E_INVALID_HANDLE;
        }

        ctx = HeapAlloc(GetProcessHeap(), 0, sizeof(*ctx));
        if (!ctx) return SEC_E_INSUFFICIENT_MEMORY;

        ctx->cert = NULL;
        handle = schan_alloc_handle(ctx, SCHAN_HANDLE_CTX);
        if (handle == SCHAN_INVALID_HANDLE)
        {
            HeapFree(GetProcessHeap(), 0, ctx);
            return SEC_E_INTERNAL_ERROR;
        }

        if (!schan_imp_create_session(&ctx->session, cred))
        {
            schan_free_handle(handle, SCHAN_HANDLE_CTX);
            HeapFree(GetProcessHeap(), 0, ctx);
            return SEC_E_INTERNAL_ERROR;
        }

        if (pszTargetName)
        {
            UINT len = WideCharToMultiByte( CP_UNIXCP, 0, pszTargetName, -1, NULL, 0, NULL, NULL );
            char *target = HeapAlloc( GetProcessHeap(), 0, len );

            if (target)
            {
                WideCharToMultiByte( CP_UNIXCP, 0, pszTargetName, -1, target, len, NULL, NULL );
                schan_imp_set_session_target( ctx->session, target );
                HeapFree( GetProcessHeap(), 0, target );
            }
        }
        phNewContext->dwLower = handle;
        phNewContext->dwUpper = 0;
    }
    else
    {
        SIZE_T record_size = 0;
        unsigned char *ptr;
        SecBuffer *buffer;
        int idx;

        if (!pInput)
            return SEC_E_INCOMPLETE_MESSAGE;

        idx = schan_find_sec_buffer_idx(pInput, 0, SECBUFFER_TOKEN);
        if (idx == -1)
            return SEC_E_INCOMPLETE_MESSAGE;

        buffer = &pInput->pBuffers[idx];
        ptr = buffer->pvBuffer;
        expected_size = 0;

        while (buffer->cbBuffer > expected_size + 5)
        {
            record_size = 5 + ((ptr[3] << 8) | ptr[4]);

            if (buffer->cbBuffer < expected_size + record_size)
                break;

            expected_size += record_size;
            ptr += record_size;
        }

        if (!expected_size)
        {
            TRACE("Expected at least %lu bytes, but buffer only contains %u bytes.\n",
                    max(6, record_size), buffer->cbBuffer);
            return SEC_E_INCOMPLETE_MESSAGE;
        }

        TRACE("Using expected_size %lu.\n", expected_size);

        ctx = schan_get_object(phContext->dwLower, SCHAN_HANDLE_CTX);
    }

    ctx->req_ctx_attr = fContextReq;

    transport.ctx = ctx;
    init_schan_buffers(&transport.in, pInput, schan_init_sec_ctx_get_next_buffer);
    transport.in.limit = expected_size;
    init_schan_buffers(&transport.out, pOutput, schan_init_sec_ctx_get_next_buffer);
    schan_imp_set_session_transport(ctx->session, &transport);

    /* Perform the TLS handshake */
    ret = schan_imp_handshake(ctx->session);

    if(transport.in.offset && transport.in.offset != pInput->pBuffers[0].cbBuffer) {
        if(pInput->cBuffers<2 || pInput->pBuffers[1].BufferType!=SECBUFFER_EMPTY)
            return SEC_E_INVALID_TOKEN;

        pInput->pBuffers[1].BufferType = SECBUFFER_EXTRA;
        pInput->pBuffers[1].cbBuffer = pInput->pBuffers[0].cbBuffer-transport.in.offset;
    }

    out_buffers = &transport.out;
    if (out_buffers->current_buffer_idx != -1)
    {
        SecBuffer *buffer = &out_buffers->desc->pBuffers[out_buffers->current_buffer_idx];
        buffer->cbBuffer = out_buffers->offset;
    }

    *pfContextAttr = 0;
    if (ctx->req_ctx_attr & ISC_REQ_ALLOCATE_MEMORY)
        *pfContextAttr |= ISC_RET_ALLOCATED_MEMORY;

    return ret;
}

/***********************************************************************
 *              InitializeSecurityContextA
 */
SECURITY_STATUS SEC_ENTRY schan_InitializeSecurityContextA(
 PCredHandle phCredential, PCtxtHandle phContext, SEC_CHAR *pszTargetName,
 ULONG fContextReq, ULONG Reserved1, ULONG TargetDataRep,
 PSecBufferDesc pInput, ULONG Reserved2, PCtxtHandle phNewContext,
 PSecBufferDesc pOutput, ULONG *pfContextAttr, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;
    SEC_WCHAR *target_name = NULL;

    TRACE("%p %p %s %d %d %d %p %d %p %p %p %p\n", phCredential, phContext,
     debugstr_a(pszTargetName), fContextReq, Reserved1, TargetDataRep, pInput,
     Reserved1, phNewContext, pOutput, pfContextAttr, ptsExpiry);

    if (pszTargetName)
    {
        INT len = MultiByteToWideChar(CP_ACP, 0, pszTargetName, -1, NULL, 0);
        target_name = HeapAlloc(GetProcessHeap(), 0, len * sizeof(*target_name));
        MultiByteToWideChar(CP_ACP, 0, pszTargetName, -1, target_name, len);
    }

    ret = schan_InitializeSecurityContextW(phCredential, phContext, target_name,
            fContextReq, Reserved1, TargetDataRep, pInput, Reserved2,
            phNewContext, pOutput, pfContextAttr, ptsExpiry);

    HeapFree(GetProcessHeap(), 0, target_name);

    return ret;
}

static
SECURITY_STATUS SEC_ENTRY schan_QueryContextAttributesW(
        PCtxtHandle context_handle, ULONG attribute, PVOID buffer)
{
    struct schan_context *ctx;

    TRACE("context_handle %p, attribute %#x, buffer %p\n",
            context_handle, attribute, buffer);

    if (!context_handle) return SEC_E_INVALID_HANDLE;
    ctx = schan_get_object(context_handle->dwLower, SCHAN_HANDLE_CTX);

    switch(attribute)
    {
        case SECPKG_ATTR_STREAM_SIZES:
        {
            SecPkgContext_ConnectionInfo info;
            SECURITY_STATUS status = schan_imp_get_connection_info(ctx->session, &info);
            if (status == SEC_E_OK)
            {
                SecPkgContext_StreamSizes *stream_sizes = buffer;
                SIZE_T mac_size = info.dwHashStrength;
                unsigned int block_size = schan_imp_get_session_cipher_block_size(ctx->session);
                unsigned int message_size = schan_imp_get_max_message_size(ctx->session);

                TRACE("Using %lu mac bytes, message size %u, block size %u\n",
                        mac_size, message_size, block_size);

                /* These are defined by the TLS RFC */
                stream_sizes->cbHeader = 5;
                stream_sizes->cbTrailer = mac_size + 256; /* Max 255 bytes padding + 1 for padding size */
                stream_sizes->cbMaximumMessage = message_size;
                stream_sizes->cbBuffers = 4;
                stream_sizes->cbBlockSize = block_size;
            }

            return status;
        }
        case SECPKG_ATTR_REMOTE_CERT_CONTEXT:
        {
            PCCERT_CONTEXT *cert = buffer;

            if (!ctx->cert) {
                HCERTSTORE cert_store;
                SECURITY_STATUS status;

                cert_store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0, CERT_STORE_CREATE_NEW_FLAG, NULL);
                if(!cert_store)
                    return GetLastError();

                status = schan_imp_get_session_peer_certificate(ctx->session, cert_store, &ctx->cert);
                CertCloseStore(cert_store, 0);
                if(status != SEC_E_OK)
                    return status;
            }

            *cert = CertDuplicateCertificateContext(ctx->cert);
            return SEC_E_OK;
        }
        case SECPKG_ATTR_CONNECTION_INFO:
        {
            SecPkgContext_ConnectionInfo *info = buffer;
            return schan_imp_get_connection_info(ctx->session, info);
        }

        default:
            FIXME("Unhandled attribute %#x\n", attribute);
            return SEC_E_UNSUPPORTED_FUNCTION;
    }
}

static
SECURITY_STATUS SEC_ENTRY schan_QueryContextAttributesA(
        PCtxtHandle context_handle, ULONG attribute, PVOID buffer)
{
    TRACE("context_handle %p, attribute %#x, buffer %p\n",
            context_handle, attribute, buffer);

    switch(attribute)
    {
        case SECPKG_ATTR_STREAM_SIZES:
            return schan_QueryContextAttributesW(context_handle, attribute, buffer);
        case SECPKG_ATTR_REMOTE_CERT_CONTEXT:
            return schan_QueryContextAttributesW(context_handle, attribute, buffer);
        case SECPKG_ATTR_CONNECTION_INFO:
            return schan_QueryContextAttributesW(context_handle, attribute, buffer);

        default:
            FIXME("Unhandled attribute %#x\n", attribute);
            return SEC_E_UNSUPPORTED_FUNCTION;
    }
}

static int schan_encrypt_message_get_next_buffer(const struct schan_transport *t, struct schan_buffers *s)
{
    SecBuffer *b;

    if (s->current_buffer_idx == -1)
        return schan_find_sec_buffer_idx(s->desc, 0, SECBUFFER_STREAM_HEADER);

    b = &s->desc->pBuffers[s->current_buffer_idx];

    if (b->BufferType == SECBUFFER_STREAM_HEADER)
        return schan_find_sec_buffer_idx(s->desc, 0, SECBUFFER_DATA);

    if (b->BufferType == SECBUFFER_DATA)
        return schan_find_sec_buffer_idx(s->desc, 0, SECBUFFER_STREAM_TRAILER);

    return -1;
}

static int schan_encrypt_message_get_next_buffer_token(const struct schan_transport *t, struct schan_buffers *s)
{
    SecBuffer *b;

    if (s->current_buffer_idx == -1)
        return schan_find_sec_buffer_idx(s->desc, 0, SECBUFFER_TOKEN);

    b = &s->desc->pBuffers[s->current_buffer_idx];

    if (b->BufferType == SECBUFFER_TOKEN)
    {
        int idx = schan_find_sec_buffer_idx(s->desc, 0, SECBUFFER_TOKEN);
        if (idx != s->current_buffer_idx) return -1;
        return schan_find_sec_buffer_idx(s->desc, 0, SECBUFFER_DATA);
    }

    if (b->BufferType == SECBUFFER_DATA)
    {
        int idx = schan_find_sec_buffer_idx(s->desc, 0, SECBUFFER_TOKEN);
        if (idx != -1)
            idx = schan_find_sec_buffer_idx(s->desc, idx + 1, SECBUFFER_TOKEN);
        return idx;
    }

    return -1;
}

static SECURITY_STATUS SEC_ENTRY schan_EncryptMessage(PCtxtHandle context_handle,
        ULONG quality, PSecBufferDesc message, ULONG message_seq_no)
{
    struct schan_transport transport;
    struct schan_context *ctx;
    struct schan_buffers *b;
    SECURITY_STATUS status;
    SecBuffer *buffer;
    SIZE_T data_size;
    SIZE_T length;
    char *data;
    int idx;

    TRACE("context_handle %p, quality %d, message %p, message_seq_no %d\n",
            context_handle, quality, message, message_seq_no);

    if (!context_handle) return SEC_E_INVALID_HANDLE;
    ctx = schan_get_object(context_handle->dwLower, SCHAN_HANDLE_CTX);

    dump_buffer_desc(message);

    idx = schan_find_sec_buffer_idx(message, 0, SECBUFFER_DATA);
    if (idx == -1)
    {
        WARN("No data buffer passed\n");
        return SEC_E_INTERNAL_ERROR;
    }
    buffer = &message->pBuffers[idx];

    data_size = buffer->cbBuffer;
    data = HeapAlloc(GetProcessHeap(), 0, data_size);
    memcpy(data, buffer->pvBuffer, data_size);

    transport.ctx = ctx;
    init_schan_buffers(&transport.in, NULL, NULL);
    if (schan_find_sec_buffer_idx(message, 0, SECBUFFER_STREAM_HEADER) != -1)
        init_schan_buffers(&transport.out, message, schan_encrypt_message_get_next_buffer);
    else
        init_schan_buffers(&transport.out, message, schan_encrypt_message_get_next_buffer_token);
    schan_imp_set_session_transport(ctx->session, &transport);

    length = data_size;
    status = schan_imp_send(ctx->session, data, &length);

    TRACE("Sent %ld bytes.\n", length);

    if (length != data_size)
        status = SEC_E_INTERNAL_ERROR;

    b = &transport.out;
    b->desc->pBuffers[b->current_buffer_idx].cbBuffer = b->offset;
    HeapFree(GetProcessHeap(), 0, data);

    TRACE("Returning %#x.\n", status);

    return status;
}

static int schan_decrypt_message_get_next_buffer(const struct schan_transport *t, struct schan_buffers *s)
{
    if (s->current_buffer_idx == -1)
        return schan_find_sec_buffer_idx(s->desc, 0, SECBUFFER_DATA);

    return -1;
}

static int schan_validate_decrypt_buffer_desc(PSecBufferDesc message)
{
    int data_idx = -1;
    unsigned int empty_count = 0;
    unsigned int i;

    if (message->cBuffers < 4)
    {
        WARN("Less than four buffers passed\n");
        return -1;
    }

    for (i = 0; i < message->cBuffers; ++i)
    {
        SecBuffer *b = &message->pBuffers[i];
        if (b->BufferType == SECBUFFER_DATA)
        {
            if (data_idx != -1)
            {
                WARN("More than one data buffer passed\n");
                return -1;
            }
            data_idx = i;
        }
        else if (b->BufferType == SECBUFFER_EMPTY)
            ++empty_count;
    }

    if (data_idx == -1)
    {
        WARN("No data buffer passed\n");
        return -1;
    }

    if (empty_count < 3)
    {
        WARN("Less than three empty buffers passed\n");
        return -1;
    }

    return data_idx;
}

static void schan_decrypt_fill_buffer(PSecBufferDesc message, ULONG buffer_type, void *data, ULONG size)
{
    int idx;
    SecBuffer *buffer;

    idx = schan_find_sec_buffer_idx(message, 0, SECBUFFER_EMPTY);
    buffer = &message->pBuffers[idx];

    buffer->BufferType = buffer_type;
    buffer->pvBuffer = data;
    buffer->cbBuffer = size;
}

static SECURITY_STATUS SEC_ENTRY schan_DecryptMessage(PCtxtHandle context_handle,
        PSecBufferDesc message, ULONG message_seq_no, PULONG quality)
{
    struct schan_transport transport;
    struct schan_context *ctx;
    SecBuffer *buffer;
    SIZE_T data_size;
    char *data;
    unsigned expected_size;
    SSIZE_T received = 0;
    int idx;
    unsigned char *buf_ptr;

    TRACE("context_handle %p, message %p, message_seq_no %d, quality %p\n",
            context_handle, message, message_seq_no, quality);

    if (!context_handle) return SEC_E_INVALID_HANDLE;
    ctx = schan_get_object(context_handle->dwLower, SCHAN_HANDLE_CTX);

    dump_buffer_desc(message);

    idx = schan_validate_decrypt_buffer_desc(message);
    if (idx == -1)
        return SEC_E_INVALID_TOKEN;
    buffer = &message->pBuffers[idx];
    buf_ptr = buffer->pvBuffer;

    expected_size = 5 + ((buf_ptr[3] << 8) | buf_ptr[4]);
    if(buffer->cbBuffer < expected_size)
    {
        TRACE("Expected %u bytes, but buffer only contains %u bytes\n", expected_size, buffer->cbBuffer);
        buffer->BufferType = SECBUFFER_MISSING;
        buffer->cbBuffer = expected_size - buffer->cbBuffer;

        /* This is a bit weird, but windows does it too */
        idx = schan_find_sec_buffer_idx(message, 0, SECBUFFER_EMPTY);
        buffer = &message->pBuffers[idx];
        buffer->BufferType = SECBUFFER_MISSING;
        buffer->cbBuffer = expected_size - buffer->cbBuffer;

        TRACE("Returning SEC_E_INCOMPLETE_MESSAGE\n");
        return SEC_E_INCOMPLETE_MESSAGE;
    }

    data_size = expected_size - 5;
    data = HeapAlloc(GetProcessHeap(), 0, data_size);

    transport.ctx = ctx;
    init_schan_buffers(&transport.in, message, schan_decrypt_message_get_next_buffer);
    transport.in.limit = expected_size;
    init_schan_buffers(&transport.out, NULL, NULL);
    schan_imp_set_session_transport(ctx->session, &transport);

    while (received < data_size)
    {
        SIZE_T length = data_size - received;
        SECURITY_STATUS status = schan_imp_recv(ctx->session, data + received, &length);

        if (status == SEC_I_CONTINUE_NEEDED)
            break;

        if (status != SEC_E_OK)
        {
            HeapFree(GetProcessHeap(), 0, data);
            ERR("Returning %x\n", status);
            return status;
        }

        if (!length)
            break;

        received += length;
    }

    TRACE("Received %ld bytes\n", received);

    memcpy(buf_ptr + 5, data, received);
    HeapFree(GetProcessHeap(), 0, data);

    schan_decrypt_fill_buffer(message, SECBUFFER_DATA,
        buf_ptr + 5, received);

    schan_decrypt_fill_buffer(message, SECBUFFER_STREAM_TRAILER,
        buf_ptr + 5 + received, buffer->cbBuffer - 5 - received);

    if(buffer->cbBuffer > expected_size)
        schan_decrypt_fill_buffer(message, SECBUFFER_EXTRA,
            buf_ptr + expected_size, buffer->cbBuffer - expected_size);

    buffer->BufferType = SECBUFFER_STREAM_HEADER;
    buffer->cbBuffer = 5;

    return SEC_E_OK;
}

SECURITY_STATUS SEC_ENTRY schan_DeleteSecurityContext(PCtxtHandle context_handle)
{
    struct schan_context *ctx;

    TRACE("context_handle %p\n", context_handle);

    if (!context_handle) return SEC_E_INVALID_HANDLE;

    ctx = schan_free_handle(context_handle->dwLower, SCHAN_HANDLE_CTX);
    if (!ctx) return SEC_E_INVALID_HANDLE;

    if (ctx->cert)
        CertFreeCertificateContext(ctx->cert);
    schan_imp_dispose_session(ctx->session);
    HeapFree(GetProcessHeap(), 0, ctx);

    return SEC_E_OK;
}

SecurityFunctionTableA schanTableA = {
    1,
    schan_EnumerateSecurityPackagesA,
    schan_QueryCredentialsAttributesA,
    schan_AcquireCredentialsHandleA,
    schan_FreeCredentialsHandle,
    NULL, /* Reserved2 */
    schan_InitializeSecurityContextA,
    NULL, /* AcceptSecurityContext */
    NULL, /* CompleteAuthToken */
    schan_DeleteSecurityContext,
    NULL, /* ApplyControlToken */
    schan_QueryContextAttributesA,
    NULL, /* ImpersonateSecurityContext */
    NULL, /* RevertSecurityContext */
    NULL, /* MakeSignature */
    NULL, /* VerifySignature */
    schan_FreeContextBuffer,
    NULL, /* QuerySecurityPackageInfoA */
    NULL, /* Reserved3 */
    NULL, /* Reserved4 */
    NULL, /* ExportSecurityContext */
    NULL, /* ImportSecurityContextA */
    NULL, /* AddCredentialsA */
    NULL, /* Reserved8 */
    NULL, /* QuerySecurityContextToken */
    schan_EncryptMessage,
    schan_DecryptMessage,
    NULL, /* SetContextAttributesA */
};

SecurityFunctionTableW schanTableW = {
    1,
    schan_EnumerateSecurityPackagesW,
    schan_QueryCredentialsAttributesW,
    schan_AcquireCredentialsHandleW,
    schan_FreeCredentialsHandle,
    NULL, /* Reserved2 */
    schan_InitializeSecurityContextW,
    NULL, /* AcceptSecurityContext */
    NULL, /* CompleteAuthToken */
    schan_DeleteSecurityContext,
    NULL, /* ApplyControlToken */
    schan_QueryContextAttributesW,
    NULL, /* ImpersonateSecurityContext */
    NULL, /* RevertSecurityContext */
    NULL, /* MakeSignature */
    NULL, /* VerifySignature */
    schan_FreeContextBuffer,
    NULL, /* QuerySecurityPackageInfoW */
    NULL, /* Reserved3 */
    NULL, /* Reserved4 */
    NULL, /* ExportSecurityContext */
    NULL, /* ImportSecurityContextW */
    NULL, /* AddCredentialsW */
    NULL, /* Reserved8 */
    NULL, /* QuerySecurityContextToken */
    schan_EncryptMessage,
    schan_DecryptMessage,
    NULL, /* SetContextAttributesW */
};

static const WCHAR schannelComment[] = { 'S','c','h','a','n','n','e','l',' ',
 'S','e','c','u','r','i','t','y',' ','P','a','c','k','a','g','e',0 };
static const WCHAR schannelDllName[] = { 's','c','h','a','n','n','e','l','.','d','l','l',0 };

void SECUR32_initSchannelSP(void)
{
    /* This is what Windows reports.  This shouldn't break any applications
     * even though the functions are missing, because the wrapper will
     * return SEC_E_UNSUPPORTED_FUNCTION if our function is NULL.
     */
    static const LONG caps =
        SECPKG_FLAG_INTEGRITY |
        SECPKG_FLAG_PRIVACY |
        SECPKG_FLAG_CONNECTION |
        SECPKG_FLAG_MULTI_REQUIRED |
        SECPKG_FLAG_EXTENDED_ERROR |
        SECPKG_FLAG_IMPERSONATION |
        SECPKG_FLAG_ACCEPT_WIN32_NAME |
        SECPKG_FLAG_STREAM;
    static const short version = 1;
    static const LONG maxToken = 16384;
    SEC_WCHAR *uniSPName = (SEC_WCHAR *)UNISP_NAME_W,
              *schannel = (SEC_WCHAR *)SCHANNEL_NAME_W;
    const SecPkgInfoW info[] = {
        { caps, version, UNISP_RPC_ID, maxToken, uniSPName, uniSPName },
        { caps, version, UNISP_RPC_ID, maxToken, schannel,
            (SEC_WCHAR *)schannelComment },
    };
    SecureProvider *provider;

    if (!schan_imp_init())
        return;

    schan_handle_table = HeapAlloc(GetProcessHeap(), 0, 64 * sizeof(*schan_handle_table));
    if (!schan_handle_table)
    {
        ERR("Failed to allocate schannel handle table.\n");
        goto fail;
    }
    schan_handle_table_size = 64;

    provider = SECUR32_addProvider(&schanTableA, &schanTableW, schannelDllName);
    if (!provider)
    {
        ERR("Failed to add schannel provider.\n");
        goto fail;
    }

    SECUR32_addPackages(provider, sizeof(info) / sizeof(info[0]), NULL, info);

    return;

fail:
    HeapFree(GetProcessHeap(), 0, schan_handle_table);
    schan_handle_table = NULL;
    schan_imp_deinit();
    return;
}

void SECUR32_deinitSchannelSP(void)
{
    SIZE_T i = schan_handle_count;

    if (!schan_handle_table) return;

    /* deinitialized sessions first because a pointer to the credentials
     * may be stored for the session. */
    while (i--)
    {
        if (schan_handle_table[i].type == SCHAN_HANDLE_CTX)
        {
            struct schan_context *ctx = schan_free_handle(i, SCHAN_HANDLE_CTX);
            schan_imp_dispose_session(ctx->session);
            HeapFree(GetProcessHeap(), 0, ctx);
        }
    }
    i = schan_handle_count;
    while (i--)
    {
        if (schan_handle_table[i].type != SCHAN_HANDLE_FREE)
        {
            struct schan_credentials *cred;
            cred = schan_free_handle(i, SCHAN_HANDLE_CRED);
            schan_imp_free_certificate_credentials(cred);
            HeapFree(GetProcessHeap(), 0, cred);
        }
    }
    HeapFree(GetProcessHeap(), 0, schan_handle_table);
    schan_imp_deinit();
}

#else /* SONAME_LIBGNUTLS || HAVE_SECURITY_SECURITY_H || SONAME_LIBMBEDTLS  */

void SECUR32_initSchannelSP(void)
{
    ERR("TLS library not found, SSL connections will fail\n");
}

void SECUR32_deinitSchannelSP(void) {}

#endif /* SONAME_LIBGNUTLS || HAVE_SECURITY_SECURITY_H || SONAME_LIBMBEDTLS  */
