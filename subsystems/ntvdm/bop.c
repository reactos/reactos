/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            bop.c
 * PURPOSE:         BIOS Operation Handlers
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "bios.h"
#include "bop.h"
#include "dos.h"
//#include "vga.h"
//#include "pic.h"
//#include "ps2.h"
//#include "timer.h"
#include "registers.h"

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

EMULATOR_BOP_PROC BopProc[EMULATOR_MAX_BOP_NUM] =
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    ControlBop
};

VOID WINAPI Exception(BYTE ExceptionNumber, LPWORD Stack)
{
    WORD CodeSegment, InstructionPointer;

    ASSERT(ExceptionNumber < 8);

    /* Get the CS:IP */
    InstructionPointer = Stack[STACK_IP];
    CodeSegment = Stack[STACK_CS];

    /* Display a message to the user */
    DisplayMessage(L"Exception: %s occured at %04X:%04X",
                   ExceptionName[ExceptionNumber],
                   CodeSegment,
                   InstructionPointer);

    /* Stop the VDM */
    VdmRunning = FALSE;
    return;
}

// VOID WINAPI IrqDispatch(BYTE IrqNumber, LPWORD Stack)
// {
    // /* Check if this was an PIC IRQ */
    // if (IntNum >= BIOS_PIC_MASTER_INT && IntNum < BIOS_PIC_MASTER_INT + 8)
    // {
        // /* It was an IRQ from the master PIC */
        // BiosHandleIrq(IntNum - BIOS_PIC_MASTER_INT, Stack);
    // }
    // else if (IntNum >= BIOS_PIC_SLAVE_INT && IntNum < BIOS_PIC_SLAVE_INT + 8)
    // {
        // /* It was an IRQ from the slave PIC */
        // BiosHandleIrq(IntNum - BIOS_PIC_SLAVE_INT + 8, Stack);
    // }

    // return;
// }

// VOID WINAPI BiosInt(BYTE IntNumber, LPWORD Stack)
// {
// }

VOID WINAPI IntDispatch(LPWORD Stack)
{
    BYTE IntNum;

    /* Get the interrupt number */
    IntNum = LOBYTE(Stack[STACK_INT_NUM]);

    /* Check if this was an exception */
    if (IntNum < 8)
    {
        Exception(IntNum, Stack);
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
        case BIOS_MEMORY_SIZE:
        {
            /* This is the BIOS "get memory size" command, call the BIOS */
            BiosGetMemorySize(Stack);
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
        case 0x2F:
        {
            DPRINT1("DOS System Function INT 0x2F, AH = %xh, AL = %xh NOT IMPLEMENTED!\n",
                    getAH(), getAL());
            Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
            break;
        }
        default:
        {
            DPRINT1("Unhandled interrupt: 0x%02X\n", IntNum);
            break;
        }
    }
}

VOID WINAPI ControlBop(LPWORD Stack)
{
    IntDispatch(Stack);
}

/* EOF */
