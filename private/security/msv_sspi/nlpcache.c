/*++

Copyright (c) 1991 - 1999  Microsoft Corporation

Module Name:

    nlpcache.c

Abstract:

    This module contains routines which implement user account caching:

        NlpCacheInitialize
        NlpCacheTerminate
        NlpAddCacheEntry
        NlpGetCacheEntry
        NlpDeleteCacheEntry
        NlpChangeCachePassword


    The cache contains the most recent validated logon information. There is
    only 1 (that's right - one) cache slot. This will probably change though

Author:

    Richard L Firth (rfirth) 17-Dec-1991

    BUGBUG - To Be Done:

             1. call Lsar routines instead of going via Rpc. Use:
                    LsaIOpenPolicyTruseted()
                    Lsar routines to do work
                    LsaIFree() to free returned buffers

Revision History:

   Scott Field (sfield)   04-Jun-99
        Add supplemental cache data.
        Store all cache related data in single location.
        Encrypt interesting elements of cache entry using per-entry key mixed with per-machine key.
        MAC interesting cache elements for integrity check.
        Drastically reduce lock contention.
        Avoid NtFlushKey() for single location cache elements.
        Avoid persisting a new cache entry that matches an existing one.
        Attempt reg query with stack based buffer first.

   Chandana Surlu         21-Jul-96      Stolen from \\kernel\razzle3\src\security\msv1_0\nlpcache.c

--*/

#include <global.h>
#undef EXTERN

#include "msp.h"
#include "nlp.h"
#include "nlpcache.h"


#include <rc4.h>


//
// manifests
//

#if DBG
#include <stdio.h>
#endif

//
// Revision numbers
//
//      NT 3.0 didn't explicitly store a revision number.
//          However, we are designating that release to be revision 0x00010000 (1.0).
//      NT 3.5 prior to build 622 is revision 0x00010001 (1.1).
//      NT 3.5 is revision 0x00010002 (1.2).
//      NT 4.0 SP 4 is revision 0x00010003 (1.3)
//      NT 5.0 build 2054+ is revision 0x00010004 (1.4)
//

#define NLP_CACHE_REVISION_NT_1_0         (0x00010000)  // NT 3.0
#define NLP_CACHE_REVISION_NT_1_0B        (0x00010002)  // NT 3.5
#define NLP_CACHE_REVISION_NT_4_SP4       (0x00010003)  // NT 4.0 SP 4 to save passwords as salted.
#define NLP_CACHE_REVISION_NT_5_0         (0x00010004)  // NT 5.0 to support opaque cache data and single location data storage.
#define NLP_CACHE_REVISION                (NLP_CACHE_REVISION_NT_5_0)





//
// The logon cache may be controlled via a value in the registry.
// If the registry key does not exist, then this default constant defines
// how many logon cache entries will be active.  The max constant
// places an upper limit on how many cache entries we will support.
// If the user specifies more than the max value, we will use the
// max value instead.
//

#define NLP_DEFAULT_LOGON_CACHE_COUNT           (10)
#define NLP_MAX_LOGON_CACHE_COUNT               (50)


//
// length of per-machine cache encryption key.
//

#define NLP_CACHE_ENCRYPTION_KEY_LEN            (64)

//
// name of LSA secret containing cache encryption key.
//

#define NLP_CACHE_ENCRYPTION_KEY_NAME           L"NL$KM"


//
// macros
//

#define AllocateCacheEntry(n)   (PLOGON_CACHE_ENTRY)RtlAllocateHeap(MspHeap, 0, n)
#define FreeCacheEntry(p)       RtlFreeHeap(MspHeap, 0, (PVOID)p)
#define AllocateFromHeap(n)     RtlAllocateHeap(MspHeap, 0, n)
#define FreeToHeap(p)           RtlFreeHeap(MspHeap, 0, (PVOID)p)

//
// guard against simultaneous access
//

#define ENTER_CACHE()   RtlEnterCriticalSection(&NlpLogonCacheCritSec)
#define LEAVE_CACHE()   RtlLeaveCriticalSection(&NlpLogonCacheCritSec)

#define INVALIDATE_HANDLE(handle) (*((PHANDLE)(&handle)) = INVALID_HANDLE_VALUE)
#define IS_VALID_HANDLE(handle)   (handle != INVALID_HANDLE_VALUE)


////////////////////////////////////////////////////////////////////////
//                                                                    //
// datatypes                                                          //
//                                                                    //
////////////////////////////////////////////////////////////////////////

typedef enum _NLP_SET_TIME_HINT {
    NLP_SMALL_TIME,
    NLP_BIG_TIME,
    NLP_NOW_TIME
} NLP_SET_TIME_HINT, *PNLP_SET_TIME_HINT;

#define BIG_PART_1      0x7fffffff  // largest positive large int is 63 bits on
#define BIG_PART_2      0xffffffff
#define SMALL_PART_1    0x0         // smallest positive large int is 64 bits off
#define SMALL_PART_2    0x0


//
// This structure is saved on disk and provides information
// about the rest of the cache.  This structure is in a value
// named "NL$Control" under the cache registry key.
//

typedef struct _NLP_CACHE_CONTROL {

    //
    // Revision of the cache on-disk structure
    //

    ULONG       Revision;

    //
    // The current on-disk size of the cache (number of entries)
    //

    ULONG       Entries;

} NLP_CACHE_CONTROL, *PNLP_CACHE_CONTROL;


//
// This data structure is a single cache table entry (CTE)
// Each entry in the cache has a corresponding CTE.
//

typedef struct _NLP_CTE {

        //
        // CTEs are linked on either an invalid list (in any order)
        // or on a valid list (in ascending order of time).
        // This makes it easy to figure out which entry is to be
        // flushed when adding to the cache.
        //

        LIST_ENTRY Link;


        //
        // Time the cache entry was established.
        // This is used to determine which cache
        // entry is the oldest, and therefore will
        // be flushed from the cache first to make
        // room for new entries.
        //

        LARGE_INTEGER       Time;


        //
        // This field contains the index of the CTE within the
        // CTE table.  This index is used to generate the names
        // of the entrie's secret key and cache key in the registry.
        // This field is valid even if the entry is marked Inactive.
        //

        ULONG               Index;

        //
        // Normally, we walk the active and inactive lists
        // to find entries.  When growing or shrinking the
        // cache, however, it is nice to be able to walk the
        // table using indexes.  In this case, it is nice to
        // have a local way of determining whether an entry
        // is on the active or inactive list.  This field
        // provides that capability.
        //
        //      TRUE  ==> on active list
        //      FALSE ==> not on active list
        //

        BOOLEAN             Active;


} NLP_CTE, *PNLP_CTE;

//
// This structure is used for keeping track of all information that
// is stored on backing store.
//

typedef struct _NLP_CACHE_AND_SECRETS {
    PLOGON_CACHE_ENTRY          CacheEntry;
    ULONG                       EntrySize;
    PLSAPR_CR_CIPHER_VALUE      NewSecret;
    PLSAPR_CR_CIPHER_VALUE      OldSecret;
    BOOLEAN                     Active;
} NLP_CACHE_AND_SECRETS,  *PNLP_CACHE_AND_SECRETS;


////////////////////////////////////////////////////////////////////////
//                                                                    //
// Local Prototypes                                                   //
//                                                                    //
////////////////////////////////////////////////////////////////////////

NTSTATUS
NlpInternalCacheInitialize(
    VOID
    );

NTSTATUS
NlpOpenCache( VOID );

VOID
NlpCloseCache( VOID );


NTSTATUS
NlpGetCacheControlInfo( VOID );

NTSTATUS
NlpCacheKeyInitialize(
    VOID
    );

NTSTATUS
NlpBuildCteTable( VOID );

NTSTATUS
NlpChangeCacheSizeIfNecessary( VOID );

NTSTATUS
NlpWriteCacheControl( VOID );

VOID
NlpMakeCacheEntryName(
    IN  ULONG               EntryIndex,
    OUT PUNICODE_STRING     Name
    );

NTSTATUS
NlpMakeNewCacheEntry(
    ULONG           Index
    );

NTSTATUS
NlpEliminateCacheEntry(
    IN  ULONG               Index
    );

NTSTATUS
NlpConvert1_0To1_0B( VOID );

NTSTATUS
NlpOpen_Nt1_0_Secret( VOID );

NTSTATUS
NlpReadCacheEntryByIndex(
    IN  ULONG               Index,
    OUT PLOGON_CACHE_ENTRY* CacheEntry,
    OUT PULONG EntrySize
    );

VOID
NlpAddEntryToActiveList(
    IN  ULONG   Index
    );

VOID
NlpAddEntryToInactiveList(
    IN  ULONG   Index
    );

VOID
NlpGetFreeEntryIndex(
    OUT PULONG  Index
    );


NTSTATUS
NlpBuildCacheEntry(
    IN  PNETLOGON_INTERACTIVE_INFO LogonInfo,
    IN  PNETLOGON_VALIDATION_SAM_INFO2 AccountInfo,
    OUT PLOGON_CACHE_ENTRY* ppCacheEntry,
    OUT PULONG pEntryLength
    );


NTSTATUS
NlpEncryptCacheEntry(
    IN  PLOGON_CACHE_ENTRY CacheEntry,
    IN  ULONG EntrySize
    );

NTSTATUS
NlpDecryptCacheEntry(
    IN  PLOGON_CACHE_ENTRY CacheEntry,
    IN  ULONG EntrySize
    );

NTSTATUS
NlpAddSupplementalCacheData(
    IN      PVOID SupplementalCacheData,
    IN      ULONG SupplementalCacheDataLength,
    IN OUT  PLOGON_CACHE_ENTRY *ppCacheEntry,
    IN OUT  PULONG pEntryLength
    );

NTSTATUS
NlpOpenCache( VOID );

VOID
NlpCloseCache( VOID );

NTSTATUS
NlpOpenSecret(
    IN  ULONG   Index
    );

VOID
NlpCloseSecret( VOID );

NTSTATUS
NlpWriteSecret(
    IN  PLSAPR_CR_CIPHER_VALUE NewSecret,
    IN  PLSAPR_CR_CIPHER_VALUE OldSecret
    );

NTSTATUS
NlpReadSecret(
    OUT PLSAPR_CR_CIPHER_VALUE * NewSecret,
    OUT PLSAPR_CR_CIPHER_VALUE * OldSecret
    );

NTSTATUS
NlpMakeSecretPassword(
    OUT PLSAPR_CR_CIPHER_VALUE Passwords,
    IN  PUNICODE_STRING UserName,
    IN  PNT_OWF_PASSWORD NtOwfPassword OPTIONAL,
    IN  PLM_OWF_PASSWORD LmOwfPassword OPTIONAL
    );

NTSTATUS
NlpMakeSecretPasswordNT5(
    IN OUT PCACHE_PASSWORDS Passwords,
    IN  PUNICODE_STRING UserName,
    IN  PNT_OWF_PASSWORD NtOwfPassword OPTIONAL,
    IN  PLM_OWF_PASSWORD LmOwfPassword OPTIONAL
    );

NTSTATUS
NlpReadCacheEntry(
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING UserName,
    OUT PULONG              Index,
    OUT PLOGON_CACHE_ENTRY* CacheEntry,
    OUT PULONG              EntrySize
    );

NTSTATUS
NlpWriteCacheEntry(
    IN  ULONG              Index,
    IN  PLOGON_CACHE_ENTRY Entry,
    IN  ULONG              EntrySize
    );

VOID
NlpCopyAndUpdateAccountInfo(
    IN  USHORT Length,
    IN  PUNICODE_STRING pUnicodeString,
    IN OUT PUCHAR* pSource,
    IN OUT PUCHAR* pDest
    );

VOID
NlpSetTimeField(
    OUT POLD_LARGE_INTEGER pTimeField,
    IN  NLP_SET_TIME_HINT Hint
    );

NTSTATUS
NlpBuildAccountInfo(
    IN  PLOGON_CACHE_ENTRY pCacheEntry,
    IN  ULONG EntryLength,
    OUT PNETLOGON_VALIDATION_SAM_INFO2* AccountInfo
    );



/////////////////////////////////////////////////////////////////////////
//                                                                     //
//          Diagnostic support services prototypes                     //
//                                                                     //
/////////////////////////////////////////////////////////////////////////


#if DBG
PCHAR
DumpOwfPasswordToString(
    OUT PCHAR Buffer,
    IN  PLM_OWF_PASSWORD Password
    );

VOID
DumpLogonInfo(
    IN  PNETLOGON_LOGON_IDENTITY_INFO LogonInfo
    );

char*
MapWeekday(
    IN  CSHORT  Weekday
    );

VOID
DumpTime(
    IN  LPSTR   String,
    IN  POLD_LARGE_INTEGER OldTime
    );

VOID
DumpGroupIds(
    IN  LPSTR   String,
    IN  ULONG   Count,
    IN  PGROUP_MEMBERSHIP GroupIds
    );

VOID
DumpSessKey(
    IN  LPSTR   String,
    IN  PUSER_SESSION_KEY Key
    );

VOID
DumpSid(
    LPSTR   String,
    PISID   Sid
    );

VOID
DumpAccountInfo(
    IN  PNETLOGON_VALIDATION_SAM_INFO2 AccountInfo
    );

VOID
DumpCacheEntry(
    IN  ULONG              Index,
    IN  PLOGON_CACHE_ENTRY pEntry
    );

#endif //DBG




////////////////////////////////////////////////////////////////////////
//                                                                    //
// global data                                                        //
//                                                                    //
////////////////////////////////////////////////////////////////////////

//
// This boolean indicates whether or not we have been able to
// initialize caching yet.  It turn out that during authentication
// package load time, we can't do everything we would like to (like
// call LSA RPC routines).  So, we delay initializing until we can
// call LSA.  All publicly exposed interfaces must check this value
// before assuming work can be done.
//

BOOLEAN     NlpInitializationNotYetPerformed = TRUE;


RTL_CRITICAL_SECTION NlpLogonCacheCritSec;



HANDLE        NlpCacheHandle  = (HANDLE)     INVALID_HANDLE_VALUE;
LSAPR_HANDLE  NlpLsaHandle    = (LSAPR_HANDLE) INVALID_HANDLE_VALUE;
LSAPR_HANDLE  NlpSecretHandle = (LSAPR_HANDLE) INVALID_HANDLE_VALUE;

//
// control information about the cache (number of entries, etc).
//

NLP_CACHE_CONTROL   NlpCacheControl;

//
// This structure is generated and maintained only in memory.
// It indicates which cache entries are valid and which aren't.
// It also indicates what time each entry was established so we
// know which order to discard them in.
//
//  This field is a pointer to an array of CTEs.  The number of CTEs
//  in the array is in NlpCacheControl.Entries.  This structure is
//  allocated at initialization time.
//

PNLP_CTE            NlpCteTable;


//
// The Cache Table Entries in NlpCteTable are linked on either an
// active or inactive list.  The entries on the active list are in
// ascending time order - so the last one on the list is the first
// one to be discarded when a flush is needed to add a new entry.
//

LIST_ENTRY          NlpActiveCtes;
LIST_ENTRY          NlpInactiveCtes;


//
// global, per-machine key used for encrypting NT_5_0 version cache
// entries.
//

CHAR                NlpCacheEncryptionKey[ NLP_CACHE_ENCRYPTION_KEY_LEN ];



#if DBG
#ifdef DUMP_CACHE_INFO
ULONG   DumpCacheInfo = 1;
#else
ULONG   DumpCacheInfo = 0;
#endif
#endif



////////////////////////////////////////////////////////////////////////
//                                                                    //
// Services Exported by this module                                   //
//                                                                    //
////////////////////////////////////////////////////////////////////////






NTSTATUS
NlpCacheInitialize(
    VOID
    )

/*++

Routine Description:

    This routine is called to initialize cached logon processing.

    Unfortunately, there isn't much we can do when we are called.
    (we can't open LSA, for example).  So, defer initialization
    until later.


Arguments:

    None.

Return Value:

    NTSTATUS

--*/

{
    return RtlInitializeCriticalSection(&NlpLogonCacheCritSec);
}


NTSTATUS
NlpCacheTerminate(
    VOID
    )

/*++

Routine Description:

    Called when process detaches

Arguments:

    None.

Return Value:

    NTSTATUS

--*/

{
#if DBG
    if (DumpCacheInfo) {
        DbgPrint("NlpCacheTerminate\n");
    }
#endif

    if (!NlpInitializationNotYetPerformed) {
        NlpCloseCache();
        NlpCloseSecret();

        if (IS_VALID_HANDLE(NlpLsaHandle)) {
            I_LsarClose( &NlpLsaHandle );
        }
        if (IS_VALID_HANDLE(NlpCacheHandle)) {
            NtClose( NlpCacheHandle );
        }

        FreeToHeap( NlpCteTable );
    }

    return RtlDeleteCriticalSection(&NlpLogonCacheCritSec);

}


NTSTATUS
NlpGetCacheEntry(
    IN  PNETLOGON_LOGON_IDENTITY_INFO LogonInfo,
    OUT PNETLOGON_VALIDATION_SAM_INFO2* AccountInfo,
    OUT PCACHE_PASSWORDS Passwords,
    OUT PVOID *ppSupplementalCacheData OPTIONAL ,
    OUT PULONG SupplementalCacheDataLength OPTIONAL
    )

/*++

Routine Description:

    If the user logging on has information stored in the cache,
    then it is retrieved. Also returns the cached password from
    'secret' storage

Arguments:

    LogonInfo   - pointer to NETLOGON_IDENTITY_INFO structure which contains
                  the domain name, user name for this user

    AccountInfo - pointer to NETLOGON_VALIDATION_SAM_INFO2 structure to
                  receive this user's specific interactive logon information

    Passwords   - pointer to CACHE_PASSWORDS structure to receive passwords
                  returned from secret storage

Return Value:

    NTSTATUS
        Success = STATUS_SUCCESS
                    *AccountInfo points to a NETLOGON_VALIDATION_SAM_INFO2
                    structure. This must be freed by caller

                    *Passwords contain USER_INTERNAL1_INFORMATION structure
                    which contains NT OWF password and LM OWF password. These
                    must be used to validate the logon

        Failure = STATUS_LOGON_FAILURE
                    The user logging on isn't in the cache.

--*/

{
    NTSTATUS
        NtStatus;

    PNETLOGON_VALIDATION_SAM_INFO2
        SamInfo = NULL;

    PLOGON_CACHE_ENTRY
        CacheEntry = NULL;

    ULONG
        EntrySize,
        Index;

    PLSAPR_CR_CIPHER_VALUE
        CurrentSecret = NULL,
        OldSecret = NULL;


    BOOLEAN fCacheLocked = FALSE;


    *AccountInfo = NULL;

    if( ppSupplementalCacheData )
        *ppSupplementalCacheData = NULL;


#if DBG
    if (DumpCacheInfo) {
        DbgPrint("NlpGetCacheEntry\n");
        DumpLogonInfo(LogonInfo);
    }
#endif

    if (NlpInitializationNotYetPerformed) {
        NtStatus = NlpInternalCacheInitialize();
        if (!NT_SUCCESS(NtStatus)) {
            return(NtStatus);
        }
    }

    if (NlpCacheControl.Entries == 0) {
        return(STATUS_LOGON_FAILURE);
    }

    ENTER_CACHE();
    fCacheLocked = TRUE;

    //
    // Find the cache entry and open its secret (if found)
    //

    NtStatus = NlpReadCacheEntry(&LogonInfo->LogonDomainName,
                                 &LogonInfo->UserName,
                                 &Index,
                                 &CacheEntry,
                                 &EntrySize);

    if(!NT_SUCCESS(NtStatus)) {
        LEAVE_CACHE();
        return (NtStatus);
    }

    if( CacheEntry->Revision >= NLP_CACHE_REVISION_NT_5_0 ) {

        //
        // for NT5, we can release the cache lock now, since all data
        // stored in one place.
        //

        LEAVE_CACHE();
        fCacheLocked = FALSE;

        //
        // if caller wanted supplemental data, give it to them.
        //

        if( ppSupplementalCacheData  && SupplementalCacheDataLength ) {

            *SupplementalCacheDataLength = CacheEntry->SupplementalCacheDataLength;

            *ppSupplementalCacheData = MIDL_user_allocate( *SupplementalCacheDataLength );

            if( *ppSupplementalCacheData == NULL ) {
                NtStatus = STATUS_NO_MEMORY;
            } else {

                LPBYTE Source;

                //
                // note: the decrypt operation that occurred during the
                // ReadCacheEntry validates any data and pointers through
                // integrity checking via HMAC.  Having said that, we can be
                // lazy and not do boundry checking.
                //

                Source = ((LPBYTE)CacheEntry + CacheEntry->SupplementalCacheDataOffset);

                CopyMemory( *ppSupplementalCacheData,
                            Source,
                            *SupplementalCacheDataLength
                            );
            }
        }

    }


    NtStatus = NlpBuildAccountInfo(CacheEntry, EntrySize, &SamInfo);
    if (NT_SUCCESS(NtStatus)) {


        if( CacheEntry->Revision >= NLP_CACHE_REVISION_NT_5_0 ) {

            //
            // for NT5, the Passwords are stored in the CacheEntry.
            // note: passwords are assumed to be salted.
            //

            RtlCopyMemory( Passwords, &(CacheEntry->CachePasswords), sizeof(*Passwords) );


        } else {

            //
            // prior to NT5, the Passwords are stored separately in their
            // own LSA secret.
            //

            NtStatus = NlpReadSecret(&CurrentSecret, &OldSecret);

            //
            // can release the cache lock now, since second data item fetched.
            //

            LEAVE_CACHE();
            fCacheLocked = FALSE;

            if (NT_SUCCESS(NtStatus)) {

                if (CurrentSecret) {
                    //
                    // Check to see which version of the passwords are stored
                    // here - the normal or the salted.
                    //

                    RtlCopyMemory((PVOID)Passwords,
                        (PVOID)CurrentSecret->Buffer,
                        (ULONG)CurrentSecret->Length
                        );
                    if (CacheEntry->Revision < NLP_CACHE_REVISION_NT_4_SP4 ) {
                        if (Passwords->SecretPasswords.NtPasswordPresent) {
                            NtStatus = NlpComputeSaltedHashedPassword(
                                            &Passwords->SecretPasswords.NtOwfPassword,
                                            &Passwords->SecretPasswords.NtOwfPassword,
                                            &SamInfo->EffectiveName
                                            );
                        }
                        if (NT_SUCCESS(NtStatus) && Passwords->SecretPasswords.LmPasswordPresent) {
                            NtStatus = NlpComputeSaltedHashedPassword(
                                            &Passwords->SecretPasswords.LmOwfPassword,
                                            &Passwords->SecretPasswords.LmOwfPassword,
                                            &SamInfo->EffectiveName
                                            );

                        }

                    }
                }
            }

        }

    }



    if( fCacheLocked ) {
        LEAVE_CACHE();
    }


    //
    // free structure allocated by NlpReadCacheEntry
    //

    if( CacheEntry ) {
        ZeroMemory( CacheEntry, EntrySize );
        FreeToHeap(CacheEntry);
    }


    //
    // free structures allocated by NlpReadSecret
    //

    if (CurrentSecret) {
        MIDL_user_free(CurrentSecret);
    }
    if (OldSecret) {
        MIDL_user_free(OldSecret);
    }


    if( NT_SUCCESS( NtStatus ) ) {
        *AccountInfo = SamInfo;
    } else {

        if ( SamInfo != NULL ) {
            MIDL_user_free( SamInfo );
        }

        if( ppSupplementalCacheData && *ppSupplementalCacheData ) {
            MIDL_user_free( *ppSupplementalCacheData );
            *ppSupplementalCacheData = NULL;
        }
    }

    return(NtStatus);
}


NTSTATUS
NlpAddCacheEntry(
    IN  PNETLOGON_INTERACTIVE_INFO LogonInfo,
    IN  PNETLOGON_VALIDATION_SAM_INFO2 AccountInfo,
    IN  PVOID SupplementalCacheData,
    IN  ULONG SupplementalCacheDataLength
    )

/*++

Routine Description:

    Adds this domain:user interactive logon information to the cache.

Arguments:

    LogonInfo   - pointer to NETLOGON_INTERACTIVE_INFO structure which contains
                  the domain name, user name and password for this user. These
                  are what the user typed to WinLogon

    AccountInfo - pointer to NETLOGON_VALIDATION_SAM_INFO2 structure which
                  contains this user's specific interactive logon information

Return Value:

    NTSTATUS
        Success = STATUS_SUCCESS
                    AccountInfo successfully added to cache

        Failure = STATUS_NO_MEMORY


--*/

{
    NTSTATUS
        NtStatus;

    PLOGON_CACHE_ENTRY
        CacheEntry = NULL;

    PLOGON_CACHE_ENTRY
        CacheEntryExisting = NULL;

    ULONG
        EntrySize,
        EntrySizeExisting,
        Index;



#if DBG
    if (DumpCacheInfo) {
        DbgPrint("NlpAddCacheEntry\n");
        DumpLogonInfo(&LogonInfo->Identity);
        DumpAccountInfo(AccountInfo);
    }
#endif

    if (NlpInitializationNotYetPerformed) {
        NtStatus = NlpInternalCacheInitialize();
        if (!NT_SUCCESS(NtStatus)) {
            return(NtStatus);
        }
    }

    if (NlpCacheControl.Entries == 0) {
        return(STATUS_SUCCESS);
    }



    //
    // build base cache entry.
    //

    NtStatus = NlpBuildCacheEntry(LogonInfo, AccountInfo, &CacheEntry, &EntrySize);

    if(!NT_SUCCESS(NtStatus) )
        return (NtStatus);



    //
    // add any supplemental data to the cache entry.
    // (this is new for NT5).
    //

    NtStatus = NlpAddSupplementalCacheData(
                            SupplementalCacheData,
                            SupplementalCacheDataLength,
                            &CacheEntry,
                            &EntrySize
                            );


    if(!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    //
    // add in salted OWFs.
    //

    NtStatus = NlpMakeSecretPasswordNT5(
                            &CacheEntry->CachePasswords,
                            &AccountInfo->EffectiveName,
                            &LogonInfo->NtOwfPassword,
                            &LogonInfo->LmOwfPassword
                            );

    if(!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }



    //
    // encrypt sensitive portions of the cache entry.
    //

    NtStatus = NlpEncryptCacheEntry(CacheEntry, EntrySize);

    if(!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }


    ENTER_CACHE();

    //
    // See if this entry already exists in the cache.
    // If so, use the same index.
    //

    NtStatus = NlpReadCacheEntry(&LogonInfo->Identity.LogonDomainName,
                                 &LogonInfo->Identity.UserName,
                                  &Index,
                                  &CacheEntryExisting,
                                  &EntrySizeExisting
                                  );

    //
    // If we didn't find an entry, then we need to allocate an
    // entry.
    //

    if (!NT_SUCCESS(NtStatus)) {

        NlpGetFreeEntryIndex( &Index );

        CacheEntryExisting = NULL;

    } else {

        //
        // We already have an entry for this user.
        // Discard the structure we got back but
        // use the same index.
        // Note: structure discarded outside cache lock.
        //

        // TODO: check if existing entry matches new built entry.
        // if so, avoid write.
    }


    //
    // now, write the entry out...
    //

    NtStatus = NlpWriteCacheEntry(Index, CacheEntry, EntrySize);


    if (NT_SUCCESS(NtStatus)) {
        NlpCteTable[Index].Time = CacheEntry->Time;
        NlpAddEntryToActiveList( Index );
    }

    LEAVE_CACHE();


Cleanup:


    if( CacheEntry ) {
        ZeroMemory( CacheEntry, EntrySize );
        FreeCacheEntry( CacheEntry );
    }

    if( CacheEntryExisting ) {
        ZeroMemory( CacheEntryExisting, EntrySizeExisting );
        FreeCacheEntry( CacheEntryExisting );
    }

    return(NtStatus);
}



NTSTATUS
NlpAddSupplementalCacheData(
    IN      PVOID SupplementalCacheData,
    IN      ULONG SupplementalCacheDataLength,
    IN OUT  PLOGON_CACHE_ENTRY *ppCacheEntry,
    IN OUT  PULONG pEntryLength
    )

/*++

Routine Description:

    Extends the supplied LOGON_CACHE_ENTRY with opaque authentication package
    SupplementalCacheData (eg: smart-card logon cache info).

Return Value:

    NTSTATUS
        Success = STATUS_SUCCESS

        Failure =

--*/

{
    PLOGON_CACHE_ENTRY NewCacheEntry = NULL;

    if( (*ppCacheEntry)->Revision < NLP_CACHE_REVISION_NT_5_0 ) {
        return STATUS_SUCCESS;
    }

    (*ppCacheEntry)->SupplementalCacheDataLength = SupplementalCacheDataLength;
    (*ppCacheEntry)->SupplementalCacheDataOffset = *pEntryLength;


    if( SupplementalCacheData == NULL || SupplementalCacheDataLength == 0 ) {
        return STATUS_SUCCESS;
    }


    //
    // allocate new entry, and copy existing entry + supplemental data to end.
    //

    NewCacheEntry = AllocateCacheEntry( *pEntryLength + SupplementalCacheDataLength );

    if( NewCacheEntry == NULL ) {
        return STATUS_NO_MEMORY;
    }

    CopyMemory( NewCacheEntry, *ppCacheEntry, *pEntryLength );
    CopyMemory( ((PBYTE)(NewCacheEntry) + *pEntryLength),
                SupplementalCacheData,
                SupplementalCacheDataLength
                );

    ZeroMemory( *ppCacheEntry, *pEntryLength );
    FreeCacheEntry( *ppCacheEntry );

    *ppCacheEntry = NewCacheEntry;
    *pEntryLength += SupplementalCacheDataLength;

    return STATUS_SUCCESS;
}


NTSTATUS
NlpDeleteCacheEntry(
    IN  PNETLOGON_INTERACTIVE_INFO LogonInfo
    )

/*++

Routine Description:

    Deletes a user account from the local user account cache, if the corresponding
    entry can be found. We actually just null out the current contents instead of
    destroying the storage - this should save us some time when we next come to
    add an entry to the cache

Arguments:

    LogonInfo   - pointer to NETLOGON_INTERACTIVE_INFO structure which contains
                  the domain name, user name and password for this user

Return Value:

    NTSTATUS
        Success = STATUS_SUCCESS

        Failure =

--*/

{
    NTSTATUS
        NtStatus;

    PLOGON_CACHE_ENTRY
        CacheEntry = NULL;

    ULONG
        EntrySize,
        Index;


    if (NlpInitializationNotYetPerformed) {
        NtStatus = NlpInternalCacheInitialize();
        if (!NT_SUCCESS(NtStatus)) {
            return(NtStatus);
        }
    }


    if (NlpCacheControl.Entries == 0) {
        return(STATUS_SUCCESS);
    }

    ENTER_CACHE();

    //
    // See if this entry exists in the cache.
    //

    NtStatus = NlpReadCacheEntry( &LogonInfo->Identity.LogonDomainName,
                                  &LogonInfo->Identity.UserName,
                                  &Index,
                                  &CacheEntry,
                                  &EntrySize
                                  );

    //
    // If we didn't find an entry, then there is nothing to do.
    //

    if (!NT_SUCCESS(NtStatus)) {
        LEAVE_CACHE();
        return(STATUS_SUCCESS);
    }

    //
    // Mark it as invalid.
    //

    CacheEntry->Valid = FALSE;

    NtStatus = NlpWriteCacheEntry( Index, CacheEntry, EntrySize );

    if (NT_SUCCESS(NtStatus)) {

        //
        // Put the CTE entry on the inactive list.
        //

        NlpAddEntryToInactiveList( Index );
    }


    LEAVE_CACHE();


    //
    // Free the structure returned from NlpReadCacheEntry()
    //

    if( CacheEntry ) {
        ZeroMemory( CacheEntry, EntrySize );
        FreeToHeap( CacheEntry );
    }


    return(NtStatus);
}


VOID
NlpChangeCachePassword(
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING UserName,
    IN PLM_OWF_PASSWORD LmOwfPassword,
    IN PNT_OWF_PASSWORD NtOwfPassword
    )

/*++

Routine Description:

    Update a cached password to the specified value, if we have
    the specified account cached.

Arguments:


    DomainName - The name of the domain in which the account exists.

    UserName - The name of the account whose password is to be changed.

    LmOwfPassword - The new LM compatible password.

    NtOwfPassword - The new NT compatible password.

Return Value:

    None.

--*/

{
    NTSTATUS
        NtStatus;

    PLOGON_CACHE_ENTRY
        CacheEntry = NULL;

    ULONG
        EntrySize,
        Index;

    PLSAPR_CR_CIPHER_VALUE
        CurrentSecret = NULL,
        OldSecret = NULL;

    LSAPR_CR_CIPHER_VALUE
        Passwords;


#if DBG
    if (DumpCacheInfo) {
        DbgPrint("NlpChangeCachePassword\n");
    }
#endif

    if (NlpInitializationNotYetPerformed) {
        NtStatus = NlpInternalCacheInitialize();
        if (!NT_SUCCESS(NtStatus)) {
            return;
        }
    }


    if (NlpCacheControl.Entries == 0) {
        return;
    }

    ENTER_CACHE();


    NtStatus = NlpReadCacheEntry( DomainName,
                                  UserName,
                                  &Index,
                                  &CacheEntry,
                                  &EntrySize);

    if(!NT_SUCCESS( NtStatus) ) {
        LEAVE_CACHE();
        return ;
    }


    if( CacheEntry->Revision >= NLP_CACHE_REVISION_NT_5_0 ) {
        UNICODE_STRING CachedUser;

        CachedUser.Length =
            CachedUser.MaximumLength = CacheEntry->UserNameLength;
        CachedUser.Buffer = (PWSTR) ((PBYTE) CacheEntry + sizeof(LOGON_CACHE_ENTRY));

        NtStatus = NlpMakeSecretPasswordNT5( &CacheEntry->CachePasswords,
                                          &CachedUser,
                                          NtOwfPassword,
                                          LmOwfPassword );


        if(NT_SUCCESS(NtStatus)) {

            //
            // encrypt the entry...
            //

            NtStatus = NlpEncryptCacheEntry( CacheEntry, EntrySize );
        }

        if(NT_SUCCESS( NtStatus )) {

            //
            // now, write the entry out...
            //

            NtStatus = NlpWriteCacheEntry(Index, CacheEntry, EntrySize);

#ifdef DBG
            if(DumpCacheInfo) {
                if( NT_SUCCESS( NtStatus ) ) {
                    DbgPrint("NlpChangeCachePassword: SUCCEED write NT5 version cache entry.\n");
                } else {
                    DbgPrint("NlpChangeCachePassword: FAIL write NT5 version cache entry.\n");
                }
            }
#endif

        }

    } else {

        NtStatus = NlpOpenSecret( Index );
        if (NT_SUCCESS(NtStatus)) {

            NtStatus = NlpReadSecret(&CurrentSecret, &OldSecret);
            if (NT_SUCCESS(NtStatus)) {
                UNICODE_STRING CachedUser;

                //
                // Grab the various strings from the cache entry.
                //
                ASSERT( CacheEntry->Revision >= NLP_CACHE_REVISION_NT_1_0B );

                CachedUser.Length =
                    CachedUser.MaximumLength = CacheEntry->UserNameLength;
                CachedUser.Buffer = (PWSTR) ((PBYTE) CacheEntry + sizeof(LOGON_CACHE_ENTRY_NT_4_SP4));

                NtStatus = NlpMakeSecretPassword( &Passwords,
                                                  &CachedUser,
                                                  NtOwfPassword,
                                                  LmOwfPassword );

                if (NT_SUCCESS(NtStatus)) {
                    NtStatus = NlpWriteSecret(&Passwords, CurrentSecret);

                    //
                    // free the buffer allocated to store the passwords
                    //

                    FreeToHeap(Passwords.Buffer);
                }

                //
                // free strings returned by NlpReadSecret
                //

                if (CurrentSecret) {
                    MIDL_user_free(CurrentSecret);
                }
                if (OldSecret) {
                    MIDL_user_free(OldSecret);
                }

            }
        }

    }

    LEAVE_CACHE();


    //
    // free structure allocated by NlpReadCacheEntry
    //

    if( CacheEntry ) {
        ZeroMemory( CacheEntry, EntrySize );
        FreeToHeap(CacheEntry);
    }

    return;
}


////////////////////////////////////////////////////////////////////////
//                                                                    //
// Services Internal to this module                                   //
//                                                                    //
////////////////////////////////////////////////////////////////////////


NTSTATUS
NlpInternalCacheInitialize(
    VOID
    )

/*++

Routine Description:

    This routine is called to initialize cached logon processing.

    This routine will automatically adjust the size of the logon
    cache if necessary to accomodate a new user-specified length
    (specified in the Winlogon part of the registry).

    NOTE: If called too early, this routine won't be able to call
          LSA's RPC routines.  In this case, initialization is
          defered until later.

Arguments:

    None.

Return Value:

    NTSTATUS

--*/

{

    NTSTATUS
        NtStatus;

    OBJECT_ATTRIBUTES
        ObjectAttributes;

// DbgPrint("\n\n\n     REMEMBER TO TAKE THIS BREAKPOINT OUT BEFORE CHECKIN.\n\n\n");
// DumpCacheInfo = 1;   // Remember to take this out too !!!!!!
// DbgBreakPoint();     // Remember to take this out before checking

#if DBG
    if (DumpCacheInfo) {
        DbgPrint("NlpCacheInitialize\n");
    }
#endif


    //
    // Upon return from this routine, if logon caching is enabled,
    // the following will be true:
    //
    //      A handle to the LsaPolicy object will be open (NlpLsaHandle).
    //
    //      A handle to the registry key in which all cache entries
    //      are held will be open (NlpCacheHandle).
    //
    //      A global structure defining how many cache entries there are
    //      will be initialized (NlpCacheControl).
    //
    //      The Cache Table Entry table (CTE table) will be initialized
    //      (NlpCteTable).
    //
    //      The active and inactive CTE lists will be built
    //      (NlpActiveCtes and NlpInactiveCtes).
    //
    //      A global cache encryption key will be initialized.
    //



    ENTER_CACHE();

    //
    // Check again if the cache is initialized now that the crit sect is locked.
    //

    if (NlpInitializationNotYetPerformed) {

        //
        // Open the local system's policy object
        //

        InitializeObjectAttributes(&ObjectAttributes,
                                   NULL,   // name
                                   0,
                                   0,
                                   NULL
                                   );
        NtStatus = I_LsaIOpenPolicyTrusted( &NlpLsaHandle );
        if (NT_SUCCESS(NtStatus)) {

            //
            // Successfully, or unsucessfully,
            // The definition of "initialized" is we could call LSA's RPC
            // routines.
            //

            NlpInitializationNotYetPerformed = FALSE;

            //
            // Open the registry key containing cache entries.
            // This will remain open.
            //

            NtStatus = NlpOpenCache();

            if (NT_SUCCESS(NtStatus)) {

                //
                // Get information on the current cache structure
                // (number of entries, et cetera).  This information is
                // placed in a global variable for use throughout this
                // module.
                //

                NtStatus = NlpGetCacheControlInfo();

                //
                // Initialize the per-machine cache encryption key.
                //

                if(NT_SUCCESS( NtStatus) ) {
                    NtStatus = NlpCacheKeyInitialize();
                }

                //
                // Now build the CTE table
                //

                if (NT_SUCCESS(NtStatus)) {
                    NtStatus = NlpBuildCteTable();
                }

                //
                // If we were successful, then see if we need to change
                // the cache due to new user-specified cache size.
                //

                if (NT_SUCCESS(NtStatus)) {
                    NtStatus = NlpChangeCacheSizeIfNecessary();
                }

                if (!NT_SUCCESS(NtStatus)) {
                    NlpCloseCache();
                }
            }

            if (!NT_SUCCESS(NtStatus)) {
                I_LsarClose( &NlpLsaHandle );
            }
        }

        //
        // If we had an error, then set our entry count to zero
        // to prevent using any cache information.
        //

        if (!NT_SUCCESS(NtStatus)) {
            NlpCacheControl.Entries = 0;
        }

    } else {
        NtStatus = STATUS_SUCCESS;
    }

    LEAVE_CACHE();

    return(NtStatus);
}


NTSTATUS
NlpCacheKeyInitialize(
    VOID
    )
/*++

Routine Description:

    Initializes the Global variable NlpCacheEncryptionKey with a per-machine
    cache encryption key.  If the per-machine key does not exist as an LSA
    secret, it is created.

--*/
{
    LSAPR_HANDLE SecretHandle;
    UNICODE_STRING ValueName;
    BOOLEAN SecretCreationNeeded = FALSE;
    NTSTATUS NtStatus;

    RtlInitUnicodeString( &ValueName, NLP_CACHE_ENCRYPTION_KEY_NAME );

    NtStatus = I_LsarOpenSecret(NlpLsaHandle,
                             (PLSAPR_UNICODE_STRING) &ValueName,
                             SECRET_QUERY_VALUE | SECRET_SET_VALUE,
                             &SecretHandle
                             );

    if (!NT_SUCCESS(NtStatus)) {

        //
        // create new key, if not present.
        //

        if (NtStatus != STATUS_OBJECT_NAME_NOT_FOUND) {
            return (NtStatus);
        }

        NtStatus = I_LsarCreateSecret(NlpLsaHandle,
                                   (PLSAPR_UNICODE_STRING) &ValueName,
                                   SECRET_SET_VALUE,
                                   &SecretHandle
                                   );

        if (!NT_SUCCESS(NtStatus)) {
            return (NtStatus);
        }

        SecretCreationNeeded = TRUE;

    } else {

        //
        // query current value...
        //

        LARGE_INTEGER
            CurrentTime;

        PLSAPR_CR_CIPHER_VALUE CurrentSecret = NULL;

        NtStatus = I_LsarQuerySecret(SecretHandle,
                                  &CurrentSecret,
                                  &CurrentTime,
                                  NULL,
                                  NULL
                                  );

        if(NT_SUCCESS( NtStatus ) ) {
            if( CurrentSecret == NULL ) {

                //
                // non existing data, create it.
                //

                SecretCreationNeeded = TRUE;
            } else {

                //
                // size of data is wrong, bail now and leave things as-is.
                //

                if( CurrentSecret->Length != sizeof( NlpCacheEncryptionKey ) ) {
                    NtStatus = STATUS_SECRET_TOO_LONG;
                } else {

                    //
                    // capture existing data into global.
                    //

                    CopyMemory( NlpCacheEncryptionKey, CurrentSecret->Buffer, CurrentSecret->Length );
                }

                MIDL_user_free(CurrentSecret);
            }
        }
    }


    if( SecretCreationNeeded ) {
        LSAPR_CR_CIPHER_VALUE SecretValue;

        SspGenerateRandomBits( NlpCacheEncryptionKey, sizeof(NlpCacheEncryptionKey) );

        //
        // write out secret...
        //

        SecretValue.Length = sizeof(NlpCacheEncryptionKey);
        SecretValue.MaximumLength = SecretValue.Length;
        SecretValue.Buffer = (PBYTE)NlpCacheEncryptionKey;

        NtStatus = I_LsarSetSecret(SecretHandle,
                                &SecretValue,
                                NULL
                                );

    }


    I_LsarClose( &SecretHandle );

    return (NtStatus);
}


NTSTATUS
NlpEncryptCacheEntry(
    IN  PLOGON_CACHE_ENTRY CacheEntry,
    IN  ULONG EntrySize
    )
/*++

Routine Description:

    Encrypts the sensitive portions of the input CacheEntry.

--*/
{
    HMACMD5_CTX hmacCtx;
    RC4_KEYSTRUCT rc4key;
    CHAR DerivedKey[ MD5DIGESTLEN ];

    PBYTE pbData;
    ULONG cbData;

    if( CacheEntry->Revision < NLP_CACHE_REVISION_NT_5_0 ) {
        return STATUS_SUCCESS;
    }


    //
    // derive encryption key from global machine LSA secret, and random
    // cache entry key.
    //

    HMACMD5Init(&hmacCtx, NlpCacheEncryptionKey, sizeof(NlpCacheEncryptionKey));
    HMACMD5Update(&hmacCtx, CacheEntry->RandomKey, sizeof(CacheEntry->RandomKey));
    HMACMD5Final(&hmacCtx, DerivedKey);


    //
    // begin encrypting at the cachepasswords field.
    //

    pbData = (PBYTE)&(CacheEntry->CachePasswords);

    //
    // data length is EntrySize - header up to CachePasswords.
    //

    cbData = EntrySize - (ULONG)( pbData - (PBYTE)CacheEntry );


    //
    // MAC the data for integrity checking.
    //

    HMACMD5Init(&hmacCtx, DerivedKey, sizeof(DerivedKey));
    HMACMD5Update(&hmacCtx, pbData, cbData);
    HMACMD5Final(&hmacCtx, CacheEntry->MAC);

    //
    // now encrypt it...
    //

    rc4_key( &rc4key, sizeof(DerivedKey), DerivedKey );
    rc4( &rc4key, cbData, pbData );

    ZeroMemory( DerivedKey, sizeof(DerivedKey) );

    return STATUS_SUCCESS;
}


NTSTATUS
NlpDecryptCacheEntry(
    IN  PLOGON_CACHE_ENTRY CacheEntry,
    IN  ULONG EntrySize
    )
/*++

Routine Description:

    Decrypts the sensitive portions of the input CacheEntry, and verified
    integrity of decrypted data.

--*/
{
    HMACMD5_CTX hmacCtx;
    RC4_KEYSTRUCT rc4key;
    CHAR DerivedKey[ MD5DIGESTLEN ];

    CHAR MAC[ MD5DIGESTLEN ];

    PBYTE pbData;
    ULONG cbData;

    if( CacheEntry->Revision < NLP_CACHE_REVISION_NT_5_0 ) {
        return STATUS_SUCCESS;
    }


    //
    // derive encryption key from global machine LSA secret, and random
    // cache entry key.
    //

    HMACMD5Init(&hmacCtx, NlpCacheEncryptionKey, sizeof(NlpCacheEncryptionKey));
    HMACMD5Update(&hmacCtx, CacheEntry->RandomKey, sizeof(CacheEntry->RandomKey));
    HMACMD5Final(&hmacCtx, DerivedKey);


    //
    // begin decrypting at the cachepasswords field.
    //

    pbData = (PBYTE)&(CacheEntry->CachePasswords);

    //
    // data length is EntrySize - header up to CachePasswords.
    //

    cbData = EntrySize - (ULONG)( pbData - (PBYTE)CacheEntry );

    //
    // now decrypt it...
    //

    rc4_key( &rc4key, sizeof(DerivedKey), DerivedKey );
    rc4( &rc4key, cbData, pbData );


    //
    // compute MAC on decrypted data for integrity checking.
    //

    HMACMD5Init(&hmacCtx, DerivedKey, sizeof(DerivedKey));
    HMACMD5Update(&hmacCtx, pbData, cbData);
    HMACMD5Final(&hmacCtx, MAC);

    ZeroMemory( DerivedKey, sizeof(DerivedKey) );


    //
    // verify MAC.
    //

    if( memcmp( MAC, CacheEntry->MAC, sizeof(MAC) ) != 0 ) {
        return STATUS_LOGON_FAILURE;
    }


    return STATUS_SUCCESS;
}




NTSTATUS
NlpBuildCacheEntry(
    IN  PNETLOGON_INTERACTIVE_INFO LogonInfo,
    IN  PNETLOGON_VALIDATION_SAM_INFO2 AccountInfo,
    OUT PLOGON_CACHE_ENTRY* ppCacheEntry,
    OUT PULONG pEntryLength
    )

/*++

Routine Description:

    Builds a LOGON_CACHE_ENTRY from a NETLOGON_VALIDATION_SAM_INFO2 structure.
    We only cache those fields that we cannot easily re-invent

Arguments:

    LogonInfo       - pointer to NETLOGON_INTERACTIVE_INFO structure containing
                      user's name and logon domain name

    AccountInfo     - pointer to NETLOGON_VALIDATION_SAM_INFO2 from successful
                      logon

    ppCacheEntry    - pointer to place to return pointer to allocated
                      LOGON_CACHE_ENTRY

    pEntryLength    - size of the buffer returned in *ppCacheEntry

Return Value:

    NTSTATUS
        Success = STATUS_SUCCESS
                    *ppCacheEntry contains pointer to allocated LOGON_CACHE_ENTRY
                    structure

        Failure = STATUS_NO_MEMORY
                    *ppCacheEntry undefined

--*/

{
    PLOGON_CACHE_ENTRY pEntry;
    ULONG length;
    PCHAR dataptr;

    UNICODE_STRING SamAccountName;
    UNICODE_STRING NetbiosDomainName;
    UNICODE_STRING DnsDomainName;
    UNICODE_STRING Upn;

    NTSTATUS NtStatus;


    //
    // Grab the various forms of the account name
    //

    NlpGetAccountNames( &LogonInfo->Identity,
                        AccountInfo,
                        &SamAccountName,
                        &NetbiosDomainName,
                        &DnsDomainName,
                        &Upn );

    //
    // assumes GROUP_MEMBERSHIP is integral multiple of DWORDs
    //

    length = ROUND_UP_COUNT(sizeof(LOGON_CACHE_ENTRY), sizeof(ULONG))
                + ROUND_UP_COUNT(NetbiosDomainName.Length, sizeof(ULONG))
                + ROUND_UP_COUNT(SamAccountName.Length, sizeof(ULONG))
                + ROUND_UP_COUNT(DnsDomainName.Length, sizeof(ULONG))
                + ROUND_UP_COUNT(Upn.Length, sizeof(ULONG))
                + ROUND_UP_COUNT(AccountInfo->EffectiveName.Length, sizeof(ULONG))
                + ROUND_UP_COUNT(AccountInfo->FullName.Length, sizeof(ULONG))
                + ROUND_UP_COUNT(AccountInfo->LogonScript.Length, sizeof(ULONG))
                + ROUND_UP_COUNT(AccountInfo->ProfilePath.Length, sizeof(ULONG))
                + ROUND_UP_COUNT(AccountInfo->HomeDirectory.Length, sizeof(ULONG))
                + ROUND_UP_COUNT(AccountInfo->HomeDirectoryDrive.Length, sizeof(ULONG))
                + ROUND_UP_COUNT(AccountInfo->LogonDomainName.Length, sizeof(ULONG))
                + ROUND_UP_COUNT(AccountInfo->GroupCount * sizeof(GROUP_MEMBERSHIP), sizeof(ULONG))
                + ROUND_UP_COUNT(RtlLengthSid(AccountInfo->LogonDomainId), sizeof(ULONG));

    if (AccountInfo->UserFlags & LOGON_EXTRA_SIDS) {
        if (AccountInfo->SidCount) {
            ULONG i;
            length += ROUND_UP_COUNT(AccountInfo->SidCount * sizeof(ULONG), sizeof(ULONG));
            for (i = 0; i < AccountInfo->SidCount ; i++ ) {
                length += ROUND_UP_COUNT(RtlLengthSid(AccountInfo->ExtraSids[i].Sid), sizeof(ULONG));
            }
        }
    }


    pEntry = AllocateCacheEntry(length);
    if (pEntry == NULL) {
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory( pEntry, length );
    pEntry->Revision = NLP_CACHE_REVISION;
    NtQuerySystemTime( &pEntry->Time );
    pEntry->Valid    = TRUE;
    pEntry->LogonPackage = LogonInfo->Identity.ParameterControl;


    dataptr = (PCHAR)(pEntry + 1);
    *pEntryLength = length;

    ASSERT(!((ULONG_PTR)dataptr & (sizeof(ULONG) - 1)));

    //
    // each of these (unicode) strings and other structures are copied to the
    // end of the fixed LOGON_CACHE_ENTRY structure, each aligned on DWORD
    // boundaries
    //

    length = pEntry->UserNameLength = SamAccountName.Length;
    RtlCopyMemory(dataptr, SamAccountName.Buffer, length);
    dataptr = ROUND_UP_POINTER(dataptr+length, sizeof(ULONG));

    length = pEntry->DomainNameLength = NetbiosDomainName.Length;
    if (length) {
        RtlCopyMemory(dataptr, NetbiosDomainName.Buffer, length);
        dataptr = ROUND_UP_POINTER(dataptr+length, sizeof(ULONG));
    }

    length = pEntry->DnsDomainNameLength = DnsDomainName.Length;
    if (length) {
        RtlCopyMemory(dataptr, DnsDomainName.Buffer, length);
        dataptr = ROUND_UP_POINTER(dataptr+length, sizeof(ULONG));
    }

    length = pEntry->UpnLength = Upn.Length;
    if (length) {
        RtlCopyMemory(dataptr, Upn.Buffer, length);
        dataptr = ROUND_UP_POINTER(dataptr+length, sizeof(ULONG));
    }

    length = pEntry->EffectiveNameLength = AccountInfo->EffectiveName.Length;
    if (length) {
        RtlCopyMemory(dataptr, AccountInfo->EffectiveName.Buffer, length);
        dataptr = ROUND_UP_POINTER(dataptr+length, sizeof(ULONG));
    }

    length = pEntry->FullNameLength = AccountInfo->FullName.Length;
    if (length) {
        RtlCopyMemory(dataptr, AccountInfo->FullName.Buffer, length);
        dataptr = ROUND_UP_POINTER(dataptr+length, sizeof(ULONG));
    }

    length = pEntry->LogonScriptLength = AccountInfo->LogonScript.Length;
    if (length) {
        RtlCopyMemory(dataptr, AccountInfo->LogonScript.Buffer, length);
        dataptr = ROUND_UP_POINTER(dataptr+length, sizeof(ULONG));
    }

    length = pEntry->ProfilePathLength = AccountInfo->ProfilePath.Length;
    if (length) {
        RtlCopyMemory(dataptr, AccountInfo->ProfilePath.Buffer, length);
        dataptr = ROUND_UP_POINTER(dataptr+length, sizeof(ULONG));
    }

    length = pEntry->HomeDirectoryLength = AccountInfo->HomeDirectory.Length;
    if (length) {
        RtlCopyMemory(dataptr, AccountInfo->HomeDirectory.Buffer, length);
        dataptr = ROUND_UP_POINTER(dataptr+length, sizeof(ULONG));
    }

    length = pEntry->HomeDirectoryDriveLength = AccountInfo->HomeDirectoryDrive.Length;
    if (length) {
        RtlCopyMemory(dataptr, AccountInfo->HomeDirectoryDrive.Buffer, length);
        dataptr = ROUND_UP_POINTER(dataptr+length, sizeof(ULONG));
    }

    pEntry->UserId = AccountInfo->UserId;
    pEntry->PrimaryGroupId = AccountInfo->PrimaryGroupId;

    length = pEntry->GroupCount = AccountInfo->GroupCount;
    length *= sizeof(GROUP_MEMBERSHIP);
    if (length) {
        RtlCopyMemory(dataptr, AccountInfo->GroupIds, length);
        dataptr = ROUND_UP_POINTER(dataptr+length, sizeof(ULONG));
    }

    length = pEntry->LogonDomainNameLength = AccountInfo->LogonDomainName.Length;
    if (length) {
        RtlCopyMemory(dataptr, AccountInfo->LogonDomainName.Buffer, length);
        dataptr = ROUND_UP_POINTER(dataptr+length, sizeof(ULONG));
    }

    if (AccountInfo->UserFlags & LOGON_EXTRA_SIDS) {
        length = pEntry->SidCount = AccountInfo->SidCount;
        length *= sizeof(ULONG);
        if (length) {
            ULONG i, sidLength;
            PULONG sidAttributes = (PULONG) dataptr;

            dataptr = ROUND_UP_POINTER(dataptr+length, sizeof(ULONG));

            //
            // Now copy over all the SIDs
            //

            for (i = 0; i < AccountInfo->SidCount ; i++ ) {
                sidAttributes[i] = AccountInfo->ExtraSids[i].Attributes;
                sidLength = RtlLengthSid(AccountInfo->ExtraSids[i].Sid);
                RtlCopySid(sidLength,(PSID) dataptr,AccountInfo->ExtraSids[i].Sid);
                dataptr = ROUND_UP_POINTER(dataptr + sidLength, sizeof(ULONG));
            }
            pEntry->SidLength = (ULONG) (dataptr - (PCHAR) sidAttributes);
        } else {
            pEntry->SidLength = 0;
        }
    } else {
        pEntry->SidCount = 0;
        pEntry->SidLength = 0;
    }

    pEntry->LogonDomainIdLength = (USHORT) RtlLengthSid(AccountInfo->LogonDomainId);

    NtStatus = RtlCopySid(pEntry->LogonDomainIdLength,
                          (PSID)dataptr,
                          AccountInfo->LogonDomainId
                          );
    ASSERT(NT_SUCCESS(NtStatus));
    dataptr = ROUND_UP_POINTER(dataptr+length, sizeof(ULONG));

    //
    // fill in randomkey for this cache entry.
    //

    SspGenerateRandomBits( pEntry->RandomKey, sizeof(pEntry->RandomKey) );

    *ppCacheEntry = pEntry;

#if DBG
    if (DumpCacheInfo) {
        DbgPrint("BuildCacheEntry:\n");
        DumpCacheEntry(999,pEntry);
    }
#endif
    return STATUS_SUCCESS;
}


NTSTATUS
NlpOpenCache( VOID )

/*++

Routine Description:

    Opens the registry node for read or write (depending on Switch) and opens
    the secret storage in the same mode.  If successful, the NlpCacheHandle
    is valid.

Arguments:

Return Value:

    NTSTATUS
        Success = STATUS_SUCCESS
                    NlpCacheHandle contains handle to use for reading/writing
                    registry

        Failure =

--*/

{
    NTSTATUS NtStatus;
    ULONG Disposition;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ObjectName;

    ObjectName.Length = ObjectName.MaximumLength = CACHE_NAME_SIZE;
    ObjectName.Buffer = CACHE_NAME;

    InitializeObjectAttributes(&ObjectAttributes,
                                &ObjectName,
                                OBJ_CASE_INSENSITIVE,
                                0,      // RootDirectory
                                NULL    // BUGBUG - put security descriptor here
                                );
    NtStatus = NtCreateKey(&NlpCacheHandle,
                           (KEY_WRITE | KEY_READ),
                           &ObjectAttributes,
                           CACHE_TITLE_INDEX,
                           NULL,   // class name
                           0,      // create options
                           &Disposition
                           );

    return NtStatus;
}


VOID
NlpCloseCache( VOID )

/*++

Routine Description:

    Closes handles opened by NlpOpenCache

Arguments:

    None.

Return Value:

    None.

--*/

{
#if DBG
    NTSTATUS NtStatus;

    if (DumpCacheInfo) {
        DbgPrint("CloseCache: Closing NlpCacheHandle (%#08x)\n", NlpCacheHandle);
    }

    if (IS_VALID_HANDLE(NlpCacheHandle)) {
        NtStatus = NtClose(NlpCacheHandle);
        if (DumpCacheInfo) {
            DbgPrint("CloseCache: NtClose returns %#08x\n", NtStatus);
        }
        ASSERT(NT_SUCCESS(NtStatus));
        INVALIDATE_HANDLE(NlpCacheHandle);
    }
#else
    if (IS_VALID_HANDLE(NlpCacheHandle)) {
        NtClose(NlpCacheHandle);
        INVALIDATE_HANDLE(NlpCacheHandle);
    }
#endif
}


NTSTATUS
NlpOpenSecret(
    IN  ULONG   Index
    )

/*++

Routine Description:

    Opens a cache entry's secret storage object for read (in order to LsaQuerySecret) and
    write (in order to LsaSetSecret).  If successful, the handle value
    is placed in the global variable NlpSecretHandle.

    If the secret does not exist, it will be created.


Arguments:

    Index - The index of the entry being opened.  This is used to build
        a name of the object.

Return Value:

    NTSTATUS
        Success = STATUS_SUCCESS
                    NlpSecretHandle can be used to read/write secret storage

        Failure =

--*/

{
    NTSTATUS
        NtStatus;

    UNICODE_STRING
        ValueName;

    WCHAR
        NameBuffer[32];


    //
    // Close previous handle if necessary
    //

    if (IS_VALID_HANDLE(NlpSecretHandle)) {
        I_LsarClose( &NlpSecretHandle );
    }


    ValueName.Buffer = &NameBuffer[0];
    ValueName.MaximumLength = 32;
    ValueName.Length = 0;
    NlpMakeCacheEntryName( Index, &ValueName );




    NtStatus = I_LsarOpenSecret(NlpLsaHandle,
                             (PLSAPR_UNICODE_STRING) &ValueName,
                             SECRET_QUERY_VALUE | SECRET_SET_VALUE,
                             &NlpSecretHandle
                             );

    if (!NT_SUCCESS(NtStatus)) {
        if (NtStatus == STATUS_OBJECT_NAME_NOT_FOUND) {
            NtStatus = I_LsarCreateSecret(NlpLsaHandle,
                                       (PLSAPR_UNICODE_STRING) &ValueName,
                                       SECRET_SET_VALUE | SECRET_QUERY_VALUE,
                                       &NlpSecretHandle
                                       );
            if (!NT_SUCCESS(NtStatus)) {
                INVALIDATE_HANDLE(NlpSecretHandle);
            }
        } else {
            INVALIDATE_HANDLE(NlpSecretHandle);
        }
    }
    return(NtStatus);
}


VOID
NlpCloseSecret( VOID )

/*++

Routine Description:

    Closes the handles opened via NlpOpenSecret

Arguments:

    None.

Return Value:

    None.

--*/

{
    NTSTATUS
        NtStatus;

    if (IS_VALID_HANDLE(NlpSecretHandle)) {
        NtStatus = I_LsarClose(&NlpSecretHandle);
#if DBG
        if (DumpCacheInfo) {
            DbgPrint("CloseSecret: LsaClose returns %#08x\n", NtStatus);
        }
#endif
        ASSERT(NT_SUCCESS(NtStatus));
        INVALIDATE_HANDLE(NlpSecretHandle);
    }
}


NTSTATUS
NlpWriteSecret(
    IN  PLSAPR_CR_CIPHER_VALUE NewSecret,
    IN  PLSAPR_CR_CIPHER_VALUE OldSecret
    )

/*++

Routine Description:

    Writes the password (and optionally the previous password) to the LSA
    secret storage

Arguments:

    NewSecret   - pointer to UNICODE_STRING containing current password
    OldSecret   - pointer to UNICODE_STRING containing previous password

Return Value:

    NTSTATUS
        Success =
        Failure =

--*/

{

    return I_LsarSetSecret(NlpSecretHandle, NewSecret, OldSecret);
}


NTSTATUS
NlpReadSecret(
    OUT PLSAPR_CR_CIPHER_VALUE * NewSecret,
    OUT PLSAPR_CR_CIPHER_VALUE * OldSecret
    )

/*++

Routine Description:

    Reads the new and old secrets (UNICODE_STRINGs) for the
    currently open LSA secret

    The Lsa routine returns us pointers to UNICODE strings

Arguments:

    NewSecret   - pointer to returned pointer to UNICODE_STRING containing
                  most recent password (if any)

    OldSecret   - pointer to returned pointer to UNICODE_STRING containing
                  previous password (if any)

Return Value:

    NTSTATUS
        Success
        Failure

--*/

{
    NTSTATUS
        NtStatus;

    LARGE_INTEGER
        NewTime,
        OldTime;



    NtStatus = I_LsarQuerySecret(NlpSecretHandle,
                              NewSecret,
                              &NewTime,
                              OldSecret,
                              &OldTime
                              );



#if DBG
    {
        char newNt[80];
        char newLm[80];
        char oldNt[80];
        char oldLm[80];

        if (DumpCacheInfo) {
            DbgPrint("NlpReadSecret: NewSecret.Nt = \"%s\"\n"
                     "            NewSecret.Lm = \"%s\"\n"
                     "            OldSecret.Nt = \"%s\"\n"
                     "            OldSecret.Lm = \"%s\"\n",
                     *NewSecret
                        ? DumpOwfPasswordToString(newNt, (PLM_OWF_PASSWORD)((*NewSecret)->Buffer))
                        : "",
                     *NewSecret
                        ? DumpOwfPasswordToString(newLm, (PLM_OWF_PASSWORD)((*NewSecret)->Buffer)+1)
                        : "",
                     *OldSecret
                        ? DumpOwfPasswordToString(oldNt, (PLM_OWF_PASSWORD)((*OldSecret)->Buffer))
                        : "",
                     *OldSecret
                        ? DumpOwfPasswordToString(oldLm, (PLM_OWF_PASSWORD)((*OldSecret)->Buffer)+1)
                        : ""
                     );
        }
    }
#endif

    return NtStatus;
}


NTSTATUS
NlpComputeSaltedHashedPassword(
    OUT PNT_OWF_PASSWORD SaltedOwfPassword,
    IN PNT_OWF_PASSWORD OwfPassword,
    IN PUNICODE_STRING UserName
    )

/*++

Routine Description:

    Computes the salted hash of a password by concatenating the user name
    with the OWF and computing the OWF of the combination.

Arguments:

    SaltedOwfPassword - receives the LM or NT salted password/
    OwfPassword - Contains the NT or LM owf password.
    UserName - Contains the name of the user, used for salt.

Return Value:

    NTSTATUS
        Success = STATUS_SUCCESS
                    Passwords created OK

        Failure = STATUS_NO_MEMORY
                    Not enough storage to create Passwords

--*/
{
    NTSTATUS Status;
    UNICODE_STRING TempString;
    UNICODE_STRING LowerUserName;

    //
    // Compute the lower case user name.
    //

    Status = RtlDowncaseUnicodeString( &LowerUserName,
                                       UserName,
                                       TRUE );

    if ( !NT_SUCCESS(Status)) {
        return Status;
    }


    //
    // Build a string that is a concatenation of the OWF and LowerCase username.
    //

    TempString.Length = TempString.MaximumLength = LowerUserName.Length + sizeof(NT_OWF_PASSWORD);
    TempString.Buffer = AllocateFromHeap( TempString.Length );
    if (TempString.Buffer == NULL) {
        RtlFreeUnicodeString( &LowerUserName );
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlCopyMemory(
        TempString.Buffer,
        OwfPassword,
        sizeof(NT_OWF_PASSWORD) );

    RtlCopyMemory(
        (PUCHAR) TempString.Buffer + sizeof(NT_OWF_PASSWORD),
        LowerUserName.Buffer,
        LowerUserName.Length );


    //
    // The Salted hash is the OWF of that.
    //
    Status = RtlCalculateNtOwfPassword(
                &TempString,
                SaltedOwfPassword
                );

    FreeToHeap(TempString.Buffer);
    RtlFreeUnicodeString( &LowerUserName );

    return(Status);

}


NTSTATUS
NlpMakeSecretPassword(
    OUT PLSAPR_CR_CIPHER_VALUE Passwords,
    IN  PUNICODE_STRING UserName,
    IN  PNT_OWF_PASSWORD NtOwfPassword OPTIONAL,
    IN  PLM_OWF_PASSWORD LmOwfPassword OPTIONAL
    )

/*++

Routine Description:

    Converts a (fixed length structure) NT_OWF_PASSWORD and a LM_OWF_PASSWORD
    to a UNICODE_STRING. Allocates memory for the unicode string in this function

    The calling function must free up the string buffer allocated in this routine.
    The caller uses FreeToHeap (RtlFreeHeap)

Arguments:

    Passwords       - returned UNICODE_STRING which actually contains a
                        CACHE_PASSWORDS structure
    NtOwfPassword   - pointer to encrypted, fixed-length NT password
    LmOwfPassword   - pointer to encrypted, fixed-length LM password

Return Value:

    NTSTATUS
        Success = STATUS_SUCCESS
                    Passwords created OK

        Failure = STATUS_NO_MEMORY
                    Not enough storage to create Passwords

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PCACHE_PASSWORDS pwd;

    Passwords->Buffer = NULL;

    pwd = (PCACHE_PASSWORDS)AllocateFromHeap(sizeof(*pwd));
    if (pwd == NULL) {
        return STATUS_NO_MEMORY;
    }

    //
    // concatenate the fixed length NT_OWF_PASSWORD and LM_OWF_PASSWORD structures
    // into a buffer which we then use as a UNICODE_STRING
    //

    if (ARGUMENT_PRESENT(NtOwfPassword)) {
        Status = NlpComputeSaltedHashedPassword(
                    &pwd->SecretPasswords.NtOwfPassword,
                    NtOwfPassword,
                    UserName
                    );
        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        pwd->SecretPasswords.NtPasswordPresent = TRUE;
    } else {
        RtlZeroMemory((PVOID)&pwd->SecretPasswords.NtOwfPassword,
                        sizeof(pwd->SecretPasswords.NtOwfPassword)
                        );
        pwd->SecretPasswords.NtPasswordPresent = FALSE;
    }

    if (ARGUMENT_PRESENT(LmOwfPassword)) {
        Status = NlpComputeSaltedHashedPassword(
                    &pwd->SecretPasswords.LmOwfPassword,
                    LmOwfPassword,
                    UserName
                    );
        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        pwd->SecretPasswords.LmPasswordPresent = TRUE;
    } else {
        RtlZeroMemory((PVOID)&pwd->SecretPasswords.LmOwfPassword,
                        sizeof(pwd->SecretPasswords.LmOwfPassword)
                        );
        pwd->SecretPasswords.LmPasswordPresent = FALSE;
    }

    Passwords->Length = Passwords->MaximumLength = sizeof(*pwd);
    Passwords->Buffer = (PUCHAR)pwd;

Cleanup:

    if( !NT_SUCCESS( Status ) ) {
        if( pwd != NULL )
            FreeToHeap( pwd );
    }

    return Status;
}


NTSTATUS
NlpMakeSecretPasswordNT5(
    IN OUT PCACHE_PASSWORDS Passwords,
    IN  PUNICODE_STRING UserName,
    IN  PNT_OWF_PASSWORD NtOwfPassword OPTIONAL,
    IN  PLM_OWF_PASSWORD LmOwfPassword OPTIONAL
    )

/*++

Routine Description:

    Populates CACHE_PASSWORDS structure with salted forms of NtOwfPassword
    and LmOwfPassword.

Arguments:

    Passwords       - populated CACHE_PASSWORDS structure.
    NtOwfPassword   - pointer to encrypted, fixed-length NT password
    LmOwfPassword   - pointer to encrypted, fixed-length LM password

Return Value:

    NTSTATUS
        Success = STATUS_SUCCESS
                    Passwords created OK

        Failure = STATUS_NO_MEMORY
                    Not enough storage to create Passwords

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PCACHE_PASSWORDS pwd;


    pwd = Passwords;

    //
    // concatenate the fixed length NT_OWF_PASSWORD and LM_OWF_PASSWORD structures
    // into a buffer which we then use as a UNICODE_STRING
    //

    if (ARGUMENT_PRESENT(NtOwfPassword)) {
        Status = NlpComputeSaltedHashedPassword(
                    &pwd->SecretPasswords.NtOwfPassword,
                    NtOwfPassword,
                    UserName
                    );
        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        pwd->SecretPasswords.NtPasswordPresent = TRUE;
    } else {
        RtlZeroMemory((PVOID)&pwd->SecretPasswords.NtOwfPassword,
                        sizeof(pwd->SecretPasswords.NtOwfPassword)
                        );
        pwd->SecretPasswords.NtPasswordPresent = FALSE;
    }

    //
    // Windows2000:
    // never store LMOWF -- since we never need it, and, this would
    // be the first thing attacked once a cache entry is unwrapped.
    //

#if 0

    if (ARGUMENT_PRESENT(LmOwfPassword)) {
        Status = NlpComputeSaltedHashedPassword(
                    &pwd->SecretPasswords.LmOwfPassword,
                    LmOwfPassword,
                    UserName
                    );
        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        pwd->SecretPasswords.LmPasswordPresent = TRUE;
    } else
#else
    UNREFERENCED_PARAMETER( LmOwfPassword );
#endif
    {
        RtlZeroMemory((PVOID)&pwd->SecretPasswords.LmOwfPassword,
                        sizeof(pwd->SecretPasswords.LmOwfPassword)
                        );
        pwd->SecretPasswords.LmPasswordPresent = FALSE;
    }


Cleanup:


    return Status;
}



NTSTATUS
NlpReadCacheEntry(
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING UserName,
    OUT PULONG              Index,
    OUT PLOGON_CACHE_ENTRY* CacheEntry,
    OUT PULONG              EntrySize
    )

/*++

Routine Description:

    Searches the active entry list for a domain\username
    match in the cache.  If a match is found, then it
    is returned.

Arguments:

    DomainName - The name of the domain in which the account exists.
        This can be the Netbios or Dns Domain Name.

    UserName - The name of the account whose password is to be changed.
        This can be the Sam Account Name.
        If DomainName is empty, this is the UPN of the account


    Index               - receives the index of the entry retrieved.

    CacheEntry          - pointer to place to return pointer to LOGON_CACHE_ENTRY

    EntrySize           - size of returned LOGON_CACHE_ENTRY


Return Value:

    NTSTATUS
        Success = STATUS_SUCCESS
                    *ppEntry points to allocated LOGON_CACHE_ENTRY
                    *EntrySize is size of returned data

        Failure = STATUS_NO_MEMORY
                    Couldn't allocate buffer for LOGON_CACHE_ENTRY

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    UNICODE_STRING CachedUser;
    UNICODE_STRING CachedDomain;
    UNICODE_STRING CachedDnsDomain;
    UNICODE_STRING CachedUpn;

    PNLP_CTE
        Next;


    //
    // Walk the active list looking for a domain/name match
    //

    Next = (PNLP_CTE)NlpActiveCtes.Flink;

    while (Next != (PNLP_CTE)&NlpActiveCtes) {

        NtStatus = NlpReadCacheEntryByIndex( Next->Index,
                                             CacheEntry,
                                             EntrySize
                                             );

        if (!NT_SUCCESS(NtStatus)) {
            break;  // out of while-loop
        }


        //
        // Grab the various strings from the cache entry.
        //
        ASSERT((*CacheEntry)->Revision >= NLP_CACHE_REVISION_NT_1_0B );

        //
        // decrypt the cache entry...
        //

        NtStatus = NlpDecryptCacheEntry( *CacheEntry, *EntrySize );

        if(!NT_SUCCESS(NtStatus)) {

            //
            // for failed decrypt, continue the search.
            // the reason for this is because the decrypt does an integrity
            // check.  We don't want one corrupt cache entry to cause (possibly)
            // the whole cache to be invalidated.
            //

            FreeToHeap( (*CacheEntry) );
            Next = (PNLP_CTE)(Next->Link.Flink);

            continue;
        }


        CachedUser.Length =
            CachedUser.MaximumLength = (*CacheEntry)->UserNameLength;
        if( (*CacheEntry)->Revision >= NLP_CACHE_REVISION_NT_5_0 ) {
            CachedUser.Buffer = (PWSTR) ((PBYTE) *CacheEntry + sizeof(LOGON_CACHE_ENTRY));
        } else {
            CachedUser.Buffer = (PWSTR) ((PBYTE) *CacheEntry + sizeof(LOGON_CACHE_ENTRY_NT_4_SP4));
        }

        CachedDomain.Length =
            CachedDomain.MaximumLength = (*CacheEntry)->DomainNameLength;
        CachedDomain.Buffer = (PWSTR)((LPBYTE)CachedUser.Buffer +
            ROUND_UP_COUNT((*CacheEntry)->UserNameLength, sizeof(ULONG)));

        CachedDnsDomain.Length =
            CachedDnsDomain.MaximumLength = (*CacheEntry)->DnsDomainNameLength;
        CachedDnsDomain.Buffer = (PWSTR)((LPBYTE)CachedDomain.Buffer +
                ROUND_UP_COUNT((*CacheEntry)->DomainNameLength, sizeof(ULONG)));

        CachedUpn.Length =
            CachedUpn.MaximumLength = (*CacheEntry)->UpnLength;
        CachedUpn.Buffer = (PWSTR)((LPBYTE)CachedDnsDomain.Buffer +
                ROUND_UP_COUNT((*CacheEntry)->DnsDomainNameLength, sizeof(ULONG)));


        //
        // If the caller passed in a domain name,
        //  the user name is the SamAccountName,
        //  and the domain name is either the Netbios or Dns Domain Name.
        //

        if ( DomainName->Length != 0 ) {
            if (RtlEqualUnicodeString(UserName, &CachedUser, (BOOLEAN) TRUE ) ) {


                if ( RtlEqualDomainName(DomainName, &CachedDomain ) ||
                     RtlEqualUnicodeString(DomainName, &CachedDnsDomain, (BOOLEAN) TRUE ) ) {


                    //
                    // found it !
                    //

                    break; // out of while-loop
                }

            }

        //
        // If no domain name was passed in,
        //  the user name is the UPN.
        //
        } else {
            if (RtlEqualUnicodeString(UserName, &CachedUpn, (BOOLEAN) TRUE ) ) {

                //
                // found it !
                //

                break; // out of while-loop

            }
        }

        //
        // Not the right entry, free the registry structure
        // and go on to the next one.
        //

        FreeToHeap( (*CacheEntry) );

        Next = (PNLP_CTE)(Next->Link.Flink);
    }

    if (Next != (PNLP_CTE)&NlpActiveCtes && NT_SUCCESS(NtStatus)) {

        //
        // We found a match - Open the corresponding secret
        //

        (*Index) = Next->Index;

        if( (*CacheEntry)->Revision < NLP_CACHE_REVISION_NT_5_0 ) {

            //
            // versions prior to NT5 require us open the corresponding LSA secret.
            //

            NtStatus = NlpOpenSecret(Next->Index);

            if (!NT_SUCCESS(NtStatus)) {
                FreeToHeap( (*CacheEntry) );
                return(NtStatus);
            }
        }

    } else {
        NtStatus = STATUS_LOGON_FAILURE;
    }

    return(NtStatus);
}


NTSTATUS
NlpWriteCacheEntry(
    IN  ULONG              Index,
    IN  PLOGON_CACHE_ENTRY Entry,
    IN  ULONG              EntrySize
    )

/*++

Routine Description:

    Writes a LOGON_CACHE_ENTRY to the registry cache.

    It is the caller's responsibility to place the corresponding
    CTE table entry in the correct active/inactive list.

Arguments:
    Index      - Index of entry to write out.

    Entry      - pointer to LOGON_CACHE_ENTRY to write to cache

    EntrySize   - size of this entry (in bytes (must be multiple of 4 thoough))


Return Value:

    NTSTATUS
        Success = STATUS_SUCCESS
                    The LOGON_CACHE_ENTRY is now in the registry (hopefully
                    on disk)

        Failure =

--*/

{
    NTSTATUS
        NtStatus;

    UNICODE_STRING
        ValueName;

    WCHAR
        NameBuffer[32];

    ValueName.MaximumLength = 32;
    ValueName.Length = 0;
    ValueName.Buffer = &NameBuffer[0];
    NlpMakeCacheEntryName( Index, &ValueName );

    NtStatus = NtSetValueKey(NlpCacheHandle,
                             &ValueName,
                             0,             // TitleIndex
                             REG_BINARY,    // Type
                             (PVOID)Entry,
                             EntrySize
                             );
    return(NtStatus);
}


VOID
NlpCopyAndUpdateAccountInfo(
    IN  USHORT Length,
    IN  PUNICODE_STRING pUnicodeString,
    IN OUT PUCHAR* pSource,
    IN OUT PUCHAR* pDest
    )

/*++

Routine Description:

    Updates a UNICODE_STRING structure and copies the associated buffer to
    *pDest, if Length is non-zero

Arguments:

    Length          - length of UNICODE_STRING.Buffer to copy
    pUnicodeString  - pointer to UNICODE_STRING structure to update
    pSource         - pointer to pointer to source WCHAR string
    pDest           - pointer to pointer to place to copy WCHAR string

Return Value:

    None.
    if string was copied, *Source and *Dest are updated to point to the next
    naturally aligned (DWORD) positions in the input and output buffers resp.

--*/

{
    PUCHAR  source = *pSource;
    PUCHAR  dest = *pDest;

    pUnicodeString->Length = Length;
    pUnicodeString->MaximumLength = Length;
    pUnicodeString->Buffer = (PWSTR)dest;
    if (Length) {
        RtlCopyMemory(dest, source, Length);
        *pSource = ROUND_UP_POINTER(source + Length, sizeof(ULONG));
        *pDest = ROUND_UP_POINTER(dest + Length, sizeof(ULONG));
    }
}


VOID
NlpSetTimeField(
    OUT POLD_LARGE_INTEGER pTimeField,
    IN  NLP_SET_TIME_HINT Hint
    )

/*++

Routine Description:

    Sets a LARGE_INTEGER time field to one of 3 values:
        NLP_BIG_TIME     = maximum positive large integer (0x7fffffffffffffff)
        NLP_SMALL_TIME   = smallest positive large integer (0)
        NLP_NOW_TIME     = current system time

Arguments:

    pTimeField  - pointer to LARGE_INTEGER structure to update
    Hint        - NLP_BIG_TIME, NLP_SMALL_TIME or NLP_NOW_TIME

Return Value:

    None.

--*/

{
    LARGE_INTEGER Time;

    switch (Hint) {
    case NLP_SMALL_TIME:
        pTimeField->HighPart = SMALL_PART_1;
        pTimeField->LowPart = SMALL_PART_2;
        break;

    case NLP_BIG_TIME:
        pTimeField->HighPart = BIG_PART_1;
        pTimeField->LowPart = BIG_PART_2;
        break;

    case NLP_NOW_TIME:
        NtQuerySystemTime(&Time);
        NEW_TO_OLD_LARGE_INTEGER( Time, (*pTimeField) );
        break;
    }
}


NTSTATUS
NlpBuildAccountInfo(
    IN  PLOGON_CACHE_ENTRY pCacheEntry,
    IN  ULONG EntryLength,
    OUT PNETLOGON_VALIDATION_SAM_INFO2* AccountInfo
    )

/*++

Routine Description:

    Performs the reverse of NlpBuildCacheEntry - creates a NETLOGON_VALIDATION_SAM_INFO2
    structure from a cache entry

Arguments:

    pCacheEntry - pointer to LOGON_CACHE_ENTRY

    EntryLength - inclusive size of *pCacheEntry, including variable data

    AccountInfo - pointer to place to create NETLOGON_VALIDATION_SAM_INFO2

Return Value:

    NTSTATUS
        Success = STATUS_SUCCESS

        Failure = STATUS_NO_MEMORY

--*/

{
    PNETLOGON_VALIDATION_SAM_INFO2 pSamInfo;
    PUCHAR source;
    PUCHAR dest;
    ULONG length;
    ULONG sidLength;
    ULONG commonBits;
    LPWSTR computerName;
    ULONG computerNameLength;


    //
    // will GetComputerName ever fail??? Its only used to fake the logon
    // server name when we logon using the cached information, so its
    // probably ok to use a NULL string if GetComputerName phones in sick
    //

    computerName = NlpComputerName.Buffer;
    computerNameLength = NlpComputerName.Length / sizeof(WCHAR);

    ASSERT( computerName );
    ASSERT( computerNameLength );

#if DBG
    if (DumpCacheInfo) {
        DbgPrint("ComputerName = %ws, length = %d\n", computerName, computerNameLength);
    }
#endif

    //
    // commonBits is the size of the variable data area common to both the
    // LOGON_CACHE_ENTRY and NETLOGON_VALIDATION_SAM_INFO2 structures
    //

    commonBits  = ROUND_UP_COUNT(pCacheEntry->EffectiveNameLength, sizeof(ULONG))
                + ROUND_UP_COUNT(pCacheEntry->FullNameLength, sizeof(ULONG))
                + ROUND_UP_COUNT(pCacheEntry->LogonScriptLength, sizeof(ULONG))
                + ROUND_UP_COUNT(pCacheEntry->ProfilePathLength, sizeof(ULONG))
                + ROUND_UP_COUNT(pCacheEntry->HomeDirectoryLength, sizeof(ULONG))
                + ROUND_UP_COUNT(pCacheEntry->HomeDirectoryDriveLength, sizeof(ULONG))
                + ROUND_UP_COUNT(pCacheEntry->GroupCount * sizeof(GROUP_MEMBERSHIP), sizeof(ULONG))
                + ROUND_UP_COUNT(pCacheEntry->LogonDomainNameLength, sizeof(ULONG))
                ;
    ASSERT(pCacheEntry->Revision >= NLP_CACHE_REVISION_NT_1_0B);

    commonBits += ROUND_UP_COUNT(sizeof(NETLOGON_SID_AND_ATTRIBUTES) * pCacheEntry->SidCount, sizeof(ULONG))
                + ROUND_UP_COUNT(pCacheEntry->SidLength, sizeof(ULONG))
                ;
    sidLength = pCacheEntry->LogonDomainIdLength;



    //
    // length is the required amount of buffer in which to build a working
    // NETLOGON_VALIDATION_SAM_INFO2 structure
    //

    length = ROUND_UP_COUNT(sizeof(NETLOGON_VALIDATION_SAM_INFO2), sizeof(ULONG))
                + commonBits
                + sidLength
                + computerNameLength * sizeof(WCHAR)
                ;

#if DBG
    if (DumpCacheInfo) {
        DbgPrint("NlpBuildAccountInfo: %d bytes required\n", length);
    }
#endif

    pSamInfo = (PNETLOGON_VALIDATION_SAM_INFO2)MIDL_user_allocate(length);
    if (pSamInfo == NULL) {
        return STATUS_NO_MEMORY;
    }

    //
    // point source at the first string to be copied out of the variable length
    // data area at the end of the cache entry
    //

    ASSERT(pCacheEntry->Revision >= NLP_CACHE_REVISION_NT_1_0B );

    if( pCacheEntry->Revision >= NLP_CACHE_REVISION_NT_5_0 ) {
        source = (PUCHAR)(pCacheEntry + 1);
    } else {
        source = (PUCHAR)( (PLOGON_CACHE_ENTRY_NT_4_SP4)pCacheEntry + 1 );
    }

    source +=   ROUND_UP_COUNT(pCacheEntry->UserNameLength, sizeof(ULONG))
              + ROUND_UP_COUNT(pCacheEntry->DomainNameLength, sizeof(ULONG))
              + ROUND_UP_COUNT(pCacheEntry->DnsDomainNameLength, sizeof(ULONG))
              + ROUND_UP_COUNT(pCacheEntry->UpnLength, sizeof(ULONG));


    //
    // point dest at the first (aligned) byte at the start of the variable data
    // area at the end of the sam info structure
    //

    dest = (PUCHAR)(pSamInfo + 1);

    //
    // pull out the variable length data from the end of the LOGON_CACHE_ENTRY
    // and stick them at the end of the NETLOGON_VALIDATION_SAM_INFO2 structure.
    // These must be copied out IN THE SAME ORDER as NlpBuildCacheEntry put them
    // in. If we want to change the order of things in the buffer, the order
    // must be changed in both routines (this & NlpBuildCacheEntry)
    //

    //
    // create the UNICODE_STRING structures in the NETLOGON_VALIDATION_SAM_INFO2
    // structure and copy the strings to the end of the buffer. 0 length strings
    // will get a pointer which should be ignored
    //

    NlpCopyAndUpdateAccountInfo(pCacheEntry->EffectiveNameLength,
                                &pSamInfo->EffectiveName,
                                &source,
                                &dest
                                );

    NlpCopyAndUpdateAccountInfo(pCacheEntry->FullNameLength,
                                &pSamInfo->FullName,
                                &source,
                                &dest
                                );

    NlpCopyAndUpdateAccountInfo(pCacheEntry->LogonScriptLength,
                                &pSamInfo->LogonScript,
                                &source,
                                &dest
                                );

    NlpCopyAndUpdateAccountInfo(pCacheEntry->ProfilePathLength,
                                &pSamInfo->ProfilePath,
                                &source,
                                &dest
                                );

    NlpCopyAndUpdateAccountInfo(pCacheEntry->HomeDirectoryLength,
                                &pSamInfo->HomeDirectory,
                                &source,
                                &dest
                                );

    NlpCopyAndUpdateAccountInfo(pCacheEntry->HomeDirectoryDriveLength,
                                &pSamInfo->HomeDirectoryDrive,
                                &source,
                                &dest
                                );

    //
    // copy the group membership array
    //

    pSamInfo->GroupIds = (PGROUP_MEMBERSHIP)dest;
    length = pCacheEntry->GroupCount * sizeof(GROUP_MEMBERSHIP);
    RtlCopyMemory(dest, source, length);
    dest = ROUND_UP_POINTER(dest + length, sizeof(ULONG));
    source = ROUND_UP_POINTER(source + length, sizeof(ULONG));

    //
    // final UNICODE_STRING from LOGON_CACHE_ENTRY. Reorganize this to:
    // strings, groups, SID?
    //

    NlpCopyAndUpdateAccountInfo(pCacheEntry->LogonDomainNameLength,
                                &pSamInfo->LogonDomainName,
                                &source,
                                &dest
                                );


    //
    // Copy all the SIDs
    //

    if (pCacheEntry->Revision >= NLP_CACHE_REVISION_NT_1_0B ) {
        pSamInfo->SidCount = pCacheEntry->SidCount;

        if (pCacheEntry->SidCount) {
            ULONG i, sidLength;
            PULONG SidAttributes = (PULONG) source;
            source = ROUND_UP_POINTER(source + pCacheEntry->SidCount * sizeof(ULONG), sizeof(ULONG));

            pSamInfo->ExtraSids = (PNETLOGON_SID_AND_ATTRIBUTES) dest;
            dest = ROUND_UP_POINTER(dest + pCacheEntry->SidCount * sizeof(NETLOGON_SID_AND_ATTRIBUTES), sizeof(ULONG));

            for (i = 0; i < pCacheEntry->SidCount ; i++ ) {
                pSamInfo->ExtraSids[i].Attributes = SidAttributes[i];
                sidLength = RtlLengthSid((PSID) source);
                RtlCopySid(sidLength, (PSID) dest, (PSID) source);
                pSamInfo->ExtraSids[i].Sid = (PSID) dest;
                dest = ROUND_UP_POINTER(dest + sidLength, sizeof(ULONG));
                source = ROUND_UP_POINTER(source + sidLength, sizeof(ULONG));
            }

            ASSERT((ULONG) (source - (PCHAR) SidAttributes) == pCacheEntry->SidLength);

        } else {
            pSamInfo->ExtraSids = NULL;
        }
    } else {
        pSamInfo->ExtraSids = NULL;
        pSamInfo->SidCount = 0;
    }


    //
    // copy the LogonDomainId SID
    //

    RtlCopySid(sidLength, (PSID)dest, (PSID)source);
    pSamInfo->LogonDomainId = (PSID)dest;
    dest = ROUND_UP_POINTER(dest + sidLength, sizeof(ULONG));

    //
    // copy the non-variable fields
    //

    pSamInfo->UserId = pCacheEntry->UserId;
    pSamInfo->PrimaryGroupId = pCacheEntry->PrimaryGroupId;
    pSamInfo->GroupCount = pCacheEntry->GroupCount;

    //
    // finally, invent some fields
    //

    NlpSetTimeField(&pSamInfo->LogonTime, NLP_NOW_TIME);
    NlpSetTimeField(&pSamInfo->LogoffTime, NLP_BIG_TIME);
    NlpSetTimeField(&pSamInfo->KickOffTime, NLP_BIG_TIME);
    NlpSetTimeField(&pSamInfo->PasswordLastSet, NLP_SMALL_TIME);
    NlpSetTimeField(&pSamInfo->PasswordCanChange, NLP_BIG_TIME);
    NlpSetTimeField(&pSamInfo->PasswordMustChange, NLP_BIG_TIME);

    pSamInfo->LogonCount = 0;
    pSamInfo->BadPasswordCount = 0;
    pSamInfo->UserFlags = LOGON_EXTRA_SIDS;
    if (pCacheEntry->LogonPackage != 0) {
        pSamInfo->UserFlags |= pCacheEntry->LogonPackage << PRIMARY_CRED_LOGON_PACKAGE_SHIFT;
    }

    RtlZeroMemory(&pSamInfo->UserSessionKey, sizeof(pSamInfo->UserSessionKey));

    //
    // final UNICODE_STRING. This one from stack. Note that we have finished
    // with source
    //

    source = (PUCHAR)computerName;
    NlpCopyAndUpdateAccountInfo((USHORT)(computerNameLength * sizeof(WCHAR)),
                                &pSamInfo->LogonServer,
                                &source,
                                &dest
                                );

#if DBG
    if (DumpCacheInfo) {
        DbgPrint("NlpBuildAccountInfo:\n");
        DumpAccountInfo(pSamInfo);
    }
#endif

    *AccountInfo = pSamInfo;
    return STATUS_SUCCESS;
    UNREFERENCED_PARAMETER( EntryLength );
}


NTSTATUS
NlpGetCacheControlInfo( VOID )

/*++

Routine Description:

    This function retrieves the cache control information from the
    registry.  This information is placed in global data for use
    throughout this module.  The Cache Table Entry table will also
    be initialized.

    If this routine returns success, then it may be assumed that
    everything completed successfully.

Arguments:

    None.

Return Value:



--*/

{
    NTSTATUS
        NtStatus;

    UNICODE_STRING
        CacheControlValueName;

    ULONG
        RequiredSize;

    PKEY_VALUE_PARTIAL_INFORMATION
        RegInfo = NULL;


    //
    // read the current control info, if it is there.
    // If it is not there, then we may be dealing with a down-level
    // system and might have a single cache entry in the registry.
    //

    RtlInitUnicodeString( &CacheControlValueName, L"NL$Control" );
    NtStatus = NtQueryValueKey(NlpCacheHandle,
                               &CacheControlValueName,
                               KeyValuePartialInformation,
                               NULL,
                               0,
                               &RequiredSize
                               );

    if (NT_SUCCESS(NtStatus) || NtStatus == STATUS_OBJECT_NAME_NOT_FOUND) {
        NTSTATUS TempStatus;

        //
        // Hmmm - no entry, that means we are dealing with a
        //        first release system here (that didn't have
        //        this value).
        //


        //
        // Set up for 1 cache entry.
        // create the secret and cache key entry
        //

        TempStatus = NlpMakeNewCacheEntry( 0 );

        if ( NT_SUCCESS(TempStatus) ) {
            //
            // Now flush out the control information
            //


            NlpCacheControl.Revision = NLP_CACHE_REVISION;
            NlpCacheControl.Entries  = 1;
            TempStatus = NlpWriteCacheControl();

            if ( NT_SUCCESS(TempStatus) ) {

                //
                // If a version 1.0 entry exists,
                //  copy the old form of cache entry to the new structure.
                //

                if (NT_SUCCESS(NtStatus)) {
                    TempStatus = NlpConvert1_0To1_0B();
                }
            }
        }

        NtStatus = TempStatus;

    } else if ( NtStatus == STATUS_BUFFER_TOO_SMALL ) {

        //
        // allocate buffer then do query again, this time receiving data
        //

        RegInfo = (PKEY_VALUE_PARTIAL_INFORMATION)AllocateFromHeap(RequiredSize);
        if (RegInfo == NULL) {
            NtStatus = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        NtStatus = NtQueryValueKey(NlpCacheHandle,
                                   &CacheControlValueName,
                                   KeyValuePartialInformation,
                                   (PVOID)RegInfo,
                                   RequiredSize,
                                   &RequiredSize
                                   );

        if (!NT_SUCCESS(NtStatus)) {
            goto Cleanup;
        }

        //
        // check the revision - we can't deal with up-level revisions.
        //

        if (RegInfo->DataLength < sizeof(NLP_CACHE_CONTROL)) {
            NtStatus = STATUS_UNKNOWN_REVISION;
            goto Cleanup;
        }

        RtlCopyMemory( &NlpCacheControl, &(RegInfo->Data[0]), sizeof(NLP_CACHE_CONTROL) );
        if (NlpCacheControl.Revision > NLP_CACHE_REVISION) {
            NtStatus = STATUS_UNKNOWN_REVISION;
            goto Cleanup;
        }


        //
        // If this is an older cache, update it with the latest revision
        //

        if (NlpCacheControl.Revision != NLP_CACHE_REVISION) {

            // There is no conversion. All the version of cache control have
            //  been the same.
            NlpCacheControl.Revision = NLP_CACHE_REVISION;
            NtStatus = NlpWriteCacheControl();

            if (!NT_SUCCESS(NtStatus)) {
                goto Cleanup;
            }
        }

        NtStatus = STATUS_SUCCESS;
    }

Cleanup:

    if (!NT_SUCCESS(NtStatus)) {
        NlpCacheControl.Entries = 0;    // Disable logon cache
    }

    if( RegInfo ) {
        FreeToHeap( RegInfo );
    }

    return(NtStatus);
}


NTSTATUS
NlpBuildCteTable( VOID )

/*++

Routine Description:

    This function initializes the CTE table from the contents of
    the cache in the registry.


Arguments:

    None.

Return Value:

    STATUS_SUCCESS - the cache is initialized.

    Other - The cache has been disabled.

--*/

{
    NTSTATUS
        NtStatus = STATUS_SUCCESS;

    PLOGON_CACHE_ENTRY
        CacheEntry;

    ULONG
        EntrySize,
        i;


    //
    // Initialize the active and inactive CTE lists
    //

    InitializeListHead( &NlpActiveCtes );
    InitializeListHead( &NlpInactiveCtes );


    //
    // Allocate a CTE table
    //

    NlpCteTable = AllocateFromHeap( sizeof( NLP_CTE ) *
                                    NlpCacheControl.Entries );
    if (NlpCteTable == NULL) {

        //
        // Can't allocate table, disable caching
        //

        NlpCacheControl.Entries = 0;    // Disable cache
        return(STATUS_NO_MEMORY);
    }

    for (i=0; i<NlpCacheControl.Entries; i++) {

        NtStatus = NlpReadCacheEntryByIndex( i,
                                             &CacheEntry,
                                             &EntrySize);
        if (!NT_SUCCESS(NtStatus) ) {
            NlpCacheControl.Entries = 0;    // Disable cache
            return(NtStatus);
        }

        //
        //
        if (EntrySize < sizeof(LOGON_CACHE_ENTRY_NT_4_SP4)) {

            //
            // Hmmm, something is bad.
            // disable caching and return an error
            //

            NlpCacheControl.Entries = 0;    // Disable cache
            FreeToHeap( CacheEntry );
            return( STATUS_INTERNAL_DB_CORRUPTION );
        }

        if (CacheEntry->Revision > NLP_CACHE_REVISION) {
            NlpCacheControl.Entries = 0;  // Disable cache
            FreeToHeap( CacheEntry );
            return(STATUS_UNKNOWN_REVISION);
        }

        NlpCteTable[i].Index  = i;
        NlpCteTable[i].Active = CacheEntry->Valid;
        NlpCteTable[i].Time   = CacheEntry->Time;

        InsertTailList( &NlpInactiveCtes, &NlpCteTable[i].Link );

        if (NlpCteTable[i].Active) {
            NlpAddEntryToActiveList( i );
        }

        FreeToHeap( CacheEntry );

    }
    return(NtStatus);
}


NTSTATUS
NlpChangeCacheSizeIfNecessary( VOID )

/*++

Routine Description:

    This function checks to see if the user has requested a
    different cache size than what we currently have.

    If so, then we try to grow or shrink our cache appropriately.
    If this succeeds, then the global cache control information is
    updated appropriately.  If it fails then one of two things will
    happen:

        1) If the user was trying to shrink the cache, then it will
           be disabled (entries set to zero).

        2) If the user was trying to grow the cache, then we will leave
           it as it is.

    In either of these two failure conditions, an error is returned.


Arguments:

    None.

Return Value:

    STATUS_SUCCESS

--*/

{

    NTSTATUS
        NtStatus;

    UINT
        CachedLogonsCount;

    PNLP_CTE
        NewCteTable,
        Next;

    LIST_ENTRY
        NewActive,
        NewInactive;

    PNLP_CACHE_AND_SECRETS
        CacheAndSecrets;


    ULONG
        ErrorCacheSize,
        EntrySize,
        i,
        j;


    // Find out how many logons to cache.
    // This is a user setable value and it may be different than
    // the last time we booted.
    //

    CachedLogonsCount = GetProfileInt(
                               TEXT("Winlogon"),
                               TEXT("CachedLogonsCount"),
                               NLP_DEFAULT_LOGON_CACHE_COUNT      // Default value
                               );

    //
    // Minimize the user-supplied value with the maximum allowable
    // value.
    //

    if (CachedLogonsCount > NLP_MAX_LOGON_CACHE_COUNT) {
        CachedLogonsCount = NLP_MAX_LOGON_CACHE_COUNT;
    }


    //
    // Compare it to what we already have and see if we need
    // to change the size of the cache
    //

    if (CachedLogonsCount == NlpCacheControl.Entries) {

        //
        // No change
        //

        return(STATUS_SUCCESS);
    }

    //
    // Set the size of the cache to be used in case of error
    // changing the size.  If we are trying to grow the cache,
    // then use the existing cache on error.  If we are trying
    // to shrink the cache, then disable caching on error.
    //

    if (CachedLogonsCount > NlpCacheControl.Entries) {
        ErrorCacheSize = NlpCacheControl.Entries;
    } else {
        ErrorCacheSize = 0;
    }

    //
    // Allocate a CTE table the size of the new table
    //

    NewCteTable = AllocateFromHeap( sizeof( NLP_CTE ) *
                                    CachedLogonsCount );
    if (NewCteTable == NULL) {

        //
        // Can't shrink table, disable caching
        //

        NlpCacheControl.Entries = ErrorCacheSize;
        return(STATUS_NO_MEMORY);
    }



    //
    // Now the tricky parts ...
    //

    if (CachedLogonsCount > NlpCacheControl.Entries) {


        //
        // Try to grow the cache -
        // Create additional secrets and cache entries.
        //
        // Copy time fields and set index
        //

        for (i=0;   i < NlpCacheControl.Entries;   i++) {
            NewCteTable[i].Index = i;
            NewCteTable[i].Time  = NlpCteTable[i].Time;
        }

        //
        // Place existing entries on either the active or inactive list
        //

        InitializeListHead( &NewActive );
        for (Next  = (PNLP_CTE)NlpActiveCtes.Flink;
             Next != (PNLP_CTE)(&NlpActiveCtes);
             Next  = (PNLP_CTE)Next->Link.Flink
             ) {

            InsertTailList( &NewActive, &NewCteTable[Next->Index].Link );
            NewCteTable[Next->Index].Active = TRUE;
        }


        InitializeListHead( &NewInactive );
        for (Next  = (PNLP_CTE)NlpInactiveCtes.Flink;
             Next != (PNLP_CTE)(&NlpInactiveCtes);
             Next  = (PNLP_CTE)Next->Link.Flink
             ) {

            InsertTailList( &NewInactive, &NewCteTable[Next->Index].Link );
            NewCteTable[Next->Index].Active = FALSE;
        }


        //
        // Make all the new table entries.
        // Mark them as invalid.
        //

        for (i=NlpCacheControl.Entries; i<CachedLogonsCount; i++) {

            //
            // Add the CTE entry to the inactive list
            //

            InsertTailList( &NewInactive, &NewCteTable[i].Link );
            NewCteTable[i].Active = FALSE;
            NewCteTable[i].Index  = i;

            NtStatus = NlpMakeNewCacheEntry( i );

            if (!NT_SUCCESS(NtStatus)) {
                FreeToHeap( NewCteTable );
                return(NtStatus);
            }
        }




    } else {


        //
        // Try to shrink the cache.
        //

        if (CachedLogonsCount != 0) {

            //
            // 0 size implies disabling the cache.
            // That is a degenerate case of shrinking that
            // requires only the last few steps of shrinking.
            //


            //
            // Allocate an array of pointers for reading registry and secret
            // info into.  Clear it to assist in cleanup.
            //

            CacheAndSecrets = (PNLP_CACHE_AND_SECRETS)
                              AllocateFromHeap( sizeof( NLP_CACHE_AND_SECRETS ) *
                                                CachedLogonsCount );

            if (CacheAndSecrets == NULL) {
                FreeToHeap( NlpCteTable );
                NlpCacheControl.Entries = ErrorCacheSize;
                return(STATUS_NO_MEMORY);
            }
            RtlZeroMemory( CacheAndSecrets,
                           (sizeof( NLP_CACHE_AND_SECRETS ) * CachedLogonsCount) );


            //
            // Set up the new CTE table to be inactive
            //

            InitializeListHead( &NewActive );
            InitializeListHead( &NewInactive );
            for (i=0; i<CachedLogonsCount; i++) {
                InsertTailList( &NewInactive, &NewCteTable[i].Link );
                NewCteTable[i].Index  = i;
                NewCteTable[i].Active = FALSE;
            }


            //
            // Walk the current active list, reading
            // entries and copying information into the new CTE table.
            //

            i = 0;
            Next = (PNLP_CTE)NlpActiveCtes.Flink;
            while (Next != (PNLP_CTE)&NlpActiveCtes && i<CachedLogonsCount) {

                NtStatus = NlpReadCacheEntryByIndex( Next->Index,
                                                     &CacheAndSecrets[i].CacheEntry,
                                                     &CacheAndSecrets[i].EntrySize
                                                     // &EntrySize
                                                     );
                if (NT_SUCCESS(NtStatus)) {

                    //
                    // for pre-Win2000 cache entries, read the associated secret.
                    //

                    if( CacheAndSecrets[i].CacheEntry->Revision < NLP_CACHE_REVISION_NT_5_0 ) {

                        NtStatus = NlpOpenSecret( Next->Index );
                        if (NT_SUCCESS(NtStatus)) {
                            NtStatus = NlpReadSecret( &CacheAndSecrets[i].NewSecret,
                                                      &CacheAndSecrets[i].OldSecret);

                            NlpCloseSecret();
                        }
                    }

                    if (NT_SUCCESS(NtStatus)) {
                        //
                        // Only make this entry active if everything was
                        // successfully read in.
                        //

                        CacheAndSecrets[i].Active = TRUE;
                        i++;    // advance our new CTE table index

                    }
                }

                Next = (PNLP_CTE)(Next->Link.Flink);

            } // end-while

            //
            // At this point "i" indicates how many CacheAndSecrets entries
            // are active.  Furthermore, the entries were assembled
            // in the CacheAndSecrets array in ascending time order, which
            // is the order they need to be placed in the new CTE table.
            //

            for ( j=0; j<i; j++) {

                Next = &NewCteTable[j];

                //
                // The Time field in the original cache entry is not aligned
                // properly, so copy each field individually.
                //

                Next->Time.LowPart = CacheAndSecrets[j].CacheEntry->Time.LowPart;
                Next->Time.HighPart = CacheAndSecrets[j].CacheEntry->Time.HighPart;

                //
                // Try writing out the new entry's information
                //

                NtStatus = NlpWriteCacheEntry( j,
                                               CacheAndSecrets[j].CacheEntry,
                                               CacheAndSecrets[j].EntrySize
                                               );
                if (NT_SUCCESS(NtStatus)) {

                    if( CacheAndSecrets[j].CacheEntry->Revision < NLP_CACHE_REVISION_NT_5_0 ) {

                        //
                        // for pre-Win2000 cache entries, write the secret back out.
                        // note: we don't bother to try to migrate pre-win2000 -> Win2000
                        // here, because this will happen later, as a side-effect
                        // of updating cache entry during successful DC validated logon.
                        //

                        NtStatus = NlpOpenSecret( j );

                        if (NT_SUCCESS(NtStatus)) {
                            NtStatus = NlpWriteSecret(CacheAndSecrets[j].NewSecret,
                                                      CacheAndSecrets[j].OldSecret);

                        }
                    }

                    if (NT_SUCCESS(NtStatus)) {

                        //
                        // move the corresponding entry into the new CTEs
                        // active list.
                        //

                        Next->Active = TRUE;
                        RemoveEntryList( &Next->Link );
                        InsertTailList( &NewActive, &Next->Link );
                    }
                }

                //
                // Free the CacheEntry and secret information
                //

                if (CacheAndSecrets[j].CacheEntry != NULL) {
                    FreeToHeap( CacheAndSecrets[j].CacheEntry );
                }
                if (CacheAndSecrets[j].NewSecret != NULL) {
                    MIDL_user_free( CacheAndSecrets[j].NewSecret );
                }
                if (CacheAndSecrets[j].OldSecret != NULL) {
                    MIDL_user_free( CacheAndSecrets[j].OldSecret );
                }
            }

            //
            // Free the CacheAndSecrets array
            // (everything in it has already been freed)
            //

            if (CacheAndSecrets != NULL) {
                FreeToHeap( CacheAndSecrets );
            }

            //
            // Change remaining entries to invalid (on disk)
            //

            for ( j=i; j<CachedLogonsCount; j++) {
                NlpMakeNewCacheEntry( j );
            }

        } // end-if (CachedLogonsCount != 0)


        //
        // Now get rid of extra (no longer needed) entries
        //

        for ( j=CachedLogonsCount; j<NlpCacheControl.Entries; j++) {
            NlpEliminateCacheEntry( j );
        }


    }

    //
    // We have successfully:
    //
    //      Allocated the new CTE table.
    //
    //      Filled the CTE table with copies of the currently
    //      active CTEs (including putting each CTE on an active
    //      or inactive list).
    //
    //      Established new CTE entries, including the corresponding
    //      secrets and cache keys in the registry, for the
    //      new CTEs.
    //
    //
    // All we have left to do is:
    //
    //
    //      Update the cache control structure in the registry
    //      to indicate we have a new length
    //
    //      move the new CTE over to the real Active and Inactive
    //      list heads (rather than the local ones we've used so far)
    //
    //      deallocate the old CTE table.
    //
    //      Re-set the entries count in the in-memory
    //      cache-control structure NlpCacheControl.
    //


    NlpCacheControl.Entries = CachedLogonsCount;
    NtStatus = NlpWriteCacheControl();

    if (CachedLogonsCount > 0) {  // Only necessary if there is a new CTE table
        if (!NT_SUCCESS(NtStatus)) {
            FreeToHeap( NewCteTable );
            NlpCacheControl.Entries = ErrorCacheSize;
            return(NtStatus);
        }

        InsertHeadList( &NewActive, &NlpActiveCtes );
        RemoveEntryList( &NewActive );
        InsertHeadList( &NewInactive, &NlpInactiveCtes );
        RemoveEntryList( &NewInactive );

        FreeToHeap( NlpCteTable );
        NlpCteTable = NewCteTable;
    }

    return(NtStatus);

}



NTSTATUS
NlpWriteCacheControl( VOID )

/*++

Routine Description:

    This function writes a new cache length out to the
    cache control structure stored in the registry.

    Note:
        When lengthening the cache, call this routine after the cache
        entries and corresponding secrets have been established for
        the new length.

        When shortening the cache, call this routine before the cache
        entries and corresponding secrets being discarded have actually
        been discarded.

        This ensures that if the system crashes during the resizing
        operation, it will be in a valid state when the system comes
        back up.


Arguments:

    None.

Return Value:

    STATUS_SUCCESS

--*/

{

    NTSTATUS
        NtStatus;

    UNICODE_STRING
        CacheControlValueName;


    RtlInitUnicodeString( &CacheControlValueName, L"NL$Control" );
    NtStatus = NtSetValueKey( NlpCacheHandle,
                              &CacheControlValueName,       // Name
                              0,                            // TitleIndex
                              REG_BINARY,                   // Type
                              &NlpCacheControl,             // Data
                              sizeof(NLP_CACHE_CONTROL)    // DataLength
                              );
    return(NtStatus);
}


VOID
NlpMakeCacheEntryName(
    IN  ULONG               EntryIndex,
    OUT PUNICODE_STRING     Name
    )

/*++

Routine Description:

    This function builds a name of a cache entry value or secret name
    for a cached entry.  The name is based upon the index of the cache
    entry.

    Names are of the form:

            "NLP1" through "NLPnnn"

    where "nnn" is the largest allowable entry count (see
    NLP_MAX_LOGON_CACHE_COUNT).

    The output UNICODE_STRING buffer is expected to be large enough
    to accept this string with a null termination on it.


Arguments:

    EntryIndex - The index of the cache entry whose name is desired.

    Name - A unicode string large enough to accept the name.


Return Value:

    STATUS_SUCCESS

--*/

{
    NTSTATUS
        NtStatus;

    UNICODE_STRING
        TmpString;

    WCHAR
        TmpStringBuffer[17];

    ASSERT(Name->MaximumLength >= 7*sizeof(WCHAR) );
    ASSERT( EntryIndex <= NLP_MAX_LOGON_CACHE_COUNT );

    Name->Length = 0;
    RtlAppendUnicodeToString( Name, L"NL$" );

    TmpString.MaximumLength = 16;
    TmpString.Length = 0;
    TmpString.Buffer = TmpStringBuffer;
    NtStatus = RtlIntegerToUnicodeString ( (EntryIndex+1),      // make 1 based index
                                           10,           // Base 10
                                           &TmpString
                                           );
    ASSERT(NT_SUCCESS(NtStatus));

    RtlAppendUnicodeStringToString( Name, &TmpString );


    return;
}


NTSTATUS
NlpMakeNewCacheEntry(
    ULONG           Index
    )

/*++

Routine Description:

    This routine creates a secret and a cache entry value for a
    new cache entry with the specified index.

    The secret handle is NOT left open.


Arguments:

    Index - The index of the cache entry whose name is desired.

    Name - A unicode string large enough to accept the name.


Return Value:

    STATUS_SUCCESS

--*/

{
    NTSTATUS
        NtStatus;

    LOGON_CACHE_ENTRY
        Entry;

    UNICODE_STRING
        ValueName;

    WCHAR
        NameBuffer[32];

    LSAPR_HANDLE
        SecretHandle;

    ValueName.Length = 0;
    ValueName.MaximumLength = 32;
    ValueName.Buffer = &NameBuffer[0];

    NlpMakeCacheEntryName( Index, &ValueName );

    NtStatus = I_LsarOpenSecret( NlpLsaHandle,
                                    (PLSAPR_UNICODE_STRING) &ValueName,
                                    DELETE,
                                    &SecretHandle
                                    );

    if( NT_SUCCESS( NtStatus ) ) {

        //
        // for Windows2000, we remove old style cache entry related
        // LSA secrets.
        //


        //
        // Deleting and object causes its handle to be closed
        //

        I_LsarDelete( SecretHandle );

//        I_LsarClose( &SecretHandle );
    }


    //
    // Create the cache entry marked as invalid
    //

    RtlZeroMemory( &Entry, sizeof(Entry) );
    Entry.Revision = NLP_CACHE_REVISION;
    Entry.Valid = FALSE;

    NtStatus = NtSetValueKey( NlpCacheHandle,
                              &ValueName,                   // Name
                              0,                            // TitleIndex
                              REG_BINARY,                   // Type
                              &Entry,                       // Data
                              sizeof(LOGON_CACHE_ENTRY)     // DataLength
                              );

    return(NtStatus);
}


NTSTATUS
NlpEliminateCacheEntry(
    IN  ULONG               Index
    )

/*++

Routine Description:

    Delete the registry value and secret object related to a
    CTE entry.

Arguments:

    Index - The index of the entry whose value and secret are to
        be deleted.  This value is used only to build a name with
        (not to reference the CTE table).


Return Value:


--*/

{
    NTSTATUS
        NtStatus;

    UNICODE_STRING
        ValueName;

    WCHAR
        NameBuffer[32];

    LSAPR_HANDLE
        SecretHandle;


    ValueName.Buffer = &NameBuffer[0];
    ValueName.MaximumLength = 32;
    ValueName.Length = 0;
    NlpMakeCacheEntryName( Index, &ValueName );

    NtStatus = I_LsarOpenSecret(NlpLsaHandle,
                             (PLSAPR_UNICODE_STRING) &ValueName,
                             DELETE,
                             &SecretHandle
                             );

    if (NT_SUCCESS(NtStatus)) {

        //
        // Deleting and object causes its handle to be closed
        //

        NtStatus = I_LsarDelete( SecretHandle );


        //
        // Now delete the registry value
        //

        NtStatus = NtDeleteValueKey( NlpCacheHandle, &ValueName );
    }

    return(NtStatus);
}


NTSTATUS
NlpConvert1_0To1_0B( VOID )

/*++

Routine Description:

    This function retrieves the cache entry used in NT1.0 systems
    and stores it (if found) in the zero'th CTE entry.  It also
    copies the secrets from 1.0 storage format to 1.0B format.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS - if the entry was successfully upgraded, or if
        it didn't exist.

    STATUS_NO_MEMORY - if we couldn't allocate memory from heap.

    other  - unexpected error.

--*/

{
    NTSTATUS
        NtStatus;

    PKEY_VALUE_FULL_INFORMATION
        RegistryStructure;

    PLOGON_CACHE_ENTRY_1_0
        CacheEntry;

    PLOGON_CACHE_ENTRY
        NewCacheEntry;

    UNICODE_STRING
        NullName;

    PLSAPR_CR_CIPHER_VALUE
        CurrentSecret = NULL,
        OldSecret = NULL;

    ULONG
        RequiredSize,
        VariableSize,
        EntrySize,
        NewSize;


     PUCHAR
        Source,
        Dest;



    //
    // This should always try to return at least
    // the KEY_VALUE_FULL_INFORMATION structure, even
    // if there isn't data available.
    //

    RtlInitUnicodeString(&NullName, NULL);
    NtStatus = NtQueryValueKey(NlpCacheHandle,
                               &NullName,
                               KeyValueFullInformation,
                               NULL,
                               0,
                               &RequiredSize
                               );
    ASSERT(!NT_SUCCESS(NtStatus));

    if (NtStatus != STATUS_BUFFER_TOO_SMALL) {
        return(NtStatus);
    }

    RegistryStructure = AllocateFromHeap( RequiredSize );
    if (RegistryStructure == NULL) {
        NtStatus = STATUS_NO_MEMORY;
    } else {


        NtStatus = NtQueryValueKey(NlpCacheHandle,
                                   &NullName,
                                   KeyValueFullInformation,
                                   RegistryStructure,
                                   RequiredSize,
                                   &RequiredSize
                                   );
        if (NT_SUCCESS(NtStatus)) {

            //
            // If we didn't get any data in the query, then there
            // wasn't a cache entry, don't do anything.  Otherwise,
            // copy it to the new scheme.
            //

            if (RequiredSize > sizeof(KEY_VALUE_FULL_INFORMATION)) {



                //
                // OK, we now have a NT1_0 cache entry.
                // This is the same as a NT1_0B cache entry, except that
                // the fields from SidCount onward are new to NT1_0B and
                // so aren't present in the structure we just read in.
                //
                // Now the challange is to build a new Registry Structure
                // that looks just like the one we just read in, but adds
                // in these new fields.
                //
                // Warning - the fields of CacheEntry are not necessarily
                //           aligned nicely because the structure starts
                //           at a random offset inside a registry header.
                //           Avoid referencing CacheEntry fields.
                //

                CacheEntry = (PLOGON_CACHE_ENTRY_1_0)
                             ((PCHAR)(RegistryStructure) +
                                     (RegistryStructure->DataOffset));
                EntrySize  = RegistryStructure->DataLength;

                Source = (PUCHAR)(((PLOGON_CACHE_ENTRY_1_0)CacheEntry) + 1);


                VariableSize = EntrySize -
                               ROUND_UP_COUNT(sizeof(LOGON_CACHE_ENTRY_1_0), sizeof(ULONG));

                NewSize =
                    ROUND_UP_COUNT(sizeof(LOGON_CACHE_ENTRY), sizeof(ULONG)) +
                    VariableSize;

                NewCacheEntry = (PLOGON_CACHE_ENTRY)AllocateFromHeap(NewSize);
                if (NewCacheEntry == NULL) {
                    NtStatus = STATUS_NO_MEMORY;
                } else {

                    RtlZeroMemory( NewCacheEntry, NewSize );

                    //
                    // Copy the fixed-length aspects of the original
                    // CacheEntry into the new cache entry.
                    //

                    RtlMoveMemory( NewCacheEntry,
                                   CacheEntry,
                                   sizeof(LOGON_CACHE_ENTRY_1_0) );


                    //
                    // We have to figure out the length of the LogonDomainId
                    //
                    {
                        ULONG commonBits, sidLength;

                        commonBits  =
                            ROUND_UP_COUNT(NewCacheEntry->EffectiveNameLength, sizeof(ULONG))
                          + ROUND_UP_COUNT(NewCacheEntry->FullNameLength, sizeof(ULONG))
                          + ROUND_UP_COUNT(NewCacheEntry->LogonScriptLength, sizeof(ULONG))
                          + ROUND_UP_COUNT(NewCacheEntry->ProfilePathLength, sizeof(ULONG))
                          + ROUND_UP_COUNT(NewCacheEntry->HomeDirectoryLength, sizeof(ULONG))
                          + ROUND_UP_COUNT(NewCacheEntry->HomeDirectoryDriveLength, sizeof(ULONG))
                          + ROUND_UP_COUNT(NewCacheEntry->GroupCount * sizeof(GROUP_MEMBERSHIP), sizeof(ULONG))
                          + ROUND_UP_COUNT(NewCacheEntry->LogonDomainNameLength, sizeof(ULONG)
                        );

                        //
                        // sidLength is the size of the SID copied to the LOGON_CACHE_ENTRY structure
                        //

                        sidLength = EntrySize - (sizeof(LOGON_CACHE_ENTRY_1_0)
                            + ROUND_UP_COUNT(NewCacheEntry->UserNameLength, sizeof(ULONG))
                            + ROUND_UP_COUNT(NewCacheEntry->DomainNameLength, sizeof(ULONG))
                            + commonBits
                            );

                            NewCacheEntry->LogonDomainIdLength = (USHORT) sidLength;
                    }


                    NtQuerySystemTime( &NewCacheEntry->Time );
                    NewCacheEntry->Revision = NLP_CACHE_REVISION;
                    NewCacheEntry->Valid    = TRUE;

                    Dest = (PUCHAR)(((PLOGON_CACHE_ENTRY)NewCacheEntry) + 1);
                    RtlMoveMemory( Dest, Source, VariableSize );



                    //
                    // put out the secrets first so that if it fails
                    // we haven't already validated the cache entry.
                    // This is done by tricking the secret routines.
                    //


                    NtStatus = NlpOpen_Nt1_0_Secret();

                    if (NT_SUCCESS(NtStatus)) {

                        NtStatus = NlpReadSecret(&CurrentSecret, &OldSecret);
                        if (NT_SUCCESS(NtStatus)) {

                            //
                            // Write out the secrets in the zero'th entry
                            //

                            NtStatus = NlpOpenSecret( 0 );

                            if (NT_SUCCESS(NtStatus)) {

                                NtStatus = NlpWriteSecret( CurrentSecret, OldSecret);

                                if (NT_SUCCESS(NtStatus)) {

                                    NtStatus = NlpWriteCacheEntry(
                                                   0,
                                                   NewCacheEntry,
                                                   NewSize
                                                   );
                                    //
                                    // The CTE table isn't built
                                    // yet, so don't try to update
                                    // this entry in the CTE.
                                }

                            }

                            //
                            // Free the secret buffers
                            //

                            if (CurrentSecret) {
                                MIDL_user_free(CurrentSecret);
                            }
                            if (OldSecret) {
                                MIDL_user_free(OldSecret);
                            }
                        }

                        NlpCloseSecret();
                    }

                    FreeToHeap( NewCacheEntry );
                }
            }
        }

        FreeToHeap( RegistryStructure );
    }

    return NtStatus ;
}


NTSTATUS
NlpOpen_Nt1_0_Secret( VOID )

/*++

Routine Description:

    Opens the secret object for the cache entry of a NT1.0 system.
    This is used to upgrade the system.  The secret is opened for
    query access.

    If the secret does not exist, it is NOT created.


Arguments:

    None.

Return Value:

    STATUS_SUCCESS - The secret was successfully openned.

    STATUS_OBJECT_NAME_NOT_FOUND - The secret didn't exist.  The handle
        value is not valid.

    Other error - the handle value is invalid.

--*/

{
    NTSTATUS
        NtStatus;

    UNICODE_STRING
        SecretName;


    //
    // Close previous handle if necessary
    //

    if (IS_VALID_HANDLE(NlpSecretHandle)) {
        I_LsarClose( &NlpSecretHandle );
    }

    SecretName.Length = SecretName.MaximumLength = SECRET_NAME_SIZE;
    SecretName.Buffer = SECRET_NAME;
    NtStatus = I_LsarOpenSecret(NlpLsaHandle,
                             (PLSAPR_UNICODE_STRING) &SecretName,
                             SECRET_QUERY_VALUE,
                             &NlpSecretHandle
                             );
    return(NtStatus);
}


NTSTATUS
NlpReadCacheEntryByIndex(
    IN  ULONG               Index,
    OUT PLOGON_CACHE_ENTRY* CacheEntry,
    OUT PULONG EntrySize
    )

/*++

Routine Description:

    Reads a cache entry from registry

Arguments:

    Index - CTE table index of the entry to open.
            This is used to build the entry's value and secret names.

    CacheEntry          - pointer to place to return pointer to LOGON_CACHE_ENTRY

    EntrySize           - size of returned LOGON_CACHE_ENTRY


Return Value:

    NTSTATUS
        Success = STATUS_SUCCESS
                    *ppEntry points to allocated LOGON_CACHE_ENTRY
                    *EntrySize is size of returned data

        Failure = STATUS_NO_MEMORY
                    Couldn't allocate buffer for LOGON_CACHE_ENTRY

--*/

{
    NTSTATUS
        NtStatus;

    UNICODE_STRING
        ValueName;

    WCHAR
        NameBuffer[32];

    ULONG
        RequiredSize;

    PKEY_VALUE_FULL_INFORMATION
        RegInfo;

    PLOGON_CACHE_ENTRY
        RCacheEntry;   // CacheEntry in registry buffer

    BYTE FastBuffer[ 512 ];
    PBYTE SlowBuffer = NULL;

    ValueName.Buffer = &NameBuffer[0];
    ValueName.MaximumLength = 32;
    ValueName.Length = 0;
    NlpMakeCacheEntryName( Index, &ValueName );


    RegInfo = (PKEY_VALUE_FULL_INFORMATION)FastBuffer;
    RequiredSize = sizeof(FastBuffer);

    //
    // perform first query to find out how much buffer to allocate
    //

    NtStatus = NtQueryValueKey(NlpCacheHandle,
                               &ValueName,
                               KeyValueFullInformation,
                               (PVOID)RegInfo,
                               RequiredSize,
                               &RequiredSize
                               );

    if( (NtStatus == STATUS_BUFFER_TOO_SMALL) ||
        (NtStatus == STATUS_BUFFER_OVERFLOW) ) {

        //
        // allocate buffer then do query again, this time receiving data
        //

        SlowBuffer = (PBYTE)AllocateFromHeap(RequiredSize);
        if (SlowBuffer == NULL) {
            return(STATUS_NO_MEMORY);
        }

        RegInfo = (PKEY_VALUE_FULL_INFORMATION)SlowBuffer;

        NtStatus = NtQueryValueKey(NlpCacheHandle,
                                   &ValueName,
                                   KeyValueFullInformation,
                                   (PVOID)RegInfo,
                                   RequiredSize,
                                   &RequiredSize
                                   );
    }


    if (NT_SUCCESS(NtStatus)) {

#if DBG
        if (DumpCacheInfo) {
            DbgPrint("NlpReadCacheEntryByIndex: Index                  : %d\n"
                     "                          NtQueryValueKey returns: %d bytes\n"
                     "                                                      DataOffset=%d\n"
                     "                                                      DataLength=%d\n",
                    Index, RequiredSize, RegInfo->DataOffset, RegInfo->DataLength);
        }
#endif

        if( RegInfo->DataLength == 0 ) {
            NtStatus = STATUS_INTERNAL_DB_CORRUPTION;
            *CacheEntry = NULL;
            *EntrySize = 0;
        } else {

            RCacheEntry = (PLOGON_CACHE_ENTRY)((PCHAR)RegInfo + RegInfo->DataOffset);
            *EntrySize = RegInfo->DataLength;

            (*CacheEntry) = (PLOGON_CACHE_ENTRY)AllocateFromHeap( (*EntrySize) );
            if ((*CacheEntry) == NULL) {
                NtStatus = STATUS_NO_MEMORY;
            } else {
                RtlCopyMemory( (*CacheEntry),
                                RCacheEntry,
                                (*EntrySize) );

            }

        }
    }

    if( SlowBuffer )
        FreeToHeap( SlowBuffer );

    return(NtStatus);
}


VOID
NlpAddEntryToActiveList(
    IN  ULONG   Index
    )

/*++

Routine Description:

    Place a CTE entry in the active CTE list.
    This requires placing the entry in the right location in
    the list chronologically.  The beginning of the list is
    the most recently updated (or referenced) cache entry.
    The end of the list is the oldest active cache entry.


    Note - The entry may be already in the active list (but
           in the wrong place), or may be on the inactive list.
           It will be removed from whichever list it is on.

Arguments:

    Index - CTE table index of the entry to make active..

Return Value:

    None.

--*/

{
    PNLP_CTE
        Next;

    //
    // Remove the entry from its current list, and then place it
    // in the active list.
    //


    RemoveEntryList( &NlpCteTable[Index].Link );


    //
    // Now walk the active list until we find a place to insert
    // the entry.  It must follow all entries with more recent
    // time stamps.
    //

    Next = (PNLP_CTE)NlpActiveCtes.Flink;

    while (Next != (PNLP_CTE)&NlpActiveCtes) {

        if ( NlpCteTable[Index].Time.QuadPart > Next->Time.QuadPart ) {

            //
            // More recent than this entry - add it here
            //

            break; // out of while-loop

        }

        Next = (PNLP_CTE)(Next->Link.Flink);  // Advance to next entry
    }


    //
    // Use the preceding entry as the list head.
    //

    InsertHeadList( Next->Link.Blink, &NlpCteTable[Index].Link );

    //
    // Mark the entry as valid
    //

    NlpCteTable[Index].Active = TRUE;

    return;
}


VOID
NlpAddEntryToInactiveList(
    IN  ULONG   Index
    )

/*++

Routine Description:

    Move the CTE entry to the inactive list.

    It doesn't matter if the entry is already inactive.

Arguments:

    Index - CTE table index of the entry to make inactive.

Return Value:

    None.

--*/

{

    //
    // Remove the entry from its current list, and then place it
    // in the inactive list.
    //


    RemoveEntryList( &NlpCteTable[Index].Link );
    InsertTailList( &NlpInactiveCtes, &NlpCteTable[Index].Link );


    //
    // Mark the entry as invalid
    //

    NlpCteTable[Index].Active = FALSE;

    return;
}


VOID
NlpGetFreeEntryIndex(
    OUT PULONG  Index
    )

/*++

Routine Description:

    This routine returns the index of either a free entry,
    or, lacking any free entries, the oldest active entry.

    The entry is left on the list it is already on.  If it
    is used by the caller, then the caller must ensure it is
    re-assigned to the active list (using NlpAddEntryToActiveList()).

    This routine is only callable if the cache is enabled (that is,
    NlpCacheControl.Entries != 0).

Arguments:

    Index - Receives the index of the next available entry.

Return Value:

    None.

--*/

{
    //
    // See if the Inactive list is empty.
    //

    if (NlpInactiveCtes.Flink != &NlpInactiveCtes) {
        (*Index) = ((PNLP_CTE)(NlpInactiveCtes.Flink))->Index;
    } else {

        //
        // Have to return the oldest active entry.
        //

        (*Index) = ((PNLP_CTE)(NlpActiveCtes.Blink))->Index;
    }

    return;
}

/////////////////////////////////////////////////////////////////////////
//                                                                     //
//          Diagnostic support services                                //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

//
// diagnostic dump routines
//

#if DBG

PCHAR
DumpOwfPasswordToString(
    OUT PCHAR Buffer,
    IN  PLM_OWF_PASSWORD Password
    )
{
    int i;
    PCHAR bufptr;

    for (i = 0, bufptr = Buffer; i < sizeof(*Password); ++i) {
        sprintf(bufptr, "%02.2x ", ((PCHAR)Password)[i] & 0xff);
        bufptr += 3;
    }
    return Buffer;
}


VOID
DumpLogonInfo(
    IN  PNETLOGON_LOGON_IDENTITY_INFO LogonInfo
    )
{

    DbgPrint(   "\n"
                "NETLOGON_INTERACTIVE_INFO:\n"
                "DomainName  : \"%*.*ws\"\n"
                "UserName    : \"%*.*ws\"\n"
                "Parm Ctrl   : %u (%x)\n"
                "LogonId     : %u.%u (%x.%x)\n"
                "Workstation : \"%*.*ws\"\n",
                LogonInfo->LogonDomainName.Length/sizeof(WCHAR),
                LogonInfo->LogonDomainName.Length/sizeof(WCHAR),
                LogonInfo->LogonDomainName.Buffer,
                LogonInfo->UserName.Length/sizeof(WCHAR),
                LogonInfo->UserName.Length/sizeof(WCHAR),
                LogonInfo->UserName.Buffer,
                LogonInfo->ParameterControl,
                LogonInfo->ParameterControl,
                LogonInfo->LogonId.HighPart,
                LogonInfo->LogonId.LowPart,
                LogonInfo->LogonId.HighPart,
                LogonInfo->LogonId.LowPart,
                LogonInfo->Workstation.Length/sizeof(WCHAR),
                LogonInfo->Workstation.Length/sizeof(WCHAR),
                LogonInfo->Workstation.Buffer
                );
}


char*
MapWeekday(
    IN  CSHORT  Weekday
    )
{
    switch (Weekday) {
    case 0: return "Sunday";
    case 1: return "Monday";
    case 2: return "Tuesday";
    case 3: return "Wednesday";
    case 4: return "Thursday";
    case 5: return "Friday";
    case 6: return "Saturday";
    }
    return "???";
}


VOID
DumpTime(
    IN  LPSTR   String,
    IN  POLD_LARGE_INTEGER OldTime
    )
{
    TIME_FIELDS tf;
    LARGE_INTEGER Time;

    OLD_TO_NEW_LARGE_INTEGER( (*OldTime), Time );

    RtlTimeToTimeFields(&Time, &tf);
    DbgPrint("%s%02d:%02d:%02d.%03d %02d/%02d/%d (%s [%d])\n",
            String,
            tf.Hour,
            tf.Minute,
            tf.Second,
            tf.Milliseconds,
            tf.Month,
            tf.Day,
            tf.Year,
            MapWeekday(tf.Weekday),
            tf.Weekday
            );
}


VOID
DumpGroupIds(
    IN  LPSTR   String,
    IN  ULONG   Count,
    IN  PGROUP_MEMBERSHIP GroupIds
    )
{
    DbgPrint(String);
    if (!Count) {
        DbgPrint("No group IDs!\n");
    } else {
        char tab[80];

        memset(tab, ' ', strlen(String));
//        tab[strcspn(String, "%")] = 0;
        tab[strlen(String)] = 0;
        while (Count--) {
            DbgPrint("%d, %d\n", GroupIds->RelativeId, GroupIds->Attributes);
            if (Count) {
                DbgPrint(tab);
            }
            ++GroupIds;
        }
    }
}


VOID
DumpSessKey(
    IN  LPSTR   String,
    IN  PUSER_SESSION_KEY Key
    )
{
    int     len;
    DbgPrint(String);
    DbgPrint("%02.2x-%02.2x-%02.2x-%02.2x-%02.2x-%02.2x-%02.2x-%02.2x\n",
            ((PUCHAR)&Key->data[0])[0],
            ((PUCHAR)&Key->data[0])[1],
            ((PUCHAR)&Key->data[0])[2],
            ((PUCHAR)&Key->data[0])[3],
            ((PUCHAR)&Key->data[0])[4],
            ((PUCHAR)&Key->data[0])[5],
            ((PUCHAR)&Key->data[0])[6],
            ((PUCHAR)&Key->data[0])[7]
            );
    len = strlen(String);
    DbgPrint("%-*.*s", len, len, "");
    DbgPrint("%02.2x-%02.2x-%02.2x-%02.2x-%02.2x-%02.2x-%02.2x-%02.2x\n",
            ((PUCHAR)&Key->data[1])[0],
            ((PUCHAR)&Key->data[1])[1],
            ((PUCHAR)&Key->data[1])[2],
            ((PUCHAR)&Key->data[1])[3],
            ((PUCHAR)&Key->data[1])[4],
            ((PUCHAR)&Key->data[1])[5],
            ((PUCHAR)&Key->data[1])[6],
            ((PUCHAR)&Key->data[1])[7]
            );
}


VOID
DumpSid(
    LPSTR   String,
    PISID   Sid
    )
{
    DbgPrint(String);

    if ( Sid == NULL ) {
        DbgPrint(0, "(null)\n");
    } else {
        UNICODE_STRING SidString;
        NTSTATUS Status;

        Status = RtlConvertSidToUnicodeString( &SidString, Sid, TRUE );

        if ( !NT_SUCCESS(Status) ) {
            DbgPrint("Invalid 0x%lX\n", Status );
        } else {
            DbgPrint( "%wZ\n", &SidString );
            RtlFreeUnicodeString( &SidString );
        }
    }
}


VOID
DumpAccountInfo(
    IN  PNETLOGON_VALIDATION_SAM_INFO2 AccountInfo
    )
{
    DbgPrint(   "\n"
                "NETLOGON_VALIDATION_SAM_INFO:\n");

    DumpTime(   "LogonTime          : ", &AccountInfo->LogonTime);

    DumpTime(   "LogoffTime         : ", &AccountInfo->LogoffTime);

    DumpTime(   "KickOffTime        : ", &AccountInfo->KickOffTime);

    DumpTime(   "PasswordLastSet    : ", &AccountInfo->PasswordLastSet);

    DumpTime(   "PasswordCanChange  : ", &AccountInfo->PasswordCanChange);

    DumpTime(   "PasswordMustChange : ", &AccountInfo->PasswordMustChange);

    DbgPrint(   "EffectiveName      : \"%*.*ws\"\n"
                "FullName           : \"%*.*ws\"\n"
                "LogonScript        : \"%*.*ws\"\n"
                "ProfilePath        : \"%*.*ws\"\n"
                "HomeDirectory      : \"%*.*ws\"\n"
                "HomeDirectoryDrive : \"%*.*ws\"\n"
                "LogonCount         : %d\n"
                "BadPasswordCount   : %d\n"
                "UserId             : %d\n"
                "PrimaryGroupId     : %d\n"
                "GroupCount         : %d\n",
                AccountInfo->EffectiveName.Length/sizeof(WCHAR),
                AccountInfo->EffectiveName.Length/sizeof(WCHAR),
                AccountInfo->EffectiveName.Buffer,
                AccountInfo->FullName.Length/sizeof(WCHAR),
                AccountInfo->FullName.Length/sizeof(WCHAR),
                AccountInfo->FullName.Buffer,
                AccountInfo->LogonScript.Length/sizeof(WCHAR),
                AccountInfo->LogonScript.Length/sizeof(WCHAR),
                AccountInfo->LogonScript.Buffer,
                AccountInfo->ProfilePath.Length/sizeof(WCHAR),
                AccountInfo->ProfilePath.Length/sizeof(WCHAR),
                AccountInfo->ProfilePath.Buffer,
                AccountInfo->HomeDirectory.Length/sizeof(WCHAR),
                AccountInfo->HomeDirectory.Length/sizeof(WCHAR),
                AccountInfo->HomeDirectory.Buffer,
                AccountInfo->HomeDirectoryDrive.Length/sizeof(WCHAR),
                AccountInfo->HomeDirectoryDrive.Length/sizeof(WCHAR),
                AccountInfo->HomeDirectoryDrive.Buffer,
                AccountInfo->LogonCount,
                AccountInfo->BadPasswordCount,
                AccountInfo->UserId,
                AccountInfo->PrimaryGroupId,
                AccountInfo->GroupCount
                );

    DumpGroupIds("GroupIds           : ",
                AccountInfo->GroupCount,
                AccountInfo->GroupIds
                );

    DbgPrint(   "UserFlags          : 0x%08x\n",
                AccountInfo->UserFlags
                );

    DumpSessKey("UserSessionKey     : ", &AccountInfo->UserSessionKey);

    DbgPrint(   "LogonServer        : \"%*.*ws\"\n"
                "LogonDomainName    : \"%*.*ws\"\n",
                AccountInfo->LogonServer.Length/sizeof(WCHAR),
                AccountInfo->LogonServer.Length/sizeof(WCHAR),
                AccountInfo->LogonServer.Buffer,
                AccountInfo->LogonDomainName.Length/sizeof(WCHAR),
                AccountInfo->LogonDomainName.Length/sizeof(WCHAR),
                AccountInfo->LogonDomainName.Buffer
                );

    DumpSid(    "LogonDomainId      : ", (PISID)AccountInfo->LogonDomainId);
}


VOID
DumpCacheEntry(
    IN  ULONG              Index,
    IN  PLOGON_CACHE_ENTRY pEntry
    )
{
    PUCHAR dataptr;
    ULONG length;

    DbgPrint(   "\n"
                "LOGON_CACHE_ENTRY:\n"
                "CTE Index          : %d\n", Index);

    if (pEntry->Valid != TRUE) {
        DbgPrint(   "State              : INVALID\n");
        return;
    }

    dataptr = (PUCHAR)(pEntry+1);

    length = pEntry->UserNameLength;

    DbgPrint(   "State              : VALID\n");
    DbgPrint(   "UserName           : \"%*.*ws\"\n", length/2, length/2, dataptr);
    dataptr = ROUND_UP_POINTER(dataptr+length, sizeof(ULONG));

    length = pEntry->DomainNameLength;
    DbgPrint(   "DomainName         : \"%*.*ws\"\n", length/2, length/2, dataptr);
    dataptr = ROUND_UP_POINTER(dataptr+length, sizeof(ULONG));

    length = pEntry->DnsDomainNameLength;
    DbgPrint(   "DnsDomainname      : \"%*.*ws\"\n", length/2, length/2, dataptr);
    dataptr = ROUND_UP_POINTER(dataptr+length, sizeof(ULONG));

    length = pEntry->UpnLength;
    DbgPrint(   "Upn                : \"%*.*ws\"\n", length/2, length/2, dataptr);
    dataptr = ROUND_UP_POINTER(dataptr+length, sizeof(ULONG));

    length = pEntry->EffectiveNameLength;
    DbgPrint(   "EffectiveName      : \"%*.*ws\"\n", length/2, length/2, dataptr);
    dataptr = ROUND_UP_POINTER(dataptr+length, sizeof(ULONG));

    length = pEntry->FullNameLength;
    DbgPrint(   "FullName           : \"%*.*ws\"\n", length/2, length/2, dataptr);
    dataptr = ROUND_UP_POINTER(dataptr+length, sizeof(ULONG));

    length = pEntry->LogonScriptLength;
    DbgPrint(   "LogonScript        : \"%*.*ws\"\n", length/2, length/2, dataptr);
    dataptr = ROUND_UP_POINTER(dataptr+length, sizeof(ULONG));

    length = pEntry->ProfilePathLength;
    DbgPrint(   "ProfilePath        : \"%*.*ws\"\n", length/2, length/2, dataptr);
    dataptr = ROUND_UP_POINTER(dataptr+length, sizeof(ULONG));

    length = pEntry->HomeDirectoryLength;
    DbgPrint(   "HomeDirectory      : \"%*.*ws\"\n", length/2, length/2, dataptr);
    dataptr = ROUND_UP_POINTER(dataptr+length, sizeof(ULONG));

    length = pEntry->HomeDirectoryDriveLength;
    DbgPrint(   "HomeDirectoryDrive : \"%*.*ws\"\n", length/2, length/2, dataptr);
    dataptr = ROUND_UP_POINTER(dataptr+length, sizeof(ULONG));

    DbgPrint(   "UserId             : %d\n"
                "PrimaryGroupId     : %d\n"
                "GroupCount         : %d\n",
                pEntry->UserId,
                pEntry->PrimaryGroupId,
                pEntry->GroupCount
                );

    DumpGroupIds(
                "GroupIds           : ",
                pEntry->GroupCount,
                (PGROUP_MEMBERSHIP)dataptr
                );

    dataptr = ROUND_UP_POINTER((dataptr+pEntry->GroupCount * sizeof(GROUP_MEMBERSHIP)), sizeof(ULONG));

    length = pEntry->LogonDomainNameLength;
    DbgPrint(   "LogonDomainName    : \"%*.*ws\"\n", length/2, length/2, dataptr);
    dataptr = ROUND_UP_POINTER(dataptr+length, sizeof(ULONG));


    if (pEntry->SidCount) {
        ULONG i, sidLength;
        PULONG SidAttributes = (PULONG) dataptr;

        dataptr = ROUND_UP_POINTER(dataptr + pEntry->SidCount * sizeof(ULONG), sizeof(ULONG));
        for (i = 0; i < pEntry->SidCount ; i++ ) {
            sidLength = RtlLengthSid ((PSID) dataptr);
            DumpSid("Sid    : ",(PISID) dataptr);
            DbgPrint("\tAttributes = 0x%x\n",SidAttributes[i]);
            dataptr = ROUND_UP_POINTER(dataptr + sidLength, sizeof(ULONG));
        }

    }

    DumpSid(    "LogonDomainId      : ", (PISID)dataptr);
}
#endif
