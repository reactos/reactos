/*++



Copyright (c) 1991  Microsoft Corporation

Module Name:

    RegeCls.c

Abstract:

    This module contains helper functions for enumerating
    class registrations via the win32 RegEnumKeyEx api

Author:

    Adam Edwards (adamed) 06-May-1998

Key Functions:

    EnumTableGetNextEnum
    EnumTableRemoveKey
    InitializeClassesEnumTable
    ClassKeyCountSubKeys

Notes:

    Starting with NT5, the HKEY_CLASSES_ROOT key is per-user
    instead of per-machine -- previously, HKCR was an alias for
    HKLM\Software\Classes.  Please see regclass.c for more information
    on this functionality.

    This feature complicates registry key enumeration because certain keys,
    such as CLSID, can have some subkeys that come from HKLM\Software\Classes, and
    other subkeys that come from HKCU\Software\Classes.  Since the feature is
    implemented in user mode, the kernel mode apis know nothing of this.  When it's
    time to enumerate keys, the kernel doesn't know that it should enumerate keys from
    two different parent keys.

    The key problem is that keys with the same name can exist in the user and machine portions.
    When this happens, we choose the user portion is belonging to HKCR -- the other
    one does not exist -- it is "overridden" by the user version. This means that
    we cannot simply enumerate from both places and return the results -- we would
    get duplicates in this case.  Thus, we have to do work in user mode to make
    sure duplicates are not returned.

    This module provides the user mode implementation for enumerating class
    registration keys in HKEY_CLASSES_ROOT.

    The general method is to maintain state between each call to RegEnumKeyEx.  The
    state is kept in a global table indexed by registry key handle and thread id. The
    state allows the api to remember where it is in the enumeration.  The rest of the code
    handles finding the next key, which is accomplished by retrieving keys from both user
    and machine locations.  Since the kernel returns keys from either of these locations in
    sorted order, we can compare the key names and return whichever one is less or greater,
    depending on if we're enumerating upward or downward.  We keep track of where
    we are for both user and machine locations, so we know which key to enumerate
    next and when to stop.

**************************
    IMPORTANT ASSUMPTIONS:
**************************

    BUGBUG: This code assumes that the caller has both query permission and enumerate subkey 
    permission in the registry key's acl -- some calls may fail with access denied if the
    acl denies access to the caller.

--*/


#ifdef LOCAL

#include <rpc.h>
#include "regrpc.h"
#include "localreg.h"
#include "regclass.h"
#include "regecls.h"
#include <malloc.h>


NTSTATUS QueryKeyInfo(
    HKEY hKey,
    PKEY_FULL_INFORMATION* ppKeyFullInfo,
    ULONG BufferLength,
    BOOL fClass,
    USHORT MaxClassLength);

//
// Global table of registry key enumeration state.  This is initialized
// at dll initialize time.
//
EnumTable gClassesEnumTable;

//
// Global indicating need for calling thread detach routines
//
BOOL gbDllHasThreadState = FALSE;

// BUGBUG: This recordset merging is all in user mode -- should be moved to the kernel for perf and
// other reasons.

BOOL InitializeClassesEnumTable()
/*++

Routine Description:

    Initializes the global classes enumeration table when
    advapi32.dll is initialized.

Arguments:

Return Value:

    Returns TRUE for success, FALSE for failure

Notes:

--*/
{
    NTSTATUS Status;

    //
    // Init the classes enumeration table
    //
    Status = EnumTableInit(&gClassesEnumTable);

    return NT_SUCCESS(Status);
}

BOOL CleanupClassesEnumTable(BOOL fThisThreadOnly)
/*++

Routine Description:

    Uninitializes the global classes enumeration table when
    advapi32.dll is unloaded -- this frees all
    heap associated with the enumeration table, including
    that for keys which have not been closed. Other resources
    required for the table are also freed.

Arguments:

    dwCriteria - if this is ENUM_TABLE_REMOVEKEY_CRITERIA_THISTHREAD,
       then only the table entries concerning this thread are cleaned up.
       If it is ENUM_TABLE_REMOVEKEY_CRITERIA_ANYTHREAD, the table entries
       for all threads in the process are cleaned up.

Return Value:

    TRUE for success, FALSE otherwise.

Notes:

--*/
{
    NTSTATUS Status;
    DWORD    dwCriteria;

    dwCriteria = fThisThreadOnly ? ENUM_TABLE_REMOVEKEY_CRITERIA_THISTHREAD :
        ENUM_TABLE_REMOVEKEY_CRITERIA_ANYTHREAD;

    //
    // Clear our enumeration table
    //
    Status = EnumTableClear(&gClassesEnumTable, dwCriteria);

    return NT_SUCCESS(Status);
}

NTSTATUS EnumTableInit(EnumTable* pEnumTable)
/*++

Routine Description:

    Initializes an enumeration state table

Arguments:

    pEnumTable - table to initialize

Return Value:

    Returns NT_SUCCESS (0) for success; error-code for failure.

Notes:

--*/
{
    NTSTATUS   Status;
    EnumState* rgNewState;

#if defined(_REGCLASS_ENUMTABLE_INSTRUMENTED_)
    DbgPrint("WINREG: Instrumented enum table data for process id 0x%x\n", NtCurrentTeb()->ClientId.UniqueProcess);
    DbgPrint("WINREG: EnumTableInit subtree state size %d\n", sizeof(rgNewState->UserState));
    DbgPrint("WINREG: EnumTableInit state size %d\n", sizeof(*rgNewState));
    DbgPrint("WINREG: EnumTableInit initial table size %d\n", sizeof(*pEnumTable));
#endif // _REGCLASS_ENUMTABLE_INSTRUMENTED_

    //
    // Initialize the thread list
    //
    StateObjectListInit(
        &(pEnumTable->ThreadEnumList),
        0);

    //
    // We have not initialized the critical section
    // for this table yet -- remember this.
    //
    pEnumTable->bCriticalSectionInitialized = FALSE;

    //
    // Initialize the critical section that will be used to
    // synchronize access to this table
    //
    Status = RtlInitializeCriticalSection(
                    &(pEnumTable->CriticalSection));

    //
    // Remember that we have initialized this critical section
    // so we can remember to delete it.
    //
    pEnumTable->bCriticalSectionInitialized = NT_SUCCESS(Status);

    return Status;
}


NTSTATUS EnumTableClear(EnumTable* pEnumTable, DWORD dwCriteria)
/*++

Routine Description:

    Clears all state in an enumeration table --
    frees all state (memory, resources) memory associated
    with the enumeration table.

Arguments:

    pEnumTable - table to clear
    
    dwCriteria - if this is ENUM_TABLE_REMOVEKEY_CRITERIA_THISTHREAD,
       enumeration states are removed for this thread only.
       If it is ENUM_TABLE_REMOVEKEY_CRITERIA_ANYTHREAD, enumeration
       states are removed for all threads in the process.

Return Value:

    none

Notes:

--*/
{
    NTSTATUS    Status;
    BOOL        fThisThreadOnly;
    DWORD       dwThreadId;

#if defined(_REGCLASS_ENUMTABLE_INSTRUMENTED_)
    DWORD        cOrphanedStates = 0;        
#endif // _REGCLASS_ENUMTABLE_INSTRUMENTED_


    ASSERT((ENUM_TABLE_REMOVEKEY_CRITERIA_THISTHREAD == dwCriteria) ||
           (ENUM_TABLE_REMOVEKEY_CRITERIA_ANYTHREAD == dwCriteria));

    Status = STATUS_SUCCESS;

    //
    // we assume that if we are called with ENUM_TABLE_REMOVEKEY_CRITERIA_ANYTHREAD
    // that we are being called at process detach to remove all keys from the
    // table and free the table itself -- this means that we are the only
    // thread executing this code.
    //

    //
    // Protect ourselves while modifying the table
    //
    if (dwCriteria != ENUM_TABLE_REMOVEKEY_CRITERIA_ANYTHREAD) {

        Status = RtlEnterCriticalSection(&(pEnumTable->CriticalSection));

        ASSERT( NT_SUCCESS( Status ) );
        if ( !NT_SUCCESS( Status ) ) {
#if DBG
            DbgPrint( "WINREG: RtlEnterCriticalSection() in EnumTableRemoveKey() failed. Status = %lx \n", Status );
#endif
            return Status;
        }
    }

    fThisThreadOnly = (ENUM_TABLE_REMOVEKEY_CRITERIA_THISTHREAD == dwCriteria);

    //
    // Find our thread id if the caller wants to remove
    // state for just this thread
    //
    if (fThisThreadOnly) {
        
        KeyStateList* pStateList;

        dwThreadId = GetCurrentThreadId();

        pStateList = (KeyStateList*) StateObjectListRemove(
            &(pEnumTable->ThreadEnumList),
            (PVOID) dwThreadId);

        //
        // Announce that this dll no longer stores state for any
        // threads -- used to avoid calls to dll thread
        // detach routines when there's no state to clean up.
        //
        if (StateObjectListIsEmpty(&(pEnumTable->ThreadEnumList))) {
            gbDllHasThreadState = FALSE;
        }

        if (pStateList) {
            KeyStateListDestroy((StateObject*) pStateList);
        }
            
    } else {

        //
        // If we're clearing all threads, just destroy this list
        //
        StateObjectListClear(&(pEnumTable->ThreadEnumList),
                             KeyStateListDestroy);

        gbDllHasThreadState = FALSE;

    }

    //
    // It's safe to unlock the table
    //
    if (dwCriteria != ENUM_TABLE_REMOVEKEY_CRITERIA_ANYTHREAD) {

        Status = RtlLeaveCriticalSection(&(pEnumTable->CriticalSection));

        ASSERT( NT_SUCCESS( Status ) );
#if DBG
        if ( !NT_SUCCESS( Status ) ) {
            DbgPrint( "WINREG: RtlLeaveCriticalSection() in EnumTableClear() failed. Status = %lx \n", Status );
        }
#endif
    }

    if (pEnumTable->bCriticalSectionInitialized && !fThisThreadOnly) {

        Status = RtlDeleteCriticalSection(&(pEnumTable->CriticalSection));

        ASSERT(NT_SUCCESS(Status));

#if DBG
        if ( !NT_SUCCESS( Status ) ) {
            DbgPrint( "WINREG: RtlDeleteCriticalSection() in EnumTableClear() failed. Status = %lx \n", Status );
        }
#endif

    }

#if defined(_REGCLASS_ENUMTABLE_INSTRUMENTED_)
    if (!fThisThreadOnly) {
        DbgPrint("WINREG: EnumTableClear() deleted %d unfreed states.\n", cOrphanedStates);
        DbgPrint("WINREG: If the number of unfreed states is > 1, either the\n"
                 "WINREG: process terminated a thread with TerminateThread, the process\n"
                 "WINREG: didn't close all registry handles before exiting,\n"
                 "WINREG: or there's a winreg bug in the classes enumeration code\n");
    }
#endif // _REGCLASS_ENUMTABLE_INSTRUMENTED_

    return Status;
}


NTSTATUS EnumTableFindKeyState(
    EnumTable*     pEnumTable,
    HKEY           hKey,
    EnumState**    ppEnumState)
/*++

Routine Description:

   Searches for the state for a registry key in
   an enumeration table

Arguments:

    pEnumTable - table in which to search

    hKey - key for whose state we're searching

    ppEnumState - out param for result of search

Return Value:

    Returns NT_SUCCESS (0) for success; error-code for failure.

Notes:

--*/
{
    KeyStateList* pStateList;

    pStateList = (KeyStateList*) StateObjectListFind(
        &(pEnumTable->ThreadEnumList),
        (PVOID) GetCurrentThreadId());

    if (!pStateList) {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    } else {

        *ppEnumState = (EnumState*) StateObjectListFind(
            (StateObjectList*) pStateList,
            hKey);
        
        if (!*ppEnumState) {
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS EnumTableAddKey(
    EnumTable*         pEnumTable,
    HKEY               hKey,
    DWORD              dwFirstSubKey,
    EnumState**        ppEnumState,
    EnumState**        ppRootState)
/*++

Routine Description:

   Adds an enumeration state to
   an enumeration table for a given key.

Arguments:

    pEnumTable - table in which to add state

    hKey - key for whom we want to add state

    dwFirstSubKey - index of first subkey requested by caller
        for enumeration

    ppEnumState - out param for result of search or add

Return Value:

    Returns NT_SUCCESS (0) for success; error-code for failure.

Notes:

--*/
{
    EnumState*    pEnumState;
    KeyStateList* pStateList;
    NTSTATUS      Status;
    
    pEnumState = NULL;

    //
    // Announce that this dll has thread state so it will
    // be properly cleaned up by dll thread detach routines
    //
    gbDllHasThreadState = TRUE;

    pStateList = (KeyStateList*) StateObjectListFind(
        (StateObjectList*) &(pEnumTable->ThreadEnumList),
        (PVOID) GetCurrentThreadId());

    if (!pStateList) {
        
        pStateList = RegClassHeapAlloc(sizeof(*pStateList));

        if (!pStateList) {
            return STATUS_NO_MEMORY;
        }

        KeyStateListInit(pStateList);

        StateObjectListAdd(
            &(pEnumTable->ThreadEnumList),
            (StateObject*) pStateList);
    }

    pEnumState = RegClassHeapAlloc(sizeof(*pEnumState));

    if (!pEnumState) {
        return STATUS_NO_MEMORY;
    }
    
    RtlZeroMemory(pEnumState, sizeof(*pEnumState));

    {
        SKeySemantics  keyinfo;
        UNICODE_STRING EmptyString = {0, 0, 0};
        BYTE           rgNameBuf[REG_MAX_CLASSKEY_LEN + REG_CHAR_SIZE + sizeof(KEY_NAME_INFORMATION)];
    
        //
        // Set buffer to store info about this key
        //
        RtlZeroMemory(&keyinfo, sizeof(keyinfo));

        keyinfo._pFullPath = (PKEY_NAME_INFORMATION) rgNameBuf;
        keyinfo._cbFullPath = sizeof(rgNameBuf);

        //
        // get information about this key
        //
        Status = BaseRegGetKeySemantics(hKey, &EmptyString, &keyinfo);

        if (!NT_SUCCESS(Status)) {
            goto error_exit;
        }

        //
        // initialize the empty spot
        //
        Status = EnumStateInit(
            pEnumState,
            hKey,
            dwFirstSubKey,
            dwFirstSubKey ? ENUM_DIRECTION_BACKWARD : ENUM_DIRECTION_FORWARD,
            &keyinfo);

        BaseRegReleaseKeySemantics(&keyinfo);

        if (!NT_SUCCESS(Status)) {
            goto error_exit;
        }

        if (IsRootKey(&keyinfo)) {

            NTSTATUS   RootStatus;

            //
            // If this fails, it is not fatal -- it just means
            // we may miss out on an optimization.  This can 
            // fail due to out of memory, so it is possible
            // that it may fail and we would still want to continue
            //
            RootStatus = EnumTableGetRootState(pEnumTable, ppRootState);

#if DBG
            if (!NT_SUCCESS(RootStatus)) {
                DbgPrint( "WINREG: EnumTableAddKey failed to get classes root state. Status = %lx \n", RootStatus );
            }
#endif // DBG


            if (NT_SUCCESS(RootStatus)) {

                RootStatus = EnumStateCopy(
                    pEnumState,
                    *ppRootState);

#if DBG
                if (!NT_SUCCESS(RootStatus)) {
                    DbgPrint( "WINREG: EnumTableAddKey failed to copy key state. Status = %lx \n", RootStatus );
                }
#endif // DBG
            }
        }
    }

    //
    // set the out parameter for the caller
    //
    *ppEnumState = pEnumState;

    StateObjectListAdd(
        (StateObjectList*) pStateList,
        (StateObject*) pEnumState);
    
    Status = STATUS_SUCCESS;

error_exit:

    if (!NT_SUCCESS(Status) && pEnumState) {
        RegClassHeapFree(pEnumState);
    }

    return Status;
}

NTSTATUS EnumTableRemoveKey(
    EnumTable* pEnumTable,
    HKEY       hKey,
    DWORD      dwCriteria)
/*++

Routine Description:

   remove an enumeration state from
   an enumeration table for a given key.

Arguments:

    pEnumTable - table in which to remove state

    hKey - key whose state we wish to remove

    dwCriteria - if this is ENUM_TABLE_REMOVEKEY_CRITERIA_THISTHREAD,
       the enumeration state for hkey is removed for this thread only.
       If it is ENUM_TABLE_REMOVEKEY_CRITERIA_ANYTHREAD, the enumeration
       state for hkey is removed for all threads in the
       process.

Return Value:

    Returns NT_SUCCESS (0) for success; error-code for failure.

Notes:

--*/
{
    KeyStateList* pStateList;
    EnumState*    pEnumState;
    BOOL          fThisThreadOnly;
    NTSTATUS      Status;

    //
    // Protect ourselves while modifying the table
    //
    Status = RtlEnterCriticalSection(&(pEnumTable->CriticalSection));

    ASSERT( NT_SUCCESS( Status ) );
    if ( !NT_SUCCESS( Status ) ) {
#if DBG
        DbgPrint( "WINREG: RtlEnterCriticalSection() in EnumTableRemoveKey() failed. Status = %lx \n", Status );
#endif
        return Status;
    }

    Status = STATUS_OBJECT_NAME_NOT_FOUND;

    fThisThreadOnly = (ENUM_TABLE_REMOVEKEY_CRITERIA_THISTHREAD == dwCriteria);

    {
        KeyStateList* pNext;

        pNext = NULL;

        for (pStateList = (KeyStateList*) (pEnumTable->ThreadEnumList.pHead);
             pStateList != NULL;
             pStateList = NULL)
        {
            EnumState* pEnumState;

            if (fThisThreadOnly) {

                pStateList = (KeyStateList*) StateObjectListFind(
                    (StateObjectList*) &(pEnumTable->ThreadEnumList),
                    (PVOID) GetCurrentThreadId());

                if (!pStateList) {
                    break;
                }

            } else {
                pNext = (KeyStateList*) (pStateList->Object.Links.Flink);
            }

            pEnumState = (EnumState*) StateObjectListRemove(
                (StateObjectList*) pStateList,
                hKey);

            if (pEnumState) {

                Status = STATUS_SUCCESS;

                EnumStateDestroy((StateObject*) pEnumState);

                //
                // Note the state list might be empty for a given thread,
                // but we will not destroy this list in order to avoid
                // excessive heap calls
                //
            }
        }
    }

    //
    // It's safe to unlock the table
    //
    Status = RtlLeaveCriticalSection(&(pEnumTable->CriticalSection));

    ASSERT( NT_SUCCESS( Status ) );
#if DBG
    if ( !NT_SUCCESS( Status ) ) {
        DbgPrint( "WINREG: RtlLeaveCriticalSection() in EnumTableRemoveKey() failed. Status = %lx \n", Status );
    }
#endif

    return Status;
}


NTSTATUS EnumTableGetNextEnum(
    EnumTable* pEnumTable,
    HKEY hKey,
    DWORD dwSubkey,
    KEY_INFORMATION_CLASS KeyInformationClass,
    PVOID pKeyInfo,
    DWORD cbKeyInfo,
    LPDWORD pcbKeyInfo)
/*++

Routine Description:

   Gets the next enumerated subkey for a
   particular subkey

Arguments:

    pEnumTable - table that holds state of
       registry key enumerations

    hKey - key for whom we want to add state

    dwSubKey - index of subkey requested by caller
        for enumeration

    KeyInformationClass - the type of key information data
        requested by caller

    pKeyInfo - out param -- buffer for key information data for caller

    cbKeyInfo - size of pKeyInfo buffer

    pcbKeyInfo - out param -- size of key information returned to caller

Return Value:

    Returns NT_SUCCESS (0) for success; error-code for failure.

Notes:

--*/
{
    EnumState* pEnumState;
    EnumState* pRootState;
    NTSTATUS   Status;
    BOOL       fFreeState;

    //
    // Protect ourselves while we enumerate
    //
    Status = RtlEnterCriticalSection(&(pEnumTable->CriticalSection));

    //
    // Very big -- unlikely to happen unless there's a runaway enumeration
    // due to a bug in this module.
    //
    // ASSERT(dwSubkey < 16383);

    ASSERT( NT_SUCCESS( Status ) );
    if ( !NT_SUCCESS( Status ) ) {
#if DBG
        DbgPrint( "WINREG: RtlEnterCriticalSection() in EnumTableGetNextENUm() failed. Status = %lx \n", Status );
#endif
        return Status;
    }

    //
    // Find the enumeration state for the requested key.  Note that even if this
    // function fails to find an existing state, which case it returns a failure code
    // it can still return an empty pEnumState for that hKey so it can be added later
    //
    Status = EnumTableGetKeyState(pEnumTable, hKey, dwSubkey, &pEnumState, &pRootState, pcbKeyInfo);

    if (!NT_SUCCESS(Status) || !pEnumState) {
        goto cleanup;
    }

    //
    // We have a state for this key, now we can use it to enumerate the next key
    //
    Status = EnumStateGetNextEnum(pEnumState, dwSubkey, KeyInformationClass, pKeyInfo, cbKeyInfo, pcbKeyInfo, &fFreeState);

    //
    // Below is an optimization for apps that enumerate HKEY_CLASSES_ROOT but close the handle and reopen it each
    // time before they call the registry enumeration api.  This is a very foolish way to use the api (that's two extra
    // kernel calls for the open and close per enumeration), but existing applications do this and 
    // without the optimization, their enumeration times can go from 3 seconds to 1 or more minutes.  With this optimization,
    // the time gets back down to a few seconds.  This happened because we lost state after the close -- when the new
    // key was opened, we had to call the kernel to enumerate all the keys up to the requested index since we had no
    // previous state to go by -- this ends up making the entire enumeration an O(n^2) operation instead of O(n) as it
    // had been when callers didn't close the key during the enumeration. Here, n is a kernel trap to enumerate a key.
    //

    //
    // Above, we retrieved an enumeration state for the root of classes -- this state reflects the enumeration state
    // of the last handle that was used to enumerate the root on this thread.  This way, when a new handle is opened
    // to enumerate the root, we start with this state which will most likely be right at the index before the requested
    // index.  Instead of making i calls to NtEnumerateKey where i is the index of enumeration requested by the caller,
    // we make 1 or at most 2 calls.
    //

    //
    // Here, we update the root state to match the recently enumerated state.  Note that this only happens
    // if the key being enumerated refers to HKEY_CLASSES_ROOT since pRootState is only non-NULL in this
    // case.
    //
    if (pRootState) {
        EnumTableUpdateRootState(pEnumTable, pRootState, pEnumState, fFreeState);
    }

    if (fFreeState) {

        NTSTATUS RemoveStatus;

        //
        // For whatever reason, we've been told to free the enumeration state for this key.
        // This could be due to an error, or it could be a normal situation such as reaching
        // the end of an enumeration.
        //

        RemoveStatus = EnumTableRemoveKey(pEnumTable, hKey, ENUM_TABLE_REMOVEKEY_CRITERIA_THISTHREAD);

        ASSERT(NT_SUCCESS(RemoveStatus));
    }

cleanup:

    //
    // It's safe to unlock the table now.
    //
    {
        NTSTATUS CriticalSectionStatus;

        CriticalSectionStatus = RtlLeaveCriticalSection(&(pEnumTable->CriticalSection));

        ASSERT( NT_SUCCESS( CriticalSectionStatus ) );
#if DBG
        if ( !NT_SUCCESS( CriticalSectionStatus ) ) {
            DbgPrint( "WINREG: RtlLeaveCriticalSection() in EnumTableGetNextEnum() failed. Status = %lx \n",
                      CriticalSectionStatus );
        }
#endif
    }

    return Status;
}


NTSTATUS EnumTableGetKeyState(
    EnumTable*  pEnumTable,
    HKEY        hKey,
    DWORD       dwSubkey,
    EnumState** ppEnumState,
    EnumState** ppRootState,
    LPDWORD     pcbKeyInfo)
/*++

Routine Description:

    Finds a key state for hKey -- creates a new state for hkey if 
    there is no existing state

Arguments:

    pEnumTable - enumeration table in which to find key's state
    hKey - handle to registry key for which to find state
    dwSubkey - subkey that we're trying to enumerate -- needed in
        case we need to create a new state
    ppEnumState - pointer to where we should return address of 
        the retrieved state,
    ppRootState - if the retrieved state is the root of the classes
        tree, this address will point to a known state for the root
        that's good across all hkey's enumerated on this thread
    pcbKeyInfo - stores size of key information on return

Return Value:

    STATUS_SUCCESS for success, other error code on error

Notes:

--*/
{
    NTSTATUS Status;

    if (ppRootState) {
        *ppRootState = NULL;
    }

    //
    // Find the enumeration state for the requested key.  Note that even if this
    // function fails to find an existing state, in which case it returns a failure code
    // it can still return an empty pEnumState for that hKey so it can be added later
    //
    Status = EnumTableFindKeyState(pEnumTable, hKey, ppEnumState);

    if (!NT_SUCCESS(Status)) {

        if (STATUS_OBJECT_NAME_NOT_FOUND == Status) {

            //
            // This means the key didn't exist, already, so we'll add it
            //
            Status = EnumTableAddKey(pEnumTable, hKey, dwSubkey, ppEnumState, ppRootState);

            if (!NT_SUCCESS(Status)) {
                return Status;
            }

            //
            // The above function can succeed but return a NULL pEnumState -- this
            // happens if it turns out this key is not a "special key" -- i.e. this key's
            // parents exist in only one hive, not two, so we don't need to do anything here
            // and regular enumeration will suffice.
            //
            if (!(*ppEnumState)) {
                //
                // We set this value to let our caller know that this isn't a class key
                //
                *pcbKeyInfo = 0;
            }
        }
    } else {

        if ((*ppEnumState)->fClassesRoot) {
            Status = EnumTableGetRootState(pEnumTable, ppRootState);
        }
    }

    return Status;
}


NTSTATUS EnumTableGetRootState(
    EnumTable*  pEnumTable,
    EnumState** ppRootState)
/*++

Routine Description:

    

Arguments:

    pEnumTable - enumeration table in which to find the root
        state
    ppRootState - points to address of root state on return

Return Value:

    Returns NT_SUCCESS (0) for success; error-code for failure.

Notes:

--*/
{
    DWORD         cbKeyInfo;
    KeyStateList* pStateList;
    
    //
    // We assume the caller has made sure that a state list
    // for this thread exists -- this should never, ever fail
    //
    pStateList = (KeyStateList*) StateObjectListFind(
        &(pEnumTable->ThreadEnumList),
        (PVOID) GetCurrentThreadId());

    ASSERT(pStateList);

    *ppRootState = &(pStateList->RootState);

    return STATUS_SUCCESS;
}


void EnumTableUpdateRootState(
    EnumTable* pEnumTable,
    EnumState* pRootState,
    EnumState* pEnumState,
    BOOL       fResetState)
/*++

Routine Description:

    Updates the state of the classes root for this thread -- this
    allows us to optimize for apps that close handles when enumerating
    hkcr -- we use this classes root state when no existing state is
    found for an hkey that refers to hkcr, and we update this state
    after enumerating an hkcr key on this thread so that it will
    be up to date.

Arguments:

    pEnumTable - enumeration table in which the classes root state resides

    pRootState - classes root state that should be updated

    ppEnumState - state that contains the data with which pRootState should
        be updated

    fResetState - if TRUE, this flag means we should not update the root state
        with pEnumState's data, just reset it.  If FALSE, we update the root
        with pEnumState's data.

Return Value:

     None.

Notes:

--*/
{
    NTSTATUS Status;

    //
    // See if we need to merely reset the root or actually
    // update it with another state
    //
    if (!fResetState) {

        //
        // Don't reset -- copy over the state from pEnumState to the
        // root state -- the root's state will be the same as pEnumState's
        // after this copy
        //
        Status = EnumStateCopy(pRootState, pEnumState);

    } else {

        //
        // Just clear out the state -- caller didn't request that we
        // use pEnumState.
        //
        Status = EnumStateInit(
            pRootState,
            0,
            0,
            ENUM_DIRECTION_FORWARD,
            NULL);
    }

    //
    // If there's a failure, it must be out-of-memory, so we should get rid
    // of this state since we can't make it accurately reflect the true
    // enumeration state
    //
    if (!NT_SUCCESS(Status)) {

#if DBG
        DbgPrint( "WINREG: failure in UpdateRootState. Status = %lx \n", Status );
#endif

        ASSERT(STATUS_NO_MEMORY == Status);

        EnumStateClear(pRootState);
    }
}


VOID KeyStateListInit(KeyStateList* pStateList)
/*++

Routine Description:

    Initializes a state list

Arguments:

    pObject -- pointer to KeyStateList object to destroy

Return Value:

    Returns NT_SUCCESS (0) for success; error-code for failure.

Notes:

--*/
{
    //
    // First initialize the base object
    //
    StateObjectListInit((StateObjectList*) pStateList,
                        (PVOID) GetCurrentThreadId());

    //
    // Now do KeyStateList specific init
    //
    (void) EnumStateInit(
        &(pStateList->RootState),
        NULL,
        0,
        ENUM_DIRECTION_FORWARD,
        NULL);
}

VOID KeyStateListDestroy(StateObject* pObject)
/*++

Routine Description:

    Destroys an KeyStateList, freeing its resources such
        as memory or kernel object handles

Arguments:

    pObject -- pointer to KeyStateList object to destroy

Return Value:

    Returns NT_SUCCESS (0) for success; error-code for failure.

Notes:

--*/
{
    KeyStateList* pThisList;

    pThisList = (KeyStateList*) pObject;

    //
    // Destroy all states in this list
    //
    StateObjectListClear(
        (StateObjectList*) pObject,
        EnumStateDestroy);

    //
    // Free resources associated with the root state
    //
    EnumStateClear(&(pThisList->RootState));

    //
    // Free the data structure for this object
    //
    RegClassHeapFree(pThisList);
} 


NTSTATUS EnumStateInit(
    EnumState*     pEnumState,
    HKEY           hKey,
    DWORD          dwFirstSubKey,
    DWORD          dwDirection,
    SKeySemantics* pKeySemantics)
/*++

Routine Description:

    Initializes enumeration state

Arguments:

    pEnumState - enumeration state to initialize
    hKey       - registry key to which this state refers
    dwFirstSubKey - index of the first subkey which this state will enumerate
    dwDirection - direction through which we should enumerate -- either
        ENUM_DIRECTION_FORWARD or ENUM_DIRECTION_BACKWARD
    pKeySemantics - structure containing information about hKey

Return Value:

    Returns NT_SUCCESS (0) for success; error-code for failure.

--*/
{
    NTSTATUS Status;
    ULONG    cMachineKeys;
    ULONG    cUserKeys;
    HKEY     hkOther;

    ASSERT((ENUM_DIRECTION_FORWARD == dwDirection) || (ENUM_DIRECTION_BACKWARD == dwDirection) ||
        (ENUM_DIRECTION_IGNORE == dwDirection));

    ASSERT((ENUM_DIRECTION_IGNORE == dwDirection) ? hKey == NULL : TRUE);

    Status = STATUS_SUCCESS;

    hkOther = NULL;

    //
    // If no hkey is specified, this is an init of a blank enum
    // state, so clear everything
    //
    if (!hKey) {
        memset(pEnumState, 0, sizeof(*pEnumState));
    }

    //
    // Clear each subtree
    //
    EnumSubtreeStateClear(&(pEnumState->UserState));
    EnumSubtreeStateClear(&(pEnumState->MachineState));

    //
    // Reset each subtree
    //
    pEnumState->UserState.Finished = FALSE;
    pEnumState->MachineState.Finished = FALSE;

    pEnumState->UserState.iSubKey = 0;
    pEnumState->MachineState.iSubKey = 0;

    cUserKeys = 0;
    cMachineKeys = 0;

    if (pKeySemantics) {
        StateObjectInit((StateObject*) &(pEnumState->Object), hKey);
    }

    if (hKey) {

        if (pKeySemantics) {
            pEnumState->fClassesRoot = IsRootKey(pKeySemantics);
        }

        //
        // open the other key if we have enough info to do so --
        //
        if (pKeySemantics) {

            //
            // Remember, only one of the handles returned below
            // is new -- the other is simply hKey
            //
            Status = BaseRegGetUserAndMachineClass(
                pKeySemantics,
                hKey,
                MAXIMUM_ALLOWED,
                &(pEnumState->hkMachineKey),
                &(pEnumState->hkUserKey));

            if (!NT_SUCCESS(Status)) {
                return Status;
            }
        }
         
        //
        // for backwards enumerations
        //
        if (ENUM_DIRECTION_BACKWARD == dwDirection) {

            ULONG             cMachineKeys;
            ULONG             cUserKeys;
            HKEY              hkUser;
            HKEY              hkMachine;
            
            cMachineKeys = 0;
            cUserKeys = 0;
            
            hkMachine = pEnumState->hkMachineKey;
            hkUser = pEnumState->hkUserKey;

            //
            // In order to query for subkey counts, we should
            // to get a new handle since the caller supplied handle
            // may not have enough permissions
            //
            {
                HKEY   hkSource;
                HANDLE hCurrentProcess;

                hCurrentProcess = NtCurrentProcess();

                hkSource = (hkMachine == hKey) ? hkMachine : hkUser;
                
                Status = NtDuplicateObject(
                    hCurrentProcess,
                    hkSource,
                    hCurrentProcess,
                    &hkOther,
                    KEY_QUERY_VALUE,
                    FALSE,
                    0);

                if (!NT_SUCCESS(Status)) {
                    goto error_exit;
                }

                if (hkSource == hkUser) {
                    hkUser = hkOther;
                } else {
                    hkMachine = hkOther;
                }
            }

            //
            // find new start -- query for index of last subkey in
            // each hive 
            //
            if (hkMachine) {

                Status = GetSubKeyCount(hkMachine, &cMachineKeys);
            
                if (!NT_SUCCESS(Status)) {
                    goto error_exit;
                }
            }

            if (hkUser) {

                Status = GetSubKeyCount(hkUser, &cUserKeys);
                
                if (!NT_SUCCESS(Status)) {
                    goto error_exit;
                }
            }

            //
            // If either subtree has no subkeys, we're done enumerating that
            // subtree
            //
            if (!cUserKeys) {
                pEnumState->UserState.Finished = TRUE;
            } else {
                pEnumState->UserState.iSubKey = cUserKeys - 1;
            }

            if (!cMachineKeys) {
                pEnumState->MachineState.Finished = TRUE;
            } else {
                pEnumState->MachineState.iSubKey = cMachineKeys - 1;
            }
        }
    }
  
    //
    // Set members of this structure
    //
        
    pEnumState->dwThreadId = GetCurrentThreadId();
    pEnumState->Direction = dwDirection;
    pEnumState->dwLastRequest = dwFirstSubKey;
    pEnumState->LastLocation = ENUM_LOCATION_NONE;
        
    pEnumState->hKey = hKey;

error_exit:

    if (!NT_SUCCESS(Status)) {
        EnumSubtreeStateClear(&(pEnumState->MachineState));
        EnumSubtreeStateClear(&(pEnumState->UserState));
    }

    if (hkOther) {
        NtClose(hkOther);
    }

    return Status;
}


NTSTATUS EnumStateGetNextEnum(
    EnumState*            pEnumState,
    DWORD                 dwSubKey,
    KEY_INFORMATION_CLASS KeyInformationClass,
    PVOID                 pKeyInfo,
    DWORD                 cbKeyInfo,
    LPDWORD               pcbKeyInfo,
    BOOL*                 pfFreeState)
/*++

Routine Description:

    Gets the next key in an enumeration based on the current state.

Arguments:

    pEnumState - enumeration state on which to base our search
                 for the next key
    dwSubKey   - index of key to enumerate
    KeyInformationClass - enum for what sort of information to retrieve in the
         enumeration -- Basic Information or Node Information

    pKeyInfo   - location to store retrieved data for caller
    cbKeyInfo  - size of caller's info buffer
    pcbKeyInfo - size of data this function writes to buffer on return.
    pfFreeState - out param -- if set to TRUE, caller should free pEnumState.

Return Value:

    Returns NT_SUCCESS (0) for success; error-code for failure.

Notes:

    This function essentially enumerates from the previous index requested
    by the caller of RegEnumKeyEx to the new one. In most cases, this just
    means one trip to the kernel -- i.e. if a caller goes from index 2 to 3,
    or from 3 to 2, this is one trip to the kernel.  However, if the caller goes
    from 2 to 5, we'll have to do several enumerations on the way from 2 to 5.
    Also, if the caller switches direction (i.e. starts off 0,1,2,3 and then
    requests 1), a large penalty may be incurred.  When switching from ascending
    to descending, we have to enumerate all keys to the end and then before we
    can then enumerate down to the caller's requested index.  Switching from
    descending to ascending is less expensive -- we know that the beginning
    is at 0 for both user and machine keys, so we can simply set our indices to
    0 without enumerating anything.  However, we must then enumerate to the
    caller's requested index.  Note that for all descending enumerations, we
    must enumerate all the way to the end first before returning anything to the
    caller.

--*/
{
    NTSTATUS          Status;
    LONG              lIncrement;
    DWORD             dwStart;
    DWORD             dwLimit;
    EnumSubtreeState* pTreeState;

    //
    // If anything bad happens, this state should be freed
    //
    *pfFreeState = TRUE;

    //
    // Find out the limits (start, finish, increment) for
    // our enumeration. The increment is either 1 or -1,
    // depending on whether this is an ascending or descending
    // enumeration.  EnumStateSetLimits will take into account
    // any changes in direction and set dwStart and dwLimit
    // accordingly.
    //
    Status = EnumStateSetLimits(
        pEnumState,
        dwSubKey,
        &dwStart,
        &dwLimit,
        &lIncrement);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Get the next enum to give back to the caller
    //
    Status = EnumStateChooseNext(
        pEnumState,
        dwSubKey,
        dwStart,
        dwLimit,
        lIncrement,
        &pTreeState);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // We have retrieved information, so we should
    // not free this state
    //
    if (!(pEnumState->UserState.Finished && pEnumState->MachineState.Finished)) {
        *pfFreeState = FALSE;
    }

    //
    // Remember the last key we enumerated
    //
    pEnumState->dwLastRequest = dwSubKey;

    //
    // Copy the retrieved information to the user's
    // buffer.
    //
    Status = EnumSubtreeStateCopyKeyInfo(
        pTreeState,
        KeyInformationClass,
        pKeyInfo,
        cbKeyInfo,
        pcbKeyInfo);

    //
    // The copy could fail if the user's buffer isn't big enough --
    // if it succeeds, clear the name information for the subkey from
    // which we retrieved the data so that the next time we're called
    // we'll get the next subkey for that subtree.
    //
    if (NT_SUCCESS(Status)) {
        EnumSubtreeStateClear(pTreeState);
    }

    return Status;
}


NTSTATUS EnumStateSetLimits(
    EnumState*   pEnumState,
    DWORD        dwSubKey,
    LPDWORD      pdwStart,
    LPDWORD      pdwLimit,
    PLONG        plIncrement)
/*++

Routine Description:

    Gets the limits (start, finish, increment) for enumerating a given
    subkey index

Arguments:

    pEnumState - enumeration state on which to base our limits

    dwSubKey   - index of key which caller wants enumerated

    pdwStart   - out param -- result is the place at which to start
                 enumerating in order to find dwSubKey

    pdwLimit   - out param -- result is the place at which to stop
                 enumerating when looking for dwSubKey

    plIncrement - out param -- increment to use for enumeration. It will
               be set to 1 if the enumeration is upward (0,1,2...) or
               -1 if it is downard (3,2,1,...).

Return Value:

    Returns NT_SUCCESS (0) for success; error-code for failure.

Notes:

--*/
{
    LONG     lNewIncrement;
    NTSTATUS Status;
    BOOL     fSameKey;

    //
    // set our increment to the direction which our state remembers
    //
    *plIncrement = pEnumState->Direction == ENUM_DIRECTION_FORWARD ? 1 : -1;

    fSameKey = FALSE;

    //
    // Figure out what the new direction should be
    // This is done by comparing the current request
    // with the last request.
    //
    if (dwSubKey > pEnumState->dwLastRequest) {
        lNewIncrement = 1;
    } else if (dwSubKey < pEnumState->dwLastRequest) {
        lNewIncrement = -1;
    } else {
        //
        // We are enumerating a key that may already
        // have been enumerated
        //
        fSameKey = TRUE;
        lNewIncrement = *plIncrement;
    }

    //
    // See if we've changed direction
    //
    if (lNewIncrement != *plIncrement) {

        //
        // If so, we should throw away all existing state and start from scratch
        //
        Status = EnumStateInit(
            pEnumState,
            pEnumState->hKey,
            (-1 == lNewIncrement) ? dwSubKey : 0,
            (-1 == lNewIncrement) ? ENUM_DIRECTION_BACKWARD : ENUM_DIRECTION_FORWARD,
            NULL);

        if (!NT_SUCCESS(Status)) {
            return Status;
        }
    }

    //
    // By default, we start enumerating where we left off
    //
    *pdwStart = pEnumState->dwLastRequest;

    //
    // for state for which we have previously enumerated a key
    //
    if (ENUM_LOCATION_NONE != pEnumState->LastLocation) {

        //
        // We're going in the same direction as on the
        // previous call. We should start
        // one past our previous position.  Note that we
        // only start there if this is a different key --
        // if we've already enumerated it we start at the
        // same spot.
        //
        if (!fSameKey) {
            *pdwStart += *plIncrement;
        } else {
            
            // 
            // If we're being asked for the same index
            // multiple times they're probably deleting
            // keys -- we should reset ourselves to
            // the beginning so their enum will hit
            // all the keys
            //

            //
            // We're starting at zero, so set ourselves
            // to start at the beginning
            //
            Status = EnumStateInit(
                pEnumState,
                pEnumState->hKey,
                0,
                ENUM_DIRECTION_FORWARD,
                NULL);
            
            if (!NT_SUCCESS(Status)) {
                return Status;
            }
            
            *plIncrement = 1;
            pEnumState->Direction = ENUM_DIRECTION_FORWARD;
            *pdwStart = 0;
        }

    } else {

        //
        // No previous calls were made for this state
        //
        if (ENUM_DIRECTION_BACKWARD == pEnumState->Direction) {

            //
            // For backwards enumerations, we want to get an
            // accurate count of total subkeys and start there
            //
            Status = ClassKeyCountSubKeys(
                pEnumState->hKey,
                pEnumState->hkUserKey,
                pEnumState->hkMachineKey,
                0,
                pdwStart);

            if (!NT_SUCCESS(Status)) {
                return Status;
            }

            //
            // Make sure we don't go past the end
            //
            if (dwSubKey >= *pdwStart) {
                return STATUS_NO_MORE_ENTRIES;
            }

            //
            // This is a zero-based index, so to
            // put our start at the very end we must
            // be one less than the number of keys
            //
            (*pdwStart)--;

            *plIncrement = -1;

        } else {
            *plIncrement = 1;
        }
    }

    //
    // Set limit to be one past requested subkey
    //
    *pdwLimit = dwSubKey + *plIncrement;

    return STATUS_SUCCESS;
}


NTSTATUS EnumStateChooseNext(
    EnumState*         pEnumState,
    DWORD              dwSubKey,
    DWORD              dwStart,
    DWORD              dwLimit,
    LONG               lIncrement,
    EnumSubtreeState** ppTreeState)
/*++

Routine Description:

    Iterates through registry keys to get the key requested by the caller

Arguments:

    pEnumState - enumeration state on which to base our search

    dwSubKey   - index of key which caller wants enumerated

    dwStart   - The place at which to start
                enumerating in order to find dwSubKey

    dwLimit   - The place at which to stop
                enumerating when looking for dwSubKey

    lIncrement - Increment to use for enumeration. It will
               be set to 1 if the enumeration is upward (0,1,2...) or
               -1 if it is downard (3,2,1,...).

    ppTreeState - out param -- pointer to address of subtree state in which this regkey
                  was found -- each EnumState has two EnumSubtreeState's -- one for user
                  and one for machine.

Return Value:

    Returns NT_SUCCESS (0) for success; error-code for failure.

Notes:

--*/
{
    DWORD    iCurrent;
    NTSTATUS Status;
    BOOL     fClearLast;

    Status = STATUS_NO_MORE_ENTRIES;

    fClearLast = FALSE;

    //
    // We will now iterate from dwStart to dwLimit so that we can find the key
    // requested by the caller
    //
    for (iCurrent = dwStart; iCurrent != dwLimit; iCurrent += lIncrement) {

        BOOL fFoundKey;
        BOOL fIgnoreFailure;

        fFoundKey = FALSE;

        fIgnoreFailure = FALSE;

        Status = STATUS_NO_MORE_ENTRIES;

        //
        // Clear last subtree
        //
        if (fClearLast) {
            EnumSubtreeStateClear(*ppTreeState);
        }

        //
        // if key names aren't present, alloc space and get names
        //
        if (pEnumState->hkUserKey) {
            if (pEnumState->UserState.pKeyInfo) {
                fFoundKey = TRUE;
            } else if (!(pEnumState->UserState.Finished)) {

                // get user key info
                Status = EnumClassKey(
                    pEnumState->hkUserKey,
                    &(pEnumState->UserState));

                fFoundKey = NT_SUCCESS(Status);

                //
                // If there are no more subkeys for this subtree,
                // mark it as finished
                //
                if (!NT_SUCCESS(Status)) {

                    if (STATUS_NO_MORE_ENTRIES != Status) {
                        return Status;
                    }

                    if (lIncrement > 0) {
                        pEnumState->UserState.Finished = TRUE;
                    } else {

                        pEnumState->UserState.iSubKey += lIncrement;
                        fIgnoreFailure = TRUE;
                    }
                }
            }
        }

        if (pEnumState->hkMachineKey) {

            if (pEnumState->MachineState.pKeyInfo) {
                fFoundKey = TRUE;
            } else if (!(pEnumState->MachineState.Finished)) {

                // get machine key info
                Status = EnumClassKey(
                    pEnumState->hkMachineKey,
                    &(pEnumState->MachineState));

                //
                // If there are no more subkeys for this subtree,
                // mark it as finished
                //
                if (NT_SUCCESS(Status)) {
                    fFoundKey = TRUE;
                } else if (STATUS_NO_MORE_ENTRIES == Status) {

                    if (lIncrement > 0) {
                        pEnumState->MachineState.Finished = TRUE;
                    } else {
                        pEnumState->MachineState.iSubKey += lIncrement;
                        fIgnoreFailure = TRUE;
                    }
                }
            }
        }

        //
        // If we found no keys in either user or machine locations, there are
        // no more keys.
        //
        if (!fFoundKey) {

            //
            // For descending enumerations, we ignore STATUS_NO_MORE_ENTRIES
            // and keep going until we find one.
            //
            if (fIgnoreFailure) {
                continue;
            }

            return Status;
        }

        //
        // If we already hit the bottom, skip to the end
        //
        if ((pEnumState->UserState.iSubKey == 0) &&
            (pEnumState->MachineState.iSubKey == 0) &&
            (lIncrement < 0)) {
            
            iCurrent = dwLimit - lIncrement;
        }

        //
        // Now we need to choose between keys in the machine hive and user hives --
        // this call will choose which key to use.
        //
        Status = EnumStateCompareSubtrees(pEnumState, lIncrement, ppTreeState);

        if (!NT_SUCCESS(Status)) {

            pEnumState->dwLastRequest = dwSubKey;

            return Status;
        }
        
        fClearLast = TRUE;

    }

    return Status;
}


NTSTATUS EnumStateCompareSubtrees(
    EnumState*         pEnumState,
    LONG               lIncrement,
    EnumSubtreeState** ppSubtree)
/*++

Routine Description:

    Compares the user and machine subtrees of an enumeration state
    to see which of the two current keys in each hive should be
    returned as the next key in an enumeration

Arguments:

    pEnumState - enumeration state on which to base our search

    lIncrement - Increment to use for enumeration. It will
               be set to 1 if the enumeration is upward (0,1,2...) or
               -1 if it is downard (3,2,1,...).

    ppSubtree - out param -- pointer to address of subtree state where
                key was found -- the name of the key can be extracted from it.

Return Value:

    Returns NT_SUCCESS (0) for success; error-code for failure.

Notes:

--*/
{
    //
    // If both subtrees have a current subkey name, we'll need to compare
    // the names
    //
    if (pEnumState->MachineState.pKeyInfo && pEnumState->UserState.pKeyInfo) {

        UNICODE_STRING MachineKeyName;
        UNICODE_STRING UserKeyName;
        LONG           lCompareResult;

        MachineKeyName.Buffer = pEnumState->MachineState.pKeyInfo->Name;
        MachineKeyName.Length = (USHORT) pEnumState->MachineState.pKeyInfo->NameLength;

        UserKeyName.Buffer = pEnumState->UserState.pKeyInfo->Name;
        UserKeyName.Length = (USHORT) pEnumState->UserState.pKeyInfo->NameLength;

        //
        // Do the comparison
        //
        lCompareResult =
            RtlCompareUnicodeString(&UserKeyName, &MachineKeyName, TRUE) * lIncrement;

        //
        // User wins comparison
        //
        if (lCompareResult < 0) {
            // choose user
            *ppSubtree = &(pEnumState->UserState);
            pEnumState->LastLocation = ENUM_LOCATION_USER;

        } else if (lCompareResult > 0) {

            //
            // Machine wins choose machine
            //
            *ppSubtree = &(pEnumState->MachineState);
            pEnumState->LastLocation = ENUM_LOCATION_MACHINE;

        } else {

            //
            // Comparison returned equality -- the keys have the same
            // name.  This means the same key name exists in both machine and
            // user, so we need to make a choice about which one we will enumerate.
            // Policy for per-user class registration enumeration is to choose user, just
            // as we do for other api's such as RegOpenKeyEx and RegCreateKeyEx.
            //
            if (!((pEnumState->MachineState.iSubKey == 0) && (lIncrement < 0))) {
                pEnumState->MachineState.iSubKey += lIncrement;
            } else {
                pEnumState->MachineState.Finished = TRUE;
            }

            //
            // Clear the machine state and move it to the next index -- we don't
            // have to clear the user state yet because the state of whichever subtree
            // was selected is cleared down below
            //
            EnumSubtreeStateClear(&(pEnumState->MachineState));
            pEnumState->LastLocation = ENUM_LOCATION_USER;
            *ppSubtree = &(pEnumState->UserState);
        }

    } else if (!(pEnumState->UserState.pKeyInfo) && !(pEnumState->MachineState.pKeyInfo)) {
        //
        // Neither subtree state has a subkey, so there are no subkeys
        //
        return STATUS_NO_MORE_ENTRIES;

    } else if (pEnumState->MachineState.pKeyInfo) {

        //
        // Only machine has a subkey
        //
        *ppSubtree = &(pEnumState->MachineState);
        pEnumState->LastLocation = ENUM_LOCATION_MACHINE;

    } else {

        //
        // only user has a subkey
        //
        *ppSubtree = &(pEnumState->UserState);
        pEnumState->LastLocation = ENUM_LOCATION_USER;
    }

    //
    // change the state of the subtree which we selected
    //
    if (!(((*ppSubtree)->iSubKey == 0) && (lIncrement < 0))) {
        (*ppSubtree)->iSubKey += lIncrement;
    } else {
        (*ppSubtree)->Finished = TRUE;
    }

    return STATUS_SUCCESS;
}

void EnumStateDestroy(StateObject* pObject)
{
    EnumStateClear((EnumState*) pObject);

    RegClassHeapFree(pObject);
}

VOID EnumStateClear(EnumState* pEnumState)
/*++

Routine Description:

    Clears the enumeration state

Arguments:

    pEnumState - enumeration state to clear

Return Value:

    Returns NT_SUCCESS (0) for success; error-code for failure.

Notes:

--*/
{
    //
    // Close an existing reference to a second key
    //
    if (pEnumState->hkMachineKey && (pEnumState->hKey != pEnumState->hkMachineKey)) {

        NtClose(pEnumState->hkMachineKey);

    } else if (pEnumState->hkUserKey && (pEnumState->hKey != pEnumState->hkUserKey)) {
        
        NtClose(pEnumState->hkUserKey);
    }

    //
    // Free any heap memory held by our subtrees
    //
    EnumSubtreeStateClear(&(pEnumState->UserState));
    EnumSubtreeStateClear(&(pEnumState->MachineState));

    //
    // reset everything in this state
    //
    memset(pEnumState, 0, sizeof(*pEnumState));
}


BOOL EnumStateIsEmpty(EnumState* pEnumState)
/*++

Routine Description:

    Returns whether or not an enumeration state is empty.
    An enumeration state is empty if it is not associated
    with any particular registry key handle

Arguments:

    pEnumState - enumeration state to clear

Return Value:

    Returns NT_SUCCESS (0) for success; error-code for failure.

Notes:

--*/
{
    return pEnumState->hKey == NULL;
}

NTSTATUS EnumStateCopy(
    EnumState*            pDestState,
    EnumState*            pEnumState)
/*++

Routine Description:

    Copies an enumeration state for one hkey
    to the state for another hkey -- note that it the 
    does not change the hkey referred to by the destination
    state, it just makes pDestState->hKey's state the
    same as pEnumState's

Arguments:

    pDestState - enumeration state which is destination
        of the copy
    pEnumState - source enumeration for the copy

Return Value:

    STATUS_SUCCESS for success, other error code on error

Notes:

--*/
{
    NTSTATUS Status;

    PKEY_NODE_INFORMATION pKeyInfoUser;
    PKEY_NODE_INFORMATION pKeyInfoMachine;

    Status = STATUS_SUCCESS;

    //
    // Copy simple data
    //
    pDestState->Direction = pEnumState->Direction;
    pDestState->LastLocation = pEnumState->LastLocation;

    pDestState->dwLastRequest = pEnumState->dwLastRequest;
    pDestState->dwThreadId = pEnumState->dwThreadId;

    //
    // Free existing data before we overwrite it -- note that the pKeyInfo can point to a fixed buffer inside the state or 
    // a heap allocated buffer, so we must see which one it points to before we decide to free it
    //
    if (pDestState->UserState.pKeyInfo &&
        (pDestState->UserState.pKeyInfo != (PKEY_NODE_INFORMATION) pDestState->UserState.KeyInfoBuffer)) {
        RegClassHeapFree(pDestState->UserState.pKeyInfo);
        pDestState->UserState.pKeyInfo = NULL;
    }

    if (pDestState->MachineState.pKeyInfo &&
        (pDestState->MachineState.pKeyInfo != (PKEY_NODE_INFORMATION) pDestState->MachineState.KeyInfoBuffer)) {
        RegClassHeapFree(pDestState->MachineState.pKeyInfo);
        pDestState->MachineState.pKeyInfo = NULL;
    }

    //
    // easy way to copy states -- we'll have to fix up below though since pKeyInfo can be
    // self-referential.
    //
    memcpy(&(pDestState->UserState), &(pEnumState->UserState), sizeof(pEnumState->UserState));
    memcpy(&(pDestState->MachineState), &(pEnumState->MachineState), sizeof(pEnumState->MachineState));

    pKeyInfoUser = NULL;
    pKeyInfoMachine = NULL;
        
    //
    // Copy new data -- as above, keep in mind that pKeyInfo can be self-referential, so check
    // for that before deciding whether to allocate heap or use the internal fixed buffer of the
    // structure.
    //
    if (pEnumState->UserState.pKeyInfo &&
        ((pEnumState->UserState.pKeyInfo != (PKEY_NODE_INFORMATION) pEnumState->UserState.KeyInfoBuffer))) {

        pKeyInfoUser = (PKEY_NODE_INFORMATION) 
            RegClassHeapAlloc(pEnumState->UserState.cbKeyInfo);

        if (!pKeyInfoUser) {
            Status = STATUS_NO_MEMORY;
        }

        pDestState->UserState.pKeyInfo = pKeyInfoUser;

        RtlCopyMemory(pDestState->UserState.pKeyInfo,
                      pEnumState->UserState.pKeyInfo,
                      pEnumState->UserState.cbKeyInfo);
    } else {
        if (pDestState->UserState.pKeyInfo) {
            pDestState->UserState.pKeyInfo = (PKEY_NODE_INFORMATION) pDestState->UserState.KeyInfoBuffer;
        }
    }
    
    if (pEnumState->MachineState.pKeyInfo &&
        ((pEnumState->MachineState.pKeyInfo != (PKEY_NODE_INFORMATION) pEnumState->MachineState.KeyInfoBuffer))) {
      
        pKeyInfoMachine = (PKEY_NODE_INFORMATION) 
            RegClassHeapAlloc(pEnumState->MachineState.cbKeyInfo);

        if (!pKeyInfoMachine) {
            Status = STATUS_NO_MEMORY;
        }

        pDestState->MachineState.pKeyInfo = pKeyInfoMachine;

        RtlCopyMemory(pDestState->MachineState.pKeyInfo,
                      pEnumState->MachineState.pKeyInfo,
                      pEnumState->MachineState.cbKeyInfo);
    } else {
        if (pDestState->MachineState.pKeyInfo) {
            pDestState->MachineState.pKeyInfo = (PKEY_NODE_INFORMATION) pDestState->MachineState.KeyInfoBuffer;
        }
    }

    //
    // On error, make sure we clean up.
    // 
    if (!NT_SUCCESS(Status)) {

        if (pKeyInfoUser) {
            RegClassHeapFree(pKeyInfoUser);
        }

        if (pKeyInfoMachine) {
            RegClassHeapFree(pKeyInfoMachine);
        }
    }

    return Status;
}


void EnumSubtreeStateClear(EnumSubtreeState* pTreeState)
/*++
Routine Description:

    This function frees the key data associated with this
    subtree state

Arguments:

    pTreeState -- tree state to clear

Return Value: None.

    Note:

--*/

{
    //
    // see if we're using pre-alloced buffer -- if not, free it
    //
    if (pTreeState->pKeyInfo && (((LPBYTE) pTreeState->pKeyInfo) != pTreeState->KeyInfoBuffer)) {

        RegClassHeapFree(pTreeState->pKeyInfo);
    }

    pTreeState->pKeyInfo = NULL;
}

NTSTATUS EnumSubtreeStateCopyKeyInfo(
    EnumSubtreeState* pTreeState,
    KEY_INFORMATION_CLASS KeyInformationClass,
    PVOID pDestKeyInfo,
    ULONG cbDestKeyInfo,
    PULONG pcbResult)
/*++

Routine Description:

    Copies information about a key into a buffer supplied by the caller

Arguments:

    pTreeState - subtree tate from which to copy

    KeyInformationClass - the type of buffer supplied by the caller -- either
        a KEY_NODE_INFORMATION or KEY_BASIC_INFORMATION structure

    pDestKeyInfo - caller's buffer for key information

    cbDestKeyInfo - size of caller's buffer

    pcbResult - out param -- amount of data to be written to caller's buffer

Return Value:

    Returns NT_SUCCESS (0) for success; error-code for failure.

Notes:

--*/
{
    ULONG cbNeeded;

    ASSERT((KeyInformationClass == KeyNodeInformation) ||
           (KeyInformationClass == KeyBasicInformation));

    //
    // Find out how big the caller's buffer needs to be.  This
    // depends on whether the caller specified full or node information
    // as well as the size of the variable size members of those
    // structures
    //

    if (KeyNodeInformation == KeyInformationClass) {

        PKEY_NODE_INFORMATION pNodeInformation;

        //
        // Copy fixed length pieces first -- caller expects them to
        // be set even when the variable length members are not large enough
        //

        //
        // Set ourselves to point to caller's buffer
        //
        pNodeInformation = (PKEY_NODE_INFORMATION) pDestKeyInfo;

        //
        // Copy all fixed-length pieces of structure
        //
        pNodeInformation->LastWriteTime = pTreeState->pKeyInfo->LastWriteTime;
        pNodeInformation->TitleIndex = pTreeState->pKeyInfo->TitleIndex;
        pNodeInformation->ClassOffset = pTreeState->pKeyInfo->ClassOffset;
        pNodeInformation->ClassLength = pTreeState->pKeyInfo->ClassLength;
        pNodeInformation->NameLength = pTreeState->pKeyInfo->NameLength;

        //
        // Take care of the size of the node information structure
        //
        cbNeeded = sizeof(KEY_NODE_INFORMATION);

        if (cbDestKeyInfo < cbNeeded) {
            return STATUS_BUFFER_TOO_SMALL;
        }

        //
        // Add in the size of the variable length members
        //
        cbNeeded += pTreeState->pKeyInfo->NameLength;
        cbNeeded += pTreeState->pKeyInfo->ClassLength;
        cbNeeded -= sizeof(WCHAR); // the structure's Name member is already set to 1,
                                   // so that one has already been accounted for in
                                   // the size of the structure

    } else {

        PKEY_BASIC_INFORMATION pBasicInformation;

        //
        // Copy fixed length pieces first -- caller expects them to
        // be set even when the variable length members are not large enough
        //

        //
        // Set ourselves to point to caller's buffer
        //
        pBasicInformation = (PKEY_BASIC_INFORMATION) pDestKeyInfo;

        //
        // Copy all fixed-length pieces of structure
        //
        pBasicInformation->LastWriteTime = pTreeState->pKeyInfo->LastWriteTime;
        pBasicInformation->TitleIndex = pTreeState->pKeyInfo->TitleIndex;
        pBasicInformation->NameLength = pTreeState->pKeyInfo->NameLength;


        cbNeeded = sizeof(KEY_BASIC_INFORMATION);

        //
        // Take care of the size of the basic information structure
        //
        if (cbDestKeyInfo < cbNeeded) {
            return STATUS_BUFFER_TOO_SMALL;
        }

        //
        // Add in the size of the variable length members
        //
        cbNeeded += pTreeState->pKeyInfo->NameLength;
        cbNeeded -= sizeof(WCHAR); // the structure's Name member is already set to 1,
                                   // so that one has already been accounted for in
                                   // the size of the structure
    }

    //
    // Store the amount needed for the caller
    //
    *pcbResult = cbNeeded;

    //
    // See if the caller supplied enough buffer -- leave if not
    //
    if (cbDestKeyInfo < cbNeeded) {
        return STATUS_BUFFER_OVERFLOW;
    }

    //
    // We copy variable-length information differently depending
    // on which type of structure was passsed in
    //
    if (KeyNodeInformation == KeyInformationClass) {

        PBYTE                 pDestClass;
        PBYTE                 pSrcClass;
        PKEY_NODE_INFORMATION pNodeInformation;

        pNodeInformation = (PKEY_NODE_INFORMATION) pDestKeyInfo;

        //
        // Copy variable length pieces such as name and class
        //
        RtlCopyMemory(pNodeInformation->Name,
                      pTreeState->pKeyInfo->Name,
                      pTreeState->pKeyInfo->NameLength);

        //
        // Only copy the class if it exists
        //
        if (pTreeState->pKeyInfo->ClassOffset >= 0) {
            pDestClass = ((PBYTE) pNodeInformation) + pTreeState->pKeyInfo->ClassOffset;
            pSrcClass = ((PBYTE) pTreeState->pKeyInfo) + pTreeState->pKeyInfo->ClassOffset;
            RtlCopyMemory(pDestClass, pSrcClass, pTreeState->pKeyInfo->ClassLength);
        }

    } else {

        PKEY_BASIC_INFORMATION pBasicInformation;

        //
        // Set ourselves to point to caller's buffer
        //
        pBasicInformation = (PKEY_BASIC_INFORMATION) pDestKeyInfo;

        //
        // Copy variable length pieces -- only name is variable length
        //
        RtlCopyMemory(pBasicInformation->Name,
                      pTreeState->pKeyInfo->Name,
                      pTreeState->pKeyInfo->NameLength);

    }

    return STATUS_SUCCESS;
}



NTSTATUS EnumClassKey(
    HKEY              hKey,
    EnumSubtreeState* pTreeState)
/*++

Routine Description:

    Enumerates a subkey for a subtree state -- calls the kernel

Arguments:

    hKey - key we want the kernel to enumerate
    pTreeState - subtree state -- either a user or machine subtree

Return Value:

    Returns NT_SUCCESS (0) for success; error-code for failure.

Notes:

--*/
{
    PKEY_NODE_INFORMATION pCurrentKeyInfo;
    NTSTATUS Status;

    ASSERT(!(pTreeState->pKeyInfo));

    //
    // First try to use the buffer built in to the subtree state
    //
    pCurrentKeyInfo = (PKEY_NODE_INFORMATION) pTreeState->KeyInfoBuffer;

    //
    // Query for the necessary information about the supplied key.
    //

    Status = NtEnumerateKey( hKey,
                             pTreeState->iSubKey,
                             KeyNodeInformation,
                             pCurrentKeyInfo,
                             sizeof(pTreeState->KeyInfoBuffer),
                             &(pTreeState->cbKeyInfo));

    ASSERT( Status != STATUS_BUFFER_TOO_SMALL );

    //
    // If the subtree state's buffer isn't big enough, we'll have
    // to ask the heap to give us one.
    //
    if (STATUS_BUFFER_OVERFLOW == Status) {

        pCurrentKeyInfo = RegClassHeapAlloc(pTreeState->cbKeyInfo);
        //
        // If the memory allocation fails, return a Registry Status.
        //
        if( ! pCurrentKeyInfo ) {
            return STATUS_NO_MEMORY;
        }

        //
        // Query for the necessary information about the supplied key.
        //

        Status = NtEnumerateKey( hKey,
                                 pTreeState->iSubKey,
                                 KeyNodeInformation,
                                 pCurrentKeyInfo,
                                 pTreeState->cbKeyInfo,
                                 &(pTreeState->cbKeyInfo));

    }

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // set the subtree state's reference to point
    // to the location of the data
    //
    pTreeState->pKeyInfo = pCurrentKeyInfo;

    return STATUS_SUCCESS;
}


NTSTATUS GetSubKeyCount(
    HKEY    hkClassKey,
    LPDWORD pdwUserSubKeys)
/*++

Routine Description:

    Counts the number of subkeys under a key

Arguments:

    hkClassKey - key whose subkeys we wish to count
    pdwUserSubKeys - out param for number of subkeys

Return Value:

    Returns NT_SUCCESS (0) for success; error-code for failure.

Notes:

--*/
{
    NTSTATUS              Status;
    PKEY_FULL_INFORMATION KeyFullInfo;
    ULONG                 BufferLength;
    BYTE                  PrivateKeyFullInfo[ sizeof( KEY_FULL_INFORMATION ) +
                                            ENUM_DEFAULT_CLASS_SIZE ];

    //
    // Initialize out params
    //
    *pdwUserSubKeys = 0;

    //
    // BUGBUG: don't need to pass in everything here
    //

    //
    // Set up to query kernel for subkey information
    //
    KeyFullInfo = (PKEY_FULL_INFORMATION) PrivateKeyFullInfo;
    BufferLength = sizeof(PrivateKeyFullInfo);

    Status = QueryKeyInfo(
                hkClassKey,
                &KeyFullInfo,
                BufferLength,
                FALSE,
                (USHORT) (BufferLength - sizeof(PKEY_FULL_INFORMATION))
                );

    if (NT_SUCCESS(Status)) {
        //
        // set the out param with the subkey data from the kernel call
        //
        *pdwUserSubKeys = KeyFullInfo->SubKeys;

        //
        // The QueryKeyInfo function will allocate memory if the buffer
        // we pass in is too small, so we should free it after we've
        // retrieved the data.
        //
        if( KeyFullInfo != ( PKEY_FULL_INFORMATION )PrivateKeyFullInfo ) {
            RegClassHeapFree(KeyFullInfo );
        }
    }

    return Status;
}


NTSTATUS ClassKeyCountSubKeys(
    HKEY    hKey,
    HKEY    hkUser,
    HKEY    hkMachine,
    DWORD   cMax,
    LPDWORD pcSubKeys)
/*++

Routine Description:

    Counts the total number of subkeys of a special key -- i.e.
    the sum of the subkeys in the user and machine portions
    of that special key minus duplicates.

Arguments:

    hkUser - user part of special key

    hkMachine - machine part of special key

    cMax - Maximum number of keys to count -- if
        zero, this is ignored

    pcSubKeys - out param -- count of subkeys

Return Value:

    Returns NT_SUCCESS (0) for success; error-code for failure.

Notes:

    This is INCREDIBLY expensive if either hkUser or hkMachine
    has more than a few subkeys.  It essentially merges two
    sorted lists by enumerating in both the user and machine
    locations, and viewing them as a merged list by doing
    comparisons betweens items in each list --
    separate user and machine pointers are advanced according
    to the results of the comparison. This means that if there are
    N keys under hkUser and M keys under hkMachine, this function
    will make N+M calls to the kernel to enumerate the keys.

    This is currently the only way to do this -- before, an approximation
    was used in which the sum of the number of subkeys in the
    user and machine versions was returned.  This method didn't take
    duplicates into account, and so it overestimated the number of keys.
    This was not thought to be a problem since there is no guarantee
    to callers that the number they receive is completely up to date,
    but it turns out that there are applications that make that assumption
    (such as regedt32) that do not function properly unless the
    exact number is returned.

--*/
{
    NTSTATUS          Status;
    BOOL              fCheckUser;
    BOOL              fCheckMachine;
    EnumSubtreeState  UserTree;
    EnumSubtreeState  MachineTree;
    DWORD             cMachineKeys;
    DWORD             cUserKeys;
    OBJECT_ATTRIBUTES Obja;
    HKEY              hkUserCount;
    HKEY              hkMachineCount;
    HKEY              hkNewKey;

    UNICODE_STRING EmptyString = {0, 0, 0};

    Status = STATUS_SUCCESS;

    hkNewKey = NULL;

    cMachineKeys = 0;
    cUserKeys = 0;

    //
    // Initialize ourselves to check in both the user
    // and machine hives for subkeys
    //
    fCheckUser = (hkUser != NULL);
    fCheckMachine = (hkMachine != NULL);

    memset(&UserTree, 0, sizeof(UserTree));
    memset(&MachineTree, 0, sizeof(MachineTree));

    //
    // We can't be sure that the user key was opened
    // with the right permissions so we'll open
    // a version that has the correct permissions
    //
    if (fCheckUser && (hkUser == hKey)) {
     
        InitializeObjectAttributes(
            &Obja,
            &EmptyString,
            OBJ_CASE_INSENSITIVE,
            hkUser,
            NULL);

        Status = NtOpenKey(
            &hkNewKey,
            KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
            &Obja);
        
        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        hkUserCount = hkNewKey;
    } else {
        hkUserCount = hkUser;
    }

    if (fCheckMachine && (hkMachine == hKey)) {
     
        ASSERT(!hkUserCount);

        InitializeObjectAttributes(
            &Obja,
            &EmptyString,
            OBJ_CASE_INSENSITIVE,
            hkMachine,
            NULL);

        Status = NtOpenKey(
            &hkNewKey,
            KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
            &Obja);
        
        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        hkMachineCount = hkNewKey;
    } else {
        hkMachineCount = hkMachine;
    }

    //
    // Now check to see how many keys are in the user subtree
    //
    if (fCheckUser) {
        Status = GetSubKeyCount(hkUserCount, &cUserKeys);

        if (!NT_SUCCESS(Status)) {
            goto cleanup;
        }

        //
        // We only need to enumerate the user portion if it has subkeys
        //
        fCheckUser = (cUserKeys != 0);
    }

    //
    // Now check to see how many keys are in the user subtree
    //
    if (fCheckMachine) {
        Status = GetSubKeyCount(hkMachineCount, &cMachineKeys);

        if (!NT_SUCCESS(Status)) {
            goto cleanup;
        }

        //
        // We only need to enumerate the machine portion if it has subkeys
        //
        fCheckMachine = (cMachineKeys != 0);
    }

    if (!fCheckUser) {

        *pcSubKeys = cMachineKeys;

        Status = STATUS_SUCCESS;
        
        goto cleanup;
    }

    if (!fCheckMachine) {

        *pcSubKeys = cUserKeys;

        Status = STATUS_SUCCESS;

        goto cleanup;
    }

    ASSERT(fCheckMachine && fCheckUser);

    *pcSubKeys = 0;

    //
    // Keep enumerating subkeys until one of the locations
    // runs out of keys
    //
    for (;;) {

        NTSTATUS EnumStatus;

        //
        // If we can still check in the user hive and we
        // are missing user key info, query the kernel for it
        //
        if (!(UserTree.pKeyInfo)) {
            EnumStatus = EnumClassKey(
                hkUserCount,
                &UserTree);

            //
            // If there are no more user subkeys, set our
            // flag so that we no longer look in the user portion
            // for subkeys
            //
            if (!NT_SUCCESS(EnumStatus)) {
                if (STATUS_NO_MORE_ENTRIES == EnumStatus) {

                    *pcSubKeys += cMachineKeys;
                    Status = STATUS_SUCCESS;
                    break;

                } else {
                    Status = EnumStatus;
                    break;
                }
            }
        }

        //
        // if we can still check in the machine hive and
        // we are missing machine info, query for it
        //
        if (!(MachineTree.pKeyInfo)) {

            EnumStatus = EnumClassKey(
                hkMachineCount,
                &MachineTree);

            //
            // Turn off checking in machine if there are
            // no more machine keys
            //
            if (!NT_SUCCESS(EnumStatus)) {
                if (STATUS_NO_MORE_ENTRIES == EnumStatus) {

                    *pcSubKeys += cUserKeys;
                    Status = STATUS_SUCCESS;
                    break;

                } else {
                    Status = EnumStatus;
                    break;
                }
            }
        }

        //
        // If we have keys in both user and machine, we need to compare
        // the key names to see when to advance our subtree pointers
        //
        {

            LONG lCompare;

            UNICODE_STRING MachineKeyName;
            UNICODE_STRING UserKeyName;

            MachineKeyName.Buffer = MachineTree.pKeyInfo->Name;
            MachineKeyName.Length = (USHORT) MachineTree.pKeyInfo->NameLength;

            UserKeyName.Buffer = UserTree.pKeyInfo->Name;
            UserKeyName.Length = (USHORT) UserTree.pKeyInfo->NameLength;

            //
            // Do the comparison of user and machine keys
            //
            lCompare =
                RtlCompareUnicodeString(&UserKeyName, &MachineKeyName, TRUE);

            //
            // User is smaller, so move our user pointer up and clear it
            // so we'll query for user data next time
            //
            if (lCompare <= 0) {
                EnumSubtreeStateClear(&UserTree);
                UserTree.iSubKey++;
                cUserKeys--;
            }

            //
            // Machine is smaller, so move our user pointer up and clear it
            // so we'll query for machine data next time
            //
            if (lCompare >= 0) {
                EnumSubtreeStateClear(&MachineTree);
                MachineTree.iSubKey++;
                cMachineKeys--;
            }

            //
            // Increase the total number of subkeys
            //
            (*pcSubKeys)++;

        }

        //
        // Only enumerate up to max -- the caller
        // doesn't need to go all the way to the end
        //
        if (cMax && (*pcSubKeys > cMax)) {
            break;
        }
    }

    //
    // Free any buffer held by these subtree states
    //
    EnumSubtreeStateClear(&UserTree);
    EnumSubtreeStateClear(&MachineTree);

cleanup:

    if (hkNewKey) {
        NtClose(hkNewKey);
    }

    return Status;
}

#endif // LOCAL

