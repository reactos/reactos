#ifndef _WINCRYPT_H
#define _WINCRYPT_H
#if __GNUC__ >= 3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif
#define MS_DEF_PROV_A	"Microsoft Base Cryptographic Provider v1.0"
#define MS_DEF_PROV_W	L"Microsoft Base Cryptographic Provider v1.0"
#define MS_ENHANCED_PROV_A	"Microsoft Enhanced Cryptographic Provider v1.0"
#define MS_ENHANCED_PROV_W	L"Microsoft Enhanced Cryptographic Provider v1.0"
#define MS_STRONG_PROV_A	"Microsoft Strong Cryptographic Provider"
#define MS_STRONG_PROV_W	L"Microsoft Strong Cryptographic Provider"
#define MS_DEF_RSA_SIG_PROV_A	"Microsoft RSA Signature Cryptographic Provider"
#define MS_DEF_RSA_SIG_PROV_W	L"Microsoft RSA Signature Cryptographic Provider"
#define MS_DEF_RSA_SCHANNEL_PROV_A	"Microsoft RSA SChannel Cryptographic Provider"
#define MS_DEF_RSA_SCHANNEL_PROV_W	L"Microsoft RSA SChannel Cryptographic Provider"
#define MS_DEF_DSS_PROV_A	"Microsoft Base DSS Cryptographic Provider"
#define MS_DEF_DSS_PROV_W	L"Microsoft Base DSS Cryptographic Provider"
#define MS_DEF_DSS_DH_PROV_A	"Microsoft Base DSS and Diffie-Hellman Cryptographic Provider"
#define MS_DEF_DSS_DH_PROV_W	L"Microsoft Base DSS and Diffie-Hellman Cryptographic Provider"
#define MS_ENH_DSS_DH_PROV_A	"Microsoft Enhanced DSS and Diffie-Hellman Cryptographic Provider"
#define MS_ENH_DSS_DH_PROV_W	L"Microsoft Enhanced DSS and Diffie-Hellman Cryptographic Provider"
#define MS_DEF_DH_SCHANNEL_PROV_A	"Microsoft DH SChannel Cryptographic Provider"
#define MS_DEF_DH_SCHANNEL_PROV_W	L"Microsoft DH SChannel Cryptographic Provider"
#define MS_SCARD_PROV_A	"Microsoft Base Smart Card Crypto Provider"
#define MS_SCARD_PROV_W	L"Microsoft Base Smart Card Crypto Provider"
#define GET_ALG_CLASS(x) (x&57344)
#define GET_ALG_TYPE(x) (x&7680)
#define GET_ALG_SID(x) (x&511)
#define ALG_CLASS_ANY 0
#define ALG_CLASS_SIGNATURE  8192
#define ALG_CLASS_MSG_ENCRYPT  16384
#define ALG_CLASS_DATA_ENCRYPT  24576
#define ALG_CLASS_HASH  32768
#define ALG_CLASS_KEY_EXCHANGE  40960
#define ALG_CLASS_ALL 57344 /* (7 << 13) */
#define ALG_TYPE_ANY 0
#define ALG_TYPE_DSS 512
#define ALG_TYPE_RSA 1024
#define ALG_TYPE_BLOCK 1536
#define ALG_TYPE_STREAM  2048
#define ALG_TYPE_DH 2560 /* (5 << 9) */
#define ALG_TYPE_SECURECHANNEL 3072 /* (6 << 9) */
#define ALG_SID_ANY 0
#define ALG_SID_RSA_ANY 0
#define ALG_SID_RSA_PKCS 1
#define ALG_SID_RSA_MSATWORK 2
#define ALG_SID_RSA_ENTRUST 3
#define ALG_SID_RSA_PGP 4
#define ALG_SID_DSS_ANY 0
#define ALG_SID_DSS_PKCS 1
#define ALG_SID_DSS_DMS 2
#define ALG_SID_DES 1
#define ALG_SID_3DES 3
#define ALG_SID_DESX 4
#define ALG_SID_IDEA 5
#define ALG_SID_CAST 6
#define ALG_SID_SAFERSK64 7
#define ALG_SID_SAFERSK128 8
#define ALG_SID_3DES_112 9
#define ALG_SID_SKIPJACK 10
#define ALG_SID_TEK 11
#define ALG_SID_CYLINK_MEK 12
#define ALG_SID_RC5 13
#define ALG_SID_RC2 2
#define ALG_SID_RC4 1
#define ALG_SID_SEAL 2
#define ALG_SID_MD2 1
#define ALG_SID_MD4 2
#define ALG_SID_MD5 3
#define ALG_SID_SHA 4
#define ALG_SID_MAC 5
#define ALG_SID_RIPEMD 6
#define ALG_SID_RIPEMD160 7
#define ALG_SID_SSL3SHAMD5 8
#define ALG_SID_HMAC 9
#define ALG_SID_TLS1PRF 10
#define ALG_SID_EXAMPLE 80

/* some typedefs for function parameters */
typedef unsigned int ALG_ID;
typedef unsigned long HCRYPTPROV;
typedef unsigned long HCRYPTKEY;
typedef unsigned long HCRYPTHASH;
typedef void *HCERTSTORE;
typedef void *HCRYPTMSG;
typedef void *HCERTSTOREPROV;
typedef void *HCRYPTOIDFUNCSET;
typedef void *HCRYPTOIDFUNCADDR;

#define CALG_MD2 (ALG_CLASS_HASH|ALG_TYPE_ANY|ALG_SID_MD2)
#define CALG_MD4 (ALG_CLASS_HASH|ALG_TYPE_ANY|ALG_SID_MD4)
#define CALG_MD5 (ALG_CLASS_HASH|ALG_TYPE_ANY|ALG_SID_MD5)
#define CALG_SHA (ALG_CLASS_HASH|ALG_TYPE_ANY|ALG_SID_SHA)
#define CALG_MAC (ALG_CLASS_HASH|ALG_TYPE_ANY|ALG_SID_MAC)
#define CALG_3DES (ALG_CLASS_DATA_ENCRYPT|ALG_TYPE_BLOCK|3)
#define CALG_CYLINK_MEK (ALG_CLASS_DATA_ENCRYPT|ALG_TYPE_BLOCK|12)
#define CALG_SKIPJACK (ALG_CLASS_DATA_ENCRYPT|ALG_TYPE_BLOCK|10)
#define CALG_KEA_KEYX (ALG_CLASS_KEY_EXCHANGE|ALG_TYPE_STREAM|ALG_TYPE_DSS|4)
#define CALG_RSA_SIGN (ALG_CLASS_SIGNATURE|ALG_TYPE_RSA|ALG_SID_RSA_ANY)
#define CALG_DSS_SIGN (ALG_CLASS_SIGNATURE|ALG_TYPE_DSS|ALG_SID_DSS_ANY)
#define CALG_RSA_KEYX (ALG_CLASS_KEY_EXCHANGE|ALG_TYPE_RSA|ALG_SID_RSA_ANY)
#define CALG_DES (ALG_CLASS_DATA_ENCRYPT|ALG_TYPE_BLOCK|ALG_SID_DES)
#define CALG_RC2 (ALG_CLASS_DATA_ENCRYPT|ALG_TYPE_BLOCK|ALG_SID_RC2)
#define CALG_RC4 (ALG_CLASS_DATA_ENCRYPT|ALG_TYPE_STREAM|ALG_SID_RC4)
#define CALG_SEAL (ALG_CLASS_DATA_ENCRYPT|ALG_TYPE_STREAM|ALG_SID_SEAL)
#define CALG_DH_EPHEM (ALG_CLASS_KEY_EXCHANGE|ALG_TYPE_STREAM|ALG_TYPE_DSS|ALG_SID_DSS_DMS)
#define CALG_DESX (ALG_CLASS_DATA_ENCRYPT|ALG_TYPE_BLOCK|ALG_SID_DESX)
#define CALG_TLS1PRF (ALG_CLASS_DHASH|ALG_TYPE_ANY|ALG_SID_TLS1PRF)

typedef struct _CERT_PRIVATE_KEY_VALIDITY {
    FILETIME NotBefore;
    FILETIME NotAfter;
} CERT_PRIVATE_KEY_VALIDITY, *PCERT_PRIVATE_KEY_VALIDITY;


/* access state flags */
#define CERT_ACCESS_STATE_WRITE_PERSIST_FLAG   0x1
#define CERT_ACCESS_STATE_SYSTEM_STORE_FLAG    0x2
#define CERT_ACCESS_STATE_LM_SYSTEM_STORE_FLAG 0x4

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

/* dwAddDisposition */
#define CERT_STORE_ADD_NEW                                 1
#define CERT_STORE_ADD_USE_EXISTING                        2
#define CERT_STORE_ADD_REPLACE_EXISTING                    3
#define CERT_STORE_ADD_ALWAYS                              4
#define CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES 5
#define CERT_STORE_ADD_NEWER                               6
#define CERT_STORE_ADD_NEWER_INHERIT_PROPERTIES            7

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

#define CRYPT_VERIFYCONTEXT 0xF0000000
#define CRYPT_NEWKEYSET 8
#define CRYPT_DELETEKEYSET 16
#define CRYPT_MACHINE_KEYSET 32
#define CRYPT_SILENT 64
#define CRYPT_EXPORTABLE 1
#define CRYPT_USER_PROTECTED 2
#define CRYPT_CREATE_SALT 4
#define CRYPT_UPDATE_KEY 8
#define SIMPLEBLOB 1
#define PUBLICKEYBLOB 6
#define PRIVATEKEYBLOB 7
#define PLAINTEXTKEYBLOB 8
#define OPAQUEKEYBLOB 9
#define PUBLICKEYBLOBEX 10
#define SYMMETRICWRAPKEYBLOB 11
#define AT_KEYEXCHANGE 1
#define AT_SIGNATURE 2
#define CRYPT_USERDATA 1
#define PKCS5_PADDING 1
#define CRYPT_MODE_CBC 1
#define CRYPT_MODE_ECB 2
#define CRYPT_MODE_OFB 3
#define CRYPT_MODE_CFB 4
#define CRYPT_MODE_CTS 5
#define CRYPT_MODE_CBCI 6
#define CRYPT_MODE_CFBP 7
#define CRYPT_MODE_OFBP 8
#define CRYPT_MODE_CBCOFM 9
#define CRYPT_MODE_CBCOFMI 10
#define CRYPT_ENCRYPT 1
#define CRYPT_DECRYPT 2
#define CRYPT_EXPORT 4
#define CRYPT_READ 8
#define CRYPT_WRITE 16
#define CRYPT_MAC 32
/* Crypt{Get|Set}Provider */
#define CRYPT_MACHINE_DEFAULT     0x00000001
#define CRYPT_USER_DEFAULT        0x00000002
#define CRYPT_DELETE_DEFAULT      0x00000004

/* cert system store flags */
#define CERT_SYSTEM_STORE_MASK 0xffff0000
#define CERT_SYSTEM_STORE_RELOCATE_FLAG 0x80000000

/* CertFindChainInStore dwFindType types */
#define CERT_CHAIN_FIND_BY_ISSUER 1

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
#define CERT_FIRST_RESERVED_PROP_ID                68
#define CERT_LAST_RESERVED_PROP_ID                 0x00007fff
#define CERT_FIRST_USER_PROP_ID                    0x00008000
#define CERT_LAST_USER_PROP_ID                     0x0000ffff

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

/* DES SIDs */
#define ALG_SID_DES                     1
#define ALG_SID_3DES                    3
#define ALG_SID_DESX                    4
#define ALG_SID_IDEA                    5
#define ALG_SID_CAST                    6
#define ALG_SID_SAFERSK64               7
#define ALG_SID_SAFERSK128              8
#define ALG_SID_3DES_112                9
/* Diffie-Hellmans SIDs */
#define ALG_SID_DH_SANDF                1
#define ALG_SID_DH_EPHEM                2
#define ALG_SID_AGREED_KEY_ANY          3
#define ALG_SID_KEA                     4
/* RC2 SIDs */
#define ALG_SID_RC4                     1
#define ALG_SID_RC2                     2
#define ALG_SID_SEAL                    2
/* Hash SIDs */
#define ALG_SID_MD2                     1
#define ALG_SID_MD4                     2
#define ALG_SID_MD5                     3
#define ALG_SID_SHA                     4
#define ALG_SID_SHA1                    CALG_SHA
#define ALG_SID_MAC                     5
#define ALG_SID_RIPEMD                  6
#define ALG_SID_RIPEMD160               7
#define ALG_SID_SSL3SHAMD5              8
#define ALG_SID_HMAC                    9
#define ALG_SID_TLS1PRF                10
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
#define CALG_RSA_SIGN             (ALG_CLASS_SIGNATURE    | ALG_TYPE_RSA           | ALG_SID_RSA_ANY)
#define CALG_DSS_SIGN             (ALG_CLASS_SIGNATURE    | ALG_TYPE_DSS           | ALG_SID_DSS_ANY)
#define CALG_DH_SF                (ALG_CLASS_KEY_EXCHANGE | ALG_TYPE_DH            | ALG_SID_DH_SANDF)
#define CALG_DH_EPHEM             (ALG_CLASS_KEY_EXCHANGE | ALG_TYPE_DH            | ALG_SID_DH_EPHEM)
#define CALG_RSA_KEYX             (ALG_CLASS_KEY_EXCHANGE | ALG_TYPE_RSA           | ALG_SID_RSA_ANY)
#define CALG_DES                  (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK         | ALG_SID_DES)
#define CALG_RC2                  (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK         | ALG_SID_RC2)
#define CALG_3DES                 (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK         | ALG_SID_3DES)
#define CALG_3DES_112             (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK         | ALG_SID_3DES_112)
#define CALG_RC4                  (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_STREAM        | ALG_SID_RC4)
#define CALG_SEAL                 (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_STREAM        | ALG_SID_SEAL)
#define CALG_SSL3_MASTER          (ALG_CLASS_MSG_ENCRYPT  | ALG_TYPE_SECURECHANNEL | ALG_SID_SSL3_MASTER)
#define CALG_SCHANNEL_MASTER_HASH (ALG_CLASS_MSG_ENCRYPT  | ALG_TYPE_SECURECHANNEL | ALG_SID_SCHANNEL_MASTER_HASH)
#define CALG_SCHANNEL_MAC_KEY     (ALG_CLASS_MSG_ENCRYPT  | ALG_TYPE_SECURECHANNEL | ALG_SID_SCHANNEL_MAC_KEY)
#define CALG_SCHANNEL_ENC_KEY     (ALG_CLASS_MSG_ENCRYPT  | ALG_TYPE_SECURECHANNEL | ALG_SID_SCHANNEL_ENC_KEY)
#define CALG_PCT1_MASTER          (ALG_CLASS_MSG_ENCRYPT  | ALG_TYPE_SECURECHANNEL | ALG_SID_PCT1_MASTER)
#define CALG_SSL2_MASTER          (ALG_CLASS_MSG_ENCRYPT  | ALG_TYPE_SECURECHANNEL | ALG_SID_SSL2_MASTER)
#define CALG_TLS1_MASTER          (ALG_CLASS_MSG_ENCRYPT  | ALG_TYPE_SECURECHANNEL | ALG_SID_TLS1_MASTER)

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

/* access state flags */
#define CERT_ACCESS_STATE_WRITE_PERSIST_FLAG   0x1
#define CERT_ACCESS_STATE_SYSTEM_STORE_FLAG    0x2
#define CERT_ACCESS_STATE_LM_SYSTEM_STORE_FLAG 0x4

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
#define szOID_DOMAIN_COMPONENT              "0.9.2342.19200300.100.1.25"
#define szOID_PKCS_12_FRIENDLY_NAME_ATTR     "1.2.840.113549.1.9.20"
#define szOID_PKCS_12_LOCAL_KEY_ID           "1.2.840.113549.1.9.21"
#define szOID_CERT_EXTENSIONS                "1.3.6.1.4.1.311.2.1.14"
#define szOID_NEXT_UPDATE_LOCATION           "1.3.6.1.4.1.311.10.2"
#define szOID_REMOVE_CERTIFICATE             "1.3.6.1.4.1.311.10.8.1"
#define szOID_CROSS_CERT_DIST_POINTS         "1.3.6.1.4.1.311.10.9.1"
#define szOID_CTL                            "1.3.6.1.4.1.311.10.10.1"
#define szOID_SORTED_CTL                     "1.3.6.1.4.1.311.10.10.1.1"
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
#define szOID_CERT_MANIFOLD                  "1.3.6.1.4.1.311.20.3"
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
#define szOID_IPSEC_KP_IKE_INTERMEDIATE      "1.3.6.1.5.5.8.2.2"

#ifndef szOID_SERIALIZED
#define szOID_SERIALIZED                     "1.3.6.1.4.1.311.10.3.3.1"
#endif

#ifndef szOID_NT_PRINCIPAL_NAME
#define szOID_NT_PRINCIPAL_NAME              "1.3.6.1.4.1.311.20.2.3"
#endif

#ifndef szOID_CERTSRV_CA_VERSION
#define szOID_CERTSRV_CA_VERSION             "1.3.6.1.4.1.311.21.1"
#endif

#ifndef szOID_PRODUCT_UPDATE
#define szOID_PRODUCT_UPDATE                 "1.3.6.1.4.1.311.31.1"
#endif

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

#define CRYPT_GET_INSTALLED_OID_FUNC_FLAG  0x1

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

#define HP_ALGID 1
#define HP_HASHVAL 2
#define HP_HASHSIZE 4
#define CRYPT_FAILED FALSE
#define CRYPT_SUCCEED TRUE
#define RCRYPT_SUCCEEDED(r) ((r)==CRYPT_SUCCEED)
#define RCRYPT_FAILED(r) ((r)==CRYPT_FAILED)
#define PP_CLIENT_HWND 1
#define PP_ENUMALGS 1
#define PP_ENUMCONTAINERS 2
#define PP_IMPTYPE 3
#define PP_NAME 4
#define PP_VERSION 5
#define PP_CONTAINER 6
#define PP_CHANGE_PASSWORD	7
#define PP_KEYSET_SEC_DESCR	8
#define PP_CERTCHAIN	9
#define PP_KEY_TYPE_SUBTYPE	10
#define PP_PROVTYPE	16
#define PP_KEYSTORAGE	17
#define PP_APPLI_CERT	18
#define PP_SYM_KEYSIZE	19
#define PP_SESSION_KEYSIZE	20
#define PP_UI_PROMPT	21
#define PP_ENUMALGS_EX	22
#define PP_ENUMMANDROOTS 25
#define PP_ENUMELECTROOTS 26
#define PP_KEYSET_TYPE 27
#define PP_ADMIN_PIN 31
#define PP_KEYEXCHANGE_PIN 32
#define PP_SIGNATURE_PIN 33
#define PP_SIG_KEYSIZE_INC 34
#define PP_KEYX_KEYSIZE_INC 35
#define PP_UNIQUE_CONTAINER 36
#define PP_SGC_INFO 37
#define PP_USE_HARDWARE_RNG 38
#define PP_KEYSPEC 39
#define PP_ENUMEX_SIGNING_PROT 40
#define CRYPT_FIRST 1
#define CRYPT_NEXT 2
#define CRYPT_IMPL_HARDWARE 1
#define CRYPT_IMPL_SOFTWARE 2
#define CRYPT_IMPL_MIXED 3
#define CRYPT_IMPL_UNKNOWN 4
#define PROV_RSA_FULL 1
#define PROV_RSA_SIG 2
#define PROV_DSS 3
#define PROV_FORTEZZA 4
#define PROV_MS_MAIL 5
#define PROV_SSL 6
#define PROV_STT_MER 7
#define PROV_STT_ACQ 8
#define PROV_STT_BRND 9
#define PROV_STT_ROOT 10
#define PROV_STT_ISS 11
#define PROV_RSA_SCHANNEL 12
#define PROV_DSS_DH 13
#define PROV_EC_ECDSA_SIG 14
#define PROV_EC_ECNRA_SIG 15
#define PROV_EC_ECDSA_FULL 16
#define PROV_EC_ECNRA_FULL 17
#define PROV_DH_SCHANNEL 18
#define PROV_SPYRUS_LYNKS 20
#define PROV_RNG 21
#define PROV_INTEL_SEC 22
#define MAXUIDLEN 64
#define CUR_BLOB_VERSION 2
#define X509_ASN_ENCODING 1
#define PKCS_7_ASN_ENCODING  65536
#define CERT_V1 0
#define CERT_V2 1
#define CERT_V3 2
#define CERT_E_CHAINING (-2146762486)
#define CERT_E_CN_NO_MATCH (-2146762481)
#define CERT_E_EXPIRED (-2146762495)
#define CERT_E_PURPOSE (-2146762490)
#define CERT_E_REVOCATION_FAILURE (-2146762482)
#define CERT_E_REVOKED (-2146762484)
#define CERT_E_ROLE (-2146762493)
#define CERT_E_UNTRUSTEDROOT (-2146762487)
#define CERT_E_UNTRUSTEDTESTROOT (-2146762483)
#define CERT_E_VALIDITYPERIODNESTING (-2146762494)
#define CERT_E_WRONG_USAGE (-2146762480)
#define CERT_E_PATHLENCONST (-2146762492)
#define CERT_E_CRITICAL (-2146762491)
#define CERT_E_ISSUERCHAINING (-2146762489)
#define CERT_E_MALFORMED (-2146762488)
#define CRYPT_E_REVOCATION_OFFLINE (-2146885613)
#define CRYPT_E_REVOKED (-2146885616)
#define TRUST_E_BASIC_CONSTRAINTS (-2146869223)
#define TRUST_E_CERT_SIGNATURE (-2146869244)
#define TRUST_E_FAIL (-2146762485)
#define CERT_TRUST_NO_ERROR 0
#define CERT_TRUST_IS_NOT_TIME_VALID 1
#define CERT_TRUST_IS_NOT_TIME_NESTED 2
#define CERT_TRUST_IS_REVOKED 4
#define CERT_TRUST_IS_NOT_SIGNATURE_VALID 8
#define CERT_TRUST_IS_NOT_VALID_FOR_USAGE 16
#define CERT_TRUST_IS_UNTRUSTED_ROOT 32
#define CERT_TRUST_REVOCATION_STATUS_UNKNOWN 64
#define CERT_TRUST_IS_CYCLIC 128
#define CERT_TRUST_IS_PARTIAL_CHAIN 65536
#define CERT_TRUST_CTL_IS_NOT_TIME_VALID 131072
#define CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID 262144
#define CERT_TRUST_CTL_IS_NOT_VALID_FOR_USAGE 524288
#define CERT_TRUST_HAS_EXACT_MATCH_ISSUER 1
#define CERT_TRUST_HAS_KEY_MATCH_ISSUER 2
#define CERT_TRUST_HAS_NAME_MATCH_ISSUER 4
#define CERT_TRUST_IS_SELF_SIGNED 8
#define CERT_TRUST_IS_COMPLEX_CHAIN 65536
#define CERT_CHAIN_POLICY_BASE ((LPCSTR) 1)
#define CERT_CHAIN_POLICY_AUTHENTICODE  ((LPCSTR) 2)
#define CERT_CHAIN_POLICY_AUTHENTICODE_TS  ((LPCSTR) 3)
#define CERT_CHAIN_POLICY_SSL  ((LPCSTR) 4)
#define CERT_CHAIN_POLICY_BASIC_CONSTRAINTS ((LPCSTR) 5)
#define CERT_CHAIN_POLICY_NT_AUTH ((LPCSTR) 6)
#define USAGE_MATCH_TYPE_AND 0
#define USAGE_MATCH_TYPE_OR 1
#define CERT_SIMPLE_NAME_STR 1
#define CERT_OID_NAME_STR 2
#define CERT_X500_NAME_STR 3
#define CERT_NAME_STR_SEMICOLON_FLAG 1073741824
#define CERT_NAME_STR_CRLF_FLAG 134217728
#define CERT_NAME_STR_NO_PLUS_FLAG 536870912
#define CERT_NAME_STR_NO_QUOTING_FLAG 268435456
#define CERT_NAME_STR_REVERSE_FLAG 33554432
#define CERT_NAME_STR_ENABLE_T61_UNICODE_FLAG 131072
#define CERT_FIND_ANY 0
#define CERT_FIND_CERT_ID 1048576
#define CERT_FIND_CTL_USAGE 655360
#define CERT_FIND_ENHKEY_USAGE 655360
#define CERT_FIND_EXISTING 851968
#define CERT_FIND_HASH 65536
#define CERT_FIND_ISSUER_ATTR 196612
#define CERT_FIND_ISSUER_NAME 131076
#define CERT_FIND_ISSUER_OF 786432
#define CERT_FIND_KEY_IDENTIFIER 983040
#define CERT_FIND_KEY_SPEC 589824
#define CERT_FIND_MD5_HASH 262144
#define CERT_FIND_PROPERTY 327680
#define CERT_FIND_PUBLIC_KEY 393216
#define CERT_FIND_SHA1_HASH 65536
#define CERT_FIND_SIGNATURE_HASH 917504
#define CERT_FIND_SUBJECT_ATTR 196615
#define CERT_FIND_SUBJECT_CERT 720896
#define CERT_FIND_SUBJECT_NAME 131079
#define CERT_FIND_SUBJECT_STR_A 458759
#define CERT_FIND_SUBJECT_STR_W 524295
#define CERT_FIND_ISSUER_STR_A 458756
#define CERT_FIND_ISSUER_STR_W 524292
#define CERT_FIND_OR_ENHKEY_USAGE_FLAG 16
#define CERT_FIND_OPTIONAL_ENHKEY_USAGE_FLAG  1
#define CERT_FIND_NO_ENHKEY_USAGE_FLAG  8
#define CERT_FIND_VALID_ENHKEY_USAGE_FLAG  32
#define CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG  2
#define CERT_CASE_INSENSITIVE_IS_RDN_ATTRS_FLAG  2
#define CERT_UNICODE_IS_RDN_ATTRS_FLAG 1
#define CERT_CHAIN_FIND_BY_ISSUER 1
#define CERT_CHAIN_FIND_BY_ISSUER_COMPARE_KEY_FLAG 1
#define CERT_CHAIN_FIND_BY_ISSUER_COMPLEX_CHAIN_FLAG 2
#define CERT_CHAIN_FIND_BY_ISSUER_CACHE_ONLY_FLAG 32768
#define CERT_CHAIN_FIND_BY_ISSUER_CACHE_ONLY_URL_FLAG 4
#define CERT_CHAIN_FIND_BY_ISSUER_LOCAL_MACHINE_FLAG 8
#define CERT_CHAIN_FIND_BY_ISSUER_NO_KEY_FLAG 16384
#define CERT_STORE_PROV_SYSTEM 10
#define CERT_SYSTEM_STORE_LOCAL_MACHINE 131072
#define szOID_PKIX_KP_SERVER_AUTH "4235600"
#define szOID_SERVER_GATED_CRYPTO "4235658"
#define szOID_SGC_NETSCAPE "2.16.840.1.113730.4.1"
#define szOID_PKIX_KP_CLIENT_AUTH "1.3.6.1.5.5.7.3.2"
#define CRYPT_NOHASHOID 0x00000001
#define CRYPT_NO_SALT 0x10
#define CRYPT_PREGEN 0x40
#define CRYPT_RECIPIENT 0x10
#define CRYPT_INITIATOR 0x40
#define CRYPT_ONLINE 0x80
#define CRYPT_SF 0x100
#define CRYPT_CREATE_IV 0x200
#define CRYPT_KEK 0x400
#define CRYPT_DATA_KEY 0x800
#define CRYPT_VOLATILE 0x1000
#define CRYPT_SGCKEY 0x2000
#define KP_IV               0x00000001
#define KP_SALT             0x00000002
#define KP_PADDING          0x00000003
#define KP_MODE             0x00000004
#define KP_MODE_BITS        0x00000005
#define KP_PERMISSIONS      0x00000006
#define KP_ALGID            0x00000007
#define KP_BLOCKLEN         0x00000008
#define KP_KEYLEN           0x00000009
#define KP_SALT_EX          0x0000000a
#define KP_P                0x0000000b
#define KP_G                0x0000000c
#define KP_Q                0x0000000d
#define KP_X                0x0000000e
#define KP_Y                0x0000000f
#define KP_RA               0x00000010
#define KP_RB               0x00000011
#define KP_INFO             0x00000012
#define KP_EFFECTIVE_KEYLEN 0x00000013
#define KP_SCHANNEL_ALG     0x00000014
#define KP_PUB_PARAMS       0x00000027
#define CRYPT_FLAG_PCT1    0x0001
#define CRYPT_FLAG_SSL2    0x0002
#define CRYPT_FLAG_SSL3    0x0004
#define CRYPT_FLAG_TLS1    0x0008
#define CRYPT_FLAG_IPSEC   0x0010
#define CRYPT_FLAG_SIGNING 0x0020
#define SCHANNEL_MAC_KEY    0x00000000
#define SCHANNEL_ENC_KEY    0x00000001
#define INTERNATIONAL_USAGE 0x00000001

typedef UINT ALG_ID;
typedef struct _VTableProvStruc {
    DWORD    Version;
    FARPROC  pFuncVerifyImage;
    FARPROC  pFuncReturnhWnd;
    DWORD    dwProvType;
    BYTE    *pbContextInfo;
    DWORD    cbContextInfo;
    LPSTR    pszProvName;
} VTableProvStruc, *PVTableProvStruc;
typedef ULONG HCRYPTPROV;
typedef ULONG HCRYPTKEY;
typedef ULONG HCRYPTHASH;
typedef PVOID HCERTSTORE;
typedef PVOID HCRYPTMSG;
typedef PVOID HCERTCHAINENGINE;
typedef struct _CRYPTOAPI_BLOB {
	DWORD cbData;
	BYTE* pbData;
} CRYPT_INTEGER_BLOB, *PCRYPT_INTEGER_BLOB,
  CRYPT_UINT_BLOB,    *PCRYPT_UINT_BLOB,
  CRYPT_OBJID_BLOB,   *PCRYPT_OBJID_BLOB,
  CERT_NAME_BLOB,     *PCERT_NAME_BLOB,
  CERT_RDN_VALUE_BLOB,*PCERT_RDN_VALUE_BLOB,
  CERT_BLOB,          *PCERT_BLOB,
  CRL_BLOB,           *PCRL_BLOB,
  DATA_BLOB,          *PDATA_BLOB,
  CRYPT_DATA_BLOB,    *PCRYPT_DATA_BLOB,
  CRYPT_HASH_BLOB,    *PCRYPT_HASH_BLOB,
  CRYPT_DIGEST_BLOB,  *PCRYPT_DIGEST_BLOB,
  CRYPT_DER_BLOB,     *PCRYPT_DER_BLOB,
  CRYPT_ATTR_BLOB,    *PCRYPT_ATTR_BLOB;
/* not described in SDK; has the same layout as HTTPSPolicyCallbackData */
typedef struct _SSL_EXTRA_CERT_CHAIN_POLICY_PARA {
	DWORD cbStruct;
	DWORD dwAuthType;
	DWORD fdwChecks;
	LPWSTR pwszServerName;
} SSL_EXTRA_CERT_CHAIN_POLICY_PARA, *PSSL_EXTRA_CERT_CHAIN_POLICY_PARA,
  HTTPSPolicyCallbackData, *PHTTPSPolicyCallbackData;
/* #if (_WIN32_WINNT>=0x500) */
typedef struct _CERT_CHAIN_POLICY_PARA {
	DWORD cbSize;
	DWORD dwFlags;
	void* pvExtraPolicyPara;
} CERT_CHAIN_POLICY_PARA, *PCERT_CHAIN_POLICY_PARA;
typedef struct _CERT_CHAIN_POLICY_STATUS {
	DWORD cbSize;
	DWORD dwError;
	LONG lChainIndex;
	LONG lElementIndex;
	void* pvExtraPolicyStatus;
} CERT_CHAIN_POLICY_STATUS, *PCERT_CHAIN_POLICY_STATUS;
/* #endif */
typedef struct _CRYPT_ALGORITHM_IDENTIFIER {
	LPSTR pszObjId;
	CRYPT_OBJID_BLOB Parameters;
} CRYPT_ALGORITHM_IDENTIFIER, *PCRYPT_ALGORITHM_IDENTIFIER;
typedef struct _CRYPT_BIT_BLOB {
	DWORD cbData;
	BYTE* pbData;
	DWORD cUnusedBits;
} CRYPT_BIT_BLOB, *PCRYPT_BIT_BLOB;
typedef struct _CERT_PUBLIC_KEY_INFO {
	CRYPT_ALGORITHM_IDENTIFIER Algorithm;
	CRYPT_BIT_BLOB PublicKey;
} CERT_PUBLIC_KEY_INFO, *PCERT_PUBLIC_KEY_INFO;
typedef struct _CERT_EXTENSION {
	LPSTR pszObjId;
	BOOL fCritical;
	CRYPT_OBJID_BLOB Value;
} CERT_EXTENSION, *PCERT_EXTENSION;
typedef struct _CERT_INFO {
	DWORD dwVersion;
	CRYPT_INTEGER_BLOB SerialNumber;
	CRYPT_ALGORITHM_IDENTIFIER SignatureAlgorithm;
	CERT_NAME_BLOB Issuer;
	FILETIME NotBefore;
	FILETIME NotAfter;
	CERT_NAME_BLOB Subject;
	CERT_PUBLIC_KEY_INFO SubjectPublicKeyInfo;
	CRYPT_BIT_BLOB IssuerUniqueId;
	CRYPT_BIT_BLOB SubjectUniqueId;
	DWORD cExtension;
	PCERT_EXTENSION rgExtension;
} CERT_INFO, *PCERT_INFO;
typedef struct _CERT_CONTEXT {
	DWORD dwCertEncodingType;
	BYTE* pbCertEncoded;
	DWORD cbCertEncoded;
	PCERT_INFO pCertInfo;
	HCERTSTORE hCertStore;
} CERT_CONTEXT, *PCERT_CONTEXT;
typedef const CERT_CONTEXT *PCCERT_CONTEXT;
typedef struct _CTL_USAGE {
	DWORD cUsageIdentifier;
	LPSTR *rgpszUsageIdentifier;
} CTL_USAGE, *PCTRL_USAGE,
  CERT_ENHKEY_USAGE, *PCERT_ENHKEY_USAGE;
typedef struct _CERT_USAGE_MATCH {
	DWORD dwType;
	CERT_ENHKEY_USAGE Usage;
} CERT_USAGE_MATCH, *PCERT_USAGE_MATCH;
typedef struct _CRYPT_ATTRIBUTE_TYPE_VALUE {
  LPSTR               pszObjId;
  CRYPT_OBJID_BLOB    Value;
} CRYPT_ATTRIBUTE_TYPE_VALUE, *PCRYPT_ATTRIBUTE_TYPE_VALUE;
/* #if (_WIN32_WINNT>=0x500) */
typedef struct _CERT_CHAIN_PARA {
	DWORD cbSize;
	CERT_USAGE_MATCH RequestedUsage;
#if CERT_CHAIN_PARA_HAS_EXTRA_FIELDS
	CERT_USAGE_MATCH RequestedIssuancePolicy;
	DWORD dwUrlRetrievalTimeout;
	BOOL fCheckRevocationFreshnessTime;
	DWORD dwRevocationFreshnessTime;
#endif
} CERT_CHAIN_PARA, *PCERT_CHAIN_PARA;
typedef BOOL (WINAPI *PFN_CERT_CHAIN_FIND_BY_ISSUER_CALLBACK)(PCCERT_CONTEXT,void*);
typedef struct _CERT_CHAIN_FIND_BY_ISSUER_PARA {
	DWORD cbSize;
	LPCSTR pszUsageIdentifier;
	DWORD dwKeySpec;
	DWORD dwAcquirePrivateKeyFlags;
	DWORD cIssuer;
	CERT_NAME_BLOB* rgIssuer;
	PFN_CERT_CHAIN_FIND_BY_ISSUER_CALLBACK pfnFIndCallback;
	void* pvFindArg;
	DWORD* pdwIssuerChainIndex;
	DWORD* pdwIssuerElementIndex;
} CERT_CHAIN_FIND_BY_ISSUER_PARA, *PCERT_CHAIN_FIND_BY_ISSUER_PARA;
/* #endif */

typedef struct _CERT_POLICY_ID {
    DWORD  cCertPolicyElementId;
    LPSTR *rgbszCertPolicyElementId;
} CERT_POLICY_ID, *PCERT_POLICY_ID;

typedef struct _PUBLICKEYSTRUC {
    BYTE   bType;
    BYTE   bVersion;
    WORD   reserved;
    ALG_ID aiKeyAlg;
} BLOBHEADER, PUBLICKEYSTRUC;

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

/* Encode/decode object */
typedef LPVOID (WINAPI *PFN_CRYPT_ALLOC)(size_t cbsize);
typedef VOID   (WINAPI *PFN_CRYPT_FREE)(LPVOID pv);

typedef struct _CRYPT_ENCRYPT_MESSAGE_PARA {
    DWORD cbSize;
    DWORD dwMsgEncodingType;
    HCRYPTPROV hCryptProv;
    CRYPT_ALGORITHM_IDENTIFIER ContentEncryptionAlgorithm;
    void* pvEncryptionAuxInfo;
    DWORD dwFlags;
    DWORD dwInnerContentType;
} CRYPT_ENCRYPT_MESSAGE_PARA, *PCRYPT_ENCRYPT_MESSAGE_PARA;
typedef struct _CRYPT_DECODE_PARA {
    DWORD           cbSize;
    PFN_CRYPT_ALLOC pfnAlloc;
    PFN_CRYPT_FREE  pfnFree;
} CRYPT_DECODE_PARA, *PCRYPT_DECODE_PARA;
typedef struct _CRYPT_SEQUENCE_OF_ANY {
    DWORD           cValue;
    PCRYPT_DER_BLOB rgValue;
} CRYPT_SEQUENCE_OF_ANY, *PCRYPT_SEQUENCE_OF_ANY;
typedef struct _CERT_TRUST_STATUS {
	DWORD dwErrorStatus;
	DWORD dwInfoStatus;
} CERT_TRUST_STATUS, *PCERT_TRUST_STATUS;
typedef struct _CRL_ENTRY {
	CRYPT_INTEGER_BLOB SerialNumber;
	FILETIME RevocationDate;
	DWORD cExtension;
	PCERT_EXTENSION rgExtension;
} CRL_ENTRY, *PCRL_ENTRY;
typedef struct _CRL_INFO {
	DWORD dwVersion;
	CRYPT_ALGORITHM_IDENTIFIER SignatureAlgorithm;
	CERT_NAME_BLOB Issuer;
	FILETIME ThisUpdate;
	FILETIME NextUpdate;
	DWORD cCRLEntry;
	PCRL_ENTRY rgCRLEntry;
	DWORD cExtension;
	PCERT_EXTENSION rgExtension;
} CRL_INFO, *PCRL_INFO;
typedef struct _CRL_CONTEXT {
	DWORD dwCertEncodingType;
	BYTE* pbCrlEncoded;
	DWORD cbCrlEncoded;
	PCRL_INFO pCrlInfo;
	HCERTSTORE hCertStore;
} CRL_CONTEXT, *PCRL_CONTEXT;
typedef const CRL_CONTEXT *PCCRL_CONTEXT;
typedef struct _CERT_REVOCATION_CRL_INFO {
	DWORD cbSize;
	PCCRL_CONTEXT pBaseCRLContext;
	PCCRL_CONTEXT pDeltaCRLContext;
	PCRL_ENTRY pCrlEntry;
	BOOL fDeltaCrlEntry;
} CERT_REVOCATION_CRL_INFO, *PCERT_REVOCATION_CRL_INFO;
typedef struct _CERT_REVOCATION_INFO {
	DWORD cbSize;
	DWORD dwRevocationResult;
	LPCSTR pszRevocationOid;
	LPVOID pvOidSpecificInfo;
	BOOL fHasFreshnessTime;
	DWORD dwFreshnessTime;
	PCERT_REVOCATION_CRL_INFO pCrlInfo;
} CERT_REVOCATION_INFO, *PCERT_REVOCATION_INFO;
/* #if (_WIN32_WINNT>=0x500) */
typedef struct _CERT_CHAIN_ELEMENT {
	DWORD cbSize;
	PCCERT_CONTEXT pCertContext;
	CERT_TRUST_STATUS TrustStatus;
	PCERT_REVOCATION_INFO pRevocationInfo;
	PCERT_ENHKEY_USAGE pIssuanceUsage;
	PCERT_ENHKEY_USAGE pApplicationUsage;
} CERT_CHAIN_ELEMENT, *PCERT_CHAIN_ELEMENT;
/* #endif */
typedef struct _CRYPT_ATTRIBUTE {
	LPSTR pszObjId;
	DWORD cValue;
	PCRYPT_ATTR_BLOB rgValue;
} CRYPT_ATTRIBUTE, *PCRYPT_ATTRIBUTE;
typedef struct _CTL_ENTRY {
	CRYPT_DATA_BLOB SubjectIdentifier;
	DWORD cAttribute;
	PCRYPT_ATTRIBUTE rgAttribute;
} CTL_ENTRY, *PCTL_ENTRY;
typedef struct _CTL_INFO {
	DWORD dwVersion;
	CTL_USAGE SubjectUsage;
	CRYPT_DATA_BLOB ListIdentifier;
	CRYPT_INTEGER_BLOB SequenceNumber;
	FILETIME ThisUpdate;
	FILETIME NextUpdate;
	CRYPT_ALGORITHM_IDENTIFIER SubjectAlgorithm;
	DWORD cCTLEntry;
	PCTL_ENTRY rgCTLEntry;
	DWORD cExtension;
	PCERT_EXTENSION rgExtension;
} CTL_INFO, *PCTL_INFO;
typedef struct _CTL_CONTEXT {
	DWORD dwMsgAndCertEncodingType;
	BYTE* pbCtlEncoded;
	DWORD cbCtlEncoded;
	PCTL_INFO pCtlInfo;
	HCERTSTORE hCertStore;
	HCRYPTMSG hCryptMsg;
	BYTE* pbCtlContent;
	DWORD cbCtlContent;
} CTL_CONTEXT, *PCTL_CONTEXT;
typedef const CTL_CONTEXT *PCCTL_CONTEXT;
typedef struct _CERT_TRUST_LIST_INFO {
	DWORD cbSize;
	PCTL_ENTRY pCtlEntry;
	PCCTL_CONTEXT pCtlContext;
} CERT_TRUST_LIST_INFO, *PCERT_TRUST_LIST_INFO;
typedef struct _CERT_SIMPLE_CHAIN {
	DWORD cbSize;
	CERT_TRUST_STATUS TrustStatus;
	DWORD cElement;
	PCERT_CHAIN_ELEMENT* rgpElement;
	PCERT_TRUST_LIST_INFO pTrustListInfo;
	BOOL fHasRevocationFreshnessTime;
	DWORD dwRevocationFreshnessTime;
} CERT_SIMPLE_CHAIN, *PCERT_SIMPLE_CHAIN;
/* #if (_WIN32_WINNT>=0x500) */
typedef const struct _CERT_CHAIN_CONTEXT* PCCERT_CHAIN_CONTEXT;
typedef struct _CERT_CHAIN_CONTEXT {
	DWORD cbSize;
	CERT_TRUST_STATUS TrustStatus;
	DWORD cChain;
	PCERT_SIMPLE_CHAIN* rgpChain;
	DWORD cLowerQualityChainContext;
	PCCERT_CHAIN_CONTEXT* rgpLowerQualityChainContext;
	BOOL fHasRevocationFreshnessTime;
	DWORD dwRevocationFreshnessTime;
} CERT_CHAIN_CONTEXT, *PCERT_CHAIN_CONTEXT;
/* #endif */
typedef struct _PROV_ENUMALGS {
	ALG_ID aiAlgid;
	DWORD dwBitLen;
	DWORD dwNameLen;
	CHAR szName[20];
} PROV_ENUMALGS;
typedef struct _RSAPUBKEY {
	DWORD magic;
	DWORD bitlen;
	DWORD pubexp;
} RSAPUBKEY;
typedef struct _CRYPTPROTECT_PROMPTSTRUCT{
  DWORD   cbSize;
  DWORD   dwPromptFlags;
  HWND    hwndApp;
  LPCWSTR szPrompt;
} CRYPTPROTECT_PROMPTSTRUCT, *PCRYPTPROTECT_PROMPTSTRUCT;

typedef struct _CERT_SIGNED_CONTENT_INFO {
    CRYPT_DER_BLOB             ToBeSigned;
    CRYPT_ALGORITHM_IDENTIFIER SignatureAlgorithm;
    CRYPT_BIT_BLOB             Signature;
} CERT_SIGNED_CONTENT_INFO, *PCERT_SIGNED_CONTENT_INFO;

typedef struct _CERT_EXTENSIONS {
    DWORD           cExtension;
    PCERT_EXTENSION rgExtension;
} CERT_EXTENSIONS, *PCERT_EXTENSIONS;

typedef struct _CERT_RDN_ATTR {
    LPSTR               pszObjId;
    DWORD               dwValueType;
    CERT_RDN_VALUE_BLOB Value;
} CERT_RDN_ATTR, *PCERT_RDN_ATTR;

typedef struct _CERT_NAME_VALUE {
    DWORD               dwValueType;
    CERT_RDN_VALUE_BLOB Value;
} CERT_NAME_VALUE, *PCERT_NAME_VALUE;

typedef struct _CERT_RDN {
    DWORD          cRDNAttr;
    PCERT_RDN_ATTR rgRDNAttr;
} CERT_RDN, *PCERT_RDN;

typedef struct _CERT_NAME_INFO {
    DWORD     cRDN;
    PCERT_RDN rgRDN;
} CERT_NAME_INFO, *PCERT_NAME_INFO;

typedef struct _CRYPT_ENCODE_PARA {
    DWORD           cbSize;
    PFN_CRYPT_ALLOC pfnAlloc;
    PFN_CRYPT_FREE  pfnFree;
} CRYPT_ENCODE_PARA, *PCRYPT_ENCODE_PARA;

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

typedef struct _CERT_STORE_PROV_INFO {
    DWORD             cbSize;
    DWORD             cStoreProvFunc;
    void            **rgpvStoreProvFunc;
    HCERTSTOREPROV    hStoreProv;
    DWORD             dwStoreProvFlags;
    HCRYPTOIDFUNCADDR hStoreProvFuncAddr2;
} CERT_STORE_PROV_INFO, *PCERT_STORE_PROV_INFO;

#define CERT_ALT_NAME_ENTRY_ERR_INDEX_MASK  0xff
#define CERT_ALT_NAME_ENTRY_ERR_INDEX_SHIFT 16
#define CERT_ALT_NAME_VALUE_ERR_INDEX_MASK  0x0000ffff
#define CERT_ALT_NAME_VALUE_ERR_INDEX_SHIFT 0
#define GET_CERT_ALT_NAME_ENTRY_ERR_INDEX(x) \
 (((x) >> CERT_ALT_NAME_ENTRY_ERR_INDEX_SHIFT) & \
  CERT_ALT_NAME_ENTRY_ERR_INDEX_MASK)
#define GET_CERT_ALT_NAME_VALUE_ERR_INDEX(x) \
 ((x) & CERT_ALT_NAME_VALUE_ERR_INDEX_MASK)

typedef struct _CERT_BASIC_CONSTRAINTS2_INFO {
    BOOL  fCA;
    BOOL  fPathLenConstraint;
    DWORD dwPathLenConstraint;
} CERT_BASIC_CONSTRAINTS2_INFO, *PCERT_BASIC_CONSTRAINTS2_INFO;

typedef BOOL (WINAPI *PFN_CERT_ENUM_SYSTEM_STORE_LOCATION)(
 LPCWSTR pwszStoreLocation, DWORD dwFlags, void *pvReserved, void *pvArg);

typedef BOOL (WINAPI *PFN_CERT_ENUM_SYSTEM_STORE)(const void *pvSystemStore,
 DWORD dwFlags, PCERT_SYSTEM_STORE_INFO pStoreInfo, void *pvReserved,
 void *pvArg);

typedef BOOL (WINAPI *PFN_CERT_ENUM_PHYSICAL_STORE)(const void *pvSystemStore,
 DWORD dwFlags, LPCWSTR pwszStoreName, PCERT_PHYSICAL_STORE_INFO pStoreInfo,
 void *pvReserved, void *pvArg);

/* Encode/decode object */
typedef LPVOID (WINAPI *PFN_CRYPT_ALLOC)(size_t cbsize);
typedef VOID   (WINAPI *PFN_CRYPT_FREE)(LPVOID pv);

typedef BOOL (WINAPI *PFN_CERT_DLL_OPEN_STORE_PROV_FUNC)(
 LPCSTR lpszStoreProvider, DWORD dwEncodingType, HCRYPTPROV hCryptProv,
 DWORD dwFlags, const void *pvPara, HCERTSTORE hCertStore,
 PCERT_STORE_PROV_INFO pStoreProvInfo);

typedef void (WINAPI *PFN_CERT_STORE_PROV_CLOSE)(HCERTSTOREPROV hStoreProv,
 DWORD dwFlags);

typedef BOOL (WINAPI *PFN_CERT_STORE_PROV_READ_CERT)(HCERTSTOREPROV hStoreProv,
 PCCERT_CONTEXT pStoreCertContext, DWORD dwFlags,
 PCCERT_CONTEXT *ppProvCertContext);

typedef BOOL (WINAPI *PFN_CERT_STORE_PROV_WRITE_CERT)(HCERTSTOREPROV hStoreProv,
 PCCERT_CONTEXT pCertContext, DWORD dwFlags);

typedef BOOL (WINAPI *PFN_CERT_STORE_PROV_DELETE_CERT)(
 HCERTSTOREPROV hStoreProv, PCCERT_CONTEXT pCertContext, DWORD dwFlags);

typedef BOOL (WINAPI *PFN_CERT_STORE_PROV_SET_CERT_PROPERTY)(
 HCERTSTOREPROV hStoreProv, PCCERT_CONTEXT pCertContext, DWORD dwPropId,
 DWORD dwFlags, const void *pvData);

typedef BOOL (WINAPI *PFN_CERT_STORE_PROV_READ_CRL)(HCERTSTOREPROV hStoreProv,
 PCCRL_CONTEXT pStoreCrlContext, DWORD dwFlags,
 PCCRL_CONTEXT *ppProvCrlContext);

typedef BOOL (WINAPI *PFN_CERT_STORE_PROV_WRITE_CRL)(HCERTSTOREPROV hStoreProv,
 PCCRL_CONTEXT pCrlContext, DWORD dwFlags);

typedef BOOL (WINAPI *PFN_CERT_STORE_PROV_DELETE_CRL)(HCERTSTOREPROV hStoreProv,
 PCCRL_CONTEXT pCrlContext, DWORD dwFlags);

typedef BOOL (WINAPI *PFN_CERT_STORE_PROV_SET_CRL_PROPERTY)(
 HCERTSTOREPROV hStoreProv, PCCRL_CONTEXT pCrlContext, DWORD dwPropId,
 DWORD dwFlags, const void *pvData);

typedef BOOL (WINAPI *PFN_CERT_STORE_PROV_READ_CTL)(HCERTSTOREPROV hStoreProv,
 PCCTL_CONTEXT pStoreCtlContext, DWORD dwFlags,
 PCCTL_CONTEXT *ppProvCtlContext);

typedef BOOL (WINAPI *PFN_CERT_STORE_PROV_WRITE_CTL)(HCERTSTOREPROV hStoreProv,
 PCCTL_CONTEXT pCtlContext, DWORD dwFlags);

typedef BOOL (WINAPI *PFN_CERT_STORE_PROV_DELETE_CTL)(
 HCERTSTOREPROV hStoreProv, PCCTL_CONTEXT pCtlContext, DWORD dwFlags);

typedef BOOL (WINAPI *PFN_CERT_STORE_PROV_SET_CTL_PROPERTY)(
 HCERTSTOREPROV hStoreProv, PCCTL_CONTEXT pCtlContext, DWORD dwPropId,
 DWORD dwFlags, const void *pvData);

typedef BOOL (WINAPI *PFN_CERT_STORE_PROV_CONTROL)(HCERTSTOREPROV hStoreProv,
 DWORD dwFlags, DWORD dwCtrlType, void const *pvCtrlPara);

typedef struct _CRYPT_OID_FUNC_ENTRY {
    LPCSTR pszOID;
    void  *pvFuncAddr;
} CRYPT_OID_FUNC_ENTRY, *PCRYPT_OID_FUNC_ENTRY;

typedef BOOL (WINAPI *PFN_CRYPT_ENUM_OID_FUNC)(DWORD dwEncodingType,
 LPCSTR pszFuncName, LPCSTR pszOID, DWORD cValue, const DWORD rgdwValueType[],
 LPCWSTR const rgpwszValueName[], const BYTE * const rgpbValueData[],
 const DWORD rgcbValueData[], void *pvArg);

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

/*  types for CryptVerifyCertificateSignatureEx */
#define CRYPT_VERIFY_CERT_SIGN_SUBJECT_BLOB 1
#define CRYPT_VERIFY_CERT_SIGN_SUBJECT_CERT 2
#define CRYPT_VERIFY_CERT_SIGN_SUBJECT_CRL  3

/* issuer types for CryptVerifyCertificateSignatureEx */
#define CRYPT_VERIFY_CERT_SIGN_ISSUER_PUBKEY 1
#define CRYPT_VERIFY_CERT_SIGN_ISSUER_CERT   2
#define CRYPT_VERIFY_CERT_SIGN_ISSUER_CHAIN  3
#define CRYPT_VERIFY_CERT_SIGN_ISSUER_NULL   4

/* crypt32.dll functions */
LPVOID WINAPI CryptMemAlloc(ULONG cbSize);
LPVOID WINAPI CryptMemRealloc(LPVOID pv, ULONG cbSize);
VOID   WINAPI CryptMemFree(LPVOID pv);

PCRYPT_ATTRIBUTE WINAPI CertFindAttribute(LPCSTR pszObjId, DWORD cAttr, CRYPT_ATTRIBUTE rgAttr[]);
PCERT_EXTENSION WINAPI CertFindExtension(LPCSTR pszObjId, DWORD cExtensions, CERT_EXTENSION rgExtensions[]);
PCERT_RDN_ATTR WINAPI CertFindRDNAttr(LPCSTR pszObjId, PCERT_NAME_INFO pName);

BOOL WINAPI CertSerializeCertificateStoreElement(PCCERT_CONTEXT pCertContext, DWORD dwFlags, BYTE *pbElement, DWORD *pcbElement);
BOOL WINAPI CertSerializeCRLStoreElement(PCCRL_CONTEXT pCrlContext, DWORD dwFlags, BYTE *pbElement, DWORD *pcbElement);
BOOL WINAPI CertSerializeCTLStoreElement(PCCTL_CONTEXT pCtlContext, DWORD dwFlags, BYTE *pbElement, DWORD *pcbElement);
BOOL WINAPI CertAddCertificateContextToStore(HCERTSTORE hCertStore, PCCERT_CONTEXT pCertContext, DWORD dwAddDisposition, PCCERT_CONTEXT *ppStoreContext);
BOOL WINAPI CertAddEncodedCertificateToStore(HCERTSTORE hCertStore, DWORD dwCertEncodingType, const BYTE *pbCertEncoded, DWORD cbCertEncoded, DWORD dwAddDisposition, PCCERT_CONTEXT *ppCertContext);
PCCERT_CONTEXT WINAPI CertCreateCertificateContext(DWORD dwCertEncodingType, const BYTE *pbCertEncoded, DWORD cbCertEncoded);
PCCERT_CONTEXT WINAPI CertEnumCertificatesInStore(HCERTSTORE hCertStore, PCCERT_CONTEXT pPrev);
PCCERT_CONTEXT WINAPI CertFindCertificateInStore(HCERTSTORE,DWORD,DWORD,DWORD,const void*,PCCERT_CONTEXT);
PCCTL_CONTEXT WINAPI CertEnumCTLsInStore(HCERTSTORE hCertStore, PCCTL_CONTEXT pPrev);
PCCTL_CONTEXT WINAPI CertCreateCTLContext(DWORD dwMsgAndCertEncodingType, const BYTE *pbCtlEncoded, DWORD cbCtlEncoded);
PCCRL_CONTEXT WINAPI CertCreateCRLContext( DWORD dwCertEncodingType,  const BYTE* pbCrlEncoded, DWORD cbCrlEncoded);
PCCRL_CONTEXT WINAPI CertEnumCRLsInStore(HCERTSTORE hCertStore, PCCRL_CONTEXT pPrev);
BOOL WINAPI CryptHashCertificate(HCRYPTPROV hCryptProv, ALG_ID Algid, DWORD dwFlags, const BYTE *pbEncoded, DWORD cbEncoded, BYTE *pbComputedHash, DWORD *pcbComputedHash);
BOOL WINAPI CertFreeCTLContext( PCCTL_CONTEXT pCtlContext );
BOOL WINAPI CertDeleteCTLFromStore(PCCTL_CONTEXT pCtlContext);
BOOL WINAPI CertSetCTLContextProperty(PCCTL_CONTEXT pCTLContext, DWORD dwPropId, DWORD dwFlags, const void *pvData);
BOOL WINAPI CertGetCTLContextProperty(PCCTL_CONTEXT pCTLContext, DWORD dwPropId, void *pvData, DWORD *pcbData);
BOOL WINAPI CertAddCTLContextToStore( HCERTSTORE hCertStore, PCCTL_CONTEXT pCtlContext, DWORD dwAddDisposition, PCCTL_CONTEXT *ppStoreContext );
BOOL WINAPI CertFreeCRLContext( PCCRL_CONTEXT pCrlContext );
BOOL WINAPI CertSetCRLContextProperty(PCCRL_CONTEXT pCRLContext, DWORD dwPropId, DWORD dwFlags, const void *pvData);
BOOL WINAPI CertDeleteCRLFromStore(PCCRL_CONTEXT pCrlContext);
BOOL WINAPI CertAddEncodedCTLToStore(HCERTSTORE hCertStore, DWORD dwMsgAndCertEncodingType, const BYTE *pbCtlEncoded, DWORD cbCtlEncoded, DWORD dwAddDisposition, PCCTL_CONTEXT *ppCtlContext);
BOOL WINAPI CertAddEncodedCRLToStore(HCERTSTORE hCertStore, DWORD dwCertEncodingType, const BYTE *pbCrlEncoded, DWORD cbCrlEncoded, DWORD dwAddDisposition, PCCRL_CONTEXT *ppCrlContext);
BOOL WINAPI CertDeleteCertificateFromStore(PCCERT_CONTEXT pCertContext);
BOOL WINAPI CertSetCertificateContextProperty(PCCERT_CONTEXT pCertContext, DWORD dwPropId, DWORD dwFlags, const void *pvData);
BOOL WINAPI CertAddCRLContextToStore( HCERTSTORE hCertStore, PCCRL_CONTEXT pCrlContext, DWORD dwAddDisposition, PCCRL_CONTEXT *ppStoreContext );
BOOL WINAPI CertGetCRLContextProperty(PCCRL_CONTEXT pCRLContext, DWORD dwPropId, void *pvData, DWORD *pcbData);
BOOL WINAPI CertGetCertificateContextProperty(PCCERT_CONTEXT pCertContext, DWORD dwPropId, void *pvData, DWORD *pcbData);
BOOL WINAPI CertCloseStore(HCERTSTORE,DWORD);
BOOL WINAPI CertGetCertificateChain(HCERTCHAINENGINE,PCCERT_CONTEXT,LPFILETIME,HCERTSTORE,PCERT_CHAIN_PARA,DWORD,LPVOID,PCCERT_CHAIN_CONTEXT*);
BOOL WINAPI CertVerifyCertificateChainPolicy(LPCSTR,PCCERT_CHAIN_CONTEXT,PCERT_CHAIN_POLICY_PARA,PCERT_CHAIN_POLICY_STATUS);
void WINAPI CertFreeCertificateChain(PCCERT_CHAIN_CONTEXT);
DWORD WINAPI CertNameToStrA(DWORD,PCERT_NAME_BLOB,DWORD,LPSTR,DWORD);
DWORD WINAPI CertNameToStrW(DWORD,PCERT_NAME_BLOB,DWORD,LPWSTR,DWORD);
HCERTSTORE WINAPI CertOpenSystemStoreA(HCRYPTPROV,LPCSTR);
HCERTSTORE WINAPI CertOpenSystemStoreW(HCRYPTPROV,LPCWSTR);
DWORD WINAPI CertGetNameStringW(PCCERT_CONTEXT pCertContext, DWORD dwType, DWORD dwFlags, LPVOID pvType, LPWSTR pszName, DWORD dwName);
DWORD WINAPI CertGetNameStringA(PCCERT_CONTEXT pCertContext, DWORD dwType, DWORD dwFlags, LPVOID pvType, LPSTR pszName, DWORD dwName);
HCERTSTORE WINAPI CertOpenStore(LPCSTR lpszStoreProvider, DWORD dwEncodingType, HCRYPTPROV hCryptProv, DWORD dwFlags, const void *pvPara);
BOOL WINAPI CertFreeCertificateContext(PCCERT_CONTEXT);
DWORD WINAPI CertNameToStrA(DWORD dwCertEncoding, PCERT_NAME_BLOB pCertName, DWORD dwType, LPSTR psz, DWORD dwSZ);
DWORD WINAPI CertNameToStrW(DWORD dwCertEncoding, PCERT_NAME_BLOB pCertName, DWORD dwType, LPWSTR psz, DWORD dwSZ);
PCCERT_CONTEXT WINAPI CertGetIssuerCertificateFromStore(HCERTSTORE,PCCERT_CONTEXT,PCCERT_CONTEXT,DWORD*);
PCCERT_CHAIN_CONTEXT WINAPI CertFindChainInStore(HCERTSTORE,DWORD,DWORD,DWORD,const void*,PCCERT_CHAIN_CONTEXT);
BOOL WINAPI CryptAcquireContextA(HCRYPTPROV*,LPCSTR,LPCSTR,DWORD,DWORD);
BOOL WINAPI CryptAcquireContextW(HCRYPTPROV*,LPCWSTR,LPCWSTR,DWORD,DWORD);
BOOL WINAPI CryptContextAddRef(HCRYPTPROV,DWORD*,DWORD);
BOOL WINAPI CryptReleaseContext(HCRYPTPROV,DWORD);
BOOL WINAPI CryptGenKey(HCRYPTPROV,ALG_ID,DWORD,HCRYPTKEY*);
BOOL WINAPI CryptDeriveKey(HCRYPTPROV,ALG_ID,HCRYPTHASH,DWORD,HCRYPTKEY*);
BOOL WINAPI CryptDestroyKey(HCRYPTKEY);
BOOL WINAPI CryptEncryptMessage( PCRYPT_ENCRYPT_MESSAGE_PARA pEncryptMessagePara, DWORD dwCert, PCCERT_CONTEXT pccertCert[],  const BYTE* pbEncrypted, DWORD dwEncrypted, BYTE* pbBlob, DWORD* dwEncryptedBlob);
#if (WINVER >= 0x0500)
BOOL WINAPI CryptDuplicateHash(HCRYPTHASH,DWORD*,DWORD,HCRYPTHASH*);
BOOL WINAPI CryptDuplicateKey(HCRYPTKEY,DWORD*,DWORD,HCRYPTKEY*);
#endif
BOOL WINAPI CertAddStoreToCollection(HCERTSTORE hCollectionStore, HCERTSTORE hSiblingStore, DWORD dwUpdateFlags, DWORD dwPriority);
BOOL WINAPI CryptExportKey (HCRYPTKEY hKey, HCRYPTKEY hExpKey, DWORD dwBlobType, DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen);
LPCSTR WINAPI CertAlgIdToOID(DWORD dwAlgId);
DWORD WINAPI CertOIDToAlgId(LPCSTR pszObjId);
BOOL WINAPI CryptExportPublicKeyInfo(HCRYPTPROV hCryptProv, DWORD dwKeySpec, DWORD dwCertEncodingType, PCERT_PUBLIC_KEY_INFO pInfo, DWORD *pcbInfo);
BOOL WINAPI CryptExportPublicKeyInfoEx(HCRYPTPROV hCryptProv, DWORD dwKeySpec, DWORD dwCertEncodingType, LPSTR pszPublicKeyObjId, DWORD dwFlags, void *pvAuxInfo, PCERT_PUBLIC_KEY_INFO pInfo, DWORD *pcbInfo);
BOOL WINAPI CryptImportPublicKeyInfo(HCRYPTPROV hCryptProv, DWORD dwCertEncodingType, PCERT_PUBLIC_KEY_INFO pInfo, HCRYPTKEY *phKey);
BOOL WINAPI CryptImportPublicKeyInfoEx(HCRYPTPROV hCryptProv, DWORD dwCertEncodingType, PCERT_PUBLIC_KEY_INFO pInfo, ALG_ID aiKeyAlg, DWORD dwFlags, void *pvAuxInfo, HCRYPTKEY *phKey);
BOOL WINAPI CryptProtectData( DATA_BLOB* pDataIn, LPCWSTR szDataDescr, DATA_BLOB* pOptionalEntropy, PVOID pvReserved, CRYPTPROTECT_PROMPTSTRUCT* pPromptStruct, DWORD dwFlags, DATA_BLOB* pDataOut );
BOOL WINAPI CryptUnprotectData( DATA_BLOB* pDataIn, LPWSTR* ppszDataDescr, DATA_BLOB* pOptionalEntropy, PVOID pvReserved, CRYPTPROTECT_PROMPTSTRUCT* pPromptStruct, DWORD dwFlags, DATA_BLOB* pDataOut );
BOOL WINAPI CryptVerifyCertificateSignature(HCRYPTPROV hCryptProv, DWORD dwCertEncodingType, const BYTE *pbEncoded, DWORD cbEncoded, PCERT_PUBLIC_KEY_INFO pPublicKey);
BOOL WINAPI CryptVerifyCertificateSignatureEx(HCRYPTPROV hCryptProv, DWORD dwCertEncodingType, DWORD dwSubjectType, void *pvSubject, DWORD dwIssuerType, void *pvIssuer, DWORD dwFlags, void *pvReserved);
BOOL WINAPI CryptSetKeyParam(HCRYPTKEY,DWORD,PBYTE,DWORD);
BOOL WINAPI CryptGetKeyParam(HCRYPTKEY,DWORD,PBYTE,PDWORD,DWORD);
BOOL WINAPI CryptDecodeObject(DWORD dwCertEncodingType, LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo);
BOOL WINAPI CryptDecodeObjectEx(DWORD dwCertEncodingType, LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags, PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo);
BOOL WINAPI CryptSetHashParam(HCRYPTHASH,DWORD,PBYTE,DWORD);
BOOL WINAPI CryptGetHashParam(HCRYPTHASH,DWORD,PBYTE,PDWORD,DWORD);
BOOL WINAPI CryptSetProvParam(HCRYPTPROV,DWORD,PBYTE,DWORD);
BOOL WINAPI CryptGetProvParam(HCRYPTPROV,DWORD,PBYTE,PDWORD,DWORD);
BOOL WINAPI CryptGenRandom(HCRYPTPROV,DWORD,PBYTE);
BOOL WINAPI CryptGetUserKey(HCRYPTPROV,DWORD,HCRYPTKEY*);
BOOL WINAPI CryptExportKey(HCRYPTKEY,HCRYPTKEY,DWORD,DWORD,PBYTE,PDWORD);
BOOL WINAPI CryptImportKey(HCRYPTPROV,PBYTE,DWORD,HCRYPTKEY,DWORD,HCRYPTKEY*);
BOOL WINAPI CryptEncrypt(HCRYPTKEY,HCRYPTHASH,BOOL,DWORD,PBYTE,PDWORD,DWORD);
BOOL WINAPI CryptDecrypt(HCRYPTKEY,HCRYPTHASH,BOOL,DWORD,PBYTE,PDWORD);
BOOL WINAPI CryptCreateHash(HCRYPTPROV,ALG_ID,HCRYPTKEY,DWORD,HCRYPTHASH*);
BOOL WINAPI CryptHashData (HCRYPTHASH hHash, const BYTE *pbData, DWORD dwDataLen, DWORD dwFlags);
BOOL WINAPI CryptHashSessionKey(HCRYPTHASH,HCRYPTKEY,DWORD);
BOOL WINAPI CryptGetHashValue(HCRYPTHASH,DWORD,PBYTE,PDWORD);
BOOL WINAPI CryptDestroyHash(HCRYPTHASH);
BOOL WINAPI CryptSignHashA(HCRYPTHASH,DWORD,LPCSTR,DWORD,PBYTE,PDWORD);
BOOL WINAPI CryptSignHashW(HCRYPTHASH,DWORD,LPCWSTR,DWORD,PBYTE,PDWORD);
BOOL WINAPI CryptVerifySignatureA(HCRYPTHASH,PBYTE,DWORD,HCRYPTKEY,LPCSTR,DWORD);
BOOL WINAPI CryptVerifySignatureW(HCRYPTHASH,PBYTE,DWORD,HCRYPTKEY,LPCWSTR,DWORD);
BOOL WINAPI CryptSetProviderA(LPCSTR,DWORD);
BOOL WINAPI CryptSetProviderW(LPCWSTR,DWORD);
BOOL WINAPI CryptSetProviderExA(LPCSTR,DWORD,DWORD*,DWORD);
BOOL WINAPI CryptSetProviderExW(LPCWSTR,DWORD,DWORD*,DWORD);
BOOL WINAPI CryptEncodeObject(DWORD dwCertEncodingType, LPCSTR lpszStructType, const void *pvStructInfo, BYTE *pbEncoded, DWORD *pcbEncoded);
BOOL WINAPI CryptEncodeObjectEx(DWORD dwCertEncodingType, LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags, PCRYPT_ENCODE_PARA pEncodePara, void *pvEncoded, DWORD *pcbEncoded);

BOOL WINAPI CryptRegisterDefaultOIDFunction(DWORD,LPCSTR,DWORD,LPCWSTR);
BOOL WINAPI CryptRegisterOIDFunction(DWORD,LPCSTR,LPCSTR,LPCWSTR,LPCSTR);
BOOL WINAPI CryptGetOIDFunctionValue(DWORD dwEncodingType, LPCSTR pszFuncName,
                                     LPCSTR pszOID, LPCWSTR szValueName, DWORD *pdwValueType,
                                     BYTE *pbValueData, DWORD *pcbValueData);
BOOL WINAPI CryptSetOIDFunctionValue(DWORD dwEncodingType, LPCSTR pszFuncName,
                                     LPCSTR pszOID, LPCWSTR pwszValueName, DWORD dwValueType,
                                     const BYTE *pbValueData, DWORD cbValueData);
BOOL WINAPI CryptUnregisterDefaultOIDFunction(DWORD,LPCSTR,LPCWSTR);
BOOL WINAPI CryptUnregisterOIDFunction(DWORD,LPCSTR,LPCSTR);
BOOL WINAPI CryptEnumOIDFunction(DWORD dwEncodingType, LPCSTR pszFuncName,
 LPCSTR pszOID, DWORD dwFlags, void *pvArg,
 PFN_CRYPT_ENUM_OID_FUNC pfnEnumOIDFunc);
HCRYPTOIDFUNCSET WINAPI CryptInitOIDFunctionSet(LPCSTR,DWORD);
BOOL WINAPI CryptGetDefaultOIDDllList(HCRYPTOIDFUNCSET hFuncSet,
 DWORD dwEncodingType, LPWSTR pwszDllList, DWORD *pcchDllList);
BOOL WINAPI CryptGetDefaultOIDFunctionAddress(HCRYPTOIDFUNCSET hFuncSet,
 DWORD dwEncodingType, LPCWSTR pwszDll, DWORD dwFlags, void *ppvFuncAddr,
 HCRYPTOIDFUNCADDR *phFuncAddr);
BOOL WINAPI CryptGetOIDFunctionAddress(HCRYPTOIDFUNCSET hFuncSet,
 DWORD dwEncodingType, LPCSTR pszOID, DWORD dwFlags, void **ppvFuncAddr,
 HCRYPTOIDFUNCADDR *phFuncAddr);
BOOL WINAPI CryptFreeOIDFunctionAddress(HCRYPTOIDFUNCADDR hFuncAddr,
 DWORD dwFlags);
BOOL WINAPI CryptInstallOIDFunctionAddress(HMODULE hModule,
 DWORD dwEncodingType, LPCSTR pszFuncName, DWORD cFuncEntry,
 const CRYPT_OID_FUNC_ENTRY rgFuncEntry[], DWORD dwFlags);

#ifdef UNICODE
#define CertNameToStr CertNameToStrW
#define CryptAcquireContext CryptAcquireContextW
#define CryptSignHash CryptSignHashW
#define CryptVerifySignature CryptVerifySignatureW
#define CryptSetProvider CryptSetProviderW
#define CertOpenSystemStore CertOpenSystemStoreW
#define CERT_FIND_SUBJECT_STR CERT_FIND_SUBJECT_STR_W
#define CERT_FIND_ISSUER_STR CERT_FIND_ISSUER_STR_W
#define MS_DEF_PROV MS_DEF_PROV_W
#define MS_ENHANCED_PROV MS_ENHANCED_PROV_W
#define MS_STRONG_PROV MS_STRONG_PROV_W
#define MS_DEF_RSA_SIG_PROV MS_DEF_RSA_SIG_PROV_W
#define MS_DEF_RSA_SCHANNEL_PROV MS_DEF_RSA_SCHANNEL_PROV_W
#define MS_DEF_DSS_PROV MS_DEF_DSS_PROV_W
#define MS_DEF_DSS_DH_PROV MS_DEF_DSS_DH_PROV_W
#define MS_ENH_DSS_DH_PROV MS_ENH_DSS_DH_PROV_W
#define MS_DEF_DH_SCHANNEL_PROV MS_DEF_DH_SCHANNEL_PROV_W
#define MS_SCARD_PROV MS_SCARD_PROV_W
#else
#define CertNameToStr CertNameToStrA
#define CryptAcquireContext CryptAcquireContextA
#define CryptSignHash CryptSignHashA
#define CryptVerifySignature CryptVerifySignatureA
#define CryptSetProvider CryptSetProviderA
#define CertOpenSystemStore CertOpenSystemStoreA
#define CERT_FIND_SUBJECT_STR CERT_FIND_SUBJECT_STR_A
#define CERT_FIND_ISSUER_STR CERT_FIND_ISSUER_STR_A
#define MS_DEF_PROV MS_DEF_PROV_A
#define MS_ENHANCED_PROV MS_ENHANCED_PROV_A
#define MS_STRONG_PROV MS_STRONG_PROV_A
#define MS_DEF_RSA_SIG_PROV MS_DEF_RSA_SIG_PROV_A
#define MS_DEF_RSA_SCHANNEL_PROV MS_DEF_RSA_SCHANNEL_PROV_A
#define MS_DEF_DSS_PROV MS_DEF_DSS_PROV_A
#define MS_DEF_DSS_DH_PROV MS_DEF_DSS_DH_PROV_A
#define MS_ENH_DSS_DH_PROV MS_ENH_DSS_DH_PROV_A
#define MS_DEF_DH_SCHANNEL_PROV MS_DEF_DH_SCHANNEL_PROV_A
#define MS_SCARD_PROV MS_SCARD_PROV_A
#endif
#ifdef __cplusplus
}
#endif
#endif /* _WINCRYPT_H */
