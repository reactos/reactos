#ifndef	__NT_BLOBS_H__
#define	__NT_BLOBS_H__

/* nt_blobs.h
 *
 *	Structure definitions for the NameTag keyblob formats.
 *
 *	Note: the code assumes that all structures begin with the
 *	STD_PRELUDE macro and end with the STD_POSTLUDE macro.
 *	This allows the "fill in the blanks" code to work more efficiently.
 *
 *	NTStdHeader is the data that goes before the encrypted portion of
 *	the key blob.
 *
 *	!!!!!!!!!!!!!!!ALERT!!!!!!!!!!!!!!!!!!!!!!!!!
 *	Since these structs define a net packet, we always
 *	assume Intel byte order on these structures!!!!!
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#define CUR_BLOB_VERSION	2
#define NT_HASH_BYTES	MAXHASHLEN

typedef struct _SIMPLEBLOB {
	ALG_ID	aiEncAlg;
} NTSimpleBlob;

typedef struct _STKXB {
	DWORD	dwRights;
	DWORD	dwKeyLen;
	BYTE	abHashData[NT_HASH_BYTES];
} NTKeyXBlob ;

#ifdef __cplusplus
}
#endif

#endif // __NT_BLOBS_H__

