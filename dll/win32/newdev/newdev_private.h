#ifndef __NEWDEV_PRIVATE_H
#define __NEWDEV_PRIVATE_H

#define COBJMACROS
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <newdev.h>
#include <regstr.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <shlobj.h>
#include <wine/debug.h>

#include <stdio.h>

#include "resource.h"

extern HINSTANCE hDllInstance;

typedef struct _DEVINSTDATA
{
	HFONT hTitleFont;
	PBYTE buffer;
	DWORD requiredSize;
	DWORD regDataType;
	HWND hDialog;
	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA devInfoData;
	SP_DRVINFO_DATA_W drvInfoData;

	LPWSTR CustomSearchPath; /* MULTI_SZ string */
} DEVINSTDATA, *PDEVINSTDATA;

#define WM_SEARCH_FINISHED  (WM_USER + 10)
#define WM_INSTALL_FINISHED (WM_USER + 11)

/* newdev.c */

BOOL
ScanFoldersForDriver(
	IN PDEVINSTDATA DevInstData);

BOOL
PrepareFoldersToScan(
	IN PDEVINSTDATA DevInstData,
	IN BOOL IncludeRemovableDevices,
	IN BOOL IncludeCustomPath,
	IN HWND hwndCombo OPTIONAL);

BOOL
InstallCurrentDriver(
	IN PDEVINSTDATA DevInstData);

/* wizard.c */
BOOL
DisplayWizard(
	IN PDEVINSTDATA DevInstData,
	IN HWND hwndParent,
	IN UINT startPage);

#endif /* __NEWDEV_PRIVATE_H */
