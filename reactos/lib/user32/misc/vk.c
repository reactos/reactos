#include <windows.h>

SHORT GetKeyState(    int  nVirtKey 	)
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