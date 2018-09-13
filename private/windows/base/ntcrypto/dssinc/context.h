/* context.h */

// definition for disabling encryption in France
#define CRYPT_DISABLE_CRYPT         0x1
#define CRYPT_IN_FRANCE             0x2

/*********************************/
/* Definitions                   */
/*********************************/

#define		KEY_MAGIC	        0xBADF

/* State definitions */
#define		KEY_INIT	0x0001

#define     MAX_BLOCKLEN    8

// types of key storage
#define PROTECTED_STORAGE_KEYS      1 
#define PROTECTION_API_KEYS         2

/*********************************/
/* Structure Definitions         */
/*********************************/

typedef struct _Key_t_ {
	int				magic;				// Magic number
    void            *pContext;
	int				state;				// State of object
	ALG_ID			algId;				// Algorithm Id
	DWORD			flags;				// General flags associated with key
	void			*algParams;			// Parameters for algorithm
	uchar           IV[MAX_BLOCKLEN];
	uchar           Temp_IV[MAX_BLOCKLEN];
	uchar           *pbKey;
    DWORD           cbKey;
	uchar           *pbSalt;
    DWORD           cbSalt;
	BYTE            *pbData;
    DWORD           cbData;
    DWORD           cbEffectiveKeyLen;
	int             mode;
	int             pad;
	int             mode_bits;
    BOOL            InProgress;         // if key is being used
    BOOL            fUIOnKey;           // flag to indicate if UI was to be set on the key
} Key_t;


/*********************************/
/* Definitions                   */
/*********************************/

#define		CONTEXT_MAGIC			0xDEADBEEF
#define		CONTEXT_RANDOM_LENGTH	20


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


/*********************************/
/* Structure Definitions         */
/*********************************/

typedef struct {
	DWORD		        magic;				    // Magic number
    DWORD               dwProvType;             // Type of provider being called as
    BOOL                fMachineKeyset;         // TRUE if keyset is for machine
	DWORD		        rights;				    // Privileges
    BOOL                fIsLocalSystem;         // check if running as local system
    KEY_CONTAINER_INFO  ContInfo;
    Key_t               *pSigKey;               // pointer to the DSS sig key
    Key_t               *pKExKey;               // pointer to the DH key exchange key
	HKEY		        hKeys;				    // Handle to registry
    DWORD               dwEnumalgs;             // index for enumerating algorithms
    DWORD               dwEnumalgsEx;           // index for enumerating algorithms
    DWORD               dwiSubKey;              // index for enumerating containers
    DWORD               dwMaxSubKey;            // max number of containers
	void		        *contextData;		    // Context specific data
    CRITICAL_SECTION    CritSec;                // critical section for decrypting keys
    HWND                hWnd;                   // handle to window for UI
    PSTORE_INFO         *pPStore;               // pointer to PStore information
    LPWSTR              pwszPrompt;             // UI prompt to be used
    DWORD               dwOldKeyFlags;          // flags to tell how keys should be migrated
    DWORD               dwKeysetType;           // type of storage used
    HANDLE              hRNGDriver;             // handle to hardware RNG driver
    CHAR                rgszMachineName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD               cbMachineName;
    EXPO_OFFLOAD_STRUCT *pOffloadInfo;          // info for offloading modular expo
} Context_t;



/*********************************/
/* Function Definitions          */
/*********************************/

void freeContext(
                 Context_t *pContext
                 );

Context_t *checkContext(
                        HCRYPTPROV hProv
                        );

Context_t *allocContext();

// Initialize a context
DWORD initContext (
                   IN OUT Context_t *pContext,
                   IN DWORD dwFlags,
                   IN DWORD dwProvType
                   );

HCRYPTPROV AddContext(
                      Context_t *pContext
                      );

HCRYPTHASH addContextHash (
                           Context_t *pContext,
                           Hash_t *pHash
                           );

Hash_t *checkContextHash (
                          Context_t *pContext,
                          HCRYPTHASH hHash
                          );

// Add key to context
HCRYPTKEY addContextKey (
                         Context_t *pContext,
                         Key_t *pKey
                         );

// Check if key exists in context
Key_t *checkContextKey (
                        IN Context_t *pContext,
                        IN HCRYPTKEY hKey
                        );

// random number generation prototype
BOOL FIPS186GenRandom(
                      IN HANDLE hRNGDriver,
                      IN BYTE **ppbContextSeed,
                      IN DWORD *pcbContextSeed,
                      IN OUT BYTE *pb,
                      IN DWORD cb
                      );

