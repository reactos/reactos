/*
 *  Entry Point for win32k.sys
 */

#include <windows.h>
#include <ddk/ntddk.h>

WINBOOL STDCALL DllMain(HANDLE hInst, 
			ULONG ul_reason_for_call,
			LPVOID lpReserved)
{
  DbgPrint("win32k:DllMain(hInst %x, ul_reason_for_call %d)\n",
           hInst, 
           ul_reason_for_call);

  return TRUE;  
}

#if 0
HDC GDICreateDC(LPCWSTR Driver,
                LPCWSTR Device,
                CONST PDEVMODE InitData)
{
  /* %%% initialize device driver here on first call for display DC.  */
}
#endif
