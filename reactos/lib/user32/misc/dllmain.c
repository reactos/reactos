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

static ULONG User32TlsIndex;

/* To make the linker happy */
VOID STDCALL KeBugCheck (ULONG	BugCheckCode) {}

HWINSTA ProcessWindowStation;
HBRUSH FrameBrushes[13];
HBITMAP hHatch;

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
VOID 
CreateFrameBrushes(HINSTANCE hInstance)
{
	FrameBrushes[0] = CreateSolidBrush(RGB(0,0,0));
	FrameBrushes[1] = CreateSolidBrush(RGB(0,0,128));
	FrameBrushes[2] = CreateSolidBrush(RGB(10,36,106));
	FrameBrushes[3] = CreateSolidBrush(RGB(128,128,128));
	FrameBrushes[4] = CreateSolidBrush(RGB(181,181,181));
	FrameBrushes[5] = CreateSolidBrush(RGB(212,208,200));
	FrameBrushes[6] = CreateSolidBrush(RGB(236,233,216));
	FrameBrushes[7] = CreateSolidBrush(RGB(255,255,255));
	FrameBrushes[8] = CreateSolidBrush(RGB(49,106,197));
	FrameBrushes[9] = CreateSolidBrush(RGB(58,110,165));
	FrameBrushes[10] = CreateSolidBrush(RGB(64,64,64));
	FrameBrushes[11] = CreateSolidBrush(RGB(255,255,225));
	hHatch = LoadBitmapW(hInstance,MAKEINTRESOURCEW(DF_HATCH));
	FrameBrushes[12] = CreatePatternBrush(hHatch);
}
VOID 
DeleteFrameBrushes(VOID)
{
	DeleteObject(FrameBrushes[0]);
	DeleteObject(FrameBrushes[1]);
	DeleteObject(FrameBrushes[2]);
	DeleteObject(FrameBrushes[3]);
	DeleteObject(FrameBrushes[4]);
	DeleteObject(FrameBrushes[5]);
	DeleteObject(FrameBrushes[6]);
	DeleteObject(FrameBrushes[7]);
	DeleteObject(FrameBrushes[8]);
	DeleteObject(FrameBrushes[9]);
	DeleteObject(FrameBrushes[10]);
	DeleteObject(FrameBrushes[11]);
	DeleteObject(FrameBrushes[12]);
	DeleteObject(hHatch);
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

  /* Allocate an index for user32 thread local data. */
  User32TlsIndex = TlsAlloc();

  UserSetupInternalPos();
  MenuInit();

  RtlInitializeCriticalSection(&U32AccelCacheLock);

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
  D(MAX_TRACE, ("hinstDll (0x%X)  dwReason (0x%X)\n", hinstDll, dwReason));

  switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
      hProcessHeap = RtlGetProcessHeap();
      Init();
      InitThread();
	  CreateFrameBrushes(hinstDll);
      break;
    case DLL_THREAD_ATTACH:
      InitThread();
      break;
    case DLL_THREAD_DETACH:
      CleanupThread();
      break;
    case DLL_PROCESS_DETACH:
	  DeleteFrameBrushes();
      CleanupThread();
      Cleanup();
      break;
    }
  return(1);
}
