/* $Id: spawn.c,v 1.1 2002/03/07 05:46:03 hyperion Exp $
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
#include <stddef.h>
#include <string.h>
#include <inttypes.h>
#include <pthread.h>
#include <psx/debug.h>
#include <psx/pdata.h>
#include <psx/stdlib.h>

VOID
__PdxSerializeProcessData
(
 IN __PPDX_PDATA ProcessData,
 OUT __PPDX_SERIALIZED_PDATA *SerializedProcessData
)
{
 __PPDX_SERIALIZED_PDATA pspdProcessData = 0;
 NTSTATUS  nErrCode;
 PBYTE     pBufferTail;
 ULONG     ulAllocSize = sizeof(__PDX_SERIALIZED_PDATA) - 1;
 size_t   *pnArgLengths;
 size_t   *pnEnvVarsLengths;
 int      nEnvVarsCount;
 int      i;

 /* calculate buffer length */
 /* FIXME please! this is the most inefficient way to do it */

 /* argv */
 pnArgLengths = __malloc(ProcessData->ArgCount * sizeof(size_t));

 for(i = 0; i < ProcessData->ArgCount; i ++)
 {
  int nStrLen = strlen(ProcessData->ArgVect[i]) + 1;
  ulAllocSize += nStrLen;
  pnArgLengths[i] = nStrLen;

  INFO
  (
   "argument %d: \"%s\", length %d\n",
   i,
   ProcessData->ArgVect[i],
   nStrLen
  );
 }

 /* environ */
 pnEnvVarsLengths = 0;
 nEnvVarsCount = 0;

 for(i = 0; *(ProcessData->Environment)[i] != 0; i++)
 {
  int nStrLen = strlen(*(ProcessData->Environment)[i]) + 1;
  ulAllocSize += nStrLen;

  nEnvVarsCount ++;
  __realloc(pnEnvVarsLengths, nEnvVarsCount * sizeof(size_t));
  pnEnvVarsLengths[i] = nStrLen;

  INFO
  (
   "environment variable %d: \"%s\", length %d\n",
   i,
   *(ProcessData->Environment)[i],
   nStrLen
  );
 }

 INFO("(%d environment variables were found)\n", nEnvVarsCount);

 /* current directory */
 ulAllocSize += ProcessData->CurDir.Length;
 INFO
 (
  "current directory: \"%Z\", length %d\n",
  &ProcessData->CurDir,
  ProcessData->CurDir.Length
 );

 /* root directory */
 ulAllocSize += ProcessData->RootPath.Length;
 INFO
 (
  "root directory: \"%Z\", length %d\n",
  &ProcessData->RootPath,
  ProcessData->RootPath.Length
 );

 /* file descriptors table */
 ulAllocSize += sizeof(__fildes_t) * ProcessData->FdTable.AllocatedDescriptors;
 INFO
 (
  "descriptors table contains %d allocated descriptors, combined length %d\n",
  ProcessData->FdTable.AllocatedDescriptors,
  sizeof(__fildes_t) * ProcessData->FdTable.AllocatedDescriptors
 );

 /* extra descriptors data */
 for(i = 0; ProcessData->FdTable.AllocatedDescriptors; i ++)
  if(ProcessData->FdTable.Descriptors[i].ExtraData != NULL)
  {
   ulAllocSize += ProcessData->FdTable.Descriptors[i].ExtraDataSize;

   INFO
   (
    "descriptor %d has %d bytes of associated data\n",
    i,
    ProcessData->FdTable.Descriptors[i].ExtraDataSize
   );

  }

 /* allocate return block */
 INFO("about to allocate %d bytes\n", ulAllocSize);

 nErrCode = NtAllocateVirtualMemory
 (
  NtCurrentProcess(),
  (PVOID *)&pspdProcessData,
  0,
  &ulAllocSize,
  MEM_COMMIT,
  PAGE_READWRITE
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtAllocateVirtualMemory() failed with status 0x%08X\n", nErrCode);
  __free(pnArgLengths);
  __free(pnEnvVarsLengths);
  *SerializedProcessData = 0;
  return;
 }

 INFO("%d bytes actually allocated\n", ulAllocSize);
 pspdProcessData->AllocSize = ulAllocSize;

 /* copy data */
 /* static data */
 memcpy(&pspdProcessData->ProcessData, ProcessData, sizeof(__PDX_PDATA));

 /* buffers */
 pBufferTail = &pspdProcessData->Buffer[0];
 INFO("buffer tail begins at 0x%08X\n", pBufferTail);

 /* argv */
 pspdProcessData->ProcessData.ArgVect = 0;

 for(i = 0; i < ProcessData->ArgCount; i ++)
 {
  INFO
  (
   "copying %d bytes of argument %d (\"%s\") to 0x%08X\n",
   pnArgLengths[i], 
   i,
   ProcessData->ArgVect[i],
   pBufferTail
  );

  strncpy(pBufferTail, ProcessData->ArgVect[i], pnArgLengths[i]);
  pBufferTail += pnArgLengths[i];

  INFO
  (
   "buffer tail increased by %d bytes, new tail at 0x%08X\n",
   pnArgLengths[i],
   pBufferTail
  );

 }

 __free(pnArgLengths);

 /* environ */
 pspdProcessData->ProcessData.Environment = (char ***)nEnvVarsCount;

 for(i = 0; i < nEnvVarsCount; i ++)
 {
  INFO
  (
   "copying %d bytes of environment variable %d (\"%s\") to 0x%08X\n",
   pnEnvVarsLengths[i], 
   i,
   ProcessData->Environment[i],
   pBufferTail
  );

  strncpy(pBufferTail, *ProcessData->Environment[i], pnEnvVarsLengths[i]);
  pBufferTail += pnEnvVarsLengths[i];

  INFO
  (
   "buffer tail increased by %d bytes, new tail at 0x%08X\n",
   pnEnvVarsLengths[i],
   pBufferTail
  );
 }

 __free(pnEnvVarsLengths);

 /* current directory */
 INFO
 (
  "copying %d bytes of current directory (\"%Z\") to 0x%08X\n",
  ProcessData->CurDir.Length,
  &ProcessData->CurDir,
  pBufferTail
 );

 memcpy(pBufferTail, ProcessData->CurDir.Buffer, ProcessData->CurDir.Length);
 pBufferTail += ProcessData->CurDir.Length;

 INFO
 (
  "buffer tail increased by %d bytes, new tail at 0x%08X\n",
  ProcessData->CurDir.Length,
  pBufferTail
 );

 /* root directory */
 INFO
 (
  "copying %d bytes of root directory (\"%Z\") to 0x%08X\n",
  ProcessData->RootPath.Length,
  &ProcessData->RootPath,
  pBufferTail
 );

 memcpy
 (
  pBufferTail,
  ProcessData->RootPath.Buffer,
  ProcessData->RootPath.Length
 );

 pBufferTail += ProcessData->RootPath.Length;

 INFO
 (
  "buffer tail increased by %d bytes, new tail at 0x%08X\n",
  ProcessData->RootPath.Length,
  pBufferTail
 );

 /* file descriptors table */
 /* save the offset to the descriptors array */
 pspdProcessData->ProcessData.FdTable.Descriptors =
  (PVOID)((ULONG)pBufferTail - (ULONG)pspdProcessData);

 INFO
 (
  "descriptors table contains %d allocated descriptors, combined length %d\n",
  ProcessData->FdTable.AllocatedDescriptors,
  sizeof(__fildes_t) * ProcessData->FdTable.AllocatedDescriptors
 );

 memcpy
 (
  pBufferTail,
  ProcessData->FdTable.Descriptors,
  sizeof(__fildes_t) * ProcessData->FdTable.AllocatedDescriptors
 );

 pBufferTail +=
  sizeof(__fildes_t) * ProcessData->FdTable.AllocatedDescriptors;

 INFO
 (
  "buffer tail increased by %d bytes, new tail at 0x%08X\n",
  sizeof(__fildes_t) * ProcessData->FdTable.AllocatedDescriptors,
  pBufferTail
 );

 /* extra descriptors data */
 for(i = 0; ProcessData->FdTable.AllocatedDescriptors; i ++)
  if(ProcessData->FdTable.Descriptors[i].ExtraData != 0)
  {
   INFO
   (
    "descriptor %d has %d bytes of associated data\n",
    i,
    ProcessData->FdTable.Descriptors[i].ExtraDataSize
   );

   memcpy
   (
    pBufferTail,
    ProcessData->FdTable.Descriptors[i].ExtraData,
    ProcessData->FdTable.Descriptors[i].ExtraDataSize
   );

   pBufferTail += ProcessData->FdTable.Descriptors[i].ExtraDataSize;

   INFO
   (
    "buffer tail increased by %d bytes, new tail at 0x%08X\n",
    ProcessData->FdTable.Descriptors[i].ExtraDataSize,
    pBufferTail
   );
  }

 /* success */
 *SerializedProcessData = pspdProcessData;
}

NTSYSAPI
NTSTATUS
NTAPI
__PdxSpawnPosixProcess
(
 OUT PHANDLE ProcessHandle,
 OUT PHANDLE ThreadHandle,
 IN POBJECT_ATTRIBUTES FileObjectAttributes,
 IN POBJECT_ATTRIBUTES ProcessObjectAttributes,
 IN HANDLE InheritFromProcessHandle,
 IN __PPDX_PDATA ProcessData
)
{
 __PPDX_SERIALIZED_PDATA   pspdProcessData;
 IO_STATUS_BLOCK           isbStatus;
 PROCESS_BASIC_INFORMATION pbiProcessInfo;
 ANSI_STRING               strStartEntry;
 PVOID                     pStartAddress;
 INITIAL_TEB               itInitialTeb;
 CONTEXT   ctxThreadContext;
 CLIENT_ID ciClientId;
 NTSTATUS nErrCode;
 HANDLE   hExeFile;
 HANDLE   hExeImage;
 HANDLE   hProcess;
 HANDLE   hThread;
 PVOID    pDestBuffer;
 ULONG    nDestBufferSize;
 ULONG    nCurFilDesOffset;
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

 /* 1.2: create a memory section for the file */
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

 /* STEP 3: write process environment */
 /* 3.1: serialize the process data for transfer */
 /* FIXME: the serialized data can be allocated and written directly in the
    destination process */
 __PdxSerializeProcessData(ProcessData, &pspdProcessData);

 /* 3.1.1: adjust some fields */
 pspdProcessData->ProcessData.Spawned = TRUE;

 /* 3.2: allocate memory in the destination process */
 nDestBufferSize = pspdProcessData->AllocSize;

 nErrCode = NtAllocateVirtualMemory
 (
  hProcess,
  &pDestBuffer,
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

 /* 3.3: get pointer to the PEB */
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

 /* 3.4: write pointer to process data in the SubSystemData field of the PEB */
 nErrCode = NtWriteVirtualMemory
 (
  hProcess,
  (PVOID)((ULONG)pbiProcessInfo.PebBaseAddress + offsetof(PEB, SubSystemData)),
  &pDestBuffer,
  sizeof(PVOID),
  NULL
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtWriteVirtualMemory() failed with status 0x%08X\n", nErrCode);
  goto undoPData;
 }

 /* 3.5: write the process data */
 nErrCode = NtWriteVirtualMemory
 (
  hProcess,
  pDestBuffer,
  pspdProcessData,
  pspdProcessData->AllocSize,
  NULL
 );

undoPData:
  /* deallocate the temporary data block in the current process */
  NtFreeVirtualMemory
  (
   NtCurrentProcess(),
   (PVOID *)&pspdProcessData,
   0,
   MEM_RELEASE
  );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
  return (nErrCode);

 /* STEP 4: duplicate handles */
 /* 4.1: handles in the structure itself */
 /* 4.1.1: root directory */
 nErrCode = NtDuplicateObject
 (
  NtCurrentProcess(),
  ProcessData->RootHandle,
  hProcess,
  (PHANDLE)((ULONG)pDestBuffer + offsetof(__PDX_PDATA, RootHandle)),
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
     (ULONG)pDestBuffer + nCurFilDesOffset + offsetof(__fildes_t, FileHandle)
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
  }

 /* STEP 5: create first thread */
 /* 5.1: get thunk routine's address */
 RtlInitializeAnsiString(&strStartEntry, "LdrInitializeThunk");

 nErrCode = LdrGetProcedureAddress
 (
  (PVOID)NTDLL_BASE,
  &strStartEntry,
  0,
  &pStartAddress
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("LdrGetProcedureAddress() failed with status 0x%08X\n", nErrCode);
  goto failProcess;
 }

 /* 5.2: set up the initial TEB */
 itInitialTeb.StackAllocate = NULL;

 /* FIXME: allow the caller to specify these values */
 itInitialTeb.StackReserve = 0x100000;
 itInitialTeb.StackCommit = itInitialTeb.StackReserve - PAGESIZE;

 /* guard page */
 itInitialTeb.StackCommit += PAGESIZE;

 /* 5.2.1: set up the stack */
 /* 5.2.1.1: reserve the stack */
 nErrCode = NtAllocateVirtualMemory
 (
  hProcess,
  &itInitialTeb.StackAllocate,
  0,
  &itInitialTeb.StackReserve,
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
  (PVOID)((ULONG)itInitialTeb.StackAllocate + itInitialTeb.StackReserve);

 itInitialTeb.StackLimit =
  (PVOID)((ULONG)itInitialTeb.StackBase - itInitialTeb.StackCommit);

 /* 5.2.1.2: commit the stack */
 nErrCode = NtAllocateVirtualMemory
 (
  hProcess,
  &itInitialTeb.StackLimit,
  0,
  &itInitialTeb.StackCommit,
  MEM_COMMIT,
  PAGE_READWRITE
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtAllocateVirtualMemory() failed with status 0x%08X\n", nErrCode);
  goto failProcess;
 }

 /* 5.2.1.3: set up the guard page */
 nErrCode = NtProtectVirtualMemory
 (
  hProcess,
  itInitialTeb.StackLimit,
  PAGESIZE,
  PAGE_GUARD | PAGE_READWRITE,
  NULL
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtProtectVirtualMemory() failed with status 0x%08X\n", nErrCode);
  goto failProcess;
 }

 /* 5.2.1.4: initialize the thread context */
 memset(&ctxThreadContext, 0, sizeof(ctxThreadContext));

 ctxThreadContext.Eip = (ULONG)pStartAddress;
 ctxThreadContext.SegGs = USER_DS;
 ctxThreadContext.SegFs = USER_DS;
 ctxThreadContext.SegEs = USER_DS;
 ctxThreadContext.SegDs = USER_DS;
 ctxThreadContext.SegCs = USER_CS;
 ctxThreadContext.SegSs = USER_DS;
 /* skip five doublewords (four - unknown - parameters for LdrInitializeThunk,
    and the return address) */
 ctxThreadContext.Esp = (ULONG)itInitialTeb.StackBase - 5 * 4;
 ctxThreadContext.EFlags = (1 << 1) + (1 << 9);

 /* 5.3: create the thread object */
 nErrCode = NtCreateThread
 (
  NULL,
  THREAD_ALL_ACCESS,
  NULL,
  hProcess,
  &ciClientId,
  &ctxThreadContext,
  &itInitialTeb,
  FALSE
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtCreateThread() failed with status 0x%08X\n", nErrCode);
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

