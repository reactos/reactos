/*
 * COPYRIGHT:       GPLv2+ - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/bios/umamgr.h
 * PURPOSE:         Upper Memory Area Manager
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _UMAMGR_H_
#define _UMAMGR_H_

/* DEFINITIONS ****************************************************************/

typedef enum
{
    UMA_FREE = 0,   // Free RAM block
    UMA_SYSTEM,     // System memory (eg. VGA memory, etc...)
    UMA_ROM,        // ROM block
    UMA_UMB,        // Upper memory block
    UMA_VDD         // VDD-reserved block
} UMA_DESC_TYPE;

/* FUNCTIONS ******************************************************************/

BOOLEAN UmaDescReserve(IN OUT PUSHORT UmbSegment, IN OUT PUSHORT Size);
BOOLEAN UmaDescRelease(IN USHORT UmbSegment);
BOOLEAN UmaDescReallocate(IN USHORT UmbSegment, IN OUT PUSHORT Size);

BOOLEAN UmaMgrInitialize(VOID);
VOID UmaMgrCleanup(VOID);

#endif // _UMAMGR_H_

/* EOF */
