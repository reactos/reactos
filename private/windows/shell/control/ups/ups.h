// NT UPS Applet
#define IDD_UPS_EXISTS              1000
#define IDD_UPS_PORTCB              1001
#define IDD_UPS_PFSIGNAL            1002
#define IDD_UPS_BATTERYLIFE         1003
#define IDD_UPS_BLTEXT1             1004
#define IDD_UPS_BLTEXT2             1005
#define IDD_UPS_RECHARGEPERMINUTE   1006
#define IDD_UPS_RPMTEXT1            1007
#define IDD_UPS_RPMTEXT2            1008
#define IDD_UPS_LOWBATTERY          1009
#define IDD_UPS_TURNOFF             1010
#define IDD_UPS_FIRSTWARNING        1012
#define IDD_UPS_WARNINGINTERVAL     1013
#define IDD_UPS_PFSIGNALHIGH        1014
#define IDD_UPS_PFSIGNALLOW         1015
#define IDD_UPS_LOWBATTERYHIGH      1016
#define IDD_UPS_LOWBATTERYLOW       1017
#define IDD_UPS_TURNOFFHIGH         1018
#define IDD_UPS_TURNOFFLOW          1019
#define IDD_UPS_TEXT                1020
#define IDD_UPS_SIGN                1021
#define IDD_UPS_FWTEXT1             1022
#define IDD_UPS_FWTEXT2             1023
#define IDD_UPS_WITEXT1             1024
#define IDD_UPS_WITEXT2             1025
#define IDD_UPS_UPSGROUP            1026
#define IDD_UPS_BLEDIT              1027
#define IDD_UPS_RPMEDIT             1028
#define IDD_UPS_FWEDIT              1029
#define IDD_UPS_WIEDIT              1030
#define IDD_UPS_CHARACTER           1031
#define IDD_UPS_SERVICE             1032
#define IDD_UPS_STATUS_TITLE        1033
#define IDD_UPS_STATUS              1034
#define IDD_UPS_BLANKGROUP          1035
#define IDD_UPS_COMMANDFILE         1036
#define IDD_UPS_FILETEXT            1037
#define IDD_UPS_FILENAME            1038

#define UPS_INSTALLED               0x00000001
#define UPS_POWERFAILSIGNAL         0x00000002
#define UPS_LOWBATTERYSIGNAL        0x00000004
#define UPS_CANTURNOFF              0x00000008
#define UPS_POWERFAIL_LOW           0x00000010
#define UPS_LOWBATTERY_LOW          0x00000020
#define UPS_TURNOFF_LOW             0x00000040
#define UPS_COMMANDFILE             0X00000080

#define UPSICON      100
#define CHILD_UPS       100
#define IDH_CHILD_UPS   (IDH_HELPFIRST + 200)
#define DLG_UPS       100

#define DEFAULTBATTERYLIFE          2
#define DEFAULTRECHARGEPERMINUTE    100
#define DEFAULTFIRSTWARNING         5
#define DEFAULTWARNINGINTERVAL      120

#define UPS_STATUS_ERROR            31
#define UPS_OPTIONS_ERROR           32
#define UPS_ACCESS_ERROR            33
#define UPS_REGISTRY_ERROR          34
#define UPS_SERVICE_ERROR           35
#define UPS_START_MSG               36
#define UPS_FW_WARNING              37
#define UPS_DELAY_WARNING           38
#define UPS_RESTART_MSG             39
#define UPS_STOP_MSG                40
#define UPS_FWRange                 41
#define UPS_WIRange                 42
#define UPS_BLRange                 43
#define UPS_RPMRange                44
#define UPS_PENDING_MSG             45
#define UPS_STARTFAIL_MSG           46
#define UPS_STOPFAIL_MSG            47
#define UPS_UNKNOWNSTATE_MSG        49
#define UPS_INVALID_PATH            50
#define UPS_INVALID_FILENAME        51
#define UPS_FILE_NOT_EXIST          52
#define UPS_CANT_FIND_SYSDIR        53

#define SERVICE_ACCESS_DENIED       0
/* INCLUDE FILES */

#include <windows.h>

/* DEFINATION FILES */
#define IDH_HELPFIRST        5000
#define LSFAIL               10
#define CPCAPTION            11
#define ERRMEM               12
#define _STOPPED             20
#define _START_PENDING       21
#define _STOP_PENDING        22
#define _RUNNING             23
#define _CONTINUE_PENDING    24
#define _PAUSE_PENDING       25
#define _PAUSED              26
#define _UNKNOWN             27
#define CHILDREN             48
#define INFO                 600
#define IDD_HELP             119
#define MAX_LDF_SEP          4
#define MENU_INDHELP         40

/* CONSTANT USED BY UPS.C */
#define KEYBZ 4096
#define SHORTBZ 16
#define MIDBZ 256
#define LONGBZ 1024
#define MASK 0x0E

typedef struct tagLDF {
  WORD Leadin;
  char LeadinSep[MAX_LDF_SEP];
  WORD Order[3];
  char Sep[2][MAX_LDF_SEP];
  } LDF;

typedef LDF NEAR *PLDF;

#define PATHMAX 158        /* path length max - used for Get...Directory() calls */
#define PORTLEN 128        /* COM port string lenght.
#define DESCMAX 129          /* max description in newexe header */
#define MODNAMEMAX 20       /* max module name in newexe header */

extern HANDLE hModule;
extern char szErrMem[133];
extern char szErrLS[133];
extern char szCtlPanel[30];
extern UINT     wHelpMessage;           // stuff for help

#ifndef NOARROWS
typedef struct
  {
    short lineup;             /* lineup/down, pageup/down are relative */
    short linedown;           /* changes.  top/bottom and the thumb    */
    short pageup;             /* elements are absolute locations, with */
    short pagedown;           /* top & bottom used as limits.          */
    short top;
    short bottom;
    short thumbpos;
    short thumbtrack;
    BYTE  flags;              /* flags set on return                   */
  } ARROWVSCROLL;
typedef ARROWVSCROLL NEAR     *NPARROWVSCROLL;
typedef ARROWVSCROLL FAR     *LPARROWVSCROLL;

#define UNKNOWNCOMMAND 1
#define OVERFLOW       2
#define UNDERFLOW      4

#endif

/* const used by ups.c */
#define MAXTRIES        3
#define SLEEP_TIME      2500L

typedef int (*PFNGETNAME)(LPSTR pszName, LPSTR pszInf);

/* FUNCTION PROTOTYPES */

/* ups.c */
void HourGlass(BOOL bOn);

/* memutil.c */
void   ErrLoadString (HWND hParent);
int    MyMessageBox (HWND hWnd, DWORD wText, DWORD wCaption, DWORD wType, ...);
LPVOID AllocMem   (DWORD cb);
BOOL   FreeMem    (LPVOID pMem, DWORD  cb);
LPVOID ReallocMem (LPVOID lpOldMem, DWORD cbOld, DWORD cbNew);
LPSTR  AllocStr   (LPSTR lpStr);
BOOL   FreeStr    (LPSTR lpStr);
BOOL   ReallocStr (LPSTR *plpStr, LPSTR lpStr);

/* arrow.c */
short ArrowVScrollProc (short wScroll, short nCurrent, LPARROWVSCROLL lpAVS);
BOOL  OddArrowWindow (HWND);
VOID  UnRegisterArrowClass (HANDLE hModule);

/* cpl.c */
extern void  CPHelp (HWND hwnd);
