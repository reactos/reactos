/* $Id: process.c,v 1.35 2004/06/27 12:21:31 weiden Exp $
 *
 * reactos/subsys/csrss/api/process.c
 *
 * "\windows\ApiPort" port process management functions
 *
 * ReactOS Operating System
 */

/* INCLUDES ******************************************************************/

#include <csrss/csrss.h>
#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include "api.h"
#include "conio.h"

#define NDEBUG
#include <debug.h>

#define LOCK   RtlEnterCriticalSection(&ProcessDataLock)
#define UNLOCK RtlLeaveCriticalSection(&ProcessDataLock)

/* GLOBALS *******************************************************************/

static ULONG NrProcess;
static PCSRSS_PROCESS_DATA ProcessData[256];
CRITICAL_SECTION ProcessDataLock;

/* FUNCTIONS *****************************************************************/

VOID STDCALL CsrInitProcessData(VOID)
{
   RtlZeroMemory (ProcessData, sizeof ProcessData);
   NrProcess = sizeof ProcessData / sizeof ProcessData[0];
   RtlInitializeCriticalSection( &ProcessDataLock );
}

PCSRSS_PROCESS_DATA STDCALL CsrGetProcessData(ULONG ProcessId)
{
   ULONG hash;
   PCSRSS_PROCESS_DATA pProcessData;

   hash = ProcessId % (sizeof(ProcessData) / sizeof(*ProcessData));
   
   LOCK;

   pProcessData = ProcessData[hash];

   while (pProcessData && pProcessData->ProcessId != ProcessId)
   {
      pProcessData = pProcessData->next;
   }
   UNLOCK;
   return pProcessData;
}

PCSRSS_PROCESS_DATA STDCALL CsrCreateProcessData(ULONG ProcessId)
{
   ULONG hash;
   PCSRSS_PROCESS_DATA pProcessData;

   hash = ProcessId % (sizeof(ProcessData) / sizeof(*ProcessData));
   
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
      }
   }
   else
   {
      DPRINT("Process data for pid %d already exist\n", ProcessId);
   }
   UNLOCK;
   if (pProcessData == NULL)
   {
      DbgPrint("CSR: CsrGetProcessData() failed\n");
   }
   return pProcessData;
}

NTSTATUS STDCALL CsrFreeProcessData(ULONG Pid)
{
  ULONG hash;
  int c;
  PCSRSS_PROCESS_DATA pProcessData, pPrevProcessData = NULL;
   
  hash = Pid % (sizeof(ProcessData) / sizeof(*ProcessData));
   
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
      if (pProcessData->Console)
        {
          RtlEnterCriticalSection(&ProcessDataLock);
          RemoveEntryList(&pProcessData->ProcessEntry);
          RtlLeaveCriticalSection(&ProcessDataLock);
        }
      if (pProcessData->HandleTable)
        {
          for (c = 0; c < pProcessData->HandleTableSize; c++)
            {
              if (pProcessData->HandleTable[c])
                {
                  CsrReleaseObject(pProcessData, (HANDLE)((c + 1) << 2));
                }
            }
          RtlFreeHeap(CsrssApiHeap, 0, pProcessData->HandleTable);
        }
      if (pProcessData->Console)
        {
          CsrReleaseObjectByPointer((Object_t *) pProcessData->Console);
        }
      if (pProcessData->CsrSectionViewBase)
        {
          NtUnmapViewOfSection(NtCurrentProcess(), pProcessData->CsrSectionViewBase);
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


/**********************************************************************
 *	CSRSS API
 *********************************************************************/

CSR_API(CsrCreateProcess)
{
   PCSRSS_PROCESS_DATA NewProcessData;
   NTSTATUS Status;
   HANDLE Process;
   CSRSS_API_REQUEST ApiRequest;
   CSRSS_API_REPLY ApiReply;

   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - LPC_MESSAGE_BASE_SIZE;
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);

   NewProcessData = CsrCreateProcessData(Request->Data.CreateProcessRequest.NewProcessId);
   if (NewProcessData == NULL)
     {
	Reply->Status = STATUS_NO_MEMORY;
	return(STATUS_NO_MEMORY);
     }

   /* Set default shutdown parameters */
   NewProcessData->ShutdownLevel = 0x280;
   NewProcessData->ShutdownFlags = 0;

   if (Request->Data.CreateProcessRequest.Flags & DETACHED_PROCESS)
     {
	NewProcessData->Console = NULL;
     }
   else if (Request->Data.CreateProcessRequest.Flags & CREATE_NEW_CONSOLE)
     {
        ApiRequest.Type = CSRSS_ALLOC_CONSOLE;
        ApiRequest.Header.DataSize = sizeof(CSRSS_ALLOC_CONSOLE_REQUEST);
        ApiRequest.Header.MessageSize = LPC_MESSAGE_BASE_SIZE + sizeof(CSRSS_ALLOC_CONSOLE_REQUEST);
        ApiRequest.Data.AllocConsoleRequest.CtrlDispatcher = Request->Data.CreateProcessRequest.CtrlDispatcher;

        ApiReply.Header.DataSize = sizeof(CSRSS_ALLOC_CONSOLE_REPLY);
        ApiReply.Header.MessageSize = LPC_MESSAGE_BASE_SIZE + sizeof(CSRSS_ALLOC_CONSOLE_REPLY);

        CsrApiCallHandler(NewProcessData, &ApiRequest, &ApiReply);

        Reply->Status = ApiReply.Status;
        if (! NT_SUCCESS(Reply->Status))
          {
            return Reply->Status;
          }
        Reply->Data.CreateProcessReply.InputHandle = ApiReply.Data.AllocConsoleReply.InputHandle;
        Reply->Data.CreateProcessReply.OutputHandle = ApiReply.Data.AllocConsoleReply.OutputHandle;
     }
   else
     {
       CLIENT_ID ClientId;

       NewProcessData->Console = ProcessData->Console;
       InterlockedIncrement( &(ProcessData->Console->Header.ReferenceCount) );
       CsrInsertObject(NewProcessData,
		       &Reply->Data.CreateProcessReply.InputHandle,
		       (Object_t *)NewProcessData->Console);
       RtlEnterCriticalSection(&ProcessDataLock );
       CsrInsertObject( NewProcessData,
          &Reply->Data.CreateProcessReply.OutputHandle,
          &(NewProcessData->Console->ActiveBuffer->Header) );

       RtlLeaveCriticalSection(&ProcessDataLock);
       ClientId.UniqueProcess = (HANDLE)NewProcessData->ProcessId;
       Status = NtOpenProcess( &Process, PROCESS_DUP_HANDLE, 0, &ClientId );
       if( !NT_SUCCESS( Status ) )
	 {
	   DbgPrint( "CSR: NtOpenProcess() failed for handle duplication\n" );
	   InterlockedDecrement( &(NewProcessData->Console->Header.ReferenceCount) );
	   CsrFreeProcessData( NewProcessData->ProcessId );
	   Reply->Status = Status;
	   return Status;
	 }
       Status = NtDuplicateObject( NtCurrentProcess(), NewProcessData->Console->ActiveEvent, Process, &NewProcessData->ConsoleEvent, SYNCHRONIZE, FALSE, 0 );
       if( !NT_SUCCESS( Status ) )
	 {
	   DbgPrint( "CSR: NtDuplicateObject() failed: %x\n", Status );
	   NtClose( Process );
	   InterlockedDecrement( &(NewProcessData->Console->Header.ReferenceCount) );
	   CsrFreeProcessData( NewProcessData->ProcessId );
	   Reply->Status = Status;
	   return Status;
	 }
       NtClose( Process );
       NewProcessData->CtrlDispatcher = Request->Data.CreateProcessRequest.CtrlDispatcher;
       RtlEnterCriticalSection(&ProcessDataLock );
       InsertHeadList(&NewProcessData->Console->ProcessList, &NewProcessData->ProcessEntry);
       RtlLeaveCriticalSection(&ProcessDataLock);
     }

   Reply->Data.CreateProcessReply.Console = NewProcessData->Console;

   Reply->Status = STATUS_SUCCESS;
   return(STATUS_SUCCESS);
}

CSR_API(CsrTerminateProcess)
{
   NTSTATUS Status;

   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY) - LPC_MESSAGE_BASE_SIZE;
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY);

   if (ProcessData == NULL)
   {
      return(Reply->Status = STATUS_INVALID_PARAMETER);
   }

   Status = CsrFreeProcessData(ProcessData->ProcessId);

   Reply->Status = Status;
   return Status;
}

CSR_API(CsrConnectProcess)
{
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - LPC_MESSAGE_BASE_SIZE;

   Reply->Status = STATUS_SUCCESS;

   return(STATUS_SUCCESS);
}

CSR_API(CsrGetShutdownParameters)
{
  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - LPC_MESSAGE_BASE_SIZE;

  if (ProcessData == NULL)
  {
     return(Reply->Status = STATUS_INVALID_PARAMETER);
  }
  
  Reply->Data.GetShutdownParametersReply.Level = ProcessData->ShutdownLevel;
  Reply->Data.GetShutdownParametersReply.Flags = ProcessData->ShutdownFlags;

  Reply->Status = STATUS_SUCCESS;

  return(STATUS_SUCCESS);
}

CSR_API(CsrSetShutdownParameters)
{
  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - LPC_MESSAGE_BASE_SIZE;

  if (ProcessData == NULL)
  {
     return(Reply->Status = STATUS_INVALID_PARAMETER);
  }
  
  ProcessData->ShutdownLevel = Request->Data.SetShutdownParametersRequest.Level;
  ProcessData->ShutdownFlags = Request->Data.SetShutdownParametersRequest.Flags;

  Reply->Status = STATUS_SUCCESS;

  return(STATUS_SUCCESS);
}

CSR_API(CsrGetInputHandle)
{
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - LPC_MESSAGE_BASE_SIZE;

   if (ProcessData == NULL)
   {
      Reply->Data.GetInputHandleReply.InputHandle = INVALID_HANDLE_VALUE;
      Reply->Status = STATUS_INVALID_PARAMETER;
   }
   else if (ProcessData->Console)
   {
      Reply->Status = CsrInsertObject(ProcessData,
		                      &Reply->Data.GetInputHandleReply.InputHandle,
		                      (Object_t *)ProcessData->Console);
   }
   else
   {
      Reply->Data.GetInputHandleReply.InputHandle = INVALID_HANDLE_VALUE;
      Reply->Status = STATUS_SUCCESS;
   }

   return Reply->Status;
}

CSR_API(CsrGetOutputHandle)
{
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - LPC_MESSAGE_BASE_SIZE;

   if (ProcessData == NULL)
   {
      Reply->Data.GetOutputHandleReply.OutputHandle = INVALID_HANDLE_VALUE;
      Reply->Status = STATUS_INVALID_PARAMETER;
   }
   else if (ProcessData->Console)
   {
      RtlEnterCriticalSection(&ProcessDataLock);
      Reply->Status = CsrInsertObject(ProcessData,
                                      &Reply->Data.GetOutputHandleReply.OutputHandle,
                                      &(ProcessData->Console->ActiveBuffer->Header));
      RtlLeaveCriticalSection(&ProcessDataLock);
   }
   else
   {
      Reply->Data.GetOutputHandleReply.OutputHandle = INVALID_HANDLE_VALUE;
      Reply->Status = STATUS_SUCCESS;
   }

   return Reply->Status;
}

CSR_API(CsrCloseHandle)
{
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - LPC_MESSAGE_BASE_SIZE;

   if (ProcessData == NULL)
   {
      Reply->Status = STATUS_INVALID_PARAMETER;
   }
   else
   {
      Reply->Status = CsrReleaseObject(ProcessData, Request->Data.CloseHandleRequest.Handle);
   }
   return Reply->Status;
}

CSR_API(CsrVerifyHandle)
{
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - LPC_MESSAGE_BASE_SIZE;

   Reply->Status = CsrVerifyObject(ProcessData, Request->Data.VerifyHandleRequest.Handle);
   if (!NT_SUCCESS(Reply->Status))
   {
      DPRINT("CsrVerifyObject failed, status=%x\n", Reply->Status);
   }

   return Reply->Status;
}

CSR_API(CsrDuplicateHandle)
{
  Object_t *Object;

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - LPC_MESSAGE_BASE_SIZE;

  ProcessData = CsrGetProcessData(Request->Data.DuplicateHandleRequest.ProcessId);
  Reply->Status = CsrGetObject(ProcessData, Request->Data.DuplicateHandleRequest.Handle, &Object);
  if (! NT_SUCCESS(Reply->Status))
    {
      DPRINT("CsrGetObject failed, status=%x\n", Reply->Status);
    }
  else
    {
      Reply->Status = CsrInsertObject(ProcessData,
                                      &Reply->Data.DuplicateHandleReply.Handle,
                                      Object);
    }
  return Reply->Status;
}

/* EOF */
