#include <windows.h>

SHORT 
STDCALL
GetKeyState(    int  nVirtKey 	)
{
	return 0;
}

UINT
STDCALL
GetKBCodePage(
	      VOID)
{
	return 0;
}

SHORT
STDCALL
GetAsyncKeyState(
		 int vKey)
{
	return 0;
}

 
WINBOOL
STDCALL
GetKeyboardState(
		 PBYTE lpKeyState)
{
	return FALSE;
}

 
WINBOOL
STDCALL
SetKeyboardState(
		 LPBYTE lpKeyState)
{
	return FALSE;
}

 
int
STDCALL
GetKeyboardType(
		int nTypeFlag)
{
	return 0;
}


int
STDCALL
ToAscii(
	UINT uVirtKey,
	UINT uScanCode,
	PBYTE lpKeyState,
	LPWORD lpChar,
	UINT uFlags)
{
	return ToAsciiEx(uVirtKey,uScanCode, lpKeyState, lpChar, uFlags, NULL);
}

 
int
STDCALL
ToAsciiEx(
	  UINT uVirtKey,
	  UINT uScanCode,
	  PBYTE lpKeyState,
	  LPWORD lpChar,
	  UINT uFlags,
	  HKL dwhkl)
{
}