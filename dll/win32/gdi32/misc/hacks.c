#include "precomp.h"

/* $Id: stubs.c 28709 2007-08-31 15:09:51Z greatlrd $
 *
 * reactos/lib/gdi32/misc/hacks.c
 *
 * GDI32.DLL hacks
 *
 * Api that are hacked but we can not do correct implemtions yetm but using own syscall
 *
 */

/*
 * @implemented
 *
 */
BOOL
STDCALL
OffsetViewportOrgEx(HDC hdc,
                    int nXOffset,
                    int nYOffset,
                    LPPOINT lpPoint)
{
    /* Unimplemented for now */
    return FALSE;
}

/*
 * @implemented
 *
 */
BOOL
STDCALL
OffsetWindowOrgEx(HDC hdc,
                  int nXOffset,
                  int nYOffset,
                  LPPOINT lpPoint)
{
    /* Unimplemented for now */
    return FALSE;
}



