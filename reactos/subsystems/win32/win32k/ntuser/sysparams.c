/*
 * COPYRIGHT:        GPL, see COPYING in the top level directory
 * PROJECT:          ReactOS win32 kernel mode subsystem server
 * PURPOSE:          System parameters functions
 * FILE:             subsystem/win32/win32k/ntuser/sysparams.c
 * PROGRAMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

// TODO:
// - check all values that are in Winsta in ros
// - does setting invalid fonts work?
// - save appropriate text metrics

#include <w32k.h>

#define NDEBUG
#include <debug.h>

#include <winsta.h>

#define KeRosDumpStackFrames(Frames, Count) KdSystemDebugControl(TAG('R', 'o', 's', 'D'), (PVOID)Frames, Count, NULL, 0, NULL, KernelMode)
HBITMAP NTAPI UserLoadImage(PCWSTR);
BOOL NTAPI W32kDosPathNameToNtPathName(PCWSTR, PUNICODE_STRING);

BOOL gbDebug = 0;
SPIVALUES gspv;
BOOL gbSpiInitialized = FALSE;
PWINSTATION_OBJECT gpwinstaCurrent = NULL;

// HACK! We initialize SPI before we have a proper surface to get this from.
#define dpi 96
//(pPrimarySurface->GDIInfo.ulLogPixelsY)
#define REG2METRIC(reg) (reg > 0 ? reg : ((-(reg) * dpi + 720) / 1440))
#define METRIC2REG(met) (-((((met) * 1440)- 0) / dpi))

#define REQ_INTERACTIVE_WINSTA(err) \
    if (gpwinstaCurrent != InputWindowStation) \
    { \
        SetLastWin32Error(err); \
        return 0; \
    }

#define DPRINTX if (gbDebug) DPRINT1

static const WCHAR* KEY_MOUSE = L"Control Panel\\Mouse";
static const WCHAR* VAL_MOUSE1 = L"MouseThreshold1";
static const WCHAR* VAL_MOUSE2 = L"MouseThreshold2";
static const WCHAR* VAL_MOUSE3 = L"MouseSpeed";
static const WCHAR* VAL_DBLCLKWIDTH = L"DoubleClickWidth";
static const WCHAR* VAL_DBLCLKHEIGHT = L"DoubleClickHeight";
static const WCHAR* VAL_DBLCLKTIME = L"DoubleClickSpeed";
static const WCHAR* VAL_SWAP = L"SwapMouseButtons";

static const WCHAR* KEY_DESKTOP = L"Control Panel\\Desktop";
static const WCHAR* VAL_SCRTO = L"ScreenSaveTimeOut";
static const WCHAR* VAL_SCRACT = L"ScreenSaveActive";
static const WCHAR* VAL_GRID = L"GridGranularity";
static const WCHAR* VAL_DRAG = L"DragFullWindows";
static const WCHAR* VAL_DRAGHEIGHT = L"DragHeight";
static const WCHAR* VAL_DRAGWIDTH = L"DragWidth";
static const WCHAR* VAL_FNTSMOOTH = L"FontSmoothing";

static const WCHAR* KEY_MDALIGN = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows";
static const WCHAR* VAL_MDALIGN = L"MenuDropAlignment";

static const WCHAR* KEY_METRIC = L"Control Panel\\Desktop\\WindowMetrics";
static const WCHAR* VAL_BORDER = L"BorderWidth";
static const WCHAR* VAL_ICONSPC = L"IconSpacing";
static const WCHAR* VAL_ICONVSPC = L"IconVerticalspacing";
static const WCHAR* VAL_ITWRAP = L"IconTitleWrap";

static const WCHAR* KEY_SOUND = L"Control Panel\\Sound";
static const WCHAR* VAL_BEEP = L"Beep";

static const WCHAR* KEY_KBD = L"Control Panel\\Keyboard";
static const WCHAR* VAL_KBDSPD = L"KeyboardSpeed";
static const WCHAR* VAL_KBDDELAY = L"KeyboardDelay";

static const WCHAR* KEY_SHOWSNDS = L"Control Panel\\Accessibility\\ShowSounds";
static const WCHAR* KEY_KDBPREF = L"Control Panel\\Accessibility\\Keyboard Preference";
static const WCHAR* KEY_SCRREAD = L"Control Panel\\Accessibility\\Blind Access";
static const WCHAR* VAL_ON = L"On";



/** Loading the settings ******************************************************/

static
INT
SpiLoadInt(PCWSTR pwszKey, PCWSTR pwszValue, INT iValue)
{
    WCHAR awcBuffer[12];
    ULONG cbSize;

    cbSize = sizeof(awcBuffer);
    if (!RegReadUserSetting(pwszKey, pwszValue, REG_SZ, awcBuffer, cbSize))
    {
        return iValue;
    }
    return _wtoi(awcBuffer);
}

static
INT
SpiLoadMouse(LPWSTR pwszValue, INT iValue)
{
    return SpiLoadInt(KEY_MOUSE, pwszValue, iValue);
}

static
INT
SpiLoadMetric(LPWSTR pwszValue, INT iValue)
{
    INT iRegVal;

    iRegVal = SpiLoadInt(KEY_METRIC, pwszValue, METRIC2REG(iValue));
    DPRINT("Loaded metric setting '%S', iValue=%d(reg:%d), ret=%d(reg:%d)\n",
           pwszValue, iValue, METRIC2REG(iValue), REG2METRIC(iRegVal), iRegVal);
    return REG2METRIC(iRegVal);
}

static
VOID
SpiLoadFont(PLOGFONTW plfOut, LPWSTR pwszValueName, PLOGFONTW plfDefault)
{
    BOOL bResult;

    bResult = RegReadUserSetting(KEY_METRIC,
                                 pwszValueName,
                                 REG_BINARY,
                                 plfOut,
                                 sizeof(LOGFONTW));
    if (!bResult)
        *plfOut = *plfDefault;
}

static
VOID
SpiFixupValues()
{
    /* Fixup values */
    gspv.ncm.iCaptionWidth = max(gspv.ncm.iCaptionWidth, 8);
    gspv.ncm.iBorderWidth = max(gspv.ncm.iBorderWidth, 1);
    gspv.ncm.iScrollWidth = max(gspv.ncm.iScrollWidth, 8);
    gspv.ncm.iScrollHeight = max(gspv.ncm.iScrollHeight, 8);
//    gspv.ncm.iMenuHeight = max(gspv.ncm.iMenuHeight, gspv.tmMenuFont.tmHeight);
//    gspv.ncm.iMenuHeight = max(gspv.ncm.iMenuHeight,
//                               2 + gspv.tmMenuFont.tmHeight +
//                               gspv.tmMenuFont.tmExternalLeading);
    if (gspv.iDblClickTime == 0) gspv.iDblClickTime = 500;

    // FIXME: hack!!!
    gspv.tmMenuFont.tmHeight = 11;
    gspv.tmMenuFont.tmExternalLeading = 2;

    gspv.tmCaptionFont.tmHeight = 11;
    gspv.tmCaptionFont.tmExternalLeading = 2;

}

static
VOID
SpiUpdatePerUserSystemParameters()
{
    static LOGFONTW lf1 = {-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE,
                           FALSE, ANSI_CHARSET, 0, 0, DEFAULT_QUALITY,
                           VARIABLE_PITCH | FF_SWISS, L"MS Sans Serif"};
    static LOGFONTW lf2 = {-11, 0, 0, 0, FW_BOLD, FALSE, FALSE,
                           FALSE, ANSI_CHARSET, 0, 0, DEFAULT_QUALITY,
                           VARIABLE_PITCH | FF_SWISS, L"MS Sans Serif"};

    DPRINT("Enter SpiUpdatePerUserSystemParameters\n");

    /* Clear the structure */
    memset(&gspv, 0, sizeof(gspv));

    /* Load mouse settings */
    gspv.caiMouse.FirstThreshold = SpiLoadMouse(L"MouseThreshold1", 6);
    gspv.caiMouse.SecondThreshold = SpiLoadMouse(L"MouseThreshold2", 10);
    gspv.caiMouse.Acceleration = SpiLoadMouse(L"MouseSpeed", 1);

    /* Load NONCLIENTMETRICS */
    gspv.ncm.cbSize = sizeof(NONCLIENTMETRICSW);
    gspv.ncm.iBorderWidth = SpiLoadMetric(L"BorderWidth", 1);
    gspv.ncm.iScrollWidth = SpiLoadMetric(L"ScrollWidth", 16);
    gspv.ncm.iScrollHeight = SpiLoadMetric(L"ScrollHeight", 16);
    gspv.ncm.iCaptionWidth = SpiLoadMetric(L"CaptionWidth", 19);
    gspv.ncm.iCaptionHeight = SpiLoadMetric(L"CaptionHeight", 19);
    gspv.ncm.iSmCaptionWidth = SpiLoadMetric(L"SmCaptionWidth", 12);
    gspv.ncm.iSmCaptionHeight =  SpiLoadMetric(L"SmCaptionHeight", 14);
    gspv.ncm.iMenuWidth = SpiLoadMetric(L"MenuWidth", 18);
    gspv.ncm.iMenuHeight = SpiLoadMetric(L"MenuHeight", 18);
#if (WINVER >= 0x0600)
    gspv.ncm.iPaddedBorderWidth = SpiLoadMetric(L"PaddedBorderWidth", 18);
#endif
    SpiLoadFont(&gspv.ncm.lfCaptionFont, L"CaptionFont", &lf2);
    SpiLoadFont(&gspv.ncm.lfSmCaptionFont, L"SmCaptionFont", &lf1);
    SpiLoadFont(&gspv.ncm.lfMenuFont, L"MenuFont", &lf1);
    SpiLoadFont(&gspv.ncm.lfStatusFont, L"StatusFont", &lf1);
    SpiLoadFont(&gspv.ncm.lfMessageFont, L"MessageFont", &lf1);

    /* Load MINIMIZEDMETRICS */
    gspv.mm.cbSize = sizeof(MINIMIZEDMETRICS);
    gspv.mm.iWidth = SpiLoadMetric(L"MinWidth", 160);
    gspv.mm.iHorzGap = SpiLoadMetric(L"MinHorzGap", 160);
    gspv.mm.iVertGap = SpiLoadMetric(L"MinVertGap", 24);
    gspv.mm.iArrange = SpiLoadInt(KEY_METRIC, L"MinArrange", ARW_HIDE);

    /* Load ICONMETRICS */
    gspv.im.cbSize = sizeof(ICONMETRICSW);
    gspv.im.iHorzSpacing = SpiLoadMetric(L"IconSpacing", 64);
    gspv.im.iVertSpacing = SpiLoadMetric(L"IconVerticalspacing", 64);
    gspv.im.iTitleWrap = SpiLoadMetric(L"IconTitleWrap", 0);
    SpiLoadFont(&gspv.im.lfFont, L"IconFont", &lf1);

    /* Some hardcoded values for now */
    gspv.tmCaptionFont.tmAveCharWidth = 6;
    gspv.bBeep = TRUE;
    gspv.bFlatMenu = FALSE;
    gspv.iDblClickTime = 500;
    gspv.uiFocusBorderWidth = 1;
    gspv.uiFocusBorderHeight = 1;
    gspv.bMenuDropAlign = 1;
    gspv.bDropShadow = 1;
    gspv.dwUserPrefMask = UPM_DEFAULT;
    gspv.dwMenuShowDelay = 100;

    gspv.bMouseBtnSwap = FALSE;
    gspv.iMouseSpeed = 10;
    gspv.iMouseHoverTime = 80;
    gspv.iMouseHoverWidth = 4;
    gspv.iMouseHoverHeight = 4;
    gspv.iDblClickTime = 500;
    gspv.iDblClickWidth = 4;
    gspv.iDblClickHeight = 4;
    gspv.iWheelScrollLines = 3;
#if (_WIN32_WINNT >= 0x0600)
    gspv.uiWheelScrollChars = 1;
#endif

    gspv.iScrSaverTimeout = 10;
    gspv.bScrSaverActive = FALSE;
    gspv.bScrSaverRunning = FALSE;
#if(WINVER >= 0x0600)
    gspv.bScrSaverSecure = FALSE;
#endif

    /* Make sure we don't use broken values */
    SpiFixupValues();

    /* Update SystemMetrics */
    InitMetrics();
}

BOOL
InitSysParams()
{
    SpiUpdatePerUserSystemParameters();
    gbSpiInitialized = TRUE;
    return TRUE;
}


BOOL
APIENTRY
NtUserUpdatePerUserSystemParameters(
    DWORD dwReserved,
    BOOL bEnable)
{
    BOOL bResult;

    DPRINT("Enter NtUserUpdatePerUserSystemParameters\n");
    UserEnterExclusive();

    SpiUpdatePerUserSystemParameters();
    bResult = IntDesktopUpdatePerUserSettings(bEnable);

    DPRINT("Leave NtUserUpdatePerUserSystemParameters, returning %d\n", bResult);
    UserLeave();

    return bResult;
}


/** Storing the settings ******************************************************/

static
VOID
SpiStoreSz(PCWSTR pwszKey, PCWSTR pwszValue, PCWSTR pwsz)
{
    RegWriteUserSetting(pwszKey,
                        pwszValue,
                        REG_SZ,
                        (PWSTR)pwsz,
                        wcslen(pwsz) * sizeof(WCHAR));
}

static
VOID
SpiStoreSzInt(PCWSTR pwszKey, PCWSTR pwszValue, INT iValue)
{
    WCHAR awcBuffer[15];

    _itow(iValue, awcBuffer, 10);
    RegWriteUserSetting(pwszKey,
                        pwszValue,
                        REG_SZ,
                        awcBuffer,
                        (wcslen(awcBuffer) + 1) * sizeof(WCHAR));
}

static
VOID
SpiStoreMetric(LPCWSTR pwszValue, INT iValue)
{
    SpiStoreSzInt(KEY_METRIC, pwszValue, METRIC2REG(iValue));
}

static
VOID
SpiStoreFont(PCWSTR pwszValue, LOGFONTW* plogfont)
{
    RegWriteUserSetting(KEY_METRIC,
                        pwszValue,
                        REG_BINARY,
                        plogfont,
                        sizeof(LOGFONTW));
}


/** Get/Set value *************************************************************/

// FIXME: get rid of the flags and only use this from um. kernel can access data directly.
static
UINT_PTR
SpiMemCopy(PVOID pvDst, PVOID pvSrc, ULONG cbSize, BOOL bProtect, BOOL bToUser)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (bProtect)
    {
        _SEH2_TRY
        {
            if (bToUser)
            {
                ProbeForWrite(pvDst, cbSize, 1);
            }
            else
            {
                ProbeForRead(pvSrc, cbSize, 1);
            }
            memcpy(pvDst, pvSrc, cbSize);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END
    }
    else
    {
        memcpy(pvDst, pvSrc, cbSize);
    }

    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        DPRINT("SpiMemCopy failed, pvDst=%p, pvSrc=%p, bProtect=%d, bToUser=%d\n", pvDst, pvSrc, bProtect, bToUser);
    }
    return NT_SUCCESS(Status);
}

static inline
UINT_PTR
SpiGet(PVOID pvParam, PVOID pvData, ULONG cbSize, FLONG fl)
{
    REQ_INTERACTIVE_WINSTA(ERROR_ACCESS_DENIED);
    return SpiMemCopy(pvParam, pvData, cbSize, fl & SPIF_PROTECT, TRUE);
}

static inline
UINT_PTR
SpiSet(PVOID pvData, PVOID pvParam, ULONG cbSize, FLONG fl)
{
    REQ_INTERACTIVE_WINSTA(ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);
    return SpiMemCopy(pvData, pvParam, cbSize, fl & SPIF_PROTECT, FALSE);
}

static inline
UINT_PTR
SpiGetEx(PVOID pvParam, PVOID pvData, ULONG cbSize, FLONG fl)
{
    ULONG cbBufSize;
    /* Get the cbSite member from UM memory */
    if (!SpiSet(&cbBufSize, pvParam, sizeof(ULONG), fl))
        return 0;
    /* Verify the correct size */
    if (cbBufSize != cbSize)
        return 0;
    return SpiGet(pvParam, pvData, cbSize, fl);
}

static inline
UINT_PTR
SpiGetInt(PVOID pvParam, PVOID piValue, FLONG fl)
{
    return SpiGet(pvParam, piValue, sizeof(INT), fl);
}

static inline
UINT_PTR
SpiSetYesNo(BOOL *pbData, BOOL bValue, PCWSTR pwszKey, PCWSTR pwszValue, FLONG fl)
{
    REQ_INTERACTIVE_WINSTA(ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);
    *pbData = bValue ? TRUE : FALSE;
    if (fl & SPIF_UPDATEINIFILE)
    {
        SpiStoreSz(pwszKey, pwszValue, bValue ? L"Yes" : L"No");
    }
    return (UINT_PTR)pwszKey;
}

static inline
UINT_PTR
SpiSetBool(BOOL *pbData, INT iValue, PCWSTR pwszKey, PCWSTR pwszValue, FLONG fl)
{
    REQ_INTERACTIVE_WINSTA(ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);
    *pbData = iValue ? TRUE : FALSE;
    if (fl & SPIF_UPDATEINIFILE)
    {
        SpiStoreSzInt(pwszKey, pwszValue, iValue);
    }
    return (UINT_PTR)pwszKey;
}

static inline
UINT_PTR
SpiSetInt(PVOID pvData, INT iValue, PCWSTR pwszKey, PCWSTR pwszValue, FLONG fl)
{
    REQ_INTERACTIVE_WINSTA(ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);
    *(INT*)pvData = iValue;
    if (fl & SPIF_UPDATEINIFILE)
    {
        SpiStoreSzInt(pwszKey, pwszValue, iValue);
    }
    return (UINT_PTR)pwszKey;
}

static inline
UINT_PTR
SpiSetMetric(PVOID pvData, INT iValue, PCWSTR pwszValue, FLONG fl)
{
    REQ_INTERACTIVE_WINSTA(ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);
    *(INT*)pvData = iValue;
    if (fl & SPIF_UPDATEINIFILE)
    {
        SpiStoreMetric(pwszValue, iValue);
    }
    return (UINT_PTR)KEY_METRIC;
}

static inline
UINT_PTR
SpiSetUserPref(DWORD dwMask, PVOID pvValue, FLONG fl)
{
    DWORD dwRegMask;
    BOOL bValue = pvValue ? 1 : 0;

    REQ_INTERACTIVE_WINSTA(ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);

    /* Set or clear bit according to bValue */
    gspv.dwUserPrefMask = bValue ? gspv.dwUserPrefMask | dwMask :
                                   gspv.dwUserPrefMask & ~dwMask;

    if (fl & SPIF_UPDATEINIFILE)
    {
        /* Read current value */
        RegReadUserSetting(KEY_DESKTOP,
                           L"UserPreferencesMask",
                           REG_BINARY,
                           &dwRegMask,
                           sizeof(DWORD));

        /* Set or clear bit according to bValue */
        dwRegMask = bValue ? dwRegMask | dwMask : dwRegMask & ~dwMask;

        /* write back value */
        RegWriteUserSetting(KEY_DESKTOP,
                            L"UserPreferencesMask",
                            REG_BINARY,
                            &dwRegMask,
                            sizeof(DWORD));
    }

    return (UINT_PTR)KEY_DESKTOP;
}

static inline
UINT_PTR
SpiGetUserPref(DWORD dwMask, PVOID pvParam, FLONG fl)
{
    INT iValue = gspv.dwUserPrefMask & dwMask ? 1 : 0;
    return SpiGetInt(pvParam, &iValue, fl);
}

static
UINT_PTR
SpiSetWallpaper(PVOID pvParam, FLONG fl)
{
    UNICODE_STRING ustr;
    WCHAR awc[MAX_PATH];
    BOOL bResult;
    HBITMAP hbmp, hOldBitmap;
    SURFACE *psurfBmp;
    ULONG ulTile, ulStyle;

    REQ_INTERACTIVE_WINSTA(ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);

    if (!pvParam)
    {
        /* FIXME: Reset Wallpaper to registry value */
        return (UINT_PTR)KEY_DESKTOP;
    }

    /* Capture UNICODE_STRING */
    bResult = SpiMemCopy(&ustr, pvParam, sizeof(UNICODE_STRING), fl & SPIF_PROTECT, 0);
    if (!bResult) return 0;
    if (ustr.Length > MAX_PATH * sizeof(WCHAR))
        return 0;

    /* Copy the string buffer name */
    bResult = SpiMemCopy(gspv.awcWallpaper, ustr.Buffer, ustr.Length, fl & SPIF_PROTECT, 0);
    if (!bResult) return 0;

    /* Update the UNICODE_STRING */
    gspv.ustrWallpaper.Buffer = gspv.awcWallpaper;
    gspv.ustrWallpaper.MaximumLength = MAX_PATH * sizeof(WCHAR);
    gspv.ustrWallpaper.Length = ustr.Length;
    gspv.awcWallpaper[ustr.Length / sizeof(WCHAR)] = 0;

    DPRINT("SpiSetWallpaper, name=%S\n", gspv.awcWallpaper);

    /* Update registry */
    if (fl & SPIF_UPDATEINIFILE)
    {
        SpiStoreSz(KEY_DESKTOP, L"Wallpaper", gspv.awcWallpaper);
    }

    /* Got a filename? */
    if (gspv.awcWallpaper[0] != 0)
    {
        /* Convert file name to nt file name */
        ustr.Buffer = awc;
        ustr.MaximumLength = MAX_PATH * sizeof(WCHAR);
        ustr.Length = 0;
        if (!W32kDosPathNameToNtPathName(gspv.awcWallpaper, &ustr))
        {
            DPRINT1("RtlDosPathNameToNtPathName_U failed\n");
            return 0;
        }

        /* Load the Bitmap */
        hbmp = UserLoadImage(ustr.Buffer);
        if (!hbmp)
        {
            DPRINT1("UserLoadImage failed\n");
            return 0;
        }

        /* Try to get the size of the wallpaper */
        if(!(psurfBmp = SURFACE_LockSurface(hbmp)))
        {
            GreDeleteObject(hbmp);
            return 0;
        }

        gpwinstaCurrent->cxWallpaper = psurfBmp->SurfObj.sizlBitmap.cx;
        gpwinstaCurrent->cyWallpaper = psurfBmp->SurfObj.sizlBitmap.cy;
        gpwinstaCurrent->WallpaperMode = wmCenter;

        SURFACE_UnlockSurface(psurfBmp);

        /* Change the bitmap's ownership */
        GDIOBJ_SetOwnership(hbmp, NULL);

        /* Yes, Windows really loads the current setting from the registry. */
        ulTile = SpiLoadInt(KEY_DESKTOP, L"TileWallpaper", 0);
        ulStyle = SpiLoadInt(KEY_DESKTOP, L"WallpaperStyle", 0);
        DPRINT("SpiSetWallpaper: ulTile=%ld, ulStyle=%d\n", ulTile, ulStyle);

        /* Check the values we found in the registry */
        if(ulTile && !ulStyle)
        {
            gpwinstaCurrent->WallpaperMode = wmTile;
        }
        else if(!ulTile && ulStyle == 2)
        {
            gpwinstaCurrent->WallpaperMode = wmStretch;
        }
    }
    else
    {
        /* Remove wallpaper */
        gpwinstaCurrent->cxWallpaper = 0;
        gpwinstaCurrent->cyWallpaper = 0;
        hbmp = 0;
    }

    /* Take care of the old wallpaper, if any */
    hOldBitmap = gpwinstaCurrent->hbmWallpaper;
    if(hOldBitmap != NULL)
    {
        /* Delete the old wallpaper */
        GDIOBJ_SetOwnership(hOldBitmap, PsGetCurrentProcess());
        GreDeleteObject(hOldBitmap);
    }

    /* Set the new wallpaper */
    gpwinstaCurrent->hbmWallpaper = hbmp;

    NtUserRedrawWindow(UserGetShellWindow(), NULL, NULL, RDW_INVALIDATE | RDW_ERASE);


    return (UINT_PTR)KEY_DESKTOP;
}

static
UINT_PTR
SpiGetSet(UINT uiAction, UINT uiParam, PVOID pvParam, FLONG fl)
{
    switch (uiAction)
    {
        case SPI_GETBEEP:
            return SpiGetInt(pvParam, &gspv.bBeep, fl);

        case SPI_SETBEEP:
            return SpiSetYesNo(&gspv.bBeep, uiParam, KEY_SOUND, VAL_BEEP, fl);

        case SPI_GETMOUSE:
            return SpiGet(pvParam, &gspv.caiMouse, 3 * sizeof(INT), fl);

        case SPI_SETMOUSE:
            if (!SpiSet(&gspv.caiMouse, pvParam, 3 * sizeof(INT), fl))
                return 0;
            if (fl & SPIF_UPDATEINIFILE)
            {
                SpiStoreSzInt(KEY_MOUSE, VAL_MOUSE1, gspv.caiMouse.FirstThreshold);
                SpiStoreSzInt(KEY_MOUSE, VAL_MOUSE2, gspv.caiMouse.SecondThreshold);
                SpiStoreSzInt(KEY_MOUSE, VAL_MOUSE3, gspv.caiMouse.Acceleration);
            }
            return (UINT_PTR)KEY_MOUSE;

        case SPI_GETBORDER:
            return SpiGetInt(pvParam, &gspv.ncm.iBorderWidth, fl);

        case SPI_SETBORDER:
            uiParam = max(uiParam, 1);
            return SpiSetInt(&gspv.ncm.iBorderWidth, uiParam, KEY_METRIC, VAL_BORDER, fl);

        case SPI_GETKEYBOARDSPEED:
            return SpiGetInt(pvParam, &gspv.dwKbdSpeed, fl);

        case SPI_SETKEYBOARDSPEED:
            return SpiSetInt(&gspv.dwKbdSpeed, uiParam, KEY_KBD, VAL_KBDSPD, fl);

        case SPI_LANGDRIVER:
            DPRINT1("SPI_LANGDRIVER is unimplemented\n");
            break;

        case SPI_GETSCREENSAVETIMEOUT:
            return SpiGetInt(pvParam, &gspv.iScrSaverTimeout, fl);

        case SPI_SETSCREENSAVETIMEOUT:
            return SpiSetInt(&gspv.iScrSaverTimeout, uiParam, KEY_DESKTOP, VAL_SCRTO, fl);

        case SPI_GETSCREENSAVEACTIVE:
            return SpiGetInt(pvParam, &gspv.bScrSaverActive, fl);

        case SPI_SETSCREENSAVEACTIVE:
            return SpiSetInt(&gspv.bScrSaverActive, uiParam, KEY_DESKTOP, VAL_SCRACT, fl);

        case SPI_GETGRIDGRANULARITY:
            return SpiGetInt(pvParam, &gspv.uiGridGranularity, fl);

        case SPI_SETGRIDGRANULARITY:
            return SpiSetInt(&gspv.uiGridGranularity, uiParam, KEY_DESKTOP, VAL_GRID, fl);

        case SPI_GETDESKWALLPAPER:
            uiParam = min(uiParam, gspv.ustrWallpaper.Length + 1);
            return SpiGet(pvParam, gspv.awcWallpaper, uiParam, fl);

        case SPI_SETDESKWALLPAPER:
            return SpiSetWallpaper(pvParam, fl);

        case SPI_SETDESKPATTERN:
            DPRINT1("SPI_SETDESKPATTERN is unimplemented\n");
            break;

        case SPI_GETKEYBOARDDELAY:
            return SpiGetInt(pvParam, &gspv.iKbdDelay, fl);

        case SPI_SETKEYBOARDDELAY:
            return SpiSetInt(&gspv.iKbdDelay, uiParam, KEY_KBD, VAL_KBDDELAY, fl);

        case SPI_ICONHORIZONTALSPACING:
            if (pvParam)
            {
                return SpiGetInt(pvParam, &gspv.im.iHorzSpacing, fl);
            }
            uiParam = max(uiParam, 32);
            return SpiSetMetric(&gspv.im.iHorzSpacing, uiParam, VAL_ICONSPC, fl);

        case SPI_ICONVERTICALSPACING:
            if (pvParam)
            {
                return SpiGetInt(pvParam, &gspv.im.iVertSpacing, fl);
            }
            uiParam = max(uiParam, 32);
            return SpiSetMetric(&gspv.im.iVertSpacing, uiParam, VAL_ICONVSPC, fl);

        case SPI_GETICONTITLEWRAP:
            return SpiGetInt(pvParam, &gspv.im.iTitleWrap, fl);

        case SPI_SETICONTITLEWRAP:
            return SpiSetInt(&gspv.im.iTitleWrap, uiParam, KEY_METRIC, VAL_ITWRAP, fl);

        case SPI_GETMENUDROPALIGNMENT:
            return SpiGetInt(pvParam, &gspv.bMenuDropAlign, fl);

        case SPI_SETMENUDROPALIGNMENT:
            return SpiSetBool(&gspv.bMenuDropAlign, uiParam, KEY_MDALIGN, VAL_MDALIGN, fl);

        case SPI_SETDOUBLECLKWIDTH:
            return SpiSetInt(&gspv.iDblClickWidth, uiParam, KEY_MOUSE, VAL_DBLCLKWIDTH, fl);

        case SPI_SETDOUBLECLKHEIGHT:
            return SpiSetInt(&gspv.iDblClickHeight, uiParam, KEY_MOUSE, VAL_DBLCLKHEIGHT, fl);

        case SPI_GETICONTITLELOGFONT:
            return SpiGet(pvParam, &gspv.im.lfFont, sizeof(LOGFONTW), fl);

        case SPI_SETICONTITLELOGFONT:
            if (!SpiSet(&gspv.im.lfFont, pvParam, sizeof(LOGFONTW), fl))
                return 0;
            if (fl & SPIF_UPDATEINIFILE)
            {
                SpiStoreFont(L"IconFont", &gspv.im.lfFont);
             }
            return (UINT_PTR)KEY_METRIC;

        case SPI_SETDOUBLECLICKTIME:
            return SpiSetInt(&gspv.iDblClickTime, uiParam, KEY_MOUSE, VAL_DBLCLKTIME, fl);

        case SPI_SETMOUSEBUTTONSWAP:
            return SpiSetInt(&gspv.bMouseBtnSwap, uiParam, KEY_MOUSE, VAL_SWAP, fl);

        case SPI_GETFASTTASKSWITCH:
            return SpiGetInt(pvParam, &gspv.bFastTaskSwitch, fl);

        case SPI_SETFASTTASKSWITCH:
            /* According to Winetest this one is unimplemented */
            return 0;

        case SPI_GETDRAGFULLWINDOWS:
            return SpiGetInt(pvParam, &gspv.bDragFullWindows, fl);

        case SPI_SETDRAGFULLWINDOWS:
            return SpiSetInt(&gspv.bDragFullWindows, uiParam, KEY_DESKTOP, VAL_DRAG, fl);

        case SPI_GETNONCLIENTMETRICS:
            return SpiGet(pvParam, &gspv.ncm, sizeof(NONCLIENTMETRICSW), fl);

        case SPI_SETNONCLIENTMETRICS:
            if (!SpiSet(&gspv.ncm, pvParam, sizeof(NONCLIENTMETRICSW), fl))
                return 0;
            if (fl & SPIF_UPDATEINIFILE)
            {
                SpiStoreMetric(L"BorderWidth", gspv.ncm.iBorderWidth);
                SpiStoreMetric(L"ScrollWidth", gspv.ncm.iScrollWidth);
                SpiStoreMetric(L"ScrollHeight", gspv.ncm.iScrollHeight);
                SpiStoreMetric(L"CaptionWidth", gspv.ncm.iCaptionWidth);
                SpiStoreMetric(L"CaptionHeight", gspv.ncm.iCaptionHeight);
                SpiStoreMetric(L"SmCaptionWidth", gspv.ncm.iSmCaptionWidth);
                SpiStoreMetric(L"SmCaptionHeight", gspv.ncm.iSmCaptionHeight);
                SpiStoreMetric(L"MenuWidth", gspv.ncm.iMenuWidth);
                SpiStoreMetric(L"MenuHeight", gspv.ncm.iMenuHeight);
#if (WINVER >= 0x0600)
                SpiStoreMetric(L"PaddedBorderWidth", gspv.ncm.iPaddedBorderWidth);
#endif
                SpiStoreFont(L"CaptionFont", &gspv.ncm.lfCaptionFont);
                SpiStoreFont(L"SmCaptionFont", &gspv.ncm.lfSmCaptionFont);
                SpiStoreFont(L"MenuFont", &gspv.ncm.lfMenuFont);
                SpiStoreFont(L"StatusFont", &gspv.ncm.lfStatusFont);
                SpiStoreFont(L"MessageFont", &gspv.ncm.lfMessageFont);
            }
            return (UINT_PTR)KEY_METRIC;

        case SPI_GETMINIMIZEDMETRICS:
            return SpiGet(pvParam, &gspv.mm, sizeof(MINIMIZEDMETRICS), fl);

        case SPI_SETMINIMIZEDMETRICS:
            if (!SpiSet(&gspv.mm, pvParam, sizeof(MINIMIZEDMETRICS), fl))
                return 0;
            gspv.mm.iWidth = max(0, gspv.mm.iWidth);
            gspv.mm.iHorzGap = max(0, gspv.mm.iHorzGap);
            gspv.mm.iVertGap = max(0, gspv.mm.iVertGap);
            gspv.mm.iArrange = gspv.mm.iArrange & 0xf;
            if (fl & SPIF_UPDATEINIFILE)
            {
                SpiStoreMetric(L"MinWidth", gspv.mm.iWidth);
                SpiStoreMetric(L"MinHorzGap", gspv.mm.iHorzGap);
                SpiStoreMetric(L"MinVertGap", gspv.mm.iVertGap);
                SpiStoreMetric(L"MinArrange", gspv.mm.iArrange);
            }
            return (UINT_PTR)KEY_METRIC;

        case SPI_GETICONMETRICS:
            return SpiGet(pvParam, &gspv.im, sizeof(ICONMETRICS), fl);

        case SPI_SETICONMETRICS:
            if (!SpiSet(&gspv.im, pvParam, sizeof(ICONMETRICS), fl))
                return 0;
            if (fl & SPIF_UPDATEINIFILE)
            {
                SpiStoreMetric(L"IconSpacing", gspv.im.iHorzSpacing);
                SpiStoreMetric(L"IconVerticalspacing", gspv.im.iVertSpacing);
                SpiStoreMetric(L"IconTitleWrap", gspv.im.iTitleWrap);
                SpiStoreFont(L"IconFont", &gspv.im.lfFont);
            }
            return (UINT_PTR)KEY_METRIC;

        case SPI_GETWORKAREA: // FIXME: the workarea should be part of the MONITOR
        {
            PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
            PDESKTOP pdesktop = pti->Desktop;
            RECTL rclWorkarea;

            if(!pdesktop)
                return 0;

            IntGetDesktopWorkArea(pdesktop, &rclWorkarea);
            return SpiGet(pvParam, &rclWorkarea, sizeof(RECTL), fl);
        }

        case SPI_SETWORKAREA: // FIXME: the workarea should be part of the MONITOR
        {
            PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
            PDESKTOP pdesktop = pti->Desktop;
            RECT rcWorkArea;

            if(!pdesktop)
                return 0;

            if (!SpiSet(&rcWorkArea, pvParam, sizeof(RECTL), fl))
                return 0;

            /* Verify the new values */
            if (rcWorkArea.left < 0 ||
                rcWorkArea.top < 0 ||
                rcWorkArea.right > gpsi->SystemMetrics[SM_CXSCREEN] ||
                rcWorkArea.bottom > gpsi->SystemMetrics[SM_CYSCREEN] ||
                rcWorkArea.right <= rcWorkArea.left ||
                rcWorkArea.bottom <= rcWorkArea.top)
                return 0;

            pdesktop->WorkArea = rcWorkArea;
            if (fl & SPIF_UPDATEINIFILE)
            {
                // FIXME: what to do?
            }
            return (UINT_PTR)KEY_DESKTOP;
        }

        case SPI_SETPENWINDOWS:
            DPRINT1("SPI_SETPENWINDOWS is unimplemented\n");
            break;

        case SPI_GETFILTERKEYS:
            return SpiGet(pvParam, &gspv.filterkeys, sizeof(FILTERKEYS), fl);

        case SPI_SETFILTERKEYS:
            if (!SpiSet(&gspv.filterkeys, pvParam, sizeof(FILTERKEYS), fl))
                return 0;
            if (fl & SPIF_UPDATEINIFILE)
            {
                // FIXME: what to do?
            }
            return (UINT_PTR)KEY_DESKTOP;

        case SPI_GETTOGGLEKEYS:
            return SpiGet(pvParam, &gspv.togglekeys, sizeof(TOGGLEKEYS), fl);

        case SPI_SETTOGGLEKEYS:
            if (!SpiSet(&gspv.togglekeys, pvParam, sizeof(TOGGLEKEYS), fl))
                return 0;
            if (fl & SPIF_UPDATEINIFILE)
            {
                // FIXME: what to do?
            }
            return (UINT_PTR)KEY_DESKTOP;

        case SPI_GETMOUSEKEYS:
            return SpiGet(pvParam, &gspv.mousekeys, sizeof(MOUSEKEYS), fl);

        case SPI_SETMOUSEKEYS:
            if (!SpiSet(&gspv.mousekeys, pvParam, sizeof(MOUSEKEYS), fl))
                return 0;
            if (fl & SPIF_UPDATEINIFILE)
            {
                // FIXME: what to do?
            }
            return (UINT_PTR)KEY_DESKTOP;

        case SPI_GETSHOWSOUNDS:
            return SpiGetInt(pvParam, &gspv.bShowSounds, fl);

        case SPI_SETSHOWSOUNDS:
            return SpiSetBool(&gspv.bShowSounds, uiParam, KEY_SHOWSNDS, VAL_ON, fl);

        case SPI_GETSTICKYKEYS:
            if (uiParam != sizeof(STICKYKEYS))
                return 0;
            return SpiGetEx(pvParam, &gspv.stickykeys, sizeof(STICKYKEYS), fl);

        case SPI_SETSTICKYKEYS:
            if (!SpiSet(&gspv.stickykeys, pvParam, sizeof(STICKYKEYS), fl))
                return 0;
            if (fl & SPIF_UPDATEINIFILE)
            {
                // FIXME: what to do?
            }
            return (UINT_PTR)KEY_DESKTOP;

        case SPI_GETACCESSTIMEOUT:
            if (uiParam != 0 && uiParam != sizeof(ACCESSTIMEOUT))
                return 0;
            return SpiGetEx(pvParam, &gspv.accesstimeout, sizeof(ACCESSTIMEOUT), fl);

        case SPI_SETACCESSTIMEOUT:
            if (!SpiSet(&gspv.accesstimeout, pvParam, sizeof(ACCESSTIMEOUT), fl))
                return 0;
            if (fl & SPIF_UPDATEINIFILE)
            {
                // FIXME: what to do?
            }
            return (UINT_PTR)KEY_DESKTOP;

        case SPI_GETSERIALKEYS:
            return SpiGet(pvParam, &gspv.serialkeys, sizeof(SERIALKEYS), fl);

        case SPI_SETSERIALKEYS:
            if (!SpiSet(&gspv.serialkeys, pvParam, sizeof(SERIALKEYS), fl))
                return 0;
            if (fl & SPIF_UPDATEINIFILE)
            {
                // FIXME: what to do?
            }
            return (UINT_PTR)KEY_DESKTOP;

        case SPI_GETSOUNDSENTRY:
            return SpiGet(pvParam, &gspv.soundsentry, sizeof(SOUNDSENTRY), fl);

        case SPI_SETSOUNDSENTRY:
            if (!SpiSet(&gspv.soundsentry, pvParam, sizeof(SOUNDSENTRY), fl))
                return 0;
            if (fl & SPIF_UPDATEINIFILE)
            {
                // FIXME: what to do?
            }
            return (UINT_PTR)KEY_DESKTOP;

        case SPI_GETHIGHCONTRAST:
            return SpiGet(pvParam, &gspv.highcontrast, sizeof(HIGHCONTRAST), fl);

        case SPI_SETHIGHCONTRAST:
            if (!SpiSet(&gspv.highcontrast, pvParam, sizeof(HIGHCONTRAST), fl))
                return 0;
            if (fl & SPIF_UPDATEINIFILE)
            {
                // FIXME: what to do?
            }
            return (UINT_PTR)KEY_DESKTOP;

        case SPI_GETKEYBOARDPREF:
            return SpiGetInt(pvParam, &gspv.bKbdPref, fl);

        case SPI_SETKEYBOARDPREF:
            return SpiSetBool(&gspv.bKbdPref, uiParam, KEY_KDBPREF, VAL_ON, fl);

        case SPI_GETSCREENREADER:
            return SpiGetInt(pvParam, &gspv.bScreenReader, fl);

        case SPI_SETSCREENREADER:
            return SpiSetBool(&gspv.bScreenReader, uiParam, KEY_SCRREAD, VAL_ON, fl);

        case SPI_GETANIMATION:
            return SpiGet(pvParam, &gspv.animationinfo, sizeof(ANIMATIONINFO), fl);

        case SPI_SETANIMATION:
            if (!SpiSet(&gspv.animationinfo, pvParam, sizeof(ANIMATIONINFO), fl))
                return 0;
            if (fl & SPIF_UPDATEINIFILE)
            {
                // FIXME: what to do?
            }
            return (UINT_PTR)KEY_DESKTOP;

        case SPI_GETFONTSMOOTHING:
            return SpiGetInt(pvParam, &gspv.bFontSmoothing, fl);

        case SPI_SETFONTSMOOTHING:
            gspv.bFontSmoothing = uiParam ? TRUE : FALSE;
            if (fl & SPIF_UPDATEINIFILE)
            {
                SpiStoreSzInt(KEY_DESKTOP, VAL_FNTSMOOTH, uiParam ? 2 : 0);
            }
            return (UINT_PTR)KEY_DESKTOP;

        case SPI_SETDRAGWIDTH:
            return SpiSetInt(&gspv.iDragWidth, uiParam, KEY_DESKTOP, VAL_DRAGWIDTH, fl);

        case SPI_SETDRAGHEIGHT:
            return SpiSetInt(&gspv.iDragHeight, uiParam, KEY_DESKTOP, VAL_DRAGHEIGHT, fl);

        case SPI_SETHANDHELD:
            return SpiSetBool(&gspv.bHandHeld, uiParam, KEY_DESKTOP, L"HandHeld", fl);

        case SPI_GETLOWPOWERTIMEOUT:
            return SpiGetInt(pvParam, &gspv.iLowPwrTimeout, fl);

        case SPI_GETPOWEROFFTIMEOUT:
            return SpiGetInt(pvParam, &gspv.iPwrOffTimeout, fl);

        case SPI_SETLOWPOWERTIMEOUT:
            return SpiSetInt(&gspv.iLowPwrTimeout, uiParam, KEY_DESKTOP, L"LowPowerTimeOut", fl);

        case SPI_SETPOWEROFFTIMEOUT:
            return SpiSetInt(&gspv.iPwrOffTimeout, uiParam, KEY_DESKTOP, L"PowerOffTimeOut", fl);

        case SPI_GETLOWPOWERACTIVE:
            return SpiGetInt(pvParam, &gspv.iPwrOffTimeout, fl);

        case SPI_GETPOWEROFFACTIVE:
            return SpiGetInt(pvParam, &gspv.bPwrOffActive, fl);

        case SPI_SETLOWPOWERACTIVE:
            return SpiSetBool(&gspv.bLowPwrActive, uiParam, KEY_DESKTOP, L"LowPowerActive", fl);

        case SPI_SETPOWEROFFACTIVE:
            return SpiSetBool(&gspv.bPwrOffActive, uiParam, KEY_DESKTOP, L"PowerOffActive", fl);

        case SPI_SETCURSORS:
            DPRINT1("SPI_SETCURSORS is unimplemented\n");
            break;

        case SPI_SETICONS:
            DPRINT1("SPI_SETICONS is unimplemented\n");
            break;

        case SPI_GETDEFAULTINPUTLANG:
            DPRINT1("SPI_GETDEFAULTINPUTLANG is unimplemented\n");
            break;

        case SPI_SETDEFAULTINPUTLANG:
            DPRINT1("SPI_SETDEFAULTINPUTLANG is unimplemented\n");
            break;

        case SPI_SETLANGTOGGLE:
            DPRINT1("SPI_SETLANGTOGGLE is unimplemented\n");
            break;

        case SPI_GETWINDOWSEXTENSION:
            DPRINT1("SPI_GETWINDOWSEXTENSION is unimplemented\n");
            break;

        case SPI_GETMOUSETRAILS:
            return SpiGetInt(pvParam, &gspv.iMouseTrails, fl);

        case SPI_SETMOUSETRAILS:
            return SpiSetInt(&gspv.iMouseTrails, uiParam, KEY_MOUSE, L"MouseTrails", fl);

        case SPI_GETSNAPTODEFBUTTON:
            return SpiGetInt(pvParam, &gspv.bSnapToDefBtn, fl);

        case SPI_SETSNAPTODEFBUTTON:
            return SpiSetBool(&gspv.bSnapToDefBtn, uiParam, KEY_MOUSE, L"SnapToDefaultButton", fl);

        case SPI_GETMOUSEHOVERWIDTH:
            return SpiGetInt(pvParam, &gspv.iMouseHoverWidth, fl);

        case SPI_SETMOUSEHOVERWIDTH:
            return SpiSetInt(&gspv.iMouseHoverWidth, uiParam, KEY_MOUSE, L"MouseHoverWidth", fl);

        case SPI_GETMOUSEHOVERHEIGHT:
            return SpiGetInt(pvParam, &gspv.iMouseHoverHeight, fl);

        case SPI_SETMOUSEHOVERHEIGHT:
            return SpiSetInt(&gspv.iMouseHoverHeight, uiParam, KEY_MOUSE, L"MouseHoverHeight", fl);

        case SPI_GETMOUSEHOVERTIME:
            return SpiGetInt(pvParam, &gspv.iMouseHoverTime, fl);

        case SPI_SETMOUSEHOVERTIME:
           /* see http://msdn2.microsoft.com/en-us/library/ms724947.aspx
            * copy text from it, if some agument why xp and 2003 behovir diffent
            * only if they do not have SP install
            * " Windows Server 2003 and Windows XP: The operating system does not
            *   enforce the use of USER_TIMER_MAXIMUM and USER_TIMER_MINIMUM until
            *   Windows Server 2003 SP1 and Windows XP SP2 "
            */
            return SpiSetInt(&gspv.iMouseHoverTime, uiParam, KEY_MOUSE, L"MouseHoverTime", fl);

        case SPI_GETWHEELSCROLLLINES:
            return SpiGetInt(pvParam, &gspv.iWheelScrollLines, fl);

        case SPI_SETWHEELSCROLLLINES:
            return SpiSetInt(&gspv.iWheelScrollLines, uiParam, KEY_DESKTOP, L"WheelScrollLines", fl);

        case SPI_GETMENUSHOWDELAY:
            return SpiGetInt(pvParam, &gspv.dwMenuShowDelay, fl);

        case SPI_SETMENUSHOWDELAY:
            return SpiSetInt(&gspv.dwMenuShowDelay, uiParam, KEY_DESKTOP, L"MenuShowDelay", fl);

#if (_WIN32_WINNT >= 0x0600)
        case SPI_GETWHEELSCROLLCHARS:
            return SpiGetInt(pvParam, &gspv.uiWheelScrollChars, fl);

        case SPI_SETWHEELSCROLLCHARS:
            return SpiSetInt(&gspv.uiWheelScrollChars, uiParam, KEY_DESKTOP, L"WheelScrollChars", fl);
#endif
        case SPI_GETSHOWIMEUI:
            return SpiGetInt(pvParam, &gspv.bShowImeUi, fl);

        case SPI_SETSHOWIMEUI:
            return SpiSetBool(&gspv.bShowImeUi, uiParam, KEY_DESKTOP, L"", fl);

        case SPI_GETMOUSESPEED:
            return SpiGetInt(pvParam, &gspv.iMouseSpeed, fl);

        case SPI_SETMOUSESPEED:
            // vgl SETMOUSE
            return SpiSetInt(&gspv.iMouseSpeed, uiParam, KEY_MOUSE, L"MouseSpeed", fl);

        case SPI_GETSCREENSAVERRUNNING:
            return SpiGetInt(pvParam, &gspv.bScrSaverRunning, fl);

        case SPI_SETSCREENSAVERRUNNING:
            // FIXME: also return value?
            return SpiSetBool(&gspv.bScrSaverRunning, uiParam, KEY_MOUSE, L"", fl);

#if(WINVER >= 0x0600)
        case SPI_GETAUDIODESCRIPTION:
            return SpiGet(pvParam, &gspv.audiodesription, sizeof(AUDIODESCRIPTION), fl);

        case SPI_SETAUDIODESCRIPTION:
            DPRINT1("SPI_SETAUDIODESCRIPTION is unimplemented\n");
            break;

        case SPI_GETSCREENSAVESECURE:
            return SpiGetInt(pvParam, &gspv.bScrSaverSecure, fl);

        case SPI_SETSCREENSAVESECURE:
            return SpiSetBool(&gspv.bScrSaverSecure, uiParam, KEY_DESKTOP, L"ScreenSaverIsSecure", fl);
#endif

        case SPI_GETACTIVEWINDOWTRACKING:
            return SpiGetUserPref(UPM_ACTIVEWINDOWTRACKING, pvParam, fl);

        case SPI_SETACTIVEWINDOWTRACKING:
            return SpiSetUserPref(UPM_ACTIVEWINDOWTRACKING, pvParam, fl);

        case SPI_GETMENUANIMATION:
            return SpiGetUserPref(UPM_MENUANIMATION, pvParam, fl);

        case SPI_SETMENUANIMATION:
            return SpiSetUserPref(UPM_MENUANIMATION, pvParam, fl);

        case SPI_GETCOMBOBOXANIMATION:
            return SpiGetUserPref(UPM_COMBOBOXANIMATION, pvParam, fl);

        case SPI_SETCOMBOBOXANIMATION:
            return SpiSetUserPref(UPM_COMBOBOXANIMATION, pvParam, fl);

        case SPI_GETLISTBOXSMOOTHSCROLLING:
            return SpiGetUserPref(UPM_LISTBOXSMOOTHSCROLLING, pvParam, fl);

        case SPI_SETLISTBOXSMOOTHSCROLLING:
            return SpiSetUserPref(UPM_LISTBOXSMOOTHSCROLLING, pvParam, fl);

        case SPI_GETGRADIENTCAPTIONS:
            return SpiGetUserPref(UPM_GRADIENTCAPTIONS, pvParam, fl);

        case SPI_SETGRADIENTCAPTIONS:
            return SpiSetUserPref(UPM_GRADIENTCAPTIONS, pvParam, fl);

        case SPI_GETKEYBOARDCUES:
            return SpiGetUserPref(UPM_KEYBOARDCUES, pvParam, fl);

        case SPI_SETKEYBOARDCUES:
            return SpiSetUserPref(UPM_KEYBOARDCUES, pvParam, fl);

        case SPI_GETACTIVEWNDTRKZORDER:
            return SpiGetUserPref(UPM_ACTIVEWNDTRKZORDER, pvParam, fl);

        case SPI_SETACTIVEWNDTRKZORDER:
            return SpiSetUserPref(UPM_ACTIVEWNDTRKZORDER, pvParam, fl);

        case SPI_GETHOTTRACKING:
            return SpiGetUserPref(UPM_HOTTRACKING, pvParam, fl);

        case SPI_SETHOTTRACKING:
            return SpiSetUserPref(UPM_HOTTRACKING, pvParam, fl);

        case SPI_GETMENUFADE:
            return SpiGetUserPref(UPM_MENUFADE, pvParam, fl);

        case SPI_SETMENUFADE:
            return SpiSetUserPref(UPM_MENUFADE, pvParam, fl);

        case SPI_GETSELECTIONFADE:
            return SpiGetUserPref(UPM_SELECTIONFADE, pvParam, fl);

        case SPI_SETSELECTIONFADE:
            return SpiSetUserPref(UPM_SELECTIONFADE, pvParam, fl);

        case SPI_GETTOOLTIPANIMATION:
            return SpiGetUserPref(UPM_TOOLTIPANIMATION, pvParam, fl);

        case SPI_SETTOOLTIPANIMATION:
            return SpiSetUserPref(UPM_TOOLTIPANIMATION, pvParam, fl);

        case SPI_GETTOOLTIPFADE:
            return SpiGetUserPref(UPM_TOOLTIPFADE, pvParam, fl);

        case SPI_SETTOOLTIPFADE:
            return SpiSetUserPref(UPM_TOOLTIPFADE, pvParam, fl);

        case SPI_GETCURSORSHADOW:
            return SpiGetUserPref(UPM_CURSORSHADOW, pvParam, fl);

        case SPI_SETCURSORSHADOW:
            return SpiSetUserPref(UPM_CURSORSHADOW, pvParam, fl);

        case SPI_GETUIEFFECTS:
            return SpiGetUserPref(UPM_UIEFFECTS, pvParam, fl);

        case SPI_SETUIEFFECTS:
            return SpiSetUserPref(UPM_UIEFFECTS, pvParam, fl);

        case SPI_GETMOUSESONAR:
            return SpiGetInt(pvParam, &gspv.bMouseSonar, fl);

        case SPI_SETMOUSESONAR:
            return SpiSetBool(&gspv.bMouseSonar, uiParam, KEY_MOUSE, L"", fl);

        case SPI_GETMOUSECLICKLOCK:
            return SpiGetInt(pvParam, &gspv.bMouseClickLock, fl);

        case SPI_SETMOUSECLICKLOCK:
            return SpiSetBool(&gspv.bMouseClickLock, uiParam, KEY_MOUSE, L"", fl);

        case SPI_GETMOUSEVANISH:
            return SpiGetInt(pvParam, &gspv.bMouseVanish, fl);

        case SPI_SETMOUSEVANISH:
            return SpiSetBool(&gspv.bMouseVanish, uiParam, KEY_MOUSE, L"", fl);

        case SPI_GETFLATMENU:
            return SpiGetInt(pvParam, &gspv.bFlatMenu, fl);

        case SPI_SETFLATMENU:
            return SpiSetBool(&gspv.bFlatMenu, uiParam, KEY_MOUSE, L"", fl);

        case SPI_GETDROPSHADOW:
            return SpiGetInt(pvParam, &gspv.bDropShadow, fl);

        case SPI_SETDROPSHADOW:
            return SpiSetBool(&gspv.bDropShadow, uiParam, KEY_MOUSE, L"", fl);

        case SPI_GETBLOCKSENDINPUTRESETS:
            return SpiGetInt(pvParam, &gspv.bBlockSendInputResets, fl);

        case SPI_SETBLOCKSENDINPUTRESETS:
            return SpiSetBool(&gspv.bBlockSendInputResets, uiParam, KEY_MOUSE, L"", fl);

#if(_WIN32_WINNT >= 0x0600)
        case SPI_GETDISABLEOVERLAPPEDCONTENT:
            return SpiGetInt(pvParam, &gspv.bDisableOverlappedContent, fl);

        case SPI_SETDISABLEOVERLAPPEDCONTENT:
            return SpiSetBool(&gspv.bDisableOverlappedContent, uiParam, KEY_MOUSE, L"", fl);

        case SPI_GETCLIENTAREAANIMATION:
            return SpiGetInt(pvParam, &gspv.bClientAnimation, fl);

        case SPI_SETCLIENTAREAANIMATION:
            return SpiSetBool(&gspv.bClientAnimation, uiParam, KEY_MOUSE, L"", fl);

        case SPI_GETCLEARTYPE:
            return SpiGetInt(pvParam, &gspv.bClearType, fl);

        case SPI_SETCLEARTYPE:
            return SpiSetBool(&gspv.bClearType, uiParam, KEY_MOUSE, L"", fl);

        case SPI_GETSPEECHRECOGNITION:
            return SpiGetInt(pvParam, &gspv.bSpeechRecognition, fl);

        case SPI_SETSPEECHRECOGNITION:
            return SpiSetBool(&gspv.bSpeechRecognition, uiParam, KEY_MOUSE, L"", fl);
#endif

        case SPI_GETFOREGROUNDLOCKTIMEOUT:
            return SpiGetInt(pvParam, &gspv.dwForegroundLockTimeout, fl);

        case SPI_SETFOREGROUNDLOCKTIMEOUT:
            return SpiSetInt(&gspv.dwForegroundLockTimeout, uiParam, KEY_MOUSE, L"", fl);

        case SPI_GETACTIVEWNDTRKTIMEOUT:
            return SpiGetInt(pvParam, &gspv.dwActiveTrackingTimeout, fl);

        case SPI_SETACTIVEWNDTRKTIMEOUT:
            return SpiSetInt(&gspv.dwActiveTrackingTimeout, uiParam, KEY_MOUSE, L"", fl);

        case SPI_GETFOREGROUNDFLASHCOUNT:
            return SpiGetInt(pvParam, &gspv.dwForegroundFlashCount, fl);

        case SPI_SETFOREGROUNDFLASHCOUNT:
            return SpiSetInt(&gspv.dwForegroundFlashCount, uiParam, KEY_MOUSE, L"", fl);

        case SPI_GETCARETWIDTH:
            return SpiGetInt(pvParam, &gspv.dwCaretWidth, fl);

        case SPI_SETCARETWIDTH:
            return SpiSetInt(&gspv.dwCaretWidth, uiParam, KEY_MOUSE, L"", fl);

        case SPI_GETMOUSECLICKLOCKTIME:
            return SpiGetInt(pvParam, &gspv.dwMouseClickLockTime, fl);

        case SPI_SETMOUSECLICKLOCKTIME:
            return SpiSetInt(&gspv.dwMouseClickLockTime, uiParam, KEY_MOUSE, L"", fl);

        case SPI_GETFONTSMOOTHINGTYPE:
            return SpiGetInt(pvParam, &gspv.uiFontSmoothingType, fl);

        case SPI_SETFONTSMOOTHINGTYPE:
            return SpiSetInt(&gspv.uiFontSmoothingType, uiParam, KEY_MOUSE, L"", fl);

        case SPI_GETFONTSMOOTHINGCONTRAST:
            return SpiGetInt(pvParam, &gspv.uiFontSmoothingContrast, fl);

        case SPI_SETFONTSMOOTHINGCONTRAST:
            return SpiSetInt(&gspv.uiFontSmoothingContrast, uiParam, KEY_MOUSE, L"", fl);

        case SPI_GETFOCUSBORDERWIDTH:
            return SpiGetInt(pvParam, &gspv.uiFocusBorderWidth, fl);

        case SPI_SETFOCUSBORDERWIDTH:
            return SpiSetInt(&gspv.uiFocusBorderWidth, uiParam, KEY_MOUSE, L"", fl);

        case SPI_GETFOCUSBORDERHEIGHT:
            return SpiGetInt(pvParam, &gspv.uiFocusBorderHeight, fl);

        case SPI_SETFOCUSBORDERHEIGHT:
            return SpiSetInt(&gspv.uiFocusBorderHeight, uiParam, KEY_MOUSE, L"", fl);

        case SPI_GETFONTSMOOTHINGORIENTATION:
            return SpiGetInt(pvParam, &gspv.uiFontSmoothingOrientation, fl);

        case SPI_SETFONTSMOOTHINGORIENTATION:
            return SpiSetInt(&gspv.uiFontSmoothingOrientation, uiParam, KEY_MOUSE, L"", fl);

        /* The following are undocumented, but valid SPI values */
        case 0x1010:
        case 0x1011:
        case 0x1028:
        case 0x1029:
        case 0x102A:
        case 0x102B:
        case 0x102C:
        case 0x102D:
        case 0x102E:
        case 0x102F:
        case 0x1030:
        case 0x1031:
        case 0x1032:
        case 0x1033:
        case 0x1034:
        case 0x1035:
        case 0x1036:
        case 0x1037:
        case 0x1038:
        case 0x1039:
        case 0x103A:
        case 0x103B:
        case 0x103C:
        case 0x103D:
            DPRINT1("Undocumented SPI value %x is unimplemented\n", uiAction);
            break;

        default:
            DPRINT1("Invalid SPI value: %d\n", uiAction);
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            return 0;
    }

    return 0;
}

BOOL
FASTCALL
UserSystemParametersInfo(
    UINT uiAction,
    UINT uiParam,
    PVOID pvParam,
    UINT fWinIni)
{
    ULONG_PTR ulResult;

    if (!gbSpiInitialized)
    {
        KeRosDumpStackFrames(NULL, 20);
        return FALSE;
    }

    /* Get a pointer to the current Windowstation */
    gpwinstaCurrent = IntGetWinStaObj();

    if (!gpwinstaCurrent)
    {
        DPRINT1("UserSystemParametersInfo called without active windowstation.\n");
        //KeRosDumpStackFrames(NULL, 0);
    }

    /* Do the actual operation */
    ulResult = SpiGetSet(uiAction, uiParam, pvParam, fWinIni);

    /* Did we change something? */
    if (ulResult > 1)
    {
        SpiFixupValues();

        /* Update system metrics */
        InitMetrics();

        /* Send notification to toplevel windows, if requested */
        if (fWinIni & (SPIF_SENDCHANGE | SPIF_SENDWININICHANGE))
        {
            /* Send WM_SETTINGCHANGE to all toplevel windows */
            co_IntSendMessageTimeout(HWND_BROADCAST,
                                     WM_SETTINGCHANGE,
                                     (WPARAM)uiAction,
                                     (LPARAM)ulResult,
                                     SMTO_NORMAL,
                                     100,
                                     &ulResult);
        }
        ulResult = 1;
    }

    /* Dereference the windowstation */
    if (gpwinstaCurrent)
    {
        ObDereferenceObject(gpwinstaCurrent);
        gpwinstaCurrent = NULL;
    }

    return ulResult;
}

BOOL
APIENTRY
NtUserSystemParametersInfo(
    UINT uiAction,
    UINT uiParam,
    PVOID pvParam,
    UINT fWinIni)
{
    BOOL bResult;

    DPRINT("Enter NtUserSystemParametersInfo(%d)\n", uiAction);
    UserEnterExclusive();

    //if (uiAction == SPI_SETMOUSE) gbDebug = 1;

    // FIXME: get rid of the flags and only use this from um. kernel can access data directly.
    /* Set UM memory protection flag */
    fWinIni |= SPIF_PROTECT;

    /* Call internal function */
    bResult = UserSystemParametersInfo(uiAction, uiParam, pvParam, fWinIni);

    //DPRINTX("NtUserSystemParametersInfo SPI_ICONHORIZONTALSPACING uiParam=%d, pvParam=%p, pvParam=%d bResult=%d\n",
    //         uiParam, pvParam, pvParam?*(UINT*)pvParam:0, bResult);

    DPRINT("Leave NtUserSystemParametersInfo, returning %d\n", bResult);
    UserLeave();

    //DPRINTX("NtUserSystemParametersInfo SPI_ICONHORIZONTALSPACING bResult=%d\n", bResult);
    //gbDebug = 0;

    return bResult;
}
