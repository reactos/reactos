/* $Id: spawn.c,v 1.9 2002/12/26 18:14:36 robd Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/misc/spawn.c
 * PURPOSE:     Create the first POSIX+ process
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              25/02/2002: Created
 */

/*
 * NOTE by KJK::Hyperion:
 *    The __PdxSpawnPosixProcess() call solves the chicken-egg dilemma of
 * creating the first POSIX+ process in a group without the ability to
 * fork+exec (for example from inside a Win32 process). Processes created by
 * __PdxSpawnPosixProcess() will *not* inherit anything from the parent, not
 * even handles: all creation parameters have to be specified explicitely
 */

#include <ddk/ntddk.h>
#include <ntdll/base.h>
#include <napi/i386/segment.h>
#include <ntdll/rtl.h>
#include <ntdll/ldr.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <psx/debug.h>
#include <psx/pdata.h>
#include <psx/spawn.h>
#include <psx/stdlib.h>

#include <windows.h>

typedef struct _PORT_MESSAGE {
 USHORT DataSize;
 USHORT MessageSize;
 USHORT MessageType;
 USHORT VirtualRangesOffset;
 CLIENT_ID ClientId;
 ULONG MessageId;
 ULONG SectionSize;
 // UCHAR Data[];
} PORT_MESSAGE, *PPORT_MESSAGE;

NTSTATUS STDCALL CsrClientCallServer(
 IN PVOID Message,
 IN PVOID Unknown,
 IN ULONG Opcode,
 IN ULONG Size
);

NTSTATUS STDCALL __PdxSpawnPosixProcess
(
 OUT PHANDLE ProcessHandle,
 OUT PHANDLE ThreadHandle,
 IN POBJECT_ATTRIBUTES FileObjectAttributes,
 IN POBJECT_ATTRIBUTES ProcessObjectAttributes,
 IN HANDLE InheritFromProcessHandle,
 IN __PPDX_PDATA ProcessData
)
{
 struct CSRSS_MESSAGE {
  ULONG Unknown1;
  ULONG Opcode;
  ULONG Status;
  ULONG Unknown2;
 };

 struct __tagcsrmsg{
  PORT_MESSAGE PortMessage;
  struct CSRSS_MESSAGE CsrssMessage;
  PROCESS_INFORMATION ProcessInformation;
  CLIENT_ID Debugger;
  ULONG CreationFlags;
  ULONG VdmInfo[2];
 } csrmsg;

 __PPDX_SERIALIZED_PDATA      pspdProcessData;
 IO_STATUS_BLOCK              isbStatus;
 PROCESS_BASIC_INFORMATION    pbiProcessInfo;
 INITIAL_TEB                  itInitialTeb;
 PRTL_USER_PROCESS_PARAMETERS pppProcessParameters;
 SECTION_IMAGE_INFORMATION    siiInfo;
 CONTEXT    ctxThreadContext;
 CLIENT_ID  ciClientId;
 NTSTATUS nErrCode;
 HANDLE   hExeFile;
 HANDLE   hExeImage;
 HANDLE   hProcess;
 PVOID    pPdataBuffer = 0;
 PVOID    pParamsBuffer = 0;
 ULONG    nDestBufferSize;
 ULONG    nCurFilDesOffset;
 ULONG    nVirtualSize;
 ULONG    nCommitSize;
 PVOID    pCommitBottom;
 ULONG    nOldProtect;
 int      i;

 /* STEP 1: map executable image in memory */
 /* 1.1: open the file for execution */
 nErrCode = NtOpenFile
 (
  &hExeFile,
  SYNCHRONIZE | FILE_EXECUTE,
  FileObjectAttributes,
  &isbStatus,
  FILE_SHARE_READ | FILE_SHARE_DELETE,
  FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtOpenFile() failed with status 0x%08X\n", nErrCode);
  return (nErrCode);
 }

 /* 1.2: create an image section for the file */
 nErrCode = NtCreateSection
 (
  &hExeImage,
  SECTION_ALL_ACCESS,
  NULL,
  0,
  PAGE_EXECUTE,
  SEC_IMAGE,
  hExeFile
 );

 /* close file handle (not needed anymore) */
 NtClose(hExeFile);

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtCreateSection() failed with status 0x%08X\n", nErrCode);
  return (nErrCode);
 }

 /* 1.3: get section image information */
 nErrCode = NtQuerySection
 (
  hExeImage,
  SectionImageInformation,
  &siiInfo,
  sizeof(siiInfo),
  NULL
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtCreateSection() failed with status 0x%08X\n", nErrCode);
  NtClose(hExeImage);
  return (nErrCode);
 }

 /* STEP 2: create process */
 nErrCode = NtCreateProcess
 (
  &hProcess,
  PROCESS_ALL_ACCESS,
  ProcessObjectAttributes,
  InheritFromProcessHandle,
  FALSE,
  hExeImage,
  NULL,
  NULL
 );
 
 /* close image handle (not needed anymore) */
 NtClose(hExeImage);

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtCreateProcess() failed with status 0x%08X\n", nErrCode);
  return (nErrCode);
 }

 /* STEP 3: write process environment and process parameters */
 /* 3.1: convert process data into process parameters */
 __PdxProcessDataToProcessParameters
 (
  &pppProcessParameters,
  ProcessData,
  FileObjectAttributes->ObjectName
 );

 /* 3.2: serialize the process data for transfer */
 /* FIXME: the serialized data can be allocated and written directly in the
    destination process */
 __PdxSerializeProcessData(ProcessData, &pspdProcessData);

 /* 3.2.1: adjust some fields */
 pspdProcessData->ProcessData.Spawned = TRUE;

 /* 3.3: allocate memory in the destination process */
 /* 3.3.1: process data */
 nDestBufferSize = pspdProcessData->AllocSize;

 nErrCode = NtAllocateVirtualMemory
 (
  hProcess,
  &pPdataBuffer,
  0,
  &nDestBufferSize,
  MEM_COMMIT,
  PAGE_READWRITE
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtAllocateVirtualMemory() failed with status 0x%08X\n", nErrCode);
  goto undoPData;
 }

 /* 3.3.2: process parameters */
 nDestBufferSize = pppProcessParameters->Size;

 nErrCode = NtAllocateVirtualMemory
 (
  hProcess,
  &pParamsBuffer,
  0,
  &nDestBufferSize,
  MEM_COMMIT,
  PAGE_READWRITE
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtAllocateVirtualMemory() failed with status 0x%08X\n", nErrCode);
  goto undoPData;
 }

 /* 3.4: get pointer to the PEB */
 nErrCode = NtQueryInformationProcess
 (
  hProcess,
  ProcessBasicInformation,
  &pbiProcessInfo,
  sizeof(pbiProcessInfo),
  NULL
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtQueryInformationProcess() failed with status 0x%08X\n", nErrCode);
  goto undoPData;
 }

 /* 3.5: write pointers in the PEB */
 /* 3.5.1: process data */
 nErrCode = NtWriteVirtualMemory
 (
  hProcess,
  (PVOID)((ULONG)pbiProcessInfo.PebBaseAddress + offsetof(PEB, SubSystemData)),
  &pPdataBuffer,
  sizeof(PVOID),
  NULL
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtWriteVirtualMemory() failed with status 0x%08X\n", nErrCode);
  goto undoPData;
 }

 /* 3.5.2: process parameters */
 nErrCode = NtWriteVirtualMemory
 (
  hProcess,
  (PVOID)((ULONG)pbiProcessInfo.PebBaseAddress + offsetof(PEB, ProcessParameters)),
  &pParamsBuffer,
  sizeof(PVOID),
  NULL
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtWriteVirtualMemory() failed with status 0x%08X\n", nErrCode);
  goto undoPData;
 }

 /* 3.6: write data */
 /* 3.6.1: process data */
 nErrCode = NtWriteVirtualMemory
 (
  hProcess,
  pPdataBuffer,
  pspdProcessData,
  pspdProcessData->AllocSize,
  NULL
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtWriteVirtualMemory() failed with status 0x%08X\n", nErrCode);
  goto undoPData;
 }

 /* 3.6.2 process parameters */
 nErrCode = NtWriteVirtualMemory
 (
  hProcess,
  pParamsBuffer,
  pppProcessParameters,
  pppProcessParameters->Size,
  NULL
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtWriteVirtualMemory() failed with status 0x%08X\n", nErrCode);
  goto undoPData;
 }

undoPData:
  /* deallocate the temporary data block in the current process */
  NtFreeVirtualMemory
  (
   NtCurrentProcess(),
   (PVOID *)&pspdProcessData,
   0,
   MEM_RELEASE
  );

 /* destroy process parameters */
 RtlDestroyProcessParameters(pppProcessParameters);

 /* failure */
 if(!NT_SUCCESS(nErrCode))
  goto failProcess;

 /* STEP 4: duplicate handles */
 /* 4.1: handles in the structure itself */
 /* 4.1.1: root directory */
 nErrCode = NtDuplicateObject
 (
  NtCurrentProcess(),
  ProcessData->RootHandle,
  hProcess,
  (PHANDLE)((ULONG)pPdataBuffer + offsetof(__PDX_PDATA, RootHandle)),
  0,
  0,
  DUPLICATE_SAME_ACCESS | 4 /* | DUPLICATE_SAME_ATTRIBUTES */ /* FIXME */
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtDuplicateObject() failed with status 0x%08X\n", nErrCode);
  goto failProcess;
 }

 /* 4.2: file descriptors table */
 for
 (
  /* pspdProcessData->ProcessData.FdTable.Descriptors contains the offset to
     the descriptors array inside pspdProcessData->Buffer[], that is to the
     first element of the array */
  i = 0,
   nCurFilDesOffset = (ULONG)pspdProcessData->ProcessData.FdTable.Descriptors;
  /* iterate through all allocated descriptors */
  i < ProcessData->FdTable.AllocatedDescriptors;
  /* at every step, go on to next input descriptor, and increase the offset to
     the next output descriptor */
  i ++, nCurFilDesOffset += sizeof(__fildes_t)
 )
  /* FIXME? check the table's bitmap instead? */
  if(ProcessData->FdTable.Descriptors[i].FileHandle != NULL)
  {
   /* duplicate the source handle, ProcessData->FdTable.Descriptors[i], from
      the current process into the process identified by hProcess, at an
      address calculated by adding to the serialized data block base address:
       - the offset to the current descriptor
       - the offset to the handle field of the descriptor */
   nErrCode = NtDuplicateObject
   (
    NtCurrentProcess(),
    ProcessData->FdTable.Descriptors[i].FileHandle,
    hProcess,
    (PHANDLE)(
     (ULONG)pPdataBuffer + nCurFilDesOffset + offsetof(__fildes_t, FileHandle)
    ),
    0,
    0,
    DUPLICATE_SAME_ACCESS | 4 /* | DUPLICATE_SAME_ATTRIBUTES */ /* FIXME */
   );  

   /* failure */
   if(!NT_SUCCESS(nErrCode))
   {
    ERR("NtDuplicateObject() failed with status 0x%08X\n", nErrCode);
    goto failProcess;
   }

   /* duplicate standard handles */
   /* standard input */
   if(i == STDIN_FILENO)
   {
    nErrCode = NtDuplicateObject
    (
     NtCurrentProcess(),
     ProcessData->FdTable.Descriptors[i].FileHandle,
     hProcess,
     (PHANDLE)((ULONG)pParamsBuffer + offsetof(RTL_USER_PROCESS_PARAMETERS, hStdInput)),
     0,
     0,
     DUPLICATE_SAME_ACCESS | 4 /* | DUPLICATE_SAME_ATTRIBUTES */ /* FIXME */
    );  
 
    /* failure */
    if(!NT_SUCCESS(nErrCode))
    {
     ERR("NtDuplicateObject() failed with status 0x%08X\n", nErrCode);
     goto failProcess;
    }
   }
   /* standard output */
   else if(i == STDOUT_FILENO)
   {
    nErrCode = NtDuplicateObject
    (
     NtCurrentProcess(),
     ProcessData->FdTable.Descriptors[i].FileHandle,
     hProcess,
     (PHANDLE)((ULONG)pParamsBuffer + offsetof(RTL_USER_PROCESS_PARAMETERS, hStdOutput)),
     0,
     0,
     DUPLICATE_SAME_ACCESS | 4 /* | DUPLICATE_SAME_ATTRIBUTES */ /* FIXME */
    );  
 
    /* failure */
    if(!NT_SUCCESS(nErrCode))
    {
     ERR("NtDuplicateObject() failed with status 0x%08X\n", nErrCode);
     goto failProcess;
    }
   }
   /* standard error */
   else if(i == STDERR_FILENO)
   {
    nErrCode = NtDuplicateObject
    (
     NtCurrentProcess(),
     ProcessData->FdTable.Descriptors[i].FileHandle,
     hProcess,
     (PHANDLE)((ULONG)pParamsBuffer + offsetof(RTL_USER_PROCESS_PARAMETERS, hStdError)),
     0,
     0,
     DUPLICATE_SAME_ACCESS | 4 /* | DUPLICATE_SAME_ATTRIBUTES */ /* FIXME */
    );  
 
    /* failure */
    if(!NT_SUCCESS(nErrCode))
    {
     ERR("NtDuplicateObject() failed with status 0x%08X\n", nErrCode);
     goto failProcess;
    }
   }
  }

 /* STEP 5: create first thread */
 /* 5.1: set up the stack */
 itInitialTeb.StackAllocate = NULL;
 nVirtualSize = 0x100000;
 nCommitSize = 0x100000 - PAGE_SIZE;

 /* 5.1.1: reserve the stack */
 nErrCode = NtAllocateVirtualMemory
 (
  hProcess,
  &itInitialTeb.StackAllocate,
  0,
  &nVirtualSize,
  MEM_RESERVE,
  PAGE_READWRITE
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtAllocateVirtualMemory() failed with status 0x%08X\n", nErrCode);
  goto failProcess;
 }

 itInitialTeb.StackBase =
  (PVOID)((ULONG)itInitialTeb.StackAllocate + nVirtualSize);

 itInitialTeb.StackLimit =
  (PVOID)((ULONG)itInitialTeb.StackBase - nCommitSize);

 /* 5.1.2: commit the stack */
 nVirtualSize = nCommitSize + PAGE_SIZE;
 pCommitBottom =
  (PVOID)((ULONG)itInitialTeb.StackBase - nVirtualSize);

 nErrCode = NtAllocateVirtualMemory
 (
  hProcess,
  &pCommitBottom,
  0,
  &nVirtualSize,
  MEM_COMMIT,
  PAGE_READWRITE
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtAllocateVirtualMemory() failed with status 0x%08X\n", nErrCode);
  goto failProcess;
 }

 /* 5.1.3: set up the guard page */
 nVirtualSize = PAGE_SIZE;

 nErrCode = NtProtectVirtualMemory
 (
  hProcess,
  &pCommitBottom,
  &nVirtualSize,
  PAGE_GUARD | PAGE_READWRITE,
  &nOldProtect
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtProtectVirtualMemory() failed with status 0x%08X\n", nErrCode);
  goto failProcess;
 }

 /* 5.2: initialize the thread context */
 memset(&ctxThreadContext, 0, sizeof(ctxThreadContext));

 ctxThreadContext.Eip = (ULONG)siiInfo.EntryPoint;
 ctxThreadContext.SegGs = USER_DS;
 ctxThreadContext.SegFs = USER_DS;
 ctxThreadContext.SegEs = USER_DS;
 ctxThreadContext.SegDs = USER_DS;
 ctxThreadContext.SegCs = USER_CS;
 ctxThreadContext.SegSs = USER_DS;
 ctxThreadContext.Esp = (ULONG)itInitialTeb.StackBase - 4;
 ctxThreadContext.EFlags = (1 << 1) + (1 << 9);

 /* 5.3: create the thread object */
 nErrCode = NtCreateThread
 (
  ThreadHandle,
  THREAD_ALL_ACCESS,
  NULL,
  hProcess,
  &ciClientId,
  &ctxThreadContext,
  &itInitialTeb,
  TRUE /* FIXME: the thread is only created in suspended state for easier
          debugging. This behavior is subject to future changes */
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtCreateThread() failed with status 0x%08X\n", nErrCode);
  goto failProcess;
 }

 /* 6: register the process with the Win32 subsystem (temporary code for
    debugging purposes) */

 memset(&csrmsg, 0, sizeof(csrmsg));

 //csrmsg.PortMessage = {0};
 //csrmsg.CsrssMessage = {0};
 csrmsg.ProcessInformation.hProcess = hProcess;
 csrmsg.ProcessInformation.hThread = *ThreadHandle;
 csrmsg.ProcessInformation.dwProcessId = (DWORD)ciClientId.UniqueProcess;
 csrmsg.ProcessInformation.dwThreadId = (DWORD)ciClientId.UniqueThread;
 //csrmsg.Debugger = {0};
 //csrmsg.CreationFlags = 0;
 //csrmsg.VdmInfo = {0};

 nErrCode = CsrClientCallServer(&csrmsg, 0, 0x10000, 0x24);

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("CsrClientCallServer() failed with status 0x%08X\n", nErrCode);
  goto failProcess;
 }

 nErrCode = NtResumeThread(*ThreadHandle, NULL);

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtResumeThread() failed with status 0x%08X\n", nErrCode);
  goto failProcess;
 }

 /* success */
 return (STATUS_SUCCESS);

 /* failure */
failProcess:
 NtTerminateProcess
 (
  hProcess,
  nErrCode
 );

 return (nErrCode);
}

/* EOF */

