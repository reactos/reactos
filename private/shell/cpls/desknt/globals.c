#include <windows.h>
#include <desk.h>

HINSTANCE hInstance;

TCHAR gszNoMem[CCH_MAX_STRING];
TCHAR gszDeskCaption[CCH_MAX_STRING];

TCHAR g_szNULL[] = TEXT("") ;
TCHAR g_szControlIni[] = TEXT("control.ini") ;
TCHAR g_szPatterns[] = TEXT("patterns") ;
TCHAR g_szNone[CCH_NONE];                      // this is the '(None)' string
TCHAR g_szClose[CCH_CLOSE];
TCHAR g_szBoot[] = TEXT("boot");
TCHAR g_szSystemIni[] = TEXT("system.ini");
TCHAR g_szWindows[] = TEXT("Windows");


HDC g_hdcMem;
HBITMAP g_hbmDefault;
