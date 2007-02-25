/* 
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 2004, 2005, 2006 ReactOS Team
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
/* $Id: */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

VOID
DIB_32BPP_HLine(SURFOBJ *SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  PBYTE byteaddr = (ULONG_PTR)SurfObj->pvScan0 + y * SurfObj->lDelta;  
  PDWORD addr = (PDWORD)byteaddr + x1;		
  LONG cx = x1;
  while(cx < x2) 
  {
    *addr = (DWORD)c;
    ++addr;
    ++cx;
   }	  
}

BOOLEAN 
DIB_32BPP_ColorFill(SURFOBJ* DestSurface, RECTL* DestRect, ULONG color)
{			 
  ULONG DestY;	

  for (DestY = DestRect->top; DestY< DestRect->bottom; DestY++)
  {
    DIB_32BPP_HLine (DestSurface, DestRect->left, DestRect->right, DestY, color);
  }

  return TRUE;
}