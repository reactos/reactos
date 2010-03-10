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

#define CsrHeap RtlGetProcessHeap()

#define CsrHashThread(t) \
    (HandleToUlong(t)&(256 - 1))
    
LIST_ENTRY CsrThreadHashTable[256];
PCSRSS_PROCESS_DATA CsrRootProcess;

PCSR_THREAD
NTAPI
CsrAllocateThread(IN PCSRSS_PROCESS_DATA CsrProcess)
{
    PCSR_THREAD CsrThread;

    /* Allocate the structure */
    CsrThread = RtlAllocateHeap(CsrHeap, HEAP_ZERO_MEMORY, sizeof(CSR_THREAD));
    if (!CsrThread) return(NULL);

    /* Reference the Thread and Process */
    CsrThread->ReferenceCount++;
   // CsrProcess->ReferenceCount++;

    /* Set the Parent Process */
    CsrThread->Process = CsrProcess;

    /* Return Thread */
    return CsrThread;
}

PCSR_THREAD
NTAPI
CsrLocateThreadByClientId(OUT PCSRSS_PROCESS_DATA *Process OPTIONAL,
                          IN PCLIENT_ID ClientId)
{
    ULONG i;
    PLIST_ENTRY ListHead, NextEntry;
    PCSR_THREAD FoundThread;

    /* Hash the Thread */
    i = CsrHashThread(ClientId->UniqueThread);
    
    /* Set the list pointers */
    ListHead = &CsrThreadHashTable[i];
    NextEntry = ListHead->Flink;

    /* Star the loop */
    while (NextEntry != ListHead)
    {
        /* Get the thread */
        FoundThread = CONTAINING_RECORD(NextEntry, CSR_THREAD, HashLinks);

        /* Compare the CID */
        if (FoundThread->ClientId.UniqueThread == ClientId->UniqueThread)
        {
            /* Match found, return the process */
            *Process = FoundThread->Process;

            /* Return thread too */
//            DPRINT1("Found: %p %p\n", FoundThread, FoundThread->Process);
            return FoundThread;
        }

        /* Next */
        NextEntry = NextEntry->Flink;
    }

    /* Nothing found */
    return NULL;
}

PCSR_THREAD
NTAPI
CsrLocateThreadInProcess(IN PCSRSS_PROCESS_DATA CsrProcess OPTIONAL,
                         IN PCLIENT_ID Cid)
{
    PLIST_ENTRY ListHead, NextEntry;
    PCSR_THREAD FoundThread = NULL;

    /* Use the Root Process if none was specified */
    if (!CsrProcess) CsrProcess = CsrRootProcess;

    /* Save the List pointers */
//    DPRINT1("Searching in: %p %d\n", CsrProcess, CsrProcess->ThreadCount);
    ListHead = &CsrProcess->ThreadList;
    NextEntry = ListHead->Flink;

    /* Start the Loop */
    while (NextEntry != ListHead)
    {
        /* Get Thread Entry */
        FoundThread = CONTAINING_RECORD(NextEntry, CSR_THREAD, Link);

        /* Check for TID Match */
        if (FoundThread->ClientId.UniqueThread == Cid->UniqueThread) break;

        /* Next entry */
        NextEntry = NextEntry->Flink;
    }

    /* Return what we found */
//    DPRINT1("Found: %p\n", FoundThread);
    return FoundThread;
}

VOID
NTAPI
CsrInsertThread(IN PCSRSS_PROCESS_DATA Process,
                IN PCSR_THREAD Thread)
{
    ULONG i;

    /* Insert it into the Regular List */
    InsertTailList(&Process->ThreadList, &Thread->Link);

    /* Increase Thread Count */
    Process->ThreadCount++;

    /* Hash the Thread */
    i = CsrHashThread(Thread->ClientId.UniqueThread);
//    DPRINT1("TID %lx HASH: %lx\n", Thread->ClientId.UniqueThread, i);

    /* Insert it there too */
    InsertHeadList(&CsrThreadHashTable[i], &Thread->HashLinks);
}


#define CsrAcquireProcessLock() LOCK
#define CsrReleaseProcessLock() UNLOCK

NTSTATUS
NTAPI
CsrCreateThreadData(IN PCSRSS_PROCESS_DATA CsrProcess,
                    IN HANDLE hThread,
                    IN PCLIENT_ID ClientId)
{
    NTSTATUS Status;
    PCSR_THREAD CsrThread;
    //PCSRSS_PROCESS_DATA CurrentProcess;
    PCSR_THREAD CurrentThread = NtCurrentTeb()->CsrClientThread;
    CLIENT_ID CurrentCid;
    KERNEL_USER_TIMES KernelTimes;

//    DPRINT1("CSRSRV: %s called\n", __FUNCTION__);

    /* Get the current thread and CID */
    CurrentCid = CurrentThread->ClientId;
//    DPRINT1("CALLER PID/TID: %lx/%lx\n", CurrentCid.UniqueProcess, CurrentCid.UniqueThread);

    /* Acquire the Process Lock */
    CsrAcquireProcessLock();
#if 0
    /* Get the current Process and make sure the Thread is valid with this CID */
    CurrentThread = CsrLocateThreadByClientId(&CurrentProcess,
                                              &CurrentCid);

    /* Something is wrong if we get an empty thread back */
    if (!CurrentThread)
    {
        DPRINT1("CSRSRV:%s: invalid thread!\n", __FUNCTION__);
        CsrReleaseProcessLock();
        return STATUS_THREAD_IS_TERMINATING;
    }
#endif
    /* Get the Thread Create Time */
    Status = NtQueryInformationThread(hThread,
                                      ThreadTimes,
                                      (PVOID)&KernelTimes,
                                      sizeof(KernelTimes),
                                      NULL);

    /* Allocate a CSR Thread Structure */
    if (!(CsrThread = CsrAllocateThread(CsrProcess)))
    {
        DPRINT1("CSRSRV:%s: out of memory!\n", __FUNCTION__);
        CsrReleaseProcessLock();
        return STATUS_NO_MEMORY;
    }

    /* Save the data we have */
    CsrThread->CreateTime = KernelTimes.CreateTime;
    CsrThread->ClientId = *ClientId;
    CsrThread->ThreadHandle = hThread;
    CsrThread->Flags = 0;

    /* Insert the Thread into the Process */
    CsrInsertThread(CsrProcess, CsrThread);

    /* Release the lock and return */
    CsrReleaseProcessLock();
    return STATUS_SUCCESS;
}

PCSR_THREAD
NTAPI
CsrAddStaticServerThread(IN HANDLE hThread,
                         IN PCLIENT_ID ClientId,
                         IN ULONG ThreadFlags)
{
    PCSR_THREAD CsrThread;

    /* Get the Lock */
    CsrAcquireProcessLock();

    /* Allocate the Server Thread */
    if ((CsrThread = CsrAllocateThread(CsrRootProcess)))
    {
        /* Setup the Object */
//        DPRINT1("New CSR thread created: %lx PID/TID: %lx/%lx\n", CsrThread, ClientId->UniqueProcess, ClientId->UniqueThread);
        CsrThread->ThreadHandle = hThread;
        CsrThread->ClientId = *ClientId;
        CsrThread->Flags = ThreadFlags;

        /* Insert it into the Thread List */
        InsertTailList(&CsrRootProcess->ThreadList, &CsrThread->Link);

        /* Increment the thread count */
        CsrRootProcess->ThreadCount++;
    }

    /* Release the Process Lock and return */
    CsrReleaseProcessLock();
    return CsrThread;
}

VOID WINAPI CsrInitProcessData(VOID)
{
    ULONG i;
   RtlZeroMemory (ProcessData, sizeof ProcessData);
   NrProcess = sizeof ProcessData / sizeof ProcessData[0];
   RtlInitializeCriticalSection( &ProcessDataLock );
   
   CsrRootProcess = CsrCreateProcessData(NtCurrentTeb()->ClientId.UniqueProcess);
   
   /* Initialize the Thread Hash List */
   for (i = 0; i < 256; i++) InitializeListHead(&CsrThreadHashTable[i]);
}

PCSRSS_PROCESS_DATA WINAPI CsrGetProcessData(HANDLE ProcessId)
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

PCSRSS_PROCESS_DATA WINAPI CsrCreateProcessData(HANDLE ProcessId)
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
                                PROCESS_ALL_ACCESS,
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
   
   pProcessData->ThreadCount = 0;
   InitializeListHead(&pProcessData->ThreadList);
   return pProcessData;
}

NTSTATUS WINAPI CsrFreeProcessData(HANDLE Pid)
{
  ULONG hash;
  UINT c;
  PCSRSS_PROCESS_DATA pProcessData, *pPrevLink;
  HANDLE Process;

  hash = ((ULONG_PTR)Pid >> 2) % (sizeof(ProcessData) / sizeof(*ProcessData));
  pPrevLink = &ProcessData[hash];

  LOCK;

  while ((pProcessData = *pPrevLink) && pProcessData->ProcessId != Pid)
    {
      pPrevLink = &pProcessData->next;
    }

  if (pProcessData)
    {
      DPRINT("CsrFreeProcessData pid: %d\n", Pid);
      Process = pProcessData->Process;
      if (pProcessData->HandleTable)
        {
          for (c = 0; c < pProcessData->HandleTableSize; c++)
            {
              if (pProcessData->HandleTable[c].Object)
                {
                  CsrReleaseObjectByPointer(pProcessData->HandleTable[c].Object);
                }
            }
          RtlFreeHeap(CsrssApiHeap, 0, pProcessData->HandleTable);
        }
      RtlDeleteCriticalSection(&pProcessData->HandleTableLock);
      if (pProcessData->Console)
        {
          RemoveEntryList(&pProcessData->ProcessEntry);
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
      *pPrevLink = pProcessData->next;

      RtlFreeHeap(CsrssApiHeap, 0, pProcessData);
      UNLOCK;
      if (Process)
        {
          NtClose(Process);
        }
      return STATUS_SUCCESS;
   }

   UNLOCK;
   return STATUS_INVALID_PARAMETER;
}

VOID
NTAPI
CsrSetToNormalPriority(VOID)
{
    KPRIORITY BasePriority = (8 + 1) + 4;

    /* Set the Priority */
    NtSetInformationProcess(NtCurrentProcess(),
                            ProcessBasePriority,
                            &BasePriority,
                            sizeof(KPRIORITY));
}

VOID
NTAPI
CsrSetToShutdownPriority(VOID)
{
    KPRIORITY SetBasePriority = (8 + 1) + 6;
    BOOLEAN Old;

    /* Get the shutdown privilege */
    if (NT_SUCCESS(RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE,
                                      TRUE,
                                      FALSE,
                                      &Old)))
    {
        /* Set the Priority */
        NtSetInformationProcess(NtCurrentProcess(),
                                ProcessBasePriority,
                                &SetBasePriority,
                                sizeof(KPRIORITY));
    }
}

NTSTATUS
NTAPI
CsrGetProcessLuid(HANDLE hProcess OPTIONAL,
                  PLUID Luid)
{
    HANDLE hToken = NULL;
    NTSTATUS Status;
    ULONG Length;
    PTOKEN_STATISTICS TokenStats;

    /* Check if we have a handle to a CSR Process */
    if (!hProcess)
    {
        /* We don't, so try opening the Thread's Token */
        Status = NtOpenThreadToken(NtCurrentThread(),
                                   TOKEN_QUERY,
                                   FALSE,
                                   &hToken);

        /* Check for success */
        if (!NT_SUCCESS(Status))
        {
            /* If we got some other failure, then return and quit */
            if (Status != STATUS_NO_TOKEN) return Status;

            /* We don't have a Thread Token, use a Process Token */
            hProcess = NtCurrentProcess();
            hToken = NULL;
        }
    }

    /* Check if we have a token by now */
    if (!hToken)
    {
        /* No token yet, so open the Process Token */
        Status = NtOpenProcessToken(hProcess,
                                    TOKEN_QUERY,
                                    &hToken);
        if (!NT_SUCCESS(Status))
        {
            /* Still no token, return the error */
            return Status;
        }
    }

    /* Now get the size we'll need for the Token Information */
    Status = NtQueryInformationToken(hToken,
                                     TokenStatistics,
                                     NULL,
                                     0,
                                     &Length);

    /* Allocate memory for the Token Info */
    if (!(TokenStats = RtlAllocateHeap(CsrHeap, 0, Length)))
    {
        /* Fail and close the token */
        NtClose(hToken);
        return STATUS_NO_MEMORY;
    }

    /* Now query the information */
    Status = NtQueryInformationToken(hToken,
                                     TokenStatistics,
                                     TokenStats,
                                     Length,
                                     &Length);

    /* Close the handle */
    NtClose(hToken);

    /* Check for success */
    if (NT_SUCCESS(Status))
    {
        /* Return the LUID */
        *Luid = TokenStats->AuthenticationId;
    }

    /* Free the query information */
    RtlFreeHeap(CsrHeap, 0, TokenStats);

    /* Return the Status */
    return Status;
}

SECURITY_QUALITY_OF_SERVICE CsrSecurityQos =
{
    sizeof(SECURITY_QUALITY_OF_SERVICE),
    SecurityImpersonation,
    SECURITY_STATIC_TRACKING,
    FALSE
};

BOOLEAN
NTAPI
CsrImpersonateClient(IN PCSR_THREAD CsrThread)
{
    NTSTATUS Status;
    PCSR_THREAD CurrentThread = NtCurrentTeb()->CsrClientThread;

    /* Use the current thread if none given */
    if (!CsrThread) CsrThread = CurrentThread;

    /* Still no thread, something is wrong */
    if (!CsrThread)
    {
        /* Failure */
        return FALSE;
    }

    /* Make the call */
    Status = NtImpersonateThread(NtCurrentThread(),
                                 CsrThread->ThreadHandle,
                                 &CsrSecurityQos);

    if (!NT_SUCCESS(Status))
    {
        /* Failure */
        return FALSE;
    }

    /* Increase the impersonation count for the current thread */
    if (CurrentThread) ++CurrentThread->ImpersonationCount;

    /* Return Success */
    return TRUE;
}

BOOLEAN
NTAPI
CsrRevertToSelf(VOID)
{
    NTSTATUS Status;
    PCSR_THREAD CurrentThread = NtCurrentTeb()->CsrClientThread;
    HANDLE ImpersonationToken = NULL;

    /* Check if we have a Current Thread */
    if (CurrentThread)
    {
        /* Make sure impersonation is on */
        if (!CurrentThread->ImpersonationCount)
        {
            return FALSE;
        }
        else if (--CurrentThread->ImpersonationCount > 0)
        {
            /* Success; impersonation count decreased but still not zero */
            return TRUE;
        }
    }

    /* Impersonation has been totally removed, revert to ourselves */
    Status = NtSetInformationThread(NtCurrentThread(),
                                    ThreadImpersonationToken,
                                    &ImpersonationToken,
                                    sizeof(HANDLE));

    /* Return TRUE or FALSE */
    return NT_SUCCESS(Status);
}

PCSRSS_PROCESS_DATA
NTAPI
FindProcessForShutdown(IN PLUID CallerLuid)
{
    ULONG Hash;
    PCSRSS_PROCESS_DATA CsrProcess, ReturnCsrProcess = NULL;
    NTSTATUS Status;
    ULONG Level = 0;
    LUID ProcessLuid;
    LUID SystemLuid = SYSTEM_LUID;
    BOOLEAN IsSystemLuid = FALSE, IsOurLuid = FALSE;
    
    for (Hash = 0; Hash < (sizeof(ProcessData) / sizeof(*ProcessData)); Hash++)
    {
        /* Get this process hash bucket */
        CsrProcess = ProcessData[Hash];
        while (CsrProcess)
        {
            /* Skip this process if it's already been processed*/
            if (CsrProcess->Flags & CsrProcessSkipShutdown) goto Next;
        
            /* Get the LUID of this Process */
            Status = CsrGetProcessLuid(CsrProcess->Process, &ProcessLuid);

            /* Check if we didn't get access to the LUID */
            if (Status == STATUS_ACCESS_DENIED)
            {
                /* FIXME:Check if we have any threads */
            }
            
            if (!NT_SUCCESS(Status))
            {
                /* We didn't have access, so skip it */
                CsrProcess->Flags |= CsrProcessSkipShutdown;
                goto Next;
            }
            
            /* Check if this is the System LUID */
            if ((IsSystemLuid = RtlEqualLuid(&ProcessLuid, &SystemLuid)))
            {
                /* Mark this process */
                CsrProcess->ShutdownFlags |= CsrShutdownSystem;
            }
            else if (!(IsOurLuid = RtlEqualLuid(&ProcessLuid, CallerLuid)))
            {
                /* Our LUID doesn't match with the caller's */
                CsrProcess->ShutdownFlags |= CsrShutdownOther;
            }
            
            /* Check if we're past the previous level */
            if (CsrProcess->ShutdownLevel > Level)
            {
                /* Update the level */
                Level = CsrProcess->ShutdownLevel;

                /* Set the final process */
                ReturnCsrProcess = CsrProcess;
            }
Next:
            /* Next process */
            CsrProcess = CsrProcess->next;
        }
    }
    
    /* Check if we found a process */
    if (ReturnCsrProcess)
    {
        /* Skip this one next time */
        ReturnCsrProcess->Flags |= CsrProcessSkipShutdown;
    }
    
    return ReturnCsrProcess;
}

/* This is really "CsrShutdownProcess", mostly */
NTSTATUS
WINAPI
CsrEnumProcesses(IN CSRSS_ENUM_PROCESS_PROC EnumProc,
                 IN PVOID Context)
{
    PVOID* RealContext = (PVOID*)Context;
    PLUID CallerLuid = RealContext[0];
    PCSRSS_PROCESS_DATA CsrProcess = NULL;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    BOOLEAN FirstTry;
    ULONG Result = 0;
    ULONG Hash;

    /* Acquire process lock */
    CsrAcquireProcessLock();

    /* Start the loop */
    for (Hash = 0; Hash < (sizeof(ProcessData) / sizeof(*ProcessData)); Hash++)
    {
        /* Get the Process */
        CsrProcess = ProcessData[Hash];
        while (CsrProcess)
        {
           /* Remove the skip flag, set shutdown flags to 0*/
            CsrProcess->Flags &= ~CsrProcessSkipShutdown;
            CsrProcess->ShutdownFlags = 0;

            /* Move to the next */
            CsrProcess = CsrProcess->next;
        }
    }

    /* Set shudown Priority */
    CsrSetToShutdownPriority();

    /* Loop all processes */
    DPRINT1("Enumerating for LUID: %lx %lx\n", CallerLuid->HighPart, CallerLuid->LowPart);
    
    /* Start looping */
    while (TRUE)
    {
        /* Find the next process to shutdown */
        FirstTry = TRUE;
        if (!(CsrProcess = FindProcessForShutdown(CallerLuid)))
        {
            /* Done, quit */
            CsrReleaseProcessLock();
            Status = STATUS_SUCCESS;
            goto Quickie;
        }

LoopAgain:
        /* Release the lock, make the callback, and acquire it back */
        DPRINT1("Found process: %lx\n", CsrProcess->ProcessId);
        CsrReleaseProcessLock();
        Result = (ULONG)EnumProc(CsrProcess, (PVOID)((ULONG_PTR)Context | FirstTry));
        CsrAcquireProcessLock();

        /* Check the result */
        DPRINT1("Result: %d\n", Result);
        if (Result == CsrShutdownCsrProcess)
        {
            /* The callback unlocked the process */
            break;
        }
        else if (Result == CsrShutdownNonCsrProcess)
        {
            /* A non-CSR process, the callback didn't touch it */
            //continue;
        }
        else if (Result == CsrShutdownCancelled)
        {
            /* Shutdown was cancelled, unlock and exit */
            CsrReleaseProcessLock();
            Status = STATUS_CANCELLED;
            goto Quickie;
        }

        /* No matches during the first try, so loop again */
        if (FirstTry && Result == CsrShutdownNonCsrProcess)
        {
            FirstTry = FALSE;
            goto LoopAgain;
        }
    }

Quickie:
    /* Return to normal priority */
    CsrSetToNormalPriority();
    return Status;
}

NTSTATUS
NTAPI
CsrLockProcessByClientId(IN HANDLE Pid,
                         OUT PCSRSS_PROCESS_DATA *CsrProcess OPTIONAL)
{
    ULONG Hash;
    PCSRSS_PROCESS_DATA CurrentProcess = NULL;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    /* Acquire the lock */
    CsrAcquireProcessLock();

    /* Start the loop */
    for (Hash = 0; Hash < (sizeof(ProcessData) / sizeof(*ProcessData)); Hash++)
    {
        /* Get the Process */
        CurrentProcess = ProcessData[Hash];
        while (CurrentProcess)
        {
            /* Check for PID match */
            if (CurrentProcess->ProcessId == Pid)
            {
                /* Get out of here with success */
//                DPRINT1("Found %p for PID %lx\n", CurrentProcess, Pid);
                Status = STATUS_SUCCESS;
                goto Found;
            }
            
            /* Move to the next */
            CurrentProcess = CurrentProcess->next;
        }
    }
    
    /* Nothing found, release the lock */
Found:
    if (!CurrentProcess) CsrReleaseProcessLock();

    /* Return the status and process */
    if (CsrProcess) *CsrProcess = CurrentProcess;
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

   if (Request->Data.CreateProcessRequest.Flags & CREATE_NEW_PROCESS_GROUP)
     {
       NewProcessData->ProcessGroup = (DWORD)(ULONG_PTR)NewProcessData->ProcessId;
     }
   else
     {
       NewProcessData->ProcessGroup = ProcessData->ProcessGroup;
     }

   return(STATUS_SUCCESS);
}

CSR_API(CsrCreateThread)
{
    PCSR_THREAD CurrentThread;
    HANDLE ThreadHandle;
    NTSTATUS Status;
    PCSRSS_PROCESS_DATA CsrProcess;
    
    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    CurrentThread = NtCurrentTeb()->CsrClientThread;
    CsrProcess = CurrentThread->Process;
//    DPRINT1("Current thread: %p %p\n", CurrentThread, CsrProcess);
//    DPRINT1("Request CID: %lx %lx %lx\n", 
//            CsrProcess->ProcessId, 
//            NtCurrentTeb()->ClientId.UniqueProcess,
 //           Request->Data.CreateThreadRequest.ClientId.UniqueProcess);
    
    if (CsrProcess->ProcessId != Request->Data.CreateThreadRequest.ClientId.UniqueProcess)
    {
        if (Request->Data.CreateThreadRequest.ClientId.UniqueProcess == NtCurrentTeb()->ClientId.UniqueProcess)
        {
            return STATUS_SUCCESS;
        }
        
        Status = CsrLockProcessByClientId(Request->Data.CreateThreadRequest.ClientId.UniqueProcess,
                                          &CsrProcess);
  //      DPRINT1("Found matching process: %p\n", CsrProcess);
        if (!NT_SUCCESS(Status)) return Status;
    }
    
//    DPRINT1("PIDs: %lx %lx\n", CurrentThread->Process->ProcessId, CsrProcess->ProcessId);
//    DPRINT1("Thread handle is: %lx Process Handle is: %lx %lx\n",
       //     Request->Data.CreateThreadRequest.ThreadHandle,
     //       CurrentThread->Process->Process,
   //         CsrProcess->Process);
    Status = NtDuplicateObject(CsrProcess->Process,
                               Request->Data.CreateThreadRequest.ThreadHandle,
                               NtCurrentProcess(),
                               &ThreadHandle,
                               0,
                               0,
                               DUPLICATE_SAME_ACCESS);
    //DPRINT1("Duplicate status: %lx\n", Status);
    if (!NT_SUCCESS(Status))
    {
        Status = NtDuplicateObject(CurrentThread->Process->Process,
                                   Request->Data.CreateThreadRequest.ThreadHandle,
                                   NtCurrentProcess(),
                                   &ThreadHandle,
                                   0,
                                   0,
                                   DUPLICATE_SAME_ACCESS);
       // DPRINT1("Duplicate status: %lx\n", Status);
    }

    Status = STATUS_SUCCESS; // hack
    if (NT_SUCCESS(Status))
    {
        Status = CsrCreateThreadData(CsrProcess,
                                     ThreadHandle,
                                     &Request->Data.CreateThreadRequest.ClientId);
       // DPRINT1("Create status: %lx\n", Status);
    }

    if (CsrProcess != CurrentThread->Process) CsrReleaseProcessLock();
    
    return Status;
}

CSR_API(CsrTerminateProcess)
{
   Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
   Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

   ProcessData->Terminated = TRUE;
   return STATUS_SUCCESS;
}

CSR_API(CsrConnectProcess)
{
   Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
   Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

   return(STATUS_SUCCESS);
}

CSR_API(CsrGetShutdownParameters)
{
  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  Request->Data.GetShutdownParametersRequest.Level = ProcessData->ShutdownLevel;
  Request->Data.GetShutdownParametersRequest.Flags = ProcessData->ShutdownFlags;

  return(STATUS_SUCCESS);
}

CSR_API(CsrSetShutdownParameters)
{
  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  ProcessData->ShutdownLevel = Request->Data.SetShutdownParametersRequest.Level;
  ProcessData->ShutdownFlags = Request->Data.SetShutdownParametersRequest.Flags;

  return(STATUS_SUCCESS);
}

CSR_API(CsrGetInputHandle)
{
   Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
   Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

   if (ProcessData->Console)
   {
      Request->Status = CsrInsertObject(ProcessData,
		                      &Request->Data.GetInputHandleRequest.InputHandle,
		                      (Object_t *)ProcessData->Console,
		                      Request->Data.GetInputHandleRequest.Access,
		                      Request->Data.GetInputHandleRequest.Inheritable);
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

   if (ProcessData->Console)
   {
      RtlEnterCriticalSection(&ProcessDataLock);
      Request->Status = CsrInsertObject(ProcessData,
                                      &Request->Data.GetOutputHandleRequest.OutputHandle,
                                      &ProcessData->Console->ActiveBuffer->Header,
                                      Request->Data.GetOutputHandleRequest.Access,
                                      Request->Data.GetOutputHandleRequest.Inheritable);
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

   return CsrReleaseObject(ProcessData, Request->Data.CloseHandleRequest.Handle);
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
    ULONG_PTR Index;
    PCSRSS_HANDLE Entry;
    DWORD DesiredAccess;

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    Index = (ULONG_PTR)Request->Data.DuplicateHandleRequest.Handle >> 2;
    RtlEnterCriticalSection(&ProcessData->HandleTableLock);
    if (Index >= ProcessData->HandleTableSize
        || (Entry = &ProcessData->HandleTable[Index])->Object == NULL)
    {
        DPRINT1("Couldn't dup invalid handle %p\n", Request->Data.DuplicateHandleRequest.Handle);
        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
        return STATUS_INVALID_HANDLE;
    }

    if (Request->Data.DuplicateHandleRequest.Options & DUPLICATE_SAME_ACCESS)
    {
        DesiredAccess = Entry->Access;
    }
    else
    {
        DesiredAccess = Request->Data.DuplicateHandleRequest.Access;
        /* Make sure the source handle has all the desired flags */
        if (~Entry->Access & DesiredAccess)
        {
            DPRINT1("Handle %p only has access %X; requested %X\n",
                Request->Data.DuplicateHandleRequest.Handle, Entry->Access, DesiredAccess);
            RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
            return STATUS_INVALID_PARAMETER;
        }
    }
    
    Request->Status = CsrInsertObject(ProcessData,
                                      &Request->Data.DuplicateHandleRequest.Handle,
                                      Entry->Object,
                                      DesiredAccess,
                                      Request->Data.DuplicateHandleRequest.Inheritable);
    if (NT_SUCCESS(Request->Status)
        && Request->Data.DuplicateHandleRequest.Options & DUPLICATE_CLOSE_SOURCE)
    {
        /* Close the original handle. This cannot drop the count to 0, since a new handle now exists */
        _InterlockedDecrement(&Entry->Object->ReferenceCount);
        Entry->Object = NULL;
    }

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
    return Request->Status;
}

CSR_API(CsrGetInputWaitHandle)
{
  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  Request->Data.GetConsoleInputWaitHandle.InputWaitHandle = ProcessData->ConsoleEvent;
  return STATUS_SUCCESS;
}

/* EOF */
