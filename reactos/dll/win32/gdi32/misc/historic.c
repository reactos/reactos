/* $Id: stubs.c 28709 2007-08-31 15:09:51Z greatlrd $
 *
 * reactos/lib/gdi32/misc/historic.c
 *
 * GDI32.DLL Stubs
 *
 * Api that does basic nothing, but is here for backwords compatible with older windows
 *
 */

#include "precomp.h"
#include <ddraw.h>
#include <ddrawi.h>
#include <ddrawint.h>
#include <ddrawgdi.h>
#include <ntgdi.h>
#include <d3dhal.h>

/*
 * @implemented
 */
BOOL
STDCALL
EngQueryEMFInfo(HDEV hdev,
                EMFINFO *pEMFInfo)
{
    return FALSE;
}

/*
 * @implemented
 */
BOOL
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
GdiConvertBitmap(HBITMAP hbm)
{
    /* Note Windows 2000/XP/VISTA always returns hbm */
    return hbm;
}

/*
 * @implemented
 */
HBRUSH
STDCALL
GdiConvertBrush(HBRUSH hbr)
{
    /* Note Windows 2000/XP/VISTA always returns hbr */
    return hbr;
}

/*
 * @implemented
 */
HDC
STDCALL
GdiConvertDC(HDC hdc)
{
    /* Note Windows 2000/XP/VISTA always returns hdc */
    return hdc;
}

/*
 * @implemented
 */
HFONT
STDCALL
GdiConvertFont(HFONT hfont)
{
    /* Note Windows 2000/XP/VISTA always returns hfont */
    return hfont;
}

/*
 * @implemented
 */
HPALETTE
STDCALL
GdiConvertPalette(HPALETTE hpal)
{
    /* Note Windows 2000/XP/VISTA always returns hpal */
    return hpal;
}

/*
 * @implemented
 */
HRGN
STDCALL
GdiConvertRegion(HRGN hregion)
{
    /* Note Windows 2000/XP/VISTA always returns hregion */
    return hregion;
}

/*
 * @implemented
 */
BOOL
STDCALL
GdiSetAttrs(HDC hdc)
{
    /* Note Windows 2000/XP/VISTA always returns TRUE */
    return TRUE;
}

/*
 * @implemented
 */
BOOL
STDCALL
GdiDeleteLocalDC(HDC hdc)
{
    /* Note Windows 2000/XP/VISTA always returns TRUE */
    return TRUE;
}


/*
 * @implemented
 */
VOID
STDCALL
GdiSetServerAttr(HDC hdc,DWORD attr)
{
    /* it does do nothing */
}


/*
 * @implemented
 */
int
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
GdiReleaseLocalDC(HDC hdc)
{
    /* Note Windows 2000/XP/VISTA always returns TRUE */
    return TRUE;
}

/*
 * @implemented
 */
HBRUSH
STDCALL
SelectBrushLocal(HBRUSH Currenthbm,
                 HBRUSH Newhbm)
{
    return Newhbm;
}

/*
 * @implemented
 */
HFONT
STDCALL
SelectFontLocal(HFONT Currenthfnt,
                HFONT newhfnt)
{
    return newhfnt;
}

/*
 * @implemented
 */
HBRUSH
STDCALL
GdiGetLocalBrush(HBRUSH hbr)
{
    return hbr;
}

/*
 * @implemented
 */
HDC
STDCALL
GdiGetLocalDC(HDC hdc)
{
    return hdc;
}

/*
 * @implemented
 */
HFONT
STDCALL
GdiGetLocalFont(HFONT hfont)
{
    return hfont;
}

