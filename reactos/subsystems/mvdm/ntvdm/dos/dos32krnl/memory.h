/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dos/dos32krnl/memory.h
 * PURPOSE:         DOS32 Memory Manager
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _DOS_MEMORY_H_
#define _DOS_MEMORY_H_

/* TYPEDEFS *******************************************************************/

#define SEGMENT_TO_MCB(seg) ((PDOS_MCB)SEG_OFF_TO_PTR((seg), 0))

#define DOS_ALLOC_HIGH      0x40
#define DOS_ALLOC_HIGH_LOW  0x80

enum DOS_ALLOC_STRATEGY
{
    DOS_ALLOC_FIRST_FIT,
    DOS_ALLOC_BEST_FIT,
    DOS_ALLOC_LAST_FIT
};

#pragma pack(push, 1)
typedef struct _DOS_MCB
{
    CHAR BlockType;
    WORD OwnerPsp;
    WORD Size;
    BYTE Unused[3];
    CHAR Name[8];
} DOS_MCB, *PDOS_MCB;
C_ASSERT(sizeof(DOS_MCB) == 0x10);
#pragma pack(pop)

/* VARIABLES ******************************************************************/

/* FUNCTIONS ******************************************************************/

WORD DosAllocateMemory(WORD Size, WORD *MaxAvailable);
BOOLEAN DosResizeMemory(WORD BlockData, WORD NewSize, WORD *MaxAvailable);
BOOLEAN DosFreeMemory(WORD BlockData);
BOOLEAN DosLinkUmb(VOID);
BOOLEAN DosUnlinkUmb(VOID);
VOID DosChangeMemoryOwner(WORD Segment, WORD NewOwner);

VOID DosInitializeUmb(VOID);
VOID DosInitializeMemory(VOID);

#endif // _DOS_MEMORY_H_

/* EOF */
