/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WOWSHELL.H
 *  16-bit SHELL API argument structures
 *
 *  History:
 *  Created 14-April-1992 by Chandan S. Chauhan (ChandanC)
 *  Added Win95 exports 17-Oct-1996 Dave Hart (davehart)
 *
--*/

/* SHELL API IDs
 */
#define FUN_REGOPENKEY            1
#define FUN_REGCREATEKEY          2
#define FUN_REGCLOSEKEY           3
#define FUN_REGDELETEKEY          4
#define FUN_REGSETVALUE           5
#define FUN_REGQUERYVALUE         6
#define FUN_REGENUMKEY            7
#define FUN_DRAGACCEPTFILES       9
#define FUN_DRAGQUERYFILE         11
#define FUN_DRAGFINISH            12
#define FUN_DRAGQUERYPOINT        13
#define FUN_SHELLEXECUTE          20
#define FUN_FINDEXECUTABLE        21
#define FUN_SHELLABOUT            22
#define FUN_WCI                   32
#define FUN_ABOUTDLGPROC          33
#define FUN_EXTRACTICON           34
#define FUN_EXTRACTASSOCIATEDICON 36
#define FUN_DOENVIRONMENTSUBST    37
#define FUN_FINDENVIRONMENTSTRING 38
#define FUN_INTERNALEXTRACTICON   39
#define FUN_HERETHARBETYGARS      41  // export 100
#define FUN_FINDEXEDLGPROC        42  // export 101
#define FUN_REGISTERSHELLHOOK     43  // export 102
#define FUN_SHELLHOOKPROC         44  // export 103

/* New for Win95 */

#define FUN_EXTRACTICONEX         40
#define FUN_RESTARTDIALOG         45  // export 157
#define FUN_PICKICONDLG           46  // export 166
#define FUN_DRIVETYPE             47  // export 262
#define FUN_SH16TO32DRIVEIOCTL    48  // export 263
#define FUN_SH16TO32INT2526       49  // export 264
#define FUN_SHGETFILEINFO         50  // export 300
#define FUN_SHFORMATDRIVE         51  // export 400
#define FUN_SHCHECKDRIVE          52  // export 401
#define FUN__RUNDLLCHECKDRIVE     53  // export 402


/* XLATOFF */
#pragma pack(2)
/* XLATON */

typedef struct _REGOPENKEY16 {		/* s1 */
    VPVOID  f3;
    VPVOID  f2;
    DWORD	f1;
} REGOPENKEY16;
typedef REGOPENKEY16 UNALIGNED *PREGOPENKEY16;

typedef struct _REGCREATEKEY16 {	/* s2 */
    VPVOID  f3;
    VPVOID  f2;
    DWORD   f1;
} REGCREATEKEY16;
typedef REGCREATEKEY16 UNALIGNED *PREGCREATEKEY16;

typedef struct _REGCLOSEKEY16 {		/* s3 */
    DWORD   f1;
} REGCLOSEKEY16;
typedef REGCLOSEKEY16 UNALIGNED *PREGCLOSEKEY16;

typedef struct _REGDELETEKEY16 {	/* s4 */
    VPVOID  f2;
    DWORD   f1;
} REGDELETEKEY16;
typedef REGDELETEKEY16 UNALIGNED *PREGDELETEKEY16;

typedef struct _REGSETVALUE16 {		/* s5 */
    DWORD   f5;
    VPVOID  f4;
    DWORD   f3;
    VPVOID  f2;
    DWORD   f1;
} REGSETVALUE16;
typedef REGSETVALUE16 UNALIGNED *PREGSETVALUE16;

typedef struct _REGQUERYVALUE16 {	/* s6 */
    VPVOID  f4;
    VPVOID  f3;
    VPVOID  f2;
    DWORD   f1;
} REGQUERYVALUE16;
typedef REGQUERYVALUE16 UNALIGNED *PREGQUERYVALUE16;

typedef struct _REGENUMKEY16 {		/* s7 */
    DWORD   f4;
    VPVOID  f3;
    DWORD   f2;
    DWORD   f1;
} REGENUMKEY16;
typedef REGENUMKEY16 UNALIGNED *PREGENUMKEY16;

typedef struct _DRAGACCEPTFILES16 {	/* s9 */
    SHORT   f2;
    HAND16  f1;
} DRAGACCEPTFILES16;
typedef DRAGACCEPTFILES16 UNALIGNED *PDRAGACCEPTFILES16;

typedef struct _DRAGQUERYFILE16 {	/* s11 */
    WORD    f4;
    VPVOID  f3;
    WORD    f2;
    HAND16  f1;
} DRAGQUERYFILE16;
typedef DRAGQUERYFILE16 UNALIGNED *PDRAGQUERYFILE16;

typedef struct _DRAGFINISH16 {		/* s12 */
    HAND16  f1;
} DRAGFINISH16;
typedef DRAGFINISH16 UNALIGNED *PDRAGFINISH16;

typedef struct _DRAGQUERYPOINT16 {	/* s13 */
    VPVOID  f2;
    HAND16  f1;
} DRAGQUERYPOINT16;
typedef DRAGQUERYPOINT16 UNALIGNED *PDRAGQUERYPOINT16;

typedef struct _SHELLABOUT16 {	      /* s22 */
    HAND16  f4;
    VPVOID  f3;
    VPVOID  f2;
    HAND16  f1;
} SHELLABOUT16;
typedef SHELLABOUT16 UNALIGNED *PSHELLABOUT16;

typedef struct _SHELLEXECUTE16 {	/* s20 */
    WORD    f6;
    VPVOID  f5;
    VPVOID  f4;
    VPVOID  f3;
    VPVOID  f2;
    HAND16  f1;
} SHELLEXECUTE16;
typedef SHELLEXECUTE16 UNALIGNED *PSHELLEXECUTE16;

typedef struct _FINDEXECUTABLE16 {	/* s21 */
    VPVOID  f3;
    VPVOID  f2;
    VPVOID  f1;
} FINDEXECUTABLE16;
typedef FINDEXECUTABLE16 UNALIGNED *PFINDEXECUTABLE16;

typedef struct _EXTRACTICON16 {		/* s34 */
    WORD    f3;
    VPVOID  f2;
    HAND16  f1;
} EXTRACTICON16;
typedef EXTRACTICON16 UNALIGNED *PEXTRACTICON16;

typedef struct _EXTRACTASSOCIATEDICON16 {	  /* s36 */
    VPVOID  f3;
    VPVOID  f2;
    HAND16  f1;
} EXTRACTASSOCIATEDICON16;
typedef EXTRACTASSOCIATEDICON16 UNALIGNED *PEXTRACTASSOCIATEDICON16;

typedef struct _DOENVIRONMENTSUBST16 {            /* s37 */
    WORD    cch;
    VPVOID  vpsz;
} DOENVIRONMENTSUBST16;
typedef DOENVIRONMENTSUBST16 UNALIGNED *PDOENVIRONMENTSUBST16;


/* New for Win95 */


typedef struct _EXTRACTICONEX16 {            /* s40 */
    SHORT   nIcons;
    VPWORD  phiconSmall;
    VPWORD  phiconLarge;
    SHORT   nIconIndex;
    VPSZ    lpszExeFileName;
} EXTRACTICONEX16;
typedef EXTRACTICONEX16 UNALIGNED *PEXTRACTICONEX16;

typedef struct _RESTARTDIALOG16 {            /* s45 */
    DWORD  dwReturn;
    VPSTR  lpszPrompt;
    HWND16 hwnd;
} RESTARTDIALOG16;
typedef RESTARTDIALOG16 UNALIGNED *PRESTARTDIALOG16;

typedef struct _PICKICONDLG16 {            /* s46 */
    VPSHORT piIconIndex;
    WORD    cbIconPath;
    VPSZ    pszIconPath;
    HWND16  hwnd;
} PICKICONDLG16;
typedef PICKICONDLG16 UNALIGNED *PPICKICONDLG16;

typedef struct _DRIVETYPE16 {            /* s47 */
    SHORT   iDrive;
} DRIVETYPE16;
typedef DRIVETYPE16 UNALIGNED *PDRIVETYPE16;

typedef struct _SH16TO32DRIVEIOCTL16 {            /* s48 */
    VPVOID  pv;
    SHORT   iCmd;
    SHORT   iDrive;
} SH16TO32DRIVEIOCTL16;
typedef SH16TO32DRIVEIOCTL16 UNALIGNED *PSH16TO32DRIVEIOCTL16;

typedef struct _SH16TO32INT252616 {            /* s49 */
    DWORD   dwSector;
    WORD    wCount;
    VPVOID  lpBuf;
    SHORT   iInt;
    SHORT   iDrive;
} SH16TO32INT252616;
typedef SH16TO32INT252616 UNALIGNED *PSH16TO32INT252616;

typedef struct _SHGETFILEINFO16 {            /* s50 */
    WORD    wFlags;
    WORD    cbFileInfo;
    VPVOID  lpsfi;
    DWORD   dwFileAttributes;
    VPSZ    lpszPath;
} SHGETFILEINFO16;
typedef SHGETFILEINFO16 UNALIGNED *PSHGETFILEINFO16;

typedef struct _SHFORMATDRIVE16 {            /* s51 */
    WORD    wOptions;
    WORD    wFmtID;
    WORD    wDrive;
    HWND16  hwnd;
} SHFORMATDRIVE16;
typedef SHFORMATDRIVE16 UNALIGNED *PSHFORMATDRIVE16;

typedef struct _SHCHECKDRIVE16 {            /* s52 */
    VPWORD lpTLhwnd;
    DWORD  dwDrvList;
    DWORD  dwOptions;
    HWND16 hwnd;
} SHCHECKDRIVE16;
typedef SHCHECKDRIVE16 UNALIGNED *PSHCHECKDRIVE16;

typedef struct __RUNDLLCHECKDRIVE16 {            /* s53 */
    SHORT   nCmdShow;
    VPSZ    lpszCmdLine;
    HINST16 hAppInstance;
    HWND16  hwndStub;
} _RUNDLLCHECKDRIVE16;
typedef _RUNDLLCHECKDRIVE16 UNALIGNED *P_RUNDLLCHECKDRIVE16;


/* XLATOFF */
#pragma pack()
/* XLATON */

