/* $Id: tlist.c,v 1.1 2001/11/04 21:53:20 ea Exp $
 *
 * ReactOS Project
 * TList
 *
 * Copyright (c) 2000,2001 Emanuele Aliberti
 */
#include <reactos/buildno.h>
#define NTOS_MODE_USER
#include <ntos.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#define ALREADY_PROCESSED ((DWORD)-1)

LPWSTR ThreadStateName [] =
{
  L"Initialized",
  L"Ready",
  L"Running",
  L"Standby",
  L"Terminated",
  L"Wait",
  L"Transition",
  L"Unknown",
  NULL
};



int STDCALL PrintBanner (VOID)
{
  printf ("ReactOS "KERNEL_RELEASE_STR" T(ask)List\n");
  printf ("Copyright (c) 2000,2001 Emanuele Aliberti\n\n");
  return EXIT_SUCCESS;
}

int STDCALL PrintSynopsys (VOID)
{
  PrintBanner ();
  printf ("Usage: tlist [-t | PID | -l]\n\n"
          "  -t   print the task list tree\n"
          "  PID  print module information for this ID\n"
          "  -l   print license information\n");
  return EXIT_SUCCESS;
}

int STDCALL PrintLicense (VOID)
{
  PrintBanner ();
  printf (
"This program is free software; you can redistribute it and/or modify\n"
"it under the terms of the GNU General Public License as published by\n"
"the Free Software Foundation; either version 2 of the License, or\n"
"(at your option) any later version.\n\n");
  printf (
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n\n");
  printf (
"You should have received a copy of the GNU General Public License\n"
"along with this program; if not, write to the Free Software\n"
"Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n\n");
  return EXIT_SUCCESS;
}

BOOL STDCALL AcquirePrivileges (VOID)
{
  /* TODO: implement it */
  return TRUE;
}

PSYSTEM_PROCESS_INFORMATION STDCALL
GetProcessAndThreadsInfo (PULONG Size)
{
  NTSTATUS                    Status = STATUS_SUCCESS;
  PSYSTEM_PROCESS_INFORMATION pInfo = NULL;
  ULONG                       Length = PAGE_SIZE;
  ULONG                       RequiredLength = 0;

  while (TRUE)
  {
    Status = NtAllocateVirtualMemory (
               NtCurrentProcess(),
	       (PVOID) & pInfo,
	       0,
	       & Length,
	       (MEM_RESERVE | MEM_COMMIT),
	       PAGE_READWRITE
	       );
    if (!NT_SUCCESS(Status) || (NULL == pInfo))
    {
      fprintf (stderr, "%s(%d): Status = 0x%08lx\n",__FUNCTION__,__LINE__,Status);
      return NULL;
    }
    /*
     *	Obtain required buffer size (well, try to...)
     */
    if (NtQuerySystemInformation (
          SystemProcessesAndThreadsInformation,
          pInfo,
          Length,
	  & RequiredLength
	  ) != STATUS_INFO_LENGTH_MISMATCH)
    {
      break;
    }
    NtFreeVirtualMemory (NtCurrentProcess(), (PVOID)&pInfo, & Length, MEM_RELEASE);
    Length += PAGE_SIZE;
  }
  if (!NT_SUCCESS(Status))
  {
    NtFreeVirtualMemory (NtCurrentProcess(), (PVOID)&pInfo, & Length, MEM_RELEASE);
    return NULL;
  }
  if (NULL != Size)
  {
    *Size = Length;
  }
  return pInfo;
}

int STDCALL
ProcessHasDescendants (
  ULONG                       Pid,
  PSYSTEM_PROCESS_INFORMATION pInfo
  )
{
  LONG Count = 0;

  if (NULL == pInfo) return 0;
  do {

      if (ALREADY_PROCESSED != pInfo->ParentProcessId)
      {
        if ((Pid != pInfo->ProcessId) && (Pid == pInfo->ParentProcessId))
        {
          ++ Count;
        }
      }
      (PBYTE) pInfo += pInfo->RelativeOffset;

  } while (0 != pInfo->RelativeOffset);

  return Count;
}


BOOL STDCALL
GetProcessInfo (
  PSYSTEM_PROCESS_INFORMATION pInfo,
  LPWSTR                      * Module,
  LPWSTR                      * Title
  )
{
      *Module = (pInfo->Name.Length ? pInfo->Name.Buffer : L"System process");
      *Title = L""; /* TODO: check if the process has any window */
      return TRUE;
}

int STDCALL PrintProcessInfoDepth (
  PSYSTEM_PROCESS_INFORMATION pInfo,
  LONG                        Depth
  )
{
  INT     d = 0;
  LPWSTR  Module = L"";
  LPWSTR  Title = L"";
  
  for (d = 0; d < Depth; d ++) printf ("  ");
  GetProcessInfo (pInfo, & Module, & Title);
  wprintf (
    L"%s (%d, %d) %s\n",
    Module,
    pInfo->ProcessId,
    pInfo->ParentProcessId,
    Title
    );
  return EXIT_SUCCESS;
}

int STDCALL
PrintProcessAndDescendants (
  PSYSTEM_PROCESS_INFORMATION pInfo,
  PSYSTEM_PROCESS_INFORMATION pInfoBase,
  LONG                        Depth
  )
{
  DWORD   Pid = 0;

  if (NULL == pInfo) return EXIT_FAILURE;
  /* Print current pInfo process */
  PrintProcessInfoDepth (pInfo, Depth ++);
  pInfo->ParentProcessId = ALREADY_PROCESSED;
  /* Save current process' PID */
  Pid = pInfo->ProcessId;
  /* Scan and print possible children */
  do {

    if (ALREADY_PROCESSED != pInfo->ParentProcessId)
    {
      if (Pid == pInfo->ParentProcessId)
      {
        if (ProcessHasDescendants (Pid, pInfoBase))
        {
          PrintProcessAndDescendants (
            pInfo,
            pInfoBase,
            Depth
            );
        }
	else
	{
          PrintProcessInfoDepth (pInfo, Depth);
	  pInfo->ParentProcessId = ALREADY_PROCESSED;
	}
      }
    }
    (PBYTE) pInfo += pInfo->RelativeOffset;

  } while (0 != pInfo->RelativeOffset);
  
  return EXIT_SUCCESS;
}

int STDCALL PrintProcessList (BOOL DisplayTree)
{
  PSYSTEM_PROCESS_INFORMATION pInfo = NULL;
  PSYSTEM_PROCESS_INFORMATION pInfoBase = NULL;
  LONG                        Length = 0;
  LPWSTR                      Module = L"";
  LPWSTR                      Title = L"";

  
  pInfo = GetProcessAndThreadsInfo (& Length);
  if (NULL == pInfo) return EXIT_FAILURE;
  pInfoBase = pInfo;
  do {
      if (FALSE == DisplayTree)
      {
        GetProcessInfo (pInfo, & Module, & Title);
        wprintf (
          L"%4d %-16s %s\n",
	  pInfo->ProcessId,
	  Module,
	  Title,
	  pInfo->ParentProcessId
	  );
      }
      else 
      {
	if (ALREADY_PROCESSED != pInfo->ParentProcessId)
	{
	  PrintProcessAndDescendants (pInfo, pInfoBase, 0);
	}
      }
      (PBYTE) pInfo += pInfo->RelativeOffset;

    } while (0 != pInfo->RelativeOffset);
  
    NtFreeVirtualMemory (
      NtCurrentProcess(),
      (PVOID) & pInfoBase,
      & Length,
      MEM_RELEASE
      );
  
    return EXIT_SUCCESS;
}


int STDCALL PrintThreads (PSYSTEM_PROCESS_INFORMATION pInfo)
{
  ULONG                    ThreadIndex = 0;
  NTSTATUS                 Status = STATUS_SUCCESS;
  HANDLE                   hThread = INVALID_HANDLE_VALUE;
  OBJECT_ATTRIBUTES        Oa = {0};
  PVOID                    Win32StartAddress = NULL;
  THREAD_BASIC_INFORMATION tInfo = {0};
  ULONG                    ReturnLength = 0;

  if (NULL == pInfo) return EXIT_FAILURE;

  wprintf (L"   NumberOfThreads: %d\n", pInfo->ThreadCount);

  for (ThreadIndex = 0; ThreadIndex < pInfo->ThreadCount; ThreadIndex ++)
  {
    Status = NtOpenThread (
	       & hThread,
	       THREAD_QUERY_INFORMATION,
	       & Oa,
	       & pInfo->ThreadSysInfo[ThreadIndex].ClientId
               );
    if (!NT_SUCCESS(Status))
    {
      continue;
    }
    
    Status = NtQueryInformationThread (
               hThread,
	       ThreadBasicInformation,
	       (PVOID) & tInfo,
	       sizeof tInfo,
	       & ReturnLength
               );
    if (!NT_SUCCESS(Status))
    {
      NtClose (hThread);
      continue;
    }

    Status = NtQueryInformationThread (
               hThread,
	       ThreadQuerySetWin32StartAddress,
	       (PVOID) & Win32StartAddress,
	       sizeof Win32StartAddress,
	       & ReturnLength
               );
    if (!NT_SUCCESS(Status))
    {
      NtClose (hThread);
      continue;
    }
    
    NtClose (hThread);

    /* Now print the collected information */
    wprintf (L"   %4d Win32StartAddr:0x%08x LastErr:0x%08x State:%s\n",
      pInfo->ThreadSysInfo[ThreadIndex].ClientId.UniqueThread,
      Win32StartAddress,
      0 /* FIXME: ((PTEB) tInfo.TebBaseAddress)->LastErrorValue */,
      ThreadStateName[pInfo->ThreadSysInfo[ThreadIndex].State]
      );
  } 
  return EXIT_SUCCESS;
}

int STDCALL PrintModules (VOID)
{
	/* TODO */
	return EXIT_SUCCESS;
}

PSYSTEM_PROCESS_INFORMATION STDCALL
GetProcessInfoPid (
  PSYSTEM_PROCESS_INFORMATION pInfoBase,
  DWORD                       Pid
  )
{
  if (NULL == pInfoBase) return NULL;
  do {

    if (Pid == pInfoBase->ProcessId)
    {
      return pInfoBase;
    }
    (PBYTE) pInfoBase += pInfoBase->RelativeOffset;

  } while (0 != pInfoBase->RelativeOffset);

  return NULL;
}

int STDCALL PrintProcess (char * PidStr)
{
  NTSTATUS                    Status = 0;
  HANDLE                      hProcess = 0;
  OBJECT_ATTRIBUTES           Oa = {0};
  CLIENT_ID                   ClientId = {0, 0};

  
  ClientId.UniqueProcess = (PVOID) atol (PidStr);
 
  if (FALSE == AcquirePrivileges ())
  {
    return EXIT_FAILURE;
  }
  
  Status = NtOpenProcess (
             & hProcess,
             PROCESS_QUERY_INFORMATION,
             & Oa,
             & ClientId
             );
  if (NT_SUCCESS(Status))
  {
    ULONG                       ReturnLength = 0;
    PROCESS_BASIC_INFORMATION   PsBasic;
    VM_COUNTERS                 PsVm;
    PSYSTEM_PROCESS_INFORMATION pInfo = NULL;
    PSYSTEM_PROCESS_INFORMATION pInfoBase = NULL;
    LONG                        pInfoBaseLength = 0;
    LPWSTR                      Module = L"";
    LPWSTR                      Title = L"";
    
    Status = NtQueryInformationProcess (
               hProcess,
	       ProcessBasicInformation,
	       & PsBasic,
	       sizeof (PsBasic),
	       & ReturnLength
               );
    if (!NT_SUCCESS(Status))
    {
      return EXIT_FAILURE;
    }
    Status = NtQueryInformationProcess (
               hProcess,
	       ProcessVmCounters,
	       & PsVm,
	       sizeof (PsVm),
	       & ReturnLength
               );
    if (!NT_SUCCESS(Status))
    {
      return EXIT_FAILURE;
    }

    pInfoBase = GetProcessAndThreadsInfo (& pInfoBaseLength);
    if (NULL == pInfoBase) return EXIT_FAILURE;

    pInfo = GetProcessInfoPid (pInfoBase, (DWORD) ClientId.UniqueProcess);
    if (NULL == pInfo) return EXIT_FAILURE;

    GetProcessInfo (pInfo, & Module, & Title);
    
    wprintf (L"%4d %s\n", ClientId.UniqueProcess, Module);
#if 0
    printf ("   CWD:     %s\n", ""); /* it won't appear if empty */
    printf ("   CmdLine: %s\n", ""); /* it won't appear if empty */
#endif
    printf ("   VirtualSize:     %5ld kb   PeakVirtualSize:     %5ld kb\n",
      ((LONG) PsVm.VirtualSize / 1024),
      ((LONG) PsVm.PeakVirtualSize / 1024)
      );
    printf ("   WorkingSetSize:  %5ld kb   PeakWorkingSetSize:  %5ld kb\n",
      ((LONG) PsVm.WorkingSetSize / 1024),
      ((LONG) PsVm.PeakWorkingSetSize / 1024)
      );
    
    PrintThreads (pInfo);

    PrintModules ();
    
    NtFreeVirtualMemory (
      NtCurrentProcess(),
      (PVOID) & pInfoBase,
      & pInfoBaseLength,
      MEM_RELEASE
      );
  
    NtClose (hProcess);
    
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}


int main (int argc, char * argv [])
{
  if (1 == argc)
  {
    return PrintProcessList (FALSE);
  }
  if (2 == argc)
  {
    if (('-' == argv [1][0]) && ('\0' == argv [1][2]))
    {
      if ('t' == argv [1][1])
      {
        return PrintProcessList (TRUE);
      }
      if ('l' == argv [1][1])
      {
        return PrintLicense ();
      }
    }
    if (isdigit(argv[1][0]))
    {
      return PrintProcess (argv[1]);
    }
  }
  return PrintSynopsys ();
}

/* EOF */
