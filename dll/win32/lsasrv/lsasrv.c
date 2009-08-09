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
    DWORD dwError;

    TRACE("LsapInitLsa()\n");

    /* Start the RPC server */
    LsarStartRpcServer();

    /* Notify the service manager */
    hEvent = CreateEventW(NULL,
                          TRUE,
                          FALSE,
                          L"LSA_RPC_SERVER_ACTIVE");
    if (hEvent == NULL)
    {
        dwError = GetLastError();
        TRACE("Failed to create the notication event (Error %lu)\n", dwError);

        if (dwError == ERROR_ALREADY_EXISTS)
        {
            hEvent = OpenEventW(GENERIC_WRITE,
                                FALSE,
                                L"LSA_RPC_SERVER_ACTIVE");
            if (hEvent != NULL)
            {
               ERR("Could not open the notification event!");
            }
        }
    }

    SetEvent(hEvent);

    /* NOTE: Do not close the event handle!!!! */

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
