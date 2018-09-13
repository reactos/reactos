/////////////////////////////////////////////////////////////////////////////
//  FILE          : swnt_pk.h                                              //
//  DESCRIPTION   :                                                        //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      Apr 19 1995 larrys  Cleanup                                        //
//  	Oct 27 1995 rajeshk  RandSeed Stuff added hUID to PKCS2Encrypt     //
//                                                                         //
//  Copyright (C) 1993 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef __SWNT_PK_H__
#define __SWNT_PK_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NTPK_USE_SIG    0
#define NTPK_USE_EXCH   1


#define PKCS_BLOCKTYPE_1        1
#define PKCS_BLOCKTYPE_2        2
        
//
// Function : EncryptAndDecryptWithRSAKey
//
// Description : This function creates a buffer and then encrypts that with
//               the passed in private key and decrypts with the passed in
//               public key.  The function is used for FIPS 140-1 compliance
//               to make sure that newly generated/imported keys work and
//               in the self test during DLL initialization.
//

DWORD EncryptAndDecryptWithRSAKey(
                                  IN BYTE *pbRSAPub,
                                  IN BYTE *pbRSAPriv,
                                  IN BOOL fSigKey,
                                  IN BOOL fEncryptCheck
                                  );

BOOL ReGenKey(HCRYPTPROV hUser,
              DWORD dwFlags,
              DWORD dwWhichKey,
              HCRYPTKEY *phKey,
              DWORD bits);

BOOL CheckDataLenForRSAEncrypt(
                               IN DWORD cbMod,   // length of the modulus
                               IN DWORD cbData,  // length of the data
                               IN DWORD dwFlags  // flags
                               );

// do the modular exponentiation calculation M^PubKey mod N
DWORD RSAPublicEncrypt(
                       IN PEXPO_OFFLOAD_STRUCT pOffloadInfo,
                       IN BSAFE_PUB_KEY *pBSPubKey,
                       IN BYTE *pbInput,
                       IN BYTE *pbOutput
                       );

// do the modular exponentiation calculation M^PrivKey Exponent mod N
DWORD RSAPrivateDecrypt(
                        IN PEXPO_OFFLOAD_STRUCT pOffloadInfo,
                        IN BSAFE_PRV_KEY *pBSPrivKey,
                        IN BYTE *pbInput,
                        IN BYTE *pbOutput
                        );

/************************************************************************/
/* RSAEncrypt performs a RSA encryption.                                */
/************************************************************************/
BOOL RSAEncrypt(
                IN PNTAGUserList pTmpUser,
                IN BSAFE_PUB_KEY *pBSPubKey,
                IN BYTE *pbPlaintext,
                IN DWORD cbPlaintext,
                IN BYTE *pbParams,
                IN DWORD cbParams,
                IN DWORD dwFlags,
                OUT BYTE *pbOut
                );

/************************************************************************/
/* RSADecrypt performs a RSA decryption.                                */
/************************************************************************/
BOOL RSADecrypt(
                IN PNTAGUserList pTmpUser,
                IN BSAFE_PRV_KEY *pBSPrivKey,
                IN BYTE *pbBlob,
                IN DWORD cbBlob,
                IN BYTE *pbParams,
                IN DWORD cbParams,
                IN DWORD dwFlags,
                OUT BYTE **ppbPlaintext,
                OUT DWORD *pcbPlaintext
                );

//
// Routine : DerivePublicFromPrivate
//
// Description : Derive the public RSA key from the private RSA key.  This is
//               done and the resulting public key is placed in the appropriate
//               place in the context pointer (pTmpUser).
//

BOOL DerivePublicFromPrivate(
                             IN PNTAGUserList pUser,
                             IN BOOL fSigKey
                             );

#ifdef __cplusplus
}
#endif

#endif // __SWNT_PK_H__

