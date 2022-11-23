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

#define NDEBUG
#include <debug.h>

#undef KdDebuggerInitialize0
#undef KdDebuggerInitialize1
#undef KdSendPacket
#undef KdReceivePacket

/* GLOBALS *******************************************************************/

#define KdpBufferSize  (1024 * 512)
static PCHAR KdpDebugBuffer = NULL;
static volatile ULONG KdpCurrentPosition = 0;
static volatile ULONG KdpFreeBytes = 0;
static KSPIN_LOCK KdpDebugLogSpinLock;
static KEVENT KdpLoggerThreadEvent;
static HANDLE KdpLogFileHandle;
ANSI_STRING KdpLogFileName = RTL_CONSTANT_STRING("\\SystemRoot\\debug.log");
extern ULONG ExpInitializationPhase;

static KSPIN_LOCK KdpSerialSpinLock;
ULONG  SerialPortNumber = DEFAULT_DEBUG_PORT;
CPPORT SerialPortInfo   = {0, DEFAULT_DEBUG_BAUD_RATE, 0};

KDP_DEBUG_MODE KdpDebugMode;
LIST_ENTRY KdProviders = {&KdProviders, &KdProviders};
KD_DISPATCH_TABLE DispatchTable[KdMax];

PKDP_INIT_ROUTINE InitRoutines[KdMax] =
{
    KdpScreenInit,
    KdpSerialInit,
    KdpDebugLogInit,
#ifdef KDBG
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
KdpAcquireLock(IN PKSPIN_LOCK SpinLock)
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
KdpReleaseLock(IN PKSPIN_LOCK SpinLock,
               IN KIRQL OldIrql)
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
KdpPrintToLogFile(PCHAR String,
                  ULONG Length)
{
}

VOID
NTAPI
KdpDebugLogInit(PKD_DISPATCH_TABLE DispatchTable,
                ULONG BootPhase)
{
    NTSTATUS Status;
    UNICODE_STRING FileName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK Iosb;
    HANDLE ThreadHandle;
    KPRIORITY Priority;

    if (!KdpDebugMode.File) return;

    if (BootPhase == 0)
    {
        /* Write out the functions that we support for now */
        DispatchTable->KdpInitRoutine = KdpDebugLogInit;
        DispatchTable->KdpPrintRoutine = KdpPrintToLogFile;

        /* Register as a Provider */
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
            return;
        }
        KdpFreeBytes = KdpBufferSize;

        /* Initialize spinlock */
        KeInitializeSpinLock(&KdpDebugLogSpinLock);

        HalDisplayString("\r\n   File log debugging enabled\r\n\r\n");
    }
    else if (BootPhase == 3)
    {
        /* Setup the log name */
        Status = RtlAnsiStringToUnicodeString(&FileName, &KdpLogFileName, TRUE);
        if (!NT_SUCCESS(Status)) return;

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
                              FILE_SUPERSEDE,
                              FILE_WRITE_THROUGH | FILE_SYNCHRONOUS_IO_NONALERT,
                              NULL,
                              0);

        RtlFreeUnicodeString(&FileName);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to open log file: 0x%08x\n", Status);
            return;
        }

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
            ZwClose(KdpLogFileHandle);
            return;
        }

        Priority = 7;
        ZwSetInformationThread(ThreadHandle,
                               ThreadPriority,
                               &Priority,
                               sizeof(Priority));

        ZwClose(ThreadHandle);
    }
}

/* SERIAL FUNCTIONS **********************************************************/

static VOID
NTAPI
KdpSerialPrint(PCHAR String,
               ULONG Length)
{
}

VOID
NTAPI
KdpSerialInit(PKD_DISPATCH_TABLE DispatchTable,
              ULONG BootPhase)
{
    if (!KdpDebugMode.Serial) return;

    if (BootPhase == 0)
    {
        /* Write out the functions that we support for now */
        DispatchTable->KdpInitRoutine = KdpSerialInit;
        DispatchTable->KdpPrintRoutine = KdpSerialPrint;

        /* Initialize the Port */
        if (!KdPortInitializeEx(&SerialPortInfo, SerialPortNumber))
        {
            KdpDebugMode.Serial = FALSE;
            return;
        }
        KdComPortInUse = SerialPortInfo.Address;

        /* Initialize spinlock */
        KeInitializeSpinLock(&KdpSerialSpinLock);

        /* Register as a Provider */
        InsertTailList(&KdProviders, &DispatchTable->KdProvidersList);
    }
    else if (BootPhase == 1)
    {
        HalDisplayString("\r\n   Serial debugging enabled\r\n\r\n");
    }
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
KdpScreenPrint(PCHAR String,
               ULONG Length)
{
}

VOID
NTAPI
KdpScreenInit(PKD_DISPATCH_TABLE DispatchTable,
              ULONG BootPhase)
{
    if (!KdpDebugMode.Screen) return;

    if (BootPhase == 0)
    {
        /* Write out the functions that we support for now */
        DispatchTable->KdpInitRoutine = KdpScreenInit;
        DispatchTable->KdpPrintRoutine = KdpScreenPrint;

        /* Register as a Provider */
        InsertTailList(&KdProviders, &DispatchTable->KdProvidersList);
    }
    else if (BootPhase == 1)
    {
        /* Take control of the display */
        KdpScreenAcquire();

        HalDisplayString("\r\n   Screen debugging enabled\r\n\r\n");
    }
}

#ifdef KDBG
/* KDBG FUNCTIONS ************************************************************/

/* NOTE: This may be moved completely into kdb_symbols.c */
VOID NTAPI
KdbInitialize(PKD_DISPATCH_TABLE DispatchTable, ULONG BootPhase);

VOID
NTAPI
KdpKdbgInit(
    PKD_DISPATCH_TABLE DispatchTable,
    ULONG BootPhase)
{
    /* Forward the call */
    KdbInitialize(DispatchTable, BootPhase);
}
#endif


/* GENERAL FUNCTIONS *********************************************************/

BOOLEAN
NTAPI
KdpPrintString(
    _In_ PSTRING Output);

extern STRING KdbPromptString;

VOID
NTAPI
KdpSendPacket(
    IN ULONG PacketType,
    IN PSTRING MessageHeader,
    IN PSTRING MessageData,
    IN OUT PKD_CONTEXT Context)
{
    KdSendPacket(PacketType, MessageHeader, MessageData, Context);

    if (PacketType == PACKET_TYPE_KD_DEBUG_IO)
    {
        PSTRING Output = MessageData;
        PLIST_ENTRY CurrentEntry;
        PKD_DISPATCH_TABLE CurrentTable;

        if (!KdpDebugMode.Value) return;

        /* Call the registered handlers */
        CurrentEntry = KdProviders.Flink;
        while (CurrentEntry != &KdProviders)
        {
            /* Get the current table */
            CurrentTable = CONTAINING_RECORD(CurrentEntry,
                                             KD_DISPATCH_TABLE,
                                             KdProvidersList);

            /* Call it */
            CurrentTable->KdpPrintRoutine(Output->Buffer, Output->Length);

            /* Next Table */
            CurrentEntry = CurrentEntry->Flink;
        }
        return;
    }
    else if (PacketType == PACKET_TYPE_KD_STATE_CHANGE64)
    {
        PDBGKD_ANY_WAIT_STATE_CHANGE WaitStateChange = (PDBGKD_ANY_WAIT_STATE_CHANGE)MessageHeader->Buffer;
        if (WaitStateChange->NewState == DbgKdLoadSymbolsStateChange)
        {
            PLDR_DATA_TABLE_ENTRY LdrEntry;
            /* Load symbols. Currently implemented only for KDBG! */
            if (KdbpSymFindModule((PVOID)(ULONG_PTR)WaitStateChange->u.LoadSymbols.BaseOfDll, -1, &LdrEntry))
            {
                KdbSymProcessSymbols(LdrEntry, !WaitStateChange->u.LoadSymbols.UnloadSymbols);
            }
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
KdpReceivePacket(
    IN ULONG PacketType,
    OUT PSTRING MessageHeader,
    OUT PSTRING MessageData,
    OUT PULONG DataLength,
    IN OUT PKD_CONTEXT Context)
{
    KIRQL OldIrql;
    STRING StringChar;
    CHAR Response;
    USHORT i;
    ULONG DummyScanCode;
    CHAR MessageBuffer[100];
    STRING ResponseString;
    KDSTATUS Status;

    Status = KdReceivePacket(PacketType, MessageHeader, MessageData, DataLength, Context);
    if (Status != KdPacketTimedOut)
        return Status;

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

    ResponseString.Buffer = MessageBuffer;
    ResponseString.Length = 0;
    ResponseString.MaximumLength = min(sizeof(MessageBuffer), MessageData->MaximumLength);
    StringChar.Buffer = &Response;
    StringChar.Length = StringChar.MaximumLength = sizeof(Response);

    /* Display the string and print a new line for log neatness */
    *StringChar.Buffer = '\n';
    KdpPrintString(&StringChar);

    /* Print the kdb prompt */
    KdpPrintString(&KdbPromptString);

    // TODO: Use an improved KdbpReadCommand() function for our purposes.

    /* Acquire the printing spinlock without waiting at raised IRQL */
    OldIrql = KdpAcquireLock(&KdpSerialSpinLock);

    if (!(KdbDebugState & KD_DEBUG_KDSERIAL))
        KbdDisableMouse();

    /* Loop the whole string */
    for (i = 0; i < ResponseString.MaximumLength; i++)
    {
        /* Check if this is serial debugging mode */
        if (KdbDebugState & KD_DEBUG_KDSERIAL)
        {
            /* Get the character from serial */
            do
            {
                Response = KdbpTryGetCharSerial(MAXULONG);
            } while (Response == -1);
        }
        else
        {
            /* Get the response from the keyboard */
            do
            {
                Response = KdbpTryGetCharKeyboard(&DummyScanCode, MAXULONG);
            } while (Response == -1);
        }

        /* Check for return */
        if (Response == '\r')
        {
            /*
             * We might need to discard the next '\n'.
             * Wait a bit to make sure we receive it.
             */
            KeStallExecutionProcessor(100000);

            /* Check the mode */
            if (KdbDebugState & KD_DEBUG_KDSERIAL)
            {
                /* Read and discard the next character, if any */
                KdbpTryGetCharSerial(5);
            }
            else
            {
                /* Read and discard the next character, if any */
                KdbpTryGetCharKeyboard(&DummyScanCode, 5);
            }

            /*
             * Null terminate the output string -- documentation states that
             * DbgPrompt does not null terminate, but it does
             */
            *(PCHAR)(ResponseString.Buffer + i) = 0;
            break;
        }

        /* Write it back and print it to the log */
        *(PCHAR)(ResponseString.Buffer + i) = Response;
        KdpReleaseLock(&KdpSerialSpinLock, OldIrql);
        KdpPrintString(&StringChar);
        OldIrql = KdpAcquireLock(&KdpSerialSpinLock);
    }

    /* Release the spinlock */
    KdpReleaseLock(&KdpSerialSpinLock, OldIrql);

    /* Print a new line */
    *StringChar.Buffer = '\n';
    KdpPrintString(&StringChar);

    /* Return the length */
    RtlCopyMemory(MessageData->Buffer, ResponseString.Buffer, i);
    *DataLength = i;

    if (!(KdbDebugState & KD_DEBUG_KDSERIAL))
        KbdEnableMouse();

    return KdPacketReceived;
}

/* EOF */
