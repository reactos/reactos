/* $Id: fork.c,v 1.1 2002/05/17 02:12:55 hyperion Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/unistd/fork.c
 * PURPOSE:     create a new process
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              14/05/2002: Created
 */

#include <ddk/ntddk.h>
#include <napi/teb.h>
#include <sys/types.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <psx/debug.h>
#include <psx/errno.h>

#include <windows.h>

typedef struct _PORT_MESSAGE {
 USHORT DataSize;
 USHORT MessageSize;
 USHORT MessageType;
 USHORT VirtualRangesOffset;
 CLIENT_ID ClientId;
 ULONG MessageId;
 ULONG SectionSize;
 /* UCHAR Data[]; */
} PORT_MESSAGE, *PPORT_MESSAGE;

struct CSRSS_MESSAGE {
 ULONG Unknown1;
 ULONG Opcode;
 ULONG Status;
 ULONG Unknown2;
};

NTSTATUS STDCALL CsrClientCallServer(
 IN PVOID Message,
 IN PVOID Unknown,
 IN ULONG Opcode,
 IN ULONG Size
);

pid_t fork(void)
{
 NTSTATUS nErrCode;
 CONTEXT ctxThreadContext;
 HANDLE hProcess;
 HANDLE hThread;
 INITIAL_TEB itInitialTeb;
 CLIENT_ID ciClientId;
 MEMORY_BASIC_INFORMATION mbiStackInfo;
 THREAD_BASIC_INFORMATION tbiThreadInfo;

 struct __tagcsrmsg{
  PORT_MESSAGE PortMessage;
  struct CSRSS_MESSAGE CsrssMessage;
  PROCESS_INFORMATION ProcessInformation;
  CLIENT_ID Debugger;
  ULONG CreationFlags;
  ULONG VdmInfo[2];
 } csrmsg;

 /* STEP 1: Duplicate current process */ 
 nErrCode = NtCreateProcess
 (
  &hProcess,
  PROCESS_ALL_ACCESS,
  NULL,
  NtCurrentProcess(),
  TRUE,
  0,
  0,
  0
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtCreateProcess() failed with status 0x%08X\n", nErrCode);
  goto fail;
 }

 /* STEP 2: Duplicate current thread */
 /* 2.1: duplicate registers */
 ctxThreadContext.ContextFlags =
  CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS | CONTEXT_FLOATING_POINT;
 
 /* get the current thread's registers */
 nErrCode = NtGetContextThread(NtCurrentThread(), &ctxThreadContext);
 
 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtGetContextThread() failed with status 0x%08X\n", nErrCode);
  goto cleanup_and_fail;
 }

 /* redirect the child process to the child_branch label (see 4.3 below) */
 ctxThreadContext.Eip = (ULONG)&&child_branch;
 
 /* 2.2: duplicate stack */
 /* get stack base and size */
 nErrCode = NtQueryVirtualMemory
 (
  NtCurrentProcess(),
  (PVOID)ctxThreadContext.Esp,
  MemoryBasicInformation,
  &mbiStackInfo,
  sizeof(mbiStackInfo),
  0
 );
 
 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtQueryVirtualMemory() failed with status 0x%08X\n", nErrCode);
  goto cleanup_and_fail;
 }
 
 itInitialTeb.StackCommit = 0;
 itInitialTeb.StackReserve = 0;
 itInitialTeb.StackBase = (PVOID)((ULONG)(mbiStackInfo.BaseAddress) + mbiStackInfo.RegionSize);
 itInitialTeb.StackLimit = mbiStackInfo.BaseAddress;
 itInitialTeb.StackAllocate = mbiStackInfo.AllocationBase;
 
 /* 2.3: create duplicate thread */
 nErrCode = NtCreateThread
 (
  &hThread,
  THREAD_ALL_ACCESS,
  NULL,
  hProcess,
  (CLIENT_ID *)&ciClientId,
  &ctxThreadContext,
  &itInitialTeb,
  TRUE
 );
 
 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtCreateThread() failed with status 0x%08X\n", nErrCode);
  goto cleanup_and_fail;
 }

 /* 2.4: duplicate the TEB */
 /* store the client id in the child thread's stack (see 4.3b) */
 nErrCode = NtWriteVirtualMemory
 (
  hProcess,
  &ciClientId,
  &ciClientId,
  sizeof(ciClientId),
  0
 );
 
 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtWriteVirtualMemory() failed with status 0x%08X\n", nErrCode);
  goto cleanup_and_fail;
 }
 
 /* get the child thread's TEB base */
 nErrCode = NtQueryInformationThread
 (
  hThread,
  ThreadBasicInformation,
  &tbiThreadInfo,
  sizeof(tbiThreadInfo),
  0
 );
 
 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtQueryInformationThread() failed with status 0x%08X\n", nErrCode);
  goto cleanup_and_fail;
 }
 
 /* copy the TEB */ 
 nErrCode = NtWriteVirtualMemory
 (
  hProcess,
  tbiThreadInfo.TebBaseAddress,
  NtCurrentTeb(),
  sizeof(TEB),
  0
 );
 
 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtWriteVirtualMemory() failed with status 0x%08X\n", nErrCode);
  goto cleanup_and_fail;
 }
 
 /* STEP 3: Call Win32 subsystem */
 memset(&csrmsg, 0, sizeof(csrmsg));

 csrmsg.ProcessInformation.hProcess = hProcess;
 csrmsg.ProcessInformation.hThread = hThread;
 csrmsg.ProcessInformation.dwProcessId = (DWORD)ciClientId.UniqueProcess;
 csrmsg.ProcessInformation.dwThreadId = (DWORD)ciClientId.UniqueThread;

 nErrCode = CsrClientCallServer(&csrmsg, 0, 0x10000, 0x24);

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("CsrClientCallServer() failed with status 0x%08X\n", nErrCode);
  goto cleanup_and_fail;
 }
 
 /* STEP 4: Finalization */
 /* 4.1: resume thread */
 nErrCode = NtResumeThread(hThread, 0);
 
 /* 4.2: close superfluous handles */
 NtClose(hProcess);
 NtClose(hThread);
 
 /* 4.3: (parent) return the child process id */
 return ((pid_t)(ciClientId.UniqueProcess));

 /* 4.3b: (child) cleanup and return 0 */
child_branch:
 /* restore the thread and process id in the TEB */ 
 memcpy(&NtCurrentTeb()->Cid, &ciClientId, sizeof(ciClientId));
 
 /* return 0 */
 return (0);

cleanup_and_fail:
 NtTerminateProcess(hProcess, nErrCode);

fail:
 errno = __status_to_errno(nErrCode);
 return (-1);
 
}

/* EOF */

