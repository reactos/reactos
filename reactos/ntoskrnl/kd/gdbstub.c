/****************************************************************************

		THIS SOFTWARE IS NOT COPYRIGHTED

   HP offers the following for use in the public domain.  HP makes no
   warranty with regard to the software or it's performance and the
   user accepts the software "AS IS" with all faults.

   HP DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD
   TO THIS SOFTWARE INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

****************************************************************************/

/****************************************************************************
 *  Header: remcom.c,v 1.34 91/03/09 12:29:49 glenne Exp $
 *
 *  Module name: remcom.c $
 *  Revision: 1.34 $
 *  Date: 91/03/09 12:29:49 $
 *  Contributor:     Lake Stevens Instrument Division$
 *
 *  Description:     low level support for gdb debugger. $
 *
 *  Considerations:  only works on target hardware $
 *
 *  Written by:      Glenn Engel $
 *  ModuleState:     Experimental $
 *
 *  NOTES:           See Below $
 *
 *  Modified for 386 by Jim Kingdon, Cygnus Support.
 *  Modified for ReactOS by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 *  To enable debugger support, two things need to happen.  One, setting
 *  up a routine so that it is in the exception path, is necessary in order
 *  to allow any breakpoints or error conditions to be properly intercepted
 *  and reported to gdb.
 *  Two, a breakpoint needs to be generated to begin communication.
 *
 *  Because gdb will sometimes write to the stack area to execute function
 *  calls, this program cannot rely on using the supervisor stack so it
 *  uses it's own stack area.
 *
 *************
 *
 *    The following gdb commands are supported:
 *
 * command          function                               Return value
 *
 *    g             return the value of the CPU Registers  hex data or ENN
 *    G             set the value of the CPU Registers     OK or ENN
 *
 *    mAA..AA,LLLL  Read LLLL bytes at address AA..AA      hex data or ENN
 *    MAA..AA,LLLL: Write LLLL bytes at address AA.AA      OK or ENN
 *
 *    c             Resume at current address              SNN   ( signal NN)
 *    cAA..AA       Continue at address AA..AA             SNN
 *
 *    s             Step one instruction                   SNN
 *    sAA..AA       Step one instruction from AA..AA       SNN
 *
 *    k             kill
 *
 *    ?             What was the last sigval ?             SNN   (signal NN)
 *
 * All commands and responses are sent with a packet which includes a
 * Checksum.  A packet consists of
 *
 * $<packet info>#<Checksum>.
 *
 * where
 * <packet info> :: <characters representing the command or response>
 * <Checksum>    :: < two hex digits computed as modulo 256 sum of <packetinfo>>
 *
 * When a packet is received, it is first acknowledged with either '+' or '-'.
 * '+' indicates a successful transfer.  '-' indicates a failed transfer.
 *
 * Example:
 *
 * Host:                  Reply:
 * $m0,10#2a               +$00010203040506070809101112131415#42
 *
 ****************************************************************************/

#include <ntddk.h>
#include <internal/kd.h>
#include <internal/ke.h>
#include <internal/ps.h>
#include <internal/module.h>
#include <internal/ldr.h>

#define NDEBUG
#include <internal/debug.h>

extern LIST_ENTRY PiThreadListHead;


/************************************************************************/
/* BUFMAX defines the maximum number of characters in inbound/outbound buffers*/
/* at least NUMREGBYTES*2 are needed for register packets */
#define BUFMAX 1000

static BOOLEAN GspInitialized;
#if 0
static PKINTERRUPT GspInterrupt;
#endif

static BOOLEAN GspRemoteDebug;

static CONST CHAR HexChars[]="0123456789abcdef";

static PETHREAD GspRunThread; /* NULL means run all threads */
static PETHREAD GspDbgThread;
static PETHREAD GspEnumThread;

/* Number of Registers.  */
#define NUMREGS	16

enum REGISTER_NAMES
{
  EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI,
	PC /* also known as eip */,
	PS /* also known as eflags */,
	CS, SS, DS, ES, FS, GS
};

typedef struct _CPU_REGISTER
{
  DWORD Size;
  DWORD OffsetInTF;
} CPU_REGISTER, *PCPU_REGISTER;

#define KTRAP_FRAME_X86 KTRAP_FRAME

#define EIP_REGNO 8

static CPU_REGISTER GspRegisters[NUMREGS] =
{
  { 4, FIELD_OFFSET (KTRAP_FRAME_X86, Eax) },
  { 4, FIELD_OFFSET (KTRAP_FRAME_X86, Ecx) },
  { 4, FIELD_OFFSET (KTRAP_FRAME_X86, Edx) },
  { 4, FIELD_OFFSET (KTRAP_FRAME_X86, Ebx) },
  { 4, FIELD_OFFSET (KTRAP_FRAME_X86, Esp) },
  { 4, FIELD_OFFSET (KTRAP_FRAME_X86, Ebp) },
  { 4, FIELD_OFFSET (KTRAP_FRAME_X86, Esi) },
  { 4, FIELD_OFFSET (KTRAP_FRAME_X86, Edi) },
  { 4, FIELD_OFFSET (KTRAP_FRAME_X86, Eip) },
  { 4, FIELD_OFFSET (KTRAP_FRAME_X86, Eflags) },
  { 4, FIELD_OFFSET (KTRAP_FRAME_X86, Cs) },
  { 4, FIELD_OFFSET (KTRAP_FRAME_X86, Ss) },
  { 4, FIELD_OFFSET (KTRAP_FRAME_X86, Ds) },
  { 4, FIELD_OFFSET (KTRAP_FRAME_X86, Es) },
  { 4, FIELD_OFFSET (KTRAP_FRAME_X86, Fs) },
  { 4, FIELD_OFFSET (KTRAP_FRAME_X86, Gs) }
};

static PCHAR GspThreadStates[THREAD_STATE_MAX] =
{
  "Initialized",  /* THREAD_STATE_INITIALIZED */
  "Ready",        /* THREAD_STATE_READY */
  "Running",      /* THREAD_STATE_RUNNING */
  "Suspended",    /* THREAD_STATE_SUSPENDED */
  "Frozen",       /* THREAD_STATE_FROZEN */
  "Terminated 1", /* THREAD_STATE_TERMINATED_1 */
  "Terminated 2", /* THREAD_STATE_TERMINATED_2 */
  "Blocked"       /* THREAD_STATE_BLOCKED */
};

char *
strtok(char *s, const char *delim)
{
  const char *spanp;
  int c, sc;
  char *tok;
  static char *last;


  if (s == NULL && (s = last) == NULL)
    return (NULL);

  /*
   * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
   */
 cont:
  c = *s++;
  for (spanp = delim; (sc = *spanp++) != 0;) {
    if (c == sc)
      goto cont;
  }

  if (c == 0) {			/* no non-delimiter characters */
    last = NULL;
    return (NULL);
  }
  tok = s - 1;

  /*
   * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
   * Note that delim must have one NUL; we stop if we see that, too.
   */
  for (;;) {
    c = *s++;
    spanp = delim;
    do {
      if ((sc = *spanp++) == c) {
	if (c == 0)
	  s = NULL;
	else
	  s[-1] = 0;
	last = s;
	return (tok);
      }
    } while (sc != 0);
  }
  /* NOTREACHED */
}


LONG
HexValue (CHAR ch)
{
  if ((ch >= '0') && (ch <= '9')) return (ch - '0');
  if ((ch >= 'a') && (ch <= 'f')) return (ch - 'a' + 10);
  if ((ch >= 'A') && (ch <= 'F')) return (ch - 'A' + 10);
  return (-1);
}

static CHAR GspInBuffer[BUFMAX];
static CHAR GspOutBuffer[BUFMAX];

/* scan for the sequence $<data>#<Checksum>     */

PCHAR
GspGetPacket()
{
  PCHAR Buffer = &GspInBuffer[0];
  CHAR Checksum;
  CHAR XmitChecksum;
  ULONG Count;
  CHAR ch;

  while (TRUE)
    {
      /* wait around for the start character, ignore all other characters */
      while ((ch = KdGetChar ()) != '$');

    retry:
      Checksum = 0;
      XmitChecksum = -1;
      Count = 0;

      /* now, read until a # or end of Buffer is found */
      while (Count < BUFMAX)
	{
	  ch = KdGetChar ();
	  if (ch == '$')
	    goto retry;
	  if (ch == '#')
	    break;
	  Checksum = Checksum + ch;
	  Buffer[Count] = ch;
	  Count = Count + 1;
	}
      Buffer[Count] = 0;

      if (ch == '#')
	{
	  ch = KdGetChar ();
	  XmitChecksum = HexValue (ch) << 4;
	  ch = KdGetChar ();
	  XmitChecksum += HexValue (ch);

	  if (Checksum != XmitChecksum)
	    {
	      KdPutChar ('-');	/* failed checksum */
	    }
	  else
	    {
	      KdPutChar ('+');	/* successful transfer */

	      /* if a sequence char is present, reply the sequence ID */
	      if (Buffer[2] == ':')
		{
		  KdPutChar (Buffer[0]);
		  KdPutChar (Buffer[1]);

		  return &Buffer[3];
		}

	      return &Buffer[0];
	    }
	}
    }
}

/* send the packet in Buffer.  */

VOID
GspPutPacket (PCHAR Buffer)
{
  CHAR Checksum;
  LONG Count;
  CHAR ch;

  /*  $<packet info>#<Checksum>. */
  do
    {
      KdPutChar ('$');
      Checksum = 0;
      Count = 0;

      while ((ch = Buffer[Count]))
				{
				  KdPutChar (ch);
				  Checksum += ch;
				  Count += 1;
				}

      KdPutChar ('#');
      KdPutChar (HexChars[(Checksum >> 4) & 0xf]);
      KdPutChar (HexChars[Checksum & 0xf]);
    }
  while (KdGetChar () != '+');
}


VOID
GspPutPacketNoWait (PCHAR Buffer)
{
  CHAR Checksum;
  LONG Count;
  CHAR ch;

  /*  $<packet info>#<Checksum>. */
  KdPutChar ('$');
  Checksum = 0;
  Count = 0;

  while ((ch = Buffer[Count]))
		{
		  KdPutChar (ch);
		  Checksum += ch;
		  Count += 1;
		}

  KdPutChar ('#');
  KdPutChar (HexChars[(Checksum >> 4) & 0xf]);
  KdPutChar (HexChars[Checksum & 0xf]);
}


VOID
GspDebugError(LPSTR Message)
{
  DbgPrint ("%s\n", Message);
}

/* Address of a routine to RTE to if we get a memory fault.  */
static VOID (*volatile MemoryFaultRoutine) () = NULL;

/* Indicate to caller of GspMem2Hex or GspHex2Mem that there has been an
   error.  */
static volatile BOOLEAN GspMemoryError = FALSE;


/* Currently not used */
VOID
GspSetMemoryError()
{
  GspMemoryError = TRUE;
}


/* These are separate functions so that they are so short and sweet
   that the compiler won't save any Registers (if there is a fault
   to MemoryFaultRoutine, they won't get restored, so there better
   not be any saved).  */
CHAR
GspGetChar (PCHAR Address)
{
  return *Address;
}


VOID
GspSetChar (PCHAR Address,
  CHAR Value)
{
  *Address = Value;
}


/* Convert the memory pointed to by Address into hex, placing result in Buffer */
/* Return a pointer to the last char put in Buffer (null) */
/* If MayFault is TRUE, then we should set GspMemoryError in response to
   a fault; if FALSE treat a fault like any other fault in the stub.  */
PCHAR
GspMem2Hex (PCHAR Address,
  PCHAR Buffer,
  LONG Count,
  BOOLEAN MayFault)
{
  ULONG i;
  CHAR ch;

  if (MayFault)
    MemoryFaultRoutine = GspSetMemoryError;
  for (i = 0; i < (ULONG) Count; i++)
    {
      ch = GspGetChar (Address++);
      if (MayFault && GspMemoryError)
        return (Buffer);
      *Buffer++ = HexChars[(ch >> 4) & 0xf];
      *Buffer++ = HexChars[ch & 0xf];
    }
  *Buffer = 0;
  if (MayFault)
    MemoryFaultRoutine = NULL;
  return (Buffer);
}


/* Convert the hex array pointed to by Buffer into binary to be placed at Address */
/* Return a pointer to the character AFTER the last byte read from Buffer */
PCHAR
GspHex2Mem (PCHAR Buffer,
  PCHAR Address,
  ULONG Count,
  BOOLEAN MayFault)
{
  ULONG i;
  CHAR ch;

  if (MayFault)
    MemoryFaultRoutine = GspSetMemoryError;
  for (i = 0; i < Count; i++)
    {
      ch = HexValue (*Buffer++) << 4;
      ch = ch + HexValue (*Buffer++);
      GspSetChar (Address++, ch);
      if (MayFault && GspMemoryError)
        return (Buffer);
    }
  if (MayFault)
    MemoryFaultRoutine = NULL;
  return (Buffer);
}


/* This function takes the 386 exception vector and attempts to
   translate this number into a unix compatible signal value */
ULONG
GspComputeSignal (NTSTATUS ExceptionCode)
{
  ULONG SigVal;

  switch (ExceptionCode)
    {
    case STATUS_INTEGER_DIVIDE_BY_ZERO:
      SigVal = 8;
      break;			/* divide by zero */
    case STATUS_SINGLE_STEP:
      SigVal = 5;
      break;			/* debug exception */
    case STATUS_BREAKPOINT:
      SigVal = 5;
      break;			/* breakpoint */
    case STATUS_INTEGER_OVERFLOW:
      SigVal = 16;
      break;			/* into instruction (overflow) */
    case STATUS_ARRAY_BOUNDS_EXCEEDED:
      SigVal = 16;
      break;			/* bound instruction */
    case STATUS_ILLEGAL_INSTRUCTION:
      SigVal = 4;
      break;			/* Invalid opcode */
#if 0
    case STATUS_FLT_INVALID_OPERATION:
      SigVal = 8;
      break;			/* coprocessor not available */
#endif
    case STATUS_STACK_OVERFLOW:
      SigVal = 11;
      break;			/* stack exception */
    case STATUS_DATATYPE_MISALIGNMENT:
      SigVal = 11;
      break;			/* page fault */
    default:
      SigVal = 7;		/* "software generated" */
    }
  return (SigVal);
}


/**********************************************/
/* WHILE WE FIND NICE HEX CHARS, BUILD A LONG */
/* RETURN NUMBER OF CHARS PROCESSED           */
/**********************************************/
LONG
GspHex2Long (PCHAR *Address,
  PLONG Value)
{
  LONG NumChars = 0;
  LONG Hex;

  *Value = 0;

  while (**Address)
    {
      Hex = HexValue (**Address);
      if (Hex >= 0)
				{
				  *Value = (*Value << 4) | Hex;
				  NumChars++;
				}
        else
	        break;

      (*Address)++;
    }

  return (NumChars);
}


VOID
GspLong2Hex (PCHAR *Address,
  LONG Value)
{
  LONG Save;

  Save = (((Value >> 0) & 0xf) << 24) |
         (((Value >> 8) & 0xf) << 16) |
         (((Value >> 16) & 0xf) << 8) |
         (((Value >> 24) & 0xf) << 0);
  *Address = GspMem2Hex ((PCHAR) &Save, *Address, 4, FALSE);
}


VOID
GspGetRegistersFromTrapFrame(PCHAR Address,
  PKTRAP_FRAME TrapFrame)
{
  PCPU_REGISTER Regs;
  ULONG Value;
  PCHAR Buffer;
  PULONG p;
  DWORD i;

  Buffer = Address;
  Regs = &GspRegisters[0];
  for (i = 0; i < NUMREGS; i++)
  {
    if (TrapFrame)
    {
      p = (PULONG) ((ULONG_PTR) TrapFrame + Regs[i].OffsetInTF);
      Value = *p;
    }
    else if (i == EIP_REGNO)
		{
      /*
       * This thread has not been sheduled yet so assume it
       * is still in PsBeginThreadWithContextInternal().
       */
      Value = (ULONG) PsBeginThreadWithContextInternal;
		}
    else
		{
      Value = 0;
    }
    Buffer = GspMem2Hex ((PCHAR) &Value, Buffer, Regs[i].Size, FALSE);
  }
}


VOID
GspSetRegistersInTrapFrame(PCHAR Address,
  PKTRAP_FRAME TrapFrame)
{
  PCPU_REGISTER Regs;
  ULONG Value;
  PCHAR Buffer;
  PULONG p;
  DWORD i;

  if (!TrapFrame)
    return;

  Buffer = Address;
  Regs = &GspRegisters[0];
  for (i = 0; i < NUMREGS; i++)
  {
    p = (PULONG) ((ULONG_PTR) TrapFrame + Regs[i].OffsetInTF);
    Value = 0;
    Buffer = GspHex2Mem (Buffer, (PCHAR) &Value, Regs[i].Size, FALSE);
    *p = Value;
  }
}


VOID
GspSetSingleRegisterInTrapFrame(PCHAR Address,
  LONG Number,
  PKTRAP_FRAME TrapFrame)
{
  ULONG Value;
  PULONG p;

  if (!TrapFrame)
    return;

  p = (PULONG) ((ULONG_PTR) TrapFrame + GspRegisters[Number].OffsetInTF);
  Value = 0;
  GspHex2Mem (Address, (PCHAR) &Value, GspRegisters[Number].Size, FALSE);
  *p = Value;
}


BOOLEAN
GspFindThread(PCHAR Data,
  PETHREAD *Thread)
{
  PETHREAD ThreadInfo = NULL;

  if (strcmp (Data, "-1") == 0)
    {
      /* All threads */
      ThreadInfo = NULL;
    }
    else if (strcmp (Data, "0") == 0)
    {
       /* Pick any thread, pick the first thread,
        * which is what most people are interested in
        */
       ThreadInfo = CONTAINING_RECORD (PiThreadListHead.Flink,
         ETHREAD, Tcb.ThreadListEntry);
    }
    else
    {
      ULONG ThreadId;
      PCHAR ptr = &Data[0];

      GspHex2Long (&ptr, (PLONG) &ThreadId);

      if (!NT_SUCCESS (PsLookupThreadByThreadId ((PVOID) ThreadId, &ThreadInfo)))
			  {
          *Thread = NULL;
          return FALSE;
        }
    }
  *Thread = ThreadInfo;
  return TRUE;
}


VOID
GspSetThread(PCHAR Request)
{
  PETHREAD ThreadInfo;
  PCHAR ptr = &Request[1];

  switch (Request[0])
  {
    case 'c': /* Run thread */
      if (GspFindThread (ptr, &ThreadInfo))
			  {
			    GspOutBuffer[0] = 'O';
			    GspOutBuffer[1] = 'K';
	        GspRunThread = ThreadInfo;
			  }
			  else
			  {
			    GspOutBuffer[0] = 'E';
			  }
      break;
    case 'g': /* Debug thread */
      if (GspFindThread (ptr, &ThreadInfo))
			  {
			    GspOutBuffer[0] = 'O';
			    GspOutBuffer[1] = 'K';
	        GspDbgThread = ThreadInfo;
			  }
			  else
			  {
			    GspOutBuffer[0] = 'E';
			  }
      break;
    default:
      break;
  }
}


VOID
GspQuery(PCHAR Request)
{
  PCHAR Command;
  ULONG Value;

	Command = strtok (Request, ",");
	if (strncmp (Command, "C", 1) == 0)
  {
    PCHAR ptr = &GspOutBuffer[2];

    /* Get current thread id */
    GspOutBuffer[0] = 'Q';
    GspOutBuffer[1] = 'C';
    Value = (ULONG) GspDbgThread->Cid.UniqueThread;
    GspLong2Hex (&ptr, Value);
  }
  else if (strncmp (Command, "fThreadInfo", 11) == 0)
  {
    PCHAR ptr = &GspOutBuffer[1];

    /* Get first thread id */
    GspOutBuffer[0] = 'm';
    GspEnumThread = CONTAINING_RECORD (PiThreadListHead.Flink,
      ETHREAD, Tcb.ThreadListEntry);
    Value = (ULONG) GspEnumThread->Cid.UniqueThread;
    GspLong2Hex (&ptr, Value);
  }
  else if (strncmp (Command, "sThreadInfo", 11) == 0)
  {
    PCHAR ptr = &GspOutBuffer[1];

    /* Get next thread id */
    if ((GspEnumThread) && (GspEnumThread->Tcb.ThreadListEntry.Flink != PiThreadListHead.Flink))
    {
      GspEnumThread = CONTAINING_RECORD (GspEnumThread->Tcb.ThreadListEntry.Flink,
        ETHREAD, Tcb.ThreadListEntry);
	    GspOutBuffer[0] = 'm';
	    Value = (ULONG) GspEnumThread->Cid.UniqueThread;
      GspLong2Hex (&ptr, Value);
    }
		else
		{
	    GspOutBuffer[0] = '1';
		}
  }
  else if (strncmp (Command, "ThreadExtraInfo", 15) == 0)
  {
    PETHREAD ThreadInfo;
    PCHAR ptr = &Command[15];

    /* Get thread information */
    if (GspFindThread (ptr, &ThreadInfo))
	  {
      PCHAR String = GspThreadStates[ThreadInfo->Tcb.State];
      GspMem2Hex (String, &GspOutBuffer[0], strlen (String), FALSE);
	  }
  }
#if 0
	else if (strncmp (Command, "L", 1) == 0)
  {
    PLIST_ENTRY CurrentEntry;
    PETHREAD Current;
    ULONG MaxThreads = 0;
    ULONG ThreadId = 0;
    ULONG ThreadCount = 0;

    /* List threads */
    GspHex2Mem (&Request[1], (PCHAR) &MaxThreads, 2, TRUE);
    GspHex2Mem (&Request[3], (PCHAR) &Value, 4, TRUE);
    GspHex2Mem (&Request[11], (PCHAR) &ThreadId, 4, TRUE);

    GspOutBuffer[0] = 'q';
    GspOutBuffer[1] = 'M';
    Value = 0;
    GspMem2Hex ((PCHAR) &Value, &GspOutBuffer[5], 4, TRUE);
    GspMem2Hex ((PCHAR) &ThreadId, &GspOutBuffer[13], 4, TRUE);

    CurrentEntry = PiThreadListHead.Flink;
    while ((CurrentEntry != &PiThreadListHead) && (ThreadCount < MaxThreads))
      {
        Current = CONTAINING_RECORD (CurrentEntry, ETHREAD, Tcb.ThreadListEntry);
        Value = 0;
        GspMem2Hex ((PCHAR) &Value, &GspOutBuffer[21+ThreadCount*16], 4, TRUE);
        Value = (ULONG) Current->Cid.UniqueThread;
        GspMem2Hex ((PCHAR) &Value, &GspOutBuffer[21+ThreadCount*16+8], 4, TRUE);
        CurrentEntry = CurrentEntry->Flink;
        ThreadCount++;
      }

    if (CurrentEntry != &PiThreadListHead)
		{
      GspOutBuffer[4] = '0';
		}
    else
    {
      GspOutBuffer[4] = '1';
    }

    GspMem2Hex ((PCHAR) &ThreadCount, &GspOutBuffer[2], 1, TRUE);
  }
#endif
  else if (strncmp (Command, "Offsets", 7) == 0)
  {
    strcpy (GspOutBuffer, "Text=0;Data=0;Bss=0");
  }
}

VOID
GspQueryThreadStatus(PCHAR Request)
{
  PETHREAD ThreadInfo;
  PCHAR ptr = &Request[0];

  if (GspFindThread (ptr, &ThreadInfo))
  {
    GspOutBuffer[0] = 'O';
    GspOutBuffer[1] = 'K';
    GspOutBuffer[2] = '\0';
  }
  else
  {
    GspOutBuffer[0] = 'E';
    GspOutBuffer[1] = '\0';
  }
}


typedef struct _GsHwBreakPoint
{
  BOOLEAN Enabled;
  ULONG Type;
  ULONG Length;
  ULONG Address;
} GsHwBreakPoint;

GsHwBreakPoint GspBreakpoints[4] =
{
  { Enabled : FALSE },
  { Enabled : FALSE },
  { Enabled : FALSE },
  { Enabled : FALSE }
};

VOID
GspCorrectHwBreakpoint()
{
  ULONG BreakpointNumber;
  BOOLEAN CorrectIt;
  BOOLEAN Bit;
  ULONG dr7;

  asm volatile (
    "movl %%db7, %0\n" : "=r" (dr7) : );
  do
    {
      ULONG addr0, addr1, addr2, addr3;

      asm volatile (
        "movl %%db0, %0\n"
        "movl %%db1, %1\n"
        "movl %%db2, %2\n"
        "movl %%db3, %3\n"
          : "=r" (addr0), "=r" (addr1),
            "=r" (addr2), "=r" (addr3) : );
    } while (FALSE);
    CorrectIt = FALSE;
    for (BreakpointNumber = 0; BreakpointNumber < 3; BreakpointNumber++)
    {
			Bit = 2 << (BreakpointNumber << 1);
			if (!(dr7 & Bit) && GspBreakpoints[BreakpointNumber].Enabled) {
		  CorrectIt = TRUE;
			dr7 |= Bit;
			dr7 &= ~(0xf0000 << (BreakpointNumber << 2));
			dr7 |= (((GspBreakpoints[BreakpointNumber].Length << 2) |
        GspBreakpoints[BreakpointNumber].Type) << 16) << (BreakpointNumber << 2);
    switch (BreakpointNumber) {
			case 0:
			  asm volatile ("movl %0, %%dr0\n"
			    : : "r" (GspBreakpoints[BreakpointNumber].Address) );
			  break;

			case 1:
				asm volatile ("movl %0, %%dr1\n"
				  : : "r" (GspBreakpoints[BreakpointNumber].Address) );
				break;

			case 2:
				asm volatile ("movl %0, %%dr2\n"
  				: : "r" (GspBreakpoints[BreakpointNumber].Address) );
  			break;

      case 3:
        asm volatile ("movl %0, %%dr3\n"
          : : "r" (GspBreakpoints[BreakpointNumber].Address) );
        break;
      }
    }
    else if ((dr7 & Bit) && !GspBreakpoints[BreakpointNumber].Enabled)
      {
        CorrectIt = TRUE;
        dr7 &= ~Bit;
        dr7 &= ~(0xf0000 << (BreakpointNumber << 2));
      }
  }
  if (CorrectIt)
    {
	    asm volatile ( "movl %0, %%db7\n" : : "r" (dr7));
    }
}

ULONG
GspRemoveHwBreakpoint(ULONG BreakpointNumber)
{
	if (!GspBreakpoints[BreakpointNumber].Enabled)
    {
	    return -1;
  	}
	GspBreakpoints[BreakpointNumber].Enabled = 0;
	return 0;
}


ULONG
GspSetHwBreakpoint(ULONG BreakpointNumber,
  ULONG Type,
  ULONG Length,
  ULONG Address)
{
	if (GspBreakpoints[BreakpointNumber].Enabled)
	  {
	  	return -1;
		}
	GspBreakpoints[BreakpointNumber].Enabled = TRUE;
	GspBreakpoints[BreakpointNumber].Type = Type;
	GspBreakpoints[BreakpointNumber].Length = Length;
	GspBreakpoints[BreakpointNumber].Address = Address;
	return 0;
}


/*
 * This function does all command procesing for interfacing to gdb.
 */
KD_CONTINUE_TYPE
KdEnterDebuggerException(PEXCEPTION_RECORD ExceptionRecord,
			 PCONTEXT Context,
			 PKTRAP_FRAME TrapFrame)
{
  BOOLEAN Stepping;
  LONG Address;
  LONG Length;
  LONG SigVal;
  LONG NewPC;
  PCHAR ptr;

  /* FIXME: Stop on other CPUs too */
  /* Disable hardware debugging while we are inside the stub */
  __asm__("movl %0,%%db7" : /* no output */ : "r" (0));

  /* reply to host that an exception has occurred */
  SigVal = GspComputeSignal (ExceptionRecord->ExceptionCode);

  ptr = &GspOutBuffer[0];

  *ptr++ = 'T';			/* notify gdb with signo, PC, FP and SP */
  *ptr++ = HexChars[(SigVal >> 4) & 0xf];
  *ptr++ = HexChars[SigVal & 0xf];

  *ptr++ = HexChars[ESP];
  *ptr++ = ':';
  ptr = GspMem2Hex ((PCHAR) &TrapFrame->Esp, ptr, 4, 0);	/* SP */
  *ptr++ = ';';

  *ptr++ = HexChars[EBP];
  *ptr++ = ':';
  ptr = GspMem2Hex ((PCHAR) &TrapFrame->Ebp, ptr, 4, 0); 	/* FP */
  *ptr++ = ';';

  *ptr++ = HexChars[PC];
  *ptr++ = ':';
  ptr = GspMem2Hex((PCHAR) &TrapFrame->Eip, ptr, 4, 0); 	/* PC */
  *ptr++ = ';';

  *ptr = '\0';

  GspPutPacket (&GspOutBuffer[0]);

  Stepping = FALSE;

  while (TRUE)
    {
      /* Zero the buffer now so we don't have to worry about the terminating zero character */
      memset (GspOutBuffer, 0, sizeof (GspInBuffer));
      ptr = GspGetPacket ();

      switch (*ptr++)
	{
	case '?':
	  GspOutBuffer[0] = 'S';
	  GspOutBuffer[1] = HexChars[SigVal >> 4];
	  GspOutBuffer[2] = HexChars[SigVal % 16];
	  GspOutBuffer[3] = 0;
	  break;
	case 'd':
	  GspRemoteDebug = !GspRemoteDebug; /* toggle debug flag */
	  break;
	case 'g':		/* return the value of the CPU Registers */
    if (GspDbgThread)
      GspGetRegistersFromTrapFrame (&GspOutBuffer[0], GspDbgThread->Tcb.TrapFrame);
    else
      GspGetRegistersFromTrapFrame (&GspOutBuffer[0], TrapFrame);
	  break;
	case 'G':		/* set the value of the CPU Registers - return OK */
    if (GspDbgThread)
      GspSetRegistersInTrapFrame (ptr, GspDbgThread->Tcb.TrapFrame);
    else
      GspSetRegistersInTrapFrame (ptr, TrapFrame);
	  strcpy (GspOutBuffer, "OK");
	  break;
	case 'P':		/* set the value of a single CPU register - return OK */
	  {
	    LONG Register;

	    if ((GspHex2Long (&ptr, &Register)) && (*ptr++ == '='))
	      if ((Register >= 0) && (Register < NUMREGS))
					{
				    if (GspDbgThread)
				      GspSetSingleRegisterInTrapFrame (ptr, Register,
                GspDbgThread->Tcb.TrapFrame);
				    else
				       GspSetSingleRegisterInTrapFrame (ptr, Register, TrapFrame);
					  strcpy (GspOutBuffer, "OK");
					  break;
					}

	    strcpy (GspOutBuffer, "E01");
	    break;
	  }

	  /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
	case 'm':
	  /* TRY TO READ %x,%x.  IF SUCCEED, SET PTR = 0 */
	  if (GspHex2Long (&ptr, &Address))
	    if (*(ptr++) == ',')
	      if (GspHex2Long (&ptr, &Length))
		{
		  ptr = 0;
		  GspMemoryError = FALSE;
		  GspMem2Hex ((PCHAR) Address, GspOutBuffer, Length, 1);
		  if (GspMemoryError)
		    {
		      strcpy (GspOutBuffer, "E03");
		      GspDebugError ("memory fault");
		    }
		}

	  if (ptr)
	    {
	      strcpy (GspOutBuffer, "E01");
	    }
	  break;

	  /* MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK */
	case 'M':
	  /* TRY TO READ '%x,%x:'.  IF SUCCEED, SET PTR = 0 */
	  if (GspHex2Long (&ptr, &Address))
	    if (*(ptr++) == ',')
	      if (GspHex2Long (&ptr, &Length))
		if (*(ptr++) == ':')
		  {
        GspMemoryError = FALSE;
		    GspHex2Mem (ptr, (PCHAR) Address, Length, TRUE);

		    if (GspMemoryError)
		      {
						strcpy (GspOutBuffer, "E03");
						GspDebugError ("memory fault");
		      }
		    else
		      {
			      strcpy (GspOutBuffer, "OK");
		      }

		    ptr = NULL;
		  }
	  if (ptr)
	    {
	      strcpy (GspOutBuffer, "E02");
	    }
	  break;

	  /* cAA..AA   Continue at address AA..AA(optional) */
	  /* sAA..AA   Step one instruction from AA..AA(optional) */
	case 's':
	  Stepping = TRUE;
	case 'c':
  {
    ULONG BreakpointNumber;
    ULONG dr6;

	  /* try to read optional parameter, pc unchanged if no parm */
	  if (GspHex2Long (&ptr, &Address))
	    Context->Eip = Address;

	  NewPC = Context->Eip;

	  /* clear the trace bit */
	  Context->EFlags &= 0xfffffeff;

	  /* set the trace bit if we're Stepping */
	  if (Stepping)
	    Context->EFlags |= 0x100;

    asm volatile ("movl %%db6, %0\n" : "=r" (dr6) : );
    if (!(dr6 & 0x4000))
	    {
	      for (BreakpointNumber = 0; BreakpointNumber < 4; ++BreakpointNumber)
	        {
	          if (dr6 & (1 << BreakpointNumber))
	            {
	              if (GspBreakpoints[BreakpointNumber].Type == 0)
	                {
	                  /* Set restore flag */
	                  Context->EFlags |= 0x10000;
	                  break;
	                }
	            }
	        }
      }
    GspCorrectHwBreakpoint();
    asm volatile ("movl %0, %%db6\n" : : "r" (0));

	  return kdHandleException;
	  break;
  }
	case 'k':	/* kill the program */
    strcpy (GspOutBuffer, "OK");
	  break;
	  /* kill the program */
	case 'H':		/* Set thread */
    GspSetThread (ptr);
	  break;
  case 'q': /* Query */
    GspQuery (ptr);
    break;
  case 'T': /* Query thread status */
    GspQueryThreadStatus (ptr);
    break;
  case 'Y':
  {
    ULONG Number;
    ULONG Length;
    ULONG Type;
    ULONG Address;

    ptr = &GspOutBuffer[1];
    GspHex2Long (&ptr, &Number);
    ptr++;
    GspHex2Long (&ptr, &Type);
    ptr++;
    GspHex2Long (&ptr, &Length);
    ptr++;
    GspHex2Long (&ptr, &Address);
    if (GspSetHwBreakpoint (Number & 0x3, Type & 0x3 , Length & 0x3, Address) == 0)
      {
        strcpy (GspOutBuffer, "OK");
      }
    else
      {
        strcpy (GspOutBuffer, "E");
      }
    break;
    /* Remove hardware breakpoint */
  }
  case 'y':
  {
    ULONG Number;

    ptr = &GspOutBuffer[1];
    GspHex2Long(&ptr, &Number);
    if (GspRemoveHwBreakpoint (Number & 0x3) == 0)
      {
        strcpy (GspOutBuffer, "OK");
      }
    else
      {
        strcpy (GspOutBuffer, "E");
      }
    break;
  }
  default:
    break;
	}			/* switch */

      /* reply to the request */
      GspPutPacket (&GspOutBuffer[0]);
    }

  return kdDoNotHandleException;
}


BOOLEAN
STDCALL
GspBreakIn(PKINTERRUPT Interrupt,
  PVOID ServiceContext)
{
  PKTRAP_FRAME TrapFrame;
  BOOLEAN DoBreakIn;
  CONTEXT Context;
  KIRQL OldIrql;
  CHAR Value;

  DPRINT ("Break In\n");

  DoBreakIn = FALSE;
  while (KdPortGetByteEx (&GdbPortInfo, &Value))
    {
      if (Value == 0x03)
        DoBreakIn = TRUE;
    }

  if (!DoBreakIn)
    return TRUE;

  KeRaiseIrql (HIGH_LEVEL, &OldIrql);

  TrapFrame = PsGetCurrentThread()->Tcb.TrapFrame;

  KeTrapFrameToContext (TrapFrame, &Context);

  KdEnterDebuggerException (NULL, &Context, TrapFrame);

  KeContextToTrapFrame (&Context, TrapFrame);

  KeLowerIrql (OldIrql);

  return TRUE;
}


extern ULONG KdpPortIrq;

/* Initialize the GDB stub */
VOID
KdGdbStubInit(ULONG Phase)
{
#if 0
  KAFFINITY Affinity;
  NTSTATUS Status;
  ULONG MappedIrq;
  KIRQL Dirql;
#endif

  if (Phase == 0)
    {
		  DbgPrint("Module 'hal.dll' loaded at 0x%.08x.\n", LdrHalBase);

		  GspInitialized = TRUE;
		  GspRunThread = PsGetCurrentThread();
		  GspDbgThread = PsGetCurrentThread();
		  GspEnumThread = NULL;

      DbgBreakPointWithStatus (DBG_STATUS_CONTROL_C);
    }
  else if (Phase == 1)
    {
#if 0
		  /* Hook an interrupt handler to allow the debugger to break into
		     the system */
		  MappedIrq = HalGetInterruptVector (Internal,
		    0,
		    0,
		    KdpPortIrq,
		    &Dirql,
		    &Affinity);

		  Status = IoConnectInterrupt(&GspInterrupt,
		    GspBreakIn,
		    NULL,
		    NULL,
		    MappedIrq,
		    Dirql,
		    Dirql,
		    0,
		    FALSE,
		    Affinity,
		    FALSE);
      if (!NT_SUCCESS (Status))
      {
        DPRINT1("Could not connect to IRQ line %d (0x%x)\n",
          KdpPortIrq, Status);
        return;
      }

       KdPortEnableInterrupts();

       DbgBreakPointWithStatus (DBG_STATUS_CONTROL_C);
#endif
  }
}


VOID
KdGdbDebugPrint(LPSTR Message)
{
#if 0
  /* This can be quite annoying! */
  if (GspInitialized)
	  {
	    ULONG Length;
	
	    GspOutBuffer[0] = 'O';
	    GspOutBuffer[1] = '\0';
	    strcat (&GspOutBuffer[0], Message);
	    Length = strlen (Message);
	    GspOutBuffer[2 + Length] = '\n';
	    GspOutBuffer[3 + Length] = '\0';
	    GspPutPacketNoWait (&GspOutBuffer[0]);
	  }
#else
  HalDisplayString(Message);
#endif
}


extern LIST_ENTRY ModuleListHead;

VOID
KdGdbListModules()
{
  PLIST_ENTRY CurrentEntry;
  PMODULE_OBJECT Current;
  ULONG ModuleCount;

  DPRINT1("\n");

  ModuleCount = 0;

  CurrentEntry = ModuleListHead.Flink;
  while (CurrentEntry != (&ModuleListHead))
    {
	    Current = CONTAINING_RECORD (CurrentEntry, MODULE_OBJECT, ListEntry);

      DbgPrint ("Module %S  Base 0x%.08x  Length 0x%.08x\n",
        Current->BaseName.Buffer, Current->Base, Current->Length);

      ModuleCount++;
      CurrentEntry = CurrentEntry->Flink;
    }

  DbgPrint ("%d modules listed\n", ModuleCount);
}
