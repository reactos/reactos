#include <windows.h>


int
STDCALL
DrawTextA(
    HDC hDC,LPCSTR lpString,
    int nCount, LPRECT lpRect, UINT uFormat)
{
	DRAWTEXTPARAMS dtp;
	dtp.cbSize = sizeof(DRAWTEXTPARAMS);
	dtp.iTabLength = 0;
	//return TEXT_DrawTextEx(hDC,(void *)lpString, nCount,lpRect,uFormat, &dtp, FALSE);
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