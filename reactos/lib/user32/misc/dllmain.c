#include <windows.h>
#include <debug.h>
#include <user32/callback.h>
#include <window.h>

#ifdef DBG

/* See debug.h for debug/trace constants */
DWORD DebugTraceLevel = MIN_TRACE;

#endif /* DBG */

/* To make the linker happy */
VOID STDCALL KeBugCheck (ULONG	BugCheckCode) {}

HWINSTA ProcessWindowStation;

DWORD
Init(VOID)
{
  DWORD Status;

  /* Set up the kernel callbacks. */
  NtCurrentPeb()->KernelCallbackTable[USER32_CALLBACK_WINDOWPROC] =
    (PVOID)User32CallWindowProcFromKernel;
  NtCurrentPeb()->KernelCallbackTable[USER32_CALLBACK_SENDASYNCPROC] =
    (PVOID)User32CallSendAsyncProcForKernel;
  NtCurrentPeb()->KernelCallbackTable[USER32_CALLBACK_SENDNCCREATE] =
    (PVOID)User32SendNCCREATEMessageForKernel;
  NtCurrentPeb()->KernelCallbackTable[USER32_CALLBACK_SENDCREATE] =
    (PVOID)User32SendCREATEMessageForKernel;
  NtCurrentPeb()->KernelCallbackTable[USER32_CALLBACK_SENDGETMINMAXINFO] =
    (PVOID)User32SendGETMINMAXINFOMessageForKernel;
  NtCurrentPeb()->KernelCallbackTable[USER32_CALLBACK_SENDNCCALCSIZE] =
    (PVOID)User32SendNCCALCSIZEMessageForKernel;
  NtCurrentPeb()->KernelCallbackTable[USER32_CALLBACK_SENDWINDOWPOSCHANGING] =
    (PVOID)User32SendWINDOWPOSCHANGINGMessageForKernel;
  NtCurrentPeb()->KernelCallbackTable[USER32_CALLBACK_SENDWINDOWPOSCHANGED] =
    (PVOID)User32SendWINDOWPOSCHANGEDMessageForKernel;
  NtCurrentPeb()->KernelCallbackTable[USER32_CALLBACK_SENDSTYLECHANGING] =
    (PVOID)User32SendSTYLECHANGINGMessageForKernel;
  NtCurrentPeb()->KernelCallbackTable[USER32_CALLBACK_SENDSTYLECHANGED] =
    (PVOID)User32SendSTYLECHANGEDMessageForKernel;

  UserSetupInternalPos();

  GdiDllInitialize(NULL, DLL_PROCESS_ATTACH, NULL);

  return(Status);
}

DWORD
Cleanup(VOID)
{
  DWORD Status;

  GdiDllInitialize(NULL, DLL_PROCESS_DETACH, NULL);

  return(Status);
}

INT STDCALL
DllMain(PVOID	hinstDll,
	ULONG	dwReason,
	PVOID	reserved)
{
  D(MAX_TRACE, ("hinstDll (0x%X)  dwReason (0x%X)\n", hinstDll, dwReason));

  switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
      Init();
      break;
    case DLL_THREAD_ATTACH:
      break;
    case DLL_THREAD_DETACH:
      break;
    case DLL_PROCESS_DETACH:
      Cleanup();
      break;
    }
  return(1);
}

