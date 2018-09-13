//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       rng.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    8-15-95   RichardW   Created
//              2-8-96    MikeSw     Copied to NTLMSSP from SSL
//              03-Aug-1996 ChandanS  Stolen from net\svcdlls\ntlmssp\common\rng.c
//              8-Dec-1997 SField    Use crypto group RNG
//
//----------------------------------------------------------------------------

#include <global.h>
//#include <windows.h>
#include <wincrypt.h>

#include <rc4.h>

HCRYPTPROV g_hProv = NULL;
PVOID g_pLockedMemory = NULL;
ULONG g_cbLockedMemory = 0;


NTSTATUS
SspGenerateRandomBits(
    VOID        *pRandomData,
    ULONG       cRandomData
    )
{
    BOOL fRet = FALSE;
    ASSERT( g_hProv );

    if( g_hProv ) {
        fRet = CryptGenRandom( g_hProv, cRandomData, (BYTE*)pRandomData );
        ASSERT( fRet );

        if( fRet )
            return STATUS_SUCCESS;
    }


    //
    // return a nebulous error message.  this is better than returning
    // success which could compromise security with non-random key.
    //

    return STATUS_UNSUCCESSFUL;
}


BOOL
NtLmInitializeRNG(VOID)
{
    BOOL fSuccess;

    if( g_hProv != NULL ) {

        //
        // the g_hProv has already been set.  This is the case when
        // client side msv1_0 is called from the LSA process.  We avoid
        // an additional AcquireContext() and subsequent leak.  This also
        // addresses a problem where CryptAcquireContext() fails the second
        // time if the first client side msv1_0 call originates from
        // a client thread which is impersonating Anonymous, as is the case
        // for a password change.
        //

        return TRUE;
    }

    fSuccess = CryptAcquireContext(
                    &g_hProv,
                    NULL,
                    NULL,
                    PROV_RSA_FULL,
                    CRYPT_VERIFYCONTEXT
                    );

    ASSERT( fSuccess );
    ASSERT( g_hProv != NULL);

    return fSuccess;
}

VOID
NtLmCleanupRNG(VOID)
{
    if( g_hProv ) {
        CryptReleaseContext( g_hProv, 0 );
        g_hProv = NULL;
    }
}

BOOL
SspProtectMemory(
    VOID        *pData,
    ULONG       cbData
    )
{
    RC4_KEYSTRUCT rc4key;

    if( pData == NULL || cbData == 0 )
        return TRUE;

    if( g_pLockedMemory == NULL )
        return FALSE;

    rc4_key( &rc4key, g_cbLockedMemory, (unsigned char*)g_pLockedMemory );
    rc4( &rc4key, cbData, (unsigned char *)pData );

    ZeroMemory( &rc4key, sizeof(rc4key) );

    return TRUE;
}


BOOL
SspUnprotectMemory(
    VOID        *pData,
    ULONG       cbData
    )
{
    return SspProtectMemory( pData, cbData );
}

VOID
NtLmCleanupProtectedMemory(VOID)
{
    if( g_pLockedMemory ) {
        ZeroMemory( g_pLockedMemory, g_cbLockedMemory );
        VirtualFree( g_pLockedMemory, 0, MEM_RELEASE );
        g_pLockedMemory = NULL;
    }
}

BOOL
NtLmInitializeProtectedMemory(VOID)
{
    //
    // locked enough memory to contain the maximum size key the algorithm
    // supports.
    //

    g_cbLockedMemory = 256;

    g_pLockedMemory = VirtualAlloc(
                                    NULL,
                                    g_cbLockedMemory,
                                    MEM_COMMIT,
                                    PAGE_READWRITE
                                    );

    if( g_pLockedMemory == NULL )
        return FALSE;

    //
    // lock memory.
    //

    VirtualLock( g_pLockedMemory, g_cbLockedMemory );

    //
    // generate random key in page locked memory.
    //

    if(!NT_SUCCESS(SspGenerateRandomBits( g_pLockedMemory, g_cbLockedMemory ))) {
        NtLmCleanupProtectedMemory();
        return FALSE;
    }

    return TRUE;
}
