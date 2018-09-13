/*******************************************************************************
*
*  (C) Copyright MICROSOFT Corp., 1993
*
*  TITLE:       NEWDEV.H
*
*  VERSION:     1.0
*
*  DATE:        2/23/94
*
*  AUTHOR:      Donald McNamara
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV  DESCRIPTION
*  ----------- ---  -------------------------------------------------------------
*  2/23/94     DJM  Created this file to hold Add Device Dlgs
*  6/23/94     DJM  Renamed to newdev.h
*********************************************************************************/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cpl.h>
#include <cplp.h>
#include <prsht.h>
#include <commctrl.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <dlgs.h>     // common dlg IDs
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shlobjp.h>
#include <devguid.h>
#include <pnpmgr.h>
#include "resource.h"


//
// Wizard Flags
//


// The Wizard type, these are mutually exclusive.
#define NDWTYPE_FOUNDNEW 1
#define NDWTYPE_UPDATE   2


typedef struct _SearchThreadData {
   HWND    hDlg;
   HANDLE  hThread;
   HANDLE  RequestEvent;
   HANDLE  ReadyEvent;
   HANDLE  CancelEvent;
   ULONG   Options;
   ULONG   Param;
   UCHAR   Function;
   BOOL    CancelRequest;
   LPTSTR  Path;
} SEARCHTHREAD, *PSEARCHTHREAD;

typedef struct _NewDeviceWizardExtension {
   HPROPSHEETPAGE hPropSheet;
   HPROPSHEETPAGE hPropSheetEnd;         // optional
   SP_NEWDEVICEWIZARD_DATA DeviceWizardData;
} WIZARDEXTENSION, *PWIZARDEXTENSION;

typedef struct _CodeDownloadManagerInfo {
   LPCWSTR InfPathName;
   LPCWSTR DisplayName;
   BOOL    Force;
   BOOL    Backup;
   BOOL    DriverWasUpgraded;
} CDMINFO, *PCDMINFO;


typedef struct _NewDeviceWizard {
    HDEVINFO                hDeviceInfo;
    int                     EnterInto;
    int                     EnterFrom;
    int                     PrevPage;

    int                     ClassGuidNum;
    int                     ClassGuidSize;
    LPGUID                  ClassGuidList;
    LPGUID                  ClassGuidSelected;
    GUID                    lvClassGuidSelected;
    GUID                    SavedClassGuid;

    HCURSOR  CurrCursor;
    HCURSOR  IdcWait;
    HCURSOR  IdcAppStarting;
    HCURSOR  IdcArrow;
    HFONT    hfontTextBigBold;

    PSEARCHTHREAD SearchThread;
    PSP_DRVINFO_DETAIL_DATA  pDriverDetailData; // used only by analyze page.
    DWORD                   AnalyzeResult;
    SP_DEVINFO_DATA         DeviceInfoData;
    SP_INSTALLWIZARD_DATA   InstallDynaWiz;
    HPROPSHEETPAGE          SelectDevicePage;
    SP_CLASSIMAGELIST_DATA  ClassImageList;

    BOOL     Installed;
    BOOL     Cancelled;
    BOOL     ExitDetect;
    BOOL     InstallPending;
    BOOL     SilentMode;
    BOOL     InstallChilds;
    BOOL     PromptForReboot;
    BOOL     ManualInstall;
    BOOL     DontReinstallCurrentDriver;
    DWORD    WizardType;
    DWORD    SearchOptions;
    DWORD    LastError;
    DWORD    Reboot;
    DWORD    Capabilities;
    PCDMINFO CdmInfo;

    WIZARDEXTENSION WizExtPreSelect;
    WIZARDEXTENSION WizExtSelect;
    WIZARDEXTENSION WizExtPreAnalyze;
    WIZARDEXTENSION WizExtPostAnalyze;
    WIZARDEXTENSION WizExtFinishInstall;

    TCHAR    ClassName[MAX_CLASS_NAME_LEN];
    TCHAR    ClassDescription[LINE_LEN];
    TCHAR    DriverDescription[LINE_LEN];
    TCHAR    BrowsePath[MAX_PATH];
    TCHAR    InstallDeviceInstanceId[MAX_DEVICE_ID_LEN];
} NEWDEVWIZ, *PNEWDEVWIZ;



void
AddNewDeviceWizard(
    HWND            hWnd
    );

BOOL
InstallDeviceWizard(
    HWND hWnd,
    PNEWDEVWIZ NewDevWiz
    );

BOOL
InstallSelectedDevice(
   HWND hwndParent,
   HDEVINFO hDeviceInfo,
   PDWORD pReboot
   );

BOOL
UpdateDeviceWizard(
    HWND hWnd,
    PNEWDEVWIZ NewDevWiz
    );

BOOL
IntializeDeviceMapInfo(
    void
    );

UINT
GetNextDriveByType(
    UINT DriveType,
    UINT DriveNumber
    );

void
UpdateFileInfo(
    HWND        hDlg,
    LPWSTR      FileName,
    int         IconType,
    int         PathType
    );

#define SIZECHARS(x) (sizeof((x))/sizeof(TCHAR))

//
// IconType and PathType used by UpdateFileInfo
//

#define DRVUPD_INTERNETICON 1
#define DRVUPD_SHELLICON    2
#define DRVUPD_FOLDERICON   3
#define DRVUPD_PATHPART     2
#define DRVUPD_FILEPART     3

#define CAN_ALLOCATE_CONFIG         0x0000
#define ERR_CONFIGMG_ERROR          0x0001
#define ERR_CANNOT_ALLOCATE_CONFIG  0x0002
#define ERR_NO_LOGCONFIG            0x0003

#define ID_DEFAULT      0
#define ID_FILESONLY    1
#define ID_NONULLDRIVER 2

#define MAX_RESTYPE_STRING     25
#define MAX_RESSETTING_STRING  30
#define MAX_MESSAGE_STRING    512
#define MAX_MESSAGE_TITLE      50
#define SDT_MAX_TEXT         1024        // Max SetDlgText


//
//
// Cheat!, setupapi private exports.
// deal with this later.
//
// Exported from setupapi.dll , can we include setupapip.h ?
//
BOOL
DoesUserHavePrivilege(
    PCTSTR PrivilegeName
    );

BOOL 
EnablePrivilege(
    PCSTR PrivlegeName, 
    BOOL Enable
    );



DWORD
pSetupStringFromGuid(
    CONST GUID *Guid,
    PTSTR       GuidString,
    DWORD       GuidStringSize
    );

DWORD
pSetupGuidFromString(
    PWCHAR GuidString,
    LPGUID Guid
    );


BOOL
InfIsFromOemLocation(
    PTSTR InfFileName
    );


// config mgr privates
DWORD
CMP_WaitNoPendingInstallEvents(
    IN DWORD dwTimeout
    );



//
// from search.c
//
void
SearchDriveForDrivers(
    PNEWDEVWIZ NewDevWiz,
    UINT DriveNumber
    );

void
SetDriverPath(
   PNEWDEVWIZ NewDevWiz,
   TCHAR      *DriverPath,
   BOOL       InetSearch
   );

BOOL
IsInstalledDriver(
   PNEWDEVWIZ NewDevWiz,
   PSP_DRVINFO_DATA DriverInfoData
   );


//
// from miscutl.c
//

BOOL
SetClassGuid(
    HDEVINFO hDeviceInfo,
    PSP_DEVINFO_DATA DeviceInfoData,
    LPGUID ClassGuid
    );


void
SetDlgText(
   HWND hDlg,
   int iControl,
   int nStartString,
   int nEndString
   );

void
LoadText(
   PTCHAR szText,
   int SizeText,
   int nStartString,
   int nEndString
   );


VOID
_OnSysColorChange(
    HWND hWnd,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
NoPrivilegeWarning(
   HWND hWnd
   );

int
NdwMessageBox(
   HWND hWnd,
   int  IdText,
   int  IdCaption,
   UINT Type
   );


LONG
NdwBuildClassInfoList(
   PNEWDEVWIZ NewDevWiz,
   DWORD ClassListFlags
   );

void
HideWindowByMove(
   HWND hDlg
   );


LONG
NdwUnhandledExceptionFilter(
   struct _EXCEPTION_POINTERS *ExceptionPointers
   );

HPROPSHEETPAGE
CreateWizExtPage(
   int PageResourceId,
   DLGPROC pfnDlgProc,
   PNEWDEVWIZ NewDevWiz
   );

BOOL
AddClassWizExtPages(
   HWND hwndParentDlg,
   PNEWDEVWIZ NewDevWiz,
   PSP_NEWDEVICEWIZARD_DATA DeviceWizardData,
   DI_FUNCTION InstallFunction
   );

void
RemoveClassWizExtPages(
   HWND hwndParentDlg,
   PSP_NEWDEVICEWIZARD_DATA DeviceWizardData
   );




extern TCHAR szUnknownDevice[64];
extern USHORT LenUnknownDevice;
extern TCHAR szUnknown[64];
extern USHORT LenUnknown;

PTCHAR
BuildLocationInformation(
    DEVINST DevInst,
    HMACHINE hMachine
    );

PTCHAR
BuildFriendlyName(
    DEVINST DevInst,
    BOOL UseNewDeviceDesc,
    HMACHINE hMachine
    );

void
RebootComputerNow(
   void
   );




//
// newdev.c, init.c
//

extern HMODULE hNewDev;
extern HWND    g_hWndNewDevice;
extern ULONG   g_hTaskNewDevice;
extern TCHAR *DevicePath; // default windows inf path
extern TCHAR DefaultInf[]; // "Default Windows Inf Path"


// TBD:
// These need to go into a central header file
// Its an exported interface for public use.
//
BOOL
InstallDevInst(
   HWND hwndParent,
   LPCWSTR DeviceInstanceId,
   BOOL UpdateDriver,
   PDWORD pReboot
   );

BOOL
InstallNewDevice(
   HWND hwndParent,
   LPGUID ClassGuid,
   PDWORD pReboot
   );



//
// finish.c
//

DWORD
InstallNullDriver(
   HWND hDlg,
   PNEWDEVWIZ NewDevWiz,
   BOOL FailedInstall
   );

//
// Intro.c
//
DWORD
PNPEnumerate(
    PNEWDEVWIZ NewDevWiz,
    PSEARCHTHREAD SearchThread
    );



//
// update.c
//

void
SetDriverDescription(
    HWND hDlg,
    int iControl,
    PNEWDEVWIZ NewDevWiz
    );

void
InstallSilentChildSiblings(
   HWND hwndParent,
   PNEWDEVWIZ NewDevWiz,
   DEVINST DeviceInstance,
   BOOL ReinstallAll
   );



//
// Search thread options
//
#define SEARCH_DEFAULT_EXCLUDE_OLD_INET 0x00000001
#define SEARCH_DEFAULT                  0x00000002
#define SEARCH_FLOPPY                   0x00000004
#define SEARCH_CDROM                    0x00000008
#define SEARCH_PATH                     0x00000010
#define SEARCH_INET                     0x00000020
#define SEARCH_WINDOWSUPDATE            0x00000040

//
// Search thread Functions
//
#define SEARCH_NULL     0
#define SEARCH_EXIT     1
#define SEARCH_DRIVERS  2
#define SEARCH_DELAY    3


#define WUM_SEARCHDRIVERS (WM_USER+279)
#define WUM_DELAYTIMER    (WM_USER+280)
#define WUM_DOINSTALL     (WM_USER+281)
#define WUM_UPDATEUI      (WM_USER+282)
#define WUM_EXIT          (WM_USER+283)


LONG
CreateSearchThread(
   PNEWDEVWIZ NewDevWiz
   );

void
DestroySearchThread(
   PSEARCHTHREAD SearchThread
   );

VOID
CancelSearchRequest(
   PNEWDEVWIZ NewDevWiz
   );

BOOL
SearchThreadRequest(
   PSEARCHTHREAD SearchThread,
   HWND    hDlg,
   UCHAR   Function,
   ULONG   Options,
   ULONG   Param
   );



//
// The wizard dialog procs
//
INT_PTR CALLBACK IntroDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NDW_PickClassDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NDW_AnalyzeDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NDW_InstallDevDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NDW_FinishDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NDW_SelectDeviceDlgProc(HWND hDlg,UINT wMsg,WPARAM wParam,LPARAM lParam);

LRESULT CALLBACK DevInstallUiOnlyDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK DevInstallDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK DriverUpdateDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DriverSearchDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DriverSearchingDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DriverBrowseDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK InstallNewDeviceDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ListDriversDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK WizExtPreSelectDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK WizExtSelectDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK WizExtPreAnalyzeDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK WizExtPreAnalyzeEndDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK WizExtPostAnalyzeDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK WizExtPostAnalyzeEndDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK WizExtFinishInstallDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK WizExtFinishInstallEndDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,LPARAM lParam);
