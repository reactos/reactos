/* $Id: processes.c,v 1.1 2004/11/03 22:43:00 weiden Exp $
*/
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * LICENSE:     See LGPL.txt in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        reactos/lib/epsapi/enum/processes.c
 * PURPOSE:     Enumerate processes and threads
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              10/06/2002: Created
 *              29/08/2002: Generalized the interface to improve reusability,
 *                          more efficient use of memory operations
 *              12/02/2003: malloc and free renamed to PsaiMalloc and PsaiFree,
 *                          for better reusability. PsaEnumerateProcesses now
 *                          expanded into:
 *                           - PsaCaptureProcessesAndThreads
 *                           - PsaFreeCapture
 *                           - PsaWalkProcessesAndThreads
 *                           - PsaWalkProcesses
 *                           - PsaWalkThreads
 *                           - PsaWalkFirstProcess
 *                           - PsaWalkNextProcess
 *                           - PsaWalkFirstThread
 *                           - PsaWalkNextThread 
 *                           - PsaEnumerateProcessesAndThreads
 *                           - PsaEnumerateProcesses
 *                           - PsaEnumerateThreads
 *              12/04/2003: internal PSAPI renamed EPSAPI (Extended PSAPI) and
 *                          isolated in its own library to clear the confusion
 *                          and improve reusability
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

NTSTATUS NTAPI
PsaCaptureProcessesAndThreads(OUT PSYSTEM_PROCESSES *ProcessesAndThreads)
{
  PSYSTEM_PROCESSES pInfoBuffer = NULL;
  SIZE_T nSize = 0x8000;
  NTSTATUS Status;

  if(ProcessesAndThreads == NULL)
  {
    return STATUS_INVALID_PARAMETER_1;
  }

  /* FIXME: if the system has loaded several processes and threads, the buffer
            could get really big. But if there's several processes and threads, the
            system is already under stress, and a huge buffer could only make things
            worse. The function should be profiled to see what's the average minimum
            buffer size, to succeed on the first shot */
  do
  {
    PVOID pTmp;

    /* free the buffer, and reallocate it to the new size. RATIONALE: since we
       ignore the buffer's contents at this point, there's no point in a realloc()
       that could end up copying a large chunk of data we'd discard anyway */
    PsaiFree(pInfoBuffer);
    pTmp = PsaiMalloc(nSize);
  
    if(pTmp == NULL)
    {
      DPRINT(FAILED_WITH_STATUS, "PsaiMalloc", STATUS_NO_MEMORY);
      Status = STATUS_NO_MEMORY;
      break;
    }
  
    pInfoBuffer = pTmp;
  
    /* query the information */
    Status = NtQuerySystemInformation(SystemProcessesAndThreadsInformation,
                                      pInfoBuffer,
                                      nSize,
                                      NULL);

    /* double the buffer size */
    nSize *= 2;
  } while(Status == STATUS_INFO_LENGTH_MISMATCH);
 
  if(!NT_SUCCESS(Status))
  {
    DPRINT(FAILED_WITH_STATUS, "NtQuerySystemInformation", Status);
    return Status;
  }

  *ProcessesAndThreads = pInfoBuffer;
  return STATUS_SUCCESS;
}

NTSTATUS NTAPI
PsaWalkProcessesAndThreads(IN PSYSTEM_PROCESSES ProcessesAndThreads,
                           IN PPROC_ENUM_ROUTINE ProcessCallback,
                           IN OUT PVOID ProcessCallbackContext,
                           IN PTHREAD_ENUM_ROUTINE ThreadCallback,
                           IN OUT PVOID ThreadCallbackContext)
{
  NTSTATUS Status;

  if(ProcessCallback == NULL && ThreadCallback == NULL)
  {
    return STATUS_INVALID_PARAMETER;
  }
  
  Status = STATUS_SUCCESS;

  ProcessesAndThreads = PsaWalkFirstProcess(ProcessesAndThreads);

  /* scan the process list */
  do
  {
    if(ProcessCallback)
    {
      Status = ProcessCallback(ProcessesAndThreads, ProcessCallbackContext);
 
      if(!NT_SUCCESS(Status))
      {
        break;
      }
    }

    /* if the caller provided a thread callback */
    if(ThreadCallback)
    {
      ULONG i;
      PSYSTEM_THREADS pCurThread;

      /* scan the current process's thread list */
      for(i = 0, pCurThread = PsaWalkFirstThread(ProcessesAndThreads);
          i < ProcessesAndThreads->ThreadCount;
          i++, pCurThread = PsaWalkNextThread(pCurThread))
      {
        Status = ThreadCallback(pCurThread, ThreadCallbackContext);
        
        if(!NT_SUCCESS(Status))
        {
          goto Bail;
        }
      }
    }

    /* move to the next process */
    ProcessesAndThreads = PsaWalkNextProcess(ProcessesAndThreads);
  } while(ProcessesAndThreads);

Bail:
  return Status;
}

NTSTATUS NTAPI
PsaEnumerateProcessesAndThreads(IN PPROC_ENUM_ROUTINE ProcessCallback,
                                IN OUT PVOID ProcessCallbackContext,
                                IN PTHREAD_ENUM_ROUTINE ThreadCallback,
                                IN OUT PVOID ThreadCallbackContext)
{
  PSYSTEM_PROCESSES pInfoBuffer;
  NTSTATUS Status;

  if(ProcessCallback == NULL && ThreadCallback == NULL)
  {
    return STATUS_INVALID_PARAMETER;
  }

  /* get the processes and threads list */
  Status = PsaCaptureProcessesAndThreads(&pInfoBuffer);

  if(!NT_SUCCESS(Status))
  {
    goto Bail;
  }

  /* walk the processes and threads list */
  Status = PsaWalkProcessesAndThreads(pInfoBuffer,
                                      ProcessCallback,
                                      ProcessCallbackContext,
                                      ThreadCallback,
                                      ThreadCallbackContext);

Bail:
  PsaFreeCapture(pInfoBuffer);
 
  return Status;
}

VOID NTAPI
PsaFreeCapture(IN PVOID Capture)
{
  PsaiFree(Capture);
}

NTSTATUS NTAPI
PsaWalkProcesses(IN PSYSTEM_PROCESSES ProcessesAndThreads,
                 IN PPROC_ENUM_ROUTINE Callback,
                 IN OUT PVOID CallbackContext)
{
  return PsaWalkProcessesAndThreads(ProcessesAndThreads,
                                    Callback,
                                    CallbackContext,
                                    NULL,
                                    NULL);
}

NTSTATUS NTAPI
PsaWalkThreads(IN PSYSTEM_PROCESSES ProcessesAndThreads,
               IN PTHREAD_ENUM_ROUTINE Callback,
               IN OUT PVOID CallbackContext)
{
  return PsaWalkProcessesAndThreads(ProcessesAndThreads,
                                    NULL,
                                    NULL,
                                   Callback,
                                   CallbackContext);
}

NTSTATUS NTAPI
PsaEnumerateProcesses(IN PPROC_ENUM_ROUTINE Callback,
                      IN OUT PVOID CallbackContext)
{
  return PsaEnumerateProcessesAndThreads(Callback,
                                         CallbackContext,
                                         NULL,
                                         NULL);
}

NTSTATUS NTAPI
PsaEnumerateThreads(IN PTHREAD_ENUM_ROUTINE Callback,
                    IN OUT PVOID CallbackContext)
{
  return PsaEnumerateProcessesAndThreads(NULL,
                                         NULL,
                                         Callback,
                                         CallbackContext);
}

PSYSTEM_PROCESSES FASTCALL
PsaWalkFirstProcess(IN PSYSTEM_PROCESSES ProcessesAndThreads)
{
  return ProcessesAndThreads;
}

PSYSTEM_PROCESSES FASTCALL
PsaWalkNextProcess(IN PSYSTEM_PROCESSES CurrentProcess)
{
  if(CurrentProcess->NextEntryDelta == 0)
  {
    return NULL;
  }
  else
  {
    return (PSYSTEM_PROCESSES)((ULONG_PTR)CurrentProcess + CurrentProcess->NextEntryDelta);
  }
}

PSYSTEM_THREADS FASTCALL
PsaWalkFirstThread(IN PSYSTEM_PROCESSES CurrentProcess)
{
  static SIZE_T nOffsetOfThreads = 0;

  /* get the offset of the Threads field (dependant on the kernel version) */
  if(!nOffsetOfThreads)
  {
    /*
     FIXME: we should probably use the build number, instead, but it isn't
     available as reliably as the major and minor version numbers
    */
    switch(SharedUserData->NtMajorVersion)
    {
      /* NT 3 and 4 */
      case 3:
      case 4:
      {
        nOffsetOfThreads = offsetof(SYSTEM_PROCESSES_NT4, Threads);
        break;
      }

      /* NT 5 and later */
      case 5:
      default:
      {
        nOffsetOfThreads = offsetof(SYSTEM_PROCESSES_NT5, Threads);
        break;
      }
    }
  }

  return (PSYSTEM_THREADS)((ULONG_PTR)CurrentProcess + nOffsetOfThreads);
}

PSYSTEM_THREADS FASTCALL
PsaWalkNextThread(IN PSYSTEM_THREADS CurrentThread)
{
  return (PSYSTEM_THREADS)((ULONG_PTR)CurrentThread +
                           (offsetof(SYSTEM_PROCESSES, Threads[1]) -
                            offsetof(SYSTEM_PROCESSES, Threads[0])));
}

/* EOF */
