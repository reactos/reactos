/** FILE: system.h ********* Module Header ********************************
 *
 *  Control Panel System applet common definitions, resource ids, typedefs,
 *  external declarations and library routine function prototypes.
 *
 * History:
 *  15:30 on Thur  25 Apr 1991  -by-  Steve Cathcart   [stevecat]
 *        Took base code from Win 3.1 source
 *  10:30 on Tues  04 Feb 1992  -by-  Steve Cathcart   [stevecat]
 *        Updated code to latest Win 3.1 sources
 *  22:00 on Wed   17 Nov 1993  -by-  Steve Cathcart   [stevecat]
 *        Changes for product update
 *  17:00 on Mon   18 Sep 1995  -by-  Steve Cathcart   [stevecat]
 *        Changes for product update - SUR release NT v4.0
 *
 *
 *  Copyright (C) 1990-1995 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                            Include Files
//==========================================================================
#include <windows.h>
#include <tchar.h>

//==========================================================================
//                        Definitions
//==========================================================================


//
//  String Resource IDs
//


#define INITS                0
#define SYSTEM              10

#define HWP                 50

#define IDS_SYSTEM          INITS+3
#define IDS_SYSTEMINFO      INITS+4

#define IDS_SYSSETCHANGE        100
#define IDS_RESTART             101
#define IDS_VIRTUALMEMCHANGE    102
#define IDS_RECOVERDLGCHANGE    103
#define IDS_VIRTANDRECCHANGE    104

#define HWP_DEF_FRIENDLYNAME        HWP+0
#define HWP_CURRENT_TAG             HWP+1
#define HWP_UNAVAILABLE             HWP+2
#define HWP_ERROR_CAPTION           HWP+3
#define HWP_ERROR_PROFILE_IN_USE    HWP+4
#define HWP_ERROR_INVALID_CHAR      HWP+5
#define HWP_ERROR_IN_USE            HWP+6


#define SYSTEM_ICON          1
#define DOCK_ICON            2
#define UP_ICON              3
#define DOWN_ICON            4


#define DLG_SYSTEM          39
#define DLG_RESTART         37
#define DLG_VIRTUALMEM      41
#define DLG_TASKING         42
#define DLG_COREDUMP        45
#define DLG_ADDOS           40

#define DLG_HWPROFILES      60
#define DLG_HWP_RENAME      61
#define DLG_HWP_COPY        62
#define DLG_HWP_GENERAL     63


//
//  These constants serve a dual purpose:  They are both the menu ID
//  as well as the value to be passed to WinHelp.  If these values are
//  changed, change the code so it passes the appropriate ContextID
//  when calling WinHelp.     15 Sept 1989  Clark R. Cyr
//

#define MENU_SCHHELP     33

#define MENU_INDHELP     40
#define MENU_USEHELP     41

#define FOO -1                  /* for useless control ids */

#define RESTART_TEXT      100

//
//  NT System Applet
//

#define IDD_SYS_TASKING          96
#define IDD_SYS_COMPUTERNAME    100
#define IDD_SYS_OS              101
#define IDD_SYS_SHOWLIST        102
#define IDD_SL_TXT1             103
#define IDD_SYS_SECONDS         104
#define IDD_SYS_SECSCROLL       105
#define IDD_SL_TXT2             106
#define IDD_SYS_LB_SYSVARS      107
#define IDD_SYS_UVLABEL         108
#define IDD_SYS_LB_USERVARS     109
#define IDD_SYS_VAR             110
#define IDD_SYS_VALUE           111
#define IDD_SYS_DELUV           112
#define IDD_SYS_SETUV           113
#define IDD_SYS_VMEM            114
#define IDD_SYS_ENABLECOUNTDOWN 115
#define IDD_SYS_COREDUMP        116

#define IDD_SYS_HWPROFILES      117

#define IDD_SYS_ANS_NAME        150
#define IDD_SYS_ANS_LOCATION    151

#define IDD_VM_VOLUMES          160
#define IDD_VM_SF_DRIVE         161
#define IDD_VM_SF_SPACE         162
#define IDD_VM_SF_SIZE          163
#define IDD_VM_SF_SIZEMAX       164
#define IDD_VM_SF_SET           165
#define IDD_VM_MIN              166
#define IDD_VM_RECOMMEND        167
#define IDD_VM_ALLOCD           168
#define IDD_VM_ST_INITSIZE      169
#define IDD_VM_ST_MAXSIZE       170
#define IDD_VMEM_ICON           171
#define IDD_VMEM_MESSAGE        172
#define IDD_VM_REG_SIZE_LIM     173
#define IDD_VM_REG_SIZE_TXT     174
#define IDD_VM_RSL_ALLOCD       175


#define IDD_CDMP_LOG            200
#define IDD_CDMP_SEND           201
#define IDD_CDMP_WRITE          202
#define IDD_CDMP_OVERWRITE      203
#define IDD_CDMP_FILENAME       204
#define IDD_CDMP_AUTOREBOOT     205
#define IDD_CDMP_BROWSE         206
#define IDD_CDMP_MESSAGE        207
#define IDD_CDMP_ICON           208


//
// IF IDS ARE ADDED OR REMOVED, THEN ADD/REMOVE THE CORRESPONDING
// HELP IDS IN HWPROF.C ALSO!!
//
#define IDD_HWP_PROFILES        300
#define IDD_HWP_PROPERTIES      301
#define IDD_HWP_COPY            302
#define IDD_HWP_RENAME          303
#define IDD_HWP_DELETE          304
#define IDD_HWP_ST_MULTIPLE     305
#define IDD_HWP_ORDERPREF       306
#define IDD_HWP_WAITFOREVER     307
#define IDD_HWP_WAITUSER        308
#define IDD_HWP_SECONDS         309
#define IDD_HWP_SECSCROLL       310
#define IDD_HWP_COPYTO          311
#define IDD_HWP_COPYFROM        312
#define IDD_HWP_ST_DOCKID       313
#define IDD_HWP_ST_SERIALNUM    314
#define IDD_HWP_DOCKID          315
#define IDD_HWP_SERIALNUM       316
#define IDD_HWP_PORTABLE        317
#define IDD_HWP_ST_DOCKSTATE    318
#define IDD_HWP_UNKNOWN         319
#define IDD_HWP_DOCKED          320
#define IDD_HWP_UNDOCKED        321
#define IDD_HWP_ST_PROFILE      322
#define IDD_HWP_ORDERUP         323
#define IDD_HWP_ORDERDOWN       324
#define IDD_HWP_RENAMEFROM      325
#define IDD_HWP_RENAMETO        326



#define PATHMAX             MAX_PATH

//
//  Reboot switch for system dlg
//

#define RET_ERROR               (-1)
#define RET_NO_CHANGE           0x0
#define RET_VIRTUAL_CHANGE      0x1
#define RET_RECOVER_CHANGE      0x2
#define RET_CHANGE_NO_REBOOT    0x4

#define RET_VIRT_AND_RECOVER (RET_VIRTUAL_CHANGE | RET_RECOVER_CHANGE)

#define IDSYSI_EXCLAMATION      (32515)

//
//  Tasking dialog stuff
//

#define ID_DLGBOX           1
#define IDS_NAME            1

#define ID_GMTLISTBOX       8

#define ID_AUTO             10
#define ID_STANDARDONLY     11
#define ID_DSTONLY          12

#define IDD_HELP            119

#define IDM_PRICTL          900

#define IDB_DEFAULT         102
#define IDB_SMALLER         103
#define IDB_NONE            104

#define IDB_OK               1
#define IDB_CANCEL           2
#define IDB_HELP            43


//
//  Help IDs
//

//
//  syshelp.h  -  help ids for System applet
//
//


#define IDH_HELPFIRST        5000
#define IDH_SYSMENU     (IDH_HELPFIRST + 2000)
#define IDH_MBFIRST     (IDH_HELPFIRST + 2001)
#define IDH_DLG_FONT2   (IDH_HELPFIRST + 2002)
#define IDH_MBLAST      (IDH_HELPFIRST + 2099)
#define IDH_DLGFIRST    (IDH_HELPFIRST + 3000)
#define IDH_HWPROFILE   (IDH_HELPFIRST + 4000)

#define IDH_MENU_SCHHELP    (IDH_HELPFIRST + MENU_SCHHELP)
#define IDH_MENU_INDHELP    (IDH_HELPFIRST + MENU_INDHELP)
#define IDH_MENU_USEHELP    (IDH_HELPFIRST + MENU_USEHELP)
#define IDH_MENU_ABOUT      (IDH_HELPFIRST + MENU_ABOUT )
#define IDH_MENU_EXIT       (IDH_HELPFIRST + MENU_EXIT)

#define IDH_CHILD_SYSTEM    (IDH_HELPFIRST + 12 /* CHILD_SYSTEM */)
#define IDH_DLG_SYSTEM      (IDH_DLGFIRST + DLG_SYSTEM)
#define IDH_DLG_ADDOS       (IDH_DLGFIRST + DLG_ADDOS)
#define IDH_DLG_VIRTUALMEM  (IDH_DLGFIRST + DLG_VIRTUALMEM)
#define IDH_DLG_TASKING     (IDH_DLGFIRST + DLG_TASKING)
#define IDH_DLG_COREDUMP    (IDH_DLGFIRST + DLG_COREDUMP)
#define IDH_DLG_HWPROFILES  (IDH_DLGFIRST + DLG_HWPROFILES)



//==========================================================================
//                           Typedefs
//==========================================================================

typedef struct
{
    short lineup;             //  lineup/down, pageup/down are relative
    short linedown;           //  changes.  top/bottom and the thumb
    short pageup;             //  elements are absolute locations, with
    short pagedown;           //  top & bottom used as limits.
    short top;
    short bottom;
    short thumbpos;
    short thumbtrack;
    BYTE  flags;              //  flags set on return
} ARROWVSCROLL;

typedef ARROWVSCROLL NEAR     *NPARROWVSCROLL;
typedef ARROWVSCROLL FAR      *LPARROWVSCROLL;

#define UNKNOWNCOMMAND 1
#define OVERFLOW       2
#define UNDERFLOW      4

//==========================================================================
//                              Macros
//==========================================================================

#define CharSizeOf(x)   (sizeof(x) / sizeof(*x))
#define ByteCountOf(x)  ((x) * sizeof(TCHAR))


//==========================================================================
//                         External Declarations
//==========================================================================
//
//  DATA

//
//  exported from cpl.c
//

extern HANDLE g_hInst;
extern UINT   g_wHelpMessage;       //  stuff for help
extern DWORD  g_dwContext;          //  help context
extern BOOL   g_bSetup;             //  TRUE if running under Setup

extern TCHAR  g_szSysDir[ ];        //  GetSystemDirectory
extern TCHAR  g_szWinDir[ ];        //  GetWindowsDirectory
extern TCHAR  g_szClose[ ];         //  "Close" string
extern TCHAR  g_szSharedDir[ ];     //  Shared dir found by Version apis
extern TCHAR  g_szErrMem[ ];        //  Low memory message
extern TCHAR  g_szSystemApplet[ ];  //  "System Control Panel Applet" title
extern TCHAR  g_szNull[];           //  Null string


//==========================================================================
//                            Function Prototypes
//==========================================================================
//
//  arrow.c
//


extern BOOL RegisterArrowClass( HANDLE );
extern VOID UnRegisterArrowClass( HANDLE );
extern BOOL RegisterProgressClass( HANDLE );
extern VOID UnRegisterProgressClass( HANDLE );

extern short ArrowVScrollProc( short wScroll, short nCurrent, LPARROWVSCROLL lpAVS );
extern BOOL  OddArrowWindow( HWND );

//
//  cpl.c
//
extern void SysHelp( HWND hwnd );


//
//  prictl.c
//

extern BOOL APIENTRY TaskingDlg( HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam );


//
//  system.c
//

extern BOOL APIENTRY SystemDlg( HWND, UINT, WPARAM, LPARAM );

//
//  util.c
//

extern LPTSTR BackslashTerm( LPTSTR pszPath );
extern BOOL   CheckVal( HWND hDlg, WORD wID, WORD wMin, WORD wMax, WORD wMsgID );
extern int    DoDialogBoxParam( int nDlg, HWND hParent, DLGPROC lpProc,
                                        DWORD dwHelpContext, DWORD dwParam );
extern void   ErrMemDlg( HWND hParent );
extern void   HourGlass( BOOL bOn);
extern int    MyAtoi( LPTSTR  string );
extern int    MyMessageBox( HWND hWnd, DWORD wText, DWORD wCaption, DWORD wType, ... );
extern BOOL   RestartDlg( HWND hDlg, UINT message, DWORD wParam, LONG lParam );
extern void   SendWinIniChange( LPTSTR szSection );

extern LPVOID AllocMem  ( DWORD cb );
extern BOOL   FreeMem   ( LPVOID pMem, DWORD  cb );
extern LPVOID ReallocMem( LPVOID lpOldMem, DWORD cbOld, DWORD cbNew );
extern LPTSTR AllocStr  ( LPTSTR lpStr );
extern BOOL   FreeStr   ( LPTSTR lpStr );
extern BOOL   ReallocStr( LPTSTR *plpStr, LPTSTR lpStr );

//
//  virtual.c
//

BOOL APIENTRY VirtualMemDlg( HWND hDlg, UINT message, DWORD wParam, LONG lParam );
BOOL APIENTRY CoreDumpDlg( HWND hDlg, UINT message, DWORD wParam, LONG lParam );


//
// hwprof.c
//
BOOL APIENTRY HardwareProfilesDlg (HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
BOOL APIENTRY CopyProfileDlg (HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
BOOL APIENTRY RenameProfileDlg (HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
BOOL APIENTRY GeneralProfileDlg (HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);


#if DBG
//#ifndef DbgPrint
//void  DbgPrint( char *, ... );
//#endif
#ifndef DbgBreakPoint
void  DbgBreakPoint( void );
#endif
#endif
