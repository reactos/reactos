/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 2003 ReactOS Team
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
/* $Id: hotkey.c,v 1.10.12.1 2004/07/15 20:07:17 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          HotKey support
 * FILE:             subsys/win32k/ntuser/hotkey.c
 * PROGRAMER:        Eric Kohl
 * REVISION HISTORY:
 *       02-11-2003  EK  Created
 */
#include <w32k.h>

#define NDEBUG
#include <debug.h>


