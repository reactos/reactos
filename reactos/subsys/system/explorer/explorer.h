/*
 * Copyright 2003 Martin Fuchs
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


 //
 // Explorer clone
 //
 // explorer.h
 //
 // Martin Fuchs, 23.07.2003
 //


#include "shell/entries.h"

#include "shell/winfs.h"
#include "shell/unixfs.h"
#include "shell/shellfs.h"

#include "utility/window.h"


#define	BUFFER_LEN				1024


#define IDW_STATUSBAR			0x100
#define IDW_TOOLBAR				0x101
#define IDW_DRIVEBAR			0x102
#define	IDW_FIRST_CHILD			0xC000	/*0x200*/


#define	WM_TRANSLATE_MSG		(WM_APP+2)
#define	WM_GET_FILEWND_PTR		(WM_APP+3)

#define	FRM_CALC_CLIENT			(WM_APP+4)
#define	Frame_CalcFrameClient(hwnd, prt) ((BOOL)SNDMSG(hwnd, FRM_CALC_CLIENT, 0, (LPARAM)(PRECT)prt))

#define	WM_OPEN_WINDOW			(WM_APP+5)

#define	WM_GET_CONTROLWINDOW	(WM_APP+6)


#define CLASSNAME_FRAME 		TEXT("CabinetWClass")	// same class name for frame window as in MS Explorer

#define CLASSNAME_CHILDWND		TEXT("WFS_Child")
#define CLASSNAME_WINEFILETREE	TEXT("WFS_Tree")


struct String
#ifdef UNICODE
 : public wstring
#else
 : public string
#endif
{
	operator LPCTSTR() const {return c_str();}
};

struct ResString : public String
{
	ResString(UINT nid);
};


#include "shell/mainframe.h"
#include "shell/pane.h"
#include "shell/filechild.h"
#include "shell/shellbrowser.h"
