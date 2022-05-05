/*
 * Copyright 2005 Juan Lang
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

#ifndef __CRYPT32_PRIVATE_H__
#define __CRYPT32_PRIVATE_H__

#include "wine/list.h"

/* a few asn.1 tags we need */
#define ASN_BOOL            (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x01)
#define ASN_BITSTRING       (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x03)
#define ASN_ENUMERATED      (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x0a)
#define ASN_UTF8STRING      (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x0c)
#define ASN_SETOF           (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x11)
#define ASN_NUMERICSTRING   (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x12)
#define ASN_PRINTABLESTRING (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x13)
#define ASN_T61STRING       (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x14)
#define ASN_VIDEOTEXSTRING  (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x15)
#define ASN_IA5STRING       (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x16)
#define ASN_UTCTIME         (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x17)
#define ASN_GENERALTIME     (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x18)
#define ASN_GRAPHICSTRING   (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x19)
#define ASN_VISIBLESTRING   (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x1a)
#define ASN_GENERALSTRING   (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x1b)
#define ASN_UNIVERSALSTRING (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x1c)
#define ASN_BMPSTRING       (ASN_UNIVERSAL | ASN_PRIMITIVE | 0x1e)

BOOL CRYPT_EncodeLen(DWORD len, BYTE *pbEncoded, DWORD *pcbEncoded) DECLSPEC_HIDDEN;

typedef BOOL (WINAPI *CryptEncodeObjectExFunc)(DWORD, LPCSTR, const void *,
 DWORD, PCRYPT_ENCODE_PARA, BYTE *, DWORD *);

struct AsnEncodeSequenceItem
{
    const void             *pvStructInfo;
    CryptEncodeObjectExFunc encodeFunc;
    DWORD                   size; /* used during encoding, not for your use */
};

BOOL WINAPI CRYPT_AsnEncodeSequence(DWORD dwCertEncodingType,
 struct AsnEncodeSequenceItem items[], DWORD cItem, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded) DECLSPEC_HIDDEN;

struct AsnConstructedItem
{
    BYTE                    tag;
    const void             *pvStructInfo;
    CryptEncodeObjectExFunc encodeFunc;
};

BOOL WINAPI CRYPT_AsnEncodeConstructed(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded) DECLSPEC_HIDDEN;
BOOL WINAPI CRYPT_AsnEncodeOid(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded) DECLSPEC_HIDDEN;
BOOL WINAPI CRYPT_AsnEncodeOctets(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded) DECLSPEC_HIDDEN;

typedef struct _CRYPT_DIGESTED_DATA
{
    DWORD                      version;
    CRYPT_ALGORITHM_IDENTIFIER DigestAlgorithm;
    CRYPT_CONTENT_INFO         ContentInfo;
    CRYPT_HASH_BLOB            hash;
} CRYPT_DIGESTED_DATA;

BOOL CRYPT_AsnEncodePKCSDigestedData(const CRYPT_DIGESTED_DATA *digestedData,
 void *pvData, DWORD *pcbData) DECLSPEC_HIDDEN;

typedef struct _CRYPT_ENCRYPTED_CONTENT_INFO
{
    LPSTR                      contentType;
    CRYPT_ALGORITHM_IDENTIFIER contentEncryptionAlgorithm;
    CRYPT_DATA_BLOB            encryptedContent;
} CRYPT_ENCRYPTED_CONTENT_INFO;

typedef struct _CRYPT_ENVELOPED_DATA
{
    DWORD                          version;
    DWORD                          cRecipientInfo;
    PCMSG_KEY_TRANS_RECIPIENT_INFO rgRecipientInfo;
    CRYPT_ENCRYPTED_CONTENT_INFO   encryptedContentInfo;
} CRYPT_ENVELOPED_DATA;

BOOL CRYPT_AsnEncodePKCSEnvelopedData(const CRYPT_ENVELOPED_DATA *envelopedData,
 void *pvData, DWORD *pcbData) DECLSPEC_HIDDEN;

BOOL CRYPT_AsnDecodePKCSEnvelopedData(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, PCRYPT_DECODE_PARA pDecodePara,
 CRYPT_ENVELOPED_DATA *envelopedData, DWORD *pcbEnvelopedData) DECLSPEC_HIDDEN;

typedef struct _CRYPT_SIGNED_INFO
{
    DWORD                 version;
    DWORD                 cCertEncoded;
    PCERT_BLOB            rgCertEncoded;
    DWORD                 cCrlEncoded;
    PCRL_BLOB             rgCrlEncoded;
    CRYPT_CONTENT_INFO    content;
    DWORD                 cSignerInfo;
    PCMSG_CMS_SIGNER_INFO rgSignerInfo;
    PDWORD                signerKeySpec;
} CRYPT_SIGNED_INFO;

BOOL CRYPT_AsnEncodeCMSSignedInfo(CRYPT_SIGNED_INFO *, void *pvData,
 DWORD *pcbData) DECLSPEC_HIDDEN;

BOOL CRYPT_AsnDecodeCMSSignedInfo(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, PCRYPT_DECODE_PARA pDecodePara,
 CRYPT_SIGNED_INFO *signedInfo, DWORD *pcbSignedInfo) DECLSPEC_HIDDEN;

/* Helper function to check *pcbEncoded, set it to the required size, and
 * optionally to allocate memory.  Assumes pbEncoded is not NULL.
 * If CRYPT_ENCODE_ALLOC_FLAG is set in dwFlags, *pbEncoded will be set to a
 * pointer to the newly allocated memory.
 */
BOOL CRYPT_EncodeEnsureSpace(DWORD dwFlags, const CRYPT_ENCODE_PARA *pEncodePara,
 BYTE *pbEncoded, DWORD *pcbEncoded, DWORD bytesNeeded) DECLSPEC_HIDDEN;

BOOL CRYPT_AsnDecodePKCSDigestedData(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, PCRYPT_DECODE_PARA pDecodePara,
 CRYPT_DIGESTED_DATA *digestedData, DWORD *pcbDigestedData) DECLSPEC_HIDDEN;

BOOL WINAPI CRYPT_AsnEncodePubKeyInfoNoNull(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded) DECLSPEC_HIDDEN;

/* The following aren't defined in wincrypt.h, as they're "reserved" */
#define CERT_CERT_PROP_ID 32
#define CERT_CRL_PROP_ID  33
#define CERT_CTL_PROP_ID  34

/* Returns a handle to the default crypto provider; loads it if necessary.
 * Returns NULL on failure.
 */
HCRYPTPROV WINAPI I_CryptGetDefaultCryptProv(ALG_ID);

extern HINSTANCE hInstance DECLSPEC_HIDDEN;

void crypt_oid_init(void) DECLSPEC_HIDDEN;
void crypt_oid_free(void) DECLSPEC_HIDDEN;
void crypt_sip_free(void) DECLSPEC_HIDDEN;
void root_store_free(void) DECLSPEC_HIDDEN;
void default_chain_engine_free(void) DECLSPEC_HIDDEN;

/* (Internal) certificate store types and functions */
struct WINE_CRYPTCERTSTORE;

typedef struct _CONTEXT_PROPERTY_LIST CONTEXT_PROPERTY_LIST;

typedef struct _context_t context_t;

typedef struct {
    void (*free)(context_t*);
    struct _context_t *(*clone)(context_t*,struct WINE_CRYPTCERTSTORE*,BOOL);
} context_vtbl_t;

struct _context_t {
    const context_vtbl_t *vtbl;
    LONG ref;
    struct WINE_CRYPTCERTSTORE *store;
    struct _context_t *linked;
    CONTEXT_PROPERTY_LIST *properties;
    union {
        struct list entry;
        void *ptr;
    } u;
};

static inline context_t *context_from_ptr(const void *ptr)
{
    return (context_t*)ptr-1;
}

static inline void *context_ptr(context_t *context)
{
    return context+1;
}

typedef struct {
    context_t base;
    CERT_CONTEXT ctx;
} cert_t;

static inline cert_t *cert_from_ptr(const CERT_CONTEXT *ptr)
{
    return CONTAINING_RECORD(ptr, cert_t, ctx);
}

typedef struct {
    context_t base;
    CRL_CONTEXT ctx;
} crl_t;

static inline crl_t *crl_from_ptr(const CRL_CONTEXT *ptr)
{
    return CONTAINING_RECORD(ptr, crl_t, ctx);
}

typedef struct {
    context_t base;
    CTL_CONTEXT ctx;
} ctl_t;

static inline ctl_t *ctl_from_ptr(const CTL_CONTEXT *ptr)
{
    return CONTAINING_RECORD(ptr, ctl_t, ctx);
}

/* Some typedefs that make it easier to abstract which type of context we're
 * working with.
 */
typedef const void *(WINAPI *CreateContextFunc)(DWORD dwCertEncodingType,
 const BYTE *pbCertEncoded, DWORD cbCertEncoded);
typedef BOOL (WINAPI *AddContextToStoreFunc)(HCERTSTORE hCertStore,
 const void *context, DWORD dwAddDisposition, const void **ppStoreContext);
typedef BOOL (WINAPI *AddEncodedContextToStoreFunc)(HCERTSTORE hCertStore,
 DWORD dwCertEncodingType, const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwAddDisposition, const void **ppContext);
typedef const void *(WINAPI *EnumContextsInStoreFunc)(HCERTSTORE hCertStore,
 const void *pPrevContext);
typedef DWORD (WINAPI *EnumPropertiesFunc)(const void *context, DWORD dwPropId);
typedef BOOL (WINAPI *GetContextPropertyFunc)(const void *context,
 DWORD dwPropID, void *pvData, DWORD *pcbData);
typedef BOOL (WINAPI *SetContextPropertyFunc)(const void *context,
 DWORD dwPropID, DWORD dwFlags, const void *pvData);
typedef BOOL (WINAPI *SerializeElementFunc)(const void *context, DWORD dwFlags,
 BYTE *pbElement, DWORD *pcbElement);
typedef BOOL (WINAPI *DeleteContextFunc)(const void *contex);

/* An abstract context (certificate, CRL, or CTL) interface */
typedef struct _WINE_CONTEXT_INTERFACE
{
    CreateContextFunc            create;
    AddContextToStoreFunc        addContextToStore;
    AddEncodedContextToStoreFunc addEncodedToStore;
    EnumContextsInStoreFunc      enumContextsInStore;
    EnumPropertiesFunc           enumProps;
    GetContextPropertyFunc       getProp;
    SetContextPropertyFunc       setProp;
    SerializeElementFunc         serialize;
    DeleteContextFunc            deleteFromStore;
} WINE_CONTEXT_INTERFACE;

extern const WINE_CONTEXT_INTERFACE *pCertInterface DECLSPEC_HIDDEN;
extern const WINE_CONTEXT_INTERFACE *pCRLInterface DECLSPEC_HIDDEN;
extern const WINE_CONTEXT_INTERFACE *pCTLInterface DECLSPEC_HIDDEN;

typedef struct WINE_CRYPTCERTSTORE * (*StoreOpenFunc)(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara);

typedef struct _CONTEXT_FUNCS
{
  /* Called to add a context to a store.  If toReplace is not NULL,
   * context replaces toReplace in the store, and access checks should not be
   * performed.  Otherwise context is a new context, and it should only be
   * added if the store allows it.  If ppStoreContext is not NULL, the added
   * context should be returned in *ppStoreContext.
   */
    BOOL (*addContext)(struct WINE_CRYPTCERTSTORE*,context_t*,context_t*,context_t**,BOOL);
    context_t *(*enumContext)(struct WINE_CRYPTCERTSTORE *store, context_t *prev);
    BOOL (*delete)(struct WINE_CRYPTCERTSTORE*,context_t*);
} CONTEXT_FUNCS;

typedef enum _CertStoreType {
    StoreTypeMem,
    StoreTypeCollection,
    StoreTypeProvider,
    StoreTypeEmpty
} CertStoreType;

#define WINE_CRYPTCERTSTORE_MAGIC 0x74726563

/* A cert store is polymorphic through the use of function pointers.  A type
 * is still needed to distinguish collection stores from other types.
 * On the function pointers:
 * - closeStore is called when the store's ref count becomes 0
 * - control is optional, but should be implemented by any store that supports
 *   persistence
 */

typedef struct {
    void (*addref)(struct WINE_CRYPTCERTSTORE*);
    DWORD (*release)(struct WINE_CRYPTCERTSTORE*,DWORD);
    void (*releaseContext)(struct WINE_CRYPTCERTSTORE*,context_t*);
    BOOL (*control)(struct WINE_CRYPTCERTSTORE*,DWORD,DWORD,void const*);
    CONTEXT_FUNCS certs;
    CONTEXT_FUNCS crls;
    CONTEXT_FUNCS ctls;
} store_vtbl_t;

typedef struct WINE_CRYPTCERTSTORE
{
    DWORD                       dwMagic;
    LONG                        ref;
    DWORD                       dwOpenFlags;
    CertStoreType               type;
    const store_vtbl_t         *vtbl;
    CONTEXT_PROPERTY_LIST      *properties;
} WINECRYPT_CERTSTORE;

void CRYPT_InitStore(WINECRYPT_CERTSTORE *store, DWORD dwFlags,
 CertStoreType type, const store_vtbl_t*) DECLSPEC_HIDDEN;
void CRYPT_FreeStore(WINECRYPT_CERTSTORE *store) DECLSPEC_HIDDEN;
BOOL WINAPI I_CertUpdateStore(HCERTSTORE store1, HCERTSTORE store2, DWORD unk0,
 DWORD unk1) DECLSPEC_HIDDEN;

WINECRYPT_CERTSTORE *CRYPT_CollectionOpenStore(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara) DECLSPEC_HIDDEN;
WINECRYPT_CERTSTORE *CRYPT_ProvCreateStore(DWORD dwFlags,
 WINECRYPT_CERTSTORE *memStore, const CERT_STORE_PROV_INFO *pProvInfo) DECLSPEC_HIDDEN;
WINECRYPT_CERTSTORE *CRYPT_ProvOpenStore(LPCSTR lpszStoreProvider,
 DWORD dwEncodingType, HCRYPTPROV hCryptProv, DWORD dwFlags,
 const void *pvPara) DECLSPEC_HIDDEN;
WINECRYPT_CERTSTORE *CRYPT_RegOpenStore(HCRYPTPROV hCryptProv, DWORD dwFlags,
 const void *pvPara) DECLSPEC_HIDDEN;
WINECRYPT_CERTSTORE *CRYPT_FileOpenStore(HCRYPTPROV hCryptProv, DWORD dwFlags,
 const void *pvPara) DECLSPEC_HIDDEN;
WINECRYPT_CERTSTORE *CRYPT_FileNameOpenStoreA(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara) DECLSPEC_HIDDEN;
WINECRYPT_CERTSTORE *CRYPT_FileNameOpenStoreW(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara) DECLSPEC_HIDDEN;

void CRYPT_ImportSystemRootCertsToReg(void) DECLSPEC_HIDDEN;
BOOL CRYPT_SerializeContextsToReg(HKEY key, DWORD flags, const WINE_CONTEXT_INTERFACE *contextInterface,
    HCERTSTORE memStore) DECLSPEC_HIDDEN;

DWORD CRYPT_IsCertificateSelfSigned(const CERT_CONTEXT *cert) DECLSPEC_HIDDEN;

/* Allocates and initializes a certificate chain engine, but without creating
 * the root store.  Instead, it uses root, and assumes the caller has done any
 * checking necessary.
 */
HCERTCHAINENGINE CRYPT_CreateChainEngine(HCERTSTORE, DWORD, const CERT_CHAIN_ENGINE_CONFIG*) DECLSPEC_HIDDEN;

/* Helper function for store reading functions and
 * CertAddSerializedElementToStore.  Returns a context of the appropriate type
 * if it can, or NULL otherwise.  Doesn't validate any of the properties in
 * the serialized context (for example, bad hashes are retained.)
 * *pdwContentType is set to the type of the returned context.
 */
const void *CRYPT_ReadSerializedElement(const BYTE *pbElement,
 DWORD cbElement, DWORD dwContextTypeFlags, DWORD *pdwContentType) DECLSPEC_HIDDEN;

/* Reads contexts serialized in the file into the memory store.  Returns FALSE
 * if the file is not of the expected format.
 */
BOOL CRYPT_ReadSerializedStoreFromFile(HANDLE file, HCERTSTORE store) DECLSPEC_HIDDEN;

/* Reads contexts serialized in the blob into the memory store.  Returns FALSE
 * if the file is not of the expected format.
 */
BOOL CRYPT_ReadSerializedStoreFromBlob(const CRYPT_DATA_BLOB *blob,
 HCERTSTORE store) DECLSPEC_HIDDEN;

/* Fixes up the pointers in info, where info is assumed to be a
 * CRYPT_KEY_PROV_INFO, followed by its container name, provider name, and any
 * provider parameters, in a contiguous buffer, but where info's pointers are
 * assumed to be invalid.  Upon return, info's pointers point to the
 * appropriate memory locations.
 */
void CRYPT_FixKeyProvInfoPointers(PCRYPT_KEY_PROV_INFO info) DECLSPEC_HIDDEN;

/**
 *  String functions
 */

DWORD cert_name_to_str_with_indent(DWORD dwCertEncodingType, DWORD indent,
 const CERT_NAME_BLOB *pName, DWORD dwStrType, LPWSTR psz, DWORD csz) DECLSPEC_HIDDEN;

/**
 *  Context functions
 */

/* Allocates a new data context, a context which owns properties directly.
 * contextSize is the size of the public data type associated with context,
 * which should be one of CERT_CONTEXT, CRL_CONTEXT, or CTL_CONTEXT.
 * Free with Context_Release.
 */
context_t *Context_CreateDataContext(size_t contextSize, const context_vtbl_t *vtbl, struct WINE_CRYPTCERTSTORE*) DECLSPEC_HIDDEN;

/* Creates a new link context.  The context refers to linked
 * rather than owning its own properties.  If addRef is TRUE (which ordinarily
 * it should be) linked is addref'd.
 * Free with Context_Release.
 */
context_t *Context_CreateLinkContext(unsigned contextSize, context_t *linked, struct WINE_CRYPTCERTSTORE*) DECLSPEC_HIDDEN;

/* Copies properties from fromContext to toContext. */
void Context_CopyProperties(const void *to, const void *from) DECLSPEC_HIDDEN;

void Context_AddRef(context_t*) DECLSPEC_HIDDEN;
void Context_Release(context_t *context) DECLSPEC_HIDDEN;
void Context_Free(context_t*) DECLSPEC_HIDDEN;

/**
 *  Context property list functions
 */

CONTEXT_PROPERTY_LIST *ContextPropertyList_Create(void) DECLSPEC_HIDDEN;

/* Searches for the property with ID id in the context.  Returns TRUE if found,
 * and copies the property's length and a pointer to its data to blob.
 * Otherwise returns FALSE.
 */
BOOL ContextPropertyList_FindProperty(CONTEXT_PROPERTY_LIST *list, DWORD id,
 PCRYPT_DATA_BLOB blob) DECLSPEC_HIDDEN;

BOOL ContextPropertyList_SetProperty(CONTEXT_PROPERTY_LIST *list, DWORD id,
 const BYTE *pbData, size_t cbData) DECLSPEC_HIDDEN;

void ContextPropertyList_RemoveProperty(CONTEXT_PROPERTY_LIST *list, DWORD id) DECLSPEC_HIDDEN;

DWORD ContextPropertyList_EnumPropIDs(CONTEXT_PROPERTY_LIST *list, DWORD id) DECLSPEC_HIDDEN;

void ContextPropertyList_Copy(CONTEXT_PROPERTY_LIST *to,
 CONTEXT_PROPERTY_LIST *from) DECLSPEC_HIDDEN;

void ContextPropertyList_Free(CONTEXT_PROPERTY_LIST *list) DECLSPEC_HIDDEN;

extern WINECRYPT_CERTSTORE empty_store DECLSPEC_HIDDEN;
void init_empty_store(void) DECLSPEC_HIDDEN;

/**
 *  Utilities.
 */

/* Align up to a DWORD_PTR boundary
 */
#define ALIGN_DWORD_PTR(x) (((x) + sizeof(DWORD_PTR) - 1) & ~(sizeof(DWORD_PTR) - 1))
#define POINTER_ALIGN_DWORD_PTR(p) ((LPVOID)ALIGN_DWORD_PTR((DWORD_PTR)(p)))

/* Check if the OID is a small int
 */
#define IS_INTOID(x)    (((ULONG_PTR)(x) >> 16) == 0)

#endif
