/*++

Copyright (C) Microsoft Corporation. All rights reserved.

Module Name:


Abstract:


Environment:


Notes:

Revision History:

--*/

#include "ntddk.h"
#include "cdrom.h"


#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#endif

/*

#define CDROM_HACK_DEC_RRD                 (0x00000001)
#define CDROM_HACK_FUJITSU_FMCD_10x        (0x00000002)
    #define CDROM_HACK_HITACHI_1750            (0x00000004) - obsolete.
#define CDROM_HACK_HITACHI_GD_2000         (0x00000008)
#define CDROM_HACK_TOSHIBA_SD_W1101        (0x00000010)
    #define CDROM_HACK_TOSHIBA_XM_3xx          (0x00000020) - obsolete.
#define CDROM_HACK_NEC_CDDA                (0x00000040)
#define CDROM_HACK_PLEXTOR_CDDA            (0x00000080)
#define CDROM_HACK_BAD_GET_CONFIG_SUPPORT  (0x00000100)
#define CDROM_HACK_FORCE_READ_CD_DETECTION (0x00000200)
#define CDROM_HACK_READ_CD_SUPPORTED       (0x00000400)

*/

CDROM_SCAN_FOR_SPECIAL_INFO CdromHackItems[] = {
    // digital put out drives using 512 byte block sizes,
    // and needed us to send a mode page to set the sector
    // size back to 2048.
    { "DEC"     , "RRD"                            , NULL,   0x0001 },
    // these fujitsu drives take longer than ten seconds to
    // timeout commands when audio discs are placed in them
    { "FUJITSU" , "FMCD-101"                       , NULL,   0x0002 },
    { "FUJITSU" , "FMCD-102"                       , NULL,   0x0002 },
    // these hitachi drives don't work properly in PIO mode
    //{ "HITACHI ", "CDR-1750S"                      , NULL,   0x0004 },
    //{ "HITACHI ", "CDR-3650/1650S"                 , NULL,   0x0004 },
    // this particular gem doesn't automatcially spin up
    // on some media access commands.
    { ""        , "HITACHI GD-2000"                , NULL,   0x0008 },
    { ""        , "HITACHI DVD-ROM GD-2000"        , NULL,   0x0008 },
    // this particular drive doesn't support DVD playback.
    // just print an error message in CHK builds.
    { "TOSHIBA ", "SD-W1101 DVD-RAM"               , NULL,   0x0010 },
    // not sure what this device's issue was.  seems to
    // require mode selects at various times.
    //{ "TOSHIBA ", "CD-ROM XM-3"                    , NULL,   0x0020 },
    // NEC defined a "READ_CD" type command before there was
    // a standard, so fall back on this as an option.
    { "NEC"     , NULL                             , NULL,   0x0040 },
    // plextor defined a "READ_CD" type command before there was
    // a standard, so fall back on this as an option.
    { "PLEXTOR ", NULL                             , NULL,   0x0080 },
    // this drive times out and sometimes disappears from the bus
    // when send GET_CONFIGURATION commands.  don't send them.
    { ""        , "LG DVD-ROM DRD-840B"            , NULL,   0x0100 },
    { ""        , "SAMSUNG DVD-ROM SD-608"         , NULL,   0x0300 },
    // these drives should have supported READ_CD, but at least
    // some firmware revisions did not.  force READ_CD detection.
    { ""        , "SAMSUNG DVD-ROM SD-"            , NULL,   0x0200 },
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
    // Polling frequently on virtual optical devices created by Hyper-V will
    // cause a significant perf / power hit. These devices need to be polled
    // less frequently for device state changes.
    { "MSFT    ", NULL                             , NULL,   0x2000 },
    // and finally, a place to finish the list. :)
    { NULL      , NULL                             , NULL,   0x0000 }
};

CDROM_SCAN_FOR_SPECIAL_INFO CdRomBadItems[] = {                     // Type (HH, slim) + WHQL Date, if known
    { ""        , "MITSUMI CD-ROM FX240"           , NULL  ,   0x02 },
    { ""        , "MITSUMI CD-ROM FX320"           , NULL  ,   0x02 },
    { ""        , "MITSUMI CD-ROM FX322"           , NULL  ,   0x02 },
    { ""        , "TEAC DV-28E-A"                  , "2.0A",   0x02 },
    { ""        , "HP CD-Writer cd16h"             , "Q000",   0x02 },
    { ""        , "_NEC NR-7800A"                  , "1.33",   0x02 },
    { ""        , "COMPAQ CRD-8481B"               , NULL  ,   0x04 },
    // The following is a list of device that report too many OpChange/Add events.
    // They require ignoring (or not sending) the OpChange flag in the GESN command.
    // This list contains vendor ID and product ID as separate strings for USB/1394 interface.
    { "HL-DT-ST", "DVDRAM GMA-4020B"               , NULL  ,   0x10 }, // hh  , 2002/04/22
    { "HL-DT-ST", "DVD-RW GCA-4020B"               , NULL  ,   0x10 }, // hh  , 2002/05/14
    { "HL-DT-ST", "DVDRAM GSA-4040B"               , NULL  ,   0x10 }, // hh  , 2003/05/06
    { "HL-DT-ST", "DVDRAM GMA-4040B"               , NULL  ,   0x10 }, // hh  , 2003/07/27
    { "HL-DT-ST", "DVD-RW GWA-4040B"               , NULL  ,   0x10 }, // hh  , 2003/11/18
    { "HL-DT-ST", "DVDRAM GSA-4081B"               , NULL  ,   0x10 }, // hh  , 2003/11/06
    { "HL-DT-ST", "DVDRAM GSA-4082B"               , NULL  ,   0x10 }, // hh  , 2004/01/27
    { "HL-DT-ST", "DVD-RW GWA-4082B"               , NULL  ,   0x10 }, // hh  , 2004/03/11
    { "HL-DT-ST", "DVDRAM GSA-4120B"               , NULL  ,   0x10 }, // hh  , 2004/05/16
    { "HL-DT-ST", "DVD+RW GRA-4120B"               , NULL  ,   0x10 }, // hh  , 2004/04/28
    { "HL-DT-ST", "DVDRAM GSA-4160B"               , NULL  ,   0x10 }, // hh  , 2004/08/12
    { "HL-DT-ST", "DVD-RW GWA-4160B"               , NULL  ,   0x10 }, // hh  , 2004/08/24
    { "HL-DT-ST", "DVDRAM GSA-4163B"               , NULL  ,   0x10 }, // hh  , 2004/11/09
    { "HL-DT-ST", "DVD-RW GWA-4163B"               , NULL  ,   0x10 }, // hh  , 2004/12/29
    { "HL-DT-ST", "DVDRAM GSA-4165B"               , NULL  ,   0x10 }, // hh  , 2005/06/09
    { "HL-DT-ST", "DVDRAM_GSA-4165B"               , NULL  ,   0x10 }, // hh  , 2005/06/28
    { "HL-DT-ST", "DVD-RW GWA-4165B"               , NULL  ,   0x10 }, // hh  , 2005/08/23
    { "HL-DT-ST", "DVDRAM GSA-4167B"               , NULL  ,   0x10 }, // hh  , 2005/07/01
    { "HL-DT-ST", "DVDRAM GSA-H10N"                , NULL  ,   0x10 }, // hh  , 2006/02/16
    { "HL-DT-ST", "DVDRAM_GSA-H10N"                , NULL  ,   0x10 }, // hh  , 2006/02/16
    { "HL-DT-ST", "DVDRAM GSA-H10L"                , NULL  ,   0x10 }, // hh  , 2006/02/27
    { "HL-DT-ST", "DVDRAM_GSA-H10L"                , NULL  ,   0x10 }, // hh  , 2006/04/21
    { "HL-DT-ST", "DVDRAM GSA-H10A"                , NULL  ,   0x10 }, // hh  , 2006/01/03
    { "HL-DT-ST", "DVDRAM_GSA-H10A"                , NULL  ,   0x10 }, // hh  , 2006/05/14
    { "HL-DT-ST", "DVD-RW GSA-H11N"                , NULL  ,   0x10 }, // hh  , 2006/04/28
    { "HL-DT-ST", "DVD-RW_GSA-H11N"                , NULL  ,   0x10 }, // hh  , 2006/02/22

    { "HL-DT-ST", "DVDRAM GSA-4080N"               , NULL  ,   0x10 }, // slim, 2004/08/08
    { "HL-DT-ST", "DVDRAM GMA-4080N"               , NULL  ,   0x10 }, // slim, 2004/11/09
    { "HL-DT-ST", "DVD-RW GCA-4080N"               , NULL  ,   0x10 }, // slim, 2004/11/22
    { "HL-DT-ST", "DVD-RW GWA-4080N"               , NULL  ,   0x10 }, // slim, 2004/08/17
    { "HL-DT-ST", "DVDRAM GSA-4082N"               , NULL  ,   0x10 }, // slim, 2005/07/12
    { "HL-DT-ST", "DVDRAM_GSA-4082N"               , NULL  ,   0x10 }, // slim, 2005/09/21
    { "HL-DT-ST", "DVDRAM GMA-4082N"               , NULL  ,   0x10 }, // slim, 2005/10/20
    { "HL-DT-ST", "DVD-RW GRA-4082N"               , NULL  ,   0x10 }, // slim, 2006/06/07
    { "HL-DT-ST", "DVD-RW GWA-4082N"               , NULL  ,   0x10 }, // slim, 2005/05/24
    { "HL-DT-ST", "DVDRAM GMA4082Nf"               , NULL  ,   0x10 }, // slim, 2006/02/28
    { "HL-DT-ST", "DVDRAM GMA4082Nj"               , NULL  ,   0x10 }, // slim, 2006/01/26

    { "HL-DT-ST", "DVDRAM GSA-4084N"               , NULL  ,   0x10 }, // slim, 2005/12/21
    { "HL-DT-ST", "DVDRAM GMA-4084N"               , NULL  ,   0x10 }, // slim, 2006/02/15
    { "HP"      , "DVD Writer 550s"                , NULL  ,   0x10 }, // slim, 2006/05/08
    { "HL-DT-ST", "DVDRAM GSA-T10N"                , NULL  ,   0x10 }, // slim, 2006/07/26
    { "HL-DT-ST", "DVDRAM_GSA-T10N"                , NULL  ,   0x10 }, // slim, 2006/07/26
    { "HL-DT-ST", "DVD+-RW GSA-T11N"               , NULL  ,   0x10 }, // slim, 2006/07/25

    { "HL-DT-ST", "DVD-ROM GDR8160B"               , NULL  ,   0x10 }, // hh  , 2001/10/12
    { "COMPAQ"  , "DVD-ROM GDR8160B"               , NULL  ,   0x10 }, // hh  , 2001/11/08
    { "HL-DT-ST", "DVD-ROM GDR8161B"               , NULL  ,   0x10 }, // hh  , 2002/07/19
    { "HL-DT-ST", "DVD-ROM GDR8162B"               , NULL  ,   0x10 }, // hh  , 2003/04/22
    { "HL-DT-ST", "DVD-ROM GDR8163B"               , NULL  ,   0x10 }, // hh  , 2004/05/19
    { "HL-DT-ST", "DVD-ROM GDR8164B"               , NULL  ,   0x10 }, // hh  , 2005/06/29
    { "HL-DT-ST", "DVD-ROM GDRH10N"                , NULL  ,   0x10 }, // hh  , 2006/03/07

    { "HL-DT-ST", "DVD-ROM GDR8081N"               , NULL  ,   0x10 }, // slim, 2001/08/27
    { "HL-DT-ST", "DVD-ROM GDR8082N"               , NULL  ,   0x10 }, // slim, 2003/02/02
    { "HL-DT-ST", "DVD-ROM GDR8083N"               , NULL  ,   0x10 }, // slim, 2003/02/02
    { "HL-DT-ST", "DVD-ROM GDR8085N"               , NULL  ,   0x10 }, // slim, 2005/11/10

    { "HL-DT-ST", "RW/DVD GCC-4080N"               , NULL  ,   0x10 }, // slim, 2001/08/21
    { "HL-DT-ST", "RW/DVD_GCC-4080N"               , NULL  ,   0x10 }, // slim,
    { "HL-DT-ST", "RW/DVD GCC-4160N"               , NULL  ,   0x10 }, // slim, 2002/04/08
    { "HL-DT-ST", "RW/DVD GCC-4240N"               , NULL  ,   0x10 }, // slim, 2002/04/26
    { "HL-DT-ST", "RW/DVD GCC-4241N"               , NULL  ,   0x10 }, // slim, 2003/04/07
    { "HL-DT-ST", "RW/DVD_GCC-4241N"               , NULL  ,   0x10 }, // slim, 2004/03/07
    { "HL-DT-ST", "RW/DVD GCC-4242N"               , NULL  ,   0x10 }, // slim, 2003/12/21
    { "HL-DT-ST", "RW/DVD GCC-4246N"               , NULL  ,   0x10 }, // slim, 2005/05/23
    { "HL-DT-ST", "BD-RE  GBW-H10N"                , NULL  ,   0x10 }, // hh  , 2006/06/27

    { "HL-DT-ST", "DVDRAM GSA-4083N"               , NULL  ,   0x10 }, // hh  , 2006/05/17
    { "HL-DT-ST", "DVD+-RW GWA4083N"               , NULL  ,   0x10 }, // hh  , 2006/06/05

    { "PIONEER",  "DVD-RW  DVR-106D"               , NULL  ,   0x10 }, // hh  , ?
    { "ASUS",     "DVD-RW DRW-0402P"               , NULL  ,   0x10 }, // hh  , ?

    //
    // This list contains devices that claims to support asynchronous notification, but
    // doesn't handle it well (e.g., some TSST devices will not report media removal if
    // the GESN command is sent down immediately after the AN interrupt, they need some
    // time in between to be able to correctly report media removal).
    //

    { "TSSTcorp", "CDDVDW SN-S083A"                , "SB00",   0x40 }, // slim, ?

    //
    // This list contains vendor ID and product ID as a single string for ATAPI interface.
    //

    { "", "HL-DT-ST DVDRAM GMA-4020B"              , NULL  ,   0x10 }, // hh  , 2002/04/22
    { "", "HL-DT-ST DVD-RW GCA-4020B"              , NULL  ,   0x10 }, // hh  , 2002/05/14
    { "", "HL-DT-ST DVDRAM GSA-4040B"              , NULL  ,   0x10 }, // hh  , 2003/05/06
    { "", "HL-DT-ST DVDRAM GMA-4040B"              , NULL  ,   0x10 }, // hh  , 2003/07/27
    { "", "HL-DT-ST DVD-RW GWA-4040B"              , NULL  ,   0x10 }, // hh  , 2003/11/18
    { "", "HL-DT-ST DVDRAM GSA-4081B"              , NULL  ,   0x10 }, // hh  , 2003/11/06
    { "", "HL-DT-ST DVDRAM GSA-4082B"              , NULL  ,   0x10 }, // hh  , 2004/01/27
    { "", "HL-DT-ST DVD-RW GWA-4082B"              , NULL  ,   0x10 }, // hh  , 2004/03/11
    { "", "HL-DT-ST DVDRAM GSA-4120B"              , NULL  ,   0x10 }, // hh  , 2004/05/16
    { "", "HL-DT-ST DVD+RW GRA-4120B"              , NULL  ,   0x10 }, // hh  , 2004/04/28
    { "", "HL-DT-ST DVDRAM GSA-4160B"              , NULL  ,   0x10 }, // hh  , 2004/08/12
    { "", "HL-DT-ST DVD-RW GWA-4160B"              , NULL  ,   0x10 }, // hh  , 2004/08/24
    { "", "HL-DT-ST DVDRAM GSA-4163B"              , NULL  ,   0x10 }, // hh  , 2004/11/09
    { "", "HL-DT-ST DVD-RW GWA-4163B"              , NULL  ,   0x10 }, // hh  , 2004/12/29
    { "", "HL-DT-ST DVDRAM GSA-4165B"              , NULL  ,   0x10 }, // hh  , 2005/06/09
    { "", "HL-DT-ST DVDRAM_GSA-4165B"              , NULL  ,   0x10 }, // hh  , 2005/06/28
    { "", "HL-DT-ST DVD-RW GWA-4165B"              , NULL  ,   0x10 }, // hh  , 2005/08/23
    { "", "HL-DT-ST DVDRAM GSA-4167B"              , NULL  ,   0x10 }, // hh  , 2005/07/01
    { "", "HL-DT-ST DVDRAM GSA-H10N"               , NULL  ,   0x10 }, // hh  , 2006/02/16
    { "", "HL-DT-ST DVDRAM_GSA-H10N"               , NULL  ,   0x10 }, // hh  , 2006/02/16
    { "", "HL-DT-ST DVDRAM GSA-H10L"               , NULL  ,   0x10 }, // hh  , 2006/02/27
    { "", "HL-DT-ST DVDRAM_GSA-H10L"               , NULL  ,   0x10 }, // hh  , 2006/04/21
    { "", "HL-DT-ST DVDRAM GSA-H10A"               , NULL  ,   0x10 }, // hh  , 2006/01/03
    { "", "HL-DT-ST DVDRAM_GSA-H10A"               , NULL  ,   0x10 }, // hh  , 2006/05/14
    { "", "HL-DT-ST DVD-RW GSA-H11N"               , NULL  ,   0x10 }, // hh  , 2006/04/28
    { "", "HL-DT-ST DVD-RW_GSA-H11N"               , NULL  ,   0x10 }, // hh  , 2006/02/22

    { "", "HL-DT-ST DVDRAM GSA-4080N"              , NULL  ,   0x10 }, // slim, 2004/08/08
    { "", "HL-DT-ST DVDRAM GMA-4080N"              , NULL  ,   0x10 }, // slim, 2004/11/09
    { "", "HL-DT-ST DVD-RW GCA-4080N"              , NULL  ,   0x10 }, // slim, 2004/11/22
    { "", "HL-DT-ST DVD-RW GWA-4080N"              , NULL  ,   0x10 }, // slim, 2004/08/17
    { "", "HL-DT-ST DVDRAM GSA-4082N"              , NULL  ,   0x10 }, // slim, 2005/07/12
    { "", "HL-DT-ST DVDRAM_GSA-4082N"              , NULL  ,   0x10 }, // slim, 2005/09/21
    { "", "HL-DT-ST DVDRAM GMA-4082N"              , NULL  ,   0x10 }, // slim, 2005/10/20
    { "", "HL-DT-ST DVD-RW GRA-4082N"              , NULL  ,   0x10 }, // slim, 2006/06/07
    { "", "HL-DT-ST DVD-RW GWA-4082N"              , NULL  ,   0x10 }, // slim, 2005/05/24
    { "", "HL-DT-ST DVDRAM GMA4082Nf"              , NULL  ,   0x10 }, // slim, 2006/02/28
    { "", "HL-DT-ST DVDRAM GMA4082Nj"              , NULL  ,   0x10 }, // slim, 2006/01/26

    { "", "HL-DT-ST DVDRAM GSA-4084N"              , NULL  ,   0x10 }, // slim, 2005/12/21
    { "", "HL-DT-ST DVDRAM GMA-4084N"              , NULL  ,   0x10 }, // slim, 2006/02/15
    { "", "HP DVD Writer 550s"                     , NULL  ,   0x10 }, // slim, 2006/05/08
    { "", "HL-DT-ST DVDRAM GSA-T10N"               , NULL  ,   0x10 }, // slim, 2006/07/26
    { "", "HL-DT-ST DVDRAM_GSA-T10N"               , NULL  ,   0x10 }, // slim, 2006/07/26
    { "", "HL-DT-ST DVD+-RW GSA-T11N"              , NULL  ,   0x10 }, // slim, 2006/07/25

    { "", "HL-DT-ST DVD-ROM GDR8160B"              , NULL  ,   0x10 }, // hh  , 2001/10/12
    { "", "COMPAQ DVD-ROM GDR8160B"                , NULL  ,   0x10 }, // hh  , 2001/11/08
    { "", "HL-DT-ST DVD-ROM GDR8161B"              , NULL  ,   0x10 }, // hh  , 2002/07/19
    { "", "HL-DT-ST DVD-ROM GDR8162B"              , NULL  ,   0x10 }, // hh  , 2003/04/22
    { "", "HL-DT-ST DVD-ROM GDR8163B"              , NULL  ,   0x10 }, // hh  , 2004/05/19
    { "", "HL-DT-ST DVD-ROM GDR8164B"              , NULL  ,   0x10 }, // hh  , 2005/06/29
    { "", "HL-DT-ST DVD-ROM GDRH10N"               , NULL  ,   0x10 }, // hh  , 2006/03/07

    { "", "HL-DT-ST DVD-ROM GDR8081N"              , NULL  ,   0x10 }, // slim, 2001/08/27
    { "", "HL-DT-ST DVD-ROM GDR8082N"              , NULL  ,   0x10 }, // slim, 2003/02/02
    { "", "HL-DT-ST DVD-ROM GDR8083N"              , NULL  ,   0x10 }, // slim, 2003/02/02
    { "", "HL-DT-ST DVD-ROM GDR8085N"              , NULL  ,   0x10 }, // slim, 2005/11/10

    { "", "HL-DT-ST RW/DVD GCC-4080N"              , NULL  ,   0x10 }, // slim, 2001/08/21
    { "", "HL-DT-ST RW/DVD_GCC-4080N"              , NULL  ,   0x10 }, // slim,
    { "", "HL-DT-ST RW/DVD GCC-4160N"              , NULL  ,   0x10 }, // slim, 2002/04/08
    { "", "HL-DT-ST RW/DVD GCC-4240N"              , NULL  ,   0x10 }, // slim, 2002/04/26
    { "", "HL-DT-ST RW/DVD GCC-4241N"              , NULL  ,   0x10 }, // slim, 2003/04/07
    { "", "HL-DT-ST RW/DVD_GCC-4241N"              , NULL  ,   0x10 }, // slim, 2004/03/07
    { "", "HL-DT-ST RW/DVD GCC-4242N"              , NULL  ,   0x10 }, // slim, 2003/12/21
    { "", "HL-DT-ST RW/DVD GCC-4246N"              , NULL  ,   0x10 }, // slim, 2005/05/23
    { "", "HL-DT-ST BD-RE  GBW-H10N"               , NULL  ,   0x10 }, // hh  , 2006/06/27

    { "", "HL-DT-ST DVDRAM GSA-4083N"              , NULL  ,   0x10 }, // hh  , 2006/05/17
    { "", "HL-DT-ST DVD+-RW GWA4083N"              , NULL  ,   0x10 }, // hh  , 2006/06/05

    { "", "PIONEER DVD-RW  DVR-106D"               , NULL  ,   0x10 }, // hh  , ?
    { "", "ASUS DVD-RW DRW-0402P"                  , NULL  ,   0x10 }, // hh  , ?


    // Sony sourced some drives from LG also....

    { NULL      , NULL                             , NULL  ,   0x00 },
};


#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif
