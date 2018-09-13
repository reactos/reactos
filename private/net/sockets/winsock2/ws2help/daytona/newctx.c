/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    newctx.c

Abstract:

    This module implements functions for creating and manipulating context
    tables. Context tables are used in WinSock 2.0 for associating 32-bit
    context values with socket handles.

Author:

    Vadim Eyldeman (VadimE)       11-Nov-1997

Revision History:

--*/


#include "precomp.h"
#include "newctx.h"


//
// Private contstants
//

//
// Min & max allowable values for number of handle lookup tables
// (must be some number == 2**N for optimal performance)
//
#define MIN_HANDLE_BUCKETS_WKS 0x8
#define MAX_HANDLE_BUCKETS_WKS 0x20
#define MIN_HANDLE_BUCKETS_SRV 0x20
#define MAX_HANDLE_BUCKETS_SRV 0x100

//
// Default & maximum spin count values (for critical section creation)
//

#define DEF_SPIN_COUNT 2000
#define MAX_SPIN_COUNT 8000


//
// Private globals
//

// Prime numbers used for closed hashing
ULONG const SockPrimes[] =
{
    31, 61, 127, 257, 521, 1031, 2053, 4099, 8191,
    16381, 32749, 65537, 131071, 261983, 
    0xFFFFFFFF  // Indicates end of the table, next number must be computed
                // on the fly.
};

DWORD   gdwSpinCount=0;     // Spin count used in critical sections
ULONG   gHandleToIndexMask; // Actual mask is currently the same for
                            // all tables in this DLL.
HANDLE  ghWriterEvent;      // Event for writer to wait on for readers
                            // if spinning and simple sleep fail.

//
// Table access macros.
//
// This uses property of Win NT handles which have two low order bits 0-ed
#define TABLE_FROM_HANDLE(_h,_tbls) \
            (&(_tbls)->Tables[((((ULONG_PTR)_h) >> 2) & (_tbls)->HandleToIndexMask)])

// Hash function (no need to shift as we use prime numbers)
#define HASH_BUCKET_FROM_HANDLE(_h,_hash) \
            ((_hash)->Buckets[(((ULONG_PTR)_h) % (_hash)->NumBuckets)])


//
// RW_LOCK macros
//

//VOID
//AcquireTableReaderLock (
//  IN  LPCTX_LOOKUP_TABLE     tbl,
//  OUT LPLONG                 pctr
//  );
/*++
*******************************************************************
Routine Description:
    Acquires reader access to table
Arguments:
    tbl - table to lock
    hash - current hash table
    pctr - pointer to buffer to store lock state for subsequent
            release operation
Return Value:
    None
*******************************************************************
--*/
#ifdef _RW_LOCK_

#define AcquireTableReaderLock(tbl,pctr)                            \
    do {                                                            \
        LONG   oldCount;                                            \
        pctr = (tbl)->ReaderCounter;/*Get current counter pointer*/ \
        oldCount = *(pctr);         /*Copy counter value*/          \
        if ((oldCount>0)            /*If counter is valid*/         \
                                    /*and it hasn't changed while*/ \
                                    /*we were checking and trying*/ \
                                    /*to increment it,*/            \
                && (InterlockedCompareExchange (                    \
                        (PLONG)(pctr),                              \
                        (oldCount+1),                               \
                        oldCount)                                   \
                   ==oldCount)) {   /*then we obtained the access*/ \
            break;                                                  \
        }                                                           \
		RecordReaderSpin(tbl);										\
    } while (1) /*otherwise, we have to do it again (possibly with*/\
                /*the other counter if writer switched it on us)*/  \

#else  //_RW_LOCK_

#define AcquireTableReaderLock(tbl,pctr)                            \
            (EnterCriticalSection(&tbl->WriterLock),hash=tbl->HashTable)

#endif //_RW_LOCK_

//VOID
//ReleaseTableReaderLock (
//  IN  LPCTX_LOOKUP_TABLE     tbl,
//  OUT LPLONG                 pctr
//  );
/*++
*******************************************************************
Routine Description:
    Releases reader access to table
Arguments:
    lock - pointer to lock
    pctr - lock state stored during acquire operation
Return Value:
    None
*******************************************************************
--*/
#ifdef _RW_LOCK_

#define ReleaseTableReaderLock(tbl,pctr)						\
            if (InterlockedDecrement((PLONG)pctr)<0) {          \
                /* Signal the event as writer is waiting */     \
                BOOL    res;                                    \
                ASSERT (ghWriterEvent!=NULL);                   \
                res = PulseEvent (ghWriterEvent);               \
                ASSERT (res);                                   \
            }


#else  //_RW_LOCK_

#define ReleaseTableReaderLock(tbl,pctr)                        \
            LeaveCriticalSection (&(tbl)->WriterLock)

#endif //_RW_LOCK_


//VOID
//AcquireTableWriterLock (
//  IN  LPCTX_LOOKUP_TABLE  tbl
//  );
/*++
*******************************************************************
Routine Description:
    Acquires writer access to table
Arguments:
    tbl - table to lock
Return Value:
    None
*******************************************************************
--*/
#define AcquireTableWriterLock(tbl) EnterCriticalSection(&(tbl)->WriterLock)

//VOID
//ReleaseTableWriterLock (
//  IN  LPCTX_LOOKUP_TABLE  tbl
//  );
/*++
*******************************************************************
Routine Description:
    Releases writer access to table
Arguments:
    tbl - table to lock
Return Value:
    None
*******************************************************************
--*/
#define ReleaseTableWriterLock(tbl) LeaveCriticalSection(&(tbl)->WriterLock)


//VOID
//WaitForAllReaders (
//  IN  LPSOCK_LOOKUP_TABLE    tbl
//  );
/*++
*******************************************************************
Routine Description:
    Waits for all readers that are in process of accessing the table
Arguments:
    tbl - table
Return Value:
    None
*******************************************************************
--*/
#ifdef _RW_LOCK_

//
// Execute long wait (context switch requred) for readers
// Extra function call won't make much difference here.
//

VOID
DoWaitForReaders (
    LPCTX_LOOKUP_TABLE  Table,
    PVOLCTR             Ctr
    )
{

	/* Force context switch to let the reader(s) go */
    Sleep (0);
    if (*Ctr>0) {
        /* If we failed we must be dealing with */              
        /* priority inversion, wait on event to let */          
        /* lower priority threads execute */                    
        if (ghWriterEvent==NULL) {                              
            /* Need to allocate a manual reset event */                     
            HANDLE  hEvent;                                     
            hEvent = CreateEvent (NULL, TRUE, FALSE, NULL);     
            if (hEvent!=NULL) {                                 
                /* Make sure someone else did not do it too*/   
                if (InterlockedCompareExchangePointer (         
                                    (PVOID *)&ghWriterEvent,    
                                    hEvent,                     
                                    NULL)!=NULL) {              
                    /* Event is already there, free ours */     
                    CloseHandle (hEvent);                       
                }                                               
            }                                                   
            else {                                              
                /* Could not allocate event, will have to */    
                /* use sleep to preempt ourselves */            
                while (*Ctr>0) {                            
                    Sleep (10);                                 
		        }                                               
            }                                                   
        }                                                       
        /* Tell the readers that we want them to signal by */   
        /* further decrementing counter so the last reader */
        /* will end up getting it below 0 and signalling us */
        if (InterlockedDecrement ((PLONG)Ctr)>=0) {         
            ASSERT (ghWriterEvent!=NULL);                       
            do {                                                
                DWORD rc;                                       
                /* We can't just wait forever because readers */
                /* pulse the event, we may miss it (can't set
                /* because event is shared))*/
                rc = WaitForSingleObject (ghWriterEvent, 10);   
                ASSERT (rc==WAIT_OBJECT_0 || rc==WAIT_TIMEOUT); 
            }                                                   
            while (*Ctr>=0);                                
        }                                                       
    }                                                           
}


#define WaitForAllReaders(tbl)         {                            \
    PVOLCTR prevCtr = (tbl)->ReaderCounter;                         \
    /*Switch the counter first*/                                    \
    if (prevCtr==&(tbl)->ReaderCount1) {                            \
        (tbl)->ReaderCount2 = 1;                                    \
        (tbl)->ReaderCounter = &(tbl)->ReaderCount2;                \
    }                                                               \
    else {                                                          \
        ASSERT (prevCtr==&(tbl)->ReaderCount2);                     \
        (tbl)->ReaderCount1 = 1;                                    \
        (tbl)->ReaderCounter = &(tbl)->ReaderCount1;                \
    }                                                               \
    /* Decrement the previous counter to force invalidation (no */  \
    /* reader will be able to increment it anymore) when last */    \
    /* owing reader releases it */                                  \
    if (InterlockedDecrement ((PLONG)prevCtr)>0) {                  \
        /* Some readers remain, we'll have to wait for them*/       \
		RecordWriterWait(tbl);										\
		/* Spin in case reader is executing on another processor */ \
        /* (SpinCount can only be non-zero on MP machines) */       \
		if ((tbl)->SpinCount) {										\
			LONG    spinCtr = (tbl)->SpinCount;						\
			while (*prevCtr>0) {									\
				if (--spinCtr<=0) {                                 \
					RecordFailedSpin(tbl);							\
					break;											\
				}													\
			}														\
		}															\
        if (*prevCtr>0) {                                           \
            /* Still someone there, we'll have to do long wait */   \
            DoWaitForReaders (tbl,prevCtr);                         \
        }                                                           \
	}																\
}

#else //_RW_LOCK_

#define WaitForAllReaders(tbl)         {                            \
}

#endif //_RW_LOCK_

//
// Function called during DLL initialization to peek up
// system parameters and initialize globals
//
VOID
NewCtxInit (
    VOID
    ) {
    ULONG           numLookupTables;
    NT_PRODUCT_TYPE productType;
    ULONG           i;
    HINSTANCE       hDll;
    HKEY            hKey;
    ULONG           dwDataSize, dwDataType, dwBitMask;
	SYSTEM_INFO		sysInfo;


    productType = NtProductWinNt;

    if (!RtlGetNtProductType (&productType)) {
        productType = NtProductWinNt;
    }

	GetSystemInfo (&sysInfo);

    //
    // Spin only on MP machines.
    //
	if (sysInfo.dwNumberOfProcessors>1) {
        gdwSpinCount = DEF_SPIN_COUNT;
    }
    else {
        gdwSpinCount = 0;
    }
    //
    // Determine the # of lookup table entries or "handle buckets".
    // This number will vary depending on whether the platform is NT
    // Server or not. Allow configuration of this value via registry,
    // and make sure that the # of buckets is reasonable given the
    // platform, and that it is some value == 2**N so our handle->lock
    // mapping scheme is optimal.
    //
    // Also retrieve the spin count for use in calling
    // InitializeCriticalSectionAndSpinCount()
    //


    numLookupTables = ( productType==NtProductWinNt ?
        MIN_HANDLE_BUCKETS_WKS : MIN_HANDLE_BUCKETS_SRV );

    if (RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            TEXT("System\\CurrentControlSet\\Services\\Winsock2\\Parameters"),
            0,
            KEY_QUERY_VALUE,
            &hKey

            ) == ERROR_SUCCESS) {

        dwDataSize = sizeof (numLookupTables);
        RegQueryValueEx(
            hKey,
            TEXT("Ws2_32NumHandleBuckets"),
            0,
            &dwDataType,
            (LPBYTE) &numLookupTables,
            &dwDataSize
            );

		if (sysInfo.dwNumberOfProcessors>1) {
			// Spinning only makes sense on multiprocessor machines
			dwDataSize = sizeof (gdwSpinCount);
			RegQueryValueEx(
				hKey,
				TEXT("Ws2_32SpinCount"),
				0,
				&dwDataType,
				(LPBYTE) &gdwSpinCount,
				&dwDataSize
				);
		}

        RegCloseKey (hKey);
    }

    // Make sure the number is power of 2 and whithin the limits
    for(
        dwBitMask = MAX_HANDLE_BUCKETS_SRV;
        (dwBitMask & numLookupTables) == 0;
        dwBitMask >>= 1
        );
    numLookupTables = dwBitMask;

    if ( productType==NtProductWinNt ) {

        if ( numLookupTables > MAX_HANDLE_BUCKETS_WKS ) {

            numLookupTables = MAX_HANDLE_BUCKETS_WKS;

        }
        else if ( numLookupTables < MIN_HANDLE_BUCKETS_WKS ){

            numLookupTables = MIN_HANDLE_BUCKETS_WKS;
        }
    }
    else {

        if ( numLookupTables > MAX_HANDLE_BUCKETS_SRV ) {

            numLookupTables = MAX_HANDLE_BUCKETS_SRV;

        }
        else if ( numLookupTables < MIN_HANDLE_BUCKETS_SRV ){

            numLookupTables = MIN_HANDLE_BUCKETS_SRV;
        }
    }
    if (gdwSpinCount>MAX_SPIN_COUNT) {
        gdwSpinCount = MAX_SPIN_COUNT;
    }

    gHandleToIndexMask = numLookupTables - 1;
}


WS2HELPAPI
DWORD
WINAPI
WahCreateHandleContextTable(
    OUT LPCONTEXT_TABLE FAR * Table
    ) 
/*++

Routine Description:

    Creates handle -> context lookup table

Arguments:

    Table   -   Returns pointer to the created table


Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/

{
    INT     return_code = ERROR_SUCCESS;
    ULONG   i;

    return_code = ENTER_WS2HELP_API ();
    if (return_code!=0)
        return return_code;


    //
    // Allocate & initialize the handle lookup table
    //

    *Table = ALLOC_MEM (FIELD_OFFSET (struct _CONTEXT_TABLE,
                            Tables[gHandleToIndexMask+1]));

    if ( *Table == NULL ) {

        return WSA_NOT_ENOUGH_MEMORY;
    }

    (*Table)->HandleToIndexMask = gHandleToIndexMask;

    for ( i = 0; i <= gHandleToIndexMask; i++ ) {

        (*Table)->Tables[i].HashTable = NULL;
#ifdef _RW_LOCK_
        (*Table)->Tables[i].ReaderCounter = &(*Table)->Tables[i].ReaderCount1;
        (*Table)->Tables[i].ReaderCount1 = 1;
        (*Table)->Tables[i].ReaderCount2 = 0;
        (*Table)->Tables[i].ExpansionInProgress = FALSE;
        (*Table)->Tables[i].SpinCount = gdwSpinCount;
#ifdef _PERF_DEBUG_
        (*Table)->Tables[i].ReaderSpins = 0;
        (*Table)->Tables[i].WriterWaits = 0;
        (*Table)->Tables[i].FailedSpins = 0;
#endif
#endif //_RW_LOCK_
        __try {

            InitializeCriticalSectionAndSpinCount (
                &(*Table)->Tables[i].WriterLock,
                gdwSpinCount
                );
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            goto Cleanup;
        }
    }

    return ERROR_SUCCESS;

Cleanup:
    for (i-=1; i>=0; i--) {
        DeleteCriticalSection (&(*Table)->Tables[i].WriterLock);
    }
    FREE_MEM (*Table);
    return WSA_NOT_ENOUGH_MEMORY;
}

WS2HELPAPI
DWORD
WINAPI
WahDestroyHandleContextTable(
    LPCONTEXT_TABLE Table
    )
/*++

Routine Description:

    Destroys handle -> context lookup table

Arguments:

    Table   -   Supplies pointer to the table to destory


Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/
{
    ULONG i;

//  Valid table pointer required anyway.
//    return_code = ENTER_WS2HELP_API ();
//    if (return_code!=0)
//        return return_code;

    if (Table!=NULL) {

        for ( i = 0; i <= Table->HandleToIndexMask; i++ ) {
			
            if ( Table->Tables[i].HashTable != NULL) {

                FREE_MEM (Table->Tables[i].HashTable);
            }

            DeleteCriticalSection (&Table->Tables[i].WriterLock);
        }

        FREE_MEM (Table);
        return ERROR_SUCCESS;
    }
    else {
        return ERROR_INVALID_PARAMETER;
    }

}

WS2HELPAPI
LPWSHANDLE_CONTEXT
WINAPI
WahReferenceContextByHandle(
    LPCONTEXT_TABLE Table,
    HANDLE          Handle
    )
/*++

Routine Description:

    Looks up context for the handle in the table

Arguments:

    Table   -   Supplies pointer to the table in which to lookup

    Handle  -   Supplies WinNT object handle to find context for


Return Value:

    REFERENCED context for the handle if found, 
    NULL if it does not exist

--*/
{
    LPWSHANDLE_CONTEXT  ctx;
    LPCTX_LOOKUP_TABLE  table = TABLE_FROM_HANDLE(Handle,Table);
    LPCTX_HASH_TABLE    hash;
    PVOLCTR             state;

//  Valid table pointer required anyway.
//    return_code = ENTER_WS2HELP_API ();
//    if (return_code!=0)
//        return return_code;

    // Take the lock
    AcquireTableReaderLock (table,state);
    hash = table->HashTable;

    // Make sure that context exists and for the right handle
    if ((hash!=NULL)
            && ((ctx=HASH_BUCKET_FROM_HANDLE(
                                        Handle,
                                        hash))!=NULL)
            && (ctx->Handle==Handle)) {
        WahReferenceHandleContext(ctx);
    }
    else {
        ctx = NULL;
    }
    ReleaseTableReaderLock (table, state);

    return ctx;
}

WS2HELPAPI
LPWSHANDLE_CONTEXT
WINAPI
WahInsertHandleContext(
    LPCONTEXT_TABLE     Table,
    LPWSHANDLE_CONTEXT  HContext
    )
/*++

Routine Description:

    Inserts context for the handle in the table.

Arguments:

    Table   -   Supplies pointer to the table into which to insert

    HContext -  Supplies handle context to insert which contains
                handle value and internally\externally used
                reference count


Return Value:
    NULL    -   context could not be inserted because of allocation failure
    HContext -  context was successfully inserted into empty cell
    other context - HContext replaced another context which is returned to
                    the caller

--*/
{
    INT					return_code;
    LPWSHANDLE_CONTEXT  *pBucket, oldContext;
    PVOLCTR             state;
    LPCTX_HASH_TABLE    hash, newHashTable;
    ULONG               newNumHashBuckets, i;
    LPCTX_LOOKUP_TABLE  table = TABLE_FROM_HANDLE(HContext->Handle,Table);

//  Valid table pointer required anyway.
//    return_code = ENTER_WS2HELP_API ();
//    if (return_code!=0)
//        return return_code;

    do {
#ifdef _RW_LOCK_
		AcquireTableReaderLock(table,state);
        hash = table->HashTable;
		//
		// First make sure we have already initialized hash table
        // and it is not being expanded at the moment
		//
        if (!table->ExpansionInProgress && (hash!=NULL)) {
            pBucket = &HASH_BUCKET_FROM_HANDLE(HContext->Handle,hash);
			//
			// Try to insert handle context into the table
			//
            if (InterlockedCompareExchangePointer (
							(PVOID *)pBucket,
							HContext,
							NULL)==NULL) {
			    //
			    // If bucket was empty and thus we succeded, get out.
			    //
			    ReleaseTableReaderLock(table,state);
                oldContext = HContext;
                break;
            }
            else {
                //
                // Another context for the same handle exists or collision 
                // in hash value, need to go the long way with the exclusive lock
                // held (we will either need to replace the context or expand
                // the table, in both cases we need to make sure that no-one
                // can be still looking at the table or the context after
                // we return.
                //
            }
        }
        else {
			//
			// Table is empty, need to create one
			//
		}

	    ReleaseTableReaderLock(table,state);
#endif //_RW_LOCK_

        //
        // Acquire writer lock for table expansion operation.
        //
		AcquireTableWriterLock (table);

        //
        // Make sure no-one is trying to modify the table
        //
#ifdef _RW_LOCK_
        table->ExpansionInProgress = TRUE;
#endif //_RW_LOCK_
        WaitForAllReaders (table);

        do {
            hash = table->HashTable;
            if (hash!=NULL) {

                //
                // First check if we can succeed with the current table
                // We use the same logic as above, except we now have full
                // control of the table, so no need for interlocked operations
                //

                pBucket = &HASH_BUCKET_FROM_HANDLE (HContext->Handle, hash);
                if (*pBucket==NULL) {
                    oldContext = HContext;
                    *pBucket = HContext;
                    break;
                }
                else if ((*pBucket)->Handle==HContext->Handle) {
                    oldContext = *pBucket;
                    *pBucket = HContext;
                    break;
                }


                //
                // We in fact have to resort to table expansion
                // Remember the table size to know where to start
                //
                newNumHashBuckets = hash->NumBuckets;
            }
            else {
                //
                // Table was in fact empty, we 'll have to build one
                //
                newNumHashBuckets = 0;
            }

			//
			// Actual table expansion loop
			//
		TryAgain:

            //
            // Find the next prime number.
            //
            for (i = 0; newNumHashBuckets>=SockPrimes[i]; i++)
                ;

            if (SockPrimes[i]!=0xFFFFFFFF) {
                newNumHashBuckets = SockPrimes[i];
            }
            else {
                //
                // Reached the end of precomputed primes, simply
                // double the size of the table (we are getting
                // real big now, any mapping should do).
                //
                newNumHashBuckets *= 2;
            }


            newHashTable = (LPCTX_HASH_TABLE) ALLOC_MEM(
                        FIELD_OFFSET (
                                CTX_HASH_TABLE,
                                Buckets[newNumHashBuckets])
                        );
            if (newHashTable!=NULL) {
                newHashTable->NumBuckets = newNumHashBuckets;

                ZeroMemory(
                    newHashTable->Buckets,
                    newNumHashBuckets * sizeof (newHashTable->Buckets[0])
                );

                //
                // Well, first insert the new object, that's why we are
                // there in the first place.
                //

                HASH_BUCKET_FROM_HANDLE(HContext->Handle, newHashTable) = HContext;

                if (hash!=NULL) {
                    //
                    // The previous table wasn't empty, we need to
                    // move all the entries.  Note that we have
                    // already freezed all the table modifications
                    // above.
                    //

                    for (i=0 ; i<hash->NumBuckets; i++) {

                        if (hash->Buckets[i] != NULL) {

                            pBucket = &HASH_BUCKET_FROM_HANDLE(
                                hash->Buckets[i]->Handle,
                                newHashTable
                                );

                            if (*pBucket == NULL) {

                                *pBucket = hash->Buckets[i];

                            } else {
                                ASSERT ((*pBucket)->Handle!=hash->Buckets[i]->Handle);
                                FREE_MEM (newHashTable);

                                //
                                // Collision *after* we expanded, goto next size table
                                //
                                goto TryAgain;
                            }
                        }
                    }
                    //
                    // Table was successfully moved, safe to destroy
                    // the old one except we need to wait while no-one
                    // is accessing it.
                    //
                    table->HashTable = newHashTable;
                    WaitForAllReaders (table);
                    FREE_MEM( hash );
                }
                else {
                    //
                    // That's our hash table now.
                    //
                    table->HashTable = newHashTable;
                }
                oldContext = HContext;
            }
            else {

                oldContext = NULL;
            }
        }
        while (0);

        //
        // Set or restore the hash table before releasing the lock
        //
#ifdef _RW_LOCK_
        table->ExpansionInProgress = FALSE;
#endif //_RW_LOCK_
		ReleaseTableWriterLock(table);
        break;
	}
	while (1);

    return oldContext;
}

WS2HELPAPI
DWORD
WINAPI
WahRemoveHandleContext(
    LPCONTEXT_TABLE     Table,
    LPWSHANDLE_CONTEXT  HContext
    )
/*++

Routine Description:

    Removes context for the handle from the table

Arguments:

    Table   -   Supplies pointer to the table from which to remove

    HContext -  Supplies handle context to insert which contains
                handle value and internally\externally used
                reference count


Return Value:

    NO_ERROR - success, ERROR_INVALID_PARAMETER context did not exist
    in the table
--*/
{
    LPWSHANDLE_CONTEXT  *pBucket;
    LPCTX_LOOKUP_TABLE  table = TABLE_FROM_HANDLE(HContext->Handle,Table);
    LPCTX_HASH_TABLE    hash;
    DWORD               rc = NO_ERROR;

//  Valid table pointer required anyway.
//    return_code = ENTER_WS2HELP_API ();
//    if (return_code!=0)
//        return return_code;

    AcquireTableWriterLock(table);
    hash = table->HashTable;
    pBucket = &HASH_BUCKET_FROM_HANDLE(HContext->Handle,hash);
    if ((hash!=NULL)
            // Use interlocked operation to make sure we won't remove
            // another context.
            && (InterlockedCompareExchangePointer (
                        (PVOID *)pBucket,
                        NULL,
                        HContext)==HContext)) {
            // Wait for all who might be trying to access this block,
            // so that the caller can free it.
        WaitForAllReaders (table);
    }
    else {
        rc = ERROR_INVALID_PARAMETER;
    }
    ReleaseTableWriterLock(table);
    return rc;
}


WS2HELPAPI
BOOL
WINAPI
WahEnumerateHandleContexts(
    LPCONTEXT_TABLE         Table,
    LPFN_CONTEXT_ENUMERATOR Enumerator,
    LPVOID                  EnumCtx
    ) 
/*++

Routine Description:

    Calls specified enumeration procedure for all contexts in the table
    untill enumeration function returns FALSE.
    While enumeration is performed the table is completely locked
    for any modifications (read access is still allowed).  It is OK to
    call table modification procedures from inside the enumeration
    function (modification lock allows for recursion).

Arguments:

    Table   -   Supplies pointer to the table to enumerate

    Enumerator  - pointer to enumeration function defined as follows:
        typedef 
            BOOL  
            (WINAPI * LPFN_CONTEXT_ENUMERATOR)(
                LPVOID              EnumCtx,    // Enumeration context
                LPWSHANDLE_CONTEXT  HContext    // Handle context
                );

    EnumCtx - context to pass to enumeration function

Return Value:

    Returns result returned by the enumeration function

--*/
{
    ULONG               i,j;
    LPWSHANDLE_CONTEXT  hContext;
    BOOL                res = TRUE;


//  Valid table pointer required anyway.
//    return_code = ENTER_WS2HELP_API ();
//    if (return_code!=0)
//        return return_code;

    for (i = 0; i <= Table->HandleToIndexMask; i++)
    {
        LPCTX_LOOKUP_TABLE  table = &Table->Tables[i];
        LPCTX_HASH_TABLE    hash;


        AcquireTableWriterLock(table);
#ifdef _RW_LOCK_
        table->ExpansionInProgress = TRUE;
#endif //_RW_LOCK_
        WaitForAllReaders (table);
        hash = table->HashTable;

        if (hash!=NULL) {
            for (j=0; j<hash->NumBuckets; j++) {
                hContext = hash->Buckets[j];
                if (hContext==NULL)
                    continue;

                res = Enumerator (EnumCtx, hContext);
                if (!res)
                    break;
            }
        }
#ifdef _RW_LOCK_
        table->ExpansionInProgress = FALSE;
#endif //_RW_LOCK_
		ReleaseTableWriterLock(table);
        if (!res)
            break;
    }
    return res;
}
