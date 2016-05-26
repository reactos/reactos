/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/pci/id.c
 * PURPOSE:         PCI Device Identification
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>
#include <stdio.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

PWCHAR
NTAPI
PciGetDescriptionMessage(IN ULONG Identifier,
                         OUT PULONG Length)
{
    PMESSAGE_RESOURCE_ENTRY Entry;
    ULONG TextLength;
    PWCHAR Description, Buffer;
    ANSI_STRING MessageString;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;

    /* Find the message identifier in the message table */
    MessageString.Buffer = NULL;
    Status = RtlFindMessage(PciDriverObject->DriverStart,
                            11, // RT_MESSAGETABLE
                            LANG_NEUTRAL,
                            Identifier,
                            &Entry);
    if (!NT_SUCCESS(Status)) return NULL;

    /* Check if the resource data is Unicode or ANSI */
    if (Entry->Flags & MESSAGE_RESOURCE_UNICODE)
    {
        /* Subtract one space for the end-of-message terminator */
        TextLength = Entry->Length -
                     FIELD_OFFSET(MESSAGE_RESOURCE_ENTRY, Text) -
                     sizeof(WCHAR);

        /* Grab the text */
        Description = (PWCHAR)Entry->Text;

        /* Validate valid message length, ending with a newline character */
        ASSERT(TextLength > 1);
        ASSERT(Description[TextLength / sizeof(WCHAR)] == L'\n');

        /* Allocate the buffer to hold the message string */
        Buffer = ExAllocatePoolWithTag(PagedPool, TextLength, 'BicP');
        if (!Buffer) return NULL;

        /* Copy the message, minus the newline character, and terminate it */
        RtlCopyMemory(Buffer, Entry->Text, TextLength - 1);
        Buffer[TextLength / sizeof(WCHAR)] = UNICODE_NULL;

        /* Return the length to the caller, minus the terminating NULL */
        if (Length) *Length = TextLength - 1;
    }
    else
    {
        /* Initialize the entry as a string */
        RtlInitAnsiString(&MessageString, (PCHAR)Entry->Text);

        /* Remove the newline character */
        MessageString.Length -= sizeof(CHAR);

        /* Convert it to Unicode */
        RtlAnsiStringToUnicodeString(&UnicodeString, &MessageString, TRUE);
        Buffer = UnicodeString.Buffer;

        /* Return the length to the caller */
        if (Length) *Length = UnicodeString.Length;
    }

    /* Return the message buffer to the caller */
    return Buffer;
}

PWCHAR
NTAPI
PciGetDeviceDescriptionMessage(IN UCHAR BaseClass,
                               IN UCHAR SubClass)
{
    PWCHAR Message;
    ULONG Identifier;

    /* The message identifier in the table is encoded based on the PCI class */
    Identifier = (BaseClass << 8) | SubClass;

    /* Go grab the description message for this device */
    Message = PciGetDescriptionMessage(Identifier, NULL);
    if (!Message)
    {
        /* It wasn't found, allocate a buffer for a generic description */
        Message = ExAllocatePoolWithTag(PagedPool, sizeof(L"PCI Device"), 'bicP');
        if (Message) RtlCopyMemory(Message, L"PCI Device", sizeof(L"PCI Device"));
    }

    /* Return the description message */
    return Message;
}

VOID
NTAPI
PciInitIdBuffer(IN PPCI_ID_BUFFER IdBuffer)
{
    /* Initialize the sizes to zero and the pointer to the start of the buffer */
    IdBuffer->TotalLength = 0;
    IdBuffer->Count = 0;
    IdBuffer->CharBuffer = IdBuffer->BufferData;
}

ULONG
__cdecl
PciIdPrintf(IN PPCI_ID_BUFFER IdBuffer,
            IN PCCH Format,
            ...)
{
    ULONG Size, Length;
    PANSI_STRING AnsiString;
    va_list va;

    ASSERT(IdBuffer->Count < MAX_ANSI_STRINGS);
 
    /* Do the actual string formatting into the character buffer */
    va_start(va, Format);
    vsprintf(IdBuffer->CharBuffer, Format, va);
    va_end(va);

    /* Initialize the ANSI_STRING that will hold this string buffer */
    AnsiString = &IdBuffer->Strings[IdBuffer->Count];
    RtlInitAnsiString(AnsiString, IdBuffer->CharBuffer);
    
    /* Calculate the final size of the string, in Unicode */
    Size = RtlAnsiStringToUnicodeSize(AnsiString);
 
    /* Update hte buffer with the size,and update the character pointer */
    IdBuffer->StringSize[IdBuffer->Count] = Size;
    IdBuffer->TotalLength += Size;
    Length = AnsiString->Length + sizeof(ANSI_NULL);
    IdBuffer->CharBuffer += Length;
    
    /* Move to the next string for next time */
    IdBuffer->Count++;
    
    /* Return the length */
    return Length;
}

ULONG
__cdecl
PciIdPrintfAppend(IN PPCI_ID_BUFFER IdBuffer,
                  IN PCCH Format,
                  ...)
{
    ULONG NextId, Size, Length, MaxLength;
    PANSI_STRING AnsiString;
    va_list va;

    ASSERT(IdBuffer->Count);
  
    /* Choose the next static ANSI_STRING to use */
    NextId = IdBuffer->Count - 1;
  
    /* Max length is from the end of the buffer up until the current pointer */
    MaxLength = (PCHAR)(IdBuffer + 1) - IdBuffer->CharBuffer; 
    
    /* Do the actual append, and return the length this string took */
    va_start(va, Format);
    Length = vsprintf(IdBuffer->CharBuffer - 1, Format, va);
    va_end(va);
    ASSERT(Length < MaxLength);

    /* Select the static ANSI_STRING, and update its length information */
    AnsiString = &IdBuffer->Strings[NextId];
    AnsiString->Length += Length;
    AnsiString->MaximumLength += Length;
  
    /* Calculate the final size of the string, in Unicode */
    Size = RtlAnsiStringToUnicodeSize(AnsiString);
  
    /* Update the buffer with the size, and update the character pointer */
    IdBuffer->StringSize[NextId] = Size;
    IdBuffer->TotalLength += Size;
    IdBuffer->CharBuffer += Length;
    
    /* Return the size */
    return Size;
}

NTSTATUS
NTAPI
PciQueryId(IN PPCI_PDO_EXTENSION DeviceExtension,
           IN BUS_QUERY_ID_TYPE QueryType,
           OUT PWCHAR *Buffer)
{
    ULONG SubsysId;
    CHAR VendorString[22];
    PPCI_PDO_EXTENSION PdoExtension;
    PPCI_FDO_EXTENSION ParentExtension;
    PWCHAR StringBuffer;
    ULONG i, Size;
    NTSTATUS Status;
    PANSI_STRING NextString;
    UNICODE_STRING DestinationString;
    PCI_ID_BUFFER IdBuffer;
    PAGED_CODE();
  
    /* Assume failure */
    Status = STATUS_SUCCESS;
    *Buffer = NULL;
    
    /* Start with the genric vendor string, which is the vendor ID + device ID */
    sprintf(VendorString,
            "PCI\\VEN_%04X&DEV_%04X",
            DeviceExtension->VendorId,
            DeviceExtension->DeviceId);
    
    /* Initialize the PCI ID Buffer */
    PciInitIdBuffer(&IdBuffer);
    
    /* Build the subsystem ID as shown in PCI ID Strings */
    SubsysId = DeviceExtension->SubsystemVendorId | (DeviceExtension->SubsystemId << 16);
  
    /* Check what the caller is requesting */
    switch (QueryType)
    {
        case BusQueryDeviceID:
        
            /* A single ID, the vendor string + the revision ID */
            PciIdPrintf(&IdBuffer,
                        "%s&SUBSYS_%08X&REV_%02X",
                        VendorString,
                        SubsysId,
                        DeviceExtension->RevisionId);
            break;
        
        case BusQueryHardwareIDs:
        
            /* First the vendor string + the subsystem ID + the revision ID */
            PciIdPrintf(&IdBuffer,
                        "%s&SUBSYS_%08X&REV_%02X",
                        VendorString,
                        SubsysId,
                        DeviceExtension->RevisionId);
            
            /* Next, without the revision */
            PciIdPrintf(&IdBuffer,
                        "%s&SUBSYS_%08X",
                        VendorString,
                        SubsysId);
            
            /* Next, the vendor string + the base class + sub class + progif */
            PciIdPrintf(&IdBuffer,
                        "%s&CC_%02X%02X%02X",
                        VendorString,
                        DeviceExtension->BaseClass,
                        DeviceExtension->SubClass,
                        DeviceExtension->ProgIf);

            /* Next, without the progif */
            PciIdPrintf(&IdBuffer,
                        "%s&CC_%02X%02X",
                        VendorString,
                        DeviceExtension->BaseClass,
                        DeviceExtension->SubClass);
            
            /* And finally, a terminator */
            PciIdPrintf(&IdBuffer, "\0");
            break;
        
        case BusQueryCompatibleIDs:
        
            /* First, the vendor + revision ID only */
            PciIdPrintf(&IdBuffer,
                        "%s&REV_%02X",
                        VendorString,
                        DeviceExtension->RevisionId);

            /* Next, the vendor string alone */
            PciIdPrintf(&IdBuffer, "%s", VendorString);
            
            /* Next, the vendor ID + the base class + the sub class + progif */
            PciIdPrintf(&IdBuffer,
                         "PCI\\VEN_%04X&CC_%02X%02X%02X",
                         DeviceExtension->VendorId,
                         DeviceExtension->BaseClass,
                         DeviceExtension->SubClass,
                         DeviceExtension->ProgIf);

            /* Now without the progif */
            PciIdPrintf(&IdBuffer,
                        "PCI\\VEN_%04X&CC_%02X%02X",
                        DeviceExtension->VendorId,
                        DeviceExtension->BaseClass,
                        DeviceExtension->SubClass);

            /* And then just the vendor ID itself */
            PciIdPrintf(&IdBuffer,
                        "PCI\\VEN_%04X",
                        DeviceExtension->VendorId);

            /* Then the base class + subclass + progif, without any vendor */
            PciIdPrintf(&IdBuffer,
                        "PCI\\CC_%02X%02X%02X",
                        DeviceExtension->BaseClass,
                        DeviceExtension->SubClass,
                        DeviceExtension->ProgIf);

            /* Next, without the progif */
            PciIdPrintf(&IdBuffer,
                        "PCI\\CC_%02X%02X",
                        DeviceExtension->BaseClass,
                        DeviceExtension->SubClass);
                        
            /* And finally, a terminator */
            PciIdPrintf(&IdBuffer, "\0");
            break;
        
        case BusQueryInstanceID:
        
            /* Start with a terminator */
            PciIdPrintf(&IdBuffer, "\0");
        
            /* And then encode the device and function number */
            PciIdPrintfAppend(&IdBuffer,
                              "%02X",
                              (DeviceExtension->Slot.u.bits.DeviceNumber << 3) |
                              DeviceExtension->Slot.u.bits.FunctionNumber);
          
            /* Loop every parent until the root */
            ParentExtension = DeviceExtension->ParentFdoExtension;
            while (!PCI_IS_ROOT_FDO(ParentExtension))
            {
                /* And encode the parent's device and function number as well */
                PdoExtension = ParentExtension->PhysicalDeviceObject->DeviceExtension;
                PciIdPrintfAppend(&IdBuffer,
                                  "%02X",
                                  (PdoExtension->Slot.u.bits.DeviceNumber << 3) |
                                  PdoExtension->Slot.u.bits.FunctionNumber);
            }
            break;
        
        default:
        
            /* Unknown query type */
            DPRINT1("PciQueryId expected ID type = %d\n", QueryType);
            return STATUS_NOT_SUPPORTED;
    }
    
    /* Something should've been generated if this has been reached */
    ASSERT(IdBuffer.Count > 0);
    
    /* Allocate the final string buffer to hold the ID */
    StringBuffer = ExAllocatePoolWithTag(PagedPool, IdBuffer.TotalLength, 'BicP');
    if (!StringBuffer) return STATUS_INSUFFICIENT_RESOURCES;
    
    /* Build the UNICODE_STRING structure for it */
    DPRINT1("PciQueryId(%d)\n", QueryType);
    DestinationString.Buffer = StringBuffer;
    DestinationString.MaximumLength = IdBuffer.TotalLength;
    
    /* Loop every ID in the buffer */
    for (i = 0; i < IdBuffer.Count; i++)
    {
        /* Select the ANSI_STRING for the ID */
        NextString = &IdBuffer.Strings[i];
        DPRINT1("    <- \"%s\"\n", NextString->Buffer);
        
        /* Convert it to a UNICODE_STRING */
        Status = RtlAnsiStringToUnicodeString(&DestinationString, NextString, FALSE);
        ASSERT(NT_SUCCESS(Status));
        
        /* Add it into the final destination buffer */
        Size = IdBuffer.StringSize[i];
        DestinationString.MaximumLength -= Size;
        DestinationString.Buffer += (Size / sizeof(WCHAR));
    }

    /* Return the buffer to the caller and return status (should be success) */
    *Buffer = StringBuffer;
    return Status;
}

NTSTATUS
NTAPI
PciQueryDeviceText(IN PPCI_PDO_EXTENSION PdoExtension,
                   IN DEVICE_TEXT_TYPE QueryType,
                   IN ULONG Locale,
                   OUT PWCHAR *Buffer)
{
    PWCHAR MessageBuffer, LocationBuffer;
    ULONG Length;
    NTSTATUS Status;
    
    UNREFERENCED_PARAMETER(Locale);

    /* Check what the caller is requesting */
    switch (QueryType)
    {
        case DeviceTextDescription:
        
            /* Get the message from the resource section */
            MessageBuffer = PciGetDeviceDescriptionMessage(PdoExtension->BaseClass,
                                                           PdoExtension->SubClass);
                                                           
            /* Return it to the caller, and select proper status code */
            *Buffer = MessageBuffer;
            Status = MessageBuffer ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;
            break;
        
        case DeviceTextLocationInformation:
        
            /* Get the message from the resource section */
            MessageBuffer = PciGetDescriptionMessage(0x10000, &Length);
            if (!MessageBuffer)
            {
                /* It should be there, but fail if it wasn't found for some reason */
                Status = STATUS_NOT_SUPPORTED;
                break;
            }
            
            /* Add space for a null-terminator, and allocate the buffer */
            Length += 2 * sizeof(UNICODE_NULL);
            LocationBuffer = ExAllocatePoolWithTag(PagedPool,
                                                   Length * sizeof(WCHAR),
                                                   'BicP');
            *Buffer = LocationBuffer;
            
            /* Check if the allocation succeeded */
            if (LocationBuffer)
            {
                /* Build the location string based on bus, function, and device */
                swprintf(LocationBuffer,
                         MessageBuffer,
                         PdoExtension->ParentFdoExtension->BaseBus,
                         PdoExtension->Slot.u.bits.FunctionNumber,
                         PdoExtension->Slot.u.bits.DeviceNumber);
            }
            
            /* Free the original string from the resource section */
            ExFreePoolWithTag(MessageBuffer, 0);
            
            /* Select the correct status */
            Status = LocationBuffer ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
            break;
            
        default:
            
            /* Anything else is unsupported */
            Status = STATUS_NOT_SUPPORTED;
            break;
    }
    
    /* Return whether or not a device text string was indeed found */
    return Status;
}

/* EOF */
