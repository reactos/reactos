/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WSPOOL.H
 *  WOW32 printer spooler support routines
 *
 *  These routines help a Win 3.0 task to use the print spooler apis. These
 *  apis were exposed by DDK in Win 3.1.
 *
 *  History:
 *  Created 1-July-1993 by Chandan Chauhan (ChandanC)
 *
--*/

ULONG FASTCALL   WG32OpenJob (PVDMFRAME pFrame);
ULONG FASTCALL   WG32StartSpoolPage (PVDMFRAME pFrame);
ULONG FASTCALL   WG32EndSpoolPage (PVDMFRAME pFrame);
ULONG FASTCALL   WG32CloseJob (PVDMFRAME pFrame);
ULONG FASTCALL   WG32WriteSpool (PVDMFRAME pFrame);
ULONG FASTCALL   WG32DeleteJob (PVDMFRAME pFrame);
ULONG FASTCALL   WG32SpoolFile (PVDMFRAME pFrame);

typedef struct _tagWOWSpool {
    HANDLE hFile;
    HANDLE hPrinter;
    BOOL   fOK;
    WORD   prn16;
} WOWSPOOL;

typedef struct _DLLENTRYPOINTS {
    char    *name;
    ULONG   (*lpfn)();
} DLLENTRYPOINTS;

extern  DLLENTRYPOINTS  spoolerapis[];

#define WOW_SPOOLERAPI_COUNT    15

#define WOW_EXTDEVICEMODE       0
#define WOW_DEVICEMODE          1
#define WOW_DEVICECAPABILITIES  2
#define WOW_OpenPrinterA        3
#define WOW_StartDocPrinterA    4
#define WOW_StartPagePrinter    5
#define WOW_EndPagePrinter      6
#define WOW_EndDocPrinter       7
#define WOW_ClosePrinter        8
#define WOW_WritePrinter        9
#define WOW_DeletePrinter       10
#define WOW_GetPrinterDriverDirectory 11
#define WOW_AddPrinter                12
#define WOW_AddPrinterDriver          13
#define WOW_AddPortEx                 14


WORD    GetPrn16(HANDLE h32);
HANDLE  Prn32(WORD h16);
VOID    FreePrn (WORD h16);

BOOL    GetDriverName (char *psz, char *szDriver);

BOOL    LoadLibraryAndGetProcAddresses(char *name, DLLENTRYPOINTS *p, int i);

#ifdef i386
HINSTANCE SafeLoadLibrary(char *name);
#else
#define SafeLoadLibrary(name) LoadLibrary(name)
#endif
