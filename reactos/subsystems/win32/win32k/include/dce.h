#ifndef _WIN32K_DCE_H
#define _WIN32K_DCE_H

/* Ported from WINE by Jason Filby */

typedef struct tagDCE *PDCE;

#include <include/window.h>

typedef HANDLE HDCE;

/* DC hook codes */
#define DCHC_INVALIDVISRGN      0x0001
#define DCHC_DELETEDC           0x0002

#define DCHF_INVALIDATEVISRGN   0x0001
#define DCHF_VALIDATEVISRGN     0x0002

typedef enum
{
    DCE_CACHE_DC,   /* This is a cached DC (allocated by USER) */
    DCE_CLASS_DC,   /* This is a class DC (style CS_CLASSDC) */
    DCE_WINDOW_DC   /* This is a window DC (style CS_OWNDC) */
} DCE_TYPE, *PDCE_TYPE;

typedef struct tagDCE
{
    struct tagDCE *next;
    HDC          hDC;
    HWND         hwndCurrent;
    HWND         hwndDC;
    HRGN         hClipRgn;
    DCE_TYPE     type;
    DWORD        DCXFlags;
    PEPROCESS    pProcess;
    HANDLE       Self;
} DCE;  /* PDCE already declared at top of file */

/* internal DCX flags, see psdk/winuser.h for the rest */
#define DCX_EXCLUDEUPDATE	0x00000100
#define DCX_DCEEMPTY		0x00000800
#define DCX_DCEBUSY		0x00001000
#define DCX_DCEDIRTY		0x00002000
#define DCX_USESTYLE		0x00010000
#define DCX_KEEPCLIPRGN		0x00040000
#define DCX_NOCLIPCHILDREN	0x00080000
#define DCX_NORECOMPUTE		0x00100000
  
BOOL FASTCALL DCE_Cleanup(PDCE pDce);
PDCE FASTCALL DceAllocDCE(PWINDOW_OBJECT Window, DCE_TYPE Type);
PDCE FASTCALL DCE_FreeDCE(PDCE dce);
VOID FASTCALL DCE_FreeWindowDCE(HWND);
INT  FASTCALL DCE_ExcludeRgn(HDC, HWND, HRGN);
BOOL FASTCALL DCE_InvalidateDCE(HWND, const PRECTL);
HWND FASTCALL IntWindowFromDC(HDC hDc);
PDCE FASTCALL DceFreeDCE(PDCE dce, BOOLEAN Force);
void FASTCALL DceFreeWindowDCE(PWINDOW_OBJECT Window);
void FASTCALL DceEmptyCache(void);
VOID FASTCALL DceResetActiveDCEs(PWINDOW_OBJECT Window);

#endif /* _WIN32K_DCE_H */
