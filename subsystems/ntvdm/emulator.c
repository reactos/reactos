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
#include "bios.h"
#include "bop.h"
#include "vddsup.h"
#include "io.h"
#include "registers.h"
#include "vga.h"
#include "pic.h"

// HACK
typedef INT VDM_MODE;

/* PRIVATE VARIABLES **********************************************************/

FAST486_STATE EmulatorContext;

static BOOLEAN A20Line = FALSE;

/* BOP Identifiers */
#define BOP_DEBUGGER    0x56    // Break into the debugger from a 16-bit app

/* PRIVATE FUNCTIONS **********************************************************/

VOID WINAPI EmulatorReadMemory(PFAST486_STATE State, ULONG Address, PVOID Buffer, ULONG Size)
{
    UNREFERENCED_PARAMETER(State);

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
        LPBYTE DestBuffer = (LPBYTE)((ULONG_PTR)BaseAddress + VgaAddress);

        /* Read from the VGA memory */
        VgaReadMemory(VgaAddress, DestBuffer, ActualSize);
    }

    /* Read the data from the virtual address space and store it in the buffer */
    RtlCopyMemory(Buffer, (LPVOID)((ULONG_PTR)BaseAddress + Address), Size);
}

VOID WINAPI EmulatorWriteMemory(PFAST486_STATE State, ULONG Address, PVOID Buffer, ULONG Size)
{
    UNREFERENCED_PARAMETER(State);

    /* If the A20 line is disabled, mask bit 20 */
    if (!A20Line) Address &= ~(1 << 20);

    /* Make sure the requested address is valid */
    if ((Address + Size) >= MAX_ADDRESS) return;

    /* Make sure we don't write to the ROM area */
    if ((Address + Size) >= ROM_AREA_START && (Address < ROM_AREA_END)) return;

    /* Read the data from the buffer and store it in the virtual address space */
    RtlCopyMemory((LPVOID)((ULONG_PTR)BaseAddress + Address), Buffer, Size);

    /*
     * Check if we modified the VGA memory.
     */
    if (((Address + Size) >= VgaGetVideoBaseAddress())
        && (Address < VgaGetVideoLimitAddress()))
    {
        DWORD VgaAddress = max(Address, VgaGetVideoBaseAddress());
        DWORD ActualSize = min(Address + Size - 1, VgaGetVideoLimitAddress())
                           - VgaAddress + 1;
        LPBYTE SrcBuffer = (LPBYTE)((ULONG_PTR)BaseAddress + VgaAddress);

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

VOID WINAPI EmulatorDebugBreak(LPWORD Stack)
{
    DPRINT1("NTVDM: BOP_DEBUGGER\n");
    DebugBreak();
}

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN EmulatorInitialize(VOID)
{
    /* Allocate memory for the 16-bit address space */
    BaseAddress = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_ADDRESS);
    if (BaseAddress == NULL) return FALSE;

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

    /* Enable interrupts */
    setIF(1);

    /* Initialize VDD support */
    VDDSupInitialize();

    /* Register the DebugBreak BOP */
    RegisterBop(BOP_DEBUGGER, EmulatorDebugBreak);

    return TRUE;
}

VOID EmulatorCleanup(VOID)
{
    /* Free the memory allocated for the 16-bit address space */
    if (BaseAddress != NULL) HeapFree(GetProcessHeap(), 0, BaseAddress);
}

// FIXME: This function assumes 16-bit mode!!!
VOID EmulatorExecute(WORD Segment, WORD Offset)
{
    /* Tell Fast486 to move the instruction pointer */
    Fast486ExecuteAt(&EmulatorContext, Segment, Offset);
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

VOID EmulatorStep(VOID)
{
    /* Dump the state for debugging purposes */
    // Fast486DumpState(&EmulatorContext);

    /* Execute the next instruction */
    Fast486StepInto(&EmulatorContext);
}

VOID EmulatorSetA20(BOOLEAN Enabled)
{
    A20Line = Enabled;
}



VOID
WINAPI
VDDTerminateVDM(VOID)
{
    /* Stop the VDM */
    VdmRunning = FALSE;
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
