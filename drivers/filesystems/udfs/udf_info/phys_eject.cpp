////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*
 Module Name: Phys_eject.cpp

 Execution: Kernel mode only

 Description:

   Contains code that implement read/write operations for physical device
*/

#include            "udf.h"
// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID        UDF_FILE_PHYS_EJECT

extern void
UDFKeyWaiter(
    IN void* Context
    );

/*
    This routine checks for User Eject request & initiates Dismount
 */
void
UDFEjectReqWaiter(
    IN void* Context
    )
{
    PUDFEjectWaitContext WC = (PUDFEjectWaitContext)Context;
    PVCB Vcb;
    OSSTATUS RC = STATUS_SUCCESS;
    OSSTATUS WRC;
    LARGE_INTEGER delay;
    LARGE_INTEGER time;
    BOOLEAN UseEvent = TRUE;
    uint32 d;
    BOOLEAN FlushWCache = FALSE;
    IO_STATUS_BLOCK IoStatus;
    BOOLEAN VcbAcquired;
    BOOLEAN AllFlushed;
    PDEVICE_OBJECT TargetDevObj;
    uint32 BM_FlushPriod;
    uint32 Tree_FlushPriod;
    uint32 SkipCount = 0;
    uint32 SkipEjectCount = 0;
    uint32 flags = 0;
    uint32 flush_stat = 0;
    BOOLEAN UseEject = TRUE;
    BOOLEAN MediaLoss = FALSE;

    BOOLEAN SkipEject = FALSE;
    BOOLEAN SkipFlush = FALSE;

//    BOOLEAN FlushAndEject = FALSE;

    KdPrint(("    UDFEjectReqWaiter: start\n"));
    uint8 supported_evt_classes = 0;
    uint32 i, j;
    uint8 evt_type;
    BOOLEAN OldLowFreeSpace = FALSE;
    uint32 space_check_counter = 0x7fffffff;
    PGET_LAST_ERROR_USER_OUT Error = NULL;

    // Drain out Event Queue
    Vcb = WC->Vcb;
    TargetDevObj = Vcb->TargetDeviceObject;
    UseEvent = Vcb->UseEvent;
    if(UseEvent) {
        supported_evt_classes = EventStat_Class_Media;
    } else {
        KdPrint(("    Eject Button ignored\n"));
    }
    for(j=0; j<4; j++) {
        KdPrint(("    Reading events... (0)\n"));
        if(supported_evt_classes) {
            for(i=1; i<=EventRetStat_Class_Mask;i++) {
                evt_type = (((UCHAR)1) << i);
                if( !(supported_evt_classes & evt_type) )
                    continue;
                ((PGET_EVENT_USER_IN)(&(WC->EjectReqBuffer)))->Immed = TRUE;
                ((PGET_EVENT_USER_IN)(&(WC->EjectReqBuffer)))->EventClass = evt_type;
                
                RC = UDFPhSendIOCTL( IOCTL_CDRW_GET_EVENT,
                                     TargetDevObj,
                                     &(WC->EjectReqBuffer),sizeof(GET_EVENT_USER_IN),
                                     &(WC->EjectReqBuffer),sizeof(GET_EVENT_USER_OUT),
                                     FALSE,NULL);

                if(RC == STATUS_INVALID_DEVICE_REQUEST) {
                    UseEvent = FALSE;
                    break;
                }
                if(RC == STATUS_NO_SUCH_DEVICE)
                    break;
                if(OS_SUCCESS(RC)) {
                    supported_evt_classes = WC->EjectReqBuffer.MediaChange.Header.SupportedClasses;
                }
            }
        }
        if(!UseEvent)
            break;
        if(RC == STATUS_NO_SUCH_DEVICE)
            break;
    }
    supported_evt_classes = 0;

    // Wait for events
    while(TRUE) {
        _SEH2_TRY {

            VcbAcquired = FALSE;
            delay.QuadPart = -10000000; // 1.0 sec
            WRC = KeWaitForSingleObject(&(Vcb->EjectWaiter->StopReq), Executive, KernelMode, FALSE, &delay);
            if(WRC == STATUS_SUCCESS) {
stop_waiter:
//                if(!
                UDFAcquireResourceExclusive(&(Vcb->VCBResource), TRUE);//) {
/*                    delay.QuadPart = -1000000; // 0.1 sec
                    KeDelayExecutionThread(KernelMode, FALSE, &delay);
                    try_return(RC);*/
//                }
                Vcb->EjectWaiter = NULL;
                UDFReleaseResource(&(Vcb->VCBResource));

                KeSetEvent(WC->WaiterStopped, 0, FALSE);
                MyFreePool__(WC);
                WC = NULL;
                KdPrint(("    UDFEjectReqWaiter: exit 3\n"));
                return;
            }
            BM_FlushPriod = Vcb->BM_FlushPriod;
            Tree_FlushPriod = Vcb->Tree_FlushPriod;

            // check if we approaching end of disk
            if(space_check_counter > 2) {
                // update FreeAllocUnits if it is necessary
                if(!(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_READ_ONLY) && Vcb->Modified) {
                    Vcb->FreeAllocUnits = UDFGetFreeSpace(Vcb);
                }
                // update LowFreeSpace flag
                Vcb->LowFreeSpace = (Vcb->FreeAllocUnits < max(Vcb->FECharge,UDF_DEFAULT_FE_CHARGE)*128);
                if(Vcb->LowFreeSpace && !OldLowFreeSpace) {
                    // initiate Flush process if we crossed LowFreeSpace boundary
                    Vcb->Tree_FlushTime = Tree_FlushPriod+1;
                    Vcb->VCBFlags &= ~UDF_VCB_SKIP_EJECT_CHECK;
                }
                OldLowFreeSpace = Vcb->LowFreeSpace;
                space_check_counter = 0;
            }
            space_check_counter++;

            if(Vcb->VCBFlags & UDF_VCB_SKIP_EJECT_CHECK) {
                SkipCount++;
                SkipEjectCount++;
                SkipEject = (SkipEjectCount <= Vcb->SkipEjectCountLimit);
                SkipFlush = (SkipEjectCount <= Vcb->SkipCountLimit);
                if(SkipEject || SkipFlush) {
                    Vcb->VCBFlags &= ~UDF_VCB_SKIP_EJECT_CHECK;
                }
            } else {
                SkipEject = FALSE;
                SkipFlush = FALSE;
            }

            if(WC->SoftEjectReq) {
                SkipEject = FALSE;
                SkipFlush = FALSE;
            }

            if(SkipFlush) {
                Vcb->BM_FlushTime =
                Vcb->Tree_FlushTime = 0;
            } else {
                SkipCount = 0;
            }
            if(!SkipEject) {
                SkipEjectCount = 0;
            }

            if(SkipEject && SkipFlush) {
wait_eject:
                delay.QuadPart = -10000000; // 1.0 sec
                WRC = KeWaitForSingleObject(&(Vcb->EjectWaiter->StopReq), Executive, KernelMode, FALSE, &delay);
                if(WRC == STATUS_SUCCESS) {
                    goto stop_waiter;
                }
                try_return(RC);
            }

            if(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_LOCKED) {
                goto wait_eject;
            }

            // check if the door is still locked
            if(!SkipEject &&
                (Vcb->VCBFlags & UDF_VCB_FLAGS_REMOVABLE_MEDIA) &&
                (Vcb->VCBFlags & UDF_VCB_FLAGS_OUR_DEVICE_DRIVER)) {

                UDFAcquireResourceExclusive(&(Vcb->IoResource), TRUE);
                RC = UDFPhSendIOCTL( IOCTL_CDRW_GET_CAPABILITIES,
                                 TargetDevObj,
                                 NULL,0,
                                 &(WC->DevCap),sizeof(GET_CAPABILITIES_USER_OUT),
                                 FALSE,NULL);

                Error = &(WC->Error);
                UDFPhSendIOCTL( IOCTL_CDRW_GET_LAST_ERROR, Vcb->TargetDeviceObject,
                                NULL,0,
                                Error,sizeof(GET_LAST_ERROR_USER_OUT),
                                TRUE,NULL);
                UDFReleaseResource(&(Vcb->IoResource));
                KdPrint(("SK=%x ASC=%x, ASCQ=%x, IE=%x\n",
                         Error->SenseKey, Error->AdditionalSenseCode, Error->AdditionalSenseCodeQualifier, Error->LastError));
                // check for Long Write In Progress
                if( ((Error->SenseKey == SCSI_SENSE_NOT_READY) &&
                     (Error->AdditionalSenseCode == SCSI_ADSENSE_LUN_NOT_READY) &&
                     (Error->AdditionalSenseCodeQualifier == SCSI_SENSEQ_LONG_WRITE_IN_PROGRESS ||
                      Error->AdditionalSenseCodeQualifier == SCSI_SENSEQ_FORMAT_IN_PROGRESS)) ) {
                    if((!Vcb->Modified &&
                        !(Vcb->VCBFlags & UDF_VCB_LAST_WRITE)) 
                                ||
                       (Vcb->VCBFlags & UDF_VCB_FLAGS_UNSAFE_IOCTL)) {
                          // we should forget about this disk...
                        KdPrint(("    LAST_WRITE %x\n", !!(Vcb->VCBFlags & UDF_VCB_LAST_WRITE)));
                        KdPrint(("    UDF_VCB_FLAGS_UNSAFE_IOCTL %x\n", !!(Vcb->VCBFlags & UDF_VCB_FLAGS_UNSAFE_IOCTL)));
                        KdPrint(("    UDFEjectReqWaiter: Unexpected write-in-progress on !Modified volume\n"));
                        //ASSERT(FALSE);
                        Vcb->ForgetVolume = TRUE;
                        Vcb->VCBFlags |= UDF_VCB_FLAGS_VOLUME_READ_ONLY | UDF_VCB_FLAGS_MEDIA_READ_ONLY;
                        MediaLoss = TRUE;
                        goto device_failure;
                    }
                }
                if(  OS_SUCCESS(RC) &&
                    (Vcb->VCBFlags & UDF_VCB_FLAGS_REMOVABLE_MEDIA) &&
                   !(WC->DevCap.Capabilities2 & DevCap_lock_state)) {
                    // probably bus reset or power failure occured
                    // re-lock tray
                    KdPrint(("    UDFEjectReqWaiter: Unexpected tray unlock encountered. Try to re-lock\n"));

                    UDFAcquireResourceExclusive(&(Vcb->VCBResource), TRUE);
                    VcbAcquired = TRUE;

/*                    UDFResetDeviceDriver(Vcb, TargetDevObj, FALSE);
                    delay.QuadPart = -1000000; // 0.1 sec
                    KeDelayExecutionThread(KernelMode, FALSE, &delay);*/
                    // lock it
                    ((PPREVENT_MEDIA_REMOVAL_USER_IN)(&(WC->DevCap)))->PreventMediaRemoval = TRUE;
                    UDFPhSendIOCTL( IOCTL_STORAGE_MEDIA_REMOVAL,
                                         TargetDevObj,
                                         &(WC->DevCap),sizeof(PREVENT_MEDIA_REMOVAL_USER_IN),
                                         NULL,0,
                                         FALSE,NULL);
                    delay.QuadPart = -1000000; // 0.1 sec
                    KeDelayExecutionThread(KernelMode, FALSE, &delay);
                    // force write mode re-initialization
                    Vcb->LastModifiedTrack = 0;
//                    try_return(RC);
                }
            }

            UDFVVerify(Vcb, 0 /* partial verify */);

            if(!SkipFlush &&
               !(Vcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK) &&
                (BM_FlushPriod || Tree_FlushPriod)) {
                KeQuerySystemTime(&delay);
                d = (uint32)((delay.QuadPart - time.QuadPart) / 10000000);
                time = delay;
                Vcb->BM_FlushTime += d;
                Vcb->Tree_FlushTime += d;

                if(!Vcb->CDR_Mode) {

                    AllFlushed = FALSE;

                    KdPrint(("    SkipCount=%x, SkipCountLimit=%x\n",
                        SkipCount,
                        Vcb->SkipCountLimit));

                    if( Tree_FlushPriod &&
                       (Tree_FlushPriod < Vcb->Tree_FlushTime)) {

                        KdPrint(("    Tree_FlushPriod %I64x, Vcb->Tree_FlushTime %I64x\n",
                            Tree_FlushPriod,
                            Vcb->Tree_FlushTime));

                        // do not touch unchanged volume
                        if((Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_READ_ONLY) ||
                           !Vcb->Modified)
                            goto skip_BM_flush;

                        // Acquire Vcb resource
                        if(!VcbAcquired) {
                            if(!UDFAcquireResourceExclusive(&(Vcb->VCBResource), FALSE)) {
                                delay.QuadPart = -10000000; // 1.0 sec
                                WRC = KeWaitForSingleObject(&(Vcb->EjectWaiter->StopReq), Executive, KernelMode, FALSE, &delay);
                                if(WRC == STATUS_SUCCESS) {
                                    goto stop_waiter;
                                }
                                try_return(RC);
                            }
                            VcbAcquired = TRUE;
                        }

                        KdPrint(("UDF: Flushing Directory Tree....\n"));
                        if( BM_FlushPriod &&
                           (BM_FlushPriod < Vcb->BM_FlushTime)) {
                            KdPrint(("  full flush\n"));
                            flush_stat = UDFFlushADirectory(Vcb, Vcb->RootDirFCB->FileInfo, &IoStatus, UDF_FLUSH_FLAGS_BREAKABLE);
                        } else {
                            KdPrint(("  light flush\n"));
                            flush_stat = UDFFlushADirectory(Vcb, Vcb->RootDirFCB->FileInfo, &IoStatus, UDF_FLUSH_FLAGS_BREAKABLE | UDF_FLUSH_FLAGS_LITE);
                        }
                        if(flush_stat & UDF_FLUSH_FLAGS_INTERRUPTED)
                            try_return(RC);
                        FlushWCache = TRUE;
                        UDFVVerify(Vcb, UFD_VERIFY_FLAG_BG /* partial verify */);
                        //UDFVFlush(Vcb);
skip_BM_flush:
                        Vcb->Tree_FlushTime = 0;
                    }
                    if( BM_FlushPriod &&
                       (BM_FlushPriod < Vcb->BM_FlushTime)) {

                        KdPrint(("    BM_FlushPriod %I64x, Vcb->BM_FlushTime %I64x\n",
                            BM_FlushPriod,
                            Vcb->BM_FlushTime));


                        // do not touch unchanged volume
                        if(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_READ_ONLY)
                            goto skip_BM_flush2;
                        if(!Vcb->Modified)
                            goto skip_BM_flush2;

                        if(!VcbAcquired) {
                            if(!UDFAcquireResourceExclusive(&(Vcb->VCBResource), FlushWCache /*|| FALSE*/)) {
                                delay.QuadPart = -10000000; // 1.0 sec
                                WRC = KeWaitForSingleObject(&(Vcb->EjectWaiter->StopReq), Executive, KernelMode, FALSE, &delay);
                                if(WRC == STATUS_SUCCESS) {
                                    goto stop_waiter;
                                }
                                try_return(RC);
                            }
                            VcbAcquired = TRUE;
                        }

                        UDFAcquireResourceExclusive(&(Vcb->BitMapResource1),TRUE);
//                        UDF_CHECK_BITMAP_RESOURCE(Vcb);

                        if(Vcb->FSBM_ByteCount != RtlCompareMemory(Vcb->FSBM_Bitmap, Vcb->FSBM_OldBitmap, Vcb->FSBM_ByteCount)) {
                            flags |= 1;
                        }
/*                        if(FlushWCache) {
                            AllFlushed = TRUE;
                        }*/
                        AllFlushed =
                        FlushWCache = TRUE;
#ifndef UDF_READ_ONLY_BUILD
                        UDFFlushAllCachedAllocations(Vcb, UDF_PREALLOC_CLASS_FE);
                        UDFFlushAllCachedAllocations(Vcb, UDF_PREALLOC_CLASS_DIR);

                        UDFUpdateVolIdent(Vcb, Vcb->PVolDescAddr, &(Vcb->VolIdent));
                        UDFUpdateVolIdent(Vcb, Vcb->PVolDescAddr2, &(Vcb->VolIdent));

                        if(Vcb->VerifyOnWrite) {
                            KdPrint(("UDF: Flushing cache for verify\n"));
                            //WCacheFlushAll__(&(Vcb->FastCache), Vcb);
                            WCacheFlushBlocks__(&(Vcb->FastCache), Vcb, 0, Vcb->LastLBA);
                            UDFVFlush(Vcb);
                        }
#ifdef UDF_DBG
                        KdPrint(("UDF: Flushing Free Space Bitmap....\n"));

//                        if(!OS_SUCCESS(UDFUpdateVolIdent(Vcb, Vcb->PVolDescAddr, &(Vcb->VolIdent))))
//                            KdPrint(("Error updating VolIdent\n"));
                        if(!OS_SUCCESS(UDFUpdateVDS(Vcb, Vcb->VDS1, Vcb->VDS1 + Vcb->VDS1_Len, flags)))
                            KdPrint(("Error updating Main VDS\n"));
                        if(!OS_SUCCESS(UDFUpdateVDS(Vcb, Vcb->VDS2, Vcb->VDS2 + Vcb->VDS2_Len, flags)))
                            KdPrint(("Error updating Reserve VDS\n"));
#else
                        UDFUpdateVDS(Vcb, Vcb->VDS1, Vcb->VDS1 + Vcb->VDS1_Len, flags);
                        UDFUpdateVDS(Vcb, Vcb->VDS2, Vcb->VDS2 + Vcb->VDS2_Len, flags);
#endif // UDF_DBG
                        // Update Integrity Desc if any
                        if(Vcb->LVid && Vcb->origIntegrityType == INTEGRITY_TYPE_CLOSE) {
                            UDFUpdateLogicalVolInt(Vcb, TRUE);
                        }
                        if(flags & 1) {
                            RtlCopyMemory(Vcb->FSBM_OldBitmap, Vcb->FSBM_Bitmap, Vcb->FSBM_ByteCount);
                        }
#endif //UDF_READ_ONLY_BUILD
                        UDFPreClrModified(Vcb);
                        UDFReleaseResource(&(Vcb->BitMapResource1));
skip_BM_flush2:
                        Vcb->BM_FlushTime = 0;
                    }
                    if(FlushWCache) {
                        FlushWCache = FALSE;
                        WCacheFlushAll__(&(Vcb->FastCache), Vcb);
                    }

                    if(AllFlushed) {
                        //Vcb->Modified = FALSE;
                        UDFClrModified(Vcb);
                    }

                    if(VcbAcquired) {
                        VcbAcquired = FALSE;
                        UDFReleaseResource(&(Vcb->VCBResource));
                    }
                    if(!Vcb->Tree_FlushTime &&
                       !Vcb->BM_FlushTime)
                        SkipCount = 0;
                }
            } else {
                //SkipCount = 0;
            }

            if(!(Vcb->VCBFlags & UDF_VCB_FLAGS_REMOVABLE_MEDIA))
                try_return(RC);

            KdPrint(("    UDFEjectReqWaiter: check removable media\n"));
            if(!WC->SoftEjectReq && SkipEject) {
                try_return(RC);
            }

            if(!WC->SoftEjectReq) {

                if(!UseEvent) {
                    KdPrint(("    Eject Button ignored\n"));
                    try_return(RC);
                }

/*                if( (Vcb->VCBFlags & UDF_VCB_LAST_WRITE) &&
                   !(Vcb->VCBFlags & UDF_VCB_FLAGS_NO_SYNC_CACHE) ){
            //        delay.QuadPart = -100000; // 0.01 sec
            //        KeDelayExecutionThread(KernelMode, FALSE, &delay);
                    OSSTATUS RC;

                    KdPrint(("    Sync cache before GET_EVENT\n"));
                    RC = UDFSyncCache(Vcb);
                    if(RC == STATUS_INVALID_DEVICE_REQUEST) {
                        Vcb->VCBFlags |= UDF_VCB_FLAGS_NO_SYNC_CACHE;
                    }

            //        delay.QuadPart = -300000; // 0.03 sec
            //        KeDelayExecutionThread(KernelMode, FALSE, &delay);
                    Vcb->VCBFlags &= ~UDF_VCB_LAST_WRITE;
                }*/

                ASSERT(sizeof(TEST_UNIT_READY_USER_OUT) <= sizeof(GET_EVENT_USER_OUT));

                RC = UDFTSendIOCTL( IOCTL_CDRW_TEST_UNIT_READY,
                                     Vcb,
                                     NULL,0,
                                     &(WC->EjectReqBuffer),sizeof(TEST_UNIT_READY_USER_OUT),
                                     FALSE,NULL);

                if(RC != STATUS_SUCCESS &&
                   RC != STATUS_DATA_OVERRUN) {
                    if(RC == STATUS_NO_SUCH_DEVICE) {
                        KdPrint(("    Device loss\n"));
                        goto device_failure;
                    }
                    if(RC == STATUS_NO_MEDIA_IN_DEVICE) {
                        KdPrint(("    Media loss\n"));
                        goto media_loss;
                    }
                }
                KdPrint(("    Reading events...\n"));
                if(supported_evt_classes) {
                    for(i=1; i<=EventRetStat_Class_Mask;i++) {
                        evt_type = (((UCHAR)1) << i);
                        if( !(supported_evt_classes & evt_type) )
                            continue;
/*
                        if( evt_type == EventStat_Class_Media ) 
                            continue;
                        if( evt_type == EventStat_Class_ExternalReq )
                            continue;
*/
                        ((PGET_EVENT_USER_IN)(&(WC->EjectReqBuffer)))->Immed = TRUE;
                        ((PGET_EVENT_USER_IN)(&(WC->EjectReqBuffer)))->EventClass = evt_type;
                        
                        RC = UDFTSendIOCTL( IOCTL_CDRW_GET_EVENT,
                                             Vcb,
                                             &(WC->EjectReqBuffer),sizeof(GET_EVENT_USER_IN),
                                             &(WC->EjectReqBuffer),sizeof(GET_EVENT_USER_OUT),
                                             FALSE,NULL);

                        if(RC == STATUS_INVALID_DEVICE_REQUEST) {
                            supported_evt_classes &= ~evt_type;
                            continue;
                        }
                        if(RC == STATUS_NO_SUCH_DEVICE) {
                            KdPrint(("    Device loss (2)\n"));
                            goto device_failure;
                        }
                        if(!OS_SUCCESS(RC)) {
                            continue;
                        }

                        if(WC->EjectReqBuffer.MediaChange.Header.Flags.Flags & EventRetStat_NEA) {
                            continue;
                        }
                        if( evt_type == EventStat_Class_Media ) {
                            KdPrint(("    EventStat_Class_Media:\n"));
                            if((WC->EjectReqBuffer.MediaChange.Header.Flags.Flags & EventRetStat_Class_Mask) !=
                                EventRetStat_Class_Media) {
                                continue;
                            }
retry_media_presence_check:
                            if(!(WC->EjectReqBuffer.MediaChange.Byte1.Flags & EventStat_MediaStat_Present) ||
                                (WC->EjectReqBuffer.MediaChange.Byte1.Flags & EventStat_MediaStat_DoorOpen)) {
                                // something wrong....
                                RC = UDFTSendIOCTL( IOCTL_CDRW_TEST_UNIT_READY,
                                                     Vcb,
                                                     NULL,0,
                                                     &(WC->EjectReqBuffer),sizeof(TEST_UNIT_READY_USER_OUT),
                                                     FALSE,NULL);

                                if(RC == STATUS_SUCCESS ||
                                   RC == STATUS_DATA_OVERRUN) {
                                    KdPrint(("    Buggy GET_EVENT media presence flag %x\n",
                                        WC->EjectReqBuffer.MediaChange.Byte1));
                                    WC->EjectReqBuffer.MediaChange.Byte1.Flags |= EventStat_MediaStat_Present;
                                    WC->EjectReqBuffer.MediaChange.Byte1.Flags &= ~EventStat_MediaStat_DoorOpen;
                                    goto retry_media_presence_check;
                                }
media_loss:
                                KdPrint(("    Unexpected media loss. Check device status\n"));
                                UseEject = FALSE;
                                MediaLoss = TRUE;
                            } else
                            // check if eject request occured
                            if( (WC->EjectReqBuffer.MediaChange.Byte0.Flags & EventStat_MediaEvent_Mask) !=
                                           EventStat_MediaEvent_EjectReq ) {
                                continue;
                            }
                            KdPrint(("    eject requested\n"));
                            WC->SoftEjectReq = TRUE;
                            break;
                        }
                        if( evt_type == EventStat_Class_ExternalReq ) {
                            KdPrint(("    EventStat_Class_ExternalReq:\n"));
                            if((WC->EjectReqBuffer.ExternalReq.Header.Flags.Flags & EventRetStat_Class_Mask) !=
                                EventRetStat_Class_ExternReq)
                                continue;
                            switch(WC->EjectReqBuffer.ExternalReq.Byte0.Flags & EventStat_ExtrnReqEvent_Mask) {
                            case EventStat_ExtrnReqEvent_KeyDown:
                            case EventStat_ExtrnReqEvent_KeyUp:
                            case EventStat_ExtrnReqEvent_ExtrnReq:
                                KdPrint(("    eject requested (%x)\n", WC->EjectReqBuffer.ExternalReq.Byte0.Flags));
                                WC->SoftEjectReq = TRUE;
                                break;
                            }
                            continue;
                        }
                    }
                    if(!supported_evt_classes) {
                        UseEvent = FALSE;
                    }
                    if(!WC->SoftEjectReq) {
                        try_return(RC);
                    }
                } else {

                    KdPrint(("    Reading Media Event...\n"));
                    ((PGET_EVENT_USER_IN)(&(WC->EjectReqBuffer)))->Immed = TRUE;
                    ((PGET_EVENT_USER_IN)(&(WC->EjectReqBuffer)))->EventClass = EventStat_Class_Media;
                
                    RC = UDFTSendIOCTL( IOCTL_CDRW_GET_EVENT,
                                         Vcb,
                                         &(WC->EjectReqBuffer),sizeof(GET_EVENT_USER_IN),
                                         &(WC->EjectReqBuffer),sizeof(GET_EVENT_USER_OUT),
                                         FALSE,NULL);

                    if(!OS_SUCCESS(RC)) {
                        if(RC == STATUS_NO_SUCH_DEVICE)
                            goto device_failure;
                        ((PGET_EVENT_USER_IN)(&(WC->EjectReqBuffer)))->Immed = TRUE;
                        ((PGET_EVENT_USER_IN)(&(WC->EjectReqBuffer)))->EventClass = EventStat_Class_ExternalReq;
                
                        RC = UDFTSendIOCTL( IOCTL_CDRW_GET_EVENT,
                                             Vcb,
                                             &(WC->EjectReqBuffer),sizeof(GET_EVENT_USER_IN),
                                             &(WC->EjectReqBuffer),sizeof(GET_EVENT_USER_OUT),
                                             FALSE,NULL);

                        if(RC == STATUS_NO_SUCH_DEVICE)
                            goto device_failure;
                        if(RC == STATUS_INVALID_DEVICE_REQUEST) {
                            UseEvent = FALSE;
                        }
                        try_return(RC);
                    }
                    supported_evt_classes = WC->EjectReqBuffer.MediaChange.Header.SupportedClasses;
                    try_return(RC);
                }
            }
//            FlushAndEject = TRUE;
device_failure:
            // Ok. Lets flush all we have in memory, dismount volume & eject disc
            // Acquire Vcb resource
            Vcb->SoftEjectReq = TRUE;

            KdPrint(("    UDFEjectReqWaiter: ejecting...\n"));
#ifdef UDF_DELAYED_CLOSE
            UDFAcquireResourceExclusive(&(Vcb->VCBResource), TRUE);
            KdPrint(("    UDFEjectReqWaiter:     set UDF_VCB_FLAGS_NO_DELAYED_CLOSE\n"));
            Vcb->VCBFlags |= UDF_VCB_FLAGS_NO_DELAYED_CLOSE;
            UDFReleaseResource(&(Vcb->VCBResource));
#endif //UDF_DELAYED_CLOSE

            KdPrint(("    UDFEjectReqWaiter:     UDFCloseAllSystemDelayedInDir\n"));
            RC = UDFCloseAllSystemDelayedInDir(Vcb, Vcb->RootDirFCB->FileInfo);
            ASSERT(OS_SUCCESS(RC));
#ifdef UDF_DELAYED_CLOSE
            KdPrint(("    UDFEjectReqWaiter:     UDFCloseAllDelayed\n"));
            UDFCloseAllDelayed(Vcb);
            //ASSERT(OS_SUCCESS(RC));
#endif //UDF_DELAYED_CLOSE

            UDFAcquireResourceExclusive(&(Vcb->VCBResource), TRUE);

            KdPrint(("    UDFEjectReqWaiter:     UDFDoDismountSequence\n"));
            UDFDoDismountSequence(Vcb, (PPREVENT_MEDIA_REMOVAL_USER_IN)&(WC->EjectReqBuffer), UseEject);
            if (MediaLoss) {
                Vcb->VCBFlags &= ~UDF_VCB_FLAGS_VOLUME_MOUNTED;
                Vcb->WriteSecurity = FALSE;
            }
            Vcb->EjectWaiter = NULL;
            Vcb->SoftEjectReq = FALSE;
            UDFReleaseResource(&(Vcb->VCBResource));

            KdPrint(("    UDFEjectReqWaiter:     set WaiterStopped\n"));
            KeSetEvent(WC->WaiterStopped, 0, FALSE);
            MyFreePool__(WC);
            WC = NULL;

            KdPrint(("    UDFEjectReqWaiter: exit 1\n"));
            return;
    
try_exit:   NOTHING;
        } _SEH2_FINALLY {
    
            if(VcbAcquired) {
                VcbAcquired = FALSE;
                UDFReleaseResource(&(Vcb->VCBResource));
            }
            
/*            if(WC) {
                delay.QuadPart = -10000000; // 1.0 sec
                WRC = KeWaitForSingleObject(&(Vcb->WaiterStopped), Executive, KernelMode, FALSE, &delay);
                if(WRC == STATUS_SUCCESS) {
                    goto stop_waiter;
                }
            }*/
        } _SEH2_END;
    }
    // Simply make compiler happy
    return;
} // end UDFEjectReqWaiter()

void
UDFStopEjectWaiter(PVCB Vcb) {

    KdPrint(("    UDFStopEjectWaiter: try\n"));
    UDFAcquireResourceExclusive(&(Vcb->VCBResource), TRUE);
    _SEH2_TRY {
        if(Vcb->EjectWaiter) {
            KdPrint(("    UDFStopEjectWaiter: set flag\n"));
            KeSetEvent( &(Vcb->EjectWaiter->StopReq), 0, FALSE );
        } else {
//            return;
        }
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;
    UDFReleaseResource(&(Vcb->VCBResource));

    _SEH2_TRY {
        if(Vcb->VCBFlags & UDF_VCB_FLAGS_STOP_WAITER_EVENT) {
            KdPrint(("    UDFStopEjectWaiter: wait\n"));
            KeWaitForSingleObject(&(Vcb->WaiterStopped), Executive, KernelMode, FALSE, NULL);
        }
        Vcb->VCBFlags &= ~UDF_VCB_FLAGS_STOP_WAITER_EVENT;
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;
    ASSERT(!Vcb->EjectWaiter);
    KdPrint(("    UDFStopEjectWaiter: exit\n"));

} // end UDFStopEjectWaiter()

OSSTATUS
UDFDoDismountSequence(
    IN PVCB Vcb,
    IN PPREVENT_MEDIA_REMOVAL_USER_IN Buf,
    IN BOOLEAN Eject
    )
{
    LARGE_INTEGER delay;
//    OSSTATUS      RC;
    ULONG i;

    // flush system cache
    UDFFlushLogicalVolume(NULL, NULL, Vcb, 0);
    KdPrint(("UDFDoDismountSequence:\n"));

    delay.QuadPart = -1000000; // 0.1 sec
    KeDelayExecutionThread(KernelMode, FALSE, &delay);
    // wait for completion of all backgroung writes
    while(Vcb->BGWriters) {
        delay.QuadPart = -5000000; // 0.5 sec
        KeDelayExecutionThread(KernelMode, FALSE, &delay);
    }
    // release WCache
    WCacheRelease__(&(Vcb->FastCache));

    UDFAcquireResourceExclusive(&(Vcb->IoResource), TRUE);

    // unlock media, drop our own Locks
    if(Vcb->VCBFlags & UDF_VCB_FLAGS_REMOVABLE_MEDIA) {
        KdPrint(("  cleanup tray-lock (%d+2):\n", Vcb->MediaLockCount));
        for(i=0; i<Vcb->MediaLockCount+2; i++) {
            Buf->PreventMediaRemoval = FALSE;
            UDFPhSendIOCTL(IOCTL_STORAGE_MEDIA_REMOVAL,
                           Vcb->TargetDeviceObject,
                           Buf,sizeof(PREVENT_MEDIA_REMOVAL_USER_IN),
                           NULL,0,
                           FALSE,NULL);
            KeDelayExecutionThread(KernelMode, FALSE, &delay);
        }
        delay.QuadPart = -2000000; // 0.2 sec
    }

    if(!Vcb->ForgetVolume) {

        if(!UDFIsDvdMedia(Vcb)) {
            // send speed limits to drive
            KdPrint(("    Restore drive speed on dismount\n"));
            Vcb->SpeedBuf.ReadSpeed  = Vcb->MaxReadSpeed;
            Vcb->SpeedBuf.WriteSpeed = Vcb->MaxWriteSpeed;
            UDFPhSendIOCTL(IOCTL_CDRW_SET_SPEED,
                                Vcb->TargetDeviceObject,
                                &(Vcb->SpeedBuf),sizeof(SET_CD_SPEED_USER_IN),
                                NULL,0,TRUE,NULL);
        }

        if(Vcb->VCBFlags & UDF_VCB_FLAGS_OUR_DEVICE_DRIVER) {
            CLOSE_TRK_SES_USER_IN CBuff;

            // reset driver
            UDFResetDeviceDriver(Vcb, Vcb->TargetDeviceObject, TRUE);
            delay.QuadPart = -2000000; // 0.2 sec
            KeDelayExecutionThread(KernelMode, FALSE, &delay);

            memset(&CBuff,0,sizeof(CLOSE_TRK_SES_USER_IN));
            // stop BG format
            if(Vcb->MRWStatus) {
                KdPrint(("    Stop background formatting\n"));

                CBuff.Byte1.Flags = 0;//CloseTrkSes_Immed;
                CBuff.Byte2.Flags = CloseTrkSes_Ses;
                CBuff.TrackNum = 1;

                UDFPhSendIOCTL(IOCTL_CDRW_CLOSE_TRK_SES,
                               Vcb->TargetDeviceObject,
                               &CBuff,sizeof(CLOSE_TRK_SES_USER_IN),
                               &CBuff,sizeof(CLOSE_TRK_SES_USER_IN),
                               FALSE, NULL );
    /*        } else
            if(Vcb->MediaClassEx == CdMediaClass_DVDRW) {
                KdPrint(("    Close BG-formatted track\n"));

                CBuff.Byte1.Flags = 0;//CloseTrkSes_Immed;
                CBuff.Byte2.Flags = CloseTrkSes_Trk;
                CBuff.TrackNum = 1;

                RC = UDFPhSendIOCTL(IOCTL_CDRW_CLOSE_TRK_SES,
                                    Vcb->TargetDeviceObject,
                                    &CBuff,sizeof(CLOSE_TRK_SES_USER_IN),
                                    &CBuff,sizeof(CLOSE_TRK_SES_USER_IN),
                                    FALSE, NULL );
    */
            }
            // reset driver
            UDFResetDeviceDriver(Vcb, Vcb->TargetDeviceObject, TRUE);
            delay.QuadPart = -1000000; // 0.1 sec
            KeDelayExecutionThread(KernelMode, FALSE, &delay);
        }
        // eject media
        if(Eject &&
           (Vcb->VCBFlags & UDF_VCB_FLAGS_REMOVABLE_MEDIA)) {

            UDFPhSendIOCTL(IOCTL_STORAGE_EJECT_MEDIA,
                           Vcb->TargetDeviceObject,
                           NULL,0,
                           NULL,0,
                           FALSE,NULL);
        }
        // notify media change
    /*    if(Vcb->VCBFlags & UDF_VCB_FLAGS_OUR_DEVICE_DRIVER) {
            ((PNOTIFY_MEDIA_CHANGE_USER_IN)Buf)->Autorun = FALSE;
            RC = UDFPhSendIOCTL(IOCTL_CDRW_NOTIFY_MEDIA_CHANGE,
                                Vcb->TargetDeviceObject,
                                Buf,sizeof(NOTIFY_MEDIA_CHANGE_USER_IN),
                                NULL,0,
                                FALSE,NULL);
        }*/
    }
    UDFReleaseResource(&(Vcb->IoResource));
    // unregister shutdown notification
    if(Vcb->ShutdownRegistered) {
        IoUnregisterShutdownNotification(Vcb->VCBDeviceObject);
        Vcb->ShutdownRegistered = FALSE;
    }
    // allow media change checks (this will lead to dismount)
    // ... and make it Read-Only...  :-\~
    Vcb->VCBFlags &= ~UDF_VCB_FLAGS_MEDIA_LOCKED;

    // Return back XP CD Burner Volume
/*
    if (Vcb->CDBurnerVolumeValid) {
        RtlWriteRegistryValue(RTL_REGISTRY_USER | RTL_REGISTRY_OPTIONAL,
                      REG_CD_BURNER_KEY_NAME,REG_CD_BURNER_VOLUME_NAME,
                      REG_SZ,(PVOID)&(Vcb->CDBurnerVolume),sizeof(Vcb->CDBurnerVolume));
        ExFreePool(Vcb->CDBurnerVolume.Buffer);
    }
*/
    KdPrint(("  set UnsafeIoctl\n"));
    Vcb->VCBFlags |= UDF_VCB_FLAGS_UNSAFE_IOCTL;

    return STATUS_SUCCESS;
} // end UDFDoDismountSequence()

