/*
 * COPYRIGHT:               See COPYING in the top level directory
 * PROJECT:                 ReactOS kernel
 * FILE:                    ntoskrnl/rtl/seqlist.c
 * PURPOSE:                 Implementing sequenced lists
 * PROGRAMMER:              David Welch (welch@cwcom.net)
 * REVISION HISTORY:
 *             28/06/98: Created
 */

/* INCLUDES ***************************************************************/

#include <ddk/ntddk.h>

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


