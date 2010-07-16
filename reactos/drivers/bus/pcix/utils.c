/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/utils.c
 * PURPOSE:         Utility/Helper Support Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

RTL_RANGE_LIST PciIsaBitExclusionList;
RTL_RANGE_LIST PciVgaAndIsaBitExclusionList;

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI
PciUnicodeStringStrStr(IN PUNICODE_STRING InputString,
                       IN PCUNICODE_STRING EqualString,
                       IN BOOLEAN CaseInSensitive)
{
    UNICODE_STRING PartialString;
    LONG EqualChars, TotalChars;

    /* Build a partial string with the smaller substring */
    PartialString.Length = EqualString->Length;
    PartialString.MaximumLength = InputString->MaximumLength;;
    PartialString.Buffer = InputString->Buffer;

    /* Check how many characters that need comparing */
    EqualChars = 0;
    TotalChars = (InputString->Length - EqualString->Length) / sizeof(WCHAR);

    /* If the substring is bigger, just fail immediately */
    if (TotalChars < 0) return FALSE;

    /* Keep checking each character */
    while (!RtlEqualUnicodeString(EqualString, &PartialString, CaseInSensitive))
    {
        /* Continue checking until all the required characters are equal */
        PartialString.Buffer++;
        PartialString.MaximumLength -= sizeof(WCHAR);
        if (++EqualChars > TotalChars) return FALSE;
    }

    /* The string is equal */
    return TRUE;
}

BOOLEAN
NTAPI
PciStringToUSHORT(IN PWCHAR String,
                  OUT PUSHORT Value)
{
    USHORT Short;
    ULONG Low, High, Length;
    WCHAR Char;

    /* Initialize everything to zero */
    Short = 0;
    Length = 0;
    while (TRUE)
    {
        /* Get the character and set the high byte based on the previous one */
        Char = *String++;
        High = 16 * Short;

        /* Check for numbers */
        if ( Char >= '0' && Char <= '9' )
        {
            /* Convert them to a byte */
            Low = Char - '0';
        }
        else if ( Char >= 'A' && Char <= 'F' )
        {
            /* Convert upper-case hex letters into a byte */
            Low = Char - '7';
        }
        else if ( Char >= 'a' && Char <= 'f' )
        {
            /* Convert lower-case hex letters into a byte */
            Low = Char - 'W';
        }
        else
        {
            /* Invalid string, fail the conversion */
            return FALSE;
        }

        /* Combine the high and low byte */
        Short = High | Low;

        /* If 4 letters have been reached, the 16-bit integer should exist */
        if (++Length >= 4)
        {
            /* Return it to the caller */
            *Value = Short;
            return TRUE;
        }
    }
}

BOOLEAN
NTAPI
PciIsSuiteVersion(IN USHORT SuiteMask)
{
    ULONGLONG Mask = 0;
    RTL_OSVERSIONINFOEXW VersionInfo;

    /* Initialize the version information */
    RtlZeroMemory(&VersionInfo, sizeof(RTL_OSVERSIONINFOEXW));
    VersionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
    VersionInfo.wSuiteMask = SuiteMask;

    /* Set the comparison mask and return if the passed suite mask matches */
    VER_SET_CONDITION(Mask, VER_SUITENAME, VER_AND);
    return NT_SUCCESS(RtlVerifyVersionInfo(&VersionInfo, VER_SUITENAME, Mask));
}

BOOLEAN
NTAPI
PciIsDatacenter(VOID)
{
    BOOLEAN Result;
    PVOID Value;
    ULONG ResultLength;
    NTSTATUS Status;

    /* Assume this isn't Datacenter */
    Result = FALSE;

    /* First, try opening the setup key */
    Status = PciGetRegistryValue(L"",
                                 L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\setupdd",
                                 0,
                                 REG_BINARY,
                                 &Value,
                                 &ResultLength);
    if (!NT_SUCCESS(Status))
    {
        /* This is not an in-progress Setup boot, so query the suite version */
        Result = PciIsSuiteVersion(VER_SUITE_DATACENTER);
    }
    else
    {
        /* This scenario shouldn't happen yet, since SetupDD isn't used */
        UNIMPLEMENTED;
        while (TRUE);
    }

    /* Return if this is Datacenter or not */
    return Result;
}

BOOLEAN
NTAPI
PciOpenKey(IN PWCHAR KeyName,
           IN HANDLE RootKey,
           IN ACCESS_MASK DesiredAccess,
           OUT PHANDLE KeyHandle,
           OUT PNTSTATUS KeyStatus)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyString;
    PAGED_CODE();

    /* Initialize the object attributes */
    RtlInitUnicodeString(&KeyString, KeyName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyString,
                               OBJ_CASE_INSENSITIVE,
                               RootKey,
                               NULL);

    /* Open the key, returning a boolean, and the status, if requested */
    Status = ZwOpenKey(KeyHandle, DesiredAccess, &ObjectAttributes);
    if (KeyStatus) *KeyStatus = Status;
    return NT_SUCCESS(Status);
}

NTSTATUS
NTAPI
PciGetRegistryValue(IN PWCHAR ValueName,
                    IN PWCHAR KeyName,
                    IN HANDLE RootHandle,
                    IN ULONG Type,
                    OUT PVOID *OutputBuffer,
                    OUT PULONG OutputLength)
{
    NTSTATUS Status;
    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo;
    ULONG NeededLength, ActualLength;
    UNICODE_STRING ValueString;
    HANDLE KeyHandle;
    BOOLEAN Result;

    /* So we know what to free at the end of the body */
    PartialInfo = NULL;
    KeyHandle = NULL;
    do
    {
        /* Open the key by name, rooted off the handle passed */
        Result = PciOpenKey(KeyName,
                            RootHandle,
                            KEY_QUERY_VALUE,
                            &KeyHandle,
                            &Status);
        if (!Result) break;

        /* Query for the size that's needed for the value that was passed in */
        RtlInitUnicodeString(&ValueString, ValueName);
        Status = ZwQueryValueKey(KeyHandle,
                                 &ValueString,
                                 KeyValuePartialInformation,
                                 NULL,
                                 0,
                                 &NeededLength);
        ASSERT(!NT_SUCCESS(Status));
        if (Status != STATUS_BUFFER_TOO_SMALL) break;

        /* Allocate an appropriate buffer for the size that was returned */
        ASSERT(NeededLength != 0);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        PartialInfo = ExAllocatePoolWithTag(PagedPool,
                                            NeededLength,
                                            PCI_POOL_TAG);
        if (!PartialInfo) break;

        /* Query the actual value information now that the size is known */
        Status = ZwQueryValueKey(KeyHandle,
                                 &ValueString,
                                 KeyValuePartialInformation,
                                 PartialInfo,
                                 NeededLength,
                                 &ActualLength);
        if (!NT_SUCCESS(Status)) break;

        /* Make sure it's of the type that the caller expects */
        Status = STATUS_INVALID_PARAMETER;
        if (PartialInfo->Type != Type) break;

        /* Subtract the registry-specific header, to get the data size */
        ASSERT(NeededLength == ActualLength);
        NeededLength -= sizeof(KEY_VALUE_PARTIAL_INFORMATION);

        /* Allocate a buffer to hold the data and return it to the caller */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        *OutputBuffer = ExAllocatePoolWithTag(PagedPool,
                                              NeededLength,
                                              PCI_POOL_TAG);
        if (!*OutputBuffer) break;

        /* Copy the data into the buffer and return its length to the caller */
        RtlCopyMemory(*OutputBuffer, PartialInfo->Data, NeededLength);
        if (OutputLength) *OutputLength = NeededLength;
        Status = STATUS_SUCCESS;
    } while (0);

    /* Close any opened keys and free temporary allocations */
    if (KeyHandle) ZwClose(KeyHandle);
    if (PartialInfo) ExFreePoolWithTag(PartialInfo, 0);
    return Status;
}

NTSTATUS
NTAPI
PciBuildDefaultExclusionLists(VOID)
{
    ULONG Start;
    NTSTATUS Status;
    ASSERT(PciIsaBitExclusionList.Count == 0);
    ASSERT(PciVgaAndIsaBitExclusionList.Count == 0);

    /* Initialize the range lists */
    RtlInitializeRangeList(&PciIsaBitExclusionList);
    RtlInitializeRangeList(&PciVgaAndIsaBitExclusionList);

    /* Loop x86 I/O ranges */
    for (Start = 0x100; Start <= 0xFEFF; Start += 0x400)
    {
        /* Add the ISA I/O ranges */
        Status = RtlAddRange(&PciIsaBitExclusionList,
                             Start,
                             Start + 0x2FF,
                             0,
                             RTL_RANGE_LIST_ADD_IF_CONFLICT,
                             NULL,
                             NULL);
        if (!NT_SUCCESS(Status)) break;

        /* Add the ISA I/O ranges */
        Status = RtlAddRange(&PciVgaAndIsaBitExclusionList,
                             Start,
                             Start + 0x2AF,
                             0,
                             RTL_RANGE_LIST_ADD_IF_CONFLICT,
                             NULL,
                             NULL);
        if (!NT_SUCCESS(Status)) break;

        /* Add the VGA I/O range for Monochrome Video */
        Status = RtlAddRange(&PciVgaAndIsaBitExclusionList,
                             Start + 0x2BC,
                             Start + 0x2BF,
                             0,
                             RTL_RANGE_LIST_ADD_IF_CONFLICT,
                             NULL,
                             NULL);
        if (!NT_SUCCESS(Status)) break;

        /* Add the VGA I/O range for certain CGA adapters */
        Status = RtlAddRange(&PciVgaAndIsaBitExclusionList,
                             Start + 0x2E0,
                             Start + 0x2FF,
                             0,
                             RTL_RANGE_LIST_ADD_IF_CONFLICT,
                             NULL,
                             NULL);
        if (!NT_SUCCESS(Status)) break;

        /* Success, ranges added done */
    };

    RtlFreeRangeList(&PciIsaBitExclusionList);
    RtlFreeRangeList(&PciVgaAndIsaBitExclusionList);
    return Status;
}

PPCI_FDO_EXTENSION
NTAPI
PciFindParentPciFdoExtension(IN PDEVICE_OBJECT DeviceObject,
                             IN PKEVENT Lock)
{
    PPCI_FDO_EXTENSION FoundExtension;

    /* Assume we'll find nothing */
    FoundExtension = NULL;

    /* Check if a lock was specified */
    if (Lock)
    {
        /* Wait for the lock to be released */
        KeEnterCriticalRegion();
        KeWaitForSingleObject(Lock, Executive, KernelMode, FALSE, NULL);
    }

    /* Now search for the extension */
    if (PciFdoExtensionListHead.Next)
    {
        /* This case should not be hit yet */
        UNIMPLEMENTED;
        while (TRUE);
    }

    /* Check if we had acquired a lock previously */
    if (Lock)
    {
        /* Release it */
        KeSetEvent(Lock, IO_NO_INCREMENT, FALSE);
        KeLeaveCriticalRegion();
    }

    /* Return which extension was found, if any */
    return FoundExtension;
}

VOID
NTAPI
PciInsertEntryAtTail(IN PSINGLE_LIST_ENTRY ListHead,
                     IN PPCI_FDO_EXTENSION DeviceExtension,
                     IN PKEVENT Lock)
{
    PSINGLE_LIST_ENTRY NextEntry;
    PAGED_CODE();

    /* Check if a lock was specified */
    if (Lock)
    {
        /* Wait for the lock to be released */
        KeEnterCriticalRegion();
        KeWaitForSingleObject(Lock, Executive, KernelMode, FALSE, NULL);
    }

    /* Loop the list until we get to the end, then insert this entry there */
    for (NextEntry = ListHead; NextEntry->Next; NextEntry = NextEntry->Next);
    NextEntry->Next = &DeviceExtension->List;

    /* Check if we had acquired a lock previously */
    if (Lock)
    {
        /* Release it */
        KeSetEvent(Lock, IO_NO_INCREMENT, FALSE);
        KeLeaveCriticalRegion();
    }
}

VOID
NTAPI
PciInsertEntryAtHead(IN PSINGLE_LIST_ENTRY ListHead,
                     IN PSINGLE_LIST_ENTRY Entry,
                     IN PKEVENT Lock)
{
    PAGED_CODE();

    /* Check if a lock was specified */
    if (Lock)
    {
        /* Wait for the lock to be released */
        KeEnterCriticalRegion();
        KeWaitForSingleObject(Lock, Executive, KernelMode, FALSE, NULL);
    }

    /* Make the entry point to the current head and make the head point to it */
    Entry->Next = ListHead->Next;
    ListHead->Next = Entry;

    /* Check if we had acquired a lock previously */
    if (Lock)
    {
        /* Release it */
        KeSetEvent(Lock, IO_NO_INCREMENT, FALSE);
        KeLeaveCriticalRegion();
    }
}

VOID
NTAPI
PcipLinkSecondaryExtension(IN PSINGLE_LIST_ENTRY List,
                           IN PVOID Lock,
                           IN PPCI_SECONDARY_EXTENSION SecondaryExtension,
                           IN PCI_SIGNATURE ExtensionType,
                           IN PVOID Destructor)
{
    PAGED_CODE();

    /* Setup the extension data, and insert it into the primary's list */
    SecondaryExtension->ExtensionType = ExtensionType;
    SecondaryExtension->Destructor = Destructor;
    PciInsertEntryAtHead(List, &SecondaryExtension->List, Lock);
}

NTSTATUS
NTAPI
PciGetDeviceProperty(IN PDEVICE_OBJECT DeviceObject,
                     IN DEVICE_REGISTRY_PROPERTY DeviceProperty,
                     OUT PVOID *OutputBuffer)
{
    NTSTATUS Status;
    ULONG BufferLength, ResultLength;
    PVOID Buffer;
    do
    {
        /* Query the requested property size */
        Status = IoGetDeviceProperty(DeviceObject,
                                     DeviceProperty,
                                     0,
                                     NULL,
                                     &BufferLength);
        if (Status != STATUS_BUFFER_TOO_SMALL)
        {
            /* Call should've failed with buffer too small! */
            DPRINT1("PCI - Unexpected status from GetDeviceProperty, saw %08X, expected %08X.\n",
                    Status,
                    STATUS_BUFFER_TOO_SMALL);
            *OutputBuffer = NULL;
            ASSERTMSG(FALSE, "PCI Successfully did the impossible!");
            break;
        }

        /* Allocate the required buffer */
        Buffer = ExAllocatePoolWithTag(PagedPool, BufferLength, 'BicP');
        if (!Buffer)
        {
            /* No memory, fail the request */
            DPRINT1("PCI - Failed to allocate DeviceProperty buffer (%d bytes).\n", BufferLength);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        /* Do the actual property query call */
        Status = IoGetDeviceProperty(DeviceObject,
                                     DeviceProperty,
                                     BufferLength,
                                     Buffer,
                                     &ResultLength);
        if (!NT_SUCCESS(Status)) break;

        /* Return the buffer to the caller */
        ASSERT(BufferLength == ResultLength);
        *OutputBuffer = Buffer;
        return STATUS_SUCCESS;
    } while (FALSE);

    /* Failure path */
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
PciSendIoctl(IN PDEVICE_OBJECT DeviceObject,
             IN ULONG IoControlCode,
             IN PVOID InputBuffer,
             IN ULONG InputBufferLength,
             IN PVOID OutputBuffer,
             IN ULONG OutputBufferLength)
{
    PIRP Irp;
    NTSTATUS Status;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;
    PDEVICE_OBJECT AttachedDevice;
    PAGED_CODE();

    /* Initialize the pending IRP event */
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    /* Get a reference to the root PDO (ACPI) */
    AttachedDevice = IoGetAttachedDeviceReference(DeviceObject);
    if (!AttachedDevice) return STATUS_INVALID_PARAMETER;

    /* Build the requested IOCTL IRP */
    Irp = IoBuildDeviceIoControlRequest(IoControlCode,
                                        AttachedDevice,
                                        InputBuffer,
                                        InputBufferLength,
                                        OutputBuffer,
                                        OutputBufferLength,
                                        0,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    /* Send the IOCTL to the driver */
    Status = IoCallDriver(AttachedDevice, Irp);
    if (Status == STATUS_PENDING)
    {
        /* Wait for a response */
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = Irp->IoStatus.Status;
    }

    /* Take away the reference we took and return the result to the caller */
    ObDereferenceObject(AttachedDevice);
    return Status;
}

PPCI_SECONDARY_EXTENSION
NTAPI
PciFindNextSecondaryExtension(IN PSINGLE_LIST_ENTRY ListHead,
                              IN PCI_SIGNATURE ExtensionType)
{
    PSINGLE_LIST_ENTRY NextEntry;
    PPCI_SECONDARY_EXTENSION Extension;

    /* Scan the list */
    for (NextEntry = ListHead; NextEntry; NextEntry = NextEntry->Next)
    {
        /* Grab each extension and check if it's the one requested */
        Extension = CONTAINING_RECORD(NextEntry, PCI_SECONDARY_EXTENSION, List);
        if (Extension->ExtensionType == ExtensionType) return Extension;
    }

    /* Nothing was found */
    return NULL;
}

/* EOF */
