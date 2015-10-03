/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/emulator.c
 * PURPOSE:         Minimal x86 machine emulator for the VDM
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"
#include "memory.h"

#include "cpu/callback.h"
#include "cpu/cpu.h"
#include "cpu/bop.h"
#include <isvbop.h>

#include "int32.h"

#include "clock.h"
#include "bios/rom.h"
#include "hardware/cmos.h"
#include "hardware/disk.h"
#include "hardware/dma.h"
#include "hardware/keyboard.h"
#include "hardware/mouse.h"
#include "hardware/pic.h"
#include "hardware/pit.h"
#include "hardware/ppi.h"
#include "hardware/ps2.h"
#include "hardware/sound/speaker.h"
#include "hardware/video/svga.h"

#include "vddsup.h"
#include "io.h"

/* PRIVATE VARIABLES **********************************************************/

LPVOID  BaseAddress = NULL;
BOOLEAN VdmRunning  = TRUE;

HANDLE VdmTaskEvent = NULL;
static HANDLE InputThread = NULL;

LPCWSTR ExceptionName[] =
{
    L"Division By Zero",
    L"Debug",
    L"Unexpected Error",
    L"Breakpoint",
    L"Integer Overflow",
    L"Bound Range Exceeded",
    L"Invalid Opcode",
    L"FPU Not Available"
};

/* BOP Identifiers */
#define BOP_DEBUGGER    0x56    // Break into the debugger from a 16-bit app

/* PRIVATE FUNCTIONS **********************************************************/

UCHAR FASTCALL EmulatorIntAcknowledge(PFAST486_STATE State)
{
    UNREFERENCED_PARAMETER(State);

    /* Get the interrupt number from the PIC */
    return PicGetInterrupt();
}

VOID FASTCALL EmulatorFpu(PFAST486_STATE State)
{
    /* The FPU is wired to IRQ 13 */
    PicInterruptRequest(13);
}

VOID EmulatorException(BYTE ExceptionNumber, LPWORD Stack)
{
    WORD CodeSegment, InstructionPointer;
    PBYTE Opcode;

    ASSERT(ExceptionNumber < 8);

    /* Get the CS:IP */
    InstructionPointer = Stack[STACK_IP];
    CodeSegment = Stack[STACK_CS];
    Opcode = (PBYTE)SEG_OFF_TO_PTR(CodeSegment, InstructionPointer);

    /* Display a message to the user */
    DisplayMessage(L"Exception: %s occurred at %04X:%04X\n"
                   L"Opcode: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                   ExceptionName[ExceptionNumber],
                   CodeSegment,
                   InstructionPointer,
                   Opcode[0],
                   Opcode[1],
                   Opcode[2],
                   Opcode[3],
                   Opcode[4],
                   Opcode[5],
                   Opcode[6],
                   Opcode[7],
                   Opcode[8],
                   Opcode[9]);

    Fast486DumpState(&EmulatorContext);

    /* Stop the VDM */
    EmulatorTerminate();
}

VOID EmulatorInterruptSignal(VOID)
{
    /* Call the Fast486 API */
    Fast486InterruptSignal(&EmulatorContext);
}

static VOID WINAPI EmulatorDebugBreakBop(LPWORD Stack)
{
    DPRINT1("NTVDM: BOP_DEBUGGER\n");
    DebugBreak();
}

static VOID WINAPI PitChan0Out(LPVOID Param, BOOLEAN State)
{
    if (State)
    {
        DPRINT("PicInterruptRequest\n");
        PicInterruptRequest(0); // Raise IRQ 0
    }
    // else < Lower IRQ 0 >
}

static VOID WINAPI PitChan1Out(LPVOID Param, BOOLEAN State)
{
#if 0
    if (State)
    {
        /* Set bit 4 of Port 61h */
        Port61hState |= 1 << 4;
    }
    else
    {
        /* Clear bit 4 of Port 61h */
        Port61hState &= ~(1 << 4);
    }
#else
    Port61hState = (Port61hState & 0xEF) | (State << 4);
#endif
}

static VOID WINAPI PitChan2Out(LPVOID Param, BOOLEAN State)
{
    BYTE OldPort61hState = Port61hState;

#if 0
    if (State)
    {
        /* Set bit 5 of Port 61h */
        Port61hState |= 1 << 5;
    }
    else
    {
        /* Clear bit 5 of Port 61h */
        Port61hState &= ~(1 << 5);
    }
#else
    Port61hState = (Port61hState & 0xDF) | (State << 5);
#endif

    if ((OldPort61hState ^ Port61hState) & 0x20)
    {
        DPRINT("PitChan2Out -- Port61hState changed\n");
        SpeakerChange(Port61hState);
    }
}


static DWORD
WINAPI
ConsoleEventThread(LPVOID Parameter)
{
    HANDLE ConsoleInput = (HANDLE)Parameter;
    HANDLE WaitHandles[2];
    DWORD  WaitResult;

    /*
     * For optimization purposes, Windows (and hence ReactOS, too, for
     * compatibility reasons) uses a static buffer if no more than five
     * input records are read. Otherwise a new buffer is used.
     * The client-side expects that we know this behaviour.
     * See consrv/coninput.c
     *
     * We exploit here this optimization by also using a buffer of 5 records.
     */
    INPUT_RECORD InputRecords[5];
    ULONG NumRecords, i;

    WaitHandles[0] = VdmTaskEvent;
    WaitHandles[1] = GetConsoleInputWaitHandle();

    while (VdmRunning)
    {
        /* Make sure the task event is signaled */
        WaitResult = WaitForMultipleObjects(ARRAYSIZE(WaitHandles),
                                            WaitHandles,
                                            TRUE,
                                            INFINITE);
        switch (WaitResult)
        {
            case WAIT_OBJECT_0 + 0:
            case WAIT_OBJECT_0 + 1:
                break;
            default:
                return GetLastError();
        }

        /* Wait for an input record */
        if (!ReadConsoleInputExW(ConsoleInput,
                                 InputRecords,
                                 ARRAYSIZE(InputRecords),
                                 &NumRecords,
                                 CONSOLE_READ_CONTINUE))
        {
            DWORD LastError = GetLastError();
            DPRINT1("Error reading console input (0x%p, %lu) - Error %lu\n", ConsoleInput, NumRecords, LastError);
            return LastError;
        }

        // ASSERT(NumRecords != 0);
        if (NumRecords == 0)
        {
            DPRINT1("Got NumRecords == 0!\n");
            continue;
        }

        /* Dispatch the events */
        for (i = 0; i < NumRecords; i++)
        {
            /* Check the event type */
            switch (InputRecords[i].EventType)
            {
                /*
                 * Hardware events
                 */
                case KEY_EVENT:
                    KeyboardEventHandler(&InputRecords[i].Event.KeyEvent);
                    break;

                case MOUSE_EVENT:
                    MouseEventHandler(&InputRecords[i].Event.MouseEvent);
                    break;

                case WINDOW_BUFFER_SIZE_EVENT:
                    ScreenEventHandler(&InputRecords[i].Event.WindowBufferSizeEvent);
                    break;

                /*
                 * Interface events
                 */
                case MENU_EVENT:
                    MenuEventHandler(&InputRecords[i].Event.MenuEvent);
                    break;

                case FOCUS_EVENT:
                    FocusEventHandler(&InputRecords[i].Event.FocusEvent);
                    break;

                default:
                    DPRINT1("Unknown input event type 0x%04x\n", InputRecords[i].EventType);
                    break;
            }
        }

        /* Let the console subsystem queue some new events */
        Sleep(10);
    }

    return 0;
}

static VOID PauseEventThread(VOID)
{
    ResetEvent(VdmTaskEvent);
}

static VOID ResumeEventThread(VOID)
{
    SetEvent(VdmTaskEvent);
}


/* PUBLIC FUNCTIONS ***********************************************************/

static VOID
DumpMemoryRaw(HANDLE hFile)
{
    PVOID  Buffer;
    SIZE_T Size;

    /* Dump the VM memory */
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    Buffer = REAL_TO_PHYS(NULL);
    Size   = MAX_ADDRESS - (ULONG_PTR)(NULL);
    WriteFile(hFile, Buffer, Size, &Size, NULL);
}

static VOID
DumpMemoryTxt(HANDLE hFile)
{
#define LINE_SIZE   75 + 2
    ULONG  i;
    PBYTE  Ptr1, Ptr2;
    CHAR   LineBuffer[LINE_SIZE];
    PCHAR  Line;
    SIZE_T LineSize;

    /* Dump the VM memory */
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    Ptr1 = Ptr2 = REAL_TO_PHYS(NULL);
    while (MAX_ADDRESS - (ULONG_PTR)PHYS_TO_REAL(Ptr1) > 0)
    {
        Ptr1 = Ptr2;
        Line = LineBuffer;

        /* Print the address */
        Line += snprintf(Line, LINE_SIZE + LineBuffer - Line, "%08x ", PHYS_TO_REAL(Ptr1));

        /* Print up to 16 bytes... */

        /* ... in hexadecimal form first... */
        i = 0;
        while (i++ <= 0x0F && (MAX_ADDRESS - (ULONG_PTR)PHYS_TO_REAL(Ptr1) > 0))
        {
            Line += snprintf(Line, LINE_SIZE + LineBuffer - Line, " %02x", *Ptr1);
            ++Ptr1;
        }

        /* ... align with spaces if needed... */
        RtlFillMemory(Line, 0x0F + 4 - i, ' ');
        Line += 0x0F + 4 - i;

        /* ... then in character form. */
        i = 0;
        while (i++ <= 0x0F && (MAX_ADDRESS - (ULONG_PTR)PHYS_TO_REAL(Ptr2) > 0))
        {
            *Line++ = ((*Ptr2 >= 0x20 && *Ptr2 <= 0x7E) || (*Ptr2 >= 0x80 && *Ptr2 < 0xFF) ? *Ptr2 : '.');
            ++Ptr2;
        }

        /* Newline */
        *Line++ = '\r';
        *Line++ = '\n';

        /* Finally write the line to the file */
        LineSize = Line - LineBuffer;
        WriteFile(hFile, LineBuffer, LineSize, &LineSize, NULL);
    }
}

VOID DumpMemory(BOOLEAN TextFormat)
{
    static ULONG DumpNumber = 0;

    HANDLE hFile;
    WCHAR  FileName[MAX_PATH];

    /* Build a suitable file name */
    _snwprintf(FileName, MAX_PATH,
               L"memdump%lu.%s",
               DumpNumber,
               TextFormat ? L"txt" : L"dat");
    ++DumpNumber;

    DPRINT1("Creating memory dump file '%S'...\n", FileName);

    /* Always create the dump file */
    hFile = CreateFileW(FileName,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        DPRINT1("Error when creating '%S' for memory dumping, GetLastError() = %u\n",
                FileName, GetLastError());
        return;
    }

    /* Dump the VM memory in the chosen format */
    if (TextFormat)
        DumpMemoryTxt(hFile);
    else
        DumpMemoryRaw(hFile);

    /* Close the file */
    CloseHandle(hFile);

    DPRINT1("Memory dump done\n");
}

VOID MountFloppy(IN ULONG DiskNumber)
{
// FIXME: This should be present in PSDK commdlg.h
//
// FlagsEx Values
#if (_WIN32_WINNT >= 0x0500)
#define  OFN_EX_NOPLACESBAR         0x00000001
#endif // (_WIN32_WINNT >= 0x0500)

    OPENFILENAMEW ofn;
    WCHAR szFile[MAX_PATH] = L"";
    UNICODE_STRING ValueString;

    ASSERT(DiskNumber < ARRAYSIZE(GlobalSettings.FloppyDisks));

    RtlZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize  = sizeof(ofn);
    ofn.hwndOwner    = hConsoleWnd;
    ofn.lpstrTitle   = L"Select a virtual floppy image";
    ofn.Flags        = OFN_EXPLORER | OFN_ENABLESIZING | OFN_LONGNAMES | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
//  ofn.FlagsEx      = OFN_EX_NOPLACESBAR;
    ofn.lpstrFilter  = L"Virtual floppy images (*.vfd;*.img;*.ima;*.dsk)\0*.vfd;*.img;*.ima;*.dsk\0All files (*.*)\0*.*\0\0";
    ofn.lpstrDefExt  = L"vfd";
    ofn.nFilterIndex = 0;
    ofn.lpstrFile    = szFile;
    ofn.nMaxFile     = ARRAYSIZE(szFile);

    if (!GetOpenFileNameW(&ofn))
    {
        DPRINT1("CommDlgExtendedError = %d\n", CommDlgExtendedError());
        return;
    }

    /* Free the old string */
    if (GlobalSettings.FloppyDisks[DiskNumber].Buffer)
        RtlFreeAnsiString(&GlobalSettings.FloppyDisks[DiskNumber]);

    /* Convert the UNICODE string to ANSI and store it */
    RtlInitEmptyUnicodeString(&ValueString, szFile, wcslen(szFile) * sizeof(WCHAR));
    ValueString.Length = ValueString.MaximumLength;
    RtlUnicodeStringToAnsiString(&GlobalSettings.FloppyDisks[DiskNumber], &ValueString, TRUE);

    /* Mount the disk */
    if (!MountDisk(FLOPPY_DISK, DiskNumber, GlobalSettings.FloppyDisks[DiskNumber].Buffer, !!(ofn.Flags & OFN_READONLY)))
    {
        DisplayMessage(L"An error happened when mounting disk %d", DiskNumber);
        RtlFreeAnsiString(&GlobalSettings.FloppyDisks[DiskNumber]);
        RtlInitEmptyAnsiString(&GlobalSettings.FloppyDisks[DiskNumber], NULL, 0);
        return;
    }

    /* Refresh the menu state */
    UpdateVdmMenuDisks();
}

VOID EjectFloppy(IN ULONG DiskNumber)
{
    ASSERT(DiskNumber < ARRAYSIZE(GlobalSettings.FloppyDisks));

    /* Unmount the disk */
    if (!UnmountDisk(FLOPPY_DISK, DiskNumber))
        DisplayMessage(L"An error happened when ejecting disk %d", DiskNumber);

    /* Free the old string */
    if (GlobalSettings.FloppyDisks[DiskNumber].Buffer)
    {
        RtlFreeAnsiString(&GlobalSettings.FloppyDisks[DiskNumber]);
        RtlInitEmptyAnsiString(&GlobalSettings.FloppyDisks[DiskNumber], NULL, 0);
    }

    /* Refresh the menu state */
    UpdateVdmMenuDisks();
}


VOID EmulatorPause(VOID)
{
    /* Pause the VDM */
    VDDBlockUserHook();
    VgaRefreshDisplay();
    PauseEventThread();
}

VOID EmulatorResume(VOID)
{
    /* Resume the VDM */
    ResumeEventThread();
    VgaRefreshDisplay();
    VDDResumeUserHook();
}

VOID EmulatorTerminate(VOID)
{
    /* Stop the VDM */
    CpuUnsimulate(); // Halt the CPU
    VdmRunning = FALSE;
}

BOOLEAN EmulatorInitialize(HANDLE ConsoleInput, HANDLE ConsoleOutput)
{
    USHORT i;

    /* Initialize memory */
    if (!MemInitialize())
    {
        wprintf(L"Memory initialization failed.\n");
        return FALSE;
    }

    /* Initialize I/O ports */
    /* Initialize RAM */

    /* Initialize the CPU */

    /* Initialize the internal clock */
    if (!ClockInitialize())
    {
        wprintf(L"FATAL: Failed to initialize the clock\n");
        EmulatorCleanup();
        return FALSE;
    }

    /* Initialize the CPU */
    CpuInitialize();

    /* Initialize DMA */
    DmaInitialize();

    /* Initialize PIC, PIT, CMOS, PC Speaker and PS/2 */
    PicInitialize();

    PitInitialize();
    PitSetOutFunction(0, NULL, PitChan0Out);
    PitSetOutFunction(1, NULL, PitChan1Out);
    PitSetOutFunction(2, NULL, PitChan2Out);

    CmosInitialize();
    SpeakerInitialize();
    PpiInitialize();

    PS2Initialize();

    /* Initialize the keyboard and mouse and connect them to their PS/2 ports */
    KeyboardInit(0);
    MouseInit(1);

    /**************** ATTACH INPUT WITH CONSOLE *****************/
    /* Create the task event */
    VdmTaskEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    ASSERT(VdmTaskEvent != NULL);

    /* Start the input thread */
    InputThread = CreateThread(NULL, 0, &ConsoleEventThread, ConsoleInput, 0, NULL);
    if (InputThread == NULL)
    {
        wprintf(L"FATAL: Failed to create the console input thread.\n");
        EmulatorCleanup();
        return FALSE;
    }
    ResumeEventThread();
    /************************************************************/

    /* Initialize the VGA */
    if (!VgaInitialize(ConsoleOutput))
    {
        wprintf(L"FATAL: Failed to initialize VGA support.\n");
        EmulatorCleanup();
        return FALSE;
    }

    /* Initialize the disk controller */
    if (!DiskCtrlInitialize())
    {
        wprintf(L"FATAL: Failed to completely initialize the disk controller.\n");
        EmulatorCleanup();
        return FALSE;
    }

    /* Mount the available floppy disks */
    for (i = 0; i < ARRAYSIZE(GlobalSettings.FloppyDisks); ++i)
    {
        if (GlobalSettings.FloppyDisks[i].Length != 0 &&
            GlobalSettings.FloppyDisks[i].Buffer      &&
            GlobalSettings.FloppyDisks[i].Buffer != '\0')
        {
            if (!MountDisk(FLOPPY_DISK, i, GlobalSettings.FloppyDisks[i].Buffer, FALSE))
            {
                DPRINT1("Failed to mount floppy disk file '%Z'.\n", &GlobalSettings.FloppyDisks[i]);
                RtlFreeAnsiString(&GlobalSettings.FloppyDisks[i]);
                RtlInitEmptyAnsiString(&GlobalSettings.FloppyDisks[i], NULL, 0);
            }
        }
    }

    /*
     * Mount the available hard disks. Contrary to floppies, failing
     * mounting a hard disk is considered as an unrecoverable error.
     */
    for (i = 0; i < ARRAYSIZE(GlobalSettings.HardDisks); ++i)
    {
        if (GlobalSettings.HardDisks[i].Length != 0 &&
            GlobalSettings.HardDisks[i].Buffer      &&
            GlobalSettings.HardDisks[i].Buffer != '\0')
        {
            if (!MountDisk(HARD_DISK, i, GlobalSettings.HardDisks[i].Buffer, FALSE))
            {
                wprintf(L"FATAL: Failed to mount hard disk file '%Z'.\n", &GlobalSettings.HardDisks[i]);
                EmulatorCleanup();
                return FALSE;
            }
        }
    }

    /* Refresh the menu state */
    UpdateVdmMenuDisks();

    /* Initialize the software callback system and register the emulator BOPs */
    InitializeInt32();
    RegisterBop(BOP_DEBUGGER  , EmulatorDebugBreakBop);
    // RegisterBop(BOP_UNSIMULATE, CpuUnsimulateBop);

    /* Initialize VDD support */
    VDDSupInitialize();

    return TRUE;
}

VOID EmulatorCleanup(VOID)
{
    DiskCtrlCleanup();

    VgaCleanup();

    /* Close the input thread handle */
    if (InputThread != NULL) CloseHandle(InputThread);
    InputThread = NULL;

    /* Close the task event */
    if (VdmTaskEvent != NULL) CloseHandle(VdmTaskEvent);
    VdmTaskEvent = NULL;

    PS2Cleanup();

    SpeakerCleanup();
    CmosCleanup();
    // PitCleanup();
    // PicCleanup();

    // DmaCleanup();

    CpuCleanup();
    MemCleanup();
}



VOID
WINAPI
VDDSimulate16(VOID)
{
    CpuSimulate();
}

VOID
WINAPI
VDDTerminateVDM(VOID)
{
    /* Stop the VDM */
    EmulatorTerminate();
}

/* EOF */
