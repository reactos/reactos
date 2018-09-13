/****************************** Module Header ******************************\
* Module Name: globals.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains all the server's global variables
* One must be executing on the server's context to manipulate
* any of these variables or call any of these functions.  Serializing access
* to them is also a good idea.
*
* History:
* 10-15-90 DarrinM      Created.
\***************************************************************************/

#ifndef _GLOBALS_
#define _GLOBALS_

extern DWORD   gdwPUDFlags;

/*
 * Wallpaper Data.
 */
extern HBITMAP  ghbmWallpaper;
extern HPALETTE ghpalWallpaper;
extern SIZERECT gsrcWallpaper;
extern UINT     gwWPStyle;
extern HBITMAP  ghbmDesktop;
extern LPWSTR   gpszWall;

/*
 * SafeBoot
 */
extern DWORD gDrawVersionAlways;

/*
 * External drivers
 */
extern BOOL gfUnsignedDrivers;

/*
 * Full-Drag.
 */
extern HRGN ghrgnUpdateSave;
extern int  gnUpdateSave;

extern PWND gspwndAltTab;

extern PWND gspwndShouldBeForeground;

/*
 * full screen variables
 */
extern PWND  gspwndScreenCapture;
extern PWND  gspwndInternalCapture;
extern PWND  gspwndFullScreen;

/*
 * pre-cached monitor for mode changes
 */
extern PMONITOR gpMonitorCached;

/*
 * logon notify window
 */
extern PWND  gspwndLogonNotify;

/*
 * event used for hugn app detection
 */
extern PKEVENT gpEventHungThread;
extern PKEVENT gpEventDiconnectDesktop;

/*
 * handle for WinSta0_DesktopSwitch event
 */
extern HANDLE  ghEventSwitchDesktop;
extern PKEVENT gpEventSwitchDesktop;

/*
 * Thread Info Variables
 */
extern PTHREADINFO     gptiTasklist;
extern PTHREADINFO     gptiShutdownNotify;
extern PTHREADINFO     gptiLockUpdate;
extern PTHREADINFO     gptiForeground;
extern PTHREADINFO     gptiBlockInput;
extern PWOWTHREADINFO  gpwtiFirst;
extern PWOWPROCESSINFO gpwpiFirstWow;

/*
 * Queue Variables
 */
extern PQ gpqForeground;
extern PQ gpqForegroundPrev;
extern PQ gpqCursor;

/*
 * Accessibility globals
 */
extern FILTERKEYS    gFilterKeys;
extern STICKYKEYS    gStickyKeys;
extern MOUSEKEYS     gMouseKeys;
extern ACCESSTIMEOUT gAccessTimeOut;
extern TOGGLEKEYS    gToggleKeys;
extern SOUNDSENTRY   gSoundSentry;

extern HIGHCONTRAST  gHighContrast;
extern WCHAR         gHighContrastDefaultScheme[];

/*
 * Fade animation globals
 */
extern FADE gfade;

/*
 * FilterKeys Support
 */
extern UINT_PTR  gtmridFKActivation;
extern UINT_PTR  gtmridFKResponse;
extern UINT_PTR  gtmridFKAcceptanceDelay;
extern int   gFilterKeysState;

extern KE    gFKKeyEvent;
extern PKE   gpFKKeyEvent;
extern ULONG gFKExtraInformation;
extern int   gFKNextProcIndex;

/*
 * ToggleKeys Support
 */
extern UINT_PTR  gtmridToggleKeys;
extern ULONG gTKExtraInformation;
extern int   gTKNextProcIndex;

/*
 * TimeOut Support
 */
extern UINT_PTR  gtmridAccessTimeOut;

/*
 * MouseKeys Support
 */
extern WORD  gwMKButtonState;
extern WORD  gwMKCurrentButton;
extern UINT_PTR  gtmridMKMoveCursor;
extern LONG  gMKDeltaX;
extern LONG  gMKDeltaY;
extern UINT  giMouseMoveTable;

extern HWND ghwndSoundSentry;
extern UINT_PTR  gtmridSoundSentry;

extern MOUSECURSOR gMouseCursor;

/*
 * Multilingual keyboard layout support.
 */
extern PKL      gspklBaseLayout;
extern HKL      gLCIDSentToShell;
extern DWORD    gSystemFS;   // System font's fontsignature (single bit)

extern KBDLANGTOGGLE gLangToggle[];
extern int           gLangToggleKeyState;

extern BYTE          gfInNumpadHexInput;
extern BOOL          gfEnableHexNumpad;

/*
 * Grave accent keyboard switch for thai locales
 */
extern BOOL gbGraveKeyToggle;



/*
 * Points to currently active Keyboard Layer tables
 */
extern PKBDTABLES    gpKbdTbl;
extern BYTE          gSystemCPCharSet;   // System's input locale charset
extern PKBDNLSTABLES gpKbdNlsTbl;
extern DWORD         gdwKeyboardAttributes;

/*
 * from kbd.c
 */
extern USHORT ausNumPadCvt[]; // VK values for Numeric keypad.

extern DWORD     gtimeStartCursorHide;
extern RECT      grcCursorClip;
extern ULONG_PTR gdwMouseMoveExtraInfo;
extern DWORD     gdwMouseMoveTimeStamp;
extern LASTINPUT glinp;
extern POINT     gptCursorAsync;
extern PPROCESSINFO gppiInputProvider;
extern PPROCESSINFO gppiLockSFW;
extern UINT guSFWLockCount;
#if DBG
extern BOOL gfDebugForegroundIgnoreDebugPort;
#endif

/*
 * Cursor related Variables
 */
extern PCURSOR gpcurLogCurrent;
extern PCURSOR gpcurPhysCurrent;
extern RECT    grcVDMCursorBounds;
extern DWORD   gdwLastAniTick;
extern UINT_PTR gidCursorTimer;

extern PWND gspwndActivate;
extern PWND gspwndLockUpdate;
extern PWND gspwndMouseOwner;
extern HWND ghwndSwitch;

extern UINT gwMouseOwnerButton;
extern BOOL gbMouseButtonsRecentlySwapped;

extern UINT gdtMNDropDown;

extern int  gcountPWO;          /* count of pwo WNDOBJs in gdi */
extern int  giwndStack;
extern int  gnKeyboardSpeed;
extern int  giScreenSaveTimeOutMs;

extern PBWL gpbwlList;

extern UINT gdtDblClk;

extern UINT gwinOldAppHackoMaticFlags; // Flags for doing special things for
                               // winold app
extern UINT gcxMouseHover;
extern UINT gcyMouseHover;
extern UINT gdtMouseHover;

extern CAPTIONCACHE    gcachedCaptions[];

extern PATTACHINFO  gpai;

extern PDESKTOP     gpdeskRecalcQueueAttach;

extern PWND         gspwndCursor;
extern PPROCESSINFO gppiStarting;
extern PPROCESSINFO gppiList;
extern PPROCESSINFO gppiWantForegroundPriority;
extern PPROCESSINFO gppiForegroundOld;

extern PW32JOB      gpJobsList;


extern UINT_PTR  gtmridAniCursor;
extern PHOTKEY gphkFirst;

extern int           gcHotKey;
extern PHOTKEYSTRUCT gpHotKeyList;
extern int           gcHotKeyAlloc;
extern PIMEHOTKEYOBJ gpImeHotKeyListHeader;


extern int gMouseSpeed;
extern int gMouseThresh1;
extern int gMouseThresh2;
extern int gMouseSensitivityFactor;
extern int gMouseSensitivity;

extern UINT   guDdeSendTimeout;


extern INT   gnFastAltTabRows;    /* Rows of icons in quick switch window     */
extern INT   gnFastAltTabColumns; /* Columns of icons in quick switch window  */

extern DWORD   gdwThreadEndSession; /* Shutting down system?                    */

extern HBRUSH  ghbrHungApp;       /* Brush used to redraw hung app windows.   */

extern HBITMAP ghbmBits;
extern HBITMAP ghbmCaption;

extern int     gcxCaptionFontChar;
extern int     gcyCaptionFontChar;
extern HFONT   ghSmCaptionFont;
extern int     gcxSmCaptionFontChar;
extern int     gcySmCaptionFontChar;

extern HFONT   ghMenuFont;
extern HFONT   ghMenuFontDef;
extern int     gcxMenuFontChar;
extern int     gcyMenuFontChar;
extern int     gcxMenuFontOverhang;
extern int     gcyMenuFontExternLeading;
extern int     gcyMenuFontAscent;
extern int     gcyMenuScrollArrow;

#if DBG
extern UINT  guModalMenuStateCount;
#endif

/*
 * From mnstate.c
 */
extern POPUPMENU gpopupMenu;
extern MENUSTATE gMenuState;

extern HFONT ghStatusFont;
extern HFONT ghIconFont;

/*
 * Cached SMWP structure
 */
extern SMWP gSMWP;

extern DWORD gpdwCPUserPreferencesMask [SPI_BOOLMASKDWORDSIZE];

extern PROFILEVALUEINFO gpviCPUserPreferences [1 + SPI_DWORDRANGECOUNT];

/*
 * Sys expunge control data.
 */
extern DWORD gdwSysExpungeMask;    // hmods to be expunged
extern DWORD gcSysExpunge;         // current count of expunges performed

/*
 * System classes
 */
extern PCLS gpclsList;

extern PCURSOR gpcurFirst;

extern SYSCFGICO gasyscur[];
extern SYSCFGICO gasysico[];

/*
 * Screen Saver Info
 */
extern PPROCESSINFO gppiScreenSaver;
extern POINT        gptSSCursor;

/*
 * Orphaned fullscreen mode changes that DDraw used to cleanup.
 */
extern PPROCESSINFO gppiFullscreen;

/*
 * accessibility byte-size data
 */
extern BYTE  gLastVkDown;
extern BYTE  gBounceVk;
extern BYTE gPhysModifierState;
extern BYTE gCurrentModifierBit;
extern BYTE gPrevModifierState;
extern BYTE gLatchBits;
extern BYTE gLockBits;
extern BYTE  gTKScanCode;
extern BYTE  gMKPreviousVk;
extern BYTE  gbMKMouseMode;

extern PSCANCODEMAP gpScancodeMap;

extern BYTE gStickyKeysLeftShiftCount;  // # of consecutive left shift key presses.
extern BYTE gStickyKeysRightShiftCount; // # of consecutive right shift key presses.


/*
 * Some terminal data is global in non-Hydra.
 */
extern DWORD               gdwGTERMFlags;   // GTERMF_ flags
extern PTHREADINFO         gptiRit;
extern PDESKTOP            grpdeskRitInput;
extern PKEVENT             gpkeHidChange;

extern PKEVENT             gpkeMouseData;

/*
 * Video Information
 */
extern BYTE                gbFullScreen;
extern PDISPLAYINFO        gpDispInfo;

/*
 * Count of available cacheDC's. This is used in determining a threshold
 * count of DCX_CACHE types available.
 */
extern int                 gnDCECount;

extern int                 gnRedirectedCount;

/*
 * Hung redraw list
 */
extern  PVWPL  gpvwplHungRedraw;

/*
 * SetWindowPos() related globals
 */
extern HRGN    ghrgnInvalidSum;
extern HRGN    ghrgnVisNew;
extern HRGN    ghrgnSWP1;
extern HRGN    ghrgnValid;
extern HRGN    ghrgnValidSum;
extern HRGN    ghrgnInvalid;

extern HRGN    ghrgnInv0;               // Temp used by InternalInvalidate()
extern HRGN    ghrgnInv1;               // Temp used by InternalInvalidate()
extern HRGN    ghrgnInv2;               // Temp used by InternalInvalidate()

extern HDC     ghdcMem;
extern HDC     ghdcMem2;

/*
 * DC Cache related globals
 */
extern HRGN    ghrgnGDC;                // Temp used by GetCacheDC et al

/*
 * SPB related globals
 */
extern HRGN    ghrgnSCR;                // Temp used by SpbCheckRect()
extern HRGN    ghrgnSPB1;
extern HRGN    ghrgnSPB2;

/*
 * ScrollWindow/ScrollDC related globals
 */
extern HRGN    ghrgnSW;              // Temps used by ScrollDC/ScrollWindow
extern HRGN    ghrgnScrl1;
extern HRGN    ghrgnScrl2;
extern HRGN    ghrgnScrlVis;
extern HRGN    ghrgnScrlSrc;
extern HRGN    ghrgnScrlDst;
extern HRGN    ghrgnScrlValid;

/*
 * General Device and Driver information
 */
extern PDEVICEINFO gpDeviceInfoList;
extern PERESOURCE  gpresDeviceInfoList;
#if DBG
extern DWORD gdwDeviceInfoListCritSecUseCount;
extern DWORD gdwInAtomicDeviceInfoListOperation;
#endif
extern PDRIVER_OBJECT gpWin32kDriverObject;
extern DWORD gnRetryReadInput;

/*
 * Mouse Information
 */
extern MOUSEEVENT  gMouseEventQueue[];
extern DWORD       gdwMouseQueueHead;
extern DWORD       gdwMouseEvents;
extern PERESOURCE  gpresMouseEventQueue;
extern int         gnMice;

/*
 * Keyboard Information
 */
extern KEYBOARD_ATTRIBUTES             gKeyboardInfo;
extern KEYBOARD_INPUT_DATA             gkei[];
extern KEYBOARD_INDICATOR_PARAMETERS   gklp;
extern KEYBOARD_INDICATOR_PARAMETERS   gklpBootTime;
extern KEYBOARD_TYPEMATIC_PARAMETERS   gktp;
extern int                             gnKeyboards;
extern IO_STATUS_BLOCK                 giosbKbdControl;
/*
 * IME status for keyboard device
 */
extern KEYBOARD_IME_STATUS             gKbdImeStatus;


extern BYTE gafAsyncKeyState[];
extern BYTE gafAsyncKeyStateRecentDown[];

extern BYTE gafRawKeyState[];

extern BOOLEAN gfKanaToggle;

extern DWORD               gdwUpdateKeyboard;
extern HARDERRORHANDLER    gHardErrorHandler;

extern UINT      gfsSASModifiers;     // SAS modifiers
extern UINT      gfsSASModifiersDown; // SAS modifiers really physically down
extern UINT      gvkSAS;              // The Secure Attention Sequence (SAS) key.

// FE_IME
extern DWORD     gdwIMEOpenStatus;
extern DWORD     gdwIMEConversionStatus;
extern HIMC      gHimcFocus;
extern BOOL      gfIMEShowStatus;
// end FE_IME


extern MOUSEMOVEPOINT gaptMouse[];
extern UINT gptInd;


extern CONST ALWAYSZERO gZero;
extern KBDTABLES KbdTablesFallback;
extern CONST HANDLETYPEINFO gahti[];

/*
 * Security data
 */

extern CONST GENERIC_MAPPING KeyMapping;

extern CONST GENERIC_MAPPING WinStaMapping;
extern PSECURITY_DESCRIPTOR gpsdInitWinSta;

extern PPUBOBJ gpPublicObjectList;

extern CONST PROC apfnSimpleCall[];
extern CONST ULONG ulMaxSimpleCall;

extern PRIVILEGE_SET psTcb;

extern PVOID Win32KBaseAddress;

extern CONST GENERIC_MAPPING DesktopMapping;

extern CONST SFNSCSENDMESSAGE gapfnScSendMessage[];

extern PSMS gpsmsList;

extern TERMINAL gTermIO;
extern TERMINAL gTermNOIO;

extern PTERMINAL gpMainTerminal;
extern PWINDOWSTATION grpWinStaList;
/*
 * the logon desktop
 */
extern PDESKTOP grpdeskLogon;


extern HANDLE CsrApiPort;
extern CONST LUID luidSystem;

extern PKBDFILE gpkfList;        // Currently loaded keyboard layout files.

extern PTHREADINFO gptiCurrent;
extern PTIMER gptmrFirst;
extern PKTIMER gptmrMaster;
extern INT gdmsNextTimer, gcmsLastTimer;
extern BOOL gbMasterTimerSet;

extern BOOL gbDisableAlpha;

extern LONG gUserProcessHandleQuota;
extern DWORD gUserPostMessageLimit;

/*
 * Active Accessibility - Window Events
 */
extern PEVENTHOOK gpWinEventHooks;    // list of installed hooks
extern PNOTIFY gpPendingNotifies;     // FILO of outstanding notifications
extern PNOTIFY gpLastPendingNotify;   // end of above list.
extern DWORD gdwDeferWinEvent;        // Defer notification is > 0

/*
 * This is the timeout value used for callbacks to low level hook procedures
 */
extern int gnllHooksTimeout;

// FE_SB
extern PUSHORT gpusMouseVKey;  //= (PUSHORT) ausMouseVKey;

extern USHORT  gNumLockVk;     // = VK_NUMLOCK;
extern USHORT  gOemScrollVk;   // = VK_OEM_SCROLL;
// FE_SB

extern CONST WCHAR szOneChar[];
extern CONST WCHAR szY[];
extern CONST WCHAR szy[];
extern CONST WCHAR szN[];

extern CONST WCHAR szNull[];

extern WCHAR szWindowStationDirectory[MAX_SESSION_PATH];

extern HBRUSH  ghbrWhite;
extern HBRUSH  ghbrBlack;
extern HFONT ghFontSys;

extern HANDLE hModuleWin;        // win32k.sys hmodule
extern HANDLE hModClient;        // user32.dll hModule

extern LONG TraceInitialization;

extern DESKTOPINFO diStatic;

extern ULONG gdwDesktopId;

extern PERESOURCE gpresUser;
extern PFAST_MUTEX gpHandleFlagsMutex;

extern PROC gpfnwp[];

#ifdef HUNGAPP_GHOSTING
extern PKEVENT gpEventScanGhosts;
extern ATOM gatomGhost;
#endif // HUNGAPP_GHOSTING

extern ATOM gatomConsoleClass;
extern ATOM gatomFirstPinned;
extern ATOM gatomLastPinned;

extern ATOM gatomMessage;
extern ATOM gaOleMainThreadWndClass;
extern ATOM gaFlashWState;
extern ATOM atomCheckpointProp;
extern ATOM atomDDETrack;
extern ATOM atomQOS;
extern ATOM atomDDEImp;
extern ATOM atomWndObj;
extern ATOM atomImeLevel;

#ifdef POOL_INSTR
extern DWORD gdwAllocCrt;          // the index for the current allocation
#endif // POOL_INSTR

extern UINT guiOtherWindowCreated;
extern UINT guiOtherWindowDestroyed;
extern UINT guiActivateShellWindow;

extern ATOM atomUSER32;
extern ATOM atomLayer;

extern HANDLE gpidLogon;
extern PEPROCESS gpepInit;

extern PEPROCESS gpepCSRSS;

extern int giLowPowerTimeOutMs;
extern int giPowerOffTimeOutMs;

extern PSERVERINFO gpsi;
extern SHAREDINFO gSharedInfo;

extern DWORD giheLast;           /* index to last allocated entry */

extern DWORD gdwDesktopSectionSize;
extern DWORD gdwNOIOSectionSize;

#if defined (USER_PERFORMANCE)
extern CSSTATISTICS gCSStatistics;
#endif // USER_PERFORMANCE

extern SECURITY_QUALITY_OF_SERVICE gqosDefault;  // system default DDE qos.

extern CONST COLORREF gargbInitial[];

extern POWERSTATE gPowerState;

extern WCHAR gszMIN[15];
extern WCHAR gszMAX[15];
extern WCHAR gszRESUP[20];
extern WCHAR gszRESDOWN[20];
/* Commented out due to TandyT ...
 * extern WCHAR gszSMENU[30];
 */
extern WCHAR gszSCLOSE[15];
extern WCHAR gszCAPTIONTOOLTIP[CAPTIONTOOLTIPLEN];

/*
 * Pointer to shared SERVERINFO data.
 */
extern HANDLE ghSectionShared;
extern PVOID  gpvSharedBase;
extern PWIN32HEAP gpvSharedAlloc;

extern BOOL gbVideoInitialized;

extern BOOL gbNoMorePowerCallouts;

extern BOOL gbCleanedUpResources;

extern WSINFO gWinStationInfo;

extern ULONG    gSessionId;
extern BOOL     gbRemoteSession;
extern BOOL    gfEnableWindowsKey;

extern PDESKTOP gspdeskDisconnect;

extern HANDLE  ghRemoteVideoChannel;

extern HANDLE  ghRemoteMouseChannel;
extern HANDLE  ghRemoteKeyboardChannel;
extern HANDLE  ghRemoteBeepChannel;
extern PVOID   gpRemoteBeepDevice;
extern HANDLE  ghRemoteThinwireChannel;
extern CLIENTKEYBOARDTYPE gRemoteClientKeyboardType;

extern BOOL gbExitInProgress;

extern PDESKTOP gspdeskShouldBeForeground;
extern BOOL     gbDesktopLocked;
extern BOOL     gbSecureDesktop;

extern BOOL  gbFreezeScreenUpdates;
extern ULONG gSetLedReceived;
extern BOOL  gbClientDoubleClickSupport;

extern BOOL gbDisconnectHardErrorAttach;

extern PKEVENT gpevtDesktopDestroyed;

extern HDESK   ghDisconnectDesk;
extern HWINSTA ghDisconnectWinSta;

extern ULONG gnShadowers;
extern BOOL  gbConnected;

extern WCHAR gstrBaseWinStationName[];

extern PFILE_OBJECT gVideoFileObject;
extern PFILE_OBJECT gThinwireFileObject;

extern PVOID gpThinWireCache;

extern PVOID ghKbdTblBase;
extern ULONG guKbdTblSize;

typedef struct tagGRAPHICS_DEVICE *PGRAPHICS_DEVICE;

typedef struct tagGRAPHICS_DEVICE {

    WCHAR            szNtDeviceName[16];  // NT device name (\\Device\\Videox)
    WCHAR            szWinDeviceName[16]; // user-mode name (\\DosDevices\\Displayx)

    //

    PGRAPHICS_DEVICE pNextGraphicsDevice; // Next device in the linked list.
    PGRAPHICS_DEVICE pVgaDevice;          // If this device is VGA compatible
                                          // and uses another isntance to operate
                                          // in VGA mode
    HANDLE           pDeviceHandle;       // Handle for the device
    PVOID            pFileObject;         // Handle for the device

    //

    DWORD            stateFlags;          // Flags describing the state of the
                                          // device
    ULONG            cbdevmodeInfo;       // Size of the devmode information
    PDEVMODEW        devmodeInfo;         // Pointer to the current list of modes
                                          // for the device
    LPWSTR           DiplayDriverNames;   // Pointer to MULTI_SZ with DD names.

} GRAPHICS_DEVICE, *PGRAPHICS_DEVICE;
extern GRAPHICS_DEVICE   gFeFullscreenGraphicsDevice;

extern DWORD gdwHydraHint;

extern DWORD gdwCanPaintDesktop;

extern HANDLE ghCanActivateForegroundPIDs[];

extern DWORD gdwGuiThreads;

/*
 * Debug only globals
 */
#if DBG
extern DWORD gdwCritSecUseCount;
extern DWORD gdwInAtomicOperation;
extern LPCSTR gapszFNID[];
extern LPCSTR gapszMessage[];
extern int gnDeferredWinEvents;

extern BOOL gbTraceHydraApi;
extern BOOL gbTraceDesktop;
extern BOOL gbTraceRIT;

/*
 * The total number of thread locks for all threads in the system at a certain time
 * should be less than MAX_THREAD_LOCKS*MAX_THREAD_LOCKS_ARRAYS
 */
#define MAX_THREAD_LOCKS        1000
#define MAX_THREAD_LOCKS_ARRAYS 100

extern PTL gpaThreadLocksArrays[MAX_THREAD_LOCKS_ARRAYS];
extern PTL gFreeTLList;
extern int gcThreadLocksArraysAllocated;

#endif  // DBG

#endif // _GLOBALS_
