/* $Id: pipe.c,v 1.3 2000/06/03 14:47:32 ea Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/create.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <windows.h>
#include <wchar.h>
#include <string.h>

#include <kernel32/kernel32.h>

/* FUNCTIONS ****************************************************************/

BOOL STDCALL CreatePipe(PHANDLE hReadPipe,
			PHANDLE hWritePipe,
			LPSECURITY_ATTRIBUTES lpPipeAttributes,
			DWORD nSize)
{
   DPRINT("CreatePipe is unimplemented\n");
   return(FALSE);
}

/* EOF */
