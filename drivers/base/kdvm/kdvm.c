/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kdvm/kdvm.c
 * PURPOSE:         VM independent function for kdvbox/kd
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include "kdvm.h"

static CHAR KdVmCmdMagic[] = "~kdVMvA ";
static CHAR KdVmReplyMagic[] = "++kdVMvA ";
static const UCHAR KDVM_CMD_TestConnection = 't';
static const UCHAR KDVM_CMD_ReceivePacket = 'r';
static const UCHAR KDVM_CMD_SendPacket = 's';
static const UCHAR KDVM_CMD_VersionReport = 'v';

UCHAR KdVmDataBuffer[KDVM_BUFFER_SIZE];
PHYSICAL_ADDRESS KdVmBufferPhysicalAddress;
ULONG KdVmBufferPos;

PFNDBGPRNT KdpDbgPrint;


/* PRIVATE FUNCTIONS **********************************************************/

static
VOID
KdVmDbgDumpRow(
    _In_ PUCHAR Buffer,
    _In_ ULONG Size)
{
    ULONG i;
    for (i = 0;i < Size; i++)
    {
        KdpDbgPrint("%02x ", Buffer[i]);
    }
    KdpDbgPrint("\n");
}

VOID
NTAPI
KdVmDbgDumpBuffer(
    _In_ PVOID Buffer,
    _In_ ULONG Size)
{
    PUCHAR CurrentRow;
    ULONG i;

    CurrentRow = Buffer;
    for (i = 0; i < (Size / 16); i++)
    {
        KdVmDbgDumpRow(CurrentRow, 16);
        CurrentRow += 16;
    }
    KdVmDbgDumpRow(CurrentRow, (Size % 16));
}

static
BOOLEAN
KdVmAddToBuffer(
    _In_ PVOID Data,
    _In_ ULONG DataSize)
{
    if (((KdVmBufferPos + DataSize) > KDVM_BUFFER_SIZE) ||
        ((KdVmBufferPos + DataSize) < KdVmBufferPos))
    {
        KDDBGPRINT("KdVmAddToBuffer: Buffer overflow! Need %lu, remaining: %lu\n",
                   DataSize, KDVM_BUFFER_SIZE - KdVmBufferPos);
        return FALSE;
    }

    RtlCopyMemory(&KdVmDataBuffer[KdVmBufferPos], Data, DataSize);
    KdVmBufferPos += DataSize;
    return TRUE;
}

static
BOOLEAN
KdVmAddCommandToBuffer(
    _In_ UCHAR Command,
    _In_ PVOID Buffer,
    _In_ SIZE_T BufferSize)
{
    KDVM_CMD_HEADER Header;

    RtlCopyMemory(&Header.Magic, KdVmCmdMagic, sizeof(Header.Magic));
    Header.Command = Command;

    if (!KdVmAddToBuffer(&Header, sizeof(Header)))
        return FALSE;

    if (!KdVmAddToBuffer(Buffer, BufferSize))
        return FALSE;

    return TRUE;
}

static
PVOID
KdVmSendReceive(
    _Out_ PULONG ReceiveDataSize)
{
    PVOID ReceiveData;
    PKDVM_RECEIVE_HEADER ReceiveHeader;

    KdVmKdVmExchangeData(&ReceiveData, ReceiveDataSize);
    ReceiveHeader = ReceiveData;

    if (*ReceiveDataSize < sizeof(*ReceiveHeader))
    {
        KDDBGPRINT("KdVmSendReceive: received data too small: 0x%x\n", *ReceiveDataSize);
        *ReceiveDataSize = 0;
        return NULL;
    }

    if (ReceiveHeader->Id != 0x2031 /* '01' */)
    {
        KDDBGPRINT("KdVmSendReceive: got invalid Id: 0x%x\n", ReceiveHeader->Id);
        *ReceiveDataSize = 0;
        return NULL;
    }

    if (RtlEqualMemory(ReceiveHeader->Magic, KdVmReplyMagic, 9))
    {
        KDDBGPRINT("KdVmSendReceive: got invalid Magic: '%*s'\n",
                   sizeof(KdVmReplyMagic), ReceiveHeader->Magic);
        *ReceiveDataSize = 0;
        return NULL;
    }

    *ReceiveDataSize -= sizeof(*ReceiveHeader);
    return (PVOID)(ReceiveHeader + 1);
}

static
NTSTATUS
KdVmNegotiateProtocolVersions(VOID)
{
    ULONG Version = KDRPC_PROTOCOL_VERSION;
    ULONG ReceivedSize;
    PULONG ReceivedVersion;
    KDDBGPRINT("KdVmNegotiateProtocolVersions()\n");

    /* Prepare the buffer */
    KdVmPrepareBuffer();

    if (!KdVmAddCommandToBuffer(KDVM_CMD_VersionReport, &Version, sizeof(Version)))
    {
        KDDBGPRINT("Failed to do VersionReport\n");
        return STATUS_CONNECTION_REFUSED;
    }

    ReceivedVersion = KdVmSendReceive(&ReceivedSize);
    if (ReceivedSize != sizeof(ULONG))
    {
        KDDBGPRINT("Invalid size for VersionReport: %lx\n", ReceivedSize);
        return STATUS_CONNECTION_REFUSED;
    }

    if (*ReceivedVersion != KDRPC_PROTOCOL_VERSION)
    {
        KDDBGPRINT("Invalid Version: %lx\n", *ReceivedVersion);
        return STATUS_CONNECTION_REFUSED; //STATUS_PROTOCOL_NOT_SUPPORTED;
    }

    return STATUS_SUCCESS;
}

static
BOOLEAN
TestConnectionOnChannel(VOID)
{
    UCHAR TestBuffer[KDRPC_TEST_BUFFER_SIZE];
    PUCHAR ReceivedBuffer;
    ULONG i, ReceivedSize;

    /* Prepare the buffer */
    KdVmPrepareBuffer();

    for (i = 0; i < sizeof(TestBuffer); i++)
        TestBuffer[i] = (UCHAR)i;

    if (!KdVmAddCommandToBuffer(KDVM_CMD_TestConnection, TestBuffer, sizeof(TestBuffer)))
    {
        KDDBGPRINT("Failed to do TestConnection\n");
        return FALSE;
    }

    ReceivedBuffer = KdVmSendReceive(&ReceivedSize);
    if (ReceivedSize != sizeof(TestBuffer))
    {
        KDDBGPRINT("Invalid size for TestConnection: %lx\n", ReceivedSize);
        return FALSE;
    }

    for (i = 0; i < sizeof(TestBuffer); i++)
    {
        if (ReceivedBuffer[i] != (UCHAR)(i ^ 0x55))
        {
            KDDBGPRINT("Wrong test data @ %lx, expected %x, got %x\n",
                       i, (UCHAR)(i ^ 0x55), TestBuffer[i]);
            return FALSE;
        }
    }

    KDDBGPRINT("TestConnectionOnChannel: success\n");
    return TRUE;
}

static
BOOLEAN
KdVmTestConnectionWithHost(VOID)
{
    ULONG i, j;
    KDDBGPRINT("KdVmTestConnectionWithHost()\n");

    for (j = 0; j < 2; j++)
    {
        //VMWareRPC::OpenChannel
        for (i = 0; i < CONNECTION_TEST_ROUNDS / 2; i++)
        {
            if (!TestConnectionOnChannel())
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

NTSTATUS
NTAPI
KdD0Transition(VOID)
{
    /* Nothing to do */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdD3Transition(VOID)
{
    /* Nothing to do */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdSave(
    _In_ BOOLEAN SleepTransition)
{
    /* Nothing to do */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdRestore(
    _In_ BOOLEAN SleepTransition)
{
    /* Nothing to do */
    return STATUS_SUCCESS;
}

/******************************************************************************
 * \name KdDebuggerInitialize0
 * \brief Phase 0 initialization.
 * \param [opt] LoaderBlock Pointer to the Loader parameter block. Can be NULL.
 * \return Status
 */
NTSTATUS
NTAPI
KdDebuggerInitialize0(
    _In_opt_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PCHAR CommandLine, PortString;
    NTSTATUS Status;

    /* Check if we have a LoaderBlock */
    if (LoaderBlock != NULL)
    {
        /* HACK */
        KdpDbgPrint = LoaderBlock->u.I386.CommonDataArea;
        KDDBGPRINT("KdDebuggerInitialize0\n");

        /* Get the Command Line */
        CommandLine = LoaderBlock->LoadOptions;

        /* Upcase it */
        _strupr(CommandLine);

        /* Check if we got the /DEBUGPORT parameter */
        PortString = strstr(CommandLine, "DEBUGPORT");
        if (PortString)
        {
            /* Move past the actual string, to reach the port*/
            PortString += strlen("DEBUGPORT");

            /* Now get past any spaces and skip the equal sign */
            while (*PortString == ' ') PortString++;
            PortString++;

            /* Do we have a serial port? */
            if (strncmp(PortString, "VBOX", 3) != 0)
            {
                KDDBGPRINT("Invalid debugport: '%s'\n", CommandLine);
                return STATUS_INVALID_PARAMETER;
            }
        }
    }

    /* Get the physical address of the data buffer */
    KdVmBufferPhysicalAddress = MmGetPhysicalAddress(KdVmDataBuffer);
    KDDBGPRINT("KdVmBufferPhysicalAddress = %llx\n", KdVmBufferPhysicalAddress.QuadPart);

    Status = KdVmNegotiateProtocolVersions();
    if (!NT_SUCCESS(Status))
        return Status;

    if (!KdVmTestConnectionWithHost())
        return STATUS_CONNECTION_REFUSED;

    return STATUS_SUCCESS;
}

/******************************************************************************
 * \name KdDebuggerInitialize1
 * \brief Phase 1 initialization.
 * \param [opt] LoaderBlock Pointer to the Loader parameter block. Can be NULL.
 * \return Status
 */
NTSTATUS
NTAPI
KdDebuggerInitialize1(
    _In_opt_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* Nothing to do */
    KDDBGPRINT("KdDebuggerInitialize1()\n");
    return STATUS_SUCCESS;
}

VOID
NTAPI
KdSendPacket(
    _In_ ULONG PacketType,
    _In_ PSTRING MessageHeader,
    _In_ PSTRING MessageData,
    _Inout_ PKD_CONTEXT KdContext)
{
    KDVM_SEND_PKT_REQUEST SendPktRequest;
    PKDVM_SEND_PKT_RESULT SendPktResult;
    ULONG ReceivedSize;
    KDDBGPRINT("KdSendPacket(0x%lx, ...)\n", PacketType);

    do
    {

        RtlZeroMemory(&SendPktRequest, sizeof(SendPktRequest));

        SendPktRequest.PacketType = PacketType;
        SendPktRequest.Info.KdDebuggerNotPresent = KD_DEBUGGER_NOT_PRESENT;
        SendPktRequest.Info.KdDebuggerEnabledAvailable = 1;
        SendPktRequest.Info.KdDebuggerEnabled = SharedUserData->KdDebuggerEnabled;

        if (MessageHeader != NULL)
        {
            SendPktRequest.MessageHeader.Length = MessageHeader->Length;
            SendPktRequest.MessageHeader.MaximumLength = MessageHeader->MaximumLength;
            SendPktRequest.HeaderSize = MessageHeader->Length;
        }

        if (MessageData != NULL)
        {
            SendPktRequest.MessageData.Length = MessageData->Length;
            SendPktRequest.MessageData.MaximumLength = MessageData->MaximumLength;
            SendPktRequest.DataSize = MessageData->Length;
        }

        if (KdContext != NULL)
        {
            RtlCopyMemory(&SendPktRequest.KdContext,
                          KdContext,
                          sizeof(SendPktRequest.KdContext));
        }


        /* Prepare the buffer */
        KdVmPrepareBuffer();

        if (!KdVmAddCommandToBuffer(KDVM_CMD_SendPacket, &SendPktRequest, sizeof(SendPktRequest)))
        {
            KDDBGPRINT("KdSendPacket: Failed to add SendPacket command\n");
            return;
        }

        if (MessageHeader != NULL)
        {
            if (!KdVmAddToBuffer(MessageHeader->Buffer, MessageHeader->Length))
            {
                KDDBGPRINT("KdSendPacket: Failed to add MessageHeader\n");
                return;
            }
        }

        if (MessageData != NULL)
        {
            if (!KdVmAddToBuffer(MessageData->Buffer, MessageData->Length))
            {
                KDDBGPRINT("KdSendPacket: Failed to add MessageData\n");
                return;
            }
        }

        SendPktResult = KdVmSendReceive(&ReceivedSize);
        if (ReceivedSize != sizeof(*SendPktResult))
        {
            KDDBGPRINT("KdSendPacket: Invalid size for SendPktResult: %lx\n", ReceivedSize);
            return;
        }

        if (KdContext != NULL)
        {
            RtlCopyMemory(KdContext,
                          &SendPktResult->KdContext,
                          sizeof(SendPktResult->KdContext));
        }

        KD_DEBUGGER_NOT_PRESENT = SendPktResult->Info.KdDebuggerNotPresent;
        if (SendPktResult->Info.KdDebuggerEnabledAvailable)
            SharedUserData->KdDebuggerEnabled = SendPktResult->Info.KdDebuggerEnabled != 0;

        if (SendPktResult->Info.RetryKdSendPacket)
        {
            KDDBGPRINT("KdSendPacket: RetryKdSendPacket!\n");
        }

    } while (SendPktResult->Info.RetryKdSendPacket);

    KDDBGPRINT("KdSendPacket: Success!\n");
}


KDP_STATUS
NTAPI
KdReceivePacket(
    _In_ ULONG PacketType,
    _Out_ PSTRING MessageHeader,
    _Out_ PSTRING MessageData,
    _Out_ PULONG DataLength,
    _Inout_opt_ PKD_CONTEXT KdContext)
{
    KDVM_RECV_PKT_REQUEST RecvPktRequest;
    PKDVM_RECV_PKT_RESULT RecvPktResult;
    ULONG ReceivedSize, ExpectedSize;
    PUCHAR Buffer;
    KDDBGPRINT("KdReceivePacket(0x%lx, ...)\n", PacketType);

    /* Prepare the buffer */
    KdVmPrepareBuffer();

    RtlZeroMemory(&RecvPktRequest, sizeof(RecvPktRequest));

    RecvPktRequest.PacketType = PacketType;
    RecvPktRequest.Info.KdDebuggerNotPresent = KD_DEBUGGER_NOT_PRESENT;
    RecvPktRequest.Info.KdDebuggerEnabledAvailable = 1;
    RecvPktRequest.Info.KdDebuggerEnabled = SharedUserData->KdDebuggerEnabled;

    if (MessageHeader != NULL)
    {
        RecvPktRequest.MessageHeader.Length = MessageHeader->Length;
        RecvPktRequest.MessageHeader.MaximumLength = MessageHeader->MaximumLength;
    }

    if (MessageData != NULL)
    {
        RecvPktRequest.MessageData.Length = MessageData->Length;
        RecvPktRequest.MessageData.MaximumLength = MessageData->MaximumLength;
    }

    if (KdContext != NULL)
    {
        RtlCopyMemory(&RecvPktRequest.KdContext,
                      KdContext,
                      sizeof(RecvPktRequest.KdContext));
    }

    if (!KdVmAddCommandToBuffer(KDVM_CMD_ReceivePacket, &RecvPktRequest, sizeof(RecvPktRequest)))
    {
        KDDBGPRINT("KdReceivePacket: Failed to add SendPacket command\n");
        return KDP_PACKET_RESEND;
    }

    RecvPktResult = KdVmSendReceive(&ReceivedSize);
    if (ReceivedSize < sizeof(*RecvPktResult))
    {
        KDDBGPRINT("KdReceivePacket: Invalid size for RecvPktResult: %lx\n", ReceivedSize);
        return KDP_PACKET_RESEND;
    }

    ExpectedSize = sizeof(*RecvPktResult) +
                   RecvPktResult->HeaderSize +
                   RecvPktResult->DataSize;
    if (ReceivedSize != ExpectedSize)
    {
        KDDBGPRINT("KdReceivePacket: Invalid size for RecvPktResult: %lu, expected %lu\n",
                   ReceivedSize, ExpectedSize);
        return KDP_PACKET_RESEND;
    }

    if (KdContext != NULL)
    {
        RtlCopyMemory(KdContext,
                      &RecvPktResult->KdContext,
                      sizeof(RecvPktResult->KdContext));
    }

    Buffer = (PUCHAR)(RecvPktResult + 1);
    if (MessageHeader != NULL)
    {
        MessageHeader->Length = RecvPktResult->MessageHeader.Length;
        if ((MessageHeader->Buffer != NULL) &&
            (MessageHeader->MaximumLength >= RecvPktResult->HeaderSize))
        {
            RtlCopyMemory(MessageHeader->Buffer,
                          Buffer,
                          RecvPktResult->HeaderSize);
        }
        else
        {
            KDDBGPRINT("MessageHeader not good\n");
        }
    }

    Buffer += RecvPktResult->HeaderSize;
    if (MessageData != NULL)
    {
        MessageData->Length = RecvPktResult->MessageData.Length;
        if ((MessageData->Buffer != NULL) &&
            (MessageData->MaximumLength >= RecvPktResult->DataSize))
        {
            RtlCopyMemory(MessageData->Buffer,
                          Buffer,
                          RecvPktResult->DataSize);
        }
        else
        {
            KDDBGPRINT("MessageData not good\n");
        }
    }

    if (DataLength != NULL)
        *DataLength = RecvPktResult->FullSize;

    KDDBGPRINT("KdReceivePacket: returning status %u\n", RecvPktResult->KdStatus);
    return RecvPktResult->KdStatus;
}

