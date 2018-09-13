/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    nlnetapi.c

Abstract:

   This module loads Netapi.dll at runtime and sets up pointers to
   the APIs called by Msv1_0.

Author:

    Dave Hart (DaveHart) 25-Mar-1992

Environment:

    User mode Win32 - msv1_0 authentication package DLL

Revision History:
    Dave Hart (DaveHart) 26-Mar-1992
        Added RxNetUserPasswordSet.

    Dave Hart (DaveHart) 30-May-1992
        Removed NetRemoteComputerSupports, added NetApiBufferAllocate.

    Chandana Surlu       21-Jul-1996
        Stolen from \\kernel\razzle3\src\security\msv1_0\nlnetapi.c

    Scott Field (sfield) 19-May-1999
        Use DELAYLOAD against ntdsapi.dll and netapi32.dll

--*/

#include "msp.h"
#include "nlp.h"

typedef NTSTATUS
            (*PI_NetNotifyNetlogonDllHandle) (
                IN PHANDLE Role
            );




VOID
NlpLoadNetlogonDll (
    VOID
    )

/*++

Routine Description:

    Uses Win32 LoadLibrary and GetProcAddress to get pointers to functions
    in Netlogon.dll that are called by Msv1_0.

Arguments:

    None.

Return Value:

    None.  If successful, NlpNetlogonDllHandle is set to non-null and function
    pointers are setup.


--*/

{
    HANDLE hModule = NULL;
    PI_NetNotifyNetlogonDllHandle pI_NetNotifyNetlogonDllHandle = NULL;



    //
    // Load netlogon.dll also.
    //

    hModule = LoadLibraryA("netlogon");

    if (NULL == hModule) {
#if DBG
        DbgPrint("Msv1_0: Unable to load netlogon.dll, Win32 error %d.\n", GetLastError());
#endif
        goto Cleanup;
    }




    NlpNetLogonSamLogon = (PNETLOGON_SAM_LOGON_PROCEDURE)
        GetProcAddress(hModule, "NetrLogonSamLogon");

    if (NlpNetLogonSamLogon == NULL) {
#if DBG
        DbgPrint(
            "Msv1_0: Can't find entrypoint NetrLogonSamLogon in netlogon.dll.\n"
            "        Win32 error %d.\n", GetLastError());
#endif
        goto Cleanup;
    }




    NlpNetLogonSamLogoff = (PNETLOGON_SAM_LOGOFF_PROCEDURE)
        GetProcAddress(hModule, "NetrLogonSamLogoff");

    if (NlpNetLogonSamLogoff == NULL) {
#if DBG
        DbgPrint(
            "Msv1_0: Can't find entrypoint NetrLogonSamLogoff in netlogon.dll.\n"
            "        Win32 error %d.\n", GetLastError());
#endif
        goto Cleanup;
    }


    //
    // Find the address of the I_NetNotifyNetlogonDllHandle procedure.
    //  This is an optional procedure so don't complain if it isn't there.
    //

    pI_NetNotifyNetlogonDllHandle = (PI_NetNotifyNetlogonDllHandle)
        GetProcAddress( hModule, "I_NetNotifyNetlogonDllHandle" );




    //
    // Found all the functions needed, so indicate success.
    //

    NlpNetlogonDllHandle = hModule;
    hModule = NULL;

    //
    // Notify Netlogon that we've loaded it.
    //

    if( pI_NetNotifyNetlogonDllHandle != NULL ) {
        (VOID) (*pI_NetNotifyNetlogonDllHandle)( &NlpNetlogonDllHandle );
    }


Cleanup:
    if ( hModule != NULL ) {
        FreeLibrary( hModule );
    }

    return;

}
