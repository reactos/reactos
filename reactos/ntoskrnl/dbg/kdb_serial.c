/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
 * PROJECT:          ReactOS kernel
 * FILE:             ntoskrnl/dbg/kdb_serial.c
 * PURPOSE:          Serial driver
 * PROGRAMMER:       Original:
 *                   Victor Kirhenshtein (sauros@iname.com)
 *                   Jason Filby (jasonfilby@yahoo.com)
 *                   Modified:
 *                   arty
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <string.h>
#include <ntos/minmax.h>
#include <internal/kd.h>

#define NDEBUG
#include <debug.h>

extern KD_PORT_INFORMATION LogPortInfo;


CHAR
KdbTryGetCharSerial()
{
  UCHAR Result;

  while( !KdPortGetByteEx (&LogPortInfo, &Result) );

  return Result;
}
