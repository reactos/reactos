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
/* $Id: kdb.c,v 1.22.2.1 2004/06/03 22:29:31 arty Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/dbg/kdb.c
 * PURPOSE:         Kernel debugger
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 01/03/01
 */

/* INCLUDES ******************************************************************/

#include <internal/ctype.h>
#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/ps.h>
#include <limits.h>
#include <internal/kd.h>
#include <internal/mm.h>
#include <internal/i386/mm.h>
#include "kdb.h"
#include "kjs.h"

#define NDEBUG
#include <internal/debug.h>

/* TYPES *********************************************************************/

/* GLOBALS *******************************************************************/

#define BS 8
#define DEL 127

BOOL KbdEchoOn = TRUE;

typedef struct
{
  BOOLEAN Enabled;
  BOOLEAN Temporary;
  BOOLEAN Assigned;
  ULONG Address;
  UCHAR SavedInst;
} KDB_ACTIVE_BREAKPOINT;

#define KDB_MAXIMUM_BREAKPOINT_COUNT (255)
#define KDB_STACK_SIZE (PAGE_SIZE * 16)

static PVOID KdbStack = NULL; /* stack which the main loop is running on */
static BOOL KdbResumeJavaScript = FALSE;
static jmp_buf KdbMainLoopJump; /* points to the end of the main loop */
static jmp_buf KdbJavaScriptJump; /* points to the last java-script state */
static volatile PKTRAP_FRAME KdbTrapFrame = NULL; /* trap frame */

static PCHAR KjsInitScript = NULL; /* \SystemRoot\kjsinit.js */

static ULONG KdbBreakPointCount = 0;
static KDB_ACTIVE_BREAKPOINT KdbActiveBreakPoints[KDB_MAXIMUM_BREAKPOINT_COUNT];

static BOOLEAN KdbIgnoreNextSingleStep = FALSE;

static ULONG KdbLastSingleStepFrom = 0xFFFFFFFF;
static BOOLEAN KdbEnteredOnSingleStep = FALSE;

ULONG KdbEntryCount = 0;

int isalpha( int );
VOID PsDumpThreads(BOOLEAN System);

STATIC LONG KdbDoCommand(PCH CommandLine);
STATIC LONG DbgContCommand(ULONG Argc, PCH Argv[]);
STATIC LONG DbgEchoToggle(ULONG Argc, PCH Argv[]);
STATIC LONG DbgRegsCommand(ULONG Argc, PCH Argv[]);
STATIC LONG DbgDRegsCommand(ULONG Argc, PCH Argv[]);
STATIC LONG DbgCRegsCommand(ULONG Argc, PCH Argv[]);
STATIC LONG DbgBugCheckCommand(ULONG Argc, PCH Argv[]);
STATIC LONG DbgBackTraceCommand(ULONG Argc, PCH Argv[]);
STATIC LONG DbgAddrCommand(ULONG Argc, PCH Argv[]);
STATIC LONG DbgXCommand(ULONG Argc, PCH Argv[]);
STATIC LONG DbgWCommand(ULONG Argc, PCH Argv[]);
STATIC LONG DbgScriptCommand(ULONG Argc, PCH Argv[]);
STATIC LONG DbgThreadListCommand(ULONG Argc, PCH Argv[]);
STATIC LONG DbgProcessListCommand(ULONG Argc, PCH Argv[]);
STATIC LONG DbgProcessHelpCommand(ULONG Argc, PCH Argv[]);
STATIC LONG DbgShowFilesCommand(ULONG Argc, PCH Argv[]);
STATIC LONG DbgEnableFileCommand(ULONG Argc, PCH Argv[]);
STATIC LONG DbgDisableFileCommand(ULONG Argc, PCH Argv[]);
STATIC LONG DbgDisassemble(ULONG Argc, PCH Argv[]);
STATIC LONG DbgSetBreakPoint(ULONG Argc, PCH Argv[]);
STATIC LONG DbgDeleteBreakPoint(ULONG Argc, PCH Argv[]);
STATIC LONG DbgSetMemoryBreakPoint(ULONG Argc, PCH Argv[]);
STATIC LONG DbgStep(ULONG Argc, PCH Argv[]);
STATIC LONG DbgStepOver(ULONG Argc, PCH Argv[]);
STATIC LONG DbgFinish(ULONG Argc, PCH Argv[]);

struct
{
  PCH Name;
  PCH Syntax;
  PCH Help;
  LONG (*Fn)(ULONG Argc, PCH Argv[]);
} DebuggerCommands[] = {
  {"cont",     "cont", "Exit the debugger", DbgContCommand},
  {"echo",     "echo", "Toggle serial echo", DbgEchoToggle},
  {"regs",     "regs", "Display general purpose registers", DbgRegsCommand},
  {"dregs",    "dregs", "Display debug registers", DbgDRegsCommand},
  {"cregs",    "cregs", "Display control registers", DbgCRegsCommand},
  {"bugcheck", "bugcheck", "Bugcheck the system", DbgBugCheckCommand},
  {"bt",       "bt [*frame-address]|[thread-id]","Do a backtrace", DbgBackTraceCommand},
  {"addr",     "addr <address>", "Displays symbol info", DbgAddrCommand},
  {"x",        "x <addr> <words>", "Displays <addr> for <words>", DbgXCommand},
  {"w",        "w <size> <value> <addr>", "Writes <value> into memory at <addr>"
               " (<size> bytes)", DbgWCommand},
  {"plist",    "plist", "Display processes in the system", DbgProcessListCommand},
  {"tlist",    "tlist [sys]", "Display threads in the system", DbgThreadListCommand},
  {"sfiles",   "sfiles", "Show files that print debug prints", DbgShowFilesCommand},
  {"efile",    "efile <filename>", "Enable debug prints from file", DbgEnableFileCommand},
  {"dfile",    "dfile <filename>", "Disable debug prints from file", DbgDisableFileCommand},
  {"js",       "js", "Script mode", DbgScriptCommand},
  {"disasm",   "disasm <address>", "Disassembles 10 instructions at <address> or eip",
               DbgDisassemble},
  {"bp",       "bp <address>", "Sets an int3 breakpoint at a given address", DbgSetBreakPoint},
  {"bc",       "bc <breakpoint number>", "Deletes a breakpoint", DbgDeleteBreakPoint},
  {"ba",       "ba <debug register> <access type> <length> <address>", 
   "Sets a breakpoint using a debug register", DbgSetMemoryBreakPoint},
  {"t",        "t", "Steps forward a single instructions", DbgStep},
  {"p",        "p", "Steps forward a single instructions skipping calls", DbgStepOver},
  {"finish",   "finish", "Runs until the current function exits", DbgFinish},
  {"help",     "help", "Display help screen", DbgProcessHelpCommand},
  {NULL, NULL, NULL}
};

volatile DWORD x_dr0 = 0, x_dr1 = 0, x_dr2 = 0, x_dr3 = 0, x_dr7 = 0;

extern LONG KdbDisassemble(ULONG Address);
extern LONG KdbGetInstLength(ULONG Address);
extern NTSTATUS MmGetPageEntry2(PVOID PAddress, PULONG* Pte, BOOLEAN MayWait);

/****************************************************************************/
/* HELPER FUNCTIONS                                                         */
/****************************************************************************/

/*
 * Convert a string to an unsigned long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
unsigned long
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


STATIC char*
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

/* Reads a command (line) from the keyboard or serial port into Buffer */
STATIC VOID
KdbGetCommand(PCH Buffer)
{
  CHAR Key;
  PCH Orig = Buffer;
  static UCHAR LastCommand[256] = "";
  static CHAR LastKey = 0;
  ULONG ScanCode = 0;
  
  KbdEchoOn = !((KdDebugState & KD_DEBUG_KDNOECHO) != 0);
  
  for (;;)
    {
      if (KdDebugState & KD_DEBUG_KDSERIAL)
	while ((Key = KdbTryGetCharSerial()) == -1);
      else
	while ((Key = KdbTryGetCharKeyboard(&ScanCode)) == -1);

      if (Key == '\n' && LastKey == '\r')
        {
	  /* ignore this key */
        }
      else if (Key == '\r' || Key == '\n')
	{
	  DbgPrint("\n");
	  /*
	    Repeat the last command if the user presses enter. Reduces the
	    risk of RSI when single-stepping.
	  */
	  if (Buffer == Orig)
	    {
	      strcpy(Buffer, LastCommand);
	    }
	  else
	    {
	      *Buffer = 0;
	      strcpy(LastCommand, Orig);
	    }
          break;
	}
      else if (Key == BS || Key == DEL)
        {
          if (Buffer > Orig)
            {
              Buffer--;
              *Buffer = 0;
	      if (KbdEchoOn)
		DbgPrint("%c %c", BS, BS);
	      else
		DbgPrint(" %c", BS);
	    }
        }
      else if (ScanCode == 72)
	{
	  ULONG i;
	  while (Buffer > Orig)
	    {
	      Buffer--;
	      *Buffer = 0;
	      if (KbdEchoOn)
		DbgPrint("%c %c", BS, BS);
	      else
		DbgPrint(" %c", BS);
	    }
	  for (i = 0; LastCommand[i] != 0; i++)
	    {
	      if (KbdEchoOn)
		DbgPrint("%c", LastCommand[i]);
	      *Buffer = LastCommand[i];
	      Buffer++;
	    }
	}
      else
        {
	  if (KbdEchoOn)
	    DbgPrint("%c", Key);

          *Buffer = Key;
          Buffer++;
        }
      LastKey = Key;
    }
    LastKey = Key;
}

/* Decodes an address of the form <module: offset> */
STATIC BOOLEAN
KdbDecodeAddress(PUCHAR Buffer, PULONG Address)
{
  while (isspace(*Buffer))
    {
      Buffer++;
    }
  if (Buffer[0] == '<')
    {
      PUCHAR ModuleName = Buffer + 1;
      PUCHAR AddressString = strpbrk(Buffer, ":");
      KDB_MODULE_INFO info;
      WCHAR ModuleNameW[256];
      ULONG i;

      if (AddressString == NULL)
	{
	  DbgPrint("Address %x is malformed.\n", Buffer);
	  return(FALSE);
	}
      *AddressString = 0;
      AddressString++;
      while (isspace(*AddressString))
	{
	  AddressString++;
	}

      for (i = 0; ModuleName[i] != 0 && !isspace(ModuleName[i]); i++)
	{
	  ModuleNameW[i] = (WCHAR)ModuleName[i];
	}
      ModuleNameW[i] = L'\0';

      /* Find the module. */
      if (!KdbFindModuleByName(ModuleNameW, &info))
	{
	  DbgPrint("Couldn't find module %ws.\n", ModuleNameW);
	  return FALSE;
	}
      *Address = (ULONG)info.Base;
      *Address += strtoul(AddressString, NULL, 16);
      return(TRUE);
    }
  else
    {
      *Address = strtoul(Buffer, NULL, 0);
      return(TRUE);
    }
}

/* Overwrites an instruction and flushed the TLB. */
STATIC NTSTATUS
KdbOverwriteInst(ULONG Address, PUCHAR PreviousInst, UCHAR NewInst)
{
  PULONG BreakPtePtr;
  ULONG SavedPte;
  NTSTATUS Status;
  /* Get the pte for the page containing the address. */
  Status =
    MmGetPageEntry2((PVOID)PAGE_ROUND_DOWN(Address), &BreakPtePtr, FALSE);
  /* Return if that page isn't present. */
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  if (!((*BreakPtePtr) & (1 << 0)))
    {
      return(STATUS_MEMORY_NOT_ALLOCATED);
    }
  /* Saved the old pte and enable write permissions. */
  SavedPte = *BreakPtePtr;
  (*BreakPtePtr) |= (1 << 1);
  /* Flush the TLB. */
  __asm__ __volatile__ ("movl %%cr3, %%eax\n\t"
			"movl %%eax, %%cr3\n\t" 
			: : : "memory", "eax");
  /* Copy the old instruction back to the caller. */
  if (PreviousInst != NULL)
    {
      Status = MmSafeCopyFromUser(PreviousInst, (PUCHAR)Address, 1);
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}
    }
  /* Copy the new instruction in its place. */
  Status = MmSafeCopyToUser((PUCHAR)Address, &NewInst, 1);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  /* Restore the old pte. */
  *BreakPtePtr = SavedPte;
  /* And flush the tlb again. */
  __asm__ __volatile__ ("movl %%cr3, %%eax\n\t"
			"movl %%eax, %%cr3\n\t" 
			: : : "memory", "eax");
  return(STATUS_SUCCESS);
}

/* Enables all breakpoints */
STATIC VOID
KdbRenableBreakPoints(VOID)
{
  ULONG i;
  for (i = 0; i < KDB_MAXIMUM_BREAKPOINT_COUNT; i++)
    {
      if (KdbActiveBreakPoints[i].Assigned &&
	  !KdbActiveBreakPoints[i].Enabled)
	{
	  KdbActiveBreakPoints[i].Enabled = TRUE;
	  (VOID)KdbOverwriteInst(KdbActiveBreakPoints[i].Address,
				 &KdbActiveBreakPoints[i].SavedInst, 
				 0xCC);
	}
    }
}

/* Tries to find a breakpoint at eip and returns it's number if found or -1 if
 * the breakpoint is not ours/none was found.
 */
STATIC LONG
KdbIsBreakPointOurs(PKTRAP_FRAME TrapFrame)
{
  ULONG i;
  for (i = 0; i < KDB_MAXIMUM_BREAKPOINT_COUNT; i++)
    {
      if (KdbActiveBreakPoints[i].Assigned &&
	  KdbActiveBreakPoints[i].Address == (TrapFrame->Eip - 1))
	{
	  return(i);
	}
    }
  return(-1);
}

/* Deletes a breakpoint */
STATIC VOID
KdbDeleteBreakPoint(ULONG BreakPointNr)
{
  KdbBreakPointCount--;
  KdbActiveBreakPoints[BreakPointNr].Assigned = FALSE;
}

/* Inserts a breakpoint */
STATIC NTSTATUS
KdbInsertBreakPoint(ULONG Address, BOOLEAN Temporary, DWORD *bpNumber)
{
  NTSTATUS Status;
  UCHAR SavedInst;
  ULONG i;  
  if (KdbBreakPointCount == KDB_MAXIMUM_BREAKPOINT_COUNT)
    {
      return(STATUS_UNSUCCESSFUL);
    }
  for (i = 0; i < KDB_MAXIMUM_BREAKPOINT_COUNT; i++)
    {
      if (!KdbActiveBreakPoints[i].Assigned)
	{
	  break;
	}
    }
  Status = KdbOverwriteInst(Address, &SavedInst, 0xCC);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  KdbActiveBreakPoints[i].Assigned = TRUE;
  KdbActiveBreakPoints[i].Enabled = TRUE;
  KdbActiveBreakPoints[i].Address = Address;
  KdbActiveBreakPoints[i].Temporary = Temporary;
  KdbActiveBreakPoints[i].SavedInst = SavedInst;
  if (bpNumber != NULL)
    *bpNumber = i;
  return(STATUS_SUCCESS);
}

/* Prints a stack backtrace */
STATIC VOID
DbgPrintBackTrace(PULONG Frame, ULONG StackBase, ULONG StackLimit)
{
  ULONG i = 1;

  DbgPrint("Frames:  ");
  while (Frame != NULL && (ULONG)Frame >= StackLimit && 
	 (ULONG)Frame < StackBase)
    {
      KdbPrintAddress((PVOID)Frame[1]);
      DbgPrint("\n");
      Frame = (PULONG)Frame[0];
      i++;
    }
}

/* Prints descriptions for each bit set in the given eflags reg. */
STATIC VOID
DbgPrintEflags(ULONG Eflags)
{
  DbgPrint("EFLAGS:");
  if (Eflags & (1 << 0))
    DbgPrint(" CF");
  if (!(Eflags & (1 << 1)))
    DbgPrint(" !BIT1");
  if (Eflags & (1 << 2))
    DbgPrint(" PF");
  if (Eflags & (1 << 3))
    DbgPrint(" BIT3");
  if (Eflags & (1 << 4))
    DbgPrint(" AF");
  if (Eflags & (1 << 5))
    DbgPrint(" BIT5");
  if (Eflags & (1 << 6))
    DbgPrint(" ZF");
  if (Eflags & (1 << 7))
    DbgPrint(" SF");
  if (Eflags & (1 << 8))
    DbgPrint(" TF");
  if (Eflags & (1 << 9))
    DbgPrint(" IF");
  if (Eflags & (1 << 10))
    DbgPrint(" DF");
  if (Eflags & (1 << 11))
    DbgPrint(" OF");
  if ((Eflags & ((1 << 12) | (1 << 13))) == 0)
    DbgPrint(" IOPL0");
  else if ((Eflags & ((1 << 12) | (1 << 13))) == 1)
    DbgPrint(" IOPL1");
  else if ((Eflags & ((1 << 12) | (1 << 13))) == 2)
    DbgPrint(" IOPL2");
  else if ((Eflags & ((1 << 12) | (1 << 13))) == 3)
    DbgPrint(" IOPL3");
  if (Eflags & (1 << 14))
    DbgPrint(" NT");
  if (Eflags & (1 << 15))
    DbgPrint(" BIT15");
  if (Eflags & (1 << 16))
    DbgPrint(" RF");
  if (Eflags & (1 << 17))
    DbgPrint(" VF");
  if (Eflags & (1 << 18))
    DbgPrint(" AC");
  if (Eflags & (1 << 19))
    DbgPrint(" VIF");
  if (Eflags & (1 << 20))
    DbgPrint(" VIP");
  if (Eflags & (1 << 21))
    DbgPrint(" ID");
  if (Eflags & (1 << 22))
    DbgPrint(" BIT22");
  if (Eflags & (1 << 23))
    DbgPrint(" BIT23");
  if (Eflags & (1 << 24))
    DbgPrint(" BIT24");
  if (Eflags & (1 << 25))
    DbgPrint(" BIT25");
  if (Eflags & (1 << 26))
    DbgPrint(" BIT26");
  if (Eflags & (1 << 27))
    DbgPrint(" BIT27");
  if (Eflags & (1 << 28))
    DbgPrint(" BIT28");
  if (Eflags & (1 << 29))
    DbgPrint(" BIT29");
  if (Eflags & (1 << 30))
    DbgPrint(" BIT30");
  if (Eflags & (1 << 31))
    DbgPrint(" BIT31");
  DbgPrint("\n");
}

/* Prints descriptions for each bit set in the given cr0 reg. */
STATIC VOID
DbgPrintCr0(ULONG Cr0)
{
  ULONG i;

  DbgPrint("CR0:");
  if (Cr0 & (1 << 0))
    DbgPrint(" PE");
  if (Cr0 & (1 << 1))
    DbgPrint(" MP");
  if (Cr0 & (1 << 2))
    DbgPrint(" EM");
  if (Cr0 & (1 << 3))
    DbgPrint(" TS");
  if (!(Cr0 & (1 << 4)))
    DbgPrint(" !BIT5");
  if (Cr0 & (1 << 5))
    DbgPrint(" NE");
  for (i = 6; i < 16; i++)
    {
      if (Cr0 & (1 << i))
        DbgPrint(" BIT%d", i);
    }
  if (Cr0 & (1 << 16))
    DbgPrint(" WP");
  if (Cr0 & (1 << 17))
    DbgPrint(" BIT17");
  if (Cr0 & (1 << 18))
    DbgPrint(" AM");
  for (i = 19; i < 29; i++)
    {
      if (Cr0 & (1 << i))
        DbgPrint(" BIT%d", i);
    }
  if (Cr0 & (1 << 29))
    DbgPrint(" NW");
  if (Cr0 & (1 << 30))
    DbgPrint(" CD");
  if (Cr0 & (1 << 31))
    DbgPrint(" PG");
  DbgPrint("\n");
}


/****************************************************************************/
/* JAVASCRIPT FUNCTIONS                                                     */
/****************************************************************************/

/* System.regread (key, value) => string, integer
 * key is a string describing a registry key, value the name of the value to
 * be read.
 * Returns the value of the key as string.
 * On error an integer (NTSTATUS) is returned.
 */
static int
KjsReadRegValue(void *context, JSNode *result, JSNode *args)
{
  PCHAR cp;
  PVOID *context_list = context;
  PKJS kjs = (PKJS)context_list[0];
  JSVirtualMachine *vm = kjs->vm;
  NTSTATUS Status;
  RTL_QUERY_REGISTRY_TABLE QueryTable[2] = { { 0 } };
  UNICODE_STRING NameString;
  UNICODE_STRING PathString;
  UNICODE_STRING DefaultString;
  UNICODE_STRING ValueResult;
  ANSI_STRING AnsiResult;
  
  if (args->u.vinteger != 2)
    {
      strcpy(vm->error, "System.regread(): illegal amount of arguments");
      js_vm_error(vm);
      return 0; /* NOTREACHED */
    }
  if (args[1].type != JS_STRING || args[2].type != JS_STRING)
    {
      strcpy(vm->error, "System.regread(): illegal argument type");
      js_vm_error(vm);
      return 0; /* NOTREACHED */
    }

  RtlInitUnicodeString(&PathString,NULL);
  RtlInitUnicodeString(&NameString,NULL);
  
  cp = js_string_to_c_string (vm, &args[1]);
  RtlCreateUnicodeStringFromAsciiz(&PathString,cp);
  js_free(cp);
  cp = js_string_to_c_string (vm, &args[2]);
  RtlCreateUnicodeStringFromAsciiz(&NameString,cp);
  js_free(cp);
  
  RtlInitUnicodeString(&ValueResult,NULL);
  RtlInitUnicodeString(&DefaultString,L"");
  RtlInitAnsiString(&AnsiResult,NULL);

  QueryTable->EntryContext = 0;
  QueryTable->Flags = RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_DIRECT;
  QueryTable->Name = NameString.Buffer;
  QueryTable->DefaultType = REG_SZ;
  QueryTable->DefaultData = &DefaultString;
  QueryTable->EntryContext = &ValueResult;
  Status = RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE, 
				   PathString.Buffer, 
				   QueryTable, 
				   NULL, 
				   NULL );
  
  RtlFreeUnicodeString(&NameString);
  RtlFreeUnicodeString(&PathString);
  
  if (NT_SUCCESS(Status)) {
    RtlInitAnsiString(&AnsiResult,NULL);
    RtlUnicodeStringToAnsiString(&AnsiResult,
				 &ValueResult,
				 TRUE);
    js_vm_make_string (vm, result, AnsiResult.Buffer, 
		       strlen(AnsiResult.Buffer));
    RtlFreeAnsiString(&AnsiResult);
  } else {
    result->type = JS_INTEGER;
    result->u.vinteger = Status;
  }

  return JS_PROPERTY_FOUND;
}

/* System.regs(int) => integer
 * Returns the value of the int'th reg in the trap frame.
 */
static int
KjsGetRegister(void *context, JSNode *result, JSNode *args)
{
  PVOID *context_list = context;
  PKJS kjs = (PKJS)context_list[0];
  JSVirtualMachine *vm = kjs->vm;
  DWORD Result;
  
  if (args->u.vinteger != 1)
    {
      strcpy(vm->error, "System.regs(): illegal amount of arguments");
      js_vm_error(vm);
      return 0; /* NOTREACHED */
    }
  if (args[1].type != JS_INTEGER)
    {
      strcpy(vm->error, "System.regs(): illegal argument type");
      js_vm_error(vm);
      return 0; /* NOTREACHED */
    }

  Result = ((DWORD *)KdbTrapFrame)[args[1].u.vinteger];
  result->type = JS_INTEGER;
  result->u.vinteger = Result;

  return JS_PROPERTY_FOUND;
}

/* System.getmodule(int) => array(module_name, base, size) or undefined
 * Returns an array (name, base, size, (sym_base, sym_len, str_base, str_len)) 
 * filled with information of the int'th module or undefined if int is out of
 * range.  If there is no debug info, the debug info array is empty.
 */
static int
KjsGetNthModule(void *context, JSNode *result, JSNode *args)
{
  PVOID *context_list = context;
  PKJS kjs = (PKJS)context_list[0];
  JSVirtualMachine *vm = kjs->vm;
  KDB_MODULE_INFO info;
  UNICODE_STRING UnicodeNameString;
  ANSI_STRING AnsiNameString;
  
  if (args->u.vinteger != 1)
    {
      strcpy(vm->error, "System.getmodule(): illegal amount of arguments");
      js_vm_error(vm);
      return 0; /* NOTREACHED */
    }
  if (args[1].type != JS_INTEGER)
    {
      strcpy(vm->error, "System.getmodule(): illegal argument type");
      js_vm_error(vm);
      return 0; /* NOTREACHED */
    }

  if (!KdbFindModuleByIndex(args[1].u.vinteger, &info))
    {
      result->type = JS_UNDEFINED;
      return JS_PROPERTY_FOUND;
    }

  RtlInitUnicodeString(&UnicodeNameString, info.Name);
  RtlUnicodeStringToAnsiString(&AnsiNameString,
                               &UnicodeNameString,
                               TRUE);

  /* If we have symbol info, follow the module info */
  js_vm_make_array(vm, result, 4);

  js_vm_make_string(vm,
                    &result->u.varray->data[0],
                    AnsiNameString.Buffer,
                    AnsiNameString.Length);

  RtlFreeAnsiString(&AnsiNameString);

  result->u.varray->data[1].type = JS_INTEGER;
  result->u.varray->data[1].u.vinteger = (DWORD)info.Base;
  result->u.varray->data[2].type = JS_INTEGER;
  result->u.varray->data[2].u.vinteger = (DWORD)info.Size;
  if( info.SymbolInfo ) {
      JSNode *dbgnode = &result->u.varray->data[3];
      js_vm_make_array(vm, dbgnode, 4);
      dbgnode->u.varray->data[0].type = JS_INTEGER;
      dbgnode->u.varray->data[0].u.vinteger = 
	  (DWORD)info.SymbolInfo->SymbolsBase;
      dbgnode->u.varray->data[1].type = JS_INTEGER;
      dbgnode->u.varray->data[1].u.vinteger = 
	  (DWORD)info.SymbolInfo->SymbolsLength;
      dbgnode->u.varray->data[2].type = JS_INTEGER;
      dbgnode->u.varray->data[2].u.vinteger = 
	  (DWORD)info.SymbolInfo->SymbolStringsBase;
      dbgnode->u.varray->data[3].type = JS_INTEGER;
      dbgnode->u.varray->data[3].u.vinteger = 
	  (DWORD)info.SymbolInfo->SymbolStringsLength;
      dbgnode->type = JS_ARRAY;
  } else {
      js_vm_make_array(vm, &result->u.varray->data[3], 0);
      result->u.varray->data[3].type = JS_ARRAY;
  }

  result->type = JS_ARRAY;

  return JS_PROPERTY_FOUND;
}

/* System.dbgcommand (string) => integer
 * Executes debugger command "string".
 * If there was an error executing the command, -1 is returned.
 * Else a positive value is returned. If the command which is called
 * continues execution of the debugged program the value returned will
 * be the exception number on which the debugger was entered.
 */
static int
KjsDebuggerCommand(void *context, JSNode *result, JSNode *args)
{
  PVOID *context_list = context;
  PKJS kjs = (PKJS)context_list[0];
  JSVirtualMachine *vm = kjs->vm;
  PCHAR cmd;
  LONG res;

  if (args->u.vinteger != 1)
    {
      strcpy(vm->error, "System.dbgcommand(): illegal amount of arguments");
      js_vm_error(vm);
      return 0; /* NOTREACHED */
    }
  if (args[1].type != JS_STRING)
    {
      strcpy(vm->error, "System.dbgcommand(): illegal argument type");
      js_vm_error(vm);
      return 0; /* NOTREACHED */
    }

  cmd = js_string_to_c_string(vm, &args[1]);
  if (strcmp(cmd, "js") == 0)
    res = -1;
  else
    res = KdbDoCommand(cmd);
  js_free(cmd);
  
  if (res == 0) /* suspend execution */
    {
      /* if setjmp does not return 0 (excution is resumed)
         we return the execption number on which the debugger
         was entered */
      if ((res = setjmp(KdbJavaScriptJump)) == 0)
        {
          /* interrupt execution */
          KdbResumeJavaScript = TRUE;
          longjmp(KdbMainLoopJump, 1);
        }
      else
        res--;
    }
  
  result->type = JS_INTEGER;
  result->u.vinteger = res;
  return JS_PROPERTY_FOUND;
}

/* System.getcommand() => string
 * Reads a command from standard input.
 */
static int
KjsGetCommand(void *context, JSNode *result, JSNode *args)
{
  PVOID *context_list = context;
  PKJS kjs = (PKJS)context_list[0];
  JSVirtualMachine *vm = kjs->vm;
  CHAR cmd[1024];
  PCHAR prompt = NULL;

  if (args->u.vinteger > 1)
    {
      strcpy(vm->error, "System.getcommand(): illegal amount of arguments");
      js_vm_error(vm);
      return 0; /* NOTREACHED */
    }
  if (args->u.vinteger == 1 && args[1].type != JS_STRING)
    {
      strcpy(vm->error, "System.getcommand(): illegal argument type");
      js_vm_error(vm);
      return 0; /* NOTREACHED */
    }

  if (args->u.vinteger == 1)
    {
      prompt = js_string_to_c_string(vm, &args[1]);
      DbgPrint(prompt);
      js_free(prompt);
    }

  KdbGetCommand(cmd);
  js_vm_make_string(vm, result, cmd, strlen(cmd));

  return JS_PROPERTY_FOUND;
}

/* System.mread(s, addr) => integer, string
 * Reads memory content. If s is 1 a BYTE is read,
 * if it's 2 a WORD will be read, 4 will read a DWORD
 * and 0 will read a 0-terminated string.
 */
static int
KjsReadMemory(void *context, JSNode *result, JSNode *args)
{
  PVOID *context_list = context;
  PKJS kjs = (PKJS)context_list[0];
  JSVirtualMachine *vm = kjs->vm;
  NTSTATUS Status;

  if (args->u.vinteger != 2)
    goto argument_error;
  if (args[1].type != JS_INTEGER || args[2].type != JS_INTEGER)
    goto argument_type_error;
      
  if (args[1].u.vinteger == 0) /* String type */
    {
      CHAR buf[4096];
      PCHAR p = (PCHAR)args[2].u.vinteger;
      INT i;
      
      for (i = 0; i < 4095; i++, p++)
        {
	  Status = MmSafeCopyFromUser(buf+i, p, sizeof (CHAR));
	  if (!NT_SUCCESS(Status))
            {
              result->type = JS_UNDEFINED;
              return JS_PROPERTY_FOUND;
	    }
	  if (buf[i] == '\0')
	    break;
        }
      buf[i] = '\0';
      
      result->type = JS_STRING;
      js_vm_make_string(vm, result, (char *)buf, i);
    }
  else if (args[1].u.vinteger == 1) /* Byte */
    {
      BYTE b;
      Status = MmSafeCopyFromUser(&b, (PVOID)args[2].u.vinteger, sizeof (BYTE));
      if (!NT_SUCCESS(Status))
        {
	  result->type = JS_UNDEFINED;
	  return JS_PROPERTY_FOUND;
	}
      result->type = JS_INTEGER;
      result->u.vinteger = (int)b;
    }
  else if (args[1].u.vinteger == 2) /* Word */
    {
      WORD w;
      Status = MmSafeCopyFromUser(&w, (PVOID)args[2].u.vinteger, sizeof (WORD));
      if (!NT_SUCCESS(Status))
        {
	  result->type = JS_UNDEFINED;
	  return JS_PROPERTY_FOUND;
	}
      result->type = JS_INTEGER;
      result->u.vinteger = (int)w;
    }
  else if (args[1].u.vinteger == 4) /* Dword */
    {
      DWORD dw;
      Status = MmSafeCopyFromUser(&dw, (PVOID)args[2].u.vinteger, sizeof (DWORD));
      if (!NT_SUCCESS(Status))
        {
	  result->type = JS_UNDEFINED;
	  return JS_PROPERTY_FOUND;
	}
      result->type = JS_INTEGER;
      result->u.vinteger = (int)dw;
    }
  else
    result->type = JS_UNDEFINED;
    
  return JS_PROPERTY_FOUND;

  /* error handling */

  argument_error:
    strcpy(vm->error, "System.mread(): illegal amount of arguments");
    js_vm_error (vm);

  argument_type_error:
    strcpy(vm->error, "System.mread(): illegal argument type");
    js_vm_error (vm);

  /* NOTREACHED */
  return 0;
}

/* System.mwrite(s, addr, value) => boolean
 * Writes value into memory. If s is 1 a BYTE is written,
 * if it's 2 a WORD will be written, 4 will write a DWORD
 * and 0 will write a 0-terminated string.
 */
static int
KjsWriteMemory(void *context, JSNode *result, JSNode *args)
{
  PVOID *context_list = context;
  PKJS kjs = (PKJS)context_list[0];
  JSVirtualMachine *vm = kjs->vm;
  NTSTATUS Status;

  if (args->u.vinteger != 3)
    goto argument_error;
  if (args[1].type != JS_INTEGER || args[2].type != JS_INTEGER)
    goto argument_type_error;
  if ((args[1].u.vinteger == 0 && args[3].type != JS_STRING) ||
      (args[1].u.vinteger != 0 && args[3].type != JS_INTEGER))
    goto argument_type_error;
  
  if (args[1].u.vinteger == 0) /* String type */
    {
      PCHAR p, str;
      INT i;
      
      p = str = js_string_to_c_string(vm, &args[3]);
      result->type = JS_BOOLEAN;

      for (i = 0;; i++, p++)
        {
	  Status = MmSafeCopyToUser((PVOID)(args[2].u.vinteger+i), p, sizeof (CHAR));
	  if (!NT_SUCCESS(Status))
            {
              js_free(str);
              result->u.vboolean = FALSE;
              return JS_PROPERTY_FOUND;
	    }
	  if (*p == '\0')
	    break;
        }
      
      js_free(str);
      result->u.vboolean = TRUE;
    }
  else if (args[1].u.vinteger == 1) /* Byte */
    {
      BYTE b = (BYTE)(args[3].u.vinteger & 0xff);
      result->type = JS_BOOLEAN;
      Status = MmSafeCopyToUser((PVOID)args[2].u.vinteger, &b, sizeof (BYTE));
      if (!NT_SUCCESS(Status))
        {
          result->u.vboolean = FALSE;
	  return JS_PROPERTY_FOUND;
	}
      result->u.vboolean = TRUE;
    }
  else if (args[1].u.vinteger == 2) /* Word */
    {
      WORD w = (WORD)(args[3].u.vinteger & 0xffff);
      result->type = JS_BOOLEAN;
      Status = MmSafeCopyToUser((PVOID)args[2].u.vinteger, &w, sizeof (WORD));
      if (!NT_SUCCESS(Status))
        {
          result->u.vboolean = FALSE;
	  return JS_PROPERTY_FOUND;
	}
      result->u.vboolean = TRUE;
    }
  else if (args[1].u.vinteger == 4) /* Dword */
    {
      DWORD dw = (DWORD)(args[3].u.vinteger);
      result->type = JS_BOOLEAN;
      Status = MmSafeCopyToUser((PVOID)args[2].u.vinteger, &dw, sizeof (DWORD));
      if (!NT_SUCCESS(Status))
        {
          result->u.vboolean = FALSE;
	  return JS_PROPERTY_FOUND;
	}
      result->u.vboolean = TRUE;
    }
  else
    result->type = JS_UNDEFINED;
    
  return JS_PROPERTY_FOUND;

  /* error handling */

  argument_error:
    strcpy(vm->error, "System.mwrite(): illegal amount of arguments");
    js_vm_error (vm);

  argument_type_error:
    strcpy(vm->error, "System.mwrite(): illegal argument type");
    js_vm_error (vm);

  /* NOTREACHED */
  return 0;
}

/* System.getinstlen(addr) => integer
 * Gets the length of the instruction at addr.
 */
static int
KjsGetInstLength(void *context, JSNode *result, JSNode *args)
{
  PVOID *context_list = context;
  PKJS kjs = (PKJS)context_list[0];
  JSVirtualMachine *vm = kjs->vm;
  unsigned int addr;
  
  if (args->u.vinteger != 1)
    {
      strcpy(vm->error, "System.getinstlen(): illegal amount of arguments");
      js_vm_error(vm);
      return 0; /* NOTREACHED */
    }
  if (args[1].type != JS_INTEGER)
    {
      strcpy(vm->error, "System.getinstlen(): illegal argument type");
      js_vm_error(vm);
      return 0; /* NOTREACHED */
    }
  
  addr = (unsigned int)(args[1].u.vinteger);
  result->type = JS_INTEGER;
  result->u.vinteger = (int)KdbGetInstLength(addr);
  
  return JS_PROPERTY_FOUND;
}

/* System.getsymbol(addr) => array(module_name, offset[, file, line, func])
 *                           or undefined
 * Gets symbol information for addr.
 */
static int
KjsGetSymbol(void *context, JSNode *result, JSNode *args)
{
  PVOID *context_list = context;
  PKJS kjs = (PKJS)context_list[0];
  JSVirtualMachine *vm = kjs->vm;
  KDB_MODULE_INFO info;
  ULONG_PTR RelativeAddress;
  NTSTATUS Status;
  UNICODE_STRING UnicodeNameString;
  ANSI_STRING AnsiNameString;
  ULONG LineNumber;
  CHAR FileName[256];
  CHAR FunctionName[256];
  PVOID address;

  if (args->u.vinteger != 1)
    {
      strcpy(vm->error, "System.getsymbol(): illegal amount of arguments");
      js_vm_error(vm);
      return 0; /* NOTREACHED */
    }
  if (args[1].type != JS_INTEGER)
    {
      strcpy(vm->error, "System.getsymbol(): illegal argument type");
      js_vm_error(vm);
      return 0; /* NOTREACHED */
    }
  
  address = (PVOID)args[1].u.vinteger;

  if (!KdbFindModuleByAddress(address, &info))
    {
      result->type = JS_UNDEFINED;     
      return JS_PROPERTY_FOUND;
    }

  RtlInitUnicodeString(&UnicodeNameString, info.Name);
  RtlUnicodeStringToAnsiString(&AnsiNameString, &UnicodeNameString, TRUE);

  /* get symbol info */
  RelativeAddress = (ULONG_PTR) address - info.Base;
  Status = LdrGetAddressInformation(info.SymbolInfo,
                                    RelativeAddress,
                                    &LineNumber,
                                    FileName,
                                    FunctionName);

  if (NT_SUCCESS(Status))
    {
      js_vm_make_array(vm, result, 5);

      js_vm_make_string(vm,
                        &result->u.varray->data[2],
                        FileName,
                        strlen(FileName));

      result->u.varray->data[3].type = JS_INTEGER;
      result->u.varray->data[3].u.vinteger = (DWORD)LineNumber;

      js_vm_make_string(vm,
                        &result->u.varray->data[4],
                        FunctionName,
                        strlen(FunctionName));
    }
  else
    {
      js_vm_make_array(vm, result, 2);
    }

  js_vm_make_string(vm, 
                    &result->u.varray->data[0], 
                    AnsiNameString.Buffer,
                    AnsiNameString.Length);
  result->u.varray->data[1].type = JS_INTEGER;
  result->u.varray->data[1].u.vinteger = (int)RelativeAddress;
  result->type = JS_ARRAY;
  RtlFreeAnsiString(&AnsiNameString);

  return JS_PROPERTY_FOUND;
}


static BOOL
FindJSEndMark(PCHAR Buffer)
{
  int i;

  for (i = 0; Buffer[i] && Buffer[i+1]; i++)
    {
      if (Buffer[i] == ';' && Buffer[i+1] == ';')
        return TRUE;
    }
  return FALSE;
}

/****************************************************************************/
/* DEBUGGER COMMANDS                                                        */
/****************************************************************************/

/* Sets an int3 breakpoint at the given address */
STATIC LONG
DbgSetBreakPoint(ULONG Argc, PCH Argv[])
{
  ULONG Addr;
  NTSTATUS Status;
  ULONG i;
  DWORD bpNum;
  
  if (Argc < 2)
    {
      DbgPrint("Need an address to set the breakpoint at.\n");
      return(-1);
    }
  /* Stitch the remaining arguments back into a single string. */
  for (i = 2; i < Argc; i++)
    {
      Argv[i][-1] = ' ';
    }
  if (!KdbDecodeAddress(Argv[1], &Addr))
    {
      return(-1);
    }
  DbgPrint("Setting breakpoint at 0x%X\n", Addr);
  if (!NT_SUCCESS(Status = KdbInsertBreakPoint(Addr, FALSE, &bpNum)))
    {
      DbgPrint("Failed to set breakpoint (Status %X)\n", Status);
      return(-1);
    }
  DbgPrint("Set breakpoint #%d\n", bpNum+1);
  return(1+bpNum);
}

/* Deletes a breakpoint */
STATIC LONG
DbgDeleteBreakPoint(ULONG Argc, PCH Argv[])
{
  ULONG BreakPointNr;
  if (Argc != 2)
    {
      DbgPrint("Need a breakpoint number to delete.\n");
      return(-1);
    }  
  BreakPointNr = strtoul(Argv[1], NULL, 10);
  DbgPrint("Deleting breakpoint %d.\n", BreakPointNr);
  KdbDeleteBreakPoint(BreakPointNr-1);
  return(1);
}

/* Sets a breakpoint using HW registers (i.e. to watch memory) */
STATIC LONG
DbgSetMemoryBreakPoint(ULONG Argc, PCH Argv[])
{
  ULONG DebugRegNr;
  UCHAR BreakType;
  ULONG Length, Address;
  ULONG Rw;
  ULONG i;
  PKTRAP_FRAME Tf = KdbTrapFrame;
  
  if (Argc != 2 && Argc < 5)
    {
      DbgPrint("ba <0-3> <r|w|e> <1|2|4> <address>\n");
      return(-1);
    }
  DebugRegNr = strtoul(Argv[1], NULL, 10);
  if (DebugRegNr >= 4)
    {
      DbgPrint("Debug register number should be between 0 and 3.\n");
      return(-1);
    }
  if (Argc == 2)
    {
      /* Clear the breakpoint. */
      Tf->Dr7 &= ~(0x3 << (DebugRegNr * 2));
      if ((Tf->Dr7 & 0xFF) == 0)
	{
	  /* 
	     If no breakpoints are enabled then 
	     clear the exact match flags. 
	  */
	  Tf->Dr7 &= 0xFFFFFCFF;
	}
      return(1);
    }
  BreakType = Argv[2][0];
  if (BreakType != 'r' && BreakType != 'w' && BreakType != 'e')
    {
      DbgPrint("Access type to break on should be either 'r', 'w' or 'e'.\n");
      return(-1);
    }
  Length = strtoul(Argv[3], NULL, 10);
  if (Length != 1 && Length != 2 && Length != 4)
    {
      DbgPrint("Length of the breakpoint should be one, two or four.\n");
      return(-1);
    }
  if (Length != 1 && BreakType == 'e')
    {
      DbgPrint("The length of an execution breakpoint should be one.\n");
      return(-1);
    }
  /* Stitch the remaining arguments back into a single string. */
  for (i = 4; i < Argc; i++)
    {
      Argv[i][-1] = ' ';
    }
  if (!KdbDecodeAddress(Argv[4], &Address))
    {
      return(-1);
    }
  if ((Address & (Length - 1)) != 0)
    {
      DbgPrint("The breakpoint address should be aligned to a multiple of "
	       "the breakpoint length.\n");
      return(-1);
    }

  /* Set the breakpoint address. */
  switch (DebugRegNr)
    {
    case 0: Tf->Dr0 = Address; break;
    case 1: Tf->Dr1 = Address; break;
    case 2: Tf->Dr2 = Address; break;
    case 3: Tf->Dr3 = Address; break;
    }
  /* Enable the breakpoint. */
  Tf->Dr7 |= (0x3 << (DebugRegNr * 2));
  /* Enable the exact match bits. */
  Tf->Dr7 |= 0x00000300;
  /* Clear existing state. */
  Tf->Dr7 &= ~(0xF << (16 + (DebugRegNr * 4)));
  /* Set the breakpoint type. */
  switch (BreakType)
    {
    case 'r': Rw = 3; break;
    case 'w': Rw = 1; break;
    case 'e': Rw = 0; break;
    }  
  Tf->Dr7 |= (Rw << (16 + (DebugRegNr * 4)));
  /* Set the breakpoint length. */
  Tf->Dr7 |= ((Length - 1) << (18 + (DebugRegNr * 4)));

  return(1);
}

/* Executes a single instruction */
STATIC LONG
DbgStep(ULONG Argc, PCH Argv[])
{
  /* Set the single step flag and return to the interrupted code. */
  KdbTrapFrame->Eflags |= (1 << 8);
  KdbIgnoreNextSingleStep = FALSE;
  KdbLastSingleStepFrom = KdbTrapFrame->Eip;
  
  return 0;
}

/* Steps over the current instruction */
STATIC LONG
DbgStepOver(ULONG Argc, PCH Argv[])
{
  PUCHAR Eip = (PUCHAR)KdbTrapFrame->Eip;
  
   /* Check if the current instruction is a call. */
  while (Eip[0] == 0x66 || Eip[0] == 0x67)
    {
      Eip++;
    }
  if (Eip[0] == 0xE8 || Eip[0] == 0x9A || Eip[0] == 0xF2 || Eip[0] == 0xF3 ||
      (Eip[0] == 0xFF && (Eip[1] & 0x38) == 0x10))
    {
      ULONG NextInst = KdbTrapFrame->Eip + KdbGetInstLength(KdbTrapFrame->Eip);
      KdbLastSingleStepFrom = KdbTrapFrame->Eip;
      KdbInsertBreakPoint(NextInst, TRUE, NULL);
      return 0;
    }
  else
    {
      return DbgStep(Argc, Argv);
    }
}

/* Finishes execution of the current function (works only of the stack frame
 * is maintained in %ebp)
 */
STATIC LONG
DbgFinish(ULONG Argc, PCH Argv[])
{
  PULONG Ebp = (PULONG)KdbTrapFrame->Ebp;
  ULONG ReturnAddress;
  NTSTATUS Status;
  PKTHREAD CurrentThread;

  /* Check that ebp points onto the stack. */
  CurrentThread = KeGetCurrentThread();
  if (CurrentThread == NULL ||
      !(Ebp >= (PULONG)CurrentThread->StackLimit && 
        Ebp <= (PULONG)CurrentThread->StackBase))
    {
      DbgPrint("This function doesn't appear to have a valid stack frame.\n");
      return -1;
    }

  /* Get the address of the caller. */
  Status = MmSafeCopyFromUser(&ReturnAddress, Ebp + 1, sizeof(ULONG));
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Memory access error (%X) while getting return address.\n", Status);
      return -1;
    }

  /* Set a temporary breakpoint at that location. */
  Status = KdbInsertBreakPoint(ReturnAddress, TRUE, NULL);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Couldn't set a temporary breakpoint at %X (Status %X)\n",
               ReturnAddress, Status);
      return -1;
    }

  /* Otherwise start running again and with any luck we will break back into
   * the debugger when the current function returns.
   */
  return 0;
}

/* Disassembles 10 instructions at eip or the given address */
STATIC LONG
DbgDisassemble(ULONG Argc, PCH Argv[])
{
  ULONG Address, i;
  LONG InstLen;

  if (Argc >= 2)
    {
      /* Stitch the remaining arguments back into a single string. */
      for (i = 2; i < Argc; i++)
        Argv[i][-1] = ' ';
      if (!KdbDecodeAddress(Argv[1], &Address))
        return -1;
    }
  else
    {
      Address = KdbTrapFrame->Eip;
    }

  for (i = 0; i < 10; i++)
    {
      if (!KdbPrintAddress((PVOID)Address))
	{
	  DbgPrint("<%x>", Address);
	}
      DbgPrint(": ");
      InstLen = KdbDisassemble(Address);      
      if (InstLen < 0)
	{
	  DbgPrint("<INVALID>\n");
	  return -1;
	}
      DbgPrint("\n");
      Address += InstLen;
    }

  return 1;
}

/* Displays help */
STATIC LONG
DbgProcessHelpCommand(ULONG Argc, PCH Argv[])
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
            DbgPrint(" ");
        }
      DbgPrint(" - %s\n", DebuggerCommands[i].Help);
    }
    
  return 1;
}

/* Lists the threads of the current process or the system */
STATIC LONG
DbgThreadListCommand(ULONG Argc, PCH Argv[])
{
  BOOL System = FALSE;
  if (Argc == 2 && _stricmp(Argv[1], "sys") == 0)
    System = TRUE;
    
  PsDumpThreads(System);
  return 1;
}

/* Lists the processes in the system */
STATIC LONG
DbgProcessListCommand(ULONG Argc, PCH Argv[])
{
  extern LIST_ENTRY PsProcessListHead;
  PLIST_ENTRY current_entry;
  PEPROCESS current;

  if (PsProcessListHead.Flink == NULL)
    {
      DbgPrint("No processes.\n");
      return 1;
    }

  DbgPrint("Process list: ");
  current_entry = PsProcessListHead.Flink;
  while (current_entry != &PsProcessListHead)
    {
      current = CONTAINING_RECORD(current_entry, EPROCESS, ProcessListEntry);
      DbgPrint("%d %s\n", current->UniqueProcessId, current->ImageFileName);
      current_entry = current_entry->Flink;
    }

  return 1;
}

/* Prints out symbol information for the given address */
STATIC LONG
DbgAddrCommand(ULONG Argc, PCH Argv[])
{
  PVOID Addr;

  if (Argc != 2)
    return -1;

  Addr = (PVOID)strtoul(Argv[1], NULL, 0);
  KdbPrintAddress(Addr);

  return 1;
}

/* Dumps memory at the given address */
STATIC LONG
DbgXCommand(ULONG Argc, PCH Argv[])
{
  PDWORD Addr = NULL;
  DWORD Items = 1;
  DWORD i = 2;
  DWORD Value = 0;
  NTSTATUS Status;

  if (Argc < 2)
    return -1;

  if (strchr(Argv[1], '<') != NULL && strchr(Argv[1], '>') == NULL)
    {
      for (; i < Argc; i++)
        {
          Argv[i][-1] = ' ';
          if (strchr(Argv[i], '>'))
            {
              i++;
              break;
            }
        }
    }

  if (!KdbDecodeAddress(Argv[1], (PULONG)&Addr))
    return -1;

  if (i < Argc)
    Items = (DWORD)strtoul(Argv[i], NULL, 0);

  for (i = 0; i < Items; i++)
    {
      if ((i % 4) == 0)
        {
	  if (i > 0)
	    DbgPrint("\n");
	  DbgPrint("%08x:", (int)(&Addr[i]));
        }
      Status = MmSafeCopyFromUser(&Value, Addr+i, sizeof (DWORD));
      if (!NT_SUCCESS(Status))
        {
	  DbgPrint("Couldn't read memory at 0x%x\n", Addr+i);
	  return -1;
        }
      DbgPrint("%08x ", Value);
    }

  return 1;
}

/* Writes value into memory at given address */
STATIC LONG
DbgWCommand(ULONG Argc, PCH Argv[])
{
  PDWORD Addr = NULL;
  DWORD Value = 0, Size = 0;
  NTSTATUS Status;
  PCHAR p = (PCHAR)&Value;
  INT i;

  if (Argc < 4)
    {
      DbgPrint("w <1|2|4> <value> <addr>\n");
      return -1;
    }

  Size = (DWORD)strtoul(Argv[1], NULL, 0);
  if (Size != 1 && Size != 2 && Size != 4)
    {
      DbgPrint("Size must be 1, 2 or 4\n");
      return -1;
    }
  Value = (DWORD)strtoul(Argv[2], NULL, 0);

  /* Stitch the remaining arguments back into a single string. */
  for (i = 4; i < Argc; i++)
    Argv[i][-1] = ' ';
  if (!KdbDecodeAddress(Argv[3], (PULONG)&Addr))
    return -1;

  if (Size == 1)
    p += 3;
  else if (Size == 2)
    p += 2;
  Status = MmSafeCopyToUser(Addr, p, Size);
  
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Couldn't write memory at 0x%x\n", Addr);
      return -1;
    }
  
  return 1;
}

/* Enters JavaScript mode */
LONG
DbgScriptCommand(ULONG Argc, PCH Argv[])
{
  PCHAR Buffer;
  PCHAR BufferStart;
  static void *interp = 0;
  void *script_cmd_context[1];
  BOOLEAN InitScriptRan = FALSE;

  if (interp == NULL)
    {
      interp = kjs_create_interp(NULL);
      if (interp == NULL)
        return -1;
    }

  BufferStart = Buffer = ExAllocatePool(NonPagedPool, 4096);
  if (Buffer == NULL)
    return -1;

  script_cmd_context[0] = interp;
  
  /* register some functions for the System object */
  kjs_system_register(interp, "regs", script_cmd_context, KjsGetRegister);
  kjs_system_register(interp, "regread", script_cmd_context, KjsReadRegValue);
  kjs_system_register(interp, "getmodule", script_cmd_context, KjsGetNthModule);
  kjs_system_register(interp, "dbgcommand", script_cmd_context, KjsDebuggerCommand);
  kjs_system_register(interp, "getcommand", script_cmd_context, KjsGetCommand);
  kjs_system_register(interp, "getinstlen", script_cmd_context, KjsGetInstLength);
  kjs_system_register(interp, "getsymbol", script_cmd_context, KjsGetSymbol);
  kjs_system_register(interp, "mread", script_cmd_context, KjsReadMemory);
  kjs_system_register(interp, "mwrite", script_cmd_context, KjsWriteMemory);

  if (KjsInitScript != NULL)
    {
      kjs_eval(interp, KjsInitScript);
      InitScriptRan = TRUE;
    }
  else
    {
      DbgPrint("Note: The KJS init script was not loaded yet,\n");
      DbgPrint("      only builtin functions are available.\n");
    }

  DbgPrint("\nKernel Debugger Script Interface (JavaScript :-)\n");
  DbgPrint("Terminate input with ;; and end scripting with .\n");
  for (;;)
    {
      if (Buffer != BufferStart)
        DbgPrint("..... ");
      else
        DbgPrint("kjs:> ");
      KdbGetCommand(BufferStart);
      if (BufferStart[0] == '.')
        {
          if (BufferStart != Buffer)
            {
              DbgPrint("Input Aborted.\n");
              BufferStart = Buffer;
            }
          else
            {
              /* Single dot input -> exit */
              break;
            }
        }
      else
        {
          if (FindJSEndMark(Buffer))
            {
              /* check if the init script was loaded */
              if (!InitScriptRan && KjsInitScript != NULL)
                {
                  kjs_eval(interp, KjsInitScript);
                  InitScriptRan = TRUE;
                }

              kjs_eval(interp, Buffer);
              BufferStart = Buffer;
              DbgPrint("\n");
            }
          else
            {
              BufferStart = BufferStart + strlen(BufferStart);
            }
        }
    }

  ExFreePool(Buffer);

  kjs_system_unregister(interp, script_cmd_context, KjsGetRegister);
  kjs_system_unregister(interp, script_cmd_context, KjsReadRegValue);
  kjs_system_unregister(interp, script_cmd_context, KjsGetNthModule);
  kjs_system_unregister(interp, script_cmd_context, KjsDebuggerCommand);
  kjs_system_unregister(interp, script_cmd_context, KjsGetCommand);
  kjs_system_unregister(interp, script_cmd_context, KjsGetInstLength);
  kjs_system_unregister(interp, script_cmd_context, KjsGetSymbol);
  kjs_system_unregister(interp, script_cmd_context, KjsReadMemory);
  kjs_system_unregister(interp, script_cmd_context, KjsWriteMemory);

  return 1;
}

/* Prints stack back trace. */
LONG
DbgBackTraceCommand(ULONG Argc, PCH Argv[])
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
      DbgPrintBackTrace((PULONG)&KdbTrapFrame->DebugEbp, StackBase, StackLimit);
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

/* Prints values of cregs. */
LONG 
DbgCRegsCommand(ULONG Argc, PCH Argv[])
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
	   Cr1, Cr2, Cr3, Cr4, (ULONG)Tr, Ldtr);
  return(1);
}

/* Prints values of the Debug Registers. */
LONG 
DbgDRegsCommand(ULONG Argc, PCH Argv[])
{
  PKTRAP_FRAME Tf = KdbTrapFrame;
  DbgPrint("Trap   : DR0 %.8x DR1 %.8x DR2 %.8x DR3 %.8x DR6 %.8x DR7 %.8x\n",
	   Tf->Dr0, Tf->Dr1, Tf->Dr2, Tf->Dr3, Tf->Dr6, Tf->Dr7);
  return(1);
}

/* Continues execution of the program (exits debugger) */
LONG
DbgContCommand(ULONG Argc, PCH Argv[])
{
  /* Not too difficult. */
  return(0);
}

/* Toggles serial echo. */
LONG
DbgEchoToggle(ULONG Argc, PCH Argv[])
{
  KbdEchoOn = !KbdEchoOn;
  return(TRUE);
}

/* Prints values of General Purpose Registers. */
LONG
DbgRegsCommand(ULONG Argc, PCH Argv[])
{
  PKTRAP_FRAME Tf = KdbTrapFrame;
  DbgPrint("CS:EIP %.4x:%.8x, EAX %.8x EBX %.8x ECX %.8x EDX %.8x\n",
	   Tf->Cs & 0xFFFF, Tf->Eip, Tf->Eax, Tf->Ebx, Tf->Ecx, Tf->Edx);
  DbgPrint("ESI %.8x EDI %.8x EBP %.8x SS:ESP %.4x:%.8x\n",
	   Tf->Esi, Tf->Edi, Tf->Ebp, Tf->Ss & 0xFFFF, Tf->Esp);
  DbgPrintEflags(Tf->Eflags);
  return(1);
}

/* Bugchecks the system. */
LONG
DbgBugCheckCommand(ULONG Argc, PCH Argv[])
{
  KEBUGCHECK(1);
  return(1);
}

LONG
DbgShowFilesCommand(ULONG Argc, PCH Argv[])
{
  DbgShowFiles();
  return(1);
}

LONG
DbgEnableFileCommand(ULONG Argc, PCH Argv[])
{
  if (Argc == 2)
    {
      if (strlen(Argv[1]) > 0)
        {
          DbgEnableFile(Argv[1]);
        }
    }
  return(1);
}

LONG
DbgDisableFileCommand(ULONG Argc, PCH Argv[])
{
  if (Argc == 2)
    {
      if (strlen(Argv[1]) > 0)
        {
          DbgDisableFile(Argv[1]);
        }
    }
  return(1);
}

VOID
KdbCreateThreadHook(PCONTEXT Context)
{
  Context->Dr0 = x_dr0;
  Context->Dr1 = x_dr1;
  Context->Dr2 = x_dr2;
  Context->Dr3 = x_dr3;
  Context->Dr7 = x_dr7;
}

/* Executes a debugger command */
LONG
KdbDoCommand(PCH CommandLine)
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
	  return DebuggerCommands[i].Fn(Argc, Argv);
	}
    }
  DbgPrint("Command '%s' is unknown.", OrigCommand);
  return -1;
}

/* Main-loop - reads commands and executes them */
VOID STDCALL
KdbMainLoop()
{
  CHAR Command[256];
  LONG s;

  if (!KdbEnteredOnSingleStep)
    {
      DbgPrint("\nEntered kernel debugger (type \"help\" for a list of commands)\n");
    }
  else
    {
      if (!KdbPrintAddress((PVOID)KdbTrapFrame->Eip))
	{
	  DbgPrint("<%x>", KdbTrapFrame->Eip);
	}
      DbgPrint(": ");
      if (KdbDisassemble(KdbTrapFrame->Eip) < 0)
	{
	  DbgPrint("<INVALID>");
	}
      KdbEnteredOnSingleStep = FALSE;
      KdbLastSingleStepFrom = 0xFFFFFFFF;
    }

  do
    {
      DbgPrint("\nkdb:> ");

      KdbGetCommand(Command);
      s = KdbDoCommand(Command);    
    }
  while (s != 0);
    
  longjmp(KdbMainLoopJump, 1);
}

/* This function calls another one with on a given stack. It is not safe to
 * return from the called function!
 */
VOID STDCALL KdbCallWithOtherStack(PVOID Stack, int StackSize, PVOID Func, PVOID Frame);
asm(
".globl _KdbCallWithOtherStack@16"     "\n\t"
".func _KdbCallWithOtherStack@16"      "\n\t"
"_KdbCallWithOtherStack@16:"           "\n\t"
"movl  0x10(%esp), %ebp"               "\n\t"
"movl  0xC(%esp),  %eax"               "\n\t" /* Func */
"movl  0x8(%esp),  %ecx"               "\n\t" /* StackSize */
"movl  %esp,       %ebx"               "\n\t" /* save current stack */
"movl  0x4(%esp),  %esp"               "\n\t" /* use new stack */
"addl  %ecx,       %esp"               "\n\t" /* set esp to end of new stack */
"pushl %ebx"                           "\n\t" /* push old stack */
"pushl $0xDEADBEEF"                    "\n\t" /* push dead return address */
"jmpl *%eax"                           "\n\t" /* call function (should not return) */
".endfunc"                             "\n\t"
);

#if 0
asm(
".globl _KdbCallWithOtherStack@16"     "\n\t"
".func _KdbCallWithOtherStack@16"      "\n\t"
"_KdbCallWithOtherStack@16:"           "\n\t"
"pushl %eax"                           "\n\t"
"pushl %ebx"                           "\n\t"
"pushl %ecx"                           "\n\t"
"pushl %edx"                           "\n\t"
"movl  0x20(%esp), %edx"               "\n\t" /* Param */
"movl  0x1C(%esp), %eax"               "\n\t" /* Func */
"movl  0x18(%esp), %ecx"               "\n\t" /* StackSize */
"movl  %esp,       %ebx"               "\n\t" /* save current stack */
"movl  0x14(%esp), %esp"               "\n\t" /* use new stack */
"addl  %ecx,       %esp"               "\n\t" /* set esp to end of new stack */
"pushl %ebx"                           "\n\t" /* push old stack */
"pushl %edx"                           "\n\t" /* push the param */
"calll *%eax"                          "\n\t" /* call function (STDCALL) */
"popl  %ebx"                           "\n\t"
"movl  %ebx,       %esp"               "\n\t" /* restore old stack */
"popl  %edx"                           "\n\t"
"popl  %ecx"                           "\n\t"
"popl  %ebx"                           "\n\t"
"popl  %eax"                           "\n\t"
"retl  $16"                            "\n\t"
".endfunc"                             "\n\t"
);
#endif

/* Internal enter function - runs KDB on it's own stack */
VOID
KdbInternalEnter(PKTRAP_FRAME Tf)
{  
  __asm__ __volatile__ ("cli\n\t");

  KbdDisableMouse();
  HalReleaseDisplayOwnership();

  /* allocate stack */
  if (KdbStack == NULL)
    {
      KdbStack = ExAllocatePool( NonPagedPool, KDB_STACK_SIZE );
      if (KdbStack == NULL)
        {
	  DbgPrint("Couldn't allocate stack for debugger!\n");
	  __asm__( "hlt" );
        }
    }

  KdbTrapFrame = Tf;
  if (setjmp(KdbMainLoopJump) == 0)
    {
      if (KdbResumeJavaScript == TRUE)
        {
          ULONG ExpNr = (ULONG)KdbTrapFrame->DebugArgMark;

          /* jump back into JS */
          KdbResumeJavaScript = FALSE;
          longjmp(KdbJavaScriptJump, ExpNr+1); /* we are not allowed to pass 0 */
        }
      else
        {
          KdbCallWithOtherStack(KdbStack, KDB_STACK_SIZE, KdbMainLoop,
                                (PVOID)__builtin_frame_address(0));
          /* will never return */
        }
    }

  KbdEnableMouse();
  __asm__ __volatile__("sti\n\t");
}

/* Called by the OS if an unhandled exception has happened. */
KD_CONTINUE_TYPE
KdbEnterDebuggerException(PEXCEPTION_RECORD ExceptionRecord,
			  PCONTEXT Context,
			  PKTRAP_FRAME TrapFrame)
{
  LONG BreakPointNr;
  ULONG ExpNr = (ULONG)TrapFrame->DebugArgMark;

  /* Exception inside the debugger? Game over. */
  if (KdbEntryCount > 0)
    {
      return(kdHandleException);
    }
  KdbEntryCount++;

  /* Clear the single step flag. */
  TrapFrame->Eflags &= ~(1 << 8);
  /* 
     Reenable any breakpoints we disabled so we could execute the breakpointed
     instructions.
  */
  KdbRenableBreakPoints();
  /* Silently ignore a debugger initiated single step. */
  if (ExpNr == 1 && KdbIgnoreNextSingleStep)
    {      
      KdbIgnoreNextSingleStep = FALSE;
      KdbEntryCount--;
      return(kdContinue);
    }
  /* If we stopped on one of our breakpoints then let the user know. */
  if (ExpNr == 3 && (BreakPointNr = KdbIsBreakPointOurs(TrapFrame)) >= 0)
    {
      DbgPrint("Entered debugger on %sbreakpoint %d.\n",
               (KdbActiveBreakPoints[BreakPointNr].Temporary) ? ("temporary ") : (""),
               BreakPointNr+1);
      /*
	The breakpoint will point to the next instruction by default so
	point it back to the start of original instruction.
      */
      TrapFrame->Eip--;
      /*
	..and restore the original instruction.
      */
      (VOID)KdbOverwriteInst(TrapFrame->Eip, NULL,
			     KdbActiveBreakPoints[BreakPointNr].SavedInst);
      /*
	If this was a breakpoint set by the debugger then delete it otherwise
	flag to enable it again after we step over this instruction.
      */
      if (KdbActiveBreakPoints[BreakPointNr].Temporary)
	{
	  KdbActiveBreakPoints[BreakPointNr].Assigned = FALSE;
	  KdbBreakPointCount--;
	  KdbEnteredOnSingleStep = TRUE;
	}
      else
	{
	  KdbActiveBreakPoints[BreakPointNr].Enabled = FALSE;
	  TrapFrame->Eflags |= (1 << 8);
	  KdbIgnoreNextSingleStep = TRUE;
	}
    }
  else if (ExpNr == 1)
    {
      if ((TrapFrame->Dr6 & 0xF) != 0)
	{
	  DbgPrint("Entered debugger on memory breakpoint(s) %s%s%s%s.\n",
		   (TrapFrame->Dr6 & 0x1) ? "1" : "",
		   (TrapFrame->Dr6 & 0x2) ? "2" : "",
		   (TrapFrame->Dr6 & 0x4) ? "3" : "",
		   (TrapFrame->Dr6 & 0x8) ? "4" : "");
	}
      else if (KdbLastSingleStepFrom != 0xFFFFFFFF)
	{
	  KdbEnteredOnSingleStep = TRUE;
	}
      else
	{
	  DbgPrint("Entered debugger on unexpected debug trap.\n");
	}
    }
  else
    {
      DbgPrint("Entered debugger on exception number %d.\n", ExpNr);
    }
  KdbInternalEnter(TrapFrame);
  KdbEntryCount--;
  if (ExpNr != 1 && ExpNr != 3)
    {
      return(kdHandleException);
    }
  else
    {
      /* Clear dr6 status flags. */
      TrapFrame->Dr6 &= 0xFFFF1F00;
      /* Set the RF flag to we don't trigger the same breakpoint again. */
      if (ExpNr == 1)
	{
	  TrapFrame->Eflags |= (1 << 16);
	}
      return(kdContinue);
    }
}

/* Init function (reads \\SystemRoot\\kjsinit.js) */
VOID INIT_FUNCTION
KdbInit2(VOID)
{
  HANDLE hFile;
  NTSTATUS Status;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING FileName;
  IO_STATUS_BLOCK Iosb;
  FILE_STANDARD_INFORMATION FileStdInfo;
  INT i;

  RtlInitUnicodeString(&FileName, L"\\SystemRoot\\kjsinit.js");
  InitializeObjectAttributes(&ObjectAttributes,
                             &FileName,
                             0,
                             NULL,
                             NULL);

  /* open the file */
  Status = NtCreateFile(&hFile,
                        FILE_READ_ACCESS,
                        &ObjectAttributes,
                        &Iosb,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        0,
                        FILE_OPEN,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NO_INTERMEDIATE_BUFFERING,
                        NULL,
                        0);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Failed to open \\SystemRoot\\kjsinit.js (Status 0x%x)\n", Status);
      return;
    }

  /* get the size of the file  */
  Status = NtQueryInformationFile(hFile,
                                  &Iosb,
                                  &FileStdInfo,
                                  sizeof(FileStdInfo),
                                  FileStandardInformation);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Could not get file size of \\SystemRoot\\kjsinit.js (Status 0x%x)\n", Status);
      NtClose(hFile);
      return;
    }

  /* allocate nonpageable memory for kjs init script */
  KjsInitScript = ExAllocatePool(NonPagedPool, FileStdInfo.AllocationSize.u.LowPart+1);
  if (KjsInitScript == NULL)
    {
      DbgPrint("Could not allocate memory for kjs init script (Status 0x%x)\n", Status);
      NtClose(hFile);
      return;
    }

  /* load file into memory */
  Status = NtReadFile(hFile,
                      0, 0, 0,
                      &Iosb,
                      KjsInitScript,
                      FileStdInfo.EndOfFile.u.LowPart,
                      0, 0);
  if (!NT_SUCCESS(Status) && Status != STATUS_END_OF_FILE)
    {
      DbgPrint("Could not read \\SystemRoot\\kjsinit.js into memory (Status 0x%x)\n", Status);
      ExFreePool(KjsInitScript);
      NtClose(hFile);
      return;
    }
  KjsInitScript[FileStdInfo.EndOfFile.u.LowPart] = '\0';
  NtClose(hFile);
  
  /* remove newlines */
  for (i = 0; i < FileStdInfo.EndOfFile.u.LowPart && KjsInitScript[i] != '\0'; i++)
    {
      if (KjsInitScript[i] == '\r' || KjsInitScript[i] == '\n')
        {
	  INT end = i+1;
	  for (; end < FileStdInfo.EndOfFile.u.LowPart && KjsInitScript[end] != '\0'; end++)
	    {
	      if (KjsInitScript[end] != '\r' && KjsInitScript[end] != '\n')
	        break;
	    }
	  strcpy(KjsInitScript+i, KjsInitScript+end);
        }
    }
}
