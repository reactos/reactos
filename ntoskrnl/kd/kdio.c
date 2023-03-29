/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/kd/kdio.c
 * PURPOSE:         NT Kernel Debugger Input/Output Functions
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include <reactos/buildno.h>
#include "kd.h"
#include "kdterminal.h"
#ifdef KDBG
#include "../kdbg/kdb.h"
#endif

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

#define KdpBufferSize  (1024 * 512)
static BOOLEAN KdpLoggingEnabled = FALSE;
static PCHAR KdpDebugBuffer = NULL;
static volatile ULONG KdpCurrentPosition = 0;
static volatile ULONG KdpFreeBytes = 0;
static KSPIN_LOCK KdpDebugLogSpinLock;
static KEVENT KdpLoggerThreadEvent;
static HANDLE KdpLogFileHandle;
ANSI_STRING KdpLogFileName = RTL_CONSTANT_STRING("\\SystemRoot\\debug.log");

static KSPIN_LOCK KdpSerialSpinLock;
ULONG  SerialPortNumber = DEFAULT_DEBUG_PORT;
CPPORT SerialPortInfo   = {0, DEFAULT_DEBUG_BAUD_RATE, 0};

#define KdpScreenLineLengthDefault 80
static CHAR KdpScreenLineBuffer[KdpScreenLineLengthDefault + 1] = "";
static ULONG KdpScreenLineBufferPos = 0, KdpScreenLineLength = 0;

KDP_DEBUG_MODE KdpDebugMode;
LIST_ENTRY KdProviders = {&KdProviders, &KdProviders};
KD_DISPATCH_TABLE DispatchTable[KdMax] = {0};

PKDP_INIT_ROUTINE InitRoutines[KdMax] =
{
    KdpScreenInit,
    KdpSerialInit,
    KdpDebugLogInit,
#ifdef KDBG // See kdb_cli.c
    KdpKdbgInit
#endif
};

static ULONG KdbgNextApiNumber = DbgKdContinueApi;
static CONTEXT KdbgContext;
static EXCEPTION_RECORD64 KdbgExceptionRecord;
static BOOLEAN KdbgFirstChanceException;
static NTSTATUS KdbgContinueStatus = STATUS_SUCCESS;

/* LOCKING FUNCTIONS *********************************************************/

KIRQL
NTAPI
KdbpAcquireLock(
    _In_ PKSPIN_LOCK SpinLock)
{
    KIRQL OldIrql;

    /* Acquire the spinlock without waiting at raised IRQL */
    while (TRUE)
    {
        /* Loop until the spinlock becomes available */
        while (!KeTestSpinLock(SpinLock));

        /* Spinlock is free, raise IRQL to high level */
        KeRaiseIrql(HIGH_LEVEL, &OldIrql);

        /* Try to get the spinlock */
        if (KeTryToAcquireSpinLockAtDpcLevel(SpinLock))
            break;

        /* Someone else got the spinlock, lower IRQL back */
        KeLowerIrql(OldIrql);
    }

    return OldIrql;
}

VOID
NTAPI
KdbpReleaseLock(
    _In_ PKSPIN_LOCK SpinLock,
    _In_ KIRQL OldIrql)
{
    /* Release the spinlock */
    KiReleaseSpinLock(SpinLock);
    // KeReleaseSpinLockFromDpcLevel(SpinLock);

    /* Restore the old IRQL */
    KeLowerIrql(OldIrql);
}

/* FILE DEBUG LOG FUNCTIONS **************************************************/

static VOID
NTAPI
KdpLoggerThread(PVOID Context)
{
    ULONG beg, end, num;
    IO_STATUS_BLOCK Iosb;

    ASSERT(ExGetPreviousMode() == KernelMode);

    KdpLoggingEnabled = TRUE;

    while (TRUE)
    {
        KeWaitForSingleObject(&KdpLoggerThreadEvent, Executive, KernelMode, FALSE, NULL);

        /* Bug */
        /* Keep KdpCurrentPosition and KdpFreeBytes values in local
         * variables to avoid their possible change from Producer part,
         * KdpPrintToLogFile function
         */
        end = KdpCurrentPosition;
        num = KdpFreeBytes;

        /* Now securely calculate values, based on local variables */
        beg = (end + num) % KdpBufferSize;
        num = KdpBufferSize - num;

        /* Nothing to do? */
        if (num == 0)
            continue;

        if (end > beg)
        {
            NtWriteFile(KdpLogFileHandle, NULL, NULL, NULL, &Iosb,
                        KdpDebugBuffer + beg, num, NULL, NULL);
        }
        else
        {
            NtWriteFile(KdpLogFileHandle, NULL, NULL, NULL, &Iosb,
                        KdpDebugBuffer + beg, KdpBufferSize - beg, NULL, NULL);

            NtWriteFile(KdpLogFileHandle, NULL, NULL, NULL, &Iosb,
                        KdpDebugBuffer, end, NULL, NULL);
        }

        (VOID)InterlockedExchangeAddUL(&KdpFreeBytes, num);
    }
}

static VOID
NTAPI
KdpPrintToLogFile(
    _In_ PCCH String,
    _In_ ULONG Length)
{
    KIRQL OldIrql;
    ULONG beg, end, num;

    if (KdpDebugBuffer == NULL) return;

    /* Acquire the printing spinlock without waiting at raised IRQL */
    OldIrql = KdbpAcquireLock(&KdpDebugLogSpinLock);

    beg = KdpCurrentPosition;
    num = min(Length, KdpFreeBytes);
    if (num != 0)
    {
        end = (beg + num) % KdpBufferSize;
        KdpCurrentPosition = end;
        KdpFreeBytes -= num;

        if (end > beg)
        {
            RtlCopyMemory(KdpDebugBuffer + beg, String, num);
        }
        else
        {
            RtlCopyMemory(KdpDebugBuffer + beg, String, KdpBufferSize - beg);
            RtlCopyMemory(KdpDebugBuffer, String + KdpBufferSize - beg, end);
        }
    }

    /* Release the spinlock */
    KdbpReleaseLock(&KdpDebugLogSpinLock, OldIrql);

    /* Signal the logger thread */
    if (OldIrql <= DISPATCH_LEVEL && KdpLoggingEnabled)
        KeSetEvent(&KdpLoggerThreadEvent, IO_NO_INCREMENT, FALSE);
}

NTSTATUS
NTAPI
KdpDebugLogInit(
    _In_ PKD_DISPATCH_TABLE DispatchTable,
    _In_ ULONG BootPhase)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (!KdpDebugMode.File)
        return STATUS_PORT_DISCONNECTED;

    if (BootPhase == 0)
    {
        /* Write out the functions that we support for now */
        DispatchTable->KdpPrintRoutine = KdpPrintToLogFile;

        /* Register for BootPhase 1 initialization and as a Provider */
        DispatchTable->KdpInitRoutine = KdpDebugLogInit;
        InsertTailList(&KdProviders, &DispatchTable->KdProvidersList);
    }
    else if (BootPhase == 1)
    {
        /* Allocate a buffer for debug log */
        KdpDebugBuffer = ExAllocatePoolZero(NonPagedPool,
                                            KdpBufferSize,
                                            TAG_KDBG);
        if (!KdpDebugBuffer)
        {
            KdpDebugMode.File = FALSE;
            RemoveEntryList(&DispatchTable->KdProvidersList);
            return STATUS_NO_MEMORY;
        }
        KdpFreeBytes = KdpBufferSize;

        /* Initialize spinlock */
        KeInitializeSpinLock(&KdpDebugLogSpinLock);

        /* Register for later BootPhase 2 reinitialization */
        DispatchTable->KdpInitRoutine = KdpDebugLogInit;

        /* Announce ourselves */
        HalDisplayString("   File log debugging enabled\r\n");
    }
    else if (BootPhase >= 2)
    {
        UNICODE_STRING FileName;
        OBJECT_ATTRIBUTES ObjectAttributes;
        IO_STATUS_BLOCK Iosb;
        HANDLE ThreadHandle;
        KPRIORITY Priority;

        /* If we have already successfully opened the log file, bail out */
        if (KdpLogFileHandle != NULL)
            return STATUS_SUCCESS;

        /* Setup the log name */
        Status = RtlAnsiStringToUnicodeString(&FileName, &KdpLogFileName, TRUE);
        if (!NT_SUCCESS(Status))
            goto Failure;

        InitializeObjectAttributes(&ObjectAttributes,
                                   &FileName,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   NULL,
                                   NULL);

        /* Create the log file */
        Status = ZwCreateFile(&KdpLogFileHandle,
                              FILE_APPEND_DATA | SYNCHRONIZE,
                              &ObjectAttributes,
                              &Iosb,
                              NULL,
                              FILE_ATTRIBUTE_NORMAL,
                              FILE_SHARE_READ,
                              FILE_OPEN_IF,
                              FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT |
                                FILE_SEQUENTIAL_ONLY | FILE_WRITE_THROUGH,
                              NULL,
                              0);

        RtlFreeUnicodeString(&FileName);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to open log file: 0x%08lx\n", Status);

            /* Schedule an I/O reinitialization if needed */
            if (Status == STATUS_OBJECT_NAME_NOT_FOUND ||
                Status == STATUS_OBJECT_PATH_NOT_FOUND)
            {
                DispatchTable->KdpInitRoutine = KdpDebugLogInit;
                return Status;
            }
            goto Failure;
        }

        /**    HACK for FILE_APPEND_DATA     **
         ** Remove once CORE-18789 is fixed. **
         ** Enforce to go to the end of file **/
        {
            FILE_STANDARD_INFORMATION FileInfo;
            FILE_POSITION_INFORMATION FilePosInfo;

            Status = ZwQueryInformationFile(KdpLogFileHandle,
                                            &Iosb,
                                            &FileInfo,
                                            sizeof(FileInfo),
                                            FileStandardInformation);
            DPRINT("Status: 0x%08lx - EOF offset: %I64d\n",
                    Status, FileInfo.EndOfFile.QuadPart);

            Status = ZwQueryInformationFile(KdpLogFileHandle,
                                            &Iosb,
                                            &FilePosInfo,
                                            sizeof(FilePosInfo),
                                            FilePositionInformation);
            DPRINT("Status: 0x%08lx - Position: %I64d\n",
                    Status, FilePosInfo.CurrentByteOffset.QuadPart);

            FilePosInfo.CurrentByteOffset.QuadPart = FileInfo.EndOfFile.QuadPart;
            Status = ZwSetInformationFile(KdpLogFileHandle,
                                          &Iosb,
                                          &FilePosInfo,
                                          sizeof(FilePosInfo),
                                          FilePositionInformation);
            DPRINT("ZwSetInformationFile(FilePositionInfo) returned: 0x%08lx\n", Status);
        }
        /** END OF HACK **/

        KeInitializeEvent(&KdpLoggerThreadEvent, SynchronizationEvent, TRUE);

        /* Create the logger thread */
        Status = PsCreateSystemThread(&ThreadHandle,
                                      THREAD_ALL_ACCESS,
                                      NULL,
                                      NULL,
                                      NULL,
                                      KdpLoggerThread,
                                      NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to create log file thread: 0x%08lx\n", Status);
            ZwClose(KdpLogFileHandle);
            goto Failure;
        }

        Priority = HIGH_PRIORITY;
        ZwSetInformationThread(ThreadHandle,
                               ThreadPriority,
                               &Priority,
                               sizeof(Priority));

        ZwClose(ThreadHandle);
        return Status;

Failure:
        KdpFreeBytes = 0;
        ExFreePoolWithTag(KdpDebugBuffer, TAG_KDBG);
        KdpDebugBuffer = NULL;
        KdpDebugMode.File = FALSE;
        RemoveEntryList(&DispatchTable->KdProvidersList);
    }

    return Status;
}

/* SERIAL FUNCTIONS **********************************************************/

static VOID
NTAPI
KdpSerialPrint(
    _In_ PCCH String,
    _In_ ULONG Length)
{
    PCCH pch = String;
    KIRQL OldIrql;

    /* Acquire the printing spinlock without waiting at raised IRQL */
    OldIrql = KdbpAcquireLock(&KdpSerialSpinLock);

    /* Output the string */
    while (pch < String + Length && *pch)
    {
        if (*pch == '\n')
        {
            KdPortPutByteEx(&SerialPortInfo, '\r');
        }
        KdPortPutByteEx(&SerialPortInfo, *pch);
        ++pch;
    }

    /* Release the spinlock */
    KdbpReleaseLock(&KdpSerialSpinLock, OldIrql);
}

NTSTATUS
NTAPI
KdpSerialInit(
    _In_ PKD_DISPATCH_TABLE DispatchTable,
    _In_ ULONG BootPhase)
{
    if (!KdpDebugMode.Serial)
        return STATUS_PORT_DISCONNECTED;

    if (BootPhase == 0)
    {
        /* Write out the functions that we support for now */
        DispatchTable->KdpPrintRoutine = KdpSerialPrint;

        /* Initialize the Port */
        if (!KdPortInitializeEx(&SerialPortInfo, SerialPortNumber))
        {
            KdpDebugMode.Serial = FALSE;
            return STATUS_DEVICE_DOES_NOT_EXIST;
        }
        KdComPortInUse = SerialPortInfo.Address;

        /* Initialize spinlock */
        KeInitializeSpinLock(&KdpSerialSpinLock);

        /* Register for BootPhase 1 initialization and as a Provider */
        DispatchTable->KdpInitRoutine = KdpSerialInit;
        InsertTailList(&KdProviders, &DispatchTable->KdProvidersList);
    }
    else if (BootPhase == 1)
    {
        /* Announce ourselves */
        HalDisplayString("   Serial debugging enabled\r\n");
    }

    return STATUS_SUCCESS;
}

/* SCREEN FUNCTIONS **********************************************************/

VOID
KdpScreenAcquire(VOID)
{
    if (InbvIsBootDriverInstalled() /* &&
        !InbvCheckDisplayOwnership() */)
    {
        /* Acquire ownership and reset the display */
        InbvAcquireDisplayOwnership();
        InbvResetDisplay();
        InbvSolidColorFill(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, BV_COLOR_BLACK);
        InbvSetTextColor(BV_COLOR_WHITE);
        InbvInstallDisplayStringFilter(NULL);
        InbvEnableDisplayString(TRUE);
        InbvSetScrollRegion(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
    }
}

// extern VOID NTAPI InbvSetDisplayOwnership(IN BOOLEAN DisplayOwned);

VOID
KdpScreenRelease(VOID)
{
    if (InbvIsBootDriverInstalled()&&
        InbvCheckDisplayOwnership())
    {
        /* Release the display */
        // InbvSetDisplayOwnership(FALSE);
        InbvNotifyDisplayOwnershipLost(NULL);
    }
}

static VOID
NTAPI
KdpScreenPrint(
    _In_ PCCH String,
    _In_ ULONG Length)
{
    PCCH pch = String;

    while (pch < String + Length && *pch)
    {
        if (*pch == '\b')
        {
            /* HalDisplayString does not support '\b'. Workaround it and use '\r' */
            if (KdpScreenLineLength > 0)
            {
                /* Remove last character from buffer */
                KdpScreenLineBuffer[--KdpScreenLineLength] = '\0';
                KdpScreenLineBufferPos = KdpScreenLineLength;

                /* Clear row and print line again */
                HalDisplayString("\r");
                HalDisplayString(KdpScreenLineBuffer);
            }
        }
        else
        {
            KdpScreenLineBuffer[KdpScreenLineLength++] = *pch;
            KdpScreenLineBuffer[KdpScreenLineLength] = '\0';
        }

        if (*pch == '\n' || KdpScreenLineLength == KdpScreenLineLengthDefault)
        {
            /* Print buffered characters */
            if (KdpScreenLineBufferPos != KdpScreenLineLength)
                HalDisplayString(KdpScreenLineBuffer + KdpScreenLineBufferPos);

            /* Clear line buffer */
            KdpScreenLineBuffer[0] = '\0';
            KdpScreenLineLength = KdpScreenLineBufferPos = 0;
        }

        ++pch;
    }

    /* Print buffered characters */
    if (KdpScreenLineBufferPos != KdpScreenLineLength)
    {
        HalDisplayString(KdpScreenLineBuffer + KdpScreenLineBufferPos);
        KdpScreenLineBufferPos = KdpScreenLineLength;
    }
}

NTSTATUS
NTAPI
KdpScreenInit(
    _In_ PKD_DISPATCH_TABLE DispatchTable,
    _In_ ULONG BootPhase)
{
    if (!KdpDebugMode.Screen)
        return STATUS_PORT_DISCONNECTED;

    if (BootPhase == 0)
    {
        /* Write out the functions that we support for now */
        DispatchTable->KdpPrintRoutine = KdpScreenPrint;

        /* Register for BootPhase 1 initialization and as a Provider */
        DispatchTable->KdpInitRoutine = KdpScreenInit;
        InsertTailList(&KdProviders, &DispatchTable->KdProvidersList);
    }
    else if (BootPhase == 1)
    {
        /* Take control of the display */
        KdpScreenAcquire();

        /* Announce ourselves */
        HalDisplayString("   Screen debugging enabled\r\n");
    }

    return STATUS_SUCCESS;
}


/* GENERAL FUNCTIONS *********************************************************/

static VOID
KdIoPrintString(
    _In_ PCCH String,
    _In_ ULONG Length)
{
    PLIST_ENTRY CurrentEntry;
    PKD_DISPATCH_TABLE CurrentTable;

    /* Call the registered providers */
    for (CurrentEntry = KdProviders.Flink;
         CurrentEntry != &KdProviders;
         CurrentEntry = CurrentEntry->Flink)
    {
        CurrentTable = CONTAINING_RECORD(CurrentEntry,
                                         KD_DISPATCH_TABLE,
                                         KdProvidersList);

        CurrentTable->KdpPrintRoutine(String, Length);
    }
}

VOID
KdIoPuts(
    _In_ PCSTR String)
{
    KdIoPrintString(String, (ULONG)strlen(String));
}

VOID
__cdecl
KdIoPrintf(
    _In_ PCSTR Format,
    ...)
{
    va_list ap;
    ULONG Length;
    CHAR Buffer[512];

    /* Format the string */
    va_start(ap, Format);
    Length = (ULONG)_vsnprintf(Buffer,
                               sizeof(Buffer),
                               Format,
                               ap);
    va_end(ap);

    /* Send it to the display providers */
    KdIoPrintString(Buffer, Length);
}


extern const CSTRING KdbPromptStr;

VOID
NTAPI
KdSendPacket(
    _In_ ULONG PacketType,
    _In_ PSTRING MessageHeader,
    _In_opt_ PSTRING MessageData,
    _Inout_ PKD_CONTEXT Context)
{
    if (PacketType == PACKET_TYPE_KD_DEBUG_IO)
    {
        ULONG ApiNumber = ((PDBGKD_DEBUG_IO)MessageHeader->Buffer)->ApiNumber;

        /* Validate API call */
        if (MessageHeader->Length != sizeof(DBGKD_DEBUG_IO))
            return;
        if ((ApiNumber != DbgKdPrintStringApi) &&
            (ApiNumber != DbgKdGetStringApi))
        {
            return;
        }
        if (!MessageData)
            return;

        /* NOTE: MessageData->Length should be equal to
         * DebugIo.u.PrintString.LengthOfString, or to
         * DebugIo.u.GetString.LengthOfPromptString */

        if (!KdpDebugMode.Value)
            return;

        /* Print the string proper */
        KdIoPrintString(MessageData->Buffer, MessageData->Length);
        return;
    }
    else if (PacketType == PACKET_TYPE_KD_STATE_CHANGE64)
    {
        PDBGKD_ANY_WAIT_STATE_CHANGE WaitStateChange = (PDBGKD_ANY_WAIT_STATE_CHANGE)MessageHeader->Buffer;
        if (WaitStateChange->NewState == DbgKdLoadSymbolsStateChange)
        {
#ifdef KDBG
            PLDR_DATA_TABLE_ENTRY LdrEntry;
            /* Load symbols. Currently implemented only for KDBG! */
            if (KdbpSymFindModule((PVOID)(ULONG_PTR)WaitStateChange->u.LoadSymbols.BaseOfDll, -1, &LdrEntry))
            {
                KdbSymProcessSymbols(LdrEntry, !WaitStateChange->u.LoadSymbols.UnloadSymbols);
            }
#endif
            return;
        }
        else if (WaitStateChange->NewState == DbgKdExceptionStateChange)
        {
            KdbgNextApiNumber = DbgKdGetContextApi;
            KdbgExceptionRecord = WaitStateChange->u.Exception.ExceptionRecord;
            KdbgFirstChanceException = WaitStateChange->u.Exception.FirstChance;
            return;
        }
    }
    else if (PacketType == PACKET_TYPE_KD_STATE_MANIPULATE)
    {
        PDBGKD_MANIPULATE_STATE64 ManipulateState = (PDBGKD_MANIPULATE_STATE64)MessageHeader->Buffer;
        if (ManipulateState->ApiNumber == DbgKdGetContextApi)
        {
            KD_CONTINUE_TYPE Result;

#ifdef KDBG
            /* Check if this is an assertion failure */
            if (KdbgExceptionRecord.ExceptionCode == STATUS_ASSERTION_FAILURE)
            {
                /* Bump EIP to the instruction following the int 2C */
                KeSetContextPc(&KdbgContext, KeGetContextPc(&KdbgContext) + 2);
            }

            Result = KdbEnterDebuggerException(&KdbgExceptionRecord,
                                               KdbgContext.SegCs & 1,
                                               &KdbgContext,
                                               KdbgFirstChanceException);
#else
            /* We'll manually dump the stack for the user... */
            KeRosDumpStackFrames(NULL, 0);
            Result = kdHandleException;
#endif
            if (Result != kdHandleException)
                KdbgContinueStatus = STATUS_SUCCESS;
            else
                KdbgContinueStatus = STATUS_UNSUCCESSFUL;
            KdbgNextApiNumber = DbgKdSetContextApi;
            return;
        }
        else if (ManipulateState->ApiNumber == DbgKdSetContextApi)
        {
            KdbgNextApiNumber = DbgKdContinueApi;
            return;
        }
    }
    UNIMPLEMENTED;
}

KDSTATUS
NTAPI
KdReceivePacket(
    _In_ ULONG PacketType,
    _Out_ PSTRING MessageHeader,
    _Out_ PSTRING MessageData,
    _Out_ PULONG DataLength,
    _Inout_ PKD_CONTEXT Context)
{
#ifdef KDBG
    STRING ResponseString;
    PDBGKD_DEBUG_IO DebugIo;
    CHAR MessageBuffer[512];
#endif

    if (PacketType == PACKET_TYPE_KD_STATE_MANIPULATE)
    {
        PDBGKD_MANIPULATE_STATE64 ManipulateState = (PDBGKD_MANIPULATE_STATE64)MessageHeader->Buffer;
        RtlZeroMemory(MessageHeader->Buffer, MessageHeader->MaximumLength);
        if (KdbgNextApiNumber == DbgKdGetContextApi)
        {
            ManipulateState->ApiNumber = DbgKdGetContextApi;
            MessageData->Length = 0;
            MessageData->Buffer = (PCHAR)&KdbgContext;
            return KdPacketReceived;
        }
        else if (KdbgNextApiNumber == DbgKdSetContextApi)
        {
            ManipulateState->ApiNumber = DbgKdSetContextApi;
            MessageData->Length = sizeof(KdbgContext);
            MessageData->Buffer = (PCHAR)&KdbgContext;
            return KdPacketReceived;
        }
        else if (KdbgNextApiNumber != DbgKdContinueApi)
        {
            UNIMPLEMENTED;
        }
        ManipulateState->ApiNumber = DbgKdContinueApi;
        ManipulateState->u.Continue.ContinueStatus = KdbgContinueStatus;

        /* Prepare for next time */
        KdbgNextApiNumber = DbgKdContinueApi;
        KdbgContinueStatus = STATUS_SUCCESS;

        return KdPacketReceived;
    }

    if (PacketType != PACKET_TYPE_KD_DEBUG_IO)
        return KdPacketTimedOut;

#ifdef KDBG
    DebugIo = (PDBGKD_DEBUG_IO)MessageHeader->Buffer;

    /* Validate API call */
    if (MessageHeader->MaximumLength != sizeof(DBGKD_DEBUG_IO))
        return KdPacketNeedsResend;
    if (DebugIo->ApiNumber != DbgKdGetStringApi)
        return KdPacketNeedsResend;

    /* NOTE: We cannot use directly MessageData->Buffer here as it points
     * to the temporary KdpMessageBuffer scratch buffer that is being
     * shared with all the possible I/O KD operations that may happen. */
    ResponseString.Buffer = MessageBuffer;
    ResponseString.Length = 0;
    ResponseString.MaximumLength = min(sizeof(MessageBuffer),
                                       MessageData->MaximumLength);
    ResponseString.MaximumLength = min(ResponseString.MaximumLength,
                                       DebugIo->u.GetString.LengthOfStringRead);

    /* The prompt string has been printed by KdSendPacket; go to
     * new line and print the kdb prompt -- for SYSREG2 support. */
    KdIoPrintString("\n", 1);
    KdIoPuts(KdbPromptStr.Buffer); // Alternatively, use "Input> "

    if (!(KdbDebugState & KD_DEBUG_KDSERIAL))
        KbdDisableMouse();

    /*
     * Read a NULL-terminated line of user input and retrieve its length.
     * Official documentation states that DbgPrompt() includes a terminating
     * newline character but does not NULL-terminate. However, experiments
     * show that this behaviour is left at the discretion of WinDbg itself.
     * WinDbg NULL-terminates the string unless its buffer is too short,
     * in which case the string is simply truncated without NULL-termination.
     */
    ResponseString.Length =
        (USHORT)KdIoReadLine(ResponseString.Buffer,
                             ResponseString.MaximumLength);

    if (!(KdbDebugState & KD_DEBUG_KDSERIAL))
        KbdEnableMouse();

    /* Adjust and return the string length */
    *DataLength = min(ResponseString.Length + sizeof(ANSI_NULL),
                      DebugIo->u.GetString.LengthOfStringRead);
    MessageData->Length = DebugIo->u.GetString.LengthOfStringRead = *DataLength;

    /* Only now we can copy back the data into MessageData->Buffer */
    RtlCopyMemory(MessageData->Buffer, ResponseString.Buffer, *DataLength);
#endif

    return KdPacketReceived;
}

/* EOF */
