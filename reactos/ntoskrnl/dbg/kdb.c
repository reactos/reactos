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
/* $Id: kdb.c,v 1.15 2004/01/10 21:06:38 arty Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/dbg/kdb.c
 * PURPOSE:         Kernel debugger
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 01/03/01
 */

/* INCLUDES ******************************************************************/

#include <ctype.h>
#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/ps.h>
#include <limits.h>
#include <internal/kd.h>
#include "kdb.h"
#include "kjs.h"

#define NDEBUG
#include <internal/debug.h>

/* TYPES *********************************************************************/

/* GLOBALS *******************************************************************/

int isalpha( int );
VOID
PsDumpThreads(BOOLEAN System);
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
DbgAddrCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf);
ULONG
DbgXCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf);
ULONG
DbgScriptCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf);
ULONG
DbgThreadListCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf);
ULONG
DbgProcessListCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf);
ULONG
DbgProcessHelpCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf);
ULONG
DbgShowFilesCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf);
ULONG
DbgEnableFileCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf);
ULONG
DbgDisableFileCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf);

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
  {"addr", "addr <address>", "Displays symbol info", DbgAddrCommand},
  {"x", "x <addr> <words>", "Displays <addr> for <words>", DbgXCommand},
  {"plist", "plist", "Display processes in the system", DbgProcessListCommand},
  {"tlist", "tlist [sys]", "Display threads in the system", DbgThreadListCommand},
  {"sfiles", "sfiles", "Show files that print debug prints", DbgShowFilesCommand},
  {"efile", "efile <filename>", "Enable debug prints from file", DbgEnableFileCommand},
  {"dfile", "dfile <filename>", "Disable debug prints from file", DbgDisableFileCommand},
  {"js", "js", "Script mode", DbgScriptCommand},
  {"help", "help", "Display help screen", DbgProcessHelpCommand},
  {NULL, NULL, NULL}
};

volatile DWORD x_dr0 = 0, x_dr1 = 0, x_dr2 = 0, x_dr3 = 0, x_dr7 = 0;

/* FUNCTIONS *****************************************************************/

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
  PCH Orig = Buffer;

  for (;;)
    {
      if (KdDebugState & KD_DEBUG_KDSERIAL)
	while ((Key = KdbTryGetCharSerial()) == -1);
      else
	while ((Key = KdbTryGetCharKeyboard()) == -1);

      if (Key == '\r' || Key == '\n')
	{
	  DbgPrint("\n");
	  *Buffer = 0;
	  return;
	}
      else if (Key == '\x8')
        {
          if (Buffer > Orig)
            {
              Buffer--;
              *Buffer = 0;
              DbgPrint("%c %c", 8, 8);
	    }
        }
      else
        {
          DbgPrint("%c", Key);

          *Buffer = Key;
          Buffer++;
        }
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
DbgThreadListCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf)
{
  BOOL System = FALSE;
  if (Argc == 2 && (!strcmp(Argv[1], "sys") || !strcmp(Argv[1], "SYS")))
    System = TRUE;
    
  PsDumpThreads(System);
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
      KdbPrintAddress((PVOID)Frame[1]);
      Frame = (PULONG)Frame[0];
      i++;
    }

  if ((i % 10) != 0)
    {
      DbgPrint("\n");
    }
}

ULONG
DbgAddrCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME tf)
{
  PVOID Addr;

  if (Argc == 2)
    {
      Addr = (PVOID)strtoul(Argv[1], NULL, 0);
      KdbPrintAddress(Addr);
    }

  return(1);
}

ULONG
DbgXCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME tf)
{
  PDWORD Addr = 0;
  DWORD Items = 1;
  DWORD i = 0;

  if (Argc >= 2)
    Addr = (PDWORD)strtoul(Argv[1], NULL, 0);
  if (Argc >= 3)
    Items = (DWORD)strtoul(Argv[2], NULL, 0);

  if( !Addr ) return(1);

  for( i = 0; i < Items; i++ ) 
    {
      if( (i % 4) == 0 ) {
	if( i ) DbgPrint("\n");
	DbgPrint("%08x:", (int)(&Addr[i]));
      }
      DbgPrint( "%08x ", Addr[i] );
    }

  return(1);
}

static int KjsReadRegValue( void *context,
			    JSNode *result,
			    JSNode *args ) {
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
  
  if (args->u.vinteger != 2 ||
      args[1].type != JS_STRING || args[2].type != JS_STRING) {
    return JS_PROPERTY_FOUND;
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
  QueryTable->Flags =
    RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_DIRECT;
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

static int KjsGetRegister( void *context, 
			   JSNode *result, 
			   JSNode *args ) {
  PVOID *context_list = context;
  if( args->u.vinteger == 1 && args->type == JS_INTEGER ) {
    DWORD Result = ((DWORD *)context_list[1])[args[1].u.vinteger];
    result->type = JS_INTEGER;
    result->u.vinteger = Result;
  }

  return JS_PROPERTY_FOUND;
}

static int KjsGetNthModule( void *context,
			    JSNode *result,
			    JSNode *args ) {
  PVOID *context_list = context;
  PKJS kjs = (PKJS)context_list[0];
  JSVirtualMachine *vm = kjs->vm;
  PLIST_ENTRY current_entry;
  MODULE_TEXT_SECTION *current;
  extern LIST_ENTRY ModuleTextListHead;
  int n = 0;
  
  if (args->u.vinteger != 1 || args[1].type != JS_INTEGER) {
    return JS_PROPERTY_FOUND;
  }

  current_entry = ModuleTextListHead.Flink;

  while (current_entry != &ModuleTextListHead &&
	 current_entry != NULL &&
	 n <= args[1].u.vinteger) {
    current = CONTAINING_RECORD(current_entry, MODULE_TEXT_SECTION,
				ListEntry);    
    current_entry = current_entry->Flink;
    n++;
  }

  if (current_entry && current) {
    ANSI_STRING NameStringNarrow;
    UNICODE_STRING NameUnicodeString;

    RtlInitUnicodeString( &NameUnicodeString, current->Name );
    RtlUnicodeStringToAnsiString( &NameStringNarrow,
				  &NameUnicodeString,
				  TRUE );

    js_vm_make_array (vm, result, 2);
  
    js_vm_make_string (vm, 
		       &result->u.varray->data[0], 
		       NameStringNarrow.Buffer,
		       NameStringNarrow.Length);

    RtlFreeAnsiString(&NameStringNarrow);

    result->u.varray->data[1].type = JS_INTEGER;
    result->u.varray->data[1].u.vinteger = (DWORD)current->Base;
    result->type = JS_ARRAY;
    return JS_PROPERTY_FOUND;
  }
  result->type = JS_UNDEFINED;
  return JS_PROPERTY_FOUND;
}

static BOOL FindJSEndMark( PCHAR Buffer ) {
  int i;

  for( i = 0; Buffer[i] && Buffer[i+1]; i++ ) {
    if( Buffer[i] == ';' && Buffer[i+1] == ';' ) return TRUE;
  }
  return FALSE;
}

ULONG
DbgScriptCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME tf)
{
  PCHAR Buffer;
  PCHAR BufferStart;
  static void *interp = 0;
  void *script_cmd_context[2];

  if( !interp ) interp = kjs_create_interp(NULL);
  if( !interp ) return 1;

  BufferStart = Buffer = ExAllocatePool( NonPagedPool, 4096 );
  if( !Buffer ) return 1;

  script_cmd_context[0] = interp;
  script_cmd_context[1] = &tf;
  
  kjs_system_register( interp, "regs", script_cmd_context,
		       KjsGetRegister );
  kjs_system_register( interp, "regread", script_cmd_context,
		       KjsReadRegValue );
  kjs_system_register( interp, "getmodule", script_cmd_context,
		       KjsGetNthModule );

  kjs_eval( interp,
	    "eval("
	    "System.regread("
	    "'\\\\Registry\\\\Machine\\\\System\\\\"
	    "CurrentControlSet\\\\Control\\\\Kdb',"
	    "'kjsinit'));" );

  DbgPrint("\nKernel Debugger Script Interface (JavaScript :-)\n");
  DbgPrint("Terminate input with ;; and end scripting with .\n");
  do
    {
      if( Buffer != BufferStart )
	DbgPrint("..... ");
      else
	DbgPrint("kjs:> ");
      KdbGetCommand( BufferStart );
      if( BufferStart[0] == '.' ) {
	if( BufferStart != Buffer ) {
	  DbgPrint("Input Aborted.\n");
	  BufferStart = Buffer;
	} else {
	  /* Single dot input -> exit */
	  break;
	}
      } else {
	if( FindJSEndMark( Buffer ) ) {
	  kjs_eval( interp, Buffer );
	  BufferStart = Buffer;
	  DbgPrint("\n");
	} else {
	  BufferStart = BufferStart + strlen(BufferStart);
	}
      }
    } while (TRUE);

  ExFreePool( Buffer );

  kjs_system_unregister( interp, script_cmd_context, KjsGetRegister );
  kjs_system_unregister( interp, script_cmd_context, KjsReadRegValue );
  kjs_system_unregister( interp, script_cmd_context, KjsGetNthModule );

  return(1);
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
  DbgPrint("Trap   : DR0 %.8x DR1 %.8x DR2 %.8x DR3 %.8x DR6 %.8x DR7 %.8x\n",
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
  KEBUGCHECK(1);
  return(1);
}

ULONG
DbgShowFilesCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf)
{
  DbgShowFiles();
  return(1);
}

ULONG
DbgEnableFileCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf)
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

ULONG
DbgDisableFileCommand(ULONG Argc, PCH Argv[], PKTRAP_FRAME Tf)
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
      DbgPrint("\nkdb:> ");

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

KD_CONTINUE_TYPE
KdbEnterDebuggerException(PEXCEPTION_RECORD ExceptionRecord,
			  PCONTEXT Context,
			  PKTRAP_FRAME TrapFrame)
{
  DbgPrint("Entered debugger on exception number %d.\n", 
	   TrapFrame->DebugArgMark);
  KdbInternalEnter(TrapFrame);
  return(kdHandleException);
}
