#pragma once

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
    LIST_ENTRY   List;
    HDC          hDC;
    HWND         hwndCurrent;
    PWND         pwndOrg;
    PWND         pwndClip;
    PWND         pwndRedirect;
    HRGN         hrgnClip;
    HRGN         hrgnClipPublic;
    HRGN         hrgnSavedVis;
    DWORD        DCXFlags;
    PTHREADINFO  ptiOwner;
    PPROCESSINFO ppiOwner;
    struct _MONITOR* pMonitor;
} DCE, *PDCE;

/* internal DCX flags, see psdk/winuser.h for the rest */
#define DCX_DCEEMPTY        0x00000800
#define DCX_DCEBUSY         0x00001000
#define DCX_DCEDIRTY        0x00002000
#define DCX_LAYEREDWIN      0x00004000
#define DCX_DCPOWNED        0x00008000
#define DCX_NOCLIPCHILDREN  0x00080000
#define DCX_NORECOMPUTE     0x00100000
#define DCX_INDESTROY       0x00400000

INIT_FUNCTION NTSTATUS NTAPI InitDCEImpl(VOID);
PDCE FASTCALL DceAllocDCE(PWND Window, DCE_TYPE Type);
HWND FASTCALL IntWindowFromDC(HDC hDc);
void FASTCALL DceFreeDCE(PDCE dce, BOOLEAN Force);
void FASTCALL DceEmptyCache(void);
VOID FASTCALL DceResetActiveDCEs(PWND Window);
void FASTCALL DceFreeClassDCE(HDC);
HWND FASTCALL UserGethWnd(HDC,PWNDOBJ*);
void FASTCALL DceFreeWindowDCE(PWND);
void FASTCALL DceFreeThreadDCE(PTHREADINFO);
VOID FASTCALL DceUpdateVisRgn(DCE *Dce, PWND Window, ULONG Flags);
DCE* FASTCALL DceGetDceFromDC(HDC hdc);
