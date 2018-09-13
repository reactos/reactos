/////////////////////////////////////////////////////////////////////////////
//  FILE          : cryptapi.c                                             //
//  DESCRIPTION   : Crypto API interface                                   //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      Dec  6 1994 larrys  New                                            //
//      Nov 13 1995 philh   Lean & mean version for the STB                //
//      Aug 18 1996 mattt   sadvapi chgs from ecm tree                     //
//      Oct 23 1997 jeffspel Checkin for SChannel use                      //
//      Feb 08 1999 sfield  avoid critical section, add exception handling //
//                                                                         //
//  Copyright (C) 1993 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <wincrypt.h>
#include "swincryp.h"
#include "scp.h"

typedef struct _VTableStruc {
    HCRYPTPROV  hProv;                          // Handle to provider
    LONG Inuse;
} VTableStruc, *PVTableStruc;

typedef struct _VKeyStruc {
    PVTableStruc pVTable;                       // pointer to provider
    HCRYPTKEY    hKey;                          // Handle to key
} VKeyStruc, *PVKeyStruc;

typedef struct _VHashStruc {
    PVTableStruc pVTable;                       // pointer to provider
    HCRYPTHASH  hHash;                          // Handle to hash
} VHashStruc, *PVHashStruc;


void __inline EnterProviderCritSec(IN PVTableStruc pVTable);
LONG __inline LeaveProviderCritSec(IN PVTableStruc pVTable);

PVKeyStruc BuildVKey(IN PVTableStruc pVTable);
PVHashStruc BuildVHash(IN PVTableStruc pVTable);

/*
 -  CryptAcquireContextW
 -
 *  Purpose:
 *               The CryptAcquireContext function is used to acquire a context
 *               handle to a cryptograghic service provider (CSP).
 *
 *
 *  Parameters:
 *               OUT    phProv      -  Handle to a CSP
 *               IN OUT pszIdentity -  Pointer to the name of the context's
 *                                     keyset.
 *               IN OUT pszProvider -  Pointer to the name of the provider.
 *               IN     dwProvType   -  Requested CSP type
 *               IN     dwFlags     -  Flags values
 *
 *  Returns:
 */
BOOL
WINAPI SCryptAcquireContextW(OUT    HCRYPTPROV *phProv,
                            IN OUT LPCWSTR pwszIdentity,
                            IN OUT LPCWSTR pwszProvider,
                            IN     DWORD dwProvType,
                            IN     DWORD dwFlags)
{
    CHAR    *pszIdentity = NULL;
    CHAR    *pszProvider = NULL;
    long    c = 0;
    long    i;
    BOOL    fRet = FALSE;

    if (pwszIdentity)
    {
        c = wcslen(pwszIdentity);
        if (NULL == (pszIdentity = (CHAR*)LocalAlloc(LMEM_ZEROINIT,
                                                     (c+1) * sizeof(CHAR))))
            goto Ret;
        for (i=0;i<c;i++)
            pszIdentity[i] = (CHAR)pwszIdentity[i];
        pszIdentity[i] = 0;
    }

    if (pwszProvider)
    {
        c = wcslen(pwszProvider);
        if (NULL == (pszProvider = (CHAR*)LocalAlloc(LMEM_ZEROINIT,
                                                     (c+1) * sizeof(CHAR))))
            goto Ret;
        for (i=0;i<c;i++)
            pszProvider[i] = (CHAR)pwszProvider[i];
        pszProvider[i] = 0;
    }

    fRet = SCryptAcquireContextA(
                            phProv,
                            pszIdentity,
                            pszProvider,
                            dwProvType,
                            dwFlags
                            );

Ret:
    if (pszIdentity)
        LocalFree(pszIdentity);
    if (pszProvider)
        LocalFree(pszProvider);
    return fRet;
}

BOOL
WINAPI SCryptAcquireContextA(OUT    HCRYPTPROV *phProv,
                            IN OUT LPCSTR pszIdentity,
                            IN OUT LPCSTR pszProvider,  // ignored
                            IN     DWORD dwProvType,    // ignored
                            IN     DWORD dwFlags)
{
    PVTableStruc        pVTable = NULL;
    VTableProvStruc     TableForProvider;
    BOOL                fRet = FALSE;


    pVTable = (PVTableStruc)LocalAlloc(
                                    LMEM_ZEROINIT,
                                    sizeof(VTableStruc)
                                    );

    if( pVTable == NULL ) {
        *phProv = 0;
        return FALSE;
    }

    memset(&TableForProvider, 0, sizeof(TableForProvider));
    TableForProvider.Version = 2;
    TableForProvider.FuncVerifyImage = NULL;
    TableForProvider.FuncReturnhWnd = 0;
    TableForProvider.dwProvType = dwProvType;
    TableForProvider.pbContextInfo = NULL;
    TableForProvider.cbContextInfo = 0;

    __try {

        fRet = CPAcquireContext(
                            &pVTable->hProv,
                            (LPSTR)pszIdentity,
                            dwFlags,
                            &TableForProvider
                            );
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        fRet = FALSE;
        ASSERT( fRet );
    }


    if(!fRet)
    {
        LocalFree(pVTable);
        pVTable = NULL;
    }
    else
    {
        if (dwFlags & CRYPT_DELETEKEYSET)
        {
            LocalFree(pVTable);
            pVTable = NULL;
        } else {
            pVTable->Inuse = 1;
        }

    }

    *phProv = (HCRYPTPROV)pVTable;

    return fRet;
}


/*
 -  CryptReleaseContext
 -
 *  Purpose:
 *               The CryptReleaseContext function is used to release a
 *               context created by CryptAcquireContext.
 *
 *  Parameters:
 *               IN  phProv        -  Handle to a CSP
 *               IN  dwFlags       -  Flags values
 *
 *  Returns:
 */
BOOL
WINAPI SCryptReleaseContext(IN HCRYPTPROV hProv,
                           IN DWORD dwFlags)
{
    PVTableStruc pVTable = (PVTableStruc) hProv;
    LONG         ContextRefCount;
    BOOL         fRet = FALSE;

    ContextRefCount = LeaveProviderCritSec( pVTable );

    __try {


        //
        // for debug builds, catch fools leaking state.
        //

        ASSERT( ContextRefCount == 0 );

        if( ContextRefCount != 0 ) {
            SetLastError(ERROR_BUSY);
            return FALSE;
        }

        fRet = CPReleaseContext(pVTable->hProv, dwFlags);

    } __except ( EXCEPTION_EXECUTE_HANDLER ) {
        fRet = FALSE;
        ASSERT( fRet );
    }

    LocalFree(pVTable);

    return fRet;
}

/*
 -  CryptGenKey
 -
 *  Purpose:
 *                Generate cryptographic keys
 *
 *
 *  Parameters:
 *               IN      hProv   -  Handle to a CSP
 *               IN      Algid   -  Algorithm identifier
 *               IN      dwFlags -  Flags values
 *               OUT     phKey   -  Handle to a generated key
 *
 *  Returns:
 */
BOOL
WINAPI SCryptGenKey(IN HCRYPTPROV hProv,
                   IN ALG_ID Algid,
                   IN DWORD dwFlags,
                   OUT HCRYPTKEY * phKey)
{
    PVTableStruc    pVTable = (PVTableStruc) hProv;
    PVKeyStruc      pVKey;
    BOOL            fCritSec = FALSE;
    BOOL            fRet = FALSE;

    __try {
        *phKey = 0;
        pVKey = BuildVKey(pVTable);

        if( pVKey == NULL )
            return FALSE;

        EnterProviderCritSec(pVTable);
        fCritSec = TRUE;

        fRet = CPGenKey(pVTable->hProv, Algid, dwFlags, &pVKey->hKey);
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        fRet = FALSE;
        ASSERT( fRet );
    }

    if( fCritSec ) {
        LeaveProviderCritSec(pVTable);
    }


    if( fRet ) {
        *phKey = (HCRYPTKEY) pVKey;
        return TRUE;
    }

    if (pVKey)
        LocalFree(pVKey);

    __try {
        *phKey = 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ; // gulp
    }

    return FALSE;
}

/*
 -      CryptDuplicateKey
 -
 *      Purpose:
 *                Duplicate a cryptographic key
 *
 *
 *      Parameters:
 *               IN      hKey           -  Handle to the key to be duplicated
 *               IN      pdwReserved    -  Reserved for later use
 *               IN      dwFlags        -  Flags values
 *               OUT     phKey          -  Handle to the new duplicate key
 *
 *      Returns:
 */
BOOL
WINAPI SCryptDuplicateKey(
                         IN HCRYPTKEY hKey,
                         IN DWORD *pdwReserved,
                         IN DWORD dwFlags,
                         OUT HCRYPTKEY * phKey
                         )
{
    PVKeyStruc      pVKey = (PVKeyStruc) hKey;
    PVTableStruc    pVTable;
    PVKeyStruc      pVNewKey;
    BOOL            fCritSec = FALSE;
    BOOL            fRet = FALSE;


    __try {
        *phKey = 0;
        pVTable = pVKey->pVTable;

        pVNewKey = BuildVKey(pVTable);

        if( pVNewKey == NULL ) {
            return FALSE;
        }

        EnterProviderCritSec(pVTable);
        fCritSec = TRUE;

        fRet = CPDuplicateKey(
                        pVTable->hProv,
                        pVKey->hKey,
                        pdwReserved,
                        dwFlags,
                        &pVNewKey->hKey
                        );
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        fRet = FALSE;
        ASSERT( fRet );
    }


    if( fCritSec ) {
        LeaveProviderCritSec(pVTable);
    }

    if( fRet ) {
        *phKey = (HCRYPTKEY) pVNewKey;
        return TRUE;
    }

    if (pVNewKey)
        LocalFree(pVNewKey);

    __try {
        *phKey = 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ; // gulp
    }

    return FALSE;
}

/*
 -  CryptDeriveKey
 -
 *  Purpose:
 *                Derive cryptographic keys from base data
 *
 *
 *  Parameters:
 *               IN      hProv      -  Handle to a CSP
 *               IN      Algid      -  Algorithm identifier
 *               IN      hHash      -  Handle to hash of base data
 *               IN      dwFlags    -  Flags values
 *               OUT     phKey      -  Handle to a generated key
 *
 *  Returns:
 */
BOOL
WINAPI SCryptDeriveKey(IN HCRYPTPROV hProv,
                      IN ALG_ID Algid,
                      IN HCRYPTHASH hHash,
                      IN DWORD dwFlags,
                      OUT HCRYPTKEY * phKey)
{
    PVTableStruc    pVTable = (PVTableStruc) hProv;
    PVHashStruc     pVHash = (PVHashStruc) hHash;
    PVKeyStruc      pVKey;
    BOOL            fCritSec = FALSE;
    BOOL            fRet = FALSE;


    __try {

        *phKey = 0;
        if (pVHash->pVTable != pVTable)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        pVKey = BuildVKey(pVTable);
        if( pVKey == NULL )
            return FALSE;

        EnterProviderCritSec(pVTable);
        fCritSec = TRUE;

        fRet = CPDeriveKey(
                        pVTable->hProv,
                        Algid,
                        pVHash->hHash,
                        dwFlags,
                        &pVKey->hKey
                        );
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        fRet = FALSE;
        ASSERT( fRet );
    }

    if( fCritSec ) {
        LeaveProviderCritSec(pVTable);
    }

    if( fRet ) {
        *phKey = (HCRYPTKEY) pVKey;
        return TRUE;
    }

    if (pVKey)
        LocalFree(pVKey);

    __try {
        *phKey = 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ; // gulp
    }

    return FALSE;
}


/*
 -  CryptDestroyKey
 -
 *  Purpose:
 *                Destroys the cryptographic key that is being referenced
 *                with the hKey parameter
 *
 *
 *  Parameters:
 *               IN      hKey   -  Handle to a key
 *
 *  Returns:
 */
BOOL
WINAPI SCryptDestroyKey(IN HCRYPTKEY hKey)
{
    PVKeyStruc      pVKey = (PVKeyStruc) hKey;
    PVTableStruc    pVTable;
    BOOL            fCritSec = FALSE;
    BOOL            fRet = FALSE;

    __try {
        pVTable = pVKey->pVTable;

        EnterProviderCritSec(pVTable);
        fCritSec = TRUE;

        fRet = CPDestroyKey(pVTable->hProv, pVKey->hKey);
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        fRet = FALSE;
        ASSERT( fRet );
    }

    if( fCritSec ) {
        LeaveProviderCritSec(pVTable);
    }

    LocalFree(pVKey);

    return fRet;
}


/*
 -  CryptSetKeyParam
 -
 *  Purpose:
 *                Allows applications to customize various aspects of the
 *                operations of a key
 *
 *  Parameters:
 *               IN      hKey    -  Handle to a key
 *               IN      dwParam -  Parameter number
 *               IN      pbData  -  Pointer to data
 *               IN      dwFlags -  Flags values
 *
 *  Returns:
 */
BOOL
WINAPI SCryptSetKeyParam(IN HCRYPTKEY hKey,
                        IN DWORD dwParam,
                        IN BYTE *pbData,
                        IN DWORD dwFlags)
{
    PVKeyStruc      pVKey = (PVKeyStruc) hKey;
    PVTableStruc    pVTable;
    BOOL            fCritSec = FALSE;
    BOOL            fRet = FALSE;

    __try {
        pVTable = pVKey->pVTable;

        EnterProviderCritSec(pVTable);
        fCritSec = TRUE;

        fRet = CPSetKeyParam(
                            pVTable->hProv,
                            pVKey->hKey,
                            dwParam,
                            pbData,
                            dwFlags
                            );
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        fRet = FALSE;
        ASSERT( fRet );
    }

    if( fCritSec )
        LeaveProviderCritSec(pVTable);

    return fRet;
}


/*
 -  CryptGetKeyParam
 -
 *  Purpose:
 *                Allows applications to get various aspects of the
 *                operations of a key
 *
 *  Parameters:
 *               IN      hKey       -  Handle to a key
 *               IN      dwParam    -  Parameter number
 *               IN      pbData     -  Pointer to data
 *               IN      pdwDataLen -  Length of parameter data
 *               IN      dwFlags    -  Flags values
 *
 *  Returns:
 */
BOOL
WINAPI SCryptGetKeyParam(IN HCRYPTKEY hKey,
                        IN DWORD dwParam,
                        IN BYTE *pbData,
                        IN DWORD *pdwDataLen,
                        IN DWORD dwFlags)
{
    PVKeyStruc      pVKey = (PVKeyStruc) hKey;
    PVTableStruc    pVTable;
    BOOL            fCritSec = FALSE;
    BOOL            fRet = FALSE;

    __try {
        pVTable = pVKey->pVTable;

        EnterProviderCritSec(pVTable);
        fCritSec = TRUE;

        fRet = CPGetKeyParam(
                        pVTable->hProv,
                        pVKey->hKey,
                        dwParam,
                        pbData,
                        pdwDataLen,
                        dwFlags
                        );
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        fRet = FALSE;
        ASSERT( fRet );
    }

    if( fCritSec ) {
        LeaveProviderCritSec(pVTable);
    }

    return fRet;
}


/*
 -  CryptGenRandom
 -
 *  Purpose:
 *                Used to fill a buffer with random bytes
 *
 *
 *  Parameters:
 *               IN  hProv      -  Handle to the user identifcation
 *               IN  dwLen      -  Number of bytes of random data requested
 *               OUT pbBuffer   -  Pointer to the buffer where the random
 *                                 bytes are to be placed
 *
 *  Returns:
 */
BOOL
WINAPI SCryptGenRandom(IN HCRYPTPROV hProv,
                      IN DWORD dwLen,
                      OUT BYTE *pbBuffer)

{
    PVTableStruc    pVTable = (PVTableStruc) hProv;
    BOOL            fCritSec = FALSE;
    BOOL            fRet = FALSE;

    __try {
        EnterProviderCritSec(pVTable);
        fCritSec = TRUE;

        fRet = CPGenRandom(pVTable->hProv, dwLen, pbBuffer);
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        fRet = FALSE;
        ASSERT( fRet );
    }

    if( fCritSec ) {
        LeaveProviderCritSec(pVTable);
    }

    return fRet;
}

/*
 -  CryptGetUserKey
 -
 *  Purpose:
 *                Gets a handle to a permanent user key
 *
 *
 *  Parameters:
 *               IN  hProv      -  Handle to the user identifcation
 *               IN  dwKeySpec  -  Specification of the key to retrieve
 *               OUT phUserKey  -  Pointer to key handle of retrieved key
 *
 *  Returns:
 */
BOOL
WINAPI SCryptGetUserKey(IN HCRYPTPROV hProv,
                       IN DWORD dwKeySpec,
                       OUT HCRYPTKEY *phUserKey)
{
    PVTableStruc    pVTable = (PVTableStruc) hProv;
    PVKeyStruc      pVKey;
    BOOL            fCritSec = FALSE;
    BOOL            fRet = FALSE;


    __try {
        *phUserKey = 0;
        pVKey = BuildVKey(pVTable);

        if( pVKey == NULL ) {
            return FALSE;
        }

        EnterProviderCritSec(pVTable);
        fCritSec = TRUE;

        fRet = CPGetUserKey(pVTable->hProv, dwKeySpec, &pVKey->hKey);
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        fRet = FALSE;
        ASSERT( fRet );
    }

    if( fCritSec ) {
        LeaveProviderCritSec(pVTable);
    }

    if( fRet ) {
        *phUserKey = (HCRYPTKEY) pVKey;
        return TRUE;
    }

    if (pVKey)
        LocalFree(pVKey);

    __try {
        *phUserKey = 0;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        ; // gulp
    }

    return FALSE;
}



/*
 -  CryptExportKey
 -
 *  Purpose:
 *                Export cryptographic keys out of a CSP in a secure manner
 *
 *
 *  Parameters:
 *               IN  hKey       - Handle to the key to export
 *               IN  hPubKey    - Handle to the exchange public key value of
 *                                the destination user
 *               IN  dwBlobType - Type of key blob to be exported
 *               IN  dwFlags -    Flags values
 *               OUT pbData -     Key blob data
 *               OUT pdwDataLen - Length of key blob in bytes
 *
 *  Returns:
 */
BOOL
WINAPI SCryptExportKey(IN HCRYPTKEY hKey,
                      IN HCRYPTKEY hPubKey,
                      IN DWORD dwBlobType,
                      IN DWORD dwFlags,
                      OUT BYTE *pbData,
                      OUT DWORD *pdwDataLen)
{
    PVKeyStruc      pVKey = (PVKeyStruc) hKey;
    PVTableStruc    pVTable;
    PVKeyStruc      pVPublicKey = (PVKeyStruc) hPubKey;
    BOOL            fCritSec = FALSE;
    BOOL            fRet = FALSE;


    __try {
        // Note: the SCP requires that the hPubKey has the same hProv as
        // the hKey. This is a problem for the MITV implementation where the
        // signature and exchange keys always have a different provider.

        pVTable = pVKey->pVTable;

        if (pVPublicKey && pVPublicKey->pVTable != pVTable)
        {
            *pdwDataLen = 0;
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        EnterProviderCritSec(pVTable);
        fCritSec = TRUE;

        fRet = CPExportKey(
                        pVTable->hProv,
                        pVKey->hKey,
                        (pVPublicKey == NULL ? 0 : pVPublicKey->hKey),
                        dwBlobType,
                        dwFlags,
                        pbData,
                        pdwDataLen
                        );
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        fRet = FALSE;
        ASSERT( fRet );
    }

    if( fCritSec ) {
        LeaveProviderCritSec(pVTable);
    }

    return fRet;
}


/*
 -  CryptImportKey
 -
 *  Purpose:
 *                Import cryptographic keys
 *
 *
 *  Parameters:
 *               IN  hProv     -  Handle to the CSP user
 *               IN  pbData    -  Key blob data
 *               IN  dwDataLen -  Length of the key blob data
 *               IN  hPubKey   -  Handle to the exchange public key value of
 *                                the destination user
 *               IN  dwFlags   -  Flags values
 *               OUT phKey     -  Pointer to the handle to the key which was
 *                                Imported
 *
 *  Returns:
 */
BOOL
WINAPI SCryptImportKey(IN HCRYPTPROV hProv,
                      IN CONST BYTE *pbData,
                      IN DWORD dwDataLen,
                      IN HCRYPTKEY hPubKey,
                      IN DWORD dwFlags,
                      OUT HCRYPTKEY *phKey)
{
    PVTableStruc    pVTable = (PVTableStruc) hProv;
    PVKeyStruc      pVKey;
    PVKeyStruc      pVPublicKey = (PVKeyStruc) hPubKey;
    BOOL            fCritSec = FALSE;
    BOOL            fRet = FALSE;

    __try {

        *phKey = 0;

        if (pVPublicKey && pVPublicKey->pVTable != pVTable)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        pVKey = BuildVKey(pVTable);

        if( pVKey == NULL )
            return FALSE;

        EnterProviderCritSec(pVTable);
        fCritSec = TRUE;

        fRet = CPImportKey(
                        pVTable->hProv,
                        pbData,
                        dwDataLen,
                        (pVPublicKey == NULL ? 0 : pVPublicKey->hKey),
                        dwFlags,
                        &pVKey->hKey
                        );
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        fRet = FALSE;
        ASSERT( fRet );
    }

    if( fCritSec ) {
        LeaveProviderCritSec(pVTable);
    }

    if( fRet ) {
        *phKey = (HCRYPTKEY) pVKey;
        return TRUE;
    }

    if (pVKey)
        LocalFree(pVKey);

    __try {
        *phKey = 0;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        ; // gulp
    }

    return FALSE;
}


/*
 -  CryptEncrypt
 -
 *  Purpose:
 *                Encrypt data
 *
 *
 *  Parameters:
 *               IN  hKey          -  Handle to the key
 *               IN  hHash         -  Optional handle to a hash
 *               IN  Final         -  Boolean indicating if this is the final
 *                                    block of plaintext
 *               IN  dwFlags       -  Flags values
 *               IN OUT pbData     -  Data to be encrypted
 *               IN OUT pdwDataLen -  Pointer to the length of the data to be
 *                                    encrypted
 *               IN dwBufLen       -  Size of Data buffer
 *
 *  Returns:
 */
BOOL
WINAPI SCryptEncrypt(IN HCRYPTKEY hKey,
                    IN HCRYPTHASH hHash,
                    IN BOOL Final,
                    IN DWORD dwFlags,
                    IN OUT BYTE *pbData,
                    IN OUT DWORD *pdwDataLen,
                    IN DWORD dwBufLen)
{
    PVKeyStruc      pVKey = (PVKeyStruc) hKey;
    PVTableStruc    pVTable;
    PVHashStruc     pVHash = (PVHashStruc) hHash;
    BOOL            fCritSec = FALSE;
    BOOL            fRet = FALSE;


    __try {

        pVTable = pVKey->pVTable;

        if (pVHash && pVHash->pVTable != pVTable)
        {
            *pdwDataLen = 0;
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        EnterProviderCritSec(pVTable);
        fCritSec = TRUE;

        fRet = CPEncrypt(
                        pVTable->hProv,
                        pVKey->hKey,
                        (pVHash == NULL ? 0 : pVHash->hHash),
                        Final,
                        dwFlags,
                        pbData,
                        pdwDataLen,
                        dwBufLen
                        );
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        fRet = FALSE;
        ASSERT( fRet );
    }

    if( fCritSec ) {
        LeaveProviderCritSec(pVTable);
    }

    return fRet;
}


/*
 -  CryptDecrypt
 -
 *  Purpose:
 *                Decrypt data
 *
 *
 *  Parameters:
 *               IN  hKey          -  Handle to the key
 *               IN  hHash         -  Optional handle to a hash
 *               IN  Final         -  Boolean indicating if this is the final
 *                                    block of ciphertext
 *               IN  dwFlags       -  Flags values
 *               IN OUT pbData     -  Data to be decrypted
 *               IN OUT pdwDataLen -  Pointer to the length of the data to be
 *                                    decrypted
 *
 *  Returns:
 */
BOOL
WINAPI SCryptDecrypt(IN HCRYPTKEY hKey,
                    IN HCRYPTHASH hHash,
                    IN BOOL Final,
                    IN DWORD dwFlags,
                    IN OUT BYTE *pbData,
                    IN OUT DWORD *pdwDataLen)

{
    PVKeyStruc      pVKey = (PVKeyStruc) hKey;
    PVTableStruc    pVTable;
    PVHashStruc     pVHash = (PVHashStruc) hHash;
    BOOL            fCritSec = FALSE;
    BOOL            fRet = FALSE;

    __try {
        pVTable = pVKey->pVTable;

        if (pVHash && pVHash->pVTable != pVTable)
        {
            *pdwDataLen = 0;
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        EnterProviderCritSec(pVTable);
        fCritSec = TRUE;

        fRet = CPDecrypt(
                        pVTable->hProv,
                        pVKey->hKey,
                        (pVHash == NULL ? 0 : pVHash->hHash),
                        Final,
                        dwFlags,
                        pbData,
                        pdwDataLen
                        );
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        fRet = FALSE;
        ASSERT( fRet );
    }

    if( fCritSec ) {
        LeaveProviderCritSec(pVTable);
    }

    return fRet;
}


/*
 -  CryptCreateHash
 -
 *  Purpose:
 *                initate the hashing of a stream of data
 *
 *
 *  Parameters:
 *               IN  hProv   -  Handle to the user identifcation
 *               IN  Algid   -  Algorithm identifier of the hash algorithm
 *                              to be used
 *               IN  hKey    -  Optional key for MAC algorithms
 *               IN  dwFlags -  Flags values
 *               OUT pHash   -  Handle to hash object
 *
 *  Returns:
 */
BOOL
WINAPI SCryptCreateHash(IN HCRYPTPROV hProv,
                       IN ALG_ID Algid,
                       IN HCRYPTKEY hKey,
                       IN DWORD dwFlags,
                       OUT HCRYPTHASH *phHash)
{
    PVTableStruc    pVTable = (PVTableStruc) hProv;
    PVKeyStruc      pVKey = (PVKeyStruc) hKey;
    PVHashStruc     pVHash;
    BOOL            fCritSec = FALSE;
    BOOL            fRet = FALSE;

    __try {
        *phHash = 0;

        if (pVKey && pVKey->pVTable != pVTable)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        pVHash = BuildVHash(pVTable);
        if( pVHash == NULL )
            return FALSE;

        EnterProviderCritSec(pVTable);
        fCritSec = TRUE;

        fRet = CPCreateHash(
                        pVTable->hProv,
                        Algid,
                        (pVKey == NULL ? 0 : pVKey->hKey),
                        dwFlags,
                        &pVHash->hHash
                        );
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        fRet = FALSE;
        ASSERT( fRet );
    }

    if( fCritSec ) {
        LeaveProviderCritSec(pVTable);
    }

    if( fRet ) {
        *phHash = (HCRYPTHASH) pVHash;
        return TRUE;
    }

    if (pVHash)
        LocalFree(pVHash);

    __try {
        *phHash = 0;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        ; // gulp
    }

    return FALSE;
}

/*
 -      CryptDuplicateHash
 -
 *      Purpose:
 *                Duplicate a cryptographic hash
 *
 *
 *      Parameters:
 *               IN      hHash          -  Handle to the hash to be duplicated
 *               IN      pdwReserved    -  Reserved for later use
 *               IN      dwFlags        -  Flags values
 *               OUT     phHash         -  Handle to the new duplicate hash
 *
 *      Returns:
 */
BOOL
WINAPI SCryptDuplicateHash(
                         IN HCRYPTHASH hHash,
                         IN DWORD *pdwReserved,
                         IN DWORD dwFlags,
                         OUT HCRYPTHASH * phHash
                         )
{
    PVHashStruc     pVHash = (PVHashStruc) hHash;
    PVTableStruc    pVTable;
    PVHashStruc     pVNewHash;
    BOOL            fCritSec = FALSE;
    BOOL            fRet = FALSE;


    __try {
        *phHash = 0;
        pVTable = pVHash->pVTable;

        pVNewHash = BuildVHash(pVTable);
        if( pVNewHash == NULL ) {
            return FALSE;
        }

        EnterProviderCritSec(pVTable);
        fCritSec = TRUE;

        fRet = CPDuplicateHash(
                        pVTable->hProv,
                        pVHash->hHash,
                        pdwReserved,
                        dwFlags,
                        &pVNewHash->hHash
                        );
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        fRet = FALSE;
        ASSERT( fRet );
    }

    if( fCritSec ) {
        LeaveProviderCritSec(pVTable);
    }

    if( fRet ) {
        *phHash = (HCRYPTHASH) pVNewHash;
        return TRUE;
    }

    if (pVNewHash)
        LocalFree(pVNewHash);

    __try {
        *phHash = 0;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        ; // gulp
    }

    return FALSE;

}

/*
 -  CryptHashData
 -
 *  Purpose:
 *                Compute the cryptograghic hash on a stream of data
 *
 *
 *  Parameters:
 *               IN  hHash     -  Handle to hash object
 *               IN  pbData    -  Pointer to data to be hashed
 *               IN  dwDataLen -  Length of the data to be hashed
 *               IN  dwFlags   -  Flags values
 *
 *
 *  Returns:
 */
BOOL
WINAPI SCryptHashData(IN HCRYPTHASH hHash,
                     IN CONST BYTE *pbData,
                     IN DWORD dwDataLen,
                     IN DWORD dwFlags)
{
    PVHashStruc     pVHash = (PVHashStruc) hHash;
    PVTableStruc    pVTable;
    BOOL            fCritSec = FALSE;
    BOOL            fRet = FALSE;


    __try {
        pVTable = pVHash->pVTable;

        EnterProviderCritSec(pVTable);
        fCritSec = TRUE;

        fRet = CPHashData(
                        pVTable->hProv,
                        pVHash->hHash,
                        pbData,
                        dwDataLen,
                        dwFlags
                        );

    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        fRet = FALSE;
        ASSERT( fRet );
    }

    if( fCritSec ) {
        LeaveProviderCritSec(pVTable);
    }

    return fRet;
}

/*
 -  CryptHashSessionKey
 -
 *  Purpose:
 *                Compute the cryptograghic hash on a key object
 *
 *
 *  Parameters:
 *               IN  hHash     -  Handle to hash object
 *               IN  hKey      -  Handle to a key object
 *               IN  dwFlags   -  Flags values
 *
 *  Returns:
 *               CRYPT_FAILED
 *               CRYPT_SUCCEED
 */
BOOL
WINAPI SCryptHashSessionKey(IN HCRYPTHASH hHash,
                           IN HCRYPTKEY hKey,
                           IN DWORD dwFlags)
{
    PVHashStruc     pVHash = (PVHashStruc) hHash;
    PVTableStruc    pVTable;
    PVKeyStruc      pVKey = (PVKeyStruc) hKey;
    BOOL            fCritSec = FALSE;
    BOOL            fRet = FALSE;


    __try {
        pVTable = pVHash->pVTable;

        if (pVKey->pVTable != pVTable)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        EnterProviderCritSec(pVTable);
        fCritSec = TRUE;

        fRet = CPHashSessionKey(
                        pVTable->hProv,
                        pVHash->hHash,
                        pVKey->hKey,
                        dwFlags
                        );
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        fRet = FALSE;
        ASSERT( fRet );
    }

    if( fCritSec ) {
        LeaveProviderCritSec(pVTable);
    }

    return fRet;
}


/*
 -  CryptDestoyHash
 -
 *  Purpose:
 *                Destory the hash object
 *
 *
 *  Parameters:
 *               IN  hHash     -  Handle to hash object
 *
 *  Returns:
 */
BOOL
WINAPI SCryptDestroyHash(IN HCRYPTHASH hHash)
{
    PVHashStruc     pVHash = (PVHashStruc) hHash;
    PVTableStruc    pVTable;
    BOOL            fCritSec = FALSE;
    BOOL            fRet = FALSE;


    __try {
        pVTable = pVHash->pVTable;

        EnterProviderCritSec(pVTable);
        fCritSec = TRUE;

        fRet = CPDestroyHash(pVTable->hProv, pVHash->hHash);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        fRet = FALSE;
        ASSERT( fRet );
    }

    if( fCritSec ) {
        LeaveProviderCritSec(pVTable);
    }

    LocalFree(pVHash);

    return fRet;
}


/*
 -      CryptSignHashW
 -
 *      Purpose:
 *                Create a digital signature from a hash
 *
 *
 *      Parameters:
 *               IN  hHash        -  Handle to hash object
 *               IN  dwKeySpec    -  Key pair that is used to sign with
 *                                   algorithm to be used
 *               IN  sDescription -  Description of data to be signed
 *               IN  dwFlags      -  Flags values
 *               OUT pbSignture   -  Pointer to signature data
 *               OUT pdwSigLen    -  Pointer to the len of the signature data
 *
 *      Returns:
 */
BOOL
WINAPI SCryptSignHashW(IN  HCRYPTHASH hHash,
              IN  DWORD dwKeySpec,
              IN  LPCWSTR sDescription,
              IN  DWORD dwFlags,
              OUT BYTE *pbSignature,
              OUT DWORD *pdwSigLen)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

BOOL
WINAPI SCryptSignHashA(IN  HCRYPTHASH hHash,
              IN  DWORD dwKeySpec,
              IN  LPCSTR sDescription,
              IN  DWORD dwFlags,
              OUT BYTE *pbSignature,
              OUT DWORD *pdwSigLen)
{
    PVHashStruc     pVHash = (PVHashStruc) hHash;
    PVTableStruc    pVTable;
    BOOL            fCritSec = FALSE;
    BOOL            fRet = FALSE;

    __try {
        pVTable = pVHash->pVTable;

        EnterProviderCritSec(pVTable);
        fCritSec = TRUE;

        fRet = CPSignHash(
                        pVTable->hProv,
                        pVHash->hHash,
                        dwKeySpec,
                        NULL,
                        dwFlags,
                        pbSignature,
                        pdwSigLen
                        );
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        fRet = FALSE;
        ASSERT( fRet );
    }

    if( fCritSec ) {
        LeaveProviderCritSec(pVTable);
    }

    return fRet;
}

/*
 -      CryptVerifySignatureW
 -
 *      Purpose:
 *                Used to verify a signature against a hash object
 *
 *
 *      Parameters:
 *               IN  hHash        -  Handle to hash object
 *               IN  pbSignture   -  Pointer to signature data
 *               IN  dwSigLen     -  Length of the signature data
 *               IN  hPubKey      -  Handle to the public key for verifying
 *                                   the signature
 *               IN  sDescription -  String describing the signed data
 *               IN  dwFlags      -  Flags values
 *
 *      Returns:
 */
BOOL
WINAPI SCryptVerifySignatureW(IN HCRYPTHASH hHash,
                 IN CONST BYTE *pbSignature,
                 IN DWORD dwSigLen,
                 IN HCRYPTKEY hPubKey,
                 IN LPCWSTR sDescription,
                 IN DWORD dwFlags)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

BOOL
WINAPI SCryptVerifySignatureA(IN HCRYPTHASH hHash,
                 IN CONST BYTE *pbSignature,
                 IN DWORD dwSigLen,
                 IN HCRYPTKEY hPubKey,
                 IN LPCSTR sDescription,
                 IN DWORD dwFlags)
{
    PVHashStruc     pVHash = (PVHashStruc) hHash;
    PVTableStruc    pVTable;
    PVKeyStruc      pVPubKey = (PVKeyStruc) hPubKey;
    BOOL            fCritSec = FALSE;
    BOOL            fRet = FALSE;


    __try {
        pVTable = pVHash->pVTable;

        if (pVPubKey && pVPubKey->pVTable != pVTable)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        EnterProviderCritSec(pVTable);
        fCritSec = TRUE;

        fRet = CPVerifySignature(
                            pVTable->hProv,
                            pVHash->hHash,
                            pbSignature,
                            dwSigLen,
                            (pVPubKey == NULL ? 0 : pVPubKey->hKey),
                            NULL,
                            dwFlags
                            );
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        fRet = FALSE;
        ASSERT( fRet );
    }

    if( fCritSec ) {
        LeaveProviderCritSec(pVTable);
    }

    return fRet;
}

/*
 -      CryptSetProvParam
 -
 *      Purpose:
 *                Allows applications to customize various aspects of the
 *                operations of a provider
 *
 *      Parameters:
 *               IN      hProv   -  Handle to a provider
 *               IN      dwParam -  Parameter number
 *               IN      pbData  -  Pointer to data
 *               IN      dwFlags -  Flags values
 *
 *      Returns:
 */
BOOL
WINAPI SCryptSetProvParam(IN HCRYPTPROV hProv,
             IN DWORD dwParam,
             IN BYTE *pbData,
             IN DWORD dwFlags)
{
    PVTableStruc    pVTable = (PVTableStruc) hProv;
    BOOL            fCritSec = FALSE;
    BOOL            fRet = FALSE;

    __try {
        EnterProviderCritSec(pVTable);
        fCritSec = TRUE;

        fRet = CPSetProvParam(pVTable->hProv, dwParam, pbData, dwFlags);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        fRet = FALSE;
        ASSERT( fRet );
    }

    if( fCritSec ) {
        LeaveProviderCritSec(pVTable);
    }

    return fRet;
}


/*
 -      CryptGetProvParam
 -
 *      Purpose:
 *                Allows applications to get various aspects of the
 *                operations of a provider
 *
 *      Parameters:
 *               IN      hProv      -  Handle to a proivder
 *               IN      dwParam    -  Parameter number
 *               IN      pbData     -  Pointer to data
 *               IN      pdwDataLen -  Length of parameter data
 *               IN      dwFlags    -  Flags values
 *
 *      Returns:
 */
BOOL
WINAPI SCryptGetProvParam(IN HCRYPTPROV hProv,
             IN DWORD dwParam,
             IN BYTE *pbData,
             IN DWORD *pdwDataLen,
             IN DWORD dwFlags)
{
    PVTableStruc    pVTable = (PVTableStruc) hProv;
    BOOL            fCritSec = FALSE;
    BOOL            fRet = FALSE;

    __try {
        EnterProviderCritSec(pVTable);
        fCritSec = TRUE;

        fRet = CPGetProvParam(
                        pVTable->hProv,
                        dwParam,
                        pbData,
                        pdwDataLen,
                        dwFlags
                        );
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        fRet = FALSE;
        ASSERT( fRet );
    }

    if( fCritSec ) {
        LeaveProviderCritSec(pVTable);
    }

    return fRet;
}


/*
 -      CryptSetHashParam
 -
 *      Purpose:
 *                Allows applications to customize various aspects of the
 *                operations of a hash
 *
 *      Parameters:
 *               IN      hHash   -  Handle to a hash
 *               IN      dwParam -  Parameter number
 *               IN      pbData  -  Pointer to data
 *               IN      dwFlags -  Flags values
 *
 *      Returns:
 */
BOOL
WINAPI SCryptSetHashParam(IN HCRYPTHASH hHash,
             IN DWORD dwParam,
             IN BYTE *pbData,
             IN DWORD dwFlags)
{
    PVHashStruc     pVHash = (PVHashStruc) hHash;
    PVTableStruc    pVTable;
    BOOL            fCritSec = FALSE;
    BOOL            fRet = FALSE;


    __try {

        pVTable = pVHash->pVTable;

        EnterProviderCritSec(pVTable);
        fCritSec = TRUE;

        fRet = CPSetHashParam(
                        pVTable->hProv,
                        pVHash->hHash,
                        dwParam,
                        pbData,
                        dwFlags
                        );
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        fRet = FALSE;
        ASSERT( fRet );
    }

    if( fCritSec ) {
        LeaveProviderCritSec(pVTable);
    }

    return fRet;
}


/*
 -      CryptGetHashParam
 -
 *      Purpose:
 *                Allows applications to get various aspects of the
 *                operations of a hash
 *
 *      Parameters:
 *               IN      hHash      -  Handle to a hash
 *               IN      dwParam    -  Parameter number
 *               IN      pbData     -  Pointer to data
 *               IN      pdwDataLen -  Length of parameter data
 *               IN      dwFlags    -  Flags values
 *
 *      Returns:
 */
BOOL
WINAPI SCryptGetHashParam(IN HCRYPTKEY hHash,
             IN DWORD dwParam,
             IN BYTE *pbData,
             IN DWORD *pdwDataLen,
             IN DWORD dwFlags)
{
    PVHashStruc     pVHash = (PVHashStruc) hHash;
    PVTableStruc    pVTable;
    BOOL            fCritSec = FALSE;
    BOOL            fRet = FALSE;

    __try {
        pVTable = pVHash->pVTable;

        EnterProviderCritSec(pVTable);
        fCritSec = TRUE;

        fRet = CPGetHashParam(
                        pVTable->hProv,
                        pVHash->hHash,
                        dwParam,
                        pbData,
                        pdwDataLen,
                        dwFlags
                        );
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        fRet = FALSE;
        ASSERT( fRet );
    }

    if( fCritSec ) {
        LeaveProviderCritSec(pVTable);
    }

    return fRet;
}

void __inline EnterProviderCritSec(IN PVTableStruc pVTable)
{
    InterlockedIncrement(&pVTable->Inuse);
}


LONG __inline LeaveProviderCritSec(IN PVTableStruc pVTable)
{
    __try {
        return InterlockedDecrement(&pVTable->Inuse);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

PVKeyStruc BuildVKey(IN PVTableStruc pVTable)
{
    PVKeyStruc      pVKey;

    pVKey = (PVKeyStruc)LocalAlloc(LMEM_ZEROINIT, sizeof(VKeyStruc));
    if( pVKey == NULL )
        return NULL;

    pVKey->pVTable = pVTable;

    return pVKey;
}

PVHashStruc BuildVHash(IN PVTableStruc pVTable)
{
    PVHashStruc     pVHash;

    pVHash = (PVHashStruc)LocalAlloc(LMEM_ZEROINIT, sizeof(VHashStruc));
    if( pVHash == NULL )
        return NULL;

    pVHash->pVTable = pVTable;

    return pVHash;
}
