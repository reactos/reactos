/* $Id: pdata.c,v 1.1 2002/03/10 17:04:07 hyperion Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/misc/pdata.c
 * PURPOSE:     Process data management
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              06/03/2002: Created
 *              07/03/2002: Added __PdxUnserializeProcessData() (KJK::Hyperion
 *                          <noog@libero.it>)
 */

#include <ddk/ntddk.h>
#include <string.h>
#include <psx/fdtable.h>
#include <psx/pdata.h>
#include <psx/stdlib.h>
#include <psx/debug.h>

/* serialize a process data block in a contiguous, page-aligned block, suitable
   for transfer across processes */
NTSTATUS
STDCALL
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

/* unserialize a process data block. Dynamic data will be moved into the default
   heap */
NTSTATUS
STDCALL
__PdxUnserializeProcessData
(
 IN OUT __PPDX_SERIALIZED_PDATA *SerializedProcessData,
 OUT __PPDX_PDATA *ProcessData OPTIONAL
)
{
 int i;
 int nEnvVarsCount;
 __PPDX_PDATA ppdReturnBlock;
 BOOLEAN      bInPlace;
 PBYTE        pBufferTail;

 /* no return buffer */
 if(NULL == ProcessData)
 {
  /* perform an in-place conversion */
  ppdReturnBlock = &((*SerializedProcessData)->ProcessData);
  bInPlace = TRUE;
 }
 else
 {
  /* use the provided return buffer */
  ppdReturnBlock = *ProcessData;
  bInPlace = FALSE;
 }

 /* non in-place conversion: copy static data */
 if(!bInPlace)
 {
  memcpy(ppdReturnBlock, *SerializedProcessData, sizeof(*ppdReturnBlock));
 }

 pBufferTail = &((*SerializedProcessData)->Buffer[0]);

 /* allocate arguments array */
 ppdReturnBlock->ArgVect = __malloc(ppdReturnBlock->ArgCount * sizeof(char *));

 /* duplicate arguments */
 for(i = 0; i < ppdReturnBlock->ArgCount; i ++)
 {
  int nStrLen = strlen(pBufferTail) + 1;
  ppdReturnBlock->ArgVect[i] = __malloc(nStrLen);
  strncpy(ppdReturnBlock->ArgVect[i], pBufferTail, nStrLen);
  pBufferTail += nStrLen;
 }

 /* allocate environment array */
 nEnvVarsCount = ppdReturnBlock->Environment;
 ppdReturnBlock->Environment = __malloc(nEnvVarsCount * sizeof(char *));

 /* duplicate environment */
 for(i = 0; i < nEnvVarsCount; i ++)
 {
  int nStrLen = strlen(pBufferTail) + 1;
  ppdReturnBlock->Environment[i] = __malloc(nStrLen);
  strncpy(ppdReturnBlock->Environment[i], pBufferTail, nStrLen);
  pBufferTail += nStrLen;
 }

 /* static buffer for path conversions */
 ppdReturnBlock->NativePathBuffer.Buffer = __malloc(0xFFFF);
 ppdReturnBlock->NativePathBuffer.Length = 0;
 ppdReturnBlock->NativePathBuffer.MaximumLength = 0xFFFF;

 /* current directory */
 ppdReturnBlock->CurDir.Buffer = __malloc(ppdReturnBlock->CurDir.Length);
 ppdReturnBlock->CurDir.MaximumLength = ppdReturnBlock->CurDir.Length;
 memcpy(ppdReturnBlock->CurDir.Buffer, pBufferTail, ppdReturnBlock->CurDir.Length);
 pBufferTail += ppdReturnBlock->CurDir.Length;

 /* root directory */
 ppdReturnBlock->RootPath.Buffer = __malloc(ppdReturnBlock->RootPath.Length);
 ppdReturnBlock->RootPath.MaximumLength = ppdReturnBlock->RootPath.Length;
 memcpy(ppdReturnBlock->RootPath.Buffer, pBufferTail, ppdReturnBlock->RootPath.Length);
 pBufferTail += ppdReturnBlock->RootPath.Length;

 /* file descriptors table */
 ppdReturnBlock->FdTable.Descriptors = __malloc(ppdReturnBlock->FdTable.AllocatedDescriptors * sizeof(__fildes_t));
 memcpy(ppdReturnBlock->FdTable.Descriptors, pBufferTail, ppdReturnBlock->FdTable.AllocatedDescriptors * sizeof(__fildes_t));
 pBufferTail += ppdReturnBlock->FdTable.AllocatedDescriptors * sizeof(__fildes_t);

 for(i = 0; i < ppdReturnBlock->FdTable.AllocatedDescriptors; i ++)
 {
  if(ppdReturnBlock->FdTable.Descriptors[i].ExtraData != 0)
  {
   ppdReturnBlock->FdTable.Descriptors[i].ExtraData = __malloc(ppdReturnBlock->FdTable.Descriptors[i].ExtraDataSize);
   memcpy(ppdReturnBlock->FdTable.Descriptors[i].ExtraData, pBufferTail, ppdReturnBlock->FdTable.Descriptors[i].ExtraDataSize);
   pBufferTail += ppdReturnBlock->FdTable.Descriptors[i].ExtraDataSize;
  }
 }
}

/* EOF */

