/* rpc.c */

#include <windows.h>
#include <rpc.h>
#include <rpcdce.h>


static RPC_BINDING_HANDLE LocalBindingHandle = NULL;


RPC_STATUS
PnpBindRpc(LPWSTR pszMachine,
           RPC_BINDING_HANDLE* BindingHandle)
{
  PWSTR pszStringBinding = NULL;
  RPC_STATUS Status;

  Status = RpcStringBindingComposeW(NULL,
				    L"ncacn_np",
				    pszMachine,
				    L"\\pipe\\umpnpmgr",
				    NULL,
				    &pszStringBinding);
  if (Status != RPC_S_OK)
    return Status;

  Status = RpcBindingFromStringBindingW(pszStringBinding,
					BindingHandle);

  RpcStringFreeW(&pszStringBinding);

  return Status;
}


RPC_STATUS
PnpUnbindRpc(RPC_BINDING_HANDLE *BindingHandle)
{
  if (BindingHandle != NULL)
  {
    RpcBindingFree(*BindingHandle);
    *BindingHandle = NULL;
  }

  return RPC_S_OK;
}


RPC_STATUS
PnpGetLocalBindingHandle(RPC_BINDING_HANDLE *BindingHandle)
{
  if (LocalBindingHandle != NULL)
  {
    BindingHandle = LocalBindingHandle;
    return RPC_S_OK;
  }

  return PnpBindRpc(NULL, BindingHandle);
}


RPC_STATUS
PnpUnbindLocalBindingHandle(VOID)
{
  return PnpUnbindRpc(&LocalBindingHandle);
}


void __RPC_FAR * __RPC_USER
midl_user_allocate(size_t len)
{
  return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER
midl_user_free(void __RPC_FAR * ptr)
{
  HeapFree(GetProcessHeap(), 0, ptr);
}

/* EOF */
