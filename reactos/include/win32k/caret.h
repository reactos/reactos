/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            include/win32k/caret.h
 * PURPOSE:         GDI32/Win32k Caret interface
 * PROGRAMMER:
 *
 */

#ifndef WIN32K_CARET_H_INCLUDED
#define WIN32K_CARET_H_INCLUDED

typedef struct _THRDCARETINFO
{
  HWND hWnd;
  HBITMAP Bitmap;
  POINT Pos;
  SIZE Size;
  BYTE Visible;
  BYTE Showing;
} THRDCARETINFO, *PTHRDCARETINFO;

#endif /* WIN32K_FONT_H_INCLUDED */
