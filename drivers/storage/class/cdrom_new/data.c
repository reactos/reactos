/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:


Abstract:


Environment:


Notes:

Revision History:

--*/

#include "cdrom.h"

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGE")
#endif

/*

#define CDROM_HACK_DEC_RRD                 (0x00000001)
#define CDROM_HACK_FUJITSU_FMCD_10x        (0x00000002)
#define CDROM_HACK_HITACHI_1750            (0x00000004)
#define CDROM_HACK_HITACHI_GD_2000         (0x00000008)
#define CDROM_HACK_TOSHIBA_SD_W1101        (0x00000010)
#define CDROM_HACK_TOSHIBA_XM_3xx          (0x00000020)
#define CDROM_HACK_NEC_CDDA                (0x00000040)
#define CDROM_HACK_PLEXTOR_CDDA            (0x00000080)
#define CDROM_HACK_BAD_GET_CONFIG_SUPPORT  (0x00000100)
#define CDROM_HACK_FORCE_READ_CD_DETECTION (0x00000200)
#define CDROM_HACK_READ_CD_SUPPORTED       (0x00000400)

*/

CLASSPNP_SCAN_FOR_SPECIAL_INFO CdromHackItems[] = {
    // digital put out drives using 512 byte block sizes,
    // and needed us to send a mode page to set the sector
    // size back to 2048.
    { "DEC"     , "RRD"                            , NULL,   0x0001 },
    // these fujitsu drives take longer than ten seconds to
    // timeout commands when audio discs are placed in them
    { "FUJITSU" , "FMCD-101"                       , NULL,   0x0002 },
    { "FUJITSU" , "FMCD-102"                       , NULL,   0x0002 },
    // these hitachi drives don't work properly in PIO mode
    { "HITACHI ", "CDR-1750S"                      , NULL,   0x0004 },
    { "HITACHI ", "CDR-3650/1650S"                 , NULL,   0x0004 },
    // this particular gem doesn't automatically spin up
    // on some media access commands.
    { ""        , "HITACHI GD-2000"                , NULL,   0x0008 },
    { ""        , "HITACHI DVD-ROM GD-2000"        , NULL,   0x0008 },
    // this particular drive doesn't support DVD playback.
    // just print an error message in CHK builds.
    { "TOSHIBA ", "SD-W1101 DVD-RAM"               , NULL,   0x0010 },
    // not sure what this device's issue was.  seems to
    // require mode selects at various times.
    { "TOSHIBA ", "CD-ROM XM-3"                    , NULL,   0x0020 },
    // NEC defined a "READ_CD" type command before there was
    // a standard, so fall back on this as an option.
    { "NEC"     , ""                               , NULL,   0x0040 },
    // plextor defined a "READ_CD" type command before there was
    // a standard, so fall back on this as an option.
    { "PLEXTOR ", ""                               , NULL,   0x0080 },
    // this drive times out and sometimes disappears from the bus
    // when send GET_CONFIGURATION commands.  don't send them.
    { ""        , "LG DVD-ROM DRD-840B"            , NULL,   0x0100 },
    { ""        , "SAMSUNG DVD-ROM SD-608"         , NULL,   0x0300 },
    // these drives should have supported READ_CD, but at least
    // some firmware revisions did not.  force READ_CD detection.
    { ""        , "SAMSUNG DVD-ROM SD-"            , NULL,   0x2000 },
    // the mitsumi drive below doesn't follow the block-only spec,
    // and we end up hanging when sending it commands it doesn't
    // understand.  this causes complications later, also.
    { "MITSUMI ", "CR-4802TE       "               , NULL,   0x0100 },
    // some drives return various funky errors (such as 3/2/0 NO_SEEK_COMPLETE)
    // during the detection of READ_CD support, resulting in iffy detection.
    // since they probably don't support mode switching, which is really old
    // legacy stuff anyways, the ability to read digitally is lost when
    // these drives return unexpected error codes.  note: MMC compliant drives
    // are presumed to support READ_CD, as are DVD drives, and anything
    // connected to a bus type other than IDE or SCSI, and therefore don't
    // need to be here.
    { "YAMAHA  ", "CRW8424S        "               , NULL,   0x0400 },
    // and finally, a place to finish the list. :)
    { NULL      , NULL                             , NULL,   0x0000 }
};

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif

