/* INCLUDES ****************************************************************/

#define WIN32_NO_STATUS
#include <windows.h>
#include <ntsecapi.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#include "lsa_s.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS *****************************************************************/

/* VARIABLES ***************************************************************/


/* FUNCTIONS ***************************************************************/

VOID
LsarStartRpcServer(VOID)
{
    RPC_STATUS Status;

    DPRINT("LsarStartRpcServer() called");

    Status = RpcServerUseProtseqEpW(L"ncacn_np",
                                    10,
                                    L"\\pipe\\lsarpc",
                                    NULL);
    if (Status != RPC_S_OK)
    {
        DPRINT1("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);
        return;
    }

    Status = RpcServerRegisterIf(lsarpc_ServerIfHandle,
                                 NULL,
                                 NULL);
    if (Status != RPC_S_OK)
    {
        DPRINT1("RpcServerRegisterIf() failed (Status %lx)\n", Status);
        return;
    }

    Status = RpcServerListen(1, 20, TRUE);
    if (Status != RPC_S_OK)
    {
        DPRINT1("RpcServerListen() failed (Status %lx)\n", Status);
        return;
    }

    DPRINT("LsarStartRpcServer() done");
}

/* Function 0 */
unsigned int
LsarClose(IN handle_t BindingHandle,
          IN unsigned long ObjectHandle)
{
    DPRINT1("LsarClose(0x%p) called!\n", ObjectHandle);
    return STATUS_INVALID_HANDLE;
}

/* EOF */
