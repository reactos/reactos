/* $Id: drive.c,v 1.2 2000/06/30 22:51:34 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            hal/x86/drive.c
 * PURPOSE:         Drive letters
 * PROGRAMMER:      
 * UPDATE HISTORY:
 *	2000-03-25
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID
STDCALL
IoAssignDriveLetters (
	IN	PLOADER_PARAMETER_BLOCK	LoaderBlock,
	IN	PSTRING			NtDeviceName,
	OUT	PUCHAR			NtSystemPath,
	OUT	PSTRING			NtSystemPathString
	)
{
#ifdef __NTOSKRNL__
	HalDispatchTable.HalIoAssignDriveLetters (LoaderBlock,
						   NtDeviceName,
						   NtSystemPath,
						   NtSystemPathString);
#else
	HalDispatchTable->HalIoAssignDriveLetters (LoaderBlock,
						   NtDeviceName,
						   NtSystemPath,
						   NtSystemPathString);
#endif
}

/* EOF */
