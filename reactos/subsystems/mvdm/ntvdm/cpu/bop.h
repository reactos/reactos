/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            bop.h
 * PURPOSE:         BIOS Operation Handlers
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _BOP_H_
#define _BOP_H_

/* DEFINES ********************************************************************/

/* BOP Identifiers */
#define EMULATOR_BOP            0xC4C4
#define EMULATOR_MAX_BOP_NUM    0xFF + 1

/* FUNCTIONS ******************************************************************/

typedef VOID (WINAPI *EMULATOR_BOP_PROC)(LPWORD Stack);

VOID RegisterBop(BYTE BopCode, EMULATOR_BOP_PROC BopHandler);
VOID FASTCALL EmulatorBiosOperation(PFAST486_STATE State, UCHAR BopCode);

#endif // _BOP_H_

/* EOF */
