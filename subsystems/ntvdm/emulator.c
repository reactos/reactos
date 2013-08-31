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

#ifndef NEW_EMULATOR
softx86_ctx EmulatorContext;
softx87_ctx FpuEmulatorContext;
#else
SOFT386_STATE EmulatorContext;
#endif

static BOOLEAN A20Line = FALSE;

/* PRIVATE FUNCTIONS **********************************************************/

static VOID EmulatorReadMemory(PVOID Context, UINT Address, LPBYTE Buffer, INT Size)
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

static VOID EmulatorWriteMemory(PVOID Context, UINT Address, LPBYTE Buffer, INT Size)
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

static VOID EmulatorReadIo(PVOID Context, UINT Address, LPBYTE Buffer, INT Size)
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

static VOID EmulatorWriteIo(PVOID Context, UINT Address, LPBYTE Buffer, INT Size)
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

#ifndef NEW_EMULATOR

static VOID EmulatorBop(WORD Code)
{
    WORD StackSegment, StackPointer, CodeSegment, InstructionPointer;
    BYTE IntNum;
    LPWORD Stack;

    /* Get the SS:SP */
#ifndef NEW_EMULATOR
    StackSegment = EmulatorContext.state->segment_reg[SX86_SREG_SS].val;
    StackPointer = EmulatorContext.state->general_reg[SX86_REG_SP].val;
#else
    StackSegment = EmulatorContext.SegmentRegs[SOFT386_REG_SS].LowWord;
    StackPointer = EmulatorContext.SegmentRegs[SOFT386_REG_SP].LowWord;
#endif

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

static VOID EmulatorSoftwareInt(PVOID Context, BYTE Number)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Number);

    /* Do nothing */
}

static VOID EmulatorHardwareInt(PVOID Context, BYTE Number)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Number);

    /* Do nothing */
}

static VOID EmulatorHardwareIntAck(PVOID Context, BYTE Number)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Number);

    /* Do nothing */
}

#endif

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN EmulatorInitialize()
{
    /* Allocate memory for the 16-bit address space */
    BaseAddress = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_ADDRESS);
    if (BaseAddress == NULL) return FALSE;

#ifndef NEW_EMULATOR
    /* Initialize the softx86 CPU emulator */
    if (!softx86_init(&EmulatorContext, SX86_CPULEVEL_80286))
    {
        HeapFree(GetProcessHeap(), 0, BaseAddress);
        return FALSE;
    }

    /* Initialize the softx87 FPU emulator*/
    if(!softx87_init(&FpuEmulatorContext, SX87_FPULEVEL_8087))
    {
        softx86_free(&EmulatorContext);
        HeapFree(GetProcessHeap(), 0, BaseAddress);
        return FALSE;
    }

    /* Set memory read/write callbacks */
    EmulatorContext.callbacks->on_read_memory = EmulatorReadMemory;
    EmulatorContext.callbacks->on_write_memory = EmulatorWriteMemory;

    /* Set MMIO read/write callbacks */
    EmulatorContext.callbacks->on_read_io = EmulatorReadIo;
    EmulatorContext.callbacks->on_write_io = EmulatorWriteIo;

    /* Set interrupt callbacks */
    EmulatorContext.callbacks->on_sw_int = EmulatorSoftwareInt;
    EmulatorContext.callbacks->on_hw_int = EmulatorHardwareInt;
    EmulatorContext.callbacks->on_hw_int_ack = EmulatorHardwareIntAck;

    /* Connect the emulated FPU to the emulated CPU */
    softx87_connect_to_CPU(&EmulatorContext, &FpuEmulatorContext);
#else
    /* Set the callbacks */
    EmulatorContext.MemReadCallback = (SOFT386_MEM_READ_PROC)EmulatorReadMemory;
    EmulatorContext.MemWriteCallback = (SOFT386_MEM_WRITE_PROC)EmulatorWriteMemory;
    EmulatorContext.IoReadCallback = (SOFT386_IO_READ_PROC)EmulatorReadIo;
    EmulatorContext.IoWriteCallback = (SOFT386_IO_WRITE_PROC)EmulatorWriteIo;

    /* Reset the CPU */
    Soft386Reset(&EmulatorContext);
#endif

    /* Enable interrupts */
    EmulatorSetFlag(EMULATOR_FLAG_IF);

    return TRUE;
}

VOID EmulatorSetStack(WORD Segment, DWORD Offset)
{
#ifndef NEW_EMULATOR
    /* Call the softx86 API */
    softx86_set_stack_ptr(&EmulatorContext, Segment, Offset);
#else
    // TODO: NOT IMPLEMENTED
#endif
}

// FIXME: This function assumes 16-bit mode!!!
VOID EmulatorExecute(WORD Segment, WORD Offset)
{
#ifndef NEW_EMULATOR
    /* Call the softx86 API */
    softx86_set_instruction_ptr(&EmulatorContext, Segment, Offset);
#else
    /* Tell Soft386 to move the instruction pointer */
    Soft386ExecuteAt(&EmulatorContext, Segment, Offset);
#endif
}

VOID EmulatorInterrupt(BYTE Number)
{
    LPDWORD IntVecTable = (LPDWORD)((ULONG_PTR)BaseAddress);
    UINT Segment, Offset;

    /* Get the segment and offset */
    Segment = HIWORD(IntVecTable[Number]);
    Offset = LOWORD(IntVecTable[Number]);

#ifndef NEW_EMULATOR
    /* Call the softx86 API */
    softx86_make_simple_interrupt_call(&EmulatorContext, &Segment, &Offset);
#else
    UNREFERENCED_PARAMETER(Segment);
    UNREFERENCED_PARAMETER(Offset);
    // TODO: NOT IMPLEMENTED
#endif
}

VOID EmulatorExternalInterrupt(BYTE Number)
{
#ifndef NEW_EMULATOR
    /* Call the softx86 API */
    softx86_ext_hw_signal(&EmulatorContext, Number);
#endif
}

ULONG EmulatorGetRegister(ULONG Register)
{
#ifndef NEW_EMULATOR
    if (Register < EMULATOR_REG_ES)
    {
        return EmulatorContext.state->general_reg[Register].val;
    }
    else
    {
        return EmulatorContext.state->segment_reg[Register - EMULATOR_REG_ES].val;
    }
#else
    if (Register < EMULATOR_REG_ES)
    {
        return EmulatorContext.GeneralRegs[Register].Long;
    }
    else
    {
        return EmulatorContext.SegmentRegs[Register - EMULATOR_REG_ES].Selector;
    }
#endif
}

ULONG EmulatorGetProgramCounter(VOID)
{
#ifndef NEW_EMULATOR
    return EmulatorContext.state->reg_ip;
#else
    return EmulatorContext.InstPtr.Long;
#endif
}

VOID EmulatorSetRegister(ULONG Register, ULONG Value)
{
#ifndef NEW_EMULATOR
    if (Register < EMULATOR_REG_CS)
    {
        EmulatorContext.state->general_reg[Register].val = Value;
    }
    else
    {
        EmulatorContext.state->segment_reg[Register - EMULATOR_REG_ES].val = (WORD)Value;
    }
#else
    // TODO: NOT IMPLEMENTED
#endif
}

BOOLEAN EmulatorGetFlag(ULONG Flag)
{
#ifndef NEW_EMULATOR
    return (EmulatorContext.state->reg_flags.val & Flag) ? TRUE : FALSE;
#else
    return (EmulatorContext.Flags.Long & Flag) ? TRUE : FALSE;
#endif
}

VOID EmulatorSetFlag(ULONG Flag)
{
#ifndef NEW_EMULATOR
    EmulatorContext.state->reg_flags.val |= Flag;
#else
    EmulatorContext.Flags.Long |= Flag;
#endif
}

VOID EmulatorClearFlag(ULONG Flag)
{
#ifndef NEW_EMULATOR
    EmulatorContext.state->reg_flags.val &= ~Flag;
#else
    EmulatorContext.Flags.Long &= ~Flag;
#endif
}

VOID EmulatorStep(VOID)
{
#ifndef NEW_EMULATOR
    LPWORD Instruction;

    /* Print the current position - useful for debugging */
    DPRINT("Executing at CS:IP = %04X:%04X\n",
           EmulatorGetRegister(EMULATOR_REG_CS),
           EmulatorContext.state->reg_ip);

    Instruction = (LPWORD)((ULONG_PTR)BaseAddress
                           + TO_LINEAR(EmulatorGetRegister(EMULATOR_REG_CS),
                           EmulatorContext.state->reg_ip));

    /* Check for the BIOS operation (BOP) sequence */
    if (Instruction[0] == EMULATOR_BOP)
    {
        /* Skip the opcodes */
        EmulatorContext.state->reg_ip += 4;

        // HACK: Refresh the display because the called function may wait.
        VgaRefreshDisplay();

        /* Call the BOP handler */
        EmulatorBop(Instruction[1]);
    }

    /* Call the softx86 API */
    if (!softx86_step(&EmulatorContext))
    {
        /* Invalid opcode */
        EmulatorInterrupt(EMULATOR_EXCEPTION_INVALID_OPCODE);
    }
#else
    /* Dump the state for debugging purposes */
    Soft386DumpState(&EmulatorContext);

    /* Execute the next instruction */
    Soft386StepInto(&EmulatorContext);
#endif
}

VOID EmulatorCleanup(VOID)
{
#ifndef NEW_EMULATOR
    /* Free the softx86 CPU and FPU emulator */
    softx87_free(&FpuEmulatorContext);
    softx86_free(&EmulatorContext);
#endif

    /* Free the memory allocated for the 16-bit address space */
    if (BaseAddress != NULL) HeapFree(GetProcessHeap(), 0, BaseAddress);
}

VOID EmulatorSetA20(BOOLEAN Enabled)
{
    A20Line = Enabled;
}

/* EOF */
