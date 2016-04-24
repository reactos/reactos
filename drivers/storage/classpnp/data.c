/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

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

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGE")
#endif

/*
#define FDO_HACK_CANNOT_LOCK_MEDIA (0x00000001)
#define FDO_HACK_GESN_IS_BAD       (0x00000002)
*/

CLASSPNP_SCAN_FOR_SPECIAL_INFO ClassBadItems[] = {
    { ""        , "MITSUMI CD-ROM FX240"           , NULL,   0x02 },
    { ""        , "MITSUMI CD-ROM FX320"           , NULL,   0x02 },
    { ""        , "MITSUMI CD-ROM FX322"           , NULL,   0x02 },
    { ""        , "COMPAQ CRD-8481B"               , NULL,   0x04 },
    { NULL      , NULL                             , NULL,   0x0  }
};


GUID ClassGuidQueryRegInfoEx = GUID_CLASSPNP_QUERY_REGINFOEX;

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif

