/* $Id: drive.c,v 1.4.32.2 2004/10/25 02:26:26 ion Exp $
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
#include <ndk/haltypes.h>
#include <ndk/halfuncs.h>
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
