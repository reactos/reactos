#define _KERNEL32_
#include <w4warn.h>
#include <windows.h>
#include <w4warn.h>

// Hack to ensure pdlparse will build on pass0 of the NT build.
// Basically, no library can be referenced except msvcrt.lib because
// on a true clean build of NT, there are no libraries except the
// CRT's until after pass 1 is finished.

extern "C" void __stdcall Sleep(unsigned long) {}
extern "C" DWORD __stdcall GetVersion(void) { return 0; }
