/*
 * PROJECT:         Win32 subsystem
 * LICENSE:         See COPYING in the top level directory
 * FILE:            win32ss/gdi/dib/dib32bppc.c
 * PURPOSE:         C language equivalents of asm optimised 32bpp functions
 * PROGRAMMERS:     Jason Filby
 *                  Magnus Olsen
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

VOID
DIB_32BPP_HLine(SURFOBJ *SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  PBYTE byteaddr = (PBYTE)((ULONG_PTR)SurfObj->pvScan0 + y * SurfObj->lDelta);
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

  /* Make WellOrdered by making top < bottom and left < right */
  RECTL_vMakeWellOrdered(DestRect);

  for (DestY = DestRect->top; DestY< DestRect->bottom; DestY++)
  {
    DIB_32BPP_HLine (DestSurface, DestRect->left, DestRect->right, DestY, color);
  }

  return TRUE;
}
