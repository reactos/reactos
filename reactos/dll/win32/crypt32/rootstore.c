/*
 * Copyright 2007 Juan Lang
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
#include "config.h"
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#include <dirent.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#include <limits.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wincrypt.h"
#include "winternl.h"
#include "wine/debug.h"
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

#define INITIAL_CERT_BUFFER 1024

struct DynamicBuffer
{
    DWORD allocated;
    DWORD used;
    BYTE *data;
};

static inline void reset_buffer(struct DynamicBuffer *buffer)
{
    buffer->used = 0;
    if (buffer->data) buffer->data[0] = 0;
}

static BOOL add_line_to_buffer(struct DynamicBuffer *buffer, LPCSTR line)
{
    BOOL ret;

    if (buffer->used + strlen(line) + 1 > buffer->allocated)
    {
        if (!buffer->allocated)
        {
            buffer->data = CryptMemAlloc(INITIAL_CERT_BUFFER);
            if (buffer->data)
            {
                buffer->data[0] = 0;
                buffer->allocated = INITIAL_CERT_BUFFER;
            }
        }
        else
        {
            DWORD new_size = max(buffer->allocated * 2,
             buffer->used + strlen(line) + 1);

            buffer->data = CryptMemRealloc(buffer->data, new_size);
            if (buffer->data)
                buffer->allocated = new_size;
        }
    }
    if (buffer->data)
    {
        strcpy((char *)buffer->data + strlen((char *)buffer->data), line);
        /* Not strlen + 1, otherwise we'd count the NULL for every line's
         * addition (but we overwrite the previous NULL character.)  Not an
         * overrun, we allocate strlen + 1 bytes above.
         */
        buffer->used += strlen(line);
        ret = TRUE;
    }
    else
        ret = FALSE;
    return ret;
}

/* Reads any base64-encoded certificates present in fp and adds them to store.
 * Returns TRUE if any certificates were successfully imported.
 */
static BOOL import_base64_certs_from_fp(FILE *fp, HCERTSTORE store)
{
    char line[1024];
    BOOL in_cert = FALSE;
    struct DynamicBuffer saved_cert = { 0, 0, NULL };
    int num_certs = 0;

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
            DWORD size;

            TRACE("end of certificate, adding cert\n");
            in_cert = FALSE;
            if (CryptStringToBinaryA((char *)saved_cert.data, saved_cert.used,
             CRYPT_STRING_BASE64, NULL, &size, NULL, NULL))
            {
                LPBYTE buf = CryptMemAlloc(size);

                if (buf)
                {
                    CryptStringToBinaryA((char *)saved_cert.data,
                     saved_cert.used, CRYPT_STRING_BASE64, buf, &size, NULL,
                     NULL);
                    if (CertAddEncodedCertificateToStore(store,
                     X509_ASN_ENCODING, buf, size, CERT_STORE_ADD_NEW, NULL))
                        num_certs++;
                    CryptMemFree(buf);
                }
            }
        }
        else if (in_cert)
            add_line_to_buffer(&saved_cert, line);
    }
    CryptMemFree(saved_cert.data);
    TRACE("Read %d certs\n", num_certs);
    return num_certs > 0;
}

static const char *trust_status_to_str(DWORD status)
{
    static char buf[1024];
    int pos = 0;

    if (status & CERT_TRUST_IS_NOT_TIME_VALID)
        pos += snprintf(buf + pos, sizeof(buf) - pos, "\n\texpired");
    if (status & CERT_TRUST_IS_NOT_TIME_NESTED)
        pos += snprintf(buf + pos, sizeof(buf) - pos, "\n\tbad time nesting");
    if (status & CERT_TRUST_IS_REVOKED)
        pos += snprintf(buf + pos, sizeof(buf) - pos, "\n\trevoked");
    if (status & CERT_TRUST_IS_NOT_SIGNATURE_VALID)
        pos += snprintf(buf + pos, sizeof(buf) - pos, "\n\tbad signature");
    if (status & CERT_TRUST_IS_NOT_VALID_FOR_USAGE)
        pos += snprintf(buf + pos, sizeof(buf) - pos, "\n\tbad usage");
    if (status & CERT_TRUST_IS_UNTRUSTED_ROOT)
        pos += snprintf(buf + pos, sizeof(buf) - pos, "\n\tuntrusted root");
    if (status & CERT_TRUST_REVOCATION_STATUS_UNKNOWN)
        pos += snprintf(buf + pos, sizeof(buf) - pos,
         "\n\tunknown revocation status");
    if (status & CERT_TRUST_IS_CYCLIC)
        pos += snprintf(buf + pos, sizeof(buf) - pos, "\n\tcyclic chain");
    if (status & CERT_TRUST_INVALID_EXTENSION)
        pos += snprintf(buf + pos, sizeof(buf) - pos,
         "\n\tunsupported critical extension");
    if (status & CERT_TRUST_INVALID_POLICY_CONSTRAINTS)
        pos += snprintf(buf + pos, sizeof(buf) - pos, "\n\tbad policy");
    if (status & CERT_TRUST_INVALID_BASIC_CONSTRAINTS)
        pos += snprintf(buf + pos, sizeof(buf) - pos,
         "\n\tbad basic constraints");
    if (status & CERT_TRUST_INVALID_NAME_CONSTRAINTS)
        pos += snprintf(buf + pos, sizeof(buf) - pos,
         "\n\tbad name constraints");
    if (status & CERT_TRUST_HAS_NOT_SUPPORTED_NAME_CONSTRAINT)
        pos += snprintf(buf + pos, sizeof(buf) - pos,
         "\n\tunsuported name constraint");
    if (status & CERT_TRUST_HAS_NOT_DEFINED_NAME_CONSTRAINT)
        pos += snprintf(buf + pos, sizeof(buf) - pos,
         "\n\tundefined name constraint");
    if (status & CERT_TRUST_HAS_NOT_PERMITTED_NAME_CONSTRAINT)
        pos += snprintf(buf + pos, sizeof(buf) - pos,
         "\n\tdisallowed name constraint");
    if (status & CERT_TRUST_HAS_EXCLUDED_NAME_CONSTRAINT)
        pos += snprintf(buf + pos, sizeof(buf) - pos,
         "\n\texcluded name constraint");
    if (status & CERT_TRUST_IS_OFFLINE_REVOCATION)
        pos += snprintf(buf + pos, sizeof(buf) - pos,
         "\n\trevocation server offline");
    if (status & CERT_TRUST_NO_ISSUANCE_CHAIN_POLICY)
        pos += snprintf(buf + pos, sizeof(buf) - pos,
         "\n\tno issuance policy");
    return buf;
}

static const char *get_cert_common_name(PCCERT_CONTEXT cert)
{
    static char buf[1024];
    const char *name = NULL;
    CERT_NAME_INFO *nameInfo;
    DWORD size;
    BOOL ret = CryptDecodeObjectEx(X509_ASN_ENCODING, X509_NAME,
     cert->pCertInfo->Subject.pbData, cert->pCertInfo->Subject.cbData,
     CRYPT_DECODE_NOCOPY_FLAG | CRYPT_DECODE_ALLOC_FLAG, NULL, &nameInfo,
     &size);

    if (ret)
    {
        PCERT_RDN_ATTR commonName = CertFindRDNAttr(szOID_COMMON_NAME,
         nameInfo);

        if (commonName)
        {
            CertRDNValueToStrA(commonName->dwValueType,
             &commonName->Value, buf, sizeof(buf));
            name = buf;
        }
        LocalFree(nameInfo);
    }
    return name;
}

static void check_and_store_certs(HCERTSTORE from, HCERTSTORE to)
{
    DWORD root_count = 0;
    CERT_CHAIN_ENGINE_CONFIG chainEngineConfig =
     { sizeof(chainEngineConfig), 0 };
    HCERTCHAINENGINE engine;

    TRACE("\n");

    CertDuplicateStore(to);
    engine = CRYPT_CreateChainEngine(to, &chainEngineConfig);
    if (engine)
    {
        PCCERT_CONTEXT cert = NULL;

        do {
            cert = CertEnumCertificatesInStore(from, cert);
            if (cert)
            {
                CERT_CHAIN_PARA chainPara = { sizeof(chainPara), { 0 } };
                PCCERT_CHAIN_CONTEXT chain;
                BOOL ret = CertGetCertificateChain(engine, cert, NULL, from,
                 &chainPara, 0, NULL, &chain);

                if (!ret)
                    TRACE("rejecting %s: %s\n", get_cert_common_name(cert),
                     "chain creation failed");
                else
                {
                    /* The only allowed error is CERT_TRUST_IS_UNTRUSTED_ROOT */
                    if (chain->TrustStatus.dwErrorStatus &
                     ~CERT_TRUST_IS_UNTRUSTED_ROOT)
                        TRACE("rejecting %s: %s\n", get_cert_common_name(cert),
                         trust_status_to_str(chain->TrustStatus.dwErrorStatus &
                         ~CERT_TRUST_IS_UNTRUSTED_ROOT));
                    else
                    {
                        DWORD i, j;

                        for (i = 0; i < chain->cChain; i++)
                            for (j = 0; j < chain->rgpChain[i]->cElement; j++)
                                if (CertAddCertificateContextToStore(to,
                                 chain->rgpChain[i]->rgpElement[j]->pCertContext,
                                 CERT_STORE_ADD_NEW, NULL))
                                    root_count++;
                    }
                    CertFreeCertificateChain(chain);
                }
            }
        } while (cert);
        CertFreeCertificateChainEngine(engine);
    }
    TRACE("Added %d root certificates\n", root_count);
}

/* Reads the file fd, and imports any certificates in it into store.
 * Returns TRUE if any certificates were successfully imported.
 */
static BOOL import_certs_from_file(int fd, HCERTSTORE store)
{
    BOOL ret = FALSE;
    FILE *fp;

    TRACE("\n");

    fp = fdopen(fd, "r");
    if (fp)
    {
        ret = import_base64_certs_from_fp(fp, store);
        fclose(fp);
    }
    return ret;
}

static BOOL import_certs_from_path(LPCSTR path, HCERTSTORE store,
 BOOL allow_dir);

/* Opens path, which must be a directory, and imports certificates from every
 * file in the directory into store.
 * Returns TRUE if any certificates were successfully imported.
 */
static BOOL import_certs_from_dir(LPCSTR path, HCERTSTORE store)
{
    BOOL ret = FALSE;
    DIR *dir;

    TRACE("(%s, %p)\n", debugstr_a(path), store);
    /* UNIX functions = bad for reactos
    dir = opendir(path);
    if (dir)
    {
        size_t bufsize = strlen(path) + 1 + PATH_MAX + 1;
        char *filebuf = CryptMemAlloc(bufsize);

        if (filebuf)
        {
            struct dirent *entry;
            while ((entry = readdir(dir)))
            {
                if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
                {
                    snprintf(filebuf, bufsize, "%s/%s", path, entry->d_name);
                    if (import_certs_from_path(filebuf, store, FALSE) && !ret)
                        ret = TRUE;
                }
            }
            closedir(dir);
            CryptMemFree(filebuf);
        }
    }
    */
    return ret;
}

/* Opens path, which may be a file or a directory, and imports any certificates
 * it finds into store.
 * Returns TRUE if any certificates were successfully imported.
 */
static BOOL import_certs_from_path(LPCSTR path, HCERTSTORE store,
 BOOL allow_dir)
{
    BOOL ret = FALSE;
    int fd;

    TRACE("(%s, %p, %d)\n", debugstr_a(path), store, allow_dir);

    fd = open(path, O_RDONLY);
    if (fd != -1)
    {
        struct stat st;

        if (fstat(fd, &st) == 0)
        {
            if (S_ISREG(st.st_mode))
                ret = import_certs_from_file(fd, store);
            else if (S_ISDIR(st.st_mode))
            {
                if (allow_dir)
                    ret = import_certs_from_dir(path, store);
                else
                    WARN("%s is a directory and directories are disallowed\n",
                     debugstr_a(path));
            }
            else
                ERR("%s: invalid file type\n", path);
        }
        close(fd);
    }
    return ret;
}

static BOOL WINAPI CRYPT_RootWriteCert(HCERTSTORE hCertStore,
 PCCERT_CONTEXT cert, DWORD dwFlags)
{
    /* The root store can't have certs added */
    return FALSE;
}

static BOOL WINAPI CRYPT_RootDeleteCert(HCERTSTORE hCertStore,
 PCCERT_CONTEXT cert, DWORD dwFlags)
{
    /* The root store can't have certs deleted */
    return FALSE;
}

static BOOL WINAPI CRYPT_RootWriteCRL(HCERTSTORE hCertStore,
 PCCRL_CONTEXT crl, DWORD dwFlags)
{
    /* The root store can have CRLs added.  At worst, a malicious application
     * can DoS itself, as the changes aren't persisted in any way.
     */
    return TRUE;
}

static BOOL WINAPI CRYPT_RootDeleteCRL(HCERTSTORE hCertStore,
 PCCRL_CONTEXT crl, DWORD dwFlags)
{
    /* The root store can't have CRLs deleted */
    return FALSE;
}

static void *rootProvFuncs[] = {
    NULL, /* CERT_STORE_PROV_CLOSE_FUNC */
    NULL, /* CERT_STORE_PROV_READ_CERT_FUNC */
    CRYPT_RootWriteCert,
    CRYPT_RootDeleteCert,
    NULL, /* CERT_STORE_PROV_SET_CERT_PROPERTY_FUNC */
    NULL, /* CERT_STORE_PROV_READ_CRL_FUNC */
    CRYPT_RootWriteCRL,
    CRYPT_RootDeleteCRL,
    NULL, /* CERT_STORE_PROV_SET_CRL_PROPERTY_FUNC */
    NULL, /* CERT_STORE_PROV_READ_CTL_FUNC */
    NULL, /* CERT_STORE_PROV_WRITE_CTL_FUNC */
    NULL, /* CERT_STORE_PROV_DELETE_CTL_FUNC */
    NULL, /* CERT_STORE_PROV_SET_CTL_PROPERTY_FUNC */
    NULL, /* CERT_STORE_PROV_CONTROL_FUNC */
};

static const char * const CRYPT_knownLocations[] = {
 "/etc/ssl/certs/ca-certificates.crt",
 "/etc/ssl/certs",
 "/etc/pki/tls/certs/ca-bundle.crt",
};

/* Reads certificates from the list of known locations.  Stops when any
 * location contains any certificates, to prevent spending unnecessary time
 * adding redundant certificates, e.g. when both a certificate bundle and
 * individual certificates exist in the same directory.
 */
static PWINECRYPT_CERTSTORE CRYPT_RootOpenStoreFromKnownLocations(void)
{
    HCERTSTORE root = NULL;
    HCERTSTORE from = CertOpenStore(CERT_STORE_PROV_MEMORY,
     X509_ASN_ENCODING, 0, CERT_STORE_CREATE_NEW_FLAG, NULL);
    HCERTSTORE to = CertOpenStore(CERT_STORE_PROV_MEMORY,
     X509_ASN_ENCODING, 0, CERT_STORE_CREATE_NEW_FLAG, NULL);

    if (from && to)
    {
        CERT_STORE_PROV_INFO provInfo = {
         sizeof(CERT_STORE_PROV_INFO),
         sizeof(rootProvFuncs) / sizeof(rootProvFuncs[0]),
         rootProvFuncs,
         NULL,
         0,
         NULL
        };
        DWORD i;
        BOOL ret = FALSE;

        for (i = 0; !ret &&
         i < sizeof(CRYPT_knownLocations) / sizeof(CRYPT_knownLocations[0]);
         i++)
            ret = import_certs_from_path(CRYPT_knownLocations[i], from, TRUE);
        check_and_store_certs(from, to);
        root = CRYPT_ProvCreateStore(0, to, &provInfo);
    }
    CertCloseStore(from, 0);
    TRACE("returning %p\n", root);
    return root;
}

static PWINECRYPT_CERTSTORE CRYPT_rootStore;

PWINECRYPT_CERTSTORE CRYPT_RootOpenStore(HCRYPTPROV hCryptProv, DWORD dwFlags)
{
    TRACE("(%ld, %08x)\n", hCryptProv, dwFlags);

    if (dwFlags & CERT_STORE_DELETE_FLAG)
    {
        WARN("root store can't be deleted\n");
        SetLastError(ERROR_ACCESS_DENIED);
        return NULL;
    }
    switch (dwFlags & CERT_SYSTEM_STORE_LOCATION_MASK)
    {
    case CERT_SYSTEM_STORE_LOCAL_MACHINE:
    case CERT_SYSTEM_STORE_CURRENT_USER:
        break;
    default:
        TRACE("location %08x unsupported\n",
         dwFlags & CERT_SYSTEM_STORE_LOCATION_MASK);
        SetLastError(E_INVALIDARG);
        return NULL;
    }
    if (!CRYPT_rootStore)
    {
        HCERTSTORE root = CRYPT_RootOpenStoreFromKnownLocations();

        InterlockedCompareExchangePointer((PVOID *)&CRYPT_rootStore, root,
         NULL);
        if (CRYPT_rootStore != root)
            CertCloseStore(root, 0);
    }
    CertDuplicateStore(CRYPT_rootStore);
    return CRYPT_rootStore;
}

void root_store_free(void)
{
    CertCloseStore(CRYPT_rootStore, 0);
}
