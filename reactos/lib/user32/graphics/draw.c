#include <windows.h>

WINBOOL STDCALL DrawEdge( HDC hdc, LPRECT rc, UINT edge, UINT flags )
{

}
WINBOOL DrawIcon(HDC  hDC,	int  xLeft,	int  yTop,	HICON  hIcon 	
   )
{
	return FALSE;
}

WINBOOL
STDCALL
DrawIconEx(HDC hdc, int xLeft, int yTop,
	   HICON hIcon, int cxWidth, int cyWidth,
	   UINT istepIfAniCur, HBRUSH hbrFlickerFreeDraw, UINT diFlags)
{
}

WINBOOL
STDCALL
DrawFocusRect(
	      HDC hDC,
	      CONST RECT * lprc)
{
}