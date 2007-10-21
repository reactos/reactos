/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native driver enumate of dxeng implementation
 * FILE:             subsys/win32k/ntddraw/dxeng.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       15/10-2007   Magnus Olsen
 */

#include <w32k.h>
#include <debug.h>

/************************************************************************/
/* DxEngNUIsTermSrv                                                     */
/************************************************************************/

/* Notes : Check see if termal server got a connections or not */
BOOL
DxEngNUIsTermSrv()
{
    /* FIXME ReactOS does not suport terminal server yet, we can not check if we got a connections or not */
    DPRINT1("We need termal server connections check");
    return FALSE;
}

/************************************************************************/
/* DxEngRedrawDesktop                                                   */
/************************************************************************/

/* Notes : it always return TRUE, and it update whole the screen (redaw current desktop) */
BOOL
DxEngRedrawDesktop()
{
    /* FIXME add redraw code */
    DPRINT1("We need add code for redraw whole desktop");
    return TRUE;
}


/************************************************************************/
/* DxEngDispUniq                                                        */
/************************************************************************/

/*  Notes : return the DisplayUniqVisrgn counter from gdishare memory  */
ULONG
DxEngDispUniq()
{
    /* FIXME DisplayUniqVisrgn from gdishare memory */
    DPRINT1("We need DisplayUniqVisrgn from gdishare memory");
    return 0;
}

/************************************************************************/
/* DxEngVisRgnUniq                                                      */
/************************************************************************/
/* Notes :  return the VisRgnUniq counter for win32k */
ULONG
DxEngVisRgnUniq()
{
    /* FIXME DisplayUniqVisrgn from gdishare memory */
    DPRINT1("We need VisRgnUniq from win32k");
    return 0;
}

/************************************************************************/
/* DxEngEnumerateHdev                                                   */
/************************************************************************/
/* Enumate all drivers in win32k */
HDEV *
DxEngEnumerateHdev(HDEV *hdev)
{
    /* FIXME Enumate all drivers in win32k */
    DPRINT1("We do not enumate any device from win32k ");
    return 0;
}

/************************************************************************/
/* DxEngGetDeviceGammaRamp                                              */
/************************************************************************/
/* same protypes NtGdiEngGetDeviceGammaRamp, diffent is we skipp the user mode checks and seh */
BOOL
DxEngGetDeviceGammaRamp(HDC hDC, LPVOID lpRamp)
{
    /* FIXME redirect it to NtGdiEngGetDeviceGammaRamp internal call  */
    DPRINT1("redirect it to NtGdiEngGetDeviceGammaRamp internal call ");
    return FALSE;
}




