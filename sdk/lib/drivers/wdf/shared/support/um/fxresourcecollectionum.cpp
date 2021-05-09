/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxResourceCollection.cpp

Abstract:

    This module implements a base object for derived collection classes and
    the derived collection classes.

Author:



Environment:

    User mode only

Revision History:

--*/

#include "FxSupportPch.hpp"
#include <intsafe.h>

#if defined(EVENT_TRACING)
// Tracing support
extern "C" {
#include "FxResourceCollectionUm.tmh"
}
#endif

FxCmResList::~FxCmResList()
{
    DeleteRegisterResourceTable();
    DeletePortResourceTable();
}

NTSTATUS
FxCmResList::BuildRegisterResourceTable(
    VOID
    )
{
    ULONG count;
    ULONG i, index;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR desc;
    ULONG numRegisterDesc;
    BOOLEAN locked = FALSE;
    NTSTATUS status;

    count = GetCount();
    numRegisterDesc = 0;

    //
    // count number of register descriptors
    //
    for (i = 0; i < count; i++) {
        desc = GetDescriptor(i);
        if (desc == NULL) {
            status = STATUS_INVALID_DEVICE_STATE;
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                        "Resource Descriptor not found %!STATUS!", status);
            goto exit;
        }

        if (desc->Type == CmResourceTypeMemory ||
            desc->Type == CmResourceTypeMemoryLarge) {
            numRegisterDesc++;
        }
    }

    if (numRegisterDesc == 0) {
        return STATUS_SUCCESS;
    }

    //
    // allocate table
    //
    LockResourceTable();
    locked = TRUE;

    status =  FxRegisterResourceInfo::_CreateAndInit(
                                                    GetDriverGlobals(),
                                                    numRegisterDesc,
                                                    &m_RegisterResourceTable
                                                    );
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                    "Failed to allocate memory for resource table"
                    " %!STATUS!", status);
        goto exit;
    }
    m_RegisterResourceTableSizeCe = numRegisterDesc;

    //
    // Populate table
    //
    index = 0;
    for (i = 0; i < count; i++) {
        desc = GetDescriptor(i);
        if (desc == NULL) {
            status = STATUS_INVALID_DEVICE_STATE;
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                        "Resource Descriptor not found %!STATUS!", status);
            goto exit;
        }

        if (desc->Type == CmResourceTypeMemory ||
            desc->Type == CmResourceTypeMemoryLarge) {
            SIZE_T len;
            PHYSICAL_ADDRESS pa;

            //
            // This will populate Length and StartPa
            //
            len = GetResourceLength(desc, &pa);
            if (len) {
                m_RegisterResourceTable[index].SetPhysicalAddress(pa, len);
            }

            index++;
        }
    }

exit:

    if (!NT_SUCCESS(status)) {
        if (m_RegisterResourceTable != NULL) {
            delete [] m_RegisterResourceTable;
            m_RegisterResourceTable = NULL;
            m_RegisterResourceTableSizeCe = 0;
        }
    }

    if (locked) {
        UnlockResourceTable();
    }

    return status;
}

NTSTATUS
FxCmResList::BuildPortResourceTable(
    VOID
    )
{
    ULONG count;
    ULONG i, index;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR desc;
    ULONG numPortDesc;
    BOOLEAN locked = FALSE;
    NTSTATUS status;

    count = GetCount();
    numPortDesc = 0;

    //
    // count number of register descriptors
    //
    for (i = 0; i < count; i++) {
        desc = GetDescriptor(i);
        if (desc == NULL) {
            status = STATUS_INVALID_DEVICE_STATE;
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                        "Resource Descriptor not found %!STATUS!", status);
            goto exit;
        }

        if (desc->Type == CmResourceTypePort) {
            numPortDesc++;
        }
    }

    if (numPortDesc == 0) {
        return STATUS_SUCCESS;
    }

    //
    // allocate table
    //
    LockResourceTable();
    locked = TRUE;

    status =  FxPortResourceInfo::_CreateAndInit(
                                                GetDriverGlobals(),
                                                numPortDesc,
                                                &m_PortResourceTable
                                                );
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                    "Failed to allocate memory for resource table"
                    " %!STATUS!", status);
        goto exit;
    }
    m_PortResourceTableSizeCe = numPortDesc;

    //
    // Populate table
    //
    index = 0;
    for (i = 0; i < count; i++) {
        desc = GetDescriptor(i);
        if (desc == NULL) {
            status = STATUS_INVALID_DEVICE_STATE;
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                        "Resource Descriptor not found %!STATUS!", status);
            goto exit;
        }

        if (desc->Type == CmResourceTypePort) {
            SIZE_T len;
            PHYSICAL_ADDRESS pa;

            //
            // This will populate Length, StartPa and EndPa
            //
            len = GetResourceLength(desc, &pa);
            if (len) {
                m_PortResourceTable[index].SetPhysicalAddress(pa, len);
            }

            index++;
        }
    }

exit:

    if (!NT_SUCCESS(status)) {
        if (m_PortResourceTable != NULL) {
            delete [] m_PortResourceTable;
            m_PortResourceTable = NULL;
            m_PortResourceTableSizeCe = 0;
        }
    }

    if (locked) {
        UnlockResourceTable();
    }

    return status;
}


VOID
FxCmResList::UpdateRegisterResourceEntryLocked(
    __in FxRegisterResourceInfo* Entry,
    __in PVOID SystemMappedAddress,
    __in SIZE_T NumberOfBytes,
    __in PVOID UsermodeMappedAddress
    )
{
    Entry->SetMappedAddress(SystemMappedAddress, NumberOfBytes, UsermodeMappedAddress);
}

VOID
FxCmResList::ClearRegisterResourceEntryLocked(
    __in FxRegisterResourceInfo* Entry
    )
{
    Entry->ClearMappedAddress();
}

HRESULT
FxCmResList::ValidateRegisterPhysicalAddressRange (
    __in PHYSICAL_ADDRESS PhysicalAddress,
    __in SIZE_T Size,
    __out FxRegisterResourceInfo** TableEntry
    )
/*++

Routine Description:

    This routine checks whether the physical address range is part of the resources
    assigned to the device by pnp manager. It also returns the table entry
    corresponding to the physical address range from register resource table.

Arguments:

    PhysicalAddress - Supplies physical address to validate

    Size - Supplies size of address range in bytes.

    TableEntry - Supplies a pointer to store the table entry that corresponds to
                 this physical address.

Return Value:

    HRESULT

    S_OK if physical address is one assigned by pnp manager to this device.
    E_INAVLIDARG otherwise.

--*/
{
    ULONG i;
    HRESULT hr;
    ULONGLONG driverStartPa, driverEndPa, systemStartPa, systemEndPa;
    ULONGLONG tmp;
    FxRegisterResourceInfo* entry = NULL;

    *TableEntry = NULL;

    //
    // Physical address is of LONGLONG type (signed) we need to cast it to
    // ULONGLONG for comparision because in a LONGLONG comprison, the
    // result is different when highest bit is set vs when it is not set.
    //
    driverStartPa = PhysicalAddress.QuadPart;

    //
    //  driverEndPa = driverStartPa + Size - 1;
    //
    hr = ULongLongAdd(driverStartPa, Size, &tmp);
    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK("Integer overflow occurred"
        "when computing register address range", SUCCEEDED(hr)),
        GetDriverGlobals()->Public.DriverName);

    driverEndPa = tmp - 1;

    //
    // We allow one physical address range mapping only. The base address and
    // length can be flexible within the assigned range.
    //
    for (i = 0; i < m_RegisterResourceTableSizeCe; i++) {
        entry = &m_RegisterResourceTable[i];

        //
        // No need to do int overflow safe additon here since start address and
        // length are assigned by pnp manager. Note that we don't store endPa in
        // resource table the way we do for SystemVa is because endPa is not
        // needed in hot path so can be computed using length.
        //
        systemStartPa = entry->m_StartPa.QuadPart;
        systemEndPa  = systemStartPa + entry->m_Length - 1;

        if (driverStartPa >= systemStartPa &&
            driverEndPa <= systemEndPa) {

            FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK("Attempt to do multiple "
                "mapping of same resource, or multiple mapping in same resource"
                " range",
                (entry->m_StartSystemVa == NULL)), GetDriverGlobals()->Public.DriverName);
            FX_VERIFY_WITH_NAME(INTERNAL, CHECK_NULL(entry->m_EndSystemVa),
                GetDriverGlobals()->Public.DriverName);
            FX_VERIFY_WITH_NAME(INTERNAL, CHECK_NULL(entry->m_StartUsermodeVa),
                GetDriverGlobals()->Public.DriverName);
            FX_VERIFY_WITH_NAME(INTERNAL, CHECK("Mapped length not zero",
                (entry->m_MappedLength == 0)), GetDriverGlobals()->Public.DriverName);

            *TableEntry = entry;

            return S_OK;
        }
    }

    return E_INVALIDARG;
}

HRESULT
FxCmResList::ValidateAndClearMapping(
    __in PVOID Address,
    __in SIZE_T Length
    )
/*++

Routine Description:

    This routine checks whether the mapped system base address and size is part
    of the resources assigned to the device by pnp manager. If so it clears the
    system and usermode address mapping from the table.

Arguments:

    Address - Supplies system base address to validate

    Size - Supplies size of address range in bytes.

Return Value:

    HRESULT

    S_OK if system address is one mapped to a register resource.
    E_INAVLIDARG otherwise.

--*/
{
    HRESULT hr = E_INVALIDARG;
    ULONG i;
    FxRegisterResourceInfo* entry = NULL;

    LockResourceTable();

    for (i = 0; i < m_RegisterResourceTableSizeCe; i++) {
        entry = &m_RegisterResourceTable[i];

        if (NULL != entry->m_StartSystemVa &&
            Address == entry->m_StartSystemVa &&
            Length == entry->m_MappedLength) {
            //
            // there is a valid mapping. clear it.
            //
            FX_VERIFY_WITH_NAME(INTERNAL, CHECK_NOT_NULL(entry->m_EndSystemVa),
                GetDriverGlobals()->Public.DriverName);

            ClearRegisterResourceEntryLocked(entry);

            hr = S_OK;
            break;
        }
    }

    UnlockResourceTable();

    return hr;
}

HRESULT
FxCmResList::ValidateRegisterSystemBaseAddress (
    __in PVOID Address,
    __out PVOID* UsermodeBaseAddress
    )
/*++

Routine Description:

    This routine checks whether the mapped system base address and size is part
    of the resources assigned to the device by pnp manager. If so, it returns
    corresponding user-mode mapped base address. It is applicable
    only when registers are mapped to user-mode.

Arguments:

    Address - Supplies system base address to validate

Return Value:

    HRESULT

    S_OK if system address is one mapped to a register resource.
    E_INAVLIDARG otherwise.

--*/
{
    ULONG i;
    FxRegisterResourceInfo* entry = NULL;

    LockResourceTable();
    for (i = 0; i < m_RegisterResourceTableSizeCe; i++) {
        entry = &m_RegisterResourceTable[i];

        if (Address == entry->m_StartSystemVa) {

            FX_VERIFY_WITH_NAME(INTERNAL, CHECK_NOT_NULL(entry->m_StartUsermodeVa),
                GetDriverGlobals()->Public.DriverName);

            *UsermodeBaseAddress = entry->m_StartUsermodeVa;

            UnlockResourceTable();
            return S_OK;
        }
    }

    UnlockResourceTable();
    return E_INVALIDARG;
}

HRESULT
FxCmResList::ValidateRegisterSystemAddressRange (
    __in PVOID SystemAddress,
    __in SIZE_T Length,
    __out_opt PVOID* UsermodeAddress
    )
/*++

Routine Description:

    This routine checks whether given system mapped address and length is within
    one of the assigned resource ranges. Optionally, tt computes the usermode
    address corresponding to the system address.

Arguments:

    Address - Supplies register address to validate

    Size - Supplies size of address range in bytes.

    UsermodeAddress - returns usermode address corresponding to system address

Return Value:

    HRESULT

    S_OK if system address range is valid.
    E_INAVLIDARG otherwise.

--*/
{
    HRESULT hr = E_INVALIDARG;
    FxRegisterResourceInfo* entry = NULL;
    SIZE_T offset = 0;
    ULONG i;
    PVOID start = NULL;
    PVOID end  = NULL;
    ULONG_PTR tmp;

    //
    // compute system address range to look for
    //
    start = SystemAddress;

    //
    // Use interger overflow safe functions
    // end = ((PUCHAR)SystemAddress) + Length - 1;
    //
    hr = ULongPtrAdd((ULONG_PTR) start, Length, &tmp);
    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK("Integer overflow occurred"
        "when computing register address range", SUCCEEDED(hr)),
        GetDriverGlobals()->Public.DriverName);

    end = (PVOID)(tmp - 1);

    //
    // check if range is in the register resource table
    //
    hr = E_INVALIDARG;
    for (i = 0; i < m_RegisterResourceTableSizeCe; i++) {
        entry = &m_RegisterResourceTable[i];

        if (start >= entry->m_StartSystemVa &&
            end <= entry->m_EndSystemVa) {
            hr = S_OK;
            break;
        }
    }

    //
    // compute the corresponding usermode address
    //
    if (SUCCEEDED(hr) && UsermodeAddress != NULL) {
        offset = ((PUCHAR)SystemAddress) - ((PUCHAR)entry->m_StartSystemVa);
        *UsermodeAddress = ((PUCHAR)entry->m_StartUsermodeVa) + offset;
    }

    return hr;
}

SIZE_T
FxCmResList::GetResourceLength(
    __in PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    __out_opt PHYSICAL_ADDRESS* Start
    )
/*++

Routine Description:

    This routine decodes the length from a CmPartialResourceDescriptor
    describing a memory resource.

Arguments:

    Descriptor - Supplies resource descriptor from which to decode length

    Start - Supplies optional buffer into which start address will be stored.

Return Value:

    Decoded Length

--*/
{
    ULONGLONG length;

    length = 0;

    ASSERT((Descriptor->Type == CmResourceTypeMemory) ||
          (Descriptor->Type == CmResourceTypeMemoryLarge) ||
          (Descriptor->Type == CmResourceTypePort));

    //
    // If it is not large memory resource than length is in u.Memory.Length.
    // For large memory resource, the length is given by different fields in
    // CM_PARTIAL_RESOURCE_DESCRIPTOR structure.
    //
    if ((Descriptor->Type == CmResourceTypeMemory) ||
        (Descriptor->Type == CmResourceTypePort)) {
        length = Descriptor->u.Memory.Length;

    } else if (Descriptor->Flags & CM_RESOURCE_MEMORY_LARGE_40) {
        length = (((ULONGLONG)Descriptor->u.Memory40.Length40) << 8);

    } else if (Descriptor->Flags & CM_RESOURCE_MEMORY_LARGE_48) {
        length = (((ULONGLONG)Descriptor->u.Memory48.Length48) << 16);

    } else if (Descriptor->Flags & CM_RESOURCE_MEMORY_LARGE_64) {
        length = (((ULONGLONG)Descriptor->u.Memory64.Length64) << 32);

    } else {
        //
        // It should not be possible to get here.
        //
        ASSERT(FALSE);
    }

    if (Start != NULL) {
        *Start = Descriptor->u.Generic.Start;
    }

    //
    // large memory descriptor is only supported on 64-bit so the casting
    // below is ok.
    //
    return (SIZE_T) length;
}

HRESULT
FxCmResList::MapIoSpaceWorker(
    __in PHYSICAL_ADDRESS PhysicalAddress,
    __in SIZE_T NumberOfBytes,
    __in MEMORY_CACHING_TYPE  CacheType,
    __deref_out VOID** PseudoBaseAddress
    )
{
    IWudfDeviceStack *deviceStack;
    PVOID systemAddress;
    PVOID usermodeAddress;
    HRESULT hr;
    FxRegisterResourceInfo* resEntry;

    //
    // check if this physical resource is among the assigned resources.
    // If it is, retrieve the table entry corresponding to to register res.
    //
    LockResourceTable();

    hr = ValidateRegisterPhysicalAddressRange(PhysicalAddress,
                                              NumberOfBytes,
                                              &resEntry);

    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO),
        CHECK("Invalid physical address or number of bytes provided",
        (SUCCEEDED(hr))), GetDriverGlobals()->Public.DriverName);

    *PseudoBaseAddress = NULL;

    //
    // Call host
    //
    deviceStack = GetDevice()->GetDeviceStack();
    systemAddress = NULL;
    usermodeAddress = NULL;

    if(GetDevice()->AreRegistersMappedToUsermode()) {
        hr = deviceStack->MapIoSpace(PhysicalAddress,
                                NumberOfBytes,
                                CacheType,
                                &systemAddress,
                                &usermodeAddress);
    }
    else {
        hr = deviceStack->MapIoSpace(PhysicalAddress,
                                NumberOfBytes,
                                CacheType,
                                &systemAddress,
                                NULL);
    }

    if (SUCCEEDED(hr)) {
        //
        // update the mapped resource list entry and add it to list
        //
        UpdateRegisterResourceEntryLocked(resEntry,
                                          systemAddress,
                                          NumberOfBytes,
                                          usermodeAddress);

        //
        // Convert system address to pseudo (opaque) base address
        //
        *PseudoBaseAddress = GetDevice()->GetPseudoAddressFromSystemAddress(
                                                            systemAddress
                                                            );
    }

    UnlockResourceTable();

    return hr;
}

VOID
FxCmResList::ValidateResourceUnmap(
    VOID
    )
{
    ULONG i;
    FxRegisterResourceInfo* entry = NULL;

    //
    // make sure driver has unmapped its resources. No need to
    // acquire the resource validation table lock as this is called in
    // ReleaseHardware pnp callback and cannot race with another framework
    // pnp callback that updates this table (PrepareHardware) so no invalid
    // access. If a driver thread unmaps after ReleaseHardware return then also
    // it will be a valid access of table entry.
    //

    for (i = 0; i < m_RegisterResourceTableSizeCe; i++) {
        entry = &m_RegisterResourceTable[i];

        FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK("Driver did not unmap its "
            "register resources", (entry->m_StartSystemVa == NULL)), GetDriverGlobals()->Public.DriverName);
    }
}

HRESULT
FxCmResList::ValidatePortAddressRange(
    __in PVOID Address,
    __in SIZE_T Length
    )
{
    ULONG i;
    HRESULT hr;
    ULONGLONG driverStartPa, driverEndPa, systemStartPa, systemEndPa;
    ULONGLONG tmp;
    FxPortResourceInfo* entry = NULL;

    driverStartPa = (ULONGLONG)Address;

    //
    //  driverEndPa = driverStartPa + Length - 1;
    //
    hr = ULongLongAdd(driverStartPa, Length, &tmp);
    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK("Integer overflow occurred"
        "when computing port address range", SUCCEEDED(hr)),
        GetDriverGlobals()->Public.DriverName);

    driverEndPa = tmp - 1;

    for (i = 0; i < m_PortResourceTableSizeCe; i++) {
        entry = &m_PortResourceTable[i];

        systemStartPa = entry->m_StartPa.QuadPart;
        systemEndPa = entry->m_EndPa.QuadPart;

        if (driverStartPa >= systemStartPa  &&
            driverEndPa <= systemEndPa) {
            return S_OK;
        }
    }

    return E_INVALIDARG;
}

_Must_inspect_result_
NTSTATUS
FxCmResList::CheckForConnectionResources(
    VOID
    )
{
    NTSTATUS status;
    ULONG i;
    ULONG count;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pDescriptor;

    status = STATUS_SUCCESS;
    count = GetCount();

    for (i = 0; i < count; i++) {
        pDescriptor = GetDescriptor(i);
        if (pDescriptor == NULL) {
            status = STATUS_INVALID_DEVICE_STATE;
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                        "Resource Descriptor not found %!STATUS!", status);
            goto exit;
        }

        if (pDescriptor->Type == CmResourceTypeConnection) {
            m_HasConnectionResources = TRUE;
            break;
        }
    }

exit:
    return status;
}

