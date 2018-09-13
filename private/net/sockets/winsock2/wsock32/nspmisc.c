/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    NspMisc.c

Abstract:

    This module contains misc support routines for loading
    Name Space providers.

Author:

    David Treadwell (davidtr)    22-Apr-1994

Revision History:

    25-May-1994  ChuckC          Split off from nspgaddr.c.

--*/

#if defined(CHICAGO)
#undef UNICODE
#else
#define UNICODE
#define _UNICODE
#endif

#include "winsockp.h"
#include <nspmisc.h>
#include <stdlib.h>

#if defined(CHICAGO)
PTSTR
KludgeMultiSz(
    HKEY hkey,
    LPDWORD lpdwLength
    );
#endif  // CHICAGO


BOOL NspInitialized = FALSE;

LIST_ENTRY NameSpaceListHead;

PDWORD DefaultExclusions;
DWORD DefaultExclusionCount;



INT
InitializeNsp (
    VOID
    )

/*++

Routine Description:

    Initializes internal structures for the NSP APIs.

Arguments:

    None.

Return Value:

    INT - NO_ERROR if initialization was successful, or a Windows error
        code if the NSP could not be initialized.

--*/

{
    INT error;
    PTSTR providerList;
    DWORD providerCount;

    //
    // While holding the global lock, check whether we are actually
    // initialized.  We must do this with the global lock held so that
    // we synchronize between multiple threads simultaneously making RNR
    // calls.
    //

    SockAcquireGlobalLockExclusive( );

    if ( NspInitialized ) {
        SockReleaseGlobalLock( );
        return NO_ERROR;
    }

    //
    // Initialize global variables.
    //

    InitializeListHead( &NameSpaceListHead );

    //
    // Read the list of name spaces excluded from default resolutions.
    //

    error = ReadDefaultExclusions( );
    if ( error != NO_ERROR ) {
        SockReleaseGlobalLock( );
        return error;
    }

    //
    // Get the list of network provider DLLs from the registry.
    //

    error = GetProviderList( &providerList, &providerCount );
    if ( error != NO_ERROR ) {
        SockReleaseGlobalLock( );
        return error;
    }

    //
    // For each network provider, check if it can act as a name space
    // provider and if so load the provider DLL.  It is OK if the load
    // of a single DLL fails as long as at least one DLL successfully
    // loads.
    //
    // LoadNspDll() loads all the name spaces exported by the provider
    // and places them in the global list in order of priority.
    //

    for ( ; providerCount > 0; providerCount-- ) {

        LoadNspDll( providerList );

        providerList += _tcslen( providerList ) + 1;
    }

    //
    // If no name spaces loaded, fail.
    //

    if ( IsListEmpty( &NameSpaceListHead ) ) {
        SockReleaseGlobalLock( );
        return ERROR_NOT_SUPPORTED;
    }

    NspInitialized = TRUE;

    SockReleaseGlobalLock( );

    return NO_ERROR;

} // InitializeNsp


INT
GetProviderList (
    OUT PTSTR *ProviderList,
    OUT PDWORD ProviderCount
    )
{
    INT error;
    DWORD providerListLength;
    HKEY providerOrderKey;
    DWORD type;
    PTSTR lpTmp, lpEnd;

    //
    // Open the key that stores the list network providers.
    //

    error = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                PROVIDER_ORDER_KEY_NAME,
                0,
                KEY_READ,
                &providerOrderKey
                );
    if ( error != NO_ERROR ) {
        return error;
    }

#if defined(CHICAGO)
    *ProviderList = KludgeMultiSz( providerOrderKey, &providerListLength );

    if( *ProviderList == NULL )
    {
        error = (INT)GetLastError();
        RegCloseKey( providerOrderKey );
        return error;
    }
#else   // !CHICAGO
    //
    // Determine the size of the provider list.  We need this so that we
    // can allocate enough memory to hold it.
    //

    providerListLength = 0;

    error = RegQueryValueEx(
                providerOrderKey,
                TEXT("ProviderOrder"),
                NULL,
                &type,
                NULL,
                &providerListLength
                );
    if ( error != ERROR_MORE_DATA && error != NO_ERROR ) {
        RegCloseKey( providerOrderKey );
        return error;
    }

    //
    // Allocate enough memory to hold the provider list.
    // Add a bit more for safety.
    //

    *ProviderList = ALLOCATE_HEAP( providerListLength + 16 );
    if ( *ProviderList == NULL ) {
        RegCloseKey( providerOrderKey );
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    memset(*ProviderList,
           0,
           providerListLength+16) ;

    //
    // Get the list of providers from the registry.
    //

    error = RegQueryValueEx(
                providerOrderKey,
                TEXT("ProviderOrder"),
                NULL,
                &type,
                (PVOID)*ProviderList,
                &providerListLength
                );

    RegCloseKey( providerOrderKey );

    if ( error != NO_ERROR ) {
        FREE_HEAP( *ProviderList );
        return error;
    }
#endif  // CHICAGO

    //
    // Figure out how many providers there are. Take care of simple
    // case first.
    //

    *ProviderCount = 0 ;
    lpTmp = *ProviderList ;
    lpEnd = *ProviderList + providerListLength ;

    if ( *lpTmp == 0 )
    {
        return NO_ERROR ;
    }

    //
    // for all strings in the multi sz
    //
    do
    {
        *ProviderCount += 1 ;

        //
        // get to end of this string
        //
        while ((lpTmp < lpEnd) && *lpTmp)
        {
            lpTmp++ ;
        }

        //
        // if no more, quit. this should not happen unless we got bad
        // data that is not a multi sz. but better to play it safe than
        // blow up.
        //
        if (lpTmp >= lpEnd)
            break ;

        //
        // check for second NULL terminator
        //
        lpTmp++ ;
        if ((lpTmp >= lpEnd) || !*lpTmp)
            break ;

    } while (TRUE) ;

    return NO_ERROR;

} // GetProviderList


INT
LoadNspDll (
    IN PTSTR ProviderName
    )
{
    INT error;
    HKEY providerKey;
    DWORD type;
    DWORD class;
    DWORD entryLength;
    PTSTR providerDllName;
    PTSTR providerDllExpandedName;
    DWORD providerDllExpandedNameLength;
    HANDLE providerDllHandle;
    LPLOAD_NAME_SPACE_PROC loadNameSpaceProc;
    PNS_ROUTINE nsRoutine;
    DWORD nspVersion;
    DWORD nameSpaceCount;
    PTSTR providerKeyName;

    //
    // Allocate space to hold the provider key name.
    //

    providerKeyName = ALLOCATE_HEAP( DOS_MAX_PATH_LENGTH*sizeof(TCHAR) );
    if ( providerKeyName == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Build the name of the network provider's information key.
    //

    _tcscpy( providerKeyName, REG_SERVICES_ROOT );
    _tcscat( providerKeyName, ProviderName );
    _tcscat( providerKeyName, TEXT("\\ServiceProvider") );

    //
    // Open the root key that holds subkeys for each of the network
    // providers.
    //

    error = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                providerKeyName,
                0,
                KEY_READ,
                &providerKey
                );
    FREE_HEAP( providerKeyName );
    if ( error != NO_ERROR ) {
        return error;
    }

    //
    // Determine the class of the provider DLL.  If the WN_SERVICE_CLASS
    // bit is not set, then we're not interested in this provider.
    //

    entryLength = sizeof(class);

    error = RegQueryValueEx(
                providerKey,
                TEXT("Class"),
                NULL,
                &type,
                (PVOID)&class,
                &entryLength
                );
    if ( error != NO_ERROR ) {
        RegCloseKey( providerKey );
        return error;
    }

    if ( (class & WN_SERVICE_CLASS) == 0 ) {
        RegCloseKey( providerKey );
        return ERROR_NOT_SUPPORTED;
    }

    //
    // The provider is an NSP, so we'll need to actually load this DLL.
    // First determine the name of the DLL.  Allocate some memory to
    // hold the name and expanded name.
    //

    providerDllName = ALLOCATE_HEAP( DOS_MAX_PATH_LENGTH*sizeof(TCHAR) );
    if ( providerDllName == NULL ) {
        RegCloseKey( providerKey );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    providerDllExpandedName = ALLOCATE_HEAP( DOS_MAX_PATH_LENGTH*sizeof(TCHAR) );
    if ( providerDllExpandedName == NULL ) {
        RegCloseKey( providerKey );
        FREE_HEAP( providerDllName );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Get the name of the provider DLL.
    //

    entryLength = DOS_MAX_PATH_LENGTH*sizeof(TCHAR);

    error = RegQueryValueEx(
                providerKey,
                TEXT("ProviderPath"),
                NULL,
                &type,
                (PVOID)providerDllName,
                &entryLength
                );
    if ( error != NO_ERROR ) {
        FREE_HEAP( providerDllName );
        FREE_HEAP( providerDllExpandedName );
        RegCloseKey( providerKey );
        return error;
    }

    //
    // Expand the name of the DLL, converting environment variables to
    // their corresponding strings.
    //

    providerDllExpandedNameLength = ExpandEnvironmentStrings(
                                        providerDllName,
                                        providerDllExpandedName,
                                        DOS_MAX_PATH_LENGTH
                                        );
    FREE_HEAP( providerDllName );

    //
    // Load the provider DLL so that we can get at it's entry points.
    //

    providerDllHandle = LoadLibrary( providerDllExpandedName );

    FREE_HEAP( providerDllExpandedName );
    RegCloseKey( providerKey );

    if ( providerDllHandle == NULL ) {
        return GetLastError( );;
    }

    //
    // Get the address of the load name spaces procedure.
    //

    loadNameSpaceProc = (LPLOAD_NAME_SPACE_PROC)
        GetProcAddress( providerDllHandle, "NPLoadNameSpaces" );

    if ( loadNameSpaceProc == NULL ) {
        FreeLibrary( providerDllHandle );
        return GetLastError( );
    }

    //
    // Call the load name spaces procedure to determine the buffer size
    // we'll need.
    //

    nspVersion = NSP_VERSION;

    (VOID)loadNameSpaceProc( &nspVersion, NULL, &entryLength );

    //
    // If the DLL does not support at least our version of the NSP
    // interface, fail.
    //

    if ( nspVersion < NSP_VERSION ) {
        FreeLibrary( providerDllHandle );
        return ERROR_NOT_SUPPORTED;
    }

    //
    // Allocate some space to hold return information from the load name
    // spaces procedure.
    //

    nsRoutine = ALLOCATE_HEAP( entryLength );
    if ( nsRoutine == NULL ) {
        FreeLibrary( providerDllHandle );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Call the load name spaces procedure with correct values to actually
    // load all the name spaces and associated information.
    //

    nspVersion = NSP_VERSION;

    nameSpaceCount = loadNameSpaceProc( &nspVersion, nsRoutine, &entryLength );
    if ( nameSpaceCount <= 0 ) {
        FreeLibrary( providerDllHandle );
        FREE_HEAP( nsRoutine );
        return -1;
    }

    //
    // For each name space, allocate and initialize an information
    // structure with pertinent information and place the structure
    // on our global list.
    //

    for ( ; nameSpaceCount != 0; nameSpaceCount--, nsRoutine++ ) {

        PNAME_SPACE_INFO nameSpaceInfo;
        DWORD i;

        //
        // Allocate the structure.  If the allocation fails, ignore the
        // failure.  There isn't anything intelligent that we can do
        // about it, and an allocation failure this small should be
        // extremely rare.
        //

        nameSpaceInfo = ALLOCATE_HEAP( sizeof(*nameSpaceInfo) );
        if ( nameSpaceInfo == NULL ) {
            continue;
        }

        //
        // Store the DLL handle in this structure, then NULL out the
        // handle value so that only the first name space for a specific
        // DLL really stires the DLL handle.  We use this handle to
        // correctly free NSP DLLs.
        //

        nameSpaceInfo->ProviderDllHandle = providerDllHandle;
        providerDllHandle = NULL;

        //
        // Fill in other information on the name space provider.
        // Note the assumption about consecutive numbering. Ie. if
        // 2 functions are returned, we assume they are for Index 0
        // and Index 1.
        //

        nameSpaceInfo->GetAddrByNameProc = (LPGET_ADDR_BY_NAME_PROC)
            ((nsRoutine->dwFunctionCount > NSPAPI_GET_ADDRESS_BY_NAME) ?
                nsRoutine->alpfnFunctions[NSPAPI_GET_ADDRESS_BY_NAME] :
                NULL) ;

        nameSpaceInfo->GetServiceProc = (LPGET_SERVICE_PROC)
            ((nsRoutine->dwFunctionCount > NSPAPI_GET_SERVICE) ?
                nsRoutine->alpfnFunctions[NSPAPI_GET_SERVICE] :
                NULL) ;

        nameSpaceInfo->SetServiceProc = (LPSET_SERVICE_PROC)
            ((nsRoutine->dwFunctionCount > NSPAPI_SET_SERVICE) ?
                nsRoutine->alpfnFunctions[NSPAPI_SET_SERVICE] :
                NULL) ;

        nameSpaceInfo->FunctionCount = nsRoutine->dwFunctionCount;
        nameSpaceInfo->Priority = nsRoutine->dwPriority;
        nameSpaceInfo->NameSpace = nsRoutine->dwNameSpace;

        //
        // Determine whether this name space should be excluded from
        // default searches.
        //

        nameSpaceInfo->EnabledByDefault = TRUE;

        for ( i = 0; i < DefaultExclusionCount; i++ ) {
            if ( nameSpaceInfo->NameSpace == DefaultExclusions[i] ) {
                nameSpaceInfo->EnabledByDefault = FALSE;
            }
        }

        //
        // Place the name space on our global list in order of it's
        // priority value.
        //

        InsertNameSpace( nameSpaceInfo );
    }

    return NO_ERROR;

} // LoadNspDll


VOID
InsertNameSpace (
    IN PNAME_SPACE_INFO NameSpace
    )
{
    PLIST_ENTRY listEntry;
    PNAME_SPACE_INFO testNameSpace;

    //
    // Walk the global list of name spaces until we either reach the end
    // or reach a name space with a higher priority value than the one
    // we're inserting.
    //

    for ( listEntry = NameSpaceListHead.Flink;
          listEntry != &NameSpaceListHead;
          listEntry = listEntry->Flink ) {

        testNameSpace = CONTAINING_RECORD(
                            listEntry,
                            NAME_SPACE_INFO,
                            NameSpaceListEntry
                            );

        if ( testNameSpace->Priority > NameSpace->Priority ) {
            break;
        }
    }

    //
    // Insert the name space just before the one we found.
    //

    InsertTailList( listEntry, &NameSpace->NameSpaceListEntry );

    return;

} // InsertNameSpace


INT
ReadDefaultExclusions (
    VOID
    )
{
    INT error;
    DWORD exclusionsListLength;
    HKEY providerOrderKey;
    DWORD type;
    PTSTR exclusionsList;
    DWORD i;
    PTCHAR w;

    //
    // Initialize the default exclusion count to 0.  For many errors, we
    // will assume that no providers are excluded from default searches.
    //

    DefaultExclusions = NULL;
    DefaultExclusionCount = 0;

    //
    // Open the key that stores the list network providers.
    //

    error = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                EXCLUDED_PROVIDERS_KEY_NAME,
                0,
                KEY_READ,
                &providerOrderKey
                );
    if ( error != NO_ERROR ) {
        return error;
    }

#if defined(CHICAGO)
    exclusionsList = KludgeMultiSz( providerOrderKey, &exclusionsListLength );

    if( exclusionsList == NULL )
    {
        error = (INT)GetLastError();
        RegCloseKey( providerOrderKey );
        return error;
    }
#else   // !CHICAGO
    //
    // Determine the size of the provider list.  We need this so that we
    // can allocate enough memory to hold it.
    //

    exclusionsListLength = 0;

    error = RegQueryValueEx(
                providerOrderKey,
                TEXT("ExcludedProviders"),
                NULL,
                &type,
                NULL,
                &exclusionsListLength
                );
    if ( error != ERROR_MORE_DATA && error != NO_ERROR ) {
        RegCloseKey( providerOrderKey );
        return NO_ERROR;
    }

    //
    // Allocate enough memory to hold the provider list.
    //

    exclusionsList = ALLOCATE_HEAP( exclusionsListLength );
    if ( exclusionsList == NULL ) {
        RegCloseKey( providerOrderKey );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Get the list of providers from the registry.
    //

    error = RegQueryValueEx(
                providerOrderKey,
                TEXT("ExcludedProviders"),
                NULL,
                &type,
                (PVOID)exclusionsList,
                &exclusionsListLength
                );

    RegCloseKey( providerOrderKey );

    if ( error != NO_ERROR ) {
        FREE_HEAP( exclusionsList );
        return error;
    }
#endif  // CHICAGO

    //
    // Count the number of providers which are excluded from default
    // searches.
    //

    DefaultExclusionCount = 0;

    for ( w = exclusionsList; *w != L'\0'; w += _tcslen( w ) + 1 ) {
        DefaultExclusionCount++;
    }

    //
    // Allocate space for the exclusions array.
    //

    DefaultExclusions = ALLOCATE_HEAP( sizeof(DWORD) * (DefaultExclusionCount+1) );
    if ( DefaultExclusions == NULL ) {
        FREE_HEAP( exclusionsList );
        DefaultExclusionCount = 0;
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Convert the REG_MULTI_SZ from the registry into an array of DWORDs.
    //

    for ( i = 0, w = exclusionsList; *w != L'\0'; i++, w += _tcslen( w ) + 1 ) {
        TCHAR *end;
        DefaultExclusions[i] = _tcstol( w, &end, 0 );
    }

    DefaultExclusions[DefaultExclusionCount] = 0;

    FREE_HEAP( exclusionsList );

    return NO_ERROR;

} // ReadDefaultExclusions


BOOL
IsValidNameSpace (
    IN DWORD dwNameSpaces,
    IN PNAME_SPACE_INFO NameSpace
    )
{

    //
    // If the request is for the default name spaces, this one is valid
    // as long as it is enabled.
    //

    if ( dwNameSpaces == NS_DEFAULT ) {

        if ( NameSpace->EnabledByDefault ) {
            return TRUE;
        } else {
            return FALSE;
        }
    }

    //
    // The request is for a specific name space.  It is only valid if
    // this is the same name space.
    //

    if ( dwNameSpaces == NameSpace->NameSpace ) {
        return TRUE;
    } else {
        return FALSE;
    }

} // IsValidNameSpace


BOOL
GuidEqual (
    IN LPGUID Guid1,
    IN LPGUID Guid2
    )
{
    //
    // Check if two GUIDs are equal in every respect.
    //

    if ( Guid1->Data1 == Guid2->Data1 &&
         Guid1->Data2 == Guid2->Data2 &&
         Guid1->Data3 == Guid2->Data3 &&
         Guid1->Data4[0] == Guid2->Data4[0] &&
         Guid1->Data4[1] == Guid2->Data4[1] &&
         Guid1->Data4[2] == Guid2->Data4[2] &&
         Guid1->Data4[3] == Guid2->Data4[3] &&
         Guid1->Data4[4] == Guid2->Data4[4] &&
         Guid1->Data4[5] == Guid2->Data4[5] &&
         Guid1->Data4[6] == Guid2->Data4[6] &&
         Guid1->Data4[7] == Guid2->Data4[7] ) {

        return TRUE;
    }

    return FALSE;

} // CompareGuids

#if defined(CHICAGO)

//
//  Chicago does not support the REG_MULTI_SZ registry value.  As
//  a hack (er, workaround), we'll create *keys* in the registry
//  in place of REG_MULTI_SZ *values*.  We'll then use the names
//  of any values under the key as the REG_MULTI_SZ entries.  So,
//  instead of this:
//
//      ..\Control\ServiceProvider
//          ProviderOrder = REG_MULTI_SZ "MSTCP"
//                                       "NWLINK"
//                                       "FOOBAR"
//
//  We'll use this:
//
//      ..\Control\Service\Provider\ProviderOrder
//          MSTCP = REG_SZ ""
//          NWLINK = REG_SZ ""
//          FOOBAR = REG_SZ ""
//
//  This function takes an open registry key handle, enumerates
//  the names of values contained within the key, and constructs
//  a REG_MULTI_SZ string from the value names.
//
//  Note that this function is not multithread safe; if another
//  thread (or process) creates or deletes values under the
//  specified key, the results are indeterminate.
//
//  This function returns NULL on error.  It returns non-NULL
//  on success, even if the resulting REG_MULTI_SZ is empty.
//
PTSTR
KludgeMultiSz(
    HKEY hkey,
    LPDWORD lpdwLength
    )
{
    LONG  err;
    DWORD iValue;
    DWORD cchTotal;
    DWORD cchValue;
    char  szValue[MAX_PATH];
    LPSTR lpMultiSz;
    LPSTR lpTmp;
    LPSTR lpEnd;

    //
    //  Enumerate the values and total up the lengths.
    //

    iValue = 0;
    cchTotal = 0;

    for( ; ; )
    {
        cchValue = sizeof(szValue);

        err = RegEnumValue( hkey,
                            iValue,
                            szValue,
                            &cchValue,
                            NULL,
                            NULL,
                            NULL,
                            NULL );

        if( err != NO_ERROR )
        {
            break;
        }

        //
        //  Add the length of the value's name, plus one
        //  for the terminator.
        //

        cchTotal += strlen( szValue ) + 1;

        //
        //  Advance to next value.
        //

        iValue++;
    }

    //
    //  Add one for the final terminating NULL.
    //

    cchTotal++;
    *lpdwLength = cchTotal;

    //
    //  Allocate the MULTI_SZ buffer.
    //

    lpMultiSz = ALLOCATE_HEAP( cchTotal );

    if( lpMultiSz == NULL )
    {
        return NULL;
    }

    memset( lpMultiSz, 0, cchTotal );

    //
    //  Enumerate the values and append to the buffer.
    //

    iValue = 0;
    lpTmp = lpMultiSz;
    lpEnd = lpMultiSz + cchTotal;

    for( ; ; )
    {
        cchValue = sizeof(szValue);

        err = RegEnumValue( hkey,
                            iValue,
                            szValue,
                            &cchValue,
                            NULL,
                            NULL,
                            NULL,
                            NULL );

        if( err != NO_ERROR )
        {
            break;
        }

        //
        //  Compute the length of the value name (including
        //  the terminating NULL).
        //

        cchValue = strlen( szValue ) + 1;

        //
        //  Determine if there is room in the array, taking into
        //  account the second NULL that terminates the string list.
        //

        if( ( lpTmp + cchValue + 1 ) > lpEnd )
        {
            break;
        }

        //
        //  Append the value name.
        //

        strcpy( lpTmp, szValue );
        lpTmp += cchValue;

        //
        //  Advance to next value.
        //

        iValue++;
    }

    //
    //  Success!
    //

    return (PTSTR)lpMultiSz;

}   // KludgeMultiSzBecauseChicagoIsSoLame (the original name, left for posterity)
#endif
