/*++

Copyright (C) Microsoft Corporation, 1990 - 1999

Module Name:

    dictlib.c

Abstract:

    Support library for maintaining a dictionary list (list of objects
    referenced by a key value).

Environment:

    kernel mode only

Notes:

    This module generates a static library

Revision History:

--*/

#include "classp.h"

#define DICTIONARY_SIGNATURE 'dsig'

typedef struct _DICTIONARY_HEADER {
    struct _DICTIONARY_HEADER* Next;
    ULONGLONG Key;
    UCHAR Data[0];
} DICTIONARY_HEADER, *PDICTIONARY_HEADER;


VOID
NTAPI
InitializeDictionary(
    IN PDICTIONARY Dictionary
    )
{
    RtlZeroMemory(Dictionary, sizeof(DICTIONARY));
    Dictionary->Signature = DICTIONARY_SIGNATURE;
    KeInitializeSpinLock(&Dictionary->SpinLock);
    return;
}

BOOLEAN
NTAPI
TestDictionarySignature(
    IN PDICTIONARY Dictionary
    )
{
    return Dictionary->Signature == DICTIONARY_SIGNATURE;
}

NTSTATUS
NTAPI
AllocateDictionaryEntry(
    IN PDICTIONARY Dictionary,
    IN ULONGLONG Key,
    IN ULONG Size,
    IN ULONG Tag,
    OUT PVOID *Entry
    )
{
    PDICTIONARY_HEADER header;
    KIRQL oldIrql;
    PDICTIONARY_HEADER *entry;

    NTSTATUS status = STATUS_SUCCESS;

    *Entry = NULL;

    header = ExAllocatePoolWithTag(NonPagedPool,
                                   Size + sizeof(DICTIONARY_HEADER),
                                   Tag);

    if(header == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(header, sizeof(DICTIONARY_HEADER) + Size);
    header->Key = Key;

    //
    // Find the correct location for this entry in the dictionary.
    //

    KeAcquireSpinLock(&(Dictionary->SpinLock), &oldIrql);

    TRY {

        entry = &(Dictionary->List);

        while(*entry != NULL) {
            if((*entry)->Key == Key) {

                //
                // Dictionary must have unique keys.
                //

                status = STATUS_OBJECT_NAME_COLLISION;
                LEAVE;

            } else if ((*entry)->Key < Key) {

                //
                // We will go ahead and insert the key in here.
                //
                break;
            } else {
                entry = &((*entry)->Next);
            }
        }

        //
        // If we make it here then we will go ahead and do the insertion.
        //

        header->Next = *entry;
        *entry = header;

    } FINALLY {
        KeReleaseSpinLock(&(Dictionary->SpinLock), oldIrql);

        if(!NT_SUCCESS(status)) {
            ExFreePool(header);
        } else {
            *Entry = (PVOID) header->Data;
        }
    }
    return status;
}

PVOID
NTAPI
GetDictionaryEntry(
    IN PDICTIONARY Dictionary,
    IN ULONGLONG Key
    )
{
    PDICTIONARY_HEADER entry;
    PVOID data;
    KIRQL oldIrql;


    data = NULL;

    KeAcquireSpinLock(&(Dictionary->SpinLock), &oldIrql);

    entry = Dictionary->List;
    while (entry != NULL) {
        
        if (entry->Key == Key) {
            data = entry->Data;
            break;
        } else {
            entry = entry->Next;
        }
    }

    KeReleaseSpinLock(&(Dictionary->SpinLock), oldIrql);

    return data;
}

VOID
NTAPI
FreeDictionaryEntry(
    IN PDICTIONARY Dictionary,
    IN PVOID Entry
    )
{
    PDICTIONARY_HEADER header;
    PDICTIONARY_HEADER *entry;
    KIRQL oldIrql;
    BOOLEAN found;

    found = FALSE;
    header = CONTAINING_RECORD(Entry, DICTIONARY_HEADER, Data);

    KeAcquireSpinLock(&(Dictionary->SpinLock), &oldIrql);

    entry = &(Dictionary->List);
    while(*entry != NULL) {

        if(*entry == header) {
            *entry = header->Next;
            found = TRUE;
            break;
        } else {
            entry = &(*entry)->Next;
        }
    }

    KeReleaseSpinLock(&(Dictionary->SpinLock), oldIrql);

    //
    // calling this w/an invalid pointer invalidates the dictionary system,
    // so ASSERT() that we never try to Free something not in the list
    //

    ASSERT(found);
    if (found) {
        ExFreePool(header);
    }

    return;

}
