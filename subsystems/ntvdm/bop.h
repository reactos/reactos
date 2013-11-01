/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            bop.h
 * PURPOSE:         BIOS Operation Handlers
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

typedef VOID (WINAPI *EMULATOR_BOP_PROC)(LPWORD Stack);

extern EMULATOR_BOP_PROC BopProc[EMULATOR_MAX_BOP_NUM];

VOID WINAPI ControlBop(LPWORD Stack);

/* EOF */
