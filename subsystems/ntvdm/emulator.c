/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            emulator.c
 * PURPOSE:         Minimal x86 machine emulator for the VDM
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#include "ntvdm.h"
#include <softx86/softx86.h>
#include <softx86/softx87.h>

softx86_ctx EmulatorContext;
softx87_ctx FpuEmulatorContext;

static VOID EmulatorReadMemory(PVOID Context, UINT Address, LPBYTE Buffer, INT Size)
{
    /* Make sure the requested address is valid */
    if ((Address + Size) >= MAX_ADDRESS) return;

    /* Are we reading some of the console video memory? */
    if (((Address + Size) >= CONSOLE_VIDEO_MEM_START)
        && (Address < CONSOLE_VIDEO_MEM_END))
    {
        /* Call the VDM BIOS to update the video memory */
        BiosUpdateConsole(max(Address, CONSOLE_VIDEO_MEM_START),
                          min(Address + Size, CONSOLE_VIDEO_MEM_END));
    }

    /* Read the data from the virtual address space and store it in the buffer */
    RtlCopyMemory(Buffer, (LPVOID)((ULONG_PTR)BaseAddress + Address), Size);
}

static VOID EmulatorWriteMemory(PVOID Context, UINT Address, LPBYTE Buffer, INT Size)
{
    /* Make sure the requested address is valid */
    if ((Address + Size) >= MAX_ADDRESS) return;

    /* Make sure we don't write to the ROM area */
    if ((Address + Size) >= ROM_AREA_START && (Address < ROM_AREA_END)) return;

    /* Read the data from the buffer and store it in the virtual address space */
    RtlCopyMemory((LPVOID)((ULONG_PTR)BaseAddress + Address), Buffer, Size);

    /* Check if we modified the console video memory */
    if (((Address + Size) >= CONSOLE_VIDEO_MEM_START)
        && (Address < CONSOLE_VIDEO_MEM_END))
    {
        /* Call the VDM BIOS to update the screen */
        BiosUpdateConsole(max(Address, CONSOLE_VIDEO_MEM_START),
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
        StackSegment = EmulatorContext.state->segment_reg[SX86_SREG_SS].val;
        StackPointer = EmulatorContext.state->general_reg[SX86_REG_SP].val;

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
        }
        else if (IntNum >= BIOS_PIC_SLAVE_INT && IntNum < BIOS_PIC_SLAVE_INT + 8)
        {
            /* It was an IRQ from the slave PIC */
            BiosHandleIrq(IntNum - BIOS_PIC_SLAVE_INT + 8);
        }

        switch (IntNum)
        {
            case VIDEO_BIOS_INTERRUPT:
            {
                /* This is the video BIOS interrupt, call the BIOS */
                BiosVideoService();
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
        }
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN EmulatorInitialize()
{
    /* Allocate memory for the 16-bit address space */
    BaseAddress = HeapAlloc(GetProcessHeap(), 0, MAX_ADDRESS);
    if (BaseAddress == NULL) return FALSE;

    /* Initialize the softx86 CPU emulator */
    if (!softx86_init(&EmulatorContext, SX86_CPULEVEL_80186))
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

    /* Connect the emulated FPU to the emulated CPU */
    softx87_connect_to_CPU(&EmulatorContext, &FpuEmulatorContext);

    return TRUE;
}

VOID EmulatorSetStack(WORD Segment, WORD Offset)
{
    /* Call the softx86 API */
    softx86_set_stack_ptr(&EmulatorContext, Segment, Offset);
}

VOID EmulatorExecute(WORD Segment, WORD Offset)
{
    /* Call the softx86 API */
    softx86_set_instruction_ptr(&EmulatorContext, Segment, Offset);
}

VOID EmulatorInterrupt(BYTE Number)
{
    LPDWORD IntVecTable = (LPDWORD)((ULONG_PTR)BaseAddress);
    UINT Segment, Offset;

    /* Get the segment and offset */
    Segment = HIWORD(IntVecTable[Number]);
    Offset = LOWORD(IntVecTable[Number]);

    /* Call the softx86 API */
    softx86_make_simple_interrupt_call(&EmulatorContext, &Segment, &Offset);
}

ULONG EmulatorGetRegister(ULONG Register)
{
    if (Register < EMULATOR_REG_ES)
    {
        return EmulatorContext.state->general_reg[Register].val;
    }
    else
    {
        return EmulatorContext.state->segment_reg[Register - EMULATOR_REG_ES].val;
    }
}

VOID EmulatorSetRegister(ULONG Register, ULONG Value)
{
    if (Register < EMULATOR_REG_CS)
    {
        EmulatorContext.state->general_reg[Register].val = Value;
    }
    else
    {
        EmulatorContext.state->segment_reg[Register - EMULATOR_REG_ES].val = Value;
    }
}

BOOLEAN EmulatorGetFlag(ULONG Flag)
{
    return (EmulatorContext.state->reg_flags.val & Flag);
}

VOID EmulatorSetFlag(ULONG Flag)
{
    EmulatorContext.state->reg_flags.val |= Flag;
}

VOID EmulatorClearFlag(ULONG Flag)
{
    EmulatorContext.state->reg_flags.val &= ~Flag;
}

VOID EmulatorStep()
{
    /* Call the softx86 API */
    softx86_step(&EmulatorContext);
}

VOID EmulatorCleanup()
{
    /* Free the memory allocated for the 16-bit address space */
    if (BaseAddress != NULL) HeapFree(GetProcessHeap(), 0, BaseAddress);

    /* Free the softx86 CPU and FPU emulator */
    softx86_free(&EmulatorContext);
    softx87_free(&FpuEmulatorContext);
}

/* EOF */
