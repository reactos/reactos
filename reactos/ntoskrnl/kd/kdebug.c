/* $Id: kdebug.c,v 1.36 2002/05/08 17:05:32 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/kd/kdebug.c
 * PURPOSE:         Kernel debugger
 * PROGRAMMER:      Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * UPDATE HISTORY:
 *                  21/10/99: Created
 */

#include <ddk/ntddk.h>
#include <internal/ntoskrnl.h>
#include <internal/kd.h>
#include <internal/mm.h>
#include <roscfg.h>
#include "../dbg/kdb.h"

/* serial debug connection */
#define DEFAULT_DEBUG_PORT      2	/* COM2 */
#define DEFAULT_DEBUG_COM1_IRQ  4	/* COM1 IRQ */
#define DEFAULT_DEBUG_COM2_IRQ  3	/* COM2 IRQ */
#define DEFAULT_DEBUG_BAUD_RATE 115200	/* 115200 Baud */

/* bochs debug output */
#define BOCHS_LOGGER_PORT (0xe9)

/* VARIABLES ***************************************************************/

BOOLEAN
__declspec(dllexport)
KdDebuggerEnabled = FALSE;		/* EXPORTED */

BOOLEAN
__declspec(dllexport)
KdDebuggerNotPresent = TRUE;		/* EXPORTED */


static BOOLEAN KdpBreakPending = FALSE;
ULONG KdDebugState = KD_DEBUG_DISABLED;
ULONG KdpPortIrq = 0;

KD_PORT_INFORMATION GdbPortInfo;
KD_PORT_INFORMATION LogPortInfo;

/* PRIVATE FUNCTIONS ********************************************************/

static VOID
PrintString(char* fmt,...)
{
  char buffer[512];
  va_list ap;

  va_start(ap, fmt);
  vsprintf(buffer, fmt, ap);
  va_end(ap);

  HalDisplayString(buffer);
}


VOID
KdInitSystem(ULONG Reserved,
	     PLOADER_PARAMETER_BLOCK LoaderBlock)
{
  KD_PORT_INFORMATION PortInfo;
  ULONG Value;
  PCHAR p1, p2;

#ifdef KDBG
  /* Initialize runtime debugging if available */
  DbgRDebugInit();
#endif

  /* Set debug port default values */
  PortInfo.ComPort = DEFAULT_DEBUG_PORT;
  PortInfo.BaudRate = DEFAULT_DEBUG_BAUD_RATE;
  KdpPortIrq = DEFAULT_DEBUG_COM2_IRQ;

  /* Set serial log port default values */
  LogPortInfo.ComPort = DEFAULT_DEBUG_PORT;
  LogPortInfo.BaudRate = DEFAULT_DEBUG_BAUD_RATE;

  /* Parse kernel command line */

  /* Check for 'DEBUGPORT' */
  p1 = (PCHAR)LoaderBlock->CommandLine;
  while (p1 && (p2 = strchr(p1, '/')))
    {
      p2++;
      if (!_strnicmp(p2, "DEBUGPORT", 9))
	{
	  p2 += 9;
	  if (*p2 == '=')
	    {
	      p2++;
	      if (!_strnicmp(p2, "SCREEN", 6))
		{
		  p2 += 6;
		  KdDebuggerEnabled = TRUE;
		  KdDebugState |= KD_DEBUG_SCREEN;
		}
	      else if (!_strnicmp(p2, "BOCHS", 5))
		{
		  p2 += 5;
		  KdDebuggerEnabled = TRUE;
		  KdDebugState |= KD_DEBUG_BOCHS;
		}
	      else if (!_strnicmp(p2, "GDB", 3))
		{
		  p2 += 3;
		  KdDebuggerEnabled = TRUE;
		  KdDebugState |= KD_DEBUG_GDB;

		  /* Reset port information to defaults */
		  RtlMoveMemory(&GdbPortInfo, &PortInfo, sizeof(KD_PORT_INFORMATION));
		  PortInfo.ComPort = DEFAULT_DEBUG_PORT;
		  PortInfo.BaudRate = DEFAULT_DEBUG_BAUD_RATE;
		}
	      else if (!_strnicmp(p2, "PICE", 4))
		{
		  p2 += 4;
		  KdDebuggerEnabled = TRUE;
		  KdDebugState |= KD_DEBUG_PICE;
		}
	      else if (!_strnicmp(p2, "COM", 3))
		{
		  p2 += 3;
		  Value = (ULONG)atol(p2);
		  if (Value > 0 && Value < 5)
		    {
		      KdDebuggerEnabled = TRUE;
			  KdDebugState |= KD_DEBUG_SERIAL;
		      LogPortInfo.ComPort = Value;
		    }
		}
	      else if (!_strnicmp(p2, "FILE", 4))
		{
		  p2 += 4;
		  KdDebuggerEnabled = TRUE;
		  KdDebugState |= KD_DEBUG_FILELOG;
		}
	    }
	}
      else if (!_strnicmp(p2, "DEBUG", 5))
	{
	  p2 += 5;
	  KdDebuggerEnabled = TRUE;
	  KdDebugState |= KD_DEBUG_SERIAL;
	}
      else if (!_strnicmp(p2, "NODEBUG", 7))
	{
	  p2 += 7;
	  KdDebuggerEnabled = FALSE;
	  KdDebugState = KD_DEBUG_DISABLED;
	}
      else if (!_strnicmp(p2, "CRASHDEBUG", 10))
	{
	  p2 += 10;
	  KdDebuggerEnabled = FALSE;
	  KdDebugState = KD_DEBUG_DISABLED;
	}
      else if (!_strnicmp(p2, "BREAK", 5))
	{
	  p2 += 5;
	  KdpBreakPending = TRUE;
	}
	  else if (!_strnicmp(p2, "COM", 3))
	    {
	      p2 += 3;
	      if (*p2 != '=')
		{
		  p2++;
		  Value = (ULONG)atol(p2);
		  if (Value > 0 && Value < 5)
		    {
		      PortInfo.ComPort = Value;
		    }
		}
	}
      else if (!_strnicmp(p2, "BAUDRATE", 8))
	{
	  p2 += 8;
	  if (*p2 != '=')
	    {
	      p2++;
	      Value = (ULONG)atol(p2);
	      if (Value > 0)
			{
			  PortInfo.BaudRate = Value;
			}
	    }
	  else if (!_strnicmp(p2, "IRQ", 3))
	    {
	      p2 += 3;
	      if (*p2 != '=')
		{
		  p2++;
		  Value = (ULONG)atol(p2);
		  if (Value > 0)
		    {
		      KdpPortIrq = Value;
		    }
		}
	    }
	}
      p1 = p2;
    }

  /* Print some information */
  if (KdDebuggerEnabled == TRUE)
    {
      if (KdDebugState & KD_DEBUG_GDB)
	    PrintString("\n   GDB debugging enabled. COM%ld %ld Baud\n\n",
			GdbPortInfo.ComPort, GdbPortInfo.BaudRate);
	  
      if (KdDebugState & KD_DEBUG_PICE)
	    PrintString("\n   Private ICE debugger enabled\n\n");

      if (KdDebugState & KD_DEBUG_SCREEN)
	    PrintString("\n   Screen debugging enabled\n\n");

      if (KdDebugState & KD_DEBUG_BOCHS)
	    PrintString("\n   Bochs debugging enabled\n\n");

      if (KdDebugState & KD_DEBUG_SERIAL)
	    PrintString("\n   Serial debugging enabled. COM%ld %ld Baud\n\n",
			LogPortInfo.ComPort, LogPortInfo.BaudRate);

      if (KdDebugState & KD_DEBUG_FILELOG)
	    PrintString("\n   File log debugging enabled\n\n");
    }

  /* Perform any initialization nescessary */
  if (KdDebuggerEnabled == TRUE)
    {
      if (KdDebugState & KD_DEBUG_GDB)
	    KdPortInitializeEx(&GdbPortInfo, 0, 0);

      if (KdDebugState & KD_DEBUG_SERIAL)
	    KdPortInitializeEx(&LogPortInfo, 0, 0);

      if (KdDebugState & KD_DEBUG_FILELOG)
	    DebugLogInit();
    }
}


VOID
KdInit1(VOID)
{
  /* Initialize kernel debugger (phase 0) */
  if ((KdDebuggerEnabled == TRUE) &&
      (KdDebugState & KD_DEBUG_GDB))
    {
      KdGdbStubInit(0);
    }
}


VOID KdInit2(VOID)
{
  /* Initialize kernel debugger (phase 1) */
  if ((KdDebuggerEnabled == TRUE) &&
      (KdDebugState & KD_DEBUG_GDB))
    {
      KdGdbStubInit(1);
    }
}

VOID
KdSerialDebugPrint (LPSTR Message)
{
  PCHAR pch = (PCHAR) Message;

  while (*pch != 0)
    {
      if (*pch == '\n')
        {
          KdPortPutByteEx (&LogPortInfo, '\r');
        }
        KdPortPutByteEx (&LogPortInfo, *pch);
        pch++;
    }
}


VOID
KdBochsDebugPrint(IN LPSTR  Message)
{
	while (*Message != 0)
	  {
	    if (*Message == '\n')
	      {
		WRITE_PORT_UCHAR((PUCHAR)BOCHS_LOGGER_PORT, '\r');
	      }
	    WRITE_PORT_UCHAR((PUCHAR)BOCHS_LOGGER_PORT, *Message);
	    Message++;
	  }
}


ULONG
KdpPrintString(PANSI_STRING String)
{
	PCH pch = String->Buffer;

	if (KdDebugState & KD_DEBUG_GDB)
		KdGdbDebugPrint(pch);

	if (KdDebugState & KD_DEBUG_SCREEN)
		HalDisplayString(pch);

	if (KdDebugState & KD_DEBUG_SERIAL)
		KdSerialDebugPrint(pch);

	if (KdDebugState & KD_DEBUG_BOCHS)
		KdBochsDebugPrint(pch);

	if (KdDebugState & KD_DEBUG_FILELOG)
		DebugLogWrite(pch);

	return((ULONG)String->Length);
}

/* PUBLIC FUNCTIONS *********************************************************/

/* NTOSKRNL.KdPollBreakIn */

BOOLEAN STDCALL
KdPollBreakIn(VOID)
{
  if ((!KdDebuggerEnabled) || (!(KdDebugState & KD_DEBUG_SERIAL)))
    return FALSE;
  return KdpBreakPending;
}

VOID STDCALL
KeEnterKernelDebugger(VOID)
{
  HalDisplayString("\n\n *** Entered kernel debugger ***\n");

  for (;;)
    __asm__("hlt\n\t");
}

VOID STDCALL
KdSystemDebugControl(ULONG Code)
{
  extern VOID PsDumpThreads(BOOLEAN IncludeSystem);

  /* A - Dump the entire contents of the non-paged pool. */
  if (Code == 0)
    {
      MiDebugDumpNonPagedPool(FALSE);
    }
  /* B - Bug check the system. */
  else if (Code == 1)
    {
      KeBugCheck(0);
    }
  /* 
   * C -  Dump statistics about the distribution of tagged blocks in 
   *      the non-paged pool.
   */
  else if (Code == 2)
    {
      MiDebugDumpNonPagedPoolStats(FALSE);
    }
  /* 
   * D - Dump the blocks created in the non-paged pool since the last
   * SysRq + D and SysRq + E command.
   */
  else if (Code == 3)
    {
      MiDebugDumpNonPagedPool(TRUE);
    }
  /* E - Dump statistics about the tags of newly created blocks. */
  else if (Code == 4)
    {
      MiDebugDumpNonPagedPoolStats(TRUE);
    }
  /* F */
  else if (Code == 5)
    {
      PsDumpThreads(TRUE);
    }
  /* G */
  else if (Code == 6)
    {
      PsDumpThreads(FALSE);
    }
  /* H */
  else if (Code == 7)
    {
    }
  /* I */
  else if (Code == 8)
    {
    }
  /* J */
  else if (Code == 9)
    {
    }
  /* K - Enter the system debugger. */
  else if (Code == 10)
    {
#ifdef KDBG
      KdbEnter();
#else /* KDBG */
      DbgPrint("No local kernel debugger\n");
#endif /* not KDBG */
    }
}


/* Support routines for the GDB stubs */

VOID
KdPutChar(UCHAR Value)
{
  KdPortPutByteEx (&GdbPortInfo, Value);
}


UCHAR
KdGetChar(VOID)
{
  UCHAR Value;

  while (!KdPortGetByteEx (&GdbPortInfo, &Value));

  return Value;
}

/* EOF */
