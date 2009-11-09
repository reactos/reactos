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

BOOL CRYPT_EncodeLen(DWORD len, BYTE *pbEncoded, DWORD *pcbEncoded);

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
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);

struct AsnConstructedItem
{
    BYTE                    tag;
    const void             *pvStructInfo;
    CryptEncodeObjectExFunc encodeFunc;
};

BOOL WINAPI CRYPT_AsnEncodeConstructed(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
BOOL WINAPI CRYPT_AsnEncodeOid(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
BOOL WINAPI CRYPT_AsnEncodeOctets(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);

typedef struct _CRYPT_DIGESTED_DATA
{
    DWORD                      version;
    CRYPT_ALGORITHM_IDENTIFIER DigestAlgorithm;
    CRYPT_CONTENT_INFO         ContentInfo;
    CRYPT_HASH_BLOB            hash;
} CRYPT_DIGESTED_DATA;

BOOL CRYPT_AsnEncodePKCSDigestedData(const CRYPT_DIGESTED_DATA *digestedData,
 void *pvData, DWORD *pcbData);

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
} CRYPT_SIGNED_INFO;

BOOL CRYPT_AsnEncodeCMSSignedInfo(CRYPT_SIGNED_INFO *, void *pvData,
 DWORD *pcbData);

BOOL CRYPT_AsnDecodeCMSSignedInfo(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, PCRYPT_DECODE_PARA pDecodePara,
 CRYPT_SIGNED_INFO *signedInfo, DWORD *pcbSignedInfo);

/* Helper function to check *pcbEncoded, set it to the required size, and
 * optionally to allocate memory.  Assumes pbEncoded is not NULL.
 * If CRYPT_ENCODE_ALLOC_FLAG is set in dwFlags, *pbEncoded will be set to a
 * pointer to the newly allocated memory.
 */
BOOL CRYPT_EncodeEnsureSpace(DWORD dwFlags, PCRYPT_ENCODE_PARA pEncodePara,
 BYTE *pbEncoded, DWORD *pcbEncoded, DWORD bytesNeeded);

BOOL CRYPT_AsnDecodePKCSDigestedData(const BYTE *pbEncoded, DWORD cbEncoded,
 DWORD dwFlags, PCRYPT_DECODE_PARA pDecodePara,
 CRYPT_DIGESTED_DATA *digestedData, DWORD *pcbDigestedData);

/* The following aren't defined in wincrypt.h, as they're "reserved" */
#define CERT_CERT_PROP_ID 32
#define CERT_CRL_PROP_ID  33
#define CERT_CTL_PROP_ID  34

/* Returns a handle to the default crypto provider; loads it if necessary.
 * Returns NULL on failure.
 */
HCRYPTPROV CRYPT_GetDefaultProvider(void);

HINSTANCE hInstance;

void crypt_oid_init(void);
void crypt_oid_free(void);
void crypt_sip_free(void);
void root_store_free(void);
void default_chain_engine_free(void);

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
typedef const void *(WINAPI *DuplicateContextFunc)(const void *context);
typedef const void *(WINAPI *EnumContextsInStoreFunc)(HCERTSTORE hCertStore,
 const void *pPrevContext);
typedef DWORD (WINAPI *EnumPropertiesFunc)(const void *context, DWORD dwPropId);
typedef BOOL (WINAPI *GetContextPropertyFunc)(const void *context,
 DWORD dwPropID, void *pvData, DWORD *pcbData);
typedef BOOL (WINAPI *SetContextPropertyFunc)(const void *context,
 DWORD dwPropID, DWORD dwFlags, const void *pvData);
typedef BOOL (WINAPI *SerializeElementFunc)(const void *context, DWORD dwFlags,
 BYTE *pbElement, DWORD *pcbElement);
typedef BOOL (WINAPI *FreeContextFunc)(const void *context);
typedef BOOL (WINAPI *DeleteContextFunc)(const void *context);

/* An abstract context (certificate, CRL, or CTL) interface */
typedef struct _WINE_CONTEXT_INTERFACE
{
    CreateContextFunc            create;
    AddContextToStoreFunc        addContextToStore;
    AddEncodedContextToStoreFunc addEncodedToStore;
    DuplicateContextFunc         duplicate;
    EnumContextsInStoreFunc      enumContextsInStore;
    EnumPropertiesFunc           enumProps;
    GetContextPropertyFunc       getProp;
    SetContextPropertyFunc       setProp;
    SerializeElementFunc         serialize;
    FreeContextFunc              free;
    DeleteContextFunc            deleteFromStore;
} WINE_CONTEXT_INTERFACE, *PWINE_CONTEXT_INTERFACE;
typedef const WINE_CONTEXT_INTERFACE *PCWINE_CONTEXT_INTERFACE;

extern PCWINE_CONTEXT_INTERFACE pCertInterface;
extern PCWINE_CONTEXT_INTERFACE pCRLInterface;
extern PCWINE_CONTEXT_INTERFACE pCTLInterface;

/* (Internal) certificate store types and functions */
struct WINE_CRYPTCERTSTORE;

typedef struct WINE_CRYPTCERTSTORE * (*StoreOpenFunc)(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara);

/* Called to enumerate the next context in a store. */
typedef void * (*EnumFunc)(struct WINE_CRYPTCERTSTORE *store, void *pPrev);

/* Called to add a context to a store.  If toReplace is not NULL,
 * context replaces toReplace in the store, and access checks should not be
 * performed.  Otherwise context is a new context, and it should only be
 * added if the store allows it.  If ppStoreContext is not NULL, the added
 * context should be returned in *ppStoreContext.
 */
typedef BOOL (*AddFunc)(struct WINE_CRYPTCERTSTORE *store, void *context,
 void *toReplace, const void **ppStoreContext);

typedef BOOL (*DeleteFunc)(struct WINE_CRYPTCERTSTORE *store, void *context);

typedef struct _CONTEXT_FUNCS
{
    AddFunc    addContext;
    EnumFunc   enumContext;
    DeleteFunc deleteContext;
} CONTEXT_FUNCS, *PCONTEXT_FUNCS;

typedef enum _CertStoreType {
    StoreTypeMem,
    StoreTypeCollection,
    StoreTypeProvider,
} CertStoreType;

struct _CONTEXT_PROPERTY_LIST;
typedef struct _CONTEXT_PROPERTY_LIST *PCONTEXT_PROPERTY_LIST;

#define WINE_CRYPTCERTSTORE_MAGIC 0x74726563

/* A cert store is polymorphic through the use of function pointers.  A type
 * is still needed to distinguish collection stores from other types.
 * On the function pointers:
 * - closeStore is called when the store's ref count becomes 0
 * - control is optional, but should be implemented by any store that supports
 *   persistence
 */
typedef struct WINE_CRYPTCERTSTORE
{
    DWORD                       dwMagic;
    LONG                        ref;
    DWORD                       dwOpenFlags;
    CertStoreType               type;
    PFN_CERT_STORE_PROV_CLOSE   closeStore;
    CONTEXT_FUNCS               certs;
    CONTEXT_FUNCS               crls;
    CONTEXT_FUNCS               ctls;
    PFN_CERT_STORE_PROV_CONTROL control; /* optional */
    PCONTEXT_PROPERTY_LIST      properties;
} WINECRYPT_CERTSTORE, *PWINECRYPT_CERTSTORE;

void CRYPT_InitStore(WINECRYPT_CERTSTORE *store, DWORD dwFlags,
 CertStoreType type);
void CRYPT_FreeStore(PWINECRYPT_CERTSTORE store);
BOOL WINAPI I_CertUpdateStore(HCERTSTORE store1, HCERTSTORE store2, DWORD unk0,
 DWORD unk1);

PWINECRYPT_CERTSTORE CRYPT_CollectionOpenStore(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara);
PWINECRYPT_CERTSTORE CRYPT_ProvCreateStore(DWORD dwFlags,
 PWINECRYPT_CERTSTORE memStore, const CERT_STORE_PROV_INFO *pProvInfo);
PWINECRYPT_CERTSTORE CRYPT_ProvOpenStore(LPCSTR lpszStoreProvider,
 DWORD dwEncodingType, HCRYPTPROV hCryptProv, DWORD dwFlags,
 const void *pvPara);
PWINECRYPT_CERTSTORE CRYPT_RegOpenStore(HCRYPTPROV hCryptProv, DWORD dwFlags,
 const void *pvPara);
PWINECRYPT_CERTSTORE CRYPT_FileOpenStore(HCRYPTPROV hCryptProv, DWORD dwFlags,
 const void *pvPara);
PWINECRYPT_CERTSTORE CRYPT_FileNameOpenStoreA(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara);
PWINECRYPT_CERTSTORE CRYPT_FileNameOpenStoreW(HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara);
PWINECRYPT_CERTSTORE CRYPT_RootOpenStore(HCRYPTPROV hCryptProv, DWORD dwFlags);

/* Allocates and initializes a certificate chain engine, but without creating
 * the root store.  Instead, it uses root, and assumes the caller has done any
 * checking necessary.
 */
HCERTCHAINENGINE CRYPT_CreateChainEngine(HCERTSTORE root,
 PCERT_CHAIN_ENGINE_CONFIG pConfig);

/* Helper function for store reading functions and
 * CertAddSerializedElementToStore.  Returns a context of the appropriate type
 * if it can, or NULL otherwise.  Doesn't validate any of the properties in
 * the serialized context (for example, bad hashes are retained.)
 * *pdwContentType is set to the type of the returned context.
 */
const void *CRYPT_ReadSerializedElement(const BYTE *pbElement,
 DWORD cbElement, DWORD dwContextTypeFlags, DWORD *pdwContentType);

/* Reads contexts serialized in the file into the memory store.  Returns FALSE
 * if the file is not of the expected format.
 */
BOOL CRYPT_ReadSerializedStoreFromFile(HANDLE file, HCERTSTORE store);

/* Fixes up the pointers in info, where info is assumed to be a
 * CRYPT_KEY_PROV_INFO, followed by its container name, provider name, and any
 * provider parameters, in a contiguous buffer, but where info's pointers are
 * assumed to be invalid.  Upon return, info's pointers point to the
 * appropriate memory locations.
 */
void CRYPT_FixKeyProvInfoPointers(PCRYPT_KEY_PROV_INFO info);

/**
 *  String functions
 */

DWORD cert_name_to_str_with_indent(DWORD dwCertEncodingType, DWORD indent,
 PCERT_NAME_BLOB pName, DWORD dwStrType, LPWSTR psz, DWORD csz);

/**
 *  Context functions
 */

/* Allocates a new data context, a context which owns properties directly.
 * contextSize is the size of the public data type associated with context,
 * which should be one of CERT_CONTEXT, CRL_CONTEXT, or CTL_CONTEXT.
 * Free with Context_Release.
 */
void *Context_CreateDataContext(size_t contextSize);

/* Creates a new link context with extra bytes.  The context refers to linked
 * rather than owning its own properties.  If addRef is TRUE (which ordinarily
 * it should be) linked is addref'd.
 * Free with Context_Release.
 */
void *Context_CreateLinkContext(unsigned int contextSize, void *linked, unsigned int extra,
 BOOL addRef);

/* Returns a pointer to the extra bytes allocated with context, which must be
 * a link context.
 */
void *Context_GetExtra(const void *context, size_t contextSize);

/* Gets the context linked to by context, which must be a link context. */
void *Context_GetLinkedContext(void *context, size_t contextSize);

/* Copies properties from fromContext to toContext. */
void Context_CopyProperties(const void *to, const void *from,
 size_t contextSize);

/* Returns context's properties, or the linked context's properties if context
 * is a link context.
 */
PCONTEXT_PROPERTY_LIST Context_GetProperties(const void *context, size_t contextSize);

void Context_AddRef(void *context, size_t contextSize);

typedef void (*ContextFreeFunc)(void *context);

/* Decrements context's ref count.  If context is a link context, releases its
 * linked context as well.
 * If a data context has its ref count reach 0, calls dataContextFree on it.
 */
void Context_Release(void *context, size_t contextSize,
 ContextFreeFunc dataContextFree);

/**
 *  Context property list functions
 */

PCONTEXT_PROPERTY_LIST ContextPropertyList_Create(void);

/* Searches for the property with ID id in the context.  Returns TRUE if found,
 * and copies the property's length and a pointer to its data to blob.
 * Otherwise returns FALSE.
 */
BOOL ContextPropertyList_FindProperty(PCONTEXT_PROPERTY_LIST list, DWORD id,
 PCRYPT_DATA_BLOB blob);

BOOL ContextPropertyList_SetProperty(PCONTEXT_PROPERTY_LIST list, DWORD id,
 const BYTE *pbData, size_t cbData);

void ContextPropertyList_RemoveProperty(PCONTEXT_PROPERTY_LIST list, DWORD id);

DWORD ContextPropertyList_EnumPropIDs(PCONTEXT_PROPERTY_LIST list, DWORD id);

void ContextPropertyList_Copy(PCONTEXT_PROPERTY_LIST to,
 PCONTEXT_PROPERTY_LIST from);

void ContextPropertyList_Free(PCONTEXT_PROPERTY_LIST list);

/**
 *  Context list functions.  A context list is a simple list of link contexts.
 */
struct ContextList;

struct ContextList *ContextList_Create(
 PCWINE_CONTEXT_INTERFACE contextInterface, size_t contextSize);

void *ContextList_Add(struct ContextList *list, void *toLink, void *toReplace);

void *ContextList_Enum(struct ContextList *list, void *pPrev);

void ContextList_Delete(struct ContextList *list, void *context);

void ContextList_Free(struct ContextList *list);

/**
 *  Utilities.
 */

/* Align up to a DWORD_PTR boundary
 */
#define ALIGN_DWORD_PTR(x) (((x) + sizeof(DWORD_PTR) - 1) & ~(sizeof(DWORD_PTR) - 1))
#define POINTER_ALIGN_DWORD_PTR(p) ((LPVOID)ALIGN_DWORD_PTR((DWORD_PTR)(p)))

#endif
