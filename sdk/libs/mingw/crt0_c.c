/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#include <windows.h>

int main (int flags, char **cmdline, char **inst)
{
  return (int) WinMain ((HINSTANCE) inst, NULL, (LPSTR) cmdline,(DWORD) flags);
}
