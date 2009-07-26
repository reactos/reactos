#ifndef _WIN32K_SYSPARAMS_H
#define _WIN32K_SYSPARAMS_H

#include "cursoricon.h"

// create one struct
// make usable for different users (multiple structs!)

#define SPI_TABLE1_MIN 1
#define SPI_TABLE1_MAX 119
#define SPI_TABLE2_MIN 4096
#define SPI_TABLE2_MAX 4171
#define SPI_TABLE3_MIN 8192
#define SPI_TABLE3_MAX 8215

#define SPIF_PROTECT 0x80000

enum
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
    // room for more
    UPM_UIEFFECTS = 0x80000000,
    UPM_DEFAULT = 0x80003E9E
} USERPREFMASKS;

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
    HIGHCONTRAST highcontrast;
    BOOL bScreenReader;
#if(WINVER >= 0x0600)
    AUDIODESCRIPTION audiodescription;
    BOOL bClientAreaAnimation;
    BOOL bDisableOverlappedContent;
    ULONG ulMsgDuration;
    BOOL bSpeechRecognition;
#endif

    /* Sound */
    SOUNDSENTRY soundsentry;
    BOOL bShowSounds;
    BOOL bBeep;

    /* Mouse */
    CURSORACCELERATION_INFO caiMouse;
    MOUSEKEYS mousekeys;
    BOOL bMouseClickLock;
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
    BOOL bFlatMenu;
    DWORD dwMenuShowDelay;
    BOOL bDropShadow;
    BOOL bBlockSendInputResets;
#if(_WIN32_WINNT >= 0x0600)
    BOOL bClearType;
#endif

    /* Text metrics */
    TEXTMETRICW tmMenuFont;
    TEXTMETRICW tmCaptionFont;

    BOOL bHandHeld;
    BOOL bFastTaskSwitch;
    UINT uiGridGranularity;
    UNICODE_STRING ustrWallpaper;
    WCHAR awcWallpaper[MAX_PATH];

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
    SOUNDSENTRY soundsentry;
    NONCLIENTMETRICSW ncmetrics;
    MINIMIZEDMETRICS mmmetrics;
    ICONMETRICS iconmetrics;
    HIGHCONTRAST highcontrast;
    ANIMATIONINFO animationinfo;
#if(WINVER >= 0x0600)
    AUDIODESCRIPTION audiodescription;
#endif
} SPIBUFFER;

extern SPIVALUES gspv;

#endif /* _WIN32K_SYSPARAMS_H */
