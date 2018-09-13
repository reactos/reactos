/////////////////////////////////////////////////////////////////////////////
//  FILE          : ntagum.h                                               //
//  DESCRIPTION   : include file                                           //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      Feb 16 1995 larrys  Fix problem for 944 build                      //
//      May 23 1997 jeffspel Added provider type checking                  //
//                                                                         //
//  Copyright (C) 1993 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////
#ifndef	__NTAGUM_H__
#define	__NTAGUM_H__

#ifdef __cplusplus
extern "C" {
#endif

// prototypes for the NameTag User Manager
BOOL WINAPI FIsWinNT(void);

BOOL NTagLogonUser (
                    char *pszUserID,
                    DWORD dwFlags,
                    void **UserInfo,
                    HCRYPTPROV *phUID,
                    DWORD dwProvType,
                    LPSTR pszProvName
                    );

BOOL LogoffUser (void *UserInfo);

BOOL ReadRegValue(
                  HKEY hLoc,
                  char *pszName,
                  BYTE **ppbData,
                  DWORD *pcbLen,
                  BOOL fAlloc
                  );

BOOL ReadKey(
             HKEY hLoc,
             char *pszName,
             BYTE **ppbData,
             DWORD *pcbLen,
             PNTAGUserList pUser,
             HCRYPTKEY hKey,
             BOOL *pfPrivKey,
             BOOL fKeyExKey,
             BOOL fLastKey
             );

BOOL SaveKey(
             HKEY hRegKey,
             CONST char *pszName,
             void *pbData,
             DWORD dwLen,
             PNTAGUserList pUser,
             BOOL fPrivKey,
             DWORD dwFlags,
             BOOL fExportable
             );

BOOL ProtectPrivKey(
                    IN OUT PNTAGUserList pTmpUser,
                    IN LPWSTR szPrompt,
                    IN DWORD dwFlags,
                    IN BOOL fSigKey
                    );

BOOL UnprotectPrivKey(
                      IN OUT PNTAGUserList pTmpUser,
                      IN LPWSTR szPrompt,
                      IN BOOL fSigKey,
                      IN BOOL fAlwaysDecrypt
                      );

BOOL RemovePublicKeyExportability(IN PNTAGUserList pUser,
                                  IN BOOL fExchange);

BOOL MakePublicKeyExportable(IN PNTAGUserList pUser,
                             IN BOOL fExchange);

BOOL CheckPublicKeyExportability(IN PNTAGUserList pUser,
                                 IN BOOL fExchange);

#ifdef __cplusplus
}
#endif


#endif // __NTAGUM_H__
