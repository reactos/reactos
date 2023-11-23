/*
 * Copyright (C) 2002 Alexandre Julliard
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _WINRESRC_
#define _WINRESRC_

#ifndef WINVER
#define WINVER 0x0500
#endif

#ifndef _WIN32_IE
#define _WIN32_IE 0x0501
#endif

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0410
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#include <winuser.rh>
#include <commctrl.rh>
#include <dde.rh>
#include <winnt.rh>
#include <dlgs.h>
#include <winver.h>

#endif /* _WINRESRC_ */
