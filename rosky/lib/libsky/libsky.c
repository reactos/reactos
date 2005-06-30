/*
 * ROSky - SkyOS Application Layer
 * Copyright (C) 2004 ReactOS Team
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
/* $Id$
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


/*
 * @implemented
 */
void __cdecl
__libc_init_memory(void *end,
                   void *__bss_end__,
                   void *__bss_start__)
{
  DBG("__libc_init_memory: end=0x%x __bss_end__=0x%x __bss_start__=0x%x\n", end, __bss_end__, __bss_start__);
  RtlZeroMemory(__bss_start__, (PCHAR)__bss_end__ - (PCHAR)__bss_start__);
  /* FIXME - initialize other stuff */
}

