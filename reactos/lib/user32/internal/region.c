#include <windows.h>

INT SelectVisRgn(HDC hdc,HRGN hrgn)
{
	return SelectClipRgn(hdc,hrgn);
}

INT  RestoreVisRgn(HDC hdc)
{
	return SelectClipRgn(hdc,NULL);
}

INT  ExcludeVisRect(HDC hDC,INT nLeftRect,INT nTopRect,INT nRightRect,INT nBottomRect)
{
	return ExcludeClipRect(hDC, nLeftRect,	nTopRect,nRightRect,nBottomRect );
}


HRGN InquireVisRgn(HDC hdc)
{
	return hdc;
}
HRGN  SaveVisRgn(HDC hdc)
{
	return NULL;
}

/***********************************************************************
 *           REGION_UnionRectWithRgn
 *           Adds a rectangle to a HRGN32
 *           A helper used by scroll.c
 */
WINBOOL REGION_UnionRectWithRgn( HRGN hrgn, const RECT *lpRect )
{

    return TRUE;
}