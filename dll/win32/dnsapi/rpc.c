#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <dnsrslvr_c.h>

#define NDEBUG
#include <debug.h>

handle_t __RPC_USER
DNSRSLVR_HANDLE_bind(DNSRSLVR_HANDLE pszMachineName)
{
    handle_t hBinding = NULL;
    LPWSTR pszStringBinding;
    RPC_STATUS Status;

    DPRINT("DNSRSLVR_HANDLE_bind(%S)\n", pszMachineName);

    Status = RpcStringBindingComposeW(NULL,
                                      L"ncalrpc",
                                      pszMachineName,
                                      L"DNSResolver",
                                      NULL,
                                      &pszStringBinding);
    if (Status != RPC_S_OK)
    {
        DPRINT1("RpcStringBindingCompose returned 0x%x\n", Status);
        return NULL;
    }

    /* Set the binding handle that will be used to bind to the server. */
    Status = RpcBindingFromStringBindingW(pszStringBinding,
                                          &hBinding);
    if (Status != RPC_S_OK)
    {
        DPRINT1("RpcBindingFromStringBinding returned 0x%x\n", Status);
    }

    Status = RpcStringFreeW(&pszStringBinding);
    if (Status != RPC_S_OK)
    {
        DPRINT1("RpcStringFree returned 0x%x\n", Status);
    }

    return hBinding;
}

void __RPC_USER
DNSRSLVR_HANDLE_unbind(DNSRSLVR_HANDLE pszMachineName,
                       handle_t hBinding)
{
    RPC_STATUS Status;

    DPRINT("DNSRSLVR_HANDLE_unbind(%S)\n", pszMachineName);

    Status = RpcBindingFree(&hBinding);
    if (Status != RPC_S_OK)
    {
        DPRINT1("RpcBindingFree returned 0x%x\n", Status);
    }
}

void __RPC_FAR * __RPC_USER
midl_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}

void __RPC_USER
midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}
