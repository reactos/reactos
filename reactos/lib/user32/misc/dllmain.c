#include <windows.h>
#include <debug.h>

#ifdef DBG

/* See debug.h for debug/trace constants */
DWORD DebugTraceLevel = MIN_TRACE;

#endif /* DBG */

/* To make the linker happy */
VOID STDCALL KeBugCheck (ULONG	BugCheckCode) {}

HANDLE ProcessHeap;
HWINSTA ProcessWindowStation;

DWORD
Init(VOID)
{
  DWORD Status;

  ProcessHeap = RtlGetProcessHeap();

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

INT
STDCALL
DllMain(
	PVOID	hinstDll,
	ULONG	dwReason,
	PVOID	reserved
	)
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

