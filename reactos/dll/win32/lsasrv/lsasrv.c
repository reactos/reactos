#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

DECLARE_HANDLE(LSAPR_HANDLE);

#define NDEBUG
#include <debug.h>

VOID LsarStartRpcServer(VOID);


NTSTATUS WINAPI
LsapInitLsa(VOID)
{
    HANDLE hEvent;

    DPRINT("LsapInitLsa() called\n");

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
