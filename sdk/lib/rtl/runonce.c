
/* Taken from Wine ntdll/sync.c */

#include <rtl_vista.h>
#include <wine/config.h>
#include <wine/port.h>

/******************************************************************
 *              RtlRunOnceInitialize (NTDLL.@)
 */
VOID NTAPI RtlRunOnceInitialize(_Out_ PRTL_RUN_ONCE RunOnce)
{
    RunOnce->Ptr = NULL;
}

/******************************************************************
 *              RtlRunOnceBeginInitialize (NTDLL.@)
 */
_Must_inspect_result_
NTSTATUS
NTAPI
RtlRunOnceBeginInitialize(
    _Inout_ PRTL_RUN_ONCE RunOnce,
    _In_ ULONG Flags,
    _Outptr_opt_result_maybenull_ PVOID *Context)
{
    if (Flags & RTL_RUN_ONCE_CHECK_ONLY)
    {
        ULONG_PTR val = (ULONG_PTR)RunOnce->Ptr;

        if (Flags & RTL_RUN_ONCE_ASYNC) return STATUS_INVALID_PARAMETER;
        if ((val & 3) != 2) return STATUS_UNSUCCESSFUL;
        if (Context) *Context = (void *)(val & ~3);
        return STATUS_SUCCESS;
    }

    for (;;)
    {
        ULONG_PTR next, val = (ULONG_PTR)RunOnce->Ptr;

        switch (val & 3)
        {
        case 0:  /* first time */
            if (!interlocked_cmpxchg_ptr( &RunOnce->Ptr,
                                          (Flags & RTL_RUN_ONCE_ASYNC) ? (void *)3 : (void *)1, 0))
                return STATUS_PENDING;
            break;

        case 1:  /* in progress, wait */
            if (Flags & RTL_RUN_ONCE_ASYNC) return STATUS_INVALID_PARAMETER;
            next = val & ~3;
            if (interlocked_cmpxchg_ptr( &RunOnce->Ptr, (void *)((ULONG_PTR)&next | 1),
                                         (void *)val ) == (void *)val)
                NtWaitForKeyedEvent( 0, &next, FALSE, NULL );
            break;

        case 2:  /* done */
            if (Context) *Context = (void *)(val & ~3);
            return STATUS_SUCCESS;

        case 3:  /* in progress, async */
            if (!(Flags & RTL_RUN_ONCE_ASYNC)) return STATUS_INVALID_PARAMETER;
            return STATUS_PENDING;
        }
    }
}

/******************************************************************
 *              RtlRunOnceComplete (NTDLL.@)
 */
NTSTATUS
NTAPI
RtlRunOnceComplete(
    _Inout_ PRTL_RUN_ONCE RunOnce,
    _In_ ULONG Flags,
    _In_opt_ PVOID Context)
{
    if ((ULONG_PTR)Context & 3) return STATUS_INVALID_PARAMETER;

    if (Flags & RTL_RUN_ONCE_INIT_FAILED)
    {
        if (Context) return STATUS_INVALID_PARAMETER;
        if (Flags & RTL_RUN_ONCE_ASYNC) return STATUS_INVALID_PARAMETER;
    }
    else Context = (void *)((ULONG_PTR)Context | 2);

    for (;;)
    {
        ULONG_PTR val = (ULONG_PTR)RunOnce->Ptr;

        switch (val & 3)
        {
        case 1:  /* in progress */
            if (interlocked_cmpxchg_ptr( &RunOnce->Ptr, Context, (void *)val ) != (void *)val) break;
            val &= ~3;
            while (val)
            {
                ULONG_PTR next = *(ULONG_PTR *)val;
                NtReleaseKeyedEvent( 0, (void *)val, FALSE, NULL );
                val = next;
            }
            return STATUS_SUCCESS;

        case 3:  /* in progress, async */
            if (!(Flags & RTL_RUN_ONCE_ASYNC)) return STATUS_INVALID_PARAMETER;
            if (interlocked_cmpxchg_ptr( &RunOnce->Ptr, Context, (void *)val) != (void *)val) break;
            return STATUS_SUCCESS;

        default:
            return STATUS_UNSUCCESSFUL;
        }
    }
}

/******************************************************************
 *              RtlRunOnceExecuteOnce (NTDLL.@)
 */
_Maybe_raises_SEH_exception_
NTSTATUS
NTAPI
RtlRunOnceExecuteOnce(
    _Inout_ PRTL_RUN_ONCE RunOnce,
    _In_ __inner_callback PRTL_RUN_ONCE_INIT_FN InitFn,
    _Inout_opt_ PVOID Parameter,
    _Outptr_opt_result_maybenull_ PVOID *Context)
{
    DWORD ret = RtlRunOnceBeginInitialize( RunOnce, 0, Context );

    if (ret != STATUS_PENDING) return ret;

    if (!InitFn( RunOnce, Parameter, Context ))
    {
        RtlRunOnceComplete( RunOnce, RTL_RUN_ONCE_INIT_FAILED, NULL );
        return STATUS_UNSUCCESSFUL;
    }

    return RtlRunOnceComplete( RunOnce, 0, Context ? *Context : NULL );
}
