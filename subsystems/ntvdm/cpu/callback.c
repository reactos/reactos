/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            callback.c
 * PURPOSE:         16 and 32-bit Callbacks Support
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/******************************************************************************\
|   WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING!
|
|   Callbacks support supposes implicitely that the callbacks are used
|   in the SAME thread as the CPU thread, otherwise messing in parallel
|   with the CPU registers is 100% prone to bugs!!
\******************************************************************************/

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "cpu.h"
#include "callback.h"
#include "emulator.h"

#include "bop.h"
#include <isvbop.h>

/* PRIVATE VARIABLES **********************************************************/

#define TRAMPOLINE_SIZE sizeof(ULONGLONG)

static BYTE Yield[] =
{
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90,         // 13x nop
    BOP(BOP_UNSIMULATE),                        // UnSimulate16 BOP
};
C_ASSERT(sizeof(Yield) == 16 * sizeof(BYTE));

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
InitializeContextEx(IN PCALLBACK16 Context,
                    IN ULONG       TrampolineSize,
                    IN USHORT      Segment,
                    IN USHORT      Offset)
{
    Context->TrampolineFarPtr = MAKELONG(Offset, Segment);
    Context->TrampolineSize   = max(TRAMPOLINE_SIZE, TrampolineSize);
    Context->Segment          = Segment;
    Context->NextOffset       = Offset + Context->TrampolineSize;
}

VOID
InitializeContext(IN PCALLBACK16 Context,
                  IN USHORT      Segment,
                  IN USHORT      Offset)
{
    InitializeContextEx(Context,
                        TRAMPOLINE_SIZE,
                        Segment,
                        Offset);
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
    CpuSimulate();

    /* Restore CS:IP */
    setCS(OrgCS);
    setIP(OrgIP);
}

VOID
RunCallback16(IN PCALLBACK16 Context,
              IN ULONG       FarPtr)
{
    PUCHAR TrampolineBase = (PUCHAR)FAR_POINTER(Context->TrampolineFarPtr);
    PUCHAR Trampoline     = TrampolineBase;
    UCHAR  OldTrampoline[TRAMPOLINE_SIZE];

    /* Save the old trampoline */
    ((PULONGLONG)&OldTrampoline)[0] = ((PULONGLONG)TrampolineBase)[0];

    DPRINT("RunCallback16(0x%p)\n", FarPtr);

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

/* EOF */
