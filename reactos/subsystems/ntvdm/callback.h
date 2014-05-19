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

/* 32-bit Interrupt Identifiers */
#define EMULATOR_MAX_INT32_NUM  0xFF + 1

typedef struct _CALLBACK16
{
    ULONG  TrampolineFarPtr; // Where the trampoline zone is placed
    ULONG  TrampolineSize;   // Size of the trampoline zone
    USHORT Segment;
    USHORT NextOffset;
} CALLBACK16, *PCALLBACK16;

extern const ULONG Int16To32StubSize;

/* FUNCTIONS ******************************************************************/

typedef VOID (WINAPI *EMULATOR_INT32_PROC)(LPWORD Stack);

VOID
InitializeContext(IN PCALLBACK16 Context,
                  IN USHORT      Segment,
                  IN USHORT      Offset);

VOID
Call16(IN USHORT Segment,
       IN USHORT Offset);

ULONG
RegisterCallback16(IN  ULONG   FarPtr,
                   IN  LPBYTE  CallbackCode,
                   IN  SIZE_T  CallbackSize,
                   OUT PSIZE_T CodeSize OPTIONAL);

VOID
RunCallback16(IN PCALLBACK16 Context,
              IN ULONG       FarPtr);

ULONG
RegisterInt16(IN  ULONG   FarPtr,
              IN  BYTE    IntNumber,
              IN  LPBYTE  CallbackCode,
              IN  SIZE_T  CallbackSize,
              OUT PSIZE_T CodeSize OPTIONAL);

ULONG
RegisterInt32(IN  ULONG   FarPtr,
              IN  BYTE    IntNumber,
              IN  EMULATOR_INT32_PROC IntHandler,
              OUT PSIZE_T CodeSize OPTIONAL);

VOID
Int32Call(IN PCALLBACK16 Context,
          IN BYTE IntNumber);

VOID WINAPI Int32Dispatch(LPWORD Stack);
VOID InitializeCallbacks(VOID);

#endif // _CALLBACK_H_

/* EOF */
