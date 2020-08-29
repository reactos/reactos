/*++

Copyright (C) Microsoft Corporation, 1991 - 2010

Module Name:

    disk.c

Abstract:

    SCSI disk class driver

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "classp.h"


/*
 *  Entry in static list used by debug extension to quickly find all class FDOs.
 */
LIST_ENTRY AllFdosList = {&AllFdosList, &AllFdosList};

#ifdef ALLOC_DATA_PRAGMA
    #pragma data_seg("PAGEDATA")
#endif

/*
#define FDO_HACK_CANNOT_LOCK_MEDIA              (0x00000001)
#define FDO_HACK_GESN_IS_BAD                    (0x00000002)
#define FDO_HACK_NO_SYNC_CACHE                  (0x00000004)
#define FDO_HACK_NO_RESERVE6                    (0x00000008)
#define FDO_HACK_GESN_IGNORE_OPCHANGE           (0x00000010)
*/

CLASSPNP_SCAN_FOR_SPECIAL_INFO ClassBadItems[] = {                     // Type (HH, slim) + WHQL Date, if known
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


GUID ClassGuidQueryRegInfoEx = GUID_CLASSPNP_QUERY_REGINFOEX;
GUID ClassGuidSenseInfo2     = GUID_CLASSPNP_SENSEINFO2;
GUID ClassGuidWorkingSet     = GUID_CLASSPNP_WORKING_SET;
GUID ClassGuidSrbSupport     = GUID_CLASSPNP_SRB_SUPPORT;

#ifdef ALLOC_DATA_PRAGMA
    #pragma data_seg()
#endif
