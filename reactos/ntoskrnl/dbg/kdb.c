/*
 *  ReactOS kernel
 *  Copyright (C) 2001 David Welch <welch@cwcom.net>
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
/* $Id: kdb.c,v 1.1 2001/04/10 17:48:16 dwelch Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/dbg/kdb.c
 * PURPOSE:         Kernel debugger
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 01/03/01
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include "kdb.h"

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID
KdbGetCommand(PCH Buffer)
{
  CHAR Key;

  for (;;)
    {
      while ((Key = KdbTryGetCharKeyboard()) == -1);

      if (Key == '\r' || Key == '\n')
	{
	  DbgPrint("\n");
	  *Buffer = 0;
	  return;
	}

      DbgPrint("%c", Key);

      *Buffer = Key;
      Buffer++;
    }
}

ULONG
KdbMainLoop(VOID)
{
  CHAR Command[256];

  for (;;)
    {
      DbgPrint("kdb:> ");

      KdbGetCommand(Command);

      switch (Command[0])
	{
	  /* Continue. */
	case 'c':
	  return(0);

	  /* Bug check the system */
	case 'p':
	  KeBugCheck(0);
	  break;	  
	}
    }
}

VOID
KdbInternalEnter(PKTRAP_FRAME Tf)
{
  __asm__ __volatile__ ("cli\n\t");
  (VOID)KdbMainLoop();
  __asm__ __volatile__("sti\n\t");
}

