/* $Id:$
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/napi.c
 * PURPOSE:         Native API support routines
 * 
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <ntdll/napi.h>
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

SSDT_ENTRY
__declspec(dllexport)
KeServiceDescriptorTable[SSDT_MAX_ENTRIES] =
{
	{ MainSSDT, NULL, NUMBER_OF_SYSCALLS, MainSSPT },
	{ NULL,    NULL,   0,   NULL   },
	{ NULL,    NULL,   0,   NULL   },
	{ NULL,    NULL,   0,   NULL   }
};

SSDT_ENTRY 
KeServiceDescriptorTableShadow[SSDT_MAX_ENTRIES] =
{
	{ MainSSDT, NULL, NUMBER_OF_SYSCALLS, MainSSPT },
	{ NULL,    NULL,   0,   NULL   },
	{ NULL,    NULL,   0,   NULL   },
	{ NULL,    NULL,   0,   NULL   }
};
