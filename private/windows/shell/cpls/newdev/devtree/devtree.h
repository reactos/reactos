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
#include <commctrl.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <prsht.h>
#include <dlgs.h>     // common dlg IDs
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shsemip.h>
#include <devguid.h>
#include "resource.h"


LRESULT CALLBACK
DevTreeDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   );


extern HMODULE hDevTree;

typedef struct _DeviceTreeNode {
  struct _DeviceTreeNode *Child;
  struct _DeviceTreeNode *Sibling;
  HTREEITEM  hTreeItem;
  DEVINST    DevInst;
  GUID       ClassGuid;
  TCHAR      Driver[MAX_PATH];
} DEVTREENODE, *PDEVTREENODE;


typedef struct _DeviceTreeData {
   SP_CLASSIMAGELIST_DATA ClassImageList;
   HDEVINFO hDeviceInfo;
   HWND hwndTree;
   HWND hwndEdit;
   HMACHINE hMachine;
   DEVTREENODE RootNode;
   TCHAR MachineName[MAX_COMPUTERNAME_LENGTH+1];
} DEVICETREE, *PDEVICETREE;

#define SIZECHARS(x) (sizeof((x))/sizeof(TCHAR))



DWORD
pSetupGuidFromString(
  PWCHAR GuidString,
  LPGUID Guid
  );
