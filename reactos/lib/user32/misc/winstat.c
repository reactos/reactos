#include <windows.h>


HWINSTA WinStation = NULL;
HDESK Desktop = NULL;

HWINSTA 
STDCALL
CreateWindowStationA(
  LPSTR lpwinsta,       
  DWORD dwReserved,       
  DWORD dwDesiredAccess,  
  LPSECURITY_ATTRIBUTES lpsa  
                             
)
{		
	return CreateFileA("\\\\.\\WinStat",
			       dwDesiredAccess,
			       0,
			       NULL,
			       OPEN_EXISTING,
			       0,
			       NULL);  
}

HWINSTA 
STDCALL
CreateWindowStationW(
  LPWSTR lpwinsta,       
  DWORD dwReserved,       
  DWORD dwDesiredAccess,  
  LPSECURITY_ATTRIBUTES lpsa  
                             
)
{		
	return CreateFileW(L"\\\\.\\WinStat",
			       dwDesiredAccess,
			       0,
			       NULL,
			       OPEN_EXISTING,
			       0,
			       NULL);  
}


#undef CreateDesktop
HDESK
STDCALL
CreateDesktopA(
    LPSTR lpszDesktop,
    LPSTR lpszDevice,
    LPDEVMODE pDevmode,
    DWORD dwFlags,
    DWORD dwDesiredAccess,
    LPSECURITY_ATTRIBUTES lpsa)

{

}

HDESK
STDCALL
CreateDesktopW(
    LPWSTR lpszDesktop,
    LPWSTR lpszDevice,
    LPDEVMODE pDevmode,
    DWORD dwFlags,
    DWORD dwDesiredAccess,
    LPSECURITY_ATTRIBUTES lpsa)

{

}
 
 
HDESK
STDCALL
OpenInputDesktop(
		 DWORD dwFlags,
		 WINBOOL fInherit,
		 DWORD dwDesiredAccess)
{
	return 0;
}

WINBOOL
STDCALL
EnumDesktopWindows(
		   HDESK hDesktop,
		   ENUMWINDOWSPROC lpfn,
		   LPARAM lParam)
{
	return FALSE;
}

 
WINBOOL
STDCALL
SwitchDesktop(
	      HDESK hDesktop)
{
	return FALSE;
}

 
WINBOOL
STDCALL
SetThreadDesktop(
		 HDESK hDesktop)
{
	return FALSE;
}



HDESK
STDCALL
OpenDesktopA(
    LPSTR lpszDesktop,
    DWORD dwFlags,
    WINBOOL fInherit,
    DWORD dwDesiredAccess)
{
	return Desktop;
} 

HDESK
STDCALL
OpenDesktopW(
    LPWSTR lpszDesktop,
    DWORD dwFlags,
    WINBOOL fInherit,
    DWORD dwDesiredAccess)
{
	return Desktop;
}

WINBOOL
STDCALL
CloseDesktop(
	     HDESK hDesktop)
{
	return 0;
}

 
HDESK
STDCALL
GetThreadDesktop(
		 DWORD dwThreadId)
{
	return 0;
}

 
WINBOOL
STDCALL
CloseWindowStation(
		   HWINSTA hWinSta)
{
	return 0;
}

 
WINBOOL
STDCALL
SetProcessWindowStation(
			HWINSTA hWinSta)
{
	return FALSE;
}

 
HWINSTA
STDCALL
GetProcessWindowStation(
			VOID)
{
	return WinStation;
}



WINBOOL 
STDCALL
EnumDesktopsA( 
    HWINSTA hwinsta, 
    DESKTOPENUMPROC lpEnumFunc, 
    LPARAM lParam) { return FALSE; }

WINBOOL 
STDCALL
EnumDesktopsW( 
    HWINSTA hwinsta, 
    DESKTOPENUMPROC lpEnumFunc, 
    LPARAM lParam) { return FALSE; }



WINBOOL 
WINAPI 
EnumWindowStationsA( 
    ENUMWINDOWSTATIONPROC lpEnumFunc, 
    LPARAM lParam)
{
	return FALSE;
}

BOOL 
WINAPI 
EnumWindowStationsW( 
    ENUMWINDOWSTATIONPROC lpEnumFunc, 
    LPARAM lParam)
{
	return FALSE;
}

HWINSTA
STDCALL
OpenWindowStationA(
    LPSTR lpszWinSta,
    WINBOOL fInherit,
    DWORD dwDesiredAccess)
{
	return WinStation;
}


HWINSTA
STDCALL
OpenWindowStationW(
    LPWSTR lpszWinSta,
    WINBOOL fInherit,
    DWORD dwDesiredAccess)
{
	return WinStation;
}


HDESK STDCALL GetInputDesktop(VOID) { return Desktop; }