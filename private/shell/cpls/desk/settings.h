#ifdef __cplusplus
extern "C" {
#endif

//==========================================================================
//                          Definitions
//==========================================================================
#define IDS_TEST_BITMAP       3
#define IDS_COLOR_RED         4
#define IDS_COLOR_GREEN       5
#define IDS_COLOR_BLUE        6
#define IDS_COLOR_YELLOW      7
#define IDS_COLOR_MAGENTA     8
#define IDS_COLOR_CYAN        9
#define IDS_COLOR_WHITE       10
#define IDS_PATTERN_VERT      11
#define IDS_PATTERN_HORZ      12
#define IDS_RED_SHADES        13
#define IDS_GREEN_SHADES      14
#define IDS_BLUE_SHADES       15
#define IDS_RESOLUTION_FMT    16
#define IDS_NO_VERSION_INFO   17
#define IDS_UNAVAILABLE       18
#define IDS_DEFDRIVE          19
#define IDS_GRAY_SHADES       20
#define IDS_DISPLAYINST       21
#define IDS_WINNTDEV_INSTRUCT 22
#define IDS_SELECTDEV_LABEL   23

#define DISPLAYICON     1

//
// Color defintions
//
#define IDB_COLORMIN    IDB_COLOR1
#define IDB_COLORMAX    IDB_COLOR8


//
// New message IDs
//

#define MSG_DSP_SETUP_MESSAGE    WM_USER + 0x200



#define SZ_REBOOT_NECESSARY     TEXT("System\\CurrentControlSet\\Control\\GraphicsDrivers\\RebootNecessary")
#define SZ_INVALID_DISPLAY      TEXT("System\\CurrentControlSet\\Control\\GraphicsDrivers\\InvalidDisplay")
#define SZ_DETECT_DISPLAY       TEXT("System\\CurrentControlSet\\Control\\GraphicsDrivers\\DetectDisplay")
#define SZ_NEW_DISPLAY          TEXT("System\\CurrentControlSet\\Control\\GraphicsDrivers\\NewDisplay")
#define SZ_DISPLAY_4BPP_MODES   TEXT("System\\CurrentControlSet\\Control\\GraphicsDrivers\\Display4BppModes")
#define SZ_CONFIGURE_AT_LOGON   TEXT("ConfigureAtLogon")

#define SZ_UNATTEND_INSTALL     TEXT("InstallDriver")
#define SZ_UNATTEND_INF         TEXT("InfFile")
#define SZ_UNATTEND_OPTION      TEXT("InfOption")
#define SZ_UNATTEND_BPP         TEXT("BitsPerPel")
#define SZ_UNATTEND_X           TEXT("XResolution")
#define SZ_UNATTEND_Y           TEXT("YResolution")
#define SZ_UNATTEND_REF         TEXT("VRefresh")
#define SZ_UNATTEND_FLAGS       TEXT("Flags")
#define SZ_UNATTEND_CONFIRM     TEXT("AutoConfirm")

#define SZ_VIDEOMAP             TEXT("HARDWARE\\DEVICEMAP\\VIDEO")
#define SZ_GROUPORDERLIST       TEXT("SYSTEM\\CurrentControlSet\\Control\\GroupOrderList")
#define SZ_VIDEO                TEXT("Video")
#define SZ_FONTDPI              TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontDPI")
#define SZ_FONTDPI_PROF         TEXT("SYSTEM\\CurrentControlSet\\Hardware Profiles\\Current\\Software\\Fonts")
#define SZ_LOGPIXELS            TEXT("LogPixels")
#define SZ_DEVICEDESCRIPTION    TEXT("Device Description")
#define SZ_INSTALLEDDRIVERS     TEXT("InstalledDisplayDrivers")
#define SZ_REGISTRYMACHINE      TEXT("\\REGISTRY\\MACHINE\\")
#define SZ_SERVICES             TEXT("\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\")
#define SZ_BACKBACKDOT          TEXT("\\\\.\\")
#define SZ_DOTSYS               TEXT(".sys")
#define SZ_DOTDLL               TEXT(".dll")

#define SZ_FILE_SEPARATOR       TEXT(", ")



//==========================================================================
//                          Typedefs
//==========================================================================

typedef enum {
    RET_NO_CHANGE,
    RET_CHANGED,
} DLGRET;

typedef struct {
    int cPix;       // logical pixel size
    int iCombo;     // index of this choice in the combo box
    int idString;   // string id for the combo box choice
} FONTSIZE;

typedef struct _HARDWARE_INFO {
    LPTSTR MemSize;
    LPTSTR ChipType;
    LPTSTR DACType;
    LPTSTR AdapString;
    LPTSTR BiosString;
} HARDWARE_INFO, *PHARDWARE_INFO;

typedef struct _NEW_DESKTOP_PARAM {
    LPDEVMODE lpdevmode;
    LPTSTR pwszDevice;
} NEW_DESKTOP_PARAM, *PNEW_DESKTOP_PARAM;



extern HWND ghwndPropSheet;

void DeleteDisplayDialogBox( LPARAM lParam );
DLGPROC NewDisplayDialogBox(HINSTANCE hmod, LPARAM *lplParam);

typedef enum {
    EXEC_NORMAL = 1,
    EXEC_SETUP,
    EXEC_DETECT,
    EXEC_INVALID_MODE,
} EXEC_MODE;

typedef enum {
    NOT_INVALID = 1,
    EXEC_INVALID_NEW_DRIVER,
    EXEC_INVALID_DEFAULT_DISPLAY_MODE,
    EXEC_INVALID_DISPLAY_DRIVER,
    EXEC_INVALID_OLD_DISPLAY_DRIVER,
    EXEC_INVALID_16COLOR_DISPLAY_MODE,
    EXEC_INVALID_DISPLAY_MODE,
    EXEC_INVALID_CONFIGURATION,
    EXEC_INVALID_DISPLAY_DEVICE,
} EXEC_INVALID_SUBMODE;

extern EXEC_MODE gbExecMode;
extern EXEC_INVALID_SUBMODE gbInvalidMode;


//==========================================================================
//                              Macros
//==========================================================================

//This macro casts a TCHAR to be a 32 bit value with the hi word set to 0
// it is useful for calling CharUpper().

#define CHARTOPSZ(ch)       ((LPTSTR)(DWORD)(WORD)(ch))

//==========================================================================
//                          Function Prototypes
//==========================================================================
void DisplayDialogBox(HINSTANCE hmod, HWND hwndParent);

BOOL CALLBACK DisplayDlgProc (HWND hwnd, UINT msg, WPARAM wParam,
                              LPARAM lParam);

BOOL DeviceListCallback(void *lpDeviceName, DWORD dwData);
BOOL ModeCallback( void *lpDevmode, DWORD dwData);
BOOL DevNameCallback( void *lpDeviceName, DWORD dwData);
BOOL CleanupCallback( void *lpDeviceName, DWORD dwData);
BOOL ModeListCallback( void *lpDevmode, DWORD dwData);

int FmtMessageBox(HWND hwnd, UINT fuStyle, BOOL fSound,
                  DWORD dwTitleID, DWORD dwTextID, ... );

LPTSTR FmtSprint(DWORD id, ...);

LPTSTR CloneString(LPTSTR psz);


//==========================================================================
//                   Prototypes from drawbmp.c
//==========================================================================

VOID Set1152Mode(int height);
VOID DrawBmp(HDC hdc);


//==========================================================================
//                   Prototypes from install.c
//==========================================================================

DWORD
InstallNewDriver(
    HWND    hwnd,
    LPCTSTR pszModel,
    PBOOL   pbKeepEnabled
    );

DWORD
InstallGraphicsDriver(
    HWND    hwnd,
    LPCTSTR pszModel,
    LPCTSTR pszInf
    );

#ifdef __cplusplus
}
#endif

