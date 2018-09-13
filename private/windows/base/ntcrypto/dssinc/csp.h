/* csp.h */

#include <windows.h>
#include <windef.h>
#include <wtypes.h>
#include "wincrypt.h"
#include "winerror.h"
#include "crypto.h"
#include "mem.h"
#include "contman.h"

// Specify which hashing algorithms are supported
#define CSP_USE_SHA1
#define CSP_USE_MD5
#define CSP_USE_RC4
#define CSP_USE_RC2
#define CSP_USE_MAC
#define CSP_USE_DES40
#define CSP_USE_DES

// Specify which encryption algorithms are supported
#ifdef STRONG 
#define CSP_USE_3DES
// domestic RC4 key and salt sizes
#define RC4_MAXKEYSIZE  16
// domestic RC2 key, salt and effective key sizes
#define RC2_MAXKEYSIZE  16
#define RC2_MAXEFFSIZE  128

#define RC_MAXSALTSIZE 256
#else
// international RC4 key and salt sizes
#define RC4_MAXKEYSIZE  7
// international RC2 key, salt and effective key sizes
#define RC2_MAXKEYSIZE  7
#define RC2_MAXEFFSIZE  56

#define RC_MAXSALTSIZE 11
#endif // _STRONG

// key sizes for france
#define RC2_MAX_FRENCH_KEYSIZE  5
#define RC4_MAX_FRENCH_KEYSIZE  5

#define RC2_DEFKEYSIZE  5
#define RC2_MINKEYSIZE  5
#define RC2_DEFEFFSIZE  40
#define RC2_MINEFFSIZE  40

#define RC4_DEFKEYSIZE  5
#define RC4_MINKEYSIZE  5

#define RC_MINSALTSIZE 0
#define RC_DEFSALTSIZE 11

#define RC2_BLOCKLEN    8

#define CSP_USE_SSL3

/*********************************/
/* Definitions                   */
/*********************************/

/*********************************/
/* Structure Definitions         */
/*********************************/

#define CALG_DES40  CALG_CYLINK_MEK

typedef struct _dhSharedNumber {
	BYTE            *shared;
	DWORD           len;
} dhSharedNumber;

/*********************************/
/* Function Definitions          */
/*********************************/

/*
 -	CPAcquireContext
 -
 *	Purpose:
 *               The CPAcquireContext function is used to acquire a context
 *               handle to a cryptographic service provider (CSP).
 *
 *
 *	Parameters:
 *               OUT phProv         -  Handle to a CSP
 *               IN  pszContainer   -  Pointer to a string which is the
 *                                     identity of the logged on user
 *               IN  dwFlags        -  Flags values
 *               IN  pVTable        -  Pointer to table of function pointers
 *
 *	Returns:
 */
BOOL CPAcquireContext(OUT HCRYPTPROV *phProv,
                      IN CHAR *pszContainer,
                      IN DWORD dwFlags,
                      IN PVTableProvStruc pVTable);

/*
 -      CPReleaseContext
 -
 *      Purpose:
 *               The CPReleaseContext function is used to release a
 *               context created by CryptAcquireContext.
 *
 *     Parameters:
 *               IN  phProv        -  Handle to a CSP
 *               IN  dwFlags       -  Flags values
 *
 *	Returns:
 */
BOOL CPReleaseContext(IN HCRYPTPROV hProv,
                      IN DWORD dwFlags);


/*
 -	CPGenKey
 -
 *	Purpose:
 *                Generate cryptographic keys
 *
 *
 *	Parameters:
 *               IN      hProv   -  Handle to a CSP
 *               IN      Algid   -  Algorithm identifier
 *               IN      dwFlags -  Flags values
 *               OUT     phKey   -  Handle to a generated key
 *
 *	Returns:
 */
BOOL CPGenKey(IN HCRYPTPROV hProv,
              IN ALG_ID Algid,
              IN DWORD dwFlags,
              OUT HCRYPTKEY * phKey);


/*
 -	CPDeriveKey
 -
 *	Purpose:
 *                Derive cryptographic keys from base data
 *
 *
 *	Parameters:
 *               IN      hProv      -  Handle to a CSP
 *               IN      Algid      -  Algorithm identifier
 *               IN      hBaseData -   Handle to base data
 *               IN      dwFlags    -  Flags values
 *               OUT     phKey      -  Handle to a generated key
 *
 *	Returns:
 */
BOOL CPDeriveKey(IN HCRYPTPROV hProv,
                 IN ALG_ID Algid,
                 IN HCRYPTHASH hBaseData,
                 IN DWORD dwFlags,
                 OUT HCRYPTKEY * phKey);


/*
 -	CPDestroyKey
 -
 *	Purpose:
 *                Destroys the cryptographic key that is being referenced
 *                with the hKey parameter
 *
 *
 *	Parameters:
 *               IN      hProv  -  Handle to a CSP
 *               IN      hKey   -  Handle to a key
 *
 *	Returns:
 */
BOOL CPDestroyKey(IN HCRYPTPROV hProv,
                  IN HCRYPTKEY hKey);



/*
 -	CPSetKeyParam
 -
 *	Purpose:
 *                Allows applications to customize various aspects of the
 *                operations of a key
 *
 *	Parameters:
 *               IN      hProv   -  Handle to a CSP
 *               IN      hKey    -  Handle to a key
 *               IN      dwParam -  Parameter number
 *               IN      pbData  -  Pointer to data
 *               IN      dwFlags -  Flags values
 *
 *	Returns:
 */
BOOL CPSetKeyParam(IN HCRYPTPROV hProv,
                   IN HCRYPTKEY hKey,
                   IN DWORD dwParam,
                   IN BYTE *pbData,
                   IN DWORD dwFlags);



/*
 -	CPGetKeyParam
 -
 *	Purpose:
 *                Allows applications to get various aspects of the
 *                operations of a key
 *
 *	Parameters:
 *               IN      hProv      -  Handle to a CSP
 *               IN      hKey       -  Handle to a key
 *               IN      dwParam    -  Parameter number
 *               OUT     pbData     -  Pointer to data
 *               IN      pdwDataLen -  Length of parameter data
 *               IN      dwFlags    -  Flags values
 *
 *	Returns:
 */
BOOL CPGetKeyParam(IN HCRYPTPROV hProv,
                   IN HCRYPTKEY hKey,
                   IN DWORD dwParam,
                   OUT BYTE *pbData,
                   IN DWORD *pdwDataLen,
                   IN DWORD dwFlags);


/*
 -	CPSetProvParam
 -
 *	Purpose:
 *                Allows applications to customize various aspects of the
 *                operations of a provider
 *
 *	Parameters:
 *               IN      hProv   -  Handle to a CSP
 *               IN      dwParam -  Parameter number
 *               IN      pbData  -  Pointer to data
 *               IN      dwFlags -  Flags values
 *
 *	Returns:
 */
BOOL CPSetProvParam(IN HCRYPTPROV hProv,
                    IN DWORD dwParam,
                    IN BYTE *pbData,
                    IN DWORD dwFlags);



/*
 -	CPGetProvParam
 -
 *	Purpose:
 *                Allows applications to get various aspects of the
 *                operations of a provider
 *
 *	Parameters:
 *               IN      hProv      -  Handle to a CSP
 *               IN      dwParam    -  Parameter number
 *               OUT     pbData     -  Pointer to data
 *               IN OUT  pdwDataLen -  Length of parameter data
 *               IN      dwFlags    -  Flags values
 *
 *	Returns:
 */
BOOL CPGetProvParam(IN HCRYPTPROV hProv,
                    IN DWORD dwParam,
                    OUT BYTE *pbData,
                    IN OUT DWORD *pdwDataLen,
                    IN DWORD dwFlags);


/*
 -	CPSetHashParam
 -
 *	Purpose:
 *                Allows applications to customize various aspects of the
 *                operations of a hash
 *
 *	Parameters:
 *               IN      hProv   -  Handle to a CSP
 *               IN      hHash   -  Handle to a hash
 *               IN      dwParam -  Parameter number
 *               IN      pbData  -  Pointer to data
 *               IN      dwFlags -  Flags values
 *
 *	Returns:
 */
BOOL CPSetHashParam(IN HCRYPTPROV hProv,
                    IN HCRYPTHASH hHash,
                    IN DWORD dwParam,
                    IN BYTE *pbData,
                    IN DWORD dwFlags);



/*
 -	CPGetHashParam
 -
 *	Purpose:
 *                Allows applications to get various aspects of the
 *                operations of a hash
 *
 *	Parameters:
 *               IN      hProv      -  Handle to a CSP
 *               IN      hHash      -  Handle to a hash
 *               IN      dwParam    -  Parameter number
 *               OUT     pbData     -  Pointer to data
 *               IN      pdwDataLen -  Length of parameter data
 *               IN      dwFlags    -  Flags values
 *
 *	Returns:
 */
BOOL CPGetHashParam(IN HCRYPTPROV hProv,
                    IN HCRYPTHASH hHash,
                    IN DWORD dwParam,
                    OUT BYTE *pbData,
                    IN DWORD *pdwDataLen,
                    IN DWORD dwFlags);



/*
 -	CPExportKey
 -
 *	Purpose:
 *                Export cryptographic keys out of a CSP in a secure manner
 *
 *
 *	Parameters:
 *               IN  hProv         - Handle to the CSP user
 *               IN  hKey          - Handle to the key to export
 *               IN  hPubKey       - Handle to exchange public key value of
 *                                   the destination user
 *               IN  dwBlobType    - Type of key blob to be exported
 *               IN  dwFlags       - Flags values
 *               OUT pbData        -     Key blob data
 *               IN OUT pdwDataLen - Length of key blob in bytes
 *
 *	Returns:
 */
BOOL CPExportKey(IN HCRYPTPROV hProv,
                 IN HCRYPTKEY hKey,
                 IN HCRYPTKEY hPubKey,
                 IN DWORD dwBlobType,
                 IN DWORD dwFlags,
                 OUT BYTE *pbData,
                 IN OUT DWORD *pdwDataLen);



/*
 -	CPImportKey
 -
 *	Purpose:
 *                Import cryptographic keys
 *
 *
 *	Parameters:
 *               IN  hProv     -  Handle to the CSP user
 *               IN  pbData    -  Key blob data
 *               IN  dwDataLen -  Length of the key blob data
 *               IN  hPubKey   -  Handle to the exchange public key value of
 *                                the destination user
 *               IN  dwFlags   -  Flags values
 *               OUT phKey     -  Pointer to the handle to the key which was
 *                                Imported
 *
 *	Returns:
 */
BOOL CPImportKey(IN HCRYPTPROV hProv,
                 IN CONST BYTE *pbData,
                 IN DWORD dwDataLen,
                 IN HCRYPTKEY hPubKey,
                 IN DWORD dwFlags,
                 OUT HCRYPTKEY *phKey);



/*
 -	CPEncrypt
 -
 *	Purpose:
 *                Encrypt data
 *
 *
 *	Parameters:
 *               IN  hProv         -  Handle to the CSP user
 *               IN  hKey          -  Handle to the key
 *               IN  hHash         -  Optional handle to a hash
 *               IN  Final         -  Boolean indicating if this is the final
 *                                    block of plaintext
 *               IN  dwFlags       -  Flags values
 *               IN OUT pbData     -  Data to be encrypted
 *               IN OUT pdwDataLen -  Pointer to the length of the data to be
 *                                    encrypted
 *               IN dwBufLen       -  Size of Data buffer
 *
 *	Returns:
 */
BOOL CPEncrypt(IN HCRYPTPROV hProv,
               IN HCRYPTKEY hKey,
               IN HCRYPTHASH hHash,
               IN BOOL Final,
               IN DWORD dwFlags,
               IN OUT BYTE *pbData,
               IN OUT DWORD *pdwDataLen,
               IN DWORD dwBufLen);



/*
 -	CPDecrypt
 -
 *	Purpose:
 *                Decrypt data
 *
 *
 *	Parameters:
 *               IN  hProv         -  Handle to the CSP user
 *               IN  hKey          -  Handle to the key
 *               IN  hHash         -  Optional handle to a hash
 *               IN  Final         -  Boolean indicating if this is the final
 *                                    block of ciphertext
 *               IN  dwFlags       -  Flags values
 *               IN OUT pbData     -  Data to be decrypted
 *               IN OUT pdwDataLen -  Pointer to the length of the data to be
 *                                    decrypted
 *
 *	Returns:
 */
BOOL CPDecrypt(IN HCRYPTPROV hProv,
               IN HCRYPTKEY hKey,
               IN HCRYPTHASH hHash,
               IN BOOL Final,
               IN DWORD dwFlags,
               IN OUT BYTE *pbData,
               IN OUT DWORD *pdwDataLen);


/*
 -	CPCreateHash
 -
 *	Purpose:
 *                initate the hashing of a stream of data
 *
 *
 *	Parameters:
 *               IN  hUID    -  Handle to the user identifcation
 *               IN  Algid   -  Algorithm identifier of the hash algorithm
 *                              to be used
 *               IN  hKey   -   Optional handle to a key
 *               IN  dwFlags -  Flags values
 *               OUT pHash   -  Handle to hash object
 *
 *	Returns:
 */
BOOL CPCreateHash(IN HCRYPTPROV hProv,
                  IN ALG_ID Algid,
                  IN HCRYPTKEY hKey,
                  IN DWORD dwFlags,
                  OUT HCRYPTHASH *phHash);



/*
 -	CPHashData
 -
 *	Purpose:
 *                Compute the cryptograghic hash on a stream of data
 *
 *
 *	Parameters:
 *               IN  hProv     -  Handle to the user identifcation
 *               IN  hHash     -  Handle to hash object
 *               IN  pbData    -  Pointer to data to be hashed
 *               IN  dwDataLen -  Length of the data to be hashed
 *               IN  dwFlags   -  Flags values
 *
 *	Returns:
 */
BOOL CPHashData(IN  HCRYPTPROV hProv,
                IN  HCRYPTHASH hHash,
                IN  CONST BYTE *pbData,
                IN  DWORD dwDataLen,
                IN  DWORD dwFlags);


/*
 -	CPHashSessionKey
 -
 *	Purpose:
 *                Compute the cryptograghic hash on a key object.
 *
 *
 *	Parameters:
 *               IN  hProv     -  Handle to the user identifcation
 *               IN  hHash     -  Handle to hash object
 *               IN  hKey      -  Handle to a key object
 *               IN  dwFlags   -  Flags values
 *
 *	Returns:
 *               CRYPT_FAILED
 *               CRYPT_SUCCEED
 */
BOOL CPHashSessionKey(IN HCRYPTPROV hProv,
                      IN HCRYPTHASH hHash,
                      IN  HCRYPTKEY hKey,
                      IN DWORD dwFlags);


/*
 -	CPDestroyHash
 -
 *	Purpose:
 *                Destroy the hash object
 *
 *
 *	Parameters:
 *               IN  hProv     -  Handle to the user identifcation
 *               IN  hHash     -  Handle to hash object
 *
 *	Returns:
 */
BOOL CPDestroyHash(IN HCRYPTPROV hProv,
                   IN HCRYPTHASH hHash);



/*
 -	CPSignHash
 -
 *	Purpose:
 *                Create a digital signature from a hash
 *
 *
 *	Parameters:
 *               IN  hProv        -  Handle to the user identifcation
 *               IN  hHash        -  Handle to hash object
 *               IN  dwKeySpec    -  Key pair to that is used to sign with
 *               IN  sDescription -  Description of data to be signed
 *               IN  dwFlags      -  Flags values
 *               OUT pbSignature  -  Pointer to signature data
 *               IN OUT dwHashLen -  Pointer to the len of the signature data
 *
 *	Returns:
 */
BOOL CPSignHash(IN     HCRYPTPROV hProv,
                IN     HCRYPTHASH hHash,
                IN     DWORD dwKeySpec,
                IN     LPCWSTR sDescription,
                IN     DWORD dwFlags,
                OUT    BYTE *pbSignature,
                IN OUT DWORD *pdwSigLen);


/*
 -	CPVerifySignature
 -
 *	Purpose:
 *                Used to verify a signature against a hash object
 *
 *
 *	Parameters:
 *               IN  hProv        -  Handle to the user identifcation
 *               IN  hHash        -  Handle to hash object
 *               IN  pbSignture   -  Pointer to signature data
 *               IN  dwSigLen     -  Length of the signature data
 *               IN  hPubKey      -  Handle to the public key for verifying
 *                                   the signature
 *               IN  sDescription -  String describing the signed data
 *               IN  dwFlags      -  Flags values
 *
 *	Returns:
 */
BOOL CPVerifySignature(IN HCRYPTPROV hProv,
                       IN HCRYPTHASH hHash,
                       IN CONST BYTE *pbSignature,
                       IN DWORD dwSigLen,
                       IN HCRYPTKEY hPubKey,
                       IN LPCWSTR sDescription,
                       IN DWORD dwFlags);


/*
 -	CPGenRandom
 -
 *	Purpose:
 *                Used to fill a buffer with random bytes
 *
 *
 *	Parameters:
 *               IN  hProv         -  Handle to the user identifcation
 *               IN  dwLen         -  Number of bytes of random data requested
 *               IN OUT pbBuffer   -  Pointer to the buffer where the random
 *                                    bytes are to be placed
 *
 *	Returns:
 */
BOOL CPGenRandom(IN HCRYPTPROV hProv,
                 IN DWORD dwLen,
                 IN OUT BYTE *pbBuffer);


/*
 -	CPGetUserKey
 -
 *	Purpose:
 *                Gets a handle to a permanent user key
 *
 *
 *	Parameters:
 *               IN  hProv      -  Handle to the user identifcation
 *               IN  dwKeySpec  -  Specification of the key to retrieve
 *               OUT phUserKey  -  Pointer to key handle of retrieved key
 *
 *	Returns:
 */
BOOL CPGetUserKey(IN HCRYPTPROV hProv,
                  IN DWORD dwKeySpec,
                  OUT HCRYPTKEY *phUserKey);


