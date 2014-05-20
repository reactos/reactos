/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            callback.c
 * PURPOSE:         16 and 32-bit Callbacks Support
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "callback.h"

#include "bop.h"
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
                                        // function code for the Control BOP Handler

#define BOP(num)            LOBYTE(EMULATOR_BOP), HIBYTE(EMULATOR_BOP), (num)
#define UnSimulate16(trap)           \
do {                                 \
    *(PUSHORT)(trap) = EMULATOR_BOP; \
    (trap) += sizeof(USHORT);        \
    *(trap) = BOP_UNSIMULATE;        \
} while(0)
// #define UnSimulate16        MAKELONG(EMULATOR_BOP, BOP_UNSIMULATE) // BOP(BOP_UNSIMULATE)

#define CALL16_TRAMPOLINE_SIZE  (1 * sizeof(ULONGLONG))
#define  INT16_TRAMPOLINE_SIZE  (1 * sizeof(ULONGLONG))

//
// WARNING WARNING!!
//
// If you modify the code stubs here, think also
// about updating them in int32.c too!!
//

/* 16-bit generic interrupt code for calling a 32-bit interrupt handler */
static BYTE Int16To32[] =
{
    0xFA,               // cli

    /* Push the value of the interrupt to be called */
    0x6A, 0xFF,         // push i (patchable to 0x6A, 0xIntNum)

    /* The BOP Sequence */
// BOP_SEQ:
    0xF8,               // clc
    BOP(BOP_CONTROL),   // Control BOP
    BOP_CONTROL_INT32,  // 32-bit Interrupt dispatcher

    0x73, 0x04,         // jnc EXIT (offset +4)

    0xFB,               // sti

    // HACK: The following instruction should be HLT!
    0x90,               // nop

    0xEB, 0xF5,         // jmp BOP_SEQ (offset -11)

// EXIT:
    0x44, 0x44,         // inc sp, inc sp
    0xCF,               // iret
};
const ULONG Int16To32StubSize = sizeof(Int16To32);

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
InitializeContext(IN PCALLBACK16 Context,
                  IN USHORT      Segment,
                  IN USHORT      Offset)
{
    Context->TrampolineFarPtr = MAKELONG(Offset, Segment);
    Context->TrampolineSize   = max(CALL16_TRAMPOLINE_SIZE,
                                     INT16_TRAMPOLINE_SIZE);
    Context->Segment          = Segment;
    Context->NextOffset       = Offset + Context->TrampolineSize;
}

VOID
Call16(IN USHORT Segment,
       IN USHORT Offset)
{
    /* Save CS:IP */
    USHORT OrgCS = getCS();
    USHORT OrgIP = getIP();

    /* Set the new CS:IP */
    setCS(Segment);
    setIP(Offset);

    DPRINT("Call16(%04X:%04X)\n", Segment, Offset);

    /* Start CPU simulation */
    EmulatorSimulate();

    /* Restore CS:IP */
    setCS(OrgCS);
    setIP(OrgIP);
}



ULONG
RegisterCallback16(IN  ULONG   FarPtr,
                   IN  LPBYTE  CallbackCode,
                   IN  SIZE_T  CallbackSize,
                   OUT PSIZE_T CodeSize OPTIONAL)
{
    LPBYTE CodeStart = (LPBYTE)FAR_POINTER(FarPtr);
    LPBYTE Code      = CodeStart;

    SIZE_T OurCodeSize = CallbackSize;

    if (CallbackCode == NULL) CallbackSize = 0;

    if (CallbackCode)
    {
        /* 16-bit interrupt code */
        RtlCopyMemory(Code, CallbackCode, CallbackSize);
        Code += CallbackSize;
    }

    /* Return the real size of the code if needed */
    if (CodeSize) *CodeSize = OurCodeSize; // == (ULONG_PTR)Code - (ULONG_PTR)CodeStart;

    // /* Return the entry-point address for 32-bit calls */
    // return (ULONG_PTR)(CodeStart + CallbackSize);
    return OurCodeSize;
}

VOID
RunCallback16(IN PCALLBACK16 Context,
              IN ULONG       FarPtr)
{
    PUCHAR TrampolineBase = (PUCHAR)FAR_POINTER(Context->TrampolineFarPtr);
    PUCHAR Trampoline     = TrampolineBase;
    UCHAR  OldTrampoline[CALL16_TRAMPOLINE_SIZE];

    /* Save the old trampoline */
    ((PULONGLONG)&OldTrampoline)[0] = ((PULONGLONG)TrampolineBase)[0];

    DPRINT1("RunCallback16(0x%p)\n", FarPtr);

    /* Build the generic entry-point for 16-bit far calls */
    *Trampoline++ = 0x9A; // Call far seg:off
    *(PULONG)Trampoline = FarPtr;
    Trampoline += sizeof(ULONG);
    UnSimulate16(Trampoline);

    /* Perform the call */
    Call16(HIWORD(Context->TrampolineFarPtr),
           LOWORD(Context->TrampolineFarPtr));

    /* Restore the old trampoline */
    ((PULONGLONG)TrampolineBase)[0] = ((PULONGLONG)&OldTrampoline)[0];
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
        DPRINT1("RegisterInt32: Interrupt 0x%X already registered!\n", IntNumber);
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
    PUCHAR TrampolineBase = (PUCHAR)FAR_POINTER(Context->TrampolineFarPtr);
    PUCHAR Trampoline     = TrampolineBase;
    UCHAR  OldTrampoline[INT16_TRAMPOLINE_SIZE];

    DPRINT("Int32Call(0x%X)\n", IntNumber);

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



VOID WINAPI Int32Dispatch(LPWORD Stack)
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

VOID InitializeCallbacks(VOID)
{
    /* Register the Control BOP */
    RegisterBop(BOP_CONTROL, ControlBop);
}

/* EOF */
