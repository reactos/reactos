/******************************************************************************
Module name: Access.h
Purpose: defines for all accstat
******************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <cpl.h>
//#include <shellapi.h>
#include <commctrl.h>

#include "acchelp.h"
#include "resource.h"

//////////////////////////////////////////////////////////////////////////////


#define ARRAY_SIZE(a)      (sizeof(a) / sizeof((a)[0]))
#define MEMBER_SIZE(s,m)   sizeof(((s *)0)->m)

// Debugging defs
#if defined(DEBUG) || defined(MEMCHECK) || defined(_DEBUG)

//Assert based on boolean f.
#define Assert(f) assert(f)

#else

#define Assert(f) ((void)0)

#endif


//////////////////////////////////////////////////////////////////////////////

extern BOOL g_SPISetValue;
extern HINSTANCE g_hinst;
extern BOOL      g_fWinNT;       // TRUE if we're running on NT and must disable
                                 // some features.

extern const DWORD g_aIds[];     // array mapping control id's to help id's

// This functions makes it easy to access the registry.
int  WINAPI RegQueryInt(int nDefault, HKEY hkey, LPTSTR lpSubKey, LPTSTR lpValueName);
BOOL WINAPI RegSetInt(HKEY hkey, LPTSTR lpSubKey, LPTSTR lpValueName, int nVal);

void WINAPI RegQueryStr(
    LPTSTR lpDefault, 
    HKEY hkey, 
    LPTSTR lpSubKey, 
    LPTSTR lpValueName,
    LPTSTR lpszValue, 
    DWORD cbData);

BOOL RegSetStr(HKEY hkey, LPCTSTR lpSection, LPCTSTR lpKeyName, LPCTSTR lpString);

DWORD WINAPI RegQueryStrDW(
    DWORD dwDefault,
    HKEY hkey, 
    LPTSTR lpSubKey, 
    LPTSTR lpValueName);

BOOL RegSetStrDW(HKEY hkey, LPTSTR lpSection, LPCTSTR lpKeyName, DWORD dwValue);

   // This function takes the current state of the variables below
// and updates the system settings.
void WINAPI SetAccessibilitySettings (void);

// These values are declared in General.c
extern BOOL  g_fSaveSettings;
extern BOOL  g_fShowWarnMsgOnFeatureActivate;
extern BOOL  g_fPlaySndOnFeatureActivate;
// Admin options in general
extern BOOL  g_fCopyToLogon;
extern BOOL  g_fCopyToDefault;

// Keyboard property page
extern STICKYKEYS     g_sk;
extern FILTERKEYS     g_fk;
   // g_dwLastBounceKeySetting is part of FilterKeys
   extern DWORD g_dwLastBounceKeySetting;
   extern DWORD g_nLastRepeatDelay;
   extern DWORD g_nLastRepeatRate;
   extern DWORD g_nLastWait;

extern TOGGLEKEYS     g_tk;
extern BOOL           g_fExtraKeyboardHelp;

// Sound Property page
extern SOUNDSENTRY    g_ss;
extern BOOL           g_fShowSounds;

// Display Property page
extern HIGHCONTRAST   g_hc;
extern TCHAR          g_szScheme[256];

// Mouse Property page
extern MOUSEKEYS      g_mk;

// General Property page
extern ACCESSTIMEOUT  g_ato;
extern SERIALKEYS     g_serk;
extern TCHAR          g_szActivePort[256];
//extern TCHAR        g_szPort[256];  // Currently reserved, should be NULL.


//////////////////////////////////////////////////////////////////////////////


// AccessSystemParametersInfo is actually defined in AccRare.c and is
// a replacement for SysParamInfoBugFix

BOOL AccessSystemParametersInfo(
	UINT wFlag,
	DWORD wParam,
	PVOID lParam,
	UINT flags);

//////////////////////////////////////////////////////////////////////////////


// Define strings for registry.
#define GENERAL_KEY           __TEXT("Control Panel\\Accessibility")
#define FILTER_KEY            __TEXT("Control Panel\\Accessibility\\Keyboard Response")
#define HC_KEY                __TEXT("Control Panel\\Accessibility\\HighContrast")
#define CONTROL_KEY           __TEXT("Control Panel\\Appearance\\Schemes")
#define WARNING_SOUNDS        __TEXT("Warning Sounds")
#define SOUND_ON_ACTIVATION   __TEXT("Sound on Activation")
#define APPLY_GLOBALLY        __TEXT("Restore Settings")
#define NORMALSCHEME          __TEXT("Current Normal Scheme")
#define HIGHCONTRAST_SCHEME   __TEXT("High Contrast Scheme")
#define VOLATILE_SCHEME       __TEXT("Volital HC Scheme")
#define WHITEBLACK_HC         __TEXT("High Contrast Black (large)")
#define LAST_BOUNCE_SETTING   __TEXT("Last BounceKey Setting")
#define LAST_REPEAT_RATE      __TEXT("Last Valid Repeat")
#define LAST_REPEAT_DELAY     __TEXT("Last Valid Delay")
#define LAST_WAIT             __TEXT("Last Valid Wait")
#define LAST_CUSTOM_SCHEME    __TEXT("Last Custom Scheme")


#define IDSENG_BLACKWHITE_SCHEME   __TEXT("High Contrast White (large)")
#define IDSENG_WHITEBLACK_SCHEME   __TEXT("High Contrast Black (large)")



//////////////////////////////////////////////////////////////////////////////


// Define prototypes
INT_PTR WINAPI HighContrastDlg (HWND, UINT , WPARAM , LPARAM);
INT_PTR WINAPI ToggleKeySettingsDlg (HWND, UINT, WPARAM, LPARAM);
INT_PTR WINAPI StickyKeyDlg (HWND, UINT , WPARAM, LPARAM);
INT_PTR WINAPI FilterKeyDlg (HWND, UINT , WPARAM, LPARAM);
INT_PTR WINAPI MouseKeyDlg (HWND, UINT, WPARAM, LPARAM);
INT_PTR WINAPI SerialKeyDlg (HWND, UINT, WPARAM, LPARAM);

int HandleScroll (HWND hwnd, WPARAM wParam, HWND hwndScroll);


DWORD SaveDefaultSettings(BOOL saveL, BOOL saveU);
BOOL IsDefaultWritable(void);


typedef
LANGID (WINAPI *pfnGetUserDefaultUILanguage)(void);

typedef
LANGID (WINAPI *pfnGetSystemDefaultUILanguage)(void);

BOOL IsMUI_Enabled();




///////////////////////////////// End of File /////////////////////////////////
