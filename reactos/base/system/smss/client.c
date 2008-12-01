/*
 * PROJECT:         ReactOS Session Manager
 * LICENSE:         GPL v2 or later - See COPYING in the top level directory
 * FILE:            base/system/smss/client.c
 * PURPOSE:         Client management.
 * PROGRAMMERS:     ReactOS Development Team
 */

/* INCLUDES ******************************************************************/
#include "smss.h"
#include <sm/helper.h>

#define NDEBUG
#include <debug.h>

/* Private ADT */

#define SM_MAX_CLIENT_COUNT 16
#define SM_INVALID_CLIENT_INDEX -1

struct _SM_CLIENT_DIRECTORY
{
	RTL_CRITICAL_SECTION  Lock;
	ULONG                 Count;
	PSM_CLIENT_DATA       Client [SM_MAX_CLIENT_COUNT];
	PSM_CLIENT_DATA       CandidateClient;

} SmpClientDirectory;


/**********************************************************************
 *	SmInitializeClientManagement/0
 */
NTSTATUS
SmInitializeClientManagement (VOID)
{
	DPRINT("SM: %s called\n", __FUNCTION__);
	RtlInitializeCriticalSection(& SmpClientDirectory.Lock);
	SmpClientDirectory.Count = 0;
	RtlZeroMemory (SmpClientDirectory.Client, sizeof SmpClientDirectory.Client);
	SmpClientDirectory.CandidateClient = NULL;
	return STATUS_SUCCESS;

}
/**********************************************************************
 * SmpSetClientInitialized/1
 */
VOID FASTCALL
SmpSetClientInitialized (PSM_CLIENT_DATA Client)
{
	DPRINT("SM: %s(%p) called\n", __FUNCTION__, Client);
	Client->Flags |= SM_CLIENT_FLAG_INITIALIZED;
}
/**********************************************************************
 * SmpGetFirstFreeClientEntry/0					PRIVATE
 *
 * NOTE: call it holding SmpClientDirectory.Lock only
 */
static INT NTAPI SmpGetFirstFreeClientEntry (VOID)
{
	INT ClientIndex = 0;

	DPRINT("SM: %s called\n", __FUNCTION__);

	if (SmpClientDirectory.Count < SM_MAX_CLIENT_COUNT)
	{
		for (ClientIndex = 0;
			(ClientIndex < SM_MAX_CLIENT_COUNT);
			ClientIndex ++)
		{
			if (NULL == SmpClientDirectory.Client[ClientIndex])
			{
				DPRINT("SM: %s => %d\n", __FUNCTION__, ClientIndex);
				return ClientIndex; // found
			}
		}
	}
	return SM_INVALID_CLIENT_INDEX; // full!
}
/**********************************************************************
 *	SmpLookupClient/1					PRIVATE
 *
 * DESCRIPTION
 * 	Lookup the subsystem server descriptor (client data) given its
 * 	base image ID.
 *
 * ARGUMENTS
 *	SubsystemId: IMAGE_SUBSYSTEM_xxx
 *
 * RETURN VALUES
 *	SM_INVALID_CLIENT_INDEX on error;
 *	otherwise an index in the range (0..SM_MAX_CLIENT_COUNT).
 *
 * WARNING
 * 	SmpClientDirectory.Lock must be held by the caller.
 */
static INT FASTCALL
SmpLookupClient (USHORT SubsystemId)
{
	INT  ClientIndex = 0;

	DPRINT("SM: %s(%d) called\n", __FUNCTION__, SubsystemId);

	if (0 != SmpClientDirectory.Count)
	{
		for (ClientIndex = 0; (ClientIndex < SM_MAX_CLIENT_COUNT); ClientIndex ++)
		{
			if (NULL != SmpClientDirectory.Client[ClientIndex])
			{
				if (SubsystemId == SmpClientDirectory.Client[ClientIndex]->SubsystemId)
				{
					return ClientIndex;
				}
			}
		}
	}
	return SM_INVALID_CLIENT_INDEX;
}
/**********************************************************************
 * 	SmpDestroyClientObject/2				PRIVATE
 *
 * WARNING
 * 	SmpClientDirectory.Lock must be held by the caller.
 */
static NTSTATUS NTAPI
SmpDestroyClientObject (PSM_CLIENT_DATA Client, NTSTATUS DestroyReason)
{
	DPRINT("SM:%s(%p,%08lx) called\n", __FUNCTION__, Client, DestroyReason);
	/* TODO: send shutdown to the SB port */
	NtTerminateProcess (Client->ServerProcess, DestroyReason);
	RtlFreeHeap (SmpHeap, 0, Client);
	-- SmpClientDirectory.Count;
	return STATUS_SUCCESS;
}
/**********************************************************************
 * 	SmBeginClientInitialization/1
 *
 * DESCRIPTION
 * 	Check if the candidate client matches the begin session
 * 	message from the subsystem process.
 *
 * ARGUMENTS
 *	Request: message received by \SmApiPort
 *	ClientData:
 *
 * RETURN VALUES
 *	NTSTATUS
 */
NTSTATUS NTAPI
SmBeginClientInitialization (IN  PSM_PORT_MESSAGE Request,
			     OUT PSM_CLIENT_DATA  * ClientData)
{
	NTSTATUS          Status = STATUS_SUCCESS;
	PSM_CONNECT_DATA  ConnectData = SmpGetConnectData (Request);
	ULONG             SbApiPortNameSize = SM_CONNECT_DATA_SIZE(*Request);
	INT               ClientIndex = SM_INVALID_CLIENT_INDEX;
    HANDLE Process;


	DPRINT("SM: %s(%p,%p) called\n", __FUNCTION__,
			Request, ClientData);

	RtlEnterCriticalSection (& SmpClientDirectory.Lock);
	/*
	 * Is there a subsystem bootstrap in progress?
	 */
	if (NULL != SmpClientDirectory.CandidateClient)
	{
		PROCESS_BASIC_INFORMATION pbi;
        OBJECT_ATTRIBUTES ObjectAttributes;

		RtlZeroMemory (& pbi, sizeof pbi);
        InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
        Status = NtOpenProcess(&Process,
                               PROCESS_ALL_ACCESS,
                               &ObjectAttributes,
                               &Request->Header.ClientId);
        ASSERT(NT_SUCCESS(Status));
		Status = NtQueryInformationProcess (Process,
					    	    ProcessBasicInformation,
						    & pbi,
						    sizeof pbi,
						    NULL);
		ASSERT(NT_SUCCESS(Status));
		{
			SmpClientDirectory.CandidateClient->ServerProcessId =
				(ULONG) pbi.UniqueProcessId;
		}
	}
	else
	{
		DPRINT1("SM: %s: subsys booting with no descriptor!\n", __FUNCTION__);
		Status = STATUS_NOT_FOUND;
		RtlLeaveCriticalSection (& SmpClientDirectory.Lock);
		return Status;
	}
	/*
	 * Check if a client for the ID already exist.
	 */
	if (SM_INVALID_CLIENT_INDEX != SmpLookupClient(ConnectData->SubSystemId))
	{
		DPRINT("SM: %s: attempt to register again subsystem %d.\n",
			__FUNCTION__,
			ConnectData->SubSystemId);
		// TODO something else to do here?
		RtlLeaveCriticalSection (& SmpClientDirectory.Lock);
		return STATUS_UNSUCCESSFUL;
	}
	/*
	 * Check if a free entry exists in SmpClientDirectory.Client[].
	 */
	ClientIndex = SmpGetFirstFreeClientEntry();
	if (SM_INVALID_CLIENT_INDEX == ClientIndex)
	{
		DPRINT("SM: %s: SM_INVALID_CLIENT_INDEX == ClientIndex ", __FUNCTION__);
		SmpDestroyClientObject (SmpClientDirectory.CandidateClient, STATUS_NO_MEMORY);
		SmpClientDirectory.CandidateClient = NULL;
		return STATUS_NO_MEMORY;
	}

	/* OK! */
	DPRINT("SM: %s: registering subsystem ID=%d \n",
		__FUNCTION__, ConnectData->SubSystemId);

	/*
	 * Initialize the client data
	 */
	SmpClientDirectory.CandidateClient->SubsystemId = ConnectData->SubSystemId;
	/* SM && DBG auto-initializes; other subsystems are required to call
	 * SM_API_COMPLETE_SESSION via SMDLL. */
	if ((IMAGE_SUBSYSTEM_NATIVE == SmpClientDirectory.CandidateClient->SubsystemId) ||
	    ((USHORT)-1 == SmpClientDirectory.CandidateClient->SubsystemId))
	{
		SmpSetClientInitialized (SmpClientDirectory.CandidateClient);
	}
	if (SbApiPortNameSize > 0)
	{
		/* Only external servers have an SB port */
		RtlCopyMemory (SmpClientDirectory.CandidateClient->SbApiPortName,
			       ConnectData->SbName,
			       SbApiPortNameSize);
	}
	/*
	 * Insert the new descriptor in the
	 * client directory.
	 */
	SmpClientDirectory.Client [ClientIndex] = SmpClientDirectory.CandidateClient;
	/*
	 * Increment the number of active subsystems.
	 */
	++ SmpClientDirectory.Count;
	/*
	 * Notify to the caller the reference to the client data.
	 */
	if (ClientData)
	{
		*ClientData = SmpClientDirectory.CandidateClient;
	}
	/*
	 * Free the slot for the candidate subsystem.
	 */
	SmpClientDirectory.CandidateClient = NULL;

	/* Done */
	RtlLeaveCriticalSection (& SmpClientDirectory.Lock);

	return STATUS_SUCCESS;
}
/**********************************************************************
 *	SmCompleteClientInitialization/1
 *
 * DESCRIPTION
 * 	Lookup the subsystem server descriptor given the process ID
 * 	of the subsystem server process.
 */
NTSTATUS NTAPI
SmCompleteClientInitialization (ULONG ProcessId)
{
	NTSTATUS  Status = STATUS_NOT_FOUND;
	INT       ClientIndex = SM_INVALID_CLIENT_INDEX;

	DPRINT("SM: %s(%lu) called\n", __FUNCTION__, ProcessId);

	RtlEnterCriticalSection (& SmpClientDirectory.Lock);
	if (SmpClientDirectory.Count > 0)
	{
		for (ClientIndex = 0; ClientIndex < SM_MAX_CLIENT_COUNT; ClientIndex ++)
		{
			if ((NULL != SmpClientDirectory.Client [ClientIndex]) &&
				(ProcessId == SmpClientDirectory.Client [ClientIndex]->ServerProcessId))
			{
				SmpSetClientInitialized (SmpClientDirectory.Client [ClientIndex]);
				Status = STATUS_SUCCESS;
				break;
			}
		}
	}
	RtlLeaveCriticalSection (& SmpClientDirectory.Lock);
	return Status;
}
/**********************************************************************
 * 	SmpDestroyClientByClientIndex/1				PRIVATE
 */
static NTSTATUS NTAPI
SmpDestroyClientByClientIndex (INT ClientIndex)
{
	NTSTATUS         Status = STATUS_SUCCESS;
	PSM_CLIENT_DATA  Client = NULL;

	DPRINT("SM: %s(%d) called\n", __FUNCTION__, ClientIndex);

	if (SM_INVALID_CLIENT_INDEX == ClientIndex)
	{
		DPRINT1("SM: %s: SM_INVALID_CLIENT_INDEX == ClientIndex!\n",
			__FUNCTION__);
		Status = STATUS_NOT_FOUND;
	}
	else
	{
		Client = SmpClientDirectory.Client [ClientIndex];
		SmpClientDirectory.Client [ClientIndex] = NULL;
		if (NULL != Client)
		{
			Status = SmpDestroyClientObject (Client, STATUS_SUCCESS);
		} else {
			DPRINT("SM:%s: NULL == Client[%d]!\n", __FUNCTION__,
				ClientIndex);
			Status = STATUS_UNSUCCESSFUL;
		}
	}
	return Status;
}
/**********************************************************************
 *	SmpTimeoutCandidateClient/1
 *
 * DESCRIPTION
 * 	Give the candidate client time to bootstrap and complete
 * 	session initialization. If the client fails in any way,
 * 	drop the pending client and kill the process.
 *
 * ARGUMENTS
 * 	x: HANDLE for the candidate process.
 *
 * RETURN VALUE
 * 	NONE.
 */
static VOID NTAPI SmpTimeoutCandidateClient (PVOID x)
{
	NTSTATUS       Status = STATUS_SUCCESS;
	HANDLE         CandidateClientProcessHandle = (HANDLE) x;
	LARGE_INTEGER  TimeOut;

	DPRINT("SM: %s(%p) called\n", __FUNCTION__, x);

	TimeOut.QuadPart = (LONGLONG) -300000000L; // 30s
	Status = NtWaitForSingleObject (CandidateClientProcessHandle,
					FALSE,
					& TimeOut);
	if (STATUS_TIMEOUT == Status)
	{
		RtlEnterCriticalSection (& SmpClientDirectory.Lock);
		if (NULL != SmpClientDirectory.CandidateClient)
		{
			DPRINT("SM:%s: destroy candidate %p\n", __FUNCTION__,
					SmpClientDirectory.CandidateClient);
			Status = SmpDestroyClientObject (SmpClientDirectory.CandidateClient,
							 STATUS_TIMEOUT);
			SmpClientDirectory.CandidateClient = NULL;
		}
		RtlLeaveCriticalSection (& SmpClientDirectory.Lock);
	}
	NtTerminateThread (NtCurrentThread(), Status);
}
/**********************************************************************
 *	SmpCreateClient/1
 *
 * DESCRIPTION
 * 	Create a "candidate" client. Client descriptor will enter the
 * 	client directory only at the end of the registration
 * 	procedure. Otherwise, we will kill the associated process.
 *
 * ARGUMENTS
 * 	ProcessHandle: handle of the subsystem server process.
 *
 * RETURN VALUE
 * 	NTSTATUS:
 * 		STATUS_SUCCESS if all OK;
 * 		STATUS_DEVICE_BUSY if another SS is still booting;
 * 		STATUS_NO_MEMORY if client descriptor allocation failed;
 *
 *
 */
NTSTATUS NTAPI
SmCreateClient (PRTL_USER_PROCESS_INFORMATION ProcessInfo, PWSTR ProgramName)
{
	NTSTATUS Status = STATUS_SUCCESS;

	DPRINT("SM: %s(%p, %S) called\n", __FUNCTION__, ProcessInfo->ProcessHandle, ProgramName);

	RtlEnterCriticalSection (& SmpClientDirectory.Lock);
	/*
	 * Check if the candidate client slot is empty.
	 */
	if (NULL == SmpClientDirectory.CandidateClient)
	{
		/*
		 * Check if there exist a free entry in the
		 * SmpClientDirectory.Client array.
		 */
		if (SM_INVALID_CLIENT_INDEX == SmpGetFirstFreeClientEntry())
		{
			DPRINT("SM: %s(%p): out of memory!\n",
				__FUNCTION__, ProcessInfo->ProcessHandle);
			Status = STATUS_NO_MEMORY;
		}
		/*
		 * Allocate the storage for client data
		 */
		SmpClientDirectory.CandidateClient =
			RtlAllocateHeap (SmpHeap,
					 HEAP_ZERO_MEMORY,
					 sizeof (SM_CLIENT_DATA));
		if (NULL == SmpClientDirectory.CandidateClient)
		{
			DPRINT("SM: %s(%p): out of memory!\n",
				__FUNCTION__, ProcessInfo->ProcessHandle);
			Status = STATUS_NO_MEMORY;
		}
		else
		{
			DPRINT("SM:%s(%p,%S): candidate is %p\n", __FUNCTION__,
					ProcessInfo, ProgramName, SmpClientDirectory.CandidateClient);
			/* Initialize the candidate client. */
			RtlInitializeCriticalSection(& SmpClientDirectory.CandidateClient->Lock);
			SmpClientDirectory.CandidateClient->ServerProcess =
				(HANDLE) ProcessInfo->ProcessHandle;
			SmpClientDirectory.CandidateClient->ServerProcessId =
				(ULONG) ProcessInfo->ClientId.UniqueProcess;
			/*
			 * Copy the program name
			 */
			RtlCopyMemory (SmpClientDirectory.CandidateClient->ProgramName,
				       ProgramName,
				       SM_SB_NAME_MAX_LENGTH);
		}
	} else {
		DPRINT1("SM: %s: CandidateClient %p pending!\n", __FUNCTION__,
				SmpClientDirectory.CandidateClient);
		Status = STATUS_DEVICE_BUSY;
	}

	RtlLeaveCriticalSection (& SmpClientDirectory.Lock);

	/* Create the timeout thread for external subsystems */
	if (_wcsicmp (ProgramName, L"Session Manager") && _wcsicmp (ProgramName, L"Debug"))
	{
		Status = RtlCreateUserThread (NtCurrentProcess(),
						NULL,
						FALSE,
						0,
						0,
						0,
						(PTHREAD_START_ROUTINE) SmpTimeoutCandidateClient,
						SmpClientDirectory.CandidateClient->ServerProcess,
						NULL,
						NULL);
		if (!NT_SUCCESS(Status))
		{
			DPRINT1("SM:%s: RtlCreateUserThread() failed (Status=%08lx)\n",
				__FUNCTION__, Status);
		}
	}

	return Status;
}
/**********************************************************************
 * 	SmpDestroyClient/1
 *
 * 	1. close any handle
 * 	2. kill client process
 * 	3. release resources
 */
NTSTATUS NTAPI
SmDestroyClient (ULONG SubsystemId)
{
	NTSTATUS  Status = STATUS_SUCCESS;
	INT       ClientIndex = SM_INVALID_CLIENT_INDEX;

	DPRINT("SM: %s(%lu) called\n", __FUNCTION__, SubsystemId);

	RtlEnterCriticalSection (& SmpClientDirectory.Lock);
	ClientIndex = SmpLookupClient (SubsystemId);
	if (SM_INVALID_CLIENT_INDEX == ClientIndex)
	{
		DPRINT1("SM: %s: del req for non existent subsystem (id=%d)\n",
			__FUNCTION__, SubsystemId);
		return STATUS_NOT_FOUND;
	}
	Status = SmpDestroyClientByClientIndex (ClientIndex);
	RtlLeaveCriticalSection (& SmpClientDirectory.Lock);
	return Status;
}

/* === Utilities for SmQryInfo === */

/**********************************************************************
 * SmGetClientBasicInformation/1
 */
NTSTATUS FASTCALL
SmGetClientBasicInformation (PSM_BASIC_INFORMATION i)
{
	INT  ClientIndex = 0;
	INT  Index = 0;

	DPRINT("SM: %s(%p) called\n", __FUNCTION__, i);

	RtlEnterCriticalSection (& SmpClientDirectory.Lock);

	i->SubSystemCount = SmpClientDirectory.Count;
	i->Unused = 0;

	if (SmpClientDirectory.Count > 0)
	{
		for (ClientIndex = 0; (ClientIndex < SM_MAX_CLIENT_COUNT); ClientIndex ++)
		{
			if ((NULL != SmpClientDirectory.Client [ClientIndex]) &&
				(Index < SM_QRYINFO_MAX_SS_COUNT))
			{
				i->SubSystem[Index].Id        = SmpClientDirectory.Client [ClientIndex]->SubsystemId;
				i->SubSystem[Index].Flags     = SmpClientDirectory.Client [ClientIndex]->Flags;
				i->SubSystem[Index].ProcessId = SmpClientDirectory.Client [ClientIndex]->ServerProcessId;
				++ Index;
			}
		}
	}

	RtlLeaveCriticalSection (& SmpClientDirectory.Lock);
	return STATUS_SUCCESS;
}

/**********************************************************************
 * SmGetSubSystemInformation/1
 */
NTSTATUS FASTCALL
SmGetSubSystemInformation (PSM_SUBSYSTEM_INFORMATION i)
{
	NTSTATUS  Status = STATUS_SUCCESS;
	INT       ClientIndex = SM_INVALID_CLIENT_INDEX;

	DPRINT("SM: %s(%p) called\n", __FUNCTION__, i);

	RtlEnterCriticalSection (& SmpClientDirectory.Lock);
	ClientIndex = SmpLookupClient (i->SubSystemId);
	if (SM_INVALID_CLIENT_INDEX == ClientIndex)
	{
		Status = STATUS_NOT_FOUND;
	}
	else
	{
		i->Flags     = SmpClientDirectory.Client [ClientIndex]->Flags;
		i->ProcessId = SmpClientDirectory.Client [ClientIndex]->ServerProcessId;
		RtlCopyMemory (i->NameSpaceRootNode,
				SmpClientDirectory.Client [ClientIndex]->SbApiPortName,
				(SM_QRYINFO_MAX_ROOT_NODE * sizeof(i->NameSpaceRootNode[0])));
	}
	RtlLeaveCriticalSection (& SmpClientDirectory.Lock);
	return Status;
}

/* EOF */
