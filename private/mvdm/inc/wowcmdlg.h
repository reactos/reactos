/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1993, Microsoft Corporation
 *
 *  WOWCMDLG.H
 *  16-bit Commdlg API argument structures
 *
 *  History:
 *      John Vert (jvert) 30-Dec-1992
 *          Created
--*/

//#include <windows.h>
//#include <winsock.h>

/* XLATOFF */
#pragma pack(2)
/* XLATON */

/*++
 *
 * Commdlg data structures
 *
--*/

typedef struct _FINDREPLACE16 {                         /* fr16 */
    DWORD   lStructSize;
    HWND16  hwndOwner;
    HAND16  hInstance;
    DWORD   Flags;
    VPSZ    lpstrFindWhat;
    VPSZ    lpstrReplaceWith;
    WORD    wFindWhatLen;
    WORD    wReplaceWithLen;
    LONG    lCustData;
    VPPROC  lpfnHook;
    VPCSTR  lpTemplateName;
} FINDREPLACE16;
typedef FINDREPLACE16 UNALIGNED *PFINDREPLACE16;
typedef VPVOID VPFINDREPLACE;

typedef struct _OPENFILENAME16 {                        /* of16 */
    DWORD   lStructSize;
    HWND16  hwndOwner;
    HAND16  hInstance;
    VPCSTR  lpstrFilter;
    VPSZ    lpstrCustomFilter;
    DWORD   nMaxCustFilter;
    DWORD   nFilterIndex;
    VPSZ    lpstrFile;
    DWORD   nMaxFile;
    VPSZ    lpstrFileTitle;
    DWORD   nMaxFileTitle;
    VPCSTR  lpstrInitialDir;
    VPCSTR  lpstrTitle;
    DWORD   Flags;
    USHORT  nFileOffset;
    USHORT  nFileExtension;
    VPCSTR  lpstrDefExt;
    LONG    lCustData;
    VPPROC  lpfnHook;
    VPCSTR  lpTemplateName;
} OPENFILENAME16;
typedef OPENFILENAME16 UNALIGNED *POPENFILENAME16;
typedef VPVOID VPOPENFILENAME;

typedef struct _CHOOSECOLORDATA16 {                     /* cc16 */
    DWORD   lStructSize;
    HWND16  hwndOwner;
    HAND16  hInstance;
    DWORD   rgbResult;
    VPDWORD lpCustColors;
    DWORD   Flags;
    LONG    lCustData;
    VPPROC  lpfnHook;
    VPCSTR  lpTemplateName;
} CHOOSECOLORDATA16;
typedef CHOOSECOLORDATA16 UNALIGNED *PCHOOSECOLORDATA16;
typedef VPVOID VPCHOOSECOLORDATA;

typedef struct _CHOOSEFONTDATA16 {                      /* cf16 */
    DWORD   lStructSize;
    HWND16  hwndOwner;
    HDC16   hDC;
    VPLOGFONT16 lpLogFont;
    SHORT   iPointSize;
    DWORD   Flags;
    DWORD   rgbColors;
    LONG    lCustData;
    VPPROC  lpfnHook;
    VPCSTR  lpTemplateName;
    HAND16  hInstance;
    VPSZ    lpszStyle;
    WORD    nFontType;
    SHORT   nSizeMin;
    SHORT   nSizeMax;
} CHOOSEFONTDATA16;
typedef CHOOSEFONTDATA16 UNALIGNED *PCHOOSEFONTDATA16;
typedef VPVOID VPCHOOSEFONTDATA;

typedef struct _PRINTDLGDATA16 {                        /* pd16 */
    DWORD   lStructSize;
    HWND16  hwndOwner;
    HAND16  hDevMode;
    HAND16  hDevNames;
    HDC16   hDC;
    DWORD   Flags;
    WORD    nFromPage;
    WORD    nToPage;
    WORD    nMinPage;
    WORD    nMaxPage;
    WORD    nCopies;
    HAND16  hInstance;
    LONG    lCustData;
    VPPROC  lpfnPrintHook;
    VPPROC  lpfnSetupHook;
    VPCSTR  lpPrintTemplateName;
    VPCSTR  lpSetupTemplateName;
    HAND16  hPrintTemplate;
    HAND16  hSetupTemplate;
} PRINTDLGDATA16;
typedef PRINTDLGDATA16 UNALIGNED *PPRINTDLGDATA16;
typedef VPVOID VPPRINTDLGDATA;

typedef struct _DEVNAMES16 {                            /* dn16 */
    WORD    wDriverOffset;
    WORD    wDeviceOffset;
    WORD    wOutputOffset;
    WORD    wDefault;
} DEVNAMES16;
typedef DEVNAMES16 UNALIGNED *PDEVNAMES16;
typedef VPVOID VPDEVNAMES;

/*++
 *
 * Commdlg API IDs (equal to ordinal numbers)
 *
--*/
#define FUN_GETOPENFILENAME         1
#define FUN_GETSAVEFILENAME         2
#define FUN_CHOOSECOLOR             5
#define FUN_FINDTEXT                11
#define FUN_REPLACETEXT             12
#define FUN_CHOOSEFONT              15
#define FUN_PRINTDLG                20
#define FUN_WOWCOMMDLGEXTENDEDERROR 26
#define FUN_GETFILETITLE            27


/*++

  Commdlg function prototypes - the seemingly unimportant number in the
  comment on each function MUST match the ones in the list above!!!

  !! BE WARNED !!

--*/

typedef struct _GETOPENFILENAME16 {                 /* cd1  */
    VPOPENFILENAME lpof;
} GETOPENFILENAME16;
typedef GETOPENFILENAME16 UNALIGNED *PGETOPENFILENAME16;

typedef struct _GETSAVEFILENAME16 {                 /* cd2  */
    VPOPENFILENAME lpcf;
} GETSAVEFILENAME16;
typedef GETSAVEFILENAME16 UNALIGNED *PGETSAVEFILENAME16;

typedef struct _CHOOSECOLOR16 {                     /* cd5  */
    VPCHOOSECOLORDATA lpcc;
} CHOOSECOLOR16;
typedef CHOOSECOLOR16 UNALIGNED *PCHOOSECOLOR16;

typedef struct _FINDTEXT16 {                        /* cd11 */
    VPFINDREPLACE lpfr;
} FINDTEXT16;
typedef FINDTEXT16 UNALIGNED *PFINDTEXT16;

typedef struct _REPLACETEXT16 {                       /* cd12 */
    VPFINDREPLACE lpfr;
} REPLACETEXT16;
typedef REPLACETEXT16 UNALIGNED *PREPLACETEXT16;

typedef struct _CHOOSEFONT16 {                      /* cd15 */
    VPCHOOSEFONTDATA lpcf;
} CHOOSEFONT16;
typedef CHOOSEFONT16 UNALIGNED *PCHOOSEFONT16;

typedef struct _PRINTDLG16 {                          /* cd20 */
    VPPRINTDLGDATA lppd;
} PRINTDLG16;
typedef PRINTDLG16 UNALIGNED *PPRINTDLG16;

/* XLATOFF */
#pragma pack()
/* XLATON */
