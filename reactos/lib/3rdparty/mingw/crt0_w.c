/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#include <windows.h>

/* Do the UNICODE prototyping of WinMain.  Be aware that in winbase.h WinMain is a macro
   defined to wWinMain.  */
int WINAPI wWinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPWSTR lpCmdLine,int nShowCmd);

extern HINSTANCE __mingw_winmain_hInstance;
extern LPWSTR __mingw_winmain_lpCmdLine;
extern DWORD __mingw_winmain_nShowCmd;

int wmain (int, wchar_t **, wchar_t **);

/*ARGSUSED*/
int wmain (int flags __attribute__ ((__unused__)),
	   wchar_t **cmdline __attribute__ ((__unused__)),
	   wchar_t **inst __attribute__ ((__unused__)))
{
  return (int) wWinMain (__mingw_winmain_hInstance, NULL,
			__mingw_winmain_lpCmdLine, __mingw_winmain_nShowCmd);
}
