/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/pci/id.c
 * PURPOSE:         PCI Device Identification
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

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
        ASSERT(Description[TextLength / sizeof(WCHAR) == L'\n']);

        /* Allocate the buffer to hold the message string */
        Buffer = ExAllocatePoolWithTag(PagedPool, TextLength, 'BicP');
        if (!Buffer) return NULL;

        /* Copy the message, minus the newline character, and terminate it */
        RtlCopyMemory(Buffer, Entry->Text, TextLength - 1);
        Buffer[TextLength / sizeof(WCHAR)] = UNICODE_NULL;

        /* Return the length to the caller */
        if (Length) *Length = UnicodeString.Length;
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

/* EOF */
