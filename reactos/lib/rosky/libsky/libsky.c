/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
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
/* $Id: libsky.c,v 1.2 2004/08/12 15:41:36 weiden Exp $
 *
 * PROJECT:         SkyOS library
 * FILE:            lib/libsky/libsky.c
 * PURPOSE:         SkyOS library
 *
 * UPDATE HISTORY:
 *      08/12/2004  Created
 */
#include <windows.h>
/* #define NDEBUG */
#include "libsky.h"
#include "resource.h"

/*
 * @implemented
 */
void __cdecl
__to_kernel(int ret)
{
  DBG("__to_kernel: ret=0x%x\n", ret);
  ExitProcess(ret);
}

