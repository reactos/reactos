#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/region.h>
#include <win32k/gdiobj.h>
#include <include/rect.h>

//#define NDEBUG
#include <win32k/debug1.h>

/* FUNCTIONS *****************************************************************/

BOOL
W32kOffsetRect(LPRECT Rect, INT x, INT y)
{
  Rect->left += x;
  Rect->right += x;
  Rect->top += y;
  Rect->bottom += y;
  return(TRUE);
}


BOOL STDCALL
W32kUnionRect(PRECT Dest, const RECT* Src1, const RECT* Src2)
{
  if (W32kIsEmptyRect(Src1))
    {
      if (W32kIsEmptyRect(Src2))
	{
	  W32kSetEmptyRect(Dest);
	  return(FALSE);
	}
      else
	{
	  *Dest = *Src2;
	}
    }
  else
    {
      if (W32kIsEmptyRect(Src2))
	{
	  *Dest = *Src1;
	}
      else
	{
	  Dest->left = min(Src1->left, Src2->left);
	  Dest->top = min(Src1->top, Src2->top);
	  Dest->right = max(Src1->right, Src2->right);
	  Dest->bottom = max(Src1->bottom, Src2->bottom);
	}
    }
  return(TRUE);
}

BOOL STDCALL
W32kSetEmptyRect(PRECT Rect)
{
  Rect->left = Rect->right = Rect->top = Rect->bottom = 0;
  return(TRUE);
}

BOOL STDCALL
W32kIsEmptyRect(PRECT Rect)
{
  return(Rect->left >= Rect->right || Rect->top >= Rect->bottom);
}

BOOL STDCALL
W32kSetRect(PRECT Rect, INT left, INT top, INT right, INT bottom)
{
  Rect->left = left;
  Rect->top = top;
  Rect->right = right;
  Rect->bottom = bottom;
  return(TRUE);
}

BOOL STDCALL
W32kIntersectRect(PRECT Dest, const RECT* Src1, const RECT* Src2)
{
  if (W32kIsEmptyRect(Src1) || W32kIsEmptyRect(Src2) ||
      Src1->left >= Src2->right || Src2->left >= Src1->right ||
      Src1->top >= Src2->bottom || Src2->top >= Src1->bottom)
    {
      W32kSetEmptyRect(Dest);
      return(FALSE);
    }
  Dest->left = max(Src1->left, Src2->left);
  Dest->right = min(Src1->right, Src2->right);
  Dest->top = max(Src1->top, Src2->top);
  Dest->bottom = min(Src1->right, Src2->right);
  return(TRUE);
}
