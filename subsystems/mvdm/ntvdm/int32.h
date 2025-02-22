/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/int32.h
 * PURPOSE:         32-bit Interrupt Handlers
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _INT32_H_
#define _INT32_H_

/* INCLUDES *******************************************************************/

#include "cpu/callback.h"

/* DEFINES ********************************************************************/

/* 32-bit Interrupt Identifiers */
#define EMULATOR_MAX_INT32_NUM  0xFF + 1


//
// WARNING WARNING!!
// If you're changing the stack indices here, you then need
// to also fix the Int16To32 handler code in int32.c !!
//

// Custom variable pushed onto the stack for INT32 interrupts
#define STACK_INT_NUM   0

// This is the standard stack layout for an interrupt
#define STACK_IP        1
#define STACK_CS        2
#define STACK_FLAGS     3

// To be adjusted with the Int16To32 handler code in int32.c
#define Int16To32StubSize   17

/* FUNCTIONS ******************************************************************/

typedef VOID (WINAPI *EMULATOR_INT32_PROC)(LPWORD Stack);

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

VOID InitializeInt32(VOID);

#endif // _INT32_H_

/* EOF */
