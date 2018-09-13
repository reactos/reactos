/////////////////////////////////////////////////////////////////////////////
//  FILE          : scp.h                                                  //
//  DESCRIPTION   : Crypto Provider prototypes                             //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//	Jan 25 1995 larrys  Changed from Nametag                           //
//      Apr  9 1995 larrys  Removed some APIs                              //
//      Apr 19 1995 larrys  Cleanup                                        //
//      May 10 1995 larrys  added private api calls                        //
//      May 16 1995 larrys  updated to spec                                //
//      Aug 30 1995 larrys  Changed a parameter to IN OUT                  //
//      Oct 06 1995 larrys  Added more APIs                                //
//      OCt 13 1995 larrys  Removed CryptGetHashValue                      //
//                                                                         //
//  Copyright (C) 1993 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include <time.h>
#include <wincrypt.h>

#ifdef __cplusplus
extern "C" {
#endif

// type definition of a NameTag error
typedef unsigned int NTAG_ERROR;

#define	NTF_FAILED		FALSE
#define	NTF_SUCCEED		TRUE

#define	NTAG_SUCCEEDED(ntag_error)	((ntag_error) == NTF_SUCCEED)
#define	NTAG_FAILED(ntag_error)		((ntag_error) == NTF_FAILED)

#define NASCENT			0x00000002

#define	NTAG_MAXPADSIZE		8
#define	MAXSIGLEN		64

// definitions max length of logon pszUserID parameter
#define	MAXUIDLEN		64

// udp type flag
#define KEP_UDP			1

// Flags for NTagGetPubKey
#define	SIGPUBKEY		0x1000
#define	EXCHPUBKEY		0x2000

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
 -  CPDuplicateKey
 -
 *  Purpose:
 *                Duplicates the state of a key and returns a handle to it
 *
 *  Parameters:
 *               IN      hUID           -  Handle to a CSP
 *               IN      hKey           -  Handle to a key
 *               IN      pdwReserved    -  Reserved
 *               IN      dwFlags        -  Flags
 *               IN      phKey          -  Handle to the new key
 *
 *  Returns:
 */
BOOL CPDuplicateKey(IN HCRYPTPROV hUID,
                    IN HCRYPTKEY hKey,
                    IN DWORD *pdwReserved,
                    IN DWORD dwFlags,
                    IN HCRYPTKEY *phKey);

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
 -  CPDuplicateHash
 -
 *  Purpose:
 *                Duplicates the state of a hash and returns a handle to it
 *
 *  Parameters:
 *               IN      hUID           -  Handle to a CSP
 *               IN      hHash          -  Handle to a hash
 *               IN      pdwReserved    -  Reserved
 *               IN      dwFlags        -  Flags
 *               IN      phHash         -  Handle to the new hash
 *
 *  Returns:
 */
BOOL CPDuplicateHash(IN HCRYPTPROV hUID,
                    IN HCRYPTHASH hHash,
                    IN DWORD *pdwReserved,
                    IN DWORD dwFlags,
                    IN HCRYPTHASH *phHash);


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


#ifdef __cplusplus
}
#endif
