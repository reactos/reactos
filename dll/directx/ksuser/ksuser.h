#ifndef KSUSER_H__
#define KSUSER_H__

#define _KSDDK_

#define WIN32_NO_STATUS
#include <windows.h>
#include <ndk/ntndk.h>


#include <ks.h>

LPVOID
__stdcall
HeapAlloc(
  HANDLE hHeap,
  DWORD dwFlags,
  DWORD dwBytes
);

#define ROUND_DOWN(n, align) \
    (((ULONG)n) & ~((align) - 1l))

#define ROUND_UP(n, align) \
    ROUND_DOWN(((ULONG)n) + (align) - 1, (align))

#endif
