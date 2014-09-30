/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            int32.h
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

extern const ULONG Int16To32StubSize;

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
