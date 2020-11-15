/*++

Copyright (C) Microsoft Corporation. All rights reserved.

Module Name:

    mmc.h

Abstract:

    Functions/macros for using the scratch buffer and scratch request/SRB
    in the serial I/O queue context

Author:

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#ifndef __SCRATCH_H__
#define __SCRATCH_H__


_IRQL_requires_max_(APC_LEVEL)
VOID
ScratchBuffer_Deallocate(
    _Inout_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

_IRQL_requires_max_(APC_LEVEL)
BOOLEAN
ScratchBuffer_Allocate(
    _Inout_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

VOID
ScratchBuffer_ResetItems(
    _Inout_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_ BOOLEAN                 ResetRequestHistory
    );

_IRQL_requires_max_(APC_LEVEL)
VOID
ScratchBuffer_SetupSrb(
    _Inout_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_opt_ WDFREQUEST             OriginalRequest,
    _In_ ULONG                      MaximumTransferLength,
    _In_ BOOLEAN                    GetDataFromDevice
    );

VOID
ScratchBuffer_SetupReadWriteSrb(
    _Inout_ PCDROM_DEVICE_EXTENSION     DeviceExtension,
    _In_    WDFREQUEST                  OriginalRequest,
    _In_    LARGE_INTEGER               StartingOffset,
    _In_    ULONG                       RequiredLength,
    _Inout_updates_bytes_(RequiredLength) UCHAR* DataBuffer,
    _In_    BOOLEAN                     IsReadRequest,
    _In_    BOOLEAN                     UsePartialMdl
    );

NTSTATUS
ScratchBuffer_SendSrb(
    _Inout_     PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_        BOOLEAN                 SynchronousSrb,
    _When_(SynchronousSrb, _Pre_null_)
    _When_(!SynchronousSrb, _In_opt_)
                PSRB_HISTORY_ITEM       *SrbHistoryItem
    );

NTSTATUS
ScratchBuffer_PerformNextReadWrite(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_ BOOLEAN                  FirstTry
    );

#if DBG
    #define ScratchBuffer_BeginUse(context) ScratchBuffer_BeginUseX((context), __FILE__, __LINE__)
#else
    #define ScratchBuffer_BeginUse(context) ScratchBuffer_BeginUseX((context), NULL, (ULONG)-1)
#endif

__inline VOID ScratchBuffer_BeginUseX(_Inout_ PCDROM_DEVICE_EXTENSION DeviceExtension, _In_opt_ LPCSTR File, ULONG Line)
{
    // NOTE: these are not "real" locks.  They are simply to help
    //       avoid multiple uses of the scratch buffer. Thus, it
    //       is not critical to have atomic operations here.
    PVOID tmp = InterlockedCompareExchangePointer((PVOID)&(DeviceExtension->ScratchContext.ScratchInUse), (PVOID)-1, NULL);
    NT_ASSERT(tmp == NULL);
    UNREFERENCED_PARAMETER(tmp); //defensive coding, avoid PREFAST warning.
    DeviceExtension->ScratchContext.ScratchInUseFileName = File;
    DeviceExtension->ScratchContext.ScratchInUseLineNumber = Line;
    ScratchBuffer_ResetItems(DeviceExtension, TRUE);
    RequestClearSendTime(DeviceExtension->ScratchContext.ScratchRequest);
    return;
}
__inline VOID ScratchBuffer_EndUse(_Inout_ PCDROM_DEVICE_EXTENSION DeviceExtension)
{
    // NOTE: these are not "real" locks.  They are simply to help
    //       avoid multiple uses of the scratch buffer.  Thus, it
    //       is not critical to have atomic operations here.

    // On lock release, we erase ScratchInUseFileName and ScratchInUseLineNumber _before_ releasing ScratchInUse,
    // because otherwise we may erase these after the lock has been acquired again by another thread. We store the
    // old values of ScratchInUseFileName and ScratchInUseLineNumber in local variables to facilitate debugging,
    // if the ASSERT at the end of the function is hit.
    PCSTR  scratchInUseFileName;
    ULONG  scratchInUseLineNumber;
    PVOID  tmp;

    scratchInUseFileName = DeviceExtension->ScratchContext.ScratchInUseFileName;
    scratchInUseLineNumber = DeviceExtension->ScratchContext.ScratchInUseLineNumber;
    UNREFERENCED_PARAMETER(scratchInUseFileName);
    UNREFERENCED_PARAMETER(scratchInUseLineNumber);
    DeviceExtension->ScratchContext.ScratchInUseFileName = NULL;
    DeviceExtension->ScratchContext.ScratchInUseLineNumber = 0;

    //
    // If we have used the PartialMdl in the scratch context we should notify MM that we will be reusing it
    // otherwise it may leak System VA if some one below us has mapped the same.
    //

    if (DeviceExtension->ScratchContext.PartialMdlIsBuilt != FALSE)
    {
        MmPrepareMdlForReuse(DeviceExtension->ScratchContext.PartialMdl);
        DeviceExtension->ScratchContext.PartialMdlIsBuilt = FALSE;
    }

    tmp = InterlockedCompareExchangePointer((PVOID)&(DeviceExtension->ScratchContext.ScratchInUse), NULL, (PVOID)-1);
    NT_ASSERT(tmp == ((PVOID)-1));
    UNREFERENCED_PARAMETER(tmp); //defensive coding, avoid PREFAST warning.
    return;
}

VOID
CompressSrbHistoryData(
    _Inout_  PSRB_HISTORY   RequestHistory
    );

VOID
ValidateSrbHistoryDataPresumptions(
    _In_     SRB_HISTORY const* RequestHistory
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
ScratchBuffer_ExecuteCdbEx(
    _Inout_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_opt_ WDFREQUEST             OriginalRequest,
    _In_ ULONG                      TransferSize,
    _In_ BOOLEAN                    GetDataFromDevice,
    _In_ PCDB                       Cdb,
    _In_ UCHAR                      OprationLength,
    _In_ ULONG                      TimeoutValue
    );

_IRQL_requires_max_(APC_LEVEL)
__inline
NTSTATUS
ScratchBuffer_ExecuteCdb(
    _Inout_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_opt_ WDFREQUEST             OriginalRequest,
    _In_ ULONG                      TransferSize,
    _In_ BOOLEAN                    GetDataFromDevice,
    _In_ PCDB                       Cdb,
    _In_ UCHAR                      OprationLength
    )
{
    return ScratchBuffer_ExecuteCdbEx(DeviceExtension,
                                      OriginalRequest,
                                      TransferSize,
                                      GetDataFromDevice,
                                      Cdb,
                                      OprationLength,
                                      0);
}

KDEFERRED_ROUTINE ScratchBuffer_ReadWriteTimerRoutine;

#endif //__SCRATCH_H__
