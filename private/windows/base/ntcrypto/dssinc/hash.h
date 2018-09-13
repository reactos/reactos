/* hash.h */

#ifdef CSP_USE_MD5
#include "md5.h"
#endif
#ifdef CSP_USE_SHA1
#include "sha.h"
#endif

/*********************************/
/* Definitions                   */
/*********************************/

#define		HASH_MAGIC	0xBADE

/* State Flags */
#define		HASH_INIT		0x0001
#define		HASH_DATA		0x0002
#define		HASH_FINISH		0x0004

#define     MAX_HASH_LEN    20

#define     CRYPT_BLKLEN    8

#define     HMAC_DEFAULT_STRING_LEN     64
#define     HMAC_STARTED    1
#define     HMAC_FINISHED   2



/*********************************/
/* Structure Definitions         */
/*********************************/

typedef struct {
	int				magic;				// Magic number
    void            *pContext;          // associated context
	int				state;				// State of hash object
	ALG_ID			algId;				// Algorithm Id
	DWORD			size;				// Size of hash
    void            *pMAC;              // pointer to mac state
    BYTE            hashval[MAX_HASH_LEN];
    BYTE            *pbData;
    DWORD           cbData;
    HCRYPTKEY       hKey;
    ALG_ID          HMACAlgid;
    DWORD           HMACState;
    BYTE            *pbHMACInner;
    DWORD           cbHMACInner;
    BYTE            *pbHMACOuter;
    DWORD           cbHMACOuter;
	union {
#ifdef CSP_USE_MD5
		MD5_CTX	md5;
#endif // CSP_USE_MD5
#ifdef CSP_USE_SHA1
		A_SHA_CTX   sha;
#endif // CSP_USE_SHA1
	} algData;
} Hash_t;

/*********************************/
/* Function Definitions          */
/*********************************/

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

Hash_t *allocHash ();
void freeHash (Hash_t *hash);

DWORD feedHashData (Hash_t *hash, BYTE *data, DWORD len);
DWORD finishHash (Hash_t *hash, BYTE *pbData, DWORD *len);
DWORD getHashParams (Hash_t *hash, DWORD param, BYTE *pbData, DWORD *len);
DWORD setHashParams (Hash_t *hash, DWORD param, BYTE *pbData);

DWORD DuplicateHash(
                    Hash_t *pHash,
                    Hash_t *pNewHash
                    );
