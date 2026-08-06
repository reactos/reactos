#include "k32_vista.h"

extern BOOL WINAPI DECLSPEC_HOTPATCH TpSimpleTryPost( PTP_SIMPLE_CALLBACK callback, PVOID userdata,
                                                           TP_CALLBACK_ENVIRON *environment );

extern NTSTATUS WINAPI TpAllocWork( TP_WORK **out, PTP_WORK_CALLBACK callback, PVOID userdata,
                                    TP_CALLBACK_ENVIRON *environment );
extern VOID WINAPI TpPostWork( TP_WORK *work );
extern VOID WINAPI TpReleaseWork( TP_WORK *work );
extern VOID WINAPI TpWaitForWork( TP_WORK *work, BOOL cancel_pending );

/***********************************************************************
 *           TrySubmitThreadpoolCallback   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH TrySubmitThreadpoolCallback( PTP_SIMPLE_CALLBACK callback, PVOID userdata,
                                                           TP_CALLBACK_ENVIRON *environment )
{
    NTSTATUS status = TpSimpleTryPost( callback, userdata, environment );
    if (!NT_SUCCESS(status))
    {
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *           CreateThreadpoolWork   (kernelbase.@)
 */
PTP_WORK WINAPI DECLSPEC_HOTPATCH CreateThreadpoolWork( PTP_WORK_CALLBACK callback, PVOID userdata,
                                                        TP_CALLBACK_ENVIRON *environment )
{
    TP_WORK *work;
    NTSTATUS status = TpAllocWork( &work, callback, userdata, environment );
    if (!NT_SUCCESS(status))
    {
        SetLastError(RtlNtStatusToDosError(status));
        return NULL;
    }

    return work;
}

/***********************************************************************
 *           SubmitThreadpoolWork   (kernelbase.@)
 */
VOID WINAPI DECLSPEC_HOTPATCH SubmitThreadpoolWork( TP_WORK *work )
{
    TpPostWork( work );
}

/***********************************************************************
 *           CloseThreadpoolWork   (kernelbase.@)
 */
VOID WINAPI DECLSPEC_HOTPATCH CloseThreadpoolWork( TP_WORK *work )
{
    TpReleaseWork( work );
}

/***********************************************************************
 *           WaitForThreadpoolWorkCallbacks   (kernelbase.@)
 */
VOID WINAPI DECLSPEC_HOTPATCH WaitForThreadpoolWorkCallbacks( TP_WORK *work, BOOL cancel_pending )
{
    TpWaitForWork( work, cancel_pending );
}
