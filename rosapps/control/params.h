/*
 *   Control
 *   Copyright (C) 1998 by Marcel Baur <mbaur@g26.ethz.ch>
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

/* alphabetical list of recognized optional command line parameters */

#define szP_COLOR          _T("COLOR")
#define szP_DATETIME       _T("DATE/TIME")
#define szP_DESKTOP        _T("DESKTOP")
#define szP_INTERNATIONAL  _T("INTERNATIONAL")
#define szP_KEYBOARD       _T("KEYBOARD")
#define szP_MOUSE          _T("MOUSE")
#define szP_PORTS          _T("PORTS")
#define szP_PRINTERS       _T("PRINTERS")


/* alphabetical list of appropriate commands to execute */

#define szEXEC_PREFIX      _T("rundll32.exe")
#define szEXEC_ARGS        _T("Shell32.dll,Control_RunDLL _T(")

#define szC_COLOR          _T("desk.cpl,,2")
#define szC_DATETIME       _T("datetime.cpl")
#define szC_DESKTOP        _T("desk.cpl")
#define szC_FONTS          _T("main.cpl @3")
#define szC_INTERNATIONAL  _T("intl.cpl")
#define szC_KEYBOARD       _T("main.cpl @1")
#define szC_MOUSE          _T("main.cpl")
#define szC_PORTS          _T("sysdm.cpl,,1")
#define szC_PRINTERS       _T("main.cpl @2")

