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
/* $Id: mda.c,v 1.2 2002/09/07 15:12:54 chorns Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/kd/mda.c
 * PURPOSE:         Support for debugging using an MDA card.
 * PROGRAMMER:      David Welch <welch@cwcom.net>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include "../dbg/kdb.h"

/* VARIABLES ***************************************************************/

STATIC ULONG MdaIndexPort;
STATIC ULONG MdaValuePort;
STATIC ULONG MdaStatusPort;
STATIC ULONG MdaGfxPort;
STATIC ULONG MdaModePort;
STATIC PUSHORT VideoBuffer;

#define MDA_COLUMNS (80)
#define MDA_LINES   (25)

STATIC ULONG MdaCursorX, MdaCursorY;

/* PRIVATE FUNCTIONS ********************************************************/

#if 0
VOID STATIC
KdWriteByteMDA(ULONG Reg, ULONG Value)
{
  WRITE_PORT_UCHAR((PUCHAR)MdaIndexPort, (CHAR)Reg);
  WRITE_PORT_UCHAR((PUCHAR)MdaValuePort, (CHAR)Value);
}

VOID STATIC
KdWriteWordMDA(ULONG Reg, ULONG Value)
{
  WRITE_PORT_UCHAR((PUCHAR)MdaIndexPort, (CHAR)Reg);
  WRITE_PORT_UCHAR((PUCHAR)MdaValuePort, (CHAR)(Value >> 8));
  WRITE_PORT_UCHAR((PUCHAR)MdaIndexPort, (CHAR)(Reg + 1));
  WRITE_PORT_UCHAR((PUCHAR)MdaValuePort, (CHAR)(Value & 0xFF));
}
#endif

VOID 
KdInitializeMda(VOID)
{
  /* Setup the variables for the various port addresses. */
  MdaIndexPort = 0x3b4;
  MdaValuePort = 0x3b5;
  MdaModePort = 0x3b8;
  MdaStatusPort = 0x3ba;
  MdaGfxPort = 0x3bf;

  VideoBuffer = (PUSHORT)(0xd0000000 + 0xb0000);

  MdaCursorX = MdaCursorY = 0;
}

VOID STATIC
KdScrollMda(VOID)
{
  memmove(&VideoBuffer[(MDA_COLUMNS * 0) + 0], 
	  &VideoBuffer[(MDA_COLUMNS * 1) + 0],
	  MDA_COLUMNS * (MDA_LINES - 1) * 2);		  
  memset(&VideoBuffer[(MDA_COLUMNS * (MDA_LINES - 1)) + 0], 0,
	 MDA_COLUMNS * 2);
}

VOID STATIC
KdPutCharMda(CHAR Ch)
{
  if (Ch == '\n')
    {
      if (MdaCursorY == (MDA_LINES - 1))
	{
	  KdScrollMda();
	}
      else
	{
	  MdaCursorY++;
	}
      MdaCursorX = 0;
      return;
    }
  VideoBuffer[(MdaCursorY * MDA_COLUMNS) + MdaCursorX] = (Ch & 0xFF) | 0x0700;
  MdaCursorX++;
  if (MdaCursorX == (MDA_COLUMNS - 1))
    {
      if (MdaCursorY == (MDA_LINES - 1))
	{
	  KdScrollMda();
	}
      else
	{
	  MdaCursorY++;
	}
      MdaCursorX = 0;
    }
}

VOID 
KdPrintMda(PCH pch)
{
  while((*pch) != 0)
    {
      if ((*pch) == '\t')
	{
	  KdPutCharMda(' ');
	  KdPutCharMda(' ');
	  KdPutCharMda(' ');
	  KdPutCharMda(' ');
	}
      if ((*pch) == '\r')
	{
	  /* Nothing. */
	}
      else
	{
	  KdPutCharMda(*pch);
	}
      pch++;
    }
}

/* EOF */
