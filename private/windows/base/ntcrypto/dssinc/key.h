/* key.h */

#ifdef __cplusplus
extern "C" {
#endif

// needs to be put into wincrypt.h or left open in wincrypt.h
#define     KP_Z                    30
#define     IPSEC_FLAG_CHECK        0xF42A19B6

static BYTE rgbSymmetricKeyWrapIV[8] = {0x4a, 0xdd, 0xa2, 0x2c, 0x79, 0xe8, 0x21, 0x05};

/*********************************/
/* Function Definitions          */
/*********************************/

void UnpickleKey(
                 ALG_ID Algid,
                 BYTE *pbData,
                 DWORD cbData,
                 BOOL *pfExportable,
	             Key_t *pKey
                 );

Key_t *allocKey ();

// Delete a key
void freeKey (
              IN OUT Key_t *key
              );

// Copy a public key
void CopyPubKey (
                 IN Key_t *pKeyIn,
                 OUT Key_t *pKeyOut
                 );

// Initialize a key
DWORD initKey (
               IN OUT Key_t *key,
               IN Context_t *pContext,
               IN ALG_ID algId,
               IN DWORD dwFlags
               );

BOOL checkKey (Key_t *key);

// Derive key
// if the pHash parameter is non zero and the key to be derived is a
// 3 Key triple DES key, then the data is expanded to the appropriate key size
DWORD deriveKey (
                 Key_t *pKey,
                 Context_t *pContext,
                 BYTE *pbData,
                 DWORD cbData,
                 DWORD dwFlags,
                 Hash_t *pHash,
                 BOOL fGenKey,
                 BOOL fAnySizeRC2
                 );

// generate a key
DWORD generateKey (
                   IN OUT Key_t *pKey,
                   IN DWORD dwFlags,
                   IN OUT uchar *pbRandom,
                   IN DWORD cbRandom,
                   IN Context_t *pContext
                   );

// duplicate a key
DWORD DuplicateKey(
                   Context_t *pContext,
                   Key_t *pKey,
                   Key_t *pNewKey,
                   BOOL fCopyContext
                   );

// set the parameters on a key
DWORD setKeyParams (
                    IN OUT Key_t *pKey,
                    IN DWORD dwParam,
                    IN BYTE *pbData,
                    IN OUT Context_t *pContext,
                    IN DWORD dwFlags
                    );

DWORD getKeyParams (
                    IN Context_t *pContext,
                    IN Key_t *key,
                    IN DWORD param,
                    IN DWORD dwFlags,
                    OUT BYTE *data,
                    OUT DWORD *len
                    );

DWORD ImportOpaqueBlob(
	                  Context_t	*pContext,
                      DWORD dwFlags,
                      CONST BYTE *pbData,
                      DWORD cbData,
                      HCRYPTKEY *phKey
                      );

// Export the requested key into blob format
DWORD exportKey(
                IN Context_t *pContext,
                IN Key_t *pKey,
                IN Key_t *pEncKey,
                IN DWORD dwBlobType,
                IN DWORD dwFlags,
                OUT BYTE *pbBlob,
                OUT DWORD *pcbBlob,
                IN BOOL fInternalExport
                );

DWORD feedPlainText (Key_t *pKey, BYTE *pbData, DWORD dwBufLen, DWORD *pdwDataLen,int final);
DWORD feedCypherText (Key_t *pKey, BYTE *pbData, DWORD *pdwDataLen, int final);

DWORD generateSignature (
                         IN Context_t *pContext,
                         IN Key_t *key,
                         IN uchar *hashVal,
                         IN DWORD hashLen,
                         OUT uchar *pbSignature,
                         OUT DWORD *pdwSigLen,
                         IN OUT uchar *random
                         );

// Verify signature
DWORD verifySignature(
                      IN Context_t *pContext,
                      IN Key_t *pKey,
                      IN uchar *pbHash,
                      IN DWORD cbHash,
                      IN uchar *pbSignature,
                      IN DWORD cbSignature
                      );

BOOL BlockEncrypt(void EncFun(BYTE *In, BYTE *Out, void *key, int op),
                  Key_t *pKey,
                  int BlockLen,
                  BOOL Final,
                  BYTE  *pbData,
                  DWORD *pdwDataLen,
                  DWORD dwBufLen);

BOOL BlockDecrypt(void DecFun(BYTE *In, BYTE *Out, void *key, int op),
                  Key_t *pKey,
                  int BlockLen,
                  BOOL Final,
                  BYTE  *pbData,
                  DWORD *pdwDataLen);

//
// Function : TestSymmetricAlgorithm
//
// Description : This function expands the passed in key buffer for the appropriate algorithm,
//               encrypts the plaintext buffer with the same algorithm and key, and the
//               compares the passed in expected ciphertext with the calculated ciphertext
//               to make sure they are the same.  The function only uses ECB mode for
//               block ciphers and the plaintext buffer must be the same length as the
//               ciphertext buffer.  The length of the plaintext must be either the
//               the block length of the cipher if it is a block cipher or less
//               than MAX_BLOCKLEN if a stream cipher is being used.
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

/*
 -  GetRC4KeyForSymWrap
 -
 *  Purpose:
 *            RC4 or more precisely stream ciphers are not supported by the CMS spec
 *            on symmetric key wrapping so we had to do something proprietary since
 *            we want to support RC4 for applications other than SMIME
 *
 *
 *  Parameters:
 *               IN  pContext   - Pointer to the context
 *               IN  pbSalt     - Pointer to the 8 byte salt buffer
 *               IN  pKey       - Pointer to the orignial key
 *               OUT ppNewKey   - Pointer to a pointer to the new key
 */
DWORD GetRC4KeyForSymWrap(
                          IN Context_t *pContext,
                          IN BYTE *pbSalt,
                          IN Key_t *pKey,
                          OUT Key_t **ppNewKey
                          );

/*
 -  GetSymmetricKeyChecksum
 -
 *  Purpose:
 *                Calculates the checksum for a symmetric key which is to be
 *                wrapped with another symmetric key.  This should meet the
 *                CMS specification
 *
 *
 *  Parameters:
 *               IN  pKey       - Pointer to the key
 *               OUT pbChecksum - Pointer to the 8 byte checksum
 */
void GetSymmetricKeyChecksum(
                             IN BYTE *pbKey,
                             IN DWORD cbKey,
                             OUT BYTE *pbChecksum
                             );

// check for symmetric wrapping support
#define UnsupportedSymKey(pKey) ((CALG_RC4 != pKey->algId) && \
                                 (CALG_RC2 != pKey->algId) && \
                                 (CALG_DES != pKey->algId) && \
                                 (CALG_CYLINK_MEK != pKey->algId) && \
                                 (CALG_3DES != pKey->algId) && \
                                 (CALG_3DES_112 != pKey->algId))

#ifdef __cplusplus
}
#endif
