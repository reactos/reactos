/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            emulator.c
 * PURPOSE:         Minimal x86 machine emulator for the VDM
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#include "emulator.h"
#include "bios.h"
#include "dos.h"
#include "pic.h"
#include "ps2.h"
#include "timer.h"

/* PRIVATE VARIABLES **********************************************************/

#ifndef NEW_EMULATOR
softx86_ctx EmulatorContext;
softx87_ctx FpuEmulatorContext;
#else
EMULATOR_CONTEXT EmulatorContext;
#endif

static BOOLEAN A20Line = FALSE;

/* PRIVATE FUNCTIONS **********************************************************/

#ifndef NEW_EMULATOR

static VOID EmulatorReadMemory(PVOID Context, UINT Address, LPBYTE Buffer, INT Size)
{
    /* If the A20 line is disabled, mask bit 20 */
    if (!A20Line) Address &= ~(1 << 20);

    /* Make sure the requested address is valid */
    if ((Address + Size) >= MAX_ADDRESS) return;

    /* Are we reading some of the console video memory? */
    if (((Address + Size) >= BiosGetVideoMemoryStart())
        && (Address < CONSOLE_VIDEO_MEM_END))
    {
        /* Call the VDM BIOS to update the video memory */
        BiosUpdateVideoMemory(max(Address, BiosGetVideoMemoryStart()),
                              min(Address + Size, CONSOLE_VIDEO_MEM_END));
    }

    /* Read the data from the virtual address space and store it in the buffer */
    RtlCopyMemory(Buffer, (LPVOID)((ULONG_PTR)BaseAddress + Address), Size);
}

static VOID EmulatorWriteMemory(PVOID Context, UINT Address, LPBYTE Buffer, INT Size)
{
    /* If the A20 line is disabled, mask bit 20 */
    if (!A20Line) Address &= ~(1 << 20);

    /* Make sure the requested address is valid */
    if ((Address + Size) >= MAX_ADDRESS) return;

    /* Make sure we don't write to the ROM area */
    if ((Address + Size) >= ROM_AREA_START && (Address < ROM_AREA_END)) return;

    /* Read the data from the buffer and store it in the virtual address space */
    RtlCopyMemory((LPVOID)((ULONG_PTR)BaseAddress + Address), Buffer, Size);

    /* Check if we modified the console video memory */
    if (((Address + Size) >= BiosGetVideoMemoryStart())
        && (Address < CONSOLE_VIDEO_MEM_END))
    {
        /* Call the VDM BIOS to update the screen */
        BiosUpdateConsole(max(Address, BiosGetVideoMemoryStart()),
                          min(Address + Size, CONSOLE_VIDEO_MEM_END));
    }
}

static VOID EmulatorReadIo(PVOID Context, UINT Address, LPBYTE Buffer, INT Size)
{
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
    }
}

static VOID EmulatorWriteIo(PVOID Context, UINT Address, LPBYTE Buffer, INT Size)
{
    BYTE Byte = *Buffer;

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
    }
}

static VOID EmulatorSoftwareInt(PVOID Context, BYTE Number)
{
    WORD StackSegment, StackPointer, CodeSegment, InstructionPointer;
    BYTE IntNum;

    /* Check if this is the special interrupt */
    if (Number == SPECIAL_INT_NUM)
    {
        /* Get the SS:SP */
#ifndef NEW_EMULATOR
        StackSegment = EmulatorContext.state->segment_reg[SX86_SREG_SS].val;
        StackPointer = EmulatorContext.state->general_reg[SX86_REG_SP].val;
#else
        StackSegment = EmulatorContext.Registers[EMULATOR_REG_SS].LowWord;
        StackPointer = EmulatorContext.Registers[EMULATOR_REG_SP].LowWord;
#endif

        /* Get the interrupt number */
        IntNum = *(LPBYTE)((ULONG_PTR)BaseAddress + TO_LINEAR(StackSegment, StackPointer));

        /* Move the stack pointer forward one word to skip the interrupt number */
        StackPointer += sizeof(WORD);

        /* Get the CS:IP */
        InstructionPointer = *(LPWORD)((ULONG_PTR)BaseAddress
                             + TO_LINEAR(StackSegment, StackPointer));
        CodeSegment = *(LPWORD)((ULONG_PTR)BaseAddress
                      + TO_LINEAR(StackSegment, StackPointer + sizeof(WORD)));

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
            BiosHandleIrq(IntNum - BIOS_PIC_MASTER_INT);
            return;
        }
        else if (IntNum >= BIOS_PIC_SLAVE_INT && IntNum < BIOS_PIC_SLAVE_INT + 8)
        {
            /* It was an IRQ from the slave PIC */
            BiosHandleIrq(IntNum - BIOS_PIC_SLAVE_INT + 8);
            return;
        }

        switch (IntNum)
        {
            case BIOS_VIDEO_INTERRUPT:
            {
                /* This is the video BIOS interrupt, call the BIOS */
                BiosVideoService();
                break;
            }
            case BIOS_EQUIPMENT_INTERRUPT:
            {
                /* This is the BIOS "get equipment" command, call the BIOS */
                BiosEquipmentService();
                break;
            }
            case BIOS_KBD_INTERRUPT:
            {
                /* This is the keyboard BIOS interrupt, call the BIOS */
                BiosKeyboardService();
                break;
            }
            case BIOS_TIME_INTERRUPT:
            {
                /* This is the time BIOS interrupt, call the BIOS */
                BiosTimeService();
                break;
            }
            case BIOS_SYS_TIMER_INTERRUPT:
            {
                /* BIOS timer update */
                BiosSystemTimerInterrupt();
                break;
            }
            case 0x20:
            {
                DosInt20h(CodeSegment);
                break;
            }
            case 0x21:
            {
                DosInt21h(CodeSegment);
                break;
            }
            case 0x23:
            {
                DosBreakInterrupt();
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

static VOID EmulatorHardwareInt(PVOID Context, BYTE Number)
{
    /* Do nothing */
}

static VOID EmulatorHardwareIntAck(PVOID Context, BYTE Number)
{
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
    // TODO: NOT IMPLEMENTED
#endif

    /* Enable interrupts */
    EmulatorSetFlag(EMULATOR_FLAG_IF);

    return TRUE;
}

VOID EmulatorSetStack(WORD Segment, WORD Offset)
{
#ifndef NEW_EMULATOR
    /* Call the softx86 API */
    softx86_set_stack_ptr(&EmulatorContext, Segment, Offset);
#else
    // TODO: NOT IMPLEMENTED
#endif
}

VOID EmulatorExecute(WORD Segment, WORD Offset)
{
#ifndef NEW_EMULATOR
    /* Call the softx86 API */
    softx86_set_instruction_ptr(&EmulatorContext, Segment, Offset);
#else
    // TODO: NOT IMPLEMENTED
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
    return EmulatorContext.Registers[Register].Long;
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
        EmulatorContext.state->segment_reg[Register - EMULATOR_REG_ES].val = Value;
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

VOID EmulatorStep()
{
#ifndef NEW_EMULATOR
    /* Print the current position - useful for debugging */
    DPRINT("Executing at CS:IP = %04X:%04X\n",
           EmulatorGetRegister(EMULATOR_REG_CS),
           EmulatorContext.state->reg_ip);

    /* Call the softx86 API */
    if (!softx86_step(&EmulatorContext))
    {
        /* Invalid opcode */
        EmulatorInterrupt(EMULATOR_EXCEPTION_INVALID_OPCODE);
    }
#else
    // TODO: NOT IMPLEMENTED
#endif
}

VOID EmulatorCleanup()
{
    /* Free the memory allocated for the 16-bit address space */
    if (BaseAddress != NULL) HeapFree(GetProcessHeap(), 0, BaseAddress);

#ifndef NEW_EMULATOR
    /* Free the softx86 CPU and FPU emulator */
    softx86_free(&EmulatorContext);
    softx87_free(&FpuEmulatorContext);
#endif
}

VOID EmulatorSetA20(BOOLEAN Enabled)
{
    A20Line = Enabled;
}

/* EOF */
