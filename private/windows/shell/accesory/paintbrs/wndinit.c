/****************************Module*Header******************************\
* Module Name: wndinit.c                                                *
*                                                                       *
*                                                                       *
*                                                                       *
* Created: 1989                                                         *
*                                                                       *
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
*                                                                       *
* A general description of how the module is used goes here.            *
*                                                                       *
* Additional information such as restrictions, limitations, or special  *
* algorithms used if they are externally visible or effect proper use   *
* of the module.                                                        *
\***********************************************************************/

#include <windows.h>

#include "port1632.h"

#include "pbrush.h"

extern TCHAR pgmName[], noFile[], pgmTitle[];
extern TCHAR *pbrushWndClass[];

BOOL WndInit(HINSTANCE hInstance)
{
  WNDCLASS  newClass;
  TCHAR     menuname[50];

  /* load strings from resource */
  LoadString(hInstance, IDSname, pgmName,  CharSizeOf(pgmName));
  LoadString(hInstance, IDStitle, pgmTitle, CharSizeOf(pgmTitle));

  /* assign values and register the parent windows class */
  newClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  newClass.hIcon = LoadIcon(hInstance, pgmName);
  newClass.hbrBackground = (HBRUSH) GetStockObject(GRAY_BRUSH);
  lstrcpy(menuname, pgmName);
//  if (2 <= (int)(0xff & GetVersion())) {
       lstrcat(menuname, TEXT("2"));
       newClass.hbrBackground = (HBRUSH) (COLOR_APPWORKSPACE + 1);
//  }
  newClass.lpszMenuName  = menuname;
  newClass.lpszClassName = pbrushWndClass[PARENTid];
  newClass.cbClsExtra    = newClass.cbWndExtra = 0;
  newClass.hInstance    = hInstance;
  newClass.style         = CS_HREDRAW | CS_VREDRAW;
  newClass.lpfnWndProc   = ParentWP;
  if (!RegisterClass(&newClass))
       return FALSE;

  /* assign values and register tool window class */
  newClass.hIcon         = NULL;
  newClass.lpszMenuName  = NULL;
  newClass.lpszClassName = pbrushWndClass[TOOLid];
  newClass.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
  newClass.style         = CS_DBLCLKS;
  newClass.lpfnWndProc   = ToolWP;
  if (!RegisterClass(&newClass))
       return FALSE;

  /* assign values and register pen window class */
  newClass.hCursor       = LoadCursor(hInstance, TEXT("sidearow"));
  newClass.lpszClassName = pbrushWndClass[SIZEid];
  newClass.lpfnWndProc   = SizeWP;
  if (!RegisterClass(&newClass))
       return FALSE;

  /* assign values and register color window class */
  newClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
  newClass.lpszClassName = pbrushWndClass[COLORid];
  newClass.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
  newClass.lpfnWndProc   = ColorWP;
  if (!RegisterClass(&newClass))
       return FALSE;

  /* assign values and register paint window class */
  newClass.hCursor       = (HCURSOR )NULL;
  newClass.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
  newClass.lpszClassName = pbrushWndClass[PAINTid];
  newClass.lpfnWndProc   = PaintWP;
  if (!RegisterClass(&newClass))
       return FALSE;

  /* assign values and register full (show screen) window class */
  newClass.hCursor       = (HCURSOR) NULL;
  newClass.lpszClassName = TEXT("pbFull");
  newClass.lpfnWndProc   = FullWP;
  if (!RegisterClass(&newClass))
       return FALSE;

  /* assign values and register zoom out window class */
  newClass.hCursor       = LoadCursor(hInstance, TEXT("pick"));
  newClass.lpszClassName = TEXT("pbZoomOut");
  newClass.hbrBackground = (HBRUSH) GetStockObject(GRAY_BRUSH);
  newClass.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
  newClass.lpfnWndProc   = ZoomOtWP;
  if (!RegisterClass(&newClass))
       return FALSE;

  return TRUE;
}
