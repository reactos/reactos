/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#include <windows.h>

extern void __main(void);

int WINAPI wWinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPWSTR lpCmdLine,int nShowCmd);

int wmain (int flags, wchar_t **cmdline, wchar_t **inst)
{
  return (int) wWinMain ((HINSTANCE) inst, NULL, (LPWSTR) cmdline,(DWORD) flags);
}
