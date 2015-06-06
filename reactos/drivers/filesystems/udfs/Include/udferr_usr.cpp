////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////
#include "udferr_usr.h"

struct err_msg_item mkudf_err_msg[] = {
    {MKUDF_OK                       , "Format completed succesfully"},
    #include "udferr_usr_cpp.h"
    //{MKUDF_HW_READ_ONLY             , "Read-only media"},
    //{MKUDF_CANT_BLANK               , "Can't erase disk"},
    {MKUDF_INVALID_PARAM_MT         , "Unknown media type specified"},
    {MKUDF_INVALID_PARAM_PT         , "Unknown partition type requested"},
    {MKUDF_INVALID_PARAM            , "Invalid comand line"},
    {MKUDF_HW_CANT_READ_LAYOUT      , "Can't read disk layout"},
    {MKUDF_AUTO_BLOCKCOUNT_FAILED   , "Can't obtain last available block address"},
    {MKUDF_CANT_RECORD_BEA01        , "Write error: Can't record BEA01 descriptor"},
    {MKUDF_CANT_RECORD_NSR02        , "Write error: Can't record NSR02 descriptor"},
    {MKUDF_CANT_RECORD_TEA01        , "Write error: Can't record TEA01 descriptor"},
    {MKUDF_CANT_RECORD_ANCHOR       , "Write error: Can't record Anchor Point descriptor"},
    {MKUDF_CANT_RECORD_PVD          , "Write error: Can't record Primary Volume descriptor"},
    {MKUDF_CANT_RECORD_LVD          , "Write error: Can't record Logical Volume descriptor"},
    {MKUDF_CANT_RECORD_LVID         , "Write error: Can't record Logical Volume Integrity descriptor"},
    {MKUDF_CANT_RECORD_PARTD        , "Write error: Can't record Partition descriptor"},
    {MKUDF_CANT_RECORD_USD          , "Write error: Can't record Unallocated Space descriptor"},
    {MKUDF_CANT_RECORD_IUVD         , "Write error: Can't record Implementation Use Volume descriptor"},
    {MKUDF_CANT_RECORD_TERMD        , "Write error: Can't record Terminating descriptor"},
    {MKUDF_CANT_RECORD_FSD          , "Write error: Can't record Free Space descriptor"},
    {MKUDF_CANT_RECORD_SPT          , "Write error: Can't record File Set descriptor"},
    {MKUDF_PARTITION_TOO_SHORT      , "Requested partition size is too small."},
    {MKUDF_CANT_RECORD_FSBM         , "Write error: Can't record Free Space Bitmap"},
    {MKUDF_CANT_RECORD_ROOT_FE      , "Write error: Can't record Root File Entry"},
    {MKUDF_CANT_RECORD_VAT_FE       , "Write error: Can't record VAT File Entry"},
    {MKUDF_CANT_OPEN_FILE           , "Can't open device or image file"},
    {MKUDF_CANT_RESERVE_TRACK       , "Can't reserve track for UDF VAT partition"},
    {MKUDF_VAT_MULTISESS_NOT_SUPP   , "Can't add UDF VAT session.\n"
                                        "Blank (empty) media required"},
    {MKUDF_NOT_ENOUGH_PARAMS        , "Too few command line parameters"},
    {MKUDF_INVALID_PT_FOR_HDD       , "Invalid partition type for HDD"},
    {MKUDF_INVALID_PT_FOR_WORM      , "Invalid partition type for CD-R"},
    {MKUDF_CANT_FORMAT              , "Can't format media"},
    {MKUDF_MEDIA_TYPE_NOT_SUPP      , "Unsupported media type"},
    {MKUDF_INVALID_PARAM_BC_FOR_VAT , "Block Count should not be specified for VAT partitions"},
    {MKUDF_HW_PARTITION_TOO_SHORT   , "Not enough free space on media"},
    {MKUDF_CANT_LOCK_VOL            , "Can't open volume for exclusive use"},
    {MKUDF_CANT_SET_WPARAMS         , "Can't set Write Parameters to prepare for recording"},
    {MKUDF_HW_WRITE_ERROR           , "Device write error"},
    {MKUDF_BLANK_MEDIA_REQUIRED     , "Blank (empty) media required"},
    {MKUDF_INVALID_PT_FOR_BLANK     , "Partition Type should not be specified for Erase operation"},
    {MKUDF_FINALIZE_TOO_MANY_PARAMS , "Extra parameters for VAT partition finalization"},
    {MKUDF_NO_ANCHOR_FOUND          , "Can't locate UDF VAT partition: No valid Anchor Point descriptor found"},
    {MKUDF_HW_INVALID_NWA           , "Invalid Next Writable Address returned"},
    {MKUDF_NO_MEDIA_IN_DEVICE       , "No media in device"},
    {MKUDF_INVALID_BC               , "Invalid Block Count"},
    {MKUDF_INVALID_MT_FOR_BLANK     , "Can't erase CD/DVD-R"},
    {MKUDF_HW_CANT_SET_WRITE_PARAMS , "Can't set proper write mode"},
    {MKUDF_CANT_ALLOW_DASD_IO       , "Direct access to volume not permitted"},
    {MKUDF_CANT_DISMOUNT_VOLUME     , "Can't dismount volume"},
    {MKUDF_FORMAT_REQUIRED          , "Disc is not formatted (empty).\n"
                                        "Physical format required."},
    {MKUDF_FULL_BLANK_REQUIRED      , "CD/DVD Drive requires Full Erase to proceed with format"},
    {MKUDF_NO_SUITABLE_MODE_FOR_FMT , "Can't find suitable write mode to format media"},
    {MKUDF_CANT_BLANK_DVDRAM        , "DVD-RAM is not erasable"},
    {MKUDF_CANT_RESERVE_ISO_SPACE   , "Can't allocate space for ISO structures"},
    {MKUDF_CANT_RECORD_NOTALLOC_FE  , "Can't record Non-Allocated Space File Entry"},
    {MKUDF_BAD_BLOCK_IN_SYSTEM_AREA , "System area of the disk is completely damaged.\n"
                                        "Sectors between 0 and 2048 are unusable."},
    {MKUDF_CANT_OPEN_ISO_IMAGE      , "Can't open ISO image"},
    {MKUDF_BAD_ISO_IMAGE_ALIGN      , "ISO image must be aligned to sector size (2048 bytes)"},
    {MKUDF_CANT_READ_ISO_IMAGE      , "ISO image read error"},
    {MKUDF_CANT_WRITE_ISO_IMAGE     , "Can't write ISO image to CD"},
    {MKUDF_INVALID_PARAM_SPEED      , "Invalid speed value"},
    {MKUDF_CANT_MAKE_WINUDF         , "System area of the disk contains some bad blocks.\n"
                                        "Can't allocate disk structres in Windows-compatible fashion"},
    {MKUDF_FLUSH_ERROR              , "Error during data flush. Disk may become unreadable or readonly"},
    {MKUDF_FINAL_VERIFY_FAILED      , "Final post-formatting Volume verification failed"},
    {MKUDF_INSUFFICIENT_MEMORY      , "Insufficient memory"},
    {MKUDF_CANT_CREATE_THREAD       , "Can't start formatting thread"},
    {MKUDF_CANT_APPLY_R             , "Can't use media as CD/DVD-R"},
    {MKUDF_USER_BREAK               , "Operation aborted by user request"},
    {MKUDF_INVALID_USER_BUFFER      , "Invalid user-supplied buffer"},
    {MKUDF_INTERNAL_ERROR           , "Internal error"},
    {MKUDF_INVALID_PARAM_VFIN       , "Invalid filename for input bad block list"},
    {MKUDF_INVALID_PARAM_VFOUT      , "Invalid filename for bad block list storage"},
    {MKUDF_CANT_CREATE_BB_LOG       , "Can't create file for bad block list"},
    {MKUDF_CANT_OPEN_BB_LOG         , "Can't open file with bad block list"},
    {MKUDF_INSUFFICIENT_PRIVILIGES  , "Insufficient privileges. Administrative rights required."},
    {MKUDF_BLANK_FORMAT_REQUIRED    , "Disc is not formatted (contains not Packet-formatted tracks).\n"
                                        "Erase and Physical format required."},
    {MKUDF_NO_DEVICE_NAME           , "No device name specified."},
    {MKUDF_CANT_FLUSH               , "Can't flush volume."},
    {MKUDF_INVALID_PARAM_ISO_MODE   , "Invalid sector mode for ISO recording"},
    {MKUDF_INVALID_PARAM_ISO_SES    , "Invalid multisession mode for ISO recording"},
    {MKUDF_SMART_BLANK_FORMAT_FAILED, "Disc is not empty.\n"
                                        "Automatic erase+format failed."},
    {MKUDF_SMART_FORMAT_FAILED,       "Disc is empty.\n"
                                        "Automatic format failed."},
    {MKUDF_OTHER_PACKET_FS          , "Disc is physically formatted, but contains other FS structures"},
    {MKUDF_RAW_PACKET_FS            , "Disc is physically formatted and doesn't contain FS structures"},
    {MKUDF_CANT_ZERO                , "Zero-filling failed after Erase-via-Format workaround."},

    {MKUDF_NO_UNERASE_FOR_THIS_MEDIA, "Unerase is available for CD media only."},
    {MKUDF_UNERASE_FAILED           , "Can't start unerase process."},
    {MKUDF_INVALID_PARAM_REVISION   , "Invalid UDF FS Revision."},
    {MKUDF_FORMAT_IN_PROGRESS       , "Format or Erase is already started for this device."},

    {MKUDF_CANT_CREATE_ISO_IMAGE    , "Can't create ISO image"},

    {MKUDF_ABORTED                  , "Aborted by user"},
//    {MKUDF_INVALID_MT_FOR_BLANK     , "Can't blank WORM"},
//    {MKUDF_INVALID_MT_FOR_BLANK     , "Can't blank WORM"},

    {CHKUDF_CANT_MOUNT              , "Can't mount volume for checking"},
    
    {0xffffffff                     , "Unknown error"}
};

