#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#include "lsasrv.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(lsasrv);


NTSTATUS WINAPI
LsapInitLsa(VOID)
{
    HANDLE hEvent;

    TRACE("LsapInitLsa()\n");

    LsarStartRpcServer();

    hEvent = OpenEventW(EVENT_MODIFY_STATE,
                        FALSE,
                        L"Global\\SECURITY_SERVICES_STARTED");
    if (hEvent != NULL)
    {
        SetEvent(hEvent);
        CloseHandle(hEvent);
    }
    return STATUS_SUCCESS;
}


void __RPC_FAR * __RPC_USER midl_user_allocate(size_t len)
{
    return RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    RtlFreeHeap(RtlGetProcessHeap(), 0, ptr);
}

/* EOF */
