#include <windows.h>
#include <debug.h>
#include <user32/callback.h>

#ifdef DBG

/* See debug.h for debug/trace constants */
DWORD DebugTraceLevel = MIN_TRACE;

#endif /* DBG */

/* To make the linker happy */
VOID STDCALL KeBugCheck (ULONG	BugCheckCode) {}

HANDLE ProcessHeap;
HWINSTA ProcessWindowStation;

PVOID
User32AllocHeap(ULONG Size)
{
  return(RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY, Size));
}

VOID
User32FreeHeap(PVOID Block)
{
  RtlFreeHeap(ProcessHeap, 0, Block);
}

PWSTR
User32ConvertString(PCSTR String)
{
  ANSI_STRING InString;
  UNICODE_STRING OutString;
  RtlInitAnsiString(&InString, String);
  RtlAnsiStringToUnicodeString(&OutString, &InString, TRUE);
  return(OutString.Buffer);
}

VOID
User32FreeString(PWSTR String)
{
  RtlFreeHeap(RtlGetProcessHeap(), 0, String);
}

DWORD
Init(VOID)
{
  DWORD Status;

  ProcessHeap = RtlGetProcessHeap();

  /* Set up the kernel callbacks. */
  NtCurrentPeb()->KernelCallbackTable[USER32_CALLBACK_WINDOWPROC] =
    (PVOID)User32CallWindowProcFromKernel;
  NtCurrentPeb()->KernelCallbackTable[USER32_CALLBACK_SENDASYNCPROC] =
    (PVOID)User32CallSendAsyncProcForKernel;
  NtCurrentPeb()->KernelCallbackTable[USER32_CALLBACK_SENDNCCREATE] =
    (PVOID)User32SendNCCREATEMessageForKernel;
  NtCurrentPeb()->KernelCallbackTable[USER32_CALLBACK_SENDCREATE] =
    (PVOID)User32SendCREATEMessageForKernel;

  //ProcessWindowStation = CreateWindowStationW(L"WinStaName",0,GENERIC_ALL,NULL);
  //Desktop = CreateDesktopA(NULL,NULL,NULL,0,0,NULL);

  //GdiDllInitialize(NULL, DLL_PROCESS_ATTACH, NULL);

  return Status;
}

DWORD
Cleanup(VOID)
{
  DWORD Status;

  //CloseWindowStation(ProcessWindowStation);

  //GdiDllInitialize(NULL, DLL_PROCESS_DETACH, NULL);

  return Status;
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

