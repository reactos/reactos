int IntSqrt(unsigned long dwNum);

#define DP_USEDCOLOR  0
#define DP_FREECOLOR  1
#define DP_USEDSHADOW 2
#define DP_FREESHADOW 3

STDAPI_(VOID) DrawPie(HDC hDC, LPCRECT prcItem, UINT uPctX10, BOOL TrueZr100,
                  UINT uOffset, const COLORREF *lpColors);
