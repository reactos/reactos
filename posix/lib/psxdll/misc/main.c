/* $Id: main.c,v 1.6 2002/10/29 04:45:33 rex Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/misc/main.c
 * PURPOSE:     psxdll.dll entry point
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              27/12/2001: Created
 */

#include <ddk/ntddk.h>
#include <string.h>
#include <psx/debug.h>
#include <psx/fdtable.h>
#include <psx/pdata.h>
#include <psx/stdlib.h>
#include <psx/tls.h>

#define DLL_PROCESS_ATTACH 1
#define DLL_THREAD_ATTACH  2
#define DLL_THREAD_DETACH  3
#define DLL_PROCESS_DETACH 0

/* WARNING: PRELIMINARY CODE FOR DEBUGGING PURPOSES ONLY - DO NOT CHANGE */
static __PDX_PDATA __PdxPdata;
static WCHAR __tempPathBuf[32768];
static char * __tempSelf = "-sh";
static char * __tempArgv[2] = {0, 0};

VOID __PdxSetProcessData(__PPDX_PDATA NewPdata)
{
 memcpy(&__PdxPdata, NewPdata, sizeof(__PdxPdata));
}

__PPDX_PDATA __PdxGetProcessData(VOID)
{
 return &__PdxPdata;
}

BOOL STDCALL DllMain(PVOID pDllInstance, DWORD nReason, PVOID pUnknown)
{
 ULONG nJunk;

 switch(nReason)
 {
  /* process created, first thread created */
  case DLL_PROCESS_ATTACH:
  {
   __PPDX_TDATA ThreadData;
   int i;
   
   INFO("new process and new thread created");

   __PdxPdata.Spawned = 1;
   
   __PdxPdata.ArgCount = 1;
   __tempArgv[0] = __tempSelf;
   __PdxPdata.ArgVect = __tempArgv;
   
   __PdxPdata.NativePathBuffer.Length = 0;
   __PdxPdata.NativePathBuffer.MaximumLength = sizeof(__tempPathBuf);
   __PdxPdata.NativePathBuffer.Buffer = __tempPathBuf;
   
   INFO("about to initialize process data lock");
   RtlInitializeCriticalSection(&__PdxPdata.Lock);
   
   INFO("about to allocate TLS slot");
   __PdxPdata.TlsIndex = RtlFindClearBitsAndSet(NtCurrentPeb()->TlsBitmap, 1, 0);
   
   if(__PdxPdata.TlsIndex == -1)
   {
    DbgBreakPoint();
    NtRaiseHardError(STATUS_NO_MEMORY, 0, 0, 0, 1, (ULONG)&nJunk);
    return (FALSE);
   }
   
   INFO("allocated TLS slot %d", __PdxPdata.TlsIndex);

   INFO("about to allocate thread data");
   ThreadData = __malloc(sizeof(*ThreadData));
   
   if(ThreadData == 0)
   {
    DbgBreakPoint();
    NtRaiseHardError(STATUS_NO_MEMORY, 0, 0, 0, 1, (ULONG)&nJunk);
    return (FALSE);   
   }
   
   NtCurrentTeb()->TlsSlots[__PdxPdata.TlsIndex] = ThreadData;
   
   INFO("about to initialize file descriptors table");
   __fdtable_init(&__PdxPdata.FdTable);

   INFO("end of initialization");   
   return (TRUE);
  }

  /* process about to exit */
  case DLL_PROCESS_DETACH:
  {
   INFO("process about to exit");
   
   INFO("about to deallocate thread data");
   __free(NtCurrentTeb()->TlsSlots[__PdxPdata.TlsIndex]);
   
   return (TRUE);
  }

  /* thread created */
  case DLL_THREAD_ATTACH:
  {
   __PPDX_TDATA ThreadData;
   
   INFO("new thread created");
   
   INFO("about to allocate thread data");
   ThreadData = __malloc(sizeof(*ThreadData));
   
   if(ThreadData == 0)
   {
    DbgBreakPoint();
    NtRaiseHardError(STATUS_NO_MEMORY, 0, 0, 0, 1, (ULONG)&nJunk);
    return (FALSE);   
   }
   
   NtCurrentTeb()->TlsSlots[__PdxPdata.TlsIndex] = ThreadData;
   
   return (TRUE);
  }

  /* thread exited */
  case DLL_THREAD_DETACH:
  {
   INFO("thread about to exit");
   
   INFO("about to deallocate thread data");
   __free(NtCurrentTeb()->TlsSlots[__PdxPdata.TlsIndex]);
   
   return (TRUE);
  }
 }
 
 return (FALSE);
}

/* EOF */

