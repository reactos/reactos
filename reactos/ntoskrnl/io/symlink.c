/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/symlink.c
 * PURPOSE:         Implements symbolic links
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* GLOBALS ******************************************************************/

typedef struct
{
   PVOID Target;
} SYMBOLIC_LINK_OBJECT;

OBJECT_TYPE SymlinkObjectType = {{NULL,0,0},
                                0,
                                0,
                                ULONG_MAX,
                                ULONG_MAX,
                                sizeof(SYMBOLIC_LINK_OBJECT),
                                0,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               };                           

/* FUNCTIONS *****************************************************************/

VOID IoInitSymbolicLinkImplementation(VOID)
{
   
}

NTSTATUS IoCreateUnprotectedSymbolicLink(PUNICODE_STRING SymbolicLinkName,
					 PUNICODE_STRING DeviceName)
{
   return(IoCreateSymbolicLink(SymbolicLinkName,DeviceName));
}

NTSTATUS IoCreateSymbolicLink(PUNICODE_STRING SymbolicLinkName,
			      PUNICODE_STRING DeviceName)
{
   UNIMPLEMENTED;
}

NTSTATUS IoDeleteSymbolicLink(PUNICODE_STRING DeviceName)
{
   UNIMPLEMENTED;
}
