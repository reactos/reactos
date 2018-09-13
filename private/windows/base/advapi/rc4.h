#ifdef __cplusplus
extern "C" {
#endif

#define RC4_KEYSIZE	5
	
/* Key structure */
typedef struct RC4_KEYSTRUCT
{
  unsigned char	S[256];		/* State table */
  unsigned char	i,j;		/* Indices */
} RC4_KEYSTRUCT;

/* rc4_key()
 *
 * Generate the key control structure.  Key can be any size.
 *
 * Parameters:
 *   Key		A KEYSTRUCT structure that will be initialized.
 *   dwLen		Size of the key, in bytes.
 *   pbKey		Pointer to the key.
 *
 * MTS: Assumes pKS is locked against simultaneous use.
 */
void rc4_key(struct RC4_KEYSTRUCT *pKS, DWORD dwLen, BYTE *pbKey);

/* rc4()
 *
 * Performs the actual encryption
 *
 * Parameters:
 *
 *   pKS		Pointer to the KEYSTRUCT created using rc4_key().
 *   dwLen		Size of buffer, in bytes.
 *   pbuf		Buffer to be encrypted.
 *
 * MTS: Assumes pKS is locked against simultaneous use.
 */
void rc4(struct RC4_KEYSTRUCT *pKS, DWORD dwLen, BYTE *pbuf);

#ifdef __cplusplus
}
#endif

