/* $Id$
 *
 * server.c - VMS Enviroment Subsystem Server - Initialization
 * 
 * ReactOS Operating System
 * 
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 */
#include "vmssrv.h"

//#define NDEBUG
#include <debug.h>

HANDLE VmsApiPort = NULL;

/**********************************************************************
 * NAME							PRIVATE
 * 	VmsApiNull/2
 */
NTSTATUS NTAPI VmsApiNull (IN OUT PCSR_API_MESSAGE ApiMessage,
			   IN OUT PULONG Reply)
{
	DPRINT("VMSSRV: %s called\n", __FUNCTION__);

	*Reply = 0;
	return STATUS_SUCCESS;
}

PCSR_API_ROUTINE VmsServerApiDispatchTable [1] =
{
	VmsApiNull
};

BOOLEAN VmsServerApiValidTable [1] =
{
    TRUE
};

PCHAR VmsServerApiNameTable [1] =
{
    "Null",
};

/*=====================================================================
 * 	PUBLIC API
 *===================================================================*/

NTSTATUS NTAPI ServerDllInitialization (PCSR_SERVER_DLL LoadedServerDll)
{
	NTSTATUS Status = STATUS_SUCCESS;
	
	DPRINT("VMSSRV: %s called\n", __FUNCTION__);

	// Get the listening port from csrsrv.dll
	VmsApiPort = CsrQueryApiPort ();
	if (NULL == VmsApiPort)
	{
		Status = STATUS_UNSUCCESSFUL;
	} else {
		// Set CSR information
		LoadedServerDll->ApiBase             = 0;
		LoadedServerDll->HighestApiSupported = 0;
		LoadedServerDll->DispatchTable       = VmsServerApiDispatchTable;
		LoadedServerDll->ValidTable          = VmsServerApiValidTable;
		LoadedServerDll->NameTable           = VmsServerApiNameTable;
		LoadedServerDll->SizeOfProcessData   = 0;
		LoadedServerDll->ConnectCallback     = NULL;
		LoadedServerDll->DisconnectCallback  = NULL;
	}
	return Status;
}

/* EOF */
