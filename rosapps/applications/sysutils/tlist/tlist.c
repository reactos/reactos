/*
 * ReactOS Project
 * TList
 *
 * Copyright (c) 2000,2001 Emanuele Aliberti
 */
#include <reactos/buildno.h>
#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <epsapi/epsapi.h>
#include <getopt.h>

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

void *PsaiMalloc(SIZE_T size)
{
 void * pBuf = NULL;
 NTSTATUS nErrCode;

 nErrCode = NtAllocateVirtualMemory
 (
  NtCurrentProcess(),
  &pBuf,
  0,
  (PULONG)&size,
  MEM_COMMIT,
  PAGE_READWRITE
 );

 if(NT_SUCCESS(nErrCode)) return pBuf;
 else return NULL;
}

void PsaiFree(void *ptr)
{
 ULONG nSize = 0;

 NtFreeVirtualMemory(NtCurrentProcess(), &ptr, &nSize, MEM_RELEASE);
}

int WINAPI PrintBanner (VOID)
{
  printf ("ReactOS "KERNEL_RELEASE_STR" T(ask)List\n");
  printf ("Copyright (c) 2000,2001 Emanuele Aliberti\n\n");
  return EXIT_SUCCESS;
}

int WINAPI PrintSynopsys (int nRetVal)
{
  PrintBanner ();
  printf ("Usage: tlist [-t | PID | -l]\n\n"
          "  -t   print the task list tree\n"
          "  PID  print module information for this ID\n"
          "  -l   print license information\n");
  return nRetVal;
}

int WINAPI PrintLicense (VOID)
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

BOOL WINAPI AcquirePrivileges (VOID)
{
  /* TODO: implement it */
  return TRUE;
}

int WINAPI
ProcessHasDescendants (
  HANDLE                      Pid,
  PSYSTEM_PROCESS_INFORMATION pInfo
  )
{
  LONG Count = 0;

  if (NULL == pInfo) return 0;
  do {

      if (ALREADY_PROCESSED != (DWORD)pInfo->InheritedFromUniqueProcessId)
      {
        if ((Pid != (HANDLE)pInfo->UniqueProcessId) && (Pid == (HANDLE)pInfo->InheritedFromUniqueProcessId))
        {
          ++ Count;
        }
      }
      pInfo = (PSYSTEM_PROCESS_INFORMATION)((PBYTE)pInfo + pInfo->NextEntryOffset);

  } while (0 != pInfo->NextEntryOffset);

  return Count;
}


BOOL WINAPI
GetProcessInfo (
  PSYSTEM_PROCESS_INFORMATION pInfo,
  LPWSTR                      * Module,
  LPWSTR                      * Title
  )
{
      *Module = (pInfo->ImageName.Length ? pInfo->ImageName.Buffer : L"System process");
      *Title = L""; /* TODO: check if the process has any window */
      return TRUE;
}

int WINAPI PrintProcessInfoDepth (
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
    pInfo->UniqueProcessId,
    pInfo->InheritedFromUniqueProcessId,
    Title
    );
  return EXIT_SUCCESS;
}

int WINAPI
PrintProcessAndDescendants (
  PSYSTEM_PROCESS_INFORMATION pInfo,
  PSYSTEM_PROCESS_INFORMATION pInfoBase,
  LONG                        Depth
  )
{
  HANDLE   Pid = 0;

  if (NULL == pInfo) return EXIT_FAILURE;
  /* Print current pInfo process */
  PrintProcessInfoDepth (pInfo, Depth ++);
  pInfo->InheritedFromUniqueProcessId = (HANDLE)ALREADY_PROCESSED;
  /* Save current process' PID */
  Pid = pInfo->UniqueProcessId;
  /* Scan and print possible children */
  do {

    if (ALREADY_PROCESSED != (DWORD)pInfo->InheritedFromUniqueProcessId)
    {
      if (Pid == pInfo->InheritedFromUniqueProcessId)
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
	  pInfo->InheritedFromUniqueProcessId = (HANDLE)ALREADY_PROCESSED;
	}
      }
    }
    pInfo = (PSYSTEM_PROCESS_INFORMATION)((PBYTE)pInfo + pInfo->NextEntryOffset);

  } while (0 != pInfo->NextEntryOffset);

  return EXIT_SUCCESS;
}

int WINAPI PrintProcessList (BOOL DisplayTree)
{
  PSYSTEM_PROCESS_INFORMATION pInfo = NULL;
  PSYSTEM_PROCESS_INFORMATION pInfoBase = NULL;
  LPWSTR                      Module = L"";
  LPWSTR                      Title = L"";

  if (!NT_SUCCESS(PsaCaptureProcessesAndThreads(&pInfoBase)))
   return EXIT_FAILURE;

  pInfo = PsaWalkFirstProcess(pInfoBase);

  while (pInfo)
  {
      if (FALSE == DisplayTree)
      {
        GetProcessInfo (pInfo, & Module, & Title);
        wprintf (
          L"%4d %-16s %s\n",
	  pInfo->UniqueProcessId,
	  Module,
	  Title,
	  pInfo->InheritedFromUniqueProcessId
	  );
      }
      else
      {
	if (ALREADY_PROCESSED != (DWORD)pInfo->InheritedFromUniqueProcessId)
	{
	  PrintProcessAndDescendants (pInfo, pInfoBase, 0);
	}
      }

      pInfo = PsaWalkNextProcess(pInfo);
    }

    PsaFreeCapture(pInfoBase);

    return EXIT_SUCCESS;
}


int WINAPI PrintThreads (PSYSTEM_PROCESS_INFORMATION pInfo)
{
  ULONG                               i = 0;
  NTSTATUS                            Status = STATUS_SUCCESS;
  HANDLE                              hThread = INVALID_HANDLE_VALUE;
  OBJECT_ATTRIBUTES                   Oa = {0};
  PVOID                               Win32StartAddress = NULL;
  THREAD_BASIC_INFORMATION            tInfo = {0};
  ULONG                               ReturnLength = 0;
  PSYSTEM_THREAD_INFORMATION          CurThread;

  if (NULL == pInfo) return EXIT_FAILURE;

  CurThread = PsaWalkFirstThread(pInfo);

  wprintf (L"   NumberOfThreads: %d\n", pInfo->NumberOfThreads);

  for (i = 0; i < pInfo->NumberOfThreads; i ++, CurThread = PsaWalkNextThread(CurThread))
  {
    Status = NtOpenThread (
	       & hThread,
	       THREAD_QUERY_INFORMATION,
	       & Oa,
	       & CurThread->ClientId
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
      CurThread->ClientId.UniqueThread,
      Win32StartAddress,
      0 /* FIXME: ((PTEB) tInfo.TebBaseAddress)->LastErrorValue */,
      ThreadStateName[CurThread->ThreadState]
      );
  }
  return EXIT_SUCCESS;
}

int WINAPI PrintModules (VOID)
{
	/* TODO */
	return EXIT_SUCCESS;
}

PSYSTEM_PROCESS_INFORMATION WINAPI
GetProcessInfoPid (
  PSYSTEM_PROCESS_INFORMATION pInfoBase,
  HANDLE                       Pid
  )
{
  if (NULL == pInfoBase) return NULL;

  pInfoBase = PsaWalkFirstProcess(pInfoBase);

  while(pInfoBase)
  {
    if (Pid == pInfoBase->UniqueProcessId)
    {
      return pInfoBase;
    }

    pInfoBase = PsaWalkNextProcess(pInfoBase);
  }

  return NULL;
}

int WINAPI PrintProcess (char * PidStr)
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

    if (!NT_SUCCESS(PsaCaptureProcessesAndThreads (&pInfoBase)))
     return EXIT_FAILURE;

    pInfo = GetProcessInfoPid (pInfoBase, ClientId.UniqueProcess);
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

    PsaFreeCapture(pInfoBase);

    NtClose (hProcess);

    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}


int main (int argc, char * argv [])
{
 int c;

 if(1 == argc) return PrintProcessList(FALSE);

 while((c = getopt(argc, argv, "tl")) != -1)
 {
  switch(c)
  {
   case 't': return PrintProcessList(TRUE);
   case 'l': return PrintLicense();
   default: return PrintSynopsys(EXIT_FAILURE);
  }
 }

 if(isdigit(argv[optind][0]))
  return PrintProcess (argv[1]);

 return PrintSynopsys(EXIT_SUCCESS);
}

/* EOF */
