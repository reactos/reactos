/*
 * PROJECT:		 ReactOS Boot Loader
 * LICENSE:		 BSD - See COPYING.ARM in the top level directory
 * FILE:		 drivers/sac/driver/memory.c
 * PURPOSE:		 Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS:	 ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "sacdrv.h"

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

BOOLEAN
InitializeMemoryManagement(VOID)
{
	return FALSE;
}

VOID
FreeMemoryManagement(
	VOID
	)
{
	
}

PVOID
MyAllocatePool(
	IN SIZE_T PoolSize,
	IN ULONG Tag,
	IN PCHAR File,
	IN ULONG Line
	)
{
	return NULL;
}

VOID
MyFreePool(
	IN PVOID *Block
	)
{

}
