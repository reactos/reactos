
#ifndef _TRAYP_INC
#define _TRAYP_INC


#define TBC_SETACTIVEALT            (WM_USER + 50)      //  50=0x32
#define TBC_VERIFYBUTTONHEIGHT      (WM_USER + 51)

#define WMTRAY_PROGCHANGE           (WM_USER + 200)     // 200=0xc8
#define WMTRAY_RECCHANGE            (WM_USER + 201)
#define WMTRAY_FASTCHANGE           (WM_USER + 202)
#define WMTRAY_PRINTCHANGE          (WM_USER + 203)
#define WMTRAY_DESKTOPCHANGE        (WM_USER + 204)

#define WMTRAY_COMMONPROGCHANGE     (WM_USER + 205)
#define WMTRAY_COMMONFASTCHANGE     (WM_USER + 206)

#define WMTRAY_FAVORITESCHANGE      (WM_USER + 207)

#define WMTRAY_PRINTICONNOTIFY      (WM_USER + 220)

#define WMTRAY_REGISTERHOTKEY       (WM_USER + 230)
#define WMTRAY_UNREGISTERHOTKEY     (WM_USER + 231)
#define WMTRAY_SETHOTKEYENABLE      (WM_USER + 232)
#define WMTRAY_SCREGISTERHOTKEY     (WM_USER + 233)
#define WMTRAY_SCUNREGISTERHOTKEY   (WM_USER + 234)
#define WMTRAY_QUERY_MENU           (WM_USER + 235)
#define WMTRAY_QUERY_VIEW           (WM_USER + 236)     // 236=0xec

#define TM_WINDOWDESTROYED          (WM_USER+0x100)
#define TM_POSTEDRCLICK             (WM_USER+0x101)
#define TM_CONTEXTMENU              (WM_USER+0x102)
#define TM_ACTASTASKSW              (WM_USER+0x104)
#define TM_SYSMENUCOUNT             (WM_USER+0x105)
#define TM_TASKTAB                  (WM_USER+0x106)

#define TM_RELAYPOSCHANGED          (WM_USER + 0x150)
// #define TM_INVALIDATEREBUILDMENU    (WM_USER + 0x151)
#define TM_BRINGTOTOP               (WM_USER + 0x152)
#define TM_WARNNOAUTOHIDE           (WM_USER + 0x153)
#define TM_WARNNODROP               (WM_USER + 0x154)
// #define TM_NEXTCTL                  (WM_USER + 0x155)
#define TM_DOEXITWINDOWS            (WM_USER + 0x156)
#define TM_SHELLSERVICEOBJECTS      (WM_USER + 0x157)
#define TM_DESKTOPSTATE             (WM_USER + 0x158)
#define TM_HANDLEDELAYBOOTSTUFF     (WM_USER + 0x159)
#define TM_GETHMONITOR              (WM_USER + 0x15a)

#ifdef DEBUG
#define TM_NEXTCTL                  (WM_USER + 0x15b)
#endif
#define TM_UIACTIVATEIO             (WM_USER + 0x15c)
#define TM_ONFOCUSCHANGEIS          (WM_USER + 0x15d)

#define TM_MARSHALBS                (WM_USER + 0x15e)
// was TM_THEATERMODE, do not reuse (WM_USER + 0x15f)
#define TM_KILLTIMER                (WM_USER + 0x160)
#define TM_REFRESH                  (WM_USER + 0x161)
#define TM_SETTIMER                 (WM_USER + 0x162)

#define TM_PRIVATECOMMAND           (WM_USER + 0x175)


#define Tray_GetHMonitor(hwndTray, phMon) \
        (DWORD)SendMessage((hwndTray), TM_GETHMONITOR, 0, (LPARAM)(HMONITOR *)phMon)


#ifdef ABM_NEW

typedef struct _TRAYAPPBARDATA
{
    APPBARDATA abd;
    DWORD dwMessage;
    HANDLE hSharedABD;
    DWORD dwProcId;
} TRAYAPPBARDATA, *PTRAYAPPBARDATA;

#endif

#define RRA_DEFAULT               0x0000
#define RRA_DELETE                0x0001        // delete each reg value when we're done with it
#define RRA_WAIT                  0x0002        // Wait for current item to finish before launching next item
#define RRA_SHELLSERVICEOBJECTS   0x0004        // treat as a shell service object instead of a command sting
#define RRA_NOUI                  0x0008        // prevents ShellExecuteEx from displaying error dialogs
#define RRA_RUNSUBKEYS            0x0010        // Run items in sub keys in alphabetical order
#define RRA_USEJOBOBJECTS         0x0020        // wait on job objects instead of process handles

typedef UINT RRA_FLAGS;

#endif // _TRAYP_INC
