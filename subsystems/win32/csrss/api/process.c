/*
 * reactos/subsys/csrss/api/process.c
 *
 * "\windows\ApiPort" port process management functions
 *
 * ReactOS Operating System
 */

/* INCLUDES ******************************************************************/

#include <csrss.h>

#define NDEBUG
#include <debug.h>

#define LOCK   RtlEnterCriticalSection(&ProcessDataLock)
#define UNLOCK RtlLeaveCriticalSection(&ProcessDataLock)

/* GLOBALS *******************************************************************/

static ULONG NrProcess;
static PCSRSS_PROCESS_DATA ProcessData[256];
RTL_CRITICAL_SECTION ProcessDataLock;

/* FUNCTIONS *****************************************************************/

VOID STDCALL CsrInitProcessData(VOID)
{
   RtlZeroMemory (ProcessData, sizeof ProcessData);
   NrProcess = sizeof ProcessData / sizeof ProcessData[0];
   RtlInitializeCriticalSection( &ProcessDataLock );
}

PCSRSS_PROCESS_DATA STDCALL CsrGetProcessData(HANDLE ProcessId)
{
   ULONG hash;
   PCSRSS_PROCESS_DATA pProcessData;

   hash = ((ULONG_PTR)ProcessId >> 2) % (sizeof(ProcessData) / sizeof(*ProcessData));

   LOCK;

   pProcessData = ProcessData[hash];

   while (pProcessData && pProcessData->ProcessId != ProcessId)
   {
      pProcessData = pProcessData->next;
   }
   UNLOCK;
   return pProcessData;
}

PCSRSS_PROCESS_DATA STDCALL CsrCreateProcessData(HANDLE ProcessId)
{
   ULONG hash;
   PCSRSS_PROCESS_DATA pProcessData;
   OBJECT_ATTRIBUTES ObjectAttributes;
   CLIENT_ID ClientId;
   NTSTATUS Status;

   hash = ((ULONG_PTR)ProcessId >> 2) % (sizeof(ProcessData) / sizeof(*ProcessData));

   LOCK;

   pProcessData = ProcessData[hash];

   while (pProcessData && pProcessData->ProcessId != ProcessId)
   {
      pProcessData = pProcessData->next;
   }
   if (pProcessData == NULL)
   {
      pProcessData = RtlAllocateHeap(CsrssApiHeap,
	                             HEAP_ZERO_MEMORY,
				     sizeof(CSRSS_PROCESS_DATA));
      if (pProcessData)
      {
	 pProcessData->ProcessId = ProcessId;
	 pProcessData->next = ProcessData[hash];
	 ProcessData[hash] = pProcessData;

         ClientId.UniqueThread = NULL;
         ClientId.UniqueProcess = pProcessData->ProcessId;
         InitializeObjectAttributes(&ObjectAttributes,
                                    NULL,
                                    0,
                                    NULL,
                                    NULL);

         /* using OpenProcess is not optimal due to HANDLE vs. DWORD PIDs... */
         Status = NtOpenProcess(&pProcessData->Process,
                                PROCESS_DUP_HANDLE | PROCESS_VM_OPERATION |
                                PROCESS_VM_WRITE | PROCESS_CREATE_THREAD | SYNCHRONIZE,
                                &ObjectAttributes,
                                &ClientId);
         if (!NT_SUCCESS(Status))
         {
            ProcessData[hash] = pProcessData->next;
	    RtlFreeHeap(CsrssApiHeap, 0, pProcessData);
	    pProcessData = NULL;
         }
         else
         {
            RtlInitializeCriticalSection(&pProcessData->HandleTableLock);
         }
      }
   }
   else
   {
      DPRINT1("Process data for pid %d already exist\n", ProcessId);
   }
   UNLOCK;
   if (pProcessData == NULL)
   {
      DPRINT1("CsrCreateProcessData() failed\n");
   }
   else
   {
      pProcessData->Terminated = FALSE;

      /* Set default shutdown parameters */
      pProcessData->ShutdownLevel = 0x280;
      pProcessData->ShutdownFlags = 0;
   }
   return pProcessData;
}

NTSTATUS STDCALL CsrFreeProcessData(HANDLE Pid)
{
  ULONG hash;
  UINT c;
  PCSRSS_PROCESS_DATA pProcessData, pPrevProcessData = NULL;

  hash = ((ULONG_PTR)Pid >> 2) % (sizeof(ProcessData) / sizeof(*ProcessData));

  LOCK;

  pProcessData = ProcessData[hash];

  while (pProcessData && pProcessData->ProcessId != Pid)
    {
      pPrevProcessData = pProcessData;
      pProcessData = pProcessData->next;
    }

  if (pProcessData)
    {
      DPRINT("CsrFreeProcessData pid: %d\n", Pid);
      if (pProcessData->Process)
      {
         NtClose(pProcessData->Process);
      }
      if (pProcessData->Console)
        {
          RemoveEntryList(&pProcessData->ProcessEntry);
        }
      if (pProcessData->HandleTable)
        {
          for (c = 0; c < pProcessData->HandleTableSize; c++)
            {
              if (pProcessData->HandleTable[c])
                {
                  CsrReleaseObject(pProcessData, (HANDLE)(((c + 1) << 2)|0x3));
                }
            }
          RtlFreeHeap(CsrssApiHeap, 0, pProcessData->HandleTable);
        }
      RtlDeleteCriticalSection(&pProcessData->HandleTableLock);
      if (pProcessData->Console)
        {
          CsrReleaseObjectByPointer((Object_t *) pProcessData->Console);
        }
      if (pProcessData->CsrSectionViewBase)
        {
          NtUnmapViewOfSection(NtCurrentProcess(), pProcessData->CsrSectionViewBase);
        }
      if (pProcessData->ServerCommunicationPort)
        {
          NtClose(pProcessData->ServerCommunicationPort);
        }
      if (pPrevProcessData)
        {
          pPrevProcessData->next = pProcessData->next;
        }
      else
        {
          ProcessData[hash] = pProcessData->next;
        }

      RtlFreeHeap(CsrssApiHeap, 0, pProcessData);
      UNLOCK;
      return STATUS_SUCCESS;
   }

   UNLOCK;
   return STATUS_INVALID_PARAMETER;
}

NTSTATUS STDCALL
CsrEnumProcesses(CSRSS_ENUM_PROCESS_PROC EnumProc, PVOID Context)
{
  UINT Hash;
  PCSRSS_PROCESS_DATA pProcessData;
  NTSTATUS Status = STATUS_SUCCESS;

  LOCK;

  for (Hash = 0; Hash < (sizeof(ProcessData) / sizeof(*ProcessData)); Hash++)
    {
      pProcessData = ProcessData[Hash];
      while (NULL != pProcessData)
        {
          Status = EnumProc(pProcessData, Context);
          if (STATUS_SUCCESS != Status)
            {
              UNLOCK;
              return Status;
            }
          pProcessData = pProcessData->next;
        }
    }

  UNLOCK;

  return Status;
}

/**********************************************************************
 *	CSRSS API
 *********************************************************************/

CSR_API(CsrCreateProcess)
{
   PCSRSS_PROCESS_DATA NewProcessData;
   NTSTATUS Status;

   Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
   Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);

   NewProcessData = CsrCreateProcessData(Request->Data.CreateProcessRequest.NewProcessId);
   if (NewProcessData == NULL)
     {
	Request->Status = STATUS_NO_MEMORY;
	return(STATUS_NO_MEMORY);
     }

   if (!(Request->Data.CreateProcessRequest.Flags & (CREATE_NEW_CONSOLE|DETACHED_PROCESS)))
     {
       NewProcessData->ParentConsole = ProcessData->Console;
       NewProcessData->bInheritHandles = Request->Data.CreateProcessRequest.bInheritHandles;
       if (Request->Data.CreateProcessRequest.bInheritHandles)
         {
           Status = CsrDuplicateHandleTable(ProcessData, NewProcessData);
         }
     }

   /* Set default shutdown parameters */
   NewProcessData->ShutdownLevel = 0x280;
   NewProcessData->ShutdownFlags = 0;

   Request->Status = STATUS_SUCCESS;
   return(STATUS_SUCCESS);
}

CSR_API(CsrTerminateProcess)
{
   Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
   Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

   if (ProcessData == NULL)
   {
      return(Request->Status = STATUS_INVALID_PARAMETER);
   }

   ProcessData->Terminated = TRUE;
   Request->Status = STATUS_SUCCESS;
   return STATUS_SUCCESS;
}

CSR_API(CsrConnectProcess)
{
   Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
   Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

   Request->Status = STATUS_SUCCESS;

   return(STATUS_SUCCESS);
}

CSR_API(CsrGetShutdownParameters)
{
  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  if (ProcessData == NULL)
  {
     return(Request->Status = STATUS_INVALID_PARAMETER);
  }

  Request->Data.GetShutdownParametersRequest.Level = ProcessData->ShutdownLevel;
  Request->Data.GetShutdownParametersRequest.Flags = ProcessData->ShutdownFlags;

  Request->Status = STATUS_SUCCESS;

  return(STATUS_SUCCESS);
}

CSR_API(CsrSetShutdownParameters)
{
  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  if (ProcessData == NULL)
  {
     return(Request->Status = STATUS_INVALID_PARAMETER);
  }

  ProcessData->ShutdownLevel = Request->Data.SetShutdownParametersRequest.Level;
  ProcessData->ShutdownFlags = Request->Data.SetShutdownParametersRequest.Flags;

  Request->Status = STATUS_SUCCESS;

  return(STATUS_SUCCESS);
}

CSR_API(CsrGetInputHandle)
{
   Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
   Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

   if (ProcessData == NULL)
   {
      Request->Data.GetInputHandleRequest.InputHandle = INVALID_HANDLE_VALUE;
      Request->Status = STATUS_INVALID_PARAMETER;
   }
   else if (ProcessData->Console)
   {
      Request->Status = CsrInsertObject(ProcessData,
		                      &Request->Data.GetInputHandleRequest.InputHandle,
		                      (Object_t *)ProcessData->Console);
   }
   else
   {
      Request->Data.GetInputHandleRequest.InputHandle = INVALID_HANDLE_VALUE;
      Request->Status = STATUS_SUCCESS;
   }

   return Request->Status;
}

CSR_API(CsrGetOutputHandle)
{
   Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
   Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

   if (ProcessData == NULL)
   {
      Request->Data.GetOutputHandleRequest.OutputHandle = INVALID_HANDLE_VALUE;
      Request->Status = STATUS_INVALID_PARAMETER;
   }
   else if (ProcessData->Console)
   {
      RtlEnterCriticalSection(&ProcessDataLock);
      Request->Status = CsrInsertObject(ProcessData,
                                      &Request->Data.GetOutputHandleRequest.OutputHandle,
                                      &(ProcessData->Console->ActiveBuffer->Header));
      RtlLeaveCriticalSection(&ProcessDataLock);
   }
   else
   {
      Request->Data.GetOutputHandleRequest.OutputHandle = INVALID_HANDLE_VALUE;
      Request->Status = STATUS_SUCCESS;
   }

   return Request->Status;
}

CSR_API(CsrCloseHandle)
{
   Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
   Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

   if (ProcessData == NULL)
   {
      Request->Status = STATUS_INVALID_PARAMETER;
   }
   else
   {
      Request->Status = CsrReleaseObject(ProcessData, Request->Data.CloseHandleRequest.Handle);
   }
   return Request->Status;
}

CSR_API(CsrVerifyHandle)
{
   Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
   Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

   Request->Status = CsrVerifyObject(ProcessData, Request->Data.VerifyHandleRequest.Handle);
   if (!NT_SUCCESS(Request->Status))
   {
      DPRINT("CsrVerifyObject failed, status=%x\n", Request->Status);
   }

   return Request->Status;
}

CSR_API(CsrDuplicateHandle)
{
  Object_t *Object;

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  ProcessData = CsrGetProcessData(Request->Data.DuplicateHandleRequest.ProcessId);
  if (NULL == ProcessData || ProcessData->Terminated)
    {
      DPRINT1("Invalid source process %d\n", Request->Data.DuplicateHandleRequest.ProcessId);
      Request->Status = STATUS_INVALID_PARAMETER;
      return Request->Status;
    }

  Request->Status = CsrGetObject(ProcessData, Request->Data.DuplicateHandleRequest.Handle, &Object);
  if (! NT_SUCCESS(Request->Status))
    {
      DPRINT("CsrGetObject failed, status=%x\n", Request->Status);
    }
  else
    {
      Request->Status = CsrInsertObject(ProcessData,
                                      &Request->Data.DuplicateHandleRequest.Handle,
                                      Object);
    }
  return Request->Status;
}

CSR_API(CsrGetInputWaitHandle)
{
  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  if (ProcessData == NULL)
  {

     Request->Data.GetConsoleInputWaitHandle.InputWaitHandle = INVALID_HANDLE_VALUE;
     Request->Status = STATUS_INVALID_PARAMETER;
  }
  else
  {
     Request->Data.GetConsoleInputWaitHandle.InputWaitHandle = ProcessData->ConsoleEvent;
     Request->Status = STATUS_SUCCESS;
  }
  return Request->Status;
}

/* EOF */
