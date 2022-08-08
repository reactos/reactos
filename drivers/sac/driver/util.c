/*
 * PROJECT:     ReactOS Drivers
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/sac/driver/util.c
 * PURPOSE:     Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "sacdrv.h"

#include <ndk/rtlfuncs.h>

/* GLOBALS ********************************************************************/

PCHAR Utf8ConversionBuffer;
ULONG Utf8ConversionBufferSize = PAGE_SIZE;

PSAC_MACHINE_INFO MachineInformation;

PVOID RequestSacCmdEventObjectBody;
PKEVENT RequestSacCmdEventWaitObjectBody;
PVOID RequestSacCmdSuccessEventObjectBody;
PKEVENT RequestSacCmdSuccessEventWaitObjectBody;
PVOID RequestSacCmdFailureEventObjectBody;
PKEVENT RequestSacCmdFailureEventWaitObjectBody;
PFILE_OBJECT ServiceProcessFileObject;
BOOLEAN HaveUserModeServiceCmdEventInfo;

PSAC_MESSAGE_ENTRY GlobalMessageTable;
ULONG GlobalMessageTableCount;

LONG SerialPortConsumerIndex, SerialPortProducerIndex;
PCHAR SerialPortBuffer;

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI
SacTranslateUtf8ToUnicode(IN CHAR Utf8Char,
                          IN PCHAR Utf8Buffer,
                          OUT PWCHAR Utf8Value)
{
    ULONG i;

    /* Find out how many valid characters we have in the buffer */
    i = 0;
    while (Utf8Buffer[i++] && (i < 3));

    /* If we have at least 3, shift everything by a byte */
    if (i >= 3)
    {
        /* The last input character goes at the end */
        Utf8Buffer[0] = Utf8Buffer[1];
        Utf8Buffer[1] = Utf8Buffer[2];
        Utf8Buffer[2] = Utf8Char;
    }
    else
    {
        /* We don't have more than 3 characters, place the input at the index */
        Utf8Buffer[i] = Utf8Char;
    }

    /* Print to debugger */
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SacTranslateUtf8ToUnicode - About to decode the UTF8 buffer.\n");
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "                                  UTF8[0]: 0x%02lx UTF8[1]: 0x%02lx UTF8[2]: 0x%02lx\n",
            Utf8Buffer[0],
            Utf8Buffer[1],
            Utf8Buffer[2]);

    /* Is this a simple ANSI character? */
    if (!(Utf8Char & 0x80))
    {
        /* Return it as Unicode, nothing left to do */
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "SACDRV: SacTranslateUTf8ToUnicode - Case1\n");
        *Utf8Value = (WCHAR)Utf8Char;
        Utf8Buffer[0] = Utf8Buffer[1];
        Utf8Buffer[1] = Utf8Buffer[2];
        Utf8Buffer[2] = UNICODE_NULL;
        return TRUE;
    }

    /* Anything else is not yet supported */
    ASSERT(FALSE);
    return FALSE;
}

BOOLEAN
NTAPI
SacTranslateUnicodeToUtf8(IN PWCHAR SourceBuffer,
                          IN ULONG SourceBufferLength,
                          OUT PCHAR DestinationBuffer,
                          IN ULONG DestinationBufferSize,
                          OUT PULONG UTF8Count,
                          OUT PULONG ProcessedCount)
{
    *UTF8Count = 0;
    *ProcessedCount = 0;

    while ((*SourceBuffer) &&
           (*UTF8Count < DestinationBufferSize) &&
           (*ProcessedCount < SourceBufferLength))
    {
        if (*SourceBuffer & 0xFF80)
        {
            if (*SourceBuffer & 0xF800)
            {
                if ((*UTF8Count + 3) >= DestinationBufferSize) break;
                DestinationBuffer[*UTF8Count] = ((*SourceBuffer >> 12) & 0xF) | 0xE0;
                ++*UTF8Count;
                DestinationBuffer[*UTF8Count] = ((*SourceBuffer >> 6) & 0x3F) | 0x80;
            }
            else
            {
                if ((*UTF8Count + 2) >= DestinationBufferSize) break;
                DestinationBuffer[*UTF8Count] = ((*SourceBuffer >> 6) & 31) | 0xC0;
            }
            ++*UTF8Count;
            DestinationBuffer[*UTF8Count] = (*SourceBuffer & 0x3F) | 0x80;
        }
        else
        {
            DestinationBuffer[*UTF8Count] = (*SourceBuffer & 0x7F);
        }

        ++*UTF8Count;
        ++*ProcessedCount;
        ++SourceBuffer;
    }

    ASSERT(*ProcessedCount <= SourceBufferLength);
    ASSERT(*UTF8Count <= DestinationBufferSize);
    return TRUE;
}

PWCHAR
NTAPI
GetMessage(IN ULONG MessageIndex)
{
    PSAC_MESSAGE_ENTRY MessageEntry;
    ULONG i;
    PWCHAR MessageData = NULL;

    /* Loop all cached messages */
    for (i = 0; i < GlobalMessageTableCount; i++)
    {
        /* Check if this one matches the index */
        MessageEntry = &GlobalMessageTable[i];
        if (MessageEntry->Index == MessageIndex)
        {
            /* It does, return the buffer */
            MessageData = MessageEntry->Buffer;
            break;
        }
    }

    /* We should always find it */
    if (!MessageData) ASSERT(FALSE);
    return MessageData;
}

NTSTATUS
NTAPI
UTF8EncodeAndSend(IN PWCHAR String)
{
    ULONG ProcessedCount, Utf8Count, i;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Call the translator routine */
    if (SacTranslateUnicodeToUtf8(String,
                                  wcslen(String),
                                  Utf8ConversionBuffer,
                                  Utf8ConversionBufferSize,
                                  &Utf8Count,
                                  &ProcessedCount))
    {
        /* Loop every character */
        for (i = 0; i < Utf8Count; i++)
        {
            /* Send it to the terminal */
            Status = HeadlessDispatch(HeadlessCmdPutData,
                                      &Utf8ConversionBuffer[i],
                                      sizeof(Utf8ConversionBuffer[i]),
                                      NULL,
                                      NULL);
            if (!NT_SUCCESS(Status)) break;
        }
    }
    else
    {
        /* Conversion failed */
        Status = STATUS_UNSUCCESSFUL;
    }

    /* All done */
    return Status;
}

VOID
NTAPI
SacFormatMessage(IN PWCHAR FormattedString,
                 IN PWCHAR MessageString,
                 IN ULONG MessageSize)
{
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC SacFormatMessage: Entering.\n");

    /* Check if any of the parameters are NULL or zero */
    if (!(MessageString) || !(FormattedString) || !(MessageSize))
    {
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC SacFormatMessage: Exiting with invalid parameters.\n");
        return;
    }

    /* Keep going as long as there's still characters */
    while ((MessageString[0]) && (MessageSize))
    {
        /* Is it a non-formatting character? */
        if (MessageString[0] != L'%')
        {
            /* Just write it back into the buffer and keep going */
            *FormattedString++ = MessageString[0];
            MessageString++;
        }
        else
        {
            /* Go over the format characters we recognize */
            switch (MessageString[1])
            {
                case L'0':
                    *FormattedString = UNICODE_NULL;
                    return;

                case L'%':
                    *FormattedString++ = L'%';
                    break;

                case L'\\':
                    *FormattedString++ = L'\r';
                    *FormattedString++ = L'\n';
                    break;

                case L'r':
                    *FormattedString++ = L'\r';
                    break;

                case L'b':
                    *FormattedString++ = L' ';
                    break;

                case L'.':
                    *FormattedString++ = L'.';
                    break;

                case L'!':
                    *FormattedString++ = L'!';
                    break;

                default:
                    /* Only move forward one character */
                    MessageString--;
                    break;
            }

            /* Move forward two characters */
            MessageString += 2;
        }

        /* Move to the next character*/
        MessageSize--;
    }

    /* All done */
    *FormattedString = UNICODE_NULL;
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC SacFormatMessage: Exiting.\n");
}

NTSTATUS
NTAPI
PreloadGlobalMessageTable(IN PVOID ImageBase)
{
    NTSTATUS Status, Status2;
    ULONG MessageId, TotalLength, TextSize, i;
    PWCHAR StringBuffer;
    PMESSAGE_RESOURCE_ENTRY MessageEntry;
    PAGED_CODE();
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC PreloadGlobalMessageTable: Entering.\n");

    /* Nothing to do if we already have a table */
    Status = STATUS_SUCCESS;
    if (GlobalMessageTable) goto Exit;

    /* Loop through up to 200 messages */
    TotalLength = 0;
    for (MessageId = 1; MessageId != SAC_MAX_MESSAGES; MessageId++)
    {
        /* Find this message ID in the string table*/
        Status2 = RtlFindMessage(ImageBase,
                                 11,
                                 LANG_NEUTRAL,
                                 MessageId,
                                 &MessageEntry);
        if (NT_SUCCESS(Status2))
        {
            /* Make sure it's Unicode */
            ASSERT(MessageEntry->Flags & MESSAGE_RESOURCE_UNICODE);

            /* Remove the space taken up by the OS header, and add our own */
            TotalLength += MessageEntry->Length -
                           FIELD_OFFSET(MESSAGE_RESOURCE_ENTRY, Text) +
                           sizeof(SAC_MESSAGE_ENTRY);

            /* One more in the table */
            GlobalMessageTableCount++;
        }
    }

    /* We should've found at least one message... */
    if (!TotalLength)
    {
        /* Bail out otherwise */
        SAC_DBG(SAC_DBG_INIT, "SAC PreloadGlobalMessageTable: No Messages.\n");
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    /* Allocate space for the buffers and headers */
    GlobalMessageTable = SacAllocatePool(TotalLength, GLOBAL_BLOCK_TAG);
    if (!GlobalMessageTable)
    {
        /* Bail out if we couldn't allocate it */
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }

    /* All the buffers are going to be at the end of the table */
    StringBuffer = (PWCHAR)(&GlobalMessageTable[GlobalMessageTableCount]);

    /* Now loop over our entries again */
    for (i = 0, MessageId = 1; MessageId != SAC_MAX_MESSAGES; MessageId++)
    {
        /* Make sure the message is still there...! */
        Status2 = RtlFindMessage(ImageBase,
                                 11,
                                 LANG_NEUTRAL,
                                 MessageId,
                                 &MessageEntry);
        if (NT_SUCCESS(Status2))
        {
            /* Write the entry in the message table*/
            GlobalMessageTable[i].Index = MessageId;
            GlobalMessageTable[i].Buffer = StringBuffer;

            /* The structure includes the size of the header, elide it */
            TextSize = MessageEntry->Length -
                       FIELD_OFFSET(MESSAGE_RESOURCE_ENTRY, Text);

            /* Format the message into the entry. It should be same or smaller */
            SacFormatMessage(StringBuffer, (PWCHAR)MessageEntry->Text, TextSize);
            ASSERT((ULONG)(wcslen(StringBuffer)*sizeof(WCHAR)) <= TextSize);

            /* Move to the next buffer space */
            StringBuffer += (TextSize / sizeof(WCHAR));

            /* Move to the next entry, make sure the status is full success */
            i++;
            Status = STATUS_SUCCESS;
        }
    }

Exit:
    /* All done, return the status code */
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC TearDownGlobalMessageTable: Exiting with status 0x%0x\n", Status);
    return Status;
}

NTSTATUS
NTAPI
TearDownGlobalMessageTable(VOID)
{
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC TearDownGlobalMessageTable: Entering.\n");

    /* Free the table if one existed */
    if (GlobalMessageTable) SacFreePool(GlobalMessageTable);

    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC TearDownGlobalMessageTable: Exiting\n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
GetRegistryValueBuffer(IN PCWSTR KeyName,
                       IN PWCHAR ValueName,
                       IN PKEY_VALUE_PARTIAL_INFORMATION* Buffer)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DestinationString;
    HANDLE Handle;
    ULONG ResultLength = 0;
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC GetRegistryValueBuffer: Entering.\n");
    CHECK_PARAMETER1(KeyName);
    CHECK_PARAMETER2(ValueName);

    /* Open the specified key */
    RtlInitUnicodeString(&DestinationString, KeyName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &DestinationString,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwOpenKey(&Handle,
                       KEY_WRITE | SYNCHRONIZE | KEY_READ,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        /* Bail out on failure */
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC GetRegistryValueBuffer: failed ZwOpenKey: %X.\n", Status);
        return Status;
    }

    /* Query the size of the key */
    RtlInitUnicodeString(&DestinationString, ValueName);
    Status = ZwQueryValueKey(Handle,
                             &DestinationString,
                             KeyValuePartialInformation,
                             NULL,
                             0,
                             &ResultLength);
    if (!ResultLength)
        goto Quit;

    /* Allocate the buffer for the partial info structure and our integer data */
    ResultLength += sizeof(ULONG);
    *Buffer = SacAllocatePool(ResultLength, GLOBAL_BLOCK_TAG);
    if (!*Buffer)
    {
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC GetRegistryValueBuffer: failed allocation\n");
        goto Quit;
    }

    /* Now read the data */
    Status = ZwQueryValueKey(Handle,
                             &DestinationString,
                             KeyValuePartialInformation,
                             *Buffer,
                             ResultLength,
                             &ResultLength);
    if (!NT_SUCCESS(Status))
    {
        /* Free the buffer if we couldn't read the data */
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC GetRegistryValueBuffer: failed ZwQueryValueKey: %X.\n", Status);
        SacFreePool(*Buffer);
    }

Quit:
    /* Close the handle and exit */
    ZwClose(Handle);
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC GetRegistryValueBuffer: Exiting.\n");
    return Status;
}

NTSTATUS
NTAPI
SetRegistryValue(IN PCWSTR KeyName,
                 IN PWCHAR ValueName,
                 IN ULONG Type,
                 IN PVOID Data,
                 IN ULONG DataSize)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DestinationString;
    HANDLE Handle;
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC SetRegistryValue: Entering.\n");
    CHECK_PARAMETER1(KeyName);
    CHECK_PARAMETER2(ValueName);
    CHECK_PARAMETER4(Data);

    /* Open the specified key */
    RtlInitUnicodeString(&DestinationString, KeyName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &DestinationString,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwOpenKey(&Handle,
                       KEY_WRITE | SYNCHRONIZE | KEY_READ,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        /* Bail out on failure */
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC SetRegistryValue: failed ZwOpenKey: %X.\n", Status);
        return Status;
    }

    /* Set the specified value */
    RtlInitUnicodeString(&DestinationString, ValueName);
    Status = ZwSetValueKey(Handle, &DestinationString, 0, Type, Data, DataSize);
    if (!NT_SUCCESS(Status))
    {
        /* Print error on failure */
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC SetRegistryValue: failed ZwSetValueKey: %X.\n", Status);
    }

    /* Close the handle and exit */
    ZwClose(Handle);
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC SetRegistryValue: Exiting.\n");
    return Status;
}

NTSTATUS
NTAPI
CopyRegistryValueData(IN PVOID* Buffer,
                      IN PKEY_VALUE_PARTIAL_INFORMATION PartialInfo)
{
    NTSTATUS Status = STATUS_SUCCESS;
    CHECK_PARAMETER1(Buffer);
    CHECK_PARAMETER2(PartialInfo);

    /* Allocate space for registry data */
    *Buffer = SacAllocatePool(PartialInfo->DataLength, GLOBAL_BLOCK_TAG);
    if (*Buffer)
    {
        /* Copy the data into the buffer */
        RtlCopyMemory(*Buffer, PartialInfo->Data, PartialInfo->DataLength);
    }
    else
    {
        /* Set the correct error code */
        SAC_DBG(SAC_DBG_UTIL, "SAC CopyRegistryValueBuffer: Failed ALLOCATE.\n");
        Status = STATUS_NO_MEMORY;
    }

    /* Return the result */
    return Status;
}

NTSTATUS
NTAPI
TranslateMachineInformationXML(IN PWCHAR *Buffer,
                               IN PWCHAR ExtraData)
{
    NTSTATUS Status;
    SIZE_T Size;
    PWCHAR p;
    CHECK_PARAMETER1(Buffer);

    /* Start by believing the world is beautiful */
    Status = STATUS_SUCCESS;

    /* First, the header */
    Size = wcslen(L"<machine-info>\r\n");

    /* Do we have a machine name? */
    if (MachineInformation->MachineName)
    {
        /* Go and add it in */
        Size += wcslen(MachineInformation->MachineName);
        Size += wcslen(L"<name>%s</name>\r\n");
    }

    /* Do we have a GUID? */
    if (MachineInformation->MachineGuid)
    {
        /* Go and add it in */
        Size += wcslen(MachineInformation->MachineGuid);
        Size += wcslen(L"<guid>%s</guid>\r\n");
    }

    /* Do we know the processor? */
    if (MachineInformation->ProcessorArchitecture)
    {
        /* Go and add it in */
        Size += wcslen(MachineInformation->ProcessorArchitecture);
        Size += wcslen(L"<processor-architecture>%s</processor-architecture>\r\n");
    }

    /* Do we have the version? */
    if (MachineInformation->MajorVersion)
    {
        /* Go and add it in */
        Size += wcslen(MachineInformation->MajorVersion);
        Size += wcslen(L"<os-version>%s</os-version>\r\n");
    }

    /* Do we have the build? */
    if (MachineInformation->BuildNumber)
    {
        /* Go and add it in */
        Size += wcslen(MachineInformation->BuildNumber);
        Size += wcslen(L"<os-build-number>%s</os-build-number>\r\n");
    }

    /* Do we have the product type? */
    if (MachineInformation->ProductType)
    {
        /* Go and add it in */
        Size += wcslen(MachineInformation->ProductType);
        Size += wcslen(L"<os-product>%s</os-product>\r\n");
    }

    /* Do we have a service pack? */
    if (MachineInformation->ServicePack)
    {
        /* Go and add it in */
        Size += wcslen(MachineInformation->ServicePack);
        Size += wcslen(L"<os-service-pack>%s</os-service-pack>\r\n");
    }

    /* Anything else we need to know? Add it in too */
    if (ExtraData) Size += wcslen(ExtraData);

    /* Finally, add the footer */
    Size += wcslen(L"</machine-info>\r\n");

    /* Convert to bytes and add a NULL */
    Size += sizeof(ANSI_NULL);
    Size *= sizeof(WCHAR);

    /* Allocate space for the buffer */
    p = SacAllocatePool(Size, GLOBAL_BLOCK_TAG);
    *Buffer = p;
    if (!p) return STATUS_NO_MEMORY;

    wcscpy(p, L"<machine-info>\r\n");
    p += wcslen(L"<machine-info>\r\n");

    if (MachineInformation->MachineName)
    {
        p += swprintf(p,
                      L"<name>%s</name>\r\n",
                      MachineInformation->MachineName);
    }

    if (MachineInformation->MachineGuid)
    {
        p += swprintf(p,
                      L"<guid>%s</guid>\r\n",
                      MachineInformation->MachineGuid);
    }

    if (MachineInformation->ProcessorArchitecture)
    {
        p += swprintf(p,
                      L"<processor-architecture>%s</processor-architecture>\r\n",
                      MachineInformation->ProcessorArchitecture);
    }

    if (MachineInformation->MajorVersion)
    {
        p += swprintf(p,
                      L"<os-version>%s</os-version>\r\n",
                      MachineInformation->MajorVersion);
    }

    if (MachineInformation->BuildNumber)
    {
        p += swprintf(p,
                      L"<os-build-number>%s</os-build-number>\r\n",
                      MachineInformation->BuildNumber);
    }

    if (MachineInformation->ProductType)
    {
        p += swprintf(p,
                      L"<os-product>%s</os-product>\r\n",
                      MachineInformation->ProductType);
    }

    if (MachineInformation->ServicePack)
    {
        p += swprintf(p,
                      L"<os-service-pack>%s</os-service-pack>\r\n",
                      MachineInformation->ServicePack);
    }

    if (ExtraData)
    {
        wcscpy(p, ExtraData);
        p += wcslen(ExtraData);
    }

    wcscpy(p, L"</machine-info>\r\n");
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "MachineInformation: %S\n", *Buffer);
    ASSERT((((ULONG)wcslen(*Buffer) + 1) * sizeof(WCHAR)) <= Size);
    return Status;
}

VOID
NTAPI
InitializeMachineInformation(VOID)
{
    NTSTATUS Status;
    PWCHAR GuidString, MajorVersion, ServicePack, BuildNumber, MessageBuffer;
    PWCHAR ProductType;
    ULONG SuiteTypeMessage;
    BOOLEAN SetupInProgress = FALSE;
    GUID SystemGuid;
    SIZE_T RealSize, Size, OutputSize;
    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo;
    RTL_OSVERSIONINFOEXW VersionInformation;
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC Initialize Machine Information : Entering.\n");

    /* Don't do anything if we already queried this */
    if (MachineInformation)
    {
        SAC_DBG(SAC_DBG_MACHINE, "SAC Initialize Machine Information:: MachineInformationBuffer already initialized.\n");
        return;
    }

    /* Allocate the machine information */
    MachineInformation = SacAllocatePool(sizeof(*MachineInformation),
                                         GLOBAL_BLOCK_TAG);
    if (!MachineInformation)
    {
        goto Fail;
    }

    /* Zero it out for now */
    RtlZeroMemory(MachineInformation, sizeof(*MachineInformation));

    /* Query OS version */
    RtlZeroMemory(&VersionInformation, sizeof(VersionInformation));
    VersionInformation.dwOSVersionInfoSize = sizeof(VersionInformation);
    Status = RtlGetVersion((PRTL_OSVERSIONINFOW)&VersionInformation);
    if (!NT_SUCCESS(Status))
    {
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC InitializeMachineInformation: Exiting (2).\n");
        goto Fail;
    }

    /* Check if setup is in progress */
    Status = GetRegistryValueBuffer(L"\\Registry\\Machine\\System\\Setup",
                                    L"SystemSetupInProgress",
                                    &PartialInfo);
    if (NT_SUCCESS(Status))
    {
        /* The key is there, is the value set? */
        if (*(PULONG)PartialInfo->Data) SetupInProgress = TRUE;
        SacFreePool(PartialInfo);
        if (SetupInProgress)
        {
            /* Yes, so we'll use a special hostname to identify this */
            MessageBuffer = GetMessage(SAC_UNINITIALIZED_MSG);
            Size = wcslen(MessageBuffer);
            ASSERT(Size > 0);
            RealSize = Size * sizeof(WCHAR) + sizeof(UNICODE_NULL);

            /* Make room for it and copy it in there */
            MachineInformation->MachineName = SacAllocatePool(RealSize,
                                                              GLOBAL_BLOCK_TAG);
            if (MachineInformation->MachineName)
            {
                wcscpy(MachineInformation->MachineName, MessageBuffer);
            }
        }
    }

    /* If we are not in setup mode, or if we failed to check... */
    if (!SetupInProgress)
    {
        /* Query the computer name */
        Status = GetRegistryValueBuffer(L"\\Registry\\Machine\\System\\"
                                        L"CurrentControlSet\\Control\\"
                                        L"ComputerName\\ComputerName",
                                        L"ComputerName",
                                        &PartialInfo);
        if (!NT_SUCCESS(Status))
        {
            /* It's not critical, but we won't have it */
            SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC InitializeMachineInformation: Failed to get machine name.\n");
        }
        else
        {
            /* We have the name, copy it from the registry */
            Status = CopyRegistryValueData((PVOID*)&MachineInformation->
                                           MachineName,
                                           PartialInfo);
            SacFreePool(PartialInfo);
            if (!NT_SUCCESS(Status))
            {
                SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC InitializeMachineInformation: Exiting (20).\n");
                goto Fail;
            }
        }
    }

    /* Next step, try to get the machine GUID */
    RtlZeroMemory(&SystemGuid, sizeof(SystemGuid));
    OutputSize = sizeof(SystemGuid);
    Status = HeadlessDispatch(HeadlessCmdQueryGUID,
                              NULL,
                              0,
                              &SystemGuid,
                              &OutputSize);
    if (!NT_SUCCESS(Status))
    {
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC InitializeMachineInformation: Failed to get Machine GUID.\n");
    }
    else
    {
        /* We have it -- make room for it */
        GuidString = SacAllocatePool(0x50, GLOBAL_BLOCK_TAG);
        if (!GuidString)
        {
            SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC InitializeMachineInformation: Exiting (31).\n");
            goto Fail;
        }

        /* Build the string with the GUID in it, and save the ppointer to it */
        swprintf(GuidString,
                 L"%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                 SystemGuid.Data1,
                 SystemGuid.Data2,
                 SystemGuid.Data3,
                 SystemGuid.Data4[0],
                 SystemGuid.Data4[1],
                 SystemGuid.Data4[2],
                 SystemGuid.Data4[3],
                 SystemGuid.Data4[4],
                 SystemGuid.Data4[5],
                 SystemGuid.Data4[6],
                 SystemGuid.Data4[7]);
        MachineInformation->MachineGuid = GuidString;
    }

    /* Next, query the processor architecture */
    Status = GetRegistryValueBuffer(L"\\Registry\\Machine\\System\\"
                                    L"CurrentControlSet\\Control\\"
                                    L"Session Manager\\Environment",
                                    L"PROCESSOR_ARCHITECTURE",
                                    &PartialInfo);
    if (!NT_SUCCESS(Status))
    {
        /* It's not critical, but we won't have it */
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC InitializeMachineInformation: Exiting (30).\n");
    }
    else
    {
        /* We have it! Copy the value from the registry */
        Status = CopyRegistryValueData((PVOID*)&MachineInformation->
                                       ProcessorArchitecture,
                                       PartialInfo);
        SacFreePool(PartialInfo);
        if (!NT_SUCCESS(Status))
        {
            SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC InitializeMachineInformation: Exiting (30).\n");
            goto Fail;
        }
    }

    /* Now allocate a buffer for the OS version number */
    MajorVersion = SacAllocatePool(0x18, GLOBAL_BLOCK_TAG);
    if (!MajorVersion)
    {
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC InitializeMachineInformation: Exiting (50).\n");
        goto Fail;
    }

    /* Build the buffer and set a pointer to it */
    swprintf(MajorVersion,
             L"%d.%d",
             VersionInformation.dwMajorVersion,
             VersionInformation.dwMinorVersion);
    MachineInformation->MajorVersion = MajorVersion;

    /* Now allocate a buffer for the OS build number */
    BuildNumber = SacAllocatePool(0xC, GLOBAL_BLOCK_TAG);
    if (!BuildNumber)
    {
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC InitializeMachineInformation: Exiting (60).\n");
        goto Fail;
    }

    /* Build the buffer and set a pointer to it */
    swprintf(BuildNumber, L"%d", VersionInformation.dwBuildNumber);
    MachineInformation->BuildNumber = BuildNumber;

    /* Now check what kind of SKU this is */
    if (ExVerifySuite(DataCenter))
    {
        SuiteTypeMessage = SAC_DATACENTER_SUITE_MSG;
    }
    else if (ExVerifySuite(EmbeddedNT))
    {
        SuiteTypeMessage = SAC_EMBEDDED_SUITE_MSG;
    }
    else if (ExVerifySuite(Enterprise))
    {
        SuiteTypeMessage = SAC_ENTERPRISE_SUITE_MSG;
    }
    else
    {
        /* Unknown or perhaps a client SKU */
        SuiteTypeMessage = SAC_NO_SUITE_MSG;
    }

    /* Get the string that corresponds to the SKU type */
    MessageBuffer = GetMessage(SuiteTypeMessage);
    if (!MessageBuffer)
    {
        /* We won't have it, but this isn't critical */
        SAC_DBG(SAC_DBG_INIT, "SAC InitializeMachineInformation: Failed to get product type.\n");
    }
    else
    {
        /* Calculate the size we need to hold the string */
        Size = wcslen(MessageBuffer);
        ASSERT(Size > 0);
        RealSize = Size * sizeof(WCHAR) + sizeof(UNICODE_NULL);

        /* Allocate a buffer for it */
        ProductType = SacAllocatePool(RealSize, GLOBAL_BLOCK_TAG);
        if (!ProductType)
        {
            SAC_DBG(SAC_DBG_INIT, "SAC InitializeMachineInformation: Failed product type memory allocation.\n");
            goto Fail;
        }

        /* Copy the string and set the pointer */
        RtlCopyMemory(ProductType, MessageBuffer, RealSize);
        MachineInformation->ProductType = ProductType;
    }

    /* Check if this is a SP version or RTM version */
    if (VersionInformation.wServicePackMajor)
    {
        /* This is a service pack, allocate a buffer for the version */
        ServicePack = SacAllocatePool(0x18, GLOBAL_BLOCK_TAG);
        if (ServicePack)
        {
            /* Build the buffer and set a pointer to it */
            swprintf(ServicePack,
                     L"%d.%d",
                     VersionInformation.wServicePackMajor,
                     VersionInformation.wServicePackMinor);
            MachineInformation->ServicePack = ServicePack;

            /* We've collected all the machine info and are done! */
            return;
        }

        /* This is the failure path */
        SAC_DBG(SAC_DBG_INIT, "SAC InitializeMachineInformation: Failed service pack memory allocation.\n");
    }
    else
    {
        /* Get a generic string that indicates there's no service pack */
        MessageBuffer = GetMessage(SAC_NO_DATA_MSG);
        Size = wcslen(MessageBuffer);
        ASSERT(Size > 0);
        RealSize = Size * sizeof(WCHAR) + sizeof(UNICODE_NULL);

        /* Allocate memory for the "no service pack" string */
        ServicePack = SacAllocatePool(RealSize, GLOBAL_BLOCK_TAG);
        if (ServicePack)
        {
            /* Copy the buffer and set a pointer to it */
            RtlCopyMemory(ServicePack, MessageBuffer, RealSize);
            MachineInformation->ServicePack = ServicePack;

            /* We've collected all the machine info and are done! */
            return;
        }

        SAC_DBG(SAC_DBG_INIT, "SAC InitializeMachineInformation: Failed service pack memory allocation.\n");
    }

Fail:
    /* In the failure path, always cleanup the machine information buffer */
    if (MachineInformation)
    {
        SacFreePool(MachineInformation);
    }
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC Initialize Machine Information : Exiting with error.\n");
}

NTSTATUS
NTAPI
GetCommandConsoleLaunchingPermission(OUT PBOOLEAN Permission)
{
    NTSTATUS Status;
    PKEY_VALUE_PARTIAL_INFORMATION Dummy;

    /* Assume success and read the key */
    *Permission = TRUE;
    Status = GetRegistryValueBuffer(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\sacdrv",
                                    L"DisableCmdSessions",
                                    &Dummy);
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        /* The default is success */
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* Only if the key is present and set, do we disable permission */
        if (NT_SUCCESS(Status)) *Permission = FALSE;
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
ImposeSacCmdServiceStartTypePolicy(VOID)
{
    NTSTATUS Status;
    PKEY_VALUE_PARTIAL_INFORMATION Buffer = NULL;
    PULONG Data;

    /* Read the service start type*/
    Status = GetRegistryValueBuffer(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\sacsvr",
                                    L"Start",
                                    &Buffer);
    if (!NT_SUCCESS(Status)) return Status;

    /* If there's no start type, fail, as this is unusual */
    if (!Buffer) return STATUS_UNSUCCESSFUL;

    /* Read the value */
    Status = CopyRegistryValueData((PVOID*)&Data, Buffer);
    SacFreePool(Buffer);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check what the current start type is */
    switch (*Data)
    {
        /* It's boot, system, or disabled */
        case 1:
        case 2:
        case 4:
            /* Leave it as is */
            return Status;

        case 3:

            /* It's set to automatic, set it to system instead */
            *Data = 2;
            Status = SetRegistryValue(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\sacsvr",
                                      L"Start",
                                      REG_DWORD,
                                      Data,
                                      sizeof(ULONG));
            if (!NT_SUCCESS(Status))
            {
                SAC_DBG(SAC_DBG_INIT, "SAC ImposeSacCmdServiceStartTypePolicy: Failed SetRegistryValue: %X\n", Status);
            }
            break;

        default:
            ASSERT(FALSE);
    }

    return Status;
}

VOID
NTAPI
InitializeCmdEventInfo(VOID)
{
    /* Check if we were already initialized */
    if (HaveUserModeServiceCmdEventInfo)
    {
        /* Full state expected */
        ASSERT(RequestSacCmdEventObjectBody);
        ASSERT(RequestSacCmdSuccessEventObjectBody);
        ASSERT(RequestSacCmdFailureEventObjectBody);

        /* Dereference each wait object in turn */
        if (RequestSacCmdEventObjectBody)
        {
            ObDereferenceObject(RequestSacCmdEventObjectBody);
        }

        if (RequestSacCmdSuccessEventObjectBody)
        {
            ObDereferenceObject(RequestSacCmdSuccessEventObjectBody);
        }

        if (RequestSacCmdFailureEventObjectBody)
        {
            ObDereferenceObject(RequestSacCmdFailureEventObjectBody);
        }
    }

    /* Claer everything */
    RequestSacCmdEventObjectBody = NULL;
    RequestSacCmdEventWaitObjectBody = NULL;
    RequestSacCmdSuccessEventObjectBody = NULL;
    RequestSacCmdSuccessEventWaitObjectBody = NULL;
    RequestSacCmdFailureEventObjectBody = NULL;
    RequestSacCmdFailureEventWaitObjectBody = NULL;
    ServiceProcessFileObject = NULL;

    /* Reset state */
    HaveUserModeServiceCmdEventInfo = FALSE;
}

NTSTATUS
NTAPI
RegisterBlueScreenMachineInformation(VOID)
{
    PWCHAR XmlBuffer;
    PHEADLESS_CMD_SET_BLUE_SCREEN_DATA BsBuffer;
    SIZE_T Length, HeaderLength, TotalLength;
    NTSTATUS Status;
    ULONG i;

    /* Create the XML buffer and make sure it's OK */
    Status = TranslateMachineInformationXML(&XmlBuffer, NULL);
    CHECK_PARAMETER_WITH_STATUS(NT_SUCCESS(Status), Status);
    CHECK_PARAMETER_WITH_STATUS(XmlBuffer, STATUS_UNSUCCESSFUL);

    /* Compute the sizes and allocate a buffer for it */
    Length = wcslen(XmlBuffer);
    HeaderLength = strlen("MACHINEINFO");
    TotalLength = HeaderLength +
                  Length +
                  sizeof(*BsBuffer) +
                  2 * sizeof(ANSI_NULL);
    BsBuffer = SacAllocatePool(TotalLength, GLOBAL_BLOCK_TAG);
    CHECK_PARAMETER_WITH_STATUS(BsBuffer, STATUS_NO_MEMORY);

    /* Copy the XML property name */
    strcpy((PCHAR)BsBuffer->Data, "MACHINEINFO");
    BsBuffer->ValueIndex = HeaderLength + sizeof(ANSI_NULL);

    /* Copy the data and NULL-terminate it */
    for (i = 0; i < Length; i++)
    {
        BsBuffer->Data[BsBuffer->ValueIndex + i] = XmlBuffer[i];
    }
    BsBuffer->Data[BsBuffer->ValueIndex + i] = ANSI_NULL;

    /* Let the OS save the buffer for later */
    Status = HeadlessDispatch(HeadlessCmdSetBlueScreenData,
                              BsBuffer,
                              TotalLength,
                              NULL,
                              NULL);

    /* Failure or not, we don't need this anymore */
    SacFreePool(BsBuffer);
    SacFreePool(XmlBuffer);

    /* Return the result */
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC Initialize Machine Information: Exiting.\n");
    return Status;
}

VOID
NTAPI
FreeMachineInformation(VOID)
{
    ASSERT(MachineInformation);

    /* Free every cached string of machine information */
    if (MachineInformation->MachineName) SacFreePool(MachineInformation);
    if (MachineInformation->MachineGuid) SacFreePool(MachineInformation->MachineGuid);
    if (MachineInformation->ProcessorArchitecture) SacFreePool(MachineInformation->ProcessorArchitecture);
    if (MachineInformation->MajorVersion) SacFreePool(MachineInformation->MajorVersion);
    if (MachineInformation->BuildNumber) SacFreePool(MachineInformation->BuildNumber);
    if (MachineInformation->ProductType) SacFreePool(MachineInformation->ProductType);
    if (MachineInformation->ServicePack) SacFreePool(MachineInformation->ServicePack);
}

BOOLEAN
NTAPI
VerifyEventWaitable(IN HANDLE Handle,
                    OUT PVOID *WaitObject,
                    OUT PVOID *ActualWaitObject)
{
    PVOID Object;
    NTSTATUS Status;
    POBJECT_TYPE ObjectType;

    /* Reference the object */
    Status = ObReferenceObjectByHandle(Handle,
                                       EVENT_ALL_ACCESS,
                                       NULL,
                                       KernelMode,
                                       &Object,
                                       NULL);
    *WaitObject = Object;
    if (!NT_SUCCESS(Status))
    {
        SAC_DBG(SAC_DBG_INIT, "SAC VerifyEventWaitable: Unable to reference event object (%lx)\n", Status);
        return FALSE;
    }

    /* Check if the object itself is NOT being used */
    ObjectType = OBJECT_TO_OBJECT_HEADER(Object)->Type;
    if (ObjectType->TypeInfo.UseDefaultObject == FALSE)
    {
        /* Get the actual object that's being used for the wait */
        *ActualWaitObject = (PVOID)((ULONG_PTR)Object +
                                    (ULONG_PTR)ObjectType->DefaultObject);
        return TRUE;
    }

    /* Drop the reference we took */
    SAC_DBG(SAC_DBG_INIT, "SAC VerifyEventWaitable: event object not waitable!\n");
    ObDereferenceObject(*WaitObject);
    return FALSE;
}

NTSTATUS
NTAPI
SerialBufferGetChar(OUT PCHAR Char)
{
    /* Check if nothing's been produced yet */
    if (SerialPortConsumerIndex == SerialPortProducerIndex)
    {
        return STATUS_NO_DATA_DETECTED;
    }

    /* Consume the produced character and clear it*/
    *Char = SerialPortBuffer[SerialPortConsumerIndex];
    SerialPortBuffer[SerialPortConsumerIndex] = ANSI_NULL;

    /* Advance the index and return success */
    _InterlockedExchange(&SerialPortConsumerIndex,
                         (SerialPortConsumerIndex + 1) &
                         (SAC_SERIAL_PORT_BUFFER_SIZE - 1));
    return STATUS_SUCCESS;
}

ULONG
NTAPI
GetMessageLineCount(IN ULONG MessageIndex)
{
    ULONG LineCount = 0;
    PWCHAR Buffer;

    /* Get the message buffer */
    Buffer = GetMessage(MessageIndex);
    if (Buffer)
    {
        /* Scan it looking for new lines, and increment the count each time */
        while (*Buffer) if (*Buffer++ == L'\n') ++LineCount;
    }

    /* Return the line count */
    return LineCount;
}

ULONG
ConvertAnsiToUnicode(
    IN PWCHAR pwch,
    IN PCHAR pch,
    IN ULONG length
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
IsCmdEventRegistrationProcess(
    IN PFILE_OBJECT FileObject
    )
{
    return FALSE;
}

NTSTATUS
InvokeUserModeService(
    VOID
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
TranslateMachineInformationText(
    IN PWCHAR Buffer)
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
CopyAndInsertStringAtInterval(
    IN PWCHAR SourceStr,
    IN ULONG Interval,
    IN PWCHAR InsertStr,
    OUT PWCHAR pDestStr
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RegisterSacCmdEvent(
    IN PVOID Object,
    IN PKEVENT SetupCmdEvent[]
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
UnregisterSacCmdEvent(
    IN PFILE_OBJECT FileObject
    )
{
    return STATUS_NOT_IMPLEMENTED;
}
