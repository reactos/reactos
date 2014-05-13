/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            emulator.c
 * PURPOSE:         Minimal x86 machine emulator for the VDM
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "callback.h"

#include "clock.h"
#include "bios/rom.h"
#include "hardware/cmos.h"
#include "hardware/pic.h"
#include "hardware/ps2.h"
#include "hardware/speaker.h"
#include "hardware/timer.h"
#include "hardware/vga.h"

#include "bop.h"
#include "vddsup.h"
#include "io.h"

#include <isvbop.h>

/* PRIVATE VARIABLES **********************************************************/

FAST486_STATE EmulatorContext;
BOOLEAN CpuSimulate = FALSE;

/* No more than 'MaxCpuCallLevel' recursive CPU calls are allowed */
static const INT MaxCpuCallLevel = 32;
static INT CpuCallLevel = 0;

LPVOID  BaseAddress = NULL;
BOOLEAN VdmRunning  = TRUE;

static BOOLEAN A20Line   = FALSE;
static BYTE Port61hState = 0x00;

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

VOID WINAPI EmulatorReadMemory(PFAST486_STATE State, ULONG Address, PVOID Buffer, ULONG Size)
{
    UNREFERENCED_PARAMETER(State);

    // BIG HACK!!!! To make BIOS images working correctly,
    // until Aleksander rewrites memory management!!
    if (Address >= 0xFFFFFFF0) Address -= 0xFFF00000;

    /* If the A20 line is disabled, mask bit 20 */
    if (!A20Line) Address &= ~(1 << 20);

    /* Make sure the requested address is valid */
    if ((Address + Size) >= MAX_ADDRESS) return;

    /*
     * Check if we are going to read the VGA memory and
     * copy it into the virtual address space if needed.
     */
    if (((Address + Size) >= VgaGetVideoBaseAddress())
        && (Address < VgaGetVideoLimitAddress()))
    {
        DWORD VgaAddress = max(Address, VgaGetVideoBaseAddress());
        DWORD ActualSize = min(Address + Size - 1, VgaGetVideoLimitAddress())
                           - VgaAddress + 1;
        LPBYTE DestBuffer = (LPBYTE)REAL_TO_PHYS(VgaAddress);

        /* Read from the VGA memory */
        VgaReadMemory(VgaAddress, DestBuffer, ActualSize);
    }

    /* Read the data from the virtual address space and store it in the buffer */
    RtlCopyMemory(Buffer, REAL_TO_PHYS(Address), Size);
}

VOID WINAPI EmulatorWriteMemory(PFAST486_STATE State, ULONG Address, PVOID Buffer, ULONG Size)
{
    UNREFERENCED_PARAMETER(State);

    // BIG HACK!!!! To make BIOS images working correctly,
    // until Aleksander rewrites memory management!!
    if (Address >= 0xFFFFFFF0) Address -= 0xFFF00000;

    /* If the A20 line is disabled, mask bit 20 */
    if (!A20Line) Address &= ~(1 << 20);

    /* Make sure the requested address is valid */
    if ((Address + Size) >= MAX_ADDRESS) return;

    /* Make sure we don't write to the ROM area */
    if ((Address + Size) >= ROM_AREA_START && (Address < ROM_AREA_END)) return;

    /* Read the data from the buffer and store it in the virtual address space */
    RtlCopyMemory(REAL_TO_PHYS(Address), Buffer, Size);

    /*
     * Check if we modified the VGA memory.
     */
    if (((Address + Size) >= VgaGetVideoBaseAddress())
        && (Address < VgaGetVideoLimitAddress()))
    {
        DWORD VgaAddress = max(Address, VgaGetVideoBaseAddress());
        DWORD ActualSize = min(Address + Size - 1, VgaGetVideoLimitAddress())
                           - VgaAddress + 1;
        LPBYTE SrcBuffer = (LPBYTE)REAL_TO_PHYS(VgaAddress);

        /* Write to the VGA memory */
        VgaWriteMemory(VgaAddress, SrcBuffer, ActualSize);
    }
}

UCHAR WINAPI EmulatorIntAcknowledge(PFAST486_STATE State)
{
    UNREFERENCED_PARAMETER(State);

    /* Get the interrupt number from the PIC */
    return PicGetInterrupt();
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
    DisplayMessage(L"Exception: %s occured at %04X:%04X\n"
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

    /* Stop the VDM */
    EmulatorTerminate();
    return;
}

// FIXME: This function assumes 16-bit mode!!!
VOID EmulatorExecute(WORD Segment, WORD Offset)
{
    /* Tell Fast486 to move the instruction pointer */
    Fast486ExecuteAt(&EmulatorContext, Segment, Offset);
}

VOID EmulatorStep(VOID)
{
    /* Dump the state for debugging purposes */
    // Fast486DumpState(&EmulatorContext);

    /* Execute the next instruction */
    Fast486StepInto(&EmulatorContext);
}

VOID EmulatorSimulate(VOID)
{
    if (CpuCallLevel > MaxCpuCallLevel)
    {
        DisplayMessage(L"Too many CPU levels of recursion (%d, expected maximum %d)",
                       CpuCallLevel, MaxCpuCallLevel);

        /* Stop the VDM */
        EmulatorTerminate();
        return;
    }
    CpuCallLevel++;

    CpuSimulate = TRUE;
    while (VdmRunning && CpuSimulate) ClockUpdate();

    CpuCallLevel--;
    if (CpuCallLevel < 0) CpuCallLevel = 0;

    /* This takes into account for reentrance */
    CpuSimulate = TRUE;
}

VOID EmulatorUnsimulate(VOID)
{
    /* Stop simulation */
    CpuSimulate = FALSE;
}

VOID EmulatorTerminate(VOID)
{
    /* Stop the VDM */
    VdmRunning = FALSE;
}

VOID EmulatorInterrupt(BYTE Number)
{
    /* Call the Fast486 API */
    Fast486Interrupt(&EmulatorContext, Number);
}

VOID EmulatorInterruptSignal(VOID)
{
    /* Call the Fast486 API */
    Fast486InterruptSignal(&EmulatorContext);
}

VOID EmulatorSetA20(BOOLEAN Enabled)
{
    A20Line = Enabled;
}

static VOID WINAPI EmulatorDebugBreakBop(LPWORD Stack)
{
    DPRINT1("NTVDM: BOP_DEBUGGER\n");
    DebugBreak();
}

static VOID WINAPI EmulatorUnsimulateBop(LPWORD Stack)
{
    EmulatorUnsimulate();
}

static BYTE WINAPI Port61hRead(ULONG Port)
{
    return Port61hState;
}

static VOID WINAPI Port61hWrite(ULONG Port, BYTE Data)
{
    // BOOLEAN SpeakerChange = FALSE;
    BYTE OldPort61hState = Port61hState;

    /* Only the four lowest bytes can be written */
    Port61hState = (Port61hState & 0xF0) | (Data & 0x0F);

    if ((OldPort61hState ^ Port61hState) & 0x01)
    {
        DPRINT("PIT 2 Gate %s\n", Port61hState & 0x01 ? "on" : "off");
        // SpeakerChange = TRUE;
    }

    PitSetGate(2, !!(Port61hState & 0x01));

    if ((OldPort61hState ^ Port61hState) & 0x02)
    {
        /* There were some change for the speaker... */
        DPRINT("Speaker %s\n", Port61hState & 0x02 ? "on" : "off");
        // SpeakerChange = TRUE;
    }
    // if (SpeakerChange) SpeakerChange();
    SpeakerChange();
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
    // BYTE OldPort61hState = Port61hState;

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
    DPRINT("Speaker PIT out\n");
    // if ((OldPort61hState ^ Port61hState) & 0x20)
        // SpeakerChange();
}

/* PUBLIC FUNCTIONS ***********************************************************/

DWORD WINAPI PumpConsoleInput(LPVOID Parameter);

BOOLEAN EmulatorInitialize(HANDLE ConsoleInput, HANDLE ConsoleOutput)
{
    /* Allocate memory for the 16-bit address space */
    BaseAddress = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_ADDRESS);
    if (BaseAddress == NULL)
    {
        wprintf(L"FATAL: Failed to allocate VDM memory.\n");
        return FALSE;
    }

    /* Initialize I/O ports */
    /* Initialize RAM */

    /* Initialize the internal clock */
    if (!ClockInitialize())
    {
        wprintf(L"FATAL: Failed to initialize the clock\n");
        return FALSE;
    }

    /* Initialize the CPU */
    Fast486Initialize(&EmulatorContext,
                      EmulatorReadMemory,
                      EmulatorWriteMemory,
                      EmulatorReadIo,
                      EmulatorWriteIo,
                      NULL,
                      EmulatorBiosOperation,
                      EmulatorIntAcknowledge,
                      NULL /* TODO: Use a TLB */);

    /* Initialize DMA */

    /* Initialize the PIC, the PIT, the CMOS and the PC Speaker */
    PicInitialize();
    PitInitialize();
    CmosInitialize();
    SpeakerInitialize();

    /* Set output functions */
    PitSetOutFunction(0, NULL, PitChan0Out);
    PitSetOutFunction(1, NULL, PitChan1Out);
    PitSetOutFunction(2, NULL, PitChan2Out);

    /* Register the I/O Ports */
    RegisterIoPort(CONTROL_SYSTEM_PORT61H, Port61hRead, Port61hWrite);

    /* Set the console input mode */
    // FIXME: Activate ENABLE_WINDOW_INPUT when we will want to perform actions
    // upon console window events (screen buffer resize, ...).
    SetConsoleMode(ConsoleInput, ENABLE_PROCESSED_INPUT /* | ENABLE_WINDOW_INPUT */);
    // SetConsoleMode(ConsoleOutput, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);

    /* Initialize the PS2 port */
    PS2Initialize(ConsoleInput);

    /**************** ATTACH INPUT WITH CONSOLE *****************/
    /* Start the input thread */
    InputThread = CreateThread(NULL, 0, &PumpConsoleInput, ConsoleInput, 0, NULL);
    if (InputThread == NULL)
    {
        DisplayMessage(L"Failed to create the console input thread.");
        return FALSE;
    }
    /************************************************************/

    /* Initialize the VGA */
    if (!VgaInitialize(ConsoleOutput))
    {
        DisplayMessage(L"Failed to initialize VGA support.");
        return FALSE;
    }

    /* Initialize the software callback system and register the emulator BOPs */
    InitializeCallbacks();
    RegisterBop(BOP_DEBUGGER  , EmulatorDebugBreakBop);
    RegisterBop(BOP_UNSIMULATE, EmulatorUnsimulateBop);

    /* Initialize VDD support */
    VDDSupInitialize();

    return TRUE;
}

VOID EmulatorCleanup(VOID)
{
    VgaCleanup();

    /* Close the input thread handle */
    if (InputThread != NULL) CloseHandle(InputThread);
    InputThread = NULL;

    PS2Cleanup();

    SpeakerCleanup();
    CmosCleanup();
    // PitCleanup();
    // PicCleanup();

    // Fast486Cleanup();

    /* Free the memory allocated for the 16-bit address space */
    if (BaseAddress != NULL) HeapFree(GetProcessHeap(), 0, BaseAddress);
}



VOID
WINAPI
VDDSimulate16(VOID)
{
    EmulatorSimulate();
}

VOID
WINAPI
VDDTerminateVDM(VOID)
{
    /* Stop the VDM */
    EmulatorTerminate();
}

PBYTE
WINAPI
Sim32pGetVDMPointer(IN ULONG   Address,
                    IN BOOLEAN ProtectedMode)
{
    // FIXME
    UNREFERENCED_PARAMETER(ProtectedMode);

    /*
     * HIWORD(Address) == Segment  (if ProtectedMode == FALSE)
     *                 or Selector (if ProtectedMode == TRUE )
     * LOWORD(Address) == Offset
     */
    return (PBYTE)FAR_POINTER(Address);
}

PBYTE
WINAPI
MGetVdmPointer(IN ULONG   Address,
               IN ULONG   Size,
               IN BOOLEAN ProtectedMode)
{
    UNREFERENCED_PARAMETER(Size);
    return Sim32pGetVDMPointer(Address, ProtectedMode);
}

PVOID
WINAPI
VdmMapFlat(IN USHORT   Segment,
           IN ULONG    Offset,
           IN VDM_MODE Mode)
{
    // FIXME
    UNREFERENCED_PARAMETER(Mode);

    return SEG_OFF_TO_PTR(Segment, Offset);
}

BOOL
WINAPI
VdmFlushCache(IN USHORT   Segment,
              IN ULONG    Offset,
              IN ULONG    Size,
              IN VDM_MODE Mode)
{
    // FIXME
    UNIMPLEMENTED;
    return TRUE;
}

BOOL
WINAPI
VdmUnmapFlat(IN USHORT   Segment,
             IN ULONG    Offset,
             IN PVOID    Buffer,
             IN VDM_MODE Mode)
{
    // FIXME
    UNIMPLEMENTED;
    return TRUE;
}

/* EOF */
