//
//  QuickRes.h
//

#include <windows.h>

#include <shellapi.h>
#include "QuickRes.rc"
#include "stdlib.h"
#include "tchar.h"

#define ODS OutputDebugString

#define TRAY_MSG                WM_USER+1
#define TRAY_ID                 42

#define DBT_CONFIGCHANGED       0x0018     // for WM_DEVICECHANGED message
#define DBT_MONITORCHANGE       0x001B


//
//  Global Quickres bit flags
//

#define QF_SHOWRESTART          0x0001     // Show modes that require a restart
#define QF_UPDATEREG            0x0002     // update registry with new devmode
#define QF_REMMODES             0x0004     // Remember good/bad modes in registry
#define QF_SORT_BYBPP           0x0008     // if not set, we sort by Resolution
#define QF_HIDE_4BPP            0x0010     // Hide 4Bpp mode if 8Bpp of same res exists
#define QF_SHOWTESTED           0x0020     // Show tested/passing modes only
#define QF_SHOWFREQS            0x0040     // Show Frequencies (disabled on Win95)


//
//  fGoodModes (below) depends on these values for these flags
//  Changing these constants requires rewriting the fGoodModes macro
//

#define MODE_INVALID             0         // Devmode is not visible
#define MODE_VALID               1         // Devmode looks good
#define MODE_UNTESTED            2         // Haven't tried it yet
#define MODE_BESTHZ              3         // Best Hz for given res/bpp


#define RESOURCE_STRINGLEN       512       // Guess at largest resource string length

#define KEEP_RES_TIMEOUT         15        // how long before reverting to old devmode

#define INT_FORMAT_TO_5_DIGITS   10        // Need 3+ more bytes : "%d" -> "12345"
                                           // Being safe here (add 10 bytes)


//
//  Constant strings in registry & for starting properties
//

#define REGSTR_SOFTWARE    TEXT("Software\\Microsoft")
#define REGSTR_QUICKRES    TEXT("Software\\Microsoft\\QuickRes")
#define QUICKRES_KEY       TEXT("QuickRes")
#define DISPLAYPROPERTIES  TEXT("rundll32 shell32.dll,Control_RunDLL desk.cpl,,3")
#define REGDEVMODES        TEXT("GoodDevmodes")
#define REGBPP             TEXT("BPP")
#define REGFLAGS           TEXT("Flags")

#ifdef UNICODE
#define ENUMDISPLAYDEVICES "EnumDisplayDevicesW"
#else
#define ENUMDISPLAYDEVICES "EnumDisplayDevicesA"
#endif


//
//  Each devmode has 4 additional properties.
//

typedef struct _DEVMODEINFO {

    DEVMODE dm;
    UINT    uFreqMenu;
    UINT    uMenuItem;
    UINT    uCDSTest;
    UINT    uValidMode;

}  DEVMODEINFO, *LPDEVMODEINFO;


//
// Per monitor information :
//   devicename, monitorname
//   devmode menu and freq submenus built on the fly
//   iModes : number of devmodes
//   pModes : array of devmodes display can handle
//   pCurrentdm : pointer (in pModes) to current devmode
//   bPrimary : primary or not?
//

typedef struct _QRMONITORINFO
{
    LPTSTR         DeviceName;
    LPTSTR         MonitorName;
    HMENU          ModeMenu;
    HMENU          *FreqMenu;
    INT            iModes;
    LPDEVMODEINFO  pModes;
    LPDEVMODEINFO  pCurrentdm;
    BOOL           bPrimary;

}  QRMONITORINFO, *LPQRMONITORINFO;



//
// prototypes
//
// quickres.c
//

HMENU    GetModeMenu ( INT, BOOL );
HMENU    GetMonitorMenu ( BOOL );
BOOL     BuildDevmodeList ( VOID );
BOOL     TrayMessage( HWND, DWORD, UINT, HICON );
int      MsgBox( int, UINT, UINT );
VOID     CheckMenuItemCurrentMode( INT );
PDEVMODE GetCurrentDevMode( INT, PDEVMODE );
LPTSTR   GetResourceString( UINT );
VOID     DestroyModeMenu( INT, BOOL, BOOL );
VOID     AppendMainMenu( VOID );

INT_PTR FAR PASCAL KeepNewResDlgProc( HWND, UINT, WPARAM, LPARAM );
INT_PTR FAR PASCAL NTOptionsDlgProc(  HWND, UINT, WPARAM, LPARAM );
INT_PTR FAR PASCAL W95OptionsDlgProc( HWND, UINT, WPARAM, LPARAM );


//
// registry.c
//

VOID SetDevmodeFlags ( INT, BOOL );
VOID GetDevmodeFlags ( INT );
VOID SetQuickResFlags( VOID );
VOID GetQuickResFlags( VOID );
VOID SetRegistryValue( LPTSTR, UINT, PVOID, UINT );
VOID SaveAllSettings ( VOID );


//
//Macros
//

#define fShowModesThatNeedRestart (QuickResFlags & QF_SHOWRESTART)
#define fUpdateReg                (QuickResFlags & QF_UPDATEREG)
#define fRememberModes            (QuickResFlags & QF_REMMODES)
#define fSortByBPP                (QuickResFlags & QF_SORT_BYBPP)
#define fHide4BppModes            (QuickResFlags & QF_HIDE_4BPP)
#define fShowTestedModes          (QuickResFlags & QF_SHOWTESTED)
#define fShowFreqs                (QuickResFlags & QF_SHOWFREQS)

//
// Devmode info
//

#define BPP(x)  ((x)->dmBitsPerPel)
#define XRES(x) ((x)->dmPelsWidth)
#define YRES(x) ((x)->dmPelsHeight)
#define HZ(x)   ((x)->dmDisplayFrequency)

#define FREQMENU(x)   ((x)->uFreqMenu)
#define MENUITEM(x)   ((x)->uMenuItem)
#define CDSTEST(x)    ((x)->uCDSTest)
#define VALIDMODE(x)  ((x)->uValidMode)


//
//  Must leave MODE_VALID=1, MODE_BESTHZ=3.  
//  Other MODE_* constants should be even

#define fGoodMode(x)  ((x)->uValidMode & 0x1)
