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
/* $Id: kdb.c,v 1.4 2001/04/22 14:47:00 chorns Exp $
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
#include <internal/ps.h>
#include <limits.h>
#include <ctype.h>
#include "kdb.h"

#define NDEBUG
#include <internal/debug.h>

/* TYPES *********************************************************************/

/* GLOBALS *******************************************************************/

ULONG 
DbgContCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf);
ULONG 
DbgRegsCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf);
ULONG 
DbgDRegsCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf);
ULONG 
DbgCRegsCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf);
ULONG 
DbgBugCheckCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf);
ULONG
DbgBackTraceCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf);
ULONG
DbgProcessListCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf);
ULONG
DbgProcessHelpCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf);

struct
{
  PCH Name;
  PCH Syntax;
  PCH Help;
  ULONG (*Fn)(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf);
} DebuggerCommands[] = {
  {"cont", "cont", "Exit the debugger", DbgContCommand},
  {"regs", "regs", "Display general purpose registers", DbgRegsCommand},
  {"dregs", "dregs", "Display debug registers", DbgDRegsCommand},
  {"cregs", "cregs", "Display control registers", DbgCRegsCommand},
  {"bugcheck", "bugcheck", "Bugcheck the system", DbgBugCheckCommand},
  {"bt", "bt [*frame-address]|[thread-id]","Do a backtrace", DbgBackTraceCommand},
  {"plist", "plist", "Display processes in the system", DbgProcessListCommand},
  {"help", "help", "Display help screen", DbgProcessHelpCommand},
  {NULL, NULL, NULL}
};

/* FUNCTIONS *****************************************************************/

/*
 * Convert a string to an unsigned long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
static unsigned long
strtoul(const char *nptr, char **endptr, int base)
{
  const char *s = nptr;
  unsigned long acc;
  int c;
  unsigned long cutoff;
  int neg = 0, any, cutlim;

  /*
   * See strtol for comments as to the logic used.
   */
  do {
    c = *s++;
  } while (isspace(c));
  if (c == '-')
  {
    neg = 1;
    c = *s++;
  }
  else if (c == '+')
    c = *s++;
  if ((base == 0 || base == 16) &&
      c == '0' && (*s == 'x' || *s == 'X'))
  {
    c = s[1];
    s += 2;
    base = 16;
  }
  if (base == 0)
    base = c == '0' ? 8 : 10;
  cutoff = (unsigned long)ULONG_MAX / (unsigned long)base;
  cutlim = (unsigned long)ULONG_MAX % (unsigned long)base;
  for (acc = 0, any = 0;; c = *s++)
  {
    if (isdigit(c))
      c -= '0';
    else if (isalpha(c))
      c -= isupper(c) ? 'A' - 10 : 'a' - 10;
    else
      break;
    if (c >= base)
      break;
    if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
      any = -1;
    else {
      any = 1;
      acc *= base;
      acc += c;
    }
  }
  if (any < 0)
  {
    acc = ULONG_MAX;
  }
  else if (neg)
    acc = -acc;
  if (endptr != 0)
    *endptr = any ? (char *)s - 1 : (char *)nptr;
  return acc;
}


char*
strpbrk(const char* s, const char* accept)
{
  int i;
  for (; (*s) != 0; s++)
    {
      for (i = 0; accept[i] != 0; i++)
	{
	  if (accept[i] == (*s))
	    {
	      return((char *)s);
	    }
	}
    }
  return(NULL);
}

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
DbgProcessHelpCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf)
{
  ULONG i, j, len;

  DbgPrint("Kernel debugger commands:\n");
  for (i = 0; DebuggerCommands[i].Name != NULL; i++)
    {
      DbgPrint("  %s", DebuggerCommands[i].Syntax);
      len = strlen(DebuggerCommands[i].Syntax);
      if (len < 35)
        {
          for (j = 0; j < 35 - len; j++)
          {
            DbgPrint(" ");
          }
        }
      DbgPrint(" - %s\n", DebuggerCommands[i].Help);
    }
  return(1);
}

ULONG
DbgProcessListCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf)
{
  extern LIST_ENTRY PsProcessListHead;
  PLIST_ENTRY current_entry;
  PEPROCESS current;
  ULONG i = 1;

  if (PsProcessListHead.Flink == NULL)
    {
      DbgPrint("No processes.\n");
      return(1);
    }

  DbgPrint("Process list: ");
  current_entry = PsProcessListHead.Flink;
  while (current_entry != &PsProcessListHead)
    {
      current = CONTAINING_RECORD(current_entry, EPROCESS, ProcessListEntry);
      DbgPrint("%d %.8s", current->UniqueProcessId, 
	       current->ImageFileName);
      i++;
      if ((i % 4) == 0)
	{
	  DbgPrint("\n");
	}
      current_entry = current_entry->Flink;
    }
  return(1);
}

VOID
DbgPrintBackTrace(PULONG Frame, ULONG StackBase, ULONG StackLimit)
{
  ULONG i = 1;

  DbgPrint("Frames:  ");
  while (Frame != NULL && (ULONG)Frame >= StackLimit && 
	 (ULONG)Frame < StackBase)
    {
      DbgPrint("%.8x  ", Frame[1]);
      Frame = (PULONG)Frame[0];
      i++;
    }

  if ((i % 10) != 0)
    {
      DbgPrint("\n");
    }
}

ULONG
DbgBackTraceCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf)
{
  ULONG StackBase, StackLimit;
  extern unsigned int init_stack, init_stack_top;

  /* Without an argument we print the current stack. */
  if (Argc == 1)
    {
      if (PsGetCurrentThread() != NULL)
	{
	  StackBase = (ULONG)PsGetCurrentThread()->Tcb.StackBase;
	  StackLimit = PsGetCurrentThread()->Tcb.StackLimit;
	}
      else
	{
	  StackBase = (ULONG)&init_stack_top;
	  StackLimit = (ULONG)&init_stack;
	}
      DbgPrintBackTrace((PULONG)Tf->Ebp, StackBase, StackLimit);
    }
  /* 
   * If there are two arguments and the second begins with a asterik treat it
   * as the address of a frame to start printing the back trace from.
   */
  else if (Argc == 2 && Argv[1][0] == '*')
    {
      PULONG Frame;
      Frame = (PULONG)strtoul(&Argv[1][1], NULL, 0);
      DbgPrintBackTrace(Frame, ULONG_MAX, 0);
    }
  /*
   * Otherwise treat the argument as the id of a thread whose back trace is to
   * be printed.
   */
  else
    {
    }
  return(1);
}

VOID
DbgPrintCr0(ULONG Cr0)
{
  ULONG i;

  DbgPrint("CR0:");
  if (Cr0 & (1 << 0))
    {
      DbgPrint(" PE");
    }
  if (Cr0 & (1 << 1))
    {
      DbgPrint(" MP");
    }
  if (Cr0 & (1 << 2))
    {
      DbgPrint(" EM");
    }
  if (Cr0 & (1 << 3))
    {
      DbgPrint(" TS");
    }
  if (!(Cr0 & (1 << 4)))
    {
      DbgPrint(" !BIT5");
    }
  if (Cr0 & (1 << 5))
    {
      DbgPrint(" NE");
    }
  for (i = 6; i < 16; i++)
    {
      if (Cr0 & (1 << i))
	{
	  DbgPrint(" BIT%d", i);
	}
    }
  if (Cr0 & (1 << 16))
    {
      DbgPrint(" WP");
    }
  if (Cr0 & (1 << 17))
    {
      DbgPrint(" BIT17");
    }
  if (Cr0 & (1 << 18))
    {
      DbgPrint(" AM");
    }
  for (i = 19; i < 29; i++)
    {
      if (Cr0 & (1 << i))
	{
	  DbgPrint(" BIT%d", i);
	}
    }
  if (Cr0 & (1 << 29))
    {
      DbgPrint(" NW");
    }
  if (Cr0 & (1 << 30))
    {
      DbgPrint(" CD");
    }
  if (Cr0 & (1 << 31))
    {
      DbgPrint(" PG");
    }
  DbgPrint("\n");
}

ULONG 
DbgCRegsCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf)
{
  ULONG Cr0, Cr1, Cr2, Cr3, Cr4;
  ULONG Ldtr;
  USHORT Tr;
  
  __asm__ __volatile__ ("movl %%cr0, %0\n\t" : "=d" (Cr0));
  /*  __asm__ __volatile__ ("movl %%cr1, %0\n\t" : "=d" (Cr1)); */
  Cr1 = 0;
  __asm__ __volatile__ ("movl %%cr2, %0\n\t" : "=d" (Cr2));
  __asm__ __volatile__ ("movl %%cr3, %0\n\t" : "=d" (Cr3));
  __asm__ __volatile__ ("movl %%cr4, %0\n\t" : "=d" (Cr4));
  __asm__ __volatile__ ("str %0\n\t" : "=d" (Tr));
  __asm__ __volatile__ ("sldt %0\n\t" : "=d" (Ldtr));
  DbgPrintCr0(Cr0);
  DbgPrint("CR1 %.8x CR2 %.8x CR3 %.8x CR4 %.8x TR %.8x LDTR %.8x\n",
	   Cr1, Cr2, Cr3, Cr4, (ULONG)Tf, Ldtr);
  return(1);
}

ULONG 
DbgDRegsCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf)
{
  DbgPrint("DR0 %.8x DR1 %.8x DR2 %.8x DR3 %.8x DR6 %.8x DR7 %.8x\n",
	   Tf->Dr0, Tf->Dr1, Tf->Dr2, Tf->Dr3, Tf->Dr6, Tf->Dr7);
  return(1);
}

ULONG
DbgContCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf)
{
  /* Not too difficult. */
  return(0);
}

VOID
DbgPrintEflags(ULONG Eflags)
{
  DbgPrint("EFLAGS:");
  if (Eflags & (1 << 0))
    {
      DbgPrint(" CF");
    }
  if (!(Eflags & (1 << 1)))
    {
      DbgPrint(" !BIT1");
    }
  if (Eflags & (1 << 2))
    {
      DbgPrint(" PF");
    }
  if (Eflags & (1 << 3))
    {
      DbgPrint(" BIT3");
    }
  if (Eflags & (1 << 4))
    {
      DbgPrint(" AF");
    }
  if (Eflags & (1 << 5))
    {
      DbgPrint(" BIT5");
    }
  if (Eflags & (1 << 6))
    {
      DbgPrint(" ZF");
    }
  if (Eflags & (1 << 7))
    {
      DbgPrint(" SF");
    }
  if (Eflags & (1 << 8))
    {
      DbgPrint(" TF");
    }
  if (Eflags & (1 << 9))
    {
      DbgPrint(" IF");
    }
  if (Eflags & (1 << 10))
    {
      DbgPrint(" DF");
    }
  if (Eflags & (1 << 11))
    {
      DbgPrint(" OF");
    }
  if ((Eflags & ((1 << 12) | (1 << 13))) == 0)
    {
      DbgPrint(" IOPL0");
    }
  else if ((Eflags & ((1 << 12) | (1 << 13))) == 1)
    {
      DbgPrint(" IOPL1");
    }
  else if ((Eflags & ((1 << 12) | (1 << 13))) == 2)
    {
      DbgPrint(" IOPL2");
    }
  else if ((Eflags & ((1 << 12) | (1 << 13))) == 3)
    {
      DbgPrint(" IOPL3");
    }
  if (Eflags & (1 << 14))
    {
      DbgPrint(" NT");
    }
  if (Eflags & (1 << 15))
    {
      DbgPrint(" BIT15");
    }
  if (Eflags & (1 << 16))
    {
      DbgPrint(" RF");
    }
  if (Eflags & (1 << 17))
    {
      DbgPrint(" VF");
    }
  if (Eflags & (1 << 18))
    {
      DbgPrint(" AC");
    }
  if (Eflags & (1 << 19))
    {
      DbgPrint(" VIF");
    }
  if (Eflags & (1 << 20))
    {
      DbgPrint(" VIP");
    }
  if (Eflags & (1 << 21))
    {
      DbgPrint(" ID");
    }
  if (Eflags & (1 << 22))
    {
      DbgPrint(" BIT22");
    }
  if (Eflags & (1 << 23))
    {
      DbgPrint(" BIT23");
    }
  if (Eflags & (1 << 24))
    {
      DbgPrint(" BIT24");
    }
  if (Eflags & (1 << 25))
    {
      DbgPrint(" BIT25");
    }
  if (Eflags & (1 << 26))
    {
      DbgPrint(" BIT26");
    }
  if (Eflags & (1 << 27))
    {
      DbgPrint(" BIT27");
    }
  if (Eflags & (1 << 28))
    {
      DbgPrint(" BIT28");
    }
  if (Eflags & (1 << 29))
    {
      DbgPrint(" BIT29");
    }
  if (Eflags & (1 << 30))
    {
      DbgPrint(" BIT30");
    }
  if (Eflags & (1 << 31))
    {
      DbgPrint(" BIT31");
    }
  DbgPrint("\n");
}

ULONG
DbgRegsCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf)
{
  DbgPrint("CS:EIP %.4x:%.8x, EAX %.8x EBX %.8x ECX %.8x EDX %.8x\n",
	   Tf->Cs & 0xFFFF, Tf->Eip, Tf->Eax, Tf->Ebx, Tf->Ecx, Tf->Edx);
  DbgPrint("ESI %.8x EDI %.8x EBP %.8x SS:ESP %.4x:%.8x\n",
	   Tf->Esi, Tf->Edi, Tf->Ebp, Tf->Ss & 0xFFFF, Tf->Esp);
  DbgPrintEflags(Tf->Eflags);
  return(1);
}

ULONG
DbgBugCheckCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf)
{
  KeBugCheck(1);
  return(1);
}

ULONG
KdbDoCommand(PCH CommandLine, PKTRAP_FRAME Tf)
{
  ULONG i;
  PCH s1;
  PCH s;
  static PCH Argv[256];
  ULONG Argc;
  static CHAR OrigCommand[256];

  strcpy(OrigCommand, CommandLine);

  Argc = 0;
  s = CommandLine;
  while ((s1 = strpbrk(s, "\t ")) != NULL)
    {
      Argv[Argc] = s;
      *s1 = 0;
      s = s1 + 1;
      Argc++;
    }
  Argv[Argc] = s;
  Argc++;

  for (i = 0; DebuggerCommands[i].Name != NULL; i++)
    {
      if (strcmp(DebuggerCommands[i].Name, Argv[0]) == 0)
	{
	  return(DebuggerCommands[i].Fn(Argc, Argv, Tf));
	}
    }
  DbgPrint("Command '%s' is unknown.", OrigCommand);
  return(1);
}

VOID
KdbMainLoop(PKTRAP_FRAME Tf)
{
  CHAR Command[256];
  ULONG s;

  DbgPrint("\nEntered kernel debugger (type \"help\" for a list of commands)\n");
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

