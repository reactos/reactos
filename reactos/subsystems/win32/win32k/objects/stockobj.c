/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998 - 2004 ReactOS Team
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
/*
 * STOCKOBJ.C - GDI Stock Objects
 *
 * $Id$
 *
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

static COLORREF SysColors[] =
{
  RGB(212, 208, 200), /* COLOR_SCROLLBAR  */
  RGB(58, 110, 165),  /* COLOR_BACKGROUND  */
  RGB(10, 36, 106),   /* COLOR_ACTIVECAPTION  */
  RGB(128, 128, 128), /* COLOR_INACTIVECAPTION  */
  RGB(212, 208, 200), /* COLOR_MENU  */
  RGB(255, 255, 255), /* COLOR_WINDOW  */
  RGB(0, 0, 0),       /* COLOR_WINDOWFRAME  */
  RGB(0, 0, 0),       /* COLOR_MENUTEXT  */
  RGB(0, 0, 0),       /* COLOR_WINDOWTEXT  */
  RGB(255, 255, 255), /* COLOR_CAPTIONTEXT  */
  RGB(212, 208, 200), /* COLOR_ACTIVEBORDER  */
  RGB(212, 208, 200), /* COLOR_INACTIVEBORDER  */
  RGB(128, 128, 128), /* COLOR_APPWORKSPACE  */
  RGB(10, 36, 106),   /* COLOR_HIGHLIGHT  */
  RGB(255, 255, 255), /* COLOR_HIGHLIGHTTEXT  */
  RGB(212, 208, 200), /* COLOR_BTNFACE  */
  RGB(128, 128, 128), /* COLOR_BTNSHADOW  */
  RGB(128, 128, 128), /* COLOR_GRAYTEXT  */
  RGB(0, 0, 0),       /* COLOR_BTNTEXT  */
  RGB(212, 208, 200), /* COLOR_INACTIVECAPTIONTEXT  */
  RGB(255, 255, 255), /* COLOR_BTNHIGHLIGHT  */
  RGB(64, 64, 64),    /* COLOR_3DDKSHADOW  */
  RGB(212, 208, 200), /* COLOR_3DLIGHT  */
  RGB(0, 0, 0),       /* COLOR_INFOTEXT  */
  RGB(255, 255, 225), /* COLOR_INFOBK  */
  RGB(181, 181, 181), /* COLOR_UNKNOWN  */
  RGB(0, 0, 128),     /* COLOR_HOTLIGHT  */
  RGB(166, 202, 240), /* COLOR_GRADIENTACTIVECAPTION  */
  RGB(192, 192, 192), /* COLOR_GRADIENTINACTIVECAPTION  */
  RGB(49, 106, 197),  /* COLOR_MENUHILIGHT  */
  RGB(236, 233, 216)  /* COLOR_MENUBAR  */
};
#define NUM_SYSCOLORS (sizeof(SysColors) / sizeof(SysColors[0]))

static HPEN SysColorPens[NUM_SYSCOLORS];
static HBRUSH SysColorBrushes[NUM_SYSCOLORS];

/*  GDI stock objects */

static LOGPEN WhitePen =
{ PS_SOLID, { 0, 0 }, RGB(255,255,255) };

static LOGPEN BlackPen =
{ PS_SOLID, { 0, 0 }, RGB(0,0,0) };

static LOGPEN NullPen =
{ PS_NULL, { 0, 0 }, 0 };

static LOGFONTW OEMFixedFont =
{ 11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, OEM_CHARSET,
  0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier New" }; //Bitstream Vera Sans Mono

static LOGFONTW AnsiFixedFont =
{ 11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier New" }; //Bitstream Vera Sans Mono

/*static LOGFONTW AnsiVarFont =
 *{ 10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
 *  0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"MS Sans Serif" }; */

static LOGFONTW SystemFont =
{ 11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"Courier New" }; //Bitstream Vera Sans

static LOGFONTW DeviceDefaultFont =
{ 11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"MS Sans Serif" }; //Bitstream Vera Sans

static LOGFONTW SystemFixedFont =
{ 11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier New" }; //Bitstream Vera Sans Mono

/* FIXME: Is this correct? */
static LOGFONTW DefaultGuiFont =
{ 11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"MS Sans Serif" }; //Bitstream Vera Sans

#define NB_STOCK_OBJECTS (STOCK_LAST + 1)

static HGDIOBJ StockObjects[NB_STOCK_OBJECTS];

/*!
 * Creates a bunch of stock objects: brushes, pens, fonts.
*/
VOID FASTCALL
CreateStockObjects(void)
{
  UINT Object;

  DPRINT("Beginning creation of stock objects\n");

  /* Create GDI Stock Objects from the logical structures we've defined */

  StockObjects[WHITE_BRUSH] =  IntGdiCreateSolidBrush(RGB(255,255,255));
  StockObjects[DC_BRUSH]    =  IntGdiCreateSolidBrush(RGB(255,255,255));
  StockObjects[LTGRAY_BRUSH] = IntGdiCreateSolidBrush(RGB(192,192,192));
  StockObjects[GRAY_BRUSH] =   IntGdiCreateSolidBrush(RGB(128,128,128));
  StockObjects[DKGRAY_BRUSH] = IntGdiCreateSolidBrush(RGB(64,64,64));
  StockObjects[BLACK_BRUSH] =  IntGdiCreateSolidBrush(RGB(0,0,0));
  StockObjects[NULL_BRUSH] =   IntGdiCreateNullBrush();

  StockObjects[WHITE_PEN] = IntGdiExtCreatePen(WhitePen.lopnStyle, WhitePen.lopnWidth.x, BS_SOLID, WhitePen.lopnColor, 0, 0, 0, NULL, 0, TRUE, NULL);
  StockObjects[BLACK_PEN] = IntGdiExtCreatePen(BlackPen.lopnStyle, BlackPen.lopnWidth.x, BS_SOLID, BlackPen.lopnColor, 0, 0, 0, NULL, 0, TRUE, NULL);
  StockObjects[DC_PEN]    = IntGdiExtCreatePen(BlackPen.lopnStyle, BlackPen.lopnWidth.x, BS_SOLID, BlackPen.lopnColor, 0, 0, 0, NULL, 0, TRUE, NULL);
  StockObjects[NULL_PEN] = IntGdiExtCreatePen(NullPen.lopnStyle, NullPen.lopnWidth.x, BS_SOLID, NullPen.lopnColor, 0, 0, 0, NULL, 0, TRUE, NULL);

  (void) TextIntCreateFontIndirect(&OEMFixedFont, (HFONT*)&StockObjects[OEM_FIXED_FONT]);
  (void) TextIntCreateFontIndirect(&AnsiFixedFont, (HFONT*)&StockObjects[ANSI_FIXED_FONT]);
  (void) TextIntCreateFontIndirect(&SystemFont, (HFONT*)&StockObjects[SYSTEM_FONT]);
  (void) TextIntCreateFontIndirect(&DeviceDefaultFont, (HFONT*)&StockObjects[DEVICE_DEFAULT_FONT]);
  (void) TextIntCreateFontIndirect(&SystemFixedFont, (HFONT*)&StockObjects[SYSTEM_FIXED_FONT]);
  (void) TextIntCreateFontIndirect(&DefaultGuiFont, (HFONT*)&StockObjects[DEFAULT_GUI_FONT]);

  StockObjects[DEFAULT_PALETTE] = (HGDIOBJ*)PALETTE_Init();

  for (Object = 0; Object < NB_STOCK_OBJECTS; Object++)
    {
      if (NULL != StockObjects[Object])
	{
	  GDIOBJ_ConvertToStockObj(GdiHandleTable, &StockObjects[Object]);
	}
    }

  DPRINT("Completed creation of stock objects\n");
}

/*!
 * Return stock object.
 * \param	Object - stock object id.
 * \return	Handle to the object.
*/
HGDIOBJ STDCALL
NtGdiGetStockObject(INT Object)
{
  DPRINT("NtGdiGetStockObject index %d\n", Object);

  return ((Object < 0) || (NB_STOCK_OBJECTS <= Object)) ? NULL : StockObjects[Object];
}

BOOL FASTCALL
IntSetSysColors(UINT nColors, INT *Elements, COLORREF *Colors)
{
  UINT i;

  ASSERT(Elements);
  ASSERT(Colors);

  for(i = 0; i < nColors; i++)
  {
    if((UINT)(*Elements) < NUM_SYSCOLORS)
    {
      SysColors[*Elements] = *Colors;
      IntGdiSetSolidBrushColor(SysColorBrushes[*Elements], *Colors);
      IntGdiSetSolidPenColor(SysColorPens[*Elements], *Colors);
    }
    Elements++;
    Colors++;
  }
  UserPostMessage(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0);

  return nColors > 0;
}

BOOL FASTCALL
IntGetSysColorBrushes(HBRUSH *Brushes, UINT nBrushes)
{
  UINT i;

  ASSERT(Brushes);

  if(nBrushes > NUM_SYSCOLORS)
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  for(i = 0; i < nBrushes; i++)
  {
    *(Brushes++) = SysColorBrushes[i];
  }

  return nBrushes > 0;
}

HGDIOBJ FASTCALL
IntGetSysColorBrush(INT Object)
{
  return ((Object < 0) || (NUM_SYSCOLORS <= Object)) ? NULL : SysColorBrushes[Object];
}

BOOL FASTCALL
IntGetSysColorPens(HPEN *Pens, UINT nPens)
{
  UINT i;

  ASSERT(Pens);

  if(nPens > NUM_SYSCOLORS)
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  for(i = 0; i < nPens; i++)
  {
    *(Pens++) = SysColorPens[i];
  }

  return nPens > 0;
}

BOOL FASTCALL
IntGetSysColors(COLORREF *Colors, UINT nColors)
{
  UINT i;
  COLORREF *col;

  ASSERT(Colors);

  if(nColors > NUM_SYSCOLORS)
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  col = &SysColors[0];
  for(i = 0; i < nColors; i++)
  {
    *(Colors++) = *(col++);
  }

  return nColors > 0;
}

DWORD FASTCALL
IntGetSysColor(INT nIndex)
{
  return (NUM_SYSCOLORS <= (UINT)nIndex) ? 0 : SysColors[nIndex];
}

VOID FASTCALL
CreateSysColorObjects(VOID)
{
  UINT i;
  LOGPEN Pen;

  /* Create the syscolor brushes */
  for(i = 0; i < NUM_SYSCOLORS; i++)
  {
    if(SysColorBrushes[i] == NULL)
    {
      SysColorBrushes[i] = IntGdiCreateSolidBrush(SysColors[i]);
      if(SysColorBrushes[i] != NULL)
      {
        GDIOBJ_ConvertToStockObj(GdiHandleTable, (HGDIOBJ*)&SysColorBrushes[i]);
      }
    }
  }

  /* Create the syscolor pens */
  Pen.lopnStyle = PS_SOLID;
  Pen.lopnWidth.x = 0;
  Pen.lopnWidth.y = 0;
  for(i = 0; i < NUM_SYSCOLORS; i++)
  {
    if(SysColorPens[i] == NULL)
    {
      Pen.lopnColor = SysColors[i];
      SysColorPens[i] = IntGdiExtCreatePen(Pen.lopnStyle, Pen.lopnWidth.x, BS_SOLID, Pen.lopnColor, 0, 0, 0, NULL, 0, TRUE, NULL);
      if(SysColorPens[i] != NULL)
      {
        GDIOBJ_ConvertToStockObj(GdiHandleTable, (HGDIOBJ*)&SysColorPens[i]);
      }
    }
  }
}

/* EOF */
