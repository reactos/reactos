#include <windows.h>
#include <debug.h>
#include <user32/callback.h>
#include <user32/accel.h>
#include <window.h>
#include <menu.h>
#include <user32.h>
#include <strpool.h>

#ifdef DBG

/* See debug.h for debug/trace constants */
DWORD DebugTraceLevel = MIN_TRACE;

#endif /* DBG */

extern RTL_CRITICAL_SECTION gcsMPH;
static ULONG User32TlsIndex;

/* To make the linker happy */
VOID STDCALL KeBugCheck (ULONG	BugCheckCode) {}

HWINSTA ProcessWindowStation;

PUSER32_THREAD_DATA
User32GetThreadData()
{
  return((PUSER32_THREAD_DATA)TlsGetValue(User32TlsIndex));
}

VOID
InitThread(VOID)
{
  PUSER32_THREAD_DATA ThreadData;

  ThreadData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 
			 sizeof(USER32_THREAD_DATA));
  TlsSetValue(User32TlsIndex, ThreadData);
}

VOID
CleanupThread(VOID)
{
  PUSER32_THREAD_DATA ThreadData;

  ThreadData = (PUSER32_THREAD_DATA)TlsGetValue(User32TlsIndex);
  HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, ThreadData);
  TlsSetValue(User32TlsIndex, 0);
}

DWORD
Init(VOID)
{
  DWORD Status;

  /* Set up the kernel callbacks. */
  NtCurrentPeb()->KernelCallbackTable[USER32_CALLBACK_WINDOWPROC] =
    (PVOID)User32CallWindowProcFromKernel;
  NtCurrentPeb()->KernelCallbackTable[USER32_CALLBACK_SENDASYNCPROC] =
    (PVOID)User32CallSendAsyncProcForKernel;
  NtCurrentPeb()->KernelCallbackTable[USER32_CALLBACK_LOADSYSMENUTEMPLATE] =
    (PVOID)User32LoadSysMenuTemplateForKernel;
  NtCurrentPeb()->KernelCallbackTable[USER32_CALLBACK_LOADDEFAULTCURSORS] =
    (PVOID)User32SetupDefaultCursors;
  NtCurrentPeb()->KernelCallbackTable[USER32_CALLBACK_HOOKPROC] =
    (PVOID)User32CallHookProcFromKernel;

  /* Allocate an index for user32 thread local data. */
  User32TlsIndex = TlsAlloc();

  MenuInit();

  RtlInitializeCriticalSection(&U32AccelCacheLock);
  RtlInitializeCriticalSection(&gcsMPH);

  GdiDllInitialize(NULL, DLL_PROCESS_ATTACH, NULL);

  return(Status);
}

DWORD
Cleanup(VOID)
{
  DWORD Status;

  GdiDllInitialize(NULL, DLL_PROCESS_DETACH, NULL);

  TlsFree(User32TlsIndex);

  return(Status);
}



INT STDCALL
DllMain(
	PVOID  hinstDll,
	ULONG  dwReason,
	PVOID  reserved
	)
{
  switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
      hProcessHeap = RtlGetProcessHeap();
      Init();
/*      InitThread();*/
      break;
    case DLL_THREAD_ATTACH:
      InitThread();
      break;
    case DLL_THREAD_DETACH:
      CleanupThread();
      break;
    case DLL_PROCESS_DETACH:
      DeleteFrameBrushes();
/*      CleanupThread();*/
      Cleanup();
      break;
    }
  return(1);
}
