#ifndef __NEWDEV_PRIVATE_H
#define __NEWDEV_PRIVATE_H

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <winuser.h>
#include <windowsx.h>
#include <newdev.h>
#include <regstr.h>
#include <dll/newdevp.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(newdev);

#include "resource.h"

extern HINSTANCE hDllInstance;

typedef struct _DEVINSTDATA
{
	HFONT hTitleFont;
	BOOL bUpdate;
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

BOOL
CheckBestDriver(
    _In_ PDEVINSTDATA DevInstData,
    _In_ PCWSTR pszDir);

/* wizard.c */
BOOL
DisplayWizard(
	IN PDEVINSTDATA DevInstData,
	IN HWND hwndParent,
	IN UINT startPage);

#endif /* __NEWDEV_PRIVATE_H */
