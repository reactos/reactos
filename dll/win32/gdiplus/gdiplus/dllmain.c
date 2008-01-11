#include <windows.h>
#include <gdiplusprivate.h>
#include <debug.h>

#ifdef DBG

/* See debug.h for debug/trace constants */
DWORD DebugTraceLevel = MIN_TRACE;

#endif /* DBG */

/*
 * @unimplemented
 */
Status WINAPI
GdiplusStartup(ULONG_PTR *token,
  const GdiplusStartupInput *input,
  GdiplusStartupOutput *output)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
VOID WINAPI
GdiplusShutdown(ULONG_PTR token)
{
}

INT STDCALL
DllMain(
	PVOID hinstDll,
	ULONG dwReason,
	PVOID reserved)
{
  switch (dwReason)
    {
      case DLL_PROCESS_ATTACH:
        break;
      case DLL_THREAD_ATTACH:
        break;
      case DLL_THREAD_DETACH:
        break;
      case DLL_PROCESS_DETACH:
        break;
    }

  return 1;
}
