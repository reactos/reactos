#include <windows.h>



INT STDCALL FrameRect( HDC hdc, const RECT *rect, HBRUSH hbrush )
{
    HBRUSH prevBrush;
    //int left, top, right, bottom;

    if ( hdc == NULL )
	return 0;

    //left   = XLPTODP( dc, rect->left );
    //top    = YLPTODP( dc, rect->top );
    //right  = XLPTODP( dc, rect->right );
    //bottom = YLPTODP( dc, rect->bottom );

    //if ( (right <= left) || (bottom <= top) ) return 0;
    if (!(prevBrush = SelectObject( hdc, hbrush ))) return 0;
    
    PatBlt( hdc, rect->left, rect->top, 1,
	      rect->bottom - rect->top, PATCOPY );
    PatBlt( hdc, rect->right - 1, rect->top, 1,
	      rect->bottom - rect->top, PATCOPY );
    PatBlt( hdc, rect->left, rect->top,
	      rect->right - rect->left, 1, PATCOPY );
    PatBlt( hdc, rect->left, rect->bottom - 1,
	      rect->right - rect->left, 1, PATCOPY );

    SelectObject( hdc, prevBrush );
    return 1;
}


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
