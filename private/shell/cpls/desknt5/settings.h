#ifdef __cplusplus
extern "C" {
#endif

//
// New message IDs
//

#define MSG_DSP_SETUP_MESSAGE    WM_USER + 0x200



#define SZ_REBOOT_NECESSARY     TEXT("System\\CurrentControlSet\\Control\\GraphicsDrivers\\RebootNecessary")
#define SZ_INVALID_DISPLAY      TEXT("System\\CurrentControlSet\\Control\\GraphicsDrivers\\InvalidDisplay")
#define SZ_DETECT_DISPLAY       TEXT("System\\CurrentControlSet\\Control\\GraphicsDrivers\\DetectDisplay")
#define SZ_NEW_DISPLAY          TEXT("System\\CurrentControlSet\\Control\\GraphicsDrivers\\NewDisplay")
#define SZ_DISPLAY_4BPP_MODES   TEXT("System\\CurrentControlSet\\Control\\GraphicsDrivers\\Display4BppModes")

#define SZ_VIDEOMAP             TEXT("HARDWARE\\DEVICEMAP\\VIDEO")
#define SZ_VIDEO                TEXT("Video")
#define SZ_FONTDPI              TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontDPI")
#define SZ_FONTDPI_PROF         TEXT("SYSTEM\\CurrentControlSet\\Hardware Profiles\\Current\\Software\\Fonts")
#define SZ_LOGPIXELS            TEXT("LogPixels")
#define SZ_DEVICEDESCRIPTION    TEXT("Device Description")
#define SZ_INSTALLEDDRIVERS     TEXT("InstalledDisplayDrivers")
#define SZ_SERVICES             TEXT("\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\")
#define SZ_BACKBACKDOT          TEXT("\\\\.\\")
#define SZ_DOTSYS               TEXT(".sys")
#define SZ_DOTDLL               TEXT(".dll")

#define SZ_FILE_SEPARATOR       TEXT(", ")

#define SZ_DEVICE               TEXT("\\Device")
#define SZ_ENUM                 TEXT("Enum")

#define SZ_WINDOWMETRICS        TEXT("Control Panel\\Desktop\\WindowMetrics")
#define SZ_APPLIEDDPI           TEXT("AppliedDPI")

#define SZ_CONTROLPANEL         TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Control Panel")
#define SZ_ORIGINALDPI          TEXT("OriginalDPI")



//==========================================================================
//                          Typedefs
//==========================================================================


extern HWND ghwndPropSheet;

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
//                   Prototypes from drawbmp.c
//==========================================================================

VOID Set1152Mode(int height);
VOID DrawBmp(HDC hdc);


#ifdef __cplusplus
}
#endif

