#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windows.h>


#if defined(_MSC_VER)
#define _CRTALLOC(x) __declspec(allocate(x))
#elif defined(__GNUC__)
#define _CRTALLOC(x) __attribute__ ((section (x) ))
#else
#error Your compiler is not supported.
#endif


static VOID (WINAPI* pTlsCallback)(IN HINSTANCE hDllHandle, IN DWORD dwReason, IN LPVOID lpvReserved);

VOID WINAPI
TlsCallback(IN HANDLE hDllHandle,
            IN DWORD dwReason,
            IN LPVOID lpvReserved)
{
    if (!pTlsCallback)
        pTlsCallback = (VOID*)GetProcAddress(NULL, "notify_TlsCallback");
    if (pTlsCallback)
    {
        pTlsCallback(hDllHandle, dwReason, lpvReserved);
        return;
    }
    OutputDebugStringA("WARNING: load_notifications.dll loaded from a process without notify_TlsCallback\n");
}

/* Tls magic stolen from sdk/lib/crt/startup/tlssup.c */

#if defined(_MSC_VER)
#pragma section(".rdata$T",long,read)
#pragma section(".tls",long,read,write)
#pragma section(".tls$ZZZ",long,read,write)
#endif

_CRTALLOC(".tls") char _tls_start = 0;
_CRTALLOC(".tls$ZZZ") char _tls_end = 0;

PIMAGE_TLS_CALLBACK __xl_a[2] = { &TlsCallback, NULL };

ULONG _tls_index = 0;

_CRTALLOC(".rdata$T") const IMAGE_TLS_DIRECTORY _tls_used = {
  (ULONG_PTR) &_tls_start+1, (ULONG_PTR) &_tls_end,
  (ULONG_PTR) &_tls_index, (ULONG_PTR) (__xl_a),
  (ULONG) 0, (ULONG) 0
};


static BOOL (WINAPI* pDllMain)(IN HINSTANCE hDllHandle, IN DWORD dwReason, IN LPVOID lpvReserved);


BOOL WINAPI
DllMain(IN HINSTANCE hDllHandle,
        IN DWORD dwReason,
        IN LPVOID lpvReserved)
{
    if (!pDllMain)
        pDllMain = (VOID*)GetProcAddress(NULL, "notify_DllMain");
    if (pDllMain)
    {
        return pDllMain(hDllHandle, dwReason, lpvReserved);
    }
    OutputDebugStringA("WARNING: load_notifications.dll loaded from a process without notify_DllMain\n");
    return TRUE;
}
