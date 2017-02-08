/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    autorun.c

Abstract:

    Code for support of media change detection in the class driver

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#include "classp.h"

#include <wmidata.h>

#define GESN_TIMEOUT_VALUE (0x4)
#define GESN_BUFFER_SIZE (0x8)
#define MAXIMUM_IMMEDIATE_MCN_RETRIES (0x20)
#define MCN_REG_SUBKEY_NAME                   (L"MediaChangeNotification")
#define MCN_REG_AUTORUN_DISABLE_INSTANCE_NAME (L"AlwaysDisableMCN")
#define MCN_REG_AUTORUN_ENABLE_INSTANCE_NAME  (L"AlwaysEnableMCN")

GUID StoragePredictFailureEventGuid = WMI_STORAGE_PREDICT_FAILURE_EVENT_GUID;

//
// Only send polling irp when device is fully powered up and a
// power down irp is not in progress.
//
// NOTE:   This helps close a window in time where a polling irp could cause
//         a drive to spin up right after it has powered down. The problem is
//         that SCSIPORT, ATAPI and SBP2 will be in the process of powering
//         down (which may take a few seconds), but won't know that. It would
//         then get a polling irp which will be put into its queue since it
//         the disk isn't powered down yet. Once the disk is powered down it
//         will find the polling irp in the queue and then power up the
//         device to do the poll. They do not want to check if the polling
//         irp has the SRB_NO_KEEP_AWAKE flag here since it is in a critical
//         path and would slow down all I/Os. A better way to fix this
//         would be to serialize the polling and power down irps so that
//         only one of them is sent to the device at a time.
//
#define ClasspCanSendPollingIrp(fdoExtension)                           \
               ((fdoExtension->DevicePowerState == PowerDeviceD0) &&  \
                (! fdoExtension->PowerDownInProgress) )

BOOLEAN
NTAPI
ClasspIsMediaChangeDisabledDueToHardwareLimitation(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
NTAPI
ClasspMediaChangeDeviceInstanceOverride(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    OUT PBOOLEAN Enabled
    );

BOOLEAN
NTAPI
ClasspIsMediaChangeDisabledForClass(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PUNICODE_STRING RegistryPath
    );

VOID
NTAPI
ClasspSetMediaChangeStateEx(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN MEDIA_CHANGE_DETECTION_STATE NewState,
    IN BOOLEAN Wait,
    IN BOOLEAN KnownStateChange // can ignore oldstate == unknown
    );

NTSTATUS
NTAPI
ClasspMediaChangeRegistryCallBack(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

VOID
NTAPI
ClasspSendMediaStateIrp(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PMEDIA_CHANGE_DETECTION_INFO Info,
    IN ULONG CountDown
    );

IO_WORKITEM_ROUTINE ClasspFailurePredict;

NTSTATUS
NTAPI
ClasspInitializePolling(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN BOOLEAN AllowDriveToSleep
    );


#if ALLOC_PRAGMA

#pragma alloc_text(PAGE, ClassInitializeMediaChangeDetection)
#pragma alloc_text(PAGE, ClassEnableMediaChangeDetection)
#pragma alloc_text(PAGE, ClassDisableMediaChangeDetection)
#pragma alloc_text(PAGE, ClassCleanupMediaChangeDetection)
#pragma alloc_text(PAGE, ClasspMediaChangeRegistryCallBack)
#pragma alloc_text(PAGE, ClasspInitializePolling)

#pragma alloc_text(PAGE, ClasspIsMediaChangeDisabledDueToHardwareLimitation)
#pragma alloc_text(PAGE, ClasspMediaChangeDeviceInstanceOverride)
#pragma alloc_text(PAGE, ClasspIsMediaChangeDisabledForClass)

#pragma alloc_text(PAGE, ClassSetFailurePredictionPoll)
#pragma alloc_text(PAGE, ClasspDisableTimer)
#pragma alloc_text(PAGE, ClasspEnableTimer)

#endif

// ISSUE -- make this public?
VOID
NTAPI
ClassSendEjectionNotification(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    //
    // For post-NT5.1 work, need to move EjectSynchronizationEvent
    // to be a MUTEX so we can attempt to grab it here and benefit
    // from deadlock detection.  This will allow checking if the media
    // has been locked by programs before broadcasting these events.
    // (what's the point of broadcasting if the media is not locked?)
    //
    // This would currently only be a slight optimization.  For post-NT5.1,
    // it would allow us to send a single PERSISTENT_PREVENT to MMC devices,
    // thereby cleaning up a lot of the ejection code.  Then, when the
    // ejection request occured, we could see if any locks for the media
    // existed.  if locked, broadcast.  if not, we send the eject irp.
    //
    
    //
    // for now, just always broadcast.  make this a public routine,
    // so class drivers can add special hacks to broadcast this for their
    // non-MMC-compliant devices also from sense codes.
    //

    DBGTRACE(ClassDebugTrace, ("ClassSendEjectionNotification: media EJECT_REQUEST"));
    ClasspSendNotification(FdoExtension,
                           &GUID_IO_MEDIA_EJECT_REQUEST,
                           0,
                           NULL);
    return;
}

VOID
NTAPI
ClasspSendNotification(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN const GUID * Guid,
    IN ULONG  ExtraDataSize,
    IN PVOID  ExtraData
    )
{
    PTARGET_DEVICE_CUSTOM_NOTIFICATION notification;
    ULONG requiredSize;
        
    requiredSize =
        (sizeof(TARGET_DEVICE_CUSTOM_NOTIFICATION) - sizeof(UCHAR)) +
        ExtraDataSize;

    if (requiredSize > 0x0000ffff) {
        // MAX_USHORT, max total size for these events!
        KdPrintEx((DPFLTR_CLASSPNP_ID, ClassDebugWarning,
                   "Error sending event: size too large! (%x)\n",
                   requiredSize));
        return;
    }
    
    notification = ExAllocatePoolWithTag(NonPagedPool,
                                         requiredSize,
                                         'oNcS');
    
    //
    // if none allocated, exit
    //

    if (notification == NULL) {
        return;
    }

    //
    // Prepare and send the request!
    //
    
    RtlZeroMemory(notification, requiredSize);
    notification->Version = 1;
    notification->Size = (USHORT)(requiredSize);
    notification->FileObject = NULL;
    notification->NameBufferOffset = -1;
    notification->Event = *Guid;
    RtlCopyMemory(notification->CustomDataBuffer, ExtraData, ExtraDataSize);

    IoReportTargetDeviceChangeAsynchronous(FdoExtension->LowerPdo,
                                           notification,
                                           NULL, NULL);
    
    ExFreePool(notification);
    notification = NULL;
    return;
}

/*++////////////////////////////////////////////////////////////////////////////

ClasspInterpretGesnData()

Routine Description:

    This routine will interpret the data returned for a GESN command, and
    (if appropriate) set the media change event, and broadcast the
    appropriate events to user mode for applications who care.

Arguments:

    FdoExtension - the device
    
    DataBuffer - the resulting data from a GESN event.
        requires at least EIGHT valid bytes (header == 4, data == 4)

    ResendImmediately - whether or not to immediately resend the request.
        this should be FALSE if there was no event, FALSE if the reported
        event was of the DEVICE BUSY class, else true.

Return Value:
    
    None
    
Notes:

    DataBuffer must be at least four bytes of valid data (header == 4 bytes),
    and have at least eight bytes of allocated memory (all events == 4 bytes).
    
    The call to StartNextPacket may occur before this routine is completed.
    the operational change notifications are informational in nature, and
    while useful, are not neccessary to ensure proper operation.  For example,
    if the device morphs to no longer supporting WRITE commands, all further
    write commands will fail.  There exists a small timing window wherein
    IOCTL_IS_DISK_WRITABLE may be called and get an incorrect response.  If
    a device supports software write protect, it is expected that the
    application can handle such a case.
    
    NOTE: perhaps setting the updaterequired byte to one should be done here.
    if so, it relies upon the setting of a 32-byte value to be an atomic
    operation.  unfortunately, there is no simple way to notify a class driver
    which wants to know that the device behavior requires updating.
    
    Not ready events may be sent every second.  For example, if we were
    to minimize the number of asynchronous notifications, an application may
    register just after a large busy time was reported.  This would then
    prevent the application from knowing the device was busy until some
    arbitrarily chosen timeout has occurred.  Also, the GESN request would
    have to still occur, since it checks for non-busy events (such as user
    keybutton presses and media change events) as well.  The specification
    states that the lower-numered events get reported first, so busy events,
    while repeating, will only be reported when all other events have been
    cleared from the device.

--*/
VOID
NTAPI
ClasspInterpretGesnData(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PNOTIFICATION_EVENT_STATUS_HEADER Header,
    IN PBOOLEAN ResendImmediately
    )
{
    PMEDIA_CHANGE_DETECTION_INFO info;
    LONG dataLength;
    LONG requiredLength;

    info = FdoExtension->MediaChangeDetectionInfo;

    //
    // note: don't allocate anything in this routine so that we can
    //       always just 'return'.
    //

    *ResendImmediately = FALSE;

    if (Header->NEA) {
        return;
    }
    if (Header->NotificationClass == NOTIFICATION_NO_CLASS_EVENTS) {
        return;
    }

    //
    // HACKHACK - REF #0001
    // This loop is only taken initially, due to the inability to reliably
    // auto-detect drives that report events correctly at boot.  When we
    // detect this behavior during the normal course of running, we will
    // disable the hack, allowing more efficient use of the system.  This
    // should occur "nearly" instantly, as the drive should have multiple
    // events queue'd (ie. power, morphing, media).
    //

    if (info->Gesn.HackEventMask) {

        //
        // all events use the low four bytes of zero to indicate
        // that there was no change in status.
        //

        UCHAR thisEvent = Header->ClassEventData[0] & 0xf;
        UCHAR lowestSetBit;
        UCHAR thisEventBit = (1 << Header->NotificationClass);

        ASSERT(TEST_FLAG(info->Gesn.EventMask, thisEventBit));


        //
        // some bit magic here... this results in the lowest set bit only
        //

        lowestSetBit = info->Gesn.EventMask;
        lowestSetBit &= (info->Gesn.EventMask - 1);
        lowestSetBit ^= (info->Gesn.EventMask);

        if (thisEventBit != lowestSetBit) {
            
            //
            // HACKHACK - REF #0001
            // the first time we ever see an event set that is not the lowest
            // set bit in the request (iow, highest priority), we know that the
            // hack is no longer required, as the device is ignoring "no change"
            // events when a real event is waiting in the other requested queues.
            //

            KdPrintEx((DPFLTR_CLASSPNP_ID, ClassDebugMCN,
                       "Classpnp => GESN::NONE: Compliant drive found, "
                       "removing GESN hack (%x, %x)\n",
                       thisEventBit, info->Gesn.EventMask));
            
            info->Gesn.HackEventMask = FALSE;

        } else if (thisEvent == 0) {
            
            //
            // HACKHACK - REF #0001
            // note: this hack prevents poorly implemented firmware from constantly
            //       returning "No Event".  we do this by cycling through the
            //       supported list of events here.
            //

            SET_FLAG(info->Gesn.NoChangeEventMask, thisEventBit);
            CLEAR_FLAG(info->Gesn.EventMask, thisEventBit);

            //
            // if we have cycled through all supported event types, then
            // we need to reset the events we are asking about. else we
            // want to resend this request immediately in case there was
            // another event pending.
            //

            if (info->Gesn.EventMask == 0) {
                info->Gesn.EventMask         = info->Gesn.NoChangeEventMask;
                info->Gesn.NoChangeEventMask = 0;
            } else {
                *ResendImmediately = TRUE;
            }
            return;
        }

    } // end if (info->Gesn.HackEventMask)

    dataLength =
        (Header->EventDataLength[0] << 8) |
        (Header->EventDataLength[1] & 0xff);
    dataLength -= 2;
    requiredLength = 4; // all events are four bytes

    if (dataLength < requiredLength) {
        KdPrintEx((DPFLTR_CLASSPNP_ID, ClassDebugWarning,
                   "Classpnp => GESN returned only %x bytes data for fdo %p\n",
                   dataLength, FdoExtension->DeviceObject));
        return;
    }
    if (dataLength != requiredLength) {
        KdPrintEx((DPFLTR_CLASSPNP_ID, ClassDebugWarning,
                   "Classpnp => GESN returned too many (%x) bytes data for fdo %p\n",
                   dataLength, FdoExtension->DeviceObject));
        dataLength = 4;
    }

/*
    ClasspSendNotification(FdoExtension,
                           &GUID_IO_GENERIC_GESN_EVENT,
                           sizeof(NOTIFICATION_EVENT_STATUS_HEADER) + dataLength,
                           Header)
*/

    switch (Header->NotificationClass) {

    case NOTIFICATION_EXTERNAL_REQUEST_CLASS_EVENTS: { // 0x3
        
        PNOTIFICATION_EXTERNAL_STATUS externalInfo = 
            (PNOTIFICATION_EXTERNAL_STATUS)(Header->ClassEventData);
        DEVICE_EVENT_EXTERNAL_REQUEST externalData;

        //
        // unfortunately, due to time constraints, we will only notify
        // about keys being pressed, and not released.  this makes keys
        // single-function, but simplifies the code significantly.
        //
        
        if (externalInfo->ExternalEvent !=
            NOTIFICATION_EXTERNAL_EVENT_BUTTON_DOWN) {
            break;
        }
        
        *ResendImmediately = TRUE;
        KdPrintEx((DPFLTR_CLASSPNP_ID, ClassDebugMCN,
                   "Classpnp => GESN::EXTERNAL: Event: %x Status %x Req %x\n",
                   externalInfo->ExternalEvent, externalInfo->ExternalStatus,
                   (externalInfo->Request[0] << 8) | externalInfo->Request[1]
                   ));

        RtlZeroMemory(&externalData, sizeof(DEVICE_EVENT_EXTERNAL_REQUEST));
        externalData.Version = 1;
        externalData.DeviceClass = 0;
        externalData.ButtonStatus = externalInfo->ExternalEvent;
        externalData.Request =
            (externalInfo->Request[0] << 8) |
            (externalInfo->Request[1] & 0xff);
        KeQuerySystemTime(&(externalData.SystemTime));
        externalData.SystemTime.QuadPart *= (LONGLONG)KeQueryTimeIncrement();

        DBGTRACE(ClassDebugTrace, ("ClasspInterpretGesnData: media DEVICE_EXTERNAL_REQUEST"));
        ClasspSendNotification(FdoExtension,
                               &GUID_IO_DEVICE_EXTERNAL_REQUEST,
                               sizeof(DEVICE_EVENT_EXTERNAL_REQUEST),
                               &externalData);
        return;
    }
    
    case NOTIFICATION_MEDIA_STATUS_CLASS_EVENTS: { // 0x4
        
        PNOTIFICATION_MEDIA_STATUS mediaInfo =
            (PNOTIFICATION_MEDIA_STATUS)(Header->ClassEventData);
        
        if (mediaInfo->MediaEvent == NOTIFICATION_MEDIA_EVENT_NO_CHANGE) {
            break;
        }
        
        *ResendImmediately = TRUE;
        KdPrintEx((DPFLTR_CLASSPNP_ID, ClassDebugMCN,
                   "Classpnp => GESN::MEDIA: Event: %x Status %x\n",
                   mediaInfo->MediaEvent, mediaInfo->MediaStatus));
        
        if ((mediaInfo->MediaEvent == NOTIFICATION_MEDIA_EVENT_NEW_MEDIA) ||
            (mediaInfo->MediaEvent == NOTIFICATION_MEDIA_EVENT_MEDIA_CHANGE)) {


            if (TEST_FLAG(FdoExtension->DeviceObject->Characteristics,
                          FILE_REMOVABLE_MEDIA) &&
                (ClassGetVpb(FdoExtension->DeviceObject) != NULL) &&
                (ClassGetVpb(FdoExtension->DeviceObject)->Flags & VPB_MOUNTED)
                ) {

                SET_FLAG(FdoExtension->DeviceObject->Flags, DO_VERIFY_VOLUME);

            }
            InterlockedIncrement((PLONG)&FdoExtension->MediaChangeCount);
            ClasspSetMediaChangeStateEx(FdoExtension,
                                        MediaPresent,
                                        FALSE,
                                        TRUE);

        } else if (mediaInfo->MediaEvent == NOTIFICATION_MEDIA_EVENT_MEDIA_REMOVAL) {
            
            ClasspSetMediaChangeStateEx(FdoExtension,
                                        MediaNotPresent,
                                        FALSE,
                                        TRUE);
        
        } else if (mediaInfo->MediaEvent == NOTIFICATION_MEDIA_EVENT_EJECT_REQUEST) {

            KdPrintEx((DPFLTR_CLASSPNP_ID, ClassDebugError,
                       "Classpnp => GESN Ejection request received!\n"));
            ClassSendEjectionNotification(FdoExtension);
        
        }
        break;

    }
    
    case NOTIFICATION_DEVICE_BUSY_CLASS_EVENTS: { // lowest priority events...
        
        PNOTIFICATION_BUSY_STATUS busyInfo =
            (PNOTIFICATION_BUSY_STATUS)(Header->ClassEventData);
        DEVICE_EVENT_BECOMING_READY busyData;
        
        //
        // NOTE: we never actually need to immediately retry for these
        //       events: if one exists, the device is busy, and if not,
        //       we still don't want to retry.
        //

        if (busyInfo->DeviceBusyStatus == NOTIFICATION_BUSY_STATUS_NO_EVENT) {
            break;
        }
        
        //
        // else we want to report the approximated time till it's ready.
        //

        RtlZeroMemory(&busyData, sizeof(DEVICE_EVENT_BECOMING_READY));
        busyData.Version = 1;
        busyData.Reason = busyInfo->DeviceBusyStatus;
        busyData.Estimated100msToReady = (busyInfo->Time[0] << 8) |
                                         (busyInfo->Time[1] & 0xff);
        
        KdPrintEx((DPFLTR_CLASSPNP_ID, ClassDebugMCN,
                   "Classpnp => GESN::BUSY: Event: %x Status %x Time %x\n",
                   busyInfo->DeviceBusyEvent, busyInfo->DeviceBusyStatus,
                   busyData.Estimated100msToReady
                   ));

        DBGTRACE(ClassDebugTrace, ("ClasspInterpretGesnData: media BECOMING_READY"));
        ClasspSendNotification(FdoExtension,
                               &GUID_IO_DEVICE_BECOMING_READY,
                               sizeof(DEVICE_EVENT_BECOMING_READY),
                               &busyData);
        break;
    }
    
    default: {
        
        break;

    }
    
    } // end switch on notification class
    return;
}

/*++////////////////////////////////////////////////////////////////////////////

ClasspInternalSetMediaChangeState()

Routine Description:

    This routine will (if appropriate) set the media change event for the
    device.  The event will be set if the media state is changed and
    media change events are enabled.  Otherwise the media state will be
    tracked but the event will not be set.

    This routine will lock out the other media change routines if possible
    but if not a media change notification may be lost after the enable has
    been completed.

Arguments:

    FdoExtension - the device

    MediaPresent - indicates whether the device has media inserted into it
                   (TRUE) or not (FALSE).
 
Return Value:

    none

--*/
VOID
NTAPI
ClasspInternalSetMediaChangeState(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN MEDIA_CHANGE_DETECTION_STATE NewState,
    IN BOOLEAN KnownStateChange // can ignore oldstate == unknown
    )
{
#if DBG
    PCSTR states[] = {"Unknown", "Present", "Not Present"};
#endif
    MEDIA_CHANGE_DETECTION_STATE oldMediaState;
    PMEDIA_CHANGE_DETECTION_INFO info = FdoExtension->MediaChangeDetectionInfo;
    ULONG data;
    //NTSTATUS status;

    ASSERT((NewState >= MediaUnknown) && (NewState <= MediaNotPresent));

    if(info == NULL) {
        return;
    }

    oldMediaState = InterlockedExchange(
        (PLONG)(&info->MediaChangeDetectionState),
        (LONG)NewState);

    if((oldMediaState == MediaUnknown) && (!KnownStateChange)) {

        //
        // The media was in an indeterminate state before - don't notify for
        // this change.
        //

        DebugPrint((ClassDebugMCN,
                    "ClassSetMediaChangeState: State was unknown - this may "
                    "not be a change\n"));
        return;

    } else if(oldMediaState == NewState) {

        //
        // Media is in the same state it was before.
        //

        return;
    }

    if(info->MediaChangeDetectionDisableCount != 0) {

        DBGTRACE(ClassDebugMCN,
                    ("ClassSetMediaChangeState: MCN not enabled, state "
                    "changed from %s to %s\n",
                    states[oldMediaState], states[NewState]));
        return;

    }

    DBGTRACE(ClassDebugMCN,
                ("ClassSetMediaChangeState: State change from %s to %s\n",
                states[oldMediaState], states[NewState]));

    //
    // make the data useful -- it used to always be zero.
    //
    data = FdoExtension->MediaChangeCount;

    if (NewState == MediaPresent) {

        DBGTRACE(ClassDebugTrace, ("ClasspInternalSetMediaChangeState: media ARRIVAL"));
        ClasspSendNotification(FdoExtension,
                               &GUID_IO_MEDIA_ARRIVAL,
                               sizeof(ULONG),
                               &data);

    }
    else if (NewState == MediaNotPresent) {

        DBGTRACE(ClassDebugTrace, ("ClasspInternalSetMediaChangeState: media REMOVAL"));
        ClasspSendNotification(FdoExtension,
                               &GUID_IO_MEDIA_REMOVAL,
                               sizeof(ULONG),
                               &data);

    } else {

        //
        // Don't notify of changed going to unknown.
        //

        return;
    }
    
    return;
} // end ClasspInternalSetMediaChangeState()

/*++////////////////////////////////////////////////////////////////////////////

ClassSetMediaChangeState()

Routine Description:

    This routine will (if appropriate) set the media change event for the
    device.  The event will be set if the media state is changed and
    media change events are enabled.  Otherwise the media state will be
    tracked but the event will not be set.

    This routine will lock out the other media change routines if possible
    but if not a media change notification may be lost after the enable has
    been completed.

Arguments:

    FdoExtension - the device

    MediaPresent - indicates whether the device has media inserted into it
                   (TRUE) or not (FALSE).

    Wait - indicates whether the function should wait until it can acquire
           the synchronization lock or not.

Return Value:

    none

--*/
VOID
NTAPI
ClasspSetMediaChangeStateEx(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN MEDIA_CHANGE_DETECTION_STATE NewState,
    IN BOOLEAN Wait,
    IN BOOLEAN KnownStateChange // can ignore oldstate == unknown
    )
{
    PMEDIA_CHANGE_DETECTION_INFO info = FdoExtension->MediaChangeDetectionInfo;
    LARGE_INTEGER zero;
    NTSTATUS status;

    DBGTRACE(ClassDebugMCN, ("> ClasspSetMediaChangeStateEx"));

    //
    // Reset SMART status on media removal as the old status may not be
    // valid when there is no media in the device or when new media is
    // inserted.
    //

    if (NewState == MediaNotPresent) {

        FdoExtension->FailurePredicted = FALSE;
        FdoExtension->FailureReason = 0;

    }


    zero.QuadPart = 0;

    if(info == NULL) {
        return;
    }

    status = KeWaitForMutexObject(&info->MediaChangeMutex,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  ((Wait == TRUE) ? NULL : &zero));

    if(status == STATUS_TIMEOUT) {

        //
        // Someone else is in the process of setting the media state
        //

        DBGWARN(("ClasspSetMediaChangeStateEx - timed out waiting for mutex"));
        return;
    }

    //
    // Change the media present state and signal an event, if applicable
    //

    ClasspInternalSetMediaChangeState(FdoExtension, NewState, KnownStateChange);

    KeReleaseMutex(&info->MediaChangeMutex, FALSE);

    DBGTRACE(ClassDebugMCN, ("< ClasspSetMediaChangeStateEx"));

    return;
} // end ClassSetMediaChangeStateEx()

VOID
NTAPI
ClassSetMediaChangeState(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN MEDIA_CHANGE_DETECTION_STATE NewState,
    IN BOOLEAN Wait
    )
{
    ClasspSetMediaChangeStateEx(FdoExtension, NewState, Wait, FALSE);
    return;
}

/*++////////////////////////////////////////////////////////////////////////////

ClasspMediaChangeDetectionCompletion()

Routine Description:

    This routine handles the completion of the test unit ready irps used to
    determine if the media has changed.  If the media has changed, this code
    signals the named event to wake up other system services that react to
    media change (aka AutoPlay).

Arguments:

    DeviceObject - the object for the completion
    Irp - the IRP being completed
    Context - the SRB from the IRP

Return Value:

    NTSTATUS

--*/
NTSTATUS
NTAPI
ClasspMediaChangeDetectionCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
{
    PSCSI_REQUEST_BLOCK srb = Context;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData;
    PMEDIA_CHANGE_DETECTION_INFO info;
    //PIO_STACK_LOCATION  nextIrpStack;
    NTSTATUS status;
    BOOLEAN retryImmediately = FALSE;

    //
    // Since the class driver created this request, it's completion routine
    // will not get a valid device object handed in.  Use the one in the
    // irp stack instead
    //

    DeviceObject = IoGetCurrentIrpStackLocation(Irp)->DeviceObject;
    fdoExtension = DeviceObject->DeviceExtension;
    fdoData = fdoExtension->PrivateFdoData;
    info         = fdoExtension->MediaChangeDetectionInfo;

    ASSERT(info->MediaChangeIrp != NULL);
    ASSERT(!TEST_FLAG(srb->SrbStatus, SRB_STATUS_QUEUE_FROZEN));
    DBGTRACE(ClassDebugMCN, ("> ClasspMediaChangeDetectionCompletion: Device %p completed MCN irp %p.", DeviceObject, Irp));

    /*
     *  HACK for IoMega 2GB Jaz drive:
     *  This drive spins down on its own to preserve the media.  
     *  When spun down, TUR fails with 2/4/0 (SCSI_SENSE_NOT_READY/SCSI_ADSENSE_LUN_NOT_READY/?).
     *  ClassInterpretSenseInfo would then call ClassSendStartUnit to spin the media up, which defeats the
     *  purpose of the spindown.
     *  So in this case, make this into a successful TUR.  
     *  This allows the drive to stay spun down until it is actually accessed again.
     *  (If the media were actually removed, TUR would fail with 2/3a/0 ).
     *  This hack only applies to drives with the CAUSE_NOT_REPORTABLE_HACK bit set; this
     *  is set by disk.sys when HackCauseNotReportableHack is set for the drive in its BadControllers list.
     */
    if ((SRB_STATUS(srb->SrbStatus) != SRB_STATUS_SUCCESS) &&
        TEST_FLAG(fdoExtension->ScanForSpecialFlags, CLASS_SPECIAL_CAUSE_NOT_REPORTABLE_HACK) &&
        (srb->SenseInfoBufferLength >= RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseCode))){
        
        PSENSE_DATA senseData = srb->SenseInfoBuffer;
        
        if ((senseData->SenseKey == SCSI_SENSE_NOT_READY) &&
            (senseData->AdditionalSenseCode == SCSI_ADSENSE_LUN_NOT_READY)){
            srb->SrbStatus = SRB_STATUS_SUCCESS;
        }
    }


    //
    // use ClassInterpretSenseInfo() to check for media state, and also
    // to call ClassError() with correct parameters.
    //
    status = STATUS_SUCCESS;
    if (SRB_STATUS(srb->SrbStatus) != SRB_STATUS_SUCCESS) {

        DBGTRACE(ClassDebugMCN, ("ClasspMediaChangeDetectionCompletion - failed - srb status=%s, sense=%s/%s/%s.", DBGGETSRBSTATUSSTR(srb), DBGGETSENSECODESTR(srb), DBGGETADSENSECODESTR(srb), DBGGETADSENSEQUALIFIERSTR(srb)));

        ClassInterpretSenseInfo(DeviceObject,
                                srb,
                                IRP_MJ_SCSI,
                                0,
                                0,
                                &status,
                                NULL);

    }
    else {
        
        fdoData->LoggedTURFailureSinceLastIO = FALSE;
        
        if (!info->Gesn.Supported) {

            DBGTRACE(ClassDebugMCN, ("ClasspMediaChangeDetectionCompletion - succeeded and GESN NOT supported, setting MediaPresent."));
        
            //
            // success != media for GESN case
            //

            ClassSetMediaChangeState(fdoExtension, MediaPresent, FALSE);

        }
        else {
            DBGTRACE(ClassDebugMCN, ("ClasspMediaChangeDetectionCompletion - succeeded (GESN supported)."));
        }
    }
    
    if (info->Gesn.Supported) {

        if (status == STATUS_DATA_OVERRUN) {
            DBGTRACE(ClassDebugMCN, ("ClasspMediaChangeDetectionCompletion - Overrun"));
            status = STATUS_SUCCESS;
        }

        if (!NT_SUCCESS(status)) {
            DBGTRACE(ClassDebugMCN, ("ClasspMediaChangeDetectionCompletion: GESN failed with status %x", status));
        } else {

            //
            // for GESN, need to interpret the results of the data.
            // this may also require an immediate retry
            //
            
            if (Irp->IoStatus.Information == 8 ) {
                ClasspInterpretGesnData(fdoExtension,
                                        (PVOID)info->Gesn.Buffer,
                                        &retryImmediately);
            }

        } // end of NT_SUCCESS(status)

    } // end of Info->Gesn.Supported

    //
    // free port-allocated sense buffer, if any.
    //

    if (PORT_ALLOCATED_SENSE(fdoExtension, srb)) {
        FREE_PORT_ALLOCATED_SENSE_BUFFER(fdoExtension, srb);
    }

    //
    // Remember the IRP and SRB for use the next time.
    //

    ASSERT(IoGetNextIrpStackLocation(Irp));
    IoGetNextIrpStackLocation(Irp)->Parameters.Scsi.Srb = srb;

    //
    // Reset the MCN timer.
    //

    ClassResetMediaChangeTimer(fdoExtension);

    //
    // run a sanity check to make sure we're not recursing continuously
    //

    if (retryImmediately) {
        
        info->MediaChangeRetryCount++;
        if (info->MediaChangeRetryCount > MAXIMUM_IMMEDIATE_MCN_RETRIES) {
            ASSERT(!"Recursing too often in MCN?");
            info->MediaChangeRetryCount = 0;
            retryImmediately = FALSE;
        }

    } else {
        
        info->MediaChangeRetryCount = 0;

    }


    //
    // release the remove lock....
    //

    {
        UCHAR uniqueValue;
        ClassAcquireRemoveLock(DeviceObject, (PIRP)(&uniqueValue));
        ClassReleaseRemoveLock(DeviceObject, Irp);

        
        //
        // set the irp as not in use
        //
        {
            volatile LONG irpWasInUse;
            irpWasInUse = InterlockedCompareExchange(&info->MediaChangeIrpInUse, 0, 1);
            #if _MSC_FULL_VER != 13009111        // This compiler always takes the wrong path here.
                ASSERT(irpWasInUse);
            #endif
        }

        //
        // now send it again before we release our last remove lock
        //

        if (retryImmediately) {
            ClasspSendMediaStateIrp(fdoExtension, info, 0);
        }
        else {
            DBGTRACE(ClassDebugMCN, ("ClasspMediaChangeDetectionCompletion - not retrying immediately"));
        }
        
        //
        // release the temporary remove lock
        //
        
        ClassReleaseRemoveLock(DeviceObject, (PIRP)(&uniqueValue));
    }

    DBGTRACE(ClassDebugMCN, ("< ClasspMediaChangeDetectionCompletion"));

    return STATUS_MORE_PROCESSING_REQUIRED;
} 

/*++////////////////////////////////////////////////////////////////////////////

ClasspSendTestUnitIrp() - ISSUE-2000/02/20-henrygab - not documented

Routine Description:

    This routine

Arguments:

    DeviceObject - 
    Irp - 

Return Value:


--*/
PIRP
NTAPI
ClasspPrepareMcnIrp(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PMEDIA_CHANGE_DETECTION_INFO Info,
    IN BOOLEAN UseGesn
)
{
    PSCSI_REQUEST_BLOCK srb;
    PIO_STACK_LOCATION irpStack;
    PIO_STACK_LOCATION nextIrpStack;
    NTSTATUS status;
    PCDB cdb;
    PIRP irp;
    PVOID buffer;

    //
    // Setup the IRP to perform a test unit ready.
    //

    irp = Info->MediaChangeIrp;

    ASSERT(irp);

    if (irp == NULL) {
        return NULL;
    }

    //
    // don't keep sending this if the device is being removed.
    //

    status = ClassAcquireRemoveLock(FdoExtension->DeviceObject, irp);
    if (status == REMOVE_COMPLETE) {
        ASSERT(status != REMOVE_COMPLETE);
        return NULL;
    }
    else if (status == REMOVE_PENDING) {
        ClassReleaseRemoveLock(FdoExtension->DeviceObject, irp);
        return NULL;
    }
    else {
        ASSERT(status == NO_REMOVE);
    }

    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;
    irp->Flags = 0;
    irp->UserBuffer = NULL;

    //
    // If the irp is sent down when the volume needs to be
    // verified, CdRomUpdateGeometryCompletion won't complete
    // it since it's not associated with a thread.  Marking
    // it to override the verify causes it always be sent
    // to the port driver
    //

    irpStack = IoGetCurrentIrpStackLocation(irp);
    irpStack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;

    nextIrpStack = IoGetNextIrpStackLocation(irp);
    nextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    nextIrpStack->Parameters.Scsi.Srb = &(Info->MediaChangeSrb);

    //
    // Prepare the SRB for execution.
    //

    srb = nextIrpStack->Parameters.Scsi.Srb;
    buffer = srb->SenseInfoBuffer;
    RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));
    RtlZeroMemory(buffer, SENSE_BUFFER_SIZE);


    srb->QueueTag = SP_UNTAGGED;
    srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;
    srb->Length = sizeof(SCSI_REQUEST_BLOCK);
    srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    srb->SenseInfoBuffer = buffer;
    srb->SrbStatus = 0;
    srb->ScsiStatus = 0;
    srb->OriginalRequest = irp;
    srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;
    
    srb->SrbFlags = FdoExtension->SrbFlags;
    SET_FLAG(srb->SrbFlags, Info->SrbFlags);

    srb->TimeOutValue = FdoExtension->TimeOutValue * 2;
    
    if (srb->TimeOutValue == 0) {

        if (FdoExtension->TimeOutValue == 0) {

            KdPrintEx((DPFLTR_CLASSPNP_ID, DPFLTR_ERROR_LEVEL,
                       "ClassSendTestUnitIrp: FdoExtension->TimeOutValue "
                       "is set to zero?! -- resetting to 10\n"));
            srb->TimeOutValue = 10 * 2;  // reasonable default
        
        } else {
            
            KdPrintEx((DPFLTR_CLASSPNP_ID, DPFLTR_ERROR_LEVEL,
                       "ClassSendTestUnitIrp: Someone set "
                       "srb->TimeOutValue to zero?! -- resetting to %x\n",
                       FdoExtension->TimeOutValue * 2));
            srb->TimeOutValue = FdoExtension->TimeOutValue * 2;

        }

    }
    
    if (!UseGesn) {
        
        srb->CdbLength = 6;
        srb->DataTransferLength = 0;
        SET_FLAG(srb->SrbFlags, SRB_FLAGS_NO_DATA_TRANSFER);
        nextIrpStack->Parameters.DeviceIoControl.IoControlCode =
                IOCTL_SCSI_EXECUTE_NONE;
        srb->DataBuffer = NULL;
        srb->DataTransferLength = 0;
        irp->MdlAddress = NULL;
        
        cdb = (PCDB) &srb->Cdb[0];
        cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;

    } else {
        
        ASSERT(Info->Gesn.Buffer);

        srb->TimeOutValue = GESN_TIMEOUT_VALUE; // much shorter timeout for GESN
        
        srb->CdbLength = 10;
        SET_FLAG(srb->SrbFlags, SRB_FLAGS_DATA_IN);
        nextIrpStack->Parameters.DeviceIoControl.IoControlCode =
                IOCTL_SCSI_EXECUTE_IN;
        srb->DataBuffer = Info->Gesn.Buffer;
        srb->DataTransferLength = Info->Gesn.BufferSize;
        irp->MdlAddress = Info->Gesn.Mdl;

        cdb = (PCDB) &srb->Cdb[0];
        cdb->GET_EVENT_STATUS_NOTIFICATION.OperationCode =
            SCSIOP_GET_EVENT_STATUS;
        cdb->GET_EVENT_STATUS_NOTIFICATION.Immediate = 1;
        cdb->GET_EVENT_STATUS_NOTIFICATION.EventListLength[0] =
            (UCHAR)((Info->Gesn.BufferSize) >> 8);
        cdb->GET_EVENT_STATUS_NOTIFICATION.EventListLength[1] =
            (UCHAR)((Info->Gesn.BufferSize) & 0xff);
        cdb->GET_EVENT_STATUS_NOTIFICATION.NotificationClassRequest =
            Info->Gesn.EventMask;

    }

    IoSetCompletionRoutine(irp,
                           ClasspMediaChangeDetectionCompletion,
                           srb,
                           TRUE,
                           TRUE,
                           TRUE);

    return irp;

}

/*++////////////////////////////////////////////////////////////////////////////

ClasspSendMediaStateIrp() - ISSUE-2000/02/20-henrygab - not documented

Routine Description:

    This routine

Arguments:

    DeviceObject - 
    Irp - 

Return Value:

--*/
VOID
NTAPI
ClasspSendMediaStateIrp(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PMEDIA_CHANGE_DETECTION_INFO Info,
    IN ULONG CountDown
    )
{
    BOOLEAN requestPending = FALSE;
    LONG irpInUse;
    //LARGE_INTEGER zero;
    //NTSTATUS status;

    DBGTRACE(ClassDebugMCN, ("> ClasspSendMediaStateIrp"));

    if (((FdoExtension->CommonExtension.CurrentState != IRP_MN_START_DEVICE) ||
         (FdoExtension->DevicePowerState != PowerDeviceD0)
         ) &&
        (!Info->MediaChangeIrpLost)) {

        //
        // the device may be stopped, powered down, or otherwise queueing io,
        // so should not timeout the autorun irp (yet) -- set to zero ticks.
        // scattered code relies upon this to not prematurely "lose" an
        // autoplay irp that was queued.
        //

        Info->MediaChangeIrpTimeInUse = 0;
    }

    //
    // if the irp is not in use, mark it as such.
    //

    irpInUse = InterlockedCompareExchange(&Info->MediaChangeIrpInUse, 1, 0);

    if (irpInUse) {

        LONG timeInUse;

        timeInUse = InterlockedIncrement(&Info->MediaChangeIrpTimeInUse);

        DebugPrint((ClassDebugMCN, "ClasspSendMediaStateIrp: irp in use for "
                    "%x seconds when synchronizing for MCD\n", timeInUse));

        if (Info->MediaChangeIrpLost == FALSE) {

            if (timeInUse > MEDIA_CHANGE_TIMEOUT_TIME) {

                //
                // currently set to five minutes.  hard to imagine a drive
                // taking that long to spin up.
                //

                DebugPrint((ClassDebugError,
                            "CdRom%d: Media Change Notification has lost "
                            "it's irp and doesn't know where to find it.  "
                            "Leave it alone and it'll come home dragging "
                            "it's stack behind it.\n",
                            FdoExtension->DeviceNumber));
                Info->MediaChangeIrpLost = TRUE;
            }
        }

        DBGTRACE(ClassDebugMCN, ("< ClasspSendMediaStateIrp - irpInUse"));
        return;

    }

    TRY {

        if (Info->MediaChangeDetectionDisableCount != 0) {
            DebugPrint((ClassDebugTrace, "ClassCheckMediaState: device %p has "
                        " detection disabled \n", FdoExtension->DeviceObject));
            LEAVE;
        }

        if (FdoExtension->DevicePowerState != PowerDeviceD0) {

            if (TEST_FLAG(Info->SrbFlags, SRB_FLAGS_NO_KEEP_AWAKE)) {
                DebugPrint((ClassDebugMCN,
                            "ClassCheckMediaState: device %p is powered "
                            "down and flags are set to let it sleep\n",
                            FdoExtension->DeviceObject));
                ClassResetMediaChangeTimer(FdoExtension);
                LEAVE;
            }

            //
            // NOTE: we don't increment the time in use until our power state
            // changes above.  this way, we won't "lose" the autoplay irp.
            // it's up to the lower driver to determine if powering up is a
            // good idea.
            //

            DebugPrint((ClassDebugMCN,
                        "ClassCheckMediaState: device %p needs to powerup "
                        "to handle this io (may take a few extra seconds).\n",
                        FdoExtension->DeviceObject));

        }

        Info->MediaChangeIrpTimeInUse = 0;
        Info->MediaChangeIrpLost = FALSE;

        if (CountDown == 0) {

            PIRP irp;

            DebugPrint((ClassDebugTrace,
                        "ClassCheckMediaState: timer expired\n"));

            if (Info->MediaChangeDetectionDisableCount != 0) {
                DebugPrint((ClassDebugTrace,
                            "ClassCheckMediaState: detection disabled\n"));
                LEAVE;
            }

            //
            // Prepare the IRP for the test unit ready
            //

            irp = ClasspPrepareMcnIrp(FdoExtension,
                                      Info,
                                      Info->Gesn.Supported);

            //
            // Issue the request.
            //

            DebugPrint((ClassDebugTrace,
                        "ClasspSendMediaStateIrp: Device %p getting TUR "
                        " irp %p\n", FdoExtension->DeviceObject, irp));

            if (irp == NULL) {
                LEAVE;
            }


            //
            // note: if we send it to the class dispatch routines, there is
            //       a timing window here (since they grab the remove lock)
            //       where we'd be removed. ELIMINATE the window by grabbing
            //       the lock ourselves above and sending it to the lower
            //       device object directly or to the device's StartIo
            //       routine (which doesn't acquire the lock).
            //
            
            requestPending = TRUE;

            DBGTRACE(ClassDebugMCN, ("  ClasspSendMediaStateIrp - calling IoCallDriver."));
            IoCallDriver(FdoExtension->CommonExtension.LowerDeviceObject, irp);
        }

    } FINALLY {

        if(requestPending == FALSE) {
            irpInUse = InterlockedCompareExchange(&Info->MediaChangeIrpInUse, 0, 1);
            #if _MSC_FULL_VER != 13009111        // This compiler always takes the wrong path here.
                ASSERT(irpInUse);
            #endif
        }

    }

    DBGTRACE(ClassDebugMCN, ("< ClasspSendMediaStateIrp"));
    
    return;
} // end ClasspSendMediaStateIrp()

/*++////////////////////////////////////////////////////////////////////////////

ClassCheckMediaState()

Routine Description:

    This routine is called by the class driver to test for a media change
    condition and/or poll for disk failure prediction.  It should be called
    from the class driver's IO timer routine once per second.

Arguments:

    FdoExtension - the device extension

Return Value:

    none

--*/
VOID
NTAPI
ClassCheckMediaState(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    PMEDIA_CHANGE_DETECTION_INFO info = FdoExtension->MediaChangeDetectionInfo;
    LONG countDown;

    if(info == NULL) {
        DebugPrint((ClassDebugTrace,
                    "ClassCheckMediaState: detection not enabled\n"));
        return;
    }

    //
    // Media change support is active and the IRP is waiting. Decrement the
    // timer.  There is no MP protection on the timer counter.  This code
    // is the only code that will manipulate the timer counter and only one
    // instance of it should be running at any given time.
    //

    countDown = InterlockedDecrement(&(info->MediaChangeCountDown));

    //
    // Try to acquire the media change event.  If we can't do it immediately
    // then bail out and assume the caller will try again later.
    //
    ClasspSendMediaStateIrp(FdoExtension,
                            info,
                            countDown);

    return;
} // end ClassCheckMediaState()

/*++////////////////////////////////////////////////////////////////////////////

ClassResetMediaChangeTimer()

Routine Description:

    Resets the media change count down timer to the default number of seconds.
    
Arguments:

    FdoExtension - the device to reset the timer for

Return Value:

    None

--*/
VOID
NTAPI
ClassResetMediaChangeTimer(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    PMEDIA_CHANGE_DETECTION_INFO info = FdoExtension->MediaChangeDetectionInfo;

    if(info != NULL) {
        InterlockedExchange(&(info->MediaChangeCountDown),
                            MEDIA_CHANGE_DEFAULT_TIME);
    }
    return;
} // end ClassResetMediaChangeTimer()

/*++////////////////////////////////////////////////////////////////////////////

ClasspInitializePolling() - ISSUE-2000/02/20-henrygab - not documented

Routine Description:

    This routine

Arguments:

    DeviceObject - 
    Irp - 

Return Value:

--*/
NTSTATUS
NTAPI
ClasspInitializePolling(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN BOOLEAN AllowDriveToSleep
    )
{
    PDEVICE_OBJECT fdo = FdoExtension->DeviceObject;
    //PCLASS_PRIVATE_FDO_DATA fdoData = FdoExtension->PrivateFdoData;

    //ULONG size;
    PMEDIA_CHANGE_DETECTION_INFO info;
    PIRP irp;

    PAGED_CODE();

    if (FdoExtension->MediaChangeDetectionInfo != NULL) {
        return STATUS_SUCCESS;
    }

    info = ExAllocatePoolWithTag(NonPagedPool,
                                 sizeof(MEDIA_CHANGE_DETECTION_INFO),
                                 CLASS_TAG_MEDIA_CHANGE_DETECTION);

    if(info != NULL) {
        RtlZeroMemory(info, sizeof(MEDIA_CHANGE_DETECTION_INFO));

        FdoExtension->KernelModeMcnContext.FileObject      = (PVOID)-1;
        FdoExtension->KernelModeMcnContext.DeviceObject    = (PVOID)-1;
        FdoExtension->KernelModeMcnContext.LockCount       = 0;
        FdoExtension->KernelModeMcnContext.McnDisableCount = 0;

        /*
         *  Allocate an IRP to carry the Test-Unit-Ready.
         *  Allocate an extra IRP stack location 
         *  so we can cache our device object in the top location.
         */
        irp = IoAllocateIrp((CCHAR)(fdo->StackSize+1), FALSE);
        
        if (irp != NULL) {

            PVOID buffer;

            buffer = ExAllocatePoolWithTag(
                        NonPagedPoolCacheAligned,
                        SENSE_BUFFER_SIZE,
                        CLASS_TAG_MEDIA_CHANGE_DETECTION);

            if (buffer != NULL) {
                PIO_STACK_LOCATION irpStack;
                PSCSI_REQUEST_BLOCK srb;
                //PCDB cdb;

                srb = &(info->MediaChangeSrb);
                info->MediaChangeIrp = irp;
                info->SenseBuffer = buffer;

                /*
                 *  For the driver that creates an IRP, there is no 'current' stack location.
                 *  Step down one IRP stack location so that the extra top one
                 *  becomes our 'current' one.
                 */
                IoSetNextIrpStackLocation(irp);

                /*
                 *  Cache our device object in the extra top IRP stack location
                 *  so we have it in our completion routine.
                 */
                irpStack = IoGetCurrentIrpStackLocation(irp);
                irpStack->DeviceObject = fdo;

                /*
                 *  Now start setting up the next IRP stack location for the call like any driver would.
                 */
                irpStack = IoGetNextIrpStackLocation(irp);
                irpStack->Parameters.Scsi.Srb = srb;
                info->MediaChangeIrp = irp;

                //
                // Initialize the SRB
                //

                RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

                //
                // Initialize and set up the sense information buffer
                //

                RtlZeroMemory(buffer, SENSE_BUFFER_SIZE);
                srb->SenseInfoBuffer = buffer;
                srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;

                //
                // Set default values for the media change notification
                // configuration.
                //

                info->MediaChangeCountDown = MEDIA_CHANGE_DEFAULT_TIME;
                info->MediaChangeDetectionDisableCount = 0;

                //
                // Assume that there is initially no media in the device
                // only notify upper layers if there is something there
                //

                info->MediaChangeDetectionState = MediaUnknown;

                info->MediaChangeIrpTimeInUse = 0;
                info->MediaChangeIrpLost = FALSE;

                //
                // setup all extra flags we'll be setting for this irp
                //
                info->SrbFlags = 0;
                if (AllowDriveToSleep) {
                    SET_FLAG(info->SrbFlags, SRB_FLAGS_NO_KEEP_AWAKE);
                }
                SET_FLAG(info->SrbFlags, SRB_CLASS_FLAGS_LOW_PRIORITY);
                SET_FLAG(info->SrbFlags, SRB_FLAGS_NO_QUEUE_FREEZE);
                SET_FLAG(info->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);

                KeInitializeMutex(&info->MediaChangeMutex, 0x100);

                //
                // It is ok to support media change events on this
                // device.
                //

                FdoExtension->MediaChangeDetectionInfo = info;

                //
                // NOTE: the DeviceType is FILE_DEVICE_CD_ROM even
                //       when the device supports DVD (no need to
                //       check for FILE_DEVICE_DVD, as it's not a
                //       valid check).
                //

                if (FdoExtension->DeviceObject->DeviceType == FILE_DEVICE_CD_ROM){

                    NTSTATUS status;

                    KdPrintEx((DPFLTR_CLASSPNP_ID, ClassDebugMCN,
                               "ClasspInitializePolling: Testing for GESN\n"));
                    status = ClasspInitializeGesn(FdoExtension, info);
                    if (NT_SUCCESS(status)) {
                        KdPrintEx((DPFLTR_CLASSPNP_ID, ClassDebugMCN,
                                   "ClasspInitializePolling: GESN available "
                                   "for %p\n", FdoExtension->DeviceObject));
                        ASSERT(info->Gesn.Supported );
                        ASSERT(info->Gesn.Buffer     != NULL);
                        ASSERT(info->Gesn.BufferSize != 0);
                        ASSERT(info->Gesn.EventMask  != 0);
                        // must return here, for ASSERTs to be valid.
                        return STATUS_SUCCESS;
                    }
                    KdPrintEx((DPFLTR_CLASSPNP_ID, ClassDebugMCN,
                               "ClasspInitializePolling: GESN *NOT* available "
                               "for %p\n", FdoExtension->DeviceObject));
                }
                
                ASSERT(info->Gesn.Supported == 0);
                ASSERT(info->Gesn.Buffer == NULL);
                ASSERT(info->Gesn.BufferSize == 0);
                ASSERT(info->Gesn.EventMask  == 0);
                info->Gesn.Supported = 0; // just in case....
                return STATUS_SUCCESS;
            }

            IoFreeIrp(irp);
        }

        ExFreePool(info);
    }

    //
    // nothing to free here
    //
    return STATUS_INSUFFICIENT_RESOURCES;

} // end ClasspInitializePolling()

NTSTATUS
NTAPI
ClasspInitializeGesn(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PMEDIA_CHANGE_DETECTION_INFO Info
    )
{    
    PNOTIFICATION_EVENT_STATUS_HEADER header;
    CLASS_DETECTION_STATE detectionState = ClassDetectionUnknown;
    PSTORAGE_ADAPTER_DESCRIPTOR adapterDescriptor;
    NTSTATUS status = STATUS_NOT_SUPPORTED;
    PIRP irp;
    KEVENT event;
    BOOLEAN retryImmediately;
    ULONG i;
    ULONG atapiResets;

    
    PAGED_CODE();
    ASSERT(Info == FdoExtension->MediaChangeDetectionInfo);

    //
    // read if we already know the abilities of the device
    //

    ClassGetDeviceParameter(FdoExtension,
                            CLASSP_REG_SUBKEY_NAME,
                            CLASSP_REG_MMC_DETECTION_VALUE_NAME,
                            (PULONG)&detectionState);
    
    if (detectionState == ClassDetectionUnsupported) {
        goto ExitWithError;
    }

    //
    // check if the device has a hack flag saying never to try this.
    //

    if (TEST_FLAG(FdoExtension->PrivateFdoData->HackFlags,
                  FDO_HACK_GESN_IS_BAD)) {
        
        detectionState = ClassDetectionUnsupported;
        ClassSetDeviceParameter(FdoExtension,
                                CLASSP_REG_SUBKEY_NAME,
                                CLASSP_REG_MMC_DETECTION_VALUE_NAME,
                                ClassDetectionSupported);
        goto ExitWithError;

    }


    //
    // else go through the process since we allocate buffers and 
    // get all sorts of device settings.
    //

    if (Info->Gesn.Buffer == NULL) {
        Info->Gesn.Buffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                                  GESN_BUFFER_SIZE,
                                                  '??cS');
    }
    if (Info->Gesn.Buffer == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ExitWithError;
    }
    if (Info->Gesn.Mdl != NULL) {
        IoFreeMdl(Info->Gesn.Mdl);
    }
    Info->Gesn.Mdl = IoAllocateMdl(Info->Gesn.Buffer,
                                   GESN_BUFFER_SIZE,
                                   FALSE, FALSE, NULL);
    if (Info->Gesn.Mdl == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ExitWithError;
    }

    MmBuildMdlForNonPagedPool(Info->Gesn.Mdl);
    Info->Gesn.BufferSize = GESN_BUFFER_SIZE;
    Info->Gesn.EventMask = 0;

    //
    // all items are prepared to use GESN (except the event mask, so don't
    // optimize this part out!).
    //
    // now see if it really works. we have to loop through this because
    // many SAMSUNG (and one COMPAQ) drives timeout when requesting
    // NOT_READY events, even when the IMMEDIATE bit is set. :(
    //
    // using a drive list is cumbersome, so this might fix the problem.
    //

    adapterDescriptor = FdoExtension->AdapterDescriptor;
    atapiResets = 0;
    retryImmediately = TRUE;
    for (i = 0; i < 16 && retryImmediately == TRUE; i++) {
    
        irp = ClasspPrepareMcnIrp(FdoExtension, Info, TRUE);
        if (irp == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto ExitWithError;
        }

        ASSERT(TEST_FLAG(Info->MediaChangeSrb.SrbFlags, SRB_FLAGS_NO_QUEUE_FREEZE));
    
        //
        // replace the completion routine with a different one this time...
        //
    
        IoSetCompletionRoutine(irp,
                               ClassSignalCompletion,
                               &event,
                               TRUE, TRUE, TRUE);
        KeInitializeEvent(&event, SynchronizationEvent, FALSE);
    
        status = IoCallDriver(FdoExtension->CommonExtension.LowerDeviceObject, irp);
    
        if (status == STATUS_PENDING) {
            status = KeWaitForSingleObject(&event,
                                           Executive,
                                           KernelMode,
                                           FALSE,
                                           NULL);
            ASSERT(NT_SUCCESS(status));
        }
        ClassReleaseRemoveLock(FdoExtension->DeviceObject, irp);
    
        if (SRB_STATUS(Info->MediaChangeSrb.SrbStatus) != SRB_STATUS_SUCCESS) {
            ClassInterpretSenseInfo(FdoExtension->DeviceObject,
                                    &(Info->MediaChangeSrb),
                                    IRP_MJ_SCSI,
                                    0,
                                    0,
                                    &status,
                                    NULL);
        }

        if ((adapterDescriptor->BusType == BusTypeAtapi) &&
            (Info->MediaChangeSrb.SrbStatus == SRB_STATUS_BUS_RESET)
            ) {

            //
            // ATAPI unfortunately returns SRB_STATUS_BUS_RESET instead
            // of SRB_STATUS_TIMEOUT, so we cannot differentiate between
            // the two.  if we get this status four time consecutively,
            // stop trying this command.  it is too late to change ATAPI
            // at this point, so special-case this here. (07/10/2001)
            // NOTE: any value more than 4 may cause the device to be
            //       marked missing.
            //

            atapiResets++;
            if (atapiResets >= 4) {
                status = STATUS_IO_DEVICE_ERROR;
                goto ExitWithError;
            }
        }

        if (status == STATUS_DATA_OVERRUN) {
            status = STATUS_SUCCESS;
        }
    
        if ((status == STATUS_INVALID_DEVICE_REQUEST) ||
            (status == STATUS_TIMEOUT) ||
            (status == STATUS_IO_DEVICE_ERROR) ||
            (status == STATUS_IO_TIMEOUT)
            ) {
    
            //
            // with these error codes, we don't ever want to try this command
            // again on this device, since it reacts poorly.
            //
    
            ClassSetDeviceParameter(FdoExtension,
                                    CLASSP_REG_SUBKEY_NAME,
                                    CLASSP_REG_MMC_DETECTION_VALUE_NAME,
                                    ClassDetectionUnsupported);
            KdPrintEx((DPFLTR_CLASSPNP_ID, ClassDebugWarning,
                       "Classpnp => GESN test failed %x for fdo %p\n",
                       status, FdoExtension->DeviceObject));
            goto ExitWithError;

    
        }
    
        if (!NT_SUCCESS(status)) {

            //
            // this may be other errors that should not disable GESN
            // for all future start_device calls.
            //

            KdPrintEx((DPFLTR_CLASSPNP_ID, ClassDebugWarning,
                       "Classpnp => GESN test failed %x for fdo %p\n",
                       status, FdoExtension->DeviceObject));
            goto ExitWithError;
        }
    
        if (i == 0) {

            //
            // the first time, the request was just retrieving a mask of
            // available bits.  use this to mask future requests.
            //
        
            header = (PNOTIFICATION_EVENT_STATUS_HEADER)(Info->Gesn.Buffer);
            
            KdPrintEx((DPFLTR_CLASSPNP_ID, ClassDebugMCN,
                       "Classpnp => Fdo %p supports event mask %x\n",
                       FdoExtension->DeviceObject, header->SupportedEventClasses));
            
        
            if (TEST_FLAG(header->SupportedEventClasses,
                          NOTIFICATION_MEDIA_STATUS_CLASS_MASK)) {
                KdPrintEx((DPFLTR_CLASSPNP_ID, ClassDebugMCN,
                           "Classpnp => GESN supports MCN\n"));
            }
            if (TEST_FLAG(header->SupportedEventClasses,
                          NOTIFICATION_DEVICE_BUSY_CLASS_MASK)) {
                KdPrintEx((DPFLTR_CLASSPNP_ID, ClassDebugMCN,
                           "Classpnp => GESN supports DeviceBusy\n"));
            }
            Info->Gesn.EventMask = header->SupportedEventClasses;

            //
            // realistically, we are only considering the following events:
            //    EXTERNAL REQUEST - this is being tested for play/stop/etc.
            //    MEDIA STATUS - autorun and ejection requests.
            //    DEVICE BUSY - to allow us to predict when media will be ready.
            // therefore, we should not bother querying for the other,
            // unknown events. clear all but the above flags.
            //
        
            Info->Gesn.EventMask &=
                NOTIFICATION_EXTERNAL_REQUEST_CLASS_MASK |
                NOTIFICATION_MEDIA_STATUS_CLASS_MASK     |
                NOTIFICATION_DEVICE_BUSY_CLASS_MASK      ;
        
        
            //
            // HACKHACK - REF #0001
            // Some devices will *never* report an event if we've also requested
            // that it report lower-priority events.  this is due to a
            // misunderstanding in the specification wherein a "No Change" is
            // interpreted to be a real event.  what should occur is that the
            // device should ignore "No Change" events when multiple event types
            // are requested unless there are no other events waiting.  this
            // greatly reduces the number of requests that the host must send
            // to determine if an event has occurred. Since we must work on all
            // drives, default to enabling the hack until we find evidence of
            // proper firmware.
            //
        
            if (CountOfSetBitsUChar(Info->Gesn.EventMask) == 1) {
                KdPrintEx((DPFLTR_CLASSPNP_ID, ClassDebugMCN,
                           "Classpnp => GESN hack %s for FDO %p\n",
                           "not required", FdoExtension->DeviceObject));
            } else {
                KdPrintEx((DPFLTR_CLASSPNP_ID, ClassDebugMCN,
                           "Classpnp => GESN hack %s for FDO %p\n",
                           "enabled", FdoExtension->DeviceObject));
                Info->Gesn.HackEventMask = 1;
            }

        } else {
            
            //
            // not the first time looping through, so interpret the results.
            //

            ClasspInterpretGesnData(FdoExtension,
                                    (PVOID)Info->Gesn.Buffer,
                                    &retryImmediately);

        }

    } // end loop of GESN requests....

    //
    // we can only use this if it can be relied upon for media changes,
    // since we are (by definition) no longer going to be polling via
    // a TEST_UNIT_READY irp, and drives will not report UNIT ATTENTION
    // for this command (although a filter driver, such as one for burning
    // cd's, might still fake those errors).
    //
    // since we also rely upon NOT_READY events to change the cursor
    // into a "wait" cursor, we can't use GESN without NOT_READY support.
    //
    
    if (TEST_FLAG(Info->Gesn.EventMask,
                  NOTIFICATION_MEDIA_STATUS_CLASS_MASK) &&
        TEST_FLAG(Info->Gesn.EventMask,
                  NOTIFICATION_DEVICE_BUSY_CLASS_MASK)
        ) {
        
        KdPrintEx((DPFLTR_CLASSPNP_ID, ClassDebugMCN,
                   "Classpnp => Enabling GESN support for fdo %p\n",
                   FdoExtension->DeviceObject));
        Info->Gesn.Supported = TRUE;

        ClassSetDeviceParameter(FdoExtension,
                                CLASSP_REG_SUBKEY_NAME,
                                CLASSP_REG_MMC_DETECTION_VALUE_NAME,
                                ClassDetectionSupported);

        return STATUS_SUCCESS;

    }
    
    KdPrintEx((DPFLTR_CLASSPNP_ID, ClassDebugMCN,
               "Classpnp => GESN available but not enabled for fdo %p\n",
               FdoExtension->DeviceObject));
    goto ExitWithError;

    // fall through...

ExitWithError:
    if (Info->Gesn.Mdl) {
        IoFreeMdl(Info->Gesn.Mdl);
        Info->Gesn.Mdl = NULL;
    }
    if (Info->Gesn.Buffer) {
        ExFreePool(Info->Gesn.Buffer);
        Info->Gesn.Buffer = NULL;
    }
    Info->Gesn.Supported  = 0;
    Info->Gesn.EventMask  = 0;
    Info->Gesn.BufferSize = 0;
    return STATUS_NOT_SUPPORTED;

}

/*++////////////////////////////////////////////////////////////////////////////

ClassInitializeTestUnitPolling()

Routine Description:

    This routine will initialize MCN regardless of the settings stored
    in the registry.  This should be used with caution, as some devices
    react badly to constant io. (i.e. never spin down, continuously cycling
    media in changers, ejection of media, etc.)  It is highly suggested to
    use ClassInitializeMediaChangeDetection() instead.

Arguments:

    FdoExtension is the device to poll
    
    AllowDriveToSleep says whether to attempt to allow the drive to sleep
        or not.  This only affects system-known spin down states, so if a 
        drive spins itself down, this has no effect until the system spins
        it down.

Return Value:

--*/
NTSTATUS
NTAPI
ClassInitializeTestUnitPolling(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN BOOLEAN AllowDriveToSleep
    )
{
    return ClasspInitializePolling(FdoExtension, AllowDriveToSleep);
} // end ClassInitializeTestUnitPolling()

/*++////////////////////////////////////////////////////////////////////////////

ClassInitializeMediaChangeDetection()

Routine Description:

    This routine checks to see if it is safe to initialize MCN (the back end
    to autorun) for a given device.  It will then check the device-type wide
    key "Autorun" in the service key (for legacy reasons), and then look in
    the device-specific key to potentially override that setting.
    
    If MCN is to be enabled, all neccessary structures and memory are
    allocated and initialized.
    
    This routine MUST be called only from the ClassInit() callback.

Arguments:

    FdoExtension - the device to initialize MCN for, if appropriate
    
    EventPrefix - unused, legacy argument.  Set to zero.

Return Value:

--*/
VOID
NTAPI
ClassInitializeMediaChangeDetection(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PUCHAR EventPrefix
    )
{
    PDEVICE_OBJECT fdo = FdoExtension->DeviceObject;
    NTSTATUS status;

    PCLASS_DRIVER_EXTENSION driverExtension = ClassGetDriverExtension(
                                                fdo->DriverObject);

    BOOLEAN disabledForBadHardware;
    BOOLEAN disabled;
    BOOLEAN instanceOverride;

    PAGED_CODE();

    //
    // NOTE: This assumes that ClassInitializeMediaChangeDetection is always
    //       called in the context of the ClassInitDevice callback. If called
    //       after then this check will have already been made and the
    //       once a second timer will not have been enabled.
    //

    disabledForBadHardware = ClasspIsMediaChangeDisabledDueToHardwareLimitation(
                                FdoExtension,
                                &(driverExtension->RegistryPath)
                                );

    if (disabledForBadHardware) {
        DebugPrint((ClassDebugMCN,
                    "ClassInitializeMCN: Disabled due to hardware"
                    "limitations for this device"));
        return;
    }

    //
    // autorun should now be enabled by default for all media types.
    //

    disabled = ClasspIsMediaChangeDisabledForClass(
                    FdoExtension,
                    &(driverExtension->RegistryPath)
                    );

    DebugPrint((ClassDebugMCN,
                "ClassInitializeMCN: Class    MCN is %s\n",
                (disabled ? "disabled" : "enabled")));

    status = ClasspMediaChangeDeviceInstanceOverride(
                FdoExtension,
                &instanceOverride);  // default value

    if (!NT_SUCCESS(status)) {
        DebugPrint((ClassDebugMCN,
                    "ClassInitializeMCN: Instance using default\n"));
    } else {
        DebugPrint((ClassDebugMCN,
                    "ClassInitializeMCN: Instance override: %s MCN\n",
                    (instanceOverride ? "Enabling" : "Disabling")));
        disabled = !instanceOverride;
    }

    DebugPrint((ClassDebugMCN,
                "ClassInitializeMCN: Instance MCN is %s\n",
                (disabled ? "disabled" : "enabled")));

    if (disabled) {
        return;
    }
    
    //
    // if the drive is not a CDROM, allow the drive to sleep
    //
    if (FdoExtension->DeviceObject->DeviceType == FILE_DEVICE_CD_ROM) {
        ClasspInitializePolling(FdoExtension, FALSE);
    } else {
        ClasspInitializePolling(FdoExtension, TRUE);
    }

    return;
} // end ClassInitializeMediaChangeDetection()

/*++////////////////////////////////////////////////////////////////////////////

ClasspMediaChangeDeviceInstanceOverride()

Routine Description:

    The user can override the global setting to enable or disable Autorun on a
    specific cdrom device via the control panel.  This routine checks and/or
    sets this value.

Arguments:

    FdoExtension - the device to set/get the value for
    Value        - the value to use in a set
    SetValue     - whether to set the value

Return Value:

    TRUE - Autorun is disabled
    FALSE - Autorun is not disabled (Default)

--*/
NTSTATUS
NTAPI
ClasspMediaChangeDeviceInstanceOverride(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    OUT PBOOLEAN Enabled
    )
{
    HANDLE                   deviceParameterHandle;  // cdrom instance key
    HANDLE                   driverParameterHandle;  // cdrom specific key
    RTL_QUERY_REGISTRY_TABLE queryTable[3];
    OBJECT_ATTRIBUTES        objectAttributes;
    UNICODE_STRING           subkeyName;
    NTSTATUS                 status;
    ULONG                    alwaysEnable;
    ULONG                    alwaysDisable;
    ULONG                    i;


    PAGED_CODE();

    deviceParameterHandle = NULL;
    driverParameterHandle = NULL;
    status = STATUS_UNSUCCESSFUL;
    alwaysEnable = FALSE;
    alwaysDisable = FALSE;

    TRY {

        status = IoOpenDeviceRegistryKey( FdoExtension->LowerPdo,
                                          PLUGPLAY_REGKEY_DEVICE,
                                          KEY_ALL_ACCESS,
                                          &deviceParameterHandle
                                          );
        if (!NT_SUCCESS(status)) {

            //
            // this can occur when a new device is added to the system
            // this is due to cdrom.sys being an 'essential' driver
            //
            DebugPrint((ClassDebugMCN,
                        "ClassMediaChangeDeviceInstanceDisabled: "
                        "Could not open device registry key [%lx]\n", status));
            LEAVE;
        }

        RtlInitUnicodeString(&subkeyName, MCN_REG_SUBKEY_NAME);
        InitializeObjectAttributes(&objectAttributes,
                                   &subkeyName,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   deviceParameterHandle,
                                   (PSECURITY_DESCRIPTOR) NULL);

        status = ZwCreateKey(&driverParameterHandle,
                             KEY_READ,
                             &objectAttributes,
                             0,
                             (PUNICODE_STRING) NULL,
                             REG_OPTION_NON_VOLATILE,
                             NULL);

        if (!NT_SUCCESS(status)) {
            DebugPrint((ClassDebugMCN,
                        "ClassMediaChangeDeviceInstanceDisabled: "
                        "subkey could not be created. %lx\n", status));
            LEAVE;
        }

        //
        // Default to not changing autorun behavior, based upon setting
        // registryValue to zero.
        //

        for (i=0;i<2;i++) {

            RtlZeroMemory(&queryTable[0], sizeof(queryTable));

            queryTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
            queryTable[0].DefaultType   = REG_DWORD;
            queryTable[0].DefaultLength = 0;

            if (i==0) {
                queryTable[0].Name          = MCN_REG_AUTORUN_DISABLE_INSTANCE_NAME;
                queryTable[0].EntryContext  = &alwaysDisable;
                queryTable[0].DefaultData   = &alwaysDisable;
            } else {
                queryTable[0].Name          = MCN_REG_AUTORUN_ENABLE_INSTANCE_NAME;
                queryTable[0].EntryContext  = &alwaysEnable;
                queryTable[0].DefaultData   = &alwaysEnable;
            }

            //
            // don't care if it succeeds, since we set defaults above
            //

            RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                   (PWSTR)driverParameterHandle,
                                   queryTable,
                                   NULL,
                                   NULL);
        }

    } FINALLY {

        if (driverParameterHandle) ZwClose(driverParameterHandle);
        if (deviceParameterHandle) ZwClose(deviceParameterHandle);

    }

    if (alwaysEnable && alwaysDisable) {

        DebugPrint((ClassDebugMCN,
                    "ClassMediaChangeDeviceInstanceDisabled: %s selected\n",
                    "Both Enable and Disable set -- DISABLE"));
        ASSERT(NT_SUCCESS(status));
        status = STATUS_SUCCESS;
        *Enabled = FALSE;

    } else if (alwaysDisable) {

        DebugPrint((ClassDebugMCN,
                    "ClassMediaChangeDeviceInstanceDisabled: %s selected\n",
                    "DISABLE"));
        ASSERT(NT_SUCCESS(status));
        status = STATUS_SUCCESS;
        *Enabled = FALSE;

    } else if (alwaysEnable) {

        DebugPrint((ClassDebugMCN,
                    "ClassMediaChangeDeviceInstanceDisabled: %s selected\n",
                    "ENABLE"));
        ASSERT(NT_SUCCESS(status));
        status = STATUS_SUCCESS;
        *Enabled = TRUE;

    } else {

        DebugPrint((ClassDebugMCN,
                    "ClassMediaChangeDeviceInstanceDisabled: %s selected\n",
                    "DEFAULT"));
        status = STATUS_UNSUCCESSFUL;

    }

    return status;

} // end ClasspMediaChangeDeviceInstanceOverride()

/*++////////////////////////////////////////////////////////////////////////////

ClasspIsMediaChangeDisabledDueToHardwareLimitation()

Routine Description:

    The key AutoRunAlwaysDisable contains a MULTI_SZ of hardware IDs for
    which to never enable MediaChangeNotification.

    The user can override the global setting to enable or disable Autorun on a
    specific cdrom device via the control panel.

Arguments:

    FdoExtension -
    RegistryPath - pointer to the unicode string inside
                   ...\CurrentControlSet\Services\Cdrom

Return Value:

    TRUE - no autorun.
    FALSE - Autorun may be enabled

--*/
BOOLEAN
NTAPI
ClasspIsMediaChangeDisabledDueToHardwareLimitation(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PUNICODE_STRING RegistryPath
    )
{
    PSTORAGE_DEVICE_DESCRIPTOR deviceDescriptor = FdoExtension->DeviceDescriptor;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE serviceKey = NULL;
    RTL_QUERY_REGISTRY_TABLE parameters[2];

    UNICODE_STRING deviceUnicodeString;
    ANSI_STRING deviceString;
    ULONG mediaChangeNotificationDisabled = FALSE;

    NTSTATUS status;


    PAGED_CODE();

    //
    // open the service key.
    //

    InitializeObjectAttributes(&objectAttributes,
                               RegistryPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = ZwOpenKey(&serviceKey,
                       KEY_READ,
                       &objectAttributes);

    ASSERT(NT_SUCCESS(status));


    if(!NT_SUCCESS(status)) {

        //
        // always take the safe path.  if we can't open the service key,
        // disable autorun
        //

        return TRUE;

    }

    TRY {
        //
        // Determine if drive is in a list of those requiring
        // autorun to be disabled.  this is stored in a REG_MULTI_SZ
        // named AutoRunAlwaysDisable.  this is required as some autochangers
        // must load the disc to reply to ChkVerify request, causing them
        // to cycle discs continuously.
        //

        PWSTR nullMultiSz;
        PCSTR vendorId;
        PCSTR productId;
        PCSTR revisionId;
        ULONG  length;
        ULONG  offset;

        deviceString.Buffer        = NULL;
        deviceUnicodeString.Buffer = NULL;

        //
        // there may be nothing to check against
        //

        if ((deviceDescriptor->VendorIdOffset == 0) &&
            (deviceDescriptor->ProductIdOffset == 0)) {
            LEAVE;
        }

        length = 0;

        if (deviceDescriptor->VendorIdOffset == 0) {
            vendorId = NULL;
        } else {
            vendorId = (PCSTR) deviceDescriptor + deviceDescriptor->VendorIdOffset;
            length = strlen(vendorId);
        }

        if ( deviceDescriptor->ProductIdOffset == 0 ) {
            productId = NULL;
        } else {
            productId = (PCSTR) deviceDescriptor + deviceDescriptor->ProductIdOffset;
            length += strlen(productId);
        }

        if ( deviceDescriptor->ProductRevisionOffset == 0 ) {
            revisionId = NULL;
        } else {
            revisionId = (PCSTR) deviceDescriptor + deviceDescriptor->ProductRevisionOffset;
            length += strlen(revisionId);
        }

        //
        // allocate a buffer for the string
        //

        deviceString.Length = (USHORT)( length );
        deviceString.MaximumLength = deviceString.Length + 1;
        deviceString.Buffer = ExAllocatePoolWithTag( NonPagedPool,
                                                     deviceString.MaximumLength,
                                                     CLASS_TAG_AUTORUN_DISABLE
                                                     );
        if (deviceString.Buffer == NULL) {
            DebugPrint((ClassDebugMCN,
                        "ClassMediaChangeDisabledForHardware: Unable to alloc "
                        "string buffer\n" ));
            LEAVE;
        }

        //
        // copy strings to the buffer
        //
        offset = 0;

        if (vendorId != NULL) {
            RtlCopyMemory(deviceString.Buffer + offset,
                          vendorId,
                          strlen(vendorId));
            offset += strlen(vendorId);
        }

        if ( productId != NULL ) {
            RtlCopyMemory(deviceString.Buffer + offset,
                          productId,
                          strlen(productId));
            offset += strlen(productId);
        }
        if ( revisionId != NULL ) {
            RtlCopyMemory(deviceString.Buffer + offset,
                          revisionId,
                          strlen(revisionId));
            offset += strlen(revisionId);
        }

        ASSERT(offset == deviceString.Length);

        deviceString.Buffer[deviceString.Length] = '\0';  // Null-terminated

        //
        // convert to unicode as registry deals with unicode strings
        //

        status = RtlAnsiStringToUnicodeString( &deviceUnicodeString,
                                               &deviceString,
                                               TRUE
                                               );
        if (!NT_SUCCESS(status)) {
            DebugPrint((ClassDebugMCN,
                        "ClassMediaChangeDisabledForHardware: cannot convert "
                        "to unicode %lx\n", status));
            LEAVE;
        }

        //
        // query the value, setting valueFound to true if found
        //

        RtlZeroMemory(parameters, sizeof(parameters));

        nullMultiSz = L"\0";
        parameters[0].QueryRoutine  = ClasspMediaChangeRegistryCallBack;
        parameters[0].Flags         = RTL_QUERY_REGISTRY_REQUIRED;
        parameters[0].Name          = L"AutoRunAlwaysDisable";
        parameters[0].EntryContext  = &mediaChangeNotificationDisabled;
        parameters[0].DefaultType   = REG_MULTI_SZ;
        parameters[0].DefaultData   = nullMultiSz;
        parameters[0].DefaultLength = 0;

        status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                        serviceKey,
                                        parameters,
                                        &deviceUnicodeString,
                                        NULL);

        if ( !NT_SUCCESS(status) ) {
            LEAVE;
        }

    } FINALLY {

        if (deviceString.Buffer != NULL) {
            ExFreePool( deviceString.Buffer );
        }
        if (deviceUnicodeString.Buffer != NULL) {
            RtlFreeUnicodeString( &deviceUnicodeString );
        }

        ZwClose(serviceKey);
    }

    if (mediaChangeNotificationDisabled) {
        DebugPrint((ClassDebugMCN, "ClassMediaChangeDisabledForHardware: "
                    "Device is on disable list\n"));
        return TRUE;
    }
    return FALSE;

} // end ClasspIsMediaChangeDisabledDueToHardwareLimitation()

/*++////////////////////////////////////////////////////////////////////////////

ClasspIsMediaChangeDisabledForClass()

Routine Description:

    The user must specify that AutoPlay is to run on the platform
    by setting the registry value HKEY_LOCAL_MACHINE\System\CurrentControlSet\
    Services\<SERVICE>\Autorun:REG_DWORD:1.

    The user can override the global setting to enable or disable Autorun on a
    specific cdrom device via the control panel.

Arguments:

    FdoExtension -
    RegistryPath - pointer to the unicode string inside
                   ...\CurrentControlSet\Services\Cdrom

Return Value:

    TRUE - Autorun is disabled for this class
    FALSE - Autorun is enabled for this class

--*/
BOOLEAN
NTAPI
ClasspIsMediaChangeDisabledForClass(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PUNICODE_STRING RegistryPath
    )
{
    //PSTORAGE_DEVICE_DESCRIPTOR deviceDescriptor = FdoExtension->DeviceDescriptor;

    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE serviceKey = NULL;
    HANDLE parametersKey = NULL;
    RTL_QUERY_REGISTRY_TABLE parameters[3];

    UNICODE_STRING paramStr;
    //UNICODE_STRING deviceUnicodeString;
    //ANSI_STRING deviceString;

    //
    //  Default to ENABLING MediaChangeNotification (!)
    //

    ULONG mcnRegistryValue = 1;

    NTSTATUS status;


    PAGED_CODE();

    //
    // open the service key.
    //

    InitializeObjectAttributes(&objectAttributes,
                               RegistryPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = ZwOpenKey(&serviceKey,
                       KEY_READ,
                       &objectAttributes);

    ASSERT(NT_SUCCESS(status));

    if(!NT_SUCCESS(status)) {

        //
        // return the default value, which is the
        // inverse of the registry setting default
        // since this routine asks if it's disabled
        //

        DebugPrint((ClassDebugMCN, "ClassCheckServiceMCN: Defaulting to %s\n",
                    (mcnRegistryValue ? "Enabled" : "Disabled")));
        return (BOOLEAN)(!mcnRegistryValue);

    }

    RtlZeroMemory(parameters, sizeof(parameters));

    //
    // Open the parameters key (if any) beneath the services key.
    //

    RtlInitUnicodeString(&paramStr, L"Parameters");

    InitializeObjectAttributes(&objectAttributes,
                               &paramStr,
                               OBJ_CASE_INSENSITIVE,
                               serviceKey,
                               NULL);

    status = ZwOpenKey(&parametersKey,
                       KEY_READ,
                       &objectAttributes);

    if (!NT_SUCCESS(status)) {
        parametersKey = NULL;
    }



    //
    // Check for the Autorun value.
    //

    parameters[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    parameters[0].Name          = L"Autorun";
    parameters[0].EntryContext  = &mcnRegistryValue;
    parameters[0].DefaultType   = REG_DWORD;
    parameters[0].DefaultData   = &mcnRegistryValue;
    parameters[0].DefaultLength = sizeof(ULONG);

    status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE | RTL_REGISTRY_OPTIONAL,
                                    serviceKey,
                                    parameters,
                                    NULL,
                                    NULL);

    DebugPrint((ClassDebugMCN, "ClassCheckServiceMCN: "
                "<Service>/Autorun flag = %d\n", mcnRegistryValue));

    if(parametersKey != NULL) {

        status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE | RTL_REGISTRY_OPTIONAL,
                                        parametersKey,
                                        parameters,
                                        NULL,
                                        NULL);
        DebugPrint((ClassDebugMCN, "ClassCheckServiceMCN: "
                    "<Service>/Parameters/Autorun flag = %d\n",
                    mcnRegistryValue));
        ZwClose(parametersKey);

    }
    ZwClose(serviceKey);
    
    DebugPrint((ClassDebugMCN, "ClassCheckServiceMCN: "
                "Autoplay for device %p is %s\n",
                FdoExtension->DeviceObject,
                (mcnRegistryValue ? "on" : "off")
                ));

    //
    // return if it is _disabled_, which is the
    // inverse of the registry setting
    //

    return (BOOLEAN)(!mcnRegistryValue);
} // end ClasspIsMediaChangeDisabledForClass()

/*++////////////////////////////////////////////////////////////////////////////

ClassEnableMediaChangeDetection() ISSUE-2000/02/20-henrygab - why public?
ClassEnableMediaChangeDetection() ISSUE-2000/02/20-henrygab - not documented

Routine Description:

    This routine

Arguments:

    DeviceObject - 
    Irp - 

Return Value:

--*/
VOID
NTAPI
ClassEnableMediaChangeDetection(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    PMEDIA_CHANGE_DETECTION_INFO info = FdoExtension->MediaChangeDetectionInfo;
    LONG oldCount;

    PAGED_CODE();

    if(info == NULL) {
        DebugPrint((ClassDebugMCN,
                    "ClassEnableMediaChangeDetection: not initialized\n"));
        return;
    }

    KeWaitForMutexObject(&info->MediaChangeMutex,
                          UserRequest,
                          KernelMode,
                          FALSE,
                          NULL);

    oldCount = --info->MediaChangeDetectionDisableCount;

    ASSERT(oldCount >= 0);

    DebugPrint((ClassDebugMCN, "ClassEnableMediaChangeDetection: Disable count "
                "reduced to %d - ",
                info->MediaChangeDetectionDisableCount));

    if(oldCount == 0) {

        //
        // We don't know what state the media is in anymore.
        //

        ClasspInternalSetMediaChangeState(FdoExtension,
                                          MediaUnknown,
                                          FALSE
                                          );

        //
        // Reset the MCN timer.
        //

        ClassResetMediaChangeTimer(FdoExtension);

        DebugPrint((ClassDebugMCN, "MCD is enabled\n"));

    } else {

        DebugPrint((ClassDebugMCN, "MCD still disabled\n"));

    }


    //
    // Let something else run.
    //

    KeReleaseMutex(&info->MediaChangeMutex, FALSE);

    return;
} // end ClassEnableMediaChangeDetection()

/*++////////////////////////////////////////////////////////////////////////////

ClassDisableMediaChangeDetection() ISSUE-2000/02/20-henrygab - why public?
ClassDisableMediaChangeDetection() ISSUE-2000/02/20-henrygab - not documented

Routine Description:

    This routine

Arguments:

    DeviceObject - 
    Irp - 

Return Value:

--*/
ULONG BreakOnMcnDisable = FALSE;

VOID
NTAPI
ClassDisableMediaChangeDetection(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    PMEDIA_CHANGE_DETECTION_INFO info = FdoExtension->MediaChangeDetectionInfo;

    PAGED_CODE();

    if(info == NULL) {
        return;
    }

    KeWaitForMutexObject(&info->MediaChangeMutex,
                         UserRequest,
                         KernelMode,
                         FALSE,
                         NULL);

    info->MediaChangeDetectionDisableCount++;

    DebugPrint((ClassDebugMCN, "ClassDisableMediaChangeDetection: "
                "disable count is %d\n",
                info->MediaChangeDetectionDisableCount));

    KeReleaseMutex(&info->MediaChangeMutex, FALSE);

    return;
} // end ClassDisableMediaChangeDetection()

/*++////////////////////////////////////////////////////////////////////////////

ClassCleanupMediaChangeDetection() ISSUE-2000/02/20-henrygab - why public?!

Routine Description:

    This routine will cleanup any resources allocated for MCN.  It is called
    by classpnp during remove device, and therefore is not typically required
    by external drivers.

Arguments:

Return Value:

--*/
VOID
NTAPI
ClassCleanupMediaChangeDetection(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    PMEDIA_CHANGE_DETECTION_INFO info = FdoExtension->MediaChangeDetectionInfo;

    PAGED_CODE()

    if(info == NULL) {
        return;
    }

    FdoExtension->MediaChangeDetectionInfo = NULL;
    
    if (info->Gesn.Buffer) {
        ExFreePool(info->Gesn.Buffer);
    }
    IoFreeIrp(info->MediaChangeIrp);
    ExFreePool(info->SenseBuffer);
    ExFreePool(info);
    return;
} // end ClassCleanupMediaChangeDetection()

/*++////////////////////////////////////////////////////////////////////////////

ClasspMcnControl() - ISSUE-2000/02/20-henrygab - not documented

Routine Description:

    This routine

Arguments:

    DeviceObject - 
    Irp - 

Return Value:

--*/
NTSTATUS
NTAPI
ClasspMcnControl(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension =
        (PCOMMON_DEVICE_EXTENSION) FdoExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PPREVENT_MEDIA_REMOVAL request = Irp->AssociatedIrp.SystemBuffer;

    PFILE_OBJECT fileObject = irpStack->FileObject;
    PFILE_OBJECT_EXTENSION fsContext = NULL;

    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    // Check to make sure we have a file object extension to keep track of this
    // request.  If not we'll fail it before synchronizing.
    //

    TRY {

        if(fileObject != NULL) {
            fsContext = ClasspGetFsContext(commonExtension, fileObject);
        }else if(Irp->RequestorMode == KernelMode) { // && fileObject == NULL
            fsContext = &FdoExtension->KernelModeMcnContext;
        }

        if (fsContext == NULL) {

            //
            // This handle isn't setup correctly.  We can't let the
            // operation go.
            //

            status = STATUS_INVALID_PARAMETER;
            LEAVE;
        }

        if(request->PreventMediaRemoval) {

            //
            // This is a lock command.  Reissue the command in case bus or
            // device was reset and the lock was cleared.
            //

            ClassDisableMediaChangeDetection(FdoExtension);
            InterlockedIncrement((PLONG)&fsContext->McnDisableCount);

        } else {

            if(fsContext->McnDisableCount == 0) {
                status = STATUS_INVALID_DEVICE_STATE;
                LEAVE;
            }

            InterlockedDecrement((PLONG)&fsContext->McnDisableCount);
            ClassEnableMediaChangeDetection(FdoExtension);
        }

    } FINALLY {

        Irp->IoStatus.Status = status;

        if(Srb) {
            ExFreePool(Srb);
        }

        ClassReleaseRemoveLock(FdoExtension->DeviceObject, Irp);
        ClassCompleteRequest(FdoExtension->DeviceObject,
                             Irp,
                             IO_NO_INCREMENT);
    }
    return status;
} // end ClasspMcnControl(

/*++////////////////////////////////////////////////////////////////////////////

ClasspMediaChangeRegistryCallBack()

Routine Description:

    This callback for a registry SZ or MULTI_SZ is called once for each
    SZ in the value.  It will attempt to match the data with the
    UNICODE_STRING passed in as Context, and modify EntryContext if a
    match is found.  Written for ClasspCheckRegistryForMediaChangeCompletion

Arguments:

    ValueName     - name of the key that was opened
    ValueType     - type of data stored in the value (REG_SZ for this routine)
    ValueData     - data in the registry, in this case a wide string
    ValueLength   - length of the data including the terminating null
    Context       - unicode string to compare against ValueData
    EntryContext  - should be initialized to 0, will be set to 1 if match found

Return Value:

    STATUS_SUCCESS
    EntryContext will be 1 if found

--*/
NTSTATUS
NTAPI
ClasspMediaChangeRegistryCallBack(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    PULONG valueFound;
    PUNICODE_STRING deviceString;
    PWSTR keyValue;

    PAGED_CODE();
    UNREFERENCED_PARAMETER(ValueName);


    //
    // if we have already set the value to true, exit
    //

    valueFound = EntryContext;
    if ((*valueFound) != 0) {
        DebugPrint((ClassDebugMCN, "ClasspMcnRegCB: already set to true\n"));
        return STATUS_SUCCESS;
    }

    if (ValueLength == sizeof(WCHAR)) {
        DebugPrint((ClassDebugError, "ClasspMcnRegCB: NULL string should "
                    "never be passed to registry call-back!\n"));
        return STATUS_SUCCESS;
    }


    //
    // if the data is not a terminated string, exit
    //

    if (ValueType != REG_SZ) {
        return STATUS_SUCCESS;
    }

    deviceString = Context;
    keyValue = ValueData;
    ValueLength -= sizeof(WCHAR); // ignore the null character

    //
    // do not compare more memory than is in deviceString
    //

    if (ValueLength > deviceString->Length) {
        ValueLength = deviceString->Length;
    }

    //
    // if the strings match, disable autorun
    //

    if (RtlCompareMemory(deviceString->Buffer, keyValue, ValueLength) == ValueLength) {
        DebugPrint((ClassDebugMCN, "ClasspRegMcnCB: Match found\n"));
        DebugPrint((ClassDebugMCN, "ClasspRegMcnCB: DeviceString at %p\n",
                    deviceString->Buffer));
        DebugPrint((ClassDebugMCN, "ClasspRegMcnCB: KeyValue at %p\n",
                    keyValue));
        (*valueFound) = TRUE;
    }

    return STATUS_SUCCESS;
} // end ClasspMediaChangeRegistryCallBack()

/*++////////////////////////////////////////////////////////////////////////////

ClasspTimerTick() - ISSUE-2000/02/20-henrygab - not documented

Routine Description:

    This routine

Arguments:

    DeviceObject - 
    Irp - 

Return Value:

--*/
VOID
NTAPI
ClasspTimerTick(
    PDEVICE_OBJECT DeviceObject,
    PVOID Context
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    ULONG isRemoved;

    ASSERT(commonExtension->IsFdo);

    //
    // Do any media change work
    //
    isRemoved = ClassAcquireRemoveLock(DeviceObject, (PIRP)ClasspTimerTick);

    //
    // We stop the timer before deleting the device.  It's safe to keep going
    // if the flag value is REMOVE_PENDING because the removal thread will be
    // blocked trying to stop the timer.
    //

    ASSERT(isRemoved != REMOVE_COMPLETE);

    //
    // This routine is reasonably safe even if the device object has a pending
    // remove

    if(!isRemoved) {

        PFAILURE_PREDICTION_INFO info = fdoExtension->FailurePredictionInfo;

        //
        // Do any media change detection work
        //

        if (fdoExtension->MediaChangeDetectionInfo != NULL) {

            ClassCheckMediaState(fdoExtension);

        }

        //
        // Do any failure prediction work
        //
        if ((info != NULL) && (info->Method != FailurePredictionNone)) {

            ULONG countDown;
            //ULONG active;

            if (ClasspCanSendPollingIrp(fdoExtension)) {

                //
                // Synchronization is not required here since the Interlocked
                // locked instruction guarantees atomicity. Other code that
                // resets CountDown uses InterlockedExchange which is also
                // atomic.
                //
                countDown = InterlockedDecrement((PLONG)&info->CountDown);
                if (countDown == 0) {

                    DebugPrint((4, "ClasspTimerTick: Send FP irp for %p\n",
                                   DeviceObject));

                    if(info->WorkQueueItem == NULL) {

                        info->WorkQueueItem =
                            IoAllocateWorkItem(fdoExtension->DeviceObject);

                        if(info->WorkQueueItem == NULL) {

                            //
                            // Set the countdown to one minute in the future.
                            // we'll try again then in the hopes there's more
                            // free memory.
                            //

                            DebugPrint((1, "ClassTimerTick: Couldn't allocate "
                                           "item - try again in one minute\n"));
                            InterlockedExchange((PLONG)&info->CountDown, 60);

                        } else {

                            //
                            // Grab the remove lock so that removal will block
                            // until the work item is done.
                            //

                            ClassAcquireRemoveLock(fdoExtension->DeviceObject,
                                                   info->WorkQueueItem);

                            IoQueueWorkItem(info->WorkQueueItem,
                                            ClasspFailurePredict,
                                            DelayedWorkQueue,
                                            info);
                        }

                    } else {

                        DebugPrint((3, "ClasspTimerTick: Failure "
                                       "Prediction work item is "
                                       "already active for device %p\n",
                                    DeviceObject));

                    }
                } // end (countdown == 0)

            } else {
                //
                // If device is sleeping then just rearm polling timer
                DebugPrint((4, "ClassTimerTick, SHHHH!!! device is %p is sleeping\n",
                            DeviceObject));
            }

        } // end failure prediction polling

        //
        // Give driver a chance to do its own specific work
        //

        if (commonExtension->DriverExtension->InitData.ClassTick != NULL) {

            commonExtension->DriverExtension->InitData.ClassTick(DeviceObject);

        } // end device specific tick handler
    } // end check for removed

    ClassReleaseRemoveLock(DeviceObject, (PIRP)ClasspTimerTick);
} // end ClasspTimerTick()

/*++////////////////////////////////////////////////////////////////////////////

ClasspEnableTimer() - ISSUE-2000/02/20-henrygab - not documented

Routine Description:

    This routine

Arguments:

    DeviceObject - 
    Irp - 

Return Value:

--*/
NTSTATUS
NTAPI
ClasspEnableTimer(
    PDEVICE_OBJECT DeviceObject
    )
{
    NTSTATUS status;

    PAGED_CODE();

    if (DeviceObject->Timer == NULL) {

        status = IoInitializeTimer(DeviceObject, ClasspTimerTick, NULL);

    } else {

        status = STATUS_SUCCESS;

    }

    if (NT_SUCCESS(status)) {

        IoStartTimer(DeviceObject);
        DebugPrint((1, "ClasspEnableTimer: Once a second timer enabled "
                    "for device %p\n", DeviceObject));

    }

    DebugPrint((1, "ClasspEnableTimer: Device %p, Status %lx "
                "initializing timer\n", DeviceObject, status));

    return status;

} // end ClasspEnableTimer()

/*++////////////////////////////////////////////////////////////////////////////

ClasspDisableTimer() - ISSUE-2000/02/20-henrygab - not documented

Routine Description:

    This routine

Arguments:

    DeviceObject - 
    Irp - 

Return Value:

--*/
NTSTATUS
NTAPI
ClasspDisableTimer(
    PDEVICE_OBJECT DeviceObject
    )
{
    //PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    //PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    //PMEDIA_CHANGE_DETECTION_INFO mCDInfo = fdoExtension->MediaChangeDetectionInfo;
    //PFAILURE_PREDICTION_INFO fPInfo = fdoExtension->FailurePredictionInfo;
    //NTSTATUS status;

    PAGED_CODE();

    if (DeviceObject->Timer != NULL) {

        //
        // we are only going to stop the actual timer in remove device routine.
        // it is the responsibility of the code within the timer routine to
        // check if the device is removed and not processing io for the final
        // call.
        // this keeps the code clean and prevents lots of bugs.
        //


        IoStopTimer(DeviceObject);
        DebugPrint((3, "ClasspDisableTimer: Once a second timer disabled "
                    "for device %p\n", DeviceObject));

    } else {

        DebugPrint((1, "ClasspDisableTimer: Timer never enabled\n"));

    }

    return STATUS_SUCCESS;
} // end ClasspDisableTimer()

/*++////////////////////////////////////////////////////////////////////////////

ClasspFailurePredict() - ISSUE-2000/02/20-henrygab - not documented

Routine Description:

    This routine

Arguments:

    DeviceObject - 
    Irp - 

Return Value:

Note:  this function can be called (via the workitem callback) after the paging device is shut down,
         so it must be PAGE LOCKED.
--*/
VOID
NTAPI
ClasspFailurePredict(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
{
    PFAILURE_PREDICTION_INFO info = Context;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PIO_WORKITEM workItem;
    STORAGE_PREDICT_FAILURE checkFailure;
    SCSI_ADDRESS scsiAddress;

    NTSTATUS status;

    ASSERT(info != NULL);

    DebugPrint((1, "ClasspFailurePredict: Polling for failure\n"));

    //
    // Mark the work item as inactive and reset the countdown timer.  we
    // can't risk freeing the work item until we've released the remove-lock
    // though - if we do it might get resused as a tag before we can release
    // the lock.
    //

    InterlockedExchange((PLONG)&info->CountDown, info->Period);
    workItem = InterlockedExchangePointer((PVOID*)&info->WorkQueueItem, NULL);

    if (ClasspCanSendPollingIrp(fdoExtension)) {

        KEVENT event;
        PDEVICE_OBJECT topOfStack;
        PIRP irp = NULL;
        IO_STATUS_BLOCK ioStatus;

        KeInitializeEvent(&event, SynchronizationEvent, FALSE);

        topOfStack = IoGetAttachedDeviceReference(DeviceObject);

        //
        // Send down irp to see if drive is predicting failure
        //

        irp = IoBuildDeviceIoControlRequest(
                        IOCTL_STORAGE_PREDICT_FAILURE,
                        topOfStack,
                        NULL,
                        0,
                        &checkFailure,
                        sizeof(STORAGE_PREDICT_FAILURE),
                        FALSE,
                        &event,
                        &ioStatus);


        if (irp != NULL) {
            status = IoCallDriver(topOfStack, irp);
            if (status == STATUS_PENDING) {
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
                status = ioStatus.Status;
            }
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

        if (NT_SUCCESS(status) && (checkFailure.PredictFailure)) {

            checkFailure.PredictFailure = 512;

            //
            // Send down irp to get scsi address
            //
            KeInitializeEvent(&event, SynchronizationEvent, FALSE);

            RtlZeroMemory(&scsiAddress, sizeof(SCSI_ADDRESS));
            irp = IoBuildDeviceIoControlRequest(
                IOCTL_SCSI_GET_ADDRESS,
                topOfStack,
                NULL,
                0,
                &scsiAddress,
                sizeof(SCSI_ADDRESS),
                FALSE,
                &event,
                &ioStatus);

            if (irp != NULL) {
                status = IoCallDriver(topOfStack, irp);
                if (status == STATUS_PENDING) {
                    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
                    status = ioStatus.Status;
                }
            }

            ClassNotifyFailurePredicted(fdoExtension,
                                    (PUCHAR)&checkFailure,
                                    sizeof(checkFailure),
                                    (BOOLEAN)(fdoExtension->FailurePredicted == FALSE),
                                    2,
                                    scsiAddress.PathId,
                                    scsiAddress.TargetId,
                                    scsiAddress.Lun);

            fdoExtension->FailurePredicted = TRUE;

        }

        ObDereferenceObject(topOfStack);
    }

    ClassReleaseRemoveLock(DeviceObject, (PIRP) workItem);
    IoFreeWorkItem(workItem);
    return;
} // end ClasspFailurePredict()

/*++////////////////////////////////////////////////////////////////////////////

ClassNotifyFailurePredicted() ISSUE-alanwar-2000/02/20 - not documented

Routine Description:

Arguments:

Return Value:

--*/
VOID
NTAPI
ClassNotifyFailurePredicted(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    PUCHAR Buffer,
    ULONG BufferSize,
    BOOLEAN LogError,
    ULONG UniqueErrorValue,
    UCHAR PathId,
    UCHAR TargetId,
    UCHAR Lun
    )
{
    PIO_ERROR_LOG_PACKET logEntry;

    DebugPrint((1, "ClasspFailurePredictPollCompletion: Failure predicted for device %p\n", FdoExtension->DeviceObject));

    //
    // Fire off a WMI event
    //
    ClassWmiFireEvent(FdoExtension->DeviceObject,
                                   &StoragePredictFailureEventGuid,
                                   0,
                                   BufferSize,
                                   Buffer);

    //
    // Log an error into the eventlog
    //

    if (LogError)
    {
        logEntry = IoAllocateErrorLogEntry(
                            FdoExtension->DeviceObject,
                           sizeof(IO_ERROR_LOG_PACKET) + (3 * sizeof(ULONG)));

        if (logEntry != NULL)
        {

            logEntry->FinalStatus     = STATUS_SUCCESS;
            logEntry->ErrorCode       = IO_WRN_FAILURE_PREDICTED;
            logEntry->SequenceNumber  = 0;
            logEntry->MajorFunctionCode = IRP_MJ_DEVICE_CONTROL;
            logEntry->IoControlCode   = IOCTL_STORAGE_PREDICT_FAILURE;
            logEntry->RetryCount      = 0;
            logEntry->UniqueErrorValue = UniqueErrorValue;
            logEntry->DumpDataSize    = 3;

            logEntry->DumpData[0] = PathId;
            logEntry->DumpData[1] = TargetId;
            logEntry->DumpData[2] = Lun;

            //
            // Write the error log packet.
            //

            IoWriteErrorLogEntry(logEntry);
        }
    }
} // end ClassNotifyFailurePredicted()

/*++////////////////////////////////////////////////////////////////////////////

ClassSetFailurePredictionPoll()

Routine Description:

    This routine enables polling for failure prediction, setting the timer
    to fire every N seconds as specified by the PollingPeriod.

Arguments:

    FdoExtension - the device to setup failure prediction for.

    FailurePredictionMethod - specific failure prediction method to use
        if set to FailurePredictionNone, will disable failure detection

    PollingPeriod - if 0 then no change to current polling timer

Return Value:

    NT Status

--*/
NTSTATUS
NTAPI
ClassSetFailurePredictionPoll(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    FAILURE_PREDICTION_METHOD FailurePredictionMethod,
    ULONG PollingPeriod
    )
{
    PFAILURE_PREDICTION_INFO info;
    NTSTATUS status;
    //DEVICE_POWER_STATE powerState;

    PAGED_CODE();

    if (FdoExtension->FailurePredictionInfo == NULL) {

        if (FailurePredictionMethod != FailurePredictionNone) {

            info = ExAllocatePoolWithTag(NonPagedPool,
                                         sizeof(FAILURE_PREDICTION_INFO),
                                         CLASS_TAG_FAILURE_PREDICT);

            if (info == NULL) {

                return STATUS_INSUFFICIENT_RESOURCES;

            }

            KeInitializeEvent(&info->Event, SynchronizationEvent, TRUE);

            info->WorkQueueItem = NULL;
            info->Period = DEFAULT_FAILURE_PREDICTION_PERIOD;

        } else {

            //
            // FaultPrediction has not been previously initialized, nor
            // is it being initialized now. No need to do anything.
            //
            return STATUS_SUCCESS;

        }

        FdoExtension->FailurePredictionInfo = info;

    } else {

        info = FdoExtension->FailurePredictionInfo;

    }

    KeWaitForSingleObject(&info->Event,
                          UserRequest,
                          UserMode,
                          FALSE,
                          NULL);


    //
    // Reset polling period and counter. Setup failure detection type
    //

    if (PollingPeriod != 0) {

        InterlockedExchange((PLONG)&info->Period, PollingPeriod);

    }

    InterlockedExchange((PLONG)&info->CountDown, info->Period);

    info->Method = FailurePredictionMethod;
    if (FailurePredictionMethod != FailurePredictionNone) {

        status = ClasspEnableTimer(FdoExtension->DeviceObject);

        if (NT_SUCCESS(status)) {
            DebugPrint((3, "ClassEnableFailurePredictPoll: Enabled for "
                        "device %p\n", FdoExtension->DeviceObject));
        }

    } else {

        status = ClasspDisableTimer(FdoExtension->DeviceObject);
        DebugPrint((3, "ClassEnableFailurePredictPoll: Disabled for "
                    "device %p\n", FdoExtension->DeviceObject));
        status = STATUS_SUCCESS;

    }

    KeSetEvent(&info->Event, IO_NO_INCREMENT, FALSE);

    return status;
} // end ClassSetFailurePredictionPoll()
