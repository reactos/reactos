/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window classes
 * FILE:             subsys/win32k/ntuser/metric.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>


static BOOL Setup = FALSE;

/* FUNCTIONS *****************************************************************/

// FIXME: These are only win xp default values, mostly hardcoded. They should be
// read from the registry. It must be possible to change or reinitialize the
// values, for example desk.cpl
BOOL
FASTCALL
InitMetrics(VOID)
{
    NTSTATUS Status;
    PWINSTATION_OBJECT WinStaObject;
    ULONG Width = 640, Height = 480;
    PSYSTEM_CURSORINFO CurInfo;
    HDC hScreenDC;
    PDC pScreenDC;

    hScreenDC = IntGdiCreateDC(NULL, NULL, NULL, NULL, TRUE);
    if (hScreenDC)
    {
        pScreenDC = DC_LockDc(hScreenDC);
        if (pScreenDC)
        {
            Width = pScreenDC->ppdev->GDIInfo.ulHorzRes;
            Height = pScreenDC->ppdev->GDIInfo.ulVertRes;
            DC_UnlockDc(pScreenDC);
        }
        NtGdiDeleteObjectApp(hScreenDC);
    }

    Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
                                            KernelMode,
                                            0,
                                            &WinStaObject);
    if (NT_SUCCESS(Status))
    {
        CurInfo = IntGetSysCursorInfo(WinStaObject);
    }
    else
    {
        CurInfo = NULL;
    }

    gpsi->SystemMetrics[SM_CXSCREEN] = Width;
    gpsi->SystemMetrics[SM_CYSCREEN] = Height;
    gpsi->SystemMetrics[SM_CXVSCROLL] = 16;
    gpsi->SystemMetrics[SM_CYHSCROLL] = 16;
    /* FIXME: "reg://Control Panel/Desktop/WindowMetrics/CaptionHeight" + 1 */
    gpsi->SystemMetrics[SM_CYCAPTION] = 19;
    gpsi->SystemMetrics[SM_CXBORDER] = 1;
    gpsi->SystemMetrics[SM_CYBORDER] = 1;
    gpsi->SystemMetrics[SM_CXDLGFRAME] = 3;
    gpsi->SystemMetrics[SM_CYDLGFRAME] = 3;
    gpsi->SystemMetrics[SM_CYVTHUMB] = 16;
    gpsi->SystemMetrics[SM_CXHTHUMB] = 16;
    gpsi->SystemMetrics[SM_CXICON] = 32;
    gpsi->SystemMetrics[SM_CYICON] = 32;
    gpsi->SystemMetrics[SM_CXCURSOR] = 32;
    gpsi->SystemMetrics[SM_CYCURSOR] = 32;
    gpsi->SystemMetrics[SM_CYMENU] = 19;
    /* FIXME: shouldn't we take borders etc into account??? */
    gpsi->SystemMetrics[SM_CXFULLSCREEN] = gpsi->SystemMetrics[SM_CXSCREEN];
    gpsi->SystemMetrics[SM_CYFULLSCREEN] = gpsi->SystemMetrics[SM_CYSCREEN];
    gpsi->SystemMetrics[SM_CYKANJIWINDOW] = 0;
    gpsi->SystemMetrics[SM_MOUSEPRESENT] = 1;
    gpsi->SystemMetrics[SM_CYVSCROLL] = 16;
    gpsi->SystemMetrics[SM_CXHSCROLL] = 16;
    gpsi->SystemMetrics[SM_DEBUG] = 0;
    gpsi->SystemMetrics[SM_SWAPBUTTON] = CurInfo ? CurInfo->SwapButtons : 0;
    gpsi->SystemMetrics[SM_RESERVED1] = 0;
    gpsi->SystemMetrics[SM_RESERVED2] = 0;
    gpsi->SystemMetrics[SM_RESERVED3] = 0;
    gpsi->SystemMetrics[SM_RESERVED4] = 0;
    gpsi->SystemMetrics[SM_CXMIN] = 112;
    gpsi->SystemMetrics[SM_CYMIN] = 27;
    gpsi->SystemMetrics[SM_CXSIZE] = 18;
    gpsi->SystemMetrics[SM_CYSIZE] = 18;
    gpsi->SystemMetrics[SM_CXFRAME] = 4;
    gpsi->SystemMetrics[SM_CYFRAME] = 4;
    gpsi->SystemMetrics[SM_CXMINTRACK] = 112;
    gpsi->SystemMetrics[SM_CYMINTRACK] = 27;
    gpsi->SystemMetrics[SM_CXDOUBLECLK] = CurInfo ? CurInfo->DblClickWidth : 4;
    gpsi->SystemMetrics[SM_CYDOUBLECLK] = CurInfo ? CurInfo->DblClickWidth : 4;
    gpsi->SystemMetrics[SM_CXICONSPACING] = 64;
    gpsi->SystemMetrics[SM_CYICONSPACING] = 64;
    gpsi->SystemMetrics[SM_MENUDROPALIGNMENT] = 0;
    gpsi->SystemMetrics[SM_PENWINDOWS] = 0;
    gpsi->SystemMetrics[SM_DBCSENABLED] = 0;
    gpsi->SystemMetrics[SM_CMOUSEBUTTONS] = 2;
    gpsi->SystemMetrics[SM_SECURE] = 0;
    gpsi->SystemMetrics[SM_CXEDGE] = 2;
    gpsi->SystemMetrics[SM_CYEDGE] = 2;
    gpsi->SystemMetrics[SM_CXMINSPACING] = 160;
    gpsi->SystemMetrics[SM_CYMINSPACING] = 24;
    gpsi->SystemMetrics[SM_CXSMICON] = 16;
    gpsi->SystemMetrics[SM_CYSMICON] = 16;
    gpsi->SystemMetrics[SM_CYSMCAPTION] = 15;
    gpsi->SystemMetrics[SM_CXSMSIZE] = 12;
    gpsi->SystemMetrics[SM_CYSMSIZE] = 14;
    gpsi->SystemMetrics[SM_CXMENUSIZE] = 18;
    gpsi->SystemMetrics[SM_CYMENUSIZE] = 18;
    gpsi->SystemMetrics[SM_ARRANGE] = 8;
    gpsi->SystemMetrics[SM_CXMINIMIZED] = 160;
    gpsi->SystemMetrics[SM_CYMINIMIZED] = 24;
    gpsi->SystemMetrics[SM_CXMAXTRACK] = gpsi->SystemMetrics[SM_CYSCREEN] + 12;
    gpsi->SystemMetrics[SM_CYMAXTRACK] = gpsi->SystemMetrics[SM_CYSCREEN] + 12;
    /* This seems to be 8 pixels greater than the screen width */
    gpsi->SystemMetrics[SM_CXMAXIMIZED] = gpsi->SystemMetrics[SM_CXSCREEN] + 8;
    /* This seems to be 20 pixels less than the screen height, taskbar maybe? */
    gpsi->SystemMetrics[SM_CYMAXIMIZED] = gpsi->SystemMetrics[SM_CYSCREEN] - 20;
    gpsi->SystemMetrics[SM_NETWORK] = 3;
    gpsi->SystemMetrics[64] = 0;
    gpsi->SystemMetrics[65] = 0;
    gpsi->SystemMetrics[66] = 0;
    gpsi->SystemMetrics[SM_CLEANBOOT] = 0;
    gpsi->SystemMetrics[SM_CXDRAG] = 4;
    gpsi->SystemMetrics[SM_CYDRAG] = 4;
    gpsi->SystemMetrics[SM_SHOWSOUNDS] = 0;
    gpsi->SystemMetrics[SM_CXMENUCHECK] = 13;
    gpsi->SystemMetrics[SM_CYMENUCHECK] = 13;
    gpsi->SystemMetrics[SM_SLOWMACHINE] = 0;
    gpsi->SystemMetrics[SM_MIDEASTENABLED] = 0;
    gpsi->SystemMetrics[SM_MOUSEWHEELPRESENT] = 1;
    gpsi->SystemMetrics[SM_XVIRTUALSCREEN] = 0;
    gpsi->SystemMetrics[SM_YVIRTUALSCREEN] = 0;
    gpsi->SystemMetrics[SM_CXVIRTUALSCREEN] = Width;
    gpsi->SystemMetrics[SM_CYVIRTUALSCREEN] = Height;
    gpsi->SystemMetrics[SM_CMONITORS] = 1;
    gpsi->SystemMetrics[SM_SAMEDISPLAYFORMAT] = 1;
    gpsi->SystemMetrics[SM_IMMENABLED] = 0;
    gpsi->SystemMetrics[SM_CXFOCUSBORDER] = 1;
    gpsi->SystemMetrics[SM_CYFOCUSBORDER] = 1;
    gpsi->SystemMetrics[SM_TABLETPC] = 0;
    gpsi->SystemMetrics[SM_MEDIACENTER] = 0;
    gpsi->SystemMetrics[SM_STARTER] = 0;
    gpsi->SystemMetrics[SM_SERVERR2] = 0;
#if (_WIN32_WINNT >= 0x0600)
    gpsi->SystemMetrics[90] = 0;
    gpsi->SystemMetrics[SM_MOUSEHORIZONTALWHEELPRESENT] = 0;
    gpsi->SystemMetrics[SM_CXPADDEDBORDER] = 0;
#endif

    gpsi->SRVINFO_Flags |= SRVINFO_METRICS;
    Setup = TRUE;

    if (NT_SUCCESS(Status))
    {
        ObDereferenceObject(WinStaObject);
    }

    return TRUE;
}

ULONG FASTCALL
UserGetSystemMetrics(ULONG Index)
{
    ASSERT(gpsi);
    DPRINT("UserGetSystemMetrics(%d)\n", Index);

    // FIXME: Do this when loading
    if (!Setup)
    {
        InitMetrics();
    }

    /* Get metrics from array */
    if (Index < SM_CMETRICS)
    {
        return gpsi->SystemMetrics[Index];
    }

    /* Handle special values */
    switch (Index)
    {
        case SM_REMOTESESSION:
            return 0; // FIXME

        case SM_SHUTTINGDOWN:
            return 0; // FIXME

        case SM_REMOTECONTROL:
            return 0; // FIXME
    }

    DPRINT1("UserGetSystemMetrics() called with invalid index %d\n", Index);
    return 0;
}


/* EOF */
