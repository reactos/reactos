/*
 * COPYRIGHT:        GPL, see COPYING in the top level directory
 * PROJECT:          ReactOS win32 kernel mode subsystem server
 * PURPOSE:          System parameters functions
 * FILE:             win32ss/user/ntuser/sysparams.c
 * PROGRAMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

// TODO:
// - Check all values that are in Winsta in ROS.
// - Does setting invalid fonts work?
// - Save appropriate text metrics.

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserSysparams);

SPIVALUES gspv;
BOOL gbSpiInitialized = FALSE;
BOOL g_PaintDesktopVersion = FALSE;

// HACK! We initialize SPI before we have a proper surface to get this from.
#define dpi 96
//(pPrimarySurface->GDIInfo.ulLogPixelsY)
#define REG2METRIC(reg) (reg > 0 ? reg : ((-(reg) * dpi + 720) / 1440))
#define METRIC2REG(met) (-((((met) * 1440)- 0) / dpi))

#define REQ_INTERACTIVE_WINSTA(err) \
do { \
    if (GetW32ProcessInfo()->prpwinsta != InputWindowStation) \
    { \
        if (GetW32ProcessInfo()->prpwinsta == NULL) \
        { \
            ERR("NtUserSystemParametersInfo called without active window station, and it requires an interactive one\n"); \
        } \
        else \
        { \
            ERR("NtUserSystemParametersInfo requires interactive window station (current is '%wZ')\n", \
                &(OBJECT_HEADER_TO_NAME_INFO(OBJECT_TO_OBJECT_HEADER(GetW32ProcessInfo()->prpwinsta))->Name)); \
        } \
        EngSetLastError(err); \
        return 0; \
    } \
} while (0)

static const WCHAR* KEY_MOUSE = L"Control Panel\\Mouse";
static const WCHAR* VAL_MOUSE1 = L"MouseThreshold1";
static const WCHAR* VAL_MOUSE2 = L"MouseThreshold2";
static const WCHAR* VAL_MOUSE3 = L"MouseSpeed";
static const WCHAR* VAL_MOUSETRAILS = L"MouseTrails";
static const WCHAR* VAL_DBLCLKWIDTH = L"DoubleClickWidth";
static const WCHAR* VAL_DBLCLKHEIGHT = L"DoubleClickHeight";
static const WCHAR* VAL_DBLCLKTIME = L"DoubleClickSpeed";
static const WCHAR* VAL_SNAPDEFBTN = L"SnapToDefaultButton";
static const WCHAR* VAL_SWAP = L"SwapMouseButtons";
static const WCHAR* VAL_HOVERTIME = L"MouseHoverTime";
static const WCHAR* VAL_HOVERWIDTH = L"MouseHoverWidth";
static const WCHAR* VAL_HOVERHEIGHT = L"MouseHoverHeight";
static const WCHAR* VAL_SENSITIVITY = L"MouseSensitivity";

static const WCHAR* KEY_DESKTOP = L"Control Panel\\Desktop";
static const WCHAR* VAL_SCRTO = L"ScreenSaveTimeOut";
static const WCHAR* VAL_SCRNSV = L"SCRNSAVE.EXE";
static const WCHAR* VAL_SCRACT = L"ScreenSaveActive";
static const WCHAR* VAL_GRID = L"GridGranularity";
static const WCHAR* VAL_DRAG = L"DragFullWindows";
static const WCHAR* VAL_DRAGHEIGHT = L"DragHeight";
static const WCHAR* VAL_DRAGWIDTH = L"DragWidth";
static const WCHAR* VAL_FONTSMOOTHING = L"FontSmoothing";
static const WCHAR* VAL_FONTSMOOTHINGTYPE = L"FontSmoothingType";
static const WCHAR* VAL_FONTSMOOTHINGCONTRAST = L"FontSmoothingGamma";
static const WCHAR* VAL_FONTSMOOTHINGORIENTATION = L"FontSmoothingOrientation";
static const WCHAR* VAL_SCRLLLINES = L"WheelScrollLines";
static const WCHAR* VAL_CLICKLOCKTIME = L"ClickLockTime";
static const WCHAR* VAL_PAINTDESKVER = L"PaintDesktopVersion";
static const WCHAR* VAL_CARETRATE = L"CursorBlinkRate";
#if (_WIN32_WINNT >= 0x0600)
static const WCHAR* VAL_SCRLLCHARS = L"WheelScrollChars";
#endif
static const WCHAR* VAL_USERPREFMASK = L"UserPreferencesMask";

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
SpiLoadDWord(PCWSTR pwszKey, PCWSTR pwszValue, INT iValue)
{
    DWORD Result;
    if (!RegReadUserSetting(pwszKey, pwszValue, REG_DWORD, &Result, sizeof(Result)))
    {
        return iValue;
    }
    return Result;
}

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
DWORD
SpiLoadUserPrefMask(DWORD dValue)
{
    DWORD Result;
    if (!RegReadUserSetting(KEY_DESKTOP, VAL_USERPREFMASK, REG_BINARY, &Result, sizeof(Result)))
    {
        return dValue;
    }
    return Result;
}

static
DWORD
SpiLoadTimeOut(VOID)
{   // Must have the string!
    WCHAR szApplicationName[MAX_PATH];
    RtlZeroMemory(&szApplicationName, sizeof(szApplicationName));
    if (!RegReadUserSetting(KEY_DESKTOP, VAL_SCRNSV, REG_SZ, &szApplicationName, sizeof(szApplicationName)))
    {
        return 0;
    }
    if (szApplicationName[0] == 0) return 0;
    return SpiLoadInt(KEY_DESKTOP, VAL_SCRTO, 600);
}

static
INT
SpiLoadMouse(PCWSTR pwszValue, INT iValue)
{
    return SpiLoadInt(KEY_MOUSE, pwszValue, iValue);
}

static
INT
SpiLoadMetric(PCWSTR pwszValue, INT iValue)
{
    INT iRegVal;

    iRegVal = SpiLoadInt(KEY_METRIC, pwszValue, METRIC2REG(iValue));
    TRACE("Loaded metric setting '%S', iValue=%d(reg:%d), ret=%d(reg:%d)\n",
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
SpiFixupValues(VOID)
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

    // FIXME: Hack!!!
    gspv.tmMenuFont.tmHeight = 11;
    gspv.tmMenuFont.tmExternalLeading = 2;

    gspv.tmCaptionFont.tmHeight = 11;
    gspv.tmCaptionFont.tmExternalLeading = 2;

}

static
VOID
SpiUpdatePerUserSystemParameters(VOID)
{
    static LOGFONTW lf1 = {-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE,
                           FALSE, ANSI_CHARSET, 0, 0, DEFAULT_QUALITY,
                           VARIABLE_PITCH | FF_SWISS, L"MS Sans Serif"};
    static LOGFONTW lf2 = {-11, 0, 0, 0, FW_BOLD, FALSE, FALSE,
                           FALSE, ANSI_CHARSET, 0, 0, DEFAULT_QUALITY,
                           VARIABLE_PITCH | FF_SWISS, L"MS Sans Serif"};

    TRACE("Enter SpiUpdatePerUserSystemParameters\n");

    /* Clear the structure */
    RtlZeroMemory(&gspv, sizeof(gspv));

    /* Load mouse settings */
    gspv.caiMouse.FirstThreshold = SpiLoadMouse(VAL_MOUSE1, 6);
    gspv.caiMouse.SecondThreshold = SpiLoadMouse(VAL_MOUSE2, 10);
    gspv.caiMouse.Acceleration = SpiLoadMouse(VAL_MOUSE3, 1);
    gspv.iMouseSpeed = SpiLoadMouse(VAL_SENSITIVITY, 10);
    gspv.bMouseBtnSwap = SpiLoadMouse(VAL_SWAP, 0);
    gspv.bSnapToDefBtn = SpiLoadMouse(VAL_SNAPDEFBTN, 0);
    gspv.iMouseTrails = SpiLoadMouse(VAL_MOUSETRAILS, 0);
    gspv.iDblClickTime = SpiLoadMouse(VAL_DBLCLKTIME, 500);
    gspv.iDblClickWidth = SpiLoadMouse(VAL_DBLCLKWIDTH, 4);
    gspv.iDblClickHeight = SpiLoadMouse(VAL_DBLCLKHEIGHT, 4);
    gspv.iMouseHoverTime = SpiLoadMouse(VAL_HOVERTIME, 400);
    gspv.iMouseHoverWidth = SpiLoadMouse(VAL_HOVERWIDTH, 4);
    gspv.iMouseHoverHeight = SpiLoadMouse(VAL_HOVERHEIGHT, 4);

    /* Load keyboard settings */
    gspv.dwKbdSpeed = SpiLoadInt(KEY_KBD, VAL_KBDSPD, 31);
    gspv.iKbdDelay = SpiLoadInt(KEY_KBD, VAL_KBDDELAY, 1);

    /* Load NONCLIENTMETRICS */
    gspv.ncm.cbSize = sizeof(NONCLIENTMETRICSW);
    gspv.ncm.iBorderWidth = SpiLoadMetric(VAL_BORDER, 1);
    gspv.ncm.iScrollWidth = SpiLoadMetric(L"ScrollWidth", 16);
    gspv.ncm.iScrollHeight = SpiLoadMetric(L"ScrollHeight", 16);
    gspv.ncm.iCaptionWidth = SpiLoadMetric(L"CaptionWidth", 19);
    gspv.ncm.iCaptionHeight = SpiLoadMetric(L"CaptionHeight", 19);
    gspv.ncm.iSmCaptionWidth = SpiLoadMetric(L"SmCaptionWidth", 12);
    gspv.ncm.iSmCaptionHeight = SpiLoadMetric(L"SmCaptionHeight", 15);
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
    gspv.im.iHorzSpacing = SpiLoadMetric(VAL_ICONSPC, 64);
    gspv.im.iVertSpacing = SpiLoadMetric(VAL_ICONVSPC, 64);
    gspv.im.iTitleWrap = SpiLoadMetric(VAL_ITWRAP, 1);
    SpiLoadFont(&gspv.im.lfFont, L"IconFont", &lf1);

    /* Load desktop settings */
    gspv.bDragFullWindows = SpiLoadInt(KEY_DESKTOP, VAL_DRAG, 0);
    gspv.iWheelScrollLines = SpiLoadInt(KEY_DESKTOP, VAL_SCRLLLINES, 3);
    gspv.dwMouseClickLockTime = SpiLoadDWord(KEY_DESKTOP, VAL_CLICKLOCKTIME, 1200);
    gpsi->dtCaretBlink = SpiLoadInt(KEY_DESKTOP, VAL_CARETRATE, 530);
    gspv.dwUserPrefMask = SpiLoadUserPrefMask(UPM_DEFAULT);
    gspv.bMouseClickLock = (gspv.dwUserPrefMask & UPM_CLICKLOCK) != 0;
    gspv.bMouseCursorShadow = (gspv.dwUserPrefMask & UPM_CURSORSHADOW) != 0;
    gspv.bFontSmoothing = SpiLoadInt(KEY_DESKTOP, VAL_FONTSMOOTHING, 0) == 2;
    gspv.uiFontSmoothingType = SpiLoadDWord(KEY_DESKTOP, VAL_FONTSMOOTHINGTYPE, 1);
    gspv.uiFontSmoothingContrast = SpiLoadDWord(KEY_DESKTOP, VAL_FONTSMOOTHINGCONTRAST, 1400);
    gspv.uiFontSmoothingOrientation = SpiLoadDWord(KEY_DESKTOP, VAL_FONTSMOOTHINGORIENTATION, 1);
#if (_WIN32_WINNT >= 0x0600)
    gspv.uiWheelScrollChars = SpiLoadInt(KEY_DESKTOP, VAL_SCRLLCHARS, 3);
#endif

    /* Some hardcoded values for now */

    gspv.tmCaptionFont.tmAveCharWidth = 6;
    gspv.bBeep = TRUE;
    gspv.uiFocusBorderWidth = 1;
    gspv.uiFocusBorderHeight = 1;
    gspv.bMenuDropAlign = 0;
    gspv.dwMenuShowDelay = SpiLoadInt(KEY_DESKTOP, L"MenuShowDelay", 400);
    gspv.dwForegroundFlashCount = 3;

    gspv.iScrSaverTimeout = SpiLoadTimeOut();
    gspv.bScrSaverActive = FALSE;
    gspv.bScrSaverRunning = FALSE;
#if(WINVER >= 0x0600)
    gspv.bScrSaverSecure = FALSE;
#endif

    gspv.bFastTaskSwitch = TRUE;

    gspv.accesstimeout.cbSize = sizeof(ACCESSTIMEOUT);
    gspv.filterkeys.cbSize = sizeof(FILTERKEYS);
    gspv.togglekeys.cbSize = sizeof(TOGGLEKEYS);
    gspv.mousekeys.cbSize = sizeof(MOUSEKEYS);
    gspv.stickykeys.cbSize = sizeof(STICKYKEYS);
    gspv.serialkeys.cbSize = sizeof(SERIALKEYS);
    gspv.soundsentry.cbSize = sizeof(SOUNDSENTRYW);
    gspv.highcontrast.cbSize = sizeof(HIGHCONTRASTW);
    gspv.animationinfo.cbSize = sizeof(ANIMATIONINFO);

    /* Make sure we don't use broken values */
    SpiFixupValues();

    /* Update SystemMetrics */
    InitMetrics();

    if (gbSpiInitialized && gpsi)
    {
       if (gspv.bKbdPref) gpsi->dwSRVIFlags |= SRVINFO_KBDPREF;
       if (SPITESTPREF(UPM_KEYBOARDCUES)) gpsi->PUSIFlags |= PUSIF_KEYBOARDCUES;
       if (SPITESTPREF(UPM_COMBOBOXANIMATION)) gpsi->PUSIFlags |= PUSIF_COMBOBOXANIMATION;
       if (SPITESTPREF(UPM_LISTBOXSMOOTHSCROLLING)) gpsi->PUSIFlags |= PUSIF_LISTBOXSMOOTHSCROLLING;
    }
    gdwLanguageToggleKey = UserGetLanguageToggle();
}

BOOL
InitSysParams(VOID)
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

    TRACE("Enter NtUserUpdatePerUserSystemParameters\n");
    UserEnterExclusive();

    SpiUpdatePerUserSystemParameters();
    if(bEnable)
        g_PaintDesktopVersion = SpiLoadDWord(KEY_DESKTOP, VAL_PAINTDESKVER, 0);
    else
        g_PaintDesktopVersion = FALSE;
    bResult = TRUE;

    TRACE("Leave NtUserUpdatePerUserSystemParameters, returning %d\n", bResult);
    UserLeave();

    return bResult;
}


/** Storing the settings ******************************************************/

static
VOID
SpiStoreDWord(PCWSTR pwszKey, PCWSTR pwszValue, DWORD Value)
{
    RegWriteUserSetting(pwszKey,
                        pwszValue,
                        REG_DWORD,
                        &Value,
                        sizeof(Value));
}

static
VOID
SpiStoreSz(PCWSTR pwszKey, PCWSTR pwszValue, PCWSTR pwsz)
{
    RegWriteUserSetting(pwszKey,
                        pwszValue,
                        REG_SZ,
                        pwsz,
                        (wcslen(pwsz) + 1) * sizeof(WCHAR));
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
SpiMemCopy(PVOID pvDst, PVOID pvSrc, ULONG cbSize, BOOL bProtect)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (bProtect)
    {
        _SEH2_TRY
        {
            RtlCopyMemory(pvDst, pvSrc, cbSize);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }
    else
    {
        RtlCopyMemory(pvDst, pvSrc, cbSize);
    }

    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        ERR("SpiMemCopy failed, pvDst=%p, pvSrc=%p, bProtect=%d\n", pvDst, pvSrc, bProtect);
    }

    return NT_SUCCESS(Status);
}

static inline
UINT_PTR
SpiGet(PVOID pvParam, PVOID pvData, ULONG cbSize, FLONG fl)
{
    REQ_INTERACTIVE_WINSTA(ERROR_ACCESS_DENIED);
    return SpiMemCopy(pvParam, pvData, cbSize, fl & SPIF_PROTECT);
}

static inline
UINT_PTR
SpiSet(PVOID pvData, PVOID pvParam, ULONG cbSize, FLONG fl)
{
    REQ_INTERACTIVE_WINSTA(ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);
    return SpiMemCopy(pvData, pvParam, cbSize, fl & SPIF_PROTECT);
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
SpiSetDWord(PVOID pvData, INT iValue, PCWSTR pwszKey, PCWSTR pwszValue, FLONG fl)
{
    REQ_INTERACTIVE_WINSTA(ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);
    *(INT*)pvData = iValue;
    if (fl & SPIF_UPDATEINIFILE)
    {
        SpiStoreDWord(pwszKey, pwszValue, iValue);
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
    BOOL bValue = PtrToUlong(pvValue);

    REQ_INTERACTIVE_WINSTA(ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);

    /* Set or clear bit according to bValue */
    gspv.dwUserPrefMask = bValue ? gspv.dwUserPrefMask | dwMask :
                                   gspv.dwUserPrefMask & ~dwMask;

    if (fl & SPIF_UPDATEINIFILE)
    {
        /* Read current value */
        if (!RegReadUserSetting(KEY_DESKTOP,
                                VAL_USERPREFMASK,
                                REG_BINARY,
                                &dwRegMask,
                                sizeof(DWORD)))
        {
            WARN("Failed to read UserPreferencesMask setting\n");
            dwRegMask = 0;
        }

        /* Set or clear bit according to bValue */
        dwRegMask = bValue ? (dwRegMask | dwMask) : (dwRegMask & ~dwMask);

        /* write back value */
        RegWriteUserSetting(KEY_DESKTOP,
                            VAL_USERPREFMASK,
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
    bResult = SpiMemCopy(&ustr, pvParam, sizeof(ustr), fl & SPIF_PROTECT);
    if (!bResult)
    {
        return 0;
    }
    if (ustr.Length > MAX_PATH * sizeof(WCHAR))
    {
        return 0;
    }

    /* Copy the string buffer name */
    bResult = SpiMemCopy(gspv.awcWallpaper, ustr.Buffer, ustr.Length, fl & SPIF_PROTECT);
    if (!bResult)
    {
        return 0;
    }

    /* Update the UNICODE_STRING */
    gspv.ustrWallpaper.Buffer = gspv.awcWallpaper;
    gspv.ustrWallpaper.MaximumLength = MAX_PATH * sizeof(WCHAR);
    gspv.ustrWallpaper.Length = ustr.Length;
    gspv.awcWallpaper[ustr.Length / sizeof(WCHAR)] = 0;

    TRACE("SpiSetWallpaper, name=%S\n", gspv.awcWallpaper);

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
            ERR("RtlDosPathNameToNtPathName_U failed\n");
            return 0;
        }

        /* Load the Bitmap */
        hbmp = UserLoadImage(ustr.Buffer);
        if (!hbmp)
        {
            ERR("UserLoadImage failed\n");
            return 0;
        }

        /* Try to get the size of the wallpaper */
        if (!(psurfBmp = SURFACE_ShareLockSurface(hbmp)))
        {
            GreDeleteObject(hbmp);
            return 0;
        }

        gspv.cxWallpaper = psurfBmp->SurfObj.sizlBitmap.cx;
        gspv.cyWallpaper = psurfBmp->SurfObj.sizlBitmap.cy;
        gspv.WallpaperMode = wmCenter;

        SURFACE_ShareUnlockSurface(psurfBmp);

        /* Change the bitmap's ownership */
        GreSetObjectOwner(hbmp, GDI_OBJ_HMGR_PUBLIC);

        /* Yes, Windows really loads the current setting from the registry. */
        ulTile = SpiLoadInt(KEY_DESKTOP, L"TileWallpaper", 0);
        ulStyle = SpiLoadInt(KEY_DESKTOP, L"WallpaperStyle", 0);
        TRACE("SpiSetWallpaper: ulTile=%lu, ulStyle=%lu\n", ulTile, ulStyle);

        /* Check the values we found in the registry */
        if (ulTile && !ulStyle)
        {
            gspv.WallpaperMode = wmTile;
        }
        else if (!ulTile && ulStyle)
        {
            if (ulStyle == 2)
            {
                gspv.WallpaperMode = wmStretch;
            }
            else if (ulStyle == 6)
            {
                gspv.WallpaperMode = wmFit;
            }
            else if (ulStyle == 10)
            {
                gspv.WallpaperMode = wmFill;
            }
        }
    }
    else
    {
        /* Remove wallpaper */
        gspv.cxWallpaper = 0;
        gspv.cyWallpaper = 0;
        hbmp = 0;
    }

    /* Take care of the old wallpaper, if any */
    hOldBitmap = gspv.hbmWallpaper;
    if(hOldBitmap != NULL)
    {
        /* Delete the old wallpaper */
        GreSetObjectOwner(hOldBitmap, GDI_OBJ_HMGR_POWNED);
        GreDeleteObject(hOldBitmap);
    }

    /* Set the new wallpaper */
    gspv.hbmWallpaper = hbmp;

    NtUserRedrawWindow(UserGetShellWindow(), NULL, NULL, RDW_INVALIDATE | RDW_ERASE);


    return (UINT_PTR)KEY_DESKTOP;
}

static BOOL
SpiNotifyNCMetricsChanged(VOID)
{
    PWND pwndDesktop, pwndCurrent;
    HWND *ahwnd;
    USER_REFERENCE_ENTRY Ref;
    int i;

    pwndDesktop = UserGetDesktopWindow();
    ASSERT(pwndDesktop);

    ahwnd = IntWinListChildren(pwndDesktop);
    if(!ahwnd)
        return FALSE;

    for (i = 0; ahwnd[i]; i++)
    {
        pwndCurrent = UserGetWindowObject(ahwnd[i]);
        if(!pwndCurrent)
            continue;

        UserRefObjectCo(pwndCurrent, &Ref);
        co_WinPosSetWindowPos(pwndCurrent, 0, pwndCurrent->rcWindow.left,pwndCurrent->rcWindow.top,
                                              pwndCurrent->rcWindow.right-pwndCurrent->rcWindow.left
                                              ,pwndCurrent->rcWindow.bottom - pwndCurrent->rcWindow.top,
                              SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOCOPYBITS|
                              SWP_NOMOVE|SWP_NOZORDER|SWP_NOREDRAW);
        UserDerefObjectCo(pwndCurrent);
    }

    ExFreePoolWithTag(ahwnd, USERTAG_WINDOWLIST);

    return TRUE;
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
            ERR("SPI_LANGDRIVER is unimplemented\n");
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
            uiParam = min(uiParam, gspv.ustrWallpaper.Length + 1UL);
            return SpiGet(pvParam, gspv.awcWallpaper, uiParam, fl);

        case SPI_SETDESKWALLPAPER:
            return SpiSetWallpaper(pvParam, fl);

        case SPI_SETDESKPATTERN:
            ERR("SPI_SETDESKPATTERN is unimplemented\n");
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
            return 1;

        case SPI_GETDRAGFULLWINDOWS:
            return SpiGetInt(pvParam, &gspv.bDragFullWindows, fl);

        case SPI_SETDRAGFULLWINDOWS:
            return SpiSetInt(&gspv.bDragFullWindows, uiParam, KEY_DESKTOP, VAL_DRAG, fl);

        case SPI_GETNONCLIENTMETRICS:
        {
            return SpiGet(pvParam, &gspv.ncm, sizeof(NONCLIENTMETRICSW), fl);
        }

        case SPI_SETNONCLIENTMETRICS:
        {
            LPNONCLIENTMETRICSW metrics = (LPNONCLIENTMETRICSW)pvParam;

            /* Fixup user's structure size */
            metrics->cbSize = sizeof(NONCLIENTMETRICSW);

            if (!SpiSet(&gspv.ncm, metrics, sizeof(NONCLIENTMETRICSW), fl))
                return 0;

            if (fl & SPIF_UPDATEINIFILE)
            {
                SpiStoreMetric(VAL_BORDER, gspv.ncm.iBorderWidth);
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

            if(!SpiNotifyNCMetricsChanged())
                return 0;

            return (UINT_PTR)KEY_METRIC;
        }

        case SPI_GETMINIMIZEDMETRICS:
        {
            return SpiGet(pvParam, &gspv.mm, sizeof(MINIMIZEDMETRICS), fl);
        }

        case SPI_SETMINIMIZEDMETRICS:
        {
            LPMINIMIZEDMETRICS metrics = (LPMINIMIZEDMETRICS)pvParam;

            /* Fixup user's structure size */
            metrics->cbSize = sizeof(MINIMIZEDMETRICS);

            if (!SpiSet(&gspv.mm, metrics, sizeof(MINIMIZEDMETRICS), fl))
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
        }

        case SPI_GETICONMETRICS:
        {
            return SpiGet(pvParam, &gspv.im, sizeof(ICONMETRICSW), fl);
        }

        case SPI_SETICONMETRICS:
        {
            LPICONMETRICSW metrics = (LPICONMETRICSW)pvParam;

            /* Fixup user's structure size */
            metrics->cbSize = sizeof(ICONMETRICSW);

            if (!SpiSet(&gspv.im, metrics, sizeof(ICONMETRICSW), fl))
                return 0;

            if (fl & SPIF_UPDATEINIFILE)
            {
                SpiStoreMetric(VAL_ICONSPC, gspv.im.iHorzSpacing);
                SpiStoreMetric(VAL_ICONVSPC, gspv.im.iVertSpacing);
                SpiStoreMetric(VAL_ITWRAP, gspv.im.iTitleWrap);
                SpiStoreFont(L"IconFont", &gspv.im.lfFont);
            }
            return (UINT_PTR)KEY_METRIC;
        }

        case SPI_GETWORKAREA:
        {
            PMONITOR pmonitor = UserGetPrimaryMonitor();

            if(!pmonitor)
                return 0;

            return SpiGet(pvParam, &pmonitor->rcWork, sizeof(RECTL), fl);
        }

        case SPI_SETWORKAREA:
        {
            PMONITOR pmonitor;
            RECTL rcWorkArea, rcIntersect;

            if (!pvParam)
                return 0;

            RtlCopyMemory(&rcWorkArea, pvParam, sizeof(rcWorkArea));

            /* fail if empty */
            if (RECTL_bIsEmptyRect(&rcWorkArea))
                return 0;

            /* get the nearest monitor */
            pmonitor = UserMonitorFromRect(&rcWorkArea, MONITOR_DEFAULTTONEAREST);
            if (!pmonitor)
                return 0;

            /* fail unless work area is completely in monitor */
            if (!RECTL_bIntersectRect(&rcIntersect, &pmonitor->rcMonitor, &rcWorkArea) ||
                !RtlEqualMemory(&rcIntersect, &rcWorkArea, sizeof(rcIntersect)))
            {
                return 0;
            }

            if (!SpiSet(&pmonitor->rcWork, pvParam, sizeof(RECTL), fl))
                return 0;

            if (fl & SPIF_UPDATEINIFILE)
            {
                // FIXME: What to do?
            }
            return (UINT_PTR)KEY_DESKTOP;
        }

        case SPI_SETPENWINDOWS:
            ERR("SPI_SETPENWINDOWS is unimplemented\n");
            break;

        case SPI_GETFILTERKEYS:
        {
            LPFILTERKEYS FilterKeys = (LPFILTERKEYS)pvParam;

            if (uiParam != 0 && uiParam != sizeof(FILTERKEYS))
                return 0;

            if (!FilterKeys || FilterKeys->cbSize != sizeof(FILTERKEYS))
                return 0;

            return SpiGet(pvParam, &gspv.filterkeys, sizeof(FILTERKEYS), fl);
        }

        case SPI_SETFILTERKEYS:
        {
            LPFILTERKEYS FilterKeys = (LPFILTERKEYS)pvParam;

            if (uiParam != 0 && uiParam != sizeof(FILTERKEYS))
                return 0;

            if (!FilterKeys || FilterKeys->cbSize != sizeof(FILTERKEYS))
                return 0;

            if (!SpiSet(&gspv.filterkeys, pvParam, sizeof(FILTERKEYS), fl))
                return 0;

            if (fl & SPIF_UPDATEINIFILE)
            {
                // FIXME: What to do?
            }
            return (UINT_PTR)KEY_DESKTOP;
        }

        case SPI_GETTOGGLEKEYS:
        {
            LPTOGGLEKEYS ToggleKeys = (LPTOGGLEKEYS)pvParam;

            if (uiParam != 0 && uiParam != sizeof(TOGGLEKEYS))
                return 0;

            if (!ToggleKeys || ToggleKeys->cbSize != sizeof(TOGGLEKEYS))
                return 0;

            return SpiGet(pvParam, &gspv.togglekeys, sizeof(TOGGLEKEYS), fl);
        }

        case SPI_SETTOGGLEKEYS:
        {
            LPTOGGLEKEYS ToggleKeys = (LPTOGGLEKEYS)pvParam;

            if (uiParam != 0 && uiParam != sizeof(TOGGLEKEYS))
                return 0;

            if (!ToggleKeys || ToggleKeys->cbSize != sizeof(TOGGLEKEYS))
                return 0;

            if (!SpiSet(&gspv.togglekeys, pvParam, sizeof(TOGGLEKEYS), fl))
                return 0;

            if (fl & SPIF_UPDATEINIFILE)
            {
                // FIXME: What to do?
            }
            return (UINT_PTR)KEY_DESKTOP;
        }

        case SPI_GETMOUSEKEYS:
        {
            LPMOUSEKEYS MouseKeys = (LPMOUSEKEYS)pvParam;

            if (uiParam != 0 && uiParam != sizeof(MOUSEKEYS))
                return 0;

            if (!MouseKeys || MouseKeys->cbSize != sizeof(MOUSEKEYS))
                return 0;

            return SpiGet(pvParam, &gspv.mousekeys, sizeof(MOUSEKEYS), fl);
        }

        case SPI_SETMOUSEKEYS:
        {
            LPMOUSEKEYS MouseKeys = (LPMOUSEKEYS)pvParam;

            if (uiParam != 0 && uiParam != sizeof(MOUSEKEYS))
                return 0;

            if (!MouseKeys || MouseKeys->cbSize != sizeof(MOUSEKEYS))
                return 0;

            if (!SpiSet(&gspv.mousekeys, pvParam, sizeof(MOUSEKEYS), fl))
                return 0;

            if (fl & SPIF_UPDATEINIFILE)
            {
                // FIXME: What to do?
            }
            return (UINT_PTR)KEY_DESKTOP;
        }

        case SPI_GETSHOWSOUNDS:
            return SpiGetInt(pvParam, &gspv.bShowSounds, fl);

        case SPI_SETSHOWSOUNDS:
            return SpiSetBool(&gspv.bShowSounds, uiParam, KEY_SHOWSNDS, VAL_ON, fl);

        case SPI_GETSTICKYKEYS:
        {
            LPSTICKYKEYS StickyKeys = (LPSTICKYKEYS)pvParam;

            if (uiParam != 0 && uiParam != sizeof(STICKYKEYS))
                return 0;

            if (!StickyKeys || StickyKeys->cbSize != sizeof(STICKYKEYS))
                return 0;

            return SpiGetEx(pvParam, &gspv.stickykeys, sizeof(STICKYKEYS), fl);
        }

        case SPI_SETSTICKYKEYS:
        {
            LPSTICKYKEYS StickyKeys = (LPSTICKYKEYS)pvParam;

            if (uiParam != 0 && uiParam != sizeof(STICKYKEYS))
                return 0;

            if (!StickyKeys || StickyKeys->cbSize != sizeof(STICKYKEYS))
                return 0;

            if (!SpiSet(&gspv.stickykeys, pvParam, sizeof(STICKYKEYS), fl))
                return 0;

            if (fl & SPIF_UPDATEINIFILE)
            {
                // FIXME: What to do?
            }
            return (UINT_PTR)KEY_DESKTOP;
        }

        case SPI_GETACCESSTIMEOUT:
        {
            LPACCESSTIMEOUT AccessTimeout = (LPACCESSTIMEOUT)pvParam;

            if (uiParam != 0 && uiParam != sizeof(ACCESSTIMEOUT))
                return 0;

            if (!AccessTimeout || AccessTimeout->cbSize != sizeof(ACCESSTIMEOUT))
                return 0;

            return SpiGetEx(pvParam, &gspv.accesstimeout, sizeof(ACCESSTIMEOUT), fl);
        }

        case SPI_SETACCESSTIMEOUT:
        {
            LPACCESSTIMEOUT AccessTimeout = (LPACCESSTIMEOUT)pvParam;

            if (uiParam != 0 && uiParam != sizeof(ACCESSTIMEOUT))
            {
                return 0;
            }

            if (!AccessTimeout || AccessTimeout->cbSize != sizeof(ACCESSTIMEOUT))
            {
                return 0;
            }

            if (!SpiSet(&gspv.accesstimeout, pvParam, sizeof(ACCESSTIMEOUT), fl))
                return 0;

            if (fl & SPIF_UPDATEINIFILE)
            {
                // FIXME: What to do?
            }
            return (UINT_PTR)KEY_DESKTOP;
        }

        case SPI_GETSERIALKEYS:
        {
            LPSERIALKEYS SerialKeys = (LPSERIALKEYS)pvParam;

            if (uiParam != 0 && uiParam != sizeof(SERIALKEYS))
                return 0;

            if (!SerialKeys || SerialKeys->cbSize != sizeof(SERIALKEYS))
                return 0;

            return SpiGet(pvParam, &gspv.serialkeys, sizeof(SERIALKEYS), fl);
        }

        case SPI_SETSERIALKEYS:
        {
            LPSERIALKEYS SerialKeys = (LPSERIALKEYS)pvParam;

            if (uiParam != 0 && uiParam != sizeof(SERIALKEYS))
                return 0;

            if (!SerialKeys || SerialKeys->cbSize != sizeof(SERIALKEYS))
                return 0;

            if (!SpiSet(&gspv.serialkeys, pvParam, sizeof(SERIALKEYS), fl))
                return 0;

            if (fl & SPIF_UPDATEINIFILE)
            {
                // FIXME: What to do?
            }
            return (UINT_PTR)KEY_DESKTOP;
        }

        case SPI_GETSOUNDSENTRY:
        {
            LPSOUNDSENTRYW SoundsEntry = (LPSOUNDSENTRYW)pvParam;

            if (uiParam != 0 && uiParam != sizeof(SOUNDSENTRYW))
                return 0;

            if (!SoundsEntry || SoundsEntry->cbSize != sizeof(SOUNDSENTRYW))
                return 0;

            return SpiGet(pvParam, &gspv.soundsentry, sizeof(SOUNDSENTRYW), fl);
        }

        case SPI_SETSOUNDSENTRY:
        {
            LPSOUNDSENTRYW SoundsEntry = (LPSOUNDSENTRYW)pvParam;

            if (uiParam != 0 && uiParam != sizeof(SOUNDSENTRYW))
                return 0;

            if (!SoundsEntry || SoundsEntry->cbSize != sizeof(SOUNDSENTRYW))
                return 0;

            if (!SpiSet(&gspv.soundsentry, pvParam, sizeof(SOUNDSENTRYW), fl))
                return 0;

            if (fl & SPIF_UPDATEINIFILE)
            {
                // FIXME: What to do?
            }
            return (UINT_PTR)KEY_DESKTOP;
        }

        case SPI_GETHIGHCONTRAST:
        {
            LPHIGHCONTRASTW highcontrast = (LPHIGHCONTRASTW)pvParam;

            if (uiParam != 0 && uiParam != sizeof(HIGHCONTRASTW))
                return 0;

            if (!highcontrast || highcontrast->cbSize != sizeof(HIGHCONTRASTW))
                return 0;

            return SpiGet(pvParam, &gspv.highcontrast, sizeof(HIGHCONTRASTW), fl);
        }

        case SPI_SETHIGHCONTRAST:
        {
            LPHIGHCONTRASTW highcontrast = (LPHIGHCONTRASTW)pvParam;

            if (uiParam != 0 && uiParam != sizeof(HIGHCONTRASTW))
                return 0;

            if (!highcontrast || highcontrast->cbSize != sizeof(HIGHCONTRASTW))
                return 0;

            if (!SpiSet(&gspv.highcontrast, pvParam, sizeof(HIGHCONTRASTW), fl))
                return 0;

            if (fl & SPIF_UPDATEINIFILE)
            {
                // FIXME: What to do?
            }
            return (UINT_PTR)KEY_DESKTOP;
        }

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
                // FIXME: What to do?
            }
            return (UINT_PTR)KEY_DESKTOP;

        case SPI_GETFONTSMOOTHING:
            return SpiGetInt(pvParam, &gspv.bFontSmoothing, fl);

        case SPI_SETFONTSMOOTHING:
            gspv.bFontSmoothing = (uiParam == 2);
            if (fl & SPIF_UPDATEINIFILE)
            {
                SpiStoreSz(KEY_DESKTOP, VAL_FONTSMOOTHING, (uiParam == 2) ? L"2" : L"0");
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
            ERR("SPI_SETCURSORS is unimplemented\n");
            break;

        case SPI_SETICONS:
            ERR("SPI_SETICONS is unimplemented\n");
            break;

        case SPI_GETDEFAULTINPUTLANG:
            if (!gspklBaseLayout)
                return FALSE;

            return SpiGet(pvParam, &gspklBaseLayout->hkl, sizeof(HKL), fl);

        case SPI_SETDEFAULTINPUTLANG:
        {
            HKL hkl;

            /* Note: SPIF_UPDATEINIFILE is not supported */
            if ((fl & SPIF_UPDATEINIFILE) || !SpiSet(&hkl, pvParam, sizeof(hkl), fl))
                return FALSE;

            return UserSetDefaultInputLang(hkl);
        }

        case SPI_SETLANGTOGGLE:
            gdwLanguageToggleKey = UserGetLanguageToggle();
            return gdwLanguageToggleKey;
            break;

        case SPI_GETWINDOWSEXTENSION:
            ERR("SPI_GETWINDOWSEXTENSION is unimplemented\n");
            break;

        case SPI_GETMOUSETRAILS:
            return SpiGetInt(pvParam, &gspv.iMouseTrails, fl);

        case SPI_SETMOUSETRAILS:
            return SpiSetInt(&gspv.iMouseTrails, uiParam, KEY_MOUSE, VAL_MOUSETRAILS, fl);

        case SPI_GETSNAPTODEFBUTTON:
            return SpiGetInt(pvParam, &gspv.bSnapToDefBtn, fl);

        case SPI_SETSNAPTODEFBUTTON:
            return SpiSetBool(&gspv.bSnapToDefBtn, uiParam, KEY_MOUSE, VAL_SNAPDEFBTN, fl);

        case SPI_GETMOUSEHOVERWIDTH:
            return SpiGetInt(pvParam, &gspv.iMouseHoverWidth, fl);

        case SPI_SETMOUSEHOVERWIDTH:
            return SpiSetInt(&gspv.iMouseHoverWidth, uiParam, KEY_MOUSE, VAL_HOVERWIDTH, fl);

        case SPI_GETMOUSEHOVERHEIGHT:
            return SpiGetInt(pvParam, &gspv.iMouseHoverHeight, fl);

        case SPI_SETMOUSEHOVERHEIGHT:
            return SpiSetInt(&gspv.iMouseHoverHeight, uiParam, KEY_MOUSE, VAL_HOVERHEIGHT, fl);

        case SPI_GETMOUSEHOVERTIME:
            return SpiGetInt(pvParam, &gspv.iMouseHoverTime, fl);

        case SPI_SETMOUSEHOVERTIME:
           /* See http://msdn2.microsoft.com/en-us/library/ms724947.aspx
            * copy text from it, if some agument why xp and 2003 behovir diffent
            * only if they do not have SP install
            * " Windows Server 2003 and Windows XP: The operating system does not
            *   enforce the use of USER_TIMER_MAXIMUM and USER_TIMER_MINIMUM until
            *   Windows Server 2003 SP1 and Windows XP SP2 "
            */
            return SpiSetInt(&gspv.iMouseHoverTime, uiParam, KEY_MOUSE, VAL_HOVERTIME, fl);

        case SPI_GETWHEELSCROLLLINES:
            return SpiGetInt(pvParam, &gspv.iWheelScrollLines, fl);

        case SPI_SETWHEELSCROLLLINES:
            return SpiSetInt(&gspv.iWheelScrollLines, uiParam, KEY_DESKTOP, VAL_SCRLLLINES, fl);

        case SPI_GETMENUSHOWDELAY:
            return SpiGetInt(pvParam, &gspv.dwMenuShowDelay, fl);

        case SPI_SETMENUSHOWDELAY:
            return SpiSetInt(&gspv.dwMenuShowDelay, uiParam, KEY_DESKTOP, L"MenuShowDelay", fl);

#if (_WIN32_WINNT >= 0x0600)
        case SPI_GETWHEELSCROLLCHARS:
            return SpiGetInt(pvParam, &gspv.uiWheelScrollChars, fl);

        case SPI_SETWHEELSCROLLCHARS:
            return SpiSetInt(&gspv.uiWheelScrollChars, uiParam, KEY_DESKTOP, VAL_SCRLLCHARS, fl);
#endif
        case SPI_GETSHOWIMEUI:
            return SpiGetInt(pvParam, &gspv.bShowImeUi, fl);

        case SPI_SETSHOWIMEUI:
            return SpiSetBool(&gspv.bShowImeUi, uiParam, KEY_DESKTOP, L"", fl);

        case SPI_GETMOUSESPEED:
            return SpiGetInt(pvParam, &gspv.iMouseSpeed, fl);

        case SPI_SETMOUSESPEED:
        {
            /* Allowed range is [1:20] */
            if ((INT_PTR)pvParam < 1 || (INT_PTR)pvParam > 20)
                return 0;
            else
                return SpiSetInt(&gspv.iMouseSpeed, (INT_PTR)pvParam, KEY_MOUSE, VAL_SENSITIVITY, fl);
        }

        case SPI_GETSCREENSAVERRUNNING:
            return SpiGetInt(pvParam, &gspv.bScrSaverRunning, fl);

        case SPI_SETSCREENSAVERRUNNING:
            // FIXME: also return value?
            return SpiSetBool(&gspv.bScrSaverRunning, uiParam, KEY_MOUSE, L"", fl);

#if(WINVER >= 0x0600)
        case SPI_GETAUDIODESCRIPTION:
            return SpiGet(pvParam, &gspv.audiodescription, sizeof(AUDIODESCRIPTION), fl);

        case SPI_SETAUDIODESCRIPTION:
            ERR("SPI_SETAUDIODESCRIPTION is unimplemented\n");
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
        {
            if (NtGdiGetDeviceCaps(ScreenDeviceContext, BITSPIXEL) <= 8)
            {
                INT iValue = 0;
                return SpiGetInt(pvParam, &iValue, fl);
            }
            else
            {
                return SpiGetUserPref(UPM_GRADIENTCAPTIONS, pvParam, fl);
            }
        }

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
            gspv.bMouseCursorShadow = PtrToUlong(pvParam);
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
            return SpiGetUserPref(UPM_CLICKLOCK, pvParam, fl);

        case SPI_SETMOUSECLICKLOCK:
            gspv.bMouseClickLock = PtrToUlong(pvParam);
            return SpiSetUserPref(UPM_CLICKLOCK, pvParam, fl);

        case SPI_GETMOUSEVANISH:
            return SpiGetInt(pvParam, &gspv.bMouseVanish, fl);

        case SPI_SETMOUSEVANISH:
            return SpiSetBool(&gspv.bMouseVanish, uiParam, KEY_MOUSE, L"", fl);

        case SPI_GETFLATMENU:
            return SpiGetUserPref(UPM_FLATMENU, pvParam, fl);

        case SPI_SETFLATMENU:
            return SpiSetUserPref(UPM_FLATMENU, pvParam, fl);

        case SPI_GETDROPSHADOW:
            return SpiGetUserPref(UPM_DROPSHADOW, pvParam, fl);

        case SPI_SETDROPSHADOW:
            return SpiSetUserPref(UPM_DROPSHADOW, pvParam, fl);

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
            return SpiGetInt(pvParam, &gspv.bClientAreaAnimation, fl);

        case SPI_SETCLIENTAREAANIMATION:
            return SpiSetBool(&gspv.bClientAreaAnimation, uiParam, KEY_MOUSE, L"", fl);

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
            return SpiSetDWord(&gspv.dwMouseClickLockTime, uiParam, KEY_DESKTOP, VAL_CLICKLOCKTIME, fl);

        case SPI_GETFONTSMOOTHINGTYPE:
            return SpiGetInt(pvParam, &gspv.uiFontSmoothingType, fl);

        case SPI_SETFONTSMOOTHINGTYPE:
            return SpiSetDWord(&gspv.uiFontSmoothingType, PtrToUlong(pvParam), KEY_DESKTOP, VAL_FONTSMOOTHINGTYPE, fl);

        case SPI_GETFONTSMOOTHINGCONTRAST:
            return SpiGetInt(pvParam, &gspv.uiFontSmoothingContrast, fl);

        case SPI_SETFONTSMOOTHINGCONTRAST:
            return SpiSetDWord(&gspv.uiFontSmoothingContrast, PtrToUlong(pvParam), KEY_DESKTOP, VAL_FONTSMOOTHINGCONTRAST, fl);

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
            return SpiSetDWord(&gspv.uiFontSmoothingOrientation, PtrToUlong(pvParam), KEY_DESKTOP, VAL_FONTSMOOTHINGORIENTATION, fl);

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
            ERR("Undocumented SPI value %x is unimplemented\n", uiAction);
            break;

        default:
            ERR("Invalid SPI value: %u\n", uiAction);
            EngSetLastError(ERROR_INVALID_PARAMETER);
            return 0;
    }

    return 0;
}

static BOOL
SpiGetSetProbeBuffer(UINT uiAction, UINT uiParam, PVOID pvParam)
{
    BOOL bToUser = TRUE;
    ULONG cbSize = 0;

    switch (uiAction)
    {
        case SPI_GETBEEP:
        case SPI_GETBORDER:
        case SPI_GETKEYBOARDSPEED:
        case SPI_GETSCREENSAVETIMEOUT:
        case SPI_GETSCREENSAVEACTIVE:
        case SPI_GETGRIDGRANULARITY:
        case SPI_GETKEYBOARDDELAY:
        case SPI_GETICONTITLEWRAP:
        case SPI_GETMENUDROPALIGNMENT:
        case SPI_GETFASTTASKSWITCH:
        case SPI_GETDRAGFULLWINDOWS:
        case SPI_GETSHOWSOUNDS:
        case SPI_GETKEYBOARDPREF:
        case SPI_GETSCREENREADER:
        case SPI_GETFONTSMOOTHING:
        case SPI_GETLOWPOWERTIMEOUT:
        case SPI_GETPOWEROFFTIMEOUT:
        case SPI_GETLOWPOWERACTIVE:
        case SPI_GETPOWEROFFACTIVE:
        case SPI_GETMOUSETRAILS:
        case SPI_GETSNAPTODEFBUTTON:
        case SPI_GETMOUSEHOVERWIDTH:
        case SPI_GETMOUSEHOVERHEIGHT:
        case SPI_GETMOUSEHOVERTIME:
        case SPI_GETWHEELSCROLLLINES:
        case SPI_GETMENUSHOWDELAY:
#if (_WIN32_WINNT >= 0x0600)
        case SPI_GETWHEELSCROLLCHARS:
#endif
        case SPI_GETSHOWIMEUI:
        case SPI_GETMOUSESPEED:
        case SPI_GETSCREENSAVERRUNNING:
#if(WINVER >= 0x0600)
        case SPI_GETSCREENSAVESECURE:
#endif
        case SPI_GETACTIVEWINDOWTRACKING:
        case SPI_GETMENUANIMATION:
        case SPI_GETCOMBOBOXANIMATION:
        case SPI_GETLISTBOXSMOOTHSCROLLING:
        case SPI_GETGRADIENTCAPTIONS:
        case SPI_GETKEYBOARDCUES:
        case SPI_GETACTIVEWNDTRKZORDER:
        case SPI_GETHOTTRACKING:
        case SPI_GETMENUFADE:
        case SPI_GETSELECTIONFADE:
        case SPI_GETTOOLTIPANIMATION:
        case SPI_GETTOOLTIPFADE:
        case SPI_GETCURSORSHADOW:
        case SPI_GETUIEFFECTS:
        case SPI_GETMOUSESONAR:
        case SPI_GETMOUSECLICKLOCK:
        case SPI_GETMOUSEVANISH:
        case SPI_GETFLATMENU:
        case SPI_GETDROPSHADOW:
        case SPI_GETBLOCKSENDINPUTRESETS:
#if(_WIN32_WINNT >= 0x0600)
        case SPI_GETDISABLEOVERLAPPEDCONTENT:
        case SPI_GETCLIENTAREAANIMATION:
        case SPI_GETCLEARTYPE:
        case SPI_GETSPEECHRECOGNITION:
#endif
        case SPI_GETFOREGROUNDLOCKTIMEOUT:
        case SPI_GETACTIVEWNDTRKTIMEOUT:
        case SPI_GETFOREGROUNDFLASHCOUNT:
        case SPI_GETCARETWIDTH:
        case SPI_GETMOUSECLICKLOCKTIME:
        case SPI_GETFONTSMOOTHINGTYPE:
        case SPI_GETFONTSMOOTHINGCONTRAST:
        case SPI_GETFOCUSBORDERWIDTH:
        case SPI_GETFOCUSBORDERHEIGHT:
        case SPI_GETFONTSMOOTHINGORIENTATION:
            cbSize = sizeof(INT);
            break;

        case SPI_ICONHORIZONTALSPACING:
        case SPI_ICONVERTICALSPACING:
            if (pvParam) cbSize = sizeof(INT);
            break;

        case SPI_GETMOUSE:
            cbSize = 3 * sizeof(INT);
            break;

        case SPI_GETDESKWALLPAPER:
            cbSize = min(uiParam, gspv.ustrWallpaper.Length + 1UL);
            break;

        case SPI_GETICONTITLELOGFONT:
            cbSize = sizeof(LOGFONTW);
            break;

        case SPI_GETNONCLIENTMETRICS:
            cbSize = sizeof(NONCLIENTMETRICSW);
            break;

        case SPI_GETMINIMIZEDMETRICS:
            cbSize = sizeof(MINIMIZEDMETRICS);
            break;

        case SPI_GETICONMETRICS:
            cbSize = sizeof(ICONMETRICSW);
            break;

        case SPI_GETWORKAREA:
            cbSize = sizeof(RECTL);
            break;

        case SPI_GETFILTERKEYS:
            cbSize = sizeof(FILTERKEYS);
            break;

        case SPI_GETTOGGLEKEYS:
            cbSize = sizeof(TOGGLEKEYS);
            break;

        case SPI_GETMOUSEKEYS:
            cbSize = sizeof(MOUSEKEYS);
            break;

        case SPI_GETSTICKYKEYS:
            cbSize = sizeof(STICKYKEYS);
            break;

        case SPI_GETACCESSTIMEOUT:
            cbSize = sizeof(ACCESSTIMEOUT);
            break;

        case SPI_GETSERIALKEYS:
            cbSize = sizeof(SERIALKEYS);
            break;

        case SPI_GETSOUNDSENTRY:
            cbSize = sizeof(SOUNDSENTRYW);
            break;

        case SPI_GETHIGHCONTRAST:
            cbSize = sizeof(HIGHCONTRASTW);
            break;

        case SPI_GETANIMATION:
            cbSize = sizeof(ANIMATIONINFO);
            break;

        case SPI_GETDEFAULTINPUTLANG:
            cbSize = sizeof(HKL);
            break;

#if(WINVER >= 0x0600)
        case SPI_GETAUDIODESCRIPTION:
            cbSize = sizeof(AUDIODESCRIPTION);
            break;
#endif

        case SPI_SETMOUSE:
            cbSize = 3 * sizeof(INT);
            bToUser = FALSE;
            break;

        case SPI_SETICONTITLELOGFONT:
            cbSize = sizeof(LOGFONTW);
            bToUser = FALSE;
            break;

        case SPI_SETNONCLIENTMETRICS:
            cbSize = sizeof(NONCLIENTMETRICSW);
            bToUser = FALSE;
            break;

        case SPI_SETMINIMIZEDMETRICS:
            cbSize = sizeof(MINIMIZEDMETRICS);
            bToUser = FALSE;
            break;

        case SPI_SETICONMETRICS:
            cbSize = sizeof(ICONMETRICSW);
            bToUser = FALSE;
            break;

        case SPI_SETWORKAREA:
            cbSize = sizeof(RECTL);
            bToUser = FALSE;
            break;

        case SPI_SETFILTERKEYS:
            cbSize = sizeof(FILTERKEYS);
            bToUser = FALSE;
            break;

        case SPI_SETTOGGLEKEYS:
            cbSize = sizeof(TOGGLEKEYS);
            bToUser = FALSE;
            break;

        case SPI_SETMOUSEKEYS:
            cbSize = sizeof(MOUSEKEYS);
            bToUser = FALSE;
            break;

        case SPI_SETSTICKYKEYS:
            cbSize = sizeof(STICKYKEYS);
            bToUser = FALSE;
            break;

        case SPI_SETACCESSTIMEOUT:
            cbSize = sizeof(ACCESSTIMEOUT);
            bToUser = FALSE;
            break;

        case SPI_SETSERIALKEYS:
            cbSize = sizeof(SERIALKEYS);
            bToUser = FALSE;
            break;

        case SPI_SETSOUNDSENTRY:
            cbSize = sizeof(SOUNDSENTRYW);
            bToUser = FALSE;
            break;

        case SPI_SETHIGHCONTRAST:
            cbSize = sizeof(HIGHCONTRASTW);
            bToUser = FALSE;
            break;

        case SPI_SETANIMATION:
            cbSize = sizeof(ANIMATIONINFO);
            bToUser = FALSE;
            break;

        case SPI_SETDEFAULTINPUTLANG:
            cbSize = sizeof(HKL);
            bToUser = FALSE;
            break;

        case SPI_SETMOUSESPEED:
            cbSize = sizeof(INT);
            bToUser = FALSE;
            break;
    }

    if (cbSize)
    {
        _SEH2_TRY
        {
            if (bToUser)
            {
                ProbeForWrite(pvParam, cbSize, sizeof(UCHAR));
            }
            else
            {
                ProbeForRead(pvParam, cbSize, sizeof(UCHAR));
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return FALSE);
        }
        _SEH2_END;
    }

    return TRUE;
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
    PPROCESSINFO ppi = PsGetCurrentProcessWin32Process();

    ASSERT(ppi);

    if (!gbSpiInitialized)
    {
        KeRosDumpStackFrames(NULL, 20);
        //ASSERT(FALSE);
        return FALSE;
    }

    /* Get a pointer to the current Windowstation */
    if (!ppi->prpwinsta)
    {
        ERR("UserSystemParametersInfo called without active window station.\n");
        //ASSERT(FALSE);
        //return FALSE;
    }

    if ((fWinIni & SPIF_PROTECT) && !SpiGetSetProbeBuffer(uiAction, uiParam, pvParam))
    {
        EngSetLastError(ERROR_NOACCESS);
        return FALSE;
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
        if (fWinIni & SPIF_SENDCHANGE)
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

    TRACE("Enter NtUserSystemParametersInfo(%u)\n", uiAction);
    UserEnterExclusive();

    // FIXME: Get rid of the flags and only use this from um. kernel can access data directly.
    /* Set UM memory protection flag */
    fWinIni |= SPIF_PROTECT;

    /* Call internal function */
    bResult = UserSystemParametersInfo(uiAction, uiParam, pvParam, fWinIni);

    TRACE("Leave NtUserSystemParametersInfo, returning %d\n", bResult);
    UserLeave();

    return bResult;
}

/* EOF */
