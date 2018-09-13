/////////////////////////////////////////////////////////////////////////////
//  FILE          : schderiv.h                                             //
//  DESCRIPTION   :                                                        //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      Oct  9 1997 jeffspel  Create                                       //
//                                                                         //
//  Copyright (C) 1993 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef __SCHDERIV_H__
#define __SCHDERIV_H__

#ifdef __cplusplus
extern "C" {
#endif

#define     RC_KEYLEN           16
#define     MAX_PREMASTER_LEN   512  // BUGBUG - DH key must be <= 4096 bits
#define     MAX_RANDOM_LEN      256

#define     TLS_MASTER_LEN   48
#define     SSL3_MASTER_LEN   48

// definition of a Secure Channel hash structure
typedef struct _SCH_KeyData
{
    BYTE        rgbPremaster[MAX_PREMASTER_LEN];
    DWORD       cbPremaster;
    BYTE        rgbClientRandom[MAX_RANDOM_LEN];
    DWORD       cbClientRandom;
    BYTE        rgbServerRandom[MAX_RANDOM_LEN];
    DWORD       cbServerRandom;
    ALG_ID      EncAlgid;
    ALG_ID      HashAlgid;
    DWORD       cbEnc;
    DWORD       cbEncMac;
    DWORD       cbHash;
    DWORD       cbIV;
    BOOL        fFinished;
    BOOL        dwFlags;
} SCH_KEY, *PSCH_KEY;

// definition of a Secure Channel hash structure
typedef struct _SCH_HashData
{
    ALG_ID      ProtocolAlgid;
    ALG_ID      EncAlgid;
    ALG_ID      HashAlgid;
    DWORD       cbEnc;
    DWORD       cbEncMac;
    DWORD       cbHash;
    DWORD       cbIV;
    BYTE        rgbClientRandom[MAX_RANDOM_LEN];
    DWORD       cbClientRandom;
    BYTE        rgbServerRandom[MAX_RANDOM_LEN];
    DWORD       cbServerRandom;
	BYTE        rgbFinal[MAX_RANDOM_LEN];
    DWORD       cbFinal;
    BOOL        dwFlags;
} SCH_HASH, *PSCH_HASH;

// definition of a TLS1 PRF hash structure
typedef struct _PRF_HashData
{
    BYTE        rgbLabel[MAX_RANDOM_LEN];
    DWORD       cbLabel;
    BYTE        rgbSeed[MD5DIGESTLEN + A_SHA_DIGEST_LEN];
    DWORD       cbSeed;
	BYTE        rgbMasterKey[TLS_MASTER_LEN];
} PRF_HASH;

BOOL SCHSetKeyParam(
                    IN Context_t *pContext,
                    IN Key_t *pKey,
                    IN DWORD dwParam,
                    IN PBYTE pbData
                    );

BOOL SCHGetKeyParam(
                    Key_t *pKey,
                    DWORD dwParam,
                    PBYTE pbData
                    );

BOOL SChGenMasterKey(
                     Key_t *pKey,
                     PSCH_HASH pSChHash
                     );

BOOL SecureChannelDeriveKey(
                            Hash_t *pHash,
                            ALG_ID Algid,
                            DWORD dwFlags,
                            HCRYPTKEY *phKey
                            );

BOOL SetPRFHashParam(
                     PRF_HASH *pPRFHash,
                     DWORD dwParam,
                     BYTE *pbData
                     );

BOOL CalculatePRF(
                  PRF_HASH *pPRFHash,
                  BYTE *pbData,
                  DWORD *pcbData
                  );

#ifdef __cplusplus
}
#endif

#endif // __SCHDERIV_H__
