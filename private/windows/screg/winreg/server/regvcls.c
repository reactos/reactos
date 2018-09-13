/*++


Copyright (c) 1991  Microsoft Corporation

Module Name:

    RegvCls.c

Abstract:

    This module contains helper functions for enumerating, 
    setting, and querying registry values in win32

Author:

    Adam Edwards (adamed) 06-May-1998

Key Functions:

Notes:


--*/


#ifdef LOCAL

#include <rpc.h>
#include "regrpc.h"
#include "localreg.h"
#include "regclass.h"
#include "regvcls.h"
#include <malloc.h>



void ValStateGetPhysicalIndexFromLogical(
    ValueState* pValState,
    HKEY        hkLogicalKey,
    DWORD       dwLogicalIndex,
    PHKEY       phkPhysicalKey,
    DWORD*      pdwPhysicalIndex)
/*++

Routine Description:

    Retrieves a logical index for a value to a physical index

Arguments:

    pValState - value state containing values for a logical key
    hkLogicalKey - logical key we wish to index
    dwLogicalIndex - logical index to map
    phkPhysicalKey - handle to key where value is physically located
    pdwPhysicalIndex - index of value in physical key

Return Value:

    None.

Notes:

--*/
{
    //
    // If no value state is supplied, this means no merging is necessary
    // and we can return the supplied logical index as the correct
    // physical index
    //
    if (!pValState) {
        *pdwPhysicalIndex = dwLogicalIndex;
        *phkPhysicalKey = hkLogicalKey;
    } else {
        *pdwPhysicalIndex = pValState->rgIndex[dwLogicalIndex].dwOffset;
        *phkPhysicalKey = pValState->rgIndex[dwLogicalIndex].fUser ?
            pValState->hkUser :
            pValState->hkMachine;
    }
}


NTSTATUS ValStateSetPhysicalIndexFromLogical(
    ValueState*                     pValState,
    DWORD                           dwLogicalIndex)
/*++

Routine Description:

    Updates a state's mapping of logical indexes to physical indexes

Arguments:

    pValState - value state containing values for a logical key
    dwLogicalIndex - logical index used as a clue for whether
        or not we can used cached values or need to refresh the state -- 
        gives us an idea of what index the caller will be interested in 
        mapping after this call.

Return Value:

    None.

Notes:

--*/
{
    NTSTATUS Status;

    Status = STATUS_SUCCESS;

    //
    // If no value state is supplied, this means no merging is necessary
    // and we can return the supplied logical index as the correct
    // physical index
    //
    if (!pValState) {
        return STATUS_SUCCESS;
    }

    if (dwLogicalIndex >= pValState->cValues) {
        
        pValState->fDelete = TRUE;
        return STATUS_NO_MORE_ENTRIES;
    }

    //
    // Always reset if they try to go backward, or
    // if they skip by more than 1, or if they
    // ask for the same index twice and we're
    // not expecting it
    //
    if ((dwLogicalIndex < pValState->dwCurrent) || 
        (dwLogicalIndex > (pValState->dwCurrent + 1)) ||
        ((dwLogicalIndex == pValState->dwCurrent) && !(pValState->fIgnoreResetOnRetry))) {
    
        Status = ValStateUpdate(pValState);

        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        pValState->fIgnoreResetOnRetry = FALSE;
    }

    return Status;
}


void ValStateRelease(
    ValueState* pValState)
/*++

Routine Description:

    Frees resources (handles, memory) associated with a value state

Arguments:

    pValState - value state containing values for a logical key

Return Value:

    None.

Notes:

--*/
{
    if (!pValState) {
        return; 
    }

    if (pValState->hkUser && (pValState->hkUser != pValState->hkLogical)) {
        NtClose(pValState->hkUser);
    }

    if (pValState->hkMachine && (pValState->hkMachine != pValState->hkLogical)) {
        NtClose(pValState->hkMachine);
    }

    if (pValState->rgIndex) {
        RegClassHeapFree(pValState->rgIndex);
    }

    RegClassHeapFree(pValState);
}


NTSTATUS ValStateUpdate(ValueState* pValState)
/*++

Routine Description:

    Updates the value state to reflect the current state
        of the logical key's physical state -- it retrieves
        the names of the values for the logical key from
        the kernel, and re-indexes the table to properly
        merge user and machine state

Arguments:

    pValState - value state containing values for a logical key

Return Value:

    STATUS_SUCCESS for success, error code otherwise.

Notes:

--*/
{
    NTSTATUS             Status;
    DWORD                cUserValues;
    DWORD                cMachineValues;
    DWORD                cMaxValues;
    DWORD                cbMaxNameLen;
    DWORD                cbMaxDataLen;
    DWORD                cbBufferLen;
    ValueLocation*       rgIndex;
    PKEY_VALUE_BASIC_INFORMATION* ppValueInfo;

    //
    // Init locals
    //
    cUserValues = 0;
    cMachineValues = 0;
    cbMaxNameLen = 0;
    rgIndex = NULL;

    pValState->cValues = 0;

    //
    // Get information about this value
    //
    Status = GetFixedKeyInfo(
        pValState->hkUser,
        pValState->hkMachine,
        &cUserValues,
        &cMachineValues,
        NULL,
        NULL,
        &cbMaxNameLen);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    cMaxValues = cUserValues + cMachineValues;

    //
    // Nothing to do if there are no Values
    //
    if (!cMaxValues) {
        return STATUS_SUCCESS;
    }

    //
    // Now allocate necessary memory
    // First get memory for index vector
    //
    rgIndex = (ValueLocation*) RegClassHeapAlloc(cMaxValues * sizeof(*rgIndex));

    if (!rgIndex) {
        return STATUS_NO_MEMORY;
    }

    //
    // Now get memory for retrieving names -- first allocate an array 
    // of pointers to values
    //
    ppValueInfo = (PKEY_VALUE_BASIC_INFORMATION*) RegClassHeapAlloc(
        sizeof(*ppValueInfo) * cMaxValues);

    if (!ppValueInfo) {

        RegClassHeapFree(rgIndex);

        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(ppValueInfo, sizeof(*ppValueInfo) * cMaxValues);

    cbBufferLen = sizeof(**ppValueInfo) + cbMaxNameLen;

    //
    // Now allocate each individual value
    //
    {
        DWORD dwValue;

        for (dwValue = 0; dwValue < cMaxValues; dwValue++) 
        {
            ppValueInfo[dwValue] = (PKEY_VALUE_BASIC_INFORMATION) RegClassHeapAlloc(
                cbBufferLen);

            if (!(ppValueInfo)[dwValue]) {
                Status = STATUS_NO_MEMORY;
                break;
            }
        }
    }

    //
    // Now fetch the values.  From this point on we are assuming success
    // and updating the index table
    // 
    {

        HKEY  hKeyPhysical;
        DWORD dwLimit;
        DWORD dwLogical;
        BOOL  fUser;

        //
        // Free the existing index table
        //
        if (pValState->rgIndex) {
            RegClassHeapFree(pValState->rgIndex);
        }

        pValState->rgIndex = rgIndex;

        dwLogical = 0;

        for( hKeyPhysical = pValState->hkUser, fUser = TRUE,
                 dwLimit = cUserValues;
             ;
             hKeyPhysical = pValState->hkMachine, fUser = FALSE,
                 dwLimit = cMachineValues)
        {
            DWORD dwPhysical;

            for (dwPhysical = 0; dwPhysical < dwLimit; dwPhysical++) 
            {
                BOOL fNewValue;

                //
                // Ask the kernel for the value
                //
                Status = EnumerateValue(
                    hKeyPhysical,
                    dwPhysical,
                    ppValueInfo[dwLogical],
                    cbBufferLen,
                    NULL);

                //
                // If we encounter an error, just keep going and try to get
                // as many values as we can
                //
                if (!NT_SUCCESS(Status)) {
                    continue;
                }

                //
                // Mark certain attributes about this value that will
                // be important later
                //
                ppValueInfo[dwLogical]->TitleIndex = dwPhysical;
                ppValueInfo[dwLogical]->Type = fUser;
                

                //
                // This will add the value to our sorted list.  Since
                // the list is sorted, it is easy to eliminated duplicates --
                // don't add duplicates -- since we add 
                // user keys first, this allows us to give user values precedence
                // over machine values of the same name.  The logical key
                // index is also incremented if a key is added.
                //
                fNewValue = ValStateAddValueToSortedValues(
                    ppValueInfo,
                    dwLogical);

                if (fNewValue) {
                    dwLogical++;
                }
            }

            //
            // Break out of this loop if we just added the user values
            // since those are the last values we add
            //
            if (!fUser) {
                break;
            }
        }

        pValState->cValues = dwLogical;
    }

    //
    // Now copy the results back to the state's index array
    //
    {

        DWORD dwLogical;

        for (dwLogical = 0; dwLogical < pValState->cValues; dwLogical++)
        {
    
            pValState->rgIndex[dwLogical].dwOffset = 
                ppValueInfo[dwLogical]->TitleIndex;
            
            pValState->rgIndex[dwLogical].fUser =
                ppValueInfo[dwLogical]->Type;
        }
    }

    //
    // Release this
    //
    ValStateReleaseValues(
        ppValueInfo,
        cMaxValues);

    return STATUS_SUCCESS;
}


void ValStateReleaseValues(
    PKEY_VALUE_BASIC_INFORMATION* ppValueInfo,
    DWORD                         cMaxValues)
/*++

Routine Description:

    Releases resources associated with the values stored
        in the value state.

Arguments:

    pValState - value state containing values for a logical key

Return Value:

    None.

Notes:

--*/
{
    DWORD dwValue;

    //
    // First, free each individual value
    //
    for (dwValue = 0; dwValue < cMaxValues; dwValue++) 
    {
        //
        // Free memory for this value
        //
        if (ppValueInfo[dwValue]) {
            RegClassHeapFree(ppValueInfo[dwValue]);
        }
    }
    
    //
    // Now free the array that held all the values
    //
    RegClassHeapFree(ppValueInfo);
}



NTSTATUS ValStateInitialize( 
    ValueState** ppValState,
    HKEY         hKey)
/*++

Routine Description:

    Initializes a value state 

Arguments:

    pValState - value state containing values for a logical key
    hKey - logical key whose state this value state will represent

Return Value:

    STATUS_SUCCESS for success, error code otherwise.

Notes:

--*/
{
    NTSTATUS    Status;
    ValueState* pValState;
    HKEY        hkUser;
    HKEY        hkMachine;

    //
    // Initialize conditionally freed resources
    //
    hkUser = NULL;
    hkMachine = NULL;

    pValState = NULL;

    //
    // Get the user and machine keys
    //
    Status = BaseRegGetUserAndMachineClass(
        NULL,
        hKey,
        MAXIMUM_ALLOWED,
        &hkMachine,
        &hkUser);

    if (NT_SUCCESS(Status)) {

        ASSERT(hkUser || hkMachine);

        //
        // We only need to create a state if there are
        // two keys -- if only one exists, we don't
        // need to do merging
        //
        if (!hkUser || !hkMachine) {
            *ppValState = NULL;
            
            return STATUS_SUCCESS;
        }

        //
        // Get memory for the value state
        //
        pValState = RegClassHeapAlloc( sizeof(*pValState) + 
                                   sizeof(DWORD) * DEFAULT_VALUESTATE_SUBKEY_ALLOC );

        //
        // Be sure to release acquired resources on failure
        //
        if (!pValState) {

            if (hkUser != hKey) {
                NtClose(hkUser);
            } else {
                NtClose(hkMachine);
            }

            return STATUS_NO_MEMORY;
        }

        RtlZeroMemory(pValState, sizeof(*pValState));

        pValState->hkUser = hkUser;
        pValState->hkMachine = hkMachine;
        pValState->hkLogical = hKey;
        pValState->fIgnoreResetOnRetry = TRUE;

        //
        // Now update the state to reflect the current registry
        //
        Status = ValStateUpdate(pValState);
    } 

    //
    // On success, set our out param
    //
    if (NT_SUCCESS(Status)) {
        *ppValState = pValState;
    } else {

        if (pValState) {
            ValStateRelease(pValState);
        }
    }

    return Status;

}


BOOL ValStateAddValueToSortedValues(
    PKEY_VALUE_BASIC_INFORMATION* ppValueInfo,
    LONG                          lNewValue)
/*++

Routine Description:

    Inserts a retrieved value into the sorted list
        of values in a value state

Arguments:

    pValState - value state containing values for a logical key
    lNewValue - index of newly added value in the sorted list --
        this value needs to be moved elsewhere in the list to maintain
        the sorted nature of the list

Return Value:

    TRUE if the state was added, FALSE if not.

Notes:

--*/
{
    PKEY_VALUE_BASIC_INFORMATION pNewValue;
    LONG                         lFinalSpot;
    LONG                         lCurrent;
    UNICODE_STRING               NewKeyName;

    lFinalSpot = 0;

    pNewValue = ppValueInfo[lNewValue];
            
    NewKeyName.Buffer = pNewValue->Name;
    NewKeyName.Length = (USHORT) pNewValue->NameLength;

    for (lCurrent = lNewValue - 1; lCurrent >= 0; lCurrent--) 
    {
        UNICODE_STRING               CurrentValueName;
        PKEY_VALUE_BASIC_INFORMATION pCurrentValue;
        LONG                         lCompareResult;

        pCurrentValue = ppValueInfo[lCurrent];

        CurrentValueName.Buffer = pCurrentValue->Name;
        CurrentValueName.Length = (USHORT) pCurrentValue->NameLength;

        lCompareResult = RtlCompareUnicodeString(
            &NewKeyName,
            &CurrentValueName,
            TRUE);

        if (lCompareResult < 0) {

            continue;

        } else if (0 == lCompareResult) {
            //
            // If it's a duplicate, don't add it
            //
            return FALSE;
            
        } else {

            lFinalSpot = lCurrent + 1;

            break;
        }
    }

    //
    // Now we know the final spot, add the value
    //
    
    //
    // Move everything up to make room for the new value
    //
    for (lCurrent = lNewValue - 1; lCurrent >= lFinalSpot; lCurrent--) 
    {
        //
        // Move the current value up one
        //
        ppValueInfo[lCurrent + 1] = ppValueInfo[lCurrent];
    }

    //
    // Copy the value to its final destination
    //
    ppValueInfo[lFinalSpot] = pNewValue;

    //
    // This means we've found no duplicate value
    // so we add it
    //
    return TRUE;
}


NTSTATUS KeyStateGetValueState(
    HKEY         hKey,
    ValueState** ppValState)
/*++

Routine Description:

    Gets the value state for a particular key

Arguments:

    hKey - key whose state we need to retrieve
    ppValState - out param pointing to a pointer to the
        retrieved state.

Return Value:

    STATUS_SUCCESS for success, error code otherwise.

Notes:

    BUGBUG: Right now, this always creates a new state -- in the future,
        we may want to change this to be cached in a table to avoid reconstructing
        on every call.

--*/
{
    //
    // Now build the value state
    //
    return ValStateInitialize(
        ppValState,
        hKey);
}


NTSTATUS BaseRegGetClassKeyValueState(
    HKEY         hKey,
    DWORD        dwLogicalIndex,
    ValueState** ppValState)
/*++

Routine Description:

    Gets the value state for a particular key and optimizes
        it for a given index

Arguments:

    hKey - key whose state we need to retrieve
    dwLogicalIndex - hint that helps us to optimize the state for this
        index so the caller's use of the state is more efficient
    ppValState - out param pointing to a pointer to the
        retrieved state.

Return Value:

    STATUS_SUCCESS for success, error code otherwise.

Notes:

--*/
{
    NTSTATUS    Status;
    ValueState* pValState;

    //
    // First retrieve the state for this key
    // 
    Status = KeyStateGetValueState(hKey, &pValState);

    if (NT_SUCCESS(Status)) {

        //
        // Now map the logical index to a physical one
        //
        Status = ValStateSetPhysicalIndexFromLogical(pValState, dwLogicalIndex);

        if (!NT_SUCCESS(Status)) {
            ValStateRelease(pValState);
        } else {
            *ppValState = pValState;
        }

    }

    return Status;
}


NTSTATUS EnumerateValue(
    HKEY                            hKey,
    DWORD                           dwValue,
    PKEY_VALUE_BASIC_INFORMATION    pSuggestedBuffer,
    DWORD                           dwSuggestedBufferLength,
    PKEY_VALUE_BASIC_INFORMATION*   ppResult)
/*++

Routine Description:

    Retrieves a value for a physical key from the kernel

Arguments:

    hKey - physical key for which we're trying to read a value
    dwValue - physical index of value to read
    pSuggestedBuffer - basinc info buffer to use by default, may not be large enough
    dwSuggestedBufferLength - size of suggested buffer
    ppResult - pointer to result basic info -- may be allocated by this function if
        suggested buffer was insufficient, which means caller will have to free
        this if it is not the same as the suggested buffer

Return Value:

    STATUS_SUCCESS for success, error code otherwise.

Notes:

--*/
{
    NTSTATUS                        Status;
    PKEY_VALUE_BASIC_INFORMATION    pKeyValueInformation;        
    DWORD                           dwResultLength;

    pKeyValueInformation = pSuggestedBuffer;

    //
    // Query for the necessary information about the supplied value.
    //
    Status = NtEnumerateValueKey( hKey,
                                  dwValue,
                                  KeyValueBasicInformation,
                                  pKeyValueInformation,
                                  dwSuggestedBufferLength,
                                  &dwResultLength);
    //
    // A return value of STATUS_BUFFER_TOO_SMALL would mean that there
    // was not enough room for even the known (i.e. fixed length portion)
    // of the structure.
    //

    ASSERT( Status != STATUS_BUFFER_TOO_SMALL );

    if (ppResult && (STATUS_BUFFER_OVERFLOW == Status)) {

        pKeyValueInformation = (PKEY_VALUE_BASIC_INFORMATION) RegClassHeapAlloc(
            dwResultLength);

        if (!pKeyValueInformation) {
            return STATUS_NO_MEMORY;
        }

        //
        // Query for the necessary information about the supplied value.
        //
        Status = NtEnumerateValueKey( hKey,
                                      dwValue,
                                      KeyValueBasicInformation,
                                      pKeyValueInformation,
                                      dwResultLength,
                                      &dwResultLength);

        ASSERT( Status != STATUS_BUFFER_TOO_SMALL );

        if (!NT_SUCCESS(Status)) {
            RegClassHeapFree(pKeyValueInformation);
        }
    }

    if (NT_SUCCESS(Status) && ppResult) {
        *ppResult = pKeyValueInformation;
    }

    return Status;
}


NTSTATUS BaseRegQueryMultipleClassKeyValues(
    HKEY     hKey,
    PRVALENT val_list,
    DWORD    num_vals,
    LPSTR    lpvalueBuf,
    LPDWORD  ldwTotsize,
    PULONG   ldwRequiredLength)
/*++

Routine Description:

    Gets the value state for a particular key and optimizes
        it for a given index

Arguments:

    hKey - Supplies a handle to the open key. The value entries returned
           are contained in the key pointed to by this key handle. Any of
           the predefined reserved handles or a previously opened key handle
           may be used for hKey.

    val_list - Supplies a pointer to an array of RVALENT structures, one for
           each value to be queried.

    num_vals - Supplies the size in bytes of the val_list array.

    lpValueBuf - Returns the data for each value

    ldwTotsize - Supplies the length of lpValueBuf. Returns the number of bytes
                 written into lpValueBuf. 

    ldwRequiredLength - If lpValueBuf is not large enough to
                 contain all the data, returns the size of lpValueBuf required
                 to return all the requested data.

Return Value:

    STATUS_SUCCESS for success, error code otherwise.

Notes:

--*/
{
    NTSTATUS    Status;
    HKEY        hkUser;
    HKEY        hkMachine;
    HKEY        hkQuery;

    //
    // Initialize conditionally freed resources
    //
    hkUser = NULL;
    hkMachine = NULL;

    //
    // First, get both user and machine keys
    //
    Status = BaseRegGetUserAndMachineClass(
        NULL,
        hKey,
        MAXIMUM_ALLOWED,
        &hkMachine,
        &hkUser);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // If we have both, we call a routine
    // to merge the values
    //
    if (hkMachine && hkUser) {

        Status = BaseRegQueryAndMergeValues(
            hkUser,
            hkMachine,
            val_list,
            num_vals,
            lpvalueBuf,
            ldwTotsize,
            ldwRequiredLength);

        goto cleanup;
    }

    //
    // We have only one key -- query the one with the 
    // highest precedence
    //
    hkQuery = hkUser ? hkUser : hkMachine;

    Status = NtQueryMultipleValueKey(hkQuery,
                                     (PKEY_VALUE_ENTRY)val_list,
                                     num_vals,
                                     lpvalueBuf,
                                     ldwTotsize,
                                     ldwRequiredLength);

cleanup:

    //
    // Close extra kernel object
    //
    if (hKey != hkUser) {
        NtClose(hkUser);
    } else {
        NtClose(hkMachine);
    }

    return Status;
}


NTSTATUS BaseRegQueryAndMergeValues(
    HKEY     hkUser,
    HKEY     hkMachine,
    PRVALENT val_list,
    DWORD    num_vals,
    LPSTR    lpvalueBuf,
    LPDWORD  ldwTotsize,
    PULONG   ldwRequiredLength)
/*++

Routine Description:

    Gets the value state for a particular key and optimizes
        it for a given index

Arguments:

    hkUser - user key to query for values
    hkMachine - machine key to query for values

    val_list - Supplies a pointer to an array of RVALENT structures, one for
           each value to be queried.

    num_vals - Supplies the size in bytes of the val_list array.

    lpValueBuf - Returns the data for each value

    ldwTotsize - Supplies the length of lpValueBuf. Returns the number of bytes
                 written into lpValueBuf. 

    ldwRequiredLength - If lpValueBuf is not large enough to
                 contain all the data, returns the size of lpValueBuf required
                 to return all the requested data.

Return Value:

    STATUS_SUCCESS for success, error code otherwise.

Notes:

    BUGBUG: this is non-atomic, unlike the regular RegQueryMultipleValues
        call.  In the future, implementing this in the kernel would make this
        atomic again. 

--*/
{
    NTSTATUS Status;
    DWORD    dwVal;
    BOOL     fOverflow;
    DWORD    dwBufferLength;
    DWORD    dwRequired;
    DWORD    dwKeyInfoLength;
    DWORD    dwBufferUsed;

    PKEY_VALUE_PARTIAL_INFORMATION pKeyInfo;

    //
    // Initialize locals
    //
    dwBufferLength = *ldwTotsize;
    dwRequired = 0;
    dwBufferUsed = 0;

    fOverflow = FALSE;

    //
    // Validate out params -- we assume that ldwTotsize and
    // ldwRequiredLength were given to us by winreg client,
    // so they should be safe to read / write from. lpValueBuf
    // comes from the caller of the win32 api, so we need to
    // validate it -- in previous versions of NT, this parameter
    // went straight to the kernel, which did the validation and
    // returned an error if it was pointing to inaccessible memory.
    // Since we're accessing it here in user mode, we need to do 
    // our own validation
    //
    if (IsBadWritePtr( lpvalueBuf, dwBufferLength)) 
    {
        return STATUS_ACCESS_VIOLATION;
    }
        
    //
    // First, we need to allocate enough memory to retrieve
    // all the values -- we can't just use lpvalueBuf 
    // because it doesn't include the overhead of the
    // KEY_VALUE_PARTIAL_INFORMATION structure.  If we allocate
    // for the size of lpvalueBuf + the structure overhead,
    // we will always have enough for our queries.
    //
    dwKeyInfoLength = sizeof(*pKeyInfo) * num_vals + *ldwTotsize;
    
    pKeyInfo = (PKEY_VALUE_PARTIAL_INFORMATION)
        RegClassHeapAlloc( dwKeyInfoLength);

    if (!pKeyInfo) {
        return STATUS_NO_MEMORY;
    }

    //
    // For each value requested by the caller, try 
    // to retrieve it from user or machine
    //
    for (dwVal = 0; dwVal < num_vals; dwVal++)
    {
        DWORD dwResultLength;
        
        //
        // Round up the used and required lengths to a ULONG boundary --
        // this means that the required size returned to the caller of the win32
        // api can be an overestimation, as much as 3 bytes per requested value.
        // We could do some work to avoid this, but since the kernel returns a rounded
        // up value in dwResultLength, the kernel api is itself overestimating, although
        // it only overestimates by at most 3 bytes over all the values.  We could avoid
        // this by allocating enough memory ahead of time to query the largest value, either
        // as one large preallocation or continually allocating and reallocating, but this will
        // be slower and / or consume more memory
        //
        dwBufferUsed = (dwBufferUsed + sizeof(ULONG)-1) & ~(sizeof(ULONG)-1);
        dwRequired = (dwRequired + sizeof(ULONG)-1) & ~(sizeof(ULONG)-1);

        //
        // Query the user key first since it has highest precedence
        //
        Status = NtQueryValueKey(
            hkUser,
            val_list[dwVal].rv_valuename,
            KeyValuePartialInformation,
            pKeyInfo,
            dwKeyInfoLength,
            &dwResultLength);

        //
        // Check for errors -- if the value didn't exist, we'll look
        // in machine -- for buffer overflow, we'll proceed as if
        // this succeeded so that we can calculate the required
        // buffer size
        //
        if (!NT_SUCCESS(Status) && 
            (STATUS_BUFFER_OVERFLOW != Status)) {
            
            if (STATUS_OBJECT_NAME_NOT_FOUND != Status) {
                goto cleanup;
            }
            
            //
            // If there is no user value, query the machine key
            //
            Status = NtQueryValueKey(
                hkMachine,
                val_list[dwVal].rv_valuename,
                KeyValuePartialInformation,
                pKeyInfo,
                dwKeyInfoLength,
                &dwResultLength);
            
            //
            // Similar error handling to above -- if we don't have enough
            // buffer, pretend we've succeeded so we can calc the required size
            //
            if (!NT_SUCCESS(Status) &&
                (STATUS_BUFFER_OVERFLOW != Status)) {
                goto cleanup;
            }
        }
        
        ASSERT(NT_SUCCESS(Status) || (STATUS_BUFFER_OVERFLOW == Status));
        
        if (NT_SUCCESS(Status)) {
            dwResultLength = pKeyInfo->DataLength;
        }
        
        //
        // Check for buffer overflow
        //
        if ( ( (dwBufferUsed + pKeyInfo->DataLength) <= dwBufferLength) && !fOverflow) {
            
            ASSERT(NT_SUCCESS(Status));
            
            //
            // Copy the data to the fixed part of the client's structure
            //
            val_list[dwVal].rv_valuelen = dwResultLength;
            val_list[dwVal].rv_valueptr = dwRequired;
            val_list[dwVal].rv_type = pKeyInfo->Type;

            //
            // We didn't overflow, so we still have enough room to copy
            // the latest value
            //
            RtlCopyMemory(
                (BYTE*)lpvalueBuf + val_list[dwVal].rv_valueptr,
                &(pKeyInfo->Data),
                dwResultLength);
            
            dwBufferUsed += pKeyInfo->DataLength;
            
        } else {
            //
            // We're out of buffer -- set this flag to
            // signal this state
            //
            fOverflow = TRUE;            
        }
        
        //
        // Update our required length with the size
        // of the data from the current value
        //
        dwRequired += dwResultLength;
    }

    //
    // At this point, we've succeeded in the sense that
    // we've copied all the data or we couldn't due to
    // insufficient buffer but we were able to calculate
    // the required size
    //
    Status = STATUS_SUCCESS;

cleanup:

    //
    // Free the allocated memory
    // 
    RegClassHeapFree(pKeyInfo);

    //
    // If we succeeded, this means we've either copied
    // the data or overflowed and copied the size -- handle
    // both below
    //
    if (NT_SUCCESS(Status)) {

        //
        // Always set this so the caller knows how much
        // was copied or needs to be allocated
        //
        *ldwRequiredLength = dwRequired;
        
        //
        // Return the appropriate error if we overflowed
        //
        if (fOverflow) {
            return STATUS_BUFFER_OVERFLOW;
        }

        //
        // Setting this, although winreg client actually
        // ignores this quantity
        //
        *ldwTotsize = dwBufferUsed;
    }

    return Status;
}

#endif LOCAL





