#include <windows.h>
#include <setupapi.h>

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

BOOL SearchDriver ( PDEVINSTDATA DevInstData, const TCHAR* Path );
BOOL InstallDriver ( PDEVINSTDATA DevInstData );
DWORD WINAPI FindDriverProc( LPVOID lpParam );
BOOL FindDriver ( PDEVINSTDATA DevInstData );

#define WM_SEARCH_FINISHED (WM_USER + 10)
