#ifdef UNICODE
#undef UNICODE
#endif

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/kapi.h>

/*
 * @implemented
 */
BOOL
STDCALL
Polygon(HDC	hdc,
	CONST POINT	*lpPoints,
	int	nCount)
{
  return NtGdiPolygon(hdc, (CONST PPOINT)lpPoints, nCount);
}


/*
 * @implemented
 */
BOOL
STDCALL
Rectangle(HDC  hdc,
	int  nLeftRect,
	int  nTopRect,
	int  nRightRect,
	int  nBottomRect)
{
  return NtGdiRectangle(hdc, nLeftRect, nTopRect, nRightRect, nBottomRect);
}

/*
 * @implemented
 */
BOOL
STDCALL
RoundRect(
	HDC hdc,
	int nLeftRect,
	int nTopRect,
	int nRightRect,
	int nBottomRect,
	int nWidth,
	int nHeight
	)
{
  return NtGdiRoundRect(hdc, nLeftRect, nTopRect, nRightRect, nBottomRect, nWidth, nHeight);
}

/*
 * @implemented
 */
BOOL
STDCALL
PolyPolygon(
	HDC		hdc,
	CONST POINT	*lpPoints,
	CONST INT	*lpPolyCounts,
	int		nCount
	)
{
  return PolyPolygon(hdc, (LPPOINT)lpPoints, (LPINT)lpPolyCounts, nCount);
}


/*
 * @implemented
 */
BOOL
STDCALL
Ellipse(HDC hdc,
        int nLeftRect,
        int nTopRect,
        int nRightRect,
        int nBottomRect)
{
  return NtGdiEllipse(hdc, nLeftRect, nTopRect, nRightRect, nBottomRect);
}


/*
 * @implemented
 */
BOOL
STDCALL
Pie(HDC hdc,
    int nLeftRect,
    int nTopRect,
    int nRightRect,
    int nBottomRect,
    int nXRadial1,
    int nYRadial1,
    int nXRadial2,
    int nYRadial2)
{
  return NtGdiPie(hdc, nLeftRect, nTopRect, nRightRect, nBottomRect, nXRadial1, nYRadial1,
                  nXRadial2, nYRadial2);
}

BOOL STDCALL
ExtFloodFill(
  HDC hdc,          // handle to DC
  int nXStart,      // starting x-coordinate 
  int nYStart,      // starting y-coordinate 
  COLORREF crColor, // fill color
  UINT fuFillType   // fill type
)
{
  return NtGdiExtFloodFill(hdc, nXStart, nYStart, crColor, fuFillType);
}

/*
 * @implemented
 */
BOOL STDCALL 
FloodFill(
  HDC hdc,          // handle to DC
  int nXStart,      // starting x-coordinate
  int nYStart,      // starting y-coordinate
  COLORREF crFill   // fill color
)
{
  return NtGdiFloodFill(hdc, nXStart, nYStart, crFill);
}

/*
 * @implemented
 */
BOOL STDCALL
GdiGradientFill(
  HDC hdc,
  PTRIVERTEX pVertex,
  ULONG dwNumVertex,
  PVOID pMesh,
  ULONG dwNumMesh,
  ULONG dwMode
)
{
  return NtGdiGradientFill(hdc, pVertex, dwNumVertex, pMesh, dwNumMesh, dwMode);
}

