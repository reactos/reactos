/*
 * COPYRIGHT:               See COPYING in the top level directory
 * PROJECT:                 ReactOS kernel
 * FILE:                    mkernel/rtl/seqlist.c
 * PURPOSE:                 Implementing sequenced lists
 * PROGRAMMER:              David Welch (welch@mcmail.com)
 * REVISION HISTORY:
 *             28/06/98: Created
 */

/* INCLUDES ***************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/kernel.h>

/* TYPES ********************************************************************/

typedef union _SLIST_HEADER 
{
   ULONGLONG Alignment;
   struct
     {
	SINGLE_LIST_ENTRY Next;
	USHORT Depth;
	USHORT Sequence;
     } s;
} SLIST_HEADER, *PSLIST_HEADER;

/* FUNCTIONS ****************************************************************/

VOID ExInitializeSListHead(PSLIST_HEADER SListHead)
{
   SListHead->s.Next.Next=NULL;
   SListHead->s.Depth = 0;
   SListHead->s.Sequence = 0;
}


