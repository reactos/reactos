/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#include <windows.h>

extern HINSTANCE __mingw_winmain_hInstance;
extern LPSTR __mingw_winmain_lpCmdLine;
extern DWORD __mingw_winmain_nShowCmd;

/*ARGSUSED*/
int main (int flags __attribute__ ((__unused__)),
	  char **cmdline __attribute__ ((__unused__)),
	  char **inst __attribute__ ((__unused__)))
{
  return (int) WinMain (__mingw_winmain_hInstance, NULL,
			__mingw_winmain_lpCmdLine, __mingw_winmain_nShowCmd);
}
