//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       hotplug.h
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
#include <commctrl.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <prsht.h>
#include <dlgs.h>     // common dlg IDs
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shlobjp.h>
#include <devguid.h>
#include <dbt.h>
#include "resource.h"


#define STWM_NOTIFYHOTPLUG  STWM_NOTIFYPCMCIA
#define STSERVICE_HOTPLUG   STSERVICE_PCMCIA
#define HOTPLUG_REGFLAG_NOWARN PCMCIA_REGFLAG_NOWARN
#define HOTPLUG_REGFLAG_VIEWALL (PCMCIA_REGFLAG_NOWARN << 1)

#define TIMERID_DEVICECHANGE 4321

#define ARRAYLEN(array)     (sizeof(array) / sizeof(array[0]))


LRESULT CALLBACK
DevTreeDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   );

LRESULT CALLBACK
RemoveConfirmDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   );

LRESULT CALLBACK
RemoveFinishDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   );

LRESULT CALLBACK
SurpriseWarnDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   );

LRESULT CALLBACK
SafeRemovalDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   );

LRESULT CALLBACK
DeviceRemovalVetoedDlgProc(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
    );

void
OnSystrayOptionClicked(
    HWND hDlg
    );


extern HMODULE hHotPlug;


typedef struct _DeviceTreeNode {
  LIST_ENTRY SiblingEntry;
  LIST_ENTRY ChildSiblingList;
  PTCHAR     InstanceId;
  PTCHAR     FriendlyName;
  PTCHAR     DeviceDesc;
  PTCHAR     ClassName;
  PTCHAR     DriveList;
  PTCHAR     Location;
  struct _DeviceTreeNode *ParentNode;
  struct _DeviceTreeNode *NextChildRemoval;
  HTREEITEM  hTreeItem;
  DEVINST    DevInst;
  GUID       ClassGuid;
  DWORD      Capabilities;
  int        TreeDepth;
  PDEVINST   EjectRelations;
  USHORT     NumEjectRelations;
  PDEVINST   RemovalRelations;
  USHORT     NumRemovalRelations;
  ULONG      Problem;
  ULONG      DevNodeStatus;
  BOOL       bCopy;
} DEVTREENODE, *PDEVTREENODE;


typedef struct _DeviceTreeData {
   DWORD Size;
   HWND hwndTree;
   HWND hDlg;
   HWND hwndRemove;
   HMACHINE hMachine;
   int  TreeDepth;
   PDEVTREENODE SelectedTreeNode;
   PDEVTREENODE ChildRemovalList;
   LIST_ENTRY ChildSiblingList;
   DEVINST    DevInst;
   PTCHAR     EjectDeviceInstanceId;
   SP_CLASSIMAGELIST_DATA ClassImageList;
   BOOLEAN    ComplexView;
   BOOLEAN    HotPlugTree;
   BOOLEAN    AllowRefresh;
   BOOLEAN    RedrawWait;
   BOOLEAN    RefreshEvent;
   BOOLEAN    IsDialog;
   BOOLEAN    HideUI;
   TCHAR      MachineName[MAX_COMPUTERNAME_LENGTH+1];
} DEVICETREE, *PDEVICETREE;

typedef enum {

    VETOED_EJECT = 1,
    VETOED_REMOVAL,
    VETOED_UNDOCK,
    VETOED_STANDBY,
    VETOED_HIBERNATE,
    VETOED_WARM_EJECT,
    VETOED_WARM_UNDOCK

} VETOED_OPERATION;

typedef struct _WIZPAGE_OBJECT {

    DWORD RefCount;

    PDEVICETREE DeviceTree;
} WIZPAGE_OBJECT, *PWIZPAGE_OBJECT;

typedef struct _GenericDeviceCollection {
    struct _GenericDeviceCollection *Next;
    TCHAR   DeviceInstanceId[MAX_DEVICE_ID_LEN];
} DEVICE_COLLECTION, *PDEVICE_COLLECTION;

typedef struct _GenericDeviceCollectionData {
    HMACHINE hMachine;
    PDEVICE_COLLECTION DeviceList;
    INT      NumDevices;
    SP_CLASSIMAGELIST_DATA ClassImageList;
} DEVICE_COLLECTION_DATA, *PDEVICE_COLLECTION_DATA;

typedef struct _VetoDeviceCollectionData {
    struct _GenericDeviceCollectionData; // Unnamed structure.
    PNP_VETO_TYPE VetoType;
    VETOED_OPERATION VetoedOperation;
} VETO_DEVICE_COLLECTION_DATA, *PVETO_DEVICE_COLLECTION_DATA;

typedef struct _SafeRemovalDeviceCollectionData {
    struct _GenericDeviceCollectionData; // Unnamed structure.
    BOOL DockInList;
} SAFE_REMOVAL_COLLECTION_DATA, *PSAFE_REMOVAL_COLLECTION_DATA;

//
// shared data structure with systray
//

typedef struct _SurpriseWarnDeviceList {
    struct _SurpriseWarnDeviceList *Next;
    TCHAR   DeviceInstanceId[MAX_DEVICE_ID_LEN];
} SURPRISEWARNDEVICES, *PSURPRISEWARNDEVICES;

typedef struct _SurpriseWarnData {
    HMACHINE hMachine;
    PSURPRISEWARNDEVICES DeviceList;
    INT      NumDevices;
    SP_CLASSIMAGELIST_DATA  ClassImageList;
    BOOL    DockInList;
} SURPRISEWARNDATA, *PSURPRISEWARNDATA;


#define SIZECHARS(x) (sizeof((x))/sizeof(TCHAR))


void
OnContextHelp(
  LPHELPINFO HelpInfo,
  PDWORD ContextHelpIDs
  );

//
// from init.c
//
DWORD
WINAPI
HandleVetoedOperation(
    LPWSTR              szCmd,
    VETOED_OPERATION    RemovalVetoType
    );

//
// from hotplug.c
//
PDEVICETREE
GetDeviceTreeFromPsPage(
    LPPROPSHEETPAGE ppsp
    );


//
// from devtree.c
//

LONG
AddChildSiblings(
    PDEVICETREE  DeviceTree,
    PDEVTREENODE ParentNode,
    DEVINST      DeviceInstance,
    int          TreeDepth,
    BOOL         Recurse
    );

void
RemoveChildSiblings(
    PDEVICETREE  DeviceTree,
    PLIST_ENTRY  ChildSiblingList
    );

PTCHAR
FetchDeviceName(
     PDEVTREENODE DeviceTreeNode
     );

BOOL
DisplayChildSiblings(
    PDEVICETREE  DeviceTree,
    PLIST_ENTRY  ChildSiblingList,
    HTREEITEM    hParentTreeItem,
    BOOL         RemovableParent
    );


void
AddChildRemoval(
    PDEVICETREE DeviceTree,
    PLIST_ENTRY ChildSiblingList
    );


void
ClearRemovalList(
    PDEVICETREE  DeviceTree
    );


PDEVTREENODE
DevTreeNodeByInstanceId(
    PTCHAR InstanceId,
    PLIST_ENTRY ChildSiblingList
    );

PDEVTREENODE
DevTreeNodeByDevInst(
    DEVINST DevInst,
    PLIST_ENTRY ChildSiblingList
    );


PDEVTREENODE
TopLevelRemovalNode(
    PDEVICETREE DeviceTree,
    PDEVTREENODE DeviceTreeNode
    );

void
AddEjectToRemoval(
    PDEVICETREE DeviceTree
    );

extern TCHAR szUnknown[64];
extern TCHAR szHotPlugFlags[];





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
    GUID *Guid,
    PTSTR GuidString,
    DWORD GuidStringSize
    );


//
// notify.c
//
void
OnTimerDeviceChange(
   PDEVICETREE DeviceTree
   );


BOOL
RefreshTree(
   PDEVICETREE DeviceTree
   );



//
// miscutil.c
//

void
SetDlgText(
    HWND hDlg,
    int iControl,
    int nStartString,
    int nEndString
    );


VOID
HotPlugPropagateMessage(
    HWND hWnd,
    UINT uMessage,
    WPARAM wParam,
    LPARAM lParam
    );


BOOL
RemovalPermission(
   void
   );

int
HPMessageBox(
   HWND hWnd,
   int  IdText,
   int  IdCaption,
   UINT Type
   );


void
InvalidateTreeItemRect(
   HWND hwndTree,
   HTREEITEM  hTreeItem
   );

DWORD
GetHotPlugFlags(
   PHKEY phKey
   );



PTCHAR
BuildFriendlyName(
   DEVINST DevInst,
   HMACHINE hMachine
   );


PTCHAR
BuildLocationInformation(
   DEVINST DevInst,
   HMACHINE hMachine
   );

LPTSTR
DevNodeToDriveLetter(
    DEVINST DevInst
    );

BOOL
IsHotPlugDevice(
    DEVINST DevInst,
    HMACHINE hMachine
    );


#define WUM_EJECTDEVINST  (WM_USER+279)
