/* $Id: kdebug.c,v 1.31 2002/01/23 23:39:25 chorns Exp $
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


/* TYPEDEFS ****************************************************************/

#define ScreenDebug  (0x1)
#define SerialDebug  (0x2)
#define BochsDebug   (0x4)
#define FileLogDebug (0x8)

/* VARIABLES ***************************************************************/

BOOLEAN
__declspec(dllexport)
KdDebuggerEnabled = FALSE;		/* EXPORTED */

BOOLEAN
__declspec(dllexport)
KdDebuggerNotPresent = TRUE;		/* EXPORTED */


static BOOLEAN KdpBreakPending = FALSE;
static ULONG KdpDebugType = ScreenDebug | BochsDebug;
ULONG KdpPortIrq = 0;

/* PRIVATE FUNCTIONS ********************************************************/

static void
PrintString (char* fmt,...)
{
	char buffer[512];
	va_list ap;

	va_start(ap, fmt);
	vsprintf(buffer, fmt, ap);
	va_end(ap);

	HalDisplayString (buffer);
}


VOID
KdInitSystem (
	ULONG			Reserved,
	PLOADER_PARAMETER_BLOCK	LoaderBlock
	)
{
	KD_PORT_INFORMATION PortInfo;
	ULONG Value;
	PCHAR p1, p2;

#ifdef KDBG
  /* Initialize runtime debugging if available */
  DbgRDebugInit();
#endif

	/* set debug port default values */
	PortInfo.ComPort  = DEFAULT_DEBUG_PORT;
	PortInfo.BaudRate = DEFAULT_DEBUG_BAUD_RATE;
  KdpPortIrq        = DEFAULT_DEBUG_COM2_IRQ;

	/*
	 * parse kernel command line
	 */

	/* check for 'DEBUGPORT' */
	p1 = (PCHAR)LoaderBlock->CommandLine;
	while (p1 && (p2 = strchr (p1, '/')))
	{
		p2++;
		if (!_strnicmp (p2, "DEBUGPORT", 9))
		{
			p2 += 9;
			if (*p2 == '=')
                        {
				p2++;
				if (!_strnicmp (p2, "SCREEN", 6))
				{
					p2 += 6;
					KdDebuggerEnabled = TRUE;
					KdpDebugType |= ScreenDebug;
				}
				else if (!_strnicmp (p2, "BOCHS", 5))
				{
					p2 += 5;
					KdDebuggerEnabled = TRUE;
					KdpDebugType |= BochsDebug;
				}
				else if (!_strnicmp (p2, "COM", 3))
				{
					p2 += 3;
					Value = (ULONG)atol (p2);
					if (Value > 0 && Value < 5)
					{
						KdDebuggerEnabled = TRUE;
						KdpDebugType |= SerialDebug;
						PortInfo.ComPort = Value;
					}
				}
			}
		}
		else if (!_strnicmp (p2, "DEBUG", 5))
		{
			p2 += 5;
			KdDebuggerEnabled = TRUE;
			KdpDebugType = KdpDebugType | SerialDebug;
		}
		else if (!_strnicmp (p2, "NODEBUG", 7))
		{
			p2 += 7;
			KdDebuggerEnabled = FALSE;
		}
		else if (!_strnicmp (p2, "CRASHDEBUG", 10))
		{
			p2 += 10;
			KdDebuggerEnabled = FALSE;
		}
		else if (!_strnicmp (p2, "BREAK", 5))
		{
			p2 += 7;
			KdpBreakPending = TRUE;
		}
		else if (!_strnicmp (p2, "BAUDRATE", 8))
		{
			p2 += 8;
			if (*p2 != '=')
			{
				p2++;
				Value = (ULONG)atol (p2);
				if (Value > 0)
				{
					KdDebuggerEnabled = TRUE;
					KdpDebugType = KdpDebugType | SerialDebug;
					PortInfo.BaudRate = Value;
				}
			}
		else if (!_strnicmp (p2, "IRQ", 3))
			{
				p2 += 3;
				if (*p2 != '=')
				{
					p2++;
					Value = (ULONG)atol (p2);
					if (Value > 0)
					{
						KdDebuggerEnabled = TRUE;
						KdpDebugType = KdpDebugType | SerialDebug;
						KdpPortIrq = Value;
					}
        }
			}
		}
		p1 = p2;
	}
	
#ifdef DBGPRINT_FILE_LOG
	KdpDebugType |= FileLogDebug;
	DebugLogInit();
#endif /* DBGPRINT_FILE_LOG */

	/* print some information */
	if (KdDebuggerEnabled == TRUE)
	{
		if (KdpDebugType & ScreenDebug)
		{
			PrintString ("\n   Screen debugging enabled\n\n");
		}
		if (KdpDebugType & BochsDebug)
		{
			PrintString ("\n   Bochs debugging enabled\n\n");
		}
		if (KdpDebugType & SerialDebug)
		{
			PrintString ("\n   Serial debugging enabled: COM%ld %ld Baud\n\n",
			             PortInfo.ComPort, PortInfo.BaudRate);
		}
		if (KdpDebugType & FileLogDebug)
		  {
		    PrintString("\n   File log debugging enabled\n\n");
		  }
	}
	else
		PrintString ("\n   Debugging disabled\n\n");


	/* initialize debug port */
	if (KdDebuggerEnabled && (KdpDebugType & SerialDebug))
	{
		KdPortInitialize (&PortInfo,
		                  0,
		                  0);
	}
}

VOID KdInit1()
{
#ifndef SERDUMP

	/* Initialize kernel debugger */
	if (KdDebuggerEnabled && (KdpDebugType & SerialDebug))
	{
    KdGdbStubInit(0);
	}

#endif /* !SERDUMP */
}

VOID KdInit2()
{
#ifndef SERDUMP

	/* Initialize kernel debugger */
	if (KdDebuggerEnabled && (KdpDebugType & SerialDebug))
	{
    KdGdbStubInit(1);
	}

#endif /* !SERDUMP */
}

ULONG KdpPrintString (PANSI_STRING String)
{
   PCH pch = String->Buffer;

   if (KdpDebugType & ScreenDebug)
     {
	HalDisplayString (String->Buffer);
     }
   if (KdpDebugType & SerialDebug)
     KdDebugPrint (pch);
   if (KdpDebugType & BochsDebug)
     {
	pch = String->Buffer;
	while (*pch != 0)
	  {
	     if (*pch == '\n')
	       {
		  WRITE_PORT_UCHAR((PUCHAR)BOCHS_LOGGER_PORT, '\r');
	       }
	     WRITE_PORT_UCHAR((PUCHAR)BOCHS_LOGGER_PORT, *pch);
	     pch++;
	  }
     }
#ifdef DBGPRINT_FILE_LOG
   if (KdpDebugType & FileLogDebug)
     {
       DebugLogWrite(String->Buffer);
     }
#endif /* DBGPRINT_FILE_LOG */
   return (ULONG)String->Length;
}

/* PUBLIC FUNCTIONS *********************************************************/

/* NTOSKRNL.KdPollBreakIn */

BOOLEAN
STDCALL
KdPollBreakIn (
	VOID
	)
{
#ifndef SERDUMP

	if (!KdDebuggerEnabled || !(KdpDebugType & SerialDebug))
		return FALSE;

  return TRUE;

#else /* SERDUMP */

  return FALSE;

#endif /* !SERDUMP */
}

VOID STDCALL
KeEnterKernelDebugger (VOID)
{
	HalDisplayString ("\n\n *** Entered kernel debugger ***\n");

#if 1
	for (;;)
		__asm__("hlt\n\t");
#else
   for(;;);
#endif
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
  KdPortPutByte (Value);
}


UCHAR
KdGetChar()
{
  UCHAR Value;

  while (!KdPortGetByte (&Value));

  return Value;
}

/* EOF */
