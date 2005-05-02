/* $Id: $
 *
 * init.c - VMS Enviroment Subsystem Server - Initialization
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
#include "vmsss.h"

//#define NDEBUG
#include <debug.h>


HANDLE VmsSbApiPort   = (HANDLE) 0; // \VMS\SbApiPort
HANDLE SmCalledBack   = (HANDLE) 0; // signalled when SM connects to \VMS\SbApiPort
HANDLE SmVmsSbApiPort = (HANDLE) 0; // server side (our one) port for SM conn request
HANDLE SmApiPort      = (HANDLE) 0; // client side of \SmApiPort

HANDLE VmsSessionPort = (HANDLE) 0; // pseudo terminals call here for a new session
HANDLE VmsApiPort     = (HANDLE) 0; // VMS processes call here for system calls

/**********************************************************************
 *	SB API Port Thread
 *********************************************************************/
static VOID STDCALL
VmsSbApiPortThread (PVOID x)
{
	HANDLE Port = (HANDLE) x;
	NTSTATUS Status = STATUS_SUCCESS;
	LPC_MAX_MESSAGE ConnectionRequest = {{0}};

	DPRINT("VMS: %s: called\n", __FUNCTION__);
	
	Status = NtListenPort (Port, & ConnectionRequest.Header);
	if(!NT_SUCCESS(Status))
	{
		DPRINT("VMS: %s: NtListenPort failed (Status=0x%08lx)\n",
			__FUNCTION__, Status);
	}else{
		DPRINT("VMS: %s received a connection request\n", __FUNCTION__);
		Status = NtAcceptConnectPort (& SmVmsSbApiPort,
					      0,
					      & ConnectionRequest.Header,
					      TRUE, /* accept it */
					      NULL,
					      NULL);
		if(!NT_SUCCESS(Status))
		{
			DPRINT("VMS: %s: NtAcceptConnectPort failed (Status=0x%08lx)\n",
				__FUNCTION__, Status);
		}else{
			DPRINT("VMS: %s accepted the connection request\n", __FUNCTION__);
			Status = NtCompleteConnectPort (SmVmsSbApiPort);
			if(!NT_SUCCESS(Status))
			{
				DPRINT("VMS: %s: NtCompleteConnectPort failed (Status=0x%08lx)\n",
					__FUNCTION__, Status);
			}else{
				DPRINT("VMS: %s completed the connection request\n", __FUNCTION__);
				Status = NtSetEvent (SmCalledBack, NULL);
				DPRINT("VMS: %s signalled the main thread to initialize the subsystem\n", __FUNCTION__);
				DPRINT("VMS: %s enters main loop\n", __FUNCTION__);
				while (TRUE)
				{
				}
			}
		}
	}
	NtClose (Port);
	NtTerminateThread (NtCurrentThread(), Status);
}
/**********************************************************************
 *	API Port Thread
 *********************************************************************/
static VOID STDCALL
VmsApiPortThread (PVOID x)
{
	HANDLE Port = (HANDLE) x;
	NTSTATUS Status = STATUS_SUCCESS;
	
	DPRINT("VMS: %s: called\n", __FUNCTION__);
	while (TRUE)
	{
	}
	NtClose (Port);
	NtTerminateThread (NtCurrentThread(), Status);
}

/**********************************************************************
 * NAME							PRIVATE
 * 	VmspCreateObDirectory/1
 */
static NTSTATUS FASTCALL
VmspCreateObDirectory (PWSTR DirectoryName)
{
	UNICODE_STRING     usDirectoryName = {0};
	OBJECT_ATTRIBUTES  DirectoryAttributes = {0};
	NTSTATUS           Status = STATUS_SUCCESS;
	HANDLE             hDirectory = (HANDLE) 0;

	DPRINT("VMS: %s called\n", __FUNCTION__);

	RtlInitUnicodeString (& usDirectoryName,
			      DirectoryName);
	InitializeObjectAttributes (& DirectoryAttributes,
				    & usDirectoryName,
				    0, NULL, NULL);
	Status = NtCreateDirectoryObject (& hDirectory,
					  DIRECTORY_CREATE_SUBDIRECTORY,
					  & DirectoryAttributes);
	if(!NT_SUCCESS(Status))
	{
		DPRINT("VMS: %s: NtCreateDirectoryObject failed (Status=0x%08lx)\n",
				__FUNCTION__, Status);
		return Status;
	}
	NtClose (hDirectory);
	return STATUS_SUCCESS;
}

/**********************************************************************
 * NAME							PRIVATE
 * 	VmspCreatePort/1
 */
static NTSTATUS STDCALL
VmspCreatePort (IN OUT PHANDLE pPortHandle,
		IN     PWSTR PortName,
		IN     ULONG MaxDataSize,
		IN     ULONG MaxMessageSize,
		IN     PTHREAD_START_ROUTINE ListeningThread)
{
	UNICODE_STRING     usPortName = {0};
	OBJECT_ATTRIBUTES  PortAttributes = {0};
	NTSTATUS           Status = STATUS_SUCCESS;

	DPRINT("VMS: %s called\n", __FUNCTION__);

	if(NULL == ListeningThread)
	{
		return STATUS_INVALID_PARAMETER;
	}
	
	RtlInitUnicodeString (& usPortName, PortName);
	Status = NtCreatePort (pPortHandle,
				& PortAttributes,
				MaxDataSize,
				MaxMessageSize,
				0);
	if(!NT_SUCCESS(Status))
	{
		DPRINT("VMS: %s: NtCreatePort failed (Status=0x%08lx)\n", __FUNCTION__, Status);
		return Status;
	}
	Status = RtlCreateUserThread (NtCurrentProcess(),
				      NULL,
				      FALSE,
				      0, 0, 0,
				      ListeningThread,
				      pPortHandle,
				      NULL, NULL);
	if(!NT_SUCCESS(Status))
	{
		DPRINT("VMS: %s: RtlCreateUserThread failed (Status=0x%08lx)\n", __FUNCTION__, Status);
		return Status;
	}
	return Status;
}

/**********************************************************************
 * VmsInitializeServer/0
 */
NTSTATUS
VmsInitializeServer(VOID)
{
	NTSTATUS Status = STATUS_SUCCESS;
	WCHAR    NameBuffer [32];

	DPRINT("VMS: %s called\n", __FUNCTION__);

	/* Create the \VMS directory */
	wcscpy (NameBuffer, L"\\VMS");
	Status = VmspCreateObDirectory (NameBuffer);
	if(!NT_SUCCESS(Status))
	{
		DPRINT("VMS: %s: VmspCreateObDirectory failed!\n", __FUNCTION__);
	}else{
		/* Create the \VMS\SbApiPort port */
		wcscat (NameBuffer, L"\\SbApiPort");
		Status = VmspCreatePort (& VmsSbApiPort,
					 NameBuffer,
					 0x104,
					 0x148,
					 VmsSbApiPortThread);
		if(!NT_SUCCESS(Status))
		{
			DPRINT("VMS %s: VmspCreatePort failed (Status=%08lx)\n",
					__FUNCTION__, Status);
			return Status;
		}else{
			OBJECT_ATTRIBUTES EventAttributes;
			
			InitializeObjectAttributes (& EventAttributes,
							NULL,
			  				0,
			  				NULL,
							NULL);
			Status = NtCreateEvent (& SmCalledBack,
						EVENT_ALL_ACCESS,
						& EventAttributes,
						SynchronizationEvent,
						FALSE);
			if(!NT_SUCCESS(Status))
			{
				DPRINT("VMS: %s: NtCreateEvent failed (Status=0x%08lx)\n",
						__FUNCTION__, Status);
				return Status;
			}else{
				UNICODE_STRING VmsSbApiPortName;

				RtlInitUnicodeString (& VmsSbApiPortName, NameBuffer);
				Status = SmConnectApiPort (& VmsSbApiPortName,
							   VmsSbApiPort,
							   77, /* VMS CUI */
							   & SmApiPort);
				if(!NT_SUCCESS(Status))
				{
					DPRINT("VMS: %s: SmConnectApiPort failed (Status=0x%08lx)\n",
							__FUNCTION__, Status);
					return Status;
				}else{
					Status = NtWaitForSingleObject (SmCalledBack,
									FALSE,
									INFINITE);
					/* OK initialize the VMS subsystem */
					wcscpy (& NameBuffer[4], L"\\ApiPort");
					Status = VmspCreatePort (& VmsApiPort,
								 NameBuffer,
								 0x104,
								 0x148,
								 VmsApiPortThread);
					/* TODO */
	
					wcscpy (& NameBuffer[4], L"\\Session");
					Status = VmspCreateObDirectory (NameBuffer);
					/* TODO */

					Status = SmCompleteSession (SmApiPort,
								    VmsSbApiPort,
								    VmsApiPort);
				}
			}
		}
	}
	return STATUS_SUCCESS;	
}

/* EOF */
