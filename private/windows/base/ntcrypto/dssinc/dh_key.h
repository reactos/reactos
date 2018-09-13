/* dh_key.h */

#ifdef __cplusplus
extern "C" {
#endif

#define         DH_MIN_LENGTH       0x00000040     // in bytes, 64 bytes, 512 bits

#ifdef STRONG
#define         DH_MAX_LENGTH       0x00000200     // in bytes, 512 bytes, 4096 bits
#define         DH_DEFAULT_LENGTH   0x00000040     // in bytes, 64 bytes, 512 bits
#else
#define         DH_MAX_LENGTH       0x00000080     // in bytes, 128 bytes, 1024 bits
#define         DH_DEFAULT_LENGTH   0x00000040     // in bytes, 64 bytes, 512 bits
#endif // STRONG

// max in france
#define     DH_MAX_FRENCH_LENGTH    0x00000040     // in bytes, 64 bytes, 512 bits

#define DH_KEYSIZE_INC  8

/*********************************/
/* Definitions                   */
/*********************************/
#define DH_PUBLIC_MAGIC         0x31484400
#define DH_PRIVATE_MAGIC        0x32484400
#define DH_PUBLIC_MAGIC_VER3    0x33484400
#define DH_PRIV_MAGIC_VER3      0x34484400

/*********************************/
/* Structure Definitions         */
/*********************************/

typedef dsa_private_t DHKey_t; // use a DSA key since X 9.42 requires key
                               // gen like DSA

/*
typedef struct {
    ALG_ID      Algid;          // algorithm type of the key (SF or EPHEM)
    DH_PRIV_KEY Priv;
} DHKey_t;
*/

/*********************************/
/* Function Definitions          */
/*********************************/

// Initialize DH key
DWORD initKeyDH (
                 IN Context_t *pContext,
                 IN OUT DHKey_t *pDH,
                 IN ALG_ID AlgId,
                 IN DWORD dwFlags,
                 IN BOOL fAnyLength
                 );

DHKey_t *allocDHKey ();
void freeKeyDH (DHKey_t *dh);

// Get the DH parameters
DWORD getDHParams (
                   IN DHKey_t *dh,
                   IN DWORD param,
                   OUT BYTE *data,
                   OUT DWORD *len
                   );

// Set the DH parameters
DWORD setDHParams (
                   IN OUT DHKey_t *pDH,
                   IN DWORD dwParam,
                   IN BYTE *pbData,
                   IN OUT Context_t *pContext
                   );

// Generate a dh key
DWORD dhGenerateKey (
                     IN Context_t *pContext,
                     IN OUT DHKey_t *pDH,
                     IN DWORD dwFlags
                     );

DWORD dhDeriveKey (DHKey_t *dh, BYTE *data, DWORD len);

// Export the DH key in blob format
DWORD exportDHKey (
                   IN Context_t *pContext,
                   IN DHKey_t *pDH,
                   IN ALG_ID Algid,
                   IN DWORD dwFlags,
                   IN DWORD dwReserved,
                   IN DWORD dwBlobType,
                   OUT BYTE *pbData,
                   OUT DWORD *pcbData,
                   IN BOOL fInternal
                   );

DWORD DHPrivBlobToKey(
                      IN Context_t *pContext,
                      IN BLOBHEADER *pBlob,
                      IN DWORD cbBlob,
                      IN DWORD dwKeysetType,
                      IN BOOL fInternal,
                      OUT Key_t *pPrivKey
                      );

// Import the blob into DH key
DWORD importDHKey(
                  IN OUT Key_t *pPrivKey,
                  IN Context_t *pContext,
                  IN BYTE *pbBlob,
                  IN DWORD cbBlob,
                  OUT Key_t *pKey,
                  IN DWORD dwFlags,
                  IN DWORD dwKeysetType,
                  IN BOOL fInternal
                  );

void copyDHPubKey(
                  IN DHKey_t *pDH1,
                  IN DHKey_t *pDH2
                  );

DWORD copyDHKey(
                IN DHKey_t *pDH1,
                IN DHKey_t *pDH2,
                IN ALG_ID Algid,
                IN Context_t *pContext
                );

//
// Function : UseDHKey
//
// Description : This function creates an ephemeral DH key and then generates
//               two agreed keys, thus simulating a DH exchange.  If the
//               agreed keys are not the same then the function fails.
//

DWORD UseDHKey(
               IN Context_t *pContext,
               IN PEXPO_OFFLOAD_STRUCT pOffloadInfo,
               IN DHKey_t *pDH
               );


#ifdef __cplusplus
}
#endif

