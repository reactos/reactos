/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WOWTH.H
 *  16-bit ToolHelp thunked API argument structures
 *
 *  History:
 *  12-Nov-92 Dave Hart (davehart) created using wowkrn.h as template
--*/


/* ToolHelp API IDs
 */
#define FUN_CLASSFIRST          1
#define FUN_CLASSNEXT           2


/* XLATOFF */
#pragma pack(2)
/* XLATON */


typedef struct _CLASSFIRST16 {      /* th1 */
    VPVOID f1;
} CLASSFIRST16;
typedef CLASSFIRST16 UNALIGNED *PCLASSFIRST16;

typedef struct _CLASSNEXT16 {       /* th2 */
    VPVOID f1;
} CLASSNEXT16;
typedef CLASSNEXT16 UNALIGNED *PCLASSNEXT16;


/* XLATOFF */
#pragma pack()
/* XLATON */
