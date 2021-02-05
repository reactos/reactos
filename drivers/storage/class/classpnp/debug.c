/*++

Copyright (C) Microsoft Corporation, 1991 - 2010

Module Name:

    debug.c

Abstract:

    CLASSPNP debug code and data

Environment:

    kernel mode only

Notes:


Revision History:

--*/


#include "classp.h"
#include "debug.h"

#ifdef DEBUG_USE_WPP
#include "debug.tmh"
#endif

#if DBG

    //
    // default to not breaking in for lost irps, five minutes before we even
    // bother checking for lost irps, using standard debug print macros, and
    // using a 64k debug print buffer
    //

    #ifndef     CLASS_GLOBAL_BREAK_ON_LOST_IRPS
        #error "CLASS_GLOBAL_BREAK_ON_LOST_IRPS undefined"
        #define CLASS_GLOBAL_BREAK_ON_LOST_IRPS 0
    #endif   // CLASS_GLOBAL_BREAK_ON_LOST_IRPS

    #ifndef     CLASS_GLOBAL_SECONDS_TO_WAIT_FOR_SYNCHRONOUS_SRB
        #error "CLASS_GLOBAL_SECONDS_TO_WAIT_FOR_SYNCHRONOUS_SRB undefined"
        #define CLASS_GLOBAL_SECONDS_TO_WAIT_FOR_SYNCHRONOUS_SRB 300
    #endif   // CLASS_GLOBAL_SECONDS_TO_WAIT_FOR_SYNCHRONOUS_SRB

    #ifndef     CLASS_GLOBAL_BUFFERED_DEBUG_PRINT
        #error "CLASS_GLOBAL_BUFFERED_DEBUG_PRINT undefined"
        #define CLASS_GLOBAL_BUFFERED_DEBUG_PRINT 0
    #endif   // CLASS_GLOBAL_BUFFERED_DEBUG_PRINT

    #ifndef     CLASS_GLOBAL_BUFFERED_DEBUG_PRINT_BUFFER_SIZE
        #error "CLASS_GLOBAL_BUFFERED_DEBUG_PRINT_BUFFER_SIZE undefined"
        #define CLASS_GLOBAL_BUFFERED_DEBUG_PRINT_BUFFER_SIZE 512
    #endif   // CLASS_GLOBAL_BUFFERED_DEBUG_PRINT_BUFFER_SIZE

    #ifndef     CLASS_GLOBAL_BUFFERED_DEBUG_PRINT_BUFFERS
        #error "CLASS_GLOBAL_BUFFERED_DEBUG_PRINT_BUFFERS undefined"
        #define CLASS_GLOBAL_BUFFERED_DEBUG_PRINT_BUFFERS 512
    #endif   // CLASS_GLOBAL_BUFFERED_DEBUG_PRINT_BUFFERS

#ifdef _MSC_VER
    #pragma data_seg("NONPAGE")
#endif



    CLASSPNP_GLOBALS ClasspnpGlobals;

    //
    // the low sixteen bits are used to see if the debug level is high enough
    // the high sixteen bits are used to singly enable debug levels 1-16
    //
    LONG ClassDebug = 0x00000000;

    BOOLEAN DebugTrapOnWarn = FALSE;

    //
    // Used to track callers when we receive an access and the disk
    // is powered down.
    //
    ULONG DiskSpinupIndex = 0;
    DISK_SPINUP_TRACES DiskSpinupTraces[NUMBER_OF_DISK_SPINUP_TRACES];

    VOID ClasspInitializeDebugGlobals()
    {
        KIRQL irql;

        if (InterlockedCompareExchange(&ClasspnpGlobals.Initializing, 1, 0) == 0) {

            KeInitializeSpinLock(&ClasspnpGlobals.SpinLock);

            KeAcquireSpinLock(&ClasspnpGlobals.SpinLock, &irql);

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT, "CLASSPNP.SYS => Initializing ClasspnpGlobals...\n"));

            ClasspnpGlobals.Buffer = NULL;
            ClasspnpGlobals.Index = (ULONG)-1;
            ClasspnpGlobals.BreakOnLostIrps = CLASS_GLOBAL_BREAK_ON_LOST_IRPS;
            ClasspnpGlobals.EachBufferSize = CLASS_GLOBAL_BUFFERED_DEBUG_PRINT_BUFFER_SIZE;
            ClasspnpGlobals.NumberOfBuffers = CLASS_GLOBAL_BUFFERED_DEBUG_PRINT_BUFFERS;
            ClasspnpGlobals.SecondsToWaitForIrps = CLASS_GLOBAL_SECONDS_TO_WAIT_FOR_SYNCHRONOUS_SRB;

            //
            // this should be the last item set
            //

            ClasspnpGlobals.UseBufferedDebugPrint = CLASS_GLOBAL_BUFFERED_DEBUG_PRINT;

            KeReleaseSpinLock(&ClasspnpGlobals.SpinLock, irql);

            InterlockedExchange(&ClasspnpGlobals.Initialized, 1);

        }
    }

    /*++////////////////////////////////////////////////////////////////////////////

    ClassDebugPrint()

    Routine Description:

        Debug print for all class drivers, NOOP on FRE versions.
        Allows printing to a debug buffer (with auto fallback to kdprint) by
        properly setting the Globals in classpnp on CHK versions.

    Arguments:

        Debug print level, or from 0 to 3 for legacy drivers.

    Return Value:

        None

    --*/
    VOID ClassDebugPrint(_In_ CLASS_DEBUG_LEVEL DebugPrintLevel, _In_z_ PCCHAR DebugMessage, ...)
    {
        va_list ap;
        va_start(ap, DebugMessage);

        if ((DebugPrintLevel <= (ClassDebug & 0x0000ffff)) ||
            ((1 << (DebugPrintLevel + 15)) & ClassDebug)) {

            if (ClasspnpGlobals.UseBufferedDebugPrint &&
                ClasspnpGlobals.Buffer == NULL) {

                //
                // this double-check prevents always taking
                // a spinlock just to ensure we have a buffer
                //

                KIRQL irql;

                KeAcquireSpinLock(&ClasspnpGlobals.SpinLock, &irql);
                if (ClasspnpGlobals.Buffer == NULL) {

                    SIZE_T bufferSize;
                    if (NT_SUCCESS(
                            RtlSIZETMult(ClasspnpGlobals.NumberOfBuffers,
                                         ClasspnpGlobals.EachBufferSize,
                                         &bufferSize))) {

                        DbgPrintEx(DPFLTR_CLASSPNP_ID, DPFLTR_ERROR_LEVEL,
                                   "ClassDebugPrint: Allocating %x bytes for "
                                   "classdebugprint buffer\n", (ULONG)bufferSize);
                        ClasspnpGlobals.Index = (ULONG)-1;
                        ClasspnpGlobals.Buffer =
                            ExAllocatePoolWithTag(NonPagedPoolNx, bufferSize, 'bDcS');
                        DbgPrintEx(DPFLTR_CLASSPNP_ID, DPFLTR_ERROR_LEVEL,
                                   "ClassDebugPrint: Allocated buffer at %p\n",
                                   ClasspnpGlobals.Buffer);

                        if (ClasspnpGlobals.Buffer) {
                            RtlZeroMemory(ClasspnpGlobals.Buffer, bufferSize);
                        }
                    }

                }
                KeReleaseSpinLock(&ClasspnpGlobals.SpinLock, irql);

            }

            if (ClasspnpGlobals.UseBufferedDebugPrint &&
                ClasspnpGlobals.Buffer != NULL) {

                //
                // we never free the buffer, so once it exists,
                // we can just print to it with immunity
                //

                ULONG index;
                PUCHAR buffer;
                NTSTATUS status;
                index = InterlockedIncrement((volatile LONG *)&ClasspnpGlobals.Index);
                index %= ClasspnpGlobals.NumberOfBuffers;
                index *= (ULONG)ClasspnpGlobals.EachBufferSize;

                buffer = ClasspnpGlobals.Buffer;
                buffer += index;

                RtlZeroMemory(buffer, ClasspnpGlobals.EachBufferSize);

                status = RtlStringCchVPrintfA((NTSTRSAFE_PSTR)buffer, ClasspnpGlobals.EachBufferSize, DebugMessage, ap);
                if (!NT_SUCCESS(status))
                {
                    *buffer = 0; // force-null on failure
                }

            } else {

                //
                // either we could not allocate a buffer for debug prints
                // or buffered debug prints are disabled
                //

                vDbgPrintEx(DPFLTR_CLASSPNP_ID, DPFLTR_INFO_LEVEL, DebugMessage, ap);

            }

        }

        va_end(ap);

    }


    /*
     *  DbgCheckReturnedPkt
     *
     *      Check a completed TRANSFER_PACKET for all sorts of error conditions
     *      and warn/trap appropriately.
     */
    VOID DbgCheckReturnedPkt(TRANSFER_PACKET *Pkt)
    {
        PCDB pCdb = ClasspTransferPacketGetCdb(Pkt);

        NT_ASSERT(SrbGetOriginalRequest(Pkt->Srb) == Pkt->Irp);
        NT_ASSERT(SrbGetDataBuffer(Pkt->Srb) == Pkt->BufPtrCopy);
        NT_ASSERT(SrbGetDataTransferLength(Pkt->Srb) <= Pkt->BufLenCopy);
        NT_ASSERT(!Pkt->Irp->CancelRoutine);

        if (SRB_STATUS(Pkt->Srb->SrbStatus) == SRB_STATUS_PENDING){
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_RW, "SRB completed with status PENDING in packet %ph: (op=%s srbstat=%s(%xh), irpstat=%xh)",
                        Pkt,
                        DBGGETSCSIOPSTR(Pkt->Srb),
                        DBGGETSRBSTATUSSTR(Pkt->Srb),
                        (ULONG)Pkt->Srb->SrbStatus,
                        Pkt->Irp->IoStatus.Status));
        }
        else if (SRB_STATUS(Pkt->Srb->SrbStatus) == SRB_STATUS_SUCCESS){
            /*
             *  Make sure SRB and IRP status match.
             */
            if (!NT_SUCCESS(Pkt->Irp->IoStatus.Status)){
                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_RW, "SRB and IRP status don't match in packet %ph: (op=%s srbstat=%s(%xh), irpstat=%xh)",
                            Pkt,
                            DBGGETSCSIOPSTR(Pkt->Srb),
                            DBGGETSRBSTATUSSTR(Pkt->Srb),
                            (ULONG)Pkt->Srb->SrbStatus,
                            Pkt->Irp->IoStatus.Status));
            }

            if (Pkt->Irp->IoStatus.Information != SrbGetDataTransferLength(Pkt->Srb)){
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_RW, "SRB and IRP result transfer lengths don't match in succeeded packet %ph: (op=%s, SrbStatus=%s, Srb.DataTransferLength=%xh, Irp->IoStatus.Information=%Ixh).",
                            Pkt,
                            DBGGETSCSIOPSTR(Pkt->Srb),
                            DBGGETSRBSTATUSSTR(Pkt->Srb),
                            SrbGetDataTransferLength(Pkt->Srb),
                            Pkt->Irp->IoStatus.Information));
            }
        }
        else {
            if (NT_SUCCESS(Pkt->Irp->IoStatus.Status)){
                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_RW, "SRB and IRP status don't match in packet %ph: (op=%s srbstat=%s(%xh), irpstat=%xh)",
                            Pkt,
                            DBGGETSCSIOPSTR(Pkt->Srb),
                            DBGGETSRBSTATUSSTR(Pkt->Srb),
                            (ULONG)Pkt->Srb->SrbStatus,
                            Pkt->Irp->IoStatus.Status));
            }
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_RW, "Packet %ph failed (op=%s srbstat=%s(%xh), irpstat=%xh, sense=%s/%s/%s)",
                            Pkt,
                            DBGGETSCSIOPSTR(Pkt->Srb),
                            DBGGETSRBSTATUSSTR(Pkt->Srb),
                            (ULONG)Pkt->Srb->SrbStatus,
                            Pkt->Irp->IoStatus.Status,
                            DBGGETSENSECODESTR(Pkt->Srb),
                            DBGGETADSENSECODESTR(Pkt->Srb),
                            DBGGETADSENSEQUALIFIERSTR(Pkt->Srb)));

            /*
             *  If the SRB failed with underrun or overrun, then the actual
             *  transferred length should be returned in both SRB and IRP.
             *  (SRB's only have an error status for overrun, so it's overloaded).
             */
            if ((SRB_STATUS(Pkt->Srb->SrbStatus) == SRB_STATUS_DATA_OVERRUN) &&
               (Pkt->Irp->IoStatus.Information != SrbGetDataTransferLength(Pkt->Srb))){
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_RW, "SRB and IRP result transfer lengths don't match in failed packet %ph: (op=%s, SrbStatus=%s, Srb.DataTransferLength=%xh, Irp->IoStatus.Information=%Ixh).",
                            Pkt,
                            DBGGETSCSIOPSTR(Pkt->Srb),
                            DBGGETSRBSTATUSSTR(Pkt->Srb),
                            SrbGetDataTransferLength(Pkt->Srb),
                            Pkt->Irp->IoStatus.Information));
            }
        }

        /*
         *  If the port driver returned STATUS_INSUFFICIENT_RESOURCES,
         *  make sure this is also the InternalStatus in the SRB so that we process it correctly.
         */
        if (Pkt->Irp->IoStatus.Status == STATUS_INSUFFICIENT_RESOURCES){
            NT_ASSERT(SRB_STATUS(Pkt->Srb->SrbStatus) == SRB_STATUS_INTERNAL_ERROR);
            NT_ASSERT(SrbGetSystemStatus(Pkt->Srb) == STATUS_INSUFFICIENT_RESOURCES);
        }

        /*
         *  Some miniport drivers have been caught changing the SCSI operation
         *  code in the SRB.  This is absolutely disallowed as it breaks our error handling.
         */
        switch (pCdb->CDB10.OperationCode){
            case SCSIOP_MEDIUM_REMOVAL:
            case SCSIOP_MODE_SENSE:
            case SCSIOP_READ_CAPACITY:
            case SCSIOP_READ:
            case SCSIOP_WRITE:
            case SCSIOP_START_STOP_UNIT:
            case SCSIOP_READ_CAPACITY16:
            case SCSIOP_READ16:
            case SCSIOP_WRITE16:
                break;
            default:
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_RW, "Miniport illegally changed Srb.Cdb.OperationCode in packet %ph failed (op=%s srbstat=%s(%xh), irpstat=%xh, sense=%s/%s/%s)",
                                Pkt,
                                DBGGETSCSIOPSTR(Pkt->Srb),
                                DBGGETSRBSTATUSSTR(Pkt->Srb),
                                (ULONG)Pkt->Srb->SrbStatus,
                                Pkt->Irp->IoStatus.Status,
                                DBGGETSENSECODESTR(Pkt->Srb),
                                DBGGETADSENSECODESTR(Pkt->Srb),
                                DBGGETADSENSEQUALIFIERSTR(Pkt->Srb)));
                break;
        }

    }


    VOID DbgLogSendPacket(TRANSFER_PACKET *Pkt)
    {
        PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Pkt->Fdo->DeviceExtension;
        PCLASS_PRIVATE_FDO_DATA fdoData = fdoExt->PrivateFdoData;
        KIRQL oldIrql;

        if (Pkt->OriginalIrp){
            Pkt->DbgOriginalIrpCopy = *Pkt->OriginalIrp;
            if (Pkt->OriginalIrp->MdlAddress){
                Pkt->DbgMdlCopy = *Pkt->OriginalIrp->MdlAddress;
            }
        }

        KeQueryTickCount(&Pkt->DbgTimeSent);
        Pkt->DbgTimeReturned.QuadPart = 0L;

        KeAcquireSpinLock(&fdoData->SpinLock, &oldIrql);
        fdoData->DbgPacketLogs[fdoData->DbgPacketLogNextIndex] = *Pkt;
        fdoData->DbgPacketLogNextIndex++;
        fdoData->DbgPacketLogNextIndex %= DBG_NUM_PACKET_LOG_ENTRIES;
        KeReleaseSpinLock(&fdoData->SpinLock, oldIrql);
    }

    VOID DbgLogReturnPacket(TRANSFER_PACKET *Pkt)
    {
        PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Pkt->Fdo->DeviceExtension;
        PCLASS_PRIVATE_FDO_DATA fdoData = fdoExt->PrivateFdoData;
        KIRQL oldIrql;

        KeQueryTickCount(&Pkt->DbgTimeReturned);

        #if 0
            // ISSUE: there are some problems with this check (e.g. multiproc), so don't include it yet
            if (Pkt->OriginalIrp){
                /*
                 *  No one should have touched the original irp while the packet was outstanding,
                 *  except for a couple fields that we ourselves update during the transfer
                 *  or that are allowed to change;
                 *  make those couple fields the same and then to a bytewise compare
                 */
                ULONG lenSame;

                Pkt->DbgOriginalIrpCopy.IoStatus.Status = Pkt->OriginalIrp->IoStatus.Status;
                Pkt->DbgOriginalIrpCopy.IoStatus.Information = Pkt->OriginalIrp->IoStatus.Information;
                Pkt->DbgOriginalIrpCopy.Tail.Overlay.DriverContext[0] = Pkt->OriginalIrp->Tail.Overlay.DriverContext[0];
                Pkt->DbgOriginalIrpCopy.ThreadListEntry = Pkt->OriginalIrp->ThreadListEntry;
                Pkt->DbgOriginalIrpCopy.Cancel = Pkt->OriginalIrp->Cancel;

                lenSame = (ULONG)RtlCompareMemory(Pkt->OriginalIrp, &Pkt->DbgOriginalIrpCopy, sizeof(IRP));
                NT_ASSERT(lenSame == sizeof(IRP));
            }
        #endif

        KeAcquireSpinLock(&fdoData->SpinLock, &oldIrql);
        fdoData->DbgPacketLogs[fdoData->DbgPacketLogNextIndex] = *Pkt;
        fdoData->DbgPacketLogNextIndex++;
        fdoData->DbgPacketLogNextIndex %= DBG_NUM_PACKET_LOG_ENTRIES;
        KeReleaseSpinLock(&fdoData->SpinLock, oldIrql);
    }


    /*++////////////////////////////////////////////////////////////////////////////

    DbgSafeInc()

    Routine Description:

        Safely increments a ULONG. If the increment would result in an overflow,
        the value is unchanged.

    Arguments:

        A pointer to the value to be incremented.

    --*/
    static VOID DbgSafeInc(PULONG pValue)
    {
        ULONG incrementResult;
        if(NT_SUCCESS(RtlULongAdd(*pValue, 1, &incrementResult))) {
            *pValue = incrementResult;
        } else {
            //
            // Leave *pValue unchanged (i.e. at ULONG_MAX).
            //
        }
    }

    VOID DbgLogFlushInfo(PCLASS_PRIVATE_FDO_DATA FdoData, BOOLEAN IsIO, BOOLEAN IsFUA, BOOLEAN IsFlush)
    {

        /*
         *  Reset all FUA/Flush logging fields.
         */
        if (FdoData->DbgInitFlushLogging){
            FdoData->DbgNumIORequests = 0;
            FdoData->DbgNumFUAs = 0;
            FdoData->DbgNumFlushes = 0;
            FdoData->DbgIOsSinceFUA = 0;
            FdoData->DbgIOsSinceFlush = 0;
            FdoData->DbgAveIOsToFUA = 0;
            FdoData->DbgAveIOsToFlush = 0;
            FdoData->DbgMaxIOsToFUA = 0;
            FdoData->DbgMaxIOsToFlush = 0;
            FdoData->DbgMinIOsToFUA = 0xffffffff;
            FdoData->DbgMinIOsToFlush = 0xffffffff;
            FdoData->DbgInitFlushLogging = FALSE;
        }

        //
        // Using DbgSafeInc for all increments (instead of ++) guarantees
        // that there will be no overflow hence no division by 0. All counters
        // are capped at ULONG_MAX.
        //

        if (IsIO){
            DbgSafeInc(&FdoData->DbgNumIORequests);
            DbgSafeInc(&FdoData->DbgIOsSinceFlush);
            if (IsFUA){
                if (FdoData->DbgNumFUAs > 0){
                    FdoData->DbgMinIOsToFUA = min(FdoData->DbgMinIOsToFUA, FdoData->DbgIOsSinceFUA);
                }
                DbgSafeInc(&FdoData->DbgNumFUAs);
                FdoData->DbgAveIOsToFUA =  FdoData->DbgNumIORequests/FdoData->DbgNumFUAs;
                FdoData->DbgIOsSinceFUA = 0;
            }
            else {
                DbgSafeInc(&FdoData->DbgIOsSinceFUA);
                FdoData->DbgMaxIOsToFUA = max(FdoData->DbgMaxIOsToFUA, FdoData->DbgIOsSinceFUA);
            }
            FdoData->DbgMaxIOsToFlush = max(FdoData->DbgMaxIOsToFlush, FdoData->DbgIOsSinceFlush);
        }
        else if (IsFlush){
            if (FdoData->DbgNumFlushes > 0){
                FdoData->DbgMinIOsToFlush = min(FdoData->DbgMinIOsToFlush, FdoData->DbgIOsSinceFlush);
            }
            DbgSafeInc(&FdoData->DbgNumFlushes);
            FdoData->DbgAveIOsToFlush =  FdoData->DbgNumIORequests/FdoData->DbgNumFlushes;
            FdoData->DbgIOsSinceFlush = 0;
        }

    }


    /*++////////////////////////////////////////////////////////////////////////////

    SnapDiskStartup()

    Routine Description:

        This function will attempt to record the caller responsible for spinning
        up the disk.

    Arguments:

        NONE.

    Return Value:

        NONE.

    --*/
    VOID
    SnapDiskStartup(
        VOID
    )
    {
        ULONG       Index;
        PDISK_SPINUP_TRACES Entry;
        LARGE_INTEGER   SpinUpTime;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4210) // nonstandard extension used : function given file scope
#endif
        extern NTSYSAPI USHORT NTAPI RtlCaptureStackBackTrace(
           _In_         ULONG FramesToSkip,
           _In_         ULONG FramesToCapture,
           _Out_writes_to_(FramesToCapture, return) PVOID * BackTrace,
           _Out_opt_    PULONG BackTraceHash );
#ifdef _MSC_VER
#pragma warning(pop)
#endif

        //
        // Grab the current count, then mod it so that it
        // becomes an index into the DiskSpinupTraces array.
        //
        Index = InterlockedIncrement( (volatile LONG *)&DiskSpinupIndex );
        Index = Index & (NUMBER_OF_DISK_SPINUP_TRACES - 1);
        Entry = &DiskSpinupTraces[Index];

        //
        // Timestamp the instance.
        //
        KeQueryTickCount(&SpinUpTime);
        SpinUpTime.QuadPart = (SpinUpTime.QuadPart * KeQueryTimeIncrement())/(10000000);


        //
        // Ask the kernel to read back up our stack by
        // DISK_SPINUP_BACKTRACE_LENGTH frames.
        //
        Entry->TimeStamp.QuadPart = SpinUpTime.QuadPart;
        RtlZeroMemory( &Entry->StackTrace[0], DISK_SPINUP_BACKTRACE_LENGTH * sizeof(PVOID) );
        RtlCaptureStackBackTrace( 5,                           // stacks to skip
                                  DISK_SPINUP_BACKTRACE_LENGTH, // buffer size
                                  Entry->StackTrace,
                                  &Index );
    }

#else

    // We have to keep this in the retail build for legacy.
    VOID ClassDebugPrint(_In_ CLASS_DEBUG_LEVEL DebugPrintLevel, _In_z_ PCCHAR DebugMessage, ...)
    {
        UNREFERENCED_PARAMETER(DebugPrintLevel);
        UNREFERENCED_PARAMETER(DebugMessage);
    }

#endif

    char *DbgGetIoctlStr(ULONG ioctl)
    {
        char *ioctlStr = "?";

        switch (ioctl){

            #undef MAKE_CASE
            #define MAKE_CASE(ioctlCode) case ioctlCode: ioctlStr = #ioctlCode; break;

            MAKE_CASE(IOCTL_STORAGE_CHECK_VERIFY)
            MAKE_CASE(IOCTL_STORAGE_CHECK_VERIFY2)
            MAKE_CASE(IOCTL_STORAGE_MEDIA_REMOVAL)
            MAKE_CASE(IOCTL_STORAGE_EJECT_MEDIA)
            MAKE_CASE(IOCTL_STORAGE_LOAD_MEDIA)
            MAKE_CASE(IOCTL_STORAGE_LOAD_MEDIA2)
            MAKE_CASE(IOCTL_STORAGE_RESERVE)
            MAKE_CASE(IOCTL_STORAGE_RELEASE)
            MAKE_CASE(IOCTL_STORAGE_PERSISTENT_RESERVE_IN)
            MAKE_CASE(IOCTL_STORAGE_PERSISTENT_RESERVE_OUT)
            MAKE_CASE(IOCTL_STORAGE_FIND_NEW_DEVICES)
            MAKE_CASE(IOCTL_STORAGE_EJECTION_CONTROL)
            MAKE_CASE(IOCTL_STORAGE_MCN_CONTROL)
            MAKE_CASE(IOCTL_STORAGE_GET_MEDIA_TYPES)
            MAKE_CASE(IOCTL_STORAGE_GET_MEDIA_TYPES_EX)
            MAKE_CASE(IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER)
            MAKE_CASE(IOCTL_STORAGE_GET_HOTPLUG_INFO)
            MAKE_CASE(IOCTL_STORAGE_RESET_BUS)
            MAKE_CASE(IOCTL_STORAGE_RESET_DEVICE)
            MAKE_CASE(IOCTL_STORAGE_GET_DEVICE_NUMBER)
            MAKE_CASE(IOCTL_STORAGE_PREDICT_FAILURE)
            MAKE_CASE(IOCTL_STORAGE_QUERY_PROPERTY)
            MAKE_CASE(OBSOLETE_IOCTL_STORAGE_RESET_BUS)
            MAKE_CASE(OBSOLETE_IOCTL_STORAGE_RESET_DEVICE)
        }

        return ioctlStr;
    }

    char *DbgGetScsiOpStr(PSTORAGE_REQUEST_BLOCK_HEADER Srb)
    {
        PCDB pCdb = SrbGetCdb(Srb);
        char *scsiOpStr = "?";

        if (pCdb) {

            switch (pCdb->CDB6GENERIC.OperationCode){

                #undef MAKE_CASE
                #define MAKE_CASE(scsiOpCode) case scsiOpCode: scsiOpStr = #scsiOpCode; break;

                MAKE_CASE(SCSIOP_TEST_UNIT_READY)
                MAKE_CASE(SCSIOP_REWIND)    // aka SCSIOP_REZERO_UNIT
                MAKE_CASE(SCSIOP_REQUEST_BLOCK_ADDR)
                MAKE_CASE(SCSIOP_REQUEST_SENSE)
                MAKE_CASE(SCSIOP_FORMAT_UNIT)
                MAKE_CASE(SCSIOP_READ_BLOCK_LIMITS)
                MAKE_CASE(SCSIOP_INIT_ELEMENT_STATUS)   // aka SCSIOP_REASSIGN_BLOCKS
                MAKE_CASE(SCSIOP_RECEIVE)       // aka SCSIOP_READ6
                MAKE_CASE(SCSIOP_SEND)  // aka SCSIOP_WRITE6, SCSIOP_PRINT
                MAKE_CASE(SCSIOP_SLEW_PRINT)    // aka SCSIOP_SEEK6, SCSIOP_TRACK_SELECT
                MAKE_CASE(SCSIOP_SEEK_BLOCK)
                MAKE_CASE(SCSIOP_PARTITION)
                MAKE_CASE(SCSIOP_READ_REVERSE)
                MAKE_CASE(SCSIOP_FLUSH_BUFFER)      // aka SCSIOP_WRITE_FILEMARKS
                MAKE_CASE(SCSIOP_SPACE)
                MAKE_CASE(SCSIOP_INQUIRY)
                MAKE_CASE(SCSIOP_VERIFY6)
                MAKE_CASE(SCSIOP_RECOVER_BUF_DATA)
                MAKE_CASE(SCSIOP_MODE_SELECT)
                MAKE_CASE(SCSIOP_RESERVE_UNIT)
                MAKE_CASE(SCSIOP_RELEASE_UNIT)
                MAKE_CASE(SCSIOP_COPY)
                MAKE_CASE(SCSIOP_ERASE)
                MAKE_CASE(SCSIOP_MODE_SENSE)
                MAKE_CASE(SCSIOP_START_STOP_UNIT)   // aka SCSIOP_STOP_PRINT, SCSIOP_LOAD_UNLOAD
                MAKE_CASE(SCSIOP_RECEIVE_DIAGNOSTIC)
                MAKE_CASE(SCSIOP_SEND_DIAGNOSTIC)
                MAKE_CASE(SCSIOP_MEDIUM_REMOVAL)
                MAKE_CASE(SCSIOP_READ_FORMATTED_CAPACITY)
                MAKE_CASE(SCSIOP_READ_CAPACITY)
                MAKE_CASE(SCSIOP_READ)
                MAKE_CASE(SCSIOP_WRITE)
                MAKE_CASE(SCSIOP_SEEK)  // aka SCSIOP_LOCATE, SCSIOP_POSITION_TO_ELEMENT
                MAKE_CASE(SCSIOP_WRITE_VERIFY)
                MAKE_CASE(SCSIOP_VERIFY)
                MAKE_CASE(SCSIOP_SEARCH_DATA_HIGH)
                MAKE_CASE(SCSIOP_SEARCH_DATA_EQUAL)
                MAKE_CASE(SCSIOP_SEARCH_DATA_LOW)
                MAKE_CASE(SCSIOP_SET_LIMITS)
                MAKE_CASE(SCSIOP_READ_POSITION)
                MAKE_CASE(SCSIOP_SYNCHRONIZE_CACHE)
                MAKE_CASE(SCSIOP_COMPARE)
                MAKE_CASE(SCSIOP_COPY_COMPARE)
                MAKE_CASE(SCSIOP_WRITE_DATA_BUFF)
                MAKE_CASE(SCSIOP_READ_DATA_BUFF)
                MAKE_CASE(SCSIOP_CHANGE_DEFINITION)
                MAKE_CASE(SCSIOP_READ_SUB_CHANNEL)
                MAKE_CASE(SCSIOP_READ_TOC)
                MAKE_CASE(SCSIOP_READ_HEADER)
                MAKE_CASE(SCSIOP_PLAY_AUDIO)
                MAKE_CASE(SCSIOP_GET_CONFIGURATION)
                MAKE_CASE(SCSIOP_PLAY_AUDIO_MSF)
                MAKE_CASE(SCSIOP_PLAY_TRACK_INDEX)
                MAKE_CASE(SCSIOP_PLAY_TRACK_RELATIVE)
                MAKE_CASE(SCSIOP_GET_EVENT_STATUS)
                MAKE_CASE(SCSIOP_PAUSE_RESUME)
                MAKE_CASE(SCSIOP_LOG_SELECT)
                MAKE_CASE(SCSIOP_LOG_SENSE)
                MAKE_CASE(SCSIOP_STOP_PLAY_SCAN)
                MAKE_CASE(SCSIOP_READ_DISK_INFORMATION)
                MAKE_CASE(SCSIOP_READ_TRACK_INFORMATION)
                MAKE_CASE(SCSIOP_RESERVE_TRACK_RZONE)
                MAKE_CASE(SCSIOP_SEND_OPC_INFORMATION)
                MAKE_CASE(SCSIOP_MODE_SELECT10)
                MAKE_CASE(SCSIOP_MODE_SENSE10)
                MAKE_CASE(SCSIOP_CLOSE_TRACK_SESSION)
                MAKE_CASE(SCSIOP_READ_BUFFER_CAPACITY)
                MAKE_CASE(SCSIOP_SEND_CUE_SHEET)
                MAKE_CASE(SCSIOP_PERSISTENT_RESERVE_IN)
                MAKE_CASE(SCSIOP_PERSISTENT_RESERVE_OUT)
                MAKE_CASE(SCSIOP_REPORT_LUNS)
                MAKE_CASE(SCSIOP_BLANK)
                MAKE_CASE(SCSIOP_SEND_KEY)
                MAKE_CASE(SCSIOP_REPORT_KEY)
                MAKE_CASE(SCSIOP_MOVE_MEDIUM)
                MAKE_CASE(SCSIOP_LOAD_UNLOAD_SLOT)  // aka SCSIOP_EXCHANGE_MEDIUM
                MAKE_CASE(SCSIOP_SET_READ_AHEAD)
                MAKE_CASE(SCSIOP_READ_DVD_STRUCTURE)
                MAKE_CASE(SCSIOP_REQUEST_VOL_ELEMENT)
                MAKE_CASE(SCSIOP_SEND_VOLUME_TAG)
                MAKE_CASE(SCSIOP_READ_ELEMENT_STATUS)
                MAKE_CASE(SCSIOP_READ_CD_MSF)
                MAKE_CASE(SCSIOP_SCAN_CD)
                MAKE_CASE(SCSIOP_SET_CD_SPEED)
                MAKE_CASE(SCSIOP_PLAY_CD)
                MAKE_CASE(SCSIOP_MECHANISM_STATUS)
                MAKE_CASE(SCSIOP_READ_CD)
                MAKE_CASE(SCSIOP_SEND_DVD_STRUCTURE)
                MAKE_CASE(SCSIOP_INIT_ELEMENT_RANGE)
                MAKE_CASE(SCSIOP_READ16)
                MAKE_CASE(SCSIOP_WRITE16)
                MAKE_CASE(SCSIOP_VERIFY16)
                MAKE_CASE(SCSIOP_SYNCHRONIZE_CACHE16)
                MAKE_CASE(SCSIOP_READ_CAPACITY16)
            }
        }

        return scsiOpStr;
    }


    char *DbgGetSrbStatusStr(PSTORAGE_REQUEST_BLOCK_HEADER Srb)
    {
        char *srbStatStr = "?";

        switch (Srb->SrbStatus){

            #undef MAKE_CASE
            #define MAKE_CASE(srbStat) \
                        case srbStat: \
                            srbStatStr = #srbStat; \
                            break; \
                        case srbStat|SRB_STATUS_QUEUE_FROZEN: \
                            srbStatStr = #srbStat "|SRB_STATUS_QUEUE_FROZEN"; \
                            break; \
                        case srbStat|SRB_STATUS_AUTOSENSE_VALID: \
                            srbStatStr = #srbStat "|SRB_STATUS_AUTOSENSE_VALID"; \
                            break; \
                        case srbStat|SRB_STATUS_QUEUE_FROZEN|SRB_STATUS_AUTOSENSE_VALID: \
                            srbStatStr = #srbStat "|SRB_STATUS_QUEUE_FROZEN|SRB_STATUS_AUTOSENSE_VALID"; \
                            break;

            MAKE_CASE(SRB_STATUS_PENDING)
            MAKE_CASE(SRB_STATUS_SUCCESS)
            MAKE_CASE(SRB_STATUS_ABORTED)
            MAKE_CASE(SRB_STATUS_ABORT_FAILED)
            MAKE_CASE(SRB_STATUS_ERROR)
            MAKE_CASE(SRB_STATUS_BUSY)
            MAKE_CASE(SRB_STATUS_INVALID_REQUEST)
            MAKE_CASE(SRB_STATUS_INVALID_PATH_ID)
            MAKE_CASE(SRB_STATUS_NO_DEVICE)
            MAKE_CASE(SRB_STATUS_TIMEOUT)
            MAKE_CASE(SRB_STATUS_SELECTION_TIMEOUT)
            MAKE_CASE(SRB_STATUS_COMMAND_TIMEOUT)
            MAKE_CASE(SRB_STATUS_MESSAGE_REJECTED)
            MAKE_CASE(SRB_STATUS_BUS_RESET)
            MAKE_CASE(SRB_STATUS_PARITY_ERROR)
            MAKE_CASE(SRB_STATUS_REQUEST_SENSE_FAILED)
            MAKE_CASE(SRB_STATUS_NO_HBA)
            MAKE_CASE(SRB_STATUS_DATA_OVERRUN)
            MAKE_CASE(SRB_STATUS_UNEXPECTED_BUS_FREE)
            MAKE_CASE(SRB_STATUS_PHASE_SEQUENCE_FAILURE)
            MAKE_CASE(SRB_STATUS_BAD_SRB_BLOCK_LENGTH)
            MAKE_CASE(SRB_STATUS_REQUEST_FLUSHED)
            MAKE_CASE(SRB_STATUS_INVALID_LUN)
            MAKE_CASE(SRB_STATUS_INVALID_TARGET_ID)
            MAKE_CASE(SRB_STATUS_BAD_FUNCTION)
            MAKE_CASE(SRB_STATUS_ERROR_RECOVERY)
            MAKE_CASE(SRB_STATUS_NOT_POWERED)
            MAKE_CASE(SRB_STATUS_INTERNAL_ERROR)
        }

        return srbStatStr;
    }


    char *DbgGetSenseCodeStr(PSTORAGE_REQUEST_BLOCK_HEADER Srb)
    {
        char *senseCodeStr = "?";

        if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID){

            PVOID senseData;
            UCHAR senseCode;
            BOOLEAN validSense;

            senseData = SrbGetSenseInfoBuffer(Srb);
            NT_ASSERT(senseData);

            validSense = ScsiGetSenseKeyAndCodes(senseData,
                                                 SrbGetSenseInfoBufferLength(Srb),
                                                 SCSI_SENSE_OPTIONS_FIXED_FORMAT_IF_UNKNOWN_FORMAT_INDICATED,
                                                 &senseCode,
                                                 NULL,
                                                 NULL);
            if (validSense) {
                switch (senseCode){

                    #undef MAKE_CASE
                    #define MAKE_CASE(snsCod) case snsCod: senseCodeStr = #snsCod; break;

                    MAKE_CASE(SCSI_SENSE_NO_SENSE)
                    MAKE_CASE(SCSI_SENSE_RECOVERED_ERROR)
                    MAKE_CASE(SCSI_SENSE_NOT_READY)
                    MAKE_CASE(SCSI_SENSE_MEDIUM_ERROR)
                    MAKE_CASE(SCSI_SENSE_HARDWARE_ERROR)
                    MAKE_CASE(SCSI_SENSE_ILLEGAL_REQUEST)
                    MAKE_CASE(SCSI_SENSE_UNIT_ATTENTION)
                    MAKE_CASE(SCSI_SENSE_DATA_PROTECT)
                    MAKE_CASE(SCSI_SENSE_BLANK_CHECK)
                    MAKE_CASE(SCSI_SENSE_UNIQUE)
                    MAKE_CASE(SCSI_SENSE_COPY_ABORTED)
                    MAKE_CASE(SCSI_SENSE_ABORTED_COMMAND)
                    MAKE_CASE(SCSI_SENSE_EQUAL)
                    MAKE_CASE(SCSI_SENSE_VOL_OVERFLOW)
                    MAKE_CASE(SCSI_SENSE_MISCOMPARE)
                    MAKE_CASE(SCSI_SENSE_RESERVED)
                }
            }
        }

        return senseCodeStr;
    }


    char *DbgGetAdditionalSenseCodeStr(PSTORAGE_REQUEST_BLOCK_HEADER Srb)
    {
        char *adSenseCodeStr = "?";

        if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID){
            PVOID senseData;
            UCHAR adSenseCode;
            BOOLEAN validSense;

            senseData = SrbGetSenseInfoBuffer(Srb);
            NT_ASSERT(senseData);

            validSense = ScsiGetSenseKeyAndCodes(senseData,
                                                 SrbGetSenseInfoBufferLength(Srb),
                                                 SCSI_SENSE_OPTIONS_FIXED_FORMAT_IF_UNKNOWN_FORMAT_INDICATED,
                                                 NULL,
                                                 &adSenseCode,
                                                 NULL);

            if (validSense) {
                switch (adSenseCode){

                    #undef MAKE_CASE
                    #define MAKE_CASE(adSnsCod) case adSnsCod: adSenseCodeStr = #adSnsCod; break;

                    MAKE_CASE(SCSI_ADSENSE_NO_SENSE)
                    MAKE_CASE(SCSI_ADSENSE_LUN_NOT_READY)
                    MAKE_CASE(SCSI_ADSENSE_TRACK_ERROR)
                    MAKE_CASE(SCSI_ADSENSE_SEEK_ERROR)
                    MAKE_CASE(SCSI_ADSENSE_REC_DATA_NOECC)
                    MAKE_CASE(SCSI_ADSENSE_REC_DATA_ECC)
                    MAKE_CASE(SCSI_ADSENSE_ILLEGAL_COMMAND)
                    MAKE_CASE(SCSI_ADSENSE_ILLEGAL_BLOCK)
                    MAKE_CASE(SCSI_ADSENSE_INVALID_CDB)
                    MAKE_CASE(SCSI_ADSENSE_INVALID_LUN)
                    MAKE_CASE(SCSI_ADSENSE_WRITE_PROTECT)   // aka SCSI_ADWRITE_PROTECT
                    MAKE_CASE(SCSI_ADSENSE_MEDIUM_CHANGED)
                    MAKE_CASE(SCSI_ADSENSE_BUS_RESET)
                    MAKE_CASE(SCSI_ADSENSE_INVALID_MEDIA)
                    MAKE_CASE(SCSI_ADSENSE_NO_MEDIA_IN_DEVICE)
                    MAKE_CASE(SCSI_ADSENSE_POSITION_ERROR)
                    MAKE_CASE(SCSI_ADSENSE_OPERATOR_REQUEST)
                    MAKE_CASE(SCSI_ADSENSE_FAILURE_PREDICTION_THRESHOLD_EXCEEDED)
                    MAKE_CASE(SCSI_ADSENSE_COPY_PROTECTION_FAILURE)
                    MAKE_CASE(SCSI_ADSENSE_VENDOR_UNIQUE)
                    MAKE_CASE(SCSI_ADSENSE_MUSIC_AREA)
                    MAKE_CASE(SCSI_ADSENSE_DATA_AREA)
                    MAKE_CASE(SCSI_ADSENSE_VOLUME_OVERFLOW)
                }
            }
        }

        return adSenseCodeStr;
    }


    char *DbgGetAdditionalSenseCodeQualifierStr(PSTORAGE_REQUEST_BLOCK_HEADER Srb)
    {
        char *adSenseCodeQualStr = "?";

        if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID){
            PVOID senseData;
            UCHAR adSenseCode;
            UCHAR adSenseCodeQual;
            BOOLEAN validSense;

            senseData = SrbGetSenseInfoBuffer(Srb);
            NT_ASSERT(senseData);

            validSense = ScsiGetSenseKeyAndCodes(senseData,
                                                 SrbGetSenseInfoBufferLength(Srb),
                                                 SCSI_SENSE_OPTIONS_FIXED_FORMAT_IF_UNKNOWN_FORMAT_INDICATED,
                                                 NULL,
                                                 &adSenseCode,
                                                 &adSenseCodeQual);
            if (validSense) {
                switch (adSenseCode){

                    #undef MAKE_CASE
                    #define MAKE_CASE(adSnsCodQual) case adSnsCodQual: adSenseCodeQualStr = #adSnsCodQual; break;

                    case SCSI_ADSENSE_LUN_NOT_READY:
                        switch (adSenseCodeQual){
                            MAKE_CASE(SCSI_SENSEQ_CAUSE_NOT_REPORTABLE)
                            MAKE_CASE(SCSI_SENSEQ_BECOMING_READY)
                            MAKE_CASE(SCSI_SENSEQ_INIT_COMMAND_REQUIRED)
                            MAKE_CASE(SCSI_SENSEQ_MANUAL_INTERVENTION_REQUIRED)
                            MAKE_CASE(SCSI_SENSEQ_FORMAT_IN_PROGRESS)
                            MAKE_CASE(SCSI_SENSEQ_REBUILD_IN_PROGRESS)
                            MAKE_CASE(SCSI_SENSEQ_RECALCULATION_IN_PROGRESS)
                            MAKE_CASE(SCSI_SENSEQ_OPERATION_IN_PROGRESS)
                            MAKE_CASE(SCSI_SENSEQ_LONG_WRITE_IN_PROGRESS)
                        }
                        break;
                    case SCSI_ADSENSE_NO_SENSE:
                        switch (adSenseCodeQual){
                            MAKE_CASE(SCSI_SENSEQ_FILEMARK_DETECTED)
                            MAKE_CASE(SCSI_SENSEQ_END_OF_MEDIA_DETECTED)
                            MAKE_CASE(SCSI_SENSEQ_SETMARK_DETECTED)
                            MAKE_CASE(SCSI_SENSEQ_BEGINNING_OF_MEDIA_DETECTED)
                        }
                        break;
                    case SCSI_ADSENSE_ILLEGAL_BLOCK:
                        switch (adSenseCodeQual){
                            MAKE_CASE(SCSI_SENSEQ_ILLEGAL_ELEMENT_ADDR)
                        }
                        break;
                    case SCSI_ADSENSE_POSITION_ERROR:
                        switch (adSenseCodeQual){
                            MAKE_CASE(SCSI_SENSEQ_DESTINATION_FULL)
                            MAKE_CASE(SCSI_SENSEQ_SOURCE_EMPTY)
                        }
                        break;
                    case SCSI_ADSENSE_INVALID_MEDIA:
                        switch (adSenseCodeQual){
                            MAKE_CASE(SCSI_SENSEQ_INCOMPATIBLE_MEDIA_INSTALLED)
                            MAKE_CASE(SCSI_SENSEQ_UNKNOWN_FORMAT)
                            MAKE_CASE(SCSI_SENSEQ_INCOMPATIBLE_FORMAT)
                            MAKE_CASE(SCSI_SENSEQ_CLEANING_CARTRIDGE_INSTALLED)
                        }
                        break;
                    case SCSI_ADSENSE_OPERATOR_REQUEST:
                        switch (adSenseCodeQual){
                            MAKE_CASE(SCSI_SENSEQ_STATE_CHANGE_INPUT)
                            MAKE_CASE(SCSI_SENSEQ_MEDIUM_REMOVAL)
                            MAKE_CASE(SCSI_SENSEQ_WRITE_PROTECT_ENABLE)
                            MAKE_CASE(SCSI_SENSEQ_WRITE_PROTECT_DISABLE)
                        }
                        break;
                    case SCSI_ADSENSE_COPY_PROTECTION_FAILURE:
                        switch (adSenseCodeQual){
                            MAKE_CASE(SCSI_SENSEQ_AUTHENTICATION_FAILURE)
                            MAKE_CASE(SCSI_SENSEQ_KEY_NOT_PRESENT)
                            MAKE_CASE(SCSI_SENSEQ_KEY_NOT_ESTABLISHED)
                            MAKE_CASE(SCSI_SENSEQ_READ_OF_SCRAMBLED_SECTOR_WITHOUT_AUTHENTICATION)
                            MAKE_CASE(SCSI_SENSEQ_MEDIA_CODE_MISMATCHED_TO_LOGICAL_UNIT)
                            MAKE_CASE(SCSI_SENSEQ_LOGICAL_UNIT_RESET_COUNT_ERROR)
                        }
                        break;
                }
            }
        }

        return adSenseCodeQualStr;
    }


