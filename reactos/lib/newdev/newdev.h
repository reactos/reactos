#include <windows.h>
#include <commctrl.h>
#include <regstr.h>
#include <setupapi.h>
#include <tchar.h>

#include <stdio.h>

#include "resource.h"

ULONG DbgPrint(PCH Format,...);

typedef struct _DEVINSTDATA
{
	HFONT hTitleFont;
	PBYTE buffer;
	DWORD requiredSize;
	DWORD regDataType;
	HWND hDialog;
	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA devInfoData;
	SP_DRVINFO_DATA drvInfoData;
} DEVINSTDATA, *PDEVINSTDATA;

#define WM_SEARCH_FINISHED (WM_USER + 10)
