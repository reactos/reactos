/////////////////////////////////////////////////////////////////////////////
//  FILE          : cppapi.h                                               //
//  DESCRIPTION   : Cryptography Provider Private APIs                     //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      May  9 1995 larrys  New                                            //
//                                                                         //
//  Copyright (C) 1993 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////


typedef unsigned long HPRIVUID;

#define	CPPAPI_FAILED		FALSE
#define	CPPAPI_SUCCEED		TRUE


/*
 -  CryptLogonVerify
 -
 *  Purpose:
 *                Used by CryptAcquireContext to verify logon password.
 *
 *
 *  Parameters:
 *               OUT     hPrivid -  Handle to the id of the user
 *
 *  Returns:
 */
BOOL CryptLogonVerify(OUT HPRIVUID *hUID);


/*
 -  CryptGetUserData
 -
 *  Purpose:
 *                Get required data from user.
 *
 *
 *  Parameters:
 *               IN      hPrivid  -  Handle to the id of the user
 *               OUT     pbData   -  bufer containing user-supplied data
 *               OUT     dwBufLen -  lenght of user-supplied data
 *
 *  Returns:
 */
BOOL CryptGetUserData(IN  HPRIVUID hUID,
                      OUT BYTE **pbData,
                      OUT DWORD *dwBufLen);

/*
 -  CryptConfirmSignature
 -
 *  Purpose:
 *                Determine weather the signing should proceed.
 *
 *
 *  Parameters:
 *               IN      pTmpUser     -  Pointer to the user list structure
 *               IN      dwKeySpec    -  Type of key to be used for signing
 *               IN      sDescription -  Description of document to be signed
 *
 *  Returns:
 */
BOOL CryptConfirmSignature(
                           IN PNTAGUserList pTmpUser,
                           IN DWORD dwKeySpec,
                           IN LPCTSTR sDescription
                           );

/*
 -  CryptUserProtectKey
 -
 *  Purpose:      Obtain or determine user protection information.
 *                
 *
 *
 *  Parameters:
 *               IN      hPrivid      -  Handle to the id of the user
 *               IN      hKey         -  Handle to key
 *
 *  Returns:
 */
BOOL CryptUserProtectKey(IN HPRIVUID hUID,
                         IN HCRYPTKEY hKey);


/*
 -  CryptConfirmEncryption
 -
 *  Purpose:
 *                Determine weather the encryption should proceed.
 *
 *
 *  Parameters:
 *               IN      hPrivid      -  Handle to the id of the user
 *               IN      hKey         -  Handle to key
 *               IN      final        -  flag indicating last encrypt for a
 *                                       block of data
 *
 *  Returns:
 */
BOOL CryptConfirmEncryption(IN HPRIVUID hUID,
                            IN HCRYPTKEY hKey,
                            IN BOOL final);

/*
 -  CryptConfirmDecryption
 -
 *  Purpose:
 *                Determine weather the DEcryption should proceed.
 *
 *
 *  Parameters:
 *               IN      hPrivid      -  Handle to the id of the user
 *               IN      hKey         -  Handle to key
 *               IN      final        -  flag indicating last encrypt for a
 *                                       block of data
 *
 *  Returns:
 */
BOOL CryptConfirmDecryption(IN HPRIVUID hUID,
                            IN HCRYPTKEY hKey,
                            IN BOOL final);

/*
 -  CryptConfirmTranslation
 -
 *  Purpose:
 *                Determine weather the translation should proceed.
 *
 *
 *  Parameters:
 *               IN      hPrivid      -  Handle to the id of the user
 *               IN      hKey         -  Handle to key
 *               IN      final        -  flag indicating last encrypt for a
 *                                       block of data
 *
 *  Returns:
 */
BOOL CryptConfirmTranslation(IN HPRIVUID hUID,
                             IN HCRYPTKEY hKey,
                             IN BOOL final);

/*
 -  CryptConfirmExportKey
 -
 *  Purpose:
 *                Determine whether the export key should proceed.
 *
 *
 *  Parameters:
 *               IN      pTmpUser     -  Pointer to the user list structure
 *               IN      dwKeySpec    -  Type of key to be exported
 *
 *  Returns:
 */
BOOL CryptConfirmExportKey(
                           IN PNTAGUserList pTmpUser,
                           IN DWORD dwKeySpec
                           );

/*
 -  CryptConfirmImportKey
 -
 *  Purpose:
 *                Determine whether the import key should proceed.
 *
 *
 *  Parameters:
 *               IN      pTmpUser     -  Pointer to the user list structure
 *               IN      pKey         -  Pointer to the key list structure
 *               IN      dwBlobType   -  Type of blob to be imported
 *               IN      dwKeySpec    -  Type of key to be imported
 *
 *  Returns:
 */
BOOL CryptConfirmImportKey(
                           IN PNTAGUserList pTmpUser,
                           IN DWORD dwBlobType,
                           IN DWORD dwKeySpec
                           );

/*
 -  CryptConfirmGenKey
 -
 *  Purpose:
 *                Determine whether the gen key should proceed.
 *
 *
 *  Parameters:
 *               IN      pTmpUser     -  Pointer to the user list structure
 *               IN      pKey         -  Pointer to the key list structure
 *               IN      dwKeySpec    -  Type of key to be imported
 *
 *  Returns:
 */
BOOL CryptConfirmGenKey(
                        IN PNTAGUserList pTmpUser,
                        IN DWORD dwKeySpec
                        );

/*
 -  CryptConfirmChangePassword
 -
 *  Purpose:
 *                Determine whether the key password should be changed.
 *
 *
 *  Parameters:
 *               IN      pTmpUser     -  Pointer to the user list structure
 *               IN      dwKeySpec    -  Type of key changing the password for
 *
 *  Returns:
 */
BOOL CryptConfirmChangePassword(
                                IN PNTAGUserList pTmpUser,
                                IN DWORD dwKeySpec
                                );
