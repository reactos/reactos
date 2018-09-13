/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    appmgr.h

Abstract:

    This is the include file for the Application Manger applet.

Author:

    Dave Hastings (daveh) creation-date 29-Apr-1997


Revision History:

--*/

#define WM_UPDATELISTVIEW WM_USER

#define NUM_PAGES 24

//
// Feature Mask defines
//
#define FEATURE_ADD             0x00000001
#define FEATURE_ADD_CORPNET     0x00000002
#define FEATURE_ADD_MEDIA       0x00000004
#define FEATURE_ADD_INTERNET    0x00000008
#define FEATURE_REPAIR          0x00000010
#define FEATURE_MODIFYBRANCH    0x00000020
#define FEATURE_MODIFY          0x00000040
#define FEATURE_REMOVE          0x00000080
#define FEATURE_PROPERTIES      0x00000100


typedef BOOL (*DLGINITPROC)(
    HWND,
	UINT,
    WPARAM,
    LPARAM
    );

typedef INT (*COMMANDPROC)(
    HWND,
    WPARAM,
    LPARAM,
    UINT
    );

typedef INT (*NOTIFYPROC)(
    HWND,
    WPARAM,
    LPNMHDR,
    UINT
    );

typedef INT (*UPDATELISTVIEWPROC)(
    HWND,
    WPARAM,
    LPARAM,
    UINT
    );

typedef struct _PageInfo {
    DWORD Flags;                            // Flags for this page (PSH_HIDEHEADER etc.)
    LPTSTR Template;                        // Template for this page.  Should be a 
                                            // resouce ID
    LPTSTR Title;                           // Title string for the header on interior 
                                            // pages.
                                            // This can (should) be a resource ID
    LPTSTR Subtitle;                        // Subtitle string for the header on interior 
                                            // pages. This can (should) be a resource ID
    DLGPROC DialogProc;                     // Dialog procedure for this page
    DLGINITPROC InitProc;                   // Called on WM_INITDIALOG to do page specific 
                                            // initialization
    COMMANDPROC CommandProc;                // Called on WM_COMMAND
    NOTIFYPROC NotifyProc;                  // Called on WM_NOTIFY
    UPDATELISTVIEWPROC UpdateListViewProc;
    UINT CancelPageID;                      // resource ID of cancel page
    HWND Dialog;                            // handle of the dialog window
} PAGEINFO, *PPAGEINFO;

typedef struct _ApplicationDescriptor {
    PIPACTIONS Actions;
    HANDLE Identifier;
    DWORD Capabilities;
} APPLICATIONDESCRIPTOR, *PAPPLICATIONDESCRIPTOR;

typedef enum _FeatureState {
    Available,
    NotAvailablePolicy,
    NotAvailableResource,
    NotAvailableError
} FEATURESTATE;

typedef struct _AppMgrConfig {
    FEATURESTATE Floppy;
    FEATURESTATE CdRom;
    FEATURESTATE Administered;
    FEATURESTATE Internet;
} APPMGRCONFIG, PAPPMGRCONFIG;

typedef struct _DarwinInstallInfo {
	PWCHAR ProductName;
	PWCHAR PackagePath;
	PWCHAR ProductCode;
} DARWININSTALLINFO, *PDARWININSTALLINFO;

INT
WelcomeNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    );

INT
WelcomeCommandProc(
    HWND Dialog,
    WPARAM wParam,
    LPARAM lParam,
    UINT Index
    );

BOOL
ModifyProgramInitProc(
    HWND DialogWindow,
	UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    );

INT
ModifyProgramNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    );

INT
ModifyProgramUpdateListViewProc(
    HWND Dialog,
    WPARAM wParam,
    LPARAM lParam,
    UINT PageIndex
    );

INT
ModifyFinishNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    );

INT
AddSourceNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    );

INT
AddSourceCommandProc(
    HWND Dialog,
    WPARAM wParam,
    LPARAM lParam,
    UINT Index
    );

INT
AddMediaSelectNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    );

INT
AddMediaSelectCommandProc(
    HWND Dialog,
    WPARAM wParam,
    LPARAM lParam,
    UINT Index
    );

INT
FinishMediaNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    );

BOOL
FindInstallProgram(
    DWORD DesiredDriveType,
    LPWSTR SetupPath,
    DWORD SetupPathSize
    );

BOOL
ExecSetup(
	PWSTR SetupName,
	HWND DialogWindow,
	HINSTANCE Instance
	);

VOID
GetMachineConfig(
    VOID
    );

BOOL
BrowseLocalMedia(
    DWORD MediaType,
    PWCHAR SetupPath,
    HWND Parent
    );

INT
AddNoMediaCommandProc(
    HWND Dialog,
    WPARAM wParam,
    LPARAM lParam,
    UINT Index
    );

INT
AddNoMediaNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    );

INT
ModifyProgramCommandProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPARAM lParam,
    UINT PageIndex
    );

INT
CancelNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    );

BOOL
AddSourceInitProc(
    HWND DialogWindow,
	UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
ScreenRectToClientRect(
    PRECT Rect,
    HWND Window
    );

VOID
UnimplementedFeature(
    HWND Dialog,
    HWND ToolTip,
    UINT ControlId
    );

BOOL
WelcomeInitProc(
    HWND DialogWindow,
	UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    );

INT AddBrowseNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    );

INT
AddBrowseCommandProc(
    HWND Dialog,
    WPARAM wParam,
    LPARAM lParam,
    UINT PageIndex
    );

BOOL
VerifySetupName(
    PWCHAR FileName
    );

INT AddFinishBrowseNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    );

HRESULT
GetPublishedApps(
    PACKAGEDISPINFO *Packages,
    PULONG NumberOfPackages
	);

BOOL
AddProgramInitProc(
    HWND DialogWindow,
	UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    );

INT
AddProgramCommandProc(
    HWND Dialog,
    WPARAM wParam,
    LPARAM lParam,
    UINT PageIndex
    );

INT AddProgramNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    );

VOID
PopulateApplicationListView(
    HWND ListViewWindow,
    DWORD DesiredCapabilities
    );

BOOL
RepairSelectInitProc(
    HWND DialogWindow,
	UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    );

INT
RepairSelectNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    );

INT
RepairSelectCommandProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPARAM lParam,
    UINT PageIndex
    );

BOOL
HandleQueryCancel(
    HWND Dialog,              
    UINT NextPageID
    );

VOID
DisplayError(
    DWORD ErrorCode
    );

LONG
CheckSetWindowLong(
    HWND hWnd,
    INT nIndex,
    LONG dwNewLong
    );

INT
InternetSiteCommandProc(
    HWND Dialog,
    WPARAM wParam,
    LPARAM lParam,
    UINT Index
    );

INT
InternetSiteNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    );

BOOL
InternetSiteInitProc(
    HWND DialogWindow,
	UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    );

INT
AddFinishInternetNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    );

INT
UpgradeProgramNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    );

BOOL
ItemSelected(
    HWND DialogWindow,
    PULONG Index
    );

BOOL
UpgradeProgramInitProc(
    HWND DialogWindow,
	UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    );

INT
UpgradeProgramCommandProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPARAM lParam,
    UINT PageIndex
    );

BOOL
SetUpFonts(
    HINSTANCE Instance,
    HWND Window,
    HFONT *LargeFont,
    HFONT *SmallFont
    );

VOID
SetExteriorTitleFont(
    HWND Dialog
    );

BOOL
CancelInitProc(
    HWND Dialog,
	UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    );


VOID
PostUpdateMessage(
    VOID
    );

VOID
UpdateApplicationLists(
    HWND ListView,
    DWORD DesiredCapabilities
    );

BOOL
InitializePolicy(
    VOID
    );

VOID
FindApplication(
    HWND Parent
    );

VOID
ShowProperties(
    PAPPLICATIONDESCRIPTOR AppDescriptor,
    HWND Parent
    );

LONG
ReadAndExpandRegString(
    HKEY RegKey,
    HANDLE Heap,
    PWCHAR ValueName,
    PWCHAR *Value
    );

VOID
CreateApplicationListViewColumns(
	HWND ListViewWindow
	);

VOID
DisableCancelButton(
    HWND Dialog
    );

extern HINSTANCE Instance;
extern HANDLE ConfigSyncHandle;
extern APPMGRCONFIG AppMgrConfig;
extern HFONT LargeTitleFont, SmallTitleFont;
extern PAGEINFO PageInfo[NUM_PAGES];
extern ULONG FeatureMask;

// BUGBUG
#define REPAIR 1
#define REINSTALL 2
