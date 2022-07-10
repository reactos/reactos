/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kdcom/kdbg.c
 * PURPOSE:         Serial i/o functions for the kernel debugger.
 * PROGRAMMER:      Alex Ionescu
 *                  Hervé Poussineau
 */

/* INCLUDES *****************************************************************/

#include "kdrosdbg.h"
#define NDEBUG
#include <debug.h>

#if defined(SARCH_PC98)
#define DEFAULT_BAUD_RATE   9600
#else
#define DEFAULT_BAUD_RATE   19200
#endif

#if defined(_M_IX86) || defined(_M_AMD64)
#if defined(SARCH_PC98)
const ULONG BaseArray[] = {0, 0x30, 0x238};
#else
const ULONG BaseArray[] = {0, 0x3F8, 0x2F8, 0x3E8, 0x2E8};
#endif
#elif defined(_M_PPC)
const ULONG BaseArray[] = {0, 0x800003F8};
#elif defined(_M_MIPS)
const ULONG BaseArray[] = {0, 0x80006000, 0x80007000};
#elif defined(_M_ARM)
const ULONG BaseArray[] = {0, 0xF1012000};
#else
#error Unknown architecture
#endif

/* These values MUST be nonzero.  They're used as bit masks. */
typedef enum _KDB_OUTPUT_SETTINGS
{
   KD_DEBUG_KDSERIAL = 1,
   KD_DEBUG_KDNOECHO = 2
} KDB_OUTPUT_SETTINGS;

/* KD Private Debug Modes */
typedef struct _KDP_DEBUG_MODE
{
    union
    {
        struct
        {
            /* Native Modes */
            UCHAR Screen :1;
            UCHAR Serial :1;
            UCHAR File :1;
        };

        /* Generic Value */
        ULONG Value;
    };
} KDP_DEBUG_MODE;

ULONG KdbDebugState = 0; /* KDBG Settings (NOECHO, KDSERIAL) */
KDP_DEBUG_MODE KdpDebugMode;

/* Port Information for the Serial Native Mode */
ULONG  SerialPortNumber;
CPPORT SerialPortInfo;

static ULONG KdpBufferSize = 0;
static PCHAR KdpDebugBuffer = NULL;
static volatile ULONG KdpCurrentPosition = 0;
static volatile ULONG KdpFreeBytes = 0;
ANSI_STRING KdpLogFileName = RTL_CONSTANT_STRING("\\SystemRoot\\debug.log");
static KSPIN_LOCK KdpDebugLogSpinLock;
static KEVENT KdpLoggerThreadEvent;
static HANDLE KdpLogFileHandle;

static VOID
NTAPI
KdbpGetCommandLineSettings(
    PCHAR p1)
{
#define CONST_STR_LEN(x) (sizeof(x)/sizeof(x[0]) - 1)

    while (p1 && (p1 = strchr(p1, ' ')))
    {
        /* Skip other spaces */
        while (*p1 == ' ') ++p1;

        if (!_strnicmp(p1, "KDSERIAL", CONST_STR_LEN("KDSERIAL")))
        {
            p1 += CONST_STR_LEN("KDSERIAL");
            KdbDebugState |= KD_DEBUG_KDSERIAL;
            KdpDebugMode.Serial = TRUE;
        }
        else if (!_strnicmp(p1, "KDNOECHO", CONST_STR_LEN("KDNOECHO")))
        {
            p1 += CONST_STR_LEN("KDNOECHO");
            KdbDebugState |= KD_DEBUG_KDNOECHO;
        }
    }
}

static PCHAR
NTAPI
KdpGetDebugMode(PCHAR Currentp2)
{
    PCHAR p1, p2 = Currentp2;
    ULONG Value;

    /* Check for Screen Debugging */
    if (!_strnicmp(p2, "SCREEN", 6))
    {
        /* Enable It */
        p2 += 6;
        KdpDebugMode.Screen = TRUE;
    }
    /* Check for Serial Debugging */
    else if (!_strnicmp(p2, "COM", 3))
    {
        /* Gheck for a valid Serial Port */
        p2 += 3;
        if (*p2 != ':')
        {
            Value = (ULONG)atol(p2);
            if (Value > 0 && Value < 5)
            {
                /* Valid port found, enable Serial Debugging */
                KdpDebugMode.Serial = TRUE;

                /* Set the port to use */
                SerialPortNumber = Value;
            }
        }
        else
        {
            Value = strtoul(p2 + 1, NULL, 0);
            if (Value)
            {
                KdpDebugMode.Serial = TRUE;
                SerialPortInfo.Address = UlongToPtr(Value);
                SerialPortNumber = 0;
            }
        }
    }
    /* Check for File Debugging */
    else if (!_strnicmp(p2, "FILE", 4))
    {
        /* Enable It */
        p2 += 4;
        KdpDebugMode.File = TRUE;
        if (*p2 == ':')
        {
            p2++;
            p1 = p2;
            while (*p2 != '\0' && *p2 != ' ') p2++;
            KdpLogFileName.MaximumLength = KdpLogFileName.Length = p2 - p1;
            KdpLogFileName.Buffer = p1;
        }
    }

    return p2;
}

/* SCREEN FUNCTIONS **********************************************************/

static VOID
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

static VOID
NTAPI
KdpScreenInit(ULONG BootPhase)
{
    if (!KdpDebugMode.Screen) return;

    if (BootPhase == 0)
    {
    }
    else if (BootPhase == 1)
    {
        /* Take control of the display */
        KdpScreenAcquire();

        HalDisplayString("\r\n   Screen debugging enabled\r\n\r\n");
    }
}

/* SERIAL FUNCTIONS **********************************************************/

static BOOLEAN
NTAPI
KdPortInitializeEx(
    IN PCPPORT PortInformation,
    IN ULONG ComPortNumber)
{
    NTSTATUS Status;

    /* Initialize the port */
    Status = CpInitialize(PortInformation,
                          (ComPortNumber == 0 ? PortInformation->Address
                                              : UlongToPtr(BaseArray[ComPortNumber])),
                          (PortInformation->BaudRate == 0 ? DEFAULT_BAUD_RATE
                                                          : PortInformation->BaudRate));
    if (!NT_SUCCESS(Status))
    {
        HalDisplayString("\r\nKernel Debugger: Serial port not found!\r\n\r\n");
        return FALSE;
    }
    else
    {
#ifndef NDEBUG
        CHAR buffer[80];

        /* Print message to blue screen */
        sprintf(buffer,
                "\r\nKernel Debugger: Serial port found: COM%ld (Port 0x%p) BaudRate %ld\r\n\r\n",
                ComPortNumber,
                PortInformation->Address,
                PortInformation->BaudRate);
        HalDisplayString(buffer);
#endif /* NDEBUG */

        return TRUE;
    }
}

static VOID
NTAPI
KdpSerialInit(ULONG BootPhase)
{
    if (!KdpDebugMode.Serial) return;

    if (BootPhase == 0)
    {
        /* Initialize the Port */
        if (!KdPortInitializeEx(&SerialPortInfo, SerialPortNumber))
        {
            KdpDebugMode.Serial = FALSE;
            return;
        }
        KdComPortInUse = SerialPortInfo.Address;
    }
    else if (BootPhase == 1)
    {
        HalDisplayString("\r\n   Serial debugging enabled\r\n\r\n");
    }
}

/* FILE FUNCTIONS **********************************************************/

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
         * KdpFileDebugPrint function
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

        (VOID)InterlockedExchangeAdd((LONG*)&KdpFreeBytes, num);
    }
}

static VOID
NTAPI
KdpFileInit(ULONG BootPhase)
{
    NTSTATUS Status;
    UNICODE_STRING FileName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK Iosb;
    HANDLE ThreadHandle;
    KPRIORITY Priority;
    ULONG BufferSize = 512 * 1024;
    PCHAR DebugBuffer;

    if (!KdpDebugMode.File) return;

    if (BootPhase == 0)
    {
    }
    else if (BootPhase == 1)
    {
        /* Allocate a buffer for debug log */
        DebugBuffer = ExAllocatePoolZero(NonPagedPool,
                                         BufferSize,
                                         TAG_KDBG);
        if (DebugBuffer)
        {
            RtlCopyMemory(DebugBuffer, KdpDebugBuffer, KdpBufferSize - KdpFreeBytes);
            KdpDebugBuffer = DebugBuffer;
            KdpFreeBytes += BufferSize - KdpBufferSize;
            KdpBufferSize = BufferSize;
        }

        /* Initialize spinlock */
        KeInitializeSpinLock(&KdpDebugLogSpinLock);

        HalDisplayString("\r\n   File log debugging enabled\r\n\r\n");
    }
    else if (BootPhase == 3 && !KdpDebugBuffer)
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
            KdpLogFileHandle = NULL;
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
            KdpLogFileHandle = NULL;
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

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
KdDebuggerInitialize0(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock OPTIONAL)
{
    PCHAR CommandLine, Port = NULL;

    if (LoaderBlock)
    {
        /* Check if we have a command line */
        CommandLine = LoaderBlock->LoadOptions;
        if (CommandLine)
        {
            /* Upcase it */
            _strupr(CommandLine);

            /* Get the KDBG Settings */
            KdbpGetCommandLineSettings(CommandLine);

            /* Get the port */
            Port = strstr(CommandLine, "DEBUGPORT");
        }
    }

    /* Check if we got the /DEBUGPORT parameter(s) */
    while (Port)
    {
        /* Move past the actual string, to reach the port*/
        Port += sizeof("DEBUGPORT") - 1;

        /* Now get past any spaces and skip the equal sign */
        while (*Port == ' ') Port++;
        Port++;

        /* Get the debug mode and wrapper */
        Port = KdpGetDebugMode(Port);
        Port = strstr(Port, "DEBUGPORT");
    }

    /* Use serial port then */
    if (KdpDebugMode.Value == 0)
        KdpDebugMode.Serial = TRUE;

    /* Call Providers at Phase 0 */
    if (KdpDebugMode.Screen)
        KdpScreenInit(0);
    if (KdpDebugMode.Serial)
        KdpSerialInit(0);
    if (KdpDebugMode.File)
        KdpFileInit(0);

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
KdDebuggerInitialize1(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock OPTIONAL)
{
    /* Call the registered handlers */
    if (KdpDebugMode.Screen)
        KdpScreenInit(1);
    if (KdpDebugMode.Serial)
        KdpSerialInit(1);
    if (KdpDebugMode.File)
        KdpFileInit(1);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdD0Transition(VOID)
{
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdD3Transition(VOID)
{
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
KdSave(
    IN BOOLEAN SleepTransition)
{
    /* Nothing to do on COM ports */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
KdRestore(
    IN BOOLEAN SleepTransition)
{
    /* Nothing to do on COM ports */
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
VOID
NTAPI
KdSendPacket(
    IN ULONG PacketType,
    IN PSTRING MessageHeader,
    IN PSTRING MessageData,
    IN OUT PKD_CONTEXT Context)
{
}

/*
 * @unimplemented
 */
KDSTATUS
NTAPI
KdReceivePacket(
    IN ULONG PacketType,
    OUT PSTRING MessageHeader,
    OUT PSTRING MessageData,
    OUT PULONG DataLength,
    IN OUT PKD_CONTEXT Context)
{
#ifndef KDBG
    if (PacketType == PACKET_TYPE_KD_STATE_MANIPULATE)
    {
        /* Let's say that everything went fine every time. */
        PDBGKD_MANIPULATE_STATE64 ManipulateState = (PDBGKD_MANIPULATE_STATE64)MessageHeader->Buffer;
        RtlZeroMemory(MessageHeader->Buffer, MessageHeader->MaximumLength);
        ManipulateState->ApiNumber = DbgKdContinueApi;
        ManipulateState->u.Continue.ContinueStatus = STATUS_SUCCESS;
        MessageHeader->Length = sizeof(*ManipulateState);
        return KdPacketReceived;
    }
#endif

    return KdPacketTimedOut;
}

/* EOF */
