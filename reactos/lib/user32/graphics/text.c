#include <windows.h>
#include <user32/text.h>


int
STDCALL
DrawTextA(
    HDC hDC,LPCSTR lpString,
    int nCount, LPRECT lpRect, UINT uFormat)
{
	DRAWTEXTPARAMS dtp;
	dtp.cbSize = sizeof(DRAWTEXTPARAMS);
	dtp.iTabLength = 0;
	return TEXT_DrawTextEx(hDC,(void *)lpString, nCount,lpRect,uFormat, &dtp, FALSE);
}


int
STDCALL
DrawTextW(
    HDC hDC, LPCWSTR lpString,
    int nCount, LPRECT lpRect, UINT uFormat)
{
	DRAWTEXTPARAMS dtp;
	dtp.cbSize = sizeof(DRAWTEXTPARAMS);
	dtp.iTabLength = 0;
	return TEXT_DrawTextEx(hDC,(void *)lpString,nCount,lpRect,uFormat, &dtp,TRUE);

}

/***********************************************************************
 *           GetTabbedTextExtentA    (USER32.293)
 */
DWORD
STDCALL
GetTabbedTextExtentA(HDC hDC, LPCSTR lpString, int nCount, int nTabPositions,
    LPINT lpnTabStopPositions)
{

 return TEXT_TabbedTextOutA( hDC, 0, 0, lpString, nCount, nTabPositions,
                                 lpnTabStopPositions,0, FALSE );
}


DWORD
STDCALL
GetTabbedTextExtentW(HDC hDC, LPCWSTR lpString, int nCount, int nTabPositions,
    LPINT lpnTabStopPositions)
{

    return TEXT_TabbedTextOutW( hDC, 0, 0, lpString, nCount, nTabPositions,
                               lpnTabStopPositions, 0,  FALSE );
}


LONG
STDCALL
TabbedTextOutA(    HDC hDC,    int X,    int Y,    LPCSTR lpString,
    int nCount,    int nTabPositions,  LPINT lpnTabStopPositions, int nTabOrigin)
{
	return TEXT_TabbedTextOutA( hDC, X, Y, lpString, nCount, nTabPositions,
                               lpnTabStopPositions, nTabOrigin,  TRUE );
}

LONG
STDCALL
TabbedTextOutW(    HDC hDC,    int X,    int Y,    LPCWSTR lpString,
    int nCount,    int nTabPositions,  LPINT lpnTabStopPositions, int nTabOrigin)
{
	return TEXT_TabbedTextOutW( hDC, 0, 0, lpString, nCount, nTabPositions,
                               lpnTabStopPositions, nTabOrigin,  TRUE );
}

