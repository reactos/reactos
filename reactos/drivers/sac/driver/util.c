/*
 * PROJECT:     ReactOS Drivers
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/sac/driver/util.c
 * PURPOSE:     Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

#include "sacdrv.h"

/* GLOBALS *******************************************************************/

PCHAR Utf8ConversionBuffer;
ULONG Utf8ConversionBufferSize;
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

/* FUNCTIONS *****************************************************************/

BOOLEAN
NTAPI
SacTranslateUnicodeToUtf8(IN PWCHAR SourceBuffer,
                          IN ULONG SourceBufferLength,
                          OUT PCHAR DestinationBuffer,
                          IN ULONG DestinationBufferSize,
                          OUT PULONG UTF8Count,
                          OUT PULONG ProcessedCount)
{
    ASSERT(FALSE);
    return FALSE;
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
    /* FIXME: For now don't format anything */
    wcsncpy(FormattedString, MessageString, MessageSize / sizeof(WCHAR));
}

NTSTATUS
NTAPI
PreloadGlobalMessageTable(IN PVOID ImageBase)
{
    NTSTATUS Status;
    ULONG MessageId, TotalLength, TextSize, i;
    PWCHAR StringBuffer;
    PMESSAGE_RESOURCE_ENTRY MessageEntry;
    PAGED_CODE();
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC PreloadGlobalMessageTable: Entering.\n");

    /* Nothing to do if we already have a table */
    if (GlobalMessageTable) goto Exit;

    /* Loop through up to 200 messages */
    for (MessageId = 1; MessageId != SAC_MAX_MESSAGES; MessageId++)
    {
        /* Find this message ID in the string table*/
        Status = RtlFindMessage(ImageBase,
                                11,
                                LANG_NEUTRAL,
                                MessageId,
                                &MessageEntry);
        if (NT_SUCCESS(Status))
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
        Status = RtlFindMessage(ImageBase,
                                11,
                                LANG_NEUTRAL,
                                MessageId,
                                &MessageEntry);
        if (NT_SUCCESS(Status))
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
            StringBuffer += TextSize;

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
TranslateMachineInformationXML(IN PWCHAR *Buffer,
                               IN PWCHAR ExtraData)
{
    NTSTATUS Status;
    ULONG Size;
    PWCHAR p;
    CHECK_PARAMETER1(Buffer);

    Status = STATUS_SUCCESS;
    Size = wcslen(L"<machine-info>\r\n");

    if (MachineInformation->MachineName)
    {
        Size += wcslen(MachineInformation->MachineName);
        Size += wcslen(L"<name>%s</name>\r\n");
    }

    if (MachineInformation->MachineGuid)
    {
        Size += wcslen(MachineInformation->MachineGuid);
        Size += wcslen(L"<guid>%s</guid>\r\n");
    }

    if (MachineInformation->ProcessorArchitecture)
    {
        Size += wcslen(MachineInformation->ProcessorArchitecture);
        Size += wcslen(L"<processor-architecture>%s</processor-architecture>\r\n");
    }

    if (MachineInformation->MajorVersion)
    {
        Size += wcslen(MachineInformation->MajorVersion);
        Size += wcslen(L"<os-version>%s</os-version>\r\n");
    }

    if (MachineInformation->BuildNumber)
    {
        Size += wcslen(MachineInformation->BuildNumber);
        Size += wcslen(L"<os-build-number>%s</os-build-number>\r\n");
    }

    if (MachineInformation->ProductType)
    {
        Size += wcslen(MachineInformation->ProductType);
        Size += wcslen(L"<os-product>%s</os-product>\r\n");
    }

    if (MachineInformation->ServicePack)
    {
        Size += wcslen(MachineInformation->ServicePack);
        Size += wcslen(L"<os-service-pack>%s</os-service-pack>\r\n");
    }

    if (ExtraData) Size += wcslen(ExtraData);

    Size += wcslen(L"</machine-info>\r\n");

    p = (PWCHAR)SacAllocatePool((Size + sizeof(ANSI_NULL)) * sizeof(WCHAR), GLOBAL_BLOCK_TAG);

    *Buffer = p;
    if (!p) return STATUS_NO_MEMORY;

    Size = wcslen(L"<machine-info>\r\n");
    wcscpy(p, L"<machine-info>\r\n");

    p += Size;

    if (MachineInformation->MachineName)
    {
        p += swprintf(p, L"<name>%s</name>\r\n", MachineInformation->MachineName);
    }

    if (MachineInformation->MachineGuid)
    {
        p += swprintf(p, L"<guid>%s</guid>\r\n", MachineInformation->MachineGuid);
    }

    if (MachineInformation->ProcessorArchitecture)
    {
        p += swprintf(p, L"<processor-architecture>%s</processor-architecture>\r\n", MachineInformation->ProcessorArchitecture);
    }

    if (MachineInformation->MajorVersion)
    {
        p += swprintf(p, L"<os-version>%s</os-version>\r\n", MachineInformation->MajorVersion);
    }

    if (MachineInformation->BuildNumber)
    {
        p += swprintf(p, L"<os-build-number>%s</os-build-number>\r\n", MachineInformation->BuildNumber);
    }

    if (MachineInformation->ProductType)
    {
        p += swprintf(p, L"<os-product>%s</os-product>\r\n", MachineInformation->ProductType);
    }

    if (MachineInformation->ServicePack)
    {
        p += swprintf(p, L"<os-service-pack>%s</os-service-pack>\r\n", MachineInformation->ServicePack);
    }

    if (ExtraData)
    {
        Size = wcslen(ExtraData);
        wcscpy(p, ExtraData);
        p += Size;
    }

    wcscpy(p, L"</machine-info>\r\n");
    ASSERT((((ULONG) wcslen(*Buffer) + 1) * sizeof(WCHAR)) <= Size);
    return Status;
}

VOID
NTAPI
InitializeMachineInformation(VOID)
{
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC Initialize Machine Information : Entering.\n");

    /* FIXME: TODO */
    MachineInformation = NULL;
    ASSERT(FALSE);

    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC Initialize Machine Information : Exiting with error.\n");
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
    SIZE_T ResultLength = 0;
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC GetRegistryValueBuffer: Entering.\n");
    CHECK_PARAMETER1(KeyName);
    CHECK_PARAMETER2(ValueName);

    /* Open the specified key */
    RtlInitUnicodeString(&DestinationString, KeyName); 
    InitializeObjectAttributes(&ObjectAttributes,
                               &DestinationString,
                               OBJ_CASE_INSENSITIVE,
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
    if (!ResultLength) return Status;

    /* Allocate the buffer for the partial info structure and our integer data */
    ResultLength += sizeof(ULONG);
    *Buffer = SacAllocatePool(ResultLength, GLOBAL_BLOCK_TAG);
    if (!*Buffer)
    {
        SAC_DBG(1, "SAC GetRegistryValueBuffer: failed allocation\n");
        return Status;
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

    /* Return the result */
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC SetRegistryValue: Exiting.\n");
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
                               OBJ_CASE_INSENSITIVE,
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
        SAC_DBG(1, "SAC SetRegistryValue: failed ZwSetValueKey: %X.\n", Status);
    }

    /* Close the handle and exit */
    NtClose(Handle);
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC SetRegistryValue: Exiting.\n");
    return Status;
}

NTSTATUS
NTAPI
CopyRegistryValueData(IN PULONG* Buffer,
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
    Status = CopyRegistryValueData(&Data, Buffer);
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
    /* Check if we were already initailized */
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
    PHEADLESS_BLUE_SCREEN_DATA BsBuffer;
    ULONG Length, HeaderLength, TotalLength;
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
    strcpy((PCHAR)BsBuffer->XMLData, "MACHINEINFO");
    BsBuffer->Property = (PUCHAR)HeaderLength + sizeof(ANSI_NULL);

    /* Copy the data and NULL-terminate it */
    for (i = 0; i < Length; i++)
    {
        BsBuffer->XMLData[HeaderLength + sizeof(ANSI_NULL) + i] = XmlBuffer[i];
    }
    BsBuffer->XMLData[HeaderLength + sizeof(ANSI_NULL) + i] = ANSI_NULL;

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

BOOLEAN
SacTranslateUtf8ToUnicode(
	IN CHAR Utf8Char,
	IN PCHAR UnicodeBuffer, 
	OUT PCHAR Utf8Value
	)
{
	return FALSE;
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

ULONG
GetMessageLineCount(
	IN ULONG MessageIndex
	)
{
	return 0;
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
