/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            int32.c
 * PURPOSE:         32-bit Interrupt Handlers
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"
#include "int32.h"

#include "cpu/bop.h"
#include <isvbop.h>

/* PRIVATE VARIABLES **********************************************************/

/*
 * This is the list of registered 32-bit Interrupt handlers.
 */
static EMULATOR_INT32_PROC Int32Proc[EMULATOR_MAX_INT32_NUM] = { NULL };

/* BOP Identifiers */
#define BOP_CONTROL             0xFF    // Control BOP Handler
    #define BOP_CONTROL_DEFFUNC 0x00    // Default Control BOP Function
    #define BOP_CONTROL_INT32   0xFF    // 32-bit Interrupt dispatcher

#define INT16_TRAMPOLINE_SIZE   sizeof(ULONGLONG) // == TRAMPOLINE_SIZE

/* 16-bit generic interrupt code for calling a 32-bit interrupt handler */
static BYTE Int16To32[] =
{
    0xFA,               // cli

    /* Push the value of the interrupt to be called */
    0x6A, 0xFF,         // push i (patchable to 0x6A, 0xIntNum)

    0xF8,               // clc

    /* The BOP Sequence */
// BOP_SEQ:
    BOP(BOP_CONTROL),   // Control BOP
    BOP_CONTROL_INT32,  // 32-bit Interrupt dispatcher

    0x73, 0x04,         // jnc EXIT (offset +4)

    0xFB,               // sti

    0xF4,               // hlt

    0xEB, 0xF6,         // jmp BOP_SEQ (offset -10)

// EXIT:
    0x44, 0x44,         // inc sp, inc sp
    0xCF,               // iret
};
C_ASSERT(sizeof(Int16To32) == Int16To32StubSize);

/* PUBLIC FUNCTIONS ***********************************************************/

static VOID WINAPI Int32Dispatch(LPWORD Stack)
{
    /* Get the interrupt number */
    BYTE IntNum = LOBYTE(Stack[STACK_INT_NUM]);

    /* Call the 32-bit Interrupt handler */
    if (Int32Proc[IntNum] != NULL)
        Int32Proc[IntNum](Stack);
    else
        DPRINT1("Unhandled 32-bit interrupt: 0x%02X, AX = 0x%04X\n", IntNum, getAX());
}

static VOID WINAPI ControlBop(LPWORD Stack)
{
    /* Get the Function Number and skip it */
    BYTE FuncNum = *(PBYTE)SEG_OFF_TO_PTR(getCS(), getIP());
    setIP(getIP() + 1);

    switch (FuncNum)
    {
        case BOP_CONTROL_INT32:
            Int32Dispatch(Stack);
            break;

        default:
            // DPRINT1("Unassigned Control BOP Function: 0x%02X\n", FuncNum);
            DisplayMessage(L"Unassigned Control BOP Function: 0x%02X", FuncNum);
            break;
    }
}

ULONG
RegisterInt16(IN  ULONG   FarPtr,
              IN  BYTE    IntNumber,
              IN  LPBYTE  CallbackCode,
              IN  SIZE_T  CallbackSize,
              OUT PSIZE_T CodeSize OPTIONAL)
{
    /* Get a pointer to the IVT and set the corresponding entry (far pointer) */
    LPDWORD IntVecTable = (LPDWORD)SEG_OFF_TO_PTR(0x0000, 0x0000);
    IntVecTable[IntNumber] = FarPtr;

    /* Register the 16-bit callback */
    return RegisterCallback16(FarPtr,
                              CallbackCode,
                              CallbackSize,
                              CodeSize);
}

ULONG
RegisterInt32(IN  ULONG   FarPtr,
              IN  BYTE    IntNumber,
              IN  EMULATOR_INT32_PROC IntHandler,
              OUT PSIZE_T CodeSize OPTIONAL)
{
    /* Array for holding our copy of the 16-bit interrupt callback */
    BYTE IntCallback[sizeof(Int16To32)/sizeof(BYTE)];

    /* Check whether the 32-bit interrupt was already registered */
#if 0
    if (Int32Proc[IntNumber] != NULL)
    {
        DPRINT1("RegisterInt32: Interrupt 0x%02X already registered!\n", IntNumber);
        return 0;
    }
#endif

    /* Register the 32-bit interrupt handler */
    Int32Proc[IntNumber] = IntHandler;

    /* Copy the generic 16-bit interrupt callback and patch it */
    RtlCopyMemory(IntCallback, Int16To32, sizeof(Int16To32));
    IntCallback[2] = IntNumber;

    /* Register the 16-bit interrupt callback */
    return RegisterInt16(FarPtr,
                         IntNumber,
                         IntCallback,
                         sizeof(IntCallback),
                         CodeSize);
}

VOID
Int32Call(IN PCALLBACK16 Context,
          IN BYTE IntNumber)
{
    /*
     * TODO: This function has almost the same code as RunCallback16.
     * Something that may be nice is to have a common interface to
     * build the trampoline...
     */

    PUCHAR TrampolineBase = (PUCHAR)FAR_POINTER(Context->TrampolineFarPtr);
    PUCHAR Trampoline     = TrampolineBase;
    UCHAR  OldTrampoline[INT16_TRAMPOLINE_SIZE];

    DPRINT("Int32Call(0x%02X)\n", IntNumber);

    ASSERT(Context->TrampolineSize == INT16_TRAMPOLINE_SIZE);

    /* Save the old trampoline */
    ((PULONGLONG)&OldTrampoline)[0] = ((PULONGLONG)TrampolineBase)[0];

    /* Build the generic entry-point for 16-bit calls */
    if (IntNumber == 0x03)
    {
        /* We are redefining for INT 03h */
        *Trampoline++ = 0xCC; // Call INT 03h
        /** *Trampoline++ = 0x90; // nop **/
    }
    else
    {
        /* Normal interrupt */
        *Trampoline++ = 0xCD; // Call INT XXh
        *Trampoline++ = IntNumber;
    }
    UnSimulate16(Trampoline);

    /* Perform the call */
    Call16(HIWORD(Context->TrampolineFarPtr),
           LOWORD(Context->TrampolineFarPtr));

    /* Restore the old trampoline */
    ((PULONGLONG)TrampolineBase)[0] = ((PULONGLONG)&OldTrampoline)[0];
}

VOID InitializeInt32(VOID)
{
    /* Register the Control BOP */
    RegisterBop(BOP_CONTROL, ControlBop);
}

/* EOF */
