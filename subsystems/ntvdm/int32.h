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

/* DEFINES ********************************************************************/

/* 32-bit Interrupt Identifiers */
#define EMULATOR_MAX_INT32_NUM  0xFF + 1

#define INT_HANDLER_OFFSET 0x1000
#define COMMON_STUB_OFFSET 0x2000

/* FUNCTIONS ******************************************************************/

typedef VOID (WINAPI *EMULATOR_INT32_PROC)(LPWORD Stack);

VOID WINAPI Int32Dispatch(LPWORD Stack);
VOID InitializeInt32(WORD BiosSegment);
VOID RegisterInt32(BYTE IntNumber, EMULATOR_INT32_PROC IntHandler);

#endif // _INT32_H_

/* EOF */
