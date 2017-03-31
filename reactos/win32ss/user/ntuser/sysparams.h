#pragma once

// Create one struct
// Make usable for different users (multiple structs!)

#define SPI_TABLE1_MIN 1
#define SPI_TABLE1_MAX 119
#define SPI_TABLE2_MIN 4096
#define SPI_TABLE2_MAX 4171
#define SPI_TABLE3_MIN 8192
#define SPI_TABLE3_MAX 8215

#define SPIF_PROTECT 0x80000

typedef enum _USERPREFMASKS
{
    UPM_ACTIVEWINDOWTRACKING = 0x01,
    UPM_MENUANIMATION = 0x02,
    UPM_COMBOBOXANIMATION = 0x04,
    UPM_LISTBOXSMOOTHSCROLLING = 0x08,
    UPM_GRADIENTCAPTIONS = 0x10,
    UPM_KEYBOARDCUES = 0x20,
    UPM_ACTIVEWNDTRKZORDER = 0x40,
    UPM_HOTTRACKING = 0x80,
    UPM_RESERVED = 0x100,
    UPM_MENUFADE = 0x200,
    UPM_SELECTIONFADE = 0x400,
    UPM_TOOLTIPANIMATION = 0x800,
    UPM_TOOLTIPFADE = 0x1000,
    UPM_CURSORSHADOW = 0x2000,
    UPM_CLICKLOCK = 0x8000,
    UPM_FLATMENU = 0x20000,
    UPM_DROPSHADOW = 0x40000,
    // room for more
    UPM_UIEFFECTS = 0x80000000,
    UPM_DEFAULT = 0x80003E9E
} USERPREFMASKS;

typedef enum
{
    wmCenter = 0,
    wmTile,
    wmStretch
} WALLPAPER_MODE;

typedef struct _SPIVALUES
{
    /* Metrics */
    NONCLIENTMETRICSW ncm;
    MINIMIZEDMETRICS mm;
    ICONMETRICSW im;
    UINT uiFocusBorderWidth;
    UINT uiFocusBorderHeight;

    /* Accessability */
    ACCESSTIMEOUT accesstimeout;
    HIGHCONTRASTW highcontrast;
    BOOL bScreenReader;
#if(WINVER >= 0x0600)
    AUDIODESCRIPTION audiodescription;
    BOOL bClientAreaAnimation;
    BOOL bDisableOverlappedContent;
    ULONG ulMsgDuration;
    BOOL bSpeechRecognition;
#endif

    /* Sound */
    SOUNDSENTRYW soundsentry;
    BOOL bShowSounds;
    BOOL bBeep;

    /* Mouse */
    CURSORACCELERATION_INFO caiMouse;
    MOUSEKEYS mousekeys;
    BOOL bMouseClickLock;
    BOOL bMouseCursorShadow;
    DWORD dwMouseClickLockTime;
    BOOL bMouseSonar;
    BOOL bMouseVanish;
    BOOL bMouseBtnSwap;
    BOOL bSmoothScrolling;
    INT iMouseSpeed;
    INT iMouseHoverWidth;
    INT iMouseHoverHeight;
    INT iMouseHoverTime;
    INT iDblClickWidth;
    INT iDblClickHeight;
    INT iDblClickTime;
    INT iDragWidth;
    INT iDragHeight;
    INT iMouseTrails;
    INT iWheelScrollLines;
#if (_WIN32_WINNT >= 0x0600)
    UINT uiWheelScrollChars;
#endif

    /* Keyboard */
    FILTERKEYS filterkeys;
    SERIALKEYS serialkeys;
    STICKYKEYS stickykeys;
    TOGGLEKEYS togglekeys;
    DWORD dwKbdSpeed;
    BOOL bKbdPref;
    HKL hklDefInputLang;
    INT iKbdDelay;

    /* Screen saver */
    INT iScrSaverTimeout;
    BOOL bScrSaverActive;
    BOOL bScrSaverRunning;
#if(WINVER >= 0x0600)
    BOOL bScrSaverSecure;
#endif

    /* Power */
    INT iLowPwrTimeout;
    INT iPwrOffTimeout;
    BOOL bLowPwrActive;
    BOOL bPwrOffActive;

    /* UI Effects */
    DWORD dwUserPrefMask;
    BOOL bFontSmoothing;
    UINT uiFontSmoothingType;
    UINT uiFontSmoothingContrast;
    UINT uiFontSmoothingOrientation;
    BOOL bDragFullWindows;
    BOOL bMenuDropAlign;
    DWORD dwMenuShowDelay;
    BOOL bBlockSendInputResets;
#if(_WIN32_WINNT >= 0x0600)
    BOOL bClearType;
#endif

    /* Text metrics */
    TEXTMETRICW tmMenuFont;
    TEXTMETRICW tmCaptionFont;

    /* Wallpaper */
    HANDLE hbmWallpaper;
    ULONG cxWallpaper, cyWallpaper;
    WALLPAPER_MODE WallpaperMode;
    UNICODE_STRING ustrWallpaper;
    WCHAR awcWallpaper[MAX_PATH + 1];

    BOOL bHandHeld;
    BOOL bFastTaskSwitch;
    UINT uiGridGranularity;

    ANIMATIONINFO animationinfo;
    BOOL bSnapToDefBtn;
    BOOL bShowImeUi;
    DWORD dwForegroundLockTimeout;
    DWORD dwActiveTrackingTimeout;
    DWORD dwForegroundFlashCount;
    DWORD dwCaretWidth;

//    SPI_LANGDRIVER
//    SPI_SETDESKPATTERN
//    SPI_SETPENWINDOWS
//    SPI_SETCURSORS
//    SPI_SETICONS
//    SPI_SETLANGTOGGLE
//    SPI_GETWINDOWSEXTENSION

} SPIVALUES, *PSPIVALUES;

typedef union _SPIBUFFER
{
    char ach[1];
    WCHAR awcWallpaper[MAX_PATH+1];
    FILTERKEYS fiterkeys;
    TOGGLEKEYS togglekeys;
    MOUSEKEYS mousekeys;
    STICKYKEYS stickykeys;
    ACCESSTIMEOUT accesstimeout;
    SERIALKEYS serialkeys;
    SOUNDSENTRYW soundsentry;
    NONCLIENTMETRICSW ncmetrics;
    MINIMIZEDMETRICS mmmetrics;
    ICONMETRICSW iconmetrics;
    HIGHCONTRASTW highcontrast;
    ANIMATIONINFO animationinfo;
#if(WINVER >= 0x0600)
    AUDIODESCRIPTION audiodescription;
#endif
} SPIBUFFER;

extern SPIVALUES gspv;
extern BOOL g_PaintDesktopVersion;

BOOL InitSysParams();
#define SPITESTPREF(x) (gspv.dwUserPrefMask & x ? 1 : 0)
