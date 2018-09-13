//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       mssip32.h
//
//  Contents:   Microsoft SIP Provider
//
//  Functions:  DllMain
//
//  History:    14-Feb-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef MSSIP32_H
#define MSSIP32_H

#define     MSSIP_ID_NONE                   0       // file only types
#define     MSSIP_ID_PE                     1
#define     MSSIP_ID_JAVA                   2
#define     MSSIP_ID_CAB                    3
#define     MSSIP_ID_FLAT                   4
#define     MSSIP_ID_CATALOG                5
#define     MSSIP_ID_CTL                    6
#define     MSSIP_ID_SS                     7

#define     MSSIP_V1ID_BEGIN                200
#define     MSSIP_V1ID_PE                   201
#define     MSSIP_V1ID_PE_EX                202
#define     MSSIP_V1ID_END                  299

#define     MSSIP_SUBJECT_FORM_FILE         1
#define     MSSIP_SUBJECT_FORM_FILEANDDISP  2

#define     MSSIP_CURRENT_VERSION           0x00000300

#define     OFFSETOF(t,f)                   ((DWORD)((DWORD_PTR)&((t*)0)->f))
#define     OBSOLETE_TEXT_W                 L"<<<Obsolete>>>"   // valid since 2/14/1997

#define     HASH_CACHE_LEN                  128

typedef void *HSPCDIGESTDATA;

typedef struct DIGEST_DATA
{
    HCRYPTHASH  hHash;
    DWORD       cbCache;
    BYTE        pbCache[HASH_CACHE_LEN];
    DWORD       dwAlgId;
    void        *pvSHA1orMD5Ctx;

} DIGEST_DATA, *PDIGEST_DATA;

extern BOOL WINAPI DigestFileData(  IN HSPCDIGESTDATA hDigestData,
                                    IN const BYTE *pbData,
                                    IN DWORD cbData);

extern void SipDestroyHash(DIGEST_DATA *psDigestData);
extern BYTE *SipGetHashValue(DIGEST_DATA *psDigestData, DWORD *pcbHash);
extern BOOL SipHashData(DIGEST_DATA *psDigestData, BYTE *pbData, DWORD cbData);
extern BOOL SipCreateHash(HCRYPTPROV hProv, DIGEST_DATA *psDigestData);

extern void CryptSIPGetRegWorkingFlags(DWORD *pdwState);


#endif // MSSIP32_H
