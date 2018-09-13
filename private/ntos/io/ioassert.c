/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ioassert.c

Abstract:

    This module implements basic support for driver correctness checks (which
    may be raised to debugger prompts, bugchecks, etc).

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

Known BUGBUGs:

--*/

#include "iop.h"

#if (( defined(_X86_) ) && ( FPO ))
#pragma optimize( "y", off )    // disable FPO for consistent stack traces
#endif

#ifndef NO_SPECIAL_IRP

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEVRFY, IopDriverCorrectnessTakeLock)
#pragma alloc_text(PAGEVRFY, IopDriverCorrectnessReleaseLock)
#pragma alloc_text(PAGEVRFY, IopDriverCorrectnessCheckUnderLock)
#pragma alloc_text(PAGEVRFY, IopDriverCorrectnessProcessParams)
#pragma alloc_text(PAGEVRFY, IopDriverCorrectnessProcessMessageText)
#pragma alloc_text(PAGEVRFY, IopDriverCorrectnessApplyControl)
#pragma alloc_text(PAGEVRFY, IopDriverCorrectnessThrowBugCheck)
#pragma alloc_text(PAGEVRFY, IopDriverCorrectnessPrintBuffer)
#pragma alloc_text(PAGEVRFY, IopDriverCorrectnessPrintParamData)
#pragma alloc_text(PAGEVRFY, IopDriverCorrectnessPrompt)
#pragma alloc_text(PAGEVRFY, IopDriverCorrectnessAddressToFileHeader)
#pragma alloc_text(PAGEVRFY, IopDriverCorrectnessPrintIrp)
#pragma alloc_text(PAGEVRFY, IopDriverCorrectnessPrintIrpStack)
#endif // ALLOC_PRAGMA

PULONG            IopDcControlCurrent = NULL;
LONG              IopDcCurrentFrameSkips = -1;
KIRQL             IopDcControlIrql;
KSPIN_LOCK        IopDcControlLock;
ULONG             IopDcControlInitial = (ULONG) -1;
ULONG             IopDcControlOverride = (ULONG) -1;

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEVRFY$data")
#endif

//
// When invoking the driver check macro's, pass Irps first, Routines second,
// DevObj's third, and any Status's last...
//
DCPARAM_TYPE_ENTRY DcParamTable[] = {
    { DCPARAM_IRP,     "Irp"     },
    { DCPARAM_ROUTINE, "Routine" },
    { DCPARAM_DEVOBJ,  "DevObj"  },
    { DCPARAM_STATUS,  "Status"  }
};

typedef struct _DCERROR_MESSAGE {

    DCERROR_ID       MessageID;
    PCDCERROR_CLASS  MessageClass;
    PSTR             MessageText;

} DCERROR_MESSAGE, *PDCERROR_MESSAGE;

//
// These are the general "classifications" of errors, along with the default
// flags that will be applied the first time this is hit.
//
// IopDcWdmDriverErrorFatal -
//     Anything in this class will cause the IO verifier to bugcheck if no
// debugger is present.
//
// IopDcWdmDriverErrorNonFatal -
//     Anything in this class will not cause the IO verifier to bugcheck, but
// will stop under the debugger.
//
// IopDcWdmDriverWarning -
//     Anything in this class will beep but continue without breaking in.
//
// IopDcWdmDriverPostponed -
//     Anything in this class will merely print and continue.
//
// IopDcWdmCoreError -
//     Issue in a core component (kernel or hal)
//
DCERROR_CLASS IopDcWdmDriverErrorFatal =
    { DIAG_BEEP | DIAG_FATAL_ERROR | DIAG_WDM_ERROR, "WDM DRIVER ERROR" };

DCERROR_CLASS IopDcWdmDriverErrorNonFatal =
    { DIAG_BEEP | DIAG_WDM_ERROR, "WDM DRIVER ERROR" };

DCERROR_CLASS IopDcWdmDriverWarning =
    { DIAG_BEEP | DIAG_ZAPPED,    "WDM DRIVER WARNING" };

DCERROR_CLASS IopDcWdmDriverPostponed = { DIAG_ZAPPED, "POSTPONED WDM DRIVER BUG" };

DCERROR_CLASS IopDcWdmCoreError = { DIAG_BEEP, "CORE DRIVER ERROR" };

//
// This is the table of error messages. Note that we've numbered the error
// message numbers continuously, so that they can be used as an indice into the
// table. Also note these constants are published, so if asserts are removed
// in the future, the current algorithm may need to be replaced by something
// akin to a binary search (or perhaps padding will need to be added).
//
DCERROR_MESSAGE IopDcMessageTable[DCERROR_MAXIMUM - DCERROR_UNSPECIFIED] = {
   { DCERROR_UNSPECIFIED, NULL, NULL },
   { DCERROR_DELETE_WHILE_ATTACHED, &IopDcWdmDriverErrorFatal,
     "A device is deleting itself while there is another device beneath it in "
     "the driver stack. This may be because the caller has forgotten to call "
     "IoDetachDevice first, or the lower driver may have incorrectly deleted "
     "itself." },
   { DCERROR_DETACH_NOT_ATTACHED, &IopDcWdmDriverErrorFatal,
     "Driver has attempted to detach from device object %DevObj, which is not "
     "attached to anything. This may occur if detach was called twice on the "
     "same device object." },
   { DCERROR_CANCELROUTINE_FORWARDED, &IopDcWdmDriverErrorFatal,
     "A driver has called IoCallDriver without setting the CancelRoutine in "
     "the Irp to NULL (Irp = %Irp )." },
   { DCERROR_NULL_DEVOBJ_FORWARDED, &IopDcWdmDriverErrorFatal,
     "Caller has passed in NULL as a DeviceObject. This is fatal (Irp = %Irp )."
     },
   { DCERROR_QUEUED_IRP_FORWARDED, &IopDcWdmDriverErrorFatal,
     "Caller is forwarding an IRP that is currently queued beneath it! The "
     "code handling IRPs returning STATUS_PENDING in this driver appears to "
     "be broken (Irp = %Irp )." },
   { DCERROR_NEXTIRPSP_DIRTY, &IopDcWdmDriverErrorFatal,
     "Caller has incorrectly forwarded an IRP (control field not zerod). The "
     "driver should use IoCopyCurrentIrpStackLocationToNext or "
     "IoSkipCurrentIrpStackLocation. (Irp = %Irp )" },
   { DCERROR_IRPSP_COPIED, &IopDcWdmDriverErrorFatal,
     "Caller has manually copied the stack and has inadvertantly copied the "
     "upper layer's completion routine. Please use "
     "IoCopyCurrentIrpStackLocationToNext. (Irp = %Irp )." },
   { DCERROR_INSUFFICIENT_STACK_LOCATIONS, &IopDcWdmDriverErrorFatal,
     "This IRP is about to run out of stack locations. Someone may have "
     "forwarded this IRP from another stack (Irp = %Irp )." },
   { DCERROR_QUEUED_IRP_COMPLETED, &IopDcWdmDriverErrorFatal,
     "Caller is completing an IRP that is currently queued beneath it! The "
     "code handling IRPs returning STATUS_PENDING in this driver appears to be "
     "broken. (Irp = %Irp )" },
   { DCERROR_FREE_OF_INUSE_TRACKED_IRP, &IopDcWdmDriverErrorFatal,
     "Caller of IoFreeIrp is freeing an IRP that is still in use! (Original "
     "Irp = %Irp1, Irp in usage is %Irp2 )" },
   { DCERROR_FREE_OF_INUSE_IRP, &IopDcWdmDriverErrorFatal,
     "Caller of IoFreeIrp is freeing an IRP that is still in use! (Irp = %Irp )"
     },
   { DCERROR_FREE_OF_THREADED_IRP, &IopDcWdmDriverErrorFatal,
     "Caller of IoFreeIrp is freeing an IRP that is still enqueued against a "
     "thread! (Irp = %Irp )" },
   { DCERROR_REINIT_OF_ALLOCATED_IRP_WITH_QUOTA, &IopDcWdmDriverErrorFatal,
     "Caller of IoInitializeIrp has passed an IRP that was allocated with "
     "IoAllocateIrp. This is illegal and unneccessary, and has caused a quota "
     "leak. Check the documentation for IoReuseIrp if this IRP is being "
     "recycled." },
   { DCERROR_PNP_IRP_BAD_INITIAL_STATUS, &IopDcWdmDriverErrorNonFatal,
     "Any PNP IRP must have status initialized to STATUS_NOT_SUPPORTED "
     "(Irp = %Irp )." },
   { DCERROR_POWER_IRP_BAD_INITIAL_STATUS, &IopDcWdmDriverErrorNonFatal,
     "Any Power IRP must have status initialized to STATUS_NOT_SUPPORTED "
     "(Irp = %Irp )." },
   { DCERROR_WMI_IRP_BAD_INITIAL_STATUS, &IopDcWdmDriverErrorNonFatal,
     "Any WMI IRP must have status initialized to STATUS_NOT_SUPPORTED "
     "(Irp = %Irp )." },
   { DCERROR_SKIPPED_DEVICE_OBJECT, &IopDcWdmDriverErrorNonFatal,
     "Caller has forwarded an Irp while skipping a device object in the stack. "
     "The caller is probably sending IRPs to the PDO instead of to the device "
     "returned by IoAttachDeviceToDeviceStack (Irp = %Irp )." },
   { DCERROR_BOGUS_FUNC_TRASHED, &IopDcWdmDriverErrorNonFatal,
     "Caller has trashed or has not properly copied IRP's stack (Irp = %Irp )."
     },
   { DCERROR_BOGUS_STATUS_TRASHED, &IopDcWdmDriverErrorNonFatal,
     "Caller has changed the status field of an IRP it does not understand "
     "(Irp = %Irp )." },
   { DCERROR_BOGUS_INFO_TRASHED, &IopDcWdmDriverErrorNonFatal,
     "Caller has changed the information field of an IRP it does not "
     "understand (Irp = %Irp )." },
   { DCERROR_PNP_FAILURE_FORWARDED, &IopDcWdmDriverErrorNonFatal, // Grrr
     "Non-successful non-STATUS_NOT_SUPPORTED IRP status for IRP_MJ_PNP is "
     "being passed down stack (Irp = %Irp ). Failed PNP IRPs must be completed."
     },
   { DCERROR_PNP_IRP_STATUS_RESET, &IopDcWdmDriverErrorNonFatal,
     "Previously set IRP_MJ_PNP status has been converted to "
     "STATUS_NOT_SUPPORTED. (Irp = %Irp )." },
   { DCERROR_PNP_IRP_NEEDS_FDO_HANDLING, &IopDcWdmDriverErrorNonFatal,
     "FDO caller has not handled a required IRP. The FDO must either fail the "
     "IRP or set the IRP's status if it is not going change the IRP's status "
     "using a completion routine. (Irp = %Irp )." },
   { DCERROR_PNP_IRP_FDO_HANDS_OFF, &IopDcWdmDriverErrorNonFatal,
     "FDO caller has responded to an IRP that is reserved for PDO use only. "
     "Stop it. (Irp = %Irp )" },
   { DCERROR_POWER_FAILURE_FORWARDED, &IopDcWdmDriverErrorNonFatal, // Grrr
     "Non-successful non-STATUS_NOT_SUPPORTED IRP status for IRP_MJ_POWER is "
     "being passed down stack (Irp = %Irp ). Failed POWER IRPs must be "
     "completed." },
   { DCERROR_POWER_IRP_STATUS_RESET, &IopDcWdmDriverErrorNonFatal,
     "Previously set IRP_MJ_POWER status has been converted to "
     "STATUS_NOT_SUPPORTED. (Irp = %Irp )." },
   { DCERROR_INVALID_STATUS, &IopDcWdmDriverErrorNonFatal,
     "Driver has returned a suspicious status. This is probably due to an "
     "uninitiaized variable bug in the driver. (Irp = %Irp )" },
   { DCERROR_UNNECCESSARY_COPY, &IopDcWdmDriverWarning,
     "Caller has copied the Irp stack but not set a completion routine. "
     "This is inefficient, use IoSkipCurrentIrpStackLocation instead "
     "(Irp = %Irp )." },
   { DCERROR_SHOULDVE_DETACHED, &IopDcWdmDriverErrorFatal,
     "An IRP dispatch handler has not properly detached from the stack below "
     "it upon receiving a remove IRP. DeviceObject = %DevObj - Dispatch = "
     "%Routine - Irp = %Irp" },
   { DCERROR_SHOULDVE_DELETED, &IopDcWdmDriverErrorFatal,
     "An IRP dispatch handler has not properly deleted it's device object upon "
     "receiving a remove IRP. DeviceObject = %DevObj - Dispatch = %Routine - "
     "Irp = %Irp" },
   { DCERROR_MISSING_DISPATCH_FUNCTION, &IopDcWdmDriverErrorNonFatal,
     "This driver has not filled out a dispatch routine for a required IRP "
     "major function (Irp = %Irp )." },
   { DCERROR_WMI_IRP_NOT_FORWARDED, &IopDcWdmDriverErrorNonFatal,
     "IRP_MJ_SYSTEM_CONTROL has been completed by someone other than the "
     "ProviderId. This IRP should either have been completed earlier or "
     "should have been passed down (Irp = %Irp ). The IRP was targetted at "
     "DeviceObject %DevObj" },
   { DCERROR_DELETED_PRESENT_PDO, &IopDcWdmDriverErrorFatal,
     "An IRP dispatch handler for a PDO has deleted it's device object, but "
     "the hardware has not been reported as missing in a bus relations query. "
     "DeviceObject = %DevObj - Dispatch = %Routine - Irp = %Irp " },
   { DCERROR_BUS_FILTER_ERRONEOUSLY_DETACHED, &IopDcWdmDriverErrorFatal,
     "A Bus Filter's IRP dispatch handler has detached upon receiving a remove "
     "IRP when the PDO is still alive. Bus Filters must clean up in "
     "FastIoDetach callbacks. DeviceObject = %DevObj - Dispatch = %Routine - "
     "Irp = %Irp" },
   { DCERROR_BUS_FILTER_ERRONEOUSLY_DELETED, &IopDcWdmDriverErrorFatal,
     "An IRP dispatch handler for a bus filter has deleted it's device object, "
     "but the PDO is still present! Bus filters must clean up in FastIoDetach "
     "callbacks. DeviceObject = %DevObj - Dispatch = %Routine - Irp = %Irp" },
   { DCERROR_INCONSISTANT_STATUS, &IopDcWdmDriverErrorFatal,
     "An IRP dispatch handler ( %Routine ) has returned a status that is "
     "inconsistent with the Irp's IoStatus.Status field. ( Irp = %Irp - "
     "Irp->IoStatus.Status = %Status1 - returned = %Status2 )" },
   { DCERROR_UNINITIALIZED_STATUS, &IopDcWdmDriverErrorNonFatal,
     "An IRP dispatch handler has returned a status that is illegal "
     "(0xFFFFFFFF). This is probably due to an uninitialized stack variable. "
     "Please do an ln on address %lx and file a bug. (Irp = %Irp )" },
   { DCERROR_IRP_RETURNED_WITHOUT_COMPLETION, &IopDcWdmDriverErrorFatal,
     "An IRP dispatch handler has returned without passing down or completing "
     "this Irp or someone forgot to return STATUS_PENDING. (Irp = %Irp )." },
   { DCERROR_COMPLETION_ROUTINE_PAGABLE, &IopDcWdmDriverErrorFatal,
     "IRP completion routines must be in nonpagable code, and this one is not: "
     "%Routine. (Irp = %Irp )" },
   { DCERROR_PENDING_BIT_NOT_MIGRATED, &IopDcWdmDriverErrorNonFatal,
     "A driver's completion routine ( %Routine ) has not marked the IRP "
     "pending if the PendingReturned field was set in the IRP passed to it. "
     "This may cause the OS to hang, especially if an error is returned by the "
     " stack. (Irp = %Irp )" },
   { DCERROR_CANCELROUTINE_ON_FORWARDED_IRP, &IopDcWdmDriverErrorFatal,
     "A cancel routine has been set for an IRP that is currently being "
     "processed by drivers lower in the stack, possibly stomping their cancel "
     "routine (Irp = %Irp, Routine=%Routine )." },
   { DCERROR_PNP_IRP_NEEDS_PDO_HANDLING, &IopDcWdmDriverErrorNonFatal,
     "PDO has not responded to a required IRP (Irp = %Irp )" },
   { DCERROR_TARGET_RELATION_LIST_EMPTY, &IopDcWdmDriverErrorNonFatal,
     "PDO has forgotten to fill out the device relation list with the PDO for "
     "the TargetDeviceRelation query (Irp = %Irp )" },
   { DCERROR_TARGET_RELATION_NEEDS_REF, &IopDcWdmDriverErrorFatal,
     "The code implementing the TargetDeviceRelation query has not called "
     "ObReferenceObject on the PDO (Irp = %Irp )." },
   { DCERROR_BOGUS_PNP_IRP_COMPLETED, &IopDcWdmDriverErrorNonFatal,
     "Caller has completed a IRP_MJ_PNP it didn't understand instead of "
     "passing it down (Irp = %Irp )." },
   { DCERROR_SUCCESSFUL_PNP_IRP_NOT_FORWARDED, &IopDcWdmDriverErrorNonFatal,
     "Caller has completed successful IRP_MJ_PNP instead of passing it down "
     "(Irp = %Irp )." },
   { DCERROR_UNTOUCHED_PNP_IRP_NOT_FORWARDED, &IopDcWdmDriverErrorNonFatal,
     "Caller has completed untouched IRP_MJ_PNP (instead of passing the irp "
     "down) or non-PDO has failed the irp using illegal value of "
     "STATUS_NOT_SUPPORTED. (Irp = %Irp )." },
   { DCERROR_BOGUS_POWER_IRP_COMPLETED, &IopDcWdmDriverErrorNonFatal,
     "Caller has completed a IRP_MJ_POWER it didn't understand instead of "
     "passing it down (Irp = %Irp )." },
   { DCERROR_SUCCESSFUL_POWER_IRP_NOT_FORWARDED, &IopDcWdmDriverErrorFatal,
     "Caller has completed successful IRP_MJ_POWER instead of passing it down "
     "(Irp = %Irp )." },
   { DCERROR_UNTOUCHED_POWER_IRP_NOT_FORWARDED, &IopDcWdmDriverErrorNonFatal,
     "Caller has completed untouched IRP_MJ_POWER (instead of passing the irp "
     "down) or non-PDO has failed the irp using illegal value of "
     "STATUS_NOT_SUPPORTED. (Irp = %Irp )." },
   { DCERROR_PNP_QUERY_CAP_BAD_VERSION, &IopDcWdmDriverErrorFatal,
     "The version field of the query capabilities structure in a query "
     "capabilities IRP was not properly initialized. (Irp = %Irp )." },
   { DCERROR_PNP_QUERY_CAP_BAD_SIZE, &IopDcWdmDriverErrorFatal,
     "The size field of the query capabilities structure in a query "
     "capabilities IRP was not properly initialized. (Irp = %Irp )." },
   { DCERROR_PNP_QUERY_CAP_BAD_ADDRESS, &IopDcWdmDriverErrorNonFatal,
     "The address field of the query capabilities structure in a query "
     "capabilities IRP was not properly initialized to -1. (Irp = %Irp )." },
   { DCERROR_PNP_QUERY_CAP_BAD_UI_NUM, &IopDcWdmDriverErrorNonFatal,
     "The UI Number field of the query capabilities structure in a query "
     "capabilities IRP was not properly initialized to -1. (Irp = %Irp )." },
   { DCERROR_RESTRICTED_IRP, &IopDcWdmDriverErrorFatal,
     "A driver has sent an IRP that is restricted for system use only. "
     "(Irp = %Irp )." },
   { DCERROR_REINIT_OF_ALLOCATED_IRP_WITHOUT_QUOTA, &IopDcWdmDriverWarning,
     "Caller of IoInitializeIrp has passed an IRP that was allocated with "
     "IoAllocateIrp. This is illegal, unneccessary, and negatively impacts "
     "performace in normal use. Check the documentation for IoReuseIrp if "
     "this IRP is being recycled." },
   { DCERROR_UNFORWARDED_IRP_COMPLETED, &IopDcWdmDriverWarning,
     "The caller of IoCompleteRequest is completing an IRP that has never "
     "been forwarded via a call to IoCallDriver or PoCallDriver. This may "
     "be a bug. (Irp = %Irp )." },
   { DCERROR_DISPATCH_CALLED_AT_BAD_IRQL, &IopDcWdmDriverErrorFatal,
     "A driver has forwarded an IRP at an IRQL that is illegal for this major"
     " code. "
     "(Irp = %Irp )." },
   { DCERROR_BOGUS_MINOR_STATUS_TRASHED, &IopDcWdmDriverErrorFatal,
     "Caller has changed the status field of an IRP it does not understand "
     "(Irp = %Irp )." }
};

typedef struct _DC_OVERRIDE_TABLE {

    DCERROR_ID       MessageID;
    PSTR             DriverName;
    PDCERROR_CLASS   ReplacementClass;

} DC_OVERRIDE_TABLE, *PDC_OVERRIDE_TABLE;

//
// This table contains things we've postponed.
//
DC_OVERRIDE_TABLE IopDcOverrideTable[] = {

    //
    // These exist because verifier.exe cannot specify kernels or hals. We still
    // want a mechanism to allow complaints.
    //
    { DCERROR_UNSPECIFIED, "HAL.DLL",      &IopDcWdmCoreError },
    { DCERROR_UNSPECIFIED, "NTOSKRNL.EXE", &IopDcWdmCoreError },
    { DCERROR_UNSPECIFIED, "NTKRNLMP.EXE", &IopDcWdmCoreError },
    { DCERROR_UNSPECIFIED, "NTKRNLPA.EXE", &IopDcWdmCoreError },
    { DCERROR_UNSPECIFIED, "NTKRPAMP.EXE", &IopDcWdmCoreError },

    //
    // ADRIAO BUGBUG 08/10/1999 -
    //     NDIS doesn't call the shutdown handlers at power-off because many are
    // unstable and that adds seconds to shutdown. This is an unsafe design as
    // the miniport, which owns an IRQ, may find it's ports drop out from under
    // it when the *parent* powers off.
    //
    { DCERROR_SUCCESSFUL_POWER_IRP_NOT_FORWARDED, "NDIS.SYS",
      &IopDcWdmDriverPostponed },

    //
    // ADRIAO BUGBUG 08/10/1999 -
    //     ACPI and PCI have to work together to handle wait-wake. In the
    // current design, ACPI.SYS gets an interface and does all the work itself.
    // The proper design should move the queueing to PCI, or tell PCI to leave
    // wait-wake IRPs alone for the given device. Cutting off any other bus
    // filters is a bad design.
    //
    { DCERROR_SUCCESSFUL_POWER_IRP_NOT_FORWARDED, "ACPI.SYS",
      &IopDcWdmDriverPostponed },

    //
    // ADRIAO BUGBUG 08/20/1999 -
    //     This is on QueryCapabilities, which PCMCIA caches but should not.
    // NeilSa has postponed this one till NT 5.1
    //
    { DCERROR_SUCCESSFUL_PNP_IRP_NOT_FORWARDED, "PCMCIA.SYS",
      &IopDcWdmDriverPostponed },

    //
    // ADRIAO BUGBUG 08/20/1999 -
    //     PCMCIA completes Queries for S-IRPs. NeilSa has postponed this one
    // till NT 5.1
    //
    { DCERROR_SUCCESSFUL_POWER_IRP_NOT_FORWARDED, "PCMCIA.SYS",
      &IopDcWdmDriverPostponed },

    //
    // ADRIAO BUGBUG 08/21/1999 -
    //     SCSIPORT doesn't forward S0 Irps if the system is already in S0.
    // Consider if a PDO succeeds a Query-S1 IRP and then waits for either a
    // Set to S1, a new Query, or a Set to S0 meaning no transition will take
    // place after all. A filter between SCSIPORT and the PDO could fail the
    // Query-S1 on the way up. SCSIPORT knows the system has decided to stay in
    // S0, but it cuts such knowledge off from the PDO. Luckily today's list of
    // likely PDO's don't have such logic though. PeterWie has postponed this
    // one till NT 5.1
    //
    { DCERROR_SUCCESSFUL_POWER_IRP_NOT_FORWARDED, "SCSIPORT.SYS",
      &IopDcWdmDriverPostponed }
};

#ifdef  ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif

PCHAR
KeBugCheckUnicodeToAnsi(
    IN PUNICODE_STRING UnicodeString,
    OUT PCHAR AnsiBuffer,
    IN ULONG MaxAnsiLength
    );

VOID
IopDriverCorrectnessTakeLock(
    IN PULONG ControlNew,
    IN LONG   StackFramesToSkip
    )
/*++

   Description:

      Worker routine for KD_ASSERT_OUT, takes the assertion spinlock. We do
      this because we *must* use a macro to instantiate a caller-specific
      tag and we cannot pass in that value along with a variable printf style
      debug output. Hence we must "pre-program" the output routine with the
      breakpoint ID, and use a spinlock to protect ourselves
--*/
{
    ASSERT(IovpInitCalled);
    ExAcquireSpinLock( &IopDcControlLock, &IopDcControlIrql );
    ASSERT(IopDcControlCurrent == NULL) ;
    IopDcControlCurrent = ControlNew ;
    IopDcCurrentFrameSkips = StackFramesToSkip ;
}

VOID
IopDriverCorrectnessReleaseLock(
    VOID
    )
/*++

   Description:

      Worker routine for KD_ASSERT_OUT, see IopDiagSetAssertLock

--*/
{
    IopDcControlCurrent = NULL ;
    IopDcCurrentFrameSkips = -1 ;
    ExReleaseSpinLock( &IopDcControlLock, IopDcControlIrql );
}

NTSTATUS
IopDriverCorrectnessCheckUnderLock(
    IN DCERROR_ID    MessageID,
    IN ULONG         MessageParameterMask,
    ...
    )
/*++

   Description:

      This routine displays an assert and provides options for
      removing the breakpoint, changing to just a text-out, etc.

    This routine is used as part of a macro for providing "Zappable" traps -
    IE, users get the option of changing how a specific instance of
    WDM_DEBUG_OUT in the code is handled (default is spew and break, options
    are spew or blow away for this boot).

    Macro must call IopDiagSetAssertLock, IopDiagAssertPrintfUnderLock, and
    IopDiagReleaseAssertLock in order

    DCPARAM_IRP*(count)+DCPARAM_ROUTINE*(count)+DCPARAM_DEVOBJ*(count),
        irp1,
        irp2,
        irp3,
        routine1,
        ..,
        ..,
        devobj1,

    count can be a max of 3.


   Arguments:

      Control - if present, this variable determines whether we will
                 actually do anything.

                if DIAG_ZAPPED is set we will print the assert text and return.
                if DIAG_CLEARED is set, we will neither print or stop.
                if DIAG_BEEP is set, we will beep if not zapped.

      StackFramesToSkip - If this parameter is -1, it is not used.
                          Otherwise, the assertion code will attempt to walk
                          back the appropriate number of frames and
                          determine the caller to blame based on return
                          address.

      IrpToFlag      - IRP to print out failure data for if mishandling of
                       this IRP was the source of the error.

      AddressToFlag  - Address that the assert should be blaimed on. This
                       should be null if StackFramesToSkip is not -1.

      AssertionClass - Text describing the overall class of the assert

      AssertionText  - Text specifically describing the assert. Carriage
                       returns should not be embedded in the text, this
                       routine will automatically ensure words are not split
                       across the screen.

   Notes:

      The text will automagically be formatted and printed as such:

      ASSERTION CLASS: ASSERTION TEXT ASSERTION TEXT ASSERTION
                       TEXT ASSERTION TEXT ...

--*/

{
    va_list arglist;
    UCHAR finalBuffer[512];
    NTSTATUS status;
    DC_CHECK_DATA dcCheckData;
    PVOID dcParamArray[3*sizeof(DcParamTable)/sizeof(DCPARAM_TYPE_ENTRY)];
    BOOLEAN exitAssertion;

    va_start(arglist, MessageParameterMask);

    //
    // Determine what our basic policy towards this check will be and fill out
    // the dcCheckData structure as well as we can.
    //
    IopDriverCorrectnessProcessParams(
        IopDcControlCurrent,
        IopDcCurrentFrameSkips,
        MessageID,
        MessageParameterMask,
        &arglist,
        dcParamArray,
        &dcCheckData
        );

    va_end(arglist);

    if (!IopDriverCorrectnessApplyControl(&dcCheckData)) {

        //
        // Nothing to see here, just ignore the assert...
        //
        return STATUS_SUCCESS;
    }

    //
    // We are going to express our disatifaction somehow. Expand out the
    // message we've prepared for this scenario.
    //
    status = IopDriverCorrectnessProcessMessageText(
        sizeof(finalBuffer),
        finalBuffer,
        &dcCheckData
        );

    if (!NT_SUCCESS(status)) {

        ASSERT(0);

        //
        // Something went wrong with the index lookup!
        //
        return status;
    }

    do {

        IopDriverCorrectnessPrintBuffer(&dcCheckData);
        IopDriverCorrectnessPrintParamData(&dcCheckData);
        IopDriverCorrectnessThrowBugCheck(&dcCheckData);
        IopDriverCorrectnessPrompt(&dcCheckData, &exitAssertion);

    } while (!exitAssertion);

    return status;
}

VOID
IopDriverCorrectnessProcessParams(
    IN OUT PULONG          Control OPTIONAL,
    IN LONG                StackFramesToSkip,
    IN DCERROR_ID          MessageID,
    IN ULONG               MessageParameterMask,
    IN va_list *           MessageParameters,
    IN PVOID *             DcParamArray,
    OUT PDC_CHECK_DATA     DcCheckData
    )
{
    PVOID returnAddress[1];
    PVOID baseOfImage, culpritAddress;
    ULONG stackHash;
    PLDR_DATA_TABLE_ENTRY dataTableEntry;
    PIMAGE_NT_HEADERS ntHeaders;
    ULONG i, currentMessageMask, paramType, maxParameterTypes, paramMask;
    ULONG tableIndex;
    char ansiDriverName[81];

    tableIndex = MessageID - DCERROR_UNSPECIFIED;

    //
    // Retrieve a pointer to the appropriate message text.
    //
    if (tableIndex >= sizeof(IopDcMessageTable)/sizeof(DCERROR_MESSAGE)) {

        //
        // Bogus message index!
        //
        ASSERT(0);
        tableIndex = 0;
    }

    ASSERT(IopDcMessageTable[tableIndex].MessageID == MessageID);

    //
    // OK, we're going to do *something*. Process the incoming parameters.
    //
    maxParameterTypes = sizeof(DcParamTable)/sizeof(DCPARAM_TYPE_ENTRY);

    //
    // First we grab parameter off the stack and slot them appropriately into
    // our array of "things".
    //
    // The array is in groups of three for each possible member of a given type
    // (three irps, three routines, three device objects, etc). Items not
    // referenced in are set to NULL.
    //
    currentMessageMask = MessageParameterMask;

    for(paramType = 0; paramType < maxParameterTypes; paramType++) {

        paramMask = DcParamTable[paramType].DcParamMask ;
        for(i=0; i<3; i++) {

            if (currentMessageMask&(paramMask*3)) {

                currentMessageMask -= paramMask;
                DcParamArray[paramType*3+i] = va_arg(*MessageParameters, PVOID);

            } else {

                DcParamArray[paramType*3+i] = NULL;
            }
        }
    }

    //
    // If this fires, part of the MessageParameterMask was not understood,
    // probably a bug by the caller!
    //
    ASSERT(currentMessageMask == 0);

    //
    // Pre-init unhelpful answers...
    //
    culpritAddress = NULL;
    DcCheckData->OffsetIntoImage = 0;
    DcCheckData->InVerifierList = TRUE;
    RtlInitUnicodeString(&DcCheckData->DriverName, NULL);

    //
    // Blame caller if appropriate
    //
    if (StackFramesToSkip != -1) {

        if (RtlCaptureStackBackTrace(StackFramesToSkip+2, 1, returnAddress,
           &stackHash)==1) {

            culpritAddress = returnAddress[0];
        }

    } else {

        //
        // %Routine1
        //
        culpritAddress = DcParamArray[3];
    }

    //
    // Extract the culprit's name if possible...
    //
    if (culpritAddress) {

        baseOfImage = IopDriverCorrectnessAddressToFileHeader(
            (PVOID) culpritAddress,
            &dataTableEntry
            );

        if (baseOfImage != NULL) {

            //
            // If we found a match, drill into the dataTableEntry's to get
            //
            //
            if (MmIsAddressValid(dataTableEntry->DllBase) != FALSE) {

                ntHeaders = RtlImageNtHeader(dataTableEntry->DllBase);

                RtlCopyMemory(
                    &DcCheckData->DriverName,
                    &dataTableEntry->BaseDllName,
                    sizeof(UNICODE_STRING)
                    );

                DcCheckData->OffsetIntoImage = (PUCHAR) culpritAddress - (PUCHAR) baseOfImage;
            }

            //
            // Now record whether this is in the verifying table.
            //
            if (!(dataTableEntry->Flags & LDRP_IMAGE_VERIFYING)) {

                DcCheckData->InVerifierList = FALSE;
            }
        }
    }

    DcCheckData->CulpritAddress = culpritAddress;
    DcCheckData->DcParamArray = DcParamArray;
    DcCheckData->TableIndex = tableIndex;
    DcCheckData->MessageID = MessageID;
    DcCheckData->Control = Control;
    DcCheckData->AssertionClass = IopDcMessageTable[DcCheckData->TableIndex].MessageClass;

    //
    // Get an ANSI version of the driver name and root through the override
    // table.
    //
    KeBugCheckUnicodeToAnsi(
        &DcCheckData->DriverName,
        ansiDriverName,
        sizeof(ansiDriverName)
        );

    for(i=0; i<sizeof(IopDcOverrideTable)/sizeof(DC_OVERRIDE_TABLE); i++) {

        if ((IopDcOverrideTable[i].MessageID == MessageID) ||
           (IopDcOverrideTable[i].MessageID == DCERROR_UNSPECIFIED)) {

            if (!_stricmp(ansiDriverName, IopDcOverrideTable[i].DriverName)) {

                //
                // We have a match, override the error class. Note that as
                // Control is not per-driver, but per assert, we null out
                // the passed in Control so that we don't apply this override
                // policy against every driver!
                //
                DcCheckData->Control = NULL;
                DcCheckData->AssertionClass = IopDcOverrideTable[i].ReplacementClass;
            }
        }
    }
}

BOOLEAN
IopDriverCorrectnessApplyControl(
    IN OUT PDC_CHECK_DATA  DcCheckData
    )
{
    ULONG assertionControl;

    if (IopDcControlOverride) {

        assertionControl = IopDcControlOverride;

    } else if (DcCheckData->Control) {

        //
        // Initialize the control if appropo
        //
        if (!((*DcCheckData->Control)&DIAG_INITIALIZED)) {

            *DcCheckData->Control |= (
                DIAG_INITIALIZED | IopDcControlInitial |
                DcCheckData->AssertionClass->ClassFlags );
        }

        assertionControl = *DcCheckData->Control;

    } else {

        assertionControl =
            ( IopDcControlInitial | DcCheckData->AssertionClass->ClassFlags );
    }

    if (assertionControl&DIAG_CLEARED) {

        //
        // If the breakpoint was cleared, then return, print/rip not.
        //
        return FALSE;
    }

    if ((!(assertionControl&DIAG_IGNORE_DRIVER_LIST)) &&
        (!DcCheckData->InVerifierList)) {

        //
        // Not of interest, skip this one.
        //
        return FALSE;
    }

    //
    // If there is no debugger, don't halt the machine. We are probably
    // ripping like mad and the user just wants to be able to boot.
    // The one exception is if DIAG_FATAL_ERROR is set. Then we shall
    // invoke the driver bugcheck...
    //
    if ((!KdDebuggerEnabled) && (!(assertionControl&DIAG_FATAL_ERROR))) {

        return FALSE;
    }

    //
    // Record our intentions and continue.
    //
    DcCheckData->AssertionControl = assertionControl;
    return TRUE;
}

NTSTATUS
IopDriverCorrectnessProcessMessageText(
    IN ULONG               MaxOutputBufferSize,
    OUT PSTR               OutputBuffer,
    IN OUT PDC_CHECK_DATA  DcCheckData
    )
{
    ULONG paramType, maxParameterTypes;
    ULONG arrayIndex, paramLength;
    PSTR messageHead, newMessage;
    LONG charsRemaining, length;

    //
    // Get the message text.
    //
    messageHead = IopDcMessageTable[DcCheckData->TableIndex].MessageText;

    //
    // Now manually build out the message.
    //
    newMessage = OutputBuffer;
    charsRemaining = (MaxOutputBufferSize/sizeof(UCHAR))-1;
    maxParameterTypes = sizeof(DcParamTable)/sizeof(DCPARAM_TYPE_ENTRY);

    while(*messageHead != '\0') {

        if (charsRemaining <= 0) {

            return STATUS_BUFFER_OVERFLOW;
        }

        if (*messageHead != '%') {

            *newMessage = *messageHead;
            newMessage++;
            messageHead++;
            charsRemaining--;

        } else {

            for(paramType = 0; paramType < maxParameterTypes; paramType++) {

                paramLength = strlen(DcParamTable[paramType].DcParamName);

                //
                // Do we have a match?
                //
                // N.B. - We don't do any case 'de-sensitizing' anywhere, so
                //        everything's cases must match!
                //
                if (RtlCompareMemory(
                    messageHead+1,
                    DcParamTable[paramType].DcParamName,
                    paramLength*sizeof(UCHAR)) == paramLength*sizeof(UCHAR)) {

                    arrayIndex = paramType*3;
                    messageHead += (paramLength+1);

                    //
                    // Was an index passed in (ie, "3rd" irp requested)?
                    //
                    if ((*messageHead >= '1') && (*messageHead <= '3')) {

                        //
                        // Adjust table index appropriately.
                        //
                        arrayIndex += (*messageHead - '1') ;
                        messageHead++;
                    }

                    length = _snprintf(
                        newMessage,
                        charsRemaining+1,
                        "%p",
                        DcCheckData->DcParamArray[arrayIndex]
                        );

                    if (length == -1) {

                        return STATUS_BUFFER_OVERFLOW;
                    }

                    charsRemaining -= length;
                    newMessage += length;
                    break;
                }
            }

            if (paramType == maxParameterTypes) {

                //
                // Either the message we looked up is malformed, we don't recognize
                // the %thing it is talking about, or this is %%!
                //
                *newMessage = *messageHead;
                messageHead++;
                newMessage++;
                charsRemaining--;

                if (*messageHead == '%') {

                    messageHead++;
                }
            }
        }
    }

    //
    // Null-terminate it (we have room because we took one off the buffer size
    // above).
    //
    *newMessage = '\0';

    DcCheckData->ClassText = DcCheckData->AssertionClass->MessageClassText;
    DcCheckData->AssertionText = OutputBuffer;
    return STATUS_SUCCESS;
}

VOID
IopDriverCorrectnessThrowBugCheck(
    IN PDC_CHECK_DATA DcCheckData
    )
{
    PVOID parameterArray[4];
    char captionBuffer[256];
    char ansiDriverName[81];
    UNICODE_STRING unicodeString;

    //
    // Do not bugcheck if a kernel debugger is attached, nor if this isn't a
    // fatal error.
    //
    if (KdDebuggerEnabled || (!(DcCheckData->AssertionControl&DIAG_FATAL_ERROR))) {

        return;
    }

    //
    // We are here because DIAG_FATAL_ERROR is set. We use
    // FATAL_UNHANDLED_HARD_ERROR so that we can give a
    // descriptive text string for the problem.
    //
    parameterArray[0] = (PVOID) DcCheckData->MessageID;
    parameterArray[1] = DcCheckData->CulpritAddress;
    parameterArray[2] = DcCheckData->DcParamArray[0];
    parameterArray[3] = DcCheckData->DcParamArray[6];

    KeBugCheckUnicodeToAnsi(
        &DcCheckData->DriverName,
        ansiDriverName,
        sizeof(ansiDriverName)
        );

    _snprintf(
        captionBuffer,
        sizeof(captionBuffer),
        "IO SYSTEM VERIFICATION ERROR in %s (%s %x)\n[%s+%x at %p]\n",
        ansiDriverName,
        DcCheckData->ClassText,
        DcCheckData->MessageID,
        ansiDriverName,
        DcCheckData->OffsetIntoImage,
        DcCheckData->CulpritAddress
        );

    KeBugCheckEx(
        FATAL_UNHANDLED_HARD_ERROR,
        DRIVER_VERIFIER_IOMANAGER_VIOLATION,
        (ULONG_PTR) parameterArray,
        (ULONG_PTR) captionBuffer,
        (ULONG_PTR) "" // DcCheckData->AssertionText is too technical
        );
}

VOID
IopDriverCorrectnessPrintBuffer(
    IN PDC_CHECK_DATA DcCheckData
    )
{
    UCHAR buffer[82];
    UCHAR classBuf[81];
    UCHAR callerBuf[81+40];
    UCHAR ansiDriverName[81];
    LONG  lMargin, i, lMarginCur, rMargin=78;
    PSTR lineStart, lastWord, current, lMarginText;

    //
    // Put down a carraige return
    //
    DbgPrint("\n") ;

    //
    // Drop a banner if this is a fatal assert.
    //
    if (DcCheckData->AssertionControl&DIAG_FATAL_ERROR) {

        DbgPrint(
            "***********************************************************************\n"
            "* THIS DRIVER BUG IS FATAL AND WILL CAUSE THE VERIFIER TO HALT        *\n"
            "* WINDOWS (BUGCHECK) WHEN THE MACHINE IS NOT UNDER A KERNEL DEBUGGER! *\n"
            "***********************************************************************\n"
            "\n"
            );
    }

    //
    // Prepare left margin (ClassText)
    //
    if (DcCheckData->ClassText != NULL) {

        lMargin = strlen(DcCheckData->ClassText)+2;

        DbgPrint("%s: ", DcCheckData->ClassText);

    } else {

        lMargin = 0;
    }

    if (lMargin+1>=rMargin) {

        lMargin=0;
    }

    for(i=0; i<lMargin; i++) classBuf[i] = ' ';
    classBuf[lMargin] = '\0';
    lMarginText = classBuf+lMargin;
    lMarginCur = lMargin;

    lineStart = lastWord = current = DcCheckData->AssertionText;

    //
    // Print out culprit if we have him...
    //
    if (DcCheckData->CulpritAddress) {

        if (DcCheckData->DriverName.Length) {

            KeBugCheckUnicodeToAnsi(
                &DcCheckData->DriverName,
                ansiDriverName,
                sizeof(ansiDriverName)
                );

            sprintf(callerBuf, "[%s @ 0x%p] ",
                ansiDriverName,
                DcCheckData->CulpritAddress
                );

        } else {

            sprintf(callerBuf, "[0x%p] ", DcCheckData->CulpritAddress);
        }

        DbgPrint("%s", callerBuf);
        lMarginCur += strlen(callerBuf);
    }

    //
    // Format and print our assertion text
    //
    while(*current) {

        if (*current == ' ') {

            if ((current - lineStart) >= (rMargin-lMarginCur-1)) {

                DbgPrint("%s", lMarginText);
                lMarginText = classBuf;
                lMarginCur = lMargin;

                if ((lastWord-lineStart)<rMargin) {

                    memcpy(buffer, lineStart, (ULONG)(lastWord-lineStart)*sizeof(UCHAR));
                    buffer[lastWord-lineStart] = '\0';
                    DbgPrint("%s\n", buffer);

                }

                lineStart = lastWord+1;
            }

            lastWord = current;
        }

        current++;
    }

    if ((current - lineStart) >= (rMargin-lMarginCur-1)) {

        DbgPrint("%s", lMarginText);
        lMarginText = classBuf;

        if ((lastWord-lineStart)<rMargin) {

            memcpy(buffer, lineStart, (ULONG)(lastWord-lineStart)*sizeof(UCHAR));
            buffer[lastWord-lineStart] = '\0';
            DbgPrint("%s\n", buffer);
        }

        lineStart = lastWord+1;
    }

    if (lineStart<current) {

        DbgPrint("%s%s\n", lMarginText, lineStart);
    }
}

VOID
IopDriverCorrectnessPrintParamData(
    IN PDC_CHECK_DATA DcCheckData
    )
{
    if (DcCheckData->DcParamArray[0]) {

        IopDriverCorrectnessPrintIrp((PIRP) DcCheckData->DcParamArray[0]);
    }
}

VOID
IopDriverCorrectnessPrompt(
    IN      PDC_CHECK_DATA  DcCheckData,
    OUT     PBOOLEAN        ExitAssertion
    )
{
    char response[2];
    ULONG assertionControl;
    BOOLEAN waitForInput;

    assertionControl = DcCheckData->AssertionControl;

    *ExitAssertion = TRUE;

    //
    // Vocalize if so ordered.
    //
    if (assertionControl&DIAG_BEEP) {

        DbgPrint("%c", 7);
    }

    if (assertionControl&DIAG_ZAPPED) {

        return;
    }

    //
    // Wait for input...
    //
    waitForInput = TRUE;
    while(waitForInput) {

        if (DcCheckData->Control) {

            DbgPrompt( "Break, Ignore, Zap, Remove, Disable all (bizrd)? ", response, sizeof( response ));
        } else {

            DbgPrompt( "Break, Ignore, Disable all (bid)? ", response, sizeof( response ));
        }

        switch (response[0]) {

            case 'B':
            case 'b':
                DbgPrint("Breaking in... (press g<enter> to return to assert menu)\n");
                DbgBreakPoint();
                waitForInput = FALSE;
                *ExitAssertion = FALSE;
                break;

            case 'I':
            case 'i':
                waitForInput = FALSE;
                break;

            case 'Z':
            case 'z':
                if (DcCheckData->Control) {

                   DbgPrint("Breakpoint zapped (OS will print text and return)\n");
                   assertionControl |= DIAG_ZAPPED;
                   assertionControl &=~ DIAG_BEEP;
                   waitForInput = FALSE;
                }
                break;

            case 'D':
            case 'd':
                IopDcControlOverride = DIAG_CLEARED;
                DbgPrint("Verification asserts disabled.\n");
                //assertionControl |= DIAG_CLEARED;
                waitForInput = FALSE;
                break;

            case 'R':
            case 'r':
                if (DcCheckData->Control) {

                   DbgPrint("Breakpoint removed\n") ;
                   assertionControl |= DIAG_CLEARED;
                   waitForInput = FALSE;
                }
                break;
        }
    }

    if (DcCheckData->Control) {
        *DcCheckData->Control = assertionControl;
    }
}

PVOID
IopDriverCorrectnessAddressToFileHeader(
    IN PVOID Address,
    OUT PLDR_DATA_TABLE_ENTRY *DataTableEntry
    )

/*++

Routine Description:

    This function returns the base of an image that contains the
    specified Address. An image contains the address if the address
    is within the ImageBase, and the ImageBase plus the size of the
    virtual image.

Arguments:

    Address - Supplies an address to resolve to a loader entry.

    DataTableEntry - Suppies a pointer to a variable that receives the
        address of the data table entry that describes the image.

Return Value:

    NULL - No image was found that contains the passed in address.

    NON-NULL - Returns the base address of the image that contain the
        address.

--*/

{

    PLIST_ENTRY ModuleListHead;
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY Next;
    UINT_PTR Bounds;
    PVOID ReturnBase=NULL, Base;

    ModuleListHead = &PsLoadedModuleList;

    //
    // It would be nice if we could call MiLookupDataTableEntry, but it's
    // pageable, so we do what the bugcheck stuff does...
    //
    Next = ModuleListHead->Flink;
    if (Next != NULL) {
        while (Next != ModuleListHead) {
            Entry = CONTAINING_RECORD(Next,
                                      LDR_DATA_TABLE_ENTRY,
                                      InLoadOrderLinks);

            Next = Next->Flink;
            Base = Entry->DllBase;
            Bounds = (UINT_PTR)Base + Entry->SizeOfImage;
            if ((UINT_PTR)Address >= (UINT_PTR)Base && (UINT_PTR)Address < Bounds) {
                *DataTableEntry = Entry;
                ReturnBase = Base;
                break;
            }
        }
    }

    return ReturnBase;
}

BOOLEAN
IopIsMemoryRangeReadable(
    IN PVOID Location,
    IN size_t Length
    )
{
   while (((ULONG_PTR)Location & (sizeof(ULONG_PTR)-1)) && (Length > 0)) {

        //
        // Check to determine if the move will succeed before actually performing
        // the operation.
        //
        if (MmIsAddressValid(Location)==FALSE) {
            return FALSE ;
        }

        ((PCHAR) Location)++ ;
        Length-- ;
    }

    while (Length > (sizeof(ULONG_PTR)-1)) {

        //
        // Check to determine if the move will succeed before actually performing
        // the operation.
        //
        if (MmIsAddressValid(Location)==FALSE) {
            return FALSE ;
        }
        ((PCHAR) Location) += sizeof(ULONG_PTR);
        Length -= sizeof(ULONG_PTR);

    }

    while (Length > 0) {

        //
        // Check to determine if the move will succeed before actually performing
        // the operation.
        //
        if (MmIsAddressValid(Location)==FALSE) {
            return FALSE;
        }

        ((PCHAR) Location)++ ;
        Length-- ;
    }
    return TRUE ;
}

VOID
IopDriverCorrectnessPrintIrp(
    IN PIRP IrpToFlag
    )
{
    PIO_STACK_LOCATION irpSpCur ;
    PIO_STACK_LOCATION irpSpNxt ;

    //
    // First see if we can touch the IRP header
    //
    if(!IopIsMemoryRangeReadable(IrpToFlag, sizeof(IRP))) {
        return ;
    }

    //
    // OK, get the next two stack locations...
    //
    irpSpNxt = IoGetNextIrpStackLocation( IrpToFlag );
    irpSpCur = IoGetCurrentIrpStackLocation( IrpToFlag );

    if (IopIsMemoryRangeReadable(irpSpNxt, 2*sizeof(IO_STACK_LOCATION))) {

        //
        // Both are present, print the best one!
        //
        if (irpSpNxt->MinorFunction == irpSpCur->MinorFunction) {

            //
            // Looks forwarded
            //
            IopDriverCorrectnessPrintIrpStack(irpSpNxt) ;
        } else if (irpSpNxt->MinorFunction == 0) {

            //
            // Next location is probably currently zero'd
            //
            IopDriverCorrectnessPrintIrpStack(irpSpCur) ;
        } else {
            DbgPrint("Next:    >") ;
            IopDriverCorrectnessPrintIrpStack(irpSpNxt) ;
            DbgPrint("Current:  ") ;
            IopDriverCorrectnessPrintIrpStack(irpSpCur) ;
        }
    } else if (IopIsMemoryRangeReadable(irpSpCur, sizeof(IO_STACK_LOCATION))) {

        IopDriverCorrectnessPrintIrpStack(irpSpCur) ;
    } else if (IopIsMemoryRangeReadable(irpSpNxt, sizeof(IO_STACK_LOCATION))) {

        IopDriverCorrectnessPrintIrpStack(irpSpNxt) ;
    }
}

PCHAR IrpMajorNames[] = {
    "IRP_MJ_CREATE",                          // 0x00
    "IRP_MJ_CREATE_NAMED_PIPE",               // 0x01
    "IRP_MJ_CLOSE",                           // 0x02
    "IRP_MJ_READ",                            // 0x03
    "IRP_MJ_WRITE",                           // 0x04
    "IRP_MJ_QUERY_INFORMATION",               // 0x05
    "IRP_MJ_SET_INFORMATION",                 // 0x06
    "IRP_MJ_QUERY_EA",                        // 0x07
    "IRP_MJ_SET_EA",                          // 0x08
    "IRP_MJ_FLUSH_BUFFERS",                   // 0x09
    "IRP_MJ_QUERY_VOLUME_INFORMATION",        // 0x0a
    "IRP_MJ_SET_VOLUME_INFORMATION",          // 0x0b
    "IRP_MJ_DIRECTORY_CONTROL",               // 0x0c
    "IRP_MJ_FILE_SYSTEM_CONTROL",             // 0x0d
    "IRP_MJ_DEVICE_CONTROL",                  // 0x0e
    "IRP_MJ_INTERNAL_DEVICE_CONTROL",         // 0x0f
    "IRP_MJ_SHUTDOWN",                        // 0x10
    "IRP_MJ_LOCK_CONTROL",                    // 0x11
    "IRP_MJ_CLEANUP",                         // 0x12
    "IRP_MJ_CREATE_MAILSLOT",                 // 0x13
    "IRP_MJ_QUERY_SECURITY",                  // 0x14
    "IRP_MJ_SET_SECURITY",                    // 0x15
    "IRP_MJ_POWER",                           // 0x16
    "IRP_MJ_SYSTEM_CONTROL",                  // 0x17
    "IRP_MJ_DEVICE_CHANGE",                   // 0x18
    "IRP_MJ_QUERY_QUOTA",                     // 0x19
    "IRP_MJ_SET_QUOTA",                       // 0x1a
    "IRP_MJ_PNP",                             // 0x1b
    NULL
    } ;

#define MAX_NAMED_MAJOR_IRPS   0x1b


PCHAR PnPIrpNames[] = {
    "IRP_MN_START_DEVICE",                    // 0x00
    "IRP_MN_QUERY_REMOVE_DEVICE",             // 0x01
    "IRP_MN_REMOVE_DEVICE - ",                // 0x02
    "IRP_MN_CANCEL_REMOVE_DEVICE",            // 0x03
    "IRP_MN_STOP_DEVICE",                     // 0x04
    "IRP_MN_QUERY_STOP_DEVICE",               // 0x05
    "IRP_MN_CANCEL_STOP_DEVICE",              // 0x06
    "IRP_MN_QUERY_DEVICE_RELATIONS",          // 0x07
    "IRP_MN_QUERY_INTERFACE",                 // 0x08
    "IRP_MN_QUERY_CAPABILITIES",              // 0x09
    "IRP_MN_QUERY_RESOURCES",                 // 0x0A
    "IRP_MN_QUERY_RESOURCE_REQUIREMENTS",     // 0x0B
    "IRP_MN_QUERY_DEVICE_TEXT",               // 0x0C
    "IRP_MN_FILTER_RESOURCE_REQUIREMENTS",    // 0x0D
    "INVALID_IRP_CODE",                       //
    "IRP_MN_READ_CONFIG",                     // 0x0F
    "IRP_MN_WRITE_CONFIG",                    // 0x10
    "IRP_MN_EJECT",                           // 0x11
    "IRP_MN_SET_LOCK",                        // 0x12
    "IRP_MN_QUERY_ID",                        // 0x13
    "IRP_MN_QUERY_PNP_DEVICE_STATE",          // 0x14
    "IRP_MN_QUERY_BUS_INFORMATION",           // 0x15
    "IRP_MN_DEVICE_USAGE_NOTIFICATION",       // 0x16
    "IRP_MN_SURPRISE_REMOVAL",                // 0x17
    "IRP_MN_QUERY_LEGACY_BUS_INFORMATION",    // 0x18
    NULL
    } ;

#define MAX_NAMED_PNP_IRP   0x18

PCHAR WmiIrpNames[] = {
    "IRP_MN_QUERY_ALL_DATA",                  // 0x00
    "IRP_MN_QUERY_SINGLE_INSTANCE",           // 0x01
    "IRP_MN_CHANGE_SINGLE_INSTANCE",          // 0x02
    "IRP_MN_CHANGE_SINGLE_ITEM",              // 0x03
    "IRP_MN_ENABLE_EVENTS",                   // 0x04
    "IRP_MN_DISABLE_EVENTS",                  // 0x05
    "IRP_MN_ENABLE_COLLECTION",               // 0x06
    "IRP_MN_DISABLE_COLLECTION",              // 0x07
    "IRP_MN_REGINFO",                         // 0x08
    "IRP_MN_EXECUTE_METHOD",                  // 0x09
    NULL
    } ;

#define MAX_NAMED_WMI_IRP   0x9

PCHAR PowerIrpNames[] = {
    "IRP_MN_WAIT_WAKE",                       // 0x00
    "IRP_MN_POWER_SEQUENCE",                  // 0x01
    "IRP_MN_SET_POWER",                       // 0x02
    "IRP_MN_QUERY_POWER",                     // 0x03
    NULL
    } ;

#define MAX_NAMED_POWER_IRP 0x3


VOID
IopDriverCorrectnessPrintIrpStack(
    IN PIO_STACK_LOCATION IrpSp
    )
{
    if ((IrpSp->MajorFunction==IRP_MJ_INTERNAL_DEVICE_CONTROL)&&(IrpSp->MinorFunction == IRP_MN_SCSI_CLASS)) {

         DbgPrint("IRP_MJ_SCSI") ;

    } else if (IrpSp->MajorFunction<=MAX_NAMED_MAJOR_IRPS) {

         DbgPrint(IrpMajorNames[IrpSp->MajorFunction]) ;

    } else if (IrpSp->MajorFunction==0xFF) {

         DbgPrint("IRP_MJ_BOGUS") ;

    } else {

         DbgPrint("IRP_MJ_??") ;
    }

    switch(IrpSp->MajorFunction) {

         case IRP_MJ_SYSTEM_CONTROL:
             DbgPrint(".") ;
             if (IrpSp->MinorFunction<=MAX_NAMED_WMI_IRP) {

                 DbgPrint(WmiIrpNames[IrpSp->MinorFunction]) ;
             } else if (IrpSp->MinorFunction==0xFF) {

                 DbgPrint("IRP_MN_BOGUS") ;
             } else {
                 DbgPrint("(Bogus)\n") ;
             }
             DbgPrint("\n") ;
             break ;
         case IRP_MJ_PNP:
             DbgPrint(".") ;
             if (IrpSp->MinorFunction<=MAX_NAMED_PNP_IRP) {

                 DbgPrint(PnPIrpNames[IrpSp->MinorFunction]) ;
             } else if (IrpSp->MinorFunction==0xFF) {

                 DbgPrint("IRP_MN_BOGUS") ;
             } else {

                 DbgPrint("(Bogus)\n") ;
             }
             switch(IrpSp->MinorFunction) {
                 case IRP_MN_QUERY_DEVICE_RELATIONS:

                     switch(IrpSp->Parameters.QueryDeviceRelations.Type) {
                         case BusRelations:
                             DbgPrint("(BusRelations)") ;
                             break ;
                         case EjectionRelations:
                             DbgPrint("(EjectionRelations)") ;
                             break ;
                         case PowerRelations:
                             DbgPrint("(PowerRelations)") ;
                             break ;
                         case RemovalRelations:
                             DbgPrint("(RemovalRelations)") ;
                             break ;
                         case TargetDeviceRelation:
                             DbgPrint("(TargetDeviceRelation)") ;
                             break ;
                         default:
                             DbgPrint("(Bogus)\n") ;
                             break ;
                     }
                     break ;
                 case IRP_MN_QUERY_INTERFACE:
                     break ;
                 case IRP_MN_QUERY_DEVICE_TEXT:
                     switch(IrpSp->Parameters.QueryId.IdType) {
                         case DeviceTextDescription:
                             DbgPrint("(DeviceTextDescription)") ;
                             break ;
                         case DeviceTextLocationInformation:
                             DbgPrint("(DeviceTextLocationInformation)") ;
                             break ;
                         default:
                             DbgPrint("(Bogus)\n") ;
                             break ;
                     }
                     break ;
                 case IRP_MN_WRITE_CONFIG:
                 case IRP_MN_READ_CONFIG:
                     DbgPrint("(WhichSpace=%x, Buffer=%x, Offset=%x, Length=%x)",
                         IrpSp->Parameters.ReadWriteConfig.WhichSpace,
                         IrpSp->Parameters.ReadWriteConfig.Buffer,
                         IrpSp->Parameters.ReadWriteConfig.Offset,
                         IrpSp->Parameters.ReadWriteConfig.Length
                         ) ;
                     break ;
                 case IRP_MN_SET_LOCK:
                     if (IrpSp->Parameters.SetLock.Lock) DbgPrint("(True)") ;
                     else DbgPrint("(False)") ;
                     break ;
                 case IRP_MN_QUERY_ID:
                     switch(IrpSp->Parameters.QueryId.IdType) {
                         case BusQueryDeviceID:
                             DbgPrint("(BusQueryDeviceID)") ;
                             break ;
                         case BusQueryHardwareIDs:
                             DbgPrint("(BusQueryHardwareIDs)") ;
                             break ;
                         case BusQueryCompatibleIDs:
                             DbgPrint("(BusQueryCompatibleIDs)") ;
                             break ;
                         case BusQueryInstanceID:
                             DbgPrint("(BusQueryInstanceID)") ;
                             break ;
                         default:
                             DbgPrint("(Bogus)\n") ;
                             break ;
                     }
                     break ;
                 case IRP_MN_QUERY_BUS_INFORMATION:
                     // BUGBUG: Should print out
                     break ;
                 case IRP_MN_DEVICE_USAGE_NOTIFICATION:
                     switch(IrpSp->Parameters.UsageNotification.Type) {
                         case DeviceUsageTypeUndefined:
                             DbgPrint("(DeviceUsageTypeUndefined") ;
                             break ;
                         case DeviceUsageTypePaging:
                             DbgPrint("(DeviceUsageTypePaging") ;
                             break ;
                         case DeviceUsageTypeHibernation:
                             DbgPrint("(DeviceUsageTypeHibernation") ;
                             break ;
                         case DeviceUsageTypeDumpFile:
                             DbgPrint("(DeviceUsageTypeDumpFile") ;
                             break ;
                         default:
                             DbgPrint("(Bogus)\n") ;
                             break ;
                     }
                     if (IrpSp->Parameters.UsageNotification.InPath) {
                         DbgPrint(", InPath=TRUE)") ;
                     } else {
                         DbgPrint(", InPath=FALSE)") ;
                     }
                     break ;
                 case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:
                     // BUGBUG: Should print out
                     break ;
                 default:
                     break ;
             }
             DbgPrint("\n") ;
             break ;

         case IRP_MJ_POWER:
             DbgPrint(".") ;
             if (IrpSp->MinorFunction<=MAX_NAMED_POWER_IRP) {

                 DbgPrint(PowerIrpNames[IrpSp->MinorFunction]) ;
             } else if (IrpSp->MinorFunction==0xFF) {

                 DbgPrint("IRP_MN_BOGUS") ;
             } else {
                 DbgPrint("(Bogus)\n") ;
             }
             DbgPrint("\n") ;
             break ;

         default:
             DbgPrint("\n") ;
             break ;
    }
}
#endif // NO_SPECIAL_IRP

