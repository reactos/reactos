/*
 * COPYRIGHT:       GPLv2+ - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/dos/dos32krnl/handle.h
 * PURPOSE:         DOS32 Handles (Job File Table)
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#pragma once

/* DEFINITIONS ****************************************************************/

#define DEFAULT_JFT_SIZE 20

/* FUNCTIONS ******************************************************************/

VOID DosCopyHandleTable(LPBYTE DestinationTable);
BOOLEAN DosResizeHandleTable(WORD NewSize);
WORD DosOpenHandle(BYTE DescriptorId);
BYTE DosQueryHandle(WORD DosHandle);
WORD DosDuplicateHandle(WORD DosHandle);
BOOLEAN DosForceDuplicateHandle(WORD OldHandle, WORD NewHandle);
BOOLEAN DosCloseHandle(WORD DosHandle);
