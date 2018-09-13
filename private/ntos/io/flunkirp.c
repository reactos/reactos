/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    flunkirp.c

Abstract:

    Note to people hitting bugs in these code paths due to core changes:

    -   "This file is NOT vital to operation of the OS, and could easily be
         disabled while a redesign to compensate for the core change is
         implemented." - the author

    This module asserts Irps are handled correctly by drivers.

To Do:

      **. Fail non-OS initiators of certain IRPs

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

Known BUGBUGs:

   >ADRIAO BUGBUG   #07 05/11/98 - Add a pass-through filter at every attach.
   ADRIAO BUGBUG   #03 06/05/98 - Need more chaff. Very much more.
   ADRIAO BUGBUG   #06 06/11/98 - Figure out when (Internal_)Device_Control
                                  IRPs must be sent at IRQL<DISPATCH_LEVEL...
Known HACKHACKs:

   ADRIAO HACKHACK 08/16/1999 - Fix ACPI to enable more chaff.
                                (HACKHACK_FOR_ACPI)

--*/

#include "iop.h"
#include "ob.h"

#if (( defined(_X86_) ) && ( FPO ))
#pragma optimize( "y", off )    // disable FPO for consistent stack traces
#endif

//
// This entire file is only present if NO_SPECIAL_IRP isn't defined
//
#ifndef NO_SPECIAL_IRP

//
// When enabled, everything is locked down on demand...
//
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEVRFY, IovpAssertIsNewRequest)
#pragma alloc_text(PAGEVRFY, IovpAssertNewIrps)
#pragma alloc_text(PAGEVRFY, IovpAssertFinalIrpStack)
#pragma alloc_text(PAGEVRFY, IovpAssertNewRequest)
#pragma alloc_text(PAGEVRFY, IovpAssertIrpStackDownward)
#pragma alloc_text(PAGEVRFY, IovpAssertIrpStackUpward)
#pragma alloc_text(PAGEVRFY, IovpThrowChaffAtStartedPdoStack)
#pragma alloc_text(PAGEVRFY, IovpThrowBogusSynchronousIrp)
#pragma alloc_text(PAGEVRFY, IovpStartObRefMonitoring)
#pragma alloc_text(PAGEVRFY, IovpStopObRefMonitoring)
#pragma alloc_text(PAGEVRFY, IovpIsSystemRestrictedIrp)
#endif

extern PUCHAR PnPIrpNames[];

#include <initguid.h>
DEFINE_GUID( GUID_BOGUS_INTERFACE, 0x00000000L, 0x0000, 0x0000,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

PCHAR
KeBugCheckUnicodeToAnsi(
    IN PUNICODE_STRING UnicodeString,
    OUT PCHAR AnsiBuffer,
    IN ULONG MaxAnsiLength
    );

BOOLEAN
IovpAssertIsNewRequest(
    IN PIO_STACK_LOCATION   IrpLastSp OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp
    )
/*++

  Description:

     Determines whether the two Irp stacks refer to the same "request",
     ie starting the same device, etc. This is used to detect whether an IRP
     has been simply forwarded or rather the IRP has been reused to initiate
     a new request.

  Arguments:

     The two IRP stacks to compare.

     N.B. - the device object is not currently part of those IRP stacks.

  Return Value:

     TRUE if the stacks represent the same request, FALSE otherwise.

--*/
{
    return ((IrpLastSp==NULL)||
        (IrpSp->MajorFunction != IrpLastSp->MajorFunction) ||
        (IrpSp->MinorFunction != IrpLastSp->MinorFunction));
}

BOOLEAN
IovpAssertDoAdvanceStatus(
    IN     PIO_STACK_LOCATION   IrpSp,
    IN     NTSTATUS             OriginalStatus,
    IN OUT NTSTATUS             *StatusToAdvance
    )
/*++

  Description:

     Given an IRP stack pointer, is it legal to change the status for
     debug-ability? If so, this function determines what the new status
     should be. Note that for each stack location, this function is iterated
     over n times where n is equal to the number of drivers who IoSkip'd this
     location.

  Arguments:

     IrpSp           - Current stack right after complete for the given stack
                       location, but before the completion routine for the
                       stack location above has been called.

     OriginalStatus  - The status of the IRP at the time listed above. Does
                       not change over iteration per skipping driver.

     StatusToAdvance - Pointer to the current status that should be updated.

  Return Value:

     TRUE if the status has been adjusted, FALSE otherwise.

--*/
{
    switch(IrpSp->MajorFunction) {

        case IRP_MJ_PNP:
        case IRP_MJ_POWER:
            if (((ULONG) OriginalStatus) < 256) {

                (*StatusToAdvance)++;
                if ((*StatusToAdvance) == STATUS_PENDING) {
                    (*StatusToAdvance)++;
                }
                return TRUE;
            }
            break;

        default:
            break;
    }
    return FALSE;
}

VOID
IovpAssertNewIrps(
    IN PIOV_REQUEST_PACKET  IrpTrackingData,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData
    )
{
}

VOID
IovpAssertFinalIrpStack(
    IN PIOV_REQUEST_PACKET  IrpTrackingData,
    IN PIO_STACK_LOCATION   IrpSp
    )
{
    ASSERT(!IrpTrackingData->RefTrackingCount);
}

VOID
IovpAssertNewRequest(
    IN PIOV_REQUEST_PACKET  IrpTrackingData,
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIO_STACK_LOCATION   IrpLastSp OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData
    )
{
    PIRP irp = IrpTrackingData->TrackedIrp;
    NTSTATUS currentStatus, lastStatus;
    BOOLEAN newRequest, statusChanged, infoChanged, firstRequest;
    PDEVICE_OBJECT possiblePdo;
    ULONG doeFlags;
    PDRIVER_OBJECT driverObject;
    PDEVICE_NODE deviceNode;
    UCHAR ansiBuffer[ 256 ];
    PDEVICE_CAPABILITIES deviceCapabilities;

    currentStatus = irp->IoStatus.Status;
    lastStatus = StackLocationData->RequestsFirstStackLocation->LastStatusBlock.Status;
    statusChanged = (currentStatus != lastStatus);
    infoChanged = (irp->IoStatus.Information != StackLocationData->RequestsFirstStackLocation->LastStatusBlock.Information);
    firstRequest = ((StackLocationData->RequestsFirstStackLocation->Flags&STACKFLAG_FIRST_REQUEST) != 0);

    if ((IrpTrackingData->Flags&TRACKFLAG_IO_ALLOCATED)&&
        (!(IrpTrackingData->Flags&TRACKFLAG_WATERMARKED))) {

        if (IovpIsSystemRestrictedIrp(IrpSp)) {

            //
            // We've caught somebody initiating an IRP they shouldn't be sending!
            //
            // ADRIAO BUGBUG 01/02/1999 -
            //     This can't be enabled until all the restricted system IRP's
            // in the kernel have been watermarked!
            //
#if 0
            WDM_FAIL_CALLER4(
                (DCERROR_RESTRICTED_IRP, DCPARAM_IRP, irp)
                );
#endif
        }
    }

    //
    // Verify new IRPs start out life accordingly
    //
    switch(IrpSp->MajorFunction) {

        case IRP_MJ_PNP:

            if (currentStatus!=STATUS_NOT_SUPPORTED) {

                //
                // This is a special WDM (9x) compatibility hack.
                //
                if (IrpSp->MinorFunction != IRP_MN_FILTER_RESOURCE_REQUIREMENTS) {

                    WDM_FAIL_CALLER4(
                        (DCERROR_PNP_IRP_BAD_INITIAL_STATUS, DCPARAM_IRP, irp)
                        );
                }

                //
                // Don't blame anyone else for this guy's mistake.
                //
                if (!NT_SUCCESS(currentStatus)) {

                    IrpTrackingData->Flags |= TRACKFLAG_PASSED_FAILURE;
                }
            }

            if (IrpSp->MinorFunction == IRP_MN_QUERY_CAPABILITIES) {

                deviceCapabilities = IrpSp->Parameters.DeviceCapabilities.Capabilities;

                if (IopIsMemoryRangeReadable(deviceCapabilities, sizeof(DEVICE_CAPABILITIES))) {

                    //
                    // Verify fields are initialized correctly
                    //
                    if (deviceCapabilities->Version < 1) {

                        //
                        // Whoops, it didn't initialize the version correctly!
                        //
                        WDM_FAIL_CALLER4(
                            (DCERROR_PNP_QUERY_CAP_BAD_VERSION, DCPARAM_IRP, irp)
                            );

                    }

                    if (deviceCapabilities->Size < sizeof(DEVICE_CAPABILITIES)) {

                        //
                        // Whoops, it didn't initialize the size field correctly!
                        //
                        WDM_FAIL_CALLER4(
                            (DCERROR_PNP_QUERY_CAP_BAD_SIZE, DCPARAM_IRP, irp)
                            );
                    }

                    if (deviceCapabilities->Address != (ULONG) -1) {

                        //
                        // Whoops, it didn't initialize the address field correctly!
                        //
                        WDM_FAIL_CALLER4(
                            (DCERROR_PNP_QUERY_CAP_BAD_ADDRESS, DCPARAM_IRP, irp)
                            );
                    }

                    if (deviceCapabilities->UINumber != (ULONG) -1) {

                        //
                        // Whoops, it didn't initialize the UI number field correctly!
                        //
                        WDM_FAIL_CALLER4(
                            (DCERROR_PNP_QUERY_CAP_BAD_UI_NUM, DCPARAM_IRP, irp)
                            );
                    }
                }
            }

            break;

        case IRP_MJ_POWER:
            if (currentStatus!=STATUS_NOT_SUPPORTED) {

                WDM_FAIL_CALLER6(
                    (DCERROR_POWER_IRP_BAD_INITIAL_STATUS, DCPARAM_IRP, irp)
                    );

                //
                // Don't blame anyone else for this guy's mistake.
                //
                if (!NT_SUCCESS(currentStatus)) {

                    IrpTrackingData->Flags |= TRACKFLAG_PASSED_FAILURE;
                }
            }
            break;

        case IRP_MJ_SYSTEM_CONTROL:

            if (currentStatus!=STATUS_NOT_SUPPORTED) {

                WDM_FAIL_CALLER4(
                    (DCERROR_WMI_IRP_BAD_INITIAL_STATUS, DCPARAM_IRP, irp)
                    );

                //
                // Don't blame anyone else for this guy's mistake.
                //
                if (!NT_SUCCESS(currentStatus)) {

                    IrpTrackingData->Flags |= TRACKFLAG_PASSED_FAILURE;
                }
            }
            break;

        default:
            break;
    }

    //
    // If this is a target device relation IRP, verify the appropriate
    // object will be referenced.
    //
    if ((IrpSp->MajorFunction == IRP_MJ_PNP)&&
        (IrpSp->MinorFunction == IRP_MN_QUERY_DEVICE_RELATIONS)&&
        (IrpSp->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation)) {

        possiblePdo = IovpGetLowestDevice(DeviceObject);
        if (possiblePdo) {

            if ((possiblePdo->DeviceObjectExtension->DeviceNode)&&(StackLocationData->ReferencingObject == NULL)) {

                //
                // Got'm!
                //
                StackLocationData->Flags |= STACKFLAG_CHECK_FOR_REFERENCE;
                StackLocationData->ReferencingObject = possiblePdo;
                StackLocationData->ReferencingCount = IovpStartObRefMonitoring(possiblePdo);
                IrpTrackingData->RefTrackingCount++;
            }

            //
            // Free our reference (we will have one if we are snapshotting anyway)
            //
            ObDereferenceObject(possiblePdo);
        }
    }

/*
    //
    // Print out some stuff...
    //
    if (IrpSp->MajorFunction == IRP_MJ_PNP) {
        switch(IrpSp->MinorFunction) {

            case IRP_MN_QUERY_DEVICE_RELATIONS:
                if (IrpSp->Parameters.QueryDeviceRelations.Type != BusRelations) {
                    break;
                }

                //
                // Fall through...
                //

            case IRP_MN_START_DEVICE:
            case IRP_MN_REMOVE_DEVICE:
            case IRP_MN_STOP_DEVICE:
                possiblePdo = IovpGetLowestDevice(DeviceObject) ;
                deviceNode = (PDEVICE_NODE) possiblePdo->DeviceObjectExtension->DeviceNode ;

                if (deviceNode) {

                    strcpy(ansiBuffer, "<not set>") ;

                    if (deviceNode->InstancePath.Buffer) {

                        KeBugCheckUnicodeToAnsi( &deviceNode->InstancePath, ansiBuffer, sizeof( ansiBuffer ));
                    }

                    DbgPrint("PNP: %s ( %08lx) initiating IRP_MJ_PNP.%s\n",
                        ansiBuffer,
                        possiblePdo,
                        PnPIrpNames[IrpSp->MinorFunction]
                        );
                }

                ObDereferenceObject(possiblePdo) ;
                break;
            default:
                break;
        }
    }
*/
}

VOID
IovpAssertIrpStackDownward(
    IN PIOV_REQUEST_PACKET  IrpTrackingData,
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIO_STACK_LOCATION   IrpLastSp OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData
    )
{
    PIRP irp = IrpTrackingData->TrackedIrp;
    NTSTATUS currentStatus, lastStatus;
    BOOLEAN newRequest, statusChanged, infoChanged, firstRequest;
    PDEVICE_OBJECT possiblePdo;
    ULONG doeFlags;
    PDRIVER_OBJECT driverObject;
    PDEVICE_NODE deviceNode;
    UCHAR ansiBuffer[ 256 ];
    PDEVICE_CAPABILITIES deviceCapabilities;
    PIOV_SESSION_DATA iovSessionData;

    currentStatus = irp->IoStatus.Status;
    lastStatus = StackLocationData->RequestsFirstStackLocation->LastStatusBlock.Status;
    statusChanged = (currentStatus != lastStatus);
    infoChanged = (irp->IoStatus.Information != StackLocationData->RequestsFirstStackLocation->LastStatusBlock.Information);
    firstRequest = ((StackLocationData->RequestsFirstStackLocation->Flags&STACKFLAG_FIRST_REQUEST) != 0);
    iovSessionData = IovpTrackingDataGetCurrentSessionData(IrpTrackingData);

    //
    // Do we have a "new" function to process?
    //
    newRequest = IovpAssertIsNewRequest(IrpLastSp, IrpSp);

    //
    // Verify the IRP was forwarded properly
    //
    switch(iovSessionData->ForwardMethod) {

        case SKIPPED_A_DO:

            switch(IrpSp->MajorFunction) {
                case IRP_MJ_PNP:
                case IRP_MJ_SYSTEM_CONTROL:

                    WDM_FAIL_CALLER4(
                        (DCERROR_SKIPPED_DEVICE_OBJECT, DCPARAM_IRP, irp)
                        );

                    break;
                case IRP_MJ_POWER:
                    //
                    // Unwind back through PoCallDriver...
                    //
                    WDM_FAIL_CALLER6(
                        (DCERROR_SKIPPED_DEVICE_OBJECT, DCPARAM_IRP, irp)
                        );

                    break;

                default:
                    break;

            }
            break;

        case STARTED_TOP_OF_STACK:
        case FORWARDED_TO_NEXT_DO:
            //
            // Perfectly normal
            //
            break;

        case STARTED_INSIDE_STACK:
            //
            // Probably an Internal irp (query cap's, etc)
            //
            break;

        case CHANGED_STACKS_MID_STACK:
            //ASSERT(0);
            break ;

        case CHANGED_STACKS_AT_BOTTOM:

            //
            // This is scary as we are very likely to run out of
            // stack locations. In theory, it is safe to do this if one
            // is careful though...
            //
            // ADRIAO BUGBUG ?? - I should check to see if there are
            //                    enough theoritical stack locations:
            //                    If there are we may be OK. Also
            //                    verify the "jump" in stack loc's from
            //                    the lower dude (5 to 8 instead of 5 to 6)
            //                    account for the stack size of the
            //                    "outbound" forward (ie, 2)
#if 0
            switch(IrpSp->MajorFunction) {
                case IRP_MJ_PNP:
                    ASSERT(0);
            }
#endif
            break ;
    }

    //
    // For some IRP major's going down a stack, there *must* be a handler
    //
    driverObject = DeviceObject->DriverObject;
    switch(IrpSp->MajorFunction) {

        case IRP_MJ_PNP:
            //
            // Umm, this would be waaay too bizarre to fail. Actually, video
            // miniports do this. Yick.
            //
#if 0
            ASSERT((driverObject->MajorFunction[IrpSp->MajorFunction] != IopInvalidDeviceRequest) ||
                    ((DeviceObject->DeviceObjectExtension->DeviceNode == NULL) &&
                    (DeviceObject->DeviceObjectExtension->AttachedTo == NULL)));
#endif
            break;

        case IRP_MJ_POWER:
        case IRP_MJ_SYSTEM_CONTROL:
            if (driverObject->MajorFunction[IrpSp->MajorFunction] == IopInvalidDeviceRequest) {

                WDM_FAIL_ROUTINE((
                    DCERROR_MISSING_DISPATCH_FUNCTION,
                    DCPARAM_IRP + DCPARAM_ROUTINE,
                    irp,
                    driverObject->MajorFunction[IRP_MJ_PNP]
                    ));

                StackLocationData->Flags |= STACKFLAG_NO_HANDLER;
            }
            break;

        default:
            break;
    }

    //
    // Verify IRQL's are legal
    //
    switch(IrpSp->MajorFunction) {

        case IRP_MJ_POWER:
        case IRP_MJ_READ:
        case IRP_MJ_WRITE:

            //
            // Fall through
            //
            // ADRIAO BUGBUG #06 06/11/98 - Figure out when the next two are
            // restricted to IRQL<DISPATCH_LEVEL...
            //
        case IRP_MJ_DEVICE_CONTROL:
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
            break;
        default:
            if (iovSessionData->ForwardMethod != FORWARDED_TO_NEXT_DO) {
                break;
            }

            if ((IrpTrackingData->CallerIrql >= DISPATCH_LEVEL) &&
                (!(IrpTrackingData->Flags & TRACKFLAG_PASSED_AT_BAD_IRQL))) {

                WDM_FAIL_CALLER4(
                    (DCERROR_DISPATCH_CALLED_AT_BAD_IRQL, DCPARAM_IRP, irp)
                    );

                IrpTrackingData->Flags |= TRACKFLAG_PASSED_AT_BAD_IRQL;
            }
    }

    //
    // The following is only executed if we are not a new IRP...
    //
    if (IrpLastSp == NULL) {
        return;
    }

    //
    // Let's verify bogus IRPs haven't been touched...
    //
    // BUGBUG ADRIAO ?? - Invent better Info/Status/Func memory...
    //
    if (IrpTrackingData->Flags&TRACKFLAG_BOGUS) {

        if (newRequest && (!firstRequest)) {

            WDM_FAIL_CALLER4(
                (DCERROR_BOGUS_FUNC_TRASHED, DCPARAM_IRP, irp)
                );
        }

        if (statusChanged) {

            if (IrpSp->MinorFunction == 0xFF) {

                WDM_FAIL_CALLER4(
                    (DCERROR_BOGUS_MINOR_STATUS_TRASHED, DCPARAM_IRP, irp)
                    );

            } else {

                WDM_FAIL_CALLER4(
                    (DCERROR_BOGUS_STATUS_TRASHED, DCPARAM_IRP, irp)
                    );
            }

        }

        if (infoChanged) {

            WDM_FAIL_CALLER4(
                (DCERROR_BOGUS_INFO_TRASHED, DCPARAM_IRP, irp)
                );
        }
    }

    //
    // Verify PnP IRPs
    //
    // ADRIAO BUGBUG 01/02/1999 -
    //     This doesn't quite work right for RAW PDO's, as they in truth have
    // no one to IoCallDriver so we never see those IRPs. This needs to be
    // *request* based in the upward code...
    //
    if (IrpSp->MajorFunction == IRP_MJ_PNP) {

        //
        // The only legit failure code to pass down is STATUS_NOT_SUPPORTED
        //
        if ((!NT_SUCCESS(currentStatus)) && (currentStatus != STATUS_NOT_SUPPORTED) &&
            (!(IrpTrackingData->Flags&TRACKFLAG_PASSED_FAILURE))) {

            WDM_FAIL_CALLER4(
                (DCERROR_PNP_FAILURE_FORWARDED, DCPARAM_IRP, irp)
                );

            //
            // Don't blame anyone else for this dude's mistakes...
            //
            IrpTrackingData->Flags |= TRACKFLAG_PASSED_FAILURE;
        }

        //
        // Status of a PnP IRP may not be converted to
        // STATUS_NOT_SUPPORTED on the way down
        //
        if ((currentStatus == STATUS_NOT_SUPPORTED)&&statusChanged) {

            WDM_FAIL_CALLER4(
                (DCERROR_PNP_IRP_STATUS_RESET, DCPARAM_IRP, irp)
                );
        }

        //
        // Some IRPs FDO's are required to handle before passing down. And
        // some IRPs should not be touched by the FDO. Assert it is so...
        //
        if (iovSessionData->DeviceLastCalled) {
            doeFlags = iovSessionData->DeviceLastCalled->DeviceObjectExtension->ExtensionFlags;
        } else {
            doeFlags = 0;
        }

        //
        // How could a Raw FDO (aka a PDO) get here? Well, a PDO could forward
        // to another stack if he's purposely reserved enough stack locations
        // for that eventuality...
        //
        //ASSERT(!(doeFlags&DOE_RAW_FDO));

        if (doeFlags&DOE_DESIGNATED_FDO) {

            switch(IrpSp->MinorFunction) {

                case IRP_MN_SURPRISE_REMOVAL:
                    //
                    // ADRIAO BUGBUG 01/22/1999 -
                    //     We are exempting this IRP from support
                    // (ie, voting success or failure) because it is
                    // late in the product. This is in my opinion quite
                    // braindead, but so be it.
                    //
                    break;
                case IRP_MN_START_DEVICE:
                case IRP_MN_QUERY_REMOVE_DEVICE:
                case IRP_MN_REMOVE_DEVICE:
                case IRP_MN_CANCEL_REMOVE_DEVICE:
                case IRP_MN_STOP_DEVICE:
                case IRP_MN_QUERY_STOP_DEVICE:
                case IRP_MN_CANCEL_STOP_DEVICE:

                    //
                    // The FDO must set the status as appropriate. If he set a
                    // completion routine, he can do it there (and thus we cannot
                    // check). If he has not though, we can indeed verify he has
                    // responded to the IRP!
                    //
                    if ((currentStatus == STATUS_NOT_SUPPORTED)&&(!IrpSp->CompletionRoutine)) {

                        WDM_FAIL_CALLER4(
                            (DCERROR_PNP_IRP_NEEDS_FDO_HANDLING, DCPARAM_IRP, irp)
                            );
                    }
                    break ;
                case IRP_MN_QUERY_DEVICE_RELATIONS:
                    switch(IrpSp->Parameters.QueryDeviceRelations.Type) {
                        case TargetDeviceRelation:
                            if ((currentStatus != STATUS_NOT_SUPPORTED)&&(!(doeFlags&DOE_RAW_FDO))) {

                                WDM_FAIL_CALLER4(
                                    (DCERROR_PNP_IRP_FDO_HANDS_OFF, DCPARAM_IRP, irp)
                                    );
                            }
                            break;
                       case BusRelations:
                       case PowerRelations:
                       case RemovalRelations:

                       case EjectionRelations:

                           //
                           // Ejection relations are usually a bad idea for
                           // FDO's - As stopping a device implies powerdown,
                           // RemovalRelations are usually the proper response
                           // for an FDO. One exception is ISAPNP, as PCI-to-ISA
                           // bridges can never be powered down.
                           //

                       default:
                           break;
                    }
                    break;
                case IRP_MN_QUERY_INTERFACE:
                case IRP_MN_QUERY_CAPABILITIES:
                case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
                    break;
                case IRP_MN_QUERY_DEVICE_TEXT:
                case IRP_MN_READ_CONFIG:
                case IRP_MN_WRITE_CONFIG:
                case IRP_MN_EJECT:
                case IRP_MN_SET_LOCK:
                case IRP_MN_QUERY_RESOURCES:
                case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
                case IRP_MN_QUERY_BUS_INFORMATION:
                    if ((currentStatus != STATUS_NOT_SUPPORTED)&&(!(doeFlags&DOE_RAW_FDO))) {

                        WDM_FAIL_CALLER4(
                            (DCERROR_PNP_IRP_FDO_HANDS_OFF, DCPARAM_IRP, irp)
                            );
                    }
                    break;
                case IRP_MN_QUERY_ID:
                    switch(IrpSp->Parameters.QueryId.IdType) {

                        case BusQueryDeviceID:
                        case BusQueryHardwareIDs:
                        case BusQueryCompatibleIDs:
                        case BusQueryInstanceID:
                            if ((currentStatus != STATUS_NOT_SUPPORTED)&&(!(doeFlags&DOE_RAW_FDO))) {

                                WDM_FAIL_CALLER4(
                                    (DCERROR_PNP_IRP_FDO_HANDS_OFF, DCPARAM_IRP, irp)
                                    );
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case IRP_MN_QUERY_PNP_DEVICE_STATE:
                case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:
                    break;
                case IRP_MN_DEVICE_USAGE_NOTIFICATION:
                    //
                    // ADRIAO BUGBUG 09/25/98 - Is this really optional for an FDO?
                    //
                    break;
                default:
                    break;
            }
        }
    }

    //
    // Verify Power IRPs
    //
    if (IrpSp->MajorFunction == IRP_MJ_POWER) {

        //
        // The only legit failure code to pass down is STATUS_NOT_SUPPORTED
        //
        if ((!NT_SUCCESS(currentStatus)) && (currentStatus != STATUS_NOT_SUPPORTED) &&
            (!(IrpTrackingData->Flags&TRACKFLAG_PASSED_FAILURE))) {

            WDM_FAIL_CALLER6(
                (DCERROR_POWER_FAILURE_FORWARDED, DCPARAM_IRP, irp)
                );

            //
            // Don't blame anyone else for this dude's mistakes...
            //
            IrpTrackingData->Flags |= TRACKFLAG_PASSED_FAILURE;
        }

        //
        // Status of a Power IRP may not be converted to
        // STATUS_NOT_SUPPORTED on the way down
        //
        if ((currentStatus == STATUS_NOT_SUPPORTED)&&statusChanged) {

            WDM_FAIL_CALLER6(
                (DCERROR_POWER_IRP_STATUS_RESET, DCPARAM_IRP, irp)
                );
        }
    }

    if (!IovpAssertIsValidIrpStatus(IrpSp, currentStatus)) {

        switch(IrpSp->MajorFunction) {
            case IRP_MJ_POWER:

                WDM_FAIL_CALLER6((DCERROR_INVALID_STATUS, DCPARAM_IRP, irp));
                break;

            default:

                WDM_FAIL_CALLER4((DCERROR_INVALID_STATUS, DCPARAM_IRP, irp));
                break;
        }
    }
}

VOID
IovpAssertIrpStackUpward(
    IN PIOV_REQUEST_PACKET  IrpTrackingData,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN BOOLEAN              IsNewlyCompleted,
    IN BOOLEAN              RequestFinalized
    )
{
    PIRP irp = IrpTrackingData->TrackedIrp;
    NTSTATUS currentStatus, lastStatus;
    BOOLEAN mustPassDown, isBogusIrp, isPdo, statusChanged, infoChanged;
    PIOV_STACK_LOCATION originalRequestSLD;
    PVOID routine;
    LONG referencesTaken;
    PDEVICE_OBJECT possiblePdo;
    PDEVICE_NODE deviceNode;
    UCHAR ansiBuffer[ 256 ];

    currentStatus = irp->IoStatus.Status;
    lastStatus = StackLocationData->RequestsFirstStackLocation->LastStatusBlock.Status;
    statusChanged = (currentStatus != lastStatus);
    infoChanged = (irp->IoStatus.Information != StackLocationData->RequestsFirstStackLocation->LastStatusBlock.Information);

    //
    // Who'd we call for this one?
    //
    routine = StackLocationData->LastDispatch;
    ASSERT(routine) ;

    //
    // If this "Request" has been "Completed", perform some checks
    //
    if (IsNewlyCompleted) {

        //
        // Remember bogosity...
        //
        isBogusIrp = ((IrpTrackingData->Flags&TRACKFLAG_BOGUS)!=0);

        //
        // Is this a PDO?
        //
        isPdo = ((StackLocationData->Flags&STACKFLAG_REACHED_PDO)!=0);

        //
        // Was anything completed too early?
        // A driver may outright fail almost anything but a bogus IRP
        //
        mustPassDown = (!(StackLocationData->Flags&STACKFLAG_NO_HANDLER));
        mustPassDown &= (!isPdo);

        switch(IrpSp->MajorFunction) {

            case IRP_MJ_SYSTEM_CONTROL:
                mustPassDown &= ((PDEVICE_OBJECT) IrpSp->Parameters.WMI.ProviderId != IrpSp->DeviceObject);
                if (mustPassDown) {

                     WDM_FAIL_ROUTINE((
                         DCERROR_WMI_IRP_NOT_FORWARDED,
                         DCPARAM_IRP + DCPARAM_ROUTINE + DCPARAM_DEVOBJ,
                         irp,
                         routine,
                         IrpSp->Parameters.WMI.ProviderId
                         ));
                }
                break;

            case IRP_MJ_PNP:
                mustPassDown &= (isBogusIrp || NT_SUCCESS(currentStatus) || (currentStatus == STATUS_NOT_SUPPORTED));
                if (mustPassDown) {

                    //
                    // Print appropriate error message
                    //
                    if (IrpTrackingData->Flags&TRACKFLAG_BOGUS) {

                        WDM_FAIL_ROUTINE((
                            DCERROR_BOGUS_PNP_IRP_COMPLETED,
                            DCPARAM_IRP + DCPARAM_ROUTINE,
                            irp,
                            routine
                            ));

                    } else if (NT_SUCCESS(currentStatus)) {

                        WDM_FAIL_ROUTINE((
                            DCERROR_SUCCESSFUL_PNP_IRP_NOT_FORWARDED,
                            DCPARAM_IRP + DCPARAM_ROUTINE,
                            irp,
                            routine
                            ));

                    } else if (currentStatus == STATUS_NOT_SUPPORTED) {

                        WDM_FAIL_ROUTINE((
                            DCERROR_UNTOUCHED_PNP_IRP_NOT_FORWARDED,
                            DCPARAM_IRP + DCPARAM_ROUTINE,
                            irp,
                            routine
                            ));
                    }
                }
                break;

            case IRP_MJ_POWER:
                mustPassDown &= (isBogusIrp || NT_SUCCESS(currentStatus) || (currentStatus == STATUS_NOT_SUPPORTED));
                if (mustPassDown) {

                    //
                    // Print appropriate error message
                    //
                    if (IrpTrackingData->Flags&TRACKFLAG_BOGUS) {

                        WDM_FAIL_ROUTINE((
                            DCERROR_BOGUS_POWER_IRP_COMPLETED,
                            DCPARAM_IRP + DCPARAM_ROUTINE,
                            irp,
                            routine
                            ));

                    } else if (NT_SUCCESS(currentStatus)) {

                        WDM_FAIL_ROUTINE((
                            DCERROR_SUCCESSFUL_POWER_IRP_NOT_FORWARDED,
                            DCPARAM_IRP + DCPARAM_ROUTINE,
                            irp,
                            routine
                            ));

                    } else if (currentStatus == STATUS_NOT_SUPPORTED) {

                        WDM_FAIL_ROUTINE((
                            DCERROR_UNTOUCHED_POWER_IRP_NOT_FORWARDED,
                            DCPARAM_IRP + DCPARAM_ROUTINE,
                            irp,
                            routine
                            ));
                    }
                }
                break;
        }

        //
        // Did the PDO respond to it's required set of IRPs?
        //
        if (isPdo) {

            switch(IrpSp->MajorFunction) {

                case IRP_MJ_PNP:

                    switch(IrpSp->MinorFunction) {

                        case IRP_MN_SURPRISE_REMOVAL:

                            //
                            // ADRIAO BUGBUG 01/22/1999 -
                            //     We are exempting this IRP from support
                            // (ie, voting success or failure) because it is
                            // late in the product. This is in my opinion quite
                            // braindead, but so be it.
                            //
                            break;

                        case IRP_MN_START_DEVICE:
                        case IRP_MN_QUERY_REMOVE_DEVICE:
                        case IRP_MN_REMOVE_DEVICE:
                        case IRP_MN_CANCEL_REMOVE_DEVICE:
                        case IRP_MN_STOP_DEVICE:
                        case IRP_MN_QUERY_STOP_DEVICE:
                        case IRP_MN_CANCEL_STOP_DEVICE:
                            if (currentStatus == STATUS_NOT_SUPPORTED) {

                                WDM_FAIL_ROUTINE((
                                    DCERROR_PNP_IRP_NEEDS_PDO_HANDLING,
                                    DCPARAM_IRP + DCPARAM_ROUTINE,
                                    irp,
                                    routine
                                    ));
                            }
                            break;
                        case IRP_MN_QUERY_DEVICE_RELATIONS:
                            switch(IrpSp->Parameters.QueryDeviceRelations.Type) {
                                case BusRelations:
                                case PowerRelations:
                                case RemovalRelations:
                                case EjectionRelations:
                                    //
                                    // Optionals or FDO's
                                    //
                                    break;
                                case TargetDeviceRelation:
                                    if (currentStatus == STATUS_NOT_SUPPORTED) {

                                        WDM_FAIL_ROUTINE((
                                            DCERROR_PNP_IRP_NEEDS_PDO_HANDLING,
                                            DCPARAM_IRP + DCPARAM_ROUTINE,
                                            irp,
                                            routine
                                            ));

                                    } else if (NT_SUCCESS(currentStatus)) {

                                        if (irp->IoStatus.Information == (ULONG_PTR) NULL) {

                                            WDM_FAIL_ROUTINE((
                                                DCERROR_TARGET_RELATION_LIST_EMPTY,
                                                DCPARAM_IRP + DCPARAM_ROUTINE,
                                                irp,
                                                routine
                                                ));
                                        }

                                        //
                                        // ADRIAO BUGBUG ?? - I could also assert the Information
                                        // matches DeviceObject.
                                        //
                                    }
                                    break;
                                default:
                                    break;
                            }
                            break;
                        case IRP_MN_QUERY_INTERFACE:
                        case IRP_MN_QUERY_CAPABILITIES:
                        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
                        case IRP_MN_QUERY_DEVICE_TEXT:
                        case IRP_MN_READ_CONFIG:
                        case IRP_MN_WRITE_CONFIG:
                        case IRP_MN_EJECT:
                        case IRP_MN_SET_LOCK:
                        case IRP_MN_QUERY_RESOURCES:
                        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
                        case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:
                            break;
                        case IRP_MN_QUERY_ID:
                            switch(IrpSp->Parameters.QueryId.IdType) {

                                case BusQueryDeviceID:
                                case BusQueryHardwareIDs:
                                case BusQueryCompatibleIDs:
                                case BusQueryInstanceID:
                                default:
                                    break;
                            }
                            break ;
                        case IRP_MN_QUERY_PNP_DEVICE_STATE:
                        case IRP_MN_QUERY_BUS_INFORMATION:
                        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
                        default:
                            break ;
                    }
                default:
                    break;
            }
        }

        //
        // Was TargetDeviceRelation implemented correctly?
        //
        originalRequestSLD = StackLocationData->RequestsFirstStackLocation;

        if (originalRequestSLD->Flags&STACKFLAG_CHECK_FOR_REFERENCE) {

            ASSERT ((IrpSp->MajorFunction == IRP_MJ_PNP)&&
                (IrpSp->MinorFunction == IRP_MN_QUERY_DEVICE_RELATIONS)&&
                (IrpSp->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation));

            ASSERT(originalRequestSLD->ReferencingObject);
            ASSERT(IrpTrackingData->RefTrackingCount);

            referencesTaken = IovpStopObRefMonitoring(
                originalRequestSLD->ReferencingObject,
                originalRequestSLD->ReferencingCount
                );

            IrpTrackingData->RefTrackingCount--;
            originalRequestSLD->ReferencingObject = NULL;

            originalRequestSLD->Flags&=~STACKFLAG_CHECK_FOR_REFERENCE;

            if (NT_SUCCESS(currentStatus)&&(!referencesTaken)) {

                WDM_FAIL_ROUTINE((
                    DCERROR_TARGET_RELATION_NEEDS_REF,
                    DCPARAM_IRP + DCPARAM_ROUTINE,
                    irp,
                    routine
                    ));
            }
        }
    }

    //
    // Did anyone stomp the status erroneously?
    //
    if ((currentStatus == STATUS_NOT_SUPPORTED)&&statusChanged) {

        //
        // Status of a PnP or Power IRP may not be converted from success to
        // STATUS_NOT_SUPPORTED on the way down.
        //
        switch(IrpSp->MajorFunction) {

            case IRP_MJ_PNP:

                WDM_FAIL_ROUTINE((
                    DCERROR_PNP_IRP_STATUS_RESET,
                    DCPARAM_IRP + DCPARAM_ROUTINE,
                    irp,
                    routine
                    ));

                break;

            case IRP_MJ_POWER:

                WDM_FAIL_ROUTINE((
                    DCERROR_POWER_IRP_STATUS_RESET,
                    DCPARAM_IRP + DCPARAM_ROUTINE,
                    irp,
                    routine
                    ));

                break;
        }
    }

    //
    // Did they touch something stupid?
    //
    if (IrpTrackingData->Flags&TRACKFLAG_BOGUS) {

        if (statusChanged) {

            if (IrpSp->MinorFunction == 0xFF) {

                WDM_FAIL_ROUTINE((
                    DCERROR_BOGUS_MINOR_STATUS_TRASHED,
                    DCPARAM_IRP + DCPARAM_ROUTINE,
                    irp,
                    routine
                    ));

            } else {

                WDM_FAIL_ROUTINE((
                    DCERROR_BOGUS_STATUS_TRASHED,
                    DCPARAM_IRP + DCPARAM_ROUTINE,
                    irp,
                    routine
                    ));
            }
        }

        if (infoChanged) {

            WDM_FAIL_ROUTINE((
                DCERROR_BOGUS_INFO_TRASHED,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                irp,
                routine
                ));
        }
    }

    if (!IovpAssertIsValidIrpStatus(IrpSp, currentStatus)) {

        WDM_FAIL_ROUTINE(
            (DCERROR_INVALID_STATUS, DCPARAM_IRP + DCPARAM_ROUTINE, irp, routine)
            );
    }

/*
    //
    // Print out some stuff...
    //
    if (RequestFinalized&&(IrpSp->MajorFunction == IRP_MJ_PNP)) {
        switch(IrpSp->MinorFunction) {

            case IRP_MN_QUERY_DEVICE_RELATIONS:
                if (IrpSp->Parameters.QueryDeviceRelations.Type != BusRelations) {
                   break;
                }

                //
                // Fall through...
                //

            case IRP_MN_START_DEVICE:
            case IRP_MN_REMOVE_DEVICE:
            case IRP_MN_STOP_DEVICE:
                possiblePdo = IovpGetLowestDevice(IrpSp->DeviceObject) ;
                deviceNode = (PDEVICE_NODE) possiblePdo->DeviceObjectExtension->DeviceNode ;
                if (deviceNode) {

                    strcpy(ansiBuffer, "<not set>") ;

                    if (deviceNode->InstancePath.Buffer) {

                        KeBugCheckUnicodeToAnsi( &deviceNode->InstancePath, ansiBuffer, sizeof( ansiBuffer ));
                    }

                    DbgPrint("PNP: %s ( %08lx) completing IRP_MJ_PNP.%s with %08lx\n",
                        ansiBuffer,
                        possiblePdo,
                        PnPIrpNames[IrpSp->MinorFunction],
                        currentStatus
                        );
                }
                ObDereferenceObject(possiblePdo) ;
                break;
            default:
                break;
        }
    }
*/
}

BOOLEAN
IovpAssertIsValidIrpStatus(
    IN PIO_STACK_LOCATION IrpSp,
    IN NTSTATUS Status
    )
/*++

    Description:
        As per the title, this function determines whether an IRP status is
        valid or probably random trash. See NTStatus.h for info on how status
        codes break down...

    Returns:

        TRUE iff IRP status looks to be valid. FALSE otherwise.
--*/
{
    ULONG severity;
    ULONG customer;
    ULONG reserved;
    ULONG facility;
    ULONG code;
    ULONG lanManClass;

    severity = (((ULONG)Status) >> 30)&3;
    customer = (((ULONG)Status) >> 29)&1;
    reserved = (((ULONG)Status) >> 28)&1;
    facility = (((ULONG)Status) >> 16)&0xFFF;
    code =     (((ULONG)Status) & 0xFFFF);

    //
    // If reserved set, definitely bogus...
    //
    if (reserved) {

        return FALSE;
    }

    //
    // Is this a microsoft defined return code? If not, do no checking.
    //
    if (customer) {

        return TRUE;
    }

    //
    // ADRIAO N.B. 10/04/1999 -
    //     The current methodology for doling out error codes appears to be
    // fairly chaotic. The primary kernel mode status codes are defined in
    // ntstatus.h. However, rtl\generr.c should also be consulted to see which
    // error codes can bubble up to user mode. Many OLE error codes from
    // winerror.h are now being used within the kernel itself.
    //
    if (facility < 0x20) {

        //
        // Facilities under 20 are currently legal.
        //
        switch(severity) {
            case STATUS_SEVERITY_SUCCESS:       return (code < 0x200);
            case STATUS_SEVERITY_INFORMATIONAL:

                //
                // ADRIAO BUGBUG 10/09/1999 -
                //     This should really be 0x50, but we've been testing with
                // 0x400 for a while (and we don't want to change it just before
                // Win2k ships).
                //
                                                return (code < 0x400);
            case STATUS_SEVERITY_WARNING:       return (code < 0x400);
            case STATUS_SEVERITY_ERROR:         break;
        }

        //
        // Why the heck does WOW use such an odd error code?
        //
        return (code < 0x400)||(code == 0x9898);

    } else if (facility == 0x98) {

        //
        // This is the lan manager service. In the case on Lan Man, the code
        // field is further subdivided into a class field.
        //
        lanManClass = code >> 12;
        code &= 0xFFF;

        //
        // Do no testing here.
        //
        return TRUE;

    } else {

        //
        // Not known, probably bogus.
        //
        return FALSE;
    }
}

VOID
IovpThrowChaffAtStartedPdoStack(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

    Description:
        As per the title, we are going to throw some IRPs at the stack to
        see if they are handled correctly.

    Returns:

        Nothing
--*/

{
    IO_STACK_LOCATION irpSp;
    PDEVICE_OBJECT lowestDeviceObject;
    PDEVICE_RELATIONS targetDeviceRelationList;
    INTERFACE interface;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //
    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    //
    // send lots of bogus PNP IRPs
    //
    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = 0xff;
    IovpThrowBogusSynchronousIrp(DeviceObject, &irpSp, (ULONG_PTR) 0, NULL, TRUE);

    irpSp.MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS ;
    irpSp.Parameters.QueryDeviceRelations.Type = (DEVICE_RELATION_TYPE) -1 ;
    IovpThrowBogusSynchronousIrp(DeviceObject, &irpSp, (ULONG_PTR) 0, NULL, TRUE);

#ifdef HACKHACKS_ENABLED
    if (!(IovpHackFlags&HACKFLAG_FOR_ACPI)) {
#endif
        //
        // ADRIAO HACKHACK 08/16/1999 - Fix ACPI to enable more chaff
        //
        irpSp.MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS ;
        irpSp.Parameters.QueryDeviceRelations.Type = (DEVICE_RELATION_TYPE) -1 ;
        IovpThrowBogusSynchronousIrp(DeviceObject, &irpSp, (ULONG_PTR) -1, NULL, TRUE) ;
#ifdef HACKHACKS_ENABLED
    }
#endif

    irpSp.MinorFunction = IRP_MN_QUERY_DEVICE_TEXT ;
    irpSp.Parameters.QueryDeviceText.DeviceTextType = (DEVICE_TEXT_TYPE) -1 ;
    IovpThrowBogusSynchronousIrp(DeviceObject, &irpSp, (ULONG_PTR) 0, NULL, TRUE);

    irpSp.MinorFunction = IRP_MN_QUERY_ID ;
    irpSp.Parameters.QueryId.IdType = (BUS_QUERY_ID_TYPE) -1 ;
    IovpThrowBogusSynchronousIrp(DeviceObject, &irpSp, (ULONG_PTR) 0, NULL, TRUE);

#ifdef HACKHACKS_ENABLED
    if (!(IovpHackFlags&HACKFLAG_FOR_BOGUSIRPS)) {
#endif

        //
        // Send a bogus WMI IRP
        //
        // Note that we aren't sending this IRP to any stack that doesn't terminate
        // with a devnode. The WmiSystemControl export from WmiLib says
        // "NotWmiIrp if it sees these. The callers should still pass down the
        // IRP.
        //
        lowestDeviceObject = IovpGetLowestDevice(DeviceObject) ;
        ASSERT(lowestDeviceObject) ;
        ASSERT(lowestDeviceObject->DeviceObjectExtension->DeviceNode) ;

        irpSp.MajorFunction = IRP_MJ_SYSTEM_CONTROL ;
        irpSp.MinorFunction = 0xff;
        irpSp.Parameters.WMI.ProviderId = (ULONG_PTR) lowestDeviceObject ;
        IovpThrowBogusSynchronousIrp(DeviceObject, &irpSp, (ULONG_PTR) 0, NULL, TRUE);
        ObDereferenceObject(lowestDeviceObject) ;

        //
        // And a bogus Power IRP
        //
        irpSp.MajorFunction = IRP_MJ_POWER ;
        irpSp.MinorFunction = 0xff;
        IovpThrowBogusSynchronousIrp(DeviceObject, &irpSp, (ULONG_PTR) 0, NULL, TRUE);

#ifdef HACKHACKS_ENABLED
    }
#endif

    //
    // Target device relation test...
    //
    irpSp.MajorFunction = IRP_MJ_PNP ;
    irpSp.MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS ;
    irpSp.Parameters.QueryDeviceRelations.Type = TargetDeviceRelation ;
    targetDeviceRelationList = NULL ;
    status = IovpThrowBogusSynchronousIrp(
        DeviceObject,
        &irpSp,
        (ULONG_PTR) 0,
        (ULONG_PTR *) &targetDeviceRelationList,
        FALSE
        );

    if (NT_SUCCESS(status)) {

        ASSERT(targetDeviceRelationList) ;
        ASSERT(targetDeviceRelationList->Count == 1) ;
        ASSERT(targetDeviceRelationList->Objects[0]) ;
        ObDereferenceObject(targetDeviceRelationList->Objects[0]) ;
        ExFreePool(targetDeviceRelationList) ;

    } else {

        //
        // IRP was asserted in other code. We need to do nothing here...
        //
    }

    RtlZeroMemory(&interface, sizeof(INTERFACE));
    irpSp.MinorFunction = IRP_MN_QUERY_INTERFACE;
    irpSp.Parameters.QueryInterface.Size = -1;
    irpSp.Parameters.QueryInterface.Version = 1;
    irpSp.Parameters.QueryInterface.InterfaceType = &GUID_BOGUS_INTERFACE;
    irpSp.Parameters.QueryInterface.Interface = &interface;
    irpSp.Parameters.QueryInterface.InterfaceSpecificData = (PVOID) -1;
    IovpThrowBogusSynchronousIrp(DeviceObject, &irpSp, (ULONG_PTR) 0, NULL, TRUE);

    //
    // ADRIAO BUGBUG #03 06/05/98 - Need more chaff. Very much more.
    // For example, bogus device usage notifications, etc...
    //
}

NTSTATUS
IovpThrowBogusSynchronousIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_STACK_LOCATION TopStackLocation,
    IN OUT OPTIONAL ULONG_PTR Information,
    IN OUT ULONG_PTR *InformationOut OPTIONAL,
    IN BOOLEAN IsBogus
    )

/*++

Routine Description:

    This function sends a synchronous irp to the top level device
    object which roots on DeviceObject. It differs from IopSynchronousIrp
    in that it sets the IRP_DIAG_IS_BOGUS flag, and passes in possibly
    nonzero information.

Parameters:

    DeviceObject - Supplies the device object of the device being removed.

    TopStackLocation - Supplies a pointer to the parameter block for the irp.

Return Value:

    NTSTATUS code.

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    IO_STATUS_BLOCK statusBlock;
    KEVENT event;
    NTSTATUS status;
    PDEVICE_OBJECT topDeviceObject;

    PAGED_CODE();

    //
    // Get a pointer to the topmost device object in the stack of devices,
    // beginning with the deviceObject.
    //

    topDeviceObject = IoGetAttachedDeviceReference(DeviceObject);

    //
    // Begin by allocating the IRP for this request.  Do not charge quota to
    // the current process for this IRP.
    //

    irp = IoAllocateIrp(topDeviceObject->StackSize, FALSE);
    if (irp == NULL){

        ObDereferenceObject(topDeviceObject) ;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (IsBogus) {

        SPECIALIRP_WATERMARK_IRP(irp, IRP_BOGUS);
    }

    //
    // Initialize it to failure.
    //
    irp->IoStatus.Status = statusBlock.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = statusBlock.Information = Information;

    //
    // Set the pointer to the status block and initialized event.
    //

    KeInitializeEvent( &event,
                       SynchronizationEvent,
                       FALSE );

    irp->UserIosb = &statusBlock;
    irp->UserEvent = &event;

    //
    // Set the address of the current thread (for debugging)
    //
    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    // Queue this irp onto the current thread. We do this because we are not
    // collecting this IRP with a STATUS_MORE_PROCESSING_REQUIRED completion
    // routine - an APC will be scheduled and we *must* give it a thread.
    //
    IopQueueThreadIrp(irp);

    //
    // Get a pointer to the stack location of the first driver which will be
    // invoked.  This is where the function codes and parameters are set.
    //

    irpSp = IoGetNextIrpStackLocation(irp);

    //
    // Copy in the caller-supplied stack location contents
    //

    *irpSp = *TopStackLocation;

    //
    // Call the driver
    //

    status = IoCallDriver(topDeviceObject, irp);
    ObDereferenceObject(topDeviceObject) ;

    //
    // If a driver returns STATUS_PENDING, we will wait for it to complete
    //
    if (status == STATUS_PENDING) {
        (VOID) KeWaitForSingleObject( &event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER) NULL );
        status = statusBlock.Status;
    }

    if (InformationOut) {

        *InformationOut = statusBlock.Information ;
    }

    return status;
}

LONG
IovpStartObRefMonitoring(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

  Description:

     Determines if ObRef has not been called between a call to this
     function and a subsequent call to IovpStopObRefMonitoring.

  Arguments:

     Device object to monitor.

  Return Value:

     A start skew time to pass into IovpStopObRefMonitoring.

     N.B. - A reference count is taken by this API and released
            by IovpStopObRefMonitoring. That reference is not
            counted among the noticed calls to ObRef.
--*/
{
    //
    // ADRIAO BUGBUG 10/05/1999 -
    //     Parclass reroutes target-device-relation IRPs around the tree. That
    // design will be fixed for NT5.1, but in the meantime this means we will
    // not be able to test this driver functionality.
    //
#if  0
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_NAME_INFO NameInfo;
    LONG startSkew, pointerCount ;

    ObReferenceObject(DeviceObject) ;

    ObjectHeader = OBJECT_TO_OBJECT_HEADER(DeviceObject);
    NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );

    ASSERT(NameInfo) ;
    //
    // We will always decrement DbgDereferenceCount prior to PointerCount,
    // so any race conditions will look like an increment occured, which
    // is an allowable misread...
    //
    do {
        pointerCount = ObjectHeader->PointerCount ;
        startSkew = pointerCount - NameInfo->DbgDereferenceCount ;

    } while(pointerCount != ObjectHeader->PointerCount) ;

    return startSkew ;
#else
    return 1;
#endif
}

LONG
IovpStopObRefMonitoring(
    IN PDEVICE_OBJECT DeviceObject,
    IN LONG StartSkew
    )
/*++

  Description:

     Determines if ObRef has not been called between a call to
     IovpStartObRefMonitoring and a call to this API.

     In a race condition (say ObDereferenceObject is ran in-simo
     with this function), the return is gaurenteed to err on
     the side of a reference occuring.

  Arguments:

     Device Object and the skew returned by IovpStartObRefMonitoring

  Return Value:

     Number of calls to ObRef that occured throughout the monitored timeframe.
     Note that the return could be positive even though the reference count
     actually dropped (ie, one ObRef and two ObDeref's).

--*/
{
    //
    // ADRIAO BUGBUG 10/05/1999 -
    //     Parclass reroutes target-device-relation IRPs around the tree. That
    // design will be fixed for NT5.1, but in the meantime this means we will
    // not be able to test this driver functionality.
    //
#if  0
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_NAME_INFO NameInfo;
    LONG currentSkew, refDelta, pointerCount ;

    ObjectHeader = OBJECT_TO_OBJECT_HEADER(DeviceObject);
    NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );

    ASSERT(NameInfo) ;

    //
    // We will always decrement DbgDereferenceCount prior to PointerCount,
    // so any race conditions will look like an increment occured, which
    // is an allowable misread...
    //
    do {
        pointerCount = ObjectHeader->PointerCount ;
        currentSkew = pointerCount - NameInfo->DbgDereferenceCount ;

    } while(pointerCount != ObjectHeader->PointerCount) ;

    refDelta = currentSkew - StartSkew ;
    ASSERT(refDelta>=0) ;

    ObDereferenceObject(DeviceObject) ;

    return refDelta ;
#else
    return 1;
#endif
}

BOOLEAN
IovpIsSystemRestrictedIrp(
    PIO_STACK_LOCATION IrpSp
    )
{
    switch(IrpSp->MajorFunction) {

        case IRP_MJ_PNP:
            switch(IrpSp->MinorFunction) {
                case IRP_MN_START_DEVICE:
                case IRP_MN_QUERY_REMOVE_DEVICE:
                case IRP_MN_REMOVE_DEVICE:
                case IRP_MN_CANCEL_REMOVE_DEVICE:
                case IRP_MN_STOP_DEVICE:
                case IRP_MN_QUERY_STOP_DEVICE:
                case IRP_MN_CANCEL_STOP_DEVICE:
                case IRP_MN_SURPRISE_REMOVAL:
                    return TRUE;

                case IRP_MN_QUERY_DEVICE_RELATIONS:
                    switch(IrpSp->Parameters.QueryDeviceRelations.Type) {
                        case BusRelations:
                        case PowerRelations:
                            return TRUE;
                        case RemovalRelations:
                        case EjectionRelations:
                        case TargetDeviceRelation:
                            return FALSE;
                        default:
                            break;
                    }
                    break;
                case IRP_MN_QUERY_INTERFACE:
                case IRP_MN_QUERY_CAPABILITIES:
                    return FALSE;
                case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
                case IRP_MN_QUERY_DEVICE_TEXT:
                    return TRUE;
                case IRP_MN_READ_CONFIG:
                case IRP_MN_WRITE_CONFIG:
                    return FALSE;
                case IRP_MN_EJECT:
                case IRP_MN_SET_LOCK:
                case IRP_MN_QUERY_RESOURCES:
                case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
                case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:
                    return TRUE;
                case IRP_MN_QUERY_ID:
                    switch(IrpSp->Parameters.QueryId.IdType) {

                        case BusQueryHardwareIDs:
                        case BusQueryCompatibleIDs:
                            return TRUE;
                        case BusQueryDeviceID:
                        case BusQueryInstanceID:
                            return FALSE;
                        default:
                            break;
                    }
                    break ;
                case IRP_MN_QUERY_PNP_DEVICE_STATE:
                case IRP_MN_QUERY_BUS_INFORMATION:
                    return TRUE;
                case IRP_MN_DEVICE_USAGE_NOTIFICATION:
                    return FALSE;
                default:
                    break ;
            }

        case IRP_MJ_POWER:
            switch(IrpSp->MinorFunction) {
                case IRP_MN_POWER_SEQUENCE:
                    return FALSE;
                case IRP_MN_QUERY_POWER:
                case IRP_MN_SET_POWER:
                case IRP_MN_WAIT_WAKE:
                    return TRUE;
                default:
                    break;
            }
        default:
            return FALSE;
    }

    return TRUE;
}

#endif // NO_SPECIAL_IRP


