/*
 * Copyright (C) 2002 Travis Michielsen
 * Copyright (C) 2004-2005 Juan Lang
 * Copyright (C) 2007 Vijay Kiran Kamuju
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

#ifndef __WINE_WINCRYPT_H
#define __WINE_WINCRYPT_H

#include <specstrings.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4201)
#endif

#include <bcrypt.h>
/* FIXME: #include <ncrypt.h> */

#ifdef _ADVAPI32_
# define WINADVAPI
#else
# define WINADVAPI DECLSPEC_IMPORT
#endif

/* some typedefs for function parameters */
typedef unsigned int ALG_ID;
typedef ULONG_PTR HCRYPTPROV;
typedef ULONG_PTR HCRYPTPROV_OR_NCRYPT_KEY_HANDLE;
typedef ULONG_PTR HCRYPTPROV_LEGACY;
typedef ULONG_PTR HCRYPTKEY;
typedef ULONG_PTR HCRYPTHASH;
typedef void *HCERTSTORE;
typedef void *HCRYPTMSG;
typedef void *HCERTSTOREPROV;
typedef void *HCRYPTOIDFUNCSET;
typedef void *HCRYPTOIDFUNCADDR;
typedef void *HCRYPTDEFAULTCONTEXT;

/* CSP Structs */

typedef struct _PROV_ENUMALGS {
  ALG_ID aiAlgid;
  DWORD  dwBitLen;
  DWORD  dwNameLen;
  CHAR   szName[20];
} PROV_ENUMALGS;

typedef struct _PROV_ENUMALGS_EX {
  ALG_ID aiAlgid;
  DWORD  dwDefaultLen;
  DWORD  dwMinLen;
  DWORD  dwMaxLen;
  DWORD  dwProtocols;
  DWORD  dwNameLen;
  CHAR   szName[20];
  DWORD  dwLongNameLen;
  CHAR   szLongName[40];
} PROV_ENUMALGS_EX;

#define SCHANNEL_MAC_KEY 0
#define SCHANNEL_ENC_KEY 1

typedef struct _SCHANNEL_ALG {
  DWORD  dwUse;
  ALG_ID Algid;
  DWORD  cBits;
  DWORD  dwFlags;
  DWORD  dwReserved;
} SCHANNEL_ALG, *PSCHANNEL_ALG;


#define CRYPT_IPSEC_HMAC_KEY 0x0100

typedef struct _HMAC_INFO {
  ALG_ID HashAlgid;
  BYTE*  pbInnerString;
  DWORD  cbInnerString;
  BYTE*  pbOuterString;
  DWORD  cbOuterString;
} HMAC_INFO, *PHMAC_INFO;
		
typedef struct _CRYPTOAPI_BLOB {
  DWORD    cbData;
  BYTE*    pbData;
} CRYPT_INTEGER_BLOB,  *PCRYPT_INTEGER_BLOB,
  CRYPT_UINT_BLOB,     *PCRYPT_UINT_BLOB,
  CRYPT_OBJID_BLOB,    *PCRYPT_OBJID_BLOB,
  CERT_NAME_BLOB,      *PCERT_NAME_BLOB,
  CERT_RDN_VALUE_BLOB, *PCERT_RDN_VALUE_BLOB,
  CERT_BLOB,           *PCERT_BLOB,
  CRL_BLOB,            *PCRL_BLOB,
  DATA_BLOB,           *PDATA_BLOB,
  CRYPT_DATA_BLOB,     *PCRYPT_DATA_BLOB,
  CRYPT_HASH_BLOB,     *PCRYPT_HASH_BLOB,
  CRYPT_DIGEST_BLOB,   *PCRYPT_DIGEST_BLOB,
  CRYPT_DER_BLOB,      *PCRYPT_DER_BLOB,
  CRYPT_ATTR_BLOB,     *PCRYPT_ATTR_BLOB;

typedef struct _CRYPTPROTECT_PROMPTSTRUCT{
  DWORD   cbSize;
  DWORD   dwPromptFlags;
  HWND    hwndApp;
  LPCWSTR szPrompt;
} CRYPTPROTECT_PROMPTSTRUCT, *PCRYPTPROTECT_PROMPTSTRUCT;

typedef struct _CRYPT_ALGORITHM_IDENTIFIER {
  LPSTR            pszObjId;
  CRYPT_OBJID_BLOB Parameters;
} CRYPT_ALGORITHM_IDENTIFIER, *PCRYPT_ALGORITHM_IDENTIFIER;

typedef struct _CRYPT_ATTRIBUTE_TYPE_VALUE {
  LPSTR               pszObjId;
  CRYPT_OBJID_BLOB    Value;
} CRYPT_ATTRIBUTE_TYPE_VALUE, *PCRYPT_ATTRIBUTE_TYPE_VALUE;

typedef struct _PUBLICKEYSTRUC {
    BYTE   bType;
    BYTE   bVersion;
    WORD   reserved;
    ALG_ID aiKeyAlg;
} BLOBHEADER, PUBLICKEYSTRUC;

typedef struct _RSAPUBKEY {
    DWORD   magic;
    DWORD   bitlen;
    DWORD   pubexp;
} RSAPUBKEY;

typedef struct _PUBKEY {
    DWORD   magic;
    DWORD   bitlen;
} DHPUBKEY, DSSPUBKEY, KEAPUBKEY, TEKPUBKEY;

typedef struct _DSSSEED {
    DWORD   counter;
    BYTE    seed[20];
} DSSSEED;

typedef struct _PUBKEYVER3 {
    DWORD   magic;
    DWORD   bitlenP;
    DWORD   bitlenQ;
    DWORD   bitlenJ;
    DSSSEED DSSSeed;
} DHPUBKEY_VER3, DSSPUBKEY_VER3;

typedef struct _PRIVKEYVER3 {
    DWORD   magic;
    DWORD   bitlenP;
    DWORD   bitlenQ;
    DWORD   bitlenJ;
    DWORD   bitlenX;
    DSSSEED DSSSeed;
} DHPRIVKEY_VER3, DSSPRIVKEY_VER3;

typedef struct _KEY_TYPE_SUBTYPE {
    DWORD   dwKeySpec;
    GUID    Type;
    GUID    SubType;
} KEY_TYPE_SUBTYPE, *PKEY_TYPE_SUBTYPE;

typedef struct _CERT_FORTEZZA_DATA_PROP {
    unsigned char   SerialNumber[8];
    int             CertIndex;
    unsigned char   CertLabel[36];
} CERT_FORTEZZA_DATA_PROP;

typedef struct _CMS_DH_KEY_INFO {
    DWORD             dwVersion;
    ALG_ID            Algid;
    LPSTR             pszContentEncObjId;
    CRYPT_DATA_BLOB   PubInfo;
    void              *pReserved;
} CMS_DH_KEY_INFO, *PCMS_DH_KEY_INFO;

typedef struct _CRYPT_BIT_BLOB {
    DWORD cbData;
    BYTE  *pbData;
    DWORD cUnusedBits;
} CRYPT_BIT_BLOB, *PCRYPT_BIT_BLOB;

typedef struct _CRYPT_KEY_PROV_PARAM {
    DWORD dwParam;
    BYTE *pbData;
    DWORD cbData;
    DWORD dwFlags;
} CRYPT_KEY_PROV_PARAM, *PCRYPT_KEY_PROV_PARAM;

typedef struct _CRYPT_KEY_PROV_INFO {
    LPWSTR                pwszContainerName;
    LPWSTR                pwszProvName;
    DWORD                 dwProvType;
    DWORD                 dwFlags;
    DWORD                 cProvParam;
    PCRYPT_KEY_PROV_PARAM rgProvParam;
    DWORD                 dwKeySpec;
} CRYPT_KEY_PROV_INFO, *PCRYPT_KEY_PROV_INFO;

typedef struct _CERT_KEY_CONTEXT {
    DWORD      cbSize;
    HCRYPTPROV hCryptProv;
    DWORD      dwKeySpec;
} CERT_KEY_CONTEXT, *PCERT_KEY_CONTEXT;

typedef struct _CERT_PUBLIC_KEY_INFO {
    CRYPT_ALGORITHM_IDENTIFIER Algorithm;
    CRYPT_BIT_BLOB             PublicKey;
} CERT_PUBLIC_KEY_INFO, *PCERT_PUBLIC_KEY_INFO;

typedef struct _CERT_EXTENSION {
    LPSTR               pszObjId;
    BOOL                fCritical;
    CRYPT_OBJID_BLOB    Value;
} CERT_EXTENSION, *PCERT_EXTENSION;

typedef struct _CERT_EXTENSIONS {
    DWORD           cExtension;
    PCERT_EXTENSION rgExtension;
} CERT_EXTENSIONS, *PCERT_EXTENSIONS;

typedef struct _CERT_INFO {
    DWORD                      dwVersion;
    CRYPT_INTEGER_BLOB         SerialNumber;
    CRYPT_ALGORITHM_IDENTIFIER SignatureAlgorithm;
    CERT_NAME_BLOB             Issuer;
    FILETIME                   NotBefore;
    FILETIME                   NotAfter;
    CERT_NAME_BLOB             Subject;
    CERT_PUBLIC_KEY_INFO       SubjectPublicKeyInfo;
    CRYPT_BIT_BLOB             IssuerUniqueId;
    CRYPT_BIT_BLOB             SubjectUniqueId;
    DWORD                      cExtension;
    PCERT_EXTENSION            rgExtension;
} CERT_INFO, *PCERT_INFO;

typedef struct _CERT_RDN_ATTR {
    LPSTR               pszObjId;
    DWORD               dwValueType;
    CERT_RDN_VALUE_BLOB Value;
} CERT_RDN_ATTR, *PCERT_RDN_ATTR;

typedef struct _CERT_RDN {
    DWORD          cRDNAttr;
    PCERT_RDN_ATTR rgRDNAttr;
} CERT_RDN, *PCERT_RDN;

typedef struct _CERT_NAME_INFO {
    DWORD     cRDN;
    PCERT_RDN rgRDN;
} CERT_NAME_INFO, *PCERT_NAME_INFO;

typedef struct _CERT_NAME_VALUE {
    DWORD               dwValueType;
    CERT_RDN_VALUE_BLOB Value;
} CERT_NAME_VALUE, *PCERT_NAME_VALUE;

typedef struct _CERT_ENCRYPTED_PRIVATE_KEY_INFO {
    CRYPT_ALGORITHM_IDENTIFIER EncryptionAlgorithm;
    CRYPT_DATA_BLOB            EncryptedPrivateKey;
} CERT_ENCRYPTED_PRIVATE_KEY_INFO, *PCERT_ENCRYPTED_PRIVATE_KEY_INFO;

typedef struct _CERT_AUTHORITY_KEY_ID_INFO {
    CRYPT_DATA_BLOB    KeyId;
    CERT_NAME_BLOB     CertIssuer;
    CRYPT_INTEGER_BLOB CertSerialNumber;
} CERT_AUTHORITY_KEY_ID_INFO, *PCERT_AUTHORITY_KEY_ID_INFO;

typedef struct _CERT_PRIVATE_KEY_VALIDITY {
    FILETIME NotBefore;
    FILETIME NotAfter;
} CERT_PRIVATE_KEY_VALIDITY, *PCERT_PRIVATE_KEY_VALIDITY;

typedef struct _CERT_KEY_ATTRIBUTES_INFO {
    CRYPT_DATA_BLOB            KeyId;
    CRYPT_BIT_BLOB             IntendedKeyUsage;
    PCERT_PRIVATE_KEY_VALIDITY pPrivateKeyUsagePeriod;
} CERT_KEY_ATTRIBUTES_INFO, *PCERT_KEY_ATTRIBUTES_INFO;

/* byte 0 */
#define CERT_DIGITAL_SIGNATURE_KEY_USAGE 0x80
#define CERT_NON_REPUDIATION_KEY_USAGE   0x40
#define CERT_KEY_ENCIPHERMENT_KEY_USAGE  0x20
#define CERT_DATA_ENCIPHERMENT_KEY_USAGE 0x10
#define CERT_KEY_AGREEMENT_KEY_USAGE     0x08
#define CERT_KEY_CERT_SIGN_KEY_USAGE     0x04
#define CERT_OFFLINE_CRL_SIGN_KEY_USAGE  0x02
#define CERT_CRL_SIGN_KEY_USAGE          0x02
#define CERT_ENCIPHER_ONLY_KEY_USAGE     0x01
/* byte 1 */
#define CERT_DECIPHER_ONLY_KEY_USAGE     0x80

typedef struct _CERT_POLICY_ID {
    DWORD  cCertPolicyElementId;
    LPSTR *rgbszCertPolicyElementId;
} CERT_POLICY_ID, *PCERT_POLICY_ID;

typedef struct _CERT_KEY_USAGE_RESTRICTION_INFO {
    DWORD           cCertPolicyId;
    PCERT_POLICY_ID rgCertPolicyId;
    CRYPT_BIT_BLOB  RestrictedKeyUsage;
} CERT_KEY_USAGE_RESTRICTION_INFO, *PCERT_KEY_USAGE_RESTRICTION_INFO;

typedef struct _CERT_OTHER_NAME {
    LPSTR            pszObjId;
    CRYPT_OBJID_BLOB Value;
} CERT_OTHER_NAME, *PCERT_OTHER_NAME;

typedef struct _CERT_ALT_NAME_ENTRY {
    DWORD dwAltNameChoice;
    union {
        PCERT_OTHER_NAME pOtherName;
        LPWSTR           pwszRfc822Name;
        LPWSTR           pwszDNSName;
        CERT_NAME_BLOB   DirectoryName;
        LPWSTR           pwszURL;
        CRYPT_DATA_BLOB  IPAddress;
        LPSTR            pszRegisteredID;
    } DUMMYUNIONNAME;
} CERT_ALT_NAME_ENTRY, *PCERT_ALT_NAME_ENTRY;

#define CERT_ALT_NAME_OTHER_NAME     1
#define CERT_ALT_NAME_RFC822_NAME    2
#define CERT_ALT_NAME_DNS_NAME       3
#define CERT_ALT_NAME_X400_ADDRESS   4
#define CERT_ALT_NAME_DIRECTORY_NAME 5
#define CERT_ALT_NAME_EDI_PARTY_NAME 6
#define CERT_ALT_NAME_URL            7
#define CERT_ALT_NAME_IP_ADDRESS     8
#define CERT_ALT_NAME_REGISTERED_ID  9

typedef struct _CERT_ALT_NAME_INFO {
    DWORD                cAltEntry;
    PCERT_ALT_NAME_ENTRY rgAltEntry;
} CERT_ALT_NAME_INFO, *PCERT_ALT_NAME_INFO;

#define CERT_ALT_NAME_ENTRY_ERR_INDEX_MASK  0xff
#define CERT_ALT_NAME_ENTRY_ERR_INDEX_SHIFT 16
#define CERT_ALT_NAME_VALUE_ERR_INDEX_MASK  0x0000ffff
#define CERT_ALT_NAME_VALUE_ERR_INDEX_SHIFT 0
#define GET_CERT_ALT_NAME_ENTRY_ERR_INDEX(x) \
 (((x) >> CERT_ALT_NAME_ENTRY_ERR_INDEX_SHIFT) & \
  CERT_ALT_NAME_ENTRY_ERR_INDEX_MASK)
#define GET_CERT_ALT_NAME_VALUE_ERR_INDEX(x) \
 ((x) & CERT_ALT_NAME_VALUE_ERR_INDEX_MASK)

typedef struct _CERT_BASIC_CONSTRAINTS_INFO {
    CRYPT_BIT_BLOB  SubjectType;
    BOOL            fPathLenConstraint;
    DWORD           dwPathLenConstraint;
    DWORD           cSubtreesConstraint;
    CERT_NAME_BLOB *rgSubtreesConstraint;
} CERT_BASIC_CONSTRAINTS_INFO, *PCERT_BASIC_CONSTRAINTS_INFO;

#define CERT_CA_SUBJECT_FLAG         0x80
#define CERT_END_ENTITY_SUBJECT_FLAG 0x40

typedef struct _CERT_BASIC_CONSTRAINTS2_INFO {
    BOOL  fCA;
    BOOL  fPathLenConstraint;
    DWORD dwPathLenConstraint;
} CERT_BASIC_CONSTRAINTS2_INFO, *PCERT_BASIC_CONSTRAINTS2_INFO;

typedef struct _CERT_POLICY_QUALIFIER_INFO {
    LPSTR            pszPolicyQualifierId;
    CRYPT_OBJID_BLOB Qualifier;
} CERT_POLICY_QUALIFIER_INFO, *PCERT_POLICY_QUALIFIER_INFO;

typedef struct _CERT_POLICY_INFO {
    LPSTR                       pszPolicyIdentifier;
    DWORD                       cPolicyQualifier;
    CERT_POLICY_QUALIFIER_INFO *rgPolicyQualifier;
} CERT_POLICY_INFO, *PCERT_POLICY_INFO;

typedef struct _CERT_POLICIES_INFO {
    DWORD             cPolicyInfo;
    CERT_POLICY_INFO *rgPolicyInfo;
} CERT_POLICIES_INFO, *PCERT_POLICIES_INFO;

typedef struct _CERT_POLICY_QUALIFIER_NOTICE_REFERENCE {
    LPSTR pszOrganization;
    DWORD cNoticeNumbers;
    int  *rgNoticeNumbers;
} CERT_POLICY_QUALIFIER_NOTICE_REFERENCE,
 *PCERT_POLICY_QUALIFIER_NOTICE_REFERENCE;

typedef struct _CERT_POLICY_QUALIFIER_USER_NOTICE {
    CERT_POLICY_QUALIFIER_NOTICE_REFERENCE *pNoticeReference;
    LPWSTR                                  pszDisplayText;
} CERT_POLICY_QUALIFIER_USER_NOTICE, *PCERT_POLICY_QUALIFIER_USER_NOTICE;

typedef struct _CPS_URLS {
    LPWSTR                      pszURL;
    CRYPT_ALGORITHM_IDENTIFIER *pAlgorithm;
    CRYPT_DATA_BLOB            *pDigest;
} CPS_URLS, *PCPS_URLS;

typedef struct _CERT_POLICY95_QUALIFIER1 {
    LPWSTR    pszPracticesReference;
    LPSTR     pszNoticeIdentifier;
    LPSTR     pszNSINoticeIdentifier;
    DWORD     cCPSURLs;
    CPS_URLS *rgCPSURLs;
} CERT_POLICY95_QUALIFIER1, *PCERT_POLICY95_QUALIFIER1;

typedef struct _CERT_POLICY_MAPPING {
    LPSTR pszIssuerDomainPolicy;
    LPSTR pszSubjectDomainPolicy;
} CERT_POLICY_MAPPING, *PCERT_POLICY_MAPPING;

typedef struct _CERT_POLICY_MAPPINGS_INFO {
    DWORD                cPolicyMapping;
    PCERT_POLICY_MAPPING rgPolicyMapping;
} CERT_POLICY_MAPPINGS_INFO, *PCERT_POLICY_MAPPINGS_INFO;

typedef struct _CERT_POLICY_CONSTRAINTS_INFO {
    BOOL  fRequireExplicitPolicy;
    DWORD dwRequireExplicitPolicySkipCerts;
    BOOL  fInhibitPolicyMapping;
    DWORD dwInhibitPolicyMappingSkipCerts;
} CERT_POLICY_CONSTRAINTS_INFO, *PCERT_POLICY_CONSTRAINTS_INFO;

typedef struct _CRYPT_CONTENT_INFO_SEQUENCE_OF_ANY {
    LPSTR           pszObjId;
    DWORD           cValue;
    PCRYPT_DER_BLOB rgValue;
} CRYPT_CONTENT_INFO_SEQUENCE_OF_ANY, *PCRYPT_CONTENT_INFO_SEQUENCE_OF_ANY;

typedef struct _CRYPT_CONTENT_INFO {
    LPSTR          pszObjId;
    CRYPT_DER_BLOB Content;
} CRYPT_CONTENT_INFO, *PCRYPT_CONTENT_INFO;

typedef struct _CRYPT_SEQUENCE_OF_ANY {
    DWORD           cValue;
    PCRYPT_DER_BLOB rgValue;
} CRYPT_SEQUENCE_OF_ANY, *PCRYPT_SEQUENCE_OF_ANY;

typedef struct _CERT_AUTHORITY_KEY_ID2_INFO {
    CRYPT_DATA_BLOB    KeyId;
    CERT_ALT_NAME_INFO AuthorityCertIssuer;
    CRYPT_INTEGER_BLOB AuthorityCertSerialNumber;
} CERT_AUTHORITY_KEY_ID2_INFO, *PCERT_AUTHORITY_KEY_ID2_INFO;

typedef struct _CERT_ACCESS_DESCRIPTION {
    LPSTR               pszAccessMethod;
    CERT_ALT_NAME_ENTRY AccessLocation;
} CERT_ACCESS_DESCRIPTION, *PCERT_ACCESS_DESCRIPTION;

typedef struct _CERT_AUTHORITY_INFO_ACCESS {
    DWORD                    cAccDescr;
    PCERT_ACCESS_DESCRIPTION rgAccDescr;
} CERT_AUTHORITY_INFO_ACCESS, *PCERT_AUTHORITY_INFO_ACCESS;

typedef struct _CERT_CONTEXT {
    DWORD      dwCertEncodingType;
    BYTE       *pbCertEncoded;
    DWORD      cbCertEncoded;
    PCERT_INFO pCertInfo;
    HCERTSTORE hCertStore;
} CERT_CONTEXT, *PCERT_CONTEXT;
typedef const CERT_CONTEXT *PCCERT_CONTEXT;

typedef struct _CRL_ENTRY {
    CRYPT_INTEGER_BLOB SerialNumber;
    FILETIME           RevocationDate;
    DWORD              cExtension;
    PCERT_EXTENSION    rgExtension;
} CRL_ENTRY, *PCRL_ENTRY;

typedef struct _CRL_INFO {
    DWORD           dwVersion;
    CRYPT_ALGORITHM_IDENTIFIER SignatureAlgorithm;
    CERT_NAME_BLOB  Issuer;
    FILETIME        ThisUpdate;
    FILETIME        NextUpdate;
    DWORD           cCRLEntry;
    PCRL_ENTRY      rgCRLEntry;
    DWORD           cExtension;
    PCERT_EXTENSION rgExtension;
} CRL_INFO, *PCRL_INFO;

typedef struct _CRL_DIST_POINT_NAME {
    DWORD dwDistPointNameChoice;
    union {
        CERT_ALT_NAME_INFO FullName;
    } DUMMYUNIONNAME;
} CRL_DIST_POINT_NAME, *PCRL_DIST_POINT_NAME;

#define CRL_DIST_POINT_NO_NAME         0
#define CRL_DIST_POINT_FULL_NAME       1
#define CRL_DIST_POINT_ISSUER_RDN_NAME 2

typedef struct _CRL_DIST_POINT {
    CRL_DIST_POINT_NAME DistPointName;
    CRYPT_BIT_BLOB      ReasonFlags;
    CERT_ALT_NAME_INFO  CRLIssuer;
} CRL_DIST_POINT, *PCRL_DIST_POINT;

#define CRL_REASON_UNUSED_FLAG                 0x80
#define CRL_REASON_KEY_COMPROMISE_FLAG         0x40
#define CRL_REASON_CA_COMPROMISE_FLAG          0x20
#define CRL_REASON_AFFILIATION_CHANGED_FLAG    0x10
#define CRL_REASON_SUPERSEDED_FLAG             0x08
#define CRL_REASON_CESSATION_OF_OPERATION_FLAG 0x04
#define CRL_REASON_CERTIFICATE_HOLD_FLAG       0x02

typedef struct _CRL_DIST_POINTS_INFO {
    DWORD           cDistPoint;
    PCRL_DIST_POINT rgDistPoint;
} CRL_DIST_POINTS_INFO, *PCRL_DIST_POINTS_INFO;

#define CRL_DIST_POINT_ERR_INDEX_MASK  0x7f
#define CRL_DIST_POINT_ERR_INDEX_SHIFT 24
#define GET_CRL_DIST_POINT_ERR_INDEX(x) \
 (((x) >> CRL_DIST_POINT_ERR_INDEX_SHIFT) & CRL_DIST_POINT_ERR_INDEX_MASK)

#define CRL_DIST_POINT_ERR_CRL_ISSUER_BIT 0x80000000L
#define IS_CRL_DIST_POINT_ERR_CRL_ISSUER(x) \
 ((x) & CRL_DIST_POINT_ERR_CRL_ISSUER_BIT)

typedef struct _CROSS_CERT_DIST_POINTS_INFO {
    DWORD               dwSyncDeltaTime;
    DWORD               cDistPoint;
    PCERT_ALT_NAME_INFO rgDistPoint;
} CROSS_CERT_DIST_POINTS_INFO, *PCROSS_CERT_DIST_POINTS_INFO;

#define CROSS_CERT_DIST_POINT_ERR_INDEX_MASK  0xff
#define CROSS_CERT_DIST_POINT_ERR_INDEX_SHIFT 24
#define GET_CROSS_CERT_DIST_POINT_ERR_INDEX(x) \
 (((x) >> CROSS_CERT_DIST_POINT_ERR_INDEX_SHIFT) & \
 CROSS_CERT_DIST_POINT_ERR_INDEX_MASK)

typedef struct _CERT_PAIR {
    CERT_BLOB Forward;
    CERT_BLOB Reverse;
} CERT_PAIR, *PCERT_PAIR;

typedef struct _CRL_ISSUING_DIST_POINT {
    CRL_DIST_POINT_NAME DistPointName;
    BOOL                fOnlyContainsUserCerts;
    BOOL                fOnlyContainsCACerts;
    CRYPT_BIT_BLOB      OnlySomeReasonFlags;
    BOOL                fIndirectCRL;
} CRL_ISSUING_DIST_POINT, *PCRL_ISSUING_DIST_POINT;

typedef struct _CERT_GENERAL_SUBTREE {
    CERT_ALT_NAME_ENTRY Base;
    DWORD               dwMinimum;
    BOOL                fMaximum;
    DWORD               dwMaximum;
} CERT_GENERAL_SUBTREE, *PCERT_GENERAL_SUBTREE;

typedef struct _CERT_NAME_CONSTRAINTS_INFO {
    DWORD                 cPermittedSubtree;
    PCERT_GENERAL_SUBTREE rgPermittedSubtree;
    DWORD                 cExcludedSubtree;
    PCERT_GENERAL_SUBTREE rgExcludedSubtree;
} CERT_NAME_CONSTRAINTS_INFO, *PCERT_NAME_CONSTRAINTS_INFO;

#define CERT_EXCLUDED_SUBTREE_BIT 0x80000000L
#define IS_CERT_EXCLUDED_SUBTREE(x) ((x) & CERT_EXCLUDED_SUBTREE_BIT)

typedef struct _CRYPT_ATTRIBUTE {
    LPSTR            pszObjId;
    DWORD            cValue;
    PCRYPT_DATA_BLOB rgValue;
} CRYPT_ATTRIBUTE, *PCRYPT_ATTRIBUTE;

typedef struct _CRYPT_ATTRIBUTES {
    DWORD            cAttr;
    PCRYPT_ATTRIBUTE rgAttr;
} CRYPT_ATTRIBUTES, *PCRYPT_ATTRIBUTES;

typedef struct _CERT_REQUEST_INFO {
    DWORD                dwVersion;
    CERT_NAME_BLOB       Subject;
    CERT_PUBLIC_KEY_INFO SubjectPublicKeyInfo;
    DWORD                cAttribute;
    PCRYPT_ATTRIBUTE     rgAttribute;
} CERT_REQUEST_INFO, *PCERT_REQUEST_INFO;

typedef struct _CERT_KEYGEN_REQUEST_INFO {
    DWORD                dwVersion;
    CERT_PUBLIC_KEY_INFO SubjectPublicKeyInfo;
    LPWSTR               pwszChallengeString;
} CERT_KEYGEN_REQUEST_INFO, *PCERT_KEYGEN_REQUEST_INFO;

typedef struct _CERT_SIGNED_CONTENT_INFO {
    CRYPT_DER_BLOB             ToBeSigned;
    CRYPT_ALGORITHM_IDENTIFIER SignatureAlgorithm;
    CRYPT_BIT_BLOB             Signature;
} CERT_SIGNED_CONTENT_INFO, *PCERT_SIGNED_CONTENT_INFO;

typedef struct _CRL_CONTEXT {
    DWORD      dwCertEncodingType;
    BYTE      *pbCrlEncoded;
    DWORD      cbCrlEncoded;
    PCRL_INFO  pCrlInfo;
    HCERTSTORE hCertStore;
} CRL_CONTEXT, *PCRL_CONTEXT;
typedef const CRL_CONTEXT *PCCRL_CONTEXT;

#define SORTED_CTL_EXT_FLAGS_OFFSET                (0*4)
#define SORTED_CTL_EXT_COUNT_OFFSET                (1*4)
#define SORTED_CTL_EXT_MAX_COLLISION_OFFSET        (2*4)
#define SORTED_CTL_EXT_HASH_BUCKET_OFFSET          (3*4)

#define SORTED_CTL_EXT_HASHED_SUBJECT_IDENTIFIER_FLAG    0x1

typedef struct _CERT_DSS_PARAMETERS {
    CRYPT_UINT_BLOB    p;
    CRYPT_UINT_BLOB    q;
    CRYPT_UINT_BLOB    g;
} CERT_DSS_PARAMETERS, *PCERT_DSS_PARAMETERS;

#define CERT_DSS_R_LEN            20
#define CERT_DSS_S_LEN            20
#define CERT_DSS_SIGNATURE_LEN    (CERT_DSS_R_LEN + CERT_DSS_S_LEN)

#define CERT_MAX_ENCODED_DSS_SIGNATURE_LEN    (2 + 2*(2 + 20 +1))

typedef struct _CERT_DH_PARAMETERS {
    CRYPT_UINT_BLOB    p;
    CRYPT_UINT_BLOB    g;
} CERT_DH_PARAMETERS, *PCERT_DH_PARAMETERS;

typedef struct _CERT_X942_DH_VALIDATION_PARAMS {
    CRYPT_BIT_BLOB     seed;
    DWORD              pgenCounter;
} CERT_X942_DH_VALIDATION_PARAMS, *PCERT_X942_DH_VALIDATION_PARAMS;

typedef struct _CERT_X942_DH_PARAMETERS {
    CRYPT_UINT_BLOB                    p;
    CRYPT_UINT_BLOB                    g;
    CRYPT_UINT_BLOB                    q;
    CRYPT_UINT_BLOB                    j;
    PCERT_X942_DH_VALIDATION_PARAMS    pValidationParams;
} CERT_X942_DH_PARAMETERS, *PCERT_X942_DH_PARAMETERS;

#define CRYPT_X942_COUNTER_BYTE_LENGTH        4
#define CRYPT_X942_KEY_LENGTH_BYTE_LENGTH     4
#define CRYPT_X942_PUB_INFO_BYTE_LENGTH       (512/8)

typedef struct _CRYPT_X942_OTHER_INFO {
    LPSTR              pszContentEncryptionObjId;
    BYTE               rgbCounter[CRYPT_X942_COUNTER_BYTE_LENGTH];
    BYTE               rgbKeyLength[CRYPT_X942_KEY_LENGTH_BYTE_LENGTH];
    CRYPT_DATA_BLOB    PubInfo;
} CRYPT_X942_OTHER_INFO, *PCRYPT_X942_OTHER_INFO;

typedef struct _CRYPT_RC2_CBC_PARAMETERS {
    DWORD    dwVersion;
    BOOL     fIV;
    BYTE     rgbIV[4];
} CRYPT_RC2_CBC_PARAMETERS, *PCRYPT_RC2_CBC_PARAMETERS;

#define CRYPT_RC2_40BIT_VERSION    160
#define CRYPT_RC2_56BIT_VERSION    52
#define CRYPT_RC2_64BIT_VERSION    120
#define CRYPT_RC2_128BIT_VERSION   58

typedef struct _CRYPT_SMIME_CAPABILITY {
    LPSTR               pszObjId;
    CRYPT_OBJID_BLOB    Parameters;
} CRYPT_SMIME_CAPABILITY, *PCRYPT_SMIME_CAPABILITY;

typedef struct _CRYPT_SMIME_CAPABILITIES {
    DWORD                   cCapability;
    PCRYPT_SMIME_CAPABILITY rgCapability;
} CRYPT_SMIME_CAPABILITIES, *PCRYPT_SMIME_CAPABILITIES;

typedef struct _VTableProvStruc {
    DWORD    Version;
#ifdef WINE_STRICT_PROTOTYPES
    BOOL     (WINAPI *FuncVerifyImage)(LPCSTR,BYTE*);
    void     (WINAPI *FuncReturnhWnd)(HWND*);
#else
    FARPROC  FuncVerifyImage;
    FARPROC  FuncReturnhWnd;
#endif
    DWORD    dwProvType;
    BYTE    *pbContextInfo;
    DWORD    cbContextInfo;
    LPSTR    pszProvName;
} VTableProvStruc, *PVTableProvStruc;

typedef struct _CERT_PRIVATE_KEY_INFO {
    DWORD                      Version;
    CRYPT_ALGORITHM_IDENTIFIER Algorithm;
    CRYPT_DER_BLOB             PrivateKey;
    PCRYPT_ATTRIBUTES          pAttributes;
} CERT_PRIVATE_KEY_INFO, *PCERT_PRIVATE_KEY_INFO;

typedef struct _CTL_USAGE {
    DWORD  cUsageIdentifier;
    LPSTR *rgpszUsageIdentifier;
} CTL_USAGE, *PCTL_USAGE, CERT_ENHKEY_USAGE, *PCERT_ENHKEY_USAGE;

typedef struct _CTL_ENTRY {
    CRYPT_DATA_BLOB  SubjectIdentifier;
    DWORD            cAttribute;
    PCRYPT_ATTRIBUTE rgAttribute;
} CTL_ENTRY, *PCTL_ENTRY;

typedef struct _CTL_INFO {
    DWORD                      dwVersion;
    CTL_USAGE                  SubjectUsage;
    CRYPT_DATA_BLOB            ListIdentifier;
    CRYPT_INTEGER_BLOB         SequenceNumber;
    FILETIME                   ThisUpdate;
    FILETIME                   NextUpdate;
    CRYPT_ALGORITHM_IDENTIFIER SubjectAlgorithm;
    DWORD                      cCTLEntry;
    PCTL_ENTRY                 rgCTLEntry;
    DWORD                      cExtension;
    PCERT_EXTENSION            rgExtension;
} CTL_INFO, *PCTL_INFO;

typedef struct _CTL_CONTEXT {
    DWORD      dwMsgAndCertEncodingType;
    BYTE      *pbCtlEncoded;
    DWORD      cbCtlEncoded;
    PCTL_INFO  pCtlInfo;
    HCERTSTORE hCertStore;
    HCRYPTMSG  hCryptMsg;
    BYTE      *pbCtlContext;
    DWORD      cbCtlContext;
} CTL_CONTEXT, *PCTL_CONTEXT;
typedef const CTL_CONTEXT *PCCTL_CONTEXT;

typedef struct _CRYPT_TIME_STAMP_REQUEST_INFO {
    LPSTR            pszTimeStampAlgorithm;
    LPSTR            pszContentType;
    CRYPT_OBJID_BLOB Content;
    DWORD            cAttribute;
    PCRYPT_ATTRIBUTE rgAttribute;
} CRYPT_TIME_STAMP_REQUEST_INFO, *PCRYPT_TIME_STAMP_REQUEST_INFO;

typedef struct _CRYPT_ENROLLMENT_NAME_VALUE_PAIR {
    LPWSTR pwszName;
    LPWSTR pwszValue;
} CRYPT_ENROLLMENT_NAME_VALUE_PAIR, *PCRYPT_ENROLLMENT_NAME_VALUE_PAIR;

typedef struct _CMSG_SIGNER_INFO {
    DWORD                      dwVersion;
    CERT_NAME_BLOB             Issuer;
    CRYPT_INTEGER_BLOB         SerialNumber;
    CRYPT_ALGORITHM_IDENTIFIER HashAlgorithm;
    CRYPT_ALGORITHM_IDENTIFIER HashEncryptionAlgorithm;
    CRYPT_DATA_BLOB            EncryptedHash;
    CRYPT_ATTRIBUTES           AuthAttrs;
    CRYPT_ATTRIBUTES           UnauthAttrs;
} CMSG_SIGNER_INFO, *PCMSG_SIGNER_INFO;

#define CMSG_VERIFY_SIGNER_PUBKEY 1
#define CMSG_VERIFY_SIGNER_CERT   2
#define CMSG_VERIFY_SIGNER_CHAIN  3
#define CMSG_VERIFY_SIGNER_NULL   4

typedef struct _CERT_REVOCATION_CRL_INFO {
    DWORD         cbSize;
    PCCRL_CONTEXT pBaseCrlContext;
    PCCRL_CONTEXT pDeltaCrlContext;
    PCRL_ENTRY    pCrlEntry;
    BOOL          fDeltaCrlEntry;
} CERT_REVOCATION_CRL_INFO, *PCERT_REVOCATION_CRL_INFO;

typedef struct _CERT_REVOCATION_INFO {
    DWORD                     cbSize;
    DWORD                     dwRevocationResult;
    LPCSTR                    pszRevocationOid;
    LPVOID                    pvOidSpecificInfo;
    BOOL                      fHasFreshnessTime;
    DWORD                     dwFreshnessTime;
    PCERT_REVOCATION_CRL_INFO pCrlInfo;
} CERT_REVOCATION_INFO, *PCERT_REVOCATION_INFO;

typedef struct _CERT_REVOCATION_PARA {
    DWORD                     cbSize;
    PCCERT_CONTEXT            pIssuerCert;
    DWORD                     cCertStore;
    HCERTSTORE               *rgCertStore;
    HCERTSTORE                hCrlStore;
    LPFILETIME                pftTimeToUse;
#ifdef CERT_REVOCATION_PARA_HAS_EXTRA_FIELDS
    DWORD                     dwUrlRetrievalTimeout;
    BOOL                      fCheckFreshnessTime;
    DWORD                     dwFreshnessTime;
    LPFILETIME                pftCurrentTime;
    PCERT_REVOCATION_CRL_INFO pCrlInfo;
#endif
} CERT_REVOCATION_PARA, *PCERT_REVOCATION_PARA;

#define CERT_CONTEXT_REVOCATION_TYPE 1
#define CERT_VERIFY_REV_CHAIN_FLAG                0x00000001
#define CERT_VERIFY_CACHE_ONLY_BASED_REVOCATION   0x00000002
#define CERT_VERIFY_REV_ACCUMULATIVE_TIMEOUT_FLAG 0x00000004

typedef struct _CTL_VERIFY_USAGE_PARA {
    DWORD           cbSize;
    CRYPT_DATA_BLOB ListIdentifier;
    DWORD           cCtlStore;
    HCERTSTORE     *rghCtlStore;
    DWORD           cSignerStore;
    HCERTSTORE     *rghSignerStore;
} CTL_VERIFY_USAGE_PARA, *PCTL_VERIFY_USAGE_PARA;

typedef struct _CTL_VERIFY_USAGE_STATUS {
    DWORD           cbSize;
    DWORD           dwError;
    DWORD           dwFlags;
    PCCTL_CONTEXT  *ppCtl;
    DWORD           dwCtlEntryIndex;
    PCCERT_CONTEXT *ppSigner;
    DWORD           dwSignerIndex;
} CTL_VERIFY_USAGE_STATUS, *PCTL_VERIFY_USAGE_STATUS;

#define CERT_VERIFY_INHIBIT_CTL_UPDATE_FLAG 0x1
#define CERT_VERIFY_TRUSTED_SIGNERS_FLAG    0x2
#define CERT_VERIFY_NO_TIME_CHECK_FLAG      0x4
#define CERT_VERIFY_ALLOW_MORE_USAGE_FLAG   0x8
#define CERT_VERIFY_UPDATED_CTL_FLAG        0x1

typedef struct _CERT_CHAIN {
    DWORD               cCerts;
    PCERT_BLOB          certs;
    CRYPT_KEY_PROV_INFO keyLocatorInfo;
} CERT_CHAIN, *PCERT_CHAIN;

typedef struct _CERT_REVOCATION_STATUS {
    DWORD cbSize;
    DWORD dwIndex;
    DWORD dwError;
    DWORD dwReason;
    BOOL  fHasFreshnessTime;
    DWORD dwFreshnessTime;
} CERT_REVOCATION_STATUS, *PCERT_REVOCATION_STATUS;

typedef struct _CERT_TRUST_LIST_INFO {
    DWORD         cbSize;
    PCTL_ENTRY    pCtlEntry;
    PCCTL_CONTEXT pCtlContext;
} CERT_TRUST_LIST_INFO, *PCERT_TRUST_LIST_INFO;

#define CERT_TRUST_NO_ERROR                          0x00000000
#define CERT_TRUST_IS_NOT_TIME_VALID                 0x00000001
#define CERT_TRUST_IS_NOT_TIME_NESTED                0x00000002
#define CERT_TRUST_IS_REVOKED                        0x00000004
#define CERT_TRUST_IS_NOT_SIGNATURE_VALID            0x00000008
#define CERT_TRUST_IS_NOT_VALID_FOR_USAGE            0x00000010
#define CERT_TRUST_IS_UNTRUSTED_ROOT                 0x00000020
#define CERT_TRUST_REVOCATION_STATUS_UNKNOWN         0x00000040
#define CERT_TRUST_IS_CYCLIC                         0x00000080
#define CERT_TRUST_INVALID_EXTENSION                 0x00000100
#define CERT_TRUST_INVALID_POLICY_CONSTRAINTS        0x00000200
#define CERT_TRUST_INVALID_BASIC_CONSTRAINTS         0x00000400
#define CERT_TRUST_INVALID_NAME_CONSTRAINTS          0x00000800
#define CERT_TRUST_HAS_NOT_SUPPORTED_NAME_CONSTRAINT 0x00001000
#define CERT_TRUST_HAS_NOT_DEFINED_NAME_CONSTRAINT   0x00002000
#define CERT_TRUST_HAS_NOT_PERMITTED_NAME_CONSTRAINT 0x00004000
#define CERT_TRUST_HAS_EXCLUDED_NAME_CONSTRAINT      0x00008000
#define CERT_TRUST_IS_OFFLINE_REVOCATION             0x01000000
#define CERT_TRUST_NO_ISSUANCE_CHAIN_POLICY          0x02000000
#define CERT_TRUST_IS_EXPLICIT_DISTRUST              0x04000000
#define CERT_TRUST_HAS_NOT_SUPPORTED_CRITICAL_EXT    0x08000000

#define CERT_TRUST_IS_PARTIAL_CHAIN                  0x00010000
#define CERT_TRUST_CTL_IS_NOT_TIME_VALID             0x00020000
#define CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID        0x00040000
#define CERT_TRUST_CTL_IS_NOT_VALID_FOR_USAGE        0x00080000

#define CERT_TRUST_HAS_EXACT_MATCH_ISSUER            0x00000001
#define CERT_TRUST_HAS_KEY_MATCH_ISSUER              0x00000002
#define CERT_TRUST_HAS_NAME_MATCH_ISSUER             0x00000004
#define CERT_TRUST_IS_SELF_SIGNED                    0x00000008

#define CERT_TRUST_HAS_PREFERRED_ISSUER              0x00000100
#define CERT_TRUST_HAS_ISSUANCE_CHAIN_POLICY         0x00000200
#define CERT_TRUST_HAS_VALID_NAME_CONSTRAINTS        0x00000400
#define CERT_TRUST_IS_PEER_TRUSTED                   0x00000800
#define CERT_TRUST_HAS_CRL_VALIDITY_EXTENDED         0x00001000

#define CERT_TRUST_IS_COMPLEX_CHAIN                  0x00010000

typedef struct _CERT_TRUST_STATUS {
    DWORD dwErrorStatus;
    DWORD dwInfoStatus;
} CERT_TRUST_STATUS, *PCERT_TRUST_STATUS;

typedef struct _CERT_CHAIN_ELEMENT {
    DWORD                 cbSize;
    PCCERT_CONTEXT        pCertContext;
    CERT_TRUST_STATUS     TrustStatus;
    PCERT_REVOCATION_INFO pRevocationInfo;
    PCERT_ENHKEY_USAGE    pIssuanceUsage;
    PCERT_ENHKEY_USAGE    pApplicationUsage;
    LPCWSTR               pwszExtendedErrorInfo;
} CERT_CHAIN_ELEMENT, *PCERT_CHAIN_ELEMENT;

typedef struct _CERT_SIMPLE_CHAIN {
    DWORD                 cbSize;
    CERT_TRUST_STATUS     TrustStatus;
    DWORD                 cElement;
    PCERT_CHAIN_ELEMENT  *rgpElement;
    PCERT_TRUST_LIST_INFO pTrustListInfo;
    BOOL                  fHasRevocationFreshnessTime;
    DWORD                 dwRevocationFreshnessTime;
} CERT_SIMPLE_CHAIN, *PCERT_SIMPLE_CHAIN;

typedef struct _CERT_CHAIN_CONTEXT CERT_CHAIN_CONTEXT, *PCERT_CHAIN_CONTEXT;
typedef const CERT_CHAIN_CONTEXT *PCCERT_CHAIN_CONTEXT;

struct _CERT_CHAIN_CONTEXT {
    DWORD                 cbSize;
    CERT_TRUST_STATUS     TrustStatus;
    DWORD                 cChain;
    PCERT_SIMPLE_CHAIN   *rgpChain;
    DWORD                 cLowerQualityChainContext;
    PCCERT_CHAIN_CONTEXT *rgpLowerQualityChainContext;
    BOOL                  fHasRevocationFreshnessTime;
    DWORD                 dwRevocationFreshnessTime;
};

typedef struct _CERT_CHAIN_POLICY_PARA {
    DWORD cbSize;
    DWORD dwFlags;
    void *pvExtraPolicyPara;
} CERT_CHAIN_POLICY_PARA, *PCERT_CHAIN_POLICY_PARA;

typedef struct _CERT_CHAIN_POLICY_STATUS {
    DWORD cbSize;
    DWORD dwError;
    LONG  lChainIndex;
    LONG  lElementIndex;
    void *pvExtraPolicyStatus;
} CERT_CHAIN_POLICY_STATUS, *PCERT_CHAIN_POLICY_STATUS;

#define CERT_CHAIN_POLICY_BASE              ((LPCSTR)1)
#define CERT_CHAIN_POLICY_AUTHENTICODE      ((LPCSTR)2)
#define CERT_CHAIN_POLICY_AUTHENTICODE_TS   ((LPCSTR)3)
#define CERT_CHAIN_POLICY_SSL               ((LPCSTR)4)
#define CERT_CHAIN_POLICY_BASIC_CONSTRAINTS ((LPCSTR)5)
#define CERT_CHAIN_POLICY_NT_AUTH           ((LPCSTR)6)
#define CERT_CHAIN_POLICY_MICROSOFT_ROOT    ((LPCSTR)7)

#define CERT_CHAIN_POLICY_IGNORE_NOT_TIME_VALID_FLAG            0x00000001
#define CERT_CHAIN_POLICY_IGNORE_CTL_NOT_TIME_VALID_FLAG        0x00000002
#define CERT_CHAIN_POLICY_IGNORE_NOT_TIME_NESTED_FLAG           0x00000004
#define CERT_CHAIN_POLICY_IGNORE_INVALID_BASIC_CONSTRAINTS_FLAG 0x00000008

#define CERT_CHAIN_POLICY_IGNORE_ALL_NOT_TIME_VALID_FLAGS ( \
 CERT_CHAIN_POLICY_IGNORE_NOT_TIME_VALID_FLAG \
 CERT_CHAIN_POLICY_IGNORE_CTL_NOT_TIME_VALID_FLAG \
 CERT_CHAIN_POLICY_IGNORE_NOT_TIME_NESTED_FLAG )

#define CERT_CHAIN_POLICY_ALLOW_UNKNOWN_CA_FLAG                 0x00000010
#define CERT_CHAIN_POLICY_IGNORE_WRONG_USAGE_FLAG               0x00000020
#define CERT_CHAIN_POLICY_IGNORE_INVALID_NAME_FLAG              0x00000040
#define CERT_CHAIN_POLICY_IGNORE_INVALID_POLICY_FLAG            0x00000080

#define CERT_CHAIN_POLICY_IGNORE_END_REV_UNKNOWN_FLAG           0x00000100
#define CERT_CHAIN_POLICY_IGNORE_CTL_SIGNER_REV_UNKNOWN_FLAG    0x00000200
#define CERT_CHAIN_POLICY_IGNORE_CA_REV_UNKNOWN_FLAG            0x00000400
#define CERT_CHAIN_POLICY_IGNORE_ROOT_REV_UNKNOWN_FLAG          0x00000800

#define CERT_CHAIN_POLICY_IGNORE_ALL_REV_UNKNOWN_FLAGS ( \
 CERT_CHAIN_POLICY_IGNORE_END_REV_UNKNOWN_FLAG \
 CERT_CHAIN_POLICY_IGNORE_CTL_SIGNER_REV_UNKNOWN_FLAG \
 CERT_CHAIN_POLICY_IGNORE_CA_REV_UNKNOWN_FLAG \
 CERT_CHAIN_POLICY_IGNORE_ROOT_REV_UNKNOWN_FLAG )

#define CERT_CHAIN_POLICY_IGNORE_PEER_TRUST_FLAG                 0x00001000
#define CERT_CHAIN_POLICY_IGNORE_NOT_SUPPORTED_CRITICAL_EXT_FLAG 0x00002000
#define CERT_CHAIN_POLICY_TRUST_TESTROOT_FLAG                    0x00004000
#define CERT_CHAIN_POLICY_ALLOW_TESTROOT_FLAG                    0x00008000
#define MICROSOFT_ROOT_CERT_CHAIN_POLICY_ENABLE_TEST_ROOT_FLAG   0x00010000

typedef struct _AUTHENTICODE_EXTRA_CERT_CHAIN_POLICY_PARA {
    DWORD             cbSize;
    DWORD             dwRegPolicySettings;
    PCMSG_SIGNER_INFO pSignerInfo;
} AUTHENTICODE_EXTRA_CERT_CHAIN_POLICY_PARA,
 *PAUTHENTICODE_EXTRA_CERT_CHAIN_POLICY_PARA;

typedef struct _AUTHENTICODE_EXTRA_CERT_CHAIN_POLICY_STATUS {
    DWORD cbSize;
    BOOL  fCommercial;
} AUTHENTICODE_EXTRA_CERT_CHAIN_POLICY_STATUS,
 *PAUTHENTICODE_EXTRA_CERT_CHAIN_POLICY_STATUS;

typedef struct _AUTHENTICODE_TS_EXTRA_CERT_CHAIN_POLICY_PARA {
    DWORD cbSize;
    DWORD dwRegPolicySettings;
    BOOL  fCommercial;
} AUTHENTICODE_TS_EXTRA_CERT_CHAIN_POLICY_PARA,
 *PAUTHENTICODE_TS_EXTRA_CERT_CHAIN_POLICY_PARA;

typedef struct _HTTPSPolicyCallbackData {
    union {
        DWORD cbStruct;
        DWORD cbSize;
    } DUMMYUNIONNAME;
    DWORD  dwAuthType;
    DWORD  fdwChecks;
    WCHAR *pwszServerName;
} HTTPSPolicyCallbackData, *PHTTPSPolicyCallbackData,
 SSL_EXTRA_CERT_CHAIN_POLICY_PARA, *PSSL_EXTRA_CERT_CHAIN_POLICY_PARA;

/* Values for HTTPSPolicyCallbackData's dwAuthType */
#define AUTHTYPE_CLIENT 1
#define AUTHTYPE_SERVER 2
/* Values for HTTPSPolicyCallbackData's fdwChecks are defined in wininet.h */

#define BASIC_CONSTRAINTS_CERT_CHAIN_POLICY_CA_FLAG         0x80000000
#define BASIC_CONSTRAINTS_CERT_CHAIN_POLICY_END_ENTITY_FLAG 0x40000000

#define MICROSOFT_ROOT_CERT_CHAIN_POLICY_ENABLE_TEST_ROOT_FLAG 0x00010000

#define USAGE_MATCH_TYPE_AND 0x00000000
#define USAGE_MATCH_TYPE_OR  0x00000001

typedef struct _CERT_USAGE_MATCH {
    DWORD             dwType;
    CERT_ENHKEY_USAGE Usage;
} CERT_USAGE_MATCH, *PCERT_USAGE_MATCH;

typedef struct _CTL_USAGE_MATCH {
    DWORD     dwType;
    CTL_USAGE Usage;
} CTL_USAGE_MATCH, *PCTL_USAGE_MATCH;

#define CERT_CHAIN_REVOCATION_CHECK_END_CERT           0x10000000
#define CERT_CHAIN_REVOCATION_CHECK_CHAIN              0x20000000
#define CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT 0x40000000
#define CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY         0x80000000

#define CERT_CHAIN_REVOCATION_ACCUMULATIVE_TIMEOUT     0x08000000

#define CERT_CHAIN_DISABLE_PASS1_QUALITY_FILTERING     0x00000040
#define CERT_CHAIN_RETURN_LOWER_QUALITY_CONTEXTS       0x00000080
#define CERT_CHAIN_DISABLE_AUTH_ROOT_AUTO_UPDATE       0x00000100
#define CERT_CHAIN_TIMESTAMP_TIME                      0x00000200

typedef struct _CERT_CHAIN_PARA {
    DWORD            cbSize;
    CERT_USAGE_MATCH RequestedUsage;
#ifdef CERT_CHAIN_PARA_HAS_EXTRA_FIELDS
    CERT_USAGE_MATCH RequestedIssuancePolicy;
    DWORD            dwUrlRetrievalTimeout;
    BOOL             fCheckRevocationFreshnessTime;
    DWORD            dwRevocationFreshnessTime;
    LPFILETIME       pftCacheResync;
#endif
} CERT_CHAIN_PARA, *PCERT_CHAIN_PARA;

typedef struct _CERT_SYSTEM_STORE_INFO {
    DWORD cbSize;
} CERT_SYSTEM_STORE_INFO, *PCERT_SYSTEM_STORE_INFO;

typedef struct _CERT_PHYSICAL_STORE_INFO {
    DWORD           cbSize;
    LPSTR           pszOpenStoreProvider;
    DWORD           dwOpenEncodingType;
    DWORD           dwOpenFlags;
    CRYPT_DATA_BLOB OpenParameters;
    DWORD           dwFlags;
    DWORD           dwPriority;
} CERT_PHYSICAL_STORE_INFO, *PCERT_PHYSICAL_STORE_INFO;

typedef struct _CERT_SYSTEM_STORE_RELOCATE_PARA {
    union {
        HKEY  hKeyBase;
        VOID *pvBase;
    } DUMMYUNIONNAME;
    union {
        void   *pvSystemStore;
        LPCSTR  pszSystemStore;
        LPCWSTR pwszSystemStore;
    } DUMMYUNIONNAME2;
} CERT_SYSTEM_STORE_RELOCATE_PARA, *PCERT_SYSTEM_STORE_RELOCATE_PARA;

typedef BOOL
(WINAPI *PFN_CERT_ENUM_SYSTEM_STORE_LOCATION)(
  _In_ LPCWSTR pwszStoreLocation,
  _In_ DWORD dwFlags,
  _Reserved_ void *pvReserved,
  _Inout_opt_ void *pvArg);

typedef BOOL
(WINAPI *PFN_CERT_ENUM_SYSTEM_STORE)(
  _In_ const void *pvSystemStore,
  _In_ DWORD dwFlags,
  _In_ PCERT_SYSTEM_STORE_INFO pStoreInfo,
  _Reserved_ void *pvReserved,
  _Inout_opt_ void *pvArg);

typedef BOOL
(WINAPI *PFN_CERT_ENUM_PHYSICAL_STORE)(
  _In_ const void *pvSystemStore,
  _In_ DWORD dwFlags,
  _In_ LPCWSTR pwszStoreName,
  _In_ PCERT_PHYSICAL_STORE_INFO pStoreInfo,
  _Reserved_ void *pvReserved,
  _Inout_opt_ void *pvArg);

/* Encode/decode object */
typedef LPVOID (__WINE_ALLOC_SIZE(1) WINAPI *PFN_CRYPT_ALLOC)(_In_ size_t cbsize);
typedef VOID(WINAPI *PFN_CRYPT_FREE)(_In_ LPVOID pv);

typedef struct _CRYPT_ENCODE_PARA {
    DWORD           cbSize;
    PFN_CRYPT_ALLOC pfnAlloc;
    PFN_CRYPT_FREE  pfnFree;
} CRYPT_ENCODE_PARA, *PCRYPT_ENCODE_PARA;

typedef struct _CRYPT_DECODE_PARA {
    DWORD           cbSize;
    PFN_CRYPT_ALLOC pfnAlloc;
    PFN_CRYPT_FREE  pfnFree;
} CRYPT_DECODE_PARA, *PCRYPT_DECODE_PARA;

typedef struct _CERT_STORE_PROV_INFO {
    DWORD             cbSize;
    DWORD             cStoreProvFunc;
    void            **rgpvStoreProvFunc;
    HCERTSTOREPROV    hStoreProv;
    DWORD             dwStoreProvFlags;
    HCRYPTOIDFUNCADDR hStoreProvFuncAddr2;
} CERT_STORE_PROV_INFO, *PCERT_STORE_PROV_INFO;

typedef BOOL
(WINAPI *PFN_CERT_DLL_OPEN_STORE_PROV_FUNC)(
  _In_ LPCSTR lpszStoreProvider,
  _In_ DWORD dwEncodingType,
  _In_opt_ HCRYPTPROV_LEGACY hCryptProv,
  _In_ DWORD dwFlags,
  _In_opt_ const void *pvPara,
  _In_ HCERTSTORE hCertStore,
  _Inout_ PCERT_STORE_PROV_INFO pStoreProvInfo);

typedef void
(WINAPI *PFN_CERT_STORE_PROV_CLOSE)(
  _Inout_opt_ HCERTSTOREPROV hStoreProv,
  _In_ DWORD dwFlags);

typedef
_Success_(return != 0)
BOOL
(WINAPI *PFN_CERT_STORE_PROV_READ_CERT)(
  _Inout_ HCERTSTOREPROV hStoreProv,
  _In_ PCCERT_CONTEXT pStoreCertContext,
  _In_ DWORD dwFlags,
  _Outptr_ PCCERT_CONTEXT *ppProvCertContext);

typedef BOOL
(WINAPI *PFN_CERT_STORE_PROV_WRITE_CERT)(
  _Inout_ HCERTSTOREPROV hStoreProv,
  _In_ PCCERT_CONTEXT pCertContext,
  _In_ DWORD dwFlags);

typedef BOOL
(WINAPI *PFN_CERT_STORE_PROV_DELETE_CERT)(
  _Inout_ HCERTSTOREPROV hStoreProv,
  _In_ PCCERT_CONTEXT pCertContext,
  _In_ DWORD dwFlags);

typedef BOOL
(WINAPI *PFN_CERT_STORE_PROV_SET_CERT_PROPERTY)(
  _Inout_ HCERTSTOREPROV hStoreProv,
  _In_ PCCERT_CONTEXT pCertContext,
  _In_ DWORD dwPropId,
  _In_ DWORD dwFlags,
  _In_opt_ const void *pvData);

typedef
_Success_(return != 0)
BOOL
(WINAPI *PFN_CERT_STORE_PROV_READ_CRL)(
  _Inout_ HCERTSTOREPROV hStoreProv,
  _In_ PCCRL_CONTEXT pStoreCrlContext,
  _In_ DWORD dwFlags,
  _Outptr_ PCCRL_CONTEXT *ppProvCrlContext);

typedef BOOL
(WINAPI *PFN_CERT_STORE_PROV_WRITE_CRL)(
  _Inout_ HCERTSTOREPROV hStoreProv,
  _In_ PCCRL_CONTEXT pCrlContext,
  _In_ DWORD dwFlags);

typedef BOOL
(WINAPI *PFN_CERT_STORE_PROV_DELETE_CRL)(
  _Inout_ HCERTSTOREPROV hStoreProv,
  _In_ PCCRL_CONTEXT pCrlContext,
  _In_ DWORD dwFlags);

typedef BOOL
(WINAPI *PFN_CERT_STORE_PROV_SET_CRL_PROPERTY)(
  _Inout_ HCERTSTOREPROV hStoreProv,
  _In_ PCCRL_CONTEXT pCrlContext,
  _In_ DWORD dwPropId,
  _In_ DWORD dwFlags,
  _In_opt_ const void *pvData);

typedef
_Success_(return != 0)
BOOL
(WINAPI *PFN_CERT_STORE_PROV_READ_CTL)(
  _Inout_ HCERTSTOREPROV hStoreProv,
  _In_ PCCTL_CONTEXT pStoreCtlContext,
  _In_ DWORD dwFlags,
  _Outptr_ PCCTL_CONTEXT *ppProvCtlContext);

typedef BOOL
(WINAPI *PFN_CERT_STORE_PROV_WRITE_CTL)(
  _Inout_ HCERTSTOREPROV hStoreProv,
  _In_ PCCTL_CONTEXT pCtlContext,
  _In_ DWORD dwFlags);

typedef BOOL
(WINAPI *PFN_CERT_STORE_PROV_DELETE_CTL)(
  _Inout_ HCERTSTOREPROV hStoreProv,
  _In_ PCCTL_CONTEXT pCtlContext,
  _In_ DWORD dwFlags);

typedef BOOL
(WINAPI *PFN_CERT_STORE_PROV_SET_CTL_PROPERTY)(
  _Inout_ HCERTSTOREPROV hStoreProv,
  _In_ PCCTL_CONTEXT pCtlContext,
  _In_ DWORD dwPropId,
  _In_ DWORD dwFlags,
  _In_opt_ const void *pvData);

typedef BOOL
(WINAPI *PFN_CERT_STORE_PROV_CONTROL)(
  _Inout_ HCERTSTOREPROV hStoreProv,
  _In_ DWORD dwFlags,
  _In_ DWORD dwCtrlType,
  _In_opt_ void const *pvCtrlPara);

typedef struct _CERT_STORE_PROV_FIND_INFO {
    DWORD       cbSize;
    DWORD       dwMsgAndCertEncodingType;
    DWORD       dwFindFlags;
    DWORD       dwFindType;
    const void *pvFindPara;
} CERT_STORE_PROV_FIND_INFO, *PCERT_STORE_PROV_FIND_INFO;
typedef const CERT_STORE_PROV_FIND_INFO CCERT_STORE_PROV_FIND_INFO,
 *PCCERT_STORE_PROV_FIND_INFO;

typedef
_Success_(return != 0)
BOOL
(WINAPI *PFN_CERT_STORE_PROV_FIND_CERT)(
  _Inout_ HCERTSTOREPROV hStoreProv,
  _In_ PCCERT_STORE_PROV_FIND_INFO pFindInfo,
  _In_ PCCERT_CONTEXT pPrevCertContext,
  _In_ DWORD dwFlags,
  _Inout_ void **ppvStoreProvFindInfo,
  _Outptr_ PCCERT_CONTEXT *ppProvCertContext);

typedef BOOL
(WINAPI *PFN_CERT_STORE_PROV_FREE_FIND_CERT)(
  _Inout_ HCERTSTOREPROV hStoreProv,
  _In_ PCCERT_CONTEXT pCertContext,
  _In_ void *pvStoreProvFindInfo,
  _In_ DWORD dwFlags);

typedef BOOL
(WINAPI *PFN_CERT_STORE_PROV_GET_CERT_PROPERTY)(
  _Inout_ HCERTSTOREPROV hStoreProv,
  _In_ PCCERT_CONTEXT pCertContext,
  _In_ DWORD dwPropId,
  _In_ DWORD dwFlags,
  _Out_writes_bytes_to_opt_(*pcbData, *pcbData) void *pvData,
  _Inout_ DWORD *pcbData);

typedef
_Success_(return != 0)
BOOL
(WINAPI *PFN_CERT_STORE_PROV_FIND_CRL)(
  _Inout_ HCERTSTOREPROV hStoreProv,
  _In_ PCCERT_STORE_PROV_FIND_INFO pFindInfo,
  _In_ PCCRL_CONTEXT pPrevCrlContext,
  _In_ DWORD dwFlags,
  _Inout_ void **ppvStoreProvFindInfo,
  _Outptr_ PCCRL_CONTEXT *ppProvCrlContext);

typedef BOOL
(WINAPI *PFN_CERT_STORE_PROV_FREE_FIND_CRL)(
  _Inout_ HCERTSTOREPROV hStoreProv,
  _In_ PCCRL_CONTEXT pCrlContext,
  _In_ void *pvStoreProvFindInfo,
  _In_ DWORD dwFlags);

typedef BOOL
(WINAPI *PFN_CERT_STORE_PROV_GET_CRL_PROPERTY)(
  _Inout_ HCERTSTOREPROV hStoreProv,
  _In_ PCCRL_CONTEXT pCrlContext,
  _In_ DWORD dwPropId,
  _In_ DWORD dwFlags,
  _Out_writes_bytes_to_opt_(*pcbData, *pcbData) void *pvData,
  _Inout_ DWORD *pcbData);

typedef
_Success_(return != 0)
BOOL
(WINAPI *PFN_CERT_STORE_PROV_FIND_CTL)(
  _In_ HCERTSTOREPROV hStoreProv,
  _In_ PCCERT_STORE_PROV_FIND_INFO pFindInfo,
  _In_ PCCTL_CONTEXT pPrevCtlContext,
  _In_ DWORD dwFlags,
  _Inout_ void **ppvStoreProvFindInfo,
  _Outptr_ PCCTL_CONTEXT *ppProvCtlContext);

typedef BOOL
(WINAPI *PFN_CERT_STORE_PROV_GET_CTL_PROPERTY)(
  _Inout_ HCERTSTOREPROV hStoreProv,
  _In_ PCCTL_CONTEXT pCtlContext,
  _In_ DWORD dwPropId,
  _In_ DWORD dwFlags,
  _Out_writes_bytes_to_opt_(*pcbData, *pcbData) void *pvData,
  _Inout_ DWORD *pcbData);

typedef struct _CERT_CREATE_CONTEXT_PARA {
    DWORD          cbSize;
    PFN_CRYPT_FREE pfnFree;
    void          *pvFree;
} CERT_CREATE_CONTEXT_PARA, *PCERT_CREATE_CONTEXT_PARA;

typedef struct _CRYPT_OID_FUNC_ENTRY {
    LPCSTR pszOID;
    void  *pvFuncAddr;
} CRYPT_OID_FUNC_ENTRY, *PCRYPT_OID_FUNC_ENTRY;

typedef BOOL
(WINAPI *PFN_CRYPT_ENUM_OID_FUNC)(
  _In_ DWORD dwEncodingType,
  _In_ LPCSTR pszFuncName,
  _In_ LPCSTR pszOID,
  _In_ DWORD cValue,
  _In_reads_(cValue) const DWORD rgdwValueType[],
  _In_reads_(cValue) LPCWSTR const rgpwszValueName[],
  _In_reads_(cValue) const BYTE * const rgpbValueData[],
  _In_reads_(cValue) const DWORD rgcbValueData[],
  _Inout_opt_ void *pvArg);

#define CRYPT_MATCH_ANY_ENCODING_TYPE 0xffffffff

typedef struct _CRYPT_OID_INFO {
    DWORD   cbSize;
    LPCSTR  pszOID;
    LPCWSTR pwszName;
    DWORD   dwGroupId;
    union {
        DWORD  dwValue;
        ALG_ID Algid;
        DWORD  dwLength;
    } DUMMYUNIONNAME;
    CRYPT_DATA_BLOB ExtraInfo;
} CRYPT_OID_INFO, *PCRYPT_OID_INFO;
typedef const CRYPT_OID_INFO CCRYPT_OID_INFO, *PCCRYPT_OID_INFO;

typedef BOOL
(WINAPI *PFN_CRYPT_ENUM_OID_INFO)(
  _In_ PCCRYPT_OID_INFO pInfo,
  _Inout_opt_ void *pvArg);

typedef struct _CRYPT_SIGN_MESSAGE_PARA {
    DWORD                      cbSize;
    DWORD                      dwMsgEncodingType;
    PCCERT_CONTEXT             pSigningCert;
    CRYPT_ALGORITHM_IDENTIFIER HashAlgorithm;
    void *                     pvHashAuxInfo;
    DWORD                      cMsgCert;
    PCCERT_CONTEXT            *rgpMsgCert;
    DWORD                      cMsgCrl;
    PCCRL_CONTEXT             *rgpMsgCrl;
    DWORD                      cAuthAttr;
    PCRYPT_ATTRIBUTE           rgAuthAttr;
    DWORD                      cUnauthAttr;
    PCRYPT_ATTRIBUTE           rgUnauthAttr;
    DWORD                      dwFlags;
    DWORD                      dwInnerContentType;
#ifdef CRYPT_SIGN_MESSAGE_PARA_HAS_CMS_FIELDS
    CRYPT_ALGORITHM_IDENTIFIER HashEncryptionAlgorithm;
    void *                     pvHashEncryptionAuxInfo;
#endif
} CRYPT_SIGN_MESSAGE_PARA, *PCRYPT_SIGN_MESSAGE_PARA;

#define CRYPT_MESSAGE_BARE_CONTENT_OUT_FLAG         0x00000001
#define CRYPT_MESSAGE_ENCAPSULATED_CONTENT_OUT_FLAG 0x00000002
#define CRYPT_MESSAGE_KEYID_SIGNER_FLAG             0x00000004
#define CRYPT_MESSAGE_SILENT_KEYSET_FLAG            0x00000008

typedef PCCERT_CONTEXT
(WINAPI *PFN_CRYPT_GET_SIGNER_CERTIFICATE)(
  _Inout_opt_ void *pvGetArg,
  _In_ DWORD dwCertEncodingType,
  _In_ PCERT_INFO pSignerId,
  _In_ HCERTSTORE hMsgCertStore);

typedef struct _CRYPT_VERIFY_MESSAGE_PARA {
    DWORD                            cbSize;
    DWORD                            dwMsgAndCertEncodingType;
    HCRYPTPROV_LEGACY                hCryptProv;
    PFN_CRYPT_GET_SIGNER_CERTIFICATE pfnGetSignerCertificate;
    void *                           pvGetArg;
} CRYPT_VERIFY_MESSAGE_PARA, *PCRYPT_VERIFY_MESSAGE_PARA;

typedef struct _CRYPT_ENCRYPT_MESSAGE_PARA {
    DWORD                      cbSize;
    DWORD                      dwMsgEncodingType;
    HCRYPTPROV_LEGACY          hCryptProv;
    CRYPT_ALGORITHM_IDENTIFIER ContentEncryptionAlgorithm;
    void *                     pvEncryptionAuxInfo;
    DWORD                      dwFlags;
    DWORD                      dwInnerContentType;
} CRYPT_ENCRYPT_MESSAGE_PARA, *PCRYPT_ENCRYPT_MESSAGE_PARA;

#define CRYPT_MESSAGE_KEYID_RECIPIENT_FLAG 0x00000004

typedef struct _CRYPT_DECRYPT_MESSAGE_PARA {
    DWORD       cbSize;
    DWORD       dwMsgAndCertEncodingType;
    DWORD       cCertStore;
    HCERTSTORE *rghCertStore;
#ifdef CRYPT_DECRYPT_MESSAGE_PARA_HAS_EXTRA_FIELDS
    DWORD       dwFlags;
#endif
} CRYPT_DECRYPT_MESSAGE_PARA, *PCRYPT_DECRYPT_MESSAGE_PARA;

typedef struct _CRYPT_HASH_MESSAGE_PARA {
    DWORD                      cbSize;
    DWORD                      dwMsgEncodingType;
    HCRYPTPROV_LEGACY          hCryptProv;
    CRYPT_ALGORITHM_IDENTIFIER HashAlgorithm;
    void *                     pvHashAuxInfo;
} CRYPT_HASH_MESSAGE_PARA, *PCRYPT_HASH_MESSAGE_PARA;

typedef struct _CRYPT_KEY_SIGN_MESSAGE_PARA {
    DWORD                      cbSize;
    DWORD                      dwMsgAndCertEncodingType;
    HCRYPTPROV                 hCryptProv;
    DWORD                      dwKeySpec;
    CRYPT_ALGORITHM_IDENTIFIER HashAlgorithm;
    void *                     pvHashAuxInfo;
} CRYPT_KEY_SIGN_MESSAGE_PARA, *PCRYPT_KEY_SIGN_MESSAGE_PARA;

typedef struct _CRYPT_KEY_VERIFY_MESSAGE_PARA {
    DWORD      cbSize;
    DWORD      dwMsgEncodingType;
    HCRYPTPROV_LEGACY hCryptProv;
} CRYPT_KEY_VERIFY_MESSAGE_PARA, *PCRYPT_KEY_VERIFY_MESSAGE_PARA;

typedef struct _CRYPT_URL_ARRAY {
    DWORD   cUrl;
    LPWSTR *rgwszUrl;
} CRYPT_URL_ARRAY, *PCRYPT_URL_ARRAY;

typedef struct _CRYPT_URL_INFO {
    DWORD  cbSize;
    DWORD  dwSyncDeltaTime;
    DWORD  cGroup;
    DWORD *rgcGroupEntry;
} CRYPT_URL_INFO, *PCRYPT_URL_INFO;

#define URL_OID_CERTIFICATE_ISSUER                  ((LPCSTR)1)
#define URL_OID_CERTIFICATE_CRL_DIST_POINT          ((LPCSTR)2)
#define URL_OID_CTL_ISSUER                          ((LPCSTR)3)
#define URL_OID_CTL_NEXT_UPDATE                     ((LPCSTR)4)
#define URL_OID_CRL_ISSUER                          ((LPCSTR)5)
#define URL_OID_CERTIFICATE_FRESHEST_CRL            ((LPCSTR)6)
#define URL_OID_CRL_FRESHEST_CRL                    ((LPCSTR)7)
#define URL_OID_CROSS_CERT_DIST_POINT               ((LPCSTR)8)
#define URL_OID_CERTIFICATE_OCSP                    ((LPCSTR)9)
#define URL_OID_CERTIFICATE_OCSP_AND_CRL_DIST_POINT ((LPCSTR)10)
#define URL_OID_CERTIFICATE_CRL_DIST_POINT_AND_OCSP ((LPCSTR)11)
#define URL_OID_CROSS_CERT_SUBJECT_INFO_ACCESS      ((LPCSTR)12)

#define URL_OID_GET_OBJECT_URL_FUNC "UrlDllGetObjectUrl"

typedef HANDLE HCRYPTASYNC, *PHCRYPTASYNC;

typedef void
(WINAPI *PFN_CRYPT_ASYNC_PARAM_FREE_FUNC)(
  _In_ LPSTR pszParamOid,
  _In_ LPVOID pvParam);

#define CRYPT_PARAM_ASYNC_RETRIEVAL_COMPLETION ((LPCSTR)1)
#define CRYPT_PARAM_CANCEL_ASYNC_RETRIEVAL     ((LPCSTR)2)

typedef void
(WINAPI *PFN_CRYPT_ASYNC_RETRIEVAL_COMPLETION_FUNC)(
  _Inout_opt_ void *pvCompletion,
  _In_ DWORD dwCompletionCode,
  _In_ LPCSTR pszURL,
  _In_opt_ LPSTR pszObjectOid,
  _In_ void *pvObject);

typedef struct _CRYPT_ASYNC_RETRIEVAL_COMPLETION {
  __callback PFN_CRYPT_ASYNC_RETRIEVAL_COMPLETION_FUNC pfnCompletion;
  _Inout_opt_ void *pvCompletion;
} CRYPT_ASYNC_RETRIEVAL_COMPLETION, *PCRYPT_ASYNC_RETRIEVAL_COMPLETION;

typedef BOOL
(WINAPI *PFN_CANCEL_ASYNC_RETRIEVAL_FUNC)(
  _In_opt_ HCRYPTASYNC hAsyncRetrieve);

typedef struct _CRYPT_BLOB_ARRAY
{
    DWORD            cBlob;
    PCRYPT_DATA_BLOB rgBlob;
} CRYPT_BLOB_ARRAY, *PCRYPT_BLOB_ARRAY;

typedef struct _CRYPT_CREDENTIALS {
    DWORD  cbSize;
    LPCSTR pszCredentialsOid;
    LPVOID pvCredentials;
} CRYPT_CREDENTIALS, *PCRYPT_CREDENTIALS;

#define CREDENTIAL_OID_PASSWORD_CREDENTIALS_A ((LPCSTR)1)
#define CREDENTIAL_OID_PASSWORD_CREDENTIALS_W ((LPCSTR)2)
#define CREDENTIAL_OID_PASSWORD_CREDENTIALS \
 WINELIB_NAME_AW(CREDENTIAL_OID_PASSWORD_CREDENTIALS_)

typedef struct _CRYPT_PASSWORD_CREDENTIALSA {
    DWORD cbSize;
    LPSTR pszUsername;
    LPSTR pszPassword;
} CRYPT_PASSWORD_CREDENTIALSA, *PCRYPT_PASSWORD_CREDENTIALSA;

typedef struct _CRYPT_PASSWORD_CREDENTIALSW {
    DWORD  cbSize;
    LPWSTR pszUsername;
    LPWSTR pszPassword;
} CRYPT_PASSWORD_CREDENTIALSW, *PCRYPT_PASSWORD_CREDENTIALSW;
#define CRYPT_PASSWORD_CREDENTIALS WINELIB_NAME_AW(CRYPT_PASSWORD_CREDENTIALS)
#define PCRYPT_PASSWORD_CREDENTIALS WINELIB_NAME_AW(PCRYPT_PASSWORD_CREDENTIALS)

typedef struct _CRYPT_RETRIEVE_AUX_INFO {
    DWORD     cbSize;
    FILETIME *pLastSyncTime;
    DWORD     dwMaxUrlRetrievalByteCount;
} CRYPT_RETRIEVE_AUX_INFO, *PCRYPT_RETRIEVE_AUX_INFO;

typedef void
(WINAPI *PFN_FREE_ENCODED_OBJECT_FUNC)(
  _In_opt_ LPCSTR pszObjectOid,
  _Inout_ PCRYPT_BLOB_ARRAY pObject,
  _Inout_opt_ void *pvFreeContext);

#define SCHEME_OID_RETRIEVE_ENCODED_OBJECT_FUNC \
 "SchemeDllRetrieveEncodedObject"
#define SCHEME_OID_RETRIEVE_ENCODED_OBJECTW_FUNC \
 "SchemeDllRetrieveEncodedObjectW"
/* The signature of SchemeDllRetrieveEncodedObjectW is:
BOOL WINAPI SchemeDllRetrieveEncodedObjectW(LPCWSTR pwszUrl,
 LPCSTR pszObjectOid, DWORD dwRetrievalFlags, DWORD dwTimeout,
 PCRYPT_BLOB_ARRAY pObject, PFN_FREE_ENCODED_OBJECT_FUNC *ppfnFreeObject,
 void **ppvFreeContext, HCRYPTASYNC hAsyncRetrieve,
 PCRYPT_CREDENTIALS pCredentials, PCRYPT_RETRIEVE_AUX_INFO pAuxInfo);
 */

#define CONTEXT_OID_CREATE_OBJECT_CONTEXT_FUNC "ContextDllCreateObjectContext"
/* The signature of ContextDllCreateObjectContext is:
BOOL WINAPI ContextDllCreateObjectContext(LPCSTR pszObjectOid,
 DWORD dwRetrievalFlags, PCRYPT_BLOB_ARRAY pObject, void **ppvContxt);
 */

#define CONTEXT_OID_CERTIFICATE ((LPCSTR)1)
#define CONTEXT_OID_CRL         ((LPCSTR)2)
#define CONTEXT_OID_CTL         ((LPCSTR)3)
#define CONTEXT_OID_PKCS7       ((LPCSTR)4)
#define CONTEXT_OID_CAPI2_ANY   ((LPCSTR)5)

#define CRYPT_RETRIEVE_MULTIPLE_OBJECTS      0x00000001
#define CRYPT_CACHE_ONLY_RETRIEVAL           0x00000002
#define CRYPT_WIRE_ONLY_RETRIEVAL            0x00000004
#define CRYPT_DONT_CACHE_RESULT              0x00000008
#define CRYPT_ASYNC_RETRIEVAL                0x00000010
#define CRYPT_STICKY_CACHE_RETRIEVAL         0x00001000
#define CRYPT_LDAP_SCOPE_BASE_ONLY_RETRIEVAL 0x00002000
#define CRYPT_OFFLINE_CHECK_RETRIEVAL        0x00004000
#define CRYPT_LDAP_INSERT_ENTRY_ATTRIBUTE    0x00008000
#define CRYPT_LDAP_SIGN_RETRIEVAL            0x00010000
#define CRYPT_NO_AUTH_RETRIEVAL              0x00020000
#define CRYPT_LDAP_AREC_EXCLUSIVE_RETRIEVAL  0x00040000
#define CRYPT_AIA_RETRIEVAL                  0x00080000

#define CRYPT_VERIFY_CONTEXT_SIGNATURE      0x00000020
#define CRYPT_VERIFY_DATA_HASH              0x00000040
#define CRYPT_KEEP_TIME_VALID               0x00000080
#define CRYPT_DONT_VERIFY_SIGNATURE         0x00000100
#define CRYPT_DONT_CHECK_TIME_VALIDITY      0x00000200
#define CRYPT_CHECK_FRESHNESS_TIME_VALIDITY 0x00000400
#define CRYPT_ACCUMULATIVE_TIMEOUT          0x00000800

typedef BOOL
(WINAPI *PFN_CRYPT_CANCEL_RETRIEVAL)(
  _In_ DWORD dwFlags,
  _Inout_opt_ void *pvArg);

typedef struct _CERT_CRL_CONTEXT_PAIR
{
    PCCERT_CONTEXT pCertContext;
    PCCRL_CONTEXT  pCrlContext;
} CERT_CRL_CONTEXT_PAIR, *PCERT_CRL_CONTEXT_PAIR;
typedef const CERT_CRL_CONTEXT_PAIR *PCCERT_CRL_CONTEXT_PAIR;

#define TIME_VALID_OID_GET_OBJECT_FUNC   "TimeValidDllGetObject"

#define TIME_VALID_OID_GET_CTL                    ((LPCSTR)1)
#define TIME_VALID_OID_GET_CRL                    ((LPCSTR)2)
#define TIME_VALID_OID_GET_CRL_FROM_CERT          ((LPCSTR)3)
#define TIME_VALID_OID_GET_FRESHEST_CRL_FROM_CERT ((LPCSTR)4)
#define TIME_VALID_OID_GET_FRESHEST_CRL_FROM_CRL  ((LPCSTR)5)

#define TIME_VALID_OID_FLUSH_OBJECT_FUNC "TimeValidDllFlushObject"

#define TIME_VALID_OID_FLUSH_CTL                    ((LPCSTR)1)
#define TIME_VALID_OID_FLUSH_CRL                    ((LPCSTR)2)
#define TIME_VALID_OID_FLUSH_CRL_FROM_CERT          ((LPCSTR)3)
#define TIME_VALID_OID_FLUSH_FRESHEST_CRL_FROM_CERT ((LPCSTR)4)
#define TIME_VALID_OID_FLUSH_FRESHEST_CRL_FROM_CRL  ((LPCSTR)5)

/* OID group IDs */
#define CRYPT_HASH_ALG_OID_GROUP_ID     1
#define CRYPT_ENCRYPT_ALG_OID_GROUP_ID  2
#define CRYPT_PUBKEY_ALG_OID_GROUP_ID   3
#define CRYPT_SIGN_ALG_OID_GROUP_ID     4
#define CRYPT_RDN_ATTR_OID_GROUP_ID     5
#define CRYPT_EXT_OR_ATTR_OID_GROUP_ID  6
#define CRYPT_ENHKEY_USAGE_OID_GROUP_ID 7
#define CRYPT_POLICY_OID_GROUP_ID       8
#define CRYPT_TEMPLATE_OID_GROUP_ID     9
#define CRYPT_LAST_OID_GROUP_ID         9

#define CRYPT_FIRST_ALG_OID_GROUP_ID CRYPT_HASH_ALG_OID_GROUP_ID
#define CRYPT_LAST_ALG_OID_GROUP_ID  CRYPT_SIGN_ALG_OID_GROUP_ID

#define CRYPT_OID_INHIBIT_SIGNATURE_FORMAT_FLAG  0x1
#define CRYPT_OID_USE_PUBKEY_PARA_FOR_PKCS7_FLAG 0x2
#define CRYPT_OID_NO_NULL_ALGORITHM_PARA_FLAG    0x4

#define CRYPT_OID_INFO_OID_KEY   1
#define CRYPT_OID_INFO_NAME_KEY  2
#define CRYPT_OID_INFO_ALGID_KEY 3
#define CRYPT_OID_INFO_SIGN_KEY  4

/* Algorithm IDs */

#define GET_ALG_CLASS(x)                (x & (7 << 13))
#define GET_ALG_TYPE(x)                 (x & (15 << 9))
#define GET_ALG_SID(x)                  (x & (511))

/* Algorithm Classes */
#define ALG_CLASS_ANY                   (0)
#define ALG_CLASS_SIGNATURE             (1 << 13)
#define ALG_CLASS_MSG_ENCRYPT           (2 << 13)
#define ALG_CLASS_DATA_ENCRYPT          (3 << 13)
#define ALG_CLASS_HASH                  (4 << 13)
#define ALG_CLASS_KEY_EXCHANGE          (5 << 13)
#define ALG_CLASS_ALL                   (7 << 13)
/* Algorithm types */
#define ALG_TYPE_ANY                    (0)
#define ALG_TYPE_DSS                    (1 << 9)
#define ALG_TYPE_RSA                    (2 << 9)
#define ALG_TYPE_BLOCK                  (3 << 9)
#define ALG_TYPE_STREAM                 (4 << 9)
#define ALG_TYPE_DH                     (5 << 9)
#define ALG_TYPE_SECURECHANNEL          (6 << 9)

/* SIDs */
#define ALG_SID_ANY                     (0)
/* RSA SIDs */
#define ALG_SID_RSA_ANY                 0
#define ALG_SID_RSA_PKCS                1
#define ALG_SID_RSA_MSATWORK            2
#define ALG_SID_RSA_ENTRUST             3
#define ALG_SID_RSA_PGP                 4
/* DSS SIDs */
#define ALG_SID_DSS_ANY                 0
#define ALG_SID_DSS_PKCS                1
#define ALG_SID_DSS_DMS                 2
#define ALG_SID_ECDSA                   3

/* DES SIDs */
#define ALG_SID_DES                     1
#define ALG_SID_3DES                    3
#define ALG_SID_DESX                    4
#define ALG_SID_IDEA                    5
#define ALG_SID_CAST                    6
#define ALG_SID_SAFERSK64               7
#define ALG_SID_SAFERSK128              8
#define ALG_SID_3DES_112                9
#define ALG_SID_CYLINK_MEK             12
#define ALG_SID_RC5                    13
#define ALG_SID_AES_128                14
#define ALG_SID_AES_192                15
#define ALG_SID_AES_256                16
#define ALG_SID_AES                    17
/* Diffie-Hellmans SIDs */
#define ALG_SID_DH_SANDF                1
#define ALG_SID_DH_EPHEM                2
#define ALG_SID_AGREED_KEY_ANY          3
#define ALG_SID_KEA                     4
#define ALG_SID_ECDH                    5
/* RC2 SIDs */
#define ALG_SID_RC4                     1
#define ALG_SID_RC2                     2
#define ALG_SID_SEAL                    2
/* Hash SIDs */
#define ALG_SID_MD2                     1
#define ALG_SID_MD4                     2
#define ALG_SID_MD5                     3
#define ALG_SID_SHA                     4
#define ALG_SID_SHA1                    ALG_SID_SHA
#define ALG_SID_MAC                     5
#define ALG_SID_RIPEMD                  6
#define ALG_SID_RIPEMD160               7
#define ALG_SID_SSL3SHAMD5              8
#define ALG_SID_HMAC                    9
#define ALG_SID_TLS1PRF                10
#define ALG_SID_HASH_REPLACE_OWF       11
#define ALG_SID_SHA_256                12
#define ALG_SID_SHA_384                13
#define ALG_SID_SHA_512                14
/* SCHANNEL SIDs */
#define ALG_SID_SSL3_MASTER             1
#define ALG_SID_SCHANNEL_MASTER_HASH    2
#define ALG_SID_SCHANNEL_MAC_KEY        3
#define ALG_SID_PCT1_MASTER             4
#define ALG_SID_SSL2_MASTER             5
#define ALG_SID_TLS1_MASTER             6
#define ALG_SID_SCHANNEL_ENC_KEY        7
#define ALG_SID_EXAMPLE                80

/* Algorithm Definitions */
#define CALG_MD2                  (ALG_CLASS_HASH         | ALG_TYPE_ANY           | ALG_SID_MD2)
#define CALG_MD4                  (ALG_CLASS_HASH         | ALG_TYPE_ANY           | ALG_SID_MD4)
#define CALG_MD5                  (ALG_CLASS_HASH         | ALG_TYPE_ANY           | ALG_SID_MD5)
#define CALG_SHA                  (ALG_CLASS_HASH         | ALG_TYPE_ANY           | ALG_SID_SHA)
#define CALG_SHA1 CALG_SHA
#define CALG_MAC                  (ALG_CLASS_HASH         | ALG_TYPE_ANY           | ALG_SID_MAC)
#define CALG_SSL3_SHAMD5          (ALG_CLASS_HASH         | ALG_TYPE_ANY           | ALG_SID_SSL3SHAMD5)
#define CALG_HMAC                 (ALG_CLASS_HASH         | ALG_TYPE_ANY           | ALG_SID_HMAC)
#define CALG_TLS1PRF              (ALG_CLASS_HASH         | ALG_TYPE_ANY           | ALG_SID_TLS1PRF)
#define CALG_HASH_REPLACE_OWF     (ALG_CLASS_HASH         | ALG_TYPE_ANY           | ALG_SID_HASH_REPLACE_OWF)
#define CALG_SHA_256              (ALG_CLASS_HASH         | ALG_TYPE_ANY           | ALG_SID_SHA_256)
#define CALG_SHA_384              (ALG_CLASS_HASH         | ALG_TYPE_ANY           | ALG_SID_SHA_384)
#define CALG_SHA_512              (ALG_CLASS_HASH         | ALG_TYPE_ANY           | ALG_SID_SHA_512)
#define CALG_RSA_SIGN             (ALG_CLASS_SIGNATURE    | ALG_TYPE_RSA           | ALG_SID_RSA_ANY)
#define CALG_DSS_SIGN             (ALG_CLASS_SIGNATURE    | ALG_TYPE_DSS           | ALG_SID_DSS_ANY)
#define CALG_NO_SIGN              (ALG_CLASS_SIGNATURE    | ALG_TYPE_ANY           | ALG_SID_ANY)
#define CALG_DH_SF                (ALG_CLASS_KEY_EXCHANGE | ALG_TYPE_DH            | ALG_SID_DH_SANDF)
#define CALG_DH_EPHEM             (ALG_CLASS_KEY_EXCHANGE | ALG_TYPE_DH            | ALG_SID_DH_EPHEM)
#define CALG_RSA_KEYX             (ALG_CLASS_KEY_EXCHANGE | ALG_TYPE_RSA           | ALG_SID_RSA_ANY)
#define CALG_ECDH                 (ALG_CLASS_KEY_EXCHANGE | ALG_TYPE_DH            | ALG_SID_ECDH)
#define CALG_ECMQV                (ALG_CLASS_KEY_EXCHANGE | ALG_TYPE_ANY           | ALG_SID_ECMQV)
#define CALG_DES                  (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK         | ALG_SID_DES)
#define CALG_RC2                  (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK         | ALG_SID_RC2)
#define CALG_3DES                 (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK         | ALG_SID_3DES)
#define CALG_3DES_112             (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK         | ALG_SID_3DES_112)
#define CALG_AES_128              (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK         | ALG_SID_AES_128)
#define CALG_AES_192              (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK         | ALG_SID_AES_192)
#define CALG_AES_256              (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK         | ALG_SID_AES_256)
#define CALG_AES                  (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK         | ALG_SID_AES)
#define CALG_RC4                  (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_STREAM        | ALG_SID_RC4)
#define CALG_SEAL                 (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_STREAM        | ALG_SID_SEAL)
#define CALG_RC5                  (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_STREAM        | ALG_SID_RC5)
#define CALG_SSL3_MASTER          (ALG_CLASS_MSG_ENCRYPT  | ALG_TYPE_SECURECHANNEL | ALG_SID_SSL3_MASTER)
#define CALG_SCHANNEL_MASTER_HASH (ALG_CLASS_MSG_ENCRYPT  | ALG_TYPE_SECURECHANNEL | ALG_SID_SCHANNEL_MASTER_HASH)
#define CALG_SCHANNEL_MAC_KEY     (ALG_CLASS_MSG_ENCRYPT  | ALG_TYPE_SECURECHANNEL | ALG_SID_SCHANNEL_MAC_KEY)
#define CALG_SCHANNEL_ENC_KEY     (ALG_CLASS_MSG_ENCRYPT  | ALG_TYPE_SECURECHANNEL | ALG_SID_SCHANNEL_ENC_KEY)
#define CALG_PCT1_MASTER          (ALG_CLASS_MSG_ENCRYPT  | ALG_TYPE_SECURECHANNEL | ALG_SID_PCT1_MASTER)
#define CALG_SSL2_MASTER          (ALG_CLASS_MSG_ENCRYPT  | ALG_TYPE_SECURECHANNEL | ALG_SID_SSL2_MASTER)
#define CALG_TLS1_MASTER          (ALG_CLASS_MSG_ENCRYPT  | ALG_TYPE_SECURECHANNEL | ALG_SID_TLS1_MASTER)
#define CALG_ECDSA                (ALG_CLASS_SIGNATURE    | ALG_TYPE_DSS           | ALG_SID_ECDSA)
/* Protocol Flags */
#define CRYPT_FLAG_PCT1    0x0001
#define CRYPT_FLAG_SSL2    0x0002
#define CRYPT_FLAG_SSL3    0x0004
#define CRYPT_FLAG_TLS1    0x0008
#define CRYPT_FLAG_IPSEC   0x0010
#define CRYPT_FLAG_SIGNING 0x0020

/* Provider names */
#define MS_DEF_PROV_A                            "Microsoft Base Cryptographic Provider v1.0"
#if defined(__GNUC__)
# define MS_DEF_PROV_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',' ', \
	'B','a','s','e',' ','C','r','y','p','t','o','g','r','a','p','h','i','c',' ', \
	'P','r','o','v','i','d','e','r',' ','v','1','.','0',0 }
#elif defined(_MSC_VER)
# define MS_DEF_PROV_W      L"Microsoft Base Cryptographic Provider v1.0"
#else
static const WCHAR MS_DEF_PROV_W[] =             { 'M','i','c','r','o','s','o','f','t',' ',
	'B','a','s','e',' ','C','r','y','p','t','o','g','r','a','p','h','i','c',' ',
	'P','r','o','v','i','d','e','r',' ','v','1','.','0',0 };
#endif
#define MS_DEF_PROV                              WINELIB_NAME_AW(MS_DEF_PROV_)

#define MS_ENHANCED_PROV_A                       "Microsoft Enhanced Cryptographic Provider v1.0"
#if defined(__GNUC__)
# define MS_ENHANCED_PROV_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',' ', \
	'E','n','h','a','n','c','e','d',' ','C','r','y','p','t','o','g','r','a','p','h','i','c',' ', \
	'P','r','o','v','i','d','e','r',' ','v','1','.','0',0 }
#elif defined(_MSC_VER)
# define MS_ENHANCED_PROV_W     L"Microsoft Enhanced Cryptographic Provider v1.0"
#else
static const WCHAR MS_ENHANCED_PROV_W[] =        { 'M','i','c','r','o','s','o','f','t',' ',
	'E','n','h','a','n','c','e','d',' ','C','r','y','p','t','o','g','r','a','p','h','i','c',' ',
	'P','r','o','v','i','d','e','r',' ','v','1','.','0',0 };
#endif
#define MS_ENHANCED_PROV                         WINELIB_NAME_AW(MS_ENHANCED_PROV_)

#define MS_STRONG_PROV_A                         "Microsoft Strong Cryptographic Provider"
#if defined(__GNUC__)
# define MS_STRONG_PROV_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',' ', \
	'S','t','r','o','n','g',' ','C','r','y','p','t','o','g','r','a','p','h','i','c',' ', \
	'P','r','o','v','i','d','e','r',0 }
#elif defined(_MSC_VER)
# define MS_STRONG_PROV_W     L"Microsoft Strong Cryptographic Provider"
#else
static const WCHAR MS_STRONG_PROV_W[] =          { 'M','i','c','r','o','s','o','f','t',' ',
	'S','t','r','o','n','g',' ','C','r','y','p','t','o','g','r','a','p','h','i','c',' ',
	'P','r','o','v','i','d','e','r',0 };
#endif
#define MS_STRONG_PROV                           WINELIB_NAME_AW(MS_STRONG_PROV_)

#define MS_DEF_RSA_SIG_PROV_A                    "Microsoft RSA Signature Cryptographic Provider"
#if defined(__GNUC__)
# define MS_DEF_RSA_SIG_PROV_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',' ', \
	'R','S','A',' ','S','i','g','n','a','t','u','r','e',' ', \
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 }
#elif defined(_MSC_VER)
# define MS_DEF_RSA_SIG_PROV_W      L"Microsoft RSA Signature Cryptographic Provider"
#else
static const WCHAR MS_DEF_RSA_SIG_PROV_W[] =     { 'M','i','c','r','o','s','o','f','t',' ',
	'R','S','A',' ','S','i','g','n','a','t','u','r','e',' ',
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 };
#endif
#define MS_DEF_RSA_SIG_PROV                      WINELIB_NAME_AW(MS_DEF_RSA_SIG_PROV_)

#define MS_DEF_RSA_SCHANNEL_PROV_A               "Microsoft RSA SChannel Cryptographic Provider"
#if defined(__GNUC__)
# define MS_DEF_RSA_SCHANNEL_PROV_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',' ', \
	'R','S','A',' ','S','C','h','a','n','n','e','l',' ', \
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 }
#elif defined(_MSC_VER)
# define MS_DEF_RSA_SCHANNEL_PROV_W     L"Microsoft RSA SChannel Cryptographic Provider"
#else
static const WCHAR MS_DEF_RSA_SCHANNEL_PROV_W[] = { 'M','i','c','r','o','s','o','f','t',' ',
	'R','S','A',' ','S','C','h','a','n','n','e','l',' ',
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 };
#endif
#define MS_DEF_RSA_SCHANNEL_PROV                 WINELIB_NAME_AW(MS_DEF_RSA_SCHANNEL_PROV_)

#define MS_DEF_DSS_PROV_A                        "Microsoft Base DSS Cryptographic Provider"
#if defined(__GNUC__)
# define MS_DEF_DSS_PROV_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',' ', \
	'B','a','s','e',' ','D','S','S',' ', \
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 }
#elif defined(_MSC_VER)
# define MS_DEF_DSS_PROV_W     L"Microsoft Base DSS Cryptographic Provider"
#else
static const WCHAR MS_DEF_DSS_PROV_W[] =         { 'M','i','c','r','o','s','o','f','t',' ',
	'B','a','s','e',' ','D','S','S',' ',
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 };
#endif
#define MS_DEF_DSS_PROV                          WINELIB_NAME_AW(MS_DEF_DSS_PROV_)

#define MS_DEF_DSS_DH_PROV_A                     "Microsoft Base DSS and Diffie-Hellman Cryptographic Provider"
#if defined(__GNUC__)
# define MS_DEF_DSS_DH_PROV_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',' ', \
	'B','a','s','e',' ','D','S','S',' ','a','n','d',' ', \
	'D','i','f','f','i','e','-','H','e','l','l','m','a','n',' ', \
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 }
#elif defined(_MSC_VER)
# define MS_DEF_DSS_DH_PROV_W     L"Microsoft Base DSS and Diffie-Hellman Cryptographic Provider"
#else
static const WCHAR MS_DEF_DSS_DH_PROV_W[] =      { 'M','i','c','r','o','s','o','f','t',' ',
	'B','a','s','e',' ','D','S','S',' ','a','n','d',' ',
	'D','i','f','f','i','e','-','H','e','l','l','m','a','n',' ',
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 };
#endif
#define MS_DEF_DSS_DH_PROV                       WINELIB_NAME_AW(MS_DEF_DSS_DH_PROV_)

#define MS_ENH_DSS_DH_PROV_A                     "Microsoft Enhanced DSS and Diffie-Hellman Cryptographic Provider"
#if defined(__GNUC__)
# define MS_ENH_DSS_DH_PROV_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',' ', \
	'E','n','h','a','n','c','e','d',' ','D','S','S',' ','a','n','d',' ', \
	'D','i','f','f','i','e','-','H','e','l','l','m','a','n',' ', \
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 }
#elif defined(_MSC_VER)
# define MS_ENH_DSS_DH_PROV_W     L"Microsoft Enhanced DSS and Diffie-Hellman Cryptographic Provider"
#else
static const WCHAR MS_ENH_DSS_DH_PROV_W[] =      { 'M','i','c','r','o','s','o','f','t',' ',
	'E','n','h','a','n','c','e','d',' ','D','S','S',' ','a','n','d',' ',
	'D','i','f','f','i','e','-','H','e','l','l','m','a','n',' ',
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 };
#endif
#define MS_ENH_DSS_DH_PROV                       WINELIB_NAME_AW(MS_ENH_DSS_DH_PROV_)

#define MS_DEF_DH_SCHANNEL_PROV_A                "Microsoft DH SChannel Cryptographic Provider"
#if defined(__GNUC__)
# define MS_DEF_DH_SCHANNEL_PROV_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',' ', \
	'D','H',' ','S','C','h','a','n','n','e','l',' ', \
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 }
#elif defined(_MSC_VER)
# define MS_DEF_DH_SCHANNEL_PROV_W     L"Microsoft DH SChannel Cryptographic Provider"
#else
static const WCHAR MS_DEF_DH_SCHANNEL_PROV_W[] = { 'M','i','c','r','o','s','o','f','t',' ',
	'D','H',' ','S','C','h','a','n','n','e','l',' ',
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 };
#endif
#define MS_DEF_DH_SCHANNEL_PROV                  WINELIB_NAME_AW(MS_DEF_DH_SCHANNEL_PROV_)

#define MS_SCARD_PROV_A                          "Microsoft Base Smart Card Cryptographic Provider"
#if defined(__GNUC__)
# define MS_SCARD_PROV_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',' ', \
	'B','a','s','e',' ','S','m','a','r','t',' ','C','a','r','d',' ', \
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 }
#elif defined(_MSC_VER)
# define MS_SCARD_PROV_W     L"Microsoft Base Smart Card Cryptographic Provider"
#else
static const WCHAR MS_SCARD_PROV_W[] =           { 'M','i','c','r','o','s','o','f','t',' ',
	'B','a','s','e',' ','S','m','a','r','t',' ','C','a','r','d',' ',
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 };
#endif
#define MS_SCARD_PROV                            WINELIB_NAME_AW(MS_SCARD_PROV_)

#define MS_ENH_RSA_AES_PROV_A                          "Microsoft Enhanced RSA and AES Cryptographic Provider"
#if defined(__GNUC__)
# define MS_ENH_RSA_AES_PROV_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',' ', \
	'E','n','h','a','n','c','e','d',' ','R','S','A',' ','a','n','d',' ','A','E','S',' ',\
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 }
#elif defined(_MSC_VER)
# define MS_ENH_RSA_AES_PROV_W     L"Microsoft Enhanced RSA and AES Cryptographic Provider"
#else
static const WCHAR MS_ENH_RSA_AES_PROV_W[] =           { 'M','i','c','r','o','s','o','f','t',' ',
	'E','n','h','a','n','c','e','d',' ','R','S','A',' ','a','n','d',' ','A','E','S',' ',
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 };
#endif
#define MS_ENH_RSA_AES_PROV                            WINELIB_NAME_AW(MS_ENH_RSA_AES_PROV_)

#define MS_ENH_RSA_AES_PROV_XP_A    "Microsoft Enhanced RSA and AES Cryptographic Provider (Prototype)"
#if defined(__GNUC__)
# define MS_ENH_RSA_AES_PROV_XP_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',' ', \
        'E','n','h','a','n','c','e','d',' ','R','S','A',' ','a','n','d',' ','A','E','S',' ',\
        'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',' ',\
        '(','P','r','o','t','o','t','y','p','e',')',0 }
#elif defined(_MSC_VER)
# define MS_ENH_RSA_AES_PROV_XP_W   L"Microsoft Enhanced RSA and AES Cryptographic Provider (Prototype)"
#else
static const WCHAR MS_ENH_RSA_AES_PROV_XP_W[] = { 'M','i','c','r','o','s','o','f','t',' ',
        'E','n','h','a','n','c','e','d',' ','R','S','A',' ','a','n','d',' ','A','E','S',' ',
        'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',' ',
        '(','P','r','o','t','o','t','y','p','e',')',0 };
#endif
#define MS_ENH_RSA_AES_PROV_XP                   WINELIB_NAME_AW(MS_ENH_RSA_AES_PROV_XP_)

/* Key Specs*/
#define AT_KEYEXCHANGE          1
#define AT_SIGNATURE            2

/* Provider Types */
#define PROV_RSA_FULL             1
#define PROV_RSA_SIG              2
#define PROV_DSS                  3
#define PROV_FORTEZZA             4
#define PROV_MS_EXCHANGE          5
#define PROV_SSL                  6
#define PROV_RSA_SCHANNEL         12
#define PROV_DSS_DH               13
#define PROV_EC_ECDSA_SIG         14
#define PROV_EC_ECNRA_SIG         15
#define PROV_EC_ECDSA_FULL        16
#define PROV_EC_ECNRA_FULL        17
#define PROV_DH_SCHANNEL          18
#define PROV_SPYRUS_LYNKS         20
#define PROV_RNG                  21
#define PROV_INTEL_SEC            22
#define PROV_REPLACE_OWF          23
#define PROV_RSA_AES              24

/* FLAGS Section */

#define CRYPT_FIRST             1
#define CRYPT_NEXT              2

#define CRYPT_IMPL_HARDWARE     1
#define CRYPT_IMPL_SOFTWARE     2
#define CRYPT_IMPL_MIXED        3
#define CRYPT_IMPL_UNKNOWN      4

/* CryptAcquireContext */
#define CRYPT_VERIFYCONTEXT       0xF0000000
#define CRYPT_NEWKEYSET           0x00000008
#define CRYPT_DELETEKEYSET        0x00000010
#define CRYPT_MACHINE_KEYSET      0x00000020
#define CRYPT_SILENT              0x00000040

/* Crypt{Get|Set}Provider */
#define CRYPT_MACHINE_DEFAULT     0x00000001
#define CRYPT_USER_DEFAULT        0x00000002
#define CRYPT_DELETE_DEFAULT      0x00000004

/* Crypt{Get/Set}ProvParam */
#define PP_CLIENT_HWND          1
#define PP_ENUMALGS             1
#define PP_ENUMCONTAINERS       2
#define PP_IMPTYPE              3
#define PP_NAME                 4
#define PP_VERSION              5
#define PP_CONTAINER            6
#define PP_CHANGE_PASSWORD      7
#define PP_KEYSET_SEC_DESCR     8
#define PP_KEY_TYPE_SUBTYPE     10
#define PP_CONTEXT_INFO         11
#define PP_KEYEXCHANGE_KEYSIZE  12
#define PP_SIGNATURE_KEYSIZE    13
#define PP_KEYEXCHANGE_ALG      14
#define PP_SIGNATURE_ALG        15
#define PP_PROVTYPE             16
#define PP_KEYSTORAGE           17
#define PP_SYM_KEYSIZE          19
#define PP_SESSION_KEYSIZE      20
#define PP_UI_PROMPT            21
#define PP_ENUMALGS_EX          22
#define PP_DELETEKEY            24
#define PP_ENUMMANDROOTS        25
#define PP_ENUMELECTROOTS       26
#define PP_KEYSET_TYPE          27
#define PP_ADMIN_PIN            31
#define PP_KEYEXCHANGE_PIN      32
#define PP_SIGNATURE_PIN        33
#define PP_SIG_KEYSIZE_INC      34
#define PP_KEYX_KEYSIZE_INC     35
#define PP_UNIQUE_CONTAINER     36
#define PP_SGC_INFO             37
#define PP_USE_HARDWARE_RNG     38
#define PP_KEYSPEC              39
#define PP_ENUMEX_SIGNING_PROT  40
#define PP_CRYPT_COUNT_KEY_USE  41
#define PP_USER_CERTSTORE       42
#define PP_SMARTCARD_READER     43
#define PP_SMARTCARD_GUID       45
#define PP_ROOT_CERTSTORE       46

/* Values returned by CryptGetProvParam of PP_KEYSTORAGE */
#define CRYPT_SEC_DESCR         0x00000001
#define CRYPT_PSTORE            0x00000002
#define CRYPT_UI_PROMPT         0x00000004

/* Crypt{Get/Set}KeyParam */
#define KP_IV                   1
#define KP_SALT                 2
#define KP_PADDING              3
#define KP_MODE                 4
#define KP_MODE_BITS            5
#define KP_PERMISSIONS          6
#define KP_ALGID                7
#define KP_BLOCKLEN             8
#define KP_KEYLEN               9
#define KP_SALT_EX              10
#define KP_P                    11
#define KP_G                    12
#define KP_Q                    13
#define KP_X                    14
#define KP_Y                    15
#define KP_RA                   16
#define KP_RB                   17
#define KP_INFO                 18
#define KP_EFFECTIVE_KEYLEN     19
#define KP_SCHANNEL_ALG         20
#define KP_CLIENT_RANDOM        21
#define KP_SERVER_RANDOM        22
#define KP_RP                   23
#define KP_PRECOMP_MD5          24
#define KP_PRECOMP_SHA          25
#define KP_CERTIFICATE          26
#define KP_CLEAR_KEY            27
#define KP_PUB_EX_LEN           28
#define KP_PUB_EX_VAL           29
#define KP_KEYVAL               30
#define KP_ADMIN_PIN            31
#define KP_KEYEXCHANGE_PIN      32
#define KP_SIGNATURE_PIN        33
#define KP_PREHASH              34
#define KP_ROUNDS               35
#define KP_OAEP_PARAMS          36
#define KP_CMS_KEY_INFO         37
#define KP_CMS_DH_KEY_INFO      38
#define KP_PUB_PARAMS           39
#define KP_VERIFY_PARAMS        40
#define KP_HIGHEST_VERSION      41
#define KP_GET_USE_COUNT        42

/* Values for KP_PADDING */
#define PKCS5_PADDING  1
#define RANDOM_PADDING 2
#define ZERO_PADDING   3

/* CryptSignHash/CryptVerifySignature */
#define CRYPT_NOHASHOID         0x00000001
#define CRYPT_TYPE2_FORMAT      0x00000002
#define CRYPT_X931_FORMAT       0x00000004

/* Crypt{Get,Set}HashParam */
#define HP_ALGID                0x0001
#define HP_HASHVAL              0x0002
#define HP_HASHSIZE             0x0004
#define HP_HMAC_INFO            0x0005
#define HP_TLS1PRF_LABEL        0x0006
#define HP_TLS1PRF_SEED         0x0007

/* Crypt{Get,Set}KeyParam */
#define CRYPT_MODE_CBC          1
#define CRYPT_MODE_ECB          2
#define CRYPT_MODE_OFB          3
#define CRYPT_MODE_CFB          4

#define CRYPT_ENCRYPT           0x0001 
#define CRYPT_DECRYPT           0x0002
#define CRYPT_EXPORT            0x0004
#define CRYPT_READ              0x0008
#define CRYPT_WRITE             0x0010
#define CRYPT_MAC               0x0020
#define CRYPT_EXPORT_KEY        0x0040
#define CRYPT_IMPORT_KEY        0x0080
#define CRYPT_ARCHIVE           0x0100

/* Crypt*Key */
#define CRYPT_EXPORTABLE        0x00000001
#define CRYPT_USER_PROTECTED    0x00000002
#define CRYPT_CREATE_SALT       0x00000004
#define CRYPT_UPDATE_KEY        0x00000008
#define CRYPT_NO_SALT           0x00000010
#define CRYPT_PREGEN            0x00000040
#define CRYPT_SERVER            0x00000400
#define CRYPT_ARCHIVABLE        0x00004000

/* CryptExportKey */
#define CRYPT_SSL2_FALLBACK     0x00000002
#define CRYPT_DESTROYKEY        0x00000004
#define CRYPT_OAEP              0x00000040

/* CryptHashSessionKey */
#define CRYPT_LITTLE_ENDIAN     0x00000001

/* Crypt{Protect,Unprotect}Data PROMPTSTRUCT flags */
#define CRYPTPROTECT_PROMPT_ON_PROTECT    0x0001
#define CRYPTPROTECT_PROMPT_ON_UNPROTECT  0x0002
/* Crypt{Protect,Unprotect}Data flags */
#define CRYPTPROTECT_UI_FORBIDDEN       0x0001
#define CRYPTPROTECT_LOCAL_MACHINE      0x0004
#define CRYPTPROTECT_AUDIT              0x0010
#define CRYPTPROTECT_VERIFY_PROTECTION  0x0040

/* Blob Types */
#define SIMPLEBLOB              0x1
#define PUBLICKEYBLOB           0x6
#define PRIVATEKEYBLOB          0x7
#define PLAINTEXTKEYBLOB        0x8
#define OPAQUEKEYBLOB           0x9
#define PUBLICKEYBLOBEX         0xA
#define SYMMETRICWRAPKEYBLOB    0xB

#define CUR_BLOB_VERSION        2

/* cert store provider types */
#define CERT_STORE_PROV_MSG                  ((LPCSTR)1)
#define CERT_STORE_PROV_MEMORY               ((LPCSTR)2)
#define CERT_STORE_PROV_FILE                 ((LPCSTR)3)
#define CERT_STORE_PROV_REG                  ((LPCSTR)4)
#define CERT_STORE_PROV_PKCS7                ((LPCSTR)5)
#define CERT_STORE_PROV_SERIALIZED           ((LPCSTR)6)
#define CERT_STORE_PROV_FILENAME_A           ((LPCSTR)7)
#define CERT_STORE_PROV_FILENAME_W           ((LPCSTR)8)
#define CERT_STORE_PROV_SYSTEM_A             ((LPCSTR)9)
#define CERT_STORE_PROV_SYSTEM_W             ((LPCSTR)10)
#define CERT_STORE_PROV_SYSTEM               CERT_STORE_PROV_SYSTEM_W
#define CERT_STORE_PROV_COLLECTION           ((LPCSTR)11)
#define CERT_STORE_PROV_SYSTEM_REGISTRY_A    ((LPCSTR)12)
#define CERT_STORE_PROV_SYSTEM_REGISTRY_W    ((LPCSTR)13)
#define CERT_STORE_PROV_SYSTEM_REGISTRY      CERT_STORE_PROV_SYSTEM_REGISTRY_W
#define CERT_STORE_PROV_PHYSICAL_W           ((LPCSTR)14)
#define CERT_STORE_PROV_PHYSICAL             CERT_STORE_PROV_PHYSICAL_W
#define CERT_STORE_PROV_SMART_CARD_W         ((LPCSTR)15)
#define CERT_STORE_PROV_SMART_CARD           CERT_STORE_PROV_SMART_CARD_W
#define CERT_STORE_PROV_LDAP_W               ((LPCSTR)16)
#define CERT_STORE_PROV_LDAP                 CERT_STORE_PROV_LDAP_W

#define sz_CERT_STORE_PROV_MEMORY            "Memory"
#define sz_CERT_STORE_PROV_FILENAME_W        "File"
#define sz_CERT_STORE_PROV_FILENAME          sz_CERT_STORE_PROV_FILENAME_W
#define sz_CERT_STORE_PROV_SYSTEM_W          "System"
#define sz_CERT_STORE_PROV_SYSTEM            sz_CERT_STORE_PROV_SYSTEM_W
#define sz_CERT_STORE_PROV_PKCS7             "PKCS7"
#define sz_CERT_STORE_PROV_SERIALIZED        "Serialized"
#define sz_CERT_STORE_PROV_COLLECTION        "Collection"
#define sz_CERT_STORE_PROV_SYSTEM_REGISTRY_W "SystemRegistry"
#define sz_CERT_STORE_PROV_SYSTEM_REGISTRY   sz_CERT_STORE_PROV_SYSTEM_REGISTRY_W
#define sz_CERT_STORE_PROV_PHYSICAL_W        "Physical"
#define sz_CERT_STORE_PROV_PHYSICAL          sz_CERT_STORE_PROV_PHYSICAL_W
#define sz_CERT_STORE_PROV_SMART_CARD_W      "SmartCard"
#define sz_CERT_STORE_PROV_SMART_CARD        sz_CERT_STORE_PROV_SMART_CARD_W
#define sz_CERT_STORE_PROV_LDAP_W            "Ldap"
#define sz_CERT_STORE_PROV_LDAP              sz_CERT_STORE_PROV_LDAP_W

/* types for CertOpenStore dwEncodingType */
#define CERT_ENCODING_TYPE_MASK 0x0000ffff
#define CMSG_ENCODING_TYPE_MASK 0xffff0000
#define GET_CERT_ENCODING_TYPE(x) ((x) & CERT_ENCODING_TYPE_MASK)
#define GET_CMSG_ENCODING_TYPE(x) ((x) & CMSG_ENCODING_TYPE_MASK)

#define CRYPT_ASN_ENCODING  0x00000001
#define CRYPT_NDR_ENCODING  0x00000002
#define X509_ASN_ENCODING   0x00000001
#define X509_NDR_ENCODING   0x00000002
#define PKCS_7_ASN_ENCODING 0x00010000
#define PKCS_7_NDR_ENCODING 0x00020000

/* system store locations */
#define CERT_SYSTEM_STORE_LOCATION_MASK  0x00ff0000
#define CERT_SYSTEM_STORE_LOCATION_SHIFT 16

/* system store location ids */
/* hkcu */
#define CERT_SYSTEM_STORE_CURRENT_USER_ID               1
/* hklm */
#define CERT_SYSTEM_STORE_LOCAL_MACHINE_ID              2
/* hklm\Software\Microsoft\Cryptography\Services */
#define CERT_SYSTEM_STORE_CURRENT_SERVICE_ID            4
#define CERT_SYSTEM_STORE_SERVICES_ID                   5
/* HKEY_USERS */
#define CERT_SYSTEM_STORE_USERS_ID                      6
/* hkcu\Software\Policies\Microsoft\SystemCertificates */
#define CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY_ID  7
/* hklm\Software\Policies\Microsoft\SystemCertificates */
#define CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY_ID 8
/* hklm\Software\Microsoft\EnterpriseCertificates */
#define CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE_ID   9

/* system store location values */
#define CERT_SYSTEM_STORE_CURRENT_USER \
 (CERT_SYSTEM_STORE_CURRENT_USER_ID << CERT_SYSTEM_STORE_LOCATION_SHIFT)
#define CERT_SYSTEM_STORE_LOCAL_MACHINE \
 (CERT_SYSTEM_STORE_LOCAL_MACHINE_ID << CERT_SYSTEM_STORE_LOCATION_SHIFT)
#define CERT_SYSTEM_STORE_CURRENT_SERVICE \
 (CERT_SYSTEM_STORE_CURRENT_SERVICE_ID << CERT_SYSTEM_STORE_LOCATION_SHIFT)
#define CERT_SYSTEM_STORE_SERVICES \
 (CERT_SYSTEM_STORE_SERVICES_ID << CERT_SYSTEM_STORE_LOCATION_SHIFT)
#define CERT_SYSTEM_STORE_USERS \
 (CERT_SYSTEM_STORE_USERS_ID << CERT_SYSTEM_STORE_LOCATION_SHIFT)
#define CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY \
 (CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY_ID << CERT_SYSTEM_STORE_LOCATION_SHIFT)
#define CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY \
 (CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY_ID << CERT_SYSTEM_STORE_LOCATION_SHIFT)
#define CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE \
 (CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE_ID << CERT_SYSTEM_STORE_LOCATION_SHIFT)

#if defined(__GNUC__)
#define CERT_LOCAL_MACHINE_SYSTEM_STORE_REGPATH (const WCHAR[])\
 {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t',\
  '\\','S','y','s','t','e','m','C','e','r','t','i','f','i','c','a','t','e','s',\
  0 }
#define CERT_GROUP_POLICY_SYSTEM_STORE_REGPATH (const WCHAR[])\
 {'S','o','f','t','w','a','r','e','\\','P','o','l','i','c','i','e','s','\\',\
  'M','i','c','r','o','s','o','f','t','\\','S','y','s','t','e','m','C','e','r',\
  't','i','f','i','c','a','t','e','s',0 }
#elif defined(_MSC_VER)
#define CERT_LOCAL_MACHINE_SYSTEM_STORE_REGPATH \
 L"Software\\Microsoft\\SystemCertificates"
#define CERT_GROUP_POLICY_SYSTEM_STORE_REGPATH \
 L"Software\\Policies\\Microsoft\\SystemCertificates"
#else
static const WCHAR CERT_LOCAL_MACHINE_SYSTEM_STORE_REGPATH[] = 
 {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
  'S','y','s','t','e','m','C','e','r','t','i','f','i','c','a','t','e','s',0 };
static const WCHAR CERT_GROUP_POLICY_SYSTEM_STORE_REGPATH[] = 
 {'S','o','f','t','w','a','r','e','\\','P','o','l','i','c','i','e','s','\\',
  'M','i','c','r','o','s','o','f','t','\\','S','y','s','t','e','m','C','e','r',
  't','i','f','i','c','a','t','e','s',0 };
#endif

#if defined(__GNUC__)
#define CERT_EFSBLOB_REGPATH (const WCHAR[])\
{'S','o','f','t','w','a','r','e','\\','P','o','l','i','c','i','e','s','\\',\
 'M','i','c','r','o','s','o','f','t','\\','S','y','s','t','e','m','C','e','r',\
 't','i','f','i','c','a','t','e','s','\\','E','F','S',0 }
#define CERT_EFSBLOB_VALUE_NAME (const WCHAR[]) {'E','F','S','B','l','o','b',0 }
#elif defined(_MSC_VER)
#define CERT_EFSBLOB_REGPATH CERT_GROUP_POLICY_SYSTEM_STORE_REGPATH L"\\EFS"
#define CERT_EFSBLOB_VALUE_NAME L"EFSBlob"
#else
static const WCHAR CERT_EFSBLOB_REGPATH[] =
 {'S','o','f','t','w','a','r','e','\\','P','o','l','i','c','i','e','s','\\',
  'M','i','c','r','o','s','o','f','t','\\','S','y','s','t','e','m','C','e','r',
  't','i','f','i','c','a','t','e','s','\\','E','F','S',0 };
static const CERT_EFSBLOB_VALUE_NAME[] = { 'E','F','S','B','l','o','b',0 };
#endif

#if defined(__GNUC__)
#define CERT_PROT_ROOT_FLAGS_REGPATH (const WCHAR[])\
{'\\','R','o','o','t','\\','P','r','o','t','e','c','t','e','d','R','o','o','t',\
 's',0 }
#define CERT_PROT_ROOT_FLAGS_VALUE_NAME (const WCHAR[])\
{'F','l','a','g','s',0 }
#elif defined(_MSC_VER)
#define CERT_PROT_ROOT_FLAGS_REGPATH L"\\Root\\ProtectedRoots"
#define CERT_PROT_ROOT_FLAGS_VALUE_NAME L"Flags"
#else
static const WCHAR CERT_PROT_ROOT_FLAGS_REGPATH[] =
 { '\\','R','o','o','t','\\','P','r','o','t','e','c','t','e','d','R','o','o',
   't','s',0 };
static const WCHAR CERT_PROT_ROOT_FLAGS_VALUE_NAME[] = {'F','l','a','g','s',0 };
#endif

#define CERT_PROT_ROOT_DISABLE_CURRENT_USER_FLAG                0x01
#define CERT_PROT_ROOT_INHIBIT_ADD_AT_INIT_FLAG                 0x02
#define CERT_PROT_ROOT_INHIBIT_PURGE_LM_FLAG                    0x04
#define CERT_PROT_ROOT_DISABLE_LM_AUTH_FLAG                     0x08
#define CERT_PROT_ROOT_DISABLE_NT_AUTH_REQUIRED_FLAG            0x10
#define CERT_PROT_ROOT_DISABLE_NOT_DEFINED_NAME_CONSTRAINT_FLAG 0x20

#if defined(__GNUC__)
#define CERT_TRUST_PUB_SAFER_GROUP_POLICY_REGPATH (const WCHAR[])\
{'S','o','f','t','w','a','r','e','\\','P','o','l','i','c','i','e','s','\\',\
 'M','i','c','r','o','s','o','f','t','\\','S','y','s','t','e','m','C','e','r',\
 't','i','f','i','c','a','t','e','s','\\','T','r','u','s','t','e','d',\
 'P','u','b','l','i','s','h','e','r','\\','S','a','f','e','r',0 }
#elif defined(_MSC_VER)
#define CERT_TRUST_PUB_SAFER_GROUP_POLICY_REGPATH \
 CERT_GROUP_POLICY_SYSTEM_STORE_REGPATH L"\\TrustedPublisher\\Safer"
#else
static const WCHAR CERT_TRUST_PUB_SAFER_GROUP_POLICY_REGPATH[] =
 {'S','o','f','t','w','a','r','e','\\','P','o','l','i','c','i','e','s','\\',
  'M','i','c','r','o','s','o','f','t','\\','S','y','s','t','e','m','C','e','r',
  't','i','f','i','c','a','t','e','s','\\','T','r','u','s','t','e','d',
  'P','u','b','l','i','s','h','e','r','\\','S','a','f','e','r',0 };
#endif

#if defined(__GNUC__)
#define CERT_TRUST_PUB_SAFER_LOCAL_MACHINE_REGPATH (const WCHAR[])\
{'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',\
 'S','y','s','t','e','m','C','e','r','t','i','f','i','c','a','t','e','s','\\',\
 'T','r','u','s','t','e','d','P','u','b','l','i','s','h','e','r','\\',\
 'S','a','f','e','r',0 }
#define CERT_TRUST_PUB_AUTHENTICODE_FLAGS_VALUE_NAME (const WCHAR[])\
{'A','u','t','h','e','n','t','i','c','o','d','e','F','l','a','g','s',0 };
#elif defined(_MSC_VER)
#define CERT_TRUST_PUB_SAFER_LOCAL_MACHINE_REGPATH \
 CERT_LOCAL_MACHINE_SYSTEM_STORE_REGPATH L"\\TrustedPublisher\\Safer"
#define CERT_TRUST_PUB_AUTHENTICODE_FLAGS_VALUE_NAME L"AuthenticodeFlags"
#else
static const WCHAR CERT_TRUST_PUB_SAFER_LOCAL_MACHINE_REGPATH[] =
 {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
  'S','y','s','t','e','m','C','e','r','t','i','f','i','c','a','t','e','s','\\',
  'T','r','u','s','t','e','d','P','u','b','l','i','s','h','e','r','\\',
  'S','a','f','e','r',0 };
static const WCHAR CERT_TRUST_PUB_AUTHENTICODE_FLAGS_VALUE_NAME[] =
 { 'A','u','t','h','e','n','t','i','c','o','d','e','F','l','a','g','s',0 };
#endif

#define CERT_TRUST_PUB_ALLOW_END_USER_TRUST         0x00000000
#define CERT_TRUST_PUB_ALLOW_MACHINE_ADMIN_TRUST    0x00000001
#define CERT_TRUST_PUB_ALLOW_ENTERPRISE_ADMIN_TRUST 0x00000002
#define CERT_TRUST_PUB_ALLOW_TRUST_MASK             0x00000003
#define CERT_TRUST_PUB_CHECK_PUBLISHER_REV_FLAG     0x00000100
#define CERT_TRUST_PUB_CHECK_TIMESTAMP_REV_FLAG     0x00000200

/* flags for CertOpenStore dwFlags */
#define CERT_STORE_NO_CRYPT_RELEASE_FLAG            0x00000001
#define CERT_STORE_SET_LOCALIZED_NAME_FLAG          0x00000002
#define CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG 0x00000004
#define CERT_STORE_DELETE_FLAG                      0x00000010
#define CERT_STORE_UNSAFE_PHYSICAL_FLAG             0x00000020
#define CERT_STORE_SHARE_STORE_FLAG                 0x00000040
#define CERT_STORE_SHARE_CONTEXT_FLAG               0x00000080
#define CERT_STORE_MANIFOLD_FLAG                    0x00000100
#define CERT_STORE_ENUM_ARCHIVED_FLAG               0x00000200
#define CERT_STORE_UPDATE_KEYID_FLAG                0x00000400
#define CERT_STORE_BACKUP_RESTORE_FLAG              0x00000800
#define CERT_STORE_MAXIMUM_ALLOWED_FLAG             0x00001000
#define CERT_STORE_CREATE_NEW_FLAG                  0x00002000
#define CERT_STORE_OPEN_EXISTING_FLAG               0x00004000
#define CERT_STORE_READONLY_FLAG                    0x00008000

#define CERT_REGISTRY_STORE_REMOTE_FLAG      0x00010000
#define CERT_REGISTRY_STORE_SERIALIZED_FLAG  0x00020000
#define CERT_REGISTRY_STORE_ROAMING_FLAG     0x00040000
#define CERT_REGISTRY_STORE_MY_IE_DIRTY_FLAG 0x00080000
#define CERT_REGISTRY_STORE_LM_GPT_FLAG      0x01000000
#define CERT_REGISTRY_STORE_CLIENT_GPT_FLAG  0x80000000

#define CERT_FILE_STORE_COMMIT_ENABLE_FLAG 0x00010000

/* CertCloseStore dwFlags */
#define CERT_CLOSE_STORE_FORCE_FLAG 0x00000001
#define CERT_CLOSE_STORE_CHECK_FLAG 0x00000002

/* dwAddDisposition */
#define CERT_STORE_ADD_NEW                                 1
#define CERT_STORE_ADD_USE_EXISTING                        2
#define CERT_STORE_ADD_REPLACE_EXISTING                    3
#define CERT_STORE_ADD_ALWAYS                              4
#define CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES 5
#define CERT_STORE_ADD_NEWER                               6
#define CERT_STORE_ADD_NEWER_INHERIT_PROPERTIES            7

/* Installable OID function defs */
#define CRYPT_OID_OPEN_STORE_PROV_FUNC     "CertDllOpenStoreProv"
#define CRYPT_OID_ENCODE_OBJECT_FUNC       "CryptDllEncodeObject"
#define CRYPT_OID_DECODE_OBJECT_FUNC       "CryptDllDecodeObject"
#define CRYPT_OID_ENCODE_OBJECT_EX_FUNC    "CryptDllEncodeObjectEx"
#define CRYPT_OID_DECODE_OBJECT_EX_FUNC    "CryptDllDecodeObjectEx"
#define CRYPT_OID_CREATE_COM_OBJECT_FUNC   "CryptDllCreateComObject"
#define CRYPT_OID_VERIFY_REVOCATION_FUNC   "CertDllVerifyRevocation"
#define CRYPT_OID_VERIFY_CTL_USAGE_FUNC    "CertDllVerifyCTLUsage"
#define CRYPT_OID_FORMAT_OBJECT_FUNC       "CryptDllFormatObject"
#define CRYPT_OID_FIND_OID_INFO_FUNC       "CryptDllFindOIDInfo"
#define CRYPT_OID_FIND_LOCALIZED_NAME_FUNC "CryptDllFindLocalizedName"
#define CRYPT_OID_EXPORT_PUBLIC_KEY_INFO_FUNC  "CryptDllExportPublicKeyInfoEx"
#define CRYPT_OID_IMPORT_PUBLIC_KEY_INFO_FUNC  "CryptDllImportPublicKeyInfoEx"
#define CRYPT_OID_EXPORT_PRIVATE_KEY_INFO_FUNC "CryptDllExportPrivateKeyInfoEx"
#define CRYPT_OID_IMPORT_PRIVATE_KEY_INFO_FUNC "CryptDllImportPrivateKeyInfoEx"
#define CRYPT_OID_VERIFY_CERTIFICATE_CHAIN_POLICY_FUNC \
 "CertDllVerifyCertificateChainPolicy"
#define URL_OID_GET_OBJECT_URL_FUNC    "UrlDllGetObjectUrl"
#define TIME_VALID_OID_GET_OBJECT_FUNC "TimeValidDllGetObject"
#define CMSG_OID_GEN_CONTENT_ENCRYPT_KEY_FUNC "CryptMsgDllGenContentEncryptKey"
#define CMSG_OID_EXPORT_KEY_TRANS_FUNC        "CryptMsgDllExportKeyTrans"
#define CMSG_OID_IMPORT_KEY_TRANS_FUNC        "CryptMsgDllImportKeyTrans"

#define CRYPT_OID_REGPATH "Software\\Microsoft\\Cryptography\\OID"
#define CRYPT_OID_REG_ENCODING_TYPE_PREFIX "EncodingType "
#if defined(__GNUC__)
# define CRYPT_OID_REG_DLL_VALUE_NAME (const WCHAR []){ 'D','l','l',0 }
# define CRYPT_OID_REG_FUNC_NAME_VALUE_NAME \
 (const WCHAR []){ 'F','u','n','c','N','a','m','e',0 }
# define CRYPT_OID_REG_FLAGS_VALUE_NAME \
 (const WCHAR []){ 'C','r','y','p','t','F','l','a','g','s',0 }
#elif defined(_MSC_VER)
# define CRYPT_OID_REG_DLL_VALUE_NAME       L"Dll"
# define CRYPT_OID_REG_FUNC_NAME_VALUE_NAME L"FuncName"
# define CRYPT_OID_REG_FLAGS_VALUE_NAME     L"CryptFlags"
#else
static const WCHAR CRYPT_OID_REG_DLL_VALUE_NAME[] = { 'D','l','l',0 };
static const WCHAR CRYPT_OID_REG_FUNC_NAME_VALUE_NAME[] =
 { 'F','u','n','c','N','a','m','e',0 };
static const WCHAR CRYPT_OID_REG_FLAGS_VALUE_NAME[] =
 { 'C','r','y','p','t','F','l','a','g','s',0 };
#endif
#define CRYPT_OID_REG_FUNC_NAME_VALUE_NAME_A "FuncName"
#define CRYPT_DEFAULT_OID                    "DEFAULT"

#define CRYPT_INSTALL_OID_FUNC_BEFORE_FLAG 1

#define CRYPT_GET_INSTALLED_OID_FUNC_FLAG  0x1

#define CRYPT_REGISTER_FIRST_INDEX 0
#define CRYPT_REGISTER_LAST_INDEX  0xffffffff

/* values for CERT_STORE_PROV_INFO's dwStoreProvFlags */
#define CERT_STORE_PROV_EXTERNAL_FLAG        0x1
#define CERT_STORE_PROV_DELETED_FLAG         0x2
#define CERT_STORE_PROV_NO_PERSIST_FLAG      0x4
#define CERT_STORE_PROV_SYSTEM_STORE_FLAG    0x8
#define CERT_STORE_PROV_LM_SYSTEM_STORE_FLAG 0x10

/* function indices */
#define CERT_STORE_PROV_CLOSE_FUNC             0
#define CERT_STORE_PROV_READ_CERT_FUNC         1
#define CERT_STORE_PROV_WRITE_CERT_FUNC        2
#define CERT_STORE_PROV_DELETE_CERT_FUNC       3
#define CERT_STORE_PROV_SET_CERT_PROPERTY_FUNC 4
#define CERT_STORE_PROV_READ_CRL_FUNC          5
#define CERT_STORE_PROV_WRITE_CRL_FUNC         6
#define CERT_STORE_PROV_DELETE_CRL_FUNC        7
#define CERT_STORE_PROV_SET_CRL_PROPERTY_FUNC  8
#define CERT_STORE_PROV_READ_CTL_FUNC          9
#define CERT_STORE_PROV_WRITE_CTL_FUNC         10
#define CERT_STORE_PROV_DELETE_CTL_FUNC        11
#define CERT_STORE_PROV_SET_CTL_PROPERTY_FUNC  12
#define CERT_STORE_PROV_CONTROL_FUNC           13
#define CERT_STORE_PROV_FIND_CERT_FUNC         14
#define CERT_STORE_PROV_FREE_FIND_CERT_FUNC    15
#define CERT_STORE_PROV_GET_CERT_PROPERTY_FUNC 16
#define CERT_STORE_PROV_FIND_CRL_FUNC          17
#define CERT_STORE_PROV_FREE_FIND_CRL_FUNC     18
#define CERT_STORE_PROV_GET_CRL_PROPERTY_FUNC  19
#define CERT_STORE_PROV_FIND_CTL_FUNC          20
#define CERT_STORE_PROV_FREE_FIND_CTL_FUNC     21
#define CERT_STORE_PROV_GET_CTL_PROPERTY_FUNC  22

/* physical store dwFlags, also used by CertAddStoreToCollection as
 * dwUpdateFlags
 */
#define CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG                  0x1
#define CERT_PHYSICAL_STORE_OPEN_DISABLE_FLAG                0x2
#define CERT_PHYSICAL_STORE_REMOVE_OPEN_DISABLE_FLAG         0x4
#define CERT_PHYSICAL_STORE_INSERT_COMPUTER_NAME_ENABLE_FLAG 0x8

/* dwFlag values for CertEnumPhysicalStore callback */
#define CERT_PHYSICAL_STORE_PREDEFINED_ENUM_FLAG 0x1

/* predefined store names */
#if defined(__GNUC__)
# define CERT_PHYSICAL_STORE_DEFAULT_NAME (const WCHAR[])\
 {'.','D','e','f','a','u','l','t','0'}
# define CERT_PHYSICAL_STORE_GROUP_POLICY_NAME (const WCHAR[])\
 {'.','G','r','o','u','p','P','o','l','i','c','y',0}
# define CERT_PHYSICAL_STORE_LOCAL_MACHINE_NAME (const WCHAR[])\
 {'.','L','o','c','a','l','M','a','c','h','i','n','e',0}
# define CERT_PHYSICAL_STORE_DS_USER_CERTIFICATE_NAME (const WCHAR[])\
 {'.','U','s','e','r','C','e','r','t','i','f','i','c','a','t','e',0}
# define CERT_PHYSICAL_STORE_LOCAL_MACHINE_GROUP_POLICY_NAME (const WCHAR[])\
 {'.','L','o','c','a','l','M','a','c','h','i','n','e','G','r','o','u','p',\
 'P','o','l','i','c','y',0}
# define CERT_PHYSICAL_STORE_ENTERPRISE_NAME (const WCHAR[])\
 {'.','E','n','t','e','r','p','r','i','s','e',0}
# define CERT_PHYSICAL_STORE_AUTH_ROOT_NAME (const WCHAR[])\
 {'.','A','u','t','h','R','o','o','t',0}
#elif defined(_MSC_VER)
# define CERT_PHYSICAL_STORE_DEFAULT_NAME \
 L".Default"
# define CERT_PHYSICAL_STORE_GROUP_POLICY_NAME \
 L".GroupPolicy"
# define CERT_PHYSICAL_STORE_LOCAL_MACHINE_NAME \
 L".LocalMachine"
# define CERT_PHYSICAL_STORE_DS_USER_CERTIFICATE_NAME \
 L".UserCertificate"
# define CERT_PHYSICAL_STORE_LOCAL_MACHINE_GROUP_POLICY_NAME \
 L".LocalMachineGroupPolicy"
# define CERT_PHYSICAL_STORE_ENTERPRISE_NAME \
 L".Enterprise"
# define CERT_PHYSICAL_STORE_AUTH_ROOT_NAME \
 L".AuthRoot"
#else
static const WCHAR CERT_PHYSICAL_STORE_DEFAULT_NAME[] = 
 {'.','D','e','f','a','u','l','t','0'};
static const WCHAR CERT_PHYSICAL_STORE_GROUP_POLICY_NAME[] =
 {'.','G','r','o','u','p','P','o','l','i','c','y',0};
static const WCHAR CERT_PHYSICAL_STORE_LOCAL_MACHINE_NAME[] =
 {'.','L','o','c','a','l','M','a','c','h','i','n','e',0};
static const WCHAR CERT_PHYSICAL_STORE_DS_USER_CERTIFICATE_NAME[] =
 {'.','U','s','e','r','C','e','r','t','i','f','i','c','a','t','e',0};
static const WCHAR CERT_PHYSICAL_STORE_LOCAL_MACHINE_GROUP_POLICY_NAME[] =
 {'.','L','o','c','a','l','M','a','c','h','i','n','e','G','r','o','u','p',
 'P','o','l','i','c','y',0};
static const WCHAR CERT_PHYSICAL_STORE_ENTERPRISE_NAME[] =
 {'.','E','n','t','e','r','p','r','i','s','e',0};
static const WCHAR CERT_PHYSICAL_STORE_AUTH_ROOT_NAME[] =
 {'.','A','u','t','h','R','o','o','t',0};
#endif

/* cert system store flags */
#define CERT_SYSTEM_STORE_MASK 0xffff0000
#define CERT_SYSTEM_STORE_RELOCATE_FLAG 0x80000000

/* CertFindChainInStore dwFindType types */
#define CERT_CHAIN_FIND_BY_ISSUER 1

/* CertSaveStore dwSaveAs values */
#define CERT_STORE_SAVE_AS_STORE 1
#define CERT_STORE_SAVE_AS_PKCS7 2
/* CertSaveStore dwSaveTo values */
#define CERT_STORE_SAVE_TO_FILE       1
#define CERT_STORE_SAVE_TO_MEMORY     2
#define CERT_STORE_SAVE_TO_FILENAME_A 3
#define CERT_STORE_SAVE_TO_FILENAME_W 4
#define CERT_STORE_SAVE_TO_FILENAME   CERT_STORE_SAVE_TO_FILENAME_W

/* CERT_INFO versions/flags */
#define CERT_V1 0
#define CERT_V2 1
#define CERT_V3 2
#define CERT_INFO_VERSION_FLAG                 1
#define CERT_INFO_SERIAL_NUMBER_FLAG           2
#define CERT_INFO_SIGNATURE_ALGORITHM_FLAG     3
#define CERT_INFO_ISSUER_FLAG                  4
#define CERT_INFO_NOT_BEFORE_FLAG              5
#define CERT_INFO_NOT_AFTER_FLAG               6
#define CERT_INFO_SUBJECT_FLAG                 7
#define CERT_INFO_SUBJECT_PUBLIC_KEY_INFO_FLAG 8
#define CERT_INFO_ISSUER_UNIQUE_ID_FLAG        9
#define CERT_INFO_SUBJECT_UNIQUE_ID_FLAG       10
#define CERT_INFO_EXTENSION_FLAG               11

/* CERT_REQUEST_INFO versions */
#define CERT_REQUEST_V1 0

/* CERT_KEYGEN_REQUEST_INFO versions */
#define CERT_KEYGEN_REQUEST_V1 0

/* CRL versions */
#define CRL_V1 0
#define CRL_V2 1

/* CTL versions */
#define CTL_V1 0

/* Certificate, CRL, CTL property IDs */
#define CERT_KEY_PROV_HANDLE_PROP_ID               1
#define CERT_KEY_PROV_INFO_PROP_ID                 2
#define CERT_SHA1_HASH_PROP_ID                     3
#define CERT_HASH_PROP_ID                          CERT_SHA1_HASH_PROP_ID
#define CERT_MD5_HASH_PROP_ID                      4
#define CERT_KEY_CONTEXT_PROP_ID                   5
#define CERT_KEY_SPEC_PROP_ID                      6
#define CERT_IE30_RESERVED_PROP_ID                 7
#define CERT_PUBKEY_HASH_RESERVED_PROP_ID          8
#define CERT_ENHKEY_USAGE_PROP_ID                  9
#define CERT_CTL_USAGE_PROP_ID                     CERT_ENHKEY_USAGE_PROP_ID
#define CERT_NEXT_UPDATE_LOCATION_PROP_ID          10
#define CERT_FRIENDLY_NAME_PROP_ID                 11
#define CERT_PVK_FILE_PROP_ID                      12
#define CERT_DESCRIPTION_PROP_ID                   13
#define CERT_ACCESS_STATE_PROP_ID                  14
#define CERT_SIGNATURE_HASH_PROP_ID                15
#define CERT_SMART_CARD_DATA_PROP_ID               16
#define CERT_EFS_PROP_ID                           17
#define CERT_FORTEZZA_DATA_PROP                    18
#define CERT_ARCHIVED_PROP_ID                      19
#define CERT_KEY_IDENTIFIER_PROP_ID                20
#define CERT_AUTO_ENROLL_PROP_ID                   21
#define CERT_PUBKEY_ALG_PARA_PROP_ID               22
#define CERT_CROSS_CERT_DIST_POINTS_PROP_ID        23
#define CERT_ISSUER_PUBLIC_KEY_MD5_HASH_PROP_ID    24
#define CERT_SUBJECT_PUBLIC_KEY_MD5_HASH_PROP_ID   25
#define CERT_ENROLLMENT_PROP_ID                    26
#define CERT_DATE_STAMP_PROP_ID                    27
#define CERT_ISSUER_SERIAL_NUMBER_MD5_HASH_PROP_ID 28
#define CERT_SUBJECT_NAME_MD5_HASH_PROP_ID         29
#define CERT_EXTENDED_ERROR_INFO_PROP_ID           30
/* 31    -- unused?
   32    -- cert prop id
   33    -- CRL prop id
   34    -- CTL prop id
   35    -- KeyId prop id
   36-63 -- reserved
 */
#define CERT_RENEWAL_PROP_ID                       64
#define CERT_ARCHIVED_KEY_HASH_PROP_ID             65
#define CERT_AUTO_ENROLL_RETRY_PROP_ID             66
#define CERT_AIA_URL_RETRIEVED_PROP_ID             67
#define CERT_AUTHORITY_INFO_ACCESS_PROP_ID         68
#define CERT_BACKED_UP_PROP_ID                     69
#define CERT_OCSP_RESPONSE_PROP_ID                 70
#define CERT_REQUEST_ORIGINATOR_PROP_ID            71
#define CERT_SOURCE_LOCATION_PROP_ID               72
#define CERT_SOURCE_URL_PROP_ID                    73
#define CERT_NEW_KEY_PROP_ID                       74
#define CERT_OCSP_CACHE_PREFIX_PROP_ID             75
#define CERT_SMART_CARD_ROOT_INFO_PROP_ID          76
#define CERT_NO_AUTO_EXPIRE_CHECK_PROP_ID          77
#define CERT_NCRYPT_KEY_HANDLE_PROP_ID             78
#define CERT_HCRYPTPROV_OR_NCRYPT_KEY_HANDLE_PROP_ID 79
#define CERT_SUBJECT_INFO_ACCESS_PROP_ID           80
#define CERT_CA_OCSP_AUTHORITY_INFO_ACCESS_PROP_ID 81
#define CERT_CA_DISABLE_CRL_PROP_ID                82
#define CERT_ROOT_PROGRAM_CERT_POLICIES_PROP_ID    83
#define CERT_ROOT_PROGRAM_NAME_CONSTRAINTS_PROP_ID 84

#define CERT_FIRST_RESERVED_PROP_ID                85
#define CERT_LAST_RESERVED_PROP_ID                 0x00007fff
#define CERT_FIRST_USER_PROP_ID                    0x00008000
#define CERT_LAST_USER_PROP_ID                     0x0000ffff

#define IS_CERT_HASH_PROP_ID(x) \
 ((x) == CERT_SHA1_HASH_PROP_ID || (x) == CERT_MD5_HASH_PROP_ID || \
  (x) == CERT_SIGNATURE_HASH_PROP_ID)

#define IS_PUBKEY_HASH_PROP_ID(x) \
 ((x) == CERT_ISSUER_PUBLIC_KEY_MD5_HASH_PROP_ID || \
  (x) == CERT_SUBJECT_PUBLIC_KEY_MD5_HASH_PROP_ID)

#define IS_CHAIN_HASH_PROP_ID(x) \
 ((x) == CERT_ISSUER_PUBLIC_KEY_MD5_HASH_PROP_ID || \
  (x) == CERT_SUBJECT_PUBLIC_KEY_MD5_HASH_PROP_ID || \
  (x) == CERT_ISSUER_SERIAL_NUMBER_MD5_HASH_PROP_ID || \
  (x) == CERT_SUBJECT_NAME_MD5_HASH_PROP_ID)

/* access state flags */
#define CERT_ACCESS_STATE_WRITE_PERSIST_FLAG   0x1
#define CERT_ACCESS_STATE_SYSTEM_STORE_FLAG    0x2
#define CERT_ACCESS_STATE_LM_SYSTEM_STORE_FLAG 0x4

/* CertSetCertificateContextProperty flags */
#define CERT_SET_PROPERTY_INHIBIT_PERSIST_FLAG      0x40000000
#define CERT_SET_PROPERTY_IGNORE_PERSIST_ERROR_FLAG 0x80000000

/* CERT_RDN attribute dwValueType types */
#define CERT_RDN_TYPE_MASK 0x000000ff
#define CERT_RDN_ANY_TYPE         0
#define CERT_RDN_ENCODED_BLOB     1
#define CERT_RDN_OCTET_STRING     2
#define CERT_RDN_NUMERIC_STRING   3
#define CERT_RDN_PRINTABLE_STRING 4
#define CERT_RDN_TELETEX_STRING   5
#define CERT_RDN_T61_STRING       5
#define CERT_RDN_VIDEOTEX_STRING  6
#define CERT_RDN_IA5_STRING       7
#define CERT_RDN_GRAPHIC_STRING   8
#define CERT_RDN_VISIBLE_STRING   9
#define CERT_RDN_ISO646_STRING    9
#define CERT_RDN_GENERAL_STRING   10
#define CERT_RDN_UNIVERSAL_STRING 11
#define CERT_RDN_INT4_STRING      11
#define CERT_RDN_BMP_STRING       12
#define CERT_RDN_UNICODE_STRING   12
#define CERT_RDN_UTF8_STRING      13

/* CERT_RDN attribute dwValueType flags */
#define CERT_RDN_FLAGS_MASK 0xff000000
#define CERT_RDN_ENABLE_T61_UNICODE_FLAG  0x80000000
#define CERT_RDN_DISABLE_CHECK_TYPE_FLAG  0x4000000
#define CERT_RDN_ENABLE_UTF8_UNICODE_FLAG 0x2000000
#define CERT_RDN_DISABLE_IE4_UTF8_FLAG    0x0100000

#define IS_CERT_RDN_CHAR_STRING(x) \
 (((x) & CERT_RDN_TYPE_MASK) >= CERT_RDN_NUMERIC_STRING)

/* CertIsRDNAttrsInCertificateName flags */
#define CERT_UNICODE_IS_RDN_ATTRS_FLAG          0x1
#define CERT_CASE_INSENSITIVE_IS_RDN_ATTRS_FLAG 0x2

/* CRL reason codes */
#define CRL_REASON_UNSPECIFIED            0
#define CRL_REASON_KEY_COMPROMISE         1
#define CRL_REASON_CA_COMPROMISE          2
#define CRL_REASON_AFFILIATION_CHANGED    3
#define CRL_REASON_SUPERSEDED             4
#define CRL_REASON_CESSATION_OF_OPERATION 5
#define CRL_REASON_CERTIFICATE_HOLD       6
#define CRL_REASON_REMOVE_FROM_CRL        8

/* CertControlStore control types */
#define CERT_STORE_CTRL_RESYNC        1
#define CERT_STORE_CTRL_NOTIFY_CHANGE 2
#define CERT_STORE_CTRL_COMMIT        3
#define CERT_STORE_CTRL_AUTO_RESYNC   4
#define CERT_STORE_CTRL_CANCEL_NOTIFY 5

#define CERT_STORE_CTRL_COMMIT_FORCE_FLAG 0x1
#define CERT_STORE_CTRL_COMMIT_CLEAR_FLAG 0x2

/* cert store properties */
#define CERT_STORE_LOCALIZED_NAME_PROP_ID 0x1000

/* CertCreateContext flags */
#define CERT_CREATE_CONTEXT_NOCOPY_FLAG       0x1
#define CERT_CREATE_CONTEXT_SORTED_FLAG       0x2
#define CERT_CREATE_CONTEXT_NO_HCRYPTMSG_FLAG 0x4
#define CERT_CREATE_CONTEXT_NO_ENTRY_FLAG     0x8

#define CERT_COMPARE_MASK                   0xffff
#define CERT_COMPARE_SHIFT                  16
#define CERT_COMPARE_ANY                    0
#define CERT_COMPARE_SHA1_HASH              1
#define CERT_COMPARE_HASH                   CERT_COMPARE_SHA1_HASH
#define CERT_COMPARE_NAME                   2
#define CERT_COMPARE_ATTR                   3
#define CERT_COMPARE_MD5_HASH               4
#define CERT_COMPARE_PROPERTY               5
#define CERT_COMPARE_PUBLIC_KEY             6
#define CERT_COMPARE_NAME_STR_A             7
#define CERT_COMPARE_NAME_STR_W             8
#define CERT_COMPARE_KEY_SPEC               9
#define CERT_COMPARE_ENHKEY_USAGE           10
#define CERT_COMPARE_CTL_USAGE              CERT_COMPARE_ENHKEY_USAGE
#define CERT_COMPARE_SUBJECT_CERT           11
#define CERT_COMPARE_ISSUER_OF              12
#define CERT_COMPARE_EXISTING               13
#define CERT_COMPARE_SIGNATURE_HASH         14
#define CERT_COMPARE_KEY_IDENTIFIER         15
#define CERT_COMPARE_CERT_ID                16
#define CERT_COMPARE_CROSS_CERT_DIST_POINTS 17
#define CERT_COMPARE_PUBKEY_MD5_HASH        18

/* values of dwFindType for CertFind*InStore */
#define CERT_FIND_ANY \
 (CERT_COMPARE_ANY << CERT_COMPARE_SHIFT)
#define CERT_FIND_SHA1_HASH \
 (CERT_COMPARE_SHA1_HASH << CERT_COMPARE_SHIFT)
#define CERT_FIND_MD5_HASH \
 (CERT_COMPARE_MD5_HASH << CERT_COMPARE_SHIFT)
#define CERT_FIND_SIGNATURE_HASH \
 (CERT_COMPARE_SIGNATURE_HASH << CERT_COMPARE_SHIFT)
#define CERT_FIND_KEY_IDENTIFIER \
 (CERT_COMPARE_KEY_IDENTIFIER << CERT_COMPARE_SHIFT)
#define CERT_FIND_HASH CERT_FIND_SHA1_HASH
#define CERT_FIND_PROPERTY \
 (CERT_COMPARE_PROPERTY << CERT_COMPARE_SHIFT)
#define CERT_FIND_PUBLIC_KEY \
 (CERT_COMPARE_PUBLIC_KEY << CERT_COMPARE_SHIFT)
#define CERT_FIND_SUBJECT_NAME \
 (CERT_COMPARE_NAME << CERT_COMPARE_SHIFT | CERT_INFO_SUBJECT_FLAG)
#define CERT_FIND_SUBJECT_ATTR \
 (CERT_COMPARE_ATTR << CERT_COMPARE_SHIFT | CERT_INFO_SUBJECT_FLAG)
#define CERT_FIND_ISSUER_NAME \
 (CERT_COMPARE_NAME << CERT_COMPARE_SHIFT | CERT_INFO_ISSUER_FLAG)
#define CERT_FIND_ISSUER_ATTR \
 (CERT_COMPARE_ATTR << CERT_COMPARE_SHIFT | CERT_INFO_ISSUER_FLAG)
#define CERT_FIND_SUBJECT_STR_A \
 (CERT_COMPARE_NAME_STR_A << CERT_COMPARE_SHIFT | CERT_INFO_SUBJECT_FLAG)
#define CERT_FIND_SUBJECT_STR_W \
 (CERT_COMPARE_NAME_STR_W << CERT_COMPARE_SHIFT | CERT_INFO_SUBJECT_FLAG)
#define CERT_FIND_SUBJECT_STR CERT_FIND_SUBJECT_STR_W
#define CERT_FIND_ISSUER_STR_A \
 (CERT_COMPARE_NAME_STR_A << CERT_COMPARE_SHIFT | CERT_INFO_ISSUER_FLAG)
#define CERT_FIND_ISSUER_STR_W \
 (CERT_COMPARE_NAME_STR_W << CERT_COMPARE_SHIFT | CERT_INFO_ISSUER_FLAG)
#define CERT_FIND_ISSUER_STR CERT_FIND_ISSUER_STR_W
#define CERT_FIND_KEY_SPEC \
 (CERT_COMPARE_KEY_SPEC << CERT_COMPARE_SHIFT)
#define CERT_FIND_ENHKEY_USAGE \
 (CERT_COMPARE_ENHKEY_USAGE << CERT_COMPARE_SHIFT)
#define CERT_FIND_CTL_USAGE CERT_FIND_ENHKEY_USAGE
#define CERT_FIND_SUBJECT_CERT \
 (CERT_COMPARE_SUBJECT_CERT << CERT_COMPARE_SHIFT)
#define CERT_FIND_ISSUER_OF \
 (CERT_COMPARE_ISSUER_OF << CERT_COMPARE_SHIFT)
#define CERT_FIND_EXISTING \
 (CERT_COMPARE_EXISTING << CERT_COMPARE_SHIFT)
#define CERT_FIND_CERT_ID \
 (CERT_COMPARE_CERT_ID << CERT_COMPARE_SHIFT)
#define CERT_FIND_CROSS_CERT_DIST_POINTS \
 (CERT_COMPARE_CROSS_CERT_DIST_POINTS << CERT_COMPARE_SHIFT)
#define CERT_FIND_PUBKEY_MD5_HASH \
 (CERT_COMPARE_PUBKEY_MD5_HASH << CERT_COMPARE_SHIFT)

#define CERT_FIND_OPTIONAL_ENHKEY_USAGE_FLAG  0x1
#define CERT_FIND_OPTIONAL_CTL_USAGE_FLAG     0x1
#define CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG  0x2
#define CERT_FIND_EXT_ONLY_CTL_USAGE_FLAG     0x2
#define CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG 0x4
#define CERT_FIND_PROP_ONLY_CTL_USAGE_FLAG    0x4
#define CERT_FIND_NO_ENHKEY_USAGE_FLAG        0x8
#define CERT_FIND_NO_CTL_USAGE_FLAG           0x8
#define CERT_FIND_OR_ENHKEY_USAGE_FLAG        0x10
#define CERT_FIND_OR_CTL_USAGE_FLAG           0x10
#define CERT_FIND_VALID_ENHKEY_USAGE_FLAG     0x20
#define CERT_FIND_VALID_CTL_USAGE_FLAG        0x20

#define CRL_FIND_ANY        0
#define CRL_FIND_ISSUED_BY  1
#define CRL_FIND_EXISTING   2
#define CRL_FIND_ISSUED_FOR 3

#define CRL_FIND_ISSUED_BY_AKI_FLAG       0x1
#define CRL_FIND_ISSUED_BY_SIGNATURE_FLAG 0x2
#define CRL_FIND_ISSUED_BY_DELTA_FLAG     0x4
#define CRL_FIND_ISSUED_BY_BASE_FLAG      0x8

typedef struct _CRL_FIND_ISSUED_FOR_PARA
{
    PCCERT_CONTEXT pSubjectCert;
    PCCERT_CONTEXT pIssuerCert;
} CRL_FIND_ISSUED_FOR_PARA, *PCRL_FIND_ISSUED_FOR_PARA;

#define CTL_FIND_ANY       0
#define CTL_FIND_SHA1_HASH 1
#define CTL_FIND_MD5_HASH  2
#define CTL_FIND_USAGE     3
#define CTL_FIND_SUBJECT   4
#define CTL_FIND_EXISTING  5

typedef struct _CTL_FIND_USAGE_PARA
{
    DWORD           cbSize;
    CTL_USAGE       SubjectUsage;
    CRYPT_DATA_BLOB ListIdentifier;
    PCERT_INFO      pSigner;
} CTL_FIND_USAGE_PARA, *PCTL_FIND_USAGE_PARA;

#define CTL_FIND_NO_LIST_ID_CBDATA 0xffffffff
#define CTL_FIND_NO_SIGNER_PTR     ((PCERT_INFO)-1)
#define CTL_FIND_SAME_USAGE_FLAG   0x00000001

typedef struct _CTL_FIND_SUBJECT_PARA
{
    DWORD                cbSize;
    PCTL_FIND_USAGE_PARA pUsagePara;
    DWORD                dwSubjectType;
    void                *pvSubject;
} CTL_FIND_SUBJECT_PARA, *PCTL_FIND_SUBJECT_PARA;

/* PFN_CERT_STORE_PROV_WRITE_CERT dwFlags values */
#define CERT_STORE_PROV_WRITE_ADD_FLAG 0x1

/* CertAddSerializedElementToStore context types */
#define CERT_STORE_CERTIFICATE_CONTEXT 1
#define CERT_STORE_CRL_CONTEXT         2
#define CERT_STORE_CTL_CONTEXT         3
#define CERT_STORE_ALL_CONTEXT_FLAG    ~0U
#define CERT_STORE_CERTIFICATE_CONTEXT_FLAG \
                                    (1 << CERT_STORE_CERTIFICATE_CONTEXT)
#define CERT_STORE_CRL_CONTEXT_FLAG (1 << CERT_STORE_CRL_CONTEXT)
#define CERT_STORE_CTL_CONTEXT_FLAG (1 << CERT_STORE_CTL_CONTEXT)

/* CryptBinaryToString/CryptStringToBinary flags */
#define CRYPT_STRING_BASE64HEADER        0x00000000
#define CRYPT_STRING_BASE64              0x00000001
#define CRYPT_STRING_BINARY              0x00000002
#define CRYPT_STRING_BASE64REQUESTHEADER 0x00000003
#define CRYPT_STRING_HEX                 0x00000004
#define CRYPT_STRING_HEXASCII            0x00000005
#define CRYPT_STRING_BASE64_ANY          0x00000006
#define CRYPT_STRING_ANY                 0x00000007
#define CRYPT_STRING_HEX_ANY             0x00000008
#define CRYPT_STRING_BASE64X509CRLHEADER 0x00000009
#define CRYPT_STRING_HEXADDR             0x0000000a
#define CRYPT_STRING_HEXASCIIADDR        0x0000000b
#define CRYPT_STRING_NOCRLF              0x40000000
#define CRYPT_STRING_NOCR                0x80000000

/* OIDs */
#define szOID_RSA                           "1.2.840.113549"
#define szOID_PKCS                          "1.2.840.113549.1"
#define szOID_RSA_HASH                      "1.2.840.113549.2"
#define szOID_RSA_ENCRYPT                   "1.2.840.113549.3"
#define szOID_PKCS_1                        "1.2.840.113549.1.1"
#define szOID_PKCS_2                        "1.2.840.113549.1.2"
#define szOID_PKCS_3                        "1.2.840.113549.1.3"
#define szOID_PKCS_4                        "1.2.840.113549.1.4"
#define szOID_PKCS_5                        "1.2.840.113549.1.5"
#define szOID_PKCS_6                        "1.2.840.113549.1.6"
#define szOID_PKCS_7                        "1.2.840.113549.1.7"
#define szOID_PKCS_8                        "1.2.840.113549.1.8"
#define szOID_PKCS_9                        "1.2.840.113549.1.9"
#define szOID_PKCS_10                       "1.2.840.113549.1.10"
#define szOID_PKCS_11                       "1.2.840.113549.1.12"
#define szOID_RSA_RSA                       "1.2.840.113549.1.1.1"
#define CERT_RSA_PUBLIC_KEY_OBJID           szOID_RSA_RSA
#define CERT_DEFAULT_OID_PUBLIC_KEY_SIGN    szOID_RSA_RSA
#define CERT_DEFAULT_OID_PUBLIC_KEY_XCHG    szOID_RSA_RSA
#define szOID_RSA_MD2RSA                    "1.2.840.113549.1.1.2"
#define szOID_RSA_MD4RSA                    "1.2.840.113549.1.1.3"
#define szOID_RSA_MD5RSA                    "1.2.840.113549.1.1.4"
#define szOID_RSA_SHA1RSA                   "1.2.840.113549.1.1.5"
#define szOID_RSA_SET0AEP_RSA               "1.2.840.113549.1.1.6"
#define szOID_RSA_SHA256RSA                 "1.2.840.113549.1.1.11"
#define szOID_RSA_SHA384RSA                 "1.2.840.113549.1.1.12"
#define szOID_RSA_SHA512RSA                 "1.2.840.113549.1.1.13"
#define szOID_RSA_DH                        "1.2.840.113549.1.3.1"
#define szOID_RSA_data                      "1.2.840.113549.1.7.1"
#define szOID_RSA_signedData                "1.2.840.113549.1.7.2"
#define szOID_RSA_envelopedData             "1.2.840.113549.1.7.3"
#define szOID_RSA_signEnvData               "1.2.840.113549.1.7.4"
#define szOID_RSA_digestedData              "1.2.840.113549.1.7.5"
#define szOID_RSA_hashedData                "1.2.840.113549.1.7.5"
#define szOID_RSA_encryptedData             "1.2.840.113549.1.7.6"
#define szOID_RSA_emailAddr                 "1.2.840.113549.1.9.1"
#define szOID_RSA_unstructName              "1.2.840.113549.1.9.2"
#define szOID_RSA_contentType               "1.2.840.113549.1.9.3"
#define szOID_RSA_messageDigest             "1.2.840.113549.1.9.4"
#define szOID_RSA_signingTime               "1.2.840.113549.1.9.5"
#define szOID_RSA_counterSign               "1.2.840.113549.1.9.6"
#define szOID_RSA_challengePwd              "1.2.840.113549.1.9.7"
#define szOID_RSA_unstructAddr              "1.2.840.113549.1.9.9"
#define szOID_RSA_extCertAttrs              "1.2.840.113549.1.9.9"
#define szOID_RSA_certExtensions            "1.2.840.113549.1.9.14"
#define szOID_RSA_SMIMECapabilities         "1.2.840.113549.1.9.15"
#define szOID_RSA_preferSignedData          "1.2.840.113549.1.9.15.1"
#define szOID_RSA_SMIMEalg                  "1.2.840.113549.1.9.16.3"
#define szOID_RSA_SMIMEalgESDH              "1.2.840.113549.1.9.16.3.5"
#define szOID_RSA_SMIMEalgCMS3DESwrap       "1.2.840.113549.1.9.16.3.6"
#define szOID_RSA_SMIMEalgCMSRC2wrap        "1.2.840.113549.1.9.16.3.7"
#define szOID_RSA_MD2                       "1.2.840.113549.2.2"
#define szOID_RSA_MD4                       "1.2.840.113549.2.4"
#define szOID_RSA_MD5                       "1.2.840.113549.2.5"
#define szOID_RSA_RC2CBC                    "1.2.840.113549.3.2"
#define szOID_RSA_RC4                       "1.2.840.113549.3.4"
#define szOID_RSA_DES_EDE3_CBC              "1.2.840.113549.3.7"
#define szOID_RSA_RC5_CBCPad                "1.2.840.113549.3.9"
#define szOID_ANSI_X942                     "1.2.840.10046"
#define szOID_ANSI_X942_DH                  "1.2.840.10046.2.1"
#define szOID_X957                          "1.2.840.10040"
#define szOID_X957_DSA                      "1.2.840.10040.4.1"
#define szOID_X957_SHA1DSA                  "1.2.840.10040.4.3"
#define szOID_DS                            "2.5"
#define szOID_DSALG                         "2.5.8"
#define szOID_DSALG_CRPT                    "2.5.8.1"
#define szOID_DSALG_HASH                    "2.5.8.2"
#define szOID_DSALG_SIGN                    "2.5.8.3"
#define szOID_DSALG_RSA                     "2.5.8.1.1"
#define szOID_OIW                           "1.3.14"
#define szOID_OIWSEC                        "1.3.14.3.2"
#define szOID_OIWSEC_md4RSA                 "1.3.14.3.2.2"
#define szOID_OIWSEC_md5RSA                 "1.3.14.3.2.3"
#define szOID_OIWSEC_md4RSA2                "1.3.14.3.2.4"
#define szOID_OIWSEC_desECB                 "1.3.14.3.2.6"
#define szOID_OIWSEC_desCBC                 "1.3.14.3.2.7"
#define szOID_OIWSEC_desOFB                 "1.3.14.3.2.8"
#define szOID_OIWSEC_desCFB                 "1.3.14.3.2.9"
#define szOID_OIWSEC_desMAC                 "1.3.14.3.2.10"
#define szOID_OIWSEC_rsaSign                "1.3.14.3.2.11"
#define szOID_OIWSEC_dsa                    "1.3.14.3.2.12"
#define szOID_OIWSEC_shaDSA                 "1.3.14.3.2.13"
#define szOID_OIWSEC_mdc2RSA                "1.3.14.3.2.14"
#define szOID_OIWSEC_shaRSA                 "1.3.14.3.2.15"
#define szOID_OIWSEC_dhCommMod              "1.3.14.3.2.16"
#define szOID_OIWSEC_desEDE                 "1.3.14.3.2.17"
#define szOID_OIWSEC_sha                    "1.3.14.3.2.18"
#define szOID_OIWSEC_mdc2                   "1.3.14.3.2.19"
#define szOID_OIWSEC_dsaComm                "1.3.14.3.2.20"
#define szOID_OIWSEC_dsaCommSHA             "1.3.14.3.2.21"
#define szOID_OIWSEC_rsaXchg                "1.3.14.3.2.22"
#define szOID_OIWSEC_keyHashSeal            "1.3.14.3.2.23"
#define szOID_OIWSEC_md2RSASign             "1.3.14.3.2.24"
#define szOID_OIWSEC_md5RSASign             "1.3.14.3.2.25"
#define szOID_OIWSEC_sha1                   "1.3.14.3.2.26"
#define szOID_OIWSEC_dsaSHA1                "1.3.14.3.2.27"
#define szOID_OIWSEC_dsaCommSHA1            "1.3.14.3.2.28"
#define szOID_OIWSEC_sha1RSASign            "1.3.14.3.2.29"
#define szOID_OIWDIR                        "1.3.14.7.2"
#define szOID_OIWDIR_CRPT                   "1.3.14.7.2.1"
#define szOID_OIWDIR_HASH                   "1.3.14.7.2.2"
#define szOID_OIWDIR_SIGN                   "1.3.14.7.2.3"
#define szOID_OIWDIR_md2                    "1.3.14.7.2.2.1"
#define szOID_OIWDIR_md2RSA                 "1.3.14.7.2.3.1"
#define szOID_INFOSEC                       "2.16.840.1.101.2.1"
#define szOID_INFOSEC_sdnsSignature         "2.16.840.1.101.2.1.1.1"
#define szOID_INFOSEC_mosaicSignature       "2.16.840.1.101.2.1.1.2"
#define szOID_INFOSEC_sdnsConfidentiality   "2.16.840.1.101.2.1.1.3"
#define szOID_INFOSEC_mosaicConfidentiality "2.16.840.1.101.2.1.1.4"
#define szOID_INFOSEC_sdnsIntegrity         "2.16.840.1.101.2.1.1.5"
#define szOID_INFOSEC_mosaicIntegrity       "2.16.840.1.101.2.1.1.6"
#define szOID_INFOSEC_sdnsTokenProtection   "2.16.840.1.101.2.1.1.7"
#define szOID_INFOSEC_mosaicTokenProtection "2.16.840.1.101.2.1.1.8"
#define szOID_INFOSEC_sdnsKeyManagement     "2.16.840.1.101.2.1.1.9"
#define szOID_INFOSEC_mosaicKeyManagement   "2.16.840.1.101.2.1.1.10"
#define szOID_INFOSEC_sdnsKMandSig          "2.16.840.1.101.2.1.1.11"
#define szOID_INFOSEC_mosaicKMandSig        "2.16.840.1.101.2.1.1.12"
#define szOID_INFOSEC_SuiteASignature       "2.16.840.1.101.2.1.1.13"
#define szOID_INFOSEC_SuiteAConfidentiality "2.16.840.1.101.2.1.1.14"
#define szOID_INFOSEC_SuiteAIntegrity       "2.16.840.1.101.2.1.1.15"
#define szOID_INFOSEC_SuiteATokenProtection "2.16.840.1.101.2.1.1.16"
#define szOID_INFOSEC_SuiteAKeyManagement   "2.16.840.1.101.2.1.1.17"
#define szOID_INFOSEC_SuiteAKMandSig        "2.16.840.1.101.2.1.1.18"
#define szOID_INFOSEC_mosaicUpdatedSig      "2.16.840.1.101.2.1.1.19"
#define szOID_INFOSEC_mosaicKMandUpdSig     "2.16.840.1.101.2.1.1.20"
#define szOID_INFOSEC_mosaicUpdateInteg     "2.16.840.1.101.2.1.1.21"
#define szOID_COMMON_NAME                   "2.5.4.3"
#define szOID_SUR_NAME                      "2.5.4.4"
#define szOID_DEVICE_SERIAL_NUMBER          "2.5.4.5"
#define szOID_COUNTRY_NAME                  "2.5.4.6"
#define szOID_LOCALITY_NAME                 "2.5.4.7"
#define szOID_STATE_OR_PROVINCE_NAME        "2.5.4.8"
#define szOID_STREET_ADDRESS                "2.5.4.9"
#define szOID_ORGANIZATION_NAME             "2.5.4.10"
#define szOID_ORGANIZATIONAL_UNIT_NAME      "2.5.4.11"
#define szOID_TITLE                         "2.5.4.12"
#define szOID_DESCRIPTION                   "2.5.4.13"
#define szOID_SEARCH_GUIDE                  "2.5.4.14"
#define szOID_BUSINESS_CATEGORY             "2.5.4.15"
#define szOID_POSTAL_ADDRESS                "2.5.4.16"
#define szOID_POSTAL_CODE                   "2.5.4.17"
#define szOID_POST_OFFICE_BOX               "2.5.4.18"
#define szOID_PHYSICAL_DELIVERY_OFFICE_NAME "2.5.4.19"
#define szOID_TELEPHONE_NUMBER              "2.5.4.20"
#define szOID_TELEX_NUMBER                  "2.5.4.21"
#define szOID_TELETEXT_TERMINAL_IDENTIFIER  "2.5.4.22"
#define szOID_FACSIMILE_TELEPHONE_NUMBER    "2.5.4.23"
#define szOID_X21_ADDRESS                   "2.5.4.24"
#define szOID_INTERNATIONAL_ISDN_NUMBER     "2.5.4.25"
#define szOID_REGISTERED_ADDRESS            "2.5.4.26"
#define szOID_DESTINATION_INDICATOR         "2.5.4.27"
#define szOID_PREFERRED_DELIVERY_METHOD     "2.5.4.28"
#define szOID_PRESENTATION_ADDRESS          "2.5.4.29"
#define szOID_SUPPORTED_APPLICATION_CONTEXT "2.5.4.30"
#define szOID_MEMBER                        "2.5.4.31"
#define szOID_OWNER                         "2.5.4.32"
#define szOID_ROLE_OCCUPANT                 "2.5.4.33"
#define szOID_SEE_ALSO                      "2.5.4.34"
#define szOID_USER_PASSWORD                 "2.5.4.35"
#define szOID_USER_CERTIFICATE              "2.5.4.36"
#define szOID_CA_CERTIFICATE                "2.5.4.37"
#define szOID_AUTHORITY_REVOCATION_LIST     "2.5.4.38"
#define szOID_CERTIFICATE_REVOCATION_LIST   "2.5.4.39"
#define szOID_CROSS_CERTIFICATE_PAIR        "2.5.4.40"
#define szOID_GIVEN_NAME                    "2.5.4.42"
#define szOID_INITIALS                      "2.5.4.43"
#define szOID_DN_QUALIFIER                  "2.5.4.46"
#define szOID_AUTHORITY_KEY_IDENTIFIER      "2.5.29.1"
#define szOID_KEY_ATTRIBUTES                "2.5.29.2"
#define szOID_CERT_POLICIES_95              "2.5.29.3"
#define szOID_KEY_USAGE_RESTRICTION         "2.5.29.4"
#define szOID_LEGACY_POLICY_MAPPINGS        "2.5.29.5"
#define szOID_SUBJECT_ALT_NAME              "2.5.29.7"
#define szOID_ISSUER_ALT_NAME               "2.5.29.8"
#define szOID_SUBJECT_DIR_ATTRS             "2.5.29.9"
#define szOID_BASIC_CONSTRAINTS             "2.5.29.10"
#define szOID_SUBJECT_KEY_IDENTIFIER        "2.5.29.14"
#define szOID_KEY_USAGE                     "2.5.29.15"
#define szOID_PRIVATEKEY_USAGE_PERIOD       "2.5.29.16"
#define szOID_SUBJECT_ALT_NAME2             "2.5.29.17"
#define szOID_ISSUER_ALT_NAME2              "2.5.29.18"
#define szOID_BASIC_CONSTRAINTS2            "2.5.29.19"
#define szOID_CRL_NUMBER                    "2.5.29.20"
#define szOID_CRL_REASON_CODE               "2.5.29.21"
#define szOID_REASON_CODE_HOLD              "2.5.29.23"
#define szOID_DELTA_CRL_INDICATOR           "2.5.29.27"
#define szOID_ISSUING_DIST_POINT            "2.5.29.28"
#define szOID_NAME_CONSTRAINTS              "2.5.29.30"
#define szOID_CRL_DIST_POINTS               "2.5.29.31"
#define szOID_CERT_POLICIES                 "2.5.29.32"
#define szOID_ANY_CERT_POLICY               "2.5.29.32.0"
#define szOID_POLICY_MAPPINGS               "2.5.29.33"
#define szOID_AUTHORITY_KEY_IDENTIFIER2     "2.5.29.35"
#define szOID_POLICY_CONSTRAINTS            "2.5.29.36"
#define szOID_ENHANCED_KEY_USAGE            "2.5.29.37"
#define szOID_FRESHEST_CRL                  "2.5.29.46"
#define szOID_INHIBIT_ANY_POLICY            "2.5.29.54"
#define szOID_DOMAIN_COMPONENT              "0.9.2342.19200300.100.1.25"
#define szOID_PKCS_12_FRIENDLY_NAME_ATTR     "1.2.840.113549.1.9.20"
#define szOID_PKCS_12_LOCAL_KEY_ID           "1.2.840.113549.1.9.21"
#define szOID_CERT_EXTENSIONS                "1.3.6.1.4.1.311.2.1.14"
#define szOID_NEXT_UPDATE_LOCATION           "1.3.6.1.4.1.311.10.2"
#define szOID_KP_CTL_USAGE_SIGNING           "1.3.6.1.4.1.311.10.3.1"
#define szOID_KP_TIME_STAMP_SIGNING          "1.3.6.1.4.1.311.10.3.2"
#ifndef szOID_SERVER_GATED_CRYPTO
#define szOID_SERVER_GATED_CRYPTO            "1.3.6.1.4.1.311.10.3.3"
#endif
#ifndef szOID_SGC_NETSCAPE
#define szOID_SGC_NETSCAPE                   "2.16.840.1.113730.4.1"
#endif
#define szOID_KP_EFS                         "1.3.6.1.4.1.311.10.3.4"
#define szOID_EFS_RECOVERY                   "1.3.6.1.4.1.311.10.3.4.1"
#define szOID_WHQL_CRYPTO                    "1.3.6.1.4.1.311.10.3.5"
#define szOID_NT5_CRYPTO                     "1.3.6.1.4.1.311.10.3.6"
#define szOID_OEM_WHQL_CRYPTO                "1.3.6.1.4.1.311.10.3.7"
#define szOID_EMBEDDED_NT_CRYPTO             "1.3.6.1.4.1.311.10.3.8"
#define szOID_ROOT_LIST_SIGNER               "1.3.6.1.4.1.311.10.3.9"
#define szOID_KP_QUALIFIED_SUBORDINATION     "1.3.6.1.4.1.311.10.3.10"
#define szOID_KP_KEY_RECOVERY                "1.3.6.1.4.1.311.10.3.11"
#define szOID_KP_DOCUMENT_SIGNING            "1.3.6.1.4.1.311.10.3.12"
#define szOID_KP_LIFETIME_SIGNING            "1.3.6.1.4.1.311.10.3.13"
#define szOID_KP_MOBILE_DEVICE_SOFTWARE      "1.3.6.1.4.1.311.10.3.14"
#define szOID_YESNO_TRUST_ATTR               "1.3.6.1.4.1.311.10.4.1"
#ifndef szOID_DRM
#define szOID_DRM                            "1.3.6.1.4.1.311.10.5.1"
#endif
#ifndef szOID_DRM_INDIVIDUALIZATION
#define szOID_DRM_INDIVIDUALIZATION          "1.3.6.1.4.1.311.10.5.2"
#endif
#ifndef szOID_LICENSES
#define szOID_LICENSES                       "1.3.6.1.4.1.311.10.6.1"
#endif
#ifndef szOID_LICENSE_SERVER
#define szOID_LICENSE_SERVER                 "1.3.6.1.4.1.311.10.6.2"
#endif
#define szOID_REMOVE_CERTIFICATE             "1.3.6.1.4.1.311.10.8.1"
#define szOID_CROSS_CERT_DIST_POINTS         "1.3.6.1.4.1.311.10.9.1"
#define szOID_CTL                            "1.3.6.1.4.1.311.10.1"
#define szOID_SORTED_CTL                     "1.3.6.1.4.1.311.10.1.1"
#define szOID_ANY_APPLICATION_POLICY         "1.3.6.1.4.1.311.10.12.1"
#define szOID_RENEWAL_CERTIFICATE            "1.3.6.1.4.1.311.13.1"
#define szOID_ENROLLMENT_NAME_VALUE_PAIR     "1.3.6.1.4.1.311.13.2.1"
#define szOID_ENROLLMENT_CSP_PROVIDER        "1.3.6.1.4.1.311.13.2.2"
#define szOID_OS_VERSION                     "1.3.6.1.4.1.311.13.2.3"
#define szOID_PKCS_12_KEY_PROVIDER_NAME_ATTR "1.3.6.1.4.1.311.17.1"
#define szOID_LOCAL_MACHINE_KEYSET           "1.3.6.1.4.1.311.17.2"
#define szOID_AUTO_ENROLL_CTL_USAGE          "1.3.6.1.4.1.311.20.1"
#define szOID_ENROLL_CERTTYPE_EXTENSION      "1.3.6.1.4.1.311.20.2"
#define szOID_ENROLLMENT_AGENT               "1.3.6.1.4.1.311.20.2.1"
#ifndef szOID_KP_SMARTCARD_LOGON
#define szOID_KP_SMARTCARD_LOGON             "1.3.6.1.4.1.311.20.2.2"
#endif
#ifndef szOID_NT_PRINCIPAL_NAME
#define szOID_NT_PRINCIPAL_NAME              "1.3.6.1.4.1.311.20.2.3"
#endif
#define szOID_CERT_MANIFOLD                  "1.3.6.1.4.1.311.20.3"
#ifndef szOID_CERTSRV_CA_VERSION
#define szOID_CERTSRV_CA_VERSION             "1.3.6.1.4.1.311.21.1"
#endif
#define szOID_CERTSRV_PREVIOUS_CERT_HASH     "1.3.6.1.4.1.311.21.2"
#define szOID_CRL_VIRTUAL_BASE               "1.3.6.1.4.1.311.21.3"
#define szOID_CRL_NEXT_PUBLISH               "1.3.6.1.4.1.311.21.4"
#define szOID_KP_CA_EXCHANGE                 "1.3.6.1.4.1.311.21.5"
#define szOID_KP_KEY_RECOVERY_AGENT          "1.3.6.1.4.1.311.21.6"
#define szOID_CERTIFICATE_TEMPLATE           "1.3.6.1.4.1.311.21.7"
#define szOID_ENTERPRISE_OID_ROOT            "1.3.6.1.4.1.311.21.8"
#define szOID_RDN_DUMMY_SIGNER               "1.3.6.1.4.1.311.21.9"
#define szOID_APPLICATION_CERT_POLICIES      "1.3.6.1.4.1.311.21.10"
#define szOID_APPLICATION_POLICY_MAPPINGS    "1.3.6.1.4.1.311.21.11"
#define szOID_APPLICATION_POLICY_CONSTRAINTS "1.3.6.1.4.1.311.21.12"
#define szOID_ARCHIVED_KEY_ATTR              "1.3.6.1.4.1.311.21.13"
#define szOID_CRL_SELF_CDP                   "1.3.6.1.4.1.311.21.14"
#define szOID_REQUIRE_CERT_CHAIN_POLICY      "1.3.6.1.4.1.311.21.15"
#define szOID_ARCHIVED_KEY_CERT_HASH         "1.3.6.1.4.1.311.21.16"
#define szOID_ISSUED_CERT_HASH               "1.3.6.1.4.1.311.21.17"
#define szOID_DS_EMAIL_REPLICATION           "1.3.6.1.4.1.311.21.19"
#define szOID_REQUEST_CLIENT_INFO            "1.3.6.1.4.1.311.21.20"
#define szOID_ENCRYPTED_KEY_HASH             "1.3.6.1.4.1.311.21.21"
#define szOID_CERTSRV_CROSSCA_VERSION        "1.3.6.1.4.1.311.21.22"
#define szOID_KEYID_RDN                      "1.3.6.1.4.1.311.10.7.1"
#define szOID_PKIX                           "1.3.6.1.5.5.7"
#define szOID_PKIX_PE                        "1.3.6.1.5.5.7.1"
#define szOID_AUTHORITY_INFO_ACCESS          "1.3.6.1.5.5.7.1.1"
#define szOID_PKIX_POLICY_QUALIFIER_CPS      "1.3.6.1.5.5.7.2.1"
#define szOID_PKIX_POLICY_QUALIFIER_USERNOTICE "1.3.6.1.5.5.7.2.2"
#define szOID_PKIX_KP                        "1.3.6.1.5.5.7.3"
#define szOID_PKIX_KP_SERVER_AUTH            "1.3.6.1.5.5.7.3.1"
#define szOID_PKIX_KP_CLIENT_AUTH            "1.3.6.1.5.5.7.3.2"
#define szOID_PKIX_KP_CODE_SIGNING           "1.3.6.1.5.5.7.3.3"
#define szOID_PKIX_KP_EMAIL_PROTECTION       "1.3.6.1.5.5.7.3.4"
#define szOID_PKIX_KP_IPSEC_END_SYSTEM       "1.3.6.1.5.5.7.3.5"
#define szOID_PKIX_KP_IPSEC_TUNNEL           "1.3.6.1.5.5.7.3.6"
#define szOID_PKIX_KP_IPSEC_USER             "1.3.6.1.5.5.7.3.7"
#define szOID_PKIX_KP_TIMESTAMP_SIGNING      "1.3.6.1.5.5.7.3.8"
#define szOID_PKIX_NO_SIGNATURE              "1.3.6.1.5.5.7.6.2"
#define szOID_CMC                            "1.3.6.1.5.5.7.7"
#define szOID_CMC_STATUS_INFO                "1.3.6.1.5.5.7.7.1"
#define szOID_CMC_IDENTIFICATION             "1.3.6.1.5.5.7.7.2"
#define szOID_CMC_IDENTITY_PROOF             "1.3.6.1.5.5.7.7.3"
#define szOID_CMC_DATA_RETURN                "1.3.6.1.5.5.7.7.4"
#define szOID_CMC_TRANSACTION_ID             "1.3.6.1.5.5.7.7.5"
#define szOID_CMC_SENDER_NONCE               "1.3.6.1.5.5.7.7.6"
#define szOID_CMC_RECIPIENT_NONCE            "1.3.6.1.5.5.7.7.7"
#define szOID_CMC_ADD_EXTENSIONS             "1.3.6.1.5.5.7.7.8"
#define szOID_CMC_ENCRYPTED_POP              "1.3.6.1.5.5.7.7.9"
#define szOID_CMC_DECRYPTED_POP              "1.3.6.1.5.5.7.7.10"
#define szOID_CMC_LRA_POP_WITNESS            "1.3.6.1.5.5.7.7.11"
#define szOID_CMC_GET_CERT                   "1.3.6.1.5.5.7.7.15"
#define szOID_CMC_GET_CRL                    "1.3.6.1.5.5.7.7.16"
#define szOID_CMC_REVOKE_REQUEST             "1.3.6.1.5.5.7.7.17"
#define szOID_CMC_REG_INFO                   "1.3.6.1.5.5.7.7.18"
#define szOID_CMC_RESPONSE_INFO              "1.3.6.1.5.5.7.7.19"
#define szOID_CMC_QUERY_PENDING              "1.3.6.1.5.5.7.7.21"
#define szOID_CMC_ID_POP_LINK_RANDOM         "1.3.6.1.5.5.7.7.22"
#define szOID_CMC_ID_POP_LINK_WITNESS        "1.3.6.1.5.5.7.7.23"
#define szOID_CT_PKI_DATA                    "1.3.6.1.5.5.7.12.2"
#define szOID_CT_PKI_RESPONSE                "1.3.6.1.5.5.7.12.3"
#define szOID_PKIX_ACC_DESCR                 "1.3.6.1.5.5.7.48"
#define szOID_PKIX_OCSP                      "1.3.6.1.5.5.7.48.1"
#define szOID_PKIX_CA_ISSUERS                "1.3.6.1.5.5.7.48.2"
#define szOID_IPSEC_KP_IKE_INTERMEDIATE      "1.3.6.1.5.5.8.2.2"

#ifndef szOID_SERIALIZED
#define szOID_SERIALIZED                     "1.3.6.1.4.1.311.10.3.3.1"
#endif

#ifndef szOID_PRODUCT_UPDATE
#define szOID_PRODUCT_UPDATE                 "1.3.6.1.4.1.311.31.1"
#endif

#define szOID_NETSCAPE                       "2.16.840.1.113730"
#define szOID_NETSCAPE_CERT_EXTENSION        "2.16.840.1.113730.1"
#define szOID_NETSCAPE_CERT_TYPE             "2.16.840.1.113730.1.1"
#define szOID_NETSCAPE_BASE_URL              "2.16.840.1.113730.1.2"
#define szOID_NETSCAPE_REVOCATION_URL        "2.16.840.1.113730.1.3"
#define szOID_NETSCAPE_CA_REVOCATION_URL     "2.16.840.1.113730.1.4"
#define szOID_NETSCAPE_CERT_RENEWAL_URL      "2.16.840.1.113730.1.7"
#define szOID_NETSCAPE_CA_POLICY_URL         "2.16.840.1.113730.1.8"
#define szOID_NETSCAPE_SSL_SERVER_NAME       "2.16.840.1.113730.1.12"
#define szOID_NETSCAPE_COMMENT               "2.16.840.1.113730.1.13"
#define szOID_NETSCAPE_DATA_TYPE             "2.16.840.1.113730.2"
#define szOID_NETSCAPE_CERT_SEQUENCE         "2.16.840.1.113730.2.5"

/* Bits for szOID_NETSCAPE_CERT_TYPE */
#define NETSCAPE_SSL_CLIENT_AUTH_CERT_TYPE 0x80
#define NETSCAPE_SSL_SERVER_AUTH_CERT_TYPE 0x40
#define NETSCAPE_SMIME_CERT_TYPE           0x20
#define NETSCAPE_SIGN_CERT_TYPE            0x10
#define NETSCAPE_SSL_CA_CERT_TYPE          0x04
#define NETSCAPE_SMIME_CA_CERT_TYPE        0x02
#define NETSCAPE_SIGN_CA_CERT_TYPE         0x01

#define CRYPT_ENCODE_DECODE_NONE             0
#define X509_CERT                            ((LPCSTR)1)
#define X509_CERT_TO_BE_SIGNED               ((LPCSTR)2)
#define X509_CERT_CRL_TO_BE_SIGNED           ((LPCSTR)3)
#define X509_CERT_REQUEST_TO_BE_SIGNED       ((LPCSTR)4)
#define X509_EXTENSIONS                      ((LPCSTR)5)
#define X509_NAME_VALUE                      ((LPCSTR)6)
#define X509_ANY_STRING                      X509_NAME_VALUE
#define X509_NAME                            ((LPCSTR)7)
#define X509_PUBLIC_KEY_INFO                 ((LPCSTR)8)
#define X509_AUTHORITY_KEY_ID                ((LPCSTR)9)
#define X509_KEY_ATTRIBUTES                  ((LPCSTR)10)
#define X509_KEY_USAGE_RESTRICTION           ((LPCSTR)11)
#define X509_ALTERNATE_NAME                  ((LPCSTR)12)
#define X509_BASIC_CONSTRAINTS               ((LPCSTR)13)
#define X509_KEY_USAGE                       ((LPCSTR)14)
#define X509_BASIC_CONSTRAINTS2              ((LPCSTR)15)
#define X509_CERT_POLICIES                   ((LPCSTR)16)
#define PKCS_UTC_TIME                        ((LPCSTR)17)
#define PKCS_TIME_REQUEST                    ((LPCSTR)18)
#define RSA_CSP_PUBLICKEYBLOB                ((LPCSTR)19)
#define X509_UNICODE_NAME                    ((LPCSTR)20)
#define X509_KEYGEN_REQUEST_TO_BE_SIGNED     ((LPCSTR)21)
#define PKCS_ATTRIBUTE                       ((LPCSTR)22)
#define PKCS_CONTENT_INFO_SEQUENCE_OF_ANY    ((LPCSTR)23)
#define X509_UNICODE_NAME_VALUE              ((LPCSTR)24)
#define X509_UNICODE_ANY_STRING              X509_UNICODE_NAME_VALUE
#define X509_OCTET_STRING                    ((LPCSTR)25)
#define X509_BITS                            ((LPCSTR)26)
#define X509_INTEGER                         ((LPCSTR)27)
#define X509_MULTI_BYTE_INTEGER              ((LPCSTR)28)
#define X509_ENUMERATED                      ((LPCSTR)29)
#define X509_CRL_REASON_CODE                 X509_ENUMERATED
#define X509_CHOICE_OF_TIME                  ((LPCSTR)30)
#define X509_AUTHORITY_KEY_ID2               ((LPCSTR)31)
#define X509_AUTHORITY_INFO_ACCESS           ((LPCSTR)32)
#define PKCS_CONTENT_INFO                    ((LPCSTR)33)
#define X509_SEQUENCE_OF_ANY                 ((LPCSTR)34)
#define X509_CRL_DIST_POINTS                 ((LPCSTR)35)
#define X509_ENHANCED_KEY_USAGE              ((LPCSTR)36)
#define PKCS_CTL                             ((LPCSTR)37)
#define X509_MULTI_BYTE_UINT                 ((LPCSTR)38)
#define X509_DSS_PUBLICKEY                   X509_MULTI_BYTE_UINT
#define X509_DSS_PARAMETERS                  ((LPCSTR)39)
#define X509_DSS_SIGNATURE                   ((LPCSTR)40)
#define PKCS_RC2_CBC_PARAMETERS              ((LPCSTR)41)
#define PKCS_SMIME_CAPABILITIES              ((LPCSTR)42)
#define PKCS_RSA_PRIVATE_KEY                 ((LPCSTR)43)
#define PKCS_PRIVATE_KEY_INFO                ((LPCSTR)44)
#define PKCS_ENCRYPTED_PRIVATE_KEY_INFO      ((LPCSTR)45)
#define X509_PKIX_POLICY_QUALIFIER_USERNOTICE ((LPCSTR)46)
#define X509_DH_PUBLICKEY                    X509_MULTI_BYTE_UINT
#define X509_DH_PARAMETERS                   ((LPCSTR)47)
#define PKCS_ATTRIBUTES                      ((LPCSTR)48)
#define PKCS_SORTED_CTL                      ((LPCSTR)49)
#define X942_DH_PARAMETERS                   ((LPCSTR)50)
#define X509_BITS_WITHOUT_TRAILING_ZEROES    ((LPCSTR)51)
#define X942_OTHER_INFO                      ((LPCSTR)52)
#define X509_CERT_PAIR                       ((LPCSTR)53)
#define X509_ISSUING_DIST_POINT              ((LPCSTR)54)
#define X509_NAME_CONSTRAINTS                ((LPCSTR)55)
#define X509_POLICY_MAPPINGS                 ((LPCSTR)56)
#define X509_POLICY_CONSTRAINTS              ((LPCSTR)57)
#define X509_CROSS_CERT_DIST_POINTS          ((LPCSTR)58)
#define CMC_DATA                             ((LPCSTR)59)
#define CMC_RESPONSE                         ((LPCSTR)60)
#define CMC_STATUS                           ((LPCSTR)61)
#define CMC_ADD_EXTENSIONS                   ((LPCSTR)62)
#define CMC_ADD_ATTRIBUTES                   ((LPCSTR)63)
#define X509_CERTIFICATE_TEMPLATE            ((LPCSTR)64)
#define PKCS7_SIGNER_INFO                    ((LPCSTR)500)
#define CMS_SIGNER_INFO                      ((LPCSTR)501)

/* encode/decode flags */
#define CRYPT_ENCODE_NO_SIGNATURE_BYTE_REVERSAL_FLAG           0x00008
#define CRYPT_ENCODE_ALLOC_FLAG                                0x08000
#define CRYPT_SORTED_CTL_ENCODE_HASHED_SUBJECT_IDENTIFIER_FLAG 0x10000
#define CRYPT_UNICODE_NAME_ENCODE_ENABLE_T61_UNICODE_FLAG \
 CERT_RDN_ENABLE_T61_UNICODE_FLAG
#define CRYPT_UNICODE_NAME_ENCODE_ENABLE_UTF8_UNICODE_FLAG \
 CERT_RDN_ENABLE_UTF8_UNICODE_FLAG
#define CRYPT_UNICODE_NAME_ENCODE_DISABLE_CHECK_TYPE_FLAG \
 CERT_RDN_DISABLE_CHECK_TYPE_FLAG

#define CRYPT_DECODE_NOCOPY_FLAG                               0x00001
#define CRYPT_DECODE_TO_BE_SIGNED_FLAG                         0x00002
#define CRYPT_DECODE_SHARE_OID_STRING_FLAG                     0x00004
#define CRYPT_DECODE_NO_SIGNATURE_BYTE_REVERSAL_FLAG           0x00008
#define CRYPT_DECODE_ALLOC_FLAG                                0x08000
#define CRYPT_UNICODE_NAME_DECODE_DISABLE_IE4_UTF8_FLAG \
 CERT_RDN_DISABLE_IE4_UTF8_FLAG

#define CERT_STORE_SIGNATURE_FLAG     0x00000001
#define CERT_STORE_TIME_VALIDITY_FLAG 0x00000002
#define CERT_STORE_REVOCATION_FLAG    0x00000004
#define CERT_STORE_NO_CRL_FLAG        0x00010000
#define CERT_STORE_NO_ISSUER_FLAG     0x00020000

#define CERT_STORE_BASE_CRL_FLAG  0x00000100
#define CERT_STORE_DELTA_CRL_FLAG 0x00000200

/* subject types for CryptVerifyCertificateSignatureEx */
#define CRYPT_VERIFY_CERT_SIGN_SUBJECT_BLOB 1
#define CRYPT_VERIFY_CERT_SIGN_SUBJECT_CERT 2
#define CRYPT_VERIFY_CERT_SIGN_SUBJECT_CRL  3

/* issuer types for CryptVerifyCertificateSignatureEx */
#define CRYPT_VERIFY_CERT_SIGN_ISSUER_PUBKEY 1
#define CRYPT_VERIFY_CERT_SIGN_ISSUER_CERT   2
#define CRYPT_VERIFY_CERT_SIGN_ISSUER_CHAIN  3
#define CRYPT_VERIFY_CERT_SIGN_ISSUER_NULL   4

#define CRYPT_GET_URL_FROM_PROPERTY         0x00000001
#define CRYPT_GET_URL_FROM_EXTENSION        0x00000002
#define CRYPT_GET_URL_FROM_UNAUTH_ATTRIBUTE 0x00000004
#define CRYPT_GET_URL_FROM_AUTH_ATTRIBUTE   0x00000008

/* Certificate name string types and flags */
#define CERT_SIMPLE_NAME_STR 1
#define CERT_OID_NAME_STR    2
#define CERT_X500_NAME_STR   3
#define CERT_NAME_STR_SEMICOLON_FLAG           0x40000000
#define CERT_NAME_STR_NO_PLUS_FLAG             0x20000000
#define CERT_NAME_STR_NO_QUOTING_FLAG          0x10000000
#define CERT_NAME_STR_CRLF_FLAG                0x08000000
#define CERT_NAME_STR_COMMA_FLAG               0x04000000
#define CERT_NAME_STR_REVERSE_FLAG             0x02000000
#define CERT_NAME_STR_ENABLE_UTF8_UNICODE_FLAG 0x00040000
#define CERT_NAME_STR_ENABLE_T61_UNICODE_FLAG  0x00020000
#define CERT_NAME_STR_DISABLE_IE4_UTF8_FLAG    0x00010000

#define CERT_NAME_EMAIL_TYPE            1
#define CERT_NAME_RDN_TYPE              2
#define CERT_NAME_ATTR_TYPE             3
#define CERT_NAME_SIMPLE_DISPLAY_TYPE   4
#define CERT_NAME_FRIENDLY_DISPLAY_TYPE 5
#define CERT_NAME_DNS_TYPE              6
#define CERT_NAME_URL_TYPE              7
#define CERT_NAME_UPN_TYPE              8

#define CERT_NAME_ISSUER_FLAG           0x00000001
#define CERT_NAME_DISABLE_IE4_UTF8_FLAG 0x00010000

/* CryptFormatObject flags */
#define CRYPT_FORMAT_STR_MULTI_LINE 0x0001
#define CRYPT_FORMAT_STR_NO_HEX     0x0010

#define CRYPT_FORMAT_SIMPLE        0x0001
#define CRYPT_FORMAT_X509          0x0002
#define CRYPT_FORMAT_OID           0x0004
#define CRYPT_FORMAT_RDN_SEMICOLON 0x0100
#define CRYPT_FORMAT_RDN_CRLF      0x0200
#define CRYPT_FORMAT_RDN_UNQUOTE   0x0400
#define CRYPT_FORMAT_RDN_REVERSE   0x0800

#define CRYPT_FORMAT_COMMA     0x1000
#define CRYPT_FORMAT_SEMICOLON CRYPT_FORMAT_RDN_SEMICOLON
#define CRYPT_FORMAT_CRLF      CRYPT_FORMAT_RDN_CRLF

/* CryptQueryObject types and flags */
#define CERT_QUERY_OBJECT_FILE 1
#define CERT_QUERY_OBJECT_BLOB 2

#define CERT_QUERY_CONTENT_CERT               1
#define CERT_QUERY_CONTENT_CTL                2
#define CERT_QUERY_CONTENT_CRL                3
#define CERT_QUERY_CONTENT_SERIALIZED_STORE   4
#define CERT_QUERY_CONTENT_SERIALIZED_CERT    5
#define CERT_QUERY_CONTENT_SERIALIZED_CTL     6
#define CERT_QUERY_CONTENT_SERIALIZED_CRL     7
#define CERT_QUERY_CONTENT_PKCS7_SIGNED       8
#define CERT_QUERY_CONTENT_PKCS7_UNSIGNED     9
#define CERT_QUERY_CONTENT_PKCS7_SIGNED_EMBED 10
#define CERT_QUERY_CONTENT_PKCS10             11
#define CERT_QUERY_CONTENT_PFX                12
#define CERT_QUERY_CONTENT_CERT_PAIR          13

#define CERT_QUERY_CONTENT_FLAG_CERT      (1 << CERT_QUERY_CONTENT_CERT)
#define CERT_QUERY_CONTENT_FLAG_CTL       (1 << CERT_QUERY_CONTENT_CTL)
#define CERT_QUERY_CONTENT_FLAG_CRL       (1 << CERT_QUERY_CONTENT_CRL)
#define CERT_QUERY_CONTENT_FLAG_SERIALIZED_STORE \
 (1 << CERT_QUERY_CONTENT_SERIALIZED_STORE)
#define CERT_QUERY_CONTENT_FLAG_SERIALIZED_CERT \
 (1 << CERT_QUERY_CONTENT_SERIALIZED_CERT)
#define CERT_QUERY_CONTENT_FLAG_SERIALIZED_CTL \
 (1 << CERT_QUERY_CONTENT_SERIALIZED_CTL)
#define CERT_QUERY_CONTENT_FLAG_SERIALIZED_CRL \
 (1 << CERT_QUERY_CONTENT_SERIALIZED_CRL)
#define CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED \
 (1 << CERT_QUERY_CONTENT_PKCS7_SIGNED)
#define CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED \
 (1 << CERT_QUERY_CONTENT_PKCS7_UNSIGNED)
#define CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED \
 (1 << CERT_QUERY_CONTENT_PKCS7_SIGNED_EMBED)
#define CERT_QUERY_CONTENT_FLAG_PKCS10    (1 << CERT_QUERY_CONTENT_PKCS10)
#define CERT_QUERY_CONTENT_FLAG_PFX       (1 << CERT_QUERY_CONTENT_PFX)
#define CERT_QUERY_CONTENT_FLAG_CERT_PAIR (1 << CERT_QUERY_CONTENT_CERT_PAIR)

#define CERT_QUERY_CONTENT_FLAG_ALL \
 CERT_QUERY_CONTENT_FLAG_CERT | \
 CERT_QUERY_CONTENT_FLAG_CTL | \
 CERT_QUERY_CONTENT_FLAG_CRL | \
 CERT_QUERY_CONTENT_FLAG_SERIALIZED_STORE | \
 CERT_QUERY_CONTENT_FLAG_SERIALIZED_CERT | \
 CERT_QUERY_CONTENT_FLAG_SERIALIZED_CTL | \
 CERT_QUERY_CONTENT_FLAG_SERIALIZED_CRL | \
 CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED | \
 CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED | \
 CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED | \
 CERT_QUERY_CONTENT_FLAG_PKCS10 | \
 CERT_QUERY_CONTENT_FLAG_PFX | \
 CERT_QUERY_CONTENT_FLAG_CERT_PAIR

#define CERT_QUERY_FORMAT_BINARY                1
#define CERT_QUERY_FORMAT_BASE64_ENCODED        2
#define CERT_QUERY_FORMAT_ASN_ASCII_HEX_ENCODED 3

#define CERT_QUERY_FORMAT_FLAG_BINARY (1 << CERT_QUERY_FORMAT_BINARY)
#define CERT_QUERY_FORMAT_FLAG_BASE64_ENCODED \
 (1 << CERT_QUERY_FORMAT_BASE64_ENCODED)
#define CERT_QUERY_FORMAT_FLAG_ASN_ASCII_HEX_ENCODED \
 (1 << CERT_QUERY_FORMAT_ASN_ASCII_HEX_ENCODED)

#define CERT_QUERY_FORMAT_FLAG_ALL \
 CERT_QUERY_FORMAT_FLAG_BINARY | \
 CERT_QUERY_FORMAT_FLAG_BASE64_ENCODED | \
 CERT_QUERY_FORMAT_FLAG_ASN_ASCII_HEX_ENCODED \

#define CERT_SET_KEY_PROV_HANDLE_PROP_ID 0x00000001
#define CERT_SET_KEY_CONTEXT_PROP_ID     0x00000001

#define CERT_CREATE_SELFSIGN_NO_SIGN     1
#define CERT_CREATE_SELFSIGN_NO_KEY_INFO 2

/* flags for CryptAcquireCertificatePrivateKey */
#define CRYPT_ACQUIRE_CACHE_FLAG         0x00000001
#define CRYPT_ACQUIRE_USE_PROV_INFO_FLAG 0x00000002
#define CRYPT_ACQUIRE_COMPARE_KEY_FLAG   0x00000004
#define CRYPT_ACQUIRE_SILENT_FLAG        0x00000040

/* flags for CryptFindCertificateKeyProvInfo */
#define CRYPT_FIND_USER_KEYSET_FLAG    0x00000001
#define CRYPT_FIND_MACHINE_KEYSET_FLAG 0x00000002
#define CRYPT_FIND_SILENT_KEYSET_FLAG  0x00000040

/* Chain engines and chains */
typedef HANDLE HCERTCHAINENGINE;
#define HCCE_CURRENT_USER  ((HCERTCHAINENGINE)NULL)
#define HCCE_LOCAL_MACHINE ((HCERTCHAINENGINE)1)

#define CERT_CHAIN_CACHE_END_CERT           0x00000001
#define CERT_CHAIN_THREAD_STORE_SYNC        0x00000002
#define CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL 0x00000004
#define CERT_CHAIN_USE_LOCAL_MACHINE_STORE  0x00000008
#define CERT_CHAIN_ENABLE_CACHE_AUTO_UPDATE 0x00000010
#define CERT_CHAIN_ENABLE_SHARE_STORE       0x00000020

typedef struct _CERT_CHAIN_ENGINE_CONFIG
{
    DWORD       cbSize;
    HCERTSTORE  hRestrictedRoot;
    HCERTSTORE  hRestrictedTrust;
    HCERTSTORE  hRestrictedOther;
    DWORD       cAdditionalStore;
    HCERTSTORE *rghAdditionalStore;
    DWORD       dwFlags;
    DWORD       dwUrlRetrievalTimeout;
    DWORD       MaximumCachedCertificates;
    DWORD       CycleDetectionModulus;
    HCERTSTORE  hExclusiveRoot;
    HCERTSTORE  hExclusiveRootTrustedPeople;
} CERT_CHAIN_ENGINE_CONFIG, *PCERT_CHAIN_ENGINE_CONFIG;

/* message-related definitions */

typedef BOOL
(WINAPI *PFN_CMSG_STREAM_OUTPUT)(
  _In_opt_ const void *pvArg,
  _In_reads_bytes_opt_(cbData) BYTE *pbData,
  _In_ DWORD cbData,
  _In_ BOOL fFinal);

#define CMSG_INDEFINITE_LENGTH 0xffffffff

typedef struct _CMSG_STREAM_INFO
{
    DWORD cbContent;
    PFN_CMSG_STREAM_OUTPUT pfnStreamOutput;
    void *pvArg;
} CMSG_STREAM_INFO, *PCMSG_STREAM_INFO;

typedef struct _CERT_ISSUER_SERIAL_NUMBER
{
    CERT_NAME_BLOB     Issuer;
    CRYPT_INTEGER_BLOB SerialNumber;
} CERT_ISSUER_SERIAL_NUMBER, *PCERT_ISSUER_SERIAL_NUMBER;

typedef struct _CERT_ID
{
    DWORD dwIdChoice;
    union {
        CERT_ISSUER_SERIAL_NUMBER IssuerSerialNumber;
        CRYPT_HASH_BLOB           KeyId;
        CRYPT_HASH_BLOB           HashId;
    } DUMMYUNIONNAME;
} CERT_ID, *PCERT_ID;

#define CERT_ID_ISSUER_SERIAL_NUMBER 1
#define CERT_ID_KEY_IDENTIFIER       2
#define CERT_ID_SHA1_HASH            3

#ifndef USE_WC_PREFIX
#undef CMSG_DATA /* may be defined by sys/socket.h */
#define CMSG_DATA                 1
#define CMSG_SIGNED               2
#define CMSG_ENVELOPED            3
#define CMSG_SIGNED_AND_ENVELOPED 4
#define CMSG_HASHED               5
#define CMSG_ENCRYPTED            6

#define CMSG_ALL_FLAGS                 ~0U
#define CMSG_DATA_FLAG                 (1 << CMSG_DATA)
#define CMSG_SIGNED_FLAG               (1 << CMSG_SIGNED)
#define CMSG_ENVELOPED_FLAG            (1 << CMSG_ENVELOPED)
#define CMSG_SIGNED_AND_ENVELOPED_FLAG (1 << CMSG_SIGNED_AND_ENVELOPED)
#define CMSG_ENCRYPTED_FLAG            (1 << CMSG_ENCRYPTED)
#else
#define WC_CMSG_DATA                 1
#define WC_CMSG_SIGNED               2
#define WC_CMSG_ENVELOPED            3
#define WC_CMSG_SIGNED_AND_ENVELOPED 4
#define WC_CMSG_HASHED               5
#define WC_CMSG_ENCRYPTED            6

#define WC_CMSG_ALL_FLAGS                 ~0U
#define WC_CMSG_DATA_FLAG                 (1 << WC_CMSG_DATA)
#define WC_CMSG_SIGNED_FLAG               (1 << WC_CMSG_SIGNED)
#define WC_CMSG_ENVELOPED_FLAG            (1 << WC_CMSG_ENVELOPED)
#define WC_CMSG_SIGNED_AND_ENVELOPED_FLAG (1 << WC_CMSG_SIGNED_AND_ENVELOPED)
#define WC_CMSG_ENCRYPTED_FLAG            (1 << WC_CMSG_ENCRYPTED)
#endif

typedef struct _CMSG_SIGNER_ENCODE_INFO
{
    DWORD                      cbSize;
    PCERT_INFO                 pCertInfo;
    HCRYPTPROV                 hCryptProv;
    DWORD                      dwKeySpec;
    CRYPT_ALGORITHM_IDENTIFIER HashAlgorithm;
    void                      *pvHashAuxInfo;
    DWORD                      cAuthAttr;
    PCRYPT_ATTRIBUTE           rgAuthAttr;
    DWORD                      cUnauthAttr;
    PCRYPT_ATTRIBUTE           rgUnauthAttr;
#ifdef CMSG_SIGNER_ENCODE_INFO_HAS_CMS_FIELDS
    CERT_ID                    SignerId;
    CRYPT_ALGORITHM_IDENTIFIER HashEncryptionAlgorithm;
    void                      *pvHashEncryptionAuxInfo;
#endif
} CMSG_SIGNER_ENCODE_INFO, *PCMSG_SIGNER_ENCODE_INFO;

typedef struct _CMSG_SIGNED_ENCODE_INFO
{
    DWORD                    cbSize;
    DWORD                    cSigners;
    PCMSG_SIGNER_ENCODE_INFO rgSigners;
    DWORD                    cCertEncoded;
    PCERT_BLOB               rgCertEncoded;
    DWORD                    cCrlEncoded;
    PCRL_BLOB                rgCrlEncoded;
#ifdef CMSG_SIGNED_ENCODE_INFO_HAS_CMS_FIELDS
    DWORD                    cAttrCertEncoded;
    PCERT_BLOB               rgAttrCertEncoded;
#endif
} CMSG_SIGNED_ENCODE_INFO, *PCMSG_SIGNED_ENCODE_INFO;

typedef struct _CMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO
{
    DWORD                      cbSize;
    CRYPT_ALGORITHM_IDENTIFIER KeyEncryptionAlgorithm;
    void                      *pvKeyEncryptionAuxInfo;
    HCRYPTPROV_LEGACY          hCryptProv;
    CRYPT_BIT_BLOB             RecipientPublicKey;
    CERT_ID                    RecipientId;
} CMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO, *PCMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO;

typedef struct _CMSG_RECIPIENT_ENCRYPTED_KEY_ENCODE_INFO
{
    DWORD                       cbSize;
    CRYPT_BIT_BLOB              RecipientPublicKey;
    CERT_ID                     RecipientId;
    FILETIME                    Date;
    PCRYPT_ATTRIBUTE_TYPE_VALUE pOtherAttr;
} CMSG_RECIPIENT_ENCRYPTED_KEY_ENCODE_INFO,
 *PCMSG_RECIPIENT_ENCRYPTED_KEY_ENCODE_INFO;

typedef struct _CMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO
{
    DWORD                      cbSize;
    CRYPT_ALGORITHM_IDENTIFIER KeyEncryptionAlgorithm;
    void                      *pvKeyEncryptionAuxInfo;
    CRYPT_ALGORITHM_IDENTIFIER KeyWrapAlgorithm;
    void                      *pvKeyWrapAuxInfo;
    HCRYPTPROV_LEGACY          hCryptProv;
    DWORD                      dwKeySpec;
    DWORD                      dwKeyChoice;
    union {
        PCRYPT_ALGORITHM_IDENTIFIER pEphemeralAlgorithm;
        PCERT_ID                    pSenderId;
    } DUMMYUNIONNAME;
    CRYPT_DATA_BLOB            UserKeyingMaterial;
    DWORD                      cRecipientEncryptedKeys;
    PCMSG_RECIPIENT_ENCRYPTED_KEY_ENCODE_INFO *rgpRecipientEncryptedKeys;
} CMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO, *PCMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO;

#define CMSG_KEY_AGREE_EPHEMERAL_KEY_CHOICE 1
#define CMSG_KEY_AGREE_STATIC_KEY_CHOICE    2

typedef struct _CMSG_MAIL_LIST_RECIPIENT_ENCODE_INFO
{
    DWORD                       cbSize;
    CRYPT_ALGORITHM_IDENTIFIER  KeyEncryptionAlgorithm;
    void                       *pvKeyEncryptionAuxInfo;
    HCRYPTPROV                  hCryptProv;
    DWORD                       dwKeyChoice;
    union {
        HCRYPTKEY hKeyEncryptionKey;
        void     *pvKeyEncryptionKey;
    } DUMMYUNIONNAME;
    CRYPT_DATA_BLOB             KeyId;
    FILETIME                    Date;
    PCRYPT_ATTRIBUTE_TYPE_VALUE pOtherAttr;
} CMSG_MAIL_LIST_RECIPIENT_ENCODE_INFO, *PCMSG_MAIL_LIST_RECIPIENT_ENCODE_INFO;

#define CMSG_MAIL_LIST_HANDLE_KEY_CHOICE 1

typedef struct _CMSG_RECIPIENT_ENCODE_INFO
{
    DWORD dwRecipientChoice;
    union {
        PCMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO pKeyTrans;
        PCMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO pKeyAgree;
        PCMSG_MAIL_LIST_RECIPIENT_ENCODE_INFO pMailList;
    } DUMMYUNIONNAME;
} CMSG_RECIPIENT_ENCODE_INFO, *PCMSG_RECIPIENT_ENCODE_INFO;

#define CMSG_KEY_TRANS_RECIPIENT 1
#define CMSG_KEY_AGREE_RECIPIENT 2
#define CMSG_MAIL_LIST_RECIPIENT 3

typedef struct _CMSG_ENVELOPED_ENCODE_INFO
{
    DWORD                       cbSize;
    HCRYPTPROV_LEGACY           hCryptProv;
    CRYPT_ALGORITHM_IDENTIFIER  ContentEncryptionAlgorithm;
    void                       *pvEncryptionAuxInfo;
    DWORD                       cRecipients;
    PCERT_INFO                 *rgpRecipientCert;
#ifdef CMSG_ENVELOPED_ENCODE_INFO_HAS_CMS_FIELDS
    PCMSG_RECIPIENT_ENCODE_INFO rgCmsRecipients;
    DWORD                       cCertEncoded;
    PCERT_BLOB                  rgCertEncoded;
    DWORD                       cCrlEncoded;
    PCRL_BLOB                   rgCrlEncoded;
    DWORD                       cAttrCertEncoded;
    PCERT_BLOB                  rgAttrCertEncoded;
    DWORD                       cUnprotectedAttr;
    PCRYPT_ATTRIBUTE            rgUnprotectedAttr;
#endif
} CMSG_ENVELOPED_ENCODE_INFO, *PCMSG_ENVELOPED_ENCODE_INFO;

typedef struct _CMSG_SIGNED_AND_ENVELOPED_ENCODE_INFO
{
    DWORD                      cbSize;
    CMSG_SIGNED_ENCODE_INFO    SignedInfo;
    CMSG_ENVELOPED_ENCODE_INFO EnvelopedInfo;
} CMSG_SIGNED_AND_ENVELOPED_ENCODE_INFO,
 *PCMSG_SIGNED_AND_ENVELOPED_ENCODE_INFO;

typedef struct _CMSG_HASHED_ENCODE_INFO
{
    DWORD                      cbSize;
    HCRYPTPROV_LEGACY          hCryptProv;
    CRYPT_ALGORITHM_IDENTIFIER HashAlgorithm;
    void                      *pvHashAuxInfo;
} CMSG_HASHED_ENCODE_INFO, *PCMSG_HASHED_ENCODE_INFO;

typedef struct _CMSG_ENCRYPTED_ENCODE_INFO
{
    DWORD                      cbSize;
    CRYPT_ALGORITHM_IDENTIFIER ContentEncryptionAlgorithm;
    void                      *pvEncryptionAuxInfo;
} CMSG_ENCRYPTED_ENCODE_INFO, *PCMSG_ENCRYPTED_ENCODE_INFO;

#define CMSG_BARE_CONTENT_FLAG             0x00000001
#define CMSG_LENGTH_ONLY_FLAG              0x00000002
#define CMSG_DETACHED_FLAG                 0x00000004
#define CMSG_AUTHENTICATED_ATTRIBUTES_FLAG 0x00000008
#define CMSG_CONTENTS_OCTETS_FLAG          0x00000010
#define CMSG_MAX_LENGTH_FLAG               0x00000020
#define CMSG_CMS_ENCAPSULATED_CONTENT_FLAG 0x00000040
#define CMSG_CRYPT_RELEASE_CONTEXT_FLAG    0x00008000

#define CMSG_CTRL_VERIFY_SIGNATURE       1
#define CMSG_CTRL_DECRYPT                2
#define CMSG_CTRL_VERIFY_HASH            5
#define CMSG_CTRL_ADD_SIGNER             6
#define CMSG_CTRL_DEL_SIGNER             7
#define CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR 8
#define CMSG_CTRL_DEL_SIGNER_UNAUTH_ATTR 9
#define CMSG_CTRL_ADD_CERT               10
#define CMSG_CTRL_DEL_CERT               11
#define CMSG_CTRL_ADD_CRL                12
#define CMSG_CTRL_DEL_CRL                13
#define CMSG_CTRL_ADD_ATTR_CERT          14
#define CMSG_CTRL_DEL_ATTR_CERT          15
#define CMSG_CTRL_KEY_TRANS_DECRYPT      16
#define CMSG_CTRL_KEY_AGREE_DECRYPT      17
#define CMSG_CTRL_MAIL_LIST_DECRYPT      18
#define CMSG_CTRL_VERIFY_SIGNATURE_EX    19
#define CMSG_CTRL_ADD_CMS_SIGNER_INFO    20

typedef struct _CMSG_CTRL_DECRYPT_PARA
{
    DWORD      cbSize;
    HCRYPTPROV hCryptProv;
    DWORD      dwKeySpec;
    DWORD      dwRecipientIndex;
} CMSG_CTRL_DECRYPT_PARA, *PCMSG_CTRL_DECRYPT_PARA;

typedef struct _CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR_PARA
{
    DWORD           cbSize;
    DWORD           dwSignerIndex;
    CRYPT_DATA_BLOB blob;
} CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR_PARA,
 *PCMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR_PARA;

typedef struct _CMSG_CTRL_DEL_SIGNER_UNAUTH_ATTR_PARA
{
    DWORD           cbSize;
    DWORD           dwSignerIndex;
    DWORD           dwUnauthAttrIndex;
} CMSG_CTRL_DEL_SIGNER_UNAUTH_ATTR_PARA,
 *PCMSG_CTRL_DEL_SIGNER_UNAUTH_ATTR_PARA;

typedef struct _CMSG_CTRL_VERIFY_SIGNATURE_EX_PARA {
    DWORD      cbSize;
    HCRYPTPROV hCryptProv;
    DWORD      dwSignerIndex;
    DWORD      dwSignerType;
    void      *pvSigner;
} CMSG_CTRL_VERIFY_SIGNATURE_EX_PARA, *PCMSG_CTRL_VERIFY_SIGNATURE_EX_PARA;

#define CMSG_VERIFY_SIGNER_PUBKEY 1
#define CMSG_VERIFY_SIGNER_CERT   2
#define CMSG_VERIFY_SIGNER_CHAIN  3
#define CMSG_VERIFY_SIGNER_NULL   4

#define CMSG_TYPE_PARAM                  1
#define CMSG_CONTENT_PARAM               2
#define CMSG_BARE_CONTENT_PARAM          3
#define CMSG_INNER_CONTENT_TYPE_PARAM    4
#define CMSG_SIGNER_COUNT_PARAM          5
#define CMSG_SIGNER_INFO_PARAM           6
#define CMSG_SIGNER_CERT_INFO_PARAM      7
#define CMSG_SIGNER_HASH_ALGORITHM_PARAM 8
#define CMSG_SIGNER_AUTH_ATTR_PARAM      9
#define CMSG_SIGNER_UNAUTH_ATTR_PARAM    10
#define CMSG_CERT_COUNT_PARAM            11
#define CMSG_CERT_PARAM                  12
#define CMSG_CRL_COUNT_PARAM             13
#define CMSG_CRL_PARAM                   14
#define CMSG_ENVELOPE_ALGORITHM_PARAM    15
#define CMSG_RECIPIENT_COUNT_PARAM       17
#define CMSG_RECIPIENT_INDEX_PARAM       18
#define CMSG_RECIPIENT_INFO_PARAM        19
#define CMSG_HASH_ALGORITHM_PARAM        20
#define CMSG_HASH_DATA_PARAM             21
#define CMSG_COMPUTED_HASH_PARAM         22
#define CMSG_ENCRYPT_PARAM               26
#define CMSG_ENCRYPTED_DIGEST            27
#define CMSG_ENCODED_SIGNER              28
#define CMSG_ENCODED_MESSAGE             29
#define CMSG_VERSION_PARAM               30
#define CMSG_ATTR_CERT_COUNT_PARAM       31
#define CMSG_ATTR_CERT_PARAM             32
#define CMSG_CMS_RECIPIENT_COUNT_PARAM   33
#define CMSG_CMS_RECIPIENT_INDEX_PARAM   34
#define CMSG_CMS_RECIPIENT_ENCRYPTED_KEY_INDEX_PARAM 35
#define CMSG_CMS_RECIPIENT_INFO_PARAM    36
#define CMSG_UNPROTECTED_ATTR_PARAM      37
#define CMSG_SIGNER_CERT_ID_PARAM        38
#define CMSG_CMS_SIGNER_INFO_PARAM       39

typedef struct _CMSG_CMS_SIGNER_INFO {
    DWORD                      dwVersion;
    CERT_ID                    SignerId;
    CRYPT_ALGORITHM_IDENTIFIER HashAlgorithm;
    CRYPT_ALGORITHM_IDENTIFIER HashEncryptionAlgorithm;
    CRYPT_DATA_BLOB            EncryptedHash;
    CRYPT_ATTRIBUTES           AuthAttrs;
    CRYPT_ATTRIBUTES           UnauthAttrs;
} CMSG_CMS_SIGNER_INFO, *PCMSG_CMS_SIGNER_INFO;

typedef CRYPT_ATTRIBUTES CMSG_ATTR, *PCMSG_ATTR;

#define CMSG_SIGNED_DATA_V1               1
#define CMSG_SIGNED_DATA_V3               3
#define CMSG_SIGNED_DATA_PKCS_1_5_VERSION CMSG_SIGNED_DATA_V1
#define CMSG_SIGNED_DATA_CMS_VERSION      CMSG_SIGNED_DATA_V3

#define CMSG_SIGNER_INFO_V1               1
#define CMSG_SIGNER_INFO_V3               3
#define CMSG_SIGNER_INFO_PKCS_1_5_VERSION CMSG_SIGNER_INFO_V1
#define CMSG_SIGNER_INFO_CMS_VERSION      CMSG_SIGNER_INFO_V3

#define CMSG_HASHED_DATA_V0               0
#define CMSG_HASHED_DATA_V2               2
#define CMSG_HASHED_DATA_PKCS_1_5_VERSION CMSG_HASHED_DATA_V0
#define CMSG_HASHED_DATA_CMS_VERSION      CMSG_HASHED_DATA_V2

#define CMSG_ENVELOPED_DATA_V0               0
#define CMSG_ENVELOPED_DATA_V2               2
#define CMSG_ENVELOPED_DATA_PKCS_1_5_VERSION CMSG_ENVELOPED_DATA_V0
#define CMSG_ENVELOPED_DATA_CMS_VERSION      CMSG_ENVELOPED_DATA_V2

typedef struct _CMSG_KEY_TRANS_RECIPIENT_INFO {
    DWORD                      dwVersion;
    CERT_ID                    RecipientId;
    CRYPT_ALGORITHM_IDENTIFIER KeyEncryptionAlgorithm;
    CRYPT_DATA_BLOB            EncryptedKey;
} CMSG_KEY_TRANS_RECIPIENT_INFO, *PCMSG_KEY_TRANS_RECIPIENT_INFO;

typedef struct _CMSG_RECIPIENT_ENCRYPTED_KEY_INFO {
    CERT_ID                     RecipientId;
    CRYPT_DATA_BLOB             EncryptedKey;
    PCRYPT_ATTRIBUTE_TYPE_VALUE pOtherAttr;
} CMSG_RECIPIENT_ENCRYPTED_KEY_INFO, *PCMSG_RECIPIENT_ENCRYPTED_KEY_INFO;

typedef struct _CMSG_KEY_AGREE_RECIPIENT_INFO {
    DWORD                               dwVersion;
    DWORD                               dwOriginatorChoice;
    union {
        CERT_ID              OriginatorCertId;
        CERT_PUBLIC_KEY_INFO OriginatorPublicKeyInfo;
    } DUMMYUNIONNAME;
    CRYPT_ALGORITHM_IDENTIFIER          UserKeyingMaterial;
    DWORD                               cRecipientEncryptedKeys;
    PCMSG_RECIPIENT_ENCRYPTED_KEY_INFO *rgpRecipientEncryptedKeys;
} CMSG_KEY_AGREE_RECIPIENT_INFO, *PCMSG_KEY_AGREE_RECIPIENT_INFO;

#define CMSG_KEY_AGREE_ORIGINATOR_CERT       1
#define CMSG_KEY_AGREE_ORIGINATOR_PUBLIC_KEY 2

typedef struct _CMSG_MAIL_LIST_RECIPIENT_INFO {
    DWORD                       dwVersion;
    CRYPT_DATA_BLOB             KeyId;
    CRYPT_ALGORITHM_IDENTIFIER  KeyEncryptionAlgorithm;
    CRYPT_DATA_BLOB             EncryptedKey;
    FILETIME                    Date;
    PCRYPT_ATTRIBUTE_TYPE_VALUE pOtherAttr;
} CMSG_MAIL_LIST_RECIPIENT_INFO, *PCMSG_MAIL_LIST_RECIPIENT_INFO;

typedef struct _CMSG_CMS_RECIPIENT_INFO {
    DWORD dwRecipientChoice;
    union {
        PCMSG_KEY_TRANS_RECIPIENT_INFO pKeyTrans;
        PCMSG_KEY_AGREE_RECIPIENT_INFO pKeyAgree;
        PCMSG_MAIL_LIST_RECIPIENT_INFO pMailList;
    } DUMMYUNIONNAME;
} CMSG_CMS_RECIPIENT_INFO, *PCMSG_CMS_RECIPIENT_INFO;

#define CMSG_ENVELOPED_RECIPIENT_V0     0
#define CMSG_ENVELOPED_RECIPIENT_V2     2
#define CMSG_ENVELOPED_RECIPIENT_V3     3
#define CMSG_ENVELOPED_RECIPIENT_V4     4
#define CMSG_KEY_TRANS_PKCS_1_5_VERSION CMSG_ENVELOPED_RECIPIENT_V0
#define CMSG_KEY_TRANS_CMS_VERSION      CMSG_ENVELOPED_RECIPIENT_V2
#define CMSG_KEY_AGREE_VERSION          CMSG_ENVELOPED_RECIPIENT_V3
#define CMSG_MAIL_LIST_VERSION          CMSG_ENVELOPED_RECIPIENT_V4

typedef void * (WINAPI *PFN_CMSG_ALLOC)(_In_ size_t cb);
typedef void (WINAPI *PFN_CMSG_FREE)(_Inout_ void *pv);

typedef struct _CMSG_CONTENT_ENCRYPT_INFO {
    DWORD                       cbSize;
    HCRYPTPROV                  hCryptProv;
    CRYPT_ALGORITHM_IDENTIFIER  ContentEncryptionAlgorithm;
    void                       *pvEncryptionAuxInfo;
    DWORD                       cRecipients;
    PCMSG_RECIPIENT_ENCODE_INFO rgCmsRecipients;
    PFN_CMSG_ALLOC              pfnAlloc;
    PFN_CMSG_FREE               pfnFree;
    DWORD                       dwEncryptFlags;
    HCRYPTKEY                   hContentEncryptKey;
    DWORD                       dwFlags;
} CMSG_CONTENT_ENCRYPT_INFO, *PCMSG_CONTENT_ENCRYPT_INFO;

typedef struct _CMSG_KEY_TRANS_ENCRYPT_INFO {
    DWORD                       cbSize;
    DWORD                       dwRecipientIndex;
    CRYPT_ALGORITHM_IDENTIFIER  KeyEncryptionAlgorithm;
    CRYPT_DATA_BLOB             EncryptedKey;
    DWORD                       dwFlags;
} CMSG_KEY_TRANS_ENCRYPT_INFO, *PCMSG_KEY_TRANS_ENCRYPT_INFO;

typedef struct _CMSG_CTRL_KEY_TRANS_DECRYPT_PARA {
    DWORD                          cbSize;
    HCRYPTPROV                     hCryptProv;
    DWORD                          dwKeySpec;
    PCMSG_KEY_TRANS_RECIPIENT_INFO pKeyTrans;
    DWORD                          dwRecipientIndex;
} CMSG_CTRL_KEY_TRANS_DECRYPT_PARA, *PCMSG_CTRL_KEY_TRANS_DECRYPT_PARA;

typedef BOOL
(WINAPI *PFN_CMSG_GEN_CONTENT_ENCRYPT_KEY)(
  _Inout_ PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
  _In_ DWORD dwFlags,
  _Reserved_ void *pvReserved);

typedef BOOL
(WINAPI *PFN_CMSG_EXPORT_KEY_TRANS)(
  _In_ PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
  _In_ PCMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO pKeyTransEncodeInfo,
  _Inout_ PCMSG_KEY_TRANS_ENCRYPT_INFO pKeyTransEncryptInfo,
  _In_ DWORD dwFlags,
  _Reserved_ void *pvReserved);

typedef BOOL
(WINAPI *PFN_CMSG_IMPORT_KEY_TRANS)(
  _In_ PCRYPT_ALGORITHM_IDENTIFIER pContentEncryptionAlgorithm,
  _In_ PCMSG_CTRL_KEY_TRANS_DECRYPT_PARA pKeyTransDecryptPara,
  _In_ DWORD dwFlags,
  _Reserved_ void *pvReserved,
  _Out_ HCRYPTKEY *phContentEncryptKey);

/* CryptMsgGetAndVerifySigner flags */
#define CMSG_TRUSTED_SIGNER_FLAG   0x1
#define CMSG_SIGNER_ONLY_FLAG      0x2
#define CMSG_USE_SIGNER_INDEX_FLAG 0x4

/* CryptMsgSignCTL flags */
#define CMSG_CMS_ENCAPSULATED_CTL_FLAG 0x00008000

/* CryptMsgEncodeAndSignCTL flags */
#define CMSG_ENCODED_SORTED_CTL_FLAG               0x1
#define CMSG_ENCODE_HASHED_SUBJECT_IDENTIFIER_FLAG 0x2

/* PFXImportCertStore flags */
#define CRYPT_USER_KEYSET           0x00001000
#define PKCS12_IMPORT_RESERVED_MASK 0xffff0000
/* PFXExportCertStore flags */
#define REPORT_NO_PRIVATE_KEY                 0x00000001
#define REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY 0x00000002
#define EXPORT_PRIVATE_KEYS                   0x00000004
#define PKCS12_EXPORT_RESERVED_MASK           0xffff0000

#define CRYPT_USERDATA 0x00000001

/* function declarations */
/* advapi32.dll */
WINADVAPI
BOOL
WINAPI
CryptAcquireContextA(
  _Out_ HCRYPTPROV *,
  _In_opt_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ DWORD,
  _In_ DWORD);

WINADVAPI
BOOL
WINAPI
CryptAcquireContextW(
  _Out_ HCRYPTPROV *,
  _In_opt_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ DWORD,
  _In_ DWORD);

#define CryptAcquireContext WINELIB_NAME_AW(CryptAcquireContext)

WINADVAPI
BOOL
WINAPI
CryptGenRandom(
  _In_ HCRYPTPROV hProv,
  _In_ DWORD dwLen,
  _Inout_updates_bytes_(dwLen) BYTE *pbBuffer);

WINADVAPI
BOOL
WINAPI
CryptContextAddRef(
  _In_ HCRYPTPROV,
  _Reserved_ DWORD *,
  _In_ DWORD);

WINADVAPI
BOOL
WINAPI
CryptCreateHash(
  _In_ HCRYPTPROV,
  _In_ ALG_ID,
  _In_ HCRYPTKEY,
  _In_ DWORD,
  _Out_ HCRYPTHASH *);

_Success_(return != 0)
WINADVAPI
BOOL
WINAPI
CryptDecrypt(
  _In_ HCRYPTKEY hKey,
  _In_ HCRYPTHASH hHash,
  _In_ BOOL Final,
  _In_ DWORD dwFlags,
  _Inout_updates_bytes_to_(*pdwDataLen, *pdwDataLen) BYTE *pbData,
  _Inout_ DWORD *pdwDataLen);

WINADVAPI
BOOL
WINAPI
CryptDeriveKey(
  _In_ HCRYPTPROV,
  _In_ ALG_ID,
  _In_ HCRYPTHASH,
  _In_ DWORD,
  _Out_ HCRYPTKEY *);

WINADVAPI BOOL WINAPI CryptDestroyHash(_In_ HCRYPTHASH);
WINADVAPI BOOL WINAPI CryptDestroyKey(_In_ HCRYPTKEY);

WINADVAPI
BOOL
WINAPI
CryptDuplicateKey(
  _In_ HCRYPTKEY,
  _Reserved_ DWORD *,
  _In_ DWORD,
  _Out_ HCRYPTKEY *);

WINADVAPI
BOOL
WINAPI
CryptDuplicateHash(
  _In_ HCRYPTHASH,
  _Reserved_ DWORD *,
  _In_ DWORD,
  _Out_ HCRYPTHASH *);

_Success_(return != 0)
WINADVAPI
BOOL
WINAPI
CryptEncrypt(
  _In_ HCRYPTKEY hKey,
  _In_ HCRYPTHASH hHash,
  _In_ BOOL Final,
  _In_ DWORD dwFlags,
  _Inout_updates_bytes_to_opt_(dwBufLen, *pdwDataLen) BYTE *pbData,
  _Inout_ DWORD *pdwDataLen,
  _In_ DWORD dwBufLen);

_Success_(return != 0)
WINADVAPI
BOOL
WINAPI
CryptEnumProvidersA(
  _In_ DWORD dwIndex,
  _Reserved_ DWORD *pdwReserved,
  _In_ DWORD dwFlags,
  _Out_ DWORD *pdwProvType,
  _Out_writes_bytes_to_opt_(*pcbProvName, *pcbProvName) LPSTR szProvName,
  _Inout_ DWORD *pcbProvName);

_Success_(return != 0)
WINADVAPI
BOOL
WINAPI
CryptEnumProvidersW(
  _In_ DWORD dwIndex,
  _Reserved_ DWORD *pdwReserved,
  _In_ DWORD dwFlags,
  _Out_ DWORD *pdwProvType,
  _Out_writes_bytes_to_opt_(*pcbProvName, *pcbProvName) LPWSTR szProvName,
  _Inout_ DWORD *pcbProvName);

#define CryptEnumProviders WINELIB_NAME_AW(CryptEnumProviders)

_Success_(return != 0)
WINADVAPI
BOOL
WINAPI
CryptEnumProviderTypesA(
  _In_ DWORD dwIndex,
  _Reserved_ DWORD *pdwReserved,
  _In_ DWORD dwFlags,
  _Out_ DWORD *pdwProvType,
  _Out_writes_bytes_to_opt_(*pcbTypeName, *pcbTypeName) LPSTR szTypeName,
  _Inout_ DWORD *pcbTypeName);

_Success_(return != 0)
WINADVAPI
BOOL
WINAPI
CryptEnumProviderTypesW(
  _In_ DWORD dwIndex,
  _Reserved_ DWORD *pdwReserved,
  _In_ DWORD dwFlags,
  _Out_ DWORD *pdwProvType,
  _Out_writes_bytes_to_opt_(*pcbTypeName, *pcbTypeName) LPWSTR szTypeName,
  _Inout_ DWORD *pcbTypeName);

#define CryptEnumProviderTypes WINELIB_NAME_AW(CryptEnumProviderTypes)

WINADVAPI
BOOL
WINAPI
CryptExportKey(
  _In_ HCRYPTKEY hKey,
  _In_ HCRYPTKEY hExpKey,
  _In_ DWORD dwBlobType,
  _In_ DWORD dwFlags,
  _Out_writes_bytes_to_opt_(*pdwDataLen, *pdwDataLen) BYTE *pbData,
  _Inout_ DWORD *pdwDataLen);

WINADVAPI
BOOL
WINAPI
CryptGenKey(
  _In_ HCRYPTPROV,
  _In_ ALG_ID,
  _In_ DWORD,
  _Out_ HCRYPTKEY *);

WINADVAPI
BOOL
WINAPI
CryptGetKeyParam(
  _In_ HCRYPTKEY hKey,
  _In_ DWORD dwParam,
  _Out_writes_bytes_to_opt_(*pdwDataLen, *pdwDataLen) BYTE *pbData,
  _Inout_ DWORD *pdwDataLen,
  _In_ DWORD dwFlags);

WINADVAPI
BOOL
WINAPI
CryptGetHashParam(
  _In_ HCRYPTHASH hHash,
  _In_ DWORD dwParam,
  _Out_writes_bytes_to_opt_(*pdwDataLen, *pdwDataLen) BYTE *pbData,
  _Inout_ DWORD *pdwDataLen,
  _In_ DWORD dwFlags);

WINADVAPI
BOOL
WINAPI
CryptGetProvParam(
  _In_ HCRYPTPROV hProv,
  _In_ DWORD dwParam,
  _Out_writes_bytes_to_opt_(*pdwDataLen, *pdwDataLen) BYTE *pbData,
  _Inout_ DWORD *pdwDataLen,
  _In_ DWORD dwFlags);

_Success_(return != 0)
WINADVAPI
BOOL
WINAPI
CryptGetDefaultProviderA(
  _In_ DWORD dwProvType,
  _Reserved_ DWORD *pdwReserved,
  _In_ DWORD dwFlags,
  _Out_writes_bytes_to_opt_(*pcbProvName, *pcbProvName) LPSTR pszProvName,
  _Inout_ DWORD *pcbProvName);

_Success_(return != 0)
WINADVAPI
BOOL
WINAPI
CryptGetDefaultProviderW(
  _In_ DWORD dwProvType,
  _Reserved_ DWORD *pdwReserved,
  _In_ DWORD dwFlags,
  _Out_writes_bytes_to_opt_(*pcbProvName, *pcbProvName) LPWSTR pszProvName,
  _Inout_ DWORD *pcbProvName);

#define CryptGetDefaultProvider WINELIB_NAME_AW(CryptGetDefaultProvider)

WINADVAPI BOOL WINAPI CryptGetUserKey(_In_ HCRYPTPROV, _In_ DWORD, _Out_ HCRYPTKEY *);

WINADVAPI
BOOL
WINAPI
CryptHashData(
  _In_ HCRYPTHASH hHash,
  _In_reads_bytes_(dwDataLen) CONST BYTE *pbData,
  _In_ DWORD dwDataLen,
  _In_ DWORD dwFlags);

WINADVAPI BOOL WINAPI CryptHashSessionKey(_In_ HCRYPTHASH, _In_ HCRYPTKEY, _In_ DWORD);

WINADVAPI
BOOL
WINAPI
CryptImportKey(
  _In_ HCRYPTPROV hProv,
  _In_reads_bytes_(dwDataLen) CONST BYTE *pbData,
  _In_ DWORD dwDataLen,
  _In_ HCRYPTKEY hPubKey,
  _In_ DWORD dwFlags,
  _Out_ HCRYPTKEY *phKey);

#if (NTDDI_VERSION >= NTDDI_WINXP)
WINADVAPI BOOL WINAPI CryptReleaseContext(_In_ HCRYPTPROV, _In_ DWORD);
#else
WINADVAPI BOOL WINAPI CryptReleaseContext(_In_ HCRYPTPROV, _In_ ULONG_PTR);
#endif

WINADVAPI
BOOL
WINAPI
CryptSetHashParam(
  _In_ HCRYPTHASH,
  _In_ DWORD,
  _In_ CONST BYTE *,
  _In_ DWORD);

WINADVAPI
BOOL
WINAPI
CryptSetKeyParam(
  _In_ HCRYPTKEY,
  _In_ DWORD,
  _In_ CONST BYTE *,
  _In_ DWORD);

WINADVAPI BOOL WINAPI CryptSetProviderA(_In_ LPCSTR, _In_ DWORD);
WINADVAPI BOOL WINAPI CryptSetProviderW(_In_ LPCWSTR, _In_ DWORD);

#define CryptSetProvider WINELIB_NAME_AW(CryptSetProvider)

WINADVAPI
BOOL
WINAPI
CryptSetProviderExA(
  _In_ LPCSTR,
  _In_ DWORD,
  _Reserved_ DWORD *,
  _In_ DWORD);

WINADVAPI
BOOL
WINAPI
CryptSetProviderExW(
  _In_ LPCWSTR,
  _In_ DWORD,
  _Reserved_ DWORD *,
  _In_ DWORD);

#define CryptSetProviderEx WINELIB_NAME_AW(CryptSetProviderEx)

WINADVAPI BOOL WINAPI CryptSetProvParam(_In_ HCRYPTPROV, _In_ DWORD, _In_ CONST BYTE *, _In_ DWORD);

WINADVAPI
BOOL
WINAPI
CryptSignHashA(
  _In_ HCRYPTHASH hHash,
  _In_ DWORD dwKeySpec,
  _In_opt_ LPCSTR szDescription,
  _In_ DWORD dwFlags,
  _Out_writes_bytes_to_opt_(*pdwSigLen, *pdwSigLen) BYTE *pbSignature,
  _Inout_ DWORD *pdwSigLen);

WINADVAPI
BOOL
WINAPI
CryptSignHashW(
  _In_ HCRYPTHASH hHash,
  _In_ DWORD dwKeySpec,
  _In_opt_ LPCWSTR szDescription,
  _In_ DWORD dwFlags,
  _Out_writes_bytes_to_opt_(*pdwSigLen, *pdwSigLen) BYTE *pbSignature,
  _Inout_ DWORD *pdwSigLen);

#define CryptSignHash WINELIB_NAME_AW(CryptSignHash)

WINADVAPI
BOOL
WINAPI
CryptVerifySignatureA(
  _In_ HCRYPTHASH hHash,
  _In_reads_bytes_(dwSigLen) CONST BYTE *pbSignature,
  _In_ DWORD dwSigLen,
  _In_ HCRYPTKEY hPubKey,
  _In_opt_ LPCSTR szDescription,
  _In_ DWORD dwFlags);

WINADVAPI
BOOL
WINAPI
CryptVerifySignatureW(
  _In_ HCRYPTHASH hHash,
  _In_reads_bytes_(dwSigLen) CONST BYTE *pbSignature,
  _In_ DWORD dwSigLen,
  _In_ HCRYPTKEY hPubKey,
  _In_opt_ LPCWSTR szDescription,
  _In_ DWORD dwFlags);

#define CryptVerifySignature WINELIB_NAME_AW(CryptVerifySignature)

/* crypt32.dll functions */
LPVOID WINAPI CryptMemAlloc(_In_ ULONG cbSize) __WINE_ALLOC_SIZE(1);
LPVOID WINAPI CryptMemRealloc(_In_opt_ LPVOID pv, _In_ ULONG cbSize) __WINE_ALLOC_SIZE(2);
VOID   WINAPI CryptMemFree(_In_opt_ LPVOID pv);

_Success_(return != 0)
BOOL
WINAPI
CryptBinaryToStringA(
  _In_reads_bytes_(cbBinary) const BYTE *pbBinary,
  _In_ DWORD cbBinary,
  _In_ DWORD dwFlags,
  _Out_writes_to_opt_(*pcchString, *pcchString) LPSTR pszString,
  _Inout_ DWORD *pcchString);

_Success_(return != 0)
BOOL
WINAPI
CryptBinaryToStringW(
  _In_reads_bytes_(cbBinary) const BYTE *pbBinary,
  _In_ DWORD cbBinary,
  _In_ DWORD dwFlags,
  _Out_writes_to_opt_(*pcchString, *pcchString) LPWSTR pszString,
  _Inout_ DWORD *pcchString);

#define CryptBinaryToString WINELIB_NAME_AW(CryptBinaryToString)

BOOL
WINAPI
CryptStringToBinaryA(
  _In_reads_(cchString) LPCSTR pszString,
  _In_ DWORD cchString,
  _In_ DWORD dwFlags,
  _Out_writes_bytes_to_opt_(*pcbBinary, *pcbBinary) BYTE *pbBinary,
  _Inout_ DWORD  *pcbBinary,
  _Out_opt_ DWORD *pdwSkip,
  _Out_opt_ DWORD *pdwFlags);

BOOL
WINAPI
CryptStringToBinaryW(
  _In_reads_(cchString) LPCWSTR pszString,
  _In_ DWORD cchString,
  _In_ DWORD dwFlags,
  _Out_writes_bytes_to_opt_(*pcbBinary, *pcbBinary) BYTE *pbBinary,
  _Inout_ DWORD  *pcbBinary,
  _Out_opt_ DWORD *pdwSkip,
  _Out_opt_ DWORD *pdwFlags);

#define CryptStringToBinary WINELIB_NAME_AW(CryptStringToBinary)

BOOL
WINAPI
CryptCreateAsyncHandle(
  _In_ DWORD dwFlags,
  _Out_ PHCRYPTASYNC phAsync);

BOOL
WINAPI
CryptSetAsyncParam(
  _In_ HCRYPTASYNC hAsync,
  _In_ LPSTR pszParamOid,
  _In_opt_ LPVOID pvParam,
  __callback PFN_CRYPT_ASYNC_PARAM_FREE_FUNC pfnFree);

BOOL
WINAPI
CryptGetAsyncParam(
  _In_ HCRYPTASYNC hAsync,
  _In_ LPSTR pszParamOid,
  _Outptr_opt_result_maybenull_ LPVOID* ppvParam,
  _Outptr_opt_result_maybenull_ __callback PFN_CRYPT_ASYNC_PARAM_FREE_FUNC* ppfnFree);

BOOL
WINAPI
CryptRegisterDefaultOIDFunction(
  _In_ DWORD,
  _In_ LPCSTR,
  _In_ DWORD,
  _In_ LPCWSTR);

BOOL
WINAPI
CryptRegisterOIDFunction(
  _In_ DWORD,
  _In_ LPCSTR,
  _In_ LPCSTR,
  _In_opt_ LPCWSTR,
  _In_opt_ LPCSTR);

BOOL
WINAPI
CryptGetOIDFunctionValue(
  _In_ DWORD dwEncodingType,
  _In_ LPCSTR pszFuncName,
  _In_ LPCSTR pszOID,
  _In_opt_ LPCWSTR pwszValueName,
  _Out_opt_ DWORD *pdwValueType,
  _Out_writes_bytes_to_opt_(*pcbValueData, *pcbValueData) BYTE *pbValueData,
  _Inout_opt_ DWORD *pcbValueData);

BOOL
WINAPI
CryptSetOIDFunctionValue(
  _In_ DWORD dwEncodingType,
  _In_ LPCSTR pszFuncName,
  _In_ LPCSTR pszOID,
  _In_opt_ LPCWSTR pwszValueName,
  _In_ DWORD dwValueType,
  _In_reads_bytes_opt_(cbValueData) const BYTE *pbValueData,
  _In_ DWORD cbValueData);

BOOL WINAPI CryptUnregisterDefaultOIDFunction(_In_ DWORD, _In_ LPCSTR, _In_ LPCWSTR);
BOOL WINAPI CryptUnregisterOIDFunction(_In_ DWORD, _In_ LPCSTR, _In_ LPCSTR);

BOOL
WINAPI
CryptEnumOIDFunction(
  _In_ DWORD dwEncodingType,
  _In_opt_ LPCSTR pszFuncName,
  _In_opt_ LPCSTR pszOID,
  _In_ DWORD dwFlags,
  _Inout_opt_ void *pvArg,
  __callback PFN_CRYPT_ENUM_OID_FUNC pfnEnumOIDFunc);

HCRYPTOIDFUNCSET WINAPI CryptInitOIDFunctionSet(_In_ LPCSTR, _In_ DWORD);

_Success_(return != 0)
BOOL
WINAPI
CryptGetDefaultOIDDllList(
  _In_ HCRYPTOIDFUNCSET hFuncSet,
  _In_ DWORD dwEncodingType,
  _Out_writes_to_opt_(*pcchDllList, *pcchDllList) _Post_ _NullNull_terminated_ WCHAR *pwszDllList,
  _Inout_ DWORD *pcchDllList);

_Success_(return != 0)
BOOL
WINAPI
CryptGetDefaultOIDFunctionAddress(
  _In_ HCRYPTOIDFUNCSET hFuncSet,
  _In_ DWORD dwEncodingType,
  _In_opt_ LPCWSTR pwszDll,
  _In_ DWORD dwFlags,
  _Outptr_ void **ppvFuncAddr,
  _Inout_ HCRYPTOIDFUNCADDR *phFuncAddr);

_Success_(return != 0)
BOOL
WINAPI
CryptGetOIDFunctionAddress(
  _In_ HCRYPTOIDFUNCSET hFuncSet,
  _In_ DWORD dwEncodingType,
  _In_ LPCSTR pszOID,
  _In_ DWORD dwFlags,
  _Outptr_ void **ppvFuncAddr,
  _Out_ HCRYPTOIDFUNCADDR *phFuncAddr);

BOOL
WINAPI
CryptFreeOIDFunctionAddress(
  _In_ HCRYPTOIDFUNCADDR hFuncAddr,
  _In_ DWORD dwFlags);

BOOL
WINAPI
CryptInstallOIDFunctionAddress(
  _In_opt_ HMODULE hModule,
  _In_ DWORD dwEncodingType,
  _In_ LPCSTR pszFuncName,
  _In_ DWORD cFuncEntry,
  _In_reads_(cFuncEntry) const CRYPT_OID_FUNC_ENTRY rgFuncEntry[],
  _In_ DWORD dwFlags);

BOOL
WINAPI
CryptInstallDefaultContext(
  _In_ HCRYPTPROV hCryptProv,
  _In_ DWORD dwDefaultType,
  _In_opt_ const void *pvDefaultPara,
  _In_ DWORD dwFlags,
  _Reserved_ void *pvReserved,
  _Out_ HCRYPTDEFAULTCONTEXT *phDefaultContext);

BOOL
WINAPI
CryptUninstallDefaultContext(
  _In_opt_ HCRYPTDEFAULTCONTEXT hDefaultContext,
  _In_ DWORD dwFlags,
  _Reserved_ void *pvReserved);

BOOL
WINAPI
CryptEnumOIDInfo(
  _In_ DWORD dwGroupId,
  _In_ DWORD dwFlags,
  _Inout_opt_ void *pvArg,
  __callback PFN_CRYPT_ENUM_OID_INFO pfnEnumOIDInfo);

PCCRYPT_OID_INFO
WINAPI
CryptFindOIDInfo(
  _In_ DWORD dwKeyType,
  _In_ void *pvKey,
  _In_ DWORD dwGroupId);

BOOL WINAPI CryptRegisterOIDInfo(_In_ PCCRYPT_OID_INFO pInfo, _In_ DWORD dwFlags);
BOOL WINAPI CryptUnregisterOIDInfo(_In_ PCCRYPT_OID_INFO pInfo);

LPCWSTR WINAPI CryptFindLocalizedName(_In_ LPCWSTR pwszCryptName);

LPCSTR WINAPI CertAlgIdToOID(_In_ DWORD dwAlgId);
DWORD WINAPI CertOIDToAlgId(_In_ LPCSTR pszObjId);

/* cert store functions */
_Must_inspect_result_
HCERTSTORE
WINAPI
CertOpenStore(
  _In_ LPCSTR lpszStoreProvider,
  _In_ DWORD dwEncodingType,
  _In_opt_ HCRYPTPROV_LEGACY hCryptProv,
  _In_ DWORD dwFlags,
  _In_opt_ const void *pvPara);

HCERTSTORE
WINAPI
CertOpenSystemStoreA(
  _In_opt_ HCRYPTPROV_LEGACY hProv,
  _In_ LPCSTR szSubsystemProtocol);

HCERTSTORE
WINAPI
CertOpenSystemStoreW(
  _In_opt_ HCRYPTPROV_LEGACY hProv,
  _In_ LPCWSTR szSubSystemProtocol);

#define CertOpenSystemStore WINELIB_NAME_AW(CertOpenSystemStore)

PCCERT_CONTEXT
WINAPI
CertEnumCertificatesInStore(
  _In_ HCERTSTORE hCertStore,
  _In_opt_ PCCERT_CONTEXT pPrev);

PCCRL_CONTEXT
WINAPI
CertEnumCRLsInStore(
  _In_ HCERTSTORE hCertStore,
  _In_opt_ PCCRL_CONTEXT pPrevCrlContext);

PCCTL_CONTEXT
WINAPI
CertEnumCTLsInStore(
  _In_ HCERTSTORE hCertStore,
  _In_opt_ PCCTL_CONTEXT pPrevCtlContext);

BOOL
WINAPI
CertEnumSystemStoreLocation(
  _In_ DWORD dwFlags,
  _Inout_opt_ void *pvArg,
  __callback PFN_CERT_ENUM_SYSTEM_STORE_LOCATION pfnEnum);

BOOL
WINAPI
CertEnumSystemStore(
  _In_ DWORD dwFlags,
  _In_opt_ void *pvSystemStoreLocationPara,
  _Inout_opt_ void *pvArg,
  __callback PFN_CERT_ENUM_SYSTEM_STORE pfnEnum);

BOOL
WINAPI
CertEnumPhysicalStore(
  _In_ const void *pvSystemStore,
  _In_ DWORD dwFlags,
  _Inout_opt_ void *pvArg,
  __callback PFN_CERT_ENUM_PHYSICAL_STORE pfnEnum);

BOOL
WINAPI
CertRegisterPhysicalStore(
  _In_ const void *pvSystemStore,
  _In_ DWORD dwFlags,
  _In_ LPCWSTR pwszStoreName,
  _In_ PCERT_PHYSICAL_STORE_INFO pStoreInfo,
  _Reserved_ void *pvReserved);

BOOL
WINAPI
CertSaveStore(
  _In_ HCERTSTORE hCertStore,
  _In_ DWORD dwEncodingType,
  _In_ DWORD dwSaveAs,
  _In_ DWORD dwSaveTo,
  _Inout_ void *pvSaveToPara,
  _In_ DWORD dwFlags);

BOOL
WINAPI
CertAddStoreToCollection(
  _In_ HCERTSTORE hCollectionStore,
  _In_opt_ HCERTSTORE hSiblingStore,
  _In_ DWORD dwUpdateFlags,
  _In_ DWORD dwPriority);

void
WINAPI
CertRemoveStoreFromCollection(
  _In_ HCERTSTORE hCollectionStore,
  _In_ HCERTSTORE hSiblingStore);

_Success_(return != 0)
BOOL
WINAPI
CertCreateCertificateChainEngine(
  _In_ PCERT_CHAIN_ENGINE_CONFIG pConfig,
  _Out_ HCERTCHAINENGINE *phChainEngine);

BOOL WINAPI CertResyncCertificateChainEngine(_In_opt_ HCERTCHAINENGINE hChainEngine);

VOID WINAPI CertFreeCertificateChainEngine(_In_opt_ HCERTCHAINENGINE hChainEngine);

_Success_(return != 0)
BOOL
WINAPI
CertGetCertificateChain(
  _In_opt_ HCERTCHAINENGINE hChainEngine,
  _In_ PCCERT_CONTEXT pCertContext,
  _In_opt_ LPFILETIME pTime,
  _In_opt_ HCERTSTORE hAdditionalStore,
  _In_ PCERT_CHAIN_PARA pChainPara,
  _In_ DWORD dwFlags,
  _Reserved_ LPVOID pvReserved,
  _Out_ PCCERT_CHAIN_CONTEXT* ppChainContext);

PCCERT_CHAIN_CONTEXT
WINAPI
CertDuplicateCertificateChain(
  _In_ PCCERT_CHAIN_CONTEXT pChainContext);

VOID WINAPI CertFreeCertificateChain(_In_ PCCERT_CHAIN_CONTEXT pChainContext);

PCCERT_CHAIN_CONTEXT
WINAPI
CertFindChainInStore(
  _In_ HCERTSTORE hCertStore,
  _In_ DWORD dwCertEncodingType,
  _In_ DWORD dwFindFlags,
  _In_ DWORD dwFindType,
  _In_opt_ const void *pvFindPara,
  _In_opt_ PCCERT_CHAIN_CONTEXT pPrevChainContext);

BOOL
WINAPI
CertVerifyCertificateChainPolicy(
  _In_ LPCSTR pszPolicyOID,
  _In_ PCCERT_CHAIN_CONTEXT pChainContext,
  _In_ PCERT_CHAIN_POLICY_PARA pPolicyPara,
  _Inout_ PCERT_CHAIN_POLICY_STATUS pPolicyStatus);

DWORD
WINAPI
CertEnumCertificateContextProperties(
  _In_ PCCERT_CONTEXT pCertContext,
  _In_ DWORD dwPropId);

BOOL
WINAPI
CertGetCertificateContextProperty(
  _In_ PCCERT_CONTEXT pCertContext,
  _In_ DWORD dwPropId,
  _Out_writes_bytes_to_opt_(*pcbData, *pcbData) void *pvData,
  _Inout_ DWORD *pcbData);

BOOL
WINAPI
CertSetCertificateContextProperty(
  _In_ PCCERT_CONTEXT pCertContext,
  _In_ DWORD dwPropId,
  _In_ DWORD dwFlags,
  _In_opt_ const void *pvData);

DWORD
WINAPI
CertEnumCRLContextProperties(
  _In_ PCCRL_CONTEXT pCRLContext,
  _In_ DWORD dwPropId);

BOOL
WINAPI
CertGetCRLContextProperty(
  _In_ PCCRL_CONTEXT pCrlContext,
  _In_ DWORD dwPropId,
  _Out_writes_bytes_to_opt_(*pcbData, *pcbData) void *pvData,
  _Inout_ DWORD *pcbData);

BOOL
WINAPI
CertSetCRLContextProperty(
  _In_ PCCRL_CONTEXT pCrlContext,
  _In_ DWORD dwPropId,
  _In_ DWORD dwFlags,
  _In_opt_ const void *pvData);

DWORD
WINAPI
CertEnumCTLContextProperties(
  _In_ PCCTL_CONTEXT pCTLContext,
  _In_ DWORD dwPropId);

BOOL
WINAPI
CertEnumSubjectInSortedCTL(
  _In_ PCCTL_CONTEXT pCtlContext,
  _Inout_ void **ppvNextSubject,
  _Out_opt_ PCRYPT_DER_BLOB pSubjectIdentifier,
  _Out_opt_ PCRYPT_DER_BLOB pEncodedAttributes);

BOOL
WINAPI
CertGetCTLContextProperty(
  _In_ PCCTL_CONTEXT pCtlContext,
  _In_ DWORD dwPropId,
  _Out_writes_bytes_to_opt_(*pcbData, *pcbData) void *pvData,
  _Inout_ DWORD *pcbData);

BOOL
WINAPI
CertSetCTLContextProperty(
  _In_ PCCTL_CONTEXT pCtlContext,
  _In_ DWORD dwPropId,
  _In_ DWORD dwFlags,
  _In_opt_ const void *pvData);

_Success_(return != 0)
BOOL
WINAPI
CertGetStoreProperty(
  _In_ HCERTSTORE hCertStore,
  _In_ DWORD dwPropId,
  _Out_writes_bytes_to_opt_(*pcbData, *pcbData) void *pvData,
  _Inout_ DWORD *pcbData);

BOOL
WINAPI
CertSetStoreProperty(
  _In_ HCERTSTORE hCertStore,
  _In_ DWORD dwPropId,
  _In_ DWORD dwFlags,
  _In_opt_ const void *pvData);

BOOL
WINAPI
CertControlStore(
  _In_ HCERTSTORE hCertStore,
  _In_ DWORD dwFlags,
  _In_ DWORD dwCtrlType,
  _In_opt_ void const *pvCtrlPara);

HCERTSTORE WINAPI CertDuplicateStore(_In_ HCERTSTORE hCertStore);

BOOL WINAPI CertCloseStore(_In_opt_ HCERTSTORE hCertStore, _In_ DWORD dwFlags);

BOOL WINAPI CertFreeCertificateContext(_In_opt_ PCCERT_CONTEXT pCertContext);

BOOL WINAPI CertFreeCRLContext(_In_opt_ PCCRL_CONTEXT pCrlContext);

BOOL WINAPI CertFreeCTLContext(_In_opt_ PCCTL_CONTEXT pCtlContext);

_Success_(return != 0)
BOOL
WINAPI
CertAddCertificateContextToStore(
  _In_opt_ HCERTSTORE hCertStore,
  _In_ PCCERT_CONTEXT pCertContext,
  _In_ DWORD dwAddDisposition,
  _Outptr_opt_ PCCERT_CONTEXT *ppStoreContext);

_Success_(return != 0)
BOOL
WINAPI
CertAddCRLContextToStore(
  _In_opt_ HCERTSTORE hCertStore,
  _In_ PCCRL_CONTEXT pCrlContext,
  _In_ DWORD dwAddDisposition,
  _Outptr_opt_ PCCRL_CONTEXT *ppStoreContext);

_Success_(return != 0)
BOOL
WINAPI
CertAddCTLContextToStore(
  _In_opt_ HCERTSTORE hCertStore,
  _In_ PCCTL_CONTEXT pCtlContext,
  _In_ DWORD dwAddDisposition,
  _Outptr_opt_ PCCTL_CONTEXT *ppStoreContext);

_Success_(return != 0)
BOOL
WINAPI
CertAddCertificateLinkToStore(
  _In_ HCERTSTORE hCertStore,
  _In_ PCCERT_CONTEXT pCertContext,
  _In_ DWORD dwAddDisposition,
  _Outptr_opt_ PCCERT_CONTEXT *ppStoreContext);

_Success_(return != 0)
BOOL
WINAPI
CertAddCRLLinkToStore(
  _In_ HCERTSTORE hCertStore,
  _In_ PCCRL_CONTEXT pCrlContext,
  _In_ DWORD dwAddDisposition,
  _Outptr_opt_ PCCRL_CONTEXT *ppStoreContext);

_Success_(return != 0)
BOOL
WINAPI
CertAddCTLLinkToStore(
  _In_ HCERTSTORE hCertStore,
  _In_ PCCTL_CONTEXT pCtlContext,
  _In_ DWORD dwAddDisposition,
  _Outptr_opt_ PCCTL_CONTEXT *ppStoreContext);

_Success_(return != 0)
BOOL
WINAPI
CertAddEncodedCertificateToStore(
  _In_opt_ HCERTSTORE hCertStore,
  _In_ DWORD dwCertEncodingType,
  _In_reads_bytes_(cbCertEncoded) const BYTE *pbCertEncoded,
  _In_ DWORD cbCertEncoded,
  _In_ DWORD dwAddDisposition,
  _Outptr_opt_ PCCERT_CONTEXT *ppCertContext);

BOOL
WINAPI
CertAddEncodedCertificateToSystemStoreA(
  _In_ LPCSTR szCertStoreName,
  _In_reads_bytes_(cbCertEncoded) const BYTE *pbCertEncoded,
  _In_ DWORD cbCertEncoded);

BOOL
WINAPI
CertAddEncodedCertificateToSystemStoreW(
  _In_ LPCWSTR szCertStoreName,
  _In_reads_bytes_(cbCertEncoded) const BYTE *pbCertEncoded,
  _In_ DWORD cbCertEncoded);

#define CertAddEncodedCertificateToSystemStore \
 WINELIB_NAME_AW(CertAddEncodedCertificateToSystemStore)

_Success_(return != 0)
BOOL
WINAPI
CertAddEncodedCRLToStore(
  _In_opt_ HCERTSTORE hCertStore,
  _In_ DWORD dwCertEncodingType,
  _In_reads_bytes_(cbCrlEncoded) const BYTE *pbCrlEncoded,
  _In_ DWORD cbCrlEncoded,
  _In_ DWORD dwAddDisposition,
  _Outptr_opt_ PCCRL_CONTEXT *ppCrlContext);

_Success_(return != 0)
BOOL
WINAPI
CertAddEncodedCTLToStore(
  _In_opt_ HCERTSTORE hCertStore,
  _In_ DWORD dwMsgAndCertEncodingType,
  _In_reads_bytes_(cbCtlEncoded) const BYTE *pbCtlEncoded,
  _In_ DWORD cbCtlEncoded,
  _In_ DWORD dwAddDisposition,
  _Outptr_opt_ PCCTL_CONTEXT *ppCtlContext);

_Success_(return != 0)
BOOL
WINAPI
CertAddSerializedElementToStore(
  _In_opt_ HCERTSTORE hCertStore,
  _In_reads_bytes_(cbElement) const BYTE *pbElement,
  _In_ DWORD cbElement,
  _In_ DWORD dwAddDisposition,
  _In_ DWORD dwFlags,
  _In_ DWORD dwContextTypeFlags,
  _Out_opt_ DWORD *pdwContextType,
  _Outptr_opt_ const void **ppvContext);

BOOL
WINAPI
CertCompareCertificate(
  _In_ DWORD dwCertEncodingType,
  _In_ PCERT_INFO pCertId1,
  _In_ PCERT_INFO pCertId2);

BOOL
WINAPI
CertCompareCertificateName(
  _In_ DWORD dwCertEncodingType,
  _In_ PCERT_NAME_BLOB pCertName1,
  _In_ PCERT_NAME_BLOB pCertName2);

BOOL
WINAPI
CertCompareIntegerBlob(
  _In_ PCRYPT_INTEGER_BLOB pInt1,
  _In_ PCRYPT_INTEGER_BLOB pInt2);

BOOL
WINAPI
CertComparePublicKeyInfo(
  _In_ DWORD dwCertEncodingType,
  _In_ PCERT_PUBLIC_KEY_INFO pPublicKey1,
  _In_ PCERT_PUBLIC_KEY_INFO pPublicKey2);

DWORD
WINAPI
CertGetPublicKeyLength(
  _In_ DWORD dwCertEncodingType,
  _In_ PCERT_PUBLIC_KEY_INFO pPublicKey);

const void *
WINAPI
CertCreateContext(
  _In_ DWORD dwContextType,
  _In_ DWORD dwEncodingType,
  _In_reads_bytes_(cbEncoded) const BYTE *pbEncoded,
  _In_ DWORD cbEncoded,
  _In_ DWORD dwFlags,
  _In_opt_ PCERT_CREATE_CONTEXT_PARA pCreatePara);

PCCERT_CONTEXT
WINAPI
CertCreateCertificateContext(
  _In_ DWORD dwCertEncodingType,
  _In_reads_bytes_(cbCertEncoded) const BYTE *pbCertEncoded,
  _In_ DWORD cbCertEncoded);

PCCRL_CONTEXT
WINAPI
CertCreateCRLContext(
  _In_ DWORD dwCertEncodingType,
  _In_reads_bytes_(cbCrlEncoded) const BYTE *pbCrlEncoded,
  _In_ DWORD cbCrlEncoded);

PCCTL_CONTEXT
WINAPI
CertCreateCTLContext(
  _In_ DWORD dwMsgAndCertEncodingType,
  _In_reads_bytes_(cbCtlEncoded) const BYTE *pbCtlEncoded,
  _In_ DWORD cbCtlEncoded);

PCCERT_CONTEXT
WINAPI
CertCreateSelfSignCertificate(
  _In_opt_ HCRYPTPROV_OR_NCRYPT_KEY_HANDLE hProv,
  _In_ PCERT_NAME_BLOB pSubjectIssuerBlob,
  _In_ DWORD dwFlags,
  _In_opt_ PCRYPT_KEY_PROV_INFO pKeyProvInfo,
  _In_opt_ PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm,
  _In_opt_ PSYSTEMTIME pStartTime,
  _In_opt_ PSYSTEMTIME pEndTime,
  _In_opt_ PCERT_EXTENSIONS pExtensions);

BOOL WINAPI CertDeleteCertificateFromStore(_In_ PCCERT_CONTEXT pCertContext);

BOOL WINAPI CertDeleteCRLFromStore(_In_ PCCRL_CONTEXT pCrlContext);

BOOL WINAPI CertDeleteCTLFromStore(_In_ PCCTL_CONTEXT pCtlContext);

PCCERT_CONTEXT
WINAPI
CertDuplicateCertificateContext(
  _In_opt_ PCCERT_CONTEXT pCertContext);

PCCRL_CONTEXT WINAPI CertDuplicateCRLContext(_In_opt_ PCCRL_CONTEXT pCrlContext);

PCCTL_CONTEXT WINAPI CertDuplicateCTLContext(_In_opt_ PCCTL_CONTEXT pCtlContext);

PCCERT_CONTEXT
WINAPI
CertFindCertificateInStore(
  _In_ HCERTSTORE hCertStore,
  _In_ DWORD dwCertEncodingType,
  _In_ DWORD dwFindFlags,
  _In_ DWORD dwFindType,
  _In_opt_ const void *pvFindPara,
  _In_opt_ PCCERT_CONTEXT pPrevCertContext);

PCCRL_CONTEXT
WINAPI
CertFindCRLInStore(
  _In_ HCERTSTORE hCertStore,
  _In_ DWORD dwCertEncodingType,
  _In_ DWORD dwFindFlags,
  _In_ DWORD dwFindType,
  _In_opt_ const void *pvFindPara,
  _In_opt_ PCCRL_CONTEXT pPrevCrlContext);

PCCTL_CONTEXT
WINAPI
CertFindCTLInStore(
  _In_ HCERTSTORE hCertStore,
  _In_ DWORD dwMsgAndCertEncodingType,
  _In_ DWORD dwFindFlags,
  _In_ DWORD dwFindType,
  _In_opt_ const void *pvFindPara,
  _In_opt_ PCCTL_CONTEXT pPrevCtlContext);

PCCERT_CONTEXT
WINAPI
CertGetIssuerCertificateFromStore(
  _In_ HCERTSTORE hCertStore,
  _In_ PCCERT_CONTEXT pSubjectContext,
  _In_opt_ PCCERT_CONTEXT pPrevIssuerContext,
  _Inout_ DWORD *pdwFlags);

PCCERT_CONTEXT
WINAPI
CertGetSubjectCertificateFromStore(
  _In_ HCERTSTORE hCertStore,
  _In_ DWORD dwCertEncodingType,
  _In_ PCERT_INFO pCertId);

PCCRL_CONTEXT
WINAPI
CertGetCRLFromStore(
  _In_ HCERTSTORE hCertStore,
  _In_opt_ PCCERT_CONTEXT pIssuerContext,
  _In_opt_ PCCRL_CONTEXT pPrevCrlContext,
  _Inout_ DWORD *pdwFlags);

BOOL
WINAPI
CertSerializeCertificateStoreElement(
  _In_ PCCERT_CONTEXT pCertContext,
  _In_ DWORD dwFlags,
  _Out_writes_bytes_to_opt_(*pcbElement, *pcbElement) BYTE *pbElement,
  _Inout_ DWORD *pcbElement);

BOOL
WINAPI
CertSerializeCRLStoreElement(
  _In_ PCCRL_CONTEXT pCrlContext,
  _In_ DWORD dwFlags,
  _Out_writes_bytes_to_opt_(*pcbElement, *pcbElement) BYTE *pbElement,
  _Inout_ DWORD *pcbElement);

BOOL
WINAPI
CertSerializeCTLStoreElement(
  _In_ PCCTL_CONTEXT pCtlContext,
  _In_ DWORD dwFlags,
  _Out_writes_bytes_to_opt_(*pcbElement, *pcbElement) BYTE *pbElement,
  _Inout_ DWORD *pcbElement);

BOOL
WINAPI
CertGetIntendedKeyUsage(
  _In_ DWORD dwCertEncodingType,
  _In_ PCERT_INFO pCertInfo,
  _Out_writes_bytes_all_(cbKeyUsage) BYTE *pbKeyUsage,
  _In_ DWORD cbKeyUsage);

BOOL
WINAPI
CertGetEnhancedKeyUsage(
  _In_ PCCERT_CONTEXT pCertContext,
  _In_ DWORD dwFlags,
  _Out_writes_bytes_to_opt_(*pcbUsage, *pcbUsage) PCERT_ENHKEY_USAGE pUsage,
  _Inout_ DWORD *pcbUsage);

BOOL
WINAPI
CertSetEnhancedKeyUsage(
  _In_ PCCERT_CONTEXT pCertContext,
  _In_opt_ PCERT_ENHKEY_USAGE pUsage);

BOOL
WINAPI
CertAddEnhancedKeyUsageIdentifier(
  _In_ PCCERT_CONTEXT pCertContext,
  _In_ LPCSTR pszUsageIdentifier);

BOOL
WINAPI
CertRemoveEnhancedKeyUsageIdentifier(
  _In_ PCCERT_CONTEXT pCertContext,
  _In_ LPCSTR pszUsageIdentifier);

_Success_(return != 0)
BOOL
WINAPI
CertGetValidUsages(
  _In_ DWORD cCerts,
  _In_reads_(cCerts) PCCERT_CONTEXT *rghCerts,
  _Out_ int *cNumOIDs,
  _Out_writes_bytes_to_opt_(*pcbOIDs, *pcbOIDs) LPSTR *rghOIDs,
  _Inout_ DWORD *pcbOIDs);

BOOL
WINAPI
CryptEncodeObject(
  _In_ DWORD dwCertEncodingType,
  _In_ LPCSTR lpszStructType,
  _In_ const void *pvStructInfo,
  _Out_writes_bytes_to_opt_(*pcbEncoded, *pcbEncoded) BYTE *pbEncoded,
  _Inout_ DWORD *pcbEncoded);

BOOL
WINAPI
CryptEncodeObjectEx(
  _In_ DWORD dwCertEncodingType,
  _In_ LPCSTR lpszStructType,
  _In_ const void *pvStructInfo,
  _In_ DWORD dwFlags,
  _In_opt_ PCRYPT_ENCODE_PARA pEncodePara,
  _Out_opt_ void *pvEncoded,
  _Inout_ DWORD *pcbEncoded);

BOOL
WINAPI
CryptDecodeObject(
  _In_ DWORD dwCertEncodingType,
  _In_ LPCSTR lpszStructType,
  _In_reads_bytes_(cbEncoded) const BYTE *pbEncoded,
  _In_ DWORD cbEncoded,
  _In_ DWORD dwFlags,
  _Out_writes_bytes_to_opt_(*pcbStructInfo, *pcbStructInfo) void *pvStructInfo,
  _Inout_ DWORD *pcbStructInfo);

BOOL
WINAPI
CryptDecodeObjectEx(
  _In_ DWORD dwCertEncodingType,
  _In_ LPCSTR lpszStructType,
  _In_reads_bytes_(cbEncoded) const BYTE *pbEncoded,
  _In_ DWORD cbEncoded,
  _In_ DWORD dwFlags,
  _In_opt_ PCRYPT_DECODE_PARA pDecodePara,
  _Out_opt_ void *pvStructInfo,
  _Inout_ DWORD *pcbStructInfo);

BOOL
WINAPI
CryptFormatObject(
  _In_ DWORD dwCertEncodingType,
  _In_ DWORD dwFormatType,
  _In_ DWORD dwFormatStrType,
  _In_opt_ void *pFormatStruct,
  _In_opt_ LPCSTR lpszStructType,
  _In_reads_bytes_(cbEncoded) const BYTE *pbEncoded,
  _In_ DWORD cbEncoded,
  _At_((WCHAR *) pbFormat, _Out_writes_bytes_to_opt_(*pcbFormat, *pcbFormat)) void *pbFormat,
  _Inout_ DWORD *pcbFormat);

BOOL
WINAPI
CryptHashCertificate(
  _In_opt_ HCRYPTPROV_LEGACY hCryptProv,
  _In_ ALG_ID Algid,
  _In_ DWORD dwFlags,
  _In_reads_bytes_(cbEncoded) const BYTE *pbEncoded,
  _In_ DWORD cbEncoded,
  _Out_writes_bytes_to_opt_(*pcbComputedHash, *pcbComputedHash) BYTE *pbComputedHash,
  _Inout_ DWORD *pcbComputedHash);

BOOL
WINAPI
CryptHashPublicKeyInfo(
  _In_opt_ HCRYPTPROV_LEGACY hCryptProv,
  _In_ ALG_ID Algid,
  _In_ DWORD dwFlags,
  _In_ DWORD dwCertEncodingType,
  _In_ PCERT_PUBLIC_KEY_INFO pInfo,
  _Out_writes_bytes_to_opt_(*pcbComputedHash, *pcbComputedHash) BYTE *pbComputedHash,
  _Inout_ DWORD *pcbComputedHash);

BOOL
WINAPI
CryptHashToBeSigned(
  _In_opt_ HCRYPTPROV_LEGACY hCryptProv,
  _In_ DWORD dwCertEncodingType,
  _In_reads_bytes_(cbEncoded) const BYTE *pbEncoded,
  _In_ DWORD cbEncoded,
  _Out_writes_bytes_to_opt_(*pcbComputedHash, *pcbComputedHash) BYTE *pbComputedHash,
  _Inout_ DWORD *pcbComputedHash);

BOOL
WINAPI
CryptQueryObject(
  _In_ DWORD dwObjectType,
  _In_ const void *pvObject,
  _In_ DWORD dwExpectedContentTypeFlags,
  _In_ DWORD dwExpectedFormatTypeFlags,
  _In_ DWORD dwFlags,
  _Out_opt_ DWORD *pdwMsgAndCertEncodingType,
  _Out_opt_ DWORD *pdwContentType,
  _Out_opt_ DWORD *pdwFormatType,
  _Out_opt_ HCERTSTORE *phCertStore,
  _Out_opt_ HCRYPTMSG *phMsg,
  _Outptr_opt_result_maybenull_ const void **ppvContext);

BOOL
WINAPI
CryptSignCertificate(
  _In_opt_ HCRYPTPROV_OR_NCRYPT_KEY_HANDLE hCryptProvOrNCryptKey,
  _In_opt_ DWORD dwKeySpec,
  _In_ DWORD dwCertEncodingType,
  _In_reads_bytes_(cbEncodedToBeSigned) const BYTE *pbEncodedToBeSigned,
  _In_ DWORD cbEncodedToBeSigned,
  _In_ PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm,
  _In_opt_ const void *pvHashAuxInfo,
  _Out_writes_bytes_to_opt_(*pcbSignature, *pcbSignature) BYTE *pbSignature,
  _Inout_ DWORD *pcbSignature);

BOOL
WINAPI
CryptSignAndEncodeCertificate(
  _In_opt_ HCRYPTPROV_OR_NCRYPT_KEY_HANDLE hCryptProvOrNCryptKey,
  _In_opt_ DWORD dwKeySpec,
  _In_ DWORD dwCertEncodingType,
  _In_ LPCSTR lpszStructType,
  _In_ const void *pvStructInfo,
  _In_ PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm,
  _In_opt_ const void *pvHashAuxInfo,
  _Out_writes_bytes_to_opt_(*pcbEncoded, *pcbEncoded) BYTE *pbEncoded,
  _Inout_ DWORD *pcbEncoded);

_Must_inspect_result_
BOOL
WINAPI
CryptVerifyCertificateSignature(
  _In_opt_ HCRYPTPROV_LEGACY hCryptProv,
  _In_ DWORD dwCertEncodingType,
  _In_reads_bytes_(cbEncoded) const BYTE *pbEncoded,
  _In_ DWORD cbEncoded,
  _In_ PCERT_PUBLIC_KEY_INFO pPublicKey);

_Must_inspect_result_
BOOL
WINAPI
CryptVerifyCertificateSignatureEx(
  _In_opt_ HCRYPTPROV_LEGACY hCryptProv,
  _In_ DWORD dwCertEncodingType,
  _In_ DWORD dwSubjectType,
  _In_ void *pvSubject,
  _In_ DWORD dwIssuerType,
  _In_opt_ void *pvIssuer,
  _In_ DWORD dwFlags,
  _Inout_opt_ void *pvExtra);

PCRYPT_ATTRIBUTE
WINAPI
CertFindAttribute(
  _In_ LPCSTR pszObjId,
  _In_ DWORD cAttr,
  _In_reads_(cAttr) CRYPT_ATTRIBUTE rgAttr[]);

PCERT_EXTENSION
WINAPI
CertFindExtension(
  _In_ LPCSTR pszObjId,
  _In_ DWORD cExtensions,
  _In_reads_(cExtensions) CERT_EXTENSION rgExtensions[]);

PCERT_RDN_ATTR
WINAPI
CertFindRDNAttr(
  _In_ LPCSTR pszObjId,
  _In_ PCERT_NAME_INFO pName);

BOOL
WINAPI
CertFindSubjectInSortedCTL(
  _In_ PCRYPT_DATA_BLOB pSubjectIdentifier,
  _In_ PCCTL_CONTEXT pCtlContext,
  _In_ DWORD dwFlags,
  _Reserved_ void *pvReserved,
  _Out_opt_ PCRYPT_DER_BLOB pEncodedAttributes);

BOOL
WINAPI
CertIsRDNAttrsInCertificateName(
  _In_ DWORD dwCertEncodingType,
  _In_ DWORD dwFlags,
  _In_ PCERT_NAME_BLOB pCertName,
  _In_ PCERT_RDN pRDN);

BOOL
WINAPI
CertIsValidCRLForCertificate(
  _In_ PCCERT_CONTEXT pCert,
  _In_ PCCRL_CONTEXT pCrl,
  _In_ DWORD dwFlags,
  _Reserved_ void *pvReserved);

BOOL
WINAPI
CertFindCertificateInCRL(
  _In_ PCCERT_CONTEXT pCert,
  _In_ PCCRL_CONTEXT pCrlContext,
  _In_ DWORD dwFlags,
  _Reserved_ void *pvReserved,
  _Outptr_result_maybenull_ PCRL_ENTRY *ppCrlEntry);

BOOL
WINAPI
CertVerifyCRLRevocation(
  _In_ DWORD dwCertEncodingType,
  _In_ PCERT_INFO pCertId,
  _In_ DWORD cCrlInfo,
  _In_reads_(cCrlInfo) PCRL_INFO rgpCrlInfo[]);

BOOL
WINAPI
CertVerifySubjectCertificateContext(
  _In_ PCCERT_CONTEXT pSubject,
  _In_opt_ PCCERT_CONTEXT pIssuer,
  _Inout_ DWORD *pdwFlags);

LONG
WINAPI
CertVerifyCRLTimeValidity(
  _In_opt_ LPFILETIME pTimeToVerify,
  _In_ PCRL_INFO pCrlInfo);

LONG
WINAPI
CertVerifyTimeValidity(
  _In_opt_ LPFILETIME pTimeToVerify,
  _In_ PCERT_INFO pCertInfo);

BOOL
WINAPI
CertVerifyValidityNesting(
  _In_ PCERT_INFO pSubjectInfo,
  _In_ PCERT_INFO pIssuerInfo);

BOOL
WINAPI
CertVerifyCTLUsage(
  _In_ DWORD dwEncodingType,
  _In_ DWORD dwSubjectType,
  _In_ void *pvSubject,
  _In_ PCTL_USAGE pSubjectUsage,
  _In_ DWORD dwFlags,
  _In_opt_ PCTL_VERIFY_USAGE_PARA pVerifyUsagePara,
  _Inout_ PCTL_VERIFY_USAGE_STATUS pVerifyUsageStatus);

BOOL
WINAPI
CertVerifyRevocation(
  _In_ DWORD dwEncodingType,
  _In_ DWORD dwRevType,
  _In_ DWORD cContext,
  _In_reads_(cContext) PVOID rgpvContext[],
  _In_ DWORD dwFlags,
  _In_opt_ PCERT_REVOCATION_PARA pRevPara,
  _Inout_ PCERT_REVOCATION_STATUS pRevStatus);

BOOL
WINAPI
CryptExportPublicKeyInfo(
  _In_ HCRYPTPROV_OR_NCRYPT_KEY_HANDLE hCryptProvOrNCryptKey,
  _In_opt_ DWORD dwKeySpec,
  _In_ DWORD dwCertEncodingType,
  _Out_writes_bytes_to_opt_(*pcbInfo, *pcbInfo) PCERT_PUBLIC_KEY_INFO pInfo,
  _Inout_ DWORD *pcbInfo);

BOOL
WINAPI
CryptExportPublicKeyInfoEx(
  _In_ HCRYPTPROV_OR_NCRYPT_KEY_HANDLE hCryptProvOrNCryptKey,
  _In_opt_ DWORD dwKeySpec,
  _In_ DWORD dwCertEncodingType,
  _In_opt_ LPSTR pszPublicKeyObjId,
  _In_ DWORD dwFlags,
  _In_opt_ void *pvAuxInfo,
  _Out_writes_bytes_to_opt_(*pcbInfo, *pcbInfo) PCERT_PUBLIC_KEY_INFO pInfo,
  _Inout_ DWORD *pcbInfo);

BOOL
WINAPI
CryptImportPublicKeyInfo(
  _In_ HCRYPTPROV hCryptProv,
  _In_ DWORD dwCertEncodingType,
  _In_ PCERT_PUBLIC_KEY_INFO pInfo,
  _Out_ HCRYPTKEY *phKey);

BOOL
WINAPI
CryptImportPublicKeyInfoEx(
  _In_ HCRYPTPROV hCryptProv,
  _In_ DWORD dwCertEncodingType,
  _In_ PCERT_PUBLIC_KEY_INFO pInfo,
  _In_ ALG_ID aiKeyAlg,
  _In_ DWORD dwFlags,
  _In_opt_ void *pvAuxInfo,
  _Out_ HCRYPTKEY *phKey);

BOOL
WINAPI
CryptAcquireCertificatePrivateKey(
  _In_ PCCERT_CONTEXT pCert,
  _In_ DWORD dwFlags,
  _In_opt_ void *pvParameters,
  _Out_ HCRYPTPROV_OR_NCRYPT_KEY_HANDLE *phCryptProvOrNCryptKey,
  _Out_opt_ DWORD *pdwKeySpec,
  _Out_opt_ BOOL *pfCallerFreeProvOrNCryptKey);

BOOL
WINAPI
CryptFindCertificateKeyProvInfo(
  _In_ PCCERT_CONTEXT pCert,
  _In_ DWORD dwFlags,
  _Reserved_ void *pvReserved);

BOOL WINAPI CryptProtectData( DATA_BLOB* pDataIn, LPCWSTR szDataDescr,
 DATA_BLOB* pOptionalEntropy, PVOID pvReserved,
 CRYPTPROTECT_PROMPTSTRUCT* pPromptStruct, DWORD dwFlags, DATA_BLOB* pDataOut );

BOOL WINAPI CryptUnprotectData( DATA_BLOB* pDataIn, LPWSTR* ppszDataDescr,
 DATA_BLOB* pOptionalEntropy, PVOID pvReserved,
 CRYPTPROTECT_PROMPTSTRUCT* pPromptStruct, DWORD dwFlags, DATA_BLOB* pDataOut );

DWORD
WINAPI
CertGetNameStringA(
  _In_ PCCERT_CONTEXT pCertContext,
  _In_ DWORD dwType,
  _In_ DWORD dwFlags,
  _In_opt_ void *pvTypePara,
  _Out_writes_to_opt_(cchNameString, return) LPSTR pszNameString,
  _In_ DWORD cchNameString);

DWORD
WINAPI
CertGetNameStringW(
  _In_ PCCERT_CONTEXT pCertContext,
  _In_ DWORD dwType,
  _In_ DWORD dwFlags,
  _In_opt_ void *pvTypePara,
  _Out_writes_to_opt_(cchNameString, return) LPWSTR pszNameString,
  _In_ DWORD cchNameString);

#define CertGetNameString WINELIB_NAME_AW(CertGetNameString)

DWORD
WINAPI
CertRDNValueToStrA(
  _In_ DWORD dwValueType,
  _In_ PCERT_RDN_VALUE_BLOB pValue,
  _Out_writes_to_opt_(csz, return) LPSTR psz,
  _In_ DWORD csz);

DWORD
WINAPI
CertRDNValueToStrW(
  _In_ DWORD dwValueType,
  _In_ PCERT_RDN_VALUE_BLOB pValue,
  _Out_writes_to_opt_(csz, return) LPWSTR psz,
  _In_ DWORD csz);

#define CertRDNValueToStr WINELIB_NAME_AW(CertRDNValueToStr)

DWORD
WINAPI
CertNameToStrA(
  _In_ DWORD dwCertEncodingType,
  _In_ PCERT_NAME_BLOB pName,
  _In_ DWORD dwStrType,
  _Out_writes_to_opt_(csz, return) LPSTR psz,
  _In_ DWORD csz);

DWORD
WINAPI
CertNameToStrW(
  _In_ DWORD dwCertEncodingType,
  _In_ PCERT_NAME_BLOB pName,
  _In_ DWORD dwStrType,
  _Out_writes_to_opt_(csz, return) LPWSTR psz,
  _In_ DWORD csz);

#define CertNameToStr WINELIB_NAME_AW(CertNameToStr)

BOOL
WINAPI
CertStrToNameA(
  _In_ DWORD dwCertEncodingType,
  _In_ LPCSTR pszX500,
  _In_ DWORD dwStrType,
  _Reserved_ void *pvReserved,
  _Out_writes_bytes_to_opt_(*pcbEncoded, *pcbEncoded) BYTE *pbEncoded,
  _Inout_ DWORD *pcbEncoded,
  _Outptr_opt_result_maybenull_ LPCSTR *ppszError);

BOOL
WINAPI
CertStrToNameW(
  _In_ DWORD dwCertEncodingType,
  _In_ LPCWSTR pszX500,
  _In_ DWORD dwStrType,
  _Reserved_ void *pvReserved,
  _Out_writes_bytes_to_opt_(*pcbEncoded, *pcbEncoded) BYTE *pbEncoded,
  _Inout_ DWORD *pcbEncoded,
  _Outptr_opt_result_maybenull_ LPCWSTR *ppszError);

#define CertStrToName WINELIB_NAME_AW(CertStrToName)

DWORD
WINAPI
CryptMsgCalculateEncodedLength(
  _In_ DWORD dwMsgEncodingType,
  _In_ DWORD dwFlags,
  _In_ DWORD dwMsgType,
  _In_ void const *pvMsgEncodeInfo,
  _In_opt_ LPSTR pszInnerContentObjID,
  _In_ DWORD cbData);

BOOL WINAPI CryptMsgClose(_In_opt_ HCRYPTMSG hCryptMsg);

BOOL
WINAPI
CryptMsgControl(
  _In_ HCRYPTMSG hCryptMsg,
  _In_ DWORD dwFlags,
  _In_ DWORD dwCtrlType,
  _In_opt_ void const *pvCtrlPara);

BOOL
WINAPI
CryptMsgCountersign(
  _In_ HCRYPTMSG hCryptMsg,
  _In_ DWORD dwIndex,
  _In_ DWORD cCountersigners,
  _In_reads_(cCountersigners) PCMSG_SIGNER_ENCODE_INFO rgCountersigners);

BOOL
WINAPI
CryptMsgCountersignEncoded(
  _In_ DWORD dwEncodingType,
  _In_reads_bytes_(cbSignerInfo) PBYTE pbSignerInfo,
  _In_ DWORD cbSignerInfo,
  _In_ DWORD cCountersigners,
  _In_reads_(cCountersigners) PCMSG_SIGNER_ENCODE_INFO rgCountersigners,
  _Out_writes_bytes_to_opt_(*pcbCountersignature, *pcbCountersignature) PBYTE pbCountersignature,
  _Inout_ PDWORD pcbCountersignature);

HCRYPTMSG WINAPI CryptMsgDuplicate(_In_opt_ HCRYPTMSG hCryptMsg);

BOOL
WINAPI
CryptMsgEncodeAndSignCTL(
  _In_ DWORD dwMsgEncodingType,
  _In_ PCTL_INFO pCtlInfo,
  _In_ PCMSG_SIGNED_ENCODE_INFO pSignInfo,
  _In_ DWORD dwFlags,
  _Out_writes_bytes_to_opt_(*pcbEncoded, *pcbEncoded) BYTE *pbEncoded,
  _Inout_ DWORD *pcbEncoded);

_Success_(return == 0)
BOOL
WINAPI
CryptMsgGetAndVerifySigner(
  _In_ HCRYPTMSG hCryptMsg,
  _In_ DWORD cSignerStore,
  _In_reads_opt_(cSignerStore) HCERTSTORE *rghSignerStore,
  _In_ DWORD dwFlags,
  _Outptr_opt_ PCCERT_CONTEXT *ppSigner,
  _Inout_opt_ DWORD *pdwSignerIndex);

BOOL
WINAPI
CryptMsgGetParam(
  _In_ HCRYPTMSG hCryptMsg,
  _In_ DWORD dwParamType,
  _In_ DWORD dwIndex,
  _Out_writes_bytes_to_opt_(*pcbData, *pcbData) void *pvData,
  _Inout_ DWORD *pcbData);

HCRYPTMSG
WINAPI
CryptMsgOpenToDecode(
  _In_ DWORD dwMsgEncodingType,
  _In_ DWORD dwFlags,
  _In_ DWORD dwMsgType,
  _In_opt_ HCRYPTPROV_LEGACY hCryptProv,
  _Reserved_ PCERT_INFO pRecipientInfo,
  _In_opt_ PCMSG_STREAM_INFO pStreamInfo);

HCRYPTMSG
WINAPI
CryptMsgOpenToEncode(
  _In_ DWORD dwMsgEncodingType,
  _In_ DWORD dwFlags,
  _In_ DWORD dwMsgType,
  _In_ void const *pvMsgEncodeInfo,
  _In_opt_ LPSTR pszInnerContentObjID,
  _In_opt_ PCMSG_STREAM_INFO pStreamInfo);

BOOL
WINAPI
CryptMsgSignCTL(
  _In_ DWORD dwMsgEncodingType,
  _In_reads_bytes_(cbCtlContent) BYTE *pbCtlContent,
  _In_ DWORD cbCtlContent,
  _In_ PCMSG_SIGNED_ENCODE_INFO pSignInfo,
  _In_ DWORD dwFlags,
  _Out_writes_bytes_to_opt_(*pcbEncoded, *pcbEncoded) BYTE *pbEncoded,
  _Inout_ DWORD *pcbEncoded);

BOOL
WINAPI
CryptMsgUpdate(
  _In_ HCRYPTMSG hCryptMsg,
  _In_reads_bytes_opt_(cbData) const BYTE *pbData,
  _In_ DWORD cbData,
  _In_ BOOL fFinal);

BOOL
WINAPI
CryptMsgVerifyCountersignatureEncoded(
  _In_opt_ HCRYPTPROV_LEGACY hCryptProv,
  _In_ DWORD dwEncodingType,
  _In_reads_bytes_(cbSignerInfo) PBYTE pbSignerInfo,
  _In_ DWORD cbSignerInfo,
  _In_reads_bytes_(cbSignerInfoCountersignature) PBYTE pbSignerInfoCountersignature,
  _In_ DWORD cbSignerInfoCountersignature,
  _In_ PCERT_INFO pciCountersigner);

BOOL
WINAPI
CryptMsgVerifyCountersignatureEncodedEx(
  _In_opt_ HCRYPTPROV_LEGACY hCryptProv,
  _In_ DWORD dwEncodingType,
  _In_reads_bytes_(cbSignerInfo) PBYTE pbSignerInfo,
  _In_ DWORD cbSignerInfo,
  _In_reads_bytes_(cbSignerInfoCountersignature) PBYTE pbSignerInfoCountersignature,
  _In_ DWORD cbSignerInfoCountersignature,
  _In_ DWORD dwSignerType,
  _In_ void *pvSigner,
  _In_ DWORD dwFlags,
  _Inout_opt_ void *pvExtra);

BOOL
WINAPI
CryptSignMessage(
  _In_ PCRYPT_SIGN_MESSAGE_PARA pSignPara,
  _In_ BOOL fDetachedSignature,
  _In_ DWORD cToBeSigned,
  _In_reads_opt_(cToBeSigned) const BYTE *rgpbToBeSigned[],
  _In_reads_(cToBeSigned) DWORD rgcbToBeSigned[],
  _Out_writes_bytes_to_opt_(*pcbSignedBlob, *pcbSignedBlob) BYTE *pbSignedBlob,
  _Inout_ DWORD *pcbSignedBlob);

BOOL
WINAPI
CryptSignMessageWithKey(
  _In_ PCRYPT_KEY_SIGN_MESSAGE_PARA pSignPara,
  _In_reads_bytes_(cbToBeSigned) const BYTE *pbToBeSigned,
  _In_ DWORD cbToBeSigned,
  _Out_writes_bytes_to_opt_(*pcbSignedBlob, *pcbSignedBlob) BYTE *pbSignedBlob,
  _Inout_ DWORD *pcbSignedBlob);

BOOL
WINAPI
CryptVerifyMessageSignature(
  _In_ PCRYPT_VERIFY_MESSAGE_PARA pVerifyPara,
  _In_ DWORD dwSignerIndex,
  _In_reads_bytes_(cbSignedBlob) const BYTE *pbSignedBlob,
  _In_ DWORD cbSignedBlob,
  _Out_writes_bytes_to_opt_(*pcbDecoded, *pcbDecoded) BYTE *pbDecoded,
  _Inout_opt_ DWORD *pcbDecoded,
  _Outptr_opt_result_maybenull_ PCCERT_CONTEXT *ppSignerCert);

BOOL
WINAPI
CryptVerifyMessageSignatureWithKey(
  _In_ PCRYPT_KEY_VERIFY_MESSAGE_PARA pVerifyPara,
  _In_opt_ PCERT_PUBLIC_KEY_INFO pPublicKeyInfo,
  _In_reads_bytes_(cbSignedBlob) const BYTE *pbSignedBlob,
  _In_ DWORD cbSignedBlob,
  _Out_writes_bytes_to_opt_(*pcbDecoded, *pcbDecoded) BYTE *pbDecoded,
  _Inout_opt_ DWORD *pcbDecoded);

BOOL
WINAPI
CryptVerifyDetachedMessageSignature(
  _In_ PCRYPT_VERIFY_MESSAGE_PARA pVerifyPara,
  _In_ DWORD dwSignerIndex,
  _In_reads_bytes_(cbDetachedSignBlob) const BYTE *pbDetachedSignBlob,
  _In_ DWORD cbDetachedSignBlob,
  _In_ DWORD cToBeSigned,
  _In_reads_(cToBeSigned) const BYTE *rgpbToBeSigned[],
  _In_reads_(cToBeSigned) DWORD rgcbToBeSigned[],
  _Outptr_opt_result_maybenull_ PCCERT_CONTEXT *ppSignerCert);

LONG
WINAPI
CryptGetMessageSignerCount(
  _In_ DWORD dwMsgEncodingType,
  _In_reads_bytes_(cbSignedBlob) const BYTE *pbSignedBlob,
  _In_ DWORD cbSignedBlob);

BOOL
WINAPI
CryptEncryptMessage(
  _In_ PCRYPT_ENCRYPT_MESSAGE_PARA pEncryptPara,
  _In_ DWORD cRecipientCert,
  _In_reads_(cRecipientCert) PCCERT_CONTEXT rgpRecipientCert[],
  _In_reads_bytes_opt_(cbToBeEncrypted) const BYTE *pbToBeEncrypted,
  _In_ DWORD cbToBeEncrypted,
  _Out_writes_bytes_to_opt_(*pcbEncryptedBlob, *pcbEncryptedBlob) BYTE *pbEncryptedBlob,
  _Inout_ DWORD *pcbEncryptedBlob);

BOOL
WINAPI
CryptDecryptMessage(
  _In_ PCRYPT_DECRYPT_MESSAGE_PARA pDecryptPara,
  _In_reads_bytes_(cbEncryptedBlob) const BYTE *pbEncryptedBlob,
  _In_ DWORD cbEncryptedBlob,
  _Out_writes_bytes_to_opt_(*pcbDecrypted, *pcbDecrypted) BYTE *pbDecrypted,
  _Inout_opt_ DWORD *pcbDecrypted,
  _Outptr_opt_result_maybenull_ PCCERT_CONTEXT *ppXchgCert);

BOOL
WINAPI
CryptSignAndEncryptMessage(
  _In_ PCRYPT_SIGN_MESSAGE_PARA pSignPara,
  _In_ PCRYPT_ENCRYPT_MESSAGE_PARA pEncryptPara,
  _In_ DWORD cRecipientCert,
  _In_reads_(cRecipientCert) PCCERT_CONTEXT rgpRecipientCert[],
  _In_reads_bytes_(cbToBeSignedAndEncrypted) const BYTE *pbToBeSignedAndEncrypted,
  _In_ DWORD cbToBeSignedAndEncrypted,
  _Out_writes_bytes_to_opt_(*pcbSignedAndEncryptedBlob, *pcbSignedAndEncryptedBlob) BYTE *pbSignedAndEncryptedBlob,
  _Inout_ DWORD *pcbSignedAndEncryptedBlob);

BOOL
WINAPI
CryptDecryptAndVerifyMessageSignature(
  _In_ PCRYPT_DECRYPT_MESSAGE_PARA pDecryptPara,
  _In_ PCRYPT_VERIFY_MESSAGE_PARA pVerifyPara,
  _In_ DWORD dwSignerIndex,
  _In_reads_bytes_(cbEncryptedBlob) const BYTE *pbEncryptedBlob,
  _In_ DWORD cbEncryptedBlob,
  _Out_writes_bytes_to_opt_(*pcbDecrypted, *pcbDecrypted) BYTE *pbDecrypted,
  _Inout_opt_ DWORD *pcbDecrypted,
  _Outptr_opt_result_maybenull_ PCCERT_CONTEXT *ppXchgCert,
  _Outptr_opt_result_maybenull_ PCCERT_CONTEXT *ppSignerCert);

HCERTSTORE
WINAPI
CryptGetMessageCertificates(
  _In_ DWORD dwMsgAndCertEncodingType,
  _In_opt_ HCRYPTPROV_LEGACY hCryptProv,
  _In_ DWORD dwFlags,
  _In_reads_bytes_(cbSignedBlob) const BYTE *pbSignedBlob,
  _In_ DWORD cbSignedBlob);

BOOL
WINAPI
CryptDecodeMessage(
  _In_ DWORD dwMsgTypeFlags,
  _In_opt_ PCRYPT_DECRYPT_MESSAGE_PARA pDecryptPara,
  _In_opt_ PCRYPT_VERIFY_MESSAGE_PARA pVerifyPara,
  _In_ DWORD dwSignerIndex,
  _In_reads_bytes_(cbEncodedBlob) const BYTE *pbEncodedBlob,
  _In_ DWORD cbEncodedBlob,
  _In_ DWORD dwPrevInnerContentType,
  _Out_opt_ DWORD *pdwMsgType,
  _Out_opt_ DWORD *pdwInnerContentType,
  _Out_writes_bytes_to_opt_(*pcbDecoded, *pcbDecoded) BYTE *pbDecoded,
  _Inout_opt_ DWORD *pcbDecoded,
  _Outptr_opt_result_maybenull_ PCCERT_CONTEXT *ppXchgCert,
  _Outptr_opt_result_maybenull_ PCCERT_CONTEXT *ppSignerCert);

BOOL
WINAPI
CryptHashMessage(
  _In_ PCRYPT_HASH_MESSAGE_PARA pHashPara,
  _In_ BOOL fDetachedHash,
  _In_ DWORD cToBeHashed,
  _In_reads_(cToBeHashed) const BYTE *rgpbToBeHashed[],
  _In_reads_(cToBeHashed) DWORD rgcbToBeHashed[],
  _Out_writes_bytes_to_opt_(*pcbHashedBlob, *pcbHashedBlob) BYTE *pbHashedBlob,
  _Inout_opt_ DWORD *pcbHashedBlob,
  _Out_writes_bytes_to_opt_(*pcbComputedHash, *pcbComputedHash) BYTE *pbComputedHash,
  _Inout_opt_ DWORD *pcbComputedHash);

BOOL
WINAPI
CryptVerifyMessageHash(
  _In_ PCRYPT_HASH_MESSAGE_PARA pHashPara,
  _In_reads_bytes_(cbHashedBlob) BYTE *pbHashedBlob,
  _In_ DWORD cbHashedBlob,
  _Out_writes_bytes_to_opt_(*pcbToBeHashed, *pcbToBeHashed) BYTE *pbToBeHashed,
  _Inout_opt_ DWORD *pcbToBeHashed,
  _Out_writes_bytes_to_opt_(*pcbComputedHash, *pcbComputedHash) BYTE *pbComputedHash,
  _Inout_opt_ DWORD *pcbComputedHash);

BOOL
WINAPI
CryptVerifyDetachedMessageHash(
  _In_ PCRYPT_HASH_MESSAGE_PARA pHashPara,
  _In_reads_bytes_(cbDetachedHashBlob) BYTE *pbDetachedHashBlob,
  _In_ DWORD cbDetachedHashBlob,
  _In_ DWORD cToBeHashed,
  _In_reads_(cToBeHashed) const BYTE *rgpbToBeHashed[],
  _In_reads_(cToBeHashed) DWORD rgcbToBeHashed[],
  _Out_writes_bytes_to_opt_(*pcbComputedHash, *pcbComputedHash) BYTE *pbComputedHash,
  _Inout_opt_ DWORD *pcbComputedHash);

/* PFX functions */
HCERTSTORE
WINAPI
PFXImportCertStore(
  _In_ CRYPT_DATA_BLOB *pPFX,
  _In_ LPCWSTR szPassword,
  _In_ DWORD dwFlags);

BOOL WINAPI PFXIsPFXBlob(_In_ CRYPT_DATA_BLOB *pPFX);

BOOL
WINAPI
PFXVerifyPassword(
  _In_ CRYPT_DATA_BLOB *pPFX,
  _In_ LPCWSTR szPassword,
  _In_ DWORD dwFlags);

BOOL
WINAPI
PFXExportCertStoreEx(
  _In_ HCERTSTORE hStore,
  _Inout_ CRYPT_DATA_BLOB* pPFX,
  _In_ LPCWSTR szPassword,
  _In_ void* pvPara,
  _In_ DWORD dwFlags);

BOOL
WINAPI
PFXExportCertStore(
  _In_ HCERTSTORE hStore,
  _Inout_ CRYPT_DATA_BLOB* pPFX,
  _In_ LPCWSTR szPassword,
  _In_ DWORD dwFlags);

BOOL WINAPI PFXVerifyPassword(CRYPT_DATA_BLOB *pPFX, LPCWSTR szPassword,
 DWORD dwFlags);

/* cryptnet.dll functions */
BOOL WINAPI CryptCancelAsyncRetrieval(_In_opt_ HCRYPTASYNC hAsyncRetrieval);

BOOL
WINAPI
CryptGetObjectUrl(
  _In_ LPCSTR pszUrlOid,
  _In_ LPVOID pvPara,
  _In_ DWORD dwFlags,
  _Out_writes_bytes_to_opt_(*pcbUrlArray, *pcbUrlArray) PCRYPT_URL_ARRAY pUrlArray,
  _Inout_ DWORD* pcbUrlArray,
  _Out_writes_bytes_to_opt_(*pcbUrlInfo, *pcbUrlInfo) PCRYPT_URL_INFO pUrlInfo,
  _Inout_opt_ DWORD* pcbUrlInfo,
  _Reserved_ LPVOID pvReserved);

_Success_(return != 0)
BOOL
WINAPI
CryptGetTimeValidObject(
  _In_ LPCSTR pszTimeValidOid,
  _In_ void *pvPara,
  _In_ PCCERT_CONTEXT pIssuer,
  _In_opt_ LPFILETIME pftValidFor,
  _In_ DWORD dwFlags,
  _In_ DWORD dwTimeout,
  _Outptr_opt_ void **ppvObject,
  _In_opt_ PCRYPT_CREDENTIALS pCredentials,
  _Inout_opt_ void *pvReserved);

BOOL
WINAPI
CryptFlushTimeValidObject(
  _In_ LPCSTR pszFlushTimeValidOid,
  _In_ void *pvPara,
  _In_ PCCERT_CONTEXT pIssuer,
  _In_ DWORD dwFlags,
  _Reserved_ void *pvReserved);

BOOL
WINAPI
CryptInstallCancelRetrieval(
  __callback PFN_CRYPT_CANCEL_RETRIEVAL pfnCancel,
  _In_opt_ const void *pvArg,
  _In_ DWORD dwFlags,
  _Reserved_ void *pvReserved);

BOOL
WINAPI
CryptUninstallCancelRetrieval(
  _In_ DWORD dwFlags,
  _Reserved_ void *pvReserved);

_Success_(return != 0)
BOOL
WINAPI
CryptRetrieveObjectByUrlA(
  _In_ LPCSTR pszUrl,
  _In_opt_ LPCSTR pszObjectOid,
  _In_ DWORD dwRetrievalFlags,
  _In_ DWORD dwTimeout,
  _Outptr_ LPVOID* ppvObject,
  _In_opt_ HCRYPTASYNC hAsyncRetrieve,
  _In_opt_ PCRYPT_CREDENTIALS pCredentials,
  _In_opt_ LPVOID pvVerify,
  _Inout_opt_ PCRYPT_RETRIEVE_AUX_INFO pAuxInfo);

_Success_(return != 0)
BOOL
WINAPI
CryptRetrieveObjectByUrlW(
  _In_ LPCWSTR pszUrl,
  _In_opt_ LPCSTR pszObjectOid,
  _In_ DWORD dwRetrievalFlags,
  _In_ DWORD dwTimeout,
  _Outptr_ LPVOID* ppvObject,
  _In_opt_ HCRYPTASYNC hAsyncRetrieve,
  _In_opt_ PCRYPT_CREDENTIALS pCredentials,
  _In_opt_ LPVOID pvVerify,
  _Inout_opt_ PCRYPT_RETRIEVE_AUX_INFO pAuxInfo);

#define CryptRetrieveObjectByUrl WINELIB_NAME_AW(CryptRetrieveObjectByUrl)

/* Not found in crypt32.dll but in softpub.dll */
HRESULT
WINAPI
FindCertsByIssuer(
  _Out_writes_bytes_to_opt_(*pcbCertChains, *pcbCertChains) PCERT_CHAIN pCertChains,
  _Inout_ DWORD *pcbCertChains,
  _Out_ DWORD *pcCertChains,
  _In_reads_bytes_opt_(cbEncodedIssuerName) BYTE* pbEncodedIssuerName,
  _In_ DWORD cbEncodedIssuerName,
  _In_opt_ LPCWSTR pwszPurpose,
  _In_ DWORD dwKeySpec);

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif
