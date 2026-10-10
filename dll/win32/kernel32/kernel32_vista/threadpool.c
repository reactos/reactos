#include "k32_vista.h"

typedef void (CALLBACK *PTP_IO_CALLBACK)(PTP_CALLBACK_INSTANCE,void*,void*,IO_STATUS_BLOCK*,PTP_IO);

extern BOOL WINAPI DECLSPEC_HOTPATCH TpSimpleTryPost( PTP_SIMPLE_CALLBACK callback, PVOID userdata,
                                                           TP_CALLBACK_ENVIRON *environment );

extern NTSTATUS WINAPI TpAllocWork( TP_WORK **out, PTP_WORK_CALLBACK callback, PVOID userdata,
                                    TP_CALLBACK_ENVIRON *environment );
extern NTSTATUS WINAPI TpAllocCleanupGroup(TP_CLEANUP_GROUP** Group);
extern NTSTATUS WINAPI TpAllocWait(TP_WAIT** out, PTP_WAIT_CALLBACK callback, PVOID userdata, TP_CALLBACK_ENVIRON* environment);
extern NTSTATUS WINAPI TpAllocTimer(TP_TIMER** out, PTP_TIMER_CALLBACK callback, PVOID userdata, TP_CALLBACK_ENVIRON* environment);
extern NTSTATUS WINAPI TpAllocPool(TP_POOL** out,PVOID reserved);
extern NTSTATUS WINAPI TpAllocIoCompletion(TP_IO **out, HANDLE file, PTP_IO_CALLBACK callback,
                                     void *userdata, TP_CALLBACK_ENVIRON *environment);

/***********************************************************************
 *           CreateThreadpoolTimer   (kernelbase.@)
 */
PTP_TIMER WINAPI DECLSPEC_HOTPATCH CreateThreadpoolTimer( PTP_TIMER_CALLBACK callback, PVOID userdata,
                                                          TP_CALLBACK_ENVIRON *environment )
{
    TP_TIMER *timer;

    NTSTATUS status = TpAllocTimer( &timer, callback, userdata, environment );
    if (!NT_SUCCESS(status))
    {
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }

    return timer;
}

static void WINAPI tp_io_callback( TP_CALLBACK_INSTANCE *instance, void *userdata, void *cvalue, IO_STATUS_BLOCK *iosb, TP_IO *io )
{
    PTP_WIN32_IO_CALLBACK callback = *(void **)io;
    callback( instance, userdata, cvalue, RtlNtStatusToDosError( iosb->Status ), iosb->Information, io );
}

/***********************************************************************
 *           CreateThreadpoolIo   (kernelbase.@)
 */
PTP_IO WINAPI DECLSPEC_HOTPATCH CreateThreadpoolIo( HANDLE handle, PTP_WIN32_IO_CALLBACK callback,
                                                    PVOID userdata, TP_CALLBACK_ENVIRON *environment )
{
    TP_IO *io;

    NTSTATUS status = TpAllocIoCompletion( &io, handle, tp_io_callback, userdata, environment );
    *(void **)io = callback; /* ntdll leaves us space to store our callback at the beginning of TP_IO struct */
    if (!NT_SUCCESS(status))
    {
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }

    return io;
}

/***********************************************************************
 *           CreateThreadpoolWait   (kernelbase.@)
 */
PTP_WAIT WINAPI DECLSPEC_HOTPATCH CreateThreadpoolWait( PTP_WAIT_CALLBACK callback, PVOID userdata,
                                                       TP_CALLBACK_ENVIRON *environment )
{
    TP_WAIT *wait;

    NTSTATUS status = TpAllocWait( &wait, callback, userdata, environment );
    if (!NT_SUCCESS(status))
    {
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }

    return wait;
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
        return FALSE;
    }
    return work;
}

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
 *           CreateThreadpoolCleanupGroup   (kernelbase.@)
 */
PTP_CLEANUP_GROUP WINAPI DECLSPEC_HOTPATCH CreateThreadpoolCleanupGroup(void)
{
    TP_CLEANUP_GROUP *group;
    NTSTATUS status = TpAllocCleanupGroup(&group);

    if (!NT_SUCCESS(status))
    {
        SetLastError(RtlNtStatusToDosError(status));
        return NULL;
    }

    return group;
}

/***********************************************************************
 *           CreateThreadpool   (kernelbase.@)
 */
PTP_POOL WINAPI DECLSPEC_HOTPATCH CreateThreadpool(void *reserved)
{
    TP_POOL *pool = NULL;
    NTSTATUS status = TpAllocPool(&pool, reserved);

    if (!NT_SUCCESS(status))
    {
        SetLastError(RtlNtStatusToDosError(status));
        return NULL;
    }
    return pool;
}
