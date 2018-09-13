/* rsa.h
 *
 *      RSA library functions.
 *
 *              
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#define RSA1 ((DWORD)'R'+((DWORD)'S'<<8)+((DWORD)'A'<<16)+((DWORD)'1'<<24))
#define RSA2 ((DWORD)'R'+((DWORD)'S'<<8)+((DWORD)'A'<<16)+((DWORD)'2'<<24))

// Key header structures.
//
//    These structs define the fixed data at the beginning of an RSA key.
//    They are followed by a variable length of data, sized by the stlen
//    field.

typedef struct {
    DWORD       magic;                  /* Should always be RSA1 */
    DWORD       keylen;                 // size of modulus buffer
    DWORD       bitlen;                 // # of bits in modulus
    DWORD       datalen;                // max number of bytes to be encoded
    DWORD       pubexp;                 //public exponent
} BSAFE_PUB_KEY, FAR *LPBSAFE_PUB_KEY;

typedef struct {
    DWORD       magic;                  /* Should always be RSA2 */
    DWORD       keylen;                 // size of modulus buffer
    DWORD       bitlen;                 // bit size of key
    DWORD       datalen;                // max number of bytes to be encoded
    DWORD       pubexp;                 // public exponent
} BSAFE_PRV_KEY, FAR *LPBSAFE_PRV_KEY;

typedef struct {
	BYTE    *modulus;
	BYTE    *prvexp;
	BYTE    *prime1;
	BYTE    *prime2;
	BYTE    *exp1;
	BYTE    *exp2;
	BYTE    *coef;
	BYTE    *invmod;
	BYTE    *invpr1;
	BYTE    *invpr2;
} BSAFE_KEY_PARTS, FAR *LPBSAFE_KEY_PARTS;

typedef const BYTE far *cLPBYTE;                // const LPBYTE resolves wrong

/* BSafeEncPublic
 *
 * BSafeEncPublic(key, part_in, part_out)
 *
 *      RSA encrypt a buffer of size key->keylen, filled with data of size
 *      key->datalen with the public key pointed to by key, returning the
 *      encrypted data in part_out.
 *
 *      Parameters
 *
 *              LPBSAFE_PUB_KEY key - points to a public key in BSAFE_KEY
 *                              format.
 *
 *              LPBYTE part_in - points to a BYTE array of size key->keylen
 *                              holding the data to be encrypted.  The
 *                              data in the buffer should be no larger
 *                              than key->datalen.  All other bytes should
 *                              be zero.
 *
 *              LPBYTE part_out - points to a BYTE array of size keylen
 *                              to receive the encrypted data.
 *
 *      Returns
 *
 *              TRUE - encryption succeeded.
 *              FALSE - encryption failed.
 *
 */

BOOL BSafeEncPublic(const LPBSAFE_PUB_KEY key,
		    cLPBYTE part_in,
		    LPBYTE part_out);


/* BSafeDecPrivate
 *
 * BSafeDecPrivate(key, part_in, part_out)
 *
 *      RSA decrypt a buffer of size keylen, containing key->datalen bytes
 *      of data with the private key pointed to by key, returning the
 *      decrypted data in part_out.
 *
 *      Parameters
 *
 *              LPBSAFE_PRV_KEY key - points to a private key in BSAFE_KEY
 *                              format.
 *
 *              LPBYTE part_in - points to a BYTE array of size key->keylen
 *                              holding the data to be decrypted.  The data
 *                              in the buffer should be no longer than
 *                              key->datalen.  All other bytes should be zero.
 *
 *              LPBYTE part_out - points to a BYTE array of size GRAINSIZE
 *                              to receive the decrypted data.
 *
 *      Returns
 *
 *              TRUE - decryption succeeded.
 *              FALSE - decryption failed.
 *
 */

BOOL BSafeDecPrivate(const LPBSAFE_PRV_KEY key,
		     cLPBYTE part_in,
		     LPBYTE part_out);

/* BSafeMakeKeyPair
 *
 * BSafeMakeKeyPair(public_key, private_key, bits)
 *
 *      Generate an RSA key pair.
 *
 *      Parameters
 *
 *              LPBSAFE_PUB_KEY public_key - points to the memory to recieve
 *                                      the public key.  This pointer must
 *                                      point to at least the number of bytes
 *                                      specified as the public key size by
 *                                      BSafeComputeKeySizes.   
 *
 *              LPBSAFE_PRV_KEY private_key - points to the memory to recieve
 *                                      the private key.  This pointer must
 *                                      point to at least the number of bytes
 *                                      specified as the private key size
 *                                      by BSafeComputeKeySizes.
 *
 *              DWORD bits - length of the requested key in bits.
 *                              This value must be even and greater than 63
 *
 *      Returns
 *
 *              TRUE - keys were successfully generated
 *              FALSE - not enough memory to generate keys
 *
 */

BOOL BSafeMakeKeyPair(LPBSAFE_PUB_KEY public_key,
		      LPBSAFE_PRV_KEY private_key,
		      DWORD bits);

/* BSafeFreePubKey
 *
 * BSafeFreePubKey(public_key)
 *
 *      Free the data associated with a public key
 *
 *      Parameters
 *
 *              LPBSAFE_PUB_KEY public_key - points to a BSAFE_PUB_KEY
 *                               structure to free.
 *
 *      Returns
 *
 *              nothing
 *
 */

void BSafeFreePubKey(LPBSAFE_PUB_KEY public_key);

/* BSafeFreePrvKey
 *
 * BSafeFreePrvKey(public_key)
 *
 *      Free the data associated with a private key
 *
 *      Parameters
 *
 *              LPBSAFE_PRV_KEY private_key - points to a BSAFE_PRV_KEY
 *                               structure to free.
 *
 *      Returns
 *
 *              nothing
 *
 */

void BSafeFreePrvKey(LPBSAFE_PRV_KEY private_key);


/* BSafeComputeKeySizes
 *
 *      BSafeComputeKeySizes(   LPDWORD PubKeySize,
 *                              LPDWORD PrivKeySize,
 *                              LPDWORD bits )
 *
 *      Computes the required memory to hold a public and private key of
 *      a specified number of bits.
 *
 *      Parameters:
 *
 *              LPDWORD PubKeySize - pointer to DWORD to return the public
 *                                   key size, in bytes.
 *
 *              LPDWORD PrivKeySize - pointer to DWORD to return the private
 *                                    key size, in bytes.
 *
 *              LPDWORD bits      - pointer to DWORD specifying number of bits
 *                                  in the RSA modulus.
 *
 *      Returns:
 *
 *              TRUE if *bits is a valid RSA modulus size.
 *              FALSE if *bits is an invalid RSA modulus size.
 *
 */

BOOL BSafeComputeKeySizes(LPDWORD PublicKeySize,
			  LPDWORD PrivateKeySize,
			  LPDWORD bits);

/* BSafeGetPrvKeyParts
 *
 * BOOL BSafeGetPrvKeyParts(    LPBSAFE_PRV_KEY key,
 *                              LPBSAFE_KEY_PARTS parts)
 *
 *      Returns pointers to the parts of a private key, and the length of
 *      the modulus in bytes.
 *
 *      Parameters:
 *
 *              LPBSAFE_PRV_KEY key     - the key to disassemble
 *              LPBSAFE_KEY_PARTS parts - the structure to fill in
 *
 *      Returns -
 *              FALSE if the key is not valid.
 */

BOOL BSafeGetPrvKeyParts(LPBSAFE_PRV_KEY key,
			  LPBSAFE_KEY_PARTS parts);


/* BSafeGetPubKeyModulus
 *
 * BYTE *BSafeGetPubKeyModulus(LPBSAFE_PUB_KEY key)
 *
 *      Returns pointer to the modulus of a public key
 *
 *      Parameters:
 *
 *              LPBSAFE_PUB_KEY key     - the key to disassemble
 *
 *      Returns -
 *
 *              Pointer to the parts, VOID on error.
 *              Fails if the key is not valid.
 */

BYTE *BSafeGetPubKeyModulus(LPBSAFE_PUB_KEY key);

#ifdef __cplusplus
}
#endif



