/*++ BUILD Version: 0001
 *
 *  MVDM v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WOWCLIP.H
 *
 *  History:
 *  09-22-92 Craig Jones (v-cjones)
 *  Created.
--*/

/* XLATOFF */
#pragma pack(2)
/* XLATON */

typedef struct tagMETAFILEPICT16 {    /* mfp16wow32 */
    WORD    mm;
    WORD    xExt;
    WORD    yExt;
    HMEM16  hMF;
} METAFILEPICT16;
typedef METAFILEPICT16 UNALIGNED *LPMETAFILEPICT16;

/* XLATOFF */
#pragma pack()
/* XLATON */
