#include "k32_vista.h"

extern BOOL WINAPI DECLSPEC_HOTPATCH TpSimpleTryPost( PTP_SIMPLE_CALLBACK callback, PVOID userdata,
                                                           TP_CALLBACK_ENVIRON *environment );

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
