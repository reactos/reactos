/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
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

VOID GuiDrawBackdrop(VOID)
{
}

VOID GuiFillArea(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, UCHAR FillChar, UCHAR Attr /* Color Attributes */)
{
}

VOID GuiDrawShadow(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom)
{
}

VOID GuiDrawBox(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, UCHAR VertStyle, UCHAR HorzStyle, BOOLEAN Fill, BOOLEAN Shadow, UCHAR Attr)
{
}

VOID GuiDrawText(ULONG X, ULONG Y, PUCHAR Text, UCHAR Attr)
{
}

VOID GuiDrawStatusText(PCSTR StatusText)
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

VOID GuiMessageBox(PCSTR MessageText)
{
}

VOID GuiMessageBoxCritical(PCSTR MessageText)
{
}

VOID GuiDrawProgressBar(ULONG Position, ULONG Range)
{
}

UCHAR GuiTextToColor(PCSTR ColorText)
{
	return 0;
}

UCHAR GuiTextToFillStyle(PCSTR FillStyleText)
{
	return 0;
}

const UIVTBL GuiVtbl =
{
	/*GuiInitialize,
	GuiUnInitialize,
	GuiDrawBackdrop,
	GuiFillArea,
	GuiDrawShadow,
	GuiDrawBox,
	GuiDrawText,
	GuiDrawCenteredText,
	GuiDrawStatusText,
	GuiUpdateDateTime,
	GuiMessageBox,
	GuiMessageBoxCritical,
	GuiDrawProgressBarCenter,
	GuiDrawProgressBar,
	GuiEditBox,
	GuiTextToColor,
	GuiTextToFillStyle,
	GuiFadeInBackdrop,
	GuiFadeOut,
	GuiDisplayMenu,*/
};
