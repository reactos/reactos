/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    RegSec.c

Abstract:

    This module contains code to apply security to the otherwise unsecured
    top level keys, in fashion that will allow existing consumers access to
    the keys that they need (print, srvmgr, etc.).


Author:

    Richard Ward (richardw) 15 May 1996

Notes:

--*/


#include <rpc.h>
#include <string.h>
#include <wchar.h>
#include "regrpc.h"
#include "localreg.h"
#include "regsec.h"

#define REGSEC_READ     1
#define REGSEC_WRITE    2


WCHAR                   RemoteRegistryKey[] = L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\SecurePipeServers\\winreg";
WCHAR                   AllowedPathsKey[] = L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\SecurePipeServers\\winreg\\AllowedPaths";
WCHAR                   MachineValue[] = L"Machine";
WCHAR                   UsersValue[] = L"Users";
PSECURITY_DESCRIPTOR    RemoteRegistrySD;
PUNICODE_STRING         MachineAllowedPaths;
PUCHAR                  MachineAllowedPathsBase;
ULONG                   MachineAllowedPathsCount;
PUNICODE_STRING         UsersAllowedPaths;
PUCHAR                  UsersAllowedPathsBase;
ULONG                   UsersAllowedPathsCount;
GENERIC_MAPPING         RemoteRegistryMappings;

LARGE_INTEGER           WinregChange ;
LARGE_INTEGER           AllowedPathsChange ;
RTL_RESOURCE            RegSecReloadLock ;





NTSTATUS
RegSecReadSDFromRegistry(
    IN  HANDLE  hKey,
    OUT PSECURITY_DESCRIPTOR *  pSDToUse)
/*++

Routine Description:

    This function checks the registry in the magic place to see if an extra
    ACL has been defined for the pipe being passed in.  If there is one, it
    is translated to a NP acl, then returned.  If there isn't one, or if
    something goes wrong, an NULL acl is returned.

Arguments:

    InterfaceName   name of the pipe to check for, e.g. winreg, etc.

    pSDToUse        returned a pointer to the security decriptor to use.

Return Value:

    STATUS_SUCCESS,
    STATUS_NO_MEMORY,
    Possible other errors from registry apis.


--*/
{
    NTSTATUS                Status ;
    PSECURITY_DESCRIPTOR    pSD;
    ULONG                   cbNeeded;
    ACL_SIZE_INFORMATION    AclSize;
    ULONG                   AceIndex;
    ACCESS_MASK             NewMask;
    PACCESS_ALLOWED_ACE     pAce;
    PACL                    pAcl;
    BOOLEAN                 DaclPresent;
    BOOLEAN                 DaclDefaulted;
    UNICODE_STRING          Interface;
    UNICODE_STRING          Allowed;
    ULONG                   i;
    BOOLEAN                 PipeNameOk;
    PSECURITY_DESCRIPTOR    pNewSD;
    PACL                    pNewAcl;
    PSID                    pSid;
    PSID                    pSidCopy;
    BOOLEAN                 Defaulted;

    *pSDToUse = NULL;


    //
    // Son of a gun, someone has established security for this pipe.
    //

    pSD = NULL;

    cbNeeded = 0;
    Status = NtQuerySecurityObject(
                    hKey,
                    DACL_SECURITY_INFORMATION |
                        OWNER_SECURITY_INFORMATION |
                        GROUP_SECURITY_INFORMATION,
                    NULL,
                    0,
                    &cbNeeded );

    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        pSD = RtlAllocateHeap(RtlProcessHeap(), 0, cbNeeded);
        if (pSD)
        {
            Status = NtQuerySecurityObject(
                        hKey,
                        DACL_SECURITY_INFORMATION |
                            OWNER_SECURITY_INFORMATION |
                            GROUP_SECURITY_INFORMATION,
                        pSD,
                        cbNeeded,
                        &cbNeeded );


            if (NT_SUCCESS(Status))
            {
                //
                // Now, the tricky part.  There is no 1-1 mapping of Key
                // permissions to Pipe permissions.  So, we do it here.
                // We walk the DACL, and examine each ACE.  We build a new
                // access mask for each ACE, and set the flags as follows:
                //
                //  if (KEY_READ) GENERIC_READ
                //  if (KEY_WRITE) GENERIC_WRITE
                //

                Status = RtlGetDaclSecurityDescriptor(
                                        pSD,
                                        &DaclPresent,
                                        &pAcl,
                                        &DaclDefaulted);


                //
                // If this failed, or there is no DACL present, then
                // we're in trouble.
                //

                if (!NT_SUCCESS(Status) || !DaclPresent)
                {
                    goto GetSDFromKey_BadAcl;
                }


                Status = RtlQueryInformationAcl(pAcl,
                                                &AclSize,
                                                sizeof(AclSize),
                                                AclSizeInformation);

                if (!NT_SUCCESS(Status))
                {
                    goto GetSDFromKey_BadAcl;
                }

                for (AceIndex = 0; AceIndex < AclSize.AceCount ; AceIndex++ )
                {
                    NewMask = 0;
                    Status = RtlGetAce( pAcl,
                                        AceIndex,
                                        & pAce);

                    //
                    // We don't care what kind of ACE it is, since we
                    // are just mapping the access types, and the access
                    // mask is always at a constant position.
                    //

                    if (NT_SUCCESS(Status))
                    {
                        if ((pAce->Header.AceType != ACCESS_ALLOWED_ACE_TYPE) &&
                            (pAce->Header.AceType != ACCESS_DENIED_ACE_TYPE))
                        {
                            //
                            // Must be an audit or random ACE type.  Skip it.
                            //

                            continue;

                        }


                        if (pAce->Mask & KEY_READ)
                        {
                            NewMask |= REGSEC_READ;
                        }

                        if (pAce->Mask & KEY_WRITE)
                        {
                            NewMask |= REGSEC_WRITE;
                        }

                        pAce->Mask = NewMask;
                    }
                    else
                    {
                        //
                        // Panic:  Bad ACL?
                        //

                        goto GetSDFromKey_BadAcl;
                    }

                }

                //
                // BUGBUG:  RPC does not understand self-relative SDs, so
                // we have to turn this into an absolute for them to turn
                // back into a self relative.
                //

                pNewSD = RtlAllocateHeap(RtlProcessHeap(), 0, cbNeeded);
                if (!pNewSD)
                {
                    goto GetSDFromKey_BadAcl;
                }

                InitializeSecurityDescriptor(   pNewSD,
                                                SECURITY_DESCRIPTOR_REVISION);

                pNewAcl = (PACL) (((PUCHAR) pNewSD) +
                                    sizeof(SECURITY_DESCRIPTOR) );

                RtlCopyMemory(pNewAcl, pAcl, AclSize.AclBytesInUse);

                SetSecurityDescriptorDacl(pNewSD, TRUE, pNewAcl, FALSE);

                Status = RtlGetOwnerSecurityDescriptor( pSD, &pSid, &Defaulted );

                if ( NT_SUCCESS( Status ) )
                {
                    pSidCopy = RtlAllocateHeap( RtlProcessHeap(),
                                                0,
                                                RtlLengthSid( pSid ) );

                    if ( pSidCopy )
                    {
                        RtlCopyMemory( pSidCopy, pSid, RtlLengthSid( pSid ) );
                    }

                    RtlSetOwnerSecurityDescriptor( pNewSD, pSidCopy, FALSE );
                }

                Status = RtlGetGroupSecurityDescriptor( pSD, &pSid, &Defaulted );

                if ( NT_SUCCESS( Status ) )
                {
                    pSidCopy = RtlAllocateHeap( RtlProcessHeap(),
                                                0,
                                                RtlLengthSid( pSid ) );

                    if ( pSidCopy )
                    {
                        RtlCopyMemory( pSidCopy, pSid, RtlLengthSid( pSid ) );
                    }

                    RtlSetGroupSecurityDescriptor( pNewSD, pSidCopy, FALSE );
                }

                RtlFreeHeap(RtlProcessHeap(), 0, pSD);

                *pSDToUse = pNewSD;
                return(Status);
            }
        }
        return(STATUS_NO_MEMORY);
    }
    else
    {


GetSDFromKey_BadAcl:

        //
        // Free the SD that we have allocated
        //

        if (pSD)
        {
            RtlFreeHeap(RtlProcessHeap(), 0, pSD);
        }

        //
        // Key exists, but there is no security descriptor, or it is unreadable
        // for whatever reason.
        //

        pSD = RtlAllocateHeap(RtlProcessHeap(), 0,
                                sizeof(SECURITY_DESCRIPTOR) );
        if (pSD)
        {
            InitializeSecurityDescriptor( pSD,
                                          SECURITY_DESCRIPTOR_REVISION );

            if (SetSecurityDescriptorDacl (
                    pSD,
                    TRUE,                           // Dacl present
                    NULL,                           // NULL Dacl
                    FALSE                           // Not defaulted
                    ) )
            {
                *pSDToUse = pSD;
                return(STATUS_SUCCESS);
            }

        }
        return(STATUS_NO_MEMORY);

    }

    return Status ;


}

NTSTATUS
RegSecCheckIfAclValid(
    VOID
    )
/*++

Routine Description:

    Checks if the local copy of the ACL from the registry is still valid (that is,
    no one has changed it.  If it is gone, the ACL is destroyed.

Arguments:

    None.

Returns:

    STATUS_SUCCESS if the state of the ACL is valid (whether it is present or not),
    other error    if an error occurred.

--*/

{
    HANDLE                  hKey;
    OBJECT_ATTRIBUTES       ObjAttr;
    UNICODE_STRING          UniString;
    PKEY_BASIC_INFORMATION  KeyInfo ;
    HANDLE                  Token ;
    HANDLE                  NullHandle ;
    UCHAR                   Buffer[ sizeof( KEY_BASIC_INFORMATION ) +
                                sizeof( RemoteRegistryKey ) + 16 ];
    NTSTATUS Status ;
    ULONG BufferSize ;


    RtlInitUnicodeString( &UniString, RemoteRegistryKey );

    InitializeObjectAttributes( &ObjAttr,
                                &UniString,
                                OBJ_CASE_INSENSITIVE,
                                NULL, NULL);

    //
    // Open the thread token.  If we're in the middle of an RPC call, we won't be
    // able to open the key (necessarily).  So, revert to local system in order to
    // open successfully.

    Status = NtOpenThreadToken( NtCurrentThread(),
                                MAXIMUM_ALLOWED,
                                TRUE,
                                &Token );

    if ( ( Status == STATUS_NO_IMPERSONATION_TOKEN ) ||
         ( Status == STATUS_NO_TOKEN ) )
    {
        Token = NULL ;
    }
    else if ( Status == STATUS_SUCCESS )
    {
        NullHandle = NULL ;

        Status = NtSetInformationThread( NtCurrentThread(),
                                ThreadImpersonationToken,
                                (PVOID) &NullHandle,
                                (ULONG) sizeof( NullHandle ) );


    }
    else
    {
        return Status ;
    }


    Status = NtOpenKey( &hKey,
                        KEY_READ,
                        &ObjAttr);

    if ( Token )
    {
        NTSTATUS RestoreStatus;

        RestoreStatus = NtSetInformationThread( NtCurrentThread(),
                                                ThreadImpersonationToken,
                                                (PVOID) &Token,
                                                sizeof( NullHandle ) );

        NtClose( Token );

        if ( !NT_SUCCESS( RestoreStatus ) )
        {
            Status = RestoreStatus ;
        }
    }

    if ( !NT_SUCCESS( Status ) )
    {
        if ( ( Status == STATUS_OBJECT_PATH_NOT_FOUND ) ||
             ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) )
        {
            //
            // The key is not present.  Either, the key has never been present,
            // in which case we're essentially done, or the key has been deleted.
            // If the key is deleted, we need to get rid of the remote acl.
            //

            if ( WinregChange.QuadPart )
            {
                //
                // Ok, the key has been deleted.  Get the exclusive lock and get to work.
                //

                RtlAcquireResourceExclusive( &RegSecReloadLock, TRUE );

                //
                // Make sure no one else got through and deleted it already:
                //

                if ( WinregChange.QuadPart )
                {

                    RtlFreeHeap( RtlProcessHeap(), 0, RemoteRegistrySD );

                    RemoteRegistrySD = NULL ;

                    WinregChange.QuadPart = 0 ;

                }

                RtlReleaseResource( &RegSecReloadLock );

            }

            Status = STATUS_SUCCESS ;
        }

        return Status ;
    }

    Status = NtQueryKey( hKey,
                         KeyBasicInformation,
                         Buffer,
                         sizeof( Buffer ),
                         & BufferSize );

    if ( !NT_SUCCESS( Status ) )
    {

        NtClose( hKey );

        return Status ;
    }

    KeyInfo = (PKEY_BASIC_INFORMATION) Buffer ;

    //
    // See if it has changed
    //

    if ( KeyInfo->LastWriteTime.QuadPart > WinregChange.QuadPart )
    {
        RtlAcquireResourceExclusive( &RegSecReloadLock, TRUE );

        //
        // Since the last check was not safe, try again.  Another thread
        // may have updated things already.
        //

        if ( KeyInfo->LastWriteTime.QuadPart > WinregChange.QuadPart )
        {

            //
            // Ok, this one is out of date.  If there is already an SD
            // allocated, free it.  We can do that, since every other thread
            // either is waiting for a shared access, or has also noticed that
            // it is out of date, and waiting for exclusive access.
            //

            if ( RemoteRegistrySD )
            {
                RtlFreeHeap( RtlProcessHeap(), 0, RemoteRegistrySD );

                RemoteRegistrySD = NULL ;
            }

            Status = RegSecReadSDFromRegistry( hKey, &RemoteRegistrySD );

            if ( NT_SUCCESS( Status ) )
            {
                WinregChange.QuadPart = KeyInfo->LastWriteTime.QuadPart ;
            }
        }

        RtlReleaseResource( &RegSecReloadLock );
    }

    NtClose( hKey );

    return Status ;
}


//+---------------------------------------------------------------------------
//
//  Function:   RegSecReadAllowedPath
//
//  Synopsis:   Pull the Allowed paths out of the registry, and set up a
//              table for searching later.  This is a flat list, since the
//              number of elements by default is 2, and shouldn't grow much
//              bigger.
//
//  Arguments:  [hKey]      --
//              [Value]     --
//              [List]      --
//              [ListBase]  --
//              [ListCount] --
//
//  History:    5-17-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
RegSecReadAllowedPath(
    HANDLE              hKey,
    PWSTR               Value,
    PUNICODE_STRING *   List,
    PUCHAR *            ListBase,
    PULONG              ListCount)
{
    NTSTATUS                Status;
    UNICODE_STRING          UniString;
    PKEY_VALUE_PARTIAL_INFORMATION  pValue;
    ULONG                   Size;
    PWSTR                   Scan;
    ULONG                   StringCount;
    PUNICODE_STRING         Paths;

    //
    // Read the value size:
    //

    RtlInitUnicodeString( &UniString, Value );

    Status = NtQueryValueKey(   hKey,
                                &UniString,
                                KeyValuePartialInformation,
                                NULL,
                                0,
                                &Size );

    if ( !NT_SUCCESS( Status ) && (Status != STATUS_BUFFER_TOO_SMALL))
    {
        if ( (Status == STATUS_OBJECT_PATH_NOT_FOUND) ||
             (Status == STATUS_OBJECT_NAME_NOT_FOUND) )
        {
            return( TRUE );
        }

        return FALSE ;
    }

    //
    // Allocate enough:
    //

    pValue = RtlAllocateHeap( RtlProcessHeap(), 0, Size );

    if ( pValue )
    {
        Status = NtQueryValueKey(   hKey,
                                    &UniString,
                                    KeyValuePartialInformation,
                                    pValue,
                                    Size,
                                    &Size );
    }


    if ( !pValue )
    {
        return( FALSE );
    }


    //
    // Okay, we should have a multi-valued set of paths that we can
    // allow access to despite the access control.
    //

    if ( pValue->Type != REG_MULTI_SZ )
    {
        RtlFreeHeap( RtlProcessHeap(), 0, pValue );
        return( FALSE );
    }

    //
    // Scan list, determine how many strings:
    //

    Scan = (PWSTR) pValue->Data;

    StringCount = 0;

    while ( *Scan )
    {
        while ( *Scan )
        {
            Scan ++;
        }

        StringCount ++;

        Scan ++;
    }

    //
    // Allocate enough UNICODE_STRING structs to point to each string
    //

    Paths = RtlAllocateHeap( RtlProcessHeap(), 0,
                                        StringCount * sizeof(UNICODE_STRING) );

    if ( !Paths )
    {
        RtlFreeHeap( RtlProcessHeap(), 0, pValue );
        return( FALSE );
    }

    Scan = ( PWSTR ) pValue->Data;

    *ListCount = StringCount;

    StringCount = 0;

    //
    // Set up one UNICODE_STRING per string in the multi_sz,
    //

    while ( *Scan )
    {
        RtlInitUnicodeString( &Paths[ StringCount ],
                              Scan );

        while ( *Scan)
        {
            Scan ++;
        }

        StringCount ++;

        Scan ++;
    }

    //
    // And pass the list back.
    //

    *ListBase = (PUCHAR) pValue;
    *List = Paths;

    return( TRUE );

}

//+---------------------------------------------------------------------------
//
//  Function:   RegSecReadAllowedPaths
//
//  Synopsis:   Reads the allowed paths out of the registry
//
//  Arguments:  (none)
//
//  History:    5-17-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
RegSecCheckAllowedPaths(
    VOID
    )
{
    HANDLE                  hKey;
    OBJECT_ATTRIBUTES       ObjAttr;
    UNICODE_STRING          UniString;
    NTSTATUS                Status;
    HANDLE                  Token ;
    HANDLE                  NullHandle ;
    PKEY_BASIC_INFORMATION  KeyInfo ;
    UCHAR                   Buffer[ sizeof( KEY_BASIC_INFORMATION ) +
                                sizeof( AllowedPathsKey ) + 16 ];
    ULONG                   BufferSize ;

    RtlInitUnicodeString(&UniString, AllowedPathsKey);

    InitializeObjectAttributes( &ObjAttr,
                                &UniString,
                                OBJ_CASE_INSENSITIVE,
                                NULL, NULL);

    //
    // Open the thread token.  If we're in the middle of an RPC call, we won't be
    // able to open the key (necessarily).  So, revert to local system in order to
    // open successfully.

    Status = NtOpenThreadToken( NtCurrentThread(),
                                MAXIMUM_ALLOWED,
                                TRUE,
                                &Token );

    if ( ( Status == STATUS_NO_IMPERSONATION_TOKEN ) ||
         ( Status == STATUS_NO_TOKEN ) )
    {
        Token = NULL ;
    }
    else if ( Status == STATUS_SUCCESS )
    {
        NullHandle = NULL ;

        Status = NtSetInformationThread( NtCurrentThread(),
                                ThreadImpersonationToken,
                                (PVOID) &NullHandle,
                                (ULONG) sizeof( NullHandle ) );


    }
    else
    {
        return Status ;
    }

    //
    // Open the key in local system context
    //

    Status = NtOpenKey( &hKey,
                        KEY_READ,
                        &ObjAttr);

    //
    // Immediately restore back to the client token.
    //

    if ( Token )
    {
        NTSTATUS RestoreStatus;

        RestoreStatus = NtSetInformationThread( NtCurrentThread(),
                                                ThreadImpersonationToken,
                                                (PVOID) &Token,
                                                sizeof( NullHandle ) );

        NtClose( Token );

        if ( !NT_SUCCESS( RestoreStatus ) )
        {
            Status = RestoreStatus ;
        }
    }

    if ( !NT_SUCCESS( Status ) )
    {
        if ( ( Status == STATUS_OBJECT_PATH_NOT_FOUND ) ||
             ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) )
        {
            //
            // The key is not present.  Either, the key has never been present,
            // in which case we're essentially done, or the key has been deleted.
            // If the key is deleted, we need to get rid of the remote acl.
            //

            if ( AllowedPathsChange.QuadPart )
            {
                //
                // Ok, the key has been deleted.  Get the exclusive lock and get to work.
                //

                RtlAcquireResourceExclusive( &RegSecReloadLock, TRUE );

                //
                // Make sure no one else has freed it already:
                //

                if ( AllowedPathsChange.QuadPart )
                {
                    if ( MachineAllowedPaths )
                    {

                        RtlFreeHeap( RtlProcessHeap(), 0, MachineAllowedPaths );

                        RtlFreeHeap( RtlProcessHeap(), 0, MachineAllowedPathsBase );

                        MachineAllowedPaths = NULL ;

                        MachineAllowedPathsBase = NULL ;

                    }

                    if ( UsersAllowedPaths )
                    {
                        RtlFreeHeap( RtlProcessHeap(), 0, UsersAllowedPaths );

                        RtlFreeHeap( RtlProcessHeap(), 0, UsersAllowedPathsBase );

                        UsersAllowedPaths = NULL ;

                        UsersAllowedPathsBase = NULL ;
                    }

                    AllowedPathsChange.QuadPart = 0;

                }

                RtlReleaseResource( &RegSecReloadLock );

            }

            Status = STATUS_SUCCESS ;
        }

        return Status ;
    }

    Status = NtQueryKey( hKey,
                         KeyBasicInformation,
                         Buffer,
                         sizeof( Buffer ),
                         & BufferSize );

    if ( !NT_SUCCESS( Status ) )
    {

        NtClose( hKey );

        return Status ;
    }

    KeyInfo = (PKEY_BASIC_INFORMATION) Buffer ;

    //
    // See if it has changed
    //

    if ( KeyInfo->LastWriteTime.QuadPart > AllowedPathsChange.QuadPart )
    {
        //
        // Well, it changed.  So, we need to flush out the old (familiar?) stuff,
        // and reload with the new stuff.  So, back to the synchronization games.
        //

        RtlAcquireResourceExclusive( &RegSecReloadLock, TRUE );

        //
        // Make sure no one else beat us to it
        //

        if ( KeyInfo->LastWriteTime.QuadPart > AllowedPathsChange.QuadPart )
        {
            if ( MachineAllowedPaths )
            {

                RtlFreeHeap( RtlProcessHeap(), 0, MachineAllowedPaths );

                RtlFreeHeap( RtlProcessHeap(), 0, MachineAllowedPathsBase );

                MachineAllowedPaths = NULL ;

                MachineAllowedPathsBase = NULL ;

            }

            if ( UsersAllowedPaths )
            {
                RtlFreeHeap( RtlProcessHeap(), 0, UsersAllowedPaths );

                RtlFreeHeap( RtlProcessHeap(), 0, UsersAllowedPathsBase );

                UsersAllowedPaths = NULL ;

                UsersAllowedPathsBase = NULL ;
            }

            //
            // Read in the paths allowed:
            //

            RegSecReadAllowedPath(  hKey,
                                    MachineValue,
                                    &MachineAllowedPaths,
                                    &MachineAllowedPathsBase,
                                    &MachineAllowedPathsCount
                                    );

            RegSecReadAllowedPath(  hKey,
                                    UsersValue,
                                    &UsersAllowedPaths,
                                    &UsersAllowedPathsBase,
                                    &UsersAllowedPathsCount
                                    );

        }

        RtlReleaseResource( &RegSecReloadLock );
    }


    NtClose( hKey );

    return STATUS_SUCCESS ;

}


//+---------------------------------------------------------------------------
//
//  Function:   InitializeRemoteSecurity
//
//  Synopsis:   Hook to initialize our look-aside stuff
//
//  Arguments:  (none)
//
//  History:    5-17-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
InitializeRemoteSecurity(
    VOID
    )
{
    RtlInitializeResource( &RegSecReloadLock );

    RemoteRegistryMappings.GenericRead = REGSEC_READ;
    RemoteRegistryMappings.GenericWrite = REGSEC_WRITE;
    RemoteRegistryMappings.GenericExecute = REGSEC_READ;
    RemoteRegistryMappings.GenericAll = REGSEC_READ | REGSEC_WRITE;

    WinregChange.QuadPart = 0 ;
    AllowedPathsChange.QuadPart = 0 ;

    return( TRUE );
}


//+---------------------------------------------------------------------------
//
//  Function:   RegSecCheckRemoteAccess
//
//  Synopsis:   Check remote access against the security descriptor we built
//              on the side.
//
//  Arguments:  [phKey] --
//
//  History:    5-17-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
RegSecCheckRemoteAccess(
    PRPC_HKEY   phKey)
{
    NTSTATUS    Status;
    ACCESS_MASK Mask;
    NTSTATUS    AccessStatus;
    HANDLE      Token;
    ULONG       Size;
    UCHAR       QuickBuffer[sizeof(PRIVILEGE_SET) + 4 * sizeof(LUID_AND_ATTRIBUTES)];
    PPRIVILEGE_SET  PrivSet;
    ULONG       PrivilegeSetLen;

    Status = RegSecCheckIfAclValid();

    if ( !NT_SUCCESS( Status ) )
    {
        return FALSE ;
    }

    RtlAcquireResourceShared( &RegSecReloadLock, TRUE );

    if ( RemoteRegistrySD )
    {

        //
        // Capture the thread's token
        //

        Status = NtOpenThreadToken(
                                NtCurrentThread(),
                                MAXIMUM_ALLOWED,
                                TRUE,
                                &Token );

        if ( !NT_SUCCESS(Status) )
        {
            RtlReleaseResource( &RegSecReloadLock );

            return( FALSE );
        }

        PrivSet = (PPRIVILEGE_SET) QuickBuffer;

        PrivilegeSetLen = sizeof( QuickBuffer );

        //
        // Do the access check.
        //

        Status = NtAccessCheck( RemoteRegistrySD,
                                Token,
                                MAXIMUM_ALLOWED,
                                &RemoteRegistryMappings,
                                PrivSet,
                                &PrivilegeSetLen,
                                &Mask,
                                &AccessStatus );

        RtlReleaseResource( &RegSecReloadLock );

        (void) NtClose( Token );

        if ( NT_SUCCESS( Status ) )
        {
            if ( NT_SUCCESS( AccessStatus ) &&
                (Mask & (REGSEC_READ | REGSEC_WRITE)) )
            {
                return( TRUE );
            }

            return( FALSE );

        }
        else 
        {
            return FALSE ;
        }

    }

    RtlReleaseResource( &RegSecReloadLock );

    return( TRUE );

}


//+---------------------------------------------------------------------------
//
//  Function:   RegSecCheckPath
//
//  Synopsis:   Check a specific key path if we should ignore the current
//              ACL.
//
//  Arguments:  [hKey]    --
//              [pSubKey] --
//
//  History:    5-17-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
RegSecCheckPath(
    HKEY                hKey,
    PUNICODE_STRING     pSubKey)

{
    UNICODE_STRING  Comparator;
    UNICODE_STRING  String;
    ULONG           i;
    ULONG           Count;
    PUNICODE_STRING List;
    BOOL            Success ;
    NTSTATUS        Status ;

    Status = RegSecCheckAllowedPaths();

    if ( !NT_SUCCESS( Status ) )
    {
        return FALSE ;
    }

    if ( (pSubKey->Buffer == NULL) ||
         (pSubKey->Length == 0 ) ||
         (pSubKey->MaximumLength == 0 ) )
    {
        return FALSE ;
    }

    RtlAcquireResourceShared( &RegSecReloadLock, TRUE );

    if ( REGSEC_TEST_HANDLE( hKey, CHECK_USER_PATHS ) )
    {
        Count = UsersAllowedPathsCount;
        List = UsersAllowedPaths;
    }
    if ( REGSEC_TEST_HANDLE( hKey, CHECK_MACHINE_PATHS ) )
    {
        Count = MachineAllowedPathsCount;
        List = MachineAllowedPaths;
    }

    Success = FALSE ;

    for ( i = 0 ; i < Count ; i++ )
    {
        String = *pSubKey;

        //
        // Ah ha, RPC strings often have the NULL included in the length.
        // touch that up.
        //

        while ( String.Buffer[ (String.Length / sizeof(WCHAR)) - 1] == L'\0' )
        {
            String.Length -= sizeof(WCHAR) ;
        }


        Comparator = List[ i ];

        //
        // If the Comparator is a prefix of the sub key, allow it (for spooler)
        //

        if ( String.Length > Comparator.Length )
        {
            if ( String.Buffer[ Comparator.Length / sizeof(WCHAR) ] == L'\\' )
            {
                //
                // Length-wise, it could be an ancestor
                //

                String.Length = Comparator.Length;

            }
        }

        //
        // If it matches, let it go...
        //

        if ( RtlCompareUnicodeString( &String, &Comparator, TRUE ) == 0 )
        {
            Success = TRUE ;

            break;
        }
    }

    RtlReleaseResource( &RegSecReloadLock ) ;

    return( Success );
}
