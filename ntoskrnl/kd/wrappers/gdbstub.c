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

static FAST_MUTEX GspLock;

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
  ULONG Size;
  ULONG OffsetInTF;
  ULONG OffsetInContext;
  BOOLEAN SetInContext;
} CPU_REGISTER, *PCPU_REGISTER;

static CPU_REGISTER GspRegisters[NUMREGS] =
{
  { 4, FIELD_OFFSET(KTRAP_FRAME, Eax), FIELD_OFFSET(CONTEXT, Eax), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, Ecx), FIELD_OFFSET(CONTEXT, Ecx), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, Edx), FIELD_OFFSET(CONTEXT, Edx), FALSE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, Ebx), FIELD_OFFSET(CONTEXT, Ebx), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, HardwareEsp ), FIELD_OFFSET(CONTEXT, Esp), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, DbgEbp), FIELD_OFFSET(CONTEXT, Ebp), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, Esi), FIELD_OFFSET(CONTEXT, Esi), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, Edi), FIELD_OFFSET(CONTEXT, Edi), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, DbgEip), FIELD_OFFSET(CONTEXT, Eip), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, EFlags), FIELD_OFFSET(CONTEXT, EFlags), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, SegCs), FIELD_OFFSET(CONTEXT, SegCs), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, HardwareSegSs), FIELD_OFFSET(CONTEXT, SegSs), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, SegDs), FIELD_OFFSET(CONTEXT, SegDs), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, SegEs), FIELD_OFFSET(CONTEXT, SegEs), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, SegFs), FIELD_OFFSET(CONTEXT, SegFs), TRUE },
  { 4, FIELD_OFFSET(KTRAP_FRAME, SegGs), FIELD_OFFSET(CONTEXT, SegGs), TRUE }
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

static CHAR
GspReadMemSafe(PCHAR Address)
{
  CHAR ch;

  if (NULL == Address)
    {
      GspMemoryError = TRUE;
      return '\0';
    }

  GspAccessLocation = Address;
  ch = *Address;
  GspAccessLocation = NULL;

  return ch;
}

/* Convert the memory pointed to by Address into hex, placing result in Buffer */
/* Return a pointer to the last char put in Buffer (null) */
/* If MayFault is TRUE, then we should set GspMemoryError in response to
   a fault; if FALSE treat a fault like any other fault in the stub.  */
static PCHAR
GspMem2Hex(PCHAR Address,
  PCHAR Buffer,
  LONG Count,
  BOOLEAN MayFault)
{
  ULONG i;
  CHAR ch;

  for (i = 0; i < (ULONG) Count; i++)
    {
      if (MayFault)
        {
          ch = GspReadMemSafe(Address);
          if (GspMemoryError)
            {
              return Buffer;
            }
        }
      else
        {
          ch = *Address;
        }
      *Buffer++ = HexChars[(ch >> 4) & 0xf];
      *Buffer++ = HexChars[ch & 0xf];
      Address++;
    }

  *Buffer = 0;
  return Buffer;
}

static ULONG
GspWriteMem(PCHAR Address,
  ULONG Count,
  BOOLEAN MayFault,
  CHAR (*GetContent)(PVOID Context, ULONG Offset),
  PVOID Context)
{
  PCHAR Current;
  PCHAR Page;
  ULONG CountInPage;
  ULONG i;
  CHAR ch;
  ULONG OldProt = 0;

  Current = Address;
  while (Current < Address + Count)
    {
      Page = (PCHAR)PAGE_ROUND_DOWN(Current);
      if (Address + Count <= Page + PAGE_SIZE)
        {
          /* Fits in this page */
          CountInPage = Count;
        }
      else
        {
          /* Flows into next page, handle only current page in this iteration */
          CountInPage = PAGE_SIZE - (Address - Page);
        }
      if (MayFault)
        {
          OldProt = MmGetPageProtect(NULL, Address);
          MmSetPageProtect(NULL, Address, PAGE_EXECUTE_READWRITE);
        }

      for (i = 0; i < CountInPage && ! GspMemoryError; i++)
        {
          ch = (*GetContent)(Context, Current - Address);

          if (MayFault)
            {
              GspAccessLocation = Current;
            }
          *Current = ch;
          if (MayFault)
            {
              GspAccessLocation = NULL;
            }
          Current++;
        }
      if (MayFault)
        {
          MmSetPageProtect(NULL, Page, OldProt);
          if (GspMemoryError)
            {
              return Current - Address;
            }
        }
    }

  return Current - Address;
}

static CHAR
GspHex2MemGetContent(PVOID Context, ULONG Offset)
{
  return (CHAR)((HexValue(*((PCHAR) Context + 2 * Offset)) << 4) +
                HexValue(*((PCHAR) Context + 2 * Offset + 1)));
}

/* Convert the hex array pointed to by Buffer into binary to be placed at Address */
/* Return a pointer to the character AFTER the last byte read from Buffer */
static PCHAR
GspHex2Mem(PCHAR Buffer,
  PCHAR Address,
  ULONG Count,
  BOOLEAN MayFault)
{
  Count = GspWriteMem(Address, Count, MayFault, GspHex2MemGetContent, Buffer);

  return Buffer + 2 * Count;
}

static CHAR
GspWriteMemSafeGetContent(PVOID Context, ULONG Offset)
{
  ASSERT(0 == Offset);

  return *((PCHAR) Context);
}

static void
GspWriteMemSafe(PCHAR Address,
  CHAR Ch)
{
  GspWriteMem(Address, 1, TRUE, GspWriteMemSafeGetContent, &Ch);
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
         ? (LONG) &TrapFrame->HardwareEsp : (LONG)TrapFrame->HardwareEsp;
}


static VOID
GspGetRegisters(PCHAR Address,
  PKTRAP_FRAME TrapFrame)
{
  ULONG_PTR Value;
  PULONG p;
  ULONG i;
  PETHREAD Thread;
  ULONG_PTR *KernelStack;

  if (NULL == GspDbgThread)
    {
      Thread = PsGetCurrentThread();
    }
  else
    {
      TrapFrame = GspDbgThread->Tcb.TrapFrame;
      Thread = GspDbgThread;
    }

  if (Waiting == Thread->Tcb.State)
    {
      KernelStack = Thread->Tcb.KernelStack;
      for (i = 0; i < sizeof(GspRegisters) / sizeof(GspRegisters[0]); i++)
        {
          switch(i)
            {
              case EBP:
                Value = KernelStack[3];
                break;
              case EDI:
                Value = KernelStack[4];
                break;
              case ESI:
                Value = KernelStack[5];
                break;
              case EBX:
                Value = KernelStack[6];
                break;
              case PC:
                Value = KernelStack[7];
                break;
              case ESP:
                Value = (ULONG_PTR) (KernelStack + 8);
                break;
              case CS:
                Value = KGDT_R0_CODE;
                break;
              case DS:
                Value = KGDT_R0_DATA;
                break;
              default:
                Value = 0;
                break;
            }
          Address = GspMem2Hex((PCHAR) &Value, Address, GspRegisters[i].Size,
                               FALSE);
        }
    }
  else
    {
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
                  p = (PULONG)((ULONG_PTR) TrapFrame +
                               GspRegisters[i].OffsetInTF);
                  Value = *p;
                }
            }
          else if (i == PC)
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
          Address = GspMem2Hex((PCHAR) &Value, Address,
                               GspRegisters[i].Size, FALSE);
        }
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
  ULONG i;

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

#define DR7_L0         0x00000001 /* Local breakpoint 0 enable */
#define DR7_G0         0x00000002 /* Global breakpoint 0 enable */
#define DR7_L1         0x00000004 /* Local breakpoint 1 enable */
#define DR7_G1         0x00000008 /* Global breakpoint 1 enable */
#define DR7_L2         0x00000010 /* Local breakpoint 2 enable */
#define DR7_G2         0x00000020 /* Global breakpoint 2 enable */
#define DR7_L3         0x00000040 /* Local breakpoint 3 enable */
#define DR7_G3         0x00000080 /* Global breakpoint 3 enable */
#define DR7_LE         0x00000100 /* Local exact breakpoint enable (old) */
#define DR7_GE         0x00000200 /* Global exact breakpoint enable (old) */
#define DR7_GD         0x00002000 /* General detect enable */
#define DR7_TYPE0_MASK 0x00030000 /* Breakpoint 0 condition */
#define DR7_LEN0_MASK  0x000c0000 /* Breakpoint 0 length */
#define DR7_TYPE1_MASK 0x00300000 /* Breakpoint 1 condition */
#define DR7_LEN1_MASK  0x00c00000 /* Breakpoint 1 length */
#define DR7_TYPE2_MASK 0x03000000 /* Breakpoint 2 condition */
#define DR7_LEN2_MASK  0x0c000000 /* Breakpoint 2 length */
#define DR7_TYPE3_MASK 0x30000000 /* Breakpoint 3 condition */
#define DR7_LEN3_MASK  0xc0000000 /* Breakpoint 3 length */
#define DR7_GLOBAL_ENABLE(Bp) (2 << (2 * (Bp)))
#define DR7_TYPE(Bp, Type)    ((Type) << (16 + 4 * (Bp)))
#define DR7_LEN(Bp, Len)      ((Len) << (18 + 4 * (Bp)))

#define I386_BP_TYPE_EXECUTE        0
#define I386_BP_TYPE_DATA_WRITE     1
#define I386_BP_TYPE_DATA_READWRITE 3

#define I386_OPCODE_INT3 0xcc

#define GDB_ZTYPE_MEMORY_BREAKPOINT   0
#define GDB_ZTYPE_HARDWARE_BREAKPOINT 1
#define GDB_ZTYPE_WRITE_WATCHPOINT    2
#define GDB_ZTYPE_READ_WATCHPOINT     3
#define GDB_ZTYPE_ACCESS_WATCHPOINT   4

typedef struct _GSPHWBREAKPOINT
{
  ULONG Type;
  ULONG_PTR Address;
  ULONG Length;
} GSPHWBREAKPOINT;

#define MAX_HW_BREAKPOINTS 4
static unsigned GspHwBreakpointCount = 0;
static GSPHWBREAKPOINT GspHwBreakpoints[MAX_HW_BREAKPOINTS];

typedef struct _GSPSWBREAKPOINT
{
  ULONG_PTR Address;
  CHAR PrevContent;
  BOOLEAN Active;
} GSPSWBREAKPOINT;

#define MAX_SW_BREAKPOINTS 64
static unsigned GspSwBreakpointCount = 0;
static GSPSWBREAKPOINT GspSwBreakpoints[MAX_SW_BREAKPOINTS];

static void
GspSetHwBreakpoint(ULONG Type, ULONG_PTR Address, ULONG Length)
{
  DPRINT("GspSetHwBreakpoint(%lu, 0x%p, %lu)\n", Type, Address, Length);

  if (GDB_ZTYPE_READ_WATCHPOINT == Type)
    {
      DPRINT1("Read watchpoint not supported\n");
      strcpy(GspOutBuffer, "E22");
    }
  else if (GDB_ZTYPE_HARDWARE_BREAKPOINT == Type && 1 != Length)
    {
      DPRINT1("Invalid length %lu for hardware breakpoint\n", Length);
      strcpy(GspOutBuffer, "E22");
    }
  else if (1 != Length && 2 != Length && 4 != Length)
    {
      DPRINT1("Invalid length %lu for GDB Z type %lu\n", Length, Type);
      strcpy(GspOutBuffer, "E22");
    }
  else if (0 != (Address & (Length - 1)))
    {
      DPRINT1("Invalid alignment for address 0x%p and length %d\n",
              Address, Length);
      strcpy(GspOutBuffer, "E22");
    }
  else if (MAX_HW_BREAKPOINTS == GspHwBreakpointCount)
    {
      DPRINT1("Trying to set too many hardware breakpoints\n");
      strcpy(GspOutBuffer, "E22");
    }
  else
    {
      DPRINT("Stored at index %u\n", GspHwBreakpointCount);
      GspHwBreakpoints[GspHwBreakpointCount].Type = Type;
      GspHwBreakpoints[GspHwBreakpointCount].Address = Address;
      GspHwBreakpoints[GspHwBreakpointCount].Length = Length;
      GspHwBreakpointCount++;
      strcpy(GspOutBuffer, "OK");
    }
}

static void
GspRemoveHwBreakpoint(ULONG Type, ULONG_PTR Address, ULONG Length)
{
  unsigned Index;

  DPRINT("GspRemoveHwBreakpoint(%lu, 0x%p, %lu)\n", Type, Address, Length);
  for (Index = 0; Index < GspHwBreakpointCount; Index++)
    {
      if (GspHwBreakpoints[Index].Type == Type &&
          GspHwBreakpoints[Index].Address == Address &&
          GspHwBreakpoints[Index].Length == Length)
        {
          DPRINT("Found match at index %u\n", Index);
          if (Index + 1 < GspHwBreakpointCount)
            {
              memmove(GspHwBreakpoints + Index,
                      GspHwBreakpoints + (Index + 1),
                      (GspHwBreakpointCount - Index - 1) *
                      sizeof(GSPHWBREAKPOINT));
            }
          GspHwBreakpointCount--;
          strcpy(GspOutBuffer, "OK");
          return;
        }
    }

  DPRINT1("Not found\n");
  strcpy(GspOutBuffer, "E22");
}

static void
GspSetSwBreakpoint(ULONG_PTR Address)
{
  DPRINT("GspSetSwBreakpoint(0x%p)\n", Address);

  if (MAX_SW_BREAKPOINTS == GspSwBreakpointCount)
    {
      DPRINT1("Trying to set too many software breakpoints\n");
      strcpy(GspOutBuffer, "E22");
    }
  else
    {
      DPRINT("Stored at index %u\n", GspSwBreakpointCount);
      GspSwBreakpoints[GspSwBreakpointCount].Address = Address;
      GspSwBreakpoints[GspSwBreakpointCount].Active = FALSE;
      GspSwBreakpointCount++;
      strcpy(GspOutBuffer, "OK");
    }
}

static void
GspRemoveSwBreakpoint(ULONG_PTR Address)
{
  unsigned Index;

  DPRINT("GspRemoveSwBreakpoint(0x%p)\n", Address);
  for (Index = 0; Index < GspSwBreakpointCount; Index++)
    {
      if (GspSwBreakpoints[Index].Address == Address)
        {
          DPRINT("Found match at index %u\n", Index);
          ASSERT(! GspSwBreakpoints[Index].Active);
          if (Index + 1 < GspSwBreakpointCount)
            {
              memmove(GspSwBreakpoints + Index,
                      GspSwBreakpoints + (Index + 1),
                      (GspSwBreakpointCount - Index - 1) *
                      sizeof(GSPSWBREAKPOINT));
            }
          GspSwBreakpointCount--;
          strcpy(GspOutBuffer, "OK");
          return;
        }
    }

  DPRINT1("Not found\n");
  strcpy(GspOutBuffer, "E22");
}

static void
GspLoadHwBreakpoint(PKTRAP_FRAME TrapFrame,
                    unsigned BpIndex,
                    ULONG_PTR Address,
                    ULONG Length,
                    ULONG Type)
{
  DPRINT("GspLoadHwBreakpoint(0x%p, %d, 0x%p, %d)\n", TrapFrame, BpIndex,
         Address, Type);

  /* Set the DR7_Gx bit to globally enable the breakpoint */
  TrapFrame->Dr7 |= DR7_GLOBAL_ENABLE(BpIndex) |
                    DR7_LEN(BpIndex, Length) |
                    DR7_TYPE(BpIndex, Type);

  switch (BpIndex)
    {
    case 0:
      DPRINT("Setting DR0 to 0x%p\n", Address);
      TrapFrame->Dr0 = Address;
      break;

    case 1:
      DPRINT("Setting DR1 to 0x%p\n", Address);
      TrapFrame->Dr1 = Address;
      break;

    case 2:
      DPRINT("Setting DR2 to 0x%p\n", Address);
      TrapFrame->Dr2 = Address;
      break;

    case 3:
      DPRINT("Setting DR3 to 0x%p\n", Address);
      TrapFrame->Dr3 = Address;
      break;
    }
}

static void
GspLoadBreakpoints(PKTRAP_FRAME TrapFrame)
{
  unsigned Index;
  ULONG i386Type;

  DPRINT("GspLoadBreakpoints\n");
  DPRINT("DR7 on entry: 0x%08x\n", TrapFrame->Dr7);
  /* Remove all breakpoints */
  TrapFrame->Dr7 &= ~(DR7_L0 | DR7_L1 | DR7_L2 | DR7_L3 |
                      DR7_G0 | DR7_G1 | DR7_G2 | DR7_G3 |
                      DR7_TYPE0_MASK | DR7_LEN0_MASK |
                      DR7_TYPE1_MASK | DR7_LEN1_MASK |
                      DR7_TYPE2_MASK | DR7_LEN2_MASK |
                      DR7_TYPE3_MASK | DR7_LEN3_MASK);

  for (Index = 0; Index < GspHwBreakpointCount; Index++)
    {
      switch(GspHwBreakpoints[Index].Type)
        {
        case GDB_ZTYPE_HARDWARE_BREAKPOINT:
          i386Type = I386_BP_TYPE_EXECUTE;
          break;
        case GDB_ZTYPE_WRITE_WATCHPOINT:
          i386Type = I386_BP_TYPE_DATA_WRITE;
          break;
        case GDB_ZTYPE_ACCESS_WATCHPOINT:
          i386Type = I386_BP_TYPE_DATA_READWRITE;
          break;
        default:
          ASSERT(FALSE);
          i386Type = I386_BP_TYPE_EXECUTE;
          break;
        }

      GspLoadHwBreakpoint(TrapFrame, Index, GspHwBreakpoints[Index].Address,
                          GspHwBreakpoints[Index].Length - 1, i386Type);
    }

  for (Index = 0; Index < GspSwBreakpointCount; Index++)
    {
      if (GspHwBreakpointCount + Index < MAX_HW_BREAKPOINTS)
        {
          DPRINT("Implementing software interrupt using hardware register\n");
          GspLoadHwBreakpoint(TrapFrame, GspHwBreakpointCount + Index,
                              GspSwBreakpoints[Index].Address, 0,
                              I386_BP_TYPE_EXECUTE);
          GspSwBreakpoints[Index].Active = FALSE;
        }
      else
        {
          DPRINT("Using real software breakpoint\n");
          GspMemoryError = FALSE;
          GspSwBreakpoints[Index].PrevContent = GspReadMemSafe((PCHAR) GspSwBreakpoints[Index].Address);
          if (! GspMemoryError)
            {
              GspWriteMemSafe((PCHAR) GspSwBreakpoints[Index].Address, I386_OPCODE_INT3);
            }
          GspSwBreakpoints[Index].Active = ! GspMemoryError;
          if (GspMemoryError)
            {
              DPRINT1("Failed to set software breakpoint at 0x%p\n",
                      GspSwBreakpoints[Index].Address);
            }
          else
            {
              DPRINT("Successfully set software breakpoint at 0x%p\n",
                     GspSwBreakpoints[Index].Address);
    DPRINT1("Successfully set software breakpoint at 0x%p\n", GspSwBreakpoints[Index].Address);
            }
        }
    }

  DPRINT("Final DR7 value 0x%08x\n", TrapFrame->Dr7);
}

static void
GspUnloadBreakpoints(PKTRAP_FRAME TrapFrame)
{
  unsigned Index;

  DPRINT("GspUnloadHwBreakpoints\n");

  for (Index = 0; Index < GspSwBreakpointCount; Index++)
    {
      if (GspSwBreakpoints[Index].Active)
        {
          GspMemoryError = FALSE;
          GspWriteMemSafe((PCHAR) GspSwBreakpoints[Index].Address,
                          GspSwBreakpoints[Index].PrevContent);
          GspSwBreakpoints[Index].Active = FALSE;
          if (GspMemoryError)
            {
              DPRINT1("Failed to remove software breakpoint from 0x%p\n",
                      GspSwBreakpoints[Index].Address);
            }
          else
            {
              DPRINT("Successfully removed software breakpoint from 0x%p\n",
                     GspSwBreakpoints[Index].Address);
            }
        }
    }
}

static BOOLEAN gdb_attached_yet = FALSE;
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
      DPRINT("Thread %p entering stub\n", PsGetCurrentThread());
      /* Can only debug 1 thread at a time... */
      ExAcquireFastMutex(&GspLock);
      DPRINT("Thread %p acquired mutex\n", PsGetCurrentThread());

      /* Disable hardware debugging while we are inside the stub */
      Ke386SetDr7(0);
      GspUnloadBreakpoints(TrapFrame);

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
              GspGetRegisters(GspOutBuffer, TrapFrame);
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
                      ptr = NULL;
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
                            if (GspHwBreakpoints[BreakpointNumber].Type == 0)
                              {
                                /* Set restore flag */
                                Context->EFlags |= 0x10000;
                                break;
                              }
                          }
                      }
                  }
                GspLoadBreakpoints(TrapFrame);
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

                DPRINT("Thread %p releasing mutex\n", PsGetCurrentThread());
                ExReleaseFastMutex(&GspLock);
                DPRINT("Thread %p leaving stub\n", PsGetCurrentThread());
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

            case 'Z':
              {
                LONG Type;
                LONG Address;
                LONG Length;

                GspHex2Long(&ptr, &Type);
                ptr++;
                GspHex2Long(&ptr, &Address);
                ptr++;
                GspHex2Long(&ptr, &Length);
                if (0 == Type)
                  {
                  GspSetSwBreakpoint((ULONG_PTR) Address);
                  }
                else
                  {
                  GspSetHwBreakpoint(Type, (ULONG_PTR) Address, Length);
                  }
                break;
              }

            case 'z':
              {
                LONG Type;
                LONG Address;
                LONG Length;

                GspHex2Long(&ptr, &Type);
                ptr++;
                GspHex2Long(&ptr, &Address);
                ptr++;
                GspHex2Long(&ptr, &Length);
                if (0 == Type)
                  {
                  GspRemoveSwBreakpoint((ULONG_PTR) Address);
                  }
                else
                  {
                  GspRemoveHwBreakpoint(Type, (ULONG_PTR) Address, Length);
                  }
                break;
              }

            default:
              break;
            }

          /* reply to the request */
          GspPutPacket(GspOutBuffer);
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

  KeContextToTrapFrame(&Context, NULL, TrapFrame, Context.ContextFlags, KernelMode);

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
      ExInitializeFastMutex(&GspLock);

      /* Write out the functions that we support for now */
      WrapperTable->KdpInitRoutine = KdpGdbStubInit;
      WrapperTable->KdpPrintRoutine = KdpGdbDebugPrint;
      WrapperTable->KdpExceptionRoutine = KdpGdbEnterDebuggerException;

      /* Initialize the Port */
      KdPortInitializeEx(&GdbPortInfo, 0, 0);

      KdpPort = GdbPortInfo.ComPort;
    }
  else if (BootPhase == 1)
    {
      GspInitialized = TRUE;

      GspRunThread = NULL;
      GspDbgThread = NULL;
      GspEnumThread = NULL;

      HalDisplayString("Waiting for GDB to attach\n");
      DbgBreakPointWithStatus(DBG_STATUS_CONTROL_C);
    }
  else if (BootPhase == 2)
    {
      HalDisplayString("\n   GDB debugging enabled\n\n");
    }
}

/* EOF */
