/*  DRIVERS.H
**
**  Copyright (C) Microsoft, 1990, All Rights Reserved.
**
**
**  Multimedia Control Panel Drivers Applet.
**
**  Display a list of all installed drivers, allow user to configure
**  existing or install new ones.
**
*/
#include "dlgs.h"
#include "commdlg.h"
#include <multimed.h>
#include <commctrl.h>
#include "mmcpl.h"
#include <setupapi.h>

#define SECTION         512                   // Maximum size of section
#define MAXSTR          256
#define DLG_BROWSE      38
#define UNLIST_LINE     1
#define NO_UNLIST_LINE  0
#define WEC_RESTART     0x42
#define DESC_SYS        3
#define DESC_INF        2
#define DESC_EXE        1
#define DESC_NOFILE     0

#define dwStatusHASSERVICE   0x00000001  // bit set if driver has service
#define dwStatusSvcENABLED   0x00000002  // bit set if has -enabled- service
#define dwStatusSvcSTARTED   0x00000004  // bit set if has now-running service
#define dwStatusDRIVEROK     0x00000008  // bit set if !svc and can open driver
#define dwStatusMAPPABLE     0x00000010  // bit set if doesn't have Mappable=0

#ifndef cchRESOURCE
#define cchRESOURCE     256
#endif

typedef struct _IDRIVER
{
    WCHAR   wszSection[MAXSTR];
    WCHAR   wszAlias[MAXSTR];
    WCHAR   wszFile[MAX_PATH];
    TCHAR    szSection[MAXSTR];
    TCHAR    szAlias[MAXSTR];
    TCHAR    szFile[MAX_PATH];
    TCHAR    szDesc[MAXSTR];
    struct  _IDRIVER *related;
    BOOL    bRelated;
    TCHAR    szRelated[MAXSTR];
    TCHAR    szRemove[MAXSTR];
    int     fQueryable;     // 0 -> can't, 1 -> can, -1 -> need to check
    BOOL    bInstall;       // 0 -> no,    1 -> yes
    BOOL    KernelDriver;   // If TRUE this is a kernel driver, not an
                            // 'installable' driver so it can't be opened,
                            // process messages etc.
#ifdef INFFILE
    BOOL    infFileProcessing;
#endif // INFFILE

    LPARAM  lp;
} IDRIVER, *PIDRIVER;

typedef enum    // DriverClass (in no particular order)
{
    dcINVALID = -1,
    dcAUX = 0,   // Aux devices
    dcMIDI,  // MIDi devices, MIDI mapper
    dcMIXER, // Mixer devices
    dcWAVE,  // Wave audio devices, Wave mapper
    dcACODEC,    // Audio codecs
    dcVCODEC,    // Video codecs
    dcMCI,   // MCI devices
    dcVIDCAP,    // Video capture devices
    dcJOY,   // Joystick devices
    dcLEGACY,    // Legacy Devices for wave, midi and mixer
    dcOTHER  // Any unknown device
} DriverClass;

typedef struct _DevTreeNode // legacy from win95 devpropsheet code
{
    HWND hwndTree;
    LPARAM lParam;
} DEVTREENODE, * PDEVTREENODE;

extern HANDLE         myInstance;
extern TCHAR           szNULL[];
extern TCHAR           szDrivers[];
extern TCHAR           szBoot[];
extern TCHAR           szDriversHlp[];
extern TCHAR           szAppName[];
extern TCHAR           szUnlisted[];
extern TCHAR           szFullPath[];
extern TCHAR           szOemInf[];
extern TCHAR           szDirOfSrc[];
extern TCHAR           szUserDrivers[];
extern TCHAR           szControlIni[];
extern TCHAR           szDriversDesc[];
extern TCHAR           szSetupInf[];
extern TCHAR           szSysIni[];
extern TCHAR           szMCI[];
extern TCHAR           szRestartDrv[];
extern TCHAR           szRelated[];
extern TCHAR           szNULL[];
extern TCHAR           szBackslash[];
extern TCHAR           szOutOfRemoveSpace[];
extern TCHAR           szKnown[];
extern TCHAR           szRelatedDesc[];
extern TCHAR           szDrv[];
extern TCHAR           szRemove[];
extern TCHAR           szSystem[];
extern TCHAR           szSystemDrivers[];
extern int            iRestartMessage;
extern UINT           wHelpMessage;
extern DWORD          dwContext;
extern BOOL           bCopyVxD;
extern BOOL           bVxd;
extern BOOL           bInstallBootLine;
extern BOOL           bFindOEM;
extern BOOL           bRestart;
extern BOOL           bCopyingRelated;
extern BOOL           bRelated;
extern HWND           hAdvDlgTree;
extern BOOL           IniFileReadAllowed;
extern BOOL           IniFileWriteAllowed;

//------------------------------------------------------------------------
//
// Public routines
//
//------------------------------------------------------------------------
extern void RemoveDriverEntry   (LPTSTR, LPTSTR, LPTSTR, BOOL);
extern int  FileNameCmp         (TCHAR *, TCHAR *);
extern void OpenDriverError     (HWND, LPTSTR, LPTSTR);
extern void RemoveSpaces        (LPTSTR, LPTSTR);
extern INT_PTR RestartDlg          (HWND, unsigned, WPARAM, LPARAM);
extern INT_PTR AddDriversDlg       (HWND, unsigned, WPARAM, LPARAM);
extern INT_PTR AddUnlistedDlg      (HWND, unsigned, WPARAM, LPARAM);
extern BOOL mmAddNewDriver      (LPTSTR, LPTSTR, PIDRIVER);
extern LONG_PTR PostRemove          (PIDRIVER, BOOL);
extern BOOL RemoveService       (LPTSTR);
extern BOOL CopyToSysDir        (void);
extern BOOL InstallDrivers      (HWND, HWND, LPTSTR);
extern void InitDrvConfigInfo   (LPDRVCONFIGINFO, PIDRIVER);
extern BOOL AddIDriverToArray   (PIDRIVER);
extern BOOL FillTreeInAdvDlg    (HWND, PIDRIVER);
extern PIDRIVER FindIDriverByName (LPTSTR);
extern void RemoveIDriver       (HWND, PIDRIVER, BOOL);
extern BOOL IsConfigurable      (PIDRIVER, HWND);
extern void BrowseDlg           (HWND, int);
extern int  LoadDescFromFile    (PIDRIVER, LPTSTR, LPTSTR);
extern BOOL wsInfParseInit      (void);
extern void wsStartWait         (void);
extern void wsEndWait           (void);
extern UINT wsCopySingleStatus  (int, DWORD_PTR, LPTSTR);
extern BOOL QueryRemoveDrivers  (HWND, LPTSTR, LPTSTR);
#ifdef FIX_BUG_15451
extern void ConfigureDriver     (HWND, LPTSTR);
extern BOOL fDeviceHasMixers    (LPTSTR);
extern BOOL WaitForNewCPLWindow (HWND);
extern void GetTreeItemNodeDesc (LPTSTR, PIRESOURCE);
extern void GetTreeItemNodeID   (LPTSTR, PIRESOURCE);
#endif // FIX_BUG_15451
extern void RefreshAdvDlgTree   (void);

extern DWORD InstallDriversForPnPDevice (HWND, HDEVINFO, PSP_DEVINFO_DATA);

// From drivers.c
extern void        lsplitpath     (LPTSTR, LPTSTR, LPTSTR, LPTSTR, LPTSTR);

// From remove.c
LPTSTR mystrtok(LPTSTR SrcString, LPCTSTR Seps, LPTSTR FAR *State);
extern DWORD Media_RemoveDevice(IN HDEVINFO         DeviceInfoSet,
                                IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
                               );

// From install.c
extern BOOL FilterOutNonNTInfs(IN HDEVINFO         DeviceInfoSet,
                               IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
                               IN DWORD DriverType
                              );
extern DWORD Media_SelectBestCompatDrv(IN HDEVINFO         DeviceInfoSet,
                                       IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
                                      );
extern DWORD Media_AllowInstall(IN HDEVINFO         DeviceInfoSet,
                                IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
                               );
extern DWORD Media_InstallDevice(IN HDEVINFO         DeviceInfoSet,
                                 IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
                                );

// From legcyinf.c
extern DWORD Media_SelectDevice(IN HDEVINFO         DeviceInfoSet,
                                IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
                               );


extern DWORD Media_MigrateLegacy(IN HDEVINFO         DeviceInfoSet,
                                 IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
                                );

#define IS_MS_MMMCI 1
#define IS_MS_MMVID 2
#define IS_MS_MMACM 3
#define IS_MS_MMVCD 4
#define IS_MS_MMDRV 5
extern int IsSpecialDriver(IN HDEVINFO         DeviceInfoSet,
                           IN PSP_DEVINFO_DATA DeviceInfoData
                          );

extern BOOL IsPnPDriver(IN PTSTR szName);

void acmDeleteCodec (WORD, WORD);       // (from MSACMCPL.C)

#ifdef FIX_BUG_15451
extern TCHAR    szDriverWhichNeedsSettings[MAX_PATH]; // See MMCPL.C
#endif // FIX_BUG_15451

void                FreeIResource             (PIRESOURCE);
INT_PTR CALLBACK    DevPropDlg                (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL                DriverClassToClassNode    (PCLASSNODE, DriverClass);
int                 DriverClassToOldClassID   (DriverClass);
DWORD               GetDriverStatus           (PIDRIVER);
BOOL                InitInstalled             (HWND, LPTSTR);

/* Resource IDs */

#define IDOK                1
#define IDCANCEL            2
#define ID_IGNORE           3
#define ID_CURRENT          4
#define ID_RETRY            5
#define ID_NEW              6

#define ID_DISK             101
#define ID_ADV_ADD          102
#define ID_EDIT             105
#define ID_TEXT             106

#define ID_DIR              202
#define ID_FILE_LIST        203
#define ID_DIR_LIST         204
#define ID_TYPE             205

#define LB_AVAILABLE        301
#define ID_DRV              302
#define ID_LIST             303
#define ID_DRVSTRING        304
#define LB_UNLISTED         306
#define ID_ADV_PROP         307
#define ID_ADV_TSHOOT       308

#define ADVDLG              1001
#define DLG_UPDATE          1002
#define DLG_KNOWN           1003
#define DLG_RESTART         1004
#define DLG_EXISTS          1005
#define DLG_INSERTDISK      1006

#define DLG_COPYERR         1007
#define ID_STATUS2          1008
#define DLG_DM_ADVDLG       1009
#define DLG_DM_LEGACY_RESOURCES 1010

#define IDS_NOINF                       2003
#define IDS_DEFDRIVE                    2004
#define IDS_OUTOFDISK                   2005
#define IDS_DISKS                       2006
#define IDS_INSTALLDRIVERS              2007
#define IDS_INSTALLDRIVERS32            2067
#define IDS_DRIVERDESC                  2008
#define IDS_OUT_OF_REMOVE_SPACE         2009
#define IDS_AVAILABLE_DRIVERS_DEFAULT   2010
#define IDS_ERROR                       2011
#define IDS_INSTALLING_DRIVERS          2012
#define IDS_NO_DESCRIPTION              2013
#define IDS_ERRORBOX                    2014
#define IDS_RESTARTTEXT                 2015
#define IDS_CONFIGURE_DRIVER            2016
#define IDS_TOO_MANY_DRIVERS            2017
#define IDS_CANNOT_FIND                 2018
#define IDS_APPNAME                     2019
#define IDS_DRIVERS                     2020
#define IDS_SETUPINF                    2021
#define IDS_CONTROLINI                  2022
#define IDS_SYSINI                      2023
#define IDS_MCI                         2024
#define IDS_CONTROL_INI                 2025
#define IDS_WIN                         2027
#define IDS_DOS                         2028
#define IDS_BROWSE                      2029
#define IDS_UPDATED                     2031
#define IDS_CLOSE                       2032
#define IDS_REMOVEORNOT                 2033
#define IDS_UNLISTED                    2034
#define IDS_KNOWN                       2035
#define IDS_REMOVE                      2036
#define IDS_REMOVEORNOTSTRICT           2037
#define IDS_USERINSTALLDRIVERS          2038
#define IDS_OEMSETUP                    2044
#define IDS_SYSTEM                      2045
#define IDS_FILE_ERROR                  2046
#define IDS_UNABLE_TOINSTALL            2049
#define IDS_BOOT                        2050
#define IDS_RESTART_ADD                 2051
#define IDS_RESTART_REM                 2052
#define IDS_FILEINUSEADD                2053
#define IDS_OEMDISKS                    2054
#define IDS_FILEINUSEREM                2055
#define IDS_LASTQUERY                   2056
#define IDS_RELATEDDESC                 2057
#define IDS_DRIVER_EXISTS               2058
#define IDS_SYSTEM_DRIVERS              2060
#define IDS_INSUFFICIENT_PRIVILEGE      2061
#define IDS_CANNOT_RESTART_PRIVILEGE    2062
#define IDS_CANNOT_RESTART_UNKNOWN      2063
#define IDS_DRIVER_CONFIG_ERROR         2064
#define IDS_CANTADD                     2065
#define IDS_CONTROL_HLP_FILE            2066
#define IDS_TSHOOT                      2068
