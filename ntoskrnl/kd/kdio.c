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

/* GLOBALS *******************************************************************/

#define KdpBufferSize  (1024 * 512)
static BOOLEAN KdpLoggingEnabled = FALSE;
static BOOLEAN KdpLoggingStarting = FALSE;
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

#define KdpScreenLineLengthDefault 80
static CHAR KdpScreenLineBuffer[KdpScreenLineLengthDefault + 1] = "";
static ULONG KdpScreenLineBufferPos = 0, KdpScreenLineLength = 0;

const ULONG KdpDmesgBufferSize = 128 * 1024; // 512*1024;
PCHAR KdpDmesgBuffer = NULL;
volatile ULONG KdpDmesgCurrentPosition = 0;
volatile ULONG KdpDmesgFreeBytes = 0;
volatile ULONG KdbDmesgTotalWritten = 0;
volatile BOOLEAN KdbpIsInDmesgMode = FALSE;
static KSPIN_LOCK KdpDmesgLogSpinLock;

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
KdpPrintToLogFile(PCHAR String,
                  ULONG Length)
{
    KIRQL OldIrql;
    ULONG beg, end, num;
    BOOLEAN DoReinit = FALSE;

    if (KdpDebugBuffer == NULL) return;

    /* Acquire the printing spinlock without waiting at raised IRQL */
    OldIrql = KdbpAcquireLock(&KdpDebugLogSpinLock);

    beg = KdpCurrentPosition;
    num = KdpFreeBytes;
    if (Length < num)
        num = Length;

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
    if (OldIrql == PASSIVE_LEVEL && !KdpLoggingStarting && !KdpLoggingEnabled && ExpInitializationPhase >= 2)
    {
        DoReinit = TRUE;
    }
    KdbpReleaseLock(&KdpDebugLogSpinLock, OldIrql);

    if (DoReinit)
    {
        KdpLoggingStarting = TRUE;
        KdpDebugLogInit(NULL, 3);
    }

    /* Signal the logger thread */
    if (OldIrql <= DISPATCH_LEVEL && KdpLoggingEnabled)
        KeSetEvent(&KdpLoggerThreadEvent, IO_NO_INCREMENT, FALSE);
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
    PCHAR pch = String;
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
        pch++;
    }

    /* Release the spinlock */
    KdbpReleaseLock(&KdpSerialSpinLock, OldIrql);
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

/*
 * Screen debug logger function KdpScreenPrint() writes text strings into
 * KdpDmesgBuffer, using it as a circular buffer. KdpDmesgBuffer contents could
 * be later (re)viewed using dmesg command of kdbg. KdpScreenPrint() protects
 * KdpDmesgBuffer from simultaneous writes by use of KdpDmesgLogSpinLock.
 */
static VOID
NTAPI
KdpScreenPrint(PCHAR String,
               ULONG Length)
{
    PCHAR pch = String;
    KIRQL OldIrql;
    ULONG beg, end, num;

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

    /* Dmesg: store the string in the buffer to show it later */
    if (KdbpIsInDmesgMode)
       return;

    if (KdpDmesgBuffer == NULL)
      return;

    /* Acquire the printing spinlock without waiting at raised IRQL */
    OldIrql = KdbpAcquireLock(&KdpDmesgLogSpinLock);

    /* Invariant: always_true(KdpDmesgFreeBytes == KdpDmesgBufferSize);
     * set num to min(KdpDmesgFreeBytes, Length).
     */
    num = (Length < KdpDmesgFreeBytes) ? Length : KdpDmesgFreeBytes;
    beg = KdpDmesgCurrentPosition;
    if (num != 0)
    {
        end = (beg + num) % KdpDmesgBufferSize;
        if (end > beg)
        {
            RtlCopyMemory(KdpDmesgBuffer + beg, String, Length);
        }
        else
        {
            RtlCopyMemory(KdpDmesgBuffer + beg, String, KdpDmesgBufferSize - beg);
            RtlCopyMemory(KdpDmesgBuffer, String + (KdpDmesgBufferSize - beg), end);
        }
        KdpDmesgCurrentPosition = end;

        /* Counting the total bytes written */
        KdbDmesgTotalWritten += num;
    }

    /* Release the spinlock */
    KdbpReleaseLock(&KdpDmesgLogSpinLock, OldIrql);

    /* Optional step(?): find out a way to notify about buffer exhaustion,
     * and possibly fall into kbd to use dmesg command: user will read
     * debug strings before they will be wiped over by next writes.
     */
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
        /* Allocate a buffer for dmesg log buffer. +1 for terminating null,
         * see kdbp_cli.c:KdbpCmdDmesg()/2
         */
        KdpDmesgBuffer = ExAllocatePoolZero(NonPagedPool,
                                            KdpDmesgBufferSize + 1,
                                            TAG_KDBG);
        /* Ignore failure if KdpDmesgBuffer is NULL */
        KdpDmesgFreeBytes = KdpDmesgBufferSize;
        KdbDmesgTotalWritten = 0;

        /* Take control of the display */
        KdpScreenAcquire();

        /* Initialize spinlock */
        KeInitializeSpinLock(&KdpDmesgLogSpinLock);

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
KdSendPacket(
    _In_ ULONG PacketType,
    _In_ PSTRING MessageHeader,
    _In_opt_ PSTRING MessageData,
    _Inout_ PKD_CONTEXT Context)
{
    if (PacketType == PACKET_TYPE_KD_DEBUG_IO)
    {
        ULONG ApiNumber = ((PDBGKD_DEBUG_IO)MessageHeader->Buffer)->ApiNumber;
        PLIST_ENTRY CurrentEntry;
        PKD_DISPATCH_TABLE CurrentTable;

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

        /* Call the registered handlers */
        CurrentEntry = KdProviders.Flink;
        while (CurrentEntry != &KdProviders)
        {
            /* Get the current table */
            CurrentTable = CONTAINING_RECORD(CurrentEntry,
                                             KD_DISPATCH_TABLE,
                                             KdProvidersList);

            /* Call it */
            CurrentTable->KdpPrintRoutine(MessageData->Buffer, MessageData->Length);

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
    STRING NewLine = RTL_CONSTANT_STRING("\n");
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
    KdpPrintString(&NewLine);
    KdpPrintString(&KdbPromptString); // Alternatively, use "Input> "

    if (!(KdbDebugState & KD_DEBUG_KDSERIAL))
        KbdDisableMouse();

    /* Read a line of user input and retrieve the length.
     * The output string is NULL-terminated -- documentation states
     * that DbgPrompt() does not NULL-terminate, but it does. */
    *DataLength = KdbpReadCommand(ResponseString.Buffer,
                                  ResponseString.MaximumLength);

    if (!(KdbDebugState & KD_DEBUG_KDSERIAL))
        KbdEnableMouse();

    /* Return the length */
    *DataLength = min(*DataLength, DebugIo->u.GetString.LengthOfStringRead);
    MessageData->Length = DebugIo->u.GetString.LengthOfStringRead = *DataLength;

    /* Only now we can copy back the data into MessageData->Buffer */
    RtlCopyMemory(MessageData->Buffer, ResponseString.Buffer, *DataLength);
#endif

    return KdPacketReceived;
}

/* EOF */
