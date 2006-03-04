/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$ */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

VOID FASTCALL
IntGdiSetEmptyRect(PRECT Rect)
{
  Rect->left = Rect->right = Rect->top = Rect->bottom = 0;
}

BOOL STDCALL
NtGdiSetEmptyRect(PRECT UnsafeRect)
{
  RECT Rect;
  NTSTATUS Status = STATUS_SUCCESS;

  IntGdiSetEmptyRect(&Rect);

  _SEH_TRY
  {
    ProbeForWrite(UnsafeRect,
                  sizeof(RECT),
                  1);
    *UnsafeRect = Rect;
  }
  _SEH_HANDLE
  {
    Status = _SEH_GetExceptionCode();
  }
  _SEH_END;

  if (! NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return FALSE;
    }

  return TRUE;
}

BOOL FASTCALL
IntGdiIsEmptyRect(const RECT* Rect)
{
  return(Rect->left >= Rect->right || Rect->top >= Rect->bottom);
}

BOOL STDCALL
NtGdiIsEmptyRect(const RECT* UnsafeRect)
{
  RECT Rect = {0};
  NTSTATUS Status = STATUS_SUCCESS;

  _SEH_TRY
  {
    ProbeForRead(UnsafeRect,
                 sizeof(RECT),
                 1);
    Rect = *UnsafeRect;
  }
  _SEH_HANDLE
  {
    Status = _SEH_GetExceptionCode();
  }
  _SEH_END;
  if (! NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return FALSE;
    }

  return IntGdiIsEmptyRect(&Rect);
}

VOID FASTCALL
IntGdiOffsetRect(LPRECT Rect, INT x, INT y)
{
  Rect->left += x;
  Rect->right += x;
  Rect->top += y;
  Rect->bottom += y;
}

BOOL STDCALL
NtGdiOffsetRect(LPRECT UnsafeRect, INT x, INT y)
{
  RECT Rect = {0};
  NTSTATUS Status = STATUS_SUCCESS;

  _SEH_TRY
  {
    ProbeForRead(UnsafeRect,
                 sizeof(RECT),
                 1);
    Rect = *UnsafeRect;
  }
  _SEH_HANDLE
  {
    Status = _SEH_GetExceptionCode();
  }
  _SEH_END;
  if (! NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return FALSE;
    }

  IntGdiOffsetRect(&Rect, x, y);

  _SEH_TRY
  {
    ProbeForWrite(UnsafeRect,
                  sizeof(RECT),
                  1);
    *UnsafeRect = Rect;
  }
  _SEH_HANDLE
  {
    Status = _SEH_GetExceptionCode();
  }
  _SEH_END;
  if (! NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return FALSE;
    }

  return TRUE;
}

BOOL FASTCALL
IntGdiUnionRect(PRECT Dest, const RECT* Src1, const RECT* Src2)
{
  if (IntGdiIsEmptyRect(Src1))
    {
      if (IntGdiIsEmptyRect(Src2))
	{
	  IntGdiSetEmptyRect(Dest);
	  return FALSE;
	}
      else
	{
	  *Dest = *Src2;
	}
    }
  else
    {
      if (IntGdiIsEmptyRect(Src2))
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

  return TRUE;
}

BOOL STDCALL
NtGdiUnionRect(PRECT UnsafeDest, const RECT* UnsafeSrc1, const RECT* UnsafeSrc2)
{
    RECT Dest, Src1 = {0}, Src2 = {0};
  NTSTATUS Status = STATUS_SUCCESS;
  BOOL Ret;

  _SEH_TRY
  {
    ProbeForRead(UnsafeSrc1,
                 sizeof(RECT),
                 1);
    ProbeForRead(UnsafeSrc2,
                 sizeof(RECT),
                 1);
    Src1 = *UnsafeSrc1;
    Src2 = *UnsafeSrc2;
  }
  _SEH_HANDLE
  {
    Status = _SEH_GetExceptionCode();
  }
  _SEH_END;
  if (! NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return FALSE;
    }

  Ret = IntGdiUnionRect(&Dest, &Src1, &Src2);

  if (Ret)
    {
      _SEH_TRY
      {
        ProbeForWrite(UnsafeDest,
                      sizeof(RECT),
                      1);
        *UnsafeDest = Dest;
      }
      _SEH_HANDLE
      {
        Status = _SEH_GetExceptionCode();
      }
      _SEH_END;
      if (! NT_SUCCESS(Status))
        {
          SetLastNtError(Status);
          return FALSE;
        }
    }

  return Ret;
}

VOID FASTCALL
IntGdiSetRect(PRECT Rect, INT left, INT top, INT right, INT bottom)
{
  Rect->left = left;
  Rect->top = top;
  Rect->right = right;
  Rect->bottom = bottom;
}

BOOL STDCALL
NtGdiSetRect(PRECT UnsafeRect, INT left, INT top, INT right, INT bottom)
{
  RECT Rect;
  NTSTATUS Status = STATUS_SUCCESS;

  IntGdiSetRect(&Rect, left, top, right, bottom);

  _SEH_TRY
  {
    ProbeForWrite(UnsafeRect,
                  sizeof(RECT),
                  1);
    *UnsafeRect = Rect;
  }
  _SEH_HANDLE
  {
    Status = _SEH_GetExceptionCode();
  }
  _SEH_END;
  if (! NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return FALSE;
    }

  return TRUE;
}

BOOL FASTCALL
IntGdiIntersectRect(PRECT Dest, const RECT* Src1, const RECT* Src2)
{
  if (IntGdiIsEmptyRect(Src1) || IntGdiIsEmptyRect(Src2) ||
      Src1->left >= Src2->right || Src2->left >= Src1->right ||
      Src1->top >= Src2->bottom || Src2->top >= Src1->bottom)
    {
      IntGdiSetEmptyRect(Dest);
      return FALSE;
    }

  Dest->left = max(Src1->left, Src2->left);
  Dest->right = min(Src1->right, Src2->right);
  Dest->top = max(Src1->top, Src2->top);
  Dest->bottom = min(Src1->bottom, Src2->bottom);

  return TRUE;
}

BOOL STDCALL
NtGdiIntersectRect(PRECT UnsafeDest, const RECT* UnsafeSrc1, const RECT* UnsafeSrc2)
{
  RECT Dest, Src1, Src2 = {0};
  NTSTATUS Status = STATUS_SUCCESS;
  BOOL Ret;

  _SEH_TRY
  {
    ProbeForRead(UnsafeSrc1,
                 sizeof(RECT),
                 1);
    ProbeForRead(UnsafeSrc2,
                 sizeof(RECT),
                 1);
    Src1 = *UnsafeSrc1;
    Src2 = *UnsafeSrc2;
  }
  _SEH_HANDLE
  {
    Status = _SEH_GetExceptionCode();
  }
  _SEH_END;
  if (! NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return FALSE;
    }

  Ret = IntGdiIntersectRect(&Dest, &Src2, &Src2);

  if (Ret)
    {
      _SEH_TRY
      {
        ProbeForWrite(UnsafeDest,
                      sizeof(RECT),
                      1);
        *UnsafeDest = Dest;
      }
      _SEH_HANDLE
      {
        Status = _SEH_GetExceptionCode();
      }
      _SEH_END;
      if (! NT_SUCCESS(Status))
        {
          SetLastNtError(Status);
          return FALSE;
        }
    }

  return Ret;
}

/* EOF */
