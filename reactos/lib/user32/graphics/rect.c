/*
 * Rectangle-related functions
 *
 * CopyxRight 1993, 1996 Alexandre Julliard
 *
 */

#include <windows.h>
#include <user32/win.h>

WINBOOL
STDCALL
SetRect(
	LPRECT lprc,
	int xLeft,
	int yTop,
	int xRight,
	int yBottom)
{
	if ( lprc == NULL )
		return FALSE;
    	lprc->left   = xLeft;
    	lprc->right  = xRight;
    	lprc->top    = yTop;
    	lprc->bottom = yBottom;
	return TRUE;
}




/***********************************************************************
 *           SetRectEmpty32    (USER32.500)
 */
WINBOOL STDCALL SetRectEmpty( LPRECT lprc )
{
	if ( lprc == NULL )
		return FALSE;
    	lprc->left = lprc->right = lprc->top = lprc->bottom = 0;
	return TRUE;
}



//added memcpy and check BD

/***********************************************************************
 *           CopyRect32    (USER32.62)
 */
WINBOOL
STDCALL
CopyRect(
	 LPRECT lprcDst,
	 CONST RECT *lprcSrc)
{
    if ( lprcDst == NULL || lprcSrc == NULL )
	return FALSE;
    *lprcDst = *lprcSrc;
    return TRUE;
}





/***********************************************************************
 *           IsRectEmpty32    (USER32.347)
 */
WINBOOL STDCALL IsRectEmpty( const RECT *lprc )
{
	if ( lprc == NULL )
		return TRUE;
    	return ((lprc->left == lprc->right) || (lprc->top == lprc->bottom));
}






/***********************************************************************
 *           PtInRect32    (USER32.424)
 */
WINBOOL
STDCALL
PtInRect(
	 CONST RECT *lprc,
	 POINT pt)
{
    return ((pt.x >= lprc->left) && (pt.x < lprc->right) &&
	    (pt.y >= lprc->top) && (pt.y < lprc->bottom));
}




/***********************************************************************
 *           OffsetRect32    (USER32.406)
 */
WINBOOL
STDCALL
OffsetRect(
	   LPRECT lprc,
	   int dx,
	   int dy)
{
	if ( lprc == NULL )
		return FALSE;
    	lprc->left   += dx;
    	lprc->right  += dx;
    	lprc->top    += dy;
    	lprc->bottom += dy;    
	return TRUE;
}





/***********************************************************************
 *           InflateRect32    (USER32.321)
 */
WINBOOL STDCALL InflateRect( LPRECT lprc, INT dx, INT dy )
{
    	lprc->left   -= dx;
    	lprc->top 	 -= dy;
    	lprc->right  += dx;
    	lprc->bottom += dy;
	return TRUE;
}





/***********************************************************************
 *           IntersectRect32    (USER32.327)
 */
WINBOOL
STDCALL
IntersectRect(
	      LPRECT lprcDst,
	      CONST RECT *lprcSrc1,
	      CONST RECT *lprcSrc2)
{
    if (IsRectEmpty(lprcSrc1) || IsRectEmpty(lprcSrc2) ||
	(lprcSrc1->left >= lprcSrc2->right) || (lprcSrc2->left >= lprcSrc1->right) ||
	(lprcSrc1->top >= lprcSrc2->bottom) || (lprcSrc2->top >= lprcSrc1->bottom))
    {
	SetRectEmpty( lprcDst );
	return FALSE;
    }
    lprcDst->left   = MAX( lprcSrc1->left, lprcSrc2->left );
    lprcDst->right  = MIN( lprcSrc1->right, lprcSrc2->right );
    lprcDst->top    = MAX( lprcSrc1->top, lprcSrc2->top );
    lprcDst->bottom = MIN( lprcSrc1->bottom, lprcSrc2->bottom );
    return TRUE;
}




/***********************************************************************
 *           UnionRect32    (USER32.559)
 */
WINBOOL STDCALL UnionRect( LPRECT lprcDst, const RECT *lprcSrc1,
                           const RECT *lprcSrc2 )
{
    if (IsRectEmpty(lprcSrc1))
    {
	if (IsRectEmpty(lprcSrc2))
	{
	    SetRectEmpty( lprcDst );
	    return FALSE;
	}
	else *lprcDst = *lprcSrc2;
    }
    else
    {
	if (IsRectEmpty(lprcSrc2)) 
		*lprcDst = *lprcSrc1;
	else
	{
	    lprcDst->left   = MIN( lprcSrc1->left, lprcSrc2->left );
	    lprcDst->right  = MAX( lprcSrc1->right, lprcSrc2->right );
	    lprcDst->top    = MIN( lprcSrc1->top, lprcSrc2->top );
	    lprcDst->bottom = MAX( lprcSrc1->bottom, lprcSrc2->bottom );	    
	}
    }
    return TRUE;
}




/***********************************************************************
 *           EqualRect32    (USER32.194)
 */
WINBOOL
STDCALL
EqualRect(
	  CONST RECT *lprc1,
	  CONST RECT *lprc2)
{
    return ((lprc1->left == lprc2->left) && (lprc1->right == lprc2->right) &&
	    (lprc1->top == lprc2->top) && (lprc1->bottom == lprc2->bottom));
}





/***********************************************************************
 *           SubtractRect32    (USER32.536)
 */
WINBOOL
STDCALL
SubtractRect(
	     LPRECT lprcDst,
	     CONST RECT *lprcSrc1,
	     CONST RECT *lprcSrc2)
{
    RECT tmp;

    if (IsRectEmpty( lprcSrc1 ))
    {
	SetRectEmpty( lprcDst );
	return FALSE;
    }
// changed BD
    CopyRect(lprcDst,lprcSrc1);
    
    if (IntersectRect( &tmp, lprcSrc1, lprcSrc2 ))
    {
	if (EqualRect( &tmp, lprcDst ))
	{
	    SetRectEmpty( lprcDst );
	    return FALSE;
	}
	if ((tmp.top == lprcDst->top) && (tmp.bottom == lprcDst->bottom))
	{
	    if (tmp.left == lprcDst->left) lprcDst->left = tmp.right;
	    else if (tmp.right == lprcDst->right) lprcDst->right = tmp.left;
	}
	else if ((tmp.left == lprcDst->left) && (tmp.right == lprcDst->right))
	{
	    if (tmp.top == lprcDst->top) lprcDst->top = tmp.bottom;
	    else if (tmp.bottom == lprcDst->bottom) lprcDst->bottom = tmp.top;
	}
    }
    return TRUE;
}
