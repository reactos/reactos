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
/* $Id: kdb.c,v 1.2 2001/04/12 00:56:04 dwelch Exp $
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

/* TYPES *********************************************************************/

/* GLOBALS *******************************************************************/

struct
{
  PCH Name;
  ULONG (*Fn)(PKTRAP_FRAME Tf, PCH Args);
} DebuggerCommands[] = {
  {"cont", DbgContCommand},
  {"regs", DbgRegsCommand},
  {"bugcheck", DbgBugCheckCommand},
  {NULL, NULL}
};

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
DbgContCommnad(PCH Args, PKTRAP_FRAME Tf)
{
  /* Not too difficult. */
  return(0);
}

ULONG
DbgRegsCommand(PCH Args, PKTRAP_FRAME Tf)
{
  DbgPrint("CS:EIP %.4x:%.8x, EAX %.8x EBX %.8x ECX %.8x EDX %.8x\n",
	   Tf->Cs, Tf->Eip, Tf->Eax, Tf->Ebx, Tf->Ecx, Tf->Edx);
  DbgPrint("ESI %.8x EDI %.8x EBP %.8x SS:ESP %.4x:%.8x\n",
	   Tf->Esi, Tf->Edi, Tf->Ebp, Tf->Ss, Tf->Esp);
  return(1);
}

ULONG
DbgBugCheckCommand(PCH Args, PKTRAP_FRAME Tf)
{
  KeBugCheck(1);
  return(1);
}

VOID
KdbDoCommand(PCH CommandLine, PKTRAP_FRAME Tf)
{
  ULONG i;
  PCH Next;
  PCH s;

  s = strpbrk(CommandLine, "\t ");
  if (s != NULL)
    {
      *s = 0;
      Next = s + 1;
    }
  else
    {
      Next = NULL;
    }

  for (i = 0; DebuggerCommands[i].Name != NULL; i++)
    {
      if (strcmp(DebuggerCommands[i], CommandLine) == 0)
	{
	  return(DebuggerCommands[i](Tf, Next));
	}
    }
  DbgPrint("Command '%s %s' is unknown\n", CommandLine, Next);
  return(1);
}

VOID
KdbMainLoop(PKTRAP_FRAME Tf)
{
  CHAR Command[256];
  ULONG s;

  do
    {
      DbgPrint("kdb:> ");

      KdbGetCommand(Command);

      s = KdbDoCommand(Command, Tf);    
    } while (s != 0);
}

VOID
KdbInternalEnter(PKTRAP_FRAME Tf)
{
  __asm__ __volatile__ ("cli\n\t");
  (VOID)KdbMainLoop(Tf);
  __asm__ __volatile__("sti\n\t");
}

