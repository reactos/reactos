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
    return hbm;
}

/*
 * @implemented
 */
HBRUSH
STDCALL
GdiConvertBrush(HBRUSH hbr)
{
    return hbr;
}

/*
 * @implemented
 */
HDC 
STDCALL
GdiConvertDC(HDC hdc)
{
    return hdc;
}

/*
 * @implemented
 */
HFONT 
STDCALL
GdiConvertFont(HFONT hfont)
{
    return hfont;
}

/*
 * @implemented
 */
HPALETTE 
STDCALL
GdiConvertPalette(HPALETTE hpal)
{
    return hpal;
}

/*
 * @implemented
 */
HRGN
STDCALL
GdiConvertRegion(HRGN hregion)
{
    return hregion;
}

/*
 * @implemented
 */
BOOL
STDCALL
GdiSetAttrs(HDC hdc)
{
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




