/* des_key.h */


/*********************************/
/* Definitions                   */
/*********************************/
#define		DES_MAGIC		0x44455331

/*********************************/
/* Function Definitions          */
/*********************************/

DWORD initKeyDES (Key_t *des);
DWORD getDESParams (Key_t *des, DWORD param, BYTE *data, DWORD *len);
DWORD setDESParams (Key_t *des, DWORD param, BYTE *data);

// Get DES key length
DWORD desGetKeyLength (
                       IN ALG_ID Algid,
                       IN DWORD dwFlags,
                       OUT DWORD *pcbKey,
                       OUT DWORD *pcbData
                       );

// Derive a des key
DWORD desDeriveKey (
                   IN OUT Key_t *pKey,
                   IN BYTE *pbData,
                   IN DWORD dwFlags
                   );

