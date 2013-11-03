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
#include "dos.h"
#include "vga.h"
#include "pic.h"
#include "ps2.h"
#include "timer.h"

/* PRIVATE VARIABLES **********************************************************/

FAST486_STATE EmulatorContext;

static BOOLEAN A20Line = FALSE;

/* PRIVATE FUNCTIONS **********************************************************/

static VOID WINAPI EmulatorReadMemory(PFAST486_STATE State, ULONG Address, PVOID Buffer, ULONG Size)
{
    UNREFERENCED_PARAMETER(State);

    /* If the A20 line is disabled, mask bit 20 */
    if (!A20Line) Address &= ~(1 << 20);

    /* Make sure the requested address is valid */
    if ((Address + Size) >= MAX_ADDRESS) return;

    /* Read the data from the virtual address space and store it in the buffer */
    RtlCopyMemory(Buffer, (LPVOID)((ULONG_PTR)BaseAddress + Address), Size);

    /* Check if we modified the console video memory */
    if (((Address + Size) >= VgaGetVideoBaseAddress())
        && (Address < VgaGetVideoLimitAddress()))
    {
        DWORD VgaAddress = max(Address, VgaGetVideoBaseAddress());
        LPBYTE VgaBuffer = (LPBYTE)((ULONG_PTR)Buffer + VgaAddress - Address);

        /* Read from the VGA memory */
        VgaReadMemory(VgaAddress, VgaBuffer, Size);
    }
}

static VOID WINAPI EmulatorWriteMemory(PFAST486_STATE State, ULONG Address, PVOID Buffer, ULONG Size)
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

    /* Check if we modified the console video memory */
    if (((Address + Size) >= VgaGetVideoBaseAddress())
        && (Address < VgaGetVideoLimitAddress()))
    {
        DWORD VgaAddress = max(Address, VgaGetVideoBaseAddress());
        LPBYTE VgaBuffer = (LPBYTE)((ULONG_PTR)Buffer + VgaAddress - Address);

        /* Write to the VGA memory */
        VgaWriteMemory(VgaAddress, VgaBuffer, Size);
    }
}

static VOID WINAPI EmulatorReadIo(PFAST486_STATE State, ULONG Port, PVOID Buffer, ULONG Size)
{
    INT i;
    LPBYTE Address = (LPBYTE)Buffer;

    UNREFERENCED_PARAMETER(State);

    for (i = 0; i < Size; i++)
    {
        switch (Port)
        {
            case PIC_MASTER_CMD:
            case PIC_SLAVE_CMD:
            {
                *(Address++) = PicReadCommand(Port);
                break;
            }

            case PIC_MASTER_DATA:
            case PIC_SLAVE_DATA:
            {
                *(Address++) = PicReadData(Port);
                break;
            }

            case PIT_DATA_PORT(0):
            case PIT_DATA_PORT(1):
            case PIT_DATA_PORT(2):
            {
                *(Address++) = PitReadData(Port - PIT_DATA_PORT(0));
                break;
            }

            case PS2_CONTROL_PORT:
            {
                *(Address++) = KeyboardReadStatus();
                break;
            }

            case PS2_DATA_PORT:
            {
                *(Address++) = KeyboardReadData();
                break;
            }

            case VGA_AC_WRITE:
            case VGA_AC_READ:
            case VGA_SEQ_INDEX:
            case VGA_SEQ_DATA:
            case VGA_DAC_READ_INDEX:
            case VGA_DAC_WRITE_INDEX:
            case VGA_DAC_DATA:
            case VGA_MISC_READ:
            case VGA_MISC_WRITE:
            case VGA_CRTC_INDEX:
            case VGA_CRTC_DATA:
            case VGA_GC_INDEX:
            case VGA_GC_DATA:
            case VGA_STAT_MONO:
            case VGA_STAT_COLOR:
            {
                *(Address++) = VgaReadPort(Port);
                break;
            }

            default:
            {
                DPRINT1("Read from unknown port: 0x%X\n", Port);
            }
        }
    }
}

static VOID WINAPI EmulatorWriteIo(PFAST486_STATE State, ULONG Port, PVOID Buffer, ULONG Size)
{
    INT i;
    LPBYTE Address = (LPBYTE)Buffer;

    UNREFERENCED_PARAMETER(State);

    for (i = 0; i < Size; i++)
    {
        switch (Port)
        {
            case PIT_COMMAND_PORT:
            {
                PitWriteCommand(*(Address++));
                break;
            }

            case PIT_DATA_PORT(0):
            case PIT_DATA_PORT(1):
            case PIT_DATA_PORT(2):
            {
                PitWriteData(Port - PIT_DATA_PORT(0), *(Address++));
                break;
            }

            case PIC_MASTER_CMD:
            case PIC_SLAVE_CMD:
            {
                PicWriteCommand(Port, *(Address++));
                break;
            }

            case PIC_MASTER_DATA:
            case PIC_SLAVE_DATA:
            {
                PicWriteData(Port, *(Address++));
                break;
            }

            case PS2_CONTROL_PORT:
            {
                KeyboardWriteCommand(*(Address++));
                break;
            }

            case PS2_DATA_PORT:
            {
                KeyboardWriteData(*(Address++));
                break;
            }

            case VGA_AC_WRITE:
            case VGA_AC_READ:
            case VGA_SEQ_INDEX:
            case VGA_SEQ_DATA:
            case VGA_DAC_READ_INDEX:
            case VGA_DAC_WRITE_INDEX:
            case VGA_DAC_DATA:
            case VGA_MISC_READ:
            case VGA_MISC_WRITE:
            case VGA_CRTC_INDEX:
            case VGA_CRTC_DATA:
            case VGA_GC_INDEX:
            case VGA_GC_DATA:
            case VGA_STAT_MONO:
            case VGA_STAT_COLOR:
            {
                VgaWritePort(Port, *(Address++));
                break;
            }

            default:
            {
                DPRINT1("Write to unknown port: 0x%X\n", Port);
            }
        }
    }
}

static VOID WINAPI EmulatorBiosOperation(PFAST486_STATE State, UCHAR BopCode)
{
    WORD StackSegment, StackPointer;
    LPWORD Stack;

    /* Get the SS:SP */
    StackSegment = State->SegmentRegs[FAST486_REG_SS].Selector;
    StackPointer = State->GeneralRegs[FAST486_REG_ESP].LowWord;

    /* Get the stack */
    Stack = (LPWORD)SEG_OFF_TO_PTR(StackSegment, StackPointer);

    if (BopProc[BopCode] != NULL)
        BopProc[BopCode](Stack);
    else
        DPRINT1("Invalid BOP code %u\n", BopCode);
}

static UCHAR WINAPI EmulatorIntAcknowledge(PFAST486_STATE State)
{
    UNREFERENCED_PARAMETER(State);

    /* Get the interrupt number from the PIC */
    return PicGetInterrupt();
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
                      EmulatorIntAcknowledge);

    /* Enable interrupts */
    EmulatorSetFlag(EMULATOR_FLAG_IF);

    return TRUE;
}

VOID EmulatorCleanup(VOID)
{
    /* Free the memory allocated for the 16-bit address space */
    if (BaseAddress != NULL) HeapFree(GetProcessHeap(), 0, BaseAddress);
}

VOID EmulatorSetStack(WORD Segment, DWORD Offset)
{
    Fast486SetStack(&EmulatorContext, Segment, Offset);
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

ULONG EmulatorGetRegister(ULONG Register)
{
    if (Register < EMULATOR_REG_ES)
    {
        return EmulatorContext.GeneralRegs[Register].Long;
    }
    else
    {
        return EmulatorContext.SegmentRegs[Register - EMULATOR_REG_ES].Selector;
    }
}

VOID EmulatorSetRegister(ULONG Register, ULONG Value)
{
    if (Register < EMULATOR_REG_ES)
    {
        EmulatorContext.GeneralRegs[Register].Long = Value;
    }
    else
    {
        Fast486SetSegment(&EmulatorContext, Register - EMULATOR_REG_ES, (USHORT)Value);
    }
}

ULONG EmulatorGetProgramCounter(VOID)
{
    return EmulatorContext.InstPtr.Long;
}

BOOLEAN EmulatorGetFlag(ULONG Flag)
{
    return (EmulatorContext.Flags.Long & Flag) ? TRUE : FALSE;
}

VOID EmulatorSetFlag(ULONG Flag)
{
    EmulatorContext.Flags.Long |= Flag;
}

VOID EmulatorClearFlag(ULONG Flag)
{
    EmulatorContext.Flags.Long &= ~Flag;
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

/* EOF */
