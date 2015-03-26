/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            emulator.c
 * PURPOSE:         Minimal x86 machine emulator for the VDM
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "cpu.h"

#include "emulator.h"
#include "memory.h"
#include "callback.h"
#include "bop.h"
#include <isvbop.h>
#include <pseh/pseh2.h>

#include "clock.h"
#include "bios/rom.h"
#include "hardware/cmos.h"
#include "hardware/keyboard.h"
#include "hardware/mouse.h"
#include "hardware/pic.h"
#include "hardware/ps2.h"
#include "hardware/sound/speaker.h"
#include "hardware/pit.h"
#include "hardware/video/vga.h"

#include "io.h"

/* PRIVATE VARIABLES **********************************************************/

FAST486_STATE EmulatorContext;
BOOLEAN CpuRunning = FALSE;

/* No more than 'MaxCpuCallLevel' recursive CPU calls are allowed */
static const INT MaxCpuCallLevel = 32;
static INT CpuCallLevel = 0; // == 0: CPU stopped; >= 1: CPU running or halted

#if 0
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
#endif

// /* BOP Identifiers */
// #define BOP_DEBUGGER    0x56    // Break into the debugger from a 16-bit app

/* PRIVATE FUNCTIONS **********************************************************/

#if 0
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
#endif

// FIXME: This function assumes 16-bit mode!!!
VOID CpuExecute(WORD Segment, WORD Offset)
{
    /* Tell Fast486 to move the instruction pointer */
    Fast486ExecuteAt(&EmulatorContext, Segment, Offset);
}

VOID CpuStep(VOID)
{
    /* Dump the state for debugging purposes */
    // Fast486DumpState(&EmulatorContext);

    /* Execute the next instruction */
    Fast486StepInto(&EmulatorContext);
}

LONG CpuExceptionFilter(IN PEXCEPTION_POINTERS ExceptionInfo)
{
    /* Get the exception record */
    PEXCEPTION_RECORD ExceptionRecord = ExceptionInfo->ExceptionRecord;

    switch (ExceptionRecord->ExceptionCode)
    {
        /* We only handle access violations so far */
        case EXCEPTION_ACCESS_VIOLATION:
        {
            BOOLEAN Writing = (ExceptionRecord->ExceptionInformation[0] == 1);

            /* Retrieve the address to which a read or write attempt was made */
            ULONG_PTR Address = ExceptionRecord->ExceptionInformation[1];

            /*
             * Check whether the access exception was done inside the virtual memory space
             * (caused by an emulated app) or outside (casued by a bug in ourselves).
             */
            if (Address <  (ULONG_PTR)BaseAddress ||
                Address >= (ULONG_PTR)BaseAddress + MAX_ADDRESS)
            {
                DPRINT1("NTVDM: %s access violation at 0x%p outside the virtual memory space!\n",
                        (Writing ? "Write" : "Read"), Address);
                return EXCEPTION_CONTINUE_SEARCH;
            }

            /* We are good to go, dispatch to our memory handlers */

            /* Fix the CPU state */
            Fast486Rewind(&EmulatorContext);

            /* Call the memory handler */
            MemExceptionHandler((ULONG)PHYS_TO_REAL(Address), Writing);

            // /* Continue executing the exception handler */
            // return EXCEPTION_EXECUTE_HANDLER;
            return EXCEPTION_CONTINUE_EXECUTION;
        }

        default:
        {
            DPRINT1("NTVDM: Exception 0x%08lx not handled!\n", ExceptionRecord->ExceptionCode);
            break;
        }
    }

    /* Continue to search for a handler */
    return EXCEPTION_CONTINUE_SEARCH;
}

VOID CpuSimulate(VOID)
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
    DPRINT("CpuSimulate --> Level %d\n", CpuCallLevel);

    CpuRunning = TRUE;
    while (VdmRunning && CpuRunning)
    {
        _SEH2_TRY
        {
            while (VdmRunning && CpuRunning) ClockUpdate();
        }
        _SEH2_EXCEPT(CpuExceptionFilter(_SEH2_GetExceptionInformation()))
        {
            DPRINT1("VDM exception handler called\n");
        }
        _SEH2_END;
    }

    DPRINT("CpuSimulate <-- Level %d\n", CpuCallLevel);
    CpuCallLevel--;
    if (!VdmRunning || CpuCallLevel < 0) CpuCallLevel = 0;

    /* This takes into account for reentrance */
    if (VdmRunning && (CpuCallLevel > 0)) CpuRunning = TRUE;
}

VOID CpuUnsimulate(VOID)
{
    /* Stop simulation */
    CpuRunning = FALSE;
}

static VOID WINAPI CpuUnsimulateBop(LPWORD Stack)
{
    CpuUnsimulate();
}

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN CpuInitialize(VOID)
{
    // /* Initialize the internal clock */
    // if (!ClockInitialize())
    // {
        // wprintf(L"FATAL: Failed to initialize the clock\n");
        // return FALSE;
    // }

    /* Initialize the CPU */
    Fast486Initialize(&EmulatorContext,
                      EmulatorReadMemory,
                      EmulatorWriteMemory,
                      EmulatorReadIo,
                      EmulatorWriteIo,
                      EmulatorBiosOperation,
                      EmulatorIntAcknowledge,
                      EmulatorFpu,
                      NULL /* TODO: Use a TLB */);

    /* Initialize the software callback system and register the emulator BOPs */
    // RegisterBop(BOP_DEBUGGER  , EmulatorDebugBreakBop);
    RegisterBop(BOP_UNSIMULATE, CpuUnsimulateBop);

    return TRUE;
}

VOID CpuCleanup(VOID)
{
    // Fast486Cleanup();
}

/* EOF */
