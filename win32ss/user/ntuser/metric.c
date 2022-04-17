/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Window classes
 * FILE:             win32ss/user/ntuser/metric.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                   Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserSysparams);

static BOOL Setup = FALSE;

/* FUNCTIONS *****************************************************************/

BOOL APIENTRY UserIsDBCSEnabled(VOID)
{
    switch (PRIMARYLANGID(gusLanguageID))
    {
        case LANG_CHINESE:
        case LANG_JAPANESE:
        case LANG_KOREAN:
            return TRUE;

        default:
            return FALSE;
    }
}

BOOL
NTAPI
InitMetrics(VOID)
{
    INT *piSysMet = gpsi->aiSysMet;
    ULONG Width, Height;

    /* Note: used for the SM_CLEANBOOT metric */
    DWORD dwValue = 0;
    HKEY hKey = 0;

    /* Clean boot */
    piSysMet[SM_CLEANBOOT] = 0; // Fallback value of 0 (normal mode)
    if(NT_SUCCESS(RegOpenKey(L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Option", &hKey)))
    {
        if(RegReadDWORD(hKey, L"OptionValue", &dwValue)) piSysMet[SM_CLEANBOOT] = (INT)dwValue;
        ZwClose(hKey);
    }

    /* FIXME: HACK, due to missing MDEV on first init */
    if (!gpmdev)
    {
        Width = 640;
        Height = 480;
    }
    else
    {
        Width = gpmdev->ppdevGlobal->gdiinfo.ulHorzRes;
        Height = gpmdev->ppdevGlobal->gdiinfo.ulVertRes;
    }

    /* Screen sizes */
    piSysMet[SM_CXSCREEN] = Width;
    piSysMet[SM_CYSCREEN] = Height;
    piSysMet[SM_XVIRTUALSCREEN] = 0;
    piSysMet[SM_YVIRTUALSCREEN] = 0;
    piSysMet[SM_CXVIRTUALSCREEN] = Width;
    piSysMet[SM_CYVIRTUALSCREEN] = Height;

    /* NC area sizes */
    piSysMet[SM_CYCAPTION] = gspv.ncm.iCaptionHeight + 1;       // 19
    piSysMet[SM_CYSMCAPTION] = gspv.ncm.iSmCaptionHeight + 1;   // 15;
    piSysMet[SM_CXSIZE] = gspv.ncm.iCaptionHeight;              // 18;
    piSysMet[SM_CYSIZE] = gspv.ncm.iCaptionHeight;              // 18;
    piSysMet[SM_CXSMSIZE] = gspv.ncm.iSmCaptionWidth;   // 12; XP: piSysMet(SM_CYSMCAPTION) - 1
    piSysMet[SM_CYSMSIZE] = gspv.ncm.iSmCaptionHeight;  // 14;
    piSysMet[SM_CXBORDER] = 1; // Seems to be hardcoded
    piSysMet[SM_CYBORDER] = 1; // Seems to be hardcoded
    piSysMet[SM_CXFOCUSBORDER] = 1;
    piSysMet[SM_CYFOCUSBORDER] = 1;
    piSysMet[SM_CXDLGFRAME] = 3;
    piSysMet[SM_CYDLGFRAME] = 3;
    piSysMet[SM_CXEDGE] = 2;
    piSysMet[SM_CYEDGE] = 2;
    piSysMet[SM_CXFRAME] = piSysMet[SM_CXDLGFRAME] + gspv.ncm.iBorderWidth; // 4
    piSysMet[SM_CYFRAME] = piSysMet[SM_CYDLGFRAME] + gspv.ncm.iBorderWidth; // 4
#if (_WIN32_WINNT >= 0x0600)
    piSysMet[SM_CXPADDEDBORDER] = 0;
#endif

    /* Window sizes */
    TRACE("ncm.iCaptionWidth=%d,GetSystemMetrics(SM_CYSIZE)=%d,GetSystemMetrics(SM_CXFRAME)=%d,avcwCaption=%d \n",
           gspv.ncm.iCaptionWidth, piSysMet[SM_CYSIZE],piSysMet[SM_CXFRAME], gspv.tmCaptionFont.tmAveCharWidth);

    piSysMet[SM_CXMIN] = 3 * max(gspv.ncm.iCaptionWidth, 8) // 112
                         + piSysMet[SM_CYSIZE] + 4
                         + 4 * gspv.tmCaptionFont.tmAveCharWidth
                         + 2 * piSysMet[SM_CXFRAME];
    piSysMet[SM_CYMIN] = piSysMet[SM_CYCAPTION] + 2 * piSysMet[SM_CYFRAME]; // 27
    piSysMet[SM_CXMAXIMIZED] = piSysMet[SM_CXSCREEN] + 2 * piSysMet[SM_CXFRAME];
    piSysMet[SM_CYMAXIMIZED] = piSysMet[SM_CYSCREEN] - 20;
    piSysMet[SM_CXFULLSCREEN] = piSysMet[SM_CXSCREEN];
    piSysMet[SM_CYFULLSCREEN] = piSysMet[SM_CYMAXIMIZED] - piSysMet[SM_CYMIN];
    piSysMet[SM_CYKANJIWINDOW] = 0;
    piSysMet[SM_CXMINIMIZED] = gspv.mm.iWidth + 6;
    piSysMet[SM_CYMINIMIZED] = piSysMet[SM_CYCAPTION] + 5;
    piSysMet[SM_CXMINSPACING] = piSysMet[SM_CXMINIMIZED] + gspv.mm.iHorzGap;
    piSysMet[SM_CYMINSPACING] = piSysMet[SM_CYMINIMIZED] + gspv.mm.iVertGap;
    piSysMet[SM_CXMAXTRACK] = piSysMet[SM_CXVIRTUALSCREEN] + 4
                              + 2 * piSysMet[SM_CXFRAME];
    piSysMet[SM_CYMAXTRACK] = piSysMet[SM_CYVIRTUALSCREEN] + 4
                              + 2 * piSysMet[SM_CYFRAME];

    /* Icon */
    piSysMet[SM_CXVSCROLL] = gspv.ncm.iScrollWidth;     // 16;
    piSysMet[SM_CYVTHUMB] = gspv.ncm.iScrollHeight;     // 16;
    piSysMet[SM_CYHSCROLL] = gspv.ncm.iScrollWidth;     // 16;
    piSysMet[SM_CXHTHUMB] = gspv.ncm.iScrollHeight;     // 16;
    piSysMet[SM_CYVSCROLL] = gspv.ncm.iScrollHeight;    // 16
    piSysMet[SM_CXHSCROLL] = gspv.ncm.iScrollHeight;    // 16;
    piSysMet[SM_CXICON] = 32;
    piSysMet[SM_CYICON] = 32;
    piSysMet[SM_CXSMICON] = 16;
    piSysMet[SM_CYSMICON] = 16;
    piSysMet[SM_CXICONSPACING] = gspv.im.iHorzSpacing;  // 64;
    piSysMet[SM_CYICONSPACING] = gspv.im.iVertSpacing;  // 64;
    piSysMet[SM_CXCURSOR] = 32;
    piSysMet[SM_CYCURSOR] = 32;
    piSysMet[SM_CXMINTRACK] = piSysMet[SM_CXMIN];       // 117
    piSysMet[SM_CYMINTRACK] = piSysMet[SM_CYMIN];       // 27
    piSysMet[SM_CXDRAG] = 4;
    piSysMet[SM_CYDRAG] = 4;
    piSysMet[SM_ARRANGE] = gspv.mm.iArrange;            // 8;

    /* Menu */
    piSysMet[SM_CYMENU] = gspv.ncm.iMenuHeight + 1;     // 19;
    piSysMet[SM_MENUDROPALIGNMENT] = gspv.bMenuDropAlign;
    piSysMet[SM_CXMENUCHECK] = ((1 + gspv.tmMenuFont.tmHeight +
                                 gspv.tmMenuFont.tmExternalLeading) & ~1) - 1; // 13;
    piSysMet[SM_CYMENUCHECK] = piSysMet[SM_CXMENUCHECK];
    piSysMet[SM_CXMENUSIZE] = gspv.ncm.iMenuWidth;      // 18;
    piSysMet[SM_CYMENUSIZE] = gspv.ncm.iMenuHeight;     // 18;

    /* Mouse */
    piSysMet[SM_MOUSEPRESENT] = 1;
    piSysMet[SM_MOUSEWHEELPRESENT] = 1;
    piSysMet[SM_CMOUSEBUTTONS] = 2;
    piSysMet[SM_SWAPBUTTON] = gspv.bMouseBtnSwap ? 1 : 0;
    piSysMet[SM_CXDOUBLECLK] = gspv.iDblClickWidth;
    piSysMet[SM_CYDOUBLECLK] = gspv.iDblClickHeight;
#if (_WIN32_WINNT >= 0x0600)
    piSysMet[SM_MOUSEHORIZONTALWHEELPRESENT] = 0;
#endif

    /* Version info */
    piSysMet[SM_TABLETPC] = 0;
    piSysMet[SM_MEDIACENTER] = 0;
    piSysMet[SM_STARTER] = 0;
    piSysMet[SM_SERVERR2] = 0;
    piSysMet[SM_PENWINDOWS] = 0;

    /* Other */
    piSysMet[SM_DEBUG] = 0;
    piSysMet[SM_NETWORK] = 3;
    piSysMet[SM_SLOWMACHINE] = 0;
    piSysMet[SM_SECURE] = 0;
    piSysMet[SM_DBCSENABLED] = UserIsDBCSEnabled();
    piSysMet[SM_SHOWSOUNDS] = gspv.bShowSounds;
    piSysMet[SM_MIDEASTENABLED] = 0;
    piSysMet[SM_CMONITORS] = 1;
    piSysMet[SM_SAMEDISPLAYFORMAT] = 1;
    piSysMet[SM_IMMENABLED] = 0;

    /* Reserved */
    piSysMet[SM_RESERVED1] = 0;
    piSysMet[SM_RESERVED2] = 0;
    piSysMet[SM_RESERVED3] = 0;
    piSysMet[SM_RESERVED4] = 0;
    piSysMet[64] = 0;
    piSysMet[65] = 0;
    piSysMet[66] = 0;
#if (_WIN32_WINNT >= 0x0600)
    piSysMet[90] = 0;
#endif

    gpsi->dwSRVIFlags |= SRVINFO_CICERO_ENABLED;
    Setup = TRUE;

    return TRUE;
}

LONG
NTAPI
UserGetSystemMetrics(ULONG Index)
{
    ASSERT(gpsi);
    ASSERT(Setup);
    TRACE("UserGetSystemMetrics(%lu)\n", Index);

    if (Index == SM_DBCSENABLED)
        return UserIsDBCSEnabled();

    /* Get metrics from array */
    if (Index < SM_CMETRICS)
    {
        return gpsi->aiSysMet[Index];
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

    ERR("UserGetSystemMetrics() called with invalid index %lu\n", Index);
    return 0;
}

/* EOF */
