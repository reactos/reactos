/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            hal/x86/drive.c
 * PURPOSE:         Drive letter assignment
 * PROGRAMMER:      
 * UPDATE HISTORY:
 *	2000-03-25
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID STDCALL
IoAssignDriveLetters(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
		     IN PSTRING NtDeviceName,
		     OUT PUCHAR NtSystemPath,
		     OUT PSTRING NtSystemPathString)
{
  HalIoAssignDriveLetters(LoaderBlock,
			  NtDeviceName,
			  NtSystemPath,
			  NtSystemPathString);
}

/* EOF */
