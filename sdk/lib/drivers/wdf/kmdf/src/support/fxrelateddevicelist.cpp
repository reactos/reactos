/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    FxDependentList.cpp

Abstract:
    This object derives from the transactioned list and provides a unique
    object check during the addition of an item.

Author:



Environment:

    Kernel mode only

Revision History:

--*/

#include "FxSupportPch.hpp"

_Must_inspect_result_
NTSTATUS
FxRelatedDeviceList::Add(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __inout FxRelatedDevice* Entry
    )
{
    return FxSpinLockTransactionedList::Add(FxDriverGlobals,
                                            &Entry->m_TransactionedEntry);
}

VOID
FxRelatedDeviceList::Remove(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PDEVICE_OBJECT Device
    )
{
    SearchForAndRemove(FxDriverGlobals, (PVOID) Device);
}

_Must_inspect_result_
FxRelatedDevice*
FxRelatedDeviceList::GetNextEntry(
    __in_opt FxRelatedDevice* Entry
    )
{
    FxTransactionedEntry *pReturn, *pEntry;

    if (Entry == NULL) {
        pEntry = NULL;
    }
    else {
        pEntry = &Entry->m_TransactionedEntry;
    }

    pReturn = FxSpinLockTransactionedList::GetNextEntry(pEntry);

    if (pReturn != NULL) {
        return CONTAINING_RECORD(pReturn, FxRelatedDevice, m_TransactionedEntry);
    }
    else {
        return NULL;
    }
}

_Must_inspect_result_
NTSTATUS
FxRelatedDeviceList::ProcessAdd(
    __in FxTransactionedEntry *NewEntry
    )
{
    FxRelatedDevice* pNew, *pInList;
    FxTransactionedEntry *pEntry;
    PLIST_ENTRY ple;

    pNew = CONTAINING_RECORD(NewEntry, FxRelatedDevice, m_TransactionedEntry);

    pEntry = NULL;

    //
    // Go over the transactions first because the device could be in the real
    // list with a transaction to remove it, so we catch first here instead
    // of adding complexity to the iteration of the already inserted list.
    //
    for (ple = m_TransactionHead.Flink;
         ple != &m_TransactionHead;
         ple = ple->Flink) {
        pEntry = FxTransactionedEntry::_FromEntry(ple);
        pInList = CONTAINING_RECORD(pEntry, FxRelatedDevice, m_TransactionedEntry);

        if (pInList->m_DeviceObject == pNew->m_DeviceObject) {
            if (pEntry->GetTransactionAction() == FxTransactionActionAdd) {
                //
                // An additional add, failure
                //
                return STATUS_DUPLICATE_OBJECTID;
            }


            // Removal is OK b/c our add will be right behind it
            //
            ASSERT(pEntry->GetTransactionAction() == FxTransactionActionRemove);
            return STATUS_SUCCESS;
        }
    }

    pEntry = NULL;
    while ((pEntry = __super::GetNextEntryLocked(pEntry)) != NULL) {
        pInList = CONTAINING_RECORD(pEntry, FxRelatedDevice, m_TransactionedEntry);

        if (pInList->m_DeviceObject == pNew->m_DeviceObject) {
            return STATUS_DUPLICATE_OBJECTID;
        }
    }

    return STATUS_SUCCESS;
}

BOOLEAN
FxRelatedDeviceList::Compare(
    __in FxTransactionedEntry* Entry,
    __in PVOID Data
    )
{
    FxRelatedDevice *pRelated;

    pRelated = CONTAINING_RECORD(Entry, FxRelatedDevice, m_TransactionedEntry);

    return pRelated->GetDevice() == (PDEVICE_OBJECT) Data ? TRUE : FALSE;
}

VOID
FxRelatedDeviceList::EntryRemoved(
    __in FxTransactionedEntry* Entry
    )
{
    FxRelatedDevice *pRelated;

    pRelated = CONTAINING_RECORD(Entry, FxRelatedDevice, m_TransactionedEntry);

    if (pRelated->m_State == RelatedDeviceStateReportedPresent) {
        m_NeedReportMissing++;
    }
}
