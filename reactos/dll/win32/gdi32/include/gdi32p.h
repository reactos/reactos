/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/gdi32/include/gdi32p.h
 * PURPOSE:         User-Mode Win32 GDI Library Private Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */


/* DATA **********************************************************************/

extern PGDI_TABLE_ENTRY GdiHandleTable;
extern PGDI_SHARED_HANDLE_TABLE GdiSharedHandleTable;
extern HANDLE hProcessHeap;
extern HANDLE CurrentProcessId;
extern DWORD GDI_BatchLimit;
extern PDEVCAPS GdiDevCaps;

typedef INT
(CALLBACK* EMFPLAYPROC)(
    HDC hdc,
    INT iFunction,
    HANDLE hPageQuery
);

/* DEFINES *******************************************************************/

#define HANDLE_LIST_INC 20

#define METAFILE_MEMORY 1
#define METAFILE_DISK   2

/* MACRO ********************************************************************/
#define ROP_USES_SOURCE(Rop)   ((Rop << 2) ^ Rop) & 0xCC0000

/* TYPES *********************************************************************/

// Based on wmfapi.h and Wine.
typedef struct tagMETAFILEDC {
  PVOID      pvMetaBuffer;
  HANDLE     hFile;
  DWORD      Size;
  METAHEADER mh;
  UINT       handles_size, cur_handles;
  HGDIOBJ   *handles;

  // more DC object stuff.
  HGDIOBJ    Pen;
  HGDIOBJ    Brush;
  HGDIOBJ    Palette;
  HGDIOBJ    Font;

  WCHAR      Filename[MAX_PATH+2];
  // Add more later.
} METAFILEDC,*PMETAFILEDC;

// Metafile Entry handle
typedef struct tagMF_ENTRY {
  LIST_ENTRY   List;
  HGDIOBJ      hmDC;             // Handle return from NtGdiCreateClientObj.
  PMETAFILEDC pmfDC;
} MF_ENTRY, *PMF_ENTRY;

typedef struct tagENHMETAFILE {
  PVOID      pvMetaBuffer;
  HANDLE     hFile;      /* Handle for disk based MetaFile */
  DWORD      Size;
  INT        iType;
  PENHMETAHEADER emf;
  UINT       handles_size, cur_handles;
  HGDIOBJ   *handles;
  INT        horzres, vertres;
  INT        horzsize, vertsize;
  INT        logpixelsx, logpixelsy;
  INT        bitspixel;
  INT        textcaps;
  INT        rastercaps;
  INT        technology;
  INT        planes;
} ENHMETAFILE,*PENHMETAFILE;


#define PDEV_UMPD_ID  0xFEDCBA98
// UMPDEV flags
#define UMPDEV_NO_ESCAPE      0x0002
#define UMPDEV_SUPPORT_ESCAPE 0x0004
typedef struct _UMPDEV
{
  DWORD           Sig;            // Init with PDEV_UMPD_ID
  struct _UMPDEV *pumpdNext;
  PDRIVER_INFO_5W pdi5Info;
  HMODULE         hModule;
  DWORD           dwFlags;
  DWORD           dwDriverAttributes;
  DWORD           dwConfigVersion; // Number of times the configuration
                                   // file for this driver has been upgraded
                                   // or downgraded since the last spooler restart.
  DWORD           dwDriverCount;   // After init should be 2
  DWORD           WOW64_UMPDev;
  DWORD           WOW64_hMod;
  WCHAR           String[188];
} UMPDEV, *PUMPDEV;


/* FUNCTIONS *****************************************************************/

PVOID
HEAP_alloc(DWORD len);

NTSTATUS
HEAP_strdupA2W(
    LPWSTR* ppszW,
    LPCSTR lpszA
);

VOID
HEAP_free(LPVOID memory);

BOOL
FASTCALL
TextMetricW2A(
    TEXTMETRICA *tma,
    TEXTMETRICW *tmw
);

BOOL
FASTCALL
NewTextMetricW2A(
    NEWTEXTMETRICA *tma,
    NEWTEXTMETRICW *tmw
);

BOOL
FASTCALL
NewTextMetricExW2A(
    NEWTEXTMETRICEXA *tma,
    NEWTEXTMETRICEXW *tmw
);

BOOL
FASTCALL
DeleteRegion( HRGN );

BOOL
GdiIsHandleValid(HGDIOBJ hGdiObj);

BOOL
GdiGetHandleUserData(
    HGDIOBJ hGdiObj,
    DWORD ObjectType,
    PVOID *UserData
);

PLDC
GdiGetLDC(HDC hDC);

HGDIOBJ
STDCALL
GdiFixUpHandle(HGDIOBJ hGO);

BOOL
WINAPI
CalculateColorTableSize(
    CONST BITMAPINFOHEADER *BitmapInfoHeader,
    UINT *ColorSpec,
    UINT *ColorTableSize
);

LPBITMAPINFO
WINAPI
ConvertBitmapInfo(
    CONST BITMAPINFO *BitmapInfo,
    UINT ColorSpec,
    UINT *BitmapInfoSize,
    BOOL FollowedByData
);

DEVMODEW *
NTAPI
GdiConvertToDevmodeW(DEVMODEA *dm);

DWORD
STDCALL
GetAndSetDCDWord( HDC, INT, DWORD, DWORD, DWORD, DWORD );

DWORD
STDCALL
GetDCDWord( HDC, INT, DWORD);

HGDIOBJ
STDCALL
GetDCObject( HDC, INT);

VOID
NTAPI
LogFontA2W(
    LPLOGFONTW pW,
    CONST LOGFONTA *pA
);

VOID
NTAPI
LogFontW2A(
    LPLOGFONTA pA,
    CONST LOGFONTW *pW
);

VOID
STDCALL
EnumLogFontExW2A(
    LPENUMLOGFONTEXA fontA,
    CONST ENUMLOGFONTEXW *fontW );

/* FIXME: Put in some public header */
UINT
WINAPI
UserRealizePalette(HDC hDC);

int
STDCALL
GdiAddFontResourceW(LPCWSTR lpszFilename,FLONG fl,DESIGNVECTOR *pdv);

VOID
STDCALL
GdiSetLastError( DWORD dwErrCode );

/* EOF */

