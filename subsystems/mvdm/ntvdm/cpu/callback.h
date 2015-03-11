/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            callback.h
 * PURPOSE:         32-bit Interrupt Handlers
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _CALLBACK_H_
#define _CALLBACK_H_

/* DEFINES ********************************************************************/

#define BOP(num)            LOBYTE(EMULATOR_BOP), HIBYTE(EMULATOR_BOP), (num)
#define UnSimulate16(trap)           \
do {                                 \
    *(PUSHORT)(trap) = EMULATOR_BOP; \
    (trap) += sizeof(USHORT);        \
    *(trap) = BOP_UNSIMULATE;        \
} while(0)
// #define UnSimulate16        MAKELONG(EMULATOR_BOP, BOP_UNSIMULATE) // BOP(BOP_UNSIMULATE)

typedef struct _CALLBACK16
{
    ULONG  TrampolineFarPtr; // Where the trampoline zone is placed
    ULONG  TrampolineSize;   // Size of the trampoline zone
    USHORT Segment;
    USHORT NextOffset;
} CALLBACK16, *PCALLBACK16;

/* FUNCTIONS ******************************************************************/

VOID
InitializeContextEx(IN PCALLBACK16 Context,
                    IN ULONG       TrampolineSize,
                    IN USHORT      Segment,
                    IN USHORT      Offset);

VOID
InitializeContext(IN PCALLBACK16 Context,
                  IN USHORT      Segment,
                  IN USHORT      Offset);

VOID
Call16(IN USHORT Segment,
       IN USHORT Offset);

VOID
RunCallback16(IN PCALLBACK16 Context,
              IN ULONG       FarPtr);

ULONG
RegisterCallback16(IN  ULONG   FarPtr,
                   IN  LPBYTE  CallbackCode,
                   IN  SIZE_T  CallbackSize,
                   OUT PSIZE_T CodeSize OPTIONAL);

#endif // _CALLBACK_H_

/* EOF */
