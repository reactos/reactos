#include "precomp.h"

BOOL
WINAPI
Arc(
	HDC hDC,
	int nLeftRect,
	int nTopRect,
	int nRightRect,
	int nBottomRect,
	int nXStartArc,
	int nYStartArc,
	int nXEndArc,
	int nYEndArc
)
{
	return NtGdiArcInternal(GdiTypeArc,
	                        hDC,
	                        nLeftRect,
	                        nTopRect,
	                        nRightRect,
	                        nBottomRect,
	                        nXStartArc,
	                        nYStartArc,
	                        nXEndArc,
	                        nYEndArc);
}

BOOL
WINAPI
ArcTo(
	HDC  hDC,
	int  nLeftRect,
	int  nTopRect,
	int  nRightRect,
	int  nBottomRect,
	int  nXRadial1,
	int  nYRadial1,
	int  nXRadial2,
	int  nYRadial2)
{
	return NtGdiArcInternal(GdiTypeArcTo,
	                        hDC,
	                        nLeftRect,
	                        nTopRect,
	                        nRightRect,
	                        nBottomRect,
	                        nXRadial1,
	                        nYRadial1,
	                        nXRadial2,
	                        nYRadial2);
}

BOOL
WINAPI
Chord(
	HDC  hDC,
	int  nLeftRect,
	int  nTopRect,
	int  nRightRect,
	int  nBottomRect,
	int  nXRadial1,
	int  nYRadial1,
	int  nXRadial2,
	int  nYRadial2)
{
	return NtGdiArcInternal(GdiTypeChord,
	                        hDC,
	                        nLeftRect,
	                        nTopRect,
	                        nRightRect,
	                        nBottomRect,
	                        nXRadial1,
	                        nYRadial1,
	                        nXRadial2,
	                        nYRadial2);
}


