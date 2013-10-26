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
#include "dos.h"
#include "vga.h"
#include "pic.h"
#include "ps2.h"
#include "timer.h"

/* PRIVATE VARIABLES **********************************************************/

FAST486_STATE EmulatorContext;

static BOOLEAN A20Line = FALSE;

/* PRIVATE FUNCTIONS **********************************************************/

static VOID WINAPI EmulatorReadMemory(PVOID Context, UINT Address, LPBYTE Buffer, INT Size)
{
    UNREFERENCED_PARAMETER(Context);

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
        LPBYTE VgaBuffer = &Buffer[VgaAddress - Address];

        /* Read from the VGA memory */
        VgaReadMemory(VgaAddress, VgaBuffer, Size);
    }
}

static VOID WINAPI EmulatorWriteMemory(PVOID Context, UINT Address, LPBYTE Buffer, INT Size)
{
    UNREFERENCED_PARAMETER(Context);

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
        LPBYTE VgaBuffer = &Buffer[VgaAddress - Address];

        /* Write to the VGA memory */
        VgaWriteMemory(VgaAddress, VgaBuffer, Size);
    }
}

static VOID WINAPI EmulatorReadIo(PVOID Context, UINT Address, LPBYTE Buffer, INT Size)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Size);

    switch (Address)
    {
        case PIC_MASTER_CMD:
        case PIC_SLAVE_CMD:
        {
            *Buffer = PicReadCommand(Address);
            break;
        }

        case PIC_MASTER_DATA:
        case PIC_SLAVE_DATA:
        {
            *Buffer = PicReadData(Address);
            break;
        }

        case PIT_DATA_PORT(0):
        case PIT_DATA_PORT(1):
        case PIT_DATA_PORT(2):
        {
            *Buffer = PitReadData(Address - PIT_DATA_PORT(0));
            break;
        }

        case PS2_CONTROL_PORT:
        {
            *Buffer = KeyboardReadStatus();
            break;
        }

        case PS2_DATA_PORT:
        {
            *Buffer = KeyboardReadData();
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
            *Buffer = VgaReadPort(Address);
            break;
        }

        default:
        {
            DPRINT1("Read from unknown port: 0x%X\n", Address);
        }
    }
}

static VOID WINAPI EmulatorWriteIo(PVOID Context, UINT Address, LPBYTE Buffer, INT Size)
{
    BYTE Byte = *Buffer;

    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Size);

    switch (Address)
    {
        case PIT_COMMAND_PORT:
        {
            PitWriteCommand(Byte);
            break;
        }

        case PIT_DATA_PORT(0):
        case PIT_DATA_PORT(1):
        case PIT_DATA_PORT(2):
        {
            PitWriteData(Address - PIT_DATA_PORT(0), Byte);
            break;
        }

        case PIC_MASTER_CMD:
        case PIC_SLAVE_CMD:
        {
            PicWriteCommand(Address, Byte);
            break;
        }

        case PIC_MASTER_DATA:
        case PIC_SLAVE_DATA:
        {
            PicWriteData(Address, Byte);
            break;
        }

        case PS2_CONTROL_PORT:
        {
            KeyboardWriteCommand(Byte);
            break;
        }

        case PS2_DATA_PORT:
        {
            KeyboardWriteData(Byte);
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
            VgaWritePort(Address, Byte);
            break;
        }

        default:
        {
            DPRINT1("Write to unknown port: 0x%X\n", Address);
        }
    }
}

static VOID WINAPI EmulatorBiosOperation(PFAST486_STATE State, WORD Code)
{
    WORD StackSegment, StackPointer, CodeSegment, InstructionPointer;
    BYTE IntNum;
    LPWORD Stack;

    /* Get the SS:SP */
    StackSegment = State->SegmentRegs[FAST486_REG_SS].Selector;
    StackPointer = State->GeneralRegs[FAST486_REG_ESP].LowWord;

    /* Get the stack */
    Stack = (LPWORD)((ULONG_PTR)BaseAddress + TO_LINEAR(StackSegment, StackPointer));

    if (Code == EMULATOR_INT_BOP)
    {
        /* Get the interrupt number */
        IntNum = LOBYTE(Stack[STACK_INT_NUM]);

        /* Get the CS:IP */
        InstructionPointer = Stack[STACK_IP];
        CodeSegment = Stack[STACK_CS];

        /* Check if this was an exception */
        if (IntNum < 8)
        {
            /* Display a message to the user */
            DisplayMessage(L"Exception: %s occured at %04X:%04X",
                           ExceptionName[IntNum],
                           CodeSegment,
                           InstructionPointer);

            /* Stop the VDM */
            VdmRunning = FALSE;
            return;
        }

        /* Check if this was an PIC IRQ */
        if (IntNum >= BIOS_PIC_MASTER_INT && IntNum < BIOS_PIC_MASTER_INT + 8)
        {
            /* It was an IRQ from the master PIC */
            BiosHandleIrq(IntNum - BIOS_PIC_MASTER_INT, Stack);
            return;
        }
        else if (IntNum >= BIOS_PIC_SLAVE_INT && IntNum < BIOS_PIC_SLAVE_INT + 8)
        {
            /* It was an IRQ from the slave PIC */
            BiosHandleIrq(IntNum - BIOS_PIC_SLAVE_INT + 8, Stack);
            return;
        }

        switch (IntNum)
        {
            case BIOS_VIDEO_INTERRUPT:
            {
                /* This is the video BIOS interrupt, call the BIOS */
                BiosVideoService(Stack);
                break;
            }
            case BIOS_EQUIPMENT_INTERRUPT:
            {
                /* This is the BIOS "get equipment" command, call the BIOS */
                BiosEquipmentService(Stack);
                break;
            }
            case BIOS_KBD_INTERRUPT:
            {
                /* This is the keyboard BIOS interrupt, call the BIOS */
                BiosKeyboardService(Stack);
                break;
            }
            case BIOS_TIME_INTERRUPT:
            {
                /* This is the time BIOS interrupt, call the BIOS */
                BiosTimeService(Stack);
                break;
            }
            case BIOS_SYS_TIMER_INTERRUPT:
            {
                /* BIOS timer update */
                BiosSystemTimerInterrupt(Stack);
                break;
            }
            case 0x20:
            {
                DosInt20h(Stack);
                break;
            }
            case 0x21:
            {
                DosInt21h(Stack);
                break;
            }
            case 0x23:
            {
                DosBreakInterrupt(Stack);
                break;
            }
            default:
            {
                DPRINT1("Unhandled interrupt: 0x%02X\n", IntNum);
                break;
            }
        }
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN EmulatorInitialize()
{
    /* Allocate memory for the 16-bit address space */
    BaseAddress = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_ADDRESS);
    if (BaseAddress == NULL) return FALSE;

    /* Set the callbacks */
    EmulatorContext.MemReadCallback = (FAST486_MEM_READ_PROC)EmulatorReadMemory;
    EmulatorContext.MemWriteCallback = (FAST486_MEM_WRITE_PROC)EmulatorWriteMemory;
    EmulatorContext.IoReadCallback = (FAST486_IO_READ_PROC)EmulatorReadIo;
    EmulatorContext.IoWriteCallback = (FAST486_IO_WRITE_PROC)EmulatorWriteIo;
    EmulatorContext.BopCallback = (FAST486_BOP_PROC)EmulatorBiosOperation;

    /* Reset the CPU */
    Fast486Reset(&EmulatorContext);

    /* Enable interrupts */
    EmulatorSetFlag(EMULATOR_FLAG_IF);

    return TRUE;
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

VOID EmulatorExternalInterrupt(BYTE Number)
{
    /* Call the Fast486 API */
    Fast486Interrupt(&EmulatorContext, Number);
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

ULONG EmulatorGetProgramCounter(VOID)
{
    return EmulatorContext.InstPtr.Long;
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

VOID EmulatorCleanup(VOID)
{
    /* Free the memory allocated for the 16-bit address space */
    if (BaseAddress != NULL) HeapFree(GetProcessHeap(), 0, BaseAddress);
}

VOID EmulatorSetA20(BOOLEAN Enabled)
{
    A20Line = Enabled;
}

/* EOF */
