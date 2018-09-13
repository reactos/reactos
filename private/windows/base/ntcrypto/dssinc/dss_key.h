#ifdef __cplusplus
extern "C" {
#endif

/* dss_key.h */

#define         DSS_MIN_LENGTH       64      // 512 bit
#define         DSS_MAX_LENGTH       128     // 1024 bit
#define         DSS_DEFAULT_LENGTH   128     // 1024 bit

#define DSS_KEYSIZE_INC     64

/*********************************/
/* Definitions                   */
/*********************************/
#define     DSS_MAGIC           0x31535344
#define     DSS_PRIVATE_MAGIC   0x32535344
#define     DSS_PUB_MAGIC_VER3  0x33535344
#define     DSS_PRIV_MAGIC_VER3 0x34535344


/*********************************/
/* Structure Definitions         */
/*********************************/

typedef dsa_private_t DSSKey_t;

/*********************************/
/* Function Definitions          */
/*********************************/

DSSKey_t *allocDSSKey ();
void freeKeyDSS (DSSKey_t *dss);

DWORD initKeyDSS (
                  IN OUT DSSKey_t *dss,
                  IN DWORD dwBitLen
                  );

// Generate the DSS keys
DWORD genDSSKeys (
                  IN Context_t *pContext,
                  IN OUT DSSKey_t *pDss,
                  IN BYTE *pbRandom,
                  IN DWORD dwFlags
                  );

void copyDSSPubKey(
                    IN DSSKey_t *dss1,
                    IN DSSKey_t *dss2
                    );

void copyDSSKey(
                IN DSSKey_t *dss1,
                IN DSSKey_t *dss2
                );

DWORD getDSSParams (DSSKey_t *dss, DWORD param, BYTE *data, DWORD *len);

DWORD setDSSParams (
                    IN Context_t *pContext,
                    IN OUT DSSKey_t *pDss,
                    IN DWORD dwParam,
                    IN BYTE *pbData
                    );

BOOL DSSValueExists(
                    IN DWORD *pdw,
                    IN DWORD cdw,
                    OUT DWORD *pcb
                    );

DWORD ExportDSSPrivBlob3(
                         IN Context_t *pContext,
                         IN DSSKey_t *pDSS,
                         IN DWORD dwMagic,
                         IN ALG_ID Algid,
                         IN BOOL fInternalExport,
                         IN BOOL fSigKey,
                         OUT BYTE *pbKeyBlob,
                         IN OUT DWORD *pcbKeyBlob
                         );

DWORD ImportDSSPrivBlob3(
                         IN BOOL fInternalExport,
                         IN BYTE *pbKeyBlob,
                         IN DWORD cbKeyBlob,
                         OUT DSSKey_t *pDSS
                         );

DWORD ExportDSSPubBlob3(
                        IN Context_t *pContext,
                        IN DSSKey_t *pDSS,
                        IN DWORD dwMagic,
                        IN ALG_ID Algid,
                        OUT BYTE *pbKeyBlob,
                        IN OUT DWORD *pcbKeyBlob
                        );

DWORD ImportDSSPubBlob3(
                        IN BYTE *pbKeyBlob,
                        IN DWORD cbKeyBlob,
                        IN BOOL fYIncluded,
                        OUT DSSKey_t *pDSS
                        );

// Export DSS key into blob format
DWORD exportDSSKey (
                    IN Context_t *pContext,
                    IN DSSKey_t *pDSS,
                    IN DWORD dwFlags,
                    IN DWORD dwBlobType,
                    IN BYTE *pbKeyBlob,
                    IN DWORD *pcbKeyBlob,
                    IN BOOL fInternalExport
                    );

// Import the blob into DSS key
DWORD importDSSKey (
                    IN Context_t *pContext,
                    IN Key_t *pKey,
                    IN BYTE *pbKeyBlob,
                    IN DWORD cbKeyBlob,
                    IN DWORD dwFlags,
                    IN DWORD dwKeysetType,
                    IN BOOL fInternal
                    );

DWORD dssGenerateSignature (
                            Context_t *pContext,
                            DSSKey_t *pDss,
                            BYTE *pbHash,
                            DWORD cbHash,
                            BYTE *pbSig,
                            DWORD *pcbSig,
                            BYTE *pbRandom
                            );

//
// Function : SignAndVerifyWithKey
//
// Description : This function creates a hash and then signs that hash with
//               the passed in key and verifies the signature.  The function
//               is used for FIPS 140-1 compliance to make sure that newly
//               generated/imported keys work and in the self test during
//               DLL initialization.
//

DWORD SignAndVerifyWithKey(
                           IN DSSKey_t *pDss,
                           IN EXPO_OFFLOAD_STRUCT *pOffloadInfo,
                           IN HANDLE hRNGDriver,
                           IN BYTE *pbData,
                           IN DWORD cbData
                           );

#ifdef __cplusplus
}
#endif
