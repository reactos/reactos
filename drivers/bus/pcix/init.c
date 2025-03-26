/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/init.c
 * PURPOSE:         Driver Initialization
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

BOOLEAN PciRunningDatacenter;
PDRIVER_OBJECT PciDriverObject;
KEVENT PciGlobalLock;
KEVENT PciBusLock;
KEVENT PciLegacyDescriptionLock;
BOOLEAN PciLockDeviceResources;
BOOLEAN PciEnableNativeModeATA;
ULONG PciSystemWideHackFlags;
PPCI_IRQ_ROUTING_TABLE PciIrqRoutingTable;
PWATCHDOG_TABLE WdTable;
PPCI_HACK_ENTRY PciHackTable;

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
PciAcpiFindRsdt(OUT PACPI_BIOS_MULTI_NODE *AcpiMultiNode)
{
    BOOLEAN Result;
    NTSTATUS Status;
    HANDLE KeyHandle, SubKey;
    ULONG NumberOfBytes, i, Length;
    PKEY_FULL_INFORMATION FullInfo;
    PKEY_BASIC_INFORMATION KeyInfo;
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
    PACPI_BIOS_MULTI_NODE NodeData;
    UNICODE_STRING ValueName;
    struct
    {
        CM_FULL_RESOURCE_DESCRIPTOR Descriptor;
        ACPI_BIOS_MULTI_NODE Node;
    } *Package;

    /* So we know what to free at the end of the body */
    ValueInfo = NULL;
    KeyInfo = NULL;
    KeyHandle = NULL;
    FullInfo = NULL;
    Package = NULL;
    do
    {
        /* Open the ACPI BIOS key */
        Result = PciOpenKey(L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\"
                            L"System\\MultiFunctionAdapter",
                            NULL,
                            KEY_QUERY_VALUE,
                            &KeyHandle,
                            &Status);
        if (!Result) break;

        /* Query how much space should be allocated for the key information */
        Status = ZwQueryKey(KeyHandle,
                            KeyFullInformation,
                            NULL,
                            sizeof(ULONG),
                            &NumberOfBytes);
        if (Status != STATUS_BUFFER_TOO_SMALL) break;

        /* Allocate the space required */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        FullInfo = ExAllocatePoolWithTag(PagedPool, NumberOfBytes, PCI_POOL_TAG);
        if ( !FullInfo ) break;

        /* Now query the key information that's needed */
        Status = ZwQueryKey(KeyHandle,
                            KeyFullInformation,
                            FullInfo,
                            NumberOfBytes,
                            &NumberOfBytes);
        if (!NT_SUCCESS(Status)) break;

        /* Allocate enough space to hold the value information plus the name */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        Length = FullInfo->MaxNameLen + 26;
        KeyInfo = ExAllocatePoolWithTag(PagedPool, Length, PCI_POOL_TAG);
        if ( !KeyInfo ) break;

        /* Allocate the value information and name we expect to find */
        ValueInfo = ExAllocatePoolWithTag(PagedPool,
                                          sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                                          sizeof(L"ACPI BIOS"),
                                          PCI_POOL_TAG);
        if (!ValueInfo) break;

        /* Loop each sub-key */
        i = 0;
        while (TRUE)
        {
            /* Query each sub-key */
            Status = ZwEnumerateKey(KeyHandle,
                                    i++,
                                    KeyBasicInformation,
                                    KeyInfo,
                                    Length,
                                    &NumberOfBytes);
            if (Status == STATUS_NO_MORE_ENTRIES) break;

            /* Null-terminate the keyname, because the kernel does not */
            KeyInfo->Name[KeyInfo->NameLength / sizeof(WCHAR)] = UNICODE_NULL;

            /* Open this subkey */
            Result = PciOpenKey(KeyInfo->Name,
                                KeyHandle,
                                KEY_QUERY_VALUE,
                                &SubKey,
                                &Status);
            if (Result)
            {
                /* Query the identifier value for this subkey */
                RtlInitUnicodeString(&ValueName, L"Identifier");
                Status = ZwQueryValueKey(SubKey,
                                         &ValueName,
                                         KeyValuePartialInformation,
                                         ValueInfo,
                                         sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                                         sizeof(L"ACPI BIOS"),
                                         &NumberOfBytes);
                if (NT_SUCCESS(Status))
                {
                    /* Check if this is the PCI BIOS subkey */
                    if (!wcsncmp((PWCHAR)ValueInfo->Data,
                                 L"ACPI BIOS",
                                 ValueInfo->DataLength))
                    {
                        /* It is, proceed to query the PCI IRQ routing table */
                        Status = PciGetRegistryValue(L"Configuration Data",
                                                     KeyInfo->Name,
                                                     KeyHandle,
                                                     REG_FULL_RESOURCE_DESCRIPTOR,
                                                     (PVOID*)&Package,
                                                     &NumberOfBytes);
                        ZwClose(SubKey);
                        break;
                    }
                }

                /* Close the subkey and try the next one */
                ZwClose(SubKey);
            }
        }

        /* Check if we got here because the routing table was found */
        if (!NT_SUCCESS(Status))
        {
            /* This should only fail if we're out of entries */
            ASSERT(Status == STATUS_NO_MORE_ENTRIES);
            break;
        }

        /* Check if a descriptor was found */
        if (!Package) break;

        /* The configuration data is a resource list, and the BIOS node follows */
        NodeData = &Package->Node;

        /* How many E820 memory entries are there? */
        Length = sizeof(ACPI_BIOS_MULTI_NODE) +
                 (NodeData->Count - 1) * sizeof(ACPI_E820_ENTRY);

        /* Allocate the buffer needed to copy the information */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        *AcpiMultiNode = ExAllocatePoolWithTag(NonPagedPool, Length, PCI_POOL_TAG);
        if (!*AcpiMultiNode) break;

        /* Copy the data */
        RtlCopyMemory(*AcpiMultiNode, NodeData, Length);
        Status = STATUS_SUCCESS;
    } while (FALSE);

    /* Close any opened keys, free temporary allocations, and return status */
    if (Package) ExFreePoolWithTag(Package, 0);
    if (ValueInfo) ExFreePoolWithTag(ValueInfo, 0);
    if (KeyInfo) ExFreePoolWithTag(KeyInfo, 0);
    if (FullInfo) ExFreePoolWithTag(FullInfo, 0);
    if (KeyHandle) ZwClose(KeyHandle);
    return Status;
}

PVOID
NTAPI
PciGetAcpiTable(IN ULONG TableCode)
{
    PDESCRIPTION_HEADER Header;
    PACPI_BIOS_MULTI_NODE AcpiMultiNode;
    PRSDT Rsdt;
    PXSDT Xsdt;
    ULONG EntryCount, TableLength, Offset, CurrentEntry;
    PVOID TableBuffer, MappedAddress;
    PHYSICAL_ADDRESS PhysicalAddress;
    NTSTATUS Status;

    /* Try to find the RSDT or XSDT */
    Status = PciAcpiFindRsdt(&AcpiMultiNode);
    if (!NT_SUCCESS(Status))
    {
        /* No ACPI on the machine */
        DPRINT1("AcpiFindRsdt() Failed!\n");
        return NULL;
    }

    /* Map the RSDT with the minimum size allowed */
    MappedAddress = MmMapIoSpace(AcpiMultiNode->RsdtAddress,
                                 sizeof(DESCRIPTION_HEADER),
                                 MmNonCached);
    Header = MappedAddress;
    if (!Header) return NULL;

    /* Check how big the table really is and get rid of the temporary header */
    TableLength = Header->Length;
    MmUnmapIoSpace(Header, sizeof(DESCRIPTION_HEADER));
    Header = NULL;

    /* Map its true size */
    MappedAddress = MmMapIoSpace(AcpiMultiNode->RsdtAddress,
                                 TableLength,
                                 MmNonCached);
    Rsdt = MappedAddress;
    Xsdt = MappedAddress;
    ExFreePoolWithTag(AcpiMultiNode, 0);
    if (!Rsdt) return NULL;

    /* Validate the table's signature */
    if ((Rsdt->Header.Signature != RSDT_SIGNATURE) &&
        (Rsdt->Header.Signature != XSDT_SIGNATURE))
    {
        /* Very bad: crash */
        HalDisplayString("RSDT table contains invalid signature\r\n");
        MmUnmapIoSpace(Rsdt, TableLength);
        return NULL;
    }

    /* Smallest RSDT/XSDT is one without table entries */
    Offset = FIELD_OFFSET(RSDT, Tables);
    if (Rsdt->Header.Signature == XSDT_SIGNATURE)
    {
        /* Figure out total size of table and the offset */
        TableLength = Xsdt->Header.Length;
        if (TableLength < Offset) Offset = Xsdt->Header.Length;

        /* The entries are each 64-bits, so count them */
        EntryCount = (TableLength - Offset) / sizeof(PHYSICAL_ADDRESS);
    }
    else
    {
        /* Figure out total size of table and the offset */
        TableLength = Rsdt->Header.Length;
        if (TableLength < Offset) Offset = Rsdt->Header.Length;

        /* The entries are each 32-bits, so count them */
        EntryCount = (TableLength - Offset) / sizeof(ULONG);
    }

    /* Start at the beginning of the array and loop it */
    for (CurrentEntry = 0; CurrentEntry < EntryCount; CurrentEntry++)
    {
        /* Are we using the XSDT? */
        if (Rsdt->Header.Signature != XSDT_SIGNATURE)
        {
            /* Read the 32-bit physical address */
            PhysicalAddress.QuadPart = Rsdt->Tables[CurrentEntry];
        }
        else
        {
            /* Read the 64-bit physical address */
            PhysicalAddress = Xsdt->Tables[CurrentEntry];
        }

        /* Map this table */
        Header = MmMapIoSpace(PhysicalAddress,
                              sizeof(DESCRIPTION_HEADER),
                              MmNonCached);
        if (!Header) break;

        /* Check if this is the table that's being asked for */
        if (Header->Signature == TableCode)
        {
            /* Allocate a buffer for it */
            TableBuffer = ExAllocatePoolWithTag(PagedPool,
                                                Header->Length,
                                                PCI_POOL_TAG);
            if (!TableBuffer) break;

            /* Copy the table into the buffer */
            RtlCopyMemory(TableBuffer, Header, Header->Length);
        }

        /* Done with this table, keep going */
        MmUnmapIoSpace(Header, sizeof(DESCRIPTION_HEADER));
    }

    if (Header) MmUnmapIoSpace(Header, sizeof(DESCRIPTION_HEADER));
    return NULL;
}

NTSTATUS
NTAPI
PciGetIrqRoutingTableFromRegistry(OUT PPCI_IRQ_ROUTING_TABLE *PciRoutingTable)
{
    BOOLEAN Result;
    NTSTATUS Status;
    HANDLE KeyHandle, SubKey;
    ULONG NumberOfBytes, i, Length;
    PKEY_FULL_INFORMATION FullInfo;
    PKEY_BASIC_INFORMATION KeyInfo;
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
    UNICODE_STRING ValueName;
    struct
    {
        CM_FULL_RESOURCE_DESCRIPTOR Descriptor;
        PCI_IRQ_ROUTING_TABLE Table;
    } *Package;

    /* So we know what to free at the end of the body */
    Package = NULL;
    ValueInfo = NULL;
    KeyInfo = NULL;
    KeyHandle = NULL;
    FullInfo = NULL;
    do
    {
        /* Open the BIOS key */
        Result = PciOpenKey(L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\"
                            L"System\\MultiFunctionAdapter",
                            NULL,
                            KEY_QUERY_VALUE,
                            &KeyHandle,
                            &Status);
        if (!Result) break;

        /* Query how much space should be allocated for the key information */
        Status = ZwQueryKey(KeyHandle,
                            KeyFullInformation,
                            NULL,
                            sizeof(ULONG),
                            &NumberOfBytes);
        if (Status != STATUS_BUFFER_TOO_SMALL) break;

        /* Allocate the space required */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        FullInfo = ExAllocatePoolWithTag(PagedPool, NumberOfBytes, PCI_POOL_TAG);
        if ( !FullInfo ) break;

        /* Now query the key information that's needed */
        Status = ZwQueryKey(KeyHandle,
                            KeyFullInformation,
                            FullInfo,
                            NumberOfBytes,
                            &NumberOfBytes);
        if (!NT_SUCCESS(Status)) break;

        /* Allocate enough space to hold the value information plus the name */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        Length = FullInfo->MaxNameLen + 26;
        KeyInfo = ExAllocatePoolWithTag(PagedPool, Length, PCI_POOL_TAG);
        if (!KeyInfo) break;

        /* Allocate the value information and name we expect to find */
        ValueInfo = ExAllocatePoolWithTag(PagedPool,
                                          sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                                          sizeof(L"PCI BIOS"),
                                          PCI_POOL_TAG);
        if (!ValueInfo) break;

        /* Loop each sub-key */
        i = 0;
        while (TRUE)
        {
            /* Query each sub-key */
            Status = ZwEnumerateKey(KeyHandle,
                                    i++,
                                    KeyBasicInformation,
                                    KeyInfo,
                                    Length,
                                    &NumberOfBytes);
            if (Status == STATUS_NO_MORE_ENTRIES) break;

            /* Null-terminate the keyname, because the kernel does not */
            KeyInfo->Name[KeyInfo->NameLength / sizeof(WCHAR)] = UNICODE_NULL;

            /* Open this subkey */
            Result = PciOpenKey(KeyInfo->Name,
                                KeyHandle,
                                KEY_QUERY_VALUE,
                                &SubKey,
                                &Status);
            if (Result)
            {
                /* Query the identifier value for this subkey */
                RtlInitUnicodeString(&ValueName, L"Identifier");
                Status = ZwQueryValueKey(SubKey,
                                         &ValueName,
                                         KeyValuePartialInformation,
                                         ValueInfo,
                                         sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                                         sizeof(L"PCI BIOS"),
                                         &NumberOfBytes);
                if (NT_SUCCESS(Status))
                {
                    /* Check if this is the PCI BIOS subkey */
                    if (!wcsncmp((PWCHAR)ValueInfo->Data,
                                 L"PCI BIOS",
                                 ValueInfo->DataLength))
                    {
                        /* It is, proceed to query the PCI IRQ routing table */
                        Status = PciGetRegistryValue(L"Configuration Data",
                                                     L"RealModeIrqRoutingTable"
                                                     L"\\0",
                                                     SubKey,
                                                     REG_FULL_RESOURCE_DESCRIPTOR,
                                                     (PVOID*)&Package,
                                                     &NumberOfBytes);
                        ZwClose(SubKey);
                        break;
                    }
                }

                /* Close the subkey and try the next one */
                ZwClose(SubKey);
            }
        }

        /* Check if we got here because the routing table was found */
        if (!NT_SUCCESS(Status)) break;

        /* Check if a descriptor was found */
        if (!Package) break;

        /* Make sure the buffer is large enough to hold the table */
        if ((NumberOfBytes < sizeof(*Package)) ||
            (Package->Table.TableSize >
             (NumberOfBytes - sizeof(CM_FULL_RESOURCE_DESCRIPTOR))))
        {
            /* Invalid package size */
            Status = STATUS_UNSUCCESSFUL;
            break;
        }

        /* Allocate space for the table */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        *PciRoutingTable = ExAllocatePoolWithTag(PagedPool,
                                                 NumberOfBytes,
                                                 PCI_POOL_TAG);
        if (!*PciRoutingTable) break;

        /* Copy the registry data */
        RtlCopyMemory(*PciRoutingTable,
                      &Package->Table,
                      NumberOfBytes - sizeof(CM_FULL_RESOURCE_DESCRIPTOR));
        Status = STATUS_SUCCESS;
    } while (FALSE);

    /* Close any opened keys, free temporary allocations, and return status */
    if (Package) ExFreePoolWithTag(Package, 0);
    if (ValueInfo) ExFreePoolWithTag(ValueInfo, 0);
    if (KeyInfo) ExFreePoolWithTag(KeyInfo, 0);
    if (FullInfo) ExFreePoolWithTag(FullInfo, 0);
    if (KeyHandle) ZwClose(KeyHandle);
    return Status;
}

NTSTATUS
NTAPI
PciBuildHackTable(IN HANDLE KeyHandle)
{
    PKEY_FULL_INFORMATION FullInfo;
    ULONG i, HackCount;
    PKEY_VALUE_FULL_INFORMATION ValueInfo;
    PPCI_HACK_ENTRY Entry;
    NTSTATUS Status;
    ULONG NameLength, ResultLength;
    ULONGLONG HackFlags;

    /* So we know what to free at the end of the body */
    FullInfo = NULL;
    ValueInfo = NULL;
    do
    {
        /* Query the size required for full key information */
        Status = ZwQueryKey(KeyHandle,
                            KeyFullInformation,
                            NULL,
                            0,
                            &ResultLength);
        if (Status != STATUS_BUFFER_TOO_SMALL) break;

        /* Allocate the space required to hold the full key information */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        ASSERT(ResultLength > 0);
        FullInfo = ExAllocatePoolWithTag(PagedPool, ResultLength, PCI_POOL_TAG);
        if (!FullInfo) break;

        /* Go ahead and query the key information */
        Status = ZwQueryKey(KeyHandle,
                            KeyFullInformation,
                            FullInfo,
                            ResultLength,
                            &ResultLength);
        if (!NT_SUCCESS(Status)) break;

        /* The only piece of information that's needed is the count of values */
        HackCount = FullInfo->Values;

        /* Free the structure now */
        ExFreePoolWithTag(FullInfo, 0);
        FullInfo = NULL;

        /* Allocate the hack table, now that the number of entries is known */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        ResultLength = sizeof(PCI_HACK_ENTRY) * HackCount;
        PciHackTable = ExAllocatePoolWithTag(NonPagedPool,
                                             ResultLength +
                                             sizeof(PCI_HACK_ENTRY),
                                             PCI_POOL_TAG);
        if (!PciHackTable) break;

        /* Allocate the space needed to hold the full value information */
        ValueInfo = ExAllocatePoolWithTag(NonPagedPool,
                                          sizeof(KEY_VALUE_FULL_INFORMATION) +
                                          PCI_HACK_ENTRY_FULL_SIZE,
                                          PCI_POOL_TAG);
        if (!PciHackTable) break;

        /* Loop each value in the registry */
        Entry = &PciHackTable[0];
        for (i = 0; i < HackCount; i++)
        {
            /* Get the entry for this value */
            Entry = &PciHackTable[i];

            /* Query the value in the key */
            Status = ZwEnumerateValueKey(KeyHandle,
                                         i,
                                         KeyValueFullInformation,
                                         ValueInfo,
                                         sizeof(KEY_VALUE_FULL_INFORMATION) +
                                         PCI_HACK_ENTRY_FULL_SIZE,
                                         &ResultLength);
            if (!NT_SUCCESS(Status))
            {
                /* Check why the call failed */
                if ((Status != STATUS_BUFFER_OVERFLOW) &&
                    (Status != STATUS_BUFFER_TOO_SMALL))
                {
                    /* The call failed due to an unknown error, bail out */
                    break;
                }

                /* The data seems to mismatch, try the next key in the list */
                continue;
            }

            /* Check if the value data matches what's expected */
            if ((ValueInfo->Type != REG_BINARY) ||
                (ValueInfo->DataLength != sizeof(ULONGLONG)))
            {
                /* It doesn't, try the next key in the list */
                continue;
            }

            /* Read the actual hack flags */
            HackFlags = *(PULONGLONG)((ULONG_PTR)ValueInfo +
                                      ValueInfo->DataOffset);

            /* Check what kind of errata entry this is, based on the name */
            NameLength = ValueInfo->NameLength;
            if ((NameLength != PCI_HACK_ENTRY_SIZE) &&
                (NameLength != PCI_HACK_ENTRY_REV_SIZE) &&
                (NameLength != PCI_HACK_ENTRY_SUBSYS_SIZE) &&
                (NameLength != PCI_HACK_ENTRY_FULL_SIZE))
            {
                /* It's an invalid entry, skip it */
                DPRINT1("Skipping hack entry with invalid length name\n");
                continue;
            }

            /* Initialize the entry */
            RtlZeroMemory(Entry, sizeof(PCI_HACK_ENTRY));

            /* Get the vendor and device data */
            if (!(PciStringToUSHORT(ValueInfo->Name, &Entry->VendorID)) ||
                !(PciStringToUSHORT(&ValueInfo->Name[4], &Entry->DeviceID)))
            {
                /* This failed, try the next entry */
                continue;
            }

            /* Check if the entry contains subsystem information */
            if ((NameLength == PCI_HACK_ENTRY_SUBSYS_SIZE) ||
                (NameLength == PCI_HACK_ENTRY_FULL_SIZE))
            {
                /* Get the data */
                if (!(PciStringToUSHORT(&ValueInfo->Name[8],
                                        &Entry->SubVendorID)) ||
                    !(PciStringToUSHORT(&ValueInfo->Name[12],
                                        &Entry->SubSystemID)))
                  {
                      /* This failed, try the next entry */
                      continue;
                  }

                  /* Save the fact this entry has finer controls */
                  Entry->Flags |= PCI_HACK_HAS_SUBSYSTEM_INFO;
             }

             /* Check if the entry contains revision information */
             if ((NameLength == PCI_HACK_ENTRY_REV_SIZE) ||
                 (NameLength == PCI_HACK_ENTRY_FULL_SIZE))
             {
                 /* Get the data */
                 if (!PciStringToUSHORT(&ValueInfo->Name[16],
                                        &Entry->RevisionID))
                 {
                     /* This failed, try the next entry */
                     continue;
                 }

                 /* Save the fact this entry has finer controls */
                 Entry->Flags |= PCI_HACK_HAS_REVISION_INFO;
             }

            /* Only the last entry should have this set */
            ASSERT(Entry->VendorID != PCI_INVALID_VENDORID);

            /* Save the actual hack flags */
            Entry->HackFlags = HackFlags;

            /* Print out for the debugger's sake */
#ifdef HACK_DEBUG
            DPRINT1("Adding Hack entry for Vendor:0x%04x Device:0x%04x ",
                    Entry->VendorID, Entry->DeviceID);
            if (Entry->Flags & PCI_HACK_HAS_SUBSYSTEM_INFO)
                DbgPrint("SybSys:0x%04x SubVendor:0x%04x ",
                         Entry->SubSystemID, Entry->SubVendorID);
            if (Entry->Flags & PCI_HACK_HAS_REVISION_INFO)
                DbgPrint("Revision:0x%02x", Entry->RevisionID);
            DbgPrint(" = 0x%I64x\n", Entry->HackFlags);
#endif
        }

        /* Bail out in case of failure */
        if (!NT_SUCCESS(Status)) break;

        /* Terminate the table with an invalid entry */
        ASSERT(Entry < (PciHackTable + HackCount + 1));
        Entry->VendorID = PCI_INVALID_VENDORID;

        /* Success path, free the temporary registry data */
        ExFreePoolWithTag(ValueInfo, 0);
        return STATUS_SUCCESS;
    } while (TRUE);

    /* Failure path, free temporary allocations and return failure code */
    ASSERT(!NT_SUCCESS(Status));
    if (FullInfo) ExFreePool(FullInfo);
    if (ValueInfo) ExFreePool(ValueInfo);
    if (PciHackTable) ExFreePool(PciHackTable);
    return Status;
}

NTSTATUS
NTAPI
PciGetDebugPorts(IN HANDLE DebugKey)
{
    UNREFERENCED_PARAMETER(DebugKey);
    /* This function is not yet implemented */
    UNIMPLEMENTED_DBGBREAK();
    return STATUS_SUCCESS;
}

DRIVER_UNLOAD PciDriverUnload;

VOID
NTAPI
PciDriverUnload(IN PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    /* This function is not yet implemented */
    UNIMPLEMENTED_DBGBREAK("PCI: Unload\n");
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    HANDLE KeyHandle, ParametersKey, DebugKey, ControlSetKey;
    BOOLEAN Result;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG ResultLength;
    PULONG Value;
    PWCHAR StartOptions;
    UNICODE_STRING OptionString, PciLockString;
    NTSTATUS Status;
    DPRINT1("PCI: DriverEntry!\n");

    /* Setup initial loop variables */
    KeyHandle = NULL;
    ParametersKey = NULL;
    DebugKey = NULL;
    ControlSetKey = NULL;
    do
    {
        /* Remember our object so we can get it to it later */
        PciDriverObject = DriverObject;

        /* Setup the IRP dispatcher */
        DriverObject->MajorFunction[IRP_MJ_POWER] = PciDispatchIrp;
        DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PciDispatchIrp;
        DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = PciDispatchIrp;
        DriverObject->MajorFunction[IRP_MJ_PNP] = PciDispatchIrp;
        DriverObject->DriverUnload = PciDriverUnload;

        /* This is how we'll detect a new PCI bus */
        DriverObject->DriverExtension->AddDevice = PciAddDevice;

        /* Open the PCI key */
        InitializeObjectAttributes(&ObjectAttributes,
                                   RegistryPath,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   NULL,
                                   NULL);
        Status = ZwOpenKey(&KeyHandle, KEY_QUERY_VALUE, &ObjectAttributes);
        if (!NT_SUCCESS(Status)) break;

        /* Open the Parameters subkey */
        Result = PciOpenKey(L"Parameters",
                            KeyHandle,
                            KEY_QUERY_VALUE,
                            &ParametersKey,
                            &Status);
        //if (!Result) break;

        /* Build the list of all known PCI erratas */
        Status = PciBuildHackTable(ParametersKey);
        //if (!NT_SUCCESS(Status)) break;

        /* Open the debug key, if it exists */
        Result = PciOpenKey(L"Debug",
                            KeyHandle,
                            KEY_QUERY_VALUE,
                            &DebugKey,
                            &Status);
        if (Result)
        {
            /* There are PCI debug devices, go discover them */
            Status = PciGetDebugPorts(DebugKey);
            if (!NT_SUCCESS(Status)) break;
        }

        /* Initialize the synchronization locks */
        KeInitializeEvent(&PciGlobalLock, SynchronizationEvent, TRUE);
        KeInitializeEvent(&PciBusLock, SynchronizationEvent, TRUE);
        KeInitializeEvent(&PciLegacyDescriptionLock, SynchronizationEvent, TRUE);

        /* Open the control set key */
        Result = PciOpenKey(L"\\Registry\\Machine\\System\\CurrentControlSet",
                            NULL,
                            KEY_QUERY_VALUE,
                            &ControlSetKey,
                            &Status);
        if (!Result) break;

        /* Read the command line */
        Status = PciGetRegistryValue(L"SystemStartOptions",
                                     L"Control",
                                     ControlSetKey,
                                     REG_SZ,
                                     (PVOID*)&StartOptions,
                                     &ResultLength);
        if (NT_SUCCESS(Status))
        {
            /* Initialize the command-line as a string */
            OptionString.Buffer = StartOptions;
            OptionString.MaximumLength = OptionString.Length = ResultLength;

            /* Check if the command-line has the PCILOCK argument */
            RtlInitUnicodeString(&PciLockString, L"PCILOCK");
            if (PciUnicodeStringStrStr(&OptionString, &PciLockString, TRUE))
            {
                /* The PCI Bus driver will keep the BIOS-assigned resources */
                PciLockDeviceResources = TRUE;
            }

            /* This data isn't needed anymore */
            ExFreePoolWithTag(StartOptions, 0);
        }

        /* The PCILOCK feature can also be enabled per-system in the registry */
        Status = PciGetRegistryValue(L"PCILock",
                                     L"Control\\BiosInfo\\PCI",
                                     ControlSetKey,
                                     REG_DWORD,
                                     (PVOID*)&Value,
                                     &ResultLength);
        if (NT_SUCCESS(Status))
        {
            /* Read the value it's been set to. This overrides /PCILOCK */
            if (ResultLength == sizeof(ULONG)) PciLockDeviceResources = *Value;
            ExFreePoolWithTag(Value, 0);
        }

        /* The system can have global PCI erratas in the registry */
        Status = PciGetRegistryValue(L"HackFlags",
                                     L"Control\\PnP\\PCI",
                                     ControlSetKey,
                                     REG_DWORD,
                                     (PVOID*)&Value,
                                     &ResultLength);
        if (NT_SUCCESS(Status))
        {
            /* Read them in */
            if (ResultLength == sizeof(ULONG)) PciSystemWideHackFlags = *Value;
            ExFreePoolWithTag(Value, 0);
        }

        /* Check if the system should allow native ATA support */
        Status = PciGetRegistryValue(L"EnableNativeModeATA",
                                     L"Control\\PnP\\PCI",
                                     ControlSetKey,
                                     REG_DWORD,
                                     (PVOID*)&Value,
                                     &ResultLength);
        if (NT_SUCCESS(Status))
        {
            /* This key is typically set by drivers, but users can force it */
            if (ResultLength == sizeof(ULONG)) PciEnableNativeModeATA = *Value;
            ExFreePoolWithTag(Value, 0);
        }

        /* Build the range lists for all the excluded resource areas */
        Status = PciBuildDefaultExclusionLists();
        if (!NT_SUCCESS(Status)) break;

        /* Read the PCI IRQ Routing Table that the loader put in the registry */
        PciGetIrqRoutingTableFromRegistry(&PciIrqRoutingTable);

        /* Take over the HAL's default PCI Bus Handler routines */
        PciHookHal();

        /* Initialize verification of PCI BIOS and devices, if requested */
        PciVerifierInit(DriverObject);

        /* Check if this is a Datacenter SKU, which impacts IRQ alignment */
        PciRunningDatacenter = PciIsDatacenter();
        if (PciRunningDatacenter) DPRINT1("PCI running on datacenter build\n");

        /* Check if the system has an ACPI Hardware Watchdog Timer */
        //WdTable = PciGetAcpiTable(WDRT_SIGNATURE);
        Status = STATUS_SUCCESS;
    } while (FALSE);

    /* Close all opened keys, return driver status to PnP Manager */
    if (KeyHandle) ZwClose(KeyHandle);
    if (ControlSetKey) ZwClose(ControlSetKey);
    if (ParametersKey) ZwClose(ParametersKey);
    if (DebugKey) ZwClose(DebugKey);
    return Status;
}

/* EOF */
