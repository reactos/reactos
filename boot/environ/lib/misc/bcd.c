/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/misc/bcd.c
 * PURPOSE:         Boot Library BCD Routines
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"
#include <bcd.h>

/* FUNCTIONS *****************************************************************/

VOID
BiNotifyEnumerationError (
    _In_ HANDLE ObjectHandle,
    _In_ PWCHAR ElementName,
    _In_ NTSTATUS Status
    )
{
    /* Stub for now */
    UNREFERENCED_PARAMETER(ObjectHandle);
    UNREFERENCED_PARAMETER(ElementName);
    UNREFERENCED_PARAMETER(Status);
    EfiPrintf(L"Error in BiNotify: %lx for element %s\r\n", Status, ElementName);
}

ULONG
BiConvertElementFormatToValueType (
    _In_ ULONG Format
    )
{
    /* Strings and objects are strings */
    if ((Format == BCD_TYPE_STRING) || (Format == BCD_TYPE_OBJECT))
    {
        return REG_SZ;
    }

    /* Object lists are arrays of strings */
    if (Format == BCD_TYPE_OBJECT_LIST)
    {
        return REG_MULTI_SZ;
    }

    /* Everything else is binary */
    return REG_BINARY;
}

NTSTATUS
BiConvertRegistryDataToElement (
    _In_ HANDLE ObjectHandle,
    _In_ PVOID Data,
    _In_ ULONG DataLength,
    _In_ BcdElementType ElementType,
    _Out_ PVOID Element,
    _Out_ PULONG ElementSize
    )
{
    NTSTATUS Status;
    SIZE_T Length, Size, ReturnedLength;
    PBL_DEVICE_DESCRIPTOR Device;
    BOOLEAN NullTerminate;
    PBCD_DEVICE_OPTION BcdDevice, ElementDevice;
    PWCHAR BcdString, ElementString;
    PGUID ElementGuid; UNICODE_STRING GuidString;
    PULONGLONG ElementInteger;
    PUSHORT ElementWord; PBOOLEAN BcdBoolean;

    /* Assume failure */
    ReturnedLength = 0;

    /* Check what type of format we are dealing with */
    switch (ElementType.Format)
    {
        /* Devices -- they are in a binary format */
        case BCD_TYPE_DEVICE:

            /* First, make sure it's at least big enough for an empty descriptor */
            if (DataLength < FIELD_OFFSET(BCD_DEVICE_OPTION,
                                          DeviceDescriptor.Unknown))
            {
                return STATUS_OBJECT_TYPE_MISMATCH;
            }

            /* Both the registry and BCD format are the same */
            BcdDevice = (PBCD_DEVICE_OPTION)Data;
            ElementDevice = (PBCD_DEVICE_OPTION)Element;

            /* Make sure the device fits in the registry data */
            Device = &BcdDevice->DeviceDescriptor;
            Size = Device->Size;
            if ((Size + sizeof(BcdDevice->AssociatedEntry)) != DataLength)
            {
                return STATUS_OBJECT_TYPE_MISMATCH;
            }

            /* Check if this is a locate device */
            if (Device->DeviceType == LocateDevice)
            {
                EfiPrintf(L"Locates not yet supported\r\n");
                return STATUS_NOT_SUPPORTED;
            }

            /* Make sure the caller's buffer can fit the device */
            ReturnedLength = Size + sizeof(BcdDevice->AssociatedEntry);
            if (ReturnedLength > *ElementSize)
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            /* It'll fit -- copy it in */
            RtlCopyMemory(&ElementDevice->DeviceDescriptor, Device, Size);
            ElementDevice->AssociatedEntry = BcdDevice->AssociatedEntry;
            Status = STATUS_SUCCESS;
            break;

        /* Strings -- they are stored as is */
        case BCD_TYPE_STRING:

            /* Make sure the string isn't empty or misaligned */
            if (!(DataLength) || (DataLength & 1))
            {
                return STATUS_OBJECT_TYPE_MISMATCH;
            }

            /* Both the registry and BCD format are the same */
            BcdString = (PWCHAR)Data;
            ElementString = (PWCHAR)Element;

            /* We'll need as much data as the string has to offer */
            ReturnedLength = DataLength;

            /* If the string isn't NULL-terminated, do it now */
            NullTerminate = FALSE;
            if (BcdString[(DataLength / sizeof(WCHAR)) - 1] != UNICODE_NULL)
            {
                ReturnedLength += sizeof(UNICODE_NULL);
                NullTerminate = TRUE;
            }

            /* Will we fit in the caller's buffer? */
            if (ReturnedLength > *ElementSize)
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            /* Yep -- copy it in, and NULL-terminate if needed */
            RtlCopyMemory(Element, Data, DataLength);
            if (NullTerminate)
            {
                ElementString[DataLength / sizeof(WCHAR)] = UNICODE_NULL;
            }

            Status = STATUS_SUCCESS;
            break;

        /* Objects -- they are stored as GUID Strings */
        case BCD_TYPE_OBJECT:

            /* Registry data is a string, BCD data is a GUID */
            BcdString = (PWCHAR)Data;
            ElementGuid = (PGUID)Element;

            /* We need a GUID-sized buffer, does the caller have one? */
            ReturnedLength = sizeof(*ElementGuid);
            if (*ElementSize < ReturnedLength)
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            /* Yep, copy the GUID */
            RtlInitUnicodeString(&GuidString, BcdString);
            Status = RtlGUIDFromString(&GuidString, ElementGuid);
            break;

        /* Object Lists -- they are stored as arrays of GUID strings */
        case BCD_TYPE_OBJECT_LIST:

            /* Assume an empty list*/
            ReturnedLength = 0;
            Length = 0;
            Status = STATUS_SUCCESS;

            /* Registry data is an array of strings, BCD data is array of GUIDs */
            BcdString = (PWCHAR)Data;
            ElementGuid = (PGUID)Element;

            /* Loop as long as the array still has strings */
            while (*BcdString)
            {
                /* Don't read beyond the registry data */
                if (Length >= DataLength)
                {
                    break;
                }

                /* One more GUID -- does the caller have space? */
                ReturnedLength += sizeof(GUID);
                if (ReturnedLength <= *ElementSize)
                {
                    /* Convert and add it in */
                    RtlInitUnicodeString(&GuidString, BcdString);
                    Status = RtlGUIDFromString(&GuidString, ElementGuid);
                    if (!NT_SUCCESS(Status))
                    {
                        break;
                    }

                    /* Move to the next GUID in the caller's buffer */
                    ElementGuid++;
                }

                /* Move to the next string in the registry array */
                Size = (wcslen(BcdString) * sizeof(WCHAR)) + sizeof(UNICODE_NULL);
                Length += Size;
                BcdString = (PWCHAR)((ULONG_PTR)BcdString + Length);
            }

            /* Check if we failed anywhere */
            if (!NT_SUCCESS(Status))
            {
                break;
            }

            /* Check if we consumed more space than we have */
            if (ReturnedLength > *ElementSize)
            {
                Status = STATUS_BUFFER_TOO_SMALL;
            }

            /* All good here */
            break;

        /* Integer -- stored as binary */
        case BCD_TYPE_INTEGER:

            /* BCD data is a ULONGLONG, registry data is 8 bytes binary */
            ElementInteger = (PULONGLONG)Element;
            ReturnedLength = sizeof(*ElementInteger);

            /* Make sure the registry data makes sense */
            if (DataLength > ReturnedLength)
            {
                return STATUS_OBJECT_TYPE_MISMATCH;
            }

            /* Make sure the caller has space */
            if (*ElementSize < ReturnedLength)
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            /* Write the integer result */
            *ElementInteger = 0;
            RtlCopyMemory(ElementInteger, Data, DataLength);
            Status = STATUS_SUCCESS;
            break;

        /* Boolean -- stored as binary */
        case BCD_TYPE_BOOLEAN:

            /* BCD data is a BOOLEAN, registry data is 2 bytes binary */
            ElementWord = (PUSHORT)Element;
            BcdBoolean = (PBOOLEAN)Data;
            ReturnedLength = sizeof(ElementWord);

            /* Make sure the registry data makes sense */
            if (DataLength != sizeof(*BcdBoolean))
            {
                return STATUS_OBJECT_TYPE_MISMATCH;
            }

            /* Make sure the caller has space */
            if (*ElementSize < ReturnedLength)
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            /* Write the boolean result */
            *ElementWord = 0;
            *ElementWord = *BcdBoolean != 0;
            Status = STATUS_SUCCESS;
            break;

        /* Integer list --stored as binary */
        case BCD_TYPE_INTEGER_LIST:

            /* BCD Data is n ULONGLONGs, registry data is n*8 bytes binary */
            ReturnedLength = DataLength;
            if (!(DataLength) || (DataLength & 7))
            {
                return STATUS_OBJECT_TYPE_MISMATCH;
            }

            /* Make sure the caller has space */
            if (*ElementSize < ReturnedLength)
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            /* Write the integer list result */
            RtlCopyMemory(Element, Data, DataLength);
            Status = STATUS_SUCCESS;
            break;

        /* Arbitrary data */
        default:

            /* Registry data is copied binary as-is */
            ReturnedLength = DataLength;

            /* Make sure it's not empty */
            if (!DataLength)
            {
                return STATUS_OBJECT_TYPE_MISMATCH;
            }

            /* Make sure the caller has space */
            if (*ElementSize < ReturnedLength)
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            /* Write the result */
            RtlCopyMemory(Element, Data, DataLength);
            Status = STATUS_SUCCESS;
            break;
    }

    /* If we got here due to success or space issues, write the size */
    if ((NT_SUCCESS(Status)) || (Status == STATUS_BUFFER_TOO_SMALL))
    {
        *ElementSize = ReturnedLength;
    }

    /* All done, return our conversion result */
    return Status;
}

NTSTATUS
BiConvertBcdElements (
    _In_ PBCD_PACKED_ELEMENT Elements,
    _Out_opt_ PBCD_ELEMENT Buffer,
    _Inout_ PULONG BufferSize,
    _Inout_ PULONG ElementCount
    )
{
    NTSTATUS Status;
    ULONG ElementSize, AlignedElementSize, AlignedDataSize;
    PBCD_ELEMENT_HEADER Header;
    PVOID Data;
    BOOLEAN Exists;
    ULONG i, j, Count;

    /* Local variable to keep track of objects */
    Count = 0;

    /* Safely compute the element bytes needed */
    Status = RtlULongMult(*ElementCount, sizeof(BCD_ELEMENT), &ElementSize);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Safely align the element size */
    Status = RtlULongAdd(ElementSize,
                         sizeof(ULONG) - 1,
                         &AlignedElementSize);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    AlignedElementSize = ALIGN_DOWN(AlignedElementSize, ULONG);

    /* Do a safe version of Add2Ptr to figure out where the headers will start */
    Status = RtlULongPtrAdd((ULONG_PTR)Buffer,
                            AlignedElementSize,
                            (PULONG_PTR)&Header);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Safely compute the header bytes needed */
    Status = RtlULongMult(*ElementCount,
                          sizeof(BCD_ELEMENT_HEADER),
                          &ElementSize);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Safely align the header size */
    Status = RtlULongAdd(ElementSize,
                         AlignedElementSize + sizeof(ULONG) - 1,
                         &AlignedElementSize);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    AlignedElementSize = ALIGN_DOWN(AlignedElementSize, ULONG);

    /* Do a safe version of Add2Ptr */
    Status = RtlULongPtrAdd((ULONG_PTR)Buffer,
                            AlignedElementSize,
                            (PULONG_PTR)&Data);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Iterate over every element */
    for (i = 0; i < *ElementCount; i++)
    {
        /* Safely align the element size */
        Status = RtlULongAdd(Elements->Size,
                             sizeof(ULONG) - 1,
                             &AlignedDataSize);
        if (!NT_SUCCESS(Status))
        {
            break;
        }
        AlignedDataSize = ALIGN_DOWN(AlignedDataSize, ULONG);

        /* Safely add the size of this data element */
        Status = RtlULongAdd(AlignedElementSize,
                             AlignedDataSize,
                             &AlignedElementSize);
        if (!NT_SUCCESS(Status))
        {
            break;
        }

        /* Do we have enough space left? */
        if (*BufferSize >= AlignedElementSize)
        {
            /* Check if our root is an inherited object */
            Exists = FALSE;
            if (Elements->RootType.PackedValue == BcdLibraryObjectList_InheritedObjects)
            {
                /* Yes, scan for us in the current buffer */
                for (j = 0; j < Count; j++)
                {
                    /* Do we already exist? */
                    while (Buffer[j].Header->Type == Elements->RootType.PackedValue)
                    {
                        /* Yep */
                        Exists = TRUE;
                        break;
                    }
                }
            }

            /* Have we already found ourselves? */
            if (!Exists)
            {
                /* Nope, one more entry */
                ++Count;

                /* Write out the unpacked object */
                Buffer->Body = Data;
                Buffer->Header = Header;

                /* Fill out its header */
                Header->Size = Elements->Size;
                Header->Type = Elements->Type;
                Header->Version = Elements->Version;

                /* And copy the data */
                RtlCopyMemory(Data, Elements->Data, Header->Size);

                /* Move to the next unpacked object and header */
                ++Buffer;
                ++Header;

                /* Move to the next data entry */
                Data = (PVOID)((ULONG_PTR)Data + AlignedDataSize);
            }
        }
        else
        {
            /* Nope, set failure code, but keep going so we can return count */
            Status = STATUS_BUFFER_TOO_SMALL;
        }

        /* Move to the next element entry */
        Elements = Elements->NextEntry;
    }

    /* Return the new final buffer size and count */
    *BufferSize = AlignedElementSize;
    *ElementCount = Count;
    return Status;
}

NTSTATUS
BcdOpenObject (
    _In_ HANDLE BcdHandle,
    _In_ PGUID ObjectId,
    _Out_ PHANDLE ObjectHandle
    )
{
    NTSTATUS Status;
    GUID LocalGuid;
    UNICODE_STRING GuidString;
    HANDLE RootObjectHandle;

    /* Assume failure */
    *ObjectHandle = NULL;

    /* Initialize GUID string */
    GuidString.Buffer = NULL;

    /* Open the root "Objects" handle */
    RootObjectHandle = NULL;
    Status = BiOpenKey(BcdHandle, L"Objects", &RootObjectHandle);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Capture the object ID and convert it into a string */
    LocalGuid = *ObjectId;
    Status = RtlStringFromGUID(&LocalGuid, &GuidString);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Now open the key containing this object ID */
    Status = BiOpenKey(RootObjectHandle, GuidString.Buffer, ObjectHandle);

Quickie:
    /* Free the GUID string if we had one allocated */
    if (GuidString.Buffer)
    {
        RtlFreeUnicodeString(&GuidString);
    }

    /* Close the root handle if it was open */
    if (RootObjectHandle)
    {
        BiCloseKey(RootObjectHandle);
    }

    /* Return the final status */
    return Status;
}

NTSTATUS
BcdDeleteElement (
    _In_ HANDLE ObjectHandle,
    _In_ ULONG Type
    )
{
    NTSTATUS Status;
    HANDLE ElementsHandle, ElementHandle;
    WCHAR TypeString[22];

    /* Open the elements key */
    Status = BiOpenKey(ObjectHandle, L"Elements", &ElementsHandle);
    if (NT_SUCCESS(Status))
    {
        /* Convert the element ID into a string */
        if (!_ultow(Type, TypeString, 16))
        {
            /* Failed to do so */
            Status = STATUS_UNSUCCESSFUL;
        }
        else
        {
            /* Open the element specifically */
            Status = BiOpenKey(ElementsHandle, TypeString, &ElementHandle);
            if (NT_SUCCESS(Status))
            {
                /* Delete it */
                Status = BiDeleteKey(ElementHandle);
                if (NT_SUCCESS(Status))
                {
                    /* No point in closing the handle anymore */
                    ElementHandle = NULL;
                }
            }
            else
            {
                /* The element doesn't exist */
                Status = STATUS_NOT_FOUND;
            }

            /* Check if we should close the key */
            if (ElementHandle)
            {
                /* Do it */
                BiCloseKey(ElementHandle);
            }
        }
    }

    /* Check if we should close the elements handle */
    if (ElementsHandle)
    {
        /* Do it */
        BiCloseKey(ElementsHandle);
    }

    /* Return whatever the result was */
    return Status;
}

NTSTATUS
BiEnumerateSubElements (
    _In_ HANDLE BcdHandle,
    _In_ PVOID Object,
    _In_ ULONG ElementType,
    _In_ ULONG Flags,
    _Out_opt_ PBCD_PACKED_ELEMENT* Elements,
    _Inout_ PULONG ElementSize,
    _Out_ PULONG ElementCount
    )
{
    NTSTATUS Status;
    PBCD_PACKED_ELEMENT Element;
    HANDLE ObjectHandle;
    ULONG ParsedElements, RequiredSize;

    /* Assume empty */
    *ElementCount = 0;
    RequiredSize = 0;
    ParsedElements = 0;

    /* Open the object */
    Status = BcdOpenObject(BcdHandle, Object, &ObjectHandle);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Read the first entry, and the size available */
    Element = *Elements;
    RequiredSize = *ElementSize;

    /* Enumerate the object into the element array */
    Status = BiEnumerateElements(BcdHandle,
                                 ObjectHandle,
                                 ElementType,
                                 Flags,
                                 Element,
                                 &RequiredSize,
                                 &ParsedElements);

    /* Close the handle and bail out if we couldn't enumerate */
    BiCloseKey(ObjectHandle);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Check if the and subelements were present  */
    if (ParsedElements)
    {
        /* Keep going until the last one */
        while (Element->NextEntry)
        {
            Element = Element->NextEntry;
        }

        /* Set the new buffer location to the last element */
        *Elements = Element;
    }

Quickie:
    /* Return the number of sub-elements and their size */
    *ElementCount = ParsedElements;
    *ElementSize = RequiredSize;
    return Status;
}

NTSTATUS
BiEnumerateSubObjectElements (
    _In_ HANDLE BcdHandle,
    _Out_ PGUID SubObjectList,
    _In_ ULONG SubObjectCount,
    _In_ ULONG Flags,
    _Out_opt_ PBCD_PACKED_ELEMENT Elements,
    _Inout_ PULONG ElementSize,
    _Out_ PULONG ElementCount
    )
{
    NTSTATUS Status;
    ULONG SubElementCount, TotalSize, RequiredSize, CurrentSize, i;
    PBCD_PACKED_ELEMENT PreviousElement;

    /* Assume empty list */
    *ElementCount = 0;
    Status = STATUS_SUCCESS;

    /* Initialize variables */
    TotalSize = 0;
    PreviousElement = NULL;

    /* Set the currently remaining size based on caller's input */
    CurrentSize = *ElementSize;

    /* Iterate over every subje object */
    for (i = 0; i < SubObjectCount; i++)
    {
        /* Set the currently remaining buffer space */
        RequiredSize = CurrentSize;

        /* Enumerate the inherited sub elements */
        Status = BiEnumerateSubElements(BcdHandle,
                                        &SubObjectList[i],
                                        BcdLibraryObjectList_InheritedObjects,
                                        Flags,
                                        &Elements,
                                        &RequiredSize,
                                        &SubElementCount);
        if ((NT_SUCCESS(Status)) || (Status == STATUS_BUFFER_TOO_SMALL))
        {
            /* Safely add the length of the sub elements */
            Status = RtlULongAdd(TotalSize, RequiredSize, &TotalSize);
            if (!NT_SUCCESS(Status))
            {
                break;
            }

            /* Add the sub elements to the total */
            *ElementCount += SubElementCount;

            /* See if we have enough space*/
            if (*ElementSize >= TotalSize)
            {
                /* Were there any subelements? */
                if (SubElementCount)
                {
                    /* Update to keep track of these new subelements */
                    CurrentSize = *ElementSize - TotalSize;

                    /* Link the subelements into the chain */
                    PreviousElement = Elements;
                    PreviousElement->NextEntry =
                        (PBCD_PACKED_ELEMENT)((ULONG_PTR)Elements + TotalSize);
                    Elements = PreviousElement->NextEntry;
                }
            }
            else
            {
                /* We're out of space */
                CurrentSize = 0;
            }
        }
        else if ((Status != STATUS_NOT_FOUND) &&
                 (Status != STATUS_OBJECT_NAME_NOT_FOUND))
        {
            /* Some other fatal error, break out */
            break;
        }
        else
        {
            /* The sub element was not found, print a warning but keep going */
            BlStatusPrint(L"Ignoring missing BCD inherit object: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
                          (&SubObjectList[i])->Data1,
                          (&SubObjectList[i])->Data2,
                          (&SubObjectList[i])->Data3,
                          (&SubObjectList[i])->Data4[0],
                          (&SubObjectList[i])->Data4[1],
                          (&SubObjectList[i])->Data4[2],
                          (&SubObjectList[i])->Data4[3],
                          (&SubObjectList[i])->Data4[4],
                          (&SubObjectList[i])->Data4[5],
                          (&SubObjectList[i])->Data4[6],
                          (&SubObjectList[i])->Data4[7]);
            Status = STATUS_SUCCESS;
        }
    }

    /* Terminate the last element, if one was left */
    if (PreviousElement)
    {
        PreviousElement->NextEntry = NULL;
    }

    /* Set failure code if we ran out of space */
    if (*ElementSize < TotalSize)
    {
        Status = STATUS_BUFFER_TOO_SMALL;
    }

    /* Return final length and status */
    *ElementSize = TotalSize;
    return Status;
}

NTSTATUS
BiEnumerateElements (
    _In_ HANDLE BcdHandle,
    _In_ HANDLE ObjectHandle,
    _In_ ULONG RootElementType,
    _In_ ULONG Flags,
    _Out_opt_ PBCD_PACKED_ELEMENT Elements,
    _Inout_ PULONG ElementSize,
    _Out_ PULONG ElementCount
    )
{
    HANDLE ElementsHandle, ElementHandle;
    ULONG TotalLength, RegistryElementDataLength, RemainingLength;
    NTSTATUS Status;
    ULONG i;
    PVOID ElementData, SubObjectList, RegistryElementData;
    BcdElementType ElementType;
    PBCD_PACKED_ELEMENT PreviousElement, ElementsStart;
    ULONG SubElementCount, SubKeyCount, SubObjectCount, ElementDataLength;
    PWCHAR ElementName;
    PWCHAR* SubKeys;

    /* Assume failure */
    *ElementCount = 0;

    /* Initialize all locals that are checked at the end*/
    SubKeys = NULL;
    ElementsHandle = NULL;
    ElementHandle = NULL;
    ElementData = NULL;
    RegistryElementData = NULL;
    PreviousElement = NULL;
    ElementName = NULL;
    SubObjectList = NULL;
    TotalLength = 0;
    ElementDataLength = 0;
    SubObjectCount = 0;
    RemainingLength = 0;
    ElementsStart = Elements;

    /* Open the root object key's elements */
    Status = BiOpenKey(ObjectHandle, L"Elements", &ElementsHandle);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Enumerate all elements */
    Status = BiEnumerateSubKeys(ElementsHandle, &SubKeys, &SubKeyCount);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Iterate over each one */
    for (i = 0; i < SubKeyCount; i++)
    {
        /* Open the element */
        ElementName = SubKeys[i];
        Status = BiOpenKey(ElementsHandle, ElementName, &ElementHandle);
        if (!NT_SUCCESS(Status))
        {
            EfiPrintf(L"ELEMENT ERROR: %lx\r\n", Status);
            EfiStall(100000);
            break;
        }

        /* The name of the element is its data type */
        ElementType.PackedValue = wcstoul(SubKeys[i], NULL, 16);
        if (!(ElementType.PackedValue) || (ElementType.PackedValue == -1))
        {
            EfiPrintf(L"Value invalid\r\n");
            BiCloseKey(ElementHandle);
            ElementHandle = NULL;
            continue;
        }

        /* Read the appropriate registry value type for this element */
        Status = BiGetRegistryValue(ElementHandle,
                                    L"Element",
                                    BiConvertElementFormatToValueType(
                                    ElementType.Format),
                                    &RegistryElementData,
                                    &RegistryElementDataLength);
        if (!NT_SUCCESS(Status))
        {
            EfiPrintf(L"Element invalid\r\n");
            break;
        }

        /* Now figure out how much space the converted element will need */
        ElementDataLength = 0;
        Status = BiConvertRegistryDataToElement(ObjectHandle,
                                                RegistryElementData,
                                                RegistryElementDataLength,
                                                ElementType,
                                                NULL,
                                                &ElementDataLength);
        if (Status != STATUS_BUFFER_TOO_SMALL)
        {
            break;
        }

        /* Allocate a buffer big enough for the converted element */
        ElementData = BlMmAllocateHeap(ElementDataLength);
        if (!ElementData)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        /* And actually convert it this time around */
        Status = BiConvertRegistryDataToElement(ObjectHandle,
                                                RegistryElementData,
                                                RegistryElementDataLength,
                                                ElementType,
                                                ElementData,
                                                &ElementDataLength);
        if (!NT_SUCCESS(Status))
        {
            break;
        }

        /* Safely add space for the packed element header */
        Status = RtlULongAdd(TotalLength,
                             FIELD_OFFSET(BCD_PACKED_ELEMENT, Data),
                             &TotalLength);
        if (!NT_SUCCESS(Status))
        {
            break;
        }

        /* Safely add space for the data of the element itself */
        Status = RtlULongAdd(TotalLength, ElementDataLength, &TotalLength);
        if (!NT_SUCCESS(Status))
        {
            break;
        }

        /* One more element */
        ++*ElementCount;

        /* See how much space we were given */
        RemainingLength = *ElementSize;
        if (RemainingLength >= TotalLength)
        {
            /* Set the next pointer */
            Elements->NextEntry = (PBCD_PACKED_ELEMENT)((ULONG_PTR)ElementsStart + TotalLength);

            /* Fill this one out */
            Elements->RootType.PackedValue = RootElementType;
            Elements->Version = 1;
            Elements->Type = ElementType.PackedValue;
            Elements->Size = ElementDataLength;

            /* Add the data */
            RtlCopyMemory(Elements->Data, ElementData, ElementDataLength);
            RemainingLength -= TotalLength;

            /* Move to the next element on the next pass */
            PreviousElement = Elements;
            Elements = Elements->NextEntry;
        }
        else
        {
            /* We're out of space */
            RemainingLength = 0;
        }

        /* Are we enumerating devices, and is this a device? */
        if ((Flags & BCD_ENUMERATE_FLAG_DEVICES) &&
            (ElementType.Format == BCD_TYPE_DEVICE))
        {
            /* Yep, so go inside to enumerate it */
            Status = BiEnumerateSubElements(BcdHandle,
                                            ElementData,
                                            ElementType.PackedValue,
                                            Flags,
                                            &Elements,
                                            &ElementDataLength,
                                            &SubElementCount);
            if ((NT_SUCCESS(Status)) || (Status == STATUS_BUFFER_TOO_SMALL))
            {
                /* Safely add the length of the sub elements */
                Status = RtlULongAdd(TotalLength,
                                     ElementDataLength,
                                     &TotalLength);
                if (!NT_SUCCESS(Status))
                {
                    break;
                }

                /* Add the sub elements to the total */
                *ElementCount += SubElementCount;

                /* See if we have enough space*/
                if (*ElementSize >= TotalLength)
                {
                    /* Were there any subelements? */
                    if (SubElementCount)
                    {
                        /* Update to keep track of these new subelements */
                        ElementDataLength = *ElementSize - TotalLength;

                        /* Link the subelements into the chain */
                        PreviousElement = Elements;
                        PreviousElement->NextEntry =
                            (PBCD_PACKED_ELEMENT)((ULONG_PTR)ElementsStart +
                                                  TotalLength);
                        Elements = PreviousElement->NextEntry;
                    }
                }
                else
                {
                    /* We're out of space */
                    ElementDataLength = 0;
                }
            }
            else if ((Status != STATUS_NOT_FOUND) &&
                     (Status != STATUS_OBJECT_NAME_NOT_FOUND))
            {
                /* Fatal error trying to read the data, so fail */
                break;
            }
        }
        else if ((Flags & BCD_ENUMERATE_FLAG_DEEP) &&
                 (ElementType.PackedValue == BcdLibraryObjectList_InheritedObjects))
        {
            /* Inherited objects are requested, so allocate a buffer for them */
            SubObjectList = BlMmAllocateHeap(ElementDataLength);
            if (!SubObjectList)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            /* Copy the elements into the list. They are arrays of GUIDs */
            RtlCopyMemory(SubObjectList, ElementData, ElementDataLength);
            SubObjectCount = ElementDataLength / sizeof(GUID);
        }

        /* Free our local buffers */
        BlMmFreeHeap(ElementData);
        BlMmFreeHeap(RegistryElementData);
        ElementData = NULL;
        RegistryElementData = NULL;

        /* Close the key */
        BiCloseKey(ElementHandle);
        ElementHandle = NULL;
        ElementName = NULL;
    }

    /* Did we end up here with a sub object list after successful loop parsing? */
    if ((i != 0) && (i == SubKeyCount) && (SubObjectList))
    {
        /* We will actually enumerate it now, at the end */
        Status = BiEnumerateSubObjectElements(BcdHandle,
                                              SubObjectList,
                                              SubObjectCount,
                                              Flags,
                                              Elements,
                                              &RemainingLength,
                                              &SubElementCount);
        if ((NT_SUCCESS(Status)) || (Status == STATUS_BUFFER_TOO_SMALL))
        {
            /* Safely add the length of the sub elements */
            Status = RtlULongAdd(TotalLength, RemainingLength, &TotalLength);
            if ((NT_SUCCESS(Status)) && (SubElementCount))
            {
                /* Add the sub elements to the total */
                *ElementCount += SubElementCount;

                /* Don't touch PreviousElement anymore */
                PreviousElement = NULL;
            }
        }
    }

Quickie:
    /* Free the sub object list, if any */
    if (SubObjectList)
    {
        BlMmFreeHeap(SubObjectList);
    }

    /* Free any local element data */
    if (ElementData)
    {
        BlMmFreeHeap(ElementData);
    }

    /* Free any local registry data */
    if (RegistryElementData)
    {
        BlMmFreeHeap(RegistryElementData);
    }

    /* Close the handle if still opened */
    if (ElementHandle)
    {
        BiCloseKey(ElementHandle);
    }

    /* Terminate the last element, if any */
    if (PreviousElement)
    {
        PreviousElement->NextEntry = NULL;
    }

    /* Close the root handle if still opened */
    if (ElementsHandle)
    {
        BiCloseKey(ElementsHandle);
    }

    /* Set  failure code if out of space */
    if (*ElementSize < TotalLength)
    {
        Status = STATUS_BUFFER_TOO_SMALL;
    }

    /* Other errors will send a notification error */
    if (!(NT_SUCCESS(Status)) && (Status != STATUS_BUFFER_TOO_SMALL))
    {
        BiNotifyEnumerationError(ObjectHandle, ElementName, Status);
    }

    /* Finally free the subkeys array */
    if (SubKeys)
    {
        BlMmFreeHeap(SubKeys);
    }

    /* And return the required, final length and status */
    *ElementSize = TotalLength;
    return Status;
}

NTSTATUS
BiAddStoreFromFile (
    _In_ PBL_FILE_PATH_DESCRIPTOR FilePath,
    _Out_ PHANDLE StoreHandle
    )
{
    NTSTATUS Status;
    HANDLE HiveHandle, KeyHandle;

    /* Load the specified hive */
    Status = BiLoadHive(FilePath, &HiveHandle);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Open the description key to make sure this is really a BCD */
    Status = BiOpenKey(HiveHandle, L"Description", &KeyHandle);
    if (NT_SUCCESS(Status))
    {
        /* It is -- close the key as we don't need it */
        BiCloseKey(KeyHandle);
        *StoreHandle = HiveHandle;
    }
    else
    {
        /* Failure, drop a reference on the hive and close the key */
        BiDereferenceHive(HiveHandle);
        BiCloseKey(HiveHandle);
    }

    /* Return the status */
    return Status;
}

NTSTATUS
BiGetObjectDescription (
    _In_ HANDLE ObjectHandle,
    _Out_ PBCD_OBJECT_DESCRIPTION Description
    )
{
    NTSTATUS Status;
    HANDLE DescriptionHandle;
    PULONG Data;
    ULONG Length;

    /* Initialize locals */
    Data = NULL;
    DescriptionHandle = NULL;

    /* Open the description key */
    Status = BiOpenKey(ObjectHandle, L"Description", &DescriptionHandle);
    if (NT_SUCCESS(Status))
    {
        /* It exists */
        Description->Valid = TRUE;

        /* Read the type */
        Length = 0;
        Status = BiGetRegistryValue(DescriptionHandle,
                                    L"Type",
                                    REG_DWORD,
                                    (PVOID*)&Data,
                                    &Length);
        if (NT_SUCCESS(Status))
        {
            /* Make sure it's the length we expected it to be */
            if (Length == sizeof(Data))
            {
                /* Return the type that is stored there */
                Description->Type = *Data;
            }
            else
            {
                /* Invalid type value */
                Status = STATUS_OBJECT_TYPE_MISMATCH;
            }
        }
    }

    /* Did we have a handle open? */
    if (DescriptionHandle)
    {
        /* Close it */
        BiCloseKey(DescriptionHandle);
    }

    /* Did we have data allocated? */
    if (Data)
    {
        /* Free it */
        BlMmFreeHeap(Data);
    }

    /* Return back to caller */
    return Status;
}

NTSTATUS
BcdEnumerateAndUnpackElements (
    _In_ HANDLE BcdHandle,
    _In_ HANDLE ObjectHandle,
    _Out_opt_ PBCD_ELEMENT Elements,
    _Inout_ PULONG ElementSize,
    _Out_ PULONG ElementCount
    )
{
    PVOID LocalElements;
    NTSTATUS Status;
    ULONG LocalElementCount, LocalElementSize;

    /* Make sure required parameters are there */
    if (!(ElementSize) || !(ElementCount) || ((Elements) && (!*ElementSize)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Set initial count to zero */
    *ElementCount = 0;

    /* Do the initial enumeration to figure out the size required */
    LocalElementSize = 0;
    LocalElementCount = 0;
    Status = BiEnumerateElements(BcdHandle,
                                 ObjectHandle,
                                 0,
                                 BCD_ENUMERATE_FLAG_IN_ORDER |
                                 BCD_ENUMERATE_FLAG_DEVICES |
                                 BCD_ENUMERATE_FLAG_DEEP,
                                 NULL,
                                 &LocalElementSize,
                                 &LocalElementCount);
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        return Status;
    }

    /* Now allocate a buffer large enough to hold them */
    LocalElements = BlMmAllocateHeap(LocalElementSize);
    if (!LocalElements)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Zero out the array and do the real enumeration this time around */
    RtlZeroMemory(LocalElements, LocalElementSize);
    Status = BiEnumerateElements(BcdHandle,
                                 ObjectHandle,
                                 0,
                                 BCD_ENUMERATE_FLAG_IN_ORDER |
                                 BCD_ENUMERATE_FLAG_DEVICES |
                                 BCD_ENUMERATE_FLAG_DEEP,
                                 LocalElements,
                                 &LocalElementSize,
                                 &LocalElementCount);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Now we know the real count */
    *ElementCount = LocalElementCount;

    /* Now unpack the data */
    Status = BiConvertBcdElements(LocalElements,
                                  Elements,
                                  ElementSize,
                                  &LocalElementCount);
    if (NT_SUCCESS(Status))
    {
        /* Not all elements may have been converted */
        *ElementCount = LocalElementCount;
    }

    /* Free the local (unpacked) buffer and return status */
    BlMmFreeHeap(LocalElements);
    return Status;
}

NTSTATUS
BcdOpenStoreFromFile (
    _In_ PUNICODE_STRING FileName,
    _In_ PHANDLE BcdHandle
    )
{
    ULONG Length;
    PBL_FILE_PATH_DESCRIPTOR FilePath;
    NTSTATUS Status;
    HANDLE LocalHandle;

    /* Assume failure */
    LocalHandle = NULL;

    /* Allocate a path descriptor */
    Length = FileName->Length + sizeof(*FilePath);
    FilePath = BlMmAllocateHeap(Length);
    if (!FilePath)
    {
        return STATUS_NO_MEMORY;
    }

    /* Initialize it */
    FilePath->Version = 1;
    FilePath->PathType = InternalPath;
    FilePath->Length = Length;

    /* Copy the name and NULL-terminate it */
    RtlCopyMemory(FilePath->Path, FileName->Buffer, Length);
    FilePath->Path[Length / sizeof(WCHAR)] = UNICODE_NULL;

    /* Open the BCD */
    Status = BiAddStoreFromFile(FilePath, &LocalHandle);
    if (NT_SUCCESS(Status))
    {
        /* Return the handle on success */
        *BcdHandle = LocalHandle;
    }

    /* Free the descriptor and return the status */
    BlMmFreeHeap(FilePath);
    return Status;
}

