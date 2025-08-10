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

#include <debug.h>

#undef KdSendPacket
#undef KdReceivePacket

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
#ifdef _M_AMD64
    {
        const char msg[] = "*** KdpSerialInit: Entry ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
    }
#endif
    if (!KdpDebugMode.Serial)
    {
#ifdef _M_AMD64
        {
            const char msg[] = "*** KdpSerialInit: Serial mode not enabled ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
#endif
        return STATUS_PORT_DISCONNECTED;
    }

    if (BootPhase == 0)
    {
#ifdef _M_AMD64
        {
            const char msg[] = "*** KdpSerialInit: BootPhase 0 ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
#endif
        /* Write out the functions that we support for now */
        DispatchTable->KdpPrintRoutine = KdpSerialPrint;

        /* Initialize the Port */
#ifdef _M_AMD64
        {
            const char msg[] = "*** KdpSerialInit: About to call KdPortInitializeEx ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
        
        /* On AMD64, the serial port might already be initialized by the bootloader */
        /* We can use it directly if we can write to it */
        BOOLEAN PortInitialized = FALSE;
        
        /* Try to initialize the port */
        if (!KdPortInitializeEx(&SerialPortInfo, SerialPortNumber))
        {
            const char msg[] = "*** KdpSerialInit: KdPortInitializeEx failed, trying direct use ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
            
            /* If we can already write to the serial port (as evidenced by our debug messages),
             * just use it directly without initialization */
            SerialPortInfo.Address = UlongToPtr(0x3F8); /* COM1 */
            PortInitialized = TRUE; /* Mark as already initialized */
        }
        else
        {
            const char msg[] = "*** KdpSerialInit: KdPortInitializeEx succeeded ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
            PortInitialized = TRUE;
        }
        
        if (!PortInitialized)
        {
            const char msg[] = "*** KdpSerialInit: Port initialization completely failed ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
            KdpDebugMode.Serial = FALSE;
            return STATUS_DEVICE_DOES_NOT_EXIST;
        }
#else
        if (!KdPortInitializeEx(&SerialPortInfo, SerialPortNumber))
        {
            KdpDebugMode.Serial = FALSE;
            return STATUS_DEVICE_DOES_NOT_EXIST;
        }
#endif
        KdComPortInUse = SerialPortInfo.Address;

        /* Initialize spinlock */
#ifdef _M_AMD64
        {
            const char msg[] = "*** KdpSerialInit: About to initialize spinlock ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
#endif
        KeInitializeSpinLock(&KdpSerialSpinLock);
        
#ifdef _M_AMD64
        {
            const char msg[] = "*** KdpSerialInit: Spinlock initialized, setting up provider ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
#endif

        /* Register for BootPhase 1 initialization and as a Provider */
        DispatchTable->KdpInitRoutine = KdpSerialInit;
        
#ifdef _M_AMD64
        {
            const char msg[] = "*** KdpSerialInit: About to InsertTailList ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
        
        /* Check if KdProviders list needs relocation on AMD64 */
        {
            ULONG_PTR FlinkAddr = (ULONG_PTR)KdProviders.Flink;
            ULONG_PTR BlinkAddr = (ULONG_PTR)KdProviders.Blink;
            
            /* Check if addresses are in low memory (not relocated) */
            if (FlinkAddr < 0xFFFF800000000000ULL || BlinkAddr < 0xFFFF800000000000ULL)
            {
                const char msg[] = "*** KdpSerialInit: KdProviders has unrelocated addresses, reinitializing ***\n";
                const char *p = msg;
                while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
                
                /* Reinitialize the list with proper addresses */
                InitializeListHead(&KdProviders);
                
                {
                    const char msg2[] = "*** KdpSerialInit: KdProviders reinitialized with kernel addresses ***\n";
                    const char *p2 = msg2;
                    while (*p2) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p2++); }
                }
            }
            else
            {
                const char msg[] = "*** KdpSerialInit: KdProviders already has kernel addresses ***\n";
                const char *p = msg;
                while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
            }
        }
        
        /* Try to insert without InitializeListHead on the entry */
        {
            const char msg[] = "*** KdpSerialInit: Checking DispatchTable pointer ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
        
        if (DispatchTable == NULL)
        {
            const char msg[] = "*** KdpSerialInit: ERROR - DispatchTable is NULL! ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
            return STATUS_INVALID_PARAMETER;
        }
        
        {
            const char msg[] = "*** KdpSerialInit: DispatchTable is valid, checking KdProviders ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
        
        /* Print addresses for debugging */
        {
            char msg[100];
            int j = 0;
            msg[j++] = '*'; msg[j++] = '*'; msg[j++] = '*';
            msg[j++] = ' '; msg[j++] = 'K'; msg[j++] = 'd';
            msg[j++] = 'P'; msg[j++] = 'r'; msg[j++] = 'o';
            msg[j++] = 'v'; msg[j++] = 'i'; msg[j++] = 'd';
            msg[j++] = 'e'; msg[j++] = 'r'; msg[j++] = 's';
            msg[j++] = '.'; msg[j++] = 'B'; msg[j++] = 'l';
            msg[j++] = 'i'; msg[j++] = 'n'; msg[j++] = 'k';
            msg[j++] = '='; msg[j++] = '0'; msg[j++] = 'x';
            
            ULONG_PTR addr = (ULONG_PTR)KdProviders.Blink;
            for (int k = 15; k >= 0; k--) {
                ULONG_PTR nibble = (addr >> (k * 4)) & 0xF;
                if (nibble < 10)
                    msg[j++] = '0' + nibble;
                else
                    msg[j++] = 'A' + (nibble - 10);
            }
            msg[j++] = '\n';
            msg[j] = '\0';
            
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
        
        /* Now do the actual list insertion */
        {
            const char msg[] = "*** KdpSerialInit: Performing InsertTailList ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
        
        /* Insert the provider into the list */
        InsertTailList(&KdProviders, &DispatchTable->KdProvidersList);
        
        {
            const char msg[] = "*** KdpSerialInit: InsertTailList succeeded! ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
        
        {
            const char msg[] = "*** KdpSerialInit: InsertTailList completed ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
#else
        InsertTailList(&KdProviders, &DispatchTable->KdProvidersList);
#endif
        
#ifdef _M_AMD64
        {
            const char msg[] = "*** KdpSerialInit: Initialization complete, returning SUCCESS ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
#endif
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
#ifdef _M_AMD64
    {
        const char msg[] = "*** KdpScreenInit: Entry ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
    }
#endif
    if (!KdpDebugMode.Screen)
        return STATUS_PORT_DISCONNECTED;

    if (BootPhase == 0)
    {
#ifdef _M_AMD64
        {
            const char msg[] = "*** KdpScreenInit: BootPhase 0 ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
#endif
        /* Write out the functions that we support for now */
        DispatchTable->KdpPrintRoutine = KdpScreenPrint;

        /* Register for BootPhase 1 initialization and as a Provider */
        DispatchTable->KdpInitRoutine = KdpScreenInit;
#ifdef _M_AMD64
        {
            const char msg[] = "*** KdpScreenInit: About to InsertTailList ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
#endif
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

#ifdef KDBG
extern const CSTRING KdbPromptStr;
#endif

VOID
NTAPI
KdSendPacket(
    _In_ ULONG PacketType,
    _In_ PSTRING MessageHeader,
    _In_opt_ PSTRING MessageData,
    _Inout_ PKD_CONTEXT Context)
{
    PDBGKD_DEBUG_IO DebugIo;


    if (PacketType == PACKET_TYPE_KD_STATE_CHANGE32 ||
        PacketType == PACKET_TYPE_KD_STATE_CHANGE64)
    {
        PDBGKD_ANY_WAIT_STATE_CHANGE WaitStateChange = (PDBGKD_ANY_WAIT_STATE_CHANGE)MessageHeader->Buffer;

        if (WaitStateChange->NewState == DbgKdLoadSymbolsStateChange)
            return; // Ignore: invoked anytime a new module is loaded.

        /* We should not get there, unless an exception has been raised */
        if (WaitStateChange->NewState == DbgKdExceptionStateChange)
        {
            PEXCEPTION_RECORD64 ExceptionRecord = &WaitStateChange->u.Exception.ExceptionRecord;

            /*
             * Claim the debugger to be present, so that KdpSendWaitContinue()
             * can call back KdReceivePacket(PACKET_TYPE_KD_STATE_MANIPULATE),
             * which, in turn, informs KD that the exception cannot be handled.
             */
            KD_DEBUGGER_NOT_PRESENT = FALSE;
            SharedUserData->KdDebuggerEnabled |= 0x00000002;

            KdIoPrintf("%s: Got exception 0x%08lx @ 0x%p, Flags 0x%08x, %s - Info[0]: 0x%p\n",
                       __FUNCTION__,
                       ExceptionRecord->ExceptionCode,
                       (PVOID)(ULONG_PTR)ExceptionRecord->ExceptionAddress,
                       ExceptionRecord->ExceptionFlags,
                       WaitStateChange->u.Exception.FirstChance ? "FirstChance" : "LastChance",
                       ExceptionRecord->ExceptionInformation[0]);
#if defined(_M_IX86) || defined(_M_AMD64) || defined(_M_ARM) || defined(_M_ARM64)
extern VOID NTAPI RtlpBreakWithStatusInstruction(VOID);
            if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
                ((PVOID)(ULONG_PTR)ExceptionRecord->ExceptionAddress == (PVOID)RtlpBreakWithStatusInstruction))
            {
                PCONTEXT ContextRecord = &KeGetCurrentPrcb()->ProcessorState.ContextFrame;
                ULONG Status =
#if defined(_M_IX86)
                    ContextRecord->Eax;
#elif defined(_M_AMD64)
                    (ULONG)ContextRecord->Rcx;
#elif defined(_M_ARM)
                    ContextRecord->R0;
#else // defined(_M_ARM64)
                    (ULONG)ContextRecord->X0;
#endif
                KdIoPrintf("STATUS_BREAKPOINT Status 0x%08lx\n", Status);
            }
// #else
// #error Unknown architecture
#endif
            return;
        }

        KdIoPrintf("%s: PACKET_TYPE_KD_STATE_CHANGE32/64 NewState %d is UNIMPLEMENTED\n",
                   __FUNCTION__, WaitStateChange->NewState);
        return;
    }
    else
    if (PacketType == PACKET_TYPE_KD_STATE_MANIPULATE)
    {
        PDBGKD_MANIPULATE_STATE64 ManipulateState = (PDBGKD_MANIPULATE_STATE64)MessageHeader->Buffer;
        KdIoPrintf("%s: PACKET_TYPE_KD_STATE_MANIPULATE for ApiNumber %lu\n",
                   __FUNCTION__, ManipulateState->ApiNumber);
        return;
    }

    if (PacketType != PACKET_TYPE_KD_DEBUG_IO)
    {
        KdIoPrintf("%s: PacketType %d is UNIMPLEMENTED\n", __FUNCTION__, PacketType);
        return;
    }

    DebugIo = (PDBGKD_DEBUG_IO)MessageHeader->Buffer;

    /* Validate API call */
    if (MessageHeader->Length != sizeof(DBGKD_DEBUG_IO))
        return;
    if ((DebugIo->ApiNumber != DbgKdPrintStringApi) &&
        (DebugIo->ApiNumber != DbgKdGetStringApi))
    {
        return;
    }
    if (!MessageData)
        return;

    /* NOTE: MessageData->Length should be equal to
     * DebugIo.u.PrintString.LengthOfString, or to
     * DebugIo.u.GetString.LengthOfPromptString */

    if (!KdpDebugMode.Value)
    {
        return;
    }


    /* Print the string proper */
    KdIoPrintString(MessageData->Buffer, MessageData->Length);
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
    PDBGKD_DEBUG_IO DebugIo;
    STRING ResponseString;
    CHAR MessageBuffer[512];
#endif

    if (PacketType == PACKET_TYPE_KD_POLL_BREAKIN)
    {
        /* We don't support breaks-in */
        return KdPacketTimedOut;
    }

    if (PacketType == PACKET_TYPE_KD_STATE_MANIPULATE)
    {
        PDBGKD_MANIPULATE_STATE64 ManipulateState = (PDBGKD_MANIPULATE_STATE64)MessageHeader->Buffer;
        RtlZeroMemory(MessageHeader->Buffer, MessageHeader->MaximumLength);

        /* The exception (notified via DbgKdExceptionStateChange in
         * KdSendPacket()) cannot be handled: return a failure code */
        ManipulateState->ApiNumber = DbgKdContinueApi;
        ManipulateState->u.Continue.ContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
        return KdPacketReceived;
    }

    if (PacketType != PACKET_TYPE_KD_DEBUG_IO)
    {
        KdIoPrintf("%s: PacketType %d is UNIMPLEMENTED\n", __FUNCTION__, PacketType);
        return KdPacketTimedOut;
    }

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
