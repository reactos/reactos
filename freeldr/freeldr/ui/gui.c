/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
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
	
#include <freeldr.h>
#include <ui.h>
#include "gui.h"
#include <rtl.h>
#include <mm.h>
#include <debug.h>
#include <inifile.h>
#include <version.h>

VOID GuiDrawBackdrop(VOID)
{
}

VOID GuiFillArea(U32 Left, U32 Top, U32 Right, U32 Bottom, UCHAR FillChar, UCHAR Attr /* Color Attributes */)
{
}

VOID GuiDrawShadow(U32 Left, U32 Top, U32 Right, U32 Bottom)
{
}

VOID GuiDrawBox(U32 Left, U32 Top, U32 Right, U32 Bottom, UCHAR VertStyle, UCHAR HorzStyle, BOOL Fill, BOOL Shadow, UCHAR Attr)
{
}

VOID GuiDrawText(U32 X, U32 Y, PUCHAR Text, UCHAR Attr)
{
}

VOID GuiDrawStatusText(PUCHAR StatusText)
{
}

VOID GuiUpdateDateTime(VOID)
{
}

VOID GuiSaveScreen(PUCHAR Buffer)
{
}

VOID GuiRestoreScreen(PUCHAR Buffer)
{
}

VOID GuiMessageBox(PUCHAR MessageText)
{
}

VOID GuiMessageBoxCritical(PUCHAR MessageText)
{
}

VOID GuiDrawProgressBar(U32 Position, U32 Range)
{
}

UCHAR GuiTextToColor(PUCHAR ColorText)
{
	return 0;
}

UCHAR GuiTextToFillStyle(PUCHAR FillStyleText)
{
	return 0;
}
