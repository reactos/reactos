/*
 * Copyright 2003, 2004 Martin Fuchs
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


#define _LIGHT_STARTMENU
#define _LAZY_ICONEXTRACT
#define _SINGLE_ICONEXTRACT
//#define _NO_WIN_FS


#include "utility/shellclasses.h"

#include "shell/entries.h"

#ifndef _NO_WIN_FS
#include "shell/winfs.h"
#endif

#include "shell/shellfs.h"

#ifndef ROSSHELL
#include "shell/unixfs.h"
#endif

#include "utility/window.h"


#define	IDW_STATUSBAR			0x100
#define	IDW_TOOLBAR				0x101
#define	IDW_EXTRABAR			0x102
#define	IDW_DRIVEBAR			0x103
#define	IDW_ADDRESSBAR			0x104
#define	IDW_SIDEBAR				0x106
#define	IDW_FIRST_CHILD			0xC000	/*0x200*/


#define	PM_GET_FILEWND_PTR		(WM_APP+0x05)
#define	PM_GET_SHELLBROWSER_PTR	(WM_APP+0x06)

#define	PM_GET_CONTROLWINDOW	(WM_APP+0x16)

#define	PM_RESIZE_CHILDREN		(WM_APP+0x17)
#define	PM_GET_WIDTH			(WM_APP+0x18)

#define	PM_REFRESH				(WM_APP+0x1B)
#define	PM_REFRESH_CONFIG		(WM_APP+0x1C)


#define	CLASSNAME_FRAME 		TEXT("CabinetWClass")	// same class name for frame window as in MS Explorer

#define	CLASSNAME_CHILDWND		TEXT("WFS_Child")
#define	CLASSNAME_WINEFILETREE	TEXT("WFS_Tree")


#include "shell/pane.h"
#include "shell/filechild.h"
#include "shell/shellbrowser.h"


#ifndef ROSSHELL

 /// Explorer command line parser
 // for commands like "/e,/root,c:\"
 // or "::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\::{21EC2020-3AEA-1069-A2DD-08002B30309D}" (launch of control panel)
struct ExplorerCmd
{
	ExplorerCmd()
	 :	_flags(0),
		_cmdShow(SW_SHOWNORMAL),
		_mdi(false),
		_valid_path(false)
	{
	}

	ExplorerCmd(LPCTSTR url, bool mdi)
	 :	_path(url),
		_flags(0),
		_cmdShow(SW_SHOWNORMAL),
		_mdi(mdi),
		_valid_path(true)	//@@
	{
	}

	bool	ParseCmdLine(LPCTSTR lpCmdLine);
	bool	EvaluateOption(LPCTSTR option);
	bool	IsValidPath() const;

	String	_path;
	int		_flags;	// OPEN_WINDOW_MODE
	int		_cmdShow;
	bool	_mdi;
	bool	_valid_path;
};

#include "shell/mainframe.h"

#endif
