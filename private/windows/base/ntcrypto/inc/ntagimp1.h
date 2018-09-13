/////////////////////////////////////////////////////////////////////////////
//  FILE          : ntagimp1.h                                             //
//  DESCRIPTION   :                                                        //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      Apr 19 1995 larrys  Cleanup                                        //
//      May  5 1995 larrys  Changed struct Hash_List_Defn                  //
//      May 10 1995 larrys  added private api calls                        //
//      Aug 15 1995 larrys  Moved CSP_USE_DES to sources file              //
//      Sep 12 1995 Jeffspel/ramas  Merged STT onto CSP                    //
//      Sep 25 1995 larrys  Changed MAXHASHLEN                             //
//      Oct 27 1995 rajeshk Added RandSeed stuff to UserList               //
//      Feb 29 1996 rajeshk Added HashFlags              				   //
//      Sep  4 1996 mattt	Changes to facilitate building STRONG algs	   //
//      Sep 16 1996 mattt   Added Domestic naming                          //
//      Apr 29 1997 jeffspel Protstor support and EnumAlgsEx support       //
//      May 23 1997 jeffspel Added provider type checking                  //
//                                                                         //
//  Copyright (C) 1993 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef __NTAGIMP1_H__
#define __NTAGIMP1_H__

#ifdef __cplusplus
extern "C" {
#endif

#define PROV_SIG                MS_DEF_RSA_SIG_PROV

#ifndef STRONG
#define PROV_NAME               MS_DEF_PROV
#else   // STRONG                   
#define PROV_NAME               MS_ENHANCED_PROV
#endif  // STRONG

#define PROV_SCHANNEL           MS_DEF_RSA_SCHANNEL_PROV



#define CSP_USE_SHA
#define CSP_USE_RC4

// define which algorithms to include
#define CSP_USE_MD2
#define CSP_USE_MD4
#define CSP_USE_MD5
#define CSP_USE_MAC
#define CSP_USE_RC2
#define CSP_USE_SSL3SHAMD5
#define CSP_USE_SSL3
#define CSP_USE_DES
#define CSP_USE_3DES

// handle definition types
#define USER_HANDLE                             0x0
#define HASH_HANDLE                             0x1
#define KEY_HANDLE                              0x2
#define SIGPUBKEY_HANDLE                        0x3
#define EXCHPUBKEY_HANDLE                       0x4

#ifdef _WIN64
#define     HANDLE_MASK     0xE35A172CD96214A0
#else
#define     HANDLE_MASK     0xE35A172C
#endif // _WIN64

typedef ULONG_PTR HNTAG;

typedef struct _htbl {
	void			*pItem;
	DWORD			dwType;
} HTABLE;

#define HNTAG_TO_HTYPE(hntag)   (BYTE)(((HTABLE*)((HNTAG)hntag ^ HANDLE_MASK))->dwType)

// maximum length for the hash
//                                              -- MD4 and MD5
#ifndef STT
#ifdef CSP_USE_SHA
#define MAXHASHLEN              A_SHA_DIGEST_LEN
#else
#define MAXHASHLEN              max(MD4DIGESTLEN, MD5DIGESTLEN)
#endif
#else
#define MAXHASHLEN              A_SHA_DIGEST_LEN        //max(MD4DIGESTLEN, MD5DIGESTLEN)
#endif //STT

#define MAX_BLOCKLEN            8

#define SSL3_MASTER_KEYSIZE     48
#define PCT1_MASTER_KEYSIZE     16
#define SSL2_MASTER_KEYSIZE     5

#define RSA_KEYSIZE_INC         8

#ifdef STRONG	

	// define the length of the RSA modulus in bytes
	#define GRAINSIZE       128
	#define RSAMODLEN       GRAINSIZE

    #define RSA_MAX_SIGN_MODLEN     2048    // 16384 bit
    #define RSA_MAX_EXCH_MODLEN     2048    // 16384 bit
    #define RSA_MIN_SIGN_MODLEN     48      // 384 bit
    #define RSA_MIN_EXCH_MODLEN     48      // 384 bit

    #define RSA_DEF_NEWSTRONG_SIGN_MODLEN   64
    #define RSA_DEF_NEWSTRONG_EXCH_MODLEN   64

    #define DEFAULT_SALT_NEWSTRONG_LENGTH 11    // salt length in bytes
    #define DEFAULT_SALT_LENGTH 0		// salt length in bytes

    #define MAX_KEY_SIZE        48      // largest key size (SSL3 masterkey)

    #define SSL2_MAX_MASTER_KEYSIZE     24

	#pragma message("Building STRONG CSP")
#else	// default

	// define the length of the RSA modulus in bytes
	#define GRAINSIZE       64
	#define RSAMODLEN       GRAINSIZE

    #define RSA_MAX_SIGN_MODLEN     2048    // 16384 bit
    #define RSA_MAX_EXCH_MODLEN     128     // 1024 bit
    #define RSA_MIN_SIGN_MODLEN     48      // 384 bit
    #define RSA_MIN_EXCH_MODLEN     48      // 384 bit

    #define DEFAULT_SALT_LENGTH 11		// salt length in bytes
    #define MAX_KEY_SIZE        48      // largest key size(SSL3 masterkey)

    #define SSL2_MAX_MASTER_KEYSIZE     5

	#pragma message("Building default CSP")
#endif 

// for non-STT builds
#define RSA_DEF_EXCH_MODLEN   RSAMODLEN
#define RSA_DEF_SIGN_MODLEN   RSAMODLEN

#define RC2_MIN_KEYSIZE     5
#define RC4_MIN_KEYSIZE     5

// effective key length defines for RC2
#define RC2_DEFAULT_EFFECTIVE_KEYLEN    40
#define RC2_SCHANNEL_DEFAULT_EFFECTIVE_KEYLEN    128
#define RC2_MIN_EFFECTIVE_KEYLEN        1
#ifdef STRONG
    // this is for the domestic provider which is backward compatible
    // with the international provider
    #define RC2_DEF_NEWSTRONG_KEYSIZE     5
    #define RC4_DEF_NEWSTRONG_KEYSIZE     5
    #define RC2_DEF_KEYSIZE     16
    #define RC4_DEF_KEYSIZE     16
    #define RC2_MAX_KEYSIZE     16
    #define RC4_MAX_KEYSIZE     16
    #define RC2_MAX_EFFECTIVE_KEYLEN        1024
#else
    #define RC2_DEF_KEYSIZE     5
    #define RC4_DEF_KEYSIZE     5
    #define RC2_MAX_KEYSIZE     7
    #define RC4_MAX_KEYSIZE     7

    #define RC2_MAX_EFFECTIVE_KEYLEN        56
#endif

// defines for France
#define RC2_MAX_FRENCH_KEYSIZE     5
#define RC4_MAX_FRENCH_KEYSIZE     5
#define RSA_MAX_EXCH_FRENCH_MODLEN     64      // 512 bit

// defines for SGC
#define SGC_RSA_MAX_EXCH_MODLEN     128      // 512 bit
#define SGC_RSA_DEF_EXCH_MODLEN     128

#define SGC_RC2_DEF_KEYSIZE         16
#define SGC_RC4_DEF_KEYSIZE         16
#define SGC_RC2_MAX_KEYSIZE         16
#define SGC_RC4_MAX_KEYSIZE         16

// check for the maximum hash length greater than the mod length
#if RSAMODLEN < MAXHASHLEN
#error  "RSAMODLEN must be greater than or equal to MAXHASHLEN"
#endif

#define     STORAGE_RC4_KEYLEN      5   // keys always stored under 40-bit RC4 key
#define     STORAGE_RC4_TOTALLEN    16  // 0-value salt fills rest

// types of key storage
#define REG_KEYS                    0 
#define PROTECTED_STORAGE_KEYS      1 
#define PROTECTION_API_KEYS         2

// structure to hold protected storage info
typedef struct _PStore_Info
{
    HINSTANCE   hInst;
    void        *pProv;
    GUID        SigType;
    GUID        SigSubtype;
    GUID        ExchType;
    GUID        ExchSubtype;
    LPWSTR      szPrompt;
    DWORD       cbPrompt;
} PSTORE_INFO;

// definition of a user list
typedef struct _UserList
{
    DWORD                           Rights;
    BOOL                            fNewStrongCSP;
    DWORD                           dwProvType;
    DWORD                           hPrivuid;
    HCRYPTPROV                      hUID;
    BOOL                            fIsLocalSystem;
    DWORD                           dwEnumalgs;
    DWORD                           dwEnumalgsEx;
    KEY_CONTAINER_INFO              ContInfo;
    DWORD                           ExchPrivLen;
    BYTE                            *pExchPrivKey;
    DWORD                           SigPrivLen;
    BYTE                            *pSigPrivKey;
    HKEY                            hKeys;		// AT NTag only
    size_t                          UserLen;
    BYTE                            *pCachePW;
    BYTE                            *pUser;
    HANDLE                          hWnd;
    DWORD                           dwKeyStorageType;
    PSTORE_INFO                     *pPStore;
    LPWSTR                          pwszPrompt;
    DWORD                           dwOldKeyFlags;
    BOOL                            dwSGCFlags;
    BYTE                            *pbSGCKeyMod;
    DWORD                           cbSGCKeyMod;
    DWORD                           dwSGCKeyExpo;
    HANDLE                          hRNGDriver;
    CHAR                            rgszMachineName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD                           cbMachineName;
    CRITICAL_SECTION                CritSec;   
    EXPO_OFFLOAD_STRUCT             *pOffloadInfo; // info for offloading modular expo
} NTAGUserList, *PNTAGUserList;


// UserList Rights flags (uses CRYPT_MACHINE_KEYSET and CRYPT_VERIFYCONTEXT)
#define CRYPT_DISABLE_CRYPT             0x1
#define CRYPT_IN_FRANCE                 0x2
#define CRYPT_DES_HASHKEY_BACKWARDS     0x4

#define CRYPT_BLKLEN    8               // Bytes in a crypt block
#define MAX_SALT_LEN    24


// definition of a key list
typedef struct _KeyList
{
    HCRYPTPROV      hUID;                   // must be first
    ALG_ID			Algid;
    DWORD			Rights;
    DWORD			cbKeyLen;
    BYTE			*pKeyValue;             // Actual Key
    DWORD			cbDataLen;
    BYTE			*pData;                 // Inflated Key or Multi-phase
    BYTE			IV[CRYPT_BLKLEN];       // Initialization vector
    BYTE			FeedBack[CRYPT_BLKLEN]; // Feedback register
    DWORD			InProgress;             // Flag to indicate encryption
    DWORD           cbSaltLen;              // Salt length
    BYTE			rgbSalt[MAX_SALT_LEN];  // Salt value
    DWORD			Padding;                // Padding values
    DWORD			Mode;                   // Mode of cipher
    DWORD			ModeBits;               // Number of bits to feedback
    DWORD			Permissions;            // Key permissions
    DWORD           EffectiveKeyLen;        // used by RC2
    BYTE            *pbParams;              // may be used in OAEP
    DWORD           cbParams;               // length of pbParams
#ifdef STT
    DWORD           cbInfo;
    BYTE            rgbInfo[MAXCCNLEN];
#endif
} NTAGKeyList, *PNTAGKeyList;

#define     HMAC_DEFAULT_STRING_LEN     64

// definition of a hash list
typedef struct Hash_List_Defn
{
    HCRYPTPROV      hUID;
    ALG_ID          Algid;
    DWORD           dwDataLen;
    void            *pHashData;
    HCRYPTKEY       hKey;
    DWORD           HashFlags;
    ALG_ID          HMACAlgid;
    DWORD           HMACState;
    BYTE            *pbHMACInner;
    DWORD           cbHMACInner;
    BYTE            *pbHMACOuter;
    DWORD           cbHMACOuter;
    DWORD           dwHashState;
} NTAGHashList, *PNTAGHashList;

#define     HMAC_STARTED    1
#define     HMAC_FINISHED   2

#define     DATA_IN_HASH    1

// Values of the HashFlags

#define HF_VALUE_SET	1

// Hash algorithm's internal state
// -- Placed into PNTAGHashList->pHashData

// for MD5
#define MD5_object      MD5_CTX

// for MD4
// see md4.h for MD4_object

// Stuff for weird SSL 3.0 signature format
#define SSL3_SHAMD5_LEN   (A_SHA_DIGEST_LEN + MD5DIGESTLEN)

// prototypes
void memnuke(volatile BYTE *data, DWORD len);

BOOL LocalCreateHash(
                     IN ALG_ID Algid,
                     OUT BYTE **ppbHashData,
                     OUT DWORD *pcbHashData
                     );

BOOL LocalHashData(
                   IN ALG_ID Algid,
                   IN OUT BYTE *pbHashData,
                   IN BYTE *pbData,
                   IN DWORD cbData
                   );

BOOL LocalEncrypt(IN HCRYPTPROV hUID,
                  IN HCRYPTKEY hKey,
                  IN HCRYPTHASH hHash,
                  IN BOOL Final,
                  IN DWORD dwFlags,
                  IN OUT BYTE *pbData,
                  IN OUT DWORD *pdwDataLen,
                  IN DWORD dwBufSize,
                  IN BOOL fIsExternal);

BOOL LocalDecrypt(IN HCRYPTPROV hUID,
                  IN HCRYPTKEY hKey,
                  IN HCRYPTHASH hHash,
                  IN BOOL Final,
                  IN DWORD dwFlags,
                  IN OUT BYTE *pbData,
                  IN OUT DWORD *pdwDataLen,
                  IN BOOL fIsExternal);

BOOL FIPS186GenRandom(
                      IN HANDLE *phRNGDriver,
                      IN BYTE **ppbContextSeed,
                      IN DWORD *pcbContextSeed,
                      IN OUT BYTE *pb,
                      IN DWORD cb
                      );
//
// Function : TestEncDec
//
// Description : This function expands the passed in key buffer for the appropriate
//               algorithm, and then either encryption or decryption is performed.
//               A comparison is then made to see if the ciphertext or plaintext
//               matches the expected value.
//               The function only uses ECB mode for block ciphers and the plaintext
//               buffer must be the same length as the ciphertext buffer.  The length
//               of the plaintext must be either the block length of the cipher if it
//               is a block cipher or less than MAX_BLOCKLEN if a stream cipher is
//               being used.
//
BOOL TestEncDec(
                IN ALG_ID Algid,
                IN BYTE *pbKey,
                IN DWORD cbKey,
                IN BYTE *pbPlaintext,
                IN DWORD cbPlaintext,
                IN BYTE *pbCiphertext,
                IN BYTE *pbIV,
                IN int iOperation
                );

//
// Function : TestSymmetricAlgorithm
//
// Description : This function expands the passed in key buffer for the appropriate algorithm,
//               encrypts the plaintext buffer with the same algorithm and key, and the
//               compares the passed in expected ciphertext with the calculated ciphertext
//               to make sure they are the same.  The opposite is then done with decryption.
//               The function only uses ECB mode for block ciphers and the plaintext
//               buffer must be the same length as the ciphertext buffer.  The length
//               of the plaintext must be either the block length of the cipher if it
//               is a block cipher or less than MAX_BLOCKLEN if a stream cipher is
//               being used.
//
BOOL TestSymmetricAlgorithm(
                            IN ALG_ID Algid,
                            IN BYTE *pbKey,
                            IN DWORD cbKey,
                            IN BYTE *pbPlaintext,
                            IN DWORD cbPlaintext,
                            IN BYTE *pbCiphertext,
                            IN BYTE *pbIV
                            );

#ifdef CSP_USE_MD5
//
// Function : TestMD5
//
// Description : This function hashes the passed in message with the MD5 hash
//               algorithm and returns the resulting hash value.
//
BOOL TestMD5(
             BYTE *pbMsg,
             DWORD cbMsg,
             BYTE *pbHash
             );
#endif // CSP_USE_MD5

#ifdef CSP_USE_SHA1
//
// Function : TestSHA1
//
// Description : This function hashes the passed in message with the SHA1 hash
//               algorithm and returns the resulting hash value.
//
BOOL TestSHA1(
              BYTE *pbMsg,
              DWORD cbMsg,
              BYTE *pbHash
              );
#endif // CSP_USE_SHA1

// These may later be changed to set/use NT's [GS]etLastErrorEx
// so make it easy to switch over..
#ifdef MTS
__declspec(thread)
#endif

#ifdef __cplusplus
}
#endif

#endif // __NTAGIMP1_H__
