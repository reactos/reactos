/* $Id$
 *
 * subsys/csr/csrsrv/server.c - CSR server - subsystem default server
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
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 */
#include "srv.h"

//#define NDEBUG
#include <debug.h>

typedef NTSTATUS (STDCALL * CSR_SERVER_DLL_INIT_PROC)(ULONG,LPWSTR*);

typedef struct _CSRSRV_SERVER_DLL
{
	USHORT             ServerIndex;
	USHORT             Sequence;		// initialization order
	UNICODE_STRING     DllName;
	UNICODE_STRING     DllEntryPoint;
	CSR_SERVER_THREAD  ServerThread;	// NULL ==> inactive

} CSRSRV_SERVER_DLL, *PCSRSRV_SERVER_DLL;

/*=====================================================================
 * 	GLOBALS
 *===================================================================*/

CSRSRV_OPTION CsrSrvOption;

HANDLE CsrSrvApiPortHandle = (HANDLE) 0;

/*=====================================================================
 * 	LOCALS
 *===================================================================*/

static HANDLE CsrSrvSbApiPortHandle = (HANDLE) 0;

static CSRSRV_SERVER_DLL ServerThread [CSR_SERVER_DLL_MAX];

VOID CALLBACK CsrSrvServerThread (PVOID);

/**********************************************************************
 * CsrSrvRegisterServerDll/1
 */
NTSTATUS STDCALL CsrSrvRegisterServerDll (PCSR_SERVER_DLL pServerDll)
{
	static USHORT  NextInSequence = 0;
	USHORT         ServerIndex = 0;

	// 1st call?
	if (0 == NextInSequence)
	{
		RtlZeroMemory (ServerThread, sizeof ServerThread);
	}
	// We can not register more than CSR_SERVER_DLL_MAX servers.
	// Note: # servers >= # DLLs (MS Win32 has 3 servers in 2 DLLs).
	if (NextInSequence >= CSR_SERVER_DLL_MAX)
	{
		return STATUS_NO_MEMORY;
	}
	// Validate the ServerIndex from the command line:
	// it may be 0, 1, 2, or 3.
	ServerIndex = pServerDll->ServerIndex;
	if (ServerIndex >= CSR_SERVER_DLL_MAX)
	{
		return STATUS_INVALID_PARAMETER;
	}
	// Register the DLL server.
	ServerThread [ServerIndex].ServerIndex = ServerIndex;
	ServerThread [ServerIndex].Sequence = NextInSequence ++;
	if (0 != ServerIndex)
	{
		RtlDuplicateUnicodeString (1, & pServerDll->DllName,       & ServerThread [ServerIndex].DllName);
		RtlDuplicateUnicodeString (1, & pServerDll->DllEntryPoint, & ServerThread [ServerIndex].DllEntryPoint);
	} else {
		// CSRSRV.DLL own static server thread
		ServerThread [ServerIndex].ServerThread = CsrSrvServerThread;
	}
	return STATUS_SUCCESS;
}
/**********************************************************************
 * CsrSrvInitializeServerDll/1					PRIVATE
 *
 * NOTE
 * 	Code dapted from CsrpInitWin32Csr.
 */
static NTSTATUS CsrSrvInitializeServerDll (USHORT ServerIndex)
{
	NTSTATUS                  Status = STATUS_SUCCESS;
	HINSTANCE                 hInst;
	ANSI_STRING               ProcName;
	CSR_SERVER_DLL_INIT_PROC  InitProc;

	DPRINT("CSRSRV: %s called\n", __FUNCTION__);

	Status = LdrLoadDll (NULL, 0, & ServerThread[ServerIndex].DllName, (PVOID *) &hInst);
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("CSRSRV:%s: loading ServerDll '%S' failed (Status=%08lx)\n",
			__FUNCTION__, ServerThread[ServerIndex].DllName.Buffer, Status);
		return Status;
	}
	RtlInitAnsiString (& ProcName, "ServerDllInitialization");
	Status = LdrGetProcedureAddress(hInst, &ProcName, 0, (PVOID *) &InitProc);
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("CSRSRV:%s: ServerDll '%S!%s' not found (Status=%08lx)\n",
			__FUNCTION__,
			ServerThread[ServerIndex].DllName.Buffer,
			"ServerDllInitialization",
			Status);
		return Status;
	}
	Status = InitProc (0, NULL);
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("CSRSRV:%s: %S.%s failed with Status=%08lx\n",
			__FUNCTION__,
			ServerThread[ServerIndex].DllName.Buffer,
			"ServerDllInitialization",
			Status);
	}
	return Status;
}

/**********************************************************************
 * CsrpCreateObjectDirectory/1					PRIVATE
 */
NTSTATUS STDCALL CsrpCreateObjectDirectory (PUNICODE_STRING pObjectDirectory)
{
	NTSTATUS           Status = STATUS_SUCCESS;
	OBJECT_ATTRIBUTES  DirectoryAttributes;

	DPRINT("CSRSRV:%s(%S) called\n", __FUNCTION__, pObjectDirectory->Buffer);

	InitializeObjectAttributes (& DirectoryAttributes,
	                            pObjectDirectory,
	                            OBJ_OPENIF,
	                            NULL,
	                            NULL);

	Status = NtCreateDirectoryObject (& CsrSrvOption.NameSpace.RootHandle,
					  (DIRECTORY_CREATE_OBJECT|DIRECTORY_CREATE_SUBDIRECTORY),
					  & DirectoryAttributes);
	if (NT_SUCCESS(Status))
	{
		Status = RtlDuplicateUnicodeString (0, pObjectDirectory, & CsrSrvOption.NameSpace.Root);
		if (!NT_SUCCESS(Status))
		{
			DPRINT1("CSRSRV:%s: RtlDuplicateUnicodeString failed (Status=0x%08lx)\n",
				__FUNCTION__, Status);
		}
	} else {
		DPRINT1("CSRSRV:%s: fatal: NtCreateDirectoryObject failed (Status=0x%08lx)\n",
				__FUNCTION__, Status);
	}	
	return Status;
}
/**********************************************************************
 * CsrSrvBootstrap/0
 *
 * DESCRIPTION
 * 	This is where a subsystem begins living.
 */
NTSTATUS STDCALL CsrSrvBootstrap (VOID)
{
	NTSTATUS Status = STATUS_SUCCESS;
	ULONG    ServerIndex = 0;
	ULONG    ServerSequence = 0;

	DPRINT("CSRSRV: %s called\n", __FUNCTION__);

	CsrSrvSbApiPortHandle = CsrSrvSbApiPortHandle; //FIXME
	
	// OBJECT DIRECTORY
	Status = CsrpCreateObjectDirectory (& CsrSrvOption.NameSpace.Root);
	if(!NT_SUCCESS(Status))
	{
		DPRINT1("CSRSRV:%s: CsrpCreateObjectDirectory failed (Status=%08lx)\n",
			__FUNCTION__, Status);
		return Status;
	}
	// SESSIONS
	Status = CsrSrvInitializeSession ();
	if(!NT_SUCCESS(Status))
	{
		DPRINT1("CSRSRV:%s: CsrSrvInitializeSession failed (Status=%08lx)\n",
			__FUNCTION__, Status);
		return Status;
	}
	// PROCESSES
	// TODO
	// THREADS
	// TODO
	// WAITS
	// TODO
	// Hosted servers
	for (ServerSequence = 0; ServerSequence < CSR_SERVER_DLL_MAX; ServerSequence ++)
	{
		for (ServerIndex = 0; (ServerIndex < CSR_SERVER_DLL_MAX); ++ ServerIndex)
		{
			if (ServerSequence == ServerThread [ServerIndex].Sequence)
			{
				if (NULL == ServerThread [ServerIndex].ServerThread)
				{
					Status = CsrSrvInitializeServerDll (ServerIndex);
					if (!NT_SUCCESS(Status))
					{
						DPRINT1("CSRSRV:%s: server thread #%d init failed!\n",
							__FUNCTION__, ServerIndex);
					}
				} else {
					DPRINT1("CSRSRV:%s: server thread #%d initialized more than once!\n",
						__FUNCTION__, ServerIndex);
				}
			}
		}
	}
	return Status;
}
/**********************************************************************
 * CsrSrvServerThread/1
 *
 * DESCRIPTION
 * 	This is actually a function called by the CsrSrvMainServerThread
 * 	when the server index is 0. Other server DLLs register their
 * 	function with CsrAddStaticServerThread.
 */
VOID STDCALL CsrSrvServerThread (PVOID x)
{
	NTSTATUS       Status = STATUS_SUCCESS;
	PPORT_MESSAGE  Request = (PPORT_MESSAGE) x;
	PPORT_MESSAGE  Reply = NULL;
	ULONG          MessageType = 0;

	DPRINT("CSRSRV: %s called\n", __FUNCTION__);

	MessageType = Request->u2.s2.Type;
	DPRINT("CSRSRV: %s received a message (Type=%d)\n",
		__FUNCTION__, MessageType);
	switch (MessageType)
	{
		//TODO
		default:
			Reply = Request;
			Status = NtReplyPort (CsrSrvApiPortHandle, Reply);
			break;
	}
}

/**********************************************************************
 * 	PUBLIC API
 *********************************************************************/

/**********************************************************************
 * CsrAddStaticServerThread/1
 */
NTSTATUS STDCALL CsrAddStaticServerThread (CSR_SERVER_THREAD ServerThread)
{
	static ULONG StaticServerThreadCount = 0;
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s(%08lx) called\n", __FUNCTION__, ServerThread);

	if (StaticServerThreadCount > CSR_SERVER_DLL_MAX)
	{
		DPRINT1("CSRSRV: subsystem tries to add mode than %d static threads!\n",
			CSR_SERVER_DLL_MAX);
		return STATUS_NO_MEMORY;
	}
	if (NT_SUCCESS(Status))
	{
		// FIXME: do we need to make it reentrant?
		++ StaticServerThreadCount;
	}
	return Status;
}

/**********************************************************************
 * CsrCallServerFromServer
 */
NTSTATUS STDCALL CsrCallServerFromServer ()
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	return Status;
}

/**********************************************************************
 * CsrExecServerThread
 */
NTSTATUS STDCALL CsrExecServerThread ()
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	return Status;
}

/**********************************************************************
 * CsrImpersonateClient
 */
NTSTATUS STDCALL CsrImpersonateClient ()
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	return Status;
}

/**********************************************************************
 * CsrQueryApiPort/0
 *
 * @implemented
 */
HANDLE STDCALL CsrQueryApiPort (VOID)
{
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	return CsrSrvApiPortHandle;
}

/**********************************************************************
 * CsrRevertToSelf
 */
NTSTATUS STDCALL CsrRevertToSelf ()
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	return Status;
}

/**********************************************************************
 * CsrSetBackgroundPriority
 */
NTSTATUS STDCALL CsrSetBackgroundPriority ()
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	return Status;
}

/**********************************************************************
 * CsrSetCallingSpooler
 */
NTSTATUS STDCALL CsrSetCallingSpooler ()
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	return Status;
}

/**********************************************************************
 * CsrSetForegroundPriority
 */
NTSTATUS STDCALL CsrSetForegroundPriority ()
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	return Status;
}

/**********************************************************************
 * CsrUnhandledExceptionFilter
 */
NTSTATUS STDCALL CsrUnhandledExceptionFilter ()
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	return Status;
}

/**********************************************************************
 * CsrValidateMessageBuffer
 */
NTSTATUS STDCALL CsrValidateMessageBuffer ()
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	return Status;
}

/**********************************************************************
 * CsrValidateMessageString
 */
NTSTATUS STDCALL CsrValidateMessageString ()
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	return Status;
}

/* EOF */
