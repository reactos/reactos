/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    FxChildListUm.cpp

Abstract:

    This module implements the FxChildList class

Author:




Environment:

    User mode only

Revision History:

--*/

#include <fxmin.hpp>

#pragma warning(push)
#pragma warning(disable:4100) //unreferenced parameter

FxDeviceDescriptionEntry::FxDeviceDescriptionEntry(
    __inout FxChildList* DeviceList,
    __in ULONG AddressDescriptionSize,
    __in ULONG IdentificationDescriptionSize
    )
{
    UfxVerifierTrapNotImpl();
}

FxDeviceDescriptionEntry::~FxDeviceDescriptionEntry()
{
    UfxVerifierTrapNotImpl();
}

_Must_inspect_result_
PVOID
FxDeviceDescriptionEntry::operator new(
    __in size_t AllocatorBlock,
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in size_t TotalDescriptionSize
    )
{
    UfxVerifierTrapNotImpl();
    return NULL;
}

VOID
FxDeviceDescriptionEntry::DeviceSurpriseRemoved(
    VOID
    )
{
    UfxVerifierTrapNotImpl();
}

BOOLEAN
FxDeviceDescriptionEntry::IsDeviceReportedMissing(
    VOID
    )
/*++

Routine Description:
    This function tells the caller if the description has been reported missing
    to the Pnp manager.  It does not change the actual state of the description,
    unlike IsDeviceRemoved().

Arguments:
    None

Return Value:
    TRUE if it has been reported missing, FALSE otherwise

  --*/
{
    UfxVerifierTrapNotImpl();
    return FALSE;
}

BOOLEAN
FxDeviceDescriptionEntry::IsDeviceRemoved(
    VOID
    )
{
    UfxVerifierTrapNotImpl();
    return FALSE;
}

VOID
FxDeviceDescriptionEntry::ProcessDeviceRemoved(
    VOID
    )
{
    UfxVerifierTrapNotImpl();
}

_Must_inspect_result_
FxDeviceDescriptionEntry*
FxDeviceDescriptionEntry::Clone(
    __inout PLIST_ENTRY FreeListHead
    )
{
    UfxVerifierTrapNotImpl();
    return NULL;
}

FxChildList::FxChildList(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in size_t TotalDescriptionSize,
    __in CfxDevice* Device,
    __in BOOLEAN Static
    ) :
    FxNonPagedObject(FX_TYPE_CHILD_LIST,sizeof(FxChildList), FxDriverGlobals),
    m_TotalDescriptionSize(TotalDescriptionSize),
    m_EvtCreateDevice(FxDriverGlobals),
    m_EvtScanForChildren(FxDriverGlobals)
{
    UfxVerifierTrapNotImpl();
}

BOOLEAN
FxChildList::Dispose(
    VOID
    )
{
    UfxVerifierTrapNotImpl();
    return TRUE;
}

_Must_inspect_result_
NTSTATUS
FxChildList::_CreateAndInit(
    __out FxChildList** ChildList,
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PWDF_OBJECT_ATTRIBUTES ListAttributes,
    __in size_t TotalDescriptionSize,
    __in CfxDevice* Device,
    __in PWDF_CHILD_LIST_CONFIG ListConfig,
    __in BOOLEAN Static
    )
{
    UfxVerifierTrapNotImpl();

    return STATUS_NOT_IMPLEMENTED;
}

VOID
FxChildList::Initialize(
    __in PWDF_CHILD_LIST_CONFIG Config
    )
{
    UfxVerifierTrapNotImpl();
}

WDFDEVICE
FxChildList::GetDevice(
    VOID
    )
{
    UfxVerifierTrapNotImpl();
    return NULL;
}

CfxDevice*
FxChildList::GetDeviceFromId(
    __in PWDF_CHILD_RETRIEVE_INFO Info
    )
{
    UfxVerifierTrapNotImpl();
    return NULL;
}

_Must_inspect_result_
NTSTATUS
FxChildList::GetAddressDescription(
    __in PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription,
    __out PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
    )
{
    UfxVerifierTrapNotImpl();
    return STATUS_NOT_IMPLEMENTED;
}

VOID
FxChildList::GetAddressDescriptionFromEntry(
    __in FxDeviceDescriptionEntry* Entry,
    __out PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
    )
{
    UfxVerifierTrapNotImpl();
}

VOID
FxChildList::BeginScan(
    __out_opt PULONG ScanTag
    )
{
    UfxVerifierTrapNotImpl();
}

VOID
FxChildList::EndScan(
    __inout_opt PULONG ScanTag
    )
{
    UfxVerifierTrapNotImpl();
}

VOID
FxChildList::CancelScan(
    __in  BOOLEAN EndTheScan,
    __inout_opt PULONG ScanTag
    )
{
    UfxVerifierTrapNotImpl();
}

VOID
FxChildList::InitIterator(
    __inout PWDF_CHILD_LIST_ITERATOR Iterator
    )
{
    UfxVerifierTrapNotImpl();
}

VOID
FxChildList::BeginIteration(
    __inout PWDF_CHILD_LIST_ITERATOR Iterator
    )
{
    UfxVerifierTrapNotImpl();
}

VOID
FxChildList::EndIteration(
    __inout PWDF_CHILD_LIST_ITERATOR Iterator
    )
{
    UfxVerifierTrapNotImpl();
}

_Must_inspect_result_
NTSTATUS
FxChildList::GetNextDevice(
    __out WDFDEVICE* Device,
    __inout PWDF_CHILD_LIST_ITERATOR Iterator,
    __inout_opt PWDF_CHILD_RETRIEVE_INFO Info
    )
{
    UfxVerifierTrapNotImpl();
    return STATUS_NOT_IMPLEMENTED;
}

WDFDEVICE
FxChildList::GetNextStaticDevice(
    __in WDFDEVICE PreviousDevice,
    __in ULONG Flags
    )
{
    UfxVerifierTrapNotImpl();
    return NULL;
}

_Must_inspect_result_
NTSTATUS
FxChildList::UpdateAsMissing(
    __in PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Description
    )
{
    UfxVerifierTrapNotImpl();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
NTSTATUS
FxChildList::UpdateDeviceAsMissing(
    __in CfxDevice* Device
    )
/*++

Routine Description:
    Same as UpdateAsMissing except instead of a device description, we have the
    device itself.

Arguments:
    Device - the device to mark as missing

Return Value:
    NTSTATUS

  --*/
{
    UfxVerifierTrapNotImpl();
    return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
FxChildList::ReenumerateEntryLocked(
    __inout FxDeviceDescriptionEntry* Entry,
    __in BOOLEAN FromQDR
    )
{
    UfxVerifierTrapNotImpl();
    return FALSE;
}

VOID
FxChildList::ReenumerateEntry(
    __inout FxDeviceDescriptionEntry* Entry
    )
{
    UfxVerifierTrapNotImpl();
}


VOID
FxChildList::UpdateAllAsPresent(
    __in_opt PULONG ScanTag
    )
{
    UfxVerifierTrapNotImpl();
}

_Must_inspect_result_
NTSTATUS
FxChildList::Add(
    __in PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription,
    __in_opt PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription,
    __in_opt PULONG ScanTag
    )
{
    UfxVerifierTrapNotImpl();
    return STATUS_NOT_IMPLEMENTED;
}

VOID
FxChildList::UpdateAddressDescriptionFromEntry(
    __inout FxDeviceDescriptionEntry* Entry,
    __in PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
    )
{
    UfxVerifierTrapNotImpl();
}

BOOLEAN
FxChildList::CloneEntryLocked(
    __inout PLIST_ENTRY FreeListHead,
    __inout FxDeviceDescriptionEntry* Entry,
    __in BOOLEAN FromQDR
    )
{
    UfxVerifierTrapNotImpl();
    return FALSE;
}

VOID
FxChildList::ProcessModificationsLocked(
    __inout PLIST_ENTRY FreeListHead
    )
{
    UfxVerifierTrapNotImpl();
}

VOID
FxChildList::MarkDescriptionNotPresentWorker(
    __inout FxDeviceDescriptionEntry* DescriptionEntry,
    __in BOOLEAN ModificationCanBeQueued
    )
/*++

Routine Description:

    This worker function marks the passed in mod or desc entry in the device
    list "not present". The change is enqueued in the mod list but the mod
    list is not drained.

Arguments:

    DescEntry - Matching description entry, if found.

    ModificationCanBeQueued - whether the caller allows for their to be a
        modification already queued on the modification list

Assumes:
    DescriptionEntry->IsPresent() == TRUE

Return Value:
    None

--*/
{
    UfxVerifierTrapNotImpl();
}

VOID
FxChildList::MarkModificationNotPresentWorker(
    __inout PLIST_ENTRY FreeListHead,
    __inout FxDeviceDescriptionEntry* ModificationEntry
    )
/*++

Routine Description:

    This worker function marks the passed in mod or desc entry in the device
    list "not present". The change is enqueued in the mod list but the mod
    list is not drained.

Arguments:

    ModEntry - Matching modification entry, if found. If this parameter is
               supplied, DescEntry should be NULL.

Return Value:
    The caller may not necessarily be interested in the transition from reported
    as present and not yet created to reported as missing...as such, many
    callers of this function will not inspect this return value.

--*/
{
    UfxVerifierTrapNotImpl();
}

VOID
FxChildList::DrainFreeListHead(
    __inout PLIST_ENTRY FreeListHead
    )
{
    UfxVerifierTrapNotImpl();
}

FxDeviceDescriptionEntry*
FxChildList::SearchBackwardsForMatchingModificationLocked(
    __in PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Id
    )
{
    UfxVerifierTrapNotImpl();
    return NULL;
}

FxDeviceDescriptionEntry*
FxChildList::SearchBackwardsForMatchingDescriptionLocked(
    __in PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Id
    )
{
    UfxVerifierTrapNotImpl();
    return NULL;
}

_Must_inspect_result_
NTSTATUS
FxChildList::VerifyDescriptionEntry(
    __in PLIST_ENTRY Entry
    )
{
    UfxVerifierTrapNotImpl();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
NTSTATUS
FxChildList::VerifyModificationEntry(
    __in PLIST_ENTRY Entry
    )
{
    UfxVerifierTrapNotImpl();
    return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
FxChildList::CreateDevice(
    __inout FxDeviceDescriptionEntry* Entry,
    __inout PBOOLEAN InvalidateRelations
    )
{
    UfxVerifierTrapNotImpl();
    return FALSE;
}

_Must_inspect_result_
NTSTATUS
FxChildList::ProcessBusRelations(
    __inout PDEVICE_RELATIONS *DeviceRelations
    )
{
    UfxVerifierTrapNotImpl();
    return STATUS_NOT_IMPLEMENTED;
}

VOID
FxChildList::InvokeReportedMissingCallback(
    VOID
    )
{
    UfxVerifierTrapNotImpl();
}

VOID
FxChildList::PostParentToD0(
    VOID
    )
{
    UfxVerifierTrapNotImpl();
}

VOID
FxChildList::IndicateWakeStatus(
    __in NTSTATUS WaitWakeStatus
    )
/*++

Routine Description:
    Propagates the wait wake status to all the child PDOs. This will cause any
    pended wait wake requests to be completed with the given wait wake status.

Arguments:
    WaitWakeStatus - The NTSTATUS value to use for compeleting any pended wait
        wake requests on the child PDOs.

Return Value:
    None

  --*/
{
    UfxVerifierTrapNotImpl();
}

VOID
FxChildList::NotifyDeviceSurpriseRemove(
    VOID
    )
/*++

Routine Description:
    Notification through IFxStateChangeNotification that the parent device is
    being surprise removed

Arguments:
    None

Return Value:
    None

  --*/
{
    UfxVerifierTrapNotImpl();
}

VOID
FxChildList::NotifyDeviceRemove(
    __inout PLONG ChildCount
    )
/*++

Routine Description:
    Notification through IFxStateChangeNotification that the parent device is
    being removed.

Arguments:
    None

Return Value:
    None

  --*/
{
    UfxVerifierTrapNotImpl();
}

_Must_inspect_result_
NTSTATUS
FxChildList::_ValidateConfig(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PWDF_CHILD_LIST_CONFIG Config,
    __in size_t* TotalDescriptionSize
    )
{
    UfxVerifierTrapNotImpl();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
NTSTATUS
FxChildList::_ComputeTotalDescriptionSize(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PWDF_CHILD_LIST_CONFIG Config,
    __in size_t* TotalDescriptionSize
    )
{
    UfxVerifierTrapNotImpl();
    return STATUS_NOT_IMPLEMENTED;
}

#pragma warning(pop)
