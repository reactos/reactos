/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    obsdata.c

Abstract:

    Object Manager Security Descriptor Caching

Author:

    Robert Reichel  (robertre)  12-Oct-1993

Revision History:

--*/

#include "obp.h"


#if DBG
#define OB_DIAGNOSTICS_ENABLED 1
#endif // DBG

//
//  These definitions are useful diagnostics aids
//

#if OB_DIAGNOSTICS_ENABLED

ULONG ObsDebugFlags = 0;

//
//  Test for enabled diagnostic
//

#define IF_OB_GLOBAL( FlagName ) if (ObsDebugFlags & (OBS_DEBUG_##FlagName))

//
//  Diagnostics print statement
//

#define ObPrint( FlagName, _Text_ ) IF_OB_GLOBAL( FlagName ) DbgPrint _Text_

#else

//
//  diagnostics not enabled - No diagnostics included in build
//

//
//  Test for diagnostics enabled
//

#define IF_OB_GLOBAL( FlagName ) if (FALSE)

//
//  Diagnostics print statement (expands to no-op)
//

#define ObPrint( FlagName, _Text_ )     ;

#endif // OB_DIAGNOSTICS_ENABLED


#if OB_DIAGNOSTICS_ENABLED

ULONG ObsTotalCacheEntries = 0;

#endif

//
//  The following flags enable or disable various diagnostic
//  capabilities within OB code.  These flags are set in
//  ObGlobalFlag (only available within a DBG system).
//
//

#define OBS_DEBUG_ALLOC_TRACKING          ((ULONG) 0x00000001L)
#define OBS_DEBUG_CACHE_FREES             ((ULONG) 0x00000002L)
#define OBS_DEBUG_BREAK_ON_INIT           ((ULONG) 0x00000004L)
#define OBS_DEBUG_SHOW_COLLISIONS         ((ULONG) 0x00000008L)
#define OBS_DEBUG_SHOW_STATISTICS         ((ULONG) 0x00000010L)
#define OBS_DEBUG_SHOW_REFERENCES         ((ULONG) 0x00000020L)
#define OBS_DEBUG_SHOW_DEASSIGN           ((ULONG) 0x00000040L)
#define OBS_DEBUG_STOP_INVALID_DESCRIPTOR ((ULONG) 0x00000080L)
#define OBS_DEBUG_SHOW_HEADER_FREE        ((ULONG) 0x00000100L)

//
//  Array of pointers to security descriptor entries
//

PLIST_ENTRY *ObsSecurityDescriptorCache = NULL;


//
//  Resource used to protect the security descriptor cache
//

ERESOURCE ObsSecurityDescriptorCacheLock;

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(PAGE,ObpDereferenceSecurityDescriptor)
#pragma alloc_text(PAGE,ObpDestroySecurityDescriptorHeader)
#pragma alloc_text(PAGE,ObpHashBuffer)
#pragma alloc_text(PAGE,ObpHashSecurityDescriptor)
#pragma alloc_text(PAGE,ObpInitSecurityDescriptorCache)
#pragma alloc_text(PAGE,ObpLogSecurityDescriptor)
#pragma alloc_text(PAGE,ObpReferenceSecurityDescriptor)
#pragma alloc_text(PAGE,ObpCreateCacheEntry)
#endif


NTSTATUS
ObpInitSecurityDescriptorCache (
    VOID
    )

/*++

Routine Description:

    Allocates and initializes the globalSecurity Descriptor Cache

Arguments:

    None

Return Value:

    STATUS_SUCCESS on success, NTSTATUS on failure.

--*/

{
    ULONG Size;
    NTSTATUS Status;

    IF_OB_GLOBAL( BREAK_ON_INIT ) {

        DbgBreakPoint();
    }

    //
    //  Allocate the cache of pointers and zero it out
    //

    Size = SECURITY_DESCRIPTOR_CACHE_ENTRIES * sizeof(PLIST_ENTRY);
    ObsSecurityDescriptorCache = ExAllocatePoolWithTag( PagedPool, Size, 'cCdS' );

    if (ObsSecurityDescriptorCache == NULL ) {

        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    RtlZeroMemory( ObsSecurityDescriptorCache, Size );

    //
    //  Initialize the resource used to protect the security cache
    //

    Status = ExInitializeResource ( &ObsSecurityDescriptorCacheLock );

    if ( !NT_SUCCESS(Status) ) {

        ExFreePool( ObsSecurityDescriptorCache );
        return( Status );
    }

    //
    //  And return to our caller
    //

    return( STATUS_SUCCESS );
}


ULONG
ObpHashSecurityDescriptor (
    PSECURITY_DESCRIPTOR SecurityDescriptor
    )

/*++

Routine Description:

    Hashes a security descriptor to a 32 bit value

Arguments:

    SecurityDescriptor - Provides the security descriptor to be hashed

Return Value:

    ULONG - a 32 bit hash value.

--*/

{
    PSID Owner = NULL;
    PSID Group = NULL;

    PACL Dacl;
    PACL Sacl;

    ULONG Hash = 0;
    BOOLEAN Junk;
    NTSTATUS Status;
    BOOLEAN DaclPresent = FALSE;
    BOOLEAN SaclPresent = FALSE;
    PISECURITY_DESCRIPTOR sd;

    //
    //  Cast the actually opaque security descriptor into something
    //  that we can decipher
    //

    sd = (PISECURITY_DESCRIPTOR)SecurityDescriptor;

    Status = RtlGetOwnerSecurityDescriptor( sd, &Owner, &Junk );
    Status = RtlGetGroupSecurityDescriptor( sd, &Group, &Junk );
    Status = RtlGetDaclSecurityDescriptor( sd, &DaclPresent, &Dacl, &Junk );
    Status = RtlGetSaclSecurityDescriptor( sd, &SaclPresent, &Sacl, &Junk );

    if ( Owner != NULL ) {

        Hash = ObpHashBuffer( Owner, RtlLengthSid( Owner ));
    }

    if ( Group != NULL ) {

        Hash += ObpHashBuffer( Group, RtlLengthSid( Group));
    }

    if ( DaclPresent && (Dacl != NULL)) {

        Hash += ObpHashBuffer( Dacl, Dacl->AclSize);
    }

    if ( SaclPresent && (Sacl != NULL)) {

        Hash += ObpHashBuffer( Sacl, Sacl->AclSize);
    }

    return( Hash );
}


ULONG
ObpHashBuffer (
    PVOID Data,
    ULONG Length
    )

/*++

Routine Description:

    Hashes a buffer into a 32 bit value

Arguments:

    Data - Buffer containing the data to be hashed.

    Length - The length in bytes of the buffer


Return Value:

    ULONG - a 32 bit hash value.

--*/

{
    PCHAR Buffer;
    ULONG Result = 0;
    LONG i;

    Buffer = (PCHAR)Data;

    for (i = 0; i <= (LONG)((Length-3)-sizeof(ULONG)); i++) {

        ULONG Tmp;

        Tmp = *((ULONG UNALIGNED *)(Buffer + i));
        Result += Tmp;
    }

    return( Result );
}


NTSTATUS
ObpLogSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR InputSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR *OutputSecurityDescriptor
    )

/*++

Routine Description:

    Takes a passed security descriptor and registers it into the
    security descriptor database.

Arguments:

    InputSecurityDescriptor - The new security descriptor to be logged into
        the database. On a successful return this memory will have been
        freed back to pool.

    OutputSecurityDescriptor - Output security descriptor to be used by the
        caller.

Return Value:

    An appropriate status value

--*/

{
    ULONG FullHash;
    UCHAR  SmallHash;
    PSECURITY_DESCRIPTOR_HEADER NewDescriptor;
    PLIST_ENTRY Front;
    PLIST_ENTRY Back;
    PSECURITY_DESCRIPTOR_HEADER Header;
    BOOLEAN Match;

    FullHash = ObpHashSecurityDescriptor( InputSecurityDescriptor );
    SmallHash = (UCHAR)FullHash;

    //
    //  See if the entry matching SmallHash is in use.
    //  Lock the table first, unlock if if we don't need it.
    //

    ObpAcquireDescriptorCacheWriteLock();

    Front = ObsSecurityDescriptorCache[SmallHash];
    Back  = NULL;
    Match = FALSE;

    //
    //  Zoom down the hash bucket looking for a full hash match
    //

    while ( Front != NULL ) {

        Header = LINK_TO_SD_HEADER( Front );

        //
        //  **** is this test really right?  Is the full hash value really
        //  ordered like this?
        //

        if ( Header->FullHash > FullHash ) {

            break;
        }

        if ( Header->FullHash == FullHash ) {

            Match = ObpCompareSecurityDescriptors( InputSecurityDescriptor,
                                                   &Header->SecurityDescriptor );

            if ( Match ) {

                break;
            }

            ObPrint( SHOW_COLLISIONS,("Got a collision on %d, no match\n",SmallHash));
        }

        Back = Front;
        Front = Front->Flink;
    }

    //
    //  If we have a match then we'll get the caller to use the old
    //  cached descriptor, but bumping its ref count, freeing what
    //  the caller supplied and returning the old one to our caller
    //

    if ( Match ) {

        Header->RefCount++;

        ObPrint( SHOW_REFERENCES, ("Reference Hash = 0x%lX, New RefCount = %d\n",Header->FullHash,Header->RefCount));

        *OutputSecurityDescriptor = &Header->SecurityDescriptor;

        ExFreePool( InputSecurityDescriptor );

        ObpReleaseDescriptorCacheLock();

        return( STATUS_SUCCESS );
    }

    //
    //  Can't use an existing one, create a new entry
    //  and insert it into the list.
    //

    NewDescriptor = ObpCreateCacheEntry( InputSecurityDescriptor,
                                         FullHash );

    if ( NewDescriptor == NULL ) {

        ObpReleaseDescriptorCacheLock();

        return( STATUS_INSUFFICIENT_RESOURCES );
    }

#if OB_DIAGNOSTICS_ENABLED

    ObsTotalCacheEntries++;

#endif

    ObPrint( SHOW_STATISTICS, ("ObsTotalCacheEntries = %d \n",ObsTotalCacheEntries));
    ObPrint( SHOW_COLLISIONS, ("Adding new entry for index #%d \n",SmallHash));

    //
    //  We don't need the old security descriptor any more.
    //

    ExFreePool( InputSecurityDescriptor );

    //
    //  The following logic inserts the new security descriptor into the
    //  small hash list.  We need to first decide if the we're adding
    //  the entry to the beginning of the list (back is null) or if we're
    //  further in the list.
    //
    //  **** This logic is plain bizzare because (1) the small hash
    //  should probably just be an array of list heads and not single
    //  pointer value and (2) the doubly linked list of security descriptors
    //  is not circular!  This should be fixed to use the regular set
    //  of link list macros
    //

    if ( Back == NULL ) {

        //
        //  We're inserting at the beginning of the list for this
        //  minor index
        //

        NewDescriptor->Link.Flink = ObsSecurityDescriptorCache[SmallHash];
        ObsSecurityDescriptorCache[SmallHash] = &NewDescriptor->Link;

        if ( NewDescriptor->Link.Flink != NULL ) {

            NewDescriptor->Link.Flink->Blink = &NewDescriptor->Link;
        }

    } else {

        //
        //  Hook new descriptor entry into list.
        //

        NewDescriptor->Link.Flink = Front;

        NewDescriptor->Link.Blink = Back;

        Back->Flink = &NewDescriptor->Link;

        if (Front != NULL) {

            Front->Blink = &NewDescriptor->Link;
        }
    }

    //
    //  Set the output security descriptor and return to our caller
    //

    *OutputSecurityDescriptor = &NewDescriptor->SecurityDescriptor;
    ObpReleaseDescriptorCacheLock();

    return( STATUS_SUCCESS );
}


PSECURITY_DESCRIPTOR_HEADER
ObpCreateCacheEntry (
    PSECURITY_DESCRIPTOR InputSecurityDescriptor,
    ULONG FullHash
    )

/*++

Routine Description:

    Allocates and initializes a new cache entry.

Arguments:

    InputSecurityDescriptor - The security descriptor to be cached.

    FullHash - Full 32 bit hash of the security descriptor.

Return Value:

    A pointer to the newly allocated cache entry, or NULL

--*/

{

    ULONG SecurityDescriptorLength;
    ULONG CacheEntrySize;
    PSECURITY_DESCRIPTOR_HEADER NewDescriptor;

    //
    //  Compute the size that we'll need to allocate.  We need space for
    //  the security descriptor cache minus the funny quad at the end and the
    //  security descriptor itself.
    //

    SecurityDescriptorLength = RtlLengthSecurityDescriptor ( InputSecurityDescriptor );
    CacheEntrySize = SecurityDescriptorLength + (sizeof(SECURITY_DESCRIPTOR_HEADER) - sizeof( QUAD ));

    //
    //  Now allocate space for the cached entry
    //

    NewDescriptor = ExAllocatePoolWithTag( PagedPool, CacheEntrySize, 'dSeS');

    if ( NewDescriptor == NULL ) {

        return( NULL );
    }

    //
    //  Fill the header, copy over the descriptor data, and return to our
    //  caller
    //

    NewDescriptor->RefCount   = 1;
    NewDescriptor->FullHash   = FullHash;
    NewDescriptor->Link.Flink = NULL;
    NewDescriptor->Link.Blink = NULL;

    RtlCopyMemory( &NewDescriptor->SecurityDescriptor,
                   InputSecurityDescriptor,
                   SecurityDescriptorLength );

    return( NewDescriptor );
}


PSECURITY_DESCRIPTOR
ObpReferenceSecurityDescriptor (
    PVOID  Object
    )

/*++

Routine Description:

    References the security descriptor of the passed object.

Arguments:

    Object - Object being access validated.

Return Value:

    The security descriptor of the object.

--*/

{
    PSECURITY_DESCRIPTOR_HEADER SecurityDescriptorHeader;
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    PSECURITY_DESCRIPTOR SecurityDescriptor;

    //
    //  Make sure the sure that the object in question is being
    //  maintained by the system and doesn't have it own security
    //  management routines.
    //
    //  **** the first two lines should probably be only done on
    //  a checked build
    //

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
    ObjectType = ObjectHeader->Type;
    ASSERT( ObpCentralizedSecurity(ObjectType) );

    //
    //  Lock the security descriptor cache and get the objects
    //  security descriptor
    //

    ObpAcquireDescriptorCacheWriteLock();

    SecurityDescriptor = OBJECT_TO_OBJECT_HEADER( Object )->SecurityDescriptor;

    IF_OB_GLOBAL( STOP_INVALID_DESCRIPTOR ) {

        if((SecurityDescriptor != NULL) &&
           (!RtlValidSecurityDescriptor ( SecurityDescriptor ))) {

            DbgBreakPoint();
        }
    }

    //
    //  If the object has a security descriptor then we need to
    //  get the security descriptor header and increment its
    //  ref count before releasing the lock and returning to
    //  our caller
    //

    if ( SecurityDescriptor != NULL ) {

        SecurityDescriptorHeader = SD_TO_SD_HEADER( SecurityDescriptor );
        ObPrint( SHOW_REFERENCES, ("Referencing Hash %lX, Refcount = %d \n",SecurityDescriptorHeader->FullHash,SecurityDescriptorHeader->RefCount));
        SecurityDescriptorHeader->RefCount++;
    }

    ObpReleaseDescriptorCacheLock();

    return( SecurityDescriptor );
}


NTSTATUS
ObDeassignSecurity (
    IN OUT PSECURITY_DESCRIPTOR *SecurityDescriptor
    )

/*++

Routine Description:

    This routine dereferences the input security descriptor

Arguments:

    SecurityDescriptor - Supplies the security descriptor
        being modified

Return Value:

    Only returns STATUS_SUCCESS

--*/

{
    PSECURITY_DESCRIPTOR_HEADER Header;

    //
    //  Lock the security descriptor cache and get a pointer
    //  to the security descriptor header
    //

    ObpAcquireDescriptorCacheWriteLock();

    //
    //  **** the following diagnostic code should really be done only on
    //  a checked build
    //

    Header = SD_TO_SD_HEADER( *SecurityDescriptor );
    ObPrint( SHOW_DEASSIGN,("Deassigning security descriptor %x, hash = %lX\n",*SecurityDescriptor, Header->FullHash));

    //
    //  Call the actual routine to dereference the security descriptor
    //

    ObpDereferenceSecurityDescriptor( *SecurityDescriptor );

    //
    //  NULL out the SecurityDescriptor in the object's
    //  header so we don't try to free it again.
    //

    *SecurityDescriptor = NULL;

    //
    //  Unlock the security descriptor cache and return to our caller
    //

    ObpReleaseDescriptorCacheLock();
    
    return( STATUS_SUCCESS );
}


VOID
ObpDereferenceSecurityDescriptor (
    PSECURITY_DESCRIPTOR SecurityDescriptor
    )

/*++

Routine Description:

    Decrements the refcount of a cached security descriptor

Arguments:

    SecurityDescriptor - Points to a cached security descriptor

Return Value:

    None.

--*/

{
    PSECURITY_DESCRIPTOR_HEADER  SecurityDescriptorHeader;

    //
    //  Lock the security descriptor cache and get a pointer
    //  to the security descriptor header
    //

    ObpAcquireDescriptorCacheWriteLock();

    SecurityDescriptorHeader = SD_TO_SD_HEADER( SecurityDescriptor );

    //
    //  Do some debug work
    //

    ObPrint( SHOW_REFERENCES, ("Dereferencing SecurityDescriptor %x, hash %lx, refcount = %d \n", SecurityDescriptor, SecurityDescriptorHeader->FullHash,SecurityDescriptorHeader->RefCount));

    ASSERT(SecurityDescriptorHeader->RefCount != 0);

    //
    //  Decrement the ref count and if it is now zero then
    //  we can completely remove this entry from the cache
    //

    if (--SecurityDescriptorHeader->RefCount == 0) {

        ObpDestroySecurityDescriptorHeader( SecurityDescriptorHeader );
    }

    //
    //  Unlock the security descriptor cache and return to our caller
    //

    ObpReleaseDescriptorCacheLock();
}


VOID
ObpDestroySecurityDescriptorHeader (
    IN PSECURITY_DESCRIPTOR_HEADER Header
    )

/*++

Routine Description:

    Frees a cached security descriptor and unlinks it from the chain.

Arguments:

    Header - Pointer to a security descriptor header (cached security
        descriptor)

Return Value:

    None.

--*/

{
    PLIST_ENTRY Forward;
    PLIST_ENTRY Rear;
    UCHAR SmallHash;

    ASSERT ( Header->RefCount == 0 );

#if OB_DIAGNOSTICS_ENABLED

    ObsTotalCacheEntries--;

#endif

    ObPrint( SHOW_STATISTICS, ("ObsTotalCacheEntries = %d \n",ObsTotalCacheEntries));

    //
    //  Unlink the cached security descriptor from its linked list and
    //  from the small hash table if it was at the head of the list
    //
    //  **** this should all be rewritten to use the regular set of
    //  link list macros
    //

    SmallHash = (UCHAR)Header->FullHash;

    Forward = Header->Link.Flink;
    Rear = Header->Link.Blink;

    if ( Forward != NULL ) {

        Forward->Blink = Rear;
    }

    if ( Rear != NULL ) {

        Rear->Flink = Forward;

    } else {

        //
        //  if Rear is NULL, we're deleting the head of the list
        //

        ObsSecurityDescriptorCache[SmallHash] = Forward;
    }

    ObPrint( SHOW_HEADER_FREE, ("Freeing memory at %x \n",Header));

    //
    //  Now return the cached descriptor to pool and return to our caller
    //

    ExFreePool( Header );

    return;
}


BOOLEAN
ObpCompareSecurityDescriptors (
    IN PSECURITY_DESCRIPTOR SD1,
    IN PSECURITY_DESCRIPTOR SD2
    )

/*++

Routine Description:

    Performs a byte by byte comparison of two self relative security
    descriptors to determine if they are identical.

Arguments:

    SD1, SD2 - Security descriptors to be compared.

Return Value:

    TRUE - They are the same.

    FALSE - They are different.

--*/

{
    ULONG Length1;
    ULONG Length2;
    ULONG Compare;

    //
    //  Calculating the length is pretty fast, see if we
    //  can get away with doing only that.
    //

    Length1 =  RtlLengthSecurityDescriptor ( SD1 );
    Length2 =  RtlLengthSecurityDescriptor ( SD2 );

    if (Length1 != Length2) {

        return( FALSE );
    }

    return (BOOLEAN)RtlEqualMemory ( SD1, SD2, Length1 );
}


VOID
ObpAcquireDescriptorCacheWriteLock (
    VOID
    )

/*++

Routine Description:

    Takes a write lock on the security descriptor cache.

Arguments:

    none

Return Value:

    None.

--*/

{
    KeEnterCriticalRegion();
    (VOID)ExAcquireResourceExclusive( &ObsSecurityDescriptorCacheLock, TRUE );

    return;
}

VOID
ObpAcquireDescriptorCacheReadLock (
    VOID
    )

/*++

Routine Description:

    Takes a read lock on the security descriptor cache.

Arguments:

    none

Return Value:

    None.

--*/

{
    KeEnterCriticalRegion();
    (VOID)ExAcquireResourceShared( &ObsSecurityDescriptorCacheLock,TRUE );

    return;
}

VOID
ObpReleaseDescriptorCacheLock (
    VOID
    )

/*++

Routine Description:

    Releases a lock on the security descriptor cache.

Arguments:

    none

Return Value:

    None.

--*/

{
    (VOID)ExReleaseResource( &ObsSecurityDescriptorCacheLock );
    KeLeaveCriticalRegion ();

    return;
}

