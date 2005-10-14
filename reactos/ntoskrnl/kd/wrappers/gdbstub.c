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

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>
#include <internal/ps.h>

/************************************************************************/
/* BUFMAX defines the maximum number of characters in inbound/outbound buffers*/
/* at least NUMREGBYTES*2 are needed for register packets */
#define BUFMAX 1000

static BOOLEAN GspInitialized;

static BOOLEAN GspRemoteDebug;

static CONST CHAR HexChars[]="0123456789abcdef";

static PETHREAD GspRunThread; /* NULL means run all threads */
static PETHREAD GspDbgThread;
static PETHREAD GspEnumThread;

extern LIST_ENTRY PsActiveProcessHead;
KD_PORT_INFORMATION GdbPortInfo = { 2, 115200, 0 }; /* FIXME hardcoded for COM2, 115200 baud */

/* Number of Registers.  */
#define NUMREGS 16

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
  DWORD OffsetInContext;
  BOOLEAN SetInContext;
} CPU_REGISTER, *PCPU_REGISTER;

#define EIP_REGNO 8

static CPU_REGISTER GspRegisters[NUMREGS] =
{
  { 4, FIELD_OFFSET(KTRAP_FRAME, Eax), FIELD_OFFSET(CONTEXT, Eax), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, Ecx), FIELD_OFFSET(CONTEXT, Ecx), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, Edx), FIELD_OFFSET(CONTEXT, Edx), FALSE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, Ebx), FIELD_OFFSET(CONTEXT, Ebx), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, Esp), FIELD_OFFSET(CONTEXT, Esp), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, DebugEbp), FIELD_OFFSET(CONTEXT, Ebp), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, Esi), FIELD_OFFSET(CONTEXT, Esi), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, Edi), FIELD_OFFSET(CONTEXT, Edi), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, DebugEip), FIELD_OFFSET(CONTEXT, Eip), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, Eflags), FIELD_OFFSET(CONTEXT, EFlags), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, Cs), FIELD_OFFSET(CONTEXT, SegCs), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, Ss), FIELD_OFFSET(CONTEXT, SegSs), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, Ds), FIELD_OFFSET(CONTEXT, SegDs), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, Es), FIELD_OFFSET(CONTEXT, SegEs), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, Fs), FIELD_OFFSET(CONTEXT, SegFs), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, Gs), FIELD_OFFSET(CONTEXT, SegGs), TRUE }
};

static PCHAR GspThreadStates[DeferredReady+1] =
{
  "Initialized",
  "Ready",
  "Running",
  "Standby",
  "Terminated",
  "Waiting",
  "Transition",
  "DeferredReady"
};


LONG
HexValue(CHAR ch)
{
  if ((ch >= '0') && (ch <= '9'))
    {
      return (ch - '0');
    }
  if ((ch >= 'a') && (ch <= 'f'))
    {
      return (ch - 'a' + 10);
    }
  if ((ch >= 'A') && (ch <= 'F'))
    {
      return (ch - 'A' + 10);
    }

  return -1;
}

static CHAR GspInBuffer[BUFMAX];
static CHAR GspOutBuffer[BUFMAX];

VOID
GdbPutChar(UCHAR Value)
{
  KdPortPutByteEx(&GdbPortInfo, Value);
}

UCHAR
GdbGetChar(VOID)
{
  UCHAR Value;

  while (!KdPortGetByteEx(&GdbPortInfo, &Value))
    ;

  return Value;
}

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
      while ((ch = GdbGetChar ()) != '$')
        ;

    retry:
      Checksum = 0;
      XmitChecksum = -1;
      Count = 0;

      /* now, read until a # or end of Buffer is found */
      while (Count < BUFMAX)
        {
          ch = GdbGetChar();
          if (ch == '$')
            {
              goto retry;
            }
          if (ch == '#')
            {
              break;
            }
          Checksum = Checksum + ch;
          Buffer[Count] = ch;
          Count = Count + 1;
        }
      Buffer[Count] = 0;

      if (ch == '#')
        {
          ch = GdbGetChar();
          XmitChecksum = (CHAR)(HexValue(ch) << 4);
          ch = GdbGetChar();
          XmitChecksum += (CHAR)(HexValue(ch));

          if (Checksum != XmitChecksum)
            {
              GdbPutChar('-'); /* failed checksum */
            }
          else
            {
              GdbPutChar('+'); /* successful transfer */

              return &Buffer[0];
            }
        }
    }
}

/* send the packet in Buffer.  */

VOID
GspPutPacket(PCHAR Buffer)
{
  CHAR Checksum;
  LONG Count;
  CHAR ch;

  /*  $<packet info>#<Checksum>. */
  do
    {
      GdbPutChar('$');
      Checksum = 0;
      Count = 0;

      while ((ch = Buffer[Count]))
        {
          GdbPutChar(ch);
          Checksum += ch;
          Count += 1;
        }

      GdbPutChar('#');
      GdbPutChar(HexChars[(Checksum >> 4) & 0xf]);
      GdbPutChar(HexChars[Checksum & 0xf]);
    }
  while (GdbGetChar() != '+');
}


VOID
GspPutPacketNoWait(PCHAR Buffer)
{
  CHAR Checksum;
  LONG Count;
  CHAR ch;

  /*  $<packet info>#<Checksum>. */
  GdbPutChar('$');
  Checksum = 0;
  Count = 0;

  while ((ch = Buffer[Count]))
    {
      GdbPutChar(ch);
      Checksum += ch;
      Count += 1;
    }

  GdbPutChar('#');
  GdbPutChar(HexChars[(Checksum >> 4) & 0xf]);
  GdbPutChar(HexChars[Checksum & 0xf]);
}

/* Indicate to caller of GspMem2Hex or GspHex2Mem that there has been an
   error.  */
static volatile BOOLEAN GspMemoryError = FALSE;
static volatile void *GspAccessLocation = NULL;


/* Convert the memory pointed to by Address into hex, placing result in Buffer */
/* Return a pointer to the last char put in Buffer (null) */
/* If MayFault is TRUE, then we should set GspMemoryError in response to
   a fault; if FALSE treat a fault like any other fault in the stub.  */
PCHAR
GspMem2Hex(PCHAR Address,
  PCHAR Buffer,
  LONG Count,
  BOOLEAN MayFault)
{
  ULONG i;
  CHAR ch;

  if (NULL == Address && MayFault)
    {
      GspMemoryError = TRUE;
      return Buffer;
    }

  for (i = 0; i < (ULONG) Count; i++)
    {
      if (MayFault)
        {
          GspAccessLocation = Address;
        }
      ch = *Address;
      GspAccessLocation = NULL;
      if (MayFault && GspMemoryError)
        {
          return Buffer;
        }
      *Buffer++ = HexChars[(ch >> 4) & 0xf];
      *Buffer++ = HexChars[ch & 0xf];
      Address++;
    }
  *Buffer = 0;
  return Buffer;
}


/* Convert the hex array pointed to by Buffer into binary to be placed at Address */
/* Return a pointer to the character AFTER the last byte read from Buffer */
PCHAR
GspHex2Mem(PCHAR Buffer,
  PCHAR Address,
  ULONG Count,
  BOOLEAN MayFault)
{
  PCHAR current;
  PCHAR page;
  ULONG countinpage;
  ULONG i;
  CHAR ch;
  ULONG oldprot = 0;

  current = Address;
  while ( current < Address + Count )
    {
      page = (PCHAR)PAGE_ROUND_DOWN(current);
      if (Address + Count <= page + PAGE_SIZE)
        {
          /* Fits in this page */
          countinpage = Count;
        }
      else
        {
          /* Flows into next page, handle only current page in this iteration */
          countinpage = PAGE_SIZE - (Address - page);
        }
      if (MayFault)
        {
          oldprot = MmGetPageProtect(NULL, Address);
          MmSetPageProtect(NULL, Address, PAGE_EXECUTE_READWRITE);
        }

      for (i = 0; i < countinpage && ! GspMemoryError; i++)
        {
          ch = (CHAR)(HexValue(*Buffer++) << 4);
          ch = (CHAR)(ch + HexValue(*Buffer++));

          GspAccessLocation = current;
          *current = ch;
          GspAccessLocation = NULL;
          current++;
        }
      if (MayFault)
        {
          MmSetPageProtect(NULL, page, oldprot);
          if (GspMemoryError)
            {
              return Buffer;
            }
        }
    }

  return Buffer;
}


/* This function takes the 386 exception vector and attempts to
   translate this number into a unix compatible signal value */
ULONG
GspComputeSignal(NTSTATUS ExceptionCode)
{
  ULONG SigVal;

  switch (ExceptionCode)
    {
    case STATUS_INTEGER_DIVIDE_BY_ZERO:
      SigVal = 8; /* divide by zero */
      break;
    case STATUS_SINGLE_STEP:
    case STATUS_BREAKPOINT:
      SigVal = 5; /* breakpoint */
      break;
    case STATUS_INTEGER_OVERFLOW:
    case STATUS_ARRAY_BOUNDS_EXCEEDED:
      SigVal = 16; /* bound instruction */
      break;
    case STATUS_ILLEGAL_INSTRUCTION:
      SigVal = 4; /* Invalid opcode */
      break;
    case STATUS_STACK_OVERFLOW:
    case STATUS_DATATYPE_MISALIGNMENT:
    case STATUS_ACCESS_VIOLATION:
      SigVal = 11; /* access violation */
      break;
    default:
      SigVal = 7; /* "software generated" */
    }
  return SigVal;
}


/**********************************************/
/* WHILE WE FIND NICE HEX CHARS, BUILD A LONG */
/* RETURN NUMBER OF CHARS PROCESSED           */
/**********************************************/
LONG
GspHex2Long(PCHAR *Address,
  PLONG Value)
{
  LONG NumChars = 0;
  LONG Hex;

  *Value = 0;

  while (**Address)
    {
      Hex = HexValue(**Address);
      if (Hex >= 0)
        {
          *Value = (*Value << 4) | Hex;
          NumChars++;
        }
      else
        {
          break;
        }

      (*Address)++;
    }

  return NumChars;
}


VOID
GspLong2Hex(PCHAR *Address,
  LONG Value)
{
  LONG Save;

  Save = (((Value >> 0) & 0xff) << 24) |
         (((Value >> 8) & 0xff) << 16) |
         (((Value >> 16) & 0xff) << 8) |
         (((Value >> 24) & 0xff) << 0);
  *Address = GspMem2Hex((PCHAR) &Save, *Address, 4, FALSE);
}


/*
 * When coming from kernel mode, Esp is not stored in the trap frame.
 * Instead, it was pointing to the location of the TrapFrame Esp member
 * when the exception occured. When coming from user mode, Esp is just
 * stored in the TrapFrame Esp member.
 */
static LONG
GspGetEspFromTrapFrame(PKTRAP_FRAME TrapFrame)
{
  return KeGetPreviousMode() == KernelMode
         ? (LONG) &TrapFrame->Esp : (LONG)TrapFrame->Esp;
}


VOID
GspGetRegistersFromTrapFrame(PCHAR Address,
  PCONTEXT Context,
  PKTRAP_FRAME TrapFrame)
{
  ULONG Value;
  PCHAR Buffer;
  PULONG p;
  DWORD i;

  Buffer = Address;
  for (i = 0; i < sizeof(GspRegisters) / sizeof(GspRegisters[0]); i++)
    {
      if (TrapFrame)
        {
          if (ESP == i)
            {
              Value = GspGetEspFromTrapFrame(TrapFrame);
            }
          else
            {
              p = (PULONG)((ULONG_PTR) TrapFrame + GspRegisters[i].OffsetInTF);
              Value = *p;
            }
        }
      else if (i == EIP_REGNO)
        {
          /*
           * This thread has not been sheduled yet so assume it
           * is still in PsBeginThreadWithContextInternal().
           */
          Value = (ULONG)KiThreadStartup;
        }
      else
        {
          Value = 0;
        }
      Buffer = GspMem2Hex((PCHAR) &Value, Buffer, GspRegisters[i].Size, FALSE);
    }
}


VOID
GspSetRegistersInTrapFrame(PCHAR Address,
  PCONTEXT Context,
  PKTRAP_FRAME TrapFrame)
{
  ULONG Value;
  PCHAR Buffer;
  PULONG p;
  DWORD i;

  if (!TrapFrame)
    {
      return;
    }

  Buffer = Address;
  for (i = 0; i < NUMREGS; i++)
    {
      if (GspRegisters[i].SetInContext)
        {
          p = (PULONG) ((ULONG_PTR) Context + GspRegisters[i].OffsetInContext);
        }
      else
        {
          p = (PULONG) ((ULONG_PTR) TrapFrame + GspRegisters[i].OffsetInTF);
        }
      Value = 0;
      Buffer = GspHex2Mem(Buffer, (PCHAR) &Value, GspRegisters[i].Size, FALSE);
      *p = Value;
    }
}


VOID
GspSetSingleRegisterInTrapFrame(PCHAR Address,
  LONG Number,
  PCONTEXT Context,
  PKTRAP_FRAME TrapFrame)
{
  ULONG Value;
  PULONG p;

  if (!TrapFrame)
    {
      return;
    }

  if (GspRegisters[Number].SetInContext)
    {
      p = (PULONG) ((ULONG_PTR) Context + GspRegisters[Number].OffsetInContext);
    }
  else
    {
      p = (PULONG) ((ULONG_PTR) TrapFrame + GspRegisters[Number].OffsetInTF);
    }
  Value = 0;
  GspHex2Mem(Address, (PCHAR) &Value, GspRegisters[Number].Size, FALSE);
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
  else
    {
      ULONG uThreadId;
      HANDLE ThreadId;
      PCHAR ptr = &Data[0];

      GspHex2Long(&ptr, (PLONG) &uThreadId);
      ThreadId = (HANDLE)uThreadId;

      if (!NT_SUCCESS(PsLookupThreadByThreadId(ThreadId, &ThreadInfo)))
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
      if (GspFindThread(ptr, &ThreadInfo))
        {
          GspOutBuffer[0] = 'O';
          GspOutBuffer[1] = 'K';

          if (NULL != GspRunThread)
            {
              ObDereferenceObject(GspRunThread);
            }
          GspRunThread = ThreadInfo;
          if (NULL != GspRunThread)
            {
              ObReferenceObject(GspRunThread);
            }
        }
      else
        {
          GspOutBuffer[0] = 'E';
        }
      break;
    case 'g': /* Debug thread */
      if (GspFindThread(ptr, &ThreadInfo))
        {
          GspOutBuffer[0] = 'O';
          GspOutBuffer[1] = 'K';

          if (NULL != GspDbgThread)
            {
              ObDereferenceObject(GspDbgThread);
            }

          if (ThreadInfo == PsGetCurrentThread())
            {
              GspDbgThread = NULL;
              ObDereferenceObject(ThreadInfo);
            }
          else
            {
              GspDbgThread = ThreadInfo;
            }
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
  ULONG Value;

  if (strncmp(Request, "C", 1) == 0)
    {
      PCHAR ptr = &GspOutBuffer[2];

      /* Get current thread id */
      GspOutBuffer[0] = 'Q';
      GspOutBuffer[1] = 'C';
      if (NULL != GspDbgThread)
        {
          Value = (ULONG) GspDbgThread->Cid.UniqueThread;
        }
      else
        {
          Value = (ULONG) PsGetCurrentThread()->Cid.UniqueThread;
        }
      GspLong2Hex(&ptr, Value);
    }
  else if (strncmp(Request, "fThreadInfo", 11) == 0)
    {
      PEPROCESS Process;
      PLIST_ENTRY AThread, AProcess;
      PCHAR ptr = &GspOutBuffer[1];

      /* Get first thread id */
      GspEnumThread = NULL;
      AProcess = PsActiveProcessHead.Flink;
      while(AProcess != &PsActiveProcessHead)
        {
          Process = CONTAINING_RECORD(AProcess, EPROCESS, ActiveProcessLinks);
          AThread = Process->ThreadListHead.Flink;
          if (AThread != &Process->ThreadListHead)
            {
              GspEnumThread = CONTAINING_RECORD(Process->ThreadListHead.Flink,
                                                ETHREAD, ThreadListEntry);
              break;
            }
          AProcess = AProcess->Flink;
        }
      if(GspEnumThread != NULL)
        {
          GspOutBuffer[0] = 'm';
          Value = (ULONG) GspEnumThread->Cid.UniqueThread;
          GspLong2Hex(&ptr, Value);
        }
      else
        {
          /* FIXME - what to do here? This case should never happen though, there
                     should always be at least one thread on the system... */
          /* GspOutBuffer[0] = 'l'; */
        }
    }
  else if (strncmp(Request, "sThreadInfo", 11) == 0)
    {
      PEPROCESS Process;
      PLIST_ENTRY AThread, AProcess;
      PCHAR ptr = &GspOutBuffer[1];

      /* Get next thread id */
      if (GspEnumThread != NULL)
        {
          /* find the next thread */
          Process = GspEnumThread->ThreadsProcess;
          if(GspEnumThread->ThreadListEntry.Flink != &Process->ThreadListHead)
            {
              GspEnumThread = CONTAINING_RECORD(GspEnumThread->ThreadListEntry.Flink,
                                                 ETHREAD, ThreadListEntry);
            }
          else
            {
              PETHREAD Thread = NULL;
              AProcess = Process->ActiveProcessLinks.Flink;
              while(AProcess != &PsActiveProcessHead)
                {
                  Process = CONTAINING_RECORD(AProcess, EPROCESS, ActiveProcessLinks);
                  AThread = Process->ThreadListHead.Flink;
                  if (AThread != &Process->ThreadListHead)
                    {
                      Thread = CONTAINING_RECORD(Process->ThreadListHead.Flink,
                                                 ETHREAD, ThreadListEntry);
                      break;
                    }
                  AProcess = AProcess->Flink;
                }
              GspEnumThread = Thread;
            }

          if (GspEnumThread != NULL)
            {
              /* return the ID */
              GspOutBuffer[0] = 'm';
              Value = (ULONG) GspEnumThread->Cid.UniqueThread;
              GspLong2Hex(&ptr, Value);
            }
          else
            {
              GspOutBuffer[0] = 'l';
            }
        }
      else
        {
          GspOutBuffer[0] = 'l';
        }
    }
  else if (strncmp(Request, "ThreadExtraInfo", 15) == 0)
    {
      PETHREAD ThreadInfo;

      /* Get thread information */
      if (GspFindThread(Request + 16, &ThreadInfo))
        {
          char Buffer[64];
          PEPROCESS Proc;

          Proc = (PEPROCESS) ThreadInfo->ThreadsProcess;

          Buffer[0] = '\0';
          if (NULL != Proc )
            {
              sprintf(Buffer, "%s [%d:0x%x], ", Proc->ImageFileName,
                      (int) Proc->UniqueProcessId,
                      (int) ThreadInfo->Cid.UniqueThread);
            }
          strcpy(Buffer + strlen(Buffer),
                 GspThreadStates[ThreadInfo->Tcb.State]);

          ObDereferenceObject(ThreadInfo);

          GspMem2Hex(Buffer, &GspOutBuffer[0], strlen(Buffer), FALSE);
        }
    }
}

VOID
GspQueryThreadStatus(PCHAR Request)
{
  PETHREAD ThreadInfo;
  PCHAR ptr = &Request[0];

  if (GspFindThread(ptr, &ThreadInfo))
    {
      ObDereferenceObject(ThreadInfo);

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

#if defined(__GNUC__)
GsHwBreakPoint GspBreakpoints[4] =
{
  { Enabled : FALSE },
  { Enabled : FALSE },
  { Enabled : FALSE },
  { Enabled : FALSE }
};
#else
GsHwBreakPoint GspBreakpoints[4] =
{
  { FALSE },
  { FALSE },
  { FALSE },
  { FALSE }
};
#endif

VOID
GspCorrectHwBreakpoint()
{
  ULONG BreakpointNumber;
  BOOLEAN CorrectIt;
  BOOLEAN Bit;
  ULONG dr7_;

#if defined(__GNUC__)
  asm volatile (
    "movl %%db7, %0\n" : "=r" (dr7_) : );
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
#elif defined(_MSC_VER)
  __asm
    {
      mov eax, dr7; mov dr7_, eax;
      mov eax, dr0; mov addr0, eax;
      mov eax, dr1; mov addr1, eax;
      mov eax, dr2; mov addr2, eax;
      mov eax, dr3; mov addr3, eax;
    }
#else
#error Unknown compiler for inline assembler
#endif
  CorrectIt = FALSE;
  for (BreakpointNumber = 0; BreakpointNumber < 3; BreakpointNumber++)
    {
      Bit = 2 << (BreakpointNumber << 1);
      if (!(dr7_ & Bit) && GspBreakpoints[BreakpointNumber].Enabled)
        {
          CorrectIt = TRUE;
          dr7_ |= Bit;
          dr7_ &= ~(0xf0000 << (BreakpointNumber << 2));
          dr7_ |= (((GspBreakpoints[BreakpointNumber].Length << 2) |
          GspBreakpoints[BreakpointNumber].Type) << 16) << (BreakpointNumber << 2);
          switch (BreakpointNumber)
            {
#if defined(__GNUC__)
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
#elif defined(_MSC_VER)
            case 0:
              {
                ULONG addr = GspBreakpoints[BreakpointNumber].Address;
                __asm mov eax, addr;
                __asm mov dr0, eax;
              }
              break;
            case 1:
              {
                ULONG addr = GspBreakpoints[BreakpointNumber].Address;
                __asm mov eax, addr;
                __asm mov dr1, eax;
              }
              break;
            case 2:
              {
                ULONG addr = GspBreakpoints[BreakpointNumber].Address;
                __asm mov eax, addr;
                __asm mov dr2, eax;
              }
              break;
            case 3:
              {
                ULONG addr = GspBreakpoints[BreakpointNumber].Address;
                __asm mov eax, addr;
                __asm mov dr3, eax;
              }
              break;
#else
#error Unknown compiler for inline assembler
#endif
            }
        }
      else if ((dr7_ & Bit) && !GspBreakpoints[BreakpointNumber].Enabled)
        {
          CorrectIt = TRUE;
          dr7_ &= ~Bit;
          dr7_ &= ~(0xf0000 << (BreakpointNumber << 2));
        }
    }
  if (CorrectIt)
    {
#if defined(__GNUC__)
      asm volatile ( "movl %0, %%db7\n" : : "r" (dr7_));
#elif defined(_MSC_VER)
      __asm mov eax, dr7_;
      __asm mov dr7, eax;
#else
#error Unknown compiler for inline assembler
#endif
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


static BOOL gdb_attached_yet = FALSE;
/*
 * This function does all command procesing for interfacing to gdb.
 */
KD_CONTINUE_TYPE
STDCALL
KdpGdbEnterDebuggerException(PEXCEPTION_RECORD ExceptionRecord,
                             PCONTEXT Context,
                             PKTRAP_FRAME TrapFrame)
{
  BOOLEAN Stepping;
  LONG Address;
  LONG Length;
  LONG SigVal = 0;
  LONG NewPC;
  PCHAR ptr;

  /* FIXME: Stop on other CPUs too */
  /* Disable hardware debugging while we are inside the stub */
#if defined(__GNUC__)
  __asm__("movl %0,%%db7" : /* no output */ : "r" (0));
#elif defined(_MSC_VER)
  __asm mov eax, 0  __asm mov dr7, eax
#else
#error Unknown compiler for inline assembler
#endif

  if (STATUS_ACCESS_VIOLATION == (NTSTATUS) ExceptionRecord->ExceptionCode &&
      NULL != GspAccessLocation &&
      (ULONG_PTR) GspAccessLocation ==
      (ULONG_PTR) ExceptionRecord->ExceptionInformation[1])
    {
      GspAccessLocation = NULL;
      GspMemoryError = TRUE;
      Context->Eip += 3;
    }
  else
    {
      /* Make sure we're debugging the current thread. */
      if (NULL != GspDbgThread)
        {
          DPRINT1("Internal error: entering stub with non-NULL GspDbgThread\n");
          ObDereferenceObject(GspDbgThread);
          GspDbgThread = NULL;
        }

      /* ugly hack to avoid attempting to send status at the very
       * beginning, right when GDB is trying to query the stub */
      if (gdb_attached_yet)
        {
          LONG Esp;
  
          stop_reply:
          /* reply to host that an exception has occurred */
          SigVal = GspComputeSignal(ExceptionRecord->ExceptionCode);

          ptr = GspOutBuffer;

          *ptr++ = 'T'; /* notify gdb with signo, PC, FP and SP */
          *ptr++ = HexChars[(SigVal >> 4) & 0xf];
          *ptr++ = HexChars[SigVal & 0xf];

          *ptr++ = HexChars[ESP];
          *ptr++ = ':';
  
          Esp = GspGetEspFromTrapFrame(TrapFrame);      /* SP */
          ptr = GspMem2Hex((PCHAR) &Esp, ptr, 4, 0);
          *ptr++ = ';';
  
          *ptr++ = HexChars[EBP];
          *ptr++ = ':';
          ptr = GspMem2Hex((PCHAR) &TrapFrame->Ebp, ptr, 4, 0);       /* FP */
          *ptr++ = ';';
  
          *ptr++ = HexChars[PC];
          *ptr++ = ':';
          ptr = GspMem2Hex((PCHAR) &TrapFrame->Eip, ptr, 4, 0);        /* PC */
          *ptr++ = ';';

          *ptr = '\0';

          GspPutPacket(&GspOutBuffer[0]);
        }
      else
        {
          gdb_attached_yet = 1;
        }
      
      Stepping = FALSE;

      while (TRUE)
        {
          /* Zero the buffer now so we don't have to worry about the terminating zero character */
          memset(GspOutBuffer, 0, sizeof(GspInBuffer));
          ptr = GspGetPacket();

          switch(*ptr++)
            {
            case '?':
              /* a little hack to send more complete status information */
              goto stop_reply;
              GspOutBuffer[0] = 'S';
              GspOutBuffer[1] = HexChars[SigVal >> 4];
              GspOutBuffer[2] = HexChars[SigVal % 16];
              GspOutBuffer[3] = 0;
              break;
            case 'd':
              GspRemoteDebug = !GspRemoteDebug; /* toggle debug flag */
              break;
            case 'g': /* return the value of the CPU Registers */
              if (NULL != GspDbgThread)
                {
                  GspGetRegistersFromTrapFrame(&GspOutBuffer[0], Context, GspDbgThread->Tcb.TrapFrame);
                }
              else
                {
                  GspGetRegistersFromTrapFrame(&GspOutBuffer[0], Context, TrapFrame);
                }
              break;
            case 'G': /* set the value of the CPU Registers - return OK */
              if (NULL != GspDbgThread)
                {
                  GspSetRegistersInTrapFrame(ptr, Context, GspDbgThread->Tcb.TrapFrame);
                }
              else
                {
                  GspSetRegistersInTrapFrame(ptr, Context, TrapFrame);
                }
              strcpy(GspOutBuffer, "OK");
              break;
            case 'P': /* set the value of a single CPU register - return OK */
              {
                LONG Register;

                if ((GspHex2Long(&ptr, &Register)) && (*ptr++ == '='))
                  {
                    if ((Register >= 0) && (Register < NUMREGS))
                      {
                        if (GspDbgThread)
                          {
                            GspSetSingleRegisterInTrapFrame(ptr, Register,
                                                            Context,
                                                            GspDbgThread->Tcb.TrapFrame);
                          }
                        else
                          {
                            GspSetSingleRegisterInTrapFrame(ptr, Register,
                                                            Context, TrapFrame);
                          }
                        strcpy(GspOutBuffer, "OK");
                        break;
                      }
                  }

                strcpy(GspOutBuffer, "E01");
                break;
              }

            /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
            case 'm':
              /* TRY TO READ %x,%x.  IF SUCCEED, SET PTR = 0 */
              if (GspHex2Long(&ptr, &Address) &&
                  *(ptr++) == ',' &&
                  GspHex2Long(&ptr, &Length))
                {
                  PEPROCESS DbgProcess = NULL;

                  ptr = NULL;
                  if (NULL != GspDbgThread &&
                      PsGetCurrentProcess() != GspDbgThread->ThreadsProcess)
                    {
                      DbgProcess = GspDbgThread->ThreadsProcess;
                      KeAttachProcess(&DbgProcess->Pcb);
                    }
                  GspMemoryError = FALSE;
                  GspMem2Hex((PCHAR) Address, GspOutBuffer, Length, 1);
                  if (NULL != DbgProcess)
                    {
                      KeDetachProcess();
                    }
                  if (GspMemoryError)
                    {
                      strcpy(GspOutBuffer, "E03");
                      DPRINT("Fault during memory read\n");
                    }
                }

              if (NULL != ptr)
                {
                  strcpy(GspOutBuffer, "E01");
                }
              break;

            /* MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK */
            case 'M':
              /* TRY TO READ '%x,%x:'.  IF SUCCEED, SET PTR = 0 */
              if (GspHex2Long(&ptr, &Address))
                {
                  if (*(ptr++) == ',' &&
                      GspHex2Long(&ptr, &Length) &&
                      *(ptr++) == ':')
                    {
                      PEPROCESS DbgProcess = NULL;

                      ptr = NULL;
                      if (NULL != GspDbgThread &&
                          PsGetCurrentProcess() != GspDbgThread->ThreadsProcess)
                        {
                          DbgProcess = GspDbgThread->ThreadsProcess;
                          KeAttachProcess(&DbgProcess->Pcb);
                        }
                      GspMemoryError = FALSE;
                      GspHex2Mem(ptr, (PCHAR) Address, Length, TRUE);
                      if (NULL != DbgProcess)
                        {
                          KeDetachProcess();
                        }
                      if (GspMemoryError)
                        {
                          strcpy(GspOutBuffer, "E03");
                          DPRINT("Fault during memory write\n");
                        }
                      else
                        {
                          strcpy(GspOutBuffer, "OK");
                        }
                    }
                }

              if (NULL != ptr)
                {
                  strcpy(GspOutBuffer, "E02");
                }
              break;

            /* cAA..AA   Continue at address AA..AA(optional) */
            /* sAA..AA   Step one instruction from AA..AA(optional) */
            case 's':
              Stepping = TRUE;
            case 'c':
              {
                ULONG BreakpointNumber;
                ULONG dr6_;

                /* try to read optional parameter, pc unchanged if no parm */
                if (GspHex2Long (&ptr, &Address))
                  {
                    Context->Eip = Address;
                  }

                NewPC = Context->Eip;

                /* clear the trace bit */
                Context->EFlags &= 0xfffffeff;

                /* set the trace bit if we're Stepping */
                if (Stepping)
                  {
                    Context->EFlags |= 0x100;
                  }

#if defined(__GNUC__)
                asm volatile ("movl %%db6, %0\n" : "=r" (dr6_) : );
#elif defined(_MSC_VER)
                __asm mov eax, dr6  __asm mov dr6_, eax;
#else
#error Unknown compiler for inline assembler
#endif
                if (!(dr6_ & 0x4000))
                  {
                    for (BreakpointNumber = 0; BreakpointNumber < 4; ++BreakpointNumber)
                      {
                        if (dr6_ & (1 << BreakpointNumber))
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
#if defined(__GNUC__)
                asm volatile ("movl %0, %%db6\n" : : "r" (0));
#elif defined(_MSC_VER)
                __asm mov eax, 0  __asm mov dr6, eax;
#else
#error Unknown compiler for inline assembler
#endif

                if (NULL != GspDbgThread)
                  {
                    ObDereferenceObject(GspDbgThread);
                    GspDbgThread = NULL;
                  }

                return kdContinue;
                break;
              }

            case 'k':  /* kill the program */
              strcpy(GspOutBuffer, "OK");
              break;
              /* kill the program */

            case 'H': /* Set thread */
              GspSetThread(ptr);
              break;

            case 'q': /* Query */
              GspQuery(ptr);
              break;

            case 'T': /* Query thread status */
              GspQueryThreadStatus(ptr);
              break;

            case 'Y':
              {
                LONG Number;
                LONG Length;
                LONG Type;
                LONG Address;

                ptr = &GspOutBuffer[1];
                GspHex2Long(&ptr, &Number);
                ptr++;
                GspHex2Long(&ptr, &Type);
                ptr++;
                GspHex2Long(&ptr, &Length);
                ptr++;
                GspHex2Long(&ptr, &Address);
                if (GspSetHwBreakpoint(Number & 0x3, Type & 0x3 , Length & 0x3, Address) == 0)
                  {
                    strcpy(GspOutBuffer, "OK");
                  }
                else
                  {
                    strcpy(GspOutBuffer, "E");
                  }
                break;
              }

            /* Remove hardware breakpoint */
            case 'y':
              {
                LONG Number;

                ptr = &GspOutBuffer[1];
                GspHex2Long(&ptr, &Number);
                if (GspRemoveHwBreakpoint(Number & 0x3) == 0)
                  {
                    strcpy(GspOutBuffer, "OK");
                  }
                else
                  {
                    strcpy(GspOutBuffer, "E");
                  }
                break;
              }

            default:
              break;
            }

          /* reply to the request */
          GspPutPacket(&GspOutBuffer[0]);
        }

      /* not reached */
      ASSERT(0);
    }

  if (NULL != GspDbgThread)
    {
      ObDereferenceObject(GspDbgThread);
      GspDbgThread = NULL;
    }

  return kdContinue;
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
  UCHAR Value;

  DPRINT("Break In\n");

  DoBreakIn = FALSE;
  while (KdPortGetByteEx(&GdbPortInfo, &Value))
    {
      if (Value == 0x03)
        {
          DoBreakIn = TRUE;
        }
    }

  if (!DoBreakIn)
    {
      return TRUE;
    }

  KeRaiseIrql(HIGH_LEVEL, &OldIrql);

  TrapFrame = PsGetCurrentThread()->Tcb.TrapFrame;

  KeTrapFrameToContext(TrapFrame, NULL, &Context);

  KdpGdbEnterDebuggerException(NULL, &Context, TrapFrame);

  KeContextToTrapFrame(&Context, NULL, TrapFrame, KernelMode);

  KeLowerIrql(OldIrql);

  return TRUE;
}

VOID
STDCALL
KdpGdbDebugPrint(PCH Message, ULONG Length)
{
}

/* Initialize the GDB stub */
VOID
STDCALL
KdpGdbStubInit(PKD_DISPATCH_TABLE WrapperTable,
               ULONG BootPhase)
{
  if (!KdDebuggerEnabled || !KdpDebugMode.Gdb)
    {
      return;
    }

  if (BootPhase == 0)
    {
      /* Write out the functions that we support for now */
      WrapperTable->KdpInitRoutine = KdpGdbStubInit;
      WrapperTable->KdpPrintRoutine = KdpGdbDebugPrint;
      WrapperTable->KdpExceptionRoutine = KdpGdbEnterDebuggerException;

      /* Initialize the Port */
      KdPortInitializeEx(&GdbPortInfo, 0, 0);
    }
  else if (BootPhase == 1)
    {
      GspInitialized = TRUE;

      GspRunThread = NULL;
      GspDbgThread = NULL;
      GspEnumThread = NULL;

      HalDisplayString("Waiting for GDB to attach\n");
      DbgPrint("Module 'hal.dll' loaded at 0x%.08x.\n", LdrHalBase);
      DbgBreakPointWithStatus(DBG_STATUS_CONTROL_C);
    }
  else if (BootPhase == 2)
    {
      HalDisplayString("\n   GDB debugging enabled\n\n");
    }
}

/* EOF */
