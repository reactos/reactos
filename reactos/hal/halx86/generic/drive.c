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

#define NDEBUG
#include <hal.h>

/* FUNCTIONS *****************************************************************/

VOID STDCALL
IoAssignDriveLetters(IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
		     IN PSTRING NtDeviceName,
		     OUT PUCHAR NtSystemPath,
		     OUT PSTRING NtSystemPathString)
{
  /* FIXME FIXME FIXME FUCK SOMEONE FIXME*/
  HalIoAssignDriveLetters(NULL,
			  NtDeviceName,
			  NtSystemPath,
			  NtSystemPathString);
}

/* EOF */
