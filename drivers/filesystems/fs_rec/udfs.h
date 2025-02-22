/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS File System Recognizer
 * FILE:             drivers/filesystems/fs_rec/udfs.h
 * PURPOSE:          UDFS Header File
 * PROGRAMMER:       Dmitry Chapyshev (dmitry@reactos.org)
 */

/* Standard Identifier (EMCA 167r2 2/9.1.2) */
#define VSD_STD_ID_NSR02    "NSR02"    /* (3/9.1) */

/* Standard Identifier (ECMA 167r3 2/9.1.2) */
#define VSD_STD_ID_BEA01    "BEA01"    /* (2/9.2) */
#define VSD_STD_ID_BOOT2    "BOOT2"    /* (2/9.4) */
#define VSD_STD_ID_CD001    "CD001"    /* (ECMA-119) */
#define VSD_STD_ID_CDW02    "CDW02"    /* (ECMA-168) */
#define VSD_STD_ID_NSR03    "NSR03"    /* (3/9.1) */
#define VSD_STD_ID_TEA01    "TEA01"    /* (2/9.3) */

/* Volume Structure Descriptor (ECMA 167r3 2/9.1) */
#define VSD_STD_ID_LEN 5
typedef struct _VOLSTRUCTDESC
{
    UCHAR Type;
    UCHAR Ident[VSD_STD_ID_LEN];
    UCHAR Version;
    UCHAR Data[2041];
} VOLSTRUCTDESC, *PVOLSTRUCTDESC;

