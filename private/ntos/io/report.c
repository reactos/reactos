/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    report.c

Abstract:

    This module contains the subroutines used to report resources used by
    the drivers and the HAL into the registry resource map.

Author:

    Andre Vachon (andreva) 15-Dec-1992

Environment:

    Kernel mode, local to I/O system

Revision History:


--*/

#include "iop.h"

#define DBG_AR 0

extern WCHAR IopWstrRaw[];
extern WCHAR IopWstrTranslated[];
extern WCHAR IopWstrBusTranslated[];
extern WCHAR IopWstrOtherDrivers[];

extern WCHAR IopWstrHal[];
extern WCHAR IopWstrSystem[];
extern WCHAR IopWstrPhysicalMemory[];
extern WCHAR IopWstrSpecialMemory[];

BOOLEAN
IopChangeInterfaceType(
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST IoResources,
    IN OUT PCM_RESOURCE_LIST *AllocatedResource
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IoReportResourceUsageInternal)
#pragma alloc_text(PAGE, IoReportResourceUsage)
#pragma alloc_text(PAGE, IoReportResourceForDetection)
#pragma alloc_text(PAGE, IopChangeInterfaceType)
#pragma alloc_text(PAGE, IopWriteResourceList)
#pragma alloc_text(INIT, IopInitializeResourceMap)
#pragma alloc_text(INIT, IoReportHalResourceUsage)
// BUGBUG - For now we want the following two dbg routines for free build.
//          So, we can track resource allocation on free build by enabling
//          a debug flag.
//#if DBG
#pragma alloc_text(PAGE, IopDumpCmResourceDescriptor)
#pragma alloc_text(PAGE, IopDumpCmResourceList)
//#endif
#endif


VOID
IopInitializeResourceMap (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

    Initializes the resource map by adding in the physical memory
    which is in use by the system.

--*/
{
    ULONG i, j, pass, length;
    LARGE_INTEGER li;
    HANDLE keyHandle;
    UNICODE_STRING  unicodeString, systemString, listString;
    NTSTATUS status;
    PCM_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor;
    BOOLEAN IncludeType[LoaderMaximum];
    ULONG MemoryAlloc[(sizeof(PHYSICAL_MEMORY_DESCRIPTOR) +
            sizeof(PHYSICAL_MEMORY_RUN)*MAX_PHYSICAL_MEMORY_FRAGMENTS) /
              sizeof(ULONG)];
    PPHYSICAL_MEMORY_DESCRIPTOR MemoryBlock;

    RtlInitUnicodeString( &systemString,  IopWstrSystem);
    RtlInitUnicodeString( &listString, IopWstrTranslated );

    for (pass=0; pass < 2; pass++) {
        switch (pass) {
            case 0:
                //
                // Add MmPhysicalMemoryBlock to regitry
                //

                RtlInitUnicodeString( &unicodeString, IopWstrPhysicalMemory);
                MemoryBlock = MmPhysicalMemoryBlock;
                break;

            case 1:

                //
                // Add LoadSpecialMemory to registry
                //

                RtlInitUnicodeString( &unicodeString, IopWstrSpecialMemory);

                //
                // Computer memory limits of LoaderSpecialMemory
                //

                MemoryBlock = (PPHYSICAL_MEMORY_DESCRIPTOR)&MemoryAlloc;
                MemoryBlock->NumberOfRuns = MAX_PHYSICAL_MEMORY_FRAGMENTS;

                for (j=0; j < LoaderMaximum; j++) {
                    IncludeType[j] = FALSE;
                }
                IncludeType[LoaderSpecialMemory] = TRUE;
                MmInitializeMemoryLimits(
                    LoaderBlock,
                    IncludeType,
                    MemoryBlock
                    );

                break;
        }

        //
        // Allocate and build a CM_RESOURCE_LIST to describe all
        // of physical memory
        //

        j = MemoryBlock->NumberOfRuns;
        if (j == 0) {
            continue;
        }

        length = sizeof(CM_RESOURCE_LIST) + (j-1) * sizeof (CM_PARTIAL_RESOURCE_DESCRIPTOR);
        ResourceList = (PCM_RESOURCE_LIST) ExAllocatePool (PagedPool, length);
        if (!ResourceList) {
            return;
        }
        RtlZeroMemory ((PVOID) ResourceList, length);

        ResourceList->Count = 1;
        ResourceList->List[0].PartialResourceList.Count = j;
        CmDescriptor = ResourceList->List[0].PartialResourceList.PartialDescriptors;

        for (i=0; i < j; i++) {
            CmDescriptor->Type = CmResourceTypeMemory;
            CmDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
            li.QuadPart = (LONGLONG)(MemoryBlock->Run[i].BasePage);
            li.QuadPart <<= PAGE_SHIFT;
            CmDescriptor->u.Memory.Start  = li;

            // fixfix - handle page frame numbers greater than 32 bits.

            CmDescriptor->u.Memory.Length =
                (ULONG)(MemoryBlock->Run[i].PageCount << PAGE_SHIFT);

            CmDescriptor++;
        }


        //
        // Add the resoruce list to the resorucemap
        //

        status = IopOpenRegistryKey( &keyHandle,
                                     (HANDLE) NULL,
                                     &CmRegistryMachineHardwareResourceMapName,
                                     KEY_READ | KEY_WRITE,
                                     TRUE );
        if (NT_SUCCESS( status )) {
            IopWriteResourceList ( keyHandle,
                                   &systemString,
                                   &unicodeString,
                                   &listString,
                                   ResourceList,
                                   length
                                   );
            ZwClose( keyHandle );
        }
        ExFreePool (ResourceList);
    }
}


NTSTATUS
IoReportHalResourceUsage(
    IN PUNICODE_STRING HalName,
    IN PCM_RESOURCE_LIST RawResourceList,
    IN PCM_RESOURCE_LIST TranslatedResourceList,
    IN ULONG ResourceListSize
    )

/*++

Routine Description:

    This routine is called by the HAL to report its resources.
    The Hal is the first component to report its resources, so we don't need
    to acquire the resourcemap semaphore, and we do not need to check for
    conflicts.

Arguments:

    HalName - Name of the HAL reporting the resources.

    RawResourceList - Pointer to the HAL's raw resource list.

    TranslatedResourceList - Pointer to the HAL's translated resource list.

    DriverListSize - Value determining the size of the HAL's resource list.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    HANDLE keyHandle;
    UNICODE_STRING halString;
    UNICODE_STRING listString;
    NTSTATUS status;

    PAGED_CODE();

    //
    // First open a handle to the RESOURCEMAP key.
    //

    RtlInitUnicodeString( &halString, IopWstrHal );

    status = IopOpenRegistryKey( &keyHandle,
                                 (HANDLE) NULL,
                                 &CmRegistryMachineHardwareResourceMapName,
                                 KEY_READ | KEY_WRITE,
                                 TRUE );

    //
    // Write out the raw resource list
    //

    if (NT_SUCCESS( status )) {

        RtlInitUnicodeString( &listString, IopWstrRaw);

        status = IopWriteResourceList( keyHandle,
                                       &halString,
                                       HalName,
                                       &listString,
                                       RawResourceList,
                                       ResourceListSize );

        //
        // If we successfully wrote out the raw resource list, write out
        // the translated resource list.
        //

        if (NT_SUCCESS( status )) {

            RtlInitUnicodeString( &listString, IopWstrTranslated);
            status = IopWriteResourceList( keyHandle,
                                           &halString,
                                           HalName,
                                           &listString,
                                           TranslatedResourceList,
                                           ResourceListSize );

        }
        ZwClose( keyHandle );
    }

    //
    // If every looks fine, we will make a copy of the Hal resources so will
    // can call Arbiters to reserve the resources after they initialized.
    //

    if (NT_SUCCESS(status)) {
        IopInitHalResources = (PCM_RESOURCE_LIST) ExAllocatePool(PagedPool, ResourceListSize);
        if (IopInitHalResources) {
            RtlMoveMemory(IopInitHalResources, RawResourceList, ResourceListSize);
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return status;
}

NTSTATUS
IoReportResourceForDetection(
    IN PDRIVER_OBJECT DriverObject,
    IN PCM_RESOURCE_LIST DriverList OPTIONAL,
    IN ULONG DriverListSize OPTIONAL,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN PCM_RESOURCE_LIST DeviceList OPTIONAL,
    IN ULONG DeviceListSize OPTIONAL,
    OUT PBOOLEAN ConflictDetected
    )

/*++

Routine Description:

    This routine will automatically search through the configuration
    registry for resource conflicts between resources requested by a device
    and the resources already claimed by previously installed drivers. The
    contents of the DriverList and the DeviceList will be matched against
    all the other resource list stored in the registry to determine
    conflicts.

    The function may be called more than once for a given device or driver.
    If a new resource list is given, the previous resource list stored in
    the registry will be replaced by the new list.

    Note, this function is for the drivers acquiring resources for detection.

Arguments:

    DriverObject - Pointer to the driver's driver object.

    DriverList - Optional pointer to the driver's resource list.

    DriverListSize - Optional value determining the size of the driver's
        resource list.

    DeviceObject - Optional pointer to driver's device object.

    DeviceList - Optional pointer to the device's resource list.

    DriverListSize - Optional value determining the size of the device's
        resource list.

    ConflictDetected - Supplies a pointer to a boolean that is set to TRUE
        if the resource list conflicts with an already existing resource
        list in the configuration registry.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    //
    // Sanity check that the caller did not pass in a PnP PDO.
    //

    if (DeviceObject) {

        if (    DeviceObject->DeviceObjectExtension->DeviceNode &&
                !(((PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode)->Flags & DNF_LEGACY_RESOURCE_DEVICENODE)) {

            KeBugCheckEx(PNP_DETECTED_FATAL_ERROR, PNP_ERR_INVALID_PDO, (ULONG_PTR)DeviceObject, 0, 0);

        }

    }

    return IoReportResourceUsageInternal(   ArbiterRequestPnpDetected,
                                            NULL,
                                            DriverObject,
                                            DriverList,
                                            DriverListSize,
                                            DeviceObject,
                                            DeviceList,
                                            DeviceListSize,
                                            FALSE,
                                            ConflictDetected);
}

NTSTATUS
IoReportResourceUsage(
    IN PUNICODE_STRING DriverClassName OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PCM_RESOURCE_LIST DriverList OPTIONAL,
    IN ULONG DriverListSize OPTIONAL,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN PCM_RESOURCE_LIST DeviceList OPTIONAL,
    IN ULONG DeviceListSize OPTIONAL,
    IN BOOLEAN OverrideConflict,
    OUT PBOOLEAN ConflictDetected
    )

/*++

Routine Description:

    This routine will automatically search through the configuration
    registry for resource conflicts between resources requested by a device
    and the resources already claimed by previously installed drivers. The
    contents of the DriverList and the DeviceList will be matched against
    all the other resource list stored in the registry to determine
    conflicts.

    If not conflict was detected, or if the OverrideConflict flag is set,
    this routine will create appropriate entries in the system resource map
    (in the registry) that will contain the specified resource lists.

    The function may be called more than once for a given device or driver.
    If a new resource list is given, the previous resource list stored in
    the registry will be replaced by the new list.

Arguments:

    DriverClassName - Optional pointer to a UNICODE_STRING which describes
        the class of driver under which the driver information should be
        stored. A default type is used if none is given.

    DriverObject - Pointer to the driver's driver object.

    DriverList - Optional pointer to the driver's resource list.

    DriverListSize - Optional value determining the size of the driver's
        resource list.

    DeviceObject - Optional pointer to driver's device object.

    DeviceList - Optional pointer to the device's resource list.

    DriverListSize - Optional value determining the size of the driver's
        resource list.

    OverrideConflict - Determines if the information should be reported
        in the configuration registry eventhough a conflict was found with
        another driver or device.

    ConflictDetected - Supplies a pointer to a boolean that is set to TRUE
        if the resource list conflicts with an already existing resource
        list in the configuration registry.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    if (DeviceObject) {

        if (    DeviceObject->DeviceObjectExtension->DeviceNode &&
                !(((PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode)->Flags & DNF_LEGACY_RESOURCE_DEVICENODE)) {

            KeBugCheckEx(PNP_DETECTED_FATAL_ERROR, PNP_ERR_INVALID_PDO, (ULONG_PTR)DeviceObject, 0, 0);

        }

    }

    return IoReportResourceUsageInternal(   ArbiterRequestLegacyReported,
                                            DriverClassName,
                                            DriverObject,
                                            DriverList,
                                            DriverListSize,
                                            DeviceObject,
                                            DeviceList,
                                            DeviceListSize,
                                            OverrideConflict,
                                            ConflictDetected);
}

NTSTATUS
IoReportResourceUsageInternal(
    IN ARBITER_REQUEST_SOURCE AllocationType,
    IN PUNICODE_STRING DriverClassName OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PCM_RESOURCE_LIST DriverList OPTIONAL,
    IN ULONG DriverListSize OPTIONAL,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN PCM_RESOURCE_LIST DeviceList OPTIONAL,
    IN ULONG DeviceListSize OPTIONAL,
    IN BOOLEAN OverrideConflict,
    OUT PBOOLEAN ConflictDetected
    )

/*++

Routine Description:

    This internal routine will do all the work for IoReportResourceUsage.

Arguments:

    AllocationType - Specifies the request type.

    DriverClassName - Optional pointer to a UNICODE_STRING which describes
        the class of driver under which the driver information should be
        stored. A default type is used if none is given.

    DriverObject - Pointer to the driver's driver object.

    DriverList - Optional pointer to the driver's resource list.

    DriverListSize - Optional value determining the size of the driver's
        resource list.

    DeviceObject - Optional pointer to driver's device object.

    DeviceList - Optional pointer to the device's resource list.

    DriverListSize - Optional value determining the size of the driver's
        resource list.

    OverrideConflict - Determines if the information should be reported
        in the configuration registry eventhough a conflict was found with
        another driver or device.

    ConflictDetected - Supplies a pointer to a boolean that is set to TRUE
        if the resource list conflicts with an already existing resource
        list in the configuration registry.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    NTSTATUS                        status = STATUS_UNSUCCESSFUL;
    PCM_RESOURCE_LIST               resourceList;
    PCM_RESOURCE_LIST               allocatedResources;
    PIO_RESOURCE_REQUIREMENTS_LIST  resourceRequirements;
    ULONG                           attempt;
    BOOLEAN                         freeAllocatedResources;

    ASSERT(DriverObject && ConflictDetected);

    if (DeviceList) {

        resourceList = DeviceList;

    } else if (DriverList) {

        resourceList = DriverList;

    } else {

        resourceList = NULL;

    }

    resourceRequirements = NULL;

    if (resourceList) {

        if (resourceList->Count && resourceList->List[0].PartialResourceList.Count) {

            resourceRequirements = IopCmResourcesToIoResources (0, resourceList, LCPRI_NORMAL);

            if (resourceRequirements == NULL) {

                return status;

            }

        } else {

            resourceList = NULL;

        }

    }

    *ConflictDetected = TRUE;
    attempt = 0;
    allocatedResources = resourceList;
    freeAllocatedResources = FALSE;
    do {

        //
        // Do the legacy resource allocation.
        //

        status = IopLegacyResourceAllocation (  AllocationType,
                                                DriverObject,
                                                DeviceObject,
                                                resourceRequirements,
                                                &allocatedResources);

        if (NT_SUCCESS(status)) {

            *ConflictDetected = FALSE;
            break;
        }

        //
        // Change the interface type and try again.
        //

        if (!IopChangeInterfaceType(resourceRequirements, &allocatedResources)) {

            break;
        }
        freeAllocatedResources = TRUE;

    } while (++attempt < 2);

    if (resourceRequirements) {

        ExFreePool(resourceRequirements);

    }

    if (freeAllocatedResources) {

        ExFreePool(allocatedResources);
    }

    if (NT_SUCCESS(status)) {

        status = STATUS_SUCCESS;

    } else {

        status = STATUS_INSUFFICIENT_RESOURCES;

    }

    return status;
}

BOOLEAN
IopChangeInterfaceType(
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST IoResources,
    IN OUT PCM_RESOURCE_LIST *AllocatedResources
    )

/*++

Routine Description:

    This routine takes an Io resourcelist and changes its interfacetype
    from internal to default type (isa or eisa or mca).

Arguments:

    IoResources - Pointer to requirement list.

    AllocatedResources - Pointer to a variable that receives the pointer to the resource list.

Return Value:

    BOOLEAN value to indicates if change made or not.

--*/

{
    PIO_RESOURCE_LIST       IoResourceList;
    PIO_RESOURCE_DESCRIPTOR IoResourceDescriptor;
    PIO_RESOURCE_DESCRIPTOR IoResourceDescriptorEnd;
    LONG                    IoResourceListCount;
    BOOLEAN                 changed;

    ASSERT(AllocatedResources);

    changed = FALSE;

    if (!IoResources) {

        return changed;

    }

    if (IoResources->InterfaceType == Internal) {

        IoResources->InterfaceType = PnpDefaultInterfaceType;
        changed = TRUE;

    }

    IoResourceList = IoResources->List;
    IoResourceListCount = IoResources->AlternativeLists;
    while (--IoResourceListCount >= 0) {

        IoResourceDescriptor = IoResourceList->Descriptors;
        IoResourceDescriptorEnd = IoResourceDescriptor + IoResourceList->Count;

        for (;IoResourceDescriptor < IoResourceDescriptorEnd; IoResourceDescriptor++) {

            if (IoResourceDescriptor->Type == CmResourceTypeReserved &&
                IoResourceDescriptor->u.DevicePrivate.Data[0] == Internal) {

                IoResourceDescriptor->u.DevicePrivate.Data[0] = PnpDefaultInterfaceType;
                changed = TRUE;

            }
        }
        IoResourceList = (PIO_RESOURCE_LIST) IoResourceDescriptorEnd;
    }

    if (changed) {

        PCM_RESOURCE_LIST               oldResources = *AllocatedResources;
        PCM_RESOURCE_LIST               newResources;
        PCM_FULL_RESOURCE_DESCRIPTOR    cmFullDesc;
        PCM_PARTIAL_RESOURCE_DESCRIPTOR cmPartDesc;
        ULONG                           size;

        if (oldResources) {

            size = IopDetermineResourceListSize(oldResources);
            newResources = ExAllocatePool(PagedPool, size);
            if (newResources == NULL) {

                changed = FALSE;

            } else {

                ULONG   i;
                ULONG   j;


                RtlCopyMemory(newResources, oldResources, size);

                //
                // Fix up the interface type
                //

                cmFullDesc = &newResources->List[0];
                for (i = 0; i < oldResources->Count; i++) {

                    if (cmFullDesc->InterfaceType == Internal) {

                        cmFullDesc->InterfaceType = PnpDefaultInterfaceType;

                    }
                    cmPartDesc = &cmFullDesc->PartialResourceList.PartialDescriptors[0];
                    for (j = 0; j < cmFullDesc->PartialResourceList.Count; j++) {

                        size = 0;
                        switch (cmPartDesc->Type) {

                        case CmResourceTypeDeviceSpecific:
                            size = cmPartDesc->u.DeviceSpecificData.DataSize;
                            break;

                        }
                        cmPartDesc++;
                        cmPartDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)cmPartDesc + size);
                    }

                    cmFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)cmPartDesc;
                }

                *AllocatedResources = newResources;
            }
        }
    }

    return changed;
}

NTSTATUS
IopWriteResourceList(
    HANDLE ResourceMapKey,
    PUNICODE_STRING ClassName,
    PUNICODE_STRING DriverName,
    PUNICODE_STRING DeviceName,
    PCM_RESOURCE_LIST ResourceList,
    ULONG ResourceListSize
    )

/*++

Routine Description:

    This routine takes a resourcelist and stores it in the registry resource
    map, using the ClassName, DriverName and DeviceName as the path of the
    key to store it in.

Arguments:

    ResourceMapKey - Handle to the root of the resource map.

    ClassName - Pointer to a Unicode String that contains the name of the Class
        for this resource list.

    DriverName - Pointer to a Unicode String that contains the name of the
        Driver for this resource list.

    DeviceName - Pointer to a Unicode String that contains the name of the
        Device for this resource list.

    ResourceList - P to the resource list.

    ResourceListSize - Value determining the size of the resource list.

Return Value:

    The status returned is the final completion status of the operation.

--*/


{
    NTSTATUS status;
    HANDLE classKeyHandle;
    HANDLE driverKeyHandle;

    PAGED_CODE();

    status = IopOpenRegistryKey( &classKeyHandle,
                                 ResourceMapKey,
                                 ClassName,
                                 KEY_READ | KEY_WRITE,
                                 TRUE );

    if (NT_SUCCESS( status )) {

        //
        // Take the resulting name to create the key.
        //

        status = IopOpenRegistryKey( &driverKeyHandle,
                                     classKeyHandle,
                                     DriverName,
                                     KEY_READ | KEY_WRITE,
                                     TRUE );

        ZwClose( classKeyHandle );


        if (NT_SUCCESS( status )) {

            //
            // With this key handle, we can now store the required information
            // in the value entries of the key.
            //

            //
            // Store the device name as a value name and the device information
            // as the rest of the data.
            // Only store the information if the CM_RESOURCE_LIST was present.
            //

            if (ResourceList->Count == 0) {

                status = ZwDeleteValueKey( driverKeyHandle,
                                           DeviceName );

            } else {

                status = ZwSetValueKey( driverKeyHandle,
                                        DeviceName,
                                        0L,
                                        REG_RESOURCE_LIST,
                                        ResourceList,
                                        ResourceListSize );

            }

            ZwClose( driverKeyHandle );

        }
    }

    return status;
}
//#if DBG

VOID
IopDumpCmResourceDescriptor (
    IN PUCHAR Indent,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Desc
    )
/*++

Routine Description:

    This routine processes a IO_RESOURCE_DESCRIPTOR and displays it.

Arguments:

    Indent - # char of indentation.

    Desc - supplies a pointer to the IO_RESOURCE_DESCRIPTOR to be displayed.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    switch (Desc->Type) {
        case CmResourceTypePort:
            DbgPrint ("%sIO  Start: %x:%08x, Length:  %x\n",
                Indent,
                Desc->u.Port.Start.HighPart, Desc->u.Port.Start.LowPart,
                Desc->u.Port.Length
                );
            break;

        case CmResourceTypeMemory:
            DbgPrint ("%sMEM Start: %x:%08x, Length:  %x\n",
                Indent,
                Desc->u.Memory.Start.HighPart, Desc->u.Memory.Start.LowPart,
                Desc->u.Memory.Length
                );
            break;

        case CmResourceTypeInterrupt:
            DbgPrint ("%sINT Level: %x, Vector: %x, Affinity: %x\n",
                Indent,
                Desc->u.Interrupt.Level,
                Desc->u.Interrupt.Vector,
                Desc->u.Interrupt.Affinity
                );
            break;

        case CmResourceTypeDma:
            DbgPrint ("%sDMA Channel: %x, Port: %x\n",
                Indent,
                Desc->u.Dma.Channel,
                Desc->u.Dma.Port
                );
            break;
    }
}

VOID
IopDumpCmResourceList (
    IN PCM_RESOURCE_LIST CmList
    )
/*++

Routine Description:

    This routine displays CM resource list.

Arguments:

    CmList - supplies a pointer to CM resource list

Return Value:

    None.

--*/
{
    PCM_FULL_RESOURCE_DESCRIPTOR fullDesc;
    PCM_PARTIAL_RESOURCE_LIST partialDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR desc;
    ULONG count, i;

    PAGED_CODE();

    if (CmList->Count > 0) {
        if (CmList) {
            fullDesc = &CmList->List[0];
            DbgPrint("Cm Resource List -\n");
            DbgPrint("  List Count = %x, Bus Number = %x\n", CmList->Count, fullDesc->BusNumber);
            partialDesc = &fullDesc->PartialResourceList;
            DbgPrint("  Version = %x, Revision = %x, Desc count = %x\n", partialDesc->Version,
                     partialDesc->Revision, partialDesc->Count);
            count = partialDesc->Count;
            desc = &partialDesc->PartialDescriptors[0];
            for (i = 0; i < count; i++) {
                IopDumpCmResourceDescriptor("    ", desc);
                desc++;
            }
        }
    }
}
//#endif
