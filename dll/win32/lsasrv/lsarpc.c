/* INCLUDES ****************************************************************/

#define WIN32_NO_STATUS
#include <windows.h>
#include <ntsecapi.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#include "lsa_s.h"

#define NDEBUG
#include <debug.h>

#define POLICY_DELETE (RTL_HANDLE_VALID << 1)
typedef struct _LSAR_POLICY_HANDLE
{
    ULONG Flags;
    LONG RefCount;
    ACCESS_MASK AccessGranted;
} LSAR_POLICY_HANDLE, *PLSAR_POLICY_HANDLE;

static RTL_CRITICAL_SECTION PolicyHandleTableLock;
static RTL_HANDLE_TABLE PolicyHandleTable;

/* FUNCTIONS ***************************************************************/

static NTSTATUS
ReferencePolicyHandle(IN LSA_HANDLE ObjectHandle,
                      IN ACCESS_MASK DesiredAccess,
                      OUT PLSAR_POLICY_HANDLE *Policy)
{
    PLSAR_POLICY_HANDLE ReferencedPolicy;
    NTSTATUS Status = STATUS_SUCCESS;

    RtlEnterCriticalSection(&PolicyHandleTableLock);

    if (RtlIsValidIndexHandle(&PolicyHandleTable,
                              (ULONG)ObjectHandle,
                              (PRTL_HANDLE_TABLE_ENTRY*)&ReferencedPolicy) &&
        !(ReferencedPolicy->Flags & POLICY_DELETE))
    {
        if (RtlAreAllAccessesGranted(ReferencedPolicy->AccessGranted,
                                     DesiredAccess))
        {
            ReferencedPolicy->RefCount++;
            *Policy = ReferencedPolicy;
        }
        else
            Status = STATUS_ACCESS_DENIED;
    }
    else
        Status = STATUS_INVALID_HANDLE;

    RtlLeaveCriticalSection(&PolicyHandleTableLock);

    return Status;
}

static VOID
DereferencePolicyHandle(IN OUT PLSAR_POLICY_HANDLE Policy,
                        IN BOOLEAN Delete)
{
    RtlEnterCriticalSection(&PolicyHandleTableLock);

    if (Delete)
    {
        Policy->Flags |= POLICY_DELETE;
        Policy->RefCount--;

        ASSERT(Policy->RefCount != 0);
    }

    if (--Policy->RefCount == 0)
    {
        ASSERT(Policy->Flags & POLICY_DELETE);
        RtlFreeHandle(&PolicyHandleTable,
                      (PRTL_HANDLE_TABLE_ENTRY)Policy);
    }

    RtlLeaveCriticalSection(&PolicyHandleTableLock);
}

VOID
LsarStartRpcServer(VOID)
{
    RPC_STATUS Status;

    RtlInitializeCriticalSection(&PolicyHandleTableLock);
    RtlInitializeHandleTable(0x1000,
                             sizeof(LSAR_POLICY_HANDLE),
                             &PolicyHandleTable);

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
NTSTATUS
LsarClose(IN handle_t BindingHandle,
          IN unsigned long ObjectHandle)
{
    PLSAR_POLICY_HANDLE Policy = NULL;
    NTSTATUS Status;

    DPRINT1("LsarClose(0x%p) called!\n", ObjectHandle);

    Status = ReferencePolicyHandle((LSA_HANDLE)ObjectHandle,
                                   0,
                                   &Policy);
    if (NT_SUCCESS(Status))
    {
        /* delete the handle */
        DereferencePolicyHandle(Policy,
                                TRUE);
    }

    return Status;
}

/* Function 1 */
NTSTATUS
LsarDelete(IN handle_t BindingHandle,
           IN unsigned long ObjectHandle)
{
    DPRINT1("LsarDelete(0x%p) UNIMPLEMENTED!\n", ObjectHandle);
    return STATUS_ACCESS_DENIED;
}

/* EOF */
