#include <windows.h>


INT STDCALL FillRect( HDC hdc, const RECT *rect, HBRUSH hbrush )
{
    HBRUSH prevBrush;

    if (!(prevBrush = SelectObject( hdc, hbrush ))) return 0;
    PatBlt( hdc, rect->left, rect->top,
              rect->right - rect->left, rect->bottom - rect->top, PATCOPY );
    SelectObject( hdc, prevBrush );
    return 1;
}


WINBOOL STDCALL InvertRect( HDC hDC, CONST RECT *lprc)
{
    return PatBlt( hDC, lprc->left, lprc->top,
              lprc->right - lprc->left, lprc->bottom - lprc->top, DSTINVERT );
}
