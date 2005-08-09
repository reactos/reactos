/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/query.c
 * PURPOSE:         Set/Query Process/Thread Information APIs
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net) - Created File
 *                  David Welch
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

static const INFORMATION_CLASS_INFO PsProcessInfoClass[] =
{
  ICI_SQ_SAME( sizeof(PROCESS_BASIC_INFORMATION),     sizeof(ULONG), ICIF_QUERY ),                     /* ProcessBasicInformation */
  ICI_SQ_SAME( sizeof(QUOTA_LIMITS),                  sizeof(ULONG), ICIF_QUERY | ICIF_SET ),          /* ProcessQuotaLimits */
  ICI_SQ_SAME( sizeof(IO_COUNTERS),                   sizeof(ULONG), ICIF_QUERY ),                     /* ProcessIoCounters */
  ICI_SQ_SAME( sizeof(VM_COUNTERS),                   sizeof(ULONG), ICIF_QUERY ),                     /* ProcessVmCounters */
  ICI_SQ_SAME( sizeof(KERNEL_USER_TIMES),             sizeof(ULONG), ICIF_QUERY ),                     /* ProcessTimes */
  ICI_SQ_SAME( sizeof(KPRIORITY),                     sizeof(ULONG), ICIF_SET ),                       /* ProcessBasePriority */
  ICI_SQ_SAME( sizeof(ULONG),                         sizeof(ULONG), ICIF_SET ),                       /* ProcessRaisePriority */
  ICI_SQ_SAME( sizeof(HANDLE),                        sizeof(ULONG), ICIF_QUERY ),                     /* ProcessDebugPort */
  ICI_SQ_SAME( sizeof(HANDLE),                        sizeof(ULONG), ICIF_SET ),                       /* ProcessExceptionPort */
  ICI_SQ_SAME( sizeof(PROCESS_ACCESS_TOKEN),          sizeof(ULONG), ICIF_SET ),                       /* ProcessAccessToken */
  ICI_SQ_SAME( 0 /* FIXME */,                         sizeof(ULONG), ICIF_QUERY | ICIF_SET ),          /* ProcessLdtInformation */
  ICI_SQ_SAME( 0 /* FIXME */,                         sizeof(ULONG), ICIF_SET ),                       /* ProcessLdtSize */
  ICI_SQ_SAME( sizeof(ULONG),                         sizeof(ULONG), ICIF_QUERY | ICIF_SET ),          /* ProcessDefaultHardErrorMode */
  ICI_SQ_SAME( 0 /* FIXME */,                         sizeof(ULONG), ICIF_SET ),                       /* ProcessIoPortHandlers */
  ICI_SQ_SAME( sizeof(POOLED_USAGE_AND_LIMITS),       sizeof(ULONG), ICIF_QUERY ),                     /* ProcessPooledUsageAndLimits */
  ICI_SQ_SAME( sizeof(PROCESS_WS_WATCH_INFORMATION),  sizeof(ULONG), ICIF_QUERY | ICIF_SET ),          /* ProcessWorkingSetWatch */
  ICI_SQ_SAME( 0 /* FIXME */,                         sizeof(ULONG), ICIF_SET ),                       /* ProcessUserModeIOPL */
  ICI_SQ_SAME( sizeof(BOOLEAN),                       sizeof(ULONG), ICIF_SET ),                       /* ProcessEnableAlignmentFaultFixup */
  ICI_SQ_SAME( sizeof(PROCESS_PRIORITY_CLASS),        sizeof(USHORT), ICIF_QUERY | ICIF_SET ),         /* ProcessPriorityClass */
  ICI_SQ_SAME( sizeof(ULONG),                         sizeof(ULONG), ICIF_QUERY ),                     /* ProcessWx86Information */
  ICI_SQ_SAME( sizeof(ULONG),                         sizeof(ULONG), ICIF_QUERY ),                     /* ProcessHandleCount */
  ICI_SQ_SAME( sizeof(KAFFINITY),                     sizeof(ULONG), ICIF_SET ),                       /* ProcessAffinityMask */
  ICI_SQ_SAME( sizeof(ULONG),                         sizeof(ULONG), ICIF_QUERY | ICIF_SET ),          /* ProcessPriorityBoost */

  ICI_SQ(/*Q*/ sizeof(((PPROCESS_DEVICEMAP_INFORMATION)0x0)->Query),                                   /* ProcessDeviceMap */
         /*S*/ sizeof(((PPROCESS_DEVICEMAP_INFORMATION)0x0)->Set),
                                                /*Q*/ sizeof(ULONG),
                                                /*S*/ sizeof(ULONG),
                                                                     ICIF_QUERY | ICIF_SET ),

  ICI_SQ_SAME( sizeof(PROCESS_SESSION_INFORMATION),   sizeof(ULONG), ICIF_QUERY | ICIF_SET ),          /* ProcessSessionInformation */
  ICI_SQ_SAME( sizeof(BOOLEAN),                       sizeof(ULONG), ICIF_SET ),                       /* ProcessForegroundInformation */
  ICI_SQ_SAME( sizeof(ULONG),                         sizeof(ULONG), ICIF_QUERY ),                     /* ProcessWow64Information */
  ICI_SQ_SAME( sizeof(UNICODE_STRING),                sizeof(ULONG), ICIF_QUERY | ICIF_SIZE_VARIABLE), /* ProcessImageFileName */

  /* FIXME */
  ICI_SQ_SAME( 0,                                     1,             0 ),                              /* ProcessLUIDDeviceMapsEnabled */
  ICI_SQ_SAME( 0,                                     1,             0 ),                              /* ProcessBreakOnTermination */
  ICI_SQ_SAME( 0,                                     1,             0 ),                              /* ProcessDebugObjectHandle */
  ICI_SQ_SAME( 0,                                     1,             0 ),                              /* ProcessDebugFlags */
  ICI_SQ_SAME( 0,                                     1,             0 ),                              /* ProcessHandleTracing */
  ICI_SQ_SAME( 0,                                     1,             0 ),                              /* ProcessUnknown33 */
  ICI_SQ_SAME( 0,                                     1,             0 ),                              /* ProcessUnknown34 */
  ICI_SQ_SAME( 0,                                     1,             0 ),                              /* ProcessUnknown35 */

  ICI_SQ_SAME( sizeof(ULONG),                         sizeof(ULONG), ICIF_QUERY),                      /* ProcessCookie */
};

/*
 * FIXME:
 *   Remove the Implemented value if all functions are implemented.
 */

static const struct
{
   BOOLEAN Implemented;
   ULONG Size;
} QueryInformationData[MaxThreadInfoClass + 1] =
{
    {TRUE, sizeof(THREAD_BASIC_INFORMATION)},	// ThreadBasicInformation
    {TRUE, sizeof(KERNEL_USER_TIMES)},		// ThreadTimes
    {TRUE, 0},					// ThreadPriority
    {TRUE, 0},					// ThreadBasePriority
    {TRUE, 0},					// ThreadAffinityMask
    {TRUE, 0},					// ThreadImpersonationToken
    {FALSE, 0},					// ThreadDescriptorTableEntry
    {TRUE, 0},					// ThreadEnableAlignmentFaultFixup
    {TRUE, 0},					// ThreadEventPair
    {TRUE, sizeof(PVOID)},			// ThreadQuerySetWin32StartAddress
    {TRUE, 0},					// ThreadZeroTlsCell
    {TRUE, sizeof(LARGE_INTEGER)},		// ThreadPerformanceCount
    {TRUE, sizeof(BOOLEAN)},			// ThreadAmILastThread
    {TRUE, 0},					// ThreadIdealProcessor
    {FALSE, 0},					// ThreadPriorityBoost
    {TRUE, 0},					// ThreadSetTlsArrayAddress
    {FALSE, 0},					// ThreadIsIoPending
    {TRUE, 0}					// ThreadHideFromDebugger
};

static const struct
{
   BOOLEAN Implemented;
   ULONG Size;
} SetInformationData[MaxThreadInfoClass + 1] =
{
    {TRUE, 0},			// ThreadBasicInformation
    {TRUE, 0},			// ThreadTimes
    {TRUE, sizeof(KPRIORITY)},	// ThreadPriority
    {TRUE, sizeof(LONG)},	// ThreadBasePriority
    {TRUE, sizeof(KAFFINITY)},	// ThreadAffinityMask
    {TRUE, sizeof(HANDLE)},	// ThreadImpersonationToken
    {TRUE, 0},			// ThreadDescriptorTableEntry
    {FALSE, 0},			// ThreadEnableAlignmentFaultFixup
    {FALSE, 0},			// ThreadEventPair
    {TRUE, sizeof(PVOID)},	// ThreadQuerySetWin32StartAddress
    {FALSE, 0},			// ThreadZeroTlsCell
    {TRUE, 0},			// ThreadPerformanceCount
    {TRUE, 0},			// ThreadAmILastThread
    {FALSE, 0},			// ThreadIdealProcessor
    {FALSE, 0},			// ThreadPriorityBoost
    {FALSE, 0},			// ThreadSetTlsArrayAddress
    {TRUE, 0},			// ThreadIsIoPending
    {FALSE, 0}			// ThreadHideFromDebugger
};

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS STDCALL
NtQueryInformationProcess(IN  HANDLE ProcessHandle,
			  IN  PROCESSINFOCLASS ProcessInformationClass,
			  OUT PVOID ProcessInformation,
			  IN  ULONG ProcessInformationLength,
			  OUT PULONG ReturnLength  OPTIONAL)
{
   PEPROCESS Process;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;

   PAGED_CODE();

   PreviousMode = ExGetPreviousMode();

   DefaultQueryInfoBufferCheck(ProcessInformationClass,
                               PsProcessInfoClass,
                               ProcessInformation,
                               ProcessInformationLength,
                               ReturnLength,
                               PreviousMode,
                               &Status);
   if(!NT_SUCCESS(Status))
   {
     DPRINT1("NtQueryInformationProcess() failed, Status: 0x%x\n", Status);
     return Status;
   }

   if(ProcessInformationClass != ProcessCookie)
   {
     Status = ObReferenceObjectByHandle(ProcessHandle,
  				      PROCESS_QUERY_INFORMATION,
  				      PsProcessType,
  				      PreviousMode,
  				      (PVOID*)&Process,
  				      NULL);
     if (!NT_SUCCESS(Status))
       {
  	return(Status);
       }
   }
   else if(ProcessHandle != NtCurrentProcess())
   {
     /* retreiving the process cookie is only allowed for the calling process
        itself! XP only allowes NtCurrentProcess() as process handles even if a
        real handle actually represents the current process. */
     return STATUS_INVALID_PARAMETER;
   }

   switch (ProcessInformationClass)
     {
      case ProcessBasicInformation:
      {
        PPROCESS_BASIC_INFORMATION ProcessBasicInformationP =
	  (PPROCESS_BASIC_INFORMATION)ProcessInformation;

        _SEH_TRY
        {
	  ProcessBasicInformationP->ExitStatus = Process->ExitStatus;
	  ProcessBasicInformationP->PebBaseAddress = Process->Peb;
	  ProcessBasicInformationP->AffinityMask = Process->Pcb.Affinity;
	  ProcessBasicInformationP->UniqueProcessId =
	    (ULONG)Process->UniqueProcessId;
	  ProcessBasicInformationP->InheritedFromUniqueProcessId =
	    (ULONG)Process->InheritedFromUniqueProcessId;
	  ProcessBasicInformationP->BasePriority =
	    Process->Pcb.BasePriority;

	  if (ReturnLength)
	  {
	    *ReturnLength = sizeof(PROCESS_BASIC_INFORMATION);
	  }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
	break;
      }

      case ProcessQuotaLimits:
      case ProcessIoCounters:
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case ProcessTimes:
      {
         PKERNEL_USER_TIMES ProcessTimeP = (PKERNEL_USER_TIMES)ProcessInformation;
         _SEH_TRY
         {
	    ProcessTimeP->CreateTime = Process->CreateTime;
            ProcessTimeP->UserTime.QuadPart = Process->Pcb.UserTime * 100000LL;
            ProcessTimeP->KernelTime.QuadPart = Process->Pcb.KernelTime * 100000LL;
	    ProcessTimeP->ExitTime = Process->ExitTime;

	   if (ReturnLength)
	   {
	     *ReturnLength = sizeof(KERNEL_USER_TIMES);
	   }
         }
         _SEH_HANDLE
         {
           Status = _SEH_GetExceptionCode();
         }
         _SEH_END;
	 break;
      }

      case ProcessDebugPort:
      {
        _SEH_TRY
        {
          *(PHANDLE)ProcessInformation = (Process->DebugPort != NULL ? (HANDLE)-1 : NULL);
	  if (ReturnLength)
	  {
	    *ReturnLength = sizeof(HANDLE);
	  }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        break;
      }

      case ProcessLdtInformation:
      case ProcessWorkingSetWatch:
      case ProcessWx86Information:
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case ProcessHandleCount:
      {
	ULONG HandleCount = ObpGetHandleCountByHandleTable(Process->ObjectTable);

	_SEH_TRY
	{
          *(PULONG)ProcessInformation = HandleCount;
	  if (ReturnLength)
	  {
	    *ReturnLength = sizeof(ULONG);
	  }
	}
	_SEH_HANDLE
	{
          Status = _SEH_GetExceptionCode();
	}
	_SEH_END;
	break;
      }

      case ProcessSessionInformation:
      {
        PPROCESS_SESSION_INFORMATION SessionInfo = (PPROCESS_SESSION_INFORMATION)ProcessInformation;

        _SEH_TRY
        {
          SessionInfo->SessionId = Process->Session;
          if (ReturnLength)
          {
            *ReturnLength = sizeof(PROCESS_SESSION_INFORMATION);
          }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        break;
      }

      case ProcessWow64Information:
        DPRINT1("We currently don't support the ProcessWow64Information information class!\n");
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case ProcessVmCounters:
      {
	PVM_COUNTERS pOut = (PVM_COUNTERS)ProcessInformation;

	_SEH_TRY
	{
	  pOut->PeakVirtualSize            = Process->PeakVirtualSize;
	  /*
	   * Here we should probably use VirtualSize.LowPart, but due to
	   * incompatibilities in current headers (no unnamed union),
	   * I opted for cast.
	   */
	  pOut->VirtualSize                = (ULONG)Process->VirtualSize;
	  pOut->PageFaultCount             = Process->Vm.PageFaultCount;
	  pOut->PeakWorkingSetSize         = Process->Vm.PeakWorkingSetSize;
	  pOut->WorkingSetSize             = Process->Vm.WorkingSetSize;
	  pOut->QuotaPeakPagedPoolUsage    = Process->QuotaPeak[0]; // TODO: Verify!
	  pOut->QuotaPagedPoolUsage        = Process->QuotaUsage[0];     // TODO: Verify!
	  pOut->QuotaPeakNonPagedPoolUsage = Process->QuotaPeak[1]; // TODO: Verify!
	  pOut->QuotaNonPagedPoolUsage     = Process->QuotaUsage[1];     // TODO: Verify!
	  pOut->PagefileUsage              = Process->QuotaUsage[2];
	  pOut->PeakPagefileUsage          = Process->QuotaPeak[2];

	  if (ReturnLength)
	  {
	    *ReturnLength = sizeof(VM_COUNTERS);
	  }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
	break;
      }

      case ProcessDefaultHardErrorMode:
      {
	PULONG HardErrMode = (PULONG)ProcessInformation;
	_SEH_TRY
	{
	  *HardErrMode = Process->DefaultHardErrorProcessing;
	  if (ReturnLength)
	  {
	    *ReturnLength = sizeof(ULONG);
	  }
	}
	_SEH_HANDLE
	{
          Status = _SEH_GetExceptionCode();
	}
	_SEH_END;
	break;
      }

      case ProcessPriorityBoost:
      {
	PULONG BoostEnabled = (PULONG)ProcessInformation;

	_SEH_TRY
	{
	  *BoostEnabled = Process->Pcb.DisableBoost ? FALSE : TRUE;

	  if (ReturnLength)
	  {
	    *ReturnLength = sizeof(ULONG);
	  }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
	break;
      }

      case ProcessDeviceMap:
      {
        PROCESS_DEVICEMAP_INFORMATION DeviceMap;

        ObQueryDeviceMapInformation(Process, &DeviceMap);

        _SEH_TRY
        {
          *(PPROCESS_DEVICEMAP_INFORMATION)ProcessInformation = DeviceMap;
	  if (ReturnLength)
          {
	    *ReturnLength = sizeof(PROCESS_DEVICEMAP_INFORMATION);
	  }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
	break;
      }

      case ProcessPriorityClass:
      {
	PUSHORT Priority = (PUSHORT)ProcessInformation;

	_SEH_TRY
	{
	  *Priority = Process->PriorityClass;

	  if (ReturnLength)
	  {
	    *ReturnLength = sizeof(USHORT);
	  }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
	break;
      }

      case ProcessImageFileName:
      {
        /*
         * We DO NOT return the file name stored in the EPROCESS structure.
         * Propably if we can't find a PEB or ProcessParameters structure for the
         * process!
         */
        if(Process->Peb != NULL)
        {
          PRTL_USER_PROCESS_PARAMETERS ProcParams = NULL;
          UNICODE_STRING LocalDest;
          BOOLEAN Attached;
          ULONG ImagePathLen = 0;
          PUNICODE_STRING DstPath = (PUNICODE_STRING)ProcessInformation;

          /* we need to attach to the process to make sure we're in the right context! */
          Attached = Process != PsGetCurrentProcess();

          if(Attached)
            KeAttachProcess(&Process->Pcb);

          _SEH_TRY
          {
            ProcParams = Process->Peb->ProcessParameters;
            ImagePathLen = ProcParams->ImagePathName.Length;
          }
          _SEH_HANDLE
          {
            Status = _SEH_GetExceptionCode();
          }
          _SEH_END;

          if(NT_SUCCESS(Status))
          {
            if(ProcessInformationLength < sizeof(UNICODE_STRING) + ImagePathLen + sizeof(WCHAR))
            {
              Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
              PWSTR StrSource = NULL;

              RtlZeroMemory(&LocalDest, sizeof(LocalDest));

              /* create a DstPath structure on the stack */
              _SEH_TRY
              {
                LocalDest.Length = ImagePathLen;
                LocalDest.MaximumLength = ImagePathLen + sizeof(WCHAR);
                LocalDest.Buffer = (PWSTR)(DstPath + 1);

                /* save a copy of the pointer to the source buffer */
                StrSource = ProcParams->ImagePathName.Buffer;
              }
              _SEH_HANDLE
              {
                Status = _SEH_GetExceptionCode();
              }
              _SEH_END;

              if(NT_SUCCESS(Status))
              {
                /* now, let's allocate some anonymous memory to copy the string to.
                   we can't just copy it to the buffer the caller pointed as it might
                   be user memory in another context */
                PWSTR PathCopy = ExAllocatePool(PagedPool, LocalDest.Length + sizeof(WCHAR));
                if(PathCopy != NULL)
                {
                  /* make a copy of the buffer to the temporary buffer */
                  _SEH_TRY
                  {
                    RtlCopyMemory(PathCopy, StrSource, LocalDest.Length);
                    PathCopy[LocalDest.Length / sizeof(WCHAR)] = L'\0';
                  }
                  _SEH_HANDLE
                  {
                    Status = _SEH_GetExceptionCode();
                  }
                  _SEH_END;

                  /* detach from the process */
                  if(Attached)
                    KeDetachProcess();

                  /* only copy the string back to the caller if we were able to
                     copy it into the temporary buffer! */
                  if(NT_SUCCESS(Status))
                  {
                    /* now let's copy the buffer back to the caller */
                    _SEH_TRY
                    {
                      *DstPath = LocalDest;
                      RtlCopyMemory(LocalDest.Buffer, PathCopy, LocalDest.Length + sizeof(WCHAR));
                      if (ReturnLength)
                      {
                        *ReturnLength = sizeof(UNICODE_STRING) + LocalDest.Length + sizeof(WCHAR);
                      }
                    }
                    _SEH_HANDLE
                    {
                      Status = _SEH_GetExceptionCode();
                    }
                    _SEH_END;
                  }

                  /* we're done with the copy operation, free the temporary kernel buffer */
                  ExFreePool(PathCopy);

                  /* we need to bail because we're already detached from the process */
                  break;
                }
                else
                {
                  Status = STATUS_INSUFFICIENT_RESOURCES;
                }
              }
            }
          }

          /* don't forget to detach from the process!!! */
          if(Attached)
            KeDetachProcess();
        }
        else
        {
          /* FIXME - what to do here? */
          Status = STATUS_UNSUCCESSFUL;
        }
        break;
      }

      case ProcessCookie:
      {
        ULONG Cookie;

        /* receive the process cookie, this is only allowed for the current
           process! */

        Process = PsGetCurrentProcess();

        Cookie = Process->Cookie;
        if(Cookie == 0)
        {
          LARGE_INTEGER SystemTime;
          ULONG NewCookie;
          PKPRCB Prcb;

          /* generate a new cookie */

          KeQuerySystemTime(&SystemTime);

          Prcb = KeGetCurrentPrcb();

          NewCookie = Prcb->KeSystemCalls ^ Prcb->InterruptTime ^
                      SystemTime.u.LowPart ^ SystemTime.u.HighPart;

          /* try to set the new cookie, return the current one if another thread
             set it in the meanwhile */
          Cookie = InterlockedCompareExchange((LONG*)&Process->Cookie,
                                              NewCookie,
                                              Cookie);
          if(Cookie == 0)
          {
            /* successfully set the cookie */
            Cookie = NewCookie;
          }
        }

        _SEH_TRY
        {
          *(PULONG)ProcessInformation = Cookie;
	  if (ReturnLength)
          {
	    *ReturnLength = sizeof(ULONG);
	  }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        break;
      }

      /*
       * Note: The following 10 information classes are verified to not be
       * implemented on NT, and do indeed return STATUS_INVALID_INFO_CLASS;
       */
      case ProcessBasePriority:
      case ProcessRaisePriority:
      case ProcessExceptionPort:
      case ProcessAccessToken:
      case ProcessLdtSize:
      case ProcessIoPortHandlers:
      case ProcessUserModeIOPL:
      case ProcessEnableAlignmentFaultFixup:
      case ProcessAffinityMask:
      case ProcessForegroundInformation:
      default:
	Status = STATUS_INVALID_INFO_CLASS;
     }

   if(ProcessInformationClass != ProcessCookie)
   {
     ObDereferenceObject(Process);
   }

   return Status;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
NtSetInformationProcess(IN HANDLE ProcessHandle,
			IN PROCESSINFOCLASS ProcessInformationClass,
			IN PVOID ProcessInformation,
			IN ULONG ProcessInformationLength)
{
   PEPROCESS Process;
   KPROCESSOR_MODE PreviousMode;
   ACCESS_MASK Access;
   NTSTATUS Status = STATUS_SUCCESS;

   PAGED_CODE();

   PreviousMode = ExGetPreviousMode();

   DefaultSetInfoBufferCheck(ProcessInformationClass,
                             PsProcessInfoClass,
                             ProcessInformation,
                             ProcessInformationLength,
                             PreviousMode,
                             &Status);
   if(!NT_SUCCESS(Status))
   {
     DPRINT1("NtSetInformationProcess() %d %x  %x called\n", ProcessInformationClass, ProcessInformation, ProcessInformationLength);
     DPRINT1("NtSetInformationProcess() %x failed, Status: 0x%x\n", Status);
     return Status;
   }

   switch(ProcessInformationClass)
   {
     case ProcessSessionInformation:
       Access = PROCESS_SET_INFORMATION | PROCESS_SET_SESSIONID;
       break;
     case ProcessExceptionPort:
       Access = PROCESS_SET_INFORMATION | PROCESS_SUSPEND_RESUME;
       break;

     default:
       Access = PROCESS_SET_INFORMATION;
       break;
   }

   Status = ObReferenceObjectByHandle(ProcessHandle,
				      Access,
				      PsProcessType,
				      PreviousMode,
				      (PVOID*)&Process,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   switch (ProcessInformationClass)
     {
      case ProcessQuotaLimits:
      case ProcessBasePriority:
      case ProcessRaisePriority:
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case ProcessExceptionPort:
      {
        HANDLE PortHandle = NULL;

        /* make a safe copy of the buffer on the stack */
        _SEH_TRY
        {
          PortHandle = *(PHANDLE)ProcessInformation;
          Status = STATUS_SUCCESS;
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(NT_SUCCESS(Status))
        {
          PEPORT ExceptionPort;

          /* in case we had success reading from the buffer, verify the provided
           * LPC port handle
           */
          Status = ObReferenceObjectByHandle(PortHandle,
                                             0,
                                             LpcPortObjectType,
                                             PreviousMode,
                                             (PVOID)&ExceptionPort,
                                             NULL);
          if(NT_SUCCESS(Status))
          {
            /* lock the process to be thread-safe! */

            Status = PsLockProcess(Process, FALSE);
            if(NT_SUCCESS(Status))
            {
              /*
               * according to "NT Native API" documentation, setting the exception
               * port is only permitted once!
               */
              if(Process->ExceptionPort == NULL)
              {
                /* keep the reference to the handle! */
                Process->ExceptionPort = ExceptionPort;
                Status = STATUS_SUCCESS;
              }
              else
              {
                ObDereferenceObject(ExceptionPort);
                Status = STATUS_PORT_ALREADY_SET;
              }
              PsUnlockProcess(Process);
            }
            else
            {
              ObDereferenceObject(ExceptionPort);
            }
          }
        }
        break;
      }

      case ProcessAccessToken:
      {
        HANDLE TokenHandle = NULL;

        /* make a safe copy of the buffer on the stack */
        _SEH_TRY
        {
          TokenHandle = ((PPROCESS_ACCESS_TOKEN)ProcessInformation)->Token;
          Status = STATUS_SUCCESS;
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(NT_SUCCESS(Status))
        {
          /* in case we had success reading from the buffer, perform the actual task */
          Status = PspAssignPrimaryToken(Process, TokenHandle);
        }
	break;
      }

      case ProcessDefaultHardErrorMode:
      {
        _SEH_TRY
        {
          InterlockedExchange((LONG*)&Process->DefaultHardErrorProcessing,
                              *(PLONG)ProcessInformation);
          Status = STATUS_SUCCESS;
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        break;
      }

      case ProcessSessionInformation:
      {
        PROCESS_SESSION_INFORMATION SessionInfo;
        Status = STATUS_SUCCESS;

        RtlZeroMemory(&SessionInfo, sizeof(SessionInfo));

        _SEH_TRY
        {
          /* copy the structure to the stack */
          SessionInfo = *(PPROCESS_SESSION_INFORMATION)ProcessInformation;
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(NT_SUCCESS(Status))
        {
          /* we successfully copied the structure to the stack, continue processing */

          /*
           * setting the session id requires the SeTcbPrivilege!
           */
          if(!SeSinglePrivilegeCheck(SeTcbPrivilege,
                                     PreviousMode))
          {
            DPRINT1("NtSetInformationProcess: Caller requires the SeTcbPrivilege privilege for setting ProcessSessionInformation!\n");
            /* can't set the session id, bail! */
            Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
          }

          /* FIXME - update the session id for the process token */

          Status = PsLockProcess(Process, FALSE);
          if(NT_SUCCESS(Status))
          {
            Process->Session = SessionInfo.SessionId;

            /* Update the session id in the PEB structure */
            if(Process->Peb != NULL)
            {
              /* we need to attach to the process to make sure we're in the right
                 context to access the PEB structure */
              KeAttachProcess(&Process->Pcb);

              _SEH_TRY
              {
                /* FIXME: Process->Peb->SessionId = SessionInfo.SessionId; */

                Status = STATUS_SUCCESS;
              }
              _SEH_HANDLE
              {
                Status = _SEH_GetExceptionCode();
              }
              _SEH_END;

              KeDetachProcess();
            }

            PsUnlockProcess(Process);
          }
        }
        break;
      }

      case ProcessPriorityClass:
      {
        PROCESS_PRIORITY_CLASS ppc;

        _SEH_TRY
        {
          ppc = *(PPROCESS_PRIORITY_CLASS)ProcessInformation;
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(NT_SUCCESS(Status))
        {
        }

        break;
      }

      case ProcessLdtInformation:
      case ProcessLdtSize:
      case ProcessIoPortHandlers:
      case ProcessWorkingSetWatch:
      case ProcessUserModeIOPL:
      case ProcessEnableAlignmentFaultFixup:
      case ProcessAffinityMask:
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case ProcessBasicInformation:
      case ProcessIoCounters:
      case ProcessTimes:
      case ProcessPooledUsageAndLimits:
      case ProcessWx86Information:
      case ProcessHandleCount:
      case ProcessWow64Information:
      case ProcessDebugPort:
      default:
	Status = STATUS_INVALID_INFO_CLASS;
     }
   ObDereferenceObject(Process);
   return(Status);
}


/**********************************************************************
 * NAME							INTERNAL
 * 	PiQuerySystemProcessInformation
 *
 * DESCRIPTION
 * 	Compute the size of a process+thread snapshot as
 * 	expected by NtQuerySystemInformation.
 *
 * RETURN VALUE
 * 	0 on error; otherwise the size, in bytes of the buffer
 * 	required to write a full snapshot.
 *
 * NOTE
 * 	We assume (sizeof (PVOID) == sizeof (ULONG)) holds.
 */
NTSTATUS
PiQuerySystemProcessInformation(PVOID Buffer,
				ULONG Size,
				PULONG ReqSize)
{
   return STATUS_NOT_IMPLEMENTED;

#if 0
	PLIST_ENTRY	CurrentEntryP;
	PEPROCESS	CurrentP;
	PLIST_ENTRY	CurrentEntryT;
	PETHREAD	CurrentT;

	ULONG		RequiredSize = 0L;
	BOOLEAN		SizeOnly = FALSE;

	ULONG		SpiSize = 0L;

	PSYSTEM_PROCESS_INFORMATION	pInfoP = (PSYSTEM_PROCESS_INFORMATION) SnapshotBuffer;
	PSYSTEM_PROCESS_INFORMATION	pInfoPLast = NULL;
	PSYSTEM_THREAD_INFO		pInfoT = NULL;


   /* Lock the process list. */
   ExAcquireFastMutex(&PspActiveProcessMutex);

	/*
	 * Scan the process list. Since the
	 * list is circular, the guard is false
	 * after the last process.
	 */
	for (	CurrentEntryP = PsActiveProcessHead.Flink;
		(CurrentEntryP != & PsActiveProcessHead);
		CurrentEntryP = CurrentEntryP->Flink
		)
	{
		/*
		 * Compute how much space is
		 * occupied in the snapshot
		 * by adding this process info.
		 * (at least one thread).
		 */
		SpiSizeCurrent = sizeof (SYSTEM_PROCESS_INFORMATION);
		RequiredSize += SpiSizeCurrent;
		/*
		 * Do not write process data in the
		 * buffer if it is too small.
		 */
		if (TRUE == SizeOnly) continue;
		/*
		 * Check if the buffer can contain
		 * the full snapshot.
		 */
		if (Size < RequiredSize)
		{
			SizeOnly = TRUE;
			continue;
		}
		/*
		 * Get a reference to the
		 * process descriptor we are
		 * handling.
		 */
		CurrentP = CONTAINING_RECORD(
				CurrentEntryP,
				EPROCESS,
				ProcessListEntry
				);
		/*
		 * Write process data in the buffer.
		 */
		RtlZeroMemory (pInfoP, sizeof (SYSTEM_PROCESS_INFORMATION));
		/* PROCESS */
		pInfoP->ThreadCount = 0L;
		pInfoP->ProcessId = CurrentP->UniqueProcessId;
		RtlInitUnicodeString (
			& pInfoP->Name,
			CurrentP->ImageFileName
			);
		/* THREAD */
		for (	pInfoT = & CurrentP->ThreadSysInfo [0],
			CurrentEntryT = CurrentP->ThreadListHead.Flink;

			(CurrentEntryT != & CurrentP->ThreadListHead);

			pInfoT = & CurrentP->ThreadSysInfo [pInfoP->ThreadCount],
			CurrentEntryT = CurrentEntryT->Flink
			)
		{
			/*
			 * Recalculate the size of the
			 * information block.
			 */
			if (0 < pInfoP->ThreadCount)
			{
				RequiredSize += sizeof (SYSTEM_THREAD_INFORMATION);
			}
			/*
			 * Do not write thread data in the
			 * buffer if it is too small.
			 */
			if (TRUE == SizeOnly) continue;
			/*
			 * Check if the buffer can contain
			 * the full snapshot.
			 */
			if (Size < RequiredSize)
			{
				SizeOnly = TRUE;
				continue;
			}
			/*
			 * Get a reference to the
			 * thread descriptor we are
			 * handling.
			 */
			CurrentT = CONTAINING_RECORD(
					CurrentEntryT,
					KTHREAD,
					ThreadListEntry
					);
			/*
			 * Write thread data.
			 */
			RtlZeroMemory (
				pInfoT,
				sizeof (SYSTEM_THREAD_INFORMATION)
				);
			pInfoT->KernelTime	= CurrentT-> ;	/* TIME */
			pInfoT->UserTime	= CurrentT-> ;	/* TIME */
			pInfoT->CreateTime	= CurrentT-> ;	/* TIME */
			pInfoT->TickCount	= CurrentT-> ;	/* ULONG */
			pInfoT->StartEIP	= CurrentT-> ;	/* ULONG */
			pInfoT->ClientId	= CurrentT-> ;	/* CLIENT_ID */
			pInfoT->ClientId	= CurrentT-> ;	/* CLIENT_ID */
			pInfoT->DynamicPriority	= CurrentT-> ;	/* ULONG */
			pInfoT->BasePriority	= CurrentT-> ;	/* ULONG */
			pInfoT->nSwitches	= CurrentT-> ;	/* ULONG */
			pInfoT->State		= CurrentT-> ;	/* DWORD */
			pInfoT->WaitReason	= CurrentT-> ;	/* KWAIT_REASON */
			/*
			 * Count the number of threads
			 * this process has.
			 */
			++ pInfoP->ThreadCount;
		}
		/*
		 * Save the size of information
		 * stored in the buffer for the
		 * current process.
		 */
		pInfoP->RelativeOffset = SpiSize;
		/*
		 * Save a reference to the last
		 * valid information block.
		 */
		pInfoPLast = pInfoP;
		/*
		 * Compute the offset of the
		 * SYSTEM_PROCESS_INFORMATION
		 * descriptor in the snapshot
		 * buffer for the next process.
		 */
		(ULONG) pInfoP += SpiSize;
	}
	/*
	 * Unlock the process list.
	 */
	ExReleaseFastMutex (
		& PspActiveProcessMutex
		);
	/*
	 * Return the proper error status code,
	 * if the buffer was too small.
	 */
	if (TRUE == SizeOnly)
	{
		if (NULL != RequiredSize)
		{
			*pRequiredSize = RequiredSize;
		}
		return STATUS_INFO_LENGTH_MISMATCH;
	}
	/*
	 * Mark the end of the snapshot.
	 */
	pInfoP->RelativeOffset = 0L;
	/* OK */
	return STATUS_SUCCESS;
#endif
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
NtSetInformationThread (IN HANDLE ThreadHandle,
			IN THREADINFOCLASS ThreadInformationClass,
			IN PVOID ThreadInformation,
			IN ULONG ThreadInformationLength)
{
  PETHREAD Thread;
  union
  {
     KPRIORITY Priority;
     LONG Increment;
     KAFFINITY Affinity;
     HANDLE Handle;
     PVOID Address;
  }u;
  KPROCESSOR_MODE PreviousMode;
  NTSTATUS Status = STATUS_SUCCESS;

  PAGED_CODE();
  
  PreviousMode = ExGetPreviousMode();

  if (ThreadInformationClass <= MaxThreadInfoClass &&
      !SetInformationData[ThreadInformationClass].Implemented)
    {
      return STATUS_NOT_IMPLEMENTED;
    }
  if (ThreadInformationClass > MaxThreadInfoClass ||
      SetInformationData[ThreadInformationClass].Size == 0)
    {
      return STATUS_INVALID_INFO_CLASS;
    }
  if (ThreadInformationLength != SetInformationData[ThreadInformationClass].Size)
    {
      return STATUS_INFO_LENGTH_MISMATCH;
    }

  if (PreviousMode != KernelMode)
    {
      _SEH_TRY
        {
          ProbeForRead(ThreadInformation,
                       SetInformationData[ThreadInformationClass].Size,
                       1);
          RtlCopyMemory(&u.Priority,
                        ThreadInformation,
                        SetInformationData[ThreadInformationClass].Size);
        }
      _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
      _SEH_END;
      
      if (!NT_SUCCESS(Status))
        {
          return Status;
        }
    }
  else
    {
      RtlCopyMemory(&u.Priority,
                    ThreadInformation,
                    SetInformationData[ThreadInformationClass].Size);
    }

  Status = ObReferenceObjectByHandle (ThreadHandle,
				      THREAD_SET_INFORMATION,
				      PsThreadType,
				      ExGetPreviousMode (),
				      (PVOID*)&Thread,
				      NULL);
   if (NT_SUCCESS(Status))
     {
       switch (ThreadInformationClass)
         {
           case ThreadPriority:
	     if (u.Priority < LOW_PRIORITY || u.Priority >= MAXIMUM_PRIORITY)
	       {
		 Status = STATUS_INVALID_PARAMETER;
		 break;
	       }
	     KeSetPriorityThread(&Thread->Tcb, u.Priority);
	     break;

           case ThreadBasePriority:
	     KeSetBasePriorityThread (&Thread->Tcb, u.Increment);
	     break;

           case ThreadAffinityMask:
	     Status = KeSetAffinityThread(&Thread->Tcb, u.Affinity);
	     break;

           case ThreadImpersonationToken:
	     Status = PsAssignImpersonationToken (Thread, u.Handle);
	     break;

           case ThreadQuerySetWin32StartAddress:
	     Thread->Win32StartAddress = u.Address;
	     break;

           default:
	     /* Shoult never occure if the data table is correct */
	     KEBUGCHECK(0);
	 }
     }
  ObDereferenceObject (Thread);

  return Status;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
NtQueryInformationThread (IN	HANDLE		ThreadHandle,
			  IN	THREADINFOCLASS	ThreadInformationClass,
			  OUT	PVOID		ThreadInformation,
			  IN	ULONG		ThreadInformationLength,
			  OUT	PULONG		ReturnLength  OPTIONAL)
{
   PETHREAD Thread;
   union
   {
      THREAD_BASIC_INFORMATION TBI;
      KERNEL_USER_TIMES TTI;
      PVOID Address;
      LARGE_INTEGER Count;
      BOOLEAN Last;
   }u;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;

   PAGED_CODE();
   
   PreviousMode = ExGetPreviousMode();

   if (ThreadInformationClass <= MaxThreadInfoClass &&
       !QueryInformationData[ThreadInformationClass].Implemented)
     {
       return STATUS_NOT_IMPLEMENTED;
     }
   if (ThreadInformationClass > MaxThreadInfoClass ||
       QueryInformationData[ThreadInformationClass].Size == 0)
     {
       return STATUS_INVALID_INFO_CLASS;
     }
   if (ThreadInformationLength != QueryInformationData[ThreadInformationClass].Size)
     {
       return STATUS_INFO_LENGTH_MISMATCH;
     }

   if (PreviousMode != KernelMode)
     {
       _SEH_TRY
         {
           ProbeForWrite(ThreadInformation,
                         QueryInformationData[ThreadInformationClass].Size,
                         1);
           if (ReturnLength != NULL)
             {
               ProbeForWrite(ReturnLength,
                             sizeof(ULONG),
                             sizeof(ULONG));
             }
         }
       _SEH_HANDLE
         {
           Status = _SEH_GetExceptionCode();
         }
       _SEH_END;
       
       if (!NT_SUCCESS(Status))
         {
           return Status;
         }
     }

   Status = ObReferenceObjectByHandle(ThreadHandle,
				      THREAD_QUERY_INFORMATION,
				      PsThreadType,
				      ExGetPreviousMode(),
				      (PVOID*)&Thread,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return Status;
     }

   switch (ThreadInformationClass)
     {
       case ThreadBasicInformation:
         /* A test on W2K agains ntdll shows NtQueryInformationThread return STATUS_PENDING
          * as ExitStatus for current/running thread, while KETHREAD's ExitStatus is
          * 0. So do the conversion here:
          * -Gunnar     */
         u.TBI.ExitStatus = (Thread->ExitStatus == 0) ? STATUS_PENDING : Thread->ExitStatus;
	 u.TBI.TebBaseAddress = (PVOID)Thread->Tcb.Teb;
	 u.TBI.ClientId = Thread->Cid;
	 u.TBI.AffinityMask = Thread->Tcb.Affinity;
	 u.TBI.Priority = Thread->Tcb.Priority;
	 u.TBI.BasePriority = KeQueryBasePriorityThread(&Thread->Tcb);
	 break;

       case ThreadTimes:
	 u.TTI.KernelTime.QuadPart = Thread->Tcb.KernelTime * 100000LL;
         u.TTI.UserTime.QuadPart = Thread->Tcb.UserTime * 100000LL;
         u.TTI.CreateTime = Thread->CreateTime;
         /*This works*/
	 u.TTI.ExitTime = Thread->ExitTime;
         break;

       case ThreadQuerySetWin32StartAddress:
         u.Address = Thread->Win32StartAddress;
         break;

       case ThreadPerformanceCount:
         /* Nebbett says this class is always zero */
         u.Count.QuadPart = 0;
         break;

       case ThreadAmILastThread:
         if (Thread->ThreadsProcess->ThreadListHead.Flink->Flink ==
	     &Thread->ThreadsProcess->ThreadListHead)
	   {
	     u.Last = TRUE;
	   }
         else
	   {
	     u.Last = FALSE;
	   }
         break;
       default:
	 /* Shoult never occure if the data table is correct */
	 KEBUGCHECK(0);
     }

   if (PreviousMode != KernelMode)
     {
       _SEH_TRY
         {
           if (QueryInformationData[ThreadInformationClass].Size)
             {
               RtlCopyMemory(ThreadInformation,
                             &u.TBI,
                             QueryInformationData[ThreadInformationClass].Size);
             }
           if (ReturnLength != NULL)
             {
               *ReturnLength = QueryInformationData[ThreadInformationClass].Size;
             }
         }
       _SEH_HANDLE
         {
           Status = _SEH_GetExceptionCode();
         }
       _SEH_END;
     }
   else
     {
       if (QueryInformationData[ThreadInformationClass].Size)
         {
           RtlCopyMemory(ThreadInformation,
                         &u.TBI,
                         QueryInformationData[ThreadInformationClass].Size);
         }

       if (ReturnLength != NULL)
         {
           *ReturnLength = QueryInformationData[ThreadInformationClass].Size;
         }
     }

   ObDereferenceObject(Thread);
   return(Status);
}
/* EOF */
