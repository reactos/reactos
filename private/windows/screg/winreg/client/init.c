/*++


Copyright (c) 1991 Microsoft Corporation

Module Name:

    Init.c

Abstract:

    This module contains the entry point for the Win32 Registry APIs
    client side DLL.

Author:

    David J. Gilman (davegi) 06-Feb-1992

--*/

#include <rpc.h>
#include "regrpc.h"
#include "client.h"

#if DBG
BOOLEAN BreakPointOnEntry = FALSE;
#endif

BOOL LocalInitializeRegCreateKey();
BOOL LocalCleanupRegCreateKey();
BOOL InitializePredefinedHandlesTable();
BOOL CleanupPredefinedHandlesTable();
BOOL InitializeHKEYRestrictedSite();
BOOL InitializeClassesRoot();
BOOL CleanupClassesRoot(BOOL fOnlyThisThread);

#if defined(_REGCLASS_MALLOC_INSTRUMENTED_)

BOOL InitializeInstrumentedRegClassHeap();
BOOL CleanupInstrumentedRegClassHeap();

#endif // defined(_REGCLASS_MALLOC_INSTRUMENTED_)

#if defined(LEAK_TRACK)
BOOL InitializeLeakTrackTable();
BOOL CleanupLeakTrackTable();
#endif // defined (LEAK_TRACK)


enum
{
    ENUM_TABLE_REMOVEKEY_CRITERIA_THISTHREAD = 1,
    ENUM_TABLE_REMOVEKEY_CRITERIA_ANYTHREAD = 2
};

HKEY HKEY_RestrictedSite = NULL;

extern BOOL gbDllHasThreadState ;

BOOL
RegInitialize (
    IN HANDLE   Handle,
    IN DWORD    Reason,
    IN PVOID    Reserved
    )

/*++

Routine Description:

    Returns TRUE.

Arguments:

    Handle      - Unused.

    Reason      - Unused.

    Reserved    - Unused.

Return Value:

    BOOL        - Returns TRUE.

--*/

{
    UNREFERENCED_PARAMETER( Handle );

    switch( Reason ) {

    case DLL_PROCESS_ATTACH:

#ifndef REMOTE_NOTIFICATION_DISABLED
        if( !InitializeRegNotifyChangeKeyValue( ) ||
            !LocalInitializeRegCreateKey() ||
            !InitializePredefinedHandlesTable() ) {
            return( FALSE );

        }
#else
#if defined(_REGCLASS_MALLOC_INSTRUMENTED_)
        if ( !InitializeInstrumentedRegClassHeap()) {
            return FALSE;
        }
#endif // defined(_REGCLASS_MALLOC_INSTRUMENTED_)

        if( !LocalInitializeRegCreateKey() ||
            !InitializePredefinedHandlesTable() ||
            !InitializeHKEYRestrictedSite() ||
            !InitializeClassesRoot()) {
            return( FALSE );

        }
#endif 
#if defined(LEAK_TRACK)
        InitializeLeakTrackTable();
        // ginore errors
#endif // LEAK_TRACK
        return( TRUE );
        break;

    case DLL_PROCESS_DETACH:

        // Reserved == NULL when this is called via FreeLibrary,
        //    we need to cleanup Performance keys.
        // Reserved != NULL when this is called during process exits,
        //    no need to do anything.

        if( Reserved == NULL &&
            !CleanupPredefinedHandles()) {
            return( FALSE );
        }

        if (NULL != HKEY_RestrictedSite)
            NtClose(HKEY_RestrictedSite);

#ifndef REMOTE_NOTIFICATION_DISABLED
        if( !CleanupRegNotifyChangeKeyValue( ) ||
            !LocalCleanupRegCreateKey() ||
            !CleanupPredefinedHandlesTable() ||
            !CleanupClassesRoot( FALSE ) {
            return( FALSE );
        }
#else
        if( !LocalCleanupRegCreateKey() ||
            !CleanupPredefinedHandlesTable() ||
            !CleanupClassesRoot( FALSE )) {
            return( FALSE );
        }
#if defined(LEAK_TRACK)
        CleanupLeakTrackTable();
#endif // LEAK_TRACK
#if defined(_REGCLASS_MALLOC_INSTRUMENTED_)
        if ( !CleanupInstrumentedRegClassHeap()) {
            return FALSE;
        }
#endif // defined(_REGCLASS_MALLOC_INSTRUMENTED_)
#endif

        return( TRUE );
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:

        if ( gbDllHasThreadState ) {

            return CleanupClassesRoot( TRUE );
        }

        break;
    }

    return TRUE;
}



BOOL 
InitializeHKEYRestrictedSite()

/*++

Routine Description:

    If the current process has a restricted token then some registry accesses
    get remapped to HKCU\RestrictedSites\<site sid>

Arguments:

    None

Return Value:


    Return TRUE if the site key was initialized

Notes:

    Actually, don't fail anything.  Stuff that tries to write to the registry
    won't work but everything else should be ok.

--*/

{
    NTSTATUS        status;
    UNICODE_STRING  CurrentUserPath;
    WCHAR           SiteDirectoryBuffer[MAX_MANGLED_SITE];
    LPWSTR          SiteDirectory = SiteDirectoryBuffer;
    UNICODE_STRING  RestrictedSitesPath;
    HANDLE          hToken;
    PSID            sidSite;
    USHORT          MaximumLength;
    PVOID           pvPrevious;

    OBJECT_ATTRIBUTES    attributes;

    status = NtOpenProcessToken(NtCurrentProcess(), TOKEN_QUERY, &hToken);

    if (STATUS_SUCCESS != status)
        goto ErrorOut;

    if (!IsTokenRestricted(hToken))
    {
        NtClose(hToken);
        return TRUE;
    }

    sidSite = GetSiteSidFromToken(hToken);

    NtClose(hToken);

    if (NULL == sidSite)
    {
        status = STATUS_NO_MEMORY;
        goto ErrorOut;
    }

    status = GetMangledSiteSid(sidSite, MAX_MANGLED_SITE, &SiteDirectory);

    RtlFreeSid(sidSite);

    if (S_OK != status)
    {
        status = STATUS_UNSUCCESSFUL;
        goto ErrorOut;
    }
    ASSERT(SiteDirectory == SiteDirectoryBuffer);

    status = RtlFormatCurrentUserKeyPath(&CurrentUserPath);

    if (STATUS_SUCCESS == status) 
    {    
        MaximumLength = (USHORT) (CurrentUserPath.Length 
                                  + sizeof(L"\\RestrictedSites\\")
                                  + lstrlenW(SiteDirectory) * sizeof(WCHAR));

        RestrictedSitesPath.Length        = 0;
        RestrictedSitesPath.MaximumLength = MaximumLength;
        RestrictedSitesPath.Buffer        = RtlAllocateHeap(
                                                    RtlProcessHeap(), 
                                                    0, 
                                                    MaximumLength);
        if (NULL != RestrictedSitesPath.Buffer)
        {
            RtlCopyUnicodeString(&RestrictedSitesPath, &CurrentUserPath);
            RtlAppendUnicodeToString(
                            &RestrictedSitesPath, 
                            L"\\RestrictedSites\\");
            RtlAppendUnicodeToString(&RestrictedSitesPath, SiteDirectory);

            InitializeObjectAttributes(
                    &attributes,
                    &RestrictedSitesPath,
                    OBJ_CASE_INSENSITIVE,
                    NULL,
                    NULL)

            status = NtOpenKey(
                            &HKEY_RestrictedSite, 
                            MAXIMUM_ALLOWED, 
                            &attributes);
                                
            RtlFreeHeap(RtlProcessHeap(), 0, RestrictedSitesPath.Buffer);
        }
        else
        {
            status = STATUS_NO_MEMORY;
        }

        RtlFreeUnicodeString(&CurrentUserPath);
    }

ErrorOut:

#if DBG
    if (STATUS_SUCCESS != status)
        DbgPrint("InitializeHKEYRestrictedSite failed (0x%08x)\n", status);
#endif

    return TRUE;
}
