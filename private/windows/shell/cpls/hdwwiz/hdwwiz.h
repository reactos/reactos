//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       hdwwiz.h
//
//--------------------------------------------------------------------------

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
#include <dlgs.h>     // common dlg IDs
#include <shellapi.h>
#include <shlobj.h>
#include <shlobjp.h> // SHELLSTATE
#include <shlwapi.h>
#include <cfgmgr32.h>
#include <setupapi.h>
#include <regstr.h>
#include <mountmgr.h>
#include <devguid.h>
#include "resource.h"

extern HMODULE hHdwWiz;
extern HMODULE hDevMgr;
extern HMODULE hHotPlug;
extern HMODULE hNewDev;

#define SIZECHARS(x) (sizeof((x))/sizeof(TCHAR))

//
// Search thread Functions
//
#define SEARCH_NULL     0
#define SEARCH_EXIT     1
#define SEARCH_DRIVERS  2
#define SEARCH_DELAY    3
#define SEARCH_DETECT   4
#define SEARCH_PNPENUM  5

#define WUM_INVOKETASK      (WM_USER+279)
#define WUM_DELAYTIMER      (WM_USER+280)
#define WUM_DOINSTALL       (WM_USER+281)
#define WUM_DETECT          (WM_USER+282)
#define WUM_PNPENUMERATE    (WM_USER+283)
#define WUM_RESOURCEPICKER  (WM_USER+284)


#define MAX_MESSAGE_STRING    512
#define MAX_MESSAGE_TITLE      50


typedef struct _ResourceBitmapPalette {
  HPALETTE hPalette;
  HBITMAP  hBitmap;
  BITMAP   Bitmap;
} RESOURCEBITMAP, *PRESOURCEBITMAP;

typedef struct _TaskThreadCreate {
   HWND    hwndParent;
   HANDLE  hEvent;
   HMODULE hModule;
   PVOID   pvTask;
   PTCHAR  MachineName;
   HMACHINE hMachine;
   DEVINST  DevInst;
   struct _TaskInitData *TaskInitData;
} TASKTHREADCREATE, *PTASKTHREADCREATE;

typedef struct _SearchThreadData {
   HWND    hDlg;
   HANDLE  hThread;
   HANDLE  RequestEvent;
   HANDLE  ReadyEvent;
   HANDLE  CancelEvent;
   ULONG   Param;
   UCHAR   Function;
   BOOLEAN CancelRequest;
   LPTSTR  Path;
} SEARCHTHREAD, *PSEARCHTHREAD;

typedef struct _ClassDeviceInfo {
   HDEVINFO Missing;
   HDEVINFO Detected;
} CLASSDEVINFO, *PCLASSDEVINFO;

typedef struct _DeviceDetectionData {
   HWND    hDlg;
   LPGUID  ClassGuid;
   UCHAR   ClassProgress;
   UCHAR   TotalProgress;
   BOOLEAN Reboot;
   BOOLEAN MissingOrNew;
   CLASSDEVINFO ClassDevInfo[1];
} DEVICEDETECTION, *PDEVICEDETECTION;

typedef struct _NewDeviceWizardExtension {
   HPROPSHEETPAGE hPropSheet;
   HPROPSHEETPAGE hPropSheetEnd;         // optional
   SP_NEWDEVICEWIZARD_DATA DeviceWizardData;
} WIZARDEXTENSION, *PWIZARDEXTENSION;

typedef BOOL
(*PFNINVOKETASK)(
   HWND hwndParent,
   PTASKTHREADCREATE TaskThreadCreate
   );

typedef struct _TaskInitData {
   int IdTask;
   int IdDesc;
   PTCHAR DllName;
   PCHAR EntryName;   // ansi
   PFNINVOKETASK InvokeTask;
} TASKINITDATA, *PTASKINITDATA;

typedef struct _HardwareWizard {
    HDEVINFO                hDeviceInfo;
    HDEVINFO                PNPEnumDeviceInfo;

    INT                     PrevPage;
    INT                     EnterFrom;
    INT                     EnterInto;

    int                     ClassGuidNum;
    int                     ClassGuidSize;
    LPGUID                  ClassGuidList;
    LPGUID                  ClassGuidSelected;
    GUID                    lvClassGuidSelected;
    GUID                    SavedClassGuid;



    HCURSOR                 IdcWait;
    HCURSOR                 IdcAppStarting;
    HCURSOR                 IdcArrow;
    HCURSOR                 CurrCursor;

    HWND                    hwndTaskList;
    PTASKINITDATA           TaskInitData;
    INT                     cxImageShadow;
    INT                     cyImageShadow;
    HDC                     hMemDc;

    HFONT                   hfontTextNormal;
    HFONT                   hfontTextHigh;
    HFONT                   hfontTextBold;
    HFONT                   hfontTextBigBold;

    INT                     cyText;
    INT                     cxSpot;
    INT                     cySpot;
    INT                     iSpotNormal;
    INT                     iSpotHigh;

    PSEARCHTHREAD           SearchThread;
    PDEVICEDETECTION        DeviceDetection;           // used by detect code
    SP_DEVINFO_DATA         DeviceInfoData;
    HANDLE                  hTaskThread;
    DWORD                   AnalyzeResult;
    HMACHINE                hMachine;
    HWND                    hwndProbList;
    DEVINST                 DevInst;
    DEVINST                 ProblemDevInst;
    SP_INSTALLWIZARD_DATA   InstallDynaWiz;
    HPROPSHEETPAGE          SelectDevicePage;
    SP_CLASSIMAGELIST_DATA  ClassImageList;

    BOOL                    DoTask;
    BOOL                    Registered;
    BOOL                    Installed;
    BOOL                    InstallPending;
    BOOL                    Cancelled;
    BOOL                    PnpDevice;
    BOOL                    FoundPnPDevices;
    BOOL                    ExitDetect;
    BOOL                    PromptForReboot;
    BOOL                    UninstallSucceeded;
    BOOL                    RunTroubleShooter;

    DWORD                   Reboot;
    DWORD                   LastError;

    WIZARDEXTENSION         WizExtPreSelect;
    WIZARDEXTENSION         WizExtSelect;
    WIZARDEXTENSION         WizExtUnplug;
    WIZARDEXTENSION         WizExtPreAnalyze;
    WIZARDEXTENSION         WizExtPostAnalyze;
    WIZARDEXTENSION         WizExtFinishInstall;

    TCHAR                   MachineName[MAX_COMPUTERNAME_LENGTH+1];
    TCHAR                   ClassName[MAX_CLASS_NAME_LEN];
    TCHAR                   ClassDescription[LINE_LEN];
    TCHAR                   DriverDescription[LINE_LEN];

} HARDWAREWIZ, *PHARDWAREWIZ;

#define NUMPROPPAGES 22

typedef struct _HardwareWizPropertySheet {
   PROPSHEETHEADER   PropSheetHeader;
   HPROPSHEETPAGE    PropSheetPages[NUMPROPPAGES];
} HDWPROPERTYSHEET, *PHDWPROPERTYSHEET;

#define TNULL ((TCHAR)0)

typedef BOOL
(CALLBACK* ADDDEVNODETOLIST_CALLBACK)(
    PHARDWAREWIZ HardwareWiz,
    PSP_DEVINFO_DATA DeviceInfoData
    );


LRESULT CALLBACK
HdwTasksDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   );

LRESULT CALLBACK
HdwIntroDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   );

LRESULT CALLBACK
HdwProbListDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   );

INT_PTR CALLBACK
HdwProbListFinishDlgProc(
   HWND   hDlg,
   UINT   wMsg,
   WPARAM wParam,
   LPARAM lParam
   );

LRESULT CALLBACK
HdwClassListDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   );

LRESULT CALLBACK
HdwDevicePropDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   );

INT_PTR CALLBACK
HdwPnpEnumDlgProc(
    HWND hDlg,
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    );

INT_PTR CALLBACK
HdwPnpFinishDlgProc(
    HWND hDlg,
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    );

INT_PTR CALLBACK
HdwAskDetectDlgProc(
    HWND hDlg,
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    );

INT_PTR CALLBACK
HdwDetectionDlgProc(
    HWND hDlg,
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    );

INT_PTR CALLBACK
HdwDetectInstallDlgProc(
    HWND hDlg,
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    );

INT_PTR CALLBACK
HdwDetectRebootDlgProc(
    HWND hDlg,
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    );

INT_PTR CALLBACK
HdwPickClassDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    );

INT_PTR CALLBACK
HdwSelectDeviceDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    );

INT_PTR CALLBACK
HdwAnalyzeDlgProc(
    HWND hDlg,
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    );

INT_PTR CALLBACK
HdwInstallDevDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR CALLBACK
HdwAddDeviceFinishDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR CALLBACK
HdwRemDeviceChoiceDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   );

INT_PTR CALLBACK
HdwUninstallListDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   );

INT_PTR CALLBACK
HdwRemDeviceUninstallConfirmDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   );

INT_PTR CALLBACK
HdwRemDeviceUninstallFinishDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   );

INT_PTR CALLBACK
InstallNewDeviceDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   );

INT_PTR CALLBACK
WizExtPreSelectDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    );

INT_PTR CALLBACK
WizExtSelectDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    );

INT_PTR CALLBACK
WizExtPreAnalyzeDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    );

INT_PTR CALLBACK
WizExtPreAnalyzeEndDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    );

INT_PTR CALLBACK
WizExtPostAnalyzeDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    );

INT_PTR CALLBACK
WizExtPostAnalyzeEndDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    );

INT_PTR CALLBACK
WizExtFinishInstallDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    );

INT_PTR CALLBACK
WizExtFinishInstallEndDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    );

INT_PTR CALLBACK
WizExtUnplugDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    );

INT_PTR CALLBACK
WizExtUnplugEndDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    );

PHDWPROPERTYSHEET
HdwWizard(
   HWND hwndParent,
   PHARDWAREWIZ HardwareWiz,
   int StartPageId
   );


//
// init.c
//

PTASKINITDATA
FindTaskInitData(
    INT idTask
    );


//
// miscutil.c
//

VOID
HdwWizPropagateMessage(
    HWND hWnd,
    UINT uMessage,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
LoadBitmapAndPalette(
    HINSTANCE hInstance,
    LPCTSTR pszResource,
    PRESOURCEBITMAP ResourceBitmap
    );

LONG
HdwBuildClassInfoList(
    PHARDWAREWIZ HardwareWiz,
    DWORD ClassListFlags,
    PTCHAR MachineName
    );

int
HdwMessageBox(
    HWND hWnd,
    LPTSTR szIdText,
    LPTSTR szIdCaption,
    UINT Type
    );

LONG
HdwUnhandledExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    );

BOOL
NoPrivilegeWarning(
   HWND hWnd,
   BOOL LaunchHotplug
   );

VOID
_OnSysColorChange(
    HWND hWnd,
    WPARAM wParam,
    LPARAM lParam
    );

void
LoadText(
    PTCHAR szText, 
    int SizeText, 
    int nStartString, 
    int nEndString
    );

void
InstallFailedWarning(
    HWND    hDlg,
    PHARDWAREWIZ HardwareWiz
    );

void
SetDlgText(
   HWND hDlg,
   int iControl,
   int nStartString,
   int nEndString
   );

void
SetDriverDescription(
    HWND hDlg,
    int iControl,
    PHARDWAREWIZ HardwareWiz
    );

HPROPSHEETPAGE
CreateWizExtPage(
   int PageResourceId,
   DLGPROC pfnDlgProc,
   PHARDWAREWIZ HardwareWiz
   );

BOOL
AddClassWizExtPages(
   HWND hwndParentDlg,
   PHARDWAREWIZ HardwareWiz,
   PSP_NEWDEVICEWIZARD_DATA DeviceWizardData,
   DI_FUNCTION InstallFunction
   );

void
RemoveClassWizExtPages(
   HWND hwndParentDlg,
   PSP_NEWDEVICEWIZARD_DATA DeviceWizardData
   );

BOOL
IsDeviceHidden(
    PHARDWAREWIZ HardwareWiz,
    PSP_DEVINFO_DATA DeviceInfoData
    );

BOOL
IsDeviceUninstallable(
    PHARDWAREWIZ HardwareWiz,
    PSP_DEVINFO_DATA DeviceInfoData
    );

BOOL
AnyHotPlugDevices(
    void
    );


//
// devcfg.c
//

typedef void
(*PFNDETECTPROBCALLBACK)(
   PHARDWAREWIZ HardwareWiz,
   DEVINST DevInst,
   ULONG Problem
   );

BOOL
BuildDeviceListView(
    PHARDWAREWIZ HardwareWiz,
    HWND hwndListView,
    BOOL ShowHiddenDevices,
    DEVINST SelectedDevInst,
    DWORD *DevicesDetected,
    ADDDEVNODETOLIST_CALLBACK AddDevNodeToListCallBack
    );

PTCHAR
BuildFriendlyName(
   DEVINST DevInst,
   HMACHINE hMachine
   );


extern TCHAR szUnknown[64];
extern USHORT LenUnknown;
extern TCHAR szUnknownDevice[64];
extern USHORT LenUnknownDevice;




//
// taskthd.c
//
LONG
AddNewInvokeTask(
   HWND hwndParent,
   PTASKTHREADCREATE TaskThreadCreate
   );

LONG
UninstallInvokeTask(
   HWND hwndParent,
   PTASKTHREADCREATE TaskThreadCreate
   );


LONG
DevProbInvokeTask(
   HWND hwndParent,
   PTASKTHREADCREATE TaskThreadCreate
   );

LONG
UpgradeInvokeTask(
   HWND hwndParent,
   PTASKTHREADCREATE TaskThreadCreate
   );

LONG
CreateTaskThread(
     HWND hDlg,
     PHARDWAREWIZ HardwareWiz
     );

//
// sthread.c
//
LONG
CreateSearchThread(
   PHARDWAREWIZ HardwareWiz
   );

void
DestroySearchThread(
   PSEARCHTHREAD SearchThread
   );

BOOL
SearchThreadRequest(
   PSEARCHTHREAD SearchThread,
   HWND    hDlg,
   UCHAR   Function,
   ULONG   Param
   );

void
CancelSearchRequest(
    PHARDWAREWIZ HardwareWiz
    );


//
// install.c
//
void
InstallSilentChildSiblings(
   HWND hwndParent,
   PHARDWAREWIZ HardwareWiz,
   DEVINST DeviceInstance,
   BOOL ReinstallAll
   );


//
// pnpenum.c
//
DWORD
PNPEnumerate(
    PHARDWAREWIZ HardwareWiz
    );


//
// detect.c
//
void
BuildDeviceDetection(
    HWND hwndParent,
    PHARDWAREWIZ HardwareWiz
    );


//
// finish.c
//
DWORD
HdwRemoveDevice(
   PHARDWAREWIZ HardwareWiz
   );

BOOL
DeviceHasResources(
   DEVINST DeviceInst
   );

void
DisplayResource(
    PHARDWAREWIZ HardwareWiz,
    HWND hWndParent,
    BOOL NeedsForcedConfig
    );


//
// getdev.c
//
PTCHAR
DeviceProblemText(
   HMACHINE hMachine,
   DEVNODE DevNode,
   ULONG ProblemNumber
   );


//
// config mgr privates
//
DWORD
CMP_WaitNoPendingInstallEvents(
    IN DWORD dwTimeout
    );



/*
 *  Cheat alert
 * setupapi private exports.
 *
 */

DWORD
pSetupGuidFromString(
  PWCHAR GuidString,
  LPGUID Guid
  );


DWORD
pSetupStringFromGuid(
    CONST GUID *Guid,
    PTSTR       GuidString,
    DWORD       GuidStringSize
    );

PCTSTR 
pSetupGetField(
    PINFCONTEXT Context, 
    DWORD FieldIndex
    );

//
// Exported from setupapi.dll
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

#ifdef DBG

void
Trace(
    LPCTSTR format,
    ...
    );
    
#define TRACE( args )          Trace args

#else

#define TRACE( args )

#endif // DBG


