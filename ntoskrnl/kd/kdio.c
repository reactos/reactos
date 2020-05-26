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

/* Current Port in use. FIXME: Do we support more than one? */
ULONG KdpPort;

#define KdpScreenLineLengthDefault 80
static CHAR KdpScreenLineBuffer[KdpScreenLineLengthDefault + 1] = "";
static ULONG KdpScreenLineBufferPos = 0, KdpScreenLineLength = 0;

const ULONG KdpDmesgBufferSize = 128 * 1024; // 512*1024; // 5*1024*1024;
PCHAR KdpDmesgBuffer = NULL;
volatile ULONG KdpDmesgCurrentPosition = 0;
volatile ULONG KdpDmesgFreeBytes = 0;
volatile ULONG KdbDmesgTotalWritten = 0;
volatile BOOLEAN KdbpIsInDmesgMode = FALSE;
static KSPIN_LOCK KdpDmesgLogSpinLock;

/* UTILITY FUNCTIONS *********************************************************/

/*
 * Get the total size of the memory before
 * Mm is initialized, by counting the number
 * of physical pages. Useful for debug logging.
 *
 * Strongly inspired by:
 * mm\ARM3\mminit.c : MiScanMemoryDescriptors(...)
 *
 * See also: kd64\kdinit.c
 */
static INIT_FUNCTION
SIZE_T
KdpGetMemorySizeInMBs(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLIST_ENTRY ListEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor;
    SIZE_T NumberOfPhysicalPages = 0;

    /* Loop the memory descriptors */
    for (ListEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
         ListEntry != &LoaderBlock->MemoryDescriptorListHead;
         ListEntry = ListEntry->Flink)
    {
        /* Get the descriptor */
        Descriptor = CONTAINING_RECORD(ListEntry,
                                       MEMORY_ALLOCATION_DESCRIPTOR,
                                       ListEntry);

        /* Check if this is invisible memory */
        if ((Descriptor->MemoryType == LoaderFirmwarePermanent) ||
            (Descriptor->MemoryType == LoaderSpecialMemory) ||
            (Descriptor->MemoryType == LoaderHALCachedMemory) ||
            (Descriptor->MemoryType == LoaderBBTMemory))
        {
            /* Skip this descriptor */
            continue;
        }

        /* Check if this is bad memory */
        if (Descriptor->MemoryType != LoaderBad)
        {
            /* Count this in the total of pages */
            NumberOfPhysicalPages += Descriptor->PageCount;
        }
    }

    /* Round size up. Assumed to better match actual physical RAM size */
    return ALIGN_UP_BY(NumberOfPhysicalPages * PAGE_SIZE, 1024 * 1024) / (1024 * 1024);
}

/* See also: kd64\kdinit.c */
static INIT_FUNCTION
VOID
KdpPrintBanner(IN SIZE_T MemSizeMBs)
{
    DPRINT1("-----------------------------------------------------\n");
    DPRINT1("ReactOS " KERNEL_VERSION_STR " (Build " KERNEL_VERSION_BUILD_STR ") (Commit " KERNEL_VERSION_COMMIT_HASH ")\n");
    DPRINT1("%u System Processor [%u MB Memory]\n", KeNumberProcessors, MemSizeMBs);
    DPRINT1("Command Line: %s\n", KeLoaderBlock->LoadOptions);
    DPRINT1("ARC Paths: %s %s %s %s\n", KeLoaderBlock->ArcBootDeviceName, KeLoaderBlock->NtHalPathName, KeLoaderBlock->ArcHalDeviceName, KeLoaderBlock->NtBootPathName);
}

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
                  ULONG StringLength)
{
    KIRQL OldIrql;
    ULONG beg, end, num;

    if (KdpDebugBuffer == NULL) return;

    /* Acquire the printing spinlock without waiting at raised IRQL */
    OldIrql = KdpAcquireLock(&KdpDebugLogSpinLock);

    beg = KdpCurrentPosition;
    num = KdpFreeBytes;
    if (StringLength < num)
        num = StringLength;

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
    KdpReleaseLock(&KdpDebugLogSpinLock, OldIrql);

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
    SIZE_T MemSizeMBs;

    if (!KdpDebugMode.File) return;

    if (BootPhase == 0)
    {
        KdComPortInUse = NULL;

        /* Write out the functions that we support for now */
        DispatchTable->KdpInitRoutine = KdpDebugLogInit;
        DispatchTable->KdpPrintRoutine = KdpPrintToLogFile;

        /* Register as a Provider */
        InsertTailList(&KdProviders, &DispatchTable->KdProvidersList);
    }
    else if (BootPhase == 1)
    {
        /* Allocate a buffer for debug log */
        KdpDebugBuffer = ExAllocatePool(NonPagedPool, KdpBufferSize);
        KdpFreeBytes = KdpBufferSize;

        /* Initialize spinlock */
        KeInitializeSpinLock(&KdpDebugLogSpinLock);

        /* Display separator + ReactOS version at start of the debug log */
        /* Round size up. Assumed to better match actual physical RAM size */
        MemSizeMBs = ALIGN_UP_BY(MmNumberOfPhysicalPages * PAGE_SIZE, 1024 * 1024) / (1024 * 1024);
        KdpPrintBanner(MemSizeMBs);
    }
    else if (BootPhase == 2)
    {
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
        Status = NtCreateFile(&KdpLogFileHandle,
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
            return;

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
            NtClose(KdpLogFileHandle);
            return;
        }

        Priority = 7;
        NtSetInformationThread(ThreadHandle,
                               ThreadPriority,
                               &Priority,
                               sizeof(Priority));
    }
}

/* SERIAL FUNCTIONS **********************************************************/

VOID
NTAPI
KdpSerialDebugPrint(PCHAR Message,
                    ULONG Length)
{
    PCHAR pch = (PCHAR)Message;
    KIRQL OldIrql;

    /* Acquire the printing spinlock without waiting at raised IRQL */
    OldIrql = KdpAcquireLock(&KdpSerialSpinLock);

    /* Output the message */
    while (pch < Message + Length && *pch != '\0')
    {
        if (*pch == '\n')
        {
            KdPortPutByteEx(&SerialPortInfo, '\r');
        }
        KdPortPutByteEx(&SerialPortInfo, *pch);
        pch++;
    }

    /* Release the spinlock */
    KdpReleaseLock(&KdpSerialSpinLock, OldIrql);
}

VOID
NTAPI
KdpSerialInit(PKD_DISPATCH_TABLE DispatchTable,
              ULONG BootPhase)
{
    SIZE_T MemSizeMBs;
    if (!KdpDebugMode.Serial) return;

    if (BootPhase == 0)
    {
        /* Write out the functions that we support for now */
        DispatchTable->KdpInitRoutine = KdpSerialInit;
        DispatchTable->KdpPrintRoutine = KdpSerialDebugPrint;

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

        /* Display separator + ReactOS version at start of the debug log */
        MemSizeMBs = KdpGetMemorySizeInMBs(KeLoaderBlock);
        KdpPrintBanner(MemSizeMBs);
    }
    else if (BootPhase == 2)
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
 * Screen debug logger function KdpScreenPrint() writes text messages into
 * KdpDmesgBuffer, using it as a circular buffer. KdpDmesgBuffer contents could
 * be later (re)viewed using dmesg command of kdbg. KdpScreenPrint() protects
 * KdpDmesgBuffer from simultaneous writes by use of KdpDmesgLogSpinLock.
 */
static VOID
NTAPI
KdpScreenPrint(PCHAR Message,
               ULONG Length)
{
    PCHAR pch = (PCHAR)Message;
    KIRQL OldIrql;
    ULONG beg, end, num;

    while (pch < Message + Length && *pch)
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

    /* Dmesg: store Message in the buffer to show it later */
    if (KdbpIsInDmesgMode)
       return;

    if (KdpDmesgBuffer == NULL)
      return;

    /* Acquire the printing spinlock without waiting at raised IRQL */
    OldIrql = KdpAcquireLock(&KdpDmesgLogSpinLock);

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
            RtlCopyMemory(KdpDmesgBuffer + beg, Message, Length);
        }
        else
        {
            RtlCopyMemory(KdpDmesgBuffer + beg, Message, KdpDmesgBufferSize - beg);
            RtlCopyMemory(KdpDmesgBuffer, Message + (KdpDmesgBufferSize - beg), end);
        }
        KdpDmesgCurrentPosition = end;

        /* Counting the total bytes written */
        KdbDmesgTotalWritten += num;
    }

    /* Release the spinlock */
    KdpReleaseLock(&KdpDmesgLogSpinLock, OldIrql);

    /* Optional step(?): find out a way to notify about buffer exhaustion,
     * and possibly fall into kbd to use dmesg command: user will read
     * debug messages before they will be wiped over by next writes.
     */
}

VOID
NTAPI
KdpScreenInit(PKD_DISPATCH_TABLE DispatchTable,
              ULONG BootPhase)
{
    SIZE_T MemSizeMBs;
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
        KdpDmesgBuffer = ExAllocatePool(NonPagedPool, KdpDmesgBufferSize + 1);
        RtlZeroMemory(KdpDmesgBuffer, KdpDmesgBufferSize + 1);
        KdpDmesgFreeBytes = KdpDmesgBufferSize;
        KdbDmesgTotalWritten = 0;

        /* Take control of the display */
        KdpScreenAcquire();

        /* Initialize spinlock */
        KeInitializeSpinLock(&KdpDmesgLogSpinLock);

        /* Display separator + ReactOS version at start of the debug log */
        /* Round size up. Assumed to better match actual physical RAM size */
        MemSizeMBs = ALIGN_UP_BY(MmNumberOfPhysicalPages * PAGE_SIZE, 1024 * 1024) / (1024 * 1024);
        KdpPrintBanner(MemSizeMBs);
    }
    else if (BootPhase == 2)
    {
        HalDisplayString("\r\n   Screen debugging enabled\r\n\r\n");
    }
}

/* GENERAL FUNCTIONS *********************************************************/

BOOLEAN
NTAPI
KdpPrintString(
    _In_ PSTRING Output)
{
    PLIST_ENTRY CurrentEntry;
    PKD_DISPATCH_TABLE CurrentTable;

    if (!KdpDebugMode.Value) return FALSE;

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

    /* Call the Wrapper Routine */
    if (WrapperTable.KdpPrintRoutine)
        WrapperTable.KdpPrintRoutine(Output->Buffer, Output->Length);

    return FALSE;
}

extern STRING KdbPromptString;

BOOLEAN
NTAPI
KdpPromptString(
    _In_ PSTRING PromptString,
    _In_ PSTRING ResponseString)
{
#ifdef KDBG
    KIRQL OldIrql;
    STRING StringChar;
    CHAR Response;
    USHORT i;
    ULONG DummyScanCode;

    StringChar.Buffer = &Response;
    StringChar.Length = StringChar.MaximumLength = sizeof(Response);

    /* Display the string and print a new line for log neatness */
    KdpPrintString(PromptString);
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
    for (i = 0; i < ResponseString->MaximumLength; i++)
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
            *(PCHAR)(ResponseString->Buffer + i) = 0;
            break;
        }

        /* Write it back and print it to the log */
        *(PCHAR)(ResponseString->Buffer + i) = Response;
        KdpReleaseLock(&KdpSerialSpinLock, OldIrql);
        KdpPrintString(&StringChar);
        OldIrql = KdpAcquireLock(&KdpSerialSpinLock);
    }

    /* Return the length */
    ResponseString->Length = i;

    if (!(KdbDebugState & KD_DEBUG_KDSERIAL))
        KbdEnableMouse();

    /* Release the spinlock */
    KdpReleaseLock(&KdpSerialSpinLock, OldIrql);

    /* Print a new line */
    *StringChar.Buffer = '\n';
    KdpPrintString(&StringChar);
#endif

    /* Success; we don't need to resend */
    return FALSE;
}

/* EOF */
