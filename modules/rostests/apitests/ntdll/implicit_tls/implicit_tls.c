#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <ndk/ntndk.h>
#include <windows.h>

#if defined(_MSC_VER)
#define _CRTALLOC(x) __declspec(allocate(x))
#elif defined(__GNUC__)
#define _CRTALLOC(x) __attribute__ ((section (x) ))
#else
#error Your compiler is not supported.
#endif


/* Tls magic stolen from sdk/lib/crt/startup/tlssup.c */
/* ROS is built with the flag /Zc:threadSafeInit- which prevents the */
/* compiling of proper TLS code. Instead, hack up a TLS directory to increment the TLS index and */
/* allow the loader to allocate a sufficient buffer in the TLS vector. */

#if defined(_MSC_VER)
#pragma section(".rdata$T",long,read)
#pragma section(".tls",long,read,write)
#pragma section(".tls$ZZZ",long,read,write)
#endif

_CRTALLOC(".tls") char _tls_start = 0;
_CRTALLOC(".tls$ZZZ") char _tls_end = 0;

ULONG _tls_index = 0;

_CRTALLOC(".rdata$T") const IMAGE_TLS_DIRECTORY _tls_used = {
  (ULONG_PTR) &_tls_start, (ULONG_PTR) &_tls_end + 3,
  (ULONG_PTR) &_tls_index, (ULONG_PTR) 0,
  (ULONG) 0, (ULONG) 0
};

BOOL WINAPI
DllMain(IN HINSTANCE hDllHandle,
        IN DWORD dwReason,
        IN LPVOID lpvReserved)
{
    PTEB Teb = NtCurrentTeb();
    PVOID* TlsVector = Teb->ThreadLocalStoragePointer;
    PULONG ModuleHandle = TlsVector[_tls_index];
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        #if defined(_MSC_VER)
        #pragma warning( disable : 4311)
        #endif
        *ModuleHandle = (int)GetModuleHandleA(NULL) + 1;
	}
    return TRUE;
}
