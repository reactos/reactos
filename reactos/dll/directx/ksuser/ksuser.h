#ifndef KSUSER_H__
#define KSUSER_H__

#define _KSDDK_

#define WIN32_NO_STATUS
#include <windows.h>
#include <ndk/ntndk.h>


#include <ks.h>
#include <reactos/helper.h>

LPVOID
__stdcall
HeapAlloc(
  HANDLE hHeap,
  DWORD dwFlags,
  DWORD dwBytes
);

#endif
