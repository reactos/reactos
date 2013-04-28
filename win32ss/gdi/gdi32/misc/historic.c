/*
 * reactos/lib/gdi32/misc/historic.c
 *
 * GDI32.DLL Stubs
 *
 * Apis that do basically nothing, but are here for backwards compatibility with older Windows
 *
 */

#include <precomp.h>

/*
 * @implemented
 */
BOOL
WINAPI
EngQueryEMFInfo(HDEV hdev,
                EMFINFO *pEMFInfo)
{
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GdiPlayDCScript(DWORD a0,
                DWORD a1,
                DWORD a2,
                DWORD a3,
                DWORD a4,
                DWORD a5)
{
    /* FIXME fix the prototype right */
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GdiPlayJournal(DWORD a0,
               DWORD a1,
               DWORD a2,
               DWORD a3,
               DWORD a4)
{
    /* FIXME fix the prototype right */
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GdiPlayScript(DWORD a0,
              DWORD a1,
              DWORD a2,
              DWORD a3,
              DWORD a4,
              DWORD a5,
              DWORD a6)
{
    /* FIXME fix the prototype right */
    return FALSE;
}

/*
 * @implemented
 */
HBITMAP
WINAPI
GdiConvertBitmap(HBITMAP hbm)
{
    /* Note Windows 2000/XP/VISTA always returns hbm */
    return hbm;
}

/*
 * @implemented
 */
HBRUSH
WINAPI
GdiConvertBrush(HBRUSH hbr)
{
    /* Note Windows 2000/XP/VISTA always returns hbr */
    return hbr;
}

/*
 * @implemented
 */
HDC
WINAPI
GdiConvertDC(HDC hdc)
{
    /* Note Windows 2000/XP/VISTA always returns hdc */
    return hdc;
}

/*
 * @implemented
 */
HFONT
WINAPI
GdiConvertFont(HFONT hfont)
{
    /* Note Windows 2000/XP/VISTA always returns hfont */
    return hfont;
}

/*
 * @implemented
 */
HPALETTE
WINAPI
GdiConvertPalette(HPALETTE hpal)
{
    /* Note Windows 2000/XP/VISTA always returns hpal */
    return hpal;
}

/*
 * @implemented
 */
HRGN
WINAPI
GdiConvertRegion(HRGN hregion)
{
    /* Note Windows 2000/XP/VISTA always returns hregion */
    return hregion;
}

/*
 * @implemented
 */
BOOL
WINAPI
GdiSetAttrs(HDC hdc)
{
    /* Note Windows 2000/XP/VISTA always returns TRUE */
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GdiDeleteLocalDC(HDC hdc)
{
    /* Note Windows 2000/XP/VISTA always returns TRUE */
    return TRUE;
}


/*
 * @implemented
 */
VOID
WINAPI
GdiSetServerAttr(HDC hdc,DWORD attr)
{
    /* it does do nothing */
}


/*
 * @implemented
 */
int
WINAPI
DeviceCapabilitiesExA(LPCSTR pDevice,
                      LPCSTR pPort,
                      WORD fwCapability,
                      LPSTR pOutput,
                      CONST DEVMODEA *pDevMode)
{
    /* Note Windows 2000/XP/VISTA always returns -1 */
    return -1;
}

/*
 * @implemented
 */
int
WINAPI
DeviceCapabilitiesExW(LPCWSTR pDevice,
                      LPCWSTR pPort,
                      WORD fwCapability,
                      LPWSTR pOutput,
                      CONST DEVMODEW *pDevMode)
{
    /* Note Windows 2000/XP/VISTA always returns -1 */
    return -1;
}

/*
 * @implemented
 */
BOOL
WINAPI
FixBrushOrgEx(HDC hDC,
              INT nXOrg,
              INT nYOrg,
              LPPOINT lpPoint)
{
    /* Note Windows 2000/XP/VISTA always returns FALSE */
    return FALSE;
}

/*
 * @implemented
 *
 * GDIEntry 16
 */
DWORD
WINAPI
DdSwapTextureHandles(LPDDRAWI_DIRECTDRAW_LCL pDDraw,
                     LPDDRAWI_DDRAWSURFACE_LCL pDDSLcl1,
                     LPDDRAWI_DDRAWSURFACE_LCL pDDSLcl2)
{
    /* Note Windows 2000/XP/VISTA always returns success */
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GdiReleaseLocalDC(HDC hdc)
{
    /* Note Windows 2000/XP/VISTA always returns TRUE */
    return TRUE;
}

/*
 * @implemented
 */
HBRUSH
WINAPI
SelectBrushLocal(HBRUSH Currenthbm,
                 HBRUSH Newhbm)
{
    return Newhbm;
}

/*
 * @implemented
 */
HFONT
WINAPI
SelectFontLocal(HFONT Currenthfnt,
                HFONT newhfnt)
{
    return newhfnt;
}

/*
 * @implemented
 */
HBRUSH
WINAPI
GdiGetLocalBrush(HBRUSH hbr)
{
    return hbr;
}

/*
 * @implemented
 */
HDC
WINAPI
GdiGetLocalDC(HDC hdc)
{
    return hdc;
}

/*
 * @implemented
 */
HFONT
WINAPI
GdiGetLocalFont(HFONT hfont)
{
    return hfont;
}

